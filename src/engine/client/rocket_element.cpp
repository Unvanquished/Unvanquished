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

#include "rocketElement.h"
#include <Rocket/Core/Core.h>
#include <Rocket/Core/Factory.h>
#include <Rocket/Core/ElementInstancer.h>
#include <Rocket/Core/ElementInstancerGeneric.h>
extern "C"
{
#include "client.h"
}

extern Rocket::Core::Element *activeElement;

// File contains support code for custom rocket elements

void Rocket_GetElementTag( char *tag, int length )
{
	if ( activeElement )
	{
		Q_strncpyz( tag, activeElement->GetTagName().CString(), length );
	}
}

void Rocket_SetElementDimensions( float x, float y )
{
	if ( activeElement )
	{
		static_cast<RocketElement*>(activeElement)->SetDimensions( x, y );
	}
}

void Rocket_RegisterElement( const char *tag )
{
	Rocket::Core::Factory::RegisterElementInstancer( tag, new Rocket::Core::ElementInstancerGeneric< RocketElement >() )->RemoveReference();
}

void Rocket_SetInnerRML( const char *name, const char *id, const char *RML )
{
	if ( ( !*name || !*id ) && activeElement )
	{
		activeElement->SetInnerRML( RML );
	}
	else
	{
		Rocket::Core::ElementDocument *document = context->GetDocument( name );
		if ( document )
		{
			document->GetElementById( id )->SetInnerRML( RML );
		}
	}
}


void Rocket_GetAttribute( const char *name, const char *id, const char *attribute, char *out, int length )
{
	if ( ( !*name || !*id ) && activeElement )
	{
		Q_strncpyz( out, activeElement->GetAttribute< Rocket::Core::String >( attribute, "" ).CString(), length );
	}
	else
	{
		Rocket::Core::ElementDocument *document = context->GetDocument( name );

		if ( document )
		{
			Q_strncpyz( out, document->GetElementById( id )->GetAttribute< Rocket::Core::String >( attribute, "" ).CString(), length );
		}
	}
}

void Rocket_SetAttribute( const char *name, const char *id, const char *attribute, const char *value )
{
	if ( ( !*name || !*id ) && activeElement )
	{
		activeElement->SetAttribute( attribute, value );
	}
	else
	{
		Rocket::Core::ElementDocument *document = context->GetDocument( name );

		if ( document )
		{
			document->GetElementById( id )->SetAttribute( attribute, value );
		}
	}
}
