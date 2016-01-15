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
#include "qcommon/q_shared.h"
#include "q_unicode.h"
#include "qcommon.h"

#include "framework/Application.h"
#include "framework/BaseCommands.h"
#include "framework/CommandSystem.h"
#include "framework/CvarSystem.h"
#include "framework/ConsoleHistory.h"
#include "framework/LogSystem.h"
#include "framework/System.h"
#include <common/FileSystem.h>

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

#define MIN_COMHUNKMEGS 256
#define DEF_COMHUNKMEGS 512

static fileHandle_t logfile;

cvar_t              *com_crashed = nullptr; // ydnar: set in case of a crash, prevents CVAR_UNSAFE variables from being set from a cfg

cvar_t *com_pid; // bani - process id

cvar_t *com_speeds;
cvar_t *com_developer;
cvar_t *com_timescale;
cvar_t *com_dropsim; // 0.0 to 1.0, simulated packet drops
cvar_t *com_timedemo;
cvar_t *com_sv_running;
cvar_t *com_cl_running;
cvar_t *com_logfile; // 1 = buffer log, 2 = flush after each print, 3 = append + flush
cvar_t *com_version;

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
int      com_hunkusedvalue;

bool com_fullyInitialized;

void     Com_WriteConfig_f();
void     Com_WriteBindings_f();
void     CIN_CloseAllVideos();

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
	static SDL_mutex *lock = nullptr;

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

void QDECL Com_LogEvent( log_event_t *event )
{
	switch (event->level)
	{
	case LOG_OFF:
		return;
	case LOG_WARN:
		Com_Printf("^3Warning: ^7%s\n", event->message);
		break;
	case LOG_ERROR:
		Com_Printf("^1Error: ^7%s\n", event->message);
		break;
	case LOG_DEBUG:
		Com_Printf("Debug: %s\n", event->message);
		break;
	case LOG_TRACE:
		Com_Printf("Trace: %s\n", event->message);
		return;
	default:
		Com_Printf("%s\n", event->message);
		break;
	}
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

	Com_LogEvent( &event );
}

void QDECL Com_Log( log_level_t level, const char* message )
{
	log_event_t event;
	event.level = level;
	event.message = message;
	Com_LogEvent( &event );
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
void QDECL PRINTF_LIKE(2) Com_Error( int code, const char *fmt, ... )
{
	char buf[ 4096 ];
	va_list argptr;

	va_start( argptr, fmt );
	Q_vsnprintf( buf, sizeof( buf ), fmt, argptr );
	va_end( argptr );

	if ( code == ERR_FATAL )
		Sys::Error( buf );
	else
		Sys::Drop( buf );
}

// *INDENT-OFF*
//bani - moved
void CL_ShutdownCGame();

// *INDENT-ON*

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
===============
Com_StartupVariable

Searches for command-line arguments that are set commands.
If match is not nullptr, only that cvar will be looked for.
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
		if (com_consoleLines[i] == 0) {
			continue;
		}

		Cmd::Args line(com_consoleLines[i]);

		if ( line.size() < 3 || strcmp( line[0].c_str(), "set" ))
		{
			continue;
		}

		s = line[1].c_str();

		if ( !match || !strcmp( s, match ) )
		{
			Cvar_Set( s, line[2].c_str() );
			cv = Cvar_Get( s, "", CVAR_USER_CREATED );
			if (cv->flags & CVAR_ROM) {
				com_consoleLines[i] = 0;
			}
		}
	}
}

//============================================================================

void Info_Print( const char *s )
{
	char key[ 8192 ];
	char value[ 8192 ];
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
==============================================================================
Global common state
==============================================================================
*/

void SetCheatMode(bool allowed)
{
	Cvar::SetCheatsAllowed(allowed);
}

//The server gives the sv_cheats cvar to the client, on 'off' it prevents the user from changing Cvar::CHEAT cvars
Cvar::Callback<Cvar::Cvar<bool>> cvar_cheats("sv_cheats", "can cheats be used in the current game", Cvar::SYSTEMINFO | Cvar::ROM, true, SetCheatMode);

bool Com_AreCheatsAllowed()
{
	return cvar_cheats.Get();
}

bool Com_IsClient()
{
    auto config = Application::GetTraits();
    return config.isClient || config.isTTYClient;
}

bool Com_IsDedicatedServer()
{
    return !Com_IsClient();
}

bool Com_ServerRunning()
{
	return com_sv_running->integer;
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

struct hunkHeader_t
{
	int magic;
	int size;
};

struct hunkUsed_t
{
	int mark;
	int permanent;
	int temp;
	int tempHighwater;
};

struct hunkblock_t
{
	int                size;
	byte               printed;
	hunkblock_t *next;

	const char         *label;
	const char         *file;
	int                line;
};
// for alignment purposes
#define SIZEOF_HUNKBLOCK_T ( ( sizeof( hunkblock_t ) + 31 ) & ~31 )

static hunkUsed_t  hunk_low, hunk_high;
static hunkUsed_t  *hunk_permanent, *hunk_temp;

static byte        *s_hunkData = nullptr;
static int         s_hunkTotal;

/*
=================
Com_Meminfo_f
=================
*/
void Com_Meminfo_f()
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
=================
Com_Allocate_Aligned

Aligned Memory Allocations for Posix and Win32
=================
*/
void *Com_Allocate_Aligned( size_t alignment, size_t size )
{
#ifdef _WIN32
	return _aligned_malloc( size, alignment );
#else
	void *ptr;
	if( !posix_memalign( &ptr, alignment, size ) )
		return ptr;
	else
		return nullptr;
#endif
}

/*
=================
Com_Free_Aligned

Free Aligned Memory for Posix and Win32
=================
*/
void Com_Free_Aligned( void *ptr )
{
#ifdef _WIN32
	_aligned_free( ptr );
#else
	free( ptr );
#endif
}

/*
=================
Com_InitHunkMemory
=================
*/
void Com_InitHunkMemory()
{
	cvar_t *cv;

	// allocate the stack based hunk allocator
	cv = Cvar_Get( "com_hunkMegs", XSTRING(DEF_COMHUNKMEGS), CVAR_LATCH  );

	if ( cv->integer < MIN_COMHUNKMEGS )
	{
		s_hunkTotal = 1024 * 1024 * MIN_COMHUNKMEGS;
		Com_Printf( "Minimum com_hunkMegs is " XSTRING(MIN_COMHUNKMEGS) ", allocating " XSTRING(MIN_COMHUNKMEGS) "MB.\n" );
	}
	else
	{
		s_hunkTotal = cv->integer * 1024 * 1024;
	}

	// cacheline aligned
	s_hunkData = ( byte * ) Com_Allocate_Aligned( 64, s_hunkTotal );

	if ( !s_hunkData )
	{
		Com_Error( ERR_FATAL, "Hunk data failed to allocate %iMB", s_hunkTotal / ( 1024 * 1024 ) );
	}

	Hunk_Clear();

	Cmd_AddCommand( "meminfo", Com_Meminfo_f );
}

/*
===================
Hunk_SetMark

The server calls this after the level and game VM have been loaded
===================
*/
void Hunk_SetMark()
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
void Hunk_ClearToMark()
{
	hunk_low.permanent = hunk_low.temp = hunk_low.mark;
	hunk_high.permanent = hunk_high.temp = hunk_high.mark;
}

void SV_ShutdownGameProgs();

/*
=================
Hunk_Clear

The server calls this before shutting down or loading a new map
=================
*/
void Hunk_Clear()
{
#ifdef BUILD_CLIENT
	CL_ShutdownCGame();
#endif
	SV_ShutdownGameProgs();
#ifdef BUILD_CLIENT
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
}

static void Hunk_SwapBanks()
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
void           *Hunk_Alloc( int size, ha_pref)
{
	void *buf;

	if ( s_hunkData == nullptr )
	{
		Com_Error( ERR_FATAL, "Hunk_Alloc: Hunk memory system not initialized" );
	}

	Hunk_SwapBanks();

	// round to cacheline
	size = ( size + 31 ) & ~31;

	if ( hunk_low.temp + hunk_high.temp + size > s_hunkTotal )
	{
		Com_Error( ERR_DROP, "Hunk_Alloc failed on %i", size );
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
	if ( s_hunkData == nullptr )
	{
		return Z_Malloc( size );
	}

	Hunk_SwapBanks();

	size = PAD( size, sizeof( intptr_t ) ) + sizeof( hunkHeader_t );

	if ( hunk_temp->temp + hunk_permanent->permanent + size > s_hunkTotal )
	{
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
	if ( s_hunkData == nullptr )
	{
		Z_Free( buf );
		return;
	}

	hdr = ( ( hunkHeader_t * ) buf ) - 1;

	if ( hdr->magic != (int) HUNK_MAGIC )
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

#define MAX_QUEUED_EVENTS  1024
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
sysEvent_t Com_GetEvent()
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
	s = CON_Input();

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

#ifdef BUILD_CLIENT
extern bool consoleButtonWasPressed;
#endif

int Com_EventLoop()
{
	sysEvent_t ev;
	netadr_t   evFrom;
	byte       bufData[ MAX_MSGLEN ];
	msg_t      buf;

	int        mouseX = 0, mouseY = 0, mouseTime = 0;
	bool       mouseHaveEvent = false;

	MSG_Init( &buf, bufData, sizeof( bufData ) );

	while ( 1 )
	{
		ev = Com_GetEvent();

		// if no more events are available
		if ( ev.evType == SE_NONE )
		{
			if ( mouseHaveEvent ){
				CL_MouseEvent( mouseX, mouseY, mouseTime );
			}

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
#ifdef BUILD_CLIENT

				// fretn
				// we just pressed the console button,
				// so ignore this event
				// this prevents chars appearing at console input
				// when you just opened it
				if ( consoleButtonWasPressed )
				{
					consoleButtonWasPressed = false;
					break;
				}

#endif
				CL_CharEvent( ev.evValue );
				break;

			case SE_MOUSE:
				mouseHaveEvent = true;
				mouseX += ev.evValue;
				mouseY += ev.evValue2;
				mouseTime += ev.evTime;
				break;

			case SE_MOUSE_POS:
				CL_MousePosEvent( ev.evValue, ev.evValue2 );
				break;

			case SE_FOCUS:
				CL_FocusEvent( ev.evValue );
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
					 * (and will so for BUILD_SERVER)
					 *
					 * the additional space gets trimmed by the parser
					 */
					Cmd::BufferCommandTextAfter(va("%s %s", com_consoleCommand.Get().c_str(), cmd), true);
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
				if ( buf.cursize > buf.maxsize )
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
int Com_Milliseconds()
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
static void Com_Error_f()
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
static void Com_Freeze_f()
{
	float s;
	int   start, now;

	if ( Cmd_Argc() != 2 )
	{
		Com_Printf( "freeze <seconds>\n" );
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
static void NORETURN Com_Crash_f()
{
	* ( volatile int * ) 0 = 0x12345678;
	Sys::OSExit(1); // silence warning
}

void Com_SetRecommended()
{
	cvar_t   *r_highQualityVideo; //, *com_recommended;
	bool goodVideo;

	// will use this for recommended settings as well.. do i outside the lower check so it gets done even with command line stuff
	r_highQualityVideo = Cvar_Get( "r_highQualityVideo", "1", 0 );
	//com_recommended = Cvar_Get("com_recommended", "-1", 0);
	goodVideo = ( r_highQualityVideo && r_highQualityVideo->integer );

	if ( goodVideo )
	{
		Com_Printf( "Found high quality video and slow CPU\n" );
		Cmd::BufferCommandText("preset preset_fast.cfg");
		Cvar_Set( "com_recommended", "2" );
	}
	else
	{
		Com_Printf( "Found low quality video and slow CPU\n" );
		Cmd::BufferCommandText("preset preset_fastest.cfg");
		Cvar_Set( "com_recommended", "3" );
	}
}

void Com_In_Restart_f()
{
	IN_Restart();
}

/*
=================
Com_Init
=================
*/
void Com_Init( char *commandLine )
{
	char              *s;
	int               qport;

	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	Com_ParseCommandLine( commandLine );

	// override anything from the config files with command line args
	Com_StartupVariable( nullptr );

	// get the developer cvar set as early as possible
	Com_StartupVariable( "developer" );

	// bani: init this early
	Com_StartupVariable( "com_ignorecrash" );

	// ydnar: init crashed variable as early as possible
	com_crashed = Cvar_Get( "com_crashed", "0", CVAR_TEMP );

	Trans_Init();

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
	com_speeds = Cvar_Get( "com_speeds", "0", 0 );
	com_timedemo = Cvar_Get( "timedemo", "0", CVAR_CHEAT );

	cl_paused = Cvar_Get( "cl_paused", "0", CVAR_ROM );
	sv_paused = Cvar_Get( "sv_paused", "0", CVAR_ROM );
	com_sv_running = Cvar_Get( "sv_running", "0", CVAR_ROM );
	com_cl_running = Cvar_Get( "cl_running", "0", CVAR_ROM );

	com_introPlayed = Cvar_Get( "com_introplayed", "0", 0 );
	com_logosPlaying = Cvar_Get( "com_logosPlaying", "0", CVAR_ROM );
	com_recommendedSet = Cvar_Get( "com_recommendedSet", "0", 0 );

	com_unfocused = Cvar_Get( "com_unfocused", "0", CVAR_ROM );
	com_minimized = Cvar_Get( "com_minimized", "0", CVAR_ROM );

#if defined( _WIN32 ) && !defined( NDEBUG )
	com_noErrorInterrupt = Cvar_Get( "com_noErrorInterrupt", "0", 0 );
#endif

	com_hunkused = Cvar_Get( "com_hunkused", "0", 0 );
	com_hunkusedvalue = 0;

	if ( com_developer && com_developer->integer )
	{
		Cmd_AddCommand( "error", Com_Error_f );
		Cmd_AddCommand( "crash", Com_Crash_f );
		Cmd_AddCommand( "freeze", Com_Freeze_f );
	}

	Cmd_AddCommand( "writeconfig", Com_WriteConfig_f );
#ifndef BUILD_SERVER
	Cmd_AddCommand( "writebindings", Com_WriteBindings_f );
#endif

	s = va( "%s %s %s %s", Q3_VERSION, PLATFORM_STRING, ARCH_STRING, __DATE__ );
	com_version = Cvar_Get( "version", s, CVAR_ROM | CVAR_SERVERINFO );

	Cmd_AddCommand( "in_restart", Com_In_Restart_f );

	// Pick a qport value that is nice and random.
	// As machines get faster, Com_Milliseconds() can't be used
	// anymore, as it results in a smaller and smaller range of
	// qport values.
	Sys::GenRandomBytes( &qport, sizeof( int ) );
	Netchan_Init( qport & 0xffff );

	SV_Init();

	CL_Init();

	// set com_frameTime so that if a map is started on the
	// command line it will still be able to count on com_frameTime
	// being random enough for a serverid
	com_frameTime = Com_Milliseconds();

	CL_StartHunkUsers();

	com_fullyInitialized = true;
	Com_Printf( "%s", "--- Common Initialization Complete ---\n" );

	NET_Init();
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
		Com_Printf( "Couldn't write %s.\n", filename );
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
void Com_WriteConfiguration()
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

#ifdef BUILD_CLIENT
	if ( bindingsModified )
	{
		bindingsModified = false;

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
void Com_WriteConfig_f()
{
	char filename[ MAX_QPATH ];

	if ( Cmd_Argc() != 2 )
	{
		Cmd_PrintUsage("<filename>", nullptr);
		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );
	Com_Printf( "Writing %s.\n", filename );
	Com_WriteConfigToFile( filename, Cvar_WriteVariables );
}

/*
===============
Com_WriteBindings_f

Write the key bindings file to a specific name
===============
*/
#ifndef BUILD_SERVER
void Com_WriteBindings_f()
{
	char filename[ MAX_QPATH ];

	if ( Cmd_Argc() != 2 )
	{
		Cmd_PrintUsage("<filename>", nullptr);
		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );
	Com_Printf( "Writing %s.\n", filename );
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

	if ( Com_IsDedicatedServer() )
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
	else if ( !Com_ServerRunning() )
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

void Com_Frame()
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
	static bool watchWarn = false;

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
		if ( Com_IsDedicatedServer() )
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

	IN_Frame(); // must be called at least once

	while ( msec < minMsec )
	{
		//give cycles back to the OS
		Sys::SleepFor(std::chrono::milliseconds(std::min(minMsec - msec, 50)));
		IN_Frame();

		com_frameTime = Com_EventLoop();
		msec = com_frameTime - lastTime;
	}

	IN_FrameEnd();

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

	//
	// client system
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

	if ( com_speeds->integer )
	{
		timeBeforeClient = Sys_Milliseconds();
	}

	CL_Frame( msec );

	if ( com_speeds->integer )
	{
		timeAfter = Sys_Milliseconds();
	}

	//
	// watchdog
	//
	if ( Com_IsDedicatedServer() && !Com_ServerRunning() && watchdogThreshold.Get() != 0 )
	{
		if ( watchdogTime == 0 )
		{
			watchdogTime = Sys_Milliseconds();
		}
		else
		{
			if ( !watchWarn && Sys_Milliseconds() - watchdogTime > ( watchdogThreshold.Get() - 4 ) * 1000 )
			{
				Com_Log( LOG_WARN, "watchdog will trigger in 4 seconds" );
				watchWarn = true;
			}
			else if ( Sys_Milliseconds() - watchdogTime > watchdogThreshold.Get() * 1000 )
			{
				Com_Printf( "Idle server with no map — triggering watchdog\n" );
				watchdogTime = 0;
				watchWarn = false;

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

	FS::FlushAll();
}

int Sys_Milliseconds()
{
	static Sys::SteadyClock::time_point baseTime = Sys::SteadyClock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(Sys::SteadyClock::now() - baseTime).count();
}
