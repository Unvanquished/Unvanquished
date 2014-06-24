/*
===========================================================================

Copyright 2014 Unvanquished Developers

This file is part of Unvanquished.

Unvanquished is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished. If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "g_local.h"

#include "../../common/Clustering.h"

/**
 * @brief A clustering of entities in the world.
 */
class EntityClustering : Cluster::EuclideanClustering<gentity_t*, 3> {
	public:
		typedef Cluster::EuclideanClustering<gentity_t*, 3> super;
		typedef super::point_type                           point_type;
		typedef super::cluster_type                         cluster_type;
		typedef super::iter_type                            iter_type;

		EntityClustering(float laxity = 1.0,
		                 std::function<bool(gentity_t*, gentity_t*)> edgeVisCallback = nullptr)
		    : super(laxity, edgeVisCallback)
		{}

		inline void Update(gentity_t *ent) {
			super::Update(ent, point_type(ent->s.origin));
		}

		inline void Remove(gentity_t *ent) {
			super::Remove(ent);
		}

		inline void Clear() {
			super::Clear();
		}

		inline void SetLaxity(float laxity = 1.0) {
			super::SetLaxity(laxity);
		}

		inline iter_type begin() {
			return super::begin();
		}

		inline iter_type end() {
			return super::end();
		}

		/**
		 * @brief An edge visibility check that checks for PVS visibility.
		 */
		static bool edgeVisPVS(gentity_t *a, gentity_t *b) {
			return trap_InPVSIgnorePortals(a->s.origin, b->s.origin);
		}
};

/**
 * @brief A wrapper around EntityClustering that keeps track of the bases of both teams.
 * @todo This is in a proof of concept state, improve the integration in the beacon system.
 */
class BaseClusterings {
	public:
		EntityClustering alienBases;
		EntityClustering humanBases;

		BaseClusterings(float laxity = 2.5, std::function<bool(gentity_t*, gentity_t*)>
		                edgeVisCallback = EntityClustering::edgeVisPVS)
		    : alienBases(laxity, edgeVisCallback), humanBases(laxity, edgeVisCallback)
		{}

		/**
		 * @brief Adds a buildable to the clusters or updates its location.
		 * @todo Remove workarounds for base/outpost beacons.
		 */
		void Update(gentity_t *ent) {
			EntityClustering *clusters = TeamClustering(ent);
			if (!clusters) return;
			clusters->Update(ent);

			// When the main structure is built, an outpost beacon needs to be removed manually.
			// The range of this is quite high so outpost beacons can be lost.
			// TODO: Enneract: Unify the two beacon types a bit.
			// TODO: Use G_IsMainStructure when merged.
			if (ent->s.modelindex == BA_A_OVERMIND || ent->s.modelindex == BA_H_REACTOR) {
				Beacon::RemoveSimilar(ent->s.origin, BCT_OUTPOST, 0, ent->buildableTeam,
				                      ENTITYNUM_NONE);
			}

			PostChangeHook(ent->buildableTeam);
		}

		/**
		 * @brief Removes a buildable from the clusters.
		 * @todo Remove workarounds for base/outpost beacons.
		 */
		void Remove(gentity_t *ent) {
			EntityClustering *clusters = TeamClustering(ent);
			if (!clusters) return;
			clusters->Remove(ent);

			// When the main structure is removed, the base beacon needs to be removed manually.
			// TODO: Enneract: Unify the two beacon types a bit.
			// TODO: Use G_IsMainStructure when merged.
			if (ent->s.modelindex == BA_A_OVERMIND || ent->s.modelindex == BA_H_REACTOR) {
				Beacon::RemoveSimilar(ent->s.origin, BCT_BASE, 0, ent->buildableTeam,
				                      ENTITYNUM_NONE);
			}

			PostChangeHook(ent->buildableTeam);
		}

		/**
		 * @brief Resets the clusterings.
		 */
		void Clear() {
			alienBases.Clear();
			humanBases.Clear();
		}

		/**
		 * @brief Prints debugging information and spawns pointer beacons for every cluster.
		 */
		void Debug() {
			for (int teamNum = TEAM_NONE + 1; teamNum < NUM_TEAMS; teamNum++) {
				team_t team = (team_t)teamNum;
				int clusterNum = 1;

				for (EntityClustering::cluster_type cluster : *TeamClustering(team)) {
					EntityClustering::point_type center = cluster.GetCenter();

					Com_Printf("%s base #%d (%lu buildings): Center: %.0f %.0f %.0f. "
					           "Avg. distance: %.0f. Standard deviation: %.0f.\n",
							   BG_TeamName(team), clusterNum, cluster.size(),
					           center[0], center[1], center[2],
					           cluster.GetAverageDistance(), cluster.GetStandardDeviation());

					Beacon::Propagate(Beacon::New(cluster.GetCenter().coords, BCT_POINTER, 0, team,
					                              ENTITYNUM_NONE));

					clusterNum++;
				}
			}
		}

	protected:
		/**
		 * @brief Called after calls to Update and Remove (but not Clear).
		 */
		void PostChangeHook(team_t team) {
			for (EntityClustering::cluster_type cluster : *TeamClustering(team)) {
				EntityClustering::point_type center = cluster.GetCenter();

				beaconType_t type = BCT_OUTPOST;
				for (EntityClustering::cluster_type::record_type record : cluster) {
					// TODO: Use G_IsMainStructure when merged.
					if (record.first->s.modelindex == BA_A_OVERMIND ||
					    record.first->s.modelindex == BA_H_REACTOR) {
						type = BCT_BASE;
						break;
					}
				}

				// TODO: Enneract: Move code from Cmd_Beaconf_f into helpers in a central location
				//       so it can be used from elsewhere without knowledge of the beacon internals.
				// TODO: Enneract: Either make the beacon spawn inside a room even if the center
				//       point is inside the world geometry or make RemoveSimiliar work with beacons
				//       stuck inside the world. Right now we have a beacon leak. :)
				Beacon::RemoveSimilar(center.coords, type, 0, team, ENTITYNUM_NONE);
				Beacon::Propagate(Beacon::New(center.coords, type, 0, team, ENTITYNUM_NONE));
			}
	}

	private:
		/**
		 * @brief Retrieves the clustering for a team.
		 */
		inline EntityClustering* TeamClustering(team_t team) {
			switch (team) {
				case TEAM_ALIENS: return &alienBases;
				case TEAM_HUMANS: return &humanBases;
				default:          return NULL;
			}
		}

		/**
		 * @brief Retreives the clustering for the team that owns the entity.
		 */
		inline EntityClustering* TeamClustering(gentity_t *ent) {
			return TeamClustering(ent->buildableTeam);
		}
};

/** The global base clusterings for both teams. */
BaseClusterings baseClusterings = BaseClusterings();

/**
 * @brief Resets the base clusterings.
 */
void G_InitBaseClusterings() {
	baseClusterings.Clear();
}

/**
 * @brief Prints debugging information and spawns pointer beacons for the base clusterings.
 */
void G_DebugBaseClusterings() {
	baseClusterings.Debug();
}

/**
 * @brief Adds a buildable to the base clusterings or updates its location.
 */
void G_BaseClusteringsUpdate(gentity_t *ent) {
	baseClusterings.Update(ent);
}

/**
 * @brief Removes a buildable from the base clusterings.
 */
void G_BaseClusteringsRemove(gentity_t *ent) {
	baseClusterings.Remove(ent);
}
