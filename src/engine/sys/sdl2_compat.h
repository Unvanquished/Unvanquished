/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifndef SDL2_COMPAT_H
#define SDL2_COMPAT_H

#include <SDL_version.h>

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
static int colorBits = 0;
#endif

#if !SDL_VERSION_ATLEAST( 2, 0, 0 )
extern int colorBits; // set this value before SDL_CreateWindow

#include <SDL.h>
#include <SDL_video.h>
#include <SDL_syswm.h>

// shims for using SDL1.2 with the SDL2 API
// due to differences in the capabilities of the two APIs
// the shims should not be expected to work 100% the same way
// however, it is enough for our use cases

typedef struct
{
	int w;
	int h;
	uint32_t format;
	int refresh_rate;
} SDL_DisplayMode;

typedef struct
{
	SDL_Surface         *surface;
} SDL_Window;

typedef void* SDL_GLContext;

#define SDL_WINDOW_OPENGL SDL_OPENGL
#define SDL_WINDOW_FULLSCREEN SDL_FULLSCREEN
#define SDL_WINDOW_RESIZABLE SDL_RESIZABLE
#define SDL_WINDOW_SHOWN 0
#define SDL_WINDOW_HIDDEN 0x10
#define SDL_WINDOW_BORDERLESS SDL_NOFRAME
#define SDL_WINDOWPOS_CENTERED -1
#define SDL_WINDOWPOS_CENTERED_DISPLAY( d ) -1
#define SDL_WINDOWPOS_UNDEFINED -2
#define SDL_WINDOWPOS_UNDEFINED_DISPLAY( d ) -2
#define SDL_WINDOW_INPUT_FOCUS SDL_APPINPUTFOCUS

#define SDL_GetWindowFlags( w ) SDL_GetAppState()
#define SDL_GetWindowDisplayIndex( w ) 0
#define SDL_GetNumVideoDisplays() 1
#define SDL_GetWindowFlags( w ) SDL_GetAppState()
#define SDL_MinimizeWindow( w ) SDL_WM_IconifyWindow()
#define SDL_SetWindowIcon( w, i ) SDL_WM_SetIcon( i, NULL )
#define SDL_SetWindowGrab( w, t ) SDL_WM_GrabInput( ( SDL_GrabMode ) t )
#define SDL_SetRelativeMouseMode( t )
#define SDL_WarpMouseInWindow( w, x, y ) SDL_WarpMouse( x, y )
#define SDL_SetWindowGammaRamp( w, r, g, b ) SDL_SetGammaRamp( r, g, b )

#define SDL_Keycode SDLKey
#define SDL_Keysym SDL_keysym
#define SDLK_APPLICATION SDLK_COMPOSE
#define SDLK_SCROLLLOCK SDLK_SCROLLOCK
#define SDLK_LGUI SDLK_LSUPER
#define SDLK_RGUI SDLK_RSUPER
#define SDLK_KP_0 SDLK_KP0
#define SDLK_KP_1 SDLK_KP1
#define SDLK_KP_2 SDLK_KP2
#define SDLK_KP_3 SDLK_KP3
#define SDLK_KP_4 SDLK_KP4
#define SDLK_KP_5 SDLK_KP5
#define SDLK_KP_6 SDLK_KP6
#define SDLK_KP_7 SDLK_KP7
#define SDLK_KP_8 SDLK_KP8
#define SDLK_KP_9 SDLK_KP9
#define SDLK_NUMLOCKCLEAR SDLK_NUMLOCK
#define SDLK_PRINTSCREEN SDLK_PRINT
#define KMOD_LGUI KMOD_LMETA
#define KMOD_RGUI KMOD_RMETA
#define SDL_GetKeyboardState( k ) SDL_GetKeyState( k )
#define SDL_SCANCODE_ESCAPE SDLK_ESCAPE
#define SDL_SCANCODE_W SDLK_w
#define SDL_SCANCODE_S SDLK_s
#define SDL_SCANCODE_A SDLK_a
#define SDL_SCANCODE_D SDLK_d
#define SDL_SCANCODE_SPACE SDLK_SPACE
#define SDL_SCANCODE_C SDLK_c
#define SDL_SCANCODE_UP SDLK_UP
#define SDL_SCANCODE_DOWN SDLK_DOWN
#define SDL_SCANCODE_LEFT SDLK_LEFT
#define SDL_SCANCODE_RIGHT SDLK_RIGHT
#define SDL_JoystickNameForIndex( i ) SDL_JoystickName( i )

#define SDL_CreateThread( t, n, d ) SDL_CreateThread( t, d )

#define SDL_GL_DeleteContext( g )
#define SDL_HasSSE3() 0
#define SDL_HasSSE41() 0
#define SDL_HasSSE42() 0
#define SDL_GetVersion( v ) *( v ) = *SDL_Linked_Version()
#define SDL_GetWindowWMInfo( w, i ) SDL_GetWMInfo( i )

#define SDL_DestroyWindow( w )
#define SDL_GL_SwapWindow( w ) SDL_GL_SwapBuffers()

const char *SDL_GetCurrentVideoDriver( void );
const char *SDL_GetCurrentAudioDriver( void );

SDL_GLContext SDL_GL_CreateContext( SDL_Window *w );

#ifdef SMP
int SDL_GL_MakeCurrent( SDL_Window *win, SDL_GLContext context );

// depends on glewInit() being called before
// only used to setup data for SDL_GL_MakeCurrent, so it must be called before then
// will always return NULL
SDL_GLContext SDL_GL_GetCurrentContext( void );
#endif

// SDL 1.2 only supports one window
SDL_Window *SDL_CreateWindow( const char *title, int x, int y, int w, int h, uint32_t flags );

int SDL_GetWindowDisplayMode( SDL_Window *w, SDL_DisplayMode *mode );
int SDL_GetNumDisplayModes( int display );
int SDL_GetDisplayMode( int display, int index, SDL_DisplayMode *mode );

int SDL_SetWindowBrightness( SDL_Window *w, float bright );
int SDL_SetWindowFullscreen( SDL_Window *w, int fullscreen );

// SDL 1.2 does not support window positioning
void SDL_GetWindowPosition( SDL_Window *w, int *x, int *y );

void SDL_GetWindowSize( SDL_Window *win, int *w, int *h );
int SDL_GetDesktopDisplayMode( int display, SDL_DisplayMode *mode );

#endif
#endif