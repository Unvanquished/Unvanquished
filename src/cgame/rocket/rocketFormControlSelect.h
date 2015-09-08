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

#ifndef ROCKETFORMCONTROLSELECT_H
#define ROCKETFORMCONTROLSELECT_H

#include "../cg_local.h"
#include <Rocket/Core/Core.h>
#include <Rocket/Controls/ElementFormControlSelect.h>

class CvarElementFormControlSelect : public Rocket::Controls::ElementFormControlSelect, public Rocket::Core::EventListener
{
public:
	CvarElementFormControlSelect( const Rocket::Core::String &tag ) : Rocket::Controls::ElementFormControlSelect( tag ) { }

	virtual void OnAttributeChange( const Rocket::Core::AttributeNameList &changed_attributes )
	{
		Rocket::Controls::ElementFormControlSelect::OnAttributeChange( changed_attributes );

		if ( changed_attributes.find( "cvar" ) != changed_attributes.end() )
		{
			cvar = GetAttribute< Rocket::Core::String >( "cvar", "" );
			UpdateValue();
		}
	}

	virtual void OnChildAdd( Element *child )
	{
		Rocket::Controls::ElementFormControlSelect::OnChildAdd( child );
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
		Rocket::Controls::ElementFormControlSelect::OnChildRemove( child );
		if ( child == this )
		{
			owner->RemoveEventListener( "show", this );
		}
	}

	virtual void ProcessEvent( Rocket::Core::Event &event )
	{
		Rocket::Controls::ElementFormControlSelect::ProcessEvent( event );

		if ( !cvar.Empty() )
		{
			if ( owner == event.GetTargetElement() && event == "show" )
			{
				UpdateValue();
			}
			else if ( this == event.GetTargetElement() && event == "change" )
			{
				Cvar::SetValue( cvar.CString(), GetValue().CString() );
				Cvar::AddFlags( cvar.CString(), Cvar::USER_ARCHIVE );
			}
		}
	}

	void UpdateValue()
	{
		Rocket::Core::String value = Cvar::GetValue( cvar.CString() ).c_str();

		for ( int i = 0; i < GetNumOptions(); ++i )
		{
			Rocket::Controls::SelectOption *o = GetOption( i );
			if ( o->GetValue() == value )
			{
				SetSelection( i );
				return;
			}
		}

		SetSelection( -1 );
	}

private:
	Rocket::Core::String cvar;
	Rocket::Core::String type;
	Rocket::Core::Element *owner;
};

#endif
