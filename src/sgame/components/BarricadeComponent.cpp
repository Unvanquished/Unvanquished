#include "BarricadeComponent.h"

BarricadeComponent::BarricadeComponent(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent)
	: BarricadeComponentBase(entity, r_AlienBuildableComponent)
{}

void BarricadeComponent::HandleDamage(float amount, gentity_t* source, Util::optional<Vec3> location,
                                      Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	if (GetAlienBuildableComponent().GetBuildableComponent().GetState() != BuildableComponent::CONSTRUCTED) {
		return;
	}

	G_SetBuildableAnim(entity.oldEnt, entity.oldEnt->shrunkTime ? BANIM_PAIN2 : BANIM_PAIN1, false);
}

void BarricadeComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	if(!entity.oldEnt->shrunkTime) {
		// The barricade is extended, use a death animation that also shrinks it.
		G_SetBuildableAnim(entity.oldEnt, BANIM_DESTROY, true);
		G_SetIdleBuildableAnim(entity.oldEnt, BANIM_DESTROYED);

		entity.oldEnt->r.maxs[2] = (int)(entity.oldEnt->r.maxs[2] * BARRICADE_SHRINKPROP);
		trap_LinkEntity(entity.oldEnt);
	} else {
		// The barricade is already shrunken, use a death aniation that doesn't change its size.
		G_SetBuildableAnim(entity.oldEnt, BANIM_DESTROY_UNPOWERED, true);
		G_SetIdleBuildableAnim(entity.oldEnt, BANIM_DESTROYED_UNPOWERED);
	}

}
