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

#include "qcommon/qcommon.h"

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

    // Returns a list of cvars matching the prefix as well as their description
    Cmd::CompletionResult Complete(Str::StringRef prefix);

    // Alter flags, returns true if the variable exists
    bool AddFlags(const std::string& cvarName, int flags);
    bool ClearFlags(const std::string& cvarName, int flags);

    // Used by statically defined cvar.
    bool Register(CvarProxy* proxy, const std::string& name, std::string description, int flags, const std::string& defaultValue);
    void Unregister(const std::string& cvarName);

    // Used by the C API
    cvar_t* FindCCvar(const std::string& cvarName);
    std::string GetCvarConfigText();
	// DEPRECATED: Use PopulateInfoMap
    char* InfoString(int flag, bool big);
	void PopulateInfoMap(int flag, InfoMap& map);
    void SetValueCProxy(const std::string& cvarName, std::string value);

    void SetCheatsAllowed(bool allowed);

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
