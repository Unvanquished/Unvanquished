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

#include "qcommon/q_shared.h"
#include "BaseCommands.h"

#include "CommandSystem.h"
#include "CvarSystem.h"

/**
 * Definition of most of the commands that provide scripting capabilities to the console
 */

namespace Cmd {

    class VstrCmd: public StaticCmd {
        public:
            VstrCmd(): StaticCmd("vstr", BASE, "executes a variable command") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                if (args.Argc() != 2) {
                    PrintUsage(args, "<variablename>", "executes a variable command");
                    return;
                }

                std::string command = Cvar::GetValue(args.Argv(1));
                ExecuteAfter(command, true);
            }

            Cmd::CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const OVERRIDE {
                Q_UNUSED(args);

                if (argNum == 1) {
                    return Cvar::Complete(prefix);
                }

                return {};
            }
    };
    static VstrCmd VstrCmdRegistration;

    class ExecCmd: public StaticCmd {
        public:
            ExecCmd(Str::StringRef name, bool readHomepath): StaticCmd(name, BASE, "executes a command file"), readHomepath(readHomepath) {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                bool executeSilent = false;
                bool failSilent = false;
                bool hasOptions = args.Argc() >= 3 and args.Argv(1).size() >= 2 and args.Argv(1)[0] == '-';

                //Check the syntax
                if (args.Argc() < 2) {
                    if (readHomepath) {
                        PrintUsage(args, "[-q|-f|-s] <filename> [<arguments>…]", "execute a user script file from config/.");
                    } else {
                        PrintUsage(args, "[-q|-f|-s] <filename> [<arguments>…]", "execute a script file from a pak.");
                    }
                    return;
                }

                //Read options
                if (hasOptions) {
                    switch (args.Argv(1)[1]) {
                        case 'q':
                            executeSilent = true;
                            break;

                        case 'f':
                            failSilent = true;
                            break;

                        case 's':
                            executeSilent = failSilent = true;
                            break;

                        default:
                            break;
                    }
                }

                int filenameArg = hasOptions ? 2 : 1;
                const std::string& filename = args.Argv(filenameArg);

                SetExecArgs(args, filenameArg + 1);
                if (not ExecFile(filename, executeSilent)) {
                    if (not failSilent) {
                        Print("couldn't exec '%s'", filename.c_str());
                    }
                    return;
                }
            }

            Cmd::CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const OVERRIDE {
                if (argNum == 1 || (argNum == 2 && Str::IsPrefix("-", args.Argv(1)))) {
                    if (readHomepath)
                        return FS::HomePath::CompleteFilename(prefix, "config", ".cfg", true, false);
                    else
                        return FS::PakPath::CompleteFilename(prefix, "", ".cfg", true, false);
                }

                return {};
            }

        private:
            bool readHomepath;

            void SetExecArgs(const Cmd::Args& args, int start) const {
                //Set some cvars up so that scripts file can be used like functions
                ExecuteAfter(Str::Format("set arg_all %s", Cmd::Escape(args.ConcatArgs(start))));
                ExecuteAfter(Str::Format("set arg_count %d", args.Argc() - start));

                for (int i = start; i < args.Argc(); i++) {
                    ExecuteAfter(Str::Format("set arg_%d %s", i - start, Cmd::Escape(args.Argv(i))));
                }
            }

            bool ExecFile(Str::StringRef filename, bool executeSilent) const {
                std::string buffer;
                std::error_code err;
                if (readHomepath) {
                    FS::File file = FS::HomePath::OpenRead(FS::Path::Build("config", filename), err);
                    if (!err)
                        buffer = file.ReadAll(err);
                } else {
                    buffer = FS::PakPath::ReadFile(filename, err);
                }
                if (err) {
                    return false;
                }

                if (not executeSilent) {
                    Print("execing '%s'", filename.c_str());
                }

                ExecuteAfter(buffer, true);
                return true;
            }
    };
    static ExecCmd ExecCmdRegistration("exec", true);
    static ExecCmd PresetCmdRegistration("preset", false);

    class EchoCmd: public StaticCmd {
        public:
            EchoCmd(): StaticCmd("echo", BASE, "prints to the console") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                std::string res = "";

                for (int i = 1; i < args.Argc(); i++) {
                    res += args.Argv(i) + " ";
                }
                Print(res);
            }
    };
    static EchoCmd EchoCmdRegistration;

    class RandomCmd: public StaticCmd {
        public:
            RandomCmd(): StaticCmd("random", BASE, "sets a variable to a random integer") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                int min, max;
                if (args.Argc() != 4 or !Str::ParseInt(min, args.Argv(2)) or !Str::ParseInt(max, args.Argv(3)) or min >= max) {
                    PrintUsage(args, "<variableToSet> <minNumber> <maxNumber>", "sets a variable to a random integer between minNumber and maxNumber");
                    return;
                }

                const std::string& cvar = args.Argv(1);
                int number = min + (std::rand() % (max - min));

                Cvar::SetValue(cvar, std::to_string(number));
            }

            Cmd::CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const OVERRIDE {
                Q_UNUSED(args);

                if (argNum == 1) {
                    return Cvar::Complete(prefix);
                }

                return {};
            }
    };
    static RandomCmd RandomCmdRegistration;

    class ConcatCmd: public StaticCmd {
        public:
            ConcatCmd(): StaticCmd("concat", BASE, "concatenatas variables") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                if (args.Argc() < 3) {
                    PrintUsage(args, "<variableToSet> <variable1> … <variableN>", "concatenates variable1 to variableN and sets the result to variableToSet");
                    return;
                }

                std::string res;
                for (int i = 2; i < args.Argc(); i++) {
                    res += Cvar::GetValue(args.Argv(i));
                }
                Cvar::SetValue(args.Argv(1), res);
            }

            Cmd::CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const OVERRIDE {
                Q_UNUSED(args);

                if (argNum >= 1) {
                    return Cvar::Complete(prefix);
                }

                return {};
            }
    };
    static ConcatCmd ConcatCmdRegistration;

    class MathCmd: public StaticCmd {
        public:
            MathCmd(): StaticCmd("math", BASE, "does math and sets the result to a variable") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                if  (args.Argc() < 2) {
                    Usage(args);
                    return;
                }

                const std::string& targetName = args.Argv(1);
                float currentValue = atof(Cvar::GetValue(targetName).c_str()); //TODO: Str::ToFloat
                float newValue;

                if (args.Argc() == 3) {
                    // only one additional argument (++, --)
                    const std::string& op = args.Argv(2);

                    if (op == "++") {
                        newValue = currentValue + 1;
                    } else if (op == "--") {
                        newValue = currentValue - 1;
                    } else {
                        Usage(args);
                        return;
                    }

                } else if (args.Argc() == 4) {
                    // two additional arguments (+=, -=, *=, /=)
                    const std::string& op = args.Argv(2);
                    float operand = std::stof(args.Argv(3));

                    if (op == "+") {
                        newValue = currentValue + operand;
                    } else if (op == "-") {
                        newValue = currentValue - operand;
                    } else if (op == "*") {
                        newValue = currentValue * operand;
                    } else if (op == "/" or op == "÷") {
                        if (operand == 0.0f) {
                            Print("Error: Cannot divide by 0!");
                            return;
                        }
                        newValue = currentValue / operand;
                    } else {
                        Usage(args);
                        return;
                    }

                } else if (args.Argc() == 6) {
                    // for additional arguments (_ = _ op _ with op in +, -, *, /)
                    float operand1 = std::stof(args.Argv(3));
                    const std::string& op = args.Argv(4);
                    float operand2 = std::stof(args.Argv(5));

                    if (op == "+") {
                        newValue = operand1 + operand2;
                    } else if (op == "-") {
                        newValue = operand1 - operand2;
                    } else if (op == "*") {
                        newValue = operand1 * operand2;
                    } else if (op == "/" or op == "÷") {
                        if (operand2 == 0.0f) {
                            Print("Error: Cannot divide by 0!");
                            return;
                        }
                        newValue = operand1 / operand2;
                    } else {
                        Usage(args);
                        return;
                    }

                } else {
                    Usage(args);
                    return;
                }

                Cvar::SetValue(targetName, std::to_string(newValue));
            }

            void Usage(const Cmd::Args& args) const{
                PrintUsage(args, "<variableToSet> = <number> <operator> <number>", "");
                PrintUsage(args, "<variableToSet> <operator> <number>", "");
                PrintUsage(args, "<variableToSet> (++|--)", "");
                Print("valid operators: + - × * ÷ /");
            }

            Cmd::CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const OVERRIDE {
                Q_UNUSED(args);

                if (argNum == 1) {
                    return Cvar::Complete(prefix);
                }

                return {};
            }
    };
    static MathCmd MathCmdRegistration;

    class IfCmd: public StaticCmd {
        public:
            IfCmd(): StaticCmd("if", BASE, "conditionally execute commands") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                if (args.Argc() != 5 and args.Argc() != 6) {
                    Usage(args);
                    return;
                }

                const std::string& value1 = args.Argv(1);
                const std::string& value2 = args.Argv(3);
                const std::string& relation = args.Argv(2);

                int intValue1, intValue2;

                bool result;

                if (relation == "eq") {
                    result = value1 == value2;

                } else if (relation == "ne") {
                    result = value1 != value2;

                } else if (relation == "in") {
                    result = value2.find(value1) != std::string::npos;

                } else if (relation == "!in") {
                    result = value2.find(value1) == std::string::npos;

                } else if (!Str::ParseInt(intValue1, value1) || !Str::ParseInt(intValue2, value2)) {
                    Usage(args);
                    return;
                

                } else if (relation == "=" or relation == "==") {
                    result = intValue1 == intValue2;

                } else if (relation == "!=" or relation == "≠") {
                    result = intValue1 != intValue2;

                } else if (relation == "<") {
                    result = intValue1 < intValue2;

                } else if (relation == "<=" or relation == "≤" ) {
                    result = intValue1 <= intValue2;

                } else if (relation == ">") {
                    result = intValue1 > intValue2;

                } else if (relation == ">=" or relation == "≥") {
                    result = intValue1 >= intValue2;

                } else {
                    Print( "invalid relation operator in if command. valid relation operators are = != ≠ < > ≥ >= ≤ <= eq ne in !in" );
                    Usage(args);
                    return;
                }

                if (!result && args.Argc() != 6) {
                    return;
                }

                const std::string& toRun = result ? args.Argv(4) : args.Argv(5);

                //if it starts with / or \ it is a quoted command
                if (toRun.size() > 0 and (toRun[0] == '/' or toRun[0] == '\\')) {
                    ExecuteAfter(toRun.c_str() + 1, true);

                } else {
                    std::string command = Cvar::GetValue(toRun);
                    ExecuteAfter(command, true);
                }
            }

            void Usage(const Cmd::Args& args) const{
                PrintUsage(args, "if <number|string> <relation> <number|string> <cmdthen> (<cmdelse>)", "compares two numbers or two strings and executes <cmdthen> if true, <cmdelse> if false\n");
                Print("-- commands are cvar names unless prefixed with / or \\");
            }

            Cmd::CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const OVERRIDE {
                Q_UNUSED(args);

                if (argNum == 4 or argNum == 5) {
                    return Cvar::Complete(prefix);
                }

                return {};
            }
    };
    static IfCmd IfCmdRegistration;

    class ToggleCmd: public Cmd::StaticCmd {
        public:
            ToggleCmd(): Cmd::StaticCmd("toggle", Cmd::BASE, "toggles a cvar between different values") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                if (args.Argc() < 2) {
                    Usage(args);
                    return;
                }

                const std::string& firstArg = args.Argv(1);

                int listStart = 2;
                int direction = 1;

                if (firstArg == "+") {
                    listStart = 3;
                } else if (firstArg == "-") {
                    listStart = 3;
                    direction = -1;
                }

                if (args.Argc() < listStart) {
                    Usage(args);
                    return;
                }

                const std::string& name = args.Argv(listStart - 1);

                if (args.Argc() == listStart) {
                    //There is no list, toggle between 0 and 1
                    bool value;
                    if (!Cvar::ParseCvarValue(Cvar::GetValue(name), value))
                        value = false;
                    Cvar::SetValue(name, va("%d", !value));
                    return;
                }

                //Toggle the cvar through a list of values
                std::string currentValue = Cvar::GetValue(name);

                for(int i = listStart; i < args.Argc(); i++) {
                    if(currentValue == args.Argv(i)) {
                        //Found the current value, choose the next one
                        int next = (i + direction) % (args.Argc() - listStart);

                        Cvar::SetValue(name, args.Argv(next + listStart));
                        return;
                    }
                }

                //fallback
                Cvar::SetValue(name, args.Argv(listStart));
            }

            Cmd::CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const OVERRIDE {
                Q_UNUSED(args);

                if (argNum == 1 or argNum == 2) {
                    return Cvar::Complete(prefix);
                }

                return {};
            }

            void Usage(const Cmd::Args& args) const{
                PrintUsage(args, "[+|-] <variable> [<value>…]", "");
            }
    };
    static ToggleCmd ToggleCmdRegistration;

    class CycleCmd: public Cmd::StaticCmd {
        public:
            CycleCmd(): Cmd::StaticCmd("cycle", Cmd::BASE, "cycles a cvar through numbers") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                int oldValue, start, end;
                if (args.Argc() < 4 || args.Argc() > 5 ||
                    !Str::ParseInt(oldValue, Cvar::GetValue(args.Argv(1))) ||
                    !Str::ParseInt(start, args.Argv(2)) ||
                    !Str::ParseInt(end, args.Argv(3))) {
                    PrintUsage(args, "<variable> <start> <end> [<step>]", "");
                    return;
                }

                int step;
                if (args.Argc() != 5 || !Str::ParseInt(step, args.Argv(4))) {
                    step = 1;
                }
                if (std::abs(end - start) < step) {
                    step = 1;
                }

                //TODO: rewrite all this nonsense
                int newValue;
                if (end < start) {
                    newValue = oldValue - step;
                    if (newValue < start) {
                        newValue = start - (step - (oldValue - end + 1));
                    }
                } else {
                    newValue = oldValue + step;
                    if (newValue > end) {
                        newValue = start + (step - (end - oldValue + 1));
                    }
                }

                Cvar::SetValue(args.Argv(1), va("%i", newValue));
            }

            Cmd::CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const OVERRIDE {
                Q_UNUSED(args);

                if (argNum == 1) {
                    return Cvar::Complete(prefix);
                }

                return {};
            }
    };
    static CycleCmd CycleCmdRegistration;

    /*
    ===============================================================================

    Cmd:: delay related commands

    ===============================================================================
    */

    enum delayType_t {
        MSEC,
        FRAME
    };

    struct delayRecord_t {
        std::string name;
        std::string command;
        Sys::SteadyClock::time_point targetTime;
        int targetFrame;
        delayType_t type;
    };

    static std::vector<delayRecord_t> delays;
    static int delayFrame = 0;

    void DelayFrame() {
        auto time = Sys::SteadyClock::now();
        delayFrame ++;

        for (auto it = delays.begin(); it != delays.end();) {
            const delayRecord_t& delay = *it;

            if ((delay.type == MSEC and delay.targetTime <= time) or (delay.type == FRAME and delay.targetFrame < delayFrame)) {
                Cmd::BufferCommandText(delay.command, true);
                it = delays.erase(it);
            } else {
                it ++;
            }
        }
    }

    Cmd::CompletionResult CompleteDelayName(Str::StringRef prefix) {
        Cmd::CompletionResult res;

        for (const auto& delay: delays) {
            if (Str::IsIPrefix(prefix, delay.name)) {
                res.push_back({delay.name, ""});
            }
        }

        return res;
    }

    class DelayCmd: public StaticCmd {
        public:
            DelayCmd(): StaticCmd("delay", BASE, "executes a command after a delay") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                int argc = args.Argc();

                if (argc < 3) {
		            PrintUsage(args, "delay (name) <delay in milliseconds> <command>\n  delay (name) <delay in frames>f <command>", "executes <command> after the delay" );
		            return;
                }

                //Get all the parameters!
                bool isNamed = !isdigit(args.Argv(1)[0]);
                const std::string& name = isNamed ? args.Argv(1) : "";
                const std::string& command = args.EscapedArgs(2 + isNamed);
                std::string delay = args.Argv(1 + isNamed);
                bool frames = delay.back() == 'f';

                if (frames) {
                    delay.erase(--delay.end()); //FIXME-gcc-4.6 delay.pop_back()
                }

                int target;
                if (!Str::ParseInt(target, delay) || target < 1) {
                    Print("delay: the delay must be a positive integer");
                    return;
                }

                if (frames) {
                    int targetFrame = target + delayFrame;
                    delays.emplace_back(delayRecord_t{name, command, {}, targetFrame, FRAME});
                } else {
                    auto targetTime = Sys::SteadyClock::now() + std::chrono::milliseconds(target);
                    delays.emplace_back(delayRecord_t{name, command, targetTime, 0, MSEC});
                }

            }

            Cmd::CompletionResult Complete(int argNum, const Args&, Str::StringRef prefix) const OVERRIDE {
                if (argNum == 1) {
                    return CompleteDelayName(prefix);
                }

                return {};
            }
    };

    class UndelayCmd: public StaticCmd {
        public:
            UndelayCmd(): StaticCmd("undelay", BASE, "removes named /delay commands") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                if (args.Argc() < 2) {
                    PrintUsage(args, "<name> (command)", "removes all commands with <name> in them.\nif (command) is specified, the removal will be limited only to delays whose commands contain (command).");
                    return;
                }

                const std::string& name = args.Argv(1);
                const std::string& command = (args.Argc() >= 3) ? args.EscapedArgs(2) : "";

                for (auto it = delays.begin(); it != delays.end();) {
                    const delayRecord_t& delay = *it;

                    if (Q_stristr(delay.name.c_str(), name.c_str()) and Q_stristr(delay.command.c_str(), command.c_str())) {
                        it = delays.erase(it);
                    } else {
                        it ++;
                    }
                }
            }

            Cmd::CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const OVERRIDE {
                Q_UNUSED(args);

                if (argNum == 1) {
                    return CompleteDelayName(prefix);
                }

                return {};
            }
    };

    class UndelayAllCmd: public StaticCmd {
        public:
            UndelayAllCmd(): StaticCmd("undelayAll", BASE, "removes all the pending /delay commands") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                Q_UNUSED(args); //TODO
                delays.clear();
            }
    };

    static DelayCmd DelayCmdRegistration;
    static UndelayCmd UndelayCmdRegistration;
    static UndelayAllCmd UndelayAllCmdRegistration;

    /*
    ===============================================================================

    Cmd:: alias related commands

    ===============================================================================
    */

    struct aliasRecord_t {
        std::string command;
        int lastRun;
    };

    static std::unordered_map<std::string, aliasRecord_t, Str::IHash, Str::IEqual> aliases;
    static int aliasRun;
    static bool inAliasRun;

    std::string GetAliasConfigText() {
        std::ostringstream result;
        result << "clearAliases\n";

        for (const auto& it: aliases) {
            result << "alias " << it.first << " " << it.second.command << "\n";
        }
        return result.str();
    }

    Cmd::CompletionResult CompleteAliasName(Str::StringRef prefix) {
        Cmd::CompletionResult res;

        for (const auto& it: aliases) {
            if (Str::IsIPrefix(prefix, it.first)) {
                res.push_back({it.first, ""});
            }
        }

        return res;
    }

    class AliasProxy: public CmdBase {
        public:
            AliasProxy(): CmdBase(0) {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                const std::string& name = args.Argv(0);
                const std::string& parameters = args.EscapedArgs(1);

                auto iter = aliases.find(name);
                if (iter == aliases.end()) {
                    Print("alias %s doesn't exist", name.c_str());
                    return;
                }

                aliasRecord_t& alias = iter->second;

                bool startsRun = not inAliasRun;
                if (startsRun) {
                    inAliasRun = true;
                    aliasRun ++;
                }

                if (alias.lastRun == aliasRun) {
                    Print("recursive alias call at alias %s", name.c_str());
                } else {
                    alias.lastRun = aliasRun;
                    ExecuteAfter(alias.command + " " + parameters, true);
                }

                if (startsRun) {
                    inAliasRun = false;
                }
            }
    };
    static AliasProxy aliasProxy;

    class AliasCmd: StaticCmd {
        public:
            AliasCmd(): StaticCmd("alias", BASE, "creates or view an alias") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                if (args.Argc() < 2) {
                    PrintUsage(args, "<name>", "show an alias");
                    PrintUsage(args, "<name> <exec>", "create an alias");
                    return;
                }

                const std::string& name = args.Argv(1);

                //Show an alias
                if (args.Argc() == 2) {
                    auto iter = aliases.find(name);
                    if (iter != aliases.end()) {
                        Print("%s ⇒ %s", name.c_str(), iter->second.command.c_str());
                    } else {
                        Print("Alias %s does not exist", name.c_str());
                    }
                    return;
                }

                //Modify or create an alias
                const std::string& command = args.ConcatArgs(2);

                auto iter = aliases.find(name);
                if (iter != aliases.end()) {
                    iter->second.command = command;
                    return;
                }

                if (CommandExists(name)) {
                    Print("Can't override a builtin function with an alias");
                    return;
                }

                if (!IsValidCmdName(name)) {
                    Print("Invalid alias name '%s'", name);
                    return;
                }

                aliases[name] = aliasRecord_t{command, aliasRun};
                AddCommand(name, aliasProxy, "a user-defined alias command");

                //Force an update of autogen.cfg (TODO: get rid of this super global variable)
                cvar_modifiedFlags |= CVAR_ARCHIVE_BITS;
            }

            Cmd::CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const OVERRIDE {
                if (argNum == 1) {
                    return CompleteAliasName(prefix);
                } else if (argNum > 1) {
                    return Cmd::CompleteArgument(Cmd::Args(args.EscapedArgs(2)), argNum - 2);
                }

                return {};
            }
    };
    static AliasCmd AliasCmdRegistration;

    class UnaliasCmd: StaticCmd {
        public:
            UnaliasCmd(): StaticCmd("unalias", BASE, "deletes an alias") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                if (args.Argc() != 2) {
                    PrintUsage(args, "<name>", "delete an alias");
                    return;
                }

                const std::string& name = args.Argv(1);

                auto iter = aliases.find(name);
                if (iter != aliases.end()) {
                    aliases.erase(iter);
                    RemoveCommand(name);
                }
            }

            Cmd::CompletionResult Complete(int argNum, const Args& args, Str::StringRef prefix) const OVERRIDE {
                Q_UNUSED(args);

                if (argNum == 1) {
                    return CompleteAliasName(prefix);
                }

                return {};
            }
    };
    static UnaliasCmd UnaliasCmdRegistration;

    class ClearAliasesCmd: StaticCmd {
        public:
            ClearAliasesCmd(): StaticCmd("clearAliases", BASE, "deletes all the aliases") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                Q_UNUSED(args); //TODO
                RemoveFlaggedCommands(ALIAS);
                aliases.clear();
            }
    };
    static ClearAliasesCmd ClearAliasesCmdRegistration;

    class ListAliasesCmd: StaticCmd {
        public:
            ListAliasesCmd(): StaticCmd("listAliases", BASE, "lists aliases") {
            }

            void Run(const Cmd::Args& args) const OVERRIDE {
                Q_UNUSED(args); //TODO
                std::string name;

                if (args.Argc() > 1) {
                    name = args.Argv(1);
                } else {
                    name = "";
                }

                std::vector<const aliasRecord_t*> matches;
                std::vector<const std::string*> matchesNames;
                unsigned long maxNameLength = 0;

                //Find all the matching aliases and their names
                for (auto it = aliases.cbegin(); it != aliases.cend(); ++it) {
                    if (Q_stristr(it->first.c_str(), name.c_str())) {
                        matches.push_back(&it->second);
                        matchesNames.push_back(&it->first);
                        maxNameLength = std::max<size_t>(maxNameLength, it->first.size());
                    }
                }

                //Print the matches, keeping the description aligned
                for (unsigned i = 0; i < matches.size(); i++) {
                    int toFill = maxNameLength - matchesNames[i]->size();
                    Print("  %s%s ⇒ %s", matchesNames[i]->c_str(), std::string(toFill, ' ').c_str(), matches[i]->command.c_str());
                }

                Print("%zu aliases", matches.size());
            }
    };
    static ListAliasesCmd ListAliasesCmdRegistration;

}
