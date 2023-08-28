#include "HumanBuildableComponent.h"
#include "../Entities.h"

static Log::Logger humanBuildableLogger("sgame.humanbuildings");

HumanBuildableComponent::HumanBuildableComponent(Entity& entity, BuildableComponent& r_BuildableComponent,
	TeamComponent& r_TeamComponent)
	: HumanBuildableComponentBase(entity, r_BuildableComponent, r_TeamComponent)
{}

void HumanBuildableComponent::HandleDie(gentity_t* /*killer*/, meansOfDeath_t /*meansOfDeath*/) {
	switch (GetBuildableComponent().GetState()) {
		// Regular death, fully constructed.
		case BuildableComponent::CONSTRUCTED:
			// Play a warning sound before reactor explosion. Don't randomize blast
			// delay for them so the sound stays synced.
			// TODO: Move to ReactorComponent if possible.
			switch (entity.oldEnt->s.modelindex) {
				case BA_H_REACTOR:
					G_AddEvent(entity.oldEnt, EV_HUMAN_BUILDABLE_DYING, 0);

					GetBuildableComponent().REGISTER_THINKER(
						Blast, ThinkingComponent::SCHEDULER_BEFORE, HUMAN_DETONATION_DELAY
					);
					break;

				default:
					GetBuildableComponent().REGISTER_THINKER(
						Blast, ThinkingComponent::SCHEDULER_BEFORE, HUMAN_DETONATION_RAND_DELAY
					);
			}
			GetBuildableComponent().SetState(BuildableComponent::PRE_BLAST);
			humanBuildableLogger.Notice("Human buildable is going to blow up.");
			break;

		// Construction was canceled.
		case BuildableComponent::CONSTRUCTING:
			G_RewardAttackers(entity.oldEnt);
			entity.FreeAt(DeferredFreeingComponent::FREE_AFTER_THINKING);
			humanBuildableLogger.Notice("Human buildable was canceled.");
			break;

		default:
			humanBuildableLogger.Warn("Handling human buildable death event when not alive.");
	}
}

void HumanBuildableComponent::Blast(int /*timeDelta*/) {
	humanBuildableLogger.Notice("Human buildable is exploding.");

	// Damage close entity.
	const buildableAttributes_t* ba = BG_Buildable(entity.oldEnt->s.modelindex);
	Entities::KnockbackRadiusDamage(entity, ba->splashDamage, ba->splashRadius, ba->meansOfDeath);

	// Reward attackers.
	G_RewardAttackers(entity.oldEnt);

	// Stop collisions, add blast event and update buildable state.
	entity.oldEnt->r.contents = 0;
	trap_LinkEntity(entity.oldEnt);
	glm::vec3 blastDirection( 0.0f, 0.0f, 1.0f );
	G_AddEvent( entity.oldEnt, EV_HUMAN_BUILDABLE_EXPLOSION, DirToByte( &blastDirection[0] ) );
	GetBuildableComponent().SetState(BuildableComponent::POST_BLAST); // Makes entity invisible.

	// Remove after the explosion event expires.
	entity.oldEnt->freeAfterEvent = true;
	GetBuildableComponent().GetThinkingComponent().UnregisterActiveThinker();
}
