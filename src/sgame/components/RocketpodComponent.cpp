#include "RocketpodComponent.h"
#include "../Entities.h"

#include <glm/geometric.hpp>

constexpr float ATTACK_RANGE         = (float)ROCKETPOD_RANGE; // cgame needs to know this.
constexpr int   ATTACK_PERIOD        = ROCKETPOD_ATTACK_PERIOD; // cgame needs to know this.
constexpr int   SHUTTER_OPEN_TIME    = 1000;
constexpr int   TARGET_SEARCH_PERIOD = 500;
constexpr int   LOCKON_TIME          = 500;

RocketpodComponent::RocketpodComponent(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent, TurretComponent& r_TurretComponent)
	: RocketpodComponentBase(entity, r_HumanBuildableComponent, r_TurretComponent)
	, firing(false)
	, lockingOn(false)
	, safeMode(false)
	, lastTargetSearch(-TARGET_SEARCH_PERIOD)
	, openingShuttersSince(-SHUTTER_OPEN_TIME)
	, lockingOnSince(-LOCKON_TIME)
	, lastShot(-ATTACK_PERIOD)
{
	GetTurretComponent().SetRange(ATTACK_RANGE);

	REGISTER_THINKER(Think, ThinkingComponent::SCHEDULER_BEFORE, 0);
}

void RocketpodComponent::HandlePrepareNetCode() {
	ModifyFlag(entity.oldEnt->s.eFlags, EF_FIRING,   firing);
	ModifyFlag(entity.oldEnt->s.eFlags, EF_B_LOCKON, lockingOn);
}

void RocketpodComponent::HandlePowerUp() {
	// Raise head.
	GetTurretComponent().ResetPitch();

	if (safeMode) {
		// HACK: BuildableComponent just started the regular power-up (idle) animation, which opens
		//       the shutters and keeps them open. However, the shutters are supposed to stay shut
		//       in safe mode, so abort the power-up animation and reset the idle animation.
		// TODO: Consider adding an 'abort animation' helper.
		ToggleFlag(entity.oldEnt->s.legsAnim, ANIM_TOGGLEBIT);
		G_SetIdleBuildableAnim(entity.oldEnt, BANIM_IDLE_UNPOWERED);
	} else {
		openingShuttersSince = level.time;
	}
}

void RocketpodComponent::HandlePowerDown() {
	// Forget about a target.
	GetTurretComponent().RemoveTarget();

	// Lower head.
	GetTurretComponent().LowerPitch();

	if (safeMode) {
		// HACK: BuildableComponent just started the regular power-down animation, which closes the
		//       shutters. However, the shutters are already closed in safe mode, so abort the
		//       animation.
		// TODO: Consider adding an 'abort animation' helper.
		ToggleFlag(entity.oldEnt->s.legsAnim, ANIM_TOGGLEBIT);
	}
}

void RocketpodComponent::HandleDie(gentity_t* /*killer*/, meansOfDeath_t /*meansOfDeath*/) {
	if (safeMode && GetBuildableComponent().GetState() == BuildableComponent::PRE_BLAST) {

		// Overwrite animation set just before in BuildableComponent.
		G_SetBuildableAnim(entity.oldEnt, BANIM_DESTROY_UNPOWERED, true);
	}

	// Lower head.
	GetTurretComponent().LowerPitch();
}

void RocketpodComponent::Think(int timeDelta) {
	firing = false;

	if (!GetBuildableComponent().Active()) {
		BuildableComponent::lifecycle_t state = GetBuildableComponent().GetState();

		// Still allow head movement if powered down or dying.
		if (state == BuildableComponent::CONSTRUCTED || state == BuildableComponent::PRE_BLAST) {
			GetTurretComponent().MoveHeadToTarget(timeDelta);
		}

		lockingOn = false;
		return;
	}

	// If there is an enemy nearby, enter safe mode.
	SetSafeMode(EnemyClose());

	if (safeMode) {
		GetTurretComponent().MoveHeadToTarget(timeDelta);

		lockingOn = false;
		return;
	}

	// Do not move while opening shutters.
	if (openingShuttersSince + SHUTTER_OPEN_TIME > level.time) {
		lockingOn = false;
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

		if (GetTurretComponent().TargetCanBeHit()) {
			// If the target origin is visible, aim for it first.
			if (G_LineOfFire(entity.oldEnt, GetTurretComponent().GetTarget()->oldEnt)) {
				GetTurretComponent().MoveHeadToTarget(timeDelta);
			}

			bool safeShot = SafeShot();

			// Lock onto the target and shoot if lock was held long enough and it's safe to do so.
			if (lockingOn && safeShot && lockingOnSince + LOCKON_TIME <= level.time) {
				// The lockon timer has expired and it's safe to shoot, so do so.
				firing = true;

				if (lastShot + ATTACK_PERIOD <= level.time) {
					Shoot();
				}
			} else if (!lockingOn) {
				// Start lockon when the target can be hit, even if it's not safe to shoot yet.
				lockingOn      = true;
				lockingOnSince = level.time;
			} else if (!safeShot) {
				// Reset the lockon timer while it's not safe to shoot.
				lockingOnSince = level.time;
			} else {
				// The target can be hit safely but the lockon timer is running, so do nothing.
			}
		} else {
			// There is an entity target but it cannot yet be hit, so track it.
			GetTurretComponent().MoveHeadToTarget(timeDelta);

			lockingOn = false;
		}
	} else {
		// Move head towards a non-entity target.
		GetTurretComponent().MoveHeadToTarget(timeDelta);

		lockingOn = false;
	}
}

bool RocketpodComponent::CompareTargets(Entity &a, Entity &b) {
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

	// Prefer the target that is currently safe to shoot at.
	glm::vec3 directionToA = glm::normalize( VEC2GLM( a.oldEnt->s.origin ) - VEC2GLM( entity.oldEnt->s.pos.trBase ) );
	glm::vec3 directionToB = glm::normalize( VEC2GLM( b.oldEnt->s.origin ) - VEC2GLM( entity.oldEnt->s.pos.trBase ) );

	bool safeToShootAtA = SafeShot( entity.oldEnt->num(), VEC2GLM( entity.oldEnt->s.pos.trBase ), directionToA );
	bool safeToShootAtB = SafeShot( entity.oldEnt->num(), VEC2GLM( entity.oldEnt->s.pos.trBase ), directionToB );

	if ( safeToShootAtA && !safeToShootAtB)
	{
		return true;
	}
	else if ( !safeToShootAtA && safeToShootAtB )
	{
		return false;
	}

	// Prefer the target that is further away.
	float distanceToA = glm::length( VEC2GLM( a.oldEnt->s.origin ) - VEC2GLM( entity.oldEnt->s.pos.trBase ) );
	float distanceToB = glm::length( VEC2GLM( b.oldEnt->s.origin ) - VEC2GLM( entity.oldEnt->s.pos.trBase ) );

	return (distanceToA > distanceToB);
}

bool RocketpodComponent::SafeShot() {
	glm::vec3 aimDirection;
	AngleVectors( entity.oldEnt->buildableAim, &aimDirection[0], nullptr, nullptr );

	return SafeShot( entity.oldEnt->num(), VEC2GLM( entity.oldEnt->s.pos.trBase ), aimDirection );
}

bool RocketpodComponent::SafeShot(int passEntityNumber, const glm::vec3& origin, const glm::vec3& direction) {
	const missileAttributes_t* missileAttributes = BG_Missile(MIS_ROCKET);

	float missileSize = missileAttributes->size;

	glm::vec3 mins = {-missileSize, -missileSize, -missileSize};
	glm::vec3 maxs = { missileSize,  missileSize,  missileSize};
	glm::vec3 end  = origin + ATTACK_RANGE * direction;

	trace_t trace;
	trap_Trace( &trace, &origin[0], &mins[0], &maxs[0], &end[0], passEntityNumber, MASK_SHOT, 0 );

	// TODO: Refactor area damage (testing) helpers.
	return !G_RadiusDamage(
		trace.endpos, nullptr, missileAttributes->splashDamage, missileAttributes->splashRadius,
		nullptr, 0, MOD_ROCKETPOD, TEAM_HUMANS
	);
}

bool RocketpodComponent::EnemyClose() {
	const missileAttributes_t* missileAttributes = BG_Missile(MIS_ROCKET);

	for (Entity& other : Entities::Having<ClientComponent>()) {
		if (other.Get<SpectatorComponent>()) continue;
		if (Entities::IsDead(other)) continue;
		if (!Entities::OnOpposingTeams(entity, other)) continue;

		float distance = G_Distance(entity.oldEnt, other.oldEnt);

		classModelConfig_t* cmc = BG_ClassModelConfig(other.oldEnt->client->pers.classSelection);

		glm::vec3 turretMins, turretMaxs;
		BG_BuildableBoundingBox( BA_H_ROCKETPOD, &turretMins[0], &turretMaxs[0] );

		float enemyRadius   = std::max( glm::length( VEC2GLM( cmc->mins ) ), glm::length( VEC2GLM( cmc->maxs ) ) );
		float turretRadius  = std::max( glm::length( turretMins ), glm::length( turretMaxs ) );
		float missileRadius = missileAttributes->size;
		float splashRadius  = missileAttributes->splashRadius;

		// The center of explosion cannot be closer to own origin than this.
		float closestExplosionCenter = distance - (enemyRadius + missileRadius);

		// If the center of explosion is this far away from our bounding box, the former's splash
		// damage cannot reach into the latter.
		float safetyDistance = splashRadius + turretRadius;

		if (closestExplosionCenter < safetyDistance) {
			return true;
		}
	}

	return false;
}

void RocketpodComponent::Shoot() {
	Entity* target = GetTurretComponent().GetTarget();

	ASSERT(target);

	entity.oldEnt->target = target->oldEnt;
	G_AddEvent(entity.oldEnt, EV_FIRE_WEAPON, 0);
	G_FireWeapon(entity.oldEnt, WP_ROCKETPOD, WPM_PRIMARY);

	lastShot = level.time;
}

void RocketpodComponent::SetSafeMode(bool on) {
	bool wasOn = safeMode;

	if (on && !wasOn) {
		// Enabling safe mode.
		// If the rocketpod is powered, the shutters are open, so close them.
		// (Otherwise they are already closed.)
		if (GetBuildableComponent().Powered()) {
			G_SetBuildableAnim(entity.oldEnt, BANIM_POWERDOWN, false);
			G_SetIdleBuildableAnim(entity.oldEnt, BANIM_IDLE_UNPOWERED);
		}
	} else if (!on && wasOn) {
		// Disabling safe mode.
		// If the rocketpod is powered, the shutters are supposed to open.
		// (Otherwise they will open once the rocketpod regains power.)
		if (GetBuildableComponent().Powered()) {
			G_SetBuildableAnim(entity.oldEnt, BANIM_POWERUP, false);
			G_SetIdleBuildableAnim(entity.oldEnt, BANIM_IDLE1);
		}
	}

	safeMode = on;
}
