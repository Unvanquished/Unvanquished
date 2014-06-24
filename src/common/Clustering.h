/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2014, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#ifndef COMMON_CLUSTERING_H_
#define COMMON_CLUSTERING_H_

#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <utility>

#include "DisjointSets.h"

namespace Cluster {
	/**
	 * @brief A point in euclidean space.
	 * @todo Use engine's vector class once available.
	 */
	template <int Dim>
	class Point {
		public:
			float coords[Dim];

			Point() = default;

			Point(const float vec[Dim]) {
				for (int i = 0; i < Dim; i++) coords[i] = vec[i];
			}

			Point(const Point &other) {
				for (int i = 0; i < Dim; i++) coords[i] = other.coords[i];
			}

			void Clear() {
				for (int i = 0; i < Dim; i++) coords[i] = 0;
			}

			float& operator[](int i) {
				return coords[i];
			}

			const float& operator[](int i) const {
				return coords[i];
			}

			Point<Dim> operator+(const Point<Dim> &other) const {
				Point<Dim> result;
				for (int i = 0; i < Dim; i++) result[i] = coords[i] + other[i];
				return result;
			}

			Point<Dim> operator-(const Point<Dim> &other) const {
				Point<Dim> result;
				for (int i = 0; i < Dim; i++) result[i] = coords[i] - other[i];
				return result;
			}

			Point<Dim> operator*(float value) const {
				Point<Dim> result;
				for (int i = 0; i < Dim; i++) result[i] = coords[i] * value;
				return result;
			}

			Point<Dim> operator/(float value) const {
				Point<Dim> result;
				for (int i = 0; i < Dim; i++) result[i] = coords[i] / value;
				return result;
			}

			void operator+=(const Point<Dim> &other) {
				for (int i = 0; i < Dim; i++) coords[i] += other[i];
			}

			void operator-=(const Point<Dim> &other) {
				for (int i = 0; i < Dim; i++) coords[i] -= other[i];
			}

			void operator*=(float value) {
				for (int i = 0; i < Dim; i++) coords[i] *= value;
			}

			void operator/=(float value) {
				for (int i = 0; i < Dim; i++) coords[i] /= value;
			}

			bool operator==(const Point<Dim> &other) const {
				for (int i = 0; i < Dim; i++) if (coords[i] != other[i]) return false;
				return true;
			}

			float Dot(const Point &other) const {
				float product = 0;
				for (int i = 0; i < Dim; i++) product += coords[i] * other[i];
				return product;
			}

			float Distance(const Point &other) const {
				float distanceSquared = 0;
				for (int i = 0; i < Dim; i++) {
					float delta = coords[i] - other[i];
					distanceSquared += delta * delta;
				}
				return std::sqrt(distanceSquared);
			}
	};

	/**
	 * @brief A cluster of objects with a location. Technically an adaptor for std::unordered_map.
	 */
	template <typename Location, typename Data>
	class Cluster {
		public:
			typedef std::pair<Data, Location>                                   record_type;
			typedef typename std::unordered_map<Data, Location>::const_iterator iter_type;

			Cluster() = default;

			void Update(const Data data, const Location location) {
				records.insert(std::make_pair(data, location));
			}

			void Remove(const Data data) {
				records.erase(data);
			}

			void Clear() {
				records.clear();
			}

			iter_type begin() const noexcept {
				return records.begin();
			}

			iter_type end() const noexcept {
				return records.end();
			}

			iter_type find(const Data &key) const {
				return records.find(key);
			}

			size_t size() const {
				return records.size();
			}

		protected:
			/** Maps data objects to their location. */
			std::unordered_map<Data, Location> records;
	};

	/**
	 * @brief A cluster of objects located in euclidean space.
	 */
	template <typename Data, int Dim>
	class EuclideanCluster : public Cluster<Point<Dim>, Data> {
		public:
			typedef Point<Dim>                  point_type;
			typedef Cluster<point_type, Data>   super;
			typedef typename super::record_type record_type;

			EuclideanCluster() = default;

			/**
			 * @brief Updates the cluster while keeping track of metadata.
			 */
			void Update(const Data data, const point_type location) {
				super::Update(data, location);
				dirty = true;
			}

			/**
			 * @brief Removes data from the cluster while keeping track of metadata.
			 */
			void Remove(const Data data) {
				super::Remove(data);
				dirty = true;
			}

			/**
			 * @return The euclidean center of the cluster.
			 */
			const point_type GetCenter() {
				if (dirty) UpdateMetadata();
				return center;
			}

			/**
			 * @return The average distance of objects from the center.
			 */
			float GetAverageDistance() {
				if (dirty) UpdateMetadata();
				return averageDistance;
			}

			/**
			 * @return The standard deviation from the average distance to the center.
			 */
			float GetStandardDeviation() {
				if (dirty) UpdateMetadata();
				return standardDeviation;
			}

		private:
			/**
			 * @brief Calculates cluster metadata.
			 */
			void UpdateMetadata() {
				dirty = false;

				center.Clear();
				averageDistance   = 0;
				standardDeviation = 0;

				int size = super::records.size();
				if (size == 0) return;

				// Find center
				for (typename super::record_type record : super::records) {
					center += record.second;
				}
				center /= size;

				// Find average distance
				for (typename super::record_type record : super::records) {
					averageDistance += record.second.Distance(center);
				}
				averageDistance /= size;

				// Find standard deviation
				for (typename super::record_type record : super::records) {
					float deviation = averageDistance - record.second.Distance(center);
					standardDeviation += deviation * deviation;
				}
				standardDeviation = std::sqrt(standardDeviation / size);
			}

			/** The cluster's center point. */
			point_type center;

			/** The average distance of objects from the cluster's center. */
			float averageDistance;

			/** The standard deviation from the average distance to the center. */
			float standardDeviation;

			/** Whether metadata needs to be recalculated. */
			bool dirty;
	};

	/**
	 * @brief A self-organizing container of clusters of objects located in euclidean space.
	 *
	 * The clustering algorihm is based on Kruskal's algorithm for minimum spanning trees.
	 * In the minimum spanning tree of all edges that pass the optional visibility check, delete the
	 * edges that are longer than the average plus the standard deviation multiplied by a "laxity"
	 * factor. The remaining trees span the clusters.
	 */
	template <typename Data, int Dim>
	class EuclideanClustering {
		public:
			typedef Point<Dim>                                         point_type;
			typedef EuclideanCluster<Data, Dim>                        cluster_type;
			typedef Data                                               vertex_type;
			typedef std::pair<Data, point_type>                        vertex_record_type;
			typedef std::pair<Data, Data>                              edge_type;
			typedef std::pair<float, edge_type>                        edge_record_type;
			typedef typename std::vector<cluster_type>::const_iterator iter_type;

			/**
			 * @param laxity Factor that scales the allowed deviation from the average edge length.
			 * @param edgeVisCallback Relation that decides whether an edge should be considered.
			 */
			EuclideanClustering(float laxity = 1.0,
			                    std::function<bool(Data, Data)> edgeVisCallback = nullptr)
			    : laxity(laxity), edgeVisCallback(edgeVisCallback)
			{}

			/**
			 * @brief Adds or updates the location of objects.
			 */
			void Update(const Data data, const Point<Dim> location) {
				// TODO: Check if the location has actually changed.

				// Remove the object first.
				Remove(data);

				// Iterate over all other objects and save the distance.
				for ( vertex_record_type record : records ) {
					if (edgeVisCallback == nullptr || edgeVisCallback(data, record.first)) {
						float distance = location.Distance(record.second);
						edges.insert(std::make_pair(distance, edge_type(data, record.first)));
					}
				}

				// The object is now known.
				records.insert(std::make_pair(data, location));

				// Rebuild MST and clusters on next read access.
				dirtyMST = true;
			}

			/**
			 * @brief Removes objects.
			 */
			void Remove(const Data data) {
				// Check if we even know the object in O(n) to avoid unnecessary O(m) iteration.
				if (records.find(data) == records.end()) return;

				// Delete all edges that involve the object.
				for (auto edge = edges.begin(); edge != edges.end(); ) {
				    if ((*edge).second.first == data || (*edge).second.second == data) {
						edge = edges.erase(edge);
					} else {
						edge++;
					}
				}

				// Forget about the object.
				records.erase(data);

				// Rebuild MST and clusters on next read access.
				dirtyMST = true;
			}

			void Clear() {
				records.clear();
				dirtyMST = true;
			}

			/**
			 * @brief Set laxity, a factor that scales the allowed deviation from the average edge
			 *        length when splitting the minimum spanning tree into cluster spanning trees.
			 */
			void SetLaxity(float laxity = 1.0) {
				this->laxity = laxity;
				dirtyClusters = true;
			}

			iter_type begin() {
				if (dirtyMST || dirtyClusters) GenerateClusters();
				return clusters.begin();
			}

			iter_type end() {
				if (dirtyMST || dirtyClusters) GenerateClusters();
				return clusters.end();
			}

		private:
			/**
			 * @brief Finds the minimum spanning tree in the graph defined by edges, where edge
			 *        weight is the euclidean distance of the data object's location.
			 *
			 * Uses Kruskal's algorithm.
			 */
			void FindMST() {
				// Clear an existing MST.
				mstEdges.clear();
				mstAverageDistance   = 0;
				mstStandardDeviation = 0;

				// Track connected components for circle prevention.
				DisjointSets<Data> components = DisjointSets<Data>();

				// The edges are implicitely sorted by distance, iterate in ascending order.
				for (edge_record_type edgeRecord : edges) {
					float distance  = edgeRecord.first;
					edge_type &edge = edgeRecord.second;

					// Stop if spanning tree is complete.
					if ((mstEdges.size() + 1) == records.size()) break;

					// Get component representatives, if available.
					Data firstVertexRepr  = components.Find(edge.first);
					Data secondVertexRepr = components.Find(edge.second);

					// Otheriwse add vertices to new components.
					if (firstVertexRepr == nullptr) {
						firstVertexRepr = components.MakeSetFast(edge.first);
					}
					if (secondVertexRepr == nullptr) {
						secondVertexRepr = components.MakeSetFast(edge.second);
					}

					// Don't create circles.
					if (firstVertexRepr == secondVertexRepr) continue;

					// Mark components as connected.
					components.Link(firstVertexRepr, secondVertexRepr);

					// Add the edge to the MST.
					mstEdges.insert(std::make_pair(distance, edge));

					// Add distance to average.
					mstAverageDistance += distance;
				}

				// Save metadata.
				int numMstEdges = mstEdges.size();
				if (numMstEdges != 0) {
					// Average distances.
					mstAverageDistance /= numMstEdges;

					// Find standard deviation.
					for (edge_record_type edgeRecord : mstEdges) {
						float deviation = mstAverageDistance - edgeRecord.first;
						mstStandardDeviation += deviation * deviation;
					}
					mstStandardDeviation = std::sqrt(mstStandardDeviation / numMstEdges);
				}

				dirtyMST = false;
			}

			/**
			 * @brief Generates the clusters.
			 *
			 * In the minimum spanning tree, delete the edges that are longer than the average plus
			 * the standard deviation multiplied by a "laxity" factor. The remaining trees span the
			 * clusters.
			 */
			void GenerateClusters() {
				if (dirtyMST) {
					FindMST();
				}

				// Clear an existing clustering.
				forestEdges.clear();
				clusters.clear();

				// Split the MST into several trees by keeping only the edges that have a length up
				// to a threshold.
				float edgeLengthThreshold = mstAverageDistance + mstStandardDeviation * laxity;
				for (edge_record_type edge : mstEdges) {
					if (edge.first <= edgeLengthThreshold) {
						forestEdges.insert(edge);
					} else {
						// Edges are implicitly sorted by distance, so we can stop here.
						break;
					}
				}

				// Retreive the connected components, excluding isolated vertices.
				DisjointSets<Data> components = DisjointSets<Data>();
				for (edge_record_type edgeRecord : forestEdges) {
					edge_type &edge = edgeRecord.second;

					// Get component representatives, if available.
					Data firstVertexRepr = components.Find(edge.first);
					Data secndVertexRepr = components.Find(edge.second);

					// Otheriwse add vertices to new components.
					if (firstVertexRepr == nullptr) {
						firstVertexRepr = components.MakeSetFast(edge.first);
					}
					if (secndVertexRepr == nullptr) {
						secndVertexRepr = components.MakeSetFast(edge.second);
					}

					// Mark components as connected.
					components.Link(firstVertexRepr, secndVertexRepr);
				}

				// Keep track of non-clustered vertices.
				std::unordered_set<Data> remainingVertices = std::unordered_set<Data>();
				for (vertex_record_type record : records) {
					remainingVertices.insert(record.first);
				}

				// Build a cluster for each connected component, excluding isolated vertices.
				for (auto component : components) {
					cluster_type newCluster;
					for (Data vertex : component.second) {
						newCluster.Update(vertex, records[vertex]);
						remainingVertices.erase(vertex);
					}
					clusters.push_back(newCluster);
				}

				// The remaining vertices are isolated, they go in a cluster of their own.
				for (Data vertex : remainingVertices) {
					cluster_type newCluster;
					newCluster.Update(vertex, records[vertex]);
					clusters.push_back(newCluster);
				}

				dirtyClusters = false;
			}

			/** The generated clusters. */
			std::vector<cluster_type> clusters;

			/** Maps data objects to their location. */
			std::unordered_map<Data, point_type> records;

			/**  The edges of a non-reflexive graph of the data objects, sorted by distance. */
			std::multimap<float, edge_type> edges;

			/** The edges of the minimum spanning tree in the graph defined by edges, sorted by
			 *  distance. Is a subset of edges. */
			std::multimap<float, edge_type> mstEdges;

			/** The edges of a forest of which each connected component spans a cluster. Is a subset
			 *  of mstEdges. */
			std::multimap<float, edge_type> forestEdges;

			/** The average edge length in the minimum spanning tree. */
			float mstAverageDistance;

			/** The standard deviation of the edge length in the minimum spanning tree. */
			float mstStandardDeviation;

			/** Whether clusters need to be rebuilt on read acces. Does not imply regeneration of
			 *  the minimum spanning tree. */
			bool dirtyClusters;

			/** Whether the minimum spanning tree needs to be rebuild on read access. Implies
			 *  regeneration of clusters. */
			bool dirtyMST;

			/** A factor that scales the allowed deviation from the average edge length when
			 *  splitting the minimum spanning tree into cluster spanning trees. */
			float laxity;

			/** A callback relation that decides whether an edge should be part of edges.
			 *  Needs to be symmetric as edges are bidirectional. */
			std::function<bool(Data, Data)> edgeVisCallback;
	};
}

#endif // COMMON_CLUSTERING_H_
