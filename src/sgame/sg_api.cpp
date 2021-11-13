/*
===========================================================================

Unvanquished BSD Source Code
Copyright (c) 2013-2016, Unvanquished Developers
All rights reserved.

This file is part of the Unvanquished BSD Source Code (Unvanquished Source Code).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Unvanquished developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===========================================================================
*/

#include "sg_local.h"
#include "sg_cm_world.h"
#include "botlib/bot_api.h"
#include "engine/server/sg_msgdef.h"
#include "shared/VMMain.h"
#include "shared/CommonProxies.h"
#include "shared/server/sg_api.h"

// Symbols required by the shared VMMain code

int VM::VM_API_VERSION = GAME_API_VERSION;

void VM::VMInit() {
	// Allocate entities and clients shared memory region
	shmRegion = IPC::SharedMemory::Create(sizeof(gentity_t) * MAX_GENTITIES + sizeof(gclient_t) * MAX_CLIENTS);
	char* shmBase = reinterpret_cast<char*>(shmRegion.GetBase());
	g_entities = reinterpret_cast<gentity_t*>(shmBase);
	g_clients = reinterpret_cast<gclient_t*>(shmBase + sizeof(gentity_t) * MAX_GENTITIES);

	// Load the map collision data
	std::string mapName = Cvar::GetValue("mapname");
	CM_LoadMap(mapName);
	G_CM_ClearWorld();
}

void VM::VMHandleSyscall(uint32_t id, Util::Reader reader) {

	int major = id >> 16;
	int minor = id & 0xffff;
	if (major == VM::QVM) {
		switch (minor) {
		case GAME_STATIC_INIT:
			IPC::HandleMsg<GameStaticInitMsg>(VM::rootChannel, std::move(reader), [] (int milliseconds) {
				VM::InitializeProxies(milliseconds);
				FS::Initialize();
				VM::VMInit();
			});
			break;

		case GAME_INIT:
			IPC::HandleMsg<GameInitMsg>(VM::rootChannel, std::move(reader), [](int levelTime, int randomSeed, bool cheats, bool inClient) {
				g_cheats = cheats;
				G_InitGame(levelTime, randomSeed, inClient);
			});
			break;

		case GAME_SHUTDOWN:
			IPC::HandleMsg<GameShutdownMsg>(VM::rootChannel, std::move(reader), [](bool restart) {
				G_ShutdownGame(restart);
			});
			break;

		case GAME_CLIENT_CONNECT:
			IPC::HandleMsg<GameClientConnectMsg>(VM::rootChannel, std::move(reader), [](int clientNum, bool firstTime, int isBot, bool& denied, std::string& reason) {
				const char* deniedStr = isBot ? ClientBotConnect(clientNum, firstTime, TEAM_NONE) : ClientConnect(clientNum, firstTime);
				denied = deniedStr != nullptr;
				if (denied)
					reason = deniedStr;
			});
			break;

		case GAME_CLIENT_THINK:
			IPC::HandleMsg<GameClientThinkMsg>(VM::rootChannel, std::move(reader), [](int clientNum) {
				ClientThink(clientNum);
			});
			break;

		case GAME_CLIENT_USERINFO_CHANGED:
			IPC::HandleMsg<GameClientUserinfoChangedMsg>(VM::rootChannel, std::move(reader), [](int clientNum) {
				ClientUserinfoChanged(clientNum, false);
			});
			break;

		case GAME_CLIENT_DISCONNECT:
			IPC::HandleMsg<GameClientDisconnectMsg>(VM::rootChannel, std::move(reader), [](int clientNum) {
				ClientDisconnect(clientNum);
			});
			break;

		case GAME_CLIENT_BEGIN:
			IPC::HandleMsg<GameClientBeginMsg>(VM::rootChannel, std::move(reader), [](int clientNum) {
				ClientBegin(clientNum);
			});
			break;

		case GAME_CLIENT_COMMAND:
			IPC::HandleMsg<GameClientCommandMsg>(VM::rootChannel, std::move(reader), [](int clientNum, std::string command) {
				Cmd::PushArgs(command);
				ClientCommand(clientNum);
				Cmd::PopArgs();
			});
			break;

		case GAME_RUN_FRAME:
			IPC::HandleMsg<GameRunFrameMsg>(VM::rootChannel, std::move(reader), [](int levelTime) {
				G_RunFrame(levelTime);
			});
			break;

		case GAME_SNAPSHOT_CALLBACK:
			Sys::Drop("GAME_SNAPSHOT_CALLBACK not implemented");
			break;

		case BOTAI_START_FRAME:
			Sys::Drop("BOTAI_START_FRAME not implemented");
			break;

		default:
			Sys::Drop("VMMain(): unknown game command %i", minor);
		}
	} else if (major < VM::LAST_COMMON_SYSCALL) {
		VM::HandleCommonSyscall(major, minor, std::move(reader), VM::rootChannel);
	} else {
		Sys::Drop("unhandled VM major syscall number %i", major);
	}
}

void trap_LinkEntity(gentity_t *ent)
{
	G_CM_LinkEntity(ent);
}

void trap_UnlinkEntity(gentity_t *ent)
{
	G_CM_UnlinkEntity(ent);
}

int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxcount)
{
	return G_CM_AreaEntities(mins, maxs, list, maxcount);
}

bool trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t *ent)
{
	return G_CM_EntityContact( mins, maxs, ent, traceType_t::TT_AABB );
}

void trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
                 const vec3_t end, int passEntityNum, int contentmask, int skipmask )
{
	G_CM_Trace(results, start, mins, maxs, end, passEntityNum, contentmask, skipmask, traceType_t::TT_AABB);
}

int trap_PointContents(const vec3_t point, int passEntityNum)
{
	return G_CM_PointContents( point, passEntityNum );
}

void trap_SetBrushModel(gentity_t *ent, const char *name)
{
	G_CM_SetBrushModel( ent, name );
}

bool trap_InPVS(const vec3_t p1, const vec3_t p2)
{
	return G_CM_inPVS( p1, p2 );
}

bool trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	return G_CM_inPVSIgnorePortals( p1, p2 );
}

void trap_AdjustAreaPortalState(gentity_t *ent, bool open)
{
	VM::SendMsg<AdjustAreaPortalStateMsg>(ent - g_entities, open);
	G_CM_AdjustAreaPortalState( ent, open );
}
