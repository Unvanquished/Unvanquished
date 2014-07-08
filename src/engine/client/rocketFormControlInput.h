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

#ifndef ROCKETFORMCONTROLINPUT_H
#define ROCKETFORMCONTROLINPUT_H

#include "client.h"
#include "../framework/CvarSystem.h"
#include <Rocket/Core/Core.h>
#include <Rocket/Controls/ElementFormControlInput.h>

class CvarElementFormControlInput : public Rocket::Controls::ElementFormControlInput, public Rocket::Core::EventListener
{
public:
	CvarElementFormControlInput( const Rocket::Core::String &tag ) : Rocket::Controls::ElementFormControlInput( tag ) { }

	virtual void OnAttributeChange( const Rocket::Core::AttributeNameList &changed_attributes )
	{
		Rocket::Controls::ElementFormControlInput::OnAttributeChange( changed_attributes );

		if ( changed_attributes.find( "cvar" ) != changed_attributes.end() )
		{
			cvar = GetAttribute< Rocket::Core::String >( "cvar", "" );
			UpdateValue();
		}

		if ( changed_attributes.find( "type" ) != changed_attributes.end() )
		{
			type = GetAttribute< Rocket::Core::String >( "type", "" );
		}
	}

	virtual void OnChildAdd( Element *child )
	{
		Rocket::Controls::ElementFormControlInput::OnChildAdd( child );
		if ( child == this )
		{
			// Need to cache this because it is not available
			// when this element is being removed
			owner = GetOwnerDocument();
			owner->AddEventListener( "show", this );
		}
	}

	virtual void OnChildRemove( Element *child )
	{
		Rocket::Controls::ElementFormControlInput::OnChildRemove( child );
		if ( child == this )
		{
			owner->RemoveEventListener( "show", this );
		}
	}

	virtual void ProcessEvent( Rocket::Core::Event &event )
	{
		Rocket::Controls::ElementFormControlInput::ProcessEvent( event );

		if ( !cvar.Empty() )
		{
			if ( owner == event.GetTargetElement() && event == "show" )
			{
				UpdateValue();
			}

			if ( this == event.GetTargetElement() )
			{
				if ( event == "blur" && ( type != "checkbox" && type != "radio" ) )
				{
					Cvar::SetValue( cvar.CString(), GetValue().CString() );
				}

				else if ( event == "change" && type == "range" )
				{
					Cvar::SetValue( cvar.CString(), GetValue().CString() );
				}

				else if ( event == "click" && !IsDisabled() )
				{
					if ( type == "checkbox" )
					{
						if ( HasAttribute( "checked" ) )
						{
							Cvar::SetValue( cvar.CString(), "1" );
						}

						else
						{
							Cvar::SetValue( cvar.CString(), "0" );
						}
					}

					else if ( type == "radio" )
					{
						Cvar::SetValue( cvar.CString(), GetValue().CString() );
					}
				}
			}
		}
	}


	void UpdateValue( void )
	{
		if ( !type.Empty() )
		{
			if ( type == "checkbox" )
			{
				bool result;

				if ( Cvar::ParseCvarValue( Cvar::GetValue( cvar.CString() ).c_str(), result ) )
				{
					if ( result )
					{
						SetAttribute( "checked", "" );
						SetValue( "1" );
					}

					else
					{
						RemoveAttribute( "checked" );
						SetValue( "0" );
					}
				}
			}

			else if ( type == "radio" )
			{
				if ( GetValue() == Cvar::GetValue( cvar.CString() ).c_str() )
				{
					SetAttribute( "checked", "" );
				}
			}

			else
			{
				SetValue( Cvar::GetValue( cvar.CString() ).c_str() );
			}
		}

	}

private:
	Rocket::Core::String cvar;
	Rocket::Core::String type;
	Rocket::Core::Element *owner;
};

#endif
