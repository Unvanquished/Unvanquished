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

#include "q_shared.h"
#include "qcommon.h"

namespace VM {

static NaCl::Module TryLoad(const char* name, const char* path, const char* game, Type type, int& abiVersion)
{
	NaCl::Module out;
	if (type == TYPE_NATIVE) {
		char exe[MAX_QPATH];
		Com_sprintf(exe, sizeof(exe), "%s%s", name, EXE_EXT);
		out = NaCl::LoadNativeModule(FS_BuildOSPath(path, game, exe));
	} else if (type == TYPE_NACL) {
		char nexe[MAX_QPATH];
		char sel_ldr[MAX_QPATH];
		char irt[MAX_QPATH];
		Com_sprintf(nexe, sizeof(nexe), "%s.nexe", name);
		Com_sprintf(sel_ldr, sizeof(sel_ldr), "%s/sel_ldr%s", path, EXE_EXT);
		Com_sprintf(irt, sizeof(irt), "%s/irt_core.nexe", path);
		out = NaCl::LoadNaClModule(FS_BuildOSPath(path, game, nexe), sel_ldr, irt);
	} else
		Com_Error(ERR_DROP, "Invalid VM type");

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
	const char* basePath = Cvar_VariableString("fs_basepath");
	int abiVersion;

	if (libPath[0])
		module = TryLoad(name, libPath, gameDir, type, abiVersion);
	if (!module && basePath[0])
		module = TryLoad(name, basePath, gameDir, type, abiVersion);

	if (!module)
		Com_Error(ERR_DROP, "Couldn't load VM %s", name);

	return abiVersion;
}

void VMBase::DoRPC(RPC::Writer& writer, RPC::Reader& reader, bool ignoreErrors)
{
	if (!module.GetRootSocket().SendMsg(writer.GetData(), writer.GetSize(), writer.GetHandles(), writer.GetNumHandles())) {
		if (ignoreErrors)
			return;
		Com_Error(ERR_DROP, "Error sending RPC message");
	}

	// Handle syscalls from the remote module until the "return" pseudo-syscall is invoked
	while (true) {
		reader.Reset();
		if (!module.GetRootSocket().RecvMsg(reader.GetDataBuffer(), reader.GetHandlesBuffer()))
			Com_Error(ERR_DROP, "Error recieving RPC message");

		int syscall = reader.ReadInt();
		if (syscall == -1)
			return;

		writer.Reset();
		writer.WriteInt(-1);
		Syscall(syscall, reader, writer);
		if (!module.GetRootSocket().SendMsg(writer.GetData(), writer.GetSize(), writer.GetHandles(), writer.GetNumHandles()))
			Com_Error(ERR_DROP, "Error sending RPC message");
	}
}

} // namespace VM
