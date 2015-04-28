#include "ResourceStorageComponent.h"

ResourceStorageComponent::ResourceStorageComponent(Entity& entity)
	: ResourceStorageComponentBase(entity)
{}

void ResourceStorageComponent::HandlePrepareNetCode() {
	// TODO: Implement ResourceStorageComponent::PrepareNetCode
}

void ResourceStorageComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	G_BPStorageDie(entity.oldEnt);
}

