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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "VirtualMachine.h"

namespace VM {

static NaCl::Module TryLoad(const char* name, const char* path, const char* game, Type type, int& abiVersion)
{
	bool debug = Cvar_Get("vm_debug", "0", CVAR_INIT)->integer;
	NaCl::Module out;
	if (type == TYPE_NATIVE) {
		char exe[MAX_QPATH];
		Com_sprintf(exe, sizeof(exe), "%s%s", name, EXE_EXT);
		out = NaCl::LoadModule(FS_BuildOSPath(path, game, exe), nullptr, debug);
	} else if (type == TYPE_NACL) {
		char nexe[MAX_QPATH];
		char sel_ldr[MAX_QPATH];
		char irt[MAX_QPATH];
#ifdef __linux__
		char bootstrap[MAX_QPATH];
#else
		const char* bootstrap = nullptr;
#endif
		Com_sprintf(nexe, sizeof(nexe), "%s-%s.nexe", name, ARCH_STRING);
		Com_sprintf(sel_ldr, sizeof(sel_ldr), "%s/sel_ldr%s", path, EXE_EXT);
		Com_sprintf(irt, sizeof(irt), "%s/irt_core-%s.nexe", path, ARCH_STRING);
#ifdef __linux__
		Com_sprintf(bootstrap, sizeof(bootstrap), "%s/nacl_helper_bootstrap%s", path, EXE_EXT);
#endif
		NaCl::LoaderParams params = {sel_ldr, irt, bootstrap};
		out = NaCl::LoadModule(FS_BuildOSPath(path, game, nexe), &params, debug);
	} else
		Com_Error(ERR_DROP, "Invalid VM type");

	if (debug && out)
		Com_Printf("Waiting for GDB connection on localhost:4014\n");

	// Read the ABI version from the root socket.
	// If this fails, we assume the remote process failed to start
	std::vector<char> buffer;
	if (!out.GetRootSocket().RecvMsg(buffer) || buffer.size() != sizeof(int))
		return NaCl::Module();
	abiVersion = *reinterpret_cast<int*>(buffer.data());

	return out;
}

int VMBase::Create(const char* name, Type type)
{
	if (module)
		Com_Error(ERR_FATAL, "Attempting to re-create an already-loaded VM");

	// TODO: Proper interaction with FS code
	const char* gameDir = Cvar_VariableString("fs_game");
	const char* libPath = Cvar_VariableString("fs_libpath");
	bool debug = Cvar_Get("vm_debug", "0", CVAR_INIT)->integer;

	if (type == TYPE_NATIVE) {
		char exe[MAX_QPATH];
		Com_sprintf(exe, sizeof(exe), "%s%s", name, EXE_EXT);
		module = NaCl::LoadModule(FS_BuildOSPath(libPath, gameDir, exe), nullptr, debug);
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
		module = NaCl::LoadModule(FS_BuildOSPath(libPath, gameDir, nexe), &params, debug);
	} else
		Com_Error(ERR_DROP, "Invalid VM type");

	if (!module)
		Com_Error(ERR_DROP, "Couldn't load VM %s", name);

	if (debug)
		Com_Printf("Waiting for GDB connection on localhost:4014\n");

	// Read the ABI version from the root socket.
	// If this fails, we assume the remote process failed to start
	std::vector<char> buffer;
	if (!module.GetRootSocket().RecvMsg(buffer) || buffer.size() != sizeof(int))
		Com_Error(ERR_DROP, "Couldn't load VM %s", name);
	return *reinterpret_cast<int*>(buffer.data());
}

RPC::Reader VMBase::DoRPC(RPC::Writer& writer, bool ignoreErrors)
{
	return RPC::DoRPC(module.GetRootSocket(), writer, ignoreErrors, [this](int index, RPC::Reader& inputs, RPC::Writer& outputs) {
		this->Syscall(index, inputs, outputs);
	});
}

} // namespace VM
