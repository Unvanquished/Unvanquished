#include "BuildableComponent.h"

BuildableComponent::BuildableComponent(Entity& entity, HealthComponent& r_HealthComponent,
	ThinkingComponent& r_ThinkingComponent, TeamComponent& r_TeamComponent)
	: BuildableComponentBase(entity, r_HealthComponent, r_ThinkingComponent, r_TeamComponent)
	, state(CONSTRUCTING)
	, constructionHasFinished(false)
	, marked(false) {
	REGISTER_THINKER(Think, ThinkingComponent::SCHEDULER_AVERAGE, 100);

	// TODO: Make power state a member variable.
	entity.oldEnt->powered = true;
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

	if (Powered()) {
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

	TeamComponent::team_t team = GetTeamComponent().Team();

	// TODO: Move animation code to BuildableComponent.
	G_SetBuildableAnim(entity.oldEnt, Powered() ? BANIM_DESTROY : BANIM_DESTROY_UNPOWERED, true);
	G_SetIdleBuildableAnim(entity.oldEnt, BANIM_DESTROYED);

	entity.oldEnt->killedBy = killer->s.number;

	G_LogDestruction(entity.oldEnt, killer, meansOfDeath);

	// TODO: Handle in TaggableComponent.
	Beacon::DetachTags(entity.oldEnt);

	// Report an attack to the defending team if the buildable was powered and there is a main
	// buildable that can report it. Note that the main buildables itself issues its own warnings.
	if (Powered() && entity.Get<MainBuildableComponent>() == nullptr &&
	    G_IsWarnableMOD(meansOfDeath) && G_ActiveMainBuildable(team)) {
		// Get a nearby location entity.
		gentity_t *location = GetCloseLocationEntity(entity.oldEnt);

		// Fall back to fake location entity if necessary.
		if (!location) location = level.fakeLocation;

		// Warn if there was no warning for this location recently.
		if (level.time > location->warnTimer) {
			bool inBase = G_InsideBase(entity.oldEnt);

			G_BroadcastEvent(EV_WARN_ATTACK, inBase ? 0 : location->s.number, team);
			Beacon::NewArea(BCT_DEFEND, entity.oldEnt->s.origin, team);
			location->warnTimer = level.time + ATTACKWARN_NEARBY_PERIOD;
		}
	}

	// If not deconstructed, add all build points to queue.
	if (meansOfDeath != MOD_DECONSTRUCT && meansOfDeath != MOD_REPLACE) {
		G_FreeBudget(team, 0, BG_Buildable(entity.oldEnt->s.modelindex)->buildPoints);
	}
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

				if (Powered() && G_ActiveMainBuildable(GetTeamComponent().Team())) {
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

				// Notify other components when construction finishes.
				if (!constructionHasFinished) {
					entity.FinishConstruction();
					constructionHasFinished = true;
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

void BuildableComponent::SetPowerState(bool powered) {
	// TODO: Make power state a member variable.
	if (entity.oldEnt->powered == powered) return;

	bool wasPowered = entity.oldEnt->powered;

	entity.oldEnt->powered = powered;

	if (powered && !wasPowered) {
		G_SetBuildableAnim(entity.oldEnt, BANIM_POWERUP, false);
		G_SetIdleBuildableAnim(entity.oldEnt, BANIM_IDLE1);

		entity.PowerUp();
	} else if (!powered &&  wasPowered) {
		G_SetBuildableAnim(entity.oldEnt, BANIM_POWERDOWN, false);
		G_SetIdleBuildableAnim(entity.oldEnt, BANIM_IDLE_UNPOWERED);

		entity.PowerDown();
	}
}
