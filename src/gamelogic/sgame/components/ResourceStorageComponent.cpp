#include "ResourceStorageComponent.h"

static Log::Logger resourceStorageLogger("sgame.bpstorage");

ResourceStorageComponent::ResourceStorageComponent(Entity& entity)
	: ResourceStorageComponentBase(entity), acquiredBuildPoints(0.0f)
{}

void ResourceStorageComponent::HandlePrepareNetCode() {
	// Fraction of total BP stored by the team.
	entity.oldEnt->s.weapon = (int)std::round(255.0f * GetStoredFraction());
}

void ResourceStorageComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	// TODO: Add TeamComponent and/or Utility::Team.
	team_t team           = entity.oldEnt->buildableTeam;
	float  storedFraction = GetStoredFraction();

	// Removes some of the owner's team's build points, proportional to the amount this structure
	// acquired and the amount of health lost (before deconstruction).
	float loss = (1.0f - entity.oldEnt->deconHealthFrac) * storedFraction *
	             level.team[team].buildPoints * g_buildPointLossFraction.value;

	resourceStorageLogger.Notice(
		"A resource storage died, removing %.0f BP from the team's pool "
		"(%.1f × %.0f%% stored × %.0f%% health lost × %.0f total).",
		loss, g_buildPointLossFraction.value, 100.0f * storedFraction,
		100.0f * (1.0f - entity.oldEnt->deconHealthFrac), level.team[team].buildPoints
	);

	G_ModifyBuildPoints(team, -loss);

	// Main structures keep their account of acquired build points across lifecycles, it's saved
	// in a per-team variable and copied over to the ResourceStorageComponent whenever it's modified.
	if (!entity.Get<MainBuildableComponent>()) {
		G_ModifyTotalBuildPointsAcquired(team, -acquiredBuildPoints);
	}

	acquiredBuildPoints = 0.0f;
}

float ResourceStorageComponent::GetStoredFraction() {
	// TODO: Add TeamComponent and/or Utility::Team.
	team_t team = entity.oldEnt->buildableTeam;

	if (!level.team[team].acquiredBuildPoints) return 1.0f;

	// The stored fraction is equal to the acquired fraction.
	float storedFraction = acquiredBuildPoints / level.team[team].acquiredBuildPoints;

	if (storedFraction < 0.0f || storedFraction > 1.0f) {
		resourceStorageLogger.Warn(
			"A resource storage stores an invalid fraction of all build points: %.1f", storedFraction
		);
	}

	return storedFraction;
}
