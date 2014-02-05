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

    void HandleSyscall(int minor, RPC::Reader& inputs, RPC::Writer& outputs) {
        switch (minor) {
            case EXECUTE:
                ExecuteSyscall(inputs, outputs);
                break;

            default:
                G_Error("Unhandled engine command syscall %i", minor);
        }
    }
}

