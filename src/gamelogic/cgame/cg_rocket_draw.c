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
	vec4_t color = { 255, 255, 255, 255 };
	trap_Rocket_GetElementAbsoluteOffset( &x, &y );
	trap_Rocket_ClearElementGeometry();
	trap_Rocket_DrawElementPic( 0, 0, atoi( CG_Rocket_GetAttribute( "", "", "width" ) ), atoi( CG_Rocket_GetAttribute( "", "", "height" ) ), 0, 0, 1, 1, color, cgs.media.creepShader );
}

static void CG_Rocket_DrawTest( void )
{
	trap_Rocket_SetInnerRML( "", "", "<span style='font-size: 5em;'><b>This is a test</b></span>" );
}

static void CG_Rocket_DrawAmmo( void )
{
	int      value;
	int      valueMarked = -1;
	qboolean bp = qfalse;

	switch ( BG_PrimaryWeapon( cg.snap->ps.stats ) )
	{
		case WP_NONE:
		case WP_BLASTER:
			return;

		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
			value = cg.snap->ps.persistant[ PERS_BP ];
			valueMarked = cg.snap->ps.persistant[ PERS_MARKEDBP ];
			bp = qtrue;
			break;

		default:
			value = cg.snap->ps.ammo;
			break;
	}

	if ( value > 999 )
	{
		value = 999;
	}

	if ( valueMarked > 999 )
	{
		valueMarked = 999;
	}

	if ( !bp )
	{
		trap_Rocket_SetInnerRML( "", "", va( "<span class='ammo_value'>%d</span>", value ) );
	}
	else if ( valueMarked > 0 )
	{
		trap_Rocket_SetInnerRML( "", "", va( "<span class='bp_value'>%d</span>+<span class='markedbp_value'>%d</span>", value, valueMarked ) );
	}
	else
	{
		trap_Rocket_SetInnerRML( "", "", va( "<span class='bp_value'>%d</span>", value ) );
	}
}

#define FPS_FRAMES 20
#define FPS_STRING "fps"
static void CG_Rocket_DrawFPS( void )
{
	char       *s;
	static int previousTimes[ FPS_FRAMES ];
	static int index;
	int        i, total;
	int        fps;
	static int previous;
	int        t, frameTime;
	float      maxX;

	if ( !cg_drawFPS.integer )
	{
		return;
	}

	// don't use serverTime, because that will be drifting to
	// correct for Internet lag changes, timescales, timedemos, etc.
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[ index % FPS_FRAMES ] = frameTime;
	index++;

	if ( index > FPS_FRAMES )
	{
		// average multiple frames together to smooth changes out a bit
		total = 0;

		for ( i = 0; i < FPS_FRAMES; i++ )
		{
			total += previousTimes[ i ];
		}

		if ( !total )
		{
			total = 1;
		}

		fps = 1000 * FPS_FRAMES / total;
	}
	else
		fps = 0;

	s = va( "<span class='fps'>%d</span>", fps );
	trap_Rocket_SetInnerRML( "", "", s );
}

static void CG_Rocket_DrawCrosshair( void )
{
	float        w, h;
	qhandle_t    hShader;
	float        x, y;
	weaponInfo_t *wi;
	weapon_t     weapon;
	vec4_t       color = { 255, 255, 255, 255 };
	const char *s;

	trap_Rocket_ClearElementGeometry();

	weapon = BG_GetPlayerWeapon( &cg.snap->ps );

	if ( cg_drawCrosshair.integer == CROSSHAIR_ALWAYSOFF )
	{
		return;
	}

	if ( cg_drawCrosshair.integer == CROSSHAIR_RANGEDONLY &&
		!BG_Weapon( weapon )->longRanged )
	{
		return;
	}

	if ( cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
	{
		return;
	}

	if ( cg.renderingThirdPerson )
	{
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION )
	{
		return;
	}

	wi = &cg_weapons[ weapon ];

	w = h = wi->crossHairSize * cg_crosshairSize.value;
	w *= cgDC.aspectScale;

	trap_Rocket_GetElementAbsoluteOffset( &x, &y );

	//FIXME: this still ignores the width/height of the rect, but at least it's
	//neater than cg_crosshairX/cg_crosshairY
	x = ( cgs.glconfig.vidWidth / 2 ) - ( w / 2 );
	y = ( cgs.glconfig.vidHeight / 2 ) - ( h / 2 );

	hShader = wi->crossHair;

	s = CG_Rocket_GetAttribute( "", "", "color" );

	if ( s && *s )
	{
		sscanf( s, "%f %f %f %f", &color[ 0 ], &color[ 1 ], &color[ 2 ], &color[ 3 ] );
	}

	//aiming at a friendly player/buildable, dim the crosshair
	if ( cg.time == cg.crosshairClientTime || cg.crosshairBuildable >= 0 )
	{
		int i;

		for ( i = 0; i < 3; i++ )
		{
			color[ i ] *= .5f;
		}
	}

	if ( hShader != 0 )
	{
		trap_Rocket_DrawElementPic( x, y, w, h, 0, 0, 1, 1, color, hShader );
	}
}


typedef struct
{
	const char *name;
	void ( *exec ) ( void );
} elementRenderCmd_t;

static const elementRenderCmd_t elementRenderCmdList[] =
{
	{ "ammo", &CG_Rocket_DrawAmmo },
	{ "crosshair", &CG_Rocket_DrawCrosshair },
	{ "fps", &CG_Rocket_DrawFPS },
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
