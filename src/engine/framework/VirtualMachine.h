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

#ifndef VIRTUALMACHINE_H_
#define VIRTUALMACHINE_H_

namespace VM {

enum vmType_t {
	TYPE_NACL,
	TYPE_NACL_DEBUG,
	TYPE_NATIVE_EXE,
	TYPE_NATIVE_EXE_DEBUG,
	TYPE_NATIVE_DLL,
	TYPE_NACL_LIBPATH,
	TYPE_NACL_LIBPATH_DEBUG,
	TYPE_END
};

struct VMParams {
	VMParams(std::string name)
		: logSyscalls("vm." + name + ".logSyscalls", "dump all the syscalls in the " + name + ".syscallLog file", Cvar::NONE, false),
		  vmType("vm." + name + ".type", "how the vm should be loaded for " + name, Cvar::NONE, TYPE_NACL, 0, TYPE_END - 1),
		  debugLoader("vm." + name + ".debugLoader", "make sel_ldr dump information to " + name + "-sel_ldr.log", Cvar::NONE, 0, 0, 5) {
	}

	Cvar::Cvar<bool> logSyscalls;
	Cvar::Range<Cvar::Cvar<int>> vmType;
	Cvar::Range<Cvar::Cvar<int>> debugLoader;
};

// Base class for a virtual machine instance
class VMBase {
public:
	VMBase(std::string name, VMParams& params)
		: processHandle(Sys::INVALID_HANDLE), name(name), params(params) {}

	// Create the VM for the named module. Returns the ABI version reported
	// by the module.
	int Create();

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
        LogMessage(false, Msg::id);
		IPC::SendMsg<Msg>(rootChannel, [this](uint32_t id, IPC::Reader reader) mutable {
			Syscall(id, std::move(reader), rootChannel);
            LogMessage(true, id);
		}, std::forward<Args>(args)...);
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
	virtual void Syscall(uint32_t id, IPC::Reader reader, IPC::Channel& channel) = 0;

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

	VMParams& params;

	// Logging the syscalls
	FS::File syscallLogFile;

	void LogMessage(bool vmToEngine, int id);
};

} // namespace VM

#endif // VIRTUALMACHINE_H_
