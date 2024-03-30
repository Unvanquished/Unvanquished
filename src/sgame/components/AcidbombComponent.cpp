#include "AcidbombComponent.h"

constexpr float ACIDBOMB_RANGE = 250.0f;

AcidbombComponent::AcidbombComponent(Entity& entity, TeamComponent& r_TeamComponent)
	: AcidbombComponentBase(entity, r_TeamComponent)
{}

void AcidbombComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	if (GetTeamComponent().Team() != G_Team(killer)) {
		Explode();
	}
}

void AcidbombComponent::Explode() {
	const vec3_t zero = {0.0f, 0.0f, 0.0f};
	G_SpawnMissile(MIS_ACIDBOMB, entity.oldEnt, entity.oldEnt->s.origin, zero, nullptr, G_ExplodeMissile, level.time);
	ForEntities<CorrodibleComponent>([&](Entity& other, CorrodibleComponent&) {
		if (G_Distance(entity.oldEnt, other.oldEnt) < ACIDBOMB_RANGE) {
			other.Corrode(entity.oldEnt, true);
		}
	});
}
