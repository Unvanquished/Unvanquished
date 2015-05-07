#include "DrillComponent.h"

DrillComponent::DrillComponent(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent, MiningComponent& r_MiningComponent)
	: DrillComponentBase(entity, r_HumanBuildableComponent, r_MiningComponent)
{}
