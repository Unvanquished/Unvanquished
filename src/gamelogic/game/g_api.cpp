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

extern "C" {
#include "g_local.h"
}
#include "../../engine/qcommon/nacl.h"
#include "../../engine/qcommon/rpc.h"

static NaCl::RootSocket rootSocket;
static NaCl::IPCHandle shmRegion;
static NaCl::SharedMemoryPtr shmMapping;

// Module RPC entry point
static void VMMain(int index, RPC::Reader& inputs, RPC::Writer& outputs)
{
	switch (index) {
	case GAME_INIT:
	{
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
		qboolean isBot = inputs.ReadInt(); // Amanieu: This seems to be unused?
		const char* denied = ClientConnect(clientNum, firstTime);
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
		ClientCommand(inputs.ReadInt());
		break;

	case GAME_RUN_FRAME:
		G_RunFrame(inputs.ReadInt());
		break;

	case GAME_CONSOLE_COMMAND:
		outputs.WriteInt(ConsoleCommand());
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
		G_Error("VMMain(): unknown game command %i", index);
	}
}

// Wrapper which uses the module root socket and VMMain
static RPC::Reader DoRPC(RPC::Writer& writer)
{
	return RPC::DoRPC(rootSocket, writer, false, VMMain);
}

int main(int argc, char** argv)
{
	rootSocket = NaCl::GetRootSocket();

	// Send syscall ABI version, also acts as a sign that the module loaded
	RPC::Writer writer;
	writer.WriteInt(GAME_ABI_VERSION);

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
	input.WriteInt(G_PRINT);
	input.WriteString(string);
	DoRPC(input);
}

void NORETURN trap_Error(const char *string)
{
	RPC::Writer input;
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
	input.WriteInt(G_MILLISECONDS);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

void trap_Cvar_Register(vmCvar_t *cvar, const char *var_name, const char *value, int flags)
{
	vmCvar_t dummy;
	if (!cvar)
		cvar = &dummy;

	RPC::Writer input;
	input.WriteInt(G_CVAR_REGISTER);
	input.Write(cvar, sizeof(vmCvar_t));
	input.WriteString(var_name);
	input.WriteString(value);
	input.WriteInt(flags);
	RPC::Reader output = DoRPC(input);
	output.Read(cvar, sizeof(vmCvar_t));
}

void trap_Cvar_Update(vmCvar_t *cvar)
{
	RPC::Writer input;
	input.WriteInt(G_CVAR_UPDATE);
	input.Write(cvar, sizeof(vmCvar_t));
	RPC::Reader output = DoRPC(input);
	output.Read(cvar, sizeof(vmCvar_t));
}

void trap_Cvar_Set(const char *var_name, const char *value)
{
	RPC::Writer input;
	input.WriteInt(G_CVAR_SET);
	input.WriteString(var_name);
	input.WriteInt(value != NULL);
	if (value)
		input.WriteString(value);
	DoRPC(input);
}

int trap_Cvar_VariableIntegerValue(const char *var_name)
{
	RPC::Writer input;
	input.WriteInt(G_CVAR_VARIABLE_INTEGER_VALUE);
	input.WriteString(var_name);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize)
{
	RPC::Writer input;
	input.WriteInt(G_CVAR_VARIABLE_STRING_BUFFER);
	input.WriteString(var_name);
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(buffer, output.ReadString(), bufsize);
}

int trap_Argc()
{
	RPC::Writer input;
	input.WriteInt(G_ARGC);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

void trap_Argv(int n, char *buffer, int bufferLength)
{
	RPC::Writer input;
	input.WriteInt(G_ARGV);
	input.WriteInt(n);
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(buffer, output.ReadString(), bufferLength);
}

void trap_SendConsoleCommand(int exec_when, const char *text)
{
	RPC::Writer input;
	input.WriteInt(G_SEND_CONSOLE_COMMAND);
	input.WriteInt(exec_when);
	input.WriteString(text);
	DoRPC(input);
}

int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
	RPC::Writer input;
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
	input.WriteInt(G_FS_READ);
	input.WriteInt(f);
	input.WriteInt(len);
	RPC::Reader output = DoRPC(input);
	output.Read(buffer, len);
}

int trap_FS_Write(const void *buffer, int len, fileHandle_t f)
{
	RPC::Writer input;
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
	input.WriteInt(G_FS_RENAME);
	input.WriteString(from);
	input.WriteString(to);
	DoRPC(input);
}

void trap_FS_FCloseFile(fileHandle_t f)
{
	RPC::Writer input;
	input.WriteInt(G_FS_FCLOSE_FILE);
	input.WriteInt(f);
	DoRPC(input);
}

int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	RPC::Writer input;
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
	RPC::Writer input;
	input.WriteInt(G_LOCATE_GAME_DATA);
	input.WriteHandle(shmRegion);
	input.WriteInt(numGEntities);
	input.WriteInt(sizeofGEntity_t);
	input.WriteInt(sizeofGClient);
	DoRPC(input);
}

void trap_DropClient(int clientNum, const char *reason)
{
	RPC::Writer input;
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
	input.WriteInt(G_SEND_SERVER_COMMAND);
	input.WriteInt(clientNum);
	input.WriteString(text);
	DoRPC(input);
}

void trap_LinkEntity(gentity_t *ent)
{
	RPC::Writer input;
	input.WriteInt(G_LINKENTITY);
	input.WriteInt(ent - g_entities);
	DoRPC(input);
}

void trap_UnlinkEntity(gentity_t *ent)
{
	RPC::Writer input;
	input.WriteInt(G_UNLINKENTITY);
	input.WriteInt(ent - g_entities);
	DoRPC(input);
}

int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxcount)
{
	RPC::Writer input;
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
	input.WriteInt(G_TRACE);
	input.Write(start, sizeof(vec3_t));
	input.Write(mins ? mins : vec3_origin, sizeof(vec3_t));
	input.Write(maxs ? maxs : vec3_origin, sizeof(vec3_t));
	input.Write(end, sizeof(vec3_t));
	input.WriteInt(passEntityNum == -1 ? ENTITYNUM_NONE : passEntityNum);
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
	input.WriteInt(G_TRACECAPSULE);
	input.Write(start, sizeof(vec3_t));
	input.Write(mins ? mins : vec3_origin, sizeof(vec3_t));
	input.Write(maxs ? maxs : vec3_origin, sizeof(vec3_t));
	input.Write(end, sizeof(vec3_t));
	input.WriteInt(passEntityNum == -1 ? ENTITYNUM_NONE : passEntityNum);
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
	input.WriteInt(G_POINT_CONTENTS);
	input.Write(point, sizeof(vec3_t));
	input.WriteInt(passEntityNum);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

void trap_SetBrushModel(gentity_t *ent, const char *name)
{
	RPC::Writer input;
	input.WriteInt(G_SET_BRUSH_MODEL);
	input.WriteInt(ent - g_entities);
	input.WriteString(name);
	DoRPC(input);
}

qboolean trap_InPVS(const vec3_t p1, const vec3_t p2)
{
	RPC::Writer input;
	input.WriteInt(G_IN_PVS);
	input.Write(p1, sizeof(vec3_t));
	input.Write(p2, sizeof(vec3_t));
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

qboolean trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	RPC::Writer input;
	input.WriteInt(G_IN_PVS_IGNORE_PORTALS);
	input.Write(p1, sizeof(vec3_t));
	input.Write(p2, sizeof(vec3_t));
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

void trap_SetConfigstring(int num, const char *string)
{
	RPC::Writer input;
	input.WriteInt(G_SET_CONFIGSTRING);
	input.WriteInt(num);
	input.WriteString(string);
	DoRPC(input);
}

void trap_GetConfigstring(int num, char *buffer, int bufferSize)
{
	RPC::Writer input;
	input.WriteInt(G_GET_CONFIGSTRING);
	input.WriteInt(num);
	input.WriteInt(bufferSize);
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(buffer, output.ReadString(), bufferSize);
}

void trap_SetConfigstringRestrictions(int num, const clientList_t *clientList)
{
	RPC::Writer input;
	input.WriteInt(G_SET_CONFIGSTRING_RESTRICTIONS);
	input.WriteInt(num);
	input.Write(clientList, sizeof(clientList_t));
	DoRPC(input);
}

void trap_SetUserinfo(int num, const char *buffer)
{
	RPC::Writer input;
	input.WriteInt(G_SET_USERINFO);
	input.WriteInt(num);
	input.WriteString(buffer);
	DoRPC(input);
}

void trap_GetUserinfo(int num, char *buffer, int bufferSize)
{
	RPC::Writer input;
	input.WriteInt(G_GET_USERINFO);
	input.WriteInt(num);
	input.WriteInt(bufferSize);
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(buffer, output.ReadString(), bufferSize);
}

void trap_GetServerinfo(char *buffer, int bufferSize)
{
	RPC::Writer input;
	input.WriteInt(G_GET_SERVERINFO);
	input.WriteInt(bufferSize);
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(buffer, output.ReadString(), bufferSize);
}

void trap_AdjustAreaPortalState(gentity_t *ent, qboolean open)
{
	RPC::Writer input;
	input.WriteInt(G_ADJUST_AREA_PORTAL_STATE);
	input.WriteInt(ent - g_entities);
	input.WriteInt(open);
	DoRPC(input);
}

qboolean trap_AreasConnected(int area1, int area2)
{
	RPC::Writer input;
	input.WriteInt(G_AREAS_CONNECTED);
	input.WriteInt(area1);
	input.WriteInt(area2);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

int trap_BotAllocateClient(int clientNum)
{
	RPC::Writer input;
	input.WriteInt(G_BOT_ALLOCATE_CLIENT);
	input.WriteInt(clientNum);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

void trap_BotFreeClient(int clientNum)
{
	RPC::Writer input;
	input.WriteInt(G_BOT_FREE_CLIENT);
	input.WriteInt(clientNum);
	DoRPC(input);
}

int trap_BotGetServerCommand(int clientNum, char *message, int size)
{
	RPC::Writer input;
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
	input.WriteInt(G_GET_USERCMD);
	input.WriteInt(clientNum);
	RPC::Reader output = DoRPC(input);
	output.Read(cmd, sizeof(usercmd_t));
}

qboolean trap_GetEntityToken(char *buffer, int bufferSize)
{
	RPC::Writer input;
	input.WriteInt(G_GET_ENTITY_TOKEN);
	RPC::Reader output = DoRPC(input);
	qboolean ret = output.ReadInt();
	Q_strncpyz(buffer, output.ReadString(), bufferSize);
	return ret;
}

int trap_GMTime(qtime_t *qtime)
{
	RPC::Writer input;
	input.WriteInt(G_GM_TIME);
	RPC::Reader output = DoRPC(input);
	qboolean ret = output.ReadInt();
	output.Read(qtime, sizeof(qtime_t));
	return ret;
}

void trap_SnapVector(float *v)
{
	RPC::Writer input;
	input.WriteInt(G_SNAPVECTOR);
	input.Write(v, sizeof(vec3_t));
	RPC::Reader output = DoRPC(input);
	output.Read(v, sizeof(vec3_t));
}

void trap_SendGameStat(const char *data)
{
	RPC::Writer input;
	input.WriteInt(G_SEND_GAMESTAT);
	input.WriteString(data);
	DoRPC(input);
}

void trap_AddCommand(const char *cmdName)
{
	RPC::Writer input;
	input.WriteInt(G_ADDCOMMAND);
	input.WriteString(cmdName);
	DoRPC(input);
}

void trap_RemoveCommand(const char *cmdName)
{
	RPC::Writer input;
	input.WriteInt(G_REMOVECOMMAND);
	input.WriteString(cmdName);
	DoRPC(input);
}

qboolean trap_GetTag(int clientNum, int tagFileNumber, const char *tagName, orientation_t *ori)
{
	RPC::Writer input;
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
	input.WriteInt(G_REGISTERTAG);
	input.WriteString(filename);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

sfxHandle_t trap_RegisterSound(const char *sample, qboolean compressed)
{
	RPC::Writer input;
	input.WriteInt(G_REGISTERSOUND);
	input.WriteString(sample);
	input.WriteInt(compressed);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

int trap_GetSoundLength(sfxHandle_t sfxHandle)
{
	RPC::Writer input;
	input.WriteInt(G_GET_SOUND_LENGTH);
	input.WriteInt(sfxHandle);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

int trap_Parse_AddGlobalDefine(const char *define)
{
	RPC::Writer input;
	input.WriteInt(G_PARSE_ADD_GLOBAL_DEFINE);
	input.WriteString(define);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

int trap_Parse_LoadSource(const char *filename)
{
	RPC::Writer input;
	input.WriteInt(G_PARSE_LOAD_SOURCE);
	input.WriteString(filename);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

int trap_Parse_FreeSource(int handle)
{
	RPC::Writer input;
	input.WriteInt(G_PARSE_FREE_SOURCE);
	input.WriteInt(handle);
	RPC::Reader output = DoRPC(input);
	return output.ReadInt();
}

int trap_Parse_ReadToken(int handle, pc_token_t *pc_token)
{
	RPC::Writer input;
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
	input.WriteInt(G_PARSE_SOURCE_FILE_AND_LINE);
	input.WriteInt(handle);
	RPC::Reader output = DoRPC(input);
	int ret = output.ReadInt();
	*line = output.ReadInt();
	Q_strncpyz(filename, output.ReadString(), 128);
	return ret;
}

void trap_SendMessage(int clientNum, char *buf, int buflen)
{
	RPC::Writer input;
	input.WriteInt(G_SENDMESSAGE);
	input.WriteInt(buflen);
	input.Write(buf, buflen);
	DoRPC(input);
}

messageStatus_t trap_MessageStatus(int clientNum)
{
	RPC::Writer input;
	input.WriteInt(G_MESSAGESTATUS);
	input.WriteInt(clientNum);
	RPC::Reader output = DoRPC(input);
	return static_cast<messageStatus_t>(output.ReadInt());
}

int trap_RSA_GenerateMessage(const char *public_key, char *cleartext, char *encrypted)
{
	RPC::Writer input;
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
	input.WriteInt(G_QUOTESTRING);
	input.WriteString(str);
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(buffer, output.ReadString(), size);
}

void trap_GenFingerprint(const char *pubkey, int size, char *buffer, int bufsize)
{
	RPC::Writer input;
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
	input.WriteInt(G_GETPLAYERPUBKEY);
	input.WriteInt(clientNum);
	input.WriteInt(size);
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(pubkey, output.ReadString(), size);
}

void trap_GetTimeString(char *buffer, int size, const char *format, const qtime_t *tm)
{
	RPC::Writer input;
	input.WriteInt(G_GETTIMESTRING);
	input.WriteInt(size);
	input.WriteString(format);
	input.Write(tm, sizeof(qtime_t));
	RPC::Reader output = DoRPC(input);
	Q_strncpyz(buffer, output.ReadString(), size);
}
