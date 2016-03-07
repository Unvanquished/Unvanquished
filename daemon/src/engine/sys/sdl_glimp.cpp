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

#ifdef SMP
#include <SDL_thread.h>
#endif

#include "renderer/tr_local.h"

#include "sdl_icon.h"
#include "SDL_syswm.h"
#include "framework/CommandSystem.h"

SDL_Window         *window = nullptr;
static SDL_GLContext glContext = nullptr;
static int colorBits = 0;

#ifdef SMP
static void GLimp_SetCurrentContext( bool enable )
{
	if ( enable )
	{
		SDL_GL_MakeCurrent( window, glContext );
	}
	else
	{
		SDL_GL_MakeCurrent( window, nullptr );
	}
}

/*
===========================================================

SMP acceleration

===========================================================
*/

/*
 * I have no idea if this will even work...most platforms don't offer
 * thread-safe OpenGL libraries.
 */

static SDL_mutex  *smpMutex = nullptr;
static SDL_cond   *renderCommandsEvent = nullptr;
static SDL_cond   *renderCompletedEvent = nullptr;
static void ( *renderThreadFunction )() = nullptr;
static SDL_Thread *renderThread = nullptr;

/*
===============
GLimp_RenderThreadWrapper
===============
*/
static int GLimp_RenderThreadWrapper( void* )
{
	// These printfs cause race conditions which mess up the console output
	Log::Notice( "Render thread starting\n" );

	renderThreadFunction();

	GLimp_SetCurrentContext( false );

	Log::Notice( "Render thread terminating\n" );

	return 0;
}

/*
===============
GLimp_SpawnRenderThread
===============
*/
bool GLimp_SpawnRenderThread( void ( *function )() )
{
	static bool warned = false;

	if ( !warned )
	{
		Log::Warn( "You enable r_smp at your own risk!\n" );
		warned = true;
	}

	if ( renderThread != nullptr ) /* hopefully just a zombie at this point... */
	{
		Log::Notice( "Already a render thread? Trying to clean it up...\n" );
		GLimp_ShutdownRenderThread();
	}

	smpMutex = SDL_CreateMutex();

	if ( smpMutex == nullptr )
	{
		Log::Notice( "smpMutex creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return false;
	}

	renderCommandsEvent = SDL_CreateCond();

	if ( renderCommandsEvent == nullptr )
	{
		Log::Notice( "renderCommandsEvent creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return false;
	}

	renderCompletedEvent = SDL_CreateCond();

	if ( renderCompletedEvent == nullptr )
	{
		Log::Notice( "renderCompletedEvent creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return false;
	}

	renderThreadFunction = function;
	renderThread = SDL_CreateThread( GLimp_RenderThreadWrapper, "render thread", nullptr );

	if ( renderThread == nullptr )
	{
		Log::Notice("SDL_CreateThread() returned %s", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return false;
	}

	return true;
}

/*
===============
GLimp_ShutdownRenderThread
===============
*/
void GLimp_ShutdownRenderThread()
{
	if ( renderThread != nullptr )
	{
		GLimp_WakeRenderer( nullptr );
		SDL_WaitThread( renderThread, nullptr );
		renderThread = nullptr;
		glConfig.smpActive = false;
	}

	if ( smpMutex != nullptr )
	{
		SDL_DestroyMutex( smpMutex );
		smpMutex = nullptr;
	}

	if ( renderCommandsEvent != nullptr )
	{
		SDL_DestroyCond( renderCommandsEvent );
		renderCommandsEvent = nullptr;
	}

	if ( renderCompletedEvent != nullptr )
	{
		SDL_DestroyCond( renderCompletedEvent );
		renderCompletedEvent = nullptr;
	}

	renderThreadFunction = nullptr;
}

static volatile void     *smpData = nullptr;
static volatile bool smpDataReady;

/*
===============
GLimp_RendererSleep
===============
*/
void           *GLimp_RendererSleep()
{
	void *data = nullptr;

	GLimp_SetCurrentContext( false );

	SDL_LockMutex( smpMutex );
	{
		smpData = nullptr;
		smpDataReady = false;

		// after this, the front end can exit GLimp_FrontEndSleep
		SDL_CondSignal( renderCompletedEvent );

		while ( !smpDataReady )
		{
			SDL_CondWait( renderCommandsEvent, smpMutex );
		}

		data = ( void * ) smpData;
	}
	SDL_UnlockMutex( smpMutex );

	GLimp_SetCurrentContext( true );

	return data;
}

/*
===============
GLimp_FrontEndSleep
===============
*/
void GLimp_FrontEndSleep()
{
	SDL_LockMutex( smpMutex );
	{
		while ( smpData )
		{
			SDL_CondWait( renderCompletedEvent, smpMutex );
		}
	}
	SDL_UnlockMutex( smpMutex );
}

/*
===============
GLimp_SyncRenderThread
===============
*/
void GLimp_SyncRenderThread()
{
	GLimp_FrontEndSleep();

	GLimp_SetCurrentContext( true );
}

/*
===============
GLimp_WakeRenderer
===============
*/
void GLimp_WakeRenderer( void *data )
{
	GLimp_SetCurrentContext( false );

	SDL_LockMutex( smpMutex );
	{
		ASSERT(smpData == nullptr);
		smpData = data;
		smpDataReady = true;

		// after this, the renderer can continue through GLimp_RendererSleep
		SDL_CondSignal( renderCommandsEvent );
	}
	SDL_UnlockMutex( smpMutex );
}

#else

// No SMP - stubs
void GLimp_RenderThreadWrapper( void* )
{
}

bool GLimp_SpawnRenderThread( void ( * )() )
{
	Log::Warn("SMP support was disabled at compile time" );
	return false;
}

void GLimp_ShutdownRenderThread()
{
}

void *GLimp_RendererSleep()
{
	return nullptr;
}

void GLimp_FrontEndSleep()
{
}

void GLimp_SyncRenderThread()
{
}

void GLimp_WakeRenderer( void* )
{
}

#endif

enum class rserr_t
{
  RSERR_OK,

  RSERR_INVALID_FULLSCREEN,
  RSERR_INVALID_MODE,
  RSERR_OLD_GL,

  RSERR_UNKNOWN
};

cvar_t                     *r_allowResize; // make window resizable
cvar_t                     *r_centerWindow;
cvar_t                     *r_displayIndex;
cvar_t                     *r_sdlDriver;

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown()
{
	Log::Debug("Shutting down OpenGL subsystem" );

	ri.IN_Shutdown();

#if defined( SMP )

	if ( renderThread != nullptr )
	{
		Log::Notice( "Destroying renderer thread...\n" );
		GLimp_ShutdownRenderThread();
	}

#endif

	if ( glContext )
	{
		SDL_GL_DeleteContext( glContext );
		glContext = nullptr;
	}

	if ( window )
	{
		SDL_DestroyWindow( window );
		window = nullptr;
	}

	SDL_QuitSubSystem( SDL_INIT_VIDEO );

	Com_Memset( &glConfig, 0, sizeof( glConfig ) );
	Com_Memset( &glState, 0, sizeof( glState ) );
}

static void GLimp_Minimize()
{
	SDL_MinimizeWindow( window );
}

/*
===============
GLimp_CompareModes
===============
*/
static int GLimp_CompareModes( const void *a, const void *b )
{
	const float ASPECT_EPSILON = 0.001f;
	SDL_Rect    *modeA = ( SDL_Rect * ) a;
	SDL_Rect    *modeB = ( SDL_Rect * ) b;
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
static void GLimp_DetectAvailableModes()
{
	char     buf[ MAX_STRING_CHARS ] = { 0 };
	SDL_Rect modes[ 128 ];
	int      numModes = 0;
	int      i;
	SDL_DisplayMode windowMode;
	int      display;

	display = SDL_GetWindowDisplayIndex( window );

	if ( SDL_GetWindowDisplayMode( window, &windowMode ) < 0 )
	{
		Log::Warn("Couldn't get window display mode: %s", SDL_GetError() );
		return;
	}

	for ( i = 0; i < SDL_GetNumDisplayModes( display ); i++ )
	{
		SDL_DisplayMode mode;

		if ( SDL_GetDisplayMode( display, i, &mode ) < 0 )
		{
			continue;
		}

		if ( !mode.w || !mode.h )
		{
			Log::Notice("Display supports any resolution" );
			return;
		}

		if ( windowMode.format != mode.format || windowMode.refresh_rate != mode.refresh_rate )
		{
			continue;
		}

		modes[ numModes ].w = mode.w;
		modes[ numModes ].h = mode.h;
		numModes++;
	}

	if ( numModes > 1 )
	{
		qsort( modes, numModes, sizeof( SDL_Rect ), GLimp_CompareModes );
	}

	for ( i = 0; i < numModes; i++ )
	{
		const char *newModeString = va( "%ux%u ", modes[ i ].w, modes[ i ].h );

		if ( strlen( newModeString ) < ( int ) sizeof( buf ) - strlen( buf ) )
		{
			Q_strcat( buf, sizeof( buf ), newModeString );
		}
		else
		{
			Log::Warn("Skipping mode %ux%x, buffer too small", modes[ i ].w, modes[ i ].h );
		}
	}

	if ( *buf )
	{
		Log::Notice("Available modes: '%s'", buf );
		ri.Cvar_Set( "r_availableModes", buf );
	}
}

/*
===============
GLimp_SetMode
===============
*/
static rserr_t GLimp_SetMode( int mode, bool fullscreen, bool noborder )
{
	const char  *glstring;
	int         perChannelColorBits;
	int         alphaBits, depthBits, stencilBits;
	int         samples;
	int         i = 0;
	SDL_Surface *icon = nullptr;
	SDL_DisplayMode desktopMode;
	Uint32      flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
	int         x, y;
	GLenum      glewResult;

	Log::Notice("Initializing OpenGL display" );

	if ( r_allowResize->integer )
	{
		flags |= SDL_WINDOW_RESIZABLE;
	}

	if ( r_centerWindow->integer )
	{
		// center window on specified display
		x = SDL_WINDOWPOS_CENTERED_DISPLAY( r_displayIndex->integer );
		y = SDL_WINDOWPOS_CENTERED_DISPLAY( r_displayIndex->integer );
	}
	else
	{
		x = SDL_WINDOWPOS_UNDEFINED_DISPLAY( r_displayIndex->integer );
		y = SDL_WINDOWPOS_UNDEFINED_DISPLAY( r_displayIndex->integer );
	}

	icon = SDL_CreateRGBSurfaceFrom( ( void * ) CLIENT_WINDOW_ICON.pixel_data,
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

	if ( SDL_GetDesktopDisplayMode( r_displayIndex->integer, &desktopMode ) == 0 )
	{
		displayAspect = ( float ) desktopMode.w / ( float ) desktopMode.h;

		Log::Notice("Display aspect: %.3f", displayAspect );
	}
	else
	{
		Com_Memset( &desktopMode, 0, sizeof( SDL_DisplayMode ) );

		Log::Notice("Cannot determine display aspect (%s), assuming 1.333", SDL_GetError() );
	}

	Log::Notice("...setting mode %d:", mode );

	if ( mode == -2 )
	{
		// use desktop video resolution
		if ( desktopMode.h > 0 )
		{
			glConfig.vidWidth = desktopMode.w;
			glConfig.vidHeight = desktopMode.h;
		}
		else
		{
			glConfig.vidWidth = 640;
			glConfig.vidHeight = 480;
			Log::Notice("Cannot determine display resolution, assuming 640x480" );
		}

		glConfig.windowAspect = ( float ) glConfig.vidWidth / ( float ) glConfig.vidHeight;
	}
	else if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) )
	{
		Log::Notice(" invalid mode" );
		return rserr_t::RSERR_INVALID_MODE;
	}

	Log::Notice(" %d %d", glConfig.vidWidth, glConfig.vidHeight );
	Cvar_Set( "r_customwidth", va("%d", glConfig.vidWidth ) );
	Cvar_Set( "r_customheight", va("%d", glConfig.vidHeight ) );

	do
	{
		if ( glContext != nullptr )
		{
			SDL_GL_DeleteContext( glContext );
			glContext = nullptr;
		}

		if ( window != nullptr )
		{
			SDL_GetWindowPosition( window, &x, &y );
			Log::Debug("Existing window at %dx%d before being destroyed", x, y );
			SDL_DestroyWindow( window );
			window = nullptr;
		}
		// we come back here if we couldn't get a visual and there's
		// something we can switch off

		if ( fullscreen )
		{
			flags |= SDL_WINDOW_FULLSCREEN;
			glConfig.isFullscreen = true;
		}
		else
		{
			if ( noborder )
			{
				flags |= SDL_WINDOW_BORDERLESS;
			}

			glConfig.isFullscreen = false;
		}

		colorBits = r_colorbits->integer;

		if ( ( !colorBits ) || ( colorBits >= 32 ) )
		{
			colorBits = 24;
		}

		alphaBits = r_alphabits->integer;

		if ( alphaBits < 0 )
		{
			alphaBits = 0;
		}

		depthBits = r_depthbits->integer;

		if ( !depthBits )
		{
			depthBits = 24;
		}

		stencilBits = r_stencilbits->integer;
		samples = r_ext_multisample->integer;

		for ( i = 0; i < 16; i++ )
		{
			int testColorBits, testDepthBits, testStencilBits;

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
						if ( colorBits == 24 )
						{
							colorBits = 16;
						}

						break;

					case 1:
						if ( depthBits == 24 )
						{
							depthBits = 16;
						}
						else if ( depthBits == 16 )
						{
							depthBits = 8;
						}

					case 3:
						if ( stencilBits == 24 )
						{
							stencilBits = 16;
						}
						else if ( stencilBits == 16 )
						{
							stencilBits = 8;
						}
				}
			}

			testColorBits = colorBits;
			testDepthBits = depthBits;
			testStencilBits = stencilBits;

			if ( ( i % 4 ) == 3 )
			{
				// reduce colorbits
				if ( testColorBits == 24 )
				{
					testColorBits = 16;
				}
			}

			if ( ( i % 4 ) == 2 )
			{
				// reduce depthbits
				if ( testDepthBits == 24 )
				{
					testDepthBits = 16;
				}
				else if ( testDepthBits == 16 )
				{
					testDepthBits = 8;
				}
			}

			if ( ( i % 4 ) == 1 )
			{
				// reduce stencilbits
				if ( testStencilBits == 24 )
				{
					testStencilBits = 16;
				}
				else if ( testStencilBits == 16 )
				{
					testStencilBits = 8;
				}
				else
				{
					testStencilBits = 0;
				}
			}

			if ( testColorBits == 24 )
			{
				perChannelColorBits = 8;
			}
			else
			{
				perChannelColorBits = 4;
			}

			SDL_GL_SetAttribute( SDL_GL_RED_SIZE, perChannelColorBits );
			SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, perChannelColorBits );
			SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, perChannelColorBits );
			SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, alphaBits );
			SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, testDepthBits );
			SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, testStencilBits );
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0 );
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, samples );
			SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

			if ( !r_glAllowSoftware->integer )
			{
				SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
			}

			if ( r_glCoreProfile->integer || r_glDebugProfile->integer )
			{
				int major = r_glMajorVersion->integer;
				int minor = r_glMinorVersion->integer;

				SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, major );
				SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, minor );

				if ( r_glCoreProfile->integer )
				{
					SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
				}
				else
				{
					SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
				}

				if ( r_glDebugProfile->integer )
				{
					SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
				}
			}
			window = SDL_CreateWindow( CLIENT_WINDOW_TITLE, x, y, glConfig.vidWidth, glConfig.vidHeight, flags );

			if ( !window )
			{
				Log::Warn("SDL_CreateWindow failed: %s\n", SDL_GetError() );
				continue;
			}

			SDL_SetWindowIcon( window, icon );

			glContext = SDL_GL_CreateContext( window );

			if ( !glContext )
			{
				Log::Warn("SDL_GL_CreateContext failed: %s\n", SDL_GetError() );
				continue;
			}
			SDL_GL_SetSwapInterval( r_swapInterval->integer );

			glConfig.colorBits = testColorBits;
			glConfig.depthBits = testDepthBits;
			glConfig.stencilBits = testStencilBits;

			Log::Notice("Using %d Color bits, %d depth, %d stencil display.",
				glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );

			break;
		}

		if ( samples && ( !glContext || !window ) )
		{
			r_ext_multisample->integer = 0;
		}

	} while ( ( !glContext || !window ) && samples );

	SDL_FreeSurface( icon );

    glewExperimental = GL_TRUE;
	glewResult = glewInit();

	if ( glewResult != GLEW_OK )
	{
		// glewInit failed, something is seriously wrong
		ri.Error( errorParm_t::ERR_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem: %s", glewGetErrorString( glewResult ) );
	}
	else
	{
		Log::Notice("Using GLEW %s", glewGetString( GLEW_VERSION ) );
	}

	int GLmajor, GLminor;
	sscanf( ( const char * ) glGetString( GL_VERSION ), "%d.%d", &GLmajor, &GLminor );
	if ( GLmajor < 2 || ( GLmajor == 2 && GLminor < 1 ) )
	{
		// missing shader support, switch to 1.x renderer
		return rserr_t::RSERR_OLD_GL;
	}

	if ( GLmajor < 3 || ( GLmajor == 3 && GLminor < 2 ) )
	{
		// shaders are supported, but not all GL3.x features
		Log::Notice("Using enhanced (GL3) Renderer in GL 2.x mode..." );
	}
	else
	{
		Log::Notice("Using enhanced (GL3) Renderer in GL 3.x mode..." );
		glConfig.driverType = glDriverType_t::GLDRV_OPENGL3;
	}
	GLimp_DetectAvailableModes();

	glstring = ( char * ) glGetString( GL_RENDERER );
	Log::Notice("GL_RENDERER: %s", glstring );

	return rserr_t::RSERR_OK;
}

static void AssertCvarRange( cvar_t *cv, float minVal, float maxVal, bool shouldBeIntegral )
{
	if ( shouldBeIntegral )
	{
		if ( ( int ) cv->value != cv->integer )
		{
			Log::Warn("cvar '%s' must be integral (%f)", cv->name, cv->value );
			ri.Cvar_Set( cv->name, va( "%d", cv->integer ) );
		}
	}

	if ( cv->value < minVal )
	{
		Log::Warn("cvar '%s' out of range (%f < %f)", cv->name, cv->value, minVal );
		ri.Cvar_Set( cv->name, va( "%f", minVal ) );
	}
	else if ( cv->value > maxVal )
	{
		Log::Warn("cvar '%s' out of range (%f > %f)", cv->name, cv->value, maxVal );
		ri.Cvar_Set( cv->name, va( "%f", maxVal ) );
	}
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static bool GLimp_StartDriverAndSetMode( int mode, bool fullscreen, bool noborder )
{
	int numDisplays;

	if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		const char *driverName;
		SDL_version v;
		SDL_GetVersion( &v );

		Log::Notice("SDL_Init( SDL_INIT_VIDEO )... " );
		Log::Notice("Using SDL Version %u.%u.%u", v.major, v.minor, v.patch );

		if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE ) == -1 )
		{
			Log::Notice("SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) FAILED (%s)", SDL_GetError() );
			return false;
		}

		driverName = SDL_GetCurrentVideoDriver();

		if ( !driverName )
		{
			ri.Error( errorParm_t::ERR_FATAL, "No video driver initialized\n" );
		}

		Log::Notice("SDL using driver \"%s\"", driverName );
		ri.Cvar_Set( "r_sdlDriver", driverName );
	}

	numDisplays = SDL_GetNumVideoDisplays();

	if ( numDisplays <= 0 )
	{
		ri.Error( errorParm_t::ERR_FATAL, "SDL_GetNumVideoDisplays FAILED (%s)\n", SDL_GetError() );
	}

	AssertCvarRange( r_displayIndex, 0, numDisplays - 1, true );

	if ( fullscreen && ri.Cvar_VariableIntegerValue( "in_nograb" ) )
	{
		Log::Notice("Fullscreen not allowed with in_nograb 1" );
		ri.Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = false;
		fullscreen = false;
	}

	rserr_t err = GLimp_SetMode(mode, fullscreen, noborder);

	switch ( err )
	{
		case rserr_t::RSERR_INVALID_FULLSCREEN:
			Log::Notice("...WARNING: fullscreen unavailable in this mode" );
			return false;

		case rserr_t::RSERR_INVALID_MODE:
			Log::Notice("...WARNING: could not set the given mode (%d)", mode );
			return false;

		case rserr_t::RSERR_OLD_GL:
			Log::Notice("...WARNING: OpenGL too old" );
			return false;

		default:
			break;
	}

	return true;
}

static GLenum debugTypes[] =
{
	0,
	GL_DEBUG_TYPE_ERROR_ARB,
	GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB,
	GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB,
	GL_DEBUG_TYPE_PORTABILITY_ARB,
	GL_DEBUG_TYPE_PERFORMANCE_ARB,
	GL_DEBUG_TYPE_OTHER_ARB
};

#ifdef _WIN32
#define DEBUG_CALLBACK_CALL APIENTRY
#else
#define DEBUG_CALLBACK_CALL
#endif
static void DEBUG_CALLBACK_CALL GLimp_DebugCallback( GLenum, GLenum type, GLuint,
                                       GLenum severity, GLsizei, const GLchar *message, const void* )
{
	const char *debugTypeName;
	const char *debugSeverity;

	if ( r_glDebugMode->integer <= Util::ordinal(glDebugModes_t::GLDEBUG_NONE))
	{
		return;
	}

	if ( r_glDebugMode->integer < Util::ordinal(glDebugModes_t::GLDEBUG_ALL))
	{
		if ( debugTypes[ r_glDebugMode->integer ] != type )
		{
			return;
		}
	}

	switch ( type )
	{
		case GL_DEBUG_TYPE_ERROR_ARB:
			debugTypeName = "DEBUG_TYPE_ERROR";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
			debugTypeName = "DEBUG_TYPE_DEPRECATED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
			debugTypeName = "DEBUG_TYPE_UNDEFINED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB:
			debugTypeName = "DEBUG_TYPE_PORTABILITY";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB:
			debugTypeName = "DEBUG_TYPE_PERFORMANCE";
			break;
		case GL_DEBUG_TYPE_OTHER_ARB:
			debugTypeName = "DEBUG_TYPE_OTHER";
			break;
		default:
			debugTypeName = "DEBUG_TYPE_UNKNOWN";
			break;
	}

	switch ( severity )
	{
		case GL_DEBUG_SEVERITY_HIGH_ARB:
			debugSeverity = "high";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM_ARB:
			debugSeverity = "med";
			break;
		case GL_DEBUG_SEVERITY_LOW_ARB:
			debugSeverity = "low";
			break;
		default:
			debugSeverity = "none";
			break;
	}

	Log::Warn("%s: severity: %s msg: %s", debugTypeName, debugSeverity, message );
}

/*
===============
GLimp_InitExtensions
===============
*/

static void RequireExt( bool hasExt, const char* name )
{
	if ( hasExt )
	{
		Log::Notice("...using GL_%s", name );
	}
	else
	{
		Log::Notice("...GL_%s not found", name );
	}
}

static bool LoadExtWithCvar( bool hasExt, const char* name, bool cvarValue )
{
	if ( hasExt )
	{
		if ( cvarValue )
		{
			Log::Notice("...using GL_%s", name );
			return true;
		}
		else
		{
			Log::Notice("...ignoring GL_%s", name );
		}
	}
	else
	{
		Log::Notice("...GL_%s not found", name );
	}
	return false;
}

#define REQUIRE_EXTENSION(ext) RequireExt(GLEW_##ext, #ext)

#define LOAD_EXTENSION_WITH_CVAR(ext, cvar) LoadExtWithCvar(GLEW_##ext, #ext, cvar->value)

static void GLimp_InitExtensions()
{
	Log::Notice("Initializing OpenGL extensions" );

	if ( LOAD_EXTENSION_WITH_CVAR(ARB_debug_output, r_glDebugProfile) )
	{
		glDebugMessageCallbackARB( (GLDEBUGPROCARB)GLimp_DebugCallback, nullptr );
		glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
	}

	// Shaders
	REQUIRE_EXTENSION( ARB_fragment_shader );
	REQUIRE_EXTENSION( ARB_vertex_shader );

	glGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &glConfig2.maxVertexUniforms );
	glGetIntegerv( GL_MAX_VERTEX_ATTRIBS_ARB, &glConfig2.maxVertexAttribs );

	int reservedComponents = 36 * 10; // approximation how many uniforms we have besides the bone matrices
	glConfig2.maxVertexSkinningBones = Math::Clamp( ( glConfig2.maxVertexUniforms - reservedComponents ) / 16, 0, MAX_BONES );
	glConfig2.vboVertexSkinningAvailable = r_vboVertexSkinning->integer && ( ( glConfig2.maxVertexSkinningBones >= 12 ) ? true : false );

	// GLSL
	REQUIRE_EXTENSION( ARB_shading_language_100 );

	Q_strncpyz( glConfig2.shadingLanguageVersionString, ( char * ) glGetString( GL_SHADING_LANGUAGE_VERSION_ARB ),
				sizeof( glConfig2.shadingLanguageVersionString ) );
	int majorVersion, minorVersion;
	if ( sscanf( glConfig2.shadingLanguageVersionString, "%i.%i", &majorVersion, &minorVersion ) != 2 )
	{
		Log::Warn("unrecognized shading language version string format" );
	}
	glConfig2.shadingLanguageVersion = majorVersion * 100 + minorVersion;

	Log::Notice("...found shading language version %i", glConfig2.shadingLanguageVersion );

	// Texture formats and compression
	REQUIRE_EXTENSION( ARB_depth_texture );

	REQUIRE_EXTENSION( ARB_texture_cube_map );
	glGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig2.maxCubeMapTextureSize );

	glConfig2.textureHalfFloatAvailable =  LOAD_EXTENSION_WITH_CVAR(ARB_half_float_pixel, r_ext_half_float_pixel);
	glConfig2.textureFloatAvailable = LOAD_EXTENSION_WITH_CVAR(ARB_texture_float, r_ext_texture_float);
	glConfig2.textureIntegerAvailable = LOAD_EXTENSION_WITH_CVAR(EXT_texture_integer, r_ext_texture_integer);
	glConfig2.textureRGAvailable = LOAD_EXTENSION_WITH_CVAR(ARB_texture_rg, r_ext_texture_rg);

	// TODO figure out what was the problem with MESA
	glConfig2.framebufferPackedDepthStencilAvailable = glConfig.driverType != glDriverType_t::GLDRV_MESA && LOAD_EXTENSION_WITH_CVAR(EXT_packed_depth_stencil, r_ext_packed_depth_stencil);

	glConfig2.ARBTextureCompressionAvailable = LOAD_EXTENSION_WITH_CVAR(ARB_texture_compression, r_ext_compressed_textures);
	glConfig.textureCompression = textureCompression_t::TC_NONE;
	if( LOAD_EXTENSION_WITH_CVAR(EXT_texture_compression_s3tc, r_ext_compressed_textures) )
	{
		glConfig.textureCompression = textureCompression_t::TC_S3TC;
	}
	//REQUIRE_EXTENSION(EXT_texture3D);

	// Texture - others
	glConfig2.textureNPOTAvailable = LOAD_EXTENSION_WITH_CVAR(ARB_texture_non_power_of_two, r_ext_texture_non_power_of_two);
	glConfig2.generateMipmapAvailable = LOAD_EXTENSION_WITH_CVAR(SGIS_generate_mipmap, r_ext_generate_mipmap);

	glConfig2.textureAnisotropyAvailable = false;
	if ( LOAD_EXTENSION_WITH_CVAR(EXT_texture_filter_anisotropic, r_ext_texture_filter_anisotropic) )
	{
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig2.maxTextureAnisotropy );
		glConfig2.textureAnisotropyAvailable = true;
	}

	// VAO and VBO
	//REQUIRE_EXTENSION( ARB_vertex_array_object );
	REQUIRE_EXTENSION( ARB_vertex_buffer_object );
	REQUIRE_EXTENSION( ARB_half_float_vertex );
	REQUIRE_EXTENSION( ARB_framebuffer_object );

	// FBO
	glGetIntegerv( GL_MAX_RENDERBUFFER_SIZE, &glConfig2.maxRenderbufferSize );
	glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &glConfig2.maxColorAttachments );

	// Other
	REQUIRE_EXTENSION( ARB_shader_objects );

	glConfig2.occlusionQueryAvailable = false;
	glConfig2.occlusionQueryBits = 0;
	if ( LOAD_EXTENSION_WITH_CVAR(ARB_occlusion_query, r_ext_occlusion_query) )
	{
		glConfig2.occlusionQueryAvailable = true;
		glGetQueryivARB( GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &glConfig2.occlusionQueryBits );
	}

	glConfig2.drawBuffersAvailable = false;
	if ( LOAD_EXTENSION_WITH_CVAR(ARB_draw_buffers, r_ext_draw_buffers) )
	{
		glGetIntegerv( GL_MAX_DRAW_BUFFERS_ARB, &glConfig2.maxDrawBuffers );
		glConfig2.drawBuffersAvailable = true;
	}

	// gDEBugger debug
	if ( GLEW_GREMEDY_string_marker )
	{
		Log::Notice("...using GL_GREMEDY_string_marker" );
	}
	else
	{
		Log::Notice("...GL_GREMEDY_string_marker not found" );
	}

#ifdef GLEW_ARB_get_program_binary
	if( GLEW_ARB_get_program_binary )
	{
		int formats = 0;

		glGetIntegerv( GL_NUM_PROGRAM_BINARY_FORMATS, &formats );

		if ( !formats )
		{
			Log::Notice("...GL_ARB_get_program_binary found, but with no binary formats");
			glConfig2.getProgramBinaryAvailable = false;
		}
		else
		{
			Log::Notice("...using GL_ARB_get_program_binary");
			glConfig2.getProgramBinaryAvailable = true;
		}
	}
	else
#endif
	{
		Log::Notice("...GL_ARB_get_program_binary not found");
		glConfig2.getProgramBinaryAvailable = false;
	}

#ifdef GLEW_ARB_buffer_storage
	if ( GLEW_ARB_buffer_storage )
	{
		if ( r_arb_buffer_storage->integer )
		{
			Log::Notice("...using GL_ARB_buffer_storage" );
			glConfig2.bufferStorageAvailable = true;
		}
		else
		{
			Log::Notice("...ignoring GL_ARB_buffer_storage" );
			glConfig2.bufferStorageAvailable = false;
		}
	}
	else
#endif
	{
		Log::Notice("...GL_ARB_buffer_storage not found" );
		glConfig2.bufferStorageAvailable = false;
	}

	glConfig2.mapBufferRangeAvailable = LOAD_EXTENSION_WITH_CVAR( ARB_map_buffer_range, r_arb_map_buffer_range );
	glConfig2.syncAvailable = LOAD_EXTENSION_WITH_CVAR( ARB_sync, r_arb_sync );

	GL_CheckErrors();
}

static const int R_MODE_FALLBACK = 3; // 640 * 480

/* Support code for GLimp_Init */

static void reportDriverType( bool force )
{
	static const char *const drivers[] = {
		"integrated", "stand-alone", "OpenGL 3+", "Mesa"
	};
	if (glConfig.driverType > glDriverType_t::GLDRV_UNKNOWN && (unsigned) glConfig.driverType < ARRAY_LEN( drivers ) )
	{
		Log::Notice("%s graphics driver class '%s'",
		           force ? "User has forced" : "Detected",
		           drivers[Util::ordinal(glConfig.driverType)] );
	}
}

static void reportHardwareType( bool force )
{
	static const char *const hardware[] = {
		"generic", "ATI Radeon", "AMD Radeon DX10-class", "nVidia DX10-class"
	};
	if (glConfig.hardwareType > glHardwareType_t::GLHW_UNKNOWN && (unsigned) glConfig.hardwareType < ARRAY_LEN( hardware ) )
	{
		Log::Notice("%s graphics hardware class '%s'",
		           force ? "User has forced" : "Detected",
		           hardware[Util::ordinal(glConfig.hardwareType)] );
	}
}

/*
===============
GLimp_Init

This routine is responsible for initializing the OS specific portions
of OpenGL
===============
*/
bool GLimp_Init()
{
	glConfig.driverType = glDriverType_t::GLDRV_ICD;

	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "", CVAR_ROM );
	r_allowResize = ri.Cvar_Get( "r_allowResize", "0", 0 );
	r_centerWindow = ri.Cvar_Get( "r_centerWindow", "0", 0 );
	r_displayIndex = ri.Cvar_Get( "r_displayIndex", "0", 0 );
	ri.Cvar_Get( "r_availableModes", "", CVAR_ROM );

	ri.Cmd_AddCommand( "minimize", GLimp_Minimize );

	if ( ri.Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		ri.Cvar_Set( "r_mode", va( "%d", R_MODE_FALLBACK ) );
		ri.Cvar_Set( "r_fullscreen", "0" );
		ri.Cvar_Set( "r_centerWindow", "0" );
		ri.Cvar_Set( "com_abnormalExit", "0" );
	}

	// Create the window and set up the context
	if ( GLimp_StartDriverAndSetMode( r_mode->integer, r_fullscreen->integer, false ) )
	{
		goto success;
	}

	// Finally, try the default screen resolution
	if ( r_mode->integer != R_MODE_FALLBACK )
	{
		Log::Notice("Setting r_mode %d failed, falling back on r_mode %d", r_mode->integer, R_MODE_FALLBACK );

		if ( GLimp_StartDriverAndSetMode( R_MODE_FALLBACK, false, false ) )
		{
			goto success;
		}
	}

	// Nothing worked, give up
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	return false;

success:
	// These values force the UI to disable driver selection
	glConfig.hardwareType = glHardwareType_t::GLHW_GENERIC;

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, ( char * ) glGetString( GL_VENDOR ), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, ( char * ) glGetString( GL_RENDERER ), sizeof( glConfig.renderer_string ) );

	if ( *glConfig.renderer_string && glConfig.renderer_string[ strlen( glConfig.renderer_string ) - 1 ] == '\n' )
	{
		glConfig.renderer_string[ strlen( glConfig.renderer_string ) - 1 ] = 0;
	}

	Q_strncpyz( glConfig.version_string, ( char * ) glGetString( GL_VERSION ), sizeof( glConfig.version_string ) );

	if ( glConfig.driverType == glDriverType_t::GLDRV_OPENGL3 )
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
		glConfig.driverType = glDriverType_t::GLDRV_MESA;
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
			glConfig.hardwareType = glHardwareType_t::GLHW_NV_DX10;
		}
	}
	else if ( Q_stristr( glConfig.renderer_string, "quadro fx" ) )
	{
		if ( Q_stristr( glConfig.renderer_string, "3600" ) )
		{
			glConfig.hardwareType = glHardwareType_t::GLHW_NV_DX10;
		}
	}
	else if ( Q_stristr( glConfig.renderer_string, "gallium" ) &&
	          Q_stristr( glConfig.renderer_string, " amd " ) )
	{
		// anything prior to R600 is listed as ATI.
		glConfig.hardwareType = glHardwareType_t::GLHW_ATI_DX10;
	}
	else if ( Q_stristr( glConfig.renderer_string, "rv770" ) ||
	          Q_stristr( glConfig.renderer_string, "eah4850" ) ||
	          Q_stristr( glConfig.renderer_string, "eah4870" ) ||
	          // previous three are too specific?
	          Q_stristr( glConfig.renderer_string, "radeon hd" ) )
	{
		glConfig.hardwareType = glHardwareType_t::GLHW_ATI_DX10;
	}
	else if ( Q_stristr( glConfig.renderer_string, "radeon" ) )
	{
		glConfig.hardwareType = glHardwareType_t::GLHW_ATI;
	}

	reportDriverType( false );
	reportHardwareType( false );

	{ // allow overriding where the user really does know better
		cvar_t          *forceGL;
		glDriverType_t   driverType   = glDriverType_t::GLDRV_UNKNOWN;
		glHardwareType_t hardwareType = glHardwareType_t::GLHW_UNKNOWN;

		forceGL = ri.Cvar_Get( "r_glForceDriver", "", CVAR_LATCH );

		if      ( !Q_stricmp( forceGL->string, "icd" ))
		{
			driverType = glDriverType_t::GLDRV_ICD;
		}
		else if ( !Q_stricmp( forceGL->string, "standalone" ))
		{
			driverType = glDriverType_t::GLDRV_STANDALONE;
		}
		else if ( !Q_stricmp( forceGL->string, "opengl3" ))
		{
			driverType = glDriverType_t::GLDRV_OPENGL3;
		}
		else if ( !Q_stricmp( forceGL->string, "mesa" ))
		{
			driverType = glDriverType_t::GLDRV_MESA;
		}

		forceGL = ri.Cvar_Get( "r_glForceHardware", "", CVAR_LATCH );

		if      ( !Q_stricmp( forceGL->string, "generic" ))
		{
			hardwareType = glHardwareType_t::GLHW_GENERIC;
		}
		else if ( !Q_stricmp( forceGL->string, "ati" ))
		{
			hardwareType = glHardwareType_t::GLHW_ATI;
		}
		else if ( !Q_stricmp( forceGL->string, "atidx10" ) ||
		          !Q_stricmp( forceGL->string, "radeonhd" ))
		{
			hardwareType = glHardwareType_t::GLHW_ATI_DX10;
		}
		else if ( !Q_stricmp( forceGL->string, "nvdx10" ))
		{
			hardwareType = glHardwareType_t::GLHW_NV_DX10;
		}

		if ( driverType != glDriverType_t::GLDRV_UNKNOWN )
		{
			glConfig.driverType = driverType;
			reportDriverType( true );
		}

		if ( hardwareType != glHardwareType_t::GLHW_UNKNOWN )
		{
			glConfig.hardwareType = hardwareType;
			reportHardwareType( true );
		}
	}

	// initialize extensions
	GLimp_InitExtensions();

	// This depends on SDL_INIT_VIDEO, hence having it here
	ri.IN_Init( window );

	return true;
}

/*
===============
GLimp_EndFrame

Responsible for doing a swapbuffers
===============
*/
void GLimp_EndFrame()
{
	// don't flip if drawing to front buffer
	if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		SDL_GL_SwapWindow( window );
	}
}

/*
===============
GLimp_HandleCvars

Responsible for handling cvars that change the window or GL state
Should only be called by the main thread
===============
*/
void GLimp_HandleCvars()
{
	if ( r_swapInterval->modified )
	{
		AssertCvarRange( r_swapInterval, -1, 1, true );
		R_SyncRenderThread();
		SDL_GL_SetSwapInterval( r_swapInterval->integer );
		r_swapInterval->modified = false;
	}

	if ( r_fullscreen->modified )
	{
		bool    fullscreen;
		bool    needToToggle = true;
		int         sdlToggled = false;
		fullscreen = !!( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN );

		if ( r_fullscreen->integer && ri.Cvar_VariableIntegerValue( "in_nograb" ) )
		{
			Log::Notice("Fullscreen not allowed with in_nograb 1" );
			ri.Cvar_Set( "r_fullscreen", "0" );
			r_fullscreen->modified = false;
		}

		// Is the state we want different from the current state?
		needToToggle = !!r_fullscreen->integer != fullscreen;

		if ( needToToggle )
		{
			sdlToggled = SDL_SetWindowFullscreen( window, r_fullscreen->integer );

			if ( sdlToggled < 0 )
			{
				Cmd::BufferCommandText("vid_restart");
			}

			ri.IN_Restart();
		}

		r_fullscreen->modified = false;
	}
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
