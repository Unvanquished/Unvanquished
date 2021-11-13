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

rocketInfo_t rocketInfo;

Cvar::Cvar<std::string> rocket_menuFile("rocket_menuFile", "VFS path of config file for menus", Cvar::NONE, "ui/rocket.txt");
Cvar::Cvar<std::string> rocket_hudFile("rocket_hudFile", "VFS path of config file for HUD", Cvar::NONE, "ui/rockethud.txt");

static connstate_t oldConnState;

void CG_Rocket_Init( glconfig_t gl )
{
	int len;
	const char *token, *text_p;
	char text[ 20000 ];
	fileHandle_t f;

	oldConnState = connstate_t::CA_UNINITIALIZED;
	cgs.glconfig = gl;

	Trans_Init();

	BG_LoadEmoticons();

	// Init Rocket
	Rocket_Init();

	CG_InitConsoleCommands();

	// Initialize data sources...
	CG_Rocket_RegisterDataSources();
	CG_Rocket_RegisterDataFormatters();

	// Register elements
	CG_Rocket_RegisterElements();

	// This gets 12px on 1920×1080 screen, which is libRocket default for 1em
	int fontSize = std::min(cgs.glconfig.vidWidth, cgs.glconfig.vidHeight) / 90;
	Rocket_RegisterProperty( "font-size", std::to_string(fontSize).c_str(), true, true, "number" );

	Rocket_RegisterProperty( "cell-color", "white", false, false, "color" );
	Rocket_RegisterProperty( "border-width", "0.5", false, false, "number" );
	Rocket_RegisterProperty( "unlocked-marker-color", "green", false, false, "color" );
	Rocket_RegisterProperty( "locked-marker-color", "red", false, false, "color" );

	// Preload all the menu files...
	len = trap_FS_FOpenFile( rocket_menuFile.Get().c_str(), &f, fsMode_t::FS_READ );

	if ( len <= 0 )
	{
		Sys::Drop( "Unable to load %s. No rocket menus loaded.", rocket_menuFile.Get() );
	}

	if ( len >= (int) sizeof( text ) - 1 )
	{
		trap_FS_FCloseFile( f );
		Sys::Drop( "File %s too long.", rocket_menuFile.Get() );
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
				Sys::Drop( "Error parsing %s. Expecting \"{\" but found \"%c\".", rocket_menuFile.Get(), *token );
			}

			for ( i = 0; i < ROCKETMENU_NUM_TYPES; ++i )
			{
				token = COM_Parse2( &text_p );

				if ( !*token )
				{
					Sys::Drop( "Error parsing %s. Unexpected end of file. Expecting path to RML menu.", rocket_menuFile.Get() );
				}

				rocketInfo.menu[ i ].path = BG_strdup( token );
				Rocket_LoadDocument( token );

				token = COM_Parse2( &text_p );

				if ( !*token )
				{
					Sys::Drop( "Error parsing %s. Unexpected end of file. Expecting RML document id.", rocket_menuFile.Get() );
				}

				rocketInfo.menu[ i ].id = BG_strdup( token );
			}

			token = COM_Parse2( &text_p );

			if ( *token != '}' )
			{
				Sys::Drop( "Error parsing %s. Expecting \"}\" but found \"%c\".", rocket_menuFile.Get(), *token );
			}

			while ( *token && *token != '}' )
			{
				token = COM_Parse2( &text_p );

				if ( !*token )
				{
					Sys::Drop( "Error parsing %s. Unexpected end of file. Expecting RML document.", rocket_menuFile.Get() );
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
				Sys::Drop( "Error parsing %s. Expecting \"{\" but found \"%c\".", rocket_menuFile.Get(), *token );
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
					Sys::Drop( "Error parsing %s. Unexpected end of file. Expecting closing '}'.", rocket_menuFile.Get() );
				}

				// Skip non-RML files
				if ( Q_stricmp( token + strlen( token ) - 4, ".rml" ) )
				{
					Log::Warn( "Non-RML file listed in %s: \"%s\" . Skipping.", rocket_menuFile.Get(), token );
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
				Sys::Drop( "Error parsing %s. Expecting \"{\" but found \"%c\".", rocket_menuFile.Get(), *token );
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
					Sys::Drop( "Error parsing %s. Unexpected end of file. Expecting closing '}'.", rocket_menuFile.Get() );
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
	len = trap_FS_FOpenFile( rocket_hudFile.Get().c_str(), &f, fsMode_t::FS_READ );

	if ( len <= 0 )
	{
		Sys::Drop( "Unable to load %s. No rocket menus loaded.", rocket_menuFile.Get() );
	}

	if ( len >= (int) sizeof( text ) - 1 )
	{
		trap_FS_FCloseFile( f );
		Sys::Drop( "File %s too long.", rocket_hudFile.Get() );
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
					Sys::Drop( "Unable to load huds from %s. Unexpected end of file. Expected closing } to close off units.", rocket_hudFile.Get() );
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
					Sys::Drop( "Unable to load huds from %s. Unexpected end of file. Expected closing } to close off human_hud.", rocket_hudFile.Get() );
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
					Sys::Drop( "Unable to load huds from %s. Unexpected end of file. Expected closing } to close off spectator_hud.", rocket_hudFile.Get() );
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
					Sys::Drop( "Unable to load huds from %s. Unexpected end of file. Expected closing } to close off alien_hud.", rocket_hudFile.Get() );
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
						Sys::Drop( "Unable to load huds from %s. Unexpected end of file. Expected closing } to close off %s_hud.", rocket_hudFile.Get(), BG_Weapon( i )->name );
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
			Sys::Drop( "Could not parse %s. Unrecognized top level item: %s", rocket_hudFile.Get(), token );
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
	return AS_GLOBAL;
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
			case connstate_t::CA_DISCONNECTED:
				// Kill the server if its still running
				if ( trap_Cvar_VariableIntegerValue( "sv_running" ) )
				{
					trap_Cvar_Set( "sv_killserver", "1" );
				}
				break;

			case connstate_t::CA_CONNECTING:
			case connstate_t::CA_CHALLENGING:
			case connstate_t::CA_CONNECTED:
				Rocket_DocumentAction( rocketInfo.menu[ ROCKETMENU_CONNECTING ].id, "show" );
				break;
			case connstate_t::CA_DOWNLOADING:
				Rocket_DocumentAction( rocketInfo.menu[ ROCKETMENU_DOWNLOADING ].id, "show" );
				break;
			case connstate_t::CA_LOADING:
			case connstate_t::CA_PRIMED:
				Rocket_DocumentAction( rocketInfo.menu[ ROCKETMENU_LOADING ].id, "show" );
				break;

			case connstate_t::CA_ACTIVE:
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
			return (rocketInfo.cstate.connState
					< connstate_t::CA_ACTIVE)
				&& (rocketInfo.cstate.connState
					> connstate_t::CA_CONNECTED);

		case ELEMENT_GAME:
			return rocketInfo.cstate.connState
				== connstate_t::CA_ACTIVE;

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
			return (ps->persistant[ PERS_TEAM ] == TEAM_ALIENS)
				&& (ps->stats[ STAT_HEALTH ] > 0)
				&& (ps->weapon != WP_NONE);

		case ELEMENT_HUMANS:
			return (ps->persistant[ PERS_TEAM ] == TEAM_HUMANS)
				&& (ps->stats[ STAT_HEALTH ] > 0)
				&& (ps->weapon != WP_NONE);

		case ELEMENT_BOTH:
			return (ps->persistant[ PERS_TEAM ] != TEAM_NONE)
				&& (ps->stats[ STAT_HEALTH ] > 0)
				&& (ps->weapon != WP_NONE);

		case ELEMENT_DEAD:
			// If you're on a team and spectating, you're probably dead
			return (ps->persistant[ PERS_TEAM ] != TEAM_NONE)
				&& (ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT);

		default:
			return false;
	}
}
