#include "MedipadComponent.h"

MedipadComponent::MedipadComponent(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent)
	: MedipadComponentBase(entity, r_HumanBuildableComponent)
{}

void MedipadComponent::HandleDie(gentity_t* /*killer*/, meansOfDeath_t /*meansOfDeath*/) {
	// Clear target's healing flag.
	// TODO: This will fail if multiple sources try to control the flag.
	if (entity.oldEnt->target && entity.oldEnt->target->client) {
		entity.oldEnt->target->client->ps.stats[STAT_STATE] &= ~SS_HEALING_2X;
	}
}
