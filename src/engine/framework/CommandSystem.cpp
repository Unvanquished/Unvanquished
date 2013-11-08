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

#include "../../shared/Log.h"
#include "../../shared/String.h"

#include <unordered_map>
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

//TODO: use case-insensitive comparisons for commands (store the lower case version?)
namespace Cmd {

    Log::Logger commandLog("common.commands");

    /*
    ===============================================================================

    Cmd:: The command buffer

    ===============================================================================
    */

    // A buffered command and its environment
    struct BufferEntry {
        std::string text;
        Environment* env;
        bool parseCvars;
    };

    std::vector<BufferEntry> commandBuffer;

    void BufferCommandTextInternal(Str::StringRef text, bool parseCvars, Environment* env, bool insertAtTheEnd) {
        auto insertPoint = insertAtTheEnd ? commandBuffer.end() : commandBuffer.begin();

        // Iterates over the commands in the text
        const char* current = text.data();
        const char* end = text.data() + text.size();
        do {
            const char* next = SplitCommand(current, end);
            std::string command(current, next != end ? next - 1 : end);

            insertPoint = ++commandBuffer.insert(insertPoint, {std::move(command), env, parseCvars});

            current = next;
        } while (current != end);
    }

    void BufferCommandText(Str::StringRef text, bool parseCvars, Environment* env) {
        BufferCommandTextInternal(text, parseCvars, env, true);
    }

    void BufferCommandTextAfter(Str::StringRef text, bool parseCvars, Environment* env) {
        BufferCommandTextInternal(text, parseCvars, env, false);
    }

    void ExecuteCommandBuffer() {
        // Note that commands may be inserted into the buffer while running other commands
        while (not commandBuffer.empty()) {
            auto entry = std::move(commandBuffer.front());
            commandBuffer.erase(commandBuffer.begin());
            ExecuteCommand(entry.text, entry.parseCvars, entry.env);
        }
        commandBuffer.clear();
    }

    /*
    ===============================================================================

    Cmd:: Registration and execution

    ===============================================================================
    */

    // Commands are stored alongside their description
    struct commandRecord_t {
        std::string description;
        const CmdBase* cmd;
    };

    typedef std::unordered_map<std::string, commandRecord_t, Str::IHash, Str::IEqual> CommandMap;

    // Command execution is sequential so we make their environment a global variable.
    Environment* storedEnvironment = nullptr;

    // The order in which static global variables are initialized is undefined and commands
    // can be registered before main. The first time this function is called the command map
    // is initialized so we are sure it is initialized as soon as we need it.
    CommandMap& GetCommandMap() {
        static CommandMap* commands = new CommandMap();
        return *commands;
    }

    // Used to emalute the C API
    //TODO: remove the need for this
    Args currentArgs;
    Args oldArgs;

    void AddCommand(std::string name, const CmdBase& cmd, std::string description) {
        CommandMap& commands = GetCommandMap();

        if (commands.count(name)) {
			commandLog.Warn(_("Cmd::AddCommand: %s already defined"), name);
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

    DefaultEnvironment defaultEnv;

    void ExecuteCommand(Str::StringRef command, bool parseCvars, Environment* env) {
        CommandMap& commands = GetCommandMap();

        if (not env) {
            commandLog.Debug("Execing command '%s'", command);
            env = &defaultEnv;
        } else {
            commandLog.Debug("Execing command '%s'", command);
            //commandLog.Debug("Execing, in environment %s, command '%s'", env->GetName(), command);
        }

        std::string parsedString;
        if (parseCvars) {
            parsedString = SubstituteCvars(command);
            command = parsedString;
        }

        Args args(command);
        currentArgs = args;

        if (args.Argc() == 0) {
            return;
        }

        const std::string& cmdName = args.Argv(0);

        auto it = commands.find(cmdName);
        if (it != commands.end()) {
            storedEnvironment = env;
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
        CL_ForwardCommandToServer(args.EscapedArgs(0).c_str());
    }

    CompletionResult CompleteArgument(const Args& args, int argNum) {
        CommandMap& commands = GetCommandMap();

        if (args.Argc() == 0) {
            return {};
        }

        if (argNum > 0) {
            const std::string& cmdName = args.Argv(0);

            auto it = commands.find(cmdName);
            if (it == commands.end()) {
                return {};
            }

            std::string prefix;
            if (argNum < args.Argc()) {
                prefix = args.Argv(argNum);
            }

            return it->second.cmd->Complete(argNum, args, prefix);
        } else {
            return CompleteCommandNames(args.Argv(0));
        }
    }

    CompletionResult CompleteCommandNames(const std::string& prefix) {
        CommandMap& commands = GetCommandMap();

        CompletionResult res;
        for (auto& entry: commands) {
            if (Str::IsIPrefix(prefix, entry.first)) {
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

    Environment* GetEnv() {
        return storedEnvironment;
    }

    /*
    ===============================================================================

    Cmd:: DefaultEnvironment

    ===============================================================================
    */

	void DefaultEnvironment::Print(Str::StringRef text) {
		Log::CodeSourceNotice(text);
	}

	void DefaultEnvironment::ExecuteAfter(Str::StringRef text, bool parseCvars) {
		BufferCommandTextAfter(text, parseCvars, this);
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
                    if (args.Argc() >= 2 and not Str::IsIPrefix(args.Argv(1), it->first)) {
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
                    Print("  %s%s %s", matchesNames[i]->c_str(), std::string(toFill, ' ').c_str(), matches[i]->description.c_str());
                }

                Print("%zu commands", matches.size());
            }

            Cmd::CompletionResult Complete(int argNum, const Cmd::Args& args, const std::string& prefix) const override {
                Q_UNUSED(args);

                if (argNum == 1) {
                    return ::Cmd::CompleteCommandNames(prefix);
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
