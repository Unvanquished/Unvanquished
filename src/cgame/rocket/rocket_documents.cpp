/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

// Code to deal with libRocket's loading and handling of documents and elements

#include "rocket.h"
#include "../cg_local.h"

void Rocket_LoadDocument( const char *path )
{
	Log::Debug( "Loading '%s' as RML document", path );
	Rml::ElementDocument* document = menuContext->LoadDocument( path );
	Rml::ElementDocument* other;

	if( document )
	{
		Rocket_SetDocumentScale( *document );
		menuContext->PullDocumentToFront( document ); // Ensure any duplicates will be found first.

		// Close any other documents which may have the same ID
		other = menuContext->GetDocument( document->GetId() );
		if ( other && other != document )
		{
			other->Close();
		}
	}
}

// Scale the UI proportional to the screen size
void Rocket_SetDocumentScale( Rml::ElementDocument& document )
{
	// This makes 1dp one pixel on a 1366x768 screen
	float size = std::min( cgs.glconfig.vidWidth, cgs.glconfig.vidHeight );
	float ratio = size / 768.0f;
	document.GetContext()->SetDensityIndependentPixelRatio(ratio);
}

void Rocket_DocumentAction( const char *name, const char *action )
{
	if ( !Q_stricmp( action, "show" ) || !Q_stricmp( action, "open" ) )
	{
		Rml::ElementDocument* document = menuContext->GetDocument( name );
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

		Rml::ElementDocument* document = menuContext->GetDocument( name );
		if ( document )
		{
			document->Close();
		}
	}
	else if ( !Q_stricmp( "goto", action ) )
	{
		Rml::ElementDocument* document = menuContext->GetDocument( name );
		if ( document )
		{
			Rml::ElementDocument *owner = menuContext->GetFocusElement()->GetOwnerDocument();
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
		Rml::ElementDocument* document = nullptr;

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
		Rml::ElementDocument* document = nullptr;

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
			Rml::String url = document->GetSourceURL();
			document->Close();
			Rml::Factory::ClearStyleSheetCache();
			Rml::Factory::ClearTemplateCache();
			document = menuContext->LoadDocument( url );
			Rocket_SetDocumentScale( *document );
			document->Show();
		}
	}
}
