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

#ifndef ROCKETCVARINLINEELEMENT_H
#define ROCKETCVARINLINEELEMENT_H

#include <RmlUi/Core/Core.h>
#include "../cg_local.h"

class RocketCvarInlineElement : public Rml::Core::Element
{
public:
	RocketCvarInlineElement( const Rocket::Core::String& tag ) : Rml::Core::Element( tag ), cvar( "" ), cvar_value( "" ), type(STRING), dirty_value( false ) {}

	enum CvarType
	{
	    NUMBER,
	    STRING
	};

	virtual void OnAttributeChange( const Rml::Core::ElementAttributes& changed_attributes )
	{
		Rml::Core::Element::OnAttributeChange( changed_attributes );

		if ( changed_attributes.find( "cvar" ) != changed_attributes.end() )
		{
			cvar = GetAttribute< Rml::Core::String >( "cvar",  "" );
			dirty_value = true;
		}

		if ( changed_attributes.find( "type" ) != changed_attributes.end() )
		{
			Rml::Core::String typeString = GetAttribute< Rml::Core::String >( "type", "" );

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
			format = GetAttribute<Rml::Core::String>( "format", "" );
			dirty_value = true;
		}
	}

	virtual void OnUpdate()
	{
		if ( dirty_value || ( !cvar.empty() && cvar_value.c_str() != Cvar::GetValue( cvar.c_str() ) ) )
		{
			Rml::Core::String value = cvar_value = Cvar::GetValue( cvar.c_str() ).c_str();

			if (!format.empty())
			{
				if (type == NUMBER)
				{
					value = va( format.c_str(), atof( Cvar::GetValue( cvar.c_str() ).c_str() ) );
				}
				else
				{
					value = va( format.c_str(), Cvar::GetValue(cvar.c_str()).c_str() );
				}
			}

			SetInnerRML( value );
			dirty_value = false;
		}
	}
private:
	Rml::Core::String cvar;
	Rml::Core::String cvar_value;
	Rml::Core::String format;
	CvarType type;
	bool dirty_value;
};

#endif
