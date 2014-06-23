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

#pragma once

#include <vector>
#include <map>
#include <set>
#include <cmath>

#include "DisjointSet.h"

namespace Cluster {
	/**
	 * @brief A point in euclidean space.
	 * @todo Use engine's vector class once available.
	 */
	template <int DIM>
	class Point {
		public:
			float coords[DIM];

			Point() = default;

			Point(const float vec[DIM]) {
				for (int i = 0; i < DIM; i++) {
					this->coords[i] = vec[i];
				}
			}

			Point(const Point &other) {
				for (int i = 0; i < DIM; i++) {
					this->coords[i] = other.coords[i];
				}
			}

			void Clear() {
				for (int i = 0; i < DIM; i++) {
					coords[i] = 0;
				}
			}

			float& operator[](int i) {
				return coords[i];
			}

			const float& operator[](int i) const {
				return coords[i];
			}

			Point<DIM> operator+(const Point<DIM> &other) const {
				Point<DIM> result = Point<DIM>();
				for (int i = 0; i < DIM; i++) {
					result[i] = coords[i] + other[i];
				}
				return result;
			}

			Point<DIM> operator-(const Point<DIM> &other) const {
				Point<DIM> result = Point<DIM>();
				for (int i = 0; i < DIM; i++) {
					result[i] = coords[i] - other[i];
				}
				return result;
			}

			Point<DIM> operator*(float value) const {
				Point<DIM> result = Point<DIM>();
				for (int i = 0; i < DIM; i++) {
					result[i] = coords[i] / value;
				}
				return result;
			}

			Point<DIM> operator/(float value) const {
				Point<DIM> result = Point<DIM>();
				for (int i = 0; i < DIM; i++) {
					result[i] = coords[i] / value;
				}
				return result;
			}

			void operator+=(const Point<DIM> &other) {
				for (int i = 0; i < DIM; i++) {
					coords[i] += other[i];
				}
			}

			void operator-=(const Point<DIM> &other) {
				for (int i = 0; i < DIM; i++) {
					coords[i] -= other[i];
				}
			}

			void operator*=(float value) {
				for (int i = 0; i < DIM; i++) {
					coords[i] *= value;
				}
			}

			void operator/=(float value) {
				for (int i = 0; i < DIM; i++) {
					coords[i] /= value;
				}
			}

			bool operator==(const Point<DIM> &other) const {
				for (int i = 0; i < DIM; i++) {
					if (coords[i] != other[i]) {
						return false;
					}
				}
				return true;
			}

			inline float Dot(const Point &other) const {
				float product = 0;
				for (int i = 0; i < DIM; i++) {
					product += coords[i] * other[i];
				}
				return product;
			}

			inline float Distance(const Point &other) const {
				float distanceSquared = 0;
				for (int i = 0; i < DIM; i++) {
					float delta = coords[i] - other[i];
					distanceSquared += delta * delta;
				}
				return std::sqrt(distanceSquared);
			}
	};

	/**
	 * A cluster of objects with a location. Technically an adaptor for std::map.
	 */
	template <class LOCATION, class DATA>
	class Cluster {
		public:
			typedef std::pair<DATA, LOCATION> record_type;
			typedef typename std::map<DATA, LOCATION>::const_iterator iter_type;

			Cluster() = default;

			inline void Update(const DATA data, const LOCATION location) {
				records.emplace(data, location);
			}

			inline void Remove(const DATA data) {
				records.erase(data);
			}

			inline void Clear() {
				records.clear();
			}

			inline iter_type begin() const noexcept {
				return records.begin();
			}

			inline iter_type end() const noexcept {
				return records.end();
			}

			inline iter_type cbegin() const noexcept {
				return records.cbegin();
			}

			inline iter_type cend() const noexcept {
				return records.cend();
			}

			inline iter_type find(const DATA &key) const {
				return records.find(key);
			}

			inline size_t size() const {
				return records.size();
			}

		protected:
			/**
			 * @brief Maps data objects to their location.
			 */
			std::map<DATA, LOCATION> records;
	};

	/**
	 * @brief A cluster of objects located in euclidean space.
	 */
	template <class DATA, int DIM>
	class EuclideanCluster : public Cluster<Point<DIM>, DATA> {
		public:
			typedef Point<DIM>                  point_type;
			typedef Cluster<point_type, DATA>   super;
			typedef typename super::record_type record_type;

			EuclideanCluster() = default;

			/**
			 * @brief Updates the cluster while keeping track of metadata.
			 */
			void Update(const DATA data, const point_type location) {
				super::Update(data, location);
				dirty = true;
			}

			/**
			 * @brief Removes data from the cluster while keeping track of metadata.
			 */
			void Remove(const DATA data) {
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
			 * @return The average distance from the center.
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

			point_type center;
			float      averageDistance;
			float      standardDeviation;
			bool       dirty;
	};

	/**
	 * @brief A self-organizing container of clusters of objects located in euclidean space.
	 *
	 * The clustering algorihm is based on Kruskal's algorithm for minimum spanning trees.
	 * In the minimum spanning tree of all edges that pass the optional visibility check, delete
	 */
	template <class DATA, int DIM>
	class EuclideanClustering {
		public:
			typedef Point<DIM>                  point_type;
			typedef EuclideanCluster<DATA, DIM> cluster_type;
			typedef DATA                        vertex_type;
			typedef std::pair<DATA, point_type> vertex_record_type;
			typedef std::pair<DATA, DATA>       edge_type;
			typedef typename std::vector<cluster_type*>::const_iterator iter_type;

			/**
			 * @brief Edges are stored in multimaps with their length as the key, so that they are
			 *        implicitly sorted.
			 */
			typedef std::pair<float, edge_type> edge_record_type;

			EuclideanClustering(float laxity = 1.0,
			                    std::function<bool(DATA, DATA)> edgeVisCallback = nullptr)
			    : laxity(laxity), edgeVisCallback(edgeVisCallback)
			{}

			/**
			 * @brief Adds or updates the location of objects.
			 */
			void Update(const DATA data, const Point<DIM> location) {
				// TODO: Check if the location has actually changed

				// Remove the object first
				Remove(data);

				// Iterate over all other objects and save the distance
				for ( vertex_record_type record : records ) {
					if (edgeVisCallback == nullptr || edgeVisCallback(data, record.first)) {
						float distance = location.Distance(record.second);
						edges.emplace(distance, edge_type(data, record.first));
					}
				}

				// The object is now known
				records.emplace(data, location);

				// Rebuild MST and clusters on next read access
				dirtyMST = true;
			}

			/**
			 * @brief Removes objects.
			 */
			void Remove(const DATA data) {
				// Check if we even know the object to avoid unnecessary O(n²) iteration.
				if (records.find(data) == records.end()) return;

				// Delete all edges that involve the object.
				for (typename std::map<float, edge_type>::iterator edge = edges.begin();
				     edge != edges.end(); ) {
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

			iter_type cbegin() {
				if (dirtyMST || dirtyClusters) GenerateClusters();
				return clusters.cbegin();
			}

			iter_type cend() {
				if (dirtyMST || dirtyClusters) GenerateClusters();
				return clusters.cend();
			}

			inline iter_type begin() {
				return cbegin();
			}

			inline iter_type end() {
				return cend();
			}

		private:
			/**
			 * @brief Finds the minimum spanning tree in the graph defined by edges, where edge
			 *        weight is the euclidean distance of the data object's location.
			 *
			 * @details Uses Kruskal's algorithm.
			 */
			void FindMST() {
				// Clear an existing MST.
				mstEdges.clear();
				mstAverageDistance   = 0;
				mstStandardDeviation = 0;

				// Track connected components for circle prevention.
				DisjointSet<DATA> components = DisjointSet<DATA>();

				// The edges are implicitely sorted by distance, iterate in ascending order.
				for (edge_record_type edgeRecord : edges) {
					float distance  = edgeRecord.first;
					edge_type *edge = &edgeRecord.second;

					// Stop if spanning tree is complete.
					if ((mstEdges.size() + 1) == records.size()) break;

					// Get component representatives, if available.
					DATA firstVertexRepr  = components.Find(edge->first);
					DATA secondVertexRepr = components.Find(edge->second);

					// Otheriwse add vertices to new components.
					if (firstVertexRepr == nullptr) {
						firstVertexRepr = components.MakeSet(edge->first);
					}
					if (secondVertexRepr == nullptr) {
						secondVertexRepr = components.MakeSet(edge->second);
					}

					// Don't create circles.
					if (firstVertexRepr == secondVertexRepr) continue;

					// Mark components as connected.
					components.Link(firstVertexRepr, secondVertexRepr);

					// Add the edge to the MST.
					mstEdges.emplace(distance, *edge);

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
			 * @details In the miminimum spanning tree, it cuts the edges that are longer than a
			 *          threshold (average_edge_length + standard_deviation * laxity).
			 *          Every tree in the resulting forest spans a cluster.
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
				DisjointSet<DATA> components = DisjointSet<DATA>();
				for (edge_record_type edgeRecord : forestEdges) {
					edge_type *edge = &edgeRecord.second;

					// Get component representatives, if available.
					DATA firstVertexRepr  = components.Find(edge->first);
					DATA secondVertexRepr = components.Find(edge->second);

					// Otheriwse add vertices to new components.
					if (firstVertexRepr == nullptr) {
						firstVertexRepr = components.MakeSet(edge->first);
					}
					if (secondVertexRepr == nullptr) {
						secondVertexRepr = components.MakeSet(edge->second);
					}

					// Mark components as connected.
					components.Link(firstVertexRepr, secondVertexRepr);
				}

				// Keep track of non-clustered vertices.
				std::set<DATA> remainingVertices = std::set<DATA>();
				for (vertex_record_type record : records) {
					remainingVertices.insert(record.first);
				}

				// Build a cluster for each connected component, excluding isolated vertices.
				for (auto component : components) {
					cluster_type *newCluster = new cluster_type();
					for (DATA vertex : *component.second) {
						newCluster->Update(vertex, records[vertex]);
						remainingVertices.erase(vertex);
					}
					clusters.push_back(newCluster);
				}

				// The remaining vertices are isolated, they go in a cluster of their own.
				for (DATA vertex : remainingVertices) {
					cluster_type *newCluster = new cluster_type();
					newCluster->Update(vertex, records[vertex]);
					clusters.push_back(newCluster);
				}

				dirtyClusters = false;
			}

			/**
			 * @brief The generated clusters.
			 */
			std::vector<cluster_type*> clusters;

			/**
			 * @brief Maps data objects to their location.
			 */
			std::map<DATA, point_type> records;

			/**
			 * @brief The edges of a non-reflexive but otherwise complete graph of all data objects,
			 *        implicitly sorted by distance.
			 */
			std::multimap<float, edge_type> edges;

			/**
			 * @brief The edges of the minimum spanning tree in the graph defined by @c edges,
			 *        implicitly sorted by distance.
			 */
			std::multimap<float, edge_type> mstEdges;

			/**
			 * @brief The edges of a forest of spanning trees, each connected component spans a
			 *        cluster. Is a subset of `mstEdges`.
			 */
			std::multimap<float, edge_type> forestEdges;

			/**
			 * @brief The average edge length in the minimum spanning tree.
			 */
			float mstAverageDistance;

			/**
			 * @brief The standard deviation of the edge length in the minimum spanning tree.
			 */
			float mstStandardDeviation;

			/**
			 * @brief Whether clusters need to be rebuild on read acces. Does not imply regeneration
			 *        of the minimum spanning tree.
			 */
			bool dirtyClusters;

			/**
			 * @brief Whether the minimum spanning tree needs to be rebuild on read access. Implies
			 *        regeneration of clusters.
			 */
			bool dirtyMST;

			/**
			 * @brief A factor that scales the allowed deviation from the average edge length when
			 *        splitting the minimum spanning tree into cluster spanning trees.
			 */
			float laxity;

			/**
			 * @brief A callback function that decides whether an edge should be considered for the
			 *        initial MST. Note that edges are bidirectional.
			 */
			std::function<bool(DATA, DATA)> edgeVisCallback;
	};
}
