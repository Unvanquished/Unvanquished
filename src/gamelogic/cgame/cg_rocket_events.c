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


static void CG_Rocket_EventOpen( void )
{
	trap_Rocket_LoadDocument( va( "%s%s.rml", rocketInfo.rootDir, CG_Argv( 1 ) ) );
}

static void CG_Rocket_EventClose( void )
{
	trap_Rocket_DocumentAction( CG_Argv( 1 ), "close" );
}

static void CG_Rocket_EventGoto( void )
{
	trap_Rocket_DocumentAction( CG_Argv( 1 ), "goto" );
}

static void CG_Rocket_EventShow( void )
{
	trap_Rocket_DocumentAction( CG_Argv( 1 ), "show" );
}

static void CG_Rocket_EventBlur( void )
{
	trap_Rocket_DocumentAction( CG_Argv( 1 ), "blur" );
}


static void CG_Rocket_InitServers( void )
{
	const char *src = CG_Argv( 1 );
	trap_LAN_ResetPings( CG_StringToNetSource( src ) );
	trap_LAN_ServerStatus( NULL, NULL, 0 );

	if ( !Q_stricmp( src, "internet" ) )
	{
		trap_Cmd_ExecuteText( EXEC_APPEND, "globalservers 0 86 full empty\n" );
	}

	else if ( !Q_stricmp( src, "local" ) )
	{
		trap_Cmd_ExecuteText( EXEC_APPEND, "localservers\n" );
	}

	trap_LAN_UpdateVisiblePings( CG_StringToNetSource( src ) );
}

static void CG_Rocket_BuildDS( void )
{
	char table[ 100 ];

	Q_strncpyz( table, CG_Argv( 2 ), sizeof( table ) );
	CG_Rocket_BuildDataSource( CG_Argv( 1 ), table );
}



static void CG_Rocket_EventExec( void )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, CG_Args() );
}

static void CG_Rocket_EventCvarForm( void )
{
	static char params[ BIG_INFO_STRING ];
	static char key[BIG_INFO_VALUE], value[ BIG_INFO_VALUE ];
	const char *s;

	trap_Rocket_GetEventParameters( params, 0 );

	if ( !*params )
	{
		return;
	}

	s = params;

	while ( *s )
	{
		Info_NextPair( &s, key, value );
		if ( !Q_strnicmp( "cvar ", key, 5 ) )
		{

			trap_Cvar_Set( key + 5, value );
		}
	}
}

static void CG_Rocket_SortDS( void )
{
	char name[ 100 ], table[ 100 ], sortBy[ 100 ];
	char *p;

	Q_strncpyz( name, CG_Argv( 1 ), sizeof( name ) );
	Q_strncpyz( table, CG_Argv( 2 ), sizeof( name ) );
	Q_strncpyz( sortBy, CG_Argv( 3 ), sizeof( name ) );

	if ( name[ 0 ] && table[ 0 ] && sortBy[ 0 ] )
	{
		CG_Rocket_SortDataSource( name, table, sortBy );
		return;
	}

	Com_Printf( "^3WARNING: Invalid syntax for 'sortDS'\n sortDS <data source> <table name> <sort by>\n" );
}

static void CG_Rocket_ExecDS( void )
{
	char table[ 100 ];
	Q_strncpyz( table, CG_Argv( 2 ), sizeof( table ) );
	CG_Rocket_ExecDataSource( CG_Argv( 1 ), table );
}

static void CG_Rocket_SetDS( void )
{
	char datasrc[ 100 ];

	Q_strncpyz( datasrc, CG_Argv( 1 ), sizeof( datasrc ) );
	CG_Rocket_SetDataSourceIndex( datasrc, atoi( CG_Argv( 2 ) ) );
}
typedef struct
{
	const char *command;
	void ( *exec ) ( void );
} eventCmd_t;

static const eventCmd_t eventCmdList[] =
{
	{ "blur", &CG_Rocket_EventBlur },
	{ "buildDS", &CG_Rocket_BuildDS },
	{ "close", &CG_Rocket_EventClose },
	{ "cvarform", &CG_Rocket_EventCvarForm },
	{ "exec", &CG_Rocket_EventExec },
	{ "execDS", &CG_Rocket_ExecDS },
	{ "goto", &CG_Rocket_EventGoto },
	{ "init_servers", &CG_Rocket_InitServers },
	{ "open", &CG_Rocket_EventOpen },
	{ "setDS", &CG_Rocket_SetDS },
	{ "show", &CG_Rocket_EventShow },
	{ "sortDS", &CG_Rocket_SortDS }
};

static const size_t eventCmdListCount = ARRAY_LEN( eventCmdList );

static int eventCmdCmp( const void *a, const void *b )
{
	return Q_stricmp( ( const char * ) a, ( ( eventCmd_t * ) b )->command );
}

void CG_Rocket_ProcessEvents( void )
{
	char *tail, *head;
	eventCmd_t *cmd;

	// Get the even command
	while ( trap_Rocket_GetEvent() )
	{

		cmd = bsearch( CG_Argv( 0 ), eventCmdList, eventCmdListCount, sizeof( eventCmd_t ), eventCmdCmp );

		if ( cmd )
		{
			cmd->exec();
		}

		trap_Rocket_DeleteEvent();
	}

}

