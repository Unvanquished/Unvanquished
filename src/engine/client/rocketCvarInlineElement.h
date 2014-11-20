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

#ifndef ROCKETCVARINLINEELEMENT_H
#define ROCKETCVARINLINEELEMENT_H

#include "../framework/CvarSystem.h"
#include <Rocket/Core/Core.h>

class RocketCvarInlineElement : public Rocket::Core::Element
{
public:
	RocketCvarInlineElement( const Rocket::Core::String& tag ) : Rocket::Core::Element( tag ), cvar( "" ), cvar_value( "" ), dirty_value( false ) {}

	enum CvarType
	{
	    NUMBER,
	    STRING
	};

	virtual void OnAttributeChange( const Rocket::Core::AttributeNameList& changed_attributes )
	{
		Rocket::Core::Element::OnAttributeChange( changed_attributes );

		if ( changed_attributes.find( "cvar" ) != changed_attributes.end() )
		{
			cvar = GetAttribute< Rocket::Core::String >( "cvar",  "" );
			dirty_value = true;
		}

		if ( changed_attributes.find( "type" ) != changed_attributes.end() )
		{
			Rocket::Core::String typeString = GetAttribute< Rocket::Core::String >( "type", "" );

			if ( typeString == "number" )
			{
				type = NUMBER;
			}
			else
			{
				type = STRING;
			}
			dirty_value = true;
		}

		if ( changed_attributes.find( "format" ) != changed_attributes.end() )
		{
			format = GetAttribute<Rocket::Core::String>( "format", "" );
			dirty_value = true;
		}
	}

	virtual void OnUpdate( void )
	{
		if ( dirty_value || ( !cvar.Empty() && cvar_value != Cvar_VariableString( cvar.CString() ) ) )
		{
			Rocket::Core::String value = cvar_value = Cvar_VariableString( cvar.CString() );

			if (!format.Empty())
			{
				if (type == NUMBER)
				{
					value = Rocket::Core::String(cvar_value.Length() + format.Length(), format.CString(), Cvar_VariableValue(cvar.CString()));
				}
				else
				{
					value = Rocket::Core::String(cvar_value.Length() + format.Length(), format.CString(), Cvar_VariableString(cvar.CString()));
				}
			}

			SetInnerRML( value );
			dirty_value = false;
		}
	}
private:
	Rocket::Core::String cvar;
	Rocket::Core::String cvar_value;
	Rocket::Core::String format;
	CvarType type;
	bool dirty_value;
};

#endif
