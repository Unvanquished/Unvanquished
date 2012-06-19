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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

// Require a minimum version of SDL
#define MINSDL_MAJOR 1
#define MINSDL_MINOR 2
#define MINSDL_PATCH 10

// Input subsystem
void         IN_Init( void );
void         IN_Frame( void );
void         IN_Shutdown( void );
void         IN_Restart( void );

// Console
void         CON_Shutdown( void );
void         CON_Init( void );
char         *CON_Input( void );
void         CON_Print( const char *message );
const char    *Con_GetText( int console );

unsigned int CON_LogSize( void );
unsigned int CON_LogWrite( const char *in );
unsigned int CON_LogRead( char *out, unsigned int outSize );

#ifdef MACOS_X
char         *Sys_StripAppBundle( char *pwd );

#endif

void     Sys_GLimpSafeInit( void );
void     Sys_GLimpInit( void );
void     Sys_PlatformInit( void );
void     Sys_SigHandler( int signal ) NORETURN;
void     Sys_ErrorDialog( const char *error );
void     Sys_AnsiColorPrint( const char *msg );

void     Sys_InitStreamThread( void );

void     Sys_PrintCpuInfo( void );
void     Sys_PrintMemoryInfo( void );

int      Sys_PID( void );
qboolean Sys_PIDIsRunning( int pid );
