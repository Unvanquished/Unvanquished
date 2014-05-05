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

// g_beacon.c
// handle the server-side beacon-related stuff

#include "g_local.h"

// beacon entityState fields:
// eType          : ET_BEACON
// otherEntityNum : creator
// modelindex     : type
// modelindex2    : additional data (e.g. buildable type for BCT_TAG)
// generic1       : team
// time           : creation time
// time2          : expiration time (-1 if never)

/*
=============
G_GetBeacon

Finds a beacon of the specified type belonging to the specified team.
Creates a new one if none exists.
=============
*/
gentity_t *G_GetBeacon( int owner, team_t team, beaconType_t type )
{
	int i, decayTime;
	gentity_t *ent;

	if( !BG_Beacon( type )->unlimited )
		for ( i = MAX_CLIENTS - 1; i < level.num_entities; i++ )
		{
			ent = g_entities + i;

			if ( ent->s.eType != ET_BEACON )
				continue;

			if ( ent->s.generic1 != team )
				continue;

			if ( ent->s.modelindex != type )
				continue;

			if ( ent->s.modelindex != type )
				continue;

			if ( BG_Beacon( type )->playerUnique &&
			     ent->s.otherEntityNum != owner )
				continue;

			G_FreeEntity( ent );
			break;
		}
		
	ent = G_NewEntity( );
	ent->s.eType = ET_BEACON;

	ent->s.modelindex = type;
	ent->s.generic1 = team;
	ent->s.otherEntityNum = owner;
	ent->s.time = level.time;	
	decayTime = BG_Beacon( type )->decayTime;	
	ent->s.time2 = ( decayTime ? level.time + decayTime : 0 );

	ent->think = G_BeaconThink;
	ent->nextthink = level.time;

	return ent;
}

/*
=================
G_MoveTowardsRoom

Move a point towards the room center (AO-like algorithm)
=================
*/
#define G_AO_SAMPLES 100
void G_MoveTowardsRoom( vec3_t origin, const vec3_t normal )
{
	int i, j;
	vec3_t accumulator, rnd, end;
	trace_t tr;

	for ( i = 0; i < G_AO_SAMPLES; i++ )
	{
		for ( j = 0; j < 3; j++ )
			rnd[ j ] = crandom();
		
		if ( DotProduct( rnd, normal ) < 0 )
			for ( j = 0; j < 3; j++ )
				rnd[ j ] = - rnd[ j ];
		
		VectorMA( origin, 500, rnd, end );
		trap_Trace( &tr, origin, NULL, NULL, end, 0, MASK_SOLID );
		VectorAdd( accumulator, tr.endpos, accumulator );
	}
	
	VectorScale( accumulator, 1.0 / G_AO_SAMPLES, accumulator );
	VectorCopy( accumulator, origin );
}

/*
=================
G_PositionBeacon

Positions the beacon depending on its type and player's position
=================
*/
qboolean G_PositionBeacon( gentity_t *ent, gentity_t *beacon )
{
	int i;
	vec3_t origin, end, forward;
	trace_t tr;
	
	BG_GetClientViewOrigin( &ent->client->ps, origin );
	AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );
	VectorMA( origin, 65536, forward, end );
	trap_Trace( &tr, origin, NULL, NULL, end, ent->s.number, MASK_PLAYERSOLID );

	//place exactly at crosshair
	if ( BG_Beacon( beacon->s.modelindex )->exact )
	{
		VectorCopy( tr.endpos, beacon->s.origin );
	}
	//place in the middle of pointed entity
	//also removes all identical beacons that are there
	else if ( BG_Beacon( beacon->s.modelindex )->entity )
	{
		gentity_t *other;
		vec3_t mins, maxs, center;

		if ( tr.entityNum == ENTITYNUM_NONE ||
		     tr.entityNum == ENTITYNUM_WORLD )
			return qfalse;

		other = g_entities + tr.entityNum;

		if( other->s.eType != ET_BUILDABLE )
			return qfalse;

		beacon->s.modelindex2 = other->s.modelindex;
		BG_BuildableBoundingBox( other->s.modelindex, mins, maxs );

		for ( i = 0; i < 3; i++ )
			center[ i ] = other->s.origin[ i ] + ( mins[ i ] + maxs[ i ] ) / 2.0;

		VectorCopy( center, beacon->s.origin );

		for ( i = 0; i < 3; i++ )
			mins[ i ] = center[ i ] - 4.0f,
			maxs[ i ] = center[ i ] + 4.0f;

		for ( i = MAX_CLIENTS - 1; i < level.num_entities; i++ )
		{
			other = g_entities + i;

			if ( other == beacon )
				continue;

			if ( other->s.eType != ET_BEACON )
				continue;

			if ( other->s.modelindex != beacon->s.modelindex )
				continue;

			if ( other->s.generic1 != beacon->s.generic1 )
				continue;

			if ( other->s.modelindex2 != beacon->s.modelindex2 )
				continue;

			if ( !PointInBounds( other->s.origin, mins, maxs ) )
				continue;

			G_FreeEntity( other );
		}
	}
	// place near the room center
	else
	{
		VectorCopy( tr.endpos, beacon->s.origin );
		G_MoveTowardsRoom( beacon->s.origin, tr.plane.normal );
	}

	return qtrue;
}

/*
=============
G_PropagateBeacon

Sets the server entity fields so that the beacon is visible only by
its team (and spectators).
=============
*/
void G_PropagateBeacon( gentity_t *ent )
{
	int       i;
	gclient_t *client;

	ent->r.svFlags = SVF_BROADCAST | SVF_CLIENTMASK;
	ent->r.loMask = 0;
	ent->r.hiMask = 0;

	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		client = g_entities[ i ].client;

		if ( !client )
			continue;

		if ( client->pers.connected == CON_DISCONNECTED )
			continue;

		if ( client->pers.team != ent->s.generic1 && 
		     client->pers.team != TEAM_NONE )
			continue;

		if ( i < 32 )
			ent->r.loMask |= BIT( i );
		else
			ent->r.hiMask |= BIT( i );
	}

	trap_LinkEntity( ent );
}

/*
=============
G_PropagateAllBeacons

Runs G_PropagateBeacon for all beacons in the world
Should be called everytime someone joins or leaves a team
=============
*/
void G_PropagateAllBeacons( void )
{
	int i;
	gentity_t *ent;
	
	for ( i = MAX_CLIENTS - 1; i < level.num_entities; i++ )
	{
		ent = g_entities + i;

		if ( ent->s.eType != ET_BEACON )
			continue;

		G_PropagateBeacon( ent );
	}
}

/*
=============
G_BeaconThink

(Thinking function for beacons)
Watches the expiration time and deletes itself when no longer needed
=============
*/
#define BEACON_THINKRATE 1000
void G_BeaconThink( gentity_t *ent )
{
	ent->nextthink = level.time + BEACON_THINKRATE;

	if ( ent->s.time2 && level.time > ent->s.time2 )
		G_FreeEntity( ent );
}

/*
=============
G_RemoveOwnedBeacons

Destroy all per-player beacons that belong to a player.
Per-team beacons get their ownership cleared.
=============
*/
void G_RemoveOwnedBeacons( int clientNum )
{
	int i;
	gentity_t *ent;

	for ( i = MAX_CLIENTS - 1; i < level.num_entities; i++ )
	{
		ent = g_entities + i;

		if ( ent->s.eType != ET_BEACON )
			continue;

		if ( ent->s.otherEntityNum != clientNum )
			continue;

		if ( BG_Beacon( ent->s.modelindex )->playerUnique )
			G_FreeEntity( ent );
		else
			ent->s.otherEntityNum = ENTITYNUM_NONE;
	}
}
