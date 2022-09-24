#include "CorrodibleComponent.h"

constexpr int CORRODE_TIME = 30000;
constexpr float CORRODE_DAMAGE = 5.0f;

CorrodibleComponent::CorrodibleComponent(Entity& entity, ThinkingComponent& r_ThinkingComponent)
	: CorrodibleComponentBase(entity, r_ThinkingComponent),
	  corrode_(false),
	  corrodeTime_(0) {
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
		corrodeTime_ = level.time;
	}
}

void CorrodibleComponent::Damage(int timeDelta) {
	if (!corrode_) return;
	if (corrodeTime_ + CORRODE_TIME < level.time) {
		HandleCorrode(false);
		return;
	}
	entity.Damage(CORRODE_DAMAGE, nullptr, Util::nullopt, Util::nullopt, 0, MOD_CORRODE);
}
