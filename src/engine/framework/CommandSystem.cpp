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

    typedef typename std::unordered_map<std::string, commandRecord_t> CommandMap;

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
        std::vector<std::string> commands = SplitCommandText(text);

        if (parseCvars) {
            for (auto& text: commands) {
                text = SubstituteCvars(text);
            }
        }

        switch (when) {
            case NOW:
                for(auto command : commands) {
                    ExecuteCommand(command);
                }
                break;

            case AFTER:
                commandBuffer.insert(commandBuffer.begin(), commands.begin(), commands.end());
                break;

            case END:
                commandBuffer.insert(commandBuffer.end(), commands.begin(), commands.end());
                break;

            default:
                Com_Printf("Cmd::BufferCommandText: unknown execWhen_t %i\n", when);
        }
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
        if (commands.count(cmdName)) {
            commands[cmdName].cmd->Run(args);
            return;
        }

        //TODO: remove that and add default command handlers or something
        // check cvars
        if (Cvar_Command(args)) {
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
        CL_ForwardCommandToServer( args.RawCmd().c_str() );
    }

    std::vector<std::string> CommandNames(const std::string& prefix) {
        CommandMap& commands = GetCommandMap();

        std::vector<std::string> res;
        for (auto& entry: commands) {
            if (Str::IsPrefix(prefix, entry.first)) {
                res.push_back(entry.first);
            }
        }
        return res;
    }

    std::vector<std::string> CompleteArgument(std::string command, int pos) {
        CommandMap& commands = GetCommandMap();

        Args args(std::move(command));
        int argNum = args.PosToArg(pos);

        if (argNum > 0) {
            const std::string& cmdName = args.Argv(0);

            if (!commands.count(cmdName)) {
                return {};
            }

            const CmdBase* cmd = commands[cmdName].cmd;
            return cmd->Complete(pos, args);
        } else {
            return CommandNames(args.Argv(0));
        }
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

    void ListFlaggedCommands(const Args& args, int flags) {
        CommandMap& commands = GetCommandMap();

        //TODO: add partial matches as in Doom3BFG
        std::vector<const commandRecord_t*> matches;
        std::vector<const std::string*> matchesNames;
        int maxNameLength = 0;

        //Find all the matching commands and their names
        for (auto it = commands.cbegin(); it != commands.cend(); ++it) {
            const commandRecord_t& record = it->second;

            if (record.cmd->GetFlags() & flags) {
                matches.push_back(&it->second);
                matchesNames.push_back(&it->first);
                maxNameLength = MAX(maxNameLength, it->first.size());
            }
        }

        //Print the matches, keeping the description aligned
        for (int i = 0; i < matches.size(); i++) {
            int toFill = maxNameLength - matchesNames[i]->size();
            Com_Printf("  %s%s %s\n", matchesNames[i]->c_str(), std::string(toFill, ' ').c_str(), matches[i]->description.c_str());
        }

        Com_Printf("%zu commands\n", matches.size());
    }

    class ListCmdsCmd: public StaticCmd {
        public:
            ListCmdsCmd(): StaticCmd("listCmds", BASE, "lists all the commands") {
            }

            void Run(const Cmd::Args& args) const override {
                ListFlaggedCommands(args, ~ALIAS);
            }
    };

    class ListBaseCmdsCmd: public StaticCmd {
        public:
            ListBaseCmdsCmd(): StaticCmd("listBaseCmds", BASE, "lists all the base commands") {
            }

            void Run(const Cmd::Args& args) const override {
                ListFlaggedCommands(args, BASE);
            }
    };

    class ListSystemCmdsCmd: public StaticCmd {
        public:
            ListSystemCmdsCmd(): StaticCmd("listSystemCmds", BASE | SYSTEM, "lists all the system commands") {
            }

            void Run(const Cmd::Args& args) const override {
                ListFlaggedCommands(args, SYSTEM);
            }
    };

    class ListRendererCmdsCmd: public StaticCmd {
        public:
            ListRendererCmdsCmd(): StaticCmd("listRendererCmds", BASE | RENDERER, "lists all the renderer commands") {
            }

            void Run(const Cmd::Args& args) const override {
                ListFlaggedCommands(args, RENDERER);
            }
    };
	static ListRendererCmdsCmd listRendererCmdsCmdRegistration;

    class ListSoundCmdsCmd: public StaticCmd {
        public:
            ListSoundCmdsCmd(): StaticCmd("listSoundCmds", BASE | SOUND, "lists all the sound commands") {
            }

            void Run(const Cmd::Args& args) const override {
                ListFlaggedCommands(args, SOUND);
            }
    };

    class ListCGameCmdsCmd: public StaticCmd {
        public:
            ListCGameCmdsCmd(): StaticCmd("listCGameCmds", BASE | CGAME, "lists all the client-side game commands") {
            }

            void Run(const Cmd::Args& args) const override {
                ListFlaggedCommands(args, CGAME);
            }
    };

    class ListGameCmdsCmd: public StaticCmd {
        public:
            ListGameCmdsCmd(): StaticCmd("listGameCmds", BASE | GAME, "lists all the server-side game commands") {
            }

            void Run(const Cmd::Args& args) const override {
                ListFlaggedCommands(args, GAME);
            }
    };

    class ListUICmdsCmd: public StaticCmd {
        public:
            ListUICmdsCmd(): StaticCmd("listUICmds", BASE | UI, "lists all the UI commands") {
            }

            void Run(const Cmd::Args& args) const override {
                ListFlaggedCommands(args, UI);
            }
    };

    class ListOldStyleCmdsCmd: public StaticCmd {
        public:
            ListOldStyleCmdsCmd(): StaticCmd("listOldStyleCmds", BASE, "lists all the commands registered through the C interface") {
            }

            void Run(const Cmd::Args& args) const override {
                ListFlaggedCommands(args, PROXY_FOR_OLD);
            }
    };

    //Automatically register the list<Subsystem>Cmds commands
	static ListCmdsCmd listCmdsCmdRegistration;
	static ListBaseCmdsCmd listBaseCmdsCmdRegistration;
	static ListSystemCmdsCmd listSystemCmdsCmdRegistration;
	static ListSoundCmdsCmd listSoundCmdsCmdRegistration;
	static ListCGameCmdsCmd listCGameCmdsCmdRegistration;
	static ListGameCmdsCmd listGameCmdsCmdRegistration;
	static ListUICmdsCmd listUICmdsCmdRegistration;
	static ListOldStyleCmdsCmd listOldStyleCmdsCmdRegistration;
}
