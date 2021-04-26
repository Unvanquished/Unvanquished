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

#include "rocket.h"
#include "rocketKeyBinder.h"
#include "../cg_key_name.h"
#include "../cg_local.h"
#include "engine/qcommon/q_unicode.h"

using namespace Rocket::Core::Input;

static std::map< int, int > keyMap;
static bool init = false;

void Rocket_InitKeys()
{
	keyMap[ K_TAB ] = KI_TAB;
	keyMap[ K_ENTER ] = KI_RETURN;
	keyMap[ K_ESCAPE ] = KI_ESCAPE;
	keyMap[ K_SPACE ] = KI_SPACE;

	keyMap[ K_BACKSPACE ] = KI_BACK;

	keyMap[ K_COMMAND ] = KI_LWIN;
	keyMap[ K_CAPSLOCK ] = KI_CAPITAL;
	keyMap[ K_POWER ] = KI_POWER;
	keyMap[ K_PAUSE ] = KI_PAUSE;

	keyMap[ K_UPARROW ] = KI_UP;
	keyMap[ K_DOWNARROW ] = KI_DOWN;
	keyMap[ K_LEFTARROW ] = KI_LEFT;
	keyMap[ K_RIGHTARROW ] = KI_RIGHT;

	keyMap[ K_ALT ] = KI_LMETA;
	keyMap[ K_CTRL ] = KI_LCONTROL;
	keyMap[ K_SHIFT ] = KI_LSHIFT;
	keyMap[ K_INS ] = KI_INSERT;
	keyMap[ K_DEL ] = KI_DELETE;
	keyMap[ K_PGDN ] = KI_NEXT;
	keyMap[ K_PGUP ] = KI_PRIOR;
	keyMap[ K_HOME ] = KI_HOME;
	keyMap[ K_END ] = KI_END;

	keyMap[ K_F1 ] = KI_F1;
	keyMap[ K_F2 ] = KI_F2;
	keyMap[ K_F3 ] = KI_F3;
	keyMap[ K_F4 ] = KI_F4;
	keyMap[ K_F5 ] = KI_F5;
	keyMap[ K_F6 ] = KI_F6;
	keyMap[ K_F7 ] = KI_F7;
	keyMap[ K_F8 ] = KI_F8;
	keyMap[ K_F9 ] = KI_F9;
	keyMap[ K_F10 ] = KI_F10;
	keyMap[ K_F11 ] = KI_F11;
	keyMap[ K_F12 ] = KI_F12;
	keyMap[ K_F13 ] = KI_F13;
	keyMap[ K_F14 ] = KI_F14;
	keyMap[ K_F15 ] = KI_F15;

	keyMap[ K_KP_HOME ] = KI_NUMPAD7;
	keyMap[ K_KP_UPARROW ] = KI_NUMPAD8;
	keyMap[ K_KP_PGUP ] = KI_NUMPAD9;
	keyMap[ K_KP_LEFTARROW ] = KI_NUMPAD4;
	keyMap[ K_KP_5 ] = KI_NUMPAD5;
	keyMap[ K_KP_RIGHTARROW ] = KI_NUMPAD6;
	keyMap[ K_KP_END ] = KI_NUMPAD1;
	keyMap[ K_KP_DOWNARROW ] = KI_NUMPAD2;
	keyMap[ K_KP_PGDN ] = KI_NUMPAD3;
	keyMap[ K_KP_ENTER ] = KI_NUMPADENTER;
	keyMap[ K_KP_INS ] = KI_NUMPAD0;
	keyMap[ K_KP_DEL ] = KI_DECIMAL;
	keyMap[ K_KP_SLASH ] = KI_DIVIDE;
	keyMap[ K_KP_MINUS ] = KI_SUBTRACT;
	keyMap[ K_KP_PLUS ] = KI_ADD;
	keyMap[ K_KP_NUMLOCK ] = KI_NUMLOCK;
	keyMap[ K_KP_STAR ] = KI_MULTIPLY;
// 	keyMap[ K_KP_EQUALS ] = KI_KP_EQUALS;

	keyMap[ K_SUPER ] = KI_LWIN;
// 	keyMap[ K_COMPOSE ] = KI_COMPOSE;
// 	keyMap[ K_MODE ] = KI_MODE;
	keyMap[ K_HELP ] = KI_HELP;
	keyMap[ K_PRINT ] = KI_PRINT;
// 	keyMap[ K_SYSREQ ] = KI_SYSREQ;
	keyMap[ K_SCROLLLOCK ] = KI_SCROLL;
// 	keyMap[ K_BREAK ] = KI_BREAK;
	keyMap[ K_MENU ] = KI_APPS;
// 	keyMap[ K_EURO ] = KI_EURO;
// 	keyMap[ K_UNDO ] = KI_UNDO;

	// Corresponds to 0 - 9
	for ( int i = '0'; i < '9' + 1; ++i )
	{
		keyMap[ i ] = KI_0 + ( i - '0' );
	}

	// Corresponds to a - z
	for ( int i = 'a'; i < 'z' + 1; ++i )
	{
		keyMap[ i ] = KI_A + ( i - 'a' );
	}


	// Other keyMap that out of ascii order and are handled separately
	keyMap[ ';' ] = KI_OEM_1;      // US standard keyboard; the ';:' key.
	keyMap[ '=' ] = KI_OEM_PLUS;   // Any region; the '=+' key.
	keyMap[ ',' ] = KI_OEM_COMMA;  // Any region; the ',<' key.
	keyMap[ '-' ] = KI_OEM_MINUS;  // Any region; the '-_' key.
	keyMap[ '.' ] = KI_OEM_PERIOD; // Any region; the '.>' key.
	keyMap[ '/' ] = KI_OEM_2;      // Any region; the '/?' key.
	keyMap[ '`' ] = KI_OEM_3;      // Any region; the '`~' key.
	keyMap[ K_CONSOLE ] = KI_OEM_3;

	keyMap[ '[' ] = KI_OEM_4;      // US standard keyboard; the '[{' key.
	keyMap[ '\\' ] = KI_OEM_5;     // US standard keyboard; the '\|' key.
	keyMap[ ']' ] = KI_OEM_6;      // US standard keyboard; the ']}' key.
	keyMap[ '\'' ] = KI_OEM_7;     // US standard keyboard; the ''"' key.

	init = true;
}

KeyIdentifier Rocket_FromQuake( int key )
{
	if ( !init )
	{
		Log::Warn( "Tried to convert keyMap before key array initialized." );
		return KI_UNKNOWN;
	}
	std::map< int, int >::iterator it;
	it = keyMap.find( key );
	if ( it != keyMap.end() )
	{
		return static_cast< KeyIdentifier >( it->second );
	}

	return KI_UNKNOWN;
}

KeyModifier Rocket_GetKeyModifiers()
{
	using Keyboard::Key;
	int mod = 0;
	static const std::vector<Key> keys = { Key(K_CTRL), Key(K_SHIFT), Key(K_ALT), Key(K_SUPER), Key(K_CAPSLOCK), Key(K_KP_NUMLOCK) };
	static const int quakeToRocketKeyModifier[] = { KM_CTRL, KM_SHIFT, KM_ALT, KM_META, KM_CAPSLOCK, KM_NUMLOCK };
	std::vector<bool> list = trap_Key_KeysDown( keys );

	for (unsigned i = 0; i < keys.size(); ++i)
	{
		if ( list[i] )
		{
			mod |= quakeToRocketKeyModifier[i];
		}
	}
	return static_cast< KeyModifier >( mod );
}

static bool wasDownBefore = false;
void Rocket_ProcessMouseClick( int button, bool down )
{
	if ( !menuContext || rocketInfo.keyCatcher & KEYCATCH_CONSOLE || !rocketInfo.keyCatcher )
	{
		return;
	}

	if ( button == K_MOUSE1 )
	{
		if ( !down && !wasDownBefore )
		{
			return;
		} else if ( !down && wasDownBefore )
		{
			wasDownBefore = false;
		}
		else if ( down )
		{
			wasDownBefore = true;
		}
	}

	int idx = 0;
	if ( button <= K_MOUSE5 )
	{
		idx = button - K_MOUSE1;
	}
	else
	{
		idx = button - K_AUX1 + 5; // Start at 5 because of K_MOUSE5
	}

	if ( down )
	{
		menuContext->ProcessMouseButtonDown( idx, Rocket_GetKeyModifiers() );
	}
	else
	{
		menuContext->ProcessMouseButtonUp( idx, Rocket_GetKeyModifiers() );
	}
}

#define MOUSEWHEEL_DELTA 5
void Rocket_ProcessKeyInput( Keyboard::Key key, bool down )
{
	if ( !menuContext || rocketInfo.keyCatcher & KEYCATCH_CONSOLE || !rocketInfo.keyCatcher )
	{
		return;
	}

	keyNum_t keynum = key.AsKeynum();
	// Our input system sends mouse events as key presses.
	if ( ( keynum >= K_MOUSE1 && keynum <= K_MOUSE5 ) || ( keynum >= K_AUX1 && keynum <= K_AUX16 ) )
	{
		Rocket_ProcessMouseClick( keynum, down );
		return;
	}

	if ( ( keynum == K_MWHEELDOWN || keynum == K_MWHEELUP ) )
	{
		// Our input system sends an up event right after a down event
		// We only want to catch one of these.
		if ( !down )
		{
			return;
		}

		menuContext->ProcessMouseWheel( keynum == K_MWHEELDOWN ? MOUSEWHEEL_DELTA : -MOUSEWHEEL_DELTA, Rocket_GetKeyModifiers() );
		return;
	}
	KeyIdentifier rocketKey = Rocket_FromQuake( key.AsLegacyInt() );
	if ( down )
	{
		if (rocketKey != KI_UNKNOWN) {
			menuContext->ProcessKeyDown( rocketKey, Rocket_GetKeyModifiers() );
		}
		Rocket::Core::Element* focus = menuContext->GetFocusElement();
		if ( focus != nullptr ) {
			Rocket::Core::Dictionary dict;
			dict.Set( BINDABLE_KEY_KEY, key.PackIntoInt() );
			// Send after the rocket key event so that if Escape is pressed,
			// it will cancel the binding and not bind Escape.
			focus->DispatchEvent( BINDABLE_KEY_EVENT, dict );
		}
	}
	else
	{
		if (rocketKey != KI_UNKNOWN) {
			menuContext->ProcessKeyUp( rocketKey, Rocket_GetKeyModifiers() );
		}
	}
}


void Rocket_ProcessTextInput( int c )
{
	if ( !menuContext || rocketInfo.keyCatcher & KEYCATCH_CONSOLE || !rocketInfo.keyCatcher )
	{
		return;
	}

	//
	// ignore any non printable chars, or ones not representable in UCS-2
	//
	if ( c < 32 || c == 0x7f || c > 0xFFFF )
	{
		return;
	}

	menuContext->ProcessTextInput( c );
}

void Rocket_MouseMove( int x, int y )
{
	if ( !menuContext || ! ( rocketInfo.keyCatcher & KEYCATCH_UI ) )
	{
		return;
	}

	menuContext->ProcessMouseMove( x, y, Rocket_GetKeyModifiers() );
}

/*
================
CG_KeyBinding

The team argument corresponds to Keyboard::BindTeam, not team_t
================
*/
std::string CG_KeyBinding( const char* bind, int team )
{
	std::vector<Keyboard::Key> keys = bind == TOGGLE_CONSOLE_COMMAND
	                                  ? trap_Key_GetConsoleKeys()
	                                  : trap_Key_GetKeysForBinds( team, {bind} )[0];

	if ( keys.empty() )
	{
		return "Unbound";
	}

	std::string keyNames = CG_KeyDisplayName( keys[0] );

	for ( size_t i = 1; i < keys.size(); i++ )
	{
		keyNames += " or ";
		keyNames += CG_KeyDisplayName( keys[i] );
	}

	return keyNames;
}
