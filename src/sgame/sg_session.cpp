/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

#include "sg_local.h"

/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistent across level loads
and map restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
static void G_WriteClientSessionData( int clientNum )
{
	const char *s;
	const char *var;
	gclient_t  *client = &level.clients[ clientNum ];
	const char *behavior = G_BotGetBehavior( clientNum );
	if ( behavior == nullptr )
	{
		behavior = "default";
	}

	s = va( "%i %i %i %i %i %s %s",
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.restartTeam,
		client->sess.seenWelcome,
		G_BotGetSkill( clientNum ),
		behavior,
		Com_ClientListString( &client->sess.ignoreList )
	);

	var = va( "session%i", clientNum );

	trap_Cvar_Set( var, s );
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client )
{
	char       s[ MAX_STRING_CHARS ];
	const char *var;
	int        spectatorState;
	int        restartTeam;
	int        botSkill;
	char       botTree[ MAX_QPATH ];
	char       ignorelist[ 17 ];

	var = va( "session%li", ( long )( client - level.clients ) );
	trap_Cvar_VariableStringBuffer( var, s, sizeof( s ) );

	sscanf( s, "%i %i %i %i %i %63s %16s",
	        &spectatorState,
	        &client->sess.spectatorClient,
	        &restartTeam,
	        &client->sess.seenWelcome,
	        &botSkill,
	        botTree,
	        ignorelist
	      );

	client->sess.spectatorState = ( spectatorState_t ) spectatorState;
	client->sess.restartTeam = ( team_t ) restartTeam;
	client->sess.botSkill = botSkill;
	Q_strncpyz( client->sess.botTree, botTree, sizeof( client->sess.botTree ) );
	Com_ClientListParse( &client->sess.ignoreList, ignorelist );
}

/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, const char *userinfo )
{
	clientSession_t *sess = &client->sess;

	sess->restartTeam = TEAM_NONE;
	sess->spectatorState = SPECTATOR_FREE;
	sess->spectatorClient = -1;
	sess->botSkill = 0;
	sess->botTree[ 0 ] = '\0';

	memset( &sess->ignoreList, 0, sizeof( sess->ignoreList ) );
	sess->seenWelcome = 0;

	G_WriteClientSessionData( client - level.clients );
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData()
{
	int i;

	//FIXME: What's this for?
	trap_Cvar_Set( "session", va( "%i", 0 ) );

	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( level.clients[ i ].pers.connected == CON_CONNECTED )
		{
			G_WriteClientSessionData( i );
		}
	}
}
