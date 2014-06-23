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

		inline iter_type cbegin() {
			return super::cbegin();
		}

		inline iter_type cend() {
			return super::cend();
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
 */
class BaseClustering {
	public:
		EntityClustering alienBases;
		EntityClustering humanBases;

		BaseClustering(float laxity = 2.5, std::function<bool(gentity_t*, gentity_t*)>
		               edgeVisCallback = EntityClustering::edgeVisPVS)
		    : alienBases(laxity, edgeVisCallback), humanBases(laxity, edgeVisCallback)
		{}

		/**
		 * @brief Adds a buildable to the clusters or updates its location.
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

			AfterChangeHook(ent->buildableTeam);
		}

		/**
		 * @brief Removes a buildable from the clusters.
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

			AfterChangeHook(ent->buildableTeam);
		}

		/**
		 * @brief Resets the structure.
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

				for (EntityClustering::cluster_type *cluster : *TeamClustering(team)) {
					EntityClustering::point_type center = cluster->GetCenter();

					Com_Printf("%s base #%d (%lu buildings): Center: %.0f %.0f %.0f. "
					           "Avg. distance: %.0f. Standard deviation: %.0f.\n",
							   BG_TeamName(team), clusterNum, cluster->size(),
					           center[0], center[1], center[2],
					           cluster->GetAverageDistance(), cluster->GetStandardDeviation());

					Beacon::Propagate(Beacon::New(cluster->GetCenter().coords, BCT_POINTER, 0, team,
					                              ENTITYNUM_NONE));

					clusterNum++;
				}
			}
		}

	protected:
		/**
		 * @brief Called after calls to Update and Remove (but not Clear).
		 */
		void AfterChangeHook(team_t team) {
			for (EntityClustering::cluster_type *cluster : *TeamClustering(team)) {
				EntityClustering::point_type center = cluster->GetCenter();

				beaconType_t type = BCT_OUTPOST;
				for (EntityClustering::cluster_type::record_type record : *cluster) {
					// TODO: Use G_IsMainStructure when merged.
					if (record.first->s.modelindex == BA_A_OVERMIND ||
					    record.first->s.modelindex == BA_H_REACTOR) {
						type = BCT_BASE;
						break;
					}
				}

				// TODO: Enneract: Move code from Cmd_Beaconf_f into helpers in a central location
				//       so it can be used from elsewhere without knowledge of the beacon internals.
				Beacon::RemoveSimilar(center.coords, type, 0, team, ENTITYNUM_NONE);
				Beacon::Propagate(Beacon::New(center.coords, type, 0, team, ENTITYNUM_NONE));
			}
	}

	private:
		inline EntityClustering* TeamClustering(team_t team) {
			switch (team) {
				case TEAM_ALIENS: return &alienBases;
				case TEAM_HUMANS: return &humanBases;
				default:          return NULL;
			}
		}

		inline EntityClustering* TeamClustering(gentity_t *ent) {
			return TeamClustering(ent->buildableTeam);
		}
};

BaseClustering baseClustering = BaseClustering();

void G_InitBaseClustering() {
	baseClustering.Clear();
}

void G_DebugBaseClustering() {
	baseClustering.Debug();
}

void G_BaseClusteringUpdate(gentity_t *ent) {
	baseClustering.Update(ent);
}

void G_BaseClusteringRemove(gentity_t *ent) {
	baseClustering.Remove(ent);
}
