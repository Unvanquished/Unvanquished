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

#ifndef ROCKETCOLORINPUT_H
#define ROCKETCOLORINPUT_H

#include <Rocket/Core.h>
#include <Rocket/Controls/ElementFormControlInput.h>
#include "client.h"

class RocketColorInput : public Rocket::Core::Element, public Rocket::Core::EventListener
{
public:
	RocketColorInput( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag )
	{
		// Initialize the input element
		Rocket::Core::XMLAttributes attribs;
		attribs.Set( "type", "text" );
		input = Rocket::Core::Factory::InstanceElement( this, "input", "input", attribs );
		color_value = Rocket::Core::Factory::InstanceElement( this, "*", "div", Rocket::Core::XMLAttributes() );
	}

	virtual void OnChildAdd( Element *child )
	{
		if ( child == this )
		{
			AppendChild(input);
			AppendChild(color_value);
			color_value->RemoveReference();
			input->RemoveReference();
			input->SetProperty( "display", "none" );
			UpdateValue();
		}
	}

	virtual void OnAttributeChange( const Rocket::Core::AttributeNameList &changed_attributes )
	{
		Rocket::Core::Element::OnAttributeChange( changed_attributes );

		// Pass all attributes down to the input element
		for ( Rocket::Core::AttributeNameList::const_iterator it = changed_attributes.begin(); it != changed_attributes.end(); ++it )
		{
			input->SetAttribute( *it, GetAttribute<Rocket::Core::String>( *it, "" ) );
		}
	}

	virtual void ProcessEvent( Rocket::Core::Event &event )
	{
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

			while ( ( elem = elem->GetParentNode() ) )
			{
				if ( elem == this )
				{
					input->SetProperty( "display", "inline" );
					color_value->SetProperty( "display", "none" );
					input->Focus();
					break;
				}
			}
		}
	}

private:
	void UpdateValue( void )
	{
		Rocket::Core::String string = "^7";

		while( color_value->HasChildNodes() )
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
