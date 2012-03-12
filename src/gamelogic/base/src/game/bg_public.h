/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

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
#define GAME_VERSION            "base"

#define DEFAULT_GRAVITY         800

#define SCORE_NOT_PRESENT       -9999 // for the CS_SCORES[12] when only one player is present

#define VOTE_TIME               30000 // 30 seconds before vote times out

#define MINS_Z                  -24
#define DEFAULT_VIEWHEIGHT      26
#define CROUCH_VIEWHEIGHT       12
#define DEAD_VIEWHEIGHT         -14 //TA: watch for mins[ 2 ] less than this causing

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define CS_MUSIC            2
#define CS_MESSAGE          3   // from the map worldspawn's message field
#define CS_MOTD             4   // g_motd string for server message of the day
#define CS_WARMUP           5   // server time when the match will be restarted
#define CS_SCORES1          6
#define CS_SCORES2          7
#define CS_VOTE_TIME        8
#define CS_VOTE_STRING      9
#define CS_VOTE_YES         10
#define CS_VOTE_NO          11

#define CS_TEAMVOTE_TIME    12
#define CS_TEAMVOTE_STRING  14
#define CS_TEAMVOTE_YES     16
#define CS_TEAMVOTE_NO      18

#define CS_GAME_VERSION     20
#define CS_LEVEL_START_TIME 21    // so the timer only shows the current level
#define CS_INTERMISSION     22    // when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS       23    // string indicating flag status in CTF
#define CS_SHADERSTATE      24
#define CS_BOTINFO          25
#define CS_CLIENTS_READY    26    //TA: following suggestion in STAT_ enum STAT_CLIENTS_READY becomes a configstring

//TA: extra stuff:
#define CS_BUILDPOINTS      28
#define CS_STAGES           29
#define CS_SPAWNS           30

#define CS_MODELS           33
#define CS_SOUNDS           (CS_MODELS+MAX_MODELS)
#define CS_SHADERS          (CS_SOUNDS+MAX_SOUNDS)
#define CS_PARTICLE_SYSTEMS (CS_SHADERS+MAX_GAME_SHADERS)
#define CS_PLAYERS          (CS_PARTICLE_SYSTEMS+MAX_GAME_PARTICLE_SYSTEMS)
#define CS_PRECACHES        (CS_PLAYERS+MAX_CLIENTS)
#define CS_LOCATIONS        (CS_PRECACHES+MAX_CLIENTS)

#define CS_MAX              (CS_LOCATIONS+MAX_LOCATIONS)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
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
  PM_NORMAL,        // can accelerate and turn
  PM_NOCLIP,        // noclip movement
  PM_SPECTATOR,     // still run into walls
  PM_JETPACK,       // jetpack physics
  PM_GRABBED,       // like dead, but for when the player is still live
  PM_DEAD,          // no acceleration or turning, but free falling
  PM_FREEZE,        // stuck in place with no control
  PM_INTERMISSION,  // no movement or status bar
  PM_SPINTERMISSION // no movement or status bar
} pmtype_t;

typedef enum
{
  WEAPON_READY,
  WEAPON_RAISING,
  WEAPON_DROPPING,
  WEAPON_FIRING,
  WEAPON_RELOADING
} weaponstate_t;

// pmove->pm_flags
#define PMF_DUCKED          1
#define PMF_JUMP_HELD       2
#define PMF_CROUCH_HELD     4
#define PMF_BACKWARDS_JUMP  8       // go into backwards land
#define PMF_BACKWARDS_RUN   16      // coast down to backwards run
#define PMF_TIME_LAND       32      // pm_time is time before rejump
#define PMF_TIME_KNOCKBACK  64      // pm_time is an air-accelerate only time
#define PMF_TIME_WATERJUMP  256     // pm_time is waterjump
#define PMF_RESPAWNED       512     // clear after attack and jump buttons come up
#define PMF_USE_ITEM_HELD   1024
#define PMF_WEAPON_RELOAD   2048    //TA: force a weapon switch
#define PMF_FOLLOW          4096    // spectate following another player
#define PMF_QUEUED          8192    //TA: player is queued
#define PMF_TIME_WALLJUMP   16384   //TA: for limiting wall jumping
#define PMF_CHARGE          32768   //TA: keep track of pouncing
#define PMF_WEAPON_SWITCH   65536   //TA: force a weapon switch


#define PMF_ALL_TIMES (PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK|PMF_TIME_WALLJUMP)

#define MAXTOUCH  32
typedef struct
{
  // state (in / out)
  playerState_t *ps;

  // command (in)
  usercmd_t     cmd;
  int           tracemask;      // collide against these types of surfaces
  int           debugLevel;     // if set, diagnostic output will be printed
  qboolean      noFootsteps;    // if the game is setup for no footsteps by the server
  qboolean      autoWeaponHit[ 32 ]; //FIXME: TA: remind myself later this might be a problem

  int           framecount;

  // results (out)
  int           numtouch;
  int           touchents[ MAXTOUCH ];

  vec3_t        mins, maxs;     // bounding box size

  int           watertype;
  int           waterlevel;

  float         xyspeed;

  // for fixed msec Pmove
  int           pmove_fixed;
  int           pmove_msec;

  // callbacks to test the world
  // these will be different functions during game and cgame
  /*void    (*trace)( trace_t *results, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );*/
  void          (*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
                          const vec3_t end, int passEntityNum, int contentMask );


  int           (*pointcontents)( const vec3_t point, int passEntityNum );
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
  STAT_SLOTS,           //TA: tracks the amount of stuff human players are carrying
  STAT_ACTIVEITEMS,
  STAT_WEAPONS,         // 16 bit fields
  STAT_WEAPONS2,        //TA: another 16 bits to push the max weapon count up
  STAT_MAX_HEALTH, // health / armor limit, changable by handicap
  STAT_PCLASS,    //TA: player class (for aliens AND humans)
  STAT_PTEAM,     //TA: player team
  STAT_STAMINA,   //TA: stamina (human only)
  STAT_STATE,     //TA: client states e.g. wall climbing
  STAT_MISC,      //TA: for uh...misc stuff
  STAT_BUILDABLE, //TA: which ghost model to display for building
  STAT_BOOSTTIME, //TA: time left for boost (alien only)
  STAT_FALLDIST,  //TA: the distance the player fell
  STAT_VIEWLOCK   //TA: direction to lock the view in
} statIndex_t;

#define SCA_WALLCLIMBER         0x00000001
#define SCA_TAKESFALLDAMAGE     0x00000002
#define SCA_CANZOOM             0x00000004
#define SCA_NOWEAPONDRIFT       0x00000008
#define SCA_FOVWARPS            0x00000010
#define SCA_ALIENSENSE          0x00000020
#define SCA_CANUSELADDERS       0x00000040
#define SCA_WALLJUMPER          0x00000080

#define SS_WALLCLIMBING         0x00000001
#define SS_WALLCLIMBINGCEILING  0x00000002
#define SS_CREEPSLOWED          0x00000004
#define SS_SPEEDBOOST           0x00000008
#define SS_INFESTING            0x00000010
#define SS_GRABBED              0x00000020
#define SS_BLOBLOCKED           0x00000040
#define SS_POISONED             0x00000080
#define SS_HOVELING             0x00000100
#define SS_BOOSTED              0x00000200
#define SS_SLOWLOCKED           0x00000400
#define SS_POISONCLOUDED        0x00000800
#define SS_MEDKIT_ACTIVE        0x00001000
#define SS_CHARGING             0x00002000

#define SB_VALID_TOGGLEBIT      0x00004000

#define MAX_STAMINA             1000

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
typedef enum
{
  PERS_SCORE,           // !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
  PERS_HITS,            // total points damage inflicted so damage beeps can sound on change
  PERS_RANK,
  PERS_TEAM,
  PERS_SPAWN_COUNT,     // incremented every respawn
  PERS_ATTACKER,        // clientnum of last damage inflicter
  PERS_KILLED,          // count of the number of times you died

  //TA:
  PERS_STATE,
  PERS_CREDIT,    // human credit
  PERS_BANK,      // human credit in the bank
  PERS_QUEUEPOS,  // position in the spawn queue
  PERS_NEWWEAPON  // weapon to switch to
} persEnum_t;

#define PS_WALLCLIMBINGFOLLOW   0x00000001
#define PS_WALLCLIMBINGTOGGLE   0x00000002
#define PS_NONSEGMODEL          0x00000004

// entityState_t->eFlags
#define EF_DEAD             0x00000001    // don't draw a foe marker over players with EF_DEAD
#define EF_TELEPORT_BIT     0x00000002    // toggled every time the origin abruptly changes
#define EF_PLAYER_EVENT     0x00000004
#define EF_BOUNCE           0x00000008    // for missiles
#define EF_BOUNCE_HALF      0x00000010    // for missiles
#define EF_NO_BOUNCE_SOUND  0x00000020    // for missiles
#define EF_WALLCLIMB        0x00000040    // TA: wall walking
#define EF_WALLCLIMBCEILING 0x00000080    // TA: wall walking ceiling hack
#define EF_NODRAW           0x00000100    // may have an event, but no model (unspawned items)
#define EF_FIRING           0x00000200    // for lightning gun
#define EF_FIRING2          0x00000400    // alt fire
#define EF_FIRING3          0x00000800    // third fire
#define EF_MOVER_STOP       0x00001000    // will push otherwise
#define EF_TALK             0x00002000    // draw a talk balloon
#define EF_CONNECTION       0x00004000    // draw a connection trouble sprite
#define EF_VOTED            0x00008000    // already cast a vote
#define EF_TEAMVOTED        0x00010000    // already cast a vote
#define EF_BLOBLOCKED       0x00020000    // TA: caught by a trapper
#define EF_REAL_LIGHT       0x00040000    // TA: light sprites according to ambient light

typedef enum
{
  PW_NONE,

  PW_QUAD,
  PW_BATTLESUIT,
  PW_HASTE,
  PW_INVIS,
  PW_REGEN,
  PW_FLIGHT,

  PW_REDFLAG,
  PW_BLUEFLAG,
  PW_BALL,

  PW_NUM_POWERUPS
} powerup_t;

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
  WP_PULSE_RIFLE,
  WP_FLAMER,
  WP_LUCIFER_CANNON,
  WP_GRENADE,

  WP_LOCKBLOB_LAUNCHER,
  WP_HIVE,
  WP_TESLAGEN,
  WP_MGTURRET,

  //build weapons must remain in a block
  WP_ABUILD,
  WP_ABUILD2,
  WP_HBUILD2,
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

typedef enum
{
  WUT_NONE,

  WUT_ALIENS,
  WUT_HUMANS,

  WUT_NUM_TEAMS
} WUTeam_t;

//TA: bitmasks for upgrade slots
#define SLOT_NONE       0x00000000
#define SLOT_HEAD       0x00000001
#define SLOT_TORSO      0x00000002
#define SLOT_ARMS       0x00000004
#define SLOT_LEGS       0x00000008
#define SLOT_BACKPACK   0x00000010
#define SLOT_WEAPON     0x00000020
#define SLOT_SIDEARM    0x00000040

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

  BA_A_HOVEL,

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

typedef enum
{
  BIT_NONE,

  BIT_ALIENS,
  BIT_HUMANS,

  BIT_NUM_TEAMS
} buildableTeam_t;

#define B_HEALTH_BITS       5
#define B_HEALTH_SCALE      (float)((1<<B_HEALTH_BITS)-1)

#define B_SPAWNED_TOGGLEBIT 0x00000020
#define B_POWERED_TOGGLEBIT 0x00000040
#define B_DCCED_TOGGLEBIT   0x00000080


// reward sounds (stored in ps->persistant[PERS_PLAYEREVENTS])
#define PLAYEREVENT_DENIEDREWARD      0x0001
#define PLAYEREVENT_GAUNTLETREWARD    0x0002
#define PLAYEREVENT_HOLYSHIT          0x0004

// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define EV_EVENT_BIT1   0x00000100
#define EV_EVENT_BIT2   0x00000200
#define EV_EVENT_BITS   (EV_EVENT_BIT1|EV_EVENT_BIT2)

#define EVENT_VALID_MSEC  300

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

  EV_PLAYER_RESPAWN, //TA: for fovwarp effects
  EV_PLAYER_TELEPORT_IN,
  EV_PLAYER_TELEPORT_OUT,

  EV_GRENADE_BOUNCE,    // eventParm will be the soundindex

  EV_GENERAL_SOUND,
  EV_GLOBAL_SOUND,    // no attenuation

  EV_BULLET_HIT_FLESH,
  EV_BULLET_HIT_WALL,

  EV_SHOTGUN,

  EV_MISSILE_HIT,
  EV_MISSILE_MISS,
  EV_MISSILE_MISS_METAL,
  EV_TESLATRAIL,
  EV_BULLET,        // otherEntity is the shooter

  EV_LEV1_GRAB,
  EV_LEV4_CHARGE_PREPARE,
  EV_LEV4_CHARGE_START,

  EV_PAIN,
  EV_DEATH1,
  EV_DEATH2,
  EV_DEATH3,
  EV_OBITUARY,

  EV_GIB_PLAYER,      // gib a previously living player

  EV_BUILD_CONSTRUCT, //TA
  EV_BUILD_DESTROY,   //TA
  EV_BUILD_DELAY,     //TA: can't build yet
  EV_BUILD_REPAIR,    //TA: repairing buildable
  EV_BUILD_REPAIRED,  //TA: buildable has full health
  EV_HUMAN_BUILDABLE_EXPLOSION,
  EV_ALIEN_BUILDABLE_EXPLOSION,
  EV_ALIEN_ACIDTUBE,

  EV_MEDKIT_USED,

  EV_ALIEN_EVOLVE,
  EV_ALIEN_EVOLVE_FAILED,

  EV_DEBUG_LINE,
  EV_STOPLOOPINGSOUND,
  EV_TAUNT,

  EV_OVERMIND_ATTACK, //TA: overmind under attack
  EV_OVERMIND_DYING,  //TA: overmind close to death
  EV_OVERMIND_SPAWNS, //TA: overmind needs spawns

  EV_DCC_ATTACK,      //TA: dcc under attack

  EV_RPTUSE_SOUND     //TA: trigger a sound
} entity_event_t;

typedef enum
{
  MN_TEAM,
  MN_A_TEAMFULL,
  MN_H_TEAMFULL,

  //alien stuff
  MN_A_CLASS,
  MN_A_BUILD,
  MN_A_INFEST,
  MN_A_HOVEL_OCCUPIED,
  MN_A_HOVEL_BLOCKED,
  MN_A_NOEROOM,
  MN_A_TOOCLOSE,
  MN_A_NOOVMND_EVOLVE,

  //alien build
  MN_A_SPWNWARN,
  MN_A_OVERMIND,
  MN_A_NOASSERT,
  MN_A_NOCREEP,
  MN_A_NOOVMND,
  MN_A_NOROOM,
  MN_A_NORMAL,
  MN_A_HOVEL,
  MN_A_HOVEL_EXIT,

  //human stuff
  MN_H_SPAWN,
  MN_H_BUILD,
  MN_H_ARMOURY,
  MN_H_NOSLOTS,
  MN_H_NOFUNDS,
  MN_H_ITEMHELD,

  //human build
  MN_H_REPEATER,
  MN_H_NOPOWER,
  MN_H_NOTPOWERED,
  MN_H_NODCC,
  MN_H_REACTOR,
  MN_H_NOROOM,
  MN_H_NORMAL,
  MN_H_TNODEWARN,
  MN_H_RPTWARN,
  MN_H_RPTWARN2
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

//TA: for buildable animations
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

typedef struct animation_s
{
  int   firstFrame;
  int   numFrames;
  int   loopFrames;     // 0 to numFrames
  int   frameLerp;      // msec between frames
  int   initialLerp;    // msec to get to first frame
  int   reversed;     // true if animation is reversed
  int   flipflop;     // true if animation should flipflop back to base
} animation_t;


// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define ANIM_TOGGLEBIT    0x80
#define ANIM_FORCEBIT     0x40


typedef enum
{
  TEAM_FREE,
  TEAM_SPECTATOR,

  TEAM_NUM_TEAMS
} team_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME   1000

// How many players on the overlay
#define TEAM_MAXOVERLAY   32

//TA: player classes
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
} pClass_t;

// modes of text communication
#define SAY_ALL 0
#define SAY_TEAM 1
#define SAY_TELL 2

//TA: player teams
typedef enum
{
  PTE_NONE,
  PTE_ALIENS,
  PTE_HUMANS,

  PTE_NUM_TEAMS
} pTeam_t;


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
  MOD_LEVEL4_CHARGE,

  MOD_SLOWBLOB,
  MOD_POISON,
  MOD_SWARM,

  MOD_HSPAWN,
  MOD_TESLAGEN,
  MOD_MGTURRET,
  MOD_REACTOR,

  MOD_ASPAWN,
  MOD_ATUBE,
  MOD_OVERMIND
} meansOfDeath_t;


//---------------------------------------------------------

//TA: player class record
typedef struct
{
  int       classNum;

  char      *className;
  char      *humanName;

  char      *modelName;
  float     modelScale;
  char      *skinName;
  float     shadowScale;

  char      *hudName;

  int       stages;

  vec3_t    mins;
  vec3_t    maxs;
  vec3_t    crouchMaxs;
  vec3_t    deadMins;
  vec3_t    deadMaxs;
  float     zOffset;

  int       viewheight;
  int       crouchViewheight;

  int       health;
  float     fallDamage;
  int       regenRate;

  int       abilities;

  weapon_t  startWeapon;

  float     buildDist;

  int       fov;
  float     bob;
  float     bobCycle;
  int       steptime;

  float     speed;
  float     acceleration;
  float     airAcceleration;
  float     friction;
  float     stopSpeed;
  float     jumpMagnitude;
  float     knockbackScale;

  int       children[ 3 ];
  int       cost;
  int       value;
} classAttributes_t;

typedef struct
{
  char      modelName[ MAX_QPATH ];
  float     modelScale;
  char      skinName[ MAX_QPATH ];
  float     shadowScale;
  char      hudName[ MAX_QPATH ];
  char      humanName[ MAX_STRING_CHARS ];

  vec3_t    mins;
  vec3_t    maxs;
  vec3_t    crouchMaxs;
  vec3_t    deadMins;
  vec3_t    deadMaxs;
  float     zOffset;
} classAttributeOverrides_t;

//stages
typedef enum
{
  S1,
  S2,
  S3
} stage_t;

#define MAX_BUILDABLE_MODELS 4

//TA: buildable item record
typedef struct
{
  int       buildNum;

  char      *buildName;
  char      *humanName;
  char      *entityName;

  char      *models[ MAX_BUILDABLE_MODELS ];
  float     modelScale;

  vec3_t    mins;
  vec3_t    maxs;
  float     zOffset;

  trType_t  traj;
  float     bounce;

  int       buildPoints;
  int       stages;

  int       health;
  int       regenRate;

  int       splashDamage;
  int       splashRadius;

  int       meansOfDeath;

  int       team;
  weapon_t  buildWeapon;

  int       idleAnim;

  int       nextthink;
  int       buildTime;
  qboolean  usable;

  int       turretRange;
  int       turretFireSpeed;
  weapon_t  turretProjType;

  float     minNormal;
  qboolean  invertNormal;

  qboolean  creepTest;
  int       creepSize;

  qboolean  dccTest;
  qboolean  reactorTest;
} buildableAttributes_t;

typedef struct
{
  char      models[ MAX_BUILDABLE_MODELS ][ MAX_QPATH ];

  float     modelScale;
  vec3_t    mins;
  vec3_t    maxs;
  float     zOffset;
} buildableAttributeOverrides_t;

//TA: weapon record
typedef struct
{
  int       weaponNum;

  int       price;
  int       stages;

  int       slots;

  char      *weaponName;
  char      *weaponHumanName;

  int       maxAmmo;
  int       maxClips;
  qboolean  infiniteAmmo;
  qboolean  usesEnergy;

  int       repeatRate1;
  int       repeatRate2;
  int       repeatRate3;
  int       reloadTime;

  qboolean  hasAltMode;
  qboolean  hasThirdMode;

  qboolean  canZoom;
  float     zoomFov;

  qboolean  purchasable;

  int       buildDelay;

  WUTeam_t  team;
} weaponAttributes_t;

//TA: upgrade record
typedef struct
{
  int       upgradeNum;

  int       price;
  int       stages;

  int       slots;

  char      *upgradeName;
  char      *upgradeHumanName;

  char      *icon;

  qboolean  purchasable;
  qboolean  usable;

  WUTeam_t  team;
} upgradeAttributes_t;


//TA:
void      BG_UnpackAmmoArray( int weapon, int psAmmo[ ], int psAmmo2[ ], int *ammo, int *clips );
void      BG_PackAmmoArray( int weapon, int psAmmo[ ], int psAmmo2[ ], int ammo, int clips );
qboolean  BG_WeaponIsFull( weapon_t weapon, int stats[ ], int psAmmo[ ], int psAmmo2[ ] );
void      BG_AddWeaponToInventory( int weapon, int stats[ ] );
void      BG_RemoveWeaponFromInventory( int weapon, int stats[ ] );
qboolean  BG_InventoryContainsWeapon( int weapon, int stats[ ] );
void      BG_AddUpgradeToInventory( int item, int stats[ ] );
void      BG_RemoveUpgradeFromInventory( int item, int stats[ ] );
qboolean  BG_InventoryContainsUpgrade( int item, int stats[ ] );
void      BG_ActivateUpgrade( int item, int stats[ ] );
void      BG_DeactivateUpgrade( int item, int stats[ ] );
qboolean  BG_UpgradeIsActive( int item, int stats[ ] );
qboolean  BG_RotateAxis( vec3_t surfNormal, vec3_t inAxis[ 3 ],
                         vec3_t outAxis[ 3 ], qboolean inverse, qboolean ceiling );
void      BG_PositionBuildableRelativeToPlayer( const playerState_t *ps,
                                                const vec3_t mins, const vec3_t maxs,
                                                void (*trace)( trace_t *, const vec3_t, const vec3_t,
                                                               const vec3_t, const vec3_t, int, int ),
                                                vec3_t outOrigin, vec3_t outAngles, trace_t *tr );
int       BG_GetValueOfHuman( playerState_t *ps );

int       BG_FindBuildNumForName( char *name );
int       BG_FindBuildNumForEntityName( char *name );
char      *BG_FindNameForBuildable( int bclass );
char      *BG_FindHumanNameForBuildable( int bclass );
char      *BG_FindEntityNameForBuildable( int bclass );
char      *BG_FindModelsForBuildable( int bclass, int modelNum );
float     BG_FindModelScaleForBuildable( int bclass );
void      BG_FindBBoxForBuildable( int bclass, vec3_t mins, vec3_t maxs );
float     BG_FindZOffsetForBuildable( int pclass );
int       BG_FindHealthForBuildable( int bclass );
int       BG_FindRegenRateForBuildable( int bclass );
trType_t  BG_FindTrajectoryForBuildable( int bclass );
float     BG_FindBounceForBuildable( int bclass );
int       BG_FindBuildPointsForBuildable( int bclass );
qboolean  BG_FindStagesForBuildable( int bclass, stage_t stage );
int       BG_FindSplashDamageForBuildable( int bclass );
int       BG_FindSplashRadiusForBuildable( int bclass );
int       BG_FindMODForBuildable( int bclass );
int       BG_FindTeamForBuildable( int bclass );
weapon_t  BG_FindBuildWeaponForBuildable( int bclass );
int       BG_FindAnimForBuildable( int bclass );
int       BG_FindNextThinkForBuildable( int bclass );
int       BG_FindBuildTimeForBuildable( int bclass );
qboolean  BG_FindUsableForBuildable( int bclass );
int       BG_FindRangeForBuildable( int bclass );
int       BG_FindFireSpeedForBuildable( int bclass );
weapon_t  BG_FindProjTypeForBuildable( int bclass );
float     BG_FindMinNormalForBuildable( int bclass );
qboolean  BG_FindInvertNormalForBuildable( int bclass );
int       BG_FindCreepTestForBuildable( int bclass );
int       BG_FindCreepSizeForBuildable( int bclass );
int       BG_FindDCCTestForBuildable( int bclass );
int       BG_FindUniqueTestForBuildable( int bclass );
void      BG_InitBuildableOverrides( void );

int       BG_FindClassNumForName( char *name );
char      *BG_FindNameForClassNum( int pclass );
char      *BG_FindHumanNameForClassNum( int pclass );
char      *BG_FindModelNameForClass( int pclass );
float     BG_FindModelScaleForClass( int pclass );
char      *BG_FindSkinNameForClass( int pclass );
float     BG_FindShadowScaleForClass( int pclass );
char      *BG_FindHudNameForClass( int pclass );
qboolean  BG_FindStagesForClass( int pclass, stage_t stage );
void      BG_FindBBoxForClass( int pclass, vec3_t mins, vec3_t maxs, vec3_t cmaxs, vec3_t dmins, vec3_t dmaxs );
float     BG_FindZOffsetForClass( int pclass );
void      BG_FindViewheightForClass( int pclass, int *viewheight, int *cViewheight );
int       BG_FindHealthForClass( int pclass );
float     BG_FindFallDamageForClass( int pclass );
int       BG_FindRegenRateForClass( int pclass );
int       BG_FindFovForClass( int pclass );
float     BG_FindBobForClass( int pclass );
float     BG_FindBobCycleForClass( int pclass );
float     BG_FindSpeedForClass( int pclass );
float     BG_FindAccelerationForClass( int pclass );
float     BG_FindAirAccelerationForClass( int pclass );
float     BG_FindFrictionForClass( int pclass );
float     BG_FindStopSpeedForClass( int pclass );
float     BG_FindJumpMagnitudeForClass( int pclass );
float     BG_FindKnockbackScaleForClass( int pclass );
int       BG_FindSteptimeForClass( int pclass );
qboolean  BG_ClassHasAbility( int pclass, int ability );
weapon_t  BG_FindStartWeaponForClass( int pclass );
float     BG_FindBuildDistForClass( int pclass );
int       BG_ClassCanEvolveFromTo( int fclass, int tclass, int credits, int num );
int       BG_FindCostOfClass( int pclass );
int       BG_FindValueOfClass( int pclass );
void      BG_InitClassOverrides( void );

int       BG_FindPriceForWeapon( int weapon );
qboolean  BG_FindStagesForWeapon( int weapon, stage_t stage );
int       BG_FindSlotsForWeapon( int weapon );
char      *BG_FindNameForWeapon( int weapon );
int       BG_FindWeaponNumForName( char *name );
char      *BG_FindHumanNameForWeapon( int weapon );
char      *BG_FindModelsForWeapon( int weapon, int modelNum );
char      *BG_FindIconForWeapon( int weapon );
char      *BG_FindCrosshairForWeapon( int weapon );
int       BG_FindCrosshairSizeForWeapon( int weapon );
void      BG_FindAmmoForWeapon( int weapon, int *maxAmmo, int *maxClips );
qboolean  BG_FindInfinteAmmoForWeapon( int weapon );
qboolean  BG_FindUsesEnergyForWeapon( int weapon );
int       BG_FindRepeatRate1ForWeapon( int weapon );
int       BG_FindRepeatRate2ForWeapon( int weapon );
int       BG_FindRepeatRate3ForWeapon( int weapon );
int       BG_FindReloadTimeForWeapon( int weapon );
qboolean  BG_WeaponHasAltMode( int weapon );
qboolean  BG_WeaponHasThirdMode( int weapon );
qboolean  BG_WeaponCanZoom( int weapon );
float     BG_FindZoomFovForWeapon( int weapon );
qboolean  BG_FindPurchasableForWeapon( int weapon );
int       BG_FindBuildDelayForWeapon( int weapon );
WUTeam_t  BG_FindTeamForWeapon( int weapon );

int       BG_FindPriceForUpgrade( int upgrade );
qboolean  BG_FindStagesForUpgrade( int upgrade, stage_t stage );
int       BG_FindSlotsForUpgrade( int upgrade );
char      *BG_FindNameForUpgrade( int upgrade );
int       BG_FindUpgradeNumForName( char *name );
char      *BG_FindHumanNameForUpgrade( int upgrade );
char      *BG_FindIconForUpgrade( int upgrade );
qboolean  BG_FindPurchasableForUpgrade( int upgrade );
qboolean  BG_FindUsableForUpgrade( int upgrade );
WUTeam_t  BG_FindTeamForUpgrade( int upgrade );

// content masks
#define MASK_ALL          (-1)
#define MASK_SOLID        (CONTENTS_SOLID)
#define MASK_PLAYERSOLID  (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define MASK_DEADSOLID    (CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define MASK_WATER        (CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define MASK_OPAQUE       (CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define MASK_SHOT         (CONTENTS_SOLID|CONTENTS_BODY)


void  BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void  BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result );

void  BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps );

void  BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap );
void  BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap );

qboolean  BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime );

#define ARENAS_PER_TIER   4
#define MAX_ARENAS      1024
#define MAX_ARENAS_TEXT   8192

#define MAX_BOTS      1024
#define MAX_BOTS_TEXT   8192

float   atof_neg( char *token, qboolean allowNegative );
int     atoi_neg( char *token, qboolean allowNegative );

void BG_ParseCSVEquipmentList( const char *string, weapon_t *weapons, int weaponsSize,
    upgrade_t *upgrades, int upgradesSize );
void BG_ParseCSVClassList( const char *string, pClass_t *classes, int classesSize );
void BG_ParseCSVBuildableList( const char *string, buildable_t *buildables, int buildablesSize );
void BG_InitAllowedGameElements( void );
qboolean BG_WeaponIsAllowed( weapon_t weapon );
qboolean BG_UpgradeIsAllowed( upgrade_t upgrade );
qboolean BG_ClassIsAllowed( pClass_t class );
qboolean BG_BuildableIsAllowed( buildable_t buildable );
qboolean BG_UpgradeClassAvailable( playerState_t *ps );
