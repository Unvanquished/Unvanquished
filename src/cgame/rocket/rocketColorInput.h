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
#include "../cg_local.h"

class RocketColorInput : public Rml::Element, public Rml::EventListener
{
public:
	RocketColorInput( const Rml::String &tag ) : Rml::Element( tag ), input( nullptr ), color_value( nullptr )
	{
	}

	virtual void OnChildAdd( Element *child )
	{
		Element::OnChildAdd( child );
		if ( child == this )
		{
			// Initialize the input element
			Rml::XMLAttributes attribs;
			attribs[ "type" ] = "text";
			input = AppendChild( Rml::Factory::InstanceElement(
					this, "input", "input", attribs ) );
			color_value = AppendChild( Rml::Factory::InstanceElement(
					this, "*", "div", Rml::XMLAttributes() ) );
			input->SetProperty( "display", "none" );
			input->SetAttributes( GetAttributes() );
			input->AddEventListener( Rml::EventId::Blur, this );
			UpdateValue();
		}
	}

	virtual void OnAttributeChange( const Rml::ElementAttributes &changed_attributes )
	{
		Rml::Element::OnAttributeChange( changed_attributes );

		// Pass all attributes down to the input element
		if ( input ) input->SetAttributes( changed_attributes );
	}

	virtual void ProcessDefaultAction( Rml::Event &event )
	{
		Element::ProcessDefaultAction( event );
		ProcessEvent( event );
	}

	virtual void ProcessEvent( Rml::Event &event )
	{
		if ( !input || !color_value ) return;

		if ( event.GetTargetElement() == input )
		{
			if ( event == Rml::EventId::Change )
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

		if ( event == Rml::EventId::Click )
		{
			Rml::Element* elem = event.GetTargetElement();

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
		Rml::String string = "^7";
		string += dynamic_cast< Rml::ElementFormControlInput* >( input )->GetValue();
		while ( color_value && color_value->HasChildNodes() )
		{
			color_value->RemoveChild( color_value->GetFirstChild() );
		}

		Rml::Factory::InstanceElementText( color_value, Rocket_QuakeToRML( string.c_str(), RP_QUAKE ) );
	}

	Rml::Element *input;
	Rml::Element *color_value;
};

#endif
