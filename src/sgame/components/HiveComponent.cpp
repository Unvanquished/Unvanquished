#include "HiveComponent.h"

HiveComponent::HiveComponent(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent)
	: HiveComponentBase(entity, r_AlienBuildableComponent)
{}

void HiveComponent::HandleDamage(float amount, gentity_t* source, Util::optional<Vec3> location,
                                 Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	// If inactive, fire on attacker even if it's out of sense range.
	if (entity.oldEnt->spawned && entity.oldEnt->powered && !entity.oldEnt->active &&
	        GetAlienBuildableComponent().GetBuildableComponent().GetHealthComponent().Alive()) {
		if (AHive_TargetValid(entity.oldEnt, source, true)) {
			entity.oldEnt->target = source;
			source->numTrackedBy++;

			AHive_Fire(entity.oldEnt);
		}
	}
}
