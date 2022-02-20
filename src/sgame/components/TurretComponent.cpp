#include "TurretComponent.h"
#include "../Entities.h"

#include <glm/geometric.hpp>

static Log::Logger turretLogger( "sgame.turrets" );

static glm::vec3 DirectionToRelativeAngles( const glm::vec3 direction, const glm::vec3 torso );
static glm::vec3 RelativeAnglesToAbsoluteAngles( const glm::vec3 relativeAngles, const glm::vec3 torso );
static glm::vec3 RelativeAnglesToDirection( const glm::vec3 relativeAngles, const glm::vec3 torso );

constexpr float MINIMUM_CLEARANCE  = 120.0f;
constexpr float PITCH_CAP          = 30.0f;  /**< In degrees from horizon downwards. */
constexpr float PITCH_SPEED        = 160.0f; /**< In degrees per second. */
constexpr float YAW_SPEED          = 120.0f; /**< In degrees per second. */
constexpr int   GIVEUP_TARGET_TIME = 1000;

TurretComponent::TurretComponent( Entity& entity )
	: TurretComponentBase( entity )
	, m_target( nullptr )
	, m_range( FLT_MAX )
	, m_relativeAimAngles( 0.0f, 0.0f, 0.0f )
{
	SetBaseDirection();
	ResetDirection();
}

TurretComponent::~TurretComponent()
{
	// Make sure a target is removed eventually so tracked-by counters can be decreased.
	RemoveTarget();
}

void TurretComponent::HandlePrepareNetCode()
{
	// TODO: Make these angles completely relative to the turret's base, not just to its yaw.
	VectorCopy( m_relativeAimAngles.Data(), entity.oldEnt->s.angles2 );
}

void TurretComponent::SetRange( float range )
{
	m_range = range;
}

Entity* TurretComponent::GetTarget()
{
	return m_target ? m_target->entity : nullptr;
}

void TurretComponent::RemoveTarget()
{
	if ( m_target )
	{
		// TODO: Decrease tracked-by counter for the target.
		turretLogger.Verbose( "Target removed." );
	}

	m_target = nullptr;
	m_lastLineOfSightToTarget = 0;
}

bool TurretComponent::TargetValid()
{
	if ( !m_target )
	{
		return false;
	}

	return TargetValid( *m_target->entity, false );
}

Entity* TurretComponent::FindEntityTarget( std::function<bool( Entity&, Entity& )> CompareTargets )
{
	// Delete old target.
	RemoveTarget();

	// Search best target.
	// TODO: Iterate over all valid targets, do not assume they have to be clients.
	ForEntities<ClientComponent>( [&]( Entity& candidate, ClientComponent& ) {
		if ( TargetValid( candidate, true ) ) {
			if ( !m_target || CompareTargets( candidate, *m_target->entity ) ) {
				m_target = candidate.oldEnt;
			}
		}
	} );

	if ( m_target )
	{
		// TODO: Increase tracked-by counter for a new target.
		turretLogger.Verbose( "Target acquired." );
	}

	return m_target ? m_target->entity : nullptr;
}

bool TurretComponent::MoveHeadToTarget( int timeDelta )
{
	// Note that a timeDelta of zero may happen on a first thinker execution.
	// We do not return in that case since we don't know the return value yet.
	ASSERT_GE( timeDelta, 0 );

	float timeMod = ( float ) timeDelta / 1000.0f;

	// Compute maximum angle changes for this execution.
	Vec3 maxAngleChange;
	maxAngleChange[PITCH] = timeMod * PITCH_SPEED;
	maxAngleChange[YAW]   = timeMod * YAW_SPEED;
	maxAngleChange[ROLL]  = 0.0f;

	// Compute angles to target, relative to the turret's base.
	glm::vec3 dirToTarget;
	VectorCopy( m_directionToTarget.Data(), dirToTarget );
	glm::vec3 relativeAnglesToTarget = DirectionToRelativeAngles( dirToTarget, TorsoAngles() );

	// Compute difference between angles to target and current angles.
	Vec3 deltaAngles;
	AnglesSubtract( &relativeAnglesToTarget[0], m_relativeAimAngles.Data(), deltaAngles.Data() );

	// Stop if there is nothing to do.
	if ( Math::Length( deltaAngles ) < 0.1f )
	{
		return true;
	}

	bool targetReached = true;
	Vec3 oldRelativeAimAngles = m_relativeAimAngles;

	// Adjust aim angles towards target angles.
	for ( int angle = 0; angle < 3; angle++ )
	{
		if ( angle == ROLL )
		{
			continue;
		}

		if ( fabs( deltaAngles[angle] ) > maxAngleChange[angle] )
		{
			m_relativeAimAngles[angle] += ( deltaAngles[angle] < 0.0f ) ? -maxAngleChange[angle] :  maxAngleChange[angle];
			targetReached = false;
		}
		else
		{
			m_relativeAimAngles[angle] = relativeAnglesToTarget[angle];
		}
	}

	// Respect pitch limits.
	if ( m_relativeAimAngles[PITCH] > PITCH_CAP )
	{
		m_relativeAimAngles[PITCH] = PITCH_CAP;
		targetReached = false;
	}

	if ( Math::DistanceSq( oldRelativeAimAngles, m_relativeAimAngles ) > 0.0f )
	{
		turretLogger.Debug( "Aiming. Elapsed: %d ms. Delta: %.2f. Max: %.2f. Old: %s. New: %s. Reached: %s.",
			timeDelta, deltaAngles, maxAngleChange, oldRelativeAimAngles, m_relativeAimAngles, targetReached );
	}

	// TODO: Move gentity_t.buildableAim to BuildableComponent.
	glm::vec3 absAim;
	VectorCopy( m_relativeAimAngles.Data(), absAim );
	VectorCopy( RelativeAnglesToAbsoluteAngles( absAim, TorsoAngles() ), entity.oldEnt->buildableAim );

	return targetReached;
}

void TurretComponent::TrackEntityTarget()
{
	if ( !m_target )
	{
		return;
	}

	Vec3 oldDirectionToTarget = m_directionToTarget;

	Vec3 targetOrigin = Vec3::Load( m_target->s.origin );
	Vec3 muzzle       = Vec3::Load( entity.oldEnt->s.pos.trBase );

	m_directionToTarget = Math::Normalize( targetOrigin - muzzle );

	if ( Math::DistanceSq( m_directionToTarget, oldDirectionToTarget ) > 0.0f )
	{
		turretLogger.Debug( "Following an entity target. New direction: %s.", m_directionToTarget );
	}
}

void TurretComponent::ResetDirection()
{
	m_directionToTarget = m_baseDirection;
	turretLogger.Verbose( "Target direction reset. New direction: %s.", m_directionToTarget );
}

void TurretComponent::ResetPitch()
{
	glm::vec3 targetRelativeAngles;
	VectorCopy( m_relativeAimAngles, targetRelativeAngles );
	targetRelativeAngles[PITCH] = 0.0f;
	VectorCopy( RelativeAnglesToDirection( targetRelativeAngles, TorsoAngles() ), m_directionToTarget );

	turretLogger.Debug( "Target pitch reset. New direction: %s.", m_directionToTarget );
}

void TurretComponent::LowerPitch()
{
	glm::vec3 targetRelativeAngles;
	VectorCopy( m_relativeAimAngles, targetRelativeAngles );
	targetRelativeAngles[PITCH] = PITCH_CAP;
	VectorCopy( RelativeAnglesToDirection( targetRelativeAngles, TorsoAngles() ), m_directionToTarget );

	turretLogger.Debug( "Target pitch lowered. New direction: %s.", m_directionToTarget );
}

bool TurretComponent::TargetCanBeHit()
{
	if ( !m_target )
	{
		return false;
	}

	glm::vec3 relAim;
	VectorCopy( m_relativeAimAngles, relAim );
	glm::vec3 aimDirection = RelativeAnglesToDirection( relAim, TorsoAngles() );
	glm::vec3 traceStart   = VEC2GLM( entity.oldEnt->s.pos.trBase );
	glm::vec3 traceEnd     = traceStart + m_range * aimDirection;

	trace_t tr;
	trap_Trace( &tr, &traceStart[0], nullptr, nullptr, &traceEnd[0], entity.oldEnt->s.number, MASK_SHOT, 0 );

	return tr.entityNum == m_target->s.number;
}

bool TurretComponent::TargetValid( Entity& m_target, bool newTarget )
{
	if ( !m_target.Get<ClientComponent>()
			|| m_target.Get<SpectatorComponent>()
			|| Entities::IsDead( m_target )
			|| ( m_target.oldEnt->flags & FL_NOTARGET )
			|| !Entities::OnOpposingTeams( entity, m_target )
			|| G_Distance( entity.oldEnt, m_target.oldEnt ) > m_range
			|| !trap_InPVS( entity.oldEnt->s.origin, m_target.oldEnt->s.origin ) )
	{

		if ( !newTarget )
		{
			turretLogger.Verbose( "Target lost: Out of range or eliminated." );
		}

		return false;
	}

	// New targets require a line of sight.
	if ( G_LineOfFire( entity.oldEnt, m_target.oldEnt ) )
	{
		m_lastLineOfSightToTarget = level.time;
	}
	else if ( newTarget )
	{
		return false;
	}

	// Give up on an existing target if there was no line of sight for a while.
	if ( m_lastLineOfSightToTarget + GIVEUP_TARGET_TIME <= level.time )
	{
		turretLogger.Verbose( "Giving up on target: No line of sight for %d ms.", level.time - m_lastLineOfSightToTarget );
		return false;
	}

	return true;
}

void TurretComponent::SetBaseDirection()
{
	vec3_t torsoDirectionOldVec;
	AngleVectors( entity.oldEnt->s.angles, torsoDirectionOldVec, nullptr, nullptr );

	glm::vec3 torsoDirection = glm::normalize( VEC2GLM( torsoDirectionOldVec ) );
	glm::vec3 traceStart     = VEC2GLM( entity.oldEnt->s.pos.trBase );
	glm::vec3 traceEnd       = traceStart + MINIMUM_CLEARANCE * torsoDirection;

	trace_t tr;
	trap_Trace( &tr, &traceStart[0], nullptr, nullptr, &traceEnd[0], entity.oldEnt->s.number, MASK_SHOT, 0 );

	// TODO: Check the presence of a PhysicsComponent to decide whether the obstacle is permanent.
	if ( tr.entityNum == ENTITYNUM_WORLD || g_entities[tr.entityNum].entity->Get<BuildableComponent>() )
	{
		VectorCopy( -torsoDirection, m_baseDirection );
	}
	else
	{
		VectorCopy( torsoDirection, m_baseDirection );
	}

	turretLogger.Verbose( "Base direction set to %s.", m_baseDirection );
}

glm::vec3 TurretComponent::TorsoAngles() const
{
	// HACK: This just works ( visually ) for turrets on even ground. The problem here is that
	//       entity.oldEnt->s.angles are only preliminary angles. The real angles of the turret
	//       model are calculated on the client side.
	return VEC2GLM( entity.oldEnt->s.angles );
}

glm::vec3 RelativeAnglesToAbsoluteAngles( const glm::vec3 relativeAngles, const glm::vec3 torso )
{
	quat_t torsoRotation;
	quat_t relativeRotation;
	quat_t absoluteRotation;
	vec3_t absoluteAngles;

	AnglesToQuat( &torso[0], torsoRotation );
	AnglesToQuat( &relativeAngles[0], relativeRotation );

	// Rotate by torso rotation in world space, then by relative orientation in torso space.
	// This is equivalent to rotating by relative orientation in world space, then by torso rotation
	// in world space. This then is equivalent to multiplying the torso rotation in world space on
	// the left hand side and the relative rotation in world space on the right hand side.
	QuatMultiply( torsoRotation, relativeRotation, absoluteRotation );

	QuatToAngles( absoluteRotation, absoluteAngles );

	return VEC2GLM( absoluteAngles );
}

glm::vec3 DirectionToRelativeAngles( const glm::vec3 direction, const glm::vec3 torso )
{
	vec3_t absAngles;
	vectoangles( &direction[0], absAngles );

	glm::vec3 absoluteAngles = VEC2GLM( absAngles );

	quat_t torsoRotation;
	quat_t absoluteRotation;
	quat_t relativeRotation;
	vec3_t relativeAngles;

	AnglesToQuat( &torso[0], torsoRotation );
	AnglesToQuat( &absoluteAngles[0], absoluteRotation );

	// This is the inverse of RelativeAnglesToAbsoluteAngles. See the comment there for details.
	quat_t inverseTorsoOrientation;
	QuatCopy( torsoRotation, inverseTorsoOrientation );
	QuatInverse( inverseTorsoOrientation );
	QuatMultiply( inverseTorsoOrientation, absoluteRotation, relativeRotation );

	QuatToAngles( relativeRotation, relativeAngles );

	return VEC2GLM( relativeAngles );
}

glm::vec3 RelativeAnglesToDirection( const glm::vec3 relativeAngles, const glm::vec3 torso )
{
	vec3_t direction;
	glm::vec3 angles = RelativeAnglesToAbsoluteAngles( relativeAngles, torso );
	AngleVectors( &angles[0], direction, nullptr, nullptr );
	return VEC2GLM( direction );
}
