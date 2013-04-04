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

cvar_t        *cvar_vars;
cvar_t        *cvar_cheats;
int           cvar_modifiedFlags;

#define MAX_CVARS 2048
cvar_t        cvar_indexes[ MAX_CVARS ];
int           cvar_numIndexes;

#define FILE_HASH_SIZE 512
static cvar_t *hashTable[ FILE_HASH_SIZE ];

cvar_t        *Cvar_Set2( const char *var_name, const char *value, qboolean force );

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname )
{
	int  i;
	long hash;
	char letter;

	if ( !fname )
	{
		Com_Error( ERR_DROP, "null name in generateHashValue" );  //gjd
	}

	hash = 0;
	i = 0;

	while ( fname[ i ] != '\0' )
	{
		letter = tolower( fname[ i ] );
		hash += ( long )( letter ) * ( i + 119 );
		i++;
	}

	hash &= ( FILE_HASH_SIZE - 1 );
	return hash;
}

/*
============
Cvar_ValidateString
============
*/
static qboolean Cvar_ValidateString( const char *s )
{
	if ( !s )
	{
		return qfalse;
	}

	if ( strchr( s, '\\' ) )
	{
		return qfalse;
	}

	if ( strchr( s, '\"' ) )
	{
		return qfalse;
	}

	if ( strchr( s, ';' ) )
	{
		return qfalse;
	}

	return qtrue;
}

/*
============
Cvar_FindVar
============
*/
static cvar_t  *Cvar_FindVar( const char *var_name )
{
	cvar_t *var;
	long   hash;

	hash = generateHashValue( var_name );

	for ( var = hashTable[ hash ]; var; var = var->hashNext )
	{
		if ( !Q_stricmp( var_name, var->name ) )
		{
			return var;
		}
	}

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue( const char *var_name )
{
	cvar_t *var;

	var = Cvar_FindVar( var_name );

	if ( !var )
	{
		return 0;
	}

	return var->value;
}

/*
============
Cvar_VariableIntegerValue
============
*/
int Cvar_VariableIntegerValue( const char *var_name )
{
	cvar_t *var;

	var = Cvar_FindVar( var_name );

	if ( !var )
	{
		return 0;
	}

	return var->integer;
}

/*
============
Cvar_VariableString
============
*/
char           *Cvar_VariableString( const char *var_name )
{
	cvar_t *var;

	var = Cvar_FindVar( var_name );

	if ( !var )
	{
		return "";
	}

	return var->string;
}

/*
============
Cvar_VariableStringBuffer
============
*/
void Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize )
{
	cvar_t *var;

	var = Cvar_FindVar( var_name );

	if ( !var )
	{
		*buffer = 0;
	}
	else
	{
		Q_strncpyz( buffer, var->string, bufsize );
	}
}

/*
============
Cvar_VariableStringBuffer
============
*/
void Cvar_LatchedVariableStringBuffer( const char *var_name, char *buffer, int bufsize )
{
	cvar_t *var;

	var = Cvar_FindVar( var_name );

	if ( !var )
	{
		*buffer = 0;
	}
	else
	{
		if ( var->latchedString )
		{
			Q_strncpyz( buffer, var->latchedString, bufsize );
		}
		else
		{
			Q_strncpyz( buffer, var->string, bufsize );
		}
	}
}

/*
============
Cvar_Flags
============
*/
int Cvar_Flags( const char *var_name )
{
	cvar_t *var;

	if ( !( var = Cvar_FindVar( var_name ) ) )
	{
		return CVAR_NONEXISTENT;
	}
	else
	{
		return var->flags;
	}
}

/*
============
Cvar_CommandCompletion
============
*/
void Cvar_CommandCompletion( void ( *callback )( const char *s ) )
{
	cvar_t *cvar;

	for ( cvar = cvar_vars; cvar; cvar = cvar->next )
	{
		callback( cvar->name );
	}
}

/*
============
Cvar_Get

If the variable already exists, the value will not be set unless CVAR_ROM
The flags will be or'ed in if the variable exists.
============
*/
cvar_t         *Cvar_Get( const char *var_name, const char *var_value, int flags )
{
	cvar_t *var;
	long   hash;

	if ( !var_name || !var_value )
	{
		Com_Error( ERR_FATAL, "Cvar_Get: NULL parameter" );
	}

	if ( !Cvar_ValidateString( var_name ) )
	{
		Com_Printf( "invalid cvar name string: %s\n", var_name );
		var_name = "BADNAME";
	}

#if 0 // FIXME: values with backslash happen

	if ( !Cvar_ValidateString( var_value ) )
	{
		Com_Printf( "invalid cvar value string: %s\n", var_value );
		var_value = "BADVALUE";
	}

#endif

	var = Cvar_FindVar( var_name );

	if ( var )
	{
		// if the C code is now specifying a variable that the user already
		// set a value for, take the new value as the reset value
		if ( ( var->flags & CVAR_USER_CREATED ) && !( flags & CVAR_USER_CREATED ) && var_value[ 0 ] )
		{
			var->flags &= ~CVAR_USER_CREATED;
			Z_Free( var->resetString );
			var->resetString = CopyString( var_value );

			if ( flags & CVAR_ROM )
			{
				// this variable was set by the user,
				// so force it to value given by the engine.

				if ( var->latchedString )
				{
					Z_Free( var->latchedString );
				}

				var->latchedString = CopyString( var_value );
			}

			// ZOID--needs to be set so that cvars the game sets as
			// SERVERINFO get sent to clients
			cvar_modifiedFlags |= flags;
		}

		var->flags |= flags;

		// only allow one non-empty reset string without a warning
		if ( !var->resetString[ 0 ] )
		{
			// we don't have a reset string yet
			Z_Free( var->resetString );
			var->resetString = CopyString( var_value );
		}
		else if ( var_value[ 0 ] && strcmp( var->resetString, var_value ) )
		{
			Com_DPrintf( "Warning: cvar \"%s\" given initial values: \"%s\" and \"%s\"\n", var_name, var->resetString, var_value );
		}

		// if we have a latched string, take that value now
		if ( var->latchedString )
		{
			char *s;

			s = var->latchedString;
			var->latchedString = NULL; // otherwise cvar_set2 would free it
			Cvar_Set2( var_name, s, qtrue );
			Z_Free( s );
		}

		// TTimo
		// if CVAR_USERINFO was toggled on for an existing cvar, check whether the value needs to be cleaned from foreign characters
		// (for instance, seta name "name-with-foreign-chars" in the config file, and toggle to CVAR_USERINFO happens later in CL_Init)
		if ( flags & CVAR_USERINFO )
		{
			const char *cleaned = Com_ClearForeignCharacters( var->string );  // NOTE: it is probably harmless to call Cvar_Set2 in all cases, but I don't want to risk it

			if ( strcmp( var->string, cleaned ) )
			{
				Cvar_Set2( var->name, var->string, qfalse );  // call Cvar_Set2 with the value to be cleaned up for verbosity
			}
		}

// use a CVAR_SET for rom sets, get won't override
#if 0

		// CVAR_ROM always overrides
		if ( flags & CVAR_ROM )
		{
			Cvar_Set2( var_name, var_value, qtrue );
		}

#endif
		return var;
	}

	//
	// allocate a new cvar
	//
	if ( cvar_numIndexes >= MAX_CVARS )
	{
		Com_Error( ERR_FATAL, "MAX_CVARS (%d) hit: too many cvars!", MAX_CVARS );
	}

	var = &cvar_indexes[ cvar_numIndexes ];
	cvar_numIndexes++;
	var->name = CopyString( var_name );
	var->string = CopyString( var_value );
	var->modified = qtrue;
	var->modificationCount = 0;
	var->value = atof( var->string );
	var->integer = atoi( var->string );
	var->resetString = CopyString( var_value );

	var->transient = qtrue;

	// link the variable in
	var->next = cvar_vars;
	cvar_vars = var;

	var->flags = flags;

	// note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
	cvar_modifiedFlags |= var->flags;

	hash = generateHashValue( var_name );
	var->hashNext = hashTable[ hash ];
	hashTable[ hash ] = var;

	return var;
}

/*
============
Cvar_Set2
============
*/
#define FOREIGN_MSG "Only printable ASCII characters are allowed in userinfo variables.\n"

cvar_t         *Cvar_Set2( const char *var_name, const char *value, qboolean force )
{
	cvar_t *var;

#if 0

	if ( strcmp( "com_hunkused", var_name ) != 0 )
	{
		Com_DPrintf( "Cvar_Set2: %s %s\n", var_name, value );
	}

#endif

	if ( !Cvar_ValidateString( var_name ) )
	{
		Com_Printf( "invalid cvar name string: %s\n", var_name );
		var_name = "BADNAME";
	}

	var = Cvar_FindVar( var_name );

	if ( !var )
	{
		if ( !value )
		{
			return NULL;
		}

		// create it
		if ( !force )
		{
			var = Cvar_Get( var_name, value, CVAR_USER_CREATED );
		}
		else
		{
			var = Cvar_Get( var_name, value, 0 );
		}
		var->modificationCount++;
		var->transient = qfalse;
		return var;
	}

	var->transient = qfalse;

	if ( !value )
	{
		value = var->resetString;
		var->transient = qtrue;
	}

	if ( var->flags & CVAR_USERINFO )
	{
		const char *cleaned = Com_ClearForeignCharacters( value );

		if ( strcmp( value, cleaned ) )
		{
			Com_Printf( FOREIGN_MSG );
			Com_Printf(_( "Using %s instead of %s\n"), cleaned, value );
			return Cvar_Set2( var_name, cleaned, force );
		}
	}

	if ( !strcmp( value, var->string ) )
	{
		if ( ( var->flags & CVAR_LATCH ) && var->latchedString )
		{
			if ( !strcmp( value, var->latchedString ) )
			{
				return var;
			}
		}
		else
		{
			return var;
		}
	}

	// note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
	cvar_modifiedFlags |= var->flags;

	if ( !force )
	{
		// ydnar: don't set unsafe variables when com_crashed is set
		if ( ( var->flags & CVAR_UNSAFE ) && com_crashed != NULL && com_crashed->integer )
		{
			Com_Printf( _("%s is unsafe. Check com_crashed.\n"), var_name );
			return var;
		}

		if ( var->flags & CVAR_ROM )
		{
			Com_Printf( _("%s is read only.\n"), var_name );
			return var;
		}

		if ( var->flags & CVAR_INIT )
		{
			Com_Printf( _("%s is write protected.\n"), var_name );
			return var;
		}

		if ( ( var->flags & CVAR_CHEAT ) && !cvar_cheats->integer )
		{
			Com_Printf( _("%s is cheat protected.\n"), var_name );
			return var;
		}

		if ( var->flags & CVAR_SHADER )
		{
			Com_Printf( _("%s will be changed upon recompiling shaders.\n"), var_name );
			Cvar_Set( "r_recompileShaders", "1" );
		}

		if ( var->flags & CVAR_LATCH )
		{
			if ( var->latchedString )
			{
				if ( strcmp( value, var->latchedString ) == 0 )
				{
					return var;
				}

				Z_Free( var->latchedString );
			}
			else
			{
				if ( strcmp( value, var->string ) == 0 )
				{
					return var;
				}
			}

			Com_Printf( _("%s will be changed upon restarting.\n"), var_name );
			var->latchedString = CopyString( value );
			var->modified = qtrue;
			var->modificationCount++;
			return var;
		}
	}
	else
	{
		if ( var->latchedString )
		{
			Z_Free( var->latchedString );
			var->latchedString = NULL;
		}
	}

	if ( !strcmp( value, var->string ) )
	{
		return var; // not changed
	}

	var->modified = qtrue;
	var->modificationCount++;

	Z_Free( var->string );  // free the old value string

	var->string = CopyString( value );
	var->value = atof( var->string );
	var->integer = atoi( var->string );

	return var;
}

/*
============
Cvar_Set
============
*/
void Cvar_Set( const char *var_name, const char *value )
{
	Cvar_Set2( var_name, value, qtrue );
}

/*
============
Cvar_SetLatched
============
*/
void Cvar_SetLatched( const char *var_name, const char *value )
{
	Cvar_Set2( var_name, value, qfalse );
}

 /*
 ============
Cvar_SetIFlag

Sets the cvar by the name that begins with a backslash to "1".  This creates a
cvar that can be set by the engine but not by the sure, and can be read by
interpreted modules.
============
*/
void Cvar_SetIFlag( const char *var_name )
{
	cvar_t *var;
	long hash;
	int index;

	if ( !var_name ) {
		Com_Error( ERR_FATAL, "Cvar_SetIFlag: NULL parameter" );
	}

  if ( *var_name != '\\' ) {
		Com_Error( ERR_FATAL, "Cvar_SetIFlag: var_name must begin with a '\\'" );
  }

  /*
  if ( Cvar_FindVar( var_name ) ) {
		Com_Error( ERR_FATAL, "Cvar_SetIFlag: %s already exists.", var_name );
  }
  */

	if ( !Cvar_ValidateString( var_name + 1 ) ) {
		Com_Printf( "invalid cvar name string: %s\n", var_name );
		var_name = "BADNAME";
	}

	// find a free cvar
	for(index = 0; index < MAX_CVARS; index++)
	{
		if(!cvar_indexes[index].name)
			break;
	}

	if(index >= MAX_CVARS)
	{
		if(!com_errorEntered)
			Com_Error(ERR_FATAL, "Error: Too many cvars, cannot create a new one!");

		return;
	}

	var = &cvar_indexes[index];

	if(index >= cvar_numIndexes)
		cvar_numIndexes = index + 1;

	var->name = CopyString (var_name);
	var->string = CopyString ("1");
	var->modified = qtrue;
	var->modificationCount = 1;
	var->value = atof (var->string);
	var->integer = atoi(var->string);
	var->resetString = CopyString( "1" );
	var->validate = qfalse;

	// link the variable in
	var->next = cvar_vars;

	cvar_vars = var;

	var->flags = CVAR_INIT;
	// note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
	cvar_modifiedFlags |= var->flags;

	hash = generateHashValue(var_name);

	var->hashNext = hashTable[hash];

	hashTable[hash] = var;
}


/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue( const char *var_name, float value )
{
	char val[ 32 ];

	if ( value == ( int ) value )
	{
		Com_sprintf( val, sizeof( val ), "%i", ( int ) value );
	}
	else
	{
		Com_sprintf( val, sizeof( val ), "%f", value );
	}

	Cvar_Set( var_name, val );
}

/*
============
Cvar_SetValueLatched
============
*/
void Cvar_SetValueLatched( const char *var_name, float value )
{
	char val[ 32 ];

	if ( value == ( int ) value )
	{
		Com_sprintf( val, sizeof( val ), "%i", ( int ) value );
	}
	else
	{
		Com_sprintf( val, sizeof( val ), "%f", value );
	}

	Cvar_Set2( var_name, val, qfalse );
}

/*
============
Cvar_Reset
============
*/
void Cvar_Reset( const char *var_name )
{
	Cvar_Set2( var_name, NULL, qfalse );
}

/*
============
Cvar_SetCheatState

Any testing variables will be reset to the safe values
============
*/
void Cvar_SetCheatState( void )
{
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
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean Cvar_Command( void )
{
	cvar_t *v;

	// check variables
	v = Cvar_FindVar( Cmd_Argv( 0 ) );

	if ( !v )
	{
		return qfalse;
	}

	// perform a variable print or set
	if ( Cmd_Argc() == 1 )
	{
		Com_Printf( _("\"%s\" is:\"%s" S_COLOR_WHITE "\" default:\"%s" S_COLOR_WHITE "\"\n"), v->name, v->string, v->resetString );

		if ( v->latchedString )
		{
			Com_Printf( _("latched: \"%s\"\n"), v->latchedString );
		}

		return qtrue;
	}

	// set the value if forcing isn't required
	Cvar_Set2( v->name, Cmd_UnquoteString( Cmd_Args() ), qfalse );
	return qtrue;
}

/*
============
Cvar_Toggle_f

Toggles a cvar for easy single key binding,
optionally through a list of given values
============
*/
void Cvar_Toggle_f( void )
{
	int        i, c;
	const char *varname, *curval;

	c = Cmd_Argc();

	if ( c < 2 )
	{
		Com_Printf("%s", _("usage: toggle <variable> [<value> …]\n") );
		return;
	}

	varname = Cmd_Argv( 1 );

	if ( c == 2 )
	{
		Cvar_Set2( varname, va( "%d", !Cvar_VariableValue( varname ) ), qfalse );
		return;
	}

	curval = Cvar_VariableString( Cmd_Argv( 1 ) );

	// don't bother checking the last value for a match, since the desired
	//  behaviour is the same as if the last value didn't match:
	//  set the variable to the first value
	for ( i = 2; i < c - 1; ++i )
	{
		if ( !strcmp( curval, Cmd_Argv( i ) ) )
		{
			Cvar_Set2( varname, Cmd_Argv( i + 1 ), qfalse );
			return;
		}
	}

	// fallback
	Cvar_Set2( varname, Cmd_Argv( 2 ), qfalse );
}

/*
============
Cvar_Cycle_f - ydnar

Cycles a cvar for easy single key binding
============
*/
void Cvar_Cycle_f( void )
{
	int start, end, step, oldvalue, value;

	if ( Cmd_Argc() < 4 || Cmd_Argc() > 5 )
	{
		Com_Printf(_( "usage: cycle <variable> <start> <end> [step]\n" ));
		return;
	}

	oldvalue = value = Cvar_VariableValue( Cmd_Argv( 1 ) );
	start = atoi( Cmd_Argv( 2 ) );
	end = atoi( Cmd_Argv( 3 ) );

	if ( Cmd_Argc() == 5 )
	{
		step = abs( atoi( Cmd_Argv( 4 ) ) );
	}
	else
	{
		step = 1;
	}

	if ( abs( end - start ) < step )
	{
		step = 1;
	}

	if ( end < start )
	{
		value -= step;

		if ( value < end )
		{
			value = start - ( step - ( oldvalue - end + 1 ) );
		}
	}
	else
	{
		value += step;

		if ( value > end )
		{
			value = start + ( step - ( end - oldvalue + 1 ) );
		}
	}

	Cvar_Set2( Cmd_Argv( 1 ), va( "%i", value ), qfalse );
}

/*
============
Cvar_Set_f

Allows setting and defining of arbitrary cvars from console, even if they
weren't declared in C code.
============
*/
void Cvar_Set_f( void )
{
	int  c, unsafe = 0;
	char *value;

	c = Cmd_Argc();

	if ( c < 3 )
	{
		Com_Printf(_( "usage: set <variable> <value> [unsafe]\n" ));
		return;
	}

	// ydnar: handle unsafe vars
	if ( c >= 4 && !strcmp( Cmd_Argv( c - 1 ), "unsafe" ) )
	{
		c--;
		unsafe = 1;

		if ( com_crashed != NULL && com_crashed->integer )
		{
			Com_Printf(_( "%s is unsafe. Check com_crashed.\n"), Cmd_Argv( 1 ) );
			return;
		}
	}

	value = strdup( Cmd_Cmd_FromNth( 2 ) );   // 3rd arg onwards, raw

	if ( unsafe )
	{
		char *end = value + strlen( value );

		// skip spaces
		while ( --end > value )
		{
			if ( *end != ' ' )
			{
				break;
			}
		}

		++end;

		// skip "unsafe" (may be quoted, so just scan it)
		while ( --end > value )
		{
			if ( *end == ' ' )
			{
				break;
			}
		}

		++end;

		// skip spaces
		while ( --end > value )
		{
			if ( *end != ' ' )
			{
				break;
			}
		}

		end[ 1 ] = 0; // end of string :-)
	}

	Cvar_Set2( Cmd_Argv( 1 ), Cmd_UnquoteString( value ), qfalse );
	free( value );
}

static void Cvar_Set_Flagged( int flag )
{
	cvar_t *v;

	if ( Cmd_Argc() < 3 )
	{
		Com_Printf(_( "usage: %s <variable> <value> [unsafe]\n" ), Cmd_Argv( 0 ) );
		return;
	}

	Cvar_Set_f();
	v = Cvar_FindVar( Cmd_Argv( 1 ) );

	if ( !v )
	{
		return;
	}

	v->flags |= flag;
}

/*
============
Cvar_SetU_f

As Cvar_Set, but also flags it as userinfo
============
*/
void Cvar_SetU_f( void )
{
	Cvar_Set_Flagged( CVAR_USERINFO );
}

/*
============
Cvar_SetS_f

As Cvar_Set, but also flags it as serverinfo
============
*/
void Cvar_SetS_f( void )
{
	Cvar_Set_Flagged( CVAR_SERVERINFO );
}

/*
============
Cvar_SetA_f

As Cvar_Set, but also flags it as archived
============
*/
void Cvar_SetA_f( void )
{
	Cvar_Set_Flagged( CVAR_ARCHIVE );
}

/*
============
Cvar_Reset_f
============
*/
void Cvar_Reset_f( void )
{
	if ( Cmd_Argc() != 2 )
	{
		Com_Printf(_( "usage: reset <variable>\n" ));
		return;
	}

	Cvar_Reset( Cmd_Argv( 1 ) );
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
	cvar_t *var;
	char   buffer[ 1024 ];

	for ( var = cvar_vars; var; var = var->next )
	{
		if ( var->flags & CVAR_ARCHIVE )
		{
			if( var->transient )
				continue;

			// write the latched value, even if it hasn't taken effect yet
			Com_sprintf( buffer, sizeof( buffer ), "seta %s %s%s\n",
			             var->name,
			             Cmd_QuoteString( var->latchedString ? var->latchedString : var->string ),
			             ( var->flags & CVAR_UNSAFE ) ? " unsafe" : "" );

			FS_Printf( f, "%s", buffer );
		}
	}
}

/*
============
Cvar_List_f
============
*/
void Cvar_List_f( void )
{
	cvar_t   *var;
	int      i = 0;
	char     *match = NULL;
	qboolean raw = qfalse;

	if ( Cmd_Argc() > 1 )
	{
		match = Cmd_Argv( 1 );

		if ( !Q_stricmp( match, "-raw" ) )
		{
			raw = qtrue;
			match = ( Cmd_Argc() > 2 ) ? Cmd_Argv( 2 ) : NULL;
		}
	}

	for ( var = cvar_vars; var; var = var->next, i++ )
	{
		if ( match && !Com_Filter( match, var->name, qfalse ) )
		{
			continue;
		}

		if ( var->flags & CVAR_SERVERINFO )
		{
			Com_Printf( "S" );
		}
		else
		{
			Com_Printf( " " );
		}

		if ( var->flags & CVAR_SYSTEMINFO )
		{
			Com_Printf( "s" );
		}
		else
		{
			Com_Printf( " " );
		}

		if ( var->flags & CVAR_USERINFO )
		{
			Com_Printf( "U" );
		}
		else
		{
			Com_Printf( " " );
		}

		if ( var->flags & CVAR_ROM )
		{
			Com_Printf( "R" );
		}
		else
		{
			Com_Printf( " " );
		}

		if ( var->flags & CVAR_INIT )
		{
			Com_Printf( "I" );
		}
		else
		{
			Com_Printf( " " );
		}

		if ( var->flags & CVAR_ARCHIVE )
		{
			Com_Printf( "A" );
		}
		else
		{
			Com_Printf( " " );
		}

		if ( var->flags & CVAR_LATCH )
		{
			Com_Printf( "L" );
		}
		else
		{
			Com_Printf( " " );
		}

		if ( var->flags & CVAR_CHEAT )
		{
			Com_Printf( "C" );
		}
		else
		{
			Com_Printf( " " );
		}

		if ( var->flags & CVAR_USER_CREATED )
		{
			Com_Printf( "?" );
		}
		else
		{
			Com_Printf( " " );
		}

		if ( raw )
		{
			char *index;

			Com_Printf( " %s \"", var->name );

			for ( index = var->string; ; )
			{
				char *hat = strchr( index, '^' );

				if ( !hat ) break;

				Com_Printf( "%.*s", (int)( hat + 1 - index ), index );
				index = hat + 1;
			}

			Com_Printf( "%s\"\n", index );
		}
		else
		{
			Com_Printf( " %s \"%s\"\n", var->name, var->string );
		}
	}

	Com_Printf( "\n%i total cvars\n", i );
	Com_Printf( "%i cvar indexes\n", cvar_numIndexes );
}

/*
============
Cvar_Restart_f

Resets all cvars to their hardcoded values
============
*/
void Cvar_Restart_f( void )
{
	cvar_t *var;
	cvar_t **prev;

	prev = &cvar_vars;

	while ( 1 )
	{
		var = *prev;

		if ( !var )
		{
			break;
		}

		// don't mess with rom values, or some inter-module
		// communication will get broken (com_cl_running, etc)
		if ( var->flags & ( CVAR_ROM | CVAR_INIT | CVAR_NORESTART ) )
		{
			prev = &var->next;
			continue;
		}

		// throw out any variables the user created
		if ( var->flags & CVAR_USER_CREATED )
		{
			*prev = var->next;

			if ( var->name )
			{
				Z_Free( var->name );
			}

			if ( var->string )
			{
				Z_Free( var->string );
			}

			if ( var->latchedString )
			{
				Z_Free( var->latchedString );
			}

			if ( var->resetString )
			{
				Z_Free( var->resetString );
			}

			// clear the var completely, since we
			// can't remove the index from the list
			memset( var, 0, sizeof( *var ) );
			continue;
		}

		Cvar_Set( var->name, var->resetString );

		prev = &var->next;
	}
}

/*
=====================
Cvar_InfoString
=====================
*/
char *Cvar_InfoString( int bit, qboolean big )
{
	static char info[ BIG_INFO_STRING ];
	cvar_t      *var;

	info[ 0 ] = 0;

	for ( var = cvar_vars; var; var = var->next )
	{
		if ( var->flags & bit )
		{
			Info_SetValueForKey( info, var->name, var->string, big );
		}
	}

	return info;
}

/*
=====================
Cvar_InfoStringBuffer
=====================
*/
void Cvar_InfoStringBuffer( int bit, char *buff, int buffsize )
{
	Q_strncpyz( buff, Cvar_InfoString( bit, qfalse ), buffsize );
}

/*
=====================
Cvar_CheckRange
=====================
*/
void Cvar_CheckRange( cvar_t *var, float min, float max, qboolean integral )
{
	var->validate = qtrue;
	var->min = min;
	var->max = max;
	var->integral = integral;

	// Force an initial range check
	Cvar_Set( var->name, var->string );
}

/*
=====================
Cvar_Register

basically a slightly modified Cvar_Get for the interpreted modules
=====================
*/
void Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags )
{
	cvar_t *cv;

	cv = Cvar_Get( varName, defaultValue, flags );

	if ( !vmCvar )
	{
		return;
	}

	vmCvar->handle = cv - cvar_indexes;
	vmCvar->modificationCount = -1;
	Cvar_Update( vmCvar );
}

/*
=====================
Cvar_Update

updates an interpreted modules' version of a cvar
=====================
*/
void Cvar_Update( vmCvar_t *vmCvar )
{
	cvar_t *cv = NULL; // bk001129

	assert( vmCvar );  // bk

	if ( ( unsigned ) vmCvar->handle >= cvar_numIndexes )
	{
		Com_Error( ERR_DROP, "Cvar_Update: handle %d out of range", ( unsigned ) vmCvar->handle );
	}

	cv = cvar_indexes + vmCvar->handle;

	if ( cv->modificationCount == vmCvar->modificationCount )
	{
		return;
	}

	if ( !cv->string )
	{
		return; // variable might have been cleared by a cvar_restart
	}

	vmCvar->modificationCount = cv->modificationCount;

	// bk001129 - mismatches.
	if ( strlen( cv->string ) + 1 > MAX_CVAR_VALUE_STRING )
	{
		Com_Error( ERR_DROP, "Cvar_Update: src %s length %lu exceeds MAX_CVAR_VALUE_STRING(%lu)",
		           cv->string, ( unsigned long ) strlen( cv->string ), ( unsigned long ) sizeof( vmCvar->string ) );
	}

	// bk001212 - Q_strncpyz guarantees zero padding and dest[MAX_CVAR_VALUE_STRING-1]==0
	// bk001129 - paranoia. Never trust the destination string.
	// bk001129 - beware, sizeof(char*) is always 4 (for cv->string).
	//            sizeof(vmCvar->string) always MAX_CVAR_VALUE_STRING
	//Q_strncpyz( vmCvar->string, cv->string, sizeof( vmCvar->string ) ); // id
	Q_strncpyz( vmCvar->string, cv->string, MAX_CVAR_VALUE_STRING );

	vmCvar->value = cv->value;
	vmCvar->integer = cv->integer;
}

/*
==================
Cvar_CompleteCvarName
==================
*/
void Cvar_CompleteCvarName( char *args, int argNum )
{
	if ( argNum == 2 )
	{
		// Skip "<cmd> "
		char *p = Com_SkipTokens( args, 1, " " );

		if ( p > args )
		{
			Field_CompleteCommand( p, qfalse, qtrue );
		}
	}
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

	Cmd_AddCommand( "toggle", Cvar_Toggle_f );
	Cmd_SetCommandCompletionFunc( "toggle", Cvar_CompleteCvarName );
	Cmd_AddCommand( "cycle", Cvar_Cycle_f );  // ydnar
	Cmd_SetCommandCompletionFunc( "cycle", Cvar_CompleteCvarName );
	Cmd_AddCommand( "set", Cvar_Set_f );
	Cmd_SetCommandCompletionFunc( "set", Cvar_CompleteCvarName );
	Cmd_AddCommand( "sets", Cvar_SetS_f );
	Cmd_SetCommandCompletionFunc( "sets", Cvar_CompleteCvarName );
	Cmd_AddCommand( "setu", Cvar_SetU_f );
	Cmd_SetCommandCompletionFunc( "setu", Cvar_CompleteCvarName );
	Cmd_AddCommand( "seta", Cvar_SetA_f );
	Cmd_SetCommandCompletionFunc( "seta", Cvar_CompleteCvarName );
	Cmd_AddCommand( "reset", Cvar_Reset_f );
	Cmd_SetCommandCompletionFunc( "reset", Cvar_CompleteCvarName );
	Cmd_AddCommand( "cvarlist", Cvar_List_f );
	Cmd_AddCommand( "cvar_restart", Cvar_Restart_f );
}
