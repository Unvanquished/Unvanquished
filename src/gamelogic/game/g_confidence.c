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

#define DECREASE_CONFIDENCE_PERIOD 1000

/*
===============
ConfidenceChanged

Has to be called whenever the confidence of a team has been modified.
===============
*/
void INLINE ConfidenceChanged( void )
{
	// send to clients
	G_SendConfidenceToClients();

	// check team progress
	G_UpdateUnlockables();
}

/*
===============
G_SendConfidenceToClients

Sends current team confidence to all clients via playerState_t.
===============
*/
void G_SendConfidenceToClients( void )
{
	int       playerNum;
	gentity_t *player;
	gclient_t *client;
	team_t    team;

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
		else
		{
			client->ps.persistant[ PERS_CONFIDENCE ] = 0;
		}
	}
}

/*
============
G_DecreaseConfidence

Decreases both teams confidence according to g_confidenceHalfLife.
g_confidenceHalfLife <= 0 disables decrease.
============
*/
void G_DecreaseConfidence( void )
{
	team_t       team;

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
		level.team[ team ].confidence *= decreaseFactor;
	}

	ConfidenceChanged();

	nextCalculation = level.time + DECREASE_CONFIDENCE_PERIOD;
}

/*
===============
G_AddConfidence

Awards confidence to a team.
Will notify the client hwo earned it if given, otherwise the whole team, with an event.
===============
*/
void G_AddConfidence( team_t team, confidence_reason_t reason, confidence_qualifier_t qualifier,
                      float amount, gentity_t *source )
{
	gentity_t *event = NULL;
	gclient_t *client;

	if ( team <= TEAM_NONE || team >= NUM_TEAMS )
	{
		return;
	}

	level.team[ team ].confidence += amount;

	ConfidenceChanged();

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
		event->s.eventParm = reason;
		event->s.otherEntityNum = qualifier;
		event->s.otherEntityNum2 = ( int )( fabs( amount ) * 10.0f + 0.5f );
		event->s.groundEntityNum = amount < 0.0f ? qtrue : qfalse;
	}

	// HACK: Notify legacy stage sensors, assume 100 confidence is a stage
	// TODO: Make this work with confidence decrease, too
	{
		int   stage;
		float confidence;

		for ( stage = 1; stage < 3; stage++ )
		{
			confidence = stage * 100.0f;

			if ( ( level.team[ team ].confidence          < confidence ) ==
			     ( level.team[ team ].confidence + amount > confidence ) )
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
}
