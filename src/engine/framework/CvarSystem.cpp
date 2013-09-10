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

#include "CvarSystem.h"
#include "CommandSystem.h"
#include "../../shared/String.h"
#include "../../shared/Command.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../qcommon/cvar.h"

#include <unordered_map>

void f(int a) {
    Com_Printf("my_int's new value is %i\n", a);
}

Cvar::Callback<Cvar::Cvar<int>> my_int("my_int", f, "awesome cvar", 0, "42");

//TODO: thread safety (not possible with the C API that doesn't care at all about this)

namespace Cvar {

    struct cvarRecord_t {
        std::string value;
        std::string resetValue;
        int flags;
        std::string description;
        CvarProxy* proxy;
        cvar_t ccvar;
        //DO: mutex?
    };

    void SetCCvar(const std::string& name, cvarRecord_t& cvar) {
        cvar_t& var = cvar.ccvar;

        if (cvar.flags & CVAR_LATCH) {
            if (not var.string) {
                var.string = CopyString(cvar.value.c_str());
            } else {
                if (var.latchedString) Z_Free(var.latchedString);
                var.latchedString = CopyString(cvar.value.c_str());
            }
        } else {
            if (var.string) Z_Free(var.string);
            var.string = CopyString(cvar.value.c_str());
        }

        var.modified = qtrue;
        var.modificationCount++;
        var.value = atof(var.string);
        var.integer = atoi(var.string);

        cvar_modifiedFlags |= var.flags;
    }

    static int registrationCount = 0;
    void GetCCvar(const std::string& name, cvarRecord_t& cvar) {
        cvar_t& var = cvar.ccvar;

        if (not var.name) {
            var.name = CopyString(name.c_str());
            var.index = registrationCount ++;
            var.modificationCount = -1;
        }

        if (var.resetString) Z_Free(var.resetString);
        var.resetString = CopyString(cvar.resetValue.c_str());

        var.flags = cvar.flags;

        if (cvar.flags & CVAR_LATCH and var.latchedString) {
            if (var.string) Z_Free(var.string);
            var.string = var.latchedString;
            var.latchedString = 0;
        } else {
            if (var.string) Z_Free(var.string);
            var.string = CopyString(cvar.value.c_str());
        }

        var.modified = qtrue;
        var.modificationCount++;
        var.value = atof(var.string);
        var.integer = atoi(var.string);

        cvar_modifiedFlags |= var.flags;
    }

    typedef std::unordered_map<std::string, cvarRecord_t*> CvarMap;

    CvarMap& GetCvarMap() {
        static CvarMap* cvars = new CvarMap();
        return *cvars;
    }

    class CvarCommand : public Cmd::CmdBase {
        public:
            CvarCommand() : Cmd::CmdBase(Cmd::CVAR) {
            }

            void Run(const Cmd::Args& args) const override{
                CvarMap& cvars = GetCvarMap();
                const std::string& name = args.Argv(0);
                cvarRecord_t* var = cvars[name];

                if (args.Argc() < 2) {
                    Com_Printf(_("\"%s\" is \"%s^7\" default: \"%s^7\"\n"), name.c_str(), var->value.c_str(), var->resetValue.c_str());
                } else {
                    SetValue(name, args.Argv(1));
                }
            }
    };
    static CvarCommand cvarCommand;

    void SetValue(const std::string& cvarName, std::string value, int flags) {
        CvarMap& cvars = GetCvarMap();

        auto it = cvars.find(cvarName);
        if (it == cvars.end()) {
            //The user creates a new cvar through a command.
            cvars[cvarName] = new cvarRecord_t{value, value, flags | CVAR_USER_CREATED, "user created", nullptr};
            Cmd::AddCommand(cvarName, cvarCommand, "user created");
            GetCCvar(cvarName, *cvars[cvarName]);

        } else {
            cvarRecord_t* cvar = it->second;

            //TODO: do not update constant cvars
            /*if (cvar->flags & CVAR_ROM) {
                Com_Printf(_("%s is read only.\n"), cvarName.c_str());
                return;
            }*/

            //TODO: handle CVAR_CHEAT

            std::string oldValue = std::move(cvar->value);
            cvar->value = std::move(value);
            cvar->flags |= flags;

            if (cvar->proxy) {
                //Tell the cvar proxy about the new value
                bool valueCorrect = cvar->proxy->OnValueChanged(cvar->value);

                if (not valueCorrect) {
                    //The proxy could not parse the value, rollback
                    Com_Printf("Value '%s' is not valid for cvar %s\n", cvar->value.c_str(), cvarName.c_str());
                    cvar->value = std::move(oldValue);
                }
            }
            SetCCvar(cvarName, *cvar);
        }

    }

    std::string GetValue(const std::string& cvarName) {
        CvarMap& cvars = GetCvarMap();
        std::string result = "";

        if (cvars.count(cvarName)) {
            result = cvars[cvarName]->value;
        }

        return result;
    }

    //TODO create command
    void Register(CvarProxy* proxy, const std::string& name, std::string description, int flags, const std::string& defaultValue) {
        CvarMap& cvars = GetCvarMap();

        auto it = cvars.find(name);
        if (it == cvars.end()) {
            //Create the cvar and parse its default value
            cvars[name] = new cvarRecord_t{defaultValue, defaultValue, flags, description, proxy};

            if (proxy) { //TODO replace me with an assert once we do not need to support the C API
                bool valueCorrect = proxy->OnValueChanged(defaultValue);
                if(not valueCorrect) {
                    Com_Printf(_("Default value '%s' is not correct for cvar '%s'\n"), defaultValue.c_str(), name.c_str());
                }
            }
            GetCCvar(name, *cvars[name]);
            Cmd::AddCommand(name, cvarCommand, std::move(description));

        } else {
            cvarRecord_t* cvar = it->second;

            if (cvar->proxy) {
                Com_Printf(_("Cvar %s cannot be registered twice\n"), name.c_str());
            }

            //Register the cvar with the previous user_created value
            cvar->flags &= ~CVAR_USER_CREATED;
            cvar->flags |= flags;

            cvar->resetValue = std::move(defaultValue);
            cvar->description = std::move(description);

            /*
            if (cvar->flags & CVAR_ROM) {
                cvar->value = cvar->resetValue;
            }
            */

            if (proxy) { //TODO replace me with an assert once we do not need to support the C API
                bool valueCorrect = proxy->OnValueChanged(cvar->value);
                if(not valueCorrect) {
                    bool defaultValueCorrect = proxy->OnValueChanged(defaultValue);
                    if(not defaultValueCorrect) {
                        Com_Printf(_("Default value '%s' is not correct for cvar '%s'\n"), defaultValue.c_str(), name.c_str());
                    }
                }
            }
            GetCCvar(name, *cvars[name]);
        }
    }

    void Unregister(const std::string& name) {
        CvarMap& cvars = GetCvarMap();

        if (cvars.count(name)) {
            cvarRecord_t* cvar = cvars[name];

            cvar->proxy = nullptr;
            cvar->flags |= CVAR_USER_CREATED;
        } //TODO else what?
    }

    std::vector<std::string> CompleteName(const std::string& prefix) {
        CvarMap& cvars = GetCvarMap();

        std::vector<std::string> res;
        for (auto it : cvars) {
            if (Str::IsPrefix(prefix, it.first)) {
                res.push_back(it.first);
            }
        }

        return res;
    }

    ///////////////

    cvar_t* FindCCvar(const std::string& cvarName) {
        CvarMap& cvars = GetCvarMap();

        if (cvars.count(cvarName)) {
            return &cvars[cvarName]->ccvar;
        }

        return nullptr;
    }

    void WriteVariables(fileHandle_t f) {
        CvarMap& cvars = GetCvarMap();

        for (auto it : cvars) {
            cvarRecord_t* cvar = it.second;

            if (cvar->flags & CVAR_ARCHIVE) {
                FS_Printf(f, "seta %s %s\n", it.first.c_str(), Cmd::Escape(cvar->value, true).c_str());
            }
        }
    }

    char* InfoString(int flag, bool big) {
        static char info[BIG_INFO_STRING];
        info[0] = 0;

        CvarMap& cvars = GetCvarMap();

        for (auto it : cvars) {
            cvarRecord_t* cvar = it.second;

            if (cvar->flags & flag) {
                Info_SetValueForKey(info, it.first.c_str(), cvar->value.c_str(), big);
            }
        }

        return info;
    }

    /*
    ===============================================================================

    Cvar:: Cvar-related commands

    ===============================================================================
    */

    class SetCmd: public Cmd::StaticCmd {
        public:
            SetCmd(const std::string& name, int flags): Cmd::StaticCmd(name, Cmd::BASE, N_("sets the value of a cvar")), flags(flags) {
            }

            void Run(const Cmd::Args& args) const override{
                int argc = args.Argc();
                int nameIndex = 1;
                bool unsafe = false;

                if (argc < 3) {
                    Com_Printf("'%s'\n", args.RawCmd().c_str());
                    PrintUsage(args, _("[-unsafe] <variable> <value>"), "");
                    return;
                }

                if (argc >= 4 and args.Argv(1) == "-unsafe") {
                    nameIndex = 2;
                    unsafe = true;
                }

                const std::string& name = args.Argv(nameIndex);

                //TODO
                if (unsafe and com_crashed != nullptr and com_crashed->integer != 0) {
                    Com_Printf(_("%s is unsafe. Check com_crashed.\n"), name.c_str());
                    return;
                }

                const std::string& value = args.Argv(nameIndex + 1);

                //TODO no flags in the argument
                ::Cvar::SetValue(name, value, flags);
            }

            std::vector<std::string> Complete(int pos, const Cmd::Args& args) const override{
                int argNum = args.PosToArg(pos);

                if (argNum == 1 or (argNum == 2 and args.Argv(1) == "-unsafe")) {
                    return ::Cvar::CompleteName(args.ArgPrefix(pos));
                }

                return {};
            }

        private:
            int flags;
    };
    static SetCmd SetCmdRegistration("set", 0);
    static SetCmd SetuCmdRegistration("setu", CVAR_USERINFO);
    static SetCmd SetsCmdRegistration("sets", CVAR_SERVERINFO);
    static SetCmd SetaCmdRegistration("seta", CVAR_ARCHIVE);

    class ResetCmd: public Cmd::StaticCmd {
        public:
            ResetCmd(): Cmd::StaticCmd("reset", Cmd::BASE, N_("resets a variable")) {
            }

            void Run(const Cmd::Args& args) const override {
                if (args.Argc() != 2) {
                    PrintUsage(args, _("<variable>"), "");
                    return;
                }

                const std::string& name = args.Argv(1);
                CvarMap& cvars = GetCvarMap();

                if (cvars.count(name)) {
                    cvarRecord_t* cvar = cvars[name];
                    ::Cvar::SetValue(name, cvar->resetValue);
                } else {
                    Com_Printf(_("Cvar '%s' doesn't exist\n"), name.c_str());
                }
            }

            std::vector<std::string> Complete(int pos, const Cmd::Args& args) const override{
                int argNum = args.PosToArg(pos);

                if (argNum == 1) {
                    return ::Cvar::CompleteName(args.ArgPrefix(pos));
                }

                return {};
            }
    };
    static ResetCmd ResetCmdRegistration;

    //TODO: print the description
    class ListCvars: public Cmd::StaticCmd {
        public:
            ListCvars(): Cmd::StaticCmd("listCvars", Cmd::BASE, N_("lists variables")) {
            }

            void Run(const Cmd::Args& args) const override {
                CvarMap& cvars = GetCvarMap();

                bool raw;
                std::string match = "";

                //Read parameters
                if (args.Argc() > 1) {
                    match = args.Argv(1);
                    if (match == "-raw") {
                        raw = true;
                        match = (args.Argc() > 2) ? args.Argv(2) : "";
                    }
                }

                std::vector<cvarRecord_t*> matches;
                std::vector<std::string> matchesNames;
                int maxNameLength = 0;

                //Find all the matching cvars
                for (auto& record : cvars) {
                    if (Q_stristr(record.first.c_str(), match.c_str())) {
                        matchesNames.push_back(record.first);
                        matches.push_back(record.second);
                        maxNameLength = MAX(maxNameLength, record.first.length());
                    }
                }

                //Print the matches, keeping the flags and descriptions aligned
                for (int i = 0; i < matches.size(); i++) {
                    const std::string& name = matchesNames[i];
                    cvarRecord_t* var = matches[i];

                    std::string filler = std::string(maxNameLength - name.length(), ' ');
                    Com_Printf("  %s%s", name.c_str(), filler.c_str());

                    std::string flags = "";
                    flags += (var->flags & CVAR_SERVERINFO) ? "S" : "_";
                    flags += (var->flags & CVAR_SYSTEMINFO) ? "s" : "_";
                    flags += (var->flags & CVAR_USERINFO) ? "U" : "_";
                    flags += (var->flags & CVAR_ROM) ? "R" : "_";
                    flags += (var->flags & CVAR_INIT) ? "I" : "_";
                    flags += (var->flags & CVAR_ARCHIVE) ? "A" : "_";
                    flags += (var->flags & CVAR_LATCH) ? "L" : "_";
                    flags += (var->flags & CVAR_CHEAT) ? "C" : "_";
                    flags += (var->flags & CVAR_USER_CREATED) ? "?" : "_";

                    Com_Printf("%s ", flags.c_str());

                    //TODO: the raw parameter is not handled, need a function to escape carets
                    Com_Printf("%s\n", Cmd::Escape(var->value, true).c_str());
                }

                Com_Printf("%zu cvars\n", matches.size());
            }
    };
    static ListCvars ListCvarsRegistration;

}
