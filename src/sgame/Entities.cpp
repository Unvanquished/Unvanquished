/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2015 Unvanquished Developers

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

#include "Entities.h"
#include "CBSE.h"

bool Entities::OnSameTeam(Entity const &firstEntity, Entity const &secndEntity) {
	TeamComponent const* firstTeamComponent = firstEntity.Get<TeamComponent>();
	TeamComponent const* secndTeamComponent = secndEntity.Get<TeamComponent>();
	if (!firstTeamComponent || !secndTeamComponent) return false;
	return *firstTeamComponent == *secndTeamComponent;
}

bool Entities::OnOpposingTeams(Entity const &firstEntity, Entity const &secndEntity) {
	TeamComponent const* firstTeamComponent = firstEntity.Get<TeamComponent>();
	TeamComponent const* secndTeamComponent = secndEntity.Get<TeamComponent>();
	if (!firstTeamComponent || !secndTeamComponent) return false;
	return *firstTeamComponent != *secndTeamComponent;
}

bool Entities::IsAlive(Entity const& entity) {
	HealthComponent const* healthComponent = entity.Get<HealthComponent>();
	return (healthComponent && healthComponent->Alive());
}

bool Entities::IsAlive(gentity_t const*ent) {
	if (!ent) return false;
	return Entities::IsAlive(*ent->entity);
}

bool Entities::IsDead(Entity const &entity) {
	HealthComponent const* healthComponent = entity.Get<HealthComponent>();
	return (healthComponent && !healthComponent->Alive());
}

bool Entities::IsDead(gentity_t const *ent) {
	if (!ent) return false;
	return Entities::IsDead(*ent->entity);
}

void Entities::Kill(Entity& entity, Entity* source, meansOfDeath_t meansOfDeath) {
	HealthComponent *healthComponent = entity.Get<HealthComponent>();
	if (healthComponent) {
		entity.Damage(healthComponent->Health(), source ? source->oldEnt : nullptr, {}, {},
		              (DAMAGE_PURE | DAMAGE_NO_PROTECTION), meansOfDeath);
	}
}

void Entities::Kill(gentity_t *ent, meansOfDeath_t meansOfDeath) {
	if (ent) Entities::Kill(*ent->entity, nullptr, meansOfDeath);
}

void Entities::Kill(gentity_t *ent, gentity_t *source, meansOfDeath_t meansOfDeath) {
	if (!source) {
		Entities::Kill(ent, meansOfDeath);
	} else {
		if (ent) Entities::Kill(*ent->entity, source->entity, meansOfDeath);
	}
}

bool Entities::HasHealthComponent(gentity_t const* ent) {
	return (ent && HasComponents<HealthComponent>(*ent->entity));
}

float Entities::HealthOf(Entity const& entity) {
	HealthComponent const *healthComponent = entity.Get<HealthComponent>();
	ASSERT_NQ(healthComponent, nullptr);
	return healthComponent->Health();
}

float Entities::HealthOf(gentity_t const* ent) {
	return Entities::HealthOf(*ent->entity);
}

bool Entities::HasFullHealth(Entity const& entity) {
	HealthComponent const*healthComponent = entity.Get<HealthComponent>();
	ASSERT_NQ(healthComponent, nullptr);
	return healthComponent->FullHealth();
}

bool Entities::HasFullHealth(gentity_t const* ent) {
	return Entities::HasFullHealth(*ent->entity);
}

float Entities::HealthFraction(Entity const& entity) {
	HealthComponent const *healthComponent = entity.Get<HealthComponent>();
	ASSERT_NQ(healthComponent, nullptr);
	return healthComponent->HealthFraction();
}

float Entities::HealthFraction(gentity_t const* ent) {
	return Entities::HealthFraction(*ent->entity);
}

bool Entities::AntiHumanRadiusDamage(Entity& entity, float amount, float range, meansOfDeath_t mod) {
	bool hit = false;

	for (Entity& other : Entities::Having<HumanClassComponent>()) {
		// Abort early if they have notarget enabled.
		if (other.oldEnt->flags & FL_NOTARGET) continue;
		// TODO: Add LocationComponent.
		float distance = G_Distance(entity.oldEnt, other.oldEnt);
		float damage   = amount * (1.0f - 0.7f * distance / range);

		if (distance > range) continue;
		if (damage <= 0.0f) continue;
		if (!G_IsVisible(entity.oldEnt, other.oldEnt, MASK_SOLID)) continue;

		if (other.Damage(damage, entity.oldEnt, {}, {}, DAMAGE_NO_LOCDAMAGE, mod)) {
			hit = true;
		}
	}

	return hit;
}

bool Entities::KnockbackRadiusDamage(Entity& entity, float amount, float range, meansOfDeath_t mod) {
	bool hit = false;

	// FIXME: Only considering entities with HealthComponent.
	// TODO: Allow ForEntities to iterate over all entities.
	// NOTE: This will hurt entities with FL_NOTARGET enabled since it isn't really aiming at them.
	for (Entity& other : Entities::Having<HealthComponent>()) {
		// TODO: Add LocationComponent.
		float distance = G_Distance(entity.oldEnt, other.oldEnt);
		float damage   = amount * (1.0f - distance / range);

		if (damage <= 0.0f) continue;
		if (!G_IsVisible(entity.oldEnt, other.oldEnt, MASK_SOLID)) continue;

		glm::vec3 knockbackDir = VEC2GLM( other.oldEnt->s.origin ) - VEC2GLM( entity.oldEnt->s.origin );

		if (other.Damage(damage, entity.oldEnt, {}, knockbackDir, DAMAGE_NO_LOCDAMAGE | DAMAGE_KNOCKBACK, mod)) {
			hit = true;
		}
	}

	return hit;
}

namespace Entities {
namespace detail {
	ComponentBitset componentSets[NumComponents()];

	int ComponentBitset::Scan(int startSigned) const
	{
		auto start = static_cast<unsigned>(startSigned);
		// HACK: assume ENTITYNUM_NONE and ENTITYNUM_WORLD don't have components
		if (start >= static_cast<unsigned>(level.num_entities)) {
			return MAX_GENTITIES;
		}
		unsigned shift = start % wordBits;
		unsigned i = start / wordBits;
		word partialWord = data_[i] >> shift;
		if (partialWord != 0) {
			return CountTrailingZeroes(partialWord) + start;
		}
		unsigned iMax = (level.num_entities - 1) / wordBits;
		while (++i <= iMax) {
			if (data_[i] != 0) {
				return i * wordBits + CountTrailingZeroes(data_[i]);
			}
		}
		return MAX_GENTITIES;
	}

	void ComponentBitset::Set(int num)
	{
		data_[num / wordBits] |= word(1) << (num % wordBits);
	}

	void ComponentBitset::Clear(int num)
	{
		data_[num / wordBits] &= ~(word(1) << (num % wordBits));
	}
} // namespace detail
} // namespace Entities

void RegisterComponentCreate(int entityNum, int componentNum)
{
	Entities::detail::componentSets[componentNum].Set(entityNum);
}

void RegisterComponentDestroy(int entityNum, int componentNum)
{
	Entities::detail::componentSets[componentNum].Clear(entityNum);
}
