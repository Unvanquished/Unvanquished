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

#include "../../shared/Cvar.h"
#include "../../shared/Command.h"
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#ifndef FRAMEWORK_CVAR_SYSTEM_H_
#define FRAMEWORK_CVAR_SYSTEM_H_

namespace Cvar {

    // KEEP
    //CVAR_ARCHIVE, CVAR_*INFO, CVAR_ROM CVAR_CHEAT are kept

    // REMOVE EVENTUALLY
    //CVAR_UNSAFE, not sure <- no support for now
    //CVAR_USER_CREATED is kept for now but not really needed
    //CVAR_LATCH, CVAR_SHADER, CVAR_INIT are not longer supported, will be implemented by the proxy

    //CVAR_NORESTART is not used will be killed
    //CVAR_TEMP seems useless, don't put the CVAR_ARCHIVE and CVAR_CHEAT
    //CVAR_SERVERINFO_NOUPDATE is not used

    void SetValue(const std::string& cvarName, std::string value);
    void SetValueForce(const std::string& cvarName, std::string value);
    std::string GetValue(const std::string& cvarName);

    //////////INTERNAL API

    void Register(CvarProxy* proxy, const std::string& name, std::string description, int flags, const std::string& defaultValue);
    void Unregister(const std::string& cvarName);
    Cmd::CompletionResult Complete(const std::string& prefix);
    void AddFlags(const std::string& cvarName, int flags);

    //////////FUNCTION FOR THE C API

    cvar_t* FindCCvar(const std::string& cvarName);
    void WriteVariables(fileHandle_t f);
    char* InfoString(int flag, bool big);
    void SetValueCProxy(const std::string& cvarName, std::string value);

}

#endif // FRAMEWORK_CVAR_SYSTEM_H_
