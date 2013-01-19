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

// cmd_c -- Quake script command processing module

#include "../qcommon/q_shared.h"
#include "qcommon.h"
#include "../client/keys.h"

#define MAX_CMD_BUFFER 131072
#define MAX_CMD_LINE   1024

typedef struct
{
	byte *data;
	int  maxsize;
	int  cursize;
} cmd_t;

int   cmd_wait;
cmd_t cmd_text;
byte  cmd_text_buf[ MAX_CMD_BUFFER ];

// Delay stuff

#define MAX_DELAYED_COMMANDS 64
#define CMD_DELAY_FRAME_FIRE 1
#define CMD_DELAY_UNUSED     0

typedef enum
{
  CMD_DELAY_MSEC,
  CMD_DELAY_FRAME
} cmdDelayType_t;

typedef struct
{
	char           name[ MAX_CMD_LINE ];
	char           text[ MAX_CMD_LINE ];
	int            delay;
	cmdDelayType_t type;
} delayed_cmd_s;

delayed_cmd_s delayed_cmd[ MAX_DELAYED_COMMANDS ];

typedef struct cmd_function_s
{
	struct cmd_function_s *next;

	char                  *name;
#ifdef DEDICATED
	xcommand_t        function;
#else
	union {
		xcommand_t        function;
		xcommand_arg_t    buttonFunction;
	};
	qboolean              isButtonCommand;
#endif
	int                   parameter;
	completionFunc_t      complete;
} cmd_function_t;

typedef struct cmdContext_s
{
	int  argc;
	char *argv[ MAX_STRING_TOKENS ]; // points into cmd.tokenized
	char tokenized[ BIG_INFO_STRING + MAX_STRING_TOKENS ]; // will have 0 bytes inserted
	char cmd[ BIG_INFO_STRING ]; // the original command we received (no token processing)
} cmdContext_t;

static cmdContext_t cmd;
static cmdContext_t savedCmd;

// static int      cmd.argc;
// static char    *cmd.argv[MAX_STRING_TOKENS]; // points into cmd.tokenized
// static char     cmd.tokenized[BIG_INFO_STRING + MAX_STRING_TOKENS];  // will have 0 bytes inserted
// static char     cmd.cmd[BIG_INFO_STRING];  // the original command we received (no token processing)

static cmd_function_t *cmd_functions; // possible commands to execute

//=============================================================================

/*
============
Cmd_FindCommand
============
*/
cmd_function_t *Cmd_FindCommand( const char *cmd_name )
{
	cmd_function_t *cmd;

	for ( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if ( !Q_stricmp( cmd_name, cmd->name ) )
		{
			return cmd;
		}
	}

	return NULL;
}

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "cmd use rocket ; +attack ; wait ; -attack ; cmd use blaster"
============
*/
void Cmd_Wait_f( void )
{
	if ( Cmd_Argc() == 2 )
	{
		cmd_wait = atoi( Cmd_Argv( 1 ) );

		if ( cmd_wait < 0 )
		{
			cmd_wait = 1; // ignore the argument
		}
	}
	else
	{
		cmd_wait = 1;
	}
}

/*
=============================================================================

                                                COMMAND BUFFER

=============================================================================
*/

/*
============
Cbuf_Init
============
*/
void Cbuf_Init( void )
{
	cmd_text.data = cmd_text_buf;
	cmd_text.maxsize = MAX_CMD_BUFFER;
	cmd_text.cursize = 0;
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer, does NOT add a final \n
============
*/
void Cbuf_AddText( const char *text )
{
	int l;

	l = strlen( text );

	if ( cmd_text.cursize + l >= cmd_text.maxsize )
	{
		Com_Printf(_( "Cbuf_AddText: overflow\n" ));
		return;
	}

	Com_Memcpy( &cmd_text.data[ cmd_text.cursize ], text, l );
	cmd_text.cursize += l;
}

/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
============
*/
void Cbuf_InsertText( const char *text )
{
	int len;
	int i;

	len = strlen( text ) + 1;

	if ( len + cmd_text.cursize > cmd_text.maxsize )
	{
		Com_Printf(_( "Cbuf_InsertText overflowed\n" ));
		return;
	}

	// move the existing command text
	for ( i = cmd_text.cursize - 1; i >= 0; i-- )
	{
		cmd_text.data[ i + len ] = cmd_text.data[ i ];
	}

	// copy the new text in
	Com_Memcpy( cmd_text.data, text, len - 1 );

	// add a \n
	cmd_text.data[ len - 1 ] = '\n';

	cmd_text.cursize += len;
}

/*
============
Cbuf_ExecuteText
============
*/
void Cbuf_ExecuteText( int exec_when, const char *text )
{
	switch ( exec_when )
	{
		case EXEC_NOW:
			if ( text && strlen( text ) > 0 )
			{
				Com_DPrintf( S_COLOR_YELLOW "EXEC_NOW %s\n", text );
				Cmd_ExecuteString( text );
			}
			else
			{
				Com_DPrintf( S_COLOR_YELLOW "EXEC_NOW %s\n", cmd_text.data );
				Cbuf_Execute();
			}

			break;

		case EXEC_INSERT:
			Cbuf_InsertText( text );
			break;

		case EXEC_APPEND:
			Cbuf_AddText( text );
			break;

		default:
			Com_Error( ERR_FATAL, "Cbuf_ExecuteText: bad exec_when" );
	}
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute( void )
{
	int  i;
	char *text;
	char line[ MAX_CMD_LINE ];
	int  quotes;

	while ( cmd_text.cursize )
	{
		if ( cmd_wait )
		{
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait--;
			break;
		}

		// find a \n or ; line break
		text = ( char * ) cmd_text.data;

		quotes = 0;

		for ( i = 0; i < cmd_text.cursize; i++ )
		{
			if ( text[ i ] == '\\' && text[ i + 1 ] )
			{
				i++;
				continue; // ignore escapes
			}

			if ( text[ i ] == '"' )
			{
				quotes++;
			}

			if ( !( quotes & 1 ) && text[ i ] == ';' )
			{
				break; // don't break if inside a quoted string
			}

			if ( text[ i ] == '\n' || text[ i ] == '\r' )
			{
				break;
			}
		}

		if ( i >= ( MAX_CMD_LINE - 1 ) )
		{
			i = MAX_CMD_LINE - 1;
		}

		memcpy( line, text, i );
		line[ i ] = 0;

// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec) can insert data at the
// beginning of the text buffer

		if ( i == cmd_text.cursize )
		{
			cmd_text.cursize = 0;
		}
		else
		{
			i++;
			cmd_text.cursize -= i;
			memmove( text, text + i, cmd_text.cursize );
		}

// execute the command line

		Cmd_ExecuteString( line );
	}
}

/*
===============
Cdelay_Frame
===============
*/

void Cdelay_Frame( void ) {
	int i;
	int sys_time = Sys_Milliseconds();
	qboolean run_it;

	for(i=0; (i<MAX_DELAYED_COMMANDS); i++)
	{
		run_it = qfalse;

		if(delayed_cmd[i].delay == CMD_DELAY_UNUSED)
			continue;

		//check if we should run the command (both type)
		if(delayed_cmd[i].type == CMD_DELAY_MSEC && delayed_cmd[i].delay < sys_time)
		{
			run_it = qtrue;
		} else if(delayed_cmd[i].type == CMD_DELAY_FRAME)
		{
			delayed_cmd[i].delay--;
			if(delayed_cmd[i].delay == CMD_DELAY_FRAME_FIRE)
				run_it = qtrue;
		}

		if(run_it)
		{
			delayed_cmd[i].delay = CMD_DELAY_UNUSED;
			Cbuf_ExecuteText(EXEC_NOW, delayed_cmd[i].text);
		}
	}
}
/*
==============================================================================

                                                SCRIPT COMMANDS

==============================================================================
*/

/*
===============
Cmd_Exec_f
===============
*/
static void Cmd_ExecFile( char *f )
{
	int i;

	COM_Compress( f );

	Cvar_Get( "arg_all", Cmd_ArgsFrom( 2 ), CVAR_TEMP | CVAR_ROM | CVAR_USER_CREATED );
	Cvar_Set( "arg_all", Cmd_ArgsFrom( 2 ) );
	Cvar_Get( "arg_count", va( "%i", Cmd_Argc() - 2 ), CVAR_TEMP | CVAR_ROM | CVAR_USER_CREATED );
	Cvar_Set( "arg_count", va( "%i", Cmd_Argc() - 2 ) );

	for ( i = Cmd_Argc() - 2; i; i-- )
	{
		Cvar_Get( va( "arg_%i", i ), Cmd_Argv( i + 1 ), CVAR_TEMP | CVAR_ROM | CVAR_USER_CREATED );
		Cvar_Set( va( "arg_%i", i ), Cmd_Argv( i + 1 ) );
	}

	Cbuf_InsertText( f );
}

void Cmd_Exec_f( void )
{
	union
	{
		char *c;
		void *v;
	} f;

	int          len;
	char         filename[ MAX_QPATH ];
	fileHandle_t h;
	qboolean     success = qfalse;
	qboolean     quiet;

	quiet = !Q_stricmp( Cmd_Argv( 0 ), "execq" );

	if ( Cmd_Argc() < 2 )
	{
		if ( quiet )
		{
			Com_Printf(_("execq <filename> (args) : execute a script file without notification\n"));
		}
		else
		{
			Com_Printf(_("exec <filename> (args) : execute a script file\n"));
		}

		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );

	len = FS_SV_FOpenFileRead( filename, &h );

	if ( h )
	{
		success = qtrue;
		f.v = Hunk_AllocateTempMemory( len + 1 );
		FS_Read( f.v, len, h );
		f.c[ len ] = 0;
		FS_FCloseFile( h );
		Cmd_ExecFile( f.c );
		Hunk_FreeTempMemory( f.v );
	}
	else
	{
		FS_ReadFile( filename, &f.v );

		if ( f.c )
		{
			success = qtrue;
			Cmd_ExecFile( f.c );
			FS_FreeFile( f.v );
		}
	}

	if ( !success )
	{
		Com_Printf( "couldn't exec %s\n", filename );
	}
	else if ( !quiet )
	{
		Com_Printf( "execing %s\n", filename );
	}
}

/*
===============
Cmd_Vstr_f

Inserts the current value of a variable as command text
===============
*/
void Cmd_Vstr_f( void )
{
	char *variableName;

	if ( Cmd_Argc() != 2 )
	{
		Com_Printf(_( "vstr <variablename> : execute a variable command\n" ));
		return;
	}

	variableName = Cvar_VariableString( Cmd_Argv( 1 ) );
	Cbuf_InsertText( va( "%s\n", variableName ) );
}

/*
===============
Helper functions for Cmd_If_f & Cmd_ModCase_f
===============
*/
#ifdef DEDICATED
static const char modifierList[] = N_("not supported in the dedicated server");
#else
static const char modifierList[] = N_("shift, ctrl, alt, command/cmd, mode, super; ! negates; e.g. shift,!alt");

static const struct
{
	char           name[ 8 ];
	unsigned short count;
	unsigned short bit;
	unsigned int   index;
} modifierKeys[] =
{
	{ "shift",   5,        1,  K_SHIFT   },
	{ "ctrl",    4,        2,  K_CTRL    },
	{ "alt",     3,        4,  K_ALT     },
	{ "command", 7,        8,  K_COMMAND },
	{ "cmd",     3,        8,  K_COMMAND },
	{ "mode",    4,        16, K_MODE    },
	{ "super",   5,        32, K_SUPER   },
	{ "",        0,        0,  0         }
};
// Following is no. of bits required for modifiers in the above list
#define NUM_RECOGNISED_MODIFIERS 6

typedef struct
{
	uint16_t down, up;
	int      bits;
} modifierMask_t;

static modifierMask_t getModifierMask( const char *mods )
{
	int                         i;
	modifierMask_t              mask;
	const char                  *ptr;
	static const modifierMask_t none;

	mask = none;

	--mods;

	while ( *++mods == ' ' ) { /* skip leading spaces */; }

	ptr = mods;

	while ( *ptr )
	{
		int invert = ( *ptr == '!' );

		if ( invert )
		{
			++ptr;
		}

		for ( i = 0; modifierKeys[ i ].bit; ++i )
		{
			// is it this modifier?
			if ( !Q_strnicmp( ptr, modifierKeys[ i ].name, modifierKeys[ i ].count )
			     && ( ptr[ modifierKeys[ i ].count ] == ' ' ||
			          ptr[ modifierKeys[ i ].count ] == ',' ||
			          ptr[ modifierKeys[ i ].count ] == 0 ) )
			{
				if ( invert )
				{
					mask.up |= modifierKeys[ i ].bit;
				}
				else
				{
					mask.down |= modifierKeys[ i ].bit;
				}

				if ( ( mask.down & mask.up ) & modifierKeys[ i ].bit )
				{
					Com_Printf(_( "can't have %s both pressed and not pressed\n"), modifierKeys[ i ].name );
					return none;
				}

				// right, parsed a word - skip it, maybe a comma, and any spaces
				ptr += modifierKeys[ i ].count - 1;

				while ( *++ptr == ' ' ) { /**/; }

				if ( *ptr == ',' )
				{
					while ( *++ptr == ' ' ) { /**/; }
				}

				// ready to parse the next one
				break;
			}
		}

		if ( !modifierKeys[ i ].bit )
		{
			Com_Printf(_( "unknown modifier key name in \"%s\"\n"), mods );
			return none;
		}
	}

	for ( i = 0; i < NUM_RECOGNISED_MODIFIERS; ++i )
	{
		if ( mask.up & ( 1 << i ) )
		{
			++mask.bits;
		}

		if ( mask.down & ( 1 << i ) )
		{
			++mask.bits;
		}
	}

	return mask;
}

static int checkKeysDown( modifierMask_t mask )
{
	int i;

	for ( i = 0; modifierKeys[ i ].bit; ++i )
	{
		if ( ( mask.down & modifierKeys[ i ].bit ) && keys[ modifierKeys[ i ].index ].down == 0 )
		{
			return 0; // should be pressed, isn't pressed
		}

		if ( ( mask.up   & modifierKeys[ i ].bit ) && keys[ modifierKeys[ i ].index ].down )
		{
			return 0; // should not be pressed, is pressed
		}
	}

	return 1; // all (not) pressed as requested
}

/*
===============
Cmd_ModCase_f

Takes a sequence of modifier/command pairs
Executes the command for the first matching modifier set

===============
*/
void Cmd_ModCase_f( void )
{
	int  argc = Cmd_Argc();
	int  index = 0;
	int  max = 0;
	int  count = ( argc - 1 ) / 2; // round down :-)
	char *v;

	int  mods[ 1 << NUM_RECOGNISED_MODIFIERS ];
	// want 'modifierMask_t mods[argc / 2 - 1];' (variable array, C99)
	// but MSVC apparently doesn't like that

	if ( argc < 3 )
	{
		Com_Printf(_( "modcase <modifiers> <command> [<modifiers> <command>] … [<command>]\n" ));
		return;
	}

	while ( index < count )
	{
		modifierMask_t mask = getModifierMask( Cmd_Argv( 2 * index + 1 ) );

		if ( mask.bits == 0 )
		{
			return; // parse failure (reported) - abort
		}

		mods[ index ] = checkKeysDown( mask ) ? mask.bits : 0;

		if ( max < mods[ index ] )
		{
			max = mods[ index ];
		}

		++index;
	}

	// If we have a tail command, use it as default
	v = ( argc & 1 ) ? NULL : Cmd_Argv( argc - 1 );

	// Search for a suitable command to execute.
	// Search is done as if the commands are sorted by modifier count
	// (descending) then parameter index no. (ascending).
	for ( ; max > 0; --max )
	{
		int i;

		for ( i = 0; i < index; ++i )
		{
			if ( mods[ i ] == max )
			{
				v = Cmd_Argv( 2 * i + 2 );
				goto found;
			}
		}
	}

found:

	if ( v && *v )
	{
		if ( *v == '/' || *v == '\\' )
		{
			Cbuf_InsertText( va( "%s\n", v + 1 ) );
		}
		else
		{
			Cbuf_InsertText( va( "vstr %s\n", v ) );
		}
	}
}

#endif // !DEDICATED

/*
===============
Cmd_If_f

Compares two numbers, if true executes the third argument, if false executes the forth
===============
*/
void Cmd_If_f( void )
{
	char           *result = NULL;
	int            firstNumber, secondNumber;
	const char     *firstString, *secondString;
	char           *consequent;
	char           *alternative = NULL;
	char           *relation;
#ifndef DEDICATED
	modifierMask_t mask;
#endif
	int            argumentCount;

	switch ( argumentCount = Cmd_Argc() )
	{
		case 4:
			alternative = Cmd_Argv( 3 );
			/* no break */

		case 3:
			consequent = Cmd_Argv( 2 );
#ifdef DEDICATED
			Com_Printf(_( "if <modifiers>… is not supported on the server — assuming true.\n" ));
			result = consequent;
#else
			result = Cmd_Argv( 1 );
			mask = getModifierMask( result );

			if ( mask.bits == 0 )
			{
				return;
			}

			result = checkKeysDown( mask ) ? consequent : alternative;
#endif
			break;

		case 6:
			alternative = Cmd_Argv( 5 );
			/* no break */

		case 5:
			consequent = Cmd_Argv( 4 );
			firstNumber = atoi( firstString = Cmd_Argv( 1 ) );
			relation = Cmd_Argv( 2 );
			secondNumber = atoi( secondString = Cmd_Argv( 3 ) );

			if      ( !strcmp( relation, "="  ) ) { result = ( firstNumber == secondNumber ) ? consequent : alternative; }
			else if ( !strcmp( relation, "!=" ) ) { result = ( firstNumber != secondNumber ) ? consequent : alternative; }
			else if ( !strcmp( relation, "<"  ) ) { result = ( firstNumber <  secondNumber ) ? consequent : alternative; }
			else if ( !strcmp( relation, "<=" ) ) { result = ( firstNumber <= secondNumber ) ? consequent : alternative; }
			else if ( !strcmp( relation, ">"  ) ) { result = ( firstNumber >  secondNumber ) ? consequent : alternative; }
			else if ( !strcmp( relation, ">=" ) ) { result = ( firstNumber >= secondNumber ) ? consequent : alternative; }
			else if ( !strcmp( relation, "eq" ) ) { result = ( Q_stricmp( firstString, secondString ) == 0 ) ? consequent : alternative; }
			else if ( !strcmp( relation, "ne" ) ) { result = ( Q_stricmp( firstString, secondString ) != 0 ) ? consequent : alternative; }
			else if ( !strcmp( relation, "in" ) ) { result = ( Q_stristr( secondString, firstString ) != 0 ) ? consequent : alternative; }
			else if ( !strcmp( relation, "!in") ) { result = ( Q_stristr( secondString, firstString ) == 0 ) ? consequent : alternative; }
			else
			{
				Com_Printf(_( "invalid relation operator in if command. valid relation operators are = != < > >= <= eq ne in !in\n" ));
				return;
			}

			break;

		default:
			Com_Printf(_( "if <number> <relation> <number> <cmdthen> (<cmdelse>) : compares the first two numbers and executes <cmdthen> if true, <cmdelse> if false\n"

			            "if <modifiers> <cmdthen> (<cmdelse>) : check if modifiers are (not) pressed\n"
			            "-- modifiers are %s\n"
			            "-- commands are cvar names unless prefixed with / or \\\n"),
			            _(modifierList) );
			return;
	}

	if ( result && *result )
	{
		if ( *result == '/' || *result == '\\' )
		{
			Cbuf_InsertText( va( "%s\n", result + 1 ) );
		}
		else
		{
			Cbuf_InsertText( va( "vstr %s\n", result ) );
		}
	}
}

/*
===============
Cmd_Math_f

Does math and saves the result to a cvar
===============
*/
void Cmd_Math_f( void )
{
	char *targetVariable;
	char *firstOperand;
	char *secondOperand;
	char *operation;

	if ( Cmd_Argc() == 3 )
	{
		targetVariable = Cmd_Argv( 1 );
		operation = Cmd_Argv( 2 );

		if ( !strcmp( operation, "++" ) )
		{
			Cvar_SetValueLatched( targetVariable, Cvar_VariableValue( targetVariable ) + 1 );
		}
		else if ( !strcmp( operation, "--" ) )
		{
			Cvar_SetValueLatched( targetVariable, Cvar_VariableValue( targetVariable ) - 1 );
		}
		else
		{
			Com_Printf(_( "math <variableToSet> = <number> <operator> <number>\nmath <variableToSet> <operator> <number>\nmath <variableToSet> ++\nmath <variableToSet> --\nvalid operators are + - * /\n" ));
			return;
		}
	}
	else if ( Cmd_Argc() == 4 )
	{
		targetVariable = Cmd_Argv( 1 );
		operation = Cmd_Argv( 2 );
		firstOperand = Cmd_Argv( 3 );

		if ( !strcmp( operation, "+" ) )
		{
			Cvar_SetValueLatched( targetVariable, ( atof( targetVariable ) + atof( firstOperand ) ) );
		}
		else if ( !strcmp( operation, "-" ) )
		{
			Cvar_SetValueLatched( targetVariable, ( atof( targetVariable ) - atof( firstOperand ) ) );
		}
		else if ( !strcmp( operation, "*" ) )
		{
			Cvar_SetValueLatched( targetVariable, ( atof( targetVariable ) * atof( firstOperand ) ) );
		}
		else if ( !strcmp( operation, "/" ) )
		{
			if ( atof( firstOperand ) == 0.f )
			{
				Com_Printf(_( "Cannot divide by 0!\n" ));
				return;
			}

			Cvar_SetValueLatched( targetVariable, ( atof( targetVariable ) / atof( firstOperand ) ) );
		}
		else
		{
			Com_Printf(_( "math <variableToSet> = <number> <operator> <number>\nmath <variableToSet> <operator> <number>\nmath <variableToSet> ++\nmath <variableToSet> --\nvalid operators are + - * /\n" ));
			return;
		}
	}
	else if ( Cmd_Argc() == 6 )
	{
		targetVariable = Cmd_Argv( 1 );
		firstOperand = Cmd_Argv( 3 );
		operation = Cmd_Argv( 4 );
		secondOperand = Cmd_Argv( 5 );

		if ( !strcmp( operation, "+" ) )
		{
			Cvar_SetValueLatched( targetVariable, ( atof( firstOperand ) + atof( secondOperand ) ) );
		}
		else if ( !strcmp( operation, "-" ) )
		{
			Cvar_SetValueLatched( targetVariable, ( atof( firstOperand ) - atof( secondOperand ) ) );
		}
		else if ( !strcmp( operation, "*" ) )
		{
			Cvar_SetValueLatched( targetVariable, ( atof( firstOperand ) * atof( secondOperand ) ) );
		}
		else if ( !strcmp( operation, "/" ) )
		{
			if ( atof( secondOperand ) == 0.f )
			{
				Com_Printf(_( "Cannot divide by 0!\n" ));
				return;
			}

			Cvar_SetValueLatched( targetVariable, ( atof( firstOperand ) / atof( secondOperand ) ) );
		}
		else
		{
			Com_Printf(_( "math <variableToSet> = <number> <operator> <number>\nmath <variableToSet> <operator> <number>\nmath <variableToSet> ++\nmath <variableToSet> --\nvalid operators are + - * /\n" ));
			return;
		}
	}
	else
	{
		Com_Printf(_( "math <variableToSet> = <number> <operator> <number>\nmath <variableToSet> <operator> <number>\nmath <variableToSet> ++\nmath <variableToSet> --\nvalid operators are + - * /\n" ));
		return;
	}
}

/*
===============
Cmd_Strcmp_f

Compares two strings, if true executes the third argument, if false executes the forth
===============
*/
void Cmd_Strcmp_f( void )
{
	char *result;
	char *firstString;
	char *secondString;
	char *consequent;
	char *alternative;
	char *relation;

	if ( ( Cmd_Argc() == 6 ) || ( Cmd_Argc() == 5 ) )
	{
		firstString = Cmd_Argv( 1 );
		relation = Cmd_Argv( 2 );
		secondString = Cmd_Argv( 3 );
		consequent = Cmd_Argv( 4 );

		if ( ( !strcmp( relation, "=" ) && !strcmp( firstString, secondString ) ) ||
		     ( !strcmp( relation, "!=" ) && strcmp( firstString, secondString ) ) )
		{
			result = consequent;
		}
		else if ( ( !strcmp( relation, "=" ) && strcmp( firstString, secondString ) ) ||
		          ( !strcmp( relation, "!=" ) && !strcmp( firstString, secondString ) ) )
		{
			if ( Cmd_Argc() == 6 )
			{
				alternative = Cmd_Argv( 5 );
				result = alternative;
			}
			else
			{
				return;
			}
		}
		else
		{
			Com_Printf(_( "invalid operator in strcmp command. valid operators are = !=\n" ));
			return;
		}
	}
	else
	{
		Com_Printf(_( "strcmp <string1> <relation> <string2> <cmdthen> (<cmdelse>) : compares the first two strings and executes <cmdthen> if true, <cmdelse> if false\n" ));
		return;
	}

	Cbuf_InsertText( va( "%s\n", result ) );
}

/*
===============
Cmd_Concat_f

concatenates cvars together
===============
*/
void Cmd_Concat_f( void )
{
	int  i;
	char variableToSet[ MAX_CVAR_VALUE_STRING ] = "";

	if ( Cmd_Argc() < 3 )
	{
		Com_Printf(_( "concat <variableToSet> <variable1> … <variableN> : concatenates variable1 to variableN and sets the result to variableToSet\n" ));
		return;
	}

	for ( i = 2; i < Cmd_Argc(); i++ )
	{
		Q_strcat( variableToSet, sizeof( variableToSet ), Cvar_VariableString( Cmd_Argv( i ) ) );
	}

	Cvar_Set( Cmd_Argv( 1 ), variableToSet );
}

/*
===============
Cmd_Calc_f

Does math and displays the value into the chat/console, this is used for basic math operations
===============
*/
void Cmd_Calc_f( void )
{
	char *firstOperand;
	char *secondOperand;
	char *operation;

	if ( Cmd_Argc() < 3 )
	{
		Com_Printf(_( "calc <number> <operator> <number>, accepted operators: +, -, /, */x\n" ));
		return;
	}

	firstOperand = Cmd_Argv( 1 );
	operation = Cmd_Argv( 2 );
	secondOperand = Cmd_Argv( 3 );

	// Add
	if ( !strcmp( operation, "+" ) )
	{
		Com_Printf( "%s %s %s = %f\n", firstOperand, operation, secondOperand, ( atof( firstOperand ) + atof( secondOperand ) ) );
		return;
	}

	// Subtract
	else if ( !strcmp( operation, "-" ) )
	{
		Com_Printf( "%s %s %s = %f\n", firstOperand, operation, secondOperand, ( atof( firstOperand ) - atof( secondOperand ) ) );
		return;
	}

	// Divide
	else if ( !strcmp( operation, "/" ) )
	{
		if ( atof( secondOperand ) == 0.f )
		{
			Com_Printf(_( "Cannot divide by 0!\n" ));
			return;
		}

		Com_Printf( "%s ÷ %s = %f\n", firstOperand, secondOperand, ( atof( firstOperand ) / atof( secondOperand ) ) );
		return;
	}

	// Multiply
	else if ( !strcmp( operation, "*" ) || !strcmp( operation, "x" ) )
	{
		//note: ⨉ (n-times operator) is not x (the letter) and might have different rendering with different fonts
		Com_Printf( "%s ⨉  %s = %f\n", firstOperand, secondOperand, ( atof( firstOperand ) * atof( secondOperand ) ) );
		return;
	}

	// Invalid function, help the poor guy out
	Com_Printf(_( "calc <number> <operator> <number>, accepted operators: +, -, /, */x\n" ));
}

/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void Cmd_Echo_f( void )
{
	Com_Printf( "%s\n", Cmd_Args() );
}

/*
===============
Cmd_Undelay_f

Removes a pending delay with a given name
===============
*/
void Cmd_Undelay_f( void )
{
	int  i;
	char *find, *limit;

	// Check if the call is valid
	if ( Cmd_Argc() < 1 )
	{
		Com_Printf(_( "undelay <name> (command)\nremoves all commands with <name> in them.\nif (command) is specified, the removal will be limited only to delays whose commands contain (command).\n" ));
		return;
	}

	find = Cmd_Argv( 1 );
	limit = Cmd_Argv( 2 );

	for ( i = 0; ( i < MAX_DELAYED_COMMANDS ); i++ )
	{
		if ( delayed_cmd[ i ].delay != CMD_DELAY_UNUSED && strstr( delayed_cmd[ i ].name, find ) && strstr( delayed_cmd[ i ].text, limit ) )   // the limit test will always pass if limit is a null string
		{
			delayed_cmd[ i ].delay = CMD_DELAY_UNUSED;
		}
	}
}

/*
===============
Cmd_UndelayAll_f

Removes all pending delays
===============
*/
void Cmd_UndelayAll_f( void )
{
	int i;

	for ( i = 0; ( i < MAX_DELAYED_COMMANDS ); i++ )
	{
		delayed_cmd[ i ].delay = CMD_DELAY_UNUSED;
	}
}

/*
===============
Cmd_Delay_f

Delays a command
===============
*/
void Cmd_Delay_f( void )
{
	int      i, delay, type, lastchar;
	char     *raw_delay, *name, *cmd;
	qboolean availiable_cmd = qfalse;

	// Check if the call is valid
	if ( Cmd_Argc() < 3 || Cmd_Argc() > 4)
	{
		Com_Printf(_( "delay (name) <delay in milliseconds> <command>\ndelay <delay in frames>f <command>\nexecutes <command> after the delay\n" ));
		return;
	}

	raw_delay = Cmd_Argv( 1 );

	if ( !isdigit( raw_delay[ 0 ] ) )
	{
		name = raw_delay;
		raw_delay = Cmd_Argv( 2 );
		cmd = Cmd_ArgsFrom( 3 );
	}
	else
	{
		name = "";
		cmd = Cmd_ArgsFrom( 2 );
	}

	delay = atoi( raw_delay );

	if ( delay < 1 )
	{
		Com_Printf(_( "delay: the delay must be a positive integer\n" ));
		return;
	}

	//search for an unused slot
	for ( i = 0; ( i < MAX_DELAYED_COMMANDS ); i++ )
	{
		if ( delayed_cmd[ i ].delay == CMD_DELAY_UNUSED )
		{
			availiable_cmd = qtrue;
			break;
		}
	}

	if ( !availiable_cmd )
	{
		Com_Printf(_( "WARNING: Maximum amount of delayed commands reached\n" ));
		return;
	}

	lastchar = strlen( raw_delay ) - 1;

	if ( raw_delay[ lastchar ] == 'f' )
	{
		delay += CMD_DELAY_FRAME_FIRE;
		type = CMD_DELAY_FRAME;
	}
	else
	{
		type = CMD_DELAY_MSEC;
		delay += Sys_Milliseconds();
	}

	delayed_cmd[ i ].delay = delay;
	delayed_cmd[ i ].type = type;
	Q_strncpyz( delayed_cmd[ i ].text, cmd, MAX_CMD_LINE );
	Q_strncpyz( delayed_cmd[ i ].name, name, MAX_CMD_LINE );
}

/*
===============
Cmd_Random_f

Print a random integer between minNumber and maxNumber
===============
*/
void Cmd_Random_f( void )
{
	int minNumber;
	int maxNumber;

	if ( Cmd_Argc() == 4 )
	{
		minNumber = atoi( Cmd_Argv( 2 ) );
		maxNumber = atoi( Cmd_Argv( 3 ) );
		Cvar_SetValueLatched( Cmd_Argv( 1 ), ( int )( minNumber + rand() / ( double ) RAND_MAX * ( maxNumber - ( double ) minNumber ) ) );
	}
	else
	{
		Com_Printf(_( "random <variableToSet> <minNumber> <maxNumber>\n" ));
	}
}

/*
=============================================================================

                                        ALIASES

=============================================================================
*/

typedef struct cmd_alias_s
{
	struct cmd_alias_s *next;

	char               *name;
	char               *exec;
} cmd_alias_t;

static cmd_alias_t *cmd_aliases = NULL;

/*
============
Cmd_RunAlias_f
============
*/
void Cmd_RunAlias_f( void )
{
	cmd_alias_t *alias;
	char        *name = Cmd_Argv( 0 );
	char        *args = Cmd_ArgsFrom( 1 );

	// Find existing alias
	for ( alias = cmd_aliases; alias; alias = alias->next )
	{
		if ( !Q_stricmp( name, alias->name ) )
		{
			break;
		}
	}

	if ( !alias )
	{
		Com_Error( ERR_FATAL, "Alias: Alias %s doesn't exist", name );
	}

	Cbuf_InsertText( va( "%s %s", alias->exec, args ) );
}

/*
============
Cmd_WriteAliases
============
*/
void Cmd_WriteAliases( fileHandle_t f )
{
	char        buffer[ 1024 ] = "clearaliases\n";
	cmd_alias_t *alias = cmd_aliases;
	FS_Write( buffer, strlen( buffer ), f );

	while ( alias )
	{
		Com_sprintf( buffer, sizeof( buffer ), "alias %s %s\n", alias->name, Cmd_QuoteString( alias->exec ) );
		FS_Write( buffer, strlen( buffer ), f );
		alias = alias->next;
	}
}

/*
============
Cmd_AliasList_f
============
*/
void Cmd_AliasList_f( void )
{
	cmd_alias_t *alias;
	int         i;
	char        *match;

	if ( Cmd_Argc() > 1 )
	{
		match = Cmd_Argv( 1 );
	}
	else
	{
		match = NULL;
	}

	i = 0;

	for ( alias = cmd_aliases; alias; alias = alias->next )
	{
		if ( match && !Com_Filter( match, alias->name, qfalse ) )
		{
			continue;
		}

		Com_Printf( "%s ⇒ %s\n", alias->name, alias->exec );
		i++;
	}

	Com_Printf("%i aliases\n", i );
}

/*
============
Cmd_ClearAliases_f
============
*/
void Cmd_ClearAliases_f( void )
{
	cmd_alias_t *alias = cmd_aliases;
	cmd_alias_t *next;

	while ( alias )
	{
		next = alias->next;
		Cmd_RemoveCommand( alias->name );
		Z_Free( alias->name );
		Z_Free( alias->exec );
		Z_Free( alias );
		alias = next;
	}

	cmd_aliases = NULL;

	// update autogen.cfg
	cvar_modifiedFlags |= CVAR_ARCHIVE;
}

/*
============
Cmd_UnAlias_f
============
*/
void Cmd_UnAlias_f( void )
{
	cmd_alias_t *alias, **back;
	const char  *name;

	// Get args
	if ( Cmd_Argc() < 2 )
	{
		Com_Printf(_( "unalias <name> : delete an alias\n" ));
		return;
	}

	name = Cmd_Argv( 1 );

	back = &cmd_aliases;

	while ( 1 )
	{
		alias = *back;

		if ( !alias )
		{
			return;
		}

		if ( !Q_stricmp( name, alias->name ) )
		{
			*back = alias->next;
			Z_Free( alias->name );
			Z_Free( alias->exec );
			Z_Free( alias );
			Cmd_RemoveCommand( name );

			// update autogen.cfg
			cvar_modifiedFlags |= CVAR_ARCHIVE;
			return;
		}

		back = &alias->next;
	}
}

/*
============
Cmd_Alias_f
============
*/
void Cmd_Alias_f( void )
{
	cmd_alias_t *alias;
	const char  *name;

	// Get args
	if ( Cmd_Argc() < 2 )
	{
		Com_Printf(_( "alias <name> : show an alias\n" ));
		Com_Printf(_( "alias <name> <exec> : create an alias\n" ));
		return;
	}

	name = Cmd_Argv( 1 );

	// Find existing alias
	for ( alias = cmd_aliases; alias; alias = alias->next )
	{
		if ( !Q_stricmp( name, alias->name ) )
		{
			break;
		}
	}

	// Modify/create an alias
	if ( Cmd_Argc() > 2 )
	{
		cmd_function_t *cmd;

		// Crude protection from infinite loops
		if ( !Q_stricmp( Cmd_Argv( 2 ), name ) )
		{
			Com_Printf(_( "Can't make an alias to itself\n" ));
			return;
		}

		// Don't allow overriding builtin commands
		cmd = Cmd_FindCommand( name );

		if ( cmd && cmd->function != Cmd_RunAlias_f )
		{
			Com_Printf(_( "Can't override a builtin function with an alias\n" ));
			return;
		}

		// Create/update an alias
		if ( !alias )
		{
			alias = S_Malloc( sizeof( cmd_alias_t ) );
			alias->name = CopyString( name );
			alias->exec = CopyString( Cmd_ArgsFrom( 2 ) );
			alias->next = cmd_aliases;
			cmd_aliases = alias;
			Cmd_AddCommand( name, Cmd_RunAlias_f );
		}
		else
		{
			// Reallocate the exec string
			Z_Free( alias->exec );
			alias->exec = CopyString( Cmd_ArgsFrom( 2 ) );
		}
	}

	// Show the alias
	if ( !alias )
	{
		Com_Printf(_( "Alias %s does not exist\n"), name );
	}
	else if ( Cmd_Argc() == 2 )
	{
		Com_Printf( "%s ⇒ %s\n", alias->name, alias->exec );
	}

	// update autogen.cfg
	cvar_modifiedFlags |= CVAR_ARCHIVE;
}

/*
============
Cmd_AliasCompletion
============
*/
void    Cmd_AliasCompletion( void ( *callback )( const char *s ) )
{
	cmd_alias_t *alias;

	for ( alias = cmd_aliases; alias; alias = alias->next )
	{
		callback( alias->name );
	}
}

/*
============
Cmd_DelayCompletion
============
*/
void    Cmd_DelayCompletion( void ( *callback )( const char *s ) )
{
	int i;

	for ( i = 0; i < MAX_DELAYED_COMMANDS; i++ )
	{
		if ( delayed_cmd[ i ].delay != CMD_DELAY_UNUSED )
		{
			callback( delayed_cmd[ i ].name );
		}
	}
}

/*
=============================================================================

                                        COMMAND EXECUTION

=============================================================================
*/

/*
============
Cmd_SaveCmdContext
============
*/
void Cmd_SaveCmdContext( void )
{
	Com_Memcpy( &savedCmd, &cmd, sizeof( cmdContext_t ) );
}

/*
============
Cmd_RestoreCmdContext
============
*/
void Cmd_RestoreCmdContext( void )
{
	Com_Memcpy( &cmd, &savedCmd, sizeof( cmdContext_t ) );
}

/*
============
Cmd_Argc
============
*/
int Cmd_Argc( void )
{
	return cmd.argc;
}

/*
============
Cmd_Argv
============
*/
char           *Cmd_Argv( int arg )
{
	if ( ( unsigned ) arg >= cmd.argc )
	{
		return "";
	}

	return cmd.argv[ arg ];
}

/*
============
Cmd_ArgvBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void Cmd_ArgvBuffer( int arg, char *buffer, int bufferLength )
{
	Q_strncpyz( buffer, Cmd_Argv( arg ), bufferLength );
}

/*
============
Cmd_Args

Returns a single string containing argv(arg) to argv(argc()-1)
============
*/
char           *Cmd_ArgsFrom( int arg )
{
	static char cmd_args[ BIG_INFO_STRING ];
	int         i;

	cmd_args[ 0 ] = 0;

	if ( arg < 0 )
	{
		arg = 0;
	}

	for ( i = arg; i < cmd.argc; i++ )
	{
		strcat( cmd_args, Cmd_QuoteString( cmd.argv[ i ] ) );

		if ( i != cmd.argc - 1 )
		{
			strcat( cmd_args, " " );
		}
	}

	return cmd_args;
}

/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char           *Cmd_Args( void )
{
	return Cmd_ArgsFrom( 1 );
}

/*
============
Cmd_ArgsBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void Cmd_ArgsBuffer( char *buffer, int bufferLength )
{
	Q_strncpyz( buffer, Cmd_Args(), bufferLength );
}

/*
============
Cmd_LiteralArgsBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void    Cmd_LiteralArgsBuffer( char *buffer, int bufferLength )
{
	Q_strncpyz( buffer, cmd.cmd, bufferLength );
}

/*
============
Cmd_Cmd

Retrieve the unmodified command string
For rcon use when you want to transmit without altering quoting
ATVI Wolfenstein Misc #284
============
*/
const char *Cmd_Cmd( void )
{
	return cmd.cmd;
}

/*
============
Cmd_Cmd

Retrieve the unmodified command string
For rcon use when you want to transmit without altering quoting
ATVI Wolfenstein Misc #284
============
*/
const char *Cmd_Cmd_FromNth( int count )
{
	char *ret = cmd.cmd - 1;
	int  i = 0, q = 0;

	while ( count && *++ret )
	{
		if ( !q && *ret == ' ' )
		{
			i = 1; // space found outside quotation marks
		}

		if ( i && *ret != ' ' )
		{
			i = 0; // non-space found after space outside quotation marks
			--count; // one word fewer to scan
		}

		if ( *ret == '"' )
		{
			q = !q; // found a quotation mark
		}
		else if ( *ret == '\\' )
		{
			if ( !*++ret)
			{
				break;
			}
		}
	}

	return ret;
}

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
The text is copied to a separate buffer and 0 characters
are inserted in the appropriate place. The argv array
will point into this temporary buffer.
============
*/
// NOTE TTimo define that to track tokenization issues
//#define TKN_DBG
static void Tokenise( const char *text, char *textOut, qboolean tokens, qboolean ignoreQuotes )
{
	// Assumption:
	// if ( tokens ) cmd.argc == 0 && textOut == cmd.tokenized
	// textOut points to a large buffer
	// text which is processed here is already of limited length...
	char *textOutStart = textOut;

	*textOut = '\0'; // initial NUL-termination in case of early exit

	while ( 1 )
	{
		if ( tokens && cmd.argc == MAX_STRING_TOKENS )
		{
			goto done; // this is usually something malicious
		}

		while ( 1 )
		{
			// skip whitespace
			while ( *text > '\0' && *text <= ' ' )
			{
				text++;
			}

			if ( !*text )
			{
				goto done; // all tokens parsed
			}

			// skip // comments
			if ( text[ 0 ] == '/' && text[ 1 ] == '/' )
			{
				goto done; // all tokens parsed
			}

			// skip /* */ comments
			if ( text[ 0 ] == '/' && text[ 1 ] == '*' )
			{
				// two increments, avoiding matching /*/
				++text;
				while ( *++text && !( text[ 0 ] == '*' && text[ 1 ] == '/' ) ) {}

				if ( !*text )
				{
					goto done; // all tokens parsed
				}

				text += 2;
			}
			else
			{
				break; // we are ready to parse a token
			}
		}

		// regular token
		if ( tokens )
		{
			cmd.argv[ cmd.argc ] = textOut;
			cmd.argc++;
		}

		while ( *text < 0 || *text > ' ' )
		{
			if ( ignoreQuotes || text[ 0 ] != '"' )
			{
				// copy until next space, quote or EOT, handling backslashes
				while ( *text < 0 || ( *text > ' ' && *text != '"' ) )
				{
					if ( *text == '\\' && !*++text )
					{
						break;
					}

					*textOut++ = *text++;
				}
			}
			else
			{
				// quoted string :-)
				// copy until next " or EOT, handling backslashes
				// missing trailing " is fine
				++text;

				while ( *text && *text != '"' )
				{
					if ( *text == '\\' && !*++text )
					{
						break;
					}

					*textOut++ = *text++;
				}

				if ( *text ) ++text; // terminating ", if any
			}
		}

		*textOut++ = tokens ? '\0' : ' ';

		if ( !*text )
		{
			goto done; // all tokens parsed
		}
	}

	done:

	if ( textOut > textOutStart )
	{
		// will be NUL-terminated if in token mode
		// otherwise there'll be a space there...
		*--textOut = '\0';
	}
}

static void Cmd_TokenizeString2( const char *text_in, qboolean ignoreQuotes, qboolean parseCvar )
{
	char       *text;
	char       *textOut;
	char       *cvarName;
	char       buffer[ BIG_INFO_STRING ];

#ifdef TKN_DBG
	// FIXME TTimo blunt hook to try to find the tokenization of userinfo
	Com_DPrintf( "Cmd_TokenizeString: %s\n", text_in );
#endif

	// clear previous args
	cmd.argc = 0;
	cmd.cmd[ 0 ] = '\0';

	if ( !text_in )
	{
		return;
	}

	// parse for cvar substitution
	if ( parseCvar )
	{
		Q_strncpyz( buffer, text_in, sizeof( buffer ) );
		text = buffer;
		textOut = cmd.cmd;

		while ( *text )
		{
			if ( text[ 0 ] == '\\' )
			{
				// Escape found. Copy it & following character (if present).
				if ( textOut == sizeof( cmd.cmd ) + cmd.cmd - 1 )
				{
					break;
				}

				*textOut++ = '\\';
				if ( *++text ) goto copy;
				break;
			}
			else if ( text[ 0 ] == '$' )
			{
				// $ found. Could be a variable name.
				cvarName = ++text;
				text = strchr( text, '$' );

				if ( !text )
				{
					// No terminating $ - treat as literal
					text = cvarName - 1;
					goto copy;
				}

				// Got $...$ - look it up
				// If found, quote the value and copy it in
				// If not found, just skip it :-)
				*text++ = 0;

				if ( Cvar_Flags( cvarName ) != CVAR_NONEXISTENT )
				{
					char cvarValue[ MAX_CVAR_VALUE_STRING ];

					Cvar_VariableStringBuffer( cvarName, cvarValue, sizeof( cvarValue ) );
					Q_strncpyz( textOut, Cmd_EscapeString( cvarValue ), sizeof( cmd.cmd ) - ( textOut - cmd.cmd ) );
					textOut += strlen( textOut );

					if ( textOut == sizeof( cmd.cmd ) + cmd.cmd - 1 )
					{
						break;
					}
				}
			}
			else
			{
				// Character literal; just copy
				copy:

				if ( textOut == sizeof( cmd.cmd ) + cmd.cmd - 1 )
				{
					break;
				}

				*textOut++ = *text++;
			}
		}

		*textOut = 0;
	}
	else
	{
		Q_strncpyz( cmd.cmd, text_in, sizeof( cmd.cmd ) );
	}

	text = cmd.cmd - 1;

/*
	// simplify whitespace handling
	while ( *++text )
	{
		if ( *text > '\0' && *text != '\n' && *text < ' ' )
		{
			*text = ' ';
		}
	}
*/

	Tokenise( cmd.cmd, cmd.tokenized, qtrue, ignoreQuotes );
}

/*
============
Cmd_TokenizeString
============
*/
void Cmd_TokenizeString( const char *text_in )
{
	Cmd_TokenizeString2( text_in, qfalse, qfalse );
}

/*
============
Cmd_TokenizeStringIgnoreQuotes
============
*/
void Cmd_TokenizeStringIgnoreQuotes( const char *text_in )
{
	Cmd_TokenizeString2( text_in, qtrue, qfalse );
}

/*
============
Cmd_TokenizeStringParseCvar
============
*/
void Cmd_TokenizeStringParseCvar( const char *text_in )
{
	Cmd_TokenizeString2( text_in, qfalse, qtrue );
}

/*
============
EscapeString

Internal.
Escape all '\' '"' '$', '/' if marking a comment, and ';' if not in quotation marks.
Optionally wrap in ""
============
*/
#define ESCAPEBUFFER_SIZE BIG_INFO_STRING
static char *GetEscapeBuffer( void )
{
	static char escapeBuffer[ 4 ][ ESCAPEBUFFER_SIZE ];
	static int escapeIndex = -1;

	escapeIndex = ( escapeIndex + 1 ) & 3;
	return escapeBuffer[ escapeIndex ];
}

static const char *EscapeString( const char *in, qboolean quote )
{
	char        *escapeBuffer = GetEscapeBuffer();
	char        *out = escapeBuffer;
	const char  *end = escapeBuffer + ESCAPEBUFFER_SIZE - 1 - !!quote;
	qboolean    quoted = qfalse;
	qboolean    forcequote = qfalse;

	if ( quote )
	{
		*out++ = '"';
	}

	while ( *in && out < end )
	{
		char c = *in++;

		if ( forcequote )
		{
			forcequote = qfalse;
			goto doquote;
		}

		switch ( c )
		{
		case '/':
			// only quote "//" and "/*"
			if ( *in != '/' && *in != '*' ) break;
			forcequote = qtrue;
			goto doquote;
		case ';':
			// no need to quote semicolons if in ""
			quoted = qtrue;
			if ( quote ) break;
		case '"':
		case '$':
		case '\\':
			doquote:
			quoted = qtrue;
			*out++ = '\\'; // could set out == end - is fine
			break;
		}

		// keep quotes if we have white space
		if ( c >= '\0' && c <= ' ' ) quoted = qtrue;

		// if out == end, overrun (but within escapeBuffer)
		*out++ = c;
	}

	if ( out > end )
	{
		out -= 2; // an escape overran; allow overwrite
	}

	if ( quote )
	{
		// we have an empty string or something was escaped
		if ( quoted || out == escapeBuffer + 1 )
		{
			*out++ = '"';
		}
		else
		{
			memmove( escapeBuffer, escapeBuffer + 1, --out - escapeBuffer );
		}
	}

	*out = '\0';
	return escapeBuffer;
}

/*
============
Cmd_EscapeString

Adds escapes (see above).
============
*/
const char *Cmd_EscapeString( const char *in )
{
	return EscapeString( in, qfalse );
}

/*
============
Cmd_QuoteString

Adds escapes (see above), wraps in quotation marks.
============
*/
const char *Cmd_QuoteString( const char *in )
{
	return EscapeString( in, qtrue );
}

/*
============
Cmd_QuoteStringBuffer

Cmd_QuoteString for VM usage
============
*/
void Cmd_QuoteStringBuffer( const char *in, char *buffer, int size )
{
	Q_strncpyz( buffer, Cmd_QuoteString( in ), size );
}

/*
============
Cmd_UnescapeString

Unescape a string
============
*/
const char *Cmd_UnescapeString( const char *in )
{
	char        *escapeBuffer = GetEscapeBuffer();
	char        *out = escapeBuffer;

	while ( *in && out < escapeBuffer + ESCAPEBUFFER_SIZE - 1)
	{
		if ( in[0] == '\\' )
		{
			++in;
		}

		*out++ = *in++;
	}

	*out = '\0';
	return escapeBuffer;
}

/*
===================
Cmd_UnquoteString

Usually passed a raw partial command string
String length is UNCHECKED
===================
*/
const char *Cmd_UnquoteString( const char *str )
{
	char *escapeBuffer = GetEscapeBuffer();
	Tokenise( str, escapeBuffer, qfalse, qfalse );
	return escapeBuffer;
}

/*
===================
Cmd_DequoteString -- FIXME QUOTING INFO

Usually passed a raw partial command string
Returns the first token
Escapes are NOT processed
String length is UNCHECKED
===================
*/
const char *Cmd_DequoteString( const char *str )
{
	char *escapeBuffer = GetEscapeBuffer();
	char *q;

	// shouldn't be any leading space, but just in case...
	while ( *str == ' ' || *str == '\n' )
	{
		++str;
	}

	if ( *str == '"' )
	{
		q = strchr( ++str, '"' );
	}
	else
	{
		q = strchr( str, ' ' );
		q = q ? q : strchr( str, '\n' );
	}

	Q_snprintf( escapeBuffer, ESCAPEBUFFER_SIZE, "%.*s", (int)( q ? q - str : ESCAPEBUFFER_SIZE ), str );

	return escapeBuffer;
}

/*
============
Cmd_CommandExists
============
*/
qboolean Cmd_CommandExists( const char *cmd_name )
{
	cmd_function_t *cmd;

	for ( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if ( !strcmp( cmd_name, cmd->name ) )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
============
Cmd_AddCommand
============
*/
void Cmd_AddCommand( const char *cmd_name, xcommand_t function )
{
	cmd_function_t *cmd;

	// fail if the command already exists
	if ( Cmd_CommandExists( cmd_name ) )
	{
		// allow completion-only commands to be silently doubled
		if ( function != NULL )
		{
			Com_Printf(_( "Cmd_AddCommand: %s already defined\n"), cmd_name );
		}

		return;
	}

	// use a small malloc to avoid zone fragmentation
	cmd = S_Malloc( sizeof( cmd_function_t ) );
	cmd->name = CopyString( cmd_name );
	cmd->function = function;
#ifndef DEDICATED
	cmd->isButtonCommand = qfalse;
#endif
	cmd->next = cmd_functions;
	cmd->complete = NULL;
	cmd_functions = cmd;
}

/*
============
Cmd_AddButtonCommand
============
*/
#ifndef DEDICATED
qboolean Cmd_AddButtonCommand( const char *cmd_name, int parameter )
{
	char           *prefixed_name;
	cmd_function_t *cmd;

	prefixed_name = S_Malloc( strlen( cmd_name ) + 2 );
	prefixed_name[0] = '-';
	strcpy( prefixed_name + 1, cmd_name );

	// fail if the command already exists
	if ( Cmd_CommandExists( prefixed_name ) )
	{
		goto fail;
	}

	prefixed_name[0] = '+';

	if ( Cmd_CommandExists( prefixed_name ) )
	{
		goto fail;
	}

	// + form
	// (prefix is already in place)
	cmd = S_Malloc( sizeof( cmd_function_t ) );
	cmd->name = CopyString( prefixed_name );
	cmd->buttonFunction = IN_ButtonDown;
	cmd->isButtonCommand = qtrue;
	cmd->parameter = parameter;
	cmd->next = cmd_functions;
	cmd->complete = NULL;
	cmd_functions = cmd;

	// - form
	prefixed_name[0] = '-';
	cmd = S_Malloc( sizeof( cmd_function_t ) );
	cmd->name = prefixed_name;
	cmd->buttonFunction = IN_ButtonUp;
	cmd->isButtonCommand = qtrue;
	cmd->parameter = parameter;
	cmd->next = cmd_functions;
	cmd->complete = NULL;
	cmd_functions = cmd;

	return qtrue;

fail:
	Z_Free( prefixed_name );
	Com_Printf(_( "Cmd_AddButtonCommand: +/-%s already defined\n"), cmd_name );
	return qfalse;
}
#endif

/*
============
Cmd_SetCommandCompletionFunc
============
*/
void Cmd_SetCommandCompletionFunc( const char *command, completionFunc_t complete )
{
	cmd_function_t *cmd;

	for ( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if ( !Q_stricmp( command, cmd->name ) )
		{
			cmd->complete = complete;
		}
	}
}

/*
============
Cmd_RemoveCommand
============
*/
void Cmd_RemoveCommand( const char *cmd_name )
{
	cmd_function_t *cmd, **back;

	back = &cmd_functions;

	while ( 1 )
	{
		cmd = *back;

		if ( !cmd )
		{
			// command wasn't active
			return;
		}

		if ( !strcmp( cmd_name, cmd->name ) )
		{
			*back = cmd->next;

			if ( cmd->name )
			{
				Z_Free( cmd->name );
			}

			Z_Free( cmd );
			return;
		}

		back = &cmd->next;
	}
}

/*
============
Cmd_CommandCompletion
============
*/
void Cmd_CommandCompletion( void ( *callback )( const char *s ) )
{
	cmd_function_t *cmd;

	for ( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		callback( cmd->name );
	}
}

/*
============
Cmd_CompleteArgument
============
*/
void Cmd_CompleteArgument( const char *command, char *args, int argNum )
{
	cmd_function_t *cmd;

	for ( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if ( !Q_stricmp( command, cmd->name ) && cmd->complete )
		{
			cmd->complete( args, argNum );
		}
	}
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
============
*/
void Cmd_ExecuteString( const char *text )
{
	cmd_function_t *cmdFunc, **prev;

	// execute the command line
	Cmd_TokenizeStringParseCvar( text );

	if ( !Cmd_Argc() )
	{
		return; // no tokens
	}

	// check registered command functions
	for ( prev = &cmd_functions; *prev; prev = &cmdFunc->next )
	{
		cmdFunc = *prev;

		if ( !Q_stricmp( cmd.argv[ 0 ], cmdFunc->name ) )
		{
			// rearrange the links so that the command will be
			// near the head of the list next time it is used
			*prev = cmdFunc->next;
			cmdFunc->next = cmd_functions;
			cmd_functions = cmdFunc;

			// perform the action
#ifndef DEDICATED
			if ( cmdFunc->isButtonCommand )
			{
				cmdFunc->buttonFunction( cmdFunc->parameter );
				return;
			}
			else
#endif
			if ( cmdFunc->function )
			{
				cmdFunc->function();
				return;
			}

			// let the cgame or game handle it
			break;
		}
	}

	// check cvars
	if ( Cvar_Command() )
	{
		return;
	}

	// check client game commands
	if ( com_cl_running && com_cl_running->integer && CL_GameCommand() )
	{
		return;
	}

	// check server game commands
	if ( com_sv_running && com_sv_running->integer && SV_GameCommand() )
	{
		return;
	}

	// check ui commands
	if ( com_cl_running && com_cl_running->integer && UI_GameCommand() )
	{
		return;
	}

	// send it as a server command if we are connected
	CL_ForwardCommandToServer( text );
}

/*
============
Cmd_List_f
============
*/
void Cmd_List_f( void )
{
	cmd_function_t *cmd;
	int            i;
	char           *match;

	if ( Cmd_Argc() > 1 )
	{
		match = Cmd_Argv( 1 );
	}
	else
	{
		match = NULL;
	}

	i = 0;

	for ( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if ( match && !Com_Filter( match, cmd->name, qfalse ) )
		{
			continue;
		}

		Com_Printf( "%s\n", cmd->name );
		i++;
	}

	Com_Printf(_( "%i commands\n"), i );
}

/*
==================
Cmd_CompleteCfgName
==================
*/
void Cmd_CompleteCfgName( char *args, int argNum )
{
	if ( argNum == 2 )
	{
		Field_CompleteFilename( "", "cfg", qfalse );
	}
}

/*
==================
Cmd_CompleteAliasName
==================
*/
void Cmd_CompleteAliasName( char *args, int argNum )
{
	if ( argNum == 2 )
	{
		Field_CompleteAlias();
	}
}

/*
==================
Cmd_CompleteConcat
==================
*/
void Cmd_CompleteConcat( char *args, int argNum )
{
	// Skip
	char *p = Com_SkipTokens( args, argNum - 1, " " );

	if ( p > args )
	{
		Field_CompleteCommand( p, qfalse, qtrue );
	}
}

/*
==================
Cmd_CompleteIf
==================
*/
void Cmd_CompleteIf( char *args, int argNum )
{
	if ( argNum == 5 || argNum == 6 )
	{
		// Skip
		char *p = Com_SkipTokens( args, argNum - 1, " " );

		if ( p > args )
		{
			Field_CompleteCommand( p, qfalse, qtrue );
		}
	}
}

/*
==================
Cmd_CompleteDelay
==================
*/
void Cmd_CompleteDelay( char *args, int argNum )
{
	if ( argNum == 3 || argNum == 4 )
	{
		// Skip "delay "
		char *p = Com_SkipTokens( args, 1, " " );

		if ( p > args )
		{
			Field_CompleteCommand( p, qtrue, qtrue );
		}
	}
}

/*
==================
Cmd_CompleteUnDelay
==================
*/
void Cmd_CompleteUnDelay( char *args, int argNum )
{
	if ( argNum == 2 )
	{
		Field_CompleteDelay();
	}
}

/*
============
Cmd_Init
============
*/
void Cmd_Init( void )
{
	Cmd_AddCommand( "cmdlist", Cmd_List_f );
	Cmd_AddCommand( "exec", Cmd_Exec_f );
	Cmd_SetCommandCompletionFunc( "exec", Cmd_CompleteCfgName );
	Cmd_AddCommand( "execq", Cmd_Exec_f );
	Cmd_SetCommandCompletionFunc( "execq", Cmd_CompleteCfgName );
	Cmd_AddCommand( "vstr", Cmd_Vstr_f );
	Cmd_SetCommandCompletionFunc( "vstr", Cvar_CompleteCvarName );
	Cmd_AddCommand( "echo", Cmd_Echo_f );
	Cmd_AddCommand( "wait", Cmd_Wait_f );
#ifndef DEDICATED
	Cmd_AddCommand( "modcase", Cmd_ModCase_f );
#endif
	Cmd_AddCommand( "if", Cmd_If_f );
	Cmd_SetCommandCompletionFunc( "if", Cmd_CompleteIf );
	Cmd_AddCommand( "calc", Cmd_Calc_f );
	Cmd_AddCommand( "math", Cmd_Math_f );
	Cmd_SetCommandCompletionFunc( "math", Cvar_CompleteCvarName );
	Cmd_AddCommand( "concat", Cmd_Concat_f );
	Cmd_SetCommandCompletionFunc( "concat", Cmd_CompleteConcat );
	Cmd_AddCommand( "strcmp", Cmd_Strcmp_f );
	Cmd_SetCommandCompletionFunc( "strcmp", Cmd_CompleteIf );
	Cmd_AddCommand( "alias", Cmd_Alias_f );
	Cmd_SetCommandCompletionFunc( "alias", Cmd_CompleteAliasName );
	Cmd_AddCommand( "unalias", Cmd_UnAlias_f );
	Cmd_SetCommandCompletionFunc( "unalias", Cmd_CompleteAliasName );
	Cmd_AddCommand( "aliaslist", Cmd_AliasList_f );
	Cmd_AddCommand( "clearaliases", Cmd_ClearAliases_f );
	Cmd_AddCommand( "delay", Cmd_Delay_f );
	Cmd_SetCommandCompletionFunc( "delay", Cmd_CompleteDelay );
	Cmd_AddCommand( "undelay", Cmd_Undelay_f );
	Cmd_SetCommandCompletionFunc( "undelay", Cmd_CompleteUnDelay );
	Cmd_AddCommand( "undelayAll", Cmd_UndelayAll_f );
	Cmd_AddCommand( "random", Cmd_Random_f );
}
