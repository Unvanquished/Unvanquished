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

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"

#define MAX_LOG 131072

Cvar::Cvar<bool> com_ansiColor("com_ansiColor", "", Cvar::NONE, true);
Cvar::Cvar<std::string> com_consoleCommand("com_consoleCommand", "", Cvar::NONE, "");

static char         consoleLog[ MAX_LOG ];
static unsigned int writePos = 0;
static unsigned int readPos = 0;

/*
==================
CON_LogSize
==================
*/
unsigned int CON_LogSize()
{
	if ( readPos <= writePos )
	{
		return writePos - readPos;
	}
	else
	{
		return writePos + MAX_LOG - readPos;
	}
}

/*
==================
CON_LogFree
==================
*/
static unsigned int CON_LogFree()
{
	return MAX_LOG - CON_LogSize() - 1;
}

/*
==================
CON_LogWrite
==================
*/
unsigned int CON_LogWrite( const char *in )
{
	unsigned int length = strlen( in );
	unsigned int firstChunk;
	unsigned int secondChunk;

	while ( CON_LogFree() < length && CON_LogSize() > 0 )
	{
		// Free enough space
		while ( consoleLog[ readPos ] != '\n' && CON_LogSize() > 1 )
		{
			readPos = ( readPos + 1 ) % MAX_LOG;
		}

		// Skip past the '\n'
		readPos = ( readPos + 1 ) % MAX_LOG;
	}

	if ( CON_LogFree() < length )
	{
		return 0;
	}

	if ( writePos + length > MAX_LOG )
	{
		firstChunk = MAX_LOG - writePos;
		secondChunk = length - firstChunk;
	}
	else
	{
		firstChunk = length;
		secondChunk = 0;
	}

	Com_Memcpy( consoleLog + writePos, in, firstChunk );
	Com_Memcpy( consoleLog, in + firstChunk, secondChunk );

	writePos = ( writePos + length ) % MAX_LOG;

	return length;
}
