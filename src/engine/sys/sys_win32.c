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
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"
#include "sys_win32.h"

#include <windows.h>
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <psapi.h>
#include <float.h>

// Used to determine where to store user-specific files
static char homePath[ MAX_OSPATH ] = { 0 };

#ifndef DEDICATED
static UINT timerResolution = 0;
#endif

/*
================
Sys_SetFPUCW
Set FPU control word to default value
================
*/

#ifndef _RC_NEAR
// mingw doesn't seem to have these defined :(

#define _MCW_EM  0x0008001fU
#define _MCW_RC  0x00000300U
#define _MCW_PC  0x00030000U
#define _RC_NEAR 0x00000000U
#define _PC_53   0x00010000U

unsigned int _controlfp( unsigned int new, unsigned int mask );

#endif

#define FPUCWMASK1 ( _MCW_RC | _MCW_EM )
#define FPUCW      ( _RC_NEAR | _MCW_EM | _PC_53 )

#if idx64
#define FPUCWMASK  ( FPUCWMASK1 )
#else
#define FPUCWMASK  ( FPUCWMASK1 | _MCW_PC )
#endif

void Sys_SetFloatEnv( void )
{
	_controlfp( FPUCW, FPUCWMASK );
}

/*
================
Sys_DefaultHomePath
================
*/
char *Sys_DefaultHomePath( void )
{
	TCHAR   szPath[ MAX_PATH ];
	FARPROC qSHGetFolderPath;
	HMODULE shfolder = LoadLibrary( "shfolder.dll" );

	if ( !*homePath )
	{
		if ( shfolder == NULL )
		{
			Com_Printf( "Unable to load SHFolder.dll\n" );
			return NULL;
		}

		qSHGetFolderPath = GetProcAddress( shfolder, "SHGetFolderPathA" );

		if ( qSHGetFolderPath == NULL )
		{
			Com_Printf( "Unable to find SHGetFolderPath in SHFolder.dll\n" );
			FreeLibrary( shfolder );
			return NULL;
		}

		if ( !SUCCEEDED( qSHGetFolderPath( NULL, CSIDL_APPDATA,
		                                   NULL, 0, szPath ) ) )
		{
			Com_Printf( "Unable to detect CSIDL_APPDATA\n" );
			FreeLibrary( shfolder );
			return NULL;
		}

		Q_strncpyz( homePath, szPath, sizeof( homePath ) );
		Q_strcat( homePath, sizeof( homePath ), "\\Daemon" );
		FreeLibrary( shfolder );
	}

	return homePath;
}

/*
================
Sys_Milliseconds
================
*/
int sys_timeBase;
int Sys_Milliseconds( void )
{
	int             sys_curtime;
	static qboolean initialized = qfalse;

	if ( !initialized )
	{
		sys_timeBase = timeGetTime();
		initialized = qtrue;
	}

	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}

/*
================
Sys_RandomBytes
================
*/
qboolean Sys_RandomBytes( byte *string, int len )
{
	HCRYPTPROV prov;

	if ( !CryptAcquireContext( &prov, NULL, NULL,
	                           PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )
	{
		return qfalse;
	}

	if ( !CryptGenRandom( prov, len, ( BYTE * ) string ) )
	{
		CryptReleaseContext( prov, 0 );
		return qfalse;
	}

	CryptReleaseContext( prov, 0 );
	return qtrue;
}

/*
================
Sys_GetCurrentUser
================
*/
char *Sys_GetCurrentUser( void )
{
	static char   s_userName[ 1024 ];
	unsigned long size = sizeof( s_userName );

	if ( !GetUserName( s_userName, &size ) )
	{
		strcpy( s_userName, "player" );
	}

	if ( !s_userName[ 0 ] )
	{
		strcpy( s_userName, "player" );
	}

	return s_userName;
}

/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData( clipboard_t clip )
{
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 )
	{
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 )
		{
			if ( ( cliptext = GlobalLock( hClipboardData ) ) != 0 )
			{
				data = Z_Malloc( GlobalSize( hClipboardData ) + 1 );
				Q_strncpyz( data, cliptext, GlobalSize( hClipboardData ) );
				GlobalUnlock( hClipboardData );

				strtok( data, "\n\r\b" );
			}
		}

		CloseClipboard();
	}

	return data;
}

#define MEM_THRESHOLD 96 * 1024 * 1024

/*
==================
Sys_LowPhysicalMemory
==================
*/
qboolean Sys_LowPhysicalMemory( void )
{
	MEMORYSTATUS stat;
	GlobalMemoryStatus( &stat );
	return ( stat.dwTotalPhys <= MEM_THRESHOLD ) ? qtrue : qfalse;
}

/*
==============
Sys_Basename
==============
*/
const char *Sys_Basename( char *path )
{
	static char base[ MAX_OSPATH ] = { 0 };
	int         length;

	length = strlen( path ) - 1;

	// Skip trailing slashes
	while ( length > 0 && path[ length ] == '\\' )
	{
		length--;
	}

	while ( length > 0 && path[ length - 1 ] != '\\' )
	{
		length--;
	}

	Q_strncpyz( base, &path[ length ], sizeof( base ) );

	length = strlen( base ) - 1;

	// Strip trailing slashes
	while ( length > 0 && base[ length ] == '\\' )
	{
		base[ length-- ] = '\0';
	}

	return base;
}

/*
==============
Sys_Dirname
==============
*/
const char *Sys_Dirname( char *path )
{
	static char dir[ MAX_OSPATH ] = { 0 };
	int         length;

	Q_strncpyz( dir, path, sizeof( dir ) );
	length = strlen( dir ) - 1;

	while ( length > 0 && dir[ length ] != '\\' )
	{
		length--;
	}

	dir[ length ] = '\0';

	return dir;
}

/*
==============
Sys_Mkdir
==============
*/
qboolean Sys_Mkdir( const char *path )
{
	if ( !CreateDirectory( path, NULL ) )
	{
		if ( GetLastError() != ERROR_ALREADY_EXISTS )
		{
			return qfalse;
		}
	}

	return qtrue;
}

/*
==================
Sys_Mkfifo
Noop on windows because named pipes do not function the same way
==================
*/
FILE *Sys_Mkfifo( const char *ospath )
{
	return NULL;
}


/*
==============
Sys_Cwd
==============
*/
char *Sys_Cwd( void )
{
	static char cwd[ MAX_OSPATH ];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[ MAX_OSPATH - 1 ] = 0;

	return cwd;
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define MAX_FOUND_FILES 0x1000

/*
==============
Sys_ListFilteredFiles
==============
*/
void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **list, int *numfiles )
{
	char               search[ MAX_OSPATH ], newsubdirs[ MAX_OSPATH ];
	char               filename[ MAX_OSPATH ];
	intptr_t           findhandle;
	struct _finddata_t findinfo;

	if ( *numfiles >= MAX_FOUND_FILES - 1 )
	{
		return;
	}

	if ( strlen( subdirs ) )
	{
		Com_sprintf( search, sizeof( search ), "%s\\%s\\*", basedir, subdirs );
	}
	else
	{
		Com_sprintf( search, sizeof( search ), "%s\\*", basedir );
	}

	findhandle = _findfirst( search, &findinfo );

	if ( findhandle == -1 )
	{
		return;
	}

	do
	{
		if ( findinfo.attrib & _A_SUBDIR )
		{
			if ( Q_stricmp( findinfo.name, "." ) && Q_stricmp( findinfo.name, ".." ) )
			{
				if ( strlen( subdirs ) )
				{
					Com_sprintf( newsubdirs, sizeof( newsubdirs ), "%s\\%s", subdirs, findinfo.name );
				}
				else
				{
					Com_sprintf( newsubdirs, sizeof( newsubdirs ), "%s", findinfo.name );
				}

				Sys_ListFilteredFiles( basedir, newsubdirs, filter, list, numfiles );
			}
		}

		if ( *numfiles >= MAX_FOUND_FILES - 1 )
		{
			break;
		}

		Com_sprintf( filename, sizeof( filename ), "%s\\%s", subdirs, findinfo.name );

		if ( !Com_FilterPath( filter, filename, qfalse ) )
		{
			continue;
		}

		list[ *numfiles ] = CopyString( filename );
		( *numfiles ) ++;
	}
	while ( _findnext( findhandle, &findinfo ) != -1 );

	_findclose( findhandle );
}

/*
==============
strgtr
==============
*/
static qboolean strgtr( const char *s0, const char *s1 )
{
	int l0, l1, i;

	l0 = strlen( s0 );
	l1 = strlen( s1 );

	if ( l1 < l0 )
	{
		l0 = l1;
	}

	for ( i = 0; i < l0; i++ )
	{
		if ( s1[ i ] > s0[ i ] )
		{
			return qtrue;
		}

		if ( s1[ i ] < s0[ i ] )
		{
			return qfalse;
		}
	}

	return qfalse;
}

/*
==============
Sys_ListFiles
==============
*/
char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs )
{
	char               search[ MAX_OSPATH ];
	int                nfiles;
	char               **listCopy;
	char               *list[ MAX_FOUND_FILES ];
	struct _finddata_t findinfo;

	intptr_t           findhandle;
	int                flag;
	int                i;

	if ( filter )
	{
		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );

		list[ nfiles ] = 0;
		*numfiles = nfiles;

		if ( !nfiles )
		{
			return NULL;
		}

		listCopy = Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );

		for ( i = 0; i < nfiles; i++ )
		{
			listCopy[ i ] = list[ i ];
		}

		listCopy[ i ] = NULL;

		return listCopy;
	}

	if ( !extension )
	{
		extension = "";
	}

	// passing a slash as extension will find directories
	if ( extension[ 0 ] == '/' && extension[ 1 ] == 0 )
	{
		extension = "";
		flag = 0;
	}
	else
	{
		flag = _A_SUBDIR;
	}

	Com_sprintf( search, sizeof( search ), "%s\\*%s", directory, extension );

	// search
	nfiles = 0;

	findhandle = _findfirst( search, &findinfo );

	if ( findhandle == -1 )
	{
		*numfiles = 0;
		return NULL;
	}

	do
	{
		if ( ( !wantsubs && flag ^ ( findinfo.attrib & _A_SUBDIR ) ) || ( wantsubs && findinfo.attrib & _A_SUBDIR ) )
		{
			if ( nfiles == MAX_FOUND_FILES - 1 )
			{
				break;
			}

			list[ nfiles ] = CopyString( findinfo.name );
			nfiles++;
		}
	}
	while ( _findnext( findhandle, &findinfo ) != -1 );

	list[ nfiles ] = 0;

	_findclose( findhandle );

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles )
	{
		return NULL;
	}

	listCopy = Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );

	for ( i = 0; i < nfiles; i++ )
	{
		listCopy[ i ] = list[ i ];
	}

	listCopy[ i ] = NULL;

	do
	{
		flag = 0;

		for ( i = 1; i < nfiles; i++ )
		{
			if ( strgtr( listCopy[ i - 1 ], listCopy[ i ] ) )
			{
				char *temp = listCopy[ i ];
				listCopy[ i ] = listCopy[ i - 1 ];
				listCopy[ i - 1 ] = temp;
				flag = 1;
			}
		}
	}
	while ( flag );

	return listCopy;
}

/*
==============
Sys_FreeFileList
==============
*/
void Sys_FreeFileList( char **list )
{
	int i;

	if ( !list )
	{
		return;
	}

	for ( i = 0; list[ i ]; i++ )
	{
		Z_Free( list[ i ] );
	}

	Z_Free( list );
}

/*
==============
Sys_Sleep

Block execution for msec or until input is received.
==============
*/
void Sys_Sleep( int msec )
{
	if ( msec == 0 )
	{
		return;
	}

#ifdef DEDICATED

	if ( msec < 0 )
	{
		WaitForSingleObject( GetStdHandle( STD_INPUT_HANDLE ), INFINITE );
	}
	else
	{
		WaitForSingleObject( GetStdHandle( STD_INPUT_HANDLE ), msec );
	}

#else

	// Client Sys_Sleep doesn't support waiting on stdin
	if ( msec < 0 )
	{
		return;
	}

	Sleep( msec );
#endif
}

/*
==============
Sys_ErrorDialog

Display an error message
==============
*/
void Sys_ErrorDialog( const char *error )
{
	if ( Sys_Dialog( DT_YES_NO, va( "%s. Copy console log to clipboard?", error ),
	                 "Error" ) == DR_YES )
	{
		HGLOBAL memoryHandle;
		char    *clipMemory;

		memoryHandle = GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, CON_LogSize() + 1 );
		clipMemory = ( char * ) GlobalLock( memoryHandle );

		if ( clipMemory )
		{
			char         *p = clipMemory;
			char         buffer[ 1024 ];
			unsigned int size;

			while ( ( size = CON_LogRead( buffer, sizeof( buffer ) ) ) > 0 )
			{
				Com_Memcpy( p, buffer, size );
				p += size;
			}

			*p = '\0';

			if ( OpenClipboard( NULL ) && EmptyClipboard() )
			{
				SetClipboardData( CF_TEXT, memoryHandle );
			}

			GlobalUnlock( clipMemory );
			CloseClipboard();
		}
	}
}

/*
==============
Sys_Dialog

Display a win32 dialog box
==============
*/
dialogResult_t Sys_Dialog( dialogType_t type, const char *message, const char *title )
{
	UINT uType;

	switch ( type )
	{
		default:
		case DT_INFO:
			uType = MB_ICONINFORMATION | MB_OK;
			break;

		case DT_WARNING:
			uType = MB_ICONWARNING | MB_OK;
			break;

		case DT_ERROR:
			uType = MB_ICONERROR | MB_OK;
			break;

		case DT_YES_NO:
			uType = MB_ICONQUESTION | MB_YESNO;
			break;

		case DT_OK_CANCEL:
			uType = MB_ICONWARNING | MB_OKCANCEL;
			break;
	}

	switch ( MessageBox( NULL, message, title, uType ) )
	{
		default:
		case IDOK:
			return DR_OK;

		case IDCANCEL:
			return DR_CANCEL;

		case IDYES:
			return DR_YES;

		case IDNO:
			return DR_NO;
	}
}

#ifndef DEDICATED
static qboolean SDL_VIDEODRIVER_externallySet = qfalse;
#endif

/*
==============
Sys_GLimpSafeInit

Windows specific "safe" GL implementation initialisation
==============
*/
void Sys_GLimpSafeInit( void )
{
#ifndef DEDICATED

	if ( !SDL_VIDEODRIVER_externallySet )
	{
		// Here, we want to let SDL decide what do to unless
		// explicitly requested otherwise
		_putenv( "SDL_VIDEODRIVER=" );
	}

#endif
}

/*
==============
Sys_GLimpInit

Windows specific GL implementation initialisation
==============
*/
void Sys_GLimpInit( void )
{
#ifndef DEDICATED

	if ( !SDL_VIDEODRIVER_externallySet )
	{
		// It's a little bit weird having in_mouse control the
		// video driver, but from ioq3's point of view they're
		// virtually the same except for the mouse input anyway
		if ( Cvar_VariableIntegerValue( "in_mouse" ) == -1 )
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
}

/*
==============
Sys_PlatformInit

Windows specific initialisation
==============
*/
void Sys_PlatformInit( void )
{
#ifndef DEDICATED
	TIMECAPS ptc;
	const char *SDL_VIDEODRIVER = getenv( "SDL_VIDEODRIVER" );
#endif

	Sys_SetFloatEnv();

#ifndef DEDICATED

	if ( SDL_VIDEODRIVER )
	{
		Com_Printf( "SDL_VIDEODRIVER is externally set to \"%s\", "
		            "in_mouse -1 will have no effect\n", SDL_VIDEODRIVER );
		SDL_VIDEODRIVER_externallySet = qtrue;
	}
	else
	{
		SDL_VIDEODRIVER_externallySet = qfalse;
	}

	
	if(timeGetDevCaps(&ptc, sizeof(ptc)) == MMSYSERR_NOERROR)
	{
		timerResolution = ptc.wPeriodMin;
		if(timerResolution > 1)
		{
			Com_Printf("Warning: Minimum supported timer resolution is %ums "
					"on this system, recommended resolution 1ms\n", timerResolution);
		}

		timeBeginPeriod(timerResolution);
	}
	else
		timerResolution = 0;
#endif
}
/*
==============
Sys_PlatformExit

Windows specific deinitialisation
==============
*/
void Sys_PlatformExit( void )
{
#ifndef DEDICATED
	if(timerResolution)
		timeEndPeriod(timerResolution);
#endif
}

/*
==============
Sys_GetPID
==============
*/
int Sys_GetPID( void )
{
	return GetCurrentProcessId();
}

/*
========================================================================

EVENT LOOP

========================================================================
*/

#define MAX_QUED_EVENTS  256
#define MASK_QUED_EVENTS ( MAX_QUED_EVENTS - 1 )

sysEvent_t eventQue[ MAX_QUED_EVENTS ];
int        eventHead, eventTail;
byte       sys_packetReceived[ MAX_MSGLEN ];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr )
{
	sysEvent_t *ev;

	ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];

	if ( eventHead - eventTail >= MAX_QUED_EVENTS )
	{
		Com_Printf( "Sys_QueEvent: overflow\n" );

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

static double   pfreq;
static qboolean hwtimer = qfalse;

double Sys_DoubleTime( void )
{
	__int64         pcount;
	static __int64  startcount;
	static DWORD    starttime;
	static qboolean first = qtrue;
	DWORD           now;

	if ( hwtimer )
	{
		QueryPerformanceCounter( ( LARGE_INTEGER * ) &pcount );

		if ( first )
		{
			first = qfalse;
			startcount = pcount;
			return 0.0;
		}

		return ( pcount - startcount ) / pfreq;
	}

	now = timeGetTime();

	if ( first )
	{
		first = qfalse;
		starttime = now;
		return 0.0;
	}

	if ( now < starttime )
	{
		return ( now / 1000.0 ) + ( LONG_MAX - starttime / 1000.0 );
	}

	if ( now - starttime == 0 )
	{
		return 0.0;
	}

	return ( now - starttime ) / 1000.0;
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

typedef struct
{
	HANDLE           threadHandle;
	int              threadId;
	CRITICAL_SECTION crit;
	streamsIO_t      sIO[ MAX_FILE_HANDLES ];
} streamState_t;

streamState_t stream;

/*
===============
Sys_StreamThread

A thread will be sitting in this loop forever
================
*/
void Sys_StreamThread( void )
{
	int buffer;
	int count;
	int readCount;
	int bufferPoint;
	int r, i;

	while ( 1 )
	{
		Sleep( 10 );
		//EnterCriticalSection (&stream.crit);

		for ( i = 1; i < MAX_FILE_HANDLES; i++ )
		{
			// if there is any space left in the buffer, fill it up
			if ( stream.sIO[ i ].active  && !stream.sIO[ i ].eof )
			{
				count = stream.sIO[ i ].bufferSize - ( stream.sIO[ i ].threadPosition - stream.sIO[ i ].streamPosition );

				if ( !count )
				{
					continue;
				}

				bufferPoint = stream.sIO[ i ].threadPosition % stream.sIO[ i ].bufferSize;
				buffer = stream.sIO[ i ].bufferSize - bufferPoint;
				readCount = buffer < count ? buffer : count;

				r = FS_Read( stream.sIO[ i ].buffer + bufferPoint, readCount, stream.sIO[ i ].file );
				stream.sIO[ i ].threadPosition += r;

				if ( r != readCount )
				{
					stream.sIO[ i ].eof = qtrue;
				}
			}
		}

		//LeaveCriticalSection (&stream.crit);
	}
}

/*
===============
Sys_InitStreamThread

================
*/
void Sys_InitStreamThread( void )
{
	int i;

	InitializeCriticalSection( &stream.crit );

	// don't leave the critical section until there is a
	// valid file to stream, which will cause the StreamThread
	// to sleep without any overhead
//	EnterCriticalSection( &stream.crit );

	stream.threadHandle = CreateThread(
	                        NULL, // LPSECURITY_ATTRIBUTES lpsa,
	                        0, // DWORD cbStack,
	                        ( LPTHREAD_START_ROUTINE ) Sys_StreamThread, // LPTHREAD_START_ROUTINE lpStartAddr,
	                        0, // LPVOID lpvThreadParm,
	                        0, //   DWORD fdwCreate,
	                        &stream.threadId );

	for ( i = 0; i < MAX_FILE_HANDLES; i++ )
	{
		stream.sIO[ i ].active = qfalse;
	}
}

/*
==================
WinMain

==================
*/

#if 0
WinVars_t   g_wv;
static char sys_cmdline[ MAX_STRING_CHARS ];
int         totalMsec, countMsec;

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	char cwd[ MAX_OSPATH ];
	int  startTime, endTime;

	// should never get a previous instance in Win32
	if ( hPrevInstance )
	{
		return 0;
	}

#ifdef EXCEPTION_HANDLER
	WinSetExceptionVersion( Q3_VERSION );
#endif

	g_wv.hInstance = hInstance;
	Q_strncpyz( sys_cmdline, lpCmdLine, sizeof( sys_cmdline ) );

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole();

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

	// get the initial time base
	Sys_Milliseconds();

	//Sys_InitStreamThread();

	Com_Init( sys_cmdline );
	NET_Init();

#ifndef DEDICATED
	IN_Init(); // fretn - directinput must be inited after video etc
#endif

	_getcwd( cwd, sizeof( cwd ) );
	Com_Printf( "Working directory: %s\n", cwd );

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if ( !com_dedicated->integer && !com_viewlog->integer )
	{
		Sys_ShowConsole( 0, qfalse );
	}

	SetFocus( g_wv.hWnd );

	// main game loop
	while ( 1 )
	{
		// if not running as a game client, sleep a bit
		if ( g_wv.isMinimized || ( com_dedicated && com_dedicated->integer ) )
		{
			Sleep( 5 );
		}

		// set low precision every frame, because some system calls
		// reset it arbitrarily
//		_controlfp( _PC_24, _MCW_PC );
//    _controlfp( -1, _MCW_EM  ); // no exceptions, even if some crappy
		// syscall turns them back on!

		startTime = Sys_Milliseconds();

		// make sure mouse and joystick are only called once a frame
		IN_Frame();

//		Com_FrameExt();
		Com_Frame();

		endTime = Sys_Milliseconds();
		totalMsec += endTime - startTime;
		countMsec++;
	}

	// never gets here
}

#endif

/*
==============
Sys_IsNumLockDown
==============
*/
qboolean Sys_IsNumLockDown( void )
{
	SHORT state = GetKeyState( VK_NUMLOCK );

	if ( state & 0x01 )
	{
		return qtrue;
	}

	return qfalse;
}
