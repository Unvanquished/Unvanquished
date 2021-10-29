#include "SpawnerComponent.h"
#include "../Entities.h"
#include "common/Util.h"

static Log::Logger logger("sgame.spawn");

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

void SpawnerComponent::HandleDie(gentity_t* /*killer*/, meansOfDeath_t /*meansOfDeath*/) {
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
		if (!blocker->oldEnt) {
			logger.Warn("Spawn blocking entity has oldEnt == nullptr");
			return;
		}

		// Suicide if blocked by the map.
		if (blocker->oldEnt->s.number == ENTITYNUM_WORLD
		    || blocker->oldEnt->s.eType == entityType_t::ET_MOVER) {
			Entities::Kill(entity, nullptr, MOD_SUICIDE);
		}

		// Free a blocking corpse.
		else if (blocker->oldEnt->s.eType == entityType_t::ET_CORPSE) {
			G_FreeEntity(blocker->oldEnt);
		}

		else if (Entities::OnSameTeam(entity, *blocker)) {
			// Suicide if blocked by own main buildable.
			if (blocker->Get<MainBuildableComponent>()) {
				Entities::Kill(entity, nullptr, MOD_SUICIDE);
			}

			// Kill a friendly blocking buildable.
			else if (blocker->Get<BuildableComponent>()) {
				Entities::Kill(*blocker, nullptr, MOD_SUICIDE);

				// Play an animation so it's clear what destroyed the buildable.
				G_SetBuildableAnim(entity.oldEnt, BANIM_SPAWN1, true);
			}

			// Do periodic damage to a friendly client.
			// TODO: Externalize constants.
			else  if (blocker->Get<ClientComponent>() && g_antiSpawnBlock.Get()) {
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
	} else if (g_antiSpawnBlock.Get()) {
		blockTime = Math::Clamp(blockTime - timeDelta, 0, BLOCKER_GRACE_PERIOD);
	}
}

Entity* SpawnerComponent::GetBlocker() {
	Entity* blocker = nullptr;
	Vec3    spawnPoint;

	entity.CheckSpawnPoint(blocker, spawnPoint);

	return blocker;
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

Entity* SpawnerComponent::CheckSpawnPointHelper(
	int spawnerNumber, const Vec3 spawnerOrigin, const Vec3 spawnPoint, const Vec3 clientMins,
	const Vec3 clientMaxs
){
	trace_t tr;

	// Check for a clear line towards the spawn location.
	trap_Trace(
		&tr, spawnerOrigin.Data(), nullptr, nullptr, spawnPoint.Data(), spawnerNumber, MASK_SHOT, 0
	);

	if (tr.entityNum != ENTITYNUM_NONE) {
		return g_entities[tr.entityNum].entity;
	} else {
		// Check whether a spawned client has space.
		trap_Trace(
			&tr, spawnPoint.Data(), clientMins.Data(), clientMaxs.Data(), spawnPoint.Data(), 0,
			MASK_PLAYERSOLID, 0
		);

		if (tr.entityNum != ENTITYNUM_NONE) {
			return g_entities[tr.entityNum].entity;
		}

		return nullptr;
	}
}
