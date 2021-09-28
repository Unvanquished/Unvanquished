/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2014 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished. If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "sg_local.h"
#include "CBSE.h"

static Log::Logger buildpointLogger("sgame.buildpoints");

/**
 * @brief Predict the efficiency of a mining structure constructed at the given point.
 * @note Creates a pseudo leech entity on the stack and calculate its rate.
 * @return Predicted efficiency of the new miner only.
 */
float G_RGSPredictOwnEfficiency(vec3_t origin) {
	gentity_t leechOldEnt;
	VectorCopy(origin, leechOldEnt.s.origin);

	LeechEntity::Params params;
	params.oldEnt           = &leechOldEnt;
	params.Health_maxHealth = 1.0f;
	params.Mining_blueprint = true;
	LeechEntity leechEntity(params);

	return leechEntity.Get<MiningComponent>()->Efficiency(true);
}

/**
 * @brief Predict the efficiency loss of an existing miner if another one is constructed closeby.
 * @return Efficiency loss as negative value.
 */
static float RGSPredictEfficiencyLoss(Entity& miner, vec3_t newMinerOrigin) {
	float distance               = Distance(miner.oldEnt->s.origin, newMinerOrigin);
	float oldPredictedEfficiency = miner.Get<MiningComponent>()->Efficiency(true);
	float newPredictedEfficiency = oldPredictedEfficiency * MiningComponent::InterferenceMod(distance);
	float efficiencyLoss         = newPredictedEfficiency - oldPredictedEfficiency;

	buildpointLogger.Debug("Predicted efficiency loss of existing miner: %f - %f = %f.",
	                       oldPredictedEfficiency, newPredictedEfficiency, efficiencyLoss);

	return efficiencyLoss;
}

/**
 * @brief Predict the total efficiency gain for a team when a miner is constructed at a given point.
 * @return Predicted efficiency delta in percent points.
 * @todo Consider RGS set for deconstruction.
 */
float G_RGSPredictEfficiencyDelta(vec3_t origin, team_t team) {
	float delta = G_RGSPredictOwnEfficiency(origin);

	buildpointLogger.Debug("Predicted efficiency of new miner itself: %f.", delta);

	ForEntities<MiningComponent>([&] (Entity& miner, MiningComponent&) {
		if (G_Team(miner.oldEnt) != team) return;

		delta += RGSPredictEfficiencyLoss(miner, origin);
	});

	buildpointLogger.Debug("Predicted efficiency delta: %f. Build point delta: %f.", delta,
	                       delta * g_buildPointBudgetPerMiner.Get());

	return delta;
}

/**
 * @brief Calculate the build point budgets for both teams.
 */
void G_UpdateBuildPointBudgets() {
	for (team_t team = TEAM_NONE; (team = G_IterateTeams(team)); ) {
		level.team[team].totalBudget = g_buildPointInitialBudget.Get();
	}

	ForEntities<MiningComponent>([&] (Entity& entity, MiningComponent& miningComponent) {
		level.team[G_Team(entity.oldEnt)].totalBudget += miningComponent.Efficiency() *
		                                                 g_buildPointBudgetPerMiner.Get();
	});
}

void G_RecoverBuildPoints() {
	static int nextBuildPoint[NUM_TEAMS] = {0};

	float rate = g_buildPointRecoveryInitialRate.Get() /
	             std::pow(2.0f, (float)level.matchTime /
	                            (60000.0f * g_buildPointRecoveryRateHalfLife.Get()));
	float interval = 60000.0f / rate;

	// The interval grows exponentially, so check for an excessively large value which could cause overflow.
	// The maximum allowed game time is 0x70000000 (7 << 28); limit interval to 1 << 27.
	if (interval > 0x1.0p27f) return;

	int nextBuildPointTime = level.time + int(interval);

	for (team_t team = TEAM_NONE; (team = G_IterateTeams(team)); ) {
		if (!level.team[team].queuedBudget) {
			nextBuildPoint[team] = -1;
			continue;
		}

		if (nextBuildPoint[team] == -1) {
			nextBuildPoint[team] = nextBuildPointTime;
			continue;
		}

		if (nextBuildPoint[team] <= level.time) {
			nextBuildPoint[team] = nextBuildPointTime;
			level.team[team].queuedBudget--;
		}
	}
}

/**
 * @brief Get the potentially negative number of free build points for a team.
 */
int G_GetFreeBudget(team_t team)
{
	return (int)level.team[team].totalBudget - (level.team[team].spentBudget + level.team[team].queuedBudget);
}

/**
 * @brief Get the number of marked build points for a team.
 */
int G_GetMarkedBudget(team_t team)
{
	int sum = 0;

	ForEntities<BuildableComponent>(
	[&](Entity& entity, BuildableComponent& buildableComponent) {
		if (G_Team(entity.oldEnt) == team && buildableComponent.MarkedForDeconstruction()) {
			sum += G_BuildableDeconValue(entity.oldEnt);
		}
	});

	return sum;
}

/**
 * @brief Get the potentially negative number of build points a team can spend, including those from
 *        marked buildables.
 */
int G_GetSpendableBudget(team_t team)
{
	return G_GetFreeBudget(team) + G_GetMarkedBudget(team);
}

void G_FreeBudget( team_t team, int immediateAmount, int queuedAmount )
{
	if ( G_IsPlayableTeam( team ) )
	{
		level.team[ team ].spentBudget  -= (immediateAmount + queuedAmount);
		level.team[ team ].queuedBudget += queuedAmount;

		// Note that there can be more build points in queue than total - spent.

		if ( level.team[ team ].spentBudget < 0 ) {
			level.team[ team ].spentBudget = 0;
			Log::Warn("A team spent a negative buildable budget total.");
		}
	}
}

void G_SpendBudget( team_t team, int amount )
{
	if ( G_IsPlayableTeam( team ) )
	{
		level.team[ team ].spentBudget += amount;
	}
}

int G_BuildableDeconValue(gentity_t *ent)
{
	HealthComponent* healthComponent = ent->entity->Get<HealthComponent>();

	if (!healthComponent->Alive()) {
		return 0;
	}

	return (int)roundf((float)BG_Buildable(ent->s.modelindex)->buildPoints
	                   * healthComponent->HealthFraction());
}

/**
 * @brief Calculates the current value of buildables (in build points) for both teams.
 */
void G_GetTotalBuildableValues(int *buildableValuesByTeam)
{
	for (team_t team = TEAM_NONE; (team = G_IterateTeams(team)); ) {
		buildableValuesByTeam[team] = 0;
	}

	ForEntities<BuildableComponent>([&](Entity& entity, BuildableComponent&) {
		buildableValuesByTeam[G_Team(entity.oldEnt)] += G_BuildableDeconValue(entity.oldEnt);
	});
}
