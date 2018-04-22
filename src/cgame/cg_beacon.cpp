/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2014 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

// cg_beacon.c
// beacon code shared by HUD and minimap

#include "cg_local.h"

// keep in sync with ../game/Beacon.cpp
#define bc_target otherEntityNum
#define bc_owner otherEntityNum2
#define bc_type modelindex
#define bc_data modelindex2
#define bc_team generic1
#define bc_ctime time
#define bc_etime time2
#define bc_mtime apos.trTime

void CG_LoadBeaconsConfig()
{
	beaconsConfig_t  *bc = &cgs.bc;
	const char       *path = "ui/beacons.cfg";
	int              vw, vh, base, fd;
	pc_token_t       token;

	vw = cgs.glconfig.vidWidth;
	vh = cgs.glconfig.vidHeight;
	base = std::min( vw, vh );

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
		READ_FLOAT  ( hudAlphaImportant )

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
		else if( !Q_stricmp( token.string, "highlightAngle" ) )
		{
			float angle;

			if( !PC_Float_Parse( fd, &angle ) )
				break;

			bc->highlightAngle = cos( angle / 180.0 * M_PI );
		}
		else
			Log::Warn( "bad keyword \"%s\" in \"%s\"\n", token.string, path );
	}

	bc->fadeMinDist = Square( bc->hudSize / bc->hudMaxSize );
	bc->fadeMaxDist = Square( bc->hudSize / bc->hudMinSize );

	trap_Parse_FreeSource( fd  );
}

#define Distance2(a,b) sqrt(Square((a)[0]-(b)[0])+Square((a)[1]-(b)[1]))

/**
 * @brief Fills cg.beacons with explicit (ET_BEACON entity) beacons.
 * @return Whether all beacons found space in cg.beacons.
 */
static bool LoadExplicitBeacons()
{
	unsigned i;
	centity_t     *ent;
	entityState_t *es;
	cbeacon_t     *beacon;

	cg.highlightedBeacon = nullptr;

	// Find beacons and add them to cg.beacons.
	for( cg.beaconCount = 0, i = 0; i < cg.snap->entities.size(); i++ )
	{
		ent    = cg_entities + cg.snap->entities[ i ].number;
		es     = &ent->currentState;
		beacon = &ent->beacon;

		if ( es->eType != entityType_t::ET_BEACON )
			continue;

		if( es->modelindex <= BCT_NONE || es->modelindex >= NUM_BEACON_TYPES )
			continue;

		if( es->bc_etime && es->bc_etime <= cg.time ) // Beacon expired.
			continue;

		beacon->ctime = es->bc_ctime;
		beacon->etime = es->bc_etime;
		beacon->mtime = es->bc_mtime;
		beacon->type = (beaconType_t)es->bc_type;
		beacon->data = es->bc_data;
		beacon->ownerTeam = (team_t)es->bc_team;
		beacon->owner = es->bc_owner;
		beacon->flags = es->eFlags;
		beacon->target = es->bc_target;
		beacon->alphaMod = 1.0f;

		// Snap beacon origin to exact player location if known.
		centity_t *targetCent; entityState_t *targetES;
		if ( beacon->target && ( targetCent = &cg_entities[ beacon->target ] )->valid &&
		     ( targetES = &targetCent->currentState )->eType == entityType_t::ET_PLAYER )
		{
			vec3_t mins, maxs, center;
			int pClass = ( ( targetES->misc >> 8 ) & 0xFF ); // TODO: Write function for this.

			VectorCopy( targetCent->lerpOrigin, center );
			BG_ClassBoundingBox( pClass, mins, maxs, nullptr, nullptr, nullptr );
			BG_MoveOriginToBBOXCenter( center, mins, maxs );

			// TODO: Interpolate when target entity pops in.
			VectorCopy( center, beacon->origin );
		}
		else
		{
			// TODO: Interpolate when target entity pops out.
			VectorCopy( ent->lerpOrigin, beacon->origin );
		}

		cg.beacons[ cg.beaconCount++ ] = beacon;
		if( cg.beaconCount >= MAX_CBEACONS ) return false;
	}

	return true;
}

/**
 * @brief Fills cg.beacons with implicit (client side generated) beacons.
 * @return Whether all beacons found space in cg.beacons.
 */
static bool LoadImplicitBeacons()
{
	// All alive aliens have enemy sense.
	if ( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_ALIENS &&
	     cg.predictedPlayerState.persistant[ PERS_SPECSTATE ] == SPECTATOR_NOT &&
	     cg.predictedPlayerState.stats[ STAT_HEALTH ] > 0 )
	{
		for ( int clientNum = 0; clientNum < MAX_CLIENTS; clientNum++ )
		{
			centity_t     *ent;
			entityState_t *es;
			clientInfo_t  *client;

			// Can only track valid targets.
			// TODO: Make beacons expire properly anyway.
			if ( !( ent = &cg_entities[ clientNum ] )->valid )            continue;

			// Only tag enemies like this, teammates have an explicit beacon.
			if ( !( client = &cgs.clientinfo[ clientNum ] )->infoValid ) continue;
			if ( client->team != TEAM_HUMANS )                           continue;

			//ent = &cg_entities[ clientNum ];
			es = &ent->currentState;

			cbeacon_t *beacon = &ent->beacon;

			// Set location on exact center of target player entity.
			{
				vec3_t mins, maxs, center;
				int pClass = ( ( es->misc >> 8 ) & 0xFF ); // TODO: Write function for this.

				VectorCopy( ent->lerpOrigin, center );
				BG_ClassBoundingBox( pClass, mins, maxs, nullptr, nullptr, nullptr );
				BG_MoveOriginToBBOXCenter( center, mins, maxs );

				VectorCopy( center, beacon->origin );
			}

			// Add alpha fade at the borders of the sense range.
			beacon->alphaMod = Math::Clamp(
				( ( (float)ALIENSENSE_RANGE -
			        Distance( cg.predictedPlayerState.origin, beacon->origin ) ) /
			      ( ALIENSENSE_BORDER_FRAC * (float)ALIENSENSE_RANGE ) ), 0.0f, 1.0f );

			// A value of 0 means the target is out of range, don't create a beacon in that case.
			if ( beacon->alphaMod == 0.0f ) continue;

			// Prepare beacon to be added.
			beacon->ctime     = ent->pvsEnterTime;
			beacon->mtime     = cg.time;
			beacon->type      = BCT_TAG;
			beacon->data      = es->weapon;
			beacon->ownerTeam = TEAM_ALIENS;
			beacon->owner     = ENTITYNUM_NONE;
			beacon->flags     = EF_BC_TAG_PLAYER | EF_BC_ENEMY;
			beacon->target    = es->number;

			// Expire beacons on corpses.
			if ( ent->currentState.eFlags & EF_DEAD ) {
				beacon->flags |= EF_BC_DYING;

				if ( !beacon->old || !( beacon->oldFlags & EF_BC_DYING ) ) {
					beacon->etime = cg.time + 1500; // TODO: Sync delay with explicit beacons.
				}

				// Don't add beacon if it's both expired and faded out.
				if ( beacon->etime <= cg.time ) {
					continue;
				}
			} else {
				beacon->etime = 0;
			}

			// Add beacon to cg.beacons.
			cg.beacons[ cg.beaconCount++ ] = beacon;
			if( cg.beaconCount >= MAX_CBEACONS ) return false;
		}
	}

	return true;
}

/**
 * @brief Used by qsort to compare beacons.
 */
static int CompareBeaconsByDist( const void *a, const void *b )
{
	return ( *(const cbeacon_t**)a )->dist > ( *(const cbeacon_t**)b )->dist;
}

/**
 * @brief Marks the nearest refreshing buildable if low on health or ammo.
 * @note  Assumes that cg.beacons is sorted.
 */
static void MarkRelevantBeacons()
{
	const playerState_t *ps = &cg.predictedPlayerState;
	int team = ps->persistant[ PERS_TEAM ];
	bool lowammo, energy;

	lowammo = BG_PlayerLowAmmo( ps, &energy );

	for( int beaconNum = 0, tofind = 3; beaconNum < cg.beaconCount && tofind; beaconNum++ )
	{
		cbeacon_t *beacon = cg.beacons[ beaconNum ];

		// Only tagged buildables are relevant so far.
		if( beacon->type != BCT_TAG ||
		    ( beacon->flags & EF_BC_TAG_PLAYER ) ||
		    ( beacon->flags & EF_BC_DYING ) )
		{
			continue;
		}

		// Find a health source.
		if( tofind & 1 )
		{
			if( ( team == TEAM_ALIENS && beacon->data == BA_A_BOOSTER ) ||
			    ( team == TEAM_HUMANS && beacon->data == BA_H_MEDISTAT ) )
			{
				if( ps->stats[ STAT_HEALTH ] < ps->stats[ STAT_MAX_HEALTH ] / 2 )
					beacon->type = BCT_HEALTH;
				tofind &= ~1;
			}
		}

		// Find an ammo source.
		if( tofind & 2 )
		{
			if( team == TEAM_HUMANS && ( beacon->data == BA_H_ARMOURY ||
			    ( energy && beacon->data == BA_H_REACTOR ) ) )
			{
				if( lowammo )
					beacon->type = BCT_AMMO;
				tofind &= ~2;
			}
		}
	}
}

static void SetHighlightedBeacon()
{
	for( int beaconNum = 0; beaconNum < cg.beaconCount; beaconNum++ ) {
		cbeacon_t *beacon = cg.beacons[ beaconNum ];
		vec3_t delta;

		VectorSubtract( beacon->origin, cg.refdef.vieworg, delta );
		VectorNormalize( delta );
		beacon->dot = DotProduct( delta, cg.refdef.viewaxis[ 0 ] );
		beacon->dist = Distance( cg.predictedPlayerState.origin, beacon->origin );

		// Set highlighted beacon to smallest angle below threshold.
		if( beacon->dot > cgs.bc.highlightAngle &&
			( !cg.highlightedBeacon || beacon->dot > cg.highlightedBeacon->dot ) )
		{
			cg.highlightedBeacon = beacon;
		}
	}
}

static inline bool IsHighlighted( const cbeacon_t *b )
{
	return cg.highlightedBeacon == b;
}

static team_t TargetTeam( const cbeacon_t *beacon )
{
	if ( beacon->type != BCT_TAG && beacon->type != BCT_BASE )
		return TEAM_NONE;

	if ( ( beacon->ownerTeam == TEAM_ALIENS && beacon->flags & EF_BC_ENEMY ) ||
	     ( beacon->ownerTeam == TEAM_HUMANS && !( beacon->flags & EF_BC_ENEMY ) ) )
		return TEAM_HUMANS;
	else
		return TEAM_ALIENS;
}

/**
 * @brief Handles beacon animations and sounds. Called every frame for every beacon.
 */
static void DrawBeacon( cbeacon_t *b )
{
	float alpha; // target
	int time_in, time_left;
	float t_fadein, t_fadeout; // t_ stands for "parameter", not "time"
	bool front;
	const beaconAttributes_t *ba = BG_Beacon( b->type );

	// reset animations
	b->scale = 1.0;
	alpha    = 1.0;

	time_in   = cg.time - b->ctime; // time since creation
	time_left = b->etime - cg.time; // time to expiration

	// check creation
	if( !b->old )
	{
		if( time_in > 1000 ) //TODO: take time since entering the game into account
			b->old = true;
		else
		{
			if( ba->inSound && ( b->type != BCT_TAG || ( b->flags & EF_BC_ENEMY ) ) )
				trap_S_StartLocalSound( ba->inSound, soundChannel_t::CHAN_LOCAL_SOUND );
		}
	}

	// check death
	if( b->old &&
	    !( b->oldFlags & EF_BC_DYING ) &&
	    ( b->flags & EF_BC_DYING ) )
	{
		if( ba->outSound )
			trap_S_StartLocalSound( ba->outSound, soundChannel_t::CHAN_LOCAL_SOUND );
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

	// apply pre-defined alpha modifier
	alpha *= b->alphaMod;

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
				trap_S_StartLocalSound( cgs.media.timerBeaconExpiredSound, soundChannel_t::CHAN_LOCAL_SOUND );
				b->eventFired = true;
			}
		}
		else
			t = 1.0;

		b->scale *= Square( 1.0 - t ) * 2.0 + 1.0;
	}
/*
	// occlusion effects
	if ( b->type == BCT_TAG && ( b->flags & EF_BC_TAG_PLAYER ) )
	{
		trace_t tr;

		CG_Trace( &tr, cg.refdef.vieworg, nullptr, nullptr, b->origin, ENTITYNUM_NONE, CONTENTS_SOLID, 0 );

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
*/
	// color
	if( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_NONE ||
	    b->type == BCT_TAG || b->type == BCT_BASE )
	{
		switch( b->ownerTeam )
		{
			case TEAM_ALIENS:
				if ( b->flags & EF_BC_ENEMY )
					b->color = cgs.bc.colorHuman;
				else
					b->color = cgs.bc.colorAlien;
				break;
			case TEAM_HUMANS:
				if ( b->flags & EF_BC_ENEMY )
					b->color = cgs.bc.colorAlien;
				else
					b->color = cgs.bc.colorHuman;
				break;
			default:
				b->color = cgs.bc.colorNeutral;
				break;
		}
	}
	else
		b->color = cgs.bc.colorNeutral;
	b->color.SetAlpha( b->color.Alpha() * alpha );

	if( b->color.Alpha() > 1.0f )
		b->color.SetAlpha( 1.0f );

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
		b->clamped = true;
	}
	else
		b->clamped = false;
}

/**
 * @brief Hands over information about highlighted beacon to UI.
 */
static void HandHLBeaconToUI()
{
	cbeacon_t *beacon;
	beaconRocket_t * const br = &cg.beaconRocket;
	bool showIcon     = false,
	         showName     = false,
	         showInfo     = false,
	         showDistance = false,
	         showAge      = false,
	         showOwner    = false;

	if( ( beacon = cg.highlightedBeacon ) )
	{
		// icon
		if ( ( br->icon = CG_BeaconDescriptiveIcon( beacon ) ) )
		{
			showIcon = true;
		}

		// name
		CG_BeaconName( beacon, br->name, sizeof( br->name ) );
		showName = true;

		// info
		if ( beacon->type == BCT_TAG &&
		     ( beacon->flags & EF_BC_TAG_PLAYER ) &&
		     TargetTeam( beacon ) == TEAM_HUMANS )
		{
			Com_sprintf( br->info, sizeof( br->info ), "Carrying %s",
			             BG_Weapon( beacon->data )->humanName );
			showInfo = true;
		}

		// distance
		int distance = (int)floor( beacon->dist * QU_TO_METER );
		if ( distance > 1 )
		{
			Com_sprintf( br->distance, sizeof( br->distance ), "%im from here", distance );
		}
		else
		{
			Com_sprintf( br->distance, sizeof( br->distance ), "Here" );
		}
		showDistance = true;

		// age
		if( beacon->type == BCT_TAG &&
		    !( beacon->flags & EF_BC_TAG_PLAYER ) &&
		    ( beacon->flags & EF_BC_ENEMY ) )
		{
			int age = cg.time - beacon->mtime;

			if( age < 1000 )
				Com_sprintf( br->age, sizeof( br->age ), "Spotted just now" );
			else
				Com_sprintf( br->age, sizeof( br->age ), "Spotted %i:%02i ago",
				             age / 60000, ( age / 1000 ) % 60 );

			showAge = true;
		}

		// owner
		if( BG_Beacon( beacon->type )->flags & BCF_IMPORTANT &&
		    beacon->owner >= 0 && beacon->owner < MAX_CLIENTS &&
		    *cgs.clientinfo[ beacon->owner ].name )
		{
				Com_sprintf( br->owner, sizeof( br->owner ), "by ^7%s",
				             cgs.clientinfo[ beacon->owner ].name );
				showOwner = true;
		}
	}

	CG_ExponentialFade( &br->iconAlpha,     showIcon     ? 1 : 0, 10 );
	CG_ExponentialFade( &br->nameAlpha,     showName     ? 1 : 0, 10 );
	CG_ExponentialFade( &br->infoAlpha,     showInfo     ? 1 : 0, 10 );
	CG_ExponentialFade( &br->distanceAlpha, showDistance ? 1 : 0, 10 );
	CG_ExponentialFade( &br->ageAlpha,      showAge      ? 1 : 0, 10 );
	CG_ExponentialFade( &br->ownerAlpha,    showOwner    ? 1 : 0, 10 );
}

/**
 * @brief Loads and runs beacons, passes beacon data to the UI.
 */
void CG_RunBeacons()
{
	// Don't load implicit beacons if there wasn't enough space for the explicit ones.
	LoadExplicitBeacons() && LoadImplicitBeacons();

	// Sort beacons by distance, code below this may assume this.
	qsort( cg.beacons, cg.beaconCount, sizeof( cbeacon_t* ), CompareBeaconsByDist );

	MarkRelevantBeacons();
	SetHighlightedBeacon();

	for( int beaconNum = 0; beaconNum < cg.beaconCount; beaconNum++ ) {
		DrawBeacon( cg.beacons[ beaconNum ] );
	}

	for( int beaconNum = 0; beaconNum < cg.beaconCount; beaconNum++ ) {
		cg.beacons[ beaconNum ]->old = true;
		cg.beacons[ beaconNum ]->oldFlags = cg.beacons[ beaconNum ]->flags;
	}

	HandHLBeaconToUI();
}

qhandle_t CG_BeaconIcon( const cbeacon_t *b )
{
	if ( b->type <= BCT_NONE || b->type >= NUM_BEACON_TYPES ) return 0;

	int hlFlag = IsHighlighted( b ) ? 1 : 0;

	if ( b->type == BCT_TAG ) {
		return BG_Beacon( b->type )->icon[ hlFlag ][ b->flags & EF_BC_TAG_PLAYER ? 1 : 0 ];
	} else if ( b->type == BCT_BASE ) {
		int index = 0;

		if( b->flags & EF_BC_BASE_OUTPOST ) index |= 1;
		if( b->flags & EF_BC_ENEMY )        index |= 2;

		return BG_Beacon( b->type )->icon[ hlFlag ][ index ];
	}

	return BG_Beacon( b->type )->icon[ hlFlag ][ 0 ];
}

qhandle_t CG_BeaconDescriptiveIcon( const cbeacon_t *b )
{
	if ( b->type == BCT_TAG ) {
		if( b->flags & EF_BC_TAG_PLAYER ) {
			switch ( TargetTeam( b ) ) {
				case TEAM_ALIENS:
					return cg_classes[ b->data ].classIcon;
				case TEAM_HUMANS:
					return cg_weapons[ b->data ].weaponIcon;
				default:
					return CG_BeaconIcon( b );
			}
		} else {
			return cg_buildables[ b->data ].buildableIcon;
		}
	} else {
		return CG_BeaconIcon( b );
	}
}

const char *CG_BeaconName( const cbeacon_t *b, char *out, size_t len )
{
	if( b->type <= BCT_NONE || b->type > NUM_BEACON_TYPES ) {
		return strncpy( out, "b->type out of range", len );
		return "b->type out of range";
	}

	team_t ownTeam    = (team_t)cg.predictedPlayerState.persistant[ PERS_TEAM ];
	team_t beaconTeam = TargetTeam( b );

	switch( b->type )
	{
		case BCT_TAG:
			if( b->flags & EF_BC_TAG_PLAYER ) {
				if ( ownTeam == TEAM_NONE || ownTeam == beaconTeam ) {
					return strncpy( out, cgs.clientinfo[ b->target ].name, len ); // Player name
				} else if ( beaconTeam == TEAM_ALIENS ) {
					return strncpy( out, BG_ClassModelConfig( b->data )->humanName, len ); // Class name
				} else if ( beaconTeam == TEAM_HUMANS ) {
					// TODO: Display "Light//Chewy/Canned Food" for different armor types.
					return strncpy( out, "Food", len );
				} else {
					return strncpy( out, "???", len );
				}
			} else {
				// Display buildable name for all teams.
				return strncpy( out, BG_Buildable( b->data )->humanName, len );
			}
			break;

		case BCT_BASE:
		{
			const char *prefix, *suffix;

			if ( ownTeam == TEAM_NONE ) {
				switch ( TargetTeam( b ) ) {
					case TEAM_ALIENS: prefix = "Alien"; break;
					case TEAM_HUMANS: prefix = "Human"; break;
					default:          prefix = "Neutral";
				}
			} else {
				if ( b->flags & EF_BC_ENEMY ) {
					prefix = "Enemy";
				} else {
					prefix = "Friendly";
				}
			}

			if ( b->flags & EF_BC_BASE_OUTPOST ) {
				suffix = "Outpost";
			} else {
				suffix = "Base";
			}

			Q_snprintf( out, len, "%s %s", prefix, suffix );
			return out;
		}

		default:
			// All other beacons have a fixed name.
			return strncpy( out, BG_Beacon( b->type )->text[ 0 ], len );
	}
}
