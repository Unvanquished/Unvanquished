/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "common/Common.h"
#include "cg_local.h"
#include "shared/parse.h"

static bool AddToServerList( const char *name, const char *label, int clients, int bots, int ping, int maxClients, char *mapName, char *addr, int netSrc )
{
	server_t *node;

	if ( rocketInfo.data.serverCount[ netSrc ] == MAX_SERVERS )
	{
		return false;
	}

	if ( !*name || !*mapName )
	{
		return false;
	}

	node = &rocketInfo.data.servers[ netSrc ][ rocketInfo.data.serverCount[ netSrc ] ];

	node->name = BG_strdup( name );
	node->clients = clients;
	node->bots = bots;
	node->ping = ping;
	node->maxClients = maxClients;
	node->addr = BG_strdup( addr );
	node->label = BG_strdup( label );
	node->mapName = BG_strdup( mapName );

	rocketInfo.data.serverCount[ netSrc ]++;
	return true;
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
void CG_Rocket_BuildServerInfo()
{
	static char serverInfoText[ MAX_SERVERSTATUS_LINES ];
	char buf[ MAX_INFO_STRING ];
	const char *p;
	server_t *server;
	int netSrc = rocketInfo.currentNetSrc;

	int serverIndex = rocketInfo.data.serverIndex[ netSrc ];

	if ( serverIndex >= rocketInfo.data.serverCount[ netSrc ] || serverIndex < 0 )
	{
		return;
	}

	if ( rocketInfo.realtime < rocketInfo.serverStatusLastRefresh + 500 )
	{
		return;
	}

	buf[0] = 0;

	rocketInfo.serverStatusLastRefresh = rocketInfo.realtime;

	if ( !rocketInfo.data.buildingServerInfo )
	{
		rocketInfo.data.buildingServerInfo = true;
	}

	server = &rocketInfo.data.servers[ netSrc ][ serverIndex ];


	if ( trap_LAN_ServerStatus( server->addr, serverInfoText, sizeof( serverInfoText ) ) )
	{
		int i = 0, score, ping;
		const char *start, *end;
		static char key[BIG_INFO_VALUE], value[ BIG_INFO_VALUE ];
		char name[ MAX_STRING_CHARS ];

		Rocket_DSClearTable( "server_browser", "serverInfo" );
		Rocket_DSClearTable( "server_browser", "serverPlayers" );

		p = serverInfoText;

		while ( *p )
		{
			Info_NextPair( &p, key, value );

			if ( key[ 0 ] )
			{
				Info_SetValueForKey( buf, "cvar", key, false );
				Info_SetValueForKey( buf, "value", value, false );
				Rocket_DSAddRow( "server_browser", "serverInfo", buf );
				*buf = '\0';
			}

			else
			{
				break;
			}
		}

		// Parse first set of players
		sscanf( value, "%d %d", &score, &ping );
		start = strchr( value, '"' );

		if ( !start )
		{
			trap_LAN_ResetServerStatus();
			rocketInfo.data.buildingServerInfo = false;
			return;
		}

		end = strchr( start + 1, '"' );

		if ( !end )
		{
			trap_LAN_ResetServerStatus();
			rocketInfo.data.buildingServerInfo = false;
			return;
		}

		Q_strncpyz( name, start + 1, end - start );
		start = end = NULL;
		Info_SetValueForKey( buf, "num", va( "%d", i++ ), false );
		Info_SetValueForKeyRocket( buf, "name", name, false );
		Info_SetValueForKey( buf, "score", va( "%d", score ), false );
		Info_SetValueForKey( buf, "ping", va( "%d", ping ), false );
		Rocket_DSAddRow( "server_browser", "serverPlayers", buf );

		while ( *p )
		{
			Info_NextPair( &p, key, value );

			if ( key[ 0 ] )
			{
				sscanf( key, "%d %d", &score, &ping );
				start = strchr( key, '"' );

				if ( !start )
				{
					break;
				}

				end = strchr( start + 1, '"' );

				if ( !end )
				{
					break;
				}

				Q_strncpyz( name, start + 1, end - start );
				start = end = NULL;
				Info_SetValueForKey( buf, "num", va( "%d", i++ ), false );
				Info_SetValueForKeyRocket( buf, "name", name, false );
				Info_SetValueForKey( buf, "score", va( "%d", score ), false );
				Info_SetValueForKey( buf, "ping", va( "%d", ping ), false );
				Rocket_DSAddRow( "server_browser", "serverPlayers", buf );
			}

			if ( value[ 0 ] )
			{
				sscanf( value, "%d %d", &score, &ping );
				start = strchr( value, '"' );

				if ( !start )
				{
					break;
				}

				end = strchr( start + 1, '"' );

				if ( !end )
				{
					break;
				}

				Q_strncpyz( name, start + 1, end - start );
				start = end = NULL;
				Info_SetValueForKey( buf, "num", va( "%d", i++ ), false );
				Info_SetValueForKeyRocket( buf, "name", name, false );
				Info_SetValueForKey( buf, "score", va( "%d", score ), false );
				Info_SetValueForKey( buf, "ping", va( "%d", ping ), false );
				Rocket_DSAddRow( "server_browser", "serverPlayers", buf );
			}
		}

		trap_LAN_ResetServerStatus();
		rocketInfo.data.buildingServerInfo = false;
	}

}

static void CG_Rocket_BuildServerList( const char *args )
{
	rocketInfo.currentNetSrc = CG_StringToNetSource( args );
	Rocket_DSClearTable("server_browser", args );
	CG_Rocket_CleanUpServerList( args );
	CG_Rocket_BuildServerList();
}

void CG_Rocket_BuildServerList()
{
	int i;

	rocketInfo.data.retrievingServers = true;

	int netSrc = rocketInfo.currentNetSrc;
	const char *srcName = CG_NetSourceToString( netSrc );

	// Hack to avoid calling UpdateVisiblePings in the same frame that /globalservers
	// is sent, which would ping the old server list
	if ( rocketInfo.realtime == rocketInfo.serversLastRefresh )
	{
		return;
	}

	int numServers = trap_LAN_GetServerCount( netSrc );

	// Still waiting for a response...
	// HACK: assume not done if there are 0 servers, although it could be true
	if ( numServers <= 0 )
	{
		return;
	}

	trap_LAN_MarkServerVisible( netSrc, -1, true );

	// Only refresh once every 200 ms
	if ( rocketInfo.realtime < 200 + rocketInfo.serversLastRefresh )
	{
		trap_LAN_UpdateVisiblePings( netSrc ); // keep the ping queue moving
		return;
	}

	rocketInfo.serversLastRefresh = rocketInfo.realtime;

	rocketInfo.data.haveServerInfo[ netSrc ].resize( numServers );

	if ( !trap_LAN_UpdateVisiblePings( netSrc ) )
	{
		// all pings have completed
		rocketInfo.data.retrievingServers = false;
	}

	int oldServerCount = rocketInfo.data.serverCount[ netSrc ];

	for ( i = 0; i < numServers; ++i )
	{
		if ( rocketInfo.data.haveServerInfo[ netSrc ][ i ] )
		{
			continue;
		}

		char info[ MAX_STRING_CHARS ];
		int ping, bots, clients, maxClients;

		if ( !trap_LAN_ServerIsVisible( netSrc, i ) )
		{
			continue;
		}

		ping = trap_LAN_GetServerPing( netSrc, i );

		if ( ping > 0 )
		{
			char addr[ 50 ]; // long enough for IPv6 literal plus port no.
			char mapname[ 256 ];
			trap_LAN_GetServerInfo( netSrc, i, info, sizeof( info ) );

			bots = atoi( Info_ValueForKey( info, "bots" ) );
			clients = atoi( Info_ValueForKey( info, "clients" ) );
			maxClients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
			Q_strncpyz( addr, Info_ValueForKey( info, "addr" ), sizeof( addr ) );
			Q_strncpyz( mapname, Info_ValueForKey( info, "mapname" ), sizeof( mapname ) );
			rocketInfo.data.haveServerInfo[ netSrc ][ i ] =
				AddToServerList( Info_ValueForKey( info, "hostname" ), Info_ValueForKey( info, "label" ), clients, bots, ping, maxClients, mapname, addr, netSrc );
		}
	}

	for ( i = oldServerCount; i < rocketInfo.data.serverCount[ netSrc ]; ++i )
	{
		if ( rocketInfo.data.servers[ netSrc ][ i ].ping <= 0 )
		{
			continue;
		}

		char data[ MAX_INFO_STRING ];
		data[ 0 ] = '\0';

		Info_SetValueForKey( data, "name", rocketInfo.data.servers[ netSrc ][ i ].name, false );
		Info_SetValueForKey( data, "players", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].clients ), false );
		Info_SetValueForKey( data, "bots", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].bots ), false );
		Info_SetValueForKey( data, "ping", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].ping ), false );
		Info_SetValueForKey( data, "maxClients", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].maxClients ), false );
		Info_SetValueForKey( data, "addr", rocketInfo.data.servers[ netSrc ][ i ].addr, false );
		Info_SetValueForKey( data, "label", rocketInfo.data.servers[ netSrc ][ i ].label, false );
		Info_SetValueForKey( data, "map", rocketInfo.data.servers[ netSrc ][ i ].mapName, false );

		Rocket_DSAddRow( "server_browser", srcName, data );
	}
}

static int ServerListCmpByPing( const void *one, const void *two )
{
	server_t* a = ( server_t* ) one;
	server_t* b = ( server_t* ) two;

	if ( a->ping > b->ping ) return 1;

	if ( b->ping > a->ping ) return -1;

	if ( a->ping == b->ping )  return 0;

	return 0; // silence compiler
}

static int ServerListCmpByName( const void* one, const void* two )
{
	char cleanName1[ MAX_INFO_VALUE ];
	char cleanName2[ MAX_INFO_VALUE ];
	server_t* a = ( server_t* ) one;
	server_t* b = ( server_t* ) two;

	Color::StripColors( a->name, cleanName1, sizeof( cleanName1 ) );
	Color::StripColors( b->name, cleanName2, sizeof( cleanName2 ) );

	return Q_stricmp( cleanName1, cleanName2 );
}

static int ServerListCmpByMap( const void* one, const void* two )
{
	server_t* a = ( server_t* ) one;
	server_t* b = ( server_t* ) two;

	return Q_stricmp( a->mapName, b->mapName );
}

static int ServerListCmpByPlayers( const void* one, const void* two )
{
	server_t* a = ( server_t* ) one;
	server_t* b = ( server_t* ) two;

	if ( a->clients > b->clients ) return 1;

	if ( b->clients > a->clients ) return -1;

	if ( a->clients == b->clients )  return 0;

	return 0; // silence compiler
}

static void CG_Rocket_SortServerList( const char *name, const char *sortBy )
{
	char data[ MAX_INFO_STRING ] = { 0 };
	int netSrc = CG_StringToNetSource( name );
	int  i;

	if ( !Q_stricmp( sortBy, "ping" ) )
	{
		qsort( rocketInfo.data.servers[ netSrc ], rocketInfo.data.serverCount[ netSrc ], sizeof( server_t ), &ServerListCmpByPing );
	}
	else if ( !Q_stricmp( sortBy, "name" ) )
	{
		qsort( rocketInfo.data.servers[ netSrc ], rocketInfo.data.serverCount[ netSrc ], sizeof( server_t ), &ServerListCmpByName );
	}
	else if ( !Q_stricmp( sortBy, "players" ) )
	{
		qsort( rocketInfo.data.servers[ netSrc ], rocketInfo.data.serverCount[ netSrc ], sizeof( server_t ), &ServerListCmpByPlayers );
	}
	else if ( !Q_stricmp( sortBy, "map" ) )
	{
		qsort( rocketInfo.data.servers[ netSrc ], rocketInfo.data.serverCount[ netSrc ], sizeof( server_t ), &ServerListCmpByMap );
	}

	Rocket_DSClearTable( "server_browser", name );

	for ( i = 0; i < rocketInfo.data.serverCount[ netSrc ]; ++i )
	{
		if ( rocketInfo.data.servers[ netSrc ][ i ].ping <= 0 )
		{
			continue;
		}

		Info_SetValueForKey( data, "name", rocketInfo.data.servers[ netSrc ][ i ].name, false );
		Info_SetValueForKey( data, "players", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].clients ), false );
		Info_SetValueForKey( data, "bots", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].bots ), false );
		Info_SetValueForKey( data, "ping", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].ping ), false );
		Info_SetValueForKey( data, "maxClients", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].maxClients ), false );
		Info_SetValueForKey( data, "addr", rocketInfo.data.servers[ netSrc ][ i ].addr, false );
		Info_SetValueForKey( data, "label", rocketInfo.data.servers[ netSrc ][ i ].label, false );
		Info_SetValueForKey( data, "map", rocketInfo.data.servers[ netSrc ][ i ].mapName, false );

		Rocket_DSAddRow( "server_browser", name, data );
	}
}

void CG_Rocket_CleanUpServerList( const char *table )
{
	int i;
	int j;
	int netSrc = CG_StringToNetSource( table );

	for ( i = AS_LOCAL; i < AS_NUM_TYPES; ++i )
	{
		if ( !table || !*table || i == netSrc )
		{
			for ( j = 0; j < rocketInfo.data.serverCount[ i ]; ++j )
			{
				BG_Free( rocketInfo.data.servers[ i ][ j ].name );
				BG_Free( rocketInfo.data.servers[ i ][ j ].label );
				BG_Free( rocketInfo.data.servers[ i ][ j ].addr );
				BG_Free( rocketInfo.data.servers[ i ][ j ].mapName );
			}
			rocketInfo.data.serverCount[ i ] = 0;
			rocketInfo.data.haveServerInfo[ i ].clear();
		}
	}
}

static void CG_Rocket_FilterServerList( const char *table, const char *filter )
{
	const char *str = ( table && *table ) ? table : CG_NetSourceToString( rocketInfo.currentNetSrc );
	int netSrc = CG_StringToNetSource( str );
	int i;

	Rocket_DSClearTable( "server_browser", str );

	for ( i = 0; i < rocketInfo.data.serverCount[ netSrc ]; ++i )
	{
		char name[ MAX_INFO_VALUE ];
		Color::StripColors( rocketInfo.data.servers[ netSrc ][ i ].name, name, sizeof( name ) );

		if ( Q_stristr( name, filter ) )
		{
			char data[ MAX_INFO_STRING ] = { 0 };

			Info_SetValueForKey( data, "name", rocketInfo.data.servers[ netSrc ][ i ].name, false );
			Info_SetValueForKey( data, "players", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].clients ), false );
			Info_SetValueForKey( data, "bots", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].bots ), false );
			Info_SetValueForKey( data, "ping", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].ping ), false );
			Info_SetValueForKey( data, "maxClients", va( "%d", rocketInfo.data.servers[ netSrc ][ i ].maxClients ), false );
			Info_SetValueForKey( data, "addr", rocketInfo.data.servers[ netSrc ][ i ].addr, false );
			Info_SetValueForKey( data, "label", rocketInfo.data.servers[ netSrc ][ i ].label, false );

			Rocket_DSAddRow( "server_browser", str, data );
		}
	}
}

static void CG_Rocket_ExecServerList( const char *table )
{
	int netSrc = CG_StringToNetSource( table );
	trap_SendConsoleCommand( va( "connect %s", rocketInfo.data.servers[ netSrc ][ rocketInfo.data.serverIndex[ netSrc ] ].addr ) );
}

static void CG_Rocket_SetResolutionListResolution( const char*, int index )
{
	if ( static_cast<size_t>( index ) < rocketInfo.data.resolutions.size() )
	{
		resolution_t res = rocketInfo.data.resolutions[ index ];

		if ( res.width == 0 )
		{
			Cvar::SetValue( "r_mode", "-2" );
		} else {
			Cvar::SetValue( "r_customwidth", std::to_string( std::abs( res.width ) ) );
			Cvar::SetValue( "r_customheight", std::to_string( std::abs( res.height ) ) );
			Cvar::SetValue( "r_mode", "-1" );
		}

		rocketInfo.data.resolutionIndex = index;
	}
}

static void SelectCurrentResolution()
{
	int mode;
	if ( !Str::ParseInt( mode, Cvar::GetValue( "r_mode" ) ) )
	{
		return;
	}

	int width = cgs.glconfig.vidWidth;
	int height = cgs.glconfig.vidHeight;

	if ( mode == -2 && width == cgs.glconfig.displayWidth && height == cgs.glconfig.displayHeight )
	{
		width = height = 0; // see resolution_t comment
	}

	for ( const resolution_t &res : rocketInfo.data.resolutions )
	{
		if ( res.width == width && res.height == height )
		{
			rocketInfo.data.resolutionIndex = int(&res - &rocketInfo.data.resolutions[ 0 ]);
			return;
		}
	}

	rocketInfo.data.resolutionIndex = 0;
	rocketInfo.data.resolutions.insert( rocketInfo.data.resolutions.begin(), {-width, -height} );
}

static void CG_Rocket_BuildResolutionList( const char* )
{
	rocketInfo.data.resolutions.clear();
	rocketInfo.data.resolutionIndex = -1;

	// Add "Same as screen" option
	rocketInfo.data.resolutions.push_back( {0, 0} );

	for ( Parse_WordListSplitter parse(Cvar::GetValue( "r_availableModes" )); *parse; ++parse )
	{
		resolution_t resolution;
		if ( 2 == sscanf( *parse, "%dx%d", &resolution.width, &resolution.height ) )
		{
			rocketInfo.data.resolutions.push_back( resolution );
		}
	}

	// Sort resolutions by size by default (larger first).
	std::sort( rocketInfo.data.resolutions.begin() + 1, rocketInfo.data.resolutions.end(),
	           [](const resolution_t& a, const resolution_t& b) { return a.width * a.height > b.width * b.height; });

	SelectCurrentResolution();

	Rocket_DSClearTable( "resolutions", "default" );
	for ( const resolution_t &res : rocketInfo.data.resolutions )
	{
		char buf[ MAX_INFO_STRING ];
		buf[ 0 ] = '\0';
		Info_SetValueForKey( buf, "width", va( "%d", res.width ), false );
		Info_SetValueForKey( buf, "height", va( "%d", res.height ), false );
		Rocket_DSAddRow( "resolutions", "default", buf );
	}
}

static int CG_Rocket_GetResolutionListIndex( const char* )
{
	return rocketInfo.data.resolutionIndex;
}

static void CG_Rocket_CleanUpResolutionList( const char* )
{
	rocketInfo.data.resolutions.clear();
}

static void AddToAlOutputs( char *name )
{
	if ( rocketInfo.data.alOutputsCount == MAX_OUTPUTS )
	{
		return;
	}

	rocketInfo.data.alOutputs[ rocketInfo.data.alOutputsCount++ ] = name;
}

static void CG_Rocket_SetAlOutputsOutput( const char*, int index )
{
	const char *device = "";
	if (index >= 0 && index < rocketInfo.data.alOutputsCount )
	{
		device = rocketInfo.data.alOutputs[ index ];
	}

	rocketInfo.data.alOutputIndex = index;
	trap_Cvar_Set( "audio.al.device", device );
	trap_Cvar_AddFlags( "audio.al.device", CVAR_ARCHIVE );
}

static void CG_Rocket_BuildAlOutputs( const char* )
{
	char buf[ MAX_STRING_CHARS ], currentDevice[ MAX_STRING_CHARS ];
	char *p, *head;
	int outputs = 0;

	trap_Cvar_VariableStringBuffer( "audio.al.device", currentDevice, sizeof( currentDevice ) );
	trap_Cvar_VariableStringBuffer( "audio.al.availableDevices", buf, sizeof( buf ) );
	head = buf;
	rocketInfo.data.alOutputIndex = -1;

	while ( ( p = strchr( head, '\n' ) ) )
	{
		*p = '\0';

		// Set current device
		if ( !Q_stricmp( currentDevice, head ) )
		{
			rocketInfo.data.alOutputIndex = rocketInfo.data.alOutputsCount;
		}

		AddToAlOutputs( BG_strdup( head ) );
		head = p + 1;
	}

	buf[ 0 ] = '\0';

	for ( outputs = 0; outputs < rocketInfo.data.alOutputsCount; ++outputs )
	{
		Info_SetValueForKey( buf, "name", rocketInfo.data.alOutputs[ outputs ], false );

		Rocket_DSAddRow( "alOutputs", "default", buf );
	}
}

static int CG_Rocket_GetAlOutputIndex( const char* )
{
	return rocketInfo.data.alOutputIndex;
}

static void CG_Rocket_CleanUpAlOutputs( const char* )
{
	int i;

	for ( i = 0; i < rocketInfo.data.alOutputsCount; ++i )
	{
		BG_Free( rocketInfo.data.alOutputs[ i ] );
	}

	rocketInfo.data.alOutputsCount = 0;
}

static void CG_Rocket_SetModListMod( const char*, int index )
{
	rocketInfo.data.modIndex = index;
}

static void CG_Rocket_BuildModList( const char* )
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
		Info_SetValueForKey( dirlist, "name", rocketInfo.data.modList[ i ].name, false );
		Info_SetValueForKey( dirlist, "description", rocketInfo.data.modList[ i ].description, false );

		Rocket_DSAddRow( "modList", "default", dirlist );
	}
}

static void CG_Rocket_CleanUpModList( const char* )
{
	int i;

	for ( i = 0; i < rocketInfo.data.modCount; ++i )
	{
		BG_Free( rocketInfo.data.modList[ i ].name );
		BG_Free( rocketInfo.data.modList[ i ].description );
	}

	rocketInfo.data.modCount = 0;
}

static void CG_Rocket_SetDemoListDemo( const char*, int index )
{
	rocketInfo.data.demoIndex = index;
}

static void CG_Rocket_ExecDemoList( const char* )
{
	trap_SendConsoleCommand( va( "demo %s", rocketInfo.data.demoList[ rocketInfo.data.demoIndex ] ) );
}

static void CG_Rocket_BuildDemoList( const char* )
{
	char  demolist[ 4096 ];
	char demoExt[ 32 ];
	char  *demoname;
	int   i;

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
		auto demoExtLen = strlen( demoExt );

		for ( i = 0; i < rocketInfo.data.demoCount; i++ )
		{
			auto len = strlen( demoname );

			if ( !Q_stricmp( demoname + len - demoExtLen, demoExt ) )
			{
				demoname[ len - demoExtLen ] = '\0';
			}

			rocketInfo.data.demoList[ i ] = BG_strdup( demoname );
			demoname += len + 1;
		}
	}

	demolist[ 0 ] = '\0';

	for ( i = 0; i < rocketInfo.data.demoCount; ++i )
	{
		Info_SetValueForKey( demolist, "name", rocketInfo.data.demoList[ i ], false );

		Rocket_DSAddRow( "demoList", "default", demolist );
	}
}

static void CG_Rocket_CleanUpDemoList( const char* )
{
	int i;

	for ( i = 0; i < rocketInfo.data.demoCount; ++i )
	{
		BG_Free( rocketInfo.data.demoList[ i ] );
	}

	rocketInfo.data.demoCount = 0;
}

void CG_Rocket_BuildPlayerList( const char* )
{
	char buf[ MAX_INFO_STRING ] = { 0 };
	clientInfo_t *ci;
	score_t *score;
	int i;

	//CG_RequestScores();

	// Do not build list if not currently playing
	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
	{
		return;
	}

	// Clear old values. Always build all three teams.
	Rocket_DSClearTable( "playerList", "spectators" );
	Rocket_DSClearTable( "playerList", "aliens" );
	Rocket_DSClearTable( "playerList", "humans" );

	for ( i = 0; i < cg.numScores; ++i )
	{
		score = &cg.scores[ i ];
		ci = &cgs.clientinfo[ score->client ];

		if ( !ci->infoValid )
		{
			continue;
		}

		Info_SetValueForKey( buf, "num", va( "%d", score->client ), false );
		Info_SetValueForKey( buf, "score", va( "%d", score->score ), false );

		const char* B = Info_ValueForKey( CG_ConfigString( CS_SERVERINFO ),"B" );
		const char bot_status = B[ score->client ];

		// HACK: Display bot status in “Ping” column.
		if ( bot_status == '-' )
		{
			// This player is not a bot.
			Info_SetValueForKey( buf, "ping", va( "%d", score->ping ), false );
		}
		// Bot skill can be 0 while spawning, just before skill is set.
		else if ( bot_status >= '0' && bot_status <= '9' )
		{
			// Bot skill.
			Info_SetValueForKey( buf, "ping", va( "sk%c", bot_status ), false );
		}
		else
		{
			// Example: “G” is for “Generating navmeshes”.
			Info_SetValueForKey( buf, "ping", va( "%c", bot_status ), false );
		}

		Info_SetValueForKey( buf, "weapon", va( "%d", score->weapon ), false );
		Info_SetValueForKey( buf, "upgrade", va( "%d", score->upgrade ), false );
		Info_SetValueForKey( buf, "time", va( "%d", score->time ), false );
		Info_SetValueForKey( buf, "credits", va( "%d", ci->credit ), false );
		Info_SetValueForKey( buf, "location", CG_ConfigString( CS_LOCATIONS + ci->location ), false );

		switch ( score->team )
		{
			case TEAM_ALIENS:
				rocketInfo.data.playerList[ score->team ][ rocketInfo.data.playerCount[ TEAM_ALIENS ]++ ] = i;
				Rocket_DSAddRow( "playerList", "aliens", buf );
				break;

			case TEAM_HUMANS:
				rocketInfo.data.playerList[ score->team ][ rocketInfo.data.playerIndex[ TEAM_HUMANS ]++ ] = i;
				Rocket_DSAddRow( "playerList", "humans", buf );
				break;

			case TEAM_NONE:
				rocketInfo.data.playerList[ score->team ][ rocketInfo.data.playerCount[ TEAM_NONE ]++ ] = i;
				Rocket_DSAddRow( "playerList", "spectators", buf );
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

static void CG_Rocket_SortPlayerList( const char*, const char *sortBy )
{
	int i;
	clientInfo_t *ci;
	score_t *score;
	char buf[ MAX_INFO_STRING ];

	// Do not sort list if not currently playing
	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
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
	Rocket_DSClearTable( "playerList", "spectators" );
	Rocket_DSClearTable( "playerList", "aliens" );
	Rocket_DSClearTable( "playerList", "humans" );

	for ( i = 0; i < rocketInfo.data.playerCount[ TEAM_NONE ]; ++i )
	{
		score = &cg.scores[ rocketInfo.data.playerList[ TEAM_NONE ][ i ] ];
		ci = &cgs.clientinfo[ score->client ];

		if ( !ci->infoValid )
		{
			continue;
		}

		Info_SetValueForKey( buf, "num", va( "%d", score->client ), false );
		Info_SetValueForKey( buf, "score", va( "%d", score->score ), false );
		Info_SetValueForKey( buf, "ping", va( "%d", score->ping ), false );
		Info_SetValueForKey( buf, "weapon", va( "%d", score->weapon ), false );
		Info_SetValueForKey( buf, "upgrade", va( "%d", score->upgrade ), false );
		Info_SetValueForKey( buf, "time", va( "%d", score->time ), false );
		Info_SetValueForKey( buf, "credits", va( "%d", ci->credit ), false );
		Info_SetValueForKey( buf, "location", CG_ConfigString( CS_LOCATIONS + ci->location ), false );

		Rocket_DSAddRow( "playerList", "spectators", buf );
	}

	for ( i = 0; i < rocketInfo.data.playerIndex[ TEAM_HUMANS ]; ++i )
	{
		score = &cg.scores[ rocketInfo.data.playerList[ TEAM_HUMANS ][ i ] ];
		ci = &cgs.clientinfo[ score->client ];

		if ( !ci->infoValid )
		{
			continue;
		}

		Info_SetValueForKey( buf, "num", va( "%d", score->client ), false );
		Info_SetValueForKey( buf, "score", va( "%d", score->score ), false );
		Info_SetValueForKey( buf, "ping", va( "%d", score->ping ), false );
		Info_SetValueForKey( buf, "weapon", va( "%d", score->weapon ), false );
		Info_SetValueForKey( buf, "upgrade", va( "%d", score->upgrade ), false );
		Info_SetValueForKey( buf, "time", va( "%d", score->time ), false );
		Info_SetValueForKey( buf, "credits", va( "%d", ci->credit ), false );
		Info_SetValueForKey( buf, "location", CG_ConfigString( CS_LOCATIONS + ci->location ), false );
		Rocket_DSAddRow( "playerList", "humans", buf );
	}

	for ( i = 0; i < rocketInfo.data.playerCount[ TEAM_ALIENS ]; ++i )
	{
		ci = &cgs.clientinfo[ rocketInfo.data.playerList[ TEAM_ALIENS ][ i ] ];
		score = &cg.scores[ rocketInfo.data.playerList[ TEAM_ALIENS ][ i ] ];

		if ( !ci->infoValid )
		{
			continue;
		}

		Info_SetValueForKey( buf, "num", va( "%d", score->client ), false );
		Info_SetValueForKey( buf, "score", va( "%d", score->score ), false );
		Info_SetValueForKey( buf, "ping", va( "%d", score->ping ), false );
		Info_SetValueForKey( buf, "weapon", va( "%d", score->weapon ), false );
		Info_SetValueForKey( buf, "upgrade", va( "%d", score->upgrade ), false );
		Info_SetValueForKey( buf, "time", va( "%d", score->time ), false );
		Info_SetValueForKey( buf, "credits", va( "%d", ci->credit ), false );
		Info_SetValueForKey( buf, "location", CG_ConfigString( CS_LOCATIONS + ci->location ), false );

		Rocket_DSAddRow( "playerList", "aliens", buf );
	}
}

static void CG_Rocket_BuildMapList( const char* )
{
	Rocket_DSClearTable( "mapList", "default" );
	CG_LoadMapList();

	for ( size_t i = 0; i < rocketInfo.data.mapList.size(); ++i )
	{
		char buf[ MAX_INFO_STRING ] = { 0 };
		Info_SetValueForKey( buf, "num", std::to_string( i ).c_str(), false );
		Info_SetValueForKey( buf, "mapName", rocketInfo.data.mapList[ i ].mapLoadName.c_str(), false );
		Info_SetValueForKey( buf, "mapLoadName", rocketInfo.data.mapList[ i ].mapLoadName.c_str(), false );

		Rocket_DSAddRow( "mapList", "default", buf );
	}

	rocketInfo.data.mapIndex = -1;
}

static void CG_Rocket_CleanUpMapList( const char* )
{
}

static void CG_Rocket_SetMapListIndex( const char*, int index )
{
	rocketInfo.data.mapIndex = index;
}


static void CG_Rocket_CleanUpPlayerList( const char* )
{
	rocketInfo.data.playerCount[ TEAM_ALIENS ] = 0;
	rocketInfo.data.playerIndex[ TEAM_HUMANS ] = 0;
	rocketInfo.data.playerCount[ TEAM_NONE ] = 0;
}

static void CG_Rocket_SetPlayerListPlayer( const char*, int )
{
}

enum
{
	ROCKETDS_WEAPONS,
	ROCKETDS_UPGRADES
};

static void CG_Rocket_CleanUpArmouryBuyList( const char *table )
{
	char c = table ? *table : 'd';
	int tblIndex;

	switch ( c )
	{
		case 'W':
		case 'w':
			tblIndex = ROCKETDS_WEAPONS;
			break;

		case 'U':
		case 'u':
			tblIndex = ROCKETDS_UPGRADES;
			break;

		default:
			return;
	}

	rocketInfo.data.selectedArmouryBuyItem[ tblIndex ] = 0;
	rocketInfo.data.armouryBuyListCount[ tblIndex ] = 0;
}

static Str::StringRef WeaponAvailability( int weapon )
{
	playerState_t *ps = &cg.snap->ps;
	int credits = ps->persistant[ PERS_CREDIT ];
	weapon_t currentweapon = BG_PrimaryWeapon( ps->stats );
	credits += BG_Weapon( currentweapon )->price;

	if ( BG_InventoryContainsWeapon( weapon, cg.predictedPlayerState.stats ) )
		return "active";

	if ( !BG_WeaponUnlocked( weapon ) || BG_WeaponDisabled( weapon ) )
		return "locked";

	if ( BG_Weapon( weapon )->price > credits )
		return "expensive";

	return "available";
}

static const char* WeaponDamage( weapon_t weapon )
{
	switch( weapon )
	{
		case WP_HBUILD: return "0";
		case WP_MACHINEGUN: return "10";
		case WP_PAIN_SAW: return "90";
		case WP_SHOTGUN: return "40";
		case WP_LAS_GUN: return "30";
		case WP_MASS_DRIVER: return "50";
		case WP_CHAINGUN: return "60";
		case WP_FLAMER: return "70";
		case WP_PULSE_RIFLE: return "70";
		case WP_LUCIFER_CANNON: return "100";
		default: return "0";
	}
}

static const char* WeaponRange( weapon_t weapon )
{
	switch( weapon )
	{
		case WP_HBUILD: return "0";
		case WP_MACHINEGUN: return "75";
		case WP_PAIN_SAW: return "10";
		case WP_SHOTGUN: return "30";
		case WP_LAS_GUN: return "100";
		case WP_MASS_DRIVER: return "100";
		case WP_CHAINGUN: return "50";
		case WP_FLAMER: return "25";
		case WP_PULSE_RIFLE: return "80";
		case WP_LUCIFER_CANNON: return "75";
		default: return "0";
	}
}

static const char* WeaponRateOfFire( weapon_t weapon )
{
	switch( weapon )
	{
		case WP_HBUILD: return "0";
		case WP_MACHINEGUN: return "70";
		case WP_PAIN_SAW: return "100";
		case WP_SHOTGUN: return "100";
		case WP_LAS_GUN: return "40";
		case WP_MASS_DRIVER: return "20";
		case WP_CHAINGUN: return "80";
		case WP_FLAMER: return "70";
		case WP_PULSE_RIFLE: return "70";
		case WP_LUCIFER_CANNON: return "10";
		default: return "0";
	}
}

static void AddWeaponToBuyList( int i, const char *table, int tblIndex )
{
	static char buf[ MAX_STRING_CHARS ];

	buf[ 0 ] = '\0';

	if ( BG_Weapon( i )->team == TEAM_HUMANS && BG_Weapon( i )->purchasable &&
	        i != WP_BLASTER )
	{
		Info_SetValueForKey( buf, "num", va( "%d", i ), false );
		Info_SetValueForKey( buf, "name", BG_Weapon( i )->humanName, false );
		Info_SetValueForKey( buf, "price", va( "%d", BG_Weapon( i )->price ), false );
		Info_SetValueForKey( buf, "description", BG_Weapon( i )->info, false );
		Info_SetValueForKey( buf, "icon", CG_GetShaderNameFromHandle( cg_weapons[ i ].ammoIcon ), false );
		Info_SetValueForKey( buf, "availability", WeaponAvailability( i ).c_str(), false );
		Info_SetValueForKey( buf, "cmdName", BG_Weapon( i )->name, false );
		Info_SetValueForKey( buf, "damage", WeaponDamage( weapon_t(i) ), false );
		Info_SetValueForKey( buf, "rate", WeaponRateOfFire( weapon_t(i) ), false );
		Info_SetValueForKey( buf, "range", WeaponRange( weapon_t(i) ), false );

		Rocket_DSAddRow( "armouryBuyList", table, buf );

		rocketInfo.data.armouryBuyList[ tblIndex ][ rocketInfo.data.armouryBuyListCount[ tblIndex ]++ ] = i;
	}
}

static bool CanAffordUpgrade( upgrade_t upgrade, int stats[] )
{
	playerState_t *ps = &cg.snap->ps;
	int credits = ps->persistant[ PERS_CREDIT ];

	const int slots = BG_Upgrade( upgrade )->slots;

	for ( int i = UP_NONE; i < UP_NUM_UPGRADES; i++ )
	{
		bool usesTheSameSlot = slots & BG_Upgrade( i )->slots;
		if ( BG_InventoryContainsUpgrade( i, stats ) && usesTheSameSlot )
		{
			// conflicting item that will be replaced, add it to funds
			credits += BG_Upgrade( i )->price;
		}
	}

	return BG_Upgrade( upgrade )->price <= credits;
}

static Str::StringRef UpgradeAvailability( upgrade_t upgrade )
{
	if ( BG_InventoryContainsUpgrade( upgrade, cg.snap->ps.stats ) )
		return "active";

	if ( !BG_UpgradeUnlocked( upgrade ) || BG_UpgradeDisabled( upgrade ) )
		return "locked";

	if ( !CanAffordUpgrade( upgrade, cg.snap->ps.stats ) )
		return "expensive";

	return "available";
}

static void AddUpgradeToBuyList( int i, const char *table, int tblIndex )
{
	static char buf[ MAX_STRING_CHARS ];

	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
	{
		return;
	}


	buf[ 0 ] = '\0';

	if ( BG_Upgrade( i )->team == TEAM_HUMANS && BG_Upgrade( i )->purchasable &&
	        i != UP_MEDKIT )
	{
		Info_SetValueForKey( buf, "num", va( "%d", i ), false );
		Info_SetValueForKey( buf, "name", BG_Upgrade( i )->humanName, false );
		Info_SetValueForKey( buf, "price", va( "%d", BG_Upgrade( i )->price ), false );
		Info_SetValueForKey( buf, "description", BG_Upgrade( i )->info, false );
		Info_SetValueForKey( buf, "availability", UpgradeAvailability( upgrade_t(i) ).c_str(), false );
		Info_SetValueForKey( buf, "cmdName", BG_Upgrade( i )->name, false );
		Info_SetValueForKey( buf, "icon", CG_GetShaderNameFromHandle( cg_upgrades[ i ].upgradeIcon ), false );

		Rocket_DSAddRow( "armouryBuyList", table, buf );

		rocketInfo.data.armouryBuyList[ tblIndex ][ rocketInfo.data.armouryBuyListCount[ tblIndex ]++ ] = i + WP_NUM_WEAPONS;


	}
}

static void CG_Rocket_BuildArmouryBuyList( const char *table )
{
	int tblIndex = -1;

	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
	{
		return;
	}

	// Only bother checking first letter
	if ( *table == 'w' || *table == 'W' )
	{
		tblIndex = ROCKETDS_WEAPONS;
	}

	if ( *table == 'u' || *table == 'U' )
	{
		tblIndex = ROCKETDS_UPGRADES;
	}

	if ( tblIndex == -1 )
	{
		Log::Warn( "unknown \"table\" name provided: \"%s\". Must either start with 'u' (for upgrades) or 'w' (for weapons).", table ? table : "[empty string]" );
		return;
	}

	if ( tblIndex == ROCKETDS_WEAPONS )
	{
		CG_Rocket_CleanUpArmouryBuyList( "weapons" );
		Rocket_DSClearTable( "armouryBuyList", "weapons" );
	}

	if ( tblIndex == ROCKETDS_UPGRADES )
	{
		CG_Rocket_CleanUpArmouryBuyList( "upgrades" );
		Rocket_DSClearTable( "armouryBuyList", "upgrades" );
	}


	if ( tblIndex == ROCKETDS_WEAPONS )
	{
		// Make ckit first so that it can be accessed with a low number key in circlemenus
		AddWeaponToBuyList( WP_HBUILD, "weapons", ROCKETDS_WEAPONS );

		for ( int i = 0; i <= WP_NUM_WEAPONS; ++i )
		{
			if ( i == WP_HBUILD ) continue;

			AddWeaponToBuyList( i, "weapons", ROCKETDS_WEAPONS );
		}
	}

	if ( tblIndex == ROCKETDS_UPGRADES )
	{
		for ( int i = UP_NONE + 1; i < UP_NUM_UPGRADES; ++i )
		{
			AddUpgradeToBuyList( i, "upgrades", ROCKETDS_UPGRADES );
		}
	}

}

static Str::StringRef EvolveAvailability( class_t alienClass )
{
	evolveInfo_t info = BG_ClassEvolveInfoFromTo( cg.predictedPlayerState.stats[ STAT_CLASS ], alienClass );

	if ( cg.predictedPlayerState.stats[ STAT_CLASS ] == alienClass )
		return "active";

	if ( !BG_ClassUnlocked( alienClass ) || BG_ClassDisabled( alienClass ) )
		return "locked";

	if ( cg.predictedPlayerState.persistant[ PERS_CREDIT ] < info.evolveCost )
		return "expensive";

	if ( info.isDevolving && CG_DistanceToBase() > cgs.devolveMaxBaseDistance )
		return "overmindfar";

	return "available";
}

static void CG_Rocket_BuildAlienEvolveList( const char *table )
{
	static char buf[ MAX_STRING_CHARS ];

	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
	{
		return;
	}

	if ( !Q_stricmp( table, "default" ) )
	{
		int i;
		float price;

		Rocket_DSClearTable( "alienEvolveList", "default" );

		for ( i = 0; i < PCL_NUM_CLASSES; ++i )
		{
			// We are building the list for alien evolutions
			if ( BG_Class( i )->team != TEAM_ALIENS )
			{
				continue;
			}

			buf[ 0 ] = '\0';
			price = static_cast<float>( BG_ClassEvolveInfoFromTo(
						cg.predictedPlayerState.stats[ STAT_CLASS ], i ).evolveCost );
			if( price < 0.0f ){
				price *= (( float ) cg.predictedPlayerState.stats[ STAT_HEALTH ] / ( float ) BG_Class( cg.predictedPlayerState.stats[ STAT_CLASS ] )->health ) * DEVOLVE_RETURN_FRACTION;
			}
			Info_SetValueForKey( buf, "num", va( "%d", i ), false );
			Info_SetValueForKey( buf, "name", BG_ClassModelConfig( i )->humanName, false );
			Info_SetValueForKey( buf, "description", BG_Class( i )->info, false );
			Info_SetValueForKey( buf, "availability", EvolveAvailability( class_t(i) ).c_str(), false );
			Info_SetValueForKey( buf, "icon", BG_Class( i )->icon, false );
			Info_SetValueForKey( buf, "cmdName", BG_Class( i )->name, false );
			if (price >= 0.0f) {
				Info_SetValueForKey( buf, "price", va( "Price: %.1f", price / CREDITS_PER_EVO ), false );
			}
			else
			{
				Info_SetValueForKey( buf, "price", va( "Returned: %.1f", -price / CREDITS_PER_EVO ), false );
			}
			bool doublegranger = ( i == PCL_ALIEN_BUILDER0 && BG_ClassUnlocked( PCL_ALIEN_BUILDER0_UPG ) && !BG_ClassDisabled( PCL_ALIEN_BUILDER0_UPG ) )
				|| ( i == PCL_ALIEN_BUILDER0_UPG && ( !BG_ClassUnlocked( PCL_ALIEN_BUILDER0_UPG ) ) );
			Info_SetValueForKey( buf, "visible", (doublegranger ? "false" : "true"), false );

			Rocket_DSAddRow( "alienEvolveList", "default", buf );
		}
	}
}

static Str::StringRef BuildableAvailability( buildable_t buildable )
{
	int spentBudget     = cg.snap->ps.persistant[ PERS_SPENTBUDGET ];
	int markedBudget    = cg.snap->ps.persistant[ PERS_MARKEDBUDGET ];
	int totalBudet      = cg.snap->ps.persistant[ PERS_TOTALBUDGET ];
	int queuedBudget    = cg.snap->ps.persistant[ PERS_QUEUEDBUDGET ];
	int availableBudget = std::max( 0, totalBudet - ((spentBudget - markedBudget) + queuedBudget));

	if ( BG_BuildableDisabled( buildable ) || !BG_BuildableUnlocked( buildable ) )
		return "locked";

	if ( BG_Buildable( buildable )->buildPoints > availableBudget )
		return "expensive";

	return "available";
}

static void CG_Rocket_BuildGenericBuildList( const char *table, team_t team, char const* tableName )
{
	static char buf[ MAX_STRING_CHARS ];

	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
	{
		return;
	}

	if ( !Q_stricmp( table, "default" ) )
	{
		int i;

		Rocket_DSClearTable( tableName, "default" );
		for ( i = BA_NONE + 1; i < BA_NUM_BUILDABLES; ++i )
		{
			// We are building the buildable list
			if ( BG_Buildable( i )->team != team )
			{
				continue;
			}

			buf[ 0 ] = '\0';

			Info_SetValueForKey( buf, "num", va( "%d", i ), false );
			Info_SetValueForKey( buf, "name", BG_Buildable( i )->humanName, false );
			Info_SetValueForKey( buf, "cost", va( "%d", BG_Buildable( i )->buildPoints ), false );
			Info_SetValueForKey( buf, "description", BG_Buildable( i )->info, false );
			Info_SetValueForKey( buf, "icon", BG_Buildable( i )->icon, false );
			Info_SetValueForKey( buf, "cmdName", BG_Buildable( i )->name, false );
			Info_SetValueForKey( buf, "availability", BuildableAvailability( buildable_t(i) ).c_str(), false );

			Rocket_DSAddRow( tableName, "default", buf );
		}
	}
}

static void CG_Rocket_BuildHumanBuildList( const char *table )
{
	CG_Rocket_BuildGenericBuildList( table, TEAM_HUMANS, "humanBuildList" );
}

static void CG_Rocket_BuildAlienBuildList( const char *table )
{
	CG_Rocket_BuildGenericBuildList( table, TEAM_ALIENS, "alienBuildList" );
}

//////// beacon shit


static void CG_Rocket_BuildBeaconList( const char *table )
{
	static char buf[ MAX_STRING_CHARS ];

	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
	{
		return;
	}

	if ( !Q_stricmp( table, "default" ) )
	{
		int i;
		const beaconAttributes_t *ba;

		Rocket_DSClearTable( "beaconList", "default" );

		for ( i = BCT_NONE + 1; i < NUM_BEACON_TYPES; i++ )
		{
			ba = BG_Beacon( i );

			if( ba->flags & BCF_RESERVED )
				continue;

			buf[ 0 ] = '\0';

			Info_SetValueForKey( buf, "num", va( "%d", i ), false );
			Info_SetValueForKey( buf, "name", ba->humanName, false );
			Info_SetValueForKey( buf, "desc", ba->desc, false );
			Info_SetValueForKey( buf, "icon", CG_GetShaderNameFromHandle( ba->icon[ 0 ][ 0 ] ), false );

			Rocket_DSAddRow( "beaconList", "default", buf );
		}
	}
}


static void nullSortFunc( const char*, const char* )
{
}

static void nullExecFunc( const char* )
{
}

static void nullFilterFunc( const char*, const char* )
{
}

static int nullGetFunc( const char* )
{
	return -1;
}

static void nullSetFunc( const char*, int )
{
}

struct dataSourceCmd_t
{
	const char *name;
	void ( *build )( const char *args );
	void ( *sort )( const char *name, const char *sortBy );
	void ( *cleanup )( const char *table );
	void ( *set )( const char *table, int index );
	void ( *filter )( const char *table, const char *filter );
	void ( *exec )( const char *table );
	int  ( *get )( const char *table );
};

static void nullCleanFunc( char const* )
{
}

static const dataSourceCmd_t dataSourceCmdList[] =
{
	{ "alienBuildList", &CG_Rocket_BuildAlienBuildList, &nullSortFunc, &nullCleanFunc, &nullSetFunc, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "alienEvolveList", &CG_Rocket_BuildAlienEvolveList, &nullSortFunc, &nullCleanFunc, &nullSetFunc, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "alOutputs", &CG_Rocket_BuildAlOutputs, &nullSortFunc, &CG_Rocket_CleanUpAlOutputs, &CG_Rocket_SetAlOutputsOutput, &nullFilterFunc, &nullExecFunc, &CG_Rocket_GetAlOutputIndex },
	{ "armouryBuyList", &CG_Rocket_BuildArmouryBuyList, &nullSortFunc, &CG_Rocket_CleanUpArmouryBuyList, &nullSetFunc, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "beaconList", &CG_Rocket_BuildBeaconList, &nullSortFunc, &nullCleanFunc, &nullSetFunc, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "demoList", &CG_Rocket_BuildDemoList, &nullSortFunc, &CG_Rocket_CleanUpDemoList, &CG_Rocket_SetDemoListDemo, &nullFilterFunc, &CG_Rocket_ExecDemoList, &nullGetFunc },
	{ "humanBuildList", &CG_Rocket_BuildHumanBuildList, &nullSortFunc, &nullCleanFunc, &nullSetFunc, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "mapList", &CG_Rocket_BuildMapList, &nullSortFunc, &CG_Rocket_CleanUpMapList, &CG_Rocket_SetMapListIndex, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "modList", &CG_Rocket_BuildModList, &nullSortFunc, &CG_Rocket_CleanUpModList, &CG_Rocket_SetModListMod, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "playerList", &CG_Rocket_BuildPlayerList, &CG_Rocket_SortPlayerList, &CG_Rocket_CleanUpPlayerList, &CG_Rocket_SetPlayerListPlayer, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "resolutions", &CG_Rocket_BuildResolutionList, &nullSortFunc, &CG_Rocket_CleanUpResolutionList, &CG_Rocket_SetResolutionListResolution, &nullFilterFunc, &nullExecFunc, &CG_Rocket_GetResolutionListIndex},
	{ "server_browser", &CG_Rocket_BuildServerList, &CG_Rocket_SortServerList, &CG_Rocket_CleanUpServerList, &CG_Rocket_SetServerListServer, &CG_Rocket_FilterServerList, &CG_Rocket_ExecServerList, &nullGetFunc },
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

	cmd = ( dataSourceCmd_t * ) bsearch( dataSrc, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd )
	{
		cmd->cleanup( table );
		cmd->build( table );
	}
}

void CG_Rocket_SortDataSource( const char *dataSource, const char *name, const char *sortBy )
{
	dataSourceCmd_t *cmd;

	cmd = ( dataSourceCmd_t * ) bsearch( dataSource, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd && cmd->sort )
	{
		cmd->sort( name, sortBy );
	}
}

void CG_Rocket_SetDataSourceIndex( const char *dataSource, const char *table, int index )
{
	dataSourceCmd_t *cmd;

	cmd = ( dataSourceCmd_t * ) bsearch( dataSource, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd && cmd->set )
	{
		cmd->set( table, index );
	}
}

void CG_Rocket_FilterDataSource( const char *dataSource, const char *table, const char *filter )
{
	dataSourceCmd_t *cmd;

	cmd = ( dataSourceCmd_t * ) bsearch( dataSource, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd && cmd->filter )
	{
		cmd->filter( table, filter );
	}
}

void CG_Rocket_ExecDataSource( const char *dataSource, const char *table )
{
	dataSourceCmd_t *cmd;

	cmd = ( dataSourceCmd_t * ) bsearch( dataSource, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd && cmd->exec )
	{
		cmd->exec( table );
	}
}

int CG_Rocket_GetDataSourceIndex( const char *dataSource, const char *table )
{
	dataSourceCmd_t *cmd;

	cmd = ( dataSourceCmd_t * ) bsearch( dataSource, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd && cmd->get )
	{
		return cmd->get( table );
	}

	return -1;
}

void CG_Rocket_RegisterDataSources()
{
	for ( unsigned i = 0; i < dataSourceCmdListCount; ++i )
	{
		// Check that the commands are in increasing order so that it can be used by bsearch
		if ( i != 0 && Q_stricmp( dataSourceCmdList[ i - 1 ].name, dataSourceCmdList[ i ].name ) > 0 )
		{
			Log::Warn( "CGame dataSourceCmdList is in the wrong order for %s and %s", dataSourceCmdList[i - 1].name, dataSourceCmdList[ i ].name );
		}

		Rocket_RegisterDataSource( dataSourceCmdList[ i ].name );
	}
}

void CG_Rocket_CleanUpDataSources()
{
	for ( unsigned i = 0; i < dataSourceCmdListCount; ++i )
	{
		dataSourceCmdList[ i ].cleanup( nullptr );
	}
}
