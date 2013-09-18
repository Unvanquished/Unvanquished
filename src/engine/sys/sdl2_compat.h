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
static int colorBits = 0;

#if !SDL_VERSION_ATLEAST( 2, 0, 0 )

#ifdef WIN32
#include <GL/wglew.h>
#else
#include <GL/glxew.h>
#endif

#ifdef MACOS_X
#include <OpenGL/OpenGL.h>
#elif SDL_VIDEO_DRIVER_X11
#include <GL/glx.h>
#endif

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
} SDL_DisplayMode;

typedef struct
{
	SDL_Surface   *surface;
	const SDL_VideoInfo *videoInfo;
#ifdef MACOS_X
	CGLContextObj context;
#elif SDL_VIDEO_DRIVER_X11
	GLXContext  ctx;
	Display     *dpy;
	GLXDrawable drawable;
#elif _WIN32
	HDC   hDC; // handle to device context
	HGLRC hGLRC; // handle to GL rendering context
#endif
} SDL_Window;

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
#define SDL_SetWindowGrab( w, t ) SDL_WM_GrabInput( t )
#define SDL_SetRelativeMouseMode( t )
#define SDL_WarpMouseInWindow( w, x, y ) SDL_WarpMouse( x, y )

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

typedef struct
{
	SDL_Surface *surface;
	const SDL_VideoInfo *videoInfo;
} SDL_GLContextInternal;

typedef SDL_GLContextInternal* SDL_GLContext;

static SDL_GLContext SDL_GL_CreateContext( SDL_Window *w )
{
	static SDL_GLContextInternal c;

	c.surface = w->surface;
	c.videoInfo = w->videoInfo;

	return &c;
}

#ifdef SMP
static int SDL_GL_MakeCurrent( SDL_Window *win, SDL_GLContext context )
{
	if ( !context  )
	{
#if MACOS_X
		return CGLSetCurrentContext( NULL );
#elif SDL_VIDEO_DRIVER_X11
		return !glXMakeCurrent( win->dpy, None, NULL );
#elif WIN32
		return !wglMakeCurrent( win->hDC, NULL );
#else
		return 1;
#endif
	}
	else
	{
#if MACOS_X
		return CGLSetCurrentContext( win->context );
#elif SDL_VIDEO_DRIVER_X11
		return !glXMakeCurrent( win->dpy, win->drawable, win->ctx );
#elif WIN32
		return !wglMakeCurrent( win->hDC, win->hGLRC );
#else
		return 1;
#endif
	}
}
#endif

static SDL_Rect **modes;

static int SDL_GetWindowDisplayMode( SDL_Window *w, SDL_DisplayMode *mode )
{
	modes = SDL_ListModes( w->videoInfo->vfmt, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN );
	if ( !modes )
	{
		return -1;
	}

	mode->w = w->surface->w;
	mode->h = w->surface->h;
	mode->format = 0;
	return 1;
}

static int SDL_GetNumDisplayModes( int display )
{
	int i;

	if ( !modes )
	{
		return 0;
	}

	for ( i = 0; modes[ i ]; i++ ) { }
	
	return i;
}

static int SDL_GetDisplayMode( int display, int index, SDL_DisplayMode *mode )
{
	if ( modes == ( SDL_Rect ** ) -1 )
	{
		mode->w = 0;
		mode->h = 0;
		mode->format = 0;
	}
	else
	{
		mode->w = modes[ index ]->w;
		mode->h = modes[ index ]->h;
		mode->format = 0;
	}
	return 0;
}

static char *SDL_GetCurrentVideoDriver()
{
	static char driverName[ 128 ];

	return SDL_VideoDriverName( driverName, sizeof( driverName ) );
}

static int SDL_SetWindowBrightness( SDL_Window *w, float bright )
{
	int result1;
	int result2;

	result1 = SDL_SetGamma( bright, bright, bright );

	// Mysteriously, if you use an NVidia graphics card and multiple monitors,
	// SDL_SetGamma will incorrectly return false... the first time; ask
	// again and you get the correct answer. This is a suspected driver bug, see
	// http://bugzilla.icculus.org/show_bug.cgi?id=4316

	result2 = SDL_SetGamma( bright, bright, bright );

	return result1 >= 0 || result2 >= 0;
}

static int SDL_SetWindowFullscreen( SDL_Window *w, int fullscreen )
{
	int currentstate = !!( w->surface->flags & SDL_WINDOW_FULLSCREEN );

	if ( currentstate != fullscreen )
	{
		if ( SDL_WM_ToggleFullScreen( w->surface ) )
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}

	return 0;
}

static SDL_Window *SDL_CreateWindow( const char *title, int x, int y, int w, int h, uint32_t flags )
{
	static SDL_Window win;
	static SDL_VideoInfo sVideoInfo;
	static SDL_PixelFormat sPixelFormat;

	if ( flags & SDL_WINDOW_HIDDEN )
	{
		win.videoInfo = SDL_GetVideoInfo();
		memcpy( &sPixelFormat, win.videoInfo->vfmt, sizeof( sPixelFormat ) );
		sPixelFormat.palette = NULL;
		memcpy( &sVideoInfo, win.videoInfo, sizeof( sVideoInfo ) );
		sVideoInfo.vfmt = &sPixelFormat;
		win.videoInfo = &sVideoInfo;
		win.surface = SDL_GetVideoSurface();
		return &win;
	}

	SDL_WM_SetCaption( title, title );
	win.videoInfo = SDL_GetVideoInfo();
	memcpy( &sPixelFormat, win.videoInfo->vfmt, sizeof( sPixelFormat ) );
	sPixelFormat.palette = NULL;
	memcpy( &sVideoInfo, win.videoInfo, sizeof( sVideoInfo ) );
	sVideoInfo.vfmt = &sPixelFormat;
	win.videoInfo = &sVideoInfo;

	win.surface = SDL_SetVideoMode( w, h, colorBits, flags );

	return win.surface ? &win : NULL;
}

static void SDL_GetWindowContext( SDL_Window *win )
{
#if MACOS_X
	win->context = CGLGetCurrentContext();
#elif SDL_VIDEO_DRIVER_X11
	win->ctx = glXGetCurrentContext();
	win->dpy = glXGetCurrentDisplay();
	win->drawable = glXGetCurrentDrawable();
#elif WIN32
	SDL_SysWMinfo info;

	SDL_VERSION( &info.version );

	if ( SDL_GetWMInfo( &info ) )
	{
		win->hDC = GetDC( info.window );
		win->hGLRC = info.hglrc;
	}
	else
	{
		win->hDC = 0;
		win->hGLRC = NULL;
	}
#endif
}

static void SDL_GetWindowPosition( SDL_Window *w, int *x, int *y )
{
	*x = 0;
	*y = 0;
}

static void SDL_GetWindowSize( SDL_Window *win, int *w, int *h )
{
	const SDL_VideoInfo *vid = SDL_GetVideoInfo();
	*w = vid->current_w;
	*h = vid->current_h;
}

static int SDL_GetDesktopDisplayMode( int display, SDL_DisplayMode *mode )
{
	const SDL_VideoInfo *v = SDL_GetVideoInfo();
	if ( v->current_h > 0 )
	{
		mode->h = v->current_h;
		mode->w = v->current_w;
		mode->format = 0;
		return 0;
	}
	
	return -1;
}

static char *SDL_GetCurrentAudioDriver( void )
{
	static char buf[ 128 ];
	return SDL_AudioDriverName( buf, sizeof( buf ) );
}
#endif
#endif