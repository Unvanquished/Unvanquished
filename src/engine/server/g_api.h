/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "../qcommon/q_shared.h"
#include "../qcommon/vm_traps.h"
#include "../botlib/bot_api.h"

#define GAME_API_VERSION          1

#define SVF_NOCLIENT              0x00000001
#define SVF_CLIENTMASK            0x00000002
#define SVF_VISDUMMY              0x00000004
#define SVF_BOT                   0x00000008
#define SVF_POW                   0x00000010 // ignored by the engine
#define SVF_BROADCAST             0x00000020
#define SVF_PORTAL                0x00000040
#define SVF_BLANK                 0x00000080 // ignored by the engine
#define SVF_NOFOOTSTEPS           0x00000100 // ignored by the engine
#define SVF_CAPSULE               0x00000200
#define SVF_VISDUMMY_MULTIPLE     0x00000400
#define SVF_SINGLECLIENT          0x00000800
#define SVF_NOSERVERINFO          0x00001000 // only meaningful for entities numbered in [0..MAX_CLIENTS)
#define SVF_NOTSINGLECLIENT       0x00002000
#define SVF_IGNOREBMODELEXTENTS   0x00004000
#define SVF_SELF_PORTAL           0x00008000
#define SVF_SELF_PORTAL_EXCLUSIVE 0x00010000
#define SVF_RIGID_BODY            0x00020000 // ignored by the engine
#define SVF_CLIENTS_IN_RANGE      0x00040000 // clients within range

#define MAX_ENT_CLUSTERS  16

typedef struct
{
	qboolean linked; // qfalse if not in any good cluster
	int      linkcount;

	int      svFlags; // SVF_NOCLIENT, SVF_BROADCAST, etc.
	int      singleClient; // only send to this client when SVF_SINGLECLIENT is set
	int      hiMask, loMask; // if SVF_CLIENTMASK is set, then only send to the
	//  clients specified by the following 64-bit bitmask:
	//  hiMask: high-order bits (32..63)
	//  loMask: low-order bits (0..31)
	float    clientRadius;    // if SVF_CLIENTS_IN_RANGE, send to all clients within this range

	qboolean bmodel; // if false, assume an explicit mins/maxs bounding box
	// only set by trap_SetBrushModel
	vec3_t   mins, maxs;
	int      contents; // CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc.
	// a non-solid entity should have this set to 0

	vec3_t absmin, absmax; // derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultaneous collision issues
	vec3_t currentOrigin;
	vec3_t currentAngles;

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

	qboolean snapshotCallback;

	int numClusters; // if -1, use headnode instead
	int clusternums[ MAX_ENT_CLUSTERS ];
	int lastCluster; // if all the clusters don't fit in clusternums
	int originCluster; // Gordon: calced upon linking, for origin only bmodel vis checks
	int areanum, areanum2;
} entityShared_t;

// the server looks at a sharedEntity_t structure, which must be at the start of a gentity_t structure
typedef struct
{
	entityState_t  s; // communicated by the server to clients
	entityShared_t r; // shared by both the server and game module
} sharedEntity_t;

// game-module-to-engine calls
typedef enum gameImport_s
{
  G_PRINT = FIRST_VM_SYSCALL,
  G_ERROR,
  G_LOG,
  G_SEND_CONSOLE_COMMAND,

  G_FS_FOPEN_FILE,
  G_FS_READ,
  G_FS_WRITE,
  G_FS_RENAME,
  G_FS_FCLOSE_FILE,
  G_FS_GET_FILE_LIST,
  G_FS_FIND_PAK,

  G_LOCATE_GAME_DATA1,
  G_LOCATE_GAME_DATA2,

  G_ADJUST_AREA_PORTAL_STATE,

  G_DROP_CLIENT,
  G_SEND_SERVER_COMMAND,
  G_SET_CONFIGSTRING,
  G_GET_CONFIGSTRING,
  G_SET_CONFIGSTRING_RESTRICTIONS,
  G_SET_USERINFO,
  G_GET_USERINFO,
  G_GET_SERVERINFO,
  G_GET_USERCMD,
  G_GET_ENTITY_TOKEN,
  G_SEND_GAME_STAT,
  G_GET_TAG,
  G_REGISTER_TAG,
  G_SEND_MESSAGE,
  G_MESSAGE_STATUS,
  G_RSA_GENMSG, // ( const char *public_key, char *cleartext, char *encrypted )
  G_GEN_FINGERPRINT,
  G_GET_PLAYER_PUBKEY,
  G_GM_TIME,
  G_GET_TIME_STRING,

  G_PARSE_ADD_GLOBAL_DEFINE,
  G_PARSE_LOAD_SOURCE,
  G_PARSE_FREE_SOURCE,
  G_PARSE_READ_TOKEN,
  G_PARSE_SOURCE_FILE_AND_LINE,

  BOT_ALLOCATE_CLIENT,
  BOT_FREE_CLIENT,
  BOT_GET_CONSOLE_MESSAGE,
  BOT_NAV_SETUP,
  BOT_NAV_SHUTDOWN,
  BOT_SET_NAVMESH,
  BOT_FIND_ROUTE,
  BOT_UPDATE_PATH,
  BOT_NAV_RAYCAST,
  BOT_NAV_RANDOMPOINT,
  BOT_NAV_RANDOMPOINTRADIUS,
  BOT_ENABLE_AREA,
  BOT_DISABLE_AREA,
  BOT_ADD_OBSTACLE,
  BOT_REMOVE_OBSTACLE,
  BOT_UPDATE_OBSTACLES
} gameImport_t;

// PrintMsg
typedef IPC::Message<IPC::Id<VM::QVM, G_PRINT>, std::string> PrintMsg;
// ErrorMsg
typedef IPC::Message<IPC::Id<VM::QVM, G_ERROR>, std::string> ErrorMsg;
// LogMsg TODO
// SendConsoleCommandMsg
typedef IPC::Message<IPC::Id<VM::QVM, G_SEND_CONSOLE_COMMAND>, std::string> SendConsoleCommandMsg;

// FSFOpenFileMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, G_FS_FOPEN_FILE>, std::string, bool, int>,
	IPC::Reply<int, int>
> FSFOpenFileMsg;
// FSReadMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, G_FS_READ>, int, int>,
	IPC::Reply<std::string>
> FSReadMsg;
// FSWriteMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, G_FS_WRITE>, int, std::string>,
    IPC::Reply<int>
> FSWriteMsg;
// FSRenameMsg
typedef IPC::Message<IPC::Id<VM::QVM, G_FS_RENAME>, std::string, std::string> FSRenameMsg;
// FSFCloseFile
typedef IPC::Message<IPC::Id<VM::QVM, G_FS_FCLOSE_FILE>, int> FSFCloseFileMsg;
// FSGetFileListMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, G_FS_GET_FILE_LIST>, std::string, std::string, int>,
    IPC::Reply<int, std::string>
> FSGetFileListMsg;
// FSFindPakMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, G_FS_FIND_PAK>, std::string>,
	IPC::Reply<bool>
> FSFindPakMsg;

// LocateGameData
typedef IPC::Message<IPC::Id<VM::QVM, G_LOCATE_GAME_DATA1>, IPC::SharedMemory, int, int, int> LocateGameDataMsg1;
typedef IPC::Message<IPC::Id<VM::QVM, G_LOCATE_GAME_DATA2>, int, int, int> LocateGameDataMsg2;

//AdjustAreaPortalStateMsg
typedef IPC::Message<IPC::Id<VM::QVM, G_ADJUST_AREA_PORTAL_STATE>, int, bool> AdjustAreaPortalStateMsg;

// DropClientMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, G_DROP_CLIENT>, int, std::string>
> DropClientMsg;
// SendServerCommandMsg
typedef IPC::Message<IPC::Id<VM::QVM, G_SEND_SERVER_COMMAND>, int, std::string> SendServerCommandMsg;
// SetConfigStringMsg
typedef IPC::Message<IPC::Id<VM::QVM, G_SET_CONFIGSTRING>, int, std::string> SetConfigStringMsg;
// GetConfigStringMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_CONFIGSTRING>, int, int>,
    IPC::Reply<std::string>
> GetConfigStringMsg;
// SetConfigStringRestrictionsMsg
typedef IPC::Message<IPC::Id<VM::QVM, G_SET_CONFIGSTRING_RESTRICTIONS>> SetConfigStringRestrictionsMsg;
// SetUserinfoMsg
typedef IPC::Message<IPC::Id<VM::QVM, G_SET_USERINFO>, int, std::string> SetUserinfoMsg;
// GetUserinfoMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_USERINFO>, int, int>,
    IPC::Reply<std::string>
> GetUserinfoMsg;
// GetServerinfoMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_SERVERINFO>, int>,
    IPC::Reply<std::string>
> GetServerinfoMsg;
// GetUsercmdMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_USERCMD>, int>,
    IPC::Reply<usercmd_t>
> GetUsercmdMsg;
//GetEntityTokenMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_ENTITY_TOKEN>>,
    IPC::Reply<bool, std::string>
> GetEntityTokenMsg;
// SendGameStatMsg
typedef IPC::Message<IPC::Id<VM::QVM, G_SEND_GAME_STAT>, std::string> SendGameStatMsg;
// GetTagMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_TAG>, int, int, std::string>,
    IPC::Reply<int, orientation_t>
> GetTagMsg;
// RegisterTagMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_REGISTER_TAG>, std::string>,
    IPC::Reply<int>
> RegisterTagMsg;
// SendMessageMsg
typedef IPC::Message<IPC::Id<VM::QVM, G_SEND_MESSAGE>, int, int, std::vector<char>> SendMessageMsg;
// MessageStatusMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_MESSAGE_STATUS>, int>,
    IPC::Reply<int>
> MessageStatusMsg;
// RSAGenMsgMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_RSA_GENMSG>, std::string>,
    IPC::Reply<int, std::string, std::string>
> RSAGenMsgMsg;
// GenFingerPrintMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GEN_FINGERPRINT>, int, std::vector<char>, int>,
    IPC::Reply<std::string>
> GenFingerprintMsg;
// GetPlayerPubkeyMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_PLAYER_PUBKEY>, int, int>,
    IPC::Reply<std::string>
> GetPlayerPubkeyMsg;
//GMTimeMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GM_TIME>>,
    IPC::Reply<int, qtime_t>
> GMTimeMsg;
// GetTimeStringMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_TIME_STRING>, int, std::string, qtime_t>,
    IPC::Reply<std::string>
> GetTimeStringMsg;

//ParseAddGlobalDefineMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_PARSE_ADD_GLOBAL_DEFINE>, std::string>,
    IPC::Reply<int>
> ParseAddGlobalDefineMsg;
//ParseLoadSourceMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_PARSE_LOAD_SOURCE>, std::string>,
    IPC::Reply<int>
> ParseLoadSourceMsg;
//ParseFreeSourceMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_PARSE_FREE_SOURCE>, int>,
    IPC::Reply<int>
> ParseFreeSourceMsg;
//ParseReadTokenMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_PARSE_READ_TOKEN>, int>,
    IPC::Reply<int, pc_token_t>
> ParseReadTokenMsg;
//ParseSourceFileAndLineMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_PARSE_SOURCE_FILE_AND_LINE>, int>,
    IPC::Reply<int, std::string, int>
> ParseSourceFileAndLineMsg;

// BotAllocateClientMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_ALLOCATE_CLIENT>>,
	IPC::Reply<int>
> BotAllocateClientMsg;
// BotFreeClientMsg
typedef IPC::Message<IPC::Id<VM::QVM, BOT_FREE_CLIENT>, int> BotFreeClientMsg;
// BotGetConsoleMessageMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_GET_CONSOLE_MESSAGE>, int, int>,
	IPC::Reply<int, std::string>
> BotGetConsoleMessageMsg;
// BotNavSetupMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_NAV_SETUP>, botClass_t>,
	IPC::Reply<int, int>
> BotNavSetupMsg;
// BotNavSetupMsg
typedef IPC::Message<IPC::Id<VM::QVM, BOT_NAV_SHUTDOWN>> BotNavShutdownMsg;
// BotSetNavmeshMsg
typedef IPC::Message<IPC::Id<VM::QVM, BOT_SET_NAVMESH>, int, int> BotSetNavmeshMsg;
// BotFindRouteMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_FIND_ROUTE>, int, botRouteTarget_t, bool>,
	IPC::Reply<int>
> BotFindRouteMsg;
// BotUpdatePathMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_UPDATE_PATH>, int, botRouteTarget_t>,
	IPC::Reply<botNavCmd_t>
> BotUpdatePathMsg;
// BotNavRaycastMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_NAV_RAYCAST>, int, std::array<float, 3>, std::array<float, 3>>,
	IPC::Reply<int, botTrace_t>
> BotNavRaycastMsg;
// BotNavRandomPointMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_NAV_RANDOMPOINT>, int>,
	IPC::Reply<std::array<float, 3>>
> BotNavRandomPointMsg;
// BotNavRandomPointRadiusMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_NAV_RANDOMPOINTRADIUS>, int, std::array<float, 3>, float>,
	IPC::Reply<int, std::array<float, 3>>
> BotNavRandomPointRadiusMsg;
// BotEnableAreaMsg
typedef IPC::Message<IPC::Id<VM::QVM, BOT_ENABLE_AREA>, std::array<float, 3>, std::array<float, 3>, std::array<float, 3>> BotEnableAreaMsg;
// BotDisableAreaMsg
typedef IPC::Message<IPC::Id<VM::QVM, BOT_DISABLE_AREA>, std::array<float, 3>, std::array<float, 3>, std::array<float, 3>> BotDisableAreaMsg;
// BotAddObstacleMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_ADD_OBSTACLE>, std::array<float, 3>, std::array<float, 3>>,
	IPC::Reply<int>
> BotAddObstacleMsg;
// BotRemoveObstacleMsg
typedef IPC::Message<IPC::Id<VM::QVM, BOT_REMOVE_OBSTACLE>, int> BotRemoveObstacleMsg;
// BotUpdateObstaclesMsg
typedef IPC::Message<IPC::Id<VM::QVM, BOT_UPDATE_OBSTACLES>> BotUpdateObstaclesMsg;





// engine-to-game-module calls
typedef enum
{
  GAME_STATIC_INIT,

  GAME_INIT, // void ()( int levelTime, int randomSeed, qboolean restart );
  // the first call to the game module

  GAME_SHUTDOWN, // void ()( void );
  // the last call to the game module

  GAME_LOADMAP,
  GAME_LOADMAPCHUNK,

  GAME_CLIENT_CONNECT, // const char * ()( int clientNum, qboolean firstTime, qboolean isBot );
  // return NULL if the client is allowed to connect,
  //  otherwise return a text string describing the reason for the denial

  GAME_CLIENT_BEGIN, // void ()( int clientNum );

  GAME_CLIENT_USERINFO_CHANGED, // void ()( int clientNum );

  GAME_CLIENT_DISCONNECT, // void ()( int clientNum );

  GAME_CLIENT_COMMAND, // void ()( int clientNum );

  GAME_CLIENT_THINK, // void ()( int clientNum );

  GAME_RUN_FRAME, // void ()( int levelTime );

  GAME_SNAPSHOT_CALLBACK, // qboolean ()( int entityNum, int clientNum );
  // return qfalse if the entity should not be sent to the client

  BOTAI_START_FRAME, // void ()( int levelTime );

  // Cast AI
  BOT_VISIBLEFROMPOS, // qboolean ()( vec3_t srcOrig, int srcNum, dstOrig, int dstNum, qboolean isDummy );
  BOT_CHECKATTACKATPOS, // qboolean ()( int entityNum, int enemyNum, vec3_t position,
  //              qboolean ducking, qboolean allowWorldHit );

  GAME_MESSAGERECEIVED, // void ()( int clientNum, const char *buffer, int bufferSize, int commandTime );
} gameExport_t;

typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_STATIC_INIT>>
> GameStaticInitMsg;
// GameInitMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_INIT>, int, int, bool, bool, bool>
> GameInitMsg;
// GameShutdownMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_SHUTDOWN>, bool>
> GameShutdownMsg;
// GameLoadMapChunkMsg
typedef IPC::Message<IPC::Id<VM::QVM, GAME_LOADMAPCHUNK>, std::vector<char>> GameLoadMapChunkMsg;
// GameLoadMapMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_LOADMAP>, std::string>
> GameLoadMapMsg;
// GameClientConnectMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_CLIENT_CONNECT>, int, bool, int>,
	IPC::Reply<bool, std::string>
> GameClientConnectMsg;
// GameClientBeginMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_CLIENT_BEGIN>, int>
> GameClientBeginMsg;
// GameClientUserinfoChangedMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_CLIENT_USERINFO_CHANGED>, int>
> GameClientUserinfoChangedMsg;
// GameClientDisconnectMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_CLIENT_DISCONNECT>, int>
> GameClientDisconnectMsg;
// GameClientCommandMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_CLIENT_COMMAND>, int, std::string>
> GameClientCommandMsg;
// GameClientThinkMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_CLIENT_THINK>, int>
> GameClientThinkMsg;
// GameRunFrameMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_RUN_FRAME>, int>
> GameRunFrameMsg;
