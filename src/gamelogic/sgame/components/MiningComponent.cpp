#include "MiningComponent.h"

MiningComponent::MiningComponent(Entity& entity)
	: MiningComponentBase(entity)
{}

void MiningComponent::HandlePrepareNetCode() {
	HealthComponent *healthComponent = entity.Get<HealthComponent>();

	if (healthComponent && !healthComponent->Alive()) {
		entity.oldEnt->s.weaponAnim = 0; // Mining efficiency.
	}
}

void MiningComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	// TODO: Move to BPStorageComponent.
	G_BPStorageDie(entity.oldEnt);

	// The remaining BP storages take over this one's account.
	G_MarkBuildPointsMined(entity.oldEnt->buildableTeam, -entity.oldEnt->acquiredBuildPoints);

	// Inform neighbours so they can increase their rate immediately.
	G_RGSInformNeighbors(entity.oldEnt);
}

