#include "TurretComponent.h"
#include <glm/gtx/norm.hpp>
#include <glm/gtx/io.hpp>
#include "../Entities.h"

static Log::Logger turretLogger("sgame.turrets");

constexpr float MINIMUM_CLEARANCE  = 120.0f;
constexpr float PITCH_CAP          = 30.0f;  /**< In degrees from horizon downwards. */
constexpr float PITCH_SPEED        = 160.0f; /**< In degrees per second. */
constexpr float YAW_SPEED          = 120.0f; /**< In degrees per second. */
constexpr int   GIVEUP_TARGET_TIME = 1000;

TurretComponent::TurretComponent(Entity& entity)
	: TurretComponentBase(entity)
	, target(nullptr)
	, range(FLT_MAX)
	, relativeAimAngles(0.0f, 0.0f, 0.0f)
{
	SetBaseDirection();
	ResetDirection();
}

TurretComponent::~TurretComponent() {
	// Make sure a target is removed eventually so tracked-by counters can be decreased.
	RemoveTarget();
}

void TurretComponent::HandlePrepareNetCode() {
	// TODO: Make these angles completely relative to the turret's base, not just to its yaw.
	VectorCopy(relativeAimAngles, entity.oldEnt->s.angles2);
}

void TurretComponent::SetRange(float range) {
	this->range = range;
}

Entity* TurretComponent::GetTarget() {
	return target ? target->entity : nullptr;
}

void TurretComponent::RemoveTarget() {
	if (target) {
		// TODO: Decrease tracked-by counter for the target.

		turretLogger.Verbose("Target removed.");
	}

	target = nullptr;
	lastLineOfSightToTarget = 0;
}

bool TurretComponent::TargetValid() {
	if (!target) return false;

	return TargetValid(*target->entity, false);
}

Entity* TurretComponent::FindEntityTarget(std::function<bool(Entity&, Entity&)> CompareTargets) {
	// Delete old target.
	RemoveTarget();

	// Search best target.
	// TODO: Iterate over all valid targets, do not assume they have to be clients.
	for (Entity& candidate : Entities::Having<ClientComponent>()) {
		if (TargetValid(candidate, true)) {
			if (!target || CompareTargets(candidate, *target->entity)) {
				target = candidate.oldEnt;
			}
		}
	}

	if (target) {
		// TODO: Increase tracked-by counter for a new target.

		turretLogger.Verbose("Target acquired.");
	}

	return target ? target->entity : nullptr;
}

bool TurretComponent::MoveHeadToTarget(int timeDelta) {
	// Note that a timeDelta of zero may happen on a first thinker execution.
	// We do not return in that case since we don't know the return value yet.
	ASSERT_GE(timeDelta, 0);

	float timeMod = (float)timeDelta / 1000.0f;

	// Compute maximum angle changes for this execution.
	glm::vec3 maxAngleChange;
	maxAngleChange[PITCH] = timeMod * PITCH_SPEED;
	maxAngleChange[YAW]   = timeMod * YAW_SPEED;
	maxAngleChange[ROLL]  = 0.0f;

	// Compute angles to target, relative to the turret's base.
	glm::vec3 relativeAnglesToTarget = DirectionToRelativeAngles(directionToTarget);

	// Compute difference between angles to target and current angles.
	glm::vec3 deltaAngles;
	AnglesSubtract(&relativeAnglesToTarget[0], &relativeAimAngles[0], &deltaAngles[0]);

	// Stop if there is nothing to do.
	if (glm::length(deltaAngles) < 0.1f) {
		return true;
	}

	bool targetReached = true;
	glm::vec3 oldRelativeAimAngles = relativeAimAngles;

	// Adjust aim angles towards target angles.
	for (int angle = 0; angle < 3; angle++) {
		if (angle == ROLL) continue;

		if (fabs(deltaAngles[angle]) > maxAngleChange[angle]) {
			relativeAimAngles[angle] += (deltaAngles[angle] < 0.0f)
				? -maxAngleChange[angle]
				:  maxAngleChange[angle];
			targetReached = false;
		} else {
			relativeAimAngles[angle] = relativeAnglesToTarget[angle];
		}
	}

	// Respect pitch limits.
	if (relativeAimAngles[PITCH] > PITCH_CAP) {
		relativeAimAngles[PITCH] = PITCH_CAP;
		targetReached = false;
	}

	if (glm::distance2(oldRelativeAimAngles, relativeAimAngles) > 0.0f) {
		turretLogger.Debug(
			"Aiming. Elapsed: %d ms. Delta: %.2f. Max: %.2f. Old: %s. New: %s. Reached: %s.",
			timeDelta, deltaAngles, maxAngleChange, oldRelativeAimAngles, relativeAimAngles, targetReached
		);
	}

	// TODO: Move gentity_t.buildableAim to BuildableComponent.
	glm::vec3 absoluteAimAngles = RelativeAnglesToAbsoluteAngles(relativeAimAngles);
	VectorCopy(absoluteAimAngles, entity.oldEnt->buildableAim);

	return targetReached;
}

void TurretComponent::TrackEntityTarget() {
	if (!target) return;

	glm::vec3 oldDirectionToTarget = directionToTarget;

	glm::vec3 targetOrigin = VEC2GLM(target->s.origin);
	glm::vec3 muzzle       = VEC2GLM(entity.oldEnt->s.pos.trBase);

	directionToTarget = glm::normalize(targetOrigin - muzzle);

	if (glm::distance2(directionToTarget, oldDirectionToTarget) > 0.0f) {
		turretLogger.Debug("Following an entity target. New direction: %s.", directionToTarget);
	}
}

void TurretComponent::ResetDirection() {
	directionToTarget = baseDirection;

	turretLogger.Verbose("Target direction reset. New direction: %s.", directionToTarget);
}

void TurretComponent::ResetPitch() {
	glm::vec3 targetRelativeAngles = relativeAimAngles;
	targetRelativeAngles[PITCH] = 0.0f;

	directionToTarget = RelativeAnglesToDirection(targetRelativeAngles);

	turretLogger.Debug("Target pitch reset. New direction: %s.", directionToTarget);
}

void TurretComponent::LowerPitch() {
	glm::vec3 targetRelativeAngles = relativeAimAngles;
	targetRelativeAngles[PITCH] = PITCH_CAP;

	directionToTarget = RelativeAnglesToDirection(targetRelativeAngles);

	turretLogger.Debug("Target pitch lowered. New direction: %s.", directionToTarget);
}

bool TurretComponent::TargetCanBeHit() {
	if (!target) return false;

	glm::vec3 aimDirection = RelativeAnglesToDirection(relativeAimAngles);
	glm::vec3 traceStart   = VEC2GLM(entity.oldEnt->s.pos.trBase);
	glm::vec3 traceEnd     = traceStart + range * aimDirection;

	trace_t tr;
	trap_Trace(&tr, &traceStart[0], nullptr, nullptr, &traceEnd[0], entity.oldEnt->num(),
	           MASK_SHOT, 0);

	return (tr.entityNum == target->num());
}

bool TurretComponent::TargetValid(Entity& target, bool newTarget) {
	if (!target.Get<ClientComponent>() ||
	    target.Get<SpectatorComponent>() ||
	    Entities::IsDead(target) ||
	    (target.oldEnt->flags & FL_NOTARGET) ||
	    !Entities::OnOpposingTeams(entity, target) ||
	    G_Distance(entity.oldEnt, target.oldEnt) > range ||
	    !trap_InPVS(entity.oldEnt->s.origin, target.oldEnt->s.origin)) {

		if (!newTarget) {
			turretLogger.Verbose("Target lost: Out of range or eliminated.");
		}

		return false;
	}

	// New targets require a line of sight.
	if (G_LineOfFire(entity.oldEnt, target.oldEnt)) {
		lastLineOfSightToTarget = level.time;
	} else if (newTarget) {
		return false;
	}

	// Give up on an existing target if there was no line of sight for a while.
	if (lastLineOfSightToTarget + GIVEUP_TARGET_TIME <= level.time) {
		turretLogger.Verbose("Giving up on target: No line of sight for %d ms.",
			level.time - lastLineOfSightToTarget
		);

		return false;
	}

	return true;
}

void TurretComponent::SetBaseDirection() {
	vec3_t torsoDirectionOldVec;
	AngleVectors(entity.oldEnt->s.angles, torsoDirectionOldVec, nullptr, nullptr);

	glm::vec3 torsoDirection = glm::normalize(VEC2GLM(torsoDirectionOldVec));
	glm::vec3 traceStart     = VEC2GLM(entity.oldEnt->s.pos.trBase);
	glm::vec3 traceEnd       = traceStart + MINIMUM_CLEARANCE * torsoDirection;

	trace_t tr;
	trap_Trace(&tr, &traceStart[0], nullptr, nullptr, &traceEnd[0], entity.oldEnt->num(),
	           MASK_SHOT, 0);

	// TODO: Check the presence of a PhysicsComponent to decide whether the obstacle is permanent.
	if (tr.entityNum == ENTITYNUM_WORLD ||
	    g_entities[tr.entityNum].entity->Get<BuildableComponent>()) {
		baseDirection = -torsoDirection;
	} else {
		baseDirection =  torsoDirection;
	}

	turretLogger.Verbose("Base direction set to %s.", baseDirection);
}

glm::vec3 TurretComponent::TorsoAngles() const {
	// HACK: This just works (visually) for turrets on even ground. The problem here is that
	//       entity.oldEnt->s.angles are only preliminary angles. The real angles of the turret
	//       model are calculated on the client side.
	return VEC2GLM(entity.oldEnt->s.angles);
}

glm::vec3 TurretComponent::RelativeAnglesToAbsoluteAngles(const glm::vec3 relativeAngles) const {
	quat_t torsoRotation;
	quat_t relativeRotation;
	quat_t absoluteRotation;
	vec3_t absoluteAngles;

	AnglesToQuat(&TorsoAngles()[0], torsoRotation);
	AnglesToQuat(&relativeAngles[0], relativeRotation);

	// Rotate by torso rotation in world space, then by relative orientation in torso space.
	// This is equivalent to rotating by relative orientation in world space, then by torso rotation
	// in world space. This then is equivalent to multiplying the torso rotation in world space on
	// the left hand side and the relative rotation in world space on the right hand side.
	QuatMultiply(torsoRotation, relativeRotation, absoluteRotation);

	QuatToAngles(absoluteRotation, absoluteAngles);
	return VEC2GLM(absoluteAngles);
}

glm::vec3 TurretComponent::AbsoluteAnglesToRelativeAngles(const glm::vec3 absoluteAngles) const {
	quat_t torsoRotation;
	quat_t absoluteRotation;
	quat_t relativeRotation;
	vec3_t relativeAngles;

	AnglesToQuat(&TorsoAngles()[0], torsoRotation);
	AnglesToQuat(&absoluteAngles[0], absoluteRotation);

	// This is the inverse of RelativeAnglesToAbsoluteAngles. See the comment there for details.
	quat_t inverseTorsoOrientation;
	QuatCopy(torsoRotation, inverseTorsoOrientation);
	QuatInverse(inverseTorsoOrientation);
	QuatMultiply(inverseTorsoOrientation, absoluteRotation, relativeRotation);

	QuatToAngles(relativeRotation, relativeAngles);
	return VEC2GLM(relativeAngles);
}

glm::vec3 TurretComponent::DirectionToAbsoluteAngles(const glm::vec3 direction) const {
	vec3_t absoluteAngles;
	vectoangles(&direction[0], absoluteAngles);
	return VEC2GLM(absoluteAngles);
}

glm::vec3 TurretComponent::DirectionToRelativeAngles(const glm::vec3 direction) const {
	return AbsoluteAnglesToRelativeAngles(DirectionToAbsoluteAngles(direction));
}

glm::vec3 TurretComponent::AbsoluteAnglesToDirection(const glm::vec3 absoluteAngles) const {
	vec3_t direction;
	AngleVectors(&absoluteAngles[0], direction, nullptr, nullptr);
	return VEC2GLM(direction);
}

glm::vec3 TurretComponent::RelativeAnglesToDirection(const glm::vec3 relativeAngles) const {
	return AbsoluteAnglesToDirection(RelativeAnglesToAbsoluteAngles(relativeAngles));
}
