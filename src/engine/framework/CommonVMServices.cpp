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

#include "CommonVMServices.h"
#include "../framework/CommandSystem.h"
#include "../framework/VirtualMachine.h"
#include "../../common/Log.h"

//TODO
#include "../server/g_api.h"

namespace VM {

    // Command Related

    void CommonVMServices::HandleCommandSyscall(int minor, RPC::Reader& inputs, RPC::Writer& outputs) {
        switch(minor) {
            case Cmd::ADD_COMMAND:
                AddCommand(inputs, outputs);
                break;

            case Cmd::REMOVE_COMMAND:
                RemoveCommand(inputs, outputs);
                break;

            case Cmd::ENV_PRINT:
                EnvPrint(inputs, outputs);
                break;

            case Cmd::ENV_EXECUTE_AFTER:
                EnvExecuteAfter(inputs, outputs);
                break;

            default:
                Com_Error(ERR_DROP, "Bad command syscall number '%d' for VM '%s'", minor, vmName.c_str());
        }
    }

    class CommonVMServices::ProxyCmd: public Cmd::CmdBase {
        public:
            ProxyCmd(CommonVMServices& services, int flag): Cmd::CmdBase(flag), services(services) {
            }
            virtual ~ProxyCmd() {
            }

            virtual void Run(const Cmd::Args& args) const {
                RPC::Writer inputs;
                inputs.WriteInt(GS_COMMAND);
                inputs.WriteInt(Cmd::EXECUTE);
                inputs.WriteString(args.EscapedArgs(0).c_str());
                services.GetVM()->DoRPC(inputs);
            }

            virtual Cmd::CompletionResult Complete(int argNum, const Cmd::Args& args, Str::StringRef prefix) const {
                RPC::Writer inputs;
                inputs.WriteInt(GS_COMMAND);
                inputs.WriteInt(Cmd::COMPLETE);
                inputs.WriteInt(argNum);
                inputs.WriteString(args.EscapedArgs(0).c_str());
                inputs.WriteString(prefix.c_str());

                RPC::Reader outputs = services.GetVM()->DoRPC(inputs);

                int resSize = outputs.ReadInt();

                Cmd::CompletionResult res;
                res.reserve(resSize);

                for (int i = 0; i < resSize; i++) {
                    Str::StringRef name = outputs.ReadString();
                    Str::StringRef desc = outputs.ReadString();
                    res.emplace_back(name, desc);
                }

                return res;
            }

        private:
            CommonVMServices& services;
    };

    void CommonVMServices::AddCommand(RPC::Reader& inputs, RPC::Writer& outputs) {
        Str::StringRef name = inputs.ReadString();
        Str::StringRef description = inputs.ReadString();

        if (Cmd::CommandExists(name)) {
            Log::Warn("VM '%s' tried to register command '%s' which is already registered", vmName, name);
            return;
        }

        Cmd::AddCommand(name, *commandProxy, description);
        registeredCommands[name] = 0;
    }

    void CommonVMServices::RemoveCommand(RPC::Reader& inputs, RPC::Writer& outputs) {
        Str::StringRef name = inputs.ReadString();

        if (registeredCommands.find(name) != registeredCommands.end()) {
            Cmd::RemoveCommand(name);
        }
    }

    void CommonVMServices::EnvPrint(RPC::Reader& inputs, RPC::Writer& outputs) {
        //TODO allow it only if we are in a command?
        Str::StringRef line = inputs.ReadString();

        Cmd::GetEnv()->Print(line);
    }

    void CommonVMServices::EnvExecuteAfter(RPC::Reader& inputs, RPC::Writer& outputs) {
        //TODO check that it isn't sending /quit or other bad commands (/lua "rootkit()")?
        Str::StringRef commandText = inputs.ReadString();
        int parseCvars = inputs.ReadInt();

        Cmd::GetEnv()->ExecuteAfter(commandText, parseCvars);
    }

    // Cvar related

    class CommonVMServices::ProxyCvar : public Cvar::CvarProxy {
        public:
            ProxyCvar(CommonVMServices* services, std::string name, std::string description, int flags, std::string defaultValue)
            :CvarProxy(std::move(name), std::move(description), flags, std::move(defaultValue)), services(services) {
                Register();
            }
            virtual ~ProxyCvar() {
            }

            virtual Cvar::OnValueChangedResult OnValueChanged(Str::StringRef newValue) OVERRIDE {
                RPC::Writer inputs;
                inputs.WriteInt(GS_CVAR);
                inputs.WriteInt(Cvar::ON_VALUE_CHANGED);
                inputs.WriteString(name.c_str());
                inputs.WriteString(newValue.c_str());

                RPC::Reader outputs = services->GetVM()->DoRPC(inputs);

                bool success = outputs.ReadInt();
                std::string description = outputs.ReadString();

                return {success, description};
            }

        private:
            CommonVMServices* services;
    };

    void CommonVMServices::HandleCvarSyscall(int minor, RPC::Reader& inputs, RPC::Writer& outputs) {
        switch(minor) {
            case Cvar::REGISTER_CVAR:
                RegisterCvar(inputs, outputs);
                break;

            case Cvar::GET_CVAR:
                GetCvar(inputs, outputs);
                break;

            case Cvar::SET_CVAR:
                SetCvar(inputs, outputs);
                break;

            default:
                Com_Error(ERR_DROP, "Bad cvar syscall number '%d' for VM '%s'", minor, vmName.c_str());
        }
    }

    void CommonVMServices::RegisterCvar(RPC::Reader& inputs, RPC::Writer& outputs) {
        Str::StringRef name = inputs.ReadString();
        Str::StringRef description = inputs.ReadString();
        int flags = inputs.ReadInt();
        Str::StringRef defaultValue = inputs.ReadString();

        // The registration of the cvar is made automatically when it is created
        registeredCvars.push_back(new ProxyCvar(this, name, description, flags, defaultValue));
    }

    void CommonVMServices::GetCvar(RPC::Reader& inputs, RPC::Writer& outputs) {
        //TODO check it is only looking at allowed cvars?
        Str::StringRef name = inputs.ReadString();

        outputs.WriteString(Cvar::GetValue(name).c_str());
    }

    void CommonVMServices::SetCvar(RPC::Reader& inputs, RPC::Writer& outputs) {
        //TODO check it is only touching allowed cvars?
        Str::StringRef name = inputs.ReadString();
        Str::StringRef value = inputs.ReadString();

        Cvar::SetValue(name, value);
    }

    // Misc, Dispatch

    CommonVMServices::CommonVMServices(VM::VMBase* vm, Str::StringRef vmName, int commandFlag)
    :vmName(vmName), vm(vm), commandFlag(commandFlag), commandProxy(new ProxyCmd(*this, commandFlag)) {
    }

    CommonVMServices::~CommonVMServices() {
        //FIXME or iterate over the commands we registered, or add Cmd::RemoveByProxy()
        Cmd::RemoveFlaggedCommands(commandFlag);
        delete commandProxy;

        for (auto cvar: registeredCvars) {
            delete cvar;
        }
    }

    void CommonVMServices::Syscall(int major, int minor, RPC::Reader& inputs, RPC::Writer& outputs) {
        switch (major) {
            case GS_COMMAND:
                HandleCommandSyscall(minor, inputs, outputs);
                break;

            case GS_CVAR:
                HandleCvarSyscall(minor, inputs, outputs);
                break;

            default:
                Com_Error(ERR_DROP, "Unhandled common engine syscall major number %i", major);
        }
    }

    VM::VMBase* CommonVMServices::GetVM() {
        return vm;
    }
}
