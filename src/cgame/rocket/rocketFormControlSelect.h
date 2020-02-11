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

#ifndef ROCKETFORMCONTROLSELECT_H
#define ROCKETFORMCONTROLSELECT_H

#include "../cg_local.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Controls/ElementFormControlSelect.h>

class CvarElementFormControlSelect : public Rml::Controls::ElementFormControlSelect, public Rml::Core::EventListener
{
public:
	CvarElementFormControlSelect( const Rml::Core::String &tag ) : Rml::Controls::ElementFormControlSelect( tag ) { }

	virtual void OnAttributeChange( const Rml::Core::ElementAttributes &changed_attributes )
	{
		Rml::Controls::ElementFormControlSelect::OnAttributeChange( changed_attributes );

		if ( changed_attributes.find( "cvar" ) != changed_attributes.end() )
		{
			cvar = GetAttribute< Rml::Core::String >( "cvar", "" );
			UpdateValue();
		}
	}

	virtual void OnChildAdd( Element *child )
	{
		Rml::Controls::ElementFormControlSelect::OnChildAdd( child );
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
		Rml::Controls::ElementFormControlSelect::OnChildRemove( child );
		if ( child == this )
		{
			owner->RemoveEventListener( "show", this );
		}
	}

	virtual void ProcessEvent( Rml::Core::Event &event )
	{
		Rml::Controls::ElementFormControlSelect::ProcessEvent( event );

		if ( !cvar.Empty() )
		{
			if ( owner == event.GetTargetElement() && event == "show" )
			{
				UpdateValue();
			}
			else if ( this == event.GetTargetElement() && event == "change" )
			{
				Cvar::SetValue( cvar.c_str(), GetValue().c_str() );
				Cvar::AddFlags( cvar.c_str(), Cvar::USER_ARCHIVE );
			}
		}
	}

	void UpdateValue()
	{
		Rml::Core::String value = Cvar::GetValue( cvar.c_str() ).c_str();

		for ( int i = 0; i < GetNumOptions(); ++i )
		{
			Rml::Controls::SelectOption *o = GetOption( i );
			if ( o->GetValue() == value )
			{
				SetSelection( i );
				return;
			}
		}

		SetSelection( -1 );
	}

private:
	Rml::Core::String cvar;
	Rml::Core::String type;
	Rml::Core::Element *owner;
};

#endif
