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

// cg_main.c -- initialization for cgame

#include "cg_local.h"

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
vmCvar_t        cg_lagometer;
vmCvar_t        cg_drawSpeed;
vmCvar_t        cg_maxSpeedTimeWindow;
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
vmCvar_t        cg_spawnEffects;

vmCvar_t        cg_fov_builder;
vmCvar_t        cg_fov_level0;
vmCvar_t        cg_fov_level1;
vmCvar_t        cg_fov_level2;
vmCvar_t        cg_fov_level3;
vmCvar_t        cg_fov_level4;
vmCvar_t        cg_fov_human;

vmCvar_t        ui_chatPromptColors;
vmCvar_t        cg_sayCommand;

typedef struct
{
	vmCvar_t   *vmCvar;
	const char *cvarName;
	const char *defaultString;
	int        cvarFlags;
} cvarTable_t;

static const cvarTable_t cvarTable[] =
{
	{ &cg_drawGun,                     "cg_drawGun",                     "1",            0                            },
	{ &cg_viewsize,                    "cg_viewsize",                    "100",          0                            },
	{ &cg_shadows,                     "cg_shadows",                     "1",            CVAR_LATCH                   },
	{ &cg_playerShadows,               "cg_playerShadows",               "1",            0                            },
	{ &cg_buildableShadows,            "cg_buildableShadows",            "0",            0                            },
	{ &cg_draw2D,                      "cg_draw2D",                      "1",            0                            },
	{ &cg_drawTimer,                   "cg_drawTimer",                   "1",            0                            },
	{ &cg_drawClock,                   "cg_drawClock",                   "0",            0                            },
	{ &cg_drawFPS,                     "cg_drawFPS",                     "1",            0                            },
	{ &cg_drawDemoState,               "cg_drawDemoState",               "1",            0                            },
	{ &cg_drawSnapshot,                "cg_drawSnapshot",                "0",            0                            },
	{ &cg_drawChargeBar,               "cg_drawChargeBar",               "1",            0                            },
	{ &cg_drawCrosshair,               "cg_drawCrosshair",               "2",            0                            },
	{ &cg_drawCrosshairHit,            "cg_drawCrosshairHit",            "1",            0                            },
	{ &cg_drawCrosshairFriendFoe,      "cg_drawCrosshairFriendFoe",      "0",            0                            },
	{ &cg_drawCrosshairNames,          "cg_drawCrosshairNames",          "1",            0                            },
	{ &cg_drawBuildableHealth,         "cg_drawBuildableHealth",         "1",            0                            },
	{ &cg_drawMinimap,                 "cg_drawMinimap",                 "1",            0                            },
	{ &cg_minimapActive,               "cg_minimapActive",               "0",            0                            },
	{ &cg_crosshairSize,               "cg_crosshairSize",               "1",            0                            },
	{ &cg_crosshairFile,               "cg_crosshairFile",               "",             0                            },
	{ &cg_addMarks,                    "cg_marks",                       "1",            0                            },
	{ &cg_lagometer,                   "cg_lagometer",                   "0",            0                            },
	{ &cg_drawSpeed,                   "cg_drawSpeed",                   "0",            0                            },
	{ &cg_maxSpeedTimeWindow,          "cg_maxSpeedTimeWindow",          "2000",         0                            },
	{ &cg_teslaTrailTime,              "cg_teslaTrailTime",              "250",          0                            },
	{ &cg_gun_x,                       "cg_gunX",                        "0",            CVAR_CHEAT                   },
	{ &cg_gun_y,                       "cg_gunY",                        "0",            CVAR_CHEAT                   },
	{ &cg_gun_z,                       "cg_gunZ",                        "0",            CVAR_CHEAT                   },
	{ &cg_mirrorgun,                   "cg_mirrorgun",                   "0",            0                            },
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
	{ &cg_tracerChance,                "cg_tracerchance",                "1",            CVAR_CHEAT                   },
	{ &cg_tracerWidth,                 "cg_tracerwidth",                 "3",            CVAR_CHEAT                   },
	{ &cg_tracerLength,                "cg_tracerlength",                "200",          CVAR_CHEAT                   },
	{ &cg_thirdPersonRange,            "cg_thirdPersonRange",            "75",           0                            },
	{ &cg_thirdPerson,                 "cg_thirdPerson",                 "0",            CVAR_CHEAT                   },
	{ &cg_thirdPersonAngle,            "cg_thirdPersonAngle",            "0",            CVAR_CHEAT                   },
	{ &cg_thirdPersonPitchFollow,      "cg_thirdPersonPitchFollow",      "0",            0                            },
	{ &cg_thirdPersonShoulderViewMode, "cg_thirdPersonShoulderViewMode", "1",            0                            },
	{ &cg_staticDeathCam,              "cg_staticDeathCam",              "0",            0                            },
	{ &cg_stats,                       "cg_stats",                       "0",            0                            },
	{ &cg_drawTeamOverlay,             "cg_drawTeamOverlay",             "1",            0                            },
	{ &cg_teamOverlaySortMode,         "cg_teamOverlaySortMode",         "1",            0                            },
	{ &cg_teamOverlayMaxPlayers,       "cg_teamOverlayMaxPlayers",       "8",            0                            },
	{ &cg_teamOverlayUserinfo,         "teamoverlay",                    "1",            CVAR_USERINFO                },
	{ &cg_teamChatsOnly,               "cg_teamChatsOnly",               "0",            0                            },
	{ &cg_noPrintDuplicate,            "cg_noPrintDuplicate",            "0",            0                            },
	{ &cg_noVoiceChats,                "cg_noVoiceChats",                "0",            0                            },
	{ &cg_noVoiceText,                 "cg_noVoiceText",                 "0",            0                            },
	{ &cg_drawSurfNormal,              "cg_drawSurfNormal",              "0",            CVAR_CHEAT                   },
	{ &cg_drawBBOX,                    "cg_drawBBOX",                    "0",            CVAR_CHEAT                   },
	{ &cg_drawEntityInfo,              "cg_drawEntityInfo",              "0",            CVAR_CHEAT                   },
	{ &cg_wwSmoothTime,                "cg_wwSmoothTime",                "150",          0                            },
	{ nullptr,                            "cg_wwFollow",                    "1",            CVAR_USERINFO                },
	{ nullptr,                            "cg_wwToggle",                    "1",            CVAR_USERINFO                },
	{ nullptr,                            "cg_disableBlueprintErrors",      "0",            CVAR_USERINFO                },
	{ &cg_stickySpec,                  "cg_stickySpec",                  "1",            CVAR_USERINFO                },
	{ &cg_sprintToggle,                "cg_sprintToggle",                "0",            CVAR_USERINFO                },
	{ &cg_unlagged,                    "cg_unlagged",                    "1",            CVAR_USERINFO                },
	{ nullptr,                            "cg_flySpeed",                    "800",          CVAR_USERINFO                },
	{ &cg_depthSortParticles,          "cg_depthSortParticles",          "1",            0                            },
	{ &cg_bounceParticles,             "cg_bounceParticles",             "0",            0                            },
	{ &cg_consoleLatency,              "cg_consoleLatency",              "3000",         0                            },
	{ &cg_lightFlare,                  "cg_lightFlare",                  "3",            0                            },
	{ &cg_debugParticles,              "cg_debugParticles",              "0",            CVAR_CHEAT                   },
	{ &cg_debugTrails,                 "cg_debugTrails",                 "0",            CVAR_CHEAT                   },
	{ &cg_debugPVS,                    "cg_debugPVS",                    "0",            CVAR_CHEAT                   },
	{ &cg_disableWarningDialogs,       "cg_disableWarningDialogs",       "0",            0                            },
	{ &cg_disableUpgradeDialogs,       "cg_disableUpgradeDialogs",       "0",            0                            },
	{ &cg_disableBuildDialogs,         "cg_disableBuildDialogs",         "0",            0                            },
	{ &cg_disableCommandDialogs,       "cg_disableCommandDialogs",       "0",            0                            },
	{ &cg_disableScannerPlane,         "cg_disableScannerPlane",         "0",            0                            },
	{ &cg_tutorial,                    "cg_tutorial",                    "1",            0                            },

	{ &cg_rangeMarkerDrawSurface,      "cg_rangeMarkerDrawSurface",      "1",            0                            },
	{ &cg_rangeMarkerDrawIntersection, "cg_rangeMarkerDrawIntersection", "0",            0                            },
	{ &cg_rangeMarkerDrawFrontline,    "cg_rangeMarkerDrawFrontline",    "0",            0                            },
	{ &cg_rangeMarkerSurfaceOpacity,   "cg_rangeMarkerSurfaceOpacity",   "0.08",         0                            },
	{ &cg_rangeMarkerLineOpacity,      "cg_rangeMarkerLineOpacity",      "0.4",          0                            },
	{ &cg_rangeMarkerLineThickness,    "cg_rangeMarkerLineThickness",    "4.0",          0                            },
	{ &cg_rangeMarkerForBlueprint,     "cg_rangeMarkerForBlueprint",     "1",            0                            },
	{ &cg_rangeMarkerBuildableTypes,   "cg_rangeMarkerBuildableTypes",   "support",      0                            },
	{ &cg_rangeMarkerWhenSpectating,   "cg_rangeMarkerWhenSpectating",   "0",            0                            },
	{ &cg_buildableRangeMarkerMask,    "cg_buildableRangeMarkerMask",    "",             0                            },
	{ &cg_binaryShaderScreenScale,     "cg_binaryShaderScreenScale",     "1.0",          0                            },

	{ &cg_hudFiles,                    "cg_hudFiles",                    "ui/hud.txt",   0                            },
	{ &cg_hudFilesEnable,              "cg_hudFilesEnable",              "0",            0                            },
	{ nullptr,                            "cg_alienConfig",                 "",             0                            },
	{ nullptr,                            "cg_humanConfig",                 "",             0                            },
	{ nullptr,                            "cg_spectatorConfig",             "",             0                            },

	{ &cg_painBlendUpRate,             "cg_painBlendUpRate",             "10.0",         0                            },
	{ &cg_painBlendDownRate,           "cg_painBlendDownRate",           "0.5",          0                            },
	{ &cg_painBlendMax,                "cg_painBlendMax",                "0.7",          0                            },
	{ &cg_painBlendScale,              "cg_painBlendScale",              "7.0",          0                            },
	{ &cg_painBlendZoom,               "cg_painBlendZoom",               "0.65",         0                            },

	{ &cg_cmdGrenadeThrown,            "cg_cmdGrenadeThrown",            "vsay_local grenade", 0                      },
	{ &cg_cmdNeedHealth,               "cg_cmdNeedHealth",               "vsay_local needhealth", 0                   },

	{ &cg_debugVoices,                 "cg_debugVoices",                 "0",            0                            },

	{ &cg_debugRandom,                 "cg_debugRandom",                 "0",            0                            },

	{ &cg_optimizePrediction,          "cg_optimizePrediction",          "1",            0                            },
	{ &cg_projectileNudge,             "cg_projectileNudge",             "1",            0                            },

	// the following variables are created in other parts of the system,
	// but we also reference them here

	{ &cg_paused,                      "cl_paused",                      "0",            CVAR_ROM                     },
	{ &cg_blood,                       "com_blood",                      "1",            0                            },
	{ &cg_timescaleFadeEnd,            "cg_timescaleFadeEnd",            "1",            CVAR_CHEAT                   },
	{ &cg_timescaleFadeSpeed,          "cg_timescaleFadeSpeed",          "0",            CVAR_CHEAT                   },
	{ &cg_timescale,                   "timescale",                      "1",            0                            },
	{ &cg_smoothClients,               "cg_smoothClients",               "0",            CVAR_USERINFO                },

	{ &cg_noTaunt,                     "cg_noTaunt",                     "0",            0                            },

	{ &cg_voice,                       "voice",                          "default",      CVAR_USERINFO                },

	{ &cg_emoticons,                   "cg_emoticons",                   "1",            CVAR_LATCH                   },
	{ &cg_emoticonsInMessages,         "cg_emoticonsInMessages",         "0",            0                            },

	{ &cg_animSpeed,                   "cg_animspeed",                   "1",            CVAR_CHEAT                   },
	{ &cg_animBlend,                   "cg_animblend",                   "5.0",          0                            },

	{ &cg_chatTeamPrefix,              "cg_chatTeamPrefix",              "1",            0                            },
	{ &cg_highPolyPlayerModels,        "cg_highPolyPlayerModels",        "1",            CVAR_LATCH                   },
	{ &cg_highPolyBuildableModels,     "cg_highPolyBuildableModels",     "1",            CVAR_LATCH                   },
	{ &cg_highPolyWeaponModels,        "cg_highPolyWeaponModels",        "1",            CVAR_LATCH                   },
	{ &cg_motionblur,                  "cg_motionblur",                  "0.05",         0                            },
	{ &cg_motionblurMinSpeed,          "cg_motionblurMinSpeed",          "600",          0                            },
	{ &cg_spawnEffects,                "cg_spawnEffects",                "1",            0                            },
	{ &cg_fov_builder,                 "cg_fov_builder",                 "0",            0                            },
	{ &cg_fov_level0,                  "cg_fov_level0",                  "0",            0                            },
	{ &cg_fov_level1,                  "cg_fov_level1",                  "0",            0                            },
	{ &cg_fov_level2,                  "cg_fov_level2",                  "0",            0                            },
	{ &cg_fov_level3,                  "cg_fov_level3",                  "0",            0                            },
	{ &cg_fov_level4,                  "cg_fov_level4",                  "0",            0                            },
	{ &cg_fov_human,                   "cg_fov_human",                   "0",            0                            },

	{ &ui_chatPromptColors,            "ui_chatPromptColors",            "1",            0                            },

	{ &cg_sayCommand,                  "cg_sayCommand",                   "",            0                            }
};

static const size_t cvarTableSize = ARRAY_LEN( cvarTable );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars()
{
	size_t i;
	const cvarTable_t *cv;

	for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
	{
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
		                    cv->defaultString, cv->cvarFlags );
	}
}

int FloatAsInt( float f )
{
	floatint_t fi;

	fi.f = f;
	return fi.i;
}


/*
===============
CG_SetPVars

Set some player cvars usable in scripts
these should refer only to playerstates that belong to the client, not the followed player, ui cvars will do that already
===============
*/
static void CG_SetPVars()
{
	playerState_t *ps;
	char          buffer[ MAX_CVAR_VALUE_STRING ];
	int           i;
	bool      first;

	if ( !cg.snap )
	{
		return;
	}

	ps = &cg.snap->ps;

	/* if we follow someone, the stats won't be about us, but the followed player instead */
	if ( ( ps->pm_flags & PMF_FOLLOW ) )
	{
	        return;
	}

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

	trap_Cvar_Set( "p_classname", BG_Class( ps->stats[ STAT_CLASS ] )->name );


	trap_Cvar_Set( "p_weapon", va( "%d", ps->stats[ STAT_WEAPON ] ) );
	trap_Cvar_Set( "p_weaponname", BG_Weapon( ps->stats[ STAT_WEAPON ] )->humanName );
	trap_Cvar_Set( "p_credits", va( "%d", ps->persistant[ PERS_CREDIT ] ) );
	trap_Cvar_Set( "p_score", va( "%d", ps->persistant[ PERS_SCORE ] ) );

	trap_Cvar_Set( "p_hp", va( "%d", ps->stats[ STAT_HEALTH ] ) );
	trap_Cvar_Set( "p_maxhp", va( "%d", ps->stats[ STAT_MAX_HEALTH ] ) );
	trap_Cvar_Set( "p_ammo", va( "%d", ps->ammo ) );
	trap_Cvar_Set( "p_clips", va( "%d", ps->clips ) );

	// set p_availableBuildings to a space-separated list of buildings
	first = true;
	*buffer = 0;

	for ( i = BA_NONE; i < BA_NUM_BUILDABLES; ++i )
	{
		const buildableAttributes_t *buildable = BG_Buildable( i );

		if ( buildable->team == ps->persistant[ PERS_TEAM ] &&
		     BG_BuildableUnlocked( i ) &&
		     (buildable->buildWeapon & ( 1 << ps->stats[ STAT_WEAPON ] ) ) )

		{
			Q_strcat( buffer, sizeof( buffer ), first ? buildable->name : va( " %s", buildable->name ) );
			first = false;
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
static void CG_SetUIVars()
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
void CG_UpdateBuildableRangeMarkerMask()
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
				           ( 1 << BA_H_MGTURRET ) | ( 1 << BA_H_ROCKETPOD ) | ( 1 << BA_H_DRILL );
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
					       ( 1 << BA_H_MGTURRET ) | ( 1 << BA_H_ROCKETPOD ) | ( 1 << BA_H_DRILL );
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
					                    ( 1 << BA_H_MGTURRET ) | ( 1 << BA_H_ROCKETPOD ) );
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

void CG_NotifyHooks()
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
void CG_UpdateCvars()
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

int CG_CrosshairPlayer()
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
void CG_RemoveNotifyLine()
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
TODO TODO this can be removed, as well as all its dependencies
=================
*/
void CG_AddNotifyText()
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

/*
================
CG_Args
================
*/
const char *CG_Args()
{
	static char buffer[ MAX_STRING_CHARS ];

	trap_LiteralArgs( buffer, sizeof( buffer ) );

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
bool CG_FileExists( const char *filename )
{
	return trap_FS_FOpenFile( filename, nullptr, FS_READ );
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
			cg.loading = true;
			cg.mediaFraction = cg.charModelFraction = cg.buildablesFraction = 0.0f;
			break;

		case LOAD_TRAILS:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.0f, choose("Tracking your movements", "Letting out the magic smoke", nullptr) );
			break;
		case LOAD_PARTICLES:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.05f, choose("Collecting bees for the hives", "Initialising fireworks", "Causing electrical faults", nullptr) );
			break;
		case LOAD_SOUNDS:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.08f, choose("Recording granger purring", "Generating annoying noises", nullptr) );
			break;
		case LOAD_GEOMETRY:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.60f, choose("Hello World!", "Making a scene.", nullptr) );
			break;
		case LOAD_ASSETS:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.63f, choose("Taking pictures of the world", "Using your laptop's camera", "Adding texture to concrete", "Drawing smiley faces", nullptr) );
			break;
		case LOAD_CONFIGS:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.80f, choose("Reading the manual", "Looking at blueprints", nullptr) );
			break;
		case LOAD_WEAPONS:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.90f, choose("Setting up the armoury", "Sharpening the aliens' claws", "Overloading lucifer cannons", nullptr) );
			break;
		case LOAD_UPGRADES:
		case LOAD_CLASSES:
			CG_UpdateLoadingProgress( LOADBAR_MEDIA, 0.95f, choose("Charging battery packs", "Replicating alien DNA", "Packing tents for jetcampers", nullptr) );
			break;
		case LOAD_BUILDINGS:
			cg.mediaFraction = 1.0f;
			CG_UpdateLoadingProgress( LOADBAR_BUILDABLES, 0.0f, choose("Finishing construction", "Adding turret spam", "Awakening the overmind", nullptr) );
			break;

		case LOAD_DONE:
			cg.mediaFraction = cg.charModelFraction = cg.buildablesFraction = 1.0f;
			Q_strncpyz(cg.currentLoadingLabel, "Done!", sizeof( cg.currentLoadingLabel ) );
			trap_UpdateScreen();
			cg.loading = false;
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
static void CG_RegisterSounds()
{
	int        i;
	char       name[ MAX_QPATH ];
	const char *soundName;

	cgs.media.weHaveEvolved = trap_S_RegisterSound( "sound/announcements/overmindevolved.wav", true );
	cgs.media.reinforcement = trap_S_RegisterSound( "sound/announcements/reinforcement.wav", true );

	cgs.media.alienOvermindAttack = trap_S_RegisterSound( "sound/announcements/overmindattack.wav", true );
	cgs.media.alienOvermindDying = trap_S_RegisterSound( "sound/announcements/overminddying.wav", true );
	cgs.media.alienOvermindSpawns = trap_S_RegisterSound( "sound/announcements/overmindspawns.wav", true );

	cgs.media.alienL4ChargePrepare = trap_S_RegisterSound( "sound/player/level4/charge_prepare.wav", true );
	cgs.media.alienL4ChargeStart = trap_S_RegisterSound( "sound/player/level4/charge_start.wav", true );

	cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change.wav", false );
	cgs.media.turretSpinupSound = trap_S_RegisterSound( "sound/buildables/mgturret/spinup.wav", false );
	cgs.media.weaponEmptyClick = trap_S_RegisterSound( "sound/weapons/click.wav", false );

	cgs.media.talkSound = trap_S_RegisterSound( "sound/misc/talk.wav", false );
	cgs.media.alienTalkSound = trap_S_RegisterSound( "sound/misc/alien_talk.wav", false );
	cgs.media.humanTalkSound = trap_S_RegisterSound( "sound/misc/human_talk.wav", false );
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav", false );

	cgs.media.watrInSound = trap_S_RegisterSound( "sound/player/watr_in.wav", false );
	cgs.media.watrOutSound = trap_S_RegisterSound( "sound/player/watr_out.wav", false );
	cgs.media.watrUnSound = trap_S_RegisterSound( "sound/player/watr_un.wav", false );

	cgs.media.disconnectSound = trap_S_RegisterSound( "sound/misc/disconnect.wav", false );

	for ( i = 0; i < 4; i++ )
	{
		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/step%i.wav", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_GENERAL ][ i ] = trap_S_RegisterSound( name, false );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/flesh%i.wav", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_FLESH ][ i ] = trap_S_RegisterSound( name, false );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/splash%i.wav", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_SPLASH ][ i ] = trap_S_RegisterSound( name, false );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/clank%i.wav", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_METAL ][ i ] = trap_S_RegisterSound( name, false );
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

		cgs.gameSounds[ i ] = trap_S_RegisterSound( soundName, false );
	}

	cgs.media.jetpackThrustLoopSound = trap_S_RegisterSound( "sound/upgrades/jetpack/hi.wav", false );

	cgs.media.medkitUseSound = trap_S_RegisterSound( "sound/upgrades/medkit/medkit.wav", false );

	cgs.media.alienEvolveSound = trap_S_RegisterSound( "sound/player/alienevolve.wav", false );

	cgs.media.alienBuildableExplosion = trap_S_RegisterSound( "sound/buildables/alien/explosion.wav", false );
	cgs.media.alienBuildablePrebuild = trap_S_RegisterSound( "sound/buildables/alien/prebuild.wav", false );

	cgs.media.humanBuildableDying = trap_S_RegisterSound( "sound/buildables/human/dying.wav", false );
	cgs.media.humanBuildableExplosion = trap_S_RegisterSound( "sound/buildables/human/explosion.wav", false );
	cgs.media.humanBuildablePrebuild = trap_S_RegisterSound( "sound/buildables/human/prebuild.wav", false );

	for ( i = 0; i < 4; i++ )
	{
		cgs.media.humanBuildableDamage[ i ] = trap_S_RegisterSound(
		                                        va( "sound/buildables/human/damage%d.wav", i ), false );
	}

	cgs.media.hardBounceSound1 = trap_S_RegisterSound( "sound/misc/hard_bounce1.wav", false );
	cgs.media.hardBounceSound2 = trap_S_RegisterSound( "sound/misc/hard_bounce2.wav", false );

	cgs.media.repeaterUseSound = trap_S_RegisterSound( "sound/buildables/repeater/use.wav", false );

	cgs.media.buildableRepairSound = trap_S_RegisterSound( "sound/buildables/human/repair.wav", false );
	cgs.media.buildableRepairedSound = trap_S_RegisterSound( "sound/buildables/human/repaired.wav", false );

	cgs.media.lCannonWarningSound = trap_S_RegisterSound( "models/weapons/lcannon/warning.wav", false );
	cgs.media.lCannonWarningSound2 = trap_S_RegisterSound( "models/weapons/lcannon/warning2.wav", false );

	cgs.media.rocketpodLockonSound = trap_S_RegisterSound( "sound/rocketpod/lockon.wav", false );

	cgs.media.timerBeaconExpiredSound = trap_S_RegisterSound( "sound/feedback/beacon-timer-expired.ogg", false );
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
static void CG_RegisterGraphics()
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
	cgs.media.humanSpawningShader = trap_R_RegisterShader("models/buildables/humanSpawning",
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
							(RegisterShaderFlags_t) RSF_SPRITE);

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

	cgs.media.beaconIconArrow = trap_R_RegisterShader( "gfx/2d/beacons/arrow", RSF_DEFAULT );
	cgs.media.beaconNoTarget = trap_R_RegisterShader( "gfx/2d/beacons/no-target", RSF_DEFAULT );
	cgs.media.beaconTagScore = trap_R_RegisterShader( "gfx/2d/beacons/tagscore", RSF_DEFAULT );

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
void CG_BuildSpectatorString()
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
static void CG_RegisterClients()
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
	    false, false, false );


	CG_RegisterWeaponAnimation(
	    &cgs.media.jetpackAnims[ JANIM_SLIDEOUT ],
	    "models/players/human_base/jetpack.iqm:slideout",
	    false, false, false );

	CG_RegisterWeaponAnimation(
	    &cgs.media.jetpackAnims[ JANIM_SLIDEIN ],
	    "models/players/human_base/jetpack.iqm:slidein",
	    false, false, false );

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

	return cgs.gameState[index].c_str();
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic()
{
	const char *s;
	char parm1[ MAX_QPATH ], parm2[ MAX_QPATH ];

	// start the background music
	s = ( char * ) CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
}

bool CG_ClientIsReady( int clientNum )
{
	clientList_t ready;

	Com_ClientListParse( &ready, CG_ConfigString( CS_CLIENTS_READY ) );

	return Com_ClientListContains( &ready, clientNum );
}

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/

void CG_Init( int serverMessageNum, int clientNum, glconfig_t gl, GameStateCSs gameState)
{
	const char *s;

	// clear everything
	// reset cgs in-place to avoid creating a huge struct on stack (caused a stack overflow)
	// this is equivalent to cgs = cgs_t()
	cgs.~cgs_t();
	new(&cgs) cgs_t{}; // Using {} instead of () to work around MSVC bug
	cg.~cg_t();
	new(&cg) cg_t{};
	memset( cg_entities, 0, sizeof( cg_entities ) );

	// Set up the pmove params with sensible default values, the server params will
	// be communicated with the "pmove_params" server commands.
	// These values are the same as the default values of the servers to preserve
	// compatibility with official Alpha 34 servers, but shouldn't be necessary anymore for Alpha 35
	cg.pmoveParams.fixed = cg.pmoveParams.synchronous = 0;
	cg.pmoveParams.accurate = 1;
	cg.pmoveParams.msec = 8;

	CG_UpdateLoadingStep( LOAD_START );
	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;

	// get the rendering configuration from the client system
	cgs.glconfig = gl;
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0f;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0f;
	cgs.aspectScale = ( ( 640.0f * cgs.glconfig.vidHeight ) /
	( 480.0f * cgs.glconfig.vidWidth ) );

	// load a few needed things before we do any screen updates
	trap_R_SetAltShaderTokens( "unpowered,destroyed" );
	cgs.media.whiteShader = trap_R_RegisterShader("white", (RegisterShaderFlags_t) RSF_DEFAULT);
	cgs.media.charsetShader = trap_R_RegisterShader("gfx/2d/bigchars",
							(RegisterShaderFlags_t) RSF_DEFAULT);
	cgs.media.outlineShader = trap_R_RegisterShader("outline",
							(RegisterShaderFlags_t) RSF_DEFAULT);

	BG_InitAllowedGameElements();

	// Initialize item locking state
	BG_InitUnlockackables();

	CG_RegisterCvars();

	CG_InitConsoleCommands();
	trap_S_BeginRegistration();


	cg.weaponSelect = WP_NONE;

	// old servers

	cgs.gameState = gameState;

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
	trap_CM_LoadMap(cgs.mapname);
	CG_InitMinimap();

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

	trap_S_EndRegistration();

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds( true );
	trap_Cvar_Set( "ui_winner", "" ); // Clear the previous round's winner.

	CG_Rocket_LoadHuds();

	CG_LoadBeaconsConfig();

	CG_UpdateLoadingStep( LOAD_DONE );
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown()
{
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
	CG_Rocket_CleanUpDataSources();
	Rocket_Shutdown();
	BG_UnloadAllConfigs();
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
