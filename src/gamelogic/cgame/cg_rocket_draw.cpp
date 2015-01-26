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

static void CG_GetRocketElementColor( vec4_t color )
{
	trap_Rocket_GetProperty( "color", color, sizeof( vec4_t ), ROCKET_COLOR );
}

static void CG_GetRocketElementBGColor( vec4_t bgColor )
{
	trap_Rocket_GetProperty( "background-color", bgColor, sizeof( vec4_t ), ROCKET_COLOR );
}

static void CG_GetRocketElementRect( rectDef_t *rect )
{
	trap_Rocket_GetElementAbsoluteOffset( &rect->x, &rect->y );
	trap_Rocket_GetProperty( "width", &rect->w, sizeof( rect->w ), ROCKET_FLOAT );
	trap_Rocket_GetProperty( "height", &rect->h, sizeof( rect->h ), ROCKET_FLOAT );

	// Convert from absolute monitor coords to a virtual 640x480 coordinate system
	rect->x = ( rect->x / cgs.glconfig.vidWidth ) * 640;
	rect->y = ( rect->y / cgs.glconfig.vidHeight ) * 480;
	rect->w = ( rect->w / cgs.glconfig.vidWidth ) * 640;
	rect->h = ( rect->h / cgs.glconfig.vidHeight ) * 480;
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
			if ( !Q_stricmp( "total", CG_Rocket_GetAttribute( "type" ) ) )
			{
				maxAmmo = BG_Weapon( weapon )->maxAmmo;
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
		trap_Rocket_SetInnerRML( va( "%d", value ), 0 );
	}

	else if ( valueMarked > 0 )
	{
		trap_Rocket_SetInnerRML( va( "%d+%d", value, valueMarked ), 0 );
	}

	else
	{
		trap_Rocket_SetInnerRML( va( "%d", value ), 0 );
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
			trap_Rocket_SetInnerRML( "", 0 );
			return;

		default:
			value = ps->clips;

			if ( value > -1 )
			{
				trap_Rocket_SetInnerRML( va( "%d", value ), 0 );
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
	trap_Rocket_SetInnerRML( s, 0 );
}

#define CROSSHAIR_INDICATOR_HITFADE 500

static void CG_Rocket_DrawCrosshairIndicator( void )
{
	rectDef_t    rect;
	float        x, y, w, h, dim;
	qhandle_t    indicator;
	vec4_t       color, drawColor, baseColor;
	weapon_t     weapon;
	weaponInfo_t *wi;
	qboolean     onRelevantEntity;

	if ( ( !cg_drawCrosshairHit.integer && !cg_drawCrosshairFriendFoe.integer ) ||
	        cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT ||
	        cg.snap->ps.pm_type == PM_INTERMISSION ||
	        cg.renderingThirdPerson )
	{
		return;
	}

	weapon = BG_GetPlayerWeapon( &cg.snap->ps );
	wi = &cg_weapons[ weapon ];
	indicator = wi->crossHairIndicator;

	if ( !indicator )
	{
		return;
	}

	CG_GetRocketElementColor( color );
	CG_GetRocketElementRect( &rect );

	// set base color (friend/foe detection)
	if ( cg_drawCrosshairFriendFoe.integer >= CROSSHAIR_ALWAYSON ||
	        ( cg_drawCrosshairFriendFoe.integer >= CROSSHAIR_RANGEDONLY && BG_Weapon( weapon )->longRanged ) )
	{
		if ( cg.crosshairFoe )
		{
			Vector4Copy( colorRed, baseColor );
			baseColor[ 3 ] = color[ 3 ] * 0.75f;
			onRelevantEntity = qtrue;
		}

		else if ( cg.crosshairFriend )
		{
			Vector4Copy( colorGreen, baseColor );
			baseColor[ 3 ] = color[ 3 ] * 0.75f;
			onRelevantEntity = qtrue;
		}

		else
		{
			Vector4Set( baseColor, 1.0f, 1.0f, 1.0f, 0.0f );
			onRelevantEntity = qfalse;
		}
	}

	else
	{
		Vector4Set( baseColor, 1.0f, 1.0f, 1.0f, 0.0f );
		onRelevantEntity = qfalse;
	}

	// add hit color
	if ( cg_drawCrosshairHit.integer && cg.hitTime + CROSSHAIR_INDICATOR_HITFADE > cg.time )
	{
		dim = ( ( cg.hitTime + CROSSHAIR_INDICATOR_HITFADE ) - cg.time ) / ( float )CROSSHAIR_INDICATOR_HITFADE;

		Vector4Lerp( dim, baseColor, colorWhite, drawColor );
	}

	else if ( !onRelevantEntity )
	{
		return;
	}

	else
	{
		Vector4Copy( baseColor, drawColor );
	}

	// set size
	w = h = wi->crossHairSize * cg_crosshairSize.value;
	w *= cgs.aspectScale;

	// HACK: This ignores the width/height of the rect (does it?)
	x = rect.x + ( rect.w / 2 ) - ( w / 2 );
	y = rect.y + ( rect.h / 2 ) - ( h / 2 );

	// draw
	trap_R_SetColor( drawColor );
	CG_DrawPic( x, y, w, h, indicator );
	trap_R_SetColor( NULL );
}

static void CG_Rocket_DrawCrosshair( void )
{
	rectDef_t    rect;
	float        w, h;
	qhandle_t    crosshair;
	float        x, y;
	weaponInfo_t *wi;
	weapon_t     weapon;
	vec4_t       color = { 255, 255, 255, 255 };


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

	CG_GetRocketElementRect( &rect );

	wi = &cg_weapons[ weapon ];

	w = h = wi->crossHairSize * cg_crosshairSize.value;
	w *= cgs.aspectScale;

	// HACK: This ignores the width/height of the rect (does it?)
	x = rect.x + ( rect.w / 2 ) - ( w / 2 );
	y = rect.y + ( rect.h / 2 ) - ( h / 2 );

	crosshair = wi->crossHair;

	if ( crosshair )
	{
		CG_GetRocketElementColor( color );
		trap_R_SetColor( color );
		CG_DrawPic( x, y, w, h, crosshair );
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
		}
		while ( cg.time - speedSampleTimes[ maxSpeedSampleInWindow ] > windowTime );

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
	float        val, max, top;
	// colour of graph is interpolated between these values
	const vec3_t slow = { 0.0, 0.0, 1.0 };
	const vec3_t medium = { 0.0, 1.0, 0.0 };
	const vec3_t fast = { 1.0, 0.0, 0.0 };
	vec4_t       color, backColor;
	rectDef_t    rect;

	if ( !cg_drawSpeed.integer )
	{
		trap_Rocket_SetInnerRML( "", 0 );
		return;
	}

	if ( cg_drawSpeed.integer & SPEEDOMETER_DRAW_GRAPH )
	{
		// grab info from libRocket
		CG_GetRocketElementColor( color );
		CG_GetRocketElementBGColor( backColor );
		CG_GetRocketElementRect( &rect );

		max = speedSamples[ maxSpeedSample ];

		if ( max < SPEEDOMETER_MIN_RANGE )
		{
			max = SPEEDOMETER_MIN_RANGE;
		}

		trap_R_SetColor( backColor );
		CG_DrawPic( rect.x, rect.y, rect.w, rect.h, cgs.media.whiteShader );

		for ( i = 1; i < SPEEDOMETER_NUM_DISPLAYED_SAMPLES; i++ )
		{
			val = speedSamples[( oldestSpeedSample + i + SPEEDOMETER_NUM_SAMPLES -
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
			top = rect.y + ( 1 - val / max ) * rect.h;
			CG_DrawPic( rect.x + ( i / ( float ) SPEEDOMETER_NUM_DISPLAYED_SAMPLES ) * rect.w, top,
				rect.w / ( float ) SPEEDOMETER_NUM_DISPLAYED_SAMPLES, val * rect.h / max,
				cgs.media.whiteShader );
		}

		trap_R_SetColor( NULL );
	}

	if ( cg_drawSpeed.integer & SPEEDOMETER_DRAW_TEXT )
	{
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
			val = speedSamples[( oldestSpeedSample - 1 + SPEEDOMETER_NUM_SAMPLES ) % SPEEDOMETER_NUM_SAMPLES ];
		}

		trap_Rocket_SetInnerRML( va( "<span class='speed_max'>%d</span><span class='speed_current'>%d</span>", ( int ) speedSamples[ maxSpeedSampleInWindow ], ( int ) val ), 0 );
	}
}

static void CG_Rocket_DrawCreditsValue( void )
{
	playerState_t *ps = &cg.snap->ps;
	int value = ps->persistant[ PERS_CREDIT ];;

	trap_Rocket_SetInnerRML( va( "%d", value ), 0 );
}

static void CG_Rocket_DrawAlienEvosValue( void )
{
	playerState_t *ps = &cg.snap->ps;
	float value = ps->persistant[ PERS_CREDIT ];;

	value /= ( float ) CREDITS_PER_EVO;

	trap_Rocket_SetInnerRML( va( "%0.1f", floor( value * 10 ) / 10 ), 0 );
}

static void CG_Rocket_DrawStaminaValue( void )
{
	playerState_t *ps = &cg.snap->ps;
	float         stamina = ps->stats[ STAT_STAMINA ];
	int           percent = 100 * ( stamina / ( float ) STAMINA_MAX );

	trap_Rocket_SetInnerRML( va( "%d", percent ), 0 );
}

static void CG_Rocket_DrawWeaponIcon( void )
{
	playerState_t *ps;
	weapon_t      weapon;
	const char    *rmlClass = NULL;

	ps = &cg.snap->ps;
	weapon = BG_GetPlayerWeapon( ps );


	// don't display if dead
	if ( cg.predictedPlayerState.stats[ STAT_HEALTH ] <= 0 || weapon == WP_NONE )
	{
		return;
	}

	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS )
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

	trap_Rocket_SetInnerRML( va( "<img class='weapon_icon%s%s' src='/%s' />", rmlClass ? " " : "", rmlClass, CG_GetShaderNameFromHandle( cg_weapons[ weapon ].weaponIcon ) ), 0 );
}

static void CG_Rocket_DrawPlayerWallclimbing( void )
{
	qboolean wallwalking = cg.snap->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING;
	trap_Rocket_SetClass( "active", wallwalking );
	trap_Rocket_SetClass( "inactive", !wallwalking );
}

static void CG_Rocket_DrawAlienSense( void )
{
	rectDef_t rect;

	if ( !BG_ClassHasAbility( cg.snap->ps.stats[ STAT_CLASS ], SCA_ALIENSENSE ) )
	{
		return;
	}

	// grab info from libRocket
	CG_GetRocketElementRect( &rect );
	CG_AlienSense( &rect );
}

static void CG_Rocket_DrawHumanScanner( void )
{
	rectDef_t rect;

	if ( !BG_InventoryContainsUpgrade( UP_RADAR, cg.snap->ps.stats ) )
	{
		trap_Rocket_SetClass( "active", qfalse );
		return;
	}

	// grab info from libRocket
	CG_GetRocketElementRect( &rect );

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
	          point, cg.predictedPlayerState.clientNum, MASK_SHOT, 0 );

	es = &cg_entities[ trace.entityNum ].currentState;

	if ( es->eType == ET_BUILDABLE && BG_Buildable( es->modelindex )->usable &&
	        cg.predictedPlayerState.persistant[ PERS_TEAM ] == BG_Buildable( es->modelindex )->team )
	{
		//hack to prevent showing the usable buildable when you aren't carrying an energy weapon
		if ( ( es->modelindex == BA_H_REACTOR || es->modelindex == BA_H_REPEATER ) &&
		        ( !BG_Weapon( cg.snap->ps.weapon )->usesEnergy ||
		          BG_Weapon( cg.snap->ps.weapon )->infiniteAmmo ) )
		{
			cg.nearUsableBuildable = BA_NONE;
			return;
		}

		trap_Rocket_SetInnerRML( va( "<img class='usable_buildable' src='%s' />", CG_Rocket_GetAttribute( "src" ) ), 0 );
		cg.nearUsableBuildable = es->modelindex;
	}

	else
	{
		// Clear the old image if there was one.
		trap_Rocket_SetInnerRML( "", 0 );
		cg.nearUsableBuildable = BA_NONE;
	}
}

static void CG_Rocket_DrawLocation( void )
{
	const char *location;
	centity_t  *locent;

	if ( cg.intermissionStarted )
	{
		trap_Rocket_SetInnerRML( "", 0 );
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

	trap_Rocket_SetInnerRML( location, RP_QUAKE );
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

	trap_Rocket_SetInnerRML( va( "%d:%d%d", mins, tens, seconds ), 0 );
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

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );

	// special check for map_restart
	if ( cmd.serverTime <= cg.snap->ps.commandTime || cmd.serverTime > cg.time )
	{
		return;
	}

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 )
	{
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader( "gfx/2d/net",
	            RSF_DEFAULT ) );
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
	float  ax, ay, aw, ah, mid, range;
	int    color;
	vec4_t adjustedColor;
	float  vscale;
	char   *ping;
	rectDef_t rect;

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
	CG_GetRocketElementRect( &rect );
	CG_GetRocketElementBGColor( adjustedColor );

	trap_R_SetColor( adjustedColor );
	CG_DrawPic( rect.x, rect.y, rect.w, rect.h, cgs.media.whiteShader );
	trap_R_SetColor( NULL );

	//
	// draw the graph
	//
	ax = rect.x;
	ay = rect.y;
	aw = rect.w;
	ah = rect.h;

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

	if ( cg_nopredict.integer || cg.pmoveParams.synchronous )
	{
		ping = "snc";
	}

	else
	{
		ping = va( "%d", cg.ping );
	}

	trap_Rocket_SetInnerRML( va( "<span class='ping'>%s</span>", ping ), 0 );
	CG_Rocket_DrawDisconnect();
}

/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void )
{
	trace_t       trace;
	vec3_t        start, end;
	team_t        ownTeam, targetTeam;
	entityState_t *targetState;

	cg.crosshairFriend = qfalse;
	cg.crosshairFoe    = qfalse;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[ 0 ], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
	          cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY, 0 );

	// ignore special entities
	if ( trace.entityNum > ENTITYNUM_MAX_NORMAL )
	{
		return;
	}

	// ignore targets in fog
	if ( trap_CM_PointContents( trace.endpos, 0 ) & CONTENTS_FOG )
	{
		return;
	}

	ownTeam = ( team_t ) cg.snap->ps.persistant[ PERS_TEAM ];
	targetState = &cg_entities[ trace.entityNum ].currentState;

	if ( trace.entityNum >= MAX_CLIENTS )
	{
		// we have a non-client entity

		// set friend/foe if it's a living buildable
		if ( targetState->eType == ET_BUILDABLE && targetState->generic1 > 0 )
		{
			targetTeam = BG_Buildable( targetState->modelindex )->team;

			if ( targetTeam == ownTeam && ownTeam != TEAM_NONE )
			{
				cg.crosshairFriend = qtrue;
			}

			else if ( targetTeam != TEAM_NONE )
			{
				cg.crosshairFoe = qtrue;
			}
		}

		// set more stuff if requested
		if ( cg_drawEntityInfo.integer && targetState->eType )
		{
			cg.crosshairClientNum = trace.entityNum;
			cg.crosshairClientTime = cg.time;
		}
	}

	else
	{
		// we have a client entitiy
		targetTeam = cgs.clientinfo[ trace.entityNum ].team;

		// only react to living clients
		if ( targetState->generic1 > 0 )
		{
			// set friend/foe
			if ( targetTeam == ownTeam && ownTeam != TEAM_NONE )
			{
				cg.crosshairFriend = qtrue;

				// only set this for friendly clients as it triggers name display
				cg.crosshairClientNum = trace.entityNum;
				cg.crosshairClientTime = cg.time;
			}

			else if ( targetTeam != TEAM_NONE )
			{
				cg.crosshairFoe = qtrue;

				if ( ownTeam == TEAM_NONE )
				{
					// spectating, so show the name
					cg.crosshairClientNum = trace.entityNum;
					cg.crosshairClientTime = cg.time;
				}
			}
		}
	}
}

static void CG_Rocket_DrawCrosshairNames( void )
{
	float alpha;
	char  *name;

	trap_Rocket_SetInnerRML( "&nbsp;", 0 );

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
	alpha = CG_FadeAlpha( cg.crosshairClientTime, CROSSHAIR_CLIENT_TIMEOUT );

	if ( cg.crosshairClientTime == cg.time )
	{
		alpha = 1.0f;
	}

	else if ( !alpha )
	{
		return;
	}

	trap_Rocket_SetProperty( "opacity", va( "%f", alpha ) );

	if ( cg_drawEntityInfo.integer )
	{
		name = va( "(" S_COLOR_CYAN "%s" S_COLOR_WHITE "|" S_COLOR_CYAN "#%d" S_COLOR_WHITE ")",
		           Com_EntityTypeName( cg_entities[cg.crosshairClientNum].currentState.eType ), cg.crosshairClientNum );
	}

	else if ( cg_drawCrosshairNames.integer >= 2 )
	{
		name = va( "%2i: %s", cg.crosshairClientNum, cgs.clientinfo[ cg.crosshairClientNum ].name );
	}

	else
	{
		name = cgs.clientinfo[ cg.crosshairClientNum ].name;
	}

	// add health from overlay info to the crosshair client name
	if ( cg_teamOverlayUserinfo.integer &&
	        cg.snap->ps.persistant[ PERS_TEAM ] != TEAM_NONE &&
	        cgs.teamInfoReceived &&
	        cgs.clientinfo[ cg.crosshairClientNum ].health > 0 )
	{
		name = va( "%s ^7[^%c%d^7]", name,
		           CG_GetColorCharForHealth( cg.crosshairClientNum ),
		           cgs.clientinfo[ cg.crosshairClientNum ].health );
	}

	trap_Rocket_SetInnerRML( va( "%s", name ), RP_EMOTICONS );
}

static void CG_Rocket_DrawMomentum( void )
{
	char  s[ MAX_TOKEN_CHARS ];
	float momentum;
	team_t team;

	if ( cg.intermissionStarted )
	{
		return;
	}

	team = ( team_t ) cg.snap->ps.persistant[ PERS_TEAM ];

	if ( team <= TEAM_NONE || team >= NUM_TEAMS )
	{
		return;
	}

	momentum = cg.predictedPlayerState.persistant[ PERS_MOMENTUM ] / 10.0f;

	Com_sprintf( s, MAX_TOKEN_CHARS, _( "%.1f momentum" ), momentum );

	trap_Rocket_SetInnerRML( va( "%s", s ), 0 );

}

static void CG_Rocket_DrawLevelshot( void )
{
	const char *map;

	if ( ( rocketInfo.data.mapIndex < 0 || rocketInfo.data.mapIndex >= rocketInfo.data.mapCount ) )
	{
		return;
	}

	trap_Rocket_SetInnerRML( va( "<img class='levelshot' src='/meta/%s/%s' />", rocketInfo.data.mapList[ rocketInfo.data.mapIndex ].mapLoadName, rocketInfo.data.mapList[ rocketInfo.data.mapIndex ].mapLoadName ), 0 );
}

static void CG_Rocket_DrawMapLoadingLevelshot( void )
{
	if ( rocketInfo.cstate.connState >= CA_LOADING )
	{
		trap_Rocket_SetInnerRML( va( "<img class='levelshot' src='/meta/%s/%s' />", Info_ValueForKey( CG_ConfigString( CS_SERVERINFO ), "mapname" ), Info_ValueForKey( CG_ConfigString( CS_SERVERINFO ), "mapname" ) ), 0 );
	}
}


#define CENTER_PRINT_DURATION 3000
void CG_Rocket_DrawCenterPrint( void )
{
	if ( !*cg.centerPrint )
	{
		return;
	}

	if ( cg.centerPrintTime + CENTER_PRINT_DURATION < cg.time )
	{
		*cg.centerPrint = '\0';
		trap_Rocket_SetInnerRML( "&nbsp;", 0 );
		return;
	}

	if ( cg.time == cg.centerPrintTime )
	{
		trap_Rocket_SetInnerRML( cg.centerPrint, RP_EMOTICONS );
	}

	trap_Rocket_SetProperty( "opacity", va( "%f", CG_FadeAlpha( cg.centerPrintTime, CENTER_PRINT_DURATION ) ) );
}

void CG_Rocket_DrawPlayerHealth( void )
{
	static int lastHealth = 0;

	if ( lastHealth != cg.snap->ps.stats[ STAT_HEALTH ] )
	{
		trap_Rocket_SetInnerRML( va( "%d", cg.snap->ps.stats[ STAT_HEALTH ] ), 0 );
	}
}

void CG_Rocket_DrawPlayerHealthCross( void )
{
	qhandle_t shader;
	vec4_t    color, ref_color;
	float     ref_alpha;
	rectDef_t rect;

	// grab info from libRocket
	CG_GetRocketElementColor( ref_color );
	CG_GetRocketElementRect( &rect );

	// Pick the current icon
	shader = cgs.media.healthCross;

	if ( cg.snap->ps.stats[ STAT_STATE ] & SS_HEALING_8X )
	{
		shader = cgs.media.healthCross3X;
	}

	else if ( cg.snap->ps.stats[ STAT_STATE ] & SS_HEALING_4X )
	{
		if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_ALIENS )
		{
			shader = cgs.media.healthCross2X;
		}

		else
		{
			shader = cgs.media.healthCrossMedkit;
		}
	}

	else if ( cg.snap->ps.stats[ STAT_STATE ] & SS_POISONED )
	{
		shader = cgs.media.healthCrossPoisoned;
	}

	// Pick the alpha value
	Vector4Copy( ref_color, color );

	if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_HUMANS &&
	        cg.snap->ps.stats[ STAT_HEALTH ] < 10 )
	{
		color[ 0 ] = 1.0f;
		color[ 1 ] = color[ 2 ] = 0.0f;
	}

	ref_alpha = ref_color[ 3 ];

	if ( cg.snap->ps.stats[ STAT_STATE ] & SS_HEALING_2X )
	{
		ref_alpha = 1.0f;
	}

	// Don't fade from nothing
	if ( !cg.lastHealthCross )
	{
		cg.lastHealthCross = shader;
	}

	// Fade the icon during transition
	if ( cg.lastHealthCross != shader )
	{
		cg.healthCrossFade += cg.frametime / 500.0f;

		if ( cg.healthCrossFade > 1.0f )
		{
			cg.healthCrossFade = 0.0f;
			cg.lastHealthCross = shader;
		}

		else
		{
			// Fading between two icons
			color[ 3 ] = ref_alpha * cg.healthCrossFade;
			trap_R_SetColor( color );
			CG_DrawPic( rect.x, rect.y, rect.w, rect.h, shader );
			color[ 3 ] = ref_alpha * ( 1.0f - cg.healthCrossFade );
			trap_R_SetColor( color );
			CG_DrawPic( rect.x, rect.y, rect.w, rect.h, cg.lastHealthCross );
			trap_R_SetColor( NULL );
			return;
		}
	}

	// Not fading, draw a single icon
	color[ 3 ] = ref_alpha;
	trap_R_SetColor( color );
	CG_DrawPic( rect.x, rect.y, rect.w, rect.h, shader );
	trap_R_SetColor( NULL );

}

void CG_Rocket_DrawAlienBarbs( void )
{
	int numBarbs = cg.snap->ps.ammo;
	char base[ MAX_STRING_CHARS ];
	char rml[ MAX_STRING_CHARS ] = { 0 };

	if ( !numBarbs )
	{
		trap_Rocket_SetInnerRML( "", 0 );
		return;
	}

	Com_sprintf( base, sizeof( base ), "<img class='barbs' src='%s' />", CG_Rocket_GetAttribute( "src" ) );

	for ( ; numBarbs > 0; numBarbs-- )
	{
		Q_strcat( rml, sizeof( rml ), base );
	}

	trap_Rocket_SetInnerRML( rml, 0 );
}

/*
============
CG_DrawStack

Helper function: draws a "stack" of <val> boxes in a row or column big
enough to fit <max> boxes. <fill> sets the proportion of space in the stack
with box drawn on it - i.e. lower values will result in thinner boxes.
If the boxes are too thin, or if fill is specified as 0, they will be merged
into a single progress bar.
============
*/

#define LALIGN_TOPLEFT     0
#define LALIGN_CENTER      1
#define LALIGN_BOTTOMRIGHT 2

static void CG_DrawStack( rectDef_t *rect, vec4_t color, float fill,
                          int align, float val, int max )
{
	int      i;
	float    each, frac;
	float    nudge;
	float    fmax = max; // we don't want integer division
	qboolean vertical; // a stack taller than it is wide is drawn vertically
	vec4_t   localColor;

	// so that the vertical and horizontal bars can share code, abstract the
	// longer dimension and the alignment parameter
	int length;


	if ( val <= 0 || max <= 0 )
	{
		return; // nothing to draw
	}

	trap_R_SetColor( color );

	if ( rect->h >= rect->w )
	{
		vertical = qtrue;
		length = rect->h;
	}

	else
	{
		vertical = qfalse;
		length = rect->w;
	}

	// the filled proportion of the length, divided into max bits
	each = fill * length / fmax;

	// if the scaled length of each segment is too small, do a single bar
	if ( each * ( vertical ? cgs.screenYScale : cgs.screenXScale ) < 4.f )
	{
		float loff;

		switch ( align )
		{
			case LALIGN_TOPLEFT:
			default:
				loff = 0;
				break;

			case LALIGN_CENTER:
				loff = length * ( 1 - val / fmax ) / 2;
				break;

			case LALIGN_BOTTOMRIGHT:
				loff = length * ( 1 - val / fmax );
		}

		if ( vertical )
		{
			CG_DrawPic( rect->x, rect->y + loff, rect->w, rect->h * val / fmax,
			            cgs.media.whiteShader );
		}

		else
		{
			CG_DrawPic( rect->x + loff, rect->y, rect->w * val / fmax, rect->h,
			            cgs.media.whiteShader );
		}

		trap_R_SetColor( NULL );
		return;
	}

	// the code would normally leave a bit of space after every square, but this
	// leaves a space on the end, too: nudge divides that space among the blocks
	// so that the beginning and end line up perfectly
	if ( fmax > 1 )
	{
		nudge = ( 1 - fill ) / ( fmax - 1 );
	}

	else
	{
		nudge = 0;
	}

	frac = val - ( int ) val;

	for ( i = ( int ) val - 1; i >= 0; i-- )
	{
		float start;

		switch ( align )
		{
			case LALIGN_TOPLEFT:
				start = ( i * ( 1 + nudge ) + frac ) / fmax;
				break;

			case LALIGN_CENTER:

				// TODO (fallthrough for now)
			default:
			case LALIGN_BOTTOMRIGHT:
				start = 1 - ( val - i - ( i + fmax - val ) * nudge ) / fmax;
				break;
		}

		if ( vertical )
		{
			CG_DrawPic( rect->x, rect->y + rect->h * start, rect->w, each,
			            cgs.media.whiteShader );
		}

		else
		{
			CG_DrawPic( rect->x + rect->w * start, rect->y, each, rect->h,
			            cgs.media.whiteShader );
		}
	}

	// if there is a partial square, draw it dropping off the end of the stack
	if ( frac <= 0.f )
	{
		trap_R_SetColor( NULL );
		return; // no partial square, we're done here
	}

	Vector4Copy( color, localColor );
	localColor[ 3 ] *= frac;
	trap_R_SetColor( localColor );

	switch ( align )
	{
		case LALIGN_TOPLEFT:
			if ( vertical )
			{
				CG_DrawPic( rect->x, rect->y - rect->h * ( 1 - frac ) / fmax,
				            rect->w, each, cgs.media.whiteShader );
			}

			else
			{
				CG_DrawPic( rect->x - rect->w * ( 1 - frac ) / fmax, rect->y,
				            each, rect->h, cgs.media.whiteShader );
			}

			break;

		case LALIGN_CENTER:

			// fallthrough
		default:
		case LALIGN_BOTTOMRIGHT:
			if ( vertical )
			{
				CG_DrawPic( rect->x, rect->y + rect->h *
				            ( 1 + ( ( 1 - fill ) / fmax ) - frac / fmax ),
				            rect->w, each, cgs.media.whiteShader );
			}

			else
			{
				CG_DrawPic( rect->x + rect->w *
				            ( 1 + ( ( 1 - fill ) / fmax ) - frac / fmax ), rect->y,
				            each, rect->h, cgs.media.whiteShader );
			}
	}

	trap_R_SetColor( NULL );
}

static void CG_DrawPlayerAmmoStack( void )
{
	float         val;
	int           maxVal, align;
	static int    lastws, maxwt, lastval, valdiff;
	playerState_t *ps = &cg.snap->ps;
	weapon_t      primary = BG_PrimaryWeapon( ps->stats );
	vec4_t        localColor, foreColor;
	rectDef_t     rect;
	static char   buf[ 100 ];

	// grab info from libRocket
	CG_GetRocketElementColor( foreColor );
	CG_GetRocketElementRect( &rect );

	maxVal = BG_Weapon( primary )->maxAmmo;

	if ( maxVal <= 0 )
	{
		return; // not an ammo-carrying weapon
	}

	val = ps->ammo;

	// smoothing effects (only if weaponTime etc. apply to primary weapon)
	if ( ps->weapon == primary && ps->weaponTime > 0 &&
	        ( ps->weaponstate == WEAPON_FIRING ||
	          ps->weaponstate == WEAPON_RELOADING ) )
	{
		// track the weaponTime we're coming down from
		// if weaponstate changed, this value is invalid
		if ( lastws != ps->weaponstate || ps->weaponTime > maxwt )
		{
			maxwt = ps->weaponTime;
			lastws = ps->weaponstate;
		}

		// if reloading, move towards max ammo value
		if ( ps->weaponstate == WEAPON_RELOADING )
		{
			val = maxVal;
		}

		// track size of change in ammo
		if ( lastval != val )
		{
			valdiff = lastval - val; // can be negative
			lastval = val;
		}

		if ( maxwt > 0 )
		{
			float f = ps->weaponTime / ( float ) maxwt;
			// move from last ammo value to current
			val += valdiff * f * f;
		}
	}

	else
	{
		// reset counters
		lastval = val;
		valdiff = 0;
		lastws = ps->weaponstate;
	}

	if ( val == 0 )
	{
		return; // nothing to draw
	}

	if ( val * 3 < maxVal )
	{
		// low on ammo
		// FIXME: don't hardcode this colour
		vec4_t lowAmmoColor = { 1.f, 0.f, 0.f, 0.f };
		// don't lerp alpha
		VectorLerpTrem( ( cg.time & 128 ), foreColor, lowAmmoColor, localColor );
		localColor[ 3 ] = foreColor[ 3 ];
	}

	else
	{
		Vector4Copy( foreColor, localColor );
	}

	trap_Rocket_GetProperty( "text-align", buf, sizeof( buf ), ROCKET_STRING );

	if ( *buf && !Q_stricmp( buf, "right" ) )
	{
		align = LALIGN_BOTTOMRIGHT;
	}

	else if ( *buf && !Q_stricmp( buf, "center" ) )
	{
		align = LALIGN_CENTER;
	}

	else
	{
		align = LALIGN_TOPLEFT;
	}

	CG_DrawStack( &rect, localColor, 0.8, align, val, maxVal );
}

static void CG_DrawPlayerClipsStack( void )
{
	float         val;
	int           maxVal;
	static int    lastws, maxwt;
	playerState_t *ps = &cg.snap->ps;
	rectDef_t      rect;
	vec4_t         foreColor;

	// grab info from libRocket
	CG_GetRocketElementColor( foreColor );
	CG_GetRocketElementRect( &rect );

	maxVal = BG_Weapon( BG_PrimaryWeapon( ps->stats ) )->maxClips;

	if ( !maxVal )
	{
		return; // not a clips weapon
	}

	val = ps->clips;

	// if reloading, do fancy interpolation effects
	if ( ps->weaponstate == WEAPON_RELOADING )
	{
		float frac;

		// if we just started a reload, note the weaponTime we're coming down from
		if ( lastws != ps->weaponstate || ps->weaponTime > maxwt )
		{
			maxwt = ps->weaponTime;
			lastws = ps->weaponstate;
		}

		// just in case, don't divide by zero
		if ( maxwt != 0 )
		{
			frac = ps->weaponTime / ( float ) maxwt;
			val -= 1 - frac * frac; // speed is proportional to distance from target
		}
	}

	CG_DrawStack( &rect, foreColor, 0.8, LALIGN_TOPLEFT, val, maxVal );
}

void CG_Rocket_DrawMinimap( void )
{
	if ( cg.minimap.defined )
	{
		vec4_t    foreColor;
		rectDef_t rect;

		// grab info from libRocket
		CG_GetRocketElementColor( foreColor );
		CG_GetRocketElementRect( &rect );

		CG_DrawMinimap( &rect, foreColor );
	}
}

#define FOLLOWING_STRING "following "
#define CHASING_STRING   "chasing "
void CG_Rocket_DrawFollow( void )
{
	if ( cg.snap && cg.snap->ps.pm_flags & PMF_FOLLOW )
	{
		char buffer[ MAX_STRING_CHARS ];

		if ( !cg.chaseFollow )
		{
			Q_strncpyz( buffer, FOLLOWING_STRING, sizeof( buffer ) );
		}

		else
		{
			Q_strncpyz( buffer, CHASING_STRING, sizeof( buffer ) );
		}

		Q_strcat( buffer, sizeof( buffer ), cgs.clientinfo[ cg.snap->ps.clientNum ].name );

		trap_Rocket_SetInnerRML( buffer, RP_EMOTICONS );
	}
	else
	{
		trap_Rocket_SetInnerRML( "", 0 );
	}
}

void CG_Rocket_DrawConnectText( void )
{
	char       rml[ MAX_STRING_CHARS ];
	const char *s;

	if ( !Q_stricmp( rocketInfo.cstate.servername, "localhost" ) )
	{
		Q_strncpyz( rml, "Starting up…", sizeof( rml ) );
	}

	else
	{
		Q_strncpyz( rml, va( "Connecting to %s <br/>", rocketInfo.cstate.servername ), sizeof( rml ) );
	}

	if ( rocketInfo.cstate.connState < CA_CONNECTED && *rocketInfo.cstate.messageString )
	{
		Q_strcat( rml, sizeof( rml ), "<br />" );
		Q_strcat( rml, sizeof( rml ), rocketInfo.cstate.messageString );
	}

	switch ( rocketInfo.cstate.connState )
	{
		case CA_CONNECTING:
			s = va( _( "Awaiting connection…%i" ), rocketInfo.cstate.connectPacketCount );
			break;

		case CA_CHALLENGING:
			s = va( _( "Awaiting challenge…%i" ), rocketInfo.cstate.connectPacketCount );
			break;

		case CA_CONNECTED:
			s = _( "Awaiting gamestate…" );
		break;

		case CA_LOADING:
			return;

		case CA_PRIMED:
			return;

		default:
			return;
	}

	Q_strcat( rml, sizeof( rml ), s );

	trap_Rocket_SetInnerRML( rml, 0 );
}

void CG_Rocket_DrawClock( void )
{
	char    *s;
	qtime_t qt;

	if ( !cg_drawClock.integer )
	{
		return;
	}

	Com_RealTime( &qt );

	if ( cg_drawClock.integer == 2 )
	{
		s = va( "%02d%s%02d", qt.tm_hour, ( qt.tm_sec % 2 ) ? ":" : " ",
		        qt.tm_min );
	}

	else
	{
		char *pm = "am";
		int  h = qt.tm_hour;

		if ( h == 0 )
		{
			h = 12;
		}

		else if ( h == 12 )
		{
			pm = "pm";
		}

		else if ( h > 12 )
		{
			h -= 12;
			pm = "pm";
		}

		s = va( "%d%s%02d%s", h, ( qt.tm_sec % 2 ) ? ":" : " ", qt.tm_min, pm );
	}

	trap_Rocket_SetInnerRML( s, 0 );
}

void CG_Rocket_DrawTutorial( void )
{
	if ( !cg_tutorial.integer )
	{
		trap_Rocket_SetInnerRML( "", 0 );
		return;
	}

	trap_Rocket_SetInnerRML( CG_TutorialText(), RP_EMOTICONS );
}

void CG_Rocket_DrawStaminaBolt( void )
{
	qboolean  activate = cg.snap->ps.stats[ STAT_STATE ] & SS_SPEEDBOOST;
	trap_Rocket_SetClass( "sprint", activate );
	trap_Rocket_SetClass( "walk", !activate );
}

void CG_Rocket_DrawChatType( void )
{
	static const struct {
		char colour[4]; // ^n
		char prompt[12];
	} sayText[] = {
		{ "",   "" },
		{ "^2", N_("Say: ") },
		{ "^5", N_("Team Say: ") },
		{ "^6", N_("Admin Say: ") },
		{ "",   N_("Command: ") },
	};

	if ( (size_t) cg.sayType < ARRAY_LEN( sayText ) )
	{
		const char *prompt = _( sayText[ cg.sayType ].prompt );

		if ( ui_chatPromptColors.integer )
		{
			trap_Rocket_SetInnerRML( va( "%s%s", sayText[ cg.sayType ].colour, prompt ), RP_QUAKE );
		}
		else
		{
			trap_Rocket_SetInnerRML( prompt, RP_QUAKE );
		}
	}
}

#define MOMENTUM_BAR_MARKWIDTH 0.5f
#define MOMENTUM_BAR_GLOWTIME  2000

static void CG_Rocket_DrawPlayerMomentumBar( void )
{
	// data
	rectDef_t     rect;
	vec4_t        foreColor, backColor, lockedColor, unlockedColor;
	playerState_t *ps;
	float         momentum, rawFraction, fraction, glowFraction, glowOffset, borderSize;
	int           threshold;
	team_t        team;
	qboolean      unlocked;

	momentumThresholdIterator_t unlockableIter = { -1 };

	// display
	vec4_t        color;
	float         x, y, w, h, b, glowStrength;
	qboolean      vertical;

	CG_GetRocketElementRect( &rect );
	CG_GetRocketElementBGColor( backColor );
	CG_GetRocketElementColor( foreColor );
	trap_Rocket_GetProperty( "border-width", &borderSize, sizeof( borderSize ), ROCKET_FLOAT );
	trap_Rocket_GetProperty( "locked-marker-color", &lockedColor, sizeof( lockedColor ), ROCKET_COLOR );
	trap_Rocket_GetProperty( "unlocked-marker-color", &unlockedColor, sizeof( unlockedColor ), ROCKET_COLOR );


	ps = &cg.predictedPlayerState;

	team = ( team_t ) ps->persistant[ PERS_TEAM ];
	momentum = ps->persistant[ PERS_MOMENTUM ] / 10.0f;

	x = rect.x;
	y = rect.y;
	w = rect.w;
	h = rect.h;
	b = 1.0f;

	vertical = ( h > w );

	// draw border
	CG_DrawRect( x, y, w, h, borderSize, backColor );

	// adjust rect to draw inside border
	x += b;
	y += b;
	w -= 2.0f * b;
	h -= 2.0f * b;

	// draw background
	Vector4Copy( backColor, color );
	color[ 3 ] *= 0.5f;
	CG_FillRect( x, y, w, h, color );

	// draw momentum bar
	fraction = rawFraction = momentum / MOMENTUM_MAX;

	if ( fraction < 0.0f )
	{
		fraction = 0.0f;
	}

	else if ( fraction > 1.0f )
	{
		fraction = 1.0f;
	}

	if ( vertical )
	{
		CG_FillRect( x, y + h * ( 1.0f - fraction ), w, h * fraction, foreColor );
	}

	else
	{
		CG_FillRect( x, y, w * fraction, h, foreColor );
	}

	// draw glow on momentum event
	if ( cg.momentumGainedTime + MOMENTUM_BAR_GLOWTIME > cg.time )
	{
		glowFraction = fabs( cg.momentumGained / MOMENTUM_MAX );
		glowStrength = ( MOMENTUM_BAR_GLOWTIME - ( cg.time - cg.momentumGainedTime ) ) /
		               ( float )MOMENTUM_BAR_GLOWTIME;

		if ( cg.momentumGained > 0.0f )
		{
			glowOffset = vertical ? 0 : glowFraction;
		}

		else
		{
			glowOffset = vertical ? glowFraction : 0;
		}

		CG_SetClipRegion( x, y, w, h );

		color[ 0 ] = 1.0f;
		color[ 1 ] = 1.0f;
		color[ 2 ] = 1.0f;
		color[ 3 ] = 0.5f * glowStrength;

		if ( vertical )
		{
			CG_FillRect( x, y + h * ( 1.0f - ( rawFraction + glowOffset ) ), w, h * glowFraction, color );
		}

		else
		{
			CG_FillRect( x + w * ( rawFraction - glowOffset ), y, w * glowFraction, h, color );
		}

		CG_ClearClipRegion();
	}

	// draw threshold markers
	while ( ( unlockableIter = BG_IterateMomentumThresholds( unlockableIter, team, &threshold, &unlocked ) ),
	        ( unlockableIter.num >= 0 ) )
	{
		fraction = threshold / MOMENTUM_MAX;

		if ( fraction > 1.0f )
		{
			fraction = 1.0f;
		}

		if ( vertical )
		{
			CG_FillRect( x, y + h * ( 1.0f - fraction ), w, MOMENTUM_BAR_MARKWIDTH, unlocked ? unlockedColor : lockedColor );
		}

		else
		{
			CG_FillRect( x + w * fraction, y, MOMENTUM_BAR_MARKWIDTH, h, unlocked ? unlockedColor : lockedColor );
		}
	}

	trap_R_SetColor( NULL );

}

void CG_Rocket_DrawMineRate( void )
{
	float levelRate, rate;
	int efficiency;

	// check if builder
	switch ( BG_GetPlayerWeapon( &cg.snap->ps ) )
	{
		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
			break;

		default:
			return;
	}

	levelRate  = cg.predictedPlayerState.persistant[ PERS_MINERATE ] / 10.0f;
	efficiency = cg.predictedPlayerState.persistant[ PERS_RGS_EFFICIENCY ];
	rate       = ( ( efficiency / 100.0f ) * levelRate );

	trap_Rocket_SetInnerRML( va( _( "%.1f BP/min (%d%% × %.1f)" ), rate, efficiency, levelRate ), 0 );
}

static INLINE qhandle_t CG_GetUnlockableIcon( int num )
{
	int index = BG_UnlockableTypeIndex( num );

	switch ( BG_UnlockableType( num ) )
	{
		case UNLT_WEAPON:
			return cg_weapons[ index ].weaponIcon;

		case UNLT_UPGRADE:
			return cg_upgrades[ index ].upgradeIcon;

		case UNLT_BUILDABLE:
			return cg_buildables[ index ].buildableIcon;

		case UNLT_CLASS:
			return cg_classes[ index ].classIcon;
	}

	return 0;
}

static void CG_Rocket_DrawPlayerUnlockedItems( void )
{
	rectDef_t     rect;
	vec4_t        foreColour, backColour;
	momentumThresholdIterator_t unlockableIter = { -1, 1 }, previousIter;

	// data
	team_t    team;

	// display
	float     x, y, w, h, iw, ih, borderSize;
	qboolean  vertical;

	int       icons, counts;
	int       count[ 32 ] = { 0 };
	struct
	{
		qhandle_t shader;
		qboolean  unlocked;
	} icon[ NUM_UNLOCKABLES ]; // more than enough(!)

	CG_GetRocketElementRect( &rect );
	trap_Rocket_GetProperty( "cell-color", backColour, sizeof( vec4_t ), ROCKET_COLOR );
	CG_GetRocketElementColor( foreColour );
	trap_Rocket_GetProperty( "border-width", &borderSize, sizeof( borderSize ), ROCKET_FLOAT );

	team = ( team_t ) cg.predictedPlayerState.persistant[ PERS_TEAM ];

	w = rect.w - 2 * borderSize;
	h = rect.h - 2 * borderSize;

	vertical = ( h > w );

	ih = vertical ? w : h;
	iw = ih * cgs.aspectScale;

	x = rect.x + borderSize;
	y = rect.y + borderSize + ( h - ih ) * vertical;

	icons = counts = 0;

	for ( ;; )
	{
		qhandle_t shader;
		int       threshold;
		qboolean  unlocked;

		previousIter = unlockableIter;
		unlockableIter = BG_IterateMomentumThresholds( unlockableIter, team, &threshold, &unlocked );

		if ( previousIter.threshold != unlockableIter.threshold && icons )
		{
			count[ counts++ ] = icons;
		}

		// maybe exit the loop?
		if ( unlockableIter.num < 0 )
		{
			break;
		}

		// okay, next icon
		shader = CG_GetUnlockableIcon( unlockableIter.num );

		if ( shader )
		{
			icon[ icons ].shader = shader;
			icon[ icons].unlocked = unlocked;
			++icons;
		}
	}

	{
		float gap;
		int i, j;
		vec4_t unlockedBg, lockedBg;

		Vector4Copy( foreColour, unlockedBg );
		unlockedBg[ 3 ] *= 0.0f;  // No background
		Vector4Copy( backColour, lockedBg );
		lockedBg[ 3 ] *= 0.0f;  // No background

		gap = vertical ? ( h - icons * ih ) : ( w - icons * iw );

		if ( counts > 2 )
		{
			gap /= counts - 1;
		}

		for ( i = 0, j = 0; count[ i ]; ++i )
		{
			if ( vertical )
			{
				float yb = y - count[ i ] * ih - i * gap;
				float hb = ( count[ i ] - j ) * ih;
				CG_DrawRect( x - borderSize, yb - borderSize, iw + 2 * borderSize, hb, borderSize, backColour );
				CG_FillRect( x, yb, iw, hb - 2 * borderSize, icon[ j ].unlocked ? unlockedBg : lockedBg );
			}

			else
			{
				float xb = x + j * iw + i * gap;
				float wb = ( count[ i ] - j ) * iw + 2;
				CG_DrawRect( xb - borderSize, y - borderSize, wb, ih + 2 * borderSize, borderSize, backColour );
				CG_FillRect( xb, y, wb - 2 * borderSize, ih, icon[ j ].unlocked ? unlockedBg : lockedBg );
			}

			j = count[ i ];
		}

		for ( i = 0, j = 0; i < icons; ++i )
		{
			trap_R_SetColor( icon[ i ].unlocked ? foreColour : backColour );

			if ( i == count[ j ] )
			{
				++j;

				if ( vertical )
				{
					y -= gap;
				}

				else
				{
					x += gap;
				}
			}

			CG_DrawPic( x, y, iw, ih, icon[ i ].shader );

			if ( vertical )
			{
				y -= ih;
			}

			else
			{
				x += iw;
			}
		}
	}

	trap_R_SetColor( NULL );
}

static void CG_Rocket_DrawVote_internal( team_t team )
{
	char   *s;
	int    sec;
	char   yeskey[ 32 ] = "", nokey[ 32 ] = "";

	if ( !cgs.voteTime[ team ] )
	{
		trap_Rocket_SetInnerRML( "", 0 );
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified[ team ] )
	{
		cgs.voteModified[ team ] = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime[ team ] ) ) / 1000;

	if ( sec < 0 )
	{
		sec = 0;
	}

	if ( cg_tutorial.integer )
	{
		Com_sprintf( yeskey, sizeof( yeskey ), "[%s]",
		             CG_KeyBinding( va( "%svote yes", team == TEAM_NONE ? "" : "team" ), team ) );
		Com_sprintf( nokey, sizeof( nokey ), "[%s]",
		             CG_KeyBinding( va( "%svote no", team == TEAM_NONE ? "" : "team" ), team ) );
	}

	s = va( "%sVOTE(%i): %s\n"
	        "    Called by: \"%s\"\n"
	        "    %s[check]:%i %s[cross]:%i\n",
	        team == TEAM_NONE ? "" : "TEAM", sec, cgs.voteString[ team ],
	        cgs.voteCaller[ team ], yeskey, cgs.voteYes[ team ], nokey, cgs.voteNo[ team ] );

	trap_Rocket_SetInnerRML( s, RP_EMOTICONS );
}

static void CG_Rocket_DrawVote( void )
{
	CG_Rocket_DrawVote_internal( TEAM_NONE );
}

static void CG_Rocket_DrawTeamVote( void )
{
	CG_Rocket_DrawVote_internal( ( team_t ) cg.predictedPlayerState.persistant[ PERS_TEAM ] );
}

static void CG_Rocket_DrawSpawnQueuePosition( void )
{
	int    position;
	const char *s;

	if ( !( cg.snap->ps.pm_flags & PMF_QUEUED ) )
	{
		trap_Rocket_SetInnerRML( "", 0 );
		return;
	}

	position = cg.snap->ps.persistant[ PERS_SPAWNQUEUE ] >> 8;

	if ( position < 1 )
	{
		return;
	}

	if ( position == 1 )
	{
		s = va( _( "You are at the front of the spawn queue" ) );
	}

	else
	{
		s = va( _( "You are at position %d in the spawn queue" ), position );
	}

	trap_Rocket_SetInnerRML( s, 0 );
}

static void CG_Rocket_DrawNumSpawns( void )
{
	int    position, spawns;
	const char *s;

	if ( !( cg.snap->ps.pm_flags & PMF_QUEUED ) )
	{
		return;
	}

	spawns   = cg.snap->ps.persistant[ PERS_SPAWNQUEUE ] & 0x000000ff;
	position = cg.snap->ps.persistant[ PERS_SPAWNQUEUE ] >> 8;

	if ( position < 1 )
	{
		return;
	}

	if ( spawns == 0 )
	{
		s = va( _( "There are no spawns remaining" ) );
	}

	else
	{
		s = va( P_( "There is 1 spawn remaining", "There are %d spawns remaining", spawns ), spawns );
	}

	trap_Rocket_SetInnerRML( s, 0 );
}

void CG_Rocket_DrawPredictedRGSRate( void )
{
	playerState_t  *ps = &cg.snap->ps;
	buildable_t   buildable = ( buildable_t )( ps->stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK );
	char color;
	int  delta = ps->stats[ STAT_PREDICTION ];

	if ( buildable != BA_H_DRILL && buildable != BA_A_LEECH )
	{
		trap_Rocket_SetInnerRML( "", 0 );
		return;
	}

	if ( delta < 0 )
	{
		color = COLOR_RED;
	}

	else if ( delta < 10 )
	{
		color = COLOR_ORANGE;
	}

	else if ( delta < 50 )
	{
		color = COLOR_YELLOW;
	}

	else
	{
		color = COLOR_GREEN;
	}

	trap_Rocket_SetInnerRML( va( "^%c%+d%%", color, delta ), RP_QUAKE );
}

static void CG_Rocket_DrawWarmup( void )
{
	int   sec = 0;

	if ( !cg.warmupTime )
	{
		trap_Rocket_SetInnerRML( "", 0 );
		return;
	}

	sec = ( cg.warmupTime - cg.time ) / 1000;

	if ( sec < 0 )
	{
		trap_Rocket_SetInnerRML( "", 0 );
		return;
	}

	trap_Rocket_SetInnerRML( va( "%s", sec ? va( "%d", sec ) : _("FIGHT!") ), 0 );
}

static void CG_Rocket_DrawProgressValue( void )
{
	const char *src = CG_Rocket_GetAttribute( "src" );
	if ( *src )
	{
		float value = CG_Rocket_ProgressBarValueByName( src );
		trap_Rocket_SetInnerRML( va( "%d", (int) ( value * 100 ) ), 0 );
	}
}

static void CG_Rocket_DrawLevelName( void )
{
	trap_Rocket_SetInnerRML( CG_ConfigString( CS_MESSAGE ), RP_QUAKE );
}

static void CG_Rocket_DrawMOTD( void )
{
	const char *s;
	char       parsed[ MAX_STRING_CHARS ];

	s = CG_ConfigString( CS_MOTD );
	Q_ParseNewlines( parsed, s, sizeof( parsed ) );
	trap_Rocket_SetInnerRML( parsed, RP_EMOTICONS );
}

static void CG_Rocket_DrawHostname( void )
{
	const char *info;
	info = CG_ConfigString( CS_SERVERINFO );
	trap_Rocket_SetInnerRML( Info_ValueForKey( info, "sv_hostname" ), RP_QUAKE );
}

static void CG_Rocket_DrawDownloadName( void )
{
	char downloadName[ MAX_STRING_CHARS ];

	trap_Cvar_VariableStringBuffer( "cl_downloadName", downloadName, sizeof( downloadName ) );

	if ( Q_stricmp( downloadName, rocketInfo.downloadName ) )
	{
		Q_strncpyz( rocketInfo.downloadName, downloadName, sizeof( rocketInfo.downloadName ) );
		trap_Rocket_SetInnerRML( rocketInfo.downloadName, RP_QUAKE );
	}
}

static void CG_Rocket_DrawDownloadTime( void )
{
	float downloadSize = trap_Cvar_VariableValue( "cl_downloadSize" );
	float downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );
	float downloadTime = trap_Cvar_VariableValue( "cl_downloadTime" );
	int xferRate;

	if ( !*rocketInfo.downloadName )
	{
		return;
	}

	if ( ( rocketInfo.realtime - downloadTime ) / 1000 )
	{
		xferRate = downloadCount / ( ( rocketInfo.realtime - downloadTime ) / 1000 );
	}
	else
	{
		xferRate = 0;
	}

	if ( xferRate && downloadSize )
	{
		char dlTimeBuf[ MAX_STRING_CHARS ];
		int n = downloadSize / xferRate; // estimated time for entire d/l in secs

		// We do it in K (/1024) because we'd overflow around 4MB
		CG_PrintTime( dlTimeBuf, sizeof dlTimeBuf,
			      ( n - ( ( ( downloadCount / 1024 ) * n ) / ( downloadSize / 1024 ) ) ) * 1000 );
		trap_Rocket_SetInnerRML( dlTimeBuf, RP_QUAKE );
	}
	else
	{
		trap_Rocket_SetInnerRML( _( "estimating" ), RP_QUAKE );
	}
}

static void CG_Rocket_DrawDownloadTotalSize( void )
{
	char totalSizeBuf[ MAX_STRING_CHARS ];
	float downloadSize = trap_Cvar_VariableValue( "cl_downloadSize" );

	if ( !*rocketInfo.downloadName )
	{
		return;
	}

	CG_ReadableSize( totalSizeBuf,  sizeof totalSizeBuf,  downloadSize );

	trap_Rocket_SetInnerRML( totalSizeBuf, RP_QUAKE );
}

static void CG_Rocket_DrawDownloadCompletedSize( void )
{
	char dlSizeBuf[ MAX_STRING_CHARS ];
	float downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );

	if ( !*rocketInfo.downloadName )
	{
		return;
	}

	CG_ReadableSize( dlSizeBuf,  sizeof dlSizeBuf,  downloadCount );

	trap_Rocket_SetInnerRML( dlSizeBuf, RP_QUAKE );
}

static void CG_Rocket_DrawDownloadSpeed( void )
{
	char xferRateBuf[ MAX_STRING_CHARS ];
	float downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );
	float downloadTime = trap_Cvar_VariableValue( "cl_downloadTime" );
	int xferRate;

	if ( !*rocketInfo.downloadName )
	{
		return;
	}

	if ( ( rocketInfo.realtime - downloadTime ) / 1000 )
	{
		xferRate = downloadCount / ( ( rocketInfo.realtime - downloadTime ) / 1000 );
		CG_ReadableSize( xferRateBuf, sizeof xferRateBuf, xferRate );
		trap_Rocket_SetInnerRML( va( "%s/Sec", xferRateBuf ), RP_QUAKE );
	}
	else
	{
		xferRate = 0;
		trap_Rocket_SetInnerRML( "0 KB/Sec", RP_QUAKE );
	}
}

static void CG_Rocket_HaveJetpck( void )
{
	qboolean jetpackInInventory = BG_InventoryContainsUpgrade( UP_JETPACK, cg.snap->ps.stats );
	trap_Rocket_SetClass( "active", jetpackInInventory );
	trap_Rocket_SetClass( "inactive", !jetpackInInventory );
}

typedef struct
{
	const char *name;
	void ( *exec )( void );
	rocketElementType_t type;
} elementRenderCmd_t;

// THESE MUST BE ALPHABETIZED
static const elementRenderCmd_t elementRenderCmdList[] =
{
	{ "alien_sense", &CG_Rocket_DrawAlienSense, ELEMENT_ALIENS },
	{ "ammo", &CG_Rocket_DrawAmmo, ELEMENT_BOTH },
	{ "ammo_stack", &CG_DrawPlayerAmmoStack, ELEMENT_HUMANS },
	{ "barbs", &CG_Rocket_DrawAlienBarbs, ELEMENT_ALIENS },
	{ "center_print", &CG_Rocket_DrawCenterPrint, ELEMENT_GAME },
	{ "chattype", &CG_Rocket_DrawChatType, ELEMENT_ALL },
	{ "clips", &CG_Rocket_DrawClips, ELEMENT_HUMANS },
	{ "clip_stack", &CG_DrawPlayerClipsStack, ELEMENT_HUMANS },
	{ "clock", &CG_Rocket_DrawClock, ELEMENT_ALL },
	{ "connecting", &CG_Rocket_DrawConnectText, ELEMENT_ALL },
	{ "credits", &CG_Rocket_DrawCreditsValue, ELEMENT_HUMANS },
	{ "crosshair", &CG_Rocket_DrawCrosshair, ELEMENT_BOTH },
	{ "crosshair_indicator", &CG_Rocket_DrawCrosshairIndicator, ELEMENT_BOTH },
	{ "crosshair_name", &CG_Rocket_DrawCrosshairNames, ELEMENT_GAME },
	{ "downloadCompletedSize", &CG_Rocket_DrawDownloadCompletedSize, ELEMENT_ALL },
	{ "downloadName", &CG_Rocket_DrawDownloadName, ELEMENT_ALL },
	{ "downloadSpeed", &CG_Rocket_DrawDownloadSpeed, ELEMENT_ALL },
	{ "downloadTime", &CG_Rocket_DrawDownloadTime, ELEMENT_ALL },
	{ "downloadTotalSize", &CG_Rocket_DrawDownloadTotalSize, ELEMENT_ALL },
	{ "evos", &CG_Rocket_DrawAlienEvosValue, ELEMENT_ALIENS },
	{ "follow", &CG_Rocket_DrawFollow, ELEMENT_GAME },
	{ "fps", &CG_Rocket_DrawFPS, ELEMENT_ALL },
	{ "health", &CG_Rocket_DrawPlayerHealth, ELEMENT_BOTH },
	{ "health_cross", &CG_Rocket_DrawPlayerHealthCross, ELEMENT_BOTH },
	{ "hostname", &CG_Rocket_DrawHostname, ELEMENT_ALL },
	{ "inventory", &CG_DrawHumanInventory, ELEMENT_HUMANS },
	{ "itemselect_text", &CG_DrawItemSelectText, ELEMENT_HUMANS },
	{ "jetpack", &CG_Rocket_HaveJetpck, ELEMENT_HUMANS },
	{ "lagometer", &CG_Rocket_DrawLagometer, ELEMENT_GAME },
	{ "levelname", &CG_Rocket_DrawLevelName, ELEMENT_ALL },
	{ "levelshot", &CG_Rocket_DrawLevelshot, ELEMENT_ALL },
	{ "levelshot_loading", &CG_Rocket_DrawMapLoadingLevelshot, ELEMENT_ALL },
	{ "location", &CG_Rocket_DrawLocation, ELEMENT_GAME },
	{ "mine_rate", &CG_Rocket_DrawMineRate, ELEMENT_BOTH },
	{ "minimap", &CG_Rocket_DrawMinimap, ELEMENT_ALL },
	{ "momentum", &CG_Rocket_DrawMomentum, ELEMENT_BOTH },
	{ "momentum_bar", &CG_Rocket_DrawPlayerMomentumBar, ELEMENT_BOTH },
	{ "motd", &CG_Rocket_DrawMOTD, ELEMENT_ALL },
	{ "numSpawns", &CG_Rocket_DrawNumSpawns, ELEMENT_DEAD },
	{ "predictedMineEfficiency", &CG_Rocket_DrawPredictedRGSRate, ELEMENT_BOTH },
	{ "progress_value", &CG_Rocket_DrawProgressValue, ELEMENT_ALL },
	{ "scanner", &CG_Rocket_DrawHumanScanner, ELEMENT_HUMANS },
	{ "spawnPos", &CG_Rocket_DrawSpawnQueuePosition, ELEMENT_DEAD },
	{ "speedometer", &CG_Rocket_DrawSpeedGraph, ELEMENT_GAME },
	{ "stamina", &CG_Rocket_DrawStaminaValue, ELEMENT_HUMANS },
	{ "stamina_bolt", &CG_Rocket_DrawStaminaBolt, ELEMENT_HUMANS },
	{ "timer", &CG_Rocket_DrawTimer, ELEMENT_GAME },
	{ "tutorial", &CG_Rocket_DrawTutorial, ELEMENT_GAME },
	{ "unlocked_items", &CG_Rocket_DrawPlayerUnlockedItems, ELEMENT_BOTH },
	{ "usable_buildable", &CG_Rocket_DrawUsableBuildable, ELEMENT_HUMANS },
	{ "votes", &CG_Rocket_DrawVote, ELEMENT_GAME },
	{ "votes_team", &CG_Rocket_DrawTeamVote, ELEMENT_BOTH },
	{ "wallwalk", &CG_Rocket_DrawPlayerWallclimbing, ELEMENT_ALIENS },
	{ "warmup_time", &CG_Rocket_DrawWarmup, ELEMENT_GAME },
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

	cmd = ( elementRenderCmd_t * ) bsearch( tag, elementRenderCmdList, elementRenderCmdListCount, sizeof( elementRenderCmd_t ), elementRenderCmdCmp );

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
		//Check that the commands are in increasing order so that it can be used by bsearch
		if ( i != 0 && Q_stricmp( elementRenderCmdList[ i - 1 ].name, elementRenderCmdList[ i ].name ) > 0 )
		{
			CG_Printf( "CGame elementRenderCmdList is in the wrong order for %s and %s\n", elementRenderCmdList[i - 1].name, elementRenderCmdList[ i ].name );
		}

		trap_Rocket_RegisterElement( elementRenderCmdList[ i ].name );
	}
}
