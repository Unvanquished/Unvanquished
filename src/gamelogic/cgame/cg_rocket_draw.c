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

static void CG_Rocket_DrawPic( void )
{
	float x, y;
	trap_Rocket_GetElementAbsoluteOffset( &x, &y );
	trap_Rocket_ClearElementGeometry();
	trap_Rocket_DrawElementPic( 0, 0, atoi( CG_Rocket_GetAttribute( "", "", "width" ) ), atoi( CG_Rocket_GetAttribute( "", "", "height" ) ), 0, 0, 1, 1,  "ui/assets/mainmenu.jpg" );
}

static void CG_Rocket_DrawTest( void )
{
	trap_Rocket_SetInnerRML( "", "", "<span style='font-size: 5em;'><b>This is a test</b></span>" );
}

typedef struct
{
	const char *name;
	void ( *exec ) ( void );
} elementRenderCmd_t;

static const elementRenderCmd_t elementRenderCmdList[] =
{
	{ "pic", &CG_Rocket_DrawPic },
	{ "test", &CG_Rocket_DrawTest }
};

static const size_t elementRenderCmdListCount = ARRAY_LEN( elementRenderCmdList );

static int elementRenderCmdCmp( const void *a, const void *b )
{
	return Q_stricmp( ( const char * ) a, ( ( elementRenderCmd_t * ) b )->name );
}

void CG_Rocket_RenderElement( void )
{
	const char *tag = CG_Rocket_GetTag();
	elementRenderCmd_t *cmd;

	cmd = bsearch( tag, elementRenderCmdList, elementRenderCmdListCount, sizeof( elementRenderCmd_t ), elementRenderCmdCmp );

	if ( cmd )
	{
		cmd->exec();
	}
}

void CG_Rocket_RegisterElements( void )
{
	int i;

	for ( i = 0; i < elementRenderCmdListCount; i++ )
	{
		trap_Rocket_RegisterElement( elementRenderCmdList[ i ].name );
	}
}
