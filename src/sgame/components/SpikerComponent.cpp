#include "SpikerComponent.h"

static Log::Logger logger("sgame.spiker");

constexpr float SAFETY_TRACE_FUDGE_FACTOR = 3.0f;

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
		if (G_Distance(entity.oldEnt, other.oldEnt) > SPIKER_SPIKE_RANGE) return;
		if (other.Get<BuildableComponent>())                              return;
		if (!G_LineOfSight(entity.oldEnt, other.oldEnt))                  return;

		// TODO: Use new vector facility.
		vec3_t vecToTarget;
		VectorSubtract(other.oldEnt->s.origin, entity.oldEnt->s.origin, vecToTarget);

		// Only entities in the spiker's upper hemisphere can be hit.
		if (DotProduct(entity.oldEnt->s.origin2, vecToTarget) < 0) return;

		// Approximate average damage the entity would receive from spikes.
		float diameter = VectorLength(other.oldEnt->r.mins) + VectorLength(other.oldEnt->r.maxs);
		float distance = VectorLength(vecToTarget);
		float effectArea = 2.0f * M_PI * distance * distance; // Half sphere.
		float targetArea = 0.5f * diameter * diameter;  // Approx. proj. of target on effect area.
		float damage = (targetArea / effectArea) * (float)SPIKER_MISSILES *
			(float)BG_Missile(MIS_SPIKER)->damage;

		expectedDamage += damage;

		if (distance < SPIKER_SENSE_RANGE) {
			sensing = true;

			if (!lastSensing) {
				logger.Verbose("Spiker #%i now senses an enemy and will check more frequently for "
					"the best moment to shoot.", entity.oldEnt->s.number);

				RegisterFastThinker();
			}
		}
	});

	// Shoot if a viable target leaves sense range even if no damage is expected anymore. This
	// guarantees that the spiker always shoots eventually when an enemy gets close enough to it.
	bool senseLost = lastSensing && !sensing;

	if (sensing || senseLost) {
		bool lessDamage = (expectedDamage <= lastExpectedDamage);

		if (sensing) {
			logger.Verbose("%i: Spiker #%i senses an enemy and expects to do %.1f damage.%s",
				level.time, entity.oldEnt->s.number, expectedDamage, lessDamage
				? " This has not increased, so it's time to shoot." : "");
		}

		if (senseLost) {
			logger.Verbose("Spiker #%i lost track of all enemies after expecting to do %.1f damage."
				" This makes the spiker angry, so it will shoot anyway.",
				entity.oldEnt->s.number, lastExpectedDamage);
		}

		if ((sensing && lessDamage) || senseLost) {
			Fire();
		}
	}

	lastExpectedDamage = expectedDamage;
	lastSensing = sensing;
}

// TODO: Use a proper trajectory trace, once available, to ensure friendly buildables are never hit.
bool SpikerComponent::SafeToShoot(Vec3 direction) {
	const missileAttributes_t* ma = BG_Missile(MIS_SPIKER);
	trace_t trace;
	vec3_t mins, maxs;
	Vec3 end = Vec3::Load(entity.oldEnt->s.origin) + (SPIKER_SPIKE_RANGE * direction);

	// Test once with normal and once with inflated missile bounding box.
	for (int i = 0; i <= 1; i++) {
		float traceSize = (float)ma->size * ((1.0f-(float)i) + (float)i*SAFETY_TRACE_FUDGE_FACTOR);
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
		return false;
	}

	// Play shooting animation.
	G_SetBuildableAnim(self, BANIM_ATTACK1, false);

	// TODO: Add a particle effect.
	//G_AddEvent(self, EV_ALIEN_SPIKER, DirToByte(self->s.origin2));

	// Calculate total perimeter of all spike rows to allow for a more even spike distribution.
	float totalPerimeter = 0.0f;

	for (int row = 0; row < SPIKER_MISSILEROWS; row++) {
		float altitude = (((float)row + SPIKER_ROWOFFSET) * M_PI_2) / (float)SPIKER_MISSILEROWS;
		float perimeter = 2.0f * M_PI * cos(altitude);

		totalPerimeter += perimeter;
	}

	// Distribute and launch missiles.
	vec3_t dir, rowBase, zenith, rotAxis;

	VectorCopy(self->s.origin2, zenith);
	PerpendicularVector(rotAxis, zenith);

	for (int row = 0; row < SPIKER_MISSILEROWS; row++) {
		float altitude = (((float)row + SPIKER_ROWOFFSET) * M_PI_2) / (float)SPIKER_MISSILEROWS;
		float perimeter = 2.0f * M_PI * cos(altitude);

		RotatePointAroundVector(rowBase, rotAxis, zenith, RAD2DEG(M_PI_2 - altitude));

		// Attempt to distribute spikes with equal distance on all rows.
		int spikes = (int)round(((float)SPIKER_MISSILES * perimeter) / totalPerimeter);

		for (int spike = 0; spike < spikes; spike++) {
			float azimuth = 2.0f * M_PI * (((float)spike + 0.5f * crandom()) / (float)spikes);
			float altitudeVariance = 0.5f * crandom() * M_PI_2 / (float)SPIKER_MISSILEROWS;

			RotatePointAroundVector(dir, zenith, rowBase, RAD2DEG(azimuth));
			RotatePointAroundVector(dir, rotAxis, dir, RAD2DEG(altitudeVariance));

			// Trace in the shooting direction and do not shoot spikes that are likely to harm
			// friendly entities.
			bool fire = SafeToShoot(Vec3::Load(dir));

			logger.Debug("Spiker #%d %s: Row %d/%d: Spike %2d/%2d: "
				"( Alt %2.0f°, Az %3.0f° → %.2f, %.2f, %.2f )", self->s.number,
				fire ? "fires" : "skips", row + 1, SPIKER_MISSILEROWS, spike + 1, spikes,
				RAD2DEG(altitude + altitudeVariance), RAD2DEG(azimuth), dir[0], dir[1], dir[2]);

			if (!fire) {
				continue;
			}

			G_SpawnMissile(
				MIS_SPIKER, self, self->s.origin, dir, nullptr, G_FreeEntity,
				level.time + (int)(1000.0f * SPIKER_SPIKE_RANGE /
								   (float)BG_Missile(MIS_SPIKER)->speed));
		}
	}

	restUntil = level.time + SPIKER_COOLDOWN;
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
