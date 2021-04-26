#include "EggComponent.h"

EggComponent::EggComponent(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent, SpawnerComponent& r_SpawnerComponent)
	: EggComponentBase(entity, r_AlienBuildableComponent, r_SpawnerComponent)
{}

void EggComponent::HandleCheckSpawnPoint(Entity *&blocker, Vec3& spawnPoint) {
	CheckSpawnPoint(
		entity.oldEnt->s.number, Vec3::Load(entity.oldEnt->s.origin),
		Vec3::Load(entity.oldEnt->s.origin2), blocker, spawnPoint
	);
}

bool EggComponent::CheckSpawnPoint(
	int spawnerNumber, const Vec3 spawnerOrigin, const Vec3 spawnerNormal, Entity*& blocker,
	Vec3& spawnPoint
){
	vec3_t spawnerMins, spawnerMaxs, clientMins, clientMaxs;

	BG_BuildableBoundingBox(BA_A_SPAWN, spawnerMins, spawnerMaxs);

	// HACK: Assumes this class is the greatest that can spawn.
	BG_ClassBoundingBox(PCL_ALIEN_BUILDER0_UPG, clientMins, clientMaxs, nullptr, nullptr, nullptr);

	// TODO: Merge both methods below.
	float displacement;

	// Method 1.
	// This describes the worst case for any angle.
	/*{
		displacement = std::max(VectorLength(spawnerMins), VectorLength(spawnerMaxs)) +
		               std::max(VectorLength(clientMins),  VectorLength(clientMaxs))  + 1.0f;
	}*/

	// Method 2
	// For normals with equal X and Y component these results are optimal. Otherwise they are still
	// better than naive worst-case calculations.
	// HACK: This only works for normals w/ angle < 45° towards either {0,0,1} or {0,0,-1}.
	{
		float minDistance, maxDistance, frac;

		if (spawnerNormal[2] < 0.0f) {
			// Best case: Client spawning right below egg.
			minDistance = -spawnerMins[2] + clientMaxs[2];

			// Worst case: Bounding boxes meet at their corners.
			maxDistance = VectorLength(spawnerMins) + VectorLength(clientMaxs);

			// The fraction of the angle seperating best (0°) and worst case (45°).
			frac = acosf(-spawnerNormal[2]) / M_PI_4;
		} else {
			// Best case: Client spawning right above egg.
			minDistance = spawnerMaxs[2] - clientMins[2];

			// Worst case: Bounding boxes meet at their corners.
			maxDistance = VectorLength(spawnerMaxs) + VectorLength(clientMins);

			// The fraction of the angle seperating best (0°) and worst case (45°).
			frac = acosf(spawnerNormal[2]) / M_PI_4;
		}

		// The linear interpolation of min & max distance should be an upper boundary for the
		// optimal (closest possible) distance.
		displacement = (1.0f - frac) * minDistance + frac * maxDistance + 1.0f;
	}

	spawnPoint = spawnerOrigin + displacement * spawnerNormal;

	blocker = SpawnerComponent::CheckSpawnPointHelper(
		spawnerNumber, spawnerOrigin, spawnPoint, Vec3::Load(clientMins), Vec3::Load(clientMaxs)
	);

	return !blocker;
}
