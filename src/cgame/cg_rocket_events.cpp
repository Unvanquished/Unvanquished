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

#include "cg_local.h"
#include "shared/CommonProxies.h"

static void CG_Rocket_EventOpen()
{
	Rocket_LoadDocument( va( "%s.rml", CG_Argv( 1 ) ) );
}

static void CG_Rocket_EventClose()
{
	Rocket_DocumentAction( CG_Argv( 1 ), "close" );
}

static void CG_Rocket_EventGoto()
{
	Rocket_DocumentAction( CG_Argv( 1 ), "goto" );
}

static void CG_Rocket_EventShow()
{
	Rocket_DocumentAction( CG_Argv( 1 ), "show" );
}

static void CG_Rocket_EventBlur()
{
	Rocket_DocumentAction( CG_Argv( 1 ), "blur" );
}

static void CG_Rocket_EventHide()
{
	Rocket_DocumentAction( CG_Argv( 1 ), "hide" );
}


static void CG_Rocket_InitServers()
{
	const char *src = CG_Argv( 1 );
	trap_LAN_ResetPings( CG_StringToNetSource( src ) );
	trap_LAN_ResetServerStatus();

	if ( !Q_stricmp( src, "internet" ) )
	{
		trap_SendConsoleCommand( "globalservers * 86 full empty\n" );
	}

	else if ( !Q_stricmp( src, "local" ) )
	{
		trap_SendConsoleCommand( "localservers\n" );
	}

	trap_LAN_UpdateVisiblePings( CG_StringToNetSource( src ) );
}

static void CG_Rocket_BuildDS()
{
	char table[ 100 ];

	Q_strncpyz( table, CG_Argv( 2 ), sizeof( table ) );
	CG_Rocket_BuildDataSource( CG_Argv( 1 ), table );
}



static void CG_Rocket_EventExec()
{
	const char *args = CG_Args();
	args = Q_stristr( args, " ");
	trap_SendConsoleCommand( args );
}

static void CG_Rocket_SortDS()
{
	char name[ 100 ], table[ 100 ], sortBy[ 100 ];

	Q_strncpyz( name, CG_Argv( 1 ), sizeof( name ) );
	Q_strncpyz( table, CG_Argv( 2 ), sizeof( table ) );
	Q_strncpyz( sortBy, CG_Argv( 3 ), sizeof( sortBy ) );

	if ( name[ 0 ] && table[ 0 ] && sortBy[ 0 ] )
	{
		CG_Rocket_SortDataSource( name, table, sortBy );
		return;
	}

	Log::Warn( "Invalid syntax for 'sortDS'\n sortDS <data source> <table name> <sort by>\n" );
}

static void CG_Rocket_ExecDS()
{
	char table[ 100 ];
	Q_strncpyz( table, CG_Argv( 2 ), sizeof( table ) );
	CG_Rocket_ExecDataSource( CG_Argv( 1 ), table );
}

static void CG_Rocket_SetDS()
{
	char datasrc[ 100 ];
	char datatbl[ 100 ];

	Q_strncpyz( datasrc, CG_Argv( 1 ), sizeof( datasrc ) );
	Q_strncpyz( datatbl, CG_Argv( 2 ), sizeof( datatbl ) );
	CG_Rocket_SetDataSourceIndex( datasrc, datatbl, atoi( CG_Argv( 3 ) ) );
}

static void CG_Rocket_SetAttribute()
{
	char attribute[ 100 ], value[ MAX_STRING_CHARS ];

	Q_strncpyz( attribute, CG_Argv( 1 ), sizeof( attribute ) );
	Q_strncpyz( value, CG_Argv( 2 ), sizeof( value ) );

	Rocket_SetAttribute( "", "", attribute, value );

}

static void CG_Rocket_FilterDS()
{
	char src[ 100 ];
	char tbl[ 100 ];
	char params[ MAX_STRING_CHARS ];

	Rocket_GetAttribute( "", "", "value", params, sizeof( params ) );

	Q_strncpyz( src, CG_Argv( 1 ), sizeof ( src ) );
	Q_strncpyz( tbl, CG_Argv( 2 ), sizeof( tbl ) );

	CG_Rocket_FilterDataSource( src, tbl, params );
}

static void CG_Rocket_SetChatCommand()
{
	const char *cmd = nullptr;

	switch ( cg.sayType )
	{
		case SAY_TYPE_PUBLIC:
			cmd = "say";
			break;

		case SAY_TYPE_TEAM:
			cmd = "say_team";
			break;

		case SAY_TYPE_ADMIN:
			cmd = "a";
			break;

		case SAY_TYPE_COMMAND:
			cmd = "/";
			break;

		default:
			return;
	}

	if ( cmd )
	{
		Rocket_SetAttribute( "", "", "exec", cmd );
	}
}

static void CG_Rocket_EventExecForm()
{
	static char params[ BIG_INFO_STRING ];
	char cmd[ MAX_STRING_CHARS ]  = { 0 };
	char Template[ MAX_STRING_CHARS ];
	char *k = Template;
	char *s = Template;

	Q_strncpyz( Template, CG_Argv( 1 ), sizeof( Template ) );
	Rocket_GetEventParameters( params, sizeof( params ) );

	if ( !*params )
	{
		Log::Warn( "Invalid form submit.  No named values exist.\n" );
		return;
	}

	while ( k && *k )
	{
		s = strchr( k, '$' );
		if ( s )
		{
			char *ss = strchr( s + 1, '$' );
			if ( ss )
			{
				*s =  0;
				*ss = 0;
				Q_strcat( cmd, sizeof( cmd ), k );
				Q_strcat( cmd, sizeof( cmd ), Info_ValueForKey( params, s + 1 ) );
				k = ss + 1;
			}
			else
			{
				Log::Warn("Unterminated $ in execForm: %s", CG_Argv( 1 ) );
				return;
			}
		}
		else
		{
			Q_strcat( cmd, sizeof( cmd ), k );
			break;
		}
	}

	if ( cmd[0] != '\0' )
	{
		trap_SendConsoleCommand( cmd );
	}
}

static void CG_Rocket_SetDataSelectValue()
{
	char src[ 100 ];
	char tbl[ 100 ];
	int index;

	Q_strncpyz( src, CG_Argv( 1 ), sizeof ( src ) );
	Q_strncpyz( tbl, CG_Argv( 2 ), sizeof( tbl ) );

	index = CG_Rocket_GetDataSourceIndex( src, tbl );

	if ( index > -1 )
	{
		Rocket_SetDataSelectIndex( index );
	}

}

static void CG_Rocket_EventPlay()
{
	const char *track = nullptr;

	// Specifying multiple files to randomly select between
	if ( trap_Argc() > 2 )
	{
		int numSounds = atoi( CG_Argv( 1 ) );
		if ( numSounds > 0 )
		{
			int selection = rand() % numSounds + 1;
			track = CG_Argv( 1 + selection );
			trap_S_StartBackgroundTrack( track, track );
		}
	}
	else
	{
		track = CG_Argv( 1 );
		trap_S_StartBackgroundTrack( track, track );
	}
}

static void CG_Rocket_ResetPings()
{
	const char *src = CG_Argv( 1 );
	trap_LAN_ResetPings( CG_StringToNetSource( src ) );
	trap_LAN_UpdateVisiblePings( CG_StringToNetSource( src ) );
}

struct eventCmd_t
{
	const char *const command;
	void ( *exec ) ();
};

static const eventCmd_t eventCmdList[] =
{
	{ "blur", &CG_Rocket_EventBlur },
	{ "buildDS", &CG_Rocket_BuildDS },
	{ "close", &CG_Rocket_EventClose },
	{ "exec", &CG_Rocket_EventExec },
	{ "execDS", &CG_Rocket_ExecDS },
	{ "execForm", &CG_Rocket_EventExecForm },
	{ "filterDS", &CG_Rocket_FilterDS },
	{ "goto", &CG_Rocket_EventGoto },
	{ "hide", &CG_Rocket_EventHide },
	{ "init_servers", &CG_Rocket_InitServers },
	{ "open", &CG_Rocket_EventOpen },
	{ "play", &CG_Rocket_EventPlay },
	{ "resetPings", &CG_Rocket_ResetPings },
	{ "setAttribute", &CG_Rocket_SetAttribute },
	{ "setChatCommand", &CG_Rocket_SetChatCommand },
	{ "setDataSelectValue", &CG_Rocket_SetDataSelectValue },
	{ "setDS", &CG_Rocket_SetDS },
	{ "show", &CG_Rocket_EventShow },
	{ "sortDS", &CG_Rocket_SortDS }
};

static const size_t eventCmdListCount = ARRAY_LEN( eventCmdList );

static int eventCmdCmp( const void *a, const void *b )
{
	return Q_stricmp( ( const char * ) a, ( ( eventCmd_t * ) b )->command );
}

void CG_Rocket_ProcessEvents()
{
	eventCmd_t *cmd;
	std::string cmdText;

	// Get the even command
	while ( Rocket_GetEvent(cmdText) )
	{
		Cmd::PushArgs(cmdText);
		cmd = (eventCmd_t*) bsearch( CG_Argv( 0 ), eventCmdList, eventCmdListCount, sizeof( eventCmd_t ), eventCmdCmp );

		if ( cmd )
		{
			cmd->exec();
		}
		Cmd::PopArgs();
		Rocket_DeleteEvent();
	}

}
