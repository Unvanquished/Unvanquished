/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "VirtualMachine.h"

namespace VM {

//TODO: error handling?
int VMBase::Create(const char* name, Type type)
{
	if (module) {
		Com_Printf(S_ERROR "Attempting to re-create an already-loaded VM");
		return -1;
	}

	// TODO: Proper interaction with FS code
	const char* gameDir = Cvar_VariableString("fs_game");
	const char* libPath = Cvar_VariableString("fs_libpath");
	bool debug = Cvar_Get("vm_debug", "0", CVAR_INIT)->integer;

	if (type == TYPE_NATIVE) {
		char exe[MAX_QPATH];
		Com_sprintf(exe, sizeof(exe), "%s%s", name, EXE_EXT);
		const char* path = FS_BuildOSPath(libPath, gameDir, exe);
		Com_Printf("Trying to load native module %s\n", path);
		module = NaCl::LoadModule(path, nullptr, debug);
	} else if (type == TYPE_NACL) {
		char nexe[MAX_QPATH];
		char sel_ldr[MAX_QPATH];
		char irt[MAX_QPATH];
#ifdef __linux__
		char bootstrap[MAX_QPATH];
		Com_sprintf(bootstrap, sizeof(bootstrap), "%s/nacl_helper_bootstrap%s", libPath, EXE_EXT);
#else
		const char* bootstrap = nullptr;
#endif
		Com_sprintf(nexe, sizeof(nexe), "%s-%s.nexe", name, ARCH_STRING);
		Com_sprintf(sel_ldr, sizeof(sel_ldr), "%s/sel_ldr%s", libPath, EXE_EXT);
		Com_sprintf(irt, sizeof(irt), "%s/irt_core-%s.nexe", libPath, ARCH_STRING);
		NaCl::LoaderParams params = {sel_ldr, irt, bootstrap};
		const char* path = FS_BuildOSPath(libPath, gameDir, nexe);
		Com_Printf("Trying to load NaCl module %s\n", path);
		module = NaCl::LoadModule(path, &params, debug);
	} else {
		Com_Printf(S_ERROR "Invalid VM type");
		return -1;
	}

	if (!module) {
		Com_Printf(S_ERROR "Couldn't load VM %s", name);
		return -1;
	}

	if (debug)
		Com_Printf("Waiting for GDB connection on localhost:4014\n");

	// Read the ABI version from the root socket.
	// If this fails, we assume the remote process failed to start
	std::vector<char> buffer;
	if (!module.GetRootSocket().RecvMsg(buffer) || buffer.size() != sizeof(int)) {
		Com_Printf(S_ERROR "The '%s' VM did not start.", name);
		return -1;
	}
	Com_Printf("Loaded module with the NaCl ABI");
	return *reinterpret_cast<int*>(buffer.data());
}

RPC::Reader VMBase::DoRPC(RPC::Writer& writer, bool ignoreErrors)
{
	return RPC::DoRPC(module.GetRootSocket(), writer, ignoreErrors, [this](int index, RPC::Reader& inputs, RPC::Writer& outputs) {
		this->Syscall(index, inputs, outputs);
	});
}

} // namespace VM
