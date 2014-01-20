/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2014, Daemon Developers
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
