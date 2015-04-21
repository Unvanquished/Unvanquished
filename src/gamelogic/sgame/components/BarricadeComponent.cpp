#include "BarricadeComponent.h"

BarricadeComponent::BarricadeComponent(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent)
	: BarricadeComponentBase(entity, r_AlienBuildableComponent)
{}

void BarricadeComponent::HandleDamage(float amount, gentity_t* source, Util::optional<Vec3> location, Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	if (!GetAlienBuildableComponent().GetBuildableComponent().GetHealthComponent().Alive()) {
		return;
	}

	G_SetBuildableAnim(entity.oldEnt, entity.oldEnt->shrunkTime ? BANIM_PAIN2 : BANIM_PAIN1, false);
}

void BarricadeComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	ABarricade_Shrink(entity.oldEnt, true);
}
