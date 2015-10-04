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

#ifndef BG_PUBLIC_H_
#define BG_PUBLIC_H_
// bg_public.h -- definitions shared by both the server game and client game modules
//==================================================================

//Unvanquished balance header
#include "bg_gameplay.h"

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame
#define GAME_VERSION       "base"

#define DEFAULT_GRAVITY    800

#define VOTE_TIME          30000 // 30 seconds before vote times out

#define MINS_Z             -24
#define DEFAULT_VIEWHEIGHT 26
#define CROUCH_VIEWHEIGHT  12
#define DEAD_VIEWHEIGHT    4 // height from ground

#define POWER_REFRESH_TIME 2000 // nextthink time for power checks

// player teams
typedef enum
{
  TEAM_ALL = -1,
  TEAM_NONE,
  TEAM_ALIENS,
  TEAM_HUMANS,

  NUM_TEAMS
} team_t;

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
enum
{
  CS_MUSIC = 2,
  CS_MESSAGE, // from the map worldspawn's message field
  CS_MOTD, // g_motd string for server message of the day
  CS_WARMUP, // server time when the match will be restarted

  CS_VOTE_TIME, // Vote stuff each needs NUM_TEAMS slots
  CS_VOTE_STRING = CS_VOTE_TIME + NUM_TEAMS,
  CS_VOTE_YES = CS_VOTE_STRING + NUM_TEAMS,
  CS_VOTE_NO = CS_VOTE_YES + NUM_TEAMS,
  CS_VOTE_CALLER = CS_VOTE_NO + NUM_TEAMS,

  CS_GAME_VERSION = CS_VOTE_CALLER + NUM_TEAMS,
  CS_LEVEL_START_TIME, // so the timer only shows the current level
  CS_INTERMISSION, // when 1, timelimit has been hit and intermission will start in a second or two
  CS_WINNER, // string indicating round winner
  CS_SHADERSTATE,
  CS_BOTINFO,
  CS_CLIENTS_READY,

  CS_UNUSED_1,
  CS_UNUSED_2,

  CS_MODELS,
  CS_SOUNDS = CS_MODELS + MAX_MODELS,
  CS_SHADERS = CS_SOUNDS + MAX_SOUNDS,
  CS_GRADING_TEXTURES = CS_SHADERS + MAX_GAME_SHADERS,
  CS_REVERB_EFFECTS = CS_GRADING_TEXTURES + MAX_GRADING_TEXTURES,
  CS_PARTICLE_SYSTEMS = CS_REVERB_EFFECTS + MAX_REVERB_EFFECTS,

  CS_PLAYERS = CS_PARTICLE_SYSTEMS + MAX_GAME_PARTICLE_SYSTEMS,
  CS_OBJECTIVES = CS_PLAYERS + MAX_CLIENTS,
  CS_LOCATIONS = CS_OBJECTIVES + MAX_OBJECTIVES,
  CS_MAX = CS_LOCATIONS + MAX_LOCATIONS
};

#if CS_MAX > MAX_CONFIGSTRINGS
#error exceeded configstrings: CS_MAX > MAX_CONFIGSTRINGS
#endif

typedef enum
{
  GENDER_MALE,
  GENDER_FEMALE,
  GENDER_NEUTER
} gender_t;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum
{
  PM_NORMAL, // can accelerate and turn
  PM_NOCLIP, // noclip movement
  PM_SPECTATOR, // still run into walls
  PM_GRABBED, // like dead, but for when the player is still alive
  PM_DEAD, // no acceleration or turning, but free falling
  PM_FREEZE, // stuck in place with no control
  PM_INTERMISSION // no movement or status bar
} pmtype_t;

// pmtype_t categories
#define PM_Paralyzed( x ) ( ( x ) == PM_DEAD || ( x ) == PM_FREEZE || ( x ) == PM_INTERMISSION )
#define PM_Live( x )      ( ( x ) == PM_NORMAL || ( x ) == PM_GRABBED )

typedef enum
{
  WEAPON_READY,
  WEAPON_RAISING,
  WEAPON_DROPPING,
  WEAPON_FIRING,
  WEAPON_RELOADING,
  WEAPON_NEEDS_RESET,
} weaponstate_t;

// pmove->pm_flags
#define PMF_DUCKED         0x000001
#define PMF_JUMP_HELD      0x000002
#define PMF_CROUCH_HELD    0x000004
#define PMF_BACKWARDS_JUMP 0x000008 // go into backwards land
#define PMF_BACKWARDS_RUN  0x000010 // coast down to backwards run
#define PMF_JUMPED         0x000020 // whether we entered the air with a jump
#define PMF_TIME_KNOCKBACK 0x000040 // pm_time is an air-accelerate only time
#define PMF_TIME_WATERJUMP 0x000080 // pm_time is waterjump
#define PMF_RESPAWNED      0x000100 // clear after attack and jump buttons come up
#define PMF_USE_ITEM_HELD  0x000200
#define PMF_WEAPON_RELOAD  0x000400 // force a weapon switch
#define PMF_FOLLOW         0x000800 // spectate following another player
#define PMF_QUEUED         0x001000 // player is queued
#define PMF_TIME_WALLJUMP  0x002000 // for limiting wall jumping
#define PMF_CHARGE         0x004000 // keep track of pouncing
#define PMF_WEAPON_SWITCH  0x008000 // force a weapon switch
#define PMF_SPRINTHELD     0x010000

#define PMF_ALL_TIMES      ( PMF_TIME_WATERJUMP | PMF_TIME_KNOCKBACK | PMF_TIME_WALLJUMP )

typedef struct
{
	int    pouncePayload;
	vec3_t fallImpactVelocity;
} pmoveExt_t;

#define MAXTOUCH 32
typedef struct pmove_s
{
	// state (in / out)
	playerState_t *ps;
	pmoveExt_t    *pmext;
	// command (in)
	usercmd_t     cmd;
	int           tracemask; // collide against these types of surfaces
	int           debugLevel; // if set, diagnostic output will be printed
	bool      noFootsteps; // if the game is setup for no footsteps by the server
	bool      autoWeaponHit[ 32 ];

	int           framecount;

	// results (out)
	int    numtouch;
	int    touchents[ MAXTOUCH ];

	vec3_t mins, maxs; // bounding box size

	int    watertype;
	int    waterlevel;

	float  xyspeed;

	// for fixed msec Pmove
	bool pmove_fixed;
	int pmove_msec;

	// don't round velocity to an integer
	int pmove_accurate;

	// callbacks to test the world
	// these will be different functions during game and cgame
	/*void    (*trace)( trace_t *results, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );*/
	void ( *trace )( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
	                 const vec3_t end, int passEntityNum, int contentMask, int skipmask );

	int ( *pointcontents )( const vec3_t point, int passEntityNum );
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd );
void Pmove( pmove_t *pmove );

//===================================================================================

// player_state->stats[] indexes
// netcode has space for 16 stats
typedef enum
{
  STAT_HEALTH,
  STAT_ITEMS,
  STAT_ACTIVEITEMS,
  STAT_WEAPON,     // current primary weapon
  STAT_MAX_HEALTH, // health limit
  STAT_CLASS,      // player class
  STAT_STATE2,     // more client states
  STAT_STAMINA,    // humans: stamina
  STAT_STATE,      // client states
  STAT_MISC,       // aliens: pounce, trample; humans: lcannon
  STAT_BUILDABLE,  // ghost model to display for building
  STAT_FALLDIST,   // distance the player fell
  STAT_VIEWLOCK,   // direction to lock the view in
  STAT_PREDICTION, // predictions for current player action
  STAT_FUEL,       // humans: jetpack fuel
  STAT_TAGSCORE    // tagging progress
} statIndex_t;

#define SCA_WALLCLIMBER     0x00000001
#define SCA_TAKESFALLDAMAGE 0x00000002
#define SCA_CANZOOM         0x00000004
#define SCA_FOVWARPS        0x00000008
#define SCA_ALIENSENSE      0x00000010
#define SCA_CANUSELADDERS   0x00000020
#define SCA_WALLJUMPER      0x00000040

// STAT_STATE fields. 16 bit available
#define SS_WALLCLIMBING     BIT(0)
#define SS_CREEPSLOWED      BIT(1)
#define SS_SPEEDBOOST       BIT(2)
#define SS_UNUSED_1         BIT(3)
#define SS_BLOBLOCKED       BIT(4)
#define SS_POISONED         BIT(5)
#define SS_BOOSTED          BIT(6)
#define SS_BOOSTEDNEW       BIT(7) // booster recharged // TODO: Unnecessary, remove
#define SS_BOOSTEDWARNING   BIT(8) // booster poison is running out // TODO: Unnecessary, remove
#define SS_SLOWLOCKED       BIT(9)
#define SS_CHARGING         BIT(10)
#define SS_HEALING_2X       BIT(11) // humans: medistation
#define SS_HEALING_4X       BIT(12) // humans: medikit active
#define SS_HEALING_8X       BIT(13)

// STAT_STATE2 fields. 16 bit available
#define SS2_JETPACK_ENABLED BIT(0)  // whether jets/wings are extended
#define SS2_JETPACK_WARM    BIT(1)  // whether we can start a thrust
#define SS2_JETPACK_ACTIVE  BIT(2)  // whether we are thrusting
#define SS2_LEVEL1SLOW      BIT(3)  // hit and slowed by a Mantis attack

// has to fit into 16 bits
#define SB_BUILDABLE_MASK        0x00FF
#define SB_BUILDABLE_STATE_MASK  0xFF00
#define SB_BUILDABLE_STATE_SHIFT 8
#define SB_BUILDABLE_FROM_IBE(x) ( ( x ) << SB_BUILDABLE_STATE_SHIFT )
#define SB_BUILDABLE_TO_IBE(x)   ( ( ( x ) & SB_BUILDABLE_STATE_MASK ) >> SB_BUILDABLE_STATE_SHIFT )

typedef enum
{
  IBE_NONE,             // no error, can build

  IBE_NOOVERMIND,       // no overmind present
  IBE_ONEOVERMIND,      // may not build two overminds
  IBE_NOALIENBP,        // not enough build points (aliens)
  IBE_NOCREEP,          // no creep in this area

  IBE_NOREACTOR,        // not enough power in this area and no reactor present
  IBE_ONEREACTOR,       // may not build two reactors
  IBE_NOHUMANBP,        // not enough build points (humans)
  IBE_NOPOWERHERE,      // not enough power in this area even though a reactor is present

  IBE_NORMAL,           // surface is too steep
  IBE_NOROOM,           // no room
  IBE_SURFACE,          // map doesn't allow building on that surface
  IBE_DISABLED,         // building has been disabled for team
  IBE_LASTSPAWN,        // may not replace last spawn with non-spawn
  IBE_MAINSTRUCTURE,    // may not replace main structure with other buildable

  IBE_MAXERRORS
} itemBuildError_t;

// player_state->persistent[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
typedef enum
{
  PERS_SCORE,          // !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
  PERS_MOMENTUM,     // the total momentum of a team
  PERS_SPAWNQUEUE,     // number of spawns and position in spawn queue
  PERS_SPECSTATE,
  PERS_SPAWN_COUNT,    // incremented every respawn
  PERS_TEAM,           // persistant team selection
  PERS_RGS_EFFICIENCY, // summed efficiency of all friendly RGS
  PERS_STATE,
  PERS_CREDIT,         // human credit
  PERS_UNLOCKABLES,    // status of unlockable items of a team
  PERS_NEWWEAPON,      // weapon to switch to
  PERS_BP,
  PERS_MARKEDBP,
  PERS_MINERATE        // level wide base mine rate. TODO: calculate clientside
  // netcode has space for 2 more. TODO: extend
} persEnum_t;

#define PS_WALLCLIMBINGFOLLOW 0x00000001
#define PS_WALLCLIMBINGTOGGLE 0x00000002
#define PS_NONSEGMODEL        0x00000004
#define PS_SPRINTTOGGLE       0x00000008

// entityState_t->eFlags
// notice that some flags are overlapped, so their meaning depends on context
#define EF_DEAD             0x0001 // don't draw a foe marker over players with EF_DEAD
#define EF_TELEPORT_BIT     0x0002 // toggled every time the origin abruptly changes
#define EF_PLAYER_EVENT     0x0004 // only used for eType > ET_EVENTS

// for missiles:
#define EF_BOUNCE           0x0008 // for missiles
#define EF_BOUNCE_HALF      0x0010 // for missiles
#define EF_NO_BOUNCE_SOUND  0x0020 // for missiles

// buildable flags:
#define EF_B_SPAWNED        0x0008
#define EF_B_POWERED        0x0010
#define EF_B_MARKED         0x0020
#define EF_B_ONFIRE         0x0040
#define EF_B_LOCKON         0x0080

// for players
#define EF_POWER_AVAILABLE  0x0010
#define EF_WARN_CHARGE      0x0020 // Lucifer Cannon is about to overcharge
#define EF_WALLCLIMB        0x0040 // wall walking
#define EF_WALLCLIMBCEILING 0x0080 // wall walking ceiling hack
#define EF_NODRAW           0x0100 // may have an event, but no model (unspawned items)
#define EF_FIRING           0x0200 // for lightning gun
#define EF_FIRING2          0x0400 // alt fire
#define EF_FIRING3          0x0800 // third fire
#define EF_MOVER_STOP       0x1000 // will push otherwise
#define EF_UNUSED_1         0x2000 // UNUSED
#define EF_CONNECTION       0x4000 // draw a connection trouble sprite
#define EF_BLOBLOCKED       0x8000 // caught by a trapper

// entityState_t->modelIndex2 "public flags" when used for client entities
#define PF_JETPACK_ENABLED  BIT(0)
#define PF_JETPACK_ACTIVE   BIT(1)

// for beacons:
#define EF_BC_DYING         BIT(3) // beacon is fading out
#define EF_BC_ENEMY         BIT(4) // entity/base is from the enemy
#define EF_BC_TAG_PLAYER    BIT(5) // entity is a player
#define EF_BC_BASE_OUTPOST  BIT(6) // base is an outpost

#define EF_BC_TAG_RELEVANT  (EF_BC_ENEMY|EF_BC_TAG_PLAYER)   // relevant flags for tags
#define EF_BC_BASE_RELEVANT (EF_BC_ENEMY|EF_BC_BASE_OUTPOST) // relevant flags for bases

typedef enum
{
  WPM_NONE,

  WPM_PRIMARY,
  WPM_SECONDARY,
  WPM_TERTIARY,

  WPM_NOTFIRING,

  WPM_NUM_WEAPONMODES
} weaponMode_t;

typedef enum
{
  WP_NONE,

  WP_ALEVEL0,
  WP_ALEVEL1,
  WP_ALEVEL2,
  WP_ALEVEL2_UPG,
  WP_ALEVEL3,
  WP_ALEVEL3_UPG,
  WP_ALEVEL4,

  // there is some ugly code that assumes WP_BLASTER is the first human weapon
  WP_BLASTER,
  WP_MACHINEGUN,
  WP_PAIN_SAW,
  WP_SHOTGUN,
  WP_LAS_GUN,
  WP_MASS_DRIVER,
  WP_CHAINGUN,
  WP_FLAMER,
  WP_PULSE_RIFLE,
  WP_LUCIFER_CANNON,
  WP_LOCKBLOB_LAUNCHER,
  WP_HIVE,
  WP_ROCKETPOD,
  WP_MGTURRET,

  // build weapons must remain in a block ← I'm not asking why but I can imagine
  WP_ABUILD,
  WP_ABUILD2,
  WP_HBUILD,

  WP_NUM_WEAPONS
} weapon_t;

typedef enum
{
  UP_NONE,

  UP_LIGHTARMOUR,
  UP_MEDIUMARMOUR,
  UP_BATTLESUIT,

  UP_RADAR,
  UP_JETPACK,

  UP_GRENADE,
  UP_FIREBOMB,

  UP_MEDKIT,

  UP_NUM_UPGRADES
} upgrade_t;

typedef enum
{
	MIS_NONE,

	MIS_FLAMER,
	MIS_BLASTER,
	MIS_PRIFLE,
	MIS_LCANNON,
	MIS_LCANNON2,
	MIS_GRENADE,
	MIS_FIREBOMB,
	MIS_FIREBOMB_SUB,
	MIS_HIVE,
	MIS_LOCKBLOB,
	MIS_SLOWBLOB,
	MIS_BOUNCEBALL,
	MIS_ROCKET,
	MIS_SPIKER,

	MIS_NUM_MISSILES
} missile_t;

// bitmasks for upgrade slots
#define SLOT_NONE     0x00000000
#define SLOT_HEAD     0x00000001
#define SLOT_TORSO    0x00000002
#define SLOT_ARMS     0x00000004
#define SLOT_LEGS     0x00000008
#define SLOT_BACKPACK 0x00000010
#define SLOT_WEAPON   0x00000020
#define SLOT_SIDEARM  0x00000040
#define SLOT_GRENADE  0x00000080

typedef enum
{
  BA_NONE,

  BA_A_SPAWN,
  BA_A_OVERMIND,

  BA_A_BARRICADE,
  BA_A_ACIDTUBE,
  BA_A_TRAPPER,
  BA_A_BOOSTER,
  BA_A_HIVE,
  BA_A_LEECH,
  BA_A_SPIKER,

  BA_H_SPAWN,

  BA_H_MGTURRET,
  BA_H_ROCKETPOD,

  BA_H_ARMOURY,
  BA_H_MEDISTAT,
  BA_H_DRILL,

  BA_H_REACTOR,
  BA_H_REPEATER,

  BA_NUM_BUILDABLES
} buildable_t;

// entityState_t->event values
// entity events are for effects that take place relative
// to an existing entity's origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define EV_EVENT_BIT1    0x00000100
#define EV_EVENT_BIT2    0x00000200
#define EV_EVENT_BITS    ( EV_EVENT_BIT1 | EV_EVENT_BIT2 )

#define EVENT_VALID_MSEC 300

const char *BG_EventName( int num );

typedef enum
{
  EV_NONE,

  EV_FOOTSTEP,
  EV_FOOTSTEP_METAL,
  EV_FOOTSTEP_SQUELCH,
  EV_FOOTSPLASH,
  EV_FOOTWADE,
  EV_SWIM,

  EV_STEP_4,
  EV_STEP_8,
  EV_STEP_12,
  EV_STEP_16,

  EV_STEPDN_4,
  EV_STEPDN_8,
  EV_STEPDN_12,
  EV_STEPDN_16,

  EV_FALL_SHORT,
  EV_FALL_MEDIUM,
  EV_FALL_FAR,
  EV_FALLING,

  EV_JUMP,

  EV_WATER_TOUCH, // foot touches
  EV_WATER_LEAVE, // foot leaves
  EV_WATER_UNDER, // head touches
  EV_WATER_CLEAR, // head leaves

  EV_JETPACK_ENABLE,  // enable jets
  EV_JETPACK_DISABLE, // disable jets
  EV_JETPACK_IGNITE,  // ignite engine
  EV_JETPACK_START,   // start thrusting
  EV_JETPACK_STOP,    // stop thrusting

  EV_NOAMMO,
  EV_CHANGE_WEAPON,
  EV_FIRE_WEAPON,
  EV_FIRE_WEAPON2,
  EV_FIRE_WEAPON3,
  EV_WEAPON_RELOAD,

  EV_PLAYER_RESPAWN, // for fovwarp effects
  EV_PLAYER_TELEPORT_IN,
  EV_PLAYER_TELEPORT_OUT,

  EV_GRENADE_BOUNCE, // eventParm will be the soundindex

  EV_GENERAL_SOUND,
  EV_GLOBAL_SOUND, // no attenuation

  EV_WEAPON_HIT_ENTITY,
  EV_WEAPON_HIT_ENVIRONMENT,

  EV_SHOTGUN,
  EV_MASS_DRIVER,

  EV_MISSILE_HIT_ENTITY,
  EV_MISSILE_HIT_ENVIRONMENT,
  EV_MISSILE_HIT_METAL, // necessary?
  EV_TESLATRAIL,
  EV_BULLET, // otherEntity is the shooter

  EV_LEV4_TRAMPLE_PREPARE,
  EV_LEV4_TRAMPLE_START,

  EV_PAIN,
  EV_DEATH1,
  EV_DEATH2,
  EV_DEATH3,
  EV_OBITUARY,

  EV_GIB_PLAYER,

  EV_BUILD_CONSTRUCT,
  EV_BUILD_DESTROY,
  EV_BUILD_DELAY, // can't build yet
  EV_BUILD_REPAIR, // repairing buildable
  EV_BUILD_REPAIRED, // buildable has full health
  EV_HUMAN_BUILDABLE_DYING,
  EV_HUMAN_BUILDABLE_EXPLOSION,
  EV_ALIEN_BUILDABLE_EXPLOSION,
  EV_ALIEN_ACIDTUBE,
  EV_ALIEN_BOOSTER,

  EV_MEDKIT_USED,

  EV_ALIEN_EVOLVE,
  EV_ALIEN_EVOLVE_FAILED,

  EV_STOPLOOPINGSOUND,
  EV_TAUNT,

  EV_OVERMIND_ATTACK_1, // overmind under attack
  EV_OVERMIND_ATTACK_2, // overmind under attack
  EV_OVERMIND_DYING, // overmind close to death
  EV_OVERMIND_SPAWNS, // overmind needs spawns

  EV_REACTOR_ATTACK_1, // reactor under attack
  EV_REACTOR_ATTACK_2, // reactor under attack
  EV_REACTOR_DYING, // reactor destroyed

  EV_WARN_ATTACK, // a building has been destroyed and the destruction noticed by a nearby om/rc/rrep

  EV_MGTURRET_SPINUP, // turret spinup sound should play

  EV_AMMO_REFILL,     // ammo for clipless weapon has been refilled
  EV_CLIPS_REFILL,    // weapon clips have been refilled
  EV_FUEL_REFILL,     // jetpack fuel has been refilled

  EV_HIT, // notify client of a hit

  EV_MOMENTUM // notify client of generated momentum
} entity_event_t;

typedef enum
{
  MN_NONE,

  MN_WELCOME,
  MN_TEAM,
  MN_A_TEAMFULL,
  MN_H_TEAMFULL,
  MN_A_TEAMLOCKED,
  MN_H_TEAMLOCKED,
  MN_PLAYERLIMIT,
  MN_WARMUP,

  // cmd stuff
  MN_CMD_CHEAT,
  MN_CMD_CHEAT_TEAM,
  MN_CMD_TEAM,
  MN_CMD_SPEC,
  MN_CMD_ALIEN,
  MN_CMD_HUMAN,
  MN_CMD_ALIVE,

  //alien stuff
  MN_A_CLASS,
  MN_A_BUILD,
  MN_A_INFEST,
  MN_A_NOEROOM,
  MN_A_TOOCLOSE,
  MN_A_NOOVMND_EVOLVE,
  MN_A_EVOLVEBUILDTIMER,
  MN_A_CANTEVOLVE,
  MN_A_EVOLVEWALLWALK,
  MN_A_UNKNOWNCLASS,
  MN_A_CLASSNOTSPAWN,
  MN_A_CLASSNOTALLOWED,
  MN_A_CLASSLOCKED,

  //shared build
  MN_B_NOROOM,
  MN_B_NORMAL,
  MN_B_CANNOT,
  MN_B_LASTSPAWN,
  MN_B_MAINSTRUCTURE,
  MN_B_DISABLED,
  MN_B_REVOKED,
  MN_B_SURRENDER,

  //alien build
  MN_A_ONEOVERMIND,
  MN_A_NOBP,
  MN_A_NOCREEP,
  MN_A_NOOVMND,

  //human stuff
  MN_H_SPAWN,
  MN_H_BUILD,
  MN_H_ARMOURY,
  MN_H_UNKNOWNITEM,
  MN_H_NOSLOTS,
  MN_H_NOFUNDS,
  MN_H_ITEMHELD,
  MN_H_NOARMOURYHERE,
  MN_H_NOENERGYAMMOHERE,
  MN_H_NOROOMARMOURCHANGE,
  MN_H_ARMOURYBUILDTIMER,
  MN_H_DEADTOCLASS,
  MN_H_UNKNOWNSPAWNITEM,

  //human buildables
  MN_H_NOPOWERHERE,
  MN_H_NOREACTOR,
  MN_H_NOBP,
  MN_H_NOTPOWERED,
  MN_H_ONEREACTOR
} dynMenu_t;

// animations
typedef enum
{
  BOTH_DEATH1,
  BOTH_DEAD1,
  BOTH_DEATH2,
  BOTH_DEAD2,
  BOTH_DEATH3,
  BOTH_DEAD3,

  TORSO_GESTURE_BLASTER,
  TORSO_GESTURE,
  TORSO_GESTURE_PSAW,
  TORSO_GESTURE_SHOTGUN,
  TORSO_GESTURE_LGUN,
  TORSO_GESTURE_MDRIVER,
  TORSO_GESTURE_CHAINGUN,
  TORSO_GESTURE_PRIFLE,
  TORSO_GESTURE_FLAMER,
  TORSO_GESTURE_LUCI,
  TORSO_GESTURE_CKIT,

  TORSO_RALLY,

  TORSO_ATTACK_BLASTER,
  TORSO_ATTACK_PSAW,
  TORSO_ATTACK,

  TORSO_DROP,
  TORSO_RAISE,

  TORSO_STAND,
  TORSO_STAND_BLASTER,

  LEGS_WALKCR,
  LEGS_WALK,
  LEGS_RUN,
  LEGS_BACK,
  LEGS_SWIM,

  LEGS_JUMP,
  LEGS_LAND,

  LEGS_JUMPB,
  LEGS_LANDB,

  LEGS_IDLE,
  LEGS_IDLECR,

  LEGS_TURN,

  TORSO_GETFLAG,
  TORSO_GUARDBASE,
  TORSO_PATROL,
  TORSO_FOLLOWME,
  TORSO_AFFIRMATIVE,
  TORSO_NEGATIVE,

  MAX_PLAYER_ANIMATIONS,

  LEGS_BACKCR,
  LEGS_BACKWALK,
  FLAG_RUN,
  FLAG_STAND,
  FLAG_STAND2RUN,

  MAX_PLAYER_TOTALANIMATIONS
} playerAnimNumber_t;

// nonsegmented animations
typedef enum
{
  NSPA_STAND,

  NSPA_GESTURE,

  NSPA_WALK,
  NSPA_RUN,
  NSPA_RUNBACK,
  NSPA_CHARGE,

  NSPA_RUNLEFT,
  NSPA_WALKLEFT,
  NSPA_RUNRIGHT,
  NSPA_WALKRIGHT,

  NSPA_SWIM,

  NSPA_JUMP,
  NSPA_LAND,
  NSPA_JUMPBACK,
  NSPA_LANDBACK,

  NSPA_TURN,

  NSPA_ATTACK1,
  NSPA_ATTACK2,
  NSPA_ATTACK3,


  NSPA_PAIN1,
  NSPA_PAIN2,

  NSPA_DEATH1,
  NSPA_DEAD1,
  NSPA_DEATH2,
  NSPA_DEAD2,
  NSPA_DEATH3,
  NSPA_DEAD3,

  MAX_NONSEG_PLAYER_ANIMATIONS,

  NSPA_WALKBACK,

  MAX_NONSEG_PLAYER_TOTALANIMATIONS
} nonSegPlayerAnimNumber_t;

// for buildable animations
typedef enum
{
  BANIM_NONE,

  BANIM_IDLE1, // inactive idle
  BANIM_IDLE2, // active idle

  BANIM_POWERDOWN, // BANIM_IDLE1 -> BANIM_IDLE_UNPOWERED
  BANIM_IDLE_UNPOWERED,

  BANIM_CONSTRUCT, // -> BANIM_IDLE1

  BANIM_POWERUP, // BANIM_IDLE_UNPOWERED -> BANIM_IDLE1

  BANIM_ATTACK1,
  BANIM_ATTACK2,

  BANIM_SPAWN1,
  BANIM_SPAWN2,

  BANIM_PAIN1,
  BANIM_PAIN2,

  BANIM_DESTROY, // BANIM_IDLE1 -> BANIM_DESTROYED
  BANIM_DESTROY_UNPOWERED, // BANIM_IDLE_UNPOWERED -> BANIM_DESTROYED
  BANIM_DESTROYED,
  BANIM_DESTROYED_UNPOWERED,

  MAX_BUILDABLE_ANIMATIONS
} buildableAnimNumber_t;

typedef enum
{
  WANIM_NONE,

  WANIM_IDLE,

  WANIM_DROP,
  WANIM_RELOAD,
  WANIM_RAISE,

  WANIM_ATTACK1,
  WANIM_ATTACK2,
  WANIM_ATTACK3,
  WANIM_ATTACK4,
  WANIM_ATTACK5,
  WANIM_ATTACK6,
  WANIM_ATTACK7,
  WANIM_ATTACK8,

  MAX_WEAPON_ANIMATIONS
} weaponAnimNumber_t;

typedef struct animation_s
{
	qhandle_t handle;
	bool  clearOrigin;

	int       firstFrame;
	int       numFrames;
	int       loopFrames; // 0 to numFrames
	int       frameLerp; // msec between frames
	int       initialLerp; // msec to get to first frame
	int       reversed; // true if animation is reversed
	int       flipflop; // true if animation should flipflop back to base
} animation_t;

// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define ANIM_TOGGLEBIT            0x80
#define ANIM_FORCEBIT             0x40

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME 500

// player classes
typedef enum
{
  PCL_NONE,

  //builder classes
  PCL_ALIEN_BUILDER0,
  PCL_ALIEN_BUILDER0_UPG,

  //offensive classes
  PCL_ALIEN_LEVEL0,
  PCL_ALIEN_LEVEL1,
  PCL_ALIEN_LEVEL2,
  PCL_ALIEN_LEVEL2_UPG,
  PCL_ALIEN_LEVEL3,
  PCL_ALIEN_LEVEL3_UPG,
  PCL_ALIEN_LEVEL4,

  //human class
  PCL_HUMAN_NAKED,
  PCL_HUMAN_LIGHT,
  PCL_HUMAN_MEDIUM,
  PCL_HUMAN_BSUIT,

  PCL_NUM_CLASSES
} class_t;
// convenience bitmasks
#define PCL_ALIEN_CLASSES ( ( 1 << PCL_HUMAN_NAKED ) - ( 1 << PCL_ALIEN_BUILDER0 ) )
#define PCL_HUMAN_CLASSES ( ( 1 << PCL_NUM_CLASSES ) - ( 1 << PCL_HUMAN_NAKED ) )
#define PCL_ALL_CLASSES   ( PCL_ALIEN_CLASSES | PCL_HUMAN_CLASSES )

// spectator state
typedef enum
{
  SPECTATOR_NOT,
  SPECTATOR_FREE,
  SPECTATOR_LOCKED,
  SPECTATOR_FOLLOW,
  SPECTATOR_SCOREBOARD
} spectatorState_t;

// modes of text communication
typedef enum
{
  SAY_ALL,
  SAY_TEAM,
  SAY_PRIVMSG,
  SAY_TPRIVMSG,
  SAY_AREA,
  SAY_AREA_TEAM,
  SAY_ADMINS,
  SAY_ADMINS_PUBLIC,
  SAY_RAW,
  SAY_DEFAULT_DEPRECATED, // old - do not use
  SAY_ALL_ADMIN,
  SAY_ALL_ME,
  SAY_TEAM_ME,
  SAY_DEFAULT = 99
} saymode_t;

// means of death
// keep modNames[] in g_combat.c in sync with this list!
// keep bg_meansOfDeathData[] in bg_misc.c in sync, too!
// TODO: Get rid of the former and use the latter instead
typedef enum
{
  MOD_UNKNOWN,
  MOD_SHOTGUN,
  MOD_BLASTER,
  MOD_PAINSAW,
  MOD_MACHINEGUN,
  MOD_CHAINGUN,
  MOD_PRIFLE,
  MOD_MDRIVER,
  MOD_LASGUN,
  MOD_LCANNON,
  MOD_LCANNON_SPLASH,
  MOD_FLAMER,
  MOD_FLAMER_SPLASH,
  MOD_BURN,
  MOD_GRENADE,
  MOD_FIREBOMB,
  MOD_WEIGHT_H,
  MOD_WATER,
  MOD_SLIME,
  MOD_LAVA,
  MOD_CRUSH,
  MOD_TELEFRAG,
  MOD_FALLING,
  MOD_SUICIDE,
  MOD_TARGET_LASER,
  MOD_TRIGGER_HURT,

  MOD_ABUILDER_CLAW,
  MOD_LEVEL0_BITE,
  MOD_LEVEL1_CLAW,
  MOD_LEVEL3_CLAW,
  MOD_LEVEL3_POUNCE,
  MOD_LEVEL3_BOUNCEBALL,
  MOD_LEVEL2_CLAW,
  MOD_LEVEL2_ZAP,
  MOD_LEVEL4_CLAW,
  MOD_LEVEL4_TRAMPLE,
  MOD_WEIGHT_A,

  MOD_SLOWBLOB,
  MOD_POISON,
  MOD_SWARM,

  MOD_HSPAWN,
  MOD_ROCKETPOD,
  MOD_MGTURRET,
  MOD_REACTOR,

  MOD_ASPAWN,
  MOD_ATUBE,
  MOD_SPIKER,
  MOD_OVERMIND,
  MOD_DECONSTRUCT,
  MOD_REPLACE,
  MOD_NOCREEP
} meansOfDeath_t;

//---------------------------------------------------------

#define BEACON_TIMER_TIME 3500

typedef enum
{
	BCT_NONE = 0,

	//local
	BCT_POINTER,
	BCT_TIMER,

	//indicators
	BCT_TAG,
	BCT_BASE,

	//commands
	BCT_ATTACK,
	BCT_DEFEND,
	BCT_REPAIR,

	//implicit
	BCT_HEALTH,
	BCT_AMMO,
	
	NUM_BEACON_TYPES
} beaconType_t;

typedef enum
{
	BCH_NONE,
	BCH_REMOVE,
	BCH_MOVE
} beaconConflictHandler_t;

// beacon flags
#define BCF_RESERVED      0x0001 // generated automatically, not created by players

#define BCF_PER_PLAYER    0x0002 // one beacon per player
#define BCF_PER_TEAM      0x0004 // one beacon per team
#define BCF_DATA_UNIQUE   0x0008 // data extends type

#define BCF_PRECISE       0x0010 // place exactly at crosshair
#define BCF_IMPORTANT     0x0020 // display at 100% alpha

typedef struct
{
	beaconType_t  number;
	int           flags;

	const char    *name;
	const char    *humanName;

#ifdef BUILD_CGAME
	const char    *text[ 4 ];
	const char    *desc;
	qhandle_t     icon[ 2 ][ 4 ];
	sfxHandle_t   inSound;
	sfxHandle_t   outSound;
#endif	

	int           decayTime;
} beaconAttributes_t;

//---------------------------------------------------------

// player class record
typedef struct
{
	class_t  number;

	const char *name;
	const char *info;
	const char *icon;
	const char *fovCvar;

	team_t   team;

	int      unlockThreshold;

	int      health;
	float    fallDamage;
	float    regenRate;

	int      abilities;

	weapon_t startWeapon;

	float    buildDist;

	int      fov;
	float    bob;
	float    bobCycle;
	int      steptime;

	float    speed;
	float    sprintMod;
	float    acceleration;
	float    airAcceleration;
	float    friction;
	float    stopSpeed;
	float    jumpMagnitude;
	int      mass;

	// stamina (human only)
	int      staminaJumpCost;
	int      staminaSprintCost;
	int      staminaJogRestore;
	int      staminaWalkRestore;
	int      staminaStopRestore;

	int      cost;
	int      value;
} classAttributes_t;

typedef struct
{
	char   modelName[ MAX_QPATH ];
	float  modelScale;
	char   skinName[ MAX_QPATH ];
	float  shadowScale;
	char   hudName[ MAX_QPATH ];
	char   *humanName;

	vec3_t mins;
	vec3_t maxs;
	vec3_t crouchMaxs;
	vec3_t deadMins;
	vec3_t deadMaxs;
	int    viewheight;
	int    crouchViewheight;
	float  zOffset;
	vec3_t shoulderOffsets;
	bool segmented;

	class_t navMeshClass; // if not PCL_NONE, which model's navmesh to use
	int     navHandle;
} classModelConfig_t;

#define MAX_BUILDABLE_MODELS 3

// buildable item record
typedef struct
{
	buildable_t number;

	const char *name;
	const char *humanName;
	const char *info;
	const char *entityName;
	const char *icon;

	trType_t    traj;
	float       bounce;

	int         buildPoints;
	int         powerConsumption;
	int         unlockThreshold;

	int         health;
	int         regenRate;

	int         splashDamage;
	int         splashRadius;

	weapon_t    weapon; // used to look up weaponInfo_t for clientside effects
	int         meansOfDeath;

	team_t      team;
	weapon_t    buildWeapon;

	int         buildTime;
	bool    usable;

	float       minNormal;
	bool    invertNormal;

	bool    creepTest;
	int         creepSize;

	bool    transparentTest;
	bool    uniqueTest;
} buildableAttributes_t;

typedef struct
{
	char   models[ MAX_BUILDABLE_MODELS ][ MAX_QPATH ];

	float  modelScale;
	vec3_t modelRotation;
	vec3_t mins;
	vec3_t maxs;
	float  zOffset;
	float  oldScale;
	float  oldOffset;
} buildableModelConfig_t;

// weapon record
typedef struct
{
	weapon_t number;

	int      price;
	int      unlockThreshold;

	int      slots;

	const char *name;
	const char *humanName;
	const char *info;

	int      maxAmmo;
	int      maxClips;
	bool infiniteAmmo;
	bool usesEnergy;

	int      repeatRate1;
	int      repeatRate2;
	int      repeatRate3;
	int      reloadTime;
	float    knockbackScale;

	bool hasAltMode;
	bool hasThirdMode;

	bool canZoom;
	float    zoomFov;

	bool purchasable;
	bool longRanged;

	team_t   team;
} weaponAttributes_t;

// upgrade record
typedef struct
{
	upgrade_t number;

	int       price;
	int       unlockThreshold;

	int       slots;

	const char *name;
	const char *humanName;
	const char *info;

	const char *icon;

	bool  purchasable;
	bool  usable;

	team_t    team;
} upgradeAttributes_t;

// missile record
typedef struct
{
	// attributes
	missile_t      number;
	const char     *name;
	bool       pointAgainstWorld;
	int            damage;
	meansOfDeath_t meansOfDeath;
	int            splashDamage;
	int            splashRadius;
	meansOfDeath_t splashMeansOfDeath;
	int            clipmask;
	int            size;
	trType_t       trajectoryType;
	int            speed;
	float          lag;
	int            flags;
	bool       doKnockback;
	bool       doLocationalDamage;

	// display
	qhandle_t      model;
	float          modelScale;
	vec3_t         modelRotation;

	sfxHandle_t    sound;
	bool       usesDlight;
	float          dlight;
	float          dlightIntensity;
	vec3_t         dlightColor;
	int            renderfx;
	bool       usesSprite;
	qhandle_t      sprite;
	int            spriteSize;
	float          spriteCharge;
	qhandle_t      particleSystem;
	qhandle_t      trailSystem;
	bool       rotates;
	bool       usesAnim;
	int            animStartFrame;
	int            animNumFrames;
	int            animFrameRate;
	bool       animLooping;

	// impact
	bool       alwaysImpact;
	qhandle_t      impactParticleSystem;
	bool       impactFlightDirection;
	bool       usesImpactMark;
	qhandle_t      impactMark;
	qhandle_t      impactMarkSize;
	sfxHandle_t    impactSound[ 4 ];
	sfxHandle_t    impactFleshSound[ 4 ];
} missileAttributes_t;

// bg_utilities.c
bool BG_GetTrajectoryPitch( vec3_t origin, vec3_t target, float v0, float g,
                                vec2_t angles, vec3_t dir1, vec3_t dir2 );
void     BG_BuildEntityDescription( char *str, size_t size, entityState_t *es );
bool     BG_IsMainStructure( buildable_t buildable );
bool     BG_IsMainStructure( entityState_t *es );
void     BG_MoveOriginToBBOXCenter( vec3_t point, const vec3_t mins, const vec3_t maxs );

bool BG_WeaponIsFull(int weapon, int ammo, int clips );
bool BG_InventoryContainsWeapon( int weapon, const int stats[] );
int      BG_SlotsForInventory( int stats[] );
void     BG_AddUpgradeToInventory( int item, int stats[] );
void     BG_RemoveUpgradeFromInventory( int item, int stats[] );
bool BG_InventoryContainsUpgrade( int item, int stats[] );
void     BG_ActivateUpgrade( int item, int stats[] );
void     BG_DeactivateUpgrade( int item, int stats[] );
bool BG_UpgradeIsActive( int item, int stats[] );
bool BG_RotateAxis( vec3_t surfNormal, vec3_t inAxis[ 3 ],
                        vec3_t outAxis[ 3 ], bool inverse, bool ceiling );
void     BG_GetClientNormal( const playerState_t *ps, vec3_t normal );
void     BG_GetClientViewOrigin( const playerState_t *ps, vec3_t viewOrigin );
void     BG_PositionBuildableRelativeToPlayer( playerState_t *ps, const vec3_t mins, const vec3_t maxs,
                                               void ( *trace )( trace_t *, const vec3_t, const vec3_t,
                                               const vec3_t, const vec3_t, int, int, int ),
                                               vec3_t outOrigin, vec3_t outAngles, trace_t *tr );
int                         BG_GetValueOfPlayer( playerState_t *ps );
bool                    BG_PlayerCanChangeWeapon( playerState_t *ps );
weapon_t                    BG_GetPlayerWeapon( playerState_t *ps );
bool                    BG_PlayerLowAmmo( const playerState_t *ps, bool *energy );

void                        BG_PackEntityNumbers( entityState_t *es, const int *entityNums, unsigned int count );
int                         BG_UnpackEntityNumbers( entityState_t *es, int *entityNums, unsigned int count );

const buildableAttributes_t *BG_BuildableByName( const char *name );
const buildableAttributes_t *BG_BuildableByEntityName( const char *name );
const buildableAttributes_t *BG_Buildable( int buildable );

buildableModelConfig_t      *BG_BuildableModelConfig( int buildable );
void                        BG_BuildableBoundingBox( int buildable, vec3_t mins, vec3_t maxs );

const classAttributes_t     *BG_ClassByName( const char *name );

const classAttributes_t     *BG_Class( int pClass );

classModelConfig_t          *BG_ClassModelConfigByName( const char * );

classModelConfig_t          *BG_ClassModelConfig( int pClass );

void                        BG_ClassBoundingBox( int pClass, vec3_t mins, vec3_t maxs, vec3_t cmaxs,
                                                 vec3_t dmins, vec3_t dmaxs );
team_t                      BG_ClassTeam( int pClass );
bool                    BG_ClassHasAbility( int pClass, int ability );

int                         BG_ClassCanEvolveFromTo(int from, int to, int credits);
bool                    BG_AlienCanEvolve(int from, int credits);

weapon_t                  BG_WeaponNumberByName( const char *name );
const weaponAttributes_t  *BG_WeaponByName( const char *name );
const weaponAttributes_t  *BG_Weapon( int weapon );

const upgradeAttributes_t *BG_UpgradeByName( const char *name );
const upgradeAttributes_t *BG_Upgrade( int upgrade );

const missileAttributes_t *BG_MissileByName( const char *name );
const missileAttributes_t *BG_Missile( int missile );

const beaconAttributes_t  *BG_BeaconByName( const char *name );
const beaconAttributes_t  *BG_Beacon( int index );

meansOfDeath_t            BG_MeansOfDeathByName( const char *name );

void                      BG_InitAllConfigs();
void                      BG_UnloadAllConfigs();

// Parsers
bool                  BG_ReadWholeFile( const char *filename, char *buffer, int size);
bool                  BG_CheckConfigVars();
bool                  BG_NonSegModel( const char *filename );
void                      BG_ParseBuildableAttributeFile( const char *filename, buildableAttributes_t *ba );
void                      BG_ParseBuildableModelFile( const char *filename, buildableModelConfig_t *bc );
void                      BG_ParseClassAttributeFile( const char *filename, classAttributes_t *ca );
void                      BG_ParseClassModelFile( const char *filename, classModelConfig_t *cc );
void                      BG_ParseWeaponAttributeFile( const char *filename, weaponAttributes_t *wa );
void                      BG_ParseUpgradeAttributeFile( const char *filename, upgradeAttributes_t *ua );
void                      BG_ParseMissileAttributeFile( const char *filename, missileAttributes_t *ma );
void                      BG_ParseMissileDisplayFile( const char *filename, missileAttributes_t *ma );
void                      BG_ParseBeaconAttributeFile( const char *filename, beaconAttributes_t *ba );

// bg_teamprogress.c
#define NUM_UNLOCKABLES WP_NUM_WEAPONS + UP_NUM_UPGRADES + BA_NUM_BUILDABLES + PCL_NUM_CLASSES

typedef enum
{
	UNLT_WEAPON,
	UNLT_UPGRADE,
	UNLT_BUILDABLE,
	UNLT_CLASS,
	UNLT_NUM_UNLOCKABLETYPES
} unlockableType_t;

typedef struct {
	int num;
	int threshold;
} momentumThresholdIterator_t;

void     BG_InitUnlockackables();
void     BG_ImportUnlockablesFromMask( int team, int mask );
int      BG_UnlockablesMask( int team );
bool BG_WeaponUnlocked( int weapon );
bool BG_UpgradeUnlocked( int upgrade );
bool BG_BuildableUnlocked( int buildable );
bool BG_ClassUnlocked( int class_ );

unlockableType_t              BG_UnlockableType( int num );
int                           BG_UnlockableTypeIndex( int num );
momentumThresholdIterator_t BG_IterateMomentumThresholds( momentumThresholdIterator_t unlockableIter, team_t team, int *threshold, bool *unlocked );
#ifdef BUILD_SGAME
void     G_UpdateUnlockables();
#endif
#ifdef BUILD_CGAME
void     CG_UpdateUnlockables( playerState_t *ps );
#endif
#ifdef BUILD_UI
void     UI_UpdateUnlockables();
#endif

// content masks
#define MASK_ALL         ( -1 )
#define MASK_SOLID       ( CONTENTS_SOLID )
#define MASK_PLAYERSOLID ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY )
#define MASK_DEADSOLID   ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP )
#define MASK_WATER       ( CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME )
#define MASK_OPAQUE      ( CONTENTS_SOLID | CONTENTS_SLIME | CONTENTS_LAVA )
#define MASK_SHOT        ( CONTENTS_SOLID | CONTENTS_BODY )
#define MASK_ENTITY      ( CONTENTS_MOVER )

void     *BG_Alloc( int size );
void     BG_InitMemory();
void     BG_Free( void *ptr );
void     BG_DefragmentMemory();
void     BG_MemoryInfo();

void     BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void     BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result );

void     BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps );

void     BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, bool snap );
void     BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, bool snap );

#define MAX_ARENAS      1024
#define MAX_ARENAS_TEXT 8192

#define MAX_BOTS        1024
#define MAX_BOTS_TEXT   8192

float    atof_neg( char *token, bool allowNegative );
int      atoi_neg( char *token, bool allowNegative );

void     BG_ParseCSVEquipmentList( const char *string, weapon_t *weapons, int weaponsSize,
                                   upgrade_t *upgrades, int upgradesSize );
void     BG_ParseCSVClassList( const char *string, class_t *classes, int classesSize );
void     BG_ParseCSVBuildableList( const char *string, buildable_t *buildables, int buildablesSize );
void     BG_InitAllowedGameElements();
bool BG_WeaponDisabled( int weapon );
bool BG_UpgradeDisabled( int upgrade );

bool BG_ClassDisabled( int class_ );
bool BG_BuildableDisabled( int buildable );

weapon_t BG_PrimaryWeapon( int stats[] );

// Friendly Fire Flags
#define FFF_HUMANS             1
#define FFF_ALIENS             2
#define FFF_BUILDABLES         4

// bg_voice.c
#define MAX_VOICES             8
#define MAX_VOICE_NAME_LEN     16
#define MAX_VOICE_CMD_LEN      16
#define VOICE_ENTHUSIASM_DECAY 0.5f // enthusiasm lost per second

typedef enum
{
  VOICE_CHAN_ALL,
  VOICE_CHAN_TEAM,
  VOICE_CHAN_LOCAL,

  VOICE_CHAN_NUM_CHANS
} voiceChannel_t;

typedef struct voiceTrack_s
{
#ifdef BUILD_CGAME
	sfxHandle_t         track;
	int                 duration;
#endif
	char                *text;
	int                 enthusiasm;
	int                 team;
	int                 pClass;
	int                 weapon;
	struct voiceTrack_s *next;
} voiceTrack_t;

typedef struct voiceCmd_s
{
	char              cmd[ MAX_VOICE_CMD_LEN ];
	voiceTrack_t      *tracks;
	struct voiceCmd_s *next;
} voiceCmd_t;

typedef struct voice_s
{
	char           name[ MAX_VOICE_NAME_LEN ];
	voiceCmd_t     *cmds;
	struct voice_s *next;
} voice_t;

voice_t      *BG_VoiceInit();
void         BG_PrintVoices( voice_t *voices, int debugLevel );

voice_t      *BG_VoiceByName( voice_t *head, const char *name );
voiceCmd_t   *BG_VoiceCmdFind( voiceCmd_t *head, const char *name, int *cmdNum );
voiceCmd_t   *BG_VoiceCmdByNum( voiceCmd_t *head, int num );
voiceTrack_t *BG_VoiceTrackByNum( voiceTrack_t *head, int num );

voiceTrack_t *BG_VoiceTrackFind( voiceTrack_t *head, int team,
                                 int pClass, int weapon,
                                 int enthusiasm, int *trackNum );

int  BG_LoadEmoticons( emoticon_t *emoticons, int num );

const char *BG_TeamName( int team );
const char *BG_TeamNamePlural( int team );

typedef struct
{
	const char *name;
} dummyCmd_t;
int cmdcmp( const void *a, const void *b );

char *Quote( const char *str );
char *Substring( const char *in, int start, int count );
char *BG_strdup( const char *string );

const char *Trans_GenderContext( gender_t gender );

//==================================================================
#endif /* BG_PUBLIC_H_ */
