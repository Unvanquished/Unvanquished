//////////////////////////////////////////////////////////////////////
#include "CustomJointLibraryStdAfx.h"
#include "JointLibrary.h"
#include "Custom6DOF.h"
#include "CustomPickBody.h"
#include "NewtonCustomJoint.h"
#include "CustomPlayerController.h"
#include "CustomMultiBodyVehicle.h"
#include "CustomDGRayCastCar.h"

// Generic 6 degree of Freedom Joint
NewtonCustomJoint *CreateCustomJoint6DOF (const dFloat* pinsAndPivoChildFrame, const dFloat* pinsAndPivoParentFrame, const NewtonBody* child, const NewtonBody* parent)
{
	return new Custom6DOF (*(dMatrix*) pinsAndPivoChildFrame, *(dMatrix*) pinsAndPivoParentFrame, child, parent);
}


void CustomJoint6DOF_SetLinearLimits (NewtonCustomJoint* customJoint6DOF, const dFloat* minLinearLimits, const dFloat* maxLinearLimits)
{
	((Custom6DOF*)customJoint6DOF)->SetLinearLimits (*(dVector*)minLinearLimits, *(dVector*)maxLinearLimits); 
}

void CustomJoint6DOF_SetAngularLimits (NewtonCustomJoint* customJoint6DOF, const dFloat* minAngularLimits, const dFloat* maxAngularLimits)
{
	((Custom6DOF*)customJoint6DOF)->SetAngularLimits(*(dVector*)minAngularLimits, *(dVector*)maxAngularLimits); 
}

void CustomJoint6DOF_GetLinearLimits (NewtonCustomJoint* customJoint6DOF, dFloat* minLinearLimits, dFloat* maxLinearLimits)
{
	((Custom6DOF*)customJoint6DOF)->GetLinearLimits (*(dVector*)minLinearLimits, *(dVector*)maxLinearLimits); 
}

void CustomJoint6DOF_GetAngularLimits (NewtonCustomJoint* customJoint6DOF, dFloat* minAngularLimits, dFloat* maxAngularLimits)
{
	((Custom6DOF*)customJoint6DOF)->GetAngularLimits(*(dVector*)minAngularLimits, *(dVector*)maxAngularLimits); 
}

void CustomJoint6DOF_SetReverseUniversal (NewtonCustomJoint* customJoint6DOF, int order)
{
	((Custom6DOF*)customJoint6DOF)->SetReverserUniversal(order);
}




// player controller functions 
class PlayerController: public CustomPlayerController 
{
	public: 

	typedef int (*PlayerCanPuchBody) (NewtonCustomJoint *me, const NewtonBody* hitBody);

	PlayerController (const dVector& pin, const NewtonBody* child, dFloat maxStairStepFactor)
		:CustomPlayerController (pin, child, maxStairStepFactor)
	{
		m_canPuchOtherBodies = CanPushThisBodyCallback;
	}

	virtual bool CanPushBody (const NewtonBody* hitBody) const 
	{
		if (m_canPuchOtherBodies) {
			return m_canPuchOtherBodies ((NewtonCustomJoint *)this, hitBody) ? true : false;
		} 
		return true;
	}


	static int CanPushThisBodyCallback(NewtonCustomJoint *me, const NewtonBody* hitBody) 
	{
		return 1;
	}


	PlayerCanPuchBody m_canPuchOtherBodies;
};

NewtonCustomJoint *CreateCustomPlayerController (const dFloat* pin, const NewtonBody* player, dFloat maxStairStepFactor)
{
	return (NewtonCustomJoint *) new CustomPlayerController (*(dVector*) pin, player, maxStairStepFactor);
}

void CustomPlayerControllerSetPushActorCallback (NewtonCustomJoint* playerController, PlayerCanPuchThisBodyCalback callback)
{
	((PlayerController*)playerController)->m_canPuchOtherBodies = (PlayerController::PlayerCanPuchBody)callback;
}

void CustomPlayerControllerSetVelocity (NewtonCustomJoint* playerController, dFloat forwardSpeed, dFloat sideSpeed, dFloat heading)
{
	((PlayerController*)playerController)->SetVelocity (forwardSpeed, sideSpeed, heading);
}

void CustomPlayerControllerSetMaxSlope (NewtonCustomJoint* playerController, dFloat maxSlopeAngleIndRadian)
{
	((PlayerController*)playerController)->SetMaxSlope (maxSlopeAngleIndRadian);
}
dFloat CustomPlayerControllerGetMaxSlope (NewtonCustomJoint* playerController)
{
	return ((PlayerController*)playerController)->GetMaxSlope();
}

const NewtonCollision* CustomPlayerControllerGetVerticalSensorShape (NewtonCustomJoint* playerController)
{
	return ((PlayerController*)playerController)->GetVerticalSensorShape();
}

const NewtonCollision* CustomPlayerControllerGetHorizontalSensorShape (NewtonCustomJoint* playerController)
{
	return ((PlayerController*)playerController)->GetHorizontalSensorShape ();
}

const NewtonCollision* CustomPlayerControllerGetDynamicsSensorShape (NewtonCustomJoint* playerController)
{
	return ((PlayerController*)playerController)->GetDynamicsSensorShape ();
}


// MultiBody Vehicle interface
NewtonCustomJoint *CreateCustomMultiBodyVehicle (const dFloat* frontDir, const dFloat* upDir, const NewtonBody* carBody)
{
	return (NewtonCustomJoint *) new CustomMultiBodyVehicle (*((dVector*) frontDir), *((dVector*) upDir), carBody);
}


int CustomMultiBodyVehicleAddTire (NewtonCustomJoint *car, const void* userData, const dFloat* localPosition, 
								  dFloat mass, dFloat radius, dFloat width,
								  dFloat suspensionLength, dFloat springConst, dFloat springDamper)
{
	dVector posit (localPosition[0], localPosition[1], localPosition[2], 0.0f);
	return ((CustomMultiBodyVehicle*)car)->AddSingleSuspensionTire ((void*)userData, &posit[0], 
																	mass, radius, width, suspensionLength, springConst, springDamper);
}

int CustomMultiBodyVehicleAddSlipDifferencial (NewtonCustomJoint *car, int leftTireIndex, int rightToreIndex, dFloat maxFriction)
{
	return ((CustomMultiBodyVehicle*)car)->AddSlipDifferencial(leftTireIndex, rightToreIndex, maxFriction);
}


int CustomMultiBodyVehicleGetTiresCount(NewtonCustomJoint *car)
{
	return ((CustomMultiBodyVehicle*)car)->GetTiresCount();
}

const NewtonBody* CustomMultiBodyVehicleGetTireBody(NewtonCustomJoint *car, int tireIndex)
{
	return ((CustomMultiBodyVehicle*)car)->GetTireBody(tireIndex);
}

dFloat CustomMultiBodyVehicleGetSpeed(NewtonCustomJoint *car)
{
	return ((CustomMultiBodyVehicle*)car)->GetSpeed();
}

void CustomMultiBodyVehicleApplyTorque (NewtonCustomJoint *car, int tireIndex, dFloat torque)
{
	((CustomMultiBodyVehicle*)car)->ApplyTireTorque(tireIndex, torque);
}

void CustomMultiBodyVehicleApplySteering (NewtonCustomJoint *car, int tireIndex, dFloat angle)
{
	((CustomMultiBodyVehicle*)car)->ApplyTireSteerAngle(tireIndex, angle);
}

void CustomMultiBodyVehicleApplyBrake (NewtonCustomJoint *car, int tireIndex, dFloat brakeTorque)
{
	((CustomMultiBodyVehicle*)car)->ApplyTireBrake (tireIndex, brakeTorque);
}

void CustomMultiBodyVehicleApplyTireRollingDrag (NewtonCustomJoint *car, int tireIndex, dFloat angularDampingCoef)
{
	((CustomMultiBodyVehicle*)car)->ApplyTireRollingDrag (tireIndex, angularDampingCoef);
}

dFloat CustomMultiBodyVehicleGetTireSteerAngle (NewtonCustomJoint *car, int tireIndex)
{
	return ((CustomMultiBodyVehicle*)car)->GetSetTireSteerAngle (tireIndex);
}


// Pick Body joint
NewtonCustomJoint *CreateCustomPickBody (const NewtonBody* body, dFloat* handleInGlobalSpace)
{
	return new CustomPickBody (body, *((dVector*)handleInGlobalSpace));
}

void CustomPickBodySetPickMode (NewtonCustomJoint* pick, int mode)
{
	((CustomPickBody*)pick)->SetPickMode (mode);
}

void CustomPickBodySetMaxLinearFriction(NewtonCustomJoint* pick, dFloat accel) 
{
	((CustomPickBody*)pick)->SetMaxLinearFriction(accel); 
}

void CustomPickBodySetMaxAngularFriction(NewtonCustomJoint* pick, dFloat alpha) 
{
	((CustomPickBody*)pick)->SetMaxAngularFriction(alpha);
}


void CustomPickBodySetTargetRotation (NewtonCustomJoint* pick, dFloat* rotation) 
{
	((CustomPickBody*)pick)->SetTargetRotation (*((dQuaternion*) rotation)) ;
}

void CustomPickBodySetTargetPosit (NewtonCustomJoint* pick, dFloat* posit) 
{
	((CustomPickBody*)pick)->SetTargetPosit ((*(dVector*)posit));
}

void CustomPickBodySetTargetMatrix (NewtonCustomJoint* pick, dFloat* matrix) 
{
	((CustomPickBody*)pick)->SetTargetMatrix ((*(dMatrix*) matrix)); 
}

void CustomPickBodyGetTargetMatrix (NewtonCustomJoint* pick, dFloat* matrix)
{
	dMatrix& retMatrix = (*(dMatrix*) matrix);
	retMatrix = ((CustomPickBody*)pick)->GetTargetMatrix();
}

// BEGIN k00m (Dave Gravel simple raycast world vehicle)
NewtonCustomJoint *DGRaycastVehicleCreate (int maxTireCount, const dFloat* cordenateSytem, NewtonBody* carBody)
{
  return new CustomDGRayCastCar (maxTireCount, *((dMatrix*) cordenateSytem), carBody);
}

void DGRaycastVehicleAddTire (NewtonCustomJoint *car, void *userData, const dFloat* localPosition, dFloat mass, dFloat radius, dFloat width, dFloat friction, dFloat suspensionLength, dFloat springConst, dFloat springDamper, int castMode)
{
  ((CustomDGRayCastCar*)car)->AddSingleSuspensionTire(userData,localPosition,mass,radius,width,friction,suspensionLength,springConst,springDamper,castMode);
}

const NewtonCollision* DGRayCarTireShape(NewtonCustomJoint *car, int tireIndex) 
{
  return ((CustomDGRayCastCar*)car)->GetTiresShape(tireIndex); 
}

void DGRayCarGetChassisMatrixLocal(NewtonCustomJoint *car, dFloat* chassisMatrix)
{
  (*(dMatrix*)chassisMatrix) = ((CustomDGRayCastCar*)car)->GetChassisMatrixLocal();
}

void DGRayCarTireMatrix(NewtonCustomJoint *car, int tire, dFloat* tireMatrix)
{
  (*(dMatrix*)tireMatrix) = ((CustomDGRayCastCar*)car)->CalculateTireMatrix(tire);
}

void DGRayCarSuspensionMatrix(NewtonCustomJoint *car, int tire, dFloat param, dFloat* SuspensionMatrix)
{
  (*(dMatrix*)SuspensionMatrix) = ((CustomDGRayCastCar*)car)->CalculateSuspensionMatrix(tire,param);
}

void DGRaycastVehicleSetCustomTireBrake (NewtonCustomJoint *car, int index, dFloat torque)
{
  ((CustomDGRayCastCar*)car)->SetCustomTireBrake(index,torque);
}

void DGRaycastVehicleSetCustomTireTorque (NewtonCustomJoint *car, int index, dFloat torque)
{
  ((CustomDGRayCastCar*)car)->SetCustomTireTorque(index,torque);
}

void DGRaycastVehicleSetCustomTireSteerAngleForce (NewtonCustomJoint *car, int index, dFloat angle, dFloat turnforce)
{
  ((CustomDGRayCastCar*)car)->SetCustomTireSteerAngleForce(index,angle,turnforce);
}

dFloat DGRaycastVehicleGetSpeed(NewtonCustomJoint *car)
{
  return ((CustomDGRayCastCar*)car)->GetSpeed();
}

dFloat DGRaycastVehicleGenerateTiresBrake (NewtonCustomJoint *car, dFloat value)
{
  return ((CustomDGRayCastCar*)car)->GenerateTiresBrake ( value );
}

dFloat DGRaycastVehicleGenerateTiresTorque (NewtonCustomJoint *car, dFloat value)
{
  return ((CustomDGRayCastCar*)car)->GenerateTiresTorque( value );
}

dFloat DGRaycastVehicleGenerateTiresSteerForce (NewtonCustomJoint *car, dFloat value)
{
  return ((CustomDGRayCastCar*)car)->GenerateTiresSteerForce( value );
}

dFloat DGRaycastVehicleGenerateTiresSteerAngle (NewtonCustomJoint *car, dFloat value)
{
  return ((CustomDGRayCastCar*)car)->GenerateTiresSteerAngle( value );
}

void DGRaycastVehicleSetVarTireMovePointForceFront (NewtonCustomJoint *car, int index, dFloat distance)
{
  ((CustomDGRayCastCar*)car)->SetVarTireMovePointForceFront( index, distance );
}

void DGRaycastVehicleSetVarTireMovePointForceRight (NewtonCustomJoint *car, int index, dFloat distance)
{
  ((CustomDGRayCastCar*)car)->SetVarTireMovePointForceRight( index, distance );
}

void DGRaycastVehicleSetVarTireMovePointForceUp (NewtonCustomJoint *car, int index, dFloat distance)
{
  ((CustomDGRayCastCar*)car)->SetVarTireMovePointForceUp( index, distance );
}

void DGRaycastVehicleSetVarFixDeceleration (NewtonCustomJoint *car, dFloat value)
{
  ((CustomDGRayCastCar*)car)->SetVarFixDeceleration( value );
}

void DGRaycastVehicleSetVarChassisRotationLimit (NewtonCustomJoint *car, dFloat value)
{
  ((CustomDGRayCastCar*)car)->SetVarChassisRotationLimit( value );
}

void DGRaycastVehicleSetVarMaxSteerAngle (NewtonCustomJoint *car, dFloat value)
{
  ((CustomDGRayCastCar*)car)->SetVarMaxSteerAngle( value );
}

void DGRaycastVehicleSetVarMaxSteerRate (NewtonCustomJoint *car, dFloat value)
{
  ((CustomDGRayCastCar*)car)->SetVarMaxSteerRate( value );
}

void DGRaycastVehicleSetVarMaxSteerForceRate (NewtonCustomJoint *car, dFloat value)
{
  ((CustomDGRayCastCar*)car)->SetVarMaxSteerForceRate( value );
}

void DGRaycastVehicleSetVarMaxSteerForce (NewtonCustomJoint *car, dFloat value)
{
  ((CustomDGRayCastCar*)car)->SetVarMaxSteerForce( value );
}

void DGRaycastVehicleSetVarMaxSteerSpeedRestriction (NewtonCustomJoint *car, dFloat value)
{
  ((CustomDGRayCastCar*)car)->SetVarMaxSteerSpeedRestriction( value );
}

void DGRaycastVehicleSetVarMaxBrakeForce (NewtonCustomJoint *car, dFloat value)
{
  ((CustomDGRayCastCar*)car)->SetVarMaxBrakeForce( value );
}

void DGRaycastVehicleSetVarMaxTorque (NewtonCustomJoint *car, dFloat value)
{
  ((CustomDGRayCastCar*)car)->SetVarMaxTorque( value );
}

void DGRaycastVehicleSetVarMaxTorqueRate (NewtonCustomJoint *car, dFloat value)
{
  ((CustomDGRayCastCar*)car)->SetVarMaxTorqueRate( value );
}

void DGRaycastVehicleSetVarEngineSteerDiv (NewtonCustomJoint *car, dFloat value)
{
  ((CustomDGRayCastCar*)car)->SetVarEngineSteerDiv( value );
}

void DGRaycastVehicleSetVarTireSuspenssionHardLimit (NewtonCustomJoint *car, int index, dFloat value)
{
  ((CustomDGRayCastCar*)car)->SetVarTireSuspenssionHardLimit( index, value );
}

void DGRaycastVehicleSetVarTireFriction (NewtonCustomJoint *car, int index, dFloat value)
{
  ((CustomDGRayCastCar*)car)->SetVarTireFriction( index, value );
}

int DGRaycastVehicleGetVehicleOnAir(NewtonCustomJoint *car)
{
  return ((CustomDGRayCastCar*)car)->GetVehicleOnAir();
}

int DGRaycastVehicleGetTireOnAir(NewtonCustomJoint *car, int index)
{
  return ((CustomDGRayCastCar*)car)->GetTireOnAir( index );
}

int DGRaycastVehicleGetTiresCount(NewtonCustomJoint *car)
{
  return ((CustomDGRayCastCar*)car)->GetTiresCount();
}

void DGRaycastVehicleDestroy (NewtonCustomJoint *car)
{
  delete car;
}

// END

// common Joints functions
int CustomGetJointID (NewtonCustomJoint *joint)
{
	return joint->GetJointID ();
}

void CustomSetJointID (NewtonCustomJoint *joint, int rttI)
{
	joint->SetJointID(rttI);
}

const NewtonBody* CustomGetBody0 (NewtonCustomJoint *joint)
{
	return joint->GetBody0();
}

const NewtonBody* CustomGetBody1 (NewtonCustomJoint *joint)
{
	return joint->GetBody1();
}

int CustomGetBodiesCollisionState (NewtonCustomJoint *joint)
{
	return joint->GetBodiesCollisionState();
}

void CustomSetBodiesCollisionState (NewtonCustomJoint *joint, int state)
{
	joint->SetBodiesCollisionState(state);
}

void CustomDestroyJoint(NewtonCustomJoint *joint)
{
	delete joint;
}



