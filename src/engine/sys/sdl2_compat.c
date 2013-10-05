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

#include <SDL_version.h>

#if !SDL_VERSION_ATLEAST( 2, 0, 0 )

int colorBits = 0;

#include <SDL.h>
#include <SDL_video.h>
#include <SDL_syswm.h>
#include "sdl2_compat.h"

#ifdef SMP
#include <GL/glew.h>

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

// information for SDL_GL_MakeCurrent
#ifdef MACOS_X
static struct
{
	int           init;
	CGLContextObj ctx;
} glInfo;
#elif SDL_VIDEO_DRIVER_X11
static struct
{
	int         init;
	GLXContext  ctx;
	Display     *dpy;
	GLXDrawable drawable;
} glInfo;
#elif _WIN32
static struct
{
	int   init;
	HDC   hDC; // handle to device context
	HGLRC hGLRC; // handle to GL rendering context
} glInfo;
#else
static struct
{
	int   init;
} glInfo;
#endif
#endif // SMP

static const SDL_VideoInfo *desktopInfo = NULL; // the desktop video info

// desktopInfo points to this
static SDL_VideoInfo   sVideoInfo;

// internal pointer in the SDL_VideoInfo struct for desktopInfo
static SDL_PixelFormat sPixelFormat;

static SDL_Rect **modes;

typedef struct
{
	SDL_Surface *surface;
} SDL_GLContextInternal;

SDL_GLContext SDL_GL_CreateContext( SDL_Window *w )
{
	static SDL_GLContextInternal c;

	c.surface = w->surface;

	return &c;
}

#ifdef SMP
static void SDL_SetContext( void )
{
	if ( !glInfo.init )
	{
#if MACOS_X
		glInfo.ctx = CGLGetCurrentContext();
		glInfo.init = 1;
#elif SDL_VIDEO_DRIVER_X11
		glInfo.ctx = glXGetCurrentContext();
		glInfo.dpy = glXGetCurrentDisplay();
		glInfo.drawable = glXGetCurrentDrawable();
		glInfo.init = 1;
#elif WIN32
		SDL_SysWMinfo info;

		SDL_VERSION( &info.version );

		if ( SDL_GetWMInfo( &info ) )
		{
			glInfo.hDC = GetDC( info.window );
			glInfo.hGLRC = info.hglrc;
			glInfo.init = 1;
		}
		else
		{
			glInfo.hDC = 0;
			glInfo.hGLRC = NULL;
		}
#endif
	}
}

int SDL_GL_MakeCurrent( SDL_Window *win, SDL_GLContext context )
{
	SDL_SetContext();

	if ( !context  )
	{
#if MACOS_X
		return CGLSetCurrentContext( NULL );
#elif SDL_VIDEO_DRIVER_X11
		return !glXMakeCurrent( glInfo.dpy, None, NULL );
#elif WIN32
		return !wglMakeCurrent( glInfo.hDC, NULL );
#else
		return 1;
#endif
	}
	else
	{
#if MACOS_X
		return CGLSetCurrentContext( glInfo.ctx );
#elif SDL_VIDEO_DRIVER_X11
		return !glXMakeCurrent( glInfo.dpy, glInfo.drawable, glInfo.ctx );
#elif WIN32
		return !wglMakeCurrent( glInfo.hDC, glInfo.hGLRC );
#else
		return 1;
#endif
	}
}
#endif // SMP

static void SDL_SetDesktopVideoInfo( void )
{
	if ( !desktopInfo )
	{
		//save desktop video format
		desktopInfo = SDL_GetVideoInfo();
		SDL_memcpy( &sPixelFormat, desktopInfo->vfmt, sizeof( sPixelFormat ) );
		sPixelFormat.palette = NULL;
		SDL_memcpy( &sVideoInfo, desktopInfo, sizeof( sVideoInfo ) );
		sVideoInfo.vfmt = &sPixelFormat;
		desktopInfo = &sVideoInfo;
	}
}

static void SDL_SetVideoModeList( void )
{
	SDL_SetDesktopVideoInfo();

	if ( !modes )
	{
		modes = SDL_ListModes( desktopInfo->vfmt, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN );
	}
}

const char *SDL_GetCurrentVideoDriver( void )
{
	static char driverName[ 128 ];
	return SDL_VideoDriverName( driverName, sizeof( driverName ) );
}

const char *SDL_GetCurrentAudioDriver( void )
{
	static char driverName[ 128 ];
	return SDL_AudioDriverName( driverName, sizeof( driverName ) );
}

int SDL_GetWindowDisplayMode( SDL_Window *w, SDL_DisplayMode *mode )
{
	SDL_SetVideoModeList();

	if ( !modes )
	{
		return -1;
	}

	mode->w = w->surface->w;
	mode->h = w->surface->h;
	mode->format = 0;
	mode->refresh_rate = 0;
	return 1;
}

int SDL_GetNumDisplayModes( int display )
{
	int i;

	SDL_SetVideoModeList();

	if ( !modes )
	{
		return 0;
	}

	for ( i = 0; modes[ i ]; i++ ) { }

	return i;
}

int SDL_GetDisplayMode( int display, int index, SDL_DisplayMode *mode )
{
	SDL_SetVideoModeList();

	if ( !modes )
	{
		return -1;
	}

	if ( modes == ( SDL_Rect ** ) -1 )
	{
		mode->w = 0;
		mode->h = 0;
		mode->format = 0;
		mode->refresh_rate = 0;
	}
	else
	{
		mode->w = modes[ index ]->w;
		mode->h = modes[ index ]->h;
		mode->format = 0;
		mode->refresh_rate = 0;
	}

	return 0;
}

int SDL_SetWindowFullscreen( SDL_Window *w, int fullscreen )
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

SDL_Window *SDL_CreateWindow( const char *title, int x, int y, int w, int h, uint32_t flags )
{
	static SDL_Window win;
#ifdef SMP
	SDL_memset( &glInfo, 0, sizeof( glInfo ) );
#endif
	SDL_WM_SetCaption( title, title );

	SDL_SetDesktopVideoInfo();

	win.surface = SDL_SetVideoMode( w, h, colorBits, flags );

	return win.surface ? &win : NULL;
}

int SDL_GetDesktopDisplayMode( int display, SDL_DisplayMode *mode )
{
	SDL_SetDesktopVideoInfo();

	if ( desktopInfo->current_h > 0 )
	{
		mode->h = desktopInfo->current_h;
		mode->w = desktopInfo->current_w;
		mode->format = 0;
		return 0;
	}

	return -1;
}

// no support for window position in SDL 1.2
void SDL_GetWindowPosition( SDL_Window *w, int *x, int *y )
{
	*x = 0;
	*y = 0;
}

void SDL_GetWindowSize( SDL_Window *win, int *w, int *h )
{
	const SDL_VideoInfo *vid = SDL_GetVideoInfo();
	*w = vid->current_w;
	*h = vid->current_h;
}

int SDL_SetWindowBrightness( SDL_Window *w, float bright )
{
	int result1;
	int result2;

	result1 = SDL_SetGamma( bright, bright, bright );

	// Mysteriously, if you use an NVidia graphics card and multiple monitors,
	// SDL_SetGamma will incorrectly return false... the first time; ask
	// again and you get the correct answer. This is a suspected driver bug, see
	// http://bugzilla.icculus.org/show_bug.cgi?id=4316

	result2 = SDL_SetGamma( bright, bright, bright );

	if ( result1 >= 0 || result2 >= 0 )
	{
		return 0;
	}

	return -1;
}
#endif
