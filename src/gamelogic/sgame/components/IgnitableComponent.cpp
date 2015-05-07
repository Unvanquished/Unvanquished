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

const float IgnitableComponent::SELF_DAMAGE          = 10.0f;
const float IgnitableComponent::SPLASH_DAMAGE        = 20.0f;
const float IgnitableComponent::SPLASH_DAMAGE_RADIUS = 60.0f;
const int   IgnitableComponent::MIN_BURN_TIME        = 2500;
const int   IgnitableComponent::STOP_CHECK_TIME      = 2500;
const float IgnitableComponent::STOP_CHANCE          = 0.5f;
const float IgnitableComponent::STOP_RADIUS          = 150.0f;
const int   IgnitableComponent::SPREAD_CHECK_TIME    = 2500;
const float IgnitableComponent::SPREAD_RADIUS        = 120.0f;

IgnitableComponent::IgnitableComponent(Entity& entity, bool alwaysOnFire, ThinkingComponent& r_ThinkingComponent)
	: IgnitableComponentBase(entity, alwaysOnFire, r_ThinkingComponent)
	, onFire(alwaysOnFire)
	, igniteTime(alwaysOnFire ? level.time : 0)
	, immuneUntil(0)
	, fireStarter(nullptr) {
	REGISTER_THINKER(DamageSelf, ThinkingComponent::SCHEDULER_AVERAGE, 100);
	REGISTER_THINKER(DamageArea, ThinkingComponent::SCHEDULER_AVERAGE, 100);
	REGISTER_THINKER(ConsiderStop, ThinkingComponent::SCHEDULER_AVERAGE, STOP_CHECK_TIME);
	REGISTER_THINKER(ConsiderSpread, ThinkingComponent::SCHEDULER_AVERAGE, SPREAD_CHECK_TIME);
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
		fireLogger.Notice("Received ignite message with no fire starter.");
	}

	if (level.time < immuneUntil) {
		fireLogger.Debug("Not ignited: Immune against fire.");

		return;
	}

	// Refresh ignite time even if already burning.
	igniteTime = level.time;

	if (!onFire) {
		onFire = true;
		this->fireStarter = fireStarter;

		fireLogger.Debug("Ignited.");
	} else {
		if (alwaysOnFire && !this->fireStarter) {
			// HACK: Igniting an alwaysOnFire entity will initialize the fire starter.
			this->fireStarter = fireStarter;

			fireLogger.Debug("Firestarter initialized.");
		} else {
			fireLogger.Debug("Re-Ignited.");
		}
	}
}

void IgnitableComponent::HandleExtinguish(int immunityTime) {
	if (!onFire) return;

	onFire = false;
	immuneUntil = level.time + immunityTime;

	if (alwaysOnFire) {
		entity.FreeAt(DeferredFreeingComponent::FREE_BEFORE_THINKING);
	}

	fireLogger.Debug("Extinguished.");
}

void IgnitableComponent::DamageSelf(int timeDelta) {
	if (!onFire) return;

	float damage = SELF_DAMAGE * timeDelta * 0.001f;

	if (entity.Damage(damage, fireStarter, {}, {}, 0, MOD_BURN)) {
		fireLogger.Debug("Self burn damage of %.1f (%.1f/s) was dealt.", damage, SELF_DAMAGE);
	}
}

void IgnitableComponent::DamageArea(int timeDelta) {
	if (!onFire) return;

	float damage = SPLASH_DAMAGE * timeDelta * 0.001f;

	if (G_SelectiveRadiusDamage(entity.oldEnt->s.origin, fireStarter, damage, SPLASH_DAMAGE_RADIUS,
			entity.oldEnt, MOD_BURN, TEAM_NONE)) {
		fireLogger.Debug("Area burn damage of %.1f (%.1f/s) was dealt.", damage, SPLASH_DAMAGE);
	}
}

void IgnitableComponent::ConsiderStop(int timeDelta) {
	if (!onFire) return;

	// Don't stop freshly (re-)ignited fires.
	if (igniteTime + MIN_BURN_TIME > level.time) {
		fireLogger.Debug("(Re-)Ignited %i ms ago, skipping stop check.", level.time - igniteTime);

		return;
	}

	float burnStopChance = STOP_CHANCE;

	// Lower burn stop chance if there are other burning entities nearby.
	ForEntities<IgnitableComponent>([&](Entity &other, IgnitableComponent &ignitable){
		if (&other == &entity) return;
		if (!ignitable.onFire) return;
		if (G_Distance(other.oldEnt, entity.oldEnt) > STOP_RADIUS) return;

		float frac = G_Distance(entity.oldEnt, other.oldEnt) / STOP_RADIUS;
		float mod  = frac * 1.0f + (1.0f - frac) * STOP_CHANCE;

		burnStopChance *= mod;
	});

	// Attempt to stop burning.
	if (random() < burnStopChance) {
		fireLogger.Debug("Stopped burning (chance was %.0f%%)", burnStopChance * 100.0f);

		entity.Extinguish(0);
		return;
	} else {
		fireLogger.Debug("Didn't stop burning (chance was %.0f%%)", burnStopChance * 100.0f);
	}
}

void IgnitableComponent::ConsiderSpread(int timeDelta) {
	if (!onFire) return;

	ForEntities<IgnitableComponent>([&](Entity &other, IgnitableComponent &ignitable){
		if (&other == &entity) return;

		// TODO: Use LocationComponent.
		float chance = 1.0f - G_Distance(entity.oldEnt, other.oldEnt) / SPREAD_RADIUS;

		if (chance <= 0.0f) return; // distance > spread radius

		if (random() < chance) {
			if (G_LineOfSight(entity.oldEnt, other.oldEnt) && other.Ignite(fireStarter)) {
				fireLogger.Debug("(Re-)Ignited a neighbour (chance was %.0f%%)", chance * 100.0f);
			} else {
				fireLogger.Debug("Tried to ignite a non-ignitable or non-LOS neighbour (chance was %.0f%%)",
								 chance * 100.0f);
			}
		} else {
			fireLogger.Debug("Didn't try to ignite a neighbour (chance was %.0f%%)", chance * 100.0f);
		}
	});
}
