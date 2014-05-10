/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

#include <duktape.h>
#include "rocket.h"
#include "client.h"
#include "../framework/CvarSystem.h"
#include "../framework/CommandSystem.h"

duk_context *ctx;

NORETURN static void Duk_Fatal_Handler( duk_context *ctx, int code, const char *msg )
{
	Com_Printf("^1ERROR: ^3Duk:^7 %s", msg );
	throw code;
}

static int Duk_Cvar_Get( duk_context *ctx )
{
	int nargs = duk_get_top( ctx );

	if ( nargs )
	{
		const char *cvar = duk_to_string( ctx, 0 );
		const char *value = Cvar_VariableString( cvar );
		duk_push_string( ctx, value );
		return 1;
	}

	duk_push_false( ctx );
	return 1;
}

static int Duk_Cvar_Set( duk_context *ctx )
{
	int nargs = duk_get_top( ctx );

	if ( nargs >= 2 )
	{
		const char *cvar = duk_to_string( ctx, 0 );
		const char *value = duk_to_string( ctx, 1 );
		Cvar_Set( cvar, value );
		duk_push_true( ctx );
		return 1;
	}

	duk_push_false( ctx );
	return 1;
}

static int Duk_Print( duk_context *ctx )
{
	int nargs = duk_get_top( ctx );

	if ( nargs )
	{
		const char *value = duk_to_string( ctx, 0 );
		Com_Printf( "%s", value );
		return 0;
	}

	duk_push_false( ctx );
	return 1;
}

static int Duk_Cmd_Exec( duk_context *ctx )
{
	int nargs = duk_get_top( ctx );

	if ( nargs )
	{
		const char *value = duk_to_string( ctx, 0 );
		Cmd::BufferCommandText( value );
		return 0;
	}

	duk_push_false( ctx );
	return 1;
}

static int Duk_Document( duk_context *ctx )
{
	int nargs = duk_get_top( ctx );

	if ( nargs == 1 )
	{
		const char *value = duk_to_string( ctx, 0 );
		Rocket::Core::ElementDocument *doc = menuContext->GetDocument( value );

		if ( doc )
		{
			Duk_Create_Document_Object( ctx );
			duk_push_this( ctx );
			duk_push_pointer( ctx, doc );
			duk_put_prop_string( ctx, -3, "pointer" );
			duk_pop( ctx );
			return 1;
		}
	}

	duk_push_false( ctx );
	return 1;
}

void* Duk_CreateBaseHeap( void )
{
	const duk_function_list_entry cvar_funcs[] = {
		{ "set", Duk_Cvar_Set, 2 },
		{ "get", Duk_Cvar_Get, 1 },
		{ NULL, NULL, 0 }
	};

	const duk_function_list_entry cmd_funcs[] = {
		{ "exec", Duk_Cmd_Exec, 2 },
		{ NULL, NULL, 0 }
	};

	duk_context *ctx = duk_create_heap( NULL, NULL, NULL, NULL, Duk_Fatal_Handler );
	duk_push_global_object( ctx );
	duk_push_c_function( ctx, Duk_Print, 1 );
	duk_put_prop_string( ctx, -2, "print" );
	duk_push_object( ctx );
	duk_put_function_list(ctx, -1, cvar_funcs);
	duk_put_prop_string(ctx, -2, "Cvar");
	duk_push_object( ctx );
	duk_put_function_list(ctx, -1, cmd_funcs);
	duk_put_prop_string(ctx, -2, "Cmd");
	duk_push_c_function( ctx, Duk_Document, 1 );
	duk_put_prop_string( ctx, -2, "Document" );
	duk_pop( ctx );

	return ctx;
}

void Duk_Init( void )
{
	ctx = Duk_CreateBaseHeap();
}

void Duk_Shutdown( void )
{
	duk_destroy_heap( ctx );
}

class DukCmd : public Cmd::StaticCmd
{
public:
	DukCmd(): Cmd::StaticCmd( "duk", Cmd::SYSTEM, N_( "execute arbitrary javascript code" ) ) {}

	void Run( const Cmd::Args &args ) const OVERRIDE
	{
		if ( args.Argc() == 2 )
		{
			try
			{
				duk_eval_string( ctx, args.Argv( 1 ).c_str() );
				duk_pop( ctx );
			}
			catch( int i )
			{

			}
		}
	}
};

static DukCmd DukCmdRegistration;


