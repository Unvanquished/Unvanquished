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

#ifndef SG_STRUCT_H_
#define SG_STRUCT_H_

struct variatingTime_s
{
	float time;
	float variance;
};

/**
 * in the context of a target, this describes the conditions to create or to act within
 * while as part of trigger or most other types, it will be used as filtering condition that needs to be fulfilled to trigger, or to act directly
 */
struct gentityConditions_s
{
	team_t   team;
	int      stage;

	class_t     classes[ PCL_NUM_CLASSES ];
	weapon_t    weapons[ WP_NUM_WEAPONS ];
	upgrade_t   upgrades[ UP_NUM_UPGRADES ];
	buildable_t buildables[ BA_NUM_BUILDABLES ];

	bool negated;
};

/**
 * struct containing the configuration data of a gentity opposed to its state data
 */
struct gentityConfig_s
{
	/* amount of a context depended size for this entity */
	int amount;

	int health;
	float speed;
	int damage;

	/**
	 * how long to wait before fullfilling the maintask act()
	 * (e.g. how long to delay sensing as sensor or relaying as relay)
	 */
	variatingTime_t delay;
	/**
	 * the duration of one cycle in a repeating event
	 */
	variatingTime_t period;
	/**
	 * how long to wait in a state after a statechange
	 */
	variatingTime_t wait;

	// trigger "range"
	int triggerRange;
};

struct entityClass_s
{
	int instanceCounter;
	/**
	 * default config
	 * entities might fallback to their classwide config if their individual is not set
	 */
	gentityConfig_t config;
};

class Entity;

struct gentity_s
{
	entityState_t  s; // communicated by server to clients
	entityShared_t r; // shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	// New style entity
	Entity* entity;
	
	struct gclient_s *client; // nullptr if not a client

	bool     inuse;
	bool     neverFree; // if true, FreeEntity will only unlink
	int          freetime; // level.time when the object was freed
	int          eventTime; // events will be cleared EVENT_VALID_MSEC after set
	bool     freeAfterEvent;
	bool     unlinkAfterEvent;

	int          flags; // FL_* variables

	/*
	 * the class of the entity
	 * this is shared among all instances of this type
	 */
	entityClass_t *eclass;

	const char   *classname;
	int          spawnflags;

	//entity creation time, i.e. when a building was build or a missile was fired (for diminishing missile damage)
	int          creationTime;

	char         *names[ MAX_ENTITY_ALIASES + 1 ];

	/**
	 * is the entity considered active?
	 * as in 'currently doing something'
	 * e.g. used for buildables (e.g. medi-stations or hives can be in an active state or being inactive)
	 * or during executing act() in general
	 */
	bool     active;
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
	gentity_t    *powerSource;

	/**
	 * Human buildables compete for power.
	 * currentSparePower takes temporary influences into account and sets a buildables power state.
	 * expectedSparePower is a prediction of the static part and is used for limiting new buildables.
	 *
	 * currentSparePower  >= 0: Buildable has enough power
	 * currentSparePower  <  0: Buildable lacks power, will shut down
	 * expectedSparePower >  0: New buildables can be built in range
	 */
	float        currentSparePower;
	float        expectedSparePower;

	/**
	 * The amount of momentum this building generated on construction
	 */
	float        momentumEarned;

	/*
	 * targets to aim at
	 */
	int          targetCount;
	char         *targets[ MAX_ENTITY_TARGETS + 1 ];
	gentity_t    *target;  /*< the currently selected target to aim at/for, is the reverse to "tracker" */
	gentity_t    *tracker; /*< entity that currently targets, aims for or tracks this entity, is the reverse to "target" */
	int          numTrackedBy;
	/* path chaining, not unlike the target/tracker relationship */
	gentity_t    *nextPathSegment;
	gentity_t    *prevPathSegment;

	/*
	 * gentities to call on certain events
	 */
	int          callTargetCount;
	gentityCallDefinition_t calltargets[ MAX_ENTITY_CALLTARGETS + 1 ];

	/**
	 * current valid call state for a single threaded call hierarchy.
	 * this allows us to lookup the current callIn,
	 * walk back further the hierarchy and even do simply loop detection
	 */
	gentityCall_t callIn;
	gentity_t    *activator; //FIXME: handle this as part of the current Call

	/*
	 * configuration, as supplied by the spawn string, external spawn scripts etc.
	 * as opposed to state data as placed everywhere else
	 */
	gentityConfig_t config;

	//conditions as trigger-filter or target-goal
	gentityConditions_t conditions;

	// entity groups
	char         *groupName;
	gentity_t    *groupChain; // next entity in group
	gentity_t    *groupMaster; // master of the group

	char     *model;
	char     *model2;

	bool physicsObject; // if true, it can be pushed by movers and fall off edges
	// all game items are physicsObjects,
	float    physicsBounce; // 1.0 = continuous bounce, 0.0 = no bounce
	int      clipmask; // brushes with this content value will be collided against
	// when moving.  items and corpses do not collide against
	// players, for instance

	//sound index, used by movers as well as target_speaker e.g. for looping sounds
	int          soundIndex;

	// movers
	moverState_t moverState;
	int          soundPos1, soundPos2;
	int          sound1to2, sound2to1;

	vec3_t       restingPosition, activatedPosition;
	float        rotatorAngle;
	gentity_t    *clipBrush; // clipping brush for model doors

	char         *message;

	/*
	 * for toggleable shaders
	 */
	char         *shaderKey;
	char         *shaderReplacement;

	float        lastHealthFrac;
	int          health;
	float        deconHealthFrac;

	float        speed;

	/* state of the amount of a context depended size for this entity
	 * example: current set gravity for a gravity afx-entity
	 */
	int          amount;

	/*
	 * do not abuse this variable (again) for anything but actual representing a count
	 *
	 * add your own number with correct semantic information to gentity_t or
	 * if you really have to use customNumber
	 */
	int count;

	// acceleration evaluation
	bool  evaluateAcceleration;
	vec3_t    oldVelocity;
	vec3_t    acceleration;
	vec3_t    oldAccel;
	vec3_t    jerk;

	vec3_t       movedir;


	/*
	 * handle the notification about an event or undertaken action, so each entity can decide to undertake special actions as result
	 */
	void ( *notifyHandler )( gentity_t *self, gentityCall_t *call );

	/**
	 * the entry function for calls to the entity;
	 * especially previous chain members will indirectly call this when firing against the given entity
	 * @returns true if the call was handled by the given function and doesn't need default handling anymore or false otherwise
	 */
	bool ( *handleCall )( gentity_t *self, gentityCall_t *call );

	int       nextthink;
	void ( *think )( gentity_t *self );

	void ( *reset )( gentity_t *self );
	void ( *reached )( gentity_t *self );       // movers call this when hitting endpoint
	void ( *blocked )( gentity_t *self, gentity_t *other );
	void ( *touch )( gentity_t *self, gentity_t *other, trace_t *trace );
	void ( *use )( gentity_t *self, gentity_t *other, gentity_t *activator );
	void ( *pain )( gentity_t *self, gentity_t *attacker, int damage );
	void ( *die )( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod );

	bool  takedamage;

	int       flightSplashDamage; // quad will increase this without increasing radius
	int       flightSplashRadius;
	int       splashDamage; // quad will increase this without increasing radius
	int       splashRadius;
	int       methodOfDeath;
	int       splashMethodOfDeath;

	int       watertype;
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
	struct namelog_s *builtBy; // clientNum of person that built this

	int         pain_debounce_time;
	int         last_move_time;
	int         timestamp; // body queue sinking, etc
	int         shrunkTime; // time when a barricade shrunk or zero
	gentity_t   *boosterUsed; // the booster an alien is using for healing
	int         boosterTime; // last time alien used a booster for healing
	int         healthSourceTime; // last time an alien had contact to a health source
	int         animTime; // last animation change
	int         time1000; // timer evaluated every second

	bool        deconMarkHack; // TODO: Remove.
	int         attackTimer, attackLastEvent; // self being attacked
	int         warnTimer; // nearby building(s) being attacked
	int         overmindDyingTimer;
	int         overmindSpawnsTimer;
	int         nextPhysicsTime; // buildables don't need to check what they're sitting on
	// every single frame.. so only do it periodically
	int         clientSpawnTime; // the time until this spawn can spawn a client
	int         spawnBlockTime; // timer for anti spawn-block

	struct {
	 	float  value;
		int    time;
		team_t team;
	}           credits[ MAX_CLIENTS ];

	int         killedBy; // clientNum of killer

	vec3_t      buildableAim; // aim vector for buildables

	// turret
	int         turretNextShot;
	int         turretLastLOSToTarget;
	int         turretLastValidTargetTime;
	int         turretLastTargetSearch;
	int         turretLastHeadMove;
	int         turretCurrentDamage;
	vec3_t      turretDirToTarget;
	vec3_t      turretBaseDir;
	bool    turretDisabled;
	int         turretSafeModeCheckTime;
	bool    turretSafeMode;
	int         turretPrepareTime; // when the turret can start locking on and/or firing
	int         turretLockonTime;  // when the turret can start firing

	// spiker
	int         spikerRestUntil;
	float       spikerLastScoring;
	bool    spikerLastSensing;

	vec4_t      animation; // animated map objects

	bool    nonSegModel; // this entity uses a nonsegmented player model

	int         suicideTime; // when the client will suicide

	int         lastDamageTime;
	int         nextRegenTime;

	bool    pointAgainstWorld; // don't use the bbox for map collisions

	qhandle_t   obstacleHandle;
	botMemory_t *botMind;

	gentity_t   *alienTag, *humanTag;
	gentity_t   **tagAttachment;
	int         tagScore;
	int         tagScoreTime;
};

/**
 * client data that stays across multiple levels or map restarts
 * this is achieved by writing all the data to cvar strings at game shutdown
 * time and reading them back at connection time.  Anything added here
 * MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
 */
struct clientSession_s
{
	int              spectatorTime; // for determining next-in-line to play
	spectatorState_t spectatorState;
	int              spectatorClient; // for chasecam and follow mode
	team_t           restartTeam; //for !restart keepteams and !restart switchteams
	int              botSkill;
	char             botTree[ MAX_QPATH ];
	clientList_t     ignoreList;
	int              seenWelcome; // determines if the client has seen server's welcome message
};

#define MAX_NAMELOG_NAMES 5
#define MAX_NAMELOG_ADDRS 5

struct namelog_s
{
	struct namelog_s *next;

	char             name[ MAX_NAMELOG_NAMES ][ MAX_NAME_LENGTH ];
	addr_t           ip[ MAX_NAMELOG_ADDRS ];
	char             guid[ 33 ];
	int              slot;
	bool         banned;

	int              nameOffset;
	int              nameChangeTime;
	int              nameChanges;
	int              voteCount;

	unnamed_t        unnamedNumber;

	bool         muted;
	bool         denyBuild;

	int              score;
	int              credits;
	team_t           team;

	int              id;
};

/**
 * client data that stays across multiple respawns, but is cleared
 * on each level change or team change at ClientBegin()
 */
struct clientPersistant_s
{
	clientConnected_t connected;
	usercmd_t         cmd; // we would lose angles if not persistent
	bool          localClient; // true if "ip" info key is "localhost"
	bool          stickySpec; // don't stop spectating a player after they get killed
	bool          pmoveFixed; //
	char              netname[ MAX_NAME_LENGTH ];
	char              country[ MAX_NAME_LENGTH ];
	int               enterTime; // level.time the client entered the game
	int               location; // player locations
	int               teamInfo; // level.time of team overlay update (disabled = 0)
	float             flySpeed; // for spectator/noclip moves
	bool          disableBlueprintErrors; // should the buildable blueprint never be hidden from the players?

	class_t           classSelection; // player class (copied to ent->client->ps.stats[ STAT_CLASS ] once spawned)
	float             evolveHealthFraction;
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
};

struct unlagged_s
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
struct gclient_s
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
	bool   inactivityWarning; // true if the five seoond warning has been given
	int        boostedTime; // last time we touched a booster

	int        airOutTime;

	int        switchTeamTime; // time the player switched teams

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
};

/**
 * store locational damage regions
 */
struct damageRegion_s
{
	char     name[ 32 ];
	float    area, modifier, minHeight, maxHeight;
	int      minAngle, maxAngle;
	bool crouch;
	bool nonlocational;
};

struct spawnQueue_s
{
	int clients[ MAX_CLIENTS ];

	int front, back;
};

/**
 * data needed to revert a change in layout
 */
struct buildLog_s
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

struct level_locals_s
{
	struct gclient_s *clients; // [maxclients]

	struct gentity_s *gentities;

	int              gentitySize;
	int              num_entities; // MAX_CLIENTS <= num_entities <= ENTITYNUM_MAX_NORMAL

	int              warmupTime; // restart match at this time
	int              timelimit; //time in minutes

	fileHandle_t     logFile;
	fileHandle_t     logGameplayFile;

	// store latched cvars here that we want to get at often
	int      maxclients;

	bool     inClient;

	int      framenum;
	int      time; // time the map was first started in milliseconds (map restart will update startTime)
	int      previousTime; // so movers can back up when blocked
	int      frameMsec; // trap_Milliseconds() at end frame

	int      startTime; // level.time the map was last (re)started in milliseconds
	int      matchTime; // ms since the current match begun

	int      lastTeamLocationTime; // last time of client team location update

	bool restarted; // waiting for a map_restart to fire

	int      numConnectedClients; // connected
	int      numAliveClients;     // on a team and alive
	int      numPlayingClients;   // on a team
	int      numPlayingPlayers;   // on a team and not a bot
	int      numPlayingBots;      // on a team and a bot

	int      sortedClients[ MAX_CLIENTS ]; // sorted by score

	int      snd_fry; // sound index for standing in lava

	int      warmupModificationCount; // for detecting if g_warmup is changed

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

	float            mineRate;

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

	emoticon_t       emoticons[ MAX_EMOTICONS ];
	int              emoticonCount;

	namelog_t        *namelogs;

	buildLog_t       buildLog[ MAX_BUILDLOG ];
	int              buildId;
	int              numBuildLogs;

	bool         overmindMuted;

	int              humanBaseAttackTimer;

	struct
	{
		// voting state
		voteType_t voteType;
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
		int              numPlayers;
		int              numBots;
		float            averageNumClients;
		float            averageNumPlayers;
		float            averageNumBots;
		int              numSamples;
		int              numAliveClients;
		float            buildPoints;
		float            acquiredBuildPoints;
		float            mainStructAcquiredBP;
		float            mineEfficiency;
		int              kills;
		spawnQueue_t     spawnQueue;
		bool         locked;
		float            momentum;
		int              layoutBuildPoints;
	} team[ NUM_TEAMS ];

	struct {
		int synchronous;
		int fixed;
		int msec;
		int accurate;
		bool initialized;
	} pmoveParams;
};

struct commands_s
{
	const char *cmdName;
	int        cmdFlags;
	void      ( *cmdHandler )( gentity_t *ent );
};

struct zap_s
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
