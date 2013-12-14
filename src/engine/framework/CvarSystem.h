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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#ifndef FRAMEWORK_CVAR_SYSTEM_H_
#define FRAMEWORK_CVAR_SYSTEM_H_

/**
 * Cvar access and registration.
 *
 * Console Variables (Cvars) are values that are accessible by the user
 * (shown and modifiable in the console) as well as directly accessible
 * in the code. They are mostly used to expose parameters that can be
 * changed at runtime or saved/loaded from configuration files.
 *
 * Sometimes modules use cvars to communicate between each other but it
 * is bad practice.
 *
 * Cvars should be declared statically using the Cvar::Cvar class, see
 * common/Cvar for more details. Facilites are also provided to do more
 * "static checking" of cvars with types, bounds, ...
 *
 * There are two ways for the code to access cvars:
 *  - Through a Cvar::Cvar<Type> using .Get() and .Set() you then get
 *  parsing and serialization and other things for free. This type of
 *  cvar is used to control the behavior of local code (filename,
 *  fps limit, ...)
 *  - Through SetValue and GetValue when the cvar doesn't belong to
 *  the local code, for example when doing commands (/set ...)
 */

//TODO add callbacks for when cvars are modified

namespace Cvar {

    // Generic ways to access cvars, might specialize it to parse and serialize automatically

    void SetValue(const std::string& cvarName, std::string value);
    //Used for ROM cvars, will trigger a warning if the cvar is not ROM
    void SetValueForce(const std::string& cvarName, std::string value);
    std::string GetValue(const std::string& cvarName);

    //Returns a list of cvars matching the prefix as well as their description
    Cmd::CompletionResult Complete(Str::StringRef prefix);

    void AddFlags(const std::string& cvarName, int flags);

    // Used by statically defined cvar.
    void Register(CvarProxy* proxy, const std::string& name, std::string description, int flags, const std::string& defaultValue);
    void Unregister(const std::string& cvarName);

    // Used by the C API
    cvar_t* FindCCvar(const std::string& cvarName);
    void WriteVariables(fileHandle_t f);
    char* InfoString(int flag, bool big);
    void SetValueCProxy(const std::string& cvarName, std::string value);

    //Kept as a reference for cvar flags

    // Keep
    //CVAR_ARCHIVE, CVAR_*INFO, CVAR_ROM CVAR_CHEAT are kept

    // Remove eventually
    //CVAR_UNSAFE, not sure <- no support for now
    //CVAR_USER_CREATED is kept for now but not really needed
    //CVAR_LATCH, CVAR_SHADER, CVAR_INIT are not longer supported, will be implemented by the proxy

    //CVAR_NORESTART is not used will be killed
    //CVAR_TEMP seems useless, don't put the CVAR_ARCHIVE and CVAR_CHEAT
    //CVAR_SERVERINFO_NOUPDATE is not used
}

#endif // FRAMEWORK_CVAR_SYSTEM_H_
