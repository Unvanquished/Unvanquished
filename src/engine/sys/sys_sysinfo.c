/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 2007 HermitWorks Entertainment Corporation
Copyright (C) 2011 Dusan Jocic

OpenWolf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

OpenWolf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "sys_local.h"
#include "../qcommon/qcommon.h"
#ifdef WIN32
#include <windows.h>
#endif

typedef BOOL (WINAPI *IsWow64Process_t)( HANDLE hProcess, PBOOL pIsWow64 );
 
/*
===============
PrintRawHexData
===============
*/
static void PrintRawHexData( const byte *buf, size_t buf_len ) {
	uint i;

	for( i = 0; i < buf_len; i++ )
		Com_Printf( "%02X", (int)buf[i] );
}

/*
===============
PrintCpuInfoFromRegistry
===============
*/
static qboolean PrintCpuInfoFromRegistry( void ) {
	DWORD i, numPrinted;
	HKEY kCpus;

	char name_buf[256];
	DWORD name_buf_len;

	if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Hardware\\Description\\System\\CentralProcessor",
		0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &kCpus ) != ERROR_SUCCESS ) {
			return qfalse;
	}

	numPrinted = 0;
	for( i = 0; name_buf_len = lengthof( name_buf ),
		RegEnumKeyEx( kCpus, i, name_buf, &name_buf_len,
		NULL, NULL, NULL, NULL ) == ERROR_SUCCESS; i++ ) {
			HKEY kCpu;
		
			int value_buf_i[256];
			LPBYTE value_buf = (char*)value_buf_i;
			DWORD value_buf_len;

			if( RegOpenKeyEx( kCpus, name_buf, 0, KEY_QUERY_VALUE, &kCpu ) != ERROR_SUCCESS ) {
				continue;
			}

			Com_Printf( "    Processor %i:\n", (int)i );

			value_buf_len = sizeof( value_buf_i );
			if( RegQueryValueEx( kCpu, "ProcessorNameString", NULL, NULL, value_buf, &value_buf_len ) == ERROR_SUCCESS ) {
				Com_Printf( "        Name: %s\n", value_buf );
			}

			value_buf_len = sizeof( value_buf_i );
			if( RegQueryValueEx( kCpu, "~MHz", NULL, NULL, value_buf, &value_buf_len ) == ERROR_SUCCESS ) {
				Com_Printf( "        Speed: %i MHz\n", (int)*(DWORD*)value_buf_i );
			}

			value_buf_len = sizeof( value_buf_i );
			if( RegQueryValueEx( kCpu, "VendorIdentifier", NULL, NULL, value_buf, &value_buf_len ) == ERROR_SUCCESS ) {
				Com_Printf( "        Vendor: %s\n", value_buf );
			}

			value_buf_len = sizeof( value_buf_i );
			if( RegQueryValueEx( kCpu, "Identifier", NULL, NULL, value_buf, &value_buf_len ) == ERROR_SUCCESS ) {
				Com_Printf( "        Identifier: %s\n", value_buf );
			}

			value_buf_len = sizeof( value_buf_i );
			if( RegQueryValueEx( kCpu, "FeatureSet", NULL, NULL, value_buf, &value_buf_len ) == ERROR_SUCCESS ) {
				Com_Printf( "        Feature Bits: %08X\n", (int)*(DWORD*)value_buf_i );
			}

			RegCloseKey( kCpu );

			numPrinted++;
	}

	RegCloseKey( kCpus );

	return numPrinted > 0;
}

/*
===============
Sys_PrintCpuInfo
===============
*/
void Sys_PrintCpuInfo( void ) {
	SYSTEM_INFO si;

	GetSystemInfo( &si );

	if( si.dwNumberOfProcessors == 1 ) {
		Com_Printf( "Processor:\n" );
	} else {
		Com_Printf( "Processors (%i):\n", (int)si.dwNumberOfProcessors );
	}

	if( PrintCpuInfoFromRegistry() )
		return;

	Com_Printf( "        Architecture: " );

	switch( si.wProcessorArchitecture ) {
		case PROCESSOR_ARCHITECTURE_INTEL:
			Com_Printf( "x86" );
			break;

		case PROCESSOR_ARCHITECTURE_MIPS:
			Com_Printf( "MIPS" );
			break;

		case PROCESSOR_ARCHITECTURE_ALPHA:
			Com_Printf( "ALPHA" );
			break;

		case PROCESSOR_ARCHITECTURE_PPC:
			Com_Printf( "PPC" );
			break;

		case PROCESSOR_ARCHITECTURE_SHX:
			Com_Printf( "SHX" );
			break;

		case PROCESSOR_ARCHITECTURE_ARM:
			Com_Printf( "ARM" );
			break;

		case PROCESSOR_ARCHITECTURE_IA64:
			Com_Printf( "IA64" );
			break;

		case PROCESSOR_ARCHITECTURE_ALPHA64:
			Com_Printf( "ALPHA64" );
			break;

		case PROCESSOR_ARCHITECTURE_MSIL:
			Com_Printf( "MSIL" );
			break;

		case PROCESSOR_ARCHITECTURE_AMD64:
			Com_Printf( "x64" );
			break;

		case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
			Com_Printf( "WoW64" );
			break;

		default:
			Com_Printf( "UNKNOWN: %i", (int)si.wProcessorArchitecture );
			break;
	}

	Com_Printf( "\n" );

	Com_Printf( "        Revision: %04X\n", (int)si.wProcessorRevision );
}

/*
===============
Sys_PrintMemoryInfo
===============
*/
void Sys_PrintMemoryInfo( void ) {
	SYSTEM_INFO si;
	MEMORYSTATUS ms;

	GetSystemInfo( &si );
	GlobalMemoryStatus( &ms );

	Com_Printf( "Memory:\n" );
	Com_Printf( "    Total Physical: %i MB\n", ms.dwTotalPhys / 1024 / 1024 );
	Com_Printf( "    Total Page File: %i MB\n", ms.dwTotalPageFile / 1024 / 1024 );
	Com_Printf( "    Load: %i%%\n", ms.dwMemoryLoad );
	Com_Printf( "    Page Size: %i K\n", si.dwPageSize / 1024 );
}