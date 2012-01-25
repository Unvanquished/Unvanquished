//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************


// CustomPickBody.cpp: implementation of the CustomPickBody class.
//
//////////////////////////////////////////////////////////////////////
#include "CustomJointLibraryStdAfx.h"
#include "CustomPickBody.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CustomPickBody::CustomPickBody(const NewtonBody* body, const dVector& handleInGlobalSpace)
	:NewtonCustomJoint(6, body, NULL)
{
	dMatrix matrix;

	// get the initial position and orientation
	NewtonBodyGetMatrix (body, &matrix[0][0]);

	m_autoSlepState = NewtonBodyGetSleepState (body);
	NewtonBodySetAutoSleep (body, 0);

	m_localHandle = matrix.UntransformVector (handleInGlobalSpace);
	matrix.m_posit = handleInGlobalSpace;

	SetPickMode (1);
	SetTargetMatrix (matrix);
	SetMaxLinearFriction(1.0f); 
	SetMaxAngularFriction(1.0f); 
}

CustomPickBody::~CustomPickBody()
{
	NewtonBodySetAutoSleep (m_body0, m_autoSlepState);
}


void CustomPickBody::SetPickMode (int mode)
{
	m_pickMode = mode ? 1 : 0;
}

void CustomPickBody::SetMaxLinearFriction(dFloat accel)
{
	dFloat Ixx;
	dFloat Iyy;
	dFloat Izz;
	dFloat mass;

	NewtonBodyGetMassMatrix (m_body0, &mass, &Ixx, &Iyy, &Izz);
	m_maxLinearFriction = dAbs (accel) * mass;
}

void CustomPickBody::SetMaxAngularFriction(dFloat alpha)
{
	dFloat Ixx;
	dFloat Iyy;
	dFloat Izz;
	dFloat mass;

	NewtonBodyGetMassMatrix (m_body0, &mass, &Ixx, &Iyy, &Izz);
	if (Iyy > Ixx) {
		Ixx = Iyy;
	}
	if (Izz > Ixx) {
		Ixx = Izz;
	}
	m_maxAngularFriction = dAbs (alpha) * Ixx;
}


void CustomPickBody::SetTargetRotation(const dQuaternion& rotation)
{
	m_targetRot = rotation;
}

void CustomPickBody::SetTargetPosit(const dVector& posit)
{
	m_targetPosit = posit;
}

void CustomPickBody::SetTargetMatrix(const dMatrix& matrix)
{
	SetTargetRotation (matrix);
	SetTargetPosit (matrix.m_posit);
}

dMatrix CustomPickBody::GetTargetMatrix () const
{
	return dMatrix (m_targetRot, m_targetPosit);
}


void CustomPickBody::GetInfo (NewtonJointRecord* info) const
{
	strcpy (info->m_descriptionType, "pickBody");
/*
	info->m_attachBody_0 = m_body0;
	info->m_attachBody_1 = m_body1;

	info->m_minLinearDof[0] = -FLT_MAX;
	info->m_maxLinearDof[0] = FLT_MAX;

	info->m_minLinearDof[1] = -FLT_MAX;
	info->m_maxLinearDof[1] = FLT_MAX;

	info->m_minLinearDof[2] = -FLT_MAX;
	info->m_maxLinearDof[2] = FLT_MAX;

	info->m_minAngularDof[0] = -FLT_MAX;
	info->m_maxAngularDof[0] = FLT_MAX;

	info->m_minAngularDof[1] = 0.0f;
	info->m_maxAngularDof[1] = 0.0f;

	info->m_minAngularDof[2] = 0.0f;
	info->m_maxAngularDof[2] = 0.0f;

	info->m_bodiesCollisionOn = 1;

	memcpy (info->m_attachmenMatrix_0, &m_localMatrix0, sizeof (dMatrix));

	// note this is not a bug
	memcpy (info->m_attachmenMatrix_1, &m_localMatrix0, sizeof (dMatrix));
*/
}


void CustomPickBody::SubmitConstrainst (dFloat timestep, int threadIndex)
{
	dVector v;
	dVector w;
	dVector cg;
	dMatrix matrix0;
	dFloat invTimestep;
	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 

	NewtonBodyGetOmega (m_body0, &w[0]);
	NewtonBodyGetVelocity (m_body0, &v[0]);
	NewtonBodyGetCentreOfMass (m_body0, &cg[0]);
	NewtonBodyGetMatrix (m_body0, &matrix0[0][0]);

	invTimestep = 1.0f / timestep;
	dVector p0 (matrix0.TransformVector (m_localHandle));

	dVector pointVeloc = v + w * matrix0.RotateVector (m_localHandle - cg);
	dVector relPosit (m_targetPosit - p0);
	dVector relVeloc (relPosit.Scale (invTimestep) - pointVeloc);
	dVector relAccel (relVeloc.Scale (invTimestep * 0.3f)); 
		
	// Restrict the movement on the pivot point along all tree orthonormal direction
	NewtonUserJointAddLinearRow (m_joint, &p0[0], &m_targetPosit[0], &matrix0.m_front[0]);
	NewtonUserJointSetRowAcceleration (m_joint, relAccel % matrix0.m_front);
	NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxLinearFriction);
	NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxLinearFriction);

	NewtonUserJointAddLinearRow (m_joint, &p0[0], &m_targetPosit[0], &matrix0.m_up[0]);
	NewtonUserJointSetRowAcceleration (m_joint, relAccel % matrix0.m_up);
	NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxLinearFriction);
	NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxLinearFriction);

	NewtonUserJointAddLinearRow (m_joint, &p0[0], &m_targetPosit[0], &matrix0.m_right[0]);
	NewtonUserJointSetRowAcceleration (m_joint, relAccel % matrix0.m_right);
	NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxLinearFriction);
	NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxLinearFriction);

	if (m_pickMode) {
		dFloat mag;
		dQuaternion rotation;

		NewtonBodyGetRotation (m_body0, &rotation.m_q0);
		if (m_targetRot.DotProduct (rotation) < 0.0f) {
			rotation.m_q0 *= -1.0f; 
			rotation.m_q1 *= -1.0f; 
			rotation.m_q2 *= -1.0f; 
			rotation.m_q3 *= -1.0f; 
		}

		dVector relOmega (rotation.CalcAverageOmega (m_targetRot, timestep) - w);
		mag = relOmega % relOmega;
		if (mag > 1.0e-6f) {
			dFloat relAlpha;
			dFloat relSpeed;
			dVector pin (relOmega.Scale (1.0f / mag));
			dMatrix basis (dgGrammSchmidt (pin)); 	
			relSpeed = dSqrt (relOmega % relOmega);
			relAlpha = relSpeed * invTimestep;

			NewtonUserJointAddAngularRow (m_joint, 0.0f, &basis.m_front[0]);
			NewtonUserJointSetRowAcceleration (m_joint, relAlpha);
			NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
			NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

			NewtonUserJointAddAngularRow (m_joint, 0.0f, &basis.m_up[0]);
			NewtonUserJointSetRowAcceleration (m_joint, 0.0f);
			NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
			NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

			NewtonUserJointAddAngularRow (m_joint, 0.0f, &basis.m_right[0]);
			NewtonUserJointSetRowAcceleration (m_joint, 0.0f);
			NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
			NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

		} else {

			dVector relAlpha = w.Scale (-invTimestep);
			NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_front[0]);
			NewtonUserJointSetRowAcceleration (m_joint, relAlpha % matrix0.m_front);
			NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
			NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

			NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_up[0]);
			NewtonUserJointSetRowAcceleration (m_joint, relAlpha % matrix0.m_up);
			NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
			NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

			NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_right[0]);
			NewtonUserJointSetRowAcceleration (m_joint, relAlpha % matrix0.m_right);
			NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
			NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);
		}

	} else {
		// this is the single handle pick mode, add soem angular frition

		dVector relAlpha = w.Scale (-invTimestep);
		NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_front[0]);
		NewtonUserJointSetRowAcceleration (m_joint, relAlpha % matrix0.m_front);
		NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction * 0.025f);
		NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction * 0.025f);

		NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_up[0]);
		NewtonUserJointSetRowAcceleration (m_joint, relAlpha % matrix0.m_up);
		NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction * 0.025f);
		NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction * 0.025f);

		NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_right[0]);
		NewtonUserJointSetRowAcceleration (m_joint, relAlpha % matrix0.m_right);
		NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction * 0.025f);
		NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction * 0.025f);
	}
}


