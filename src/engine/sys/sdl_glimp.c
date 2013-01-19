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

#include <SDL.h>

#if !SDL_VERSION_ATLEAST(1, 2, 10)
#       define SDL_GL_ACCELERATED_VISUAL 15
#       define SDL_GL_SWAP_CONTROL       16
#elif MINSDL_PATCH >= 10
#       error Code block no longer necessary, please remove
#endif

#ifdef SMP
#       include <SDL_thread.h>
#       ifdef SDL_VIDEO_DRIVER_X11
#               include <X11/Xlib.h>
#       endif
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined( USE_XREAL_RENDERER )
#       include "../rendererGL/tr_local.h"
#else
#       include "../renderer/tr_local.h"
#endif

#include "../sys/sys_local.h"
#include "sdl_icon.h"
#include "SDL_syswm.h"

#if defined( WIN32 )
#       include <GL/wglew.h>
#else
#       include <GL/glxew.h>
#endif

//static qboolean SDL_VIDEODRIVER_externallySet = qfalse;

/* Just hack it for now. */
#ifdef MACOS_X
#include <OpenGL/OpenGL.h>
typedef CGLContextObj QGLContext;

static QGLContext opengl_context;

static void GLimp_GetCurrentContext( void )
{
	opengl_context = CGLGetCurrentContext();
}

#ifdef SMP
static void GLimp_SetCurrentContext( qboolean enable )
{
	if ( enable )
	{
		CGLSetCurrentContext( opengl_context );
	}
	else
	{
		CGLSetCurrentContext( NULL );
	}
}

#endif

#elif SDL_VIDEO_DRIVER_X11

#include <GL/glx.h>
typedef struct
{
	GLXContext  ctx;
	Display     *dpy;
	GLXDrawable drawable;
} QGLContext_t;
typedef QGLContext_t QGLContext;

static QGLContext opengl_context;

static void GLimp_GetCurrentContext( void )
{
	opengl_context.ctx = glXGetCurrentContext();
	opengl_context.dpy = glXGetCurrentDisplay();
	opengl_context.drawable = glXGetCurrentDrawable();
}

#ifdef SMP
static void GLimp_SetCurrentContext( qboolean enable )
{
	if ( enable )
	{
		glXMakeCurrent( opengl_context.dpy, opengl_context.drawable, opengl_context.ctx );
	}
	else
	{
		glXMakeCurrent( opengl_context.dpy, None, NULL );
	}
}

#endif

#elif _WIN32

typedef struct
{
	HDC   hDC; // handle to device context
	HGLRC hGLRC; // handle to GL rendering context
} QGLContext_t;
typedef QGLContext_t QGLContext;

static QGLContext opengl_context;

static void GLimp_GetCurrentContext( void )
{
	SDL_SysWMinfo info;

	SDL_VERSION( &info.version );

	if ( !SDL_GetWMInfo( &info ) )
	{
		ri.Printf( PRINT_WARNING, "Failed to obtain HWND from SDL (InputRegistry)\n" );
		return;
	}

	opengl_context.hDC = GetDC( info.window );
	opengl_context.hGLRC = info.hglrc;
}

#ifdef SMP
static void GLimp_SetCurrentContext( qboolean enable )
{
	if ( enable )
	{
		wglMakeCurrent( opengl_context.hDC, opengl_context.hGLRC );
	}
	else
	{
		wglMakeCurrent( opengl_context.hDC, NULL );
	}
}

#endif
#else
static void GLimp_GetCurrentContext( void )
{
}

#ifdef SMP
static void GLimp_SetCurrentContext( qboolean enable )
{
}

#endif
#endif

#ifdef SMP

/*
===========================================================

SMP acceleration

===========================================================
*/

/*
 * I have no idea if this will even work...most platforms don't offer
 * thread-safe OpenGL libraries, and it looks like the original Linux
 * code counted on each thread claiming the GL context with glXMakeCurrent(),
 * which you can't currently do in SDL. We'll just have to hope for the best.
 */

static SDL_mutex  *smpMutex = NULL;
static SDL_cond   *renderCommandsEvent = NULL;
static SDL_cond   *renderCompletedEvent = NULL;
static void ( *renderThreadFunction )( void ) = NULL;
static SDL_Thread *renderThread = NULL;

/*
===============
GLimp_RenderThreadWrapper
===============
*/
static int GLimp_RenderThreadWrapper( void *arg )
{
	// These printfs cause race conditions which mess up the console output
	Com_Printf( "Render thread starting\n" );

	renderThreadFunction();

	GLimp_SetCurrentContext( qfalse );

	Com_Printf( "Render thread terminating\n" );

	return 0;
}

/*
===============
GLimp_SpawnRenderThread
===============
*/
qboolean GLimp_SpawnRenderThread( void ( *function )( void ) )
{
	static qboolean warned = qfalse;

	if ( !warned )
	{
		Com_Printf( "WARNING: You enable r_smp at your own risk!\n" );
		warned = qtrue;
	}

#if !defined( MACOS_X ) && !defined( WIN32 ) && !defined ( SDL_VIDEO_DRIVER_X11 )
	return qfalse; /* better safe than sorry for now. */
#endif

	if ( renderThread != NULL ) /* hopefully just a zombie at this point... */
	{
		Com_Printf( "Already a render thread? Trying to clean it up...\n" );
		GLimp_ShutdownRenderThread();
	}

	smpMutex = SDL_CreateMutex();

	if ( smpMutex == NULL )
	{
		Com_Printf( "smpMutex creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}

	renderCommandsEvent = SDL_CreateCond();

	if ( renderCommandsEvent == NULL )
	{
		Com_Printf( "renderCommandsEvent creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}

	renderCompletedEvent = SDL_CreateCond();

	if ( renderCompletedEvent == NULL )
	{
		Com_Printf( "renderCompletedEvent creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}

	renderThreadFunction = function;
	renderThread = SDL_CreateThread( GLimp_RenderThreadWrapper, NULL );

	if ( renderThread == NULL )
	{
		ri.Printf( PRINT_ALL, "SDL_CreateThread() returned %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}
	else
	{
		// tma 01/09/07: don't think this is necessary anyway?
		//
		// !!! FIXME: No detach API available in SDL!
		//ret = pthread_detach( renderThread );
		//if ( ret ) {
		//ri.Printf( PRINT_ALL, "pthread_detach returned %d: %s", ret, strerror( ret ) );
		//}
	}

	return qtrue;
}

/*
===============
GLimp_ShutdownRenderThread
===============
*/
void GLimp_ShutdownRenderThread( void )
{
	if ( renderThread != NULL )
	{
		SDL_WaitThread( renderThread, NULL );
		renderThread = NULL;
	}

	if ( smpMutex != NULL )
	{
		SDL_DestroyMutex( smpMutex );
		smpMutex = NULL;
	}

	if ( renderCommandsEvent != NULL )
	{
		SDL_DestroyCond( renderCommandsEvent );
		renderCommandsEvent = NULL;
	}

	if ( renderCompletedEvent != NULL )
	{
		SDL_DestroyCond( renderCompletedEvent );
		renderCompletedEvent = NULL;
	}

	renderThreadFunction = NULL;
}

static volatile void     *smpData = NULL;
static volatile qboolean smpDataReady;

/*
===============
GLimp_RendererSleep
===============
*/
void           *GLimp_RendererSleep( void )
{
	void *data = NULL;

	GLimp_SetCurrentContext( qfalse );

	SDL_LockMutex( smpMutex );
	{
		smpData = NULL;
		smpDataReady = qfalse;

		// after this, the front end can exit GLimp_FrontEndSleep
		SDL_CondSignal( renderCompletedEvent );

		while ( !smpDataReady )
		{
			SDL_CondWait( renderCommandsEvent, smpMutex );
		}

		data = ( void * ) smpData;
	}
	SDL_UnlockMutex( smpMutex );

	GLimp_SetCurrentContext( qtrue );

	return data;
}

/*
===============
GLimp_FrontEndSleep
===============
*/
void GLimp_FrontEndSleep( void )
{
	SDL_LockMutex( smpMutex );
	{
		while ( smpData )
		{
			SDL_CondWait( renderCompletedEvent, smpMutex );
		}
	}
	SDL_UnlockMutex( smpMutex );

	GLimp_SetCurrentContext( qtrue );
}

/*
===============
GLimp_WakeRenderer
===============
*/
void GLimp_WakeRenderer( void *data )
{
	GLimp_SetCurrentContext( qfalse );

	SDL_LockMutex( smpMutex );
	{
		assert( smpData == NULL );
		smpData = data;
		smpDataReady = qtrue;

		// after this, the renderer can continue through GLimp_RendererSleep
		SDL_CondSignal( renderCommandsEvent );
	}
	SDL_UnlockMutex( smpMutex );
}

#else

// No SMP - stubs
void GLimp_RenderThreadWrapper( void *arg )
{
}

qboolean GLimp_SpawnRenderThread( void ( *function )( void ) )
{
	ri.Printf( PRINT_WARNING, "ERROR: SMP support was disabled at compile time\n" );
	return qfalse;
}

void GLimp_ShutdownRenderThread( void )
{
}

void           *GLimp_RendererSleep( void )
{
	return NULL;
}

void GLimp_FrontEndSleep( void )
{
}

void GLimp_WakeRenderer( void *data )
{
}

#endif

typedef enum
{
  RSERR_OK,

  RSERR_INVALID_FULLSCREEN,
  RSERR_INVALID_MODE,
  RSERR_OLD_GL,

  RSERR_UNKNOWN
} rserr_t;

static SDL_Surface         *screen = NULL;
static const SDL_VideoInfo *videoInfo = NULL;

cvar_t                     *r_allowResize; // make window resizable
cvar_t                     *r_centerWindow;
cvar_t                     *r_sdlDriver;
cvar_t                     *r_minimize;

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown( void )
{
	ri.Printf( PRINT_DEVELOPER, "Shutting down OpenGL subsystem\n" );

	ri.IN_Shutdown();

	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	screen = NULL;

#if defined( SMP )

	if ( renderThread != NULL )
	{
		Com_Printf( "Destroying renderer thread...\n" );
		GLimp_ShutdownRenderThread();
	}

#endif

	Com_Memset( &glConfig, 0, sizeof( glConfig ) );
	Com_Memset( &glState, 0, sizeof( glState ) );
}

/*
===============
GLimp_CompareModes
===============
*/
static int GLimp_CompareModes( const void *a, const void *b )
{
	const float ASPECT_EPSILON = 0.001f;
	SDL_Rect    *modeA = * ( SDL_Rect ** ) a;
	SDL_Rect    *modeB = * ( SDL_Rect ** ) b;
	float       aspectA = ( float ) modeA->w / ( float ) modeA->h;
	float       aspectB = ( float ) modeB->w / ( float ) modeB->h;
	int         areaA = modeA->w * modeA->h;
	int         areaB = modeB->w * modeB->h;
	float       aspectDiffA = fabs( aspectA - displayAspect );
	float       aspectDiffB = fabs( aspectB - displayAspect );
	float       aspectDiffsDiff = aspectDiffA - aspectDiffB;

	if ( aspectDiffsDiff > ASPECT_EPSILON )
	{
		return 1;
	}
	else if ( aspectDiffsDiff < -ASPECT_EPSILON )
	{
		return -1;
	}
	else
	{
		return areaA - areaB;
	}
}

/*
===============
GLimp_DetectAvailableModes
===============
*/
static void GLimp_DetectAvailableModes( void )
{
	char     buf[ MAX_STRING_CHARS ] = { 0 };
	SDL_Rect **modes;
	int      numModes;
	int      i;

	modes = SDL_ListModes( videoInfo->vfmt, SDL_OPENGL | SDL_FULLSCREEN );

	if ( !modes )
	{
		ri.Printf( PRINT_WARNING, "Can't get list of available modes\n" );
		return;
	}

	if ( modes == ( SDL_Rect ** ) - 1 )
	{
		ri.Printf( PRINT_ALL, "Display supports any resolution\n" );
		return; // can set any resolution
	}

	for ( numModes = 0; modes[ numModes ]; numModes++ ) {; }

	if ( numModes > 1 )
	{
		qsort( modes, numModes, sizeof( SDL_Rect * ), GLimp_CompareModes );
	}

	for ( i = 0; i < numModes; i++ )
	{
		const char *newModeString = va( "%ux%u ", modes[ i ]->w, modes[ i ]->h );

		if ( strlen( newModeString ) < ( int ) sizeof( buf ) - strlen( buf ) )
		{
			Q_strcat( buf, sizeof( buf ), newModeString );
		}
		else
		{
			ri.Printf( PRINT_WARNING, "Skipping mode %ux%x, buffer too small\n", modes[ i ]->w, modes[ i ]->h );
		}
	}

	if ( *buf )
	{
		buf[ strlen( buf ) - 1 ] = 0;
		ri.Printf( PRINT_ALL, "Available modes: '%s'\n", buf );
		ri.Cvar_Set( "r_availableModes", buf );
	}
}

#if defined( USE_XREAL_RENDERER )
static qboolean GLimp_InitOpenGL3xContext( void )
{
#if defined( WIN32 ) || defined( __linux__ )
	int        retVal;
	const char *success[] = { "failed", "success" };
#endif
	int        GLmajor, GLminor;

	GLimp_GetCurrentContext();
	sscanf( ( const char * ) glGetString( GL_VERSION ), "%d.%d", &GLmajor, &GLminor );

	// GL_VERSION returns the highest OpenGL version supported by the driver
	// which is also compatible with the version we requested (i.e. OpenGL 1.1).
	// Requesting a version below that will just give us the same GL version
	// again, so just keep the context, but pretend to the engine that we
	// have the lower version.
	if ( r_glMajorVersion->integer && r_glMinorVersion->integer &&
	     100 * r_glMajorVersion->integer + r_glMinorVersion->integer <=
	     100 * GLmajor + GLminor )
	{
		GLmajor = r_glMajorVersion->integer;
		GLminor = r_glMinorVersion->integer;
	}

	// Check if we have to create a core profile.
	// Core profiles are not necessarily compatible, so we have
	// to request the desired version.
#if defined( WIN32 )

	if ( WGLEW_ARB_create_context_profile &&
	     ( r_glCoreProfile->integer || r_glDebugProfile->integer ) )
	{
		int attribs[ 256 ]; // should be really enough
		int numAttribs;

		memset( attribs, 0, sizeof( attribs ) );
		numAttribs = 0;

		if ( r_glMajorVersion->integer > 0 )
		{
			attribs[ numAttribs++ ] = WGL_CONTEXT_MAJOR_VERSION_ARB;
			attribs[ numAttribs++ ] = r_glMajorVersion->integer;

			attribs[ numAttribs++ ] = WGL_CONTEXT_MINOR_VERSION_ARB;
			attribs[ numAttribs++ ] = r_glMinorVersion->integer;
		}

		attribs[ numAttribs++ ] = WGL_CONTEXT_FLAGS_ARB;

		if ( r_glDebugProfile->integer )
		{
			attribs[ numAttribs++ ] = WGL_CONTEXT_DEBUG_BIT_ARB;
		}
		else
		{
			attribs[ numAttribs++ ] = 0;
		}

		attribs[ numAttribs++ ] = WGL_CONTEXT_PROFILE_MASK_ARB;

		if ( r_glCoreProfile->integer )
		{
			attribs[ numAttribs++ ] = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
		}
		else
		{
			attribs[ numAttribs++ ] = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
		}

		// set current context to NULL
		retVal = wglMakeCurrent( opengl_context.hDC, NULL ) != 0;
		ri.Printf( PRINT_ALL, "...wglMakeCurrent( %p, %p ): %s\n", opengl_context.hDC, NULL, success[ retVal ] );

		// delete HGLRC
		if ( opengl_context.hGLRC )
		{
			retVal = wglDeleteContext( opengl_context.hGLRC ) != 0;
			ri.Printf( PRINT_ALL, "...deleting initial GL context: %s\n", success[ retVal ] );
			opengl_context.hGLRC = NULL;
		}

		ri.Printf( PRINT_ALL, "...initializing new OpenGL context" );

		opengl_context.hGLRC = wglCreateContextAttribsARB( opengl_context.hDC, 0, attribs );

		if ( wglMakeCurrent( opengl_context.hDC, opengl_context.hGLRC ) )
		{
			ri.Printf( PRINT_ALL, " done\n" );
		}
		else
		{
			ri.Printf( PRINT_WARNING, " Could not initialize requested OpenGL profile\n" );
		}
	}

#elif defined( __linux__ )

	if ( GLXEW_ARB_create_context_profile &&
	     ( r_glCoreProfile->integer || r_glDebugProfile->integer ) )
	{
		int         numAttribs;
		int         attribs[ 256 ];
		GLXFBConfig *FBConfig;

		// get FBConfig XID
		memset( attribs, 0, sizeof( attribs ) );
		numAttribs = 0;

		attribs[ numAttribs++ ] = GLX_FBCONFIG_ID;
		glXQueryContext( opengl_context.dpy, opengl_context.ctx,
		                 GLX_FBCONFIG_ID, &attribs[ numAttribs++ ] );
		FBConfig = glXChooseFBConfig( opengl_context.dpy, 0,
		                              attribs, &numAttribs );

		if ( numAttribs == 0 )
		{
			ri.Printf( PRINT_WARNING, "Could not get FBConfig for XID %d\n", attribs[ 1 ] );
		}

		memset( attribs, 0, sizeof( attribs ) );
		numAttribs = 0;

		if ( r_glMajorVersion->integer > 0 )
		{
			attribs[ numAttribs++ ] = GLX_CONTEXT_MAJOR_VERSION_ARB;
			attribs[ numAttribs++ ] = r_glMajorVersion->integer;

			attribs[ numAttribs++ ] = GLX_CONTEXT_MINOR_VERSION_ARB;
			attribs[ numAttribs++ ] = r_glMinorVersion->integer;
		}

		attribs[ numAttribs++ ] = GLX_CONTEXT_FLAGS_ARB;

		if ( r_glDebugProfile->integer )
		{
			attribs[ numAttribs++ ] = GLX_CONTEXT_DEBUG_BIT_ARB;
		}
		else
		{
			attribs[ numAttribs++ ] = 0;
		}

		attribs[ numAttribs++ ] = GLX_CONTEXT_PROFILE_MASK_ARB;

		if ( r_glCoreProfile->integer )
		{
			attribs[ numAttribs++ ] = GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
		}
		else
		{
			attribs[ numAttribs++ ] = GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
		}

		// set current context to NULL
		retVal = glXMakeCurrent( opengl_context.dpy, None, NULL ) != 0;
		ri.Printf( PRINT_ALL, "...glXMakeCurrent( %p, %p ): %s\n", opengl_context.dpy, NULL, success[ retVal ] );

		// delete dpy
		if ( opengl_context.ctx )
		{
			glXDestroyContext( opengl_context.dpy, opengl_context.ctx );
			retVal = ( glGetError() == 0 );
			ri.Printf( PRINT_ALL, "...deleting initial GL context: %s\n", success[ retVal ] );
			opengl_context.ctx = NULL;
		}

		ri.Printf( PRINT_ALL, "...initializing new OpenGL context" );

		opengl_context.ctx = glXCreateContextAttribsARB( opengl_context.dpy,
		                     FBConfig[ 0 ], NULL, GL_TRUE, attribs );

		if ( glXMakeCurrent( opengl_context.dpy, opengl_context.drawable, opengl_context.ctx ) )
		{
			ri.Printf( PRINT_ALL, " done\n" );
		}
		else
		{
			ri.Printf( PRINT_WARNING, " Could not initialize requested OpenGL profile\n" );
		}
	}

#endif

	if ( GLmajor < 2 )
	{
		// missing shader support, switch to 1.x renderer
		return qfalse;
	}

	if ( GLmajor < 3 || ( GLmajor == 3 && GLminor < 2 ) )
	{
		// shaders are supported, but not all GL3.x features
		ri.Printf( PRINT_ALL, "Using enhanced (GL3) Renderer in GL 2.x mode...\n" );
		return qtrue;
	}

	ri.Printf( PRINT_ALL, "Using enhanced (GL3) Renderer in GL 3.x mode...\n" );
	glConfig.driverType = GLDRV_OPENGL3;

	return qtrue;
}

#endif

/*
===============
GLimp_SetMode
===============
*/
static int GLimp_SetMode( int mode, qboolean fullscreen, qboolean noborder )
{
	const char  *glstring;
	int         sdlcolorbits;
	int         colorbits, alphabits, depthbits, stencilbits;
	int         tcolorbits, tdepthbits, tstencilbits;
	int         samples;
	int         i = 0;
	SDL_Surface *vidscreen = NULL;
	Uint32      flags = SDL_OPENGL;
	GLenum      glewResult;

	ri.Printf( PRINT_ALL, "Initializing OpenGL display\n" );

	if ( r_allowResize->integer )
	{
		flags |= SDL_RESIZABLE;
	}

	if ( videoInfo == NULL )
	{
		static SDL_VideoInfo   sVideoInfo;
		static SDL_PixelFormat sPixelFormat;

		videoInfo = SDL_GetVideoInfo();

		// Take a copy of the videoInfo
		Com_Memcpy( &sPixelFormat, videoInfo->vfmt, sizeof( SDL_PixelFormat ) );
		sPixelFormat.palette = NULL; // Should already be the case
		Com_Memcpy( &sVideoInfo, videoInfo, sizeof( SDL_VideoInfo ) );
		sVideoInfo.vfmt = &sPixelFormat;
		videoInfo = &sVideoInfo;

		if ( videoInfo->current_h > 0 )
		{
			// Guess the display aspect ratio through the desktop resolution
			// by assuming (relatively safely) that it is set at or close to
			// the display's native aspect ratio
			displayAspect = ( float ) videoInfo->current_w / ( float ) videoInfo->current_h;

			ri.Printf( PRINT_ALL, "Estimated display aspect: %.3f\n", displayAspect );
		}
		else
		{
			ri.Printf( PRINT_ALL, "Cannot estimate display aspect, assuming 1.333\n" );
		}
	}
	if ( videoInfo->current_h > 0 )
	{
		glConfig.vidWidth = videoInfo->current_w;
		glConfig.vidHeight = videoInfo->current_h;
	}
	else
	{
		glConfig.vidWidth = 480;
		glConfig.vidHeight = 640;
		ri.Printf( PRINT_ALL, "Cannot determine display resolution, assuming 640x480\n" );
	}

	ri.Printf( PRINT_ALL, "...setting mode %d:", mode );

	if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) )
	{
		ri.Printf( PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}

	ri.Printf( PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight );

retry:
	// we come back here if we couldn't get a visual and there's
	// something we can switch off

	if ( fullscreen )
	{
		flags |= SDL_FULLSCREEN;
		glConfig.isFullscreen = qtrue;
	}
	else
	{
		if ( noborder )
		{
			flags |= SDL_NOFRAME;
		}

		glConfig.isFullscreen = qfalse;
	}

	colorbits = r_colorbits->integer;

	if ( ( !colorbits ) || ( colorbits >= 32 ) )
	{
		colorbits = 24;
	}

	alphabits = r_alphabits->integer;

	if ( alphabits < 0 )
	{
		alphabits = 0;
	}

	if ( !r_depthbits->integer )
	{
		depthbits = 24;
	}
	else
	{
		depthbits = r_depthbits->integer;
	}

	stencilbits = r_stencilbits->integer;
	samples = r_ext_multisample->integer;

	for ( i = 0; i < 16; i++ )
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if ( ( i % 4 ) == 0 && i )
		{
			// one pass, reduce
			switch ( i / 4 )
			{
				case 2:
					if ( colorbits == 24 )
					{
						colorbits = 16;
					}

					break;

				case 1:
					if ( depthbits == 24 )
					{
						depthbits = 16;
					}
					else if ( depthbits == 16 )
					{
						depthbits = 8;
					}

				case 3:
					if ( stencilbits == 24 )
					{
						stencilbits = 16;
					}
					else if ( stencilbits == 16 )
					{
						stencilbits = 8;
					}
			}
		}

		tcolorbits = colorbits;
		tdepthbits = depthbits;
		tstencilbits = stencilbits;

		if ( ( i % 4 ) == 3 )
		{
			// reduce colorbits
			if ( tcolorbits == 24 )
			{
				tcolorbits = 16;
			}
		}

		if ( ( i % 4 ) == 2 )
		{
			// reduce depthbits
			if ( tdepthbits == 24 )
			{
				tdepthbits = 16;
			}
			else if ( tdepthbits == 16 )
			{
				tdepthbits = 8;
			}
		}

		if ( ( i % 4 ) == 1 )
		{
			// reduce stencilbits
			if ( tstencilbits == 24 )
			{
				tstencilbits = 16;
			}
			else if ( tstencilbits == 16 )
			{
				tstencilbits = 8;
			}
			else
			{
				tstencilbits = 0;
			}
		}

		sdlcolorbits = 4;

		if ( tcolorbits == 24 )
		{
			sdlcolorbits = 8;
		}

		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, sdlcolorbits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, sdlcolorbits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, sdlcolorbits );
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, alphabits );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, tdepthbits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, tstencilbits );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, samples );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

		if ( SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, r_swapInterval->integer ) < 0 )
		{
			ri.Printf( PRINT_ALL, "r_swapInterval requires libSDL >= 1.2.10\n" );
		}

//#ifdef USE_ICON
		{
			SDL_Surface *icon = SDL_CreateRGBSurfaceFrom( ( void * ) CLIENT_WINDOW_ICON.pixel_data,
			                    CLIENT_WINDOW_ICON.width,
			                    CLIENT_WINDOW_ICON.height,
			                    CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
			                    CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
			                    0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
			                    0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
			                                            );

			SDL_WM_SetIcon( icon, NULL );
			SDL_FreeSurface( icon );
		}
//#endif

		SDL_WM_SetCaption( CLIENT_WINDOW_TITLE, CLIENT_WINDOW_MIN_TITLE );
		SDL_ShowCursor( 0 );

		if ( !( vidscreen = SDL_SetVideoMode( glConfig.vidWidth, glConfig.vidHeight, colorbits, flags ) ) )
		{
			ri.Printf( PRINT_DEVELOPER, "SDL_SetVideoMode failed: %s\n", SDL_GetError() );
			continue;
		}

		ri.Printf( PRINT_ALL, "Using %d/%d/%d Color bits, %d depth, %d stencil display.\n",
		           sdlcolorbits, sdlcolorbits, sdlcolorbits, tdepthbits, tstencilbits );

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		break;
	}

	if ( !vidscreen && r_ext_multisample->integer )
	{
		ri.Printf( PRINT_ALL, "Multisample is not supported by the video driver and/or hardware.\n");
		r_ext_multisample->integer = 0;
		goto retry;
	}

	glewResult = glewInit();

	if ( GLEW_OK != glewResult )
	{
		// glewInit failed, something is seriously wrong
		ri.Error( ERR_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem: %s", glewGetErrorString( glewResult ) );
	}
	else
	{
		ri.Printf( PRINT_ALL, "Using GLEW %s\n", glewGetString( GLEW_VERSION ) );
	}

#if defined( USE_XREAL_RENDERER )

	if ( !GLimp_InitOpenGL3xContext() )
	{
		return RSERR_OLD_GL;
	}

#endif

	GLimp_DetectAvailableModes();

	if ( !vidscreen )
	{
		ri.Printf( PRINT_ALL, "Couldn't get a visual\n" );
		return RSERR_INVALID_MODE;
	}

	screen = vidscreen;

	glstring = ( char * ) glGetString( GL_RENDERER );
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glstring );

	return RSERR_OK;
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static qboolean GLimp_StartDriverAndSetMode( int mode, qboolean fullscreen, qboolean noborder )
{
	rserr_t err;

	if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		char driverName[ 64 ];

		ri.Printf( PRINT_ALL, "SDL_Init( SDL_INIT_VIDEO )... " );

		if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE ) == -1 )
		{
			ri.Printf( PRINT_ALL, "SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) FAILED (%s)\n", SDL_GetError() );
			return qfalse;
		}

		SDL_VideoDriverName( driverName, sizeof( driverName ) - 1 );
		ri.Printf( PRINT_ALL, "SDL using driver \"%s\"\n", driverName );
		ri.Cvar_Set( "r_sdlDriver", driverName );
	}

	if ( fullscreen && ri.Cvar_VariableIntegerValue( "in_nograb" ) )
	{
		ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n" );
		ri.Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;
	}

	err = GLimp_SetMode( mode, fullscreen, noborder );

	switch ( err )
	{
		case RSERR_INVALID_FULLSCREEN:
			ri.Printf( PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n" );
			return qfalse;

		case RSERR_INVALID_MODE:
			ri.Printf( PRINT_ALL, "...WARNING: could not set the given mode (%d)\n", mode );
			return qfalse;

		case RSERR_OLD_GL:
			ri.Printf( PRINT_ALL, "...WARNING: OpenGL too old\n" );
			return qfalse;

		default:
			break;
	}

	return qtrue;
}

/*
===============
GLimp_XreaLInitExtensions
===============
*/
#if defined USE_XREAL_RENDERER
static void GLimp_XreaLInitExtensions( void )
{
	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	// GL_ARB_multitexture
	if ( glConfig.driverType != GLDRV_OPENGL3 )
	{
		if ( GLEW_ARB_multitexture )
		{
			glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glConfig.maxActiveTextures );

			if ( glConfig.maxActiveTextures > 1 )
			{
				ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
			}
			else
			{
				ri.Error( ERR_FATAL, "...not using GL_ARB_multitexture, < 2 texture units" );
			}
		}
		else
		{
			ri.Error( ERR_FATAL, "...GL_ARB_multitexture not found" );
		}
	}

	// GL_ARB_depth_texture
	if ( GLEW_ARB_depth_texture )
	{
		ri.Printf( PRINT_ALL, "...using GL_ARB_depth_texture\n" );
	}
	else
	{
		ri.Error( ERR_FATAL, "...GL_ARB_depth_texture not found" );
	}

	// GL_ARB_texture_cube_map
	if ( GLEW_ARB_texture_cube_map )
	{
		glGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig2.maxCubeMapTextureSize );
		ri.Printf( PRINT_ALL, "...using GL_ARB_texture_cube_map\n" );
	}
	else
	{
		ri.Error( ERR_FATAL, "...GL_ARB_texture_cube_map not found" );
	}

	GL_CheckErrors();

	// GL_ARB_vertex_program
	if ( GLEW_ARB_vertex_program )
	{
		ri.Printf( PRINT_ALL, "...using GL_ARB_vertex_program\n" );
	}
	else
	{
		ri.Error( ERR_FATAL, "...GL_ARB_vertex_program not found" );
	}

	// GL_ARB_vertex_buffer_object
	if ( GLEW_ARB_vertex_buffer_object )
	{
		ri.Printf( PRINT_ALL, "...using GL_ARB_vertex_buffer_object\n" );
	}
	else
	{
		ri.Error( ERR_FATAL, "...GL_ARB_vertex_buffer_object not found" );
	}

	// GL_ARB_occlusion_query
	glConfig2.occlusionQueryAvailable = qfalse;
	glConfig2.occlusionQueryBits = 0;

	if ( GLEW_ARB_occlusion_query )
	{
		if ( r_ext_occlusion_query->value )
		{
			glConfig2.occlusionQueryAvailable = qtrue;
			glGetQueryivARB( GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &glConfig2.occlusionQueryBits );
			ri.Printf( PRINT_ALL, "...using GL_ARB_occlusion_query\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_occlusion_query\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_occlusion_query not found\n" );
	}

	GL_CheckErrors();

	// GL_ARB_shader_objects
	if ( GLEW_ARB_shader_objects )
	{
		ri.Printf( PRINT_ALL, "...using GL_ARB_shader_objects\n" );
	}
	else
	{
		ri.Error( ERR_FATAL, "...GL_ARB_shader_objects not found" );
	}

	// GL_ARB_vertex_shader
	if ( GLEW_ARB_vertex_shader )
	{
		int reservedComponents;

		GL_CheckErrors();
		glGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &glConfig2.maxVertexUniforms );
		GL_CheckErrors();
		//glGetIntegerv(GL_MAX_VARYING_FLOATS_ARB, &glConfig.maxVaryingFloats); GL_CheckErrors();
		glGetIntegerv( GL_MAX_VERTEX_ATTRIBS_ARB, &glConfig2.maxVertexAttribs );
		GL_CheckErrors();

		reservedComponents = 16 * 10; // approximation how many uniforms we have besides the bone matrices

		/*
		if(glConfig.driverType == GLDRV_MESA)
		{
		        // HACK
		        // restrict to number of vertex uniforms to 512 because of:
		        // xreal.x86_64: nv50_program.c:4181: nv50_program_validate_data: Assertion `p->param_nr <= 512' failed

		        glConfig2.maxVertexUniforms = Q_bound(0, glConfig2.maxVertexUniforms, 512);
		}
		*/

		glConfig2.maxVertexSkinningBones = MIN( MAX( 0, glConfig2.maxVertexUniforms - reservedComponents ) / 16, MAX_BONES );
		glConfig2.vboVertexSkinningAvailable = r_vboVertexSkinning->integer && ( ( glConfig2.maxVertexSkinningBones >= 12 ) ? qtrue : qfalse );

		ri.Printf( PRINT_ALL, "...using GL_ARB_vertex_shader\n" );
	}
	else
	{
		ri.Error( ERR_FATAL, "...GL_ARB_vertex_shader not found" );
	}

	GL_CheckErrors();

	// GL_ARB_fragment_shader
	if ( GLEW_ARB_fragment_shader )
	{
		ri.Printf( PRINT_ALL, "...using GL_ARB_fragment_shader\n" );
	}
	else
	{
		ri.Error( ERR_FATAL, "...GL_ARB_fragment_shader not found" );
	}

	// GL_ARB_shading_language_100
	if ( GLEW_ARB_shading_language_100 )
	{
		Q_strncpyz( glConfig2.shadingLanguageVersion, ( char * ) glGetString( GL_SHADING_LANGUAGE_VERSION_ARB ),
		            sizeof( glConfig2.shadingLanguageVersion ) );
		ri.Printf( PRINT_ALL, "...using GL_ARB_shading_language_100\n" );
	}
	else
	{
		ri.Printf( ERR_FATAL, "...GL_ARB_shading_language_100 not found\n" );
	}

	GL_CheckErrors();

	// GL_ARB_texture_non_power_of_two
	glConfig2.textureNPOTAvailable = qfalse;

	if ( GLEW_ARB_texture_non_power_of_two )
	{
		if ( r_ext_texture_non_power_of_two->integer )
		{
			glConfig2.textureNPOTAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_ARB_texture_non_power_of_two\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_texture_non_power_of_two\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_texture_non_power_of_two not found\n" );
	}

	// GL_ARB_draw_buffers
	glConfig2.drawBuffersAvailable = qfalse;

	if ( GLEW_ARB_draw_buffers )
	{
		glGetIntegerv( GL_MAX_DRAW_BUFFERS_ARB, &glConfig2.maxDrawBuffers );

		if ( r_ext_draw_buffers->integer )
		{
			glConfig2.drawBuffersAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_ARB_draw_buffers\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_draw_buffers\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_draw_buffers not found\n" );
	}

	// GL_ARB_half_float_pixel
	glConfig2.textureHalfFloatAvailable = qfalse;

	if ( GLEW_ARB_half_float_pixel )
	{
		if ( r_ext_half_float_pixel->integer )
		{
			glConfig2.textureHalfFloatAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_ARB_half_float_pixel\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_half_float_pixel\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_half_float_pixel not found\n" );
	}

	// GL_ARB_texture_float
	glConfig2.textureFloatAvailable = qfalse;

	if ( GLEW_ARB_texture_float )
	{
		if ( r_ext_texture_float->integer )
		{
			glConfig2.textureFloatAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_ARB_texture_float\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_texture_float\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_texture_float not found\n" );
	}

	// GL_ARB_texture_compression
	glConfig.textureCompression = TC_NONE;

	if ( GLEW_ARB_texture_compression )
	{
		if ( r_ext_compressed_textures->integer )
		{
			glConfig2.ARBTextureCompressionAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_ARB_texture_compression\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_texture_compression\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_texture_compression not found\n" );
	}

	// GL_ARB_vertex_array_object
	glConfig2.vertexArrayObjectAvailable = qfalse;

	if ( GLEW_ARB_vertex_array_object )
	{
		if ( r_ext_vertex_array_object->integer )
		{
			glConfig2.vertexArrayObjectAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_ARB_vertex_array_object\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_vertex_array_object\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_vertex_array_object not found\n" );
	}

	// GL_EXT_texture_compression_s3tc
	if ( GLEW_EXT_texture_compression_s3tc )
	{
		if ( r_ext_compressed_textures->integer )
		{
			glConfig.textureCompression = TC_S3TC;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_compression_s3tc\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_compression_s3tc not found\n" );
	}

	// GL_EXT_texture3D
	glConfig2.texture3DAvailable = qfalse;

	if ( GLEW_EXT_texture3D )
	{
		//if(r_ext_texture3d->value)
		{
			glConfig2.texture3DAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture3D\n" );
		}

		/*
		else
		{
		        ri.Printf(PRINT_ALL, "...ignoring GL_EXT_texture3D\n");
		}
		*/
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture3D not found\n" );
	}

	// GL_EXT_stencil_wrap
	glConfig2.stencilWrapAvailable = qfalse;

	if ( GLEW_EXT_stencil_wrap )
	{
		if ( r_ext_stencil_wrap->value )
		{
			glConfig2.stencilWrapAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_stencil_wrap\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_stencil_wrap\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_stencil_wrap not found\n" );
	}

	// GL_EXT_texture_filter_anisotropic
	glConfig2.textureAnisotropyAvailable = qfalse;

	if ( GLEW_EXT_texture_filter_anisotropic )
	{
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig2.maxTextureAnisotropy );

		if ( r_ext_texture_filter_anisotropic->value )
		{
			glConfig2.textureAnisotropyAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n" );
	}

	GL_CheckErrors();

	// GL_EXT_stencil_two_side
	if ( GLEW_EXT_stencil_two_side )
	{
		if ( r_ext_stencil_two_side->value )
		{
			ri.Printf( PRINT_ALL, "...using GL_EXT_stencil_two_side\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_stencil_two_side\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_stencil_two_side not found\n" );
	}

	// GL_EXT_depth_bounds_test
	if ( GLEW_EXT_depth_bounds_test )
	{
		if ( r_ext_depth_bounds_test->value )
		{
			ri.Printf( PRINT_ALL, "...using GL_EXT_depth_bounds_test\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_depth_bounds_test\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_depth_bounds_test not found\n" );
	}

	// GL_EXT_framebuffer_object
	glConfig2.framebufferObjectAvailable = qfalse;

	if ( GLEW_EXT_framebuffer_object )
	{
		glGetIntegerv( GL_MAX_RENDERBUFFER_SIZE_EXT, &glConfig2.maxRenderbufferSize );
		glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS_EXT, &glConfig2.maxColorAttachments );

		if ( r_ext_framebuffer_object->value )
		{
			glConfig2.framebufferObjectAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_framebuffer_object\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_framebuffer_object\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_framebuffer_object not found\n" );
	}

	GL_CheckErrors();

	// GL_EXT_packed_depth_stencil
	glConfig2.framebufferPackedDepthStencilAvailable = qfalse;

	if ( GLEW_EXT_packed_depth_stencil && glConfig.driverType != GLDRV_MESA )
	{
		if ( r_ext_packed_depth_stencil->integer )
		{
			glConfig2.framebufferPackedDepthStencilAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_packed_depth_stencil\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_packed_depth_stencil\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_packed_depth_stencil not found\n" );
	}

	// GL_EXT_framebuffer_blit
	glConfig2.framebufferBlitAvailable = qfalse;

	if ( GLEW_EXT_framebuffer_blit )
	{
		if ( r_ext_framebuffer_blit->integer )
		{
			glConfig2.framebufferBlitAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_framebuffer_blit\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_framebuffer_blit\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_framebuffer_blit not found\n" );
	}

	// GL_EXTX_framebuffer_mixed_formats

	/*
	glConfig.framebufferMixedFormatsAvailable = qfalse;
	if(GLEW_EXTX_framebuffer_mixed_formats)
	{
	        if(r_extx_framebuffer_mixed_formats->integer)
	        {
	                glConfig.framebufferMixedFormatsAvailable = qtrue;
	                ri.Printf(PRINT_ALL, "...using GL_EXTX_framebuffer_mixed_formats\n");
	        }
	        else
	        {
	                ri.Printf(PRINT_ALL, "...ignoring GL_EXTX_framebuffer_mixed_formats\n");
	        }
	}
	else
	{
	        ri.Printf(PRINT_ALL, "...GL_EXTX_framebuffer_mixed_formats not found\n");
	}
	*/

	// GL_ATI_separate_stencil
	if ( GLEW_ATI_separate_stencil )
	{
		if ( r_ext_separate_stencil->value )
		{
			ri.Printf( PRINT_ALL, "...using GL_ATI_separate_stencil\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ATI_separate_stencil\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ATI_separate_stencil not found\n" );
	}

	// GL_SGIS_generate_mipmap
	glConfig2.generateMipmapAvailable = qfalse;

	if ( GLEW_SGIS_generate_mipmap )
	{
		if ( r_ext_generate_mipmap->value )
		{
			glConfig2.generateMipmapAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_SGIS_generate_mipmap\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_SGIS_generate_mipmap\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_SGIS_generate_mipmap not found\n" );
	}

	// GL_GREMEDY_string_marker
	if ( GLEW_GREMEDY_string_marker )
	{
		ri.Printf( PRINT_ALL, "...using GL_GREMEDY_string_marker\n" );
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_GREMEDY_string_marker not found\n" );
	}


#ifdef GLEW_ARB_get_program_binary
	if( GLEW_ARB_get_program_binary )
	{
		ri.Printf( PRINT_ALL, "...using GL_ARB_get_program_binary\n");
		glConfig2.getProgramBinaryAvailable = qtrue;
	} 
	else
#endif
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_get_program_binary not found\n");
		glConfig2.getProgramBinaryAvailable = qfalse;
	}
	
}

#endif

/*
===============
GLimp_InitExtensions
===============
*/
#if !defined( USE_XREAL_RENDERER )
static void GLimp_InitExtensions( void )
{
	if ( !r_allowExtensions->integer )
	{
		ri.Printf( PRINT_ALL, "* IGNORING OPENGL EXTENSIONS *\n" );
		return;
	}

	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	glConfig.textureCompression = TC_NONE;

	// GL_EXT_texture_compression_s3tc
	if ( GLEW_ARB_texture_compression &&
	     GLEW_EXT_texture_compression_s3tc )
	{
		if ( r_ext_compressed_textures->value )
		{
			glConfig.textureCompression = TC_EXT_COMP_S3TC;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_compression_s3tc\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_compression_s3tc not found\n" );
	}

	// GL_S3_s3tc ... legacy extension before GL_EXT_texture_compression_s3tc.
	if ( glConfig.textureCompression == TC_NONE )
	{
		if ( GLEW_S3_s3tc )
		{
			if ( r_ext_compressed_textures->value )
			{
				glConfig.textureCompression = TC_S3TC;
				ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
			}
			else
			{
				ri.Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...GL_S3_s3tc not found\n" );
		}
	}

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;

	if ( GLEW_EXT_texture_env_add )
	{
		if ( r_ext_texture_env_add->integer )
		{
			glConfig.textureEnvAddAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
		}
		else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}

	// GL_ARB_multitexture
	glConfig.maxActiveTextures = 1;

	if ( GLEW_ARB_multitexture )
	{
		if ( r_ext_multitexture->value )
		{
			GLint glint = 0;

			glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glint );

			glConfig.maxActiveTextures = ( int ) glint;

			if ( glConfig.maxActiveTextures > 1 )
			{
				ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
			}
			else
			{
				ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_multitexture\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );
	}
}

#endif

#define R_MODE_FALLBACK 3 // 640 * 480

/* Support code for GLimp_Init */

static void reportDriverType( qboolean force )
{
	static const char *const drivers[] = {
		"integrated", "stand-alone", "Voodoo", "OpenGL 3+", "Mesa"
	};
	if (glConfig.driverType > GLDRV_UNKNOWN && (int) glConfig.driverType < ARRAY_LEN( drivers ) )
	{
		ri.Printf( PRINT_ALL, "%s graphics driver class '%s'\n",
		           force ? "User has forced" : "Detected",
		           drivers[glConfig.driverType] );
	}
}

static void reportHardwareType( qboolean force )
{
	static const char *const hardware[] = {
		"generic", "Voodoo", "Riva 128", "Rage Pro", "Permedia 2",
		"ATI Radeon", "AMD Radeon DX10-class", "nVidia DX10-class"
	};
	if (glConfig.hardwareType > GLHW_UNKNOWN && (int) glConfig.hardwareType < ARRAY_LEN( hardware ) )
	{
		ri.Printf( PRINT_ALL, "%s graphics hardware class '%s'\n",
		           force ? "User has forced" : "Detected",
		           hardware[glConfig.hardwareType] );
	}
}

/*
===============
GLimp_Init

This routine is responsible for initializing the OS specific portions
of OpenGL
===============
*/
qboolean GLimp_Init( void )
{
	//qboolean        success = qtrue;

	glConfig.driverType = GLDRV_ICD;

	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "", CVAR_ROM );
	r_allowResize = ri.Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE );
	r_centerWindow = ri.Cvar_Get( "r_centerWindow", "0", CVAR_ARCHIVE );
	r_minimize = ri.Cvar_Get( "r_minimize", "0", CVAR_ARCHIVE );

	if ( ri.Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		ri.Cvar_Set( "r_mode", va( "%d", R_MODE_FALLBACK ) );
		ri.Cvar_Set( "r_fullscreen", "0" );
		ri.Cvar_Set( "r_centerWindow", "0" );
		ri.Cvar_Set( "com_abnormalExit", "0" );
	}

	//Sys_SetEnv("SDL_VIDEO_CENTERED", r_centerWindow->integer ? "1" : "");

	//ri.Sys_GLimpInit();

#if 0 //defined(WIN32)

	if ( !SDL_VIDEODRIVER_externallySet )
	{
		// It's a little bit weird having in_mouse control the
		// video driver, but from ioq3's point of view they're
		// virtually the same except for the mouse input anyway
		if ( ri.Cvar_VariableIntegerValue( "in_mouse" ) == -1 )
		{
			// Use the windib SDL backend, which is closest to
			// the behaviour of idq3 with in_mouse set to -1
			_putenv( "SDL_VIDEODRIVER=windib" );
		}
		else
		{
			// Use the DirectX SDL backend
			_putenv( "SDL_VIDEODRIVER=directx" );
		}
	}

#endif

	// Create the window and set up the context
	if ( GLimp_StartDriverAndSetMode( r_mode->integer, r_fullscreen->integer, qfalse ) )
	{
		goto success;
	}

	// Try again, this time in a platform specific "safe mode"
	ri.Sys_GLimpSafeInit();

#if 0 //defined(WIN32)

	if ( !SDL_VIDEODRIVER_externallySet )
	{
		// Here, we want to let SDL decide what do to unless
		// explicitly requested otherwise
		_putenv( "SDL_VIDEODRIVER=" );
	}

#endif

	if ( GLimp_StartDriverAndSetMode( r_mode->integer, r_fullscreen->integer, qfalse ) )
	{
		goto success;
	}

	// Finally, try the default screen resolution
	if ( r_mode->integer != R_MODE_FALLBACK )
	{
		ri.Printf( PRINT_ALL, "Setting r_mode %d failed, falling back on r_mode %d\n", r_mode->integer, R_MODE_FALLBACK );

		if ( GLimp_StartDriverAndSetMode( R_MODE_FALLBACK, qfalse, qfalse ) )
		{
			goto success;
		}
	}

	// Nothing worked, give up
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	return qfalse;

success:
	// This values force the UI to disable driver selection
	glConfig.hardwareType = GLHW_GENERIC;
	glConfig.deviceSupportsGamma = SDL_SetGamma( 1.0f, 1.0f, 1.0f ) >= 0;

	// Mysteriously, if you use an NVidia graphics card and multiple monitors,
	// SDL_SetGamma will incorrectly return false... the first time; ask
	// again and you get the correct answer. This is a suspected driver bug, see
	// http://bugzilla.icculus.org/show_bug.cgi?id=4316
	glConfig.deviceSupportsGamma = SDL_SetGamma( 1.0f, 1.0f, 1.0f ) >= 0;

	if ( r_ignorehwgamma->integer )
	{
		glConfig.deviceSupportsGamma = 0;
	}

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, ( char * ) glGetString( GL_VENDOR ), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, ( char * ) glGetString( GL_RENDERER ), sizeof( glConfig.renderer_string ) );

	if ( *glConfig.renderer_string && glConfig.renderer_string[ strlen( glConfig.renderer_string ) - 1 ] == '\n' )
	{
		glConfig.renderer_string[ strlen( glConfig.renderer_string ) - 1 ] = 0;
	}

	Q_strncpyz( glConfig.version_string, ( char * ) glGetString( GL_VERSION ), sizeof( glConfig.version_string ) );

	if ( glConfig.driverType == GLDRV_OPENGL3 )
	{
		GLint numExts, i;

		glGetIntegerv( GL_NUM_EXTENSIONS, &numExts );

		glConfig.extensions_string[ 0 ] = '\0';
		for ( i = 0; i < numExts; ++i )
			Q_strcat( glConfig.extensions_string, sizeof( glConfig.extensions_string ), ( char * ) glGetStringi( GL_EXTENSIONS, i ) );
	}
	else
	{
		Q_strncpyz( glConfig.extensions_string, ( char * ) glGetString( GL_EXTENSIONS ), sizeof( glConfig.extensions_string ) );
	}

	if ( Q_stristr( glConfig.renderer_string, "mesa" ) ||
	     Q_stristr( glConfig.renderer_string, "gallium" ) ||
	     Q_stristr( glConfig.vendor_string, "nouveau" ) ||
	     Q_stristr( glConfig.vendor_string, "mesa" ) )
	{
		// suckage
		glConfig.driverType = GLDRV_MESA;
	}

	if ( Q_stristr( glConfig.renderer_string, "geforce" ) )
	{
		if ( Q_stristr( glConfig.renderer_string, "8400" ) ||
		     Q_stristr( glConfig.renderer_string, "8500" ) ||
		     Q_stristr( glConfig.renderer_string, "8600" ) ||
		     Q_stristr( glConfig.renderer_string, "8800" ) ||
		     Q_stristr( glConfig.renderer_string, "9500" ) ||
		     Q_stristr( glConfig.renderer_string, "9600" ) ||
		     Q_stristr( glConfig.renderer_string, "9800" ) ||
		     Q_stristr( glConfig.renderer_string, "gts 240" ) ||
		     Q_stristr( glConfig.renderer_string, "gts 250" ) ||
		     Q_stristr( glConfig.renderer_string, "gtx 260" ) ||
		     Q_stristr( glConfig.renderer_string, "gtx 275" ) ||
		     Q_stristr( glConfig.renderer_string, "gtx 280" ) ||
		     Q_stristr( glConfig.renderer_string, "gtx 285" ) ||
		     Q_stristr( glConfig.renderer_string, "gtx 295" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 320" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 330" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 340" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 415" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 420" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 425" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 430" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 435" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 440" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 520" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 525" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 540" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 550" ) ||
		     Q_stristr( glConfig.renderer_string, "gt 555" ) ||
		     Q_stristr( glConfig.renderer_string, "gts 450" ) ||
		     Q_stristr( glConfig.renderer_string, "gtx 460" ) ||
		     Q_stristr( glConfig.renderer_string, "gtx 470" ) ||
		     Q_stristr( glConfig.renderer_string, "gtx 480" ) ||
		     Q_stristr( glConfig.renderer_string, "gtx 485" ) ||
		     Q_stristr( glConfig.renderer_string, "gtx 560" ) ||
		     Q_stristr( glConfig.renderer_string, "gtx 570" ) ||
		     Q_stristr( glConfig.renderer_string, "gtx 580" ) ||
		     Q_stristr( glConfig.renderer_string, "gtx 590" ) )
		{
			glConfig.hardwareType = GLHW_NV_DX10;
		}
	}
	else if ( Q_stristr( glConfig.renderer_string, "quadro fx" ) )
	{
		if ( Q_stristr( glConfig.renderer_string, "3600" ) )
		{
			glConfig.hardwareType = GLHW_NV_DX10;
		}
	}
	else if ( Q_stristr( glConfig.renderer_string, "gallium" ) &&
	          Q_stristr( glConfig.renderer_string, " amd " ) )
	{
		// anything prior to R600 is listed as ATI.
		glConfig.hardwareType = GLHW_ATI_DX10;
	}
	else if ( Q_stristr( glConfig.renderer_string, "rv770" ) ||
	          Q_stristr( glConfig.renderer_string, "eah4850" ) ||
	          Q_stristr( glConfig.renderer_string, "eah4870" ) ||
	          // previous three are too specific?
	          Q_stristr( glConfig.renderer_string, "radeon hd" ) )
	{
		glConfig.hardwareType = GLHW_ATI_DX10;
	}
	else if ( Q_stristr( glConfig.renderer_string, "radeon" ) )
	{
		glConfig.hardwareType = GLHW_ATI;
	}

	reportDriverType( qfalse );
	reportHardwareType( qfalse );

	{ // allow overriding where the user really does know better
		cvar_t          *forceGL;
		glDriverType_t   driverType   = GLDRV_UNKNOWN;
		glHardwareType_t hardwareType = GLHW_UNKNOWN;

		forceGL = ri.Cvar_Get( "r_glForceDriver", "", CVAR_LATCH );

		if      ( !Q_stricmp( forceGL->string, "icd" ))
		{
			driverType = GLDRV_ICD;
		}
		else if ( !Q_stricmp( forceGL->string, "standalone" ))
		{
			driverType = GLDRV_STANDALONE;
		}
		else if ( !Q_stricmp( forceGL->string, "voodoo" ))
		{
			driverType = GLDRV_VOODOO;
		}
		else if ( !Q_stricmp( forceGL->string, "opengl3" ))
		{
			driverType = GLDRV_OPENGL3;
		}
		else if ( !Q_stricmp( forceGL->string, "mesa" ))
		{
			driverType = GLDRV_MESA;
		}

		forceGL = ri.Cvar_Get( "r_glForceHardware", "", CVAR_LATCH );

		if      ( !Q_stricmp( forceGL->string, "generic" ))
		{
			hardwareType = GLHW_GENERIC;
		}
		else if ( !Q_stricmp( forceGL->string, "voodoo" ))
		{
			hardwareType = GLHW_3DFX_2D3D;
		}
		else if ( !Q_stricmp( forceGL->string, "riva128" ))
		{
			hardwareType = GLHW_RIVA128;
		}
		else if ( !Q_stricmp( forceGL->string, "ragepro" ))
		{
			hardwareType = GLHW_RAGEPRO;
		}
		else if ( !Q_stricmp( forceGL->string, "permedia2" ))
		{
			hardwareType = GLHW_PERMEDIA2;
		}
		else if ( !Q_stricmp( forceGL->string, "ati" ))
		{
			hardwareType = GLHW_ATI;
		}
		else if ( !Q_stricmp( forceGL->string, "atidx10" ) ||
		          !Q_stricmp( forceGL->string, "radeonhd" ))
		{
			hardwareType = GLHW_ATI_DX10;
		}
		else if ( !Q_stricmp( forceGL->string, "nvdx10" ))
		{
			hardwareType = GLHW_NV_DX10;
		}

		if ( driverType != GLDRV_UNKNOWN )
		{
			glConfig.driverType = driverType;
			reportDriverType( qtrue );
		}

		if ( hardwareType != GLHW_UNKNOWN )
		{
			glConfig.hardwareType = hardwareType;
			reportHardwareType( qtrue );
		}
	}

	// initialize extensions
#if defined USE_XREAL_RENDERER
	GLimp_XreaLInitExtensions();
#endif
#if !defined USE_XREAL_RENDERER
	GLimp_InitExtensions();
#endif

	ri.Cvar_Get( "r_availableModes", "", CVAR_ROM );

	// This depends on SDL_INIT_VIDEO, hence having it here
	ri.IN_Init();

	return qtrue;
}

void GLimp_ReleaseGL( void )
{
}

/*
===============
GLimp_EndFrame

Responsible for doing a swapbuffers
===============
*/
void GLimp_EndFrame( void )
{
	// don't flip if drawing to front buffer
	if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		SDL_GL_SwapBuffers();
	}

	if ( r_minimize && r_minimize->integer )
	{
#ifdef MACOS_X
		SDL_Surface *s = SDL_GetVideoSurface();
		qboolean    fullscreen = ( s && ( s->flags & SDL_FULLSCREEN ) );

		if ( !fullscreen )
		{
			SDL_WM_IconifyWindow();
			ri.Cvar_Set( "r_minimize", "0" );
		}
		else if ( r_fullscreen->integer )
		{
			ri.Cvar_Set( "r_fullscreen", "0" );
		}
#else
		SDL_WM_IconifyWindow();
		ri.Cvar_Set( "r_minimize", "0" );
#endif
	}

	if ( r_fullscreen->modified )
	{
		qboolean    fullscreen;
		qboolean    needToToggle = qtrue;
		qboolean    sdlToggled = qfalse;
		SDL_Surface *s = SDL_GetVideoSurface();

		if ( s )
		{
			// Find out the current state
			fullscreen = !!( s->flags & SDL_FULLSCREEN );

			if ( r_fullscreen->integer && ri.Cvar_VariableIntegerValue( "in_nograb" ) )
			{
				ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n" );
				ri.Cvar_Set( "r_fullscreen", "0" );
				r_fullscreen->modified = qfalse;
			}

			// Is the state we want different from the current state?
			needToToggle = !!r_fullscreen->integer != fullscreen;

			if ( needToToggle )
			{
				sdlToggled = SDL_WM_ToggleFullScreen( s );
			}
		}

		if ( needToToggle )
		{
			// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
			if ( !sdlToggled )
			{
				ri.Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
			}

			ri.IN_Restart();
		}

		r_fullscreen->modified = qfalse;
	}
}

void GLimp_AcquireGL( void )
{
}

void GLimp_LogComment( const char *comment )
{
	static char buf[ 4096 ];

	if ( r_logFile->integer && GLEW_GREMEDY_string_marker )
	{
		// copy string and ensure it has a trailing '\0'
		Q_strncpyz( buf, comment, sizeof( buf ) );

		glStringMarkerGREMEDY( strlen( buf ), buf );
	}
}
