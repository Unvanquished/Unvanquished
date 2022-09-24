#include "CorrodibleComponent.h"

constexpr int CORRODE_TIME = 30000;
constexpr float CORRODE_DAMAGE = 5.0f;
constexpr int CORRODE_HEAL_RATE = 10000;

CorrodibleComponent::CorrodibleComponent(Entity& entity, ThinkingComponent& r_ThinkingComponent)
	: CorrodibleComponentBase(entity, r_ThinkingComponent),
	  corrode_(false),
	  corrodeEndTime_(0) {
	REGISTER_THINKER(Damage, ThinkingComponent::SCHEDULER_AVERAGE, 500);
}

void CorrodibleComponent::HandlePrepareNetCode() {
	if (corrode_) {
		entity.oldEnt->s.eFlags |= EF_B_CORRODE;
	} else {
		entity.oldEnt->s.eFlags &= ~EF_B_CORRODE;
	}
}

void CorrodibleComponent::HandleCorrode(bool c) {
	corrode_ = c;
	if (c) {
		corrodeEndTime_ = level.time + CORRODE_TIME;
	}
}

void CorrodibleComponent::HandleHeal(float amount, gentity_t* source) {
	// Make sure a player explicitly heals.
	if (!corrode_ || !source || !source->client) return;
	corrodeEndTime_ -= CORRODE_HEAL_RATE;
}

void CorrodibleComponent::Damage(int timeDelta) {
	if (!corrode_) return;
	if (corrodeEndTime_ < level.time) {
		HandleCorrode(false);
		return;
	}
	entity.Damage(CORRODE_DAMAGE, nullptr, Util::nullopt, Util::nullopt, 0, MOD_CORRODE);
}
