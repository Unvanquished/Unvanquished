/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

// cvar.c -- dynamic variable tracking

#include "qcommon/q_shared.h"
#include "qcommon.h"

#include "framework/CommandSystem.h"
#include "framework/CvarSystem.h"

cvar_t        *cvar_vars;

/*
============
Cvar_FindVar
============
*/
static cvar_t* Cvar_FindVar(const char* name) {
    return Cvar::FindCCvar(name);
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue(const char* name) {
    cvar_t* var = Cvar_FindVar(name);
    return var ? var->value : 0.0f;
}

/*
============
Cvar_VariableIntegerValue
============
*/
int Cvar_VariableIntegerValue( const char* name ) {
    cvar_t* var = Cvar_FindVar(name);
    return var ? var->integer : 0;
}

/*
============
Cvar_VariableString
============
*/
char* Cvar_VariableString(const char* name) {
    cvar_t* var = Cvar_FindVar(name);
    return var ? var->string : (char*)"";
}

/*
============
Cvar_VariableStringBuffer
============
*/
void Cvar_VariableStringBuffer(const char* name, char* buffer, int bufsize) {
    cvar_t* var = Cvar_FindVar(name);

    if (not var) {
        buffer[0] = '\0';
    } else {
        Q_strncpyz(buffer, var->string, bufsize);
    }
}

/*
============
Cvar_Get

If the variable already exists, the value will not be set unless CVAR_ROM
The flags will be or'ed in if the variable exists.
============
*/
cvar_t *Cvar_Get(const char* name, const char* value, int flags) {
    Cvar::Register(nullptr, name, "--", flags, value);
    return Cvar_FindVar(name);
}

/*
============
Cvar_Set
============
*/
void Cvar_Set(const char* name, const char* value) {
    if (value == nullptr) {
        Cvar_Reset(name);
    } else {
        Cvar::SetValueCProxy(name, value);
    }
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue(const char* name, float value) {
	if (value == (int) value) {
        Cvar_Set(name, va("%i", (int) value));
	} else {
        Cvar_Set(name, va("%f", value));
	}
}

/*
============
Cvar_Reset
============
*/
void Cvar_Reset(const char* name) {
    cvar_t* var = Cvar::FindCCvar(name);
    if (var != nullptr) {
        Cvar_Set(name, var->resetString);
    }
}

/*
============
Cvar_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set that are not in a transient state.
============
*/
void Cvar_WriteVariables( fileHandle_t f )
{
    std::string text = Cvar::GetCvarConfigText();
    FS_Write(text.c_str(), text.size(), f);
}

/*
=====================
Cvar_InfoString
=====================
*/
char* Cvar_InfoString(int flag, bool big) {
    return Cvar::InfoString(flag, big);
}
