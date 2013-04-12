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
			int ping, bots, clients;

			Com_Memset( &data, 0, sizeof( data ) );

			if ( !trap_LAN_ServerIsVisible( CG_StringToNetSource( args ), i ) )
			{
				continue;
			}

			ping = trap_LAN_GetServerPing( CG_StringToNetSource( args ), i );

			if ( qtrue || !Q_stricmp( args, "favorites" ) )
			{
				trap_LAN_GetServerInfo( CG_StringToNetSource( args ), i, info, MAX_INFO_STRING );

				bots = atoi( Info_ValueForKey( info, "bots" ) );
				clients = atoi( Info_ValueForKey( info, "clients" ) );

				Info_SetValueForKey( data, "name", Info_ValueForKey( info, "hostname" ), qfalse );
				Info_SetValueForKey( data, "players", va( "%d + (%d)", clients, bots ), qfalse );
				Info_SetValueForKey( data, "ping", va( "%d", ping ), qfalse );

				if ( ping > 0 )
				{
					trap_Rocket_DSAddRow( "server_browser", args, data );
				}
			}
		}
	}

	rocketInfo.serversLastRefresh = trap_Milliseconds();
}

static void CG_Rocket_SortServerList( const char *sortBy )
{
	//TODO: Sort data
}

typedef struct
{
	const char *name;
	void ( *build ) ( const char *args );
	void ( *sort ) ( const char *sortBy );
} dataSourceCmd_t;

static const dataSourceCmd_t dataSourceCmdList[] =
{
	{ "server_browser", &CG_Rocket_BuildServerList, &CG_Rocket_SortServerList }
};

static const size_t dataSourceCmdListCount = ARRAY_LEN( dataSourceCmdList );

static int dataSourceCmdCmp( const void *a, const void *b )
{
	return Q_stricmp( ( const char * ) a, ( ( dataSourceCmd_t * ) b )->name );
}

void CG_Rocket_BuildDataSource( const char *data )
{
	dataSourceCmd_t *cmd;
	char *tail, *head, *args;

	head = BG_strdup( data );

	// No data
	if ( !*head )
	{
		return;

	}

	tail = strchr( head, ' ' );
	if ( tail )
	{
		*tail = '\0';
		args = head + strlen( head ) + 1;
	}
	else
	{
		args = NULL;
	}

	cmd = bsearch( head, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd )
	{
		cmd->build( args );
	}

	BG_Free( head );
}

void CG_Rocket_SortDataSource( const char *dataSource, const char *sortBy )
{
	dataSourceCmd_t *cmd;

	cmd = bsearch( dataSource, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd && cmd->sort )
	{
		cmd->sort( sortBy );
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

