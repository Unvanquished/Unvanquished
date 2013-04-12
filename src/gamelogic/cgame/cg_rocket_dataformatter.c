/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

#include "cg_local.h"

static void CG_Rocket_DFServerPing( int handle, const char *data )
{
	trap_Rocket_DataFormatterFormattedData( handle, va( "%s ms", Info_ValueForKey( data, "1" ) ) );
}

static void CG_Rocket_DFServerPlayers( int handle, const char *data )
{
	trap_Rocket_DataFormatterFormattedData( handle, va( "%s + (%s)", Info_ValueForKey( data, "1" ), Info_ValueForKey( data, "2" ) ) );
}

typedef struct
{
	const char *name;
	void ( *exec ) ( int handle, const char *data );
} dataFormatterCmd_t;

static const dataFormatterCmd_t dataFormatterCmdList[] =
{
	{ "ServerPing", &CG_Rocket_DFServerPing },
	{ "ServerPlayers", &CG_Rocket_DFServerPlayers }
};

static const size_t dataFormatterCmdListCount = ARRAY_LEN( dataFormatterCmdList );

static int dataFormatterCmdCmp( const void *a, const void *b )
{
	return Q_stricmp( ( const char * ) a, ( ( dataFormatterCmd_t * ) b )->name );
}

void CG_Rocket_FormatData( int handle )
{
	static char name[ 200 ], data[ BIG_INFO_STRING ];
	dataFormatterCmd_t *cmd;

	trap_Rocket_DataFormatterRawData( handle, name, sizeof( name ), data, sizeof( data ) );

	cmd = bsearch( name, dataFormatterCmdList, dataFormatterCmdListCount, sizeof( dataFormatterCmd_t ), dataFormatterCmdCmp );

	if ( cmd )
	{
		cmd->exec( handle, data );
	}
}

void CG_Rocket_RegisterDataFormatters( void )
{
	int i;

	for ( i = 0; i < dataFormatterCmdListCount; i++ )
	{
		trap_Rocket_RegisterDataFormatter( dataFormatterCmdList[ i ].name );
	}
}
