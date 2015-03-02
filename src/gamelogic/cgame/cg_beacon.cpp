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
		READ_FLOAT  ( fadeInAlpha )
		READ_FLOAT  ( fadeInScale )

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
		else
			Com_Printf( "^3WARNING: bad keyword \"%s\" in \"%s\"\n", token.string, path );
	}

	bc->fadeMinDist = Square( bc->hudSize / bc->hudMaxSize );
	bc->fadeMaxDist = Square( bc->hudSize / bc->hudMinSize );

	trap_Parse_FreeSource( fd  );
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
	const beaconAttributes_t *ba = BG_Beacon( b->type );

	// reset animations
	b->scale = 1.0;
	alpha = 1.0;

	time_in = cg.time - b->ctime; // time since creation
	time_left = b->etime - cg.time; // time to expiration

	// check creation
	if( !b->old )
	{
		if( time_in > 1000 ) //TODO: take time since entering the game into account
			b->old = qtrue;
		else
		{
			if( ba->inSound && ( b->type != BCT_TAG || ( b->flags & EF_BC_ENEMY ) ) )
				trap_S_StartLocalSound( ba->inSound, CHAN_LOCAL_SOUND );
		}
	}

	// check death
	if( b->old &&
	    !( b->oldFlags & EF_BC_DYING ) &&
	    ( b->flags & EF_BC_DYING ) )
	{
		if( ba->outSound )
			trap_S_StartLocalSound( ba->outSound, CHAN_LOCAL_SOUND );
	}

	// fade in
	if ( time_in >= cgs.bc.fadeIn )
		t_fadein = 1.0;
	else
		t_fadein = (float)time_in / cgs.bc.fadeIn;

	b->scale *= LinearRemap( t_fadein, 0.0f, 1.0f, cgs.bc.fadeInScale, 1.0f );
	alpha *= LinearRemap( t_fadein, 0.0f, 1.0f, cgs.bc.fadeInAlpha, 1.0f );

	// fade out
	if ( b->etime && time_left < cgs.bc.fadeOut )
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

			if( !b->eventFired )
			{
				trap_S_StartLocalSound( cgs.media.timerBeaconExpiredSound, CHAN_LOCAL_SOUND );
				b->eventFired = qtrue;
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

		CG_Trace( &tr, cg.refdef.vieworg, NULL, NULL, b->origin, ENTITYNUM_NONE, CONTENTS_SOLID, 0 );

		target = ( ( tr.fraction > 1.0f - FLT_EPSILON ) ? 1.0 : 0.0 );
		CG_ExponentialFade( &b->t_occlusion, target, 10 );
		alpha *= LinearRemap( b->t_occlusion, 0, 1, 1, 0.5 );
	}

	// Fade out when too close. Span the same distance as the size change but fade linearly.
	// Do not fade important beacons.
	if( !( ba->flags & BCF_IMPORTANT ) )
	{
		if( b->dist < cgs.bc.fadeMinDist )
			alpha *= cgs.bc.fadeMinAlpha;
		else if( b->dist > cgs.bc.fadeMaxDist )
			alpha *= cgs.bc.fadeMaxAlpha;
		else
			alpha *= LinearRemap( b->dist, cgs.bc.fadeMinDist, cgs.bc.fadeMaxDist, cgs.bc.fadeMinAlpha, cgs.bc.fadeMaxAlpha );
	}

	// color
	if( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_NONE ||
	    b->type == BCT_TAG || b->type == BCT_BASE )
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

	if( b->color[ 3 ] > 1.0f )
		b->color[ 3 ] = 1.0f;

	// calculate HUD size
	b->size = cgs.bc.hudSize / sqrt( b->dist );
	if( b->size > cgs.bc.hudMaxSize )
		b->size = cgs.bc.hudMaxSize;
	else if ( b->size < cgs.bc.hudMinSize )
		b->size = cgs.bc.hudMinSize;
	b->size *= b->scale;

	// project onto screen
	front = CG_WorldToScreen( b->origin, b->pos, b->pos + 1);

	// CG_WorldToScreen flips the result if behind so correct it
	if( !front )
	{
		b->pos[ 0 ] = 640.0 - b->pos[ 0 ],
		b->pos[ 1 ] = 480.0 - b->pos[ 1 ];
	}

	// virtual 640x480 to real
	b->pos[ 0 ] *= cgs.glconfig.vidWidth / 640.0;
	b->pos[ 1 ] *= cgs.glconfig.vidHeight / 480.0;

	// clamp to edges
	if( !front ||
	    b->pos[ 0 ] < cgs.bc.hudRect[0][0] + b->size/2 ||
	    b->pos[ 0 ] > cgs.bc.hudRect[1][0] - b->size/2 ||
	    b->pos[ 1 ] < cgs.bc.hudRect[0][1] + b->size/2 ||
	    b->pos[ 1 ] > cgs.bc.hudRect[1][1] - b->size/2 )
	{
		vec2_t screen[ 2 ];
		Vector2Set( screen[ 0 ], cgs.bc.hudRect[0][0] + b->size/2,
		                         cgs.bc.hudRect[0][1] + b->size/2 );
		Vector2Set( screen[ 1 ], cgs.bc.hudRect[1][0] - b->size/2,
		                         cgs.bc.hudRect[1][1] - b->size/2 );
		Vector2Subtract( b->pos, cgs.bc.hudCenter, b->clamp_dir );
		ProjectPointOntoRectangleOutwards( b->pos, cgs.bc.hudCenter, b->clamp_dir, (const vec2_t*)screen );
		b->clamped = qtrue;
	}
	else
		b->clamped = qfalse;

	// highlight
	if( b == cg.highlightedBeacon )
	{
		if( Distance2( b->pos, cgs.bc.hudCenter ) <= cgs.bc.highlightRadius )
			b->highlighted = qtrue;
		else
		{
			cg.highlightedBeacon = NULL;
			b->highlighted = qfalse;
		}
	}
	else
		b->highlighted = qfalse;
}

/*
=============
CG_CompareBeaconsBy*

Used for qsort
=============
*/
static int CG_CompareBeaconsByDot( const void *a, const void *b )
{
	return ( *(const cbeacon_t**)a )->dot > ( *(const cbeacon_t**)b )->dot;
}

static int CG_CompareBeaconsByDist( const void *a, const void *b )
{
	return ( *(const cbeacon_t**)a )->dist > ( *(const cbeacon_t**)b )->dist;
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
	entityState_t *es;
	cbeacon_t *b;
	vec3_t delta;

	for( cg.beaconCount = 0, i = 0; i < cg.snap->entities.size(); i++ )
	{
		cent = cg_entities + cg.snap->entities[ i ].number;
		es = &cent->currentState;
		b = &cent->beacon;

		if( es->eType == ET_BEACON )
		{
			if( es->modelindex <= BCT_NONE ||
					es->modelindex >= NUM_BEACON_TYPES )
				continue;

			if( es->time2 && es->time2 <= cg.time ) //expired
				continue;

			b->inuse = qtrue;
			b->ctime = es->time;
			b->etime = es->time2;

			VectorCopy( cent->lerpOrigin, b->origin );
			b->type = (beaconType_t)es->modelindex;
			b->data = es->modelindex2;
			b->team = (team_t)es->generic1;
			b->owner = es->otherEntityNum;
			b->flags = es->eFlags;
		}
		else
			continue; //TODO: alien radar

		VectorSubtract( b->origin, cg.refdef.vieworg, delta );
		VectorNormalize( delta );
		b->dot = DotProduct( delta, cg.refdef.viewaxis[ 0 ] );
		b->dist = Distance( cg.predictedPlayerState.origin, b->origin );

		cg.beacons[ cg.beaconCount ] = b;
		if( ++cg.beaconCount >= MAX_CBEACONS )
			break;
	}

	if( !cg.beaconCount )
		return;

	{
		const playerState_t *ps = &cg.predictedPlayerState;
		int tofind, team = ps->persistant[ PERS_TEAM ];
		qboolean lowammo, energy;

		lowammo = BG_PlayerLowAmmo( ps, &energy );

		qsort( cg.beacons, cg.beaconCount, sizeof( cbeacon_t* ), CG_CompareBeaconsByDist );

		for( i = 0, tofind = 3; i < cg.beaconCount && tofind; i++ )
		{
			b = cg.beacons[ i ];

			if( b->type != BCT_TAG )
				continue;

			if( b->flags & EF_BC_DYING )
				continue;

			if( tofind & 1 )
				if( ( team == TEAM_ALIENS && b->data == BA_A_BOOSTER ) ||
						( team == TEAM_HUMANS && b->data == BA_H_MEDISTAT ) )
				{
					if( ps->stats[ STAT_HEALTH ] < ps->stats[ STAT_MAX_HEALTH ] / 2 )
						b->type = BCT_HEALTH;
					tofind &= ~1;
				}

			if( tofind & 2 )
				if( team == TEAM_HUMANS &&
				    ( b->data == BA_H_ARMOURY ||
				      ( energy &&
				        ( b->data == BA_H_REPEATER || b->data == BA_H_REACTOR ) ) ) )
				{
					if( lowammo )
						b->type = BCT_AMMO;
					tofind &= ~2;
				}
		}
	}

	qsort( cg.beacons, cg.beaconCount, sizeof( cbeacon_t* ), CG_CompareBeaconsByDot );

	cg.highlightedBeacon = cg.beacons[ cg.beaconCount - 1 ];

	for( i = 0; i < cg.beaconCount; i++ )
		CG_RunBeacon( cg.beacons[ i ] );

	for( i = 0; i < cg.beaconCount; i++ )
	{
		cg.beacons[ i ]->old = qtrue;
		cg.beacons[ i ]->oldFlags = cg.beacons[ i ]->flags;
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
		if ( b->highlighted || !hud )
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
		delta = 0.001f * ( cg.time - b->ctime - BEACON_TIMER_TIME );
		text = va( "T %c %.2fs", ( delta >= 0 ? '+' : '-' ), fabs( delta ) );
	}
	else
		text = _( BG_Beacon( b->type )->text[ 0 ] );

	if( text )
		return text;
	return "(null)";
}
