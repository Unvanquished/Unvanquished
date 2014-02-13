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

#ifndef VIRTUALMACHINE_H_
#define VIRTUALMACHINE_H_

#include "../../common/IPC.h"
#include "../../common/String.h"

namespace VM {

enum vmType_t {
	TYPE_NATIVE,
	TYPE_NACL
};

// Base class for a virtual machine instance
class VMBase {
public:
	VMBase()
		: processHandle(IPC::INVALID_HANDLE) {}

	// Create the VM for the named module. Returns the ABI version reported
	// by the module.
	int Create(Str::StringRef name, vmType_t type);

	// Free the VM
	void Free();

	// Check if the VM is active
	bool IsActive() const
	{
		return processHandle != IPC::INVALID_HANDLE;
	}

	// Make sure the VM is closed on exit
	~VMBase()
	{
		Free();
	}

	// Perform an RPC call with the given inputs, returns results in output
	IPC::Reader DoRPC(const IPC::Writer& writer) const
	{
		return IPC::DoRPC(rootSocket, writer, [this](uint32_t id, IPC::Reader reader) {
			Syscall(id, std::move(reader), rootSocket);
		});
	}

protected:
	// System call handler
	virtual void Syscall(uint32_t id, IPC::Reader reader, const IPC::Socket& socket) const = 0;

private:
	IPC::OSHandleType processHandle;
	IPC::Socket rootSocket;
};

} // namespace VM

#endif // VIRTUALMACHINE_H_
