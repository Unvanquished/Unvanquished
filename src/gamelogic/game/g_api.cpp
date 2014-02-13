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
#include "../../libs/nacl/nacl.h"
#include "../../common/RPC.h"
#include "../../common/String.h"

#include "../shared/CommonProxies.h"

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

void trap_Print(const char *string)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_PRINT);
	input.WriteString(string);
	DoRPC(input);
}

void NORETURN trap_Error(const char *string)
{
	static bool recursiveError = false;
	if (recursiveError)
		exit(1);
	recursiveError = true;
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_ERROR);
	input.WriteString(string);
	DoRPC(input);
	exit(1); // Amanieu: Need to implement proper error handling
}

void trap_Log(log_event_t *event)
{
	G_Error("trap_Log not implemented");
}

int trap_Milliseconds(void)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_MILLISECONDS);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

void trap_SendConsoleCommand(int exec_when, const char *text)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_SEND_CONSOLE_COMMAND);
	input.WriteInt(exec_when);
	input.WriteString(text);
	DoRPC(input);
}

int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_FS_FOPEN_FILE);
	input.WriteString(qpath);
	input.WriteInt(f != NULL);
	input.WriteInt(mode);
	RPC::Reader output = DoRPC(input);
	int ret = output.ReadInt();
	if (f)
		*f = output.ReadInt();
	return ret;
}

void trap_FS_Read(void *buffer, int len, fileHandle_t f)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_FS_READ);
	input.WriteInt(f);
	input.WriteInt(len);
	RPC::Reader output = DoRPC(input);
	output.Read(buffer, len);
}

int trap_FS_Write(const void *buffer, int len, fileHandle_t f)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_FS_WRITE);
	input.WriteInt(f);
	input.WriteInt(len);
	input.Write(buffer, len);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

void trap_FS_Rename(const char *from, const char *to)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_FS_RENAME);
	input.WriteString(from);
	input.WriteString(to);
	DoRPC(input);
}

void trap_FS_FCloseFile(fileHandle_t f)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_FS_FCLOSE_FILE);
	input.WriteInt(f);
	DoRPC(input);
}

int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_FS_GETFILELIST);
	input.WriteString(path);
	input.WriteString(extension);
	input.WriteInt(bufsize);
	RPC::Reader output = DoRPC(input);
	int ret = output.ReadInt();
	output.Read(listbuf, bufsize);
	return ret;
}

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

void trap_DropClient(int clientNum, const char *reason)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_DROP_CLIENT);
	input.WriteInt(clientNum);
	input.WriteString(reason);
	DoRPC(input);
}

void trap_SendServerCommand(int clientNum, const char *text)
{
	if (strlen(text) > 1022) {
		G_LogPrintf("%s: trap_SendServerCommand( %d, ... ) length exceeds 1022.\n", GAME_VERSION, clientNum);
		G_LogPrintf("%s: text [%.950s]... truncated\n", GAME_VERSION, text);
		return;
	}

	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_SEND_SERVER_COMMAND);
	input.WriteInt(clientNum);
	input.WriteString(text);
	DoRPC(input);
}

void trap_LinkEntity(gentity_t *ent)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_LINKENTITY);
	input.WriteInt(ent - g_entities);
	DoRPC(input);
}

void trap_UnlinkEntity(gentity_t *ent)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_UNLINKENTITY);
	input.WriteInt(ent - g_entities);
	DoRPC(input);
}

int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxcount)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_ENTITIES_IN_BOX);
	input.Write(mins, sizeof(vec3_t));
	input.Write(maxs, sizeof(vec3_t));
	input.WriteInt(maxcount);
	RPC::Reader output = DoRPC(input);
	int len = std::min(output.ReadInt(), maxcount);
	output.Read(list, len * sizeof(int));
	return len;
}

qboolean trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t *ent)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_ENTITY_CONTACT);
	input.Write(mins, sizeof(vec3_t));
	input.Write(maxs, sizeof(vec3_t));
	input.WriteInt(ent - g_entities);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

qboolean trap_EntityContactCapsule(const vec3_t mins, const vec3_t maxs, const gentity_t *ent)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_ENTITY_CONTACTCAPSULE);
	input.Write(mins, sizeof(vec3_t));
	input.Write(maxs, sizeof(vec3_t));
	input.WriteInt(ent - g_entities);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

void trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_TRACE);
	input.Write(start, sizeof(vec3_t));
	input.Write(mins ? mins : vec3_origin, sizeof(vec3_t));
	input.Write(maxs ? maxs : vec3_origin, sizeof(vec3_t));
	input.Write(end, sizeof(vec3_t));
	input.WriteInt(passEntityNum);
	input.WriteInt(contentmask);
	RPC::Reader output = DoRPC(input);
	output.Read(results, sizeof(trace_t));
}

void trap_TraceNoEnts(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	trap_Trace(results, start, mins, maxs, end, -2, contentmask);
}

void trap_TraceCapsule(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_TRACECAPSULE);
	input.Write(start, sizeof(vec3_t));
	input.Write(mins ? mins : vec3_origin, sizeof(vec3_t));
	input.Write(maxs ? maxs : vec3_origin, sizeof(vec3_t));
	input.Write(end, sizeof(vec3_t));
	input.WriteInt(passEntityNum);
	input.WriteInt(contentmask);
	RPC::Reader output = DoRPC(input);
	output.Read(results, sizeof(trace_t));
}

void trap_TraceCapsuleNoEnts(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	trap_TraceCapsule(results, start, mins, maxs, end, -2, contentmask);
}

int trap_PointContents(const vec3_t point, int passEntityNum)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_POINT_CONTENTS);
	input.Write(point, sizeof(vec3_t));
	input.WriteInt(passEntityNum);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

void trap_SetBrushModel(gentity_t *ent, const char *name)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_SET_BRUSH_MODEL);
	input.WriteInt(ent - g_entities);
	input.WriteString(name);
	DoRPC(input);
}

qboolean trap_InPVS(const vec3_t p1, const vec3_t p2)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_IN_PVS);
	input.Write(p1, sizeof(vec3_t));
	input.Write(p2, sizeof(vec3_t));
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

qboolean trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_IN_PVS_IGNORE_PORTALS);
	input.Write(p1, sizeof(vec3_t));
	input.Write(p2, sizeof(vec3_t));
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

void trap_SetConfigstring(int num, const char *string)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_SET_CONFIGSTRING);
	input.WriteInt(num);
	input.WriteString(string);
	DoRPC(input);
}

void trap_GetConfigstring(int num, char *buffer, int bufferSize)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_GET_CONFIGSTRING);
	input.WriteInt(num);
	input.WriteInt(bufferSize);
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(buffer, output.ReadString(), bufferSize);
}

void trap_SetConfigstringRestrictions(int num, const clientList_t *clientList)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_SET_CONFIGSTRING_RESTRICTIONS);
	input.WriteInt(num);
	input.Write(clientList, sizeof(clientList_t));
	DoRPC(input);
}

void trap_SetUserinfo(int num, const char *buffer)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_SET_USERINFO);
	input.WriteInt(num);
	input.WriteString(buffer);
	DoRPC(input);
}

void trap_GetUserinfo(int num, char *buffer, int bufferSize)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_GET_USERINFO);
	input.WriteInt(num);
	input.WriteInt(bufferSize);
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(buffer, output.ReadString(), bufferSize);
}

void trap_GetServerinfo(char *buffer, int bufferSize)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_GET_SERVERINFO);
	input.WriteInt(bufferSize);
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(buffer, output.ReadString(), bufferSize);
}

void trap_AdjustAreaPortalState(gentity_t *ent, qboolean open)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_ADJUST_AREA_PORTAL_STATE);
	input.WriteInt(ent - g_entities);
	input.WriteInt(open);
	DoRPC(input);
}

qboolean trap_AreasConnected(int area1, int area2)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_AREAS_CONNECTED);
	input.WriteInt(area1);
	input.WriteInt(area2);
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

void trap_GetUsercmd(int clientNum, usercmd_t *cmd)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_GET_USERCMD);
	input.WriteInt(clientNum);
	RPC::Reader output = DoRPC(input);
	output.Read(cmd, sizeof(usercmd_t));
}

qboolean trap_GetEntityToken(char *buffer, int bufferSize)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_GET_ENTITY_TOKEN);
	RPC::Reader output = DoRPC(input);
	qboolean ret = output.ReadInt();
	Q_strncpyz(buffer, output.ReadString(), bufferSize);
	return ret;
}

int trap_GMTime(qtime_t *qtime)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_GM_TIME);
	RPC::Reader output = DoRPC(input);
	qboolean ret = output.ReadInt();
	if (qtime)
		output.Read(qtime, sizeof(qtime_t));
	return ret;
}

void trap_SnapVector(float *v)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_SNAPVECTOR);
	input.Write(v, sizeof(vec3_t));
	RPC::Reader output = DoRPC(input);
	output.Read(v, sizeof(vec3_t));
}

void trap_SendGameStat(const char *data)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_SEND_GAMESTAT);
	input.WriteString(data);
	DoRPC(input);
}

qboolean trap_GetTag(int clientNum, int tagFileNumber, const char *tagName, orientation_t *ori)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_GETTAG);
	input.WriteInt(clientNum);
	input.WriteInt(tagFileNumber);
	input.WriteString(tagName);
	RPC::Reader output = DoRPC(input);
	qboolean ret = output.ReadInt();
	output.Read(ori, sizeof(orientation_t));
	return ret;
}

qboolean trap_LoadTag(const char *filename)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_REGISTERTAG);
	input.WriteString(filename);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
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

int trap_Parse_AddGlobalDefine(const char *define)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_PARSE_ADD_GLOBAL_DEFINE);
	input.WriteString(define);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

int trap_Parse_LoadSource(const char *filename)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_PARSE_LOAD_SOURCE);
	input.WriteString(filename);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

int trap_Parse_FreeSource(int handle)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_PARSE_FREE_SOURCE);
	input.WriteInt(handle);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

int trap_Parse_ReadToken(int handle, pc_token_t *pc_token)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_PARSE_READ_TOKEN);
	input.WriteInt(handle);
	RPC::Reader output = DoRPC(input);
	int ret = output.ReadInt();
	output.Read(pc_token, sizeof(pc_token_t));
	return ret;
}

int trap_Parse_SourceFileAndLine(int handle, char *filename, int *line)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_PARSE_SOURCE_FILE_AND_LINE);
	input.WriteInt(handle);
	RPC::Reader output = DoRPC(input);
	int ret = output.ReadInt();
	Q_strncpyz(filename, output.ReadString(), 128);
	*line = output.ReadInt();
	return ret;
}

void trap_SendMessage(int clientNum, char *buf, int buflen)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_SENDMESSAGE);
	input.WriteInt(buflen);
	input.Write(buf, buflen);
	DoRPC(input);
}

messageStatus_t trap_MessageStatus(int clientNum)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_MESSAGESTATUS);
	input.WriteInt(clientNum);
	RPC::Reader output = DoRPC(input);
	return static_cast<messageStatus_t>(output.ReadInt());
}

int trap_RSA_GenerateMessage(const char *public_key, char *cleartext, char *encrypted)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_RSA_GENMSG);
	input.WriteString(public_key);
	RPC::Reader output = DoRPC(input);
	int ret = output.ReadInt();
	Q_strncpyz(cleartext, output.ReadString(), RSA_STRING_LENGTH);
	Q_strncpyz(encrypted, output.ReadString(), RSA_STRING_LENGTH);
	return ret;
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

void trap_GenFingerprint(const char *pubkey, int size, char *buffer, int bufsize)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_GENFINGERPRINT);
	input.WriteInt(size);
	input.Write(pubkey, size);
	input.WriteInt(bufsize);
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(buffer, output.ReadString(), bufsize);
}

void trap_GetPlayerPubkey(int clientNum, char *pubkey, int size)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_GETPLAYERPUBKEY);
	input.WriteInt(clientNum);
	input.WriteInt(size);
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(pubkey, output.ReadString(), size);
}

void trap_GetTimeString(char *buffer, int size, const char *format, const qtime_t *tm)
{
	RPC::Writer input;
	input.WriteInt(GS_QVM_SYSCALL);
	input.WriteInt(G_GETTIMESTRING);
	input.WriteInt(size);
	input.WriteString(format);
	input.Write(tm, sizeof(qtime_t));
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(buffer, output.ReadString(), size);
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
