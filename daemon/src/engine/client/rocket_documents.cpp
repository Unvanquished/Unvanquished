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

// Code to deal with libRocket's loading and handling of documents and elements

#include "rocket.h"
#include "client.h"

void Rocket_LoadDocument( const char *path )
{
	Rocket::Core::ElementDocument* document = menuContext->LoadDocument( path );
	Rocket::Core::ElementDocument* other;

	if( document )
	{
		document->RemoveReference();
		menuContext->PullDocumentToFront( document ); // Ensure any duplicates will be found first.

		// Close any other documents which may have the same ID
		other = menuContext->GetDocument( document->GetId() );
		if ( other && other != document )
		{
			other->Close();
		}
	}

}

void Rocket_LoadCursor( const char *path )
{
	Rocket::Core::ElementDocument* document = menuContext->LoadMouseCursor( path );
	if( document )
	{
		document->RemoveReference();
	}
}

void Rocket_DocumentAction( const char *name, const char *action )
{
	if ( !Q_stricmp( action, "show" ) || !Q_stricmp( action, "open" ) )
	{
		Rocket::Core::ElementDocument* document = menuContext->GetDocument( name );
		if ( document )
		{
			document->Show();
		}
	}
	else if ( !Q_stricmp( "close", action ) )
	{
		if ( !*name ) // If name is empty, hide active
		{
			if ( menuContext->GetFocusElement() &&
				menuContext->GetFocusElement()->GetOwnerDocument() )
			{
				menuContext->GetFocusElement()->GetOwnerDocument()->Close();
			}

			return;
		}

		Rocket::Core::ElementDocument* document = menuContext->GetDocument( name );
		if ( document )
		{
			document->Close();
		}
	}
	else if ( !Q_stricmp( "goto", action ) )
	{
		Rocket::Core::ElementDocument* document = menuContext->GetDocument( name );
		if ( document )
		{
			Rocket::Core::ElementDocument *owner = menuContext->GetFocusElement()->GetOwnerDocument();
			if ( owner )
			{
				owner->Close();
			}
			document->Show();
		}
	}
	else if ( !Q_stricmp( "load", action ) )
	{
		Rocket_LoadDocument( name );
	}
	else if ( !Q_stricmp( "blur", action ) || !Q_stricmp( "hide", action ) )
	{
		Rocket::Core::ElementDocument* document = nullptr;

		if ( !*name ) // If name is empty, hide active
		{
			if ( menuContext->GetFocusElement() &&
				menuContext->GetFocusElement()->GetOwnerDocument() )
			{
				document = menuContext->GetFocusElement()->GetOwnerDocument();
			}
		}
		else
		{
			document = menuContext->GetDocument( name );
		}

		if ( document )
		{
			document->Hide();
		}
	}
	else if ( !Q_stricmp ( "blurall", action ) )
	{
		for ( int i = 0; i < menuContext->GetNumDocuments(); ++i )
		{
			menuContext->GetDocument( i )->Hide();
		}
	}
	else if ( !Q_stricmp( "reload", action ) )
	{
		Rocket::Core::ElementDocument* document = nullptr;

		if ( !*name ) // If name is empty, hide active
		{
			if ( menuContext->GetFocusElement() &&
				menuContext->GetFocusElement()->GetOwnerDocument() )
			{
				document = menuContext->GetFocusElement()->GetOwnerDocument();
			}
		}
		else
		{
			document = menuContext->GetDocument( name );
		}

		if ( document )
		{
			Rocket::Core::String url = document->GetSourceURL();
			document->Close();
			document = menuContext->LoadDocument( url );
			document->Show();
		}
	}
}
