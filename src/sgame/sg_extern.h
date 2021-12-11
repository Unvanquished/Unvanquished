/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012-2013 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef SG_EXTERN_H_
#define SG_EXTERN_H_

// -----
// world
// -----

extern  level_locals_t level;

extern gentity_t *g_entities;
extern gclient_t *g_clients;

extern bool g_cheats;

// ---------
// temporary, compatibility layer between legacy code and CBSE logic
// ---------

extern damageRegion_t g_damageRegions[ PCL_NUM_CLASSES ][ MAX_DAMAGE_REGIONS ];
extern int            g_numDamageRegions[ PCL_NUM_CLASSES ];

// -----
// cvars
// -----

extern Cvar::Cvar<int> g_maxGameClients;
extern Cvar::Cvar<bool> g_lockTeamsAtStart;
extern Cvar::Cvar<float> g_minNameChangePeriod;
extern Cvar::Cvar<int> g_maxNameChanges;

extern Cvar::Range<Cvar::Cvar<int>> g_showHelpOnConnection;
extern Cvar::Cvar<int> g_timelimit;
extern Cvar::Cvar<bool> g_dretchPunt;
extern Cvar::Callback<Cvar::Cvar<std::string>> g_password;
extern Cvar::Range<Cvar::Cvar<int>> g_needpass;
extern  Cvar::Range<Cvar::Cvar<int>> g_gravity;
extern Cvar::Cvar<float> g_speed;
extern Cvar::Cvar<std::string> g_inactivity;
extern Cvar::Cvar<int> g_debugMove;
extern Cvar::Cvar<bool> g_debugFire;
extern Cvar::Cvar<std::string> g_motd;
extern Cvar::Cvar<int> g_warmup;
extern Cvar::Cvar<bool> g_doWarmup;
extern Cvar::Cvar<bool> g_allowVote;
extern Cvar::Cvar<int> g_voteLimit;
extern Cvar::Cvar<int> g_extendVotesPercent;
extern Cvar::Cvar<int> g_extendVotesTime;
extern Cvar::Cvar<int> g_kickVotesPercent;
extern Cvar::Cvar<int> g_denyVotesPercent;
extern Cvar::Cvar<int> g_mapVotesPercent;
extern Cvar::Cvar<int> g_mapVotesBefore;
extern Cvar::Cvar<int> g_nextMapVotesPercent;
extern Cvar::Cvar<int> g_drawVotesPercent;
extern Cvar::Cvar<int> g_drawVotesAfter;
extern Cvar::Cvar<bool> g_drawVoteReasonRequired;
extern Cvar::Cvar<int> g_admitDefeatVotesPercent;
extern Cvar::Cvar<int> g_pollVotesPercent;
extern Cvar::Range<Cvar::Cvar<int>> g_teamForceBalance;
extern Cvar::Cvar<bool> g_smoothClients;

extern  Cvar::Callback<Cvar::Cvar<int>> g_buildPointInitialBudget;
extern  Cvar::Callback<Cvar::Cvar<int>> g_buildPointBudgetPerMiner;
extern  Cvar::Cvar<int> g_buildPointRecoveryInitialRate;
extern  Cvar::Cvar<int> g_buildPointRecoveryRateHalfLife;

extern Cvar::Range<Cvar::Cvar<int>> g_debugMomentum;
extern Cvar::Cvar<float> g_momentumHalfLife;
extern Cvar::Cvar<float> g_momentumRewardDoubleTime;
extern Cvar::Cvar<float> g_unlockableMinTime;
extern Cvar::Cvar<float> g_momentumBaseMod;
extern Cvar::Cvar<float> g_momentumKillMod;
extern Cvar::Cvar<float> g_momentumBuildMod;
extern Cvar::Cvar<float> g_momentumDeconMod;
extern Cvar::Cvar<float> g_momentumDestroyMod;

extern Cvar::Cvar<bool> g_humanAllowBuilding;
extern Cvar::Cvar<bool> g_alienAllowBuilding;

extern Cvar::Cvar<float> g_alienOffCreepRegenHalfLife;

extern Cvar::Cvar<bool> g_teamImbalanceWarnings;
extern Cvar::Cvar<int> g_freeFundPeriod;

extern Cvar::Range<Cvar::Cvar<int>> g_unlagged;

extern Cvar::Cvar<std::string> g_disabledVoteCalls;

extern Cvar::Cvar<bool> g_debugMapRotation;
extern Cvar::Cvar<int> g_currentMapRotation;
extern Cvar::Cvar<std::string> g_mapRotationNodes;
extern Cvar::Cvar<std::string> g_mapRotationStack;
extern Cvar::Cvar<std::string> g_nextMap;
extern Cvar::Cvar<std::string> g_nextMapLayouts;
extern Cvar::Cvar<std::string> g_initialMapRotation;
extern Cvar::Cvar<std::string> g_mapLog;
extern Cvar::Cvar<std::string> g_mapStartupMessage;
extern Cvar::Cvar<int> g_mapStartupMessageDelay;
extern Cvar::Cvar<float> g_sayAreaRange;

extern Cvar::Cvar<bool> g_debugVoices;
extern Cvar::Cvar<bool> g_enableVsays;

extern Cvar::Cvar<int> g_floodMaxDemerits;
extern Cvar::Cvar<int> g_floodMinTime;

extern Cvar::Cvar<float> g_shove;
extern Cvar::Cvar<bool> g_antiSpawnBlock;

extern Cvar::Cvar<std::string> g_mapConfigs;

extern Cvar::Cvar<std::string> g_defaultLayouts;
extern Cvar::Cvar<std::string> g_layouts;
extern Cvar::Cvar<bool> g_layoutAuto;

extern Cvar::Cvar<bool> g_emoticonsAllowedInNames;
extern Cvar::Cvar<int> g_unnamedNumbering;
extern Cvar::Cvar<std::string> g_unnamedNamePrefix;
extern Cvar::Cvar<std::string> g_unnamedBotNamePrefix;

extern Cvar::Cvar<std::string> g_admin;
extern Cvar::Cvar<std::string> g_adminWarn;
extern Cvar::Cvar<std::string> g_adminTempBan;
extern Cvar::Cvar<std::string> g_adminMaxBan;
extern Cvar::Cvar<bool> g_adminRetainExpiredBans;

extern Cvar::Cvar<bool> g_privateMessages;
extern Cvar::Cvar<bool> g_specChat;
extern Cvar::Cvar<bool> g_publicAdminMessages;
extern Cvar::Cvar<bool> g_allowTeamOverlay;

extern Cvar::Cvar<bool> g_showKillerHP;
extern Cvar::Cvar<int> g_combatCooldown;

extern Cvar::Cvar<bool> g_geoip;

extern Cvar::Range<Cvar::Cvar<int>> g_debugEntities;

extern Cvar::Cvar<bool> g_instantBuilding;

extern  Cvar::Cvar<float> g_evolveAroundHumans;
extern  Cvar::Cvar<float> g_devolveMaxBaseDistance;

// bots: buying weapons cvars
extern Cvar::Cvar<bool> g_bot_buy;
extern Cvar::Cvar<int> g_bot_radarRatio;
extern Cvar::Cvar<bool> g_bot_ckit;
extern Cvar::Cvar<bool> g_bot_rifle;
extern Cvar::Cvar<bool> g_bot_painsaw;
extern Cvar::Cvar<bool> g_bot_shotgun;
extern Cvar::Cvar<bool> g_bot_lasgun;
extern Cvar::Cvar<bool> g_bot_mdriver;
extern Cvar::Cvar<bool> g_bot_chaingun;
extern Cvar::Cvar<bool> g_bot_prifle;
extern Cvar::Cvar<bool> g_bot_flamer;
extern Cvar::Cvar<bool> g_bot_lcannon;
// bots: buying armors cvars
extern Cvar::Cvar<bool> g_bot_battlesuit;
extern Cvar::Cvar<bool> g_bot_mediumarmour;
extern Cvar::Cvar<bool> g_bot_lightarmour;
// bots: buying other equipments cvars
extern Cvar::Cvar<bool> g_bot_radar;
extern Cvar::Cvar<bool> g_bot_jetpack; //can't be really used, yet
extern Cvar::Cvar<bool> g_bot_grenade;
extern Cvar::Cvar<bool> g_bot_firebomb;

// bot evolution cvars
extern Cvar::Cvar<bool> g_bot_evolve;
extern Cvar::Cvar<bool> g_bot_builder;
extern Cvar::Cvar<bool> g_bot_builderupg;
extern Cvar::Cvar<bool> g_bot_level0;
extern Cvar::Cvar<bool> g_bot_level1;
extern Cvar::Cvar<bool> g_bot_level2;
extern Cvar::Cvar<bool> g_bot_level2upg;
extern Cvar::Cvar<bool> g_bot_level3;
extern Cvar::Cvar<bool> g_bot_level3upg;
extern Cvar::Cvar<bool> g_bot_level4;

// bot default configurations
extern Cvar::Range<Cvar::Cvar<int>> g_bot_default_skill;

// misc bot cvars
extern Cvar::Cvar<bool> g_bot_attackStruct;
extern Cvar::Cvar<float> g_bot_fov;
extern Cvar::Cvar<int> g_bot_chasetime;
extern Cvar::Cvar<int> g_bot_reactiontime;
extern Cvar::Cvar<bool> g_bot_infinite_funds;

#endif // SG_EXTERN_H_
