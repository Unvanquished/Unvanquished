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
	trap_Rocket_LoadDocument( va( "%s.rml", CG_Argv( 1 ) ) );
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

static void CG_Rocket_EventHide( void )
{
	trap_Rocket_DocumentAction( CG_Argv( 1 ), "hide" );
}


static void CG_Rocket_InitServers( void )
{
	const char *src = CG_Argv( 1 );
	trap_LAN_ResetPings( CG_StringToNetSource( src ) );
	trap_LAN_ServerStatus( NULL, NULL, 0 );

	if ( !Q_stricmp( src, "internet" ) )
	{
		trap_SendConsoleCommand( "globalservers 0 86 full empty\n" );
	}

	else if ( !Q_stricmp( src, "local" ) )
	{
		trap_SendConsoleCommand( "localservers\n" );
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
	const char *args = CG_Args();
	args = Q_stristr( args, " ");
	trap_SendConsoleCommand( args );
}

static void CG_Rocket_SortDS( void )
{
	char name[ 100 ], table[ 100 ], sortBy[ 100 ];
	char *p;

	Q_strncpyz( name, CG_Argv( 1 ), sizeof( name ) );
	Q_strncpyz( table, CG_Argv( 2 ), sizeof( table ) );
	Q_strncpyz( sortBy, CG_Argv( 3 ), sizeof( sortBy ) );

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
	char datatbl[ 100 ];

	Q_strncpyz( datasrc, CG_Argv( 1 ), sizeof( datasrc ) );
	Q_strncpyz( datatbl, CG_Argv( 2 ), sizeof( datatbl ) );
	CG_Rocket_SetDataSourceIndex( datasrc, datatbl, atoi( CG_Argv( 3 ) ) );
}

static void CG_Rocket_SetAttribute( void )
{
	char attribute[ 100 ], value[ MAX_STRING_CHARS ];

	Q_strncpyz( attribute, CG_Argv( 1 ), sizeof( attribute ) );
	Q_strncpyz( value, CG_Argv( 2 ), sizeof( value ) );

	trap_Rocket_SetAttribute( attribute, value );

}

static void CG_Rocket_FilterDS( void )
{
	char src[ 100 ];
	char tbl[ 100 ];
	char params[ MAX_STRING_CHARS ];

	trap_Rocket_GetAttribute( "value", params, sizeof( params ) );

	Q_strncpyz( src, CG_Argv( 1 ), sizeof ( src ) );
	Q_strncpyz( tbl, CG_Argv( 2 ), sizeof( tbl ) );

	CG_Rocket_FilterDataSource( src, tbl, params );
}

static void CG_Rocket_SetChatCommand( void )
{
	const char *cmd = NULL;

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
		trap_Rocket_SetAttribute( "exec", cmd );
	}
}

static void CG_Rocket_EventExecForm( void )
{
	static char params[ BIG_INFO_STRING ];
	static char key[BIG_INFO_VALUE], value[ BIG_INFO_VALUE ];
	char cmd[ MAX_STRING_CHARS ]  = { 0 };
	char Template[ MAX_STRING_CHARS ];
	char *k = Template;
	char *s = Template;

	Q_strncpyz( Template, CG_Argv( 1 ), sizeof( Template ) );
	trap_Rocket_GetEventParameters( params, sizeof( params ) );

	if ( !*params )
	{
		Com_Printf( "^3WARNING: Invalid form submit.  No named values exist.\n" );
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
			}

			k = ss + 1;
		}
	}

	if ( *cmd )
	{
		trap_SendConsoleCommand( cmd );
	}
}

static void CG_Rocket_SetDataSelectValue( void )
{
	char src[ 100 ];
	char tbl[ 100 ];
	int index;

	Q_strncpyz( src, CG_Argv( 1 ), sizeof ( src ) );
	Q_strncpyz( tbl, CG_Argv( 2 ), sizeof( tbl ) );

	index = CG_Rocket_GetDataSourceIndex( src, tbl );

	if ( index > -1 )
	{
		trap_Rocket_SetDataSelectIndex( index );
	}

}

static void CG_Rocket_GraphicsQualityChanged( void )
{
	switch ( trap_Cvar_VariableIntegerValue( "ui_glCustom" ) )
	{
		case 0: // high quality
			trap_Cvar_Set( "r_subdivisions", "4" );
			trap_Cvar_Set( "r_vertexlighting", "0" );
			trap_Cvar_Set( "r_picmip", "0" );
			trap_Cvar_Set( "r_inGameVideo", "1" );
			trap_Cvar_Set( "cg_shadows", "4" );
			trap_Cvar_Set( "r_dynamiclight", "1" );
			trap_Cvar_Set( "cg_bounceParticles", "1" );
			trap_Cvar_Set( "r_normalMapping", "1" );
			trap_Cvar_Set( "r_bloom", "1" );
			trap_Cvar_Set( "r_rimlighting", "1" );
			trap_Cvar_Set( "cg_motionblur", "0.05" );
			trap_Cvar_Set( "r_ext_multisample", "8" );
			trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "8" );
			trap_Cvar_Set( "r_heathaze", "1" );
			trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
			break;

		case 1: // intermediate
			trap_Cvar_Set( "r_subdivisions", "8" );
			trap_Cvar_Set( "r_vertexlighting", "0" );
			trap_Cvar_Set( "r_picmip", "0" );
			trap_Cvar_Set( "r_inGameVideo", "1" );
			trap_Cvar_Set( "cg_shadows", "1" );
			trap_Cvar_Set( "r_dynamiclight", "1" );
			trap_Cvar_Set( "cg_bounceParticles", "0" );
			trap_Cvar_Set( "r_normalMapping", "1" );
			trap_Cvar_Set( "r_bloom", "1" );
			trap_Cvar_Set( "r_rimlighting", "1" );
			trap_Cvar_Set( "cg_motionblur", "0" );
			trap_Cvar_Set( "r_ext_multisample", "4" );
			trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "4" );
			trap_Cvar_Set( "r_heathaze", "0" );
			trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
			break;

		case 2: // fast
			trap_Cvar_Set( "r_subdivisions", "12" );
			trap_Cvar_Set( "r_vertexlighting", "0" );
			trap_Cvar_Set( "r_picmip", "1" );
			trap_Cvar_Set( "r_inGameVideo", "0" );
			trap_Cvar_Set( "cg_shadows", "0" );
			trap_Cvar_Set( "r_dynamiclight", "0" );
			trap_Cvar_Set( "cg_bounceParticles", "0" );
			trap_Cvar_Set( "r_normalMapping", "0" );
			trap_Cvar_Set( "r_bloom", "1" );
			trap_Cvar_Set( "r_rimlighting", "1" );
			trap_Cvar_Set( "cg_motionblur", "0" );
			trap_Cvar_Set( "r_ext_multisample", "2" );
			trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "2" );
			trap_Cvar_Set( "r_heathaze", "0" );
			trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;

		case 3: // fastest
			trap_Cvar_Set( "r_subdivisions", "20" );
			trap_Cvar_Set( "r_vertexlighting", "1" );
			trap_Cvar_Set( "r_picmip", "2" );
			trap_Cvar_Set( "r_inGameVideo", "0" );
			trap_Cvar_Set( "cg_shadows", "0" );
			trap_Cvar_Set( "r_dynamiclight", "0" );
			trap_Cvar_Set( "cg_bounceParticles", "0" );
			trap_Cvar_Set( "r_normalMapping", "0" );
			trap_Cvar_Set( "r_bloom", "0" );
			trap_Cvar_Set( "r_rimlighting", "0" );
			trap_Cvar_Set( "cg_motionblur", "0" );
			trap_Cvar_Set( "r_ext_multisample", "0" );
			trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
			trap_Cvar_Set( "r_heathaze", "0" );
			trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;

		// 4 is custom

		case 5: // dlights, no shader effects
			trap_Cvar_Set( "r_subdivisions", "20" );
			trap_Cvar_Set( "r_vertexlighting", "0" );
			trap_Cvar_Set( "r_picmip", "0" );
			trap_Cvar_Set( "r_inGameVideo", "0" );
			trap_Cvar_Set( "cg_shadows", "0" );
			trap_Cvar_Set( "r_dynamiclight", "1" );
			trap_Cvar_Set( "cg_bounceParticles", "0" );
			trap_Cvar_Set( "r_normalMapping", "0" );
			trap_Cvar_Set( "r_bloom", "0" );
			trap_Cvar_Set( "r_rimlighting", "0" );
			trap_Cvar_Set( "cg_motionblur", "0" );
			trap_Cvar_Set( "r_ext_multisample", "0" );
			trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
			trap_Cvar_Set( "r_heathaze", "0" );
			trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;
	}
}

static void CG_Rocket_EventPlay( void )
{
	const char *track = NULL;

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

typedef struct
{
	const char *const command;
	void ( *exec ) ( void );
} eventCmd_t;

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
	{ "graphicsqualitychanged", &CG_Rocket_GraphicsQualityChanged },
	{ "hide", &CG_Rocket_EventHide },
	{ "init_servers", &CG_Rocket_InitServers },
	{ "open", &CG_Rocket_EventOpen },
	{ "play", &CG_Rocket_EventPlay },
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

void CG_Rocket_ProcessEvents( void )
{
	char *tail, *head;
	eventCmd_t *cmd;

	// Get the even command
	while ( trap_Rocket_GetEvent() )
	{
		cmd = (eventCmd_t*) bsearch( CG_Argv( 0 ), eventCmdList, eventCmdListCount, sizeof( eventCmd_t ), eventCmdCmp );

		if ( cmd )
		{
			cmd->exec();
		}

		trap_Rocket_DeleteEvent();
	}

}


