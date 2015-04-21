#include "MainBuildableComponent.h"

MainBuildableComponent::MainBuildableComponent(Entity& entity, BuildableComponent& r_BuildableComponent)
	: MainBuildableComponentBase(entity, r_BuildableComponent)
{}

void MainBuildableComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	// TODO: Move to BPStorageComponent.
	G_BPStorageDie(entity.oldEnt);
}

