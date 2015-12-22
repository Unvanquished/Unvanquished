#include "SpikerComponent.h"

static Log::Logger logger("sgame.spiker");

SpikerComponent::SpikerComponent(
	Entity &entity, AlienBuildableComponent &r_AlienBuildableComponent)
	: SpikerComponentBase(entity, r_AlienBuildableComponent)
	, activeThinker(AT_NONE)
	, restUntil(0)
	, lastScoring(-1)
	, lastSensing(false) {
	RegisterSlowThinker();
}

void SpikerComponent::HandleDamage(float amount, gentity_t *source, Util::optional<Vec3> location,
                                   Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	if (!GetAlienBuildableComponent().GetBuildableComponent().Active()) {
		return;
	}

	// Shoot if there is a viable target.
	if (lastScoring > 0.0f) {
		Fire();
	}
}

void SpikerComponent::Think(int timeDelta) {
	if (!GetAlienBuildableComponent().GetBuildableComponent().Active()) {
		return;
	}

	// Stop here if recovering from shot.
	if (level.time < restUntil) {
		lastScoring = 0.0f;
		lastSensing = false;

		return;
	}

	float enemyDamage    = 0.0f,
	      friendlyDamage = 0.0f;
	bool  sensing        = false;

	// Calculate a "scoring" of the situation to decide on the best moment to shoot.
	ForEntities<HealthComponent>([&](Entity& other, HealthComponent& healthComponent) {
		// TODO: Check if "entity == other" does the job.
		if (entity.oldEnt == other.oldEnt)                                return;
		if (G_Team(other.oldEnt) == TEAM_NONE)                            return;
		if ((other.oldEnt->flags & FL_NOTARGET))                          return;
		if (G_Distance(entity.oldEnt, other.oldEnt) > SPIKER_SPIKE_RANGE) return;
		if (!G_LineOfSight(entity.oldEnt, other.oldEnt))                  return;

		bool  onSameTeam;
		float health, durability;

		if ((onSameTeam = G_OnSameTeam(entity.oldEnt, other.oldEnt))) {
			// Consider the current health of friendly entities.
			health = healthComponent.Health();
		} else if (other.Get<BuildableComponent>()) {
			// Enemy buildables don't count in the scoring.
			return;
		} else {
			// Assume no knowledge of the current health of enemies.
			health = healthComponent.MaxHealth();
		}

		ArmorComponent *armorComponent = other.Get<ArmorComponent>();

		if (armorComponent) {
			durability = health / armorComponent->GetNonLocationalDamageMod();
		} else {
			durability = health;
		}

		// Ignore targets that are dead or not visible.
		if (durability <= 0.0f || !G_LineOfSight(entity.oldEnt, other.oldEnt)) return;

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
		float expectedDamage = (targetArea / effectArea) * (float)SPIKER_MISSILES *
		                       (float)BG_Missile(MIS_SPIKER)->damage;
		float relativeDamage = expectedDamage / durability;

		if (onSameTeam) {
			friendlyDamage += relativeDamage;
		} else {
			if (distance < SPIKER_SENSE_RANGE) {
				sensing = true;
				if (sensing && !lastSensing) {
					RegisterFastThinker();
				}
			}
			enemyDamage += relativeDamage;
		}
	});

	// Friendly entities that can receive damage substract from the scoring, enemies add to it.
	float scoring = enemyDamage - friendlyDamage;

	// Shoot if a viable target leaves sense range even if the new scoring is bad.
	// Guarantees that the spiker always shoots eventually when an enemy gets close enough to it.
	bool senseLost = lastScoring > 0.0f && lastSensing && !sensing;

	if (scoring > 0.0f || senseLost) {
		logger.Verbose("Spiker #%i scoring %.3f - %.3f = %.3f%s%s",
		               entity.oldEnt->s.number, enemyDamage, friendlyDamage, scoring,
		               sensing ? " (sensing)" : "", senseLost ? " (lost target)" : "");

		if ((scoring <= lastScoring && sensing) || senseLost) {
			Fire();
		}
	}

	lastScoring = scoring;
	lastSensing = sensing;
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

			logger.Debug("Spiker #%d fires: Row %d/%d: Spike %2d/%2d: "
			             "( Alt %2.0f°, Az %3.0f° → %.2f, %.2f, %.2f )",
			             self->s.number, row + 1, SPIKER_MISSILEROWS, spike + 1, spikes,
			             RAD2DEG(altitude + altitudeVariance), RAD2DEG(azimuth),
			             dir[0], dir[1], dir[2]);

			G_SpawnMissile(
				MIS_SPIKER, self, self->s.origin, dir, nullptr, G_FreeEntity,
				level.time + (int)(1000.0f * SPIKER_SPIKE_RANGE /
								   (float)BG_Missile(MIS_SPIKER)->speed));
		}
	}

	// Do radius damage in addition to spike missiles.
	//G_SelectiveRadiusDamage(self->s.origin, source, SPIKER_RADIUS_DAMAGE, SPIKER_RANGE, self,
	//                        MOD_SPIKER, TEAM_ALIENS);

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
