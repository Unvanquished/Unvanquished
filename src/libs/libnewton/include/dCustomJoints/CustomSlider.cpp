//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************


// CustomSlider.cpp: implementation of the CustomSlider class.
//
//////////////////////////////////////////////////////////////////////
#include "CustomJointLibraryStdAfx.h"
#include "CustomSlider.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define MIN_JOINT_PIN_LENGTH	50.0f

CustomSlider::CustomSlider (const dMatrix& pinsAndPivoFrame, NewtonBody* child, NewtonBody* parent)
	:NewtonCustomJoint(6, child, parent)
{
	m_limitsOn = false;
	m_hitLimitOnLastUpdate = false;
	m_minDist = -1.0f;
	m_maxDist =  1.0f;

	// calculate the two local matrix of the pivot point
	CalculateLocalMatrix (pinsAndPivoFrame, m_localMatrix0, m_localMatrix1);
}

CustomSlider::~CustomSlider()
{
}


void CustomSlider::EnableLimits(bool state)
{
	m_limitsOn = state;
}

void CustomSlider::SetLimis(dFloat minDist, dFloat maxDist)
{
//	_ASSERTE (minDist < 0.0f);
//	_ASSERTE (maxDist > 0.0f);

	m_minDist = minDist;
	m_maxDist = maxDist;
}

bool CustomSlider::JoinHitLimit () const 
{
	return m_hitLimitOnLastUpdate;
}

void CustomSlider::SubmitConstrainst (dFloat timestep, int threadIndex)
{
	dFloat dist;
	dMatrix matrix0;
	dMatrix matrix1;

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (m_localMatrix0, m_localMatrix1, matrix0, matrix1);

	// Restrict the movement on the pivot point along all tree orthonormal direction
	dVector p0 (matrix0.m_posit);
	dVector p1 (matrix1.m_posit + matrix1.m_front.Scale ((p0 - matrix1.m_posit) % matrix1.m_front));

	NewtonUserJointAddLinearRow (m_joint, &p0[0], &p1[0], &matrix0.m_up[0]);
	NewtonUserJointAddLinearRow (m_joint, &p0[0], &p1[0], &matrix0.m_right[0]);
	
	// get a point along the ping axis at some reasonable large distance from the pivot
	dVector q0 (p0 + matrix0.m_front.Scale(MIN_JOINT_PIN_LENGTH));
	dVector q1 (p1 + matrix1.m_front.Scale(MIN_JOINT_PIN_LENGTH));

	// two constraints row perpendicular to the pin
 	NewtonUserJointAddLinearRow (m_joint, &q0[0], &q1[0], &matrix0.m_up[0]);
	NewtonUserJointAddLinearRow (m_joint, &q0[0], &q1[0], &matrix0.m_right[0]);

	// get a point along the ping axis at some reasonable large distance from the pivot
	dVector r0 (p0 + matrix0.m_up.Scale(MIN_JOINT_PIN_LENGTH));
	dVector r1 (p1 + matrix1.m_up.Scale(MIN_JOINT_PIN_LENGTH));

	// one constraint row perpendicular to the pin
 	NewtonUserJointAddLinearRow (m_joint, &r0[0], &r1[0], &matrix0.m_right[0]);

	// if limit are enable ...
	m_hitLimitOnLastUpdate = false;
	if (m_limitsOn) {
		dist = (matrix0.m_posit - matrix1.m_posit) % matrix0.m_front;
		if (dist < m_minDist) {
			// indicate that this row hit a limit
			m_hitLimitOnLastUpdate = true;

			// get a point along the up vector and set a constraint  
			const dVector& p0 = matrix0.m_posit;
			dVector p1 (p0 + matrix0.m_front.Scale (m_minDist - dist));
			NewtonUserJointAddLinearRow (m_joint, &p0[0], &p1[0], &matrix0.m_front[0]);
			// allow the object to return but not to kick going forward
			NewtonUserJointSetRowMinimumFriction (m_joint, 0.0f);
			
		} else if (dist > m_maxDist) {
			// indicate that this row hit a limit
			m_hitLimitOnLastUpdate = true;

			// get a point along the up vector and set a constraint  

			const dVector& p0 = matrix0.m_posit;
			dVector p1 (p0 + matrix0.m_front.Scale (m_maxDist - dist));
			NewtonUserJointAddLinearRow (m_joint, &p0[0], &p1[0], &matrix0.m_front[0]);
			// allow the object to return but not to kick going forward
			NewtonUserJointSetRowMaximumFriction (m_joint, 0.0f);

		} else {

/*
			// uncommnet thsi for a slider with friction

			// take any point on body0 (origin)
			const dVector& p0 = matrix0.m_posit;

			dVector veloc0; 
			dVector veloc1; 
			dVector omega1; 

			NewtonBodyGetVelocity(m_body0, &veloc0[0]);
			NewtonBodyGetVelocity(m_body1, &veloc1[0]);
			NewtonBodyGetOmega(m_body1, &omega1[0]);

			// this assumes the origin of the bodies the matrix pivot are the same
			veloc1 += omega1 * (matrix1.m_posit - p0);

			dFloat relAccel; 
			relAccel = ((veloc1 - veloc0) % matrix0.m_front) / timestep;

			#define MaxFriction 10.0f
			NewtonUserJointAddLinearRow (m_joint, &p0[0], &p0[0], &matrix0.m_front[0]);
			NewtonUserJointSetRowAcceleration (m_joint, relAccel);
			NewtonUserJointSetRowMinimumFriction (m_joint, -MaxFriction);
			NewtonUserJointSetRowMaximumFriction(m_joint, MaxFriction);
*/
		}
	} 
 }


void CustomSlider::GetInfo (NewtonJointRecord* info) const
{
	strcpy (info->m_descriptionType, "slider");

	info->m_attachBody_0 = m_body0;
	info->m_attachBody_1 = m_body1;

	if (m_limitsOn) {
		dFloat dist;
		dMatrix matrix0;
		dMatrix matrix1;

		// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
		CalculateGlobalMatrix (m_localMatrix0, m_localMatrix1, matrix0, matrix1);
		dist = (matrix0.m_posit - matrix1.m_posit) % matrix0.m_front;

		info->m_minLinearDof[0] = m_minDist - dist;
		info->m_maxLinearDof[0] = m_maxDist - dist;
	} else {
		info->m_minLinearDof[0] = -FLT_MAX ;
		info->m_maxLinearDof[0] =  FLT_MAX ;
	}


	info->m_minLinearDof[1] = 0.0f;
	info->m_maxLinearDof[1] = 0.0f;;

	info->m_minLinearDof[2] = 0.0f;
	info->m_maxLinearDof[2] = 0.0f;

	info->m_minAngularDof[0] = 0.0f;
	info->m_maxAngularDof[0] = 0.0f;

	info->m_minAngularDof[1] = 0.0f;
	info->m_maxAngularDof[1] = 0.0f;

	info->m_minAngularDof[2] = 0.0f;
	info->m_maxAngularDof[2] = 0.0f;

	memcpy (info->m_attachmenMatrix_0, &m_localMatrix0, sizeof (dMatrix));
	memcpy (info->m_attachmenMatrix_1, &m_localMatrix1, sizeof (dMatrix));
}

