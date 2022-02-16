#ifndef EGG_COMPONENT_H_
#define EGG_COMPONENT_H_

#include "../backend/CBSEBackend.h"
#include "../backend/CBSEComponents.h"

class EggComponent: public EggComponentBase {
	public:
		// ///////////////////// //
		// Autogenerated Members //
		// ///////////////////// //

		/**
		 * @brief Default constructor of the EggComponent.
		 * @param entity The entity that owns the component instance.
		 * @param r_AlienBuildableComponent A AlienBuildableComponent instance that this component depends on.
		 * @param r_SpawnerComponent A SpawnerComponent instance that this component depends on.
		 * @note This method is an interface for autogenerated code, do not modify its signature.
		 */
		EggComponent(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent, SpawnerComponent& r_SpawnerComponent);

		/**
		 * @brief Handle the CheckSpawnPoint message.
		 * @param blocker
		 * @param spawnPoint
		 * @note This method is an interface for autogenerated code, do not modify its signature.
		 */
		void HandleCheckSpawnPoint(Entity*& blocker, vec3_t spawnPoint);

		// ///////////////////// //

		/**
		 * @brief Checks whether spawning at an egg at the given location is possible.
		 * @param spawnerNumber Entity number of an existing egg.
		 * @param spawnerOrigin The origin of the egg.
		 * @param spawnerNormal The normal of the egg.
		 * @param blocker Set to an entity that blocks the egg, or nullptr.
		 * @param spawnPoint Set to the asscoiated spawn point.
		 * @return Whether spawning from an egg at the given location is currently possible.
		 */
		static bool CheckSpawnPoint(
			int spawnerNumber, const Vec3 spawnerOrigin, const Vec3 spawnerNormal, Entity*& blocker,
			vec3_t spawnPoint
		);

	private:

};

#endif // EGG_COMPONENT_H_
