#ifndef NEWTON_CUSTOM_JOINTS_H_INCLUDED_
#define NEWTON_CUSTOM_JOINTS_H_INCLUDED_


#include "Newton.h"
#include "CustomJointLibraryStdAfx.h"

#ifdef __cplusplus 
extern "C" {
#endif

//	typedef struct NewtonCustomJoint{} NewtonCustomJoint;

	typedef int (*PlayerCanPuchThisBodyCalback) (NewtonCustomJoint *me, const NewtonBody* hitBody);

	// Generic 6 degree of Freedom Joint
	JOINTLIBRARY_API NewtonCustomJoint *CreateCustomJoint6DOF (const dFloat* pinsAndPivoChildFrame, const dFloat* pinsAndPivoParentFrame, const NewtonBody* child, const NewtonBody* parent);
	JOINTLIBRARY_API void CustomJoint6DOF_SetLinearLimits (NewtonCustomJoint* customJoint6DOF, const dFloat* minLinearLimits, const dFloat* maxLinearLimits);
	JOINTLIBRARY_API void CustomJoint6DOF_SetAngularLimits (NewtonCustomJoint* customJoint6DOF, const dFloat* minAngularLimits, const dFloat* maxAngularLimits);
	JOINTLIBRARY_API void CustomJoint6DOF_GetLinearLimits (NewtonCustomJoint* customJoint6DOF, dFloat* minLinearLimits, dFloat* maxLinearLimits);
	JOINTLIBRARY_API void CustomJoint6DOF_GetAngularLimits (NewtonCustomJoint* customJoint6DOF, dFloat* minAngularLimits, dFloat* maxAngularLimits);
	JOINTLIBRARY_API void CustomJoint6DOF_SetReverseUniversal (NewtonCustomJoint* customJoint6DOF, int order);


	// player controller functions
	JOINTLIBRARY_API NewtonCustomJoint *CreateCustomPlayerController (const dFloat* pins, const NewtonBody* player, dFloat maxStairStepFactor);
	JOINTLIBRARY_API void CustomPlayerControllerSetPushActorCallback (NewtonCustomJoint* playerController, PlayerCanPuchThisBodyCalback callback);
	JOINTLIBRARY_API void CustomPlayerControllerSetVelocity (NewtonCustomJoint* playerController, dFloat forwardSpeed, dFloat sideSpeed, dFloat heading);
	JOINTLIBRARY_API void CustomPlayerControllerSetMaxSlope (NewtonCustomJoint* playerController, dFloat maxSlopeAngleIndRadian);
	JOINTLIBRARY_API dFloat CustomPlayerControllerGetMaxSlope (NewtonCustomJoint* playerController);
	JOINTLIBRARY_API const NewtonCollision* CustomPlayerControllerGetVerticalSensorShape (NewtonCustomJoint* playerController);
	JOINTLIBRARY_API const NewtonCollision* CustomPlayerControllerGetHorizontalSensorShape (NewtonCustomJoint* playerController);
	JOINTLIBRARY_API const NewtonCollision* CustomPlayerControllerGetDynamicsSensorShape (NewtonCustomJoint* playerController);

	// Multi rigid BodyCar controller functions
	JOINTLIBRARY_API NewtonCustomJoint *CreateCustomMultiBodyVehicle (const dFloat* frontDir, const dFloat* upDir, const NewtonBody* carBody);
	JOINTLIBRARY_API int CustomMultiBodyVehicleAddTire (NewtonCustomJoint *car, const void* userData, const dFloat* localPosition, 
													    dFloat mass, dFloat radius, dFloat width,
													    dFloat suspensionLength, dFloat springConst, dFloat springDamper);
	JOINTLIBRARY_API int CustomMultiBodyVehicleAddSlipDifferencial (NewtonCustomJoint *car, int leftTireIndex, int rightToreIndex, dFloat maxFriction);
	JOINTLIBRARY_API int CustomMultiBodyVehicleGetTiresCount(NewtonCustomJoint *car);
	JOINTLIBRARY_API const NewtonBody* CustomMultiBodyVehicleGetTireBody(NewtonCustomJoint *car, int tireIndex);
	JOINTLIBRARY_API dFloat CustomMultiBodyVehicleGetSpeed(NewtonCustomJoint *car);
	JOINTLIBRARY_API dFloat CustomMultiBodyVehicleGetTireSteerAngle (NewtonCustomJoint *car, int index);
	JOINTLIBRARY_API void CustomMultiBodyVehicleApplyTorque (NewtonCustomJoint *car, int tireIndex, dFloat torque);
	JOINTLIBRARY_API void CustomMultiBodyVehicleApplySteering (NewtonCustomJoint *car, int tireIndex, dFloat angle);
	JOINTLIBRARY_API void CustomMultiBodyVehicleApplyBrake (NewtonCustomJoint *car, int tireIndex, dFloat brakeTorque);
	JOINTLIBRARY_API void CustomMultiBodyVehicleApplyTireRollingDrag (NewtonCustomJoint *car, int index, dFloat angularDampingCoef);


    // BEGIN k00m (Dave Gravel simple raycast world vehicle)
    JOINTLIBRARY_API NewtonCustomJoint *DGRaycastVehicleCreate (int maxTireCount, const dFloat* cordenateSytem, NewtonBody* carBody);
    JOINTLIBRARY_API void DGRaycastVehicleAddTire (NewtonCustomJoint *car, void *userData, const dFloat* localPosition, dFloat mass, dFloat radius, dFloat width, dFloat friction, dFloat suspensionLength, dFloat springConst, dFloat springDamper, int castMode);
    JOINTLIBRARY_API void DGRayCarGetChassisMatrixLocal(NewtonCustomJoint *car, dFloat* chassisMatrix);
    JOINTLIBRARY_API void DGRayCarTireMatrix(NewtonCustomJoint *car, int tire, dFloat* tireMatrix);
	JOINTLIBRARY_API void DGRayCarSuspensionMatrix(NewtonCustomJoint *car, int tire, dFloat param, dFloat* SuspensionMatrix);	
	JOINTLIBRARY_API const NewtonCollision* DGRayCarTireShape(NewtonCustomJoint *car, int tireIndex);    
	JOINTLIBRARY_API dFloat DGRaycastVehicleGetSpeed(NewtonCustomJoint *car);
	JOINTLIBRARY_API void DGRaycastVehicleSetCustomTireBrake (NewtonCustomJoint *car, int index, dFloat torque);
    JOINTLIBRARY_API void DGRaycastVehicleSetCustomTireTorque (NewtonCustomJoint *car, int index, dFloat torque);
    JOINTLIBRARY_API void DGRaycastVehicleSetCustomTireSteerAngleForce (NewtonCustomJoint *car, int index, dFloat angle, dFloat turnforce);
    JOINTLIBRARY_API dFloat DGRaycastVehicleGenerateTiresBrake (NewtonCustomJoint *car, dFloat value);
	JOINTLIBRARY_API dFloat DGRaycastVehicleGenerateTiresTorque (NewtonCustomJoint *car, dFloat value);
	JOINTLIBRARY_API dFloat DGRaycastVehicleGenerateTiresSteerForce (NewtonCustomJoint *car, dFloat value);
	JOINTLIBRARY_API dFloat DGRaycastVehicleGenerateTiresSteerAngle (NewtonCustomJoint *car, dFloat value);
	JOINTLIBRARY_API void DGRaycastVehicleSetVarTireMovePointForceFront (NewtonCustomJoint *car, int index, dFloat distance);
	JOINTLIBRARY_API void DGRaycastVehicleSetVarTireMovePointForceRight (NewtonCustomJoint *car, int index, dFloat distance);
	JOINTLIBRARY_API void DGRaycastVehicleSetVarTireMovePointForceUp (NewtonCustomJoint *car, int index, dFloat distance);
	JOINTLIBRARY_API void DGRaycastVehicleSetVarFixDeceleration (NewtonCustomJoint *car, dFloat value);
	JOINTLIBRARY_API void DGRaycastVehicleSetVarChassisRotationLimit (NewtonCustomJoint *car, dFloat value);
	JOINTLIBRARY_API void DGRaycastVehicleSetVarMaxSteerAngle (NewtonCustomJoint *car, dFloat value);
    JOINTLIBRARY_API void DGRaycastVehicleSetVarMaxSteerRate (NewtonCustomJoint *car, dFloat value);
	JOINTLIBRARY_API void DGRaycastVehicleSetVarMaxSteerForceRate (NewtonCustomJoint *car, dFloat value);
	JOINTLIBRARY_API void DGRaycastVehicleSetVarMaxSteerForce (NewtonCustomJoint *car, dFloat value);
	JOINTLIBRARY_API void DGRaycastVehicleSetVarMaxSteerSpeedRestriction (NewtonCustomJoint *car, dFloat value);
	JOINTLIBRARY_API void DGRaycastVehicleSetVarMaxBrakeForce (NewtonCustomJoint *car, dFloat value);
    JOINTLIBRARY_API void DGRaycastVehicleSetVarMaxTorque (NewtonCustomJoint *car, dFloat value);
    JOINTLIBRARY_API void DGRaycastVehicleSetVarMaxTorqueRate (NewtonCustomJoint *car, dFloat value);
    JOINTLIBRARY_API void DGRaycastVehicleSetVarEngineSteerDiv (NewtonCustomJoint *car, dFloat value);
    JOINTLIBRARY_API void DGRaycastVehicleSetVarTireSuspenssionHardLimit (NewtonCustomJoint *car, int index, dFloat value);		
	JOINTLIBRARY_API void DGRaycastVehicleSetVarTireFriction (NewtonCustomJoint *car, int index, dFloat value);
	JOINTLIBRARY_API int DGRaycastVehicleGetTiresCount(NewtonCustomJoint *car);
	JOINTLIBRARY_API int DGRaycastVehicleGetVehicleOnAir(NewtonCustomJoint *car);
	JOINTLIBRARY_API int DGRaycastVehicleGetTireOnAir(NewtonCustomJoint *car, int index);
    JOINTLIBRARY_API void DGRaycastVehicleDestroy (NewtonCustomJoint *car);
	// END


	// Pick Body joint
	JOINTLIBRARY_API NewtonCustomJoint *CreateCustomPickBody (const NewtonBody* player, dFloat* handleInGlobalSpace);
	JOINTLIBRARY_API void CustomPickBodySetPickMode (NewtonCustomJoint *pick, int mode);
	JOINTLIBRARY_API void CustomPickBodySetMaxLinearFriction(NewtonCustomJoint *pick, dFloat accel); 
	JOINTLIBRARY_API void CustomPickBodySetMaxAngularFriction(NewtonCustomJoint *pick, dFloat alpha); 
	
	JOINTLIBRARY_API void CustomPickBodySetTargetRotation (NewtonCustomJoint *pick, dFloat* rotation); 
	JOINTLIBRARY_API void CustomPickBodySetTargetPosit (NewtonCustomJoint *pick, dFloat* posit); 
	JOINTLIBRARY_API void CustomPickBodySetTargetMatrix (NewtonCustomJoint *pick, dFloat* matrix); 

	JOINTLIBRARY_API void CustomPickBodyGetTargetMatrix (NewtonCustomJoint *pick, dFloat* matrix);



	// genetic joint functions
	JOINTLIBRARY_API int CustomGetJointID (NewtonCustomJoint *joint);
	JOINTLIBRARY_API void CustomSetJointID (NewtonCustomJoint *joint, int rttI);
	JOINTLIBRARY_API const NewtonBody* CustomGetBody0 (NewtonCustomJoint *joint);
	JOINTLIBRARY_API const NewtonBody* CustomGetBody1 (NewtonCustomJoint *joint);
	JOINTLIBRARY_API int CustomGetBodiesCollisionState (NewtonCustomJoint *joint);
	JOINTLIBRARY_API void CustomSetBodiesCollisionState (NewtonCustomJoint *joint, int state);
	JOINTLIBRARY_API void CustomDestroyJoint(NewtonCustomJoint *joint);

#ifdef __cplusplus 
}
#endif


#endif