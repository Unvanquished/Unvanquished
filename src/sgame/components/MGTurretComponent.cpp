#include "MGTurretComponent.h"

constexpr float ATTACK_RANGE         = (float)MGTURRET_RANGE; // cgame needs to know this.
constexpr int   ATTACK_PERIOD        = MGTURRET_ATTACK_PERIOD; // cgame needs to know this.
constexpr float MAX_DAMAGE           = 4.0f;
constexpr float MIN_DAMAGE           = 2.0f;
constexpr int   TARGET_SEARCH_PERIOD = 500;

MGTurretComponent::MGTurretComponent(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent, TurretComponent& r_TurretComponent)
	: MGTurretComponentBase(entity, r_HumanBuildableComponent, r_TurretComponent)
	, firing(false)
	, lastTargetSearch(-TARGET_SEARCH_PERIOD)
	, lastShot(-ATTACK_PERIOD)
{
	GetTurretComponent().SetRange(ATTACK_RANGE);

	REGISTER_THINKER(Think, ThinkingComponent::SCHEDULER_BEFORE, 0);
}

void MGTurretComponent::HandlePrepareNetCode() {
	ModifyFlag(entity.oldEnt->s.eFlags, EF_FIRING, firing);
}

void MGTurretComponent::HandlePowerUp() {
	// Raise head.
	GetTurretComponent().ResetPitch();
}

void MGTurretComponent::HandlePowerDown() {
	// Forget about a target so a new one is acquired after power-up.
	GetTurretComponent().RemoveTarget();

	// Lower head.
	GetTurretComponent().LowerPitch();
}

void MGTurretComponent::HandleDie(gentity_t* /*killer*/, meansOfDeath_t /*meansOfDeath*/) {
	// Lower head.
	GetTurretComponent().LowerPitch();
}

void MGTurretComponent::Think(int timeDelta) {
	// Reset firing flag for now.
	firing = false;

	if (!GetHumanBuildableComponent().GetBuildableComponent().Active()) {
		BuildableComponent::lifecycle_t state = GetHumanBuildableComponent().GetBuildableComponent().GetState();

		// Still allow head movement if powered down or dying.
		if (state == BuildableComponent::CONSTRUCTED || state == BuildableComponent::PRE_BLAST) {
			GetTurretComponent().MoveHeadToTarget(timeDelta);
		}

		return;
	}

	// Look for a new entity target if there isn't a valid one already.
	if (lastTargetSearch + TARGET_SEARCH_PERIOD <= level.time &&
	    !GetTurretComponent().TargetValid()) {

		GetTurretComponent().FindEntityTarget(
			[this](Entity& a, Entity& b)->bool{ return this->CompareTargets(a, b); }
		);

		lastTargetSearch = level.time;
	}

	if (GetTurretComponent().TargetValid()) {
		// Our targets are clients that can move, so track their position.
		GetTurretComponent().TrackEntityTarget();

		// Shoot as soon as the target can be hit.
		if (GetTurretComponent().TargetCanBeHit()) {
			// If the target origin is visible, aim for it first.
			if (G_LineOfFire(entity.oldEnt, GetTurretComponent().GetTarget()->oldEnt)) {
				GetTurretComponent().MoveHeadToTarget(timeDelta);
			}

			if (lastShot + ATTACK_PERIOD <= level.time) {
				Shoot();
			}

			firing = true;
		} else {
			GetTurretComponent().MoveHeadToTarget(timeDelta);
		}
	} else {
		// Move head towards a non-entity target.
		GetTurretComponent().MoveHeadToTarget(timeDelta);
	}
}

bool MGTurretComponent::CompareTargets(Entity& a, Entity& b) {
	// TODO: Prefer target that isn't yet targeted.
	// This makes group attacks more and dretch spam less efficient.

	// Prefer the target that is in a line of sight.
	// This prevents the turret from keeping a lock on a target behind cover when there is another
	// enemy in reach that is not yet targeted.
	bool canSeeA = G_LineOfFire(entity.oldEnt, a.oldEnt);
	bool canSeeB = G_LineOfFire(entity.oldEnt, b.oldEnt);

	if        ( canSeeA && !canSeeB) {
		return true;
	} else if (!canSeeA &&  canSeeB) {
		return false;
	}

	// Prefer target that can be aimed at more quickly.
	// This makes the turret spend less time tracking enemies.
	vec3_t aimDirectionOldVec;
	AngleVectors(entity.oldEnt->buildableAim, aimDirectionOldVec, nullptr, nullptr);
	Vec3 aimDirection = Vec3::Load(aimDirectionOldVec);

	Vec3 directionToA = Math::Normalize(
		Vec3::Load(a.oldEnt->s.origin) - Vec3::Load(entity.oldEnt->s.pos.trBase)
	);

	Vec3 directionToB = Math::Normalize(
		Vec3::Load(b.oldEnt->s.origin) - Vec3::Load(entity.oldEnt->s.pos.trBase)
	);

	return (Math::Dot(directionToA, aimDirection) > Math::Dot(directionToB, aimDirection));
}

void MGTurretComponent::Shoot() {
	Entity* target = GetTurretComponent().GetTarget();

	ASSERT(target);

	float damageMod = 1.0f - Math::Clamp(
		G_Distance(entity.oldEnt, target->oldEnt) / ATTACK_RANGE, 0.0f, 1.0f
	);

	float damage = MIN_DAMAGE + (MAX_DAMAGE - MIN_DAMAGE) * damageMod;

	entity.oldEnt->turretCurrentDamage = damage;
	G_AddEvent(entity.oldEnt, EV_FIRE_WEAPON, 0);
	G_FireWeapon(entity.oldEnt, WP_MGTURRET, WPM_PRIMARY);

	lastShot = level.time;
}
