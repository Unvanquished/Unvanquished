/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

This file is part of OpenWolf.

OpenWolf is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenWolf is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// g_local.h -- local definitions for game module

#include "../../../../engine/qcommon/q_shared.h"
#include "bg_public.h"
#include "../../../../engine/server/g_api.h"

//==================================================================

#define INFINITE      1000000

#define FRAMETIME     100         // msec
#define CARNAGE_REWARD_TIME 3000
#define REWARD_SPRITE_TIME  2000

#define INTERMISSION_DELAY_TIME 1000
#define SP_INTERMISSION_DELAY_TIME 5000

// gentity->flags
#define FL_GODMODE        0x00000010
#define FL_NOTARGET       0x00000020
#define FL_TEAMSLAVE      0x00000400  // not the first on the team
#define FL_NO_KNOCKBACK   0x00000800
#define FL_DROPPED_ITEM   0x00001000
#define FL_NO_BOTS        0x00002000  // spawn point not for bot use
#define FL_NO_HUMANS      0x00004000  // spawn point just for bots
#define FL_FORCE_GESTURE  0x00008000  // spawn point just for bots

// movers are things like doors, plats, buttons, etc
typedef enum
{
  MOVER_POS1,
  MOVER_POS2,
  MOVER_1TO2,
  MOVER_2TO1,

  ROTATOR_POS1,
  ROTATOR_POS2,
  ROTATOR_1TO2,
  ROTATOR_2TO1,

  MODEL_POS1,
  MODEL_POS2,
  MODEL_1TO2,
  MODEL_2TO1
} moverState_t;

#define SP_PODIUM_MODEL   "models/mapobjects/podium/podium4.md3"

//============================================================================

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

struct gentity_s
{
  entityState_t     s;        // communicated by server to clients
  entityShared_t    r;        // shared by both the server system and game

  // DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
  // EXPECTS THE FIELDS IN THAT ORDER!
  //================================

  struct gclient_s  *client;        // NULL if not a client

  qboolean          inuse;

  char              *classname;     // set in QuakeEd
  int               spawnflags;     // set in QuakeEd

  qboolean          neverFree;      // if true, FreeEntity will only unlink
                                    // bodyque uses this

  int               flags;          // FL_* variables

  char              *model;
  char              *model2;
  int               freetime;       // level.time when the object was freed

  int               eventTime;      // events will be cleared EVENT_VALID_MSEC after set
  qboolean          freeAfterEvent;
  qboolean          unlinkAfterEvent;

  qboolean          physicsObject;  // if true, it can be pushed by movers and fall off edges
                                    // all game items are physicsObjects,
  float             physicsBounce;  // 1.0 = continuous bounce, 0.0 = no bounce
  int               clipmask;       // brushes with this content value will be collided against
                                    // when moving.  items and corpses do not collide against
                                    // players, for instance

  // movers
  moverState_t      moverState;
  int               soundPos1;
  int               sound1to2;
  int               sound2to1;
  int               soundPos2;
  int               soundLoop;
  gentity_t         *parent;
  gentity_t         *nextTrain;
  gentity_t         *prevTrain;
  vec3_t            pos1, pos2;
  float             rotatorAngle;
  gentity_t         *clipBrush;   //TA: clipping brush for model doors

  char              *message;

  int               timestamp;      // body queue sinking, etc

  float             angle;          // set in editor, -1 = up, -2 = down
  char              *target;
  char              *targetname;
  char              *team;
  char              *targetShaderName;
  char              *targetShaderNewName;
  gentity_t         *target_ent;

  float             speed;
  float             lastSpeed;  //TA: used by trains that have been restarted
  vec3_t            movedir;

  //TA: acceleration evaluation
  qboolean          evaluateAcceleration;
  vec3_t            oldVelocity;
  vec3_t            acceleration;
  vec3_t            oldAccel;
  vec3_t            jerk;

  int               nextthink;
  void              (*think)( gentity_t *self );
  void              (*reached)( gentity_t *self );  // movers call this when hitting endpoint
  void              (*blocked)( gentity_t *self, gentity_t *other );
  void              (*touch)( gentity_t *self, gentity_t *other, trace_t *trace );
  void              (*use)( gentity_t *self, gentity_t *other, gentity_t *activator );
  void              (*pain)( gentity_t *self, gentity_t *attacker, int damage );
  void              (*die)( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod );

  int               pain_debounce_time;
  int               fly_sound_debounce_time;  // wind tunnel
  int               last_move_time;

  int               health;
  int               lastHealth; //TA: currently only used for overmind

  qboolean          takedamage;

  int               damage;
  int               splashDamage; // quad will increase this without increasing radius
  int               splashRadius;
  int               methodOfDeath;
  int               splashMethodOfDeath;
  int               chargeRepeat;

  int               count;

  gentity_t         *chain;
  gentity_t         *enemy;
  gentity_t         *activator;
  gentity_t         *teamchain;   // next entity in team
  gentity_t         *teammaster;  // master of the team

  int               watertype;
  int               waterlevel;

  int               noise_index;

  // timing variables
  float             wait;
  float             random;

  pTeam_t           stageTeam;
  stage_t           stageStage;

  int               biteam;       //TA: buildable item team
  gentity_t         *parentNode;  //TA: for creep and defence/spawn dependencies
  qboolean          active;       //TA: for power repeater, but could be useful elsewhere
  qboolean          powered;      //TA: for human buildables
  int               builtBy;      //TA: clientNum of person that built this
  gentity_t         *dccNode;     //TA: controlling dcc
  gentity_t         *overmindNode;//TA: controlling overmind
  qboolean          dcced;        //TA: controlled by a dcc or not?
  qboolean          spawned;      //TA: whether or not this buildable has finished spawning
  int               buildTime;    //TA: when this buildable was built
  int               time1000;     //TA: timer evaluated every second
  int               overmindAttackTimer;
  int               overmindDyingTimer;
  int               overmindSpawnsTimer;
  int               nextPhysicsTime; //TA: buildables don't need to check what they're sitting on
                                     //    every single frame.. so only do it periodically
  int               clientSpawnTime; //TA: the time until this spawn can spawn a client
  qboolean          lev1Grabbed;      //TA: for turrets interacting with lev1s
  int               lev1GrabTime;     //TA: for turrets interacting with lev1s

  int               credits[ MAX_CLIENTS ]; //TA: human credits for each client
  qboolean          creditsHash[ MAX_CLIENTS ]; //TA: track who has claimed credit
  int               killedBy;                   //TA: clientNum of killer

  gentity_t         *targeted;    //TA: true if the player is currently a valid target of a turret
  vec3_t            turretAim;    //TA: aim vector for turrets

  vec4_t            animation;    //TA: animated map objects

  gentity_t         *builder;     //TA: occupant of this hovel

  qboolean          nonSegModel;  //TA: this entity uses a nonsegmented player model

  buildable_t       bTriggers[ BA_NUM_BUILDABLES ]; //TA: which buildables are triggers
  pClass_t          cTriggers[ PCL_NUM_CLASSES ];   //TA: which classes are triggers
  weapon_t          wTriggers[ WP_NUM_WEAPONS ];    //TA: which weapons are triggers
  upgrade_t         uTriggers[ UP_NUM_UPGRADES ];   //TA: which upgrades are triggers

  int               triggerGravity;                 //TA: gravity for this trigger

  int               suicideTime;  //TA: when the client will suicide

  int               lastDamageTime;
};

typedef enum
{
  CON_DISCONNECTED,
  CON_CONNECTING,
  CON_CONNECTED
} clientConnected_t;

typedef enum
{
  SPECTATOR_NOT,
  SPECTATOR_FREE,
  SPECTATOR_LOCKED,
  SPECTATOR_FOLLOW,
  SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum
{
  TEAM_BEGIN,   // Beginning a team game, spawn at base
  TEAM_ACTIVE   // Now actively playing
} playerTeamStateState_t;

typedef struct
{
  playerTeamStateState_t  state;

  int     location;

  int     captures;
  int     basedefense;
  int     carrierdefense;
  int     flagrecovery;
  int     fragcarrier;
  int     assists;

  float   lasthurtcarrier;
  float   lastreturnedflag;
  float   flagsince;
  float   lastfraggedcarrier;
} playerTeamState_t;

// the auto following clients don't follow a specific client
// number, but instead follow the first two active players
#define FOLLOW_ACTIVE1  -1
#define FOLLOW_ACTIVE2  -2

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct
{
  team_t            sessionTeam;
  int               spectatorTime;    // for determining next-in-line to play
  spectatorState_t  spectatorState;
  int               spectatorClient;  // for chasecam and follow mode
  int               wins, losses;     // tournament stats
  qboolean          teamLeader;       // true when this client is a team leader
} clientSession_t;

#define MAX_NETNAME       36
#define MAX_VOTE_COUNT    3

// data to store details of clients that have abnormally disconnected
typedef struct connectionRecord_s
{
  int       clientNum;
  pTeam_t   clientTeam;
  int       clientCredit;

  int       ptrCode;
} connectionRecord_t;

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct
{
  clientConnected_t   connected;
  usercmd_t           cmd;                // we would lose angles if not persistant
  qboolean            localClient;        // true if "ip" info key is "localhost"
  qboolean            initialSpawn;       // the first spawn should be at a cool location
  qboolean            predictItemPickup;  // based on cg_predictItems userinfo
  qboolean            pmoveFixed;         //
  char                netname[ MAX_NETNAME ];
  int                 maxHealth;          // for handicapping
  int                 enterTime;          // level.time the client entered the game
  playerTeamState_t   teamState;          // status in teamplay games
  int                 voteCount;          // to prevent people from constantly calling votes
  int                 teamVoteCount;      // to prevent people from constantly calling votes
  qboolean            teamInfo;           // send team overlay updates?

  pClass_t            classSelection;     //TA: player class (copied to ent->client->ps.stats[ STAT_PCLASS ] once spawned)
  float               evolveHealthFraction;
  weapon_t            humanItemSelection; //TA: humans have a starting item
  pTeam_t             teamSelection;      //TA: player team (copied to ps.stats[ STAT_PTEAM ])

  qboolean            joinedATeam;        //TA: used to tell when a PTR code is valid
  connectionRecord_t  *connection;

  vec3_t              lastDeathLocation;
} clientPersistant_t;

// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s
{
  // ps MUST be the first element, because the server expects it
  playerState_t       ps;       // communicated by server to clients

  // the rest of the structure is private to game
  clientPersistant_t  pers;
  clientSession_t     sess;

  qboolean            readyToExit;    // wishes to leave the intermission

  qboolean            noclip;

  int                 lastCmdTime;    // level.time of last usercmd_t, for EF_CONNECTION
                                      // we can't just use pers.lastCommand.time, because
                                      // of the g_sycronousclients case
  int                 buttons;
  int                 oldbuttons;
  int                 latched_buttons;

  vec3_t              oldOrigin;

  // sum up damage over an entire frame, so
  // shotgun blasts give a single big kick
  int                 damage_armor;     // damage absorbed by armor
  int                 damage_blood;     // damage taken out of health
  int                 damage_knockback; // impact damage
  vec3_t              damage_from;      // origin for vector calculation
  qboolean            damage_fromWorld; // if true, don't use the damage_from vector

  //
  int                 lastkilled_client;// last client that this client killed
  int                 lasthurt_client;  // last client that damaged this client
  int                 lasthurt_mod;     // type of damage the client did

  // timers
  int                 respawnTime;      // can respawn when time > this, force after g_forcerespwan
  int                 inactivityTime;   // kick players when time > this
  qboolean            inactivityWarning;// qtrue if the five seoond warning has been given
  int                 rewardTime;       // clear the EF_AWARD_IMPRESSIVE, etc when time > this

  int                 airOutTime;

  int                 lastKillTime;     // for multiple kill rewards

  qboolean            fireHeld;         // used for hook
  qboolean            fire2Held;        // used for alt fire
  gentity_t           *hook;            // grapple hook if out

  int                 switchTeamTime;   // time the player switched teams

  // timeResidual is used to handle events that happen every second
  // like health / armor countdowns and regeneration
  //TA: two timers, one every 100 msecs, another every sec
  int                 time100;
  int                 time1000;
  int                 time10000;

  char                *areabits;

  gentity_t           *hovel;      //TA: body that is being infested. must be persistant

  int                 lastPoisonTime;
  int                 poisonImmunityTime;
  gentity_t           *lastPoisonClient;
  int                 lastPoisonCloudedTime;
  gentity_t           *lastPoisonCloudedClient;
  int                 grabExpiryTime;
  int                 lastLockTime;
  int                 lastSlowTime;
  int                 lastBoostedTime;
  int                 lastMedKitTime;
  int                 medKitHealthToRestore;
  int                 medKitIncrementTime;
  int                 lastCreepSlowTime; //TA: time until creep can be removed

  int                 pouncePayload;    //TA: amount of damage pounce attack will do
  qboolean            allowedToPounce;

  qboolean            charging;

  vec3_t              hovelOrigin;      //TA: player origin before entering hovel

  int                 lastFlameBall;    //TA: s.number of the last flame ball fired

#define RAM_FRAMES  1                       //TA: number of frames to wait before retriggering
  int                 retriggerArmouryMenu; //TA: frame number to retrigger the armoury menu
};


typedef struct spawnQueue_s
{
  int clients[ MAX_CLIENTS ];

  int front, back;
} spawnQueue_t;

#define QUEUE_PLUS1(x)  (((x)+1)%MAX_CLIENTS)
#define QUEUE_MINUS1(x) (((x)+MAX_CLIENTS-1)%MAX_CLIENTS)

void      G_InitSpawnQueue( spawnQueue_t *sq );
int       G_GetSpawnQueueLength( spawnQueue_t *sq );
int       G_PopSpawnQueue( spawnQueue_t *sq );
int       G_PeekSpawnQueue( spawnQueue_t *sq );
void      G_PushSpawnQueue( spawnQueue_t *sq, int clientNum );
qboolean  G_RemoveFromSpawnQueue( spawnQueue_t *sq, int clientNum );
int       G_GetPosInSpawnQueue( spawnQueue_t *sq, int clientNum );


#define MAX_LOCDAMAGE_TEXT    8192
#define MAX_LOCDAMAGE_REGIONS 16

//TA: store locational damage regions
typedef struct damageRegion_s
{
  float     minHeight, maxHeight;
  int       minAngle, maxAngle;

  float     modifier;

  qboolean  crouch;
} damageRegion_t;

#define MAX_ARMOUR_TEXT    8192
#define MAX_ARMOUR_REGIONS 16

//TA: store locational armour regions
typedef struct armourRegion_s
{
  float     minHeight, maxHeight;
  int       minAngle, maxAngle;

  float     modifier;

  qboolean  crouch;
} armourRegion_t;

//
// this structure is cleared as each map is entered
//
#define MAX_SPAWN_VARS      64
#define MAX_SPAWN_VARS_CHARS  4096

typedef struct
{
  struct gclient_s  *clients;   // [maxclients]

  struct gentity_s  *gentities;
  int               gentitySize;
  int               num_entities;   // current number, <= MAX_GENTITIES

  int               warmupTime;     // restart match at this time

  fileHandle_t      logFile;

  // store latched cvars here that we want to get at often
  int               maxclients;

  int               framenum;
  int               time;                         // in msec
  int               previousTime;                 // so movers can back up when blocked

  int               startTime;                    // level.time the map was started

  int               teamScores[ TEAM_NUM_TEAMS ];
  int               lastTeamLocationTime;         // last time of client team location update

  qboolean          newSession;                   // don't use any old session data, because
                                                  // we changed gametype

  qboolean          restarted;                    // waiting for a map_restart to fire

  int               numConnectedClients;
  int               numNonSpectatorClients;       // includes connecting clients
  int               numPlayingClients;            // connected, non-spectators
  int               sortedClients[MAX_CLIENTS];   // sorted by score
  int               follow1, follow2;             // clientNums for auto-follow spectators

  int               snd_fry;                      // sound index for standing in lava

  int               warmupModificationCount;      // for detecting if g_warmup is changed

  // voting state
  char              voteString[MAX_STRING_CHARS];
  char              voteDisplayString[MAX_STRING_CHARS];
  int               voteTime;                     // level.time vote was called
  int               voteExecuteTime;              // time the vote is executed
  int               voteYes;
  int               voteNo;
  int               numVotingClients;             // set by CalculateRanks

  // team voting state
  char              teamVoteString[ 2 ][ MAX_STRING_CHARS ];
  int               teamVoteTime[ 2 ];            // level.time vote was called
  int               teamVoteYes[ 2 ];
  int               teamVoteNo[ 2 ];
  int               numteamVotingClients[ 2 ];    // set by CalculateRanks

  // spawn variables
  qboolean          spawning;                     // the G_Spawn*() functions are valid
  int               numSpawnVars;
  char              *spawnVars[ MAX_SPAWN_VARS ][ 2 ];  // key / value pairs
  int               numSpawnVarChars;
  char              spawnVarChars[ MAX_SPAWN_VARS_CHARS ];

  // intermission state
  int               intermissionQueued;           // intermission was qualified, but
                                                  // wait INTERMISSION_DELAY_TIME before
                                                  // actually going there so the last
                                                  // frag can be watched.  Disable future
                                                  // kills during this delay
  int               intermissiontime;             // time the intermission was started
  char              *changemap;
  qboolean          readyToExit;                  // at least one client wants to exit
  int               exitTime;
  vec3_t            intermission_origin;          // also used for spectator spawns
  vec3_t            intermission_angle;

  qboolean          locationLinked;               // target_locations get linked
  gentity_t         *locationHead;                // head of the location list

  //TA: extra stuff:
  int               numAlienSpawns;
  int               numHumanSpawns;

  int               numAlienClients;
  int               numHumanClients;

  float             averageNumAlienClients;
  int               numAlienSamples;
  float             averageNumHumanClients;
  int               numHumanSamples;

  int               numLiveAlienClients;
  int               numLiveHumanClients;

  int               alienBuildPoints;
  int               humanBuildPoints;
  int               humanBuildPointsPowered;

  int               bankCredits[ MAX_CLIENTS ];   //global credits storage

  int               alienKills;
  int               humanKills;

  qboolean          reactorPresent;
  qboolean          overmindPresent;

  int               humanBaseAttackTimer;

  pTeam_t           lastWin;

  spawnQueue_t      alienSpawnQueue;
  spawnQueue_t      humanSpawnQueue;

  int               alienStage2Time;
  int               alienStage3Time;
  int               humanStage2Time;
  int               humanStage3Time;

  qboolean          uncondAlienWin;
  qboolean          uncondHumanWin;
} level_locals_t;

//
// g_spawn.c
//
qboolean  G_SpawnString( const char *key, const char *defaultString, char **out );
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean  G_SpawnFloat( const char *key, const char *defaultString, float *out );
qboolean  G_SpawnInt( const char *key, const char *defaultString, int *out );
qboolean  G_SpawnVector( const char *key, const char *defaultString, float *out );
void      G_SpawnEntitiesFromString( void );
char      *G_NewString( const char *string );

//
// g_cmds.c
//
void      Cmd_Score_f( gentity_t *ent );
void      G_StopFollowing( gentity_t *ent );
qboolean  G_FollowNewClient( gentity_t *ent, int dir );
void      Cmd_Follow_f( gentity_t *ent, qboolean toggle );

//
// g_physics.c
//
void G_Physics( gentity_t *ent, int msec );

//
// g_buildable.c
//

#define MAX_ALIEN_BBOX  25

typedef enum
{
  IBE_NONE,

  IBE_NOOVERMIND,
  IBE_OVERMIND,
  IBE_NOASSERT,
  IBE_SPWNWARN,
  IBE_NOCREEP,
  IBE_HOVEL,
  IBE_HOVELEXIT,

  IBE_REACTOR,
  IBE_REPEATER,
  IBE_TNODEWARN,
  IBE_RPTWARN,
  IBE_RPTWARN2,
  IBE_NOPOWER,
  IBE_NODCC,

  IBE_NORMAL,
  IBE_NOROOM,
  IBE_PERMISSION,

  IBE_MAXERRORS
} itemBuildError_t;

qboolean          AHovel_Blocked( gentity_t *hovel, gentity_t *player, qboolean provideExit );
gentity_t         *G_CheckSpawnPoint( int spawnNum, vec3_t origin, vec3_t normal,
                    buildable_t spawn, vec3_t spawnOrigin );

qboolean          G_isPower( vec3_t origin );
qboolean          G_isDCC( void );
qboolean          G_isOvermind( void );

void              G_BuildableThink( gentity_t *ent, int msec );
qboolean          G_BuildableRange( vec3_t origin, float r, buildable_t buildable );
itemBuildError_t  G_itemFits( gentity_t *ent, buildable_t buildable, int distance, vec3_t origin );
gentity_t         *G_buildItem( gentity_t *builder, buildable_t buildable, vec3_t origin, vec3_t angles );
qboolean          G_ValidateBuild( gentity_t *ent, buildable_t buildable );
void              G_setBuildableAnim( gentity_t *ent, buildableAnimNumber_t anim, qboolean force );
void              G_setIdleBuildableAnim( gentity_t *ent, buildableAnimNumber_t anim );
void              G_SpawnBuildable(gentity_t *ent, buildable_t buildable);
void              FinishSpawningBuildable( gentity_t *ent );

//
// g_utils.c
//
int         G_ParticleSystemIndex( char *name );
int         G_ShaderIndex( char *name );
int         G_ModelIndex( char *name );
int         G_SoundIndex( char *name );
void        G_TeamCommand( pTeam_t team, char *cmd );
void        G_KillBox (gentity_t *ent);
gentity_t   *G_Find (gentity_t *from, int fieldofs, const char *match);
gentity_t   *G_PickTarget (char *targetname);
void        G_UseTargets (gentity_t *ent, gentity_t *activator);
void        G_SetMovedir ( vec3_t angles, vec3_t movedir);

void        G_InitGentity( gentity_t *e );
gentity_t   *G_Spawn( void );
gentity_t   *G_TempEntity( vec3_t origin, int event );
void        G_Sound( gentity_t *ent, int channel, int soundIndex );
void        G_FreeEntity( gentity_t *e );
qboolean    G_EntitiesFree( void );

void        G_TouchTriggers( gentity_t *ent );
void        G_TouchSolids( gentity_t *ent );

float       *tv( float x, float y, float z );
char        *vtos( const vec3_t v );

float       vectoyaw( const vec3_t vec );

void        G_AddPredictableEvent( gentity_t *ent, int event, int eventParm );
void        G_AddEvent( gentity_t *ent, int event, int eventParm );
void        G_BroadcastEvent( int event, int eventParm );
void        G_SetOrigin( gentity_t *ent, vec3_t origin );
void        AddRemap(const char *oldShader, const char *newShader, float timeOffset);
const char  *BuildShaderStateConfig( void );


#define MAX_QUEUE_COMMANDS  64 //should be MAX_RELIABLE_COMMANDS/server.h

typedef struct commandQueueElement_s
{
  qboolean                      used;
  struct commandQueueElement_s  *next;
  char                          command[ MAX_TOKEN_CHARS ];
} commandQueueElement_t;

typedef struct commandQueue_s
{
  int                   nextCommandTime; //next time that the queue can be popped

  int                   numElements;
  commandQueueElement_t *front;
  commandQueueElement_t *back;

  commandQueueElement_t pool[ MAX_QUEUE_COMMANDS ];
} commandQueue_t;

qboolean    G_ClientIsLagging( gclient_t *client );
void        G_ProcessCommandQueues( void );
void        G_SendCommandFromServer( int clientNum, const char *cmd );
void        G_InitCommandQueue( int clientNum );

void        G_TriggerMenu( int clientNum, dynMenu_t menu );
void        G_CloseMenus( int clientNum );

qboolean    G_Visible( gentity_t *ent1, gentity_t *ent2 );
gentity_t   *G_ClosestEnt( vec3_t origin, gentity_t **entities, int numEntities );

//
// g_combat.c
//
qboolean  CanDamage( gentity_t *targ, vec3_t origin );
void      G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
                    vec3_t dir, vec3_t point, int damage, int dflags, int mod );
void      G_SelectiveDamage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir,
                             vec3_t point, int damage, int dflags, int mod, int team );
qboolean  G_RadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius,
                          gentity_t *ignore, int mod );
qboolean  G_SelectiveRadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius,
                                   gentity_t *ignore, int mod, int team );
void      body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath );
void      AddScore( gentity_t *ent, int score );

//TA:
void      G_InitDamageLocations( void );

// damage flags
#define DAMAGE_RADIUS         0x00000001  // damage was indirect
#define DAMAGE_NO_ARMOR       0x00000002  // armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK   0x00000004  // do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION  0x00000008  // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_NO_LOCDAMAGE   0x00000010  // do not apply locational damage

//
// g_missile.c
//
void      G_RunMissile( gentity_t *ent );

gentity_t *fire_flamer( gentity_t *self, vec3_t start, vec3_t aimdir );
gentity_t *fire_blaster( gentity_t *self, vec3_t start, vec3_t dir );
gentity_t *fire_pulseRifle( gentity_t *self, vec3_t start, vec3_t dir );
gentity_t *fire_luciferCannon( gentity_t *self, vec3_t start, vec3_t dir, int damage, int radius );
gentity_t *fire_lockblob( gentity_t *self, vec3_t start, vec3_t dir );
gentity_t *fire_paraLockBlob( gentity_t *self, vec3_t start, vec3_t dir );
gentity_t *fire_slowBlob( gentity_t *self, vec3_t start, vec3_t dir );
gentity_t *fire_bounceBall( gentity_t *self, vec3_t start, vec3_t dir );
gentity_t *fire_hive( gentity_t *self, vec3_t start, vec3_t dir );
gentity_t *launch_grenade( gentity_t *self, vec3_t start, vec3_t dir );


//
// g_mover.c
//
void G_RunMover( gentity_t *ent );
void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace );
void manualTriggerSpectator( gentity_t *trigger, gentity_t *player );

//
// g_trigger.c
//
void trigger_teleporter_touch( gentity_t *self, gentity_t *other, trace_t *trace );
void G_Checktrigger_stages( pTeam_t team, stage_t stage );


//
// g_misc.c
//
void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles );
void ShineTorch( gentity_t *self );

//
// g_weapon.c
//

#define MAX_ZAP_TARGETS LEVEL2_AREAZAP_MAX_TARGETS

typedef struct zap_s
{
  qboolean      used;

  gentity_t     *creator;
  gentity_t     *targets[ MAX_ZAP_TARGETS ];
  int           numTargets;

  int           timeToLive;

  gentity_t     *effectChannel;
} zap_t;

void      G_ForceWeaponChange( gentity_t *ent, weapon_t weapon );
void      G_GiveClientMaxAmmo( gentity_t *ent, qboolean buyingEnergyAmmo );
void      CalcMuzzlePoint( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint );
void      SnapVectorTowards( vec3_t v, vec3_t to );
qboolean  CheckVenomAttack( gentity_t *ent );
void      CheckGrabAttack( gentity_t *ent );
qboolean  CheckPounceAttack( gentity_t *ent );
void      ChargeAttack( gentity_t *ent, gentity_t *victim );
void      G_UpdateZaps( int msec );


//
// g_client.c
//
void      G_AddCreditToClient( gclient_t *client, short credit, qboolean cap );
team_t    TeamCount( int ignoreClientNum, int team );
void      SetClientViewAngle( gentity_t *ent, vec3_t angle );
gentity_t *SelectTremulousSpawnPoint( pTeam_t team, vec3_t preference, vec3_t origin, vec3_t angles );
gentity_t *SelectSpawnPoint( vec3_t avoidPoint, vec3_t origin, vec3_t angles );
void      SpawnCorpse( gentity_t *ent );
void      respawn( gentity_t *ent );
void      BeginIntermission( void );
void      ClientSpawn( gentity_t *ent, gentity_t *spawn, vec3_t origin, vec3_t angles );
void      player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod );
qboolean  SpotWouldTelefrag( gentity_t *spot );

//
// g_svcmds.c
//
qboolean  ConsoleCommand( void );
void      G_ProcessIPBans( void );
qboolean  G_FilterPacket( char *from );

//
// g_weapon.c
//
void FireWeapon( gentity_t *ent );
void FireWeapon2( gentity_t *ent );
void FireWeapon3( gentity_t *ent );

//
// g_cmds.c
//

//
// g_main.c
//
void ScoreboardMessage( gentity_t *client );
void MoveClientToIntermission( gentity_t *client );
void CalculateRanks( void );
void FindIntermissionPoint( void );
void G_RunThink( gentity_t *ent );
void QDECL G_LogPrintf( const char *fmt, ... ) __attribute__((format(printf, 1, 2)));
void SendScoreboardMessageToAllClients( void );
void QDECL G_Printf( const char *fmt, ... ) __attribute__((format(printf, 1, 2)));
void QDECL G_Error( const char *fmt, ... ) __attribute__((format(printf, 1, 2)));

//
// g_client.c
//
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot );
void ClientUserinfoChanged( int clientNum );
void ClientDisconnect( int clientNum );
void ClientBegin( int clientNum );
void ClientCommand( int clientNum );

//
// g_active.c
//
void ClientThink( int clientNum );
void ClientEndFrame( gentity_t *ent );
void G_RunClient( gentity_t *ent );

//
// g_team.c
//
qboolean  OnSameTeam( gentity_t *ent1, gentity_t *ent2 );
gentity_t *Team_GetLocation( gentity_t *ent );
qboolean  Team_GetLocationMsg( gentity_t *ent, char *loc, int loclen );
void      TeamplayInfoMessage( gentity_t *ent );
void      CheckTeamStatus( void );

//
// g_mem.c
//
void *G_Alloc( int size );
void G_InitMemory( void );
void G_Free( void *ptr );
void G_DefragmentMemory( void );
void Svcmd_GameMem_f( void );

//
// g_session.c
//
void G_ReadSessionData( gclient_t *client );
void G_InitSessionData( gclient_t *client, char *userinfo );
void G_WriteSessionData( void );

//
// g_maprotation.c
//
#define MAX_MAP_ROTATIONS       16
#define MAX_MAP_ROTATION_MAPS   64
#define MAX_MAP_COMMANDS        16
#define MAX_MAP_ROTATION_CONDS  4

#define NOT_ROTATING          -1

typedef enum
{
  MCV_ERR,
  MCV_RANDOM,
  MCV_NUMCLIENTS,
  MCV_LASTWIN
} mapConditionVariable_t;

typedef enum
{
  MCO_LT,
  MCO_EQ,
  MCO_GT
} mapConditionOperator_t;

typedef enum
{
  MCT_ERR,
  MCT_MAP,
  MCT_ROTATION
} mapConditionType_t;

typedef struct mapRotationCondition_s
{
  char                    dest[ MAX_QPATH ];

  qboolean                unconditional;

  mapConditionVariable_t  lhs;
  mapConditionOperator_t  op;

  int                     numClients;
  pTeam_t                 lastWin;
} mapRotationCondition_t;

typedef struct mapRotationEntry_s
{
  char                    name[ MAX_QPATH ];

  char                    postCmds[ MAX_QPATH ][ MAX_MAP_COMMANDS ];
  int                     numCmds;

  mapRotationCondition_t  conditions[ MAX_MAP_ROTATION_CONDS ];
  int                     numConditions;
} mapRotationEntry_t;

typedef struct mapRotation_s
{
  char                name[ MAX_QPATH ];

  mapRotationEntry_t  maps[ MAX_MAP_ROTATION_MAPS ];
  int                 numMaps;
  int                 currentMap;
} mapRotation_t;

typedef struct mapRotations_s
{
  mapRotation_t   rotations[ MAX_MAP_ROTATIONS ];
  int             numRotations;
} mapRotations_t;

void      G_PrintRotations( void );
qboolean  G_AdvanceMapRotation( void );
qboolean  G_StartMapRotation( char *name, qboolean changeMap );
void      G_StopMapRotation( void );
qboolean  G_MapRotationActive( void );
void      G_InitMapRotations( void );

//
// g_ptr.c
//
void                G_UpdatePTRConnection( gclient_t *client );
connectionRecord_t  *G_GenerateNewConnection( gclient_t *client );
qboolean            G_VerifyPTRC( int code );
void                G_ResetPTRConnections( void );
connectionRecord_t  *G_FindConnectionForCode( int code );
void                G_DeletePTRConnection( connectionRecord_t *connection );


//some maxs
#define MAX_FILEPATH      144

extern  level_locals_t  level;
extern  gentity_t       g_entities[ MAX_GENTITIES ];

#define FOFS(x) ((intptr_t)&(((gentity_t *)0)->x))

extern  vmCvar_t  g_dedicated;
extern  vmCvar_t  g_cheats;
extern  vmCvar_t  g_maxclients;     // allow this many total, including spectators
extern  vmCvar_t  g_maxGameClients;   // allow this many active
extern  vmCvar_t  g_restarted;
extern  vmCvar_t  g_minCommandPeriod;

extern  vmCvar_t  g_timelimit;
extern  vmCvar_t  g_suddenDeathTime;
extern  vmCvar_t  g_friendlyFire;
extern  vmCvar_t  g_password;
extern  vmCvar_t  g_needpass;
extern  vmCvar_t  g_gravity;
extern  vmCvar_t  g_speed;
extern  vmCvar_t  g_knockback;
extern  vmCvar_t  g_quadfactor;
extern  vmCvar_t  g_forcerespawn;
extern  vmCvar_t  g_inactivity;
extern  vmCvar_t  g_debugMove;
extern  vmCvar_t  g_debugAlloc;
extern  vmCvar_t  g_debugDamage;
extern  vmCvar_t  g_weaponRespawn;
extern  vmCvar_t  g_weaponTeamRespawn;
extern  vmCvar_t  g_synchronousClients;
extern  vmCvar_t  g_motd;
extern  vmCvar_t  g_warmup;
extern  vmCvar_t  g_doWarmup;
extern  vmCvar_t  g_blood;
extern  vmCvar_t  g_allowVote;
extern  vmCvar_t  g_teamAutoJoin;
extern  vmCvar_t  g_teamForceBalance;
extern  vmCvar_t  g_banIPs;
extern  vmCvar_t  g_filterBan;
extern  vmCvar_t  g_smoothClients;
extern  vmCvar_t  pmove_fixed;
extern  vmCvar_t  pmove_msec;
extern  vmCvar_t  g_rankings;
extern  vmCvar_t  g_enableDust;
extern  vmCvar_t  g_enableBreath;
extern  vmCvar_t  g_singlePlayer;

extern  vmCvar_t  g_humanBuildPoints;
extern  vmCvar_t  g_alienBuildPoints;
extern  vmCvar_t  g_humanStage;
extern  vmCvar_t  g_humanKills;
extern  vmCvar_t  g_humanMaxStage;
extern  vmCvar_t  g_humanStage2Threshold;
extern  vmCvar_t  g_humanStage3Threshold;
extern  vmCvar_t  g_alienStage;
extern  vmCvar_t  g_alienKills;
extern  vmCvar_t  g_alienMaxStage;
extern  vmCvar_t  g_alienStage2Threshold;
extern  vmCvar_t  g_alienStage3Threshold;

extern  vmCvar_t  g_disabledEquipment;
extern  vmCvar_t  g_disabledClasses;
extern  vmCvar_t  g_disabledBuildables;

extern  vmCvar_t  g_debugMapRotation;
extern  vmCvar_t  g_currentMapRotation;
extern  vmCvar_t  g_currentMap;
extern  vmCvar_t  g_initialMapRotation;

void 			trap_Print(const char *fmt);
void 			trap_Error(const char *fmt);
int 			trap_Milliseconds(void);
void 			trap_Cvar_Register(vmCvar_t * cvar, const char *var_name, const char *value, int flags);
void            trap_Cvar_Set(const char *var_name, const char *value);
void 			trap_Cvar_Update(vmCvar_t * cvar);
int 			trap_Cvar_VariableIntegerValue(const char *var_name);
void 			trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);
void 			trap_Cvar_LatchedVariableStringBuffer(const char *var_name, char *buffer, int bufsize);
int 			trap_Argc(void);
void 			trap_Argv(int n, char *buffer, int bufferLength);
void 			trap_SendConsoleCommand(int exec_when, const char *text);
int 			trap_FS_FOpenFile(const char *qpath, fileHandle_t * f, fsMode_t mode);
void 			trap_FS_Read(void *buffer, int len, fileHandle_t f);
int 			trap_FS_Write(const void *buffer, int len, fileHandle_t f);
int 			trap_FS_Rename(const char *from, const char *to);
void 			trap_FS_FCloseFile(fileHandle_t f);
int 			trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
void 			trap_LocateGameData(gentity_t * gEnts, int numGEntities, int sizeofGEntity_t, playerState_t * clients, int sizeofGClient);
void 			trap_DropClient(int clientNum, const char *reason, int length);
void 			trap_SendServerCommand(int clientNum, const char *text);
void            trap_SetConfigstring(int num, const char *string);
void 			trap_LinkEntity(gentity_t * ent);
void 			trap_UnlinkEntity(gentity_t * ent);
int 			trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxcount);
qboolean		trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t * ent);
qboolean		trap_EntityContactCapsule(const vec3_t mins, const vec3_t maxs, const gentity_t * ent);
void 			trap_Trace(trace_t * results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
void            trap_TraceNoEnts( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
void 			trap_TraceCapsule(trace_t * results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
void            trap_TraceCapsuleNoEnts( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
int 			trap_PointContents(const vec3_t point, int passEntityNum);
void 			trap_SetBrushModel(gentity_t * ent, const char *name);
qboolean 		trap_InPVS(const vec3_t p1, const vec3_t p2);
qboolean		trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2);
void 			trap_SetConfigstringRestrictions( int num, const clientList_t *clientList );
void 			trap_GetConfigstring(int num, char *buffer, int bufferSize);
void 			trap_SetConfigstringRestrictions( int num, const clientList_t *clientList );
void 			trap_SetUserinfo(int num, const char *buffer);
void 			trap_GetUserinfo(int num, char *buffer, int bufferSize);
void 			trap_GetServerinfo(char *buffer, int bufferSize);
void 			trap_AdjustAreaPortalState(gentity_t * ent, qboolean open);
qboolean		trap_AreasConnected(int area1, int area2);
void 			trap_UpdateSharedConfig( unsigned int port, const char *rconpass );
int 			trap_BotAllocateClient(int clientNum);
void 			trap_BotFreeClient( int clientNum );
void 			trap_GetUsercmd(int clientNum, usercmd_t * cmd);
qboolean		trap_GetEntityToken(char *buffer, int bufferSize);
int 			trap_DebugPolygonCreate(int color, int numPoints, vec3_t * points);
void 			trap_DebugPolygonDelete(int id);
int 			trap_RealTime(qtime_t * qtime);
void 			trap_SnapVector(float *v);
void 			trap_SendGameStat( const char *data );
void 			trap_AddCommand( const char *cmdName );
void 			trap_RemoveCommand( const char *cmdName );
qboolean		trap_GetTag(int clientNum, int tagFileNumber, char *tagName, orientation_t * ori);
qboolean		trap_LoadTag(const char *filename);
sfxHandle_t		trap_RegisterSound(const char *sample, qboolean compressed);
int 			trap_GetSoundLength(sfxHandle_t sfxHandle);
int 			trap_Parse_AddGlobalDefine( char *define );
int 			trap_Parse_LoadSource( const char *filename );
int 			trap_Parse_FreeSource( int handle );
int 			trap_Parse_ReadToken( int handle, pc_token_t *pc_token );
int 			trap_Parse_SourceFileAndLine( int handle, char *filename, int *line );
int 			trap_BotLibSetup( void );
int 			trap_BotLibVarSet(char *var_name, char *value);
int 			trap_BotLibVarSet(char *var_name, char *value);
int 			trap_BotLibVarGet(char *var_name, char *value, int size);
int 			trap_BotLibDefine(char *string);
int 			trap_PC_LoadSource( const char *filename );
int 			trap_PC_FreeSource( int handle );
int 			trap_PC_ReadToken( int handle, pc_token_t *pc_token );
int 			trap_PC_SourceFileAndLine( int handle, char *filename, int *line );
int 			trap_PC_UnReadToken( int handle );
int 			trap_BotLibStartFrame(float time);
int 			trap_BotLibLoadMap(const char *mapname);
int 			trap_BotLibUpdateEntity(int ent, void /* struct bot_updateentity_s */ *bue);
int 			trap_BotLibTest(int parm0, char *parm1, vec3_t parm2, vec3_t parm3);
int 			trap_BotGetSnapshotEntity( int clientNum, int sequence );
int 			trap_BotGetServerCommand(int clientNum, char *message, int size);
void 			trap_BotUserCommand(int clientNum, usercmd_t *ucmd);
void 			trap_AAS_EntityInfo(int entnum, void /* struct aas_entityinfo_s */ *info);
int 			trap_AAS_Initialized(void);
void 			trap_AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs);
float 			trap_AAS_Time(void);
void 			trap_AAS_SetCurrentWorld(int index);
int 			trap_AAS_PointAreaNum(vec3_t point);
int 			trap_AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas);
int 			trap_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas);
void 			trap_AAS_AreaCenter(int areanum, vec3_t center);
qboolean 		trap_AAS_AreaWaypoint(int areanum, vec3_t center);
int 			trap_AAS_PointContents(vec3_t point);
int 			trap_AAS_NextBSPEntity(int ent);
int 			trap_AAS_ValueForBSPEpairKey(int ent, char *key, char *value, int size);
int 			trap_AAS_VectorForBSPEpairKey(int ent, char *key, vec3_t v);
int 			trap_AAS_FloatForBSPEpairKey(int ent, char *key, float *value);
int 			trap_AAS_IntForBSPEpairKey(int ent, char *key, int *value);
int 			trap_AAS_AreaReachability(int areanum);
int 			trap_AAS_AreaLadder(int areanum);
int 			trap_AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags);
int 			trap_AAS_Swimming(vec3_t origin);
int 			trap_AAS_PredictClientMovement(void /* struct aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize);
void 			trap_AAS_RT_ShowRoute( vec3_t srcpos, int	srcnum, int destnum );
int 			trap_AAS_NearestHideArea(int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum, int travelflags, float maxdist, vec3_t distpos);
int 			trap_AAS_ListAreasInRange(vec3_t srcpos, int srcarea, float range, int travelflags, float **outareas, int maxareas);
int 			trap_AAS_AvoidDangerArea(vec3_t srcpos, int srcarea, vec3_t dangerpos, int dangerarea, float range, int travelflags);
int 			trap_AAS_Retreat(int *dangerSpots, int dangerSpotCount, vec3_t srcpos, int srcarea, vec3_t dangerpos, int dangerarea, float range, float dangerRange, int travelflags);
void 			trap_AAS_SetAASBlockingEntity( vec3_t absmin, vec3_t absmax, int blocking );
void 			trap_AAS_RecordTeamDeathArea( vec3_t srcpos, int srcarea, int team, int teamCount, int travelflags );
void 			trap_EA_Say(int client, char *str);
void 			trap_EA_SayTeam(int client, char *str);
void 			trap_EA_UseItem(int client, char *it);
void 			trap_EA_DropItem(int client, char *it);
void 			trap_EA_UseInv(int client, char *inv);
void 			trap_EA_DropInv(int client, char *inv);
void 			trap_EA_Gesture(int client);
void 			trap_EA_Command(int client, char *command);
void 			trap_EA_SelectWeapon(int client, int weapon);
void 			trap_EA_Talk(int client);
void 			trap_EA_Attack(int client);
void 			trap_EA_Reload(int client);
void 			trap_EA_Activate(int client);
void 			trap_EA_Respawn(int client);
void 			trap_EA_Jump(int client);
void 			trap_EA_DelayedJump(int client);
void 			trap_EA_Crouch(int client);
void 			trap_EA_Walk(int client);
void 			trap_EA_MoveUp(int client);
void 			trap_EA_MoveDown(int client);
void 			trap_EA_MoveForward(int client);
void 			trap_EA_MoveBack(int client);
void 			trap_EA_MoveLeft(int client);
void 			trap_EA_MoveRight(int client);
void			trap_EA_Move(int client, vec3_t dir, float speed);
void			trap_EA_View(int client, vec3_t viewangles);
void			trap_EA_Prone(int client);
void			trap_EA_EndRegular(int client, float thinktime);
void			trap_EA_GetInput(int client, float thinktime, void /* struct bot_input_s */ *input);
void			trap_EA_ResetInput(int client, void *init);
int				trap_BotLoadCharacter(char *charfile, int skill);
void			trap_BotFreeCharacter(int character);
float			trap_Characteristic_Float(int character, int index);
float			trap_Characteristic_BFloat(int character, int index, float min, float max);
int				trap_Characteristic_Integer(int character, int index);
int				trap_Characteristic_BInteger(int character, int index, int min, int max);
void			trap_Characteristic_String(int character, int index, char *buf, int size);
int				trap_BotAllocChatState(void);
void			trap_BotFreeChatState(int handle);
void			trap_BotQueueConsoleMessage(int chatstate, int type, char *message);
void			trap_BotRemoveConsoleMessage(int chatstate, int handle);
int				trap_BotNextConsoleMessage(int chatstate, void /* struct bot_consolemessage_s */ *cm);
int				trap_BotNumConsoleMessages(int chatstate);
void			trap_BotInitialChat(int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
int				trap_BotNumInitialChats(int chatstate, char *type);
int				trap_BotReplyChat(int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
int				trap_BotChatLength(int chatstate);
void			trap_BotEnterChat(int chatstate, int client, int sendto);
void			trap_BotGetChatMessage(int chatstate, char *buf, int size);
int				trap_BotFindMatch(char *str, void /* struct bot_match_s */ *match, unsigned long int context);
void			trap_BotMatchVariable(void /* struct bot_match_s */ *match, int variable, char *buf, int size);
void			trap_UnifyWhiteSpaces(char *string);
void			trap_BotReplaceSynonyms(char *string, unsigned long int context);
int				trap_BotLoadChatFile(int chatstate, char *chatfile, char *chatname);
void			trap_BotSetChatGender(int chatstate, int gender);
void			trap_BotSetChatName(int chatstate, char *name);
void			trap_BotResetGoalState(int goalstate);
void			trap_BotResetAvoidGoals(int goalstate);
void			trap_BotRemoveFromAvoidGoals(int goalstate, int number);
void			trap_BotPushGoal(int goalstate, void /* struct bot_goal_s */ *goal);
void			trap_BotPopGoal(int goalstate);
void			trap_BotEmptyGoalStack(int goalstate);
void			trap_BotDumpAvoidGoals(int goalstate);
void			trap_BotDumpGoalStack(int goalstate);
void			trap_BotGoalName(int number, char *name, int size);
int				trap_BotGetTopGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int				trap_BotGetSecondGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int				trap_BotChooseLTGItem(int goalstate, vec3_t origin, int *inventory, int travelflags);
int				trap_BotChooseNBGItem(int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime);
int				trap_BotTouchingGoal(vec3_t origin, void /* struct bot_goal_s */ *goal);
int				trap_BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal);
int				trap_BotGetLevelItemGoal(int index, char *classname, void /* struct bot_goal_s */ *goal);
int				trap_BotGetNextCampSpotGoal(int num, void /* struct bot_goal_s */ *goal);
int				trap_BotGetMapLocationGoal(char *name, void /* struct bot_goal_s */ *goal);
float			trap_BotAvoidGoalTime(int goalstate, int number);
void			trap_BotInitLevelItems(void);
void			trap_BotUpdateEntityItems(void);
int				trap_BotLoadItemWeights(int goalstate, char *filename);
void			trap_BotFreeItemWeights(int goalstate);
void			trap_BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child);
void			trap_BotSaveGoalFuzzyLogic(int goalstate, char *filename);
void			trap_BotMutateGoalFuzzyLogic(int goalstate, float range);
int				trap_BotAllocGoalState(int state);
void			trap_BotFreeGoalState(int handle);
void			trap_BotResetMoveState(int movestate);
void			trap_BotMoveToGoal(void /* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal, int travelflags);
int				trap_BotMoveInDirection(int movestate, vec3_t dir, float speed, int type);
void			trap_BotResetAvoidReach(int movestate);
void			trap_BotResetLastAvoidReach(int movestate);
int				trap_BotReachabilityArea(vec3_t origin, int testground);
int				trap_BotMovementViewTarget(int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead, vec3_t target);
int				trap_BotPredictVisiblePosition(vec3_t origin, int areanum, void /* struct bot_goal_s */ *goal, int travelflags, vec3_t target);
int				trap_BotAllocMoveState(void);
void			trap_BotFreeMoveState(int handle);
void			trap_BotInitMoveState(int handle, void /* struct bot_initmove_s */ *initmove);
void			trap_BotInitAvoidReach(int handle);
int				trap_BotChooseBestFightWeapon(int weaponstate, int *inventory);
void			trap_BotGetWeaponInfo(int weaponstate, int weapon, void /* struct weaponinfo_s */ *weaponinfo);
int				trap_BotLoadWeaponWeights(int weaponstate, char *filename);
int				trap_BotAllocWeaponState(void);
void			trap_BotFreeWeaponState(int weaponstate);
void			trap_BotResetWeaponState(int weaponstate);
int				trap_GeneticParentsAndChildSelection(int numranks, float *ranks, int *parent1, int *parent2, int *child);
void			trap_AddPhysicsEntity(gentity_t * ent);
void			trap_AddPhysicsStatic(gentity_t * ent);
void			trap_SendMessage(int clientNum, char *buf, int buflen);
messageStatus_t trap_MessageStatus(int clientNum);
#if defined(ET_MYSQL)
int				trap_SQL_RunQuery( const char *query );
void			trap_SQL_FinishQuery( int queryid );
qboolean		trap_SQL_NextRow( int queryid );
int				trap_SQL_RowCount( int queryid );
void			trap_SQL_GetFieldbyID( int queryid, int fieldid, char *buffer, int len );
void			trap_SQL_GetFieldbyName( int queryid, const char *name, char *buffer, int len );
int				trap_SQL_GetFieldbyID_int( int queryid, int fieldid );
int				trap_SQL_GetFieldbyName_int( int queryid, const char *name );
int				trap_SQL_FieldCount( int queryid );
void			trap_SQL_CleanString( const char *in, char *out, int len );
#endif
