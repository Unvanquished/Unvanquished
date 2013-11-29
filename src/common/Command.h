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

#include "../engine/qcommon/q_shared.h"
#include "String.h"
#include <string>
#include <vector>

#ifndef COMMON_COMMAND_H_
#define COMMON_COMMAND_H_

namespace Cmd {

    /**
     * Commands can be in different namespaces which are used to
     * list commands related to a specific subsystem as well as
     * mass removal of commands.
     */
    enum {
        BASE             = BIT(0),
        CVAR             = BIT(1),
        ALIAS            = BIT(2),
        SYSTEM           = BIT(3),
        RENDERER         = BIT(4),
        SOUND            = BIT(5),
        GAME             = BIT(6),
        CGAME            = BIT(7),
        UI               = BIT(8),
        PROXY_FOR_OLD    = BIT(31) // OLD: The command has been registered through the proxy function in cmd.c
    };

    // OLD: Parsing functions that should go in their modules at some point
    std::string Escape(Str::StringRef text);
    const char* SplitCommand(const char* start, const char* end);
    std::string SubstituteCvars(Str::StringRef text);

    /**
     * Cmd::Args represents the arguments given to an invoked command.
     * It has a number of convenient methods to access the raw command
     * line, quote arguments etc.
     * The first argument (index 0) is the name of the command.
     */
    class Args {
        public:
            // Represents an empty command line
            Args();
            // Represents a command line that is the concatenation of the arguments
            Args(std::vector<std::string> args);
            // Represents the command line
            Args(Str::StringRef command);

            // Basic access
            int Argc() const;
            const std::string& Argv(int argNum) const;

            // Returns an escaped string which will tokenize into the same args
            std::string EscapedArgs(int start, int end = -1) const;
            // Concatenate a set of arguments, separated by spaces
            std::string ConcatArgs(int start, int end = -1) const;

            // Returns all the arguments in a vector
            const std::vector<std::string>& ArgVector() const;

            // Aliases
            int size() const; // same as Argc()
            const std::string& operator[] (int argNum) const; // same as Argv(int)

        private:
            std::vector<std::string> args;
    };

    // A completion result is a list of (result, short description)
    typedef std::vector<std::pair<std::string, std::string>> CompletionResult;

    class Environment;

    /**
     * Commands are defined by subclassing Cmd::CmdBase and defining both
     * the Run method and the namespace in which the command is.
     * Then it can be registered in the command system, this can be done
     * automatically and that's why most of the time you will want to use
     * Cmd::StaticCmd instad of Cmd::CmdBase.
     * In the engine, Run and Complete will always be called from the main thread.
     */
    class CmdBase {
        public:
            // Called when the command is run with the command line args
            virtual void Run(const Args& args) const = 0;
            // Called when the user wants to autocomplete a call to this command.
            virtual Cmd::CompletionResult Complete(int argNum, const Args& args, const std::string& prefix) const;

            // Prints the usage of this command with a standard formatting
            void PrintUsage(const Args& args, Str::StringRef syntax, Str::StringRef description = "") const;

            // Used by the command system.
            int GetFlags() const;

        protected:
            // Is given the namespace of the command.
            CmdBase(int flags);

            // Shortcuts for this->GetEnv().*
            // Commands should use these functions when possible.
            template <typename ... Args>
            void Print(Str::StringRef text, Args ... args) const;
            void ExecuteAfter(Str::StringRef text, bool parseCvars = false) const;

        private:
            Environment& GetEnv() const;

            int flags;
    };

    /**
     * Cmd::StaticCmd automatically register the command when it is
     * instanciated and removes it when it is destroyed. A typical usage is
     *
     *  class MyCmd : public Cmd::StaticCmd {
     *      MyCmd() : Cmd::StaticCmd("my_command", NAMESPACE, "my_desc"){}
     *      //Other stuff
     *  };
     *  static MyCmd MyCmdRegistration;
     */
    class StaticCmd : public CmdBase {
        protected:
            StaticCmd(std::string name, int flags, std::string description);
            //TODO: sometimes (in the gamelogic) we already know what the flags is, provide another constructor for it.
    };

    /**
     * Commands can be run from an environment. For now it is only
     * used to redirect some calls (Print, ...).
     *
     * A DefaultEnvironment in CommandSystem can be inherited from
     * when not all calls are redefined.
     */
    class Environment {
        public:
            virtual void Print(Str::StringRef text) = 0;
            virtual void ExecuteAfter(Str::StringRef text, bool parseCvars = false) = 0;
    };

    // Implementation of templates.

    template <typename ... Args>
    void CmdBase::Print(Str::StringRef text, Args ... args) const {
        GetEnv().Print(Str::Format(text, args ...));
    }

}

#endif // COMMON_COMMAND_H_
