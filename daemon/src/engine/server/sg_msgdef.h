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

#include "common/IPC/CommonSyscalls.h"

// game-module-to-engine calls
typedef enum gameImport_s
{

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
  G_SEND_MESSAGE,
  G_MESSAGE_STATUS,
  G_RSA_GENMSG, // ( const char *public_key, char *cleartext, char *encrypted )
  G_GEN_FINGERPRINT,
  G_GET_PLAYER_PUBKEY,
  G_GET_TIME_STRING,
  G_CRASH_DUMP,

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
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_SEND_SERVER_COMMAND>, int, std::string>
> SendServerCommandMsg;
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
// GetTimeStringMsg
typedef IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_TIME_STRING>, int, std::string, qtime_t>,
    IPC::Reply<std::string>
> GetTimeStringMsg;
// CrashDumpMsg
typedef IPC::SyncMessage <
    IPC::Message<IPC::Id<VM::QVM, G_CRASH_DUMP>, std::vector<uint8_t> >
> CrashDumpMsg;

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

  GAME_INIT, // void ()( int levelTime, int randomSeed, bool restart );
  // the first call to the game module

  GAME_SHUTDOWN, // void ()();
  // the last call to the game module

  GAME_CLIENT_CONNECT, // const char * ()( int clientNum, bool firstTime, bool isBot );
  // return nullptr if the client is allowed to connect,
  //  otherwise return a text string describing the reason for the denial

  GAME_CLIENT_BEGIN, // void ()( int clientNum );

  GAME_CLIENT_USERINFO_CHANGED, // void ()( int clientNum );

  GAME_CLIENT_DISCONNECT, // void ()( int clientNum );

  GAME_CLIENT_COMMAND, // void ()( int clientNum );

  GAME_CLIENT_THINK, // void ()( int clientNum );

  GAME_RUN_FRAME, // void ()( int levelTime );

  GAME_SNAPSHOT_CALLBACK, // bool ()( int entityNum, int clientNum );
  // return false if the entity should not be sent to the client

  BOTAI_START_FRAME, // void ()( int levelTime );

  // Cast AI
  BOT_VISIBLEFROMPOS, // bool ()( vec3_t srcOrig, int srcNum, dstOrig, int dstNum, bool isDummy );
  BOT_CHECKATTACKATPOS, // bool ()( int entityNum, int enemyNum, vec3_t position,
  //              bool ducking, bool allowWorldHit );

  GAME_MESSAGERECEIVED, // void ()( int clientNum, const char *buffer, int bufferSize, int commandTime );
} gameExport_t;

// GameStaticInitMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_STATIC_INIT>, int>
> GameStaticInitMsg;
// GameInitMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_INIT>, int, int, bool, bool>
> GameInitMsg;
// GameShutdownMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_SHUTDOWN>, bool>
> GameShutdownMsg;
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
