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

#include "CommandProxy.h"
#include "../framework/CommandSystem.h"
#include "../framework/VirtualMachine.h"
#include "../../common/Log.h"

//TODO
#include "../server/g_api.h"

namespace Cmd {

    class ProxyCmd: public Cmd::CmdBase {
        public:
            ProxyCmd(CommandProxy& cmdProxy, int flag): Cmd::CmdBase(flag), cmdProxy(cmdProxy) {
            }

            virtual void Run(const Args& args) const {
                RPC::Writer inputs;
                inputs.WriteInt(GS_COMMAND);
                inputs.WriteInt(EXECUTE);
                inputs.WriteString(args.EscapedArgs(0).c_str());
                cmdProxy.GetVM()->DoRPC(inputs);
            }

            virtual Cmd::CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const {
                return {};
            }

        private:
            CommandProxy& cmdProxy;
    };

    CommandProxy::CommandProxy(VM::VMBase* vm, int commandFlag, Str::StringRef vmName)
    :flag(commandFlag), vmName(vmName), proxy(new ProxyCmd(*this, flag)), vm(vm) {
    }

    CommandProxy::~CommandProxy() {
        //FIXME or iterate over the commands we registered, or add Cmd::RemoveByProxy()
        Cmd::RemoveFlaggedCommands(flag);
        delete proxy;
    }

    void CommandProxy::Syscall(int index, RPC::Reader& inputs, RPC::Writer& outputs) {
        switch(index) {
            case ADD_COMMAND:
                AddCommand(inputs, outputs);
                break;

            case REMOVE_COMMAND:
                RemoveCommand(inputs, outputs);
                break;

            case ENV_PRINT:
                EnvPrint(inputs, outputs);
                break;

            case ENV_EXECUTE_AFTER:
                EnvExecuteAfter(inputs, outputs);
                break;

            default:
                Com_Error(ERR_DROP, "Bad command syscall number '%d' for VM '%s'", index, vmName.c_str());
        }
    }

    VM::VMBase* CommandProxy::GetVM() {
        return vm;
    }

    void CommandProxy::AddCommand(RPC::Reader& inputs, RPC::Writer& outputs) {
        Str::StringRef name = inputs.ReadString();
        Str::StringRef description = inputs.ReadString();

        if (Cmd::CommandExists(name)) {
            //TODO
            Log::Warn("VM '%s' tried to register command '%s' which is already registered", vmName, name);
            return;
        }

        //TODO
        Cmd::AddCommand(name, *proxy, description);
        registeredCommands[name] = 0;
    }

    void CommandProxy::RemoveCommand(RPC::Reader& inputs, RPC::Writer& outputs) {
        Str::StringRef name = inputs.ReadString();

        if (registeredCommands.find(name) != registeredCommands.end()) {
            Cmd::RemoveCommand(name);
        }
    }

    void CommandProxy::EnvPrint(RPC::Reader& inputs, RPC::Writer& outputs) {
        //TODO allow it only if we are in a command?
        Str::StringRef line = inputs.ReadString();

        Cmd::GetEnv()->Print(line);
    }

    void CommandProxy::EnvExecuteAfter(RPC::Reader& inputs, RPC::Writer& outputs) {
        //TODO check that it isn't sending /quit or other bad commands (/lua "rootkit()")?
        Str::StringRef commandText = inputs.ReadString();
        int parseCvars = inputs.ReadInt();

        Cmd::GetEnv()->ExecuteAfter(commandText, parseCvars);
    }

}
