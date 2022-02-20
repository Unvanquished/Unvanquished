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

#include "sg_local.h"

#define MININUM_BASE_RADIUS 128.0f

namespace Clustering {
	/**
	 * @brief A clustering of entities in the world.
	 */
	class EntityClustering : public EuclideanClustering<gentity_t*, 3> {
		public:
			typedef Clustering::EuclideanClustering<gentity_t*, 3> super;

			EntityClustering(float laxity = 1.0,
							 std::function<bool(gentity_t*, gentity_t*)> edgeVisCallback = nullptr)
				: super(laxity, edgeVisCallback)
			{}

			void Update(gentity_t *ent) {
				super::Update(ent, VEC2GLM(ent->s.origin));
			}

			/**
			 * @brief An edge visibility check that checks for PVS visibility.
			 */
			static bool edgeVisPVS(gentity_t *a, gentity_t *b) {
				return trap_InPVSIgnorePortals(a->s.origin, b->s.origin);
			}
	};
}

/**
 * @brief Uses EntityClusterings to keep track of the bases of both teams.
 */
namespace BaseClustering {
	using namespace Clustering;

	enum baseClusteringLayer_t {
		BCL_ALIEN_FRIENDLY,
		BCL_ALIEN_ENEMY,
		BCL_HUMAN_FRIENDLY,
		BCL_HUMAN_ENEMY,

		NUM_BC_LAYERS
	};

	static std::map<baseClusteringLayer_t, EntityClustering>               bases;
	static std::map<baseClusteringLayer_t, std::unordered_set<gentity_t*>> beacons;

	/**
	 * @return Clustering identifier by team and enemy flag.
	 */
	static inline baseClusteringLayer_t GetClusteringLayer(team_t team, bool enemy) {
		if (team == TEAM_ALIENS) return enemy ? BCL_ALIEN_ENEMY : BCL_ALIEN_FRIENDLY;
		if (team == TEAM_HUMANS) return enemy ? BCL_HUMAN_ENEMY : BCL_HUMAN_FRIENDLY;
		return NUM_BC_LAYERS;
	}

	/**
	 * @return The team that receives information about the bases.
	 */
	static inline team_t GetInformedTeam(baseClusteringLayer_t layer) {
		if (layer == BCL_ALIEN_FRIENDLY || layer == BCL_ALIEN_ENEMY) return TEAM_ALIENS;
		if (layer == BCL_HUMAN_FRIENDLY || layer == BCL_HUMAN_ENEMY) return TEAM_HUMANS;
		return TEAM_NONE;
	}

	/**
	 * @return Whether the clustering is of enemy bases.
	 */
	static inline bool MarksEnemyBase(baseClusteringLayer_t layer) {
		if (layer == BCL_ALIEN_ENEMY || layer == BCL_HUMAN_ENEMY) return true;
		return false;
	}

	/**
	 * @brief Called after calls to Update and Remove.
	 */
	static void PostChangeHook(baseClusteringLayer_t layer) {
		std::unordered_set<gentity_t*> &oldBeacons = beacons[layer];
		std::unordered_set<gentity_t*> newBeacons;

		team_t team  = GetInformedTeam(layer);
		bool   enemy = MarksEnemyBase(layer);

		// Add a beacon for every cluster.
		for (EntityClustering::cluster_type& cluster : bases[layer]) {
			const EntityClustering::point_type &center = cluster.GetCenter();
			gentity_t *mean                            = cluster.GetMeanObject();
			float averageDistance                      = cluster.GetAverageDistance();
			float standardDeviation                    = cluster.GetStandardDeviation();

			float baseRadius = std::max(averageDistance + standardDeviation, MININUM_BASE_RADIUS);

			// Find out if it's an outpost or main base.
			bool mainBase = false;
			for (const EntityClustering::cluster_type::record_type& record : cluster) {
				gentity_t* ent = record.first;

				// TODO: Use G_IsMainStructure when merged.
				if (ent->s.modelindex2 == BA_A_OVERMIND || ent->s.modelindex2 == BA_H_REACTOR) {
					mainBase = true;
				}
			}

			// Trace from mean buildable's origin towards cluster center, so that the beacon does
			// not spawn inside a wall. Then use MoveTowardsRoom on the trace results.
			trace_t tr;
			trap_Trace(&tr, mean->s.origin, nullptr, nullptr, &center[0], 0, MASK_SOLID, 0);
			Beacon::MoveTowardsRoom(tr.endpos);

			// Prepare beacon flags.
			int eFlags = 0;
			if (!mainBase) eFlags |= EF_BC_BASE_OUTPOST;
			if (enemy)     eFlags |= EF_BC_ENEMY;

			// If a fitting beacon close to the target location already exists, move it silently,
			// otherwise add a new one.
			gentity_t *beacon;
			if (!(beacon = Beacon::MoveSimilar(&center[0], tr.endpos, BCT_BASE, 0, team, 0,
			                                   baseRadius, eFlags, EF_BC_BASE_RELEVANT))) {
				beacon = Beacon::New(tr.endpos, BCT_BASE, (int)baseRadius, team );
				beacon->s.eFlags |= eFlags;
				Beacon::Propagate(beacon);
			}

			newBeacons.insert(beacon);
		}

		// Delete all orphaned base beacons.
		for (gentity_t *beacon : oldBeacons) {
			if (newBeacons.find(beacon) == newBeacons.end()) {
				Beacon::Delete(beacon, (level.matchTime > 1000));
			}
		}

		oldBeacons.swap(newBeacons);
	}

	/**
	 * @brief Resets the clusterings.
	 */
	void Init() {
		// Reset clusterings and beacon lists for both teams
		for (int clusteringNum = 0; clusteringNum < NUM_BC_LAYERS; clusteringNum++) {
			baseClusteringLayer_t layer = (baseClusteringLayer_t)clusteringNum;

			// Reset clusterings
			std::map<baseClusteringLayer_t, EntityClustering>::iterator layerBases;
			if ((layerBases = bases.find(layer)) != bases.end()) {
				layerBases->second.Clear();
			} else {
				bases.insert(std::make_pair(
					layer, EntityClustering(2.5, EntityClustering::edgeVisPVS)));
			}

			// Reset beacon lists
			std::map<baseClusteringLayer_t, std::unordered_set<gentity_t*>>::iterator layerBeacons;
			if ((layerBeacons = beacons.find(layer)) != beacons.end()) {
				layerBeacons->second.clear();
			} else {
				beacons.insert(std::make_pair(layer, std::unordered_set<gentity_t*>()));
			}
		}
	}

	/**
	 * @brief Adds a buildable beacon to the clusterings or updates its location.
	 */
	void Update(gentity_t *beacon) {
		baseClusteringLayer_t layer =
			GetClusteringLayer((team_t)beacon->s.generic1, (beacon->s.eFlags & EF_BC_ENEMY));
		bases[layer].Update(beacon);
		PostChangeHook(layer);
	}

	/**
	 * @brief Removes a buildable beacon from the clusterings.
	 */
	void Remove(gentity_t *beacon) {
		baseClusteringLayer_t layer =
			GetClusteringLayer((team_t)beacon->s.generic1, (beacon->s.eFlags & EF_BC_ENEMY));
		if (bases[layer].Remove(beacon)) PostChangeHook(layer);
	}
}
