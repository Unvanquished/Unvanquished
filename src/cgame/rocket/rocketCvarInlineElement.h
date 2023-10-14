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

#include <RmlUi/Core.h>
#include "../cg_local.h"

class RocketCvarInlineElement : public Rml::Element
{
public:
	RocketCvarInlineElement( const Rml::String& tag ) : Rml::Element( tag ), cvar( "" ), cvar_value( "" ), type(STRING), dirty_value( false ) {}

	enum CvarType
	{
	    NUMBER,
	    STRING
	};

	virtual void OnAttributeChange( const Rml::ElementAttributes& changed_attributes )
	{
		Rml::Element::OnAttributeChange( changed_attributes );

		if ( changed_attributes.find( "cvar" ) != changed_attributes.end() )
		{
			cvar = GetAttribute< Rml::String >( "cvar",  "" );
			dirty_value = true;
		}

		if ( changed_attributes.find( "type" ) != changed_attributes.end() )
		{
			Rml::String typeString = GetAttribute< Rml::String >( "type", "" );

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
			format = GetAttribute<Rml::String>( "format", "" );
			dirty_value = true;
		}
	}

	virtual void OnUpdate()
	{
		if ( dirty_value || ( !cvar.empty() && cvar_value.c_str() != Cvar::GetValue( cvar.c_str() ) ) )
		{
			Rml::String value = cvar_value = Cvar::GetValue( cvar.c_str() ).c_str();

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

			if ( type != NUMBER )
			{
				int flags = 0;

				if ( HasAttribute( "quake" ) )
				{
					flags |= RP_QUAKE;
				}

				if ( HasAttribute( "emoticons" ) )
				{
					flags |= RP_EMOTICONS;
				}

				if ( flags )
				{
					value = Rocket_QuakeToRML( value.c_str(), flags );
				}
			}

			SetInnerRML( value );
			dirty_value = false;
		}
	}
private:
	Rml::String cvar;
	Rml::String cvar_value;
	Rml::String format;
	CvarType type;
	bool dirty_value;
};

#endif
