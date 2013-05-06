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
#ifndef CVAR_H_
#define CVAR_H_

/**
 * cvar_t variables are used to hold scalar or string variables that can be changed
 * or displayed at the console or prog code as well as accessed directly
 * in C code.
 * The user can basically access cvars from the console in three ways:
 * <name>             — prints the current value of the named cvar, or says that the cvar does not exist
 * (unless the name is actually the name of a command, in which case the command is executed)
 * <name> <value>     — sets the value of the cvar if the cvar exists (unless, see above)
 * set <name> <value> — sets the value of the cvar, but creates the cvar if it does not exist yet
 * They are also occasionally used to communicate information between different modules of the program.
 *
 * nothing outside the Cvar_*() functions should modify these fields!
 */
typedef struct cvar_s
{
	char *name;
	char *string;
	char *resetString; // cvar_restart will reset to this value
	char *latchedString; // for CVAR_LATCH vars
	int flags;
	qboolean modified; // set each time the cvar is changed
	int modificationCount; // incremented each time the cvar is changed
	float value; // atof( string )
	int integer; // atoi( string )

	/**
	 * indicate whether the cvar won't be archived, even if it's an ARCHIVE flagged cvar.
	 * this allows us to keep ARCHIVE cvars unwritten to autogen until a user changes them
	 */
	qboolean transient;

	qboolean validate;
	qboolean integral;
	float min;
	float max;

	struct cvar_s *next;

	struct cvar_s *hashNext;
} cvar_t;

/**
 * creates the variable if it doesn't exist, or returns the existing one if it exists.
 * the value will not be changed, but flags will be ORed in that allows variables
 * to be unarchived without needing bitflags if value is "",
 * the value will not override a previously set value.
 */
cvar_t *Cvar_Get(const char *var_name, const char *value, int flags);

/**
 * basically a slightly modified Cvar_Get for the interpreted modules
 */
void Cvar_Register(vmCvar_t *vmCvar, const char *varName,
		const char *defaultValue, int flags);

/**
 * updates a module's version of a cvar
 */
void Cvar_Update(vmCvar_t *vmCvar);

/**
 * will create the variable with no flags if it doesn't exist
 */
void Cvar_Set(const char *var_name, const char *value);

/**
 * sets the cvar by the name that begins with a backslash to "1"
 */
void Cvar_SetIFlag(const char *var_name);

/**
 * don't set the cvar immediately
 */
void Cvar_SetLatched(const char *var_name, const char *value);

/**
 * expands value to a string and calls Cvar_Set
 */
void Cvar_SetValue(const char *var_name, float value);
void Cvar_SetValueLatched(const char *var_name, float value);

// returns 0 if not defined or non numeric
float Cvar_VariableValue(const char *var_name);
int Cvar_VariableIntegerValue(const char *var_name);

// returns an empty string if not defined
char *Cvar_VariableString(const char *var_name);
void Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);


/**
 * returns the latched value if there is one, and the current value otherwise
 * (an empty string in case the cvar does not exist)
 */
void Cvar_LatchedVariableStringBuffer(const char *var_name, char *buffer,
		int bufsize);

int Cvar_Flags(const char *var_name);

// callback with each valid string
void Cvar_CommandCompletion(void (*callback)(const char *s));


// reset all testing vars to a safe value
void Cvar_Reset(const char *var_name);

void Cvar_SetCheatState(void);

/**
 * called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
 * command.  Returns true if the command was a variable reference that
 * was handled. (print or change)
 */
qboolean Cvar_Command(void);

/**
 * writes lines containing "set variable value" for all variables
 * with the archive flag set to true.
 */
void Cvar_WriteVariables(fileHandle_t f);

void Cvar_CompleteCvarName(char *args, int argNum);
void Cvar_Init(void);

/**
 * returns an info string containing all the cvars that have the given bit set
 * in their flags ( CVAR_USERINFO, CVAR_SERVERINFO, CVAR_SYSTEMINFO, etc )
 */
char *Cvar_InfoString(int bit, qboolean big);

void Cvar_InfoStringBuffer(int bit, char *buff, int buffsize);

void Cvar_CheckRange(cvar_t *cv, float minVal, float maxVal, qboolean shouldBeIntegral);

void Cvar_Restart_f(void);
void Cvar_Clean_f(void);


/**
 *  whenever a cvar is modified, its flags will be OR'd into this, so
 *  a single check can determine if any CVAR_USERINFO, CVAR_SERVERINFO,
 *  etc, variables have been modified since the last check.  The bit
 *  can then be cleared to allow another change detection.
 */
extern int cvar_modifiedFlags;
#ifndef DEDICATED
extern qboolean bindingsModified;
#endif


#endif /* CVAR_H_ */
