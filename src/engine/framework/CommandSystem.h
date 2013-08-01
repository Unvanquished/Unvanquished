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

/**
 * Console command execution and command text buffering.
 *
 * Any number of commands can be added in a frame from several different
 * sources. Most commands come from either key bindings or console line input,
 * but entire text files can be execed.
 *
 * The command text is first split into command strings ten tokens,
 * the first token is the name of the command to be executed
 *
 * NOTE: All the functions in this file most be called from the main thread.
 */

namespace Cmd {

    /**
     * Commands can be buffered to be executed right NOW,
     * AFTER the current command or be put at the END of
     * the command buffer.
     */
    enum execWhen_t {
        NOW,
        AFTER,
        END
    };

    // Adds a command text to by executed, optionnally parsing cvars ($cvarname$) if the text is a user input.
    void BufferCommandText(const std::string& text, execWhen_t when = END, bool parseCvars = false);
    // Executes all the buffered commands.
    void ExecuteCommandBuffer();

    // Registers a command
    void AddCommand(std::string name, const CmdBase& cmd, std::string description);
    // Removes a command
    void RemoveCommand(const std::string& name);
    // Removes all the commands with the given flag
    void RemoveFlaggedCommands(int flag);

    // Executes a raw command string as a single command.
    void ExecuteCommand(std::string command);

    //Completion stuff, highly unstable :-)
    std::vector<std::string> CommandNames();
    std::vector<std::string> CompleteArgument(std::string command, int pos);

    //Function to ease the transition to C++
    bool CommandExists(const std::string& name);
    const Args& GetCurrentArgs();
    void SetCurrentArgs(const Args& args);
    void SaveArgs();
    void LoadArgs();
}

#endif // FRAMEWORK_COMMAND_SYSTEM_H_
