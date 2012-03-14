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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <eh.h>

#define MAX_STACK_TRACE_LINES 32

HWND      g_hWnd = NULL;
const char *g_Version = NULL;

typedef int fileHandle_t;

typedef enum
{
  FS_READ,
  FS_WRITE,
  FS_APPEND,
  FS_APPEND_SYNC
} fsMode_t;

extern "C" {
	void         Com_Frame ( void );
	void         Q_strcat ( char *dest, int size, const char *src );
	void __cdecl Com_sprintf ( char *dest, int size, const char *fmt, ... );
	int          FS_FOpenFileByMode ( const char *qpath, fileHandle_t *f, fsMode_t mode );
	int          FS_Write ( const void *buffer, int len, fileHandle_t h );
	void         FS_FCloseFile ( fileHandle_t f );
}

#define CASE( seCode ) case EXCEPTION_ ## seCode: \
  Com_sprintf( minibuffer, sizeof( minibuffer ), "Exception %s (0x%.8x) at address 0x%.8x.", # seCode, EXCEPTION_ ## seCode, m_exPointers->ExceptionRecord->ExceptionAddress ); \
  break;

class CWolfException
{
public:
	CWolfException ( UINT nSeCode, _EXCEPTION_POINTERS *pExcPointers )
	{
		m_seCode = nSeCode;
		m_exPointers = pExcPointers;
	}

	~CWolfException()
	{
	}

	void BuildErrorMessage ( char *buffer, int size )
	{
		char minibuffer[ 256 ];

		switch ( m_seCode )
		{
			case EXCEPTION_ACCESS_VIOLATION:
				Com_sprintf (    minibuffer, sizeof ( minibuffer ),
				                 "Exception ACCESS_VIOLATION (0x%.8x) at address 0x%.8x trying to %s address 0x%.8x.", EXCEPTION_ACCESS_VIOLATION,
				                 m_exPointers->ExceptionRecord->ExceptionAddress,
				                 m_exPointers->ExceptionRecord->ExceptionInformation[ 0 ] ? "write" : "read",
				                 m_exPointers->ExceptionRecord->ExceptionInformation[ 1 ] );
				break;
				CASE ( DATATYPE_MISALIGNMENT );
				CASE ( BREAKPOINT );
				CASE ( SINGLE_STEP );
				CASE ( ARRAY_BOUNDS_EXCEEDED );
				CASE ( FLT_DENORMAL_OPERAND );
				CASE ( FLT_DIVIDE_BY_ZERO );
				CASE ( FLT_INEXACT_RESULT );
				CASE ( FLT_INVALID_OPERATION );
				CASE ( FLT_OVERFLOW );
				CASE ( FLT_STACK_CHECK );
				CASE ( FLT_UNDERFLOW );
				CASE ( INT_DIVIDE_BY_ZERO );
				CASE ( INT_OVERFLOW );
				CASE ( PRIV_INSTRUCTION );
				CASE ( IN_PAGE_ERROR );
				CASE ( ILLEGAL_INSTRUCTION );
				CASE ( NONCONTINUABLE_EXCEPTION );
				CASE ( STACK_OVERFLOW );
				CASE ( INVALID_DISPOSITION );
				CASE ( GUARD_PAGE );
				CASE ( INVALID_HANDLE );

			default:
				Com_sprintf ( minibuffer, sizeof ( minibuffer ), "Unknown exception." );
				break;
		}

		Q_strcat ( buffer, size, minibuffer );
	}

	void BuildDump ( char *buffer, int size )
	{
		char minibuffer[ 256 ];

		Com_sprintf (    minibuffer, sizeof ( minibuffer ),
		                 "Exception      : %.8x\r\n"
		                 "Address        : %.8x\r\n"
		                 "Access Type    : %s\r\n"
		                 "Access Address : %.8x\r\n",

		                 m_exPointers->ExceptionRecord->ExceptionCode,
		                 m_exPointers->ExceptionRecord->ExceptionAddress,
                 m_exPointers->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ? m_exPointers->ExceptionRecord->ExceptionInformation[ 0 ] ? "write" : "read" : "NA",
		                 m_exPointers->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ? m_exPointers->ExceptionRecord->ExceptionInformation[ 1 ] : 0 );

		Q_strcat ( buffer, size, minibuffer );
	}

	void BuildRegisters ( char *buffer, int size )
	{
		char minibuffer[ 256 ];

		Com_sprintf ( minibuffer, sizeof ( minibuffer ),
		              "Registers      : EAX=%.8x CS=%.4x EIP=%.8x EFLGS=%.8x\r\n"
		              "               : EBX=%.8x SS=%.4x ESP=%.8x EBP=%.8x\r\n"
		              "               : ECX=%.8x DS=%.4x ESI=%.8x FS=%.4x\r\n"
		              "               : EDX=%.8x ES=%.4x EDI=%.8x GS=%.4x\r\n",
		              m_exPointers->ContextRecord->Eax,
		              m_exPointers->ContextRecord->SegCs,
		              m_exPointers->ContextRecord->Eip,
		              m_exPointers->ContextRecord->EFlags,
		              m_exPointers->ContextRecord->Ebx,
		              m_exPointers->ContextRecord->SegSs,
		              m_exPointers->ContextRecord->Esp,
		              m_exPointers->ContextRecord->Ebp,
		              m_exPointers->ContextRecord->Ecx,
		              m_exPointers->ContextRecord->SegDs,
		              m_exPointers->ContextRecord->Esi,
		              m_exPointers->ContextRecord->SegFs,
		              m_exPointers->ContextRecord->Edx,
		              m_exPointers->ContextRecord->SegEs,
		              m_exPointers->ContextRecord->Edi,
		              m_exPointers->ContextRecord->SegGs
		            );

		Q_strcat ( buffer, size, minibuffer );
	}

	void BuildStackTrace ( char *buffer, int size )
	{
		int   i, j;
		DWORD *sp;
		DWORD stackTrace[ MAX_STACK_TRACE_LINES ];

		sp = ( DWORD * ) ( m_exPointers->ContextRecord->Ebp );

		for ( i = 0; i < MAX_STACK_TRACE_LINES; i++ )
		{
			if ( !IsBadReadPtr ( sp, sizeof ( DWORD ) ) && *sp )
			{
				DWORD *np = ( DWORD * ) *sp;
				stackTrace[ i ] = * ( sp + 1 );

				sp = np;
			}
			else
			{
				stackTrace[ i ] = 0;
			}
		}

		for ( i = 0; i < MAX_STACK_TRACE_LINES; i++ )
		{
			if ( i == 0 )
			{
				Q_strcat ( buffer, size, "Stack Trace    : " );
			}
			else
			{
				Q_strcat ( buffer, size, "               : " );
			}

			for ( j = 0; j < 4 && i < MAX_STACK_TRACE_LINES; i++, j++ )
			{
				char minibuffer[ 16 ];
				Com_sprintf ( minibuffer, sizeof ( minibuffer ), "%.8x ", stackTrace[ i ] );

				Q_strcat ( buffer, size, minibuffer );
			}

			Q_strcat ( buffer, size, "\r\n" );
		}

//		MessageBox( g_hWnd, buffer, "Arf!", MB_OK );
	}

private:
	UINT               m_seCode;
	_EXCEPTION_POINTERS *m_exPointers;
};

void WinExceptionHandler ( UINT nSeCode, _EXCEPTION_POINTERS *pExcPointers )
{
	CWolfException *we = new CWolfException ( nSeCode, pExcPointers );
	throw we;
}

void RunFrame ( void )
{
	try
	{
		// run the game
		Com_Frame();
	}
	catch ( CWolfException *we )
	{
		char         buffer[ 2048 ];
		fileHandle_t handle;

		/*    *buffer = '\0';

		                we->BuildErrorMessage(  buffer, sizeof( buffer ) );

		                MessageBox( g_hWnd, buffer, "Error!", MB_OK | MB_ICONEXCLAMATION );*/

		*buffer = '\0';

		Q_strcat ( buffer, sizeof ( buffer ), g_Version );
		Q_strcat ( buffer, sizeof ( buffer ), "\r\n" );

		we->BuildDump (          buffer, sizeof ( buffer ) );
		we->BuildRegisters (     buffer, sizeof ( buffer ) );
		we->BuildStackTrace (    buffer, sizeof ( buffer ) );

		Q_strcat ( buffer, sizeof ( buffer ), "\r\n" );

		FS_FOpenFileByMode ( "crash.log", &handle, FS_APPEND );

		if ( handle )
		{
			FS_Write ( buffer, strlen ( buffer ), handle );
			FS_FCloseFile ( handle );
		}

		delete we;

		throw;
	}
}

extern "C" {
	void WinSetExceptionWnd ( HWND wnd )
	{
		if ( wnd )
		{
			_set_se_translator ( WinExceptionHandler );
		}
		else
		{
			_set_se_translator ( NULL );
		}

		g_hWnd = wnd;
	}

	void Com_FrameExt ( void )
	{
		RunFrame();
	}

	void WinSetExceptionVersion ( const char *version )
	{
		g_Version = version;
	}
}
