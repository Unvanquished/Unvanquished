#include "SpikerComponent.h"

static Log::Logger logger("sgame.spiker");

/** Delay between shots. */
constexpr int   COOLDOWN = 5000;
/** Shoot prematurely whenever this much damage is expected. */
constexpr float DAMAGE_THRESHOLD = 60.0f;
/** Total number of missiles to shoot. Some might be skipped if they endanger allies.
 *  Actual values is +/- SPIKER_MISSILEROWS. */
constexpr float MISSILES = 26;
/** Number of rows from which to launch spikes. */
constexpr int   MISSILEROWS = 4;
/** 0.0: Spikes are shot upwards, 1.0: Spikes are shot sideways. */
constexpr float ROWOFFSET = 0.5f;
/** An estimate on how far away spikes are still relevant, used to upper bound computations. */
constexpr float SPIKE_RANGE = 500.0f;
/** Bounding box scaling factor Used to improve friendly fire prevention. */
constexpr float SAFETY_TRACE_INFLATION = 3.0f;

SpikerComponent::SpikerComponent(
	Entity &entity, AlienBuildableComponent &r_AlienBuildableComponent)
	: SpikerComponentBase(entity, r_AlienBuildableComponent)
	, activeThinker(AT_NONE)
	, restUntil(0)
	, lastExpectedDamage(0.0f)
	, lastSensing(false) {
	RegisterSlowThinker();
}

void SpikerComponent::HandleDamage(float amount, gentity_t *source, Util::optional<Vec3> location,
                                   Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	if (!GetAlienBuildableComponent().GetBuildableComponent().Active()) {
		return;
	}

	// Shoot if there is a viable target.
	if (lastExpectedDamage > 0.0f) {
		logger.Verbose("Spiker #%i was hurt while an enemy is close enough to also get hurt, so "
			"go eye for an eye.", entity.oldEnt->s.number);

		Fire();
	}
}

void SpikerComponent::Think(int timeDelta) {
	// Don't act if recovering from shot or disabled.
	if (!GetAlienBuildableComponent().GetBuildableComponent().Active() || level.time < restUntil) {
		lastExpectedDamage = 0.0f;
		lastSensing = false;

		return;
	}

	float expectedDamage = 0.0f;
	bool  sensing = false;

	// Calculate expected damage to decide on the best moment to shoot.
	ForEntities<HealthComponent>([&](Entity& other, HealthComponent& healthComponent) {
		// TODO: Check if "entity == other" does the job.
		if (entity.oldEnt == other.oldEnt)                                return;
		if (G_Team(other.oldEnt) == TEAM_NONE)                            return;
		if (G_OnSameTeam(entity.oldEnt, other.oldEnt))                    return;
		if ((other.oldEnt->flags & FL_NOTARGET))                          return;
		if (!healthComponent.Alive())                                     return;
		if (G_Distance(entity.oldEnt, other.oldEnt) > SPIKE_RANGE)        return;
		if (other.Get<BuildableComponent>())                              return;
		if (!G_LineOfSight(entity.oldEnt, other.oldEnt))                  return;

		Vec3 dorsal    = Vec3::Load(entity.oldEnt->s.origin2);
		Vec3 toTarget  = Vec3::Load(other.oldEnt->s.origin) - Vec3::Load(entity.oldEnt->s.origin);
		Vec3 otherMins = Vec3::Load(other.oldEnt->r.mins);
		Vec3 otherMaxs = Vec3::Load(other.oldEnt->r.maxs);

		// Only entities in the spiker's upper hemisphere can be hit.
		if (Math::Dot(toTarget, dorsal) < 0.0f) return;

		// Approximate average damage the entity would receive from spikes.
		const missileAttributes_t* ma = BG_Missile(MIS_SPIKER);
		float spikeDamage  = ma->damage;
		float distance     = Math::Length(toTarget);
		float bboxDiameter = Math::Length(otherMins) + Math::Length(otherMaxs);
		float bboxEdge     = 0.57735026918962584f * bboxDiameter; // diameter = sqrt(3) * edge
		float hitEdge      = bboxEdge + (0.57735026918962584f * ma->size); // Add half missile edge.
		float hitArea      = hitEdge * hitEdge; // Approximate area resulting in a hit.
		float effectArea   = 2.0f * M_PI * distance * distance; // Area of a half sphere.
		float damage       = (hitArea / effectArea) * (float)MISSILES * spikeDamage;

		// Sum up expected damage for all targets, regardless of whether they are in sense range.
		expectedDamage += damage;

		// Start sensing (frequent search for best moment to shoot) as soon as an enemy that can be
		// damaged is close enough. Note that the Spiker will shoot eventually after it started
		// sensing, and then returns to a less alert state.
		if (distance < SPIKER_SENSE_RANGE && !sensing) {
			sensing = true;

			if (!lastSensing) {
				logger.Verbose("Spiker #%i now senses an enemy and will check more frequently for "
					"the best moment to shoot.", entity.oldEnt->s.number);

				RegisterFastThinker();
			}
		}
	});

	bool senseLost = lastSensing && !sensing;

	if (sensing || senseLost) {
		bool lessDamage = (expectedDamage <= lastExpectedDamage);
		bool enoughDamage = (expectedDamage >= DAMAGE_THRESHOLD);

		if (sensing) {
			logger.Verbose("Spiker #%i senses an enemy and expects to do %.1f damage.%s%s",
				entity.oldEnt->s.number, expectedDamage, (lessDamage && !enoughDamage)
				? " This has not increased, so it's time to shoot." : "", enoughDamage ?
				" This is already enough, shoot now." : "");
		}

		if (senseLost) {
			logger.Verbose("Spiker #%i lost track of all enemies after expecting to do %.1f damage."
				" This makes the spiker angry, so it will shoot anyway.",
				entity.oldEnt->s.number, lastExpectedDamage);
		}

		// Shoot when
		// - a threshold was reached by the expected damage, implying a very close enemy,
		// - the expected damage has decreased, witnessing a recent local maximum, or
		// - whenever all viable targets have left the sense range.
		// The first trigger plays around the delay in sensing a local maximum and in having the
		// spikes travel towards their destination.
		// The last trigger guarantees that the spiker always shoots eventually after sensing.
		if (enoughDamage || (sensing && lessDamage) || senseLost) {
			Fire();
		}
	}

	lastExpectedDamage = expectedDamage;
	lastSensing = sensing;
}

// TODO: Use a proper trajectory trace, once available, to ensure friendly buildables are never hit.
bool SpikerComponent::SafeToShoot(Vec3 direction) {
	const missileAttributes_t* ma = BG_Missile(MIS_SPIKER);
	float missileSize = (float)ma->size;
	trace_t trace;
	vec3_t mins, maxs;
	Vec3 end = Vec3::Load(entity.oldEnt->s.origin) + (SPIKE_RANGE * direction);

	// Test once with normal and once with inflated missile bounding box.
	for (float traceSize : {missileSize, missileSize * SAFETY_TRACE_INFLATION}) {
		mins[0] = mins[1] = mins[2] = -traceSize;
		maxs[0] = maxs[1] = maxs[2] =  traceSize;
		trap_Trace(&trace, entity.oldEnt->s.origin, mins, maxs, end.Data(), entity.oldEnt->s.number,
			ma->clipmask, 0);
		gentity_t* hit = &g_entities[trace.entityNum];

		if (hit && G_OnSameTeam(entity.oldEnt, hit)) {
			return false;
		}
	}

	return true;
}

bool SpikerComponent::Fire() {
	gentity_t *self = entity.oldEnt;
	// Check if still resting.
	if (restUntil > level.time) {
		logger.Verbose("Spiker #%i wanted to fire but wasn't ready.", entity.oldEnt->s.number);

		return false;
	} else {
		logger.Verbose("Spiker #%i is firing!", entity.oldEnt->s.number);
	}

	// Play shooting animation.
	G_SetBuildableAnim(self, BANIM_ATTACK1, false);

	// TODO: Add a particle effect.
	//G_AddEvent(self, EV_ALIEN_SPIKER, DirToByte(self->s.origin2));

	// Calculate total perimeter of all spike rows to allow for a more even spike distribution.
	float totalPerimeter = 0.0f;

	for (int row = 0; row < MISSILEROWS; row++) {
		float altitude = (((float)row + ROWOFFSET) * M_PI_2) / (float)MISSILEROWS;
		float perimeter = 2.0f * M_PI * cos(altitude);

		totalPerimeter += perimeter;
	}

	// Distribute and launch missiles.
	// TODO: Use new vector library.
	vec3_t dir, rowBase, zenith, rotAxis;

	VectorCopy(self->s.origin2, zenith);
	PerpendicularVector(rotAxis, zenith);

	for (int row = 0; row < MISSILEROWS; row++) {
		float altitude = (((float)row + ROWOFFSET) * M_PI_2) / (float)MISSILEROWS;
		float perimeter = 2.0f * M_PI * cos(altitude);

		RotatePointAroundVector(rowBase, rotAxis, zenith, RAD2DEG(M_PI_2 - altitude));

		// Attempt to distribute spikes with equal distance on all rows.
		int spikes = (int)round(((float)MISSILES * perimeter) / totalPerimeter);

		for (int spike = 0; spike < spikes; spike++) {
			float azimuth = 2.0f * M_PI * (((float)spike + 0.5f * crandom()) / (float)spikes);
			float altitudeVariance = 0.5f * crandom() * M_PI_2 / (float)MISSILEROWS;

			RotatePointAroundVector(dir, zenith, rowBase, RAD2DEG(azimuth));
			RotatePointAroundVector(dir, rotAxis, dir, RAD2DEG(altitudeVariance));

			// Trace in the shooting direction and do not shoot spikes that are likely to harm
			// friendly entities.
			bool fire = SafeToShoot(Vec3::Load(dir));

			logger.Debug("Spiker #%d %s: Row %d/%d: Spike %2d/%2d: "
				"( Alt %2.0f°, Az %3.0f° → %.2f, %.2f, %.2f )", self->s.number,
				fire ? "fires" : "skips", row + 1, MISSILEROWS, spike + 1, spikes,
				RAD2DEG(altitude + altitudeVariance), RAD2DEG(azimuth), dir[0], dir[1], dir[2]);

			if (!fire) {
				continue;
			}

			G_SpawnMissile(
				MIS_SPIKER, self, self->s.origin, dir, nullptr, G_FreeEntity,
				level.time + (int)(1000.0f * SPIKE_RANGE / (float)BG_Missile(MIS_SPIKER)->speed));
		}
	}

	restUntil = level.time + COOLDOWN;
	RegisterSlowThinker();

	return true;
}

void SpikerComponent::RegisterFastThinker() {
	if (activeThinker == AT_FAST) {
		return;
	}

	GetAlienBuildableComponent().GetBuildableComponent().GetThinkingComponent().UnregisterActiveThinker();
	GetAlienBuildableComponent().GetBuildableComponent().REGISTER_THINKER(
		Think, ThinkingComponent::SCHEDULER_AVERAGE, 50);
}

void SpikerComponent::RegisterSlowThinker() {
	if (activeThinker == AT_SLOW) {
		return;
	}

	GetAlienBuildableComponent().GetBuildableComponent().GetThinkingComponent().UnregisterActiveThinker();
	GetAlienBuildableComponent().GetBuildableComponent().REGISTER_THINKER(
		Think, ThinkingComponent::SCHEDULER_AVERAGE, 500);
}
