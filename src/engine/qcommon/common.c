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

// htons
#if defined __linux__ || defined __FreeBSD__ || MACOS_X
#include <sys/types.h>
#include <netinet/in.h>
#else
#include <winsock.h>
#endif

#define MAX_NUM_ARGVS             50

#define MIN_DEDICATED_COMHUNKMEGS 4
#define MIN_COMHUNKMEGS           256 // JPW NERVE changed this to 42 for MP, was 56 for team arena and 75 for wolfSP
#define DEF_COMHUNKMEGS           512 // RF, increased this, some maps are exceeding 56mb
// JPW NERVE changed this for multiplayer back to 42, 56 for depot/mp_cpdepot, 42 for everything else
#define DEF_COMZONEMEGS           96 // RF, increased this from 16, to account for botlib/AAS
#define DEF_COMHUNKMEGS_S         XSTRING(DEF_COMHUNKMEGS)
#define DEF_COMZONEMEGS_S         XSTRING(DEF_COMZONEMEGS)

int                 com_argc;
char                *com_argv[ MAX_NUM_ARGVS + 1 ];

jmp_buf             abortframe; // an ERR_DROP has occurred, exit the entire frame

FILE                *debuglogfile;
static fileHandle_t logfile;
static fileHandle_t pipefile;
fileHandle_t        com_journalFile; // events are written here
fileHandle_t        com_journalDataFile; // config files are written here

cvar_t              *com_crashed = NULL; // ydnar: set in case of a crash, prevents CVAR_UNSAFE variables from being set from a cfg

cvar_t *com_ignorecrash = NULL; // bani - let experienced users ignore crashes, explicit NULL to make win32 happy
cvar_t *com_pid; // bani - process id

cvar_t *com_viewlog;
cvar_t *com_speeds;
cvar_t *com_developer;
cvar_t *com_dedicated;
cvar_t *com_timescale;
cvar_t *com_fixedtime;
cvar_t *com_dropsim; // 0.0 to 1.0, simulated packet drops
cvar_t *com_journal;
cvar_t *com_maxfps;
cvar_t *com_timedemo;
cvar_t *com_sv_running;
cvar_t *com_cl_running;
cvar_t *com_logfile; // 1 = buffer log, 2 = flush after each print, 3 = append + flush
cvar_t *com_pipefile;
cvar_t *com_showtrace;
cvar_t *com_version;

cvar_t *con_drawnotify;
cvar_t *com_ansiColor;

cvar_t *com_consoleCommand;

cvar_t *com_unfocused;
cvar_t *com_maxfpsUnfocused;
cvar_t *com_minimized;
cvar_t *com_maxfpsMinimized;

cvar_t *com_introPlayed;
cvar_t *com_logosPlaying;
cvar_t *cl_paused;
cvar_t *sv_paused;
cvar_t *cl_packetdelay;
cvar_t *sv_packetdelay;

#if defined( _WIN32 ) && !defined( NDEBUG )
cvar_t *com_noErrorInterrupt;
#endif
cvar_t *com_recommendedSet;

cvar_t *com_watchdog;
cvar_t *com_watchdog_cmd;

cvar_t *com_hunkused; // Ridah

#if idx64 || idx64_32
int ( *Q_VMftol )( void );
#elif id386
long( QDECL *Q_ftol )( float f );
int ( QDECL *Q_VMftol )( void );
void ( QDECL *Q_SnapVector )( vec3_t vec );
#endif

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

static char *rd_buffer;
static int  rd_buffersize;
static void ( *rd_flush )( char *buffer );

void Com_BeginRedirect( char *buffer, int buffersize, void ( *flush )( char * ) )
{
	if ( !buffer || !buffersize || !flush )
	{
		return;
	}

	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void Com_EndRedirect( void )
{
	if ( rd_flush )
	{
		rd_flush( rd_buffer );
	}

	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}

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
	char            msg[ MAXPRINTMSG ];
	static qboolean opening_qconsole = qfalse;

	// FIXME TTimo
	// switched vsprintf -> vsnprintf
	// rcon could cause buffer overflow
	//
	Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );

	if ( rd_buffer )
	{
		if ( ( strlen( msg ) + strlen( rd_buffer ) ) > ( rd_buffersize - 1 ) )
		{
			rd_flush( rd_buffer );
			*rd_buffer = 0;
		}

		Q_strcat( rd_buffer, rd_buffersize, msg );
		// show_bug.cgi?id=51
		// only flush the rcon buffer when it's necessary, avoid fragmenting
		//rd_flush(rd_buffer);
		//*rd_buffer = 0;
		return strlen( msg );
	}

	// echo to console if we're not a dedicated server
	if ( com_dedicated && !com_dedicated->integer )
	{
		CL_ConsolePrint( msg );
	}

	// echo to dedicated console and early console
	Sys_Print( msg );

	// logfile
	if ( com_logfile && com_logfile->integer )
	{
		// TTimo: only open the qconsole.log if the filesystem is in an initialized state
		//   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on)
		if ( !logfile && FS_Initialized() && !opening_qconsole )
		{
			struct tm *newtime;

			time_t    aclock;

			opening_qconsole = qtrue;

			time( &aclock );
			newtime = localtime( &aclock );

			if ( com_logfile->integer != 3 )
			{
				logfile = FS_FOpenFileWrite( "unvconsole.log" );
			}
			else
			{
				logfile = FS_FOpenFileAppend( "unvconsole.log" );
			}

			Com_Printf(_( "logfile opened on %s\n"), asctime( newtime ) );

			if ( com_logfile->integer > 1 )
			{
				// force it to not buffer so we get valid
				// data even if we are crashing
				FS_ForceFlush( logfile );
			}

			opening_qconsole = qfalse;
		}

		if ( logfile && FS_Initialized() )
		{
			FS_Write( msg, strlen( msg ), logfile );
		}
	}

	return strlen( msg );
}

void QDECL PRINTF_LIKE(1) Com_Printf( const char *fmt, ... )
{
	va_list argptr;

	va_start( argptr, fmt );
	Com_VPrintf( fmt, argptr );
	va_end( argptr );
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
	FS_PureServerSetLoadedPaks( "", "" );

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
		VM_Forced_Unload_Start();
		Com_Printf( "^7%s\n", com_errorMessage );
		SV_Shutdown( "Server disconnected" );
		CL_Disconnect( qtrue );
		CL_FlushMemory();
		VM_Forced_Unload_Done();
		com_errorEntered = qfalse;
		longjmp( abortframe, -1 );
	}
	else if ( code == ERR_DROP )
	{
		VM_Forced_Unload_Start();
		Com_Printf( "^8%s\n", com_errorMessage );
		SV_Shutdown( va( "********************\nServer crashed: %s\n********************\n", com_errorMessage ) );
		CL_Disconnect( qtrue );
		CL_FlushMemory();
		VM_Forced_Unload_Done();
		com_errorEntered = qfalse;
		longjmp( abortframe, -1 );
	}
	else
	{
		VM_Forced_Unload_Start();
		CL_Shutdown();
		SV_Shutdown( va( "Server fatal crashed: %s\n", com_errorMessage ) );
		VM_Forced_Unload_Done();
	}

	Com_Shutdown( code == ERR_VID_FATAL ? qtrue : qfalse );

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
		VM_Forced_Unload_Start();
		SV_Shutdown( p[ 0 ] ? p : "Server quit\n" );
//bani
#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)
		CL_ShutdownCGame();
#endif
		CL_Shutdown();
		VM_Forced_Unload_Done();
		Com_Shutdown( qfalse );
		FS_Shutdown( qtrue );
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
		Cmd_TokenizeString( com_consoleLines[ i ] );

		if ( !Q_stricmp( Cmd_Argv( 0 ), "safe" ) || !Q_stricmp( Cmd_Argv( 0 ), "cvar_restart" ) )
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
	char   *s;
	cvar_t *cv;

	for ( i = 0; i < com_numConsoleLines; i++ )
	{
		Cmd_TokenizeString( com_consoleLines[ i ] );

		if ( strcmp( Cmd_Argv( 0 ), "set" ) )
		{
			continue;
		}

		s = Cmd_Argv( 1 );

		if ( !match || !strcmp( s, match ) )
		{
			Cvar_Set( s, Cmd_Argv( 2 ) );
			cv = Cvar_Get( s, "", 0 );
			cv->flags |= CVAR_USER_CREATED;
//          com_consoleLines[i] = 0;
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
		if ( Q_stricmpn( com_consoleLines[ i ], "set", 3 ) )
		{
			added = qtrue;
		}

		Cbuf_AddText( com_consoleLines[ i ] );
		Cbuf_AddText( "\n" );
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

/*
============
Com_FilterPath
============
*/
int Com_FilterPath( char *filter, char *name, int casesensitive )
{
	int  i;
	char new_filter[ MAX_QPATH ];
	char new_name[ MAX_QPATH ];

	for ( i = 0; i < MAX_QPATH - 1 && filter[ i ]; i++ )
	{
		if ( filter[ i ] == '\\' || filter[ i ] == ':' )
		{
			new_filter[ i ] = '/';
		}
		else
		{
			new_filter[ i ] = filter[ i ];
		}
	}

	new_filter[ i ] = '\0';

	for ( i = 0; i < MAX_QPATH - 1 && name[ i ]; i++ )
	{
		if ( name[ i ] == '\\' || name[ i ] == ':' )
		{
			new_name[ i ] = '/';
		}
		else
		{
			new_name[ i ] = name[ i ];
		}
	}

	new_name[ i ] = '\0';
	return Com_Filter( new_filter, new_name, casesensitive );
}

/*
================
Com_RealTime
================
*/
int Com_RealTime( qtime_t *qtime )
{
	time_t    t;
	struct tm *tms;

	t = time( NULL );

	if ( !qtime )
	{
		return t;
	}

	tms = localtime( &t );

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
==============================================================================

                                                ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

#define ZONEID      0x1d4a11
#define MINFRAGMENT 64

typedef struct zonedebug_s
{
	const char *label;
	const char *file;
	int  line;
	int  allocSize;
} zonedebug_t;

typedef struct memblock_s
{
	int               size; // including the header and possibly tiny fragments
	int               tag; // a tag of 0 is a free block
	struct memblock_s *next, *prev;

	int               id; // should be ZONEID
#ifdef ZONE_DEBUG
	zonedebug_t       d;
#endif
} memblock_t;

typedef struct
{
	int        size; // total bytes malloced, including header
	int        used; // total bytes used
	memblock_t blocklist; // start / end cap for linked list
	memblock_t *rover;
} memzone_t;

// main zone for all "dynamic" memory allocation
memzone_t *mainzone;

// we also have a small zone for small allocations that would only
// fragment the main zone (think of cvar and cmd strings)
memzone_t *smallzone;

void      Z_CheckHeap( void );

/*
========================
Z_ClearZone
========================
*/
void Z_ClearZone( memzone_t *zone, int size )
{
	memblock_t *block;

	// set the entire zone to one free block

	zone->blocklist.next = zone->blocklist.prev = block = ( memblock_t * )( ( byte * ) zone + sizeof( memzone_t ) );
	zone->blocklist.tag = 1; // in use block
	zone->blocklist.id = 0;
	zone->blocklist.size = 0;
	zone->rover = block;
	zone->size = size;
	zone->used = 0;

	block->prev = block->next = &zone->blocklist;
	block->tag = 0; // free block
	block->id = ZONEID;
	block->size = size - sizeof( memzone_t );
}

/*
========================
Z_Free
========================
*/
void Z_Free( void *ptr )
{
	memblock_t *block, *other;
	memzone_t  *zone;

	if ( !ptr )
	{
		Com_Error( ERR_DROP, "Z_Free: NULL pointer" );
	}

	block = ( memblock_t * )( ( byte * ) ptr - sizeof( memblock_t ) );

	if ( block->id != ZONEID )
	{
		Com_Error( ERR_FATAL, "Z_Free: freed a pointer without ZONEID" );
	}

	if ( block->tag == 0 )
	{
		Com_Error( ERR_FATAL, "Z_Free: freed a freed pointer" );
	}

	// if static memory
	if ( block->tag == TAG_STATIC )
	{
		return;
	}

	// check the memory trash tester
	if ( * ( int * )( ( byte * ) block + block->size - 4 ) != ZONEID )
	{
		Com_Error( ERR_FATAL, "Z_Free: memory block wrote past end" );
	}

	if ( block->tag == TAG_SMALL )
	{
		zone = smallzone;
	}
	else
	{
		zone = mainzone;
	}

	zone->used -= block->size;
	// set the block to something that should cause problems
	// if it is referenced...
	memset( ptr, 0xaa, block->size - sizeof( *block ) );

	block->tag = 0; // mark as free

	other = block->prev;

	if ( !other->tag )
	{
		// merge with previous free block
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;

		if ( block == zone->rover )
		{
			zone->rover = other;
		}

		block = other;
	}

	zone->rover = block;

	other = block->next;

	if ( !other->tag )
	{
		// merge the next free block onto the end
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;

		if ( other == zone->rover )
		{
			zone->rover = block;
		}
	}
}

/*
================
Z_FreeTags
================
*/
void Z_FreeTags( int tag )
{
	int       count;
	memzone_t *zone;

	if ( tag == TAG_SMALL )
	{
		zone = smallzone;
	}
	else
	{
		zone = mainzone;
	}

	count = 0;
	// use the rover as our pointer, because
	// Z_Free automatically adjusts it
	zone->rover = zone->blocklist.next;

	do
	{
		if ( zone->rover->tag == tag )
		{
			count++;
			Z_Free( ( void * )( zone->rover + 1 ) );
			continue;
		}

		zone->rover = zone->rover->next;
	}
	while ( zone->rover != &zone->blocklist );
}

/*
================
Z_TagMalloc
================
*/

memblock_t *debugblock; // RF, jusy so we can track a block to find out when it's getting trashed

#ifdef ZONE_DEBUG
void           *Z_TagMallocDebug( int size, int tag, const char *label, const char *file, int line )
{
#else
void           *Z_TagMalloc( int size, int tag )
{
#endif
	int        extra;
	memblock_t *start, *rover, *new, *base;
	memzone_t  *zone;
#ifdef ZONE_DEBUG
	int        allocSize;
	allocSize = size;
#endif

	if ( !tag )
	{
		Com_Error( ERR_FATAL, "Z_TagMalloc: tried to use a 0 tag" );
	}

	if ( tag == TAG_SMALL )
	{
		zone = smallzone;
	}
	else
	{
		zone = mainzone;
	}

	// scan through the block list looking for the first free block
	// of sufficient size
	//
	size += sizeof( memblock_t );  // account for size of block header
	size += 4; // space for memory trash tester
	size = PAD( size, sizeof( intptr_t ) );   // align to 32/64 bit boundary

	base = rover = zone->rover;
	start = base->prev;

	do
	{
		if ( rover == start )
		{
			// scaned all the way around the list
#ifdef ZONE_DEBUG
			Z_LogHeap();

			Com_Error( ERR_FATAL, "Z_Malloc: failed on allocation of %i bytes from the %s zone: %s, line: %d (%s)",
			           size, zone == smallzone ? "small" : "main", file, line, label );
#else
			Com_Error( ERR_FATAL, "Z_Malloc: failed on allocation of %i bytes from the %s zone",
			           size, zone == smallzone ? "small" : "main" );
#endif
			return NULL;
		}

		if ( rover->tag )
		{
			base = rover = rover->next;
		}
		else
		{
			rover = rover->next;
		}
	}
	while ( base->tag || base->size < size );

	//
	// found a block big enough
	//
	extra = base->size - size;

	if ( extra > MINFRAGMENT )
	{
		// there will be a free fragment after the allocated block
		new = ( memblock_t * )( ( byte * ) base + size );
		new->size = extra;
		new->tag = 0; // free block
		new->prev = base;
		new->id = ZONEID;
		new->next = base->next;
		new->next->prev = new;
		base->next = new;
		base->size = size;
	}

	base->tag = tag; // no longer a free block

	zone->rover = base->next; // next allocation will start looking here
	zone->used += base->size; //

	base->id = ZONEID;

#ifdef ZONE_DEBUG
	base->d.label = label;
	base->d.file = file;
	base->d.line = line;
	base->d.allocSize = allocSize;
#endif

	// marker for memory trash testing
	* ( int * )( ( byte * ) base + base->size - 4 ) = ZONEID;

	return ( void * )( ( byte * ) base + sizeof( memblock_t ) );
}

/*
========================
Z_Malloc
========================
*/
#ifdef ZONE_DEBUG
void           *Z_MallocDebug( int size, const char *label, const char *file, int line )
{
#else
void           *Z_Malloc( int size )
{
#endif
	void *buf;

	//Z_CheckHeap ();   // DEBUG

#ifdef ZONE_DEBUG
	buf = Z_TagMallocDebug( size, TAG_GENERAL, label, file, line );
#else
	buf = Z_TagMalloc( size, TAG_GENERAL );
#endif
	Com_Memset( buf, 0, size );

	return buf;
}

#ifdef ZONE_DEBUG
void           *S_MallocDebug( int size, const char *label, const char *file, int line )
{
	return Z_TagMallocDebug( size, TAG_SMALL, label, file, line );
}

#else
void           *S_Malloc( int size )
{
	return Z_TagMalloc( size, TAG_SMALL );
}

#endif

/*
========================
Z_CheckHeap
========================
*/
void Z_CheckHeap( void )
{
	memblock_t *block;

	for ( block = mainzone->blocklist.next;; block = block->next )
	{
		if ( block->next == &mainzone->blocklist )
		{
			break; // all blocks have been hit
		}

		if ( ( byte * ) block + block->size != ( byte * ) block->next )
		{
			Com_Error( ERR_FATAL, "Z_CheckHeap: block size does not touch the next block" );
		}

		if ( block->next->prev != block )
		{
			Com_Error( ERR_FATAL, "Z_CheckHeap: next block doesn't have proper back link" );
		}

		if ( !block->tag && !block->next->tag )
		{
			Com_Error( ERR_FATAL, "Z_CheckHeap: two consecutive free blocks" );
		}
	}
}

/*
========================
Z_LogZoneHeap
========================
*/
void Z_LogZoneHeap( const memzone_t *zone, const char *name )
{
#ifdef ZONE_DEBUG
	char       dump[ 32 ];
	const char *ptr;
	int        i, j;
#endif
	const memblock_t *block;
	char       buf[ 4096 ];
	int        size, allocSize, numBlocks;

	if ( !logfile || !FS_Initialized() )
	{
		return;
	}

	size = allocSize = numBlocks = 0;
	Com_sprintf( buf, sizeof( buf ), "\r\n================\r\n%s log\r\n================\r\n", name );
	FS_Write( buf, strlen( buf ), logfile );

	for ( block = zone->blocklist.next; block->next != &zone->blocklist; block = block->next )
	{
		if ( block->tag )
		{
#ifdef ZONE_DEBUG
			ptr = ( ( const char * ) block ) + sizeof( memblock_t );
			j = 0;

			for ( i = 0; i < 20 && i < block->d.allocSize; i++ )
			{
				if ( ptr[ i ] >= 32 && ptr[ i ] < 127 )
				{
					dump[ j++ ] = ptr[ i ];
				}
				else
				{
					dump[ j++ ] = '_';
				}
			}

			dump[ j ] = '\0';
			Com_sprintf( buf, sizeof( buf ), "size = %8d: %s, line: %d (%s) [%s]\r\n", block->d.allocSize, block->d.file,
			             block->d.line, block->d.label, dump );
			FS_Write( buf, strlen( buf ), logfile );
			allocSize += block->d.allocSize;
#endif
			size += block->size;
			numBlocks++;
		}
	}

#ifdef ZONE_DEBUG
	// subtract debug memory
	size -= numBlocks * sizeof( zonedebug_t );
#else
	allocSize = numBlocks * sizeof( memblock_t );  // + 32 bit alignment
#endif
	Com_sprintf( buf, sizeof( buf ), "%d %s memory in %d blocks\r\n", size, name, numBlocks );
	FS_Write( buf, strlen( buf ), logfile );
	Com_sprintf( buf, sizeof( buf ), "%d %s memory overhead\r\n", size - allocSize, name );
	FS_Write( buf, strlen( buf ), logfile );
}

/*
========================
Z_LogHeap
========================
*/
void Z_LogHeap( void )
{
	Z_LogZoneHeap( mainzone, "MAIN" );
	Z_LogZoneHeap( smallzone, "SMALL" );
}

// static mem blocks to reduce a lot of small zone overhead
typedef struct memstatic_s
{
	memblock_t b;
	byte       mem[ 2 ];
} memstatic_t;

// bk001204 - initializer brackets
static const memstatic_t emptystring = { { ( sizeof( memblock_t ) + 2 + 3 ) &~3, TAG_STATIC, NULL, NULL, ZONEID }
	, { '\0',                            '\0' }
};
static const memstatic_t numberstring[] =
{
	{	{ ( sizeof( memstatic_t ) + 3 ) &~3, TAG_STATIC, NULL, NULL, ZONEID }
		, { '0', '\0' }
	}
	,
	{	{ ( sizeof( memstatic_t ) + 3 ) &~3, TAG_STATIC, NULL, NULL, ZONEID }
		, { '1', '\0' }
	}
	,
	{	{ ( sizeof( memstatic_t ) + 3 ) &~3, TAG_STATIC, NULL, NULL, ZONEID }
		, { '2', '\0' }
	}
	,
	{	{ ( sizeof( memstatic_t ) + 3 ) &~3, TAG_STATIC, NULL, NULL, ZONEID }
		, { '3', '\0' }
	}
	,
	{	{ ( sizeof( memstatic_t ) + 3 ) &~3, TAG_STATIC, NULL, NULL, ZONEID }
		, { '4', '\0' }
	}
	,
	{	{ ( sizeof( memstatic_t ) + 3 ) &~3, TAG_STATIC, NULL, NULL, ZONEID }
		, { '5', '\0' }
	}
	,
	{	{ ( sizeof( memstatic_t ) + 3 ) &~3, TAG_STATIC, NULL, NULL, ZONEID }
		, { '6', '\0' }
	}
	,
	{	{ ( sizeof( memstatic_t ) + 3 ) &~3, TAG_STATIC, NULL, NULL, ZONEID }
		, { '7', '\0' }
	}
	,
	{	{ ( sizeof( memstatic_t ) + 3 ) &~3, TAG_STATIC, NULL, NULL, ZONEID }
		, { '8', '\0' }
	}
	,
	{	{ ( sizeof( memstatic_t ) + 3 ) &~3, TAG_STATIC, NULL, NULL, ZONEID }
		, { '9', '\0' }
	}
};

/*
========================
CopyString

 NOTE:  never write over the memory CopyString returns because
                memory from a memstatic_t might be returned
========================
*/
char           *CopyString( const char *in )
{
	char *out;

	if ( !in[ 0 ] )
	{
		return ( ( char * ) &emptystring ) + sizeof( memblock_t );
	}
	else if ( !in[ 1 ] )
	{
		if ( in[ 0 ] >= '0' && in[ 0 ] <= '9' )
		{
			return ( ( char * ) &numberstring[ in[ 0 ] - '0' ] ) + sizeof( memblock_t );
		}
	}

	out = S_Malloc( strlen( in ) + 1 );
	strcpy( out, in );
	return out;
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

static hunkblock_t *hunkblocks;

static hunkUsed_t  hunk_low, hunk_high;
static hunkUsed_t  *hunk_permanent, *hunk_temp;

static byte        *s_hunkData = NULL;
static int         s_hunkTotal;

static int         s_zoneTotal;
static int         s_smallZoneTotal;

/*
=================
Com_Meminfo_f
=================
*/
void Com_Meminfo_f( void )
{
	memblock_t *block;
	int        zoneBytes, zoneBlocks;
	int        smallZoneBytes, smallZoneBlocks;
	int        botlibBytes, rendererBytes;
	int        unused;

	zoneBytes = 0;
	botlibBytes = 0;
	rendererBytes = 0;
	zoneBlocks = 0;

	for ( block = mainzone->blocklist.next;; block = block->next )
	{
		if ( Cmd_Argc() != 1 )
		{
			Com_Printf( "block:%p    size:%7i    tag:%3i\n", ( void * ) block, block->size, block->tag );
		}

		if ( block->tag )
		{
			zoneBytes += block->size;
			zoneBlocks++;

			if ( block->tag == TAG_BOTLIB )
			{
				botlibBytes += block->size;
			}
			else if ( block->tag == TAG_RENDERER )
			{
				rendererBytes += block->size;
			}
		}

		if ( block->next == &mainzone->blocklist )
		{
			break; // all blocks have been hit
		}

		if ( ( byte * ) block + block->size != ( byte * ) block->next )
		{
			Com_Printf( "ERROR: block size does not touch the next block\n" );
		}

		if ( block->next->prev != block )
		{
			Com_Printf( "ERROR: next block doesn't have proper back link\n" );
		}

		if ( !block->tag && !block->next->tag )
		{
			Com_Printf( "ERROR: two consecutive free blocks\n" );
		}
	}

	smallZoneBytes = 0;
	smallZoneBlocks = 0;

	for ( block = smallzone->blocklist.next;; block = block->next )
	{
		if ( block->tag )
		{
			smallZoneBytes += block->size;
			smallZoneBlocks++;
		}

		if ( block->next == &smallzone->blocklist )
		{
			break; // all blocks have been hit
		}
	}

	Com_Printf( "%9i bytes (%6.2f MB) total hunk\n", s_hunkTotal, s_hunkTotal / Square( 1024.f ) );
	Com_Printf( "%9i bytes (%6.2f MB) total zone\n", s_zoneTotal, s_zoneTotal / Square( 1024.f ) );
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
	unused = 0;

	if ( hunk_low.tempHighwater > hunk_low.permanent )
	{
		unused += hunk_low.tempHighwater - hunk_low.permanent;
	}

	if ( hunk_high.tempHighwater > hunk_high.permanent )
	{
		unused += hunk_high.tempHighwater - hunk_high.permanent;
	}

	Com_Printf( "%9i bytes (%6.2f MB) unused highwater\n", unused, unused / Square( 1024.f ) );
	Com_Printf( "\n" );
	Com_Printf( "%9i bytes (%6.2f MB) in %i zone blocks\n", zoneBytes, zoneBytes / Square( 1024.f ), zoneBlocks );
	Com_Printf( "        %9i bytes (%6.2f MB) in dynamic botlib\n", botlibBytes, botlibBytes / Square( 1024.f ) );
	Com_Printf( "        %9i bytes (%6.2f MB) in dynamic renderer\n", rendererBytes, rendererBytes / Square( 1024.f ) );
	Com_Printf( "        %9i bytes (%6.2f MB) in dynamic other\n", zoneBytes - ( botlibBytes + rendererBytes ),
	            ( zoneBytes - ( botlibBytes + rendererBytes ) ) / Square( 1024.f ) );
	Com_Printf( "        %9i bytes (%6.2f MB) in small Zone memory\n", smallZoneBytes, smallZoneBytes / Square( 1024.f ) );
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
	memblock_t *block;

	Z_CheckHeap();

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

	for ( block = mainzone->blocklist.next;; block = block->next )
	{
		if ( block->tag )
		{
			j = block->size >> 2;

			for ( i = 0; i < j; i += 64 )
			{
				// only need to touch each page
				sum += ( ( int * ) block ) [ i ];
			}
		}

		if ( block->next == &mainzone->blocklist )
		{
			break; // all blocks have been hit
		}
	}

	end = Sys_Milliseconds();

	Com_DPrintf( "Com_TouchMemory: %i msec\n", end - start );
}

/*
=================
Com_DetectSSE
Find out whether we have SSE support for Q_ftol function
=================
*/

#if id386 || idx64 || idx64_32

static void Com_DetectSSE( void )
{
#if !idx64 && !idx64_32
	cpuFeatures_t feat;

	feat = Sys_GetProcessorFeatures();

	if ( feat & CF_SSE )
	{
		if ( feat & CF_SSE2 )
		{
			Q_SnapVector = qsnapvectorsse;
		}
		else
		{
			Q_SnapVector = qsnapvectorx87;
		}

		Q_ftol = qftolsse;
#endif
		Q_VMftol = qvmftolsse;

		Com_Printf( "Have SSE support\n" );
#if !idx64 && !idx64_32
	}

	else
	{
		Q_ftol = qftolx87;
		Q_VMftol = qvmftolx87;
		Q_SnapVector = qsnapvectorx87;

		Com_Printf( "No SSE support on this machine\n" );
	}

#endif
}

#else

#define Com_DetectSSE()

#endif

/*
=================
Com_InitZoneMemory
=================
*/
void Com_InitSmallZoneMemory( void )
{
	s_smallZoneTotal = 512 * 1024;
	// bk001205 - was malloc
	smallzone = calloc( s_smallZoneTotal, 1 );

	if ( !smallzone )
	{
		Com_Error( ERR_FATAL, "Small zone data failed to allocate %1.1fMB", ( float ) s_smallZoneTotal / ( 1024 * 1024 ) );
	}

	Z_ClearZone( smallzone, s_smallZoneTotal );
}

void Com_InitZoneMemory( void )
{
	cvar_t *cv;

	// allocate the random block zone
	Com_StartupVariable( "com_zoneMegs" );  // config files and command line options haven't been taken in account yet
	cv = Cvar_Get( "com_zoneMegs", DEF_COMZONEMEGS_S, CVAR_LATCH | CVAR_ARCHIVE );

	if ( cv->integer < DEF_COMZONEMEGS )
	{
		s_zoneTotal = 1024 * 1024 * DEF_COMZONEMEGS;
	}
	else
	{
		s_zoneTotal = cv->integer * 1024 * 1024;
	}

	// bk001205 - was malloc
	mainzone = calloc( s_zoneTotal, 1 );

	if ( !mainzone )
	{
		Com_Error( ERR_FATAL, "Zone data failed to allocate %iMB", s_zoneTotal / ( 1024 * 1024 ) );
	}

	Z_ClearZone( mainzone, s_zoneTotal );
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

	if ( !logfile || !FS_Initialized() )
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

	if ( !logfile || !FS_Initialized() )
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
Com_InitZoneMemory
=================
*/
void Com_InitHunkMemory( void )
{
	cvar_t *cv;
	int    nMinAlloc;
	char   *pMsg = NULL;

	// make sure the file system has allocated and "not" freed any temp blocks
	// this allows the config and product id files ( journal files too ) to be loaded
	// by the file system without redundant routines in the file system utilizing different
	// memory systems
	if ( FS_LoadStack() != 0 )
	{
		Com_Error( ERR_FATAL, "Hunk initialization failed. File system load stack not zero" );
	}

	// allocate the stack based hunk allocator
	cv = Cvar_Get( "com_hunkMegs", DEF_COMHUNKMEGS_S, CVAR_LATCH | CVAR_ARCHIVE );

	// if we are not dedicated min allocation is 56, otherwise min is 1
	if ( com_dedicated && com_dedicated->integer )
	{
		nMinAlloc = MIN_DEDICATED_COMHUNKMEGS;
		pMsg = "Minimum com_hunkMegs for a dedicated server is %i, allocating %iMB.\n";
	}
	else
	{
		nMinAlloc = MIN_COMHUNKMEGS;
		pMsg = "Minimum com_hunkMegs is %i, allocating %iMB.\n";
	}

	if ( cv->integer < nMinAlloc )
	{
		s_hunkTotal = 1024 * 1024 * nMinAlloc;
		Com_Printf( pMsg, nMinAlloc, s_hunkTotal / ( 1024 * 1024 ) );
	}
	else
	{
		s_hunkTotal = cv->integer * 1024 * 1024;
	}

	s_hunkData = malloc( s_hunkTotal + 31 );

	if ( !s_hunkData )
	{
		Com_Error( ERR_FATAL, "Hunk data failed to allocate %iMB", s_hunkTotal / ( 1024 * 1024 ) );
	}

	// cacheline align
	s_hunkData = ( byte * )( ( ( intptr_t ) s_hunkData + 31 ) & ~31 );
	Hunk_Clear();

	Cmd_AddCommand( "meminfo", Com_Meminfo_f );
#ifdef ZONE_DEBUG
	Cmd_AddCommand( "zonelog", Z_LogHeap );
#endif
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

/*
=================
Hunk_CheckMark
=================
*/
qboolean Hunk_CheckMark( void )
{
	if ( hunk_low.mark || hunk_high.mark )
	{
		return qtrue;
	}

	return qfalse;
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
	VM_Clear(); // (SA) FIXME:TODO: was commented out in wolf
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
	size += sizeof( hunkblock_t );
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
		block->size = size - sizeof( hunkblock_t );
		block->file = file;
		block->label = label;
		block->line = line;
		block->next = hunkblocks;
		hunkblocks = block;
		buf = ( ( byte * ) buf ) + sizeof( hunkblock_t );
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
=================
Hunk_ClearTempMemory

The temp space is no longer needed.  If we have left more
touched but unused memory on this side, have future
permanent allocs use this side.
=================
*/
void Hunk_ClearTempMemory( void )
{
	if ( s_hunkData != NULL )
	{
		hunk_temp->temp = hunk_temp->permanent;
	}
}

/*
===================================================================

EVENTS AND JOURNALING

In addition to these events, .cfg files are also copied to the
journaled file
===================================================================
*/

// bk001129 - here we go again: upped from 64
#define MAX_PUSHED_EVENTS 256
// bk001129 - init, also static
static int        com_pushedEventsHead = 0;
static int        com_pushedEventsTail = 0;

// bk001129 - static
static sysEvent_t com_pushedEvents[ MAX_PUSHED_EVENTS ];

/*
=================
Com_InitJournaling
=================
*/
void Com_InitJournaling( void )
{
	int i;

	Com_StartupVariable( "journal" );
	com_journal = Cvar_Get( "journal", "0", CVAR_INIT );

	if ( !com_journal->integer )
	{
		if ( com_journal->string && com_journal->string[ 0 ] == '_' )
		{
			Com_Printf( "Replaying journaled events\n" );
			FS_FOpenFileRead( va( "journal%s.dat", com_journal->string ), &com_journalFile, qtrue );
			FS_FOpenFileRead( va( "journal_data%s.dat", com_journal->string ), &com_journalDataFile, qtrue );
			com_journal->integer = 2;
		}
		else
		{
			return;
		}
	}
	else
	{
		for ( i = 0; i <= 9999; i++ )
		{
			char f[ MAX_OSPATH ];
			Com_sprintf( f, sizeof( f ), "journal_%04d.dat", i );

			if ( !FS_FileExists( f ) )
			{
				break;
			}
		}

		if ( com_journal->integer == 1 )
		{
			Com_Printf( "Journaling events\n" );
			com_journalFile = FS_FOpenFileWrite( va( "journal_%04d.dat", i ) );
			com_journalDataFile = FS_FOpenFileWrite( va( "journal_data_%04d.dat", i ) );
		}
		else if ( com_journal->integer == 2 )
		{
			i--;
			Com_Printf( "Replaying journaled events\n" );
			FS_FOpenFileRead( va( "journal_%04d.dat", i ), &com_journalFile, qtrue );
			FS_FOpenFileRead( va( "journal_data_%04d.dat", i ), &com_journalDataFile, qtrue );
		}
	}

	if ( !com_journalFile || !com_journalDataFile )
	{
		Cvar_Set( "journal", "0" );
		com_journalFile = 0;
		com_journalDataFile = 0;
		Com_Printf( "Couldn't open journal files\n" );
	}
}

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
Com_GetSystemEvent

================
*/
sysEvent_t Com_GetSystemEvent( void )
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
		b = Z_Malloc( len );
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
		buf = Z_Malloc( len );
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
Com_GetRealEvent
=================
*/
sysEvent_t      Com_GetRealEvent( void )
{
	int        r;
	sysEvent_t ev;

	// either get an event from the system or the journal file
	if ( com_journal->integer == 2 )
	{
		r = FS_Read( &ev, sizeof( ev ), com_journalFile );

		if ( r != sizeof( ev ) )
		{
			//Com_Error( ERR_FATAL, "Error reading from journal file" );
			com_journal->integer = 0;
			ev.evType = SE_NONE;
		}

		if ( ev.evPtrLength )
		{
			ev.evPtr = Z_Malloc( ev.evPtrLength );
			r = FS_Read( ev.evPtr, ev.evPtrLength, com_journalFile );

			if ( r != ev.evPtrLength )
			{
				//Com_Error( ERR_FATAL, "Error reading from journal file" );
				com_journal->integer = 0;
				ev.evType = SE_NONE;
			}
		}
	}
	else
	{
		ev = Com_GetSystemEvent();

		// write the journal value out if needed
		if ( com_journal->integer == 1 )
		{
			r = FS_Write( &ev, sizeof( ev ), com_journalFile );

			if ( r != sizeof( ev ) )
			{
				Com_Error( ERR_FATAL, "Error writing to journal file" );
			}

			if ( ev.evPtrLength )
			{
				r = FS_Write( ev.evPtr, ev.evPtrLength, com_journalFile );

				if ( r != ev.evPtrLength )
				{
					Com_Error( ERR_FATAL, "Error writing to journal file" );
				}
			}
		}
	}

	return ev;
}

/*
=================
Com_InitPushEvent
=================
*/
// bk001129 - added
void Com_InitPushEvent( void )
{
	// clear the static buffer array
	// this requires SE_NONE to be accepted as a valid but NOP event
	memset( com_pushedEvents, 0, sizeof( com_pushedEvents ) );
	// reset counters while we are at it
	// beware: GetEvent might still return an SE_NONE from the buffer
	com_pushedEventsHead = 0;
	com_pushedEventsTail = 0;
}

/*
=================
Com_PushEvent
=================
*/
void Com_PushEvent( sysEvent_t *event )
{
	sysEvent_t *ev;
	static int printedWarning = 0; // bk001129 - init, bk001204 - explicit int

	ev = &com_pushedEvents[ com_pushedEventsHead & ( MAX_PUSHED_EVENTS - 1 ) ];

	if ( com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS )
	{
		// don't print the warning constantly, or it can give time for more...
		if ( !printedWarning )
		{
			printedWarning = qtrue;
			Com_Printf( "WARNING: Com_PushEvent overflow\n" );
		}

		if ( ev->evPtr )
		{
			Z_Free( ev->evPtr );
		}

		com_pushedEventsTail++;
	}
	else
	{
		printedWarning = qfalse;
	}

	*ev = *event;
	com_pushedEventsHead++;
}

/*
=================
Com_GetEvent
=================
*/
sysEvent_t Com_GetEvent( void )
{
	if ( com_pushedEventsHead > com_pushedEventsTail )
	{
		com_pushedEventsTail++;
		return com_pushedEvents[( com_pushedEventsTail - 1 ) & ( MAX_PUSHED_EVENTS - 1 ) ];
	}

	return Com_GetRealEvent();
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
				CL_CharEvent( (const char *)ev.evPtr );
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
					Cbuf_AddText( cmd + 1);
					Cbuf_AddText( "\n" );
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
					Cbuf_AddText( va("%s %s\n", com_consoleCommand->string, cmd) );
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
	sysEvent_t ev;

	// get events and push them until we get a null event with the current time
	do
	{
		ev = Com_GetRealEvent();

		if ( ev.evType != SE_NONE )
		{
			Com_PushEvent( &ev );
		}
	}
	while ( ev.evType != SE_NONE );

	return ev.evTime;
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
	r_highQualityVideo = Cvar_Get( "r_highQualityVideo", "1", CVAR_ARCHIVE );
	//com_recommended = Cvar_Get("com_recommended", "-1", CVAR_ARCHIVE);
	goodVideo = ( r_highQualityVideo && r_highQualityVideo->integer );

	if ( goodVideo )
	{
		Com_Printf(_( "Found high quality video and slow CPU\n" ));
		Cbuf_AddText( "exec preset_fast.cfg\n" );
		Cvar_Set( "com_recommended", "2" );
	}
	else
	{
		Com_Printf(_( "Found low quality video and slow CPU\n" ));
		Cbuf_AddText( "exec preset_fastest.cfg\n" );
		Cvar_Set( "com_recommended", "3" );
	}
}

// bani - checks if profile.pid is valid
// return qtrue if it is
// return qfalse if it isn't(!)
qboolean Com_CheckProfile( char *profile_path )
{
	fileHandle_t f;
	char         f_data[ 32 ];
	int          f_pid;

	//let user override this
	if ( com_ignorecrash->integer )
	{
		return qtrue;
	}

	if ( FS_FOpenFileRead( profile_path, &f, qtrue ) < 0 )
	{
		//no profile found, we're ok
		return qtrue;
	}

	if ( FS_Read( &f_data, sizeof( f_data ) - 1, f ) < 0 )
	{
		//b0rk3d!
		FS_FCloseFile( f );
		//try to delete corrupted pid file
		FS_Delete( profile_path );
		return qfalse;
	}

	f_pid = atoi( f_data );

	if ( f_pid != com_pid->integer )
	{
		//pid doesn't match
		FS_FCloseFile( f );
		return qfalse;
	}

	//we're all ok
	FS_FCloseFile( f );
	return qtrue;
}

//bani - from files.c
extern char fs_gamedir[ MAX_OSPATH ];
char        last_fs_gamedir[ MAX_OSPATH ];
char        last_profile_path[ MAX_OSPATH ];

//bani - track profile changes, delete old profile.pid if we change fs_game(dir)
//hackish, we fiddle with fs_gamedir to make FS_* calls work "right"
void Com_TrackProfile( char *profile_path )
{
	char temp_fs_gamedir[ MAX_OSPATH ];

//  Com_Printf(_( "Com_TrackProfile: Tracking profile [%s] [%s]\n"), fs_gamedir, profile_path );
	//have we changed fs_game(dir)?
	if ( strcmp( last_fs_gamedir, fs_gamedir ) )
	{
		if ( strlen( last_fs_gamedir ) && strlen( last_profile_path ) )
		{
			//save current fs_gamedir
			Q_strncpyz( temp_fs_gamedir, fs_gamedir, sizeof( temp_fs_gamedir ) );
			//set fs_gamedir temporarily to make FS_* stuff work "right"
			Q_strncpyz( fs_gamedir, last_fs_gamedir, sizeof( fs_gamedir ) );

			if ( FS_FileExists( last_profile_path ) )
			{
				Com_Printf( "Com_TrackProfile: Deleting old pid file [%s] [%s]\n", fs_gamedir, last_profile_path );
				FS_Delete( last_profile_path );
			}

			//restore current fs_gamedir
			Q_strncpyz( fs_gamedir, temp_fs_gamedir, sizeof( fs_gamedir ) );
		}

		//and save current vars for future reference
		Q_strncpyz( last_fs_gamedir, fs_gamedir, sizeof( last_fs_gamedir ) );
		Q_strncpyz( last_profile_path, profile_path, sizeof( last_profile_path ) );
	}
}

// bani - writes pid to profile
// returns qtrue if successful
// returns qfalse if not(!!)
qboolean Com_WriteProfile( char *profile_path )
{
	fileHandle_t f;

	if ( FS_FileExists( profile_path ) )
	{
		FS_Delete( profile_path );
	}

	f = FS_FOpenFileWrite( profile_path );

	if ( f < 0 )
	{
		Com_Printf( "Com_WriteProfile: Can't write %s.\n", profile_path );
		return qfalse;
	}

	FS_Printf( f, "%d", com_pid->integer );

	FS_FCloseFile( f );

	//track profile changes
	Com_TrackProfile( profile_path );

	return qtrue;
}

qboolean Crypto_Init( void );

/*
=================
Com_Init
=================
*/
void Com_Init( char *commandLine )
{
	char              *s;
	int               pid, qport;

	pid = Sys_GetPID();

	Com_Printf( "%s %s %s\n%s\n", Q3_VERSION, ARCH_STRING, __DATE__, commandLine );

	if ( setjmp( abortframe ) )
	{
		Sys_Error( "Error during initialization" );
	}

	// bk001129 - do this before anything else decides to push events
	Com_InitPushEvent();

	Com_InitSmallZoneMemory();
	Cvar_Init();

	Com_DetectSSE();

	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	Com_ParseCommandLine( commandLine );

	Cbuf_Init();

	Com_InitZoneMemory();
	Cmd_Init();

	// override anything from the config files with command line args
	Com_StartupVariable( NULL );

	// get the developer cvar set as early as possible
	Com_StartupVariable( "developer" );

	// bani: init this early
	Com_StartupVariable( "com_ignorecrash" );
	com_ignorecrash = Cvar_Get( "com_ignorecrash", "0", 0 );

	// ydnar: init crashed variable as early as possible
	com_crashed = Cvar_Get( "com_crashed", "0", CVAR_TEMP );

	s = va( "%d", pid );
	com_pid = Cvar_Get( "com_pid", s, CVAR_ROM );

	// done early so bind command exists
	CL_InitKeyCommands();

	FS_InitFilesystem();
	Com_InitJournaling();

	Cbuf_AddText( "exec default.cfg\n" );

#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)
	// skip the q3config.cfg if "safe" is on the command line
	if ( !Com_SafeMode() )
	{
		char *cl_profileStr = Cvar_VariableString( "cl_profile" );

		if ( !cl_profileStr[ 0 ] )
		{
			char *defaultProfile = NULL;

			if( FS_ReadFile( "profiles/defaultprofile.dat", ( void ** ) &defaultProfile ) > 0 )
			{
				Q_CleanStr( defaultProfile );
				Q_CleanDirName( defaultProfile );

				if ( defaultProfile )
				{
					Cvar_Set( "cl_defaultProfile", defaultProfile );
					Cvar_Set( "cl_profile", defaultProfile );

					FS_FreeFile( defaultProfile );

					cl_profileStr = Cvar_VariableString( "cl_defaultProfile" );
				}
			}
		}

		if ( cl_profileStr[ 0 ] )
		{
			// bani - check existing pid file and make sure it's ok
			if ( !Com_CheckProfile( va( "profiles/%s/profile.pid", cl_profileStr ) ) )
			{
#if 0
				Com_Printf(_( "^3WARNING: profile.pid found for profile '%s' — the system settings will revert to their defaults\n"),
				            cl_profileStr );
				// ydnar: set crashed state
				Cbuf_AddText( "set com_crashed 1\n" );
#endif
			}

			// bani - write a new one
			if ( !Com_WriteProfile( va( "profiles/%s/profile.pid", cl_profileStr ) ) )
			{
				Com_Printf(_( "^3WARNING: couldn't write profiles/%s/profile.pid\n"), cl_profileStr );
			}

			// exec the config
			Cbuf_AddText( va( "exec profiles/%s/%s\n", cl_profileStr, CONFIG_NAME ) );
			Cbuf_AddText( va( "exec profiles/%s/%s\n", cl_profileStr, KEYBINDINGS_NAME ) );
			Cbuf_AddText( va( "exec profiles/%s/autoexec.cfg\n", cl_profileStr ) );
		}
		else
		{
			Cbuf_AddText( va( "exec %s\n", CONFIG_NAME ) );
			Cbuf_AddText( va( "exec %s\n", KEYBINDINGS_NAME ) );
			Cbuf_AddText( "exec autoexec.cfg\n" );
		}
	}
#else
	Cbuf_AddText( va( "exec %s\n", CONFIG_NAME ) );
#endif


	// ydnar: reset crashed state
	Cbuf_AddText( "set com_crashed 0\n" );

	// execute the queued commands
	Cbuf_Execute();

	// override anything from the config files with command line args
	Com_StartupVariable( NULL );

#if defined(DEDICATED)
	// TTimo: default to Internet dedicated, not LAN dedicated
	com_dedicated = Cvar_Get( "dedicated", "2", CVAR_ROM );
	Cvar_CheckRange( com_dedicated, 1, 2, qtrue );
#else
	com_dedicated = Cvar_Get( "dedicated", "0", CVAR_LATCH );
	Cvar_CheckRange( com_dedicated, 0, 2, qtrue );
#endif
	// allocate the stack based hunk allocator
	Com_InitHunkMemory();

	Trans_Init();

	// if any archived cvars are modified after this, we will trigger a writing
	// of the config file
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	//
	// init commands and vars
	//
	// Gordon: no need to latch this in ET, our recoil is framerate-independent
	com_maxfps = Cvar_Get( "com_maxfps", "125", CVAR_ARCHIVE /*|CVAR_LATCH */ );

	com_developer = Cvar_Get( "developer", "0", CVAR_TEMP );

	com_logfile = Cvar_Get( "logfile", "0", CVAR_TEMP );

	com_timescale = Cvar_Get( "timescale", "1", CVAR_CHEAT | CVAR_SYSTEMINFO );
	com_fixedtime = Cvar_Get( "fixedtime", "0", CVAR_CHEAT );
	com_showtrace = Cvar_Get( "com_showtrace", "0", CVAR_CHEAT );
	com_dropsim = Cvar_Get( "com_dropsim", "0", CVAR_CHEAT );
	com_viewlog = Cvar_Get( "viewlog", "0", CVAR_CHEAT );
	com_speeds = Cvar_Get( "com_speeds", "0", 0 );
	com_timedemo = Cvar_Get( "timedemo", "0", CVAR_CHEAT );

	com_watchdog = Cvar_Get( "com_watchdog", "60", CVAR_ARCHIVE );
	com_watchdog_cmd = Cvar_Get( "com_watchdog_cmd", "", CVAR_ARCHIVE );

	cl_paused = Cvar_Get( "cl_paused", "0", CVAR_ROM );
	sv_paused = Cvar_Get( "sv_paused", "0", CVAR_ROM );
	com_sv_running = Cvar_Get( "sv_running", "0", CVAR_ROM );
	com_cl_running = Cvar_Get( "cl_running", "0", CVAR_ROM );

	con_drawnotify = Cvar_Get( "con_drawnotify", "0", CVAR_CHEAT );

	//on a server, commands have to be used a lot more often than say
	//we could differentiate server and client, but would change the default behavior many might be used to
	com_consoleCommand = Cvar_Get( "com_consoleCommand", "", CVAR_ARCHIVE );

	com_introPlayed = Cvar_Get( "com_introplayed", "0", CVAR_ARCHIVE );
	com_ansiColor = Cvar_Get( "com_ansiColor", "0", CVAR_ARCHIVE );
	com_logosPlaying = Cvar_Get( "com_logosPlaying", "0", CVAR_ROM );
	com_recommendedSet = Cvar_Get( "com_recommendedSet", "0", CVAR_ARCHIVE );

	com_unfocused = Cvar_Get( "com_unfocused", "0", CVAR_ROM );
	com_maxfpsUnfocused = Cvar_Get( "com_maxfpsUnfocused", "0", CVAR_ARCHIVE );
	com_minimized = Cvar_Get( "com_minimized", "0", CVAR_ROM );
	com_maxfpsMinimized = Cvar_Get( "com_maxfpsMinimized", "0", CVAR_ARCHIVE );

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

	s = va( "%s %s %s", Q3_VERSION, ARCH_STRING, __DATE__ );
	com_version = Cvar_Get( "version", s, CVAR_ROM | CVAR_SERVERINFO );

	Sys_Init();

	// Pick a qport value that is nice and random.
	// As machines get faster, Com_Milliseconds() can't be used
	// anymore, as it results in a smaller and smaller range of
	// qport values.
	Com_RandomBytes( ( byte * )&qport, sizeof( int ) );
	Netchan_Init( qport & 0xffff );

	VM_Init();
	SV_Init();
	Hist_Load();

	if ( !Crypto_Init() )
	{
	}

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

	// start in full screen ui mode
	Cvar_Set( "r_uiFullScreen", "1" );

	CL_StartHunkUsers();

	// NERVE - SMF - force recommendedSet and don't do vid_restart if in safe mode
	if ( !com_recommendedSet->integer && !Com_SafeMode() )
	{
		Com_SetRecommended();
		Cbuf_ExecuteText( EXEC_APPEND, "vid_restart\n" );
	}

	Cvar_Set( "com_recommendedSet", "1" );

	if ( !com_dedicated->integer )
	{
		//Cvar_Set( "com_logosPlaying", "1" );
		//Cbuf_AddText( "cinematic etintro.roq\n" );

		/*Cvar_Set( "sv_nextmap", "cinematic avlogo.roq" );
		   if( !com_introPlayed->integer ) {
		   Cvar_Set( com_introPlayed->name, "1" );
		   //Cvar_Set( "sv_nextmap", "cinematic avlogo.roq" );
		   } */
	}
#ifndef _WIN32
# ifdef DEDICATED
	com_pipefile = Cvar_Get( "com_pipefile", "svpipe", CVAR_ARCHIVE | CVAR_LATCH );
# else
	com_pipefile = Cvar_Get( "com_pipefile", "pipe", CVAR_ARCHIVE | CVAR_LATCH );
# endif
#else
	com_pipefile = Cvar_Get( "com_pipefile", "", CVAR_ARCHIVE | CVAR_LATCH );
#endif
	if ( com_pipefile->string[0] )
	{
		pipefile = FS_FCreateOpenPipeFile( com_pipefile->string );
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

	while ( ( numNew = FS_Read( buf + numAccd, sizeof( buf ) - 1 - numAccd, pipefile ) ) > 0 )
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
			Cbuf_ExecuteText( EXEC_APPEND, buf );
			*brk = tmp;

			numAccd -= brk - buf;
			memmove( buf, brk, numAccd );
		}
		else if ( numAccd >= sizeof( buf ) - 1 ) // there are no CR/LF characters, but the buffer is full
		{
			// unfortunately, this command line gets chopped
			//  (but Cbuf_ExecuteText() chops long command lines at (MAX_STRING_CHARS - 1) anyway)
			buf[ sizeof( buf ) - 1 ] = '\0';
			Cbuf_ExecuteText( EXEC_APPEND, buf );
			numAccd = 0;
		}
	}
}

//==================================================================

void Com_WriteConfigToFile( const char *filename, void (*writeConfig)( fileHandle_t ) )
{
	fileHandle_t f;
	char         tmp[ MAX_QPATH + 5 ];

	Com_sprintf( tmp, sizeof( tmp ), "%s.tmp", filename );

	f = FS_FOpenFileWrite( tmp );

	if ( !f )
	{
		Com_Printf(_( "Couldn't write %s.\n"), filename );
		return;
	}

	FS_Printf( f, "// generated by Unvanquished, do not modify\n" );
//	Key_WriteBindings( f );
	writeConfig( f );
	FS_FCloseFile( f );

	FS_Rename( tmp, filename ); // will unlink the original
}

/*
===============
Com_WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void Com_WriteConfiguration( void )
{
	char *cl_profileStr = Cvar_VariableString( "cl_profile" );

	// if we are quiting without fully initializing, make sure
	// we don't write out anything
	if ( !com_fullyInitialized )
	{
		return;
	}

	if ( cvar_modifiedFlags & CVAR_ARCHIVE )
	{
		cvar_modifiedFlags &= ~CVAR_ARCHIVE;

		if ( cl_profileStr[ 0 ] )
		{
			Com_WriteConfigToFile( va( "profiles/%s/%s", cl_profileStr, CONFIG_NAME ), Cvar_WriteVariables );
		}
		else
		{
			Com_WriteConfigToFile( CONFIG_NAME, Cvar_WriteVariables );
		}
	}

#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)
	if ( bindingsModified )
	{
		bindingsModified = qfalse;

		if ( cl_profileStr[ 0 ] )
		{
			Com_WriteConfigToFile( va( "profiles/%s/%s", cl_profileStr, KEYBINDINGS_NAME ), Key_WriteBindings );
		}
		else
		{
			Com_WriteConfigToFile( KEYBINDINGS_NAME, Key_WriteBindings );
		}
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
		Com_Printf(_( "Usage: writeconfig <filename>\n" ));
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
		Com_Printf(_( "Usage: writebindings <filename>\n" ));
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
int Com_ModifyMsec( int msec )
{
	int clampTime;

	//
	// modify time for debugging values
	//
	if ( com_fixedtime->integer )
	{
		msec = com_fixedtime->integer;
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
void Com_Frame( void )
{
	int             msec, minMsec;
	static int      lastTime;
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
	if ( !com_dedicated->integer && !com_timedemo->integer )
	{
		if ( com_minimized->integer && com_maxfpsMinimized->integer > 0 )
		{
			minMsec = 1000 / com_maxfpsMinimized->integer;
		}
		else if ( com_unfocused->integer && com_maxfpsUnfocused->integer > 0 )
		{
			minMsec = 1000 / com_maxfpsUnfocused->integer;
		}
		else if ( com_maxfps->integer > 0 )
		{
			minMsec = 1000 / com_maxfps->integer;
		}
		else
		{
			minMsec = 1;
		}
	}
	else
	{
		minMsec = 0; // let's not spin at all
	}

	com_frameTime = Com_EventLoop();

	if ( lastTime > com_frameTime )
	{
		lastTime = com_frameTime; // possible on first frame
	}

	msec = com_frameTime - lastTime;

	while ( msec < minMsec )
	{
		//give cycles back to the OS
		Sys_Sleep( minMsec - msec );

		com_frameTime = Com_EventLoop();
		msec = com_frameTime - lastTime;
	}

	Cbuf_Execute();

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
		Cbuf_Execute();
		Cdelay_Frame();
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
	if ( com_dedicated->integer && !com_sv_running->integer && com_watchdog->integer )
	{
		if ( watchdogTime == 0 )
		{
			watchdogTime = Sys_Milliseconds();
		}
		else
		{
			if ( !watchWarn && Sys_Milliseconds() - watchdogTime > ( com_watchdog->integer - 4 ) * 1000 )
			{
				Com_Printf(_( "WARNING: watchdog will trigger in 4 seconds\n" ));
				watchWarn = qtrue;
			}
			else if ( Sys_Milliseconds() - watchdogTime > com_watchdog->integer * 1000 )
			{
				Com_Printf(_( "Idle server with no map — triggering watchdog\n" ));
				watchdogTime = 0;
				watchWarn = qfalse;

				if ( com_watchdog_cmd->string[ 0 ] == '\0' )
				{
					Cbuf_AddText( "quit\n" );
				}
				else
				{
					Cbuf_AddText( va( "%s\n", com_watchdog_cmd->string ) );
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
	if ( com_showtrace->integer )
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
void Com_Shutdown( qboolean badProfile )
{
	char *cl_profileStr = Cvar_VariableString( "cl_profile" );

	// delete pid file
	if ( cl_profileStr[ 0 ] && !badProfile )
	{
		if ( FS_FileExists( va( "profiles/%s/profile.pid", cl_profileStr ) ) )
		{
			FS_Delete( va( "profiles/%s/profile.pid", cl_profileStr ) );
		}
	}

	if ( logfile )
	{
		FS_FCloseFile( logfile );
		logfile = 0;
	}

	if ( com_journalFile )
	{
		FS_FCloseFile( com_journalFile );
		com_journalFile = 0;
	}

	if ( pipefile )
	{
		FS_FCloseFile( pipefile );
		FS_HomeRemove( com_pipefile->string );
	}
}

//------------------------------------------------------------------------

/*
===========================================
command line completion
===========================================
*/

/*
==================
Field_Clear
==================
*/
void Field_Clear( field_t *edit )
{
	memset( edit->buffer, 0, MAX_EDIT_LINE );
	edit->cursor = 0;
	edit->scroll = 0;
}

/*
==================
Field_SetCursor
==================
*/
void Field_SetCursor( field_t *edit, int cursor )
{
	edit->cursor = cursor;
	edit->scroll = ( cursor >= edit->widthInChars ) ? ( cursor - edit->widthInChars + 1 ) : 0;
}

/*
==================
Field_Set
==================
*/
void Field_Set( field_t *edit, const char *content )
{
	memset( edit->buffer, 0, MAX_EDIT_LINE );
	strncpy( edit->buffer, content, MAX_EDIT_LINE );
	Field_SetCursor( edit, Q_UTF8_Strlen( edit->buffer ) );
}

/*
==================
Field_CursorToOffset
==================
*/
int Field_CursorToOffset( field_t *edit )
{
	int i = -1, j = 0;

	while ( ++i < edit->cursor )
	{
		j += Q_UTF8_Width( edit->buffer + j );
	}

	return j;
}

/*
==================
Field_ScrollToOffset
==================
*/
int Field_ScrollToOffset( field_t *edit )
{
	int i = -1, j = 0;

	while ( ++i < edit->scroll )
	{
		j += Q_UTF8_Width( edit->buffer + j );
	}

	return j;
}

/*
==================
Field_OffsetToCursor
==================
*/
int Field_OffsetToCursor( field_t *edit, int offset )
{
	int i = 0, j = 0;

	while ( i < offset )
	{
		i += Q_UTF8_Width( edit->buffer + i );
		++j;
	}

	return j;
}

/*
==================
Field_WordDelete
==================
*/
void Field_WordDelete( field_t *edit )
{
	int index = Field_CursorToOffset( edit );
	int start = index;

	// search back past spaces
	while ( --index >= 0 && edit->buffer[ index ] == ' ' ) {}

	// search back past non-spaces
	if ( index ) while ( --index >= 0 && edit->buffer[ index ] != ' ' ) {}

	// strcpy would probably do, but possible overlap
	memmove( edit->buffer + index, edit->buffer + start, strlen( edit->buffer + start ) + 1 );

	Field_SetCursor( edit, Field_OffsetToCursor( edit, index ) );
}

static const char *completionString;
static char       shortestMatch[ MAX_TOKEN_CHARS ];
static int        matchCount;

// field we are working on, passed to Field_CompleteCommand (&g_consoleCommand for instance)
static field_t    *completionField;
static const char *completionPrompt;

/*
===============
FindMatches

===============
*/
static void FindMatches( const char *s )
{
	int i;

	if ( Q_stricmpn( s, completionString, strlen( completionString ) ) )
	{
		return;
	}

	matchCount++;

	if ( matchCount == 1 )
	{
		Q_strncpyz( shortestMatch, s, sizeof( shortestMatch ) );
		return;
	}

	// cut shortestMatch to the amount common with s
	for ( i = 0; s[ i ]; i++ )
	{
		if ( tolower( shortestMatch[ i ] ) != tolower( s[ i ] ) )
		{
			shortestMatch[ i ] = 0;
		}
	}

	shortestMatch[ i ] = 0;
}

/*
===============
PrintCmdMatches
===============
*/
#if 0
static void PrintCmdMatches( const char *s )
{
	if ( !Q_stricmpn( s, shortestMatch, strlen( shortestMatch ) ) )
	{
		Com_Printf( " %s\n", s );
	}
}

#endif

/*
===============
PrintMatches

===============
*/
static void PrintMatches( const char *s )
{
	if ( !Q_stricmpn( s, shortestMatch, strlen( shortestMatch ) ) )
	{
		Com_Printf( "    %s\n", s );
	}
}

/*
===============
PrintCvarMatches
===============
*/
static void PrintCvarMatches( const char *s )
{
	if ( !Q_stricmpn( s, shortestMatch, strlen( shortestMatch ) ) )
	{
		Com_Printf( " %s = \"%s\"\n", s, Cvar_VariableString( s ) );
	}
}

#if 0
static void keyConcatArgs( void )
{
	int  i;
	char *arg;

	for ( i = 1; i < Cmd_Argc(); i++ )
	{
		Q_strcat( completionField->buffer, sizeof( completionField->buffer ), " " );
		arg = Cmd_Argv( i );

		while ( *arg )
		{
			if ( *arg == ' ' )
			{
				Q_strcat( completionField->buffer, sizeof( completionField->buffer ), "\"" );
				break;
			}

			arg++;
		}

		Q_strcat( completionField->buffer, sizeof( completionField->buffer ), Cmd_Argv( i ) );

		if ( *arg == ' ' )
		{
			Q_strcat( completionField->buffer, sizeof( completionField->buffer ), "\"" );
		}
	}
}

#endif

#if 0
static void ConcatRemaining( const char *src, const char *start )
{
	char *str;

	str = strstr( src, start );

	if ( !str )
	{
		keyConcatArgs();
		return;
	}

	str += strlen( start );
	Q_strcat( completionField->buffer, sizeof( completionField->buffer ), str );
}

#endif

/*
===============
Field_FindFirstSeparator
===============
*/
static char *Field_FindFirstSeparator( char *s )
{
	int i;

	// quotes are ignored, but escapes are processed
	for ( i = 0; i < strlen( s ); i++ )
	{
		if ( s[ i ] == '\\' && s[ i + 1] )
		{
			++i;
		}
		else if ( s[ i ] == ';' )
		{
			return &s[ i ];
		}
	}

	return NULL;
}

/*
===============
Field_Complete
===============
*/
static qboolean Field_Complete( void )
{
	int completionOffset;

	if ( matchCount == 0 )
	{
		return qtrue;
	}

	completionOffset = strlen( completionField->buffer ) - strlen( completionString );

	Q_strncpyz( &completionField->buffer[ completionOffset ], shortestMatch,
	            sizeof( completionField->buffer ) - completionOffset );

	completionField->cursor = Q_UTF8_Strlen( completionField->buffer );

	if ( matchCount == 1 )
	{
		Q_strcat( completionField->buffer, sizeof( completionField->buffer ), " " );
		completionField->cursor++;
		return qtrue;
	}

	Com_Printf( "%s^7%s\n", completionPrompt, completionField->buffer );

	return qfalse;
}

#if !defined(DEDICATED)

static void Field_TeamnameCompletion( void ( *callback )( const char *s ), int flags )
{
	if ( flags & FIELD_TEAM_SPECTATORS )
	{
		callback( "spectators" );
	}

	if ( flags & FIELD_TEAM_DEFAULT )
	{
		callback( "default" );
	}

	callback( "humans" );
	callback( "aliens" );
}

/*
===============
Field_CompleteKeyname
===============
*/
void Field_CompleteKeyname( int flags )
{
	matchCount = 0;
	shortestMatch[ 0 ] = 0;

	if ( flags & FIELD_TEAM )
	{
		Field_TeamnameCompletion( FindMatches, flags );
	}

	Key_KeynameCompletion( FindMatches );

	if ( !Field_Complete() )
	{
		if ( flags & FIELD_TEAM )
		{
			Field_TeamnameCompletion( PrintMatches, flags );
		}

		Key_KeynameCompletion( PrintMatches );
	}
}

/*
===============
Field_CompleteTeamname
===============
*/
void Field_CompleteTeamname( int flags )
{
	matchCount = 0;
	shortestMatch[ 0 ] = 0;

	Field_TeamnameCompletion( FindMatches, flags );

	if ( !Field_Complete() )
	{
		Field_TeamnameCompletion( PrintMatches, flags );
	}
}

/*
===============
Field_CompleteCgame
===============
*/
void Field_CompleteCgame( int argNum )
{
	matchCount = 0;
	shortestMatch[ 0 ] = 0;

	CL_CgameCompletion( FindMatches, argNum );

	if ( !Field_Complete() )
	{
		CL_CgameCompletion( PrintMatches, argNum );
	}
}

#endif

/*
===============
Field_CompleteFilename
===============
*/
void Field_CompleteFilename( const char *dir,
                             const char *ext, qboolean stripExt )
{
	matchCount = 0;
	shortestMatch[ 0 ] = 0;

	FS_FilenameCompletion( dir, ext, stripExt, FindMatches );

	if ( !Field_Complete() )
	{
		FS_FilenameCompletion( dir, ext, stripExt, PrintMatches );
	}
}

/*
===============
Field_CompleteAlias
===============
*/
void Field_CompleteAlias( void )
{
	matchCount = 0;
	shortestMatch[ 0 ] = 0;

	Cmd_AliasCompletion( FindMatches );

	if ( !Field_Complete() )
	{
		Cmd_AliasCompletion( PrintMatches );
	}
}

/*
===============
Field_CompleteDelay
===============
*/
void Field_CompleteDelay( void )
{
	matchCount = 0;
	shortestMatch[ 0 ] = 0;

	Cmd_DelayCompletion( FindMatches );

	if ( !Field_Complete() )
	{
		Cmd_DelayCompletion( PrintMatches );
	}
}

/*
===============
Field_CompleteCommand
===============
*/
void Field_CompleteCommand( char *cmd,
                            qboolean doCommands, qboolean doCvars )
{
	int completionArgument = 0;

	// Skip leading whitespace and quotes
	cmd = Com_SkipCharset( cmd, " \"" );

	Cmd_TokenizeString( cmd );
	completionArgument = Cmd_Argc();

	// If there is trailing whitespace on the cmd
	if ( * ( cmd + strlen( cmd ) - 1 ) == ' ' )
	{
		completionString = "";
		completionArgument++;
	}
	else
	{
		completionString = Cmd_Argv( completionArgument - 1 );
	}

#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)

	// Unconditionally add a '\' to the start of the buffer
	if ( completionField->buffer[ 0 ] &&
	     completionField->buffer[ 0 ] != '\\' )
	{
		if ( completionField->buffer[ 0 ] != '/' )
		{
			// Buffer is full, refuse to complete
			if ( strlen( completionField->buffer ) + 1 >=
			     sizeof( completionField->buffer ) )
			{
				return;
			}

			memmove( &completionField->buffer[ 1 ],
			         &completionField->buffer[ 0 ],
			         strlen( completionField->buffer ) + 1 );
			completionField->cursor++;
		}

		completionField->buffer[ 0 ] = '\\';
	}

#endif

	if ( completionArgument > 1 )
	{
		const char *baseCmd = Cmd_Argv( 0 );
		char       *p;

#if !defined(DEDICATED) && !defined(BUILD_TTY_CLIENT)

		// This should always be true
		if ( baseCmd[ 0 ] == '\\' || baseCmd[ 0 ] == '/' )
		{
			baseCmd++;
		}

#endif

		if ( ( p = Field_FindFirstSeparator( cmd ) ) )
		{
			Field_CompleteCommand( p + 1, qtrue, qtrue );  // Compound command
		}
		else
		{
			Cmd_CompleteArgument( baseCmd, cmd, completionArgument );
		}
	}
	else
	{
		if ( completionString[ 0 ] == '\\' || completionString[ 0 ] == '/' )
		{
			completionString++;
		}

		matchCount = 0;
		shortestMatch[ 0 ] = 0;

		if ( strlen( completionString ) == 0 )
		{
			return;
		}

		if ( doCommands )
		{
			Cmd_CommandCompletion( FindMatches );
		}

		if ( doCvars )
		{
			Cvar_CommandCompletion( FindMatches );
		}

		if ( !Field_Complete() )
		{
			// run through again, printing matches
			if ( doCommands )
			{
				Cmd_CommandCompletion( PrintMatches );
			}

			if ( doCvars )
			{
				Cvar_CommandCompletion( PrintCvarMatches );
			}
		}
	}
}

/*
===============
Field_AutoComplete

Perform Tab expansion
===============
*/
void Field_AutoComplete( field_t *field, const char *prompt )
{
	completionField = field;
	completionPrompt = prompt;

	Field_CompleteCommand( completionField->buffer, qtrue, qtrue );
}

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

#define CON_HISTORY      64
#define CON_HISTORY_FILE "conhistory"
static char history[ CON_HISTORY ][ MAX_EDIT_LINE ];
static int  hist_current, hist_next;

/*
==================
Hist_Load
==================
*/
void Hist_Load( void )
{
	int          i;
	fileHandle_t f;
	char         *buf, *end;
	char         buffer[ sizeof( history ) ];

	FS_SV_FOpenFileRead( CON_HISTORY_FILE, &f );

	if ( !f )
	{
		Com_Printf(_( "Couldn't read %s.\n"), CON_HISTORY_FILE );
		return;
	}

	memset( buffer, '\0', sizeof( buffer ) );
	memset( history, '\0', sizeof( history ) );

	FS_Read( buffer, sizeof( buffer ), f );
	FS_FCloseFile( f );

	buf = buffer;

	for ( i = 0; i < CON_HISTORY; i++ )
	{
		end = strchr( buf, '\n' );

		if ( !end )
		{
			Q_strncpyz( history[ i ], buf, sizeof( history[ 0 ] ) );
			break;
		}

		*end = '\0';
		Q_strncpyz( history[ i ], buf, sizeof( history[ 0 ] ) );
		buf = end + 1;

		if ( !*buf )
		{
			break;
		}
	}

	hist_current = hist_next = i + 1;
}

/*
==================
Hist_Save
==================
*/
static void Hist_Save( void )
{
	int          i;
	fileHandle_t f;

	f = FS_SV_FOpenFileWrite( CON_HISTORY_FILE );

	if ( !f )
	{
		Com_Printf(_( "Couldn't write %s.\n"), CON_HISTORY_FILE );
		return;
	}

	i = ( hist_next + 1 ) % CON_HISTORY;

	do
	{
		if ( history[ i ][ 0 ] )
		{
			FS_Write( history[ i ], strlen( history[ i ] ), f );
			FS_Write( "\n", 1, f );
		}
		i = ( i + 1 ) % CON_HISTORY;
	}
	while ( i != hist_next % CON_HISTORY );

	FS_FCloseFile( f );
}

/*
==================
Hist_Add
==================
*/
void Hist_Add( const char *field )
{
	const char *prev = history[( hist_next - 1 ) % CON_HISTORY ];

	// don't add "", "\" or "/"
	if ( !field[ 0 ] ||
	     ( ( field[ 0 ] == '/' || field[ 0 ] == '\\' ) && !field[ 1 ] ) )
	{
		hist_current = hist_next;
		return;
	}

	// don't add if same as previous (treat leading \ and / as equivalent)
	if ( ( *field == *prev ||
	       ( *field == '/' && *prev == '\\' ) ||
	       ( *field == '\\' && *prev  == '/' ) ) &&
	     !strcmp( &field[ 1 ], &prev[ 1 ] ) )
	{
		hist_current = hist_next;
		return;
	}

	Q_strncpyz( history[ hist_next % CON_HISTORY ], field, sizeof( history[ 0 ] ) );
	hist_next++;
	hist_current = hist_next;
	Hist_Save();
}

/*
==================
Hist_Prev
==================
*/
const char *Hist_Prev( void )
{
	if ( ( hist_current - 1 ) % CON_HISTORY != hist_next % CON_HISTORY &&
	     history[( hist_current - 1 ) % CON_HISTORY ][ 0 ] )
	{
		hist_current--;
	}

	return history[ hist_current % CON_HISTORY ];
}

/*
==================
Hist_Next
==================
*/
const char *Hist_Next( void )
{
	if ( hist_current % CON_HISTORY != hist_next % CON_HISTORY )
	{
		hist_current++;
	}

	if ( hist_current % CON_HISTORY == hist_next % CON_HISTORY )
	{
		return NULL;
	}

	return history[ hist_current % CON_HISTORY ];
}
