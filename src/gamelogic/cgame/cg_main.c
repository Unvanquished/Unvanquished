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

// cg_main.c -- initialization and primary entry point for cgame

#include "cg_local.h"

#include "../ui/ui_shared.h"
// display context for new ui stuff
displayContextDef_t cgDC;

void                CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void                CG_RegisterCvars( void );
void                CG_Shutdown( void );
static char         *CG_VoIPString( void );

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
EXTERN_C Q_EXPORT
intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4,
                 int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11 )
{
	Q_UNUSED(arg3); Q_UNUSED(arg4);  Q_UNUSED(arg5);
	Q_UNUSED(arg6); Q_UNUSED(arg7);  Q_UNUSED(arg8);
	Q_UNUSED(arg9); Q_UNUSED(arg10); Q_UNUSED(arg11);
	switch ( command )
	{
		case CG_INIT:
			CG_Init( arg0, arg1, arg2 );
			return 0;

		case CG_INIT_CVARS:
			trap_SyscallABIVersion( SYSCALL_ABI_VERSION_MAJOR, SYSCALL_ABI_VERSION_MINOR );
			CG_RegisterCvars();
			CG_Shutdown();
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
			CG_DrawActiveFrame( arg0, (stereoFrame_t) arg1, arg2 );
			return 0;

		case CG_CROSSHAIR_PLAYER:
			return CG_CrosshairPlayer();

		case CG_KEY_EVENT:
			if ( arg1 & KEYEVSTATE_CHAR )
			{
				CG_KeyEvent( 0, arg0, arg1 );
			}
			else
			{
				CG_KeyEvent( arg0, 0, arg1 );
			}
			return 0;

		case CG_MOUSE_EVENT:
			// cgame doesn't care where the cursor is
			return 0;

		case CG_VOIP_STRING:
			return ( intptr_t ) CG_VoIPString();

		case CG_COMPLETE_COMMAND:
			CG_CompleteCommand( arg0 );
			return 0;

		default:
			CG_Error( "vmMain(): unknown cgame command %i", command );
	}

	return -1;
}

cg_t            cg;
cgs_t           cgs;
centity_t       cg_entities[ MAX_GENTITIES ];

weaponInfo_t    cg_weapons[ 32 ];
upgradeInfo_t   cg_upgrades[ 32 ];
classInfo_t     cg_classes[ PCL_NUM_CLASSES ];
buildableInfo_t cg_buildables[ BA_NUM_BUILDABLES ];

vmCvar_t        cg_teslaTrailTime;
vmCvar_t        cg_centertime;
vmCvar_t        cg_runpitch;
vmCvar_t        cg_runroll;
vmCvar_t        cg_swingSpeed;
vmCvar_t        cg_shadows;
vmCvar_t        cg_playerShadows;
vmCvar_t        cg_buildableShadows;
vmCvar_t        cg_drawTimer;
vmCvar_t        cg_drawClock;
vmCvar_t        cg_drawFPS;
vmCvar_t        cg_drawDemoState;
vmCvar_t        cg_drawSnapshot;
vmCvar_t        cg_drawChargeBar;
vmCvar_t        cg_drawCrosshair;
vmCvar_t        cg_drawCrosshairHit;
vmCvar_t        cg_drawCrosshairFriendFoe;
vmCvar_t        cg_drawCrosshairNames;
vmCvar_t        cg_drawBuildableHealth;
vmCvar_t        cg_drawMinimap;
vmCvar_t        cg_minimapActive;
vmCvar_t        cg_crosshairSize;
vmCvar_t        cg_crosshairFile;
vmCvar_t        cg_draw2D;
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
vmCvar_t        cg_viewsize;
vmCvar_t        cg_drawGun;
vmCvar_t        cg_gun_frame;
vmCvar_t        cg_gun_x;
vmCvar_t        cg_gun_y;
vmCvar_t        cg_gun_z;
vmCvar_t        cg_mirrorgun;
vmCvar_t        cg_tracerChance;
vmCvar_t        cg_tracerWidth;
vmCvar_t        cg_tracerLength;
vmCvar_t        cg_thirdPerson;
vmCvar_t        cg_thirdPersonAngle;
vmCvar_t        cg_thirdPersonShoulderViewMode;
vmCvar_t        cg_staticDeathCam;
vmCvar_t        cg_thirdPersonPitchFollow;
vmCvar_t        cg_thirdPersonRange;
vmCvar_t        cg_stereoSeparation;
vmCvar_t        cg_lagometer;
vmCvar_t        cg_drawSpeed;
vmCvar_t        cg_maxSpeedTimeWindow;
vmCvar_t        cg_synchronousClients;
vmCvar_t        cg_stats;
vmCvar_t        cg_paused;
vmCvar_t        cg_blood;
vmCvar_t        cg_teamChatsOnly;
vmCvar_t        cg_drawTeamOverlay;
vmCvar_t        cg_teamOverlaySortMode;
vmCvar_t        cg_teamOverlayMaxPlayers;
vmCvar_t        cg_teamOverlayUserinfo;
vmCvar_t        cg_noPrintDuplicate;
vmCvar_t        cg_noVoiceChats;
vmCvar_t        cg_noVoiceText;
vmCvar_t        cg_hudFiles;
vmCvar_t        cg_hudFilesEnable;
vmCvar_t        cg_smoothClients;
vmCvar_t        pmove_fixed;
vmCvar_t        pmove_msec;
vmCvar_t        pmove_accurate;
vmCvar_t        cg_timescaleFadeEnd;
vmCvar_t        cg_timescaleFadeSpeed;
vmCvar_t        cg_timescale;
vmCvar_t        cg_noTaunt;
vmCvar_t        cg_drawSurfNormal;
vmCvar_t        cg_drawBBOX;
vmCvar_t        cg_drawEntityInfo;
vmCvar_t        cg_wwSmoothTime;
vmCvar_t        cg_disableBlueprintErrors;
vmCvar_t        cg_depthSortParticles;
vmCvar_t        cg_bounceParticles;
vmCvar_t        cg_consoleLatency;
vmCvar_t        cg_lightFlare;
vmCvar_t        cg_debugParticles;
vmCvar_t        cg_debugTrails;
vmCvar_t        cg_debugPVS;
vmCvar_t        cg_disableWarningDialogs;
vmCvar_t        cg_disableUpgradeDialogs;
vmCvar_t        cg_disableBuildDialogs;
vmCvar_t        cg_disableCommandDialogs;
vmCvar_t        cg_disableScannerPlane;
vmCvar_t        cg_tutorial;

vmCvar_t        cg_rangeMarkerDrawSurface;
vmCvar_t        cg_rangeMarkerDrawIntersection;
vmCvar_t        cg_rangeMarkerDrawFrontline;
vmCvar_t        cg_rangeMarkerSurfaceOpacity;
vmCvar_t        cg_rangeMarkerLineOpacity;
vmCvar_t        cg_rangeMarkerLineThickness;
vmCvar_t        cg_rangeMarkerForBlueprint;
vmCvar_t        cg_rangeMarkerBuildableTypes;
vmCvar_t        cg_rangeMarkerWhenSpectating;
vmCvar_t        cg_buildableRangeMarkerMask;
vmCvar_t        cg_binaryShaderScreenScale;

vmCvar_t        cg_painBlendUpRate;
vmCvar_t        cg_painBlendDownRate;
vmCvar_t        cg_painBlendMax;
vmCvar_t        cg_painBlendScale;
vmCvar_t        cg_painBlendZoom;

vmCvar_t        cg_stickySpec;
vmCvar_t        cg_sprintToggle;
vmCvar_t        cg_unlagged;

vmCvar_t        cg_cmdGrenadeThrown;
vmCvar_t        cg_cmdNeedHealth;

vmCvar_t        cg_debugVoices;

vmCvar_t        ui_currentClass;
vmCvar_t        ui_carriage;
vmCvar_t        ui_dialog;
vmCvar_t        ui_voteActive;
vmCvar_t        ui_alienTeamVoteActive;
vmCvar_t        ui_humanTeamVoteActive;
vmCvar_t        ui_unlockables;
vmCvar_t        ui_momentumHalfLife;
vmCvar_t        ui_unlockablesMinTime;

vmCvar_t        cg_debugRandom;

vmCvar_t        cg_optimizePrediction;
vmCvar_t        cg_projectileNudge;

vmCvar_t        cg_voice;

vmCvar_t        cg_emoticons;
vmCvar_t        cg_emoticonsInMessages;

vmCvar_t        cg_chatTeamPrefix;

vmCvar_t        cg_animSpeed;
vmCvar_t        cg_animBlend;

vmCvar_t        cg_highPolyPlayerModels;
vmCvar_t        cg_highPolyBuildableModels;
vmCvar_t        cg_highPolyWeaponModels;
vmCvar_t        cg_motionblur;
vmCvar_t        cg_motionblurMinSpeed;

vmCvar_t        cg_fov_builder;
vmCvar_t        cg_fov_level0;
vmCvar_t        cg_fov_level1;
vmCvar_t        cg_fov_level2;
vmCvar_t        cg_fov_level3;
vmCvar_t        cg_fov_level4;
vmCvar_t        cg_fov_human;

typedef struct
{
	vmCvar_t   *vmCvar;
	const char *cvarName;
	const char *defaultString;
	int        cvarFlags;
} cvarTable_t;

static const cvarTable_t cvarTable[] =
{
	{ &cg_drawGun,                     "cg_drawGun",                     "1",            CVAR_ARCHIVE                 },
	{ &cg_viewsize,                    "cg_viewsize",                    "100",          0                            },
	{ &cg_stereoSeparation,            "cg_stereoSeparation",            "0.4",          0                            },
	{ &cg_shadows,                     "cg_shadows",                     "1",            CVAR_LATCH | CVAR_ARCHIVE    },
	{ &cg_playerShadows,               "cg_playerShadows",               "1",            0                            },
	{ &cg_buildableShadows,            "cg_buildableShadows",            "0",            0                            },
	{ &cg_draw2D,                      "cg_draw2D",                      "1",            0                            },
	{ &cg_drawTimer,                   "cg_drawTimer",                   "1",            CVAR_ARCHIVE                 },
	{ &cg_drawClock,                   "cg_drawClock",                   "0",            CVAR_ARCHIVE                 },
	{ &cg_drawFPS,                     "cg_drawFPS",                     "1",            CVAR_ARCHIVE                 },
	{ &cg_drawDemoState,               "cg_drawDemoState",               "1",            CVAR_ARCHIVE                 },
	{ &cg_drawSnapshot,                "cg_drawSnapshot",                "0",            0                            },
	{ &cg_drawChargeBar,               "cg_drawChargeBar",               "1",            CVAR_ARCHIVE                 },
	{ &cg_drawCrosshair,               "cg_drawCrosshair",               "2",            CVAR_ARCHIVE                 },
	{ &cg_drawCrosshairHit,            "cg_drawCrosshairHit",            "1",            0                            },
	{ &cg_drawCrosshairFriendFoe,      "cg_drawCrosshairFriendFoe",      "0",            0                            },
	{ &cg_drawCrosshairNames,          "cg_drawCrosshairNames",          "1",            0                            },
	{ &cg_drawBuildableHealth,         "cg_drawBuildableHealth",         "1",            0                            },
	{ &cg_drawMinimap,                 "cg_drawMinimap",                 "1",            0                            },
	{ &cg_minimapActive,               "cg_minimapActive",               "0",            0                            },
	{ &cg_crosshairSize,               "cg_crosshairSize",               "1",            CVAR_ARCHIVE                 },
	{ &cg_crosshairFile,               "cg_crosshairFile",               "",             0                            },
	{ &cg_addMarks,                    "cg_marks",                       "1",            CVAR_ARCHIVE                 },
	{ &cg_lagometer,                   "cg_lagometer",                   "0",            CVAR_ARCHIVE                 },
	{ &cg_drawSpeed,                   "cg_drawSpeed",                   "0",            CVAR_ARCHIVE                 },
	{ &cg_maxSpeedTimeWindow,          "cg_maxSpeedTimeWindow",          "2000",         0                            },
	{ &cg_teslaTrailTime,              "cg_teslaTrailTime",              "250",          0                            },
	{ &cg_gun_x,                       "cg_gunX",                        "0",            CVAR_CHEAT                   },
	{ &cg_gun_y,                       "cg_gunY",                        "0",            CVAR_CHEAT                   },
	{ &cg_gun_z,                       "cg_gunZ",                        "0",            CVAR_CHEAT                   },
	{ &cg_mirrorgun,                   "cg_mirrorgun",                   "0",            CVAR_ARCHIVE                 },
	{ &cg_centertime,                  "cg_centertime",                  "3",            CVAR_CHEAT                   },
	{ &cg_runpitch,                    "cg_runpitch",                    "0.002",        0                            },
	{ &cg_runroll,                     "cg_runroll",                     "0.005",        0                            },
	{ &cg_swingSpeed,                  "cg_swingSpeed",                  "0.3",          CVAR_CHEAT                   },
	{ &cg_debugAnim,                   "cg_debuganim",                   "0",            CVAR_CHEAT                   },
	{ &cg_debugPosition,               "cg_debugposition",               "0",            CVAR_CHEAT                   },
	{ &cg_debugEvents,                 "cg_debugevents",                 "0",            CVAR_CHEAT                   },
	{ &cg_errorDecay,                  "cg_errordecay",                  "100",          0                            },
	{ &cg_nopredict,                   "cg_nopredict",                   "0",            0                            },
	{ &cg_debugMove,                   "cg_debugMove",                   "0",            0                            },
	{ &cg_noPlayerAnims,               "cg_noplayeranims",               "0",            CVAR_CHEAT                   },
	{ &cg_showmiss,                    "cg_showmiss",                    "0",            0                            },
	{ &cg_footsteps,                   "cg_footsteps",                   "1",            CVAR_CHEAT                   },
	{ &cg_tracerChance,                "cg_tracerchance",                "0.4",          CVAR_CHEAT                   },
	{ &cg_tracerWidth,                 "cg_tracerwidth",                 "1",            CVAR_CHEAT                   },
	{ &cg_tracerLength,                "cg_tracerlength",                "100",          CVAR_CHEAT                   },
	{ &cg_thirdPersonRange,            "cg_thirdPersonRange",            "75",           0                            },
	{ &cg_thirdPerson,                 "cg_thirdPerson",                 "0",            CVAR_CHEAT                   },
	{ &cg_thirdPersonAngle,            "cg_thirdPersonAngle",            "0",            CVAR_CHEAT                   },
	{ &cg_thirdPersonPitchFollow,      "cg_thirdPersonPitchFollow",      "0",            0                            },
	{ &cg_thirdPersonShoulderViewMode, "cg_thirdPersonShoulderViewMode", "1",            0                            },
	{ &cg_staticDeathCam,              "cg_staticDeathCam",              "0",            CVAR_ARCHIVE                 },
	{ &cg_stats,                       "cg_stats",                       "0",            0                            },
	{ &cg_drawTeamOverlay,             "cg_drawTeamOverlay",             "1",            CVAR_ARCHIVE                 },
	{ &cg_teamOverlaySortMode,         "cg_teamOverlaySortMode",         "1",            CVAR_ARCHIVE                 },
	{ &cg_teamOverlayMaxPlayers,       "cg_teamOverlayMaxPlayers",       "8",            0                            },
	{ &cg_teamOverlayUserinfo,         "teamoverlay",                    "1",            CVAR_USERINFO                },
	{ &cg_teamChatsOnly,               "cg_teamChatsOnly",               "0",            CVAR_ARCHIVE                 },
	{ &cg_noPrintDuplicate,            "cg_noPrintDuplicate",            "0",            0                            },
	{ &cg_noVoiceChats,                "cg_noVoiceChats",                "0",            0                            },
	{ &cg_noVoiceText,                 "cg_noVoiceText",                 "0",            0                            },
	{ &cg_drawSurfNormal,              "cg_drawSurfNormal",              "0",            CVAR_CHEAT                   },
	{ &cg_drawBBOX,                    "cg_drawBBOX",                    "0",            CVAR_CHEAT                   },
	{ &cg_drawEntityInfo,              "cg_drawEntityInfo",              "0",            CVAR_CHEAT                   },
	{ &cg_wwSmoothTime,                "cg_wwSmoothTime",                "150",          CVAR_ARCHIVE                 },
	{ NULL,                            "cg_wwFollow",                    "1",            CVAR_USERINFO | CVAR_ARCHIVE },
	{ NULL,                            "cg_wwToggle",                    "1",            CVAR_USERINFO | CVAR_ARCHIVE },
	{ NULL,                            "cg_disableBlueprintErrors",      "0",            CVAR_USERINFO                },
	{ &cg_stickySpec,                  "cg_stickySpec",                  "1",            CVAR_USERINFO | CVAR_ARCHIVE },
	{ &cg_sprintToggle,                "cg_sprintToggle",                "0",            CVAR_USERINFO | CVAR_ARCHIVE },
	{ &cg_unlagged,                    "cg_unlagged",                    "1",            CVAR_USERINFO                },
	{ NULL,                            "cg_flySpeed",                    "600",          CVAR_USERINFO                },
	{ &cg_depthSortParticles,          "cg_depthSortParticles",          "1",            CVAR_ARCHIVE                 },
	{ &cg_bounceParticles,             "cg_bounceParticles",             "0",            CVAR_ARCHIVE                 },
	{ &cg_consoleLatency,              "cg_consoleLatency",              "3000",         0                            },
	{ &cg_lightFlare,                  "cg_lightFlare",                  "3",            CVAR_ARCHIVE                 },
	{ &cg_debugParticles,              "cg_debugParticles",              "0",            CVAR_CHEAT                   },
	{ &cg_debugTrails,                 "cg_debugTrails",                 "0",            CVAR_CHEAT                   },
	{ &cg_debugPVS,                    "cg_debugPVS",                    "0",            CVAR_CHEAT                   },
	{ &cg_disableWarningDialogs,       "cg_disableWarningDialogs",       "0",            CVAR_ARCHIVE                 },
	{ &cg_disableUpgradeDialogs,       "cg_disableUpgradeDialogs",       "0",            0                            },
	{ &cg_disableBuildDialogs,         "cg_disableBuildDialogs",         "0",            0                            },
	{ &cg_disableCommandDialogs,       "cg_disableCommandDialogs",       "0",            0                            },
	{ &cg_disableScannerPlane,         "cg_disableScannerPlane",         "0",            0                            },
	{ &cg_tutorial,                    "cg_tutorial",                    "1",            CVAR_ARCHIVE                 },

	{ &cg_rangeMarkerDrawSurface,      "cg_rangeMarkerDrawSurface",      "1",            CVAR_ARCHIVE                 },
	{ &cg_rangeMarkerDrawIntersection, "cg_rangeMarkerDrawIntersection", "0",            CVAR_ARCHIVE                 },
	{ &cg_rangeMarkerDrawFrontline,    "cg_rangeMarkerDrawFrontline",    "0",            CVAR_ARCHIVE                 },
	{ &cg_rangeMarkerSurfaceOpacity,   "cg_rangeMarkerSurfaceOpacity",   "0.08",         CVAR_ARCHIVE                 },
	{ &cg_rangeMarkerLineOpacity,      "cg_rangeMarkerLineOpacity",      "0.4",          CVAR_ARCHIVE                 },
	{ &cg_rangeMarkerLineThickness,    "cg_rangeMarkerLineThickness",    "4.0",          CVAR_ARCHIVE                 },
	{ &cg_rangeMarkerForBlueprint,     "cg_rangeMarkerForBlueprint",     "1",            CVAR_ARCHIVE                 },
	{ &cg_rangeMarkerBuildableTypes,   "cg_rangeMarkerBuildableTypes",   "support",      CVAR_ARCHIVE                 },
	{ &cg_rangeMarkerWhenSpectating,   "cg_rangeMarkerWhenSpectating",   "0",            0                            },
	{ &cg_buildableRangeMarkerMask,    "cg_buildableRangeMarkerMask",    "",             0                            },
	{ &cg_binaryShaderScreenScale,     "cg_binaryShaderScreenScale",     "1.0",          CVAR_ARCHIVE                 },

	{ &cg_hudFiles,                    "cg_hudFiles",                    "ui/hud.txt",   CVAR_ARCHIVE                 },
	{ &cg_hudFilesEnable,              "cg_hudFilesEnable",              "0",            CVAR_ARCHIVE                 },
	{ NULL,                            "cg_alienConfig",                 "",             0                            },
	{ NULL,                            "cg_humanConfig",                 "",             0                            },
	{ NULL,                            "cg_spectatorConfig",             "",             0                            },

	{ &cg_painBlendUpRate,             "cg_painBlendUpRate",             "10.0",         0                            },
	{ &cg_painBlendDownRate,           "cg_painBlendDownRate",           "0.5",          0                            },
	{ &cg_painBlendMax,                "cg_painBlendMax",                "0.7",          0                            },
	{ &cg_painBlendScale,              "cg_painBlendScale",              "7.0",          0                            },
	{ &cg_painBlendZoom,               "cg_painBlendZoom",               "0.65",         0                            },

	{ &cg_cmdGrenadeThrown,            "cg_cmdGrenadeThrown",            "vsay_local grenade", 0                      },
	{ &cg_cmdNeedHealth,               "cg_cmdNeedHealth",               "vsay_local needhealth", 0                   },

	{ &cg_debugVoices,                 "cg_debugVoices",                 "0",            0                            },

	// communication cvars set by the cgame to be read by ui
	{ &ui_currentClass,                "ui_currentClass",                "0",            CVAR_ROM                     },
	{ &ui_carriage,                    "ui_carriage",                    "",             CVAR_ROM                     },
	{ &ui_dialog,                      "ui_dialog",                      "Text not set", CVAR_ROM                     },
	{ &ui_voteActive,                  "ui_voteActive",                  "0",            CVAR_ROM                     },
	{ &ui_humanTeamVoteActive,         "ui_humanTeamVoteActive",         "0",            CVAR_ROM                     },
	{ &ui_alienTeamVoteActive,         "ui_alienTeamVoteActive",         "0",            CVAR_ROM                     },
	{ &ui_unlockables,                 "ui_unlockables",                 "0 0",          CVAR_ROM                     },
	{ &ui_momentumHalfLife,          "ui_momentumHalfLife",          "0",            CVAR_ROM                         },
	{ &ui_unlockablesMinTime,          "ui_unlockablesMinTime",          "0",            CVAR_ROM                     },

	{ &cg_debugRandom,                 "cg_debugRandom",                 "0",            0                            },

	{ &cg_optimizePrediction,          "cg_optimizePrediction",          "1",            0                            },
	{ &cg_projectileNudge,             "cg_projectileNudge",             "1",            0                            },

	// the following variables are created in other parts of the system,
	// but we also reference them here

	{ &cg_paused,                      "cl_paused",                      "0",            CVAR_ROM                     },
	{ &cg_blood,                       "com_blood",                      "1",            0                            },
	{ &cg_synchronousClients,          "g_synchronousClients",           "0",            CVAR_SYSTEMINFO              }, // communicated by systeminfo
	{ &cg_timescaleFadeEnd,            "cg_timescaleFadeEnd",            "1",            CVAR_CHEAT                   },
	{ &cg_timescaleFadeSpeed,          "cg_timescaleFadeSpeed",          "0",            CVAR_CHEAT                   },
	{ &cg_timescale,                   "timescale",                      "1",            0                            },
	{ &cg_smoothClients,               "cg_smoothClients",               "0",            CVAR_USERINFO                },

	{ &pmove_fixed,                    "pmove_fixed",                    "0",            CVAR_SYSTEMINFO              },
	{ &pmove_msec,                     "pmove_msec",                     "8",            CVAR_SYSTEMINFO              },
	{ &pmove_accurate,                 "pmove_accurate",                 "0",            CVAR_SYSTEMINFO              },
	{ &cg_noTaunt,                     "cg_noTaunt",                     "0",            CVAR_ARCHIVE                 },

	{ &cg_voice,                       "voice",                          "default",      CVAR_USERINFO                },

	{ &cg_emoticons,                   "cg_emoticons",                   "1",            CVAR_LATCH                   },
	{ &cg_emoticonsInMessages,         "cg_emoticonsInMessages",         "0",            0                            },

	{ &cg_animSpeed,                   "cg_animspeed",                   "1",            CVAR_CHEAT                   },
	{ &cg_animBlend,                   "cg_animblend",                   "5.0",          0                            },

	{ &cg_chatTeamPrefix,              "cg_chatTeamPrefix",              "1",            0                            },
	{ &cg_highPolyPlayerModels,        "cg_highPolyPlayerModels",        "1",            CVAR_LATCH                   },
	{ &cg_highPolyBuildableModels,     "cg_highPolyBuildableModels",     "1",            CVAR_LATCH                   },
	{ &cg_highPolyWeaponModels,        "cg_highPolyWeaponModels",        "1",            CVAR_LATCH                   },
	{ &cg_motionblur,                  "cg_motionblur",                  "0.05",         CVAR_ARCHIVE                 },
	{ &cg_motionblurMinSpeed,          "cg_motionblurMinSpeed",          "600",          0                            },
	{ &cg_fov_builder,                 "cg_fov_builder",                 "0",            0                            },
	{ &cg_fov_level0,                  "cg_fov_level0",                  "0",            0                            },
	{ &cg_fov_level1,                  "cg_fov_level1",                  "0",            0                            },
	{ &cg_fov_level2,                  "cg_fov_level2",                  "0",            0                            },
	{ &cg_fov_level3,                  "cg_fov_level3",                  "0",            0                            },
	{ &cg_fov_level4,                  "cg_fov_level4",                  "0",            0                            },
	{ &cg_fov_human,                   "cg_fov_human",                   "0",            0                            },
};

static const size_t cvarTableSize = ARRAY_LEN( cvarTable );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void )
{
	size_t i;
	const cvarTable_t *cv;

	for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
	{
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
		                    cv->defaultString, cv->cvarFlags );
	}
}

/*
===============
CG_SetPVars

Set some player cvars usable in scripts
these should refer only to playerstates that belong to the client, not the followed player, ui cvars will do that already
===============
*/
static void CG_SetPVars( void )
{
	playerState_t *ps;
	char          buffer[ MAX_CVAR_VALUE_STRING ];
	int           i;
	qboolean      first;

	if ( !cg.snap )
	{
		return;
	}

	ps = &cg.snap->ps;
	/* if we follow someone, the stats won't be about us, but the followed player instead */
	if ( ( ps->pm_flags & PMF_FOLLOW ) )
		return;

	trap_Cvar_Set( "p_teamname", BG_TeamName( ps->persistant[ PERS_TEAM ] ) );

	switch ( ps->persistant[ PERS_TEAM ] )
	{
		case TEAM_ALIENS:
		case TEAM_HUMANS:
			break;

		default:
		case TEAM_NONE:
			trap_Cvar_Set( "p_classname", "Spectator" );
			trap_Cvar_Set( "p_weaponname", "Nothing" );

			/*
			 * if we were on a team before we would want these to be reset (or they could mess with bindings)
			 * the only ones, that actually are helpful to hold on to are p_score and p_credits, since these actually
			 * might be taken over when joining a team again, or be used to look up old values before leaving the game
			 */
			trap_Cvar_Set( "p_class" , "0" );
			trap_Cvar_Set( "p_weapon", "0" );
			trap_Cvar_Set( "p_hp", "0" );
			trap_Cvar_Set( "p_maxhp", "0" );
			trap_Cvar_Set( "p_ammo", "0" );
			trap_Cvar_Set( "p_clips", "0" );
			return;
	}

	trap_Cvar_Set( "p_class", va( "%d", ps->stats[ STAT_CLASS ] ) );

	switch ( ps->stats[ STAT_CLASS ] )
	{
		case PCL_ALIEN_BUILDER0:
			trap_Cvar_Set( "p_classname", "Builder" );
			break;

		case PCL_ALIEN_BUILDER0_UPG:
			trap_Cvar_Set( "p_classname", "Advanced Builder" );
			break;

		case PCL_ALIEN_LEVEL0:
			trap_Cvar_Set( "p_classname", "Dretch" );
			break;

		case PCL_ALIEN_LEVEL1:
			trap_Cvar_Set( "p_classname", "Mantis" );
			break;

		case PCL_ALIEN_LEVEL2:
			trap_Cvar_Set( "p_classname", "Marauder" );
			break;

		case PCL_ALIEN_LEVEL2_UPG:
			trap_Cvar_Set( "p_classname", "Advanced Marauder" );
			break;

		case PCL_ALIEN_LEVEL3:
			trap_Cvar_Set( "p_classname", "Dragoon" );
			break;

		case PCL_ALIEN_LEVEL3_UPG:
			trap_Cvar_Set( "p_classname", "Advanced Dragoon" );
			break;

		case PCL_ALIEN_LEVEL4:
			trap_Cvar_Set( "p_classname", "Tyrant" );
			break;

		case PCL_HUMAN_NAKED:
			trap_Cvar_Set( "p_classname", "Naked Human" );
			break;

		case PCL_HUMAN_LIGHT:
			trap_Cvar_Set( "p_classname", "Light Human" );
			break;

		case PCL_HUMAN_MEDIUM:
			trap_Cvar_Set( "p_classname", "Medium Human" );
			break;

		case PCL_HUMAN_BSUIT:
			trap_Cvar_Set( "p_classname", "Battlesuit" );
			break;

		case PCL_NONE: //used between death and spawn
			trap_Cvar_Set( "p_classname", "Ghost" );
			break;

		default:
			trap_Cvar_Set( "p_classname", "Unknown" );
			break;
	}

	trap_Cvar_Set( "p_weapon", va( "%d", ps->stats[ STAT_WEAPON ] ) );

	switch ( ps->stats[ STAT_WEAPON ] )
	{
		case WP_HBUILD:
			trap_Cvar_Set( "p_weaponname", "Construction Kit" );
			break;

		case WP_BLASTER:
			trap_Cvar_Set( "p_weaponname", "Blaster" );
			break;

		case WP_MACHINEGUN:
			trap_Cvar_Set( "p_weaponname", "Machine Gun" );
			break;

		case WP_PAIN_SAW:
			trap_Cvar_Set( "p_weaponname", "Painsaw" );
			break;

		case WP_SHOTGUN:
			trap_Cvar_Set( "p_weaponname", "Shotgun" );
			break;

		case WP_LAS_GUN:
			trap_Cvar_Set( "p_weaponname", "Laser Gun" );
			break;

		case WP_MASS_DRIVER:
			trap_Cvar_Set( "p_weaponname", "Mass Driver" );
			break;

		case WP_CHAINGUN:
			trap_Cvar_Set( "p_weaponname", "Chain Gun" );
			break;

		case WP_PULSE_RIFLE:
			trap_Cvar_Set( "p_weaponname", "Pulse Rifle" );
			break;

		case WP_FLAMER:
			trap_Cvar_Set( "p_weaponname", "Flame Thrower" );
			break;

		case WP_LUCIFER_CANNON:
			trap_Cvar_Set( "p_weaponname", "Lucifier cannon" );
			break;

		case WP_ALEVEL0:
			trap_Cvar_Set( "p_weaponname", "Teeth" );
			break;

		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_ALEVEL1:
		case WP_ALEVEL2:
		case WP_ALEVEL2_UPG:
		case WP_ALEVEL3:
		case WP_ALEVEL3_UPG:
		case WP_ALEVEL4:
			trap_Cvar_Set( "p_weaponname", "Claws" );
			break;

		case WP_NONE:
			trap_Cvar_Set( "p_weaponname", "Nothing" );
			break;


		default:
			trap_Cvar_Set( "p_weaponname", "Unknown" );
			break;
	}

	trap_Cvar_Set( "p_credits", va( "%d", ps->persistant[ PERS_CREDIT ] ) );
	trap_Cvar_Set( "p_score", va( "%d", ps->persistant[ PERS_SCORE ] ) );

	trap_Cvar_Set( "p_hp", va( "%d", ps->stats[ STAT_HEALTH ] ) );
	trap_Cvar_Set( "p_maxhp", va( "%d", ps->stats[ STAT_MAX_HEALTH ] ) );
	trap_Cvar_Set( "p_ammo", va( "%d", ps->ammo ) );
	trap_Cvar_Set( "p_clips", va( "%d", ps->clips ) );

	// set p_availableBuildings to a space-separated list of buildings
	first = qtrue;
	*buffer = 0;

	for ( i = BA_NONE; i < BA_NUM_BUILDABLES; ++i )
	{
		const buildableAttributes_t *buildable = BG_Buildable( i );

		if ( buildable->team == ps->persistant[ PERS_TEAM ] &&
		     BG_BuildableUnlocked( i ) &&
		     (buildable->buildWeapon & ( 1 << ps->stats[ STAT_WEAPON ] ) ) )

		{
			Q_strcat( buffer, sizeof( buffer ), first ? buildable->name : va( " %s", buildable->name ) );
			first = qfalse;
		}
	}

	trap_Cvar_Set( "p_availableBuildings", buffer );
}

/*
===============
CG_SetUIVars

Set some cvars used by the UI
these will change when following another player
===============
*/
static void CG_SetUIVars( void )
{
	if ( !cg.snap )
	{
		return;
	}

	trap_Cvar_Set( "ui_carriage", va( "%d %d %d", cg.snap->ps.stats[ STAT_WEAPON ],
	               cg.snap->ps.stats[ STAT_ITEMS ], cg.snap->ps.persistant[ PERS_CREDIT ] ) );
}

/*
================
CG_UpdateBuildableRangeMarkerMask
================
*/
void CG_UpdateBuildableRangeMarkerMask( void )
{
	static int btmc = 0;
	static int spmc = 0;

	if ( cg_rangeMarkerBuildableTypes.modificationCount != btmc ||
	     cg_rangeMarkerWhenSpectating.modificationCount != spmc )
	{
		int         brmMask;
		char        buffer[ MAX_CVAR_VALUE_STRING ];
		char        *p, *q;
		buildable_t buildable;

		brmMask = cg_rangeMarkerWhenSpectating.integer ? ( 1 << BA_NONE ) : 0;

		if ( !cg_rangeMarkerBuildableTypes.string[ 0 ] )
		{
			goto empty;
		}

		Q_strncpyz( buffer, cg_rangeMarkerBuildableTypes.string, sizeof( buffer ) );
		p = &buffer[ 0 ];

		for ( ;; )
		{
			q = strchr( p, ',' );

			if ( q )
			{
				*q = '\0';
			}

			while ( *p == ' ' )
			{
				++p;
			}

			buildable = BG_BuildableByName( p )->number;

			if ( buildable != BA_NONE )
			{
				brmMask |= 1 << buildable;
			}
			else if ( !Q_stricmp( p, "all" ) )
			{
				brmMask |= ( 1 << BA_A_OVERMIND ) | ( 1 << BA_A_SPAWN ) | ( 1 << BA_A_ACIDTUBE ) |
				           ( 1 << BA_A_TRAPPER ) | ( 1 << BA_A_HIVE ) | ( 1 << BA_A_LEECH ) |
				           ( 1 << BA_A_BOOSTER ) | ( 1 << BA_H_REACTOR ) | ( 1 << BA_H_REPEATER ) |
				           ( 1 << BA_H_MGTURRET ) | ( 1 << BA_H_TESLAGEN ) | ( 1 << BA_H_DRILL );
			}
			else if ( !Q_stricmp( p, "none" ) )
			{
				brmMask = 0;
			}
			else
			{
				char *pp;
				int  only;

				if ( !Q_strnicmp( p, "alien", 5 ) )
				{
					pp = p + 5;
					only = ( 1 << BA_A_OVERMIND ) | ( 1 << BA_A_SPAWN ) |
					       ( 1 << BA_A_ACIDTUBE ) | ( 1 << BA_A_TRAPPER ) | ( 1 << BA_A_HIVE ) | ( 1 << BA_A_LEECH ) | ( 1 << BA_A_BOOSTER );
				}
				else if ( !Q_strnicmp( p, "human", 5 ) )
				{
					pp = p + 5;
					only = ( 1 << BA_H_REACTOR ) | ( 1 << BA_H_REPEATER ) |
					       ( 1 << BA_H_MGTURRET ) | ( 1 << BA_H_TESLAGEN ) | ( 1 << BA_H_DRILL );
				}
				else
				{
					pp = p;
					only = ~0;
				}

				if ( pp != p && !*pp )
				{
					brmMask |= only;
				}
				else if ( !Q_stricmp( pp, "support" ) )
				{
					brmMask |= only & ( ( 1 << BA_A_OVERMIND ) | ( 1 << BA_A_SPAWN ) | ( 1 << BA_A_LEECH ) | ( 1 << BA_A_BOOSTER ) |
					                    ( 1 << BA_H_REACTOR ) | ( 1 << BA_H_REPEATER ) | ( 1 << BA_H_DRILL ) );
				}
				else if ( !Q_stricmp( pp, "offensive" ) )
				{
					brmMask |= only & ( ( 1 << BA_A_ACIDTUBE ) | ( 1 << BA_A_TRAPPER ) | ( 1 << BA_A_HIVE ) |
					                    ( 1 << BA_H_MGTURRET ) | ( 1 << BA_H_TESLAGEN ) );
				}
				else
				{
					Com_Printf( S_WARNING "unknown buildable or group: %s\n", p );
				}
			}

			if ( q )
			{
				p = q + 1;
			}
			else
			{
				break;
			}
		}

empty:
		trap_Cvar_Set( "cg_buildableRangeMarkerMask", va( "%i", brmMask ) );

		btmc = cg_rangeMarkerBuildableTypes.modificationCount;
		spmc = cg_rangeMarkerWhenSpectating.modificationCount;
	}
}

void CG_NotifyHooks( void )
{
	playerState_t *ps;
	char config[ MAX_CVAR_VALUE_STRING ];
	static int lastTeam = INT_MIN; //to make sure we run the hook initially as well

	if ( !cg.snap )
	{
		return;
	}

	ps = &cg.snap->ps;
	if ( !( ps->pm_flags & PMF_FOLLOW ) )
	{
		if( lastTeam != ps->persistant[ PERS_TEAM ] )
		{
			trap_notify_onTeamChange( ps->persistant[ PERS_TEAM ] );

			/* execute team-specific config files */
			trap_Cvar_VariableStringBuffer( va( "cg_%sConfig", BG_TeamName( ps->persistant[ PERS_TEAM ] ) ), config, sizeof( config ) );
			if ( config[ 0 ] )
			{
				trap_SendConsoleCommand( va( "exec %s\n", Quote( config ) ) );
			}

			lastTeam = ps->persistant[ PERS_TEAM ];
		}
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void )
{
	size_t i;
	const cvarTable_t *cv;

	for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
	{
		if ( cv->vmCvar )
		{
			trap_Cvar_Update( cv->vmCvar );
		}
	}

	// check for modifications here
	CG_SetPVars();
	CG_SetUIVars();
	CG_UpdateBuildableRangeMarkerMask();
}

int CG_CrosshairPlayer( void )
{
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) )
	{
		return -1;
	}

	return cg.crosshairClientNum;
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

	offset = cg.consoleLines[ 0 ].length;
	totalLength = strlen( cg.consoleText ) - offset;

	//slide up consoleText
	for ( i = 0; i <= totalLength; i++ )
	{
		cg.consoleText[ i ] = cg.consoleText[ i + offset ];
	}

	//pop up the first consoleLine
	cg.numConsoleLines--;
	for ( i = 0; i < cg.numConsoleLines; i++ )
	{
		cg.consoleLines[ i ] = cg.consoleLines[ i + 1 ];
	}
}

/*
=================
CG_AddNotifyText
=================
*/
void CG_AddNotifyText( void )
{
	char buffer[ BIG_INFO_STRING ];
	int  bufferLen, textLen;

	if ( cg.loading )
	{
		return;
	}

	trap_LiteralArgs( buffer, BIG_INFO_STRING );

	if ( !buffer[ 0 ] )
	{
		cg.consoleText[ 0 ] = '\0';
		cg.numConsoleLines = 0;
		return;
	}

	bufferLen = strlen( buffer );
	textLen = strlen( cg.consoleText );

	// Ignore console messages that were just printed
	if ( cg_noPrintDuplicate.integer && textLen >= bufferLen &&
	     !strcmp( cg.consoleText + textLen - bufferLen, buffer ) )
	{
		return;
	}

	if ( cg.numConsoleLines == MAX_CONSOLE_LINES )
	{
		CG_RemoveNotifyLine();
		textLen = strlen( cg.consoleText );
	}

	Q_strncpyz( cg.consoleText + textLen, buffer, MAX_CONSOLE_TEXT - textLen );
	cg.consoleLines[ cg.numConsoleLines ].time = cg.time;
	cg.consoleLines[ cg.numConsoleLines ].length =
		MIN( bufferLen, MAX_CONSOLE_TEXT - textLen - 1 );
	cg.numConsoleLines++;
}

void QDECL PRINTF_LIKE(1) CG_Printf( const char *msg, ... )
{
	va_list argptr;
	char    text[ 1024 ];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Print( text );
}

void QDECL PRINTF_LIKE(1) NORETURN CG_Error( const char *msg, ... )
{
	va_list argptr;
	char    text[ 1024 ];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Error( text );
}

void QDECL PRINTF_LIKE(2) NORETURN Com_Error( int level, const char *error, ... )
{
	va_list argptr;
	char    text[ 1024 ];

	Q_UNUSED(level);

	va_start( argptr, error );
	Q_vsnprintf( text, sizeof( text ), error, argptr );
	va_end( argptr );

	trap_Error( text );
}

void QDECL PRINTF_LIKE(1) Com_Printf( const char *msg, ... )
{
	va_list argptr;
	char    text[ 1024 ];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

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

static const char *choose( const char *first, ... )
{
	va_list    ap;
	int        count = 1;
	const char *ret;

	va_start( ap, first );
	while ( va_arg( ap, const char * ) )
	{
		++count;
	}
	va_end( ap );

	if ( count < 2 )
	{
		return first;
	}

	count = rand() % count;

	ret = first;
	va_start( ap, first );
	while ( count-- )
	{
		ret = va_arg( ap, const char * );
	}
	va_end( ap );

	return ret;
}


/*
=================
CG_FileExists

Test if a specific file exists or not
=================
*/
qboolean CG_FileExists( const char *filename )
{
	return trap_FS_FOpenFile( filename, NULL, FS_READ );
}

/*
======================
CG_UpdateLoadingProgress

======================
*/

enum {
	LOADBAR_MEDIA,
	LOADBAR_CHARACTER_MODELS,
	LOADBAR_BUILDABLES
} typedef loadingBar_t;

static void CG_UpdateLoadingProgress( loadingBar_t progressBar, float progress, const char *label )
{
	if(!cg.loading)
		return;

	switch (progressBar) {
		case LOADBAR_MEDIA:
			cg.mediaFraction = progress;
			break;
		case LOADBAR_CHARACTER_MODELS:
			cg.charModelFraction = progress;
			break;
		case LOADBAR_BUILDABLES:
			cg.buildablesFraction = progress;
			break;
		default:
			break;
	}

	Q_strncpyz(cg.currentLoadingLabel, label, sizeof( cg.currentLoadingLabel ) );

	trap_UpdateScreen();
}

static void CG_UpdateMediaFraction( float newFract )
{
	cg.mediaFraction = newFract;
	trap_UpdateScreen();
}

enum {
	LOAD_START = 1,
	LOAD_TRAILS,
	LOAD_PARTICLES,
	LOAD_SOUNDS,
	LOAD_GEOMETRY,
	LOAD_ASSETS,
	LOAD_CONFIGS,
	LOAD_WEAPONS,
	LOAD_UPGRADES,
	LOAD_CLASSES,
	LOAD_BUILDINGS,
	LOAD_REMAINING,
	LOAD_DONE
} typedef cgLoadingStep_t;

static void CG_UpdateLoadingStep( cgLoadingStep_t step )
{
#if 0
	static int startTime = 0;
	static int lastStepTime = 0;
	const int thisStepTime = trap_Milliseconds();

	switch (step) {
		case LOAD_START:
			startTime = thisStepTime;
			CG_Printf("^4%%^5 Start loading.\n");
			break;
		case LOAD_DONE:
			CG_Printf("^4%%^5 Done loading everything after %is (%ims).\n",
					(thisStepTime - startTime)/1000, (thisStepTime - startTime));
			break;
		default:
			CG_Printf("^4%%^5 Done with Step %i after %is (%ims)… Starting Step %i\n",
					step - 1, (thisStepTime - lastStepTime)/1000, (thisStepTime - lastStepTime), step );
			break;
	}
	lastStepTime = thisStepTime;
#endif

	switch (step) {
		case LOAD_START:
			cg.loading = qtrue;
			cg.mediaFraction = cg.charModelFraction = cg.buildablesFraction = 0.0f;
			break;

		case LOAD_TRAILS:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.0f, choose("Tracking your movements", "Letting out the magic smoke", NULL) );
			break;
		case LOAD_PARTICLES:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.05f, choose("Collecting bees for the hives", "Initialising fireworks", "Causing electrical faults", NULL) );
			break;
		case LOAD_SOUNDS:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.08f, choose("Recording granger purring", "Generating annoying noises", NULL) );
			break;
		case LOAD_GEOMETRY:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.60f, choose("Hello World!", "Making a scene.", NULL) );
			break;
		case LOAD_ASSETS:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.63f, choose("Taking pictures of the world", "Using your laptop's camera", "Adding texture to concrete", "Drawing smiley faces", NULL) );
			break;
		case LOAD_CONFIGS:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.80f, choose("Reading the manual", "Looking at blueprints", NULL) );
			break;
		case LOAD_WEAPONS:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.90f, choose("Setting up the armoury", "Sharpening the aliens' claws", "Overloading lucifer cannons", NULL) );
			break;
		case LOAD_UPGRADES:
		case LOAD_CLASSES:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.95f, choose("Charging battery packs", "Replicating alien DNA", "Packing tents for jetcampers", NULL) );
			break;
		case LOAD_BUILDINGS:
			cg.mediaFraction = 1.0f;
			CG_UpdateLoadingProgress( LOADBAR_BUILDABLES, 0.0f, choose("Finishing construction", "Adding turret spam", "Awakening the overmind", NULL) );
			break;

		case LOAD_DONE:
			cg.mediaFraction = cg.charModelFraction = cg.buildablesFraction = 1.0f;
			Q_strncpyz(cg.currentLoadingLabel, "Done!", sizeof( cg.currentLoadingLabel ) );
			trap_UpdateScreen();
			cg.loading = qfalse;
			break;

		default:
			break;
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

	cgs.media.weHaveEvolved = trap_S_RegisterSound( "sound/announcements/overmindevolved.wav", qtrue );
	cgs.media.reinforcement = trap_S_RegisterSound( "sound/announcements/reinforcement.wav", qtrue );

	cgs.media.alienOvermindAttack = trap_S_RegisterSound( "sound/announcements/overmindattack.wav", qtrue );
	cgs.media.alienOvermindDying = trap_S_RegisterSound( "sound/announcements/overminddying.wav", qtrue );
	cgs.media.alienOvermindSpawns = trap_S_RegisterSound( "sound/announcements/overmindspawns.wav", qtrue );

	cgs.media.alienL4ChargePrepare = trap_S_RegisterSound( "sound/player/level4/charge_prepare.wav", qtrue );
	cgs.media.alienL4ChargeStart = trap_S_RegisterSound( "sound/player/level4/charge_start.wav", qtrue );

	cgs.media.tracerSound = trap_S_RegisterSound( "sound/weapons/tracer.wav", qfalse );
	cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change.wav", qfalse );
	cgs.media.turretSpinupSound = trap_S_RegisterSound( "sound/buildables/mgturret/spinup.wav", qfalse );
	cgs.media.weaponEmptyClick = trap_S_RegisterSound( "sound/weapons/click.wav", qfalse );

	cgs.media.talkSound = trap_S_RegisterSound( "sound/misc/talk.wav", qfalse );
	cgs.media.alienTalkSound = trap_S_RegisterSound( "sound/misc/alien_talk.wav", qfalse );
	cgs.media.humanTalkSound = trap_S_RegisterSound( "sound/misc/human_talk.wav", qfalse );
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav", qfalse );

	cgs.media.watrInSound = trap_S_RegisterSound( "sound/player/watr_in.wav", qfalse );
	cgs.media.watrOutSound = trap_S_RegisterSound( "sound/player/watr_out.wav", qfalse );
	cgs.media.watrUnSound = trap_S_RegisterSound( "sound/player/watr_un.wav", qfalse );

	cgs.media.disconnectSound = trap_S_RegisterSound( "sound/misc/disconnect.wav", qfalse );

	for ( i = 0; i < 4; i++ )
	{
		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/step%i.wav", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_GENERAL ][ i ] = trap_S_RegisterSound( name, qfalse );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/flesh%i.wav", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_FLESH ][ i ] = trap_S_RegisterSound( name, qfalse );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/splash%i.wav", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_SPLASH ][ i ] = trap_S_RegisterSound( name, qfalse );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/clank%i.wav", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_METAL ][ i ] = trap_S_RegisterSound( name, qfalse );
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

	cgs.media.jetpackThrustLoopSound = trap_S_RegisterSound( "sound/upgrades/jetpack/hi.wav", qfalse );

	cgs.media.medkitUseSound = trap_S_RegisterSound( "sound/upgrades/medkit/medkit.wav", qfalse );

	cgs.media.alienEvolveSound = trap_S_RegisterSound( "sound/player/alienevolve.wav", qfalse );

	cgs.media.alienBuildableExplosion = trap_S_RegisterSound( "sound/buildables/alien/explosion.wav", qfalse );
	cgs.media.alienBuildablePrebuild = trap_S_RegisterSound( "sound/buildables/alien/prebuild.wav", qfalse );

	cgs.media.humanBuildableDying = trap_S_RegisterSound( "sound/buildables/human/dying.wav", qfalse );
	cgs.media.humanBuildableExplosion = trap_S_RegisterSound( "sound/buildables/human/explosion.wav", qfalse );
	cgs.media.humanBuildablePrebuild = trap_S_RegisterSound( "sound/buildables/human/prebuild.wav", qfalse );

	for ( i = 0; i < 4; i++ )
	{
		cgs.media.humanBuildableDamage[ i ] = trap_S_RegisterSound(
		                                        va( "sound/buildables/human/damage%d.wav", i ), qfalse );
	}

	cgs.media.hardBounceSound1 = trap_S_RegisterSound( "sound/misc/hard_bounce1.wav", qfalse );
	cgs.media.hardBounceSound2 = trap_S_RegisterSound( "sound/misc/hard_bounce2.wav", qfalse );

	cgs.media.repeaterUseSound = trap_S_RegisterSound( "sound/buildables/repeater/use.wav", qfalse );

	cgs.media.buildableRepairSound = trap_S_RegisterSound( "sound/buildables/human/repair.wav", qfalse );
	cgs.media.buildableRepairedSound = trap_S_RegisterSound( "sound/buildables/human/repaired.wav", qfalse );

	cgs.media.lCannonWarningSound = trap_S_RegisterSound( "models/weapons/lcannon/warning.wav", qfalse );
	cgs.media.lCannonWarningSound2 = trap_S_RegisterSound( "models/weapons/lcannon/warning2.wav", qfalse );
}

//===================================================================================

/*
=================
CG_RegisterGrading
=================
*/
void CG_RegisterGrading( int slot, const char *str )
{
	int   model;
	float dist;
	char  texture[MAX_QPATH];

	if( !str || !*str ) {
		cgs.gameGradingTextures[ slot ]  = 0;
		cgs.gameGradingModels[ slot ]    = 0;
		cgs.gameGradingDistances[ slot ] = 0.0f;
		return;
	}

	sscanf(str, "%d %f %s", &model, &dist, texture);
	cgs.gameGradingTextures[ slot ] =
		trap_R_RegisterShader(texture, (RegisterShaderFlags_t) ( RSF_NOMIP | RSF_NOLIGHTSCALE ) );
	cgs.gameGradingModels[ slot ] = model;
	cgs.gameGradingDistances[ slot ] = dist;
}

/*
=================
CG_RegisterReverb
=================
*/
void CG_RegisterReverb( int slot, const char *str )
{
	int   model;
	float dist, intensity;
	char  name[MAX_NAME_LENGTH];

	if( !str || !*str ) {
		Q_strncpyz(cgs.gameReverbEffects[ slot ], "none", MAX_NAME_LENGTH);
		cgs.gameReverbModels[ slot ]        = 0;
		cgs.gameReverbDistances[ slot ]     = 0.0f;
		cgs.gameReverbIntensities[ slot ]   = 0.0f;
		return;
	}

	sscanf(str, "%d %f %s %f", &model, &dist, name, &intensity);
	Q_strncpyz(cgs.gameReverbEffects[ slot ], name, MAX_NAME_LENGTH);
	cgs.gameReverbModels[ slot ] = model;
	cgs.gameReverbDistances[ slot ] = dist;
	cgs.gameReverbIntensities[ slot ] = intensity;
}

/*
=================
CG_RegisterGraphics
=================
*/
static void CG_RegisterGraphics( void )
{
	int         i;
	static const char *const sb_nums[ 11 ] =
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
	static const char *const buildWeaponTimerPieShaders[ 8 ] =
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

	CG_UpdateLoadingStep( LOAD_GEOMETRY );
	trap_R_LoadWorldMap( va( "maps/%s.bsp", cgs.mapname ) );

	CG_UpdateLoadingStep( LOAD_ASSETS );
	for ( i = 0; i < 11; i++ )
	{
		cgs.media.numberShaders[ i ] = trap_R_RegisterShader(sb_nums[i],
								     (RegisterShaderFlags_t) RSF_DEFAULT);
	}

	cgs.media.viewBloodShader = trap_R_RegisterShader("gfx/damage/fullscreen_painblend",
							  (RegisterShaderFlags_t) RSF_DEFAULT);

	cgs.media.connectionShader = trap_R_RegisterShader("gfx/2d/net",
							   (RegisterShaderFlags_t) RSF_DEFAULT);

	cgs.media.creepShader = trap_R_RegisterShader("creep", (RegisterShaderFlags_t) RSF_DEFAULT);

	cgs.media.scannerBlipShader = trap_R_RegisterShader("gfx/2d/blip",
							    (RegisterShaderFlags_t) RSF_DEFAULT);

	cgs.media.scannerBlipBldgShader = trap_R_RegisterShader("gfx/2d/blip_bldg",
								(RegisterShaderFlags_t) RSF_DEFAULT);

	cgs.media.scannerLineShader = trap_R_RegisterShader("gfx/2d/stalk",
							    (RegisterShaderFlags_t) RSF_DEFAULT);

	cgs.media.teamOverlayShader = trap_R_RegisterShader("gfx/2d/teamoverlay",
							    (RegisterShaderFlags_t) RSF_DEFAULT);

	cgs.media.tracerShader = trap_R_RegisterShader("gfx/misc/tracer",
						       (RegisterShaderFlags_t) RSF_DEFAULT);

	cgs.media.backTileShader = trap_R_RegisterShader("console",
							 (RegisterShaderFlags_t) RSF_DEFAULT);

	// building shaders
	cgs.media.greenBuildShader = trap_R_RegisterShader("gfx/misc/greenbuild",
							   (RegisterShaderFlags_t) RSF_DEFAULT);
	cgs.media.redBuildShader = trap_R_RegisterShader("gfx/misc/redbuild",
							 (RegisterShaderFlags_t) RSF_DEFAULT);
	cgs.media.humanSpawningShader = trap_R_RegisterShader("models/buildables/telenode/rep_cyl",
							      (RegisterShaderFlags_t) RSF_DEFAULT);

	for ( i = 0; i < 8; i++ )
	{
		cgs.media.buildWeaponTimerPie[ i ] = trap_R_RegisterShader(buildWeaponTimerPieShaders[i],
									   (RegisterShaderFlags_t) RSF_DEFAULT);
	}

	// player health cross shaders
	cgs.media.healthCross = trap_R_RegisterShader("ui/assets/neutral/cross",
						      (RegisterShaderFlags_t) RSF_DEFAULT);
	cgs.media.healthCross2X = trap_R_RegisterShader("ui/assets/neutral/cross2",
							(RegisterShaderFlags_t) RSF_DEFAULT);
	cgs.media.healthCross3X = trap_R_RegisterShader("ui/assets/neutral/cross3",
							(RegisterShaderFlags_t) RSF_DEFAULT);
	cgs.media.healthCrossMedkit = trap_R_RegisterShader("ui/assets/neutral/cross_medkit",
							    (RegisterShaderFlags_t) RSF_DEFAULT);
	cgs.media.healthCrossPoisoned = trap_R_RegisterShader("ui/assets/neutral/cross_poison",
							      (RegisterShaderFlags_t) RSF_DEFAULT);

	cgs.media.desaturatedCgrade = trap_R_RegisterShader("gfx/cgrading/desaturated",
								 (RegisterShaderFlags_t) ( RSF_NOMIP | RSF_NOLIGHTSCALE ) );

	cgs.media.neutralCgrade = trap_R_RegisterShader("gfx/cgrading/neutral",
								 (RegisterShaderFlags_t) ( RSF_NOMIP | RSF_NOLIGHTSCALE ) );

	cgs.media.redCgrade = trap_R_RegisterShader("gfx/cgrading/red-only",
								 (RegisterShaderFlags_t) ( RSF_NOMIP | RSF_NOLIGHTSCALE ) );

	cgs.media.tealCgrade = trap_R_RegisterShader("gfx/cgrading/teal-only",
								 (RegisterShaderFlags_t) ( RSF_NOMIP | RSF_NOLIGHTSCALE ) );

	cgs.media.balloonShader = trap_R_RegisterShader("gfx/sprites/chatballoon",
							(RegisterShaderFlags_t) RSF_DEFAULT);

	cgs.media.disconnectPS = CG_RegisterParticleSystem( "disconnectPS" );

	cgs.media.scopeShader = trap_R_RegisterShader( "scope", (RegisterShaderFlags_t) ( RSF_DEFAULT | RSF_NOMIP ) );

	CG_UpdateMediaFraction( 0.7f );

	memset( cg_weapons, 0, sizeof( cg_weapons ) );
	memset( cg_upgrades, 0, sizeof( cg_upgrades ) );

	cgs.media.shadowMarkShader = trap_R_RegisterShader("gfx/marks/shadow",
							   (RegisterShaderFlags_t) RSF_DEFAULT);
	cgs.media.wakeMarkShader = trap_R_RegisterShader("gfx/marks/wake",
							 (RegisterShaderFlags_t) RSF_DEFAULT);

	cgs.media.alienEvolvePS = CG_RegisterParticleSystem( "alienEvolvePS" );
	cgs.media.alienAcidTubePS = CG_RegisterParticleSystem( "alienAcidTubePS" );
	cgs.media.alienBoosterPS = CG_RegisterParticleSystem( "alienBoosterPS" );

	cgs.media.jetPackThrustPS = CG_RegisterParticleSystem( "jetPackAscendPS" );

	cgs.media.humanBuildableDamagedPS = CG_RegisterParticleSystem( "humanBuildableDamagedPS" );
	cgs.media.alienBuildableDamagedPS = CG_RegisterParticleSystem( "alienBuildableDamagedPS" );
	cgs.media.humanBuildableDestroyedPS = CG_RegisterParticleSystem( "humanBuildableDestroyedPS" );
	cgs.media.humanBuildableNovaPS = CG_RegisterParticleSystem( "humanBuildableNovaPS" );
	cgs.media.alienBuildableDestroyedPS = CG_RegisterParticleSystem( "alienBuildableDestroyedPS" );

	cgs.media.humanBuildableBleedPS = CG_RegisterParticleSystem( "humanBuildableBleedPS" );
	cgs.media.alienBuildableBleedPS = CG_RegisterParticleSystem( "alienBuildableBleedPS" );
	cgs.media.alienBuildableBurnPS  = CG_RegisterParticleSystem( "alienBuildableBurnPS" );

	cgs.media.floorFirePS = CG_RegisterParticleSystem( "floorFirePS" );

	cgs.media.alienBleedPS = CG_RegisterParticleSystem( "alienBleedPS" );
	cgs.media.humanBleedPS = CG_RegisterParticleSystem( "humanBleedPS" );

	cgs.media.sphereModel = trap_R_RegisterModel( "models/generic/sphere.md3" );
	cgs.media.sphericalCone64Model = trap_R_RegisterModel( "models/generic/sphericalCone64.md3" );
	cgs.media.sphericalCone240Model = trap_R_RegisterModel( "models/generic/sphericalCone240.md3" );

	cgs.media.plainColorShader = trap_R_RegisterShader("gfx/plainColor",
							   (RegisterShaderFlags_t) RSF_DEFAULT);
	cgs.media.binaryAlpha1Shader = trap_R_RegisterShader("gfx/binary/alpha1",
							     (RegisterShaderFlags_t) RSF_DEFAULT);

	for ( i = 0; i < NUM_BINARY_SHADERS; ++i )
	{
		cgs.media.binaryShaders[ i ].f1 = trap_R_RegisterShader(va("gfx/binary/%03i_F1", i),
									(RegisterShaderFlags_t) RSF_DEFAULT);
		cgs.media.binaryShaders[ i ].f2 = trap_R_RegisterShader(va("gfx/binary/%03i_F2", i),
									(RegisterShaderFlags_t) RSF_DEFAULT);
		cgs.media.binaryShaders[ i ].f3 = trap_R_RegisterShader(va("gfx/binary/%03i_F3", i),
									(RegisterShaderFlags_t) RSF_DEFAULT);
		cgs.media.binaryShaders[ i ].b1 = trap_R_RegisterShader(va("gfx/binary/%03i_B1", i),
									(RegisterShaderFlags_t) RSF_DEFAULT);
		cgs.media.binaryShaders[ i ].b2 = trap_R_RegisterShader(va("gfx/binary/%03i_B2", i),
									(RegisterShaderFlags_t) RSF_DEFAULT);
		cgs.media.binaryShaders[ i ].b3 = trap_R_RegisterShader(va("gfx/binary/%03i_B3", i),
									(RegisterShaderFlags_t) RSF_DEFAULT);
	}

	CG_BuildableStatusParse( "ui/assets/human/buildstat.cfg", &cgs.humanBuildStat );
	CG_BuildableStatusParse( "ui/assets/alien/buildstat.cfg", &cgs.alienBuildStat );

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();

	if ( cgs.numInlineModels > MAX_SUBMODELS )
	{
		CG_Error( "MAX_SUBMODELS (%d) exceeded by %d", MAX_SUBMODELS, cgs.numInlineModels - MAX_SUBMODELS );
	}

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

	CG_UpdateMediaFraction( 0.75f );

	// register all the server specified shaders
	for ( i = 1; i < MAX_GAME_SHADERS; i++ )
	{
		const char *shaderName;

		shaderName = CG_ConfigString( CS_SHADERS + i );

		if ( !shaderName[ 0 ] )
		{
			break;
		}

		cgs.gameShaders[ i ] = trap_R_RegisterShader(shaderName,
							     (RegisterShaderFlags_t) RSF_DEFAULT);
	}

	CG_UpdateMediaFraction( 0.77f );

	// register all the server specified grading textures
	// starting with the world wide one
	for ( i = 0; i < MAX_GRADING_TEXTURES; i++ )
	{
		CG_RegisterGrading( i, CG_ConfigString( CS_GRADING_TEXTURES + i ) );
	}

	// register all the server specified reverb effects
	// starting with the world wide one
	for ( i = 0; i < MAX_REVERB_EFFECTS; i++ )
	{
		CG_RegisterReverb( i, CG_ConfigString( CS_REVERB_EFFECTS + i ) );
	}

	CG_UpdateMediaFraction( 0.79f );

	// register all the server specified particle systems
	for ( i = 1; i < MAX_GAME_PARTICLE_SYSTEMS; i++ )
	{
		const char *psName;

		psName = CG_ConfigString( CS_PARTICLE_SYSTEMS + i );

		if ( !psName[ 0 ] )
		{
			break;
		}

		cgs.gameParticleSystems[ i ] = CG_RegisterParticleSystem( ( char * ) psName );
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
		if ( cgs.clientinfo[ i ].infoValid && cgs.clientinfo[ i ].team == TEAM_NONE )
		{
			Q_strcat( cg.spectatorList, sizeof( cg.spectatorList ),
			          va( S_COLOR_WHITE "%s     ", cgs.clientinfo[ i ].name ) );
		}
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
		CG_PrecacheClientInfo( (class_t) i, BG_ClassModelConfig( i )->modelName,
		                       BG_ClassModelConfig( i )->skinName );

		cg.charModelFraction = ( float ) i / ( float ) PCL_NUM_CLASSES;
		trap_UpdateScreen();
	}

	if ( !cg_highPolyPlayerModels.integer )
	{
		cgs.media.larmourHeadSkin = trap_R_RegisterSkin( "models/players/human_base/head_light.skin" );
		cgs.media.larmourLegsSkin = trap_R_RegisterSkin( "models/players/human_base/lower_light.skin" );
		cgs.media.larmourTorsoSkin = trap_R_RegisterSkin( "models/players/human_base/upper_light.skin" );
	}
	else
	{
		// Borrow these variables for MD5 models so we don't have to create new ones.
		cgs.media.larmourHeadSkin = trap_R_RegisterSkin( "models/players/human_base/body_helmet.skin" );
		cgs.media.larmourLegsSkin = trap_R_RegisterSkin( "models/players/human_base/body_larmour.skin" );
		cgs.media.larmourTorsoSkin = trap_R_RegisterSkin( "models/players/human_base/body_helmetlarmour.skin" );
	}

	cgs.media.jetpackModel = trap_R_RegisterModel( "models/players/human_base/jetpack.iqm" );
	cgs.media.jetpackFlashModel = trap_R_RegisterModel( "models/players/human_base/jetpack_flash.md3" );
	cgs.media.radarModel = trap_R_RegisterModel( "models/players/human_base/battpack.md3" ); // HACK: Use old battpack

	CG_RegisterWeaponAnimation(
	    &cgs.media.jetpackAnims[ JANIM_NONE ],
	    "models/players/human_base/jetpack.iqm:idle",
	    qfalse, qfalse, qfalse );


	CG_RegisterWeaponAnimation(
	    &cgs.media.jetpackAnims[ JANIM_SLIDEOUT ],
	    "models/players/human_base/jetpack.iqm:slideout",
	    qfalse, qfalse, qfalse );

	CG_RegisterWeaponAnimation(
	    &cgs.media.jetpackAnims[ JANIM_SLIDEIN ],
	    "models/players/human_base/jetpack.iqm:slidein",
	    qfalse, qfalse, qfalse );

	cg.charModelFraction = 1.0f;
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
	s = ( char * ) CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
}

//
// ==============================
// HUD stuff (mission pack)
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
		trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n",
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
	const char *fallbackFont = "fonts/unifont.ttf";

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return qfalse;
	}

	if ( Q_stricmp( token.string, "{" ) != 0 )
	{
		return qfalse;
	}

	while ( 1 )
	{
		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			return qfalse;
		}

		if ( Q_stricmp( token.string, "}" ) == 0 )
		{
			return qtrue;
		}

		// fallback font
		if ( Q_stricmp( token.string, "fallbackfont" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &fallbackFont ) )
			{
				return qfalse;
			}
			continue;
		}

		// font
		if ( Q_stricmp( token.string, "font" ) == 0 )
		{
			int pointSize;

			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
			{
				return qfalse;
			}

			cgDC.registerFont( tempStr, fallbackFont, pointSize, &cgDC.Assets.textFont );
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

			cgDC.registerFont( tempStr, fallbackFont, pointSize, &cgDC.Assets.smallFont );
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

			cgDC.registerFont( tempStr, fallbackFont, pointSize, &cgDC.Assets.bigFont );
			continue;
		}

		// gradientbar
		if ( Q_stricmp( token.string, "gradientbar" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			cgDC.Assets.gradientBar = trap_R_RegisterShader(tempStr,
									(RegisterShaderFlags_t) RSF_NOMIP);
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

			cgDC.Assets.cursor = trap_R_RegisterShader(cgDC.Assets.cursorStr,
								   (RegisterShaderFlags_t) RSF_NOMIP);
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

	return qfalse;
}

void CG_ParseMenu( const char *menuFile )
{
	pc_token_t token;
	int        handle;

	handle = trap_Parse_LoadSource( menuFile );

	if ( !handle )
	{
		handle = trap_Parse_LoadSource( "ui/testhud.menu" );
	}

	if ( !handle )
	{
		return;
	}

	while ( 1 )
	{
		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//  Com_Printf(_( "Missing { in menu file\n" ));
		//  break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//  Com_Printf(_( "Too many menus!\n" ));
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

	trap_Parse_FreeSource( handle );
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
	int          len;
	fileHandle_t f;
	static char  buf[ MAX_MENUDEFFILE ];
	char         assetScale[ 20 ];

	len = trap_FS_FOpenFile( menuFile, &f, FS_READ );

	if ( !f )
	{
		Com_Printf( S_COLOR_YELLOW  "menu file not found: %s, using default\n", menuFile );
		len = trap_FS_FOpenFile( "ui/hud.txt", &f, FS_READ );

		if ( !f )
		{
			trap_Error( S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!" );
		}
	}

	if ( len >= MAX_MENUDEFFILE )
	{
		trap_FS_FCloseFile( f );
		trap_Error( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i",
		                menuFile, len, MAX_MENUDEFFILE ) );
	}

	trap_FS_Read( buf, len, f );
	buf[ len ] = 0;
	trap_FS_FCloseFile( f );

	COM_Compress( buf );

	Menu_Reset();

	trap_Cvar_VariableStringBuffer( "ui_assetScale", assetScale, sizeof( assetScale ) );
	trap_Parse_AddGlobalDefine( va( "ASSET_SCALE %f", assetScale[ 0 ] ? atof( assetScale ) : 1.0f ) );

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
}

static qboolean CG_OwnerDrawHandleKey( int ownerDraw, int key )
{
	Q_UNUSED(ownerDraw);
	Q_UNUSED(key);
	return qfalse;
}

static int CG_FeederCount( int feederID )
{
	int i, count = 0;

	if ( feederID == FEEDER_ALIENTEAM_LIST )
	{
		for ( i = 0; i < cg.numScores; i++ )
		{
			if ( cg.scores[ i ].team == TEAM_ALIENS )
			{
				count++;
			}
		}
	}
	else if ( feederID == FEEDER_HUMANTEAM_LIST )
	{
		for ( i = 0; i < cg.numScores; i++ )
		{
			if ( cg.scores[ i ].team == TEAM_HUMANS )
			{
				count++;
			}
		}
	}

	return count;
}

void CG_SetScoreSelection( menuDef_t *menu )
{
	playerState_t *ps = &cg.snap->ps;
	int           i, alien, human;
	int           feeder;

	alien = human = 0;

	for ( i = 0; i < cg.numScores; i++ )
	{
		if ( cg.scores[ i ].team == TEAM_ALIENS )
		{
			alien++;
		}
		else if ( cg.scores[ i ].team == TEAM_HUMANS )
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
	i = alien;

	if ( cg.scores[ cg.selectedScore ].team == TEAM_HUMANS )
	{
		feeder = FEEDER_HUMANTEAM_LIST;
		i = human;
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

qboolean CG_ClientIsReady( int clientNum )
{
	clientList_t ready;

	Com_ClientListParse( &ready, CG_ConfigString( CS_CLIENTS_READY ) );

	return Com_ClientListContains( &ready, clientNum );
}

static const char *CG_FeederItemText( int feederID, int index, int column, qhandle_t *handle )
{
	int          scoreIndex = 0;
	clientInfo_t *info = NULL;
	int          team = -1;
	score_t      *sp = NULL;
	qboolean     showIcons = qfalse;

	*handle = -1;

	if ( feederID == FEEDER_ALIENTEAM_LIST )
	{
		team = TEAM_ALIENS;
	}
	else if ( feederID == FEEDER_HUMANTEAM_LIST )
	{
		team = TEAM_HUMANS;
	}

	info = CG_InfoFromScoreIndex( index, team, &scoreIndex );
	sp = &cg.scores[ scoreIndex ];

	if ( cg.intermissionStarted && CG_ClientIsReady( sp->client ) )
	{
		showIcons = qfalse;
	}
	else if ( cg.snap->ps.pm_type == PM_SPECTATOR ||
	          cg.snap->ps.pm_type == PM_NOCLIP ||
	          cg.snap->ps.pm_flags & PMF_FOLLOW ||
	          team == cg.snap->ps.persistant[ PERS_TEAM ] ||
	          cg.intermissionStarted )
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
					if ( sp->team == TEAM_HUMANS && sp->upgrade != UP_NONE )
					{
						*handle = cg_upgrades[ sp->upgrade ].upgradeIcon;
					}
				}

				break;

			case 2:
				if ( cg.intermissionStarted && CG_ClientIsReady( sp->client ) )
				{
					return "Ready";
				}

				break;

			case 3:
				return va( S_COLOR_WHITE "%s", info->name );

			case 4:
				return va( "%d", sp->score );

			case 5:
				return va( "%4d", sp->time );

			case 6:
				if ( sp->ping == -1 )
				{
					return "";
				}

				return va( "%4d", sp->ping );
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage( int feederID, int index )
{
	Q_UNUSED(feederID);
	Q_UNUSED(index);
	return 0;
}

static void CG_FeederSelection( int feederID, int index )
{
	int i, count;
	int team = ( feederID == FEEDER_ALIENTEAM_LIST ) ? TEAM_ALIENS : TEAM_HUMANS;
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

static int CG_OwnerDrawWidth( int ownerDraw, float scale )
{
	switch ( ownerDraw )
	{
		case CG_KILLER:
			return UI_Text_Width( CG_GetKillerText(), scale );
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

// hack to prevent warning
static qboolean CG_OwnerDrawVisible( int parameter )
{
	Q_UNUSED(parameter);
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

	cgDC.aspectScale = ( ( 640.0f * cgs.glconfig.vidHeight ) /
	                     ( 480.0f * cgs.glconfig.vidWidth ) );
	cgDC.xscale = cgs.glconfig.vidWidth / 640.0f;
	cgDC.yscale = cgs.glconfig.vidHeight / 480.0f;

	cgDC.smallFontScale = CG_Cvar_Get( "ui_smallFont" );
	cgDC.bigFontScale = CG_Cvar_Get( "ui_bigFont" );

	cgDC.registerShader = &trap_R_RegisterShader;
	cgDC.setColor = &trap_R_SetColor;
	cgDC.drawHandlePic = &CG_DrawPic;
	cgDC.drawNoStretchPic = &CG_DrawNoStretchPic;
	cgDC.drawStretchPic = &trap_R_DrawStretchPic;
	cgDC.registerModel = &trap_R_RegisterModel;
	cgDC.modelBounds = &trap_R_ModelBounds;
	cgDC.fillRect = &CG_FillRect;
	cgDC.drawRect = &CG_DrawRect;
	cgDC.drawSides = &CG_DrawSides;
	cgDC.drawTopBottom = &CG_DrawTopBottom;
	cgDC.clearScene = &trap_R_ClearScene;
	cgDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	cgDC.renderScene = &trap_R_RenderScene;
	cgDC.registerFont = &trap_R_RegisterFont;
	cgDC.glyph = &UI_R_Glyph;
	cgDC.glyphChar = &UI_R_GlyphChar;
	cgDC.freeCachedGlyphs = &UI_R_UnregisterFont;

	cgDC.ownerDrawItem = &CG_OwnerDraw;
	cgDC.getValue = &CG_GetValue;
	cgDC.ownerDrawVisible = &CG_OwnerDrawVisible;
	cgDC.runScript = &CG_RunMenuScript;
	cgDC.setCVar = trap_Cvar_Set;
	cgDC.getCVarString = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarLatchedString = trap_Cvar_LatchedVariableStringBuffer;
	cgDC.getCVarValue = CG_Cvar_Get;
	cgDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	cgDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound = &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey = &CG_OwnerDrawHandleKey;
	cgDC.feederCount = &CG_FeederCount;
	cgDC.feederItemImage = &CG_FeederItemImage;
	cgDC.feederItemText = &CG_FeederItemText;
	cgDC.feederSelection = &CG_FeederSelection;
	//cgDC.setBinding           = &trap_Key_SetBinding;
	//cgDC.getBindingBuf        = &trap_Key_GetBindingBuf;
	//cgDC.keynumToStringBuf    = &trap_Key_KeynumToStringBuf;
	//cgDC.executeText          = &trap_Cmd_ExecuteText;
	cgDC.Error = &Com_Error;
	cgDC.Print = &Com_Printf;
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	//cgDC.ownerDrawText        = &CG_OwnerDrawText;
	//cgDC.Pause                = &CG_Pause;
	cgDC.registerSound = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	cgDC.playCinematic = &CG_PlayCinematic;
	cgDC.stopCinematic = &CG_StopCinematic;
	cgDC.drawCinematic = &CG_DrawCinematic;
	cgDC.runCinematicFrame = &CG_RunCinematicFrame;

	Init_Display( &cgDC );

	Menu_Reset();

	trap_Cvar_VariableStringBuffer( "cg_hudFiles", buff, sizeof( buff ) );
	hudSet = buff;

	if ( !cg_hudFilesEnable.integer || hudSet[ 0 ] == '\0' )
	{
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus( hudSet );
}

void CG_AssetCache( void )
{
	int i;

	cgDC.Assets.gradientBar = trap_R_RegisterShader(ASSET_GRADIENTBAR,
							(RegisterShaderFlags_t) RSF_NOMIP);
	cgDC.Assets.scrollBar = trap_R_RegisterShader(ASSET_SCROLLBAR,
						      (RegisterShaderFlags_t) RSF_NOMIP);
	cgDC.Assets.scrollBarArrowDown = trap_R_RegisterShader(ASSET_SCROLLBAR_ARROWDOWN,
							       (RegisterShaderFlags_t) RSF_NOMIP);
	cgDC.Assets.scrollBarArrowUp = trap_R_RegisterShader(ASSET_SCROLLBAR_ARROWUP,
							     (RegisterShaderFlags_t) RSF_NOMIP);
	cgDC.Assets.scrollBarArrowLeft = trap_R_RegisterShader(ASSET_SCROLLBAR_ARROWLEFT,
							       (RegisterShaderFlags_t) RSF_NOMIP);
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShader(ASSET_SCROLLBAR_ARROWRIGHT,
								(RegisterShaderFlags_t) RSF_NOMIP);
	cgDC.Assets.scrollBarThumb = trap_R_RegisterShader(ASSET_SCROLL_THUMB,
							   (RegisterShaderFlags_t) RSF_NOMIP);
	cgDC.Assets.sliderBar = trap_R_RegisterShader(ASSET_SLIDER_BAR,
						      (RegisterShaderFlags_t) RSF_NOMIP);
	cgDC.Assets.sliderThumb = trap_R_RegisterShader(ASSET_SLIDER_THUMB,
							(RegisterShaderFlags_t) RSF_NOMIP);

	if ( cg_emoticons.integer )
	{
		cgDC.Assets.emoticonCount = BG_LoadEmoticons( cgDC.Assets.emoticons,
		                            MAX_EMOTICONS );
	}
	else
	{
		cgDC.Assets.emoticonCount = 0;
	}

	for ( i = 0; i < cgDC.Assets.emoticonCount; i++ )
	{
		cgDC.Assets.emoticons[ i ].shader = trap_R_RegisterShader(va("emoticons/%s_%dx1", cgDC.Assets.emoticons[i].name, cgDC.Assets.emoticons[i].width),
									  (RegisterShaderFlags_t) RSF_NOMIP);
	}
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
	trap_SyscallABIVersion( SYSCALL_ABI_VERSION_MAJOR, SYSCALL_ABI_VERSION_MINOR );

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof( cg_entities ) );

	CG_UpdateLoadingStep( LOAD_START );
	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0f;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0f;

	// load a few needed things before we do any screen updates
	trap_R_SetAltShaderTokens( "unpowered,destroyed" );
	cgs.media.whiteShader = trap_R_RegisterShader("white", (RegisterShaderFlags_t) RSF_DEFAULT);
	cgs.media.charsetShader = trap_R_RegisterShader("gfx/2d/bigchars",
							(RegisterShaderFlags_t) RSF_DEFAULT);
	cgs.media.outlineShader = trap_R_RegisterShader("outline",
							(RegisterShaderFlags_t) RSF_DEFAULT);

	// Dynamic memory
	BG_InitMemory();

	BG_InitAllowedGameElements();

	// Initialize item locking state
	BG_InitUnlockackables();

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	String_Init();

	trap_S_BeginRegistration();

	CG_AssetCache();
	CG_LoadHudMenu();

	cg.weaponSelect = WP_NONE;

	// old servers

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	// copy vote display strings so they don't show up blank if we see
	// the same one directly after connecting
	Q_strncpyz( cgs.voteString[ TEAM_NONE ],
	            CG_ConfigString( CS_VOTE_STRING + TEAM_NONE ),
	            sizeof( cgs.voteString ) );
	Q_strncpyz( cgs.voteString[ TEAM_ALIENS ],
	            CG_ConfigString( CS_VOTE_STRING + TEAM_ALIENS ),
	            sizeof( cgs.voteString[ TEAM_ALIENS ] ) );
	Q_strncpyz( cgs.voteString[ TEAM_HUMANS ],
	            CG_ConfigString( CS_VOTE_STRING + TEAM_HUMANS ),
	            sizeof( cgs.voteString[ TEAM_HUMANS ] ) );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );

//   if( strcmp( s, GAME_VERSION ) )
//     CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	CG_ParseServerinfo();

	// load the new map
	trap_CM_LoadMap( va( "maps/%s.bsp", cgs.mapname) );

	srand( serverMessageNum * serverCommandSequence ^ clientNum );

	CG_UpdateLoadingStep( LOAD_TRAILS );
	CG_LoadTrailSystems();

	CG_UpdateLoadingStep( LOAD_PARTICLES );
	CG_LoadParticleSystems();

	CG_UpdateLoadingStep( LOAD_SOUNDS );
	CG_RegisterSounds();

	// updates loading step by itself
	CG_RegisterGraphics();

	// load configs after initializing particles and trails since it registers some
	CG_UpdateLoadingStep( LOAD_CONFIGS );
	BG_InitAllConfigs();

	// load weapons upgrades and buildings after configs
	CG_UpdateLoadingStep( LOAD_WEAPONS );
	CG_InitWeapons();

	CG_UpdateLoadingStep( LOAD_UPGRADES );
	CG_InitUpgrades();

	CG_UpdateLoadingStep( LOAD_CLASSES );
	CG_InitClasses();

	CG_UpdateLoadingStep( LOAD_BUILDINGS );
	CG_InitBuildables();

	CG_UpdateLoadingStep( LOAD_REMAINING );

	cgs.voices = BG_VoiceInit();
	BG_PrintVoices( cgs.voices, cg_debugVoices.integer );

	CG_RegisterClients(); // if low on memory, some clients will be deferred

	CG_InitMarkPolys();

	CG_InitMinimap();

	trap_S_EndRegistration();

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds( qtrue );
	trap_Cvar_Set( "ui_winner", "" ); // Clear the previous round's winner.

	CG_UpdateLoadingStep( LOAD_DONE );
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
	UIS_Shutdown();

	BG_UnloadAllConfigs();
}

/*
================
CG_VoIPString
================
*/
static char *CG_VoIPString( void )
{
	// a generous overestimate of the space needed for 0,1,2...61,62,63
	static char voipString[ MAX_CLIENTS * 4 ];

	int i, slen, nlen;

	for ( slen = i = 0; i < cgs.maxclients; i++ )
	{
		if ( !cgs.clientinfo[ i ].infoValid || i == cg.clientNum )
		{
			continue;
		}

		if ( cgs.clientinfo[ i ].team != cgs.clientinfo[ cg.clientNum ].team )
		{
			continue;
		}

		nlen = Q_snprintf( &voipString[ slen ], sizeof( voipString ) - slen,
							"%s%d", ( slen > 0 ) ? "," : "", i );

		if ( slen + nlen + 1 >= (int) sizeof( voipString ) )
		{
			CG_Printf( S_WARNING "voipString overflowed\n" );
			break;
		}

		slen += nlen;
	}

	// Notice that if the snprintf was truncated, slen was not updated
	// so this will remove any trailing commas or partially-completed numbers
	voipString[ slen ] = '\0';

	return voipString;
}

const vec3_t cg_shaderColors[ SHC_NUM_SHADER_COLORS ] =
{
	{ 0.0f,   0.0f,     0.75f    }, // dark blue
	{ 0.3f,   0.35f,    0.625f   }, // light blue
	{ 0.0f,   0.625f,   0.563f   }, // green-cyan
	{ 0.313f, 0.0f,     0.625f   }, // violet
	{ 0.54f,  0.0f,     1.0f     }, // indigo
	{ 0.625f, 0.625f,   0.0f     }, // yellow
	{ 0.875f, 0.313f,   0.0f     }, // orange
	{ 0.375f, 0.625f,   0.375f   }, // light green
	{ 0.0f,   0.438f,   0.0f     }, // dark green
	{ 1.0f,   0.0f,     0.0f     }, // red
	{ 0.625f, 0.375f,   0.4f     }, // pink
	{ 0.313f, 0.313f,   0.313f   } // grey
};
