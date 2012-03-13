/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding


#include "cg_local.h"



/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f( void )
{
  trap_Cvar_Set( "cg_viewsize", va( "%i", MIN( cg_viewsize.integer + 10, 100 ) ) );
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f( void )
{
  trap_Cvar_Set( "cg_viewsize", va( "%i", MAX( cg_viewsize.integer - 10, 30 ) ) );
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f( void )
{
  CG_Printf( "(%i %i %i) : %i\n", (int)cg.refdef.vieworg[ 0 ],
    (int)cg.refdef.vieworg[ 1 ], (int)cg.refdef.vieworg[ 2 ],
    (int)cg.refdefViewAngles[ YAW ] );
}

qboolean CG_RequestScores( void )
{
  if( cg.scoresRequestTime + 2000 < cg.time )
  {
    // the scores are more than two seconds out of data,
    // so request new ones
    cg.scoresRequestTime = cg.time;
    trap_SendClientCommand( "score\n" );

    return qtrue;
  }
  else
    return qfalse;
}

extern menuDef_t *menuScoreboard;

static void CG_scrollScoresDown_f( void )
{
  if( menuScoreboard && cg.scoreBoardShowing )
  {
    Menu_ScrollFeeder( menuScoreboard, FEEDER_ALIENTEAM_LIST, qtrue );
    Menu_ScrollFeeder( menuScoreboard, FEEDER_HUMANTEAM_LIST, qtrue );
  }
}


static void CG_scrollScoresUp_f( void )
{
  if( menuScoreboard && cg.scoreBoardShowing )
  {
    Menu_ScrollFeeder( menuScoreboard, FEEDER_ALIENTEAM_LIST, qfalse );
    Menu_ScrollFeeder( menuScoreboard, FEEDER_HUMANTEAM_LIST, qfalse );
  }
}

static void CG_ScoresDown_f( void )
{
  if( !cg.showScores )
  {
    Menu_SetFeederSelection( menuScoreboard, FEEDER_ALIENTEAM_LIST, 0, NULL );
    Menu_SetFeederSelection( menuScoreboard, FEEDER_HUMANTEAM_LIST, 0, NULL );
  }

  if( CG_RequestScores( ) )
  {
    // leave the current scores up if they were already
    // displayed, but if this is the first hit, clear them out
    if( !cg.showScores )
    {
      if( cg_debugRandom.integer )
        CG_Printf( "CG_ScoresDown_f: scores out of date\n" );

      cg.showScores = qtrue;
      cg.numScores = 0;
    }
  }
  else
  {
    // show the cached contents even if they just pressed if it
    // is within two seconds
    cg.showScores = qtrue;
  }
}

static void CG_ScoresUp_f( void )
{
  if( cg.showScores )
  {
    cg.showScores = qfalse;
    cg.scoreFadeTime = cg.time;
  }
}

void CG_ClientList_f( void )
{
  clientInfo_t *ci;
  int i;
  int count = 0;

  for( i = 0; i < MAX_CLIENTS; i++ ) 
  {
    ci = &cgs.clientinfo[ i ];
    if( !ci->infoValid ) 
      continue;

    switch( ci->team ) 
    {
      case TEAM_ALIENS:
        Com_Printf( "%2d " S_COLOR_RED "A   " S_COLOR_WHITE "%s\n", i,
          ci->name );
        break;

      case TEAM_HUMANS:
        Com_Printf( "%2d " S_COLOR_CYAN "H   " S_COLOR_WHITE "%s\n", i,
          ci->name );
        break;

      default:
      case TEAM_NONE:
      case NUM_TEAMS:
        Com_Printf( "%2d S   %s\n", i, ci->name );
        break;
    }

    count++;
  }

  Com_Printf( "Listed %2d clients\n", count );
}

static void CG_ReloadHUD_f( void ) {
  char        buff[ 1024 ];
  const char  *hudSet;
  UI_InitMemory( );
  String_Init( );
  Menu_Reset( );

  trap_Cvar_VariableStringBuffer( "cg_hudFiles", buff, sizeof( buff ) );
  hudSet = buff;

  if( !cg_hudFilesEnable.integer || hudSet[ 0 ] == '\0' )
    hudSet = "ui/hud.txt";

  CG_LoadMenus( hudSet );
}

static void CG_UIMenu_f( void )
{
  trap_SendConsoleCommand( va( "menu %s\n", CG_Argv( 1 ) ) );
}

static void CG_NullFunc( void )
{
}

// TODO: Use functions from bg_misc.c so this stuff isn't hardcoded. The problem with that is that they also include invalid values.

static void CG_CompleteClass( void )
{
  int i = 0;
  if( cgs.clientinfo[ cg.clientNum ].team == TEAM_ALIENS ) {
    const char *classes[] = { "builder", "builderupg", "level0", "level1", "level1upg", "level2",
      "level2upg", "level3", "level3upg", "level4" };
    for( i=0; i<ARRAY_LEN( classes ); i++ )
      trap_CompleteCallback( classes[i] );
  }
  else if( cgs.clientinfo[ cg.clientNum ].team == TEAM_HUMANS ) {
    const char *classes[] = { "rifle", "ckit" };
    for( i=0; i<ARRAY_LEN( classes ); i++ )
      trap_CompleteCallback( classes[i] );
  }
}

static void CG_CompleteBuy( void )
{
  int i = 0;
  const char *items[] = { "ckit", "rifle", "psaw", "shotgun", "lgun", "mdriver", "chaingun",
    "prifle", "flamer", "lcannon", "larmour", "helmet", "bsuit", "grenade", "battpack"
    "jetpack", "ammo" };

  if( cgs.clientinfo[ cg.clientNum ].team != TEAM_HUMANS )
    return;
    
  
  for( i=0; i<ARRAY_LEN( items ); i++ )
    trap_CompleteCallback( items[i] );
}

static void CG_CompleteBuild( void )
{
  int i = 0;
  if( cgs.clientinfo[ cg.clientNum ].team == TEAM_ALIENS ) {
    const char *structs[] = { "eggpod", "overmind", "barricade", "acid_tube", "trapper",
      "booster", "hive" };
    for( i=0; i<ARRAY_LEN( structs ); i++ )
      trap_CompleteCallback( structs[i] );
  }
  else if( cgs.clientinfo[ cg.clientNum ].team == TEAM_HUMANS ) {
    const char *structs[] = { "telenode", "mgturret", "tesla", "arm", "dcc", "medistat", "reactor", "repeater" };
    for( i=0; i<ARRAY_LEN( structs ); i++ )
      trap_CompleteCallback( structs[i] );
  }
}

static struct {
	char *cmd; 
	void ( *function )( void ); 
	void ( *completer )( void );
} commands[ ] = {
  { "+scores", CG_ScoresDown_f, NULL },
  { "-scores", CG_ScoresUp_f, NULL },
  { "build", NULL, CG_CompleteBuild },
  { "buy", NULL, CG_CompleteBuy },
  { "class", NULL, CG_CompleteClass },
  { "cgame_memory", BG_MemoryInfo, NULL },
  { "clientlist", CG_ClientList_f, NULL },
  { "destroyTestPS", CG_DestroyTestPS_f, NULL },
  { "destroyTestTS", CG_DestroyTestTS_f, NULL },
  { "nextframe", CG_TestModelNextFrame_f, NULL },
  { "nextskin", CG_TestModelNextSkin_f, NULL },
  { "prevframe", CG_TestModelPrevFrame_f, NULL },
  { "prevskin", CG_TestModelPrevSkin_f, NULL },
  { "reloadhud", CG_ReloadHUD_f, NULL },
  { "scoresDown", CG_scrollScoresDown_f, NULL },
  { "scoresUp", CG_scrollScoresUp_f, NULL },
  { "sell", NULL, CG_CompleteBuy },
  { "sizedown", CG_SizeDown_f, NULL },
  { "sizeup", CG_SizeUp_f, NULL },
  { "testgun", CG_TestGun_f, NULL },
  { "testmodel", CG_TestModel_f, NULL },
  { "testPS", CG_TestPS_f, NULL },
  { "testTS", CG_TestTS_f, NULL },
  { "ui_menu", CG_UIMenu_f, NULL },
  { "viewpos", CG_Viewpos_f, NULL },
  { "weapnext", CG_NextWeapon_f, NULL },
  { "weapon", CG_Weapon_f, NULL },
  { "weapprev", CG_PrevWeapon_f, NULL }
};

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void )
{
  consoleCommand_t *cmd;

  cmd = bsearch( CG_Argv( 0 ), commands,
    sizeof( commands ) / sizeof( commands[ 0 ]), sizeof( commands[ 0 ] ),
    cmdcmp );

  if( !cmd || !cmd->function )
    return qfalse;

  cmd->function( );
  return qtrue;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void )
{
  int   i;

  for( i = 0 ; i < sizeof( commands ) / sizeof( commands[ 0 ] ) ; i++ )
    trap_AddCommand( commands[ i ].cmd );

  //
  // the game server will interpret these commands, which will be automatically
  // forwarded to the server after they are not recognized locally
  //
  trap_AddCommand( "kill" );
  trap_AddCommand( "ui_messagemode" );
  trap_AddCommand( "ui_messagemode2" );
  trap_AddCommand( "ui_messagemode3" );
  trap_AddCommand( "ui_messagemode4" );
  trap_AddCommand( "say" );
  trap_AddCommand( "say_team" );
  trap_AddCommand( "vsay" );
  trap_AddCommand( "vsay_team" );
  trap_AddCommand( "vsay_local" );
  trap_AddCommand( "m" );
  trap_AddCommand( "mt" );
  trap_AddCommand( "give" );
  trap_AddCommand( "god" );
  trap_AddCommand( "notarget" );
  trap_AddCommand( "noclip" );
  trap_AddCommand( "team" );
  trap_AddCommand( "follow" );
  trap_AddCommand( "setviewpos" );
  trap_AddCommand( "callvote" );
  trap_AddCommand( "vote" );
  trap_AddCommand( "callteamvote" );
  trap_AddCommand( "teamvote" );
  trap_AddCommand( "reload" );
  trap_AddCommand( "itemact" );
  trap_AddCommand( "itemdeact" );
  trap_AddCommand( "itemtoggle" );
  trap_AddCommand( "destroy" );
  trap_AddCommand( "deconstruct" );
  trap_AddCommand( "ignore" );
  trap_AddCommand( "unignore" );
}

/*
================= 
CG_CompleteCommand

The command has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
================= 
*/

void CG_CompleteCommand( int argNum ) {
	const char *cmd;
	int i; 

	cmd = CG_Argv( 0 );
	while( *cmd == '\\' || *cmd == '/' )
	  cmd++;

	for( i = 0; i < sizeof( commands ) / sizeof( commands[ 0 ] ); i++ )  {
		if( !Q_stricmp( cmd, commands[ i ].cmd ) ) {
			commands[ i ].completer( );
			return;
		}
	}
}