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

// Beacon.cpp
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

namespace Beacon //this should eventually become a class
{
	////// Beacon::Think
	// Think function for all beacons
	#define BEACON_THINKRATE 1000
	void Think( gentity_t *ent )
	{
		ent->nextthink = level.time + BEACON_THINKRATE;

		if ( ent->s.time2 && level.time > ent->s.time2 )
			Delete( ent );
	}

	////// Beacon::New
	// Create a new ET_BEACON entity
	gentity_t *New( vec3_t origin, beaconType_t type, int data, 
	                team_t team, int owner )
	{
		gentity_t *ent;
		int decayTime;

		ent = G_NewEntity( );
		ent->s.eType = ET_BEACON;

		VectorCopy( origin, ent->s.origin );
		ent->s.modelindex = type;
		ent->s.modelindex2 = data;
		ent->s.generic1 = team;
		ent->s.otherEntityNum = owner;
		ent->think = Think;
		ent->nextthink = level.time;

		ent->s.time = level.time;
		decayTime = BG_Beacon( type )->decayTime;
		ent->s.time2 = ( decayTime ? level.time + decayTime : 0 );

		return ent;
	}

	////// Beacon::Delete
	// Delete a beacon
	void Delete( gentity_t *ent )
	{
		if( !ent )
			return;
		G_FreeEntity( ent );
	}

	////// Beacon::MoveTowardsRoom
	// Move a point towards empty space (away from map geometry)
	// Note: normal is only for making this function converge quicker
	#define BEACONS_MTR_SAMPLES 100
	void MoveTowardsRoom( vec3_t origin, const vec3_t normal )
	{
		int i, j;
		vec3_t accumulator, rnd, end;
		trace_t tr;

		for ( i = 0; i < BEACONS_MTR_SAMPLES; i++ )
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

		VectorScale( accumulator, 1.0 / BEACONS_MTR_SAMPLES, accumulator );
		VectorCopy( accumulator, origin );
	}

	////// Beacon::RemoveSimilar
	// Remove beacons matching a pattern
	void RemoveSimilar( vec3_t origin, beaconType_t type, int data, int team, int owner )
	{
		int i, j;
		gentity_t *ent;
		vec3_t mins, maxs;
		float radius;

		for( i = 0; i < level.num_entities; i++ )
		{
			ent = g_entities + i;

			// conditions common to all beacons

			if ( ent->s.eType != ET_BEACON )
				continue;

			if ( ent->s.modelindex != type )
				continue;

			if( ent->s.generic1 != team )
				continue;

			if ( ( BG_Beacon( type )->flags & BCF_DATA_UNIQUE ) &&
			     ent->s.modelindex2 != data )
				continue;

			// conditions common to specific kinds of beacons

			if( BG_Beacon( type )->flags & BCF_PER_TEAM )
			{
				// no new conditions
			}
			else if( BG_Beacon( type )->flags & BCF_PER_PLAYER )
			{
				if( ent->s.otherEntityNum != owner )
					continue;
			}
			else
			{
				if( BG_Beacon( type )->flags & BCF_ENTITY )
					radius = 5.0;
				else
					radius = 250.0;

				for( j = 0; j < 3; j++ )
					mins[ j ] = origin[ j ] - radius,
					maxs[ j ] = origin[ j ] + radius;

				if ( !PointInBounds( ent->s.origin, mins, maxs ) )
					continue;
			}

			Delete( ent );
		}
	}

	////// Beacon::Propagate
	// Sets the server entity fields so that the beacon is visible only by
  // its team (and spectators).
	void Propagate( gentity_t *ent )
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

	////// Beacon::PropagateAll
	// Runs G_PropagateBeacon for all beacons in the world
	// Should be called everytime someone joins or leaves a team
	void PropagateAll( void )
	{
		int i;
		gentity_t *ent;

		for ( i = MAX_CLIENTS - 1; i < level.num_entities; i++ )
		{
			ent = g_entities + i;

			if ( ent->s.eType != ET_BEACON )
				continue;

			Propagate( ent );
		}
	}

	////// Beacon::RemoveOrphaned
	// Remove all per-player beacons that belong to a player.
	// Per-team beacons get their ownership cleared.
	void RemoveOrphaned( int clientNum )
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

			if ( BG_Beacon( ent->s.modelindex )->flags & BCF_PER_PLAYER )
				Delete( ent );
			else
				ent->s.otherEntityNum = ENTITYNUM_NONE;
		}
	}

	////// Beacon::UpdateTags
	// ...
	void UpdateTags( gentity_t *ent )
	{
		// buildables are supposed to be static
		if( ent->s.eType == ET_BUILDABLE )
			return;

		if( ent->alienTag )
			VectorCopy( ent->s.origin, ent->alienTag->s.origin );
		if( ent->humanTag )
			VectorCopy( ent->s.origin, ent->humanTag->s.origin );
	}

	////// Beacon::DetachTag
	// ...
	static void DetachTag( gentity_t *ent )
	{
		if( !ent )
			return;

		ent->s.eFlags |= EF_BC_TAG_DETACHED;
		ent->s.time2 = level.time + 1500;
	}

	////// Beacon::DetachTags
	// ...
	void DetachTags( gentity_t *ent )
	{
		DetachTag( ent->alienTag );
		ent->alienTag = NULL;
		DetachTag( ent->humanTag );
		ent->humanTag = NULL;
	}

	////// Beacon::DeleteTags
	void DeleteTags( gentity_t *ent )
	{
		Delete( ent->alienTag );
		ent->alienTag = NULL;
		Delete( ent->humanTag );
		ent->humanTag = NULL;
	}

	////// Beacon::Tag
	// Tag an entity
	void Tag( gentity_t *ent, team_t team, int owner )
	{
		int i, data;
		vec3_t origin, mins, maxs;
		qboolean dead, alien = qfalse, human = qfalse;
		gentity_t *beacon, **attachment;

		switch( ent->s.eType )
		{
			case ET_BUILDABLE:
				BG_BuildableBoundingBox( ent->s.modelindex, mins, maxs );
				data = ent->s.modelindex;
				dead = ( ent->health <= 0 );
				break;

			case ET_PLAYER:
				BG_ClassBoundingBox( ent->client->pers.classSelection, mins, maxs, NULL, NULL, NULL );
				dead = ( ent->client && ent->client->ps.stats[ STAT_HEALTH ] <= 0 );
				switch( ent->client->pers.team )
				{
					case TEAM_ALIENS:
						alien = qtrue;
						data = ent->client->ps.stats[ STAT_CLASS ];
						break;

					case TEAM_HUMANS:
						human = qtrue;
						data = BG_GetPlayerWeapon( &ent->client->ps );
						break;

					default:
						return;
				}
				break;

			default:
				return;
		}

		for( i = 0; i < 3; i++ )
			origin[ i ] = ent->s.origin[ i ] + ( mins[ i ] + maxs[ i ] ) / 2.0;

		switch( team )
		{
			case TEAM_ALIENS:
				attachment = &ent->alienTag;
				break;

			case TEAM_HUMANS:
				attachment = &ent->humanTag;
				break;

			default:
				return;
		}

		RemoveSimilar( origin, BCT_TAG, data, team, owner );
		beacon = New( origin, BCT_TAG, data, team, owner );

		if( alien )
		{
			beacon->s.eFlags |= EF_BC_TAG_ALIEN;
			beacon->s.time2 = level.time + 4000;
		}
		else if( human )
		{
			beacon->s.eFlags |= EF_BC_TAG_HUMAN;
			beacon->s.time2 = level.time + 4000;
		}
		else
			beacon->s.time2 = level.time + 15000;

		if( dead )
			DetachTag( beacon );
		else
		{
			//if( *attachment )
			//	Delete( *attachment );
			*attachment = beacon;
		}

		Propagate( beacon );
	}
}
