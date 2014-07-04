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

#ifdef _WIN32

#include "revision.h"
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"
#include "sys_win32.h"

#include <windows.h>
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <psapi.h>

// Used to determine where to store user-specific files
static char homePath[ MAX_OSPATH ] = { 0 };

#ifdef BUILD_CLIENT
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

#ifdef _WIN64
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
	HRESULT (WINAPI *qSHGetFolderPath)(
	  _In_   HWND hwndOwner,
	  _In_   int nFolder,
	  _In_   HANDLE hToken,
	  _In_   DWORD dwFlags,
	  _Out_  LPTSTR pszPath
	);
	HMODULE shfolder = LoadLibrary( "shfolder.dll" );

	if ( !*homePath )
	{
		if ( shfolder == NULL )
		{
			Com_Printf( "Unable to load SHFolder.dll\n" );
			return NULL;
		}

		qSHGetFolderPath = (decltype(qSHGetFolderPath))GetProcAddress( shfolder, "SHGetFolderPathA" );

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
	}

	FreeLibrary( shfolder );
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
const char *Sys_GetCurrentUser( void )
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
			if ( ( cliptext = ( char * )GlobalLock( hClipboardData ) ) != 0 )
			{
				data = ( char * ) Z_Malloc( GlobalSize( hClipboardData ) + 1 );
				Q_strncpyz( data, cliptext, GlobalSize( hClipboardData ) );
				GlobalUnlock( hClipboardData );

				strtok( data, "\n\r\b" );
			}
		}

		CloseClipboard();
	}

	return data;
}

/*
==============
Sys_Basename
==============
*/
char *Sys_Basename( char *path )
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
char *Sys_Dirname( char *path )
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
Sys_FOpen
==============
*/
FILE *Sys_FOpen( const char *ospath, const char *mode )
{
	return fopen( ospath, mode );
}

/*
==============
Sys_Chmod
==============
*/
void Sys_Chmod( const char *ospath, int mode )
{
	// chmod( ospath, (mode_t) mode ); // FIXME - need equivalent to POSIX chmod
}

/*
==============
Sys_FChmod
==============
*/
void Sys_FChmod( FILE *f, int mode )
{
	// fchmod( fileno( f ), (mode_t) mode ); // FIXME - need equivalent to POSIX fchmod
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

#ifdef BUILD_SERVER

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

/*
==============
Sys_PlatformInit

Windows specific initialisation
==============
*/
void Sys_PlatformInit( void )
{
#ifdef BUILD_CLIENT
	TIMECAPS ptc;
#endif

	Sys_SetFloatEnv();

	// Mark the process as DPI-aware on Vista and above
	HMODULE user32 = LoadLibrary("user32.dll");
	if (user32) {
		FARPROC pSetProcessDPIAware = GetProcAddress(user32, "SetProcessDPIAware");
		if (pSetProcessDPIAware)
			pSetProcessDPIAware();
		FreeLibrary(user32);
	}

#ifdef BUILD_CLIENT
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
#ifdef BUILD_CLIENT
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

#else
#error "Don't compile me as part of an awesome operating system. This is meant for Win32 and Win64 only!"
#endif /* _WIN32 */
