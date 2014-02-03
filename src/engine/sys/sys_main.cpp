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

#include "revision.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef OPENMP
#include <omp.h>
#endif

#if defined(_WIN32) || (!defined(DEDICATED) && !defined(BUILD_TTY_CLIENT))
#include <SDL.h>
#include "sdl2_compat.h"
#endif

#include "sys_local.h"
#include "sys_loadlib.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

static char binaryPath[ MAX_OSPATH ] = { 0 };

#ifdef USE_CURSES
static qboolean nocurses = qfalse;
void            CON_Init_tty( void );

#endif

/*
=================
Sys_SetBinaryPath
=================
*/
void Sys_SetBinaryPath( const char *path )
{
	Q_strncpyz( binaryPath, path, sizeof( binaryPath ) );
}

/*
=================
Sys_DefaultBasePath
=================
*/
char *Sys_DefaultBasePath( void )
{
	static char installdir[ MAX_OSPATH ];

	Com_sprintf( installdir, sizeof( installdir ), "%s", Sys_Cwd() );

	Q_strreplace( installdir, sizeof( installdir ), "bin32", "" );
	Q_strreplace( installdir, sizeof( installdir ), "bin64", "" );

	Q_strreplace( installdir, sizeof( installdir ), "src/engine", "" );
	Q_strreplace( installdir, sizeof( installdir ), "src\\engine", "" );

	Q_strreplace( installdir, sizeof( installdir ), "bin/win32", "" );
	Q_strreplace( installdir, sizeof( installdir ), "bin\\win32", "" );

	Q_strreplace( installdir, sizeof( installdir ), "bin/win64", "" );
	Q_strreplace( installdir, sizeof( installdir ), "bin\\win64", "" );

	Q_strreplace( installdir, sizeof( installdir ), "bin/linux-x86", "" );
	Q_strreplace( installdir, sizeof( installdir ), "bin/linux-x86_64", "" );

	// MacOS X x86 and x64
	Q_strreplace( installdir, sizeof( installdir ), "bin/macosx", "" );

	return installdir;
}

/*
=================
Sys_DefaultLibPath
=================
*/
char *Sys_DefaultLibPath( void )
{
	return Sys_Cwd();
}


/*
=================
Sys_DefaultAppPath
=================
*/
char *Sys_DefaultAppPath( void )
{
	return binaryPath;
}

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( void )
{
	IN_Restart();
}

/*
=================
Sys_ConsoleInput

Handle new console input
=================
*/
char *Sys_ConsoleInput( void )
{
	return CON_Input();
}

/*
=================
Sys_Exit

Single exit point (regular exit or in case of error)
=================
*/
static void NORETURN Sys_Exit( int exitCode )
{
	CON_Shutdown();

#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)
	SDL_Quit();
#endif

	Sys_PlatformExit();
	exit( exitCode );
}

/*
=================
Sys_Quit
=================
*/
void NORETURN Sys_Quit( void )
{
	Sys_Exit( 0 );
}

/*
=================
Sys_GetProcessorFeatures
=================
*/
int Sys_GetProcessorFeatures( void )
{
	int features = 0;
#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)
	if( SDL_HasRDTSC( ) ) features |= CF_RDTSC;
	if( SDL_HasMMX( ) ) features |= CF_MMX;
	if( SDL_Has3DNow( ) ) features |= CF_3DNOW;
	if( SDL_HasSSE( ) ) features |= CF_SSE;
	if( SDL_HasSSE2( ) ) features |= CF_SSE2;
	if( SDL_HasSSE3( ) ) features |= CF_SSE3;
	if( SDL_HasSSE41( ) ) features |= CF_SSE4_1;
	if( SDL_HasSSE42( ) ) features |= CF_SSE4_2;
	if( SDL_HasAltiVec( ) ) features |= CF_ALTIVEC;
#endif
	return features;
}

/*
=================
Sys_Init
=================
*/
void Sys_Init( void )
{
	Cmd_AddCommand( "in_restart", Sys_In_Restart_f );
	Cvar_Set( "arch", PLATFORM_STRING );
	Cvar_Set( "username", Sys_GetCurrentUser() );
}

/*
=================
Sys_AnsiColorPrint

Transform Q3 colour codes to ANSI escape sequences
=================
*/
void Sys_AnsiColorPrint( const char *msg )
{
	static char buffer[ MAXPRINTMSG ];
	int         length = 0;

	// Approximations of g_color_table (q_math.c)
#define A_BOLD 16
#define A_DIM  32
	static const char colour16map[2][32] = {
		{ // Variant 1 (xterm)
			0 | A_BOLD, 1,          2,          3,
			4,          6,          5,          7,
			3 | A_DIM,  7 | A_DIM,  7 | A_DIM,  7 | A_DIM,
			2 | A_DIM,  3 | A_DIM,  4 | A_DIM,  1 | A_DIM,
			3 | A_DIM,  3 | A_DIM,  6 | A_DIM,  5 | A_DIM,
			6 | A_DIM,  5 | A_DIM,  6 | A_DIM,  2 | A_BOLD,
			2 | A_DIM,  1,          1 | A_DIM,  3 | A_DIM,
			3 | A_DIM,  2 | A_DIM,  5,          3 | A_BOLD
		},
		{ // Variant 1 (vte)
			0 | A_BOLD, 1,          2,          3 | A_BOLD,
			4,          6,          5,          7,
			3        ,  7 | A_DIM,  7 | A_DIM,  7 | A_DIM,
			2 | A_DIM,  3,          4 | A_DIM,  1 | A_DIM,
			3 | A_DIM,  3 | A_DIM,  6 | A_DIM,  5 | A_DIM,
			6 | A_DIM,  5 | A_DIM,  6 | A_DIM,  2 | A_BOLD,
			2 | A_DIM,  1,          1 | A_DIM,  3 | A_DIM,
			3 | A_DIM,  2 | A_DIM,  5,          3 | A_BOLD
		}
	};
	static const char modifier[][4] = { "", ";1", ";2", "" };

	int index = abs( com_ansiColor->integer ) - 1;

	if ( index >= ARRAY_LEN( colour16map ) )
	{
		index = 0;
	}

	while ( *msg )
	{
		if ( Q_IsColorString( msg ) || *msg == '\n' )
		{
			// First empty the buffer
			if ( length > 0 )
			{
				buffer[ length ] = '\0';
				fputs( buffer, stderr );
				length = 0;
			}

			if ( *msg == '\n' )
			{
				// Issue a reset and then the newline
				fputs( "\033[0;40;37m\n", stderr );
				msg++;
			}
			else
			{
				// Print the color code
				int colour = colour16map[ index ][ ( msg[ 1 ] - '0' ) & 31 ];

				Com_sprintf( buffer, sizeof( buffer ), "\033[%s%d%sm",
				             (colour & 0x30) == 0 ? "0;" : "",
				             30 + ( colour & 15 ), modifier[ ( colour / 16 ) & 3 ] );
				fputs( buffer, stderr );
				msg += 2;
			}
		}
		else
		{
			if ( length >= MAXPRINTMSG - 1 )
			{
				break;
			}

			if ( *msg == Q_COLOR_ESCAPE && msg[1] == Q_COLOR_ESCAPE )
			{
				++msg;
			}

			buffer[ length ] = *msg;
			length++;
			msg++;
		}
	}

	// Empty anything still left in the buffer
	if ( length > 0 )
	{
		buffer[ length ] = '\0';
		fputs( buffer, stderr );
	}
}

/*
=================
Sys_Print
=================
*/
void Sys_Print( const char *msg )
{
	CON_LogWrite( msg );
	CON_Print( msg );
}

/*
=================
Sys_Error
=================
*/
void PRINTF_LIKE(1) NORETURN Sys_Error( const char *error, ... )
{
	va_list argptr;
	char    string[ 1024 ];

	va_start( argptr, error );
	Q_vsnprintf( string, sizeof( string ), error, argptr );
	va_end( argptr );

	// Print text in the console window/box
	Sys_Print( string );
	Sys_Print( "\n" );

	CL_Shutdown();

	Q_CleanStr( string );
	Sys_ErrorDialog( string );

	Sys_Exit( 3 );
}

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int Sys_FileTime( char *path )
{
	struct stat buf;

	if ( stat( path, &buf ) == -1 )
	{
		return -1;
	}

	return buf.st_mtime;
}

/*
=================
Sys_UnloadDll
=================
*/
void Sys_UnloadDll( void *dllHandle )
{
	if ( !dllHandle )
	{
		Com_Printf( "Sys_UnloadDll(NULL)\n" );
		return;
	}

	Sys_UnloadLibrary( dllHandle );
}

/*
=================
Sys_LoadDll

Used to load a DLL instead of a virtual machine
look in fs_libpath
=================
*/
void *QDECL Sys_LoadDll( const char *name,
                         intptr_t ( QDECL  * *entryPoint )( int, ... ),
                         intptr_t ( QDECL *systemcalls )( intptr_t, ... ) )
{
	void *libHandle = NULL;
	void ( QDECL * dllEntry )( intptr_t ( QDECL * syscallptr )( intptr_t, ... ) );
	char fname[ MAX_QPATH ];

	assert( name );

	Com_sprintf( fname, sizeof( fname ), "%s%s", name, DLL_EXT );

	std::string fn = FS::Path::Build( FS::GetLibPath(), fname );
	Com_DPrintf( "Sys_LoadDll(%s)...\n", fn.c_str() );

	libHandle = Sys_LoadLibrary( fn.c_str() );

	if ( !libHandle )
	{
		Com_Printf( "Sys_LoadDll(%s) failed:\n\"%s\"\n", name, Sys_LibraryError() );
		return NULL;
	}

	// Try to load the dllEntry and vmMain function.
	dllEntry = ( void ( QDECL * )( intptr_t ( QDECL * )( intptr_t, ... ) ) ) Sys_LoadFunction( libHandle, "dllEntry" );
	*entryPoint = ( intptr_t ( QDECL  * )( int, ... ) ) Sys_LoadFunction( libHandle, "vmMain" );

	if ( !*entryPoint || !dllEntry )
	{
#ifndef NDEBUG
		if ( !dllEntry )
		{
			Com_Error( ERR_FATAL, "Sys_LoadDll(%s) failed SDL_LoadFunction(dllEntry):\n\"%s\" !", name, Sys_LibraryError() );
		}
		else
		{
			Com_Error( ERR_FATAL, "Sys_LoadDll(%s) failed SDL_LoadFunction(vmMain):\n\"%s\" !", name, Sys_LibraryError() );
		}
#else
		if ( !dllEntry )
		{
			Com_Printf( "Sys_LoadDll(%s) failed SDL_LoadFunction(dllEntry):\n\"%p\" !\n", name, Sys_LibraryError() );
		}
		else
		{
			Com_Printf( "Sys_LoadDll(%s) failed SDL_LoadFunction(vmMain):\n\"%p\" !\n", name, Sys_LibraryError() );
		}
#endif
		Sys_UnloadLibrary( libHandle );
		return NULL;
	}

	Com_DPrintf( "Sys_LoadDll(%s) found vmMain function at %p\n", name, *entryPoint );
	dllEntry( systemcalls );

	Com_DPrintf( "Sys_LoadDll(%s) succeeded!\n", name );

	return libHandle;
}

/*
=================
Sys_ParseArgs
=================
*/
void Sys_ParseArgs( int argc, char **argv )
{
	if ( argc == 2 )
	{
		if ( !strcmp( argv[ 1 ], "--version" ) ||
		     !strcmp( argv[ 1 ], "-v" ) )
		{
			const char *date = __DATE__;
#ifdef DEDICATED
			fprintf( stdout, Q3_VERSION " dedicated server (%s)\n", date );
#else
			fprintf( stdout, Q3_VERSION " client (%s)\n", date );
#endif
			Sys_Exit( 0 );
		}
	}
}

/*
=================
Sys_SigHandler
=================
*/
void NORETURN Sys_SigHandler( int signal )
{
	static qboolean signalcaught = qfalse;

	if ( signalcaught )
	{
		fprintf( stderr, "DOUBLE SIGNAL FAULT: Received signal %d, exiting...\n",
		         signal );
	}
	else
	{
		signalcaught = qtrue;
#if !defined(DEDICATED)
		CL_Shutdown();
#endif
		SV_Shutdown( va( "Received signal %d", signal ) );
	}

	if ( signal == SIGTERM || signal == SIGINT )
	{
		Sys_Exit( 1 );
	}
	else
	{
		Sys_Exit( 2 );
	}
}

/*
=================
main
=================
*/

#ifdef DEDICATED
#define UNVANQUISHED_URL ""
#else
#define UNVANQUISHED_URL " [unv://ADDRESS[:PORT]]"
#endif

void Sys_HelpText( const char *binaryName )
{
	printf( PRODUCT_NAME " " PRODUCT_VERSION "\n"
	        "Usage: %s" UNVANQUISHED_URL " [+COMMAND...]\n"
	        , binaryName );
}

// GCC expects a 16-byte aligned stack but Windows only guarantees 4-byte alignment.
// The MinGW startup code should be handling this but for some reason it isn't.
#if defined(_WIN32) && defined(__GNUC__)
#define ALIGN_STACK __attribute__((force_align_arg_pointer))
#else
#define ALIGN_STACK
#endif
int ALIGN_STACK main( int argc, char **argv )
{
	int  i;
	char commandLine[ MAX_STRING_CHARS ] = { 0 };

#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)
	// Run time
	SDL_version ver;
	SDL_GetVersion( &ver );
#endif

#ifdef OPENMP
	int nthreads, tid, procs, maxt, inpar, dynamic, nested;
#endif

	if ( argc > 1 )
	{
		if ( !strcmp( argv[1], "--help" ) || !strcmp( argv[1], "-h" ) )
		{
			Sys_HelpText( argv[0] );
			return 0;
		}

		if ( !strcmp( argv[1], "--version" ) || !strcmp( argv[1], "-v" ) )
		{
			printf( PRODUCT_NAME " " PRODUCT_VERSION "\n" );
			return 0;
		}
	}

#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)
	// SDL version check

	// Compile time
#       if !SDL_VERSION_ATLEAST(MINSDL_MAJOR,MINSDL_MINOR,MINSDL_PATCH)
#               error A more recent version of SDL is required
#       endif

#define MINSDL_VERSION \
  XSTRING(MINSDL_MAJOR) "." \
  XSTRING(MINSDL_MINOR) "." \
  XSTRING(MINSDL_PATCH)

	if ( SDL_VERSIONNUM( ver.major, ver.minor, ver.patch ) <
	     SDL_VERSIONNUM( MINSDL_MAJOR, MINSDL_MINOR, MINSDL_PATCH ) )
	{
		Sys_Dialog( DT_ERROR, va( "SDL version " MINSDL_VERSION " or greater is required, "
		                          "but only version %d.%d.%d was found. You may be able to obtain a more recent copy "
		                          "from http://www.libsdl.org/.", ver.major, ver.minor, ver.patch ), "SDL Library Too Old" );

		Sys_Exit( 1 );
	}

#endif

#ifdef OPENMP
	Com_Printf( "-----------------------------------\n" );
	/* Start parallel region */
#pragma omp parallel private(nthreads, tid)
	{
		/* Obtain thread number */
		tid = omp_get_thread_num();

		/* Only master thread does this */
		if ( tid == 0 )
		{
			Com_Printf( "Thread %d getting environment info...\n", tid );

			/* Get environment information */
			procs = omp_get_num_procs();
			nthreads = omp_get_num_threads();
			maxt = omp_get_max_threads();
			inpar = omp_in_parallel();
			dynamic = omp_get_dynamic();
			nested = omp_get_nested();

			Com_Printf( "-----------------------------------\n"
			            "-----------------------------------\n" );

			/* Print environment information */
			Com_Printf( "Number of processors = %d\n", procs );
			Com_Printf( "Number of threads = %d\n", nthreads );
			Com_Printf( "Max threads = %d\n", maxt );
			Com_Printf( "In parallel? = %d\n", inpar );
			Com_Printf( "Dynamic threads enabled? = %d\n", dynamic );
			Com_Printf( "Nested parallelism supported? = %d\n", nested );
		}
	} /* Done */

	Com_Printf( "-----------------------------------\n" );
#endif

	Sys_PlatformInit();

	// Locale initialisation
	// Set from environment, but try to make LC_CTYPE to use UTF-8 and force LC_NUMERIC to C
	{
		char locale[ 64 ], *dot;

		setlocale( LC_ALL, "" );
		setlocale( LC_NUMERIC, "C" );

		// Get the info for LC_CTYPE (ensuring space for appending)
		Q_strncpyz( locale, setlocale( LC_CTYPE, NULL ), sizeof( locale ) - 6 );

		// Remove any existing encoding info then set to UTF-8
		if ( ( dot = strchr( locale, '.') ) )
		{
			*dot = 0;
		}
		strcat( locale, ".UTF-8" );

		// try current language but with UTF-8, falling back on C.UTF-8
		setlocale( LC_CTYPE, locale ) || setlocale( LC_CTYPE, "C.UTF-8" );
	}

	// Set the initial time base
	Sys_Milliseconds();

	Sys_ParseArgs( argc, argv );
	Sys_SetBinaryPath( Sys_Dirname( argv[ 0 ] ) );

 	// Concatenate the command line for passing to Com_Init
	for ( i = 1; i < argc; i++ )
	{

#ifdef USE_CURSES
		if ( !strcmp( "+nocurses", argv[ i ] ) )
		{
			nocurses = qtrue;
			continue;
		}
#endif

		// Allow URIs to be passed without +connect
		if ( !Q_strnicmp( argv[ i ], URI_SCHEME, URI_SCHEME_LENGTH ) && Q_strnicmp( argv[ i - 1 ], "+connect", 8 ) )
		{
			Q_strcat( commandLine, sizeof( commandLine ), "+connect " );
		}

		Q_strcat( commandLine, sizeof( commandLine ), Cmd_QuoteString( argv[ i ] ) );
		Q_strcat( commandLine, sizeof( commandLine ), " " );
	}

#ifdef USE_CURSES

	if ( nocurses )
	{
		CON_Init_tty();
	}
	else
	{
		CON_Init();
	}

#else
	CON_Init();
#endif

	Com_Init( commandLine );
	NET_Init();

#ifdef NDEBUG
	signal( SIGILL, Sys_SigHandler );
	signal( SIGFPE, Sys_SigHandler );
	signal( SIGSEGV, Sys_SigHandler );
#endif
	signal( SIGTERM, Sys_SigHandler );
	signal( SIGINT, Sys_SigHandler );

	while ( 1 )
	{
		IN_Frame();
		Com_Frame();
	}
}
