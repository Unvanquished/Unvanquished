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

//TODO: error handling?
int VMBase::Create(Str::StringRef name, Type type)
{
	if (module) {
		Com_Printf(S_ERROR "Attempting to re-create an already-loaded VM");
		return -1;
	}

	const std::string& libPath = FS::GetLibPath();
	bool debug = Cvar_Get("vm_debug", "0", CVAR_INIT)->integer;

	if (type == TYPE_NATIVE) {
		std::string exe = FS::Path::Build(libPath, name + EXE_EXT);
		Com_Printf("Trying to load native module %s\n", exe.c_str());
		module = NaCl::LoadModule(exe.c_str(), nullptr, debug);
	} else if (type == TYPE_NACL) {
		// Extract the nexe from the pak so that sel_ldr can load it
		std::string nexe = name + "-" ARCH_STRING ".nexe";
		try {
			FS::File out = FS::HomePath::OpenWrite(nexe);
			FS::PakPath::CopyFile(nexe, out);
			out.Close();
		} catch (std::system_error& err) {
			Com_Printf(S_ERROR "Failed to extract NaCl module %s: %s\n", nexe.c_str(), err.what());
			return -1;
		}
		std::string sel_ldr = FS::Path::Build(libPath, "sel_ldr" EXE_EXT);
		std::string irt = FS::Path::Build(libPath, "irt_core-" ARCH_STRING ".nexe");
#ifdef __linux__
		std::string bootstrap = FS::Path::Build(libPath, "nacl_helper_bootstrap");
		NaCl::LoaderParams params = {sel_ldr.c_str(), irt.c_str(), bootstrap.c_str()};
#else
		NaCl::LoaderParams params = {sel_ldr.c_str(), irt.c_str(), nullptr};
#endif
		Com_Printf("Trying to load NaCl module %s\n", nexe.c_str());
		module = NaCl::LoadModule(FS::Path::Build(FS::GetHomePath(), nexe).c_str(), &params, debug);
	} else {
		Com_Printf(S_ERROR "Invalid VM type");
		return -1;
	}

	if (!module) {
		Com_Printf(S_ERROR "Couldn't load VM %s", name.c_str());
		return -1;
	}

	if (debug)
		Com_Printf("Waiting for GDB connection on localhost:4014\n");

	// Read the ABI version from the root socket.
	// If this fails, we assume the remote process failed to start
	std::vector<char> buffer;
	if (!module.GetRootSocket().RecvMsg(buffer) || buffer.size() != sizeof(int)) {
		Com_Printf(S_ERROR "The '%s' VM did not start.", name.c_str());
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
