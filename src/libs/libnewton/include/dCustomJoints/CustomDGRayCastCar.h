// CustomDGRayCastCar.cpp: implementation of the CustomDGRayCastCar class.
// This raycast vehicle is currently a work in progress by Dave Gravel - 2009.
// Vehicle Raycast and convexCast Version 3.0b
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_CUSTOM_DGRAYCASTCAR_INCLUDED)
#define AFX_CUSTOM_DGRAYCASTCAR_INCLUDED

#include "NewtonCustomJoint.h"




// Dave Gravel raycas/convexcat contribution
class JOINTLIBRARY_API CustomDGRayCastCar: public NewtonCustomJoint  
{
public:

	struct Tire 
	{
		// constant data
		dMatrix m_suspensionMatrix;
		dMatrix m_steerMatrix;
		dVector m_harpoint;				   // attachment point of this tire to the chassis 
		dVector m_localAxis;               // tire local axis of rotation 
		dVector m_contactPoint;			   // contact point in global space
		dVector m_contactNormal;           // contact normal in global space
		dVector m_lateralDir;
		dVector m_longitudinalDir;
		dVector m_lateralPin;
		dVector m_longitudinalPin;
		dVector m_tireRadius;	
		dVector m_tireAxelPosit;
		dVector m_localAxelPosit;
		dVector m_tireAxelVeloc;
		dVector m_tireSideForce;
		dVector m_TireForceTorque;   
		dVector m_rayDestination;

		NewtonBody* m_HitBody;
		NewtonCollision* m_shape;          // collision shape of this tire 
		void* m_userData;                  // user data pointing to the visual tire
		dFloat m_mass;					   // tire Mass matrix
		dFloat m_Ixx;					   // axis inertia
		dFloat m_massInv;				   // tire Mass matrix
		dFloat m_IxxInv;                   // axis inertia
		dFloat m_radius;				   // tire Radius
		dFloat m_maxTireRPS;

		dFloat m_suspenssionHardLimit;
		dFloat m_MovePointForceFront;
		dFloat m_MovePointForceUp;
		dFloat m_MovePointForceRight;
		dFloat m_localLateralSpeed;				  	
		dFloat m_localSuspentionSpeed;				  	
		dFloat m_localLongitudinalSpeed;
		dFloat m_currentSlipVeloc;		   // the tire sleep acceleration	
		dFloat m_springConst;			   // normalized spring Ks
		dFloat m_springDamper;			   // normal;ized spring Damper Kc


		dFloat m_tireWidth;
		dFloat m_groundFriction;		   // coefficient of friction of the ground surface

		dFloat m_posit;					   // parametric position for this tire ( alway positive value between 0 and m_suspensionLength)
		dFloat m_suspensionLenght;
		dFloat m_tireSpeed;
		dFloat m_torque;				   // tire toque
		dFloat m_turnforce;				   // tire turnforce
		dFloat m_breakTorque;			   // tire break torque
		dFloat m_tireLoad;				   // force generate by the suspension compression (must be alway positive)		
		dFloat m_steerAngle;               // current tire steering angle  
		dFloat m_spinAngle;                // current tire spin angle  
		dFloat m_angularVelocity;          // current tire spin angle  
		//int m_tireJacobianRowIndex;		   // index to the jacobian row that calculated the tire last force.	
		int	m_tireUseConvexCastMode;      // default to false (can be set to true for fast LOD cars)
		int m_tireCastSide;
		int m_sideHit;
		int m_isBraking;
		int m_tireIsOnAir;
	};


	CustomDGRayCastCar(int maxTireCount, const dMatrix& chassisMatrix, NewtonBody* carBody);
	virtual ~CustomDGRayCastCar();

	dFloat GetSpeed() const;
	int GetTiresCount() const;
	int GetVehicleOnAir() const;
	int GetTireOnAir(int index) const;
	Tire& GetTire (int index) const;
	dFloat GetParametricPosition (int index) const;

	virtual void SetBrake (dFloat torque);
	virtual void SetTorque (dFloat torque);
	virtual void SetSteering (dFloat angle);


	void AddSingleSuspensionTire (void* userData, const dVector& localPosition, 
		dFloat mass, dFloat radius, dFloat with, dFloat friction, dFloat suspensionLenght, dFloat springConst, dFloat springDamper,
		int castMode);

	void SetTireMaxRPS (int tireIndex, dFloat maxTireRPS);

	const dMatrix& GetChassisMatrixLocal () const;
	const NewtonCollision* GetTiresShape (int tireIndex) const;
	dMatrix CalculateTireMatrix (int tire) const;
	dMatrix CalculateSuspensionMatrix (int tire, dFloat param) const;

	void SetCustomTireBrake (int index, dFloat torque);
	void SetCustomTireTorque (int index, dFloat torque);
	void SetCustomTireSteerAngleForce (int index, dFloat angle, dFloat turnforce);
	//
	dFloat GenerateTiresBrake (dFloat value);
	dFloat GenerateTiresTorque (dFloat value);
	dFloat GenerateTiresSteerForce (dFloat value);
	dFloat GenerateTiresSteerAngle (dFloat value);
	//
	void SetVarTireMovePointForceFront (int index, dFloat distance);
	void SetVarTireMovePointForceRight (int index, dFloat distance);
	void SetVarTireMovePointForceUp (int index, dFloat distance);
	//
	void SetVarFixDeceleration (dFloat value);
	void SetVarChassisRotationLimit (dFloat value);
	void SetVarMaxSteerAngle (dFloat value);
	void SetVarMaxSteerRate (dFloat value);
	void SetVarMaxSteerForceRate (dFloat value);
	void SetVarMaxSteerForce (dFloat value);
	void SetVarMaxSteerSpeedRestriction (dFloat value);
	void SetVarMaxBrakeForce (dFloat value);
	void SetVarMaxTorque (dFloat value);
	void SetVarMaxTorqueRate (dFloat value);
	void SetVarEngineSteerDiv (dFloat value);
	void SetVarTireSuspenssionHardLimit (int index, dFloat value);
	void SetVarTireFriction (int index, dFloat value);

protected:

	static unsigned ConvexCastPrefilter(const NewtonBody* body, const NewtonCollision* collision, void* userData);
	dFloat CalculateNormalizeForceVsSlipAngle (const Tire& tire, float slipAngle) const;
	void CalculateTireCollision (Tire& tire, int threadIndex) const;

	virtual void SubmitConstrainst (dFloat timestep, int threadIndex);




	virtual void GetInfo (NewtonJointRecord* info) const;


	void ApplyTireForces (const dMatrix& chassisMatrix, dFloat tiemStep) const;
	void ApplySuspensionForces (const dMatrix& chassisMatrix, dFloat tiemStep) const;
	void ApplyTireFrictionModel(const dMatrix& chassisMatrix, dFloat timestep);
	void ApplyOmegaCorrection();
	dFloat ApplySuspenssionLimit(Tire& tire);
	void ApplyChassisForceAndTorque(const dVector& vForce, const dVector& vPoint);
	void ApplyChassisTorque(const dVector& vForce, const dVector& vPoint);
	void ApplyDeceleration(Tire& tire);
	void ApplyTiresTorqueVisual(Tire& tire, dFloat timestep, int threadIndex);
	void ApplyTireFrictionVelocitySiding(Tire& tire,const dMatrix& chassisMatrix,const dVector& tireAxelVeloc, const dVector& tireAxelPosit, dFloat timestep, dFloat invTimestep, int& longitudinalForceIndex);

	dFloat m_engineTireTorque;
	dFloat m_steerAngle;
	dFloat m_maxSteerAngle;
	dFloat m_maxSteerRate;
	dFloat m_maxSteerForceRate;
	dFloat m_maxSteerForce;
	dFloat m_maxBrakeForce;
	dFloat m_engineSteerDiv;
	dFloat m_engineTorqueDiv; 
	dFloat m_maxTorque;
	dFloat m_maxTorqueRate;
	dFloat m_maxSteerSpeedRestriction;
	dFloat m_aerodynamicDrag;       // coefficient of aerodynamics drag  
	dFloat m_aerodynamicDownForce;  // coefficient of aerodynamics down force (inverse lift)
	dFloat m_chassisRotationLimit;
	dFloat m_fixDeceleration;
	dVector m_chassisOmega;         // chassis omega correction
	dVector m_chassisVelocity;      // chassis velocity correction
	dVector m_chassisTorque;        // chassis Torque Global
	int m_tiresRollSide;            // visual rolling side
	int m_vehicleOnAir;

	dMatrix m_bodyChassisMatrix;
	dMatrix m_localFrame;			// local coordinate system of the vehicle
	dFloat m_mass;
	dFloat m_curSpeed;
	int m_tiresCount;				// current number of tires
	dVector m_gravity;
	Tire* m_tires;					// tires array
};

#endif