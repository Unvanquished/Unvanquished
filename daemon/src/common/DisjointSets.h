/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
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

#ifndef COMMON_DISJOINTSETS_H_
#define COMMON_DISJOINTSETS_H_

/**
 * @brief A simple container of disjoint sets.
 */
template<typename Element> class DisjointSets {
	public:
		using set_type = std::unordered_set<Element>;
		using iter_type = typename std::unordered_map<Element, set_type>::const_iterator;

		DisjointSets()
		    : totalSize(0)
		{}

		/**
		 * @brief Creates a new set with a single element. Careful: Does not enforce disjointness!
		 * @return The representative element for the set, which happens to be the parameter.
		 */
		Element MakeSetFast(const Element& x) {
			sets.insert(std::make_pair(x, set_type{x}));
			totalSize++;
			return x;
		}

		/**
		 * @brief Creates a new set with a single element. Enforces disjointness at some cost.
		 * @return The representative element for the set that contains x after the operation.
		 */
		Element MakeSetSecure(const Element& x) {
			Element& xRepr = Find(x);
			if (xRepr == nullptr) {
				sets.insert(std::make_pair(x, set_type{x}));
				totalSize++;
				return x;
			} else {
				return xRepr;
			}
		}

		/**
		 * @return The representative of the set containing x or nullptr.
		 */
		Element Find(const Element& x) const {
			for (auto& pair : sets) {
				if (pair.second.find(x) != pair.second.end()) {
					return pair.first;
				}
			}
			return nullptr;
		}

		/**
		 * @brief Unites the sets represented by x and y. The new representative is x.
		 * @return The new representative.
		 */
		Element Link(const Element& x, const Element& y) {
			if (x != y) {
				set_type& xSet = sets.at(x);
				set_type& ySet = sets.at(y);
				totalSize -= xSet.size() + ySet.size();
				xSet.insert(ySet.begin(), ySet.end());
				sets.erase(y);
				totalSize += xSet.size();
			}
			return x;
		}

		/**
		 * @brief Finds the representative elements and unites their sets.
		 *        The same as Link(Find(x), Find(y)).
		 * @return The new representative.
		 */
		Element Union(const Element& x, const Element& y) {
			return Link(Find(x), Find(y));
		}

		/**
		 * @return Whether the elements are in the same set.
		 */
		bool Connected(const Element& x, const Element& y) {
			Element& xRepr = Find(x);
			return (xRepr != nullptr && xRepr == Find(y));
		}

		iter_type begin() const {
			return sets.begin();
		}

		iter_type end() const {
			return sets.end();
		}

		/**
		 * @return The size of the container as seen when iterating.
		 */
		size_t size() {
			return sets.size();
		}

		/**
		 * @return The total amount of elements stored.
		 */
		size_t TotalSize() {
			return totalSize;
		}

	protected:
		std::unordered_map<Element, set_type> sets;
		size_t totalSize;
};

#endif // COMMON_DISJOINTSETS_H_
