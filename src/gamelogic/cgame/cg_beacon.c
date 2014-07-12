/*
===========================================================================

Copyright 2014 Unvanquished Developers

This file is part of Daemon.

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

// cg_beacon.c
// beacon code shared by HUD and minimap

#include "cg_local.h"

/*
=============
CG_NearestBuildable

Returns the nearest _powered_ buildable of the given type (up to 3 types)
and its origin. If a is -1 then return the nearest alien builable.
Ideally this function should take the actual route length into account
and not just the straight line distance.

FIXME: Make it take a func as an argument instead of a weird spam of ints.
       Probably will need lambda functions or else it'll be even messier.
=============
*/
static int CG_NearestBuildable( int a, int b, int c, vec3_t origin )
{
	int i;
	centity_t *cent;
	entityState_t *es, *nearest = NULL;
	float distance, minDistance = 1.0e20;

	for ( i = 0; i < cg.snap->numEntities; i++ )
	{
		cent = cg_entities + cg.snap->entities[ i ].number;
		es = &cent->currentState;

		if( es->eType != ET_BUILDABLE )
			continue;

		if( !( es->eFlags & EF_B_POWERED ) )
			continue;

		if( a == -1 )
		{
			if( BG_Buildable( es->modelindex )->team != TEAM_ALIENS )
				continue;
		}
		else
			if( !( es->modelindex == a ||
		       ( b && es->modelindex == b ) ||
		       ( c && es->modelindex == c ) ) )
				continue;

		if( ( distance = Distance( cg.predictedPlayerState.origin, es->origin ) ) < minDistance )
			minDistance = distance,
			nearest = es;
	}

	if( nearest && origin )
	{
		vec3_t mins, maxs;
		BG_BuildableBoundingBox( nearest->modelindex, mins, maxs );
		for( i = 0; i < 3; i++ )
			origin[ i ] = cg_entities[ nearest->number ].lerpOrigin[ i ] + ( mins[ i ] + maxs[ i ] ) / 2.0;
	}

	if( nearest )
		return nearest->number;
	else
		return ENTITYNUM_NONE;
}

/*
=============
CG_RunBeacon

Handles all animations and sounds.
Called every frame for every beacon.
=============
*/
#define BEACON_FADEIN 200
#define BEACON_FADEOUT 500
#define BEACON_OCCLUSION_FADE_SPEED 14.0
#define BEACON_HIGHLIGHT_FADE_SPEED 25.0
#define BEACON_MARGIN ( base * 0.14f )

#define LinearRemap(x,an,ap,bn,bp) ((x)-(an))/((ap)-(an))*((bp)-(bn))+(bn)

static void CG_RunBeacon( cbeacon_t *b )
{
	qboolean front;
	float vw, vh, base;
	int time_in, time_left;
	float t_fadein, t_fadeout; // t_ stands for "parameter", not "time"
	float target; // for exponential fades
	trace_t tr;

	vw = cgs.glconfig.vidWidth;
	vh = cgs.glconfig.vidHeight;
	base = MIN( vw, vh );

	// reset animations
	b->scale = 1.0;
	b->alpha = 1.0;

	time_in = cg.time - b->s->ctime; // time since creation
	time_left = b->s->etime - cg.time; // time to expiration

	// handle new beacons
	if( !b->s->old )
	{
		// clear static data (cg_ents already does memset zero but that won't work for floats)
		b->s->t_occlusion = 1.0;
		b->s->t_highlight = 0.0;

		// creation events
		// note: old beacons will be treated by cgame as new if it's the
		//       first time it sees them (e.g. after changing teams),
		//       so omit the events if the event was created long time ago
		if( time_in < 1000 &&
		    !( b->flags & EF_BC_NO_TARGET ) )
			if( BG_Beacon( b->type )->sound )
				trap_S_StartLocalSound( BG_Beacon( b->type )->sound, CHAN_LOCAL_SOUND );
	}

	// detached
	if( !( b->s->oldFlags & EF_BC_NO_TARGET ) &&
	    ( b->flags & EF_BC_NO_TARGET ) )
		trap_S_StartLocalSound( cgs.media.beaconTargetLostSound, CHAN_LOCAL_SOUND );

	// fade in
	if ( time_in >= BEACON_FADEIN )
		t_fadein = 1.0;
	else
		t_fadein = (float)time_in / BEACON_FADEIN;
	b->scale *= Square( 1.0 - t_fadein ) + 1.0;

	// fade out
	if ( b->s->etime && time_left < BEACON_FADEOUT )
		t_fadeout = 1.0 - (float)time_left / BEACON_FADEOUT;
	else
		t_fadeout = 0.0;
	b->alpha *= 1.0 - t_fadeout;

	// pulsation
	if( b->type == BCT_HEALTH && time_in > 600 )
		b->scale *= 1.0 + Square( 1.0 - (float)( time_in % 600 ) / 600.0 ) * 0.4;

	// timer expired
	if( b->type == BCT_TIMER )
	{
		float t;

		if( time_in >= BEACON_TIMER_TIME + BEACON_FADEIN )
			t = 1.0;
		else if( time_in >= BEACON_TIMER_TIME )
		{
			t = (float)( time_in - BEACON_TIMER_TIME ) / BEACON_FADEIN;
			
			if( !b->s->eventFired )
			{
				trap_S_StartLocalSound( cgs.media.timerBeaconExpiredSound, CHAN_LOCAL_SOUND );
				b->s->eventFired = qtrue;
			}
		}
		else
			t = 1.0;

		b->scale *= Square( 1.0 - t ) * 2.0 + 1.0;
	}

	// fade when too close
	if( b->dist < 500 )
		b->alpha *= LinearRemap( b->dist, 0, 500, 0.5, 1 );

	// occlusion fade
	target = 1.0;
	if ( !b->highlighted )
	{
		CG_Trace( &tr, cg.refdef.vieworg, NULL, NULL, b->s->origin, ENTITYNUM_NONE, CONTENTS_SOLID );
		if ( tr.fraction < 0.99f )
			target = 0;
	}
	CG_ExponentialFade( &b->s->t_occlusion, target, 10 );
	b->alpha *= LinearRemap( b->s->t_occlusion, 0, 1, 0.5, 1 );

	// highlight animation
	target = ( b->highlighted ? 1.0 : 0.0 );
	CG_ExponentialFade( &b->s->t_highlight, target, 20 );
	b->scale *= LinearRemap( b->s->t_highlight, 0, 1, 1, 1.5 );

	// calculate HUD size
	#define BEACON_SIZE      ( 25.0 * base )
	#define BEACON_SIZE_MIN  ( 0.029 * base )
	#define BEACON_SIZE_MAX  ( 0.086 * base )

	b->size = BEACON_SIZE / b->dist;
	if( b->size > BEACON_SIZE_MAX )
		b->size = BEACON_SIZE_MAX;
	else if ( b->size < BEACON_SIZE_MIN )
		b->size = BEACON_SIZE_MIN;
	b->size *= b->scale * cg_beaconHUDScale.value;

	// project onto screen
	front = CG_WorldToScreen( b->s->origin, b->pos_proj, b->pos_proj + 1);

	// CG_WorldToScreen flips the result if behind so correct it
	if( !front )
	{
		b->pos_proj[ 0 ] = 640.0 - b->pos_proj[ 0 ],
		b->pos_proj[ 1 ] = 480.0 - b->pos_proj[ 1 ];
	}

	// virtual 640x480 to real
	b->pos_proj[ 0 ] *= vw / 640.0;
	b->pos_proj[ 1 ] *= vh / 480.0;

	// clamp to edges
	if( !front || b->pos_proj[ 0 ] < BEACON_MARGIN + b->size/2 || b->pos_proj[ 0 ] > vw - BEACON_MARGIN - b->size/2 ||
								b->pos_proj[ 1 ] < BEACON_MARGIN + b->size/2 || b->pos_proj[ 1 ] > vh - BEACON_MARGIN - b->size/2 )
	{
		vec2_t screen[ 2 ], point;
		Vector2Set( screen[ 0 ], (vec_t)BEACON_MARGIN + b->size/2, (vec_t)BEACON_MARGIN + b->size/2 );
		Vector2Set( screen[ 1 ], (vec_t)( vw - BEACON_MARGIN - b->size/2 ), (vec_t)( vh - BEACON_MARGIN - b->size/2 ) );
		Vector2Set( point, (vec_t)vw / 2.0 , (vec_t)vh / 2.0 );
		b->clamp_dir[ 0 ] = b->pos_proj[ 0 ] - point[ 0 ];
		b->clamp_dir[ 1 ] = b->pos_proj[ 1 ] - point[ 1 ];
		ProjectPointOntoRectangleOutwards( b->pos_proj, point, b->clamp_dir, (const vec2_t*)screen );
		b->clamped = qtrue;
	}
	else
		b->clamped = qfalse;
}

/*
=============
CG_BeaconDynamics
=============
*/

void CG_BeaconDynamics( void )
{
#if 0 //doesn't work properly yet
	float dt, r;
	cbeacon_t *a, *b;
	vec2_t delta;

	dt = 0.001f * cg.frametime;

	//calculate forces
	for( a = cg.beacons; a < cg.beacons + cg.num_beacons; a++ )
	{
		Vector2Set( a->s->acc, 0, 0 );

		//init if new
		if( !a->s->old )
		{
			Vector2Copy( a->pos_proj, a->s->pos );
			Vector2Set( a->s->vel, 0, 0 );
			return;
		}

/*
 * 		//spring
		VEC2 delta[j] = x0[j] - x[j];
		r = LEN( delta );
		VEC2 a[j] += delta[j] * r;

		//damping
		r = LEN( v );
		VEC2 a[j] += v[j] * -r * 0.9;

		VEC2 v[j] += a[j] * dt;
		VEC2 x[j] += v[j] * dt;*/

		//spring
		Vector2Subtract( a->s->pos, a->pos_proj, delta );
		//r = sqrt( Square( delta[ 0 ] ) + Square( delta[ 1 ] ) );
		a->s->acc[ 0 ] += delta[ 0 ] * -300.0f;
		a->s->acc[ 1 ] += delta[ 1 ] * -300.0f;

		//damping
		r = sqrt( Square( a->s->vel[ 0 ] ) + Square( a->s->vel[ 1 ] ) );
		a->s->acc[ 0 ] += a->s->vel[ 0 ] * -r * 0.02f;
		a->s->acc[ 1 ] += a->s->vel[ 1 ] * -r * 0.02f;
/*
		//external forces
		for( b = cg.beacons; b < cg.beacons + cg.num_beacons; b++ )
		{
			if( a == b )
				continue;

			Vector2Subtract( a->s->pos, b->s->pos, delta );
			r = sqrt( Square( delta[ 0 ] ) + Square( delta[ 1 ] ) );

			a->s->acc[ 0 ] += delta[ 0 ] / r * 150.0;
			a->s->acc[ 1 ] += delta[ 1 ] / r * 150.0;
		}*/
	}

	//integrate
	for( a = cg.beacons; a < cg.beacons + cg.num_beacons; a++ )
	{
		a->s->vel[ 0 ] += a->s->acc[ 0 ] * dt;
		a->s->vel[ 1 ] += a->s->acc[ 1 ] * dt;
		a->s->pos[ 0 ] += a->s->vel[ 0 ] * dt;
		a->s->pos[ 1 ] += a->s->vel[ 1 ] * dt;
	}

#else
	cbeacon_t *b;

	for( b = cg.beacons; b < cg.beacons + cg.num_beacons; b++ )
		Vector2Copy( b->pos_proj, b->s->pos );
#endif
}

/*
=============
CG_CompareBeaconsByDot

Used for qsort
=============
*/
static int CG_CompareBeaconsByDot( const void *a, const void *b )
{
	return ( (const cbeacon_t*)a )->dot > ( (const cbeacon_t*)b )->dot;
}

/*
=============
CG_AddImplicitBeacon

Checks a cbeacon_t and copies it to cg.beacons if valid
This is to be used for client-side beacons (e.g. personal waypoint)
=============
*/
static void CG_AddImplicitBeacon( cbeaconPersistent_t *bp, beaconType_t type )
{
	cbeacon_t *beacon;
	vec3_t vdelta;

	if ( cg.num_beacons >= MAX_CBEACONS )
		return;

	beacon = cg.beacons + cg.num_beacons;

	if ( bp->fadingOut ) // invalid but give it time to fade out
	{
		if ( cg.time >= bp->etime )
		{
			memset( bp, 0, sizeof( cbeaconPersistent_t ) );
			return;
		}
	}
	else if ( !bp->valid ) // invalid
	{
		if ( bp->old ) // invalid & old = start fading out
		{
			bp->fadingOut = qtrue;
			bp->etime = cg.time + BEACON_FADEOUT;
		}
		else
		{
			memset( bp, 0, sizeof( cbeaconPersistent_t ) );
			return;
		}
	}
	
	if ( bp->etime && bp->etime <= cg.time ) // expired
	{
		memset( bp, 0, sizeof( cbeaconPersistent_t ) );
		return;
	}

	beacon->type = type;
	beacon->s = bp;

	// cache some stuff
	VectorSubtract( beacon->s->origin, cg.refdef.vieworg, vdelta );
	VectorNormalize( vdelta );
	beacon->dot = DotProduct( vdelta, cg.refdef.viewaxis[ 0 ] );
	beacon->dist = Distance( cg.predictedPlayerState.origin, beacon->s->origin );

	cg.num_beacons++;
}

/*
=============
CG_ListImplicitBeacons

Figures out all implicit beacons and adds them to cg.beacons
=============
*/
static void CG_ListImplicitBeacons( )
{
	// every implicit beacon must have its very own cbeaconPeristent that
	// is kept between frames, but cbeacon_t can be shared
	static cbeaconPersistent_t bp_health = { qfalse, qfalse };
	static cbeaconPersistent_t bp_ammo = { qfalse, qfalse };
	playerState_t *ps;
	int entityNum;
	qboolean energy;

	ps = &cg.predictedPlayerState;

	bp_health.valid = qfalse;
	bp_ammo.valid = qfalse;

	// need to be in-game to get any personal beacons
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ||
			 cg.snap->ps.pm_type == PM_DEAD ||
	     ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
	{
		memset( &bp_health, 0, sizeof( cbeaconPersistent_t ) );
		memset( &bp_ammo, 0, sizeof( cbeaconPersistent_t ) );
		return;
	}

	// health
	if ( ps->stats[ STAT_HEALTH ] < ps->stats[ STAT_MAX_HEALTH ] / 2 )
	{
		bp_health.altIcon = qfalse;

		if ( ps->persistant[ PERS_TEAM ] == TEAM_ALIENS )
		{
			bp_health.altIcon = qtrue;
			entityNum = CG_NearestBuildable( BA_A_BOOSTER, 0, 0, bp_health.origin );
			/*if( entityNum == ENTITYNUM_NONE )
			{
				entityNum = CG_NearestBuildable( -1, 0, 0, bp_health.origin );
				bp_health.altIcon = qfalse;
			}*/
		}
		else
			entityNum = CG_NearestBuildable( BA_H_MEDISTAT, 0, 0, bp_health.origin );		

		if ( entityNum != ENTITYNUM_NONE )
		{
			// keep it alive
			bp_health.valid = qtrue;
			bp_health.fadingOut = qfalse;
			bp_health.etime = 0;

			// renew if it's a different medistat
			if ( entityNum != bp_health.oldEntityNum )
			{
				bp_health.old = qfalse;
				// this is supposed to be uncommented but jumping smoothly
				// from one medistat to another looks really interesting
				//bp_health.old_hud = qfalse;
			}

			// initialize if new/renewed
			if ( !bp_health.old )
			{
				bp_health.ctime = cg.time;
				bp_health.etime = 0;
			}
		}

		bp_health.oldEntityNum = entityNum;
	}

	// ammo
	if ( ps->persistant[ PERS_TEAM ] == TEAM_HUMANS &&
	     BG_PlayerLowAmmo( ps, &energy ) )
	{
		if ( energy )
			entityNum = CG_NearestBuildable( BA_H_ARMOURY, BA_H_REACTOR, BA_H_REPEATER, bp_ammo.origin );
		else
			entityNum = CG_NearestBuildable( BA_H_ARMOURY, 0, 0, bp_ammo.origin );

		if ( entityNum != ENTITYNUM_NONE )
		{
			// keep it alive
			bp_ammo.valid = qtrue;
			bp_ammo.fadingOut = qfalse;
			bp_ammo.etime = 0;

			// renew if it's a different medistat
			if ( entityNum != bp_ammo.oldEntityNum )
				bp_ammo.old = qfalse;

			// initialize if new/renewed
			if ( !bp_ammo.old )
			{
				bp_ammo.ctime = cg.time;
				bp_ammo.etime = 0;
			}
		}

		bp_ammo.oldEntityNum = entityNum;
	}

	CG_AddImplicitBeacon( &bp_health, BCT_HEALTH );
	CG_AddImplicitBeacon( &bp_ammo, BCT_AMMO );
}

/*
=============
CG_ListBeacons

Fill the cg.beacons array with explicit (ET_BEACON entities) and
implicit beacons.
Used to draw beacons on HUD and the minimap.
=============
*/
void CG_ListBeacons( void )
{
	int i;
	centity_t *cent;
	cbeacon_t *beacon;
	entityState_t *es;
	vec3_t delta;

	cg.num_beacons = 0;

	// add all client-side beacons to cg.beacons
	CG_ListImplicitBeacons();

	beacon = cg.beacons + cg.num_beacons;

	// add all ET_BEACONs to cg.beacons
	for ( i = 0; i < cg.snap->numEntities; i++ )
	{
		cent = cg_entities + cg.snap->entities[ i ].number;
		es = &cent->currentState;

		if( es->eType != ET_BEACON )
			continue;

		if( es->modelindex <= BCT_NONE ||
				es->modelindex >= NUM_BEACON_TYPES )
			continue;

		if( es->time2 && es->time2 <= cg.time ) //expired
			continue;

		beacon->s = &cent->beaconPersistent;

		beacon->s->valid = qtrue;
		beacon->s->ctime = es->time;
		beacon->s->etime = es->time2;

		VectorCopy( es->origin, beacon->s->origin );
		beacon->type = (beaconType_t)es->modelindex;
		beacon->data = es->modelindex2;
		beacon->team = (team_t)es->generic1;
		beacon->owner = es->otherEntityNum;
		beacon->flags = es->eFlags;

		// cache some stuff
		VectorSubtract( beacon->s->origin, cg.refdef.vieworg, delta );
		VectorNormalize( delta );
		beacon->dot = DotProduct( delta, cg.refdef.viewaxis[ 0 ] );
		beacon->dist = Distance( cg.predictedPlayerState.origin, beacon->s->origin );

		cg.num_beacons++, beacon++;
		if( cg.num_beacons >= MAX_CBEACONS )
			break;
	}

	qsort( cg.beacons, cg.num_beacons, sizeof( cbeacon_t ), CG_CompareBeaconsByDot );

	for( i = 0; i < cg.num_beacons; i++ )
		CG_RunBeacon( cg.beacons + i );

	CG_BeaconDynamics( );

	for( i = 0; i < cg.num_beacons; i++ )
	{
		cg.beacons[ i ].s->old = qtrue;
		cg.beacons[ i ].s->oldFlags = cg.beacons[ i ].flags;
	}
}

/*
=============
CG_BeaconIcon

Figures out the icon shader for a beacon.
=============
*/
qhandle_t CG_BeaconIcon( const cbeacon_t *b )
{
	if ( b->type == BCT_TAG )
	{
		if( b->flags & EF_BC_TAG_ALIEN )
		{
			if( b->data <= PCL_NONE || b->data >= PCL_NUM_CLASSES )
				return 0;
			return cg_classes[ b->data ].classIcon;
		}
		else if( b->flags & EF_BC_TAG_HUMAN )
		{
			if( b->data <= WP_NONE || b->data >= WP_NUM_WEAPONS )
				return 0;
			return cg_weapons[ b->data ].weaponIcon;
		}
		else
		{
			if( b->data <= BA_NONE || b->data >= BA_NUM_BUILDABLES )
				return 0;
			return cg_buildables[ b->data ].buildableIcon;
		}
	}

	if ( b->type <= BCT_NONE || b->type >= NUM_BEACON_TYPES )
		return 0;

	if ( b->s->altIcon )
		return BG_Beacon( b->type )->altIcon;
	else
		return BG_Beacon( b->type )->icon;
}

/*
=============
CG_BeaconText

Figures out the text for a beacon.
The returned string is localized.
=============
*/
const char *CG_BeaconText( const cbeacon_t *b )
{
	const char *text = "";

	if ( b->type == BCT_TAG )
	{
		if( b->flags & EF_BC_TAG_ALIEN )
		{
			if( b->data <= PCL_NONE || b->data >= PCL_NUM_CLASSES )
				return 0;
			text = _( BG_ClassModelConfig( b->data )->humanName );
		}
		else if( b->flags & EF_BC_TAG_HUMAN )
		{
			if( b->data <= WP_NONE || b->data >= WP_NUM_WEAPONS )
				return 0;
			text = _( BG_Weapon( b->data )->humanName );
		}
		else
		{
			if( b->data <= BA_NONE || b->data >= BA_NUM_BUILDABLES )
				return 0;
			text = _( BG_Buildable( b->data )->humanName );
		}
	}
	else
		if( BG_Beacon( b->type )->text )
			text = _( BG_Beacon( b->type )->text );

	if ( b->type == BCT_TIMER )
	{
		float delta;
		delta = 0.001f * ( cg.time - b->s->ctime - BEACON_TIMER_TIME );
		return va( "T %c %.2fs", ( delta >= 0 ? '+' : '-' ), fabs( delta ) );
	}

	if ( b->type <= BCT_NONE || b->type >= NUM_BEACON_TYPES )
		return va( "INVALID(%d)", b->type );

	return text;
}
