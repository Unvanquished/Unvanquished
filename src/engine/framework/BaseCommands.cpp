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

}
