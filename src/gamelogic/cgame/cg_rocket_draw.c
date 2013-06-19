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
		trap_Rocket_SetInnerRML( "", "", va( "%d", value ) );
	}
	else if ( valueMarked > 0 )
	{
		trap_Rocket_SetInnerRML( "", "", va( "%d+%d", value, valueMarked ) );
	}
	else
	{
		trap_Rocket_SetInnerRML( "", "", va( "%d", value ) );
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
				trap_Rocket_SetInnerRML( "", "", va( "%d", value ) );
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

	s = va( "%d", fps );
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

	//FIXME: this still ignores the width/height of the rect, but at least it's
	//neater than cg_crosshairX/cg_crosshairY
	x = ( cgs.glconfig.vidWidth / 2 ) - ( w / 2 );
	y = ( cgs.glconfig.vidHeight / 2 ) - ( h / 2 );

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
		trap_R_SetColor( color );
		trap_R_DrawStretchPic( x, y, w, h, 0, 0, 1, 1, hShader );
		trap_R_SetColor( NULL );
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

	if ( !cg_drawSpeed.integer )
	{
		return;
	}

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

	trap_Rocket_SetInnerRML( "", "", va( "%d", value ) );
}

static void CG_Rocket_DrawAlienEvosValue( void )
{
	playerState_t *ps = &cg.snap->ps;
	float value = ps->persistant[ PERS_CREDIT ];;

	value /= ( float ) ALIEN_CREDITS_PER_KILL;

	trap_Rocket_SetInnerRML( "", "", va( "%0.1f", floor( value * 10 ) / 10 ) );
}

static void CG_Rocket_DrawStaminaValue( void )
{
	playerState_t *ps = &cg.snap->ps;
	float         stamina = ps->stats[ STAT_STAMINA ];
	int           percent = 100 * ( stamina + ( float ) STAMINA_MAX ) / ( 2 * ( float ) STAMINA_MAX );

	trap_Rocket_SetInnerRML( "", "", va( "%d", percent ) );
}

static void CG_Rocket_DrawWeaponIcon( void )
{
	centity_t     *cent;
	playerState_t *ps;
	weapon_t      weapon;
	const char    *rmlClass = NULL;

	cent = &cg_entities[ cg.snap->ps.clientNum ];
	ps = &cg.snap->ps;
	weapon = BG_GetPlayerWeapon( ps );


	// don't display if dead
	if ( cg.predictedPlayerState.stats[ STAT_HEALTH ] <= 0 )
	{
		return;
	}

	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS )
	{
		CG_Error( "CG_DrawWeaponIcon: weapon out of range: %d", weapon );
	}

	if ( !cg_weapons[ weapon ].registered )
	{
		Com_Printf( S_WARNING "CG_DrawWeaponIcon: weapon %d (%s) "
		"is not registered\n", weapon, BG_Weapon( weapon )->name );
		return;
	}

	if ( ps->clips == 0 && ps->ammo == 0 && !BG_Weapon( weapon )->infiniteAmmo )
	{
		rmlClass = "no_ammo";
	}

	trap_Rocket_SetInnerRML( "", "", va( "<img class='weapon_icon%s%s' src='/%s' />", rmlClass ? " " : "", rmlClass, CG_GetShaderNameFromHandle( cg_weapons[ weapon ].weaponIcon ) ) );
}

static void CG_Rocket_DrawPlayerWallclimbing( void )
{
	const char *wallwalking = NULL;
	if ( cg.snap->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING )
	{
		wallwalking = "wallwalking";
	}

	trap_Rocket_SetInnerRML( "", "", va( "<img class='wallclimb_indictator %s%s' src='%s' />", wallwalking ? " " : "", wallwalking, CG_Rocket_GetAttribute( "", "", "src" ) ) );
}

static void CG_Rocket_DrawAlienSense( void )
{
	float x, y, w, h;
	rectDef_t rect;

	if ( !BG_ClassHasAbility( cg.snap->ps.stats[ STAT_CLASS ], SCA_ALIENSENSE ) )
	{
		return;
	}

	// grab info from libRocket
	trap_Rocket_GetElementAbsoluteOffset( &x, &y );
	trap_Rocket_GetProperty( "width", &w, sizeof( w ), ROCKET_FLOAT );
	trap_Rocket_GetProperty( "height", &h, sizeof( h ), ROCKET_FLOAT );

	// Convert from absolute monitor coords to a virtual 640x480 coordinate system
	x = ( x / cgs.glconfig.vidWidth ) * 640;
	y = ( y / cgs.glconfig.vidHeight ) * 480;
	w = ( w / cgs.glconfig.vidWidth ) * 640;
	h = ( h / cgs.glconfig.vidHeight ) * 480;

	rect.x = x, rect.y = y, rect.w = w, rect.h = h;

	CG_AlienSense( &rect );
}

static void CG_Rocket_DrawHumanScanner( void )
{
	float x, y, w, h;
	rectDef_t rect;

	if ( !BG_InventoryContainsUpgrade( UP_HELMET, cg.snap->ps.stats ) )
	{
		trap_Rocket_SetClass( "active", qfalse );
		return;
	}

	// grab info from libRocket
	trap_Rocket_GetElementAbsoluteOffset( &x, &y );
	trap_Rocket_GetProperty( "width", &w, sizeof( w ), ROCKET_FLOAT );
	trap_Rocket_GetProperty( "height", &h, sizeof( h ), ROCKET_FLOAT );

	// Convert from absolute monitor coords to a virtual 640x480 coordinate system
	x = ( x / cgs.glconfig.vidWidth ) * 640;
	y = ( y / cgs.glconfig.vidHeight ) * 480;
	w = ( w / cgs.glconfig.vidWidth ) * 640;
	h = ( h / cgs.glconfig.vidHeight ) * 480;

	rect.x = x, rect.y = y, rect.w = w, rect.h = h;

	trap_Rocket_SetClass( "active", qtrue );
	CG_Scanner( &rect );
}

static void CG_Rocket_DrawUsableBuildable( void )
{
	vec3_t        view, point;
	trace_t       trace;
	entityState_t *es;

	AngleVectors( cg.refdefViewAngles, view, NULL, NULL );
	VectorMA( cg.refdef.vieworg, 64, view, point );
	CG_Trace( &trace, cg.refdef.vieworg, NULL, NULL,
		  point, cg.predictedPlayerState.clientNum, MASK_SHOT );

	es = &cg_entities[ trace.entityNum ].currentState;

	if ( es->eType == ET_BUILDABLE && BG_Buildable( es->modelindex )->usable &&
		cg.predictedPlayerState.stats[ STAT_TEAM ] == BG_Buildable( es->modelindex )->team )
	{
		//hack to prevent showing the usable buildable when you aren't carrying an energy weapon
		if ( ( es->modelindex == BA_H_REACTOR || es->modelindex == BA_H_REPEATER ) &&
			( !BG_Weapon( cg.snap->ps.weapon )->usesEnergy ||
			BG_Weapon( cg.snap->ps.weapon )->infiniteAmmo ) )
		{
			cg.nearUsableBuildable = BA_NONE;
			return;
		}
		trap_Rocket_SetInnerRML( "", "", va( "<img class='usable_buildable' src='%s' />", CG_Rocket_GetAttribute( "", "", "src" ) ) );
		cg.nearUsableBuildable = es->modelindex;
	}
	else
	{
		// Clear the old image if there was one.
		trap_Rocket_SetInnerRML( "", "", "" );
		cg.nearUsableBuildable = BA_NONE;
	}
}

static void CG_Rocket_DrawLocation( void )
{
	const char *location;
	centity_t  *locent;

	if ( cg.intermissionStarted )
	{
		return;
	}

	locent = CG_GetPlayerLocation();

	if ( locent )
	{
		location = CG_ConfigString( CS_LOCATIONS + locent->currentState.generic1 );
	}
	else
	{
		location = CG_ConfigString( CS_LOCATIONS );
	}

	trap_Rocket_SetInnerRML( "", "", va( "%s", CG_Rocket_QuakeToRML( location ) ) );
}

static void CG_Rocket_DrawTimer( void )
{
	int   mins, seconds, tens;
	int   msec;

	if ( !cg_drawTimer.integer )
	{
		return;
	}

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	trap_Rocket_SetInnerRML( "", "", va( "%d:%d%d", mins, tens, seconds ) );
}

#define LAG_SAMPLES 128

typedef struct
{
	int frameSamples[ LAG_SAMPLES ];
	int frameCount;
	int snapshotFlags[ LAG_SAMPLES ];
	int snapshotSamples[ LAG_SAMPLES ];
	int snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void )
{
	int offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1 ) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
#define PING_FRAMES 40
void CG_AddLagometerSnapshotInfo( snapshot_t *snap )
{
	static int previousPings[ PING_FRAMES ];
	static int index;
	int        i;

	// dropped packet
	if ( !snap )
	{
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->snapFlags;
	lagometer.snapshotCount++;

	cg.ping = 0;

	if ( cg.snap )
	{
		previousPings[ index++ ] = cg.snap->ping;
		index = index % PING_FRAMES;

		for ( i = 0; i < PING_FRAMES; i++ )
		{
			cg.ping += previousPings[ i ];
		}

		cg.ping /= PING_FRAMES;
	}
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_Rocket_DrawDisconnect( void )
{
	float      x, y;
	int        cmdNum;
	usercmd_t  cmd;
	const char *s;
	int        w;
	vec4_t     color = { 1.0f, 1.0f, 1.0f, 1.0f };

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );

	// special check for map_restart
	if ( cmd.serverTime <= cg.snap->ps.commandTime || cmd.serverTime > cg.time )
	{
		return;
	}

	// also add text in center of screen
	s = _("Connection Interrupted");

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 )
	{
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga",
							RSF_DEFAULT));
}

#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

/*
==============
CG_Rocket_DrawLagometer
==============
*/
static void CG_Rocket_DrawLagometer( void )
{
	int    a, i;
	float  v;
	float  ax, ay, aw, ah, mid, range, x, y, w, h;
	int    color;
	vec4_t adjustedColor;
	float  vscale;
	char   *ping;

	if ( cg.snap->ps.pm_type == PM_INTERMISSION )
	{
		return;
	}

	if ( !cg_lagometer.integer )
	{
		return;
	}

	if ( cg.demoPlayback )
	{
		return;
	}

	// grab info from libRocket
	trap_Rocket_GetElementAbsoluteOffset( &x, &y );
	trap_Rocket_GetProperty( "background-color", &adjustedColor, sizeof( adjustedColor ), ROCKET_COLOR );
	trap_Rocket_GetProperty( "width", &w, sizeof( w ), ROCKET_FLOAT );
	trap_Rocket_GetProperty( "height", &h, sizeof( h ), ROCKET_FLOAT );

	// Convert from absolute monitor coords to a virtual 640x480 coordinate system
	x = ( x / cgs.glconfig.vidWidth ) * 640;
	y = ( y / cgs.glconfig.vidHeight ) * 480;
	w = ( w / cgs.glconfig.vidWidth ) * 640;
	h = ( h / cgs.glconfig.vidHeight ) * 480;

	// Color from 0..255 to 0..1
	Vector4Scale( adjustedColor, 1/255.0f, adjustedColor );

	trap_R_SetColor( adjustedColor );
	CG_DrawPic( x, y, w, h, cgs.media.whiteShader );
	trap_R_SetColor( NULL );

	//
	// draw the graph
	//
	ax = x;
	ay = y;
	aw = w;
	ah = h;

	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0; a < aw; a++ )
	{
		i = ( lagometer.frameCount - 1 - a ) & ( LAG_SAMPLES - 1 );
		v = lagometer.frameSamples[ i ];
		v *= vscale;

		if ( v > 0 )
		{
			if ( color != 1 )
			{
				color = 1;
				trap_R_SetColor( g_color_table[ ColorIndex( COLOR_YELLOW ) ] );
			}

			if ( v > range )
			{
				v = range;
			}

			trap_R_DrawStretchPic( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
		else if ( v < 0 )
		{
			if ( color != 2 )
			{
				color = 2;
				trap_R_SetColor( g_color_table[ ColorIndex( COLOR_BLUE ) ] );
			}

			v = -v;

			if ( v > range )
			{
				v = range;
			}

			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0; a < aw; a++ )
	{
		i = ( lagometer.snapshotCount - 1 - a ) & ( LAG_SAMPLES - 1 );
		v = lagometer.snapshotSamples[ i ];

		if ( v > 0 )
		{
			if ( lagometer.snapshotFlags[ i ] & SNAPFLAG_RATE_DELAYED )
			{
				if ( color != 5 )
				{
					color = 5; // YELLOW for rate delay
					trap_R_SetColor( g_color_table[ ColorIndex( COLOR_YELLOW ) ] );
				}
			}
			else
			{
				if ( color != 3 )
				{
					color = 3;

					trap_R_SetColor( g_color_table[ ColorIndex( COLOR_GREEN ) ] );
				}
			}

			v = v * vscale;

			if ( v > range )
			{
				v = range;
			}

			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
		else if ( v < 0 )
		{
			if ( color != 4 )
			{
				color = 4; // RED for dropped snapshots
				trap_R_SetColor( g_color_table[ ColorIndex( COLOR_RED ) ] );
			}

			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer )
	{
		ping = "snc";
	}
	else
	{
		ping = va( "%d", cg.ping );
	}

	trap_Rocket_SetInnerRML( "", "", va( "%s", ping ) );
	CG_Rocket_DrawDisconnect();
}

/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void )
{
	trace_t trace;
	vec3_t  start, end;
	int     content;
	team_t  team;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[ 0 ], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
	          cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY );

	// if the player is in fog, don't show it
	content = trap_CM_PointContents( trace.endpos, 0 );

	if ( content & CONTENTS_FOG )
	{
		return;
	}

	if ( trace.entityNum >= MAX_CLIENTS )
	{
		entityState_t *s = &cg_entities[ trace.entityNum ].currentState;

		if ( s->eType == ET_BUILDABLE && BG_Buildable( s->modelindex )->team ==
		     cg.snap->ps.stats[ STAT_TEAM ] )
		{
			cg.crosshairBuildable = trace.entityNum;
		}
		else
		{
			cg.crosshairBuildable = -1;
		}

		if ( cg_drawEntityInfo.integer && s->eType )
		{
			cg.crosshairClientNum = trace.entityNum;
			cg.crosshairClientTime = cg.time;
		}

		return;
	}

	team = cgs.clientinfo[ trace.entityNum ].team;

	if ( cg.snap->ps.stats[ STAT_TEAM ] != TEAM_NONE )
	{
		//only display team names of those on the same team as this player
		if ( team != cg.snap->ps.stats[ STAT_TEAM ] )
		{
			return;
		}
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}

static void CG_Rocket_DrawCrosshairNames( void )
{
	float *color;
	char  *name;
	float w, x;

	if ( !cg_drawCrosshairNames.integer )
	{
		return;
	}

	if ( cg.renderingThirdPerson )
	{
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, CROSSHAIR_CLIENT_TIMEOUT );

	if ( !color )
	{
		trap_R_SetColor( NULL );
		return;
	}

	if( cg_drawEntityInfo.integer )
	{
		name = va( "(" S_COLOR_CYAN "%s" S_COLOR_WHITE "|" S_COLOR_CYAN "#%d" S_COLOR_WHITE ")",
			   Com_EntityTypeName( cg_entities[cg.crosshairClientNum].currentState.eType ), cg.crosshairClientNum );
	}
	else
	{
		if ( cg_drawCrosshairNames.integer >= 2 )
		{
			name = va( "%2i: %s", cg.crosshairClientNum, cgs.clientinfo[ cg.crosshairClientNum ].name );
		}
		else
		{
			name = cgs.clientinfo[ cg.crosshairClientNum ].name;
		}
	}

	// add health from overlay info to the crosshair client name
	if ( cg_teamOverlayUserinfo.integer &&
		cg.snap->ps.stats[ STAT_TEAM ] != TEAM_NONE &&
		cgs.teamInfoReceived &&
		cgs.clientinfo[ cg.crosshairClientNum ].health > 0 )
	{
		name = va( "%s ^7[^%c%d^7]", name,
			   CG_GetColorCharForHealth( cg.crosshairClientNum ),
			   cgs.clientinfo[ cg.crosshairClientNum ].health );
	}

	trap_Rocket_SetInnerRML( "", "", va( "%s", name ) );
}

static void CG_Rocket_DrawStageReport( void )
{
	char  s[ MAX_TOKEN_CHARS ];

	if ( cg.intermissionStarted )
	{
		return;
	}

	if ( cg.snap->ps.stats[ STAT_TEAM ] == TEAM_NONE )
	{
		return;
	}

	if ( cg.snap->ps.stats[ STAT_TEAM ] == TEAM_ALIENS )
	{
		int kills = ceil( ( float )( cgs.alienNextStageThreshold - cgs.alienCredits ) / ALIEN_CREDITS_PER_KILL );

		if ( kills < 0 )
		{
			kills = 0;
		}

		if ( cgs.alienNextStageThreshold < 0 )
		{
			Com_sprintf( s, MAX_TOKEN_CHARS, _("Stage %d"), cgs.alienStage + 1 );
		}
		else if ( kills == 1 )
		{
			Com_sprintf( s, MAX_TOKEN_CHARS, _("Stage %d, 1 frag for next stage"),
				     cgs.alienStage + 1 );
		}
		else
		{
			Com_sprintf( s, MAX_TOKEN_CHARS, _("Stage %d, %d frags for next stage"),
				     cgs.alienStage + 1, kills );
		}
	}
	else if ( cg.snap->ps.stats[ STAT_TEAM ] == TEAM_HUMANS )
	{
		int credits = cgs.humanNextStageThreshold - cgs.humanCredits;

		if ( credits < 0 )
		{
			credits = 0;
		}

		if ( cgs.humanNextStageThreshold < 0 )
		{
			Com_sprintf( s, MAX_TOKEN_CHARS, _("Stage %d"), cgs.humanStage + 1 );
		}
		else if ( credits == 1 )
		{
			Com_sprintf( s, MAX_TOKEN_CHARS, _("Stage %d, 1 credit for next stage"),
				     cgs.humanStage + 1 );
		}
		else
		{
			Com_sprintf( s, MAX_TOKEN_CHARS, _("Stage %d, %d credits for next stage"),
				     cgs.humanStage + 1, credits );
		}
	}

	trap_Rocket_SetInnerRML( "", "", va( "%s", s ) );

}

static void CG_Rocket_DrawLevelshot( void )
{
	qhandle_t shader;

	if ( rocketInfo.data.mapIndex < 0 && rocketInfo.rocketState != LOADING )
	{
		return;
	}

	if ( !rocketInfo.rocketState != LOADING )
	{

		shader = rocketInfo.data.mapList[ rocketInfo.data.mapIndex ].levelShot;

		if ( shader == -1 )
		{
			shader = rocketInfo.data.mapList[ rocketInfo.data.mapIndex ].levelShot = trap_R_RegisterShader( rocketInfo.data.mapList[ rocketInfo.data.mapIndex ].imageName, RSF_NOMIP );
		}
	}
	else
	{
		shader = trap_R_RegisterShader( va( "levelshots/%s", Info_ValueForKey( CG_ConfigString( CS_SERVERINFO ), "mapname" ) ), RSF_NOMIP );
	}

	trap_Rocket_SetInnerRML( "", "", va( "<img class='levelshot' src='/%s' />", CG_GetShaderNameFromHandle( shader ) ) );
}


#define CENTER_PRINT_DURATION 2000
#define CENTER_PRINT_FADEIN_TIME 300.0f
void CG_Rocket_DrawCenterPrint( void )
{
	float opacity;

	if ( !*cg.centerPrint )
	{
		return;
	}

	if ( cg.centerPrintTime + CENTER_PRINT_DURATION < cg.time )
	{
		*cg.centerPrint = '\0';
		return;
	}

	if ( cg.time == cg.centerPrintTime )
	{
		trap_Rocket_SetInnerRML( "", "", cg.centerPrint );
	}

	// Just showed up. Fade in.
	if ( cg.time - CENTER_PRINT_FADEIN_TIME < cg.centerPrintTime )
	{
		opacity = ( cg.time - cg.centerPrintTime ) / CENTER_PRINT_FADEIN_TIME;
	}
	else if ( cg.time - cg.centerPrintTime < CENTER_PRINT_DURATION - CENTER_PRINT_FADEIN_TIME )
	{
		opacity = 1.0f;
	}
	else
	{
		opacity = ( ( cg.centerPrintTime + CENTER_PRINT_DURATION ) - cg.time ) / CENTER_PRINT_FADEIN_TIME;
	}

	trap_Rocket_SetPropertyById( "", "opacity", va( "%f", opacity ) );
}

typedef struct
{
	const char *name;
	void ( *exec ) ( void );
	rocketElementType_t type;
} elementRenderCmd_t;

static const elementRenderCmd_t elementRenderCmdList[] =
{
	{ "alien_sense", &CG_Rocket_DrawAlienSense, ELEMENT_ALIENS },
	{ "ammo", &CG_Rocket_DrawAmmo, ELEMENT_BOTH },
	{ "center_print", &CG_Rocket_DrawCenterPrint, ELEMENT_GAME },
	{ "clips", &CG_Rocket_DrawClips, ELEMENT_HUMANS },
	{ "credits", &CG_Rocket_DrawCreditsValue, ELEMENT_HUMANS },
	{ "crosshair", &CG_Rocket_DrawCrosshair, ELEMENT_BOTH },
	{ "crosshair_name", &CG_Rocket_DrawCrosshairNames, ELEMENT_GAME },
	{ "evos", &CG_Rocket_DrawAlienEvosValue, ELEMENT_ALIENS },
	{ "fps", &CG_Rocket_DrawFPS, ELEMENT_ALL },
	{ "itemselect", &CG_DrawItemSelect, ELEMENT_BOTH },
	{ "lagometer", &CG_Rocket_DrawLagometer, ELEMENT_GAME },
	{ "levelshot", &CG_Rocket_DrawLevelshot, ELEMENT_ALL },
	{ "location", &CG_Rocket_DrawLocation, ELEMENT_GAME },
	{ "scanner", &CG_Rocket_DrawHumanScanner, ELEMENT_HUMANS },
	{ "speedometer", &CG_Rocket_DrawSpeedGraph, ELEMENT_GAME },
	{ "stage_report", &CG_Rocket_DrawStageReport, ELEMENT_BOTH },
	{ "stamina", &CG_Rocket_DrawStaminaValue, ELEMENT_HUMANS },
	{ "timer", &CG_Rocket_DrawTimer, ELEMENT_GAME },
	{ "usable_buildable", &CG_Rocket_DrawUsableBuildable, ELEMENT_HUMANS },
	{ "wallwalk", &CG_Rocket_DrawPlayerWallclimbing, ELEMENT_ALIENS },
	{ "weapon_icon", &CG_Rocket_DrawWeaponIcon, ELEMENT_BOTH },
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

	if ( cmd && CG_Rocket_IsCommandAllowed( cmd->type ) )
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
