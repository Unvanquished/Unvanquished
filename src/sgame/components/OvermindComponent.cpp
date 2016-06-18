#include "OvermindComponent.h"

OvermindComponent::OvermindComponent(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent,
                                     MainBuildableComponent& r_MainBuildableComponent)
	: OvermindComponentBase(entity, r_AlienBuildableComponent, r_MainBuildableComponent)
{}

void OvermindComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	// TODO: Reuse or remove message handler.
}

void OvermindComponent::HandleFinishConstruction() {
	// TODO: Move to MainBuildableComponent.
	G_TeamCommand(TEAM_ALIENS, "cp \"The Overmind has awakened!\"");
}
