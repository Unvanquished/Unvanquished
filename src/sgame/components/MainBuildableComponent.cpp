#include "MainBuildableComponent.h"

MainBuildableComponent::MainBuildableComponent(Entity& entity, BuildableComponent& r_BuildableComponent,
                                               ResourceStorageComponent& r_ResourceStorageComponent)
	: MainBuildableComponentBase(entity, r_BuildableComponent, r_ResourceStorageComponent) {

	// Initialize acquired build points, as they are shared between lifecycles of the main buildable.
	GetResourceStorageComponent().SetAcquiredBuildPoints(level.team[entity.oldEnt->buildableTeam].mainStructAcquiredBP);
}
