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

#include <string>

//TODO we need to include headers in this order
//#include "../../engine/qcommon/q_shared.h"
//#include "bg_public.h"

// We only do game for now but later have a common list of game services
#include "../game/g_local.h"

#include "../../common/Command.h"
#include "../../common/Cvar.h"
#include "../../common/Log.h"
#include "../../common/RPC.h"
#include "unordered_map"

//TODO HACK avoid compile link errors
namespace Log {
    void Dispatch(Event, int) {
    }
}

const char* Trans_Gettext(const char* text) {
    return text;
}

// Forward declare DoRPC, would need to expose it somehow
// but the RPC system will get rewritten so...
RPC::Reader DoRPC(RPC::Writer& writer);


//TODO END HACK

// Command related syscall handling

namespace Cmd {

    // This is a proxy environment given to commands, to replicate the common API
    // Calls to its methods will be proxied to Cmd::GetEnv() in the engine.
    class ProxyEnvironment: public Cmd::Environment {
        public:
            virtual void Print(Str::StringRef text) {
                RPC::Writer input;
                input.WriteInt(GS_COMMAND);
                input.WriteInt(ENV_PRINT);
                input.WriteString(text.c_str());
                DoRPC(input);
            }
            virtual void ExecuteAfter(Str::StringRef text, bool parseCvars = false) {
                RPC::Writer input;
                input.WriteInt(GS_COMMAND);
                input.WriteInt(ENV_EXECUTE_AFTER);
                input.WriteString(text.c_str());
                input.WriteInt(parseCvars);
                DoRPC(input);
            }
    };

    Environment* GetEnv() {
        static Environment* env = new ProxyEnvironment;
        return env;
    }

    // Commands can be statically initialized so we must store the description so that we can register them after main

    struct CommandRecord {
        const Cmd::CmdBase* cmd;
        std::string description;
    };

    typedef std::unordered_map<std::string, CommandRecord> CommandMap;

    CommandMap& GetCommandMap() {
        static CommandMap map;
        return map;
    }

    bool commandsInitialized = false;

    static void AddCommandRPC(std::string name, std::string description) {
        RPC::Writer input;
        input.WriteInt(GS_COMMAND);
        input.WriteInt(ADD_COMMAND);
        input.WriteString(name.c_str());
        input.WriteString(description.c_str());
        DoRPC(input);
    }

    static void InitializeProxy() {
        for(auto& record: GetCommandMap()) {
            AddCommandRPC(record.first, std::move(record.second.description));
        }

        commandsInitialized = true;
    }

    void AddCommand(std::string name, const Cmd::CmdBase& cmd, std::string description) {
        if (commandsInitialized) {
            GetCommandMap()[name] = {&cmd, ""};
            AddCommandRPC(std::move(name), std::move(description));
        } else {
            GetCommandMap()[std::move(name)] = {&cmd, std::move(description)};
        }
    }

    void RemoveCommand(const std::string& name) {
        GetCommandMap().erase(name);

        RPC::Writer input;
        input.WriteInt(GS_COMMAND);
        input.WriteInt(REMOVE_COMMAND);
        input.WriteString(name.c_str());
        DoRPC(input);
    }

    // Implementation of the engine syscalls

    void ExecuteSyscall(RPC::Reader& inputs, RPC::Writer&) {
        Cmd::Args args(inputs.ReadString());

        auto map = GetCommandMap();
        auto it = map.find(args.Argv(0));

        if (it == map.end()) {
            return;
        }

        it->second.cmd->Run(args);
    }

    void CompleteSyscall(RPC::Reader& inputs, RPC::Writer& outputs) {
        int argNum = inputs.ReadInt();
        Cmd::Args args(inputs.ReadString());
        Str::StringRef prefix = inputs.ReadString();

        auto map = GetCommandMap();
        auto it = map.find(args.Argv(0));

        if (it == map.end()) {
            return;
        }

        Cmd::CompletionResult res = it->second.cmd->Complete(argNum, args, prefix);

        outputs.WriteInt(res.size());
        for (unsigned i = 0; i < res.size(); i++) {
            outputs.WriteString(res[i].first.c_str());
            outputs.WriteString(res[i].second.c_str());
        }
    }

    void HandleSyscall(int minor, RPC::Reader& inputs, RPC::Writer& outputs) {
        switch (minor) {
            case EXECUTE:
                ExecuteSyscall(inputs, outputs);
                break;

            case COMPLETE:
                CompleteSyscall(inputs, outputs);
                break;

            default:
                G_Error("Unhandled engine command syscall %i", minor);
        }
    }
}

// Simulation of the command-related trap calls
// The old VM command code registers the command as a string. When the engine calls the command
// it performs a special VM call and the VM will check the string to dispatch to the right function.
// Then to get the arguments the VM uses the trap_Argc and trap_Argv calls.

// This vector is used as a stack of Cmd::Args for when commands are called recursively.
static std::vector<const Cmd::Args*> argStack;

// A proxy command added instead of the string when the VM registers a command? It will just
// setup the args right and call the command Dispatcher
class TrapProxyCommand: public Cmd::CmdBase {
    public:
        TrapProxyCommand() : Cmd::CmdBase(0) {
        }
        virtual void Run(const Cmd::Args& args) const {
            // Push a pointer to args, it is fine because we remove the pointer before args goes out of scope
            argStack.push_back(&args);
            ConsoleCommand();
            argStack.pop_back();
        }
};

static TrapProxyCommand trapProxy;

void trap_AddCommand(const char *cmdName) {
    Cmd::AddCommand(cmdName, trapProxy, "an old style vm command");
}

void trap_RemoveCommand(const char *cmdName) {
    Cmd::RemoveCommand(cmdName);
}

int trap_Argc(void) {
    if (argStack.empty()) {
        return 0;
    }
    return argStack.back()->Argc();
}

void trap_Argv(int n, char *buffer, int bufferLength) {
    if (bufferLength <= 0 or argStack.empty()) {
        return;
    }
    Q_strncpyz(buffer, argStack.back()->Argv(n).c_str(), bufferLength);
}

namespace Cmd {

    // We also provide an API to manipulate the stack for example when the VM asks for commands to be tokenized
    void PushArgs(Str::StringRef args) {
        argStack.push_back(new Cmd::Args(args));
    }

    void PopArgs() {
        delete argStack.back();
        argStack.pop_back();
    }

}

// Cvar related commands

namespace Cvar{

    // Commands can be statically initialized so we must store the registration parameters
    // so that we can register them after main

    struct CvarRecord {
        CvarProxy* cvar;
        std::string description;
        int flags;
        std::string defaultValue;
    };

    typedef std::unordered_map<std::string, CvarRecord> CvarMap;

    CvarMap& GetCvarMap() {
        static CvarMap map;
        return map;
    }

    bool cvarsInitialized = false;

    void RegisterCvarRPC(const std::string& name, std::string description, int flags, std::string defaultValue) {
        RPC::Writer input;
        input.WriteInt(GS_CVAR);
        input.WriteInt(REGISTER_CVAR);
        input.WriteString(name.c_str());
        input.WriteString(description.c_str());
        input.WriteInt(flags);
        input.WriteString(defaultValue.c_str());
        DoRPC(input);
    }

    static void InitializeProxy() {
        for(auto& record: GetCvarMap()) {
            RegisterCvarRPC(record.first, std::move(record.second.description), record.second.flags, std::move(record.second.defaultValue));
        }

        cvarsInitialized = true;
    }

    void Register(CvarProxy* cvar, const std::string& name, std::string description, int flags, const std::string& defaultValue) {
        if (cvarsInitialized) {
            GetCvarMap()[name] = {cvar, "", 0, defaultValue};
            RegisterCvarRPC(name, std::move(description), flags, defaultValue);
        } else {
            GetCvarMap()[name] = {cvar, std::move(description), flags, defaultValue};
        }
    }

    std::string GetValue(const std::string& name) {
        RPC::Writer inputs;
        inputs.WriteInt(GS_CVAR);
        inputs.WriteInt(GET_CVAR);
        inputs.WriteString(name.c_str());
        RPC::Reader outputs = DoRPC(inputs);
        return outputs.ReadString();
    }

    void SetValue(const std::string& name, std::string value) {
        RPC::Writer inputs;
        inputs.WriteInt(GS_CVAR);
        inputs.WriteInt(SET_CVAR);
        inputs.WriteString(name.c_str());
        inputs.WriteString(value.c_str());
        DoRPC(inputs);
    }

    // Syscalls called by the engine

    void CallOnValueChangedSyscall(RPC::Reader& inputs, RPC::Writer& outputs) {
        Str::StringRef name = inputs.ReadString();
        Str::StringRef value = inputs.ReadString();

        auto map = GetCvarMap();
        auto it = map.find(name);

        if (it == map.end()) {
            outputs.WriteInt(true);
            outputs.WriteString("");
            return;
        }

        auto res = it->second.cvar->OnValueChanged(value);

        outputs.WriteInt(res.success);
        outputs.WriteString(res.description.c_str());
    }

    void HandleSyscall(int minor, RPC::Reader& inputs, RPC::Writer& outputs) {
        switch (minor) {
            case ON_VALUE_CHANGED:
                CallOnValueChangedSyscall(inputs, outputs);
                break;

            default:
                G_Error("Unhandled engine cvar syscall %i", minor);
        }
    }
}

// In the QVMs are used registered through vmCvar_t which contains a copy of the values of the cvar
// each frame will call Cvar_Update for each cvar. Previously the cvar system would use a modification
// count in both the engine cvar and the vmcvar to update the latter lazily. However we cannot afford
// that many roundtrips anymore.
// The following code registers a special CvarProxy for each vmCvar_t and will cache the new value.
// That way we are able to update vmcvars lazily without roundtrips. See qcommon/cmd.cpp for the
// legacy implementation.

class VMCvarProxy : public Cvar::CvarProxy {
    public:
        VMCvarProxy(Str::StringRef name, int flags, Str::StringRef defaultValue)
        : Cvar::CvarProxy(name, "a vmCvar_t", flags, defaultValue), modificationCount(0), value(defaultValue) {
            Register();
        }

        virtual Cvar::OnValueChangedResult OnValueChanged(Str::StringRef newValue) OVERRIDE {
            value = newValue;
            modificationCount++;
            return {true, ""};
        }

        void Update(vmCvar_t* cvar) {
            if (cvar->modificationCount == modificationCount) {
                return;
            }

            Q_strncpyz(cvar->string, value.c_str(), MAX_CVAR_VALUE_STRING);

            if (not Str::ToFloat(value, cvar->value)) {
                cvar->value = 0,0;
            }
            if (not Str::ParseInt(cvar->integer, value)) {
                cvar->integer = 0;
            }

            cvar->modificationCount = modificationCount;
        }

    private:
        int modificationCount;
        std::string value;
};

std::vector<VMCvarProxy*> vmCvarProxies;

static void UpdateVMCvar(vmCvar_t* cvar) {
    vmCvarProxies[cvar->handle]->Update(cvar);
}

void trap_Cvar_Register(vmCvar_t *cvar, const char *varName, const char *value, int flags) {
    vmCvarProxies.push_back(new VMCvarProxy(varName, flags, value));

    if (!cvar) {
        return;
    }

    cvar->modificationCount = -1;
    cvar->handle = vmCvarProxies.size() - 1;
    UpdateVMCvar(cvar);
}

void trap_Cvar_Set(const char *varName, const char *value) {
    Cvar::SetValue(varName, value);
}

void trap_Cvar_Update(vmCvar_t *cvar) {
    UpdateVMCvar(cvar);
}

int trap_Cvar_VariableIntegerValue(const char *varName) {
    std::string value = Cvar::GetValue(varName);
    int res;
    if (Str::ParseInt(res, value)) {
        return res;
    }
    return 0;
}

void trap_Cvar_VariableStringBuffer(const char *varName, char *buffer, int bufsize) {
    std::string value = Cvar::GetValue(varName);
    Q_strncpyz(buffer, value.c_str(), bufsize);
}

// Common functions for all syscalls

namespace VM {

    void InitializeProxies() {
        Cmd::InitializeProxy();
        Cvar::InitializeProxy();
    }

    void HandleCommonSyscall(int major, int minor, RPC::Reader& inputs, RPC::Writer& outputs) {
        switch (major) {
            case GS_COMMAND:
                Cmd::HandleSyscall(minor, inputs, outputs);
                break;

            case GS_CVAR:
                Cvar::HandleSyscall(minor, inputs, outputs);
                break;

            default:
                G_Error("Unhandled common VM syscall major number %i", major);
        }
    }
}
