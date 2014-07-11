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
	#define BEACON_THINKRATE 1000

	/**
	 * @brief Think function for all beacons.
	 */
	void Think( gentity_t *ent )
	{
		ent->nextthink = level.time + BEACON_THINKRATE;

		if ( ent->s.time2 && level.time > ent->s.time2 )
			Delete( ent );
	}

	/**
	 * @brief Create a new ET_BEACON entity.
	 * @return A pointer to the new entity.
	 */
	gentity_t *New( const vec3_t origin, beaconType_t type, int data,
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

	/**
	 * @brief Create and set up an area beacon (i.e. "Defend").
	 */
	void NewArea( beaconType_t type, vec3_t point, team_t team )
	{
		vec3_t origin;
		gentity_t *beacon;

		VectorCopy( point, origin );
		MoveTowardsRoom( origin, NULL );
		RemoveSimilar( origin, type, 0, team, ENTITYNUM_NONE, 0, 0 );
		beacon = Beacon::New( origin, type, 0, team, ENTITYNUM_NONE );
		Beacon::Propagate( beacon );
	}

	/**
	 * @brief Delete a beacon instantly, without playing a sound or effect.
	 */
	void Delete( gentity_t *ent )
	{
		if( !ent )
			return;
		G_FreeEntity( ent );
	}

	#define BEACONS_MTR_SAMPLES 100

	/**
	 * @brief Move a point towards empty space (away from map geometry).
	 * @param normal Optional direction to move towards.
	 */
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

	/**
	 * @brief Find beacon matching a pattern.
	 * @return An ET_BEACON entity or NULL.
	 */
	gentity_t *FindSimilar( const vec3_t origin, beaconType_t type, int data, int team, int owner,
	                        float radius, int eFlags )
	{
		for ( gentity_t *ent = NULL; (ent = G_IterateEntities(ent, NULL, true, 0, NULL)); )
		{
			// conditions common to all beacons

			if ( ent->s.eType != ET_BEACON )
				continue;

			if ( ent->s.modelindex != type )
				continue;

			if ( ( ent->s.eFlags & eFlags ) != eFlags )
				continue;

			if ( ( ent->s.eFlags | eFlags ) != ent->s.eFlags )
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
				if ( radius <= 0 )
					radius = ( BG_Beacon( type )->flags & BCF_ENTITY ) ? 5.0f : 250.0f;

				if ( Distance( ent->s.origin, origin ) > radius )
					continue;

				if ( !trap_InPVS( ent->s.origin, origin ) )
					continue;
			}

			return ent;
		}

		return NULL;
	}

	/**
	 * @brief Remove beacon matching a pattern.
	 */
	void RemoveSimilar( const vec3_t origin, beaconType_t type, int data, int team, int owner,
	                    float radius, int eFlags )
	{
		gentity_t *ent = FindSimilar(origin, type, data, team, owner, radius, eFlags );
		if ( ent ) Delete( ent );
	}

	/**
	 * @brief Sets the entity fields so that the beacon is visible only by its team and spectators.
	 */
	void Propagate( gentity_t *ent )
	{
		ent->r.svFlags = SVF_BROADCAST | SVF_CLIENTMASK;

		G_TeamToClientmask( (team_t)ent->s.generic1, &ent->r.loMask, &ent->r.hiMask );

		if ( BG_Beacon( ent->s.modelindex )->flags & BCF_SPECTATOR )
		{
			int loMask, hiMask;
			G_TeamToClientmask( TEAM_NONE, &loMask, &hiMask );
			ent->r.loMask |= loMask;
			ent->r.hiMask |= hiMask;
		}

		// Don't send a tag to the tagged client itself.
		if ( ent->s.modelindex == BCT_TAG && (ent->s.eFlags & (EF_BC_TAG_ALIEN|EF_BC_TAG_HUMAN)) )
		{
			int loMask, hiMask;
			G_ClientnumToMask( ent->s.otherEntityNum, &loMask, &hiMask );
			ent->r.loMask &= ~loMask;
			ent->r.hiMask &= ~hiMask;
		}

		trap_LinkEntity( ent );
	}

	/**
	 * @brief Propagates all beacons in the world.
	 *
	 * Should be called everytime someone joins or leaves a team.
	 */
	void PropagateAll( void )
	{
		for ( gentity_t *ent = NULL; (ent = G_IterateEntities( ent )); )
		{
			if ( ent->s.eType != ET_BEACON )
				continue;

			Propagate( ent );
		}
	}

	/**
	 * @brief Remove all per-player beacons that belong to a player.
	 *
	 * Per-team beacons get their ownership cleared.
	 */
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

	/**
	 * @brief Update tags attached to an entity.
	 */
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

	/**
	 * @brief Sets the "no target" flag and makes the tag expire soon.
	 */
	static void DetachTag( gentity_t *ent )
	{
		if( !ent )
			return;

		ent->s.eFlags |= EF_BC_NO_TARGET;
		ent->s.time2 = level.time + 1500;
	}

	/**
	 * @brief Calls DetachTag for all tags attached to an entity.
	 */
	void DetachTags( gentity_t *ent )
	{
		DetachTag( ent->alienTag );
		ent->alienTag = NULL;
		DetachTag( ent->humanTag );
		ent->humanTag = NULL;
	}

	/**
	 * @brief Immediately deletes all tags attached to an entity (skips all effects).
	 */
	void DeleteTags( gentity_t *ent )
	{
		Delete( ent->alienTag );
		ent->alienTag = NULL;
		Delete( ent->humanTag );
		ent->humanTag = NULL;
	}

	/**
	 * @brief Tags an entity.
	 */
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
				if ( ent->buildableTeam != team && ent->taggedByEnemy != team )
				{
					ent->taggedByEnemy = team;
					BaseClustering::Touch(ent->buildableTeam);
				}
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

		RemoveSimilar( origin, BCT_TAG, data, team, owner, 0, 0 );
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
}
