#include "SpikerComponent.h"

SpikerComponent::SpikerComponent(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent)
	: SpikerComponentBase(entity, r_AlienBuildableComponent)
{}

void SpikerComponent::HandleDamage(float amount, gentity_t* source, Util::optional<Vec3> location,
                                   Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	if (!GetAlienBuildableComponent().GetBuildableComponent().GetHealthComponent().Alive()) {
		return;
	}

	if (entity.oldEnt->spikerLastScoring > 0.0f) {
		ASpiker_Fire(entity.oldEnt);
	}
}
