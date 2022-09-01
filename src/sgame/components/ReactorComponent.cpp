#include "ReactorComponent.h"

const float ReactorComponent::ATTACK_RANGE  = 200.0f;
const float ReactorComponent::ATTACK_DAMAGE = 25.0f;
const float DAMAGE_INC_PER_MINER = 75.0f;
const float RANGE_INC_PER_MINER = 400.0f;

ReactorComponent::ReactorComponent(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent,
                                   MainBuildableComponent& r_MainBuildableComponent)
	: ReactorComponentBase(entity, r_HumanBuildableComponent, r_MainBuildableComponent)
{
	GetMainBuildableComponent().GetBuildableComponent().REGISTER_THINKER(
		Think, ThinkingComponent::SCHEDULER_AVERAGE, 1000
	);
}

void ReactorComponent::Think(int timeDelta) {
	if (!GetMainBuildableComponent().GetBuildableComponent().Active()) return;
	int miners = level.team[GetTeamComponent().Team()].numMiners;
	float baseDamage = (ATTACK_DAMAGE + (miners * DAMAGE_INC_PER_MINER)) * ((float)timeDelta / 1000.0f);
	float range = (ATTACK_RANGE + (RANGE_INC_PER_MINER * miners));

	// Zap close enemies.
	ForEntities<AlienClassComponent>([&](Entity& other, AlienClassComponent&) {
		// Respect the no-target flag.
		if (other.oldEnt->flags & FL_NOTARGET) return;

		// Don't zap through walls
		if ( !G_LineOfSight( entity.oldEnt, other.oldEnt, MASK_SOLID, false ) ) return;

		// TODO: Add LocationComponent and Utility::BBOXDistance.
		float distance = G_Distance(entity.oldEnt, other.oldEnt);

		if (distance >= range) return;

		float damage = baseDamage * (1.0f - (0.7f * distance / range));

		CreateTeslaTrail(other);

		other.Damage(damage, entity.oldEnt, {}, {}, 0, MOD_REACTOR);
	});
}

void ReactorComponent::CreateTeslaTrail(Entity& target) {
	gentity_t* trail = G_NewTempEntity( VEC2GLM( entity.oldEnt->s.origin ), EV_TESLATRAIL);
	trail->s.generic1  = entity.oldEnt->s.number; // Source.
	trail->s.clientNum = target.oldEnt->s.number; // Destination.
}
