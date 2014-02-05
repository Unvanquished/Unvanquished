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
#include "../../common/RPC.h"
#include "../../common/String.h"

#include <unordered_map>

//TODO or CommandService?

#ifndef PROXY_COMMAND_PROXY_H_
#define PROXY_COMMAND_PROXY_H_

namespace VM {
    class VMBase;
}

namespace Cmd {

    class ProxyCmd;

    class CommandProxy {
        public:
            CommandProxy(VM::VMBase* vm, int commandFlag, Str::StringRef vmName);
            ~CommandProxy();

            void Syscall(int index, RPC::Reader& inputs, RPC::Writer& outputs);

            VM::VMBase* GetVM();

        private:
            void AddCommand(RPC::Reader& inputs, RPC::Writer& outputs);
            void RemoveCommand(RPC::Reader& inputs, RPC::Writer& outputs);

            void EnvPrint(RPC::Reader& inputs, RPC::Writer& outputs);
            void EnvExecuteAfter(RPC::Reader& inputs, RPC::Writer& outputs);

            int flag;
            Str::StringRef vmName;
            std::unordered_map<std::string, uint64_t> registeredCommands; // values not used for now

            ProxyCmd* proxy;
            VM::VMBase* vm;
    };
}
#endif // PROXY_COMMAND_PROXY_H_
