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


static void CG_Rocket_EventOpen( const char *args )
{
	trap_Rocket_LoadDocument( va( "%s%s.rml", rocketInfo.rootDir, args ) );
}

static void CG_Rocket_EventClose( const char *args )
{
	trap_Rocket_DocumentAction( args, "close" );
}

static void CG_Rocket_EventGoto( const char *args )
{
	trap_Rocket_DocumentAction( args, "goto" );
}

static void CG_Rocket_EventShow( const char *args )
{
	trap_Rocket_DocumentAction( args, "show" );
}

static void CG_Rocket_EventBlur( const char *args )
{
	trap_Rocket_DocumentAction( args, "blur" );
}


static void CG_Rocket_InitServers( const char *args )
{
	trap_LAN_ResetPings( CG_StringToNetSource( args ) );
	trap_LAN_ServerStatus( NULL, NULL, 0 );

	if ( !Q_stricmp( args, "internet" ) )
	{
		trap_Cmd_ExecuteText( EXEC_APPEND, "globalservers 0 86 full empty\n" );
	}

	else if ( !Q_stricmp( args, "local" ) )
	{
		trap_Cmd_ExecuteText( EXEC_APPEND, "localservers\n" );
	}

	trap_LAN_UpdateVisiblePings( CG_StringToNetSource( args ) );
}

static void CG_Rocket_BuildDS( const char *args )
{
	CG_Rocket_BuildDataSource( args );
}



static void CG_Rocket_EventExec( const char *args )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, args );
}

static void CG_Rocket_EventCvarForm( const char *args )
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

static void CG_Rocket_SortDS( const char *args )
{
	char *name, *table, *sortBy;
	char *p;

	name = BG_strdup( args );

	p = strchr( name, ' ' );
	if ( p )
	{
		*p = '\0';
		table = p + 1;
		if ( table )
		{
			p = strchr( table, ' ' );
			if ( p )
			{
				*p = '\0';
				sortBy = p + 1;
				if ( sortBy )
				{
					CG_Rocket_SortDataSource( name, table, sortBy );
					BG_Free( name );
					return;
				}
			}
		}
	}

	BG_Free( name );
	Com_Printf( "^3WARNING: Invalid syntax for 'sortDS'\n sortDS <data source> <table name> <sort by>\n" );
}

typedef struct
{
	const char *command;
	void ( *exec ) ( const char *args );
} eventCmd_t;

static const eventCmd_t eventCmdList[] =
{
	{ "blur", &CG_Rocket_EventBlur },
	{ "buildDS", &CG_Rocket_BuildDS },
	{ "close", &CG_Rocket_EventClose },
	{ "cvarform", &CG_Rocket_EventCvarForm },
	{ "exec", &CG_Rocket_EventExec },
	{ "goto", &CG_Rocket_EventGoto },
	{ "init_servers", &CG_Rocket_InitServers },
	{ "open", &CG_Rocket_EventOpen },
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
	static char commands[ 2000 ];
	char *tail, *head;
	eventCmd_t *cmd;

	// Get the even command
	trap_Rocket_GetEvent( commands, sizeof( commands ) );

	head = commands;

	// No events to process
	if ( !*head )
	{
		return;
	}

	while ( 1 )
	{
		char *p, *args;

		// Parse it. Check for semicolons first
		tail = strchr( head, ';' );
		if ( tail )
		{
			*tail = '\0';
		}

		p = strchr( head, ' ' );
		if ( p )
		{
			*p = '\0';
		}

		// Special case for when head has no arguments
		args = head + strlen( head ) + ( head + strlen( head ) == tail ? 0 : 1 );

		cmd = bsearch( head, eventCmdList, eventCmdListCount, sizeof( eventCmd_t ), eventCmdCmp );

		if ( cmd )
		{
			cmd->exec( args );
		}

		head = args + strlen( args ) + 1;

		if ( !*head )
		{
			break;
		}

		// Skip whitespaces
		while ( *head == ' ' )
		{
			head++;
		}
	}

	trap_Rocket_DeleteEvent();
}
