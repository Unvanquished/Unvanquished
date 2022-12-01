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
#include "../Entities.h"

static Log::Logger fireLogger("sgame.fire");

const float IgnitableComponent::SELF_DAMAGE             = 12.5f;
const float IgnitableComponent::SPLASH_DAMAGE           = 20.0f;
const int   IgnitableComponent::MIN_BURN_TIME           = 4000;
const int   IgnitableComponent::BASE_AVERAGE_BURN_TIME  = 8000;
const int   IgnitableComponent::EXTRA_AVERAGE_BURN_TIME = 5000;
const float IgnitableComponent::EXTRA_BURN_TIME_RADIUS  = 150.0f;
const float IgnitableComponent::SPREAD_RADIUS           = 120.0f;

static_assert(IgnitableComponent::BASE_AVERAGE_BURN_TIME > IgnitableComponent::MIN_BURN_TIME,
              "Average burn time needs to be greater than minimum burn time.");

IgnitableComponent::IgnitableComponent(Entity& entity, bool alwaysOnFire, ThinkingComponent& r_ThinkingComponent)
	: IgnitableComponentBase(entity, alwaysOnFire, r_ThinkingComponent)
	, onFire(alwaysOnFire)
	, igniteTime(alwaysOnFire ? level.time : 0)
	, immuneUntil(0)
	, spreadAt(INT_MAX)
	, fireStarter(nullptr)
	, randomGenerator(rand()) // TODO: Have one PRNG for all of sgame.
	, normalDistribution(0.0f, (float)BASE_AVERAGE_BURN_TIME) {
	REGISTER_THINKER(DamageSelf, ThinkingComponent::SCHEDULER_AVERAGE, 100);
	REGISTER_THINKER(DamageArea, ThinkingComponent::SCHEDULER_AVERAGE, 100);
	REGISTER_THINKER(ConsiderStop, ThinkingComponent::SCHEDULER_AVERAGE, 500);
	REGISTER_THINKER(ConsiderSpread, ThinkingComponent::SCHEDULER_AVERAGE, 500);
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

	// Start burning on initial ignition.
	if (!onFire) {
		onFire = true;
		this->fireStarter = fireStarter;

		fireLogger.Notice("Ignited.");
	} else {
		if (alwaysOnFire && !this->fireStarter) {
			// HACK: Igniting an alwaysOnFire entity will initialize the fire starter.
			this->fireStarter = fireStarter;

			fireLogger.Debug("Firestarter set.");
		} else {
			fireLogger.Debug("Re-ignited.");
		}
	}

	// Refresh ignite time even if already burning.
	igniteTime = level.time;

	// The spread delay follows a normal distribution: More likely to spread early than late.
	int spreadTarget = level.time + (int)std::abs(normalDistribution(randomGenerator));

	// Allow re-ignition to update the spread delay to a lower value.
	if (spreadTarget < spreadAt) {
		fireLogger.DoNoticeCode([&]{
			int newDelay = spreadTarget - level.time;
			if (spreadAt == INT_MAX) {
				fireLogger.Notice("Spread delay set to %.1fs.", newDelay * 0.001f);
			} else {
				int oldDelay = spreadAt - level.time;
				fireLogger.Notice("Spread delay updated from %.1fs to %.1fs.",
				                  oldDelay * 0.001f, newDelay * 0.001f);
			}
		});
		spreadAt = spreadTarget;
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

	if (G_SelectiveRadiusDamage(entity.oldEnt->s.origin, fireStarter, damage, FIRE_DAMAGE_RADIUS,
			entity.oldEnt, MOD_BURN, TEAM_NONE)) {
		fireLogger.Debug("Area burn damage of %.1f (%.1f/s) was dealt.", damage, SPLASH_DAMAGE);
	}
}

void IgnitableComponent::ConsiderStop(int timeDelta) {
	if (!onFire) return;

	// Don't stop freshly (re-)ignited fires.
	if (igniteTime + MIN_BURN_TIME > level.time) {
		fireLogger.DoDebugCode([&]{
			int elapsed   = level.time - igniteTime;
			int remaining = MIN_BURN_TIME - elapsed;
			fireLogger.Debug("Burning for %.1fs, skipping stop check for another %.1fs.",
			                 (float)elapsed/1000.0f, (float)remaining/1000.0f);
		});

		return;
	}

	float averagePostMinBurnTime = BASE_AVERAGE_BURN_TIME - MIN_BURN_TIME;

	// Increase average burn time dynamically for burning entities in range.
	for (IgnitableComponent& ignitable : Entities::Each<IgnitableComponent>()) {
		if (&ignitable == this) continue;
		if (!ignitable.onFire) continue;

		// TODO: Use LocationComponent.
		float distance = G_Distance(ignitable.entity.oldEnt, entity.oldEnt);

		if (distance > EXTRA_BURN_TIME_RADIUS) continue;

		float distanceFrac = distance / EXTRA_BURN_TIME_RADIUS;
		float distanceMod  = 1.0f - distanceFrac;

		averagePostMinBurnTime += EXTRA_AVERAGE_BURN_TIME * distanceMod;
	}

	// The burn stop chance follows an exponential distribution.
	float lambda = 1.0f / averagePostMinBurnTime;
	float burnStopChance = 1.0f - std::exp(-1.0f * lambda * (float)timeDelta);

	float averageTotalBurnTime = averagePostMinBurnTime + (float)MIN_BURN_TIME;

	// Attempt to stop burning.
	if (random() < burnStopChance) {
		fireLogger.Notice("Stopped burning after %.1fs, target average lifetime was %.1fs.",
		                  (float)(level.time - igniteTime) / 1000.0f, averageTotalBurnTime / 1000.0f);

		entity.Extinguish(0);
		return;
	} else {
		fireLogger.Debug("Burning for %.1fs, target average lifetime is %.1fs.",
		                 (float)(level.time - igniteTime) / 1000.0f, averageTotalBurnTime / 1000.0f);
	}
}

void IgnitableComponent::ConsiderSpread(int /*timeDelta*/) {
	if (!onFire) return;
	if (level.time < spreadAt) return;

	fireLogger.Notice("Trying to spread.");

	for (Entity& other : Entities::Having<IgnitableComponent>()) {
		if (&other == &entity) continue;

		// Don't re-ignite.
		if (other.Get<IgnitableComponent>()->onFire) continue;

		// TODO: Use LocationComponent.
		float distance = G_Distance(other.oldEnt, entity.oldEnt);

		if (distance > SPREAD_RADIUS) continue;

		float distanceFrac = distance / SPREAD_RADIUS;
		float distanceMod  = 1.0f - distanceFrac;
		float spreadChance = distanceMod;

		if (random() < spreadChance) {
			if (G_LineOfSight(entity.oldEnt, other.oldEnt) && other.Ignite(fireStarter)) {
				fireLogger.Notice("Ignited a neighbour, chance to do so was %.0f%%.",
				                  spreadChance*100.0f);
			}
		}
	}

	// Don't spread again until re-ignited.
	spreadAt = INT_MAX;
}
