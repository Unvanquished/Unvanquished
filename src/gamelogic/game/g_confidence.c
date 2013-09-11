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

// This also sets the frequency for G_UpdateUnlockables
#define DECREASE_CONFIDENCE_PERIOD  500

// Used for legacy stage sensors
#define CONFIDENCE_PER_LEGACY_STAGE 100

typedef enum
{
	CONF_GENERIC,
	CONF_BUILDING,
	CONF_DECONSTRUCTING,
	CONF_DESTROYING,
	CONF_KILLING,

	NUM_CONF
} confidence_t;

// -------------
// local methods
// -------------

const char *ConfidenceTypeToReason( confidence_t type )
{
	switch ( type )
	{
		case CONF_GENERIC:        return "generic actions";
		case CONF_BUILDING:       return "building a structure";
		case CONF_DECONSTRUCTING: return "deconstructing a structure";
		case CONF_DESTROYING:     return "destryoing a structure";
		case CONF_KILLING:        return "killing a player";
		default:                  return "(unknown confidence type)";
	}
}

/**
 * Has to be called whenever the confidence of a team has been modified.
 */
void ConfidenceChanged( void )
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

		team = client->pers.team;

		if ( team > TEAM_NONE && team < NUM_TEAMS )
		{
			client->ps.persistant[ PERS_CONFIDENCE ] = ( short )
				( level.team[ team ].confidence * 10.0f + 0.5f );
		}
	}

	// check team progress
	G_UpdateUnlockables();
}

/**
 * Notifies legacy stage sensors by assuming a certain amount of confidence is a stage.
 *
 * To be called after the team's confidence has been modified.
 */
void NotifyLegacyStageSensors( team_t team, float amount )
{
	int   stage;
	float confidence;

	for ( stage = 1; stage < 3; stage++ )
	{
		confidence = stage * ( float )CONFIDENCE_PER_LEGACY_STAGE;

		if ( ( level.team[ team ].confidence - amount < confidence ) ==
		     ( level.team[ team ].confidence          > confidence ) )
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

/**
 * Awards confidence to a team.
 *
 * Will notify the client hwo earned it if given, otherwise the whole team, with an event.
 */
static void AddConfidence( confidence_t type, team_t team, float amount,
                           gentity_t *source, qboolean skipChangeHook )
{
	gentity_t *event = NULL;
	gclient_t *client;
	char      *clientName;

	if ( team <= TEAM_NONE || team >= NUM_TEAMS )
	{
		return;
	}

	level.team[ team ].confidence += amount;

	if ( !skipChangeHook )
	{
		ConfidenceChanged();
	}

	// notify source
	if ( source )
	{
		client = source->client;

		if ( client && client->pers.team == team )
		{
			event = G_NewTempEntity( client->ps.origin, EV_CONFIDENCE );
			event->r.svFlags = SVF_SINGLECLIENT;
			event->r.singleClient = client->ps.clientNum;
		}
	}
	else
	{
		event = G_NewTempEntity( vec3_origin, EV_CONFIDENCE );
		event->r.svFlags = ( SVF_BROADCAST | SVF_CLIENTMASK );
		G_TeamToClientmask( team, &( event->r.loMask ), &( event->r.hiMask ) );
	}

	if ( event )
	{
		// TODO: Use more bits for confidence value
		event->s.eventParm = 0;
		event->s.otherEntityNum = 0;
		event->s.otherEntityNum2 = ( int )( fabs( amount ) * 10.0f + 0.5f );
		event->s.groundEntityNum = amount < 0.0f ? qtrue : qfalse;
	}

	// notify legacy stage sensors
	NotifyLegacyStageSensors( team, amount );

	if ( g_debugConfidence.integer > 0 )
	{
		if ( source && source->client )
		{
			clientName = source->client->pers.netname;
		}
		else
		{
			clientName = "no source";
		}

		Com_Printf( "Confidence: %.2f to %s (%s by %s for %s)\n",
		            amount, BG_TeamNamePlural( team ), amount < 0.0f ? "lost" : "earned",
		            clientName, ConfidenceTypeToReason( type ) );
	}
}

// ------------
// GAME methods
// ------------

/**
 * Exponentially decreases confidence.
 */
void G_DecreaseConfidence( void )
{
	team_t       team;
	float        amount;

	static float decreaseFactor = 1.0f, lastConfidenceHalfLife = 0.0f;
	static int   nextCalculation = 0;

	if ( level.time < nextCalculation )
	{
		return;
	}

	if ( g_confidenceHalfLife.value <= 0.0f )
	{
		return;
	}

	// only calculate decreaseFactor if the server configuration changed
	if ( lastConfidenceHalfLife != g_confidenceHalfLife.value )
	{
		// ln(2) ~= 0.6931472
		decreaseFactor = exp( ( -0.6931472f / ( ( 60000.0f / DECREASE_CONFIDENCE_PERIOD ) *
		                                        g_confidenceHalfLife.value ) ) );

		lastConfidenceHalfLife = g_confidenceHalfLife.value;
	}

	// decrease confidence
	for ( team = TEAM_NONE + 1; team < NUM_TEAMS; team++ )
	{
		amount = level.team[ team ].confidence * ( 1.0f - decreaseFactor );

		level.team[ team ].confidence += amount;

		// notify legacy stage sensors
		NotifyLegacyStageSensors( team, amount );
	}

	ConfidenceChanged();

	nextCalculation = level.time + DECREASE_CONFIDENCE_PERIOD;
}

/**
 * Adds confidence.
 */
float G_AddConfidenceGeneric( team_t team, float amount )
{
	AddConfidence( CONF_GENERIC, team, amount, NULL, qfalse );

	return amount;
}

/**
 * Adds confidence.
 *
 * G_AddConfidenceEnd has to be called after all G_AddConfidenceFor*Step steps are done.
 */
float G_AddConfidenceGenericStep( team_t team, float amount )
{
	AddConfidence( CONF_GENERIC, team, amount, NULL, qtrue );

	return amount;
}

/**
 * Predicts the confidence reward for building a buildable.
 *
 * Is used for the buildlog entry, which is written before the actual reward happens.
 */
float G_PredictConfidenceForBuilding( gentity_t *buildable )
{
	float     value, reward;

	if ( !buildable || !buildable->s.eType == ET_BUILDABLE )
	{
		return 0.0f;
	}

	value   = BG_Buildable( buildable->s.modelindex )->value;
	reward = value * CONFIDENCE_BUILDING_BASEMOD;

	return reward;
}

/**
 * Adds confidence for building a buildable.
 *
 * Will save the reward with the buildable.
 */
float G_AddConfidenceForBuilding( gentity_t *buildable )
{
	float     value, reward;
	team_t    team;
	gentity_t *builder;

	if ( !buildable || !buildable->s.eType == ET_BUILDABLE )
	{
		return 0.0f;
	}

	value   = BG_Buildable( buildable->s.modelindex )->value;
	team    = BG_Buildable( buildable->s.modelindex )->team;
	builder = &g_entities[ buildable->builtBy->slot ];

	// Calculate reward
	reward = value * CONFIDENCE_BUILDING_BASEMOD;

	AddConfidence( CONF_BUILDING, team, reward, builder, qfalse );

	// Save reward with buildable so it can be reverted
	buildable->confidenceEarned = reward;

	return reward;
}

/**
 * Removes confidence for deconstructing a buildable.
 */
float G_RemoveConfidenceForDecon( gentity_t *buildable, gentity_t *deconner )
{
	float     healthFraction, value, penalty;
	team_t    team;

	// sanity check buildable
	if ( !buildable || !buildable->s.eType == ET_BUILDABLE )
	{
		return 0.0f;
	}

	healthFraction = buildable->health / ( float )BG_Buildable( buildable->s.modelindex )->health;
	team           = BG_Buildable( buildable->s.modelindex )->team;

	if ( buildable->confidenceEarned )
	{
		value = buildable->confidenceEarned;
	}
	else
	{
		value = BG_Buildable( buildable->s.modelindex )->value * CONFIDENCE_BUILDING_BASEMOD;
	}

	// Calculate reward
	penalty = -value * healthFraction;

	AddConfidence( CONF_DECONSTRUCTING, team, penalty, deconner, qfalse );

	return penalty;
}

/**
 * Adds confidence for destroying a buildable.
 *
 * G_AddConfidenceEnd has to be called after all G_AddConfidenceFor*Step steps are done.
 */
float G_AddConfidenceForDestroyingStep( gentity_t *buildable, gentity_t *attacker, float share )
{
	float  value, reward;
	team_t team;

	// sanity check buildable
	if ( !buildable || !buildable->s.eType == ET_BUILDABLE )
	{
		return 0.0f;
	}

	// sanity check attacker
	if ( !attacker || !attacker->client )
	{
		return 0.0f;
	}

	value = BG_Buildable( buildable->s.modelindex )->value;
	team  = attacker->client->pers.team;

	// Calculate reward
	reward = value * share;

	AddConfidence( CONF_DESTROYING, team, reward, attacker, qtrue );

	return reward;
}

/**
 * Adds confidence for killing a player.
 *
 * G_AddConfidenceEnd has to be called after all G_AddConfidenceFor*Step steps are done.
 */
float G_AddConfidenceForKillingStep( gentity_t *victim, gentity_t *attacker, float share )
{
	float     value, reward;
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

	value = BG_GetValueOfPlayer( &victim->client->ps );
	team  = attacker->client->pers.team;

	// Calculate reward
	reward = value * CONFIDENCE_PER_CREDIT * share;

	AddConfidence( CONF_KILLING, team, reward, attacker, qtrue );

	return reward;
}

/**
 * Has to be called after the last G_AddConfidenceFor*Step step.
 */
void G_AddConfidenceEnd( void )
{
	ConfidenceChanged();
}
