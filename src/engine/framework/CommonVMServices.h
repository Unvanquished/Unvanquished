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

#include "../../common/Command.h"
#include "../../common/IPC.h"
#include "../../common/String.h"

#include <unordered_map>

#ifndef COMMON_VM_SERVICES_H_
#define COMMON_VM_SERVICES_H_

namespace VM {

    class VMBase;

    class CommonVMServices {
        public:
            CommonVMServices(VMBase* vm, Str::StringRef vmName, int commandFlag);
            ~CommonVMServices();

            void Syscall(int major, int minor, IPC::Reader reader, const IPC::Socket& socket);

        private:
            Str::StringRef vmName;
            VMBase* vm;

            VMBase* GetVM();

            // Command Related
            void HandleCommandSyscall(int minor, IPC::Reader& reader, const IPC::Socket& socket);

            void AddCommand(IPC::Reader& reader, const IPC::Socket& socket);
            void RemoveCommand(IPC::Reader& reader, const IPC::Socket& socket);
            void EnvPrint(IPC::Reader& reader, const IPC::Socket& socket);
            void EnvExecuteAfter(IPC::Reader& reader, const IPC::Socket& socket);

            class ProxyCmd;
            int commandFlag;
            ProxyCmd* commandProxy;
            //TODO use the values to help the VM cache the location of the commands instead of doing a second hashtable lookup
            std::unordered_map<std::string, uint64_t> registeredCommands;

            // Cvar Related
            void HandleCvarSyscall(int minor, IPC::Reader& reader, const IPC::Socket& socket);

            void RegisterCvar(IPC::Reader& reader, const IPC::Socket& socket);
            void GetCvar(IPC::Reader& reader, const IPC::Socket& socket);
            void SetCvar(IPC::Reader& reader, const IPC::Socket& socket);

            class ProxyCvar;
            std::vector<ProxyCvar*> registeredCvars;
    };
}

#endif // COMMAND_VM_SERVICES_H_
