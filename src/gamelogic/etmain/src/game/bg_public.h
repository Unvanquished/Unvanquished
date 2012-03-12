/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

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

In addition, the Daemon Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the Daemon 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

/*
 * name:		bg_public.h
 *
 * desc:		definitions shared by both the server game and client game modules
 *
*/

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

#ifndef __BG_PUBLIC_H__
#define __BG_PUBLIC_H__

#include "../../../../engine/qcommon/q_shared.h"

#define GAME_VERSION    "Enemy Territory"

#if defined( _DEBUG )
#define GAME_VERSION_DATED          GAME_VERSION
#else
#define GAME_VERSION_DATED          ( GAME_VERSION ", " Q3_VERSION )
#endif

//bani
#if defined __GNUC__ || defined __clang__
#define _attribute( x ) __attribute__( x )
#else
#define _attribute( x )
#endif

#define SPRINTTIME 20000.0f

#define DEBUG_BOT_RETREATBEHAVIOR 1

#define DEFAULT_GRAVITY     800
#define FORCE_LIMBO_HEALTH  -75	// JPW NERVE
#define GIB_HEALTH          -175	// JPW NERVE
#define ARMOR_PROTECTION    0.66

#define HOLDBREATHTIME      12000

#define MAX_ITEMS           256

#define RANK_TIED_FLAG      0x4000

//#define DEFAULT_SHOTGUN_SPREAD    700
//#define DEFAULT_SHOTGUN_COUNT 11

//#define   ITEM_RADIUS         15      // item sizes are needed for client side pickup detection
#define ITEM_RADIUS     10		// Rafael changed the radius so that the items would fit in the 3 new containers

// RF, zombie getup
//#define   TIMER_RESPAWN   (38*(1000/15)+100)

//#define   LIGHTNING_RANGE     600
//#define   TESLA_RANGE         800

#define FLAMETHROWER_RANGE  2500	// DHM - Nerve :: multiplayer range, was 850 in SP

//#define ZOMBIE_FLAME_RADIUS 300

// RF, AI effects
//#define   PORTAL_ZOMBIE_SPAWNTIME     3000
//#define   PORTAL_FEMZOMBIE_SPAWNTIME  3000

#define SCORE_NOT_PRESENT   -9999	// for the CS_SCORES[12] when only one player is present

#define VOTE_TIME           30000	// 30 seconds before vote times out

// Ridah, disabled these
//#define   MINS_Z              -24
//#define   DEFAULT_VIEWHEIGHT  26
//#define CROUCH_VIEWHEIGHT 12
// done.

// Rafael
// note to self: Corky test
//#define   DEFAULT_VIEWHEIGHT  26
//#define CROUCH_VIEWHEIGHT 12
#define DEFAULT_VIEWHEIGHT  40
#define CROUCH_VIEWHEIGHT   16
#define DEAD_VIEWHEIGHT     -16

#define PRONE_VIEWHEIGHT    -8

extern vec3_t   playerlegsProneMins;
extern vec3_t   playerlegsProneMaxs;

#define MAX_COMMANDMAP_LAYERS	16	// CHRUKER: b043 - Increased from 4 to 16

#define DEFAULT_MODEL       "multi"
#define DEFAULT_HEAD        "default"	// technically the default head skin.  this means "head_default.skin" for the head

// RF, on fire effects
#define FIRE_FLASH_TIME         2000
#define FIRE_FLASH_FADEIN_TIME  1000

#define LIGHTNING_FLASH_TIME    150

#define AAGUN_DAMAGE        25
#define AAGUN_SPREAD        10

// NOTE: use this value, and THEN the cl_input.c scales to tweak the feel
#define MG42_IDLEYAWSPEED   80.0	// degrees per second (while returning to base)
#define MG42_SPREAD_MP      100

#define MG42_DAMAGE_MP      20
#define MG42_RATE_OF_FIRE_MP    66

#define MG42_DAMAGE_SP      40
#define MG42_RATE_OF_FIRE_SP    100

#define AAGUN_RATE_OF_FIRE  100
#define MG42_YAWSPEED       300.f	// degrees per second

#define SAY_ALL     0
#define SAY_TEAM    1
#define SAY_BUDDY   2
#define SAY_TEAMNL  3

// RF, client damage identifiers

// Arnout: different entity states
typedef enum
{
	STATE_DEFAULT,				// ent is linked, can be used and is solid
	STATE_INVISIBLE,			// ent is unlinked, can't be used, doesn't think and is not solid
	STATE_UNDERCONSTRUCTION		// ent is being constructed
} entState_t;

typedef enum
{
	SELECT_BUDDY_ALL = 0,
	SELECT_BUDDY_1,
	SELECT_BUDDY_2,
	SELECT_BUDDY_3,
	SELECT_BUDDY_4,
	SELECT_BUDDY_5,
	SELECT_BUDDY_6,

	SELECT_BUDDY_LAST			// must be the last one in the enum
} SelectBuddyFlag;

// START - TAT 10/21/2002
// New icon based bot action command system
typedef enum
{
	BOT_ACTION_ATTACK = 0,
	BOT_ACTION_COVER,			// 1
	BOT_ACTION_MOUNTGUN,		// 2
	BOT_ACTION_OPENDOOR,		// 3
	BOT_ACTION_USEDYNAMITE,		// 4
	BOT_ACTION_DISARM,			// 5
	BOT_ACTION_CONSTRUCT,		// 6
	BOT_ACTION_REPAIR,			// 7
	BOT_ACTION_REVIVE,			// 8
	BOT_ACTION_GETDISGUISE,		// 9
	BOT_ACTION_HEAL,			// 10
	BOT_ACTION_AMMO,			// 11
	BOT_ACTION_GRENADELAUNCH,	// 12
	BOT_ACTION_PICKUPITEM,		// 13
	BOT_ACTION_PANZERFAUST,		// 14
	BOT_ACTION_FLAMETHROW,		// 15
	BOT_ACTION_MG42,			// 16
	BOT_ACTION_MOUNTEDATTACK,	// 17       -- attack when mounted on mg42
	BOT_ACTION_KNIFEATTACK,		// 18
	BOT_ACTION_LOCKPICK,		// 19

	BOT_ACTION_MAXENTITY,

	// None of these need an entity...
	BOT_ACTION_RECON = BOT_ACTION_MAXENTITY,	// 20
	BOT_ACTION_SMOKEBOMB,		// 21
	BOT_ACTION_FINDMINES,		// 22
	BOT_ACTION_PLANTMINE,		// 23
	BOT_ACTION_ARTILLERY,		// 24
	BOT_ACTION_AIRSTRIKE,		// 25
	BOT_ACTION_MOVETOLOC,		// 26

	// NOTE: if this gets bigger than 32 items, need to make botMenuIcons bigger
	BOT_ACTION_MAX
} botAction_t;

// END - TAT 10/21/2002

// RF
#define MAX_TAGCONNECTS     64

// (SA) zoom sway values
#define ZOOM_PITCH_AMPLITUDE        0.13f
#define ZOOM_PITCH_FREQUENCY        0.24f
#define ZOOM_PITCH_MIN_AMPLITUDE    0.1f	// minimum amount of sway even if completely settled on target

#define ZOOM_YAW_AMPLITUDE          0.7f
#define ZOOM_YAW_FREQUENCY          0.12f
#define ZOOM_YAW_MIN_AMPLITUDE      0.2f

// DHM - Nerve
#define MAX_OBJECTIVES      8
#define MAX_OID_TRIGGERS    18
// dhm

#define MAX_GAMETYPES 16

typedef struct
{
	const char     *mapName;
	const char     *mapLoadName;
	const char     *imageName;

	int             typeBits;
	int             cinematic;

	// Gordon: FIXME: remove
	const char     *opponentName;
	int             teamMembers;
	int             timeToBeat[MAX_GAMETYPES];

	qhandle_t       levelShot;
	qboolean        active;

	// NERVE - SMF
	int             Timelimit;
	int             AxisRespawnTime;
	int             AlliedRespawnTime;
	// -NERVE - SMF

	vec2_t          mappos;

	const char     *briefing;
	const char     *lmsbriefing;
	const char     *objectives;
} mapInfo;

// Campaign saves
// rain - 128 -> 512, campaigns are commonplace
#define MAX_CAMPAIGNS           512

// START Mad Doc - TDF
// changed this from 6 to 10
#define MAX_MAPS_PER_CAMPAIGN   10
// END Mad Doc - TDF

#define CPS_IDENT   ( ( 'S' << 24 ) + ( 'P' << 16 ) + ( 'C' << 8 ) + 'I' )
#define CPS_VERSION 1

typedef struct
{
	int             mapnameHash;
} cpsMap_t;

typedef struct
{
	int             shortnameHash;
	int             progress;

	cpsMap_t        maps[MAX_MAPS_PER_CAMPAIGN];
} cpsCampaign_t;

typedef struct
{
	int             ident;
	int             version;

	int             numCampaigns;
	int             profileHash;
} cpsHeader_t;

typedef struct
{
	cpsHeader_t     header;
	cpsCampaign_t   campaigns[MAX_CAMPAIGNS];
} cpsFile_t;

qboolean        BG_LoadCampaignSave(const char *filename, cpsFile_t * file, const char *profile);
qboolean        BG_StoreCampaignSave(const char *filename, cpsFile_t * file, const char *profile);

typedef struct
{
	const char     *campaignShortName;
	const char     *campaignName;
	const char     *campaignDescription;
	const char     *nextCampaignShortName;
	const char     *maps;
	int             mapCount;
	mapInfo        *mapInfos[MAX_MAPS_PER_CAMPAIGN];
	vec2_t          mapTC[2];
	cpsCampaign_t  *cpsCampaign;	// if this campaign was found in the campaignsave, more detailed info can be found here

	const char     *campaignShotName;
	int             campaignCinematic;
	qhandle_t       campaignShot;

	qboolean        unlocked;
	int             progress;

	qboolean        initial;
	int             order;

	int             typeBits;
} campaignInfo_t;

// Random reinforcement seed settings
#define MAX_REINFSEEDS  8
#define REINF_RANGE     16		// (0 to n-1 second offset)
#define REINF_BLUEDELT  3		// Allies shift offset
#define REINF_REDDELT   2		// Axis shift offset
extern const unsigned int aReinfSeeds[MAX_REINFSEEDS];

// Client flags for server processing
#define CGF_AUTORELOAD      0x01
#define CGF_STATSDUMP       0x02
#define CGF_AUTOACTIVATE    0x04
#define CGF_PREDICTITEMS    0x08

#define MAX_MOTDLINES   6

// Multiview settings
#define MAX_MVCLIENTS               32
#define MV_SCOREUPDATE_INTERVAL     5000	// in msec

#define MAX_CHARACTERS  16

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define CS_MUSIC                        2
#define CS_MESSAGE                      3	// from the map worldspawn's message field
#define CS_MOTD                         4	// g_motd string for server message of the day
#define CS_WARMUP                       5	// server time when the match will be restarted
#define CS_VOTE_TIME                    6
#define CS_VOTE_STRING                  7
#define CS_VOTE_YES                     8
#define CS_VOTE_NO                      9
#define CS_GAME_VERSION                 10

#define CS_LEVEL_START_TIME             11	// so the timer only shows the current level
#define CS_INTERMISSION                 12	// when 1, intermission will start in a second or two
#define CS_MULTI_INFO                   13
#define CS_MULTI_MAPWINNER              14
#define CS_MULTI_OBJECTIVE              15
//
#define CS_SCREENFADE                   17	// Ridah, used to tell clients to fade their screen to black/normal
#define CS_FOGVARS                      18	//----(SA) used for saving the current state/settings of the fog
#define CS_SKYBOXORG                    19	// this is where we should view the skybox from

#define CS_TARGETEFFECT                 20	//----(SA)
#define CS_WOLFINFO                     21	// NERVE - SMF
#define CS_FIRSTBLOOD                   22	// Team that has first blood
#define CS_ROUNDSCORES1                 23	// Axis round wins
#define CS_ROUNDSCORES2                 24	// Allied round wins
#define CS_MAIN_AXIS_OBJECTIVE          25	// Most important current objective
#define CS_MAIN_ALLIES_OBJECTIVE        26	// Most important current objective
#define CS_MUSIC_QUEUE                  27
#define CS_SCRIPT_MOVER_NAMES           28
#define CS_CONSTRUCTION_NAMES           29

#define CS_VERSIONINFO                  30	// Versioning info for demo playback compatibility
#define CS_REINFSEEDS                   31	// Reinforcement seeds
#define CS_SERVERTOGGLES                32	// Shows current enable/disabled settings (for voting UI)
#define CS_GLOBALFOGVARS                33
#define CS_AXIS_MAPS_XP                 34
#define CS_ALLIED_MAPS_XP               35
#define CS_INTERMISSION_START_TIME      36	//
#define CS_ENDGAME_STATS                37
#define CS_CHARGETIMES                  38
#define CS_FILTERCAMS                   39

#define CS_MODELS                       64
#define CS_SOUNDS                       ( CS_MODELS +               MAX_MODELS                  )
#define CS_SHADERS                      ( CS_SOUNDS +               MAX_SOUNDS                  )
#define CS_SHADERSTATE                  ( CS_SHADERS +              MAX_CS_SHADERS              )	// Gordon: this MUST be after CS_SHADERS
#define CS_SKINS                        ( CS_SHADERSTATE +          1                           )
#define CS_CHARACTERS                   ( CS_SKINS +                MAX_CS_SKINS                )
#define CS_PLAYERS                      ( CS_CHARACTERS +           MAX_CHARACTERS              )
#define CS_MULTI_SPAWNTARGETS           ( CS_PLAYERS +              MAX_CLIENTS                 )
#define CS_OID_TRIGGERS                 ( CS_MULTI_SPAWNTARGETS +   MAX_MULTI_SPAWNTARGETS      )
#define CS_OID_DATA                     ( CS_OID_TRIGGERS +         MAX_OID_TRIGGERS            )
#define CS_DLIGHTS                      ( CS_OID_DATA +             MAX_OID_TRIGGERS            )
#define CS_SPLINES                      ( CS_DLIGHTS +              MAX_DLIGHT_CONFIGSTRINGS    )
#define CS_TAGCONNECTS                  ( CS_SPLINES +              MAX_SPLINE_CONFIGSTRINGS    )
#define CS_FIRETEAMS                    ( CS_TAGCONNECTS +          MAX_TAGCONNECTS             )
#define CS_CUSTMOTD                     ( CS_FIRETEAMS +            MAX_FIRETEAMS               )
#define CS_STRINGS                      ( CS_CUSTMOTD +             MAX_MOTDLINES               )
#define CS_MAX                          ( CS_STRINGS +              MAX_CSSTRINGS               )

#if ( CS_MAX ) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

//#ifndef GAMETYPES
//#define GAMETYPES
typedef enum
{
	GT_SINGLE_PLAYER,
	GT_COOP,
	GT_WOLF,
	GT_WOLF_STOPWATCH,
	GT_WOLF_CAMPAIGN,			// Exactly the same as GT_WOLF, but uses campaign roulation (multiple maps form one virtual map)
	GT_WOLF_LMS,
	GT_MAX_GAME_TYPE
} gametype_t;

//#define GAMETYPES

// Rafael gameskill
/*typedef enum {
	GSKILL_EASY = 1,
	GSKILL_MEDIUM,
	GSKILL_MEDIUMHARD, // normal default level
	GSKILL_HARD,
	GSKILL_VERYHARD,
	GSKILL_MAX		// must always be last
} gameskill_t;*/

//#endif // ifndef GAMETYPES

typedef enum
{ GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

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
	PM_NORMAL,					// can accelerate and turn
	PM_NOCLIP,					// noclip movement
	PM_SPECTATOR,				// still run into walls
	PM_DEAD,					// no acceleration or turning, but free falling
	PM_FREEZE,					// stuck in place with no control
	PM_INTERMISSION				// no movement or status bar
} pmtype_t;

typedef enum
{
	WEAPON_READY,
	WEAPON_RAISING,
	WEAPON_RAISING_TORELOAD,
	WEAPON_DROPPING,
	WEAPON_DROPPING_TORELOAD,
	WEAPON_READYING,			// getting from 'ready' to 'firing'
	WEAPON_RELAXING,			// weapon is ready, but since not firing, it's on it's way to a "relaxed" stance
	WEAPON_FIRING,
	WEAPON_FIRINGALT,
	WEAPON_RELOADING,			//----(SA)  added
} weaponstate_t;

typedef enum
{
	WSTATE_IDLE,
	WSTATE_SWITCH,
	WSTATE_FIRE,
	WSTATE_RELOAD
} weaponstateCompact_t;

// pmove->pm_flags  (sent as max 16 bits in msg.c)
#define PMF_DUCKED          1
#define PMF_JUMP_HELD       2
#define PMF_LADDER          4	// player is on a ladder
#define PMF_BACKWARDS_JUMP  8	// go into backwards land
#define PMF_BACKWARDS_RUN   16	// coast down to backwards run
#define PMF_TIME_LAND       32	// pm_time is time before rejump
#define PMF_TIME_KNOCKBACK  64	// pm_time is an air-accelerate only time
#define PMF_TIME_WATERJUMP  256	// pm_time is waterjump
#define PMF_RESPAWNED       512	// clear after attack and jump buttons come up
//#define PMF_PRONE_BIPOD       1024    // prone with a bipod set
#define PMF_FLAILING        2048
#define PMF_FOLLOW          4096	// spectate following another player
#define PMF_TIME_LOAD       8192	// hold for this time after a load game, and prevent large thinks
#define PMF_LIMBO           16384	// JPW NERVE limbo state, pm_time is time until reinforce
#define PMF_TIME_LOCKPLAYER 32768	// DHM - Nerve :: Lock all movement and view changes

#define PMF_ALL_TIMES   ( PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_KNOCKBACK | PMF_TIME_LOCKPLAYER /*|PMF_TIME_LOAD*/ )

typedef struct
{
	qboolean        bAutoReload;	// do we predict autoreload of weapons

	int             jumpTime;	// used in MP to prevent jump accel

	int             weapAnimTimer;	// don't change low priority animations until this runs out     //----(SA)  added
	int             silencedSideArm;	// Gordon: Keep track of whether the luger/colt is silenced "in holster", prolly want to do this for the kar98 etc too
	int             sprintTime;

	int             airleft;

	// Arnout: MG42 aiming
	float           varc, harc;
	vec3_t          centerangles;

	int             dtmove;		// doubletap move

	int             dodgeTime;
	int             proneTime;	// time a go-prone or stop-prone move starts, to sync the animation to

	int             proneGroundTime;	// time a prone player last had ground under him
	float           proneLegsOffset;	// offset legs bounding box

	vec3_t          mountedWeaponAngles;	// mortar, mg42 (prone), etc

	int             weapRecoilTime;	// Arnout: time at which a weapon that has a recoil kickback has been fired last
	int             weapRecoilDuration;
	float           weapRecoilYaw;
	float           weapRecoilPitch;
	int             lastRecoilDeltaTime;

	qboolean        releasedFire;
} pmoveExt_t;					// data used both in client and server - store it here

				// instead of playerstate to prevent different engine versions of playerstate between XP and MP

#define MAXTOUCH    32
typedef struct
{
	// state (in / out)
	playerState_t  *ps;
	pmoveExt_t     *pmext;
	struct bg_character_s *character;

	// command (in)
	usercmd_t       cmd, oldcmd;
	int             tracemask;	// collide against these types of surfaces
	int             debugLevel;	// if set, diagnostic output will be printed
	qboolean        noFootsteps;	// if the game is setup for no footsteps by the server
	qboolean        noWeapClips;	// if the game is setup for no weapon clips by the server
	qboolean        gauntletHit;	// true if a gauntlet attack would actually hit something

	// NERVE - SMF (in)
	int             gametype;
	int             ltChargeTime;
	int             soldierChargeTime;
	int             engineerChargeTime;
	int             medicChargeTime;
	// -NERVE - SMF
	int             covertopsChargeTime;

	// results (out)
	int             numtouch;
	int             touchents[MAXTOUCH];

	vec3_t          mins, maxs;	// bounding box size

	int             watertype;
	int             waterlevel;

	float           xyspeed;

	int            *skill;		// player skills

#ifdef GAMEDLL					// the whole stamina thing is only in qagame
	qboolean        leadership;	// within 512 units of a player with level 5 Signals skill (that player has to be in PVS as well to make sue we can predict it)
#endif							// GAMEDLL

	// for fixed msec Pmove
	int             pmove_fixed;
	int             pmove_msec;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void            (*trace) (trace_t * results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
							  int passEntityNum, int contentMask);
	int             (*pointcontents) (const vec3_t point, int passEntityNum);
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void            PM_UpdateViewAngles(playerState_t * ps, pmoveExt_t * pmext, usercmd_t * cmd,
									void (trace) (trace_t * results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
												  const vec3_t end, int passEntityNum, int contentMask), int tracemask);
int             Pmove(pmove_t * pmove);

//===================================================================================

#define PC_SOLDIER              0	//  shoot stuff
#define PC_MEDIC                1	//  heal stuff
#define PC_ENGINEER             2	//  build stuff
#define PC_FIELDOPS             3	//  bomb stuff
#define PC_COVERTOPS            4	//  sneak about ;o

#define NUM_PLAYER_CLASSES      5

// JPW NERVE
#define MAX_WEAPS_IN_BANK_MP    12
#define MAX_WEAP_BANKS_MP       10
// jpw

// player_state->stats[] indexes
typedef enum
{
	STAT_HEALTH,
	STAT_KEYS,					// 16 bit fields
	STAT_DEAD_YAW,				// look this direction when dead (FIXME: get rid of?)
	STAT_CLIENTS_READY,			// bit mask of clients wishing to exit the intermission (FIXME: configstring?)
	STAT_MAX_HEALTH,			// health / armor limit, changable by handicap
	STAT_PLAYER_CLASS,			// DHM - Nerve :: player class in multiplayer
	STAT_CAPTUREHOLD_RED,		// JPW NERVE - red team score
	STAT_CAPTUREHOLD_BLUE,		// JPW NERVE - blue team score
	STAT_XP,					// Gordon: "realtime" version of xp that doesnt need to go thru the scoreboard
} statIndex_t;

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
typedef enum
{
	PERS_SCORE,					// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,					// total points damage inflicted so damage beeps can sound on change
	PERS_RANK,
	PERS_TEAM,
	PERS_SPAWN_COUNT,			// incremented every respawn
	PERS_ATTACKER,				// clientnum of last damage inflicter
	PERS_KILLED,				// count of the number of times you died
	// these were added for single player awards tracking
	PERS_RESPAWNS_LEFT,			// DHM - Nerve :: number of remaining respawns
	PERS_RESPAWNS_PENALTY,		// how many respawns you have to sit through before respawning again

	PERS_REVIVE_COUNT,
	PERS_BLEH_2,
	PERS_BLEH_3,

	// Rafael - mg42        // (SA) I don't understand these here.  can someone explain?
	PERS_HWEAPON_USE,
	// Rafael wolfkick
	PERS_WOLFKICK
} persEnum_t;


// entityState_t->eFlags
#define EF_DEAD             0x00000001	// don't draw a foe marker over players with EF_DEAD
#define EF_NONSOLID_BMODEL  0x00000002	// bmodel is visible, but not solid
#define EF_FORCE_END_FRAME  EF_NONSOLID_BMODEL	// force client to end of current animation (after loading a savegame)
#define EF_TELEPORT_BIT     0x00000004	// toggled every time the origin abruptly changes
#define EF_READY            0x00000008	// player is ready

#define EF_CROUCHING        0x00000010	// player is crouching
#define EF_MG42_ACTIVE      0x00000020	// currently using an MG42
#define EF_NODRAW           0x00000040	// may have an event, but no model (unspawned items)
#define EF_FIRING           0x00000080	// for lightning gun
#define EF_INHERITSHADER    EF_FIRING	// some ents will never use EF_FIRING, hijack it for "USESHADER"

#define EF_SPINNING         0x00000100	// (SA) added for level editor control of spinning pickup items
#define EF_BREATH           EF_SPINNING	// Characters will not have EF_SPINNING set, hijack for drawing character breath
#define EF_TALK             0x00000200	// draw a talk balloon
#define EF_CONNECTION       0x00000400	// draw a connection trouble sprite
#define EF_SMOKINGBLACK     0x00000800	// JPW NERVE -- like EF_SMOKING only darker & bigger

#define EF_HEADSHOT         0x00001000	// last hit to player was head shot (Gordon: NOTE: not last hit, but has BEEN shot in the head since respawn)
#define EF_SMOKING          0x00002000	// DHM - Nerve :: ET_GENERAL ents will emit smoke if set // JPW switched to this after my code change
#define EF_OVERHEATING      ( EF_SMOKING | EF_SMOKINGBLACK )	// ydnar: light smoke/steam effect
#define EF_VOTED            0x00004000	// already cast a vote
#define EF_TAGCONNECT       0x00008000	// connected to another entity via tag
#define EF_MOUNTEDTANK      EF_TAGCONNECT	// Gordon: duplicated for clarity

#define EF_SPARE3           0x00010000	// Gordon: freed
#define EF_PATH_LINK        0x00020000	// Gordon: linking trains together
#define EF_ZOOMING          0x00040000	// client is zooming
#define EF_PRONE            0x00080000	// player is prone

#define EF_PRONE_MOVING     0x00100000	// player is prone and moving
#define EF_VIEWING_CAMERA   0x00200000	// player is viewing a camera
#define EF_AAGUN_ACTIVE     0x00400000	// Gordon: player is manning an AA gun
#define EF_SPARE0           0x00800000	// Gordon: freed

// !! NOTE: only place flags that don't need to go to the client beyond 0x00800000
#define EF_SPARE1           0x01000000	// Gordon: freed
#define EF_SPARE2           0x02000000	// Gordon: freed
#define EF_BOUNCE           0x04000000	// for missiles
#define EF_BOUNCE_HALF      0x08000000	// for missiles
#define EF_MOVER_STOP       0x10000000	// will push otherwise  // (SA) moved down to make space for one more client flag
#define EF_MOVER_BLOCKED    0x20000000	// mover was blocked dont lerp on the client // xkan, moved down to make space for client flag

#define BG_PlayerMounted( eFlags ) ( ( eFlags & EF_MG42_ACTIVE ) || ( eFlags & EF_MOUNTEDTANK ) || ( eFlags & EF_AAGUN_ACTIVE ) )

// !! NOTE: only place flags that don't need to go to the client beyond 0x00800000
typedef enum
{
	PW_NONE,

	// (SA) for Wolf
	PW_INVULNERABLE,
	PW_FIRE,					//----(SA)
	PW_ELECTRIC,				//----(SA)
	PW_BREATHER,				//----(SA)
	PW_NOFATIGUE,				//----(SA)

	PW_REDFLAG,
	PW_BLUEFLAG,

	PW_OPS_DISGUISED,
	PW_OPS_CLASS_1,
	PW_OPS_CLASS_2,
	PW_OPS_CLASS_3,

	PW_ADRENALINE,

	PW_BLACKOUT = 14,			// OSP - spec blackouts. FIXME: we don't need 32bits here...relocate
	PW_MVCLIENTLIST = 15,		// OSP - MV client info.. need a full 32 bits

	PW_NUM_POWERUPS
} powerup_t;

typedef enum
{
	//----(SA)  These will probably all change to INV_n to get the word 'key' out of the game.
	//          id and DM don't want references to 'keys' in the game.
	//          I'll change to 'INV' as the item becomes 'permanent' and not a test item.


	KEY_NONE,
	KEY_1,						// skull
	KEY_2,						// chalice
	KEY_3,						// eye
	KEY_4,						// field radio
	KEY_5,						// satchel charge
	INV_BINOCS,					// binoculars
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_10,
	KEY_11,
	KEY_12,
	KEY_13,
	KEY_14,
	KEY_15,
	KEY_16,
	KEY_LOCKED_PICKABLE,		// Mad Doc - TDF: ent can be unlocked with the WP_LOCKPICK.
	KEY_NUM_KEYS
} wkey_t;						// conflicts with types.h

typedef enum
{
	HI_NONE,

//  HI_TELEPORTER,
	HI_MEDKIT,

	// new for Wolf
	HI_WINE,
	HI_SKULL,
	HI_WATER,
	HI_ELECTRIC,
	HI_FIRE,
	HI_STAMINA,
	HI_BOOK1,					//----(SA)  added
	HI_BOOK2,					//----(SA)  added
	HI_BOOK3,					//----(SA)  added
	HI_11,
	HI_12,
	HI_13,
	HI_14,
//  HI_15,  // ?

	HI_NUM_HOLDABLE
} holdable_t;

#ifdef KITS
// START Mad Doc - TDF
// for kits dropped by allied bots in SP
typeef enum
{
	KIT_SOLDIER,
	KIT_MEDIC,
	KIT_ENGINEER,
	KIT_LT,
	KIT_COVERTOPS
} kit_t;

// END Mad Doc - TDF
#endif

// NOTE: we can only use up to 15 in the client-server stream
// SA NOTE: should be 31 now (I added 1 bit in msg.c)
// RF NOTE: if this changes, please update etmain\botfiles\inv.h
typedef enum
{
	WP_NONE,					// 0
	WP_KNIFE,					// 1
	WP_LUGER,					// 2
	WP_MP40,					// 3
	WP_GRENADE_LAUNCHER,		// 4
	WP_PANZERFAUST,				// 5
	WP_FLAMETHROWER,			// 6

	WP_COLT,					// 7    // equivalent american weapon to german luger
	WP_THOMPSON,				// 8    // equivalent american weapon to german mp40
	WP_GRENADE_PINEAPPLE,		// 9
	WP_STEN,					// 10   // silenced sten sub-machinegun
	WP_MEDIC_SYRINGE,			// 11   // JPW NERVE -- broken out from CLASS_SPECIAL per Id request
	WP_AMMO,					// 12   // JPW NERVE likewise
	WP_ARTY,					// 13

	WP_SILENCER,				// 14   // used to be sp5
	WP_DYNAMITE,				// 15
	WP_SMOKETRAIL,				// 16
	WP_MAPMORTAR,				// 17
	VERYBIGEXPLOSION,			// 18   // explosion effect for airplanes
	WP_MEDKIT,					// 19
	WP_BINOCULARS,				// 20

	WP_PLIERS,					// 21
	WP_SMOKE_MARKER,			// 22   // Arnout: changed name to cause less confusion
	WP_KAR98,					// 23   // WolfXP weapons
	WP_CARBINE,					// 24
	WP_GARAND,					// 25
	WP_LANDMINE,				// 26
	WP_SATCHEL,					// 27
	WP_SATCHEL_DET,				// 28
	WP_TRIPMINE,				// 29
	WP_SMOKE_BOMB,				// 30

	WP_MOBILE_MG42,				// 31
	WP_K43,						// 32
	WP_FG42,					// 33
	WP_DUMMY_MG42,				// 34 // Gordon: for storing heat on mounted mg42s...
	WP_MORTAR,					// 35
	WP_LOCKPICK,				// 36   // Mad Doc - TDF lockpick
	WP_AKIMBO_COLT,				// 37
	WP_AKIMBO_LUGER,			// 38

// Gordon: ONLY secondaries below this mark, as they are checked >= WP_GPG40 && < WP_NUM_WEAPONS

	WP_GPG40,					// 39

	WP_M7,						// 40
	WP_SILENCED_COLT,			// 41
	WP_GARAND_SCOPE,			// 42
	WP_K43_SCOPE,				// 43
	WP_FG42SCOPE,				// 44
	WP_MORTAR_SET,				// 45
	WP_MEDIC_ADRENALINE,		// 46
	WP_AKIMBO_SILENCEDCOLT,		// 47
	WP_AKIMBO_SILENCEDLUGER,	// 48
	WP_MOBILE_MG42_SET,			// 49

	WP_NUM_WEAPONS				// WolfMP: 32 WolfXP: 50
		// NOTE: this cannot be larger than 64 for AI/player weapons!
} weapon_t;

// JPW NERVE moved from cg_weapons (now used in g_active) for drop command, actual array in bg_misc.c
extern int      weapBanksMultiPlayer[MAX_WEAP_BANKS_MP][MAX_WEAPS_IN_BANK_MP];

// jpw

// TAT 10/4/2002
//      Using one unified list for which weapons can received ammo
//      This is used both by the ammo pack code and by the bot code to determine if reloads are needed
extern int      reloadableWeapons[];

typedef struct
{
	int             kills, teamkills, killedby;
} weaponStats_t;

typedef enum
{
	HR_HEAD,
	HR_ARMS,
	HR_BODY,
	HR_LEGS,
	HR_NUM_HITREGIONS,
} hitRegion_t;

typedef enum
{
	SK_BATTLE_SENSE,
	SK_EXPLOSIVES_AND_CONSTRUCTION,
	SK_FIRST_AID,
	SK_SIGNALS,
	SK_LIGHT_WEAPONS,
	SK_HEAVY_WEAPONS,
	SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS,
	SK_NUM_SKILLS
} skillType_t;

extern const char *skillNames[SK_NUM_SKILLS];
extern const char *skillNamesLine1[SK_NUM_SKILLS];
extern const char *skillNamesLine2[SK_NUM_SKILLS];
extern const char *medalNames[SK_NUM_SKILLS];

#define NUM_SKILL_LEVELS 5
extern const int skillLevels[NUM_SKILL_LEVELS];

typedef struct
{
	weaponStats_t   weaponStats[WP_NUM_WEAPONS];
	int             suicides;
	int             hitRegions[HR_NUM_HITREGIONS];
	int             objectiveStats[MAX_OBJECTIVES];
} playerStats_t;

typedef struct ammotable_s
{
	int             maxammo;	//
	int             uses;		//
	int             maxclip;	//
	int             defaultStartingAmmo;	// Mad Doc - TDF
	int             defaultStartingClip;	// Mad Doc - TDF
	int             reloadTime;	//
	int             fireDelayTime;	//
	int             nextShotTime;	//
//----(SA)  added
	int             maxHeat;	// max active firing time before weapon 'overheats' (at which point the weapon will fail)
	int             coolRate;	// how fast the weapon cools down. (per second)
//----(SA)  end
	int             mod;		// means of death
} ammotable_t;


// Lookup table to find ammo table entry
extern ammotable_t *GetAmmoTableData(int ammoIndex);

extern int      weapAlts[];		// defined in bg_misc.c


//----(SA)
// for routines that need to check if a WP_ is </=/> a given set of weapons
#define WP_BEGINSECONDARY   WP_GPG40
#define WP_LASTSECONDARY    WP_SILENCED_COLT
#define WEAPS_ONE_HANDED    ( ( 1 << WP_KNIFE ) | ( 1 << WP_LUGER ) | ( 1 << WP_COLT ) | ( 1 << WP_SILENCER ) | ( 1 << WP_SILENCED_COLT ) | ( 1 << WP_GRENADE_LAUNCHER ) | ( 1 << WP_GRENADE_PINEAPPLE ) )

// TTimo
// NOTE: what about WP_VENOM and other XP weapons?
// rain - #81 - added added akimbo weapons and deployed MG42
#define IS_AUTORELOAD_WEAPON( weapon ) \
	(	\
		weapon == WP_LUGER    || weapon == WP_COLT          || weapon == WP_MP40          || \
		weapon == WP_THOMPSON || weapon == WP_STEN      || \
		weapon == WP_KAR98    || weapon == WP_CARBINE       || weapon == WP_GARAND_SCOPE  || \
		weapon == WP_FG42     || weapon == WP_K43           || weapon == WP_MOBILE_MG42   || \
		weapon == WP_SILENCED_COLT    || weapon == WP_SILENCER      || \
		weapon == WP_GARAND   || weapon == WP_K43_SCOPE     || weapon == WP_FG42SCOPE     || \
		BG_IsAkimboWeapon( weapon ) || weapon == WP_MOBILE_MG42_SET	\
	)

// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define EV_EVENT_BIT1       0x00000100
#define EV_EVENT_BIT2       0x00000200
#define EV_EVENT_BITS       ( EV_EVENT_BIT1 | EV_EVENT_BIT2 )

typedef enum
{
	EV_NONE,
	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSTEP_WOOD,
	EV_FOOTSTEP_GRASS,
	EV_FOOTSTEP_GRAVEL,
	EV_FOOTSTEP_ROOF,
	EV_FOOTSTEP_SNOW,
	EV_FOOTSTEP_CARPET,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,
	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,
	EV_FALL_SHORT,
	EV_FALL_MEDIUM,
	EV_FALL_FAR,
	EV_FALL_NDIE,
	EV_FALL_DMG_10,
	EV_FALL_DMG_15,
	EV_FALL_DMG_25,
	EV_FALL_DMG_50,
	EV_JUMP,
	EV_WATER_TOUCH,				// foot touches
	EV_WATER_LEAVE,				// foot leaves
	EV_WATER_UNDER,				// head touches
	EV_WATER_CLEAR,				// head leaves
	EV_ITEM_PICKUP,				// normal item pickups are predictable
	EV_ITEM_PICKUP_QUIET,		// (SA) same, but don't play the default pickup sound as it was specified in the ent
	EV_GLOBAL_ITEM_PICKUP,		// powerup / team sounds are broadcast to everyone
	EV_NOAMMO,
	EV_WEAPONSWITCHED,
	EV_EMPTYCLIP,
	EV_FILL_CLIP,
	EV_MG42_FIXED,				// JPW NERVE
	EV_WEAP_OVERHEAT,
	EV_CHANGE_WEAPON,
	EV_CHANGE_WEAPON_2,
	EV_FIRE_WEAPON,
	EV_FIRE_WEAPONB,
	EV_FIRE_WEAPON_LASTSHOT,
	EV_NOFIRE_UNDERWATER,
	EV_FIRE_WEAPON_MG42,
	EV_FIRE_WEAPON_MOUNTEDMG42,
	EV_ITEM_RESPAWN,
	EV_ITEM_POP,
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,
	EV_GRENADE_BOUNCE,			// eventParm will be the soundindex
	EV_GENERAL_SOUND,
	EV_GENERAL_SOUND_VOLUME,
	EV_GLOBAL_SOUND,			// no attenuation
	EV_GLOBAL_CLIENT_SOUND,		// DHM - Nerve :: no attenuation, only plays for specified client
	EV_GLOBAL_TEAM_SOUND,		// no attenuation, team only
	EV_FX_SOUND,
	EV_BULLET_HIT_FLESH,
	EV_BULLET_HIT_WALL,
	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_RAILTRAIL,
	EV_VENOM,
	EV_BULLET,					// otherEntity is the shooter
	EV_LOSE_HAT,				//----(SA)
	EV_PAIN,
	EV_CROUCH_PAIN,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,
	EV_OBITUARY,
	EV_STOPSTREAMINGSOUND,		// JPW NERVE swiped from sherman
	EV_POWERUP_QUAD,
	EV_POWERUP_BATTLESUIT,
	EV_POWERUP_REGEN,
	EV_GIB_PLAYER,				// gib a previously living player
	EV_DEBUG_LINE,
	EV_STOPLOOPINGSOUND,
	EV_TAUNT,
	EV_SMOKE,
	EV_SPARKS,
	EV_SPARKS_ELECTRIC,
	EV_EXPLODE,					// func_explosive
	EV_RUBBLE,
	EV_EFFECT,					// target_effect
	EV_MORTAREFX,				// mortar firing
	EV_SPINUP,					// JPW NERVE panzerfaust preamble
	EV_SNOW_ON,
	EV_SNOW_OFF,
	EV_MISSILE_MISS_SMALL,
	EV_MISSILE_MISS_LARGE,
	EV_MORTAR_IMPACT,
	EV_MORTAR_MISS,
	EV_SPIT_HIT,
	EV_SPIT_MISS,
	EV_SHARD,
	EV_JUNK,
	EV_EMITTER,					//----(SA)  added // generic particle emitter that uses client-side particle scripts
	EV_OILPARTICLES,
	EV_OILSLICK,
	EV_OILSLICKREMOVE,
	EV_MG42EFX,
	EV_FLAKGUN1,
	EV_FLAKGUN2,
	EV_FLAKGUN3,
	EV_FLAKGUN4,
	EV_EXERT1,
	EV_EXERT2,
	EV_EXERT3,
	EV_SNOWFLURRY,
	EV_CONCUSSIVE,
	EV_DUST,
	EV_RUMBLE_EFX,
	EV_GUNSPARKS,
	EV_FLAMETHROWER_EFFECT,
	EV_POPUP,
	EV_POPUPBOOK,
	EV_GIVEPAGE,
	EV_MG42BULLET_HIT_FLESH,	// Arnout: these two send the seed as well
	EV_MG42BULLET_HIT_WALL,
	EV_SHAKE,
	EV_DISGUISE_SOUND,
	EV_BUILDDECAYED_SOUND,
	EV_FIRE_WEAPON_AAGUN,
	EV_DEBRIS,
	EV_ALERT_SPEAKER,
	EV_POPUPMESSAGE,
	EV_ARTYMESSAGE,
	EV_AIRSTRIKEMESSAGE,
	EV_MEDIC_CALL,
	EV_MAX_EVENTS				// just added as an 'endcap'
} entity_event_t;


// new (10/18/00)
typedef enum
{
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEAD1_WATER,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEAD2_WATER,
	BOTH_DEATH3,
	BOTH_DEAD3,
	BOTH_DEAD3_WATER,

	BOTH_CLIMB,
/*10*/ BOTH_CLIMB_DOWN,
	BOTH_CLIMB_DISMOUNT,

	BOTH_SALUTE,

	BOTH_PAIN1,					// head
	BOTH_PAIN2,					// chest
	BOTH_PAIN3,					// groin
	BOTH_PAIN4,					// right shoulder
	BOTH_PAIN5,					// left shoulder
	BOTH_PAIN6,					// right knee
	BOTH_PAIN7,					// left knee
	/*20 */ BOTH_PAIN8,
	// dazed

	BOTH_GRAB_GRENADE,

	BOTH_ATTACK1,
	BOTH_ATTACK2,
	BOTH_ATTACK3,
	BOTH_ATTACK4,
	BOTH_ATTACK5,

	BOTH_EXTRA1,
	BOTH_EXTRA2,
	BOTH_EXTRA3,
/*30*/ BOTH_EXTRA4,
	BOTH_EXTRA5,
	BOTH_EXTRA6,
	BOTH_EXTRA7,
	BOTH_EXTRA8,
	BOTH_EXTRA9,
	BOTH_EXTRA10,
	BOTH_EXTRA11,
	BOTH_EXTRA12,
	BOTH_EXTRA13,
/*40*/ BOTH_EXTRA14,
	BOTH_EXTRA15,
	BOTH_EXTRA16,
	BOTH_EXTRA17,
	BOTH_EXTRA18,
	BOTH_EXTRA19,
	BOTH_EXTRA20,

	TORSO_GESTURE,
	TORSO_GESTURE2,
	TORSO_GESTURE3,
/*50*/ TORSO_GESTURE4,

	TORSO_DROP,

	TORSO_RAISE,				// (low)
	TORSO_ATTACK,
	TORSO_STAND,
	TORSO_STAND_ALT1,
	TORSO_STAND_ALT2,
	TORSO_READY,
	TORSO_RELAX,

	TORSO_RAISE2,				// (high)
/*60*/ TORSO_ATTACK2,
	TORSO_STAND2,
	TORSO_STAND2_ALT1,
	TORSO_STAND2_ALT2,
	TORSO_READY2,
	TORSO_RELAX2,

	TORSO_RAISE3,				// (pistol)
	TORSO_ATTACK3,
	TORSO_STAND3,
	TORSO_STAND3_ALT1,
/*70*/ TORSO_STAND3_ALT2,
	TORSO_READY3,
	TORSO_RELAX3,

	TORSO_RAISE4,				// (shoulder)
	TORSO_ATTACK4,
	TORSO_STAND4,
	TORSO_STAND4_ALT1,
	TORSO_STAND4_ALT2,
	TORSO_READY4,
	TORSO_RELAX4,

	/*80 */ TORSO_RAISE5,
	// (throw)
	TORSO_ATTACK5,
	TORSO_ATTACK5B,
	TORSO_STAND5,
	TORSO_STAND5_ALT1,
	TORSO_STAND5_ALT2,
	TORSO_READY5,
	TORSO_RELAX5,

	TORSO_RELOAD1,				// (low)
	TORSO_RELOAD2,				// (high)
	/*90 */ TORSO_RELOAD3,
	// (pistol)
	TORSO_RELOAD4,				// (shoulder)

	TORSO_MG42,					// firing tripod mounted weapon animation

	TORSO_MOVE,					// torso anim to play while moving and not firing (swinging arms type thing)
	TORSO_MOVE_ALT,

	TORSO_EXTRA,
	TORSO_EXTRA2,
	TORSO_EXTRA3,
	TORSO_EXTRA4,
	TORSO_EXTRA5,
/*100*/ TORSO_EXTRA6,
	TORSO_EXTRA7,
	TORSO_EXTRA8,
	TORSO_EXTRA9,
	TORSO_EXTRA10,

	LEGS_WALKCR,
	LEGS_WALKCR_BACK,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
/*110*/ LEGS_SWIM,
	LEGS_SWIM_IDLE,

	LEGS_JUMP,
	LEGS_JUMPB,
	LEGS_LAND,

	LEGS_IDLE,
	LEGS_IDLE_ALT,				// LEGS_IDLE2
	LEGS_IDLECR,

	LEGS_TURN,

	LEGS_BOOT,					// kicking animation

/*120*/ LEGS_EXTRA1,
	LEGS_EXTRA2,
	LEGS_EXTRA3,
	LEGS_EXTRA4,
	LEGS_EXTRA5,
	LEGS_EXTRA6,
	LEGS_EXTRA7,
	LEGS_EXTRA8,
	LEGS_EXTRA9,
	LEGS_EXTRA10,

/*130*/ MAX_ANIMATIONS
} animNumber_t;

// text represenation for scripting
extern char    *animStrings[];	// defined in bg_misc.c
extern char    *animStringsOld[];	// defined in bg_misc.c


typedef enum
{
	WEAP_IDLE1,
	WEAP_IDLE2,
	WEAP_ATTACK1,
	WEAP_ATTACK2,
	WEAP_ATTACK_LASTSHOT,		// used when firing the last round before having an empty clip.
	WEAP_DROP,
	WEAP_RAISE,
	WEAP_RELOAD1,
	WEAP_RELOAD2,
	WEAP_RELOAD3,
	WEAP_ALTSWITCHFROM,			// switch from alt fire mode weap (scoped/silencer/etc)
	WEAP_ALTSWITCHTO,			// switch to alt fire mode weap
	WEAP_DROP2,
	MAX_WP_ANIMATIONS
} weapAnimNumber_t;

typedef enum hudHeadAnimNumber_s
{
	HD_IDLE1,
	HD_IDLE2,
	HD_IDLE3,
	HD_IDLE4,
	HD_IDLE5,
	HD_IDLE6,
	HD_IDLE7,
	HD_IDLE8,
	HD_DAMAGED_IDLE1,
	HD_DAMAGED_IDLE2,
	HD_DAMAGED_IDLE3,
	HD_LEFT,
	HD_RIGHT,
	HD_ATTACK,
	HD_ATTACK_END,
	HD_PAIN,
	MAX_HD_ANIMATIONS
} hudHeadAnimNumber_t;

#define ANIMFL_LADDERANIM   0x1
#define ANIMFL_FIRINGANIM   0x2
#define ANIMFL_REVERSED     0x4

typedef struct animation_s
{
#ifdef CGAMEDLL
	qhandle_t       mdxFile;
#else
	char            mdxFileName[MAX_QPATH];
#endif							// CGAMEDLL
	char            name[MAX_QPATH];
	int             firstFrame;
	int             numFrames;
	int             loopFrames;	// 0 to numFrames
	int             frameLerp;	// msec between frames
	int             initialLerp;	// msec to get to first frame
	int             moveSpeed;
	int             animBlend;	// take this long to blend to next anim

	//
	// derived
	//
	int             duration;
	int             nameHash;
	int             flags;
	int             movetype;
} animation_t;

// Ridah, head animations
typedef enum
{
	HEAD_NEUTRAL_CLOSED,
	HEAD_NEUTRAL_A,
	HEAD_NEUTRAL_O,
	HEAD_NEUTRAL_I,
	HEAD_NEUTRAL_E,
	HEAD_HAPPY_CLOSED,
	HEAD_HAPPY_O,
	HEAD_HAPPY_I,
	HEAD_HAPPY_E,
	HEAD_HAPPY_A,
	HEAD_ANGRY_CLOSED,
	HEAD_ANGRY_O,
	HEAD_ANGRY_I,
	HEAD_ANGRY_E,
	HEAD_ANGRY_A,

	MAX_HEAD_ANIMS
} animHeadNumber_t;

typedef struct headAnimation_s
{
	int             firstFrame;
	int             numFrames;
} headAnimation_t;

// done.

// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define ANIM_TOGGLEBIT      ( 1 << ( ANIM_BITS - 1 ) )

// Gordon: renamed these to team_axis/allies, it really was awful....
typedef enum
{
	TEAM_FREE,
	TEAM_AXIS,
	TEAM_ALLIES,
	TEAM_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME       1000

// OSP - weapon stat info: mapping between MOD_ and WP_ types (FIXME for new ET weapons)
typedef enum extWeaponStats_s
{
	WS_KNIFE,					// 0
	WS_LUGER,					// 1
	WS_COLT,					// 2
	WS_MP40,					// 3
	WS_THOMPSON,				// 4
	WS_STEN,					// 5
	WS_FG42,					// 6    -- Also includes WS_BAR (allies version of fg42)
	WS_PANZERFAUST,				// 7
	WS_FLAMETHROWER,			// 8
	WS_GRENADE,					// 9    -- Includes axis and allies grenade types
	WS_MORTAR,					// 10
	WS_DYNAMITE,				// 11
	WS_AIRSTRIKE,				// 12   -- Lt. smoke grenade attack
	WS_ARTILLERY,				// 13   -- Lt. binocular attack
	WS_SYRINGE,					// 14   -- Medic syringe uses/successes

	WS_SMOKE,					// 15
	WS_SATCHEL,					// 16
	WS_GRENADELAUNCHER,			// 17
	WS_LANDMINE,				// 18
	WS_MG42,					// 19
	WS_GARAND,					// 20 // Gordon: (carbine and garand)
	WS_K43,						// 21 // Gordon: (kar98 and k43)

	WS_MAX
} extWeaponStats_t;

typedef struct
{
	qboolean        fHasHeadShots;
	const char     *pszCode;
	const char     *pszName;
} weap_ws_t;

extern const weap_ws_t aWeaponInfo[WS_MAX];

// OSP

// means of death
typedef enum
{
	MOD_UNKNOWN,
	MOD_MACHINEGUN,
	MOD_BROWNING,
	MOD_MG42,
	MOD_GRENADE,
	MOD_ROCKET,

	// (SA) modified wolf weap mods
	MOD_KNIFE,
	MOD_LUGER,
	MOD_COLT,
	MOD_MP40,
	MOD_THOMPSON,
	MOD_STEN,
	MOD_GARAND,
	MOD_SNOOPERSCOPE,
	MOD_SILENCER,				//----(SA)
	MOD_FG42,
	MOD_FG42SCOPE,
	MOD_PANZERFAUST,
	MOD_GRENADE_LAUNCHER,
	MOD_FLAMETHROWER,
	MOD_GRENADE_PINEAPPLE,
	MOD_CROSS,
	// end

	MOD_MAPMORTAR,
	MOD_MAPMORTAR_SPLASH,

	MOD_KICKED,
	MOD_GRABBER,

	MOD_DYNAMITE,
	MOD_AIRSTRIKE,				// JPW NERVE
	MOD_SYRINGE,				// JPW NERVE
	MOD_AMMO,					// JPW NERVE
	MOD_ARTY,					// JPW NERVE

	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
	MOD_EXPLOSIVE,

	MOD_CARBINE,
	MOD_KAR98,
	MOD_GPG40,
	MOD_M7,
	MOD_LANDMINE,
	MOD_SATCHEL,
	MOD_TRIPMINE,
	MOD_SMOKEBOMB,
	MOD_MOBILE_MG42,
	MOD_SILENCED_COLT,
	MOD_GARAND_SCOPE,

	MOD_CRUSH_CONSTRUCTION,
	MOD_CRUSH_CONSTRUCTIONDEATH,
	MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER,

	MOD_K43,
	MOD_K43_SCOPE,

	MOD_MORTAR,

	MOD_AKIMBO_COLT,
	MOD_AKIMBO_LUGER,
	MOD_AKIMBO_SILENCEDCOLT,
	MOD_AKIMBO_SILENCEDLUGER,

	MOD_SMOKEGRENADE,

	// RF
	MOD_SWAP_PLACES,

	// OSP -- keep these 2 entries last
	MOD_SWITCHTEAM,

	MOD_NUM_MODS
} meansOfDeath_t;


//---------------------------------------------------------

// gitem_t->type
typedef enum
{
	IT_BAD,
	IT_WEAPON,					// EFX: rotate + upscale + minlight

	IT_AMMO,					// EFX: rotate
	IT_ARMOR,					// EFX: rotate + minlight
	IT_HEALTH,					// EFX: static external sphere + rotating internal
	IT_HOLDABLE,				// single use, holdable item
	// EFX: rotate + bob
	IT_KEY,
	IT_TREASURE,				// gold bars, etc.  things that can be picked up and counted for a tally at end-level
	IT_TEAM,
} itemType_t;

#define MAX_ITEM_MODELS 3
#define MAX_ITEM_ICONS 4

// JOSEPH 4-17-00
typedef struct gitem_s
{
	char           *classname;	// spawning name
	char           *pickup_sound;
	char           *world_model[MAX_ITEM_MODELS];

	char           *icon;
	char           *ammoicon;
	char           *pickup_name;	// for printing on pickup

	int             quantity;	// for ammo how much, or duration of powerup (value not necessary for ammo/health.  that value set in gameskillnumber[] below)
	itemType_t      giType;		// IT_* flags

	int             giTag;

	int             giAmmoIndex;	// type of weapon ammo this uses.  (ex. WP_MP40 and WP_LUGER share 9mm ammo, so they both have WP_LUGER for giAmmoIndex)
	int             giClipIndex;	// which clip this weapon uses.  this allows the sniper rifle to use the same clip as the garand, etc.

	char           *precaches;	// string of all models and images this item will use
	char           *sounds;		// string of all sounds this item will use

//  int         gameskillnumber[5];
} gitem_t;

// END JOSEPH

// included in both the game dll and the client
extern gitem_t  bg_itemlist[];
extern int      bg_numItems;

gitem_t        *BG_FindItem(const char *pickupName);
gitem_t        *BG_FindItemForClassName(const char *className);
gitem_t        *BG_FindItemForWeapon(weapon_t weapon);
gitem_t        *BG_FindItemForPowerup(powerup_t pw);
gitem_t        *BG_FindItemForHoldable(holdable_t pw);
gitem_t        *BG_FindItemForAmmo(int weapon);

//gitem_t *BG_FindItemForKey        ( wkey_t k, int *index );
weapon_t        BG_FindAmmoForWeapon(weapon_t weapon);
weapon_t        BG_FindClipForWeapon(weapon_t weapon);

qboolean        BG_AkimboFireSequence(int weapon, int akimboClip, int mainClip);
qboolean        BG_IsAkimboWeapon(int weaponNum);
qboolean        BG_IsAkimboSideArm(int weaponNum, playerState_t * ps);
int             BG_AkimboSidearm(int weaponNum);

#define ITEM_INDEX( x ) ( ( x ) - bg_itemlist )

qboolean        BG_CanUseWeapon(int classNum, int teamNum, weapon_t weapon);

qboolean        BG_CanItemBeGrabbed(const entityState_t * ent, const playerState_t * ps, int *skill, int teamNum);


// content masks
#define MASK_ALL                ( -1 )
#define MASK_SOLID              ( CONTENTS_SOLID )
#define MASK_PLAYERSOLID        ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY )
#define MASK_DEADSOLID          ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP )
#define MASK_WATER              ( CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME )
//#define   MASK_OPAQUE             (CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define MASK_OPAQUE             ( CONTENTS_SOLID | CONTENTS_LAVA )	//----(SA)  modified since slime is no longer deadly
#define MASK_SHOT               ( CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE )
#define MASK_MISSILESHOT        ( MASK_SHOT | CONTENTS_MISSILECLIP )

//
// entityState_t->eType
//

// cursorhints (stored in ent->s.dmgFlags since that's only used for players at the moment)
typedef enum
{
	HINT_NONE,					// reserved
	HINT_FORCENONE,				// reserved
	HINT_PLAYER,
	HINT_ACTIVATE,
	HINT_DOOR,
	HINT_DOOR_ROTATING,
	HINT_DOOR_LOCKED,
	HINT_DOOR_ROTATING_LOCKED,
	HINT_MG42,
	HINT_BREAKABLE,
	HINT_BREAKABLE_DYNAMITE,
	HINT_CHAIR,
	HINT_ALARM,
	HINT_HEALTH,
	HINT_TREASURE,
	HINT_KNIFE,
	HINT_LADDER,
	HINT_BUTTON,
	HINT_WATER,
	HINT_CAUTION,
	HINT_DANGER,
	HINT_SECRET,
	HINT_QUESTION,
	HINT_EXCLAMATION,
	HINT_CLIPBOARD,
	HINT_WEAPON,
	HINT_AMMO,
	HINT_ARMOR,
	HINT_POWERUP,
	HINT_HOLDABLE,
	HINT_INVENTORY,
	HINT_SCENARIC,
	HINT_EXIT,
	HINT_NOEXIT,
	HINT_PLYR_FRIEND,
	HINT_PLYR_NEUTRAL,
	HINT_PLYR_ENEMY,
	HINT_PLYR_UNKNOWN,
	HINT_BUILD,					// DHM - Nerve
	HINT_DISARM,				// DHM - Nerve
	HINT_REVIVE,				// DHM - Nerve
	HINT_DYNAMITE,				// DHM - Nerve
	HINT_CONSTRUCTIBLE,
	HINT_UNIFORM,
	HINT_LANDMINE,
	HINT_TANK,
	HINT_SATCHELCHARGE,
	HINT_LOCKPICK,

	HINT_BAD_USER,				// invisible user with no target

	HINT_NUM_HINTS
} hintType_t;



void            BG_EvaluateTrajectory(const trajectory_t * tr, int atTime, vec3_t result, qboolean isAngle, int splinePath);
void            BG_EvaluateTrajectoryDelta(const trajectory_t * tr, int atTime, vec3_t result, qboolean isAngle, int splineData);
void            BG_GetMarkDir(const vec3_t dir, const vec3_t normal, vec3_t out);

void            BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t * ps);

//void  BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad );

void            BG_PlayerStateToEntityState(playerState_t * ps, entityState_t * s, qboolean snap);
void            BG_PlayerStateToEntityStateExtraPolate(playerState_t * ps, entityState_t * s, int time, qboolean snap);
weapon_t        BG_DuplicateWeapon(weapon_t weap);
gitem_t        *BG_ValidStatWeapon(weapon_t weap);
weapon_t        BG_WeaponForMOD(int MOD);

qboolean        BG_WeaponInWolfMP(int weapon);
qboolean        BG_PlayerTouchesItem(playerState_t * ps, entityState_t * item, int atTime);
qboolean        BG_PlayerSeesItem(playerState_t * ps, entityState_t * item, int atTime);
qboolean        BG_AddMagicAmmo(playerState_t * ps, int *skill, int teamNum, int numOfClips);

#define OVERCLIP        1.001

//----(SA)  removed PM_ammoNeeded 11/27/00
void            PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce);

//#define ARENAS_PER_TIER       4
#define MAX_ARENAS          64
#define MAX_ARENAS_TEXT     8192

#define MAX_BOTS            64
#define MAX_BOTS_TEXT       8192

#define MAX_CAMPAIGNS_TEXT  8192

typedef enum
{
	FOOTSTEP_NORMAL,
	FOOTSTEP_METAL,
	FOOTSTEP_WOOD,
	FOOTSTEP_GRASS,
	FOOTSTEP_GRAVEL,
	FOOTSTEP_SPLASH,
	FOOTSTEP_ROOF,
	FOOTSTEP_SNOW,
	FOOTSTEP_CARPET,

	FOOTSTEP_TOTAL
} footstep_t;

typedef enum
{
	BRASSSOUND_METAL = 0,
	BRASSSOUND_SOFT,
	BRASSSOUND_STONE,
	BRASSSOUND_WOOD,
	BRASSSOUND_MAX,
} brassSound_t;

typedef enum
{
	FXTYPE_WOOD = 0,
	FXTYPE_GLASS,
	FXTYPE_METAL,
	FXTYPE_GIBS,
	FXTYPE_BRICK,
	FXTYPE_STONE
} fxType_t;

//==================================================================
// New Animation Scripting Defines

#define MAX_ANIMSCRIPT_MODELS               32
#define MAX_ANIMSCRIPT_ITEMS_PER_MODEL      2048
#define MAX_MODEL_ANIMATIONS                512	// animations per model
#define MAX_ANIMSCRIPT_ANIMCOMMANDS         8
#define MAX_ANIMSCRIPT_ITEMS                128
// NOTE: these must all be in sync with string tables in bg_animation.c

typedef enum
{
	ANIM_MT_UNUSED,
	ANIM_MT_IDLE,
	ANIM_MT_IDLECR,
	ANIM_MT_WALK,
	ANIM_MT_WALKBK,
	ANIM_MT_WALKCR,
	ANIM_MT_WALKCRBK,
	ANIM_MT_RUN,
	ANIM_MT_RUNBK,
	ANIM_MT_SWIM,
	ANIM_MT_SWIMBK,
	ANIM_MT_STRAFERIGHT,
	ANIM_MT_STRAFELEFT,
	ANIM_MT_TURNRIGHT,
	ANIM_MT_TURNLEFT,
	ANIM_MT_CLIMBUP,
	ANIM_MT_CLIMBDOWN,
	ANIM_MT_FALLEN,				// DHM - Nerve :: dead, before limbo
	ANIM_MT_PRONE,
	ANIM_MT_PRONEBK,
	ANIM_MT_IDLEPRONE,
	ANIM_MT_FLAILING,
//  ANIM_MT_TALK,
	ANIM_MT_SNEAK,
	ANIM_MT_AFTERBATTLE,		// xkan, 1/8/2003, just finished battle

	NUM_ANIM_MOVETYPES
} scriptAnimMoveTypes_t;

typedef enum
{
	ANIM_ET_PAIN,
	ANIM_ET_DEATH,
	ANIM_ET_FIREWEAPON,
	ANIM_ET_FIREWEAPON2,
	ANIM_ET_JUMP,
	ANIM_ET_JUMPBK,
	ANIM_ET_LAND,
	ANIM_ET_DROPWEAPON,
	ANIM_ET_RAISEWEAPON,
	ANIM_ET_CLIMB_MOUNT,
	ANIM_ET_CLIMB_DISMOUNT,
	ANIM_ET_RELOAD,
	ANIM_ET_PICKUPGRENADE,
	ANIM_ET_KICKGRENADE,
	ANIM_ET_QUERY,
	ANIM_ET_INFORM_FRIENDLY_OF_ENEMY,
	ANIM_ET_KICK,
	ANIM_ET_REVIVE,
	ANIM_ET_FIRSTSIGHT,
	ANIM_ET_ROLL,
	ANIM_ET_FLIP,
	ANIM_ET_DIVE,
	ANIM_ET_PRONE_TO_CROUCH,
	ANIM_ET_BULLETIMPACT,
	ANIM_ET_INSPECTSOUND,
	ANIM_ET_SECONDLIFE,
	ANIM_ET_DO_ALT_WEAPON_MODE,
	ANIM_ET_UNDO_ALT_WEAPON_MODE,
	ANIM_ET_DO_ALT_WEAPON_MODE_PRONE,
	ANIM_ET_UNDO_ALT_WEAPON_MODE_PRONE,
	ANIM_ET_FIREWEAPONPRONE,
	ANIM_ET_FIREWEAPON2PRONE,
	ANIM_ET_RAISEWEAPONPRONE,
	ANIM_ET_RELOADPRONE,
	ANIM_ET_TALK,
	ANIM_ET_NOPOWER,

	NUM_ANIM_EVENTTYPES
} scriptAnimEventTypes_t;

typedef enum
{
	ANIM_BP_UNUSED,
	ANIM_BP_LEGS,
	ANIM_BP_TORSO,
	ANIM_BP_BOTH,

	NUM_ANIM_BODYPARTS
} animBodyPart_t;

typedef enum
{
	ANIM_COND_WEAPON,
	ANIM_COND_ENEMY_POSITION,
	ANIM_COND_ENEMY_WEAPON,
	ANIM_COND_UNDERWATER,
	ANIM_COND_MOUNTED,
	ANIM_COND_MOVETYPE,
	ANIM_COND_UNDERHAND,
	ANIM_COND_LEANING,
	ANIM_COND_IMPACT_POINT,
	ANIM_COND_CROUCHING,
	ANIM_COND_STUNNED,
	ANIM_COND_FIRING,
	ANIM_COND_SHORT_REACTION,
	ANIM_COND_ENEMY_TEAM,
	ANIM_COND_PARACHUTE,
	ANIM_COND_CHARGING,
	ANIM_COND_SECONDLIFE,
	ANIM_COND_HEALTH_LEVEL,
	ANIM_COND_FLAILING_TYPE,
	ANIM_COND_GEN_BITFLAG,		// xkan 1/15/2003 - general bit flags (to save some space)
	ANIM_COND_AISTATE,			// xkan 1/17/2003 - our current ai state (sometimes more convenient than creating a separate section)

	NUM_ANIM_CONDITIONS
} scriptAnimConditions_t;

//-------------------------------------------------------------------

typedef struct
{
	char           *string;
	int             hash;
} animStringItem_t;

typedef struct
{
	int             index;		// reference into the table of possible conditionals
	int             value[2];	// can store anything from weapon bits, to position enums, etc
} animScriptCondition_t;

typedef struct
{
	short int       bodyPart[2];	// play this animation on legs/torso/both
	short int       animIndex[2];	// animation index in our list of animations
	short int       animDuration[2];
	short int       soundIndex;
} animScriptCommand_t;

typedef struct
{
	int             numConditions;
	animScriptCondition_t conditions[NUM_ANIM_CONDITIONS];
	int             numCommands;
	animScriptCommand_t commands[MAX_ANIMSCRIPT_ANIMCOMMANDS];
} animScriptItem_t;

typedef struct
{
	int             numItems;
	animScriptItem_t *items[MAX_ANIMSCRIPT_ITEMS];	// pointers into a global list of items
} animScript_t;

typedef struct
{
	char            animationGroup[MAX_QPATH];
	char            animationScript[MAX_QPATH];

	// parsed from the start of the cfg file (this is basically obsolete now - need to get rid of it)
	gender_t        gender;
	footstep_t      footsteps;
	vec3_t          headOffset;
	int             version;
	qboolean        isSkeletal;

	// parsed from animgroup file
	animation_t    *animations[MAX_MODEL_ANIMATIONS];	// anim names, frame ranges, etc
	headAnimation_t headAnims[MAX_HEAD_ANIMS];
	int             numAnimations, numHeadAnims;

	// parsed from script file
	animScript_t    scriptAnims[MAX_AISTATES][NUM_ANIM_MOVETYPES];	// locomotive anims, etc
	animScript_t    scriptCannedAnims[NUM_ANIM_MOVETYPES];	// played randomly
	animScript_t    scriptEvents[NUM_ANIM_EVENTTYPES];	// events that trigger special anims

	// global list of script items for this model
	animScriptItem_t scriptItems[MAX_ANIMSCRIPT_ITEMS_PER_MODEL];
	int             numScriptItems;

} animModelInfo_t;

// this is the main structure that is duplicated on the client and server
typedef struct
{
//  int                 clientModels[MAX_CLIENTS];      // so we know which model each client is using
	animModelInfo_t modelInfo[MAX_ANIMSCRIPT_MODELS];
	int             clientConditions[MAX_CLIENTS][NUM_ANIM_CONDITIONS][2];
	//
	// pointers to functions from the owning module
	//
	// TTimo: constify the arg
	int             (*soundIndex) (const char *name);
	void            (*playSound) (int soundIndex, vec3_t org, int clientNum);
} animScriptData_t;

//------------------------------------------------------------------
// Conditional Constants

typedef enum
{
	POSITION_UNUSED,
	POSITION_BEHIND,
	POSITION_INFRONT,
	POSITION_RIGHT,
	POSITION_LEFT,

	NUM_ANIM_COND_POSITIONS
} animScriptPosition_t;

typedef enum
{
	MOUNTED_UNUSED,
	MOUNTED_MG42,
	MOUNTED_AAGUN,

	NUM_ANIM_COND_MOUNTED
} animScriptMounted_t;

typedef enum
{
	LEANING_UNUSED,
	LEANING_RIGHT,
	LEANING_LEFT,

	NUM_ANIM_COND_LEANING
} animScriptLeaning_t;

typedef enum
{
	IMPACTPOINT_UNUSED,
	IMPACTPOINT_HEAD,
	IMPACTPOINT_CHEST,
	IMPACTPOINT_GUT,
	IMPACTPOINT_GROIN,
	IMPACTPOINT_SHOULDER_RIGHT,
	IMPACTPOINT_SHOULDER_LEFT,
	IMPACTPOINT_KNEE_RIGHT,
	IMPACTPOINT_KNEE_LEFT,

	NUM_ANIM_COND_IMPACTPOINT
} animScriptImpactPoint_t;

typedef enum
{
	FLAILING_UNUSED,
	FLAILING_INAIR,
	FLAILING_VCRASH,
	FLAILING_HCRASH,

	NUM_ANIM_COND_FLAILING
} animScriptFlailingType_t;

typedef enum
{
/*	ANIM_BITFLAG_SNEAKING,
	ANIM_BITFLAG_AFTERBATTLE,*/
	ANIM_BITFLAG_ZOOMING,

	NUM_ANIM_COND_BITFLAG
} animScriptGenBitFlag_t;

typedef enum
{
	ACC_BELT_LEFT,				// belt left (lower)
	ACC_BELT_RIGHT,				// belt right (lower)
	ACC_BELT,					// belt (upper)
	ACC_BACK,					// back (upper)
	ACC_WEAPON,					// weapon (upper)
	ACC_WEAPON2,				// weapon2 (upper)
	ACC_HAT,					// hat (head)
	ACC_MOUTH2,					//
	ACC_MOUTH3,					//
	ACC_RANK,					//
	ACC_MAX						// this is bound by network limits, must change network stream to increase this
} accType_t;

#define ACC_NUM_MOUTH 3			// matches the above count

#define MAX_GIB_MODELS      16

#define MAX_WEAPS_PER_CLASS 10

typedef struct
{
	int             classNum;
	const char     *characterFile;
	const char     *iconName;
	const char     *iconArrow;

	weapon_t        classWeapons[MAX_WEAPS_PER_CLASS];

	qhandle_t       icon;
	qhandle_t       arrow;

} bg_playerclass_t;

typedef struct bg_character_s
{
	char            characterFile[MAX_QPATH];

#ifdef CGAMEDLL
	qhandle_t       mesh;
	qhandle_t       skin;

	qhandle_t       headModel;
	qhandle_t       headSkin;

	qhandle_t       accModels[ACC_MAX];
	qhandle_t       accSkins[ACC_MAX];

	qhandle_t       gibModels[MAX_GIB_MODELS];

	qhandle_t       undressedCorpseModel;
	qhandle_t       undressedCorpseSkin;

	qhandle_t       hudhead;
	qhandle_t       hudheadskin;
	animation_t     hudheadanimations[MAX_HD_ANIMATIONS];
#endif							// CGAMEDLL

	animModelInfo_t *animModelInfo;
} bg_character_t;

/*
==============================================================

SAVE

	12 -
	13 - (SA) added 'episode' tracking to savegame
	14 - RF added 'skill'
	15 - (SA) moved time info above the main game reading
	16 - (SA) added fog
	17 - (SA) rats, changed fog.
	18 - TTimo targetdeath fix
	   show_bug.cgi?id=434
	30 - Arnout: initial Enemy Territory implementation
	31 - Arnout: added new global fog

==============================================================
*/

#define SAVE_VERSION            31
#define SAVE_INFOSTRING_LENGTH  256

//------------------------------------------------------------------
// Global Function Decs

//animModelInfo_t *BG_ModelInfoForModelname( char *modelname );
void            BG_InitWeaponStrings(void);
void            BG_AnimParseAnimScript(animModelInfo_t * modelInfo, animScriptData_t * scriptData, const char *filename,
									   char *input);
int             BG_AnimScriptAnimation(playerState_t * ps, animModelInfo_t * modelInfo, scriptAnimMoveTypes_t movetype,
									   qboolean isContinue);
int             BG_AnimScriptCannedAnimation(playerState_t * ps, animModelInfo_t * modelInfo);
int             BG_AnimScriptEvent(playerState_t * ps, animModelInfo_t * modelInfo, scriptAnimEventTypes_t event,
								   qboolean isContinue, qboolean force);
int             BG_IndexForString(char *token, animStringItem_t * strings, qboolean allowFail);
int             BG_PlayAnimName(playerState_t * ps, animModelInfo_t * animModelInfo, char *animName, animBodyPart_t bodyPart,
								qboolean setTimer, qboolean isContinue, qboolean force);
void            BG_ClearAnimTimer(playerState_t * ps, animBodyPart_t bodyPart);
qboolean        BG_ValidAnimScript(int clientNum);
char           *BG_GetAnimString(animModelInfo_t * animModelInfo, int anim);
void            BG_UpdateConditionValue(int client, int condition, int value, qboolean checkConversion);
int             BG_GetConditionValue(int client, int condition, qboolean checkConversion);
qboolean        BG_GetConditionBitFlag(int client, int condition, int bitNumber);
void            BG_SetConditionBitFlag(int client, int condition, int bitNumber);
void            BG_ClearConditionBitFlag(int client, int condition, int bitNumber);
int             BG_GetAnimScriptAnimation(int client, animModelInfo_t * animModelInfo, aistateEnum_t aistate,
										  scriptAnimMoveTypes_t movetype);
void            BG_AnimUpdatePlayerStateConditions(pmove_t * pmove);
animation_t    *BG_AnimationForString(char *string, animModelInfo_t * animModelInfo);
animation_t    *BG_GetAnimationForIndex(animModelInfo_t * animModelInfo, int index);
int             BG_GetAnimScriptEvent(playerState_t * ps, scriptAnimEventTypes_t event);
int             PM_IdleAnimForWeapon(int weapon);
int             PM_RaiseAnimForWeapon(int weapon);
void            PM_ContinueWeaponAnim(int anim);

extern animStringItem_t animStateStr[];
extern animStringItem_t animBodyPartsStr[];

bg_playerclass_t *BG_GetPlayerClassInfo(int team, int cls);
bg_playerclass_t *BG_PlayerClassForPlayerState(playerState_t * ps);
qboolean        BG_ClassHasWeapon(bg_playerclass_t * classInfo, weapon_t weap);
qboolean        BG_WeaponIsPrimaryForClassAndTeam(int classnum, team_t team, weapon_t weapon);
int             BG_ClassWeaponCount(bg_playerclass_t * classInfo, team_t team);
const char     *BG_ShortClassnameForNumber(int classNum);
const char     *BG_ClassnameForNumber(int classNum);
const char     *BG_ClassLetterForNumber(int classNum);

void            BG_DisableClassWeapon(bg_playerclass_t * classinfo, int weapon);
void            BG_DisableWeaponForAllClasses(int weapon);

extern bg_playerclass_t bg_allies_playerclasses[NUM_PLAYER_CLASSES];
extern bg_playerclass_t bg_axis_playerclasses[NUM_PLAYER_CLASSES];

#define MAX_PATH_CORNERS        512

typedef struct
{
	char            name[64];
	vec3_t          origin;
} pathCorner_t;

extern int      numPathCorners;
extern pathCorner_t pathCorners[MAX_PATH_CORNERS];

#define NUM_EXPERIENCE_LEVELS 11

typedef enum
{
	ME_PLAYER,
	ME_PLAYER_REVIVE,
	ME_PLAYER_DISGUISED,
	ME_CONSTRUCT,
	ME_DESTRUCT,
	ME_DESTRUCT_2,
	ME_LANDMINE,
	ME_TANK,
	ME_TANK_DEAD,
	//ME_LANDMINE_ARMED,
	ME_COMMANDMAP_MARKER,
} mapEntityType_t;

extern const char *rankNames_Axis[NUM_EXPERIENCE_LEVELS];
extern const char *rankNames_Allies[NUM_EXPERIENCE_LEVELS];
extern const char *miniRankNames_Axis[NUM_EXPERIENCE_LEVELS];
extern const char *miniRankNames_Allies[NUM_EXPERIENCE_LEVELS];
extern const char *rankSoundNames_Axis[NUM_EXPERIENCE_LEVELS];
extern const char *rankSoundNames_Allies[NUM_EXPERIENCE_LEVELS];

#define MAX_SPLINE_PATHS        512
#define MAX_SPLINE_CONTROLS     4
#define MAX_SPLINE_SEGMENTS     16

typedef struct splinePath_s splinePath_t;

typedef struct
{
	vec3_t          start;
	vec3_t          v_norm;
	float           length;
} splineSegment_t;

struct splinePath_s
{
	pathCorner_t    point;

	char            strTarget[64];

	splinePath_t   *next;
	splinePath_t   *prev;

	pathCorner_t    controls[MAX_SPLINE_CONTROLS];
	int             numControls;
	splineSegment_t segments[MAX_SPLINE_SEGMENTS];

	float           length;

	qboolean        isStart;
	qboolean        isEnd;
};

extern int      numSplinePaths;
extern splinePath_t splinePaths[MAX_SPLINE_PATHS];

pathCorner_t   *BG_Find_PathCorner(const char *match);
splinePath_t   *BG_GetSplineData(int number, qboolean * backwards);
void            BG_AddPathCorner(const char *name, vec3_t origin);
splinePath_t   *BG_AddSplinePath(const char *name, const char *target, vec3_t origin);
void            BG_BuildSplinePaths();
splinePath_t   *BG_Find_Spline(const char *match);
float           BG_SplineLength(splinePath_t * pSpline);
void            BG_AddSplineControl(splinePath_t * spline, const char *name);
void            BG_LinearPathOrigin2(float radius, splinePath_t ** pSpline, float *deltaTime, vec3_t result, qboolean backwards);

int             BG_MaxAmmoForWeapon(weapon_t weaponNum, int *skill);

void            BG_InitLocations(vec2_t world_mins, vec2_t world_maxs);
char           *BG_GetLocationString(vec_t * pos);

// START Mad Doc - TDF
typedef struct botpool_x
{
	int             num;
	int             playerclass;
	int             rank;
	struct botpool_x *next;
} botpool_t;

// END Mad Doc - TDF

#define MAX_FIRETEAMS       12

extern const char *bg_fireteamNames[MAX_FIRETEAMS / 2];

typedef struct
{
	int             ident;
	char            joinOrder[MAX_CLIENTS];	// order in which clients joined the fire team (server), client uses to store if a client is on this fireteam
	int             leader;		// leader = joinOrder[0] on server, stored here on client
	qboolean        inuse;
	qboolean        priv;
} fireteamData_t;

long            BG_StringHashValue(const char *fname);
long            BG_StringHashValue_Lwr(const char *fname);

void            BG_RotatePoint(vec3_t point, const vec3_t matrix[3]);
void            BG_TransposeMatrix(const vec3_t matrix[3], vec3_t transpose[3]);
void            BG_CreateRotationMatrix(const vec3_t angles, vec3_t matrix[3]);

int             trap_PC_AddGlobalDefine(char *define);
int             trap_PC_LoadSource(const char *filename);
int             trap_PC_FreeSource(int handle);
int             trap_PC_ReadToken(int handle, pc_token_t * pc_token);
int             trap_PC_SourceFileAndLine(int handle, char *filename, int *line);
int             trap_PC_UnReadToken(int handle);

void            PC_SourceError(int handle, char *format, ...) __attribute__ ((format (printf, 2, 3)));
void            PC_SourceWarning(int handle, char *format, ...) __attribute__ ((format (printf, 2, 3)));

#ifdef GAMEDLL
const char     *PC_String_Parse(int handle);
const char     *PC_Line_Parse(int handle);
#else
const char     *String_Alloc(const char *p);
qboolean        PC_String_Parse(int handle, const char **out);
#endif
qboolean        PC_String_ParseNoAlloc(int handle, char *out, size_t size);
qboolean        PC_Int_Parse(int handle, int *i);
qboolean        PC_Color_Parse(int handle, vec4_t * c);
qboolean        PC_Vec_Parse(int handle, vec3_t * c);
qboolean        PC_Float_Parse(int handle, float *f);

void            BG_AdjustAAGunMuzzleForBarrel(vec_t * origin, vec_t * forward, vec_t * right, vec_t * up, int barrel);

int             BG_ClassTextToClass(char *token);
skillType_t     BG_ClassSkillForClass(int classnum);

qboolean        BG_isLightWeaponSupportingFastReload(int weapon);
qboolean        BG_IsScopedWeapon(int weapon);

int             BG_FootstepForSurface(int surfaceFlags);

#define MATCH_MINPLAYERS "4"	//"1"  // Minimum # of players needed to start a match

// Multiview support
int             BG_simpleHintsCollapse(int hint, int val);
int             BG_simpleHintsExpand(int hint, int val);
int             BG_simpleWeaponState(int ws);

// Color escape handling
int             BG_colorstrncpyz(char *in, char *out, int str_max, int out_max);
int             BG_drawStrlen(const char *str);
int             BG_strRelPos(char *in, int index);
int             BG_cleanName(const char *pszIn, char *pszOut, int dwMaxLength, qboolean fCRLF); // CHRUKER: b069 - Cleaned up a few compiler warnings

// Crosshair support
void            BG_setCrosshair(char *colString, float *col, float alpha, char *cvarName);

// Voting
#define VOTING_DISABLED     ( ( 1 << numVotesAvailable ) - 1 )

typedef struct
{
	const char     *pszCvar;
	int             flag;
} voteType_t;

extern const voteType_t voteToggles[];
extern int      numVotesAvailable;

// Tracemap
#ifdef CGAMEDLL
void            CG_GenerateTracemap(void);
#endif							// CGAMEDLL
qboolean        BG_LoadTraceMap(char *rawmapname, vec2_t world_mins, vec2_t world_maxs);
float           BG_GetSkyHeightAtPoint(vec3_t pos);
float           BG_GetSkyGroundHeightAtPoint(vec3_t pos);
float           BG_GetGroundHeightAtPoint(vec3_t pos);
int             BG_GetTracemapGroundFloor(void);
int             BG_GetTracemapGroundCeil(void);

//
// bg_animgroup.c
//
void            BG_ClearAnimationPool(void);
qboolean        BG_R_RegisterAnimationGroup(const char *filename, animModelInfo_t * animModelInfo);

//
// bg_character.c
//

typedef struct bg_characterDef_s
{
	char            mesh[MAX_QPATH];
	char            animationGroup[MAX_QPATH];
	char            animationScript[MAX_QPATH];
	char            skin[MAX_QPATH];
	char            undressedCorpseModel[MAX_QPATH];
	char            undressedCorpseSkin[MAX_QPATH];
	char            hudhead[MAX_QPATH];
	char            hudheadanims[MAX_QPATH];
	char            hudheadskin[MAX_QPATH];
} bg_characterDef_t;

qboolean        BG_ParseCharacterFile(const char *filename, bg_characterDef_t * characterDef);
bg_character_t *BG_GetCharacter(int team, int cls);
bg_character_t *BG_GetCharacterForPlayerstate(playerState_t * ps);
void            BG_ClearCharacterPool(void);
bg_character_t *BG_FindFreeCharacter(const char *characterFile);
bg_character_t *BG_FindCharacter(const char *characterFile);

//
// bg_sscript.c
//
typedef enum
{
	S_LT_NOT_LOOPED,
	S_LT_LOOPED_ON,
	S_LT_LOOPED_OFF
} speakerLoopType_t;

typedef enum
{
	S_BT_LOCAL,
	S_BT_GLOBAL,
	S_BT_NOPVS
} speakerBroadcastType_t;

typedef struct bg_speaker_s
{
	char            filename[MAX_QPATH];
	qhandle_t       noise;
	vec3_t          origin;
	char            targetname[32];
	long            targetnamehash;

	speakerLoopType_t loop;
	speakerBroadcastType_t broadcast;
	int             wait;
	int             random;
	int             volume;
	int             range;

	qboolean        activated;
	int             nextActivateTime;
	int             soundTime;
} bg_speaker_t;

void            BG_ClearScriptSpeakerPool(void);
int             BG_NumScriptSpeakers(void);
int             BG_GetIndexForSpeaker(bg_speaker_t * speaker);
bg_speaker_t   *BG_GetScriptSpeaker(int index);
qboolean        BG_SS_DeleteSpeaker(int index);
qboolean        BG_SS_StoreSpeaker(bg_speaker_t * speaker);
qboolean        BG_LoadSpeakerScript(const char *filename);

// Lookup table to find ammo table entry
extern ammotable_t ammoTableMP[WP_NUM_WEAPONS];

#define GetAmmoTableData( ammoIndex ) ( (ammotable_t*)( &ammoTableMP[ammoIndex] ) )

#define MAX_MAP_SIZE 65536

qboolean        BG_BBoxCollision(vec3_t min1, vec3_t max1, vec3_t min2, vec3_t max2);

//#define VISIBLE_TRIGGERS

//
// bg_stats.c
//

typedef struct weap_ws_convert_s
{
	weapon_t        iWeapon;
	extWeaponStats_t iWS;
} weap_ws_convert_t;

typedef struct
{
	meansOfDeath_t  iMOD;
	extWeaponStats_t iWS;
} mod_ws_convert_t;

extWeaponStats_t BG_WeapStatForWeapon(weapon_t iWeaponID);

typedef enum popupMessageType_e
{
	PM_DYNAMITE,
	PM_CONSTRUCTION,
	PM_MINES,
	PM_DEATH,
	PM_MESSAGE,
	PM_OBJECTIVE,
	PM_DESTRUCTION,
	PM_TEAM,
	PM_NUM_TYPES
} popupMessageType_t;

typedef enum popupMessageBigType_e
{
	PM_SKILL,
	PM_RANK,
	PM_DISGUISE,
	PM_BIG_NUM_TYPES
} popupMessageBigType_t;

#define NUM_HEAVY_WEAPONS 6
extern weapon_t bg_heavyWeapons[NUM_HEAVY_WEAPONS];

int             PM_AltSwitchFromForWeapon(int weapon);
int             PM_AltSwitchToForWeapon(int weapon);

void            PM_TraceLegs(trace_t * trace, float *legsOffset, vec3_t start, vec3_t end, trace_t * bodytrace, vec3_t viewangles,
							 void (tracefunc) (trace_t * results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
											   const vec3_t end, int passEntityNum, int contentMask), int ignoreent,
							 int tracemask);
void            PM_TraceAllLegs(trace_t * trace, float *legsOffset, vec3_t start, vec3_t end);
void            PM_TraceAll(trace_t * trace, vec3_t start, vec3_t end);

#endif
