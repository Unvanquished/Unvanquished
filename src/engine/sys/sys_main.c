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

#include <CPUInfo.h>
// the CPUInfo.h implemntation of lengthof is unsafe, so use the one
// from q_shared.h
#undef lengthof

#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef OPENMP
#include <omp.h>
#endif

#ifndef DEDICATED
#ifdef USE_LOCAL_HEADERS
#include "SDL.h"
#else
#include <SDL.h>
#endif
#endif

#include "sys_local.h"
#include "sys_loadlib.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

static char binaryPath[ MAX_OSPATH ] = { 0 };
static char installPath[ MAX_OSPATH ] = { 0 };
static char libPath[ MAX_OSPATH ] = { 0 };

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
Sys_BinaryPath
=================
*/
char *Sys_BinaryPath( void )
{
	return binaryPath;
}

/*
=================
Sys_SetDefaultInstallPath
=================
*/
void Sys_SetDefaultInstallPath( const char *path )
{
	Q_strncpyz( installPath, path, sizeof( installPath ) );
}

/*
=================
Sys_DefaultInstallPath
=================
*/
char *Sys_DefaultInstallPath( void )
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
Sys_SetDefaultLibPath
=================
*/
void Sys_SetDefaultLibPath( const char *path )
{
	Q_strncpyz( libPath, path, sizeof( libPath ) );
}

/*
=================
Sys_DefaultLibPath
=================
*/
char *Sys_DefaultLibPath( void )
{
	if ( *libPath )
	{
		return libPath;
	}
	else
	{
		return Sys_Cwd();
	}
}


/*
=================
Sys_DefaultAppPath
=================
*/
char *Sys_DefaultAppPath( void )
{
	return Sys_BinaryPath();
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

#ifdef DEDICATED
#       define PID_FILENAME PRODUCT_NAME_UPPPER "_server.pid"
#else
#       define PID_FILENAME PRODUCT_NAME_UPPPER ".pid"
#endif

/*
=================
Sys_PIDFileName
=================
*/
static char *Sys_PIDFileName( void )
{
	return va( "%s/%s", Sys_TempPath(), PID_FILENAME );
}

/*
=================
Sys_WritePIDFile

Return qtrue if there is an existing stale PID file
=================
*/
qboolean Sys_WritePIDFile( void )
{
	char     *pidFile = Sys_PIDFileName();
	FILE     *f;
	qboolean stale = qfalse;

	// First, check if the pid file is already there
	if ( ( f = fopen( pidFile, "r" ) ) != NULL )
	{
		char pidBuffer[ 64 ] = { 0 };
		int  pid;

		fread( pidBuffer, sizeof( char ), sizeof( pidBuffer ) - 1, f );
		fclose( f );

		pid = atoi( pidBuffer );

		if ( !Sys_PIDIsRunning( pid ) )
		{
			stale = qtrue;
		}
	}

	if ( ( f = fopen( pidFile, "w" ) ) != NULL )
	{
		fprintf( f, "%d", Sys_PID() );
		fclose( f );
	}
	else
	{
		Com_Printf( S_COLOR_YELLOW "Couldn't write %s.\n", pidFile );
	}

	return stale;
}

/*
=================
Sys_Exit

Single exit point (regular exit or in case of error)
=================
*/
static void Sys_Exit( int exitCode )
{
	CON_Shutdown();

#ifndef DEDICATED
	SDL_Quit();
#endif

	if ( exitCode < 2 )
	{
		// Normal exit
		remove( Sys_PIDFileName() );
	}

	exit( exitCode );
}

/*
=================
Sys_Quit
=================
*/
void Sys_Quit( void )
{
	Sys_Exit( 0 );
}

/*
=================
Sys_GetProcessorFeatures
=================
*/
cpuFeatures_t Sys_GetProcessorFeatures( void )
{
#ifdef USE_CPUINFO
	cpuFeatures_t features = 0;
	CPUINFO       cpuinfo;

	GetCPUInfo( &cpuinfo, CI_FALSE );

	if ( HasCPUID( &cpuinfo ) ) { features |= CF_RDTSC; }

	if ( HasMMX( &cpuinfo ) ) { features |= CF_MMX; }

	if ( HasMMXExt( &cpuinfo ) ) { features |= CF_MMX_EXT; }

	if ( Has3DNow( &cpuinfo ) ) { features |= CF_3DNOW; }

	if ( Has3DNowExt( &cpuinfo ) ) { features |= CF_3DNOW_EXT; }

	if ( HasSSE( &cpuinfo ) ) { features |= CF_SSE; }

	if ( HasSSE2( &cpuinfo ) ) { features |= CF_SSE2; }

	if ( HasSSE3( &cpuinfo ) ) { features |= CF_SSE3; }

	if ( HasSSSE3( &cpuinfo ) ) { features |= CF_SSSE3; }

	if ( HasSSE4_1( &cpuinfo ) ) { features |= CF_SSE4_1; }

	if ( HasSSE4_2( &cpuinfo ) ) { features |= CF_SSE4_2; }

	if ( HasHTT( &cpuinfo ) ) { features |= CF_HasHTT; }

	if ( HasSerial( &cpuinfo ) ) { features |= CF_HasSerial; }

	if ( Is64Bit( &cpuinfo ) ) { features |= CF_Is64Bit; }

	return features;
#else
	return 0;
#endif
}

/*
=================
Sys_Init
=================
*/
void Sys_Init( void )
{
	Cmd_AddCommand( "in_restart", Sys_In_Restart_f );
	Cvar_Set( "arch", OS_STRING " " ARCH_STRING );
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
	static int  q3ToAnsi[ 8 ] =
	{
		30, // COLOR_BLACK
		31, // COLOR_RED
		32, // COLOR_GREEN
		33, // COLOR_YELLOW
		34, // COLOR_BLUE
		36, // COLOR_CYAN
		35, // COLOR_MAGENTA
		0 // COLOR_WHITE
	};

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
				fputs( "\033[0m\n", stderr );
				msg++;
			}
			else
			{
				// Print the color code
				Com_sprintf( buffer, sizeof( buffer ), "\033[%dm",
				             q3ToAnsi[ ColorIndex( * ( msg + 1 ) ) ] );
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
void Sys_Error( const char *error, ... )
{
#if defined ( IPHONE )
	NSString *errorString;
	va_list  ap;

	va_start( ap, error );
errorString = [[[ NSString alloc ] initWithFormat: [ NSString stringWithCString: error encoding: NSUTF8StringEncoding ]
                arguments: ap ] autorelease ];
	va_end( ap );
#ifdef IPHONE_USE_THREADS
[[ OWApplication sharedApplication ] performSelectorOnMainThread : @selector( presentErrorMessage: )
 withObject : errorString
 waitUntilDone : YES ];
#else
[( OWApplication * ) [ OWApplication sharedApplication ] presentErrorMessage : errorString ];
#endif // IPHONE_USE_THREADS
#else
	va_list argptr;
	char    string[ 1024 ];

	va_start( argptr, error );
	Q_vsnprintf( string, sizeof( string ), error, argptr );
	va_end( argptr );

	// Print text in the console window/box
	Sys_Print( string );
	Sys_Print( "\n" );

	CL_Shutdown();
	Sys_ErrorDialog( string );

	Sys_Exit( 3 );
#endif
}

/*
=================
Sys_Warn
=================
*/
void __attribute__( ( format( printf, 1, 2 ) ) ) Sys_Warn( char *warning, ... )
{
#if defined ( IPHONE )
	NSString *warningString;
	va_list  ap;

	va_start( ap, warning );
warningString = [[[ NSString alloc ] initWithFormat: [ NSString stringWithCString: warning encoding: NSUTF8StringEncoding ]
                  arguments: ap ] autorelease ];
	va_end( ap );
#ifdef IPHONE_USE_THREADS
[[ OWApplication sharedApplication ] performSelectorOnMainThread : @selector( presentWarningMessage: )
 withObject : warningString
 waitUntilDone : YES ];
#else
[( OWApplication * ) [ OWApplication sharedApplication ] presentWarningMessage : warningString ];
#endif // IPHONE_USE_THREADS
#else
	va_list argptr;
	char    string[ 1024 ];

	va_start( argptr, warning );
	Q_vsnprintf( string, sizeof( string ), warning, argptr );
	va_end( argptr );

	CON_Print( va( "Warning: %s", string ) );
#endif
}

#if defined ( IPHONE )
void applicationDidFinishLaunching( id unused )
{
[[ UIApplication sharedApplication ] setStatusBarOrientation : UIInterfaceOrientationLandscapeLeft ];
}

#endif

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
Sys_GetDLLName

Used to load a development dll instead of a virtual machine
=================
*/
extern int cl_connectedToPureServer;

char *Sys_GetDLLName( const char *name )
{
#if defined _WIN32
	return va( "%s_mp_" ARCH_STRING DLL_EXT, name );
#else
	return va( "%s.mp." ARCH_STRING DLL_EXT, name );
#endif
}

/*
=================
Sys_TryLibraryLoad
=================
*/
static void *Sys_TryLibraryLoad( const char *base, const char *gamedir, const char *fname, char *fqpath )
{
	void *libHandle = NULL;
	char *fn;

	*fqpath = 0;

	fn = FS_BuildOSPath( base, gamedir, fname );
	Com_Printf( "Sys_LoadDll(%s)... \n", fn );

	libHandle = Sys_LoadLibrary( fn );

	if ( !libHandle )
	{
		Com_Printf( "Sys_LoadDll(%s) failed:\n\"%s\"\n", fn, Sys_LibraryError() );
		return NULL;
	}

	Com_DPrintf( "Sys_LoadDll(%s): succeeded ...\n", fn );
	Q_strncpyz( fqpath, fn, MAX_QPATH );

	return libHandle;
}

/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine
#1 look in fs_homepath
#2 look in fs_basepath
#4 look in fs_libpath (if not "")
=================
*/
void *QDECL Sys_LoadDll( const char *name, char *fqpath,
                         intptr_t ( QDECL  * *entryPoint )( int, ... ),
                         intptr_t ( QDECL *systemcalls )( intptr_t, ... ) )
{
	void *libHandle = NULL;
	void ( QDECL * dllEntry )( intptr_t ( QDECL * syscallptr )( intptr_t, ... ) );
	char fname[ MAX_QPATH ];
	char *basepath;
	char *homepath;
	char *gamedir;
	char *libpath;

	assert( name );

	Q_strncpyz( fname, Sys_GetDLLName( name ), sizeof( fname ) );

	// TODO: use fs_searchpaths from files.c
	basepath = Cvar_VariableString( "fs_basepath" );
	homepath = Cvar_VariableString( "fs_homepath" );
	gamedir = Cvar_VariableString( "fs_game" );
	libpath = Cvar_VariableString( "fs_libpath" );

#ifndef DEDICATED

	// if the server is pure, extract the dlls from the mp_bin.pk3 so
	// that they can be referenced
	if ( Cvar_VariableValue( "sv_pure" ) && Q_stricmp( name, "qagame" ) )
	{
		FS_CL_ExtractFromPakFile( homepath, gamedir, fname );
	}

#endif

#ifdef NO_UNTRUSTED_PLUGINS
	libHandle = NULL;
#else
	libHandle = Sys_TryLibraryLoad( homepath, gamedir, fname, fqpath );
#endif

	if ( !libHandle && libpath && libpath[0] )
	{
		libHandle = Sys_TryLibraryLoad( libpath, gamedir, fname, fqpath );
	}


	if ( !libHandle && basepath )
	{
		libHandle = Sys_TryLibraryLoad( basepath, gamedir, fname, fqpath );
	}

	if ( !libHandle )
	{
		Com_Printf( "Sys_LoadDll(%s) could not find it\n", fname );
		return NULL;
	}

	if ( !libHandle )
	{
		Com_Printf( "Sys_LoadDll(%s) failed:\n\"%s\"\n", name, Sys_LibraryError() );
		return NULL;
	}

	// Try to load the dllEntry and vmMain function.
	dllEntry = Sys_LoadFunction( libHandle, "dllEntry" );
	*entryPoint = Sys_LoadFunction( libHandle, "vmMain" );

	if ( !*entryPoint || !dllEntry )
	{
#ifndef NDEBUG

		if ( !dllEntry )
		{
			Com_Error( ERR_FATAL, "Sys_LoadDll(%s) failed SDL_LoadFunction(dllEntry):\n\"%s\" !\n", name, Sys_LibraryError() );
		}
		else
		{
			Com_Error( ERR_FATAL, "Sys_LoadDll(%s) failed SDL_LoadFunction(vmMain):\n\"%s\" !\n", name, Sys_LibraryError() );
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

	Com_Printf( "Sys_LoadDll(%s) found vmMain function at %p\n", name, *entryPoint );
	dllEntry( systemcalls );

	Com_Printf( "Sys_LoadDll(%s) succeeded!\n", name );

	// Copy the fname to fqpath.
	Q_strncpyz( fqpath, fname, MAX_QPATH );

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

#ifndef DEFAULT_BASEDIR
#       ifdef MACOS_X
#               define DEFAULT_BASEDIR Sys_StripAppBundle(Sys_BinaryPath())
#       else
#               define DEFAULT_BASEDIR Sys_BinaryPath()
#       endif
#endif

/*
=================
Sys_SigHandler
=================
*/
void Sys_SigHandler( int signal )
{
	static qboolean signalcaught = qfalse;

	if ( signalcaught )
	{
		VM_Forced_Unload_Start();
		fprintf( stderr, "DOUBLE SIGNAL FAULT: Received signal %d, exiting...\n",
		         signal );
	}
	else
	{
		signalcaught = qtrue;
#ifndef DEDICATED
		CL_Shutdown();
#endif
		SV_Shutdown( va( "Received signal %d", signal ) );
		VM_Forced_Unload_Done();
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

typedef struct
{
	fileHandle_t file;
	byte         *buffer;
	qboolean     eof;
	qboolean     active;
	int          bufferSize;
	int          streamPosition; // next byte to be returned by Sys_StreamRead
	int          threadPosition; // next byte to be read from file
} streamsIO_t;

/*
=================
main
=================
*/
int main( int argc, char **argv )
{
#if defined ( IPHONE )
	NSAutoreleasePool *pool = [ NSAutoreleasePool new ];

	[[ OWApplication sharedApplication ] setPriority : 1.0 ];

	UIApplicationMain( ac, av, nil, nil );

	[ pool release ];
	return 0;
#else
	int  i;
	char commandLine[ MAX_STRING_CHARS ] = { 0 };

#ifndef DEDICATED
	// Run time
	const SDL_version *ver = SDL_Linked_Version();
#endif

#ifdef OPENMP
	int nthreads, tid, procs, maxt, inpar, dynamic, nested;
#endif

#ifndef DEDICATED
	// SDL version check

	// Compile time
#       if !SDL_VERSION_ATLEAST(MINSDL_MAJOR,MINSDL_MINOR,MINSDL_PATCH)
#               error A more recent version of SDL is required
#       endif

#define MINSDL_VERSION \
  XSTRING(MINSDL_MAJOR) "." \
  XSTRING(MINSDL_MINOR) "." \
  XSTRING(MINSDL_PATCH)

	if ( SDL_VERSIONNUM( ver->major, ver->minor, ver->patch ) <
	     SDL_VERSIONNUM( MINSDL_MAJOR, MINSDL_MINOR, MINSDL_PATCH ) )
	{
		Sys_Dialog( DT_ERROR, va( "SDL version " MINSDL_VERSION " or greater is required, "
		                          "but only version %d.%d.%d was found. You may be able to obtain a more recent copy "
		                          "from http://www.libsdl.org/.", ver->major, ver->minor, ver->patch ), "SDL Library Too Old" );

		Sys_Exit( 1 );
	}

#endif

#ifdef _DEBUG
	Sys_PrintCpuInfo();
	Sys_PrintMemoryInfo();
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

			Com_Printf( "-----------------------------------\n" );
			Com_Printf( "-----------------------------------\n" );

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

	// Set the initial time base
	Sys_Milliseconds();

	Sys_ParseArgs( argc, argv );
	Sys_SetBinaryPath( Sys_Dirname( argv[ 0 ] ) );
	Sys_SetDefaultInstallPath( DEFAULT_BASEDIR );

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
		const qboolean containsSpaces = strchr( argv[ i ], ' ' ) != NULL;

		if ( containsSpaces )
		{
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );
		}

		Q_strcat( commandLine, sizeof( commandLine ), argv[ i ] );

		if ( containsSpaces )
		{
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );
		}

		Q_strcat( commandLine, sizeof( commandLine ), " " );
	}

#ifdef _WIN32
	Sys_InitStreamThread();
#endif

	Com_Init( commandLine );
	NET_Init();
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

	signal( SIGILL, Sys_SigHandler );
	signal( SIGFPE, Sys_SigHandler );
	signal( SIGSEGV, Sys_SigHandler );
	signal( SIGTERM, Sys_SigHandler );
	signal( SIGINT, Sys_SigHandler );

	while ( 1 )
	{
		IN_Frame();
		Com_Frame();
	}

	return 0;
#endif
}
