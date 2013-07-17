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

#include "BaseCommands.h"

#include <list>

#include "CommandSystem.h"
#include "../../shared/Command.h"
#include "../qcommon/qcommon.h"

namespace Cmd {

    class VstrCmd: public StaticCmd {
        public:
            VstrCmd(): StaticCmd("vstr", BASE, N_("executes a variable command")) {
            }

            void Run(const Cmd::Args& args) const override {
                if (args.Argc() != 2) {
                    PrintUsage(args, _("<variablename>"), _("executes a variable command"));
                    return;
                }

                std::string command = Cvar_VariableString(args.Argv(1).c_str());
                Cmd::BufferCommandText(command, Cmd::AFTER, true);
            }
    };
    static VstrCmd VstrCmdRegistration;

    class ExecCmd: public StaticCmd {
        public:
            ExecCmd(const std::string& name, bool silent): StaticCmd(name, BASE, N_("executes a command file")), silent(silent) {
            }

            void Run(const Cmd::Args& args) const override {
                bool executeSilent = silent;
                bool failSilent = false;
                bool hasOptions = args.Argc() >= 3 and args.Argv(1).size() >= 2 and args.Argv(1)[0] == '-';

                //Check the syntax
                if (args.Argc() < 2) {
                    if (silent) {
                        PrintUsage(args, _("<filename> [<arguments>…]"), _("execute a script file without notification, a shortcut for exec -q"));
                    } else {
                        PrintUsage(args, _("[-q|-f|-s] <filename> [<arguments>…]"), _("execute a script file."));
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
                if (not ExecFile(filename)) {
                    if (not failSilent) {
                        Com_Printf(_("couldn't exec '%s'\n"), filename.c_str());
                    }
                    return;
                }

                if (not executeSilent) {
                    Com_Printf(_("execing '%s'\n"), filename.c_str());
                }
            }

        private:
            bool silent;

            void SetExecArgs(const Cmd::Args& args, int start) const {
                //Set some cvars up so that scripts file can be used like functions
                Cvar_Get("arg_all", args.RawArgsFrom(start).c_str(), CVAR_TEMP | CVAR_ROM | CVAR_USER_CREATED);
                Cvar_Set("arg_all", args.RawArgsFrom(start).c_str());
                Cvar_Get("arg_count", va("%i", args.Argc() - start), CVAR_TEMP | CVAR_ROM | CVAR_USER_CREATED);
                Cvar_Set("arg_count", va("%i", args.Argc() - start));

                for (int i = start; i < args.Argc(); i++) {
                    Cvar_Get(va("arg_%i", i), args.Argv(i + start - 1).c_str(), CVAR_TEMP | CVAR_ROM | CVAR_USER_CREATED);
                    Cvar_Set(va("arg_%i", i), args.Argv(i + start - 1).c_str());
                }
            }

            bool ExecFile(const std::string& filename) const {
                bool success = false;
                fileHandle_t h;
                int len = FS_SV_FOpenFileRead(filename.c_str(), &h);

                //TODO: rewrite this
                if (h) {
                    success = true;
                    char* content = (char*) Hunk_AllocateTempMemory(len + 1);
                    FS_Read(content, len, h);
                    content[len] = '\0';
                    FS_FCloseFile(h);
                    Cmd::BufferCommandText(content, Cmd::AFTER, true);
                    Hunk_FreeTempMemory(content);
                } else {
                    char* content;
                    FS_ReadFile(filename.c_str(), (void**) &content);

                    if (content) {
                        success = true;
                        Cmd::BufferCommandText(content, Cmd::AFTER, true);
                        FS_FreeFile(content);
                    }
                }
                return success;
            }
    };
    static ExecCmd ExecCmdRegistration("exec", false);
    static ExecCmd ExecqCmdRegistration("execq", true);

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
        int target;
        delayType_t type;
    };

    std::list<delayRecord_t> delays;
    int delayFrame = 0;

    void DelayFrame() {
        int time = Sys_Milliseconds();
        delayFrame ++;

        //We store delays to execute in a second list to avoid problems with delays that create delays
        std::list<delayRecord_t> toRun;

        for (auto it = delays.begin(); it != delays.end();) {
            const delayRecord_t& delay = *it;

            if ((delay.type == MSEC and delay.target <= time) or (delay.type == FRAME and delay.target < delayFrame)) {
                toRun.splice(toRun.begin(), delays, it ++);
            }
            it ++;
        }

        for (auto& delay : toRun) {
            Cmd::BufferCommandText(delay.command);
        }
    }

    class DelayCmd: public StaticCmd {
        public:
            DelayCmd(): StaticCmd("delay", BASE, N_("executes a command after a delay")) {
            }

            void Run(const Cmd::Args& args) const override {
                int argc = args.Argc();

                if (argc < 3 or argc > 4) {
		            PrintUsage(args, _( "delay (name) <delay in milliseconds> <command>\n  delay <delay in frames>f <command>"), _("executes <command> after the delay\n" ));
		            return;
                }

                //Get all the parameters!
                const std::string& command = args.Argv(argc - 1);
                std::string delay = args.Argv(argc - 2);
                int target = std::stoi(delay);
                const std::string& name = (argc == 4) ? args.Argv(1) : "";
                delayType_t type;

                if (target < 1) {
                    Com_Printf(_("delay: the delay must be a positive integer\n"));
                    return;
                }

                if (delay[delay.size() - 1] == 'f') {
                    target += delayFrame;
                    type = FRAME;
                } else {
                    target += Sys_Milliseconds();
                    type = MSEC;
                }

                delays.emplace_back(delayRecord_t{name, command, target, type});
            }
    };

    class UndelayCmd: public StaticCmd {
        public:
            UndelayCmd(): StaticCmd("undelay", BASE, N_("removes named /delay commands")) {
            }

            void Run(const Cmd::Args& args) const override {
                if (args.Argc() < 2) {
                    PrintUsage(args, _("<name> (command)"), _( "removes all commands with <name> in them.\nif (command) is specified, the removal will be limited only to delays whose commands contain (command)."));
                    return;
                }

                const std::string& name = args.Argv(1);
                const std::string& command = (args.Argc() >= 3) ? args.Argv(2) : "";

                for (auto it = delays.begin(); it != delays.end();) {
                    const delayRecord_t& delay = *it;

                    if (Q_stristr(delay.name.c_str(), name.c_str()) and Q_stristr(delay.command.c_str(), command.c_str())) {
                        delays.erase(it ++);
                    }
                    it ++;
                }
            }
    };

    class UndelayAllCmd: public StaticCmd {
        public:
            UndelayAllCmd(): StaticCmd("undelayAll", BASE, N_("removes all the pending /delay commands")) {
            }

            void Run(const Cmd::Args& args) const override {
                delays.clear();
            }
    };

    static DelayCmd DelayCmdRegistration;
    static UndelayCmd UndelayCmdRegistration;
    static UndelayAllCmd UndelayAllCmdRegistration;

}
