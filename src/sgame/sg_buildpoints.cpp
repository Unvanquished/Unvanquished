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
	float delta = MiningComponent::FindEfficiencies(team, VEC2GLM(origin), nullptr).predicted;

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
	int abp = g_BPInitialBudgetAliens.Get();
	int hbp = g_BPInitialBudgetHumans.Get();
	for (team_t team = TEAM_NONE; (team = G_IterateTeams(team)); ) {
		if ( team == TEAM_ALIENS && abp >= 0 )
		{
			level.team[team].totalBudget = abp;
		}
		else if ( team == TEAM_HUMANS && hbp >= 0 )
		{
			level.team[team].totalBudget = hbp;
		}
		else
		{
			level.team[team].totalBudget = g_buildPointInitialBudget.Get();
		}
	}

	ForEntities<MiningComponent>([&] (Entity& entity, MiningComponent& miningComponent) {
		level.team[G_Team(entity.oldEnt)].totalBudget += miningComponent.Efficiency() *
		                                                 g_buildPointBudgetPerMiner.Get();
	});
}

/**
 * @brief Get the potentially negative number of free build points for a team.
 */
int G_GetFreeBudget(team_t team)
{
	return (int)level.team[team].totalBudget - level.team[team].spentBudget;
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
			sum += G_BuildableCost(entity.oldEnt);
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

void G_FreeBudget( team_t team, int amount )
{
	if ( G_IsPlayableTeam( team ) )
	{
		level.team[ team ].spentBudget  -= amount;

		if ( level.team[ team ].spentBudget < 0 ) {
			level.team[ team ].spentBudget = 0;
			Log::Warn("A team spent a negative amount of build points.");
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

/**
 * @brief The build point cost of a buildable.
 */
int G_BuildableCost(gentity_t *ent)
{
	return BG_Buildable(ent->s.modelindex)->buildPoints;
}

/**
 * @brief Calculates the health-adjusted value of buildables (in BP) for both teams.
 */
void G_GetTotalBuildableValues(int *buildableValuesByTeam)
{
	for (team_t team = TEAM_NONE; (team = G_IterateTeams(team)); ) {
		buildableValuesByTeam[team] = 0;
	}

	ForEntities<BuildableComponent, HealthComponent>([&](
			Entity& entity, BuildableComponent&, HealthComponent& healthComponent
	) {
		if (!healthComponent.Alive()) return;

		int cost = G_BuildableCost(entity.oldEnt);
		float vitality = healthComponent.HealthFraction();
		int value = (int)roundf((float)cost * vitality);

		buildableValuesByTeam[G_Team(entity.oldEnt)] += value;
	});
}
