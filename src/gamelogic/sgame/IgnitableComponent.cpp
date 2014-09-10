/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2014 Unvanquished Developers

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

===========================================================================
*/

#include "IgnitableComponent.h"

static Log::Logger fireLogger("sgame.fire");

IgnitableComponent::IgnitableComponent(Entity &entity, bool alwaysOnFire) :
    IgnitableComponentBase(entity, alwaysOnFire), onFire(false), immuneUntil(0)
{}

void IgnitableComponent::HandlePrepareNetCode() {
	if (onFire) {
		entity.oldEnt->s.eFlags |= EF_B_ONFIRE;
	} else {
		entity.oldEnt->s.eFlags &= ~EF_B_ONFIRE;
	}
}

void IgnitableComponent::HandleIgnite(gentity_t* fireStarter) {
	if (level.time < immuneUntil) {
		fireLogger.DoDebugCode([&]{
			char selfDescr[64];
			if (onFire) {
				BG_BuildEntityDescription(selfDescr, sizeof(selfDescr), &entity.oldEnt->s);
				fireLogger.Debug("%s was immune against fire.", selfDescr);
			}
		});

		return;
	}

	// Start a new fire or refresh an existing one.
	if (not onFire) {
		// Start a new fire.
		onFire            = true;
		this->fireStarter = fireStarter;
		nextSelfDamage    = level.time + BURN_SELFDAMAGE_PERIOD * BURN_PERIODS_RAND_MOD;
		nextSplashDamage  = level.time + BURN_SPLDAMAGE_PERIOD  * BURN_PERIODS_RAND_MOD;
		nextStatusAction  = level.time + BURN_ACTION_PERIOD     * BURN_PERIODS_RAND_MOD;

		fireLogger.DoDebugCode([&]{
			char selfDescr[64], fireStarterDescr[64];
			BG_BuildEntityDescription(selfDescr, sizeof(selfDescr), &entity.oldEnt->s);
			BG_BuildEntityDescription(fireStarterDescr, sizeof(fireStarterDescr), &fireStarter->s);
			fireLogger.Debug("%s ignited %s.", fireStarterDescr, selfDescr);
		});
	} else {
		// Reset the status action timer to refresh an existing fire.
		// This leads to prolonged burning but slow spread in burning groups.
		nextStatusAction = level.time + BURN_ACTION_PERIOD * BURN_PERIODS_RAND_MOD;

		fireLogger.DoDebugCode([&]{
			char selfDescr[64], fireStarterDescr[64];
			BG_BuildEntityDescription(selfDescr, sizeof(selfDescr), &entity.oldEnt->s);
			BG_BuildEntityDescription(fireStarterDescr, sizeof(fireStarterDescr), &fireStarter->s);
			fireLogger.Debug("%s reset burning action timer of %s.", fireStarterDescr, selfDescr);
		});
	}
}

void IgnitableComponent::HandleExtinguish(int immunityTime) {
	fireLogger.DoDebugCode([&]{
		char selfDescr[64];
		if (onFire) {
			BG_BuildEntityDescription(selfDescr, sizeof(selfDescr), &entity.oldEnt->s);
			fireLogger.Debug("%s was extinquished.", selfDescr);
		}
	});

	onFire      = false;
	immuneUntil = level.time + immunityTime;

	if (alwaysOnFire) {
		entity.FreeAt(DeferedFreeingComponent::FREE_AFTER_GROUP_THINKING);
	}
}

void IgnitableComponent::HandleThink(int timeDelta) {
	if (not onFire) return;

	char descr[64] = {0};
	fireLogger.DoDebugCode([&]{
		BG_BuildEntityDescription(descr, sizeof(descr), &entity.oldEnt->s);
	});

	// Damage self.
	if (nextSelfDamage < level.time) {
		nextSelfDamage = level.time + BURN_SELFDAMAGE_PERIOD * BURN_PERIODS_RAND_MOD;

		// TODO: Replace with a damage message.
		if (entity.oldEnt->takedamage) {
			G_Damage( entity.oldEnt, entity.oldEnt, fireStarter, NULL, NULL, BURN_SELFDAMAGE, 0, MOD_BURN );
			fireLogger.Debug("%s took burn damage.", descr);
		}
	}

	// Damage close players.
	if (nextSplashDamage < level.time) {
		nextSplashDamage = level.time + BURN_SPLDAMAGE_PERIOD * BURN_PERIODS_RAND_MOD;

		bool hit = G_SelectiveRadiusDamage( entity.oldEnt->s.origin, fireStarter, BURN_SPLDAMAGE,
				BURN_SPLDAMAGE_RADIUS, entity.oldEnt, MOD_BURN, TEAM_NONE );

		if (hit) {
			fireLogger.Debug("%s dealt burn damage.", descr);
		}
	}

	// Evaluate chances to stop burning or ignite other ignitables.
	if (nextStatusAction < level.time) {
		nextStatusAction = level.time + BURN_ACTION_PERIOD * BURN_PERIODS_RAND_MOD;

		float burnStopChance = BURN_STOP_CHANCE;

		// Lower burn stop chance if there are other burning entities nearby.
		// TODO: Once there is a global list of all entity/component instances that use a given
		//       component, iterate over that instead. For now we just iterate over all (g)entities
		//       since component-local data structures aren't really the way it should be.
		gentity_t *otherOldEnt = NULL;
		while ((otherOldEnt = G_IterateEntitiesWithinRadius(otherOldEnt, entity.oldEnt->s.origin,
				BURN_STOP_RADIUS))) {
			Entity *otherEntity = otherOldEnt->entity;

			if (otherEntity == &entity) continue;

			IgnitableComponent *ignitableComponent = otherEntity->Get<IgnitableComponent>();

			if (!ignitableComponent || !ignitableComponent->onFire) continue;

			float frac = G_Distance(entity.oldEnt, otherOldEnt) / BURN_STOP_RADIUS;
			float mod  = frac * 1.0f + ( 1.0f - frac ) * BURN_STOP_CHANCE;

			burnStopChance *= mod;
		}

		// Attempt to stop burning.
		if (random() < burnStopChance) {
			fireLogger.Debug("%s has chance to stop burning of %.2f → stop",
			                 descr, burnStopChance);

			entity.Extinguish(0);

			return;
		} else {
			fireLogger.Debug("%s has chance to stop burning of %.2f → continue",
			                 descr, burnStopChance);
		}

		// Attempt to ignite close ignitables.
		// TODO: Once there is a global list of all entity/component instances that use a given
		//       component, iterate over that instead. For now we just iterate over all (g)entities
		//       since component-local data structures aren't really the way it should be.
		otherOldEnt = NULL;
		while ((otherOldEnt = G_IterateEntitiesWithinRadius(otherOldEnt, entity.oldEnt->s.origin,
				BURN_SPREAD_RADIUS))) {
			Entity *other = otherOldEnt->entity;

			if (other == &entity) continue;

			float chance = 1.0f - G_Distance(entity.oldEnt, otherOldEnt) / BURN_SPREAD_RADIUS;

			if (random() < chance && G_LineOfSight(entity.oldEnt, otherOldEnt)) {
				if (other->Ignite(fireStarter)) {
					fireLogger.Debug("%s has chance to ignite a neighbour of %.2f → try to ignite",
					                 descr, chance);
				} else {
					fireLogger.Debug("%s has chance to ignite a neighbour of %.2f → not ignitable",
					                 descr, chance);
				}
			} else {
				fireLogger.Debug("%s has chance to ignite a neighbour of %.2f → failed or no los",
				                 descr, chance);
			}
		}
	}
}
