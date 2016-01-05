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

#ifndef COMMON_VM_SERVICES_H_
#define COMMON_VM_SERVICES_H_

namespace VM {

    class VMBase;

    class CommonVMServices {
        public:
            CommonVMServices(VMBase& vm, Str::StringRef vmName, int commandFlag);
            ~CommonVMServices();

            void Syscall(int major, int minor, Util::Reader reader, IPC::Channel& channel);

        private:
            std::string vmName;
            VMBase& vm;

            VMBase& GetVM();

            // Misc Related
            void HandleMiscSyscall(int minor, Util::Reader& reader, IPC::Channel& channel);

            // Command Related
            void HandleCommandSyscall(int minor, Util::Reader& reader, IPC::Channel& channel);

            void AddCommand(Util::Reader& reader, IPC::Channel& channel);
            void RemoveCommand(Util::Reader& reader, IPC::Channel& channel);
            void EnvPrint(Util::Reader& reader, IPC::Channel& channel);
            void EnvExecuteAfter(Util::Reader& reader, IPC::Channel& channel);

            class ProxyCmd;
            std::unique_ptr<ProxyCmd> commandProxy;
            //TODO use the values to help the VM cache the location of the commands instead of doing a second hashtable lookup
            std::unordered_map<std::string, uint64_t> registeredCommands;

            // Cvar Related
            void HandleCvarSyscall(int minor, Util::Reader& reader, IPC::Channel& channel);

            void RegisterCvar(Util::Reader& reader, IPC::Channel& channel);
            void GetCvar(Util::Reader& reader, IPC::Channel& channel);
            void SetCvar(Util::Reader& reader, IPC::Channel& channel);
            void AddCvarFlags(Util::Reader& reader, IPC::Channel& channel);

            class ProxyCvar;
            std::vector<std::unique_ptr<ProxyCvar>> registeredCvars;

            // Log Related
            void HandleLogSyscall(int minor, Util::Reader& reader, IPC::Channel& channel);

            // Common common QVM syscalls
            void HandleCommonQVMSyscall(int minor, Util::Reader& reader, IPC::Channel& channel);
    };
}

#endif // COMMAND_VM_SERVICES_H_
