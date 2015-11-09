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
 * See also common/Command.h for the interface used to define commands.
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
    void AddCommand(const std::string& name, const CmdBase& cmd, std::string description);
    // Changes the description of a command
    void ChangeDescription(std::string name, std::string description);
    // Removes a command
    void RemoveCommand(const std::string& name);
    // Removes all the commands with the given flag
    void RemoveFlaggedCommands(int flag);
    // Removes all the commands registered with the same handler
    void RemoveSameCommands(const CmdBase& cmd);

    //TODO: figure out a way to make this convenient for non-main threads
    // Executes a raw command string as a single command. Must be called by the main thread.
    void ExecuteCommand(Str::StringRef command, bool parseCvars = false, Environment* env = nullptr);

    //Completion stuff, highly unstable :-)
    CompletionResult CompleteArgument(const Args& args, int argNum);
    CompletionResult CompleteCommandNames(Str::StringRef prefix = "");

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
            virtual void Print(Str::StringRef text) OVERRIDE;
            virtual void ExecuteAfter(Str::StringRef text, bool parseCvars) OVERRIDE;
    };
}

#endif // FRAMEWORK_COMMAND_SYSTEM_H_
