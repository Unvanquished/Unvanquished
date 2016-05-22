/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
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

#include <engine/server/sg_msgdef.h>
#include <shared/VMMain.h>

IPC::SharedMemory shmRegion;

// Definition of the VM->Engine calls

// The actual shared memory region is handled in this file, and is pretty much invisible to the rest of the code
void trap_LocateGameData(int numGEntities, int sizeofGEntity_t, playerState_t*, int sizeofGClient)
{
    static bool firstTime = true;
    if (firstTime) {
        VM::SendMsg<LocateGameDataMsg1>(shmRegion, numGEntities, sizeofGEntity_t, sizeofGClient);
        firstTime = false;
    } else
        VM::SendMsg<LocateGameDataMsg2>(numGEntities, sizeofGEntity_t, sizeofGClient);
}

void trap_DropClient(int clientNum, const char *reason)
{
    VM::SendMsg<DropClientMsg>(clientNum, reason);
}

void trap_SendServerCommand(int clientNum, const char *text)
{
    if (strlen(text) > 1022) {
        Log::Notice("trap_SendServerCommand( %d, ... ) length exceeds 1022.\n", clientNum);
        Log::Notice("text [%.950s]... truncated\n", text);
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
    std::string res;
    VM::SendMsg<GetConfigStringMsg>(num, bufferSize, res);
    Q_strncpyz(buffer, res.c_str(), bufferSize);
}

void trap_SetConfigstringRestrictions(int, const clientList_t*)
{
    VM::SendMsg<SetConfigStringRestrictionsMsg>(); // not implemented
}

void trap_SetUserinfo(int num, const char *buffer)
{
    VM::SendMsg<SetUserinfoMsg>(num, buffer);
}

void trap_GetUserinfo(int num, char *buffer, int bufferSize)
{
    std::string res;
    VM::SendMsg<GetUserinfoMsg>(num, bufferSize, res);
    Q_strncpyz(buffer, res.c_str(), bufferSize);
}

void trap_GetServerinfo(char *buffer, int bufferSize)
{
    std::string res;
    VM::SendMsg<GetServerinfoMsg>(bufferSize, res);
    Q_strncpyz(buffer, res.c_str(), bufferSize);
}

void trap_GetUsercmd(int clientNum, usercmd_t *cmd)
{
    VM::SendMsg<GetUsercmdMsg>(clientNum, *cmd);
}

bool trap_GetEntityToken(char *buffer, int bufferSize)
{
    std::string text;
    bool res;
    VM::SendMsg<GetEntityTokenMsg>(res, text);
    Q_strncpyz(buffer, text.c_str(), bufferSize);
    return res;
}

void trap_SendGameStat(const char *data)
{
    VM::SendMsg<SendGameStatMsg>(data);
}

void trap_SendMessage(int clientNum, uint8_t *buf, size_t buflen)
{
    std::vector<uint8_t> buffer(buflen, 0);
    memcpy(buffer.data(), buf, buflen);
    VM::SendMsg<SendMessageMsg>(clientNum, buflen, buffer);
}

messageStatus_t trap_MessageStatus(int clientNum)
{
    messageStatus_t res;
    VM::SendMsg<MessageStatusMsg>(clientNum, res);
    return res;
}

int trap_RSA_GenerateMessage(const char *public_key, char *cleartext, char *encrypted)
{
    std::string cleartext2, encrypted2;
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
    std::string fingerprint;
    VM::SendMsg<GenFingerprintMsg>(size, pubkey2, bufsize, fingerprint);
    Q_strncpyz(buffer, fingerprint.c_str(), bufsize);
}

void trap_GetPlayerPubkey(int clientNum, char *pubkey, int size)
{
    std::string pubkey2;
    VM::SendMsg<GetPlayerPubkeyMsg>(clientNum, size, pubkey2);
    Q_strncpyz(pubkey, pubkey2.c_str(), size);
}

void trap_GetTimeString(char *buffer, int size, const char *format, const qtime_t *tm)
{
    std::string text;
    VM::SendMsg<GetTimeStringMsg>(size, format, *tm, text);
    Q_strncpyz(buffer, text.c_str(), size);
}

void trap_QuoteString(const char *str, char *buffer, int size)
{
    Q_strncpyz(buffer, Cmd::Escape(str).c_str(), size);
}

int trap_BotAllocateClient()
{
    int res;
    VM::SendMsg<BotAllocateClientMsg>(res);
    return res;
}

void trap_BotFreeClient(int clientNum)
{
    VM::SendMsg<BotFreeClientMsg>(clientNum);
}

int trap_BotGetServerCommand(int clientNum, char *message, int size)
{
    int res;
    std::string message2;
    VM::SendMsg<BotGetConsoleMessageMsg>(clientNum, size, res, message2);
    Q_strncpyz(message, message2.c_str(), size);
    return res;
}

bool trap_BotSetupNav(const botClass_t *botClass, qhandle_t *navHandle)
{
    int res;
    VM::SendMsg<BotNavSetupMsg>(*botClass, res, *navHandle);
    return res;
}

void trap_BotShutdownNav()
{
    VM::SendMsg<BotNavShutdownMsg>();
}

void trap_BotSetNavMesh(int botClientNum, qhandle_t navHandle)
{
    VM::SendMsg<BotSetNavmeshMsg>(botClientNum, navHandle);
}

bool trap_BotFindRoute(int botClientNum, const botRouteTarget_t *target, bool allowPartial)
{
    int res;
    VM::SendMsg<BotFindRouteMsg>(botClientNum, *target, allowPartial, res);
    return res;
}

bool trap_BotUpdatePath(int botClientNum, const botRouteTarget_t *target, botNavCmd_t *cmd)
{
    VM::SendMsg<BotUpdatePathMsg>(botClientNum, *target, *cmd);
    return 0; // Amanieu: This always returns 0, but the value isn't used
}

bool trap_BotNavTrace(int botClientNum, botTrace_t *botTrace, const vec3_t start, const vec3_t end)
{
    std::array<float, 3> start2, end2;
    VectorCopy(start, start2.data());
    VectorCopy(end, end2.data());
    int res;
    VM::SendMsg<BotNavRaycastMsg>(botClientNum, start2, end2, res, *botTrace);
    return res;
}

void trap_BotFindRandomPoint(int botClientNum, vec3_t point)
{
    std::array<float, 3> point2;
    VM::SendMsg<BotNavRandomPointMsg>(botClientNum, point2);
    VectorCopy(point2.data(), point);
}

bool trap_BotFindRandomPointInRadius(int botClientNum, const vec3_t origin, vec3_t point, float radius)
{
    std::array<float, 3> point2, origin2;
    VectorCopy(origin, origin2.data());
    int res;
    VM::SendMsg<BotNavRandomPointRadiusMsg>(botClientNum, origin2, radius, res, point2);
    VectorCopy(point2.data(), point);
    return res;
}

void trap_BotEnableArea(const vec3_t origin, const vec3_t mins, const vec3_t maxs)
{
    std::array<float, 3> origin2, mins2, maxs2;
    VectorCopy(origin, origin2.data());
    VectorCopy(mins, mins2.data());
    VectorCopy(maxs, maxs2.data());
    VM::SendMsg<BotEnableAreaMsg>(origin2, mins2, maxs2);
}

void trap_BotDisableArea(const vec3_t origin, const vec3_t mins, const vec3_t maxs)
{
    std::array<float, 3> origin2, mins2, maxs2;
    VectorCopy(origin, origin2.data());
    VectorCopy(mins, mins2.data());
    VectorCopy(maxs, maxs2.data());
    VM::SendMsg<BotDisableAreaMsg>(origin2, mins2, maxs2);
}

void trap_BotAddObstacle(const vec3_t mins, const vec3_t maxs, qhandle_t *handle)
{
    std::array<float, 3> mins2, maxs2;
    VectorCopy(mins, mins2.data());
    VectorCopy(maxs, maxs2.data());
    VM::SendMsg<BotAddObstacleMsg>(mins2, maxs2, *handle);
}

void trap_BotRemoveObstacle(qhandle_t handle)
{
    VM::SendMsg<BotRemoveObstacleMsg>(handle);
}

void trap_BotUpdateObstacles()
{
    VM::SendMsg<BotUpdateObstaclesMsg>();
}
