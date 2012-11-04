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
	CG_Printf( "(%i %i %i) : %i\n", ( int ) cg.refdef.vieworg[ 0 ],
	           ( int ) cg.refdef.vieworg[ 1 ], ( int ) cg.refdef.vieworg[ 2 ],
	           ( int ) cg.refdefViewAngles[ YAW ] );
}

qboolean CG_RequestScores( void )
{
	if ( cg.scoresRequestTime + 2000 < cg.time )
	{
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score\n" );

		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

extern menuDef_t *menuScoreboard;

static void CG_scrollScoresDown_f( void )
{
	if ( menuScoreboard && cg.scoreBoardShowing )
	{
		Menu_ScrollFeeder( menuScoreboard, FEEDER_ALIENTEAM_LIST, qtrue );
		Menu_ScrollFeeder( menuScoreboard, FEEDER_HUMANTEAM_LIST, qtrue );
	}
}

static void CG_scrollScoresUp_f( void )
{
	if ( menuScoreboard && cg.scoreBoardShowing )
	{
		Menu_ScrollFeeder( menuScoreboard, FEEDER_ALIENTEAM_LIST, qfalse );
		Menu_ScrollFeeder( menuScoreboard, FEEDER_HUMANTEAM_LIST, qfalse );
	}
}

static void CG_ScoresDown_f( void )
{
	if ( !cg.showScores )
	{
		Menu_SetFeederSelection( menuScoreboard, FEEDER_ALIENTEAM_LIST, 0, NULL );
		Menu_SetFeederSelection( menuScoreboard, FEEDER_HUMANTEAM_LIST, 0, NULL );
	}

	if ( CG_RequestScores() )
	{
		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores )
		{
			if ( cg_debugRandom.integer )
			{
				CG_Printf( "CG_ScoresDown_f: scores out of date\n" );
			}

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
	if ( cg.showScores )
	{
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}

void CG_ClientList_f( void )
{
	clientInfo_t *ci;
	int          i;
	int          count = 0;

	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		ci = &cgs.clientinfo[ i ];

		if ( !ci->infoValid )
		{
			continue;
		}

		switch ( ci->team )
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

	Com_Printf(_( "Listed %2d clients\n"), count ); // FIXME PLURAL
}

static void CG_ReloadHUD_f( void )
{
	char       buff[ 1024 ];
	const char *hudSet;
	UI_InitMemory();
	String_Init();
	Menu_Reset();

	trap_Cvar_VariableStringBuffer( "cg_hudFiles", buff, sizeof( buff ) );
	hudSet = buff;

	if ( !cg_hudFilesEnable.integer || hudSet[ 0 ] == '\0' )
	{
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus( hudSet );
}

static void CG_UIMenu_f( void )
{
	trap_SendConsoleCommand( va( "menu %s\n", Quote( CG_Argv( 1 ) ) ) );
}

static void CG_CompleteClass( void )
{
	int i = 0;

	if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_ALIENS )
	{
		for ( i = PCL_ALIEN_BUILDER0; i < PCL_HUMAN; i++ )
		{
			trap_CompleteCallback( BG_Class( i )->name );
		}
	}
	else if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_HUMANS )
	{
		trap_CompleteCallback( BG_Weapon( WP_HBUILD )->name );
		trap_CompleteCallback( BG_Weapon( WP_MACHINEGUN )->name );
	}
}

static void CG_CompleteBuy( void )
{
	int i;

	if( cgs.clientinfo[ cg.clientNum ].team != TEAM_HUMANS )
	{
		return;
	}

	for( i = 0; i < UP_NUM_UPGRADES; i++ )
	{
		const upgradeAttributes_t *item = BG_Upgrade( i );
		if ( item->purchasable && item->team == TEAM_HUMANS )
		{
			trap_CompleteCallback( item->name );
		}
	}

	trap_CompleteCallback( "grenade" ); // called "gren" elsewhere, so special-case it

	for( i = 0; i < WP_NUM_WEAPONS; i++ )
	{
		const weaponAttributes_t *item = BG_Weapon( i );
		if ( item->purchasable && item->team == TEAM_HUMANS )
		{
			trap_CompleteCallback( item->name );
		}
	}
}

static void CG_CompleteSell( void )
{
	if( cgs.clientinfo[ cg.clientNum ].team != TEAM_HUMANS )
	{
		return;
	}

	trap_CompleteCallback( "weapons" );
	trap_CompleteCallback( "upgrades" );
	CG_CompleteBuy( );
}

static void CG_CompleteBuild( void )
{
	int i;

	for ( i = 0; i < BA_NUM_BUILDABLES; i++ )
	{
		const buildableAttributes_t *item = BG_Buildable( i );
		if ( item->team == cgs.clientinfo[ cg.clientNum ].team )
		{
			trap_CompleteCallback( item->name );
		}
	}
}

static void CG_CompleteName( void )
{
	int           i;
	clientInfo_t *ci;

	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		char name[ MAX_NAME_LENGTH ];
		ci = &cgs.clientinfo[ i ];
		strcpy( name, ci->name );

		if ( !ci->infoValid )
		{
			continue;
		}

		trap_CompleteCallback( Q_CleanStr( name ) );
	}
}

static void CG_CompleteVsay( void )
{
	voice_t     *voice = cgs.voices;
	voiceCmd_t  *voiceCmd = voice->cmds;

	while ( voiceCmd != NULL )
	{
		trap_CompleteCallback( voiceCmd->cmd );
		voiceCmd = voiceCmd->next;
	}
}

static void CG_CompleteGive( void )
{
	int               i = 0;
	static const char give[][ 12 ] =
	{
		"all", "health", "funds", "stamina", "poison", "gas", "ammo"
	};

	for( i = 0; i < ARRAY_LEN( give ); i++ )
	{
		trap_CompleteCallback( give[i] );
	}
}

static void CG_CompleteTeamVote( void )
{
	int           i = 0;
	static const char vote[][ 16 ] =
	{
		"kick", "spectate", "denybuild", "allowbuild", "admitdefeat", "poll"
	};

	for( i = 0; i < ARRAY_LEN( vote ); i++ )
	{
		trap_CompleteCallback( vote[i] );
	}
}
static void CG_CompleteVote( void )
{
	int           i = 0;
	static const char vote[][ 16 ] =
	{
		"kick", "spectate", "mute", "unmute", "sudden_death", "extend",
		"draw", "map_restart", "map", "layout", "nextmap", "poll"
	};

	for( i = 0; i < ARRAY_LEN( vote ); i++ )
	{
		trap_CompleteCallback( vote[i] );
	}
}

static void CG_CompleteItem( void )
{
	int i = 0;

	if( cgs.clientinfo[ cg.clientNum ].team == TEAM_ALIENS )
	{
		return;
	}

	trap_CompleteCallback( "weapon" );

	for( i = 0; i < UP_NUM_UPGRADES; i++ )
	{
		const upgradeAttributes_t *item = BG_Upgrade( i );
		if ( item->usable )
		{
			trap_CompleteCallback( item->name );
		}
	}

	for( i = 0; i < WP_NUM_WEAPONS; i++ )
	{
		const weaponAttributes_t *item = BG_Weapon( i );
		if( item->team == TEAM_HUMANS )
		{
			trap_CompleteCallback( item->name );
		}
	}
}

static const struct
{
	const char *cmd;
	void ( *function )( void );
	void ( *completer )( void );
} commands[] =
{
	{ "+scores",       CG_ScoresDown_f,         0                },
	{ "-scores",       CG_ScoresUp_f,           0                },
	{ "build",         0,                       CG_CompleteBuild },
	{ "buy",           0,                       CG_CompleteBuy   },
	{ "class",         0,                       CG_CompleteClass },
	{ "cgame_memory",  BG_MemoryInfo,           0                },
	{ "clientlist",    CG_ClientList_f,         0                },
	{ "callvote",      0,                       CG_CompleteVote  },
	{ "callteamvote",  0,                       CG_CompleteTeamVote },
	{ "destroyTestPS", CG_DestroyTestPS_f,      0                },
	{ "destroyTestTS", CG_DestroyTestTS_f,      0                },
	{ "follow",        0,                       CG_CompleteName  },
	{ "give",          0,                       CG_CompleteGive  },
	{ "ignore",        0,                       CG_CompleteName  },
	{ "itemact",       0,                       CG_CompleteItem  },
	{ "itemdeact",     0,                       CG_CompleteItem  },
	{ "itemtoggle",    0,                       CG_CompleteItem  },
	{ "m",             0,                       CG_CompleteName  },
	{ "mt",            0,                       CG_CompleteName  },
	{ "nextframe",     CG_TestModelNextFrame_f, 0                },
	{ "nextskin",      CG_TestModelNextSkin_f,  0                },
	{ "prevframe",     CG_TestModelPrevFrame_f, 0                },
	{ "prevskin",      CG_TestModelPrevSkin_f,  0                },
	{ "reloadhud",     CG_ReloadHUD_f,          0                },
	{ "scoresDown",    CG_scrollScoresDown_f,   0                },
	{ "scoresUp",      CG_scrollScoresUp_f,     0                },
	{ "sell",          0,                       CG_CompleteSell  },
	{ "sizedown",      CG_SizeDown_f,           0                },
	{ "sizeup",        CG_SizeUp_f,             0                },
	{ "testgun",       CG_TestGun_f,            0                },
	{ "testmodel",     CG_TestModel_f,          0                },
	{ "testPS",        CG_TestPS_f,             0                },
	{ "testTS",        CG_TestTS_f,             0                },
	{ "ui_menu",       CG_UIMenu_f,             0                },
	{ "unignore",      0,                       CG_CompleteName  },
	{ "viewpos",       CG_Viewpos_f,            0                },
	{ "vsay",          0,                       CG_CompleteVsay  },
	{ "vsay_local",    0,                       CG_CompleteVsay  },
	{ "vsay_team",     0,                       CG_CompleteVsay  },
	{ "weapnext",      CG_NextWeapon_f,         0                },
	{ "weapon",        CG_Weapon_f,             0                },
	{ "weapprev",      CG_PrevWeapon_f,         0                }
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
	               ARRAY_LEN( commands ), sizeof( commands[ 0 ] ),
	               cmdcmp );

	if ( !cmd || !cmd->function )
	{
		return qfalse;
	}

	cmd->function();
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
	int i;

	for ( i = 0; i < ARRAY_LEN( commands ); i++ )
	{
		trap_AddCommand( commands[ i ].cmd );
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand( "kill" );
	trap_AddCommand( "ui_messagemode" );
	trap_AddCommand( "ui_messagemode2" );
	trap_AddCommand( "ui_messagemode3" );
	trap_AddCommand( "ui_messagemode4" );
	trap_AddCommand( "ui_messagemodec" );
	trap_AddCommand( "say" );
	trap_AddCommand( "say_team" );
	trap_AddCommand( "god" );
	trap_AddCommand( "notarget" );
	trap_AddCommand( "noclip" );
	trap_AddCommand( "where" );
	trap_AddCommand( "team" );
	trap_AddCommand( "follownext" );
	trap_AddCommand( "followprev" );
	trap_AddCommand( "setviewpos" );
	trap_AddCommand( "vote" );
	trap_AddCommand( "teamvote" );
	trap_AddCommand( "reload" );
	trap_AddCommand( "destroy" );
	trap_AddCommand( "deconstruct" );

	trap_RegisterButtonCommands(
	    // 0      12       3     45      6        78       9ABCDEF      <- bit nos.
	      "attack,,useitem,taunt,,sprint,activate,,attack2,,,,,,,dodge"
	    );
}

/*
=================
CG_CompleteCommand

The command has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/

void CG_CompleteCommand( int argNum )
{
	const char *cmd;
	int        i;

	cmd = CG_Argv( 0 );

	while ( *cmd == '\\' || *cmd == '/' )
	{
		cmd++;
	}

	for ( i = 0; i < ARRAY_LEN( commands ); i++ )
	{
		if ( !Q_stricmp( cmd, commands[ i ].cmd ) && commands[ i ].completer )
		{
			commands[ i ].completer();
			return;
		}
	}
}
