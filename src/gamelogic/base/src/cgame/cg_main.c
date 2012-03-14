/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

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

// cg_main.c -- initialization and primary entry point for cgame

#include "cg_local.h"

#include "../ui/ui_shared.h"
// display context for new ui stuff
displayContextDef_t cgDC;

int                 forceModelModificationCount = -1;

void                CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void                CG_Shutdown( void );

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
intptr_t vmMain( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11  )
{
	switch ( command )
	{
		case CG_INIT:
			CG_Init( arg0, arg1, arg2 );
			return 0;

		case CG_SHUTDOWN:
			CG_Shutdown();
			return 0;

		case CG_CONSOLE_COMMAND:
			return CG_ConsoleCommand();

		case CG_CONSOLE_TEXT:
			CG_AddNotifyText();
			return 0;

		case CG_DRAW_ACTIVE_FRAME:
			CG_DrawActiveFrame( arg0, arg1, arg2 );
			return 0;

		case CG_CROSSHAIR_PLAYER:
			return CG_CrosshairPlayer();

		case CG_LAST_ATTACKER:
			return CG_LastAttacker();

		case CG_KEY_EVENT:
			CG_KeyEvent( arg0, arg1 );
			return 0;

		case CG_MOUSE_EVENT:
			cgDC.cursorx = cgs.cursorX;
			cgDC.cursory = cgs.cursorY;
			CG_MouseEvent( arg0, arg1 );
			return 0;

		case CG_EVENT_HANDLING:
			CG_EventHandling( arg0 );
			return 0;

		case CG_COMPLETE_COMMAND:
			CG_CompleteCommand( arg0 );
			return 0;

		default:
			CG_Error( "vmMain: unknown command %i", ( int )command );
			break;
	}

	return -1;
}

cg_t      cg;
cgs_t     cgs;
centity_t cg_entities[ MAX_GENTITIES ];

//TA: weapons limit expanded:
//weaponInfo_t    cg_weapons[MAX_WEAPONS];
weaponInfo_t    cg_weapons[ 32 ];
upgradeInfo_t   cg_upgrades[ 32 ];

buildableInfo_t cg_buildables[ BA_NUM_BUILDABLES ];

vmCvar_t        cg_teslaTrailTime;
vmCvar_t        cg_railTrailTime;
vmCvar_t        cg_centertime;
vmCvar_t        cg_runpitch;
vmCvar_t        cg_runroll;
vmCvar_t        cg_bobup;
vmCvar_t        cg_bobpitch;
vmCvar_t        cg_bobroll;
vmCvar_t        cg_swingSpeed;
vmCvar_t        cg_shadows;
vmCvar_t        cg_gibs;
vmCvar_t        cg_drawTimer;
vmCvar_t        cg_drawFPS;
vmCvar_t        cg_drawDemoState;
vmCvar_t        cg_drawSnapshot;
vmCvar_t        cg_draw3dIcons;
vmCvar_t        cg_drawIcons;
vmCvar_t        cg_drawAmmoWarning;
vmCvar_t        cg_drawCrosshair;
vmCvar_t        cg_drawCrosshairNames;
vmCvar_t        cg_drawRewards;
vmCvar_t        cg_crosshairX;
vmCvar_t        cg_crosshairY;
vmCvar_t        cg_draw2D;
vmCvar_t        cg_drawStatus;
vmCvar_t        cg_animSpeed;
vmCvar_t        cg_debugAnim;
vmCvar_t        cg_debugPosition;
vmCvar_t        cg_debugEvents;
vmCvar_t        cg_errorDecay;
vmCvar_t        cg_nopredict;
vmCvar_t        cg_debugMove;
vmCvar_t        cg_noPlayerAnims;
vmCvar_t        cg_showmiss;
vmCvar_t        cg_footsteps;
vmCvar_t        cg_addMarks;
vmCvar_t        cg_brassTime;
vmCvar_t        cg_viewsize;
vmCvar_t        cg_drawGun;
vmCvar_t        cg_gun_frame;
vmCvar_t        cg_gun_x;
vmCvar_t        cg_gun_y;
vmCvar_t        cg_gun_z;
vmCvar_t        cg_tracerChance;
vmCvar_t        cg_tracerWidth;
vmCvar_t        cg_tracerLength;
vmCvar_t        cg_autoswitch;
vmCvar_t        cg_ignore;
vmCvar_t        cg_simpleItems;
vmCvar_t        cg_fov;
vmCvar_t        cg_zoomFov;
vmCvar_t        cg_thirdPerson;
vmCvar_t        cg_thirdPersonRange;
vmCvar_t        cg_thirdPersonAngle;
vmCvar_t        cg_stereoSeparation;
vmCvar_t        cg_lagometer;
vmCvar_t        cg_drawAttacker;
vmCvar_t        cg_synchronousClients;
vmCvar_t        cg_teamChatTime;
vmCvar_t        cg_teamChatHeight;
vmCvar_t        cg_stats;
vmCvar_t        cg_buildScript;
vmCvar_t        cg_forceModel;
vmCvar_t        cg_paused;
vmCvar_t        cg_blood;
vmCvar_t        cg_predictItems;
vmCvar_t        cg_deferPlayers;
vmCvar_t        cg_drawTeamOverlay;
vmCvar_t        cg_teamOverlayUserinfo;
vmCvar_t        cg_drawFriend;
vmCvar_t        cg_teamChatsOnly;
vmCvar_t        cg_noVoiceChats;
vmCvar_t        cg_noVoiceText;
vmCvar_t        cg_hudFiles;
vmCvar_t        cg_scorePlum;
vmCvar_t        cg_smoothClients;
vmCvar_t        pmove_fixed;
//vmCvar_t  cg_pmove_fixed;
vmCvar_t        pmove_msec;
vmCvar_t        cg_pmove_msec;
vmCvar_t        cg_cameraMode;
vmCvar_t        cg_cameraOrbit;
vmCvar_t        cg_cameraOrbitDelay;
vmCvar_t        cg_timescaleFadeEnd;
vmCvar_t        cg_timescaleFadeSpeed;
vmCvar_t        cg_timescale;
vmCvar_t        cg_smallFont;
vmCvar_t        cg_bigFont;
vmCvar_t        cg_noTaunt;
vmCvar_t        cg_noProjectileTrail;
vmCvar_t        cg_oldRail;
vmCvar_t        cg_oldRocket;
vmCvar_t        cg_oldPlasma;
vmCvar_t        cg_trueLightning;
vmCvar_t        cg_creepRes;
vmCvar_t        cg_drawSurfNormal;
vmCvar_t        cg_drawBBOX;
vmCvar_t        cg_debugAlloc;
vmCvar_t        cg_wwSmoothTime;
vmCvar_t        cg_wwFollow;
vmCvar_t        cg_wwToggle;
vmCvar_t        cg_depthSortParticles;
vmCvar_t        cg_consoleLatency;
vmCvar_t        cg_lightFlare;
vmCvar_t        cg_debugParticles;
vmCvar_t        cg_debugTrails;
vmCvar_t        cg_debugPVS;
vmCvar_t        cg_disableWarningDialogs;
vmCvar_t        cg_disableScannerPlane;
vmCvar_t        cg_tutorial;

vmCvar_t        cg_painBlendUpRate;
vmCvar_t        cg_painBlendDownRate;
vmCvar_t        cg_painBlendMax;
vmCvar_t        cg_painBlendScale;
vmCvar_t        cg_painBlendZoom;

//TA: hack to get class and carriage through to UI module
vmCvar_t        ui_currentClass;
vmCvar_t        ui_carriage;
vmCvar_t        ui_stages;
vmCvar_t        ui_dialog;
vmCvar_t        ui_loading;
vmCvar_t        ui_voteActive;
vmCvar_t        ui_alienTeamVoteActive;
vmCvar_t        ui_humanTeamVoteActive;

vmCvar_t        cg_debugRandom;

typedef struct
{
	vmCvar_t *vmCvar;
	char     *cvarName;
	char     *defaultString;
	int      cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[] =
{
	{ &cg_ignore,                "cg_ignore",                "0",            0                            }, // used for debugging
	{ &cg_autoswitch,            "cg_autoswitch",            "1",            CVAR_ARCHIVE                 },
	{ &cg_drawGun,               "cg_drawGun",               "1",            CVAR_ARCHIVE                 },
	{ &cg_zoomFov,               "cg_zoomfov",               "22.5",         CVAR_ARCHIVE                 },
	{ &cg_fov,                   "cg_fov",                   "90",           CVAR_ARCHIVE                 },
	{ &cg_viewsize,              "cg_viewsize",              "100",          CVAR_ARCHIVE                 },
	{ &cg_stereoSeparation,      "cg_stereoSeparation",      "0.4",          CVAR_ARCHIVE                 },
	{ &cg_shadows,               "cg_shadows",               "1",            CVAR_ARCHIVE                 },
	{ &cg_gibs,                  "cg_gibs",                  "1",            CVAR_ARCHIVE                 },
	{ &cg_draw2D,                "cg_draw2D",                "1",            CVAR_ARCHIVE                 },
	{ &cg_drawStatus,            "cg_drawStatus",            "1",            CVAR_ARCHIVE                 },
	{ &cg_drawTimer,             "cg_drawTimer",             "1",            CVAR_ARCHIVE                 },
	{ &cg_drawFPS,               "cg_drawFPS",               "1",            CVAR_ARCHIVE                 },
	{ &cg_drawDemoState,         "cg_drawDemoState",         "1",            CVAR_ARCHIVE                 },
	{ &cg_drawSnapshot,          "cg_drawSnapshot",          "0",            CVAR_ARCHIVE                 },
	{ &cg_draw3dIcons,           "cg_draw3dIcons",           "1",            CVAR_ARCHIVE                 },
	{ &cg_drawIcons,             "cg_drawIcons",             "1",            CVAR_ARCHIVE                 },
	{ &cg_drawAmmoWarning,       "cg_drawAmmoWarning",       "1",            CVAR_ARCHIVE                 },
	{ &cg_drawAttacker,          "cg_drawAttacker",          "1",            CVAR_ARCHIVE                 },
	{ &cg_drawCrosshair,         "cg_drawCrosshair",         "4",            CVAR_ARCHIVE                 },
	{ &cg_drawCrosshairNames,    "cg_drawCrosshairNames",    "1",            CVAR_ARCHIVE                 },
	{ &cg_drawRewards,           "cg_drawRewards",           "1",            CVAR_ARCHIVE                 },
	{ &cg_crosshairX,            "cg_crosshairX",            "0",            CVAR_ARCHIVE                 },
	{ &cg_crosshairY,            "cg_crosshairY",            "0",            CVAR_ARCHIVE                 },
	{ &cg_brassTime,             "cg_brassTime",             "2500",         CVAR_ARCHIVE                 },
	{ &cg_simpleItems,           "cg_simpleItems",           "0",            CVAR_ARCHIVE                 },
	{ &cg_addMarks,              "cg_marks",                 "1",            CVAR_ARCHIVE                 },
	{ &cg_lagometer,             "cg_lagometer",             "0",            CVAR_ARCHIVE                 },
	{ &cg_teslaTrailTime,        "cg_teslaTrailTime",        "250",          CVAR_ARCHIVE                 },
	{ &cg_railTrailTime,         "cg_railTrailTime",         "400",          CVAR_ARCHIVE                 },
	{ &cg_gun_x,                 "cg_gunX",                  "0",            CVAR_CHEAT                   },
	{ &cg_gun_y,                 "cg_gunY",                  "0",            CVAR_CHEAT                   },
	{ &cg_gun_z,                 "cg_gunZ",                  "0",            CVAR_CHEAT                   },
	{ &cg_centertime,            "cg_centertime",            "3",            CVAR_CHEAT                   },
	{ &cg_runpitch,              "cg_runpitch",              "0.002",        CVAR_ARCHIVE                 },
	{ &cg_runroll,               "cg_runroll",               "0.005",        CVAR_ARCHIVE                 },
	{ &cg_bobup,                 "cg_bobup",                 "0.005",        CVAR_CHEAT                   },
	{ &cg_bobpitch,              "cg_bobpitch",              "0.002",        CVAR_ARCHIVE                 },
	{ &cg_bobroll,               "cg_bobroll",               "0.002",        CVAR_ARCHIVE                 },
	{ &cg_swingSpeed,            "cg_swingSpeed",            "0.3",          CVAR_CHEAT                   },
	{ &cg_animSpeed,             "cg_animspeed",             "1",            CVAR_CHEAT                   },
	{ &cg_debugAnim,             "cg_debuganim",             "0",            CVAR_CHEAT                   },
	{ &cg_debugPosition,         "cg_debugposition",         "0",            CVAR_CHEAT                   },
	{ &cg_debugEvents,           "cg_debugevents",           "0",            CVAR_CHEAT                   },
	{ &cg_errorDecay,            "cg_errordecay",            "100",          0                            },
	{ &cg_nopredict,             "cg_nopredict",             "0",            0                            },
	{ &cg_debugMove,             "cg_debugMove",             "0",            0                            },
	{ &cg_noPlayerAnims,         "cg_noplayeranims",         "0",            CVAR_CHEAT                   },
	{ &cg_showmiss,              "cg_showmiss",              "0",            0                            },
	{ &cg_footsteps,             "cg_footsteps",             "1",            CVAR_CHEAT                   },
	{ &cg_tracerChance,          "cg_tracerchance",          "0.4",          CVAR_CHEAT                   },
	{ &cg_tracerWidth,           "cg_tracerwidth",           "1",            CVAR_CHEAT                   },
	{ &cg_tracerLength,          "cg_tracerlength",          "100",          CVAR_CHEAT                   },
	{ &cg_thirdPersonRange,      "cg_thirdPersonRange",      "40",           CVAR_CHEAT                   },
	{ &cg_thirdPersonAngle,      "cg_thirdPersonAngle",      "0",            CVAR_CHEAT                   },
	{ &cg_thirdPerson,           "cg_thirdPerson",           "0",            CVAR_CHEAT                   },
	{ &cg_teamChatTime,          "cg_teamChatTime",          "3000",         CVAR_ARCHIVE                 },
	{ &cg_teamChatHeight,        "cg_teamChatHeight",        "0",            CVAR_ARCHIVE                 },
	{ &cg_forceModel,            "cg_forceModel",            "0",            CVAR_ARCHIVE                 },
	{ &cg_predictItems,          "cg_predictItems",          "1",            CVAR_ARCHIVE                 },
	{ &cg_deferPlayers,          "cg_deferPlayers",          "1",            CVAR_ARCHIVE                 },
	{ &cg_drawTeamOverlay,       "cg_drawTeamOverlay",       "0",            CVAR_ARCHIVE                 },
	{ &cg_teamOverlayUserinfo,   "teamoverlay",              "0",            CVAR_ROM | CVAR_USERINFO     },
	{ &cg_stats,                 "cg_stats",                 "0",            0                            },
	{ &cg_drawFriend,            "cg_drawFriend",            "1",            CVAR_ARCHIVE                 },
	{ &cg_teamChatsOnly,         "cg_teamChatsOnly",         "0",            CVAR_ARCHIVE                 },
	{ &cg_noVoiceChats,          "cg_noVoiceChats",          "0",            CVAR_ARCHIVE                 },
	{ &cg_noVoiceText,           "cg_noVoiceText",           "0",            CVAR_ARCHIVE                 },
	{ &cg_creepRes,              "cg_creepRes",              "16",           CVAR_ARCHIVE                 },
	{ &cg_drawSurfNormal,        "cg_drawSurfNormal",        "0",            CVAR_CHEAT                   },
	{ &cg_drawBBOX,              "cg_drawBBOX",              "0",            CVAR_CHEAT                   },
	{ &cg_debugAlloc,            "cg_debugAlloc",            "0",            0                            },
	{ &cg_wwSmoothTime,          "cg_wwSmoothTime",          "300",          CVAR_ARCHIVE                 },
	{ &cg_wwFollow,              "cg_wwFollow",              "1",            CVAR_ARCHIVE | CVAR_USERINFO },
	{ &cg_wwToggle,              "cg_wwToggle",              "1",            CVAR_ARCHIVE | CVAR_USERINFO },
	{ &cg_depthSortParticles,    "cg_depthSortParticles",    "1",            CVAR_ARCHIVE                 },
	{ &cg_consoleLatency,        "cg_consoleLatency",        "3000",         CVAR_ARCHIVE                 },
	{ &cg_lightFlare,            "cg_lightFlare",            "3",            CVAR_ARCHIVE                 },
	{ &cg_debugParticles,        "cg_debugParticles",        "0",            CVAR_CHEAT                   },
	{ &cg_debugTrails,           "cg_debugTrails",           "0",            CVAR_CHEAT                   },
	{ &cg_debugPVS,              "cg_debugPVS",              "0",            CVAR_CHEAT                   },
	{ &cg_disableWarningDialogs, "cg_disableWarningDialogs", "0",            CVAR_ARCHIVE                 },
	{ &cg_disableScannerPlane,   "cg_disableScannerPlane",   "0",            CVAR_ARCHIVE                 },
	{ &cg_tutorial,              "cg_tutorial",              "1",            CVAR_ARCHIVE                 },
	{ &cg_hudFiles,              "cg_hudFiles",              "ui/hud.txt",   CVAR_ARCHIVE                 },

	{ &cg_painBlendUpRate,       "cg_painBlendUpRate",       "10.0",         0                            },
	{ &cg_painBlendDownRate,     "cg_painBlendDownRate",     "0.5",          0                            },
	{ &cg_painBlendMax,          "cg_painBlendMax",          "0.7",          0                            },
	{ &cg_painBlendScale,        "cg_painBlendScale",        "7.0",          0                            },
	{ &cg_painBlendZoom,         "cg_painBlendZoom",         "0.65",         0                            },

	{ &ui_currentClass,          "ui_currentClass",          "0",            0                            },
	{ &ui_carriage,              "ui_carriage",              "",             0                            },
	{ &ui_stages,                "ui_stages",                "0 0",          0                            },
	{ &ui_dialog,                "ui_dialog",                "Text not set", 0                            },
	{ &ui_loading,               "ui_loading",               "0",            0                            },
	{ &ui_voteActive,            "ui_voteActive",            "0",            0                            },
	{ &ui_humanTeamVoteActive,   "ui_humanTeamVoteActive",   "0",            0                            },
	{ &ui_alienTeamVoteActive,   "ui_alienTeamVoteActive",   "0",            0                            },

	{ &cg_debugRandom,           "cg_debugRandom",           "0",            0                            },

	// the following variables are created in other parts of the system,
	// but we also reference them here

	{ &cg_buildScript,           "com_buildScript",          "0",            0                            }, // force loading of all possible data amd error on failures
	{ &cg_paused,                "cl_paused",                "0",            CVAR_ROM                     },
	{ &cg_blood,                 "com_blood",                "1",            CVAR_ARCHIVE                 },
	{ &cg_synchronousClients,    "g_synchronousClients",     "0",            0                            }, // communicated by systeminfo
	{ &cg_cameraOrbit,           "cg_cameraOrbit",           "0",            CVAR_CHEAT                   },
	{ &cg_cameraOrbitDelay,      "cg_cameraOrbitDelay",      "50",           CVAR_ARCHIVE                 },
	{ &cg_timescaleFadeEnd,      "cg_timescaleFadeEnd",      "1",            0                            },
	{ &cg_timescaleFadeSpeed,    "cg_timescaleFadeSpeed",    "0",            0                            },
	{ &cg_timescale,             "timescale",                "1",            0                            },
	{ &cg_scorePlum,             "cg_scorePlums",            "1",            CVAR_USERINFO | CVAR_ARCHIVE },
	{ &cg_smoothClients,         "cg_smoothClients",         "0",            CVAR_USERINFO | CVAR_ARCHIVE },
	{ &cg_cameraMode,            "com_cameraMode",           "0",            CVAR_CHEAT                   },

	{ &pmove_fixed,              "pmove_fixed",              "0",            0                            },
	{ &pmove_msec,               "pmove_msec",               "8",            0                            },
	{ &cg_noTaunt,               "cg_noTaunt",               "0",            CVAR_ARCHIVE                 },
	{ &cg_noProjectileTrail,     "cg_noProjectileTrail",     "0",            CVAR_ARCHIVE                 },
	{ &cg_smallFont,             "ui_smallFont",             "0.2",          CVAR_ARCHIVE                 },
	{ &cg_bigFont,               "ui_bigFont",               "0.5",          CVAR_ARCHIVE                 },
	{ &cg_oldRail,               "cg_oldRail",               "1",            CVAR_ARCHIVE                 },
	{ &cg_oldRocket,             "cg_oldRocket",             "1",            CVAR_ARCHIVE                 },
	{ &cg_oldPlasma,             "cg_oldPlasma",             "1",            CVAR_ARCHIVE                 },
	{ &cg_trueLightning,         "cg_trueLightning",         "0.0",          CVAR_ARCHIVE                 }
//  { &cg_pmove_fixed, "cg_pmove_fixed", "0", CVAR_USERINFO | CVAR_ARCHIVE }
};

static int         cvarTableSize = sizeof( cvarTable ) / sizeof( cvarTable[ 0 ] );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void )
{
	int         i;
	cvarTable_t *cv;
	char        var[ MAX_TOKEN_CHARS ];

	for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
	{
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
		                    cv->defaultString, cv->cvarFlags );
	}

	//repress standard Q3 console
	trap_Cvar_Set( "con_notifytime", "-2" );

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer             = atoi( var );
	forceModelModificationCount = cg_forceModel.modificationCount;

	trap_Cvar_Register( NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register( NULL, "headmodel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register( NULL, "team_model", DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register( NULL, "team_headmodel", DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE );
}

/*
===================
CG_ForceModelChange
===================
*/
static void CG_ForceModelChange( void )
{
	int i;

	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		const char *clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS + i );

		if ( !clientInfo[ 0 ] )
		{
			continue;
		}

		CG_NewClientInfo( i );
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void )
{
	int         i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
	{
		trap_Cvar_Update( cv->vmCvar );
	}

	// check for modications here

	// if force model changed
	if ( forceModelModificationCount != cg_forceModel.modificationCount )
	{
		forceModelModificationCount = cg_forceModel.modificationCount;
		CG_ForceModelChange();
	}
}

int CG_CrosshairPlayer( void )
{
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) )
	{
		return -1;
	}

	return cg.crosshairClientNum;
}

int CG_LastAttacker( void )
{
	if ( !cg.attackerTime )
	{
		return -1;
	}

	return cg.snap->ps.persistant[ PERS_ATTACKER ];
}

/*
=================
CG_RemoveNotifyLine
=================
*/
void CG_RemoveNotifyLine( void )
{
	int i, offset, totalLength;

	if ( cg.numConsoleLines == 0 )
	{
		return;
	}

	offset      = cg.consoleLines[ 0 ].length;
	totalLength = strlen( cg.consoleText ) - offset;

	//slide up consoleText
	for ( i = 0; i <= totalLength; i++ )
	{
		cg.consoleText[ i ] = cg.consoleText[ i + offset ];
	}

	//pop up the first consoleLine
	for ( i = 0; i < cg.numConsoleLines; i++ )
	{
		cg.consoleLines[ i ] = cg.consoleLines[ i + 1 ];
	}

	cg.numConsoleLines--;
}

/*
=================
CG_AddNotifyText
=================
*/
void CG_AddNotifyText( void )
{
	char buffer[ BIG_INFO_STRING ];

	trap_LiteralArgs( buffer, BIG_INFO_STRING );

	if ( !buffer[ 0 ] )
	{
		cg.consoleText[ 0 ] = '\0';
		cg.numConsoleLines  = 0;
		return;
	}

	if ( cg.numConsoleLines == MAX_CONSOLE_LINES )
	{
		CG_RemoveNotifyLine();
	}

	Q_strcat( cg.consoleText, MAX_CONSOLE_TEXT, buffer );
	cg.consoleLines[ cg.numConsoleLines ].time   = cg.time;
	cg.consoleLines[ cg.numConsoleLines ].length = strlen( buffer );
	cg.numConsoleLines++;
}

void QDECL CG_Printf( const char *msg, ... )
{
	va_list argptr;
	char    text[ 1024 ];

	va_start( argptr, msg );
	vsprintf( text, msg, argptr );
	va_end( argptr );

	trap_Print( text );
}

void QDECL CG_Error( const char *msg, ... )
{
	va_list argptr;
	char    text[ 1024 ];

	va_start( argptr, msg );
	vsprintf( text, msg, argptr );
	va_end( argptr );

	trap_Error( text );
}

void QDECL Com_Error( int level, const char *error, ... )
{
	va_list argptr;
	char    text[ 1024 ];

	va_start( argptr, error );
	vsprintf( text, error, argptr );
	va_end( argptr );

	trap_Error( text );
}

void QDECL Com_Printf( const char *msg, ... )
{
	va_list argptr;
	char    text[ 1024 ];

	va_start ( argptr, msg );
	vsprintf ( text, msg, argptr );
	va_end ( argptr );

	trap_Print( text );
}

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg )
{
	static char buffer[ MAX_STRING_CHARS ];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}

//========================================================================

/*
=================
CG_FileExists

Test if a specific file exists or not
=================
*/
qboolean CG_FileExists( char *filename )
{
	fileHandle_t f;

	if ( trap_FS_FOpenFile( filename, &f, FS_READ ) > 0 )
	{
		//file exists so close it
		trap_FS_FCloseFile( f );

		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void )
{
	int        i;
	char       name[ MAX_QPATH ];
	const char *soundName;

	cgs.media.alienStageTransition = trap_S_RegisterSound( "sound/announcements/overmindevolved.wav", qtrue );
	cgs.media.humanStageTransition = trap_S_RegisterSound( "sound/announcements/reinforcement.wav", qtrue );

	cgs.media.alienOvermindAttack  = trap_S_RegisterSound( "sound/announcements/overmindattack.wav", qtrue );
	cgs.media.alienOvermindDying   = trap_S_RegisterSound( "sound/announcements/overminddying.wav", qtrue );
	cgs.media.alienOvermindSpawns  = trap_S_RegisterSound( "sound/announcements/overmindspawns.wav", qtrue );

	cgs.media.alienL1Grab          = trap_S_RegisterSound( "sound/player/level1/grab.wav", qtrue );
	cgs.media.alienL4ChargePrepare = trap_S_RegisterSound( "sound/player/level4/charge_prepare.wav", qtrue );
	cgs.media.alienL4ChargeStart   = trap_S_RegisterSound( "sound/player/level4/charge_start.wav", qtrue );

	cgs.media.tracerSound          = trap_S_RegisterSound( "sound/weapons/tracer.wav", qfalse );
	cgs.media.selectSound          = trap_S_RegisterSound( "sound/weapons/change.wav", qfalse );

	cgs.media.talkSound            = trap_S_RegisterSound( "sound/misc/talk.wav", qfalse );
	cgs.media.alienTalkSound       = trap_S_RegisterSound( "sound/misc/alien_talk.wav", qfalse );
	cgs.media.humanTalkSound       = trap_S_RegisterSound( "sound/misc/human_talk.wav", qfalse );
	cgs.media.landSound            = trap_S_RegisterSound( "sound/player/land1.wav", qfalse );

	cgs.media.watrInSound          = trap_S_RegisterSound( "sound/player/watr_in.wav", qfalse );
	cgs.media.watrOutSound         = trap_S_RegisterSound( "sound/player/watr_out.wav", qfalse );
	cgs.media.watrUnSound          = trap_S_RegisterSound( "sound/player/watr_un.wav", qfalse );

	cgs.media.disconnectSound      = trap_S_RegisterSound( "sound/misc/disconnect.wav", qfalse );

	for ( i = 0; i < 4; i++ )
	{
		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/step%i.wav", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_NORMAL ][ i ] = trap_S_RegisterSound( name, qfalse );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/flesh%i.wav", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_FLESH ][ i ]  = trap_S_RegisterSound( name, qfalse );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/splash%i.wav", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_SPLASH ][ i ] = trap_S_RegisterSound( name, qfalse );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/clank%i.wav", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_METAL ][ i ]  = trap_S_RegisterSound( name, qfalse );
	}

	for ( i = 1; i < MAX_SOUNDS; i++ )
	{
		soundName = CG_ConfigString( CS_SOUNDS + i );

		if ( !soundName[ 0 ] )
		{
			break;
		}

		if ( soundName[ 0 ] == '*' )
		{
			continue; // custom sound
		}

		cgs.gameSounds[ i ] = trap_S_RegisterSound( soundName, qfalse );
	}

	cgs.media.jetpackDescendSound     = trap_S_RegisterSound( "sound/upgrades/jetpack/low.wav", qfalse );
	cgs.media.jetpackIdleSound        = trap_S_RegisterSound( "sound/upgrades/jetpack/idle.wav", qfalse );
	cgs.media.jetpackAscendSound      = trap_S_RegisterSound( "sound/upgrades/jetpack/hi.wav", qfalse );

	cgs.media.medkitUseSound          = trap_S_RegisterSound( "sound/upgrades/medkit/medkit.wav", qfalse );

	cgs.media.alienEvolveSound        = trap_S_RegisterSound( "sound/player/alienevolve.wav", qfalse );

	cgs.media.alienBuildableExplosion = trap_S_RegisterSound( "sound/buildables/alien/explosion.wav", qfalse );
	cgs.media.alienBuildableDamage    = trap_S_RegisterSound( "sound/buildables/alien/damage.wav", qfalse );
	cgs.media.alienBuildablePrebuild  = trap_S_RegisterSound( "sound/buildables/alien/prebuild.wav", qfalse );

	cgs.media.humanBuildableExplosion = trap_S_RegisterSound( "sound/buildables/human/explosion.wav", qfalse );
	cgs.media.humanBuildablePrebuild  = trap_S_RegisterSound( "sound/buildables/human/prebuild.wav", qfalse );

	for ( i = 0; i < 4; i++ )
	{
		cgs.media.humanBuildableDamage[ i ] = trap_S_RegisterSound(
		                                        va( "sound/buildables/human/damage%d.wav", i ), qfalse );
	}

	cgs.media.hardBounceSound1       = trap_S_RegisterSound( "sound/misc/hard_bounce1.wav", qfalse );
	cgs.media.hardBounceSound2       = trap_S_RegisterSound( "sound/misc/hard_bounce2.wav", qfalse );

	cgs.media.repeaterUseSound       = trap_S_RegisterSound( "sound/buildables/repeater/use.wav", qfalse );

	cgs.media.buildableRepairSound   = trap_S_RegisterSound( "sound/buildables/human/repair.wav", qfalse );
	cgs.media.buildableRepairedSound = trap_S_RegisterSound( "sound/buildables/human/repaired.wav", qfalse );

	cgs.media.lCannonWarningSound    = trap_S_RegisterSound( "models/weapons/lcannon/warning.wav", qfalse );
}

//===================================================================================

/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void )
{
	int         i;
	static char *sb_nums[ 11 ] =
	{
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};
	static char *buildWeaponTimerPieShaders[ 8 ] =
	{
		"ui/assets/neutral/1_5pie",
		"ui/assets/neutral/3_0pie",
		"ui/assets/neutral/4_5pie",
		"ui/assets/neutral/6_0pie",
		"ui/assets/neutral/7_5pie",
		"ui/assets/neutral/9_0pie",
		"ui/assets/neutral/10_5pie",
		"ui/assets/neutral/12_0pie",
	};

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	trap_R_LoadWorldMap( cgs.mapname );
	CG_UpdateMediaFraction( 0.66f );

	for ( i = 0; i < 11; i++ )
	{
		cgs.media.numberShaders[ i ] = trap_R_RegisterShader( sb_nums[ i ] );
	}

	cgs.media.viewBloodShader     = trap_R_RegisterShader( "gfx/damage/fullscreen_painblend" );

	cgs.media.connectionShader    = trap_R_RegisterShader( "gfx/2d/net" );

	cgs.media.creepShader         = trap_R_RegisterShader( "creep" );

	cgs.media.scannerBlipShader   = trap_R_RegisterShader( "gfx/2d/blip" );
	cgs.media.scannerLineShader   = trap_R_RegisterShader( "gfx/2d/stalk" );

	cgs.media.tracerShader        = trap_R_RegisterShader( "gfx/misc/tracer" );

	cgs.media.backTileShader      = trap_R_RegisterShader( "console" );

	//TA: building shaders
	cgs.media.greenBuildShader    = trap_R_RegisterShader( "gfx/misc/greenbuild" );
	cgs.media.redBuildShader      = trap_R_RegisterShader( "gfx/misc/redbuild" );
	cgs.media.noPowerShader       = trap_R_RegisterShader( "gfx/misc/nopower" );
	cgs.media.humanSpawningShader = trap_R_RegisterShader( "models/buildables/telenode/rep_cyl" );

	for ( i = 0; i < 8; i++ )
	{
		cgs.media.buildWeaponTimerPie[ i ] = trap_R_RegisterShader( buildWeaponTimerPieShaders[ i ] );
	}

	cgs.media.upgradeClassIconShader = trap_R_RegisterShader( "icons/icona_upgrade.tga" );

	cgs.media.balloonShader          = trap_R_RegisterShader( "gfx/sprites/chatballoon" );

	cgs.media.disconnectPS           = CG_RegisterParticleSystem( "disconnectPS" );

	CG_UpdateMediaFraction( 0.7f );

	memset( cg_weapons, 0, sizeof( cg_weapons ) );
	memset( cg_upgrades, 0, sizeof( cg_upgrades ) );

	cgs.media.shadowMarkShader          = trap_R_RegisterShader( "gfx/marks/shadow" );
	cgs.media.wakeMarkShader            = trap_R_RegisterShader( "gfx/marks/wake" );

	cgs.media.poisonCloudPS             = CG_RegisterParticleSystem( "firstPersonPoisonCloudPS" );
	cgs.media.alienEvolvePS             = CG_RegisterParticleSystem( "alienEvolvePS" );
	cgs.media.alienAcidTubePS           = CG_RegisterParticleSystem( "alienAcidTubePS" );

	cgs.media.jetPackDescendPS          = CG_RegisterParticleSystem( "jetPackDescendPS" );
	cgs.media.jetPackHoverPS            = CG_RegisterParticleSystem( "jetPackHoverPS" );
	cgs.media.jetPackAscendPS           = CG_RegisterParticleSystem( "jetPackAscendPS" );

	cgs.media.humanBuildableDamagedPS   = CG_RegisterParticleSystem( "humanBuildableDamagedPS" );
	cgs.media.alienBuildableDamagedPS   = CG_RegisterParticleSystem( "alienBuildableDamagedPS" );
	cgs.media.humanBuildableDestroyedPS = CG_RegisterParticleSystem( "humanBuildableDestroyedPS" );
	cgs.media.alienBuildableDestroyedPS = CG_RegisterParticleSystem( "alienBuildableDestroyedPS" );

	cgs.media.alienBleedPS              = CG_RegisterParticleSystem( "alienBleedPS" );
	cgs.media.humanBleedPS              = CG_RegisterParticleSystem( "humanBleedPS" );

	// register the inline models
	cgs.numInlineModels                 = trap_CM_NumInlineModels();

	for ( i = 1; i < cgs.numInlineModels; i++ )
	{
		char   name[ 10 ];
		vec3_t mins, maxs;
		int    j;

		Com_sprintf( name, sizeof( name ), "*%i", i );

		cgs.inlineDrawModel[ i ] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[ i ], mins, maxs );

		for ( j = 0; j < 3; j++ )
		{
			cgs.inlineModelMidpoints[ i ][ j ] = mins[ j ] + 0.5 * ( maxs[ j ] - mins[ j ] );
		}
	}

	// register all the server specified models
	for ( i = 1; i < MAX_MODELS; i++ )
	{
		const char *modelName;

		modelName = CG_ConfigString( CS_MODELS + i );

		if ( !modelName[ 0 ] )
		{
			break;
		}

		cgs.gameModels[ i ] = trap_R_RegisterModel( modelName );
	}

	CG_UpdateMediaFraction( 0.8f );

	// register all the server specified shaders
	for ( i = 1; i < MAX_GAME_SHADERS; i++ )
	{
		const char *shaderName;

		shaderName = CG_ConfigString( CS_SHADERS + i );

		if ( !shaderName[ 0 ] )
		{
			break;
		}

		cgs.gameShaders[ i ] = trap_R_RegisterShader( shaderName );
	}

	CG_UpdateMediaFraction( 0.9f );

	// register all the server specified particle systems
	for ( i = 1; i < MAX_GAME_PARTICLE_SYSTEMS; i++ )
	{
		const char *psName;

		psName = CG_ConfigString( CS_PARTICLE_SYSTEMS + i );

		if ( !psName[ 0 ] )
		{
			break;
		}

		cgs.gameParticleSystems[ i ] = CG_RegisterParticleSystem( ( char * )psName );
	}
}

/*
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString( void )
{
	int i;

	cg.spectatorList[ 0 ] = 0;

	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( cgs.clientinfo[ i ].infoValid && cgs.clientinfo[ i ].team == PTE_NONE )
		{
			Q_strcat( cg.spectatorList, sizeof( cg.spectatorList ), va( "%s     " S_COLOR_WHITE, cgs.clientinfo[ i ].name ) );
		}
	}

	i = strlen( cg.spectatorList );

	if ( i != cg.spectatorLen )
	{
		cg.spectatorLen   = i;
		cg.spectatorWidth = -1;
	}
}

/*
===================
CG_RegisterClients

===================
*/
static void CG_RegisterClients( void )
{
	int i;

	cg.charModelFraction = 0.0f;

	//precache all the models/sounds/etc
	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		CG_PrecacheClientInfo( i, BG_FindModelNameForClass( i ),
		                       BG_FindSkinNameForClass( i ) );

		cg.charModelFraction = ( float )i / ( float )PCL_NUM_CLASSES;
		trap_UpdateScreen();
	}

	cgs.media.larmourHeadSkin   = trap_R_RegisterSkin( "models/players/human_base/head_light.skin" );
	cgs.media.larmourLegsSkin   = trap_R_RegisterSkin( "models/players/human_base/lower_light.skin" );
	cgs.media.larmourTorsoSkin  = trap_R_RegisterSkin( "models/players/human_base/upper_light.skin" );

	cgs.media.jetpackModel      = trap_R_RegisterModel( "models/players/human_base/jetpack.md3" );
	cgs.media.jetpackFlashModel = trap_R_RegisterModel( "models/players/human_base/jetpack_flash.md3" );
	cgs.media.battpackModel     = trap_R_RegisterModel( "models/players/human_base/battpack.md3" );

	cg.charModelFraction        = 1.0f;
	trap_UpdateScreen();

	//load all the clientinfos of clients already connected to the server
	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		const char *clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS + i );

		if ( !clientInfo[ 0 ] )
		{
			continue;
		}

		CG_NewClientInfo( i );
	}

	CG_BuildSpectatorString();
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index )
{
	if ( index < 0 || index >= MAX_CONFIGSTRINGS )
	{
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}

	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( void )
{
	char *s;
	char parm1[ MAX_QPATH ], parm2[ MAX_QPATH ];

	// start the background music
	s = ( char * )CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
}

/*
======================
CG_PlayerCount
======================
*/
int CG_PlayerCount( void )
{
	int i, count = 0;

	CG_RequestScores();

	for ( i = 0; i < cg.numScores; i++ )
	{
		if ( cg.scores[ i ].team == PTE_ALIENS ||
		     cg.scores[ i ].team == PTE_HUMANS )
		{
			count++;
		}
	}

	return count;
}

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
char *CG_GetMenuBuffer( const char *filename )
{
	int          len;
	fileHandle_t f;
	static char  buf[ MAX_MENUFILE ];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( !f )
	{
		trap_Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return NULL;
	}

	if ( len >= MAX_MENUFILE )
	{
		trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i",
		                filename, len, MAX_MENUFILE ) );
		trap_FS_FCloseFile( f );
		return NULL;
	}

	trap_FS_Read( buf, len, f );
	buf[ len ] = 0;
	trap_FS_FCloseFile( f );

	return buf;
}

qboolean CG_Asset_Parse( int handle )
{
	pc_token_t token;
	const char *tempStr;

	if ( !trap_PC_ReadToken( handle, &token ) )
	{
		return qfalse;
	}

	if ( Q_stricmp( token.string, "{" ) != 0 )
	{
		return qfalse;
	}

	while ( 1 )
	{
		if ( !trap_PC_ReadToken( handle, &token ) )
		{
			return qfalse;
		}

		if ( Q_stricmp( token.string, "}" ) == 0 )
		{
			return qtrue;
		}

		// font
		if ( Q_stricmp( token.string, "font" ) == 0 )
		{
			int pointSize;

			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
			{
				return qfalse;
			}

			cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.textFont );
			continue;
		}

		// smallFont
		if ( Q_stricmp( token.string, "smallFont" ) == 0 )
		{
			int pointSize;

			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
			{
				return qfalse;
			}

			cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.smallFont );
			continue;
		}

		// font
		if ( Q_stricmp( token.string, "bigfont" ) == 0 )
		{
			int pointSize;

			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
			{
				return qfalse;
			}

			cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.bigFont );
			continue;
		}

		// gradientbar
		if ( Q_stricmp( token.string, "gradientbar" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( tempStr );
			continue;
		}

		// enterMenuSound
		if ( Q_stricmp( token.string, "menuEnterSound" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			cgDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// exitMenuSound
		if ( Q_stricmp( token.string, "menuExitSound" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			cgDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// itemFocusSound
		if ( Q_stricmp( token.string, "itemFocusSound" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			cgDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// menuBuzzSound
		if ( Q_stricmp( token.string, "menuBuzzSound" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if ( Q_stricmp( token.string, "cursor" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &cgDC.Assets.cursorStr ) )
			{
				return qfalse;
			}

			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr );
			continue;
		}

		if ( Q_stricmp( token.string, "fadeClamp" ) == 0 )
		{
			if ( !PC_Float_Parse( handle, &cgDC.Assets.fadeClamp ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "fadeCycle" ) == 0 )
		{
			if ( !PC_Int_Parse( handle, &cgDC.Assets.fadeCycle ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "fadeAmount" ) == 0 )
		{
			if ( !PC_Float_Parse( handle, &cgDC.Assets.fadeAmount ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "shadowX" ) == 0 )
		{
			if ( !PC_Float_Parse( handle, &cgDC.Assets.shadowX ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "shadowY" ) == 0 )
		{
			if ( !PC_Float_Parse( handle, &cgDC.Assets.shadowY ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "shadowColor" ) == 0 )
		{
			if ( !PC_Color_Parse( handle, &cgDC.Assets.shadowColor ) )
			{
				return qfalse;
			}

			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[ 3 ];
			continue;
		}
	}

	return qfalse; // bk001204 - why not?
}

void CG_ParseMenu( const char *menuFile )
{
	pc_token_t token;
	int        handle;

	handle = trap_PC_LoadSource( menuFile );

	if ( !handle )
	{
		handle = trap_PC_LoadSource( "ui/testhud.menu" );
	}

	if ( !handle )
	{
		return;
	}

	while ( 1 )
	{
		if ( !trap_PC_ReadToken( handle, &token ) )
		{
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//  Com_Printf( "Missing { in menu file\n" );
		//  break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//  Com_Printf( "Too many menus!\n" );
		//  break;
		//}

		if ( token.string[ 0 ] == '}' )
		{
			break;
		}

		if ( Q_stricmp( token.string, "assetGlobalDef" ) == 0 )
		{
			if ( CG_Asset_Parse( handle ) )
			{
				continue;
			}
			else
			{
				break;
			}
		}

		if ( Q_stricmp( token.string, "menudef" ) == 0 )
		{
			// start a new menu
			Menu_New( handle );
		}
	}

	trap_PC_FreeSource( handle );
}

qboolean CG_Load_Menu( char **p )
{
	char *token;

	token = COM_ParseExt( p, qtrue );

	if ( token[ 0 ] != '{' )
	{
		return qfalse;
	}

	while ( 1 )
	{
		token = COM_ParseExt( p, qtrue );

		if ( Q_stricmp( token, "}" ) == 0 )
		{
			return qtrue;
		}

		if ( !token || token[ 0 ] == 0 )
		{
			return qfalse;
		}

		CG_ParseMenu( token );
	}

	return qfalse;
}

void CG_LoadMenus( const char *menuFile )
{
	char         *token;
	char         *p;
	int          len, start;
	fileHandle_t f;
	static char  buf[ MAX_MENUDEFFILE ];

	start = trap_Milliseconds();

	len   = trap_FS_FOpenFile( menuFile, &f, FS_READ );

	if ( !f )
	{
		CG_Printf( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile );
		len = trap_FS_FOpenFile( "ui/hud.txt", &f, FS_READ );

		if ( !f )
		{
			trap_Error( S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!\n" );
		}
	}

	if ( len >= MAX_MENUDEFFILE )
	{
		trap_Error( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i",
		                menuFile, len, MAX_MENUDEFFILE ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[ len ] = 0;
	trap_FS_FCloseFile( f );

	COM_Compress( buf );

	Menu_Reset();

	p = buf;

	while ( 1 )
	{
		token = COM_ParseExt( &p, qtrue );

		if ( !token || token[ 0 ] == 0 || token[ 0 ] == '}' )
		{
			break;
		}

		if ( Q_stricmp( token, "}" ) == 0 )
		{
			break;
		}

		if ( Q_stricmp( token, "loadmenu" ) == 0 )
		{
			if ( CG_Load_Menu( &p ) )
			{
				continue;
			}
			else
			{
				break;
			}
		}
	}

	Com_Printf( "UI menu load time = %d milli seconds\n", trap_Milliseconds() - start );
}

static qboolean CG_OwnerDrawHandleKey( int ownerDraw, int flags, float *special, int key )
{
	return qfalse;
}

static int CG_FeederCount( float feederID )
{
	int i, count = 0;

	if ( feederID == FEEDER_ALIENTEAM_LIST )
	{
		for ( i = 0; i < cg.numScores; i++ )
		{
			if ( cg.scores[ i ].team == PTE_ALIENS )
			{
				count++;
			}
		}
	}
	else if ( feederID == FEEDER_HUMANTEAM_LIST )
	{
		for ( i = 0; i < cg.numScores; i++ )
		{
			if ( cg.scores[ i ].team == PTE_HUMANS )
			{
				count++;
			}
		}
	}

	return count;
}

void CG_SetScoreSelection( void *p )
{
	menuDef_t     *menu = ( menuDef_t * )p;
	playerState_t *ps   = &cg.snap->ps;
	int           i, alien, human;
	int           feeder;

	alien = human = 0;

	for ( i = 0; i < cg.numScores; i++ )
	{
		if ( cg.scores[ i ].team == PTE_ALIENS )
		{
			alien++;
		}
		else if ( cg.scores[ i ].team == PTE_HUMANS )
		{
			human++;
		}

		if ( ps->clientNum == cg.scores[ i ].client )
		{
			cg.selectedScore = i;
		}
	}

	if ( menu == NULL )
	{
		// just interested in setting the selected score
		return;
	}

	feeder = FEEDER_ALIENTEAM_LIST;
	i      = alien;

	if ( cg.scores[ cg.selectedScore ].team == PTE_HUMANS )
	{
		feeder = FEEDER_HUMANTEAM_LIST;
		i      = human;
	}

	Menu_SetFeederSelection( menu, feeder, i, NULL );
}

// FIXME: might need to cache this info
static clientInfo_t *CG_InfoFromScoreIndex( int index, int team, int *scoreIndex )
{
	int i, count;
	count = 0;

	for ( i = 0; i < cg.numScores; i++ )
	{
		if ( cg.scores[ i ].team == team )
		{
			if ( count == index )
			{
				*scoreIndex = i;
				return &cgs.clientinfo[ cg.scores[ i ].client ];
			}

			count++;
		}
	}

	*scoreIndex = index;
	return &cgs.clientinfo[ cg.scores[ index ].client ];
}

static const char *CG_FeederItemText( float feederID, int index, int column, qhandle_t *handle )
{
	int          scoreIndex = 0;
	clientInfo_t *info      = NULL;
	int          team       = -1;
	score_t      *sp        = NULL;
	qboolean     showIcons  = qfalse;

	*handle = -1;

	if ( feederID == FEEDER_ALIENTEAM_LIST )
	{
		team = PTE_ALIENS;
	}
	else if ( feederID == FEEDER_HUMANTEAM_LIST )
	{
		team = PTE_HUMANS;
	}

	info = CG_InfoFromScoreIndex( index, team, &scoreIndex );
	sp   = &cg.scores[ scoreIndex ];

	if ( ( atoi( CG_ConfigString( CS_CLIENTS_READY ) ) & ( 1 << sp->client ) ) &&
	     cg.intermissionStarted )
	{
		showIcons = qfalse;
	}
	else if ( cg.snap->ps.pm_type == PM_SPECTATOR || cg.snap->ps.pm_flags & PMF_FOLLOW ||
	          team == cg.snap->ps.stats[ STAT_PTEAM ] || cg.intermissionStarted )
	{
		showIcons = qtrue;
	}

	if ( info && info->infoValid )
	{
		switch ( column )
		{
			case 0:
				if ( showIcons )
				{
					if ( sp->weapon != WP_NONE )
					{
						*handle = cg_weapons[ sp->weapon ].weaponIcon;
					}
				}

				break;

			case 1:
				if ( showIcons )
				{
					if ( sp->team == PTE_HUMANS && sp->upgrade != UP_NONE )
					{
						*handle = cg_upgrades[ sp->upgrade ].upgradeIcon;
					}
					else if ( sp->team == PTE_ALIENS )
					{
						switch ( sp->weapon )
						{
							case WP_ABUILD2:
							case WP_ALEVEL1_UPG:
							case WP_ALEVEL2_UPG:
							case WP_ALEVEL3_UPG:
								*handle = cgs.media.upgradeClassIconShader;
								break;

							default:
								break;
						}
					}
				}

				break;

			case 2:
				if ( ( atoi( CG_ConfigString( CS_CLIENTS_READY ) ) & ( 1 << sp->client ) ) &&
				     cg.intermissionStarted )
				{
					return "Ready";
				}

				break;

			case 3:
				return info->name;
				break;

			case 4:
				return va( "%d", info->score );
				break;

			case 5:
				return va( "%4d", sp->time );
				break;

			case 6:
				if ( sp->ping == -1 )
				{
					return "connecting";
				}

				return va( "%4d", sp->ping );
				break;
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage( float feederID, int index )
{
	return 0;
}

static void CG_FeederSelection( float feederID, int index )
{
	int i, count;
	int team = ( feederID == FEEDER_ALIENTEAM_LIST ) ? PTE_ALIENS : PTE_HUMANS;
	count = 0;

	for ( i = 0; i < cg.numScores; i++ )
	{
		if ( cg.scores[ i ].team == team )
		{
			if ( index == count )
			{
				cg.selectedScore = i;
			}

			count++;
		}
	}
}

static float CG_Cvar_Get( const char *cvar )
{
	char buff[ 128 ];

	memset( buff, 0, sizeof( buff ) );
	trap_Cvar_VariableStringBuffer( cvar, buff, sizeof( buff ) );
	return atof( buff );
}

void CG_Text_PaintWithCursor( float x, float y, float scale, vec4_t color, const char *text,
                              int cursorPos, char cursor, int limit, int style )
{
	CG_Text_Paint( x, y, scale, color, text, 0, limit, style );
}

static int CG_OwnerDrawWidth( int ownerDraw, float scale )
{
	switch ( ownerDraw )
	{
		case CG_KILLER:
			return CG_Text_Width( CG_GetKillerText(), scale, 0 );
			break;
	}

	return 0;
}

static int CG_PlayCinematic( const char *name, float x, float y, float w, float h )
{
	return trap_CIN_PlayCinematic( name, x, y, w, h, CIN_loop );
}

static void CG_StopCinematic( int handle )
{
	trap_CIN_StopCinematic( handle );
}

static void CG_DrawCinematic( int handle, float x, float y, float w, float h )
{
	trap_CIN_SetExtents( handle, x, y, w, h );
	trap_CIN_DrawCinematic( handle );
}

static void CG_RunCinematicFrame( int handle )
{
	trap_CIN_RunCinematic( handle );
}

//TA: hack to prevent warning
static qboolean CG_OwnerDrawVisible( int parameter )
{
	return qfalse;
}

/*
=================
CG_LoadHudMenu
=================
*/
void CG_LoadHudMenu( void )
{
	char       buff[ 1024 ];
	const char *hudSet;

	cgDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	cgDC.setColor            = &trap_R_SetColor;
	cgDC.drawHandlePic       = &CG_DrawPic;
	cgDC.drawStretchPic      = &trap_R_DrawStretchPic;
	cgDC.drawText            = &CG_Text_Paint;
	cgDC.textWidth           = &CG_Text_Width;
	cgDC.textHeight          = &CG_Text_Height;
	cgDC.registerModel       = &trap_R_RegisterModel;
	cgDC.modelBounds         = &trap_R_ModelBounds;
	cgDC.fillRect            = &CG_FillRect;
	cgDC.drawRect            = &CG_DrawRect;
	cgDC.drawSides           = &CG_DrawSides;
	cgDC.drawTopBottom       = &CG_DrawTopBottom;
	cgDC.clearScene          = &trap_R_ClearScene;
	cgDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	cgDC.renderScene         = &trap_R_RenderScene;
	cgDC.registerFont        = &trap_R_RegisterFont;
	cgDC.ownerDrawItem       = &CG_OwnerDraw;
	cgDC.getValue            = &CG_GetValue;
	cgDC.ownerDrawVisible    = &CG_OwnerDrawVisible;
	cgDC.runScript           = &CG_RunMenuScript;
	cgDC.getTeamColor        = &CG_GetTeamColor;
	cgDC.setCVar             = trap_Cvar_Set;
	cgDC.getCVarString       = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue        = CG_Cvar_Get;
	cgDC.drawTextWithCursor  = &CG_Text_PaintWithCursor;
	//cgDC.setOverstrikeMode    = &trap_Key_SetOverstrikeMode;
	//cgDC.getOverstrikeMode    = &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound     = &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey  = &CG_OwnerDrawHandleKey;
	cgDC.feederCount         = &CG_FeederCount;
	cgDC.feederItemImage     = &CG_FeederItemImage;
	cgDC.feederItemText      = &CG_FeederItemText;
	cgDC.feederSelection     = &CG_FeederSelection;
	//cgDC.setBinding           = &trap_Key_SetBinding;
	//cgDC.getBindingBuf        = &trap_Key_GetBindingBuf;
	//cgDC.keynumToStringBuf    = &trap_Key_KeynumToStringBuf;
	//cgDC.executeText          = &trap_Cmd_ExecuteText;
	cgDC.Error                = &Com_Error;
	cgDC.Print                = &Com_Printf;
	cgDC.ownerDrawWidth       = &CG_OwnerDrawWidth;
	//cgDC.Pause                = &CG_Pause;
	cgDC.registerSound        = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack  = &trap_S_StopBackgroundTrack;
	cgDC.playCinematic        = &CG_PlayCinematic;
	cgDC.stopCinematic        = &CG_StopCinematic;
	cgDC.drawCinematic        = &CG_DrawCinematic;
	cgDC.runCinematicFrame    = &CG_RunCinematicFrame;

	Init_Display( &cgDC );

	Menu_Reset();

	trap_Cvar_VariableStringBuffer( "cg_hudFiles", buff, sizeof( buff ) );
	hudSet = buff;

	if ( hudSet[ 0 ] == '\0' )
	{
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus( hudSet );
}

void CG_AssetCache( void )
{
	cgDC.Assets.gradientBar         = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	cgDC.Assets.scrollBar           = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	cgDC.Assets.scrollBarArrowDown  = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	cgDC.Assets.scrollBarArrowUp    = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	cgDC.Assets.scrollBarArrowLeft  = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	cgDC.Assets.scrollBarThumb      = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	cgDC.Assets.sliderBar           = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	cgDC.Assets.sliderThumb         = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
}

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum )
{
	const char *s;

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof( cg_entities ) );

	cg.clientNum              = clientNum;

	cgs.processedSnapshotNum  = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	cgs.media.whiteShader     = trap_R_RegisterShader( "white" );
	cgs.media.charsetShader   = trap_R_RegisterShader( "gfx/2d/bigchars" );
	cgs.media.outlineShader   = trap_R_RegisterShader( "outline" );

	//inform UI to repress cursor whilst loading
	trap_Cvar_Set( "ui_loading", "1" );

	//TA: load overrides
	BG_InitClassOverrides();
	BG_InitBuildableOverrides();
	BG_InitAllowedGameElements();

	//TA: dyn memory
	CG_InitMemory();

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	//TA: moved up for LoadHudMenu
	String_Init();

	//TA: TA UI
	CG_AssetCache();
	CG_LoadHudMenu(); // load new hud stuff

	cg.weaponSelect = WP_NONE;

	// old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );

	if ( strcmp( s, GAME_VERSION ) )
	{
		CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );
	}

	s                  = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	CG_ParseServerinfo();

	// load the new map
	trap_CM_LoadMap( cgs.mapname );

	cg.loading = qtrue; // force players to load instead of defer

	CG_LoadTrailSystems();
	CG_UpdateMediaFraction( 0.05f );

	CG_LoadParticleSystems();
	CG_UpdateMediaFraction( 0.05f );

	CG_RegisterSounds();
	CG_UpdateMediaFraction( 0.60f );

	CG_RegisterGraphics();
	CG_UpdateMediaFraction( 0.90f );

	CG_InitWeapons();
	CG_UpdateMediaFraction( 0.95f );

	CG_InitUpgrades();
	CG_UpdateMediaFraction( 1.0f );

	//TA:
	CG_InitBuildables();

	CG_RegisterClients(); // if low on memory, some clients will be deferred

	cg.loading = qfalse;  // future players will be deferred

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[ 0 ] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds( qtrue );

	trap_Cvar_Set( "ui_loading", "0" );
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void )
{
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}
