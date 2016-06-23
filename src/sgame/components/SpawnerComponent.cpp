#include "SpawnerComponent.h"

SpawnerComponent::SpawnerComponent(Entity& entity, TeamComponent& r_TeamComponent)
	: SpawnerComponentBase(entity, r_TeamComponent)
	, dying(false) {
	level.team[GetTeamComponent().Team()].numSpawns++;
}

SpawnerComponent::~SpawnerComponent() {
	if (!dying) {
		OnRemoval();
	}
}

void SpawnerComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	OnRemoval();
	dying = true;
}

void SpawnerComponent::OnRemoval() {
	TeamComponent::team_t team = GetTeamComponent().Team();

	int newNumSpawns = --level.team[team].numSpawns;

	ASSERT_GE(newNumSpawns, 0);

	// Warn if the last spawn is lost and there is a main buildable.
	if (newNumSpawns == 0 && G_ActiveMainBuildable(team)) {
		G_BroadcastEvent(EV_NO_SPAWNS, 0, team);
	}
}
