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

#ifndef SG_STRUCT_H_
#define SG_STRUCT_H_

#include "sg_entities.h"
#include "sg_map_entity.h"

struct botMemory_t;

#define MAX_NAMELOG_NAMES 5
#define MAX_NAMELOG_ADDRS 5

using unnamed_t = signed int;
struct namelog_t
{
	namelog_t *next;

	char   name[ MAX_NAMELOG_NAMES ][ MAX_NAME_LENGTH ];
	addr_t ip[ MAX_NAMELOG_ADDRS ];
	char   guid[ 33 ];
	int    slot;
	bool   banned;

	int nameOffset;
	int nameChangeTime;
	int nameChanges;
	int voteCount;

	unnamed_t unnamedNumber;

	bool muted;
	bool denyBuild;

	int    score;
	int    credits;
	team_t team;

	int id;
};

class Entity;

// Replacement for gentity_t* that can detect the case where an entity has been recycled.
// operator bool checks that the entity is non-null and has not been freed since the reference
// was formed.
// For a player entity, we increment the generation number when it spawns or dies. It does
// not increment upon evolve/armor change, although the CBSE entity is reallocated in that case.
template<typename T>
struct GentityRef_impl
{
	T entity;
	unsigned generation;

	GentityRef_impl() = default; // uninitialized!
	GentityRef_impl(T ent) { *this = ent; }

	GentityRef_impl<T>& operator=(T ent) {
		entity = ent;
		if (ent) {
			generation = ent->generation;
		}
		return *this;
	}

	operator bool() const {
		return entity != nullptr && entity->generation == generation;
	}

	T get() const {
		if (!*this) {
			return nullptr;
		}
		return entity;
	}

	T operator->() const {
		ASSERT(*this);
		return entity;
	}
};

extern gentity_t *g_entities;
extern gclient_t *g_clients;

using GentityRef = GentityRef_impl<gentity_t *>;
using GentityConstRef = GentityRef_impl<const gentity_t *>;

struct gentity_t
{
	entityState_t  s; // communicated by server to clients
	entityShared_t r; // shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	// New style entity
	Entity* entity;

	const char   *classname; //used by buildables & other spawned at start of map
	mapEntity_t mapEntity; // fields only used by BSP entities (not counting layout buildables)

	gclient_t *client; // nullptr if not a client

	unsigned generation; // used with GentityRef
	int          freetime; // level.time when the object was freed
	int          eventTime; // events will be cleared EVENT_VALID_MSEC after set
	bool     inuse;
	bool     freeAfterEvent;

	int          flags; // FL_* variables

	//entity creation time, i.e. when a building was build or a missile was fired (for diminishing missile damage)
	int          creationTime;

	// These formerly all used a field named "active" and are now split such that it gets easier
	// to convert them to CBSE component members.
	bool medistationIsHealing;
	bool bodyStartedSinking;
	bool shaderActive;

	/**
	 * delay being really active until this time, e.g for act() delaying or for spinup for norfenturrets
	 * this will most probably be set by think() before act()ing, probably by using the config.delay time
	 */
	int          nextAct;
	/**
	 * Fulfill the main task of this entity.
	 * act, therefore also become active,
	 * but only if enabled
	 */
	void ( *act )( gentity_t *self, gentity_t *caller, gentity_t *activator );

	/**
	 * is the entity able to become active?
	 * e.g. used for buildables to indicate being usable or a stationary weapon being "live"
	 * or for sensors to indicate being able to sense other entities and fire events
	 *
	 * as a reasonable assumption we default to entities being enabled directly after they are spawned,
	 * since most of the time we want them to be
	 */
	bool     enabled;

	/**
	 * for entities taking a longer time to spawn,
	 * this boolean indicates when this spawn process was finished
	 * this can e.g. be indicated by an animation
	 */
	bool     spawned;
	gentity_t    *parent; // the gentity that spawned this one

	/**
	 * is the buildable getting support by reactor or overmind?
	 * this is tightly coupled with enabled
	 * but buildables might be disabled independently of rc/om support
	 * unpowered buildables are expected to be disabled though
	 * other entities might also consider the powergrid for behavior changes
	 */
	bool     powered;

	/**
	 * The amount of momentum this building generated on construction
	 */
	float        momentumEarned;

	GentityRef   target; // target of trapper, medistation, hive, rocketpod, builder's +deconstruct

	/* path chaining, not unlike the target/tracker relationship */
	gentity_t    *nextPathSegment;

	/**
	 * current valid call state for a single threaded call hierarchy.
	 * this allows us to lookup the current callIn,
	 * walk back further the hierarchy and even do simply loop detection
	 */
	gentityCall_t callIn;
	gentity_t    *activator; //FIXME: handle this as part of the current Call

	bool physicsObject; // if true, it can be pushed by movers and fall off edges
	// all game items are physicsObjects,
	float    physicsBounce; // 1.0 = continuous bounce, 0.0 = no bounce
	int      clipmask; // brushes with this content value will be collided against
	// when moving.  items and corpses do not collide against
	// players, for instance

	int          health;

	// acceleration evaluation
	bool  evaluateAcceleration;
	vec3_t    oldVelocity;
	vec3_t    acceleration;
	vec3_t    oldAccel;
	vec3_t    jerk;

	int       nextthink;
	void ( *think )( gentity_t *self );
	void ( *reset )( gentity_t *self );
	void ( *touch )( gentity_t *self, gentity_t *other, trace_t *trace );
	void ( *use )( gentity_t *self, gentity_t *other, gentity_t *activator );
	void ( *pain )( gentity_t *self, gentity_t *attacker, int damage );
	void ( *die )( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod );

	// Parameters for G_RadiusDamage. Mostly used for missiles.
	// splashDamage is the maximum damage which can be done by a splash hit.
	// The damage decrease linearly with distance, from splashDamage at distance 0, to 0 at distance
	// splashRadius, where distance is defined as the distance from the splash origin to the
	// targeted entity's bounding box.
	// An entity directly hit by a missile is exempt from splash damage.
	int       splashDamage;
	int       splashRadius;

	int       watertype;
	// from -inf to 3, apparently.
	// * 0 and less: air/ground
	// * 1: touch water I guess (toe in water)
	// * 2: standing water, whater that means
	// * 3: completely (eyes) underwater (not sure if this triggers swimming or if can breath)
	int       waterlevel;

	/*
	 * variables that got randomly semantically abused by everyone
	 * so now we rather name them to indicate the fact, that we cannot imply any meaning by the name
	 *
	 * please try not to use them for new things if you can avoid it
	 * but prefer them over semantically abusing other variables if you are into that sort of thing
	 */
	int       damage;
	int       customNumber;


	team_t      buildableTeam; // buildable item team
	namelog_t   *builtBy; // clientNum of person that built this

	int         pain_debounce_time;
	int         timestamp; // body queue sinking, etc
	int         shrunkTime; // time when a barricade shrunk or zero
	gentity_t   *boosterUsed; // the booster an alien is using for healing
	int         boosterTime; // last time alien used a booster for healing
	int         healthSourceTime; // last time an alien had contact to a health source
	int         animTime; // last animation change
	float       barbRegeneration; // goon barb regeneration is complete if this value is >= 1

	bool        deconMarkHack; // TODO: Remove.
	int         nextPhysicsTime; // buildables don't need to check what they're sitting on
	// every single frame.. so only do it periodically
	int         clientSpawnTime; // the time until this spawn can spawn a client

	struct {
	 	float  value;
		int    time;
		team_t team;
	}           credits[ MAX_CLIENTS ];

	int         killedBy; // clientNum of killer

	// turret
	float       turretCurrentDamage;

	vec4_t      animation; // animated map objects

	bool    nonSegModel; // this entity uses a nonsegmented player model

	int         suicideTime; // when the client will suicide

	int         lastDamageTime;
	int         nextRegenTime;

	bool    pointAgainstWorld; // don't use the bbox for map collisions

	botMemory_t *botMind;

	gentity_t   *alienTag, *humanTag;
	gentity_t   **tagAttachment;
	int         tagScore;
	int         tagScoreTime;

	// gives the entityNum
	int num() const {
		ASSERT(this - g_entities >= 0);
		ASSERT(this - g_entities < MAX_GENTITIES);
		return this - g_entities;
	}

	bool Damage( float amount, gentity_t* source,
		Util::optional<glm::vec3> location,
		Util::optional<glm::vec3> direction,
		int flags, meansOfDeath_t meansOfDeath );
};

/**
 * client data that stays across multiple levels or map restarts
 * this is achieved by writing all the data to cvar strings at game shutdown
 * time and reading them back at connection time.  Anything added here
 * MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
 */
struct clientSession_t
{
	spectatorState_t spectatorState;
	int              spectatorClient; // for chasecam and follow mode
	team_t           restartTeam; //for !restart keepteams and !restart switchteams
	int              botSkill;
	char             botTree[ MAX_QPATH ];
	clientList_t     ignoreList;
	int              seenWelcome; // determines if the client has seen server's welcome message
};

/**
 * client data that stays across multiple respawns, but is cleared
 * on each level change or team change at ClientBegin()
 */
struct clientPersistant_t
{
	clientConnected_t connected;
	usercmd_t         cmd; // we would lose angles if not persistent
	bool          localClient; // true if "ip" info key is "localhost"
	bool          stickySpec; // don't stop spectating a player after they get killed
	bool          pmoveFixed; //
	char              netname[ MAX_NAME_LENGTH ];
	int               enterTime; // level.time the client entered the game
	int               location; // player locations
	int               teamInfo; // level.time of team overlay update (disabled = 0)
	float             flySpeed; // for spectator/noclip moves
	bool          disableBlueprintErrors; // should the buildable blueprint never be hidden from the players?

	class_t           classSelection; // player class (copied to ent->client->ps.stats[ STAT_CLASS ] once spawned)
	float             evolveHealthFraction;
	int               devolveReturningCredits;
	weapon_t          humanItemSelection; // humans have a starting item

	int               teamChangeTime; // level.time of last team change
	namelog_t         *namelog;
	g_admin_admin_t   *admin;

	int               aliveSeconds; // time player has been alive in seconds

	// These have a copy in playerState_t.persistent but we use them in GAME so they don't get invalidated by
	// SPECTATOR_FOLLOW mode
	int team;
	int credit;

	int voted;
	int voteYes, voteNo;

	// flood protection
	int      floodDemerits;
	int      floodTime;

	vec3_t   lastDeathLocation;
	char     guid[ 33 ];
	addr_t   ip;
	char     voice[ MAX_VOICE_NAME_LEN ];
	bool useUnlagged;
	int      pubkey_authenticated; // -1 = does not have pubkey, 0 = not authenticated, 1 = authenticated
	int      pubkey_challengedAt; // time at which challenge was sent

	// level.time when teamoverlay info changed so we know to tell other players.
	int                 infoChangeTime;

	// warnings in the ban log
	bool            hasWarnings;

	bool isFillerBot;
};

struct unlagged_t
{
	vec3_t   origin;
	vec3_t   mins;
	vec3_t   maxs;
	bool used;
};

#define MAX_UNLAGGED_MARKERS 256
#define MAX_TRAMPLE_BUILDABLES_TRACKED 20

/**
 * this structure is cleared on each ClientSpawn(),
 * except for 'client->pers' and 'client->sess'
 */
struct gclient_t
{
	// ps MUST be the first element, because the server expects it
	playerState_t ps; // communicated by server to clients

	// exported into pmove, but not communicated to clients
	pmoveExt_t pmext;

	// the rest of the structure is private to game
	clientPersistant_t pers;
	clientSession_t    sess;

	bool           readyToExit; // wishes to leave the intermission

	bool           noclip;
	int                cliprcontents; // the backup layer of ent->r.contents for when noclipping

	int                lastCmdTime; // level.time of last usercmd_t, for EF_CONNECTION
	// we can't just use pers.lastCommand.time, because
	// of the g_synchronousClients case
	byte   buttons[ USERCMD_BUTTONS / 8 ];
	byte   oldbuttons[ USERCMD_BUTTONS / 8 ];
	byte   latched_buttons[ USERCMD_BUTTONS / 8 ];

	vec3_t oldOrigin;

	// sum up damage over an entire frame, so shotgun blasts give a single big kick
	int      damage_received;  // damage received this frame
	vec3_t   damage_from;      // last damage direction
	bool damage_fromWorld; // if true, don't use the damage_from vector

	// timers
	int        respawnTime; // can respawn when time > this
	int        inactivityTime; // kick players when time > this
	bool   inactivityWarning; // true if the five second warning has been given
	int        boostedTime; // last time we touched a booster

	int        lowOxygenDamage; // the damage we will take when we run out of oxygen
	// (see also lowOxygenTime which is in playerState_t since it needs to be transmitted)

	int        time100; // timer for 100ms interval events
	int        time1000; // timer for one second interval events
	int        time10000; // timer for ten second interval events

	int        lastPoisonTime;
	int        poisonImmunityTime;
	gentity_t  *lastPoisonClient;
	int        lastLockTime;
	int        lastSlowTime;
	int        lastMedKitTime;
	int        medKitHealthToRestore;
	int        medKitIncrementTime;
	int        lastCreepSlowTime; // time until creep can be removed
	int        lastCombatTime; // time of last damage received/dealt from/to clients
	int        lastAmmoRefillTime;
	int        lastFuelRefillTime;
	int        lastLockWarnTime; // used for the entity locking system

	unlagged_t unlaggedHist[ MAX_UNLAGGED_MARKERS ];
	unlagged_t unlaggedBackup;
	unlagged_t unlaggedCalc;
	int        unlaggedTime;

	float      voiceEnthusiasm;
	char       lastVoiceCmd[ MAX_VOICE_CMD_LEN ];

	int        trampleBuildablesHitPos;
	int        trampleBuildablesHit[ MAX_TRAMPLE_BUILDABLES_TRACKED ];

	int        nextCrushTime;

	int        lastLevel1SlowTime;

	// gives the entityNum
	int num() const {
		ASSERT(this - g_clients >= 0);
		ASSERT(this - g_clients < MAX_CLIENTS);
		return this - g_clients;
	}

	// strictly speaking, we could return a non-const pointer, but this is
	// probably more in phase with the idea
	const gentity_t *ent() const {
		ASSERT(this - g_clients >= 0);
		ASSERT(this - g_clients < MAX_CLIENTS);
		return &g_entities[this - g_clients];
	}

	gentity_t *ent() {
		ASSERT(this - g_clients >= 0);
		ASSERT(this - g_clients < MAX_CLIENTS);
		return &g_entities[this - g_clients];
	}
};

/**
 * store locational damage regions
 */
struct damageRegion_t
{
	char     name[ 32 ];
	float    area, modifier, minHeight, maxHeight;
	int      minAngle, maxAngle;
	bool crouch;
	bool nonlocational;
};

struct spawnQueue_t
{
	int clients[ MAX_CLIENTS ];

	int front, back;
};

/**
 * data needed to revert a change in layout
 */
struct buildLog_t
{
	int         time;
	buildFate_t fate;
	namelog_t   *actor;
	namelog_t   *builtBy;
	team_t      buildableTeam;
	buildable_t modelindex;
	float       momentumEarned;
	bool        markedForDeconstruction;
	vec3_t      origin;
	vec3_t      angles;
	vec3_t      origin2;
	vec3_t      angles2;
};

#define MAX_SPAWN_VARS       64
#define MAX_SPAWN_VARS_CHARS 4096
#define MAX_BUILDLOG         1024

struct level_locals_t
{
	gclient_t *clients; // [maxclients]

	gentity_t *gentities;

	int              num_entities; // MAX_CLIENTS <= num_entities <= ENTITYNUM_MAX_NORMAL

	int              warmupTime; // restart match at this time
	int              timelimit; //time in minutes

	fileHandle_t     logFile;
	fileHandle_t     logGameplayFile;

	// store latched cvars here that we want to get at often
	int      maxclients;

	bool     inClient;

	int      time; // time the map was first started in milliseconds (map restart will update startTime)
	int      previousTime; // so movers can back up when blocked

	int      startTime; // level.time the map was last (re)started in milliseconds
	int      matchTime; // ms since the current match begun

	int      lastTeamLocationTime; // last time of client team location update

	bool restarted; // waiting for a map_restart to fire

	int      numConnectedClients; // connected clients (players + bots)
	int      numConnectedPlayers; // connected players (not bot) in a team or spec mode
	int      numAliveClients;     // on a team and alive
	int      numPlayingClients;   // on a team
	int      numPlayingPlayers;   // on a team and not a bot
	int      numPlayingBots;      // on a team and a bot

	int      sortedClients[ MAX_CLIENTS ]; // sorted by score

	int      snd_fry; // sound index for standing in lava

	// spawn variables
	bool spawning; // the G_Spawn*() functions are valid
	int      numSpawnVars;
	char     *spawnVars[ MAX_SPAWN_VARS ][ 2 ]; // key / value pairs
	int      numSpawnVarChars;
	char     spawnVarChars[ MAX_SPAWN_VARS_CHARS ];

	// intermission state
	int intermissionQueued; // intermission was qualified, but
	// wait INTERMISSION_DELAY_TIME before
	// actually going there so the last
	// frag can be watched.  Disable future
	// kills during this delay
	int              intermissiontime; // time the intermission was started
	char             *changemap;
	bool         readyToExit; // at least one client wants to exit
	int              exitTime;
	vec3_t           intermission_origin; // also used for spectator spawns
	vec3_t           intermission_angle;

	gentity_t        *locationHead; // head of the location list
	gentity_t        *fakeLocation; // fake location for anything which might need one

	gentity_t        *markedBuildables[ MAX_GENTITIES ];
	int              numBuildablesForRemoval;

	team_t           lastWin;

	timeWarning_t    timelimitWarning;

	team_t           unconditionalWin;

	int              pausedTime;

	int              unlaggedIndex;
	int              unlaggedTimes[ MAX_UNLAGGED_MARKERS ];

	char             layout[ MAX_QPATH ];

	team_t           surrenderTeam;
	int              lastTeamImbalancedTime;
	int              numTeamImbalanceWarnings;

	voice_t          *voices;

	namelog_t        *namelogs;

	buildLog_t       buildLog[ MAX_BUILDLOG ];
	int              buildId;
	int              numBuildLogs;

	// this is an estimate of the number of each buildable type
	// the buildables are counted efficiently in the server's main entity loop,
	// these numbers might be slightly outdated
	// however, the numbers will not be more than two frames behind
	int numBuildablesEstimate[ BA_NUM_BUILDABLES ];

	struct
	{
		// voting state
		std::string voteType;
		int  voteThreshold; // need at least this percent to pass
		char voteString[ MAX_STRING_CHARS ];
		char voteDisplayString[ MAX_STRING_CHARS ];
		int  voteTime; // level.time vote was called
		int  voteExecuteTime; // time the vote is executed
		int  voteDelay; // it doesn't make sense to always delay vote execution
		int  voteYes;
		int  voteNo;
		int  voted;
		int  quorum;

		// gameplay state
		int              numSpawns;
		int              numClients;
		int              numPlayers; // non-bot clients
		int              numBots;
		float            averageNumClients;
		float            averageNumPlayers;
		float            averageNumBots;
		int              numSamples;
		int              numAliveClients;
		float            totalBudget; // Read access always rounds towards zero.
		int              spentBudget;
		int              queuedBudget;
		int              kills;
		spawnQueue_t     spawnQueue;
		bool             locked;
		float            momentum;
		int              layoutBuildPoints;
		int              botFillTeamSize;
		int              botFillSkillLevel;
		int              lastTeamStatus;
		int              lastTacticId;
		int              lastTacticTime;
	} team[ NUM_TEAMS ];

	struct {
		int synchronous;
		bool fixed;
		int msec;
		bool accurate;
		bool initialized;
	} pmoveParams;
};

struct commands_t
{
	const char *cmdName;
	int        cmdFlags;
	void      ( *cmdHandler )( gentity_t *ent );
};

struct zap_t
{
	bool  used;

	gentity_t *creator;
	gentity_t *targets[ LEVEL2_AREAZAP_MAX_TARGETS ];
	int       numTargets;
	float     distances[ LEVEL2_AREAZAP_MAX_TARGETS ];

	int       timeToLive;

	gentity_t *effectChannel;
};

#endif // SG_STRUCT_H_
