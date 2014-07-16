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
	 * @brief Move a beacon to a specified point in space.
	 */
	void Move( gentity_t *ent, const vec3_t origin )
	{
		VectorCopy( origin, ent->s.pos.trBase );
		VectorCopy( origin, ent->r.currentOrigin );
		VectorCopy( origin, ent->s.origin );
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
		ent->classname = "beacon";

		ent->s.modelindex = type;
		ent->s.modelindex2 = data;
		ent->s.generic1 = team;
		ent->s.otherEntityNum = owner;
		ent->think = Think;
		ent->nextthink = level.time;

		ent->s.time = level.time;
		decayTime = BG_Beacon( type )->decayTime;
		ent->s.time2 = ( decayTime ? level.time + decayTime : 0 );

		ent->s.pos.trType = TR_INTERPOLATE;
		Move( ent, origin );

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
		RemoveSimilar( origin, type, 0, team, ENTITYNUM_NONE, 0, 0, 0 );
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

	/**
	 * @brief Move a point towards empty space (away from map geometry).
	 * @param normal Optional direction to move towards.
	 */
	void MoveTowardsRoom( vec3_t origin, const vec3_t normal )
	{
		static const vec3_t vecs[ 162 ] =
		{
			{0.000000, 0.000000, -1.000000}, {0.723607, -0.525725, -0.447220}, {-0.276388, -0.850649, -0.447220},
			{-0.894426, 0.000000, -0.447216}, {-0.276388, 0.850649, -0.447220}, {0.723607, 0.525725, -0.447220},
			{0.276388, -0.850649, 0.447220}, {-0.723607, -0.525725, 0.447220}, {-0.723607, 0.525725, 0.447220},
			{0.276388, 0.850649, 0.447220}, {0.894426, 0.000000, 0.447216}, {0.000000, 0.000000, 1.000000},
			{-0.232822, -0.716563, -0.657519}, {-0.162456, -0.499995, -0.850654}, {-0.077607, -0.238853, -0.967950},
			{0.203181, -0.147618, -0.967950}, {0.425323, -0.309011, -0.850654}, {0.609547, -0.442856, -0.657519},
			{0.531941, -0.681712, -0.502302}, {0.262869, -0.809012, -0.525738}, {-0.029639, -0.864184, -0.502302},
			{0.812729, 0.295238, -0.502301}, {0.850648, 0.000000, -0.525736}, {0.812729, -0.295238, -0.502301},
			{0.203181, 0.147618, -0.967950}, {0.425323, 0.309011, -0.850654}, {0.609547, 0.442856, -0.657519},
			{-0.753442, 0.000000, -0.657515}, {-0.525730, 0.000000, -0.850652}, {-0.251147, 0.000000, -0.967949},
			{-0.483971, -0.716565, -0.502302}, {-0.688189, -0.499997, -0.525736}, {-0.831051, -0.238853, -0.502299},
			{-0.232822, 0.716563, -0.657519}, {-0.162456, 0.499995, -0.850654}, {-0.077607, 0.238853, -0.967950},
			{-0.831051, 0.238853, -0.502299}, {-0.688189, 0.499997, -0.525736}, {-0.483971, 0.716565, -0.502302},
			{-0.029639, 0.864184, -0.502302}, {0.262869, 0.809012, -0.525738}, {0.531941, 0.681712, -0.502302},
			{0.956626, -0.147618, 0.251149}, {0.951058, -0.309013, -0.000000}, {0.860698, -0.442858, -0.251151},
			{0.860698, 0.442858, -0.251151}, {0.951058, 0.309013, 0.000000}, {0.956626, 0.147618, 0.251149},
			{0.155215, -0.955422, 0.251152}, {0.000000, -1.000000, -0.000000}, {-0.155215, -0.955422, -0.251152},
			{0.687159, -0.681715, -0.251152}, {0.587786, -0.809017, 0.000000}, {0.436007, -0.864188, 0.251152},
			{-0.860698, -0.442858, 0.251151}, {-0.951058, -0.309013, -0.000000}, {-0.956626, -0.147618, -0.251149},
			{-0.436007, -0.864188, -0.251152}, {-0.587786, -0.809017, 0.000000}, {-0.687159, -0.681715, 0.251152},
			{-0.687159, 0.681715, 0.251152}, {-0.587786, 0.809017, -0.000000}, {-0.436007, 0.864188, -0.251152},
			{-0.956626, 0.147618, -0.251149}, {-0.951058, 0.309013, 0.000000}, {-0.860698, 0.442858, 0.251151},
			{0.436007, 0.864188, 0.251152}, {0.587786, 0.809017, -0.000000}, {0.687159, 0.681715, -0.251152},
			{-0.155215, 0.955422, -0.251152}, {0.000000, 1.000000, 0.000000}, {0.155215, 0.955422, 0.251152},
			{0.831051, -0.238853, 0.502299}, {0.688189, -0.499997, 0.525736}, {0.483971, -0.716565, 0.502302},
			{0.029639, -0.864184, 0.502302}, {-0.262869, -0.809012, 0.525738}, {-0.531941, -0.681712, 0.502302},
			{-0.812729, -0.295238, 0.502301}, {-0.850648, 0.000000, 0.525736}, {-0.812729, 0.295238, 0.502301},
			{-0.531941, 0.681712, 0.502302}, {-0.262869, 0.809012, 0.525738}, {0.029639, 0.864184, 0.502302},
			{0.483971, 0.716565, 0.502302}, {0.688189, 0.499997, 0.525736}, {0.831051, 0.238853, 0.502299},
			{0.077607, -0.238853, 0.967950}, {0.162456, -0.499995, 0.850654}, {0.232822, -0.716563, 0.657519},
			{0.753442, 0.000000, 0.657515}, {0.525730, 0.000000, 0.850652}, {0.251147, 0.000000, 0.967949},
			{-0.203181, -0.147618, 0.967950}, {-0.425323, -0.309011, 0.850654}, {-0.609547, -0.442856, 0.657519},
			{-0.203181, 0.147618, 0.967950}, {-0.425323, 0.309011, 0.850654}, {-0.609547, 0.442856, 0.657519},
			{0.077607, 0.238853, 0.967950}, {0.162456, 0.499995, 0.850654}, {0.232822, 0.716563, 0.657519},
			{0.052790, -0.688185, -0.723612}, {0.138199, -0.425321, -0.894429}, {0.361805, -0.587779, -0.723611},
			{0.670817, 0.162457, -0.723611}, {0.670818, -0.162458, -0.723610}, {0.447211, -0.000001, -0.894428},
			{-0.638195, -0.262864, -0.723609}, {-0.361801, -0.262864, -0.894428}, {-0.447211, -0.525729, -0.723610},
			{-0.447211, 0.525727, -0.723612}, {-0.361801, 0.262863, -0.894429}, {-0.638195, 0.262863, -0.723609},
			{0.361803, 0.587779, -0.723612}, {0.138197, 0.425321, -0.894429}, {0.052789, 0.688186, -0.723611},
			{1.000000, 0.000000, 0.000000}, {0.947213, -0.162458, -0.276396}, {0.947213, 0.162458, -0.276396},
			{0.309017, -0.951056, 0.000000}, {0.138199, -0.951055, -0.276398}, {0.447216, -0.850648, -0.276398},
			{-0.809018, -0.587783, 0.000000}, {-0.861803, -0.425324, -0.276396}, {-0.670819, -0.688191, -0.276397},
			{-0.809018, 0.587783, -0.000000}, {-0.670819, 0.688191, -0.276397}, {-0.861803, 0.425324, -0.276396},
			{0.309017, 0.951056, -0.000000}, {0.447216, 0.850648, -0.276398}, {0.138199, 0.951055, -0.276398},
			{0.670820, -0.688190, 0.276396}, {0.809019, -0.587783, -0.000002}, {0.861804, -0.425323, 0.276394},
			{-0.447216, -0.850648, 0.276397}, {-0.309017, -0.951056, -0.000001}, {-0.138199, -0.951055, 0.276397},
			{-0.947213, 0.162458, 0.276396}, {-1.000000, -0.000000, 0.000001}, {-0.947213, -0.162458, 0.276397},
			{-0.138199, 0.951055, 0.276397}, {-0.309016, 0.951057, -0.000000}, {-0.447215, 0.850649, 0.276397},
			{0.861804, 0.425322, 0.276396}, {0.809019, 0.587782, 0.000000}, {0.670821, 0.688189, 0.276397},
			{0.361800, -0.262863, 0.894429}, {0.447209, -0.525728, 0.723612}, {0.638194, -0.262864, 0.723610},
			{-0.138197, -0.425319, 0.894430}, {-0.361804, -0.587778, 0.723612}, {-0.052790, -0.688185, 0.723612},
			{-0.447210, 0.000000, 0.894429}, {-0.670817, 0.162457, 0.723611}, {-0.670817, -0.162457, 0.723611},
			{-0.138197, 0.425319, 0.894430}, {-0.052790, 0.688185, 0.723612}, {-0.361804, 0.587778, 0.723612},
			{0.361800, 0.262863, 0.894429}, {0.638194, 0.262864, 0.723610}, {0.447209, 0.525728, 0.723612}
		};
		const int numvecs = sizeof( vecs ) / sizeof( vecs[ 0 ] );
		int i;
		vec3_t accumulator, end;
		trace_t tr;

		VectorClear( accumulator );

		for ( i = 0; i < numvecs; i++ )
		{
			VectorMA( origin, 500, vecs[ i ], end );
			trap_Trace( &tr, origin, NULL, NULL, end, 0, MASK_SOLID );
			VectorAdd( accumulator, tr.endpos, accumulator );
		}

		VectorScale( accumulator, 1.0 / numvecs, accumulator );
		VectorCopy( accumulator, origin );
	}

	/**
	 * @brief Find a beacon matching a pattern.
	 * @return An ET_BEACON entity or NULL.
	 */
	gentity_t *FindSimilar( const vec3_t origin, beaconType_t type, int data, int team, int owner,
	                        float radius, int eFlags, int eFlagsRelevant )
	{
		for ( gentity_t *ent = NULL; (ent = G_IterateEntities(ent, NULL, true, 0, NULL)); )
		{
			// conditions common to all beacons

			if ( ent->s.eType != ET_BEACON )
				continue;

			if ( ent->s.modelindex != type )
				continue;

			if ( ( ent->s.eFlags & eFlagsRelevant ) != ( eFlags & eFlagsRelevant ) )
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
	 * @brief Remove all beacons matching a pattern.
	 * @return Number of beacons removed.
	 */
	int RemoveSimilar( const vec3_t origin, beaconType_t type, int data, int team, int owner,
	                   float radius, int eFlags, int eFlagsRelevant )
	{
		gentity_t *ent;
		int numRemoved = 0;
		while ((ent = FindSimilar(origin, type, data, team, owner, radius, eFlags, eFlagsRelevant)))
		{
			Delete( ent );
			numRemoved++;
		}
		return numRemoved;
	}

	/**
	 * @brief Move a single beacon matching a pattern.
	 * @return The moved beacon or NULL.
	 */
	gentity_t *MoveSimilar( const vec3_t from, const vec3_t to, beaconType_t type, int data,
	                        int team, int owner, float radius, int eFlags, int eFlagsRelevant )
	{
		gentity_t *ent;
		if ((ent = FindSimilar(from, type, data, team, owner, radius, eFlags, eFlagsRelevant)))
			Move(ent, to);
		return ent;
	}

	/**
	 * @brief Sets the entity fields so that the beacon is visible only by its team and spectators.
	 */
	void Propagate( gentity_t *ent )
	{
		ent->r.svFlags = SVF_BROADCAST | SVF_CLIENTMASK;

		G_TeamToClientmask( (team_t)ent->s.generic1, &ent->r.loMask, &ent->r.hiMask );

		// Don't send enemy bases or tagged enemy entities to spectators.
		if ( ent->s.eFlags & EF_BC_ENEMY )
		{}
		// Don't send tagged structures to spectators.
		else if ( ent->s.modelindex == BCT_TAG && !(ent->s.eFlags & EF_BC_TAG_PLAYER) )
		{}
		else
		{
			int loMask, hiMask;
			G_TeamToClientmask( TEAM_NONE, &loMask, &hiMask );
			ent->r.loMask |= loMask;
			ent->r.hiMask |= hiMask;
		}

		// Don't send a player tag to the tagged client itself.
		if ( ent->s.modelindex == BCT_TAG && (ent->s.eFlags & EF_BC_TAG_PLAYER) )
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

	static void UpdateTag( gentity_t *ent, gentity_t *parent )
	{
		Move( ent, parent->s.origin );

		if( parent->client )
		{
			if( parent->client->pers.team == TEAM_HUMANS )
				ent->s.modelindex2 = BG_GetPlayerWeapon( &parent->client->ps );
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
			UpdateTag( ent->alienTag, ent );
		if( ent->humanTag )
			UpdateTag( ent->humanTag, ent );
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
		qboolean dead, player;
		gentity_t *beacon, **attachment;
		team_t targetTeam;

		switch( ent->s.eType )
		{
			case ET_BUILDABLE:
				targetTeam = ent->buildableTeam;
				BG_BuildableBoundingBox( ent->s.modelindex, mins, maxs );
				data = ent->s.modelindex;
				dead = ( ent->health <= 0 );
				player = qfalse;

				// if tagging an enemy structure, check whether the base shall be tagged, too
				if ( targetTeam != team && ent->taggedByEnemy != team )
				{
					ent->taggedByEnemy = team;
					BaseClustering::Touch(ent->buildableTeam);
				}

				break;

			case ET_PLAYER:
				targetTeam = (team_t)ent->client->pers.team;
				BG_ClassBoundingBox( ent->client->pers.classSelection, mins, maxs, NULL, NULL, NULL );
				dead = ( ent->client && ent->client->ps.stats[ STAT_HEALTH ] <= 0 );
				owner = ent->s.number;
				player = qtrue;

				// data is the class (aliens) or the weapon number (humans)
				switch( targetTeam )
				{
					case TEAM_ALIENS:
						data = ent->client->ps.stats[ STAT_CLASS ];
						break;

					case TEAM_HUMANS:
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

		RemoveSimilar( origin, BCT_TAG, data, team, owner, 0, 0, 0 );
		beacon = New( origin, BCT_TAG, data, team, owner );

		if( player )
			beacon->s.eFlags |= EF_BC_TAG_PLAYER;

		if( team != targetTeam )
			beacon->s.eFlags |= EF_BC_ENEMY;

		if( permanent )
			beacon->s.time2 = 0;
		else if( player )
			beacon->s.time2 = level.time + 4000;
		else
			beacon->s.time2 = level.time + 35000;

		if( dead )
			DetachTag( beacon );
		else
			*attachment = beacon;

		Propagate( beacon );
	}
}
