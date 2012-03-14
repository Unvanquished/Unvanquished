/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "g_local.h"
#include "../../ui/menudef.h"

/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client, qboolean restart )
{
	int        mvc = G_smvGenerateClientList( g_entities + ( client - level.clients ) );
	const char *s;

	// OSP -- stats reset check
	if ( level.fResetStats )
	{
		G_deleteStats( client - level.clients );
	}

	s = va( "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i", client->sess.sessionTeam, client->sess.spectatorTime, client->sess.spectatorState, client->sess.spectatorClient, client->sess.playerType, // DHM - Nerve
	        client->sess.playerWeapon,                                                                                                                                                                                                 // DHM - Nerve
	        client->sess.playerWeapon2, client->sess.latchPlayerType,                                                                                                                                                                  // DHM - Nerve
	        client->sess.latchPlayerWeapon,                                                                                                                                                                                            // DHM - Nerve
	        client->sess.latchPlayerWeapon2,
	        // OSP
	        client->sess.coach_team,
	        client->sess.deaths,
	        client->sess.game_points,
	        client->sess.kills,
	        client->sess.referee,
	        client->sess.spec_invite,
	        client->sess.spec_team, client->sess.suicides, client->sess.team_kills, ( mvc & 0xFFFF ), ( ( mvc >> 16 ) & 0xFFFF )
	        // Damage and rounds played rolled in with weapon stats (below)
	        // OSP
	        ,
//      client->sess.experience,
	        client->sess.muted,
	        client->sess.ignoreClients[ 0 ],
	        client->sess.ignoreClients[ 1 ], client->pers.enterTime, restart ? client->sess.spawnObjectiveIndex : 0 );

	trap_Cvar_Set( va( "session%li", ( long )( client - level.clients ) ), s );

	// Arnout: store the clients stats (7) and medals (7)
	// addition: but only if it isn't a forced map_restart (done by someone on the console)
	if ( !( restart && !level.warmupTime ) )
	{
		s = va( "%.2f %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i %i %i %i %i",
		        client->sess.skillpoints[ 0 ],
		        client->sess.skillpoints[ 1 ],
		        client->sess.skillpoints[ 2 ],
		        client->sess.skillpoints[ 3 ],
		        client->sess.skillpoints[ 4 ],
		        client->sess.skillpoints[ 5 ],
		        client->sess.skillpoints[ 6 ],
		        client->sess.medals[ 0 ],
		        client->sess.medals[ 1 ],
		        client->sess.medals[ 2 ],
		        client->sess.medals[ 3 ], client->sess.medals[ 4 ], client->sess.medals[ 5 ], client->sess.medals[ 6 ] );

		trap_Cvar_Set( va( "sessionstats%li", ( long )( client - level.clients ) ), s );
	}

	// OSP -- save weapon stats too
	if ( !level.fResetStats )
	{
		trap_Cvar_Set( va( "wstats%li", ( long )( client - level.clients ) ), G_createStats( &g_entities[ client - level.clients ] ) );
	}

	// OSP
}

/*
================
G_ClientSwap

Client swap handling
================
*/
void G_ClientSwap( gclient_t *client )
{
	int flags = 0;

	if ( client->sess.sessionTeam == TEAM_AXIS )
	{
		client->sess.sessionTeam = TEAM_ALLIES;
	}
	else if ( client->sess.sessionTeam == TEAM_ALLIES )
	{
		client->sess.sessionTeam = TEAM_AXIS;
	}

	// Swap spec invites as well
	if ( client->sess.spec_invite & TEAM_AXIS )
	{
		flags |= TEAM_ALLIES;
	}

	if ( client->sess.spec_invite & TEAM_ALLIES )
	{
		flags |= TEAM_AXIS;
	}

	client->sess.spec_invite = flags;

	// Swap spec follows as well
	flags                    = 0;

	if ( client->sess.spec_team & TEAM_AXIS )
	{
		flags |= TEAM_ALLIES;
	}

	if ( client->sess.spec_team & TEAM_ALLIES )
	{
		flags |= TEAM_AXIS;
	}

	client->sess.spec_team = flags;
}

void G_CalcRank( gclient_t *client )
{
	int i, highestskill = 0;

	for ( i = 0; i < SK_NUM_SKILLS; i++ )
	{
		G_SetPlayerSkill( client, i );

		if ( client->sess.skill[ i ] > highestskill )
		{
			highestskill = client->sess.skill[ i ];
		}
	}

	// set rank
	client->sess.rank = highestskill;

	if ( client->sess.rank >= 4 )
	{
		int cnt = 0;

		// Gordon: count the number of maxed out skills
		for ( i = 0; i < SK_NUM_SKILLS; i++ )
		{
			if ( client->sess.skill[ i ] >= 4 )
			{
				cnt++;
			}
		}

		client->sess.rank = cnt + 3;

		if ( client->sess.rank > 10 )
		{
			client->sess.rank = 10;
		}
	}
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client )
{
	int      mvc_l, mvc_h;
	char     s[ MAX_STRING_CHARS ];
	qboolean test;

	trap_Cvar_VariableStringBuffer( va( "session%li", ( long )( client - level.clients ) ), s, sizeof( s ) );

	sscanf( s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i", ( int * )&client->sess.sessionTeam, &client->sess.spectatorTime, ( int * )&client->sess.spectatorState, &client->sess.spectatorClient, &client->sess.playerType, // DHM - Nerve
	        &client->sess.playerWeapon,                                                                                                                                                                                                                      // DHM - Nerve
	        &client->sess.playerWeapon2, &client->sess.latchPlayerType,                                                                                                                                                                                      // DHM - Nerve
	        &client->sess.latchPlayerWeapon,                                                                                                                                                                                                                 // DHM - Nerve
	        &client->sess.latchPlayerWeapon2,
	        // OSP
	        &client->sess.coach_team,
	        &client->sess.deaths,
	        &client->sess.game_points,
	        &client->sess.kills,
	        &client->sess.referee,
	        &client->sess.spec_invite, &client->sess.spec_team, &client->sess.suicides, &client->sess.team_kills, &mvc_l, &mvc_h
	        // Damage and round count rolled in with weapon stats (below)
	        // OSP
	        ,
//      &client->sess.experience,
	        ( int * )&client->sess.muted,
	        &client->sess.ignoreClients[ 0 ],
	        &client->sess.ignoreClients[ 1 ], &client->pers.enterTime, &client->sess.spawnObjectiveIndex );

	// OSP -- reinstate MV clients
	client->pers.mvReferenceList = ( mvc_h << 16 ) | mvc_l;
	// OSP

	// OSP -- pull and parse weapon stats
	*s = 0;
	trap_Cvar_VariableStringBuffer( va( "wstats%li", ( long )( client - level.clients ) ), s, sizeof( s ) );

	if ( *s )
	{
		G_parseStats( s );

		if ( g_gamestate.integer == GS_PLAYING )
		{
			client->sess.rounds++;
		}
	}

	// OSP

	// Arnout: likely there are more cases in which we don't want this
	if ( g_gametype.integer != GT_SINGLE_PLAYER &&
	     g_gametype.integer != GT_COOP &&
	     g_gametype.integer != GT_WOLF &&
	     g_gametype.integer != GT_WOLF_STOPWATCH &&
	     !( g_gametype.integer == GT_WOLF_CAMPAIGN && ( g_campaigns[ level.currentCampaign ].current == 0 || level.newCampaign ) ) &&
	     !( g_gametype.integer == GT_WOLF_LMS && g_currentRound.integer == 0 ) )
	{
		trap_Cvar_VariableStringBuffer( va( "sessionstats%li", ( long )( client - level.clients ) ), s, sizeof( s ) );

		// Arnout: read the clients stats (7) and medals (7)
		sscanf( s, "%f %f %f %f %f %f %f %i %i %i %i %i %i %i",
		        &client->sess.skillpoints[ 0 ],
		        &client->sess.skillpoints[ 1 ],
		        &client->sess.skillpoints[ 2 ],
		        &client->sess.skillpoints[ 3 ],
		        &client->sess.skillpoints[ 4 ],
		        &client->sess.skillpoints[ 5 ],
		        &client->sess.skillpoints[ 6 ],
		        &client->sess.medals[ 0 ],
		        &client->sess.medals[ 1 ],
		        &client->sess.medals[ 2 ],
		        &client->sess.medals[ 3 ], &client->sess.medals[ 4 ], &client->sess.medals[ 5 ], &client->sess.medals[ 6 ] );
	}

	G_CalcRank( client );

	test = ( g_altStopwatchMode.integer != 0 || g_currentRound.integer == 1 );

	if ( g_gametype.integer == GT_WOLF_STOPWATCH && g_gamestate.integer != GS_PLAYING && test )
	{
		G_ClientSwap( client );
	}

	if ( g_swapteams.integer )
	{
		trap_Cvar_Set( "g_swapteams", "0" );
		G_ClientSwap( client );
	}

	{
		int j;

		client->sess.startxptotal = 0;

		for ( j = 0; j < SK_NUM_SKILLS; j++ )
		{
			client->sess.startskillpoints[ j ] = client->sess.skillpoints[ j ];
			client->sess.startxptotal         += client->sess.skillpoints[ j ];
		}
	}
}

/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo )
{
	clientSession_t *sess;

//  const char      *value;

	sess                      = &client->sess;

	// initial team determination
	sess->sessionTeam         = TEAM_SPECTATOR;

	sess->spectatorState      = SPECTATOR_FREE;
	sess->spectatorTime       = level.time;

	// DHM - Nerve
	sess->latchPlayerType     = sess->playerType = 0;
	sess->latchPlayerWeapon   = sess->playerWeapon = 0;
	sess->latchPlayerWeapon2  = sess->playerWeapon2 = 0;

	sess->spawnObjectiveIndex = 0;
	// dhm - end

	memset( sess->ignoreClients, 0, sizeof( sess->ignoreClients ) );
//  sess->experience = 0;
	sess->muted = qfalse;
	memset( sess->skill, 0, sizeof( sess->skill ) );
	memset( sess->skillpoints, 0, sizeof( sess->skillpoints ) );
	memset( sess->startskillpoints, 0, sizeof( sess->startskillpoints ) );    // CHRUKER: b017 - startskillpoints didn't get reset
	memset( sess->medals, 0, sizeof( sess->medals ) );
	sess->rank         = 0;
	sess->startxptotal = 0;         // CHRUKER: b017 - startxptotal didn't get reset

	// OSP
	sess->coach_team   = 0;
	sess->referee      = ( client->pers.localClient ) ? RL_REFEREE : RL_NONE;
	sess->spec_invite  = 0;
	sess->spec_team    = 0;
	G_deleteStats( client - level.clients );
	// OSP

	G_WriteClientSessionData( client, qfalse );
}

/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void )
{
	char s[ MAX_STRING_CHARS ];
	int  gt;
	int  i, j;

	trap_Cvar_VariableStringBuffer( "session", s, sizeof( s ) );
	gt = atoi( s );

	// if the gametype changed since the last session, don't use any
	// client sessions
	if ( g_gametype.integer != gt )
	{
		level.newSession  = qtrue;
		level.fResetStats = qtrue;
		G_Printf( "Gametype changed, clearing session data.\n" );
	}
	else
	{
		char     *tmp = s;
		qboolean test = ( g_altStopwatchMode.integer != 0 || g_currentRound.integer == 1 );

#define GETVAL( x ) if ( ( tmp = strchr( tmp, ' ' ) ) == NULL ) { return; \
} x = atoi( ++tmp );

		// Get team lock stuff
		GETVAL( gt );
		teamInfo[ TEAM_AXIS ].spec_lock   = ( gt & TEAM_AXIS ) ? qtrue : qfalse;
		teamInfo[ TEAM_ALLIES ].spec_lock = ( gt & TEAM_ALLIES ) ? qtrue : qfalse;

		// See if we need to clear player stats
		// FIXME: deal with the multi-map missions
		if ( g_gametype.integer != GT_WOLF_CAMPAIGN )
		{
			if ( ( tmp = strchr( va( "%s", tmp ), ' ' ) ) != NULL )
			{
				tmp++;
				trap_GetServerinfo( s, sizeof( s ) );

				if ( Q_stricmp( tmp, Info_ValueForKey( s, "mapname" ) ) )
				{
					level.fResetStats = qtrue;
					G_Printf( "Map changed, clearing player stats.\n" );
				}
			}
		}

		// OSP - have to make sure spec locks follow the right teams
		if ( g_gametype.integer == GT_WOLF_STOPWATCH && g_gamestate.integer != GS_PLAYING && test )
		{
			G_swapTeamLocks();
		}

		if ( g_swapteams.integer )
		{
			G_swapTeamLocks();
		}
	}

	for ( i = 0; i < MAX_FIRETEAMS; i++ )
	{
		char *p, *c;

		trap_Cvar_VariableStringBuffer( va( "fireteam%i", i ), s, sizeof( s ) );

		/*    p = Info_ValueForKey( s, "n" );

		                if(p && *p) {
		                        Q_strncpyz( level.fireTeams[i].name, p, 32 );
		                        level.fireTeams[i].inuse = qtrue;
		                } else {
		                        *level.fireTeams[i].name = '\0';
		                        level.fireTeams[i].inuse = qfalse;
		                }*/

		p = Info_ValueForKey( s, "id" );
		j = atoi( p );

		if ( !*p || j == -1 )
		{
			level.fireTeams[ i ].inuse = qfalse;
		}
		else
		{
			level.fireTeams[ i ].inuse = qtrue;
		}

		level.fireTeams[ i ].ident = j + 1;

		p                          = Info_ValueForKey( s, "p" );
		level.fireTeams[ i ].priv  = !atoi( p ) ? qfalse : qtrue;

		p                          = Info_ValueForKey( s, "i" );

		j                          = 0;

		if ( p && *p )
		{
			c = p;

			for ( c = strchr( c, ' ' ) + 1; c && *c; )
			{
				char str[ 8 ];
				char *l = strchr( c, ' ' );

				if ( !l )
				{
					break;
				}

				Q_strncpyz( str, c, l - c + 1 );
				str[ l - c ]                          = '\0';
				level.fireTeams[ i ].joinOrder[ j++ ] = atoi( str );
				c                                     = l + 1;
			}
		}

		for ( ; j < MAX_CLIENTS; j++ )
		{
			level.fireTeams[ i ].joinOrder[ j ] = -1;
		}

		G_UpdateFireteamConfigString( &level.fireTeams[ i ] );
	}
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( qboolean restart )
{
	int  i;
	char strServerInfo[ MAX_INFO_STRING ];
	int  j;

#ifdef USEXPSTORAGE
	G_StoreXPBackup();
#endif                                                  // USEXPSTORAGE

	trap_GetServerinfo( strServerInfo, sizeof( strServerInfo ) );
	trap_Cvar_Set( "session", va( "%i %i %s", g_gametype.integer,
	                              ( teamInfo[ TEAM_AXIS ].spec_lock * TEAM_AXIS | teamInfo[ TEAM_ALLIES ].spec_lock * TEAM_ALLIES ),
	                              Info_ValueForKey( strServerInfo, "mapname" ) ) );

	// Keep stats for all players in sync
	for ( i = 0; !level.fResetStats && i < level.numConnectedClients; i++ )
	{
		if ( ( g_gamestate.integer == GS_WARMUP_COUNTDOWN &&
		       ( ( g_gametype.integer == GT_WOLF_STOPWATCH && level.clients[ level.sortedClients[ i ] ].sess.rounds >= 2 ) ||
		         ( g_gametype.integer != GT_WOLF_STOPWATCH && level.clients[ level.sortedClients[ i ] ].sess.rounds >= 1 ) ) ) )
		{
			level.fResetStats = qtrue;
		}
	}

	for ( i = 0; i < level.numConnectedClients; i++ )
	{
		if ( level.clients[ level.sortedClients[ i ] ].pers.connected == CON_CONNECTED )
		{
			G_WriteClientSessionData( &level.clients[ level.sortedClients[ i ] ], restart );
			// For slow connecters and a short warmup
		}
		else if ( level.fResetStats )
		{
			G_deleteStats( level.sortedClients[ i ] );
		}
	}

	for ( i = 0; i < MAX_FIRETEAMS; i++ )
	{
		char buffer[ MAX_STRING_CHARS ];

		if ( !level.fireTeams[ i ].inuse )
		{
			Com_sprintf( buffer, MAX_STRING_CHARS, "\\id\\-1" );
		}
		else
		{
			char buffer2[ MAX_STRING_CHARS ];

			*buffer2 = '\0';

			for ( j = 0; j < MAX_CLIENTS; j++ )
			{
				char p[ 8 ];

				Com_sprintf( p, 8, " %i", level.fireTeams[ i ].joinOrder[ j ] );
				Q_strcat( buffer2, MAX_STRING_CHARS, p );
			}

			//      Com_sprintf(buffer, MAX_STRING_CHARS, "\\n\\%s\\i\\%s", level.fireTeams[i].name, buffer2);
			Com_sprintf( buffer, MAX_STRING_CHARS, "\\id\\%i\\i\\%s\\p\\%i", level.fireTeams[ i ].ident - 1, buffer2,
			             level.fireTeams[ i ].priv ? 1 : 0 );
		}

		trap_Cvar_Set( va( "fireteam%i", i ), buffer );
	}
}
