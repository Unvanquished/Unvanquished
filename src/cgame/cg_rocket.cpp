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

rocketInfo_t rocketInfo;

vmCvar_t rocket_menuFile;
vmCvar_t rocket_hudFile;
vmCvar_t rocket_pak;

typedef struct
{
	vmCvar_t   *vmCvar;
	const char *cvarName;
	const char *defaultString;
	int        cvarFlags;
} cvarTable_t;

static const cvarTable_t rocketCvarTable[] =
{
	{ &rocket_hudFile, "rocket_hudFile", "ui/rockethud.txt", 0 },
	{ &rocket_menuFile, "rocket_menuFile", "ui/rocket.txt", 0 },
	{ &rocket_pak, "rocket_pak", "", 0 },
};

static const size_t rocketCvarTableSize = ARRAY_LEN( rocketCvarTable );

/*
=================
CG_RegisterRocketCvars
=================
*/
void CG_RegisterRocketCvars()
{
	unsigned i;
	const cvarTable_t *cv;

	for ( i = 0, cv = rocketCvarTable; i < rocketCvarTableSize; i++, cv++ )
	{
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
		                    cv->defaultString, cv->cvarFlags );
	}
}

static connstate_t oldConnState;

void CG_Rocket_Init( glconfig_t gl )
{
	int len;
	const char *token, *text_p;
	char text[ 20000 ];
	fileHandle_t f;

	oldConnState = CA_UNINITIALIZED;
	cgs.glconfig = gl;

	// Init Rocket
	Rocket_Init();

	// rocket cvars
	CG_RegisterRocketCvars();
	CG_InitConsoleCommands();

	// Intialize data sources...
	CG_Rocket_RegisterDataSources();
	CG_Rocket_RegisterDataFormatters();

	// Register elements
	CG_Rocket_RegisterElements();

	Rocket_RegisterProperty( "cell-color", "white", false, false, "color" );
	Rocket_RegisterProperty( "border-width", "0.5", false, false, "number" );
	Rocket_RegisterProperty( "unlocked-marker-color", "green", false, false, "color" );
	Rocket_RegisterProperty( "locked-marker-color", "red", false, false, "color" );

	// Load custom rocket pak if necessary
	if ( *rocket_pak.string )
	{
		// Only load stuff from ui/
		if ( !trap_FS_LoadPak( rocket_pak.string, "ui/" ) )
		{
			Com_Error( ERR_DROP, "Unable to load custom UI pak: %s.", rocket_pak.string );
		}
	}

	// Preload all the menu files...
	len = trap_FS_FOpenFile( rocket_menuFile.string, &f, FS_READ );

	if ( len <= 0 )
	{
		Com_Error( ERR_DROP, "Unable to load %s. No rocket menus loaded.", rocket_menuFile.string );
	}

	if ( len >= (int) sizeof( text ) - 1 )
	{
		trap_FS_FCloseFile( f );
		Com_Error( ERR_DROP, "File %s too long.", rocket_menuFile.string );
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	text_p = text;
	trap_FS_FCloseFile( f );

	// Parse files to load...
	while ( 1 )
	{
		token = COM_Parse2( &text_p );

		// Closing bracket. EOF
		if ( !*token || *token == '}' )
		{
			break;
		}

		// Ignore opening bracket
		if ( *token == '{' )
		{
			continue;
		}

		// Set the cursor
		if ( !Q_stricmp( token, "cursor" ) )
		{
			token = COM_Parse2( &text_p );

			// Skip non-RML files
			if ( Q_stricmp( token + strlen( token ) - 4, ".rml" ) )
			{
				continue;
			}

			Rocket_LoadCursor( token );
			continue;
		}

		if ( !Q_stricmp( token, "main" ) )
		{
			int i;

			token = COM_Parse2( &text_p );

			if ( *token != '{' )
			{
				Com_Error( ERR_DROP, "Error parsing %s. Expecting \"{\" but found \"%c\".", rocket_menuFile.string, *token );
			}

			for ( i = 0; i < ROCKETMENU_NUM_TYPES; ++i )
			{
				token = COM_Parse2( &text_p );

				if ( !*token )
				{
					Com_Error( ERR_DROP, "Error parsing %s. Unexpected end of file. Expecting path to RML menu.", rocket_menuFile.string );
				}

				rocketInfo.menu[ i ].path = BG_strdup( token );
				Rocket_LoadDocument( token );

				token = COM_Parse2( &text_p );

				if ( !*token )
				{
					Com_Error( ERR_DROP, "Error parsing %s. Unexpected end of file. Expecting RML document id.", rocket_menuFile.string );
				}

				rocketInfo.menu[ i ].id = BG_strdup( token );
			}

			token = COM_Parse2( &text_p );

			if ( *token != '}' )
			{
				Com_Error( ERR_DROP, "Error parsing %s. Expecting \"}\" but found \"%c\".", rocket_menuFile.string, *token );
			}

			while ( *token && *token != '}' )
			{
				token = COM_Parse2( &text_p );

				if ( !*token )
				{
					Com_Error( ERR_DROP, "Error parsing %s. Unexpected end of file. Expecting RML document.", rocket_menuFile.string );
				}

				Rocket_LoadDocument( token );
			}

			continue;
		}

		if ( !Q_stricmp( token, "misc" ) )
		{
			token = COM_Parse2( &text_p );

			if ( *token != '{' )
			{
				Com_Error( ERR_DROP, "Error parsing %s. Expecting \"{\" but found \"%c\".", rocket_menuFile.string, *token );
			}

			while ( 1 )
			{
				token = COM_Parse2( &text_p );

				if ( *token == '}' )
				{
					break;
				}

				if ( !*token )
				{
					Com_Error( ERR_DROP, "Error parsing %s. Unexpected end of file. Expecting closing '}'.", rocket_menuFile.string );
				}

				// Skip non-RML files
				if ( Q_stricmp( token + strlen( token ) - 4, ".rml" ) )
				{
					Com_Printf( "^3WARNING: Non-RML file listed in %s: \"%s\" . Skipping.", rocket_menuFile.string, token );
					continue;
				}

				Rocket_LoadDocument( token );

			}

			continue;
		}

		if ( !Q_stricmp( token, "fonts" ) )
		{
			token = COM_Parse2( &text_p );

			if ( *token != '{' )
			{
				Com_Error( ERR_DROP, "Error parsing %s. Expecting \"{\" but found \"%c\".", rocket_menuFile.string, *token );
			}

			while ( 1 )
			{
				token = COM_Parse2( &text_p );

				if ( *token == '}' )
				{
					break;
				}

				if ( !*token )
				{
					Com_Error( ERR_DROP, "Error parsing %s. Unexpected end of file. Expecting closing '}'.", rocket_menuFile.string );
				}

				Rocket_LoadFont( token );
			}

			continue;
		}
	}

	Rocket_DocumentAction( rocketInfo.menu[ ROCKETMENU_MAIN ].id, "open" );

	// Check if we need to display a server connect/disconnect error
	text[ 0 ] = '\0';
	trap_Cvar_VariableStringBuffer( "com_errorMessage", text, sizeof( text ) );
	if ( *text )
	{
		trap_Cvar_Set( "ui_errorMessage", text );
		Rocket_DocumentAction( rocketInfo.menu[ ROCKETMENU_ERROR ].id, "open" );
	}

	CG_SetKeyCatcher( KEYCATCH_UI );
}

void CG_Rocket_LoadHuds()
{
	int i, len;
	const char *token, *text_p;
	char text[ 20000 ];
	fileHandle_t f;

	// Preload all the menu files...
	len = trap_FS_FOpenFile( rocket_hudFile.string, &f, FS_READ );

	if ( len <= 0 )
	{
		Com_Error( ERR_DROP, "Unable to load %s. No rocket menus loaded.", rocket_menuFile.string );
	}

	if ( len >= (int) sizeof( text ) - 1 )
	{
		trap_FS_FCloseFile( f );
		Com_Error( ERR_DROP, "File %s too long.", rocket_hudFile.string );
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	text_p = text;
	trap_FS_FCloseFile( f );

	Rocket_InitializeHuds( WP_NUM_WEAPONS );

	// Parse files to load...

	while ( 1 )
	{
		bool valid = false;

		token = COM_Parse2( &text_p );

		if ( !*token )
		{
			break;
		}

		if ( !Q_stricmp( token, "units" ) )
		{
			while ( 1 )
			{
				int toklen;

				token = COM_Parse2( &text_p );
				toklen = strlen( token );

				if ( !*token )
				{
					Com_Error( ERR_DROP, "Unable to load huds from %s. Unexpected end of file. Expected closing } to close off units.", rocket_hudFile.string );
				}

				if ( *token == '}' )
				{
					break;
				}

				// Skip non-RML files and opening brace
				if ( toklen < 4 || Q_stricmp( token + toklen - 4, ".rml" ) )
				{
					continue;
				}

				Rocket_LoadUnit( token );
			}

			continue;
		}

		if ( !Q_stricmp( token, "human.hudgroup" ) )
		{
			// Clear old values
			for ( i = WP_BLASTER; i <= WP_LUCIFER_CANNON; ++i )
			{
				Rocket_ClearHud( i );
			}

			Rocket_ClearHud( WP_HBUILD );

			while ( 1 )
			{
				token = COM_Parse2( &text_p );

				if ( !*token )
				{
					Com_Error( ERR_DROP, "Unable to load huds from %s. Unexpected end of file. Expected closing } to close off human_hud.", rocket_hudFile.string );
				}

				if ( *token == '{' )
				{
					continue;
				}

				if ( *token == '}' )
				{
					break;
				}


				for ( i = WP_BLASTER; i <= WP_LUCIFER_CANNON; ++i )
				{
					Rocket_AddUnitToHud( i, token );
				}

				Rocket_AddUnitToHud( WP_HBUILD, token );
			}


			continue;
		}

		if ( !Q_stricmp( token, "spectator.hudgroup" ) )
		{
			for ( i = WP_NONE; i < WP_NUM_WEAPONS; ++i )
			{
				Rocket_ClearHud( i );
			}

			while ( 1 )
			{
				token = COM_Parse2( &text_p );

				if ( !*token )
				{
					Com_Error( ERR_DROP, "Unable to load huds from %s. Unexpected end of file. Expected closing } to close off spectator_hud.", rocket_hudFile.string );
				}

				if ( *token == '{' )
				{
					continue;
				}

				if ( *token == '}' )
				{
					break;
				}


				for ( i = WP_NONE; i < WP_NUM_WEAPONS; ++i )
				{
					Rocket_AddUnitToHud( i, token );
				}
			}

			continue;
		}

		if ( !Q_stricmp( token, "alien.hudgroup" ) )
		{
			for ( i = WP_ALEVEL0; i <= WP_ALEVEL4; ++i )
			{
				Rocket_ClearHud( i );
			}

			Rocket_ClearHud( WP_ABUILD );
			Rocket_ClearHud( WP_ABUILD2 );

			while ( 1 )
			{
				token = COM_Parse2( &text_p );

				if ( !*token )
				{
					Com_Error( ERR_DROP, "Unable to load huds from %s. Unexpected end of file. Expected closing } to close off alien_hud.", rocket_hudFile.string );
				}

				if ( *token == '{' )
				{
					continue;
				}

				if ( *token == '}' )
				{
					break;
				}

				for ( i = WP_ALEVEL0; i <= WP_ALEVEL4; ++i )
				{
					Rocket_AddUnitToHud( i, token );
				}

				Rocket_AddUnitToHud( WP_ABUILD, token );
				Rocket_AddUnitToHud( WP_ABUILD2, token );
			}

			continue;
		}

		for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; ++i )
		{
			if ( !Q_stricmp( token, va( "%s.hudgroup", BG_Weapon( i )->name ) ) )
			{
				Rocket_ClearHud( i );
				while ( 1 )
				{
					token = COM_Parse2( &text_p );

					if ( !*token )
					{
						Com_Error( ERR_DROP, "Unable to load huds from %s. Unexpected end of file. Expected closing } to close off %s_hud.", rocket_hudFile.string, BG_Weapon( i )->name );
					}

					if ( *token == '{' )
					{
						continue;
					}

					if ( *token == '}' )
					{
						break;
					}

					Rocket_AddUnitToHud( i, token );
				}

				valid = true;
				break;
			}
		}

		if ( !valid )
		{
			Com_Error( ERR_DROP, "Could not parse %s. Unrecognized top level item: %s", rocket_hudFile.string, token );
		}
	}
}

int CG_StringToNetSource( const char *src )
{
	if ( !Q_stricmp( src, "local" ) )
	{
		return AS_LOCAL;
	}

	else if ( !Q_stricmp( src, "favorites" ) )
	{
		return AS_FAVORITES;
	}

	else
	{
		return AS_GLOBAL;
	}
}

const char *CG_NetSourceToString( int netSrc )
{
	switch ( netSrc )
	{
		case AS_LOCAL:
			return "local";

		case AS_FAVORITES:
			return "favorites";

		default:
			return "internet";
	}
}


void CG_Rocket_Frame( cgClientState_t state )
{
	rocketInfo.cstate = state;
	rocketInfo.realtime = trap_Milliseconds();
	rocketInfo.keyCatcher = trap_Key_GetCatcher();

	if ( oldConnState != rocketInfo.cstate.connState )
	{
		switch ( rocketInfo.cstate.connState )
		{
			case CA_DISCONNECTED:
				// Kill the server if its still running
				if ( trap_Cvar_VariableIntegerValue( "sv_running" ) )
				{
					trap_Cvar_Set( "sv_killserver", "1" );
				}
				break;

			case CA_CONNECTING:
			case CA_CHALLENGING:
			case CA_CONNECTED:
				Rocket_DocumentAction( rocketInfo.menu[ ROCKETMENU_CONNECTING ].id, "show" );
				break;
			case CA_DOWNLOADING:
				Rocket_DocumentAction( rocketInfo.menu[ ROCKETMENU_DOWNLOADING ].id, "show" );
				break;
			case CA_LOADING:
			case CA_PRIMED:
				Rocket_DocumentAction( rocketInfo.menu[ ROCKETMENU_LOADING ].id, "show" );
				break;

			case CA_ACTIVE:
				Rocket_DocumentAction( "", "blurall" );
                break;

            default:
                break;
		}

		oldConnState = rocketInfo.cstate.connState;
	}

	// Continue to attempt to update serverlisting
	if ( rocketInfo.data.retrievingServers )
	{
		if ( !trap_LAN_UpdateVisiblePings( rocketInfo.currentNetSrc ) )
		{
			CG_Rocket_BuildServerList( CG_NetSourceToString( rocketInfo.currentNetSrc ) );
		}
	}

	// Continue to attempt to update serverinfo
	if ( rocketInfo.data.buildingServerInfo )
	{
		CG_Rocket_BuildServerInfo();
	}

	if ( cg.scoreInvalidated )
	{
		CG_Rocket_BuildPlayerList( nullptr );
		cg.scoreInvalidated = false;
	}

	// Update scores as long as they are showing
	if ( ( cg.showScores || cg.intermissionStarted ) && cg.scoresRequestTime + 2000 < cg.time )
	{
		CG_RequestScores();
	}

	CG_Rocket_ProcessEvents();
	Rocket_Update();
	Rocket_Render();
}

const char *CG_Rocket_GetTag()
{
	static char tag[ 100 ];

	Rocket_GetElementTag( tag, sizeof( tag ) );

	return tag;
}

const char *CG_Rocket_GetAttribute( const char *attribute )
{
	static char buffer[ MAX_STRING_CHARS ];

	Rocket_GetAttribute( "", "", attribute, buffer, sizeof( buffer ) );

	return buffer;
}

const char *CG_Rocket_QuakeToRML( const char *in )
{
	static char buffer[ MAX_STRING_CHARS ];
	Rocket_QuakeToRMLBuffer( in, buffer, sizeof( buffer ) );
	return buffer;
}

bool CG_Rocket_IsCommandAllowed( rocketElementType_t type )
{
	playerState_t *ps;

	switch ( type )
	{
		case ELEMENT_ALL:
			return true;

		case ELEMENT_LOADING:
			if ( rocketInfo.cstate.connState < CA_ACTIVE && rocketInfo.cstate.connState > CA_CONNECTED )
			{
				return true;
			}

			return false;

		case ELEMENT_GAME:
			if ( rocketInfo.cstate.connState == CA_ACTIVE )
			{
				return true;
			}

			return false;

		default:
			break;
	}


	if ( !cg.snap )
	{
		return false;
	}
	ps = &cg.snap->ps;
	switch( type )
	{
		case ELEMENT_ALIENS:
			if ( ps->persistant[ PERS_TEAM ] == TEAM_ALIENS && ps->stats[ STAT_HEALTH ] > 0 && ps->weapon != WP_NONE )
			{
				return true;
			}

			return false;

		case ELEMENT_HUMANS:
			if ( ps->persistant[ PERS_TEAM ] == TEAM_HUMANS && ps->stats[ STAT_HEALTH ] > 0 && ps->weapon != WP_NONE )
			{
				return true;
			}

			return false;

		case ELEMENT_BOTH:
			if ( ps->persistant[ PERS_TEAM ] != TEAM_NONE && ps->stats[ STAT_HEALTH ] > 0 && ps->weapon != WP_NONE )
			{
				return true;
			}

			return false;

		case ELEMENT_DEAD:
			// If you're on a team and spectating, you're probably dead
			if ( ps->persistant[ PERS_TEAM ] != TEAM_NONE && ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
			{
				return true;
			}

			return false;

        default:
            return false;
	}

	return false;
}
