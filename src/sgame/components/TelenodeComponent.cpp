#include "TelenodeComponent.h"

TelenodeComponent::TelenodeComponent(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent, SpawnerComponent& r_SpawnerComponent)
	: TelenodeComponentBase(entity, r_HumanBuildableComponent, r_SpawnerComponent)
{}

void TelenodeComponent::HandleCheckSpawnPoint(Entity *&blocker, glm::vec3& spawnPoint) {
	CheckSpawnPoint(
		entity.oldEnt->num(), VEC2GLM( entity.oldEnt->s.origin ),
		VEC2GLM( entity.oldEnt->s.origin2 ), blocker, spawnPoint
	);
}

bool TelenodeComponent::CheckSpawnPoint(
	int spawnerNumber, const glm::vec3 spawnerOrigin, const glm::vec3 spawnerNormal, Entity*& blocker,
	glm::vec3& spawnPoint
){
	glm::vec3 spawnerMins, spawnerMaxs, clientMins, clientMaxs;

	BG_BoundingBox( BA_H_SPAWN, spawnerMins, spawnerMaxs );

	// HACK: Assumes this class is the greatest that can spawn.
	BG_BoundingBox( PCL_HUMAN_NAKED, clientMins, clientMaxs );

	// TODO: Does this work if the Telenode is at an angle?
	float displacement = spawnerMaxs[2] - clientMins[2] + 1.0f;

	spawnPoint = spawnerOrigin + displacement * spawnerNormal;

	blocker = SpawnerComponent::CheckSpawnPointHelper(
		spawnerNumber, spawnerOrigin, spawnPoint, clientMins, clientMaxs
	);

	return !blocker;
}
