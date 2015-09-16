#include "OvermindComponent.h"

OvermindComponent::OvermindComponent(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent,
                                     MainBuildableComponent& r_MainBuildableComponent)
	: OvermindComponentBase(entity, r_AlienBuildableComponent, r_MainBuildableComponent)
{}

void OvermindComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	if (G_IsWarnableMOD(meansOfDeath)) {
		G_BroadcastEvent(EV_OVERMIND_DYING, 0, TEAM_ALIENS);
	}
}
