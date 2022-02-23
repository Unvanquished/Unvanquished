/*
===========================================================================

Copyright 2016 Unvanquished Developers

This file is part of Unvanquished.

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "cg_local.h"

namespace CombatFeedback {

static Log::Logger logger("cgame.cf");

const float DAMAGE_INDICATOR_ASPECT_RATIO = 982.0f / 2048.0f;
const float DAMAGE_INDICATOR_BASE_SCALE = 0.7f;
const float DAMAGE_INDICATOR_DISTANCE_EXPONENT = -0.5f;     

Cvar::Cvar<bool> damageIndicators_enable(
	"cgame.damageIndicators.enable",
	"enable/disable damage indicators",
	Cvar::NONE, true);

Cvar::Cvar<float> damageIndicators_scale(
	"cgame.damageIndicators.scale",
	"scale of damage indicators",
	Cvar::NONE, 1.0f);

Cvar::Cvar<bool> killSounds_enable(
	"cgame.killSounds.enable",
	"enable/disable kill sounds",
	Cvar::NONE, true);

enum damageIndicatorLayer_t {
	DIL_ENEMY,
	DIL_TEAMMATE,
	DIL_ALIEN_BUILDING,
	DIL_HUMAN_BUILDING,

	DAMAGE_INDICATOR_LAYERS
};

Color::Color damageIndicatorColors[DAMAGE_INDICATOR_LAYERS] = {
	{1, 1, 1},  // enemy
	{1, 0.5, 0}, // friendly
	{0.54, 1, 0.68}, // alien building
	{0, 0.82, 1} // human building
};

struct DamageIndicator {
	int ctime, victim;
	float value;
	Vec3 origin, velocity;
	damageIndicatorLayer_t layer;
	bool lethal;

	// the following fields are computed every frame and can be ignored
	// in all init/clustering code
	vec2_t pos2d;
	float dist, scale;
	Color::Color color;
	float alpha;
};

class Clustering : public ::Clustering::EuclideanClustering<DamageIndicator*, 3> {
	using super = Clustering::EuclideanClustering<DamageIndicator*, 3>;

public:
	Clustering(float laxity = 2.0f, std::function<bool(DamageIndicator*,
	           DamageIndicator*)> visibilityTest = nullptr)
	: super(laxity, visibilityTest)
	{
	}

	void Update(DamageIndicator *di)
	{
		super::Update(di, di->origin);
	}
};

static std::list<DamageIndicator> damageIndicators;
static std::list<DamageIndicator> damageIndicatorQueue;

/**
 * @brief Create a new damage indicator and add it to the queue.
 */
static void EnqueueDamageIndicator(Vec3 point, int flags, float value, int victim)
{
	DamageIndicator di;

	di.ctime = cg.time;
	di.origin = point;
	di.value = value;
	di.victim = victim;

	if (flags & HIT_BUILDING) {
		bool alien = (CG_MyTeam() == TEAM_ALIENS) ^ !(flags & HIT_FRIENDLY);

		if (alien)
			di.layer = DIL_ALIEN_BUILDING;
		else
			di.layer = DIL_HUMAN_BUILDING;
	} else {
		if (flags & HIT_FRIENDLY)
			di.layer = DIL_TEAMMATE;
		else
			di.layer = DIL_ENEMY;
	}

	// there will always be only one HIT_LETHAL damage indicator
	// and it'll be always the last one, so it doesn't need its own layer
	di.lethal = (flags & HIT_LETHAL) == HIT_LETHAL;

	// FIXME: avoid copying DamageIndicators
	damageIndicatorQueue.push_back(di);
}

static bool DamageIndicatorsSameKind(DamageIndicator *A, DamageIndicator *B)
{
	return (A->layer == B->layer) && (A->victim == B->victim);
}

/**
 * @brief Finally spawn a damage indicator that will be drawn.
 */
static void AddDamageIndicator(DamageIndicator di)
{
	// combine older indicators of the same type if they're close enough
	for (auto i = damageIndicators.begin(); i != damageIndicators.end(); ) {
		if (DamageIndicatorsSameKind(&*i, &di) &&
		    di.ctime - i->ctime < 300 &&
		    Distance(di.origin, i->origin) < 200) {
			di.value += i->value;
			i = damageIndicators.erase(i);
		}
		else
			i++;
	}

	di.color = damageIndicatorColors[di.layer];
	di.velocity = Vec3(crandom() * 20, crandom() * 20, 100);

	damageIndicators.push_back(di);
}

/**
 * @brief The slow path of DequeueDamageIndicators.
 */
static void DequeueDamageIndicators2(void)
{
	Clustering clustering(2.0f, DamageIndicatorsSameKind);

	for (auto &di : damageIndicatorQueue)
		clustering.Update(&di);

	for (auto cluster : clustering) {
		const auto &center = cluster.GetCenter();
		const DamageIndicator *mean = cluster.GetMeanObject();
		DamageIndicator total;

		total.ctime = cg.time;
		total.origin = center;
		total.layer = mean->layer;
		total.victim = mean->victim;
		// velocity is set in AddDamageIndicator

		total.value = 0;
		total.lethal = false;

		for (auto record : cluster) {
			total.value += record.first->value;

			if (record.first->lethal)
				total.lethal = true;
		}

		AddDamageIndicator(total);
	}

	damageIndicatorQueue.clear();
}

/**
 * @brief Move indicators from the queue, merging similar indicators.
 */
static void DequeueDamageIndicators(void)
{
	// most of the time there will be only one or no indicators per frame
	// so these fast paths will save a lot of time
	if (damageIndicatorQueue.empty())
		return;

	if (damageIndicatorQueue.size() == 1) {
		AddDamageIndicator(damageIndicatorQueue.front());
		damageIndicatorQueue.pop_front();
		return;
	}

	// compute clusterings if there's more than one indicator
	DequeueDamageIndicators2();
}

/**
 * @brief Called when an EV_COMBAT_FEEDBACK event arrives.
 */
void Event(entityState_t *es)
{
	int flags, victim;
	float value;

	flags = es->otherEntityNum2;
	value = es->angles2[0];
	victim = es->otherEntityNum;

	if (damageIndicators_enable.Get())
	{
		Vec3 origin = Vec3::Load(es->origin);
		EnqueueDamageIndicator(origin, flags, value, victim);
	}

	if ( (flags & HIT_LETHAL) && !(flags & HIT_BUILDING) && killSounds_enable.Get() )
	{
		trap_S_StartLocalSound(cgs.media.killSound, soundChannel_t::CHAN_LOCAL_SOUND);
	}

	cg.hitTime = cg.time;
}

/**
 * @brief Update a damage indicator (animations, dynamics, etc.)
 * @return False if the damage indicator expired and should be erased.
 * @param draw Set to false if the damage indicator is off screen and shouldn't
 *             be drawn
 */
static bool EvaluateDamageIndicator(DamageIndicator *di, bool *draw)
{
	float dt = 0.001f * cg.frametime;
	float t_fade, tmp[2];

	if (di->ctime + 1000 <= cg.time)
		return false;

	di->velocity += Vec3(0, 0, -400) * dt;
	di->origin += di->velocity * dt;

	if (!CG_WorldToScreen(di->origin.Data(), tmp, tmp + 1)) {
		*draw = false;
		return true;
	}

	// CG_WorldToScreen returns a point on the virtual 640x480 screen
	di->pos2d[ 0 ] = tmp[ 0 ] * cgs.screenXScale;
	di->pos2d[ 1 ] = tmp[ 1 ] * cgs.screenYScale;
	*draw = true;

	t_fade = (float)(cg.time - di->ctime) / 1000;
	di->alpha = pow(1 - t_fade, 2);

	di->dist = Distance(Vec3::Load(cg.refdef.vieworg), di->origin);
	di->scale = damageIndicators_scale.Get();
	di->scale *= pow(di->dist, DAMAGE_INDICATOR_DISTANCE_EXPONENT);

	return true;
}

/**
 * @brief Draw a damage indicator.
 */
static void DrawDamageIndicator(DamageIndicator *di)
{
	float width, height, total_width, x, y;
	std::string text;

	height = std::min(cgs.glconfig.vidWidth, cgs.glconfig.vidHeight);
	height *= di->scale * DAMAGE_INDICATOR_BASE_SCALE;
	width = height * DAMAGE_INDICATOR_ASPECT_RATIO;

	text = std::to_string((int)ceilf(di->value));
	if (di->lethal)
		text = "!" + text;
	total_width = width * text.length();

	x = di->pos2d[ 0 ] - total_width / 2;
	y = di->pos2d[ 1 ] - height / 2;

	for (size_t i = 0; i < text.length(); i++) {
		int glyph;
		vec2_t st0, st1;

		if (text[i] >= '0' && text[i] <= '9')
			glyph = text[i] - '0';
		else
			glyph = 10;

		st0[ 0 ] = ( glyph % 4 ) / 4.f;
		st0[ 1 ] = ( glyph / 4 ) / 4.f;
		st1[ 0 ] = st0[ 0 ] + 0.25f;
		st1[ 1 ] = st0[ 1 ] + 0.25f;

		trap_R_SetColor( di->color );
		trap_R_DrawStretchPic(x, y, width, height,
		                      st0[ 0 ], st0[ 1 ], st1[ 0 ], st1[ 1 ],
		                      cgs.media.damageIndicatorFont);

		x += width;
	}
}

static bool CompareDamageIndicators(DamageIndicator *A, DamageIndicator *B)
{
	return A->dist < B->dist;
}

/**
 * @brief Evaluate and draw all damage indicators.
 */
void DrawDamageIndicators(void)
{
	std::vector<DamageIndicator*> drawList;

	DequeueDamageIndicators();

	if (!damageIndicators_enable.Get()) {
		if (!damageIndicators.empty())
			damageIndicators.erase(damageIndicators.begin(),
		                               damageIndicators.end());
		return;
	}

	for (auto i = damageIndicators.begin(); i != damageIndicators.end(); ) {
		bool draw;

		if (!EvaluateDamageIndicator(&*i, &draw)) {
			i = damageIndicators.erase(i);
			continue;
		}

		if (draw)
			drawList.push_back(&*i);

		i++;
	}

	std::sort(drawList.begin(), drawList.end(), CompareDamageIndicators);

	for (auto i: drawList)
		DrawDamageIndicator(i);

	trap_R_SetColor(Color::White);
}

} // namespace CombatFeedback
