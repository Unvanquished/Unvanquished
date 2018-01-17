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

#include "cg_local.h"

typedef struct
{
	const char *command;
	const char *humanName;
	int keys[ 2 ];
} bind_t;

static bind_t bindings[] =
{
	{ "+useitem",       N_( "Activate Upgrade" ),                      { -1, -1 } },
	{ "+speed",         N_( "Run/Walk" ),                              { -1, -1 } },
	{ "+sprint",        N_( "Sprint" ),                                { -1, -1 } },
	{ "+moveup",        N_( "Jump" ),                                  { -1, -1 } },
	{ "+movedown",      N_( "Crouch" ),                                { -1, -1 } },
	{ "+attack",        N_( "Primary Attack" ),                        { -1, -1 } },
	{ "+attack2",       N_( "Secondary Attack" ),                      { -1, -1 } },
	{ "reload",         N_( "Reload" ),                                { -1, -1 } },
	{ "buy ammo",       N_( "Buy Ammo" ),                              { -1, -1 } },
	{ "itemact medkit", N_( "Use Medkit" ),                            { -1, -1 } },
	{ "+activate",      N_( "Use Structure/Evolve" ),                  { -1, -1 } },
	{ "modcase alt \"/deconstruct marked\" /deconstruct",
                            N_( "Deconstruct Structure" ),                 { -1, -1 } },
	{ "weapprev",       N_( "Previous Upgrade" ),                      { -1, -1 } },
	{ "weapnext",       N_( "Next Upgrade" ),                          { -1, -1 } },
	{ "toggleconsole",  N_( "Toggle Console" ),                        { -1, -1 } },
	{ "itemact grenade", N_( "Throw a grenade" ),                      { -1, -1 } }
};

static const size_t numBindings = ARRAY_LEN( bindings );

/*
=================
CG_GetBindings
=================
*/
static void CG_GetBindings( team_t team )
{
    std::vector<std::string> binds;

    for (unsigned i = 0; i < numBindings; i++) {
		bindings[i].keys[0] = bindings[i].keys[1] = K_NONE;
        binds.push_back(bindings[i].command);
    }

    std::vector<std::vector<int>> keyNums = trap_Key_GetKeynumForBinds(team, binds);

    for (unsigned i = 0; i < numBindings; i++) {
        if (keyNums[i].size() > 0) {
            bindings[i].keys[0] = keyNums[i][0];
        }
        if (keyNums[i].size() > 1) {
            bindings[i].keys[1] = keyNums[i][1];
        }
    }
}

/*
===============
CG_KeyNameForCommand
===============
*/
static const char *CG_KeyNameForCommand( const char *command )
{
	unsigned    i;
	static char buffer[ 2 ][ MAX_STRING_CHARS ];
	static int  which = 1;
	char        keyName[ 2 ][ 32 ];

	which ^= 1;

	buffer[ which ][ 0 ] = '\0';

	for ( i = 0; i < numBindings; i++ )
	{
		if ( !Q_stricmp( command, bindings[ i ].command ) )
		{
			if ( bindings[ i ].keys[ 0 ] != K_NONE )
			{
				trap_Key_KeynumToStringBuf( bindings[ i ].keys[ 0 ],
				                            keyName[ 0 ], sizeof( keyName[ 0 ] ) );

				if ( bindings[ i ].keys[ 1 ] != K_NONE )
				{
					trap_Key_KeynumToStringBuf( bindings[ i ].keys[ 1 ],
					                            keyName[ 1 ], sizeof( keyName[ 1 ] ) );
					Q_snprintf( buffer[ which ], sizeof( buffer[ 0 ] ), _("%s or %s"),
					            Q_strupr( keyName[ 0 ] ), Q_strupr( keyName[ 1 ] ) );
				}
				else
				{
					Q_strncpyz( buffer[ which ], Q_strupr( keyName[ 0 ] ), sizeof( buffer[ 0 ] ) );
				}
			}
			else
			{
				Com_sprintf( buffer[ which ], MAX_STRING_CHARS, _( "\"%s\" (unbound)" ),
				             _( bindings[ i ].humanName ) );
			}

			return buffer[ which ];
		}
	}

	return "(⚠ BUG)"; // shouldn't happen: if it does, BUG
}

#define MAX_TUTORIAL_TEXT 4096

/*
===============
CG_BuildableInRange
===============
*/
static entityState_t *CG_BuildableInRange( playerState_t *ps, float *healthFraction )
{
	vec3_t        view, point;
	trace_t       trace;
	entityState_t *es;
	int           health;

	AngleVectors( cg.refdefViewAngles, view, nullptr, nullptr );
	VectorMA( cg.refdef.vieworg, 64, view, point );
	CG_Trace( &trace, cg.refdef.vieworg, nullptr, nullptr, point, ps->clientNum, MASK_SHOT, 0 );

	es = &cg_entities[ trace.entityNum ].currentState;

	if ( healthFraction )
	{
		health = es->generic1;
		*healthFraction = ( float ) health / BG_Buildable( es->modelindex )->health;
	}

	if ( es->eType == entityType_t::ET_BUILDABLE &&
	     ps->persistant[ PERS_TEAM ] == BG_Buildable( es->modelindex )->team )
	{
		return es;
	}
	else
	{
		return nullptr;
	}
}

/*
===============
CG_BuilderText
===============
*/
static void CG_BuilderText( char *text, playerState_t *ps )
{
	buildable_t   buildable = (buildable_t) ( ps->stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK );
	entityState_t *es;

	if ( buildable > BA_NONE )
	{
		const char *item = _( BG_Buildable( buildable )->humanName );
		Q_strcat( text, MAX_TUTORIAL_TEXT,
		          va( _( "Press %s to place the %s\n"),
		              CG_KeyNameForCommand( "+attack" ), item ) );
		Q_strcat( text, MAX_TUTORIAL_TEXT,
		          va( _( "Press %s to cancel placing the %s\n" ),
		              CG_KeyNameForCommand( "+attack2" ), item ) );
	}
	else
	{
		Q_strcat( text, MAX_TUTORIAL_TEXT,
		          va( _( "Press %s to build a structure\n" ),
		              CG_KeyNameForCommand( "+attack" ) ) );
	}

	if ( ( es = CG_BuildableInRange( ps, nullptr ) ) )
	{
		const char *key = CG_KeyNameForCommand( "modcase alt \"/deconstruct marked\" /deconstruct" );

		if ( es->eFlags & EF_B_MARKED )
		{
			Q_strcat( text, MAX_TUTORIAL_TEXT,
					  va( _( "Press %s to unmark this structure for replacement\n" ), key ) );
		}
		else
		{
			Q_strcat( text, MAX_TUTORIAL_TEXT,
					  va( _( "Press %s to mark this structure for replacement\n" ), key ) );
		}
	}
}

/*
===============
CG_AlienBuilderText
===============
*/
static void CG_AlienBuilderText( char *text, playerState_t *ps )
{
	CG_BuilderText( text, ps );

	if ( ( ps->stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK ) == BA_NONE )
	{
		Q_strcat( text, MAX_TUTORIAL_TEXT,
		          va( _( "Press %s to swipe\n" ),
		              CG_KeyNameForCommand( "+attack2" ) ) );
	}

	if ( ps->stats[ STAT_CLASS ] == PCL_ALIEN_BUILDER0_UPG )
	{
		Q_strcat( text, MAX_TUTORIAL_TEXT,
		          va( _( "Press %s to launch a projectile\n" ),
		              CG_KeyNameForCommand( "+useitem" ) ) );

		Q_strcat( text, MAX_TUTORIAL_TEXT,
		          va( _( "Press %s to walk on walls\n" ),
		              CG_KeyNameForCommand( "+movedown" ) ) );
	}
}

/*
===============
CG_AlienLevel0Text
===============
*/
static void CG_AlienLevel0Text( char *text, playerState_t *ps )
{
	Q_UNUSED(ps);

	Q_strcat( text, MAX_TUTORIAL_TEXT,
	          _( "Touch humans to damage them\n"
	             "Aim at their heads to cause more damage\n" ) );

	Q_strcat( text, MAX_TUTORIAL_TEXT,
	          va( _( "Press %s to walk on walls\n" ),
	              CG_KeyNameForCommand( "+movedown" ) ) );
}

/*
===============
CG_AlienLevel1Text
===============
*/
static void CG_AlienLevel1Text( char *text )
{
	Q_strcat( text, MAX_TUTORIAL_TEXT,
	          va( _( "Press %s to swipe\n" ),
	              CG_KeyNameForCommand( "+attack" ) ) );

	Q_strcat( text, MAX_TUTORIAL_TEXT,
	          va( _( "Press %s to walk on walls\n" ),
	              CG_KeyNameForCommand( "+movedown" ) ) );
}

/*
===============
CG_AlienLevel2Text
===============
*/
static void CG_AlienLevel2Text( char *text, playerState_t *ps )
{
	Q_strcat( text, MAX_TUTORIAL_TEXT,
	          va( _( "Press %s to bite\n" ),
	              CG_KeyNameForCommand( "+attack" ) ) );

	if ( ps->stats[ STAT_CLASS ] == PCL_ALIEN_LEVEL2_UPG )
	{
		Q_strcat( text, MAX_TUTORIAL_TEXT,
		          va( _( "Press %s to invoke an electrical attack\n" ),
		              CG_KeyNameForCommand( "+attack2" ) ) );
	}

	Q_strcat( text, MAX_TUTORIAL_TEXT,
	          va( _( "Hold down %s then touch a wall to wall jump\n" ),
	              CG_KeyNameForCommand( "+moveup" ) ) );
}

/*
===============
CG_AlienLevel3Text
===============
*/
static void CG_AlienLevel3Text( char *text, playerState_t *ps )
{
	Q_strcat( text, MAX_TUTORIAL_TEXT,
	          va( _( "Press %s to bite\n" ),
	              CG_KeyNameForCommand( "+attack" ) ) );

	if ( ps->stats[ STAT_CLASS ] == PCL_ALIEN_LEVEL3_UPG )
	{
		Q_strcat( text, MAX_TUTORIAL_TEXT,
		          va( _( "Press %s to launch a projectile\n" ),
		              CG_KeyNameForCommand( "+useitem" ) ) );
	}

	Q_strcat( text, MAX_TUTORIAL_TEXT,
	          va( _( "Hold down and release %s to pounce\n" ),
	              CG_KeyNameForCommand( "+attack2" ) ) );
}

/*
===============
CG_AlienLevel4Text
===============
*/
static void CG_AlienLevel4Text( char *text, playerState_t *ps )
{
	Q_UNUSED(ps);

	Q_strcat( text, MAX_TUTORIAL_TEXT,
	          va( _( "Press %s to swipe\n" ),
	              CG_KeyNameForCommand( "+attack" ) ) );

	Q_strcat( text, MAX_TUTORIAL_TEXT,
	          va( _( "Hold down and release %s while moving forwards to trample\n" ),
	              CG_KeyNameForCommand( "+attack2" ) ) );
}

/*
===============
CG_HumanCkitText
===============
*/
static void CG_HumanCkitText( char *text, playerState_t *ps )
{
	CG_BuilderText( text, ps );
}

/*
===============
CG_HumanText
===============
*/
static void CG_HumanText( char *text, playerState_t *ps )
{
	const char *name;
	upgrade_t upgrade = UP_NONE;

	if ( cg.weaponSelect < 32 )
	{
		name = cg_weapons[ cg.weaponSelect ].humanName;
	}
	else
	{
		name = cg_upgrades[ cg.weaponSelect - 32 ].humanName;
		upgrade = (upgrade_t) ( cg.weaponSelect - 32 );
	}

	if ( !ps->ammo && !ps->clips && !BG_Weapon( ps->weapon )->infiniteAmmo )
	{
		//no ammo
		switch ( ps->weapon )
		{
			case WP_MACHINEGUN:
			case WP_CHAINGUN:
			case WP_SHOTGUN:
			case WP_FLAMER:
				Q_strcat( text, MAX_TUTORIAL_TEXT,
				          _( "Find an Armoury for more ammo\n" ) );
				break;

			case WP_LAS_GUN:
			case WP_PULSE_RIFLE:
			case WP_MASS_DRIVER:
			case WP_LUCIFER_CANNON:
				Q_strcat( text, MAX_TUTORIAL_TEXT,
				          _( "Find an Armoury or Reactor for more ammo\n" ) );
				break;

			default:
				break;
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
				Q_strcat( text, MAX_TUTORIAL_TEXT,
				          va( _( "Press %s to fire the %s\n" ),
				              CG_KeyNameForCommand( "+attack" ),
				              _( BG_Weapon( ps->weapon )->humanName ) ) );
				break;

			case WP_MASS_DRIVER:
				Q_strcat( text, MAX_TUTORIAL_TEXT,
				          va( _( "Press %s to fire the %s\n" ),
				              CG_KeyNameForCommand( "+attack" ),
				              _( BG_Weapon( ps->weapon )->humanName ) ) );

				Q_strcat( text, MAX_TUTORIAL_TEXT,
				          va( _( "Hold %s to zoom\n" ),
				              CG_KeyNameForCommand( "+attack2" ) ) );
				break;

			case WP_PAIN_SAW:
				Q_strcat( text, MAX_TUTORIAL_TEXT,
				          va( _( "Hold %s to activate the %s\n" ),
				              CG_KeyNameForCommand( "+attack" ),
				              _( BG_Weapon( ps->weapon )->humanName ) ) );
				break;

			case WP_LUCIFER_CANNON:
				Q_strcat( text, MAX_TUTORIAL_TEXT,
				          va( _( "Hold and release %s to fire a charged shot\n" ),
				              CG_KeyNameForCommand( "+attack" ) ) );

				Q_strcat( text, MAX_TUTORIAL_TEXT,
				          va( _( "Press %s to fire the %s\n" ),
				              CG_KeyNameForCommand( "+attack2" ),
				              _( BG_Weapon( ps->weapon )->humanName ) ) );
				break;

			case WP_HBUILD:
				CG_HumanCkitText( text, ps );
				break;

			default:
				break;
		}
	}

	if ( upgrade == UP_NONE ||
	     ( upgrade > UP_NONE && BG_Upgrade( upgrade )->usable ) )
	{
		Q_strcat( text, MAX_TUTORIAL_TEXT,
		          va( _( "Press %s to use the %s\n" ),
		              CG_KeyNameForCommand( "+useitem" ),
		              name ) );
	}

	if ( ps->stats[ STAT_HEALTH ] <= 35 &&
	     BG_InventoryContainsUpgrade( UP_MEDKIT, ps->stats ) )
	{
		Q_strcat( text, MAX_TUTORIAL_TEXT,
		          va( _( "Press %s to use your %s\n" ),
		              CG_KeyNameForCommand( "itemact medkit" ),
		              _( BG_Upgrade( UP_MEDKIT )->humanName ) ) );
	}

	switch ( cg.nearUsableBuildable )
	{
		case BA_H_ARMOURY:
			Q_strcat( text, MAX_TUTORIAL_TEXT,
			          va( _( "Press %s to buy equipment upgrades at the %s\n" ),
			              CG_KeyNameForCommand( "+activate" ),
			              _( BG_Buildable( cg.nearUsableBuildable )->humanName ) ) );
			break;

		case BA_NONE:
			break;

		default:
			Q_strcat( text, MAX_TUTORIAL_TEXT,
			          va( _( "Press %s to use the %s\n" ),
			              CG_KeyNameForCommand( "+activate" ),
			              _( BG_Buildable( cg.nearUsableBuildable )->humanName ) ) );
			break;
	}

	Q_strcat( text, MAX_TUTORIAL_TEXT,
	          va( _( "Press %s and any direction to sprint\n" ),
	              CG_KeyNameForCommand( "+sprint" ) ) );

	if ( BG_InventoryContainsUpgrade( UP_FIREBOMB, ps->stats ) ||
		BG_InventoryContainsUpgrade( UP_GRENADE, ps->stats ) )
	{
		Q_strcat( text, MAX_TUTORIAL_TEXT, va( _( "Press %s to throw a grenade\n" ),
			CG_KeyNameForCommand( "itemact grenade" )
		));
	}
}

/*
===============
CG_SpectatorText
===============
*/
static void CG_SpectatorText( char *text, playerState_t *ps )
{
	if ( cgs.clientinfo[ cg.clientNum ].team != TEAM_NONE )
	{
		if ( ps->pm_flags & PMF_QUEUED )
		{
			Q_strcat( text, MAX_TUTORIAL_TEXT,
			          va( _( "Press %s to leave spawn queue\n" ),
			              CG_KeyNameForCommand( "+attack" ) ) );
		}
		else
		{
			Q_strcat( text, MAX_TUTORIAL_TEXT,
			          va( _( "Press %s to spawn\n" ),
			              CG_KeyNameForCommand( "+attack" ) ) );
		}
	}
	else
	{
		Q_strcat( text, MAX_TUTORIAL_TEXT,
		          va( _( "Press %s to join a team\n" ),
		              CG_KeyNameForCommand( "+attack" ) ) );
	}

	if ( ps->pm_flags & PMF_FOLLOW )
	{
		if ( !cg.chaseFollow )
		{
			Q_strcat( text, MAX_TUTORIAL_TEXT,
			          va( _( "Press %s to switch to chase-cam spectator mode\n" ),
			              CG_KeyNameForCommand( "+useitem" ) ) );
		}
		else if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_NONE )
		{
			Q_strcat( text, MAX_TUTORIAL_TEXT,
			          va( _( "Press %s to return to free spectator mode\n" ),
			              CG_KeyNameForCommand( "+useitem" ) ) );
		}
		else
		{
			Q_strcat( text, MAX_TUTORIAL_TEXT,
			          va( _( "Press %s to stop following\n" ),
			              CG_KeyNameForCommand( "+useitem" ) ) );
		}
	}
	else
	{
		Q_strcat( text, MAX_TUTORIAL_TEXT,
		          va( _( "Press %s to follow a player\n" ),
		              CG_KeyNameForCommand( "+useitem" ) ) );
	}
}

#define BINDING_REFRESH_INTERVAL 30

/*
===============
CG_TutorialText

Returns context help for the current class/weapon
===============
*/
const char *CG_TutorialText()
{
	playerState_t *ps;
	static char   text[ MAX_TUTORIAL_TEXT ];
	static int    refreshBindings = 0;

	text[ 0 ] = '\0';
	ps = &cg.snap->ps;

	if ( refreshBindings == 0 )
	{
		CG_GetBindings( (team_t) ps->persistant[ PERS_TEAM ] );
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
				if ( BG_AlienCanEvolve( ps->stats[ STAT_CLASS ], ps->persistant[ PERS_CREDIT ] ) )
				{
					Q_strcat( text, MAX_TUTORIAL_TEXT,
					          va( _( "Press %s to evolve\n" ),
					              CG_KeyNameForCommand( "+activate" ) ) );
				}
			}
		}
	}
	else if ( !cg.demoPlayback )
	{
		if ( !CG_ClientIsReady( ps->clientNum ) )
		{
			Q_strcat( text, MAX_TUTORIAL_TEXT,
			          va( _( "Press %s when ready to continue\n" ),
			              CG_KeyNameForCommand( "+attack" ) ) );
		}
		else
		{
			Q_strcat( text, MAX_TUTORIAL_TEXT, _( "Waiting for other players to be ready\n" ) );
		}
	}

	if ( !cg.demoPlayback )
	{
		Q_strcat( text, MAX_TUTORIAL_TEXT, va( _( "Press %s to open the console\n" ), CG_KeyNameForCommand( "toggleconsole" ) ) );
		Q_strcat( text, MAX_TUTORIAL_TEXT, _( "Press ESC for the menu" ) );
	}

	return text;
}
