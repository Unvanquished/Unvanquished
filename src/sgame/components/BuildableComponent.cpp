#include "BuildableComponent.h"

BuildableComponent::BuildableComponent(Entity& entity, HealthComponent& r_HealthComponent, ThinkingComponent& r_ThinkingComponent)
	: BuildableComponentBase(entity, r_HealthComponent, r_ThinkingComponent)
	, state(CONSTRUCTING)
	, marked(false) {
	REGISTER_THINKER(Think, ThinkingComponent::SCHEDULER_AVERAGE, 100);
}

void BuildableComponent::HandlePrepareNetCode() {
	if (state == CONSTRUCTING) {
		entity.oldEnt->s.eFlags &= ~EF_B_SPAWNED;
	} else {
		entity.oldEnt->s.eFlags |= EF_B_SPAWNED;
	}

	if (state == POST_BLAST) {
		entity.oldEnt->s.eFlags |= EF_NODRAW;
	} else {
		entity.oldEnt->s.eFlags &= ~EF_NODRAW;
	}

	if (marked) {
		entity.oldEnt->s.eFlags |= EF_B_MARKED;
	} else {
		entity.oldEnt->s.eFlags &= ~EF_B_MARKED;
	}

	if (entity.oldEnt->powered) {
		entity.oldEnt->s.eFlags |= EF_B_POWERED;
	} else {
		entity.oldEnt->s.eFlags &= ~EF_B_POWERED;
	}

	if (GetHealthComponent().Alive()) {
		entity.oldEnt->s.eFlags &= ~EF_DEAD;
	} else {
		entity.oldEnt->s.eFlags |= EF_DEAD;
		entity.oldEnt->s.eFlags &= ~EF_FIRING;
	}
}

void BuildableComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	// Note that this->state is adjusted in (Alien|Human)BuildableComponent::HandleDie so they have
	// access to its current value.

	// TODO: Move animation code to BuildableComponent.
	G_SetBuildableAnim(entity.oldEnt, entity.oldEnt->powered ? BANIM_DESTROY : BANIM_DESTROY_UNPOWERED, true);
	G_SetIdleBuildableAnim(entity.oldEnt, BANIM_DESTROYED);

	entity.oldEnt->killedBy = killer->s.number;

	G_LogDestruction(entity.oldEnt, killer, meansOfDeath);

	// TODO: Handle in TaggableComponent.
	Beacon::DetachTags(entity.oldEnt);
}

void BuildableComponent::Think(int timeDelta) {
	lifecycle_t oldState;
	do {
		switch ((oldState = state)) {
			case CONSTRUCTING: {
				int constructionTime = BG_Buildable(entity.oldEnt->s.modelindex)->buildTime;

				if (entity.oldEnt->creationTime + constructionTime < level.time) {
					// Finish construction.
					state = CONSTRUCTED;

					// Notify alien team of new overmind.
					// TODO: Send FinishConstruction message instead and do this in OvermindComponent.
					if (entity.oldEnt->s.modelindex == BA_A_OVERMIND) {
						G_TeamCommand(TEAM_ALIENS, "cp \"The Overmind has awakened!\"");
					}

					// Award momentum.
					G_AddMomentumForBuilding(entity.oldEnt);
				} else {
					// Gain health while constructing.
					entity.Heal(GetHealthComponent().MaxHealth() * ((float)timeDelta / (float)constructionTime)
					            * (1.0f - BUILDABLE_START_HEALTH_FRAC), nullptr);
				}
			} break;

			case CONSTRUCTED: {
				// Set legacy gentity_t fields.
				entity.oldEnt->spawned = true;
				entity.oldEnt->enabled = true;

				if (entity.oldEnt->powered) {
					// Regenerate health.
					int   regenWait;
					float regenRate = (float)BG_Buildable(entity.oldEnt->s.modelindex)->regenRate;

					switch (entity.oldEnt->buildableTeam) {
						case TEAM_ALIENS: regenWait = ALIEN_BUILDABLE_REGEN_WAIT; break;
						case TEAM_HUMANS: regenWait = HUMAN_BUILDABLE_REGEN_WAIT; break;
						default:          regenWait = 0;                          break;
					}

					if (regenRate && (entity.oldEnt->lastDamageTime + regenWait) < level.time) {
						entity.Heal(timeDelta * 0.001f * regenRate, nullptr);
					}
				}
			} break;

			default:
				// All other states are death states.
				return;
		}
	} while (state != oldState); // After every state change, the new state is evaluated, too.

	// TODO: Move this to SpawnerComponent?
	entity.oldEnt->clientSpawnTime = std::max(0, entity.oldEnt->clientSpawnTime - timeDelta);

	// Check if touching any triggers.
	// TODO: Move helper here.
	G_BuildableTouchTriggers(entity.oldEnt);
}

bool BuildableComponent::Active() {
	return (state == CONSTRUCTED && entity.oldEnt->powered);
}
