#include "CustomJointLibraryStdAfx.h"
#include "CustomPlayerController.h"

#define USE_OMEGA_BASED_TURNS

#define SENSOR_SHAPE_SEGMENTS	 32
#define MAX_COLLISIONS_ITERATION 8

#define MAX_PLAYER_SPHAPE_PADD_FACTOR	 0.01f

CustomPlayerController::CustomPlayerController(
	const dVector& pin, 
	const NewtonBody* child, 
	dFloat maxStairStepFactor)
	:NewtonCustomJoint(6, child, NULL)
{
	dFloat radius; 
	dFloat sensorHigh; 
	dFloat staticRadiusFactor;
	dFloat dynamicRadiusFactor;
	dFloat floorFinderRadiusFactor;
	dMatrix pivotMatrix;
	NewtonCollision* shape;

	m_heading = 0.0f;
	m_sideSpeed = 0.0f;
	m_forwardSpeed = 0.0f;
	m_playerHeight = 0.0f;
	m_stairHeight = 0.0f;
	m_isInJumpState = 0;
	m_restitution = 0.0f;

//	SetMaxSlope (25.0f * 3.141592f / 180.0f);
	SetMaxSlope (45.0f * 3.141592f / 180.0f);

	NewtonBodyGetMatrix(child, &pivotMatrix[0][0]);

	dMatrix matrix (dgGrammSchmidt(pin));
	matrix.m_posit = pivotMatrix.m_posit;

	CalculateLocalMatrix (matrix, m_localMatrix0, m_localMatrix1);

	// calculate the dimensions of the Player internal auxiliary shapes
	shape = NewtonBodyGetCollision(child);
	dVector localPin (pivotMatrix.UnrotateVector(pin));
	matrix = dgGrammSchmidt(localPin);

	dVector top;
	dVector bottom;
	NewtonCollisionSupportVertex (shape, &matrix[0][0], &top[0]);
	dVector downDir (matrix[0].Scale (-1.0f));
	NewtonCollisionSupportVertex (shape, &downDir[0], &bottom[0]);
	m_playerHeight = (top - bottom) % matrix[0];

	// make the player hight a bit larger
	m_playerHeight += (m_playerHeight * MAX_PLAYER_SPHAPE_PADD_FACTOR);

	// set stairs step high as a percent of the player high
	m_stairHeight = m_playerHeight * maxStairStepFactor;
	

	// calculate the radius
	dFloat r1;
	dFloat r2;
	dVector radiusVector1;
	dVector radiusVector2;
	NewtonCollisionSupportVertex (shape, &matrix[1][0], &radiusVector1[0]);
	NewtonCollisionSupportVertex (shape, &matrix[2][0], &radiusVector2[0]);
	r1 = radiusVector1 % matrix[1];
	r2 = radiusVector1 % matrix[1];
	radius = r1 > r2 ? r1 : r2;

	
	dVector verticalSensorPoints[SENSOR_SHAPE_SEGMENTS * 2];
	dVector horizontalSensorPoints[SENSOR_SHAPE_SEGMENTS * 2];
	dVector dynamicsSensorPoints[SENSOR_SHAPE_SEGMENTS * 2];
	

	staticRadiusFactor = 1.125f;
	dynamicRadiusFactor = 1.5f;
	floorFinderRadiusFactor = 1.0f;
	
	sensorHigh = (m_playerHeight - m_stairHeight) * 0.5f;
	for (int i = 0; i < SENSOR_SHAPE_SEGMENTS; i ++) {
		dFloat x;
		dFloat z;
		x = radius * dCos (2.0f * 3.141592 * dFloat(i) / dFloat(SENSOR_SHAPE_SEGMENTS));
		z = radius * dSin (2.0f * 3.141592 * dFloat(i) / dFloat(SENSOR_SHAPE_SEGMENTS));

		dynamicsSensorPoints[i].m_x = x * dynamicRadiusFactor;		
		dynamicsSensorPoints[i].m_y = m_playerHeight * 0.45f;
		dynamicsSensorPoints[i].m_z = z * dynamicRadiusFactor;
		dynamicsSensorPoints[i + SENSOR_SHAPE_SEGMENTS].m_x =  dynamicsSensorPoints[i].m_x;
		dynamicsSensorPoints[i + SENSOR_SHAPE_SEGMENTS].m_y = -dynamicsSensorPoints[i].m_y;
		dynamicsSensorPoints[i + SENSOR_SHAPE_SEGMENTS].m_z =  dynamicsSensorPoints[i].m_z;

		verticalSensorPoints[i].m_x = x * floorFinderRadiusFactor;	
		verticalSensorPoints[i].m_y = sensorHigh;
		verticalSensorPoints[i].m_z = z * floorFinderRadiusFactor;
		verticalSensorPoints[i + SENSOR_SHAPE_SEGMENTS].m_x =  verticalSensorPoints[i].m_x;
		verticalSensorPoints[i + SENSOR_SHAPE_SEGMENTS].m_y = -verticalSensorPoints[i].m_y;
		verticalSensorPoints[i + SENSOR_SHAPE_SEGMENTS].m_z =  verticalSensorPoints[i].m_z;

		horizontalSensorPoints[i].m_x = x * staticRadiusFactor;	
		horizontalSensorPoints[i].m_y = sensorHigh;
		horizontalSensorPoints[i].m_z = z * staticRadiusFactor;
		horizontalSensorPoints[i + SENSOR_SHAPE_SEGMENTS].m_x =  horizontalSensorPoints[i].m_x;
		horizontalSensorPoints[i + SENSOR_SHAPE_SEGMENTS].m_y = -horizontalSensorPoints[i].m_y;
		horizontalSensorPoints[i + SENSOR_SHAPE_SEGMENTS].m_z =  horizontalSensorPoints[i].m_z;
	}
	m_verticalSensorShape = NewtonCreateConvexHull (m_world, SENSOR_SHAPE_SEGMENTS * 2, &verticalSensorPoints[0].m_x, sizeof (dVector), 0.0f, NULL);
	m_horizontalSensorShape = NewtonCreateConvexHull (m_world, SENSOR_SHAPE_SEGMENTS * 2, &horizontalSensorPoints[0].m_x, sizeof (dVector), 0.0f, NULL);
	m_dynamicsCollisionShape = NewtonCreateConvexHull (m_world, SENSOR_SHAPE_SEGMENTS * 2, &dynamicsSensorPoints[0].m_x, sizeof (dVector), 0.0f, NULL);
}

CustomPlayerController::~CustomPlayerController()
{
	NewtonReleaseCollision(m_world, m_verticalSensorShape);
	NewtonReleaseCollision(m_world, m_horizontalSensorShape);
	NewtonReleaseCollision(m_world, m_dynamicsCollisionShape);
}

const NewtonCollision* CustomPlayerController::GetVerticalSensorShape () const
{
	return m_verticalSensorShape;
}

const NewtonCollision* CustomPlayerController::GetHorizontalSensorShape () const
{
	return m_horizontalSensorShape;
}

const NewtonCollision* CustomPlayerController::GetDynamicsSensorShape () const
{
	return m_dynamicsCollisionShape;
}

void CustomPlayerController::SetMaxSlope (dFloat maxSlopeAngleIndRadian)
{
	m_maxSlope = dCos (maxSlopeAngleIndRadian);
}

dFloat CustomPlayerController::GetMaxSlope () const
{
	return dAcos (m_maxSlope);
}

void CustomPlayerController::GetInfo (NewtonJointRecord* info) const
{
}

void CustomPlayerController::SetVelocity (dFloat forwardSpeed, dFloat sideSpeed, dFloat heading)
{
	m_heading = heading;
	m_sideSpeed = sideSpeed;
	m_forwardSpeed = forwardSpeed;
}


unsigned CustomPlayerController::ConvexStaticCastPrefilter(const NewtonBody* body, const NewtonCollision* collision, void* userData)
{
/*
	// for now just collide with static bodies
//	dFloat mass; 
//	dFloat Ixx; 
//	dFloat Iyy; 
//	dFloat Izz; 
	FilterData* filterData;

	filterData = (FilterData*) userData;
	for (int i =0; i < filterData->m_count;i ++){
		if (filterData->m_filter[i] == body) {
			return 0;
		}
	}

//	NewtonBodyGetMassMatrix(body, &mass, &Ixx, &Iyy, &Izz);
//	return (mass == 0.0f);
	return 1;
*/

	dFloat mass; 
	dFloat Ixx; 
	dFloat Iyy; 
	dFloat Izz; 
	NewtonBodyGetMassMatrix(body, &mass, &Ixx, &Iyy, &Izz);
	return (mass == 0.0f);
}



unsigned CustomPlayerController::ConvexDynamicCastPrefilter(const NewtonBody* body, const NewtonCollision* collision, void* userData)
{
	// for now just collide with static bodies
	dFloat mass; 
	dFloat Ixx; 
	dFloat Iyy; 
	dFloat Izz; 
	FilterData* filterData;

	filterData = (FilterData*) userData;
	for (int i =0; i < filterData->m_count;i ++){
		if (filterData->m_filter[i] == body) {
			return 0;
		}
	}
	NewtonBodyGetMassMatrix(body, &mass, &Ixx, &Iyy, &Izz);
	return (mass > 0.0f);
}

unsigned CustomPlayerController::ConvexAllBodyCastPrefilter(const NewtonBody* body, const NewtonCollision* collision, void* userData)
{
	FilterData* filterData;

	filterData = (FilterData*) userData;
	for (int i =0; i < filterData->m_count;i ++){
		if (filterData->m_filter[i] == body) {
			return 0;
		}
	}
	return 1;
}



dFloat CustomPlayerController::FindFloorCallback(const NewtonBody* body, const dFloat* hitNormal, int collisionID, void* userData, dFloat intersetParam)
{
	dFloat param;

	param = 1.0f;
	FindFloorData& data = *((FindFloorData*)userData);
	if (body != data.m_me) {
		param = intersetParam;
		if (param < data.m_param) {
			data.m_param = param;
			data.m_normal.m_x = hitNormal[0];
			data.m_normal.m_y = hitNormal[1];
			data.m_normal.m_z = hitNormal[2];
			data.m_hitBody = body;
		}
	}

	return param;
}


void CustomPlayerController::SubmitConstrainst (dFloat timestep, int threadIndex)
{
//	_ASSERTE (0);

	dFloat mag; 
	dFloat angle; 
	dFloat mass;
	dFloat Ixx;
	dFloat Iyy;
	dFloat Izz;
	dFloat timestepInv; 
	dFloat turnAngle;
	dFloat turnOmega;
	dMatrix matrix0;
	
	dVector omega;
	dVector veloc;
	dVector extForce;
	dVector extTorque;
	dMatrix bodyMatrix;

	// Get the global matrices of each rigid body.
	NewtonBodyGetMatrix(m_body0, &bodyMatrix[0][0]);
	matrix0 = m_localMatrix0 * bodyMatrix;
	const dMatrix matrix1 = m_localMatrix1;

//_ASSERTE (bodyMatrix[0].m_x >= 0.999f);

	// get mass properties of the body
	NewtonBodyGetMassMatrix(m_body0, &mass, &Ixx, &Iyy, &Izz);

	// get linear velocity
	NewtonBodyGetVelocity(m_body0, &veloc[0]);

	// enlarge the time step to no overshot too much
	timestepInv = 1.0f / timestep;

	// if the body has rotated by some amount, the there will be a plane of rotation
	dVector lateralDir (matrix0.m_front * matrix1.m_front);
	mag = lateralDir % lateralDir;
	if (mag > 1.0e-6f) {
		// if the side vector is not zero, it means the body has rotated
		mag = dSqrt (mag);
		lateralDir = lateralDir.Scale (1.0f / mag);
		angle = dAsin (mag);

		// add an angular constraint to correct the error angle
		NewtonUserJointAddAngularRow (m_joint, angle, &lateralDir[0]);

		// in theory only one correction is needed, but this produces instability as the body may move sideway.
		// a lateral correction prevent this from happening.
		dVector frontDir (lateralDir * matrix1.m_front);
		NewtonUserJointAddAngularRow (m_joint, 0.0f, &frontDir[0]);
	} else {
		// if the angle error is very small then two angular correction along the plane axis do the trick
		NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_up[0]);
		NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_right[0]);
	}


	// Note:
	// changing the velocity was not allowed in previews versions of newton.
	// this is still incorrect both physically and and for the engine
	// but it is better tolerated the engine now.

	//calculate the turn angle and omega
	dVector heading (dCos (m_heading), 0.0f, dSin (m_heading), 0.0f);
	turnAngle = min (max ((bodyMatrix.m_front * heading).m_y, -1.0f), 1.0f);
	turnAngle = dAsin (turnAngle); 
	turnOmega = turnAngle * timestepInv;

#ifdef USE_OMEGA_BASED_TURNS
	omega = bodyMatrix.m_up.Scale (turnOmega);
	NewtonBodySetOmega(m_body0, &omega.m_x);

#else
	// Calculate the heading Torque 
	NewtonBodyGetOmega(m_body0, &omega[0]);
	NewtonBodyGetTorqueAcc(m_body0, &extTorque[0]);
	extTorque = bodyMatrix.m_up.Scale ((extTorque % bodyMatrix.m_up));
	dVector torque (bodyMatrix.m_up.Scale ((turnOmega - omega.m_y) * Iyy * timestepInv - extTorque.m_y));
	NewtonBodyAddTorque(m_body0, &torque[0]);
#endif

	// check if the play is not on any illegal slope
	FindFloorData findFloor (m_body0);
	dVector p1 (bodyMatrix.m_posit - bodyMatrix.m_up.Scale (m_playerHeight));
	NewtonWorldRayCast (m_world, &bodyMatrix.m_posit[0], &p1[0], FindFloorCallback, &findFloor, NULL);

	if (findFloor.m_normal[1] > m_maxSlope) {
		// calculate the linear force
		//	dVector force ()
		dVector desiredVeloc (bodyMatrix.m_front.Scale (m_forwardSpeed) + bodyMatrix.m_up.Scale (veloc.m_y) + bodyMatrix.m_right.Scale (m_sideSpeed));
		if (m_isInJumpState) {
			// if in jump state, move by the gravity only
			desiredVeloc = veloc;
		}

		// if we hit a body the we need to consider its velocity
		if (findFloor.m_hitBody) {
			dVector hitVeloc; 
			dVector hitOmega; 
			dVector hitPoint; 
			dMatrix hitMatrix; 

			hitPoint = bodyMatrix.m_posit + (p1 - bodyMatrix.m_posit).Scale (findFloor.m_param);
			NewtonBodyGetOmega(findFloor.m_hitBody, &hitOmega[0]);
			NewtonBodyGetVelocity(findFloor.m_hitBody, &hitVeloc[0]);
			NewtonBodyGetMatrix(findFloor.m_hitBody, &hitMatrix[0][0]);

			// I am not using the com here (maybe i should)
			hitVeloc += hitOmega * (hitPoint - hitMatrix.m_posit);
			desiredVeloc += hitVeloc;
		}

		// look ahead for obstacles in along the desire velocity
		int iterations;
		dMatrix startCasting (bodyMatrix);
		dVector horizontalVelocity (desiredVeloc);
		horizontalVelocity.m_y = 0.0f;

		#define MAX_CONTACTS 16
		int contacts;
		NewtonWorldConvexCastReturnInfo info[MAX_CONTACTS];
		FilterData staticFilterData (m_body0);

		// check for collision with other bodies
		dVector destination;
		destination = startCasting.m_posit + horizontalVelocity.Scale (timestep);
//		contacts = NewtonWorldConvexCast (m_world, &startCasting[0][0], &destination[0], m_dynamicsCollisionShape, &staticFilterData, ConvexDynamicCastPrefilter, info, sizeof (info) / sizeof (info[0]));
		dFloat param;
		contacts = NewtonWorldConvexCast (m_world, &startCasting[0][0], &destination[0], m_dynamicsCollisionShape, &param, &staticFilterData, ConvexDynamicCastPrefilter, info, sizeof (info) / sizeof (info[0]), threadIndex);
		for (iterations = 0; contacts && (iterations < MAX_COLLISIONS_ITERATION); iterations ++) {
			int bodyCounts;
			NewtonBody const* hitBodies[MAX_CONTACTS]; 
			dVector hitVeloc[MAX_CONTACTS]; 
			dVector hitOmega[MAX_CONTACTS]; 
			int velocCorrection;
			dFloat time;

			// save the state of hit bodies
			bodyCounts = 1;
			hitBodies[0] = info[0].m_hitBody;
			NewtonBodyGetOmega (hitBodies[0], &hitOmega[0][0]);
			NewtonBodyGetVelocity (hitBodies[0], &hitVeloc[0][0]);
			for (int i = 1; i < contacts; i ++) {
				int j; 
				for (j = 0; j < bodyCounts; j ++) {
					if (hitBodies[j] == info[i].m_hitBody) {
						break;
					}
				}
				if (j == bodyCounts) {
					hitBodies[bodyCounts] = info[i].m_hitBody;
					NewtonBodyGetOmega (hitBodies[bodyCounts], &hitOmega[bodyCounts][0]);
					NewtonBodyGetVelocity (hitBodies[bodyCounts], &hitVeloc[bodyCounts][0]);
					bodyCounts ++;
				}
			}

			// if the player will hit an obstacle in this direction then we need to change the velocity direction
			// we will declare a collision at the player origin.
			//dVector posit (startCasting.m_posit + (destination - startCasting.m_posit).Scale(info[0].m_intersectionParam));
			dVector posit (startCasting.m_posit + (destination - startCasting.m_posit).Scale(param));

			// calculate the travel time, and subtract from time remaining
			time = ((posit - startCasting.m_posit) % horizontalVelocity) / (horizontalVelocity % horizontalVelocity) * 0.9f;

			velocCorrection = 0;
			for (int i = 0; i < contacts; i ++) {
				dFloat mass1;
				dFloat Ixx;
				dFloat Iyy;
				dFloat Izz;
				const NewtonBody* body;

				// flatten contact normal
				dVector normal (info[i].m_normal);
				normal.m_y = 0.0f;
				normal = normal.Scale (1.0f / dSqrt (normal % normal));

				body = info[i].m_hitBody;
				NewtonBodyGetMassMatrix(body, &mass1, &Ixx, &Iyy, &Izz);
				if (!CanPushBody (body)) {
					dFloat reboundVeloc;
					dFloat penetrationVeloc;
					// calculate the reflexion velocity
					penetrationVeloc = -0.5f * timestepInv * ((info[i].m_penetration > 0.1f) ? 0.1f : info[i].m_penetration);

					reboundVeloc = (horizontalVelocity % normal) * (1.0f + m_restitution) + penetrationVeloc;
					if (reboundVeloc < 0.0f) {
						velocCorrection = 1;
						horizontalVelocity -= normal.Scale(reboundVeloc);
					}
				} else {
					dFloat relVeloc;
					dFloat projVeloc;
					dFloat massWeigh;
					dFloat momentumDamper;
					dFloat playerNormalVelocity;
					dVector veloc;
					dVector omega;
					dMatrix matrix;
					dVector comLocal;

					NewtonBodyGetOmega (body, &omega[0]);
					NewtonBodyGetVelocity (body, &veloc[0]);
					NewtonBodyGetMatrix (body, &matrix[0][0]);
					NewtonBodyGetCentreOfMass (body, &comLocal[0]);

					// calculate local point relative to center of mass
					dVector point (info[i].m_point);
					dVector com (matrix.TransformVector(comLocal));
					point.m_y = com.m_y;
					dVector pointVeloc (veloc + omega * (point - com));
					//dVector pointVeloc (veloc);

					massWeigh = mass / (mass + mass1);
					if (massWeigh > 0.5f) {
						massWeigh = 0.5f;
					}

					projVeloc = pointVeloc % normal;
					playerNormalVelocity = (horizontalVelocity % normal);
					relVeloc = playerNormalVelocity * massWeigh - projVeloc;
					if (relVeloc < 0.0f) {
						momentumDamper = 0.1f;
						// apply impulse resistance to body
						velocCorrection = 1;

						//horizontalVelocity -= normal.Scale(playerNormalVelocity * (1.0f - massWeigh));
						horizontalVelocity -= normal.Scale(relVeloc * (1.0f - momentumDamper) + playerNormalVelocity * (1.0f - massWeigh));

						// apply impulse to hit body
						dVector veloc (normal.Scale(relVeloc * momentumDamper));
						NewtonBodyAddImpulse (body, &veloc[0], &com[0]);
					}
				}
			}

			// now restore hit body state and apply a force to archive the hi impulse
			for (int i = 0; i < bodyCounts; i ++) {
				dFloat mass;
				dFloat Ixx;
				dFloat Iyy;
				dFloat Izz;
				const NewtonBody* body;

				body = hitBodies[i];
				NewtonBodyGetMassMatrix(body, &mass, &Ixx, &Iyy, &Izz);
				if (!((mass < 1.0e-3f) || !CanPushBody (body))) {
					dVector veloc;
					dVector omega;
					dVector forceAcc;
					dVector torqueAcc;
					dMatrix matrix;

					NewtonBodyGetOmega (body, &omega[0]);
					NewtonBodyGetVelocity (body, &veloc[0]);
					NewtonBodyGetOmega (body, &hitOmega[i][0]);
					NewtonBodyGetVelocity (body, &hitVeloc[i][0]);

					// calculate the force and torque to archive desired push
					NewtonBodyGetForceAcc(body, &forceAcc[0]);
					dVector force ((veloc - hitVeloc[i]).Scale (mass * timestepInv) - forceAcc);
					NewtonBodyAddForce(body, &force[0]);


					NewtonBodyGetMatrix (body, &matrix[0][0]);
					dVector deltaOmega (matrix.UnrotateVector (omega - hitOmega[i]).Scale (timestepInv));
					deltaOmega.m_x *= Ixx;
					deltaOmega.m_y *= Iyy;
					deltaOmega.m_z *= Izz;
					NewtonBodyGetTorqueAcc(body, &torqueAcc[0]);
					dVector torque (matrix.RotateVector(deltaOmega) - torqueAcc);
					NewtonBodyAddForce(body, &torque[0]);
				}
			}

			contacts = 0;
			if ((time > 1.0e-2f * timestep) && velocCorrection) {
				dVector destination (startCasting.m_posit + horizontalVelocity.Scale (timestep));
				//contacts = NewtonWorldConvexCast (m_world, &startCasting[0][0], &destination[0], m_horizontalSensorShape, &staticFilterData, ConvexDynamicCastPrefilter, info, sizeof (info) / sizeof (info[0]));
				dFloat param;
				contacts = NewtonWorldConvexCast (m_world, &startCasting[0][0], &destination[0], m_dynamicsCollisionShape, &param, &staticFilterData, ConvexDynamicCastPrefilter, info, sizeof (info) / sizeof (info[0]), threadIndex);
				
			}
		}
		_ASSERTE (iterations < MAX_COLLISIONS_ITERATION);


		startCasting.m_posit += bodyMatrix.m_up.Scale (m_stairHeight * 0.5f);
		destination = startCasting.m_posit + horizontalVelocity.Scale (timestep);
		contacts = NewtonWorldConvexCast (m_world, &startCasting[0][0], &destination[0], m_horizontalSensorShape, &param, &staticFilterData, ConvexStaticCastPrefilter, info, sizeof (info) / sizeof (info[0]), threadIndex);
		for (iterations = 0; contacts && (iterations < MAX_COLLISIONS_ITERATION); iterations ++) {
			int velocCorrection;
			dFloat time;

			// if the player will hit an obstacle in this direction then we need to change the velocity direction
			// we will declare a collision at the player origin.
			//dVector posit (startCasting.m_posit + (destination - startCasting.m_posit).Scale(info[0].m_intersectionParam));
			dVector posit (startCasting.m_posit + (destination - startCasting.m_posit).Scale(param));

			// calculate the travel time, and subtract from time remaining
			time = ((posit - startCasting.m_posit) % horizontalVelocity) / (horizontalVelocity % horizontalVelocity) * 0.9f;

			velocCorrection = 0;
			for (int i = 0; i < contacts; i ++) {
				dFloat mass;
				dFloat Ixx;
				dFloat Iyy;
				dFloat Izz;
				dFloat reboundVeloc;
				dFloat penetrationVeloc;
				const NewtonBody* body;

				// flatten contact normal
				dVector normal (info[i].m_normal);
				normal.m_y = 0.0f;
				normal = normal.Scale (1.0f / dSqrt (normal % normal));

				body = info[i].m_hitBody;
				NewtonBodyGetMassMatrix(body, &mass, &Ixx, &Iyy, &Izz);

				// calculate the reflexion velocity
				penetrationVeloc = -0.5f * timestepInv * ((info[i].m_penetration > 0.1f) ? 0.1f : info[i].m_penetration);

				reboundVeloc = (horizontalVelocity % normal) * (1.0f + m_restitution) + penetrationVeloc;
				if (reboundVeloc < 0.0f) {
					velocCorrection = 1;
					horizontalVelocity -= normal.Scale(reboundVeloc);
				}
			}

			contacts = 0;
			if ((time > 1.0e-2f * timestep) && velocCorrection) {
				dVector destination (startCasting.m_posit + horizontalVelocity.Scale (timestep));
				dFloat param;
				contacts = NewtonWorldConvexCast (m_world, &startCasting[0][0], &destination[0], m_horizontalSensorShape, &param, &staticFilterData, ConvexStaticCastPrefilter, info, sizeof (info) / sizeof (info[0]), threadIndex);
			}
		}
		_ASSERTE (iterations < MAX_COLLISIONS_ITERATION);


		// now determine the ground tracking, predict the destination position;
		if (m_isInJumpState) {
			// player of in the air look ahead for the land
			dMatrix destMatrix (bodyMatrix);
			destMatrix.m_posit += (horizontalVelocity.Scale (timestep) - bodyMatrix.m_up.Scale (m_stairHeight * 0.5f));
			dVector target (destMatrix.m_posit - bodyMatrix.m_up.Scale(m_stairHeight));

			FilterData staticFilterData (m_body0);
			NewtonWorldConvexCastReturnInfo info;
			dFloat param;
			if (NewtonWorldConvexCast (m_world, &destMatrix[0][0], &target[0], m_verticalSensorShape, &param, &staticFilterData, ConvexStaticCastPrefilter, &info, 1, threadIndex)) {

				dFloat dist;
				dFloat correctionVelocity;
				// player is about to land, snap position to the ground
				//dist = - m_stairHeight * info.m_intersectionParam;
				dist = - m_stairHeight * param;

				correctionVelocity = dist * timestepInv - veloc.m_y; 
				NewtonUserJointAddLinearRow (m_joint, &bodyMatrix.m_posit[0], &bodyMatrix.m_posit[0], &bodyMatrix.m_up.m_x);
				NewtonUserJointSetRowAcceleration (m_joint, correctionVelocity * timestepInv);
				m_isInJumpState = 0;
			}
		} else {
			// player is moving on the ground, look ahead for stairs steps 
			dMatrix destMatrix (bodyMatrix);
			destMatrix.m_posit += (horizontalVelocity.Scale (timestep) + bodyMatrix.m_up.Scale (m_stairHeight * 0.5f));
			dVector target (destMatrix.m_posit - bodyMatrix.m_up.Scale(m_stairHeight * 2.0f));

			FilterData staticFilterData (m_body0);
			NewtonWorldConvexCastReturnInfo info;
//			if (NewtonWorldConvexCast (m_world, &destMatrix[0][0], &target[0], m_verticalSensorShape, &staticFilterData, ConvexStaticCastPrefilter, &info, 1)) {
			dFloat param;
			if (NewtonWorldConvexCast (m_world, &destMatrix[0][0], &target[0], m_verticalSensorShape, &param, &staticFilterData, ConvexAllBodyCastPrefilter, &info, 1, threadIndex)) {
				//if (info.m_intersectionParam > 0.01f) {
				if (param > 0.01f) {
				
					// check if the contact violates the maxSlope constraint 

					if (info.m_normal[1] < m_maxSlope) {
						// we are hitting stiff slope
						int attrb;
						dFloat param;
						dMatrix matrix;
						dVector	shapeLocalNormal;
						NewtonCollision* collision;
						
						// cast a ray at the hit point and verify this is a face normal and not and edge of vertex normal
						// that can be pointing in the wrong direction
						NewtonBodyGetMatrix(info.m_hitBody, &matrix[0][0]);

						dVector	localNormal (matrix.UnrotateVector(info.m_normal));
						dVector	origin (matrix.UntransformVector(info.m_point));
						
						dVector p0 (origin + localNormal.Scale (0.1f));
						dVector p1 (origin - localNormal.Scale (0.1f));

						collision = NewtonBodyGetCollision (info.m_hitBody);
						param = NewtonCollisionRayCast(collision, &p0[0], &p1[0], &shapeLocalNormal[0], &attrb);
						_ASSERTE (param < 1.0f);
						if (param < 1.0f) {
							if ((shapeLocalNormal % localNormal) > 0.9f) {
								horizontalVelocity = dVector (0.0f, 0.0f, 0.0f);
								//info.m_intersectionParam = 0.5f;
								param = 0.5f;
							}
						}
					}


					dFloat dist;
					dFloat correctionVelocity;
					// the player will hit surface, make sure is is grounded
					//dist = m_stairHeight * (1.0f - 2.0f * info.m_intersectionParam);
					dist = m_stairHeight * (1.0f - 2.0f * param);
					
					correctionVelocity = dist * timestepInv - veloc.m_y; 
					NewtonUserJointAddLinearRow (m_joint, &bodyMatrix.m_posit[0], &bodyMatrix.m_posit[0], &bodyMatrix.m_up.m_x);
					NewtonUserJointSetRowAcceleration (m_joint, correctionVelocity * timestepInv);

					m_isInJumpState = 0;
				} else {
					// something when wrong because the vertical sensor shape is colliding 
					// at origin of the predicted destination
					// this should never happens as a precaution set the vertical body velocity to zero
					NewtonBodyGetVelocity(m_body0, &veloc[0]);
					veloc.m_y = 0.0f;
					NewtonBodySetVelocity(m_body0, &veloc[0]);
				}
			} else {
				m_isInJumpState = 1;
			}
		}


		// finally calculate the necessary force to enforce the desired velocity
		desiredVeloc.m_x = horizontalVelocity.m_x;
		desiredVeloc.m_z = horizontalVelocity.m_z;


		dVector force;
		NewtonBodyCalculateInverseDynamicsForce(m_body0, timestep, &desiredVeloc[0], &force[0]);
		NewtonBodyGetForceAcc(m_body0, &extForce[0]);
		extForce = extForce - bodyMatrix.m_up.Scale ((extForce % bodyMatrix.m_up));
		force -= extForce;
		NewtonBodyAddForce(m_body0, &force[0]);
	}

}

