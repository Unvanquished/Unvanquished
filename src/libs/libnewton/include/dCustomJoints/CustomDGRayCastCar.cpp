// CustomDGRayCastCar.cpp: implementation of the CustomDGRayCastCar class.
// This raycast vehicle is currently a work in progress by Dave Gravel - 2009.
// Vehicle Raycast and convexCast Version 3.0b
//////////////////////////////////////////////////////////////////////
#include "CustomJointLibraryStdAfx.h"
#include "CustomDGRayCastCar.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define VEHICLE_MAX_TIRE_COUNT 16
#define DefPO (3.1416f / 180.0f)

CustomDGRayCastCar::CustomDGRayCastCar (int maxTireCount, const dMatrix& cordenateSytem, NewtonBody* carBody)
	:NewtonCustomJoint(2 * maxTireCount, carBody, NULL)
{
	dVector com;
	dMatrix tmp;

	m_vehicleOnAir = 0;
	m_fixDeceleration = 0.9975f;
	m_engineTireTorque = 0.0f;
	m_steerAngle = 0.0f;
	m_maxSteerAngle = 30.0f * DefPO;
	m_maxSteerRate = 0.075f;
	m_maxSteerForceRate = 0.03f;
	m_maxSteerSpeedRestriction = 2.0f;
	m_maxSteerForce = 6000.0f;
	m_maxBrakeForce = 350.0f;
	m_tiresRollSide = 0;
	m_maxTorque = 6000.0f;
	m_maxTorqueRate = 500.0f;
	m_engineSteerDiv = 100.0f;
	m_engineTorqueDiv = 200.0f; 
	// chassis rotation fix...
	m_chassisRotationLimit = 0.98f;
	// set the chassis matrix at the center of mass
	NewtonBodyGetCentreOfMass( m_body0, &com[0] );
	com.m_w = 1.0f;
	// set the joint reference point at the center of mass of the body
	dMatrix chassisMatrix ( cordenateSytem );
	chassisMatrix.m_posit += chassisMatrix.RotateVector( com );
	CalculateLocalMatrix ( chassisMatrix, m_localFrame, tmp );
	m_bodyChassisMatrix = chassisMatrix;

	// allocate space for the tires;
	m_tiresCount = 0;
	m_tires = new Tire[maxTireCount];

	m_curSpeed = 0.0f;
	m_aerodynamicDrag = 0.1f; 
	m_aerodynamicDownForce = 0.1f; 

	dFloat Ixx; 
	dFloat Iyy; 
	dFloat Izz; 
	NewtonBodyGetMassMatrix( m_body0, &m_mass, &Ixx, &Iyy, &Izz );
}

CustomDGRayCastCar::~CustomDGRayCastCar ()
{
	NewtonWorld *world;
	world = NewtonBodyGetWorld (m_body0);
	for ( int i = 0; i < m_tiresCount; i ++ ) {
		NewtonReleaseCollision ( world, m_tires[i].m_shape );
	}
	if( m_tires ) {
		delete[] m_tires;
	}
}

void CustomDGRayCastCar::SetVarChassisRotationLimit (dFloat value)
{
	m_chassisRotationLimit = value;
}

void CustomDGRayCastCar::SetVarFixDeceleration (dFloat value)
{
	m_fixDeceleration = value;
}

void CustomDGRayCastCar::SetVarMaxSteerAngle (dFloat value)
{
	m_maxSteerAngle = value * DefPO;
}

void CustomDGRayCastCar::SetVarMaxSteerRate (dFloat value)
{
	m_maxSteerRate = value;
}

void CustomDGRayCastCar::SetVarMaxSteerForceRate (dFloat value)
{
	m_maxSteerForceRate = value;
}

void CustomDGRayCastCar::SetVarMaxSteerForce (dFloat value)
{
	m_maxSteerForce = value;
}

void CustomDGRayCastCar::SetVarMaxSteerSpeedRestriction (dFloat value)
{
	m_maxSteerSpeedRestriction = value;
}

void CustomDGRayCastCar::SetVarMaxBrakeForce (dFloat value)
{
	m_maxBrakeForce = value;
}

void CustomDGRayCastCar::SetVarMaxTorque (dFloat value)
{
	m_maxTorque = value;
}

void CustomDGRayCastCar::SetVarMaxTorqueRate (dFloat value)
{
	m_maxTorqueRate = value;
}

void CustomDGRayCastCar::SetVarEngineSteerDiv (dFloat value)
{
	m_engineSteerDiv = value;
}

/*
void CustomDGRayCastCar::SetVarEngineTorqueDiv (dFloat value)
{
m_engineTorqueDiv = value;
}
*/

int CustomDGRayCastCar::GetTiresCount() const
{
	return m_tiresCount;
}

int CustomDGRayCastCar::GetVehicleOnAir() const
{
	return m_vehicleOnAir;
}

int CustomDGRayCastCar::GetTireOnAir(int index) const
{
	const Tire& tire = m_tires[index];	
	return tire.m_tireIsOnAir;
}

const NewtonCollision* CustomDGRayCastCar::GetTiresShape (int tireIndex) const
{
	const Tire& tire = m_tires[tireIndex];	
	return tire.m_shape;
}

void CustomDGRayCastCar::GetInfo (NewtonJointRecord* info) const
{
}

//this function is to be overloaded by a derive class
void CustomDGRayCastCar::SetSteering (dFloat angle)
{
}

//this function is to be overloaded by a derive class
void CustomDGRayCastCar::SetBrake (dFloat torque)
{
}

//this function is to be overloaded by a derive class
void CustomDGRayCastCar::SetTorque (dFloat torque)
{
}

dFloat CustomDGRayCastCar::GetSpeed() const
{
	return m_curSpeed;
}

void CustomDGRayCastCar::SetTireMaxRPS (int tireIndex, dFloat maxTireRPS)
{
	m_tires[tireIndex].m_maxTireRPS = maxTireRPS;
}

void CustomDGRayCastCar::SetVarTireSuspenssionHardLimit (int index, dFloat value)
{
	m_tires[index].m_suspenssionHardLimit = value;
}

void CustomDGRayCastCar::SetVarTireFriction (int index, dFloat value)
{
	m_tires[index].m_groundFriction = value;
}

CustomDGRayCastCar::Tire& CustomDGRayCastCar::GetTire (int index) const
{
	return m_tires[index];
}

dFloat CustomDGRayCastCar::GetParametricPosition (int index) const
{
	return m_tires[index].m_posit / m_tires[index].m_suspensionLenght;
}

void CustomDGRayCastCar::SetCustomTireSteerAngleForce (int index, dFloat angle, dFloat turnforce)
{
	m_tires[index].m_steerAngle = angle;
	m_tires[index].m_localAxis.m_z = dCos (angle);
	m_tires[index].m_localAxis.m_x = dSin (angle);
	if (m_tiresRollSide==0) {
		m_tires[index].m_turnforce = turnforce;
	} else {
		m_tires[index].m_turnforce = -turnforce;
	}
}

void CustomDGRayCastCar::SetCustomTireTorque (int index, dFloat torque)
{
	m_tires[index].m_torque = torque;
}

void CustomDGRayCastCar::SetCustomTireBrake (int index, dFloat torque)
{
	m_tires[index].m_breakTorque = torque;
	m_tires[index].m_torque = 0;
}

dFloat CustomDGRayCastCar::GenerateTiresTorque (dFloat value)
{
	dFloat speed;
	speed = dAbs( GetSpeed() );
	if ( value > 0.0f ) {
		m_engineTireTorque += m_maxTorqueRate; 
		if ( m_engineTireTorque > m_maxTorque ) {
			m_engineTireTorque = m_maxTorque;
		}
	} else 
		if ( value < 0.0f ) {
			m_engineTireTorque -= m_maxTorqueRate; 
			if ( m_engineTireTorque < -m_maxTorque ) {
				m_engineTireTorque = -m_maxTorque;
			}
		} else {
			if ( m_engineTireTorque > 0.0f ) {
				m_engineTireTorque -= m_maxTorqueRate;
				if ( m_engineTireTorque < 0.0f ) {
					m_engineTireTorque = 0.0f;
				}
			} else 
				if ( m_engineTireTorque < 0.0f ) {
					m_engineTireTorque += m_maxTorqueRate;
					if ( m_engineTireTorque > 0.0f ) {
						m_engineTireTorque = 0.0f;
					}
				} 
		}
		/*
		dFloat speed;
		speed = dAbs( GetSpeed() );
		dFloat nvalue = value;
		if ( nvalue < 0.0f ) {
		nvalue = m_engineTireTorque * ( 1.0f - speed / m_engineTorqueDiv );
		} else 
		if ( nvalue > 0.0f) {
		nvalue = -m_engineTireTorque * ( 1.0f - speed / m_engineTorqueDiv );
		} else {
		nvalue = 0.0f;
		}
		*/
		return -m_engineTireTorque;
}

dFloat CustomDGRayCastCar::GenerateTiresBrake (dFloat value)
{
	return value * m_maxBrakeForce; 
}

dFloat CustomDGRayCastCar::GenerateTiresSteerAngle (dFloat value)
{
	if ( value > 0.0f ) {
		m_steerAngle += m_maxSteerRate;
		if ( m_steerAngle > m_maxSteerAngle ) {
			m_steerAngle = m_maxSteerAngle;
		}
	} else 
		if ( value < 0.0f ) {
			m_steerAngle -= m_maxSteerRate;
			if ( m_steerAngle < -m_maxSteerAngle ) {
				m_steerAngle = -m_maxSteerAngle;
			}
		} else {
			if ( m_steerAngle > 0.0f ) {
				m_steerAngle -= m_maxSteerRate;
				if ( m_steerAngle < 0.0f ) {
					m_steerAngle = 0.0f;
				}
			} else 
				if ( m_steerAngle < 0.0f ) {
					m_steerAngle += m_maxSteerRate;
					if ( m_steerAngle > 0.0f ) {
						m_steerAngle = 0.0f;
					}
				} 
		}
		return m_steerAngle;
}

dFloat CustomDGRayCastCar::GenerateTiresSteerForce (dFloat value)
{
	dFloat speed, relspeed;
	relspeed = dAbs ( GetSpeed() ); 
	speed = relspeed;
	speed *= m_maxSteerForceRate;
	dFloat value2 = value;
	if ( speed > m_maxSteerSpeedRestriction ) {
		speed = m_maxSteerSpeedRestriction; 
	}
	if ( value2 > 0.0f ) {
		value2 = -( m_maxSteerForce * speed ) * ( 1.0f - relspeed / m_engineSteerDiv );
	} else 
		if ( value < 0.0f ) {
			value2 = ( m_maxSteerForce * speed ) * ( 1.0f - relspeed / m_engineSteerDiv );
		} else {
			value2 = 0.0f;
		}
		return value2;
}

void CustomDGRayCastCar::AddSingleSuspensionTire (
	void *userData,
	const dVector& localPosition, 
	dFloat mass,
	dFloat radius, 
	dFloat width,
	dFloat friction,
	dFloat suspensionLenght,
	dFloat springConst,
	dFloat springDamper,
	int castMode)
{
	m_tires[m_tiresCount].m_suspensionMatrix      = GetIdentityMatrix();
	m_tires[m_tiresCount].m_steerMatrix           = GetIdentityMatrix();
	m_tires[m_tiresCount].m_contactPoint          = dVector (0.0f, 0.0f, 0.0f, 1.0f);
	m_tires[m_tiresCount].m_tireAxelPosit         = dVector (0.0f, 0.0f, 0.0f, 1.0f);
	m_tires[m_tiresCount].m_localAxelPosit        = dVector (0.0f, 0.0f, 0.0f, 1.0f);
	m_tires[m_tiresCount].m_tireAxelVeloc         = dVector (0.0f, 0.0f, 0.0f, 1.0f);
	m_tires[m_tiresCount].m_lateralPin            = dVector (0.0f, 0.0f, 0.0f, 1.0f);
	m_tires[m_tiresCount].m_longitudinalPin       = dVector (0.0f, 0.0f, 0.0f, 1.0f);
	m_tires[m_tiresCount].m_tireSideForce         = dVector (0.0f, 0.0f, 0.0f, 1.0f);
	m_tires[m_tiresCount].m_TireForceTorque       = dVector (0.0f, 0.0f, 0.0f, 1.0f);
	m_tires[m_tiresCount].m_rayDestination        = dVector (0.0f, 0.0f, 0.0f, 1.0f);
	m_tires[m_tiresCount].m_HitBody               = NULL;
	m_tires[m_tiresCount].m_torque                = 0.0f;
	m_tires[m_tiresCount].m_sideHit               = 0;
	m_tires[m_tiresCount].m_isBraking             = 0;
	m_tires[m_tiresCount].m_tireIsOnAir           = 0;
	m_tires[m_tiresCount].m_tireSpeed             = 0.0f;
	m_tires[m_tiresCount].m_turnforce             = 0.0f;
	m_tires[m_tiresCount].m_suspenssionHardLimit  = radius * 0.5f;

	dVector relTirePos (localPosition);
	suspensionLenght = dAbs ( suspensionLenght );
	relTirePos += m_bodyChassisMatrix.m_up.Scale ( suspensionLenght );

	m_tires[m_tiresCount].m_harpoint              = m_localFrame.UntransformVector( relTirePos ); 
	m_tires[m_tiresCount].m_localAxis             = m_localFrame.UnrotateVector( dVector (0.0f, 0.0f, 1.0f, 0.0f) );
	m_tires[m_tiresCount].m_localAxis.m_w         = 0.0f;
	m_tires[m_tiresCount].m_userData              = userData;
	m_tires[m_tiresCount].m_angularVelocity       = 0.0f;
	m_tires[m_tiresCount].m_spinAngle             = 0.0f;
	m_tires[m_tiresCount].m_steerAngle            = 0.0f;	
	m_tires[m_tiresCount].m_tireWidth             = width;
	m_tires[m_tiresCount].m_posit                 = suspensionLenght;
	m_tires[m_tiresCount].m_suspensionLenght	  = suspensionLenght;	
	m_tires[m_tiresCount].m_tireLoad              = 0.0f;
	m_tires[m_tiresCount].m_breakTorque           = 0.0f;
	m_tires[m_tiresCount].m_localSuspentionSpeed  = 0.0f;
	m_tires[m_tiresCount].m_springConst           = springConst;
	m_tires[m_tiresCount].m_springDamper          = springDamper;
	m_tires[m_tiresCount].m_groundFriction        = friction;
	m_tires[m_tiresCount].m_MovePointForceFront	  = 0.0f;
	m_tires[m_tiresCount].m_MovePointForceUp	  = 0.0f;
	m_tires[m_tiresCount].m_MovePointForceRight   = 0.0f;
	m_tires[m_tiresCount].m_tireUseConvexCastMode = castMode; 

#define TIRE_SHAPE_SIZE 12
	dVector shapePoints[TIRE_SHAPE_SIZE * 2];
	for ( int i = 0; i < TIRE_SHAPE_SIZE; i ++ ) {
		shapePoints[i].m_x = -width * 0.5f;	
		shapePoints[i].m_y = radius * dCos ( 2.0f * 3.1416 * dFloat( i )/ dFloat( TIRE_SHAPE_SIZE ) );
		shapePoints[i].m_z = radius * dSin ( 2.0f * 3.1416 * dFloat( i )/ dFloat( TIRE_SHAPE_SIZE ) );
		shapePoints[i + TIRE_SHAPE_SIZE].m_x = -shapePoints[i].m_x;
		shapePoints[i + TIRE_SHAPE_SIZE].m_y = shapePoints[i].m_y;
		shapePoints[i + TIRE_SHAPE_SIZE].m_z = shapePoints[i].m_z;
	}
	m_tires[m_tiresCount].m_shape = NewtonCreateConvexHull ( m_world, TIRE_SHAPE_SIZE * 2, &shapePoints[0].m_x, sizeof (dVector), 0.0f, NULL );
	// NewtonCreateChamferCylinder(m_world,radius,width,NULL); 
	// NewtonCreateSphere(m_world,radius,radius,radius,&offmat[0][0]);
	// NewtonCreateCone(m_world,radius,width,NULL);
	// NewtonCreateCapsule(m_world,radius,width,NULL);
	// NewtonCreateChamferCylinder(m_world,radius,width,NULL);
	// NewtonCreateCylinder(m_world,radius*2,width*2,NULL);
	// NewtonCreateBox(m_world,radius*2,radius*2,radius*2,NULL);

	// calculate the tire geometrical parameters
	m_tires[m_tiresCount].m_radius = radius;
	// m_tires[m_tiresCount].m_radiusInv  = 1.0f / m_tires[m_tiresCount].m_radius;
	m_tires[m_tiresCount].m_mass = mass;	
	m_tires[m_tiresCount].m_massInv = 1.0f / m_tires[m_tiresCount].m_mass;	
	m_tires[m_tiresCount].m_Ixx = mass * radius * radius / 2.0f;
	m_tires[m_tiresCount].m_IxxInv = 1.0f / m_tires[m_tiresCount].m_Ixx;
	SetTireMaxRPS ( m_tiresCount, 150.0f / radius );
	m_tiresCount ++;
}

void CustomDGRayCastCar::SetVarTireMovePointForceFront (int index, dFloat distance)
{
	m_tires[index].m_MovePointForceFront = distance;
}

void CustomDGRayCastCar::SetVarTireMovePointForceRight (int index, dFloat distance)
{
	m_tires[index].m_MovePointForceRight = distance;
}

void CustomDGRayCastCar::SetVarTireMovePointForceUp (int index, dFloat distance)
{
	m_tires[index].m_MovePointForceUp = distance;
}

const dMatrix& CustomDGRayCastCar::GetChassisMatrixLocal () const
{
	return m_localFrame;
}

dMatrix CustomDGRayCastCar::CalculateSuspensionMatrix (int tireIndex, dFloat distance) const
{
	const Tire& tire = m_tires[tireIndex];
	dMatrix matrix;
	// calculate the steering angle matrix for the axis of rotation
	matrix.m_front = tire.m_localAxis;
	matrix.m_up    = dVector (0.0f, 1.0f, 0.0f, 0.0f);
	matrix.m_right = dVector (-tire.m_localAxis.m_z, 0.0f, tire.m_localAxis.m_x, 0.0f);
	matrix.m_posit = tire.m_harpoint - m_localFrame.m_up.Scale ( distance );
	return matrix;
}

dMatrix CustomDGRayCastCar::CalculateTireMatrix (int tireIndex) const
{
	const Tire& tire = m_tires[tireIndex];
	// calculate the rotation angle matrix
	dMatrix angleMatrix ( dPitchMatrix( tire.m_spinAngle ) );
	// get the tire body matrix
	dMatrix bodyMatrix;
	NewtonBodyGetMatrix( m_body0, &bodyMatrix[0][0] );
	return angleMatrix * CalculateSuspensionMatrix ( tireIndex, tire.m_posit ) * m_localFrame * bodyMatrix;

}

unsigned CustomDGRayCastCar::ConvexCastPrefilter (const NewtonBody* body, const NewtonCollision* collision, void* userData)
{
	NewtonBody* me;
	me = (NewtonBody*) userData;
	// do no cast myself
	return ( me != body );
}

void CustomDGRayCastCar::ApplyTiresTorqueVisual (Tire& tire, dFloat timestep, int threadIndex)
{
	dFloat timestepInv;
	// get the simulation time
	timestepInv = 1.0f / timestep;
	dVector tireRadius (tire.m_contactPoint - tire.m_tireAxelPosit);
	dFloat tireLinearSpeed;
	dFloat tireContactSpeed;
	tireLinearSpeed = tire.m_tireAxelVeloc % tire.m_longitudinalDir;	
	tireContactSpeed = (tire.m_lateralDir * tireRadius) % tire.m_longitudinalDir;
	//check if any engine torque or brake torque is applied to the tire
	if (dAbs(tire.m_torque) < 1.0e-3f){
		//tire is coasting, calculate the tire zero slip angular velocity
		// this is the velocity that satisfy the constraint equation
		// V % dir + W * R % dir = 0
		// where V is the tire Axel velocity
		// W is the tire local angular velocity
		// R is the tire radius
		// dir is the longitudinal direction of of the tire.		
		// this checkup is suposed to fix a infinit division by zero...
		if ( dAbs(tireContactSpeed)  > 1.0e-3) { 
			tire.m_angularVelocity = - (tireLinearSpeed) / (tireContactSpeed);
		}
		//tire.m_angularVelocity = - tireLinearSpeed / tireContactSpeed;
		tire.m_spinAngle = dMod (tire.m_spinAngle + tire.m_angularVelocity * timestep, 3.1416f * 2.0f);
	} else {
		// tire is under some power, need to do the free body integration to apply the net torque
		dFloat nettorque = tire.m_angularVelocity;
		// this checkup is suposed to fix a infinit division by zero...
		if ( dAbs(tireContactSpeed)  > 1.0e-3) { 
			nettorque = - (tireLinearSpeed) / (tireContactSpeed);
		} 
		//tire.m_angularVelocity = - tireLinearSpeed / tireContactSpeed;
		dFloat torque;
		torque = tire.m_torque - nettorque - tire.m_angularVelocity * tire.m_Ixx * 0.1f;
		tire.m_angularVelocity  += torque * tire.m_IxxInv * timestep;
		tire.m_spinAngle = dMod (tire.m_spinAngle + tire.m_angularVelocity * timestep, 3.1416f * 2.0f); 
	}
	// integrate tire angular velocity and rotation
	// tire.m_spinAngle = dMod (tire.m_spinAngle + tire.m_angularVelocity * timestep, 3.1416f * 2.0f); 
	// reset tire torque to zero after integration; 
	// tire.m_torque = 0.0f;
}

dFloat CustomDGRayCastCar::ApplySuspenssionLimit (Tire& tire)
{ 
	// This add a hard suspenssion limit. 
	// At very high speed the vehicle mass can make get the suspenssion limit faster.
	// This solution avoid this problem and push up the suspenssion.
	dFloat distance;
	distance = tire.m_suspensionLenght - tire.m_posit;
	if (distance>=tire.m_suspensionLenght){
		// The tire is inside the vehicle chassis.
		// Normally at this place the tire can't give torque on the wheel because the tire touch the chassis.
		// Set the tire speed to zero here is a bad idea.
		// Because the spring coming very bumpy and make the chassis go on any direction.
		// tire.m_tireSpeed = 0.0f;
		tire.m_torque = 0.0f;
		distance = (tire.m_suspensionLenght - tire.m_posit) + tire.m_suspenssionHardLimit;
	}
	return distance;
}

void CustomDGRayCastCar::ApplyOmegaCorrection ()
{ 
	m_chassisOmega = m_chassisOmega.Scale( m_chassisRotationLimit );
	NewtonBodySetOmega( m_body0, &m_chassisOmega[0] );
}

void CustomDGRayCastCar::ApplyChassisForceAndTorque (const dVector& vForce, const dVector& vPoint)
{
	NewtonBodyAddForce( m_body0, &vForce[0] );
	ApplyChassisTorque( vForce, vPoint ); 
}

void CustomDGRayCastCar::ApplyDeceleration (Tire& tire)
{
	if ( dAbs( tire.m_torque ) < 1.0e-3f ){
		dVector cvel = m_chassisVelocity;   
		cvel = cvel.Scale( m_fixDeceleration );
		cvel.m_y = m_chassisVelocity.m_y;
		NewtonBodySetVelocity( m_body0, &cvel.m_x );
	}	
}

void CustomDGRayCastCar::ApplyChassisTorque (const dVector& vForce, const dVector& vPoint)
{
	dVector Torque;
	dMatrix M;
	dVector com;
	NewtonBodyGetCentreOfMass( m_body0, &com[0] );
	NewtonBodyGetMatrix( m_body0, &M[0][0] );
	Torque = ( vPoint - M.TransformVector( dVector( com.m_x, com.m_y, com.m_z, 1.0f ) ) ) * vForce;
	NewtonBodyAddTorque( m_body0, &Torque[0] );
}

void CustomDGRayCastCar::ApplyTireFrictionVelocitySiding (Tire& tire, const dMatrix& chassisMatrix, const dVector& tireAxelVeloc, const dVector& tireAxelPosit, dFloat timestep, dFloat invTimestep, int& longitudinalForceIndex)
{
	tire.m_lateralPin = ( chassisMatrix.RotateVector ( tire.m_localAxis ) );
	tire.m_longitudinalPin = ( chassisMatrix.m_up * tire.m_lateralPin );
	tire.m_longitudinalDir = tire.m_longitudinalPin;
	tire.m_lateralDir = tire.m_lateralPin; 
	// TO DO: need to subtract the velocity at the contact point of the hit body
	// for now assume the ground is a static body
	dVector hitBodyContactVeloc ( 0, 0, 0, 0 );
	if ( tire.m_HitBody ){
		NewtonBodyGetVelocity( tire.m_HitBody, &hitBodyContactVeloc[0] );
	} 
	// calculate relative velocity at the tire center
	dVector tireAxelRelativeVelocity ( tireAxelVeloc - hitBodyContactVeloc ); 
	// now calculate relative velocity a velocity at contact point
	dVector tireAngularVelocity ( tire.m_lateralPin.Scale ( tire.m_angularVelocity ) );
	dVector tireRadius ( tire.m_contactPoint - tireAxelPosit );
	dVector tireContactVelocity ( tireAngularVelocity * tireRadius );	
	dVector tireContactRelativeVelocity ( tireAxelRelativeVelocity + tireContactVelocity ); 
	tire.m_tireRadius = tireRadius;
	// Apply brake, need some little fix here.
	// The fix is need to generate axial force when the brake is apply when the vehicle turn from the steer or on sliding.
	if ( dAbs( tire.m_breakTorque ) > 1.0e-3f ) {
		tire.m_isBraking = 1;
		tire.m_torque = 0.0f;
		tire.m_turnforce = tire.m_turnforce * 0.5f;
		tire.m_breakTorque /= timestep;
		NewtonUserJointAddLinearRow ( m_joint, &tireAxelPosit[0], &tireAxelPosit[0], &chassisMatrix.m_front.m_x /*&tire.m_longitudinalPin[0]*/ );
		NewtonUserJointSetRowMaximumFriction( m_joint, tire.m_breakTorque );
		NewtonUserJointSetRowMinimumFriction( m_joint, -tire.m_breakTorque );
	} 
	tire.m_breakTorque = 0.0f;  
	// the tire is in coasting mode
	//		dFloat tireContactSpeed;
	//		dFloat tireRelativeSpeed;
	//		dFloat lateralForceMagnitud;
	//these tire is coasting, so the lateral friction dominates the behaviors
	dFloat invMag2;
	dFloat frictionCircleMag;
	dFloat lateralFrictionForceMag;
	dFloat longitudinalFrictionForceMag;
	frictionCircleMag = tire.m_tireLoad * tire.m_groundFriction;
	lateralFrictionForceMag = frictionCircleMag;
	longitudinalFrictionForceMag = tire.m_tireLoad * 1.0f;
	invMag2 = frictionCircleMag / dSqrt ( lateralFrictionForceMag * lateralFrictionForceMag + longitudinalFrictionForceMag * longitudinalFrictionForceMag );
	lateralFrictionForceMag *= invMag2;
	longitudinalFrictionForceMag = invMag2;
	NewtonUserJointAddLinearRow ( m_joint, &tireAxelPosit[0], &tireAxelPosit[0], &tire.m_lateralPin[0] );
	NewtonUserJointSetRowMaximumFriction( m_joint,  lateralFrictionForceMag );
	NewtonUserJointSetRowMinimumFriction( m_joint,  -lateralFrictionForceMag );
	// save the tire contact longitudinal velocity for integration after the solver
	tire.m_currentSlipVeloc = tireAxelRelativeVelocity % tire.m_longitudinalPin;
}

void CustomDGRayCastCar::CalculateTireCollision (Tire& tire, int threadIndex) const
{
	int floorcontact = 0;
	tire.m_HitBody = NULL;
	if ( tire.m_tireUseConvexCastMode ) {
		NewtonWorldConvexCastReturnInfo info;
		tire.m_rayDestination = ( tire.m_suspensionMatrix.TransformVector( m_localFrame.m_up.Scale ( -tire.m_suspensionLenght ) ) );   
		dFloat hitParam;
		if ( NewtonWorldConvexCast ( m_world, &tire.m_suspensionMatrix[0][0], &tire.m_rayDestination[0], tire.m_shape, &hitParam, (void*)m_body0, ConvexCastPrefilter, &info, 1, threadIndex ) ){
			tire.m_posit = hitParam * tire.m_suspensionLenght;
			tire.m_contactPoint = info.m_point;
			tire.m_contactNormal = info.m_normal;
			tire.m_HitBody = (NewtonBody*)info.m_hitBody; 
			floorcontact = 1;
		} else {
			tire.m_posit = tire.m_suspensionLenght;
			tire.m_contactPoint = dVector (0.0f, 0.0f, 0.0f, 1.0f);
			tire.m_lateralPin = dVector (0.0f, 0.0f, 0.0f, 1.0f);
			tire.m_longitudinalPin = dVector (0.0f, 0.0f, 0.0f, 1.0f);
			tire.m_longitudinalDir = dVector (0.0f, 0.0f, 0.0f, 0.0f);
			tire.m_lateralDir = dVector (0.0f, 0.0f, 0.0f, 0.0f);
		}
		/*
		// I'm really not sure about what I need to do for get the side tire cast and the force to apply on.
		tire.m_tireSideForce = dVector (0.0f, 0.0f, 0.0f, 1.0f);
		tire.m_sideHit = 0;
		NewtonWorldConvexCastReturnInfo info2;
		NewtonWorldConvexCastReturnInfo info3;
		dFloat hitParam2;
		dFloat hitParam3;
		dVector destination2 = (tire.m_suspensionMatrix.TransformVector(m_localFrame.m_front.Scale (-tire.m_tireWidth*0.5f))); 
		dVector destination3 = (tire.m_suspensionMatrix.TransformVector(m_localFrame.m_front.Scale (tire.m_tireWidth*0.5f))); 
		if (NewtonWorldConvexCast (m_world, &tire.m_suspensionMatrix[0][0], &destination2[0], tire.m_shape, &hitParam2, (void*)m_body0, ConvexCastPrefilter, &info2, 1, threadIndex)){
		tire.m_sideHit = 1;
		} else
		if (NewtonWorldConvexCast (m_world, &tire.m_suspensionMatrix[0][0], &destination3[0], tire.m_shape, &hitParam3, (void*)m_body0, ConvexCastPrefilter, &info3, 1, threadIndex)){
		tire.m_sideHit = 2;
		} 
		*/
	} else {
		struct RayCastInfo
		{
			RayCastInfo (const NewtonBody* body)
			{
				m_param = 1.0f;
				m_me = body;
				m_hitBody = NULL;
				m_contactID = 0;
				m_normal = dVector (0.0f, 0.0f, 0.0f, 1.0f);
			}
			static dFloat RayCast (const NewtonBody* body, const dFloat* normal, int collisionID, void* userData, dFloat intersetParam)
			{
				RayCastInfo& caster = *( (RayCastInfo*) userData ); 
				// if this body is not the vehicle, see if a close hit
				if ( body != caster.m_me ) {
					if ( intersetParam < caster.m_param) {
						// this is a close hit, record the information. 
						caster.m_param = intersetParam;
						caster.m_hitBody = body;
						caster.m_contactID = collisionID;
						caster.m_normal = dVector (normal[0], normal[1], normal[2], 1.0f);
					} 
				}
				return intersetParam;
			}
			dFloat m_param;
			dVector m_normal;
			const NewtonBody* m_me;
			const NewtonBody* m_hitBody;
			int m_contactID;
		};
		RayCastInfo info ( m_body0 );
		// extend the ray by the radius of the tire
		dFloat dist ( tire.m_suspensionLenght + tire.m_radius );
		tire.m_rayDestination = ( tire.m_suspensionMatrix.TransformVector( m_localFrame.m_up.Scale ( -dist ) ) );	
		// cast a ray to the world ConvexCastPrefilter
		NewtonWorldRayCast( m_world, &tire.m_suspensionMatrix.m_posit[0], &tire.m_rayDestination[0], RayCastInfo::RayCast, &info, &ConvexCastPrefilter );
		// if the ray hit something, it means the tire has some traction
		if ( info.m_hitBody ) {
			dFloat intesectionDist;
			tire.m_HitBody = (NewtonBody*)info.m_hitBody; 
			tire.m_contactPoint = tire.m_suspensionMatrix.m_posit + ( tire.m_rayDestination - tire.m_suspensionMatrix.m_posit ).Scale ( info.m_param ); 
			tire.m_contactNormal = info.m_normal;  
			// TO DO: get the material properties for tire frictions on different roads 
			intesectionDist = ( dist * info.m_param - tire.m_radius );
			if ( intesectionDist < 0.0f ) {
				intesectionDist = 0.0f;
			} else if ( intesectionDist > tire.m_suspensionLenght ) {
				intesectionDist = tire.m_suspensionLenght;
			}
			tire.m_posit = intesectionDist;
		} else {
			tire.m_posit = tire.m_suspensionLenght;
		}
	}
}

void CustomDGRayCastCar::SubmitConstrainst (dFloat timestep, int threadIndex)
{
	dFloat invTimestep;
	dMatrix bodyMatrix;  
	// get the simulation time
	invTimestep = 1.0f / timestep ;
	// get the vehicle global matrix, and use it in several calculations
	NewtonBodyGetMatrix( m_body0, &bodyMatrix[0][0] );
	dMatrix chassisMatrix ( m_localFrame * bodyMatrix );

	// get the chassis instantaneous linear and angular velocity in the local space of the chassis
	int longitidunalForceIndex;
	longitidunalForceIndex = 0;
	NewtonBodyGetVelocity( m_body0, &m_chassisVelocity[0] );
	NewtonBodyGetOmega( m_body0, &m_chassisOmega[0] );
	// all tire is on air check
	m_vehicleOnAir = 0;
	for ( int i = 0; i < m_tiresCount; i ++ ) {
		Tire& tire = m_tires[i];
		tire.m_isBraking = 0;
		tire.m_tireIsOnAir = 0;
		tire.m_TireForceTorque = dVector(0.0f, 0.0f, 0.0f, 0.0f);

		// calculate all suspension matrices in global space and tire collision
		tire.m_suspensionMatrix = CalculateSuspensionMatrix ( i, 0.0f ) * chassisMatrix;

		// calculate the tire collision
		CalculateTireCollision (tire, threadIndex);

		// calculate the linear velocity of the tire at the ground contact
		tire.m_tireAxelPosit = ( chassisMatrix.TransformVector( tire.m_harpoint - m_localFrame.m_up.Scale ( tire.m_posit ) ) );
		tire.m_localAxelPosit = ( tire.m_tireAxelPosit - chassisMatrix.m_posit );
		tire.m_tireAxelVeloc = ( m_chassisVelocity + m_chassisOmega * tire.m_localAxelPosit ); 

		tire.m_lateralPin = ( chassisMatrix.RotateVector ( tire.m_localAxis ) );
		tire.m_longitudinalPin = ( chassisMatrix.m_up * tire.m_lateralPin );
		tire.m_longitudinalDir = tire.m_longitudinalPin;
		tire.m_lateralDir = tire.m_lateralPin; 

		if ( tire.m_posit < tire.m_suspensionLenght )  {
			//dFloat speed;
			// TO DO: need to calculate the velocity if the other body at the point
			// for now assume the ground is a static body
			dVector hitBodyVeloc( 0, 0, 0, 0 );
			if (tire.m_HitBody){
				NewtonBodyGetVelocity( tire.m_HitBody, &hitBodyVeloc[0] );
			} 
			// calculate the relative velocity
			dVector relVeloc ( tire.m_tireAxelVeloc - hitBodyVeloc );
			tire.m_tireSpeed = -( relVeloc % chassisMatrix.m_up );
			// now calculate the tire load at the contact point
			// Tire suspenssion distance and hard limit.
			// Important when the vehicle go at very high speed.
			dFloat distance = ApplySuspenssionLimit( tire );
			tire.m_tireLoad = - NewtonCalculateSpringDamperAcceleration ( timestep, tire.m_springConst, distance, tire.m_springDamper, tire.m_tireSpeed );
			if ( tire.m_tireLoad < 0.0f ) {
				// since the tire is not a body with real mass it can only push the chassis.
				tire.m_tireLoad = 0.0f;
			} else {
				//this suspension is applying a normalize force to the car chassis, need to scales by the mass of the car
				tire.m_tireLoad *= m_mass * 0.5f;
				// apply the tire model to these wheel		
				ApplyTireFrictionVelocitySiding( tire, chassisMatrix, tire.m_tireAxelVeloc, tire.m_tireAxelPosit, timestep, invTimestep, longitidunalForceIndex );
				tire.m_tireIsOnAir = 1;
			}
			// convert the tire load force magnitude to a torque and force.
			dVector tireForce ( chassisMatrix.m_up.Scale ( tire.m_tireLoad ) );
			// accumulate the force and torque form this suspension
			tire.m_TireForceTorque = tireForce;
			/*
			// tire force on side... unfinished not sure how to do this...
			if (tire.m_sideHit==1) {
			printf("hit Side Left <- ");
			tire.m_TireForceTorque += tire.m_tireSideForce;
			} else
			if (tire.m_sideHit==2) {
			printf("hit Side Right -> ");
			tire.m_TireForceTorque += tire.m_tireSideForce;
			}
			*/
			if ( tire.m_tireIsOnAir != 0 ) {
				tire.m_TireForceTorque += tire.m_suspensionMatrix.m_right.Scale( tire.m_torque ); 
				if ( dAbs( m_curSpeed ) != 0.0f ) {
					tire.m_TireForceTorque += tire.m_suspensionMatrix.m_front.Scale( tire.m_turnforce ); 
				}
				//if ( tire.m_groundFriction != 0 ) {
				// Displace the global force on tire
				// Very usefull on many thing, and usefull to simulation losing grip and more.
				// It can help on many vehicle type.
				dVector DisplaceAxelPosit = tire.m_tireAxelPosit;
				DisplaceAxelPosit += tire.m_suspensionMatrix.m_front.Scale( tire.m_MovePointForceFront );
				DisplaceAxelPosit += tire.m_suspensionMatrix.m_right.Scale( tire.m_MovePointForceRight );
				DisplaceAxelPosit += tire.m_suspensionMatrix.m_up.Scale( tire.m_MovePointForceUp );
				//
				ApplyChassisForceAndTorque ( tire.m_TireForceTorque, DisplaceAxelPosit ); 
				ApplyOmegaCorrection ();  
				ApplyDeceleration ( tire );
				//} 
				m_vehicleOnAir = 1;
			}
		} else {
			tire.m_posit = tire.m_suspensionLenght;
			//tire is on the air  not force applied to the vehicle.
			tire.m_tireLoad = 0.0f;
			// tire.m_tireJacobianRowIndex = -1;
			// simulate continual tire rotation on air...
			// This is very not complet, and currently it is a rotation inverted.
			// Need some fix here, to fix the rotation side and add a sort of deceleration on the torque simulation.
			// Currently it is based on the tire movement on air but it is wrong idea for now.
			// I try to find a better way on the next version.
			if ( dAbs( tire.m_breakTorque ) < 1.0e-3f ) {
				tire.m_contactPoint = tire.m_suspensionMatrix.m_posit + ( tire.m_rayDestination - tire.m_suspensionMatrix.m_posit ).Scale ( tire.m_posit );
			} else {
				tire.m_contactPoint = tire.m_suspensionMatrix.m_posit + ( tire.m_rayDestination - tire.m_suspensionMatrix.m_posit ).Scale ( 1.0f );
			}
		}
		dFloat torque;
		torque = tire.m_torque - tire.m_angularVelocity * tire.m_Ixx * 0.1f;
		tire.m_angularVelocity  += torque * tire.m_IxxInv * timestep;

		ApplyTiresTorqueVisual( tire, timestep, threadIndex );
		tire.m_torque = 0.0f;
		tire.m_turnforce = 0.0f;
		// set the current vehicle speed
		m_curSpeed = bodyMatrix.m_front % m_chassisVelocity;
		if ( m_curSpeed > 0 ) { 
			m_tiresRollSide = 0; 
		} else {
			m_tiresRollSide = 1;
		}
	}
}


