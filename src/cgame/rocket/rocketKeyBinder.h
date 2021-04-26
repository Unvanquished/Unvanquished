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

#ifndef ROCKETKEYBINDER_H
#define ROCKETKEYBINDER_H

#include <Rocket/Core.h>
#include <Rocket/Core/Element.h>
#include "../cg_local.h"
#include "rocket.h"

static const Str::StringRef TOGGLE_CONSOLE_COMMAND = "<console>";

static const Rocket::Core::String BINDABLE_KEY_EVENT = "bindableKey";
static const Rocket::Core::String BINDABLE_KEY_KEY = "bkey";

// The displayed bindings are refreshed periodically since they can also change due to a layout change or /bind command.
constexpr int KEY_BINDING_REFRESH_INTERVAL_MS = 500;

class RocketKeyBinder : public Rocket::Core::Element, public Rocket::Core::EventListener
{
public:
	RocketKeyBinder( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag ), nextKeyUpdateTime( 0 ), waitingForKeypress( false ), team( 0 ), cmd( "" ), mouse_x( 0 ), mouse_y( 0 ), context( nullptr )
	{
	}

	void OnAttributeChange( const Rocket::Core::AttributeNameList &changed_attributes )
	{
		Rocket::Core::Element::OnAttributeChange( changed_attributes );
		if ( changed_attributes.find( "cmd" ) != changed_attributes.end() )
		{
			cmd = GetAttribute( "cmd" )->Get<Rocket::Core::String>();
			nextKeyUpdateTime = rocketInfo.realtime;
		}

		if ( changed_attributes.find( "team" ) != changed_attributes.end() )
		{
			team = GetTeam( GetAttribute( "team" )->Get<Rocket::Core::String>().CString() );
			nextKeyUpdateTime = rocketInfo.realtime;
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
			context->AddEventListener( BINDABLE_KEY_EVENT, this );
		}
	}

	virtual void OnChildRemove( Element *child )
	{
		Element::OnChildRemove( child );
		if ( child == this )
		{
			context->RemoveEventListener( "mousemove", this );
			context->RemoveEventListener( "keydown", this );
			context->RemoveEventListener( BINDABLE_KEY_EVENT, this );
			context = nullptr;
		}
	}

	void OnUpdate()
	{
		if ( rocketInfo.realtime >= nextKeyUpdateTime && team >= 0 && !cmd.Empty() && !waitingForKeypress )
		{
			nextKeyUpdateTime = rocketInfo.realtime + KEY_BINDING_REFRESH_INTERVAL_MS;
			SetInnerRML( CG_EscapeHTMLText( CG_KeyBinding( cmd.CString(), team ) ).c_str() );
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
			auto keyIdentifier = ( Rocket::Core::Input::KeyIdentifier ) event.GetParameter< int >( "key_identifier", 0 );

			if ( keyIdentifier == Rocket::Core::Input::KeyIdentifier::KI_ESCAPE )
			{
				CancelSelection();
			}

			event.StopPropagation();
			return;
		}

		else if ( waitingForKeypress && event == "mousedown" && event.GetTargetElement() == this )
		{
			int button = event.GetParameter<int>( "button", 0 );

			// Convert from Rocket mouse buttons to Quake mouse buttons
			BindKey( Keyboard::Key( keyNum_t( button < 5 ? K_MOUSE1 + button : ( button - 5 ) + K_AUX1 ) ) );

			event.StopPropagation();
			return;
		}

		else if ( waitingForKeypress && event == "mousemove" )
		{
			context->ProcessMouseMove( mouse_x, mouse_y, 0 );
			event.StopPropagation();
			return;
		}

		else if ( waitingForKeypress && event == BINDABLE_KEY_EVENT )
		{
			auto key = Keyboard::Key::UnpackFromInt( event.GetParameter<int>( BINDABLE_KEY_KEY, 0 ) );
			BindKey(key);
			return;
		}
	}

protected:
	void CancelSelection() {
		waitingForKeypress = false;
		nextKeyUpdateTime = rocketInfo.realtime;
	}

	void BindKey( Keyboard::Key newKey )
	{
		// Don't want to make character binds
		if ( newKey.kind() != Keyboard::Key::Kind::SCANCODE && newKey.kind() != Keyboard::Key::Kind::KEYNUM )
		{
			return;
		}

		nextKeyUpdateTime = rocketInfo.realtime;
		waitingForKeypress = false;

		if (cmd == TOGGLE_CONSOLE_COMMAND.c_str()) {
			trap_Key_SetConsoleKeys({newKey});
			return;
		}

		// For a team-specific bind, this returns keys that have the command set for the specific
		// team as well as for the default team (when there is no team-specific bind overriding it.
		auto previouslyBoundKeys = trap_Key_GetKeysForBinds(team, { cmd.CString() })[0];
		for (Keyboard::Key key : previouslyBoundKeys)
		{
			trap_Key_SetBinding( key, team, "" );
		}
		if (team != 0) {
			// If the bind was set for the default team, then the previous attempt will have failed to remove it.
			previouslyBoundKeys = trap_Key_GetKeysForBinds(team, { cmd.CString() })[0];
			for (Keyboard::Key key : previouslyBoundKeys)
			{
				trap_Key_SetBinding( key, 0, "" );
			}
		}
		trap_Key_SetBinding( newKey, team, cmd.CString() );
	}

	int GetTeam( Rocket::Core::String team )
	{
		static const struct {
			int team;
			Rocket::Core::String label;
		} labels[] = {
			{ 3, "spectators" },
			{ 0, "default" },
			{ 1, "aliens" },
			{ 2, "humans" }
		};

		for ( const auto& teamLabel : labels )
		{
			if ( team == teamLabel.label )
			{
				return teamLabel.team;
			}
		}

		Log::Warn( "Team %s not found", team.CString() );
		return -1;
	}

private:
	int nextKeyUpdateTime;
	bool waitingForKeypress;
	int team;

	Rocket::Core::String cmd;
	int mouse_x;
	int mouse_y;
	Rocket::Core::Context* context;
};


#endif
