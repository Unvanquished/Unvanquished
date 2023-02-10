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
}

#endif /* SGAME_ENTITIES_H_ */
