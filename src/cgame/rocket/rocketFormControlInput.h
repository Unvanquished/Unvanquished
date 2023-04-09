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
#include <RmlUi/Core.h>

class CvarElementFormControlInput : public Rml::ElementFormControlInput, public Rml::EventListener
{
public:
	CvarElementFormControlInput( const Rml::String &tag ) : Rml::ElementFormControlInput( tag ), owner( nullptr ) { }

	virtual void OnAttributeChange( const Rml::ElementAttributes &changed_attributes )
	{
		Rml::ElementFormControlInput::OnAttributeChange( changed_attributes );
		Rml::ElementAttributes::const_iterator it = changed_attributes.find( "cvar" );
		if ( it != changed_attributes.end() )
		{
			cvar = it->second.Get< Rml::String >( "cvar" );
			UpdateValue();
		}

		it = changed_attributes.find( "type" );
		if ( it != changed_attributes.end() )
		{
			type = it->second.Get< Rml::String >( "type" );
		}
	}

	virtual void OnChildAdd( Element *child )
	{
		Rml::ElementFormControlInput::OnChildAdd( child );
		if ( child == this )
		{
			// Need to cache this because it is not available
			// when this element is being removed
			owner = GetOwnerDocument();
			owner->AddEventListener( Rml::EventId::Show, this );

			AddEventListener( Rml::EventId::Change, this );
		}
	}

	virtual void OnChildRemove( Element *child )
	{
		Rml::ElementFormControlInput::OnChildRemove( child );
		if ( child == this )
		{
			owner->RemoveEventListener( Rml::EventId::Show, this );
		}
	}

	virtual void ProcessDefaultAction( Rml::Event &event )
	{
		ElementFormControlInput::ProcessDefaultAction( event );
		ProcessEvent( event );
	}

	virtual void ProcessEvent( Rml::Event &event )
	{
		if ( !cvar.empty() )
		{
			if ( owner == event.GetTargetElement() && event == Rml::EventId::Show )
			{
				UpdateValue();
			}

			if ( this == event.GetTargetElement() )
			{
				if ( event == Rml::EventId::Blur && ( type != "checkbox" && type != "radio" ) )
				{
					SetCvarValueAndFlags( cvar, GetValue() );
				}

				else if ( event == Rml::EventId::Change && !ignoreChangeEvent && type == "range" )
				{
					SetCvarValueAndFlags( cvar, GetValue() );
				}

				else if ( event == Rml::EventId::Click && !IsDisabled() )
				{
					if ( type == "checkbox" )
					{
						if ( HasAttribute( "checked" ) )
						{
							SetCvarValueAndFlags( cvar, GetAttribute<Rml::String>( "checked-value", "1" ) );
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
		if ( !type.empty() )
		{
			// A change event may be dispatched from inside SetAttribute. Ignore it to prevent spurious
			// Cvar::Set calls. In particular this prevents a slider cvar from being clamped if it is
			// outside the slider's range (in other cases the cvar setting is merely wasteful).
			ignoreChangeEvent = true;

			if ( type == "checkbox" )
			{
				bool result;
				std::string stringValue = Cvar::GetValue( cvar.c_str() );
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
				if ( GetValue() == Cvar::GetValue( cvar.c_str() ).c_str() )
				{
					SetAttribute( "checked", "" );
				}
			}

			else
			{
				SetValue( Cvar::GetValue( cvar.c_str() ).c_str() );
			}

			ignoreChangeEvent = false;
		}
	}

	void SetCvarValueAndFlags( const Rml::String& cvar, Rml::String value )
	{
		if ( type == "range" )
		{
			size_t point = value.find( "." );
			if ( point != value.npos &&
			     point + 1 + strspn( value.c_str() + point + 1, "0" ) == value.size() )
			{
				value.erase( point ); // for compatibility with integer cvars
			}
		}
		Cvar::SetValue( cvar.c_str(), value.c_str() );
		Cvar::AddFlags( cvar.c_str(), Cvar::USER_ARCHIVE );
	}

	Rml::String cvar;
	Rml::String type;
	Rml::Element *owner;
	bool ignoreChangeEvent = false;
};

#endif
