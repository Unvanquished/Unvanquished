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

#include "sg_local.h"
#include "CBSE.h"

#define INTERMISSION_DELAY_TIME 1000

level_locals_t level;

typedef struct
{
	vmCvar_t   *vmCvar;
	const char *cvarName;
	const char *defaultString;
	int        cvarFlags;
	int        modificationCount; // for tracking changes
	bool   trackChange; // track this variable, and announce if changed

	/* certain cvars can be set in worldspawn, but we don't want those values to
	   persist, so keep track of non-worldspawn changes and restore that on map
	   end. unfortunately, if the server crashes, the value set in worldspawn may
	   persist */
	char      *explicit_;
} cvarTable_t;

#ifdef QVM_ABI
gentity_t          g_entities[ MAX_GENTITIES ];
gclient_t          g_clients[ MAX_GENTITIES ];
#else
gentity_t          *g_entities;
gclient_t          *g_clients;
#endif

vmCvar_t           g_showHelpOnConnection;

vmCvar_t           g_timelimit;
vmCvar_t           g_friendlyFire;
vmCvar_t           g_friendlyBuildableFire;
vmCvar_t           g_dretchPunt;
vmCvar_t           g_password;
vmCvar_t           g_needpass;
vmCvar_t           g_maxclients;
vmCvar_t           g_maxGameClients;
vmCvar_t           g_speed;
vmCvar_t           g_gravity;
vmCvar_t           g_cheats;
vmCvar_t           g_inactivity;
vmCvar_t           g_debugMove;
vmCvar_t           g_debugDamage;
vmCvar_t           g_debugKnockback;
vmCvar_t           g_debugTurrets;
vmCvar_t           g_debugFire;
vmCvar_t           g_motd;
vmCvar_t           g_synchronousClients;
vmCvar_t           g_warmup;
vmCvar_t           g_doWarmup;
vmCvar_t           g_lockTeamsAtStart;
vmCvar_t           g_mapRestarted;
vmCvar_t           g_logFile;
vmCvar_t           g_logGameplayStatsFrequency;
vmCvar_t           g_logFileSync;
vmCvar_t           g_allowVote;
vmCvar_t           g_voteLimit;
vmCvar_t           g_extendVotesPercent;
vmCvar_t           g_extendVotesTime;
vmCvar_t           g_kickVotesPercent;
vmCvar_t           g_denyVotesPercent;
vmCvar_t           g_mapVotesPercent;
vmCvar_t           g_mapVotesBefore;
vmCvar_t           g_drawVotesPercent;
vmCvar_t           g_drawVotesAfter;
vmCvar_t           g_drawVoteReasonRequired;
vmCvar_t           g_admitDefeatVotesPercent;
vmCvar_t           g_nextMapVotesPercent;
vmCvar_t           g_pollVotesPercent;
vmCvar_t           g_botKickVotesAllowed;
vmCvar_t           g_botKickVotesAllowedThisMap;

vmCvar_t           g_teamForceBalance;
vmCvar_t           g_smoothClients;
vmCvar_t           pmove_fixed;
vmCvar_t           pmove_msec;
vmCvar_t           pmove_accurate;
vmCvar_t           g_minNameChangePeriod;
vmCvar_t           g_maxNameChanges;

vmCvar_t           g_initialBuildPoints;
vmCvar_t           g_initialMineRate;
vmCvar_t           g_mineRateHalfLife;
vmCvar_t           g_minimumMineRate;
vmCvar_t           g_buildPointLossFraction;

vmCvar_t           g_debugMomentum;
vmCvar_t           g_momentumHalfLife;
vmCvar_t           g_momentumRewardDoubleTime;
vmCvar_t           g_unlockableMinTime;
vmCvar_t           g_momentumBaseMod;
vmCvar_t           g_momentumKillMod;
vmCvar_t           g_momentumBuildMod;
vmCvar_t           g_momentumDeconMod;
vmCvar_t           g_momentumDestroyMod;

vmCvar_t           g_humanAllowBuilding;
vmCvar_t           g_alienAllowBuilding;

vmCvar_t           g_powerCompetitionRange;
vmCvar_t           g_powerBaseSupply;
vmCvar_t           g_powerReactorSupply;
vmCvar_t           g_powerReactorRange;
vmCvar_t           g_powerRepeaterSupply;
vmCvar_t           g_powerRepeaterRange;

vmCvar_t           g_alienOffCreepRegenHalfLife;

vmCvar_t           g_teamImbalanceWarnings;
vmCvar_t           g_freeFundPeriod;

vmCvar_t           g_unlagged;

vmCvar_t           g_disabledEquipment;
vmCvar_t           g_disabledClasses;
vmCvar_t           g_disabledBuildables;
vmCvar_t           g_disabledVoteCalls;

vmCvar_t           g_debugMapRotation;
vmCvar_t           g_currentMapRotation;
vmCvar_t           g_mapRotationNodes;
vmCvar_t           g_mapRotationStack;
vmCvar_t           g_nextMap;
vmCvar_t           g_nextMapLayouts;
vmCvar_t           g_initialMapRotation;
vmCvar_t           g_mapLog;
vmCvar_t           g_mapStartupMessageDelay;

vmCvar_t           g_debugVoices;
vmCvar_t           g_enableVsays;

vmCvar_t           g_shove;
vmCvar_t           g_antiSpawnBlock;

vmCvar_t           g_mapConfigs;
vmCvar_t           g_sayAreaRange;

vmCvar_t           g_floodMaxDemerits;
vmCvar_t           g_floodMinTime;

vmCvar_t           g_defaultLayouts;
vmCvar_t           g_layouts;
vmCvar_t           g_layoutAuto;

vmCvar_t           g_emoticonsAllowedInNames;
vmCvar_t           g_unnamedNumbering;
vmCvar_t           g_unnamedNamePrefix;

vmCvar_t           g_admin;
vmCvar_t           g_adminWarn;
vmCvar_t           g_adminTempBan;
vmCvar_t           g_adminMaxBan;
vmCvar_t           g_adminRetainExpiredBans;

vmCvar_t           g_privateMessages;
vmCvar_t           g_specChat;
vmCvar_t           g_publicAdminMessages;
vmCvar_t           g_allowTeamOverlay;

vmCvar_t           g_tag;

vmCvar_t           g_showKillerHP;
vmCvar_t           g_combatCooldown;

vmCvar_t           g_geoip;

vmCvar_t           g_debugEntities;

vmCvar_t           g_instantBuilding;


// <bot stuff>

// bot buy cvars
vmCvar_t g_bot_buy;
vmCvar_t g_bot_rifle;
vmCvar_t g_bot_painsaw;
vmCvar_t g_bot_shotgun;
vmCvar_t g_bot_lasgun;
vmCvar_t g_bot_mdriver;
vmCvar_t g_bot_chaingun;
vmCvar_t g_bot_prifle;
vmCvar_t g_bot_flamer;
vmCvar_t g_bot_lcannon;

// bot evolution cvars
vmCvar_t g_bot_evolve;
vmCvar_t g_bot_level1;
vmCvar_t g_bot_level2;
vmCvar_t g_bot_level2upg;
vmCvar_t g_bot_level3;
vmCvar_t g_bot_level3upg;
vmCvar_t g_bot_level4;

// misc bot cvars
vmCvar_t g_bot_attackStruct;
vmCvar_t g_bot_roam;
vmCvar_t g_bot_rush;
vmCvar_t g_bot_repair;
vmCvar_t g_bot_build;
vmCvar_t g_bot_retreat;
vmCvar_t g_bot_fov;
vmCvar_t g_bot_chasetime;
vmCvar_t g_bot_reactiontime;
vmCvar_t g_bot_infinite_funds;
vmCvar_t g_bot_numInGroup;
vmCvar_t g_bot_persistent;
vmCvar_t g_bot_debug;
vmCvar_t g_bot_buildLayout;

//</bot stuff>

// copy cvars that can be set in worldspawn so they can be restored later
static char        cv_gravity[ MAX_CVAR_VALUE_STRING ];

static cvarTable_t gameCvarTable[] =
{
	// special purpose (external source, read only, etc.)
	// TODO: Split and comment this section
	{ nullptr,                           "gamename",                      GAME_VERSION,                       CVAR_SERVERINFO | CVAR_ROM,                      0, false , nullptr          },
	{ nullptr,                           "gamedate",                      __DATE__,                           CVAR_ROM,                                        0, false , nullptr          },
	{ nullptr,                           "sv_mapname",                    "",                                 CVAR_SERVERINFO,                                 0, false , nullptr          },
	{ nullptr,                           "P",                             "",                                 CVAR_SERVERINFO,                                 0, false , nullptr          },
	{ nullptr,                           "B",                             "",                                 CVAR_SERVERINFO,                                 0, false , nullptr          },
	{ nullptr,                           "g_mapStartupMessage",           "",                                 0,                                               0, false , nullptr          },
	{ nullptr,                           "g_mapConfigsLoaded",            "0",                                0,                                               0, false , nullptr          },
	{ &g_maxclients,                  "sv_maxclients",                 "24",                               CVAR_SERVERINFO | CVAR_LATCH,                    0, false    , nullptr       },
	{ &g_mapRestarted,                "g_mapRestarted",                "0",                                0,                                               0, false    , nullptr       },
	{ &g_lockTeamsAtStart,            "g_lockTeamsAtStart",            "0",                                0,                                               0, false    , nullptr       },
	{ &g_tag,                         "g_tag",                         "unv",                              CVAR_INIT,                                       0, false    , nullptr       },


	// server: basic
	{ &g_maxGameClients,              "g_maxGameClients",              "0",                                CVAR_SERVERINFO,                                 0, false    , nullptr       },
	{ &g_needpass,                    "g_needpass",                    "0",                                CVAR_SERVERINFO,                                 0, false    , nullptr       },
	{ &g_password,                    "g_password",                    "",                                 CVAR_USERINFO,                                   0, false    , nullptr       },
	{ &g_motd,                        "g_motd",                        "",                                 0,                                               0, false    , nullptr       },
	{ &g_showHelpOnConnection,        "g_showHelpOnConnection",        "1",                                0,                                               0, false    , nullptr       },

	// server: network related
	{ &g_unlagged,                    "g_unlagged",                    "1",                                CVAR_SERVERINFO,                                 0, true     , nullptr       },
	{ &g_smoothClients,               "g_smoothClients",               "1",                                0,                                               0, false    , nullptr       },
	{ &g_synchronousClients,          "g_synchronousClients",          "0",                                0,                                               0, false    , nullptr       },
	{ &pmove_fixed,                   "pmove_fixed",                   "0",                                0,                                               0, false    , nullptr       },
	{ &pmove_msec,                    "pmove_msec",                    "8",                                0,                                               0, false    , nullptr       },
	{ &pmove_accurate,                "pmove_accurate",                "1",                                0,                                               0, false    , nullptr       },
	{ &g_floodMaxDemerits,            "g_floodMaxDemerits",            "5000",                             0,                                               0, false    , nullptr       },
	{ &g_floodMinTime,                "g_floodMinTime",                "2000",                             0,                                               0, false    , nullptr       },

	// clients: limits
	{ &g_minNameChangePeriod,         "g_minNameChangePeriod",         "5",                                0,                                               0, false    , nullptr       },
	{ &g_maxNameChanges,              "g_maxNameChanges",              "5",                                0,                                               0, false    , nullptr       },
	{ &g_enableVsays,                 "g_voiceChats",                  "1",                                0,                                               0, false    , nullptr       },
	{ &g_inactivity,                  "g_inactivity",                  "0",                                0,                                               0, true     , nullptr       },
	{ &g_emoticonsAllowedInNames,     "g_emoticonsAllowedInNames",     "1",                                CVAR_LATCH,                                      0, false    , nullptr       },
	{ &g_privateMessages,             "g_privateMessages",             "1",                                0,                                               0, false    , nullptr       },
	{ &g_specChat,                    "g_specChat",                    "1",                                0,                                               0, false    , nullptr       },

	// clients: voting
	{ &g_allowVote,                   "g_allowVote",                   "1",                                0,                                               0, false    , nullptr       },
	{ &g_voteLimit,                   "g_voteLimit",                   "5",                                0,                                               0, false    , nullptr       },
	{ &g_disabledVoteCalls,           "g_disabledVoteCalls",           "",                                 0,                                               0, false    , nullptr       },
	{ &g_extendVotesPercent,          "g_extendVotesPercent",          "74",                               0,                                               0, false    , nullptr       },
	{ &g_extendVotesTime,             "g_extendVotesTime",             "10",                               0,                                               0, false    , nullptr       },
	{ &g_kickVotesPercent,            "g_kickVotesPercent",            "51",                               0,                                               0, true     , nullptr       },
	{ &g_denyVotesPercent,            "g_denyVotesPercent",            "51",                               0,                                               0, true     , nullptr       },
	{ &g_mapVotesPercent,             "g_mapVotesPercent",             "51",                               0,                                               0, true     , nullptr       },
	{ &g_mapVotesBefore,              "g_mapVotesBefore",              "5",                                0,                                               0, true     , nullptr       },
	{ &g_nextMapVotesPercent,         "g_nextMapVotesPercent",         "51",                               0,                                               0, true     , nullptr       },
	{ &g_drawVotesPercent,            "g_drawVotesPercent",            "51",                               0,                                               0, true     , nullptr       },
	{ &g_drawVotesAfter,              "g_drawVotesAfter",              "0",                                0,                                               0, true     , nullptr       },
	{ &g_drawVoteReasonRequired,      "g_drawVoteReasonRequired",      "0",                                0,                                               0, true     , nullptr       },
	{ &g_admitDefeatVotesPercent,     "g_admitDefeatVotesPercent",     "74",                               0,                                               0, true     , nullptr       },
	{ &g_pollVotesPercent,            "g_pollVotesPercent",            "0",                                0,                                               0, true     , nullptr       },
	{ &g_botKickVotesAllowed,         "g_botKickVotesAllowed",         "1",                                0,                                               0, true     , nullptr       },
	{ &g_botKickVotesAllowedThisMap,  "g_botKickVotesAllowedThisMap",  "1",                                0,                                               0, true     , nullptr       },

	// clients: misc
	{ &g_geoip,                       "g_geoip",                       "1",                                0,                                               0, false    , nullptr       },
	{ &g_unnamedNumbering,            "g_unnamedNumbering",            "-1",                               0,                                               0, false    , nullptr       },
	{ &g_unnamedNamePrefix,           "g_unnamedNamePrefix",           UNNAMED_PLAYER"#",                  0,                                               0, false    , nullptr       },

	// admin system
	{ &g_admin,                       "g_admin",                       "admin.dat",                        0,                                               0, false    , nullptr       },
	{ &g_adminWarn,                   "g_adminWarn",                   "1h",                               0,                                               0, false    , nullptr       },
	{ &g_adminTempBan,                "g_adminTempBan",                "2m",                               0,                                               0, false    , nullptr       },
	{ &g_adminMaxBan,                 "g_adminMaxBan",                 "2w",                               0,                                               0, false    , nullptr       },
	{ &g_adminRetainExpiredBans,      "g_adminRetainExpiredBans",      "1",                                0,                                               0, false    , nullptr       },
	{ &g_publicAdminMessages,         "g_publicAdminMessages",         "1",                                0,                                               0, false    , nullptr       },

	// logging
	{ &g_logFile,                     "g_logFile",                     "games.log",                        0,                                               0, false    , nullptr       },
	{ &g_logGameplayStatsFrequency,   "g_logGameplayStatsFrequency",   "10",                               0,                                               0, false    , nullptr       },
	{ &g_logFileSync,                 "g_logFileSync",                 "0",                                0,                                               0, false    , nullptr       },

	// maps, layouts & rotation
	{ &g_currentMapRotation,          "g_currentMapRotation",          "-1",                               0,                                               0, false    , nullptr       },
	{ &g_mapRotationNodes,            "g_mapRotationNodes",            "",                                 0,                                               0, false    , nullptr       },
	{ &g_mapRotationStack,            "g_mapRotationStack",            "",                                 0,                                               0, false    , nullptr       },
	{ &g_nextMap,                     "g_nextMap",                     "",                                 0,                                               0, true     , nullptr       },
	{ &g_nextMapLayouts,              "g_nextMapLayouts",              "",                                 0,                                               0, true     , nullptr       },
	{ &g_initialMapRotation,          "g_initialMapRotation",          "rotation1",                        0,                                               0, false    , nullptr       },
	{ &g_mapLog,                      "g_mapLog",                      "",                                 0,                                               0, false    , nullptr       },
	{ &g_mapStartupMessageDelay,      "g_mapStartupMessageDelay",      "5000",                             CVAR_LATCH,                                      0, false    , nullptr       },
	{ &g_mapConfigs,                  "g_mapConfigs",                  "",                                 0,                                               0, false    , nullptr       },
	{ &g_defaultLayouts,              "g_defaultLayouts",              "",                                 CVAR_LATCH,                                      0, false    , nullptr       },
	{ &g_layouts,                     "g_layouts",                     "",                                 CVAR_LATCH,                                      0, false    , nullptr       },
	{ &g_layoutAuto,                  "g_layoutAuto",                  "0",                                0,                                               0, false    , nullptr       },

	// debug switches
	{ &g_debugMove,                   "g_debugMove",                   "0",                                0,                                               0, false    , nullptr       },
	{ &g_debugDamage,                 "g_debugDamage",                 "0",                                0,                                               0, false    , nullptr       },
	{ &g_debugKnockback,              "g_debugKnockback",              "0",                                0,                                               0, false    , nullptr       },
	{ &g_debugTurrets,                "g_debugTurrets",                "0",                                0,                                               0, false    , nullptr       },
	{ &g_debugMomentum,               "g_debugMomentum",               "0",                                0,                                               0, false    , nullptr       },
	{ &g_debugMapRotation,            "g_debugMapRotation",            "0",                                0,                                               0, false    , nullptr       },
	{ &g_debugVoices,                 "g_debugVoices",                 "0",                                0,                                               0, false    , nullptr       },
	{ &g_debugEntities,               "g_debugEntities",               "0",                                0,                                               0, false    , nullptr       },
	{ &g_debugFire,                   "g_debugFire",                   "0",                                0,                                               0, false    , nullptr       },

	// gameplay: basic
	{ &g_timelimit,                   "timelimit",                     "45",                               CVAR_SERVERINFO,                                 0, true     , nullptr       },
	{ &g_friendlyFire,                "g_friendlyFire",                "1",                                CVAR_SERVERINFO,                                 0, true     , nullptr       },
	{ &g_friendlyBuildableFire,       "g_friendlyBuildableFire",       "1",                                CVAR_SERVERINFO,                                 0, true     , nullptr       },

	// gameplay: team balance
	{ &g_teamForceBalance,            "g_teamForceBalance",            "0",                                0,                                               0, true     , nullptr       },
	{ &g_teamImbalanceWarnings,       "g_teamImbalanceWarnings",       "30",                               0,                                               0, false    , nullptr       },
	{ &g_warmup,                      "g_warmup",                      "10",                               0,                                               0, true     , nullptr       },
	{ &g_doWarmup,                    "g_doWarmup",                    "0",                                0,                                               0, true     , nullptr       },

	// gameplay: mining
	{ &g_initialBuildPoints,          "g_initialBuildPoints",          DEFAULT_INITIAL_BUILD_POINTS,       0,                                               0, false    , nullptr       },
	{ &g_initialMineRate,             "g_initialMineRate",             DEFAULT_INITIAL_MINE_RATE,          0,                                               0, false    , nullptr       },
	{ &g_mineRateHalfLife,            "g_mineRateHalfLife",            DEFAULT_MINE_RATE_HALF_LIFE,        0,                                               0, false    , nullptr       },
	{ &g_minimumMineRate,             "g_minimumMineRate",             DEFAULT_MINIMUM_MINE_RATE,          0,                                               0, false    , nullptr       },
	{ &g_buildPointLossFraction,      "g_buildPointLossFraction",      DEFAULT_BP_LOSS_FRAC,               0,                                               0, false    , nullptr       },

	// gameplay: momentum
	{ &g_unlockableMinTime,           "g_unlockableMinTime",           DEFAULT_UNLOCKABLE_MIN_TIME,        CVAR_SERVERINFO,                                 0, false    , nullptr       },
	{ &g_momentumHalfLife,            "g_momentumHalfLife",            DEFAULT_MOMENTUM_HALF_LIFE,         CVAR_SERVERINFO,                                 0, false    , nullptr       },
	{ &g_momentumRewardDoubleTime,    "g_momentumRewardDoubleTime",    DEFAULT_CONF_REWARD_DOUBLE_TIME,    0,                                               0, false    , nullptr       },
	{ &g_momentumBaseMod,             "g_momentumBaseMod",             DEFAULT_MOMENTUM_BASE_MOD,          0,                                               0, false    , nullptr       },
	{ &g_momentumKillMod,             "g_momentumKillMod",             DEFAULT_MOMENTUM_KILL_MOD,          0,                                               0, false    , nullptr       },
	{ &g_momentumBuildMod,            "g_momentumBuildMod",            DEFAULT_MOMENTUM_BUILD_MOD,         0,                                               0, false    , nullptr       },
	{ &g_momentumDeconMod,            "g_momentumDeconMod",            DEFAULT_MOMENTUM_DECON_MOD,         0,                                               0, false    , nullptr       },
	{ &g_momentumDestroyMod,          "g_momentumDestroyMod",          DEFAULT_MOMENTUM_DESTROY_MOD,       0,                                               0, false    , nullptr       },

	// gameplay: buildable power
	{ &g_powerCompetitionRange,       "g_powerCompetitionRange",       "320",                              0,                                               0, false    , nullptr       },
	{ &g_powerBaseSupply,             "g_powerBaseSupply",             "20",                               0,                                               0, false    , nullptr       },
	{ &g_powerReactorSupply,          "g_powerReactorSupply",          "30",                               0,                                               0, false    , nullptr       },
	{ &g_powerReactorRange,           "g_powerReactorRange",           "1000",                             CVAR_SERVERINFO,                                 0, false    , nullptr       },
	{ &g_powerRepeaterSupply,         "g_powerRepeaterSupply",         "20",                               0,                                               0, false    , nullptr       },
	{ &g_powerRepeaterRange,          "g_powerRepeaterRange",          "500",                              CVAR_SERVERINFO,                                 0, false    , nullptr       },

	// gameplay: limits
	{ &g_humanAllowBuilding,          "g_humanAllowBuilding",          "1",                                0,                                               0, false    , nullptr       },
	{ &g_alienAllowBuilding,          "g_alienAllowBuilding",          "1",                                0,                                               0, false    , nullptr       },
	{ &g_disabledEquipment,           "g_disabledEquipment",           "",                                 CVAR_SYSTEMINFO,                                 0, false    , nullptr       },
	{ &g_disabledClasses,             "g_disabledClasses",             "",                                 CVAR_SYSTEMINFO,                                 0, false    , nullptr       },
	{ &g_disabledBuildables,          "g_disabledBuildables",          "",                                 CVAR_SYSTEMINFO,                                 0, false    , nullptr       },

	// gameplay: misc
	{ &g_alienOffCreepRegenHalfLife,  "g_alienOffCreepRegenHalfLife",  "0",                                0,                                               0, false    , nullptr       },
	{ &g_freeFundPeriod,              "g_freeFundPeriod",              DEFAULT_FREEKILL_PERIOD,            0,                                               0, true     , nullptr       },
	{ &g_sayAreaRange,                "g_sayAreaRange",                "1000",                             0,                                               0, true     , nullptr       },
	{ &g_speed,                       "g_speed",                       "320",                              0,                                               0, true     , nullptr       },
	{ &g_gravity,                     "g_gravity",                     "800",                              0,                                               0, true, cv_gravity},
	{ &g_antiSpawnBlock,              "g_antiSpawnBlock",              "0",                                0,                                               0, false    , nullptr       },
	{ &g_shove,                       "g_shove",                       "0.0",                              0,                                               0, false    , nullptr       },
	{ &g_dretchPunt,                  "g_dretchPunt",                  "1",                                0,                                               0, true     , nullptr       },
	{ &g_allowTeamOverlay,            "g_allowTeamOverlay",            "1",                                0,                                               0, true     , nullptr       },
	{ &g_showKillerHP,                "g_showKillerHP",                "0",                                0,                                               0, false    , nullptr       },
	{ &g_combatCooldown,              "g_combatCooldown",              "15",                               0,                                               0, false    , nullptr       },

	{ &g_instantBuilding,             "g_instantBuilding",             "0",                                0,                                               0, true     , nullptr       },

	// bots: buying
	{ &g_bot_buy, "g_bot_buy", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_rifle, "g_bot_rifle", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_painsaw, "g_bot_painsaw", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_shotgun, "g_bot_shotgun", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_lasgun, "g_bot_lasgun", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_mdriver, "g_bot_mdriver", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_chaingun, "g_bot_chain", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_prifle, "g_bot_prifle", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_flamer, "g_bot_flamer", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_lcannon, "g_bot_lcannon", "1",  CVAR_NORESTART, 0, false, nullptr },

	// bots: evolution
	{ &g_bot_evolve, "g_bot_evolve", "1", CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_level1, "g_bot_level1", "1", CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_level2, "g_bot_level2", "1", CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_level2upg, "g_bot_level2upg", "1", CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_level3, "g_bot_level3", "1", CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_level3upg, "g_bot_level3upg", "1", CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_level4, "g_bot_level4", "1", CVAR_NORESTART, 0, false, nullptr },

	// bots: misc
	{ &g_bot_attackStruct, "g_bot_attackStruct", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_roam, "g_bot_roam", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_rush, "g_bot_rush", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_repair, "g_bot_repair", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_build, "g_bot_build", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_retreat, "g_bot_retreat", "1",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_fov, "g_bot_fov", "125",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_chasetime, "g_bot_chasetime", "5000",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_reactiontime, "g_bot_reactiontime", "500",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_infinite_funds, "g_bot_infinite_funds", "0",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_numInGroup, "g_bot_numInGroup", "3",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_debug, "g_bot_debug", "0",  CVAR_NORESTART, 0, false, nullptr },
	{ &g_bot_buildLayout, "g_bot_buildLayout", "botbuild",  CVAR_NORESTART, 0, false, nullptr }
};

static const size_t gameCvarTableSize = ARRAY_LEN( gameCvarTable );

void               CheckExitRules();
void               G_CountSpawns();
static void        G_LogGameplayStats( int state );

// state field of G_LogGameplayStats
enum
{
	LOG_GAMEPLAY_STATS_HEADER,
	LOG_GAMEPLAY_STATS_BODY,
	LOG_GAMEPLAY_STATS_FOOTER
};

void QDECL PRINTF_LIKE(1) G_Printf( const char *fmt, ... )
{
	va_list argptr;
	char    text[ 1024 ];

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	trap_Print( text );
}

void QDECL PRINTF_LIKE(1) NORETURN G_Error( const char *fmt, ... )
{
	va_list argptr;
	char    text[ 1024 ];

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	trap_Error( text );
}

/*
================
G_FindEntityGroups

Chain together all entities with a matching groupName field.
Entity groups are used for item groups and multi-entity mover groups.

All but the first will have the FL_GROUPSLAVE flag set and groupMaster field set
All but the last will have the groupChain field set to the next one
================
*/
void G_FindEntityGroups()
{
	gentity_t *masterEntity, *comparedEntity;
	int       i, j, k;
	int       groupCount, entityCount;

	groupCount = 0;
	entityCount = 0;

	for ( i = MAX_CLIENTS, masterEntity = g_entities + i; i < level.num_entities; i++, masterEntity++ )
	{
		if ( !masterEntity->groupName )
		{
			continue;
		}

		if ( masterEntity->flags & FL_GROUPSLAVE )
		{
			continue;
		}

		masterEntity->groupMaster = masterEntity;
		groupCount++;
		entityCount++;

		for ( j = i + 1, comparedEntity = masterEntity + 1; j < level.num_entities; j++, comparedEntity++ )
		{
			if ( !comparedEntity->groupName )
			{
				continue;
			}

			if ( comparedEntity->flags & FL_GROUPSLAVE )
			{
				continue;
			}

			if ( !strcmp( masterEntity->groupName, comparedEntity->groupName ) )
			{
				entityCount++;
				comparedEntity->groupChain = masterEntity->groupChain;
				masterEntity->groupChain = comparedEntity;
				comparedEntity->groupMaster = masterEntity;
				comparedEntity->flags |= FL_GROUPSLAVE;

				// make sure that targets only point at the master
				for (k = 0; comparedEntity->names[k]; k++)
				{
					masterEntity->names[k] = comparedEntity->names[k];
					comparedEntity->names[k] = nullptr;
				}
			}
		}
	}

	G_Printf( "%i groups with %i entities\n", groupCount, entityCount );
}
/*
================
G_InitSetEntities
goes through all entities and concludes the spawn
by calling their reset function as initiation if available
================
*/
void G_InitSetEntities()
{
	int i;
	gentity_t *entity;

	for ( i = MAX_CLIENTS, entity = g_entities + i; i < level.num_entities; i++, entity++ )
	{
		if(entity->inuse && entity->reset)
			entity->reset( entity );
	}
}

static int cvarCompare( const void *a, const void *b )
{
	cvarTable_t *ac = ( cvarTable_t * ) a;
	cvarTable_t *bc = ( cvarTable_t * ) b;
	return Q_stricmp( ac->cvarName, bc->cvarName );
}

/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars()
{
	unsigned i;
	cvarTable_t *cvarTable;

	// sort the table for fast lookup
	qsort( gameCvarTable, gameCvarTableSize, sizeof( *gameCvarTable ), cvarCompare );

	for ( i = 0, cvarTable = gameCvarTable; i < gameCvarTableSize; i++, cvarTable++ )
	{
		trap_Cvar_Register( cvarTable->vmCvar, cvarTable->cvarName,
		                    cvarTable->defaultString, cvarTable->cvarFlags );

		if ( cvarTable->vmCvar )
		{
			cvarTable->modificationCount = cvarTable->vmCvar->modificationCount;

			if ( cvarTable->explicit_ )
			{
				strcpy( cvarTable->explicit_, cvarTable->vmCvar->string );
			}
		}
	}
}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars()
{
	unsigned i;
	cvarTable_t *cv;

	for ( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ )
	{
		if ( cv->vmCvar )
		{
			trap_Cvar_Update( cv->vmCvar );

			if ( cv->modificationCount != cv->vmCvar->modificationCount )
			{
				cv->modificationCount = cv->vmCvar->modificationCount;

				if ( cv->trackChange )
				{
					trap_SendServerCommand( -1, va( "print_tr %s %s %s", QQ( N_("Server: $1$ changed to $2$\n") ),
					                                Quote( cv->cvarName ), Quote( cv->vmCvar->string ) ) );
				}

				if ( !level.spawning && cv->explicit_ )
				{
					strcpy( cv->explicit_, cv->vmCvar->string );
				}
			}
		}
	}
}

/*
=================
G_RestoreCvars
=================
*/
void G_RestoreCvars()
{
	unsigned i;
	cvarTable_t *cv;

	for ( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ )
	{
		if ( cv->vmCvar && cv->explicit_ )
		{
			trap_Cvar_Set( cv->cvarName, cv->explicit_ );
		}
	}
}

vmCvar_t *G_FindCvar( const char *name )
{
	cvarTable_t *c = nullptr;
	cvarTable_t comp;
	comp.cvarName = name;
	c = ( cvarTable_t * ) bsearch( &comp, gameCvarTable, gameCvarTableSize, sizeof( *gameCvarTable ), cvarCompare );

	if ( !c )
	{
		return nullptr;
	}

	return c->vmCvar;
}

/*
=================
G_MapConfigs
=================
*/
void G_MapConfigs( const char *mapname )
{
	if ( !g_mapConfigs.string[ 0 ] )
	{
		return;
	}

	if ( trap_Cvar_VariableIntegerValue( "g_mapConfigsLoaded" ) )
	{
		return;
	}

	trap_SendConsoleCommand( va( "exec %s/default.cfg\n", Quote( g_mapConfigs.string ) ) );

	trap_SendConsoleCommand( va( "exec %s/%s.cfg\n", Quote( g_mapConfigs.string ), Quote( mapname ) ) );

	trap_Cvar_Set( "g_mapConfigsLoaded", "1" );
	trap_SendConsoleCommand( "maprestarted\n" );
}

/*
============
G_InitGame

============
*/
void G_InitGame( int levelTime, int randomSeed, bool inClient )
{
	int i;

	srand( randomSeed );

	G_RegisterCvars();

	G_Printf( "------- Game Initialization -------\n" );
	G_Printf( "gamename: %s\n", GAME_VERSION );
	G_Printf( "gamedate: %s\n", __DATE__ );

	// set some level globals
	memset( &level, 0, sizeof( level ) );
	level.time = levelTime;
	level.inClient = inClient;
	level.startTime = levelTime;
	level.snd_fry = G_SoundIndex( "sound/misc/fry.wav" );  // FIXME standing in lava / slime

	// TODO: Move this in a seperate function
	if ( g_logFile.string[ 0 ] )
	{
		if ( g_logFileSync.integer )
		{
			trap_FS_FOpenFile( g_logFile.string, &level.logFile, FS_APPEND_SYNC );
		}
		else
		{
			trap_FS_FOpenFile( g_logFile.string, &level.logFile, FS_APPEND );
		}

		if ( !level.logFile )
		{
			G_Printf( "WARNING: Couldn't open logfile: %s\n", g_logFile.string );
		}
		else
		{
			char    serverinfo[ MAX_INFO_STRING ];
			qtime_t qt;

			trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

			G_LogPrintf( "------------------------------------------------------------\n" );
			G_LogPrintf( "InitGame: %s\n", serverinfo );

			Com_GMTime( &qt );
			G_LogPrintf( "RealTime: %04i-%02i-%02i %02i:%02i:%02i Z\n",
			             1900 + qt.tm_year, qt.tm_mon + 1, qt.tm_mday,
			             qt.tm_hour, qt.tm_min, qt.tm_sec );
		}
	}
	else
	{
		G_Printf( "Not logging to disk\n" );
	}

	// gameplay statistics logging
	// TODO: Move this in a seperate function
	if ( g_logGameplayStatsFrequency.integer > 0 )
	{
		char    logfile[ 128 ], mapname[ 64 ];
		qtime_t qt;

		Com_GMTime( &qt );
		trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );

		Com_sprintf( logfile, sizeof( logfile ),
		             "stats/gameplay/%04i%02i%02i_%02i%02i%02i_%s.log",
		             1900 + qt.tm_year, qt.tm_mon + 1, qt.tm_mday,
		             qt.tm_hour, qt.tm_min, qt.tm_sec,
		             mapname );

		trap_FS_FOpenFile( logfile, &level.logGameplayFile, FS_WRITE );

		if ( !level.logGameplayFile )
		{
			G_Printf( "WARNING: Couldn't open gameplay statistics logfile: %s\n", logfile );
		}
		else
		{
			G_LogGameplayStats( LOG_GAMEPLAY_STATS_HEADER );
		}
	}

	// initialise whether bot vote kicks are allowed. the map rotation may clear this flag.
	trap_Cvar_Set( "g_botKickVotesAllowedThisMap", g_botKickVotesAllowed.integer ? "1" : "0" );

	// clear these now; they'll be set, if needed, from rotation
	trap_Cvar_Set( "g_mapStartupMessage", "" );
	trap_Cvar_Set( "g_disabledVoteCalls", "" );

	// retrieve map name and load per-map configuration
	{
		char map[ MAX_CVAR_VALUE_STRING ] = { "" };

		trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
		G_MapConfigs( map );
	}

	// load config files
	BG_InitAllConfigs();

	// we're done with g_mapConfigs, so reset this for the next map
	trap_Cvar_Set( "g_mapConfigsLoaded", "0" );

	G_RegisterCommands();
	G_admin_readconfig( nullptr );

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof( g_entities[ 0 ] ) );
	level.gentities = g_entities;

	// initilize special entities so they don't need to be special cased in the CBSE code later on
	G_InitGentityMinimal( g_entities + ENTITYNUM_NONE );
	G_InitGentityMinimal( g_entities + ENTITYNUM_WORLD );

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset( g_clients, 0, MAX_CLIENTS * sizeof( g_clients[ 0 ] ) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( i = 0; i < level.maxclients; i++ )
	{
		g_entities[ i ].client = level.clients + i;
	}

	// always leave room for the max number of clients, even if they aren't all used, so numbers
	// inside that range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		g_entities[ i ].classname = "clientslot";
	}

	// let the server system know where the entites are
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ),
	                     &level.clients[ 0 ].ps, sizeof( level.clients[ 0 ] ) );

	level.emoticonCount = BG_LoadEmoticons( level.emoticons, MAX_EMOTICONS );

	trap_SetConfigstring( CS_INTERMISSION, "0" );

	// test to see if a custom buildable layout will be loaded
	G_LayoutSelect();

	// this has to be flipped after the first UpdateCvars
	level.spawning = true;

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// add any fake entities
	G_SpawnFakeEntities();

	BaseClustering::Init();

	// load up a custom building layout if there is one
	G_LayoutLoad();

	// setup bot code
	G_BotInit();

	// the map might disable some things
	BG_InitAllowedGameElements();

	// Initialize item locking state
	BG_InitUnlockackables();

	G_FindEntityGroups();
	G_InitSetEntities();

	G_CheckPmoveParamChanges();

	G_InitDamageLocations();

	G_InitMapRotations();

	G_InitSpawnQueue( &level.team[ TEAM_ALIENS ].spawnQueue );
	G_InitSpawnQueue( &level.team[ TEAM_HUMANS ].spawnQueue );

	if ( g_debugMapRotation.integer )
	{
		G_PrintRotations();
	}

	level.voices = BG_VoiceInit();
	BG_PrintVoices( level.voices, g_debugVoices.integer );

	// Give both teams some build points to start out with.
	for (team_t team = TEAM_NONE; (team = G_IterateTeams(team)); ) {
		float startBP = (float)std::max(0, g_initialBuildPoints.integer - level.team[team].layoutBuildPoints);

		G_ModifyBuildPoints(team, startBP);
	}

	G_Printf( "-----------------------------------\n" );

	// So the server counts the spawns without a client attached
	G_CountSpawns();

	G_UpdateTeamConfigStrings();

	G_MapLog_NewMap();

	if ( g_lockTeamsAtStart.integer )
	{
		level.team[ TEAM_ALIENS ].locked = true;
		level.team[ TEAM_HUMANS ].locked = true;
		trap_Cvar_Set( "g_lockTeamsAtStart", "0" );
	}

	G_notify_sensor_start();
}

/*
==================
G_ClearVotes

remove all currently active votes
==================
*/
static void G_ClearVotes( bool all )
{
	int i;

	for ( i = 0; i < NUM_TEAMS; i++ )
	{
		if ( all || G_CheckStopVote( (team_t) i ) )
		{
			level.team[ i ].voteTime = 0;
			trap_SetConfigstring( CS_VOTE_TIME + i, "" );
			trap_SetConfigstring( CS_VOTE_STRING + i, "" );
		}
	}
}

/*
==================
G_VotesRunning

Check if there are any votes currently running
==================
*/
static bool G_VotesRunning()
{
	int i;

	for ( i = 0; i < NUM_TEAMS; i++ )
	{
		if ( level.team[ i ].voteTime )
		{
			return true;
		}
	}

	return false;
}

/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart )
{
	// in case of a map_restart
	G_ClearVotes( true );

	G_RestoreCvars();

	G_Printf( "==== ShutdownGame ====\n" );

	if ( level.logFile )
	{
		G_LogPrintf( "ShutdownGame:\n" );
		G_LogPrintf( "------------------------------------------------------------\n" );
		trap_FS_FCloseFile( level.logFile );
		level.logFile = 0;
	}

	// finalize logging of gameplay statistics
	if ( level.logGameplayFile )
	{
		G_LogGameplayStats( LOG_GAMEPLAY_STATS_FOOTER );
		trap_FS_FCloseFile( level.logGameplayFile );
		level.logGameplayFile = 0;
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();

	G_admin_cleanup();
	G_BotCleanup( restart );
	G_namelog_cleanup();

	G_UnregisterCommands();

	G_ShutdownMapRotations();
	BG_UnloadAllConfigs();

	level.restarted = false;
	level.surrenderTeam = TEAM_NONE;
	trap_SetConfigstring( CS_WINNER, "" );
}

//===================================================================

void QDECL PRINTF_LIKE(2) NORETURN Com_Error( int, const char *error, ... )
{
	va_list argptr;
	char    text[ 1024 ];

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

void G_CheckPmoveParamChanges() {
	if ( pmove_msec.integer < 8 )
	{
		trap_Cvar_Set( "pmove_msec", "8" );
	}
	else if ( pmove_msec.integer > 33 )
	{
		trap_Cvar_Set( "pmove_msec", "33" );
	}

	if(not level.pmoveParams.initialized or
			level.pmoveParams.synchronous != g_synchronousClients.integer or
			level.pmoveParams.msec != pmove_msec.integer or
			level.pmoveParams.fixed != pmove_fixed.integer or
			level.pmoveParams.accurate != pmove_accurate.integer) {
		level.pmoveParams.initialized = true;
		level.pmoveParams.synchronous = g_synchronousClients.integer;
		level.pmoveParams.msec = pmove_msec.integer;
		level.pmoveParams.fixed = pmove_fixed.integer;
		level.pmoveParams.accurate = pmove_accurate.integer;
		G_SendClientPmoveParams(-1);
	}
}
void G_SendClientPmoveParams(int client) {
	trap_SendServerCommand(client, va("pmove_params %i %i %i %i",
		level.pmoveParams.synchronous,
		level.pmoveParams.fixed,
		level.pmoveParams.msec,
		level.pmoveParams.accurate));
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void *a, const void *b )
{
	gclient_t *ca, *cb;

	ca = &level.clients[ * ( int * ) a ];
	cb = &level.clients[ * ( int * ) b ];

	// then sort by score
	if ( ca->ps.persistant[ PERS_SCORE ] > cb->ps.persistant[ PERS_SCORE ] )
	{
		return -1;
	}

	if ( ca->ps.persistant[ PERS_SCORE ] < cb->ps.persistant[ PERS_SCORE ] )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/*
============
G_InitSpawnQueue

Initialise a spawn queue
============
*/
void G_InitSpawnQueue( spawnQueue_t *sq )
{
	int i;

	sq->back = sq->front = 0;
	sq->back = QUEUE_MINUS1( sq->back );

	//0 is a valid clientNum, so use something else
	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		sq->clients[ i ] = -1;
	}
}

/*
============
G_GetSpawnQueueLength

Return the length of a spawn queue
============
*/
int G_GetSpawnQueueLength( spawnQueue_t *sq )
{
	int length = sq->back - sq->front + 1;

	while ( length < 0 )
	{
		length += MAX_CLIENTS;
	}

	while ( length >= MAX_CLIENTS )
	{
		length -= MAX_CLIENTS;
	}

	return length;
}

/*
============
G_PopSpawnQueue

Remove from front element from a spawn queue
============
*/
int G_PopSpawnQueue( spawnQueue_t *sq )
{
	int clientNum = sq->clients[ sq->front ];

	if ( G_GetSpawnQueueLength( sq ) > 0 )
	{
		sq->clients[ sq->front ] = -1;
		sq->front = QUEUE_PLUS1( sq->front );
		G_StopFollowing( g_entities + clientNum );
		g_entities[ clientNum ].client->ps.pm_flags &= ~PMF_QUEUED;

		return clientNum;
	}
	else
	{
		return -1;
	}
}

/*
============
G_PeekSpawnQueue

Look at front element from a spawn queue
============
*/
int G_PeekSpawnQueue( spawnQueue_t *sq )
{
	return sq->clients[ sq->front ];
}

/*
============
G_SearchSpawnQueue

Look to see if clientNum is already in the spawnQueue
============
*/
bool G_SearchSpawnQueue( spawnQueue_t *sq, int clientNum )
{
	int i;

	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( sq->clients[ i ] == clientNum )
		{
			return true;
		}
	}

	return false;
}

/*
============
G_PushSpawnQueue

Add an element to the back of the spawn queue
============
*/
bool G_PushSpawnQueue( spawnQueue_t *sq, int clientNum )
{
	// don't add the same client more than once
	if ( G_SearchSpawnQueue( sq, clientNum ) )
	{
		return false;
	}

	sq->back = QUEUE_PLUS1( sq->back );
	sq->clients[ sq->back ] = clientNum;

	g_entities[ clientNum ].client->ps.pm_flags |= PMF_QUEUED;
	return true;
}

/*
============
G_RemoveFromSpawnQueue

remove a specific client from a spawn queue
============
*/
bool G_RemoveFromSpawnQueue( spawnQueue_t *sq, int clientNum )
{
	int i = sq->front;

	if ( G_GetSpawnQueueLength( sq ) )
	{
		do
		{
			if ( sq->clients[ i ] == clientNum )
			{
				//and this kids is why it would have
				//been better to use an LL for internal
				//representation
				do
				{
					sq->clients[ i ] = sq->clients[ QUEUE_PLUS1( i ) ];

					i = QUEUE_PLUS1( i );
				}
				while ( i != QUEUE_PLUS1( sq->back ) );

				sq->back = QUEUE_MINUS1( sq->back );
				g_entities[ clientNum ].client->ps.pm_flags &= ~PMF_QUEUED;

				return true;
			}

			i = QUEUE_PLUS1( i );
		}
		while ( i != QUEUE_PLUS1( sq->back ) );
	}

	return false;
}

/*
============
G_GetPosInSpawnQueue

Get the position of a client in a spawn queue
============
*/
int G_GetPosInSpawnQueue( spawnQueue_t *sq, int clientNum )
{
	int i = sq->front;

	if ( G_GetSpawnQueueLength( sq ) )
	{
		do
		{
			if ( sq->clients[ i ] == clientNum )
			{
				if ( i < sq->front )
				{
					return i + MAX_CLIENTS - sq->front + 1;
				}
				else
				{
					return i - sq->front + 1;
				}
			}

			i = QUEUE_PLUS1( i );
		}
		while ( i != QUEUE_PLUS1( sq->back ) );
	}

	return 0;
}

/*
============
G_PrintSpawnQueue

Print the contents of a spawn queue
============
*/
void G_PrintSpawnQueue( spawnQueue_t *sq )
{
	int i = sq->front;
	int length = G_GetSpawnQueueLength( sq );

	G_Printf( "l:%d f:%d b:%d    :", length, sq->front, sq->back );

	if ( length > 0 )
	{
		do
		{
			if ( sq->clients[ i ] == -1 )
			{
				G_Printf( "*:" );
			}
			else
			{
				G_Printf( "%d:", sq->clients[ i ] );
			}

			i = QUEUE_PLUS1( i );
		}
		while ( i != QUEUE_PLUS1( sq->back ) );
	}

	G_Printf( "\n" );
}

/*
============
G_SpawnClients

Spawn queued clients
============
*/
void G_SpawnClients( team_t team )
{
	int          clientNum;
	gentity_t    *ent, *spawn;
	vec3_t       spawn_origin, spawn_angles;
	spawnQueue_t *sq = nullptr;
	int          numSpawns = 0;

	assert(team == TEAM_ALIENS || team == TEAM_HUMANS);
	sq = &level.team[ team ].spawnQueue;

	numSpawns = level.team[ team ].numSpawns;

	if ( G_GetSpawnQueueLength( sq ) > 0 && numSpawns > 0 )
	{
		clientNum = G_PeekSpawnQueue( sq );
		ent = &g_entities[ clientNum ];

		if ( ( spawn = G_SelectUnvanquishedSpawnPoint( team,
		               ent->client->pers.lastDeathLocation,
		               spawn_origin, spawn_angles ) ) )
		{
			clientNum = G_PopSpawnQueue( sq );

			if ( clientNum < 0 )
			{
				return;
			}

			ent = &g_entities[ clientNum ];

			ent->client->sess.spectatorState = SPECTATOR_NOT;
			ClientUserinfoChanged( clientNum, false );
			ClientSpawn( ent, spawn, spawn_origin, spawn_angles );
		}
	}
}

/*
============
G_CountSpawns

Counts the number of spawns for each team
============
*/
void G_CountSpawns()
{
	int       i;
	gentity_t *ent;

	//I guess this could be changed into one function call per team
	level.team[ TEAM_ALIENS ].numSpawns = 0;
	level.team[ TEAM_HUMANS ].numSpawns = 0;

	for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( !ent->inuse || ent->s.eType != ET_BUILDABLE || G_Dead( ent ) )
		{
			continue;
			// is it really useful? Seriously?
		}

		//TODO create a function to check if a building is a spawn
		if( ent->s.modelindex == BA_A_SPAWN )
		{
			level.team[ TEAM_ALIENS ].numSpawns++;
		}
		else if ( ent->s.modelindex == BA_H_SPAWN )
		{
			level.team[ TEAM_HUMANS ].numSpawns++;
		}
	}
}

/*
============
G_CalculateAvgPlayers

Calculates the average number of players on each team.
Resets completely if all players leave a team.
============
*/
void G_CalculateAvgPlayers()
{
	int        team;
	int        *samples, currentPlayers, currentBots;
	float      *avgClients, *avgPlayers, *avgBots;

	static int nextCalculation = 0;

	if ( level.time < nextCalculation )
	{
		return;
	}

	for ( team = TEAM_NONE + 1; team < NUM_TEAMS; team++ )
	{
		samples        = &level.team[ team ].numSamples;
		currentPlayers =  level.team[ team ].numPlayers;
		currentBots    =  level.team[ team ].numBots;
		avgClients     = &level.team[ team ].averageNumClients;
		avgPlayers     = &level.team[ team ].averageNumPlayers;
		avgBots        = &level.team[ team ].averageNumBots;

		if ( *samples == 0 )
		{
			*avgPlayers = ( float )currentPlayers;
			*avgBots    = ( float )currentBots;
		}
		else
		{
			*avgPlayers = ( ( *avgPlayers * *samples ) + currentPlayers ) / ( *samples + 1 );
			*avgBots    = ( ( *avgBots    * *samples ) + currentBots    ) / ( *samples + 1 );
		}

		*avgClients = *avgPlayers + *avgBots;

		(*samples)++;
	}

	nextCalculation = level.time + 1000;
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks()
{
	int  clientNum;
	int  team;
	char P[ MAX_CLIENTS + 1 ] = "", B[ MAX_CLIENTS + 1 ] = "";

	level.numConnectedClients = 0;
	level.numPlayingClients   = 0;
	level.numPlayingPlayers   = 0;
	level.numPlayingBots      = 0;
	level.numAliveClients     = 0;

	for ( team = TEAM_NONE; team < NUM_TEAMS; team++ )
	{
		level.team[ team ].numClients      = 0;
		level.team[ team ].numPlayers      = 0;
		level.team[ team ].numBots         = 0;
		level.team[ team ].numAliveClients = 0;
	}

	for ( clientNum = 0; clientNum < level.maxclients; clientNum++ )
	{
		P[ clientNum ] = '-';
		B[ clientNum ] = '-';

		if ( level.clients[ clientNum ].pers.connected != CON_DISCONNECTED )
		{
			bool bot = false;
			int      entNum;

			for ( entNum = 0; entNum < level.num_entities; ++entNum)
			{
				if ( level.gentities[ clientNum ].client == &level.clients[ clientNum ] )
				{
					bot = ( level.gentities[ clientNum ].r.svFlags & SVF_BOT );
					break;
				}
			}

			level.sortedClients[ level.numConnectedClients ] = clientNum;

			team = level.clients[ clientNum ].pers.team;
			P[ clientNum ] = ( char ) '0' + team;

			level.numConnectedClients++;
			level.team[ team ].numClients++;

			if ( bot )
			{
				B[ clientNum ] = 'b';
				level.team[ team ].numBots++;
			}
			else
			{
				level.team[ team ].numPlayers++;
			}

			if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED )
			{
				continue;
			}

			// clients on a team are "playing"
			if ( team != TEAM_NONE )
			{
				level.numPlayingClients++;

				if ( bot )
				{
					level.numPlayingBots++;
				}
				else
				{
					level.numPlayingPlayers++;
				}

				// clients on a team that don't spectate are "alive"
				if ( level.clients[ clientNum ].sess.spectatorState == SPECTATOR_NOT )
				{
					level.numAliveClients++;
					level.team[ team ].numAliveClients++;
				}
			}
		}
	}

	// voting code expects level.team[ TEAM_NONE ].numPlayers to be all players, spectating or playing
	// TODO: Use TEAM_ALL or the latter version for this everywhere
	level.team[ TEAM_NONE ].numPlayers += level.numPlayingPlayers;

	P[ clientNum ] = '\0';
	trap_Cvar_Set( "P", P );

	B[ clientNum ] = '\0';
	trap_Cvar_Set( "B", B );

	qsort( level.sortedClients, level.numConnectedClients,
	       sizeof( level.sortedClients[ 0 ] ), SortRanks );

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission, send the new info to everyone
	if ( level.intermissiontime )
	{
		SendScoreboardMessageToAllClients();
	}
}

/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients()
{
	int i;

	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( level.clients[ i ].pers.connected == CON_CONNECTED )
		{
			ScoreboardMessage( g_entities + i );
		}
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission( gentity_t *ent )
{
	// take out of follow mode if needed
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW )
	{
		G_StopFollowing( ent );
	}

	// move to the spot
	VectorCopy( level.intermission_origin, ent->s.origin );
	VectorCopy( level.intermission_origin, ent->client->ps.origin );
	VectorCopy( level.intermission_angle, ent->client->ps.viewangles );
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up misc info
	memset( ent->client->ps.misc, 0, sizeof( ent->client->ps.misc ) );

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint()
{
	gentity_t *ent, *target;
	vec3_t    dir;

	// find the intermission spot
	ent = G_PickRandomEntityOfClass( S_POS_PLAYER_INTERMISSION );

	if ( !ent )
	{
		// the map creator forgot to put in an intermission point...
		G_SelectRandomFurthestSpawnPoint( vec3_origin, level.intermission_origin, level.intermission_angle );
	}
	else
	{
		VectorCopy( ent->s.origin, level.intermission_origin );
		VectorCopy( ent->s.angles, level.intermission_angle );

		// if it has a target, look towards it
		if ( ent->targetCount  )
		{
			target = G_PickRandomTargetFor( ent );

			if ( target )
			{
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}
}

/*
==================
BeginIntermission
==================
*/
void BeginIntermission()
{
	int       i;
	gentity_t *client;

	if ( level.intermissiontime )
	{
		return; // already active
	}

	level.intermissiontime = level.time;

	G_ClearVotes( false );

	G_UpdateTeamConfigStrings();

	FindIntermissionPoint();

	// move all clients to the intermission point
	for ( i = 0; i < level.maxclients; i++ )
	{
		client = g_entities + i;

		if ( !client->inuse )
		{
			continue;
		}

		// respawn if dead
		if ( G_Dead( client ) )
		{
			respawn( client );
		}

		MoveClientToIntermission( client );
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();
}

/*
=============
ExitLevel

When the intermission has been exited, the server is either moved
to a new map based on the map rotation or the current map restarted
=============
*/
void ExitLevel()
{
	int       i;
	gclient_t *cl;
	char currentMapName[ MAX_STRING_CHARS ];

	trap_Cvar_VariableStringBuffer( "mapname", currentMapName, sizeof( currentMapName ) );

	// Restart if map is the same
	if ( !Q_stricmp( currentMapName, g_nextMap.string ) )
	{
		trap_Cvar_Set( "g_layouts", g_nextMapLayouts.string );
		trap_SendConsoleCommand( "map_restart" );
	}
	else if ( G_MapExists( g_nextMap.string ) )
	{
		trap_SendConsoleCommand( va( "map %s %s\n", Quote( g_nextMap.string ), Quote( g_nextMapLayouts.string ) ) );
	}
	else if ( G_MapRotationActive() )
	{
		G_AdvanceMapRotation( 0 );
	}
	else
	{
		trap_SendConsoleCommand( "map_restart\n" );
	}

	trap_Cvar_Set( "g_nextMap", "" );

	level.restarted = true;
	level.changemap = nullptr;
	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	for ( i = 0; i < level.maxclients; i++ )
	{
		cl = level.clients + i;

		if ( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		cl->ps.persistant[ PERS_SCORE ] = 0;
	}

	// we need to do this here before changing to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( level.clients[ i ].pers.connected == CON_CONNECTED )
		{
			level.clients[ i ].pers.connected = CON_CONNECTING;
		}
	}
}

/*
=================
G_AdminMessage

Print to all active server admins, and to the logfile, and to the server console
=================
*/
void G_AdminMessage( gentity_t *ent, const char *msg )
{
	char string[ 1024 ];
	int  i;

	Com_sprintf( string, sizeof( string ), "chat %ld %d %s",
	             ent ? ( long )( ent - g_entities ) : -1,
	             G_admin_permission( ent, ADMF_ADMINCHAT ) ? SAY_ADMINS : SAY_ADMINS_PUBLIC,
	             Quote( msg ) );

	// Send to all appropriate clients
	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( G_admin_permission( &g_entities[ i ], ADMF_ADMINCHAT ) )
		{
			trap_SendServerCommand( i, string );
		}
	}

	// Send to the logfile and server console
	G_LogPrintf( "%s: %d \"%s" S_COLOR_WHITE "\": " S_COLOR_MAGENTA "%s\n",
	             G_admin_permission( ent, ADMF_ADMINCHAT ) ? "AdminMsg" : "AdminMsgPublic",
	             ent ? ( int )( ent - g_entities ) : -1, ent ? ent->client->pers.netname : "console",
	             msg );
}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open, and to the server console
=================
*/
void QDECL PRINTF_LIKE(1) G_LogPrintf( const char *fmt, ... )
{
	va_list argptr;
	char    string[ 1024 ], decolored[ 1024 ];
	int     min, tens, sec;
	size_t  tslen;

	sec = level.matchTime / 1000;

	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf( string, sizeof( string ), "%3i:%i%i ", min, tens, sec );

	tslen = strlen( string );

	va_start( argptr, fmt );
	Q_vsnprintf( string + tslen, sizeof( string ) - tslen, fmt, argptr );
	va_end( argptr );

	if ( !level.inClient )
	{
		G_UnEscapeString( string, decolored, sizeof( decolored ) );
		G_Printf( "%s", decolored + 7 );
	}

	if ( !level.logFile )
	{
		return;
	}

	G_DecolorString( string, decolored, sizeof( decolored ) );
	trap_FS_Write( decolored, strlen( decolored ), level.logFile );
}

/*
=================
GetAverageCredits

Calculates the average amount of spare credits as well as the value of each teams' players.
=================
*/
static void GetAverageCredits( int teamCredits[], int teamValue[] )
{
	int       teamCnt[ NUM_TEAMS ];
	int       playerNum;
	gentity_t *playerEnt;
	gclient_t *client;
	int       team;

	for ( team = TEAM_ALIENS ; team < NUM_TEAMS ; ++team)
	{
		teamCnt[ team ] = 0;
		teamCredits[ team ] = 0;
		teamValue[ team ] = 0;
	}

	for ( playerNum = 0; playerNum < MAX_CLIENTS; playerNum++ )
	{
		playerEnt = &g_entities[ playerNum ];
		client = playerEnt->client;

		if ( !client )
		{
			continue;
		}

		team = client->pers.team;

		teamCredits[ team ] += client->pers.credit;
		teamValue[ team ] += BG_GetValueOfPlayer( &client->ps );
		teamCnt[ team ]++;
	}

	for ( team = TEAM_ALIENS ; team < NUM_TEAMS ; ++team)
	{
		teamCredits[ team ] = ( teamCnt[ team ] == 0 ) ? 0 : ( teamCredits[ team ] / teamCnt[ team ] );
		teamValue[ team ] = ( teamCnt[ team ] == 0 ) ? 0 : ( teamValue[ team ] / teamCnt[ team ] );
	}
}

/*
=================
G_LogGameplayStats
=================
*/
// Increment this if you add/change columns or otherwise change the log format
#define LOG_GAMEPLAY_STATS_VERSION 2

static void G_LogGameplayStats( int state )
{
	char       mapname[ 128 ];
	char       logline[ sizeof( Q3_VERSION ) + sizeof( mapname ) + 1024 ];

	static int nextCalculation = 0;

	if ( !level.logGameplayFile )
	{
		return;
	}

	switch ( state )
	{
		case LOG_GAMEPLAY_STATS_HEADER:
		{
			qtime_t t;

			trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );
			Com_GMTime( &t );

			Com_sprintf( logline, sizeof( logline ),
			             "# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"
			             "#\n"
			             "# Version: %s\n"
			             "# Map:     %s\n"
			             "# Date:    %04i-%02i-%02i\n"
			             "# Time:    %02i:%02i:%02i\n"
			             "# Format:  %i\n"
			             "#\n"
			             "# g_momentumHalfLife:        %4i\n"
			             "# g_initialBuildPoints:      %4i\n"
			             "# g_initialMineRate:         %4i\n"
			             "# g_mineRateHalfLife:        %4i\n"
			             "#\n"
			             "#  1  2  3    4    5    6    7    8    9   10   11   12   13   14   15   16\n"
			             "#  T #A #H AMom HMom  LMR  AME  HME  ABP  HBP ABRV HBRV ACre HCre AVal HVal\n"
			             "# -------------------------------------------------------------------------\n",
			             Q3_VERSION,
			             mapname,
			             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
			             t.tm_hour, t.tm_min, t.tm_sec,
			             LOG_GAMEPLAY_STATS_VERSION,
			             g_momentumHalfLife.integer,
			             g_initialBuildPoints.integer,
			             g_initialMineRate.integer,
			             g_mineRateHalfLife.integer );

			break;
		}
		case LOG_GAMEPLAY_STATS_BODY:
		{
			int    time;
			float  LMR;
			int    team;
			int    num[ NUM_TEAMS ];
			int    Mom[ NUM_TEAMS ];
			int    ME [ NUM_TEAMS ];
			int    BP [ NUM_TEAMS ];
			int    BRV[ NUM_TEAMS ];
			int    Cre[ NUM_TEAMS ];
			int    Val[ NUM_TEAMS ];

			if ( level.time < nextCalculation )
			{
				return;
			}

			time = level.matchTime / 1000;
			LMR  = level.mineRate; // float

			for( team = TEAM_NONE + 1; team < NUM_TEAMS; team++ )
			{
				num[ team ] = level.team[ team ].numClients;
				Mom[ team ] = ( int )level.team[ team ].momentum;
				ME [ team ] = ( int )level.team[ team ].mineEfficiency;
				BP [ team ] = G_GetBuildPointsInt( (team_t)team );
			}

			G_GetBuildableResourceValue( BRV );
			GetAverageCredits( Cre, Val );

			Com_sprintf( logline, sizeof( logline ),
			             "%4i %2i %2i %4i %4i %4.1f %4i %4i %4i %4i %4i %4i %4i %4i %4i %4i\n",
			             time, num[ TEAM_ALIENS ], num[ TEAM_HUMANS ], Mom[ TEAM_ALIENS ], Mom[ TEAM_HUMANS ],
			             LMR, ME[ TEAM_ALIENS ], ME[ TEAM_HUMANS ], BP[ TEAM_ALIENS ], BP[ TEAM_HUMANS ],
			             BRV[ TEAM_ALIENS ], BRV[ TEAM_HUMANS ], Cre[ TEAM_ALIENS ], Cre[ TEAM_HUMANS ],
			             Val[ TEAM_ALIENS ], Val[ TEAM_HUMANS ] );
			break;
		}
		case LOG_GAMEPLAY_STATS_FOOTER:
		{
			const char *winner;
			int        min, sec;

			switch ( level.lastWin )
			{
				case TEAM_ALIENS:
					winner = "Aliens";
					break;

				case TEAM_HUMANS:
					winner = "Humans";
					break;

				default:
					winner = "-";
			}

			min = level.matchTime / 60000;
			sec = ( level.matchTime / 1000 ) % 60;

			Com_sprintf( logline, sizeof( logline ),
			             "# -------------------------------------------------------------------------\n"
			             "#\n"
			             "# Match duration:  %i:%02i\n"
			             "# Winning team:    %s\n"
			             "# Average Players: %.1f + %.1f\n"
			             "# Average Aliens:  %.1f + %.1f\n"
			             "# Average Humans:  %.1f + %.1f\n"
			             "#\n",
			             min, sec,
			             winner,
			             level.team[ TEAM_ALIENS ].averageNumPlayers + level.team[ TEAM_HUMANS ].averageNumPlayers,
			             level.team[ TEAM_ALIENS ].averageNumBots    + level.team[ TEAM_HUMANS ].averageNumBots,
			             level.team[ TEAM_ALIENS ].averageNumPlayers, level.team[ TEAM_ALIENS ].averageNumBots,
			             level.team[ TEAM_HUMANS ].averageNumPlayers, level.team[ TEAM_HUMANS ].averageNumBots);
			break;
		}
		default:
			return;
	}

	trap_FS_Write( logline, strlen( logline ), level.logGameplayFile );

	if ( state == LOG_GAMEPLAY_STATS_BODY )
	{
		nextCalculation = level.time + MAX( 1, g_logGameplayStatsFrequency.integer ) * 1000;
	}
	else
	{
		nextCalculation = 0;
	}
}

/*
=================
G_SendGameStat
=================
*/
void G_SendGameStat( team_t team )
{
	char      map[ MAX_STRING_CHARS ];
	char      teamChar;
	char      data[ BIG_INFO_STRING ];
	char      entry[ MAX_STRING_CHARS ];
	int       i, dataLength, entryLength;
	gclient_t *cl;

	// games with cheats enabled are not very good for balance statistics
	if ( g_cheats.integer )
	{
		return;
	}

	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );

	switch ( team )
	{
		case TEAM_ALIENS:
			teamChar = 'A';
			break;

		case TEAM_HUMANS:
			teamChar = 'H';
			break;

		case TEAM_NONE:
			teamChar = 'L';
			break;

		default:
			return;
	}

	Com_sprintf( data, BIG_INFO_STRING,
	             "%s %s T:%c A:%f H:%f M:%s D:%d CL:%d",
	             Q3_VERSION,
	             g_tag.string,
	             teamChar,
	             level.team[ TEAM_ALIENS ].averageNumClients,
	             level.team[ TEAM_HUMANS ].averageNumClients,
	             map,
	             level.matchTime,
	             level.numConnectedClients );

	dataLength = strlen( data );

	for ( i = 0; i < level.numConnectedClients; i++ )
	{
		int ping;

		cl = &level.clients[ level.sortedClients[ i ] ];

		if ( cl->pers.connected == CON_CONNECTING )
		{
			ping = -1;
		}
		else
		{
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		switch ( cl->pers.team )
		{
			case TEAM_ALIENS:
				teamChar = 'A';
				break;

			case TEAM_HUMANS:
				teamChar = 'H';
				break;

			case TEAM_NONE:
				teamChar = 'S';
				break;

			default:
				return;
		}

		Com_sprintf( entry, MAX_STRING_CHARS,
		             " \"%s\" %c %d %d %d",
		             cl->pers.netname,
		             teamChar,
		             cl->ps.persistant[ PERS_SCORE ],
		             ping,
		             ( level.time - cl->pers.enterTime ) / 60000 );

		entryLength = strlen( entry );

		if ( dataLength + entryLength >= BIG_INFO_STRING )
		{
			break;
		}

		strcpy( data + dataLength, entry );
		dataLength += entryLength;
	}

	trap_SendGameStat( data );
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char *string )
{
	int       i, numSorted;
	gclient_t *cl;

	G_LogPrintf( "Exit: %s\n", string );

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring( CS_INTERMISSION, "1" );

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;

	if ( numSorted > 32 )
	{
		numSorted = 32;
	}

	for ( i = 0; i < numSorted; i++ )
	{
		int ping;

		cl = &level.clients[ level.sortedClients[ i ] ];

		if ( cl->pers.team == TEAM_NONE )
		{
			continue;
		}

		if ( cl->pers.connected == CON_CONNECTING )
		{
			continue;
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		G_LogPrintf( "score: %i  ping: %i  client: %i %s\n",
		             cl->ps.persistant[ PERS_SCORE ], ping, level.sortedClients[ i ],
		             cl->pers.netname );
	}

	G_SendGameStat( level.lastWin );
}

/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit()
{
	int          ready, notReady;
	int          i;
	gclient_t    *cl;
	clientList_t readyMasks;
	bool     voting = G_VotesRunning();

	//if no clients are connected, just exit
	if ( level.numConnectedClients == 0 && !voting)
	{
		ExitLevel();
		return;
	}

	// see which players are ready
	ready = 0;
	notReady = 0;
	Com_Memset( &readyMasks, 0, sizeof( readyMasks ) );

	for ( i = 0; i < level.maxclients; i++ )
	{
		cl = level.clients + i;

		if ( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if ( cl->pers.team == TEAM_NONE )
		{
			continue;
		}

		if ( cl->readyToExit )
		{
			ready++;

			Com_ClientListAdd( &readyMasks, i );
		}
		else
		{
			notReady++;
		}
	}

	trap_SetConfigstring( CS_CLIENTS_READY, Com_ClientListString( &readyMasks ) );

	// never exit in less than five seconds or if there's an ongoing vote
	if ( voting || level.time < level.intermissiontime + 5000 )
	{
		return;
	}

	// never let intermission go on for over 1 minute
	if ( level.time > level.intermissiontime + 60000 )
	{
		ExitLevel();
		return;
	}

	// if nobody wants to go, clear timer
	if ( ready == 0 && notReady > 0 )
	{
		level.readyToExit = false;
		return;
	}

	// if everyone wants to go, go now
	if ( notReady == 0 )
	{
		ExitLevel();
		return;
	}

	// the first person to ready starts the thirty second timeout
	if ( !level.readyToExit )
	{
		level.readyToExit = true;
		level.exitTime = level.time;
	}

	// if we have waited thirty seconds since at least one player
	// wanted to exit, go ahead
	if ( level.time < level.exitTime + 30000 )
	{
		return;
	}

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
bool ScoreIsTied()
{
	int a, b;

	if ( level.numPlayingClients < 2 )
	{
		return false;
	}

	a = level.clients[ level.sortedClients[ 0 ] ].ps.persistant[ PERS_SCORE ];
	b = level.clients[ level.sortedClients[ 1 ] ].ps.persistant[ PERS_SCORE ];

	return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
void CheckExitRules()
{
	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if ( level.intermissiontime )
	{
		CheckIntermissionExit();
		return;
	}

	if ( level.intermissionQueued )
	{
		if ( level.time - level.intermissionQueued >= INTERMISSION_DELAY_TIME )
		{
			level.intermissionQueued = 0;
			BeginIntermission();
		}

		return;
	}

	if ( level.timelimit )
	{
		if ( level.matchTime >= level.timelimit * 60000 )
		{
			level.lastWin = TEAM_NONE;
			trap_SendServerCommand( -1, "print_tr \"" N_("Timelimit hit\n") "\"" );
			trap_SetConfigstring( CS_WINNER, "Stalemate" );
			G_notify_sensor_end( TEAM_NONE );
			LogExit( "Timelimit hit." );
			G_MapLog_Result( 't' );
			return;
		}
		else if ( level.matchTime >= ( level.timelimit - 5 ) * 60000 &&
		          level.timelimitWarning < TW_IMMINENT )
		{
			trap_SendServerCommand( -1, "cp \"5 minutes remaining!\"" );
			level.timelimitWarning = TW_IMMINENT;
		}
		else if ( level.matchTime >= ( level.timelimit - 1 ) * 60000 &&
		          level.timelimitWarning < TW_PASSED )
		{
			trap_SendServerCommand( -1, "cp \"1 minute remaining!\"" );
			level.timelimitWarning = TW_PASSED;
		}
	}

	if ( level.unconditionalWin == TEAM_HUMANS ||
	     ( level.unconditionalWin != TEAM_ALIENS &&
	       ( level.time > level.startTime + 1000 ) &&
	       ( level.team[ TEAM_ALIENS ].numSpawns == 0 ) &&
	       ( level.team[ TEAM_ALIENS ].numAliveClients == 0 ) ) )
	{
		//humans win
		level.lastWin = TEAM_HUMANS;
		trap_SendServerCommand( -1, "print_tr \"" N_("Humans win\n") "\"" );
		trap_SetConfigstring( CS_WINNER, "Humans Win" );
		G_notify_sensor_end( TEAM_HUMANS );
		LogExit( "Humans win." );
		G_MapLog_Result( 'h' );
	}
	else if ( level.unconditionalWin == TEAM_ALIENS ||
	          ( level.unconditionalWin != TEAM_HUMANS &&
	            ( level.time > level.startTime + 1000 ) &&
	            ( level.team[ TEAM_HUMANS ].numSpawns == 0 ) &&
	            ( level.team[ TEAM_HUMANS ].numAliveClients == 0 ) ) )
	{
		//aliens win
		level.lastWin = TEAM_ALIENS;
		trap_SendServerCommand( -1, "print_tr \"" N_("Aliens win\n") "\"" );
		trap_SetConfigstring( CS_WINNER, "Aliens Win" );
		G_notify_sensor_end( TEAM_ALIENS );
		LogExit( "Aliens win." );
		G_MapLog_Result( 'a' );
	}
}

/*
==================
G_Vote
==================
*/
void G_Vote( gentity_t *ent, team_t team, bool voting )
{
	if ( !level.team[ team ].voteTime )
	{
		return;
	}

	if ( voting && (ent->client->pers.voted & ( 1 << team )) )
	{
		return;
	}

	if ( !voting && !( ent->client->pers.voted & ( 1 << team ) ) )
	{
		return;
	}

	ent->client->pers.voted |= 1 << team;

	//TODO maybe refactor vote no/yes in one and only one variable and divide the NLOC by 2 ?

	if ( voting )
	{
		level.team[ team ].voted++;
	}
	else
	{
		level.team[ team ].voted--;
	}

	if ( ent->client->pers.voteYes & ( 1 << team ) )
	{
		if ( voting )
		{
			level.team[ team ].voteYes++;
		}
		else
		{
			level.team[ team ].voteYes--;
		}

		trap_SetConfigstring( CS_VOTE_YES + team,
		                      va( "%d", level.team[ team ].voteYes ) );
	}

	if ( ent->client->pers.voteNo & ( 1 << team ) )
	{
		if ( voting )
		{
			level.team[ team ].voteNo++;
		}
		else
		{
			level.team[ team ].voteNo--;
		}

		trap_SetConfigstring( CS_VOTE_NO + team,
		                      va( "%d", level.team[ team ].voteNo ) );
	}
}

void G_ResetVote( team_t team )
{
	int i;

	level.team[ team ].voteTime = 0;
	level.team[ team ].voteYes = 0;
	level.team[ team ].voteNo = 0;
	level.team[ team ].voted = 0;

	for ( i = 0; i < level.maxclients; i++ )
	{
		level.clients[ i ].pers.voted &= ~( 1 << team );
		level.clients[ i ].pers.voteYes &= ~( 1 << team );
		level.clients[ i ].pers.voteNo &= ~( 1 << team );
	}

	trap_SetConfigstring( CS_VOTE_TIME + team, "" );
	trap_SetConfigstring( CS_VOTE_STRING + team, "" );
	trap_SetConfigstring( CS_VOTE_YES + team, "0" );
	trap_SetConfigstring( CS_VOTE_NO + team, "0" );
}
/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/

void G_ExecuteVote( team_t team )
{
	level.team[ team ].voteExecuteTime = 0;

	trap_SendConsoleCommand( va( "%s\n", level.team[ team ].voteString ) );

	if ( !Q_stricmp( level.team[ team ].voteString, "map_restart" ) )
	{
		G_MapLog_Result( 'r' );
		level.restarted = true;
	}
	else if ( !Q_strnicmp( level.team[ team ].voteString, "map", 3 ) )
	{
		G_MapLog_Result( 'm' );
		level.restarted = true;
	}
}


/*
==================
G_CheckVote
==================
*/
void G_CheckVote( team_t team )
{
	float    votePassThreshold = ( float ) level.team[ team ].voteThreshold / 100.0f;
	bool pass = false;
	bool quorum = true;
	char     *cmd;

	if ( level.team[ team ].voteExecuteTime /* > 0 ?? more readable imho */ &&
	     level.team[ team ].voteExecuteTime < level.time )
	{
		G_ExecuteVote( team );
	}

	if ( !level.team[ team ].voteTime )
	{
		return;
	}

	if ( ( level.time - level.team[ team ].voteTime >= VOTE_TIME ) ||
	     ( level.team[ team ].voteYes + level.team[ team ].voteNo == level.team[ team ].numPlayers ) )
	{
		pass = ( level.team[ team ].voteYes &&
		         ( float ) level.team[ team ].voteYes / ( ( float ) level.team[ team ].voteYes + ( float ) level.team[ team ].voteNo ) > votePassThreshold );
	}
	else
	{
		if ( ( float ) level.team[ team ].voteYes >
		     ( float ) level.team[ team ].numPlayers * votePassThreshold )
		{
			pass = true;
		}
		else if ( ( float ) level.team[ team ].voteNo <=
		          ( float ) level.team[ team ].numPlayers * ( 1.0f - votePassThreshold ) )
		{
			return;
		}
	}

	// If quorum is required, check whether at least half of who could vote did
	if ( level.team[ team ].quorum && level.team[ team ].voted < floor( powf( level.team[ team ].numPlayers, 0.6 ) ) )
	{
		quorum = false;
	}

	if ( pass && quorum )
	{
		level.team[ team ].voteExecuteTime = level.time + level.team[ team ].voteDelay;
	}

	G_LogPrintf( "EndVote: %s %s %d %d %d %d\n",
	             team == TEAM_NONE ? "global" : BG_TeamName( team ),
	             pass ? "pass" : "fail",
	             level.team[ team ].voteYes, level.team[ team ].voteNo, level.team[ team ].numPlayers, level.team[ team ].voted );

	if ( !quorum )
	{
		cmd = va( "print_tr %s %d %d", ( team == TEAM_NONE ) ? QQ( N_("Vote failed ($1$ of $2$; quorum not reached)\n") ) : QQ( N_("Team vote failed ($1$ of $2$; quorum not reached)\n") ),
		            level.team[ team ].voteYes + level.team[ team ].voteNo, level.team[ team ].numPlayers );
	}
	else if ( pass )
	{
		cmd = va( "print_tr %s %d %d", ( team == TEAM_NONE ) ? QQ( N_("Vote passed ($1$ — $2$)\n") ) : QQ( N_("Team vote passed ($1$ — $2$)\n") ),
		            level.team[ team ].voteYes, level.team[ team ].voteNo );
	}
	else
	{
		cmd = va( "print_tr %s %d %d %.0f", ( team == TEAM_NONE ) ? QQ( N_("Vote failed ($1$ — $2$; $3$% needed)\n") ) : QQ( N_("Team vote failed ($1$ — $2$; $3$% needed)\n") ),
		            level.team[ team ].voteYes, level.team[ team ].voteNo, votePassThreshold * 100 );
	}

	if ( team == TEAM_NONE )
	{
		trap_SendServerCommand( -1, cmd );
	}
	else
	{
		G_TeamCommand( team, cmd );
	}

	G_ResetVote( team );
}

/*
==================
CheckCvars
==================
*/
void CheckCvars()
{
	static int lastPasswordModCount = -1;

	if ( g_password.modificationCount != lastPasswordModCount )
	{
		lastPasswordModCount = g_password.modificationCount;

		if ( *g_password.string && Q_stricmp( g_password.string, "none" ) )
		{
			trap_Cvar_Set( "g_needpass", "1" );
		}
		else
		{
			trap_Cvar_Set( "g_needpass", "0" );
		}
	}

	level.frameMsec = trap_Milliseconds();
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
// TODO: Convert entirely to CBSE style thinking.
// TODO: Make sure this is run for all entities.
=============
*/
void G_RunThink( gentity_t *ent )
{
	// Free entities with FREE_BEFORE_THINKING set.
	DeferredFreeingComponent *deferredFreeing;
	if ((deferredFreeing = ent->entity->Get<DeferredFreeingComponent>())) {
		if (deferredFreeing->GetFreeTime() == DeferredFreeingComponent::FREE_BEFORE_THINKING) {
			G_FreeEntity(ent);
			return;
		}
	}

	// Do CBSE style thinking.
	ForEntities<ThinkingComponent>([] (Entity &entity, ThinkingComponent &thinkingComponent) {
		thinkingComponent.Think();
	});

	// Do legacy thinking.
	// TODO: Replace this kind of thinking entirely with CBSE.
	if (ent->think) {
		float thinktime = ent->nextthink;
		if (thinktime <= 0 || thinktime > level.time) return;
		ent->nextthink = 0;
		ent->think(ent);
	}

	// Free entities with FREE_AFTER_THINKING set.
	if ((deferredFreeing = ent->entity->Get<DeferredFreeingComponent>())) {
		if (deferredFreeing->GetFreeTime() == DeferredFreeingComponent::FREE_AFTER_THINKING) {
			G_FreeEntity(ent);
			return;
		}
	}
}

/*
=============
G_RunAct

Runs act code for this frame if it should
=============
*/
void G_RunAct( gentity_t *entity )
{

	if ( entity->nextAct <= 0 )
	{
		return;
	}

	if ( entity->nextAct > level.time )
	{
		return;
	}

	if ( !entity->act )
	{
		/*
		 * e.g. turrets will make use of act and nextAct as part of their think()
		 * without having an act() function
		 * other uses might be valid too, so lets not error for now
		 */
		return;
	}

	G_ExecuteAct( entity, &entity->callIn );
}

/*
=============
G_EvaluateAcceleration

Calculates the acceleration for an entity
=============
*/
void G_EvaluateAcceleration( gentity_t *ent, int msec )
{
	vec3_t deltaVelocity;
	vec3_t deltaAccel;

	VectorSubtract( ent->s.pos.trDelta, ent->oldVelocity, deltaVelocity );
	VectorScale( deltaVelocity, 1.0f / ( float ) msec, ent->acceleration );

	VectorSubtract( ent->acceleration, ent->oldAccel, deltaAccel );
	VectorScale( deltaAccel, 1.0f / ( float ) msec, ent->jerk );

	VectorCopy( ent->s.pos.trDelta, ent->oldVelocity );
	VectorCopy( ent->acceleration, ent->oldAccel );
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame( int levelTime )
{
	int        i;
	gentity_t  *ent;
	int        msec;
	static int ptime3000 = 0;

	// if we are waiting for the level to restart, do nothing
	if ( level.restarted )
	{
		return;
	}

	if ( level.pausedTime )
	{
		msec = levelTime - level.time - level.pausedTime;
		level.pausedTime = levelTime - level.time;

		ptime3000 += msec;

		while ( ptime3000 > 3000 )
		{
			ptime3000 -= 3000;
			trap_SendServerCommand( -1, "cp \"The game has been paused. Please wait.\"" );

			if ( level.pausedTime >= 110000  && level.pausedTime <= 119000 )
			{
				trap_SendServerCommand( -1, va( "print_tr %s %d", QQ( N_("Server: Game will auto-unpause in $1$ seconds\n") ),
				                                ( int )( ( float )( 120000 - level.pausedTime ) / 1000.0f ) ) );
			}
		}

		// Prevents clients from getting lagged-out messages
		for ( i = 0; i < level.maxclients; i++ )
		{
			if ( level.clients[ i ].pers.connected == CON_CONNECTED )
			{
				level.clients[ i ].ps.commandTime = levelTime;
			}
		}

		if ( level.pausedTime > 120000 )
		{
			trap_SendServerCommand( -1, "print_tr \"" N_("Server: The game has been unpaused automatically (2 minute max)\n") "\"" );
			trap_SendServerCommand( -1, "cp \"The game has been unpaused!\"" );
			level.pausedTime = 0;
		}

		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;
	level.matchTime = levelTime - level.startTime;

	msec = level.time - level.previousTime;

	// generate public-key messages
	G_admin_pubkey();

	// get any cvar changes
	G_UpdateCvars();
	CheckCvars();

	// now we are done spawning
	level.spawning = false;

	G_CheckPmoveParamChanges();

	// go through all allocated objects
	ent = &g_entities[ 0 ];
	for ( i = 0; i < level.num_entities; i++, ent++ )
	{
		if ( !ent->inuse ) continue;

		// clear events that are too old
		if ( level.time - ent->eventTime > EVENT_VALID_MSEC )
		{
			if ( ent->s.event )
			{
				ent->s.event = 0; // &= EV_EVENT_BITS;

				if ( ent->client )
				{
					ent->client->ps.externalEvent = 0;
					//ent->client->ps.events[0] = 0;
					//ent->client->ps.events[1] = 0;
				}
			}

			if ( ent->freeAfterEvent )
			{
				// tempEntities or dropped items completely go away after their event
				G_FreeEntity( ent );
				continue;
			}
			else if ( ent->unlinkAfterEvent )
			{
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = false;
				trap_UnlinkEntity( ent );
			}
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) continue;

		// calculate the acceleration of this entity
		if ( ent->evaluateAcceleration ) G_EvaluateAcceleration( ent, msec );

		if ( !ent->r.linked && ent->neverFree ) continue;

		// think/run entitiy by type
		switch ( ent->s.eType )
		{
			case ET_MISSILE:
				G_RunMissile( ent );
				continue;

			case ET_BUILDABLE:
				// TODO: Do buildables make any use of G_Physics' functionality apart from the call
				//       to G_RunThink?
				G_Physics( ent, msec );
				continue;

			case ET_CORPSE:
				G_Physics( ent, msec );
				continue;

			case ET_MOVER:
				G_RunMover( ent );
				continue;

			default:
				if ( ent->physicsObject )
				{
					G_Physics( ent, msec );
					continue;
				}
				else if ( i < MAX_CLIENTS )
				{
					G_RunClient( ent );
					continue;
				}
				else
				{
					G_RunThink( ent );

					// allow entities to free themselves before acting
					if ( ent->inuse )
					{
						// TODO: Is this even used/necessary?
						//       Why do only randomly chose entities do this?
						G_RunAct( ent );
					}
				}
		}
	}

	// perform final fixups on the players
	ent = &g_entities[ 0 ];

	for ( i = 0; i < level.maxclients; i++, ent++ )
	{
		if ( ent->inuse )
		{
			ClientEndFrame( ent );
		}
	}

	// save position information for all active clients
	G_UnlaggedStore();

	G_CountSpawns();
	G_SetHumanBuildablePowerState();
	G_MineBuildPoints();
	G_DecreaseMomentum();
	G_CalculateAvgPlayers();
	G_SpawnClients( TEAM_ALIENS );
	G_SpawnClients( TEAM_HUMANS );
	G_UpdateZaps( msec );
	Beacon::Frame( );

	G_PrepareEntityNetCode();

	// log gameplay statistics
	G_LogGameplayStats( LOG_GAMEPLAY_STATS_BODY );

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	for ( i = 0; i < NUM_TEAMS; i++ )
	{
		G_CheckVote( (team_t) i );
	}

	trap_BotUpdateObstacles();
	level.frameMsec = trap_Milliseconds();
}

void G_PrepareEntityNetCode() {
	// TODO: Allow ForEntities with empty template arguments.
	gentity_t *oldEnt = &g_entities[0];
	for (int i = 0; i < level.num_entities; i++, oldEnt++) {
		if (oldEnt->entity) {
			oldEnt->entity->PrepareNetCode();
		}
	}
}
