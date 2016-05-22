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

// sv_game.c -- interface to the game module

#include "server.h"
#include "sg_msgdef.h"
#include "qcommon/crypto.h"
#include "framework/CommonVMServices.h"
#include "framework/CommandSystem.h"

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part

sharedEntity_t *SV_GentityNum( int num )
{
	sharedEntity_t *ent;

	if ( num < 0 || num >= MAX_GENTITIES )
	{
		Com_Error( errorParm_t::ERR_DROP, "SV_GentityNum: bad num %d", num );
	}

	ent = ( sharedEntity_t * )( ( byte * ) sv.gentities + sv.gentitySize * ( num ) );

	return ent;
}

playerState_t  *SV_GameClientNum( int num )
{
	playerState_t *ps;

	if ( num >= sv_maxclients->integer )
	{
		Com_Error( errorParm_t::ERR_DROP, "SV_GameClientNum: bad num" );
	}

	ps = ( playerState_t * )( ( byte * ) sv.gameClients + sv.gameClientSize * ( num ) );

	return ps;
}

svEntity_t     *SV_SvEntityForGentity( sharedEntity_t *gEnt )
{
	if ( !gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES )
	{
		Com_Error( errorParm_t::ERR_DROP, "SV_SvEntityForGentity: bad gEnt" );
	}

	return &sv.svEntities[ gEnt->s.number ];
}

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand( int clientNum, const char *text )
{
	if ( clientNum == -1 )
	{
		SV_SendServerCommand( nullptr, "%s", text );
	}
	else if ( clientNum == -2 )
	{
		SV_PrintTranslatedText( text, false, false );
	}
	else
	{
		if ( clientNum < 0 || clientNum >= sv_maxclients->integer )
		{
			return;
		}

		SV_SendServerCommand( svs.clients + clientNum, "%s", text );
	}
}

/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient( int clientNum, const char *reason )
{
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer )
	{
		return;
	}

	SV_DropClient( svs.clients + clientNum, reason );
}

/*
===============
SV_RSAGenMsg

Generate an encrypted RSA message
===============
*/
int SV_RSAGenMsg( const char *pubkey, char *cleartext, char *encrypted )
{
	struct rsa_public_key public_key;

	mpz_t                 message;
	unsigned char         buffer[ RSA_KEY_LENGTH / 8 - 11 ];
	int                   retval;
	Sys::GenRandomBytes( buffer, RSA_KEY_LENGTH / 8 - 11 );
	nettle_mpz_init_set_str_256_u( message, RSA_KEY_LENGTH / 8 - 11, buffer );
	mpz_get_str( cleartext, 16, message );
	rsa_public_key_init( &public_key );
	mpz_set_ui( public_key.e, RSA_PUBLIC_EXPONENT );
	retval = mpz_set_str( public_key.n, pubkey, 16 ) + 1;

	if ( retval )
	{
		rsa_public_key_prepare( &public_key );
		retval = rsa_encrypt( &public_key, nullptr, qnettle_random, RSA_KEY_LENGTH / 8 - 11, buffer, message );
	}

	rsa_public_key_clear( &public_key );
	mpz_get_str( encrypted, 16, message );
	mpz_clear( message );
	return retval;
}

/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo( char *buffer, int bufferSize )
{
	if ( bufferSize < 1 )
	{
		Com_Error( errorParm_t::ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize );
	}

	Q_strncpyz( buffer, Cvar_InfoString( CVAR_SERVERINFO, false ), bufferSize );
}

/*
===============
SV_LocateGameData

===============
*/

void SV_LocateGameData( const IPC::SharedMemory& shmRegion, int numGEntities, int sizeofGEntity_t,
                        int sizeofGameClient )
{
	if ( numGEntities < 0 || sizeofGEntity_t < 0 || sizeofGameClient < 0 )
		Com_Error( errorParm_t::ERR_DROP, "SV_LocateGameData: Invalid game data parameters" );
	if ( (int) shmRegion.GetSize() < numGEntities * sizeofGEntity_t + sv_maxclients->integer * sizeofGameClient )
		Com_Error( errorParm_t::ERR_DROP, "SV_LocateGameData: Shared memory region too small" );

	char* base = static_cast<char*>(shmRegion.GetBase());
	sv.gentities = reinterpret_cast<sharedEntity_t*>(base);
	sv.gentitySize = sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gameClients = reinterpret_cast<playerState_t*>(base + MAX_GENTITIES * sizeofGEntity_t);
	sv.gameClientSize = sizeofGameClient;
}

/*
===============
SV_GetUsercmd

===============
*/
void SV_GetUsercmd( int clientNum, usercmd_t *cmd )
{
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer )
	{
		Com_Error( errorParm_t::ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum );
	}

	*cmd = svs.clients[ clientNum ].lastUsercmd;
}

/*
====================
SV_SendBinaryMessage
====================
*/
static void SV_SendBinaryMessage(int cno, std::vector<uint8_t> message)
{
	if (cno < 0 || cno >= sv_maxclients->integer) {
		Com_Error(errorParm_t::ERR_DROP, "SV_SendBinaryMessage: bad client %i", cno);
	}

	if (message.size() > MAX_BINARY_MESSAGE) {
		Com_Error(errorParm_t::ERR_DROP, "SV_SendBinaryMessage: bad length %zi", message.size());
	}

	auto &cl = svs.clients[cno];
	memcpy(cl.binaryMessage, message.data(), cl.binaryMessageLength = message.size());
}

/*
====================
SV_BinaryMessageStatus
====================
*/
static messageStatus_t SV_BinaryMessageStatus(int cno)
{
	if (cno < 0 || cno >= sv_maxclients->integer) {
		return messageStatus_t::MESSAGE_EMPTY;
	}
	const auto &cl = svs.clients[cno];
	if (cl.binaryMessageLength == 0) {
		return messageStatus_t::MESSAGE_EMPTY;
	}
	if (cl.binaryMessageOverflowed) {
		return messageStatus_t::MESSAGE_WAITING_OVERFLOW;
	}
	return messageStatus_t::MESSAGE_WAITING;
}

/*
====================
SV_GameBinaryMessageReceived
====================
*/
void SV_GameBinaryMessageReceived(int cno, const byte *buf, size_t buflen, int commandTime)
{
	gvm.GameMessageRecieved( cno, buf, buflen, commandTime );
}

//==============================================

/*
====================
SV_GetTimeString

Returns 0 if we have a representable time
Truncation is ignored
====================
*/
static void SV_GetTimeString( char *buffer, int length, const char *format, const qtime_t *tm )
{
	if ( tm )
	{
		struct tm t;

		t.tm_sec   = tm->tm_sec;
		t.tm_min   = tm->tm_min;
		t.tm_hour  = tm->tm_hour;
		t.tm_mday  = tm->tm_mday;
		t.tm_mon   = tm->tm_mon;
		t.tm_year  = tm->tm_year;
		t.tm_wday  = tm->tm_wday;
		t.tm_yday  = tm->tm_yday;
		t.tm_isdst = tm->tm_isdst;

		strftime ( buffer, length, format, &t );
	}
	else
	{
		strftime( buffer, length, format, gmtime( nullptr ) );
	}
}

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs()
{
	if ( !gvm.IsActive() )
	{
		return;
	}

	gvm.GameShutdown( false );
    gvm.Free();
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
static void SV_InitGameVM()
{
	int i;

	// start the entity parsing at the beginning
	sv.entityParsePoint = CM_EntityString();

	// clear all gentity pointers that might still be set from
	// a previous level
	for ( i = 0; i < sv_maxclients->integer; i++ )
	{
		svs.clients[ i ].gentity = nullptr;
	}

	// use the current msec count for a random seed
	// init for this gamestate
	gvm.GameInit( sv.time, Com_Milliseconds());
}

/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a map change
===================
*/
void SV_RestartGameProgs()
{
	if ( !gvm.IsActive() )
	{
		return;
	}

	gvm.GameShutdown( true );

	gvm.Start();

	SV_InitGameVM();
}

/*
===============
SV_InitGameProgs

Called on a map change, not on a map_restart
===============
*/
void SV_InitGameProgs()
{
	sv.num_tagheaders = 0;
	sv.num_tags = 0;

	// load the game module
	gvm.Start();

	SV_InitGameVM();
}

GameVM::GameVM(): VM::VMBase("sgame"), services(nullptr){
}

void GameVM::Start()
{
	services = std::unique_ptr<VM::CommonVMServices>(new VM::CommonVMServices(*this, "SGame", Cmd::SGAME_VM));

	uint32_t version = this->Create();
	if ( version != GAME_API_VERSION ) {
		Com_Error( errorParm_t::ERR_DROP, "SGame ABI mismatch, expected %d, got %d", GAME_API_VERSION, version );
	}

	this->GameStaticInit();
}

void GameVM::GameStaticInit()
{
	this->SendMsg<GameStaticInitMsg>(Sys_Milliseconds());
}

void GameVM::GameInit(int levelTime, int randomSeed)
{
	this->SendMsg<GameInitMsg>(levelTime, randomSeed, Com_AreCheatsAllowed(), Com_IsClient());
}

void GameVM::GameShutdown(bool restart)
{
	// Ignore errors when shutting down
	try {
		this->SendMsg<GameShutdownMsg>(restart);
		this->Free();
	} catch (Sys::DropErr&) {}
	services = nullptr;

	// Release the shared memory region
	this->shmRegion.Close();
}

bool GameVM::GameClientConnect(char* reason, size_t size, int clientNum, bool firstTime, bool isBot)
{
	bool denied;
	std::string sentReason;
	this->SendMsg<GameClientConnectMsg>(clientNum, firstTime, isBot, denied, sentReason);

	if (denied) {
		Q_strncpyz(reason, sentReason.c_str(), size);
	}
	return denied;
}

void GameVM::GameClientBegin(int clientNum)
{
	this->SendMsg<GameClientBeginMsg>(clientNum);
}

void GameVM::GameClientUserInfoChanged(int clientNum)
{
	this->SendMsg<GameClientUserinfoChangedMsg>(clientNum);
}

void GameVM::GameClientDisconnect(int clientNum)
{
	this->SendMsg<GameClientDisconnectMsg>(clientNum);
}

void GameVM::GameClientCommand(int clientNum, const char* command)
{
	this->SendMsg<GameClientCommandMsg>(clientNum, command);
}

void GameVM::GameClientThink(int clientNum)
{
	this->SendMsg<GameClientThinkMsg>(clientNum);
}

void GameVM::GameRunFrame(int levelTime)
{
	this->SendMsg<GameRunFrameMsg>(levelTime);
}

bool GameVM::GameSnapshotCallback(int, int)
{
	Com_Error(errorParm_t::ERR_DROP, "GameVM::GameSnapshotCallback not implemented");
}

void GameVM::BotAIStartFrame(int)
{
	Com_Error(errorParm_t::ERR_DROP, "GameVM::BotAIStartFrame not implemented");
}

void GameVM::GameMessageRecieved(int clientNum, const byte *buf, size_t size, int commandTime)
{
	auto shm = IPC::SharedMemory::Create(size);
	memcpy(shm.GetBase(), buf, size);
	gvm.SendMsg<GameRecvMessageMsg>(clientNum, shm, size, commandTime);
}

void GameVM::Syscall(uint32_t id, Util::Reader reader, IPC::Channel& channel)
{
	int major = id >> 16;
	int minor = id & 0xffff;
	if (major == VM::QVM) {
		this->QVMSyscall(minor, reader, channel);

    } else if (major < VM::LAST_COMMON_SYSCALL) {
        services->Syscall(major, minor, std::move(reader), channel);

    } else {
		Com_Error(errorParm_t::ERR_DROP, "Bad major game syscall number: %d", major);
	}
}

void GameVM::QVMSyscall(int index, Util::Reader& reader, IPC::Channel& channel)
{
	switch (index) {
	case G_LOCATE_GAME_DATA1:
		IPC::HandleMsg<LocateGameDataMsg1>(channel, std::move(reader), [this](IPC::SharedMemory shm, int numEntities, int entitySize, int playerSize) {
			shmRegion = std::move(shm);
			SV_LocateGameData(shmRegion, numEntities, entitySize, playerSize);
		});
		break;

	case G_LOCATE_GAME_DATA2:
		IPC::HandleMsg<LocateGameDataMsg2>(channel, std::move(reader), [this](int numEntities, int entitySize, int playerSize) {
			SV_LocateGameData(shmRegion, numEntities, entitySize, playerSize);
		});
		break;

	case G_ADJUST_AREA_PORTAL_STATE:
		IPC::HandleMsg<AdjustAreaPortalStateMsg>(channel, std::move(reader), [this](int entityNum, bool open) {
			sharedEntity_t* ent = SV_GentityNum(entityNum);
			CM_AdjustAreaPortalState(ent->r.areanum, ent->r.areanum2, open);
		});
		break;

	case G_DROP_CLIENT:
		IPC::HandleMsg<DropClientMsg>(channel, std::move(reader), [this](int clientNum, std::string reason) {
			SV_GameDropClient(clientNum, reason.c_str());
		});
		break;

	case G_SEND_SERVER_COMMAND:
		IPC::HandleMsg<SendServerCommandMsg>(channel, std::move(reader), [this](int clientNum, std::string text) {
			SV_GameSendServerCommand(clientNum, text.c_str());
		});
		break;

	case G_SET_CONFIGSTRING:
		IPC::HandleMsg<SetConfigStringMsg>(channel, std::move(reader), [this](int index, std::string val) {
			SV_SetConfigstring(index, val.c_str());
		});
		break;

	case G_GET_CONFIGSTRING:
		IPC::HandleMsg<GetConfigStringMsg>(channel, std::move(reader), [this](int index, int len, std::string& res) {
			std::unique_ptr<char[]> buffer(new char[len]);
			buffer[0] = '\0';
			SV_GetConfigstring(index, buffer.get(), len);
			res.assign(buffer.get(), len);
		});
		break;

	case G_SET_CONFIGSTRING_RESTRICTIONS:
		IPC::HandleMsg<SetConfigStringRestrictionsMsg>(channel, std::move(reader), [this] {
			//Log::Notice("SV_SetConfigstringRestrictions not implemented\n");
		});
		break;

	case G_SET_USERINFO:
		IPC::HandleMsg<SetUserinfoMsg>(channel, std::move(reader), [this](int index, std::string val) {
			SV_SetUserinfo(index, val.c_str());
		});
		break;

	case G_GET_USERINFO:
		IPC::HandleMsg<GetUserinfoMsg>(channel, std::move(reader), [this](int index, int len, std::string& res) {
			std::unique_ptr<char[]> buffer(new char[len]);
			buffer[0] = '\0';
			SV_GetUserinfo(index, buffer.get(), len);
			res.assign(buffer.get(), len);
		});
		break;

	case G_GET_SERVERINFO:
		IPC::HandleMsg<GetServerinfoMsg>(channel, std::move(reader), [this](int len, std::string& res) {
			std::unique_ptr<char[]> buffer(new char[len]);
			buffer[0] = '\0';
			SV_GetServerinfo(buffer.get(), len);
			res.assign(buffer.get(), len);
		});
		break;

	case G_GET_USERCMD:
		IPC::HandleMsg<GetUsercmdMsg>(channel, std::move(reader), [this](int index, usercmd_t& cmd) {
			SV_GetUsercmd(index, &cmd);
		});
		break;

	case G_GET_ENTITY_TOKEN:
		IPC::HandleMsg<GetEntityTokenMsg>(channel, std::move(reader), [this](bool& boolRes, std::string& res) {
			res = COM_Parse(&sv.entityParsePoint);
			boolRes = sv.entityParsePoint or res.size() > 0;
		});
		break;

	case G_SEND_GAME_STAT:
		IPC::HandleMsg<SendGameStatMsg>(channel, std::move(reader), [this](std::string text) {
			SV_MasterGameStat(text.c_str());
		});
		break;

	case G_SEND_MESSAGE:
		IPC::HandleMsg<SendMessageMsg>(channel, std::move(reader), [this](int clientNum, std::vector<uint8_t> message) {
			SV_SendBinaryMessage(clientNum, std::move(message));
		});
		break;

	case G_MESSAGE_STATUS:
		IPC::HandleMsg<MessageStatusMsg>(channel, std::move(reader), [this](int index, messageStatus_t& status) {
			status = SV_BinaryMessageStatus(index);
		});
		break;

	case G_RSA_GENMSG:
		IPC::HandleMsg<RSAGenMsgMsg>(channel, std::move(reader), [this](std::string pubkey, int& res, std::string& cleartext, std::string& encrypted) {
			char cleartextBuffer[RSA_STRING_LENGTH];
			char encryptedBuffer[RSA_STRING_LENGTH];
			res = SV_RSAGenMsg(pubkey.c_str(), cleartextBuffer, encryptedBuffer);
			cleartext = cleartextBuffer;
			encrypted = encryptedBuffer;
		});
		break;

	case G_GEN_FINGERPRINT:
		IPC::HandleMsg<GenFingerprintMsg>(channel, std::move(reader), [this](int keylen, const std::vector<char>& key, int len, std::string& res) {
			std::unique_ptr<char[]> buffer(new char[len]);
			buffer[0] = '\0';
			Com_MD5Buffer(key.data(), keylen, buffer.get(), len);
			res.assign(buffer.get(), len);
		});
		break;

	case G_GET_PLAYER_PUBKEY:
		IPC::HandleMsg<GetPlayerPubkeyMsg>(channel, std::move(reader), [this](int clientNum, int len, std::string& pubkey) {
			std::unique_ptr<char[]> buffer(new char[len]);
			buffer[0] = '\0';
			SV_GetPlayerPubkey(clientNum, buffer.get(), len);
			pubkey.assign(buffer.get());
		});
		break;

	case G_GET_TIME_STRING:
		IPC::HandleMsg<GetTimeStringMsg>(channel, std::move(reader), [this](int len, std::string format, const qtime_t& time, std::string& res) {
			std::unique_ptr<char[]> buffer(new char[len]);
			buffer[0] = '\0';
			SV_GetTimeString(buffer.get(), len, format.c_str(), &time);
			res.assign(buffer.get(), len);
		});
		break;

	case BOT_ALLOCATE_CLIENT:
		IPC::HandleMsg<BotAllocateClientMsg>(channel, std::move(reader), [this](int& output) {
			output = SV_BotAllocateClient();
		});
		break;

	case BOT_FREE_CLIENT:
		IPC::HandleMsg<BotFreeClientMsg>(channel, std::move(reader), [this](int input) {
			SV_BotFreeClient(input);
		});
		break;

	case BOT_GET_CONSOLE_MESSAGE:
		IPC::HandleMsg<BotGetConsoleMessageMsg>(channel, std::move(reader), [this](int client, int len, int& res, std::string& message) {
			std::unique_ptr<char[]> buffer(new char[len]);
			buffer[0] = '\0';
			res = SV_BotGetConsoleMessage(client, buffer.get(), len);
			message.assign(buffer.get(), len);
		});
		break;

	case BOT_NAV_SETUP:
		IPC::HandleMsg<BotNavSetupMsg>(channel, std::move(reader), [this](botClass_t botClass, int& res, int& handle) {
			res = BotSetupNav(&botClass, &handle);
		});
		break;

	case BOT_NAV_SHUTDOWN:
		BotShutdownNav();
		break;

	case BOT_SET_NAVMESH:
		IPC::HandleMsg<BotSetNavmeshMsg>(channel, std::move(reader), [this](int clientNum, int navHandle) {
			BotSetNavMesh(clientNum, navHandle);
		});
		break;

	case BOT_FIND_ROUTE:
		IPC::HandleMsg<BotFindRouteMsg>(channel, std::move(reader), [this](int clientNum, botRouteTarget_t target, bool allowPartial, int& res) {
			res = BotFindRouteExt(clientNum, &target, allowPartial);
		});
		break;

	case BOT_UPDATE_PATH:
		IPC::HandleMsg<BotUpdatePathMsg>(channel, std::move(reader), [this](int clientNum, botRouteTarget_t target, botNavCmd_t& cmd) {
			BotUpdateCorridor(clientNum, &target, &cmd);
		});
		break;

	case BOT_NAV_RAYCAST:
		IPC::HandleMsg<BotNavRaycastMsg>(channel, std::move(reader), [this](int clientNum, std::array<float, 3> start, std::array<float, 3> end, int& res, botTrace_t& botTrace) {
			res = BotNavTrace(clientNum, &botTrace, start.data(), end.data());
		});
		break;

	case BOT_NAV_RANDOMPOINT:
		IPC::HandleMsg<BotNavRandomPointMsg>(channel, std::move(reader), [this](int clientNum, std::array<float, 3>& point) {
			BotFindRandomPoint(clientNum, point.data());
		});
		break;

	case BOT_NAV_RANDOMPOINTRADIUS:
		IPC::HandleMsg<BotNavRandomPointRadiusMsg>(channel, std::move(reader), [this](int clientNum, std::array<float, 3> origin, float radius, int& res, std::array<float, 3>& point) {
			res = BotFindRandomPointInRadius(clientNum, origin.data(), point.data(), radius);
		});
		break;

	case BOT_ENABLE_AREA:
		IPC::HandleMsg<BotEnableAreaMsg>(channel, std::move(reader), [this](std::array<float, 3> origin, std::array<float, 3> mins, std::array<float, 3> maxs) {
			BotEnableArea(origin.data(), mins.data(), maxs.data());
		});
		break;

	case BOT_DISABLE_AREA:
		IPC::HandleMsg<BotDisableAreaMsg>(channel, std::move(reader), [this](std::array<float, 3> origin, std::array<float, 3> mins, std::array<float, 3> maxs) {
			BotDisableArea(origin.data(), mins.data(), maxs.data());
		});
		break;

	case BOT_ADD_OBSTACLE:
		IPC::HandleMsg<BotAddObstacleMsg>(channel, std::move(reader), [this](std::array<float, 3> mins, std::array<float, 3> maxs, int& handle) {
			BotAddObstacle(mins.data(), maxs.data(), &handle);
		});
		break;

	case BOT_REMOVE_OBSTACLE:
		IPC::HandleMsg<BotRemoveObstacleMsg>(channel, std::move(reader), [this](int handle) {
			BotRemoveObstacle(handle);
		});
		break;

	case BOT_UPDATE_OBSTACLES:
		BotUpdateObstacles();
		break;

	default:
		Com_Error(errorParm_t::ERR_DROP, "Bad game system trap: %d", index);
	}
}
