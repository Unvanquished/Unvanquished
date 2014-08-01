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
CG_LoadBeaconsConfig
=============
*/

void CG_LoadBeaconsConfig( void )
{
	beaconsConfig_t  *bc = &cgs.bc;
	const char       *path = "ui/beacons.cfg";
	int              vw, vh, base, fd;
	pc_token_t       token;

	vw = cgs.glconfig.vidWidth;
	vh = cgs.glconfig.vidHeight;
	base = MIN( vw, vh );

	memset( bc, 0, sizeof( beaconsConfig_t ) );

	bc->hudCenter[ 0 ] = vw / 2;
	bc->hudCenter[ 1 ] = vh / 2;

	fd = trap_Parse_LoadSource( path );
	if ( !fd )
		return;

	while ( 1 )
	{
		if( !trap_Parse_ReadToken( fd, &token ) )
			break;

#define READ_INT(x) \
else if( !Q_stricmp( token.string, #x ) ) \
{ \
	if( !PC_Int_Parse( fd, &bc-> x ) ) \
		break; \
}

#define READ_FLOAT(x) \
else if( !Q_stricmp( token.string, #x ) ) \
{ \
	if( !PC_Float_Parse( fd, &bc-> x ) ) \
		break; \
}

#define READ_FLOAT_S(x) \
else if( !Q_stricmp( token.string, #x ) ) \
{ \
	if( !PC_Float_Parse( fd, &bc-> x ) ) \
		break; \
	bc-> x *= base; \
}

#define READ_COLOR(x) \
else if( !Q_stricmp( token.string, #x ) ) \
{ \
	if( !PC_Color_Parse( fd, &bc-> x ) ) \
		break; \
}

		READ_INT    ( fadeIn )
		READ_INT    ( fadeOut )
		READ_FLOAT_S( highlightRadius )
		READ_FLOAT  ( highlightScale )
		READ_FLOAT  ( fadeMinAlpha )
		READ_FLOAT  ( fadeMaxAlpha )

		READ_COLOR  ( colorNeutral )
		READ_COLOR  ( colorAlien )
		READ_COLOR  ( colorHuman )
		READ_FLOAT_S( arrowWidth )
		READ_FLOAT_S( arrowDotSize )
		READ_FLOAT  ( arrowAlphaLow )
		READ_FLOAT  ( arrowAlphaHigh )

		READ_FLOAT_S( hudSize )
		READ_FLOAT_S( hudMinSize )
		READ_FLOAT_S( hudMaxSize )
		READ_FLOAT  ( hudAlpha )

		READ_FLOAT  ( minimapScale )
		READ_FLOAT  ( minimapAlpha )

		else if( !Q_stricmp( token.string, "hudMargin" ) )
		{
			float margin;

			if( !PC_Float_Parse( fd, &margin ) )
				break;

			margin *= base;

			bc->hudRect[ 0 ][ 0 ] = margin;
			bc->hudRect[ 1 ][ 0 ] = vw - margin;
			bc->hudRect[ 0 ][ 1 ] = margin;
			bc->hudRect[ 1 ][ 1 ] = vh - margin;
		}
		else if( !Q_stricmp( token.string, "tagScoreSize" ) )
		{
			float size;

			if( !PC_Float_Parse( fd, &size ) )
				break;

			size *= base;

			bc->tagScoreSize = size;
			bc->tagScorePos[ 0 ] = bc->hudCenter[ 0 ] - size/2;
			bc->tagScorePos[ 1 ] = bc->hudCenter[ 1 ] - size/2;
		}
	}

	bc->fadeMinDist = Square( bc->hudSize / bc->hudMaxSize );
	bc->fadeMaxDist = Square( bc->hudSize / bc->hudMinSize );

	trap_Parse_FreeSource( fd  );
}

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
#define Distance2(a,b) sqrt(Square((a)[0]-(b)[0])+Square((a)[1]-(b)[1]))

static void CG_RunBeacon( cbeacon_t *b )
{
	float target, alpha;
	int time_in, time_left;
	float t_fadein, t_fadeout; // t_ stands for "parameter", not "time"
	qboolean front;

	// reset animations
	b->scale = 1.0;
	alpha = 1.0;

	time_in = cg.time - b->s->ctime; // time since creation
	time_left = b->s->etime - cg.time; // time to expiration

	// check creation
	if( !b->s->old )
	{
		if( time_in > 1000 )
			b->s->old = qtrue;
		else
		{
			if( b->type == BCT_TAG && !( b->flags & EF_BC_ENEMY ) )
				goto no_in_sound;

			if( b->type == BCT_TAG && b->owner == cg.predictedPlayerState.clientNum )
				trap_S_StartLocalSound( cgs.media.ownedTagSound, CHAN_LOCAL_SOUND );
			else if( BG_Beacon( b->type )->inSound )
				trap_S_StartLocalSound( BG_Beacon( b->type )->inSound, CHAN_LOCAL_SOUND );

			no_in_sound:;
		}
	}

	// check death
	if( b->s->old &&
	    !( b->s->oldFlags & EF_BC_DYING ) &&
	    ( b->flags & EF_BC_DYING ) )
	{
		if( BG_Beacon( b->type )->outSound )
			trap_S_StartLocalSound( BG_Beacon( b->type )->outSound, CHAN_LOCAL_SOUND );
	}

	// fade in
	if ( time_in >= cgs.bc.fadeIn )
		t_fadein = 1.0;
	else
		t_fadein = (float)time_in / cgs.bc.fadeIn;
	b->scale *= Square( 1.0 - t_fadein ) + 1.0;

	// fade out
	if ( b->s->etime && time_left < cgs.bc.fadeOut )
		t_fadeout = 1.0 - (float)time_left / cgs.bc.fadeOut;
	else
		t_fadeout = 0.0;
	alpha *= 1.0 - t_fadeout;

	// pulsation
	if( b->type == BCT_HEALTH && time_in > 600 )
		b->scale *= 1.0 + Square( 1.0 - (float)( time_in % 600 ) / 600.0 ) * 0.4;

	// timer expired
	if( b->type == BCT_TIMER )
	{
		float t;

		if( time_in >= BEACON_TIMER_TIME + cgs.bc.fadeIn )
			t = 1.0;
		else if( time_in >= BEACON_TIMER_TIME )
		{
			t = (float)( time_in - BEACON_TIMER_TIME ) / cgs.bc.fadeIn;

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

	// occlusion effects
	if ( b->type == BCT_TAG && ( b->flags & EF_BC_TAG_PLAYER ) )
	{
		trace_t tr;

		CG_Trace( &tr, cg.refdef.vieworg, NULL, NULL, b->s->origin, ENTITYNUM_NONE, CONTENTS_SOLID );

		target = ( ( tr.fraction > 1.0f - FLT_EPSILON ) ? 1.0 : 0.0 );
		CG_ExponentialFade( &b->s->t_occlusion, target, 10 );
		alpha *= LinearRemap( b->s->t_occlusion, 0, 1, 1, 0.5 );
	}

	// Fade out when too close. Span the same distance as the size change but fade linearly.
	// Do not fade important beacons.
	if( !( BG_Beacon( b->type )->flags & BCF_IMPORTANT ) )
	{
		if( b->dist < cgs.bc.fadeMinDist )
			alpha *= cgs.bc.fadeMinAlpha;
		else if( b->dist > cgs.bc.fadeMaxDist )
			alpha *= cgs.bc.fadeMaxAlpha;
		else
			alpha *= LinearRemap( b->dist, cgs.bc.fadeMinDist, cgs.bc.fadeMaxDist, cgs.bc.fadeMinAlpha, cgs.bc.fadeMaxAlpha );
	}

	// color
	if( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_NONE || b->type == BCT_TAG )
	{
		switch( b->team )
		{
			case TEAM_ALIENS:
				if ( b->flags & EF_BC_ENEMY )
					Vector4Copy( cgs.bc.colorHuman, b->color );
				else
					Vector4Copy( cgs.bc.colorAlien, b->color );
				break;
			case TEAM_HUMANS:
				if ( b->flags & EF_BC_ENEMY )
					Vector4Copy( cgs.bc.colorAlien, b->color );
				else
					Vector4Copy( cgs.bc.colorHuman, b->color );
				break;
			default:
				Vector4Copy( cgs.bc.colorNeutral, b->color );
				break;
		}
	}
	else
		Vector4Copy( cgs.bc.colorNeutral, b->color );
	b->color[ 3 ] *= alpha;

	// calculate HUD size
	b->size = cgs.bc.hudSize / sqrt( b->dist );
	if( b->size > cgs.bc.hudMaxSize )
		b->size = cgs.bc.hudMaxSize;
	else if ( b->size < cgs.bc.hudMinSize )
		b->size = cgs.bc.hudMinSize;
	b->size *= b->scale;

	// project onto screen
	front = CG_WorldToScreen( b->s->origin, b->pos_proj, b->pos_proj + 1);

	// CG_WorldToScreen flips the result if behind so correct it
	if( !front )
	{
		b->pos_proj[ 0 ] = 640.0 - b->pos_proj[ 0 ],
		b->pos_proj[ 1 ] = 480.0 - b->pos_proj[ 1 ];
	}

	// virtual 640x480 to real
	b->pos_proj[ 0 ] *= cgs.glconfig.vidWidth / 640.0;
	b->pos_proj[ 1 ] *= cgs.glconfig.vidHeight / 480.0;

	// clamp to edges
	if( !front ||
	    b->pos_proj[ 0 ] < cgs.bc.hudRect[0][0] + b->size/2 ||
	    b->pos_proj[ 0 ] > cgs.bc.hudRect[1][0] - b->size/2 ||
	    b->pos_proj[ 1 ] < cgs.bc.hudRect[0][1] + b->size/2 ||
	    b->pos_proj[ 1 ] > cgs.bc.hudRect[1][1] - b->size/2 )
	{
		vec2_t screen[ 2 ];
		Vector2Set( screen[ 0 ], cgs.bc.hudRect[0][0] + b->size/2,
		                         cgs.bc.hudRect[0][1] + b->size/2 );
		Vector2Set( screen[ 1 ], cgs.bc.hudRect[1][0] - b->size/2,
		                         cgs.bc.hudRect[1][1] - b->size/2 );
		Vector2Subtract( b->pos_proj, cgs.bc.hudCenter, b->clamp_dir );
		ProjectPointOntoRectangleOutwards( b->pos_proj, cgs.bc.hudCenter, b->clamp_dir, (const vec2_t*)screen );
		b->clamped = qtrue;
	}
	else
		b->clamped = qfalse;

	// highlight
	if( b == cg.highlightedBeacon )
	{
		if( Distance2( b->pos_proj, cgs.bc.hudCenter ) <= cgs.bc.highlightRadius )
			b->highlighted = qtrue;
		else
			cg.highlightedBeacon = NULL;
	}
	else
		b->highlighted = qfalse;
}

/*
=============
CG_BeaconDynamics
=============
*/

void CG_BeaconDynamics( void )
{
	cbeacon_t *b;

	for( b = cg.beacons; b < cg.beacons + cg.num_beacons; b++ )
		Vector2Copy( b->pos_proj, b->s->pos );
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
			bp->etime = cg.time + cgs.bc.fadeOut;
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
				//bp_health.old = qfalse;
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

		VectorCopy( cent->lerpOrigin, beacon->s->origin );
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

	if( !cg.num_beacons )
		return;

	qsort( cg.beacons, cg.num_beacons, sizeof( cbeacon_t ), CG_CompareBeaconsByDot );

	cg.highlightedBeacon = cg.beacons + cg.num_beacons - 1;

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
qhandle_t CG_BeaconIcon( const cbeacon_t *b, qboolean hud )
{
	if ( b->type <= BCT_NONE || b->type >= NUM_BEACON_TYPES )
		return 0;

	if ( b->type == BCT_TAG )
	{
		if ( b == cg.highlightedBeacon || !hud )
		{
			if ( b->flags & EF_BC_TAG_PLAYER )
			{
				if ( ( b->team == TEAM_ALIENS ) == !( b->flags & EF_BC_ENEMY ) )
				{
					if( b->data <= PCL_NONE || b->data >= PCL_NUM_CLASSES )
						return 0;
					return cg_classes[ b->data ].classIcon;
				}
				else
				{
					if( b->data <= WP_NONE || b->data >= WP_NUM_WEAPONS )
						return 0;
					return cg_weapons[ b->data ].weaponIcon;
				}
			}
			else
			{
				if( b->data <= BA_NONE || b->data >= BA_NUM_BUILDABLES )
					return 0;
				return cg_buildables[ b->data ].buildableIcon;
			}
		}
		else
		{
			if ( b->flags & EF_BC_TAG_PLAYER )
				return BG_Beacon( b->type )->icon[ 1 ];
			else
				return BG_Beacon( b->type )->icon[ 0 ];
		}
	}
	else if ( b->type == BCT_BASE )
	{
		int index = 0;

		if( b->flags & EF_BC_BASE_OUTPOST )
			index |= 1;

		if( b->flags & EF_BC_ENEMY )
			index |= 2;

		return BG_Beacon( b->type )->icon[ index ];
	}

	if ( b->s->altIcon )
		return BG_Beacon( b->type )->icon[ 1 ];
	else
		return BG_Beacon( b->type )->icon[ 0 ];
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
	const char *text;

	if ( b->type <= BCT_NONE || b->type >= NUM_BEACON_TYPES )
		return "";

	if ( b->type == BCT_TAG )
	{
		if ( b->flags & EF_BC_TAG_PLAYER )
		{
			if ( ( b->team == TEAM_ALIENS ) == !( b->flags & EF_BC_ENEMY ) )
			{
				if( b->data <= PCL_NONE || b->data >= PCL_NUM_CLASSES )
					return 0;
				text = _( BG_ClassModelConfig( b->data )->humanName );
			}
			else
			{
				if( b->data <= WP_NONE || b->data >= WP_NUM_WEAPONS )
					return 0;
				text = _( BG_Weapon( b->data )->humanName );
			}
		}
		else
		{
			if( b->data <= BA_NONE || b->data >= BA_NUM_BUILDABLES )
				return 0;
			text = _( BG_Buildable( b->data )->humanName );
		}
	}
	else if ( b->type == BCT_BASE )
	{
		int index = 0;

		if( b->flags & EF_BC_BASE_OUTPOST )
			index |= 1;

		if( b->flags & EF_BC_ENEMY )
			index |= 2;

		text = BG_Beacon( b->type )->text[ index ];
	}
	else if ( b->type == BCT_TIMER )
	{
		float delta;
		delta = 0.001f * ( cg.time - b->s->ctime - BEACON_TIMER_TIME );
		text = va( "T %c %.2fs", ( delta >= 0 ? '+' : '-' ), fabs( delta ) );
	}
	else
		text = _( BG_Beacon( b->type )->text[ 0 ] );

	if( text )
		return text;
	return "(null)";
}
