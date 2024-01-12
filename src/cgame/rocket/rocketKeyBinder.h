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

#include <RmlUi/Core.h>
#include "../cg_local.h"
#include "rocket.h"

static const Str::StringRef TOGGLE_CONSOLE_COMMAND = "<console>";

static const Rml::String BINDABLE_KEY_EVENT = "bindableKey";
static const Rml::String BINDABLE_KEY_KEY = "bkey";

// The displayed bindings are refreshed periodically since they can also change due to a layout change or /bind command.
constexpr int KEY_BINDING_REFRESH_INTERVAL_MS = 500;

class RocketKeyBinder : public Rml::Element, public Rml::EventListener
{
public:
	RocketKeyBinder( const Rml::String &tag ) : Rml::Element( tag ), nextKeyUpdateTime( 0 ), waitingForKeypress( false ), team( 0 ), cmd( "" ), context( nullptr )
	{
	}

	void OnAttributeChange( const Rml::ElementAttributes &changed_attributes ) override
	{
		Rml::Element::OnAttributeChange( changed_attributes );
		auto it = changed_attributes.find( "cmd" );
		if ( it != changed_attributes.end() )
		{
			cmd = it->second.Get<Rml::String>();
			nextKeyUpdateTime = rocketInfo.realtime;
		}

		it = changed_attributes.find( "team" );
		if ( it != changed_attributes.end() )
		{
			team = GetTeam( it->second.Get<Rml::String>().c_str() );
			nextKeyUpdateTime = rocketInfo.realtime;
		}
	}

	virtual void OnChildAdd( Element *child ) override
	{
		Element::OnChildAdd( child );
		if ( child == this )
		{
			context = GetContext();
			context->AddEventListener( "keydown", this, true ); // use capture phase so we can grab it before <body>'s escape handler
			context->AddEventListener( BINDABLE_KEY_EVENT, this );
		}
	}

	virtual void OnChildRemove( Element *child ) override
	{
		Element::OnChildRemove( child );
		if ( child == this )
		{
			context->RemoveEventListener( "keydown", this, true );
			context->RemoveEventListener( BINDABLE_KEY_EVENT, this );
			context = nullptr;
		}
	}

	void OnUpdate() override
	{
		Element::OnUpdate();

		if ( waitingForKeypress )
		{
			rocketInfo.cursorFreezeTime = rocketInfo.realtime;
		}
		else if ( rocketInfo.realtime >= nextKeyUpdateTime && team >= 0 && !cmd.empty() )
		{
			nextKeyUpdateTime = rocketInfo.realtime + KEY_BINDING_REFRESH_INTERVAL_MS;
			SetInnerRML( CG_EscapeHTMLText( CG_KeyBinding( cmd.c_str(), team ) ).c_str() );
		}
	}

	void ProcessDefaultAction( Rml::Event &event) override
	{
		Element::ProcessDefaultAction( event );
		ProcessEvent( event );
	}

	void ProcessEvent( Rml::Event &event ) override
	{
		if ( !waitingForKeypress && event == "mousedown" && event.GetTargetElement() == this )
		{
			waitingForKeypress = true;
			SetInnerRML( "Enter desired key..." );

			// fix mouse position inside the widget
			rocketInfo.cursorFreezeTime = rocketInfo.realtime;
			rocketInfo.cursorFreezeX = event.GetParameter<int>( "mouse_x", 0 );
			rocketInfo.cursorFreezeY = event.GetParameter<int>( "mouse_y", 0 );
		}

		else if ( waitingForKeypress && event == "keydown" )
		{
			auto keyIdentifier = ( Rml::Input::KeyIdentifier ) event.GetParameter< int >( "key_identifier", 0 );

			if ( keyIdentifier == Rml::Input::KeyIdentifier::KI_ESCAPE )
			{
				StopWaitingForKeypress();
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

		else if ( waitingForKeypress && event == BINDABLE_KEY_EVENT )
		{
			auto key = Keyboard::Key::UnpackFromInt( event.GetParameter<int>( BINDABLE_KEY_KEY, 0 ) );
			BindKey(key);
			return;
		}
	}

protected:
	void StopWaitingForKeypress() {
		waitingForKeypress = false;
		nextKeyUpdateTime = rocketInfo.realtime;
		rocketInfo.cursorFreezeTime = -1;
	}

	void BindKey( Keyboard::Key newKey )
	{
		// Don't want to make character binds
		if ( newKey.kind() != Keyboard::Key::Kind::SCANCODE && newKey.kind() != Keyboard::Key::Kind::KEYNUM )
		{
			return;
		}

		StopWaitingForKeypress();

		if (cmd == TOGGLE_CONSOLE_COMMAND.c_str()) {
			trap_Key_SetConsoleKeys({newKey});
			return;
		}

		// For a team-specific bind, this returns keys that have the command set for the specific
		// team as well as for the default team (when there is no team-specific bind overriding it.
		auto previouslyBoundKeys = trap_Key_GetKeysForBinds(team, { cmd.c_str() })[0];
		for (Keyboard::Key key : previouslyBoundKeys)
		{
			trap_Key_SetBinding( key, team, "" );
		}
		if (team != 0) {
			// If the bind was set for the default team, then the previous attempt will have failed to remove it.
			previouslyBoundKeys = trap_Key_GetKeysForBinds(team, { cmd.c_str() })[0];
			for (Keyboard::Key key : previouslyBoundKeys)
			{
				trap_Key_SetBinding( key, 0, "" );
			}
		}
		trap_Key_SetBinding( newKey, team, cmd.c_str() );
	}

	int GetTeam( Rml::String team )
	{
		static const struct {
			int team;
			Rml::String label;
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

		Log::Warn( "Team %s not found", team.c_str() );
		return -1;
	}

private:
	int nextKeyUpdateTime;
	bool waitingForKeypress;
	int team;

	Rml::String cmd;
	Rml::Context* context;
};


#endif
