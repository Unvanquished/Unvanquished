/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012-2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef SG_EXTERN_H_
#define SG_EXTERN_H_

// -----
// world
// -----

extern  level_locals_t level;
#ifdef QVM_ABI
extern  gentity_t      g_entities[ MAX_GENTITIES ];
#else
extern gentity_t *g_entities;
extern gclient_t *g_clients;
#endif

// ---------
// temporary, compatibility layer between legacy code and CBSE logic
// ---------

extern damageRegion_t g_damageRegions[ PCL_NUM_CLASSES ][ MAX_DAMAGE_REGIONS ];
extern int            g_numDamageRegions[ PCL_NUM_CLASSES ];

// -----
// cvars
// -----

extern  vmCvar_t g_cheats;
extern  vmCvar_t g_maxclients;
extern  vmCvar_t g_maxGameClients;
extern  vmCvar_t g_lockTeamsAtStart;
extern  vmCvar_t g_minNameChangePeriod;
extern  vmCvar_t g_maxNameChanges;

extern  vmCvar_t g_showHelpOnConnection;
extern  vmCvar_t g_timelimit;
extern  vmCvar_t g_friendlyFire;
extern  vmCvar_t g_friendlyBuildableFire;
extern  vmCvar_t g_dretchPunt;
extern  vmCvar_t g_password;
extern  vmCvar_t g_needpass;
extern  vmCvar_t g_gravity;
extern  vmCvar_t g_speed;
extern  vmCvar_t g_inactivity;
extern  vmCvar_t g_debugMove;
extern  vmCvar_t g_debugDamage;
extern  vmCvar_t g_debugKnockback;
extern  vmCvar_t g_debugTurrets;
extern  vmCvar_t g_debugFire;
extern  vmCvar_t g_motd;
extern  vmCvar_t g_warmup;
extern  vmCvar_t g_doWarmup;
extern  vmCvar_t g_allowVote;
extern  vmCvar_t g_voteLimit;
extern  vmCvar_t g_extendVotesPercent;
extern  vmCvar_t g_extendVotesTime;
extern  vmCvar_t g_kickVotesPercent;
extern  vmCvar_t g_denyVotesPercent;
extern  vmCvar_t g_mapVotesPercent;
extern  vmCvar_t g_mapVotesBefore;
extern  vmCvar_t g_nextMapVotesPercent;
extern  vmCvar_t g_drawVotesPercent;
extern  vmCvar_t g_drawVotesAfter;
extern  vmCvar_t g_drawVoteReasonRequired;
extern  vmCvar_t g_admitDefeatVotesPercent;
extern  vmCvar_t g_pollVotesPercent;
extern  vmCvar_t g_botKickVotesAllowed;
extern  vmCvar_t g_botKickVotesAllowedThisMap;
extern  vmCvar_t g_teamForceBalance;
extern  vmCvar_t g_smoothClients;

extern  vmCvar_t g_initialMineRate;
extern  vmCvar_t g_initialBuildPoints;
extern  vmCvar_t g_mineRateHalfLife;
extern  vmCvar_t g_minimumMineRate;
extern  vmCvar_t g_buildPointLossFraction;

extern  vmCvar_t g_debugMomentum;
extern  vmCvar_t g_momentumHalfLife;
extern  vmCvar_t g_momentumRewardDoubleTime;
extern  vmCvar_t g_unlockableMinTime;
extern  vmCvar_t g_momentumBaseMod;
extern  vmCvar_t g_momentumKillMod;
extern  vmCvar_t g_momentumBuildMod;
extern  vmCvar_t g_momentumDeconMod;
extern  vmCvar_t g_momentumDestroyMod;

extern  vmCvar_t g_humanAllowBuilding;
extern  vmCvar_t g_alienAllowBuilding;

extern  vmCvar_t g_powerCompetitionRange;
extern  vmCvar_t g_powerBaseSupply;
extern  vmCvar_t g_powerReactorSupply;
extern  vmCvar_t g_powerReactorRange;
extern  vmCvar_t g_powerRepeaterSupply;
extern  vmCvar_t g_powerRepeaterRange;

extern  vmCvar_t g_alienOffCreepRegenHalfLife;

extern  vmCvar_t g_teamImbalanceWarnings;
extern  vmCvar_t g_freeFundPeriod;

extern  vmCvar_t g_unlagged;

extern  vmCvar_t g_disabledEquipment;
extern  vmCvar_t g_disabledClasses;
extern  vmCvar_t g_disabledBuildables;
extern  vmCvar_t g_disabledVoteCalls;

extern  vmCvar_t g_debugMapRotation;
extern  vmCvar_t g_currentMapRotation;
extern  vmCvar_t g_mapRotationNodes;
extern  vmCvar_t g_mapRotationStack;
extern  vmCvar_t g_nextMap;
extern  vmCvar_t g_nextMapLayouts;
extern  vmCvar_t g_initialMapRotation;
extern  vmCvar_t g_mapLog;
extern  vmCvar_t g_mapStartupMessageDelay;
extern  vmCvar_t g_sayAreaRange;

extern  vmCvar_t g_debugVoices;
extern  vmCvar_t g_enableVsays;

extern  vmCvar_t g_floodMaxDemerits;
extern  vmCvar_t g_floodMinTime;

extern  vmCvar_t g_shove;
extern  vmCvar_t g_antiSpawnBlock;

extern  vmCvar_t g_mapConfigs;

extern  vmCvar_t g_defaultLayouts;
extern  vmCvar_t g_layouts;
extern  vmCvar_t g_layoutAuto;

extern  vmCvar_t g_emoticonsAllowedInNames;
extern  vmCvar_t g_unnamedNumbering;
extern  vmCvar_t g_unnamedNamePrefix;

extern  vmCvar_t g_admin;
extern  vmCvar_t g_adminWarn;
extern  vmCvar_t g_adminTempBan;
extern  vmCvar_t g_adminMaxBan;
extern  vmCvar_t g_adminRetainExpiredBans;

extern  vmCvar_t g_privateMessages;
extern  vmCvar_t g_specChat;
extern  vmCvar_t g_publicAdminMessages;
extern  vmCvar_t g_allowTeamOverlay;

extern  vmCvar_t g_showKillerHP;
extern  vmCvar_t g_combatCooldown;

extern  vmCvar_t g_geoip;

extern  vmCvar_t g_debugEntities;

extern  vmCvar_t g_instantBuilding;

// bot buy cvars
extern vmCvar_t g_bot_buy;
extern vmCvar_t g_bot_rifle;
extern vmCvar_t g_bot_painsaw;
extern vmCvar_t g_bot_shotgun;
extern vmCvar_t g_bot_lasgun;
extern vmCvar_t g_bot_mdriver;
extern vmCvar_t g_bot_chaingun;
extern vmCvar_t g_bot_prifle;
extern vmCvar_t g_bot_flamer;
extern vmCvar_t g_bot_lcannon;

// bot evolution cvars
extern vmCvar_t g_bot_evolve;
extern vmCvar_t g_bot_level1;
extern vmCvar_t g_bot_level2;
extern vmCvar_t g_bot_level2upg;
extern vmCvar_t g_bot_level3;
extern vmCvar_t g_bot_level3upg;
extern vmCvar_t g_bot_level4;

// misc bot cvars
extern vmCvar_t g_bot_attackStruct;
extern vmCvar_t g_bot_roam;
extern vmCvar_t g_bot_rush;
extern vmCvar_t g_bot_build;
extern vmCvar_t g_bot_repair;
extern vmCvar_t g_bot_retreat;
extern vmCvar_t g_bot_fov;
extern vmCvar_t g_bot_chasetime;
extern vmCvar_t g_bot_reactiontime;
extern vmCvar_t g_bot_infinite_funds;
extern vmCvar_t g_bot_numInGroup;
extern vmCvar_t g_bot_persistent;
extern vmCvar_t g_bot_buildLayout;
extern vmCvar_t g_bot_debug;

#endif // SG_EXTERN_H_
