#include "TurretComponent.h"

TurretComponent::TurretComponent(Entity& entity)
	: TurretComponentBase(entity)
{}

void TurretComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	if (entity.oldEnt->target) entity.oldEnt->target->numTrackedBy--;

	// TODO: This assumes HumanBuildableComponent's HandleDie is called first, validate this.
	if (entity.Get<BuildableComponent>()->GetState() == BuildableComponent::PRE_BLAST) {
		// Rocketpod: The safe mode has the same idle as the unpowered state.
		// TODO: Move to RocketPodComponent.
		if (entity.oldEnt->turretSafeMode) {
			// TODO: Move animation code to BuildableComponent.
			G_SetBuildableAnim(entity.oldEnt, BANIM_DESTROY_UNPOWERED, true);
		}

		// Do some last movements before entering blast state.
		entity.oldEnt->think = HTurret_PreBlast;
		entity.oldEnt->nextthink = level.time;
	}
}
