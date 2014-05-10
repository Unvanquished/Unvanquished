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

static int Duk_Rocket_Document_SetTitle( duk_context *ctx )
{
	if ( duk_get_top( ctx ) == 1 )
	{
		const char *title = duk_to_string( ctx, 0 );
		duk_push_this( ctx );
		duk_get_prop_string( ctx, -1, "pointer" );
		void* ptr = duk_get_pointer( ctx, -1 );
		Rocket::Core::ElementDocument *doc = static_cast< Rocket::Core::ElementDocument* >( ptr );
		if ( doc )
		{
			doc->SetTitle( title );
		}

		duk_push_true( ctx );
		return 1;
	}

	duk_push_false( ctx );
	return 1;
}

static int Duk_Rocket_Document_GetTitle( duk_context *ctx )
{
	if ( duk_get_top( ctx ) == 0 )
	{
		duk_push_this( ctx );
		duk_get_prop_string( ctx, -1, "pointer" );
		void* ptr = duk_get_pointer( ctx, -1 );
		Rocket::Core::ElementDocument *doc = static_cast< Rocket::Core::ElementDocument* >( ptr );
		if ( doc )
		{
			duk_pop( ctx );
			duk_push_string( ctx, doc->GetTitle().CString() );
			return 1;
		}
	}

	duk_push_false( ctx );
	return 1;
}

static int Duk_Rocket_Document_Show( duk_context *ctx )
{
	if ( duk_get_top( ctx ) == 0 )
	{
		duk_push_this( ctx );
		duk_get_prop_string( ctx, -1, "pointer" );
		void* ptr = duk_get_pointer( ctx, -1 );
		Rocket::Core::ElementDocument *doc = static_cast< Rocket::Core::ElementDocument* >( ptr );
		if ( doc )
		{
			doc->Show();
			duk_push_true( ctx );
			return 1;
		}
	}

	duk_push_false( ctx );
	return 1;
}

static int Duk_Rocket_Document_Hide( duk_context *ctx )
{
	if ( duk_get_top( ctx ) == 0 )
	{
		duk_push_this( ctx );
		duk_get_prop_string( ctx, -1, "pointer" );
		void* ptr = duk_get_pointer( ctx, -1 );
		Rocket::Core::ElementDocument *doc = static_cast< Rocket::Core::ElementDocument* >( ptr );
		if ( doc )
		{
			doc->Hide();
			duk_push_true( ctx );
			return 1;
		}
	}

	duk_push_false( ctx );
	return 1;
}

static int Duk_Rocket_Document_Close( duk_context *ctx )
{
	if ( duk_get_top( ctx ) == 0 )
	{
		duk_push_this( ctx );
		duk_get_prop_string( ctx, -1, "pointer" );
		void* ptr = duk_get_pointer( ctx, -1 );
		Rocket::Core::ElementDocument *doc = static_cast< Rocket::Core::ElementDocument* >( ptr );
		if ( doc )
		{
			doc->Close();
			duk_push_true( ctx );
			return 1;
		}
	}

	duk_push_false( ctx );
	return 1;
}

void Duk_Create_Document_Object( void *ctx )
{
	const duk_function_list_entry doc_funcs[] = {
		{ "setTitle", Duk_Rocket_Document_SetTitle, 1 },
		{ "getTitle", Duk_Rocket_Document_GetTitle, 0 },
		{ "show", Duk_Rocket_Document_Show, 0 },
		{ "hide", Duk_Rocket_Document_Hide, 0 },
		{ "close", Duk_Rocket_Document_Close, 0 },
		{ NULL, NULL, 0 }
	};

	duk_push_object( ctx );
	duk_put_function_list(ctx, -1, doc_funcs);
}

