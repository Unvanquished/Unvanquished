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

#include "sg_local.h"
#include "CBSE.h"

// entityState_t   | cbeacon_t    | description
// ----------------+--------------+-------------
// eType           | n/a          | always ET_BEACON
// otherEntityNum  | target       | tagged entity's number
// otherEntityNum2 | owner        | beacon's owner (ENTITYNUM_NONE if none, ENTITYNUM_WORLD if auto)
// modelindex      | type         | beacon's type (BCT_*)
// modelindex2     | data         | additional data (e.g. buildable type for BCT_TAG)
// generic1        | team         | beacon's team
// time            | ctime        | creation time
// time2           | etime        | expiration time
// apos.trTime     | mtime        | modification (update) time

#define bc_target otherEntityNum
#define bc_owner otherEntityNum2
#define bc_type modelindex
#define bc_data modelindex2
#define bc_team generic1
#define bc_ctime time
#define bc_etime time2
#define bc_mtime apos.trTime

namespace Beacon //this should eventually become a class
{
	/**
	 * @brief A meaningless think function for beacons (everything is now handled in Beacon::Frame).
	 */
	static void Think( gentity_t *ent )
	{
		ent->nextthink = level.time + 1000000;
	}

	/**
	 * @brief Handles beacon expiration and tag score decay. Called every server frame.
	 */
	void Frame()
	{
		gentity_t *ent = nullptr;
		static int nextframe = 0;

		if( nextframe > level.time )
			return;

		while ( ( ent = G_IterateEntities( ent ) ) )
		{
			switch( ent->s.eType )
			{
				case ET_BUILDABLE:
				case ET_PLAYER:
					if( ent->tagScoreTime + 2000 < level.time )
						ent->tagScore -= 50;
					if( ent->tagScore < 0 )
						ent->tagScore = 0;
					break;

				case ET_BEACON:
					if ( ent->s.bc_etime && level.time > ent->s.bc_etime )
						Delete( ent );
					continue;

				default:
					break;
			}
		}

		nextframe = level.time + 100;
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
	 * @param conflictHandler How to handle existing similiar beacons.
	 * @return A pointer to the new entity.
	 */
	gentity_t *New( const vec3_t origin, beaconType_t type, int data,
	                team_t team, int owner, beaconConflictHandler_t conflictHandler )
	{
		gentity_t *ent;
		int decayTime;

		switch( conflictHandler )
		{
			case BCH_MOVE:
				return MoveSimilar( origin, origin, type, data, team, owner );

			case BCH_REMOVE:
				RemoveSimilar( origin, type, data, team, owner );
				break;

			default:
				break;
		}

		ent = G_NewEntity( );
		ent->s.eType = ET_BEACON;
		ent->classname = "beacon";

		ent->s.bc_type = type;
		ent->s.bc_data = data;
		ent->s.bc_team = team;
		ent->s.bc_owner = owner;
		ent->think = Think;
		ent->nextthink = level.time;

		ent->s.bc_ctime = level.time;
		ent->s.bc_mtime = level.time;
		decayTime = BG_Beacon( type )->decayTime;
		ent->s.bc_etime = ( decayTime ? level.time + decayTime : 0 );

		ent->s.pos.trType = TR_INTERPOLATE;
		Move( ent, origin );

		return ent;
	}

	/**
	 * @brief Create and set up an area beacon (i.e. "Defend").
	 * @return A pointer to the new beacon entity.
	 */
	gentity_t *NewArea( beaconType_t type, const vec3_t point, team_t team )
	{
		vec3_t origin;
		gentity_t *beacon;

		VectorCopy( point, origin );
		MoveTowardsRoom( origin );
		beacon = Beacon::New( origin, type, 0, team, ENTITYNUM_NONE, BCH_REMOVE );
		Beacon::Propagate( beacon );

		return beacon;
	}

	/**
	 * @brief Delete a beacon instantly, without playing a sound or effect.
	 * @param verbose Whether to play an effect or remove the entity immediately.
	 */
	void Delete( gentity_t *ent, bool verbose )
	{
		if( !ent ) return;

		if( verbose )
		{
			// Defer removal and play death effects.
			if ( !( ent->s.eFlags & EF_BC_DYING ) )
			{
				ent->s.eFlags |= EF_BC_DYING;
				ent->s.bc_etime = level.time + 1500;
				BaseClustering::Remove( ent );
			}
		}
		else
		{
			// Detach link from tagged entity to its beacon.
			if( ent->tagAttachment ) *ent->tagAttachment = nullptr;

			// BaseClustering::Remove will be called inside G_FreeEntity since we need to be sure
			// that it happens no matter how the beacon was destroyed.
			G_FreeEntity( ent );
		}
	}

	/**
	 * @brief Move a point towards empty space (away from map geometry).
	 * @param normal Optional direction to move towards.
	 */
	void MoveTowardsRoom( vec3_t origin )
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
			trap_Trace( &tr, origin, nullptr, nullptr, end, 0, MASK_SOLID, 0 );
			VectorAdd( accumulator, tr.endpos, accumulator );
		}

		VectorScale( accumulator, 1.0 / numvecs, accumulator );
		VectorCopy( accumulator, origin );
	}

	/**
	 * @brief Find a beacon matching a pattern.
	 * @return An ET_BEACON entity or nullptr.
	 */
	gentity_t *FindSimilar( const vec3_t origin, beaconType_t type, int data, int team, int owner,
	                        float radius, int eFlags, int eFlagsRelevant )
	{
		int flags = BG_Beacon( type )->flags;

		for ( gentity_t *ent = nullptr; (ent = G_IterateEntities(ent)); )
		{
			if ( ent->s.eType != ET_BEACON )
				continue;

			if ( ent->s.bc_type != type )
				continue;

			if ( ( ent->s.eFlags & eFlagsRelevant ) != ( eFlags & eFlagsRelevant ) )
				continue;

			if( ent->s.bc_team != team )
				continue;

			if ( ( flags & BCF_DATA_UNIQUE ) && ent->s.bc_data != data )
				continue;

			if ( ent->s.eFlags & EF_BC_DYING )
				continue;

			if     ( flags & BCF_PER_TEAM )
			{}
			else if( flags & BCF_PER_PLAYER )
			{
				if( ent->s.bc_owner != owner )
					continue;
			}
			else
			{
				if ( Distance( ent->s.origin, origin ) > radius )
					continue;

				if ( !trap_InPVS( ent->s.origin, origin ) )
					continue;
			}

			return ent;
		}

		return nullptr;
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
	 * @return The moved beacon or nullptr.
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

		G_TeamToClientmask( (team_t)ent->s.bc_team, &ent->r.loMask, &ent->r.hiMask );

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
		if ( ent->s.bc_type == BCT_TAG && (ent->s.eFlags & EF_BC_TAG_PLAYER) )
		{
			int loMask, hiMask;
			G_ClientnumToMask( ent->s.bc_target, &loMask, &hiMask );
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
	void PropagateAll()
	{
		for ( gentity_t *ent = nullptr; (ent = G_IterateEntities( ent )); )
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
		for ( gentity_t *ent = nullptr; ( ent = G_IterateEntities( ent ) ); )
		{
			if ( ent->s.eType != ET_BEACON )
				continue;

			if ( ent->s.bc_owner != clientNum )
				continue;

			Delete( ent );
		}
	}

	/**
	 * @brief Moves a tag to the parent entity's bounding box center.
	 */
	static void UpdateTagLocation( gentity_t *ent, gentity_t *parent )
	{
		vec3_t mins, maxs, center;

		if ( !parent ) return;

		VectorCopy( parent->s.origin, center );

		if ( parent->client )
		{
			BG_ClassBoundingBox( parent->client->ps.stats[ STAT_CLASS ], mins, maxs, nullptr, nullptr, nullptr );
			BG_MoveOriginToBBOXCenter( center, mins, maxs );

			// Also update weapon for humans.
			if( parent->client->pers.team == TEAM_HUMANS )
			{
				ent->s.bc_data = BG_GetPlayerWeapon( &parent->client->ps );
			}
		}
		else if ( parent->s.eType == ET_BUILDABLE )
		{
			BG_BuildableBoundingBox( parent->s.modelindex, mins, maxs );
			BG_MoveOriginToBBOXCenter( center, mins, maxs );
		}

		Move( ent, center );
	}

	/**
	 * @brief Update tags attached to an entity.
	 */
	void UpdateTags( gentity_t *ent )
	{
		// Buildables are supposed to be static.
		if( ent->s.eType == ET_BUILDABLE )
			return;

		if( ent->alienTag )
			UpdateTagLocation( ent->alienTag, ent );
		if( ent->humanTag )
			UpdateTagLocation( ent->humanTag, ent );
	}

	/**
	 * @brief Deletes all tags attached to an entity (plays effects).
	 *
	 * Deletion is deferred for the enemy team's buildable tags until their death was confirmed.
	 */
	void DetachTags( gentity_t *ent )
	{
		if ( ent->alienTag  )
		{
			if ( ( ent->alienTag->s.eFlags & EF_BC_ENEMY ) &&
			     !( ent->alienTag->s.eFlags & EF_BC_TAG_PLAYER ) )
			{
				ent->alienTag->tagAttachment = nullptr;
				ent->alienTag = nullptr;
			}
			else
				Delete( ent->alienTag, true );
		}

		if ( ent->humanTag  )
		{
			if ( ( ent->humanTag->s.eFlags & EF_BC_ENEMY ) &&
			     !( ent->humanTag->s.eFlags & EF_BC_TAG_PLAYER ) )
			{
				ent->humanTag->tagAttachment = nullptr;
				ent->humanTag = nullptr;
			}
			else
				Delete( ent->humanTag, true );
		}
	}

	/**
	 * @brief Immediately deletes all tags attached to an entity (skips all effects).
	 */
	void DeleteTags( gentity_t *ent )
	{
		Delete( ent->alienTag );
		Delete( ent->humanTag );
	}

	/**
	 * @brief Reset a tag's expiration timer.
	 */
	static inline void RefreshTag( gentity_t *ent, bool force = false )
	{
		// Don't refreshing dying beacons, this would reset their animation.
		if ( ent->s.eFlags & EF_BC_DYING ) return;

		// Touch update time.
		ent->s.bc_mtime = level.time;

		// Don't "refresh" permanent/uninitialized tags unless forced to.
		if( !force && !ent->s.bc_etime ) return;

		if( ent->s.eFlags & EF_BC_TAG_PLAYER )
			ent->s.bc_etime = level.time + 2000;
	}

	static inline bool CheckRefreshTag( gentity_t *ent, team_t team )
	{
		gentity_t *existingTag = ( team == TEAM_ALIENS ) ? ent->alienTag : ent->humanTag;

		if( existingTag )
			RefreshTag( existingTag );

		return existingTag != nullptr;
	}

	/**
	 * @brief Check if an entity can be tagged.
	 * @param num Entity number of target.
	 * @param team Tagging team.
	 * @param trace Whether the tag is done manually by looking at an entity.
	 */
	bool EntityTaggable( int num, team_t team, bool trace )
	{
		gentity_t *ent;

		if( num == ENTITYNUM_NONE || num == ENTITYNUM_WORLD ) return false;

		ent = g_entities + num;

		switch( ent->s.eType )
		{
			case ET_BUILDABLE:
				if( G_Dead( ent ) )
					return false;
				if( ent->buildableTeam == team )
					return false;
				return true;

			case ET_PLAYER:
				if ( trace ) return false;

				if( !ent->client )
					return false;
				// don't tag teammates
				if( team != TEAM_NONE &&
				    ent->client->pers.team == team )
					return false;
				return true;

			default:
				return false;
		}
	}

	//TODO: clean this mess
	typedef struct
	{
		gentity_t *ent;
		float dot;
	} tagtrace_ent_t;

	static int TagTrace_EntCmp( const void *a, const void *b )
	{
		return ( ( (const tagtrace_ent_t*)a )->dot < ( (const tagtrace_ent_t*)b )->dot );
	}

	/**
	 * @brief Perform an approximate trace to find a taggable entity.
	 * @param team           Team the caller belongs to.
	 * @param refreshTagged  Refresh all already tagged entities's tags and exclude these entities from further consideration.
	 */
	gentity_t *TagTrace( const vec3_t begin, const vec3_t end, int skip, int mask, team_t team, bool refreshTagged )
	{
		tagtrace_ent_t list[ MAX_GENTITIES ];
		int i, count = 0;
		gentity_t *ent, *reticleEnt = nullptr;
		vec3_t seg, delta;
		float dot;

		VectorSubtract( end, begin, seg );

		// Do a trace for bounding boxes under the reticle first, they are prefered
		{
			trace_t tr;
			trap_Trace( &tr, begin, nullptr, nullptr, end, skip, mask, 0 );
			if ( EntityTaggable( tr.entityNum, team, true ) )
			{
				reticleEnt = g_entities + tr.entityNum;
				if ( !refreshTagged || !CheckRefreshTag( reticleEnt, team ) )
					return reticleEnt;
			}
		}

		for( i = 0; i < level.num_entities; i++ )
		{
			ent = g_entities + i;

			if( ent == reticleEnt )
				continue;

			if( !ent->inuse )
				continue;

			if( !EntityTaggable( i, team, true ) )
				continue;

			VectorSubtract( ent->r.currentOrigin, begin, delta );
			dot = DotProduct( seg, delta ) / VectorLength( seg ) / VectorLength( delta );

			if( dot < 0.9 )
				continue;

			if( !trap_InPVS( ent->r.currentOrigin, begin ) )
				continue;

			// LOS
			{
				trace_t tr;
				trap_Trace( &tr, begin, nullptr, nullptr, ent->r.currentOrigin, skip, mask, 0 );
				if( tr.entityNum != i )
					continue;
			}

			if( refreshTagged && CheckRefreshTag( ent, team ) )
				continue;

			list[ count ].ent = ent;
			list[ count++ ].dot = dot;
		}

		if( !count )
			return nullptr;

		qsort( list, count, sizeof( tagtrace_ent_t ), TagTrace_EntCmp );

		return list[ 0 ].ent;
	}

	/**
	 * @brief Tags an entity.
	 */
	void Tag( gentity_t *ent, team_t team, bool permanent )
	{
		int data;
		vec3_t origin, mins, maxs;
		bool dead, player;
		gentity_t *beacon, **attachment;
		team_t targetTeam;

		// Get the beacon attachment owned by the tagging team.
		switch( team ) {
			case TEAM_ALIENS: attachment = &ent->alienTag; break;
			case TEAM_HUMANS: attachment = &ent->humanTag; break;
			default:                                       return;
		}

		// Just refresh existing tags.
		if ( *attachment ) {
			RefreshTag( *attachment, true );
			return;
		}

		switch( ent->s.eType ) {
			case ET_BUILDABLE:
				targetTeam = ent->buildableTeam;
				data       = ent->s.modelindex;
				dead       = G_Dead( ent );
				player     = false;
				BG_BuildableBoundingBox( ent->s.modelindex, mins, maxs );
				break;

			case ET_PLAYER:
				targetTeam = (team_t)ent->client->pers.team;
				dead       = G_Dead( ent );
				player     = true;
				BG_ClassBoundingBox( ent->client->pers.classSelection, mins, maxs, nullptr, nullptr, nullptr );

				// Set beacon data to class (aliens) or weapon (humans).
				switch( targetTeam ) {
					case TEAM_ALIENS: data = ent->client->ps.stats[ STAT_CLASS ];    break;
					case TEAM_HUMANS: data = BG_GetPlayerWeapon( &ent->client->ps ); break;
					default:                                                         return;
				}

				break;

			default:
				return;
		}

		// Don't tag dead targets.
		if ( dead ) return;

		// Set beacon origin to center of target bounding box.
		VectorCopy( ent->s.origin, origin );
		BG_MoveOriginToBBOXCenter( origin, mins, maxs );

		// Create new beacon and attach it.
		beacon = New( origin, BCT_TAG, data, team );
		beacon->tagAttachment = attachment;
		*attachment = beacon;
		beacon->s.bc_target = ent - g_entities;

		// Reset the entity's tag score.
		ent->tagScore = 0;

		// Set flags.
		if( player )             beacon->s.eFlags |= EF_BC_TAG_PLAYER;
		if( team != targetTeam ) beacon->s.eFlags |= EF_BC_ENEMY;

		// Set expiration time.
		if( permanent ) beacon->s.bc_etime = 0;
		else            RefreshTag( beacon, true );

		// Update the base clusterings.
		if ( ent->s.eType == ET_BUILDABLE ) BaseClustering::Update( beacon );

		Propagate( beacon );
	}
}
