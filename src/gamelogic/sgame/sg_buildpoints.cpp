/*
===========================================================================

Copyright 2014 Unvanquished Developers

This file is part of Unvanquished.

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

#define MINING_PERIOD 1000

/**
 * @brief Predict the efficiecy loss of a mining structre if another one is constructed closeby.
 * @return Efficiency loss as negative value.
 */
static float RGSPredictInterferenceLoss(Entity& miner, vec3_t newMinerOrigin) {
	float distance      = Distance(miner.oldEnt->s.origin, newMinerOrigin);
	float currentRate   = miner.Get<MiningComponent>()->MineRate();
	float predictedRate = currentRate * MiningComponent::InterferenceMod(distance);
	float rateLoss      = predictedRate - currentRate;

	return rateLoss / level.mineRate;
}

/**
 * @brief Predict the efficiency of a mining structure constructed at the given point.
 * @return Predicted efficiency in percent points.
 */
float G_RGSPredictEfficiency( vec3_t origin ) {
	// HACK: Create a pseudo leech entity on the stack and calculate its rate.

	gentity_t leechOldEntity;
	VectorCopy(origin, leechOldEntity.s.origin);
	leechOldEntity.powered = true;

	LeechEntity::Params params;
	params.oldEnt = &leechOldEntity;
	params.Health_maxHealth = 1.0f;

	LeechEntity leechEntity(params);

	MiningComponent *miningComponent = leechEntity.Get<MiningComponent>();
	BuildableComponent *buildableComponent = leechEntity.Get<BuildableComponent>();

	buildableComponent->SetState(BuildableComponent::CONSTRUCTED);
	miningComponent->CalculateEfficiency();
	return miningComponent->Efficiency();
}

/**
 * @brief Predict the total efficiency gain for a team when a miner is constructed at a given point.
 * @return Predicted efficiency delta in percent points.
 * @todo Consider RGS set for deconstruction.
 */
float G_RGSPredictEfficiencyDelta(vec3_t origin, team_t team) {
	float delta = G_RGSPredictEfficiency(origin);

	ForEntities<MiningComponent>([&] (Entity& miner, MiningComponent& miningComponent) {
		// HACK: This just works for miners that are buildables.
		// TODO: Retrieve entity team properly.
		if (miner.oldEnt->buildableTeam != team) return;

		delta += RGSPredictInterferenceLoss(miner, origin);
	});

	return delta;
}

/**
 * @brief Calculate the level mine rate and the teams' mining efficiencies, add build points.
 */
void G_MineBuildPoints() {
	static int nextCalculation = 0;
	if (level.time < nextCalculation) return;

	level.team[TEAM_HUMANS].mineEfficiency = 0.0f;
	level.team[TEAM_ALIENS].mineEfficiency = 0.0f;

	// Calculate current level wide mine rate.
	level.mineRate = g_initialMineRate.value *
	                 std::pow(2.0f, -level.matchTime / (60000.0f * g_mineRateHalfLife.value));

	// Calculate current efficiency to build point gain modifier.
	float mineMod = (level.mineRate / 60.0f) * (MINING_PERIOD / 1000.0f);

	// Sum up efficiencies of miners and save amount of build points acquired by each miner.
	ForEntities<MiningComponent>([&] (Entity& miner, MiningComponent& miningComponent) {
		float efficiency = miningComponent.Efficiency();

		miningComponent.GetResourceStorageComponent().AcquireBuildPoints(efficiency * mineMod);

		// HACK: This only works with miners that are buildables.
		level.team[miner.oldEnt->buildableTeam].mineEfficiency += efficiency;
	});

	// Main structure provides the rest of the minimum mining efficiency.
	float minEff = (g_minimumMineRate.value / 100.0f);
	for (team_t team = TEAM_NONE; (team = G_IterateTeams(team)); ) {
		gentity_t *mainStructure;
		float     deltaEff;

		switch (team) {
			case TEAM_ALIENS: mainStructure = G_ActiveOvermind(); break;
			case TEAM_HUMANS: mainStructure = G_ActiveReactor();  break;
			default: continue;
		}

		if (mainStructure && (deltaEff = minEff - level.team[TEAM_HUMANS].mineEfficiency) > 0) {
			level.team[team].mineEfficiency       += deltaEff;
			level.team[team].mainStructAcquiredBP += deltaEff * mineMod;

			// Copy acquired build points to the current main structure's ResourceStorageComponent.
			mainStructure->entity->Get<ResourceStorageComponent>()->SetAcquiredBuildPoints(
				level.team[team].mainStructAcquiredBP
			);
		}
	}

	// Add build points.
	for (team_t team = TEAM_NONE; (team = G_IterateTeams(team)); ) {
		float earnedBP = level.team[team].mineEfficiency * mineMod;

		G_ModifyBuildPoints(team, earnedBP);
		G_ModifyTotalBuildPointsAcquired(team, earnedBP);
	}

	// Send mine efficiencies to clients.
	for (int playerNum = 0; playerNum < level.maxclients; playerNum++) {
		gentity_t *player = &g_entities[playerNum];
		gclient_t *client = player->client;

		if (!client) continue;

		client->ps.persistant[PERS_MINERATE] = (short)(level.mineRate * 10.0f);

		team_t team = (team_t)client->pers.team;

		if (G_IsPlayableTeam(team)) {
			client->ps.persistant[PERS_RGS_EFFICIENCY] =
				(short)(level.team[team].mineEfficiency * 100.0f);
		} else {
			// TODO: Check if this is related to efficiency display flickering for spectators.
			client->ps.persistant[PERS_RGS_EFFICIENCY] = 0;
		}
	}

	nextCalculation = level.time + MINING_PERIOD;
}

/**
 * @brief Get the number of build points for a team.
 */
int G_GetBuildPointsInt( team_t team )
{
	if ( G_IsPlayableTeam( team ) )
	{
		return ( int )level.team[ team ].buildPoints;
	}
	else
	{
		return 0;
	}
}

/**
 * @brief Get the number of marked build points for a team.
 */
int G_GetMarkedBuildPointsInt( team_t team )
{
	gentity_t *ent;
	int       i;
	int       sum = 0;
	const buildableAttributes_t *attr;

	for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( !ent->inuse || ent->s.eType != ET_BUILDABLE || G_Dead( ent ) ||
		     ent->buildableTeam != team || !ent->entity->Get<BuildableComponent>()->MarkedForDeconstruction() )
		{
			continue;
		}

		attr = BG_Buildable( ent->s.modelindex );
		sum += attr->buildPoints * ent->entity->Get<HealthComponent>()->HealthFraction();
	}

	return sum;
}

/**
 * @brief Tests wether a team can afford an amont of build points.
 * @param amount Amount of build points, the sign is discarded.
 */
bool G_CanAffordBuildPoints( team_t team, float amount )
{
	float *bp;

	if ( G_IsPlayableTeam( team ) )
	{
		bp = &level.team[ team ].buildPoints;
	}
	else
	{
		return false;
	}

	if ( fabs( amount ) > *bp )
	{
		return false;
	}
	else
	{
		return true;
	}
}

/**
 * @brief Calculates the value of buildables (in build points) for both teams.
 */
void G_GetBuildableResourceValue( int *teamValue )
{
	int       entityNum;
	gentity_t *ent;
	team_t    team;
	const buildableAttributes_t *attr;

	for ( team = TEAM_NONE; ( team = G_IterateTeams( team ) ); )
	{
		teamValue[ team ] = 0;
	}

	for ( entityNum = MAX_CLIENTS; entityNum < level.num_entities; entityNum++ )
	{
		ent = &g_entities[ entityNum ];

		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		team = ent->buildableTeam ;
		attr = BG_Buildable( ent->s.modelindex );

		teamValue[ team ] += attr->buildPoints * ent->entity->Get<HealthComponent>()->HealthFraction();
	}
}

static void ModifyBuildPoints( team_t team, float amount, bool acquired )
{
	float *availBP, *acquiredBP;

	if ( G_IsPlayableTeam( team ) )
	{
		availBP    = &level.team[ team ].buildPoints;
		acquiredBP = &level.team[ team ].acquiredBuildPoints;
	}
	else
	{
		return;
	}

	*availBP    = acquired ? *availBP : std::max( *availBP + amount, 0.0f );
	*acquiredBP = acquired ? std::max( *acquiredBP + amount, 0.0f ) : *acquiredBP;
}

/**
 * @brief Adds or removes build points from a team.
 */
void G_ModifyBuildPoints( team_t team, float amount )
{
	ModifyBuildPoints( team, amount, false );
}

/**
 * @brief Adds or removes build points to the virtual pool of acquired build points.
 * @note The sum of all resource storage's acquired build points plus the pool of build points
 *       acquired by the main structure should equal this pool.
 */
void G_ModifyTotalBuildPointsAcquired( team_t team, float amount )
{
	ModifyBuildPoints( team, amount, true );
}
