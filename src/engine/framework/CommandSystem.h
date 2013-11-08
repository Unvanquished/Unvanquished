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

#include "../../shared/Command.h"

#ifndef FRAMEWORK_COMMAND_SYSTEM_H_
#define FRAMEWORK_COMMAND_SYSTEM_H_

/*
 * Command execution and command text buffering.
 *
 * Commands represent "functions" in the code that are invoked with a string,
 * command strings can come from different sources and are buffered and
 * executed at least once per frame.
 * Typically commands come from the console or key bindings but entire files
 * can be exec'ed and sometimes the code use commands to sent messages between
 * modules.
 *
 * A command text is composed of several commands separated by a semi-colon
 * or a new line and each command is composed of tokens separated by spaces.
 * The first token is the command name. Note that additional parsing rules
 * may apply.
 *
 * This file contains
 *
 * TODO: shared might be moved
 * See also shared/Command.h for the interface used to define commands.
 *
 * CommandSystem is responsible of managing, buffering and executing
 * commands.
 */

//TODO: figure out the threading mode for commands and change the doc accordingly

namespace Cmd {

    /*
     * The command buffer stores command to be executed for later.
     * Command texts can be parsed for cvar substition ($cvarname$)
     * when run and can also be executed in a custom Environment.
     */

    //TODO make it thread safe
    // Adds a command text to by executed (last, after what's already in the buffer)
    void BufferCommandText(Str::StringRef text, bool parseCvars = false, Environment* env = nullptr);
    // Adds a command text to be executed just after the current command (used by commands execute other commands)
    void BufferCommandTextAfter(Str::StringRef text, bool parseCvars = false, Environment* env = nullptr);

    //TODO: figure out a way to make this convenient for non-main threads
    // Executes all the buffered commands. Must be called by the main thread.
    void ExecuteCommandBuffer();

    // Managing commands.

    //TODO make it thread safe
    // Registers a command
    void AddCommand(std::string name, const CmdBase& cmd, std::string description);
    // Removes a command
    void RemoveCommand(const std::string& name);
    // Removes all the commands with the given flag
    void RemoveFlaggedCommands(int flag);

    //TODO: figure out a way to make this convenient for non-main threads
    // Executes a raw command string as a single command. Must be called by the main thread.
    void ExecuteCommand(Str::StringRef command, bool parseCvars = false, Environment* env = nullptr);

    //Completion stuff, highly unstable :-)
    CompletionResult CompleteArgument(const Args& args, int argNum);
    CompletionResult CompleteCommandNames(const std::string& prefix = "");

    //Function to ease the transition to C++
    bool CommandExists(const std::string& name);
    const Args& GetCurrentArgs();
    void SetCurrentArgs(const Args& args);
    void SaveArgs();
    void LoadArgs();

    //Environment related private functions
    Environment* GetEnv();

    class DefaultEnvironment: public Environment {
        public:
            virtual void Print(Str::StringRef text) override;
            virtual void ExecuteAfter(Str::StringRef text, bool parseCvars) override;
    };
}

#endif // FRAMEWORK_COMMAND_SYSTEM_H_
