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

#include <map>
#include <set>

/**
 * @brief A simple disjoint set container.
 */
template<class ELEMENT> class DisjointSet {
	public:
		typedef std::set<ELEMENT> set_type;
		typedef typename std::map<ELEMENT, set_type*>::const_iterator iter_type;

		DisjointSet() = default;

		DisjointSet(const DisjointSet& other) {
			for (auto pair : other.sets) {
				sets.emplace(pair.first, new set_type(pair.second->begin(), pair.second->end()));
			}
			_size = other._size;
		}

		/**
		 * @brief Creates a new set with a single element. Careful: Does not enforce disjointness!
		 * @return The representative element for the set, which happens to be the parameter.
		 */
		ELEMENT MakeSet(const ELEMENT x) {
			set_type *newSet = new set_type();
			newSet->insert(x);
			sets.emplace(x, newSet);
			_size++;
			return x;
		}

		/**
		 * @return The representative of the set containing x or nullptr.
		 */
		ELEMENT Find(const ELEMENT x) const {
			for (auto pair : sets) {
				if (pair.second->find(x) != pair.second->end()) {
					return pair.first;
				}
			}
			return nullptr;
		}

		/**
		 * @brief Unites the sets represented by x and y. The new representative is x.
		 * @return The new representative.
		 */
		ELEMENT Link(const ELEMENT x, const ELEMENT y) {
			if (x != y) {
				set_type *xSet = sets.at(x);
				set_type *ySet = sets.at(y);
				_size -= xSet->size() + ySet->size();
				xSet->insert(ySet->cbegin(), ySet->cend());
				sets.erase(y);
				delete ySet;
				_size += xSet->size();
			}
			return x;
		}

		/**
		 * @brief Finds the representative elements and unites their sets.
		 *        The same as Link(Find(x), Find(y)).
		 */
		inline void Union(const ELEMENT x, const ELEMENT y) {
			Link(Find(x), Find(y));
		}

		/**
		 * @return Whether the elements are in the same set.
		 */
		bool Connected(const ELEMENT x, const ELEMENT y) {
			ELEMENT xRepresentative = Find(x);
			return (xRepresentative == Find(y) && xRepresentative != nullptr);
		}

		inline const iter_type begin() const {
			return sets.cbegin();
		}

		inline const iter_type cbegin() const {
			return sets.cbegin();
		}

		inline const iter_type end() const {
			return sets.cend();
		}

		inline const iter_type cend() const {
			return sets.cend();
		}

		/**
		 * @return The size of the container as seen when iterating.
		 */
		std::size_t size() {
			return sets.size();
		}

		/**
		 * @return The total amount of elements stored.
		 */
		std::size_t TotalSize() {
			return _size;
		}

	protected:
		std::map<ELEMENT, set_type*> sets;
		std::size_t _size;
};
