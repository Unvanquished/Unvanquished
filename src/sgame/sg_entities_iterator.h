/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2021 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#ifndef SG_ENTITIES_ITERATOR_H_
#define SG_ENTITIES_ITERATOR_H_

#include "sg_local.h"

//==================================================================

/*
 * An iterator type that allows checking for a condition on every element
 *
 * (Note this will stop at ENTITYNUM_MAX_NORMAL, so you technically
 * can iterate on every entity, except ENTITYNUM_WORLD. But if you want to do
 * something with it, you probably should special-case it in all case.)
 */
class EntityIterator
{
public:
	using predicate_t = bool (*)(const gentity_t *);
private:
	int index;
	predicate_t predicate;
	int min;
	int max;
public:
	using value_type        = gentity_t*; // see remark on operator*()
	using difference_type   = std::ptrdiff_t;
	using pointer           = gentity_t*;
	using reference         = gentity_t&;
	using iterator_category = std::bidirectional_iterator_tag;

	// Note that dereferencing this may provide an incorrect result.
	// It is present only because you need this function to provide
	// a forward or bidirectional iterator.
	EntityIterator() : index(0), predicate(nullptr), min(0), max(0) {}

	EntityIterator(const EntityIterator &other)
		: index(other.index), predicate(other.predicate),
		  min(other.min), max(other.max) {}

	// Note that this assumes a correct index, or that the calling code
	// uses "++" or "--" on the operator afterwards to reach an index
	// that satisfies the predicate
	EntityIterator(int index_, predicate_t predicate_, int min_, int max_)
		: index(index_), predicate(predicate_), min(min_), max(max_) {}

	EntityIterator& operator=(const EntityIterator &other) {
		index = other.index;
		predicate = other.predicate;
		min = other.min;
		max = other.max;
		return *this;
	}

	// this really should be gentity_t& operator*(), but this is
	// just so practical. CHECKME: is this fine?
	gentity_t* operator*() const { return &g_entities[index]; }

	// automatic casting to gentity_t* for convenience
	operator gentity_t*() const { return &g_entities[index]; }

	// the rest of the things needed to make a bidirectional iterator
	bool operator==(const EntityIterator& other) const {
		return index == other.index;
	}
	bool operator!=(const EntityIterator& other) const {
		return index != other.index;
	}
	EntityIterator operator++(int) {
		EntityIterator saved_copy(*this);
		++*this;
		return saved_copy;
	}
	EntityIterator& operator++() {
		while (++index < max)
		{
			if (!predicate || (*predicate)(&g_entities[index]))
			{
				break;
			}
		}
		return *this;
	}
	EntityIterator operator--(int) {
		EntityIterator saved_copy(*this);
		--*this;
		return saved_copy;
	}
	EntityIterator& operator--() {
		while (--index >= min)
		{
			if (!predicate || (*predicate)(&g_entities[index]))
			{
				break;
			}
		}
		return *this;
	}
	void swap(EntityIterator &other) {
		std::swap(other.predicate, predicate);
		std::swap(other.index, index);
	}
};


/*
 * Practical classes for iterating over using C++11 loops
 */

struct entity_iterator {
private:
	EntityIterator::predicate_t predicate;

public:
	entity_iterator(EntityIterator::predicate_t predicate_)
		: predicate(predicate_) {}

	EntityIterator begin() const {
		return ++EntityIterator(-1, predicate, 0, level.maxclients);
	}
	EntityIterator end() const {
		return EntityIterator(level.maxclients, predicate, 0, level.maxclients);
	}
};

// iterate over each entity that is in use
extern const entity_iterator iterate_entities;


struct client_entity_iterator {
private:
	EntityIterator::predicate_t predicate;

public:
	client_entity_iterator(EntityIterator::predicate_t predicate_)
		: predicate(predicate_) {}

	EntityIterator begin() const {
		return ++EntityIterator(-1, predicate, 0, level.maxclients);
	}
	EntityIterator end() const {
		return EntityIterator(level.maxclients, predicate, 0, level.maxclients);
	}
};

// iterate over the entity of each connected client
extern const client_entity_iterator iterate_client_entities;

// iterate over the entity of each bot
extern const client_entity_iterator iterate_bot_entities;


struct non_client_entity_iterator {
private:
	EntityIterator::predicate_t predicate;

public:
	non_client_entity_iterator(EntityIterator::predicate_t predicate_)
		: predicate(predicate_) {}

	EntityIterator begin() const {
		return ++EntityIterator(MAX_CLIENTS-1, predicate, MAX_CLIENTS, level.num_entities);
	}
	EntityIterator end() const {
		return EntityIterator(level.num_entities, predicate, MAX_CLIENTS, level.num_entities);
	}
};


// iterate over the entity of each non-client entity
extern const non_client_entity_iterator iterate_non_client_entities;

// iterate over the entity of each buildable
extern const non_client_entity_iterator iterate_buildable_entities;


#endif /* SG_ENTITIES_ITERATOR_H_ */
