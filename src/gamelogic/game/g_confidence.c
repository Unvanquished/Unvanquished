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

		team = (team_t) client->pers.team;

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

static INLINE float ConfidenceTimeMod( void )
{
	if ( g_confidenceRewardDoubleTime.value <= 0.0f )
	{
		return 1.0f;
	}
	else
	{
		// ln(2) ~= 0.6931472
		return exp( 0.6931472f * ( ( level.matchTime / 60000.0f ) / g_confidenceRewardDoubleTime.value ) );
	}
}

/**
 * Modifies a confidence reward based on type, player count and match time.
 */
static float ConfidenceMod( confidence_t type )
{
	float baseMod, typeMod, timeMod, playerCountMod, mod;

	// type mod
	switch ( type )
	{
		case CONF_KILLING:
			baseMod        = g_confidenceBaseMod.value;
			typeMod        = g_confidenceKillMod.value;
			timeMod        = ConfidenceTimeMod();
			playerCountMod = 1.0f;
			break;

		case CONF_BUILDING:
			baseMod        = g_confidenceBaseMod.value;
			typeMod        = g_confidenceBuildMod.value;
			timeMod        = ConfidenceTimeMod();
			playerCountMod = 1.0f;
			break;

		case CONF_DECONSTRUCTING:
			// always used on top of build mod, so neutral baseMod/timeMod/playerCountMod
			baseMod        = 1.0f;
			typeMod        = g_confidenceDeconMod.value;
			timeMod        = 1.0f;
			playerCountMod = 1.0f;
			break;

		case CONF_DESTROYING:
			baseMod        = g_confidenceBaseMod.value;
			typeMod        = g_confidenceDestroyMod.value;
			timeMod        = ConfidenceTimeMod();
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

	if ( g_debugConfidence.integer > 1 )
	{
		Com_Printf( "Confidence mod for %s: Base %.2f, Type %.2f, Time %.2f, Playercount %.2f → %.2f\n",
		            ConfidenceTypeToReason( type ), baseMod, typeMod, timeMod, playerCountMod, mod );
	}

	return mod;
}

/**
 * Awards confidence to a team.
 *
 * Will notify the client hwo earned it if given, otherwise the whole team, with an event.
 */
static float AddConfidence( confidence_t type, team_t team, float amount,
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
	amount *= ConfidenceMod( type );

	if ( amount != 0.0f )
	{
		// add confidence to team
		level.team[ team ].confidence += amount;

		// run change hook if requested
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
	}

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

	return amount;
}

// ------------
// GAME methods
// ------------

/**
 * Exponentially decreases confidence.
 */
void G_DecreaseConfidence( void )
{
	int          team;
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
		amount = level.team[ team ].confidence * ( decreaseFactor - 1.0f );

		level.team[ team ].confidence += amount;

		// notify legacy stage sensors
		NotifyLegacyStageSensors( (team_t) team, amount );
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
 * G_AddConfidenceEnd has to be called after all G_AddConfidence*Step steps are done.
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
 * Also used to calculate the deconstruction penalty for preplaced buildables.
 */
float G_PredictConfidenceForBuilding( gentity_t *buildable )
{
	if ( !buildable || !buildable->s.eType == ET_BUILDABLE )
	{
		return 0.0f;
	}

	return BG_Buildable( buildable->s.modelindex )->value * ConfidenceMod( CONF_BUILDING );
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

	reward = AddConfidence( CONF_BUILDING, team, value, builder, qfalse );

	// Save reward with buildable so it can be reverted
	buildable->confidenceEarned = reward;

	return reward;
}

/**
 * Removes confidence for deconstructing a buildable.
 */
float G_RemoveConfidenceForDecon( gentity_t *buildable, gentity_t *deconner )
{
	float     healthFraction, value;
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
		// assume the buildable has just been built
		value = G_PredictConfidenceForBuilding( buildable );
	}

	value *= healthFraction;

	return AddConfidence( CONF_DECONSTRUCTING, team, -value, deconner, qfalse );
}

/**
 * Adds confidence for destroying a buildable.
 *
 * G_AddConfidenceEnd has to be called after all G_AddConfidence*Step steps are done.
 */
float G_AddConfidenceForDestroyingStep( gentity_t *buildable, gentity_t *attacker, float share )
{
	float  value;
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

	value = BG_Buildable( buildable->s.modelindex )->value * share;
	team  = (team_t) attacker->client->pers.team;

	return AddConfidence( CONF_DESTROYING, team, value, attacker, qtrue );
}

/**
 * Adds confidence for killing a player.
 *
 * G_AddConfidenceEnd has to be called after all G_AddConfidence*Step steps are done.
 */
float G_AddConfidenceForKillingStep( gentity_t *victim, gentity_t *attacker, float share )
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

	value = BG_GetValueOfPlayer( &victim->client->ps ) * CONFIDENCE_PER_CREDIT * share;
	team  = (team_t) attacker->client->pers.team;

	return AddConfidence( CONF_KILLING, team, value, attacker, qtrue );
}

/**
 * Has to be called after the last G_AddConfidence*Step step.
 */
void G_AddConfidenceEnd( void )
{
	ConfidenceChanged();
}
