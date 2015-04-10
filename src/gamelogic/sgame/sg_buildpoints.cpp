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

#include "g_local.h"

#define MINING_PERIOD 1000

/**
 * @brief Calculates modifier for the efficiency of one RGS when another one interfers at given
 *        distance.
 */
static float RGSInterferenceMod( float distance )
{
	float dr, q;

	if ( RGS_RANGE <= 0.0f )
	{
		return 1.0f;
	}

	// q is the ratio of the part of a sphere with radius RGS_RANGE that intersects
	// with another sphere of equal size and given distance
	dr = distance / RGS_RANGE;
	q = ((dr * dr * dr) - 12.0f * dr + 16.0f) / 16.0f;

	// Two RGS together should mine at a rate proportional to the volume of the
	// union of their areas of effect. If more RGS intersect, this is just an
	// approximation that tends to punish cluttering of RGS.
	return ( (1.0f - q) + 0.5f * q );
}

/**
 * @brief Adjust the rate of a RGS.
 */
static void RGSCalculateRate( gentity_t *self )
{
	gentity_t       *rgs;
	float           rate;

	if ( self->s.modelindex != BA_A_LEECH && self->s.modelindex != BA_H_DRILL )
	{
		return;
	}

	if ( !self->spawned || !self->powered )
	{
		self->mineRate       = 0.0f;
		self->mineEfficiency = 0.0f;

		// HACK: Save RGS mining efficiency in entityState.weaponAnim
		self->s.weaponAnim = 0;
	}
	else
	{
		rate = level.mineRate;

		for ( rgs = NULL;
		      ( rgs = G_IterateEntitiesWithinRadius( rgs, self->s.origin, RGS_RANGE * 2.0f ) ); )
		{
			if ( rgs->s.eType == ET_BUILDABLE &&
			     ( rgs->s.modelindex == BA_H_DRILL || rgs->s.modelindex == BA_A_LEECH )
				 && rgs != self && rgs->spawned && rgs->powered && rgs->health > 0 )
			{
				rate *= RGSInterferenceMod( Distance( self->s.origin, rgs->s.origin ) );
			}
		}

		self->mineRate       = rate;
		self->mineEfficiency = rate / level.mineRate;

		// HACK: Save RGS mining efficiency in entityState.weaponAnim
		self->s.weaponAnim = std::round( self->mineEfficiency * 255.0f );
	}
}

/**
 * @brief Adjust the rate of all RGS in range.
 */
static void RGSInformNeighbors( gentity_t *self )
{
	gentity_t *rgs;

	for ( rgs = NULL;
	      ( rgs = G_IterateEntitiesWithinRadius( rgs, self->s.origin, RGS_RANGE * 2.0f ) ); )
	{
		if ( rgs->s.eType == ET_BUILDABLE &&
		     ( rgs->s.modelindex == BA_H_DRILL || rgs->s.modelindex == BA_A_LEECH )
			 && rgs != self && rgs->spawned && rgs->powered && rgs->health > 0 )
		{
			RGSCalculateRate( rgs );
		}
	}
}

/**
 * @brief Called when a RGS thinks.
 */
void G_RGSThink( gentity_t *self )
{
	bool active          = ( self->spawned && self->powered );
	bool lastThinkActive = ( self->mineEfficiency > 0.0f );

	if ( active ^ lastThinkActive )
	{
		// If the state of this RGS has changed, adjust own rate and inform active closeby RGS so
		// they can adjust their rate immediately.
		RGSCalculateRate( self );
		RGSInformNeighbors( self );
	}

	// HACK: Save fraction of stored build points in entityState.weapon
	self->s.weapon = std::round( ( 255.0f * self->acquiredBuildPoints ) /
	                             level.team[ self->buildableTeam ].acquiredBuildPoints );
}

/**
 * @brief Called when a main structure thinks in its role as a BP storage.
 */
void G_MainStructBPStorageThink( gentity_t *self )
{
	self->acquiredBuildPoints = level.team[ self->buildableTeam ].mainStructAcquiredBP;

	// HACK: Save fraction of stored build points in entityState.weapon
	self->s.weapon = std::round( ( 255.0f * self->acquiredBuildPoints ) /
	                             level.team[ self->buildableTeam ].acquiredBuildPoints );
}

/**
 * @brief Called when a BP storage dies or gets deconstructed.
 */
void G_BPStorageDie( gentity_t *self )
{
	team_t team = self->buildableTeam;

	// HACK: entityState.weapon stores BP fraction
	self->s.weapon = 0;

	// Removes some of the owner's team's build points, proportional to the amount this structure
	// acquired and the amount of health lost (before deconstruction).
	float acquiredFrac = self->acquiredBuildPoints / level.team[ team ].acquiredBuildPoints;
	float loss         = ( 1.0f - self->deconHealthFrac ) * std::min( acquiredFrac, 1.0f ) *
	                     level.team[ team ].buildPoints * g_buildPointLossFraction.value;
	G_ModifyBuildPoints( team, -loss );
}

/**
 * @brief Called when a RGS dies or gets deconstructed.
 */
void G_RGSDie( gentity_t *self )
{
	// HACK: entityState.weaponAnim stores mining efficiency
	self->s.weaponAnim = 0;

	G_BPStorageDie( self );

	// The remaining BP storages take over this one's account
	G_MarkBuildPointsMined( self->buildableTeam, -self->acquiredBuildPoints );

	// Inform neighbours so they can increase their rate immediately.
	RGSInformNeighbors( self );
}

/**
 * @brief Predict the efficiecy loss of a RGS if another one is constructed at given origin.
 * @return Efficiency loss as negative value
 */
static float RGSPredictInterferenceLoss( gentity_t *self, vec3_t origin )
{
	float currentRate, predictedRate, rateLoss;

	currentRate   = self->mineRate;
	predictedRate = currentRate * RGSInterferenceMod( Distance( self->s.origin, origin ) );
	rateLoss      = predictedRate - currentRate;

	return ( rateLoss / level.mineRate );
}

/**
 * @brief Predict the efficiency of a RGS constructed at the given point.
 * @return Predicted efficiency in percent points
 */
float G_RGSPredictEfficiency( vec3_t origin )
{
	gentity_t dummy;

	memset( &dummy, 0, sizeof( gentity_t ) );
	VectorCopy( origin, dummy.s.origin );
	dummy.s.eType = ET_BUILDABLE;
	dummy.s.modelindex = BA_A_LEECH;
	dummy.spawned = qtrue;
	dummy.powered = qtrue;

	RGSCalculateRate( &dummy );

	return dummy.mineEfficiency;
}

/**
 * @brief Predict the total efficiency gain for a team when a RGS is constructed at the given point.
 * @return Predicted efficiency delta in percent points
 * @todo Consider RGS set for deconstruction
 */
float G_RGSPredictEfficiencyDelta( vec3_t origin, team_t team )
{
	gentity_t *rgs;
	float     delta;

	delta = G_RGSPredictEfficiency( origin );

	for ( rgs = NULL; ( rgs = G_IterateEntitiesWithinRadius( rgs, origin, RGS_RANGE * 2.0f ) ); )
	{
		if ( rgs->s.eType == ET_BUILDABLE &&
		     ( rgs->s.modelindex == BA_H_DRILL || rgs->s.modelindex == BA_A_LEECH )
		     && rgs->spawned && rgs->powered && rgs->health > 0 && rgs->buildableTeam == team )
		{
			delta += RGSPredictInterferenceLoss( rgs, origin );
		}
	}

	return delta;
}

/**
 * @brief Calculate the level mine rate and the teams' mining efficiencies, add build points.
 */
void G_MineBuildPoints( void )
{
	int              i, playerNum;
	gentity_t        *ent, *player;
	gclient_t        *client;
	float            mineMod;

	static int       nextCalculation = 0;

	if ( level.time < nextCalculation )
	{
		return;
	}

	level.team[ TEAM_HUMANS ].mineEfficiency = 0.0f;
	level.team[ TEAM_ALIENS ].mineEfficiency = 0.0f;

	// calculate level wide mine rate
	level.mineRate = g_initialMineRate.value *
	                 std::pow( 2.0f, -level.matchTime / ( 60000.0f * g_mineRateHalfLife.value ) );

	// calculate efficiency to build point gain modifier
	mineMod = ( level.mineRate / 60.0f ) * ( MINING_PERIOD / 1000.0f );

	// sum up mine rates of RGS, store how many build points they mined
	for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( ent->s.eType != ET_BUILDABLE ) continue;

		ent->acquiredBuildPoints += ent->mineEfficiency * mineMod;

		switch ( ent->s.modelindex )
		{
			case BA_H_DRILL:
				level.team[ TEAM_HUMANS ].mineEfficiency += ent->mineEfficiency;
				break;

			case BA_A_LEECH:
				level.team[ TEAM_ALIENS ].mineEfficiency += ent->mineEfficiency;
				break;
		}
	}

	// main structure provides the rest of the minimum mining efficiency
	float minEff = ( g_minimumMineRate.value / 100.0f ), deltaEff;
	if ( G_ActiveReactor() && ( deltaEff = minEff - level.team[ TEAM_HUMANS ].mineEfficiency ) > 0 )
	{
		level.team[ TEAM_HUMANS ].mineEfficiency       += deltaEff;
		level.team[ TEAM_HUMANS ].mainStructAcquiredBP += deltaEff * mineMod;
	}
	if ( G_ActiveOvermind() &&( deltaEff = minEff - level.team[ TEAM_ALIENS ].mineEfficiency ) > 0 )
	{
		level.team[ TEAM_ALIENS ].mineEfficiency       += deltaEff;
		level.team[ TEAM_ALIENS ].mainStructAcquiredBP += deltaEff * mineMod;
	}

	// add build points, mark them mined
	for ( team_t team = TEAM_NONE; ( team = G_IterateTeams( team ) ); )
	{
		float earnedBP = mineMod * level.team[ team ].mineEfficiency;

		G_ModifyBuildPoints( team, earnedBP );
		G_MarkBuildPointsMined( team, earnedBP );
	}

	// send to clients
	for ( playerNum = 0; playerNum < level.maxclients; playerNum++ )
	{
		player = &g_entities[ playerNum ];
		client = player->client;

		if ( !client ) continue;

		client->ps.persistant[ PERS_MINERATE ] = ( short )( level.mineRate * 10.0f );

		team_t team = (team_t)client->pers.team;

		if ( G_IsPlayableTeam( team ) )
		{
			client->ps.persistant[ PERS_RGS_EFFICIENCY ] =
				( short )( level.team[ team ].mineEfficiency * 100.0f );
		}
		else
		{
			// TODO: Check if this is related to efficiency display flickering for spectators
			client->ps.persistant[ PERS_RGS_EFFICIENCY ] = 0;
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
		if ( ent->s.eType != ET_BUILDABLE || !ent->inuse || ent->health <= 0 ||
		     ent->buildableTeam != team || !ent->deconstruct )
		{
			continue;
		}

		attr = BG_Buildable( ent->s.modelindex );
		sum += attr->buildPoints * ( ent->health / (float)attr->health );
	}

	return sum;
}

/**
 * @brief Tests wether a team can afford an amont of build points.
 * @param amount Amount of build points, the sign is discarded.
 */
qboolean G_CanAffordBuildPoints( team_t team, float amount )
{
	float *bp;

	if ( G_IsPlayableTeam( team ) )
	{
		bp = &level.team[ team ].buildPoints;
	}
	else
	{
		return qfalse;
	}

	if ( fabs( amount ) > *bp )
	{
		return qfalse;
	}
	else
	{
		return qtrue;
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

		teamValue[ team ] += ( attr->buildPoints * MAX( 0, ent->health ) ) / attr->health;
	}
}

static void ModifyBuildPoints( team_t team, float amount, bool mined )
{
	float *availBP, *minedBP;

	if ( G_IsPlayableTeam( team ) )
	{
		availBP = &level.team[ team ].buildPoints;
		minedBP = &level.team[ team ].acquiredBuildPoints;
	}
	else
	{
		return;
	}

	*availBP = mined ? *availBP : std::max( *availBP + amount, 0.0f );
	*minedBP = mined ? std::max( *minedBP + amount, 0.0f ) : *minedBP;
}

/**
 * @brief Adds or removes build points from a team.
 */
void G_ModifyBuildPoints( team_t team, float amount )
{
	ModifyBuildPoints( team, amount, false );
}

/**
 * @brief Adds or removes build points to the virtual pool of mined build points.
 */
void G_MarkBuildPointsMined( team_t team, float amount )
{
	ModifyBuildPoints( team, amount, true );
}
