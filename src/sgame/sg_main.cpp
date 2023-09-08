/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
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

#include "sg_local.h"
#include "shared/parse.h"
#include "Entities.h"
#include "CBSE.h"
#include "backend/CBSEBackend.h"
#include "botlib/bot_api.h"
#include "common/FileSystem.h"

#define INTERMISSION_DELAY_TIME 1000

level_locals_t level;

gentity_t          *g_entities;
gclient_t          *g_clients;

Cvar::Range<Cvar::Cvar<int>> g_showWelcome("g_showWelcome", "show MN_WELCOME on connect (0 = none, 1 = all, 2 = unregistered)", Cvar::NONE, 0, 0, 2);

Cvar::Range<Cvar::Cvar<int>> g_gravity(
		"g_gravity",
		"how strongly things will be attracted towards the ground",
		Cvar::NONE,
		800, 100, 2000);

Cvar::Cvar<int> g_timelimit("timelimit", "max game length in minutes", Cvar::SERVERINFO, 45);
Cvar::Cvar<bool> g_dretchPunt("g_dretchPunt", "aliens can propel dretches by attacking them", Cvar::NONE, true);
// This isn't a configuration variable to set whether a password is needed.
// Rather, it's value is automatically set based on whether g_password is empty;
// it's a hack to stick a value into the serverinfo string.
Cvar::Range<Cvar::Cvar<int>> g_needpass("g_needpass", "FOR INTERNAL USE", Cvar::SERVERINFO, 0, 0, 1);
Cvar::Cvar<int> g_maxGameClients("g_maxGameClients", "max number of players (see also sv_maxclients)", Cvar::SERVERINFO, 0);
Cvar::Cvar<float> g_speed("g_speed", "player movement speed multiplier", Cvar::NONE, 320);
bool g_cheats;
Cvar::Cvar<std::string> g_inactivity("g_inactivity", "seconds of inactivity before a player is removed. append 's' to spec instead of kick", Cvar::NONE, "0");
Cvar::Cvar<int> g_debugMove("g_debugMove", "sgame pmove debug level", Cvar::NONE, 0);
Cvar::Cvar<bool> g_debugFire("g_debugFire", "debug ground fire spawning", Cvar::NONE, false);
Cvar::Cvar<std::string> g_motd("g_motd", "message of the day", Cvar::NONE, "");
// g_synchronousClients stays as an int for now instead of a bool
// because there is a place in cl_main.cpp that tries to parse it
Cvar::Range<Cvar::Cvar<int>> g_synchronousClients("g_synchronousClients", "calculate player movement once per server frame", Cvar::NONE, 0, 0, 1);
Cvar::Cvar<int> g_warmup("g_warmup", "seconds after game start before players can join", Cvar::NONE, 10);
Cvar::Cvar<bool> g_doWarmup("g_doWarmup", "whether to enable warmup period (g_warmup)", Cvar::NONE, false);
Cvar::Cvar<bool> g_lockTeamsAtStart("g_lockTeamsAtStart", "(internal use) lock teams at start of match", Cvar::NONE, false);
Cvar::Cvar<std::string> g_logFile("g_logFile", "sgame log file, relative to <homepath>/game/", Cvar::NONE, "games.log");
Cvar::Cvar<int> g_logGameplayStatsFrequency("g_logGameplayStatsFrequency", "log gameplay stats every x seconds", Cvar::NONE, 10);
Cvar::Cvar<bool> g_logFileSync("g_logFileSync", "flush g_logFile on every write", Cvar::NONE, false);
Cvar::Cvar<bool> g_allowVote("g_allowVote", "whether votes of any kind are allowed", Cvar::NONE, true);
Cvar::Cvar<int> g_voteLimit("g_voteLimit", "max votes per player per round", Cvar::NONE, 5);
Cvar::Cvar<int> g_extendVotesPercent("g_extendVotesPercent", "percentage required for extend timelimit vote", Cvar::NONE, 74);
Cvar::Cvar<int> g_extendVotesTime("g_extendVotesTime", "number of minutes 'extend' vote adds to timelimit", Cvar::NONE, 10);
Cvar::Cvar<int> g_kickVotesPercent("g_kickVotesPercent", "percentage required for votes to remove players", Cvar::NONE, 51);
Cvar::Cvar<int> g_denyVotesPercent("g_denyVotesPercent", "percentage required for votes to strip/reinstate a player's chat/build privileges", Cvar::NONE, 51);
Cvar::Cvar<int> g_mapVotesPercent("g_mapVotesPercent", "percentage required for map changing votes", Cvar::NONE, 51);
Cvar::Cvar<int> g_mapVotesBefore("g_mapVotesBefore", "map change votes not allowed after this many minutes", Cvar::NONE, 5);
Cvar::Cvar<int> g_drawVotesPercent("g_drawVotesPercent", "percentage required for draw vote", Cvar::NONE, 51);
Cvar::Cvar<int> g_drawVotesAfter("g_drawVotesAfter", "draw votes not allowed before this many minutes", Cvar::NONE, 0);
Cvar::Cvar<bool> g_drawVoteReasonRequired("g_drawVoteReasonRequired", "whether a reason is required for draw vote", Cvar::NONE, false);
Cvar::Cvar<int> g_admitDefeatVotesPercent("g_admitDefeatVotesPercent", "percentage required for admitdefeat vote", Cvar::NONE, 74);
Cvar::Cvar<int> g_nextMapVotesPercent("g_nextMapVotesPercent", "percentage required for nextmap vote", Cvar::NONE, 51);
Cvar::Cvar<int> g_pollVotesPercent("g_pollVotesPercent", "percentage required for a poll to 'pass'", Cvar::NONE, 0);
Cvar::Cvar<int> g_maxVoteFillBots("g_maxVoteFillBots", "maximum number of bots per team allowed by votes", Cvar::NONE, 10);
Cvar::Cvar<int> g_fillBotsVotesPercent("g_fillBotsVotesPercent", "percentage required for votes to fill both teams with bots", Cvar::NONE, 51);
Cvar::Cvar<int> g_fillBotsTeamVotesPercent("g_fillBotsTeamVotesPercent", "percentage required for votes to fill only one team with bots", Cvar::NONE, 67);
Cvar::Cvar<int> g_customVotesPercent( "g_customVotesPercent",
                                      "percentage required for custom votes to pass", Cvar::NONE,
                                      51 );

Cvar::Range<Cvar::Cvar<int>> g_teamForceBalance("g_teamForceBalance", "disallow joining a team with more players (1 = always, 2 = allow N vs. 0)", Cvar::NONE, 0, 0, 2);
Cvar::Cvar<bool> g_smoothClients("g_smoothClients", "something about player movement extrapolation", Cvar::NONE, true);
Cvar::Cvar<bool> pmove_fixed("pmove_fixed", "use pmove_msec instead of 66", Cvar::NONE, false);
Cvar::Range<Cvar::Cvar<int>> pmove_msec("pmove_msec", "max sgame pmove period in milliseconds", Cvar::NONE, 8, 8, 33);
Cvar::Cvar<bool> pmove_accurate("pmove_accurate", "don't round player velocity to integer", Cvar::NONE, true);
Cvar::Cvar<float> g_minNameChangePeriod("g_minNameChangePeriod", "player must wait x seconds to change name", Cvar::NONE, 5);
Cvar::Cvar<int> g_maxNameChanges("g_maxNameChanges", "max name changes per game", Cvar::NONE, 5);

// gameplay: mining
Cvar::Callback<Cvar::Cvar<int>> g_buildPointInitialBudget(
		"g_BPInitialBudget",
		"Initial build points count",
		Cvar::SERVERINFO,
		DEFAULT_BP_INITIAL_BUDGET,
		[](int) {
			G_UpdateBuildPointBudgets();
		});
Cvar::Callback<Cvar::Cvar<int>> g_BPInitialBudgetHumans(
		"g_BPInitialBudgetHumans",
		"Initial build points count for humans",
		Cvar::SERVERINFO,
		-1,
		[](int) {
			G_UpdateBuildPointBudgets();
		});
Cvar::Callback<Cvar::Cvar<int>> g_BPInitialBudgetAliens(
		"g_BPInitialBudgetAliens",
		"Initial build points count for humans",
		Cvar::SERVERINFO,
		-1,
		[](int) {
			G_UpdateBuildPointBudgets();
		});
Cvar::Callback<Cvar::Cvar<int>> g_buildPointBudgetPerMiner(
		"g_BPBudgetPerMiner",
		"Budget per Miner",
		Cvar::SERVERINFO,
		DEFAULT_BP_BUDGET_PER_MINER,
		[](int) {
			G_UpdateBuildPointBudgets();
		});
Cvar::Cvar<int> g_buildPointRecoveryInitialRate(
		"g_BPRecoveryInitialRate",
		"The initial speed at which BP will be recovered (in BP per minute)",
		Cvar::SERVERINFO,
		DEFAULT_BP_RECOVERY_INITIAL_RATE);
Cvar::Cvar<int> g_buildPointRecoveryRateHalfLife(
		"g_BPRecoveryRateHalfLife",
		"The duration one will wait before BP recovery gets twice as slow (in minutes)",
		Cvar::SERVERINFO,
		DEFAULT_BP_RECOVERY_RATE_HALF_LIFE);

Cvar::Range<Cvar::Cvar<int>> g_debugMomentum("g_debugMomentum", "momentum debug level", Cvar::NONE, 0, 0, 2);
Cvar::Cvar<float> g_momentumHalfLife("g_momentumHalfLife", "minutes for momentum to decrease 50%", Cvar::SERVERINFO, DEFAULT_MOMENTUM_HALF_LIFE);
Cvar::Cvar<float> g_momentumRewardDoubleTime("g_momentumRewardDoubleTime", "some momentum rewards double after x minutes", Cvar::NONE, DEFAULT_CONF_REWARD_DOUBLE_TIME);
Cvar::Cvar<float> g_unlockableMinTime("g_unlockableMinTime", "an unlock is lost after x seconds without further momentum rewards", Cvar::SERVERINFO, DEFAULT_UNLOCKABLE_MIN_TIME);
Cvar::Cvar<float> g_momentumBaseMod("g_momentumBaseMod", "generic momentum reward multiplier", Cvar::NONE, DEFAULT_MOMENTUM_BASE_MOD);
Cvar::Cvar<float> g_momentumKillMod("g_momentumKillMod", "momentum reward multiplier for kills", Cvar::NONE, DEFAULT_MOMENTUM_KILL_MOD);
Cvar::Cvar<float> g_momentumBuildMod("g_momentumBuildMod", "momentum reward multiplier for construction", Cvar::NONE, DEFAULT_MOMENTUM_BUILD_MOD);
Cvar::Cvar<float> g_momentumDeconMod("g_momentumDeconMod", "momentum penalty multiplier for deconstruction", Cvar::NONE, DEFAULT_MOMENTUM_DECON_MOD);
Cvar::Cvar<float> g_momentumDestroyMod("g_momentumDestroyMod", "momentum reward multiplier for killing buildables", Cvar::NONE, DEFAULT_MOMENTUM_DESTROY_MOD);


Cvar::Cvar<bool> g_humanAllowBuilding(
		"g_humanAllowBuilding",
		"can human build",
		Cvar::NONE,
		true);

Cvar::Cvar<bool> g_alienAllowBuilding(
		"g_alienAllowBuilding",
		"can aliens build",
		Cvar::NONE,
		true);

Cvar::Cvar<float> g_alienOffCreepRegenHalfLife("g_alienOffCreepRegenHalfLife", "half-life in seconds for decay of creep's healing bonus", Cvar::NONE, 0);

Cvar::Cvar<int> g_teamImbalanceWarnings("g_teamImbalanceWarnings", "send 'Teams are imbalanced' messages every x seconds if >0", Cvar::NONE, 30);
Cvar::Cvar<int> g_freeFundPeriod("g_freeFundPeriod", "every x seconds, players get funds for nothing", Cvar::NONE, DEFAULT_FREEKILL_PERIOD);
// int instead of bool for now to avoid changing the serverinfo format
Cvar::Range<Cvar::Cvar<int>> g_unlagged("g_unlagged", "whether latency compensation is enabled", Cvar::SERVERINFO, 1, 0, 1);
Cvar::Cvar<std::string> g_disabledVoteCalls(
		"g_disabledVoteCalls",
		"Forbidden votes, like " QQ("kickbots, nextmap, spectate"),
		Cvar::SERVERINFO,
		"" // everything is allowed by default
		);

Cvar::Cvar<bool> g_debugMapRotation("g_debugMapRotation", "print map rotation debug info", Cvar::NONE, false);
Cvar::Cvar<int> g_currentMapRotation("g_currentMapRotation", "FOR INTERNAL USE", Cvar::NONE, -1);
Cvar::Cvar<std::string> g_mapRotationNodes("g_mapRotationNodes", "FOR INTERNAL USE", Cvar::NONE, "");
Cvar::Cvar<std::string> g_mapRotationStack("g_mapRotationStack", "FOR INTERNAL USE", Cvar::NONE, "");
Cvar::Cvar<std::string> g_nextMap("g_nextMap", "map to load next (cleared after use)", Cvar::NONE, "");
Cvar::Cvar<std::string> g_nextMapLayouts("g_nextMapLayouts", "list of layouts (one's chosen randomly) to go with g_nextMap", Cvar::NONE, "");
Cvar::Cvar<std::string> g_initialMapRotation("g_initialMapRotation", "map rotation to use on server startup", Cvar::NONE, "rotation1");
static Cvar::Cvar<bool> g_persistDevmap("g_persistDevmap", "allow cheats next map if allowed now", Cvar::NONE, false);
Cvar::Cvar<std::string> g_mapLog("g_mapLog", "contains results of recent games", Cvar::NONE, "");
Cvar::Cvar<std::string> g_mapStartupMessage("g_mapStartupMessage", "message sent to players on connection (reset after game)", Cvar::NONE, "");
Cvar::Cvar<int> g_mapStartupMessageDelay("g_mapStartupMessageDelay", "show g_mapStartupMessage x milliseconds after connection", Cvar::NONE, 5000);

Cvar::Cvar<bool> g_debugVoices("g_debugVoices", "print sgame's list of vsays on startup", Cvar::NONE, false);
Cvar::Cvar<bool> g_enableVsays("g_voiceChats", "allow vsays (prerecorded audio messages)", Cvar::NONE, true);

Cvar::Cvar<float> g_shove("g_shove", "force multiplier when pushing players", Cvar::NONE, 0.0);
Cvar::Cvar<bool> g_antiSpawnBlock("g_antiSpawnBlock", "push away players who block their spawns", Cvar::NONE, false);

Cvar::Cvar<std::string> g_mapConfigs("g_mapConfigs", "map config directory, relative to <homepath>/config/", Cvar::NONE, "map");
Cvar::Cvar<float> g_sayAreaRange("g_sayAreaRange", "distance for area chat messages", Cvar::NONE, 1000);

Cvar::Cvar<int> g_floodMaxDemerits("g_floodMaxDemerits", "client message rate control (lower = stricter)", Cvar::NONE, 5000);
Cvar::Cvar<int> g_floodMinTime("g_floodMinTime", "mute period after flooding, in milliseconds", Cvar::NONE, 2000);
Cvar::Cvar<int> g_teamStatus("g_teamStatus", "allow /teamstatus command. 0 = disabled, otherwise you can only /teamstatus every <g_teamStatus> seconds.", Cvar::NONE, 5);

Cvar::Cvar<int> g_tacticMilliseconds("g_tacticMilliseconds", "clients can only /tactic every <g_tacticMilliseconds> milliseconds, -1 = disabled.", Cvar::NONE, 1000);

Cvar::Cvar<std::string> g_defaultLayouts("g_defaultLayouts", "layouts to pick randomly from each map", Cvar::NONE, "");
Cvar::Cvar<std::string> g_layouts("g_layouts", "layouts for next map (cleared after use)", Cvar::NONE, "");
Cvar::Cvar<bool> g_layoutAuto("g_layoutAuto", "pick arbitrary layout instead of builtin", Cvar::NONE, false);

Cvar::Cvar<bool> g_emoticonsAllowedInNames("g_emoticonsAllowedInNames", "allow [emoticon]s in player names", Cvar::NONE, true);
Cvar::Cvar<int> g_unnamedNumbering("g_unnamedNumbering", "FOR INTERNAL USE", Cvar::NONE, -1);
Cvar::Cvar<std::string> g_unnamedNamePrefix("g_unnamedNamePrefix", "prefix for auto-assigned player names", Cvar::NONE, UNNAMED_PLAYER "#");
Cvar::Cvar<std::string> g_unnamedBotNamePrefix("g_unnamedBotNamePrefix", "default name prefix for bots", Cvar::NONE, UNNAMED_BOT "#");

Cvar::Cvar<std::string> g_admin("g_admin", "admin levels file, relative to <homepath>/game/", Cvar::NONE, "admin.dat");
Cvar::Cvar<std::string> g_adminWarn("g_adminWarn", "duration of warning \"bans\"", Cvar::NONE, "1h");
Cvar::Cvar<std::string> g_adminTempBan("g_adminTempBan", "ban duration for kick and speclock", Cvar::NONE, "2m");
Cvar::Cvar<std::string> g_adminMaxBan("g_adminMaxBan", "maximum ban duration", Cvar::NONE, "2w");
Cvar::Range<Cvar::Cvar<int>> g_adminStealthMode("g_adminStealthMode", "default admin Stealth mode. 0 = disabled, 1 = use registered name, 2 = all admins are anonymous. overridden by giving admins or levels the STEALTH / SUPERSTEALTH flags", Cvar::NONE, 0, 0, 2);
Cvar::Cvar<std::string> g_adminStealthName("g_adminStealthName", "substitute name used for anonymous admins", Cvar::NONE, "^aan administrator");
Cvar::Cvar<bool> g_adminStealthConsole("g_adminStealthConsole", "console is anonymous", Cvar::NONE, false);
Cvar::Cvar<bool> g_adminRetainExpiredBans("g_adminRetainExpiredBans", "keep records of expired bans", Cvar::NONE, true);

Cvar::Cvar<bool> g_privateMessages("g_privateMessages", "allow private messages", Cvar::NONE, true);
Cvar::Cvar<bool> g_specChat("g_specChat", "allow public messages by spectators", Cvar::NONE, true);
Cvar::Cvar<bool> g_publicAdminMessages("g_publicAdminMessages", "allow non-admins to write to admin chat", Cvar::NONE, true);
Cvar::Cvar<bool> g_allowTeamOverlay("g_allowTeamOverlay", "provide teammate info for HUD", Cvar::NONE, true);

Cvar::Cvar<bool> g_showKillerHP("g_showKillerHP", "show killer's health to killed player", Cvar::NONE, false);
Cvar::Cvar<int> g_combatCooldown("g_combatCooldown", "team change disallowed until x seconds after combat", Cvar::NONE, 15);

Cvar::Range<Cvar::Cvar<int>> g_debugEntities("g_debugEntities", "entity debug level", Cvar::NONE, 0, -2, 3);

Cvar::Cvar<bool> g_instantBuilding("g_instantBuilding", "cheat mode for building", Cvar::NONE, false);
Cvar::Cvar<bool> g_ignoreNobuild("g_ignoreNobuild", "ignore nobuild area", Cvar::NONE, false);

Cvar::Cvar<int> g_emptyTeamsSkipMapTime("g_emptyTeamsSkipMapTime", "end game over x minutes if no real players", Cvar::NONE, 0);

Cvar::Cvar<bool>   g_neverEnd("g_neverEnd", "cheat to never end a game, helpful to load a map without spawn for testing purpose", Cvar::NONE, false);

Cvar::Cvar<float>  g_evolveAroundHumans("g_evolveAroundHumans", "Ratio of alien buildings to human entities that always allow evolution", Cvar::NONE, 1.5f);
Cvar::Cvar<float>  g_devolveMaxBaseDistance("g_devolveMaxBaseDistance", "Max Overmind distance to allow devolving", Cvar::SERVERINFO, 1000.0f);

Cvar::Cvar<bool>   g_autoPause("g_autoPause", "pause empty server", Cvar::NONE, false);

Cvar::Cvar<int> g_maxMiners("g_maxMiners", "set maximum number of miners per team. -1 = disabled.", Cvar::NONE, -1);

// <bot stuff>

// bot buy cvars
Cvar::Cvar<bool> g_bot_buy("g_bot_buy", "whether bots use the Armoury", Cvar::NONE, true);
// human weapons
Cvar::Cvar<bool> g_bot_ckit("g_bot_ckit", "whether bots use the Construction Kit (only current use is for repairs)", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_rifle("g_bot_rifle", "whether bots use the SMG", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_painsaw("g_bot_painsaw", "whether bots buy the Painsaw", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_shotgun("g_bot_shotgun", "whether bots buy the Shotgun", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_lasgun("g_bot_lasgun", "whether bots buy the Lasgun", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_mdriver("g_bot_mdriver", "whether bots buy the Mass Driver", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_chaingun("g_bot_chain", "whether bots buy the Chaingun", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_prifle("g_bot_prifle", "whether bots buy the Pulse Rifle", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_flamer("g_bot_flamer", "whether bots buy the Flamethrower", Cvar::NONE, false);
Cvar::Cvar<bool> g_bot_lcannon("g_bot_lcannon", "whether bots buy the Lucifer Cannon", Cvar::NONE, true);
// human armors
Cvar::Cvar<bool> g_bot_battlesuit("g_bot_battlesuit", "whether bots buy the Battlesuit", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_mediumarmour("g_bot_mediumarmour", "whether bots buy Medium Armour", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_lightarmour("g_bot_lightarmour", "whether bots buy Light Armour", Cvar::NONE, true);
// human upgrades
Cvar::Cvar<bool> g_bot_radar("g_bot_radar", "whether bots buy the Radar", Cvar::NONE, true);
// bots won't buy radars if more than this percent allies already have it
Cvar::Cvar<int> g_bot_radarRatio("g_bot_radarRatio", "bots target x% of team owning radar", Cvar::NONE, 75);
Cvar::Cvar<bool> g_bot_jetpack("g_bot_jetpack", "whether bots buy the Jetpack", Cvar::NONE, false);
Cvar::Cvar<bool> g_bot_grenade("g_bot_grenade", "whether bots buy the Grenade", Cvar::NONE, false);
Cvar::Cvar<bool> g_bot_firebomb("g_bot_firebomb", "whether bots buy the Firebomb", Cvar::NONE, false);

// bot evolution cvars
Cvar::Cvar<bool> g_bot_evolve("g_bot_evolve", "whether bots can evolve", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_builder("g_bot_builder", "whether bots use non-advanced Granger", Cvar::NONE, false);
Cvar::Cvar<bool> g_bot_builderupg("g_bot_builderupg", "whether bots use Advanced Granger", Cvar::NONE, false);
Cvar::Cvar<bool> g_bot_level0("g_bot_level0", "whether bots use Dretch", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_level1("g_bot_level1", "whether bots use Mantis", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_level2("g_bot_level2", "whether bots use non-advanced Marauder", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_level2upg("g_bot_level2upg", "whether bots use Advanced Marauder", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_level3("g_bot_level3", "whether bots use non-advanced Dragoon", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_level3upg("g_bot_level3upg", "whether bots use Advanced Dragoon", Cvar::NONE, true);
Cvar::Cvar<bool> g_bot_level4("g_bot_level4", "whether bots use Tyrant", Cvar::NONE, true);

// misc bot cvars
Cvar::Cvar<bool> g_bot_attackStruct("g_bot_attackStruct", "whether bots target buildables", Cvar::NONE, true);
Cvar::Cvar<float> g_bot_fov("g_bot_fov", "bots' \"field of view\"", Cvar::NONE, 125);
Cvar::Cvar<int> g_bot_chasetime("g_bot_chasetime", "bots stop chasing after x ms out of sight", Cvar::NONE, 5000);
Cvar::Cvar<int> g_bot_reactiontime("g_bot_reactiontime", "bots' reaction time to enemies (milliseconds)", Cvar::NONE, 500);
Cvar::Cvar<bool> g_bot_infiniteFunds("g_bot_infiniteFunds", "give bots unlimited funds", Cvar::NONE, false);
Cvar::Cvar<bool> g_bot_infiniteMomentum("g_bot_infiniteMomentum", "allow bots to ignore momentum, but not other restrictions", Cvar::NONE, false);
Cvar::Cvar<int> g_bot_aliensenseRange("g_bot_aliensenseRange", "custom aliensense range for bots", Cvar::NONE, ALIENSENSE_RANGE);

//</bot stuff>

static Cvar::Cvar<std::string> gamename("gamename", "game/mod identifier", Cvar::SERVERINFO | Cvar::ROM, GAME_VERSION);
static Cvar::Cvar<std::string> gamedate("gamedate", "date the sgame was compiled", Cvar::ROM, __DATE__);

void               CheckExitRules();
static void        G_LogGameplayStats( int state );

// state field of G_LogGameplayStats
enum
{
	LOG_GAMEPLAY_STATS_HEADER,
	LOG_GAMEPLAY_STATS_BODY,
	LOG_GAMEPLAY_STATS_FOOTER
};

/*
================
G_FindEntityGroups

Chain together all entities with a matching groupName field.
Entity groups are used for item groups and multi-entity mover groups.

All but the first will have the FL_GROUPSLAVE flag set and groupMaster field set
All but the last will have the groupChain field set to the next one
================
*/
static void G_FindEntityGroups()
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

	Log::Notice( "%i groups with %i entities", groupCount, entityCount );
}

/*
================
G_InitSetEntities
goes through all entities and concludes the spawn
by calling their reset function as initiation if available
================
*/
static void G_InitSetEntities()
{
	int i;
	gentity_t *entity;

	for ( i = MAX_CLIENTS, entity = g_entities + i; i < level.num_entities; i++, entity++ )
	{
		if(entity->inuse && entity->reset)
			entity->reset( entity );
	}
}

/*
=================
G_MapConfigs
=================
*/
void G_MapConfigs( Str::StringRef mapname, Str::StringRef layout )
{
	if ( g_mapConfigs.Get().empty() )
	{
		return;
	}
	std::string resolvedLayout = layout.empty() ? "builtin" : layout;
	Log::Notice( "Loading map configs for map: %s layout: %s", mapname, resolvedLayout );

	std::array<std::string, 3> scriptSuffixes = {
		// Run for every map.
		"default.cfg",
		// Run for every layout of a particular map.
		Str::Format( "%s.cfg", mapname ),
		//  Run for only for a specific map and layout.
		FS::Path::Build( mapname, Str::Format( "%s.cfg", resolvedLayout ) ),
	};

	for ( const auto& suffix : scriptSuffixes )
	{
		// NOTE: We cannot check for the existence of this file since the
		// sgame can only access fs_homepath/game, but exec only works relative
		// to fs_homepath/config.
		std::string script = FS::Path::Build( g_mapConfigs.Get(), suffix );
		// -f so that the users are not spammed for non-existing configs.
		trap_SendConsoleCommand( ("exec -f " + Cmd::Escape( script )).c_str() );
	}

	trap_SendConsoleCommand( "maprestarted" );
}

/*
============
G_InitGame

============
*/
void G_InitGame( int levelTime, int randomSeed, bool inClient )
{
	srand( randomSeed );

	Log::Notice( "------- Game Initialization -------" );
	Log::Notice( "gamename: %s", GAME_VERSION );
	Log::Notice( "gamedate: %s", __DATE__ );

	// set some level globals
	level.~level_locals_t();
	new (&level) level_locals_t();
	level.time = levelTime;
	level.inClient = inClient;
	level.startTime = levelTime;
	level.snd_fry = G_SoundIndex( "sound/misc/fry" );  // FIXME standing in lava / slime

	// TODO: Move this in a seperate function
	if ( !g_logFile.Get().empty() )
	{
		if ( g_logFileSync.Get() )
		{
			trap_FS_FOpenFile( g_logFile.Get().c_str(), &level.logFile, fsMode_t::FS_APPEND_SYNC );
		}
		else
		{
			trap_FS_FOpenFile( g_logFile.Get().c_str(), &level.logFile, fsMode_t::FS_APPEND );
		}

		if ( !level.logFile )
		{
			Log::Warn("Couldn't open logfile: %s", g_logFile.Get() );
		}
		else
		{
			char    serverinfo[ MAX_INFO_STRING ];
			qtime_t qt;

			trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

			G_LogPrintf( "------------------------------------------------------------" );
			G_LogPrintf( "InitGame: %s", serverinfo );

			Com_GMTime( &qt );
			G_LogPrintf( "RealTime: %04i-%02i-%02i %02i:%02i:%02i Z",
			             1900 + qt.tm_year, qt.tm_mon + 1, qt.tm_mday,
			             qt.tm_hour, qt.tm_min, qt.tm_sec );
		}
	}
	else
	{
		Log::Notice( "Not logging to disk" );
	}

	// gameplay statistics logging
	// TODO: Move this in a seperate function
	if ( g_logGameplayStatsFrequency.Get() > 0 )
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

		trap_FS_FOpenFile( logfile, &level.logGameplayFile, fsMode_t::FS_WRITE );

		if ( !level.logGameplayFile )
		{
			Log::Warn("Couldn't open gameplay statistics logfile: %s", logfile );
		}
		else
		{
			G_LogGameplayStats( LOG_GAMEPLAY_STATS_HEADER );
		}
	}

	// clear this now; it'll be set, if needed, from rotation
	g_mapStartupMessage.Set("");

	// load config files
	BG_InitAllConfigs();

	G_RegisterCommands();
	G_admin_readconfig( nullptr );

	// initialize all entities for this game
	for ( int i=0; i < MAX_GENTITIES; i++ )
	{
		g_entities[i] = {};
	}
	level.gentities = g_entities;

	// initialize special entities
	G_InitGentityMinimal( g_entities + ENTITYNUM_NONE );
	G_InitGentityMinimal( g_entities + ENTITYNUM_WORLD );

	// initialize all clients for this game
	if (!Cvar::ParseCvarValue(Cvar::GetValue("sv_maxclients"), level.maxclients))
	{
		ASSERT_UNREACHABLE();
	}
	memset( g_clients, 0, MAX_CLIENTS * sizeof( g_clients[ 0 ] ) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( int i = 0; i < level.maxclients; i++ )
	{
		g_entities[ i ].client = level.clients + i;
	}

	// always leave room for the max number of clients, even if they aren't all used, so numbers
	// inside that range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	for( int i = 0; i < MAX_CLIENTS; i++ )
	{
		g_entities[ i ].classname = "clientslot";
	}

	// let the server system know where the entites are
	trap_LocateGameData( level.num_entities, sizeof( gentity_t ),
	                     sizeof( level.clients[ 0 ] ) );

	BG_LoadEmoticons();

	trap_SetConfigstring( CS_INTERMISSION, "0" );

	// test to see if a custom buildable layout will be loaded
	G_LayoutSelect();

	BotAssertionInit();

	// retrieve map name and layout to load configs.
	{
		std::string map = Cvar::GetValue( "mapname");
		std::string layout = Cvar::GetValue( "layout" );
		G_MapConfigs( map, layout );
	}

	level.spawning = true;

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// add any fake entities
	G_SpawnFakeEntities();

	BaseClustering::Init();

	// load up a custom building layout if there is one
	G_LayoutLoad();

	// Initialize item locking state
	BG_InitUnlockackables();

	G_FindEntityGroups();
	G_InitSetEntities();

	G_CheckPmoveParamChanges();

	G_InitDamageLocations();

	G_InitMapRotations();

	G_InitSpawnQueue( &level.team[ TEAM_ALIENS ].spawnQueue );
	G_InitSpawnQueue( &level.team[ TEAM_HUMANS ].spawnQueue );

	G_InitSkilltreeCvars();

	if ( g_debugMapRotation.Get() )
	{
		G_PrintRotations();
	}

	level.voices = BG_VoiceInit();
	BG_PrintVoices( level.voices, g_debugVoices.Get() );

	// Spend build points for layout buildables.
	for (team_t team = TEAM_NONE; (team = G_IterateTeams(team)); ) {
		G_SpendBudget(team, level.team[team].layoutBuildPoints);
	}

	Log::Notice( "-----------------------------------" );

	G_UpdateTeamConfigStrings();

	G_MapLog_NewMap();

	if ( g_lockTeamsAtStart.Get() )
	{
		level.team[ TEAM_ALIENS ].locked = true;
		level.team[ TEAM_HUMANS ].locked = true;
		g_lockTeamsAtStart.Set(false);
	}

	G_notify_sensor_start();

	// Initialize build point counts for the intial layout.
	G_UpdateBuildPointBudgets();
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
void G_ShutdownGame( int /* restart */ )
{
	// in case of a map_restart
	G_ClearVotes( true );

	Log::Notice( "==== ShutdownGame ====" );

	if ( level.logFile )
	{
		G_LogPrintf( "ShutdownGame:" );
		G_LogPrintf( "------------------------------------------------------------" );
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
	G_BotCleanup();
	G_namelog_cleanup();

	G_UnregisterCommands();

	G_ShutdownMapRotations();
	BG_UnloadAllConfigs();

	level.restarted = false;
	level.surrenderTeam = TEAM_NONE;
	trap_SetConfigstring( CS_WINNER, "" );

	/*
	 * delete cbse entities attached to gentities
	 */
	for (int i = 0; i < level.num_entities; i++) {
		delete level.gentities[i].entity;
	}

	Parse_FreeGlobalDefines();
}

//===================================================================

void G_CheckPmoveParamChanges() {
	if(not level.pmoveParams.initialized or
			level.pmoveParams.synchronous != g_synchronousClients.Get() or
			level.pmoveParams.msec != pmove_msec.Get() or
			level.pmoveParams.fixed != pmove_fixed.Get() or
			level.pmoveParams.accurate != pmove_accurate.Get()) {
		level.pmoveParams.initialized = true;
		level.pmoveParams.synchronous = g_synchronousClients.Get();
		level.pmoveParams.msec = pmove_msec.Get();
		level.pmoveParams.fixed = pmove_fixed.Get();
		level.pmoveParams.accurate = pmove_accurate.Get();
		G_SendClientPmoveParams(-1);
	}
}
void G_SendClientPmoveParams(int client) {
	trap_SendServerCommand(client, va("pmove_params %i %i %i %i",
		level.pmoveParams.synchronous,
		+level.pmoveParams.fixed,
		level.pmoveParams.msec,
		+level.pmoveParams.accurate));
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
static int SortRanks( const void *a, const void *b )
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
		g_clients[ clientNum ].ps.pm_flags &= ~PMF_QUEUED;

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

	g_clients[ clientNum ].ps.pm_flags |= PMF_QUEUED;
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
				g_clients[ clientNum ].ps.pm_flags &= ~PMF_QUEUED;

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

	Log::Notice( "l:%d f:%d b:%d    :", length, sq->front, sq->back );

	if ( length > 0 )
	{
		do
		{
			if ( sq->clients[ i ] == -1 )
			{
				Log::Notice( "*:" );
			}
			else
			{
				Log::Notice( "%d:", sq->clients[ i ] );
			}

			i = QUEUE_PLUS1( i );
		}
		while ( i != QUEUE_PLUS1( sq->back ) );
	}

	Log::Notice( "" );
}

/*
============
G_SpawnClients

Spawn queued clients
============
*/
static void G_SpawnClients( team_t team )
{
	int          clientNum;
	gentity_t    *ent, *spawn;
	vec3_t       spawn_origin, spawn_angles;
	spawnQueue_t *sq = nullptr;
	int          numSpawns = 0;

	ASSERT( G_IsPlayableTeam( team ) );
	sq = &level.team[ team ].spawnQueue;

	numSpawns = level.team[ team ].numSpawns;

	if ( G_GetSpawnQueueLength( sq ) > 0 && numSpawns > 0 )
	{
		clientNum = G_PeekSpawnQueue( sq );
		if ( clientNum < 0 )
		{
			return;
		}

		ent = &g_entities[ clientNum ];

		if ( ( spawn = G_SelectUnvanquishedSpawnPoint( team,
		               ent->client->pers.lastDeathLocation,
		               spawn_origin, spawn_angles ) ) )
		{
			if ( ent->r.svFlags & SVF_BOT )
			{
				G_BotSelectSpawnClass( ent );
			}

			G_PopSpawnQueue( sq );
			ent->client->sess.spectatorState = SPECTATOR_NOT;
			ClientUserinfoChanged( clientNum, false );
			ClientSpawn( ent, spawn, spawn_origin, spawn_angles );
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
static void G_CalculateAvgPlayers()
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
static Cvar::Cvar<std::string> slotTeams("P", "[serverinfo] client slot -> team", Cvar::SERVERINFO, "");
static Cvar::Cvar<std::string> slotBots("B", "[serverinfo] client slot -> is bot", Cvar::SERVERINFO, "");
void CalculateRanks()
{
	int  clientNum;
	int  team;
	char P[ MAX_CLIENTS + 1 ] = "", B[ MAX_CLIENTS + 1 ] = "";

	level.numConnectedClients = 0;
	level.numConnectedPlayers = 0;
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
			bool bot = level.gentities[ clientNum ].r.svFlags & SVF_BOT;

			level.sortedClients[ level.numConnectedClients ] = clientNum;

			team = level.clients[ clientNum ].pers.team;
			P[ clientNum ] = ( char ) '0' + team;

			level.numConnectedClients++;
			level.team[ team ].numClients++;

			if ( bot )
			{
				if ( navMeshLoaded == navMeshStatus_t::GENERATING )
				{
					B[ clientNum ] = 'G';
				}
				else
				{
					int skill = level.gentities[ clientNum ].botMind->skillLevel;

					// Bot skill can be 0 while spawning, just before skill is set.
					ASSERT( skill >= 0 && skill <= 9 );

					B[ clientNum ] = '0' + (char) skill;
				}

				level.team[ team ].numBots++;
			}
			else
			{
				level.team[ team ].numPlayers++;
				level.numConnectedPlayers++;
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
	slotTeams.Set(P);

	B[ clientNum ] = '\0';
	slotBots.Set(B);

	qsort( level.sortedClients, level.numConnectedClients,
	       sizeof( level.sortedClients[ 0 ] ), SortRanks );

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
	ent->s.eType = entityType_t::ET_GENERAL;
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
		if ( Entities::IsDead( client ) )
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
static void ExitLevel()
{
	int       i;
	gclient_t *cl;
	char currentMapName[ MAX_STRING_CHARS ];

	trap_Cvar_VariableStringBuffer( "mapname", currentMapName, sizeof( currentMapName ) );

	// Restart if map is the same
	if ( !Q_stricmp( currentMapName, g_nextMap.Get().c_str() ) )
	{
		g_layouts.Set( g_nextMapLayouts.Get() );
		trap_SendConsoleCommand( "map_restart" );
	}
	else if ( G_MapExists( g_nextMap.Get().c_str() ) )
	{
		trap_SendConsoleCommand( Str::Format( "%s %s %s", G_NextMapCommand(), Cmd::Escape( g_nextMap.Get() ), Cmd::Escape( g_nextMapLayouts.Get() ) ).c_str() );
	}
	else if ( G_MapRotationActive() )
	{
		G_AdvanceMapRotation( 0 );
	}
	else
	{
		trap_SendConsoleCommand( "map_restart" );
	}

	g_nextMap.Set("");

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

	Com_sprintf( string, sizeof( string ), "chat %d %d %s",
	             ent ? ent->num() : -1,
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
	G_LogPrintf( "%s: %d \"%s^*\": ^6%s",
	             G_admin_permission( ent, ADMF_ADMINCHAT ) ? "AdminMsg" : "AdminMsgPublic",
	             ent ? ent->num() : -1, ent ? ent->client->pers.netname : "console",
	             msg );
}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open, and to the server console.
Will append a newline for you.
=================
*/
void PRINTF_LIKE(1) G_LogPrintf( const char *fmt, ... )
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
		Log::Notice( decolored + 7 );
	}

	if ( !level.logFile )
	{
		return;
	}

	Color::StripColors( string, decolored, sizeof( decolored ) );
	trap_FS_Write( decolored, strlen( decolored ), level.logFile );
    trap_FS_Write( "\n", 1, level.logFile );
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
	gclient_t *client;
	int       team;

	for ( team = TEAM_ALIENS ; team < NUM_TEAMS ; ++team)
	{
		teamCnt[ team ] = 0;
		teamCredits[ team ] = 0;
		teamValue[ team ] = 0;
	}

	for ( playerNum = 0; playerNum < level.maxclients; playerNum++ )
	{
		client = &g_clients[ playerNum ];

		if ( !client->ent()->inuse ) continue;

		team = client->pers.team;

		teamCredits[ team ] += client->pers.credit;
		teamValue[ team ] += BG_GetPlayerValue( client->ps );
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
			             "# g_momentumHalfLife:        %4g\n"
			             "# g_initialBuildPoints:      %4i\n"
			             "# g_budgetPerMiner:          %4i\n"
			             "#\n"
			             "#  1  2  3    4    5    6    7    8    9   10   11   12   13   14   15   16\n"
			             "#  T #A #H AMom HMom ---- ATBP HTBP AUBP HUBP ABRV HBRV ACre HCre AVal HVal\n"
			             "# -------------------------------------------------------------------------\n",
			             Q3_VERSION,
			             mapname,
			             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
			             t.tm_hour, t.tm_min, t.tm_sec,
			             LOG_GAMEPLAY_STATS_VERSION,
			             g_momentumHalfLife.Get(),
			             g_buildPointInitialBudget.Get(),
			             g_buildPointBudgetPerMiner.Get() );

			break;
		}
		case LOG_GAMEPLAY_STATS_BODY:
		{
			int    time;
			int    XXX;
			int    team;
			int    num[ NUM_TEAMS ];
			int    Mom[ NUM_TEAMS ];
			int    TBP[ NUM_TEAMS ];
			int    UBP[ NUM_TEAMS ];
			int    BRV[ NUM_TEAMS ];
			int    Cre[ NUM_TEAMS ];
			int    Val[ NUM_TEAMS ];

			if ( level.time < nextCalculation )
			{
				return;
			}

			time = level.matchTime / 1000;
			XXX  = 0;

			for( team = TEAM_NONE + 1; team < NUM_TEAMS; team++ )
			{
				num[ team ] = level.team[ team ].numClients;
				Mom[ team ] = ( int )level.team[ team ].momentum;
				TBP[ team ] = ( int )level.team[ team ].totalBudget;
				UBP[ team ] = ( int )G_GetFreeBudget( ( team_t )team );
			}

			G_GetTotalBuildableValues( BRV );
			GetAverageCredits( Cre, Val );

			Com_sprintf( logline, sizeof( logline ),
			             "%4i %2i %2i %4i %4i %4i %4i %4i %4i %4i %4i %4i %4i %4i %4i %4i\n",
			             time, num[ TEAM_ALIENS ], num[ TEAM_HUMANS ], Mom[ TEAM_ALIENS ], Mom[ TEAM_HUMANS ],
			             XXX, TBP[ TEAM_ALIENS ], TBP[ TEAM_HUMANS ], UBP[ TEAM_ALIENS ], UBP[ TEAM_HUMANS ],
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
		nextCalculation = level.time + std::max( 1, g_logGameplayStatsFrequency.Get() ) * 1000;
	}
	else
	{
		nextCalculation = 0;
	}
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

	G_LogPrintf( "Exit: %s", string );

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

		G_LogPrintf( "score: %i  ping: %i  client: %i %s",
		             cl->ps.persistant[ PERS_SCORE ], ping, level.sortedClients[ i ],
		             cl->pers.netname );
	}
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
static void CheckIntermissionExit()
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
	memset( &readyMasks, 0, sizeof( readyMasks ) );

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
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
void CheckExitRules()
{
	if ( g_cheats && g_neverEnd.Get() ) {
		return;
	}

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
			trap_SendServerCommand( -1, "print_tr " QQ( N_("Timelimit hit") ) );
			trap_SetConfigstring( CS_WINNER, "Stalemate" );
			G_notify_sensor_end( TEAM_NONE );
			LogExit( "Timelimit hit." );
			G_MapLog_Result( 't' );
			return;
		}
		else if ( level.matchTime >= ( level.timelimit - 5 ) * 60000 &&
		          level.timelimitWarning < TW_IMMINENT )
		{
			trap_SendServerCommand( -1, va( "cp_tr %s %d",
				QQ( N_("$1$ minutes remaining!" ) ),
				5 ) );
			level.timelimitWarning = TW_IMMINENT;
		}
		else if ( level.matchTime >= ( level.timelimit - 1 ) * 60000 &&
		          level.timelimitWarning < TW_PASSED )
		{
			trap_SendServerCommand( -1, "cp_tr "
				QQ( N_("1 minute remaining!") ) );
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
		trap_SendServerCommand( -1, "print_tr " QQ( N_("Humans win") ) );
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
		trap_SendServerCommand( -1, "print_tr " QQ( N_("Aliens win") ) );
		trap_SetConfigstring( CS_WINNER, "Aliens Win" );
		G_notify_sensor_end( TEAM_ALIENS );
		LogExit( "Aliens win." );
		G_MapLog_Result( 'a' );
	}
	else if ( g_emptyTeamsSkipMapTime.Get() &&
		( level.time - level.startTime ) / 60000 >=
		g_emptyTeamsSkipMapTime.Get() &&
		level.team[ TEAM_ALIENS ].numPlayers == 0 && level.team[ TEAM_HUMANS ].numPlayers == 0 )
	{
		// nobody wins because the teams are empty after x amount of game time
		level.lastWin = TEAM_NONE;
		trap_SendServerCommand( -1, "print_tr "
			QQ( N_("Empty teams skip map time exceeded.") ) );
		trap_SetConfigstring( CS_WINNER, "Stalemate" );
		LogExit( "Timelimit hit." );
		G_MapLog_Result( 't' );
	}
}

/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/

static void SetNeedpass(const std::string& password)
{
	g_needpass.Set( !password.empty() && !Str::IsIEqual( password, "none" ) );
}
Cvar::Callback<Cvar::Cvar<std::string>> g_password("password", "password to join the server", Cvar::NONE, "", SetNeedpass);


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
	if (auto* thinkingComponent = ent->entity->Get<ThinkingComponent>()) {
		thinkingComponent->Think();
	}

	// Do legacy thinking.
	// TODO: Replace this kind of thinking entirely with CBSE.
	if (ent->think) {
		float thinktime = ent->nextthink;
		if (thinktime <= 0 || thinktime > level.time) return;
		ent->nextthink = 0;
		ent->think(ent);
	}

	if ( !ent->inuse )
		return;

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
static void G_RunAct( gentity_t *entity )
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
static void G_EvaluateAcceleration( gentity_t *ent, int msec )
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

	if ( !level.numConnectedPlayers && g_autoPause.Get() && level.matchTime > 1000)
	{
		level.time = levelTime;
		level.matchTime = levelTime - level.startTime;

		CheckExitRules();

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
			trap_SendServerCommand( -1, "cp_tr "
				QQ( N_("The game has been paused. Please wait.") ) );

			if ( level.pausedTime >= 110000  && level.pausedTime <= 119000 )
			{
				trap_SendServerCommand( -1, va( "print_tr %s %d", QQ( N_("Server: Game will auto-unpause in $1$ seconds") ),
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

		// We may de-hardcode this value.
		if ( level.pausedTime > 120000 )
		{
			// FIXME: This assumes plural.
			trap_SendServerCommand( -1, va( "print_tr_p %s %d",
				QQ( N_("Server: The game has been unpaused automatically ($1$ minutes max)") ),
				2 ) );
			trap_SendServerCommand( -1, "cp_tr "
				QQ( N_("The game has been unpaused!") ) );
			level.pausedTime = 0;
		}

		return;
	}

	level.previousTime = level.time;
	level.time = levelTime;
	level.matchTime = levelTime - level.startTime;

	msec = level.time - level.previousTime;

	// generate public-key messages
	G_admin_pubkey();

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
		}

		// temporary entities or ones about to be removed don't think
		if ( ent->freeAfterEvent ) continue;

		// calculate the acceleration of this entity
		if ( ent->evaluateAcceleration ) G_EvaluateAcceleration( ent, msec );

		// think/run entity by type
		switch ( ent->s.eType )
		{
			case entityType_t::ET_MISSILE:
				G_RunMissile( ent );
				continue;

			case entityType_t::ET_BUILDABLE:
				// TODO: Do buildables make any use of G_Physics' functionality apart from the call
				//       to G_RunThink?
				G_Physics( ent );
				continue;

			case entityType_t::ET_CORPSE:
				G_Physics( ent );
				continue;

			case entityType_t::ET_MOVER:
				G_RunMover( ent );
				continue;

			default:
				if ( ent->physicsObject )
				{
					G_Physics( ent );
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

	// ThinkingComponent should have been called already but who knows maybe we forgot some.
	ForEntities<ThinkingComponent>([](Entity& entity, ThinkingComponent& thinkingComponent) {
		// A newly created entity can randomly run things, or not, in the above loop over
		// entities depending on whether it was added in a hole in g_entities or at the end, so
		// ignore the entity if it was created this frame.
		if (entity.oldEnt->creationTime != level.time && thinkingComponent.GetLastThinkTime() != level.time
			&& !entity.oldEnt->freeAfterEvent) {
			Log::Warn("ThinkingComponent was not called");
			thinkingComponent.Think();
		}
	});

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

	// Check if a build point can be removed from the queue.
	G_RecoverBuildPoints();

	// Power down buildables if there is a budget deficit.
	G_UpdateBuildablePowerStates();

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

	G_BotBackgroundNavgen();
	G_BotFill( false );

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	for ( i = 0; i < NUM_TEAMS; i++ )
	{
		G_CheckVote( (team_t) i );
	}

	BotDebugDrawMesh();
	G_BotUpdateObstacles();
}

void G_PrepareEntityNetCode() {
	// TODO: Allow ForEntities with empty template arguments.
	gentity_t *oldEnt = &g_entities[0];
	// Prepare netcode for all non-specs first.
	for (int i = 0; i < level.num_entities; i++, oldEnt++) {
		if (oldEnt->entity) {
			if (oldEnt->entity->Get<SpectatorComponent>()) {
				continue;
			}
			oldEnt->entity->PrepareNetCode();
		}
	}

	// Prepare netcode for specs
	ForEntities<SpectatorComponent>([&](Entity& entity, SpectatorComponent&){
		entity.PrepareNetCode();
	});
}

Str::StringRef G_NextMapCommand()
{
	if ( g_cheats && g_persistDevmap.Get() )
	{
		return "devmap";
	}

	return "map";
}
