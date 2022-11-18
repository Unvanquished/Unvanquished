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
#include <RmlUi/Core.h>

class CvarElementFormControlSelect : public Rml::ElementFormControlSelect, public Rml::EventListener
{
public:
	CvarElementFormControlSelect( const Rml::String &tag ) :
			Rml::ElementFormControlSelect( tag ),
			owner( nullptr ),
			// Ignore the first value change since that is the one that is set by default.
			ignore_value_change( true ) { }

	virtual void OnAttributeChange( const Rml::ElementAttributes &changed_attributes )
	{
		Rml::ElementFormControlSelect::OnAttributeChange( changed_attributes );
		Rml::ElementAttributes::const_iterator it = changed_attributes.find( "cvar" );
		if ( it != changed_attributes.end() )
		{
			cvar = it->second.Get<Rml::String>();
			// Don't try to compare values when none have been added yet. This will happen because
			// attributes are parsed before children are added, so when this code runs initially,
			// we will never have any children.
			if ( GetNumOptions() > 0 )
			{
				UpdateValue();
			}
		}
	}

	virtual void OnChildAdd( Element *child )
	{
		Rml::ElementFormControlSelect::OnChildAdd( child );
		if ( child == this )
		{
			// Need to cache this because it is not available
			// when this element is being removed
			owner = GetOwnerDocument();
			if ( owner == nullptr )
			{
				return;
			}
			owner->AddEventListener( "show", this );
			owner->AddEventListener( "change", this );
		}
	}

	virtual void OnChildRemove( Element *child )
	{
		Rml::ElementFormControlSelect::OnChildRemove( child );
		if ( child == this && owner )
		{
			owner->RemoveEventListener( "show", this );
			owner->RemoveEventListener( "change", this );
		}
	}

	virtual void ProcessEvent( Rml::Event &event )
	{
		if ( !owner )
		{
			return;
		}
		if ( !cvar.empty() )
		{
			if ( owner == event.GetTargetElement() && event == "show" )
			{
				UpdateValue();
			}
			else if ( this == event.GetTargetElement() && event == "change" )
			{
				if ( ignore_value_change )
				{
					ignore_value_change = false;
					return;
				}
				Cvar::SetValue( cvar.c_str(), GetValue().c_str() );
				Cvar::AddFlags( cvar.c_str(), Cvar::USER_ARCHIVE );
			}
		}
	}

	void UpdateValue()
	{
		Rml::String cvarValue = Cvar::GetValue( cvar.c_str() );
		for ( int i = 0; i < GetNumOptions(); ++i )
		{
			Rml::Element* e = GetOption(i);
			Rml::String value = e->GetAttribute<Rml::String>( "value", Rml::String() );
			if ( value == cvarValue )
			{
				// If the selection is already correct, do not force set the cvar.
				ignore_value_change = GetSelection() != i;
				SetSelection( i );
				return;
			}
		}
		// No matches...Let's add an option for our current value.
		Add( Str::Format( "Custom Value (%s)", cvarValue ), cvarValue, 0, false );
		SetSelection( 0 );
		ignore_value_change = true;
	}

private:
	Rml::String cvar;
	Rml::Element *owner;
	bool ignore_value_change;
};

#endif
