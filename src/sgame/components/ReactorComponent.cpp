#include "ReactorComponent.h"

ReactorComponent::ReactorComponent(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent,
                                   MainBuildableComponent& r_MainBuildableComponent)
	: ReactorComponentBase(entity, r_HumanBuildableComponent, r_MainBuildableComponent)
{}

void ReactorComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	// TODO: Reuse or remove message handler.
}
