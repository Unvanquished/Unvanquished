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

static void CG_Rocket_DimensionPic( void )
{
	int x, y;

	x = atoi( CG_Rocket_GetAttribute( "", "", "width" ) );
	y = atoi( CG_Rocket_GetAttribute( "", "", "height" ) );

	trap_Rocket_SetElementDimensions( x, y );
}

static void CG_Rocket_DimensionTest( void )
{
	trap_Rocket_SetElementDimensions( 100, 100 );
}

typedef struct
{
	const char *name;
	void ( *exec ) ( void );
} elementDimensionCmd_t;

static const elementDimensionCmd_t elementDimensionCmdList[] =
{
	{ "pic", &CG_Rocket_DimensionPic },
	{ "test", &CG_Rocket_DimensionTest }
};

static const size_t elementDimensionCmdListCount = ARRAY_LEN( elementDimensionCmdList );

static int elementDimensionCmdCmp( const void *a, const void *b )
{
	return Q_stricmp( ( const char * ) a, ( ( elementDimensionCmd_t * ) b )->name );
}

void CG_Rocket_SetElementDimensions( void )
{
	const char *tag = CG_Rocket_GetTag();
	elementDimensionCmd_t *cmd;

	cmd = bsearch( tag, elementDimensionCmdList, elementDimensionCmdListCount, sizeof( elementDimensionCmd_t ), elementDimensionCmdCmp );

	if ( cmd )
	{
		cmd->exec();
	}
	else
	{
		trap_Rocket_SetElementDimensions( -1, -1 );
	}
}
