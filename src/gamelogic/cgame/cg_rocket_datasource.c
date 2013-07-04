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

static void AddToServerList( char *name, char *label, int clients, int bots, int ping, int maxClients, char *addr, int netSrc )
{
	server_t *node;

	if ( rocketInfo.data.serverCount[ netSrc ] == MAX_SERVERS )
	{
		return;
	}

	node = &rocketInfo.data.servers[ netSrc ][ rocketInfo.data.serverCount[ netSrc ] ];

	node->name = BG_strdup( name );
	node->clients = clients;
	node->bots = bots;
	node->ping = ping;
	node->maxClients = maxClients;
	node->addr = BG_strdup( addr );
	node->label = BG_strdup( label );

	rocketInfo.data.serverCount[ netSrc ]++;
}

static void CG_Rocket_SetServerListServer( const char *table, int index )
{
	int netSrc = CG_StringToNetSource( table );

	if ( !Q_stricmp( table, "severInfo" ) || !Q_stricmp( table, "serverPlayers" ) )
	{
		return;
	}

	rocketInfo.data.serverIndex[ netSrc ] = index;
	rocketInfo.currentNetSrc = netSrc;
	CG_Rocket_BuildServerInfo();
}
#define MAX_SERVERSTATUS_LINES 4096
void CG_Rocket_BuildServerInfo( void )
{
	static char serverInfoText[ MAX_SERVERSTATUS_LINES ];
	static rocketState_t state;
	char buf[ MAX_INFO_STRING ];
	const char *p;
	server_t *server;
	int netSrc = rocketInfo.currentNetSrc;

	int serverIndex = rocketInfo.data.serverIndex[ netSrc ];

	if ( serverIndex >= rocketInfo.data.serverCount[ netSrc ] || serverIndex < 0 )
	{
		return;
	}

	server = &rocketInfo.data.servers[ netSrc ][ serverIndex ];

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
		trap_Rocket_DSClearTable( "server_browser", "serverPlayers" );

		p = serverInfoText;

		while ( *p )
		{
			Info_NextPair( &p, key, value );
			if ( key[ 0 ] )
			{
				Info_SetValueForKey( buf, "cvar", key, qfalse );
				Info_SetValueForKey( buf, "value", value, qfalse );
				trap_Rocket_DSAddRow( "server_browser", "serverInfo", buf );
				*buf = '\0';
			}
			else
			{
				break;
			}
		}

		// Parse first set of players
		sscanf( value, "%d %d %s", &score, &ping, name );
		Info_SetValueForKey( buf, "num", va( "%d", i++ ), qfalse );
		Info_SetValueForKeyRocket( buf, "name", name );
		Info_SetValueForKey( buf, "score", va( "%d", score ), qfalse );
		Info_SetValueForKey( buf, "ping", va( "%d", ping ), qfalse );
		trap_Rocket_DSAddRow( "server_browser", "serverPlayers", buf );

		while ( *p )
		{
			Info_NextPair( &p, key, value );
			if ( key[ 0 ] )
			{
				sscanf( key, "%d %d %s", &score, &ping, name );
				Info_SetValueForKey( buf, "num", va( "%d", i++ ), qfalse );
				Info_SetValueForKeyRocket( buf, "name", name );
				Info_SetValueForKey( buf, "score", va( "%d", score ), qfalse );
				Info_SetValueForKey( buf, "ping", va( "%d", ping ), qfalse );
				trap_Rocket_DSAddRow( "server_browser", "serverPlayers", buf );
			}

			if ( value[ 0 ] )
			{
				sscanf( value, "%d %d %s", &score, &ping, name );
				Info_SetValueForKey( buf, "num", va( "%d", i++ ), qfalse );
				Info_SetValueForKeyRocket( buf, "name", name );
				Info_SetValueForKey( buf, "score", va( "%d", score ), qfalse );
				Info_SetValueForKey( buf, "ping", va( "%d", ping ), qfalse );
				trap_Rocket_DSAddRow( "server_browser", "serverPlayers", buf );
			}
		}

		rocketInfo.rocketState = state;
	}

}

static void CG_Rocket_BuildServerList( const char *args )
{
	char data[ MAX_INFO_STRING ] = { 0 };
	int netSrc = CG_StringToNetSource( args );
	int i;

	// Only refresh once every second
	if ( trap_Milliseconds() < 1000 + rocketInfo.serversLastRefresh )
	{
		return;
	}

	rocketInfo.currentNetSrc = netSrc;
	rocketInfo.rocketState = RETRIEVING_SERVERS;

	if ( netSrc != AS_FAVORITES )
	{
		int numServers;

		trap_Rocket_DSClearTable( "server_browser", args );

		trap_LAN_MarkServerVisible( netSrc, -1, qtrue );

		numServers = trap_LAN_GetServerCount( netSrc );

		for ( i = 0; i < numServers; ++i )
		{
			char info[ MAX_STRING_CHARS ];
			int ping, bots, clients, maxClients;

			Com_Memset( &data, 0, sizeof( data ) );

			if ( !trap_LAN_ServerIsVisible( netSrc, i ) )
			{
				continue;
			}

			ping = trap_LAN_GetServerPing( netSrc, i );

			if ( ping >= 0 || !Q_stricmp( args, "favorites" ) )
			{
				char addr[ 25 ];
				trap_LAN_GetServerInfo( netSrc, i, info, sizeof( info ) );

				bots = atoi( Info_ValueForKey( info, "bots" ) );
				clients = atoi( Info_ValueForKey( info, "clients" ) );
				maxClients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
				Q_strncpyz( addr, Info_ValueForKey( info, "addr" ), sizeof( addr ) );
				AddToServerList( Info_ValueForKey( info, "hostname" ), Info_ValueForKey( info, "label" ), clients, bots, ping, maxClients, addr, netSrc );
			}
		}

		for ( i = 0; i < rocketInfo.data.serverCount[ netSrc ]; ++i )
		{
			if ( rocketInfo.data.servers[ netSrc ][ i ].ping <= 0 )
			{
				continue;
			}

			Info_SetValueForKey( data, "name", rocketInfo.data.servers[ netSrc ][ i ].name, qfalse );
			Info_SetValueForKey( data, "players", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].clients ), qfalse );
			Info_SetValueForKey( data, "bots", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].bots ), qfalse );
			Info_SetValueForKey( data, "ping", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].ping ), qfalse );
			Info_SetValueForKey( data, "maxClients", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].maxClients ), qfalse );
			Info_SetValueForKey( data, "addr", rocketInfo.data.servers[ netSrc ][ i ].addr, qfalse );
			Info_SetValueForKey( data, "label", rocketInfo.data.servers[ netSrc ][ i ].label, qfalse );

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
	int netSrc = CG_StringToNetSource( name );
	int  i;

	if ( !Q_stricmp( sortBy, "ping" ) )
	{
		qsort( rocketInfo.data.servers, rocketInfo.data.serverCount[ netSrc ], sizeof( server_t ), &ServerListCmpByPing );
	}

	trap_Rocket_DSClearTable( "server_browser", name );

	for ( i = 0; i < rocketInfo.data.serverCount[ netSrc ]; ++i )
	{
		if ( rocketInfo.data.servers[ netSrc ][ i ].ping <= 0 )
		{
			continue;
		}

		Info_SetValueForKey( data, "name", rocketInfo.data.servers[ netSrc ][ i ].name, qfalse );
		Info_SetValueForKey( data, "players", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].clients ), qfalse );
		Info_SetValueForKey( data, "bots", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].bots ), qfalse );
		Info_SetValueForKey( data, "ping", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].ping ), qfalse );
		Info_SetValueForKey( data, "maxClients", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].maxClients ), qfalse );
		Info_SetValueForKey( data, "addr", rocketInfo.data.servers[ netSrc ][ i ].addr, qfalse );
		Info_SetValueForKey( data, "label", rocketInfo.data.servers[ netSrc ][ i ].label, qfalse );

		trap_Rocket_DSAddRow( "server_browser", name, data );
	}
}

void CG_Rocket_CleanUpServerList( const char *table )
{
	int i;
	int j;
	int netSrc = CG_StringToNetSource( table );

	for ( i = AS_LOCAL; i <= AS_FAVORITES; ++i )
	{
		if ( !table || !*table || i == netSrc )
		{
			for ( j = 0; j < rocketInfo.data.serverCount[ i ]; ++j )
			{
				BG_Free( rocketInfo.data.servers[ i ][ j ].name );
				BG_Free( rocketInfo.data.servers[ i ][ j ].label );
				BG_Free( rocketInfo.data.servers[ i ][ j ].addr );
				rocketInfo.data.serverCount[ i ] = 0;
			}
		}
	}
}

static void CG_Rocket_FilterServerList( const char *table, const char *filter )
{
	const char *str = ( table && *table ) ? table : CG_NetSourceToString( rocketInfo.currentNetSrc );
	int netSrc = CG_StringToNetSource( str );
	int i;

	trap_Rocket_DSClearTable( "server_browser", str );

	for ( i = 0; i < rocketInfo.data.serverCount[ netSrc ]; ++i )
	{
		char name[ MAX_INFO_VALUE ];

		Q_strncpyz( name, rocketInfo.data.servers[ netSrc ][ i ].name, sizeof( name ) );
		if ( Q_stristr( Q_CleanStr( name ), filter ) )
		{
			char data[ MAX_INFO_STRING ] = { 0 };

			Info_SetValueForKey( data, "name", rocketInfo.data.servers[ netSrc ][ i ].name, qfalse );
			Info_SetValueForKey( data, "players", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].clients ), qfalse );
			Info_SetValueForKey( data, "bots", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].bots ), qfalse );
			Info_SetValueForKey( data, "ping", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].ping ), qfalse );
			Info_SetValueForKey( data, "maxClients", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].maxClients ), qfalse );
			Info_SetValueForKey( data, "addr", rocketInfo.data.servers[ netSrc ][ i ].addr, qfalse );
			Info_SetValueForKey( data, "label", rocketInfo.data.servers[ netSrc ][ i ].label, qfalse );

			trap_Rocket_DSAddRow( "server_browser", str, data );
		}
	}
}

void CG_Rocket_ExecServerList( const char *table )
{
	int netSrc = CG_StringToNetSource( table );
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s", rocketInfo.data.servers[ netSrc ][ rocketInfo.data.serverIndex[ netSrc ] ].addr ) );
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

void CG_Rocket_SetResolutionListResolution( const char *table, int index )
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


void CG_Rocket_CleanUpResolutionList( const char *table )
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

void CG_Rocket_SetLanguageListLanguage( const char *table, int index )
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

void CG_Rocket_CleanUpLanguageList( const char *args )
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

void CG_Rocket_SetVoipInputsInput( const char *args, int index )
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

void CG_Rocket_CleanUpVoIPInputs( const char *table )
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

void CG_Rocket_SetAlOutputsOutput( const char *table, int index )
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

void CG_Rocket_CleanUpAlOutputs( const char *args )
{
	int i;

	for ( i = 0; i < rocketInfo.data.alOutputsCount; ++i )
	{
		BG_Free( rocketInfo.data.alOutputs[ i ] );
	}

	rocketInfo.data.alOutputsCount = 0;
}

void CG_Rocket_SetModListMod( const char *table, int index )
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

void CG_Rocket_CleanUpModList( const char *args )
{
	int i;

	for ( i = 0; i < rocketInfo.data.modCount; ++i )
	{
		BG_Free( rocketInfo.data.modList[ i ].name );
		BG_Free( rocketInfo.data.modList[ i ].description );
	}

	rocketInfo.data.modCount = 0;
}

void CG_Rocket_SetDemoListDemo( const char *table, int index )
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

void CG_Rocket_CleanUpDemoList( const char *args )
{
	int i;

	for ( i = 0; i < rocketInfo.data.demoCount; ++i )
	{
		BG_Free( rocketInfo.data.demoList[ i ] );
	}

	rocketInfo.data.demoCount = 0;
}

void CG_Rocket_BuildPlayerList( const char *args )
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
	trap_Rocket_DSClearTable( "playerList", "spectators" );
	trap_Rocket_DSClearTable( "playerList", "aliens" );
	trap_Rocket_DSClearTable( "playerList", "humans" );

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
				trap_Rocket_DSAddRow( "playerList", "aliens", buf );
				break;

			case TEAM_HUMANS:
				rocketInfo.data.playerList[ score->team ][ rocketInfo.data.playerIndex[ TEAM_HUMANS ]++ ] = i;
				trap_Rocket_DSAddRow( "playerList", "humans", buf );
				break;

			case TEAM_NONE:
				rocketInfo.data.playerList[ score->team ][ rocketInfo.data.playerCount[ TEAM_NONE ]++ ] = i;
				trap_Rocket_DSAddRow( "playerList", "spectators", buf );
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

void CG_Rocket_SortPlayerList( const char *name, const char *sortBy )
{
	int i;
	clientInfo_t *ci;
	score_t *score;
	char buf[ MAX_INFO_STRING ];

	// Do not sort list if not currently playing
	if ( rocketInfo.rocketState < PLAYING )
	{
		return;
	}



	if ( !Q_stricmp( "score", sortBy ) )
	{
		qsort( rocketInfo.data.playerList[ TEAM_NONE ], rocketInfo.data.playerCount[ TEAM_NONE ], sizeof( int ), &PlayerListCmpByScore );
		qsort( rocketInfo.data.playerList[ TEAM_ALIENS ], rocketInfo.data.playerCount[ TEAM_ALIENS ], sizeof( int ), &PlayerListCmpByScore );
		qsort( rocketInfo.data.playerList[ TEAM_HUMANS ], rocketInfo.data.playerIndex[ TEAM_HUMANS ], sizeof( int ), &PlayerListCmpByScore );
	}

	// Clear old values. Always build all three teams.
	trap_Rocket_DSClearTable( "playerList", "spectators" );
	trap_Rocket_DSClearTable( "playerList", "aliens" );
	trap_Rocket_DSClearTable( "playerList", "humans" );

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

		trap_Rocket_DSAddRow( "playerList", "spectators", buf );
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
		trap_Rocket_DSAddRow( "playerList", "spectators", buf );
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

		trap_Rocket_DSAddRow( "playerList", "spectators", buf );
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

void CG_Rocket_CleanUpMapList( const char *args )
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

void CG_Rocket_SetMapListIndex( const char *table, int index )
{
	rocketInfo.data.mapIndex = index;
}


void CG_Rocket_CleanUpPlayerList( const char *args )
{
	rocketInfo.data.playerCount[ TEAM_ALIENS ] = 0;
	rocketInfo.data.playerIndex[ TEAM_HUMANS ] = 0;
	rocketInfo.data.playerCount[ TEAM_NONE ] = 0;
}

void CG_Rocket_SetPlayerListPlayer( const char *table, int index )
{
}

void CG_Rocket_BuildTeamList( const char *args )
{
	static const char *data[] = {
		"\\name\\Aliens\\description\\The Alien Team\n\n"
		"The Aliens' strengths are in movement and the ability to "
		"quickly construct new bases quickly. They possess a range "
		"of abilities including basic melee attacks, movement-"
		"crippling poisons and more.\\"
		,
		"\\name\\Humans\\description\\The Human Team\n\n"
		"The humans are the masters of technology. Although their "
		"bases take long to construct, their automated defense "
		"ensures they stay built. A wide range of upgrades and "
		"weapons are available to the humans, each contributing "
		"to eradicate the alien threat.\\"
		,
		"\\name\\Spectate\\description\\Watch the game without playing.\\"
		,
		"\\name\\Auto Select\\description\\Join the team with the least players.\\"
		,
		NULL
	};

	int i = 0;

	while ( data[ i ] )
	{
		trap_Rocket_DSAddRow( "teamList", "default", data[ i++ ] );
	}
}

void CG_Rocket_SetTeamList( const char *table, int index )
{
	rocketInfo.data.selectedTeamIndex = index;
}

void CG_Rocket_ExecTeamList( const char *table )
{
	const char *cmd = NULL;

	switch ( rocketInfo.data.selectedTeamIndex )
	{
		case 1: cmd = "team aliens"; break;
		case 2: cmd = "team humans"; break;
		case 3: cmd = "team spectate"; break;
		case 4: cmd = "team auto"; break;
	}

	if ( cmd )
	{
		trap_SendConsoleCommand( cmd );
	}
}

void CG_Rocket_CleanUpTeamList( const char *table )
{
	rocketInfo.data.selectedTeamIndex = -1;
}



static void nullSortFunc( const char *name, const char *sortBy )
{
}

static void nullExecFunc( const char *table )
{
}

static void nullFilterFunc( const char *table, const char *filter )
{
}

typedef struct
{
	const char *name;
	void ( *build ) ( const char *args );
	void ( *sort ) ( const char *name, const char *sortBy );
	void ( *cleanup ) ( const char *table );
	void ( *set ) ( const char *table, int index );
	void ( *filter ) ( const char *table, const char *filter );
	void ( *exec ) ( const char *table );
} dataSourceCmd_t;

static const dataSourceCmd_t dataSourceCmdList[] =
{
	{ "alOutputs", &CG_Rocket_BuildAlOutputs, &nullSortFunc, &CG_Rocket_CleanUpAlOutputs, &CG_Rocket_SetAlOutputsOutput, &nullFilterFunc, &nullExecFunc },
	{ "demoList", &CG_Rocket_BuildDemoList, &nullSortFunc, &CG_Rocket_CleanUpDemoList, &CG_Rocket_SetDemoListDemo, &nullFilterFunc, &CG_Rocket_ExecDemoList },
	{ "languages", &CG_Rocket_BuildLanguageList, &nullSortFunc, &CG_Rocket_CleanUpLanguageList, &CG_Rocket_SetLanguageListLanguage, &nullFilterFunc, &nullExecFunc },
	{ "mapList", &CG_Rocket_BuildMapList, &nullSortFunc, &CG_Rocket_CleanUpMapList, &CG_Rocket_SetMapListIndex, &nullFilterFunc, &nullExecFunc },
	{ "modList", &CG_Rocket_BuildModList, &nullSortFunc, &CG_Rocket_CleanUpModList, &CG_Rocket_SetModListMod, &nullFilterFunc, &nullExecFunc },
	{ "resolutions", &CG_Rocket_BuildResolutionList, &CG_Rocket_SortResolutionList, &CG_Rocket_CleanUpResolutionList, &CG_Rocket_SetResolutionListResolution, &nullFilterFunc, &nullExecFunc },
	{ "server_browser", &CG_Rocket_BuildServerList, &CG_Rocket_SortServerList, &CG_Rocket_CleanUpServerList, &CG_Rocket_SetServerListServer, &CG_Rocket_FilterServerList, &CG_Rocket_ExecServerList },
	{ "playerList", &CG_Rocket_BuildPlayerList, &CG_Rocket_SortPlayerList, &CG_Rocket_CleanUpPlayerList, &CG_Rocket_SetPlayerListPlayer, &nullFilterFunc, &nullExecFunc },
	{ "teamList", &CG_Rocket_BuildTeamList, &nullSortFunc, &CG_Rocket_CleanUpTeamList, &CG_Rocket_SetTeamList, &nullFilterFunc, &CG_Rocket_ExecTeamList },
	{ "voipInputs", &CG_Rocket_BuildVoIPInputs, &nullSortFunc, &CG_Rocket_CleanUpVoIPInputs, &CG_Rocket_SetVoipInputsInput, &nullFilterFunc, &nullExecFunc },

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
		cmd->cleanup( table );
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

void CG_Rocket_SetDataSourceIndex( const char *dataSource, const char *table, int index )
{
	dataSourceCmd_t *cmd;

	cmd = bsearch( dataSource, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd && cmd->set )
	{
		cmd->set( table, index );
	}
}

void CG_Rocket_FilterDataSource( const char *dataSource, const char *table, const char *filter )
{
	dataSourceCmd_t *cmd;

	cmd = bsearch( dataSource, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd && cmd->filter )
	{
		cmd->filter( table, filter );
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

void CG_Rocket_CleanUpDataSources( void )
{
	int i;

	for ( i = 0; i < dataSourceCmdListCount; ++i )
	{
		dataSourceCmdList[ i ].cleanup( NULL );
	}
}
