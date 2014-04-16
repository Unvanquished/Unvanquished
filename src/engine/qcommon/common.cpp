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

// common.c -- misc functions used in client and server

#include "revision.h"
#include "../qcommon/q_shared.h"
#include "q_unicode.h"
#include "qcommon.h"
#include <setjmp.h>

#include "../framework/BaseCommands.h"
#include "../framework/CommandSystem.h"
#include "../framework/ConsoleHistory.h"
#include "../framework/LogSystem.h"
#include "../../common/Cvar.h"
#include "../../common/Log.h"

// htons
#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#ifdef SMP
#include <SDL_mutex.h>
#endif

#define MAX_NUM_ARGVS             50

#define MIN_DEDICATED_COMHUNKMEGS 4
#define MIN_COMHUNKMEGS           256 // JPW NERVE changed this to 42 for MP, was 56 for team arena and 75 for wolfSP
#define DEF_COMHUNKMEGS           512 // RF, increased this, some maps are exceeding 56mb
// JPW NERVE changed this for multiplayer back to 42, 56 for depot/mp_cpdepot, 42 for everything else
#define DEF_COMHUNKMEGS_S         XSTRING(DEF_COMHUNKMEGS)

jmp_buf             abortframe; // an ERR_DROP has occurred, exit the entire frame

static fileHandle_t logfile;
static FILE         *pipefile;

cvar_t              *com_crashed = NULL; // ydnar: set in case of a crash, prevents CVAR_UNSAFE variables from being set from a cfg

cvar_t *com_pid; // bani - process id

cvar_t *com_viewlog;
cvar_t *com_speeds;
cvar_t *com_developer;
cvar_t *com_dedicated;
cvar_t *com_timescale;
cvar_t *com_dropsim; // 0.0 to 1.0, simulated packet drops
cvar_t *com_timedemo;
cvar_t *com_sv_running;
cvar_t *com_cl_running;
cvar_t *com_logfile; // 1 = buffer log, 2 = flush after each print, 3 = append + flush
cvar_t *com_version;

cvar_t *com_ansiColor;

cvar_t *com_consoleCommand;

cvar_t *com_unfocused;
cvar_t *com_minimized;


cvar_t *com_introPlayed;
cvar_t *com_logosPlaying;
cvar_t *cl_paused;
cvar_t *sv_paused;

#if defined( _WIN32 ) && !defined( NDEBUG )
cvar_t *com_noErrorInterrupt;
#endif
cvar_t *com_recommendedSet;

cvar_t *com_hunkused; // Ridah

// com_speeds times
int      time_game;
int      time_frontend; // renderer frontend time
int      time_backend; // renderer backend time

int      com_frameTime;
int      com_frameMsec;
int      com_frameNumber;
int      com_expectedhunkusage;
int      com_hunkusedvalue;

qboolean com_errorEntered;
qboolean com_fullyInitialized;

char     com_errorMessage[ MAXPRINTMSG ];

void     Com_WriteConfig_f( void );
void     Com_WriteBindings_f( void );
void     CIN_CloseAllVideos( void );

//============================================================================

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the appropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
int QDECL VPRINTF_LIKE(1) Com_VPrintf( const char *fmt, va_list argptr )
{
#ifdef SMP
	static SDL_mutex *lock = NULL;

	// would be racy, but this gets called prior to renderer threads etc. being started
	if ( !lock )
	{
		lock = SDL_CreateMutex();
	}

	SDL_LockMutex( lock );
#endif

	//Build the message
	char msg[MAXPRINTMSG];
	memset( msg, 0, sizeof( msg ) );
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	msg[ MAXPRINTMSG - 1 ] = '\0';

	//Remove a trailing newline, this is handled by Log::*
	int len = strlen(msg);
	if (msg[len - 1] == '\n')
	{
		msg[len - 1] = '\0';
	}

	Cmd::GetEnv()->Print( msg );

#ifdef SMP
	SDL_UnlockMutex( lock );
#endif
	return strlen( msg );
}

void QDECL PRINTF_LIKE(1) Com_Printf( const char *fmt, ... )
{
	va_list argptr;

	va_start( argptr, fmt );
	Com_VPrintf( fmt, argptr );
	va_end( argptr );
}

void QDECL Com_LogEvent( log_event_t *event, log_location_info_t *location )
{
	switch (event->level)
	{
	case LOG_OFF:
		return;
	case LOG_WARN:
		Com_Printf(_("^3Warning: ^7%s\n"), event->message);
		break;
	case LOG_ERROR:
		Com_Printf(_("^1Error: ^7%s\n"), event->message);
		break;
	case LOG_DEBUG:
		Com_Printf(_("Debug: %s\n"), event->message);
		break;
	case LOG_TRACE:
		Com_Printf("Trace: %s\n", event->message);
		return;
	default:
		Com_Printf("%s\n", event->message);
		break;
	}
#ifndef NDEBUG
	if (location)
	{
		Com_Printf("\tin %s at %s:%i\n", location->function, location->file, location->line);
	}
#endif
}

void QDECL PRINTF_LIKE(2) Com_Logf( log_level_t level, const char *fmt, ... )
{
	va_list argptr;
	char    text[ MAXPRINTMSG ];
	log_event_t event;

	event.level = level;
	event.message = text;

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	Com_LogEvent( &event, NULL );
}

void QDECL Com_Log( log_level_t level, const char* message )
{
	log_event_t event;
	event.level = level;
	event.message = message;
	Com_LogEvent( &event, NULL );
}

/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
================
*/
void QDECL PRINTF_LIKE(1) Com_DPrintf( const char *fmt, ... )
{
	va_list argptr;
	char    msg[ MAXPRINTMSG ];

	if ( !com_developer || com_developer->integer != 1 )
	{
		return; // don't confuse non-developers with techie stuff...
	}

	va_start( argptr, fmt );
	Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );

	Com_Printf( "%s", msg );
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the appropriate things.
=============
*/
// *INDENT-OFF*
void QDECL PRINTF_LIKE(2) NORETURN Com_Error( int code, const char *fmt, ... )
{
	va_list    argptr;
	static int lastErrorTime;
	static int errorCount;
	int        currentTime;

	// make sure we can get at our local stuff
	if (code != ERR_FATAL) {
		FS::PakPath::ClearPaks();
		FS_LoadBasePak();
#ifndef DEDICATED
		// Load map pk3s to allow menus to load levelshots
		FS_LoadAllMaps();
#endif
	}

	// if we are getting a solid stream of ERR_DROP, do an ERR_FATAL
	currentTime = Sys_Milliseconds();

	if ( currentTime - lastErrorTime < 100 )
	{
		if ( ++errorCount > 3 )
		{
			code = ERR_FATAL;
		}
	}
	else
	{
		errorCount = 0;
	}

	lastErrorTime = currentTime;

	if ( com_errorEntered )
	{
		char buf[ 4096 ];

		va_start( argptr, fmt );
		Q_vsnprintf( buf, sizeof( buf ), fmt, argptr );
		va_end( argptr );

		Sys_Error( "recursive error '%s' after: %s", buf, com_errorMessage );
	}

	com_errorEntered = qtrue;

	va_start( argptr, fmt );
	Q_vsnprintf( com_errorMessage, sizeof( com_errorMessage ), fmt, argptr );
	va_end( argptr );

	Cvar_Set("com_errorMessage", com_errorMessage);

	if ( code == ERR_SERVERDISCONNECT )
	{
		Com_Printf( S_COLOR_WHITE "%s\n", com_errorMessage );
		SV_Shutdown( "Server disconnected" );
		CL_Disconnect( qtrue );
		CL_FlushMemory();
		com_errorEntered = qfalse;
		longjmp( abortframe, -1 );
	}
	else if ( code == ERR_DROP )
	{
		Com_Printf( S_COLOR_ORANGE "%s\n", com_errorMessage );
		SV_Shutdown( va( "********************\nServer crashed: %s\n********************\n", com_errorMessage ) );
		CL_Disconnect( qtrue );
		CL_FlushMemory();
		com_errorEntered = qfalse;
		longjmp( abortframe, -1 );
	}
	else
	{
		CL_Shutdown();
		SV_Shutdown( va( "Server fatal crashed: %s\n", com_errorMessage ) );
	}

	Com_Shutdown();

	Sys_Error( "%s", com_errorMessage );
}

// *INDENT-OFF*
//bani - moved
void CL_ShutdownCGame( void );

// *INDENT-ON*

/*
=============
Com_Quit_f

Both client and server can use this, and it will
do the appropriate things.
=============
*/
void NORETURN Com_Quit_f( void )
{
	// don't try to shutdown if we are in a recursive error
	char *p = Cmd_Args();

	if ( !com_errorEntered )
	{
		// Some VMs might execute "quit" command directly,
		// which would trigger an unload of active VM error.
		// Sys_Quit will kill this process anyways, so
		// a corrupt call stack makes no difference
		SV_Shutdown( p[ 0 ] ? p : "Server quit\n" );
//bani
#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)
		CL_ShutdownCGame();
#endif
		CL_Shutdown();
		Com_Shutdown();
	}

	Sys_Quit();
}

/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters separate the commandLine string into multiple console
command lines.

All of these are valid:

quake3 +set test blah +map test
quake3 set test blah+map test
quake3 set test blah + map test

============================================================================
*/

#define MAX_CONSOLE_LINES 32
int  com_numConsoleLines;
char *com_consoleLines[ MAX_CONSOLE_LINES ];

/*
==================
Com_ParseCommandLine

Break it up into multiple console lines
==================
*/
void Com_ParseCommandLine( char *commandLine )
{
	com_consoleLines[ 0 ] = commandLine;
	com_numConsoleLines = 1;

	while ( *commandLine )
	{
		// look for a + separating character
		// if commandLine came from a file, we might have real line separators
		if ( *commandLine == '+' || *commandLine == '\n' || *commandLine == '\r' )
		{
			if ( com_numConsoleLines == MAX_CONSOLE_LINES )
			{
				return;
			}

			com_consoleLines[ com_numConsoleLines ] = commandLine + 1;
			com_numConsoleLines++;
			*commandLine = 0;
		}

		commandLine++;
	}
}

/*
===================
Com_SafeMode

Check for "safe" on the command line, which will
skip loading of wolfconfig.cfg
===================
*/
qboolean Com_SafeMode( void )
{
	int i;

	for ( i = 0; i < com_numConsoleLines; i++ )
	{
		Cmd::Args line(com_consoleLines[i]);

		if ( line.size() > 1 && ( !Q_stricmp( line[0].c_str(), "safe" ) || !Q_stricmp( line[0].c_str(), "cvar_restart" ) ) )
		{
			com_consoleLines[ i ][ 0 ] = 0;
			return qtrue;
		}
	}

	return qfalse;
}

/*
===============
Com_StartupVariable

Searches for command-line arguments that are set commands.
If match is not NULL, only that cvar will be looked for.
That is necessary because the fs_* cvars need to be set
before the filesystem is started, but all other sets should
be after execing the config and default.
===============
*/
void Com_StartupVariable( const char *match )
{
	int    i;
	const char   *s;
	cvar_t *cv;

	for ( i = 0; i < com_numConsoleLines; i++ )
	{
		Cmd::Args line(com_consoleLines[i]);

		if ( line.size() < 3 || strcmp( line[0].c_str(), "set" ))
		{
			continue;
		}

		s = line[1].c_str();

		if ( !match || !strcmp( s, match ) )
		{
			Cvar_Set( s, line[2].c_str() );
			cv = Cvar_Get( s, "", 0 );
			cv->flags |= CVAR_USER_CREATED;
			if (cv->flags & CVAR_ROM) {
				com_consoleLines[i] = 0;
			}
		}
	}
}

/*
=================
Com_AddStartupCommands

Adds command-line arguments as script statements
Commands are separated by + signs

Returns qtrue if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
qboolean Com_AddStartupCommands( void )
{
	int      i;
	qboolean added;

	added = qfalse;

	// quote every token, so args with semicolons can work
	for ( i = 0; i < com_numConsoleLines; i++ )
	{
		if ( !com_consoleLines[ i ] || !com_consoleLines[ i ][ 0 ] )
		{
			continue;
		}

		// set commands won't override menu startup
		if ( Q_strnicmp( com_consoleLines[ i ], "set", 3 ) )
		{
			added = qtrue;
		}

		Cmd::BufferCommandText(com_consoleLines[i], true);
	}

	return added;
}

//============================================================================

void Info_Print( const char *s )
{
	char key[ 512 ];
	char value[ 512 ];
	char *o;
	int  l;

	if ( *s == '\\' )
	{
		s++;
	}

	while ( *s )
	{
		o = key;

		while ( *s && *s != '\\' )
		{
			*o++ = *s++;
		}

		l = o - key;

		if ( l < 20 )
		{
			memset( o, ' ', 20 - l );
			key[ 20 ] = 0;
		}
		else
		{
			*o = 0;
		}

		Com_Printf( "%s", key );

		if ( !*s )
		{
			Com_Printf( "MISSING VALUE\n" );
			return;
		}

		o = value;
		s++;

		while ( *s && *s != '\\' )
		{
			*o++ = *s++;
		}

		*o = 0;

		if ( *s )
		{
			s++;
		}

		Com_Printf( "%s\n", value );
	}
}

/* Internals for Com_RealTime & Com_GMTime */
static int internalTime( qtime_t *qtime, struct tm *( *timefunc )( const time_t * ) )
{
	time_t    t;
	struct tm *tms;

	t = time( NULL );

	if ( !qtime )
	{
		return t;
	}

	tms = timefunc( &t );

	if ( tms )
	{
		qtime->tm_sec = tms->tm_sec;
		qtime->tm_min = tms->tm_min;
		qtime->tm_hour = tms->tm_hour;
		qtime->tm_mday = tms->tm_mday;
		qtime->tm_mon = tms->tm_mon;
		qtime->tm_year = tms->tm_year;
		qtime->tm_wday = tms->tm_wday;
		qtime->tm_yday = tms->tm_yday;
		qtime->tm_isdst = tms->tm_isdst;
	}

	return t;
}

/*
================
Com_RealTime
================
*/
int Com_RealTime( qtime_t *qtime )
{
	return internalTime( qtime, localtime );
}

/*
================
Com_GMTime
================
*/
int Com_GMTime( qtime_t *qtime )
{
	return internalTime( qtime, gmtime );
}

/*
==============================================================================

Goals:
        reproducable without history effects -- no out of memory errors on weird map to map changes
        allow restarting of the client without fragmentation
        minimize total pages in use at run time
        minimize total pages needed during load time

  Single block of memory with stack allocators coming from both ends towards the middle.

  One side is designated the temporary memory allocator.

  Temporary memory can be allocated and freed in any order.

  A highwater mark is kept of the most in use at any time.

  When there is no temporary memory allocated, the permanent and temp sides
  can be switched, allowing the already touched temp memory to be used for
  permanent storage.

  Temp memory must never be allocated on two ends at once, or fragmentation
  could occur.

  If we have any in-use temp memory, additional temp allocations must come from
  that side.

  If not, we can choose to make either side the new temp side and push future
  permanent allocations to the other side.  Permanent allocations should be
  kept on the side that has the current greatest wasted highwater mark.

==============================================================================
*/

#define HUNK_MAGIC      0x89537892
#define HUNK_FREE_MAGIC 0x89537893

typedef struct
{
	int magic;
	int size;
} hunkHeader_t;

typedef struct
{
	int mark;
	int permanent;
	int temp;
	int tempHighwater;
} hunkUsed_t;

typedef struct hunkblock_s
{
	int                size;
	byte               printed;
	struct hunkblock_s *next;

	const char         *label;
	const char         *file;
	int                line;
} hunkblock_t;
// for alignment purposes
#define SIZEOF_HUNKBLOCK_T ( ( sizeof( hunkblock_t ) + 31 ) & ~31 )

static hunkblock_t *hunkblocks;

static hunkUsed_t  hunk_low, hunk_high;
static hunkUsed_t  *hunk_permanent, *hunk_temp;

static byte        *s_hunkData = NULL;
static int         s_hunkTotal;

/*
=================
Com_Meminfo_f
=================
*/
void Com_Meminfo_f( void )
{
	Com_Printf( "%9i bytes (%6.2f MB) total hunk\n", s_hunkTotal, s_hunkTotal / Square( 1024.f ) );
	Com_Printf( "\n" );
	Com_Printf( "%9i bytes (%6.2f MB) low mark\n", hunk_low.mark, hunk_low.mark / Square( 1024.f ) );
	Com_Printf( "%9i bytes (%6.2f MB) low permanent\n", hunk_low.permanent, hunk_low.permanent / Square( 1024.f ) );

	if ( hunk_low.temp != hunk_low.permanent )
	{
		Com_Printf( "%9i bytes (%6.2f MB) low temp\n", hunk_low.temp, hunk_low.temp / Square( 1024.f ) );
	}

	Com_Printf( "%9i bytes (%6.2f MB) low tempHighwater\n", hunk_low.tempHighwater, hunk_low.tempHighwater / Square( 1024.f ) );
	Com_Printf( "\n" );
	Com_Printf( "%9i bytes (%6.2f MB) high mark\n", hunk_high.mark, hunk_high.mark / Square( 1024.f ) );
	Com_Printf( "%9i bytes (%6.2f MB) high permanent\n", hunk_high.permanent, hunk_high.permanent / Square( 1024.f ) );

	if ( hunk_high.temp != hunk_high.permanent )
	{
		Com_Printf( "%9i bytes (%6.2f MB) high temp\n", hunk_high.temp, hunk_high.temp / Square( 1024.f ) );
	}

	Com_Printf( "%9i bytes (%6.2f MB) high tempHighwater\n", hunk_high.tempHighwater, hunk_high.tempHighwater / Square( 1024.f ) );
	Com_Printf( "\n" );
	Com_Printf( "%9i bytes (%6.2f MB) total hunk in use\n", hunk_low.permanent + hunk_high.permanent,
	            ( hunk_low.permanent + hunk_high.permanent ) / Square( 1024.f ) );
	int unused = 0;

	if ( hunk_low.tempHighwater > hunk_low.permanent )
	{
		unused += hunk_low.tempHighwater - hunk_low.permanent;
	}

	if ( hunk_high.tempHighwater > hunk_high.permanent )
	{
		unused += hunk_high.tempHighwater - hunk_high.permanent;
	}

	Com_Printf( "%9i bytes (%6.2f MB) unused highwater\n", unused, unused / Square( 1024.f ) );
}

/*
===============
Com_TouchMemory

Touch all known used data to make sure it is paged in
===============
*/
void Com_TouchMemory( void )
{
	int        start, end;
	int        i, j;
	int        sum;
	start = Sys_Milliseconds();

	sum = 0;

	j = hunk_low.permanent >> 2;

	for ( i = 0; i < j; i += 64 )
	{
		// only need to touch each page
		sum += ( ( int * ) s_hunkData ) [ i ];
	}

	i = ( s_hunkTotal - hunk_high.permanent ) >> 2;
	j = hunk_high.permanent >> 2;

	for ( ; i < j; i += 64 )
	{
		// only need to touch each page
		sum += ( ( int * ) s_hunkData ) [ i ];
	}

	end = Sys_Milliseconds();

	Com_DPrintf( "Com_TouchMemory: %i msec\n", end - start );
}

/*
=================
Hunk_Log
=================
*/
void Hunk_Log( void )
{
	hunkblock_t *block;
	char        buf[ 4096 ];
	int         size, numBlocks;

	if ( !logfile || !FS::IsInitialized() )
	{
		return;
	}

	size = 0;
	numBlocks = 0;
	Com_sprintf( buf, sizeof( buf ), "\r\n================\r\nHunk log\r\n================\r\n" );
	FS_Write( buf, strlen( buf ), logfile );

	for ( block = hunkblocks; block; block = block->next )
	{
#ifdef HUNK_DEBUG
		Com_sprintf( buf, sizeof( buf ), "size = %8d: %s, line: %d (%s)\r\n", block->size, block->file, block->line, block->label );
		FS_Write( buf, strlen( buf ), logfile );
#endif
		size += block->size;
		numBlocks++;
	}

	Com_sprintf( buf, sizeof( buf ), "%d Hunk memory\r\n", size );
	FS_Write( buf, strlen( buf ), logfile );
	Com_sprintf( buf, sizeof( buf ), "%d hunk blocks\r\n", numBlocks );
	FS_Write( buf, strlen( buf ), logfile );
}

/*
=================
Hunk_SmallLog
=================
*/
void Hunk_SmallLog( void )
{
	hunkblock_t *block, *block2;
	char        buf[ 4096 ];
	int         size, locsize, numBlocks;

	if ( !logfile || !FS::IsInitialized() )
	{
		return;
	}

	for ( block = hunkblocks; block; block = block->next )
	{
		block->printed = qfalse;
	}

	size = 0;
	numBlocks = 0;
	Com_sprintf( buf, sizeof( buf ), "\r\n================\r\nHunk Small log\r\n================\r\n" );
	FS_Write( buf, strlen( buf ), logfile );

	for ( block = hunkblocks; block; block = block->next )
	{
		if ( block->printed )
		{
			continue;
		}

		locsize = block->size;

		for ( block2 = block->next; block2; block2 = block2->next )
		{
			if ( block->line != block2->line )
			{
				continue;
			}

			if ( Q_stricmp( block->file, block2->file ) )
			{
				continue;
			}

			size += block2->size;
			locsize += block2->size;
			block2->printed = qtrue;
		}

#ifdef HUNK_DEBUG
		Com_sprintf( buf, sizeof( buf ), "size = %8d (%6.2f MB / %6.2f MB): %s, line: %d (%s)\r\n", locsize,
		             locsize / Square( 1024.f ), ( size + block->size ) / Square( 1024.f ), block->file, block->line, block->label );
		FS_Write( buf, strlen( buf ), logfile );
#endif
		size += block->size;
		numBlocks++;
	}

	Com_sprintf( buf, sizeof( buf ), "%d Hunk memory\r\n", size );
	FS_Write( buf, strlen( buf ), logfile );
	Com_sprintf( buf, sizeof( buf ), "%d hunk blocks\r\n", numBlocks );
	FS_Write( buf, strlen( buf ), logfile );
}

/*
=================
Com_InitHunkMemory
=================
*/
void Com_InitHunkMemory( void )
{
	cvar_t *cv;
	int    nMinAlloc;
	qboolean isDedicated;

	isDedicated = (com_dedicated && com_dedicated->integer);

	// allocate the stack based hunk allocator
	cv = Cvar_Get( "com_hunkMegs", DEF_COMHUNKMEGS_S, CVAR_LATCH  );

	// if we are not dedicated min allocation is 56, otherwise min is 1
	nMinAlloc = isDedicated ? MIN_DEDICATED_COMHUNKMEGS : MIN_COMHUNKMEGS;

	if ( cv->integer < nMinAlloc )
	{
		s_hunkTotal = 1024 * 1024 * nMinAlloc;
		Com_Printf(	isDedicated	? "Minimum com_hunkMegs for a dedicated server is %i, allocating %iMB.\n"
				: "Minimum com_hunkMegs is %i, allocating %iMB.\n", nMinAlloc, s_hunkTotal / ( 1024 * 1024 ) );
	}
	else
	{
		s_hunkTotal = cv->integer * 1024 * 1024;
	}

	s_hunkData = ( byte * ) malloc( s_hunkTotal + 31 );

	if ( !s_hunkData )
	{
		Com_Error( ERR_FATAL, "Hunk data failed to allocate %iMB", s_hunkTotal / ( 1024 * 1024 ) );
	}

	// cacheline align
	s_hunkData = ( byte * )( ( ( intptr_t ) s_hunkData + 31 ) & ~31 );
	Hunk_Clear();

	Cmd_AddCommand( "meminfo", Com_Meminfo_f );
#ifdef HUNK_DEBUG
	Cmd_AddCommand( "hunklog", Hunk_Log );
	Cmd_AddCommand( "hunksmalllog", Hunk_SmallLog );
#endif
}

/*
====================
Hunk_MemoryRemaining
====================
*/
int Hunk_MemoryRemaining( void )
{
	int low, high;

	low = hunk_low.permanent > hunk_low.temp ? hunk_low.permanent : hunk_low.temp;
	high = hunk_high.permanent > hunk_high.temp ? hunk_high.permanent : hunk_high.temp;

	return s_hunkTotal - ( low + high );
}

/*
===================
Hunk_SetMark

The server calls this after the level and game VM have been loaded
===================
*/
void Hunk_SetMark( void )
{
	hunk_low.mark = hunk_low.permanent;
	hunk_high.mark = hunk_high.permanent;
}

/*
=================
Hunk_ClearToMark

The client calls this before starting a vid_restart or snd_restart
=================
*/
void Hunk_ClearToMark( void )
{
	hunk_low.permanent = hunk_low.temp = hunk_low.mark;
	hunk_high.permanent = hunk_high.temp = hunk_high.mark;
}

void CL_ShutdownUI( void );
void SV_ShutdownGameProgs( void );

/*
=================
Hunk_Clear

The server calls this before shutting down or loading a new map
=================
*/
void Hunk_Clear( void )
{
#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)
	CL_ShutdownCGame();
	CL_ShutdownUI();
#endif
	SV_ShutdownGameProgs();
#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)
	CIN_CloseAllVideos();
#endif
	hunk_low.mark = 0;
	hunk_low.permanent = 0;
	hunk_low.temp = 0;
	hunk_low.tempHighwater = 0;

	hunk_high.mark = 0;
	hunk_high.permanent = 0;
	hunk_high.temp = 0;
	hunk_high.tempHighwater = 0;

	hunk_permanent = &hunk_low;
	hunk_temp = &hunk_high;

	Cvar_Set( "com_hunkused", va( "%i", hunk_low.permanent + hunk_high.permanent ) );
	com_hunkusedvalue = hunk_low.permanent + hunk_high.permanent;

	Com_DPrintf( "Hunk_Clear: reset the hunk ok\n" );
#ifdef HUNK_DEBUG
	hunkblocks = NULL;
#endif
}

static void Hunk_SwapBanks( void )
{
	hunkUsed_t *swap;

	// can't swap banks if there is any temp already allocated
	if ( hunk_temp->temp != hunk_temp->permanent )
	{
		return;
	}

	// if we have a larger highwater mark on this side, start making
	// our permanent allocations here and use the other side for temp
	if ( hunk_temp->tempHighwater - hunk_temp->permanent > hunk_permanent->tempHighwater - hunk_permanent->permanent )
	{
		swap = hunk_temp;
		hunk_temp = hunk_permanent;
		hunk_permanent = swap;
	}
}

/*
=================
Hunk_Alloc

Allocate permanent (until the hunk is cleared) memory
=================
*/
#ifdef HUNK_DEBUG
void           *Hunk_AllocDebug( int size, ha_pref preference, const char *label, const char *file, int line )
{
#else
void           *Hunk_Alloc( int size, ha_pref preference )
{
#endif
	void *buf;

	if ( s_hunkData == NULL )
	{
		Com_Error( ERR_FATAL, "Hunk_Alloc: Hunk memory system not initialized" );
	}

	Hunk_SwapBanks();

#ifdef HUNK_DEBUG
	size += SIZEOF_HUNKBLOCK_T;
#endif

	// round to cacheline
	size = ( size + 31 ) & ~31;

	if ( hunk_low.temp + hunk_high.temp + size > s_hunkTotal )
	{
#ifdef HUNK_DEBUG
		Hunk_Log();
		Hunk_SmallLog();

		Com_Error( ERR_DROP, "Hunk_Alloc failed on %i: %s, line: %d (%s)", size, file, line, label );
#else
		Com_Error( ERR_DROP, "Hunk_Alloc failed on %i", size );
#endif
	}

	if ( hunk_permanent == &hunk_low )
	{
		buf = ( void * )( s_hunkData + hunk_permanent->permanent );
		hunk_permanent->permanent += size;
	}
	else
	{
		hunk_permanent->permanent += size;
		buf = ( void * )( s_hunkData + s_hunkTotal - hunk_permanent->permanent );
	}

	hunk_permanent->temp = hunk_permanent->permanent;

	memset( buf, 0, size );

#ifdef HUNK_DEBUG
	{
		hunkblock_t *block;

		block = ( hunkblock_t * ) buf;
		block->size = size - SIZEOF_HUNKBLOCK_T;
		block->file = file;
		block->label = label;
		block->line = line;
		block->next = hunkblocks;
		hunkblocks = block;
		buf = ( ( byte * ) buf ) + SIZEOF_HUNKBLOCK_T;
	}
#endif

	// Ridah, update the com_hunkused cvar in increments, so we don't update it too often, since this cvar call isn't very efficent
	if ( ( hunk_low.permanent + hunk_high.permanent ) > com_hunkused->integer + 2500 )
	{
		Cvar_Set( "com_hunkused", va( "%i", hunk_low.permanent + hunk_high.permanent ) );
	}

	com_hunkusedvalue = hunk_low.permanent + hunk_high.permanent;

	return buf;
}

/*
=================
Hunk_AllocateTempMemory

This is used by the file loading system.
Multiple files can be loaded in temporary memory.
When the files-in-use count reaches zero, all temp memory will be deleted
=================
*/
void           *Hunk_AllocateTempMemory( int size )
{
	void         *buf;
	hunkHeader_t *hdr;

	// return a Z_Malloc'd block if the hunk has not been initialized
	// this allows the config and product id files ( journal files too ) to be loaded
	// by the file system without redundant routines in the file system utilizing different
	// memory systems
	if ( s_hunkData == NULL )
	{
		return Z_Malloc( size );
	}

	Hunk_SwapBanks();

	size = PAD( size, sizeof( intptr_t ) ) + sizeof( hunkHeader_t );

	if ( hunk_temp->temp + hunk_permanent->permanent + size > s_hunkTotal )
	{
#ifdef HUNK_DEBUG
		Hunk_Log();
		Hunk_SmallLog();
#endif
		Com_Error( ERR_DROP, "Hunk_AllocateTempMemory: failed on %i", size );
	}

	if ( hunk_temp == &hunk_low )
	{
		buf = ( void * )( s_hunkData + hunk_temp->temp );
		hunk_temp->temp += size;
	}
	else
	{
		hunk_temp->temp += size;
		buf = ( void * )( s_hunkData + s_hunkTotal - hunk_temp->temp );
	}

	if ( hunk_temp->temp > hunk_temp->tempHighwater )
	{
		hunk_temp->tempHighwater = hunk_temp->temp;
	}

	hdr = ( hunkHeader_t * ) buf;
	buf = ( void * )( hdr + 1 );

	hdr->magic = HUNK_MAGIC;
	hdr->size = size;

	// don't bother clearing, because we are going to load a file over it
	return buf;
}

/*
==================
Hunk_FreeTempMemory
==================
*/
void Hunk_FreeTempMemory( void *buf )
{
	hunkHeader_t *hdr;

	// free with Z_Free if the hunk has not been initialized
	// this allows the config and product id files ( journal files too ) to be loaded
	// by the file system without redundant routines in the file system utilizing different
	// memory systems
	if ( s_hunkData == NULL )
	{
		Z_Free( buf );
		return;
	}

	hdr = ( ( hunkHeader_t * ) buf ) - 1;

	if ( hdr->magic != HUNK_MAGIC )
	{
		Com_Error( ERR_FATAL, "Hunk_FreeTempMemory: bad magic" );
	}

	hdr->magic = HUNK_FREE_MAGIC;

	// this only works if the files are freed in stack order,
	// otherwise the memory will stay around until Hunk_ClearTempMemory
	if ( hunk_temp == &hunk_low )
	{
		if ( hdr == ( void * )( s_hunkData + hunk_temp->temp - hdr->size ) )
		{
			hunk_temp->temp -= hdr->size;
		}
		else
		{
			Com_Printf( "Hunk_FreeTempMemory: not the final block\n" );
		}
	}
	else
	{
		if ( hdr == ( void * )( s_hunkData + s_hunkTotal - hunk_temp->temp ) )
		{
			hunk_temp->temp -= hdr->size;
		}
		else
		{
			Com_Printf( "Hunk_FreeTempMemory: not the final block\n" );
		}
	}
}

/*
===================================================================

EVENTS

===================================================================
*/

/*
========================================================================
EVENT LOOP
========================================================================
*/

#define MAX_QUEUED_EVENTS  256
#define MASK_QUEUED_EVENTS ( MAX_QUEUED_EVENTS - 1 )

static sysEvent_t eventQueue[ MAX_QUEUED_EVENTS ];
static int        eventHead = 0;
static int        eventTail = 0;
static byte       sys_packetReceived[ MAX_MSGLEN ];

/*
================
Com_QueueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Com_QueueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr )
{
	sysEvent_t *ev;

	ev = &eventQueue[ eventHead & MASK_QUEUED_EVENTS ];

	if ( eventHead - eventTail >= MAX_QUEUED_EVENTS )
	{
		Com_Printf( "Com_QueueEvent: overflow\n" );

		// we are discarding an event, but don't leak memory
		if ( ev->evPtr )
		{
			Z_Free( ev->evPtr );
		}

		eventTail++;
	}

	eventHead++;

	if ( time == 0 )
	{
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
================
Com_GetEvent
================
*/
sysEvent_t Com_GetEvent( void )
{
	sysEvent_t ev;
	char       *s;
	msg_t      netmsg;
	netadr_t   adr;

	// return if we have data
	if ( eventHead > eventTail )
	{
		eventTail++;
		return eventQueue[( eventTail - 1 ) & MASK_QUEUED_EVENTS ];
	}

	// check for console commands
	s = Sys_ConsoleInput();

	if ( s )
	{
		char *b;
		int  len;

		len = strlen( s ) + 1;
		b = ( char * ) Z_Malloc( len );
		strcpy( b, s );
		Com_QueueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	adr.type = NA_UNSPEC;

	if ( Sys_GetPacket( &adr, &netmsg ) )
	{
		netadr_t *buf;
		int      len;

		// copy out to a separate buffer for queuing
		len = sizeof( netadr_t ) + netmsg.cursize;
		buf = ( netadr_t * ) Z_Malloc( len );
		*buf = adr;
		memcpy( buf + 1, &netmsg.data[ netmsg.readcount ], netmsg.cursize - netmsg.readcount );
		Com_QueueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}

	// return if we have data
	if ( eventHead > eventTail )
	{
		eventTail++;
		return eventQueue[( eventTail - 1 ) & MASK_QUEUED_EVENTS ];
	}

	// create an empty event to return
	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = Sys_Milliseconds();

	return ev;
}

/*
=================
Com_RunAndTimeServerPacket
=================
*/
void Com_RunAndTimeServerPacket( netadr_t *evFrom, msg_t *buf )
{
	int t1, t2, msec;

	t1 = 0;

	if ( com_speeds->integer )
	{
		t1 = Sys_Milliseconds();
	}

	SV_PacketEvent( *evFrom, buf );

	if ( com_speeds->integer )
	{
		t2 = Sys_Milliseconds();
		msec = t2 - t1;

		if ( com_speeds->integer == 3 )
		{
			Com_Printf( "SV_PacketEvent time: %i\n", msec );
		}
	}
}

/*
=================
Com_EventLoop

Returns last event time
=================
*/

#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)
extern qboolean consoleButtonWasPressed;
#endif

int Com_EventLoop( void )
{
	sysEvent_t ev;
	netadr_t   evFrom;
	byte       bufData[ MAX_MSGLEN ];
	msg_t      buf;

	MSG_Init( &buf, bufData, sizeof( bufData ) );

	while ( 1 )
	{
		ev = Com_GetEvent();

		// if no more events are available
		if ( ev.evType == SE_NONE )
		{
			// manually send packet events for the loopback channel
			while ( NET_GetLoopPacket( NS_CLIENT, &evFrom, &buf ) )
			{
				CL_PacketEvent( evFrom, &buf );
			}

			while ( NET_GetLoopPacket( NS_SERVER, &evFrom, &buf ) )
			{
				// if the server just shut down, flush the events
				if ( com_sv_running->integer )
				{
					Com_RunAndTimeServerPacket( &evFrom, &buf );
				}
			}

			return ev.evTime;
		}

		switch ( ev.evType )
		{
			default:
				// bk001129 - was ev.evTime
				Com_Error( ERR_FATAL, "Com_EventLoop: bad event type %i", ev.evType );

			case SE_NONE:
				break;

			case SE_KEY:
				CL_KeyEvent( ev.evValue, ev.evValue2, ev.evTime );
				break;

			case SE_CHAR:
#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)

				// fretn
				// we just pressed the console button,
				// so ignore this event
				// this prevents chars appearing at console input
				// when you just opened it
				if ( consoleButtonWasPressed )
				{
					consoleButtonWasPressed = qfalse;
					break;
				}

#endif
				CL_CharEvent( ev.evValue );
				break;

			case SE_MOUSE:
				CL_MouseEvent( ev.evValue, ev.evValue2, ev.evTime );
				break;

			case SE_JOYSTICK_AXIS:
				CL_JoystickEvent( ev.evValue, ev.evValue2, ev.evTime );
				break;

			case SE_CONSOLE:
			{
				char *cmd = (char *) ev.evPtr;

				if (cmd[0] == '/' || cmd[0] == '\\')
				{
					//make sure, explicit commands are not getting handled with com_consoleCommand
					Cmd::BufferCommandTextAfter(cmd + 1, true);
				}
				else
				{
					/*
					 * when there was no command prefix, execute the command prefixed by com_consoleCommand
					 * if the cvar is empty, it will interpret the text as command direclty
					 * (and will so for DEDICATED)
					 *
					 * the additional space gets trimmed by the parser
					 */
					Cmd::BufferCommandTextAfter(va("%s %s", com_consoleCommand->string, cmd), true);
				}

				break;
			}

			case SE_PACKET:

				// this cvar allows simulation of connections that
				// drop a lot of packets.  Note that loopback connections
				// don't go through here at all.
				if ( com_dropsim->value > 0 )
				{
					static int seed;

					if ( Q_random( &seed ) < com_dropsim->value )
					{
						break; // drop this packet
					}
				}

				evFrom = * ( netadr_t * ) ev.evPtr;
				buf.cursize = ev.evPtrLength - sizeof( evFrom );

				// we must copy the contents of the message out, because
				// the event buffers are only large enough to hold the
				// exact payload, but channel messages need to be large
				// enough to hold fragment reassembly
				if ( ( unsigned ) buf.cursize > buf.maxsize )
				{
					Com_Printf( "Com_EventLoop: oversize packet\n" );
					continue;
				}

				memcpy( buf.data, ( byte * )( ( netadr_t * ) ev.evPtr + 1 ), buf.cursize );

				if ( com_sv_running->integer )
				{
					Com_RunAndTimeServerPacket( &evFrom, &buf );
				}
				else
				{
					CL_PacketEvent( evFrom, &buf );
				}

				break;
		}

		// free any block data
		if ( ev.evPtr )
		{
			Z_Free( ev.evPtr );
		}
	}
}

/*
================
Com_Milliseconds

Can be used for profiling, but will be journaled accurately
================
*/
int Com_Milliseconds( void )
{
    return Sys_Milliseconds();
}

//============================================================================

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
static void NORETURN Com_Error_f( void )
{
	if ( Cmd_Argc() > 1 )
	{
		Com_Error( ERR_DROP, "Testing drop error" );
	}
	else
	{
		Com_Error( ERR_FATAL, "Testing fatal error" );
	}
}

/*
=============
Com_Freeze_f

Just freeze in place for a given number of seconds to test
error recovery
=============
*/
static void Com_Freeze_f( void )
{
	float s;
	int   start, now;

	if ( Cmd_Argc() != 2 )
	{
		Com_Printf(_( "freeze <seconds>\n" ));
		return;
	}

	s = atof( Cmd_Argv( 1 ) );

	start = Com_Milliseconds();

	while ( 1 )
	{
		now = Com_Milliseconds();

		if ( ( now - start ) * 0.001 > s )
		{
			break;
		}
	}
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
static void NORETURN Com_Crash_f( void )
{
	* ( volatile int * ) 0 = 0x12345678;
	exit( 1 ); // silence warning
}

void Com_SetRecommended( void )
{
	cvar_t   *r_highQualityVideo; //, *com_recommended;
	qboolean goodVideo;

	// will use this for recommended settings as well.. do i outside the lower check so it gets done even with command line stuff
	r_highQualityVideo = Cvar_Get( "r_highQualityVideo", "1", 0 );
	//com_recommended = Cvar_Get("com_recommended", "-1", 0);
	goodVideo = ( r_highQualityVideo && r_highQualityVideo->integer );

	if ( goodVideo )
	{
		Com_Printf(_( "Found high quality video and slow CPU\n" ));
		Cmd::BufferCommandText("preset preset_fast.cfg");
		Cvar_Set( "com_recommended", "2" );
	}
	else
	{
		Com_Printf(_( "Found low quality video and slow CPU\n" ));
		Cmd::BufferCommandText("preset preset_fastest.cfg");
		Cvar_Set( "com_recommended", "3" );
	}
}

/*
=================
Com_Init
=================
*/


#ifndef _WIN32
# ifdef DEDICATED
	const char* defaultPipeFilename = "svpipe";
# else
	const char* defaultPipeFilename = "pipe";
# endif
#else
	const char* defaultPipeFilename = "";
#endif

void Com_Init( char *commandLine )
{
	char              *s;
	int               pid, qport;

	pid = Sys_GetPID();

	Com_Printf( "%s %s %s %s\n%s\n", Q3_VERSION, PLATFORM_STRING, ARCH_STRING, __DATE__, commandLine );

	if ( setjmp( abortframe ) )
	{
		Sys_Error( "Error during initialization" );
	}

	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	Com_ParseCommandLine( commandLine );

	// override anything from the config files with command line args
	Com_StartupVariable( NULL );

	// get the developer cvar set as early as possible
	Com_StartupVariable( "developer" );

	// bani: init this early
	Com_StartupVariable( "com_ignorecrash" );

	// ydnar: init crashed variable as early as possible
	com_crashed = Cvar_Get( "com_crashed", "0", CVAR_TEMP );

	s = va( "%d", pid );
	com_pid = Cvar_Get( "com_pid", s, CVAR_ROM );

	// done early so bind command exists
	CL_InitKeyCommands();

	FS::Initialize();
	FS_LoadBasePak();
#ifndef DEDICATED
	// Load map pk3s to allow menus to load levelshots
	FS_LoadAllMaps();
#endif

	Trans_Init();

#ifndef DEDICATED
	Cmd::BufferCommandText("preset default.cfg");
#endif

#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)
	// skip the q3config.cfg if "safe" is on the command line
	if ( !Com_SafeMode() )
	{
		Cmd::BufferCommandText("exec " CONFIG_NAME);
		Cmd::BufferCommandText("exec " KEYBINDINGS_NAME);
		Cmd::BufferCommandText("exec " AUTOEXEC_NAME);
	}
#else
	Cmd::BufferCommandText("exec " CONFIG_NAME);
#endif

	// ydnar: reset crashed state
	Cmd::BufferCommandText("set com_crashed 0");

	// execute the queued commands
	Cmd::ExecuteCommandBuffer();

	// override anything from the config files with command line args
	Com_StartupVariable( NULL );

#if defined(DEDICATED)
	// TTimo: default to Internet dedicated, not LAN dedicated
	com_dedicated = Cvar_Get( "dedicated", "2", CVAR_ROM );
#else
	com_dedicated = Cvar_Get( "dedicated", "0", CVAR_LATCH );
#endif
	// allocate the stack based hunk allocator
	Com_InitHunkMemory();
	Trans_LoadDefaultLanguage();

	// if any archived cvars are modified after this, we will trigger a writing
	// of the config file
	cvar_modifiedFlags &= ~CVAR_ARCHIVE_BITS;

	//
	// init commands and vars
	//
	com_developer = Cvar_Get( "developer", "0", CVAR_TEMP );

	com_logfile = Cvar_Get( "logfile", "0", CVAR_TEMP );

	com_timescale = Cvar_Get( "timescale", "1", CVAR_CHEAT | CVAR_SYSTEMINFO );
	com_dropsim = Cvar_Get( "com_dropsim", "0", CVAR_CHEAT );
	com_viewlog = Cvar_Get( "viewlog", "0", CVAR_CHEAT );
	com_speeds = Cvar_Get( "com_speeds", "0", 0 );
	com_timedemo = Cvar_Get( "timedemo", "0", CVAR_CHEAT );

	cl_paused = Cvar_Get( "cl_paused", "0", CVAR_ROM );
	sv_paused = Cvar_Get( "sv_paused", "0", CVAR_ROM );
	com_sv_running = Cvar_Get( "sv_running", "0", CVAR_ROM );
	com_cl_running = Cvar_Get( "cl_running", "0", CVAR_ROM );

	//on a server, commands have to be used a lot more often than say
	//we could differentiate server and client, but would change the default behavior many might be used to
	com_consoleCommand = Cvar_Get( "com_consoleCommand", "", 0 );

	com_introPlayed = Cvar_Get( "com_introplayed", "0", 0 );
	com_ansiColor = Cvar_Get( "com_ansiColor", "1", 0 );
	com_logosPlaying = Cvar_Get( "com_logosPlaying", "0", CVAR_ROM );
	com_recommendedSet = Cvar_Get( "com_recommendedSet", "0", 0 );

	com_unfocused = Cvar_Get( "com_unfocused", "0", CVAR_ROM );
	com_minimized = Cvar_Get( "com_minimized", "0", CVAR_ROM );

#if defined( _WIN32 ) && !defined( NDEBUG )
	com_noErrorInterrupt = Cvar_Get( "com_noErrorInterrupt", "0", 0 );
#endif

	com_hunkused = Cvar_Get( "com_hunkused", "0", 0 );
	com_hunkusedvalue = 0;

	if ( com_dedicated->integer )
	{
		if ( !com_viewlog->integer )
		{
			Cvar_Set( "viewlog", "1" );
		}
	}

	if ( com_developer && com_developer->integer )
	{
		Cmd_AddCommand( "error", Com_Error_f );
		Cmd_AddCommand( "crash", Com_Crash_f );
		Cmd_AddCommand( "freeze", Com_Freeze_f );
	}

	Cmd_AddCommand( "quit", Com_Quit_f );
	Cmd_AddCommand( "writeconfig", Com_WriteConfig_f );
#if !defined(DEDICATED)
	Cmd_AddCommand( "writebindings", Com_WriteBindings_f );
#endif

	s = va( "%s %s %s %s", Q3_VERSION, PLATFORM_STRING, ARCH_STRING, __DATE__ );
	com_version = Cvar_Get( "version", s, CVAR_ROM | CVAR_SERVERINFO );

	Sys_Init();

	// Pick a qport value that is nice and random.
	// As machines get faster, Com_Milliseconds() can't be used
	// anymore, as it results in a smaller and smaller range of
	// qport values.
	Com_RandomBytes( ( byte * )&qport, sizeof( int ) );
	Netchan_Init( qport & 0xffff );

	VM_Init();
	// Ignore any errors
	VM_Forced_Unload_Start();

	SV_Init();
	Console::LoadHistory();

	com_dedicated->modified = qfalse;

	if ( !com_dedicated->integer )
	{
		CL_Init();
	}

	// set com_frameTime so that if a map is started on the
	// command line it will still be able to count on com_frameTime
	// being random enough for a serverid
	com_frameTime = Com_Milliseconds();

	// add + commands from command line
	if ( !Com_AddStartupCommands() )
	{
		// if the user didn't give any commands, run default action
	}

	CL_StartHunkUsers();

	if ( !com_dedicated->integer )
	{
		//Cvar_Set( "com_logosPlaying", "1" );
		//Cmd::BufferCommandText("cinematic etintro.roq\n");

		/*Cvar_Set( "sv_nextmap", "cinematic avlogo.roq" );
		   if( !com_introPlayed->integer ) {
		   Cvar_Set( com_introPlayed->name, "1" );
		   //Cvar_Set( "sv_nextmap", "cinematic avlogo.roq" );
		   } */
	}

	if (defaultPipeFilename[0])
	{
		std::string ospath = FS::Path::Build(FS::GetHomePath(), defaultPipeFilename);
		pipefile = Sys_Mkfifo(ospath.c_str());
		if (!pipefile)
		{
			Com_Printf( S_WARNING "Could not create new pipefile at %s. "
			"pipefile will not be used.\n", ospath.c_str() );
		}
	}
	com_fullyInitialized = qtrue;
	Com_Printf( "%s", "--- Common Initialization Complete ---\n" );
}

/*
===============
Com_ReadFromPipe

Read whatever is in the pipe, and if a line gets accumulated, executed it
===============
*/
void Com_ReadFromPipe( void )
{
	static char buf[ MAX_STRING_CHARS ];
	static int  numAccd = 0;
	int         numNew;

	if ( !pipefile )
	{
		return;
	}

	while ( ( numNew = fread( buf + numAccd, 1, sizeof( buf ) - 1 - numAccd, pipefile ) ) > 0 )
	{
		char *brk = NULL; // will point to after the last CR/LF character, if any
		int i;

		for ( i = numAccd; i < numAccd + numNew; ++i )
		{
			if( buf[ i ] == '\0' )
				buf[ i ] = '\n';
			if( buf[ i ] == '\n' || buf[ i ] == '\r' )
				brk = &buf[ i + 1 ];
		}

		numAccd += numNew;

		if ( brk )
		{
			char tmp = *brk;
			*brk = '\0';
			Cmd::BufferCommandText(buf);
			*brk = tmp;

			numAccd -= brk - buf;
			memmove( buf, brk, numAccd );
		}
		else if ( numAccd >= sizeof( buf ) - 1 ) // there are no CR/LF characters, but the buffer is full
		{
			// unfortunately, this command line gets chopped
			//  (but Cbuf_ExecuteText() chops long command lines at (MAX_STRING_CHARS - 1) anyway)
			buf[ sizeof( buf ) - 1 ] = '\0';
			Cmd::BufferCommandText(buf);
			numAccd = 0;
		}
	}
}

//==================================================================

void Com_WriteConfigToFile( const char *filename, void (*writeConfig)( fileHandle_t ) )
{
	fileHandle_t f;
	char         tmp[ MAX_QPATH ];

	Com_sprintf( tmp, sizeof( tmp ), "config/%s", filename );

	f = FS_FOpenFileWriteViaTemporary( tmp );

	if ( !f )
	{
		Com_Printf(_( "Couldn't write %s.\n"), filename );
		return;
	}

	FS_Printf( f, "// generated by Unvanquished, do not modify\n" );
	writeConfig( f );
	FS_FCloseFile( f );
}

/*
===============
Com_WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void Com_WriteConfiguration( void )
{
	// if we are quiting without fully initializing, make sure
	// we don't write out anything
	if ( !com_fullyInitialized )
	{
		return;
	}

	if ( cvar_modifiedFlags & CVAR_ARCHIVE_BITS )
	{
		cvar_modifiedFlags &= ~CVAR_ARCHIVE_BITS;

		Com_WriteConfigToFile( CONFIG_NAME, Cvar_WriteVariables );
	}

#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)
	if ( bindingsModified )
	{
		bindingsModified = qfalse;

		Com_WriteConfigToFile( KEYBINDINGS_NAME, Key_WriteBindings );
	}
#endif
}

/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
void Com_WriteConfig_f( void )
{
	char filename[ MAX_QPATH ];

	if ( Cmd_Argc() != 2 )
	{
		Cmd_PrintUsage(_("<filename>"), NULL);
		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );
	Com_Printf(_( "Writing %s.\n"), filename );
	Com_WriteConfigToFile( filename, Cvar_WriteVariables );
}

/*
===============
Com_WriteBindings_f

Write the key bindings file to a specific name
===============
*/
#if !defined(DEDICATED)
void Com_WriteBindings_f( void )
{
	char filename[ MAX_QPATH ];

	if ( Cmd_Argc() != 2 )
	{
		Cmd_PrintUsage(_("<filename>"), NULL);
		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );
	Com_Printf(_( "Writing %s.\n"), filename );
	Com_WriteConfigToFile( filename, Key_WriteBindings );
}
#endif

/*
================
Com_ModifyMsec
================
*/
static Cvar::Cvar<int> fixedtime("common.framerate.fixed", "in milliseconds, forces the frame time, 0 for no effect", Cvar::CHEAT, 0);

int Com_ModifyMsec( int msec )
{
	int clampTime;

	//
	// modify time for debugging values
	//
	if ( fixedtime.Get() )
	{
		msec = fixedtime.Get();
	}
	else if ( com_timescale->value )
	{
		msec *= com_timescale->value;
	}

	// don't let it scale below 1 msec
	if ( msec < 1 && com_timescale->value )
	{
		msec = 1;
	}

	if ( com_dedicated->integer )
	{
		// dedicated servers don't want to clamp for a much longer
		// period, because it would mess up all the client's views
		// of time.
		if ( msec > 500 && msec < 500000 )
		{
			Com_Printf( "Hitch warning: %i msec frame time\n", msec );
		}

		clampTime = 5000;
	}
	else if ( !com_sv_running->integer )
	{
		// clients of remote servers do not want to clamp time, because
		// it would skew their view of the server's time temporarily
		clampTime = 5000;
	}
	else
	{
		// for local single player gaming
		// we may want to clamp the time to prevent players from
		// flying off edges when something hitches.
		clampTime = 200;
	}

	if ( msec > clampTime )
	{
		msec = clampTime;
	}

	return msec;
}

/*
=================
Com_Frame
=================
*/

//TODO 0 for the same value as common.maxFPS
static Cvar::Cvar<int> maxfps("common.framerate.max", "the max framerate, 0 for unlimited", Cvar::NONE, 125);
static Cvar::Cvar<int> maxfpsUnfocused("common.framerate.maxUnfocused", "the max framerate when the game is unfocused, 0 for unlimited", Cvar::NONE, 0);
static Cvar::Cvar<int> maxfpsMinimized("common.framerate.maxMinimized", "the max framerate when the game is minimized, 0 for unlimited", Cvar::NONE, 0);

static Cvar::Cvar<int> watchdogThreshold("common.watchdogTime", "seconds of server running without a map after which common.watchdogCmd is executed", Cvar::NONE, 60);
static Cvar::Cvar<std::string> watchdogCmd("common.watchdogCmd", "the command triggered by the watchdog, empty for /quit", Cvar::NONE, "");

static Cvar::Cvar<bool> showTraceStats("common.showTraceStats", "are physics traces stats printed each frame", Cvar::CHEAT, false);

void Com_Frame( void (*GetInput)( void ), void (*DoneInput)( void ) )
{
	int             msec, minMsec;
	static int      lastTime = 0;
	//int             key;

	int             timeBeforeFirstEvents;
	int             timeBeforeServer;
	int             timeBeforeEvents;
	int             timeBeforeClient;
	int             timeAfter;

	static int      watchdogTime = 0;
	static qboolean watchWarn = qfalse;

	if ( setjmp( abortframe ) )
	{
		return; // an ERR_DROP was thrown
	}

	// bk001204 - init to zero.
	//  also:  might be clobbered by `longjmp' or `vfork'
	timeBeforeFirstEvents = 0;
	timeBeforeServer = 0;
	timeBeforeEvents = 0;
	timeBeforeClient = 0;
	timeAfter = 0;

	// Check to make sure we don't have any http data waiting
	// comment this out until I get things going better under win32
	// old net chan encryption key
	//key = 0x87243987;

	// write config file if anything changed
	Com_WriteConfiguration();

	// if "viewlog" has been modified, show or hide the log console
	if ( com_viewlog->modified )
	{
		com_viewlog->modified = qfalse;
	}

	//
	// main event loop
	//
	if ( com_speeds->integer )
	{
		timeBeforeFirstEvents = Sys_Milliseconds();
	}

	// we may want to spin here if things are going too fast
	if ( !com_timedemo->integer )
	{
		if ( com_dedicated->integer )
		{
			minMsec = SV_FrameMsec();
		}
		else
		{
			if ( com_minimized->integer && maxfpsMinimized.Get() > 0 )
			{
				minMsec = 1000 / maxfpsMinimized.Get();
			}
			else if ( com_unfocused->integer && maxfpsUnfocused.Get() > 0 )
			{
				minMsec = 1000 / maxfpsUnfocused.Get();
			}
			else if ( maxfps.Get() > 0 )
			{
				minMsec = 1000 / maxfps.Get();
			}
			else
			{
				minMsec = 1;
			}
		}
	}
	else
	{
		minMsec = 1; // Bad things happen if this is 0
	}

	com_frameTime = Com_EventLoop();

	if ( lastTime > com_frameTime )
	{
		lastTime = com_frameTime; // possible on first frame
	}

	msec = com_frameTime - lastTime;

	GetInput(); // must be called at least once

	while ( msec < minMsec )
	{
		//give cycles back to the OS
		Sys_Sleep( std::min( minMsec - msec, 50 ) );
		GetInput();

		com_frameTime = Com_EventLoop();
		msec = com_frameTime - lastTime;
	}

	DoneInput();

	Cmd::ExecuteCommandBuffer();

	lastTime = com_frameTime;

	// mess with msec if needed
	com_frameMsec = msec;
	msec = Com_ModifyMsec( msec );

	//
	// server side
	//
	if ( com_speeds->integer )
	{
		timeBeforeServer = Sys_Milliseconds();
	}

	SV_Frame( msec );

	// if "dedicated" has been modified, start up
	// or shut down the client system.
	// Do this after the server may have started,
	// but before the client tries to auto-connect
	if ( com_dedicated->modified )
	{
		// get the latched value
		Cvar_Get( "dedicated", "0", 0 );
		com_dedicated->modified = qfalse;

		if ( !com_dedicated->integer )
		{
			CL_Init();
		}
		else
		{
			CL_Shutdown();
		}
	}

	//
	// client system
	//
	if ( !com_dedicated->integer )
	{
		//
		// run event loop a second time to get server to client packets
		// without a frame of latency
		//
		if ( com_speeds->integer )
		{
			timeBeforeEvents = Sys_Milliseconds();
		}

		Com_EventLoop();
		Cmd::DelayFrame();
		Cmd::ExecuteCommandBuffer();
		//
		// client side
		//
		if ( com_speeds->integer )
		{
			timeBeforeClient = Sys_Milliseconds();
		}

		CL_Frame( msec );

		if ( com_speeds->integer )
		{
			timeAfter = Sys_Milliseconds();
		}
	}
	else
	{
		timeAfter = Sys_Milliseconds();
	}

	//
	// watchdog
	//
	if ( com_dedicated->integer && !com_sv_running->integer && watchdogThreshold.Get() != 0 )
	{
		if ( watchdogTime == 0 )
		{
			watchdogTime = Sys_Milliseconds();
		}
		else
		{
			if ( !watchWarn && Sys_Milliseconds() - watchdogTime > ( watchdogThreshold.Get() - 4 ) * 1000 )
			{
				Com_Log( LOG_WARN, _( "watchdog will trigger in 4 seconds" ));
				watchWarn = qtrue;
			}
			else if ( Sys_Milliseconds() - watchdogTime > watchdogThreshold.Get() * 1000 )
			{
				Com_Printf(_( "Idle server with no map — triggering watchdog\n" ));
				watchdogTime = 0;
				watchWarn = qfalse;

				if ( watchdogCmd.Get().empty() )
				{
					Cmd::BufferCommandText("quit");
				}
				else
				{
					Cmd::BufferCommandText(watchdogCmd.Get());
				}
			}
		}
	}

	//
	// report timing information
	//
	if ( com_speeds->integer )
	{
		int all, sv, sev, cev, cl;

		all = timeAfter - timeBeforeServer;
		sv = timeBeforeEvents - timeBeforeServer;
		sev = timeBeforeServer - timeBeforeFirstEvents;
		cev = timeBeforeClient - timeBeforeEvents;
		cl = timeAfter - timeBeforeClient;
		sv -= time_game;
		cl -= time_frontend + time_backend;

		Com_Printf( "frame:%i all:%3i sv:%3i sev:%3i cev:%3i cl:%3i gm:%3i rf:%3i bk:%3i\n",
		            com_frameNumber, all, sv, sev, cev, cl, time_game, time_frontend, time_backend );
	}

	//
	// trace optimization tracking
	//
	if ( showTraceStats.Get() )
	{
		extern int c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces;
		extern int c_pointcontents;

		Com_Printf( "%4i traces  (%ib %ip %it) %4i points\n", c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces,
		            c_pointcontents );
		c_traces = 0;
		c_brush_traces = 0;
		c_patch_traces = 0;
		c_trisoup_traces = 0;
		c_pointcontents = 0;
	}

	// old net chan encryption key
	//key = lastTime * 0x87243987;

	Com_ReadFromPipe();

	com_frameNumber++;
}

/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown()
{
	if ( logfile )
	{
		FS_FCloseFile( logfile );
		logfile = 0;
	}

	if ( pipefile )
	{
		fclose( pipefile );
		FS_Delete( defaultPipeFilename );
	}

	FS::FlushAll();
}

//------------------------------------------------------------------------

void Com_GetHunkInfo( int *hunkused, int *hunkexpected )
{
	*hunkused = com_hunkusedvalue;
	*hunkexpected = com_expectedhunkusage;
}

/*
==================
Com_RandomBytes

fills string array with len radom bytes, peferably from the OS randomizer
==================
*/
void Com_RandomBytes( byte *string, int len )
{
	int i;

	if ( Sys_RandomBytes( string, len ) )
	{
		return;
	}

	Com_Printf( "Com_RandomBytes: using weak randomization\n" );

	for ( i = 0; i < len; i++ )
	{
		string[ i ] = ( unsigned char )( rand() % 255 );
	}
}

/*
==================
Com_IsVoipTarget

Returns non-zero if given clientNum is enabled in voipTargets, zero otherwise.
If clientNum is negative return if any bit is set.
==================
*/
qboolean Com_IsVoipTarget( uint8_t *voipTargets, int voipTargetsSize, int clientNum )
{
	int index;

	if ( clientNum < 0 )
	{
		for ( index = 0; index < voipTargetsSize; index++ )
		{
			if ( voipTargets[ index ] )
			{
				return qtrue;
			}
		}

		return qfalse;
	}

	index = clientNum >> 3;

	if ( index < voipTargetsSize )
	{
		return ( voipTargets[ index ] & ( 1 << ( clientNum & 0x07 ) ) );
	}

	return qfalse;
}

