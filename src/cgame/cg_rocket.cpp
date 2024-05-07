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

#include "common/FileSystem.h"
#include "cg_local.h"

rocketInfo_t rocketInfo = {};

Cvar::Cvar<std::string> rocket_menuFile("rocket_menuFile", "VFS path of config file for menus", Cvar::CHEAT, "ui/rocket.txt");
Cvar::Cvar<std::string> rocket_hudFile("rocket_hudFile", "VFS path of config file for HUD", Cvar::CHEAT, "ui/rockethud.txt");

static connstate_t oldConnState;

void CG_Rocket_Init( glconfig_t gl )
{
	const char *token, *text_p;
	char text[ 20000 ];

	oldConnState = connstate_t::CA_UNINITIALIZED;
	cgs.glconfig = gl;
	rocketInfo.keyCatcher = trap_Key_GetCatcher();

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

	Rocket_RegisterProperty( "cell-color", "white", false, false, "color" );
	Rocket_RegisterProperty( "momentum-border-width", "0.5", false, false, "number" );
	Rocket_RegisterProperty( "unlocked-marker-color", "green", false, false, "color" );
	Rocket_RegisterProperty( "locked-marker-color", "red", false, false, "color" );

	// Preload all the menu files...
	std::string content = FS::PakPath::ReadFile( rocket_menuFile.Get() );

	text_p = content.c_str();

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
		// TODO: Use the RmlUi cursor mechanism to allow per-cursor stuff...
		if ( !Q_stricmp( token, "cursor" ) )
		{
			token = COM_Parse2( &text_p );


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

	CG_SetKeyCatcher( rocketInfo.keyCatcher | KEYCATCH_UI );
}

// Cvars set in cgame and made public
#define CGAME_CVAR( cvar ) if ( name == #cvar ) { return std::to_string( cvar.Get() ); }
#define CGAME_CVAR2( cvar, obj ) if ( name == #cvar ) { return std::to_string( obj.Get() ); }
// Cvars set in cgame but not made public
#define PRIVATE_CVAR( cvar ) if ( name == #cvar ) { return Cvar::GetValue( #cvar ); }
// Cvars set in cgame using trap_Cvar_Set()
#define DIRECT_CVAR PRIVATE_CVAR
// Cvars set in RML
#define RML_CVAR PRIVATE_CVAR
// Cvars set in sgame
#define SGAME_CVAR PRIVATE_CVAR
// Cvars set in engine
#define ENGINE_CVAR PRIVATE_CVAR

// Set to 1 to catch cvars that are used by the UI but unknown yet.
#define REPORT_UNKNOWN_CVARS 0

// HACK: Use local cvar access for known ones when possible.
std::string CG_Rocket_GetCvarValue( std::string name )
{
	/* HUD cvars used on every frame,
	using direct access is critical for performance. */
	CGAME_CVAR( cg_drawClock );
	CGAME_CVAR( cg_drawFPS );
	CGAME_CVAR( cg_drawMinimap );
	CGAME_CVAR( cg_drawSpeed );
	CGAME_CVAR( cg_drawTimer );
	CGAME_CVAR( cg_lagometer );
	CGAME_CVAR( cg_minimapActive );

	// Menus
	CGAME_CVAR( cg_bounceParticles );
	CGAME_CVAR( cg_crosshairColorAlpha );
	CGAME_CVAR( cg_crosshairColorBlue );
	CGAME_CVAR( cg_crosshairColorGreen );
	CGAME_CVAR( cg_crosshairColorRed );
	CGAME_CVAR( cg_crosshairOutlineColorAlpha );
	CGAME_CVAR( cg_crosshairOutlineColorBlue );
	CGAME_CVAR( cg_crosshairOutlineColorGreen );
	CGAME_CVAR( cg_crosshairOutlineColorRed );
	CGAME_CVAR( cg_crosshairOutlineOffset );
	CGAME_CVAR( cg_crosshairOutlineScale );
	CGAME_CVAR( cg_crosshairOutlineStyle );
	CGAME_CVAR( cg_crosshairSize );
	CGAME_CVAR( cg_crosshairStyle );
	CGAME_CVAR( cg_drawCrosshair );
	CGAME_CVAR( cg_mirrorgun );
	CGAME_CVAR( cg_motionblur );
	CGAME_CVAR( cg_motionblurMinSpeed );
	CGAME_CVAR( cg_sprintToggle );
	CGAME_CVAR( cg_teamChatsOnly );
	CGAME_CVAR( cg_tutorial );
	CGAME_CVAR( cg_wwSmoothTime );

	CGAME_CVAR2( cg_drawgun, cg_drawGun );
	CGAME_CVAR2( cg_marks, cg_addMarks );

#if REPORT_UNKNOWN_CVARS
	// Not private but there is no to_string() for this one:
	PRIVATE_CVAR( cg_rangeMarkerBuildableTypes );

	// Silence the Debug log for every remaining known ones.
	PRIVATE_CVAR( cg_fov_builder );
	PRIVATE_CVAR( cg_fov_human );
	PRIVATE_CVAR( cg_fov_level0 );
	PRIVATE_CVAR( cg_fov_level1 );
	PRIVATE_CVAR( cg_fov_level2 );
	PRIVATE_CVAR( cg_fov_level3 );
	PRIVATE_CVAR( cg_fov_level4 );
	PRIVATE_CVAR( cg_navgenMaxThreads );
	PRIVATE_CVAR( cg_navgenOnLoad );
	PRIVATE_CVAR( cg_welcome );
	PRIVATE_CVAR( cg_wwFollow );
	PRIVATE_CVAR( cg_wwToggle );

	DIRECT_CVAR( p_classname );
	DIRECT_CVAR( ui_errorMessage );
	DIRECT_CVAR( ui_winner );

	RML_CVAR( ui_dialogCvar1 );
	RML_CVAR( ui_dialogCvar2 );
	RML_CVAR( m_pitch );
	RML_CVAR( m_filter );

	SGAME_CVAR( g_bot_alienAimDelay );
	SGAME_CVAR( g_bot_defaultFill );
	SGAME_CVAR( g_bot_defaultSkill );
	SGAME_CVAR( g_bot_fov );
	SGAME_CVAR( g_bot_humanAimDelay );
	SGAME_CVAR( g_bot_reactiontime );
	SGAME_CVAR( g_BPBudgetPerMiner );
	SGAME_CVAR( g_BPInitialBudget );
	SGAME_CVAR( g_BPRecoveryInitialRate );
	SGAME_CVAR( g_BPRecoveryRateHalfLife );
	SGAME_CVAR( g_doWarmup );
	SGAME_CVAR( g_freeFundPeriod );
	SGAME_CVAR( g_friendlyFireAlienMultiplier );
	SGAME_CVAR( g_friendlyFireHumanMultiplier );
	SGAME_CVAR( g_gravity );
	SGAME_CVAR( g_momentumBaseMod );
	SGAME_CVAR( g_momentumBuildMod );
	SGAME_CVAR( g_momentumDeconMod );
	SGAME_CVAR( g_momentumDestroyMod );
	SGAME_CVAR( g_momentumHalfLife );
	SGAME_CVAR( g_momentumKillMod );
	SGAME_CVAR( g_momentumRewardDoubleTime );
	SGAME_CVAR( g_warmup );
	SGAME_CVAR( sv_running );
	SGAME_CVAR( g_friendlyBuildableFire );
	SGAME_CVAR( g_allowVote );
	SGAME_CVAR( g_alienAllowBuilding );
	SGAME_CVAR( g_humanAllowBuilding );
	SGAME_CVAR( g_dretchpunt );
	SGAME_CVAR( g_antiSpawnBlock );
	SGAME_CVAR( g_bot_attackStruct );
	SGAME_CVAR( g_bot_infiniteFunds );
	SGAME_CVAR( g_bot_infiniteMomentum );
	SGAME_CVAR( g_bot_buy );
	SGAME_CVAR( g_bot_ckit );
	SGAME_CVAR( g_bot_rifle );
	SGAME_CVAR( g_bot_painsaw );
	SGAME_CVAR( g_bot_shotgun );
	SGAME_CVAR( g_bot_lasgun );
	SGAME_CVAR( g_bot_mdriver );
	SGAME_CVAR( g_bot_chain );
	SGAME_CVAR( g_bot_flamer );
	SGAME_CVAR( g_bot_prifle );
	SGAME_CVAR( g_bot_lcannon );
	SGAME_CVAR( g_bot_lightarmour );
	SGAME_CVAR( g_bot_mediumarmour );
	SGAME_CVAR( g_bot_battlesuit );
	SGAME_CVAR( g_bot_firebomb );
	SGAME_CVAR( g_bot_grenade );
	SGAME_CVAR( g_bot_radar );
	SGAME_CVAR( g_bot_evolve );
	SGAME_CVAR( g_bot_level1 );
	SGAME_CVAR( g_bot_level2 );
	SGAME_CVAR( g_bot_level2upg );
	SGAME_CVAR( g_bot_level3 );
	SGAME_CVAR( g_bot_level3upg );
	SGAME_CVAR( g_bot_level4 );
	SGAME_CVAR( sv_hostname );
	SGAME_CVAR( g_password );

	ENGINE_CVAR( audio.dopplerExaggeration );
	ENGINE_CVAR( audio.reverbIntensity );
	ENGINE_CVAR( audio.volume.effects );
	ENGINE_CVAR( audio.volume.master );
	ENGINE_CVAR( audio.volume.music );
	ENGINE_CVAR( cg_shadows );
	ENGINE_CVAR( cl_freelook );
	ENGINE_CVAR( con_colorAlpha );
	ENGINE_CVAR( con_colorBlue );
	ENGINE_CVAR( con_colorGreen );
	ENGINE_CVAR( con_colorRed );
	ENGINE_CVAR( fs_extrapaks );
	ENGINE_CVAR( in_joystick );
	ENGINE_CVAR( language );
	ENGINE_CVAR( name );
	ENGINE_CVAR( r_bloom );
	ENGINE_CVAR( r_deluxeMapping );
	ENGINE_CVAR( r_dynamicLight );
	ENGINE_CVAR( r_ext_texture_filter_anisotropic );
	ENGINE_CVAR( r_fullscreen );
	ENGINE_CVAR( r_FXAA );
	ENGINE_CVAR( r_gamma );
	ENGINE_CVAR( r_halfLambertLighting );
	ENGINE_CVAR( r_heatHaze );
	ENGINE_CVAR( r_lightStyles );
	ENGINE_CVAR( r_noBorder );
	ENGINE_CVAR( r_normalMapping );
	ENGINE_CVAR( r_physicalMapping );
	ENGINE_CVAR( r_picmip );
	ENGINE_CVAR( r_reliefMapping );
	ENGINE_CVAR( r_rimLighting );
	ENGINE_CVAR( r_specularMapping );
	ENGINE_CVAR( r_ssao );
	ENGINE_CVAR( r_swapinterval );
	ENGINE_CVAR( r_textureMode );
	ENGINE_CVAR( r_vertexLighting );
	ENGINE_CVAR( sensitivity );

	Log::Warn( "Cvar used in UI without optimized call: %s", name );
#endif

	return Cvar::GetValue( name.c_str() );
}

void CG_Rocket_LoadHuds()
{
	int i;
	const char *token, *text_p;

	// Preload all the menu files...
	std::string text = FS::PakPath::ReadFile( rocket_hudFile.Get() );
	text_p = text.c_str();

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
	return AS_GLOBAL;
}

const char *CG_NetSourceToString( int netSrc )
{
	switch ( netSrc )
	{
		case AS_LOCAL:
			return "local";

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
				Rocket_DocumentAction( rocketInfo.menu[ ROCKETMENU_LOADING ].id, "show" );
				break;
			case connstate_t::CA_PRIMED:
				Rocket_DocumentAction( "", "blurall" );
				break;
			case connstate_t::CA_ACTIVE:
				// In order to preserve the welcome screen, we need to not hide all the windows
				// if the previous state was CA_PRIMED (ie, if the client is first connecting to the server),
				// however, in other instances (like vid_restart), the client will go directly from CA_LOADING
				// to CA_ACTIVE, so we need to ensure that no windows are open in that case.
				if ( oldConnState != connstate_t::CA_PRIMED )
				{
					Rocket_DocumentAction( "", "blurall" );
				}
				break;

			default:
				break;
		}

		oldConnState = rocketInfo.cstate.connState;
	}

	// Continue to attempt to update serverlisting
	if ( rocketInfo.data.retrievingServers )
	{
		CG_Rocket_BuildServerList();
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
	if ( rocketInfo.renderCursor && rocketInfo.cursor )
	{
		trap_R_ClearColor();
		if ( rocketInfo.cursorFreezeTime == rocketInfo.realtime )
		{
			trap_R_DrawStretchPic( rocketInfo.cursorFreezeX, rocketInfo.cursorFreezeY,
			                       rocketInfo.cursor_pos.w, rocketInfo.cursor_pos.h,
			                       0, 0, 1, 1, rocketInfo.cursor );
		}
		else
		{
			trap_R_DrawStretchPic( rocketInfo.cursor_pos.x, rocketInfo.cursor_pos.y,
			                       rocketInfo.cursor_pos.w, rocketInfo.cursor_pos.h,
			                       0, 0, 1, 1, rocketInfo.cursor );
		}
	}
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
				== connstate_t::CA_ACTIVE && cg.snap != nullptr;

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

		default:
			return false;
	}
}

bool CG_Rocket_LoadCursor( Str::StringRef cursorPath )
{
	rocketInfo.cursor = trap_R_RegisterShader( cursorPath.c_str(), (RegisterShaderFlags_t) RSF_DEFAULT );
	if ( rocketInfo.cursor == 0 )
	{
		return false;
	}
	// Scale cursor with resolution while maintaining the original aspect ratio.
	int x, y;
	trap_R_GetTextureSize( rocketInfo.cursor, &x, &y );
	float ratio = static_cast<float>( x ) / static_cast<float>( y );
	rocketInfo.cursor_pos.h = cgs.glconfig.vidHeight * 0.025f;
	rocketInfo.cursor_pos.w = rocketInfo.cursor_pos.h * ratio;
	return true;
}

void CG_Rocket_EnableCursor( bool enable )
{
	rocketInfo.renderCursor = enable;
}
