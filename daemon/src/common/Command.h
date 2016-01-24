/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
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

#ifndef COMMON_COMMAND_H_
#define COMMON_COMMAND_H_

#include <engine/qcommon/q_shared.h>

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
        AUDIO            = BIT(5),
        SGAME_VM         = BIT(6),
        CGAME_VM         = BIT(7),
        UI_VM            = BIT(8),
        PROXY_FOR_OLD    = BIT(31) // OLD: The command has been registered through the proxy function in cmd.c
    };

    // OLD: Parsing functions that should go in their modules at some point
    std::string Escape(Str::StringRef text);
    const char* SplitCommand(const char* start, const char* end);
    std::string SubstituteCvars(Str::StringRef text);
    bool IsValidCmdName(Str::StringRef text);
    bool IsValidCvarName(Str::StringRef text);

    bool IsSwitch(Str::StringRef arg, const char *name);

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

            // Range-based for loop support
            std::vector<std::string>::const_iterator begin() const;
            std::vector<std::string>::const_iterator end() const;

        private:
            std::vector<std::string> args;
    };

    // A completion result is a list of (result, short description)
    typedef std::pair<std::string, std::string> CompletionItem;
    typedef std::vector<CompletionItem> CompletionResult;

    CompletionResult FilterCompletion(Str::StringRef prefix, std::initializer_list<CompletionItem> list);
    void AddToCompletion(CompletionResult& res, Str::StringRef prefix, std::initializer_list<CompletionItem> list);

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
            virtual CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const;

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
            void Print(Str::StringRef text, Args&& ... args) const;
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
     *      MyCmd() : Cmd::StaticCmd("my_command", NAMESPACE, "my_description"){}
     *      //Other stuff
     *  };
     *  static MyCmd MyCmdRegistration;
     */
    class StaticCmd : public CmdBase {
        protected:
            StaticCmd(std::string name, std::string description);
            StaticCmd(std::string name, int flags, std::string description);
    };

    /**
     * Cmd::LambdaCmd an automatically registered command whose callbacks can be
     * defined by lambdas. A typical usage is
     *  static LambdaCMd MyCmd("my_command", NAMESPACE, "my_description",
     *      [](const Args& args) {
     *          // Do stuff
     *      }
     *  );
     */
    CompletionResult NoopComplete(int argNum, const Args& args, Str::StringRef prefix);

    class LambdaCmd : public StaticCmd {
        public:
            typedef std::function<void(const Args&)> RunFn;
            typedef std::function<CompletionResult(int, const Args&, Str::StringRef)> CompleteFn;
            LambdaCmd(std::string name, std::string description, RunFn run, CompleteFn complete = NoopComplete);
            LambdaCmd(std::string name, int flags, std::string description, RunFn run, CompleteFn complete = NoopComplete);

            void Run(const Args& args) const OVERRIDE;
            CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const OVERRIDE;

        private:
            RunFn run;
            CompleteFn complete;
    };

    /**
     * Commands can be run from an environment. For now it is only
     * used to redirect some calls (Print, ...).
     *
     * A DefaultEnvironment in CommandSystem can be inherited from
     * when not all calls are redefined.
     *
     * Note that the environment is only used to support the print redirection
     * for rcon. Changing that mechanism is TODO but will allow the removal of
     * Environment.
     */
    class Environment {
        public:
            virtual void Print(Str::StringRef text) = 0;
            virtual void ExecuteAfter(Str::StringRef text, bool parseCvars = false) = 0;
    };

    // Engine calls available everywhere

    void AddCommand(const std::string& name, const CmdBase& cmd, std::string description);
    void RemoveCommand(const std::string& name);
    Environment* GetEnv();

    // Implementation of templates.

    template <typename ... Args>
    void CmdBase::Print(Str::StringRef text, Args&& ... args) const {
        GetEnv().Print(Str::Format(text, std::forward<Args>(args) ...));
    }

}

#endif // COMMON_COMMAND_H_
