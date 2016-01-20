/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

#include <SDL.h>
#include "client/client.h"
#include "qcommon/q_unicode.h"
#include "framework/CommandSystem.h"

static cvar_t       *in_keyboardDebug = nullptr;

static SDL_Joystick *stick = nullptr;

static bool     mouseAvailable = false;

static cvar_t       *in_mouse = nullptr;

static cvar_t       *in_nograb;

static cvar_t       *in_joystick = nullptr;
static cvar_t       *in_joystickDebug = nullptr;
static cvar_t       *in_joystickThreshold = nullptr;
static cvar_t       *in_joystickNo = nullptr;
static cvar_t       *in_joystickUseAnalog = nullptr;

//static cvar_t  *in_fullscreen = nullptr;

static cvar_t *in_xbox360Controller = nullptr;
static cvar_t *in_xbox360ControllerAvailable = nullptr;
static cvar_t *in_xbox360ControllerDebug = nullptr;

#define CTRL(a) (( a ) - 'a' + 1 )

static SDL_Window *window = nullptr;

/*
===============
IN_PrintKey
===============
*/
static void IN_PrintKey( const SDL_Keysym *keysym, keyNum_t key, bool down )
{
	if ( keysym->mod & KMOD_LSHIFT ) { Com_Printf( " KMOD_LSHIFT" ); }

	if ( keysym->mod & KMOD_RSHIFT ) { Com_Printf( " KMOD_RSHIFT" ); }

	if ( keysym->mod & KMOD_LCTRL ) { Com_Printf( " KMOD_LCTRL" ); }

	if ( keysym->mod & KMOD_RCTRL ) { Com_Printf( " KMOD_RCTRL" ); }

	if ( keysym->mod & KMOD_LALT ) { Com_Printf( " KMOD_LALT" ); }

	if ( keysym->mod & KMOD_RALT ) { Com_Printf( " KMOD_RALT" ); }

	if ( keysym->mod & KMOD_LGUI ) { Com_Printf( " KMOD_LGUI" ); }

	if ( keysym->mod & KMOD_RGUI ) { Com_Printf( " KMOD_RGUI" ); }

	if ( keysym->mod & KMOD_NUM ) { Com_Printf( " KMOD_NUM" ); }

	if ( keysym->mod & KMOD_CAPS ) { Com_Printf( " KMOD_CAPS" ); }

	if ( keysym->mod & KMOD_MODE ) { Com_Printf( " KMOD_MODE" ); }

	if ( keysym->mod & KMOD_RESERVED ) { Com_Printf( " KMOD_RESERVED" ); }

	Com_Printf( "%c 0x%02x \"%s\" Q:0x%02x(%s)\n", down ? '+' : ' ', keysym->scancode,
		    SDL_GetKeyName( keysym->sym ), key, Key_KeynumToString( key ) );
}

#define MAX_CONSOLE_KEYS 16

/*
===============
IN_IsConsoleKey

TODO: If the SDL_Scancode situation improves, use it instead of
      both of these methods
===============
*/
static bool IN_IsConsoleKey( keyNum_t key, const unsigned char character )
{
	typedef struct consoleKey_s
	{
		enum
		{
		  KEY,
		  CHARACTER
		} type;

		union
		{
			keyNum_t      key;
			int           character;
		} u;
	} consoleKey_t;

	static const struct {
		char name[8];
		int  key;
	} modMap[] = {
		{ "shift", K_SHIFT },
		{ "ctrl",  K_CTRL  },
		{ "alt",   K_ALT   },
		{ "super", K_SUPER },
	};

	static consoleKey_t consoleKeys[ MAX_CONSOLE_KEYS ];
	static int          numConsoleKeys = 0;
	static int          ifMod, unlessMod = 0;

	// Only parse the variable when it changes
	if ( cl_consoleKeys->modified )
	{
		const char *text_p, *token;

		cl_consoleKeys->modified = false;
		text_p = cl_consoleKeys->string;
		numConsoleKeys = 0;
		ifMod = unlessMod = 0;

		while ( numConsoleKeys < MAX_CONSOLE_KEYS )
		{
			consoleKey_t *c = &consoleKeys[ numConsoleKeys ];
			int          charCode = 0;

			token = COM_Parse( &text_p );

			if ( !token[ 0 ] )
			{
				break;
			}

			if ( token[ 0 ] == '+' && token[ 1 ] )
			{
				for (unsigned i = 0; i < ARRAY_LEN( modMap ); ++i )
				{
					if ( !Q_stricmp( token + 1, modMap[i].name ) )
					{
						ifMod |= 1 << i;
					}
				}
			}
			else if ( token[ 0 ] == '-' && token[ 1 ] )
			{
				for (unsigned i = 0; i < ARRAY_LEN( modMap ); ++i )
				{
					if ( !Q_stricmp( token + 1, modMap[i].name ) )
					{
						unlessMod |= 1 << i;
					}
				}
			}
			else if ( strlen( token ) == 4 )
			{
				charCode = Com_HexStrToInt( token );
			}

			if ( charCode > 0 )
			{
				c->type = consoleKey_t::CHARACTER;
				c->u.character = charCode;
			}
			else
			{
				c->type = consoleKey_t::KEY;
				c->u.key = (keyNum_t) Key_StringToKeynum( token );

				// 0 isn't a key
				if ( c->u.key <= 0 )
				{
					continue;
				}
			}

			numConsoleKeys++;
		}

		// if MOD is requested pressed and released, clear released
		unlessMod &= ~ifMod;
	}

	// require a +MOD, if there are any, to be pressed
	if ( ifMod )
	{
		bool flag = false;

		for (unsigned i = 0; i < ARRAY_LEN( modMap ); ++i )
		{
			if ( ( ifMod & 1 << i ) && keys[ modMap[i].key ].down )
			{
				flag = true;
				break;
			}
		}

		if ( !flag )
		{
			return false;
		}
	}

	// require all -MOD not to be pressed
	if ( unlessMod )
	{
		for (unsigned i = 0; i < ARRAY_LEN( modMap ); ++i )
		{
			if ( ( unlessMod & 1 << i ) && keys[ modMap[i].key ].down )
			{
				return false;
			}
		}
	}

	// If the character is the same as the key, prefer the character
	if ( key == character )
	{
		key = (keyNum_t) 0;
	}

	for (int i = 0; i < numConsoleKeys; i++ )
	{
		consoleKey_t *c = &consoleKeys[ i ];

		switch ( c->type )
		{
            case consoleKey_t::KEY:
				if ( key && c->u.key == key )
				{
					return true;
				}

				break;

            case consoleKey_t::CHARACTER:
				if ( c->u.character == character )
				{
					return true;
				}

				break;
		}
	}

	return false;
}

/*
===============
IN_TranslateSDLToQ3Key
===============
*/
static keyNum_t IN_TranslateSDLToQ3Key( SDL_Keysym *keysym, bool down )
{
	keyNum_t key = (keyNum_t) 0;

	if ( keysym->sym >= SDLK_SPACE && keysym->sym < SDLK_DELETE )
	{
		// These happen to match the ASCII chars
		key = ( keyNum_t ) keysym->sym;
	}
	else
	{
		switch ( keysym->sym )
		{
			case SDLK_PAGEUP:
				key = K_PGUP;
				break;

			case SDLK_KP_9:
				key = K_KP_PGUP;
				break;

			case SDLK_PAGEDOWN:
				key = K_PGDN;
				break;

			case SDLK_KP_3:
				key = K_KP_PGDN;
				break;

			case SDLK_KP_7:
				key = K_KP_HOME;
				break;

			case SDLK_HOME:
				key = K_HOME;
				break;

			case SDLK_KP_1:
				key = K_KP_END;
				break;

			case SDLK_END:
				key = K_END;
				break;

			case SDLK_KP_4:
				key = K_KP_LEFTARROW;
				break;

			case SDLK_LEFT:
				key = K_LEFTARROW;
				break;

			case SDLK_KP_6:
				key = K_KP_RIGHTARROW;
				break;

			case SDLK_RIGHT:
				key = K_RIGHTARROW;
				break;

			case SDLK_KP_2:
				key = K_KP_DOWNARROW;
				break;

			case SDLK_DOWN:
				key = K_DOWNARROW;
				break;

			case SDLK_KP_8:
				key = K_KP_UPARROW;
				break;

			case SDLK_UP:
				key = K_UPARROW;
				break;

			case SDLK_ESCAPE:
				key = K_ESCAPE;
				break;

			case SDLK_KP_ENTER:
				key = K_KP_ENTER;
				break;

			case SDLK_RETURN:
				key = K_ENTER;
				break;

			case SDLK_TAB:
				key = K_TAB;
				break;

			case SDLK_F1:
				key = K_F1;
				break;

			case SDLK_F2:
				key = K_F2;
				break;

			case SDLK_F3:
				key = K_F3;
				break;

			case SDLK_F4:
				key = K_F4;
				break;

			case SDLK_F5:
				key = K_F5;
				break;

			case SDLK_F6:
				key = K_F6;
				break;

			case SDLK_F7:
				key = K_F7;
				break;

			case SDLK_F8:
				key = K_F8;
				break;

			case SDLK_F9:
				key = K_F9;
				break;

			case SDLK_F10:
				key = K_F10;
				break;

			case SDLK_F11:
				key = K_F11;
				break;

			case SDLK_F12:
				key = K_F12;
				break;

			case SDLK_F13:
				key = K_F13;
				break;

			case SDLK_F14:
				key = K_F14;
				break;

			case SDLK_F15:
				key = K_F15;
				break;

			case SDLK_BACKSPACE:
				key = K_BACKSPACE;
				break;

			case SDLK_KP_PERIOD:
				key = K_KP_DEL;
				break;

			case SDLK_DELETE:
				key = K_DEL;
				break;

			case SDLK_PAUSE:
				key = K_PAUSE;
				break;

			case SDLK_LSHIFT:
			case SDLK_RSHIFT:
				key = K_SHIFT;
				break;

			case SDLK_LCTRL:
			case SDLK_RCTRL:
				key = K_CTRL;
				break;

			case SDLK_RGUI:
			case SDLK_LGUI:
				key = K_COMMAND;
				break;

			case SDLK_RALT:
			case SDLK_LALT:
				key = K_ALT;
				break;

			case SDLK_KP_5:
				key = K_KP_5;
				break;

			case SDLK_INSERT:
				key = K_INS;
				break;

			case SDLK_KP_0:
				key = K_KP_INS;
				break;

			case SDLK_KP_MULTIPLY:
				key = K_KP_STAR;
				break;

			case SDLK_KP_PLUS:
				key = K_KP_PLUS;
				break;

			case SDLK_KP_MINUS:
				key = K_KP_MINUS;
				break;

			case SDLK_KP_DIVIDE:
				key = K_KP_SLASH;
				break;

			case SDLK_MODE:
				key = K_MODE;
				break;

			case SDLK_HELP:
				key = K_HELP;
				break;

			case SDLK_PRINTSCREEN:
				key = K_PRINT;
				break;

			case SDLK_SYSREQ:
				key = K_SYSREQ;
				break;

			case SDLK_MENU:
				key = K_MENU;
				break;

			case SDLK_APPLICATION:
				key = K_COMPOSE;
				break;

			case SDLK_POWER:
				key = K_POWER;
				break;

			case SDLK_UNDO:
				key = K_UNDO;
				break;

			case SDLK_SCROLLLOCK:
				key = K_SCROLLOCK;
				break;

			case SDLK_NUMLOCKCLEAR:
				key = K_KP_NUMLOCK;
				break;

			case SDLK_CAPSLOCK:
				key = K_CAPSLOCK;
				break;

			default:
				break;
		}
	}

	if ( in_keyboardDebug->integer )
	{
		IN_PrintKey( keysym, key, down );
	}

	if ( IN_IsConsoleKey( key, 0 ) && !keys[ K_ALT ].down)
	{
		// Console keys can't be bound or generate characters
		// (but allow Alt+key for text input)
		key = K_CONSOLE;
	}

	return key;
}


// Whether the cursor is enabled
static MouseMode mouse_mode = MouseMode::SystemCursor;

/*
 * Returns whether the cursor is enabled
 */
MouseMode IN_GetMouseMode()
{
    return mouse_mode;
}

/*
 * Enables or disables the cursor
 */
void IN_SetMouseMode(MouseMode newMode)
{
	if ( newMode != mouse_mode )
	{
		if ( !mouseAvailable || !SDL_WasInit( SDL_INIT_VIDEO ) )
		{
			return;
		}

		SDL_ShowCursor( newMode == MouseMode::SystemCursor ? SDL_ENABLE : SDL_DISABLE );
		auto grab = newMode != MouseMode::Deltas || in_nograb->integer ? SDL_FALSE : SDL_TRUE;
		SDL_SetRelativeMouseMode( grab );
		SDL_SetWindowGrab( window, grab );

		if ( mouse_mode == MouseMode::Deltas )
		{
			IN_CenterMouse();
		}
		mouse_mode = newMode;
	}
}

static bool in_focus = false;

static void IN_SetFocus(bool hasFocus)
{
	if ( hasFocus == in_focus )
	{
		return;
	}

	in_focus = hasFocus;

	Com_QueueEvent( 0, SE_FOCUS, in_focus, 0, 0, nullptr );

}

/*
 * Moves the mouse at the center of the window
 */
void IN_CenterMouse()
{
	int w, h;
	SDL_GetWindowSize( window, &w, &h );
	SDL_WarpMouseInWindow( window, w / 2, h / 2 );
}

// We translate axes movement into keypresses
static int joy_keys[ 16 ] =
{
	K_LEFTARROW, K_RIGHTARROW,
	K_UPARROW,   K_DOWNARROW,
	K_JOY16,     K_JOY17,
	K_JOY18,     K_JOY19,
	K_JOY20,     K_JOY21,
	K_JOY22,     K_JOY23,

	K_JOY24,     K_JOY25,
	K_JOY26,     K_JOY27
};

// translate hat events into keypresses
// the 4 highest buttons are used for the first hat ...
static int hat_keys[ 16 ] =
{
	K_JOY29, K_JOY30,
	K_JOY31, K_JOY32,
	K_JOY25, K_JOY26,
	K_JOY27, K_JOY28,
	K_JOY21, K_JOY22,
	K_JOY23, K_JOY24,
	K_JOY17, K_JOY18,
	K_JOY19, K_JOY20
};

struct
{
	bool     buttons[ 16 ]; // !!! FIXME: these might be too many.
	unsigned int oldaxes;
	int          oldaaxes[ 16 ];
	unsigned int oldhats;
} stick_state;

/*
===============
IN_InitJoystick
===============
*/
static void IN_InitJoystick()
{
	int i = 0;
	int total = 0;

	if ( stick != nullptr )
	{
		SDL_JoystickClose( stick );
	}

	stick = nullptr;
	memset( &stick_state, '\0', sizeof( stick_state ) );

	if ( !in_joystick->integer && !in_xbox360Controller->integer )
	{
		Com_DPrintf( "Joystick is not active.\n" );

		if ( !in_xbox360Controller->integer )
		{
			Com_DPrintf( "Gamepad is not active.\n" );
			Cvar_Set( "in_xbox360ControllerAvailable", "0" );
		}

		return;
	}

	if ( !SDL_WasInit( SDL_INIT_JOYSTICK ) )
	{
		Com_DPrintf( "Calling SDL_Init(SDL_INIT_JOYSTICK)...\n" );

		if ( SDL_Init( SDL_INIT_JOYSTICK ) == -1 )
		{
			Com_DPrintf( "SDL_Init(SDL_INIT_JOYSTICK) failed: %s\n", SDL_GetError() );
			return;
		}

		Com_DPrintf( "SDL_Init(SDL_INIT_JOYSTICK) passed.\n" );
	}

	total = SDL_NumJoysticks();
	Com_DPrintf( "%d possible joysticks\n", total );

	for ( i = 0; i < total; i++ )
	{
		Com_DPrintf( "[%d] %s\n", i, SDL_JoystickNameForIndex( i ) );
	}

	in_joystickNo = Cvar_Get( "in_joystickNo", "0", 0 );

	if ( in_joystickNo->integer < 0 || in_joystickNo->integer >= total )
	{
		Cvar_Set( "in_joystickNo", "0" );
	}

	in_joystickUseAnalog = Cvar_Get( "in_joystickUseAnalog", "0", 0 );

	stick = SDL_JoystickOpen( in_joystickNo->integer );

	if ( stick == nullptr )
	{
		Com_DPrintf( "No joystick opened.\n" );
		return;
	}

	Com_DPrintf( "Joystick %d opened\n", in_joystickNo->integer );
	Com_DPrintf( "Name:    %s\n", SDL_JoystickNameForIndex( in_joystickNo->integer ) );
	Com_DPrintf( "Axes:    %d\n", SDL_JoystickNumAxes( stick ) );
	Com_DPrintf( "Hats:    %d\n", SDL_JoystickNumHats( stick ) );
	Com_DPrintf( "Buttons: %d\n", SDL_JoystickNumButtons( stick ) );
	Com_DPrintf( "Balls: %d\n", SDL_JoystickNumBalls( stick ) );
	Com_DPrintf( "Use Analog: %s\n", in_joystickUseAnalog->integer ? "Yes" : "No" );

	SDL_JoystickEventState( SDL_QUERY );

	// XBox 360 controller support
	if ( !Q_stricmp( SDL_JoystickNameForIndex( in_joystickNo->integer ), "Microsoft X-Box 360 pad" ) )
	{
		Cvar_Set( "in_xbox360ControllerAvailable", "1" );
	}
	else
	{
		Cvar_Set( "in_xbox360ControllerAvailable", "0" );
	}
}

/*
===============
IN_ShutdownJoystick
===============
*/
static void IN_ShutdownJoystick()
{
	if ( stick )
	{
		SDL_JoystickClose( stick );
		stick = nullptr;
	}

	SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
}

/*
===============
IN_JoyMove
===============
*/
static void IN_JoyMove()
{
	bool     joy_pressed[ ARRAY_LEN( joy_keys ) ];
	unsigned int axes = 0;
	unsigned int hats = 0;
	int          total = 0;
	int          i = 0;

	if ( !stick )
	{
		return;
	}

	if ( !in_joystick->integer )
	{
		return;
	}

	SDL_JoystickUpdate();

	memset( joy_pressed, '\0', sizeof( joy_pressed ) );

	// update the ball state.
	total = SDL_JoystickNumBalls( stick );

	if ( total > 0 )
	{
		int balldx = 0;
		int balldy = 0;

		for ( i = 0; i < total; i++ )
		{
			int dx = 0;
			int dy = 0;
			SDL_JoystickGetBall( stick, i, &dx, &dy );
			balldx += dx;
			balldy += dy;
		}

		if ( balldx || balldy )
		{
			// !!! FIXME: is this good for stick balls, or just mice?
			// Scale like the mouse input...
			if ( abs( balldx ) > 1 )
			{
				balldx *= 2;
			}

			if ( abs( balldy ) > 1 )
			{
				balldy *= 2;
			}

			Com_QueueEvent( 0, SE_MOUSE, balldx, balldy, 0, nullptr );
		}
	}

	// now query the stick buttons...
	total = SDL_JoystickNumButtons( stick );

	if ( total > 0 )
	{
		if ( total > (int) ARRAY_LEN( stick_state.buttons ) )
		{
			total = ARRAY_LEN( stick_state.buttons );
		}

		for ( i = 0; i < total; i++ )
		{
			bool pressed = ( SDL_JoystickGetButton( stick, i ) != 0 );

			if ( pressed != stick_state.buttons[ i ] )
			{
				if ( in_xbox360ControllerAvailable->integer )
				{
					if ( i == 0 )
					{
						Com_QueueEvent( 0, SE_KEY, K_XBOX360_A, pressed, 0, nullptr );
					}
				}
				else
				{
					Com_QueueEvent( 0, SE_KEY, K_JOY1 + i, pressed, 0, nullptr );
				}

				stick_state.buttons[ i ] = pressed;
			}
		}
	}

	// look at the hats...
	total = SDL_JoystickNumHats( stick );

	if ( total > 0 )
	{
		if ( total > 4 ) { total = 4; }

		for ( i = 0; i < total; i++ )
		{
			( ( Uint8 * ) &hats ) [ i ] = SDL_JoystickGetHat( stick, i );
		}
	}

	// update hat state
	if ( hats != stick_state.oldhats )
	{
		for ( i = 0; i < 4; i++ )
		{
			if ( ( ( Uint8 * ) &hats ) [ i ] != ( ( Uint8 * ) &stick_state.oldhats ) [ i ] )
			{
				// release event
				switch ( ( ( Uint8 * ) &stick_state.oldhats ) [ i ] )
				{
					case SDL_HAT_UP:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 0 ], false, 0, nullptr );
						break;

					case SDL_HAT_RIGHT:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 1 ], false, 0, nullptr );
						break;

					case SDL_HAT_DOWN:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 2 ], false, 0, nullptr );
						break;

					case SDL_HAT_LEFT:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 3 ], false, 0, nullptr );
						break;

					case SDL_HAT_RIGHTUP:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 0 ], false, 0, nullptr );
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 1 ], false, 0, nullptr );
						break;

					case SDL_HAT_RIGHTDOWN:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 2 ], false, 0, nullptr );
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 1 ], false, 0, nullptr );
						break;

					case SDL_HAT_LEFTUP:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 0 ], false, 0, nullptr );
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 3 ], false, 0, nullptr );
						break;

					case SDL_HAT_LEFTDOWN:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 2 ], false, 0, nullptr );
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 3 ], false, 0, nullptr );
						break;

					default:
						break;
				}

				// press event
				switch ( ( ( Uint8 * ) &hats ) [ i ] )
				{
					case SDL_HAT_UP:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 0 ], true, 0, nullptr );
						break;

					case SDL_HAT_RIGHT:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 1 ], true, 0, nullptr );
						break;

					case SDL_HAT_DOWN:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 2 ], true, 0, nullptr );
						break;

					case SDL_HAT_LEFT:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 3 ], true, 0, nullptr );
						break;

					case SDL_HAT_RIGHTUP:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 0 ], true, 0, nullptr );
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 1 ], true, 0, nullptr );
						break;

					case SDL_HAT_RIGHTDOWN:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 2 ], true, 0, nullptr );
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 1 ], true, 0, nullptr );
						break;

					case SDL_HAT_LEFTUP:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 0 ], true, 0, nullptr );
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 3 ], true, 0, nullptr );
						break;

					case SDL_HAT_LEFTDOWN:
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 2 ], true, 0, nullptr );
						Com_QueueEvent( 0, SE_KEY, hat_keys[ 4 * i + 3 ], true, 0, nullptr );
						break;

					default:
						break;
				}
			}
		}
	}

	// save hat state
	stick_state.oldhats = hats;

	// finally, look at the axes...
	total = SDL_JoystickNumAxes( stick );

	if ( total > 0 )
	{
		if ( total > 16 ) { total = 16; }

		for ( i = 0; i < total; i++ )
		{
			Sint16 axis = SDL_JoystickGetAxis( stick, i );

			if ( !in_joystickUseAnalog->integer )
			{
				float f = ( ( float ) axis ) / 32767.0f;

				if ( f < -in_joystickThreshold->value )
				{
					axes |= ( 1 << ( i * 2 ) );
				}
				else if ( f > in_joystickThreshold->value )
				{
					axes |= ( 1 << ( ( i * 2 ) + 1 ) );
				}
			}
			else
			{
				float f = ( ( float ) abs( axis ) ) / 32767.0f;

				if ( f < in_joystickThreshold->value ) { axis = 0; }

				if ( axis != stick_state.oldaaxes[ i ] )
				{
					Com_QueueEvent( 0, SE_JOYSTICK_AXIS, i, axis, 0, nullptr );

					stick_state.oldaaxes[ i ] = axis;
				}
			}
		}
	}

	/* Time to update axes state based on old vs. new. */
	if ( axes != stick_state.oldaxes )
	{
		for ( i = 0; i < 16; i++ )
		{
			if ( ( axes & ( 1 << i ) ) && !( stick_state.oldaxes & ( 1 << i ) ) )
			{
				Com_QueueEvent( 0, SE_KEY, joy_keys[ i ], true, 0, nullptr );
			}

			if ( !( axes & ( 1 << i ) ) && ( stick_state.oldaxes & ( 1 << i ) ) )
			{
				Com_QueueEvent( 0, SE_KEY, joy_keys[ i ], false, 0, nullptr );
			}
		}
	}

	/* Save for future generations. */
	stick_state.oldaxes = axes;
}

static void IN_XBox360Axis( int controllerAxis, int gameAxis, float scale )
{
	Sint16 axis = SDL_JoystickGetAxis( stick, controllerAxis );
	float  f = ( ( float ) axis ) / 32767.0f;

	if ( f > -in_joystickThreshold->value && f < in_joystickThreshold->value )
	{
		Com_QueueEvent( 0, SE_JOYSTICK_AXIS, gameAxis, 0, 0, nullptr );
	}
	else
	{
		if ( in_xbox360ControllerDebug->integer )
		{
			Com_Printf( "xbox axis %i = %f\n", controllerAxis, f );
		}

		Com_QueueEvent( 0, SE_JOYSTICK_AXIS, gameAxis, f * scale, 0, nullptr );
	}
}

static int IN_XBox360AxisToButton( int controllerAxis, int key, float expectedStartValue, float threshold )
{
	unsigned int axes = 0;

	Sint16       axis = SDL_JoystickGetAxis( stick, controllerAxis );
	float        f = ( ( float ) axis ) / 32767.0f;

	/*
	if(f < -in_joystickThreshold->value)
	{
	        axes |= (1 << (controllerAxis * 2));
	}
	else if(f > in_joystickThreshold->value)
	{
	        axes |= (1 << ((controllerAxis * 2) + 1));
	}
	*/

	if ( f > ( expectedStartValue + threshold + in_joystickThreshold->value ) )
	{
		axes |= ( 1 << ( controllerAxis ) );
	}

	if ( ( axes & ( 1 << controllerAxis ) ) && !( stick_state.oldaxes & ( 1 << controllerAxis ) ) )
	{
		Com_QueueEvent( 0, SE_KEY, key, true, 0, nullptr );

		if ( in_xbox360ControllerDebug->integer )
		{
			Com_Printf( "xbox axis = %i to key = Q:0x%02x(%s), value = %f\n", controllerAxis, key, Key_KeynumToString( key ), f );
		}
	}

	if ( !( axes & ( 1 << controllerAxis ) ) && ( stick_state.oldaxes & ( 1 << controllerAxis ) ) )
	{
		Com_QueueEvent( 0, SE_KEY, key, false, 0, nullptr );

		if ( in_xbox360ControllerDebug->integer )
		{
			Com_Printf( "xbox axis = %i to key = Q:0x%02x(%s), value = %f\n", controllerAxis, key, Key_KeynumToString( key ), f );
		}
	}

	return axes;
}

/*
===============
IN_Xbox360ControllerMove
===============
*/
static void IN_Xbox360ControllerMove()
{
	bool     joy_pressed[ ARRAY_LEN( joy_keys ) ];
	unsigned int axes = 0;
	unsigned int hat = 0;
	int          total = 0;
	int          i = 0;

	if ( !stick )
	{
		return;
	}

	if ( !in_joystick->integer )
	{
		return;
	}

	SDL_JoystickUpdate();

	memset( joy_pressed, '\0', sizeof( joy_pressed ) );

	// query the stick buttons...
	total = SDL_JoystickNumButtons( stick );

	if ( total > 0 )
	{
		if ( total > (int) ARRAY_LEN( stick_state.buttons ) )
		{
			total = ARRAY_LEN( stick_state.buttons );
		}

		for ( i = 0; i < total; i++ )
		{
			bool pressed = ( SDL_JoystickGetButton( stick, i ) != 0 );

			if ( pressed != stick_state.buttons[ i ] )
			{
				Com_QueueEvent( 0, SE_KEY, K_XBOX360_A + i, pressed, 0, nullptr );

				if ( in_xbox360ControllerDebug->integer )
				{
					Com_Printf( "xbox button = %i to key = Q:0x%02x(%s)\n", i, K_XBOX360_A + i, Key_KeynumToString( K_XBOX360_A + i ) );
				}

				stick_state.buttons[ i ] = pressed;
			}
		}
	}

	// look at the hats...
	total = SDL_JoystickNumHats( stick );
	hat = SDL_JoystickGetHat( stick, 0 );

	// update hat state
	if ( hat != stick_state.oldhats )
	{
		if ( hat != stick_state.oldhats )
		{
			int       key;

			const int allHatDirections = ( SDL_HAT_UP |
			                               SDL_HAT_RIGHT |
			                               SDL_HAT_DOWN |
			                               SDL_HAT_LEFT );

			if ( in_xbox360ControllerDebug->integer )
			{
				switch ( hat & allHatDirections )
				{
					case SDL_HAT_UP:
						key = K_XBOX360_DPAD_UP;
						break;

					case SDL_HAT_RIGHT:
						key = K_XBOX360_DPAD_RIGHT;
						break;

					case SDL_HAT_DOWN:
						key = K_XBOX360_DPAD_DOWN;
						break;

					case SDL_HAT_LEFT:
						key = K_XBOX360_DPAD_LEFT;
						break;

					case SDL_HAT_RIGHTUP:
						key = K_XBOX360_DPAD_RIGHTUP;
						break;

					case SDL_HAT_RIGHTDOWN:
						key = K_XBOX360_DPAD_RIGHTDOWN;
						break;

					case SDL_HAT_LEFTUP:
						key = K_XBOX360_DPAD_LEFTUP;
						break;

					case SDL_HAT_LEFTDOWN:
						key = K_XBOX360_DPAD_LEFTDOWN;
						break;

					default:
						key = 0;
						break;
				}

				if ( hat != SDL_HAT_CENTERED )
				{
					Com_Printf( "xbox hat bits = %i to key = Q:0x%02x(%s)\n", hat, key, Key_KeynumToString( key ) );
				}
			}

			// release event
			switch ( stick_state.oldhats & allHatDirections )
			{
				case SDL_HAT_UP:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_UP, false, 0, nullptr );
					break;

				case SDL_HAT_RIGHT:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_RIGHT, false, 0, nullptr );
					break;

				case SDL_HAT_DOWN:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_DOWN, false, 0, nullptr );
					break;

				case SDL_HAT_LEFT:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_LEFT, false, 0, nullptr );
					break;

				case SDL_HAT_RIGHTUP:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_RIGHTUP, false, 0, nullptr );
					break;

				case SDL_HAT_RIGHTDOWN:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_RIGHTDOWN, false, 0, nullptr );
					break;

				case SDL_HAT_LEFTUP:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_LEFTUP, false, 0, nullptr );
					break;

				case SDL_HAT_LEFTDOWN:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_LEFTDOWN, false, 0, nullptr );
					break;

				default:
					break;
			}

			// press event
			switch ( hat & allHatDirections )
			{
				case SDL_HAT_UP:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_UP, true, 0, nullptr );
					break;

				case SDL_HAT_RIGHT:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_RIGHT, true, 0, nullptr );
					break;

				case SDL_HAT_DOWN:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_DOWN, true, 0, nullptr );
					break;

				case SDL_HAT_LEFT:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_LEFT, true, 0, nullptr );
					break;

				case SDL_HAT_RIGHTUP:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_RIGHTUP, true, 0, nullptr );
					break;

				case SDL_HAT_RIGHTDOWN:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_RIGHTDOWN, true, 0, nullptr );
					break;

				case SDL_HAT_LEFTUP:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_LEFTUP, true, 0, nullptr );
					break;

				case SDL_HAT_LEFTDOWN:
					Com_QueueEvent( 0, SE_KEY, K_XBOX360_DPAD_LEFTDOWN, true, 0, nullptr );
					break;

				default:
					break;
			}
		}
	}

	// save hat state
	stick_state.oldhats = hat;

#if defined( WIN32 )
	// use left stick for strafing
	IN_XBox360Axis( 0, AXIS_SIDE, 127 );
	IN_XBox360Axis( 1, AXIS_FORWARD, -127 );

	// use right stick for viewing
	IN_XBox360Axis( 4, AXIS_YAW, -127 );
	IN_XBox360Axis( 3, AXIS_PITCH, 127 );

	axes |= IN_XBox360AxisToButton( 2, K_XBOX360_LT, -1, 0 );
	axes |= IN_XBox360AxisToButton( 5, K_XBOX360_RT, -1, 0 );
#else
	// use left stick for strafing
	IN_XBox360Axis( 0, AXIS_SIDE, 127 );
	IN_XBox360Axis( 1, AXIS_FORWARD, -127 );

	// use right stick for viewing
	IN_XBox360Axis( 3, AXIS_YAW, -127 );
	IN_XBox360Axis( 4, AXIS_PITCH, 127 );

	axes |= IN_XBox360AxisToButton( 2, K_XBOX360_LT, -1, 0 );
	axes |= IN_XBox360AxisToButton( 5, K_XBOX360_RT, -1, 0 );
#endif

	/* Save for future generations. */
	stick_state.oldaxes = axes;
}

/*
===============
IN_ProcessEvents
===============
*/
static void IN_ProcessEvents( bool dropInput )
{
	SDL_Event  e;
	keyNum_t   key = (keyNum_t) 0;
	static keyNum_t lastKeyDown = (keyNum_t) 0;

	if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		return;
	}

	while ( SDL_PollEvent( &e ) )
	{
		switch ( e.type )
		{
			case SDL_KEYDOWN:
				if ( !dropInput && ( !e.key.repeat || cls.keyCatchers ) )
				{
					key = IN_TranslateSDLToQ3Key( &e.key.keysym, true );

					if ( key )
					{
						Com_QueueEvent( 0, SE_KEY, key, true, 0, nullptr );
					}

					lastKeyDown = key;
				}

				break;

			case SDL_KEYUP:
				if ( !dropInput )
				{
					key = IN_TranslateSDLToQ3Key( &e.key.keysym, false );

					if ( key )
					{
						Com_QueueEvent( 0, SE_KEY, key, false, 0, nullptr );
					}

					lastKeyDown = (keyNum_t) 0;
				}

				break;
			case SDL_TEXTINPUT:
				if ( lastKeyDown != K_CONSOLE )
				{
					char *c = e.text.text;

					while ( *c )
					{
						int width = Q_UTF8_Width( c );
						int sc = Q_UTF8_Store( c );
						Com_QueueEvent( 0, SE_CHAR, sc, 0, 0, nullptr );
						c += width;
					}
				}
				break;
			case SDL_MOUSEMOTION:
				if ( !dropInput )
				{
					if ( mouse_mode != MouseMode::Deltas )
					{
						Com_QueueEvent( 0, SE_MOUSE_POS, e.motion.x, e.motion.y, 0, nullptr );
					}
					else
					{
						Com_QueueEvent( 0, SE_MOUSE, e.motion.xrel, e.motion.yrel, 0, nullptr );
#if defined( __linux__ ) || defined( __BSD__ )
						if ( !in_nograb->integer )
						{
							// work around X window managers and edge-based workspace flipping
							// - without this, we get LeaveNotify, no mouse button events, EnterNotify;
							//   we would likely miss some button presses and releases.
							IN_CenterMouse();
						}
#endif
					}
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if ( !dropInput )
				{
					unsigned char b;

					switch ( e.button.button )
					{
						case SDL_BUTTON_LEFT:
							b = K_MOUSE1;
							break;

						case SDL_BUTTON_MIDDLE:
							b = K_MOUSE3;
							break;

						case SDL_BUTTON_RIGHT:
							b = K_MOUSE2;
							break;
						case SDL_BUTTON_X1:
							b = K_MOUSE4;
							break;

						case SDL_BUTTON_X2:
							b = K_MOUSE5;
							break;

						default:
							b = K_AUX1 + ( e.button.button - ( SDL_BUTTON_X2 + 1 ) ) % 16;
							break;
					}

					Com_QueueEvent( 0, SE_KEY, b,
					                ( e.type == SDL_MOUSEBUTTONDOWN ? true : false ), 0, nullptr );
				}
				break;
			case SDL_MOUSEWHEEL:
				// FIXME: mouse wheel support shouldn't use keys!
				if ( e.wheel.y > 0 )
				{
					Com_QueueEvent( 0, SE_KEY, K_MWHEELUP, true, 0, nullptr );
					Com_QueueEvent( 0, SE_KEY, K_MWHEELUP, false, 0, nullptr );
				}
				else
				{
					Com_QueueEvent( 0, SE_KEY, K_MWHEELDOWN, true, 0, nullptr );
					Com_QueueEvent( 0, SE_KEY, K_MWHEELDOWN, false, 0, nullptr );
				}
				break;

			case SDL_WINDOWEVENT:
				switch( e.window.event )
				{
					case SDL_WINDOWEVENT_RESIZED:
						{
							char width[32], height[32];
							Com_sprintf( width, sizeof( width ), "%d", e.window.data1 );
							Com_sprintf( height, sizeof( height ), "%d", e.window.data2 );
							Cvar_Set( "r_customwidth", width );
							Cvar_Set( "r_customheight", height );
							Cvar_Set( "r_mode", "-1" );
						}
						break;

					case SDL_WINDOWEVENT_MINIMIZED:    Cvar_SetValue( "com_minimized", 1 ); break;
					case SDL_WINDOWEVENT_RESTORED:
					case SDL_WINDOWEVENT_MAXIMIZED:    Cvar_SetValue( "com_minimized", 0 ); break;
					case SDL_WINDOWEVENT_FOCUS_LOST:   Cvar_SetValue( "com_unfocused", 1 ); break;
					case SDL_WINDOWEVENT_FOCUS_GAINED: Cvar_SetValue( "com_unfocused", 0 ); break;
				}
				break;
			case SDL_QUIT:
				Cmd::ExecuteCommand("quit Closed window");
				break;
			default:
				break;
		}
	}
}

/*
===============
IN_IsNumLockDown
===============
*/

bool IN_IsNumLockDown() {
    return SDL_GetModState() & KMOD_NUM;
}
/*
===============
IN_Frame
===============
*/
static bool dropInput = false;

void IN_Frame()
{
	if ( in_xbox360ControllerAvailable->integer )
	{
		IN_Xbox360ControllerMove();
	}
	else
	{
		IN_JoyMove();
	}

	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		// Console is down in windowed mode
		IN_SetFocus( false );
	}
	else if ( !( SDL_GetWindowFlags( window ) & SDL_WINDOW_INPUT_FOCUS ) )
	{
		// Window doesn't have focus
		IN_SetFocus( false );
	}
	else if ( com_minimized->integer )
	{
		// Minimized
		IN_SetFocus( false );
	}
	else
	{
		IN_SetFocus( true );
	}

	IN_ProcessEvents( dropInput );
}

void IN_DropInputsForFrame()
{
	dropInput = true;
	in_focus = false;
}

void IN_FrameEnd()
{
	dropInput = false;
}

/*
===============
IN_Init
===============
*/
void IN_Init( void *windowData )
{
	int appState;

	if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		Com_Error( ERR_FATAL, "IN_Init called before SDL_Init( SDL_INIT_VIDEO )" );
	}

	window = (SDL_Window*) windowData;

	Com_DPrintf( "\n------- Input Initialization -------\n" );

	in_keyboardDebug = Cvar_Get( "in_keyboardDebug", "0", CVAR_TEMP );

	// mouse variables
	in_mouse = Cvar_Get( "in_mouse", "1", 0 );
	in_nograb = Cvar_Get( "in_nograb", "0", 0 );

	in_joystick = Cvar_Get( "in_joystick", "0",  CVAR_LATCH );
	in_joystickDebug = Cvar_Get( "in_joystickDebug", "0", CVAR_TEMP );
	in_joystickThreshold = Cvar_Get( "in_joystickThreshold", "0.15", 0 );

	in_xbox360Controller = Cvar_Get( "in_xbox360Controller", "1", CVAR_TEMP );
	in_xbox360ControllerAvailable = Cvar_Get( "in_xbox360ControllerAvailable", "0", CVAR_ROM );
	in_xbox360ControllerDebug = Cvar_Get( "in_xbox360ControllerDebug", "0", CVAR_TEMP );
	SDL_StartTextInput();
	mouseAvailable = ( in_mouse->value != 0 );
	IN_SetMouseMode( MouseMode::CustomCursor );

	appState = SDL_GetWindowFlags( window );
	Cvar_SetValue( "com_unfocused", !( appState & SDL_WINDOW_INPUT_FOCUS ) );
	Cvar_SetValue( "com_minimized", ( appState & SDL_WINDOW_MINIMIZED ) );
	IN_InitJoystick();
	Com_DPrintf( "------------------------------------\n" );
}

/*
===============
IN_Shutdown
===============
*/
void IN_Shutdown()
{
	SDL_StopTextInput();
	IN_SetMouseMode(MouseMode::SystemCursor);
	mouseAvailable = false;

	IN_ShutdownJoystick();

	window = nullptr;
}

/*
===============
IN_Restart
===============
*/
void IN_Restart()
{
	IN_ShutdownJoystick();
	IN_Init( window );
}
