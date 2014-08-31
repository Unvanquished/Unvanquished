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

#include "VMMain.h"
#include "CommonProxies.h"

IPC::Channel VM::rootChannel;

class ExitException{};

void VM::Exit() {
  throw ExitException();
}

static IPC::Channel GetRootChannel(int argc, char** argv) {
	const char* channel;
	if (argc == 1) {
		channel = getenv("ROOT_SOCKET");
		if (!channel) {
			fprintf(stderr, "Environment variable ROOT_SOCKET not found\n");
			VM::Exit();
		}
	} else {
		channel = argv[1];
	}

	char* end;
	IPC::OSHandleType h = (IPC::OSHandleType)strtol(channel, &end, 10);
	if (channel == end || *end != '\0') {
		fprintf(stderr, "Environment variable ROOT_SOCKET does not contain a valid handle\n");
		VM::Exit();
	}

	return IPC::Channel(IPC::Socket::FromHandle(h));
}

DLLEXPORT int main(int argc, char** argv) {
	try {
		VM::rootChannel = GetRootChannel(argc, argv);

		// Send syscall ABI version, also acts as a sign that the module loaded
		IPC::Writer writer;
		writer.Write<uint32_t>(VM::VM_API_VERSION);
		VM::rootChannel.SendMsg(writer);

		VM::InitializeProxies();
		VM::VMInit();

		// Start main loop
		while (true) {
			IPC::Reader reader = VM::rootChannel.RecvMsg();
			uint32_t id = reader.Read<uint32_t>();
			VM::VMHandleSyscall(id, std::move(reader));
		}
	} catch (ExitException e) {
		return 0;
	}
}

