/*
===========================================================================
Copyright (C) 2000-2006 Tim Angus

This file is part of OpenWolf.

OpenWolf is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenWolf is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// cg_tutorial.c -- the tutorial system

#include "cg_local.h"

typedef struct
{
  char      *command;
  char      *humanName;
  keyNum_t  keys[ 2 ];
} bind_t;

static bind_t bindings[ ] =
{
  { "+button2",       "Activate Upgrade",       { -1, -1 } },
  { "+speed",         "Run/Walk",               { -1, -1 } },
  { "boost",          "Sprint",                 { -1, -1 } },
  { "+moveup",        "Jump",                   { -1, -1 } },
  { "+movedown",      "Crouch",                 { -1, -1 } },
  { "+zoom",          "ZoomView",               { -1, -1 } },
  { "+attack",        "Primary Attack",         { -1, -1 } },
  { "+button5",       "Secondary Attack",       { -1, -1 } },
  { "reload",         "Reload",                 { -1, -1 } },
  { "buy ammo",       "Buy Ammo",               { -1, -1 } },
  { "itemact medkit", "Use Medkit",             { -1, -1 } },
  { "+button7",       "Use Structure/Evolve",   { -1, -1 } },
  { "deconstruct",    "Deconstruct Structure",  { -1, -1 } },
  { "weapprev",       "Previous Upgrade",       { -1, -1 } },
  { "weapnext",       "Next Upgrade",           { -1, -1 } }
};

static const int numBindings = sizeof( bindings ) / sizeof( bind_t );

/*
=================
CG_GetBindings
=================
*/
static void CG_GetBindings( void )
{
  int   i, j, numKeys;
  char  buffer[ MAX_STRING_CHARS ];

  for( i = 0; i < numBindings; i++ )
  {
    bindings[ i ].keys[ 0 ] = bindings[ i ].keys[ 1 ] = K_NONE;
    numKeys = 0;

    for( j = 0; j < K_LAST_KEY; j++ )
    {
      trap_Key_GetBindingBuf( j, buffer, MAX_STRING_CHARS );

      if( buffer[ 0 ] == 0 )
        continue;

      if( !Q_stricmp( buffer, bindings[ i ].command ) )
      {
        bindings[ i ].keys[ numKeys++ ] = j;

        if( numKeys > 1 )
          break;
      }
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
  int         i, j;
  static char buffer[ MAX_STRING_CHARS ];
  int         firstKeyLength;

  buffer[ 0 ] = '\0';

  for( i = 0; i < numBindings; i++ )
  {
    if( !Q_stricmp( command, bindings[ i ].command ) )
    {
      if( bindings[ i ].keys[ 0 ] != K_NONE )
      {
        trap_Key_KeynumToStringBuf( bindings[ i ].keys[ 0 ],
            buffer, MAX_STRING_CHARS );
        firstKeyLength = strlen( buffer );

        for( j = 0; j < firstKeyLength; j++ )
          buffer[ j ] = toupper( buffer[ j ] );

        if( bindings[ i ].keys[ 1 ] != K_NONE )
        {
          Q_strcat( buffer, MAX_STRING_CHARS, " or " );
          trap_Key_KeynumToStringBuf( bindings[ i ].keys[ 1 ],
              buffer + strlen( buffer ), MAX_STRING_CHARS - strlen( buffer ) );

          for( j = firstKeyLength + 4; j < strlen( buffer ); j++ )
            buffer[ j ] = toupper( buffer[ j ] );
        }
      }
      else
      {
        Q_strncpyz( buffer, va( "\"%s\"", bindings[ i ].humanName ),
            MAX_STRING_CHARS );
        Q_strcat( buffer, MAX_STRING_CHARS, " (unbound)" );
      }

      return buffer;
    }
  }

  return "";
}

#define MAX_TUTORIAL_TEXT 4096

/*
===============
CG_BuildableInRange
===============
*/
static qboolean CG_BuildableInRange( playerState_t *ps )
{
  vec3_t        view, point;
  trace_t       trace;
  entityState_t *es;

  AngleVectors( cg.refdefViewAngles, view, NULL, NULL );
  VectorMA( cg.refdef.vieworg, 64, view, point );
  CG_Trace( &trace, cg.refdef.vieworg, NULL, NULL,
            point, ps->clientNum, MASK_SHOT );

  es = &cg_entities[ trace.entityNum ].currentState;

  if( es->eType == ET_BUILDABLE &&
      ps->stats[ STAT_PTEAM ] == BG_FindTeamForBuildable( es->modelindex ) )
    return qtrue;
  else
    return qfalse;
}

/*
===============
CG_AlienBuilderText
===============
*/
static void CG_AlienBuilderText( char *text, playerState_t *ps )
{
  buildable_t buildable = ps->stats[ STAT_BUILDABLE ] & ~SB_VALID_TOGGLEBIT;

  if( buildable > BA_NONE )
  {
    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to place the %s\n",
          CG_KeyNameForCommand( "+attack" ),
          BG_FindHumanNameForBuildable( buildable ) ) );

    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to cancel placing the %s\n",
          CG_KeyNameForCommand( "+button5" ),
          BG_FindHumanNameForBuildable( buildable ) ) );
  }
  else
  {
    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to build a structure\n",
          CG_KeyNameForCommand( "+attack" ) ) );

    if( CG_BuildableInRange( ps ) )
    {
      Q_strcat( text, MAX_TUTORIAL_TEXT,
          va( "Press %s to destroy this structure\n",
            CG_KeyNameForCommand( "deconstruct" ) ) );
    }
  }

  if( ps->stats[ STAT_PCLASS ] == PCL_ALIEN_BUILDER0_UPG )
  {
    if( ( ps->stats[ STAT_BUILDABLE ] & ~SB_VALID_TOGGLEBIT ) == BA_NONE )
    {
      Q_strcat( text, MAX_TUTORIAL_TEXT,
          va( "Press %s to swipe\n",
            CG_KeyNameForCommand( "+button5" ) ) );
    }

    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to launch a projectile\n",
          CG_KeyNameForCommand( "+button2" ) ) );

    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to walk on walls\n",
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
  Q_strcat( text, MAX_TUTORIAL_TEXT,
      "Touch a human to damage it\n" );

  Q_strcat( text, MAX_TUTORIAL_TEXT,
      va( "Press %s to walk on walls\n",
        CG_KeyNameForCommand( "+movedown" ) ) );
}

/*
===============
CG_AlienLevel1Text
===============
*/
static void CG_AlienLevel1Text( char *text, playerState_t *ps )
{
  Q_strcat( text, MAX_TUTORIAL_TEXT,
      "Touch a human to grab it\n" );

  Q_strcat( text, MAX_TUTORIAL_TEXT,
      va( "Press %s to swipe\n",
        CG_KeyNameForCommand( "+attack" ) ) );

  if( ps->stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL1_UPG )
  {
    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to spray poisonous gas\n",
          CG_KeyNameForCommand( "+button5" ) ) );
  }

  Q_strcat( text, MAX_TUTORIAL_TEXT,
      va( "Press %s to walk on walls\n",
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
      va( "Press %s to bite\n",
        CG_KeyNameForCommand( "+attack" ) ) );

  if( ps->stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL2_UPG )
  {
    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to invoke an electrical attack\n",
          CG_KeyNameForCommand( "+button5" ) ) );
  }

  Q_strcat( text, MAX_TUTORIAL_TEXT,
      va( "Hold down %s then touch a wall to wall jump\n",
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
      va( "Press %s to bite\n",
        CG_KeyNameForCommand( "+attack" ) ) );

  if( ps->stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL3_UPG )
  {
    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to launch a projectile\n",
          CG_KeyNameForCommand( "+button2" ) ) );
  }

  Q_strcat( text, MAX_TUTORIAL_TEXT,
      va( "Hold down and release %s to pounce\n",
        CG_KeyNameForCommand( "+button5" ) ) );
}

/*
===============
CG_AlienLevel4Text
===============
*/
static void CG_AlienLevel4Text( char *text, playerState_t *ps )
{
  Q_strcat( text, MAX_TUTORIAL_TEXT,
      va( "Press %s to swipe\n",
        CG_KeyNameForCommand( "+attack" ) ) );

  Q_strcat( text, MAX_TUTORIAL_TEXT,
      va( "Hold down and release %s to charge\n",
        CG_KeyNameForCommand( "+button5" ) ) );
}

/*
===============
CG_HumanCkitText
===============
*/
static void CG_HumanCkitText( char *text, playerState_t *ps )
{
  buildable_t buildable = ps->stats[ STAT_BUILDABLE ] & ~SB_VALID_TOGGLEBIT;

  if( buildable > BA_NONE )
  {
    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to place the %s\n",
          CG_KeyNameForCommand( "+attack" ),
          BG_FindHumanNameForBuildable( buildable ) ) );

    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to cancel placing the %s\n",
          CG_KeyNameForCommand( "+button5" ),
          BG_FindHumanNameForBuildable( buildable ) ) );
  }
  else
  {
    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to build a structure\n",
          CG_KeyNameForCommand( "+attack" ) ) );

    if( CG_BuildableInRange( ps ) )
    {
      Q_strcat( text, MAX_TUTORIAL_TEXT,
          va( "Press %s to destroy this structure\n",
            CG_KeyNameForCommand( "deconstruct" ) ) );
    }
  }
}

/*
===============
CG_HumanText
===============
*/
static void CG_HumanText( char *text, playerState_t *ps )
{
  char      *name;
  int       ammo, clips;
  upgrade_t upgrade = UP_NONE;

  if( cg.weaponSelect <= 32 )
    name = cg_weapons[ cg.weaponSelect ].humanName;
  else if( cg.weaponSelect > 32 )
  {
    name = cg_upgrades[ cg.weaponSelect - 32 ].humanName;
    upgrade = cg.weaponSelect - 32;
  }

  BG_UnpackAmmoArray( ps->weapon, ps->ammo, ps->powerups, &ammo, &clips );

  if( !ammo && !clips && !BG_FindInfinteAmmoForWeapon( ps->weapon ) )
  {
    //no ammo
    switch( ps->weapon )
    {
      case WP_MACHINEGUN:
      case WP_CHAINGUN:
      case WP_SHOTGUN:
      case WP_FLAMER:
        Q_strcat( text, MAX_TUTORIAL_TEXT,
            va( "Find an Armoury and press %s for more ammo\n",
              CG_KeyNameForCommand( "buy ammo" ) ) );
        break;

      case WP_LAS_GUN:
      case WP_PULSE_RIFLE:
      case WP_MASS_DRIVER:
      case WP_LUCIFER_CANNON:
        Q_strcat( text, MAX_TUTORIAL_TEXT,
            va( "Find a Reactor or Repeater and press %s for more ammo\n",
              CG_KeyNameForCommand( "buy ammo" ) ) );
        break;

      default:
        break;
    }
  }
  else
  {
    switch( ps->weapon )
    {
      case WP_BLASTER:
      case WP_MACHINEGUN:
      case WP_SHOTGUN:
      case WP_LAS_GUN:
      case WP_CHAINGUN:
      case WP_PULSE_RIFLE:
      case WP_FLAMER:
        Q_strcat( text, MAX_TUTORIAL_TEXT,
            va( "Press %s to fire the %s\n",
              CG_KeyNameForCommand( "+attack" ),
              BG_FindHumanNameForWeapon( ps->weapon ) ) );
        break;

      case WP_MASS_DRIVER:
        Q_strcat( text, MAX_TUTORIAL_TEXT,
            va( "Press %s to fire the %s\n",
              CG_KeyNameForCommand( "+attack" ),
              BG_FindHumanNameForWeapon( ps->weapon ) ) );

        Q_strcat( text, MAX_TUTORIAL_TEXT,
            va( "Hold %s to zoom\n",
              CG_KeyNameForCommand( "+zoom" ) ) );
        break;

      case WP_PAIN_SAW:
        Q_strcat( text, MAX_TUTORIAL_TEXT,
            va( "Hold %s to activate the %s\n",
              CG_KeyNameForCommand( "+attack" ),
              BG_FindHumanNameForWeapon( ps->weapon ) ) );
        break;

      case WP_LUCIFER_CANNON:
        Q_strcat( text, MAX_TUTORIAL_TEXT,
            va( "Hold and release %s to fire a charged shot\n",
              CG_KeyNameForCommand( "+attack" ) ) );

        Q_strcat( text, MAX_TUTORIAL_TEXT,
            va( "Press %s to fire the %s\n",
              CG_KeyNameForCommand( "+button5" ),
              BG_FindHumanNameForWeapon( ps->weapon ) ) );
        break;

      case WP_HBUILD:
      case WP_HBUILD2:
        CG_HumanCkitText( text, ps );
        break;

      default:
        break;
    }
  }

  Q_strcat( text, MAX_TUTORIAL_TEXT,
      va( "Press %s and ",
          CG_KeyNameForCommand( "weapprev" ) ) );
  Q_strcat( text, MAX_TUTORIAL_TEXT,
      va( "%s to select an upgrade\n",
          CG_KeyNameForCommand( "weapnext" ) ) );

  if( upgrade == UP_NONE ||
      ( upgrade > UP_NONE && BG_FindUsableForUpgrade( upgrade ) ) )
  {
    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to use the %s\n",
            CG_KeyNameForCommand( "+button2" ),
            name ) );
  }

  if( ps->stats[ STAT_HEALTH ] <= 35 &&
      BG_InventoryContainsUpgrade( UP_MEDKIT, ps->stats ) )
  {
    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to use your %s\n",
          CG_KeyNameForCommand( "itemact medkit" ),
          BG_FindHumanNameForUpgrade( UP_MEDKIT ) ) );
  }

  Q_strcat( text, MAX_TUTORIAL_TEXT,
      va( "Press %s to use a structure\n",
        CG_KeyNameForCommand( "+button7" ) ) );
}

/*
===============
CG_SpectatorText
===============
*/
static void CG_SpectatorText( char *text, playerState_t *ps )
{
  if( ps->pm_flags & PMF_FOLLOW )
  {
    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to return to free spectator mode\n",
          CG_KeyNameForCommand( "+button2" ) ) );

    if( CG_PlayerCount( ) > 1 )
    {
      Q_strcat( text, MAX_TUTORIAL_TEXT,
          va( "Press %s or ",
            CG_KeyNameForCommand( "weapprev" ) ) );
      Q_strcat( text, MAX_TUTORIAL_TEXT,
          va( "%s to change player\n",
            CG_KeyNameForCommand( "weapnext" ) ) );
    }
  }
  else if( ps->pm_type == PM_SPECTATOR )
  {
    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to join a team\n",
          CG_KeyNameForCommand( "+attack" ) ) );

    if( CG_PlayerCount( ) > 0 )
    {
      Q_strcat( text, MAX_TUTORIAL_TEXT,
          va( "Press %s to enter spectator follow mode\n",
            CG_KeyNameForCommand( "+button2" ) ) );
    }
  }
  else
  {
    Q_strcat( text, MAX_TUTORIAL_TEXT,
        va( "Press %s to spawn\n",
          CG_KeyNameForCommand( "+attack" ) ) );
  }
}

/*
===============
CG_TutorialText

Returns context help for the current class/weapon
===============
*/
const char *CG_TutorialText( void )
{
  playerState_t *ps;
  static char   text[ MAX_TUTORIAL_TEXT ];

  CG_GetBindings( );

  text[ 0 ] = '\0';
  ps = &cg.snap->ps;

  if( !cg.intermissionStarted && !cg.demoPlayback )
  {
    if( ps->persistant[ PERS_TEAM ] == TEAM_SPECTATOR ||
        ps->pm_flags & PMF_FOLLOW )
    {
      CG_SpectatorText( text, ps );
    }
    else if( ps->stats[ STAT_HEALTH ] > 0 )
    {
      switch( ps->stats[ STAT_PCLASS ] )
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
          CG_AlienLevel1Text( text, ps );
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

        case PCL_HUMAN:
          CG_HumanText( text, ps );
          break;

        default:
          break;
      }

      if( ps->stats[ STAT_PTEAM ] == PTE_ALIENS &&
          BG_UpgradeClassAvailable( ps ) )
      {
        Q_strcat( text, MAX_TUTORIAL_TEXT,
            va( "Press %s to evolve\n",
              CG_KeyNameForCommand( "+button7" ) ) );
      }
    }

    Q_strcat( text, MAX_TUTORIAL_TEXT, "Press ESC for the menu" );
  }

  return text;
}
