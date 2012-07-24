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

// bg_public.h -- definitions shared by both the server game and client game modules

//tremulous balance header
#include "tremulous.h"

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame
#define GAME_VERSION       "base"

#define DEFAULT_GRAVITY    800

#define VOTE_TIME          30000 // 30 seconds before vote times out

#define MINS_Z             -24
#define DEFAULT_VIEWHEIGHT 26
#define CROUCH_VIEWHEIGHT  12
#define DEAD_VIEWHEIGHT    4 // height from ground

// QVM-specific
#ifdef Q3_VM
#define QVM_STATIC static
#else
#define QVM_STATIC
#endif

// player teams
typedef enum
{
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

  CS_ALIEN_STAGES,
  CS_HUMAN_STAGES,

  CS_MODELS,
  CS_SOUNDS = CS_MODELS + MAX_MODELS,
  CS_SHADERS = CS_SOUNDS + MAX_SOUNDS,
  CS_PARTICLE_SYSTEMS = CS_SHADERS + MAX_GAME_SHADERS,
  CS_PLAYERS = CS_PARTICLE_SYSTEMS + MAX_GAME_PARTICLE_SYSTEMS,
  CS_LOCATIONS = CS_PLAYERS + MAX_CLIENTS,

  CS_MAX = CS_LOCATIONS + MAX_LOCATIONS
};
// CS_MAX had better not be greater than MAX_CONFIGSTRINGS !

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
  PM_JETPACK, // jetpack physics
  PM_GRABBED, // like dead, but for when the player is still alive
  PM_DEAD, // no acceleration or turning, but free falling
  PM_FREEZE, // stuck in place with no control
  PM_INTERMISSION // no movement or status bar
} pmtype_t;

// pmtype_t categories
#define PM_Paralyzed( x ) ( ( x ) == PM_DEAD || ( x ) == PM_FREEZE || \
                            ( x ) == PM_INTERMISSION )
#define PM_Live( x )      ( ( x ) == PM_NORMAL || ( x ) == PM_JETPACK || \
                            ( x ) == PM_GRABBED )

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
#define PMF_TIME_LAND      0x000020 // pm_time is time before rejump
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

#define PMF_ALL_TIMES      ( PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_KNOCKBACK | PMF_TIME_WALLJUMP )

typedef struct
{
	int   pouncePayload;
	float fallVelocity;
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
	qboolean      noFootsteps; // if the game is setup for no footsteps by the server
	qboolean      autoWeaponHit[ 32 ];

	int           framecount;

	// results (out)
	int    numtouch;
	int    touchents[ MAXTOUCH ];

	vec3_t mins, maxs; // bounding box size

	int    watertype;
	int    waterlevel;

	float  xyspeed;

	// for fixed msec Pmove
	int pmove_fixed;
	int pmove_msec;

	// callbacks to test the world
	// these will be different functions during game and cgame
	/*void    (*trace)( trace_t *results, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );*/
	void ( *trace )( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
	                 const vec3_t end, int passEntityNum, int contentMask );

	int ( *pointcontents )( const vec3_t point, int passEntityNum );
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd );
void Pmove( pmove_t *pmove );

//===================================================================================

// player_state->stats[] indexes
typedef enum
{
  STAT_HEALTH,
  STAT_ITEMS,
  STAT_ACTIVEITEMS,
  STAT_WEAPON, // current primary weapon
  STAT_MAX_HEALTH, // health / armor limit
  STAT_CLASS, // player class (for aliens AND humans)
  STAT_TEAM, // player team
  STAT_STAMINA, // stamina (human only)
  STAT_STATE, // client states e.g. wall climbing
  STAT_MISC, // for uh...misc stuff (pounce, trample, lcannon)
  STAT_BUILDABLE, // which ghost model to display for building
  STAT_FALLDIST, // the distance the player fell
  STAT_VIEWLOCK // direction to lock the view in
  // netcode has space for 3 more
} statIndex_t;

#define SCA_WALLCLIMBER     0x00000001
#define SCA_TAKESFALLDAMAGE 0x00000002
#define SCA_CANZOOM         0x00000004
#define SCA_FOVWARPS        0x00000008
#define SCA_ALIENSENSE      0x00000010
#define SCA_CANUSELADDERS   0x00000020
#define SCA_WALLJUMPER      0x00000040

#define SS_WALLCLIMBING     0x00000001
#define SS_CREEPSLOWED      0x00000002
#define SS_SPEEDBOOST       0x00000004
#define SS_GRABBED          0x00000008
#define SS_BLOBLOCKED       0x00000010
#define SS_POISONED         0x00000020
#define SS_BOOSTED          0x00000040
#define SS_BOOSTEDWARNING   0x00000080 // booster poison is running out
#define SS_SLOWLOCKED       0x00000100
#define SS_CHARGING         0x00000200
#define SS_HEALING_ACTIVE   0x00000400 // medistat for humans, creep for aliens
#define SS_HEALING_2X       0x00000800 // medkit or double healing rate
#define SS_HEALING_3X       0x00001000 // triple healing rate

#define SB_VALID_TOGGLEBIT  0x00002000

// player_state->persistent[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
typedef enum
{
  PERS_SCORE, // !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
  PERS_HITS, // total points damage inflicted so damage beeps can sound on change
  PERS_SPAWNS, // how many spawns your team has
  PERS_SPECSTATE,
  PERS_SPAWN_COUNT, // incremented every respawn
  PERS_ATTACKER, // clientnum of last damage inflicter
  PERS_KILLED, // count of the number of times you died

  PERS_STATE,
  PERS_CREDIT, // human credit
  PERS_QUEUEPOS, // position in the spawn queue
  PERS_NEWWEAPON, // weapon to switch to
  PERS_BP,
  PERS_MARKEDBP
  // netcode has space for 3 more
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

#define EF_WARN_CHARGE      0x0020 // Lucifer Cannon is about to overcharge
#define EF_WALLCLIMB        0x0040 // wall walking
#define EF_WALLCLIMBCEILING 0x0080 // wall walking ceiling hack
#define EF_NODRAW           0x0100 // may have an event, but no model (unspawned items)
#define EF_FIRING           0x0200 // for lightning gun
#define EF_FIRING2          0x0400 // alt fire
#define EF_FIRING3          0x0800 // third fire
#define EF_MOVER_STOP       0x1000 // will push otherwise
#define EF_POISONCLOUDED    0x2000 // player hit with basilisk gas
#define EF_CONNECTION       0x4000 // draw a connection trouble sprite
#define EF_BLOBLOCKED       0x8000 // caught by a trapper

typedef enum
{
  HI_NONE,

  HI_TELEPORTER,
  HI_MEDKIT,

  HI_NUM_HOLDABLE
} holdable_t;

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
  WP_ALEVEL1_UPG,
  WP_ALEVEL2,
  WP_ALEVEL2_UPG,
  WP_ALEVEL3,
  WP_ALEVEL3_UPG,
  WP_ALEVEL4,

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
  WP_GRENADE,

  WP_LOCKBLOB_LAUNCHER,
  WP_HIVE,
  WP_TESLAGEN,
  WP_MGTURRET,

  //build weapons must remain in a block
  WP_ABUILD,
  WP_ABUILD2,
  WP_HBUILD,
  //ok?

  WP_NUM_WEAPONS
} weapon_t;

typedef enum
{
  UP_NONE,

  UP_LIGHTARMOUR,
  UP_HELMET,
  UP_MEDKIT,
  UP_BATTPACK,
  UP_JETPACK,
  UP_BATTLESUIT,
  UP_GRENADE,

  UP_AMMO,

  UP_NUM_UPGRADES
} upgrade_t;

// bitmasks for upgrade slots
#define SLOT_NONE     0x00000000
#define SLOT_HEAD     0x00000001
#define SLOT_TORSO    0x00000002
#define SLOT_ARMS     0x00000004
#define SLOT_LEGS     0x00000008
#define SLOT_BACKPACK 0x00000010
#define SLOT_WEAPON   0x00000020
#define SLOT_SIDEARM  0x00000040

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

  BA_H_SPAWN,

  BA_H_MGTURRET,
  BA_H_TESLAGEN,

  BA_H_ARMOURY,
  BA_H_DCC,
  BA_H_MEDISTAT,

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

  EV_NOAMMO,
  EV_CHANGE_WEAPON,
  EV_FIRE_WEAPON,
  EV_FIRE_WEAPON2,
  EV_FIRE_WEAPON3,

  EV_PLAYER_RESPAWN, // for fovwarp effects
  EV_PLAYER_TELEPORT_IN,
  EV_PLAYER_TELEPORT_OUT,

  EV_GRENADE_BOUNCE, // eventParm will be the soundindex

  EV_GENERAL_SOUND,
  EV_GLOBAL_SOUND, // no attenuation

  EV_BULLET_HIT_FLESH,
  EV_BULLET_HIT_WALL,

  EV_SHOTGUN,
  EV_MASS_DRIVER,

  EV_MISSILE_HIT,
  EV_MISSILE_MISS,
  EV_MISSILE_MISS_METAL,
  EV_TESLATRAIL,
  EV_BULLET, // otherEntity is the shooter

  EV_LEV1_GRAB,
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
  EV_HUMAN_BUILDABLE_EXPLOSION,
  EV_ALIEN_BUILDABLE_EXPLOSION,
  EV_ALIEN_ACIDTUBE,

  EV_MEDKIT_USED,

  EV_ALIEN_EVOLVE,
  EV_ALIEN_EVOLVE_FAILED,

  EV_DEBUG_LINE,
  EV_STOPLOOPINGSOUND,
  EV_TAUNT,

  EV_OVERMIND_ATTACK, // overmind under attack
  EV_OVERMIND_DYING, // overmind close to death
  EV_OVERMIND_SPAWNS, // overmind needs spawns

  EV_DCC_ATTACK, // dcc under attack

  EV_MGTURRET_SPINUP, // turret spinup sound should play

  EV_RPTUSE_SOUND, // trigger a sound
  EV_LEV2_ZAP
} entity_event_t;

typedef enum
{
  MN_NONE,

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
  MN_A_CLASSNOTATSTAGE,

  //shared build
  MN_B_NOROOM,
  MN_B_NORMAL,
  MN_B_CANNOT,
  MN_B_LASTSPAWN,
  MN_B_SUDDENDEATH,
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
  MN_H_NOROOMBSUITON,
  MN_H_NOROOMBSUITOFF,
  MN_H_ARMOURYBUILDTIMER,
  MN_H_DEADTOCLASS,
  MN_H_UNKNOWNSPAWNITEM,

  //human build
  MN_H_NOPOWERHERE,
  MN_H_NOBP,
  MN_H_NOTPOWERED,
  MN_H_NODCC,
  MN_H_ONEREACTOR,
  MN_H_RPTPOWERHERE,
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

  TORSO_GESTURE,

  TORSO_ATTACK,
  TORSO_ATTACK2,

  TORSO_DROP,
  TORSO_RAISE,

  TORSO_STAND,
  TORSO_STAND2,

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

  BANIM_CONSTRUCT1,
  BANIM_CONSTRUCT2,

  BANIM_IDLE1,
  BANIM_IDLE2,
  BANIM_IDLE3,

  BANIM_ATTACK1,
  BANIM_ATTACK2,

  BANIM_SPAWN1,
  BANIM_SPAWN2,

  BANIM_PAIN1,
  BANIM_PAIN2,

  BANIM_DESTROY1,
  BANIM_DESTROY2,
  BANIM_DESTROYED,

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
	qboolean  clearOrigin;

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
  PCL_ALIEN_LEVEL1_UPG,
  PCL_ALIEN_LEVEL2,
  PCL_ALIEN_LEVEL2_UPG,
  PCL_ALIEN_LEVEL3,
  PCL_ALIEN_LEVEL3_UPG,
  PCL_ALIEN_LEVEL4,

  //human class
  PCL_HUMAN,
  PCL_HUMAN_BSUIT,

  PCL_NUM_CLASSES
} class_t;

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
  SAY_DEFAULT
} saymode_t;

// means of death
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
  MOD_GRENADE,
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
  MOD_LEVEL1_PCLOUD,
  MOD_LEVEL3_CLAW,
  MOD_LEVEL3_POUNCE,
  MOD_LEVEL3_BOUNCEBALL,
  MOD_LEVEL2_CLAW,
  MOD_LEVEL2_ZAP,
  MOD_LEVEL4_CLAW,
  MOD_LEVEL4_TRAMPLE,
  MOD_LEVEL4_CRUSH,

  MOD_SLOWBLOB,
  MOD_POISON,
  MOD_SWARM,

  MOD_HSPAWN,
  MOD_TESLAGEN,
  MOD_MGTURRET,
  MOD_REACTOR,

  MOD_ASPAWN,
  MOD_ATUBE,
  MOD_OVERMIND,
  MOD_DECONSTRUCT,
  MOD_REPLACE,
  MOD_NOCREEP
} meansOfDeath_t;

//---------------------------------------------------------

// player class record
typedef struct
{
	class_t  number;

	const char *name;
	const char *info;

	int      stages;

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
	float    acceleration;
	float    airAcceleration;
	float    friction;
	float    stopSpeed;
	float    jumpMagnitude;
	float    knockbackScale;

	int      children[ 3 ];
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
	char   humanName[ MAX_STRING_CHARS ];

	vec3_t mins;
	vec3_t maxs;
	vec3_t crouchMaxs;
	vec3_t deadMins;
	vec3_t deadMaxs;
	int    viewheight;
	int    crouchViewheight;
	float  zOffset;
	vec3_t shoulderOffsets;
} classConfig_t;

//stages
typedef enum
{
  S1,
  S2,
  S3
} stage_t;

#define MAX_BUILDABLE_MODELS 3

// buildable item record
typedef struct
{
	buildable_t number;

	const char  *name;
	const char  *humanName;
	const char  *info;
	const char  *entityName;

	trType_t    traj;
	float       bounce;

	int         buildPoints;
	int         stages;

	int         health;
	int         regenRate;

	int         splashDamage;
	int         splashRadius;

	int         meansOfDeath;

	team_t      team;
	weapon_t    buildWeapon;

	int         idleAnim;

	int         nextthink;
	int         buildTime;
	qboolean    usable;

	int         turretRange;
	int         turretFireSpeed;
	weapon_t    turretProjType;

	float       minNormal;
	qboolean    invertNormal;

	qboolean    creepTest;
	int         creepSize;

	qboolean    dccTest;
	qboolean    transparentTest;
	qboolean    uniqueTest;

	int         value;
} buildableAttributes_t;

typedef struct
{
	char   models[ MAX_BUILDABLE_MODELS ][ MAX_QPATH ];

	float  modelScale;
	vec3_t mins;
	vec3_t maxs;
	float  zOffset;
} buildableConfig_t;

// weapon record
typedef struct
{
	weapon_t number;

	int      price;
	int      stages;

	int      slots;

	const char *name;
	const char *humanName;
	const char *info;

	int      maxAmmo;
	int      maxClips;
	qboolean infiniteAmmo;
	qboolean usesEnergy;

	int      repeatRate1;
	int      repeatRate2;
	int      repeatRate3;
	int      reloadTime;
	float    knockbackScale;

	qboolean hasAltMode;
	qboolean hasThirdMode;

	qboolean canZoom;
	float    zoomFov;

	qboolean purchasable;
	qboolean longRanged;

	team_t   team;
} weaponAttributes_t;

// upgrade record
typedef struct
{
	upgrade_t number;

	int       price;
	int       stages;

	int       slots;

	const char *name;
	const char *humanName;
	const char *info;

	const char *icon;

	qboolean  purchasable;
	qboolean  usable;

	team_t    team;
} upgradeAttributes_t;

qboolean BG_WeaponIsFull( weapon_t weapon, int stats[], int ammo, int clips );
qboolean BG_InventoryContainsWeapon( int weapon, int stats[] );
int      BG_SlotsForInventory( int stats[] );
void     BG_AddUpgradeToInventory( int item, int stats[] );
void     BG_RemoveUpgradeFromInventory( int item, int stats[] );
qboolean BG_InventoryContainsUpgrade( int item, int stats[] );
void     BG_ActivateUpgrade( int item, int stats[] );
void     BG_DeactivateUpgrade( int item, int stats[] );
qboolean BG_UpgradeIsActive( int item, int stats[] );
qboolean BG_RotateAxis( vec3_t surfNormal, vec3_t inAxis[ 3 ],
                        vec3_t outAxis[ 3 ], qboolean inverse, qboolean ceiling );
void     BG_GetClientNormal( const playerState_t *ps, vec3_t normal );
void     BG_GetClientViewOrigin( const playerState_t *ps, vec3_t viewOrigin );
void     BG_PositionBuildableRelativeToPlayer( playerState_t *ps,
    const vec3_t mins, const vec3_t maxs,
    void ( *trace )( trace_t *, const vec3_t, const vec3_t,
                     const vec3_t, const vec3_t, int, int ),
    vec3_t outOrigin, vec3_t outAngles, trace_t *tr );
int                         BG_GetValueOfPlayer( playerState_t *ps );
qboolean                    BG_PlayerCanChangeWeapon( playerState_t *ps );
int                         BG_PlayerPoisonCloudTime( playerState_t *ps );
weapon_t                    BG_GetPlayerWeapon( playerState_t *ps );
qboolean                    BG_HasEnergyWeapon( playerState_t *ps );

void                        BG_PackEntityNumbers( entityState_t *es, const int *entityNums, int count );
int                         BG_UnpackEntityNumbers( entityState_t *es, int *entityNums, int count );

const buildableAttributes_t *BG_BuildableByName( const char *name );
const buildableAttributes_t *BG_BuildableByEntityName( const char *name );
const buildableAttributes_t *BG_Buildable( buildable_t buildable );
qboolean                    BG_BuildableAllowedInStage( buildable_t buildable,
    stage_t stage );

buildableConfig_t           *BG_BuildableConfig( buildable_t buildable );
void                        BG_BuildableBoundingBox( buildable_t buildable,
    vec3_t mins, vec3_t maxs );
void                        BG_InitBuildableConfigs( void );

const classAttributes_t     *BG_ClassByName( const char *name );

const classAttributes_t     *BG_Class( class_t class );
qboolean                    BG_ClassAllowedInStage( class_t class,
    stage_t stage );

classConfig_t               *BG_ClassConfig( class_t class );

void                        BG_ClassBoundingBox( class_t class, vec3_t mins,
    vec3_t maxs, vec3_t cmaxs,
    vec3_t dmins, vec3_t dmaxs );
qboolean                    BG_ClassHasAbility( class_t class, int ability );
int                         BG_ClassCanEvolveFromTo( class_t fclass,
    class_t tclass,
    int credits, int alienStage, int num );

qboolean                  BG_AlienCanEvolve( class_t class, int credits, int alienStage );

void                      BG_InitClassConfigs( void );

const weaponAttributes_t  *BG_WeaponByName( const char *name );
const weaponAttributes_t  *BG_Weapon( weapon_t weapon );
qboolean                  BG_WeaponAllowedInStage( weapon_t weapon,
    stage_t stage );

const upgradeAttributes_t *BG_UpgradeByName( const char *name );
const upgradeAttributes_t *BG_Upgrade( upgrade_t upgrade );
qboolean                  BG_UpgradeAllowedInStage( upgrade_t upgrade,
    stage_t stage );

// content masks
#define MASK_ALL         ( -1 )
#define MASK_SOLID       ( CONTENTS_SOLID )
#define MASK_PLAYERSOLID ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY )
#define MASK_DEADSOLID   ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP )
#define MASK_WATER       ( CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME )
#define MASK_OPAQUE      ( CONTENTS_SOLID | CONTENTS_SLIME | CONTENTS_LAVA )
#define MASK_SHOT        ( CONTENTS_SOLID | CONTENTS_BODY )

void     *BG_Alloc( int size );
void     BG_InitMemory( void );
void     BG_Free( void *ptr );
void     BG_DefragmentMemory( void );
void     BG_MemoryInfo( void );

void     BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void     BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result );

void     BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps );

void     BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap );
void     BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap );

qboolean BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime );

#define ARENAS_PER_TIER 4
#define MAX_ARENAS      1024
#define MAX_ARENAS_TEXT 8192

#define MAX_BOTS        1024
#define MAX_BOTS_TEXT   8192

float    atof_neg( char *token, qboolean allowNegative );
int      atoi_neg( char *token, qboolean allowNegative );

void     BG_ParseCSVEquipmentList( const char *string, weapon_t *weapons, int weaponsSize,
                                   upgrade_t *upgrades, int upgradesSize );
void     BG_ParseCSVClassList( const char *string, class_t *classes, int classesSize );
void     BG_ParseCSVBuildableList( const char *string, buildable_t *buildables, int buildablesSize );
void     BG_InitAllowedGameElements( void );
qboolean BG_WeaponIsAllowed( weapon_t weapon );
qboolean BG_UpgradeIsAllowed( upgrade_t upgrade );

qboolean BG_ClassIsAllowed( class_t class );
qboolean BG_BuildableIsAllowed( buildable_t buildable );

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
#ifdef CGAME
	sfxHandle_t         track;
	int                 duration;
#endif
	char                *text;
	int                 enthusiasm;
	int                 team;
	int                    class;
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

voice_t      *BG_VoiceInit( void );
void         BG_PrintVoices( voice_t *voices, int debugLevel );

voice_t      *BG_VoiceByName( voice_t *head, const char *name );
voiceCmd_t   *BG_VoiceCmdFind( voiceCmd_t *head, const char *name, int *cmdNum );
voiceCmd_t   *BG_VoiceCmdByNum( voiceCmd_t *head, int num );
voiceTrack_t *BG_VoiceTrackByNum( voiceTrack_t *head, int num );

voiceTrack_t *BG_VoiceTrackFind( voiceTrack_t *head, team_t team,
                                 class_t class, weapon_t weapon,
                                 int enthusiasm, int *trackNum );

int  BG_LoadEmoticons( emoticon_t *emoticons, int num );

const char *BG_TeamName( team_t team );
const char *BG_TeamNamePlural( team_t team );

typedef struct
{
	const char *name;
} dummyCmd_t;
int cmdcmp( const void *a, const void *b );

char *Quote( const char *str );
char *Substring( const char *in, int start, int count );

const char *Trans_GenderContext( gender_t gender );
