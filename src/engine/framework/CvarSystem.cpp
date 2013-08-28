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
#include "../../shared/String.h"
#include "../../shared/Command.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../qcommon/cvar.h"

#include <unordered_map>

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

    typedef typename std::unordered_map<std::string, cvarRecord_t*> CvarMap;

    CvarMap& GetCvarMap() {
        static CvarMap* cvars = new CvarMap();
        return *cvars;
    }

    void SetValue(const std::string& cvarName, std::string value, int flags) {
        CvarMap& cvars = GetCvarMap();

        if (not cvars.count(cvarName)) {
            //The user creates a new cvar through a command.
            cvars[cvarName] = new cvarRecord_t{value, value, flags | CVAR_USER_CREATED, "user created", nullptr};
            //TODO add the command
            GetCCvar(cvarName, *cvars[cvarName]);

        } else {
            cvarRecord_t* cvar = cvars[cvarName];

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

        if (not cvars.count(name)) {
            //Create the cvar and parse its default value
            cvars[name] = new cvarRecord_t{defaultValue, defaultValue, flags, std::move(description), proxy};

            if (proxy) { //TODO replace me with an assert once we do not need to support the C API
                bool valueCorrect = proxy->OnValueChanged(defaultValue);
                if(not valueCorrect) {
                    Com_Printf(_("Default value '%s' is not correct for cvar '%s'\n"), defaultValue.c_str(), name.c_str());
                }
            }
            GetCCvar(name, *cvars[name]);

        } else {
            cvarRecord_t* cvar = cvars[name];

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

    void InnerSet(const std::string& name, std::string value) {
        CvarMap& cvars = GetCvarMap();

        if (cvars.count(name)) {
            cvarRecord_t* cvar = cvars[name];
            cvar->value = std::move(value);
            SetCCvar(name, *cvar);
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
}
