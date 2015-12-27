#include "SpikerComponent.h"

SpikerComponent::SpikerComponent(
	Entity &entity, AlienBuildableComponent &r_AlienBuildableComponent)
	: SpikerComponentBase(entity, r_AlienBuildableComponent),
	  restUntil(0),
	  lastScoring(-1),
	  lastSensing(false) {
	RegisterDefaultThinker();
}

void SpikerComponent::HandleDamage(float amount, gentity_t *source,
								   Util::optional<Vec3> location,
								   Util::optional<Vec3> direction, int flags,
								   meansOfDeath_t meansOfDeath) {
	if (!GetBuildableComponent().GetHealthComponent().Alive()) {
		return;
	}

	if (lastScoring > 0.0f) {
		Fire();
	}
}

void SpikerComponent::Think(int timeDelta) {
	gentity_t *ent, *self;
	float scoring, enemyDamage, friendlyDamage;
	bool sensing;

	self = entity.oldEnt;

	// TODO: port gentity_t::spawned and gentity_t::powered
	if (!self->spawned || !self->powered || G_Dead(self)) {
		return;
	}

	// stop here if recovering from shot
	if (level.time < restUntil) {
		lastScoring = 0.0f;
		lastSensing = false;

		return;
	}

	directions.clear();

	// calculate a "scoring" of the situation to decide on the best moment to
	// shoot
	enemyDamage = friendlyDamage = 0.0f;
	sensing = false;
	for (ent = nullptr; (ent = G_IterateEntitiesWithinRadius(
							 ent, self->s.origin, SPIKER_SPIKE_RANGE));) {
		float health, durability;

		if (self == ent || (ent->flags & FL_NOTARGET)) continue;

		HealthComponent *healthComponent = ent->entity->Get<HealthComponent>();

		if (!healthComponent) continue;

		if (G_OnSameTeam(self, ent)) {
			health = healthComponent->Health();
		} else if (ent->s.eType == ET_BUILDABLE) {
			// Enemy buildables don't count in the scoring.
			continue;
		} else {
			health = healthComponent->MaxHealth();
		}

		ArmorComponent *armorComponent = ent->entity->Get<ArmorComponent>();

		if (armorComponent) {
			durability = health / armorComponent->GetNonLocationalDamageMod();
		} else {
			durability = health;
		}

		if (durability <= 0.0f || !G_LineOfSight(self, ent)) continue;

		vec3_t vecToTarget;
		VectorSubtract(ent->s.origin, self->s.origin, vecToTarget);


		// only entities in the spiker's upper hemisphere can be hit
		if (DotProduct(self->s.origin2, vecToTarget) < 0) {
			continue;
		}

		// approximate average damage the entity would receive from spikes
		float diameter = VectorLength(ent->r.mins) + VectorLength(ent->r.maxs);
		float distance = VectorLength(vecToTarget);
		float effectArea = 2.0f * M_PI * distance * distance;  // half sphere
		// approx. proj. of target on effect area
		float targetArea = 0.5f * diameter * diameter;
		float expectedDamage = (targetArea / effectArea) *
							   (float)SPIKER_MISSILES *
							   (float)BG_Missile(MIS_SPIKER)->damage;
		float relativeDamage = expectedDamage / durability;

		if (G_OnSameTeam(self, ent)) {
			friendlyDamage += relativeDamage;
		} else {
			if (distance < SPIKER_SENSE_RANGE) {
				sensing = true;
				if (sensing && !lastSensing) {
					RegisterFastThinker();
				}
			}
			enemyDamage += relativeDamage;
			directions.push_back(Vec3::Load(vecToTarget));
		}
	}

	// friendly entities that can receive damage substract from the scoring,
	// enemies add to it
	scoring = enemyDamage - friendlyDamage;

	// Shoot if a viable target leaves sense range even if scoring is bad.
	// Guarantees that the spiker always shoots eventually when an enemy gets
	// close enough to it.
	bool senseLost = lastScoring > 0.0f && lastSensing && !sensing;

	if (scoring > 0.0f || senseLost) {
		if (g_debugTurrets.integer) {
			Com_Printf("Spiker #%i scoring %.1f - %.1f = %.1f%s%s\n",
					   self->s.number, enemyDamage, friendlyDamage, scoring,
					   sensing ? " (sensing)" : "",
					   senseLost ? " (lost target)" : "");
		}

		if ((scoring <= lastScoring && sensing) || senseLost) {
			Fire();
		}
	}

	lastScoring = scoring;
	lastSensing = sensing;
}

bool SpikerComponent::Fire() {
	gentity_t *self = entity.oldEnt;
	// check if still resting
	if (restUntil > level.time) {
		return false;
	}
	G_SetBuildableAnim(self, BANIM_ATTACK1, false);
	int batches = std::min(SPIKER_MISSILES, static_cast<int>(directions.size()));
	for (int i = 0; i < batches; ++i) {
		for (int j = 0; j < SPIKER_MISSILES / batches; ++j) {
		// play shooting animation
		vec3_t dir;
		directions[i].Store(dir);
		for (int i = 0; i < 3; ++i) {
			dir[i] += dir[i] * crandom() * 1.5f;
		}
		G_SpawnMissile(
			MIS_SPIKER, self, self->s.origin, dir, nullptr, G_FreeEntity,
			level.time + (int)(1000.0f * SPIKER_SPIKE_RANGE /
			(float)BG_Missile(MIS_SPIKER)->speed));
		}

	}

	// do radius damage in addition to spike missiles
	// G_SelectiveRadiusDamage( self->s.origin, source, SPIKER_RADIUS_DAMAGE,
	// SPIKER_RANGE, self,
	//                         MOD_SPIKER, TEAM_ALIENS );

	restUntil = level.time + SPIKER_COOLDOWN;
	RegisterDefaultThinker();
	return true;
}

BuildableComponent &SpikerComponent::GetBuildableComponent() {
	return GetAlienBuildableComponent().GetBuildableComponent();
}

void SpikerComponent::RegisterFastThinker() {
	GetBuildableComponent().GetThinkingComponent().UnregisterActiveThinker();
	GetBuildableComponent().REGISTER_THINKER(
		Think, ThinkingComponent::SCHEDULER_CLOSEST, 1);
}

void SpikerComponent::RegisterDefaultThinker() {
	GetBuildableComponent().GetThinkingComponent().UnregisterActiveThinker();
	GetBuildableComponent().REGISTER_THINKER(
		Think, ThinkingComponent::SCHEDULER_CLOSEST, 500);
}
