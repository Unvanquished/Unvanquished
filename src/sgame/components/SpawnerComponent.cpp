#include "SpawnerComponent.h"

constexpr int   BLOCKER_GRACE_PERIOD = 4000;
constexpr int   BLOCKER_WARN_PERIOD  = 2000;
constexpr float BLOCKER_DAMAGE       = 10.0f;

SpawnerComponent::SpawnerComponent(Entity& entity, TeamComponent& r_TeamComponent,
	ThinkingComponent& r_ThinkingComponent)
	: SpawnerComponentBase(entity, r_TeamComponent, r_ThinkingComponent)
	, dying(false)
	, blockTime(0)
{
	REGISTER_THINKER(Think, ThinkingComponent::SCHEDULER_AVERAGE, 500);
	level.team[GetTeamComponent().Team()].numSpawns++;
}

SpawnerComponent::~SpawnerComponent() {
	if (!dying) {
		OnLoss();
	}
}

void SpawnerComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	OnLoss();
	dying = true;
}

void SpawnerComponent::OnLoss() {
	TeamComponent::team_t team = GetTeamComponent().Team();

	int newNumSpawns = --level.team[team].numSpawns;

	ASSERT_GE(newNumSpawns, 0);

	// Warn if the last spawn is lost and there is a main buildable.
	if (newNumSpawns == 0 && G_ActiveMainBuildable(team)) {
		G_BroadcastEvent(EV_NO_SPAWNS, 0, team);
	}
}

void SpawnerComponent::Think(int timeDelta) {
	BuildableComponent *buildableComponent = entity.Get<BuildableComponent>();

	if (buildableComponent && !buildableComponent->Active()) return;

	Entity* blocker = GetBlocker();

	if (blocker) {
		// Suicide if blocked by the map.
		if (blocker->oldEnt->s.number == ENTITYNUM_WORLD
		    || blocker->oldEnt->s.eType == entityType_t::ET_MOVER) {
			Utility::Kill(entity, nullptr, MOD_SUICIDE);
		}

		// Free a blocking corpse.
		else if (blocker->oldEnt->s.eType == entityType_t::ET_CORPSE) {
			G_FreeEntity(blocker->oldEnt);
		}

		else if (Utility::OnSameTeam(entity, *blocker)) {
			// Suicide if blocked by own main buildable.
			if (blocker->Get<MainBuildableComponent>()) {
				Utility::Kill(entity, nullptr, MOD_SUICIDE);
			}

			// Kill a friendly blocking buildable.
			else if (blocker->Get<BuildableComponent>()) {
				Utility::Kill(*blocker, nullptr, MOD_SUICIDE);

				// Play an animation so it's clear what destroyed the buildable.
				G_SetBuildableAnim(entity.oldEnt, BANIM_SPAWN1, true);
			}

			// Do periodic damage to a friendly client.
			// TODO: Externalize constants.
			else  if (blocker->Get<ClientComponent>() && g_antiSpawnBlock.integer) {
				blockTime += timeDelta;

				if (blockTime > BLOCKER_GRACE_PERIOD
				    && blockTime - timeDelta <= BLOCKER_GRACE_PERIOD) {
					WarnBlocker(*blocker, false);
				}

				if (blockTime > BLOCKER_GRACE_PERIOD + BLOCKER_WARN_PERIOD) {
					if (blockTime - timeDelta <= BLOCKER_GRACE_PERIOD + BLOCKER_WARN_PERIOD) {
						WarnBlocker(*blocker, true);
					}

					blocker->Damage(
						BLOCKER_DAMAGE * ((float)timeDelta / 1000.0f), entity.oldEnt, {}, {},
						DAMAGE_PURE, MOD_TRIGGER_HURT
					);
				}
			}
		}
	} else if (g_antiSpawnBlock.integer) {
		blockTime = Math::Clamp(blockTime - timeDelta, 0, BLOCKER_GRACE_PERIOD);
	}
}

// TODO: Inline and refactor spawn checking.
//       Possibly add a message to retrieve the blocker from different spawners.
Entity* SpawnerComponent::GetBlocker() {
	buildable_t buildable;

	if (entity.Get<EggComponent>()) {
		buildable = BA_A_SPAWN;
	} else if (entity.Get<TelenodeComponent>()) {
		buildable = BA_H_SPAWN;
	} else {
		buildable = BA_NONE;
	}

	gentity_t* blockerOldEnt = G_CheckSpawnPoint(
		entity.oldEnt->s.number, entity.oldEnt->s.origin, entity.oldEnt->s.origin2, buildable,
		nullptr
	);

	if (blockerOldEnt) {
		return blockerOldEnt->entity;
	} else {
		return nullptr;
	}
}

void SpawnerComponent::WarnBlocker(Entity& blocker, bool lastWarning) {
	std::string message = lastWarning ? Color::ToString(Color::Red)
	                                  : Color::ToString(Color::Yellow);

	switch (GetTeamComponent().Team()) {
		case TEAM_ALIENS:
			message += "You are blocking an egg!";
			break;

		case TEAM_HUMANS:
			message += "You are blocking a telenode!";
			break;

		default:
			message += "You are blocking a spawn!";
			break;
	}

	message = "cp \"" + message + "\"";

	trap_SendServerCommand(blocker.oldEnt - g_entities, message.c_str());
}
