#ifndef HUMANBUILDABLE_COMPONENT_H_
#define HUMANBUILDABLE_COMPONENT_H_

#include "../backend/CBSEBackend.h"
#include "../backend/CBSEComponents.h"

// TODO: Remove or move this once logic outside this component doesn't need it anymore.
#define HUMAN_DETONATION_RAND_DELAY HUMAN_DETONATION_DELAY + \
                                    (((rand() - RAND_MAX / 2) / (float)(RAND_MAX / 2)) * \
	                                DETONATION_DELAY_RAND_RANGE * HUMAN_DETONATION_DELAY)

class HumanBuildableComponent: public HumanBuildableComponentBase {
	public:
		// ///////////////////// //
		// Autogenerated Members //
		// ///////////////////// //

		/**
		 * @brief Default constructor of the HumanBuildableComponent.
		 * @param entity The entity that owns the component instance.
		 * @param r_BuildableComponent A BuildableComponent instance that this component depends on.
		 * @param r_TeamComponent A TeamComponent instance that this component depends on.
		 * @param r_CorrodibleComponent A CorrodibleComponent instance that this component depends on.
		 * @note This method is an interface for autogenerated code, do not modify its signature.
		 */
		HumanBuildableComponent(Entity& entity, BuildableComponent& r_BuildableComponent, TeamComponent& r_TeamComponent, CorrodibleComponent& r_CorrodibleComponent);

		/**
		 * @brief Handle the Die message.
		 * @param killer
		 * @param meansOfDeath
		 * @note This method is an interface for autogenerated code, do not modify its signature.
		 */
		void HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath);

		// ///////////////////// //

	private:
		void Blast(int timeDelta);
};

#endif // HUMANBUILDABLE_COMPONENT_H_
