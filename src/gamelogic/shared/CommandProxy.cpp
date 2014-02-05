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
namespace Cvar{
    std::string GetValue(const std::string& name) {
        return "";
    }
    void Register(CvarProxy*, std::string const&, std::string, int, std::string const&) {
    }
    void SetValue(std::string const&, std::string) {
    }
}

namespace Log {
    void Dispatch(Event, int) {
    }
}

const char* Trans_Gettext(const char*) {
    return "";
}

// Forward declare DoRPC, would need to expose it somehow
// but the RPC system will get rewritten so...
RPC::Reader DoRPC(RPC::Writer& writer);


//TODO END HACK

namespace Cmd {

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

    struct CommandRecord {
        const Cmd::CmdBase* cmd;
        std::string description;
    };

    typedef std::unordered_map<std::string, CommandRecord> CommandMap;

    CommandMap& GetCommandMap() {
        static CommandMap map;
        return map;
    }

    bool initialized = false;

    static void AddCommandRPC(std::string name, std::string description) {
        RPC::Writer input;
        input.WriteInt(GS_COMMAND);
        input.WriteInt(ADD_COMMAND);
        input.WriteString(name.c_str());
        input.WriteString(description.c_str());
        DoRPC(input);
    }

    void AddCommand(std::string name, const Cmd::CmdBase& cmd, std::string description) {
        if (initialized) {
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

    void InitializeProxy() {
        for(auto& record: GetCommandMap()) {
            AddCommandRPC(record.first, std::move(record.second.description));
        }

        initialized = true;
    }

    class TempCmd: Cmd::StaticCmd {
        public:
            TempCmd() : Cmd::StaticCmd("vmTempCommand", "a temp VM command made with the beautiful proxies") {
            }

            virtual void Run(const Args& args) const {
                Print("The temp command is running");
            }

            virtual Cmd::CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const {
                return {{"toto", "tata"}, {"titi", "tutu"}};
            }
    };

    static TempCmd tempCmdRegistration;

    void ExecuteSyscall(RPC::Reader& inputs, RPC::Writer& outputs) {
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
        for (int i = 0; i < res.size(); i++) {
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

    void PushArgs(Str::StringRef args) {
        argStack.push_back(new Cmd::Args(args));
    }

    void PopArgs() {
        delete argStack.back();
        argStack.pop_back();
    }

}
