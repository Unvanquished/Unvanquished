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

#ifndef ROCKETFORMCONTROLINPUT_H
#define ROCKETFORMCONTROLINPUT_H

#include "../cg_local.h"
#include <Rocket/Core/Core.h>
#include <Rocket/Controls/ElementFormControlInput.h>

class CvarElementFormControlInput : public Rocket::Controls::ElementFormControlInput, public Rocket::Core::EventListener
{
public:
	CvarElementFormControlInput( const Rocket::Core::String &tag ) : Rocket::Controls::ElementFormControlInput( tag ), owner( nullptr ) { }

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
					SetCvarValueAndFlags( cvar, GetValue() );
				}

				else if ( event == "change" && type == "range" )
				{
					SetCvarValueAndFlags( cvar, GetValue() );
				}

				else if ( event == "click" && !IsDisabled() )
				{
					if ( type == "checkbox" )
					{
						if ( HasAttribute( "checked" ) )
						{
							SetCvarValueAndFlags( cvar, GetAttribute<Rocket::Core::String>( "checked-value", "1" ) );
						}

						else
						{
							SetCvarValueAndFlags( cvar, "0" );
						}
					}

					else if ( type == "radio" )
					{
						SetCvarValueAndFlags( cvar, GetValue() );
					}
				}
			}
		}
	}

private:
	void UpdateValue()
	{
		if ( !type.Empty() )
		{
			if ( type == "checkbox" )
			{
				bool result;
				std::string stringValue = Cvar::GetValue( cvar.CString() );
				if ( !Cvar::ParseCvarValue( stringValue, result ) )
				{
					// ParseCvarValue<bool> will only work correctly for a Cvar<bool>
					// but it's also possible the cvar might be a legacy cvar which is parsed with atoi
					result = atoi(stringValue.c_str()) != 0;
				}
				if ( result )
				{
					SetAttribute( "checked", "" );
				}

				else
				{
					RemoveAttribute( "checked" );
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

	void SetCvarValueAndFlags( const Rocket::Core::String& cvar, Rocket::Core::String value )
	{
		if ( type == "range" )
		{
			size_t point = value.Find( "." );
			if ( point != value.npos &&
			     point + 1 + strspn( value.CString() + point + 1, "0" ) == value.Length() )
			{
				value.Erase( point ); // for compatibility with integer cvars
			}
		}
		Cvar::SetValue( cvar.CString(), value.CString() );
		Cvar::AddFlags( cvar.CString(), Cvar::USER_ARCHIVE );
	}

	Rocket::Core::String cvar;
	Rocket::Core::String type;
	Rocket::Core::Element *owner;
};

#endif
