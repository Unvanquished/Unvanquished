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

#define CALCULATE_MINE_RATE_PERIOD 1000

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
void G_RGSCalculateRate( gentity_t *self )
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

		// HACK: Save rate and efficiency entityState.weapon and entityState.weaponAnim
		self->s.weapon     = 0;
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

		// HACK: Save rate and efficiency in entityState.weapon and entityState.weaponAnim
		self->s.weapon     = ( int )( self->mineRate * 1000.0f );
		self->s.weaponAnim = ( int )( self->mineEfficiency * 100.0f );

		// The transmitted rate must be positive to indicate that the RGS is active
		if ( self->s.weapon < 1 )
		{
			self->s.weapon = 1;
		}
	}
}

/**
 * @brief Adjust the rate of all RGS in range.
 */
void G_RGSInformNeighbors( gentity_t *self )
{
	gentity_t       *rgs;

	for ( rgs = NULL;
	      ( rgs = G_IterateEntitiesWithinRadius( rgs, self->s.origin, RGS_RANGE * 2.0f ) ); )
	{
		if ( rgs->s.eType == ET_BUILDABLE &&
		     ( rgs->s.modelindex == BA_H_DRILL || rgs->s.modelindex == BA_A_LEECH )
			 && rgs != self && rgs->spawned && rgs->powered && rgs->health > 0 )
		{
			G_RGSCalculateRate( rgs );
		}
	}
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

	G_RGSCalculateRate( &dummy );

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
 * @brief Recalculate the mine rate and the teams' mine efficiencies.
 */
void G_CalculateMineRate( void )
{
	int              i, playerNum;
	gentity_t        *ent, *player;
	gclient_t        *client;
	float            tmp;

	static int       nextCalculation = 0;

	if ( level.time < nextCalculation )
	{
		return;
	}

	level.team[ TEAM_HUMANS ].mineEfficiency = 0.0f;
	level.team[ TEAM_ALIENS ].mineEfficiency = 0.0f;

	// sum up mine rates of RGS
	for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

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

	// minimum mine rate
	// g_minimumMineRate is really a minimum mine efficiency in percent points
	if ( G_ActiveReactor() &&
	     level.team[ TEAM_HUMANS ].mineEfficiency < ( g_minimumMineRate.value / 100.0f ) )
	{
		level.team[ TEAM_HUMANS ].mineEfficiency = ( g_minimumMineRate.value / 100.0f );
	}
	if ( G_ActiveOvermind() &&
	     level.team[ TEAM_ALIENS ].mineEfficiency < ( g_minimumMineRate.value / 100.0f ) )
	{
		level.team[ TEAM_ALIENS ].mineEfficiency = ( g_minimumMineRate.value / 100.0f );
	}

	// calculate level wide mine rate. ln(2) ~= 0.6931472
	level.mineRate = g_initialMineRate.value *
	                 exp( ( -0.6931472f * level.matchTime ) /
	                      ( 60000.0f * g_mineRateHalfLife.value ) );

	// add build points
	tmp = ( level.mineRate / 60.0f ) * ( CALCULATE_MINE_RATE_PERIOD / 1000.0f );
	G_ModifyBuildPoints( TEAM_HUMANS, tmp * level.team[ TEAM_HUMANS ].mineEfficiency );
	G_ModifyBuildPoints( TEAM_ALIENS, tmp * level.team[ TEAM_ALIENS ].mineEfficiency );

	// send to clients
	for ( playerNum = 0; playerNum < level.maxclients; playerNum++ )
	{
		team_t team;

		player = &g_entities[ playerNum ];
		client = player->client;

		if ( !client )
		{
			continue;
		}

		team = (team_t) client->pers.team;

		client->ps.persistant[ PERS_MINERATE ] = ( short )( level.mineRate * 10.0f );

		if ( team > TEAM_NONE && team < NUM_TEAMS )
		{
			client->ps.persistant[ PERS_RGS_EFFICIENCY ] =
				( short )( level.team[ team ].mineEfficiency * 100.0f );
		}
		else
		{
			client->ps.persistant[ PERS_RGS_EFFICIENCY ] = 0;
		}
	}

	nextCalculation = level.time + CALCULATE_MINE_RATE_PERIOD;
}

/**
 * @brief Get the number of build points for a team.
 */
int G_GetBuildPointsInt( team_t team )
{
	if ( team > TEAM_NONE && team < NUM_TEAMS )
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

	//TODO write a function to check if a team is a playable one
	if ( TEAM_ALIENS == team || TEAM_HUMANS == team )
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
	int       team;
	const buildableAttributes_t *attr;

	for ( team = TEAM_NONE + 1; team < NUM_TEAMS; team++ )
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

/**
 * @brief Adds or removes build points from a team.
 */
void G_ModifyBuildPoints( team_t team, float amount )
{
	float *bp, newbp;

	if ( team > TEAM_NONE && team < NUM_TEAMS )
	{
		bp = &level.team[ team ].buildPoints;
	}
	else
	{
		return;
	}

	newbp = *bp + amount;

	if ( newbp < 0.0f )
	{
		*bp = 0.0f;
	}
	else
	{
		*bp = newbp;
	}
}

/**
 * @brief Sets a team's build points to a given value.
 */
void G_SetBuildPoints( team_t team, float amount )
{
	if ( team > TEAM_NONE && team < NUM_TEAMS )
	{
		level.team[ team ].buildPoints = std::max( amount, 0.0f );
	}
}
