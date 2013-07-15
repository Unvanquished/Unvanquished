/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "CommandSystem.h"

#include <unordered_map>
#include <list>
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

namespace Cmd {

    std::list<std::string> commandBuffer;

    void BufferCommandText(const std::string& text, execWhen_t when, bool parseCvars) {
        std::list<std::string> splitted = SplitCommandText(text);

        std::list<std::string> commands;
        if (parseCvars) {
            for (auto text: splitted) {
                commands.push_back(SubstituteCvars(text));
            }
        } else {
            commands = std::move(splitted);
        }

        switch (when) {
            case NOW:
                for(auto command : commands) {
                    ExecuteCommand(command);
                }
                break;

            case AFTER:
                commandBuffer.splice(commandBuffer.begin(), commands);
                break;

            case END:
                commandBuffer.splice(commandBuffer.end(), commands);
                break;

            default:
                Com_Printf("Cmd::BufferCommandText: unknown execWhen_t %i\n", when);
        }
    }

    //TODO: reimplement the wait command, maybe?
    void ExecuteCommandBuffer() {
        while (not commandBuffer.empty()) {
            std::string command = commandBuffer.front();
            commandBuffer.pop_front();
            ExecuteCommand(command);
        }
        commandBuffer.clear();
    }

    struct commandRecord_t {
        std::string description;
        const CmdBase* cmd;
    };

    typedef typename std::unordered_map<std::string, commandRecord_t> CommandMap;

    CommandMap& GetCommandMap() {
        static CommandMap* commands = new CommandMap();
        return *commands;
    }

    //TODO: remove the need for this
    Args currentArgs;
    Args oldArgs;

    void AddCommand(std::string name, const CmdBase& cmd, std::string description) {
        CommandMap& commands = GetCommandMap();

        if (commands.count(name)) {
			Com_Printf(_( "Cmd::AddCommand: %s already defined\n"), name.c_str() );
			return;
        }

        commands[std::move(name)] = commandRecord_t{std::move(description), &cmd};
    }

    void RemoveCommand(const std::string& name) {
        CommandMap& commands = GetCommandMap();

        commands.erase(name);
    }

    void RemoveFlaggedCommands(cmdFlags_t flag) {
        CommandMap& commands = GetCommandMap();

        for (auto it = commands.cbegin(); it != commands.cend();) {
            const commandRecord_t& record = it->second;

            if (record.cmd->GetFlags() & flag) {
                commands.erase(it ++);
            } else {
                ++ it;
            }
        }
    }

    bool CommandExists(const std::string& name) {
        CommandMap& commands = GetCommandMap();

        return commands.count(name);
    }

    void ExecuteCommand(std::string command) {
        CommandMap& commands = GetCommandMap();

        Args args(std::move(command));
        currentArgs = args;

        if (args.Argc() == 0) {
            return;
        }

        const std::string& cmdName = args.Argv(0);
        if (commands.count(cmdName)) {
            commands[cmdName].cmd->Run(args);
            return;
        }

        //TODO: remove that and add default command handlers or something
        // check cvars
        if (Cvar_Command()) {
            return;
        }

        // check client game commands
        if (com_cl_running && com_cl_running->integer && CL_GameCommand()) {
            return;
        }

        // check server game commands
        if (com_sv_running && com_sv_running->integer && SV_GameCommand()) {
            return;
        }

        // check ui commands
        if (com_cl_running && com_cl_running->integer && UI_GameCommand()) {
            return;
        }

        // send it as a server command if we are connected
        // (cvars are expanded locally)
        CL_ForwardCommandToServer( args.RawArgs().c_str() );
    }

    std::vector<std::string> CommandNames() {
        CommandMap& commands = GetCommandMap();

        std::vector<std::string> res;
        for (auto& entry: commands) {
            res.push_back(entry.first);
        }
        return res;
    }

    std::vector<std::string> CompleteArgument(std::string command, int pos) {
        CommandMap& commands = GetCommandMap();

        Args args(std::move(command));
        int argNum = args.PosToArg(pos);
        const std::string& cmdName = args.Argv(0);

        if (!commands.count(cmdName)) {
            return {};
        }

        const CmdBase* cmd = commands[cmdName].cmd;
        return cmd->Complete(argNum, args);
    }

    const Args& GetCurrentArgs() {
        return currentArgs;
    }

    void SetCurrentArgs(const Args& args) {
        currentArgs = args;
    }

    void SaveArgs() {
        oldArgs = currentArgs;
    }

    void LoadArgs() {
        currentArgs = oldArgs;
    }
}
