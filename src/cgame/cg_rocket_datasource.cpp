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

static bool AddToServerList( const char *name, const char *label, std::string version, std::string abiVersion, int clients, int bots,
	int ping, int maxClients, char *mapName, char *addr, int netSrc )
{
	if ( !*name || !*mapName )
	{
		return false;
	}

	server_t node;
	node.name = BG_strdup( name );
	node.clients = clients;
	node.bots = bots;
	node.ping = ping;
	node.maxClients = maxClients;
	node.addr = BG_strdup( addr );
	node.label = BG_strdup( label );
	node.version = version;
	node.mapName = BG_strdup( mapName );
	node.abiVersion = abiVersion;

	rocketInfo.data.servers[netSrc].push_back( node );
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

void CG_Rocket_BuildServerInfo()
{
	int netSrc = rocketInfo.currentNetSrc;

	int serverIndex = rocketInfo.data.serverIndex[ netSrc ];

	if ( serverIndex >= rocketInfo.data.servers->size() || serverIndex < 0 )
	{
		return;
	}

	if ( rocketInfo.realtime < rocketInfo.serverStatusLastRefresh + 500 )
	{
		return;
	}

	rocketInfo.serverStatusLastRefresh = rocketInfo.realtime;

	if ( !rocketInfo.data.buildingServerInfo )
	{
		rocketInfo.data.buildingServerInfo = true;
	}

	server_t* server = &rocketInfo.data.servers[ netSrc ][ serverIndex ];

	// FIXME: This was already broken and might need to be redone entirely
	std::string serverInfoText;
	if ( trap_LAN_ServerStatus( server->addr, serverInfoText ) )
	{
		Rocket_DSClearTable( "server_browser", "serverInfo" );
		Rocket_DSClearTable( "server_browser", "serverPlayers" );

		InfoMap info = InfoStringToMap( serverInfoText );

		for ( const std::pair<std::string, std::string>& pair : info )
		{
			const std::string buf = "\\cvar\\" + pair.first + "\\value\\" + pair.second;
			Rocket_DSAddRow( "server_browser", "serverInfo", buf.c_str() );
		}

		int i = 0;
		for( const std::pair<std::string, std::string>& pair : info )
		{
			int score;
			int ping;
			sscanf( pair.first.c_str(), "%d %d", &score, &ping);

			uint32_t start = pair.first.find( "\"" );
			uint32_t end   = pair.first.find( "\"", start );

			if ( start == std::string::npos || end == std::string::npos ) {
				continue;
			}

			info["num"] = std::to_string( i );
			i++;
			info["name"] = pair.first.substr( start + 1, end - start - 1 );

			const uint32_t offset = pair.first.find( " " );
			info["score"] = pair.first.substr( 0, offset );
			info["ping"] = pair.first.substr( offset + 1, pair.first.find( " ", offset ) );

			Rocket_DSAddRow( "server_browser", "serverPlayers", InfoMapToString( info ).c_str() );
		}

		trap_LAN_ResetServerStatus();
		rocketInfo.data.buildingServerInfo = false;
	}
}

static void CG_Rocket_BuildServerList( const char *args )
{
	rocketInfo.currentNetSrc = CG_StringToNetSource( args );
	Rocket_DSClearTable( "server_browser", args );
	rocketInfo.data.servers->clear();
	rocketInfo.data.haveServerInfo->clear();
	CG_Rocket_BuildServerList();
}

void CG_Rocket_BuildServerList()
{
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

	int oldServerCount = rocketInfo.data.servers->size();

	for ( int i = 0; i < numServers; ++i )
	{
		if ( rocketInfo.data.haveServerInfo[ netSrc ][ i ] )
		{
			continue;
		}

		int ping, bots, clients, maxClients;

		if ( !trap_LAN_ServerIsVisible( netSrc, i ) )
		{
			continue;
		}

		ping = trap_LAN_GetServerPing( netSrc, i );

		if ( ping > 0 )
		{
			char mapname[ 256 ];
			trustedServerInfo_t trustedInfo;
			std::string info;
			trap_LAN_GetServerInfo( netSrc, i, trustedInfo, info );

			bots = atoi( Info_ValueForKey( info.c_str(), "bots"));
			clients = atoi( Info_ValueForKey( info.c_str(), "clients" ) );
			maxClients = atoi( Info_ValueForKey( info.c_str(), "sv_maxclients" ) );
			Q_strncpyz( mapname, Info_ValueForKey( info.c_str(), "mapname" ), sizeof( mapname ) );

			const std::string version = Info_ValueForKey( info.c_str(), "version" );
			const std::string abiVersion = Info_ValueForKey( info.c_str(), "abiVersion" );
			rocketInfo.data.haveServerInfo[ netSrc ][ i ] =
				AddToServerList( Info_ValueForKey( info.c_str(), "hostname" ), trustedInfo.featuredLabel,
					version, abiVersion, clients, bots, ping, maxClients, mapname, trustedInfo.addr, netSrc );
		}
	}

	for ( int i = oldServerCount; i < rocketInfo.data.servers->size(); ++i )
	{
		if ( rocketInfo.data.servers[ netSrc ][ i ].ping <= 0 )
		{
			continue;
		}

		InfoMap info;
		info["name"] = rocketInfo.data.servers[ netSrc ][ i ].name;
		info["players"] = std::to_string( rocketInfo.data.servers[ netSrc ][ i ].clients );
		info["bots"] = std::to_string( rocketInfo.data.servers[ netSrc ][ i ].bots );
		info["ping"] = std::to_string( rocketInfo.data.servers[ netSrc ][ i ].ping );
		info["maxClients"] = std::to_string( rocketInfo.data.servers[ netSrc ][ i ].maxClients );
		info["addr"] = rocketInfo.data.servers[ netSrc ][ i ].addr;
		info["label"] = rocketInfo.data.servers[ netSrc ][ i ].label;

		std::string version;
		if ( !Q_stricmp( rocketInfo.data.servers[netSrc][i].abiVersion.c_str(), IPC::SYSCALL_ABI_VERSION) ) {
			version = rocketInfo.data.servers[netSrc][i].version;
		} else {
			version = Str::Format( "^1!%s!", rocketInfo.data.servers[netSrc][i].version );
		}

		info["version"] = version;
		info["map"] = rocketInfo.data.servers[ netSrc ][ i ].mapName;

		Rocket_DSAddRow( "server_browser", srcName, InfoMapToString( info ).c_str() );
	}
}

static void CG_Rocket_SortServerList( const char *name, const char *sortBy )
{
	int netSrc = CG_StringToNetSource( name );

	if ( !Q_stricmp( sortBy, "ping" ) )
	{
		std::sort( rocketInfo.data.servers[netSrc].begin(), rocketInfo.data.servers[netSrc].end(),
			[]( const server_t& lhs, const server_t& rhs ) {
				return lhs.ping < rhs.ping;
			} );
	}
	else if ( !Q_stricmp( sortBy, "name" ) )
	{
		std::sort( rocketInfo.data.servers[netSrc].begin(), rocketInfo.data.servers[netSrc].end(),
			[]( const server_t& lhs, const server_t& rhs ) {
				return Color::StripColors( lhs.name ) < Color::StripColors( rhs.name );
			} );
	}
	else if ( !Q_stricmp( sortBy, "players" ) )
	{
		std::sort( rocketInfo.data.servers[netSrc].begin(), rocketInfo.data.servers[netSrc].end(),
			[]( const server_t& lhs, const server_t& rhs ) {
				return lhs.clients < rhs.clients;
			} );
	}
	else if ( !Q_stricmp( sortBy, "map" ) )
	{
		std::sort( rocketInfo.data.servers[netSrc].begin(), rocketInfo.data.servers[netSrc].end(),
			[]( const server_t& lhs, const server_t& rhs ) {
				return lhs.mapName < rhs.mapName;
			} );
	}

	Rocket_DSClearTable( "server_browser", name );

	for ( const server_t& server : rocketInfo.data.servers[netSrc] )
	{
		if ( server.ping <= 0 )
		{
			continue;
		}

		InfoMap info;
		info["name"] = server.name;
		info["players"] = std::to_string( server.clients );
		info["bots"] = std::to_string( server.bots );
		info["ping"] = std::to_string( server.ping );
		info["maxClients"] = std::to_string( server.maxClients );
		info["addr"] = server.addr;
		info["label"] = server.label;
		info["version"] = server.version;
		info["map"] = server.mapName;

		Rocket_DSAddRow( "server_browser", name, InfoMapToString( info ).c_str() );
	}
}

static void CG_Rocket_FilterServerList( const char *table, const char *filter )
{
	const char *str = ( table && *table ) ? table : CG_NetSourceToString( rocketInfo.currentNetSrc );
	int netSrc = CG_StringToNetSource( str );

	Rocket_DSClearTable( "server_browser", str );

	for ( const server_t& server : rocketInfo.data.servers[netSrc] )
	{
		char name[ MAX_INFO_VALUE ];
		Color::StripColors( server.name, name, sizeof( name ) );

		if ( Q_stristr( name, filter ) )
		{
			InfoMap info;
			info["name"] = server.name;
			info["players"] = std::to_string( server.clients );
			info["bots"] = std::to_string( server.bots );
			info["ping"] = std::to_string( server.ping );
			info["maxClients"] = std::to_string( server.maxClients );
			info["addr"] = server.addr;
			info["label"] = server.label;
			info["version"] = server.version;

			Rocket_DSAddRow( "server_browser", str, InfoMapToString( info ).c_str() );
		}
	}
}

static std::string cg_currentSelectedServer;

class ConnectToCurrentSelectedServerCmd : public Cmd::StaticCmd {
	public:
	ConnectToCurrentSelectedServerCmd() : StaticCmd( "connectToCurrentSelectedServer",
		Cmd::CLIENT, "For internal use" ) {}

	void Run( const Cmd::Args& ) const override {
		Rocket_DocumentAction( "server_mismatch", "close" );
		trap_SendConsoleCommand(
			Str::Format( "connect %s", cg_currentSelectedServer ).c_str()
		);
	}
};
static ConnectToCurrentSelectedServerCmd ConnectToCurrentSelectedServerCmdRegistration;

static void CG_Rocket_ExecServerList( const char *table )
{
	int netSrc = CG_StringToNetSource( table );
	const server_t& server = rocketInfo.data.servers[netSrc][rocketInfo.data.serverIndex[netSrc]];
	if ( Q_stricmp( server.abiVersion.c_str(), IPC::SYSCALL_ABI_VERSION ) ) {
		cg_currentSelectedServer = server.addr;
		Rocket_DocumentAction( "server_mismatch", "show" );
	} else {
		trap_SendConsoleCommand( Str::Format( "connect %s", server.addr ).c_str() );
	}
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

	int width = cgs.windowConfig.vidWidth;
	int height = cgs.windowConfig.vidHeight;

	if ( mode == -2 && width == cgs.windowConfig.displayWidth && height == cgs.windowConfig.displayHeight )
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
		InfoMap info;
		info["width"] = std::to_string( res.width );
		info["height"] = std::to_string( res.height );

		Rocket_DSAddRow( "resolutions", "default", InfoMapToString( info ).c_str() );
	}
}

static int CG_Rocket_GetResolutionListIndex( const char* )
{
	return rocketInfo.data.resolutionIndex;
}

static void CG_Rocket_SetAlOutputsOutput( const char*, int index )
{
	std::string device;
	if (index >= 0 && index < rocketInfo.data.alOutputs.size() )
	{
		device = rocketInfo.data.alOutputs[ index ];
	}

	rocketInfo.data.alOutputIndex = index;
	trap_Cvar_Set( "audio.al.device", device.c_str() );
	trap_Cvar_AddFlags( "audio.al.device", CVAR_ARCHIVE );
}

static void CG_Rocket_BuildAlOutputs( const char* )
{
	char buf[ MAX_STRING_CHARS ], currentDevice[ MAX_STRING_CHARS ];
	char *p, *head;

	rocketInfo.data.alOutputs.clear();

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
			rocketInfo.data.alOutputIndex = rocketInfo.data.alOutputs.size();
		}

		rocketInfo.data.alOutputs.push_back( head );

		head = p + 1;
	}

	for ( const std::string& alOutput : rocketInfo.data.alOutputs )
	{
		const std::string row = "\\name\\" + alOutput;

		Rocket_DSAddRow( "alOutputs", "default", row.c_str() );
	}
}

static int CG_Rocket_GetAlOutputIndex( const char* )
{
	return rocketInfo.data.alOutputIndex;
}

static void CG_Rocket_SetModListMod( const char*, int index )
{
	rocketInfo.data.modIndex = index;
}

static void CG_Rocket_BuildModList( const char* )
{
	char dirlist[ 2048 ];
	int numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof( dirlist ) );
	char* dirptr = dirlist;

	rocketInfo.data.modList.clear();

	for ( int i = 0; i < numdirs; i++ )
	{
		int dirlen = strlen( dirptr ) + 1;
		char* descptr = dirptr + dirlen;

		rocketInfo.data.modList.push_back( { dirptr, descptr } );

		dirptr += dirlen + strlen( descptr ) + 1;
	}

	for ( const modInfo_t& mod : rocketInfo.data.modList )
	{
		const std::string row = "\\name\\" + mod.name + "\\descripton\\" + mod.description;

		Rocket_DSAddRow( "modList", "default", row.c_str() );
	}
}

static void CG_Rocket_SetDemoListDemo( const char*, int index )
{
	rocketInfo.data.demoIndex = index;
}

static void CG_Rocket_ExecDemoList( const char* )
{
	trap_SendConsoleCommand( Str::Format( "demo %s", rocketInfo.data.demoList[rocketInfo.data.demoIndex] ).c_str() );
}

static void CG_Rocket_BuildDemoList( const char* )
{
	char  demolist[ 4096 ];
	char  *demoname;

	std::string demoExt = Str::Format( "dm_%d", trap_Cvar_VariableIntegerValue( "protocol" ) );

	rocketInfo.data.demoList.reserve( trap_FS_GetFileList( "demos", demoExt.c_str(), demolist, 4096 ) );

	demoExt = Str::Format( ".dm_%d", trap_Cvar_VariableIntegerValue( "protocol" ) );

	demoname = demolist;
	auto demoExtLen = demoExt.size();

	rocketInfo.data.demoList.clear();

	for ( int i = 0; i < rocketInfo.data.demoList.capacity(); i++ )
	{
		auto len = strlen( demoname );

		if ( !Q_stricmp( demoname + len - demoExtLen, demoExt.c_str() ) )
		{
			demoname[ len - demoExtLen ] = '\0';
		}

		rocketInfo.data.demoList.push_back( demoname );

		demoname += len + 1;
	}

	for ( const std::string& name : rocketInfo.data.demoList )
	{
		const std::string row = "\\name\\" + name;

		Rocket_DSAddRow( "demoList", "default", row.c_str() );
	}
}

void CG_Rocket_BuildPlayerList( const char* )
{
	// Do not build list if not currently playing
	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
	{
		return;
	}

	// Clear old values. Always build all three teams.
	Rocket_DSClearTable( "playerList", "spectators" );
	Rocket_DSClearTable( "playerList", "aliens" );
	Rocket_DSClearTable( "playerList", "humans" );

	rocketInfo.data.playerCount[TEAM_ALIENS] = 0;
	rocketInfo.data.playerIndex[TEAM_HUMANS] = 0;
	rocketInfo.data.playerCount[TEAM_NONE] = 0;

	InfoMap info;
	for ( int i = 0; i < cg.numScores; ++i )
	{
		score_t* score = &cg.scores[ i ];
		clientInfo_t* ci = &cgs.clientinfo[ score->client ];

		if ( !ci->infoValid )
		{
			continue;
		}

		info["num"] = Str::Format( "%d", score->client );
		info["score"] = Str::Format( "%d", score->score );

		const char* B = Info_ValueForKey( CG_ConfigString( CS_SERVERINFO ),"B" );
		const char bot_status = B[ score->client ];

		// HACK: Display bot status in “Ping” column.
		if ( bot_status == '-' )
		{
			// This player is not a bot.
			info["ping"] = Str::Format( "%d", score->ping );
		}
		// Bot skill can be 0 while spawning, just before skill is set.
		else if ( bot_status >= '0' && bot_status <= '9' )
		{
			// Bot skill.
			info["ping"] = Str::Format( "sk%c", bot_status );
		}
		else
		{
			// Example: “G” is for “Generating navmeshes”.
			info["ping"] = Str::Format( "%c", bot_status );
		}

		info["weapon"] = Str::Format( "%d", score->weapon );
		info["upgrade"] = Str::Format( "%d", score->upgrade );
		info["time"] = Str::Format( "%d", score->time );
		info["credits"] = Str::Format( "%d", ci->credit );
		info["location"] = CG_ConfigString( CS_LOCATIONS + ci->location );

		switch ( score->team )
		{
			case TEAM_ALIENS:
				rocketInfo.data.playerList[ score->team ][ rocketInfo.data.playerCount[ TEAM_ALIENS ]++ ] = i;
				Rocket_DSAddRow( "playerList", "aliens", InfoMapToString( info ).c_str() );
				break;

			case TEAM_HUMANS:
				rocketInfo.data.playerList[ score->team ][ rocketInfo.data.playerIndex[ TEAM_HUMANS ]++ ] = i;
				Rocket_DSAddRow( "playerList", "humans", InfoMapToString( info ).c_str() );
				break;

			case TEAM_NONE:
				rocketInfo.data.playerList[ score->team ][ rocketInfo.data.playerCount[ TEAM_NONE ]++ ] = i;
				Rocket_DSAddRow( "playerList", "spectators", InfoMapToString( info ).c_str() );
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

static void AddPlayerListRow( const std::string& name, const std::string& table, const team_t team ) {
	for ( int i = 0; i < rocketInfo.data.playerCount[team]; ++i ) {
		score_t* score = &cg.scores[rocketInfo.data.playerList[team][i]];
		clientInfo_t* ci = &cgs.clientinfo[score->client];

		if ( !ci->infoValid ) {
			continue;
		}

		InfoMap info;
		info["num"] = std::to_string( score->client );
		info["score"] = std::to_string( score->score );
		info["ping"] = std::to_string( score->ping );
		info["weapon"] = std::to_string( score->weapon );
		info["upgrade"] = std::to_string( score->upgrade );
		info["time"] = std::to_string( score->time );
		info["credits"] = std::to_string( ci->credit );
		info["location"] = CG_ConfigString( CS_LOCATIONS + ci->location );

		Rocket_DSAddRow( name.c_str(), table.c_str(), InfoMapToString( info ).c_str() );
	}
}

static void CG_Rocket_SortPlayerList( const char*, const char *sortBy )
{
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

	AddPlayerListRow( "playerList", "spectators", TEAM_NONE );
	AddPlayerListRow( "playerList", "humans",     TEAM_HUMANS );
	AddPlayerListRow( "playerList", "aliens",     TEAM_ALIENS );
}

static void CG_Rocket_BuildMapList( const char* )
{
	Rocket_DSClearTable( "mapList", "default" );
	CG_LoadMapList();

	for ( size_t i = 0; i < rocketInfo.data.mapList.size(); ++i )
	{
		InfoMap info;
		info["num"] = std::to_string( i );
		info["mapName"] = rocketInfo.data.mapList[ i ].mapLoadName;
		info["mapLoadName"] = rocketInfo.data.mapList[ i ].mapLoadName;

		Rocket_DSAddRow( "mapList", "default", InfoMapToString( info ).c_str() );
	}

	rocketInfo.data.mapIndex = -1;
}

static void CG_Rocket_SetMapListIndex( const char*, int index )
{
	rocketInfo.data.mapIndex = index;
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
	if ( BG_Weapon( i )->team == TEAM_HUMANS && BG_Weapon( i )->purchasable &&
	        i != WP_BLASTER )
	{
		InfoMap info;
		info["num"] = std::to_string( i );
		info["name"] = BG_Weapon( i )->humanName;
		info["price"] = std::to_string( BG_Weapon( i )->price );
		info["description"] = BG_Weapon( i )->info;
		info["icon"] = Str::Format( "$handle/%d", cg_weapons[i].ammoIcon );
		info["availability"] = WeaponAvailability( i );
		info["cmdName"] = BG_Weapon( i )->name;
		info["damage"] = WeaponDamage( weapon_t( i ) );
		info["rate"] = WeaponRateOfFire( weapon_t( i ) );
		info["range"] = WeaponRange( weapon_t( i ) );

		Rocket_DSAddRow( "armouryBuyList", table, InfoMapToString( info ).c_str() );

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
	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
	{
		return;
	}

	if ( BG_Upgrade( i )->team == TEAM_HUMANS && BG_Upgrade( i )->purchasable &&
	        i != UP_MEDKIT )
	{
		InfoMap info;
		info["num"] = std::to_string( i );
		info["name"] = BG_Upgrade( i )->humanName;
		info["price"] = std::to_string( BG_Upgrade( i )->price );
		info["description"] = BG_Upgrade( i )->info;
		info["availability"] = UpgradeAvailability( upgrade_t( i ) );
		info["cmdName"] = BG_Upgrade( i )->name;
		info["icon"] = Str::Format( "$handle/%d", cg_upgrades[ i ].upgradeIcon );

		Rocket_DSAddRow( "armouryBuyList", table, InfoMapToString( info ).c_str() );

		rocketInfo.data.armouryBuyList[ tblIndex ][ rocketInfo.data.armouryBuyListCount[ tblIndex ]++ ] = i + WP_NUM_WEAPONS;
	}
}

static void CG_Rocket_BuildArmouryBuyList( const char *table )
{
	CG_Rocket_CleanUpArmouryBuyList( table );

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
	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
	{
		return;
	}

	if ( !Q_stricmp( table, "default" ) )
	{
		Rocket_DSClearTable( "alienEvolveList", "default" );

		for ( int i = 0; i < PCL_NUM_CLASSES; ++i )
		{
			// We are building the list for alien evolutions
			if ( BG_Class( i )->team != TEAM_ALIENS )
			{
				continue;
			}

			float price = static_cast<float>( BG_ClassEvolveInfoFromTo(
						cg.predictedPlayerState.stats[ STAT_CLASS ], i ).evolveCost );
			if( price < 0.0f ){
				price *= (
					( float ) cg.predictedPlayerState.stats[ STAT_HEALTH ]
					/ ( float ) BG_Class( cg.predictedPlayerState.stats[ STAT_CLASS ] )->health
				) * DEVOLVE_RETURN_FRACTION;
			}

			InfoMap info;
			info["num"] = std::to_string( i );
			info["name"] = BG_ClassModelConfig( i )->humanName;
			info["description"] = BG_Class( i )->info;
			info["availability"] = EvolveAvailability( class_t(i) );
			info["icon"] = BG_Class( i )->icon;
			info["cmdName"] = BG_Class( i )->name;
			
			if (price >= 0.0f) {
				info["price"] = Str::Format( "Price: %.1f", price / CREDITS_PER_EVO );
			}
			else
			{
				info["price"] = Str::Format( "Returned: %.1f", -price / CREDITS_PER_EVO );
			}

			const bool doublegranger =
				( i == PCL_ALIEN_BUILDER0
					&& BG_ClassUnlocked( PCL_ALIEN_BUILDER0_UPG ) && !BG_ClassDisabled( PCL_ALIEN_BUILDER0_UPG )
				)
				|| ( i == PCL_ALIEN_BUILDER0_UPG
					&& ( !BG_ClassUnlocked( PCL_ALIEN_BUILDER0_UPG ) )
				);

			info["visible"] = doublegranger ? "false" : "true";

			Rocket_DSAddRow( "alienEvolveList", "default", InfoMapToString( info ).c_str() );
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
	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
	{
		return;
	}

	if ( !Q_stricmp( table, "default" ) )
	{
		Rocket_DSClearTable( tableName, "default" );
		for ( int i = BA_NONE + 1; i < BA_NUM_BUILDABLES; ++i )
		{
			// We are building the buildable list
			if ( BG_Buildable( i )->team != team )
			{
				continue;
			}

			InfoMap info;
			info["num"] = std::to_string( i );
			info["name"] = BG_Buildable( i )->humanName;
			info["cost"] = std::to_string( BG_Buildable( i )->buildPoints );
			info["description"] = BG_Buildable( i )->info;
			info["icon"] = BG_Buildable( i )->icon;
			info["cmdName"] = BG_Buildable( i )->name;
			info["availability"] = BuildableAvailability( buildable_t(i) );

			Rocket_DSAddRow( tableName, "default", InfoMapToString( info ).c_str() );
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
	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
	{
		return;
	}

	if ( !Q_stricmp( table, "default" ) )
	{
		Rocket_DSClearTable( "beaconList", "default" );

		for ( int i = BCT_NONE + 1; i < NUM_BEACON_TYPES; i++ )
		{
			const beaconAttributes_t* ba = BG_Beacon( i );

			if ( ba->flags & BCF_RESERVED ) {
				continue;
			}

			InfoMap info;
			info["num"] = std::to_string( i );
			info["name"] = ba->humanName;
			info["desc"] = ba->desc;
			info["icon"] = Str::Format( "$handle/%d", ba->icon[ 0 ][ 0 ] );

			Rocket_DSAddRow( "beaconList", "default", InfoMapToString( info ).c_str() );
		}
	}
}

static void CG_Rocket_BuildBotTacticList( const char *table )
{
	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
	{
		return;
	}

	if ( !Q_stricmp( table, "default" ) )
	{
		Rocket_DSClearTable( "botTacticList", "default" );

		auto setCommand = [&]( std::string num, std::string name, std::string title, std::string desc, std::string icon )
		{
			InfoMap info;
			info["num"] = num;
			info["name"] = name;
			info["title"] = title;
			info["desc"] = desc;
			info["icon"] = icon;

			Rocket_DSAddRow( "botTacticList", "default", InfoMapToString( info ).c_str() );
		};

		setCommand( "0", "default", N_( "Default" ), N_( "The default behavior. This is what bots do when the game starts." ), "gfx/feedback/bottactic/default" );
		setCommand( "1", "defend", N_( "Defend" ), N_( "The bots stay in the base." ), "gfx/feedback/bottactic/defend" );
		setCommand( "2", "attack", N_( "Attack" ), N_( "The bots attack the enemy base." ), "gfx/feedback/bottactic/attack" );
		setCommand( "3", "stay_here", N_( "Stay Here" ), N_( "The bots stay where you are currently." ), "gfx/feedback/bottactic/stay_here" );
		setCommand( "4", "follow", N_( "Follow" ), N_( "The bots follow you wherever you go." ), "gfx/feedback/bottactic/follow" );
	}
}

static void CG_Rocket_BuildVsayList( const char *table )
{
	if ( rocketInfo.cstate.connState < connstate_t::CA_ACTIVE )
	{
		return;
	}

	if ( !Q_stricmp( table, "default" ) )
	{
		Rocket_DSClearTable( "vsayList", "default" );

		auto setCommand = [&]( std::string num, std::string name, std::string title, std::string desc, std::string icon )
		{
			InfoMap info;
			info["num"] = num;
			info["name"] = name;
			info["title"] = title;
			info["desc"] = desc;
			info["icon"] = icon;

			Rocket_DSAddRow( "vsayList", "default", InfoMapToString( info ).c_str() );
		};

		setCommand( "0", "defend", N_( "Defend" ), N_( "Defend our base!!" ), "gfx/feedback/vsay/defend" );
		setCommand( "2", "attack", N_( "Attack" ), N_( "Attack!" ), "gfx/feedback/vsay/attack" );
		setCommand( "1", "followme", N_( "Follow" ), N_( "Follow me!" ), "gfx/feedback/vsay/follow" );
		setCommand( "3", "no", N_( "No" ), N_( "No." ), "gfx/feedback/vsay/no" );
		setCommand( "4", "yes", N_( "Yes" ), N_( "Yes." ), "gfx/feedback/vsay/yes" );
		setCommand( "5", "flank", N_( "Flank" ), N_( "Flank, behind you!" ), "gfx/feedback/vsay/flank" );
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
	void ( *set )( const char *table, int index );
	void ( *filter )( const char *table, const char *filter );
	void ( *exec )( const char *table );
	int  ( *get )( const char *table );
};

static const dataSourceCmd_t dataSourceCmdList[] =
{
	{ "alienBuildList", &CG_Rocket_BuildAlienBuildList, &nullSortFunc, &nullSetFunc, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "alienEvolveList", &CG_Rocket_BuildAlienEvolveList, &nullSortFunc, &nullSetFunc, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "alOutputs", &CG_Rocket_BuildAlOutputs, &nullSortFunc, &CG_Rocket_SetAlOutputsOutput, &nullFilterFunc, &nullExecFunc, &CG_Rocket_GetAlOutputIndex },
	{ "armouryBuyList", &CG_Rocket_BuildArmouryBuyList, &nullSortFunc, &nullSetFunc, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "beaconList", &CG_Rocket_BuildBeaconList, &nullSortFunc, &nullSetFunc, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "botTacticList", &CG_Rocket_BuildBotTacticList, &nullSortFunc, &nullSetFunc, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "demoList", &CG_Rocket_BuildDemoList, &nullSortFunc, &CG_Rocket_SetDemoListDemo, &nullFilterFunc, &CG_Rocket_ExecDemoList, &nullGetFunc },
	{ "humanBuildList", &CG_Rocket_BuildHumanBuildList, &nullSortFunc, &nullSetFunc, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "mapList", &CG_Rocket_BuildMapList, &nullSortFunc, &CG_Rocket_SetMapListIndex, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "modList", &CG_Rocket_BuildModList, &nullSortFunc, &CG_Rocket_SetModListMod, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "playerList", &CG_Rocket_BuildPlayerList, &CG_Rocket_SortPlayerList, &CG_Rocket_SetPlayerListPlayer, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
	{ "resolutions", &CG_Rocket_BuildResolutionList, &nullSortFunc, &CG_Rocket_SetResolutionListResolution, &nullFilterFunc, &nullExecFunc, &CG_Rocket_GetResolutionListIndex},
	{ "server_browser", &CG_Rocket_BuildServerList, &CG_Rocket_SortServerList, &CG_Rocket_SetServerListServer, &CG_Rocket_FilterServerList, &CG_Rocket_ExecServerList, &nullGetFunc },
	{ "vsayList", &CG_Rocket_BuildVsayList, &nullSortFunc, &nullSetFunc, &nullFilterFunc, &nullExecFunc, &nullGetFunc },
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

	cmd = ( dataSourceCmd_t * )
		bsearch( dataSrc, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd )
	{
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