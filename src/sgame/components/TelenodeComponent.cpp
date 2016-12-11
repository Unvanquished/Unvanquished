#include "TelenodeComponent.h"

TelenodeComponent::TelenodeComponent(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent, SpawnerComponent& r_SpawnerComponent)
	: TelenodeComponentBase(entity, r_HumanBuildableComponent, r_SpawnerComponent)
{}

void TelenodeComponent::HandleCheckSpawnPoint(Entity *&blocker, Vec3& spawnPoint) {
	CheckSpawnPoint(
		entity.oldEnt->s.number, Vec3::Load(entity.oldEnt->s.origin),
		Vec3::Load(entity.oldEnt->s.origin2), blocker, spawnPoint
	);
}

bool TelenodeComponent::CheckSpawnPoint(
	int spawnerNumber, const Vec3 spawnerOrigin, const Vec3 spawnerNormal, Entity*& blocker,
	Vec3& spawnPoint
){
	vec3_t spawnerMins, spawnerMaxs, clientMins, clientMaxs;

	BG_BuildableBoundingBox(BA_H_SPAWN, spawnerMins, spawnerMaxs);

	// HACK: Assumes this class is the greatest that can spawn.
	BG_ClassBoundingBox(PCL_HUMAN_NAKED, clientMins, clientMaxs, nullptr, nullptr, nullptr);

	// TODO: Does this work if the Telenode is at an angle?
	float displacement = spawnerMaxs[2] - clientMins[2] + 1.0f;

	spawnPoint = spawnerOrigin + displacement * spawnerNormal;

	blocker = SpawnerComponent::CheckSpawnPointHelper(
		spawnerNumber, spawnerOrigin, spawnPoint, Vec3::Load(clientMins), Vec3::Load(clientMaxs)
	);

	return !blocker;
}
