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
	int      maxAmmo;
	weapon_t weapon;
	qboolean bp = qfalse;

	switch ( weapon = BG_PrimaryWeapon( cg.snap->ps.stats ) )
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
			if ( !Q_stricmp( "total", CG_Rocket_GetAttribute( "", "", "type" ) ) )
			{
				maxAmmo = BG_Weapon( weapon )->maxAmmo;

				if ( BG_Weapon( weapon )->usesEnergy &&
					BG_InventoryContainsUpgrade( UP_BATTPACK, cg.snap->ps.stats ) )
				{
					maxAmmo *= BATTPACK_MODIFIER;
				}

				value = cg.snap->ps.ammo + ( cg.snap->ps.clips * maxAmmo );
			}
			else
			{
				value = cg.snap->ps.ammo;
			}
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

static void CG_Rocket_DrawClips( void )
{
	int           value;
	playerState_t *ps = &cg.snap->ps;

	switch ( BG_PrimaryWeapon( ps->stats ) )
	{
		case WP_NONE:
		case WP_BLASTER:
		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
			return;

		default:
			value = ps->clips;

			if ( value > -1 )
			{
				trap_Rocket_SetInnerRML( "", "", va( "<span class='clips_value'>%d</span>", value ) );
			}

			break;
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
// 	w *= cgDC.aspectScale;

	trap_Rocket_GetElementAbsoluteOffset( &x, &y );

	//FIXME: this still ignores the width/height of the rect, but at least it's
	//neater than cg_crosshairX/cg_crosshairY
	x = ( cgs.glconfig.vidWidth / 2 ) - ( w / 2 ) - x;
	y = ( cgs.glconfig.vidHeight / 2 ) - ( h / 2 ) - y;

	hShader = wi->crossHair;

	trap_Rocket_GetProperty( "color", &color, sizeof( color ), ROCKET_COLOR );

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

#define SPEEDOMETER_NUM_SAMPLES 4096
#define SPEEDOMETER_NUM_DISPLAYED_SAMPLES 160
#define SPEEDOMETER_DRAW_TEXT   0x1
#define SPEEDOMETER_DRAW_GRAPH  0x2
#define SPEEDOMETER_IGNORE_Z    0x4
float speedSamples[ SPEEDOMETER_NUM_SAMPLES ];
int speedSampleTimes[ SPEEDOMETER_NUM_SAMPLES ];
// array indices
int   oldestSpeedSample = 0;
int   maxSpeedSample = 0;
int   maxSpeedSampleInWindow = 0;

/*
===================
CG_AddSpeed

append a speed to the sample history
===================
*/
void CG_AddSpeed( void )
{
	float  speed;
	vec3_t vel;
	int    windowTime;
	qboolean newSpeedGteMaxSpeed, newSpeedGteMaxSpeedInWindow;

	VectorCopy( cg.snap->ps.velocity, vel );

	if ( cg_drawSpeed.integer & SPEEDOMETER_IGNORE_Z )
	{
		vel[ 2 ] = 0;
	}

	speed = VectorLength( vel );

	windowTime = cg_maxSpeedTimeWindow.integer;
	if ( windowTime < 0 )
	{
		windowTime = 0;
	}
	else if ( windowTime > SPEEDOMETER_NUM_SAMPLES * 1000 )
	{
		windowTime = SPEEDOMETER_NUM_SAMPLES * 1000;
	}

	if ( ( newSpeedGteMaxSpeed = ( speed >= speedSamples[ maxSpeedSample ] ) ) )
	{
		maxSpeedSample = oldestSpeedSample;
	}

	if ( ( newSpeedGteMaxSpeedInWindow = ( speed >= speedSamples[ maxSpeedSampleInWindow ] ) ) )
	{
		maxSpeedSampleInWindow = oldestSpeedSample;
	}

	speedSamples[ oldestSpeedSample ] = speed;

	speedSampleTimes[ oldestSpeedSample ] = cg.time;

	if ( !newSpeedGteMaxSpeed && maxSpeedSample == oldestSpeedSample )
	{
		// if old max was overwritten find a new one
		int i;

		for ( maxSpeedSample = 0, i = 1; i < SPEEDOMETER_NUM_SAMPLES; i++ )
		{
			if ( speedSamples[ i ] > speedSamples[ maxSpeedSample ] )
			{
				maxSpeedSample = i;
			}
		}
	}

	if ( !newSpeedGteMaxSpeedInWindow && ( maxSpeedSampleInWindow == oldestSpeedSample ||
	     cg.time - speedSampleTimes[ maxSpeedSampleInWindow ] > windowTime ) )
	{
		int i;
		do
		{
			maxSpeedSampleInWindow = ( maxSpeedSampleInWindow + 1 ) % SPEEDOMETER_NUM_SAMPLES;
		} while( cg.time - speedSampleTimes[ maxSpeedSampleInWindow ] > windowTime );
		for ( i = maxSpeedSampleInWindow; ; i = ( i + 1 ) % SPEEDOMETER_NUM_SAMPLES )
		{
			if ( speedSamples[ i ] > speedSamples[ maxSpeedSampleInWindow ] )
			{
				maxSpeedSampleInWindow = i;
			}
			if ( i == oldestSpeedSample )
			{
				break;
			}
		}
	}

	oldestSpeedSample = ( oldestSpeedSample + 1 ) % SPEEDOMETER_NUM_SAMPLES;
}

#define SPEEDOMETER_MIN_RANGE 900
#define SPEED_MED             1000.f
#define SPEED_FAST            1600.f
static void CG_Rocket_DrawSpeedGraph( void )
{
	int          i;
	float        val, max, top, x, y, w, h;
	// colour of graph is interpolated between these values
	const vec3_t slow = { 0.0, 0.0, 1.0 };
	const vec3_t medium = { 0.0, 1.0, 0.0 };
	const vec3_t fast = { 1.0, 0.0, 0.0 };
	vec4_t       color, backColor;

	// grab info from libRocket
	trap_Rocket_GetElementAbsoluteOffset( &x, &y );
	trap_Rocket_GetProperty( "color", &color, sizeof( color ), ROCKET_COLOR );
	trap_Rocket_GetProperty( "background-color", &backColor, sizeof( backColor ), ROCKET_COLOR );
	trap_Rocket_GetProperty( "width", &w, sizeof( w ), ROCKET_FLOAT );
	trap_Rocket_GetProperty( "height", &h, sizeof( h ), ROCKET_FLOAT );

	// Convert from absolute monitor coords to a virtual 640x480 coordinate system
	x = ( x / cgs.glconfig.vidWidth ) * 640;
	y = ( y / cgs.glconfig.vidHeight ) * 480;
	w = ( w / cgs.glconfig.vidWidth ) * 640;
	h = ( h / cgs.glconfig.vidHeight ) * 480;

	// Convert from byte scale to [0,1]
	Vector4Scale( color, 1 / 255.0f, color );
	Vector4Scale( backColor, 1 / 255.0f, backColor );

	max = speedSamples[ maxSpeedSample ];

	if ( max < SPEEDOMETER_MIN_RANGE )
	{
		max = SPEEDOMETER_MIN_RANGE;
	}

	trap_R_SetColor( backColor );
	CG_DrawPic( x, y, w, h, cgs.media.whiteShader );

	for ( i = 1; i < SPEEDOMETER_NUM_DISPLAYED_SAMPLES; i++ )
	{
		val = speedSamples[ ( oldestSpeedSample + i + SPEEDOMETER_NUM_SAMPLES -
		SPEEDOMETER_NUM_DISPLAYED_SAMPLES ) % SPEEDOMETER_NUM_SAMPLES ];

		if ( val < SPEED_MED )
		{
			VectorLerpTrem( val / SPEED_MED, slow, medium, color );
		}
		else if ( val < SPEED_FAST )
		{
			VectorLerpTrem( ( val - SPEED_MED ) / ( SPEED_FAST - SPEED_MED ),
					medium, fast, color );
		}
		else
		{
			VectorCopy( fast, color );
		}

		trap_R_SetColor( color );
		top = y + ( 1 - val / max ) * h;
		CG_DrawPic( x + ( i / ( float ) SPEEDOMETER_NUM_DISPLAYED_SAMPLES ) * w, top,
			    w / ( float ) SPEEDOMETER_NUM_DISPLAYED_SAMPLES, val * h / max,
			    cgs.media.whiteShader );
	}

	trap_R_SetColor( NULL );

	// Add text to be configured via CSS
	if ( cg.predictedPlayerState.clientNum == cg.clientNum )
	{
		vec3_t vel;
		VectorCopy( cg.predictedPlayerState.velocity, vel );

		if ( cg_drawSpeed.integer & SPEEDOMETER_IGNORE_Z )
		{
			vel[ 2 ] = 0;
		}

		val = VectorLength( vel );
	}
	else
	{
		val = speedSamples[ ( oldestSpeedSample - 1 + SPEEDOMETER_NUM_SAMPLES ) % SPEEDOMETER_NUM_SAMPLES ];
	}

	trap_Rocket_SetInnerRML( "", "", va( "<span class='speed_max'>%d</span><span class='speed_current'>%d</span>", ( int ) speedSamples[ maxSpeedSampleInWindow ], ( int ) val ) );
}

static void CG_Rocket_DrawCreditsValue( void )
{
	playerState_t *ps = &cg.snap->ps;
	int value = ps->persistant[ PERS_CREDIT ];;

	trap_Rocket_SetInnerRML( "", "", va( "<span class='credits_value'>%d</span>", value ) );
}

static void CG_Rocket_DrawAlienEvosValue( void )
{
	playerState_t *ps = &cg.snap->ps;
	float value = ps->persistant[ PERS_CREDIT ];;

	value /= ( float ) ALIEN_CREDITS_PER_KILL;

	trap_Rocket_SetInnerRML( "", "", va( "<span class='evos_value'>%0.1f</span>", floor( value * 10 ) / 10 ) );
}


typedef struct
{
	const char *name;
	void ( *exec ) ( void );
} elementRenderCmd_t;

static const elementRenderCmd_t elementRenderCmdList[] =
{
	{ "ammo", &CG_Rocket_DrawAmmo },
	{ "clips", &CG_Rocket_DrawClips },
	{ "credits", &CG_Rocket_DrawCreditsValue },
	{ "crosshair", &CG_Rocket_DrawCrosshair },
	{ "evos", &CG_Rocket_DrawAlienEvosValue },
	{ "fps", &CG_Rocket_DrawFPS },
	{ "pic", &CG_Rocket_DrawPic },
	{ "speedometer", &CG_Rocket_DrawSpeedGraph },
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
