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
enum gameImport_t
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
};

using LocateGameDataMsg1 = IPC::Message<IPC::Id<VM::QVM, G_LOCATE_GAME_DATA1>, IPC::SharedMemory, int, int, int>;
using LocateGameDataMsg2 = IPC::Message<IPC::Id<VM::QVM, G_LOCATE_GAME_DATA2>, int, int, int>;

using AdjustAreaPortalStateMsg = IPC::Message<IPC::Id<VM::QVM, G_ADJUST_AREA_PORTAL_STATE>, int, bool>;

using DropClientMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, G_DROP_CLIENT>, int, std::string>
>;
using SendServerCommandMsg = IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_SEND_SERVER_COMMAND>, int, std::string>
>;
using SetConfigStringMsg = IPC::Message<IPC::Id<VM::QVM, G_SET_CONFIGSTRING>, int, std::string>;
using GetConfigStringMsg = IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_CONFIGSTRING>, int, int>,
    IPC::Reply<std::string>
>;
using SetConfigStringRestrictionsMsg = IPC::Message<IPC::Id<VM::QVM, G_SET_CONFIGSTRING_RESTRICTIONS>>;
using SetUserinfoMsg = IPC::Message<IPC::Id<VM::QVM, G_SET_USERINFO>, int, std::string>;
using GetUserinfoMsg =  IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_USERINFO>, int, int>,
    IPC::Reply<std::string>
>;
using GetServerinfoMsg = IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_SERVERINFO>, int>,
    IPC::Reply<std::string>
>;
using GetUsercmdMsg = IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_USERCMD>, int>,
    IPC::Reply<usercmd_t>
>;
using GetEntityTokenMsg = IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_ENTITY_TOKEN>>,
    IPC::Reply<bool, std::string>
>;
using SendGameStatMsg = IPC::Message<IPC::Id<VM::QVM, G_SEND_GAME_STAT>, std::string>;
using SendMessageMsg = IPC::Message<IPC::Id<VM::QVM, G_SEND_MESSAGE>, int, int, std::vector<char>>;
using MessageStatusMsg = IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_MESSAGE_STATUS>, int>,
    IPC::Reply<int>
>;
using RSAGenMsgMsg = IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_RSA_GENMSG>, std::string>,
    IPC::Reply<int, std::string, std::string>
>;
using GenFingerprintMsg = IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GEN_FINGERPRINT>, int, std::vector<char>, int>,
    IPC::Reply<std::string>
>;
using GetPlayerPubkeyMsg = IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_PLAYER_PUBKEY>, int, int>,
    IPC::Reply<std::string>
>;
using GetTimeStringMsg = IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, G_GET_TIME_STRING>, int, std::string, qtime_t>,
    IPC::Reply<std::string>
>;

using BotAllocateClientMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_ALLOCATE_CLIENT>>,
	IPC::Reply<int>
>;
using BotFreeClientMsg = IPC::Message<IPC::Id<VM::QVM, BOT_FREE_CLIENT>, int>;
using BotGetConsoleMessageMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_GET_CONSOLE_MESSAGE>, int, int>,
	IPC::Reply<int, std::string>
>;
using BotNavSetupMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_NAV_SETUP>, botClass_t>,
	IPC::Reply<int, int>
>;
using BotNavShutdownMsg = IPC::Message<IPC::Id<VM::QVM, BOT_NAV_SHUTDOWN>>;
using BotSetNavmeshMsg = IPC::Message<IPC::Id<VM::QVM, BOT_SET_NAVMESH>, int, int>;
using BotFindRouteMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_FIND_ROUTE>, int, botRouteTarget_t, bool>,
	IPC::Reply<int>
>;
using BotUpdatePathMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_UPDATE_PATH>, int, botRouteTarget_t>,
	IPC::Reply<botNavCmd_t>
>;
using BotNavRaycastMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_NAV_RAYCAST>, int, std::array<float, 3>, std::array<float, 3>>,
	IPC::Reply<int, botTrace_t>
>;
using BotNavRandomPointMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_NAV_RANDOMPOINT>, int>,
	IPC::Reply<std::array<float, 3>>
>;
using BotNavRandomPointRadiusMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_NAV_RANDOMPOINTRADIUS>, int, std::array<float, 3>, float>,
	IPC::Reply<int, std::array<float, 3>>
>;
using BotEnableAreaMsg = IPC::Message<IPC::Id<VM::QVM, BOT_ENABLE_AREA>, std::array<float, 3>, std::array<float, 3>, std::array<float, 3>>;
using BotDisableAreaMsg = IPC::Message<IPC::Id<VM::QVM, BOT_DISABLE_AREA>, std::array<float, 3>, std::array<float, 3>, std::array<float, 3>>;
using BotAddObstacleMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, BOT_ADD_OBSTACLE>, std::array<float, 3>, std::array<float, 3>>,
	IPC::Reply<int>
>;
using BotRemoveObstacleMsg = IPC::Message<IPC::Id<VM::QVM, BOT_REMOVE_OBSTACLE>, int>;
using BotUpdateObstaclesMsg = IPC::Message<IPC::Id<VM::QVM, BOT_UPDATE_OBSTACLES>>;





// engine-to-game-module calls
enum gameExport_t
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
};

using GameStaticInitMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_STATIC_INIT>, int>
>;
using GameInitMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_INIT>, int, int, bool, bool>
>;
using GameShutdownMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_SHUTDOWN>, bool>
>;
using GameClientConnectMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_CLIENT_CONNECT>, int, bool, int>,
	IPC::Reply<bool, std::string>
>;
using GameClientBeginMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_CLIENT_BEGIN>, int>
>;
using GameClientUserinfoChangedMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_CLIENT_USERINFO_CHANGED>, int>
>;
using GameClientDisconnectMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_CLIENT_DISCONNECT>, int>
>;
using GameClientCommandMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_CLIENT_COMMAND>, int, std::string>
>;
using GameClientThinkMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_CLIENT_THINK>, int>
>;
using GameRunFrameMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, GAME_RUN_FRAME>, int>
>;
