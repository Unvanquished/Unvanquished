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

#include "g_local.h"
#include "../shared/VMMain.h"
#include "../../common/String.h"

#include "../shared/CommonProxies.h"
/*
static NaCl::RootSocket rootSocket;
static NaCl::IPCHandle shmRegion;
static NaCl::SharedMemoryPtr shmMapping;

// Module RPC entry point
static void VMMain(int major, int minor, RPC::Reader& inputs, RPC::Writer& outputs)
{
    if (major == GS_QVM_SYSCALL) {
        switch (minor) {
        case GAME_INIT:
        {
            VM::InitializeProxies();
            int levelTime = inputs.ReadInt();
            int randomSeed = inputs.ReadInt();
            qboolean restart = inputs.ReadInt();
            G_InitGame(levelTime, randomSeed, restart);
            break;
        }

        case GAME_SHUTDOWN:
            G_ShutdownGame(inputs.ReadInt());
            break;

        case GAME_CLIENT_CONNECT:
        {
            int clientNum = inputs.ReadInt();
            qboolean firstTime = inputs.ReadInt();
            qboolean isBot = inputs.ReadInt();
            const char* denied = isBot ? ClientBotConnect(clientNum, firstTime, TEAM_NONE) : ClientConnect(clientNum, firstTime);
            outputs.WriteInt(denied ? qtrue : qfalse);
            if (denied)
                outputs.WriteString(denied);
            break;
        }

        case GAME_CLIENT_THINK:
            ClientThink(inputs.ReadInt());
            break;

        case GAME_CLIENT_USERINFO_CHANGED:
            ClientUserinfoChanged(inputs.ReadInt(), qfalse);
            break;

        case GAME_CLIENT_DISCONNECT:
            ClientDisconnect(inputs.ReadInt());
            break;

        case GAME_CLIENT_BEGIN:
            ClientBegin(inputs.ReadInt());
            break;

        case GAME_CLIENT_COMMAND:
            {
                int clientNum = inputs.ReadInt();
                Str::StringRef command = inputs.ReadString();
                Cmd::PushArgs(command);
                ClientCommand(clientNum);
                Cmd::PopArgs();
            }
            break;

        case GAME_RUN_FRAME:
            G_RunFrame(inputs.ReadInt());
            break;

        case GAME_SNAPSHOT_CALLBACK:
            G_Error("GAME_SNAPSHOT_CALLBACK not implemented");
            break;

        case BOTAI_START_FRAME:
            G_Error("BOTAI_START_FRAME not implemented");
            break;

        case GAME_MESSAGERECEIVED:
            G_Error("GAME_MESSAGERECEIVED not implemented");
            break;

        default:
            G_Error("VMMain(): unknown game command %i", minor);
        }

    } else if (major < IPC::LAST_COMMON_SYSCALL) {
        VM::HandleCommonSyscall(major, minor, inputs, outputs);

    } else {
        G_Error("unhandled VM major syscall number %i", major);
    }

}

// Wrapper which uses the module root socket and VMMain
RPC::Reader DoRPC(RPC::Writer& writer)
{
	return RPC::DoRPC(rootSocket, writer, false, VMMain);
}

int main(int argc, char** argv)
{
	rootSocket = NaCl::GetRootSocket();

	// Send syscall ABI version, also acts as a sign that the module loaded
	RPC::Writer writer;
	writer.WriteInt(GAME_API_VERSION);

	// Allocate entities and clients shared memory region
	shmRegion = NaCl::CreateSharedMemory(sizeof(gentity_t) * MAX_GENTITIES + sizeof(gclient_t) * MAX_CLIENTS);
	if (!shmRegion)
		G_Error("Could not create shared memory region");
	shmMapping = shmRegion.Map();
	if (!shmMapping)
		G_Error("Could not map shared memory region");
	char* shmBase = reinterpret_cast<char*>(shmMapping.Get());
	g_entities = reinterpret_cast<gentity_t*>(shmBase);
	g_clients = reinterpret_cast<gclient_t*>(shmBase + sizeof(gentity_t) * MAX_GENTITIES);

	// Start main RPC loop
	DoRPC(writer);
}
*/
void trap_Print(const char *string)
{
	VM::SendMsg<PrintMsg>(string);
}

void NORETURN trap_Error(const char *string)
{
	static bool recursiveError = false;
	if (recursiveError)
		exit(1);
	recursiveError = true;
	VM::SendMsg<ErrorMsg>(string);
	exit(1); // Amanieu: Need to implement proper error handling
}

void trap_Log(log_event_t *event)
{
	G_Error("trap_Log not implemented");
}

int trap_Milliseconds(void)
{
	int ms;
	VM::SendMsg<MillisecondsMsg>(ms);
	return ms;
}

void trap_SendConsoleCommand(int exec_when, const char *text)
{
	VM::SendMsg<SendConsoleCommandMsg>(exec_when, text);
}

int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
	int ret, handle;
	VM::SendMsg<FSFOpenFileMsg>(qpath, f == NULL, mode, ret, handle);
	if (f)
		*f = handle;
	return ret;
}

void trap_FS_Read(void *buffer, int len, fileHandle_t f)
{
	Str::StringRef res;
	VM::SendMsg<FSReadMsg>(f, len, res);
	memcpy(buffer, res.c_str(), std::min((int)res.size() + 1, len));
}

int trap_FS_Write(const void *buffer, int len, fileHandle_t f)
{
	int res;
	std::string text((const char*)buffer, len);
	VM::SendMsg<FSWriteMsg>(f, text, res);
	return res;
}

void trap_FS_Rename(const char *from, const char *to)
{
	VM::SendMsg<FSRenameMsg>(from, to);
}

void trap_FS_FCloseFile(fileHandle_t f)
{
	VM::SendMsg<FSFCloseFileMsg>(f);
}

int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	int res;
	Str::StringRef text;
	VM::SendMsg<FSGetFileListMsg>(path, extension, bufsize, res, text);
	memcpy(listbuf, text.c_str(), std::min((int)text.size() + 1, bufsize));
	return res;
}

qboolean trap_FindPak( const char *name )
{
	bool res;
	VM::SendMsg<FSFindPakMsg>(name, res);
	return res;
}

/*
// Amanieu: This still has pointers for backward-compatibility, maybe remove them at some point?
// The actual shared memory region is handled in this file, and is pretty much invisible to the rest of the code
void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGClient)
{
	static bool firstTime = true;
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_LOCATE_GAME_DATA);
	if (firstTime) {
		input.WriteHandle(shmRegion);
		firstTime = false;
	}
	input.WriteInt(numGEntities);
	input.WriteInt(sizeofGEntity_t);
	input.WriteInt(sizeofGClient);
	DoRPC(input);
}
*/

void trap_LinkEntity(gentity_t *ent)
{
	VM::SendMsg<LinkEntityMsg>(ent - g_entities);
}

void trap_UnlinkEntity(gentity_t *ent)
{
	VM::SendMsg<UnlinkEntityMsg>(ent - g_entities);
}

int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxcount)
{
	std::vector<int> entityList;
	std::array<float, 3> mins2, maxs2;
	VectorCopy(mins, mins2.data());
	VectorCopy(maxs, maxs2.data());
	VM::SendMsg<EntitiesInBoxMsg>(mins2, maxs2, maxcount, entityList);
	memcpy(list, entityList.data(), sizeof(int) * std::min((int) entityList.size(), maxcount));
	return entityList.size();
}

qboolean trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t *ent)
{
	std::array<float, 3> mins2, maxs2;
	VectorCopy(mins, mins2.data());
	VectorCopy(maxs, maxs2.data());
	int res;
	VM::SendMsg<EntityContactMsg>(mins2, maxs2, ent - g_entities, res);
	return res;
}

void trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	std::array<float, 3> start2, mins2, maxs2, end2;
	VectorCopy(start, start2.data());
	if (mins) {
		VectorCopy(mins, mins2.data());
	} else {
		mins2 = {{0.0f, 0.0f, 0.0f}};
	}
	if (maxs) {
		VectorCopy(maxs, maxs2.data());
	} else {
		maxs2 = {{0.0f, 0.0f, 0.0f}};
	}
	VectorCopy(end, end2.data());
	VM::SendMsg<TraceMsg>(start2, mins2, maxs2, end2, passEntityNum, contentmask, *results);
}

void trap_TraceNoEnts(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	trap_Trace(results, start, mins, maxs, end, -2, contentmask);
}

int trap_PointContents(const vec3_t point, int passEntityNum)
{
	std::array<float, 3> point2;
	VectorCopy(point, point2.data());
	int res;
	VM::SendMsg<PointContentsMsg>(point2, passEntityNum, res);
	return res;
}

void trap_SetBrushModel(gentity_t *ent, const char *name)
{
	VM::SendMsg<SetBrushModelMsg>(ent - g_entities, name);
}

qboolean trap_InPVS(const vec3_t p1, const vec3_t p2)
{
	std::array<float, 3> point1;
	std::array<float, 3> point2;
	VectorCopy(p1, point1.data());
	VectorCopy(p2, point2.data());
	bool res;
	VM::SendMsg<InPVSMsg>(point1, point2, res);
	return res;
}

qboolean trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	std::array<float, 3> point1;
	std::array<float, 3> point2;
	VectorCopy(p1, point1.data());
	VectorCopy(p2, point2.data());
	bool res;
	VM::SendMsg<InPVSIgnorePortalsMsg>(point1, point2, res);
	return res;
}

void trap_AdjustAreaPortalState(gentity_t *ent, qboolean open)
{
	VM::SendMsg<AdjustAreaPortalStateMsg>(ent-g_entities, open);
}

qboolean trap_AreasConnected(int area1, int area2)
{
	bool res;
	VM::SendMsg<AreasConnectedMsg>(area1, area2, res);
	return res;
}

void trap_DropClient(int clientNum, const char *reason)
{
	VM::SendMsg<DropClientMsg>(clientNum, reason);
}

void trap_SendServerCommand(int clientNum, const char *text)
{
	if (strlen(text) > 1022) {
		G_LogPrintf("%s: trap_SendServerCommand( %d, ... ) length exceeds 1022.\n", GAME_VERSION, clientNum);
		G_LogPrintf("%s: text [%.950s]... truncated\n", GAME_VERSION, text);
		return;
	}

	VM::SendMsg<SendServerCommandMsg>(clientNum, text);
}

void trap_SetConfigstring(int num, const char *string)
{
	VM::SendMsg<SetConfigStringMsg>(num, string);
}

void trap_GetConfigstring(int num, char *buffer, int bufferSize)
{
	Str::StringRef res;
	VM::SendMsg<GetConfigStringMsg>(num, bufferSize, res);
	Q_strncpyz(buffer, res.c_str(), bufferSize);
}

void trap_SetConfigstringRestrictions(int num, const clientList_t *clientList)
{
	VM::SendMsg<SetConfigStringRestrictionsMsg>(); // not implemented
}

void trap_SetUserinfo(int num, const char *buffer)
{
	VM::SendMsg<SetUserinfoMsg>(num, buffer);
}

void trap_GetUserinfo(int num, char *buffer, int bufferSize)
{
	Str::StringRef res;
	VM::SendMsg<GetUserinfoMsg>(num, bufferSize, res);
	Q_strncpyz(buffer, res.c_str(), bufferSize);
}

void trap_GetServerinfo(char *buffer, int bufferSize)
{
	Str::StringRef res;
	VM::SendMsg<GetServerinfoMsg>(bufferSize, res);
	Q_strncpyz(buffer, res.c_str(), bufferSize);
}

void trap_GetUsercmd(int clientNum, usercmd_t *cmd)
{
	VM::SendMsg<GetUsercmdMsg>(clientNum, *cmd);
}

qboolean trap_GetEntityToken(char *buffer, int bufferSize)
{
	Str::StringRef text;
	bool res;
	VM::SendMsg<GetEntityTokenMsg>(res, text);
	Q_strncpyz(buffer, text.c_str(), bufferSize);
	return res;
}

void trap_SendGameStat(const char *data)
{
	VM::SendMsg<SendGameStatMsg>(data);
}

qboolean trap_GetTag(int clientNum, int tagFileNumber, const char *tagName, orientation_t *ori)
{
	int res;
	VM::SendMsg<GetTagMsg>(clientNum, tagFileNumber, tagName, res, *ori);
	return res;
}

qboolean trap_LoadTag(const char *filename)
{
	int res;
	VM::SendMsg<RegisterTagMsg>(filename, res);
	return res;
}

void trap_SendMessage(int clientNum, char *buf, int buflen)
{
	std::vector<char> buffer(buflen, 0);
	memcpy(buffer.data(), buf, buflen);
	VM::SendMsg<SendMessageMsg>(clientNum, buflen, buffer);
}

messageStatus_t trap_MessageStatus(int clientNum)
{
	int res;
	VM::SendMsg<MessageStatusMsg>(clientNum, res);
	return static_cast<messageStatus_t>(res);
}

int trap_RSA_GenerateMessage(const char *public_key, char *cleartext, char *encrypted)
{
	Str::StringRef cleartext2, encrypted2;
	int res;
	VM::SendMsg<RSAGenMsgMsg>(public_key, res, cleartext2, encrypted2);
	Q_strncpyz(cleartext, cleartext2.c_str(), RSA_STRING_LENGTH);
	Q_strncpyz(encrypted, encrypted2.c_str(), RSA_STRING_LENGTH);
	return res;
}

void trap_GenFingerprint(const char *pubkey, int size, char *buffer, int bufsize)
{
	std::vector<char> pubkey2(size, 0);
	memcpy(pubkey2.data(), pubkey, size);
	Str::StringRef fingerprint;
	VM::SendMsg<GenFingerprintMsg>(size, pubkey2, bufsize, fingerprint);
	Q_strncpyz(buffer, fingerprint.c_str(), bufsize);
}

void trap_GetPlayerPubkey(int clientNum, char *pubkey, int size)
{
	Str::StringRef pubkey2;
	VM::SendMsg<GetPlayerPubkeyMsg>(clientNum, size, pubkey2);
	Q_strncpyz(pubkey, pubkey2.c_str(), size);
}

int trap_GMTime(qtime_t *qtime)
{
	int res;
	if (qtime) {
		VM::SendMsg<GMTimeMsg>(res, *qtime);
	} else {
		qtime_t t;
		VM::SendMsg<GMTimeMsg>(res, t);
	}
	return res;
}

void trap_GetTimeString(char *buffer, int size, const char *format, const qtime_t *tm)
{
	Str::StringRef text;
	VM::SendMsg<GetTimeStringMsg>(size, format, *tm, text);
	Q_strncpyz(buffer, text.c_str(), size);
}

int trap_Parse_AddGlobalDefine(const char *define)
{
	int res;
	VM::SendMsg<ParseAddGlobalDefineMsg>(define, res);
	return res;
}

int trap_Parse_LoadSource(const char *filename)
{
	int res;
	VM::SendMsg<ParseLoadSourceMsg>(filename, res);
	return res;
}

int trap_Parse_FreeSource(int handle)
{
	int res;
	VM::SendMsg<ParseFreeSourceMsg>(handle, res);
	return res;
}

int trap_Parse_ReadToken(int handle, pc_token_t *pc_token)
{
	int res;
	VM::SendMsg<ParseReadTokenMsg>(handle, res, *pc_token);
	return res;
}

int trap_Parse_SourceFileAndLine(int handle, char *filename, int *line)
{
	int res;
	Str::StringRef filename2;
	VM::SendMsg<ParseSourceFileAndLineMsg>(handle, res, filename2, *line);
	Q_strncpyz(filename, filename2.c_str(), 128);
	return res;
}

/*
void trap_SnapVector(float *v)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_SNAPVECTOR);
	input.Write(v, sizeof(vec3_t));
	RPC::Reader output = DoRPC(input);
	output.Read(v, sizeof(vec3_t));
}

void trap_QuoteString(const char *str, char *buffer, int size)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_QUOTESTRING);
	input.WriteString(str);
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(buffer, output.ReadString(), size);
}

sfxHandle_t trap_RegisterSound(const char *sample, qboolean compressed)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_REGISTERSOUND);
	input.WriteString(sample);
	input.WriteInt(compressed);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

int trap_BotAllocateClient(int clientNum)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_BOT_ALLOCATE_CLIENT);
	input.WriteInt(clientNum);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

void trap_BotFreeClient(int clientNum)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_BOT_FREE_CLIENT);
	input.WriteInt(clientNum);
	DoRPC(input);
}

int trap_BotGetServerCommand(int clientNum, char *message, int size)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_GET_SERVERINFO);
	input.WriteInt(clientNum);
	input.WriteInt(size);
	RPC::Reader output = DoRPC(input);
	int ret = output.ReadInt();
	Q_strncpyz(message, output.ReadString(), size);
	return ret;
}

qboolean trap_BotSetupNav(const botClass_t *botClass, qhandle_t *navHandle)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(BOT_NAV_SETUP);
	input.Write(botClass, sizeof(botClass_t));
	RPC::Reader output = DoRPC(input);
	qboolean ret = output.ReadInt();
	*navHandle = output.ReadInt();
	return ret;
}

void trap_BotShutdownNav(void)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(BOT_NAV_SHUTDOWN);
	DoRPC(input);
}

void trap_BotSetNavMesh(int botClientNum, qhandle_t navHandle)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(BOT_SET_NAVMESH);
	input.WriteInt(botClientNum);
	input.WriteInt(navHandle);
	DoRPC(input);
}

qboolean trap_BotFindRoute(int botClientNum, const botRouteTarget_t *target, qboolean allowPartial)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(BOT_FIND_ROUTE);
	input.WriteInt(botClientNum);
	input.Write(target, sizeof(botRouteTarget_t));
	input.WriteInt(allowPartial);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

qboolean trap_BotUpdatePath(int botClientNum, const botRouteTarget_t *target, botNavCmd_t *cmd)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(BOT_UPDATE_PATH);
	input.WriteInt(botClientNum);
	input.Write(target, sizeof(botRouteTarget_t));
	RPC::Reader output = DoRPC(input);
	output.Read(cmd, sizeof(botNavCmd_t));
	return 0; // Amanieu: This always returns 0, but the value isn't used
}

qboolean trap_BotNavTrace(int botClientNum, botTrace_t *botTrace, const vec3_t start, const vec3_t end)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(BOT_NAV_RAYCAST);
	input.WriteInt(botClientNum);
	input.Write(start, sizeof(vec3_t));
	input.Write(end, sizeof(vec3_t));
	RPC::Reader output = DoRPC(input);
	qboolean ret = output.ReadInt();
	output.Read(botTrace, sizeof(botTrace_t));
	return ret;
}

void trap_BotFindRandomPoint(int botClientNum, vec3_t point)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(BOT_NAV_RANDOMPOINT);
	input.WriteInt(botClientNum);
	RPC::Reader output = DoRPC(input);
	output.Read(point, sizeof(vec3_t));
}

qboolean trap_BotFindRandomPointInRadius(int botClientNum, const vec3_t origin, vec3_t point, float radius)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(BOT_NAV_RANDOMPOINT);
	input.WriteInt(botClientNum);
	input.Write(origin, sizeof(vec3_t));
	input.WriteFloat(radius);
	RPC::Reader output = DoRPC(input);
	qboolean ret = output.ReadInt();
	output.Read(point, sizeof(vec3_t));
	return ret;
}

void trap_BotEnableArea(const vec3_t origin, const vec3_t mins, const vec3_t maxs)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(BOT_ENABLE_AREA);
	input.Write(origin, sizeof(vec3_t));
	input.Write(mins, sizeof(vec3_t));
	input.Write(maxs, sizeof(vec3_t));
	DoRPC(input);
}

void trap_BotDisableArea(const vec3_t origin, const vec3_t mins, const vec3_t maxs)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(BOT_DISABLE_AREA);
	input.Write(origin, sizeof(vec3_t));
	input.Write(mins, sizeof(vec3_t));
	input.Write(maxs, sizeof(vec3_t));
	DoRPC(input);
}

void trap_BotAddObstacle(const vec3_t mins, const vec3_t maxs, qhandle_t *handle)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(BOT_ADD_OBSTACLE);
	input.Write(mins, sizeof(vec3_t));
	input.Write(maxs, sizeof(vec3_t));
	RPC::Reader output = DoRPC(input);
	*handle = output.ReadInt();
}

void trap_BotRemoveObstacle(qhandle_t handle)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(BOT_REMOVE_OBSTACLE);
	input.WriteInt(handle);
	DoRPC(input);
}

void trap_BotUpdateObstacles(void)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(BOT_UPDATE_OBSTACLES);
	DoRPC(input);
}
*/
