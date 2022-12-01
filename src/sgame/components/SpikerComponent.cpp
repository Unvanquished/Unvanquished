#include "SpikerComponent.h"

#include <glm/geometric.hpp>
#include "../Entities.h"

static Log::Logger logger("sgame.spiker");

/** Delay between shots. */
constexpr int   COOLDOWN = 5000;
/** Shoot prematurely whenever this much damage is expected. */
constexpr float DAMAGE_THRESHOLD = 60.0f;
/** Total number of missiles to shoot. Some might be skipped if they endanger allies.
 *  Actual values is +/- MISSILEROWS. */
constexpr int   MISSILES = 26;
/** Number of rows from which to launch spikes. */
constexpr int   MISSILEROWS = 4;
/** An estimate on how far away spikes are still relevant, used to upper bound computations. */
constexpr float SPIKE_RANGE = 500.0f;
/** Bounding box scaling factor Used to improve friendly fire prevention. */
constexpr float SAFETY_TRACE_INFLATION = 3.0f;
/** Maximum angle added to or substracted rom the 180° radius of damage, in positive degrees. */
constexpr float GRAVITY_COMPENSATION_ANGLE = 25.0f;

SpikerComponent::SpikerComponent(
	Entity &entity, AlienBuildableComponent &r_AlienBuildableComponent)
	: SpikerComponentBase(entity, r_AlienBuildableComponent)
	, activeThinker(AT_NONE)
	, restUntil(0)
	, lastExpectedDamage(0.0f)
	, lastSensing(false)
	, gravityCompensation(0) {
	SetGravityCompensation();
	RegisterSlowThinker();
}

void SpikerComponent::HandleDamage(float /*amount*/, gentity_t* /*source*/, Util::optional<glm::vec3> /*location*/,
                                   Util::optional<glm::vec3> /*direction*/, int /*flags*/, meansOfDeath_t /*meansOfDeath*/) {
	if (!GetAlienBuildableComponent().GetBuildableComponent().Active()) {
		return;
	}

	// Shoot if there is a viable target.
	if (lastExpectedDamage > 0.0f) {
		logger.Verbose("Spiker #%i was hurt while an enemy is close enough to also get hurt, so "
			"go eye for an eye.", entity.oldEnt->num());

		Fire();
	}
}

void SpikerComponent::SetGravityCompensation() {
	glm::vec3 dorsal = glm::normalize( VEC2GLM( entity.oldEnt->s.origin2  ));
	glm::vec3 upwards = { 0.0f, 0.0f, 1.0f };
	float uprightness = glm::dot( dorsal, upwards );
	// A negative value means the Spiker's radius of damage increases.
	gravityCompensation = -uprightness * sinf(DEG2RAD(0.5f * GRAVITY_COMPENSATION_ANGLE));
}

void SpikerComponent::Think(int /*timeDelta*/) {
	// Don't act if recovering from shot or disabled.
	if (!GetAlienBuildableComponent().GetBuildableComponent().Active() || level.time < restUntil) {
		lastExpectedDamage = 0.0f;
		lastSensing = false;

		return;
	}

	float expectedDamage = 0.0f;
	bool  sensing = false;

	// Calculate expected damage to decide on the best moment to shoot.
	for (Entity& other : Entities::Having<HealthComponent>()) {
		if (G_Team(other.oldEnt) == TEAM_NONE)                            continue;
		if (G_OnSameTeam(entity.oldEnt, other.oldEnt))                    continue;
		if ((other.oldEnt->flags & FL_NOTARGET))                          continue;
		if (!other.Get<HealthComponent>()->Alive())                       continue;
		if (G_Distance(entity.oldEnt, other.oldEnt) > SPIKE_RANGE)        continue;
		if (other.Get<BuildableComponent>())                              continue;
		if (!G_LineOfSight(entity.oldEnt, other.oldEnt))                  continue;

		glm::vec3 dorsal    = VEC2GLM( entity.oldEnt->s.origin2 );
		glm::vec3 toTarget  = VEC2GLM( other.oldEnt->s.origin ) - VEC2GLM( entity.oldEnt->s.origin );
		glm::vec3 otherMins = VEC2GLM( other.oldEnt->r.mins );
		glm::vec3 otherMaxs = VEC2GLM( other.oldEnt->r.maxs );

		// With a straight shot, only entities in the spiker's upper hemisphere can be hit.
		// Since the spikes obey gravity, increase or decrease this radius of damage by up to
		// GRAVITY_COMPENSATION_ANGLE degrees depending on the spiker's orientation.
		if (glm::dot( glm::normalize( toTarget  ), dorsal) < gravityCompensation) continue;

		// Approximate average damage the entity would receive from spikes.
		const missileAttributes_t* ma = BG_Missile(MIS_SPIKER);
		float spikeDamage  = ma->damage;
		float distance     = glm::length( toTarget );
		float bboxDiameter = glm::length( otherMins ) + glm::length( otherMaxs );
		float bboxEdge     = (1.0f / M_ROOT3) * bboxDiameter; // Assumes a cube.
		float hitEdge      = bboxEdge + ((1.0f / M_ROOT3) * ma->size); // Add half missile edge.
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
					"the best moment to shoot.", entity.oldEnt->num());

				RegisterFastThinker();
			}
		}
	}

	bool senseLost = lastSensing && !sensing;

	if (sensing || senseLost) {
		bool lessDamage = (expectedDamage <= lastExpectedDamage);
		bool enoughDamage = (expectedDamage >= DAMAGE_THRESHOLD);

		if (sensing) {
			logger.Verbose("Spiker #%i senses an enemy and expects to do %.1f damage.%s%s",
				entity.oldEnt->num(), expectedDamage, (lessDamage && !enoughDamage)
				? " This has not increased, so it's time to shoot." : "", enoughDamage ?
				" This is already enough, shoot now." : "");
		}

		if (senseLost) {
			logger.Verbose("Spiker #%i lost track of all enemies after expecting to do %.1f damage."
				" This makes the spiker angry, so it will shoot anyway.",
				entity.oldEnt->num(), lastExpectedDamage);
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
bool SpikerComponent::SafeToShoot(glm::vec3 direction) {
	const missileAttributes_t* ma = BG_Missile(MIS_SPIKER);
	float missileSize = (float)ma->size;
	trace_t trace;
	vec3_t mins, maxs;
	glm::vec3 end = VEC2GLM( entity.oldEnt->s.origin ) + (SPIKE_RANGE * direction);

	// Test once with normal and once with inflated missile bounding box.
	for (float traceSize : {missileSize, missileSize * SAFETY_TRACE_INFLATION}) {
		mins[0] = mins[1] = mins[2] = -traceSize;
		maxs[0] = maxs[1] = maxs[2] =  traceSize;
		trap_Trace( &trace, entity.oldEnt->s.origin, mins, maxs, &end[0], entity.oldEnt->num(), ma->clipmask, 0 );
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
		logger.Verbose("Spiker #%i wanted to fire but wasn't ready.", entity.oldEnt->num());

		return false;
	} else {
		logger.Verbose("Spiker #%i is firing!", entity.oldEnt->num());
	}

	// Play shooting animation.
	G_SetBuildableAnim(self, BANIM_ATTACK1, false);
	GetBuildableComponent().ProtectAnimation(5000);

	// TODO: Add a particle effect.
	//G_AddEvent(self, EV_ALIEN_SPIKER, DirToByte(self->s.origin2));

	// Calculate total perimeter of all spike rows to allow for a more even spike distribution.
	// A "row" is a group of missile launch directions with a common base altitude (angle measured
	// from the Spiker's horizon to its zenith) which is slightly adjusted for each new missile in
	// the row (at most halfway to the base altitude of a neighbouring row).
	float totalPerimeter = 0.0f;
	for (int row = 0; row < MISSILEROWS; row++) {
		float rowAltitude = (((float)row + 0.5f) * M_PI_2) / (float)MISSILEROWS;
		float rowPerimeter = 2.0f * M_PI * cosf(rowAltitude);
		totalPerimeter += rowPerimeter;
	}

	// TODO: Use new vector library.
	vec3_t dir, zenith, rotAxis;

	// As rotation axis for setting the altitude, any vector perpendicular to the zenith works.
	VectorCopy(self->s.origin2, zenith);
	PerpendicularVector(rotAxis, zenith);

	// Distribute and launch missiles.
	for (int row = 0; row < MISSILEROWS; row++) {
		// Set the base altitude and get the perimeter for the current row.
		float rowAltitude = (((float)row + 0.5f) * M_PI_2) / (float)MISSILEROWS;
		float rowPerimeter = 2.0f * M_PI * cosf(rowAltitude);

		// Attempt to distribute spikes with equal expected angular distance on all rows.
		int spikes = (int)round(((float)MISSILES * rowPerimeter) / totalPerimeter);

		// Launch missiles in the current row.
		for (int spike = 0; spike < spikes; spike++) {
			float spikeAltitude = rowAltitude + (0.5f * crandom() * M_PI_2 / (float)MISSILEROWS);
			float spikeAzimuth = 2.0f * M_PI * (((float)spike + 0.5f * crandom()) / (float)spikes);

			// Set launch direction altitude.
			RotatePointAroundVector(dir, rotAxis, zenith, RAD2DEG(M_PI_2 - spikeAltitude));

			// Set launch direction azimuth.
			RotatePointAroundVector(dir, zenith, dir, RAD2DEG(spikeAzimuth));

			// Trace in the shooting direction and do not shoot spikes that are likely to harm
			// friendly entities.
			bool fire = SafeToShoot(VEC2GLM( dir ));

			logger.Debug("Spiker #%d %s: Row %d/%d: Spike %2d/%2d: "
				"( Alt %2.0f°, Az %3.0f° → %.2f, %.2f, %.2f )", self->num(),
				fire ? "fires" : "skips", row + 1, MISSILEROWS, spike + 1, spikes,
				RAD2DEG(spikeAltitude), RAD2DEG(spikeAzimuth), dir[0], dir[1], dir[2]);

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
