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

#ifndef ROCKETKEYBINDER_H
#define ROCKETKEYBINDER_H

#include <Rocket/Core.h>
#include <Rocket/Core/Element.h>
#include "../cg_local.h"
#include "rocket.h"

#define DEFAULT_BINDING 0

class RocketKeyBinder : public Rocket::Core::Element, public Rocket::Core::EventListener
{
public:
	RocketKeyBinder( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag ), dirty_key( false ), waitingForKeypress( false ), team( 0 ), key( -1 ), cmd( "" ), mouse_x( 0 ), mouse_y( 0 )
	{
	}

	void OnAttributeChange( const Rocket::Core::AttributeNameList &changed_attributes )
	{
		Rocket::Core::Element::OnAttributeChange( changed_attributes );
		if ( changed_attributes.find( "cmd" ) != changed_attributes.end() )
		{
			cmd = GetAttribute( "cmd" )->Get<Rocket::Core::String>();
			dirty_key = true;
		}

		if ( changed_attributes.find( "team" ) != changed_attributes.end() )
		{
			team = GetTeam( GetAttribute( "team" )->Get<Rocket::Core::String>().CString() );
			dirty_key = true;
		}
	}

	virtual void OnChildAdd( Element *child )
	{
		Element::OnChildAdd( child );
		if ( child == this )
		{
			context = GetContext();
			context->AddEventListener( "mousemove", this );
			context->AddEventListener( "keydown", this );
		}
	}

	virtual void OnChildRemove( Element *child )
	{
		Element::OnChildRemove( child );
		if (  child == this )
		{
			context->RemoveEventListener( "mousemove", this );
			context->RemoveEventListener( "keydown", this );
			context = nullptr;
		}
	}

	void OnUpdate()
	{
		if ( dirty_key && team >= 0 )
		{
			dirty_key = false;
			SetInnerRML( CG_KeyBinding( cmd.CString(), team ) );
		}
	}

	void ProcessEvent( Rocket::Core::Event &event )
	{
		Element::ProcessEvent( event );
		if ( !waitingForKeypress && event == "mousedown" && event.GetTargetElement() == this )
		{
			waitingForKeypress = true;
			SetInnerRML( "Enter desired key..." );

			// fix mouse position inside the widget
			mouse_x = event.GetParameter<int>( "mouse_x", 0 );
			mouse_y = event.GetParameter<int>( "mouse_y", 0 );
		}

		else if ( waitingForKeypress && event == "keydown" )
		{
			int newKey = Rocket_ToQuake( ( Rocket::Core::Input::KeyIdentifier ) event.GetParameter< int >( "key_identifier", 0 ) );

			BindKey( newKey );

			event.StopPropagation();
			return;
		}

		else if ( waitingForKeypress && event == "mousedown" && event.GetTargetElement() == this )
		{
			int button = event.GetParameter<int>( "button", 0 );

			// Convert from Rocket mouse buttons to Quake mouse buttons
			BindKey( button < 5 ? K_MOUSE1 + button : ( button - 5 ) + K_AUX1 );

			event.StopPropagation();
			return;
		}

		else if ( waitingForKeypress && event == "mousemove" )
		{
			context->ProcessMouseMove( mouse_x, mouse_y, 0 );
			event.StopPropagation();
			return;
		}
	}

protected:
	void BindKey( int newKey )
	{
		// Don't accept the same key
		if ( key == newKey )
		{
			waitingForKeypress = false;
			dirty_key = true;
			return;
		}
		// Cancel selection
		else if ( newKey == K_ESCAPE )
		{
			waitingForKeypress = false;
			dirty_key = true;
			return;
		}

		trap_Key_SetBinding( newKey, team, cmd.CString() );

		if ( key > 0 )
		{
			trap_Key_SetBinding( key, team, "" );
		}

		key = newKey;
		dirty_key = true;
		waitingForKeypress = false;
	}

	int GetTeam( Rocket::Core::String team )
	{
		static const struct {
			char team;
			Rocket::Core::String label;
		} labels[] = {
			{ 0, "spectators" },
			{ 0, "default" },
			{ 1, "aliens" },
			{ 2, "humans" }
		};
		static const int NUM_LABELS = 4;

		for ( int i = 0; i < NUM_LABELS; ++i )
		{
			if ( team == labels[i].label )
			{
				return labels[ i ].team;
			}
		}

		Com_Printf( "^3Warning: Team %s not found", team.CString() );
		return -1;
	}

private:
	bool dirty_key;
	bool waitingForKeypress;
	int team;
	int key;

	Rocket::Core::String cmd;
	int mouse_x;
	int mouse_y;
	Rocket::Core::Context* context;
};


#endif
