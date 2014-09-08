/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012-2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "g_local.h"
#include "IgnitableComponent.h"

static Log::Logger fireLogger("sgame.fire");

static std::set<IgnitableComponent*> ignitables;

IgnitableComponent::IgnitableComponent(Entity* entity, bool alwaysOnFire) :
    IgnitableComponentBase(entity, alwaysOnFire), onFire(false), fireImmunityUntil(0) {
	ignitables.insert(this);
}

IgnitableComponent::~IgnitableComponent() {
	ignitables.erase(this);
}

void IgnitableComponent::OnDoNetCode() {
	if (onFire) {
		entity->oldEnt->s.eFlags |= EF_B_ONFIRE;
	} else {
		entity->oldEnt->s.eFlags &= ~EF_B_ONFIRE;
	}
}

void IgnitableComponent::Ignite(gentity_t* _igniter) {
	if (level.time < fireImmunityUntil) {
		return;
	}

	// Start a new fire
	if (!onFire) {
		onFire               = true;
		igniter              = _igniter;
		nextBurnDamage       = level.time + BURN_SELFDAMAGE_PERIOD * BURN_PERIODS_RAND_MOD;
		nextBurnSplashDamage = level.time + BURN_SPLDAMAGE_PERIOD  * BURN_PERIODS_RAND_MOD;
		nextBurnAction       = level.time + BURN_ACTION_PERIOD     * BURN_PERIODS_RAND_MOD;

		fireLogger.DoDebugCode([&]{
				char selfDescr[64], fireStarterDescr[64];
				BG_BuildEntityDescription(selfDescr, sizeof(selfDescr), &entity->oldEnt->s);
				BG_BuildEntityDescription(fireStarterDescr, sizeof(fireStarterDescr), &igniter->s);
				fireLogger.Debug("%s ^2ignited^7 %s.", fireStarterDescr, selfDescr);
				});
	} else {
		fireLogger.DoDebugCode([&]{
				char selfDescr[64], fireStarterDescr[64];
				BG_BuildEntityDescription(selfDescr, sizeof(selfDescr), &entity->oldEnt->s);
				BG_BuildEntityDescription(fireStarterDescr, sizeof(fireStarterDescr), &igniter->s);
				fireLogger.Debug("%s ^2reset burning action timer of^7 %s.", fireStarterDescr, selfDescr);
				});

		// always reset the action timer
		// this leads to prolonged burning but slow spread in burning groups
		nextBurnAction = level.time + BURN_ACTION_PERIOD * BURN_PERIODS_RAND_MOD;
	}
}

void IgnitableComponent::PutOut(int immunityTime) {
	onFire = false;
	fireImmunityUntil = level.time + immunityTime;
}

bool IgnitableComponent::Think() {
	if (not onFire) {
		return false;
	}

	//assert(onFire or not alwaysOnFire);

	char descr[64] = {0};
	fireLogger.DoDebugCode([&]{
			BG_BuildEntityDescription(descr, sizeof(descr), &entity->oldEnt->s);
			});

	// Damage itself
	if (nextBurnDamage < level.time) {
		nextBurnDamage = level.time + BURN_SELFDAMAGE_PERIOD * BURN_PERIODS_RAND_MOD;

		// TODO replaced by a Damage message
		if (entity->oldEnt->takedamage) {
			G_Damage( entity->oldEnt, entity->oldEnt, igniter, NULL, NULL, BURN_SELFDAMAGE, 0, MOD_BURN );
			fireLogger.Debug("%s ^3took burn damage^7.", descr);
		}
	}

	// damage close players
	if (nextBurnSplashDamage < level.time) {
		nextBurnSplashDamage = level.time + BURN_SPLDAMAGE_PERIOD * BURN_PERIODS_RAND_MOD;

		bool hit = G_SelectiveRadiusDamage( entity->oldEnt->s.origin, igniter, BURN_SPLDAMAGE,
				BURN_SPLDAMAGE_RADIUS, entity->oldEnt, MOD_BURN, TEAM_NONE );

		if (hit) {
			fireLogger.Debug("%s ^8dealt burn damage^7.", descr);
		}
	}

	// evaluate chances to stop burning or ignite other ignitables
	if (nextBurnAction < level.time) {
		nextBurnAction = level.time + BURN_ACTION_PERIOD * BURN_PERIODS_RAND_MOD;

		float burnStopChance = BURN_STOP_CHANCE;

		// lower burn stop chance if there are other burning entities nearby
		for (auto& other: ignitables) {
			if (other == this or not other->onFire) {
				continue;
			}

			float distance = DistanceToIgnitable(other);

			if (distance > BURN_STOP_RADIUS) {
				continue;
			}
			//TODO: check for health > 0 ???
			float frac = distance / BURN_STOP_RADIUS;
			float mod  = frac * 1.0f + ( 1.0f - frac ) * BURN_STOP_CHANCE;

			burnStopChance *= mod;
		}

		// attempt to stop burning
		if (random() < burnStopChance) {
			onFire = false;

			fireLogger.Debug("%s has chance to stop burning of %.2f → ^1stop^7", descr, burnStopChance);

			// FIXME HACK: alwaysOnFire means that we should delete the entity when it is put off
			// it seems logical but to be completely honest, it was just the simplest way to detect ET_FIRE
			if (alwaysOnFire) {
				return true;
			}
		} else {
			fireLogger.Debug("%s has chance to stop burning of %.2f → ^2continue^7", descr, burnStopChance);
		}

		// attempt to ignite close ignitables
		for (auto& other: ignitables) {
			if (other == this or other->alwaysOnFire) {
				continue;
			}

			float distance = DistanceToIgnitable(other);

			if (distance > BURN_SPREAD_RADIUS) {
				continue;
			}

			float chance = 1.0f - distance / BURN_SPREAD_RADIUS;

			// TODO lineofsight will be in the physics component?
			if (random() < chance && G_LineOfSight(entity->oldEnt, other->entity->oldEnt)) {
				other->Ignite(igniter);
				fireLogger.Debug("%s has chance to ignite a neighbour of %.2f → ^2ignite^7", descr, chance);
			} else {
				fireLogger.Debug("%s has chance to ignite a neighbour of %.2f → failed or no los", descr, chance);
			}
		}
	}

	return false;
}

float IgnitableComponent::DistanceToIgnitable(IgnitableComponent* other) {
	return Distance(entity->oldEnt->s.origin, other->entity->oldEnt->s.origin);
}

void G_IgnitableThink() {
	std::vector<IgnitableComponent*> toDelete;
	for (auto& ignitable: ignitables) {
		bool needDeletion = ignitable->Think();
		if (needDeletion) {
			toDelete.push_back(ignitable);
		}
	}
	for (auto& ignitable: toDelete) {
		G_FreeEntity(ignitable->entity->oldEnt);
	}
}

