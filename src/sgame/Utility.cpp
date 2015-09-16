/*
===========================================================================

Copyright 2015 Unvanquished Developers

This file is part of Unvanquished.

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

/*
 * This file contains:
 *   - Helper functions for working with CBSE entities.
 *
 * It aims to relace sg_utils.cpp, which contains C89 helpers for legacy entities.
 * TODO: Port all helpers that use gentity_t here and make them use Entity instead.
 */

#include "CBSE.h"

void Utility::Kill(Entity& entity, Entity* source, meansOfDeath_t meansOfDeath) {
	HealthComponent *healthComponent = entity.Get<HealthComponent>();
	if (healthComponent) {
		entity.Damage(healthComponent->Health(), source ? source->oldEnt : nullptr, {}, {},
		              (DAMAGE_PURE | DAMAGE_NO_PROTECTION), meansOfDeath);
	}
}

bool Utility::AntiHumanRadiusDamage(Entity& entity, float amount, float range, meansOfDeath_t mod) {
	bool hit = false;

	ForEntities<HumanClassComponent>([&] (Entity& other, HumanClassComponent& humanClassComponent) {
		// TODO: Add LocationComponent.
		float distance = G_Distance(entity.oldEnt, other.oldEnt);
		float damage   = amount * (1.0f - distance / range);

		if (damage <= 0.0f) return;
		if (!G_IsVisible(entity.oldEnt, other.oldEnt, MASK_SOLID)) return;

		if (other.Damage(damage, entity.oldEnt, {}, {}, DAMAGE_NO_LOCDAMAGE, mod)) {
			hit = true;
		}
	});

	return hit;
}

bool Utility::KnockbackRadiusDamage(Entity& entity, float amount, float range, meansOfDeath_t mod) {
	bool hit = false;

	// FIXME: Only considering entities with HealthComponent.
	// TODO: Allow ForEntities to iterate over all entities.
	ForEntities<HealthComponent>([&] (Entity& other, HealthComponent& healthComponent) {
		// TODO: Add LocationComponent.
		float distance = G_Distance(entity.oldEnt, other.oldEnt);
		float damage   = amount * (1.0f - distance / range);

		if (damage <= 0.0f) return;
		if (!G_IsVisible(entity.oldEnt, other.oldEnt, MASK_SOLID)) return;

		if (other.Damage(damage, entity.oldEnt, {}, {}, DAMAGE_NO_LOCDAMAGE | DAMAGE_KNOCKBACK, mod)) {
			hit = true;
		}
	});

	return hit;
}
