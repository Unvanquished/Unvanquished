/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

#include "cg_local.h"

static void AddToServerList( char *name, char *label, int clients, int bots, int ping, int maxClients, char *addr )
{
	server_t *node;

	if ( rocketInfo.data.serverCount == MAX_SERVERS )
	{
		return;
	}

	node = &rocketInfo.data.servers[ rocketInfo.data.serverCount ];

	node->name = BG_strdup( name );
	node->clients = clients;
	node->bots = bots;
	node->ping = ping;
	node->maxClients = maxClients;
	node->addr = BG_strdup( addr );
	node->label = BG_strdup( label );

	rocketInfo.data.serverCount++;
}

static void CG_Rocket_SetServerListServer( int index )
{
	rocketInfo.data.serverIndex = index;
}
#define MAX_SERVERSTATUS_LINES 4096
void CG_Rocket_BuildServerInfo( void )
{
	int serverIndex = rocketInfo.data.serverIndex;
	static char serverInfoText[ MAX_SERVERSTATUS_LINES ];
	static rocketState_t state;
	char buf[ MAX_INFO_STRING ];
	const char *p;
	server_t *server;


	if ( serverIndex >= rocketInfo.data.serverCount || serverIndex < 0 )
	{
		return;
	}

	server = &rocketInfo.data.servers[ serverIndex ];

	if ( rocketInfo.rocketState != BUILDING_SERVER_INFO )
	{
		state = rocketInfo.rocketState;
		rocketInfo.rocketState = BUILDING_SERVER_INFO;
	}

	if ( trap_LAN_ServerStatus( server->addr, serverInfoText, sizeof( serverInfoText ) ) )
	{
		int line = 0;
		int i = 0, score, ping;
		static char key[BIG_INFO_VALUE], value[ BIG_INFO_VALUE ];
		char name[ MAX_STRING_CHARS ];

		trap_Rocket_DSClearTable( "server_browser", "serverInfo" );

		p = serverInfoText;

		while ( *p )
		{
			Info_NextPair( &p, key, value );
			if ( key[ 0 ] )
			{
				Info_SetValueForKey( buf, "name", key, qfalse );
				Info_SetValueForKey( buf, "players", value, qfalse );
				trap_Rocket_DSAddRow( "server_browser", "serverInfo", buf );
				*buf = '\0';
			}
			else
			{
				break;
			}
		}

		// header
		Info_SetValueForKey( buf, "label", "num", qfalse );
		Info_SetValueForKey( buf, "name", "name", qfalse );
		Info_SetValueForKey( buf, "players", "score", qfalse );
		Info_SetValueForKey( buf, "ping", "ping", qfalse );
		trap_Rocket_DSAddRow( "server_browser", "serverInfo", buf );

		// Parse first set of players
		sscanf( value, "%d %d %s", &score, &ping, name );
		Info_SetValueForKey( buf, "label", va( "%d", i++ ), qfalse );
		Info_SetValueForKeyRocket( buf, "name", name );
		Info_SetValueForKey( buf, "players", va( "%d", score ), qfalse );
		Info_SetValueForKey( buf, "ping", va( "%d", ping ), qfalse );
		trap_Rocket_DSAddRow( "server_browser", "serverInfo", buf );

		while ( *p )
		{
			Info_NextPair( &p, key, value );
			if ( key[ 0 ] )
			{
				sscanf( key, "%d %d %s", &score, &ping, name );
				Info_SetValueForKey( buf, "label", va( "%d", i++ ), qfalse );
				Info_SetValueForKeyRocket( buf, "name", name );
				Info_SetValueForKey( buf, "players", va( "%d", score ), qfalse );
				Info_SetValueForKey( buf, "ping", va( "%d", ping ), qfalse );
				trap_Rocket_DSAddRow( "server_browser", "serverInfo", buf );
			}

			if ( value[ 0 ] )
			{
				sscanf( value, "%d %d %s", &score, &ping, name );
				Info_SetValueForKey( buf, "label", va( "%d", i++ ), qfalse );
				Info_SetValueForKeyRocket( buf, "name", name );
				Info_SetValueForKey( buf, "players", va( "%d", score ), qfalse );
				Info_SetValueForKey( buf, "ping", va( "%d", ping ), qfalse );
				trap_Rocket_DSAddRow( "server_browser", "serverInfo", buf );
			}
		}

		rocketInfo.rocketState = state;
	}

}

static void CG_Rocket_BuildServerList( const char *args )
{
	char data[ MAX_INFO_STRING ] = { 0 };
	int i;

	// Only refresh once every second
	if ( trap_Milliseconds() < 1000 + rocketInfo.serversLastRefresh )
	{
		return;
	}

	Q_strncpyz( rocketInfo.currentNetSource, args, sizeof( rocketInfo.currentNetSource ) );
	rocketInfo.rocketState = RETRIEVING_SERVERS;

	if ( !Q_stricmp( args, "internet" ) )
	{
		int numServers;

		trap_Rocket_DSClearTable( "server_browser", args );

		trap_LAN_MarkServerVisible( CG_StringToNetSource( args ), -1, qtrue );

		numServers = trap_LAN_GetServerCount( CG_StringToNetSource( args ) );

		for ( i = 0; i < numServers; ++i )
		{
			char info[ MAX_STRING_CHARS ];
			int ping, bots, clients, maxClients;

			Com_Memset( &data, 0, sizeof( data ) );

			if ( !trap_LAN_ServerIsVisible( CG_StringToNetSource( args ), i ) )
			{
				continue;
			}

			ping = trap_LAN_GetServerPing( CG_StringToNetSource( args ), i );

			if ( ping >= 0 || !Q_stricmp( args, "favorites" ) )
			{
				char addr[ 25 ];
				trap_LAN_GetServerInfo( CG_StringToNetSource( args ), i, info, MAX_INFO_STRING );

				bots = atoi( Info_ValueForKey( info, "bots" ) );
				clients = atoi( Info_ValueForKey( info, "clients" ) );
				maxClients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
				Q_strncpyz( addr, Info_ValueForKey( info, "addr" ), sizeof( addr ) );
				AddToServerList( Info_ValueForKey( info, "hostname" ), Info_ValueForKey( info, "label" ), clients, bots, ping, maxClients, addr );
			}
		}

		for ( i = 0; i < rocketInfo.data.serverCount; ++i )
		{
			if ( rocketInfo.data.servers[ i ].ping <= 0 )
			{
				continue;
			}

			Info_SetValueForKey( data, "name", rocketInfo.data.servers[ i ].name, qfalse );
			Info_SetValueForKey( data, "players", va( "%d", rocketInfo.data.servers[ i ].clients ), qfalse );
			Info_SetValueForKey( data, "bots", va( "%d", rocketInfo.data.servers[ i ].bots ), qfalse );
			Info_SetValueForKey( data, "ping", va( "%d", rocketInfo.data.servers[ i ].ping ), qfalse );
			Info_SetValueForKey( data, "maxClients", va( "%d", rocketInfo.data.servers[ i ].maxClients ), qfalse );
			Info_SetValueForKey( data, "addr", rocketInfo.data.servers[ i ].addr, qfalse );
			Info_SetValueForKey( data, "label", rocketInfo.data.servers[ i ].label, qfalse );

			trap_Rocket_DSAddRow( "server_browser", args, data );
		}
	}
	else if ( !Q_stricmp( args, "serverInfo" ) )
	{
		CG_Rocket_BuildServerInfo();
	}

	rocketInfo.serversLastRefresh = trap_Milliseconds();
}

static int ServerListCmpByPing( const void *one, const void *two )
{
	server_t *a = ( server_t * ) one;
	server_t *b = ( server_t * ) two;

	if ( a->ping > b->ping ) return 1;
	if ( b->ping > a->ping ) return -1;
	if ( a->ping == b->ping )  return 0;
	return 0; // silence compiler
}

static void CG_Rocket_SortServerList( const char *name, const char *sortBy )
{
	char data[ MAX_INFO_STRING ] = { 0 };
	int  i;

	if ( !Q_stricmp( sortBy, "ping" ) )
	{
		qsort( rocketInfo.data.servers, rocketInfo.data.serverCount, sizeof( server_t ), &ServerListCmpByPing );
	}

	trap_Rocket_DSClearTable( "server_browser", name );

	for ( i = 0; i < rocketInfo.data.serverCount; ++i )
	{
		if ( rocketInfo.data.servers[ i ].ping <= 0 )
		{
			continue;
		}

		Info_SetValueForKey( data, "name", rocketInfo.data.servers[ i ].name, qfalse );
		Info_SetValueForKey( data, "players", va( "%d", rocketInfo.data.servers[ i ].clients ), qfalse );
		Info_SetValueForKey( data, "bots", va( "%d", rocketInfo.data.servers[ i ].bots ), qfalse );
		Info_SetValueForKey( data, "ping", va( "%d", rocketInfo.data.servers[ i ].ping ), qfalse );
		Info_SetValueForKey( data, "maxClients", va( "%d", rocketInfo.data.servers[ i ].maxClients ), qfalse );
		Info_SetValueForKey( data, "addr", rocketInfo.data.servers[ i ].addr, qfalse );
		Info_SetValueForKey( data, "label", rocketInfo.data.servers[ i ].label, qfalse );

		trap_Rocket_DSAddRow( "server_browser", name, data );
	}
}

void CG_Rocket_CleanUpServerList( void )
{
	int i;

	for ( i = 0; i < rocketInfo.data.serverCount; ++i )
	{
		BG_Free( rocketInfo.data.servers[ i ].name );
		BG_Free( rocketInfo.data.servers[ i ].label );
		BG_Free( rocketInfo.data.servers[ i ].addr );
	}

	rocketInfo.data.serverCount = 0;
}

void CG_Rocket_ExecServerList( const char *table )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s", rocketInfo.data.servers[ rocketInfo.data.serverIndex ].addr ) );
}

static qboolean Parse( char **p, char **out )
{
	char *token;

	token = COM_ParseExt( p, qfalse );

	if ( token && token[ 0 ] != 0 )
	{
		* ( out ) = BG_strdup( token );
		return qtrue;
	}

	return qfalse;
}

static void AddToResolutionList( int w, int h )
{
	resolution_t *node;

	if ( rocketInfo.data.resolutionCount == MAX_RESOLUTIONS )
	{
		return;
	}

	node = &rocketInfo.data.resolutions[ rocketInfo.data.resolutionCount ];

	node->width = w;
	node->height = h;
	rocketInfo.data.resolutionCount++;
}

void CG_Rocket_SetResolutionListResolution( int index )
{
	rocketInfo.data.resolutionIndex = index;
}

void CG_Rocket_BuildResolutionList( const char *args )
{
	char        buf[ MAX_STRING_CHARS ];
	int         w, h;
	char        *p;
	char        *out;
	char        *s = NULL;
	resolution_t *resolution;
	int          i;

	trap_Cvar_VariableStringBuffer( "r_availableModes", buf, sizeof( buf ) );
	p = buf;
	rocketInfo.data.resolutionCount = 0;

	while ( Parse( &p, &out ) )
	{

		sscanf( out, "%dx%d", &w, &h );
		AddToResolutionList( w, h );
		BG_Free( out );
	}

	buf[ 0 ] = '\0';

	trap_Rocket_DSClearTable( "resolutions", "default" );

	for ( i = 0; i < rocketInfo.data.resolutionCount; ++i )
	{
		Info_SetValueForKey( buf, "width", va( "%d", rocketInfo.data.resolutions[ i ].width ), qfalse );
		Info_SetValueForKey( buf, "height", va( "%d", rocketInfo.data.resolutions[ i ].height ), qfalse );

		trap_Rocket_DSAddRow( "resolutions", "default", buf );
	}

}

static int ResolutionListCmpByWidth( const void *one, const void *two )
{
	resolution_t *a = ( resolution_t * ) one;
	resolution_t *b = ( resolution_t * ) two;

	if ( a->width > b->width ) return -1;
	if ( b->width > a->width ) return 1;
	if ( a->width == b->width )  return 0;
	return 0; // silence compiler
}

void CG_Rocket_SortResolutionList( const char *name, const char *sortBy )
{
	static char buf[ MAX_STRING_CHARS ];
	int i;

	if ( !Q_stricmp( sortBy, "width" ) )
	{
		qsort( rocketInfo.data.resolutions, rocketInfo.data.resolutionCount, sizeof( resolution_t ), &ResolutionListCmpByWidth );
	}

	trap_Rocket_DSClearTable( "resolutions", "default" );

	for ( i = 0; i < rocketInfo.data.resolutionCount; ++i )
	{
		Info_SetValueForKey( buf, "width", va( "%d", rocketInfo.data.resolutions[ i ].width ), qfalse );
		Info_SetValueForKey( buf, "height", va( "%d", rocketInfo.data.resolutions[ i ].height ), qfalse );

		trap_Rocket_DSAddRow( "resolutions", "default", buf );
	}
}


void CG_Rocket_CleanUpResolutionList( void )
{
	rocketInfo.data.resolutionCount = 0;
}

static void AddToLanguageList( char *name, char *lang )
{
	language_t *node;

	if ( rocketInfo.data.languageCount == MAX_LANGUAGES )
	{
		return;
	}

	node = &rocketInfo.data.languages[ rocketInfo.data.languageCount ];

	node->name = name;
	node->lang = lang;
	rocketInfo.data.languageCount++;
}

void CG_Rocket_SetLanguageListLanguage( int index )
{
	rocketInfo.data.languageIndex = index;
}

// FIXME: use COM_Parse or something instead of this way
void CG_Rocket_BuildLanguageList( const char *args )
{
	char        buf[ MAX_STRING_CHARS ], temp[ MAX_TOKEN_CHARS ];
	int         index = 0, lang = 0;
	qboolean    quoted = qfalse;
	char        *p;

	trap_Cvar_VariableStringBuffer( "trans_languages", buf, sizeof( buf ) );
	p = buf;
	memset( &temp, 0, sizeof( temp ) );
	while( p && *p )
	{
		if( *p == '"' && quoted )
		{
			AddToLanguageList( BG_strdup( temp ), NULL );
			quoted = qfalse;
			index = 0;
		}

		else if( *p == '"' || quoted )
		{
			if( !quoted ) { p++; }
			quoted = qtrue;
			temp[ index++ ] = *p;
		}
		p++;
	}
	trap_Cvar_VariableStringBuffer( "trans_encodings", buf, sizeof( buf ) );
	p = buf;
	memset( &temp, 0, sizeof( temp ) );
	while( p && *p )
	{
		if( *p == '"' && quoted )
		{
			rocketInfo.data.languages[ lang++ ].lang = BG_strdup( temp );
			quoted = qfalse;
			index = 0;
		}

		else if( *p == '"' || quoted )
		{
			if( !quoted ) { p++; }
			quoted = qtrue;
			temp[ index++ ] = *p;
		}
		p++;
	}

	buf[ 0 ] = '\0';

	for ( index = 0; index < rocketInfo.data.languageCount; ++index )
	{
		Info_SetValueForKey( buf, "name", rocketInfo.data.languages[ index ].name, qfalse );
		Info_SetValueForKey( buf, "lang", rocketInfo.data.languages[ index ].lang, qfalse );

		trap_Rocket_DSAddRow( "language", "default", buf );
	}
}

void CG_Rocket_CleanUpLanguageList( void )
{
	int i;

	for ( i = 0; i < rocketInfo.data.languageCount; ++i )
	{
		BG_Free( rocketInfo.data.languages[ i ].lang );
		BG_Free( rocketInfo.data.languages[ i ].name );
	}

	rocketInfo.data.languageCount = 0;
}

static void AddToVoipInputs( char *name )
{
	if ( rocketInfo.data.voipInputsCount == MAX_INPUTS )
	{
		return;
	}

	rocketInfo.data.voipInputs[ rocketInfo.data.voipInputsCount++ ] = name;
}

void CG_Rocket_SetVoipInputsInput( int index )
{
	rocketInfo.data.voipInputIndex = index;
}

void CG_Rocket_BuildVoIPInputs( const char *args )
{
	char buf[ MAX_STRING_CHARS ];
	char *p, *head;
	int inputs = 0;

	trap_Cvar_VariableStringBuffer( "s_alAvailableInputDevices", buf, sizeof( buf ) );
	head = buf;
	while ( ( p = strchr( head, '\n' ) ) )
	{
		*p = '\0';
		AddToVoipInputs( BG_strdup( head ) );
		head = p + 1;
	}

	buf[ 0 ] = '\0';
	for ( inputs = 0; inputs < rocketInfo.data.voipInputsCount; ++inputs )
	{
		Info_SetValueForKey( buf, "name", rocketInfo.data.voipInputs[ inputs ], qfalse );

		trap_Rocket_DSAddRow( "voipInputs", "default", buf );
	}
}

void CG_Rocket_CleanUpVoIPInputs( void )
{
	int i;

	for ( i = 0; i < rocketInfo.data.voipInputsCount; ++i )
	{
		BG_Free( rocketInfo.data.voipInputs[ i ] );
	}

	rocketInfo.data.voipInputsCount = 0;
}

static void AddToAlOutputs( char *name )
{
	if ( rocketInfo.data.alOutputsCount == MAX_OUTPUTS )
	{
		return;
	}

	rocketInfo.data.alOutputs[ rocketInfo.data.alOutputsCount++ ] = name;
}

void CG_Rocket_SetAlOutputsOutput( int index )
{
	rocketInfo.data.alOutputIndex = index;
}

void CG_Rocket_BuildAlOutputs( const char *args )
{
	char buf[ MAX_STRING_CHARS ];
	char *p, *head;
	int outputs = 0;

	trap_Cvar_VariableStringBuffer( "s_alAvailableDevices", buf, sizeof( buf ) );
	head = buf;
	while ( ( p = strchr( head, '\n' ) ) )
	{
		*p = '\0';
		AddToAlOutputs( BG_strdup( head ) );
		head = p + 1;
	}

	buf[ 0 ] = '\0';

	for ( outputs = 0; outputs < rocketInfo.data.alOutputsCount; ++outputs )
	{
		Info_SetValueForKey( buf, "name", rocketInfo.data.alOutputs[ outputs ], qfalse );

		trap_Rocket_DSAddRow( "alOutputs", "default", buf );
	}
}

void CG_Rocket_CleanUpAlOutputs( void )
{
	int i;

	for ( i = 0; i < rocketInfo.data.alOutputsCount; ++i )
	{
		BG_Free( rocketInfo.data.alOutputs[ i ] );
	}

	rocketInfo.data.alOutputsCount = 0;
}

void CG_Rocket_SetModListMod( int index )
{
	rocketInfo.data.modIndex = index;
}

void CG_Rocket_BuildModList( const char *args )
{
	int   numdirs;
	char  dirlist[ 2048 ];
	char  *dirptr;
	char  *descptr;
	int   i;
	int   dirlen;

	rocketInfo.data.modCount = 0;
	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof( dirlist ) );
	dirptr = dirlist;

	for ( i = 0; i < numdirs; i++ )
	{
		dirlen = strlen( dirptr ) + 1;
		descptr = dirptr + dirlen;
		rocketInfo.data.modList[ rocketInfo.data.modCount ].name = BG_strdup( dirptr );
		rocketInfo.data.modList[ rocketInfo.data.modCount ].description = BG_strdup( descptr );
		dirptr += dirlen + strlen( descptr ) + 1;
		rocketInfo.data.modCount++;

		if ( rocketInfo.data.modCount >= MAX_MODS )
		{
			break;
		}
	}

	dirlist[ 0 ] = '\0';

	for ( i = 0; i < rocketInfo.data.modCount; ++i )
	{
		Info_SetValueForKey( dirlist, "name", rocketInfo.data.modList[ i ].name, qfalse );
		Info_SetValueForKey( dirlist, "description", rocketInfo.data.modList[ i ].description, qfalse );

		trap_Rocket_DSAddRow( "modList", "default", dirlist );
	}
}

void CG_Rocket_CleanUpModList( void )
{
	int i;

	for ( i = 0; i < rocketInfo.data.modCount; ++i )
	{
		BG_Free( rocketInfo.data.modList[ i ].name );
		BG_Free( rocketInfo.data.modList[ i ].description );
	}

	rocketInfo.data.modCount = 0;
}

void CG_Rocket_SetDemoListDemo( int index )
{
	rocketInfo.data.demoIndex = index;
}

void CG_Rocket_ExecDemoList( const char *args )
{
	trap_SendConsoleCommand( va( "demo %s", rocketInfo.data.demoList[ rocketInfo.data.demoIndex ] ) );
}

void CG_Rocket_BuildDemoList( const char *args )
{
	char  demolist[ 4096 ];
	char demoExt[ 32 ];
	char  *demoname;
	int   i, len;

	Com_sprintf( demoExt, sizeof( demoExt ), "dm_%d", ( int ) trap_Cvar_VariableIntegerValue( "protocol" ) );

	rocketInfo.data.demoCount = trap_FS_GetFileList( "demos", demoExt, demolist, 4096 );

	Com_sprintf( demoExt, sizeof( demoExt ), ".dm_%d", ( int ) trap_Cvar_VariableIntegerValue( "protocol" ) );

	if ( rocketInfo.data.demoCount )
	{
		if ( rocketInfo.data.demoCount > MAX_DEMOS )
		{
			rocketInfo.data.demoCount = MAX_DEMOS;
		}

		demoname = demolist;

		for ( i = 0; i < rocketInfo.data.demoCount; i++ )
		{
			len = strlen( demoname );

			if ( !Q_stricmp( demoname +  len - strlen( demoExt ), demoExt ) )
			{
				demoname[ len - strlen( demoExt ) ] = '\0';
			}

			rocketInfo.data.demoList[ i ] = BG_strdup( demoname );
			demoname += len + 1;
		}
	}

	demolist[ 0 ] = '\0';

	for ( i = 0; i < rocketInfo.data.demoCount; ++i )
	{
		Info_SetValueForKey( demolist, "name", rocketInfo.data.demoList[ i ], qfalse );

		trap_Rocket_DSAddRow( "demoList", "default", demolist );
	}
}

void CG_Rocket_CleanUpDemoList( void )
{
	int i;

	for ( i = 0; i < rocketInfo.data.demoCount; ++i )
	{
		BG_Free( rocketInfo.data.demoList[ i ] );
	}

	rocketInfo.data.demoCount = 0;
}

void CG_Rocket_BuildTeamList( const char *args )
{
	char buf[ MAX_INFO_STRING ];
	clientInfo_t *ci;
	score_t *score;
	int i;

// 	// Do not build list if not currently playing
// 	if ( rocketInfo.rocketState < PLAYING )
// 	{
// 		return;
// 	}

	CG_RequestScores();

	// Clear old values. Always build all three teams.
	trap_Rocket_DSClearTable( "teams", "spectators" );
	trap_Rocket_DSClearTable( "teams", "aliens" );
	trap_Rocket_DSClearTable( "teams", "humans" );

	for ( i = 0; i < MAX_CLIENTS; ++i )
	{
		ci = &cgs.clientinfo[ i ];
		score = &cg.scores[ i ];
		if ( !ci->infoValid )
		{
			continue;
		}

		Info_SetValueForKey( buf, "num", va( "%d", score->client ), qfalse );
		Info_SetValueForKey( buf, "score", va( "%d", score->score ), qfalse );
		Info_SetValueForKey( buf, "ping", va( "%d", score->ping ), qfalse );
		Info_SetValueForKey( buf, "weapon", va( "%d", score->weapon ), qfalse );
		Info_SetValueForKey( buf, "upgrade", va( "%d", score->upgrade ), qfalse );
		Info_SetValueForKey( buf, "time", va( "%d", score->time ), qfalse );
		Info_SetValueForKey( buf, "credits", va( "%d", ci->credit ), qfalse );
		Info_SetValueForKey( buf, "location", CG_ConfigString( CS_LOCATIONS + ci->location ), qfalse );

		switch ( score->team )
		{
			case TEAM_ALIENS:
				rocketInfo.data.playerList[ score->team ][ rocketInfo.data.playerCount[ TEAM_ALIENS ]++ ] = i;
				trap_Rocket_DSAddRow( "teams", "aliens", buf );
				break;

			case TEAM_HUMANS:
				rocketInfo.data.playerList[ score->team ][ rocketInfo.data.playerIndex[ TEAM_HUMANS ]++ ] = i;
				trap_Rocket_DSAddRow( "teams", "humans", buf );
				break;

			case TEAM_NONE:
				rocketInfo.data.playerList[ score->team ][ rocketInfo.data.playerCount[ TEAM_NONE ]++ ] = i;
				trap_Rocket_DSAddRow( "teams", "spectators", buf );
				break;
		}
	}

}

static int PlayerListCmpByScore( const void *one, const void *two )
{
	int *a = ( int * ) one;
	int *b = ( int * ) two;

	if ( cg.scores[ *a ].score > cg.scores[ *b ].score ) return 1;
	if ( cg.scores[ *b ].score > cg.scores[ *a ].score ) return -1;
	if ( cg.scores[ *a ].score == cg.scores[ *b ].score )  return 0;
	return 0; // silence compiler
}

void CG_Rocket_SortTeamList( const char *name, const char *sortBy )
{
	int i;
	clientInfo_t *ci;
	score_t *score;
	char buf[ MAX_INFO_STRING ];

// 	// Do not sort list if not currently playing
// 	if ( rocketInfo.rocketState < PLAYING )
// 	{
// 		return;
// 	}



	if ( !Q_stricmp( "score", sortBy ) )
	{
		qsort( rocketInfo.data.playerList[ TEAM_NONE ], rocketInfo.data.playerCount[ TEAM_NONE ], sizeof( int ), &PlayerListCmpByScore );
		qsort( rocketInfo.data.playerList[ TEAM_ALIENS ], rocketInfo.data.playerCount[ TEAM_ALIENS ], sizeof( int ), &PlayerListCmpByScore );
		qsort( rocketInfo.data.playerList[ TEAM_HUMANS ], rocketInfo.data.playerIndex[ TEAM_HUMANS ], sizeof( int ), &PlayerListCmpByScore );
	}

	// Clear old values. Always build all three teams.
	trap_Rocket_DSClearTable( "teams", "spectators" );
	trap_Rocket_DSClearTable( "teams", "aliens" );
	trap_Rocket_DSClearTable( "teams", "humans" );

	for ( i = 0; i < rocketInfo.data.playerCount[ TEAM_NONE ]; ++i )
	{
		ci = &cgs.clientinfo[ rocketInfo.data.playerList[ TEAM_NONE ][ i ] ];
		score = &cg.scores[ rocketInfo.data.playerList[ TEAM_NONE ][ i ] ];
		if ( !ci->infoValid )
		{
			continue;
		}

		Info_SetValueForKey( buf, "num", va( "%d", score->client ), qfalse );
		Info_SetValueForKey( buf, "score", va( "%d", score->score ), qfalse );
		Info_SetValueForKey( buf, "ping", va( "%d", score->ping ), qfalse );
		Info_SetValueForKey( buf, "weapon", va( "%d", score->weapon ), qfalse );
		Info_SetValueForKey( buf, "upgrade", va( "%d", score->upgrade ), qfalse );
		Info_SetValueForKey( buf, "time", va( "%d", score->time ), qfalse );
		Info_SetValueForKey( buf, "credits", va( "%d", ci->credit ), qfalse );
		Info_SetValueForKey( buf, "location", CG_ConfigString( CS_LOCATIONS + ci->location ), qfalse );

		trap_Rocket_DSAddRow( "teams", "spectators", buf );
	}

	for ( i = 0; i < rocketInfo.data.playerIndex[ TEAM_HUMANS ]; ++i )
	{
		ci = &cgs.clientinfo[ rocketInfo.data.playerList[ TEAM_HUMANS ][ i ] ];
		score = &cg.scores[ rocketInfo.data.playerList[ TEAM_NONE ][ i ] ];
		if ( !ci->infoValid )
		{
			continue;
		}

		Info_SetValueForKey( buf, "num", va( "%d", score->client ), qfalse );
		Info_SetValueForKey( buf, "score", va( "%d", score->score ), qfalse );
		Info_SetValueForKey( buf, "ping", va( "%d", score->ping ), qfalse );
		Info_SetValueForKey( buf, "weapon", va( "%d", score->weapon ), qfalse );
		Info_SetValueForKey( buf, "upgrade", va( "%d", score->upgrade ), qfalse );
		Info_SetValueForKey( buf, "time", va( "%d", score->time ), qfalse );
		Info_SetValueForKey( buf, "credits", va( "%d", ci->credit ), qfalse );
		Info_SetValueForKey( buf, "location", CG_ConfigString( CS_LOCATIONS + ci->location ), qfalse );
		trap_Rocket_DSAddRow( "team", "spectators", buf );
	}

	for ( i = 0; i < rocketInfo.data.playerCount[ TEAM_NONE ]; ++i )
	{
		ci = &cgs.clientinfo[ rocketInfo.data.playerList[ TEAM_NONE ][ i ] ];
		score = &cg.scores[ rocketInfo.data.playerList[ TEAM_NONE ][ i ] ];
		if ( !ci->infoValid )
		{
			continue;
		}

		Info_SetValueForKey( buf, "num", va( "%d", score->client ), qfalse );
		Info_SetValueForKey( buf, "score", va( "%d", score->score ), qfalse );
		Info_SetValueForKey( buf, "ping", va( "%d", score->ping ), qfalse );
		Info_SetValueForKey( buf, "weapon", va( "%d", score->weapon ), qfalse );
		Info_SetValueForKey( buf, "upgrade", va( "%d", score->upgrade ), qfalse );
		Info_SetValueForKey( buf, "time", va( "%d", score->time ), qfalse );
		Info_SetValueForKey( buf, "credits", va( "%d", ci->credit ), qfalse );
		Info_SetValueForKey( buf, "location", CG_ConfigString( CS_LOCATIONS + ci->location ), qfalse );

		trap_Rocket_DSAddRow( "teams", "spectators", buf );
	}
}

void CG_Rocket_BuildMapList( const char *args )
{
	int i;

	trap_Rocket_DSClearTable( "mapList", "default" );
	CG_LoadArenas();

	for ( i = 0; i < rocketInfo.data.mapCount; ++i )
	{
		char buf[ MAX_INFO_STRING ];

		Info_SetValueForKey( buf, "name", rocketInfo.data.mapList[ i ].mapName, qfalse );
		Info_SetValueForKey( buf, "levelshot", va( "%d", rocketInfo.data.mapList[ i ].levelShot ), qfalse );

		trap_Rocket_DSAddRow( "mapList", "default", buf );
	}

}

void CG_Rocket_CleanUpMapList( void )
{
	int i;

	for ( i = 0; i < rocketInfo.data.mapCount; ++i )
	{
		BG_Free( rocketInfo.data.mapList[ i ].imageName );
		BG_Free( rocketInfo.data.mapList[ i ].mapLoadName );
		BG_Free( rocketInfo.data.mapList[ i ].mapName );
	}

	rocketInfo.data.mapCount = 0;
}

void CG_Rocket_SetMapListIndex( int index )
{
	rocketInfo.data.mapIndex = index;
}


void CG_Rocket_CleanUpTeamList( void )
{
	rocketInfo.data.playerCount[ TEAM_ALIENS ] = 0;
	rocketInfo.data.playerIndex[ TEAM_HUMANS ] = 0;
	rocketInfo.data.playerCount[ TEAM_NONE ] = 0;
}

void CG_Rocket_SetTeamListPlayer( int index )
{
}

static void nullSortFunc( const char *name, const char *sortBy )
{
}

static void nullExecFunc( const char *table )
{
}

typedef struct
{
	const char *name;
	void ( *build ) ( const char *args );
	void ( *sort ) ( const char *name, const char *sortBy );
	void ( *cleanup ) ( void );
	void ( *set ) ( int index );
	void ( *exec ) ( const char *table );
} dataSourceCmd_t;

static const dataSourceCmd_t dataSourceCmdList[] =
{
	{ "alOutputs", &CG_Rocket_BuildAlOutputs, &nullSortFunc, &CG_Rocket_CleanUpAlOutputs, &CG_Rocket_SetAlOutputsOutput, &nullExecFunc },
	{ "demoList", &CG_Rocket_BuildDemoList, &nullSortFunc, &CG_Rocket_CleanUpDemoList, &CG_Rocket_SetDemoListDemo, &CG_Rocket_ExecDemoList },
	{ "languages", &CG_Rocket_BuildLanguageList, &nullSortFunc, &CG_Rocket_CleanUpLanguageList, &CG_Rocket_SetLanguageListLanguage, &nullExecFunc },
	{ "mapList", &CG_Rocket_BuildMapList, &nullSortFunc, &CG_Rocket_CleanUpMapList, &CG_Rocket_SetMapListIndex, &nullExecFunc },
	{ "modList", &CG_Rocket_BuildModList, &nullSortFunc, &CG_Rocket_CleanUpModList, &CG_Rocket_SetModListMod, &nullExecFunc },
	{ "resolutions", &CG_Rocket_BuildResolutionList, &CG_Rocket_SortResolutionList, &CG_Rocket_CleanUpResolutionList, &CG_Rocket_SetResolutionListResolution, &nullExecFunc },
	{ "server_browser", &CG_Rocket_BuildServerList, &CG_Rocket_SortServerList, &CG_Rocket_CleanUpServerList, &CG_Rocket_SetServerListServer, &CG_Rocket_ExecServerList },
	{ "teams", &CG_Rocket_BuildTeamList, &CG_Rocket_SortTeamList, &CG_Rocket_CleanUpTeamList, &CG_Rocket_SetTeamListPlayer, &nullExecFunc },
	{ "voipInputs", &CG_Rocket_BuildVoIPInputs, &nullSortFunc, &CG_Rocket_CleanUpVoIPInputs, &CG_Rocket_SetVoipInputsInput, &nullExecFunc },

};

static const size_t dataSourceCmdListCount = ARRAY_LEN( dataSourceCmdList );

static int dataSourceCmdCmp( const void *a, const void *b )
{
	return Q_stricmp( ( const char * ) a, ( ( dataSourceCmd_t * ) b )->name );
}

void CG_Rocket_BuildDataSource( const char *dataSrc, const char *table )
{
	dataSourceCmd_t *cmd;

	// No data
	if ( !dataSrc )
	{
		return;

	}

	cmd = bsearch( dataSrc, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd )
	{
		cmd->build( table );
	}
}

void CG_Rocket_SortDataSource( const char *dataSource, const char *name, const char *sortBy )
{
	dataSourceCmd_t *cmd;

	cmd = bsearch( dataSource, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd && cmd->sort )
	{
		cmd->sort( name, sortBy );
	}
}

void CG_Rocket_SetDataSourceIndex( const char *dataSource, int index )
{
	dataSourceCmd_t *cmd;

	cmd = bsearch( dataSource, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd && cmd->set )
	{
		cmd->set( index );
	}
}

void CG_Rocket_ExecDataSource( const char *dataSource, const char *table )
{
	dataSourceCmd_t *cmd;

	cmd = bsearch( dataSource, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd && cmd->exec )
	{
		cmd->exec( table );
	}
}

void CG_Rocket_RegisterDataSources( void )
{
	int i;

	for ( i = 0; i < dataSourceCmdListCount; ++i )
	{
		trap_Rocket_RegisterDataSource( dataSourceCmdList[ i ].name );
	}
}

