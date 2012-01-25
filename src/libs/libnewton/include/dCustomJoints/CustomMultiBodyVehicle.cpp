#include "CustomJointLibraryStdAfx.h"
#include "CustomMultiBodyVehicle.h"


#define MIN_JOINT_PIN_LENGTH	50.0f



class CustomMultiBodyVehicleTire: public NewtonCustomJoint  
{
	public:
	CustomMultiBodyVehicleTire(
		const NewtonBody* hubBody, 
		const NewtonBody* tire, 
		dFloat suspensionLength, dFloat springConst, dFloat damperConst, dFloat radio)
		:NewtonCustomJoint(6, hubBody, tire)
	{
		dFloat mass;
		dFloat Ixx;
		dFloat Iyy;
		dFloat Izz;
		dMatrix pinAndPivotFrame;
		
		m_radius = radio;
		m_steeAngle = 0.0f;
		m_brakeToque = 0.0f;
		m_enginetorque = 0.0f;
		m_angularDragCoef = 0.0f;
		m_spring = springConst;
		m_damper = damperConst;
		m_suspenstionSpan = suspensionLength;


		NewtonBodyGetMassMatrix(tire, &mass, &Ixx, &Iyy, &Izz);
		m_Ixx = Ixx;

//		dFloat mass0;
//		dFloat mass1;
//		NewtonBodyGetMassMatrix(hubBody, &mass1, &Ixx, &Iyy, &Izz);
//		m_effectiveSpringMass = mass0 * mass1 / (mass0 + mass1);
		NewtonBodyGetMassMatrix(hubBody, &mass, &Ixx, &Iyy, &Izz);
		m_effectiveSpringMass = mass * 0.25f;

		NewtonBodyGetMatrix(tire, &pinAndPivotFrame[0][0]);
		CalculateLocalMatrix (pinAndPivotFrame, m_chassisLocalMatrix, m_tireLocalMatrix);

		m_refChassisLocalMatrix = m_chassisLocalMatrix;
	}

	~CustomMultiBodyVehicleTire(void)
	{
	}

	void GetInfo (NewtonJointRecord* info) const
	{
	}

	dFloat GetSteerAngle () const
	{
		return m_steeAngle;
	}

	void SetSteerAngle (dFloat angle)
	{
		if (dAbs (angle - m_steeAngle) > 1.0e-4f) {
			m_steeAngle = angle;
			dMatrix rotation (dYawMatrix(m_steeAngle));
			m_chassisLocalMatrix = rotation * m_refChassisLocalMatrix;
		}
	}

	void SetTorque (dFloat torque)
	{
		m_enginetorque = torque;
	}

	void SetBrakeTorque (dFloat torque)
	{
		m_brakeToque = torque;
	}

	void SetAngulaRollingDrag (dFloat angularDampingCoef)
	{
		m_angularDragCoef = angularDampingCoef;		
	}


	void ProjectTireMatrix()
	{
		const NewtonBody* tire;
		const NewtonBody* chassis;

		dMatrix tireMatrix;
		dMatrix chassisMatrix;

		tire = m_body1; 
		chassis = m_body0;


		NewtonBodyGetMatrix(tire, &tireMatrix[0][0]);
		NewtonBodyGetMatrix(chassis, &chassisMatrix[0][0]);
		
		
		// project the tire matrix to the right space.
		dMatrix tireMatrixInGlobalSpace (m_tireLocalMatrix * tireMatrix);
		dMatrix chassisMatrixInGlobalSpace (m_chassisLocalMatrix * chassisMatrix);

		dFloat projectDist; 
		projectDist = (tireMatrixInGlobalSpace.m_posit - chassisMatrixInGlobalSpace.m_posit) % chassisMatrixInGlobalSpace.m_up;
		chassisMatrixInGlobalSpace.m_posit += chassisMatrixInGlobalSpace.m_up.Scale (projectDist);

		chassisMatrixInGlobalSpace.m_up = tireMatrixInGlobalSpace.m_right * chassisMatrixInGlobalSpace.m_front;
		chassisMatrixInGlobalSpace.m_up = chassisMatrixInGlobalSpace.m_up.Scale (1.0f / dSqrt (chassisMatrixInGlobalSpace.m_up % chassisMatrixInGlobalSpace.m_up));
		chassisMatrixInGlobalSpace.m_right = chassisMatrixInGlobalSpace.m_front * chassisMatrixInGlobalSpace.m_up;
		chassisMatrixInGlobalSpace.m_up.m_w = 0.0f;
		chassisMatrixInGlobalSpace.m_right.m_w = 0.0f;



		dMatrix projectedChildMatrixInGlobalSpace (m_tireLocalMatrix.Inverse() * chassisMatrixInGlobalSpace);
		NewtonBodySetMatrix(tire, &projectedChildMatrixInGlobalSpace[0][0]);

		
		dVector tireVeloc;
		dVector chassisCom; 
		dVector tireOmega;
		dVector chassisOmega; 
		dVector chassisVeloc; 

		// project the tire velocity
		NewtonBodyGetVelocity (tire, &tireVeloc[0]);
		NewtonBodyGetVelocity (chassis, &chassisVeloc[0]);
		NewtonBodyGetOmega (chassis, &chassisOmega[0]);

		NewtonBodyGetCentreOfMass (chassis, &chassisCom[0]);
		chassisCom = chassisMatrix.TransformVector(chassisCom);
		chassisVeloc += chassisOmega * (chassisMatrixInGlobalSpace.m_posit - chassisCom);

		dVector projTireVeloc (chassisVeloc - chassisMatrixInGlobalSpace.m_up.Scale (chassisVeloc % chassisMatrixInGlobalSpace.m_up));
		projTireVeloc += chassisMatrixInGlobalSpace.m_up.Scale(tireVeloc % chassisMatrixInGlobalSpace.m_up);
		NewtonBodySetVelocity(tire, &projTireVeloc[0]);

		// project angular velocity
		NewtonBodyGetOmega (tire, &tireOmega[0]);
		dVector projTireOmega (chassisOmega - chassisMatrixInGlobalSpace.m_front.Scale (chassisOmega % chassisMatrixInGlobalSpace.m_front));
		projTireOmega += chassisMatrixInGlobalSpace.m_front.Scale (tireOmega % chassisMatrixInGlobalSpace.m_front);
		NewtonBodySetOmega (tire, &projTireOmega[0]);
	}


	void SubmitConstrainst (dFloat timestep, int threadIndex)
	{
		dMatrix tirePivotMatrix;
		dMatrix chassisPivotMatrix;

		ProjectTireMatrix();

		// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
		CalculateGlobalMatrix (m_chassisLocalMatrix, m_tireLocalMatrix, chassisPivotMatrix, tirePivotMatrix);

		// Restrict the movement on the pivot point along all two orthonormal direction
		dVector centerInTire (tirePivotMatrix.m_posit);
		dVector centerInChassis (chassisPivotMatrix.m_posit + chassisPivotMatrix.m_up.Scale ((centerInTire - chassisPivotMatrix.m_posit) % chassisPivotMatrix.m_up));
		NewtonUserJointAddLinearRow (m_joint, &centerInChassis[0], &centerInTire[0], &chassisPivotMatrix.m_front[0]);
		NewtonUserJointAddLinearRow (m_joint, &centerInChassis[0], &centerInTire[0], &chassisPivotMatrix.m_right[0]);

		// get a point along the pin axis at some reasonable large distance from the pivot
		dVector pointInPinInTire (centerInChassis + chassisPivotMatrix.m_front.Scale(MIN_JOINT_PIN_LENGTH));
		dVector pointInPinInChassis (centerInTire + tirePivotMatrix.m_front.Scale(MIN_JOINT_PIN_LENGTH));
		NewtonUserJointAddLinearRow (m_joint, &pointInPinInTire[0], &pointInPinInChassis[0], &chassisPivotMatrix.m_right[0]);
		NewtonUserJointAddLinearRow (m_joint, &pointInPinInTire[0], &pointInPinInChassis[0], &chassisPivotMatrix.m_up[0]);

		//calculate the suspension spring and damper force
		dFloat dist;
		dFloat speed;
		dFloat force;

		dVector tireVeloc; 
		dVector chassisVeloc; 
		dVector chassisOmega; 
		dVector chassisCom; 
		dMatrix chassisMatrix; 
		
		const NewtonBody* tire;
		const NewtonBody* chassis;

		// calculate the velocity of tire attachments point on the car chassis
		
		tire = GetBody1(); 
		chassis = GetBody0();
		
		NewtonBodyGetVelocity (tire, &tireVeloc[0]);
		NewtonBodyGetVelocity (chassis, &chassisVeloc[0]);
		NewtonBodyGetOmega (chassis, &chassisOmega[0]);
		NewtonBodyGetMatrix (chassis, &chassisMatrix[0][0]);
		NewtonBodyGetCentreOfMass (chassis, &chassisCom[0]);
		chassisCom = chassisMatrix.TransformVector(chassisCom);
		chassisVeloc += chassisOmega * (centerInChassis - chassisCom);
		

		// get the spring damper parameters
		speed = (chassisVeloc - tireVeloc) % chassisPivotMatrix.m_up;
		dist = (chassisPivotMatrix.m_posit - tirePivotMatrix.m_posit) % chassisPivotMatrix.m_up;
		// check if the suspension pass the bumpers limits
		if (-dist > m_suspenstionSpan* 0.5f) {
			// if it hit the bumpers then speed is zero
			speed = 0;
			NewtonUserJointAddLinearRow (m_joint, &centerInChassis[0], &centerInChassis[0], &chassisPivotMatrix.m_up[0]);
			NewtonUserJointSetRowMinimumFriction(m_joint, 0.0f);
		} else if (dist > 0.0f) {
			// if it hit the bumpers then speed is zero
			speed = 0;
			NewtonUserJointAddLinearRow (m_joint, &centerInChassis[0], &centerInChassis[0], &chassisPivotMatrix.m_up[0]);
			NewtonUserJointSetRowMaximumFriction(m_joint, 0.0f);
		}

		// calculate magnitude of suspension force 
		force = NewtonCalculateSpringDamperAcceleration (timestep, m_spring, dist, m_damper, speed) * m_effectiveSpringMass;

		dVector chassisForce (chassisMatrix.m_up.Scale (force));
		dVector chassisTorque ((centerInChassis - chassisCom) * chassisForce);
		NewtonBodyAddForce (chassis, &chassisForce[0]);
		NewtonBodyAddTorque (chassis, &chassisTorque[0]);

		dVector tireForce (chassisForce.Scale (-1.0f));
		NewtonBodyAddForce(tire, &tireForce[0]);

		// apply the engine torque to tire torque
		dFloat relOmega;
		dMatrix tireMatrix;
		dVector tireOmega;
		
		NewtonBodyGetOmega(tire, &tireOmega[0]); 
		NewtonBodyGetMatrix (tire, &tireMatrix[0][0]); 

		relOmega = ((tireOmega - chassisOmega) % tireMatrix.m_front);

		// apply engine torque plus some tire angular drag
		dVector tireTorque (tireMatrix.m_front.Scale (m_enginetorque - relOmega * m_Ixx * m_angularDragCoef));
		NewtonBodyAddTorque (tire, &tireTorque[0]);

		dVector chassisReationTorque (chassisMatrix.m_right.Scale (- m_enginetorque));
		NewtonBodyAddTorque(chassis, &chassisTorque[0]);

		m_enginetorque = 0.0f;

		// add the brake torque row
		if (dAbs(m_brakeToque) > 1.0e-3f) {
			
			relOmega /= timestep;
			NewtonUserJointAddAngularRow (m_joint, 0.0f, &tireMatrix.m_front[0]);
			NewtonUserJointSetRowAcceleration(m_joint, relOmega);
			NewtonUserJointSetRowMaximumFriction(m_joint, m_brakeToque);
			NewtonUserJointSetRowMinimumFriction(m_joint, -m_brakeToque);
		}
		m_brakeToque = 0.0f;

	}

	dMatrix m_tireLocalMatrix;			// body1 is the tire 
	dMatrix m_chassisLocalMatrix;		// body0 is the chassis      
	dMatrix m_refChassisLocalMatrix;

	dFloat m_Ixx;
	dFloat m_radius;
	dFloat m_spring;
	dFloat m_damper;
	
	dFloat m_steeAngle;
	dFloat m_brakeToque;
	dFloat m_enginetorque;
	dFloat m_angularDragCoef;
	dFloat m_effectiveSpringMass;
	dFloat m_suspenstionSpan;
	
};


class CustomMultiBodyVehicleAxleDifferencial: public NewtonCustomJoint  
{
	public:
	CustomMultiBodyVehicleAxleDifferencial (
		CustomMultiBodyVehicleTire* leftTire, 
		CustomMultiBodyVehicleTire* rightTire,
		dFloat maxFriction)
		:NewtonCustomJoint(1, leftTire->GetBody1(), rightTire->GetBody1())
	{
		_ASSERTE (rightTire->GetBody0() == leftTire->GetBody0());

		m_maxFrition = dAbs (maxFriction);
		m_chassis = rightTire->GetBody0();
		m_leftTire = leftTire;
		m_rightTire = rightTire;

	}

	void SubmitConstrainst (dFloat timestep, int threadIndex)
	{
		dFloat den;
		dFloat relAccel; 
		dFloat jacobian0[6];
		dFloat jacobian1[6];

		dMatrix leftMatrix;
		dMatrix rightMatrix;
		dMatrix chassisMatrix;

		NewtonBodyGetMatrix (m_chassis, &chassisMatrix[0][0]); 
		NewtonBodyGetMatrix (m_leftTire->GetBody1(), &leftMatrix[0][0]); 
		NewtonBodyGetMatrix (m_rightTire->GetBody1(), &rightMatrix[0][0]); 

		// calculate the geometrical turn radius of for this axle 

		dVector leftOrigin (chassisMatrix.UntransformVector(leftMatrix.m_posit));
		dVector rightOrigin (chassisMatrix.UntransformVector(rightMatrix.m_posit));
		dVector axleCenter ((rightOrigin + leftOrigin).Scale (0.5f));

		dVector tireAxisDir (chassisMatrix.UnrotateVector((leftMatrix.m_front + rightMatrix.m_front).Scale (0.5f)));
		axleCenter.m_y = 0.0f;
		tireAxisDir.m_y = 0.0f;

		dVector sideDir (0.0f, 0.0f, 1.0f, 0.0f);
		dVector deltaDir (tireAxisDir - sideDir);
		relAccel = 0.0f;
		den = deltaDir % deltaDir;
		if (den > 1.0e-6f) {
			dFloat R; 
			dFloat num; 
			dFloat ratio; 
			dFloat wl; 
			dFloat wr; 
			dFloat rl; 
			dFloat rr; 
			dFloat relOmega; 

			num = axleCenter % deltaDir;
			R = - num / den;

			rr = (rightOrigin % sideDir);
			rl = (leftOrigin % sideDir);
			ratio = (R + rr) / (R + rl);

			dVector omegaLeft;
			dVector omegaRight;
			// calculate the angular velocity for both bodies
			NewtonBodyGetOmega(m_leftTire->GetBody1(), &omegaLeft[0]);
			NewtonBodyGetOmega(m_rightTire->GetBody1(), &omegaRight[0]);

			// get angular velocity relative to the pin vector
			wl = -(omegaLeft % leftMatrix.m_front);
			wr = omegaRight % rightMatrix.m_front;

			// establish the gear equation.
			relOmega = wl + ratio * wr;
			relAccel = - 0.5f * relOmega / timestep;
		}
		
		jacobian0[0] = 0.0f;
		jacobian0[1] = 0.0f;
		jacobian0[2] = 0.0f;
		jacobian0[3] = leftMatrix.m_front.m_x * -1.0f;
		jacobian0[4] = leftMatrix.m_front.m_y * -1.0f;
		jacobian0[5] = leftMatrix.m_front.m_z * -1.0f;

		jacobian1[0] = 0.0f;
		jacobian1[1] = 0.0f;
		jacobian1[2] = 0.0f;
		jacobian1[3] = rightMatrix.m_front.m_x;
		jacobian1[4] = rightMatrix.m_front.m_y;
		jacobian1[5] = rightMatrix.m_front.m_z;
		
		NewtonUserJointAddGeneralRow (m_joint, jacobian0, jacobian1);
		NewtonUserJointSetRowAcceleration (m_joint, relAccel);
		NewtonUserJointSetRowMaximumFriction(m_joint, m_maxFrition);
		NewtonUserJointSetRowMinimumFriction(m_joint, -m_maxFrition);

	}

	dFloat m_maxFrition;
	const NewtonBody* m_chassis;
	CustomMultiBodyVehicleTire* m_leftTire;
	CustomMultiBodyVehicleTire* m_rightTire;

};



CustomMultiBodyVehicle::CustomMultiBodyVehicle(const dVector& frontDir, const dVector& upDir, const NewtonBody* carBody)
	:NewtonCustomJoint(1, carBody, NULL)
{
	dVector com;
	dMatrix tmp;
	dMatrix chassisMatrix;

	m_tiresCount = 0;
	m_diffencialCount = 0;

	NewtonBodyGetMatrix(m_body0, &tmp[0][0]);
	NewtonBodyGetCentreOfMass(m_body0, &com[0]);
	com.m_w = 1.0f;

	// set the joint reference point at the center of mass of the body
	chassisMatrix.m_front = frontDir;
	chassisMatrix.m_up = upDir;
	chassisMatrix.m_right = frontDir * upDir;
	chassisMatrix.m_posit = tmp.TransformVector(com);

	chassisMatrix.m_front.m_w = 0.0f;
	chassisMatrix.m_up.m_w = 0.0f;
	chassisMatrix.m_right.m_w = 0.0f;
	chassisMatrix.m_posit.m_w = 1.0f;
	CalculateLocalMatrix (chassisMatrix, m_localFrame, tmp);
}

CustomMultiBodyVehicle::~CustomMultiBodyVehicle(void)
{
	// the joint do not need to be destroyed because the joint destructor takes care of that
}


void CustomMultiBodyVehicle::SubmitConstrainst (dFloat timestep, int threadIndex)
{

}

void CustomMultiBodyVehicle::GetInfo (NewtonJointRecord* info) const
{

}


void CustomMultiBodyVehicle::ApplyTorque (dFloat torque)
{
}

void CustomMultiBodyVehicle::ApplySteering (dFloat angle)
{
}

void CustomMultiBodyVehicle::ApplyBrake (dFloat brakeTorque)
{
}

dFloat CustomMultiBodyVehicle::GetSetTireSteerAngle (int index) const
{
	return m_tires[index]->GetSteerAngle ();
}

void CustomMultiBodyVehicle::ApplyTireSteerAngle (int index, dFloat angle)
{
	m_tires[index]->SetSteerAngle (angle);
}

void CustomMultiBodyVehicle::ApplyTireBrake (int index, dFloat brakeTorque)
{
	m_tires[index]->SetBrakeTorque (brakeTorque);
}

void CustomMultiBodyVehicle::ApplyTireTorque (int index, dFloat torque)
{
	m_tires[index]->SetTorque (torque);	
}

void CustomMultiBodyVehicle::ApplyTireRollingDrag (int index, dFloat angularDampingCoef)
{
	m_tires[index]->SetAngulaRollingDrag (angularDampingCoef);
}

int CustomMultiBodyVehicle::GetTiresCount() const 
{
	return m_tiresCount;
}

const NewtonBody* CustomMultiBodyVehicle::GetTireBody(int tireIndex) const
{
	return m_tires[tireIndex]->GetBody1();
}



dFloat CustomMultiBodyVehicle::GetSpeed() const
{
	dVector veloc;
	dMatrix chassisMatrix;

	NewtonBodyGetMatrix (m_body0, &chassisMatrix[0][0]); 
	NewtonBodyGetVelocity(m_body0, &veloc[0]);
	return veloc % chassisMatrix.m_front;
}


int CustomMultiBodyVehicle::AddSingleSuspensionTire (
	void* userData, 
	const dVector& localPosition, 
	dFloat mass, 
	dFloat radius, 
	dFloat width,
	dFloat suspensionLength, 
	dFloat springConst, 
	dFloat springDamper)
{
	dFloat Ixx;
	dFloat Iyy;
	dFloat Izz;
	dMatrix carMatrix;
	NewtonBody* tire;
	NewtonWorld* world;
	NewtonCollision *collision;

	world = NewtonBodyGetWorld(GetBody0());

	// create the tire RogidBody 
	collision = NewtonCreateChamferCylinder(world, radius, width, NULL);

	//create the rigid body
	tire = NewtonCreateBody (world, collision);

	// release the collision
	NewtonReleaseCollision (world, collision);	

	// save the user data
	NewtonBodySetUserData (tire, userData);

	// set the material group id for vehicle
	NewtonBodySetMaterialGroupID (tire, 0);
//	NewtonBodySetMaterialGroupID (tire, woodID);

	// set the force and torque call back function
	NewtonBodySetForceAndTorqueCallback (tire, NewtonBodyGetForceAndTorqueCallback (GetBody0()));

	// body part do not collision
	NewtonBodySetJointRecursiveCollision (tire, 0);

	// calculate the moment of inertia and the relative center of mass of the solid
	dVector origin;
	dVector inertia;
	NewtonConvexCollisionCalculateInertialMatrix (collision, &inertia[0], &origin[0]);	
	Ixx = mass * inertia[0];
	Iyy = mass * inertia[1];
	Izz = mass * inertia[2];

	// set the mass matrix
	NewtonBodySetMassMatrix (tire, mass, Ixx, Iyy, Izz);

	// calculate the tire local base pose matrix
	dMatrix tireMatrix;
	tireMatrix.m_front = m_localFrame.m_right;
	tireMatrix.m_up = m_localFrame.m_up;
	tireMatrix.m_right = tireMatrix.m_front * tireMatrix.m_up;
	tireMatrix.m_posit = localPosition;
	NewtonBodyGetMatrix(GetBody0(), &carMatrix[0][0]);
	tireMatrix = tireMatrix * carMatrix;

	// set the matrix for both the rigid body and the graphic body
	NewtonBodySetMatrix (tire, &tireMatrix[0][0]);

	// add a single tire
	m_tires[m_tiresCount] = new CustomMultiBodyVehicleTire (GetBody0(), tire, suspensionLength, springConst, springDamper, radius);
	m_tiresCount ++;

	return m_tiresCount - 1;
}

int CustomMultiBodyVehicle::AddSlipDifferencial (int leftTireIndex, int rightTireIndex, dFloat maxFriction)
{
	m_differencials[m_diffencialCount] = new CustomMultiBodyVehicleAxleDifferencial (m_tires[leftTireIndex], m_tires[rightTireIndex], maxFriction);
	m_diffencialCount ++;
	return m_diffencialCount - 1;
}


