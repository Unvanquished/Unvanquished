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

static void CG_Rocket_DimensionGeneric( void )
{
	float w, h;
	trap_Rocket_GetProperty( "width", &w, sizeof( w ), ROCKET_FLOAT );
	trap_Rocket_GetProperty( "height", &h, sizeof( h ), ROCKET_FLOAT );
	trap_Rocket_SetElementDimensions( w, h );
}

typedef struct
{
	const char *name;
	void ( *exec ) ( void );
	rocketElementType_t type;
} elementDimensionCmd_t;

static const elementDimensionCmd_t elementDimensionCmdList[] =
{
	{ "alien_sense", &CG_Rocket_DimensionGeneric, ELEMENT_ALIENS },
	{ "ammo_stack", &CG_Rocket_DimensionGeneric, ELEMENT_HUMANS },
	{ "clip_stack", &CG_Rocket_DimensionGeneric, ELEMENT_HUMANS },
	{ "lagometer", &CG_Rocket_DimensionGeneric, ELEMENT_GAME },
	{ "minimap", &CG_Rocket_DimensionGeneric, ELEMENT_GAME },
	{ "scanner", &CG_Rocket_DimensionGeneric, ELEMENT_HUMANS },
	{ "speedometer", &CG_Rocket_DimensionGeneric, ELEMENT_GAME },
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

	if ( cmd && CG_Rocket_IsCommandAllowed( cmd->type ) )
	{
		cmd->exec();
	}
	else
	{
		trap_Rocket_SetElementDimensions( -1, -1 );
	}
}
