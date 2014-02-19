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

#include "../../common/CommonSyscalls.h"

//TODO
#include "../server/g_api.h"

namespace VM {

    // Command Related

    void CommonVMServices::HandleCommandSyscall(int minor, IPC::Reader& reader, const IPC::Socket& socket) {
        switch(minor) {
            case ADD_COMMAND:
                AddCommand(reader, socket);
                break;

            case REMOVE_COMMAND:
                RemoveCommand(reader, socket);
                break;

            case ENV_PRINT:
                EnvPrint(reader, socket);
                break;

            case ENV_EXECUTE_AFTER:
                EnvExecuteAfter(reader, socket);
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
                services.GetVM().SendMsg<ExecuteMsg>(args.EscapedArgs(0));
            }

            virtual Cmd::CompletionResult Complete(int argNum, const Cmd::Args& args, Str::StringRef prefix) const {
                Cmd::CompletionResult res;
                services.GetVM().SendMsg<CompleteMsg>(argNum, args.EscapedArgs(0), prefix, res);
                return res;
            }

        private:
            CommonVMServices& services;
    };

    void CommonVMServices::AddCommand(IPC::Reader& reader, const IPC::Socket& socket) {
        IPC::HandleMsg<AddCommandMsg>(socket, std::move(reader), [this](std::string name, std::string description){
            if (Cmd::CommandExists(name)) {
                Log::Warn("VM '%s' tried to register command '%s' which is already registered", vmName, name);
                return;
            }

            Cmd::AddCommand(name, *commandProxy, description);
            registeredCommands[name] = 0;
        });
    }

    void CommonVMServices::RemoveCommand(IPC::Reader& reader, const IPC::Socket& socket) {
        IPC::HandleMsg<RemoveCommandMsg>(socket, std::move(reader), [this](std::string name){
            if (registeredCommands.find(name) != registeredCommands.end()) {
                Cmd::RemoveCommand(name);
            }
        });
    }

    void CommonVMServices::EnvPrint(IPC::Reader& reader, const IPC::Socket& socket) {
        IPC::HandleMsg<EnvPrintMsg>(socket, std::move(reader), [this](std::string line){
            //TODO allow it only if we are in a command?
            Cmd::GetEnv()->Print(line);
        });
    }

    void CommonVMServices::EnvExecuteAfter(IPC::Reader& reader, const IPC::Socket& socket) {
        IPC::HandleMsg<EnvExecuteAfterMsg>(socket, std::move(reader), [this](std::string commandText, bool parseCvars){
            //TODO check that it isn't sending /quit or other bad commands (/lua "rootkit()")?
            Cmd::GetEnv()->ExecuteAfter(commandText, parseCvars);
        });
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
                Cvar::OnValueChangedResult result;
                services->GetVM().SendMsg<OnValueChangedMsg>(name, newValue, result.success, result.description);
                return result;
            }

        private:
            CommonVMServices* services;
    };

    void CommonVMServices::HandleCvarSyscall(int minor, IPC::Reader& reader, const IPC::Socket& socket) {
        switch(minor) {
            case REGISTER_CVAR:
                RegisterCvar(reader, socket);
                break;

            case GET_CVAR:
                GetCvar(reader, socket);
                break;

            case SET_CVAR:
                SetCvar(reader, socket);
                break;

            default:
                Com_Error(ERR_DROP, "Bad cvar syscall number '%d' for VM '%s'", minor, vmName.c_str());
        }
    }

    void CommonVMServices::RegisterCvar(IPC::Reader& reader, const IPC::Socket& socket) {
        IPC::HandleMsg<RegisterCvarMsg>(socket, std::move(reader), [this](Str::StringRef name, Str::StringRef description,
                int flags, Str::StringRef defaultValue){
            // The registration of the cvar is made automatically when it is created
            registeredCvars.emplace_back(new ProxyCvar(this, name, description, flags, defaultValue));
        });
    }

    void CommonVMServices::GetCvar(IPC::Reader& reader, const IPC::Socket& socket) {
        IPC::HandleMsg<GetCvarMsg>(socket, std::move(reader), [&, this](Str::StringRef name, std::string& value){
            //TODO check it is only looking at allowed cvars?
            value = Cvar::GetValue(name);
        });
    }

    void CommonVMServices::SetCvar(IPC::Reader& reader, const IPC::Socket& socket) {
        IPC::HandleMsg<SetCvarMsg>(socket, std::move(reader), [this](Str::StringRef name, Str::StringRef value){
            //TODO check it is only touching allowed cvars?
            Cvar::SetValue(name, value);
        });
    }

    // Misc, Dispatch

    CommonVMServices::CommonVMServices(VMBase& vm, Str::StringRef vmName, int commandFlag)
    :vmName(vmName), vm(vm), commandFlag(commandFlag), commandProxy(new ProxyCmd(*this, commandFlag)) {
    }

    CommonVMServices::~CommonVMServices() {
        //FIXME or iterate over the commands we registered, or add Cmd::RemoveByProxy()
        Cmd::RemoveFlaggedCommands(commandFlag);
        //TODO unregesiter cvars
    }

    void CommonVMServices::Syscall(int major, int minor, IPC::Reader reader, const IPC::Socket& socket) {
        switch (major) {
            case COMMAND:
                HandleCommandSyscall(minor, reader, socket);
                break;

            case CVAR:
                HandleCvarSyscall(minor, reader, socket);
                break;

            default:
                Com_Error(ERR_DROP, "Unhandled common engine syscall major number %i", major);
        }
    }

    VMBase& CommonVMServices::GetVM() {
        return vm;
    }
}
