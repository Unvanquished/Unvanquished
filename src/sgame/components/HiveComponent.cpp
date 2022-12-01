#include "HiveComponent.h"
#include "../Entities.h"

#include <glm/geometric.hpp>
constexpr float SENSE_RANGE   = HIVE_SENSE_RANGE; // Also needed by cgame.
constexpr int   ATTACK_PERIOD = 3000;

HiveComponent::HiveComponent(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent)
	: HiveComponentBase(entity, r_AlienBuildableComponent)
	, insectsReady(true)
	, insectsActiveSince(-ATTACK_PERIOD)
{
	GetAlienBuildableComponent().GetBuildableComponent().REGISTER_THINKER(
		Think, ThinkingComponent::SCHEDULER_AVERAGE, 500
	);
}

void HiveComponent::HandleDamage(
	float /*amount*/, gentity_t* source, Util::optional<glm::vec3> /*location*/, Util::optional<glm::vec3> /*direction*/,
	int /*flags*/, meansOfDeath_t /*meansOfDeath*/)
{
	if (!GetAlienBuildableComponent().GetBuildableComponent().Active()) {
		return;
	}

	// If insects are ready and the damage source is a valid target, fire on the latter even if it's
	// out of sense range.
	if (insectsReady && source && TargetValid(*source->entity, false)) {
		Fire(*source->entity);
	}
}

void HiveComponent::Think(int /*timeDelta*/) {
	if (insectsActiveSince + ATTACK_PERIOD < level.time) {
		insectsReady = true;
	}

	if (!insectsReady) {
		return;
	}

	if (!GetAlienBuildableComponent().GetBuildableComponent().Active()) {
		return;
	}

	Entity* target = FindTarget();

	if (target) {
		Fire(*target);
	}
}

Entity* HiveComponent::FindTarget() {
	Entity* target = nullptr;

	for (Entity& candidate : Entities::Having<HumanClassComponent>()) {
		// Check if target is valid and in sense range.
		if (!TargetValid(candidate, true)) continue;

		// Check if better target.
		if (!target || CompareTargets(candidate, *target)) {
			target = &candidate;
		}
	}

	return target;
}

bool HiveComponent::TargetValid(Entity& candidate, bool checkDistance) const {
	// Only target human clients. Note that this will always passed when called from within
	// FindTarget but it may fail when called from within HandleDamage.
	if (!candidate.Get<HumanClassComponent>()) return false;

	// Do not target the dead.
	if (Entities::IsDead(candidate)) return false;

	// Respect the no-target flag.
	if ((candidate.oldEnt->flags & FL_NOTARGET)) return false;

	// If asked to, check if target is in sense range.
	if (checkDistance && G_Distance(entity.oldEnt, candidate.oldEnt) > SENSE_RANGE) return false;

	// Check for line of sight.
	if (!G_LineOfFire(entity.oldEnt, candidate.oldEnt)) return false;

	return true;
}

bool HiveComponent::CompareTargets(Entity& a, Entity& b) const {
	float aDistance = G_Distance(entity.oldEnt, a.oldEnt);
	float bDistance = G_Distance(entity.oldEnt, b.oldEnt);

	// Prefer the one in attack range.
	if (aDistance < SENSE_RANGE && bDistance > SENSE_RANGE) return true;
	if (aDistance > SENSE_RANGE && bDistance < SENSE_RANGE) return false;

	// TODO: Prefer target that is targeted by fewer hives.

	// Break tie by random choice.
	return rand() % 2 ? true : false;
}

void HiveComponent::Fire(Entity& target) {
	insectsReady       = false;
	insectsActiveSince = level.time;

	glm::vec3 muzzle      = VEC2GLM( entity.oldEnt->s.pos.trBase );
	glm::vec3 targetPoint = VEC2GLM( target.oldEnt->s.origin );
	glm::vec3 dirToTarget = glm::normalize( targetPoint - muzzle );

	vectoangles( &dirToTarget[0], entity.oldEnt->buildableAim );
	entity.oldEnt->target = target.oldEnt;

	G_FireWeapon(entity.oldEnt, WP_HIVE, WPM_PRIMARY);

	G_SetBuildableAnim(entity.oldEnt, BANIM_ATTACK1, false);
}
