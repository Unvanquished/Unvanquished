/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

// cg_tutorial.c -- the tutorial system

#include "cg_key_name.h"
#include "cg_local.h"

namespace {
struct bind_t {
	const char *command;
	const char *humanName;
	std::vector<Keyboard::Key> keys;
};
} // namespace

static const char* const OPEN_CONSOLE_CMD = "toggleConsole";
static const char* const OPEN_MENU_CMD = "toggleMenu";

static bind_t bindings[] =
{
	{ "+speed",         N_( "Run/Walk" ),                              {} },
	{ "+sprint",        N_( "Sprint" ),                                {} },
	{ "+moveup",        N_( "Jump" ),                                  {} },
	{ "+movedown",      N_( "Crouch" ),                                {} },
	{ "+attack",        N_( "Primary Attack" ),                        {} },
	{ "+attack2",       N_( "Secondary Attack" ),                      {} },
	{ "+attack3",       N_( "Tertiary Attack" ),                       {} },
	{ "reload",         N_( "Reload" ),                                {} },
	{ "itemact medkit", N_( "Use Medkit" ),                            {} },
	{ "+activate",      N_( "Use Structure/Evolve" ),                  {} },
	{ "+deconstruct",   N_( "Deconstruct Structure" ),                 {} },
	{ "weapprev",       N_( "Previous Weapon" ),                       {} },
	{ "weapnext",       N_( "Next Weapon" ),                           {} },
	{ "message_public", N_( "Global chat" ),                           {} },
	{ "message_team",   N_( "Team chat" ),                             {} },
	{ OPEN_CONSOLE_CMD, N_( "Toggle Console" ),                        {} },
	{ OPEN_MENU_CMD,    N_( "Toggle Menu" ),                           {} },
	{ "itemact grenade", N_( "Throw a grenade" ),                      {} }
};

static const size_t numBindings = ARRAY_LEN( bindings );

/*
=================
CG_RefreshBindings
=================
*/
static void CG_RefreshBindings()
{
	std::vector<std::string> binds;

	for (bind_t const& bind : bindings )
	{
		binds.push_back(bind.command);
	}

	std::vector<std::vector<Keyboard::Key>> keys = trap_Key_GetKeysForBinds(CG_CurrentBindTeam(), binds);

	for (unsigned i = 0; i < numBindings; i++) {
		bindings[i].keys = keys[i];
	}
}

/*
===============
CG_KeyNameForCommand
===============
*/
static const char *CG_KeyNameForCommand( const char *command )
{
	static std::string buffer;

	bind_t const* it = std::find_if( std::begin( bindings ), std::end( bindings ),
			[&]( bind_t const& o ){ return 0 == Q_stricmp( command, o.command ); } );
	if ( it == std::end( bindings ) )
	{
		return "(⚠ BUG)"; // shouldn't happen: if it does, BUG
	}
	std::string keyNames;
	if ( it->command == OPEN_CONSOLE_CMD )
	{
		// Hard-coded console toggle key binding
		keyNames = _( "SHIFT-ESCAPE" );
		// cl_consoleKeys is yet another source of keys for toggling the console,
		// but it is omitted out of laziness.
	}
	else if ( it->command == OPEN_MENU_CMD )
	{
		// Hard-coded menu toggle key binding
		keyNames = _( "ESCAPE" );
	}

	for ( Keyboard::Key key : it->keys )
	{
		if ( keyNames.empty() )
		{
			keyNames = CG_KeyDisplayName( key );
		}
		else if ( key == it->keys.back() )
		{
			keyNames = Str::Format( _("%s or %s"), keyNames, CG_KeyDisplayName( key ) );
		}
		else
		{
			keyNames = Str::Format( _("%s, %s"), keyNames, CG_KeyDisplayName( key ) );
		}
	}

	if ( keyNames.empty() )
	{
		keyNames = Str::Format( _( "\"%s\" (unbound)" ), _( it->humanName ) );
	}
	return ( buffer = keyNames ).c_str();
}

/*
===============
CG_BuildableInRange
===============
*/
static entityState_t *CG_BuildableInRange( playerState_t *ps )
{
	vec3_t        view, point;
	trace_t       trace;

	AngleVectors( cg.refdefViewAngles, view, nullptr, nullptr );
	VectorMA( cg.refdef.vieworg, ENTITY_USE_RANGE - 0.2f, view, point );
	CG_Trace( &trace, cg.refdef.vieworg, nullptr, nullptr, point, ps->clientNum, MASK_SHOT, 0 );

	entityState_t &es = cg_entities[ trace.entityNum ].currentState;

	if ( es.eType != entityType_t::ET_BUILDABLE || !CG_IsOnMyTeam(es) )
	{
		return nullptr;
	}

	return &es;
}

#define COLOR_ALARM "^8" // Orange

/*
===============
CG_BuilderText
===============
*/
static void CG_BuilderText( std::string& text, playerState_t *ps )
{
	buildable_t   buildable = (buildable_t) ( ps->stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK );
	entityState_t *es;

	if ( buildable > BA_NONE )
	{
		const char *item = _( BG_Buildable( buildable )->humanName );
		text += va( _( "Press %s to place the %s."), CG_KeyNameForCommand( "+attack" ), item );
		text += '\n';

		text += va( _( "Press %s to cancel placing the %s." ), CG_KeyNameForCommand( "+attack2" ), item );
		text += '\n';
	}
	else
	{
		text += va( _( "Press %s to build a structure." ), CG_KeyNameForCommand( "+attack" ) );
		text += '\n';
	}

	if ( ( es = CG_BuildableInRange( ps ) ) )
	{
		const char *key = CG_KeyNameForCommand( "+deconstruct" );

		if ( es->eFlags & EF_B_MARKED )
		{
			text += va( _( "Press %s to unmark this structure for replacement." ), key );
			text += '\n';
		}
		else
		{
			text += va( _( "Press %s to mark this structure for replacement." ), key );
			text += '\n';
		}

		text += va( _( "Hold %s to deconstruct this structure." ), key );
		text += '\n';
	}
}

/*
===============
CG_AlienBuilderText
===============
*/
static void CG_AlienBuilderText( std::string& text, playerState_t *ps )
{
	CG_BuilderText( text, ps );

	if ( ( ps->stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK ) == BA_NONE )
	{
		text += va( _( "Press %s to swipe." ), CG_KeyNameForCommand( "+attack2" ) );
		text += '\n';
	}

	if ( ps->stats[ STAT_CLASS ] == PCL_ALIEN_BUILDER0_UPG )
	{
		text += va( _( "Press %s to spit. Spit can slow humans and remove fire." ), CG_KeyNameForCommand( "+attack3" ) );
		text += '\n';

		text += va( _( "Press %s to walk on walls." ), CG_KeyNameForCommand( "+movedown" ) );
		text += '\n';
	}
}

/*
===============
CG_AlienLevel0Text
===============
*/
static void CG_AlienLevel0Text( std::string& text, playerState_t *ps )
{
	Q_UNUSED(ps);

	text += _( "Touch humans to damage them." );
	text += '\n';

	text += _( "Aim at their heads to cause more damage." );
	text += '\n';

	text += va( _( "Press %s to walk on walls." ), CG_KeyNameForCommand( "+movedown" ) );
	text += '\n';
}

/*
===============
CG_AlienLevel1Text
===============
*/
static void CG_AlienLevel1Text( std::string& text )
{
	text += va( _( "Press %s to swipe. Swiping buildings corrodes them." ), CG_KeyNameForCommand( "+attack" ) );
	text += '\n';

	text += va( _( "Press %s to lunge." ), CG_KeyNameForCommand( "+attack2" ) );
	text += '\n';

	text += va( _( "Press %s to walk on walls." ), CG_KeyNameForCommand( "+movedown" ) );
	text += '\n';
}

/*
===============
CG_AlienLevel2Text
===============
*/
static void CG_AlienLevel2Text( std::string& text, playerState_t *ps )
{
	text += va( _( "Press %s to bite." ), CG_KeyNameForCommand( "+attack" ) );
	text += '\n';

	if ( ps->stats[ STAT_CLASS ] == PCL_ALIEN_LEVEL2_UPG )
	{
		text += va( _( "Press %s to invoke an electrical attack." ), CG_KeyNameForCommand( "+attack2" ) );
		text += '\n';
	}

	text += va( _( "Hold down %s then touch a wall to wall jump." ), CG_KeyNameForCommand( "+moveup" ) );
	text += '\n';
}

/*
===============
CG_AlienLevel3Text
===============
*/
static void CG_AlienLevel3Text( std::string& text, playerState_t *ps )
{
	text += va( _( "Press %s to bite." ), CG_KeyNameForCommand( "+attack" ) );
	text += '\n';

	if ( ps->stats[ STAT_CLASS ] == PCL_ALIEN_LEVEL3_UPG )
	{
		text += va( _( "Press %s to launch a barb." ), CG_KeyNameForCommand( "+attack3" ) );
		text += '\n';
	}

	text += va( _( "Hold down and release %s to pounce." ), CG_KeyNameForCommand( "+attack2" ) );
	text += '\n';
}

/*
===============
CG_AlienLevel4Text
===============
*/
static void CG_AlienLevel4Text( std::string& text, playerState_t *ps )
{
	Q_UNUSED(ps);

	text += va( _( "Press %s to swipe." ), CG_KeyNameForCommand( "+attack" ) );
	text += '\n';

	text += va( _( "Hold down %s while moving forwards to trample." ), CG_KeyNameForCommand( "+attack2" ) );
	text += '\n';
}

/*
===============
CG_HumanCkitText
===============
*/
static void CG_HumanCkitText( std::string& text, playerState_t *ps )
{
	CG_BuilderText( text, ps );
}

/*
===============
CG_HumanText
===============
*/
static void CG_HumanText( std::string& text, playerState_t *ps )
{
	if ( !ps->ammo && !ps->clips && !BG_Weapon( ps->weapon )->infiniteAmmo )
	{
		// Need to resupply ammo
		text += COLOR_ALARM;
		if ( BG_Weapon( ps->weapon )->usesEnergy )
		{
			text += _( "Find an Armoury, Drill or Reactor for more ammo." );
			text += '\n';
		}
		else
		{
			text += _( "Find an Armoury for more ammo." );
			text += '\n';
		}
	}
	else
	{
		switch ( ps->weapon )
		{
			case WP_BLASTER:
			case WP_MACHINEGUN:
			case WP_SHOTGUN:
			case WP_LAS_GUN:
			case WP_CHAINGUN:
			case WP_PULSE_RIFLE:
			case WP_FLAMER:
				text += va( _( "Press %s to fire the %s." ), CG_KeyNameForCommand( "+attack" ), _( BG_Weapon( ps->weapon )->humanName ) );
				text += '\n';
				break;

			case WP_MASS_DRIVER:
				text += va( _( "Press %s to fire the %s." ), CG_KeyNameForCommand( "+attack" ), _( BG_Weapon( ps->weapon )->humanName ) );
				text += '\n';

				text += va( _( "Hold %s to zoom." ), CG_KeyNameForCommand( "+attack2" ) );
				text += '\n';
				break;

			case WP_PAIN_SAW:
				text += va( _( "Hold %s to activate the %s." ), CG_KeyNameForCommand( "+attack" ), _( BG_Weapon( ps->weapon )->humanName ) );
				text += '\n';
				break;

			case WP_LUCIFER_CANNON:
				text += va( _( "Hold and release %s to fire a charged shot." ), CG_KeyNameForCommand( "+attack" ) );
				text += '\n';

				text += va( _( "Press %s to fire the %s." ), CG_KeyNameForCommand( "+attack2" ), _( BG_Weapon( ps->weapon )->humanName ) );
				text += '\n';
				break;

			case WP_HBUILD:
				CG_HumanCkitText( text, ps );
				break;

			default:
				break;
		}
	}

	upgrade_t upgrade = UP_NONE;
	for ( const auto u : { UP_GRENADE, UP_FIREBOMB } )
	{
		if ( BG_InventoryContainsUpgrade( u, ps->stats ) )
		{
			upgrade = u;
		}
	}

	if ( upgrade != UP_NONE )
	{
		text += va( _( "Press %s to throw the %s." ), CG_KeyNameForCommand( "itemact grenade" ), _( BG_Upgrade( upgrade )->humanName ) );
		text += '\n';
	}

	// Find next weapon in inventory.
	weapon_t nextWeapon = CG_FindNextWeapon( ps );

	if ( nextWeapon != WP_NONE )
	{
		text += va( _( "Press %s to use the %s." ), CG_KeyNameForCommand( "weapnext" ), _( BG_Weapon( nextWeapon )->humanName ) );
		text += '\n';
	}

	if ( BG_InventoryContainsUpgrade( UP_JETPACK, ps->stats ) )
	{
		text += va( _( "Hold %s to use the Jetpack." ), CG_KeyNameForCommand( "+moveup" ) );
		text += '\n';
	}

	if ( ps->stats[ STAT_HEALTH ] <= 35 &&
	     BG_InventoryContainsUpgrade( UP_MEDKIT, ps->stats ) )
	{
		text += COLOR_ALARM;
		text += va( _( "Press %s to use your Medkit." ), CG_KeyNameForCommand( "itemact medkit" ) );
		text += '\n';
	}

	switch ( cg.nearUsableBuildable )
	{
		case BA_H_ARMOURY:
			text += va( _( "Press %s to buy equipment upgrades at the Armoury." ), CG_KeyNameForCommand( "+activate" ) );
			text += '\n';
			break;

		case BA_NONE:
			break;

		default:
			text += va( _( "Press %s to use the %s." ), CG_KeyNameForCommand( "+activate" ), _( BG_Buildable( cg.nearUsableBuildable )->humanName ) );
			text += '\n';
			break;
	}

	text += va( _( "Press %s and any direction to sprint." ), CG_KeyNameForCommand( "+sprint" ) );
	text += '\n';

	text += va( _( "Press %s to crouch." ), CG_KeyNameForCommand( "+movedown" ) );
	text += '\n';
}

/*
===============
CG_SpectatorText
===============
*/
static void CG_SpectatorText( std::string& text, playerState_t *ps )
{
	if ( cgs.clientinfo[ cg.clientNum ].team != TEAM_NONE )
	{
		if ( ps->pm_flags & PMF_QUEUED )
		{
			text += va( _( "Press %s to leave spawn queue." ), CG_KeyNameForCommand( "+attack" ) );
			text += '\n';
		}
		else
		{
			text += va( _( "Press %s to spawn." ), CG_KeyNameForCommand( "+attack" ) );
			text += '\n';
		}
	}
	else
	{
		text += va( _( "Press %s to join a team." ), CG_KeyNameForCommand( "+attack" ) );
		text += '\n';
	}

	if ( ps->pm_flags & PMF_FOLLOW )
	{
		text += va( _( "Press %s to stop following." ), CG_KeyNameForCommand( "+attack2" ) );
		text += '\n';

		if ( !cg.chaseFollow )
		{
			text += va( _( "Press %s to switch to chase-cam spectator mode." ), CG_KeyNameForCommand( "+attack3" ) );
			text += '\n';
		}
		else
		{
			text += va( _( "Press %s to return to first-person spectator mode." ), CG_KeyNameForCommand( "+attack3" ) );
			text += '\n';
		}

		text += va( _( "Press %s to follow the next player." ), CG_KeyNameForCommand( "weapnext" ) );
		text += '\n';
	}
	else
	{
		text += va( _( "Press %s to follow a player." ), CG_KeyNameForCommand( "+attack2" ) );
		text += '\n';
	}
}

#define BINDING_REFRESH_INTERVAL 30

/*
===============
CG_TutorialText

Returns context help for the current class/weapon
===============
*/
const std::string& CG_TutorialText()
{
	playerState_t *ps;
	static std::string text;
	static int    refreshBindings = 0;

	text.clear();
	ps = &cg.snap->ps;

	if ( refreshBindings == 0 )
	{
		CG_RefreshBindings();
	}

	refreshBindings = ( refreshBindings + 1 ) % BINDING_REFRESH_INTERVAL;

	if ( !cg.intermissionStarted && !cg.demoPlayback )
	{
		if ( ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT ||
		     ps->pm_flags & PMF_FOLLOW )
		{
			CG_SpectatorText( text, ps );
		}
		else if ( ps->stats[ STAT_HEALTH ] > 0 )
		{
			switch ( ps->stats[ STAT_CLASS ] )
			{
				case PCL_ALIEN_BUILDER0:
				case PCL_ALIEN_BUILDER0_UPG:
					CG_AlienBuilderText( text, ps );
					break;

				case PCL_ALIEN_LEVEL0:
					CG_AlienLevel0Text( text, ps );
					break;

				case PCL_ALIEN_LEVEL1:
				case PCL_ALIEN_LEVEL1_UPG:
					CG_AlienLevel1Text( text );
					break;

				case PCL_ALIEN_LEVEL2:
				case PCL_ALIEN_LEVEL2_UPG:
					CG_AlienLevel2Text( text, ps );
					break;

				case PCL_ALIEN_LEVEL3:
				case PCL_ALIEN_LEVEL3_UPG:
					CG_AlienLevel3Text( text, ps );
					break;

				case PCL_ALIEN_LEVEL4:
					CG_AlienLevel4Text( text, ps );
					break;

				case PCL_HUMAN_NAKED:
				case PCL_HUMAN_LIGHT:
				case PCL_HUMAN_MEDIUM:
				case PCL_HUMAN_BSUIT:
					CG_HumanText( text, ps );
					break;

				default:
					break;
			}

			if ( ps->persistant[ PERS_TEAM ] == TEAM_ALIENS )
			{
				text += va( _( "Press %s to evolve." ), CG_KeyNameForCommand( "+activate" ) );
				text += '\n';
			}
		}
	}
	else if ( !cg.demoPlayback )
	{
		if ( !CG_ClientIsReady( ps->clientNum ) )
		{
			text += va( _( "Press %s when ready to continue." ), CG_KeyNameForCommand( "+attack" ) );
		}
		else
		{
			text += _( "Waiting for other players to be ready." );
		}
		text += '\n';
	}

	if ( !cg.demoPlayback )
	{
		text += '\n';
		text += va( _( "Press %s to chat with all players." ), CG_KeyNameForCommand( "message_public" ) );
		text += '\n';
		text += va( _( "Press %s to chat with your team." ), CG_KeyNameForCommand( "message_team" ) );
		text += '\n';
		text += va( _( "Press %s to open the console." ), CG_KeyNameForCommand( "toggleConsole" ) );
		text += '\n';
		text += va( _( "Press %s for the menu." ), CG_KeyNameForCommand( "toggleMenu" ) );
	}

	return text;
}
