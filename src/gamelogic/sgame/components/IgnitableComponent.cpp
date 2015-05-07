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

IgnitableComponent::IgnitableComponent(Entity& entity, bool freeOnExtinguish, ThinkingComponent& r_ThinkingComponent)
	: IgnitableComponentBase(entity, freeOnExtinguish, r_ThinkingComponent), onFire(freeOnExtinguish) {
	REGISTER_THINKER(DamageSelf, ThinkingComponent::SCHEDULER_AVERAGE, 100);
	REGISTER_THINKER(DamageArea, ThinkingComponent::SCHEDULER_AVERAGE, 100);
}

void IgnitableComponent::HandlePrepareNetCode() {
	if (onFire) {
		entity.oldEnt->s.eFlags |= EF_B_ONFIRE;
	} else {
		entity.oldEnt->s.eFlags &= ~EF_B_ONFIRE;
	}
}

void IgnitableComponent::HandleIgnite(gentity_t* fireStarter) {
	if (!fireStarter) {
		// TODO: Find out why this happens.
		fireLogger.Notice("Fire starter not known.");
		fireStarter = entity.oldEnt;
	}

	if (level.time < immuneUntil) {
		fireLogger.DoDebugCode([&]{
			char selfDescr[64];
			BG_BuildEntityDescription(selfDescr, sizeof(selfDescr), &entity.oldEnt->s);
			fireLogger.Debug("%s was immune against fire.", selfDescr);
		});

		return;
	}

	// Start a new fire or refresh an existing one.
	if (not onFire) {
		// Start a new fire.
		onFire            = true;
		this->fireStarter = fireStarter;
		nextStatusAction  = level.time + BURN_ACTION_PERIOD * BURN_PERIODS_RAND_MOD;

		fireLogger.DoNoticeCode([&]{
			char selfDescr[64], fireStarterDescr[64];
			BG_BuildEntityDescription(selfDescr, sizeof(selfDescr), &entity.oldEnt->s);
			BG_BuildEntityDescription(fireStarterDescr, sizeof(fireStarterDescr), &fireStarter->s);
			fireLogger.Notice("%s ignited %s.", fireStarterDescr, selfDescr);
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
	fireLogger.DoNoticeCode([&]{
		char selfDescr[64];
		if (onFire) {
			BG_BuildEntityDescription(selfDescr, sizeof(selfDescr), &entity.oldEnt->s);
			fireLogger.Notice("%s was extinquished.", selfDescr);
		}
	});

	onFire      = false;
	immuneUntil = level.time + immunityTime;

	if (freeOnExtinguish) {
		entity.FreeAt(DeferredFreeingComponent::FREE_BEFORE_THINKING);
	}
}

void IgnitableComponent::DamageSelf(int timeDelta) {
	if (!onFire) return;

	float damage = BURN_SELFDAMAGE * timeDelta * 0.001f;

	if (entity.Damage(damage, fireStarter, {}, {}, 0, MOD_BURN)) {
		fireLogger.Debug("Self burn damage of %.1f (%i/s) was dealt.", damage, BURN_SELFDAMAGE);
	}
}

void IgnitableComponent::DamageArea(int timeDelta) {
	if (!onFire) return;

	float damage = BURN_SPLDAMAGE * timeDelta * 0.001f;

	if (G_SelectiveRadiusDamage(entity.oldEnt->s.origin, fireStarter, damage,
		BURN_SPLDAMAGE_RADIUS, entity.oldEnt, MOD_BURN, TEAM_NONE)) {
		fireLogger.Debug("Area burn damage of %.1f (%i/s) was dealt.", damage, BURN_SPLDAMAGE);
	}
}

// TODO: Replace with thinking.
void IgnitableComponent::HandleFrame(int timeDelta) {
	if (not onFire) return;

	char descr[64] = {0};
	fireLogger.DoDebugCode([&]{
		BG_BuildEntityDescription(descr, sizeof(descr), &entity.oldEnt->s);
	});

	// Evaluate chances to stop burning or ignite other ignitables.
	if (nextStatusAction < level.time) {
		nextStatusAction = level.time + BURN_ACTION_PERIOD * BURN_PERIODS_RAND_MOD;

		float burnStopChance = BURN_STOP_CHANCE;

		// Lower burn stop chance if there are other burning entities nearby.
		ForEntities<IgnitableComponent>([&](Entity &other, IgnitableComponent &ignitable){
			if (&other == &entity) return;
			if (!ignitable.onFire) return;
			if (G_Distance(other.oldEnt, entity.oldEnt) > BURN_STOP_RADIUS) return;

			float frac = G_Distance(entity.oldEnt, other.oldEnt) / BURN_STOP_RADIUS;
			float mod  = frac * 1.0f + ( 1.0f - frac ) * BURN_STOP_CHANCE;

			burnStopChance *= mod;
		});

		// Attempt to stop burning.
		if (random() < burnStopChance) {
			fireLogger.Debug("%s stopped burning (chance was %.0f%%)", descr, burnStopChance * 100.0f);

			entity.Extinguish(0);
			return;
		} else {
			fireLogger.Debug("%s didn't stop burning (chance was %.0f%%)", descr, burnStopChance * 100.0f);
		}

		// Attempt to ignite close ignitables.
		ForEntities<IgnitableComponent>([&](Entity &other, IgnitableComponent &ignitable){
			if (&other == &entity) return;

			float chance = 1.0f - G_Distance(entity.oldEnt, other.oldEnt) / BURN_SPREAD_RADIUS;

			if (chance <= 0.0f) return; // distance > spread radius

			if (random() < chance) {
				if (G_LineOfSight(entity.oldEnt, other.oldEnt) && other.Ignite(fireStarter)) {
					fireLogger.Debug("%s (re-)ignited a neighbour (chance was %.0f%%)", descr, chance * 100.0f);
				} else {
					fireLogger.Debug("%s tried to ignite a non-ignitable or non-LOS neighbour (chance was %.0f%%)",
					                 descr, chance * 100.0f);
				}
			} else {
				fireLogger.Debug("%s didn't try to ignite a neighbour (chance was %.0f%%)",
				                 descr, chance * 100.0f);
			}
		});
	}
}
