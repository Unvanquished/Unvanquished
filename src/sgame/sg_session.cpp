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

#include "common/Common.h"
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

	s = va( "%i %i %i %i %s",
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.restartTeam,
		client->sess.seenWelcome,
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
	int        spectatorState;
	int        restartTeam;
	char       ignorelist[ 17 ];

	std::string var = Str::Format( "session%i", client->num() );
	std::string data = Cvar::GetValue( var );

	if ( sscanf( data.c_str(), "%i %i %i %i %16s %*c",
	        &spectatorState,
	        &client->sess.spectatorClient,
	        &restartTeam,
	        &client->sess.seenWelcome,
	        ignorelist
	     ) != 5 )
	{
		Log::Warn( "bad data in cvar %s", var );
		return;
	}

	client->sess.spectatorState = ( spectatorState_t ) spectatorState;
	client->sess.restartTeam = ( team_t ) restartTeam;
	Com_ClientListParse( &client->sess.ignoreList, ignorelist );
}

/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client )
{
	clientSession_t *sess = &client->sess;

	sess->restartTeam = TEAM_NONE;
	sess->spectatorState = SPECTATOR_FREE;
	sess->spectatorClient = -1;

	sess->ignoreList = {};
	sess->seenWelcome = 0;

	G_WriteClientSessionData( client->num() );
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
		if ( level.clients[ i ].pers.connected == CON_CONNECTED && !level.clients[ i ].pers.isBot )
		{
			G_WriteClientSessionData( i );
		}
	}
}
