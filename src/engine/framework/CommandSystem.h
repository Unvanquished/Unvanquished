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

namespace Cmd {

    typedef enum {
        NOW,
        AFTER,
        END
    } execWhen_t;

    void BufferCommandText(const std::string& text, execWhen_t when = END, bool parseCvars = false);
    void ExecuteCommandBuffer();

    void AddCommand(std::string name, const CmdBase* cmd);
    void RemoveCommand(const std::string& name);
    void RemoveFlaggedCommands(cmdFlags_t flag);

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
