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

#include "../qcommon/q_shared.h"
#include "qcommon.h"

#include "../framework/CommandSystem.h"
#include "../framework/CvarSystem.h"
#include "../../shared/String.h"

#include <unordered_map>

cvar_t        *cvar_vars;
cvar_t        *cvar_cheats;
int           cvar_modifiedFlags;

#define MAX_CVARS 2048
cvar_t        cvar_indexes[ MAX_CVARS ];
int           cvar_numIndexes;

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
Cvar_VariableStringBuffer
============
*/
void Cvar_LatchedVariableStringBuffer(const char* name, char* buffer, int bufsize) {
    cvar_t* var = Cvar_FindVar(name);

    if (not var) {
        buffer[0] = '\0';
    } else {
        if (var->latchedString) {
            Q_strncpyz(buffer, var->latchedString, bufsize);
        } else {
            Q_strncpyz(buffer, var->string, bufsize);
        }
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
    Cvar::Register(nullptr, name, "a cvar created through the C API", flags, value);
//    Com_Printf("%i\n", Cvar_FindVar(name)->integer);
    return Cvar_FindVar(name);
}

/*
============
Cvar_Set
============
*/
void Cvar_Set(const char* name, const char* value) {
    Cvar::SetValue(name, value);
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
    Cvar_Set(name, var->resetString);
}

/*
============
Cvar_SetCheatState

Any testing variables will be reset to the safe values
============
*/
void Cvar_SetCheatState( void ) {
    //TODO
    /*
	cvar_t *var;

	// set all default vars to the safe value
	for ( var = cvar_vars; var; var = var->next )
	{
		if ( var->flags & CVAR_CHEAT )
		{
			if ( strcmp( var->resetString, var->string ) )
			{
				Cvar_Set( var->name, var->resetString );
			}
		}
	}
	*/
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
bool Cvar_Command(const Cmd::Args& args) {
    if (args.Argc() == 0) {
        return false;
    }

    cvar_t* var = Cvar_FindVar(args.Argv(0).c_str());

	if (!var) {
	    return false;
	}

    if (args.Argc() == 1) {
        //Just print the cvar
        Com_Printf(_("\"%s\" is \"%s^7\" default: \"%s^7\"\n"), var->name, var->string, var->resetString);

        if (var->latchedString) {
            Com_Printf(_("latched: \"%s\"\n"), var->latchedString);
        }
        return true;
    }

    // set the value if forcing isn't required
    Cvar_Set(var->name, args.Argv(1).c_str());
    return true;
}

/*
============
SetCmd
============
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

            if (unsafe and com_crashed != nullptr and com_crashed->integer != 0) {
                Com_Printf(_("%s is unsafe. Check com_crashed.\n"), name.c_str());
                return;
            }

            const std::string& value = args.Argv(nameIndex + 1);
            Cvar_Set(name.c_str(), value.c_str());

            cvar_t* var = Cvar_FindVar(name.c_str());
            var->flags |= flags;
        }

        std::vector<std::string> Complete(int pos, const Cmd::Args& args) const override{
            int argNum = args.PosToArg(pos);

            if (argNum == 1 or (argNum == 2 and args.Argv(1) == "-unsafe")) {
                return CVar::CompleteName(args.ArgPrefix(pos));
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

/*
============
ResetCmd
============
*/
class ResetCmd: public Cmd::StaticCmd {
    public:
        ResetCmd(): Cmd::StaticCmd("reset", Cmd::BASE, N_("resets a variable")) {
        }

        void Run(const Cmd::Args& args) const override {
            if (args.Argc() != 2) {
                PrintUsage(args, _("<variable>"), "");
            } else {
                Cvar_Reset(args.Argv(1).c_str());
            }
        }

        std::vector<std::string> Complete(int pos, const Cmd::Args& args) const override{
            int argNum = args.PosToArg(pos);

            if (argNum == 1) {
                return CVar::CompleteName(args.ArgPrefix(pos));
            }

            return {};
        }
};
static ResetCmd ResetCmdRegistration;

/*
============
ListCmd
============
*/
//TODO
/*
class ListCmd: public Cmd::StaticCmd {
    public:
        ListCmd(): Cmd::StaticCmd("listCvars", Cmd::BASE, N_("lists variables")) {
        }

        void Run(const Cmd::Args& args) const override {
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

            std::vector<cvar_t*> matches;
            int maxNameLength = 0;

            //Find all the matching cvars
            for (cvar_t* var = cvar_vars; var; var = var->next) {
                if (Q_stristr(var->name, match.c_str())) {
                    matches.push_back(var);
                    maxNameLength = MAX(maxNameLength, strlen(var->name));
                }
            }

            //Print the matches, keeping the flags and descriptions aligned
            for (auto var: matches) {
                std::string filler = std::string(maxNameLength - strlen(var->name), ' ');
                Com_Printf("  %s%s", var->name, filler.c_str());

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
                flags += (var->transient) ? "T" : " ";

                Com_Printf("%s ", flags.c_str());

                //TODO: the raw parameter is not handled, need a function to escape carets
                Com_Printf("%s\n", Cmd::Escape(var->string, true).c_str());
            }

            Com_Printf("%zu cvars\n", matches.size());
            Com_Printf("%i cvars indexed\n", cvar_numIndexes);
        }
};
static ListCmd ListCmdRegistration;
*/

/*
============
Cvar_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set that are not in a transient state.
============
*/
void Cvar_WriteVariables( fileHandle_t f )
{
    Cvar::WriteVariables(f);
}

/*
=====================
Cvar_InfoString
=====================
*/
char* Cvar_InfoString(int flag, qboolean big) {
    return Cvar::InfoString(flag, big);
}

/*
=====================
Cvar_InfoStringBuffer
=====================
*/
void Cvar_InfoStringBuffer(int bit, char* buff, int buffsize) {
	Q_strncpyz(buff, Cvar_InfoString( bit, qfalse), buffsize);
}

/*
=====================
Cvar_Register

basically a slightly modified Cvar_Get for the interpreted modules
=====================
*/

static std::unordered_map<int, std::string> vmCvarsIndices;

void Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags) {
	cvar_t* cv = Cvar_Get( varName, defaultValue, flags );
	vmCvarsIndices[cv->index] = cv->name;

	if (!vmCvar) {
		return;
	}

	vmCvar->handle = cv->index;
	vmCvar->modificationCount = -1;
	Cvar_Update(vmCvar);
}

/*
=====================
Cvar_Update

updates an interpreted modules' version of a cvar
=====================
*/
void Cvar_Update(vmCvar_t *vmCvar) {
    if (not vmCvarsIndices.count(vmCvar->handle)) {
		Com_Error( ERR_DROP, "Cvar_Update: handle %d out of range", (unsigned) vmCvar->handle );
    }

	cvar_t* cv = Cvar::FindCCvar(vmCvarsIndices[vmCvar->handle]);

	if (cv->modificationCount == vmCvar->modificationCount) {
		return;
	}

	if (!cv->string) {
		return; // variable might have been cleared by a cvar_restart
	}

	vmCvar->modificationCount = cv->modificationCount;

	// bk001129 - mismatches.
	if (strlen( cv->string ) + 1 > MAX_CVAR_VALUE_STRING) {
		Com_Error(ERR_DROP, "Cvar_Update: src %s length %lu exceeds MAX_CVAR_VALUE_STRING(%lu)",
		           cv->string, (unsigned long) strlen(cv->string), (unsigned long) sizeof(vmCvar->string));
	}

	// bk001212 - Q_strncpyz guarantees zero padding and dest[MAX_CVAR_VALUE_STRING-1]==0
	// bk001129 - paranoia. Never trust the destination string.
	// bk001129 - beware, sizeof(char*) is always 4 (for cv->string).
	//            sizeof(vmCvar->string) always MAX_CVAR_VALUE_STRING
	//Q_strncpyz( vmCvar->string, cv->string, sizeof( vmCvar->string ) ); // id
	Q_strncpyz(vmCvar->string, cv->string, MAX_CVAR_VALUE_STRING);

	vmCvar->value = cv->value;
	vmCvar->integer = cv->integer;
}

/*
============
Cvar_Init

Reads in all archived cvars
============
*/
void Cvar_Init( void )
{
	cvar_cheats = Cvar_Get( "sv_cheats", "1", CVAR_ROM | CVAR_SYSTEMINFO );
}

namespace CVar {
    std::vector<std::string> CompleteName(const std::string& prefix) {
        return Cvar::CompleteName(prefix);
    }
}
