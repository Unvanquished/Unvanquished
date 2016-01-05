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

#include "common/Common.h"
#include "common/IPC/Channel.h"

#ifndef VIRTUALMACHINE_H_
#define VIRTUALMACHINE_H_

namespace VM {

/*
 * To better support mods the gamelogic is treated like any other asset and can
 * be downloaded from a server. However it is considered untrusted code so we
 * need to run it in a sandbox. We use NaCl to provide the sandboxing which
 * means that the gamelogic is in another process and that we have to use IPC and
 * shared memory to communicate with it.
 *
 * The gamelogic can be compiled to and ran from several executable formats that
 * will all start at the "main" function whose first job will be to retrieve the
 * root socket handle to communicate with the engine. The different executable
 * formats are:
 *  - A native shared library loaded at runtime and started in the engine process,
 * this shared lib exports a "main" function pointer which is called by the
 * engine, providing a handle to the root socket in argv[1]. Because the gamelogic
 * lives in the same process as the engine, any leaks in the gamelogic will be
 * engine leaks. However because it is in the same process, it is easier to debug
 * as the debugger doesn't automatically attach to child process.
 *  - An NaCl executable started in a new sandboxed process. The "main" function is
 * ran first and the root socket handle is given via an environment variable. This
 * is how the gamelogic is ran when we do not trust the code as it won't be able to
 * access anything apart from the file handles the engine gives it. When the NaCl
 * gamelogic exits the OS will clean up any leaks for us. It is also slightly slower
 * than native format (no SSE and code alignement constraints).
 *  - A native executable started in a new process, obviously the "main" function
 * is the first one ran. The root socket handle is given in argv[1]. It doesn't
 * provide sandboxing like the nacl executable but the OS will still clean up any
 * leak for us.
 *
 * TL;DR
 * - Native DLL: no sandboxing, no cleaning up but debugger support. Use for dev.
 * - NaCl exe: sandboxing, no leaks, slightly slower, hard to debug. Use for regular players.
 * - Native exe: no sandboxing, no leaks, hard to debug. Might be used by server owners for perf.
 */
enum vmType_t {
	// Loads the VM as an nacl executable from the homepath, potentially from a pk3
	// USE THIS BY DEFAULT FOR PROD
	TYPE_NACL,

	// Loads the VM as an nacl executable from the libpath, for freshly compiled NaCl VMs (no need to put it in a pk3)
	TYPE_NACL_LIBPATH,

	// Loads the VM as a native exe from the libpath
	TYPE_NATIVE_EXE,

	// Loads the VM as a native DLL from the libpath
	// USE THIS FOR DEVELOPMENT
	TYPE_NATIVE_DLL,
	TYPE_END
};


struct VMParams {
	VMParams(std::string name)
		: logSyscalls("vm." + name + ".logSyscalls", "dump all the syscalls in the " + name + ".syscallLog file", Cvar::NONE, false),
		  vmType("vm." + name + ".type", "how the vm should be loaded for " + name, Cvar::NONE, TYPE_NACL, 0, TYPE_END - 1),
		  debug("vm." + name + ".debug", "run a gdbserver on localhost:4014 to debug the VM", Cvar::NONE, false),
		  debugLoader("vm." + name + ".debugLoader", "make nacl_loader dump information to " + name + "-nacl_loader.log", Cvar::NONE, 0, 0, 5) {
	}

	Cvar::Cvar<bool> logSyscalls;
	Cvar::Range<Cvar::Cvar<int>> vmType;
	Cvar::Cvar<bool> debug;
	Cvar::Range<Cvar::Cvar<int>> debugLoader;
};

// Base class for a virtual machine instance
class VMBase {
public:
	VMBase(std::string name)
		: processHandle(Sys::INVALID_HANDLE), name(name), params(name) {}

	// Create the VM for the named module. Returns the ABI version reported
	// by the module. This will automatically free any existing VM.
	uint32_t Create();

	// Free the VM
	void Free();

	// Check if the VM is active
	bool IsActive() const
	{
		return Sys::IsValidHandle(processHandle) || inProcess.thread.joinable();
	}

	// Make sure the VM is closed on exit
	~VMBase()
	{
		Free();
	}

	// Send a message to the VM
	template<typename Msg, typename... Args> void SendMsg(Args&&... args)
	{
		// Marking lambda as mutable to work around a bug in gcc 4.6
		LogMessage(false, true, Msg::id);
		IPC::SendMsg<Msg>(rootChannel, [this](uint32_t id, Util::Reader reader) mutable {
			LogMessage(true, true, id);
			Syscall(id, std::move(reader), rootChannel);
			LogMessage(true, false, id);
		}, std::forward<Args>(args)...);
		LogMessage(false, false, Msg::id);
	}

	struct InProcessInfo {
		std::thread thread;
		std::mutex mutex;
		std::condition_variable condition;
		Sys::DynamicLib sharedLib;
		bool running;

		InProcessInfo()
			: running(false) {}
	};

protected:
	// System call handler
	virtual void Syscall(uint32_t id, Util::Reader reader, IPC::Channel& channel) = 0;

private:
	void FreeInProcessVM();

	// Used for the NaCl VMs
	Sys::OSHandle processHandle;

	// Used by the native, in process VMs
	InProcessInfo inProcess;

	// Common
	IPC::Channel rootChannel;

	std::string name;

	vmType_t type;

	VMParams params;

	// Logging the syscalls
	FS::File syscallLogFile;

	void LogMessage(bool vmToEngine, bool start, int id);
};

} // namespace VM

#endif // VIRTUALMACHINE_H_
