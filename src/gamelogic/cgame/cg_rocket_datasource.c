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
#include "cg_rocket_datasource.h"

static void AddToServerList( char *name, int clients, int bots, int ping )
{
	server_t *node;

	if ( serverCount == MAX_SERVERS )
	{
		return;
	}

	node = &servers[ serverCount ];

	node->name = BG_strdup( name );
	node->clients = clients;
	node->bots = bots;
	node->ping = ping;
	serverCount++;
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

				AddToServerList( Info_ValueForKey( info, "hostname" ), clients, bots, ping );
			}
		}

		for ( i = 0; i < serverCount; ++i )
		{
			if ( servers[ i ].ping <= 0 )
			{
				continue;
			}

			Info_SetValueForKey( data, "name", servers[ i ].name, qfalse );
			Info_SetValueForKey( data, "players", va( "%d", servers[ i ].clients ), qfalse );
			Info_SetValueForKey( data, "bots", va( "%d", servers[ i ].bots ), qfalse );
			Info_SetValueForKey( data, "ping", va( "%d", servers[ i ].ping ), qfalse );

			trap_Rocket_DSAddRow( "server_browser", args, data );
		}
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
	server_t *server;
	char data[ MAX_INFO_STRING ] = { 0 };
	int  i;

	if ( !Q_stricmp( sortBy, "ping" ) )
	{
		qsort( servers, serverCount, sizeof( server_t ), &ServerListCmpByPing );
	}

	trap_Rocket_DSClearTable( "server_browser", name );

	for ( i = 0; i < serverCount; ++i, server = &servers[ i ] )
	{
		if ( server->ping <= 0 )
		{
			continue;
		}

		Info_SetValueForKey( data, "name", server->name, qfalse );
		Info_SetValueForKey( data, "players", va( "%d", server->clients ), qfalse );
		Info_SetValueForKey( data, "bots", va( "%d", server->bots ), qfalse );
		Info_SetValueForKey( data, "ping", va( "%d", server->ping ), qfalse );

		trap_Rocket_DSAddRow( "server_browser", name, data );
	}
}

void CG_Rocket_CleanUpServerList( void )
{
	int i;

	for ( i = 0; i < serverCount; ++i )
	{
		BG_Free( servers[ i ].name );
	}

	serverCount = 0;
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

	if ( resolutionCount == MAX_RESOLUTIONS )
	{
		return;
	}

	node = &resolutions[ resolutionCount ];

	node->width = w;
	node->height = h;
	resolutionCount++;
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
	resolutionCount = 0;

	while ( Parse( &p, &out ) )
	{

		sscanf( out, "%dx%d", &w, &h );
		AddToResolutionList( w, h );
		BG_Free( out );
	}

	buf[ 0 ] = '\0';

	trap_Rocket_DSClearTable( "resolutions", "default" );

	for ( i = 0; i < resolutionCount; ++i )
	{
		Info_SetValueForKey( buf, "width", va( "%d", resolutions[ i ].width ), qfalse );
		Info_SetValueForKey( buf, "height", va( "%d", resolutions[ i ].height ), qfalse );

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
		qsort( resolutions, resolutionCount, sizeof( resolution_t ), &ResolutionListCmpByWidth );
	}

	trap_Rocket_DSClearTable( "resolutions", "default" );

	for ( i = 0; i < resolutionCount; ++i )
	{
		Info_SetValueForKey( buf, "width", va( "%d", resolutions[ i ].width ), qfalse );
		Info_SetValueForKey( buf, "height", va( "%d", resolutions[ i ].height ), qfalse );

		trap_Rocket_DSAddRow( "resolutions", "default", buf );
	}
}


void CG_Rocket_CleanUpResolutionList( void )
{
	resolutionCount = 0;
}

static void AddToLanguageList( char *name, char *lang )
{
	language_t *node;

	if ( languageCount == MAX_LANGUAGES )
	{
		return;
	}

	node = &languages[ languageCount ];

	node->name = name;
	node->lang = lang;
	languageCount++;
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
			languages[ lang++ ].lang = BG_strdup( temp );
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

	for ( index = 0; index < languageCount; ++index )
	{
		Info_SetValueForKey( buf, "name", languages[ index ].name, qfalse );
		Info_SetValueForKey( buf, "lang", languages[ index ].lang, qfalse );

		trap_Rocket_DSAddRow( "language", "default", buf );
	}
}

void CG_Rocket_CleanUpLanguageList( void )
{
	int i;

	for ( i = 0; i < languageCount; ++i )
	{
		BG_Free( languages[ i ].lang );
		BG_Free( languages[ i ].name );
	}

	languageCount = 0;
}

static void AddToVoipInputs( char *name )
{
	if ( voipInputsCount == MAX_INPUTS )
	{
		return;
	}

	voipInputs[ voipInputsCount++ ] = name;
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
	for ( inputs = 0; inputs < voipInputsCount; ++inputs )
	{
		Info_SetValueForKey( buf, "name", voipInputs[ inputs ], qfalse );

		trap_Rocket_DSAddRow( "voipInputs", "default", buf );
	}
}

void CG_Rocket_CleanUpVoIPInputs( void )
{
	int i;

	for ( i = 0; i < voipInputsCount; ++i )
	{
		BG_Free( voipInputs[ i ] );
	}

	voipInputsCount = 0;
}

static void AddToAlOutputs( char *name )
{
	if ( alOutputsCount == MAX_OUTPUTS )
	{
		return;
	}

	alOutputs[ alOutputsCount++ ] = name;
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

	for ( outputs = 0; outputs < alOutputsCount; ++outputs )
	{
		Info_SetValueForKey( buf, "name", alOutputs[ outputs ], qfalse );

		trap_Rocket_DSAddRow( "alOutputs", "default", buf );
	}
}

void CG_Rocket_CleanUpAlOutputs( void )
{
	int i;

	for ( i = 0; i < alOutputsCount; ++i )
	{
		BG_Free( alOutputs[ i ] );
	}

	alOutputsCount = 0;
}

static void nullSortFunc( const char *name, const char *sortBy )
{
}

typedef struct
{
	const char *name;
	void ( *build ) ( const char *args );
	void ( *sort ) ( const char *name, const char *sortBy );
	void ( *cleanup ) ( void );
} dataSourceCmd_t;

static const dataSourceCmd_t dataSourceCmdList[] =
{
	{ "alOutputs", &CG_Rocket_BuildAlOutputs, &nullSortFunc, &CG_Rocket_CleanUpAlOutputs },
	{ "languages", &CG_Rocket_BuildLanguageList, &nullSortFunc, &CG_Rocket_CleanUpLanguageList },
	{ "resolutions", &CG_Rocket_BuildResolutionList, &CG_Rocket_SortResolutionList, &CG_Rocket_CleanUpResolutionList },
	{ "server_browser", &CG_Rocket_BuildServerList, &CG_Rocket_SortServerList, &CG_Rocket_CleanUpServerList },
	{ "voipInputs", &CG_Rocket_BuildVoIPInputs, &nullSortFunc, &CG_Rocket_CleanUpVoIPInputs },

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

void CG_Rocket_SortDataSource( const char *dataSource, const char *name, const char *sortBy )
{
	dataSourceCmd_t *cmd;

	cmd = bsearch( dataSource, dataSourceCmdList, dataSourceCmdListCount, sizeof( dataSourceCmd_t ), dataSourceCmdCmp );

	if ( cmd && cmd->sort )
	{
		cmd->sort( name, sortBy );
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

