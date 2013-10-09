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

#include "../../shared/String.h"

#include <unordered_map>
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

//TODO: use case-insensitive comparisons for commands (store the lower case version?)
namespace Cmd {

    struct commandRecord_t {
        std::string description;
        const CmdBase* cmd;
    };

    typedef std::unordered_map<std::string, commandRecord_t> CommandMap;

    CommandMap& GetCommandMap() {
        static CommandMap* commands = new CommandMap();
        return *commands;
    }

    /*
    ===============================================================================

    Cmd:: The command buffer

    ===============================================================================
    */

    std::vector<std::string> commandBuffer;

    void BufferCommandText(const std::string& text, execWhen_t when, bool parseCvars) {
        const char* current = text.data();
        const char* end = text.data() + text.size();
        do {
            const char* next = SplitCommand(current, end);
            std::string command(current, next != end ? next - 1 : end);
            if (parseCvars)
                command = SubstituteCvars(command);
            switch (when) {
                case NOW:
                    ExecuteCommand(std::move(command));
                    break;

                case AFTER:
                    commandBuffer.insert(commandBuffer.begin(), std::move(command));
                    break;

                case END:
                    commandBuffer.insert(commandBuffer.end(), std::move(command));
                    break;

                default:
                    Com_Printf("Cmd::BufferCommandText: unknown execWhen_t %i\n", when);
            }
            current = next;
        } while (current != end);
    }

    //TODO: reimplement the wait command, maybe?
    void ExecuteCommandBuffer() {
        // Note that commands may be inserted into the buffer while running other commands
        while (not commandBuffer.empty()) {
            std::string command = commandBuffer.front();
            commandBuffer.erase(commandBuffer.begin());
            ExecuteCommand(command);
        }
        commandBuffer.clear();
    }

    /*
    ===============================================================================

    Cmd:: Registration and execution

    ===============================================================================
    */

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

    void RemoveFlaggedCommands(int flag) {
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

        auto it = commands.find(cmdName);
        if (it != commands.end()) {
            it->second.cmd->Run(args);
            return;
        }

        //TODO: remove that and add default command handlers or something
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
        CL_ForwardCommandToServer( args.RawCmd().c_str() );
    }

    CompletionResult CompleteArgument(std::string command, int pos) {
        CommandMap& commands = GetCommandMap();

        Args args(std::move(command));

        if (args.Argc() == 0) {
            return {};
        }

        int argNum = args.PosToArg(pos);

        if (argNum > 0) {
            const std::string& cmdName = args.Argv(0);

            auto it = commands.find(cmdName);
            if (it == commands.end()) {
                return {};
            }

            return it->second.cmd->Complete(pos, args);
        } else {
            return CompleteCommandNames(args.Argv(0));
        }
    }

    CompletionResult CompleteCommandNames(const std::string& prefix) {
        CommandMap& commands = GetCommandMap();

        CompletionResult res;
        for (auto& entry: commands) {
            if (Str::IsPrefix(prefix, entry.first)) {
                res.push_back({entry.first, entry.second.description});
            }
        }
        return res;
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

    /*
    ===============================================================================

    Cmd:: /list<Subsystem>Commands

    ===============================================================================
    */


    class ListCmdsCmd: public StaticCmd {
        public:
            ListCmdsCmd(std::string name, int cmdFlags, std::string description, int showCmdFlags)
            :StaticCmd(std::move(name), cmdFlags, std::move(description)), showCmdFlags(showCmdFlags) {
            }

            void Run(const Cmd::Args& args) const override {
                CommandMap& commands = GetCommandMap();

                std::vector<const commandRecord_t*> matches;
                std::vector<const std::string*> matchesNames;
                unsigned long maxNameLength = 0;

                //Find all the matching commands and their names
                for (auto it = commands.cbegin(); it != commands.cend(); ++it) {
                    const commandRecord_t& record = it->second;

                    // /listCmds's arguement is used for prefix matching
                    if (args.Argc() >= 2 and not Str::IsPrefix(args.Argv(1), it->first)) {
                        continue;
                    }

                    if (record.cmd->GetFlags() & showCmdFlags) {
                        matches.push_back(&it->second);
                        matchesNames.push_back(&it->first);
                        maxNameLength = std::max(maxNameLength, it->first.size());
                    }
                }

                //Print the matches, keeping the description aligned
                for (unsigned i = 0; i < matches.size(); i++) {
                    int toFill = maxNameLength - matchesNames[i]->size();
                    Com_Printf("  %s%s %s\n", matchesNames[i]->c_str(), std::string(toFill, ' ').c_str(), matches[i]->description.c_str());
                }

                Com_Printf("%zu commands\n", matches.size());
            }

            Cmd::CompletionResult Complete(int pos, const Cmd::Args& args) const override {
                if (args.PosToArg(pos) == 1) {
                    return ::Cmd::CompleteCommandNames(args.ArgPrefix(pos));
                }

                return {};
            }

        private:
            int showCmdFlags;
    };

    static ListCmdsCmd listCmdsRegistration("listCmds", BASE, "lists all the commands", ~(CVAR|ALIAS));
    static ListCmdsCmd listBaseCmdsRegistration("listBaseCmds", BASE, "lists all the base commands", BASE);
    static ListCmdsCmd listSystemCmdsRegistration("listSystemCmds", BASE | SYSTEM, "lists all the system commands", SYSTEM);
    static ListCmdsCmd listRendererCmdsRegistration("listRendererCmds", BASE | RENDERER, "lists all the renderer commands", RENDERER);
    static ListCmdsCmd listSoundCmdsRegistration("listSoundCmds", BASE | SOUND, "lists all the sound commands", SOUND);
    static ListCmdsCmd listCGameCmdsRegistration("listCGameCmds", BASE | CGAME, "lists all the client-side game commands", CGAME);
    static ListCmdsCmd listGameCmdsRegistration("listGameCmds", BASE | GAME, "lists all the server-side game commands", GAME);
    static ListCmdsCmd listUICmdsRegistration("listUICmds", BASE | UI, "lists all the UI commands", UI);
    static ListCmdsCmd listOldStyleCmdsRegistration("listOldStyleCmds", BASE, "lists all the commands registered through the C interface", PROXY_FOR_OLD);
}
