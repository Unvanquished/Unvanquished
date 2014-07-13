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

#include "rocket.h"

static int handle = 0;
static std::unordered_map<int, Rocket::Core::Element*> elementMap;

int Rocket_DynamicHud_CreateElement( const char *tag )
{
	Rocket::Core::Element* element = dynamicHudContext->GetDocument( 0 )->CreateElement( tag );
	elementMap[ handle ] = element;
	return handle++;
}

void Rocket_DynamicHud_RemoveElement( int handle )
{
	Rocket::Core::Element* element = elementMap[ handle ];
	elementMap.erase( handle );
	element->GetOwnerDocument()->RemoveChild( element );
	element->RemoveReference();
}

void Rocket_DynamicHud_SetProperty( int handle, const char *property, const char *value )
{
	if ( elementMap.find( handle) != elementMap.end() )
	{
		elementMap[ handle ]->SetProperty( property, value );
	}
}

void Rocket_DynamicHud_SetAttribute( int handle, const char *attribute, const char *value )
{
	if ( elementMap.find( handle) != elementMap.end() )
	{
		elementMap[ handle ]->SetAttribute( attribute, value );
	}
}

// reduces an rml string to a common format so two rml strings can be compared
static Rocket::Core::String ReduceRML( const Rocket::Core::String &rml )
{
	Rocket::Core::String ret;
	Rocket::Core::String::size_type length = rml.Length();
	ret.Reserve( length );

	for ( int i = 0; i < length; i++ )
	{
		if ( rml[ i ] == ' ' || rml[ i ] == '\n' )
		{
			continue;
		}

		if ( rml[ i ] == '"' )
		{
			ret += '\'';
		}

		else
		{
			ret += rml[ i ];
		}
	}

	return ret;
}

static inline void Rocket_SetInnerRMLGuarded( Rocket::Core::Element *e, const Rocket::Core::String &newRML )
{
	Rocket::Core::String newReducedRML = ReduceRML( newRML );
	Rocket::Core::String oldReducedRML = ReduceRML( e->GetInnerRML() );

	if ( newReducedRML != oldReducedRML )
	{
		e->SetInnerRML( newRML );
	}
}

void Rocket_DynamicHud_SetInnerRML( int handle, const char *RML, int parseFlags )
{
	if ( elementMap.find( handle) != elementMap.end() )
	{
		Rocket::Core::String newRML = parseFlags  ? Rocket_QuakeToRML( RML, parseFlags ) : RML;
		Rocket_SetInnerRMLGuarded( elementMap[ handle ], newRML );
	}
}
