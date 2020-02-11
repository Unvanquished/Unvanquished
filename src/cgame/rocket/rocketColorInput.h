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

#ifndef ROCKETCOLORINPUT_H
#define ROCKETCOLORINPUT_H

#include <RmlUi/Core.h>
#include <RmlUi/Controls/ElementFormControlInput.h>
#include "../cg_local.h"

class RocketColorInput : public Rocket::Core::Element, public Rocket::Core::EventListener
{
public:
	RocketColorInput( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag ), input( nullptr ), color_value( nullptr )
	{
	}

	virtual void OnChildAdd( Element *child )
	{
		Element::OnChildAdd( child );
		if ( child == this )
		{
			// Initialize the input element
			Rocket::Core::XMLAttributes attribs;
			attribs[ "type" ] = "text";
			input = AppendChild( Rocket::Core::Factory::InstanceElement(
					this, "input", "input", attribs ) );
			color_value = AppendChild( Rocket::Core::Factory::InstanceElement(
					this, "*", "div", Rocket::Core::XMLAttributes() ) );
			input->SetProperty( "display", "none" );
			input->SetAttributes( GetAttributes() );
			UpdateValue();
		}
	}

	virtual void OnAttributeChange( const Rocket::Core::ElementAttributes &changed_attributes )
	{
		Rocket::Core::Element::OnAttributeChange( changed_attributes );

		// Pass all attributes down to the input element
		if ( input ) input->SetAttributes( changed_attributes );
	}

	virtual void ProcessEvent( Rocket::Core::Event &event )
	{
		if ( input || color_value ) return;
		if ( event.GetTargetElement() == input )
		{
			if ( event == "change" )
			{
				UpdateValue();
			}

			else if ( event == "blur" )
			{
				input->SetProperty( "display", "none" );
				color_value->SetProperty( "display", "inline" );
				UpdateValue();
			}
		}

		if ( event == "click" )
		{
			Rocket::Core::Element* elem = event.GetTargetElement();

			do
			{
				if ( elem == this )
				{
					input->SetProperty( "display", "inline" );
					color_value->SetProperty( "display", "none" );
					input->Focus();
					break;
				}
			} while ( ( elem = elem->GetParentNode() ) );
		}
	}

private:
	void UpdateValue()
	{
		Rocket::Core::String string = "^7";

		while( color_value && color_value->HasChildNodes() )
		{
			color_value->RemoveChild( color_value->GetFirstChild() );
		}

		string += dynamic_cast< Rocket::Controls::ElementFormControlInput* >( input )->GetValue();

		Rocket::Core::Factory::InstanceElementText( color_value, Rocket_QuakeToRML( string.CString(), RP_QUAKE ) );
	}

	Rocket::Core::Element *input;
	Rocket::Core::Element *color_value;
};

#endif
