#include "BuildableComponent.h"

BuildableComponent::BuildableComponent(Entity& entity, HealthComponent& r_HealthComponent, ThinkingComponent& r_ThinkingComponent)
	: BuildableComponentBase(entity, r_HealthComponent, r_ThinkingComponent)
{}

void BuildableComponent::HandlePrepareNetCode() {
	if (!GetHealthComponent().Alive()) {
		entity.oldEnt->s.eFlags &= ~EF_FIRING;
	}
}

void BuildableComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	// TODO: Move animation code to BuildableComponent.
	G_SetBuildableAnim(entity.oldEnt, entity.oldEnt->powered ? BANIM_DESTROY : BANIM_DESTROY_UNPOWERED, true);
	G_SetIdleBuildableAnim(entity.oldEnt, BANIM_DESTROYED);

	entity.oldEnt->killedBy = killer->s.number;

	G_LogDestruction(entity.oldEnt, killer, meansOfDeath);

	// TODO: Handle in TaggableComponent.
	Beacon::DetachTags(entity.oldEnt);
}
