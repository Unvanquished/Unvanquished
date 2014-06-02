/*
===========================================================================

Copyright 2013 Unvanquished Developers

This file is part of Daemon.

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "g_local.h"

// -----------
// definitions
// -----------

// This also sets a minimum frequency for G_UpdateUnlockables
#define DECREASE_MOMENTUM_PERIOD  500

// Used for legacy stage sensors
#define MOMENTUM_PER_LEGACY_STAGE 100

typedef enum
{
	CONF_GENERIC,
	CONF_BUILDING,
	CONF_DECONSTRUCTING,
	CONF_DESTROYING,
	CONF_KILLING,

	NUM_CONF
} momentum_t;

// -------------
// local methods
// -------------

const char *MomentumTypeToReason( momentum_t type )
{
	switch ( type )
	{
		case CONF_GENERIC:        return "generic actions";
		case CONF_BUILDING:       return "building a structure";
		case CONF_DECONSTRUCTING: return "deconstructing a structure";
		case CONF_DESTROYING:     return "destryoing a structure";
		case CONF_KILLING:        return "killing a player";
		default:                  return "(unknown momentum type)";
	}
}

/**
 * Has to be called whenever the momentum of a team has been modified.
 */
void MomentumChanged( void )
{
	int       playerNum;
	gentity_t *player;
	gclient_t *client;
	team_t    team;

	// send to clients
	for ( playerNum = 0; playerNum < level.maxclients; playerNum++ )
	{
		player = &g_entities[ playerNum ];
		client = player->client;

		if ( !client )
		{
			continue;
		}

		team = (team_t) client->pers.team;

		if ( team > TEAM_NONE && team < NUM_TEAMS )
		{
			client->ps.persistant[ PERS_MOMENTUM ] = ( short )
				( level.team[ team ].momentum * 10.0f + 0.5f );
		}
	}

	// check team progress
	G_UpdateUnlockables();
}

/**
 * Notifies legacy stage sensors by assuming a certain amount of momentum is a stage.
 *
 * To be called after the team's momentum has been modified.
 */
void NotifyLegacyStageSensors( team_t team, float amount )
{
	int   stage;
	float momentum;

	for ( stage = 1; stage < 3; stage++ )
	{
		momentum = stage * ( float )MOMENTUM_PER_LEGACY_STAGE;

		if ( ( level.team[ team ].momentum - amount < momentum ) ==
		     ( level.team[ team ].momentum          > momentum ) )
		{
			if      ( amount > 0.0f )
			{
				G_notify_sensor_stage( team, stage - 1, stage     );
			}
			else if ( amount < 0.0f )
			{
				G_notify_sensor_stage( team, stage,     stage - 1 );
			}
		}
	}
}

static INLINE float MomentumTimeMod( void )
{
	if ( g_momentumRewardDoubleTime.value <= 0.0f )
	{
		return 1.0f;
	}
	else
	{
		// ln(2) ~= 0.6931472
		return exp( 0.6931472f * ( ( level.matchTime / 60000.0f ) / g_momentumRewardDoubleTime.value ) );
	}
}

/**
 * Modifies a momentum reward based on type, player count and match time.
 */
static float MomentumMod( momentum_t type )
{
	float baseMod, typeMod, timeMod, playerCountMod, mod;

	// type mod
	switch ( type )
	{
		case CONF_KILLING:
			baseMod        = g_momentumBaseMod.value;
			typeMod        = g_momentumKillMod.value;
			timeMod        = MomentumTimeMod();
			playerCountMod = 1.0f;
			break;

		case CONF_BUILDING:
			baseMod        = g_momentumBaseMod.value;
			typeMod        = g_momentumBuildMod.value;
			timeMod        = MomentumTimeMod();
			playerCountMod = 1.0f;
			break;

		case CONF_DECONSTRUCTING:
			// always used on top of build mod, so neutral baseMod/timeMod/playerCountMod
			baseMod        = 1.0f;
			typeMod        = g_momentumDeconMod.value;
			timeMod        = 1.0f;
			playerCountMod = 1.0f;
			break;

		case CONF_DESTROYING:
			baseMod        = g_momentumBaseMod.value;
			typeMod        = g_momentumDestroyMod.value;
			timeMod        = MomentumTimeMod();
			playerCountMod = 1.0f;
			break;

		case CONF_GENERIC:
		default:
			baseMod        = 1.0f;
			typeMod        = 1.0f;
			timeMod        = 1.0f;
			playerCountMod = 1.0f;
	}

	mod = baseMod * typeMod * timeMod * playerCountMod;

	if ( g_debugMomentum.integer > 1 )
	{
		Com_Printf( "Momentum mod for %s: Base %.2f, Type %.2f, Time %.2f, Playercount %.2f → %.2f\n",
		            MomentumTypeToReason( type ), baseMod, typeMod, timeMod, playerCountMod, mod );
	}

	return mod;
}

/**
 * Awards momentum to a team.
 *
 * Will notify the client hwo earned it if given, otherwise the whole team, with an event.
 */
static float AddMomentum( momentum_t type, team_t team, float amount,
                            gentity_t *source, qboolean skipChangeHook )
{
	gentity_t *event = NULL;
	gclient_t *client;
	char      *clientName;

	if ( team <= TEAM_NONE || team >= NUM_TEAMS )
	{
		return 0.0f;
	}

	// apply modifier
	amount *= MomentumMod( type );

	// limit a team's total
	if ( level.team[ team ].momentum + amount > MOMENTUM_MAX )
	{
		amount = MOMENTUM_MAX - level.team[ team ].momentum;
	}

	if ( amount != 0.0f )
	{
		// add momentum to team
		level.team[ team ].momentum += amount;

		// run change hook if requested
		if ( !skipChangeHook )
		{
			MomentumChanged();
		}

		// notify source
		if ( source )
		{
			client = source->client;

			if ( client && client->pers.team == team )
			{
				event = G_NewTempEntity( client->ps.origin, EV_MOMENTUM );
				event->r.svFlags = SVF_SINGLECLIENT;
				event->r.singleClient = client->ps.clientNum;
			}
		}
		else
		{
			event = G_NewTempEntity( vec3_origin, EV_MOMENTUM );
			event->r.svFlags = ( SVF_BROADCAST | SVF_CLIENTMASK );
			G_TeamToClientmask( team, &( event->r.loMask ), &( event->r.hiMask ) );
		}
		if ( event )
		{
			// TODO: Use more bits for momentum value
			event->s.eventParm = 0;
			event->s.otherEntityNum = 0;
			event->s.otherEntityNum2 = ( int )( fabs( amount ) * 10.0f + 0.5f );
			event->s.groundEntityNum = amount < 0.0f ? qtrue : qfalse;
		}

		// notify legacy stage sensors
		NotifyLegacyStageSensors( team, amount );
	}

	if ( g_debugMomentum.integer > 0 )
	{
		if ( source && source->client )
		{
			clientName = source->client->pers.netname;
		}
		else
		{
			clientName = "no source";
		}

		Com_Printf( "Momentum: %.2f to %s (%s by %s for %s)\n",
		            amount, BG_TeamNamePlural( team ), amount < 0.0f ? "lost" : "earned",
		            clientName, MomentumTypeToReason( type ) );
	}

	return amount;
}

// ------------
// GAME methods
// ------------

/**
 * Exponentially decreases momentum.
 */
void G_DecreaseMomentum( void )
{
	int          team;
	float        amount;

	static float decreaseFactor = 1.0f, lastMomentumHalfLife = 0.0f;
	static int   nextCalculation = 0;

	if ( level.time < nextCalculation )
	{
		return;
	}

	if ( g_momentumHalfLife.value <= 0.0f )
	{
		return;
	}

	// only calculate decreaseFactor if the server configuration changed
	if ( lastMomentumHalfLife != g_momentumHalfLife.value )
	{
		// ln(2) ~= 0.6931472
		decreaseFactor = exp( ( -0.6931472f / ( ( 60000.0f / DECREASE_MOMENTUM_PERIOD ) *
		                                        g_momentumHalfLife.value ) ) );

		lastMomentumHalfLife = g_momentumHalfLife.value;
	}

	// decrease momentum
	for ( team = TEAM_NONE + 1; team < NUM_TEAMS; team++ )
	{
		amount = level.team[ team ].momentum * ( decreaseFactor - 1.0f );

		level.team[ team ].momentum += amount;

		// notify legacy stage sensors
		NotifyLegacyStageSensors( (team_t) team, amount );
	}

	MomentumChanged();

	nextCalculation = level.time + DECREASE_MOMENTUM_PERIOD;
}

/**
 * Adds momentum.
 */
float G_AddMomentumGeneric( team_t team, float amount )
{
	AddMomentum( CONF_GENERIC, team, amount, NULL, qfalse );

	return amount;
}

/**
 * Adds momentum.
 *
 * G_AddMomentumEnd has to be called after all G_AddMomentum*Step steps are done.
 */
float G_AddMomentumGenericStep( team_t team, float amount )
{
	AddMomentum( CONF_GENERIC, team, amount, NULL, qtrue );

	return amount;
}

/**
 * Predicts the momentum reward for building a buildable.
 *
 * Is used for the buildlog entry, which is written before the actual reward happens.
 * Also used to calculate the deconstruction penalty for preplaced buildables.
 */
float G_PredictMomentumForBuilding( gentity_t *buildable )
{
	if ( !buildable || buildable->s.eType != ET_BUILDABLE )
	{
		return 0.0f;
	}

	return BG_Buildable( buildable->s.modelindex )->value * MomentumMod( CONF_BUILDING );
}

/**
 * Adds momentum for building a buildable.
 *
 * Will save the reward with the buildable.
 */
float G_AddMomentumForBuilding( gentity_t *buildable )
{
	float     value, reward;
	team_t    team;
	gentity_t *builder;

	if ( !buildable || buildable->s.eType != ET_BUILDABLE )
	{
		return 0.0f;
	}

	value   = BG_Buildable( buildable->s.modelindex )->value;
	team    = BG_Buildable( buildable->s.modelindex )->team;
	builder = &g_entities[ buildable->builtBy->slot ];

	reward = AddMomentum( CONF_BUILDING, team, value, builder, qfalse );

	// Save reward with buildable so it can be reverted
	buildable->momentumEarned = reward;

	return reward;
}

/**
 * Removes momentum for deconstructing a buildable.
 */
float G_RemoveMomentumForDecon( gentity_t *buildable, gentity_t *deconner )
{
	float     healthFraction, value;
	team_t    team;

	// sanity check buildable
	if ( !buildable || buildable->s.eType != ET_BUILDABLE )
	{
		return 0.0f;
	}

	healthFraction = buildable->health / ( float )BG_Buildable( buildable->s.modelindex )->health;
	team           = BG_Buildable( buildable->s.modelindex )->team;

	if ( buildable->momentumEarned )
	{
		value = buildable->momentumEarned;
	}
	else
	{
		// assume the buildable has just been built
		value = G_PredictMomentumForBuilding( buildable );
	}

	value *= healthFraction;

	return AddMomentum( CONF_DECONSTRUCTING, team, -value, deconner, qfalse );
}

/**
 * Adds momentum for destroying a buildable.
 *
 * G_AddMomentumEnd has to be called after all G_AddMomentum*Step steps are done.
 */
float G_AddMomentumForDestroyingStep( gentity_t *buildable, gentity_t *attacker, float share )
{
	float  value;
	team_t team;

	// sanity check buildable
	if ( !buildable || buildable->s.eType != ET_BUILDABLE )
	{
		return 0.0f;
	}

	// sanity check attacker
	if ( !attacker || !attacker->client )
	{
		return 0.0f;
	}

	value = BG_Buildable( buildable->s.modelindex )->value * share;
	team  = (team_t) attacker->client->pers.team;

	return AddMomentum( CONF_DESTROYING, team, value, attacker, qtrue );
}

/**
 * Adds momentum for killing a player.
 *
 * G_AddMomentumEnd has to be called after all G_AddMomentum*Step steps are done.
 */
float G_AddMomentumForKillingStep( gentity_t *victim, gentity_t *attacker, float share )
{
	float     value;
	team_t    team;

	// sanity check victim
	if ( !victim || !victim->client )
	{
		return 0.0f;
	}

	// sanity check attacker
	if ( !attacker || !attacker->client )
	{
		return 0.0f;
	}

	value = BG_GetValueOfPlayer( &victim->client->ps ) * MOMENTUM_PER_CREDIT * share;
	team  = (team_t) attacker->client->pers.team;

	return AddMomentum( CONF_KILLING, team, value, attacker, qtrue );
}

/**
 * Has to be called after the last G_AddMomentum*Step step.
 */
void G_AddMomentumEnd( void )
{
	MomentumChanged();
}
