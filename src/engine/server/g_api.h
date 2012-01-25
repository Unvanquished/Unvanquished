/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

OpenWolf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

OpenWolf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "../qcommon/q_shared.h"

#define GAME_API_VERSION 9

#define SVF_NOCLIENT                  0x00000001
#define SVF_CLIENTMASK                0x00000002
#define SVF_VISDUMMY                  0x00000004
#define SVF_BOT                       0x00000008
#define SVF_POW                       0x00000010 // ignored by the engine
#define SVF_BROADCAST                 0x00000020
#define SVF_PORTAL                    0x00000040
#define SVF_BLANK                     0x00000080 // ignored by the engine
#define SVF_NOFOOTSTEPS               0x00000100 // ignored by the engine
#define SVF_CAPSULE                   0x00000200
#define SVF_VISDUMMY_MULTIPLE         0x00000400
#define SVF_SINGLECLIENT              0x00000800
#define SVF_NOSERVERINFO              0x00001000 // only meaningful for entities numbered in [0..MAX_CLIENTS)
#define SVF_NOTSINGLECLIENT           0x00002000
#define SVF_IGNOREBMODELEXTENTS       0x00004000
#define SVF_SELF_PORTAL               0x00008000
#define SVF_SELF_PORTAL_EXCLUSIVE     0x00010000
#define SVF_RIGID_BODY                0x00020000 // ignored by the engine
#define SVF_USE_CURRENT_ORIGIN        0x00040000 // ignored by the engine

typedef struct {
	qboolean linked;         // qfalse if not in any good cluster
	int      linkcount;

	int      svFlags;        // SVF_NOCLIENT, SVF_BROADCAST, etc.
	int      singleClient;   // only send to this client when SVF_SINGLECLIENT is set
	int      hiMask, loMask; // if SVF_CLIENTMASK is set, then only send to the
	                         //  clients specified by the following 64-bit bitmask:
	                         //  hiMask: high-order bits (32..63)
	                         //  loMask: low-order bits (0..31)

	qboolean bmodel;         // if false, assume an explicit mins/maxs bounding box
	                         // only set by trap_SetBrushModel
	vec3_t   mins, maxs;
	int      contents;       // CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc.
	                         // a non-solid entity should have this set to 0

	vec3_t   absmin, absmax; // derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vec3_t   currentOrigin;
	vec3_t   currentAngles;

	// when a trace call is made and the specified pass entity isn't none,
	//  then a given entity will be excluded from testing if:
	// - the given entity is the pass entity (use case: don't interact with self),
	// - the owner of the given entity is the pass entity (use case: don't interact with your own missiles), or
	// - the given entity and the pass entity have the same owner entity (that is not none)
	//    (use case: don't interact with other missiles from owner).
	// that is, ent will be excluded if
	// ( passEntityNum != ENTITYNUM_NONE &&
	//   ( ent->s.number == passEntityNum || ent->r.ownerNum == passEntityNum ||
	//     ( ent->r.ownerNum != ENTITYNUM_NONE && ent->r.ownerNum == entities[passEntityNum].r.ownerNum ) ) )
	int      ownerNum;
	int      eventTime;

	int      worldflags;

	qboolean snapshotCallback;
} entityShared_t;

// the server looks at a sharedEntity_t structure, which must be at the start of a gentity_t structure
typedef struct
{
	entityState_t  s; // communicated by the server to clients
	entityShared_t r; // shared by both the server and game module
} sharedEntity_t;

// game-module-to-engine calls
typedef enum {
	G_PRINT,
	G_ERROR,
	G_MILLISECONDS,
	G_CVAR_REGISTER,
	G_CVAR_UPDATE,
	G_CVAR_SET,
	G_CVAR_VARIABLE_INTEGER_VALUE,
	G_CVAR_VARIABLE_STRING_BUFFER,
	G_CVAR_LATCHEDVARIABLESTRINGBUFFER,
	G_ARGC,
	G_ARGV,
	G_SEND_CONSOLE_COMMAND,
	G_FS_FOPEN_FILE,
	G_FS_READ,
	G_FS_WRITE,
	G_FS_RENAME,
	G_FS_FCLOSE_FILE,
	G_FS_GETFILELIST,
	G_LOCATE_GAME_DATA,
	G_DROP_CLIENT,
	G_SEND_SERVER_COMMAND,
	G_LINKENTITY,
	G_UNLINKENTITY,
	G_ENTITIES_IN_BOX,
	G_ENTITY_CONTACT,
	G_ENTITY_CONTACTCAPSULE,
	G_TRACE,
	G_TRACECAPSULE,
	G_POINT_CONTENTS,
	G_SET_BRUSH_MODEL,
	G_IN_PVS,
	G_IN_PVS_IGNORE_PORTALS,
	G_SET_CONFIGSTRING,
	G_GET_CONFIGSTRING,
	G_SET_CONFIGSTRING_RESTRICTIONS,
	G_SET_USERINFO,
	G_GET_USERINFO,
	G_GET_SERVERINFO,
	G_ADJUST_AREA_PORTAL_STATE,
	G_AREAS_CONNECTED,
	G_UPDATE_SHARED_CONFIG,
	G_BOT_ALLOCATE_CLIENT,
	G_BOT_FREE_CLIENT,
	G_GET_USERCMD,
	G_GET_ENTITY_TOKEN,
	G_DEBUG_POLYGON_CREATE,
	G_DEBUG_POLYGON_DELETE,
	G_REAL_TIME,
	G_SNAPVECTOR,
	G_SEND_GAMESTAT,
	G_ADDCOMMAND,
	G_REMOVECOMMAND,
	G_GETTAG,
	G_REGISTERTAG,
	G_REGISTERSOUND,
	G_GET_SOUND_LENGTH,
	G_PARSE_ADD_GLOBAL_DEFINE,
	G_PARSE_LOAD_SOURCE,
	G_PARSE_FREE_SOURCE,
	G_PARSE_READ_TOKEN,
	G_PARSE_SOURCE_FILE_AND_LINE,
	BOTLIB_SETUP,
	BOTLIB_SHUTDOWN,
	BOTLIB_LIBVAR_SET,
	BOTLIB_LIBVAR_GET,
	BOTLIB_PC_ADD_GLOBAL_DEFINE,
	BOTLIB_PC_LOAD_SOURCE,
	BOTLIB_PC_FREE_SOURCE,
	BOTLIB_PC_READ_TOKEN,
	BOTLIB_PC_SOURCE_FILE_AND_LINE,
	BOTLIB_PC_UNREAD_TOKEN,
	BOTLIB_START_FRAME,
	BOTLIB_LOAD_MAP,
	BOTLIB_UPDATENTITY,
	BOTLIB_TEST,
	BOTLIB_GET_SNAPSHOT_ENTITY,
	BOTLIB_GET_CONSOLE_MESSAGE,
	BOTLIB_USER_COMMAND,
	BOTLIB_AAS_ENTITY_INFO,
	BOTLIB_AAS_INITIALIZED,
	BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX,
	BOTLIB_AAS_TIME,
	BOTLIB_AAS_SETCURRENTWORLD,
	BOTLIB_AAS_POINT_AREA_NUM,
	BOTLIB_AAS_TRACE_AREAS,
	BOTLIB_AAS_BBOX_AREAS,
	BOTLIB_AAS_AREA_CENTER,
	BOTLIB_AAS_AREA_WAYPOINT,
	BOTLIB_AAS_POINT_CONTENTS,
	BOTLIB_AAS_NEXT_BSP_ENTITY,
	BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY,
	BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY,
	BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY,
	BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY,
	BOTLIB_AAS_AREA_REACHABILITY,
	BOTLIB_AAS_AREA_LADDER,
	BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA,
	BOTLIB_AAS_SWIMMING,
	BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT,
	BOTLIB_AAS_RT_SHOWROUTE,
	BOTLIB_AAS_NEARESTHIDEAREA,
	BOTLIB_AAS_LISTAREASINRANGE,
	BOTLIB_AAS_AVOIDDANGERAREA,
	BOTLIB_AAS_RETREAT,
	BOTLIB_AAS_ALTROUTEGOALS,
	BOTLIB_AAS_SETAASBLOCKINGENTITY,
	BOTLIB_AAS_RECORDTEAMDEATHAREA,
	BOTLIB_EA_SAY,
	BOTLIB_EA_SAY_TEAM,
	BOTLIB_EA_USE_ITEM,
	BOTLIB_EA_DROP_ITEM,
	BOTLIB_EA_USE_INV,
	BOTLIB_EA_DROP_INV,
	BOTLIB_EA_GESTURE,
	BOTLIB_EA_COMMAND,
	BOTLIB_EA_SELECT_WEAPON,
	BOTLIB_EA_TALK,
	BOTLIB_EA_ATTACK,
	BOTLIB_EA_RELOAD,
	BOTLIB_EA_USE,
	BOTLIB_EA_RESPAWN,
	BOTLIB_EA_JUMP,
	BOTLIB_EA_DELAYED_JUMP,
	BOTLIB_EA_CROUCH,
	BOTLIB_EA_WALK,
	BOTLIB_EA_MOVE_UP,
	BOTLIB_EA_MOVE_DOWN,
	BOTLIB_EA_MOVE_FORWARD,
	BOTLIB_EA_MOVE_BACK,
	BOTLIB_EA_MOVE_LEFT,
	BOTLIB_EA_MOVE_RIGHT,
	BOTLIB_EA_MOVE,
	BOTLIB_EA_VIEW,
	BOTLIB_EA_PRONE,
	BOTLIB_EA_END_REGULAR,
	BOTLIB_EA_GET_INPUT,
	BOTLIB_EA_RESET_INPUT,
	BOTLIB_AI_LOAD_CHARACTER,
	BOTLIB_AI_FREE_CHARACTER,
	BOTLIB_AI_CHARACTERISTIC_FLOAT,
	BOTLIB_AI_CHARACTERISTIC_BFLOAT,
	BOTLIB_AI_CHARACTERISTIC_INTEGER,
	BOTLIB_AI_CHARACTERISTIC_BINTEGER,
	BOTLIB_AI_CHARACTERISTIC_STRING,
	BOTLIB_AI_ALLOC_CHAT_STATE,
	BOTLIB_AI_FREE_CHAT_STATE,
	BOTLIB_AI_QUEUE_CONSOLE_MESSAGE,
	BOTLIB_AI_REMOVE_CONSOLE_MESSAGE,
	BOTLIB_AI_NEXT_CONSOLE_MESSAGE,
	BOTLIB_AI_NUM_CONSOLE_MESSAGE,
	BOTLIB_AI_INITIAL_CHAT,
	BOTLIB_AI_NUM_INITIAL_CHATS,
	BOTLIB_AI_REPLY_CHAT,
	BOTLIB_AI_CHAT_LENGTH,
	BOTLIB_AI_ENTER_CHAT,
	BOTLIB_AI_GET_CHAT_MESSAGE,
	BOTLIB_AI_STRING_CONTAINS,
	BOTLIB_AI_FIND_MATCH,
	BOTLIB_AI_MATCH_VARIABLE,
	BOTLIB_AI_UNIFY_WHITE_SPACES,
	BOTLIB_AI_REPLACE_SYNONYMS,
	BOTLIB_AI_LOAD_CHAT_FILE,
	BOTLIB_AI_SET_CHAT_GENDER,
	BOTLIB_AI_SET_CHAT_NAME,
	BOTLIB_AI_RESET_GOAL_STATE,
	BOTLIB_AI_RESET_AVOID_GOALS,
	BOTLIB_AI_REMOVE_FROM_AVOID_GOALS,
	BOTLIB_AI_PUSH_GOAL,
	BOTLIB_AI_POP_GOAL,
	BOTLIB_AI_EMPTY_GOAL_STACK,
	BOTLIB_AI_DUMP_AVOID_GOALS,
	BOTLIB_AI_DUMP_GOAL_STACK,
	BOTLIB_AI_GOAL_NAME,
	BOTLIB_AI_GET_TOP_GOAL,
	BOTLIB_AI_GET_SECOND_GOAL,
	BOTLIB_AI_CHOOSE_LTG_ITEM,
	BOTLIB_AI_CHOOSE_NBG_ITEM,
	BOTLIB_AI_TOUCHING_GOAL,
	BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE,
	BOTLIB_AI_GET_LEVEL_ITEM_GOAL,
	BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL,
	BOTLIB_AI_GET_MAP_LOCATION_GOAL,
	BOTLIB_AI_AVOID_GOAL_TIME,
	BOTLIB_AI_INIT_LEVEL_ITEMS,
	BOTLIB_AI_UPDATE_ENTITY_ITEMS,
	BOTLIB_AI_LOAD_ITEM_WEIGHTS,
	BOTLIB_AI_FREE_ITEM_WEIGHTS,
	BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC,
	BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC,
	BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC,
	BOTLIB_AI_ALLOC_GOAL_STATE,
	BOTLIB_AI_FREE_GOAL_STATE,
	BOTLIB_AI_RESET_MOVE_STATE,
	BOTLIB_AI_MOVE_TO_GOAL,
	BOTLIB_AI_MOVE_IN_DIRECTION,
	BOTLIB_AI_RESET_AVOID_REACH,
	BOTLIB_AI_RESET_LAST_AVOID_REACH,
	BOTLIB_AI_REACHABILITY_AREA,
	BOTLIB_AI_MOVEMENT_VIEW_TARGET,
	BOTLIB_AI_PREDICT_VISIBLE_POSITION,
	BOTLIB_AI_ALLOC_MOVE_STATE,
	BOTLIB_AI_FREE_MOVE_STATE,
	BOTLIB_AI_INIT_MOVE_STATE,
	BOTLIB_AI_INIT_AVOID_REACH,
	BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON,
	BOTLIB_AI_GET_WEAPON_INFO,
	BOTLIB_AI_LOAD_WEAPON_WEIGHTS,
	BOTLIB_AI_ALLOC_WEAPON_STATE,
	BOTLIB_AI_FREE_WEAPON_STATE,
	BOTLIB_AI_RESET_WEAPON_STATE,
	BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION,
	G_ADD_PHYSICS_ENTITY,
	G_ADD_PHYSICS_STATIC,
	G_SENDMESSAGE,
	G_MESSAGESTATUS,
#ifdef ET_MYSQL
	G_SQL_RUNQUERY,
	G_SQL_FINISHQUERY,
	G_SQL_NEXTROW,
	G_SQL_ROWCOUNT,
	G_SQL_GETFIELDBYID,
	G_SQL_GETFIELDBYNAME,
	G_SQL_GETFIELDBYID_INT,
	G_SQL_GETFIELDBYNAME_INT,
	G_SQL_FIELDCOUNT,
	G_SQL_CLEANSTRING,
#endif
	G_RSA_GENMSG // ( const char *public_key, char *cleartext, char *encrypted )
} gameImport_t;

// engine-to-game-module calls
typedef enum
{
	GAME_INIT, // void ()( int levelTime, int randomSeed, qboolean restart );
	// the first call to the game module

	GAME_SHUTDOWN, // void ()( void );
	// the last call to the game module

	GAME_CLIENT_CONNECT, // const char * ()( int clientNum, qboolean firstTime, qboolean isBot );
	// return NULL if the client is allowed to connect,
	//  otherwise return a text string describing the reason for the denial

	GAME_CLIENT_BEGIN, // void ()( int clientNum );

	GAME_CLIENT_USERINFO_CHANGED, // void ()( int clientNum );

	GAME_CLIENT_DISCONNECT, // void ()( int clientNum );

	GAME_CLIENT_COMMAND, // void ()( int clientNum );

	GAME_CLIENT_THINK, // void ()( int clientNum );

	GAME_RUN_FRAME, // void ()( int levelTime );

	GAME_CONSOLE_COMMAND, // void ()( void );
	// this will be called when a client-to-server command has been
	//  issued that is not recognized as an engine command.
	// the game module can issue trap_Argc() and trap_Argv() calls to get the command and arguments.
	// return qfalse if the game module doesn't recognize the command

	GAME_SNAPSHOT_CALLBACK, // qboolean ()( int entityNum, int clientNum );
	// return qfalse if the entity should not be sent to the client

	BOTAI_START_FRAME, // void ()( int levelTime );

	// Cast AI
	BOT_VISIBLEFROMPOS, // qboolean ()( vec3_t srcOrig, int srcNum, dstOrig, int dstNum, qboolean isDummy );
	BOT_CHECKATTACKATPOS, // qboolean ()( int entityNum, int enemyNum, vec3_t position,
	                      //              qboolean ducking, qboolean allowWorldHit );

	GAME_MESSAGERECEIVED, // void ()( int clientNum, const char *buffer, int bufferSize, int commandTime );
} gameExport_t;
