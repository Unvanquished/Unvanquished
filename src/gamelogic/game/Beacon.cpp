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

		if( !( ent->s.eFlags & EF_BC_NO_TARGET ) &&
		    ( BG_Beacon( ent->s.modelindex )->flags & BCF_BASE ) )
			if( !FindBase( (beaconType_t)ent->s.modelindex, (team_t)ent->s.generic1, ent->s.origin ) )
			{
				ent->nextthink = ent->s.time2 = level.time + 1500;
				ent->s.eFlags |= EF_BC_NO_TARGET;
			}

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


	////// Beacon::NewArea
	// Create and set up an area beacon (i.e. "Defend")
	void NewArea( beaconType_t type, vec3_t point, team_t team )
	{
		vec3_t origin;
		gentity_t *beacon;

		VectorCopy( point, origin );
		MoveTowardsRoom( origin, NULL );
		RemoveSimilar( origin, type, 0, team, ENTITYNUM_NONE );
		beacon = Beacon::New( origin, type, 0, team, ENTITYNUM_NONE );
		Beacon::Propagate( beacon );
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
	// Normal is optional
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

			if( normal )
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
		int i;
		gentity_t *ent;
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
				if ( BG_Beacon( type )->flags & BCF_ENTITY )
					radius = 5.0;
				else if ( BG_Beacon( type )->flags & BCF_BASE )
				{
					switch( type )
					{
						case BCT_BASE:
						case BCT_BASE_ENEMY:
							radius = BEACON_BASE_RANGE;
							break;

						case BCT_OUTPOST:
						case BCT_OUTPOST_ENEMY:
							radius = BEACON_OUTPOST_RANGE;
							break;

						default:
							radius = 400.0;
					}
				}
				else
					radius = 250.0;

				if ( Distance( ent->s.origin, origin ) > radius )
					continue;

				if ( !trap_InPVS( ent->s.origin, origin ) )
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
	// Update tags attached to an entity
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
	// Sets the "no target" flag and makes the tag expire soon.
	static void DetachTag( gentity_t *ent )
	{
		if( !ent )
			return;

		ent->s.eFlags |= EF_BC_NO_TARGET;
		ent->s.time2 = level.time + 1500;
	}

	////// Beacon::DetachTags
	// Calls DetachTag for all tags attached to an entity.
	void DetachTags( gentity_t *ent )
	{
		DetachTag( ent->alienTag );
		ent->alienTag = NULL;
		DetachTag( ent->humanTag );
		ent->humanTag = NULL;
	}

	////// Beacon::DeleteTags
	// Immediately deletes all tags attached to an entity (skips all effects).
	void DeleteTags( gentity_t *ent )
	{
		Delete( ent->alienTag );
		ent->alienTag = NULL;
		Delete( ent->humanTag );
		ent->humanTag = NULL;
	}

	////// Beacon::Tag
	// Tag an entity
	void Tag( gentity_t *ent, team_t team, int owner, qboolean permanent )
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
				owner = ent->s.number;
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
			beacon->s.time2 = level.time + 35000;

		if( permanent )
			beacon->s.time2 = 0;

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

	////// Beacon::FindBase
	// Look for a base for a base beacon
	qboolean FindBase( beaconType_t type, team_t ownerTeam, vec3_t origin )
	{
		qboolean enemy, outpost;
		team_t team;
		float radius;
		int i, count, list[ MAX_GENTITIES ];
		vec3_t mins, maxs;
		gentity_t *ent;

		enemy = ( type == BCT_BASE_ENEMY ||
		          type == BCT_OUTPOST_ENEMY );
		outpost = ( type == BCT_OUTPOST ||
		            type == BCT_OUTPOST_ENEMY );
		team = ( ( ownerTeam == TEAM_ALIENS ) ^ enemy ? TEAM_ALIENS : TEAM_HUMANS );
		radius = ( outpost ? BEACON_OUTPOST_RANGE : BEACON_BASE_RANGE );

		for( i = 0; i < 3; i++ )
			mins[ i ] = origin[ i ] - radius,
			maxs[ i ] = origin[ i ] + radius;

		count = trap_EntitiesInBox( mins, maxs, list, MAX_GENTITIES );

		for( i = 0; i < count; i++ )
		{
			ent = g_entities + list[ i ];

			if( ent->s.eType != ET_BUILDABLE )
				continue;

			if( !( ent->s.eFlags & EF_B_POWERED ) )
				continue;

			if( ent->health <= 0 )
				continue;

			if( (team_t)ent->buildableTeam != team )
				continue;

			return qtrue;
		}

		return qfalse;
	}
}
