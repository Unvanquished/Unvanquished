/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2018 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvnaquished is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef SGAME_ENTITIES_H_
#define SGAME_ENTITIES_H_

/*
 * External facades to CBSE entities and components.
 * This aims to reduce (re)compilation times by avoiding CBSE.h includes in
 * code that isn't instantiating entities, to aid migration and generally simplify code.
 *
 * It also contains CBSE or entity related helper functions:
 *   - Convenience functions and helpers that have not found a better place yet.
 *   - Compatibility functions to help migration.
 *
 * There is an inverse correlation between quality of architecture and amount of utility code needed.
 *
 * Implementations of helper functions that have a more cohesive place to go, should go there instead.
 * Facades to them however may stay here.
 */

#include "sg_local.h"

// only forward declarations needed for this header; no CBSE.h, or this would be pointless
class Entity;
template<class T> constexpr int ComponentPriority();

namespace Entities {
	/**
	 * @brief Whether two entities both are assigned to a team and it is the same one.
	 */
	bool OnSameTeam(Entity const& firstEntity, Entity const& secondEntity);

	/**
	 * @brief Whether two entities both are assigned to a team and the teams differ.
	 */
	bool OnOpposingTeams(Entity const& firstEntity, Entity const& secondEntity);

	/**
	 * @brief Whether the entity can die (has health) but is alive.
	 */
	bool IsAlive(Entity const& entity);
	bool IsAlive(gentity_t const*ent);

	/**
	 * @brief Whether the entity can be alive (has health) but is dead now.
	 */
	bool IsDead(Entity const& entity);
	bool IsDead(gentity_t const*ent);

	bool HasHealthComponent(gentity_t const*ent);

	/**
	 * @brief Returns the health of an entity.
	 * @warning passing an entity without HealthComponent likely crashes,
	 * 			use Entities::HasHealthComponent beforehand where necessary
	 */
	float HealthOf(Entity const& entity);
	float HealthOf(gentity_t const*ent);

	/**
	 * @brief Returns whether the entity is at full health.
	 * @warning passing an entity without HealthComponent likely crashes,
	 * 			use Entities::HasHealthComponent beforehand where necessary
	 */
	bool HasFullHealth(Entity const& entity);
	bool HasFullHealth(gentity_t const*ent);

	/**
	 * @brief Returns the fraction of health of the entity.
	 * @warning passing an entity without HealthComponent likely crashes,
	 * 			use Entities::HasHealthComponent beforehand where necessary
	 */
	float HealthFraction(Entity const& entity);
	float HealthFraction(gentity_t const*ent);

	/**
	 * @brief Deals the exact amount of damage necessary to kill the entity.
	 */
	void Kill(Entity& entity, Entity *source, meansOfDeath_t meansOfDeath);

	void Kill(gentity_t *ent, gentity_t *source, meansOfDeath_t meansOfDeath);
	void Kill(gentity_t *ent, meansOfDeath_t meansOfDeath);

	bool AntiHumanRadiusDamage(Entity& entity, float amount, float range, meansOfDeath_t mod);
	bool KnockbackRadiusDamage(Entity& entity, float amount, float range, meansOfDeath_t mod);

	// Component iteration internals
	namespace detail {
		class ComponentBitset {
			using word = uint32_t;
			static constexpr unsigned wordBits = std::numeric_limits<word>::digits;
			static constexpr unsigned numWords = (MAX_GENTITIES + wordBits - 1) / wordBits;

			word data_[numWords];

		public:
			ComponentBitset() : data_{} {}
			int Scan(int start) const; // find nonzero bit or return MAX_GENTITIES
			void Set(int num); // set bit to 1
			void Clear(int num); // set bit to 0
		};

		extern ComponentBitset componentSets[];

		// template hack to reuse the same code for EntityIterator and ComponentIterator
		template<typename ThisType>
		class EntityIteratorBase {
		protected:
			const ComponentBitset* bits_;
			int entityNum_;

			EntityIteratorBase() = default;

		public:
			bool operator!=(const ThisType& other) const {
				return entityNum_ != other.entityNum_;
			}

			void operator++()
			{
				entityNum_ = bits_->Scan(entityNum_ + 1);
			}

			static ThisType MakeBegin(const ComponentBitset* bits)
			{
				ThisType it;
				it.bits_ = bits;
				it.entityNum_ = -1;
				++it;
				return it;
			}

			static ThisType MakeEnd()
			{
				ThisType it;
				it.entityNum_ = MAX_GENTITIES;
				return it;
			}

			struct Collection {
				ComponentBitset* bits;

				ThisType begin() { return ThisType::MakeBegin(bits); }
				ThisType end() { return ThisType::MakeEnd(); }
			};
		};

		class EntityIterator : public EntityIteratorBase<EntityIterator> {
		public:
			Entity& operator*() const
			{
				return *g_entities[this->entityNum_].entity;
			}
		};

		// Hack to trick two-phase template compilation into letting me use Get() without
		// having the definition of Entity
		template<typename ComponentT, typename OldEntity = gentity_t>
		ComponentT* Get(OldEntity& ent)
		{
			return ent.entity->template Get<ComponentT>();
		}

		template<typename ComponentT>
		class ComponentIterator : public EntityIteratorBase<ComponentIterator<ComponentT>> {
		public:
			ComponentT& operator*() const
			{
				return *Get<ComponentT>(g_entities[this->entityNum_]);
			}
		};
	}

	// === Looping over all entities with a given component ===
	//
	// You may use Entities::Having to loop over all entities having a given component, or
	// Entities::Each to loop over the components themselves.
	//
	// The only approved way to use the entity iterators and entity collections is to request a
	// collection and then immediately iterate over it with a for-each loop. Other uses are
	// likely to result in unexpected behavior because the C++ iterator model is not a good fit
	// for the domain.
	//
	// Destroying entities during the loop is safe. Even the current one, as long as you don't
	// access the reference afterwards.
	// Creating entities during the loop is also safe, but it may be stupid because the new entity,
	// supposing it has the component in question, will be randomly included in the loop or not
	// depending on its entity num.

	// for (Entity& ent : Entities::Having<HealthComponent>()) { ... }
	template<typename ComponentT>
	detail::EntityIterator::Collection Having()
	{
		return { &detail::componentSets[ComponentPriority<ComponentT>()] };
	}

	// for (MiningComponent& miner : Entities::Each<MiningComponent>()) { ... }
	template<typename ComponentT>
	typename detail::ComponentIterator<ComponentT>::Collection Each()
	{
		return { &detail::componentSets[ComponentPriority<ComponentT>()] };
	}

	// Find a single entity with the specified component, or nullptr
	template<typename ComponentT>
	Entity* AnyWith()
	{
		int num = detail::componentSets[ComponentPriority<ComponentT>()].Scan(0);
		return num < MAX_GENTITIES ? g_entities[num].entity : nullptr;
	}

	// Find a single component of the specified type, or nullptr
	template<typename ComponentT>
	ComponentT* Any()
	{
		int num = detail::componentSets[ComponentPriority<ComponentT>()].Scan(0);
		return num < MAX_GENTITIES ? detail::Get<ComponentT>(g_entities[num]) : nullptr;
	}

	// TODO: add Count<Component> ?
}

#endif /* SGAME_ENTITIES_H_ */
