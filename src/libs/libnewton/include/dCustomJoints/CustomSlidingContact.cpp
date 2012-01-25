//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************


// CustomSlidingContact.cpp: implementation of the CustomSlidingContact class.
//
//////////////////////////////////////////////////////////////////////
#include "CustomJointLibraryStdAfx.h"
#include "CustomSlidingContact.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define MIN_JOINT_PIN_LENGTH	50.0f

CustomSlidingContact::CustomSlidingContact (const dMatrix& pinsAndPivoFrame, const NewtonBody* child, const NewtonBody* parent)
	:NewtonCustomJoint(6, child, parent)
{
	EnableLinearLimits(false);
	EnableAngularLimits(false);
	SetLinearLimis(-1.0f, 1.0f);
	SetAngularLimis(-30.0f * 3.141592f / 180.0f, 30.0f * 3.141592f / 180.0f);

	// calculate the two local matrix of the pivot point
	CalculateLocalMatrix (pinsAndPivoFrame, m_localMatrix0, m_localMatrix1);
}

CustomSlidingContact::~CustomSlidingContact()
{
}

void CustomSlidingContact::EnableLinearLimits(bool state)
{
	m_limitsLinearOn = state;
}

void CustomSlidingContact::EnableAngularLimits(bool state)
{
	m_limitsAngularOn = state;
}


void CustomSlidingContact::SetLinearLimis(dFloat minDist, dFloat maxDist)
{
	//_ASSERTE (minDist < 0.0f);
	//_ASSERTE (maxDist > 0.0f);

	m_minLinearDist = minDist;
	m_maxLinearDist = maxDist;
}

void CustomSlidingContact::SetAngularLimis(dFloat minDist, dFloat maxDist)
{
	//_ASSERTE (minDist < 0.0f);
	//_ASSERTE (maxDist > 0.0f);

	m_minAngularDist = minDist;
	m_maxAngularDist = maxDist;
}



void CustomSlidingContact::GetInfo (NewtonJointRecord* info) const
{
	strcpy (info->m_descriptionType, "slidingContact");

	info->m_attachBody_0 = m_body0;
	info->m_attachBody_1 = m_body1;


	dMatrix matrix0;
	dMatrix matrix1;
	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (m_localMatrix0, m_localMatrix1, matrix0, matrix1);

	if (m_limitsLinearOn) {
		dFloat dist;
		dist = (matrix0.m_posit - matrix1.m_posit) % matrix0.m_front;
		info->m_minLinearDof[0] = m_minLinearDist - dist;
		info->m_maxLinearDof[0] = m_maxLinearDist - dist;
	} else {
		info->m_minLinearDof[0] = -FLT_MAX ;
		info->m_maxLinearDof[0] = FLT_MAX ;
	}

	info->m_minLinearDof[1] = 0.0f;
	info->m_maxLinearDof[1] = 0.0f;;

	info->m_minLinearDof[2] = 0.0f;
	info->m_maxLinearDof[2] = 0.0f;

	//	info->m_minAngularDof[0] = -FLT_MAX;
	//	info->m_maxAngularDof[0] =  FLT_MAX;
	if (m_limitsAngularOn) {
		dFloat angle;
		dFloat sinAngle;
		dFloat cosAngle;

		sinAngle = (matrix0.m_up * matrix1.m_up) % matrix0.m_front;
		cosAngle = matrix0.m_up % matrix1.m_up;
		angle = dAtan2 (sinAngle, cosAngle);
		info->m_minAngularDof[0] = (m_minAngularDist - angle) * 180.0f / 3.141592f ;
		info->m_maxAngularDof[0] = (m_maxAngularDist - angle) * 180.0f / 3.141592f ;
	} else {
		info->m_minAngularDof[0] = -FLT_MAX ;
		info->m_maxAngularDof[0] =  FLT_MAX;
	}

	info->m_minAngularDof[1] = 0.0f;
	info->m_maxAngularDof[1] = 0.0f;

	info->m_minAngularDof[2] = 0.0f;
	info->m_maxAngularDof[2] = 0.0f;

	memcpy (info->m_attachmenMatrix_0, &m_localMatrix0, sizeof (dMatrix));
	memcpy (info->m_attachmenMatrix_1, &m_localMatrix1, sizeof (dMatrix));
}



void CustomSlidingContact::SubmitConstrainst (dFloat timestep, int threadIndex)
{
//	dFloat dist;
	dMatrix matrix0;
	dMatrix matrix1;

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (m_localMatrix0, m_localMatrix1, matrix0, matrix1);

	// Restrict the movement on the pivot point along all two orthonormal direction
	dVector p0 (matrix0.m_posit);
	dVector p1 (matrix1.m_posit + matrix1.m_front.Scale ((p0 - matrix1.m_posit) % matrix1.m_front));
	NewtonUserJointAddLinearRow (m_joint, &p0[0], &p1[0], &matrix1.m_up[0]);
	NewtonUserJointAddLinearRow (m_joint, &p0[0], &p1[0], &matrix1.m_right[0]);

	// get a point along the pin axis at some reasonable large distance from the pivot
	dVector q0 (p0 + matrix0.m_up.Scale(MIN_JOINT_PIN_LENGTH));
	dVector q1 (p1 + matrix1.m_up.Scale(MIN_JOINT_PIN_LENGTH));
	NewtonUserJointAddLinearRow (m_joint, &q0[0], &q1[0], &matrix1.m_right[0]);
	NewtonUserJointAddLinearRow (m_joint, &q0[0], &q1[0], &matrix1.m_front[0]);


	// if limit are enable ...
	if (m_limitsLinearOn) {
		_ASSERTE (0);
/*
	dist = (matrix0.m_posit - matrix1.m_posit) % matrix0.m_front;
	if (dist < m_minLinearDist) {
	// get a point along the up vector and set a constraint  
	NewtonUserJointAddLinearRow (m_joint, &matrix0.m_posit[0], &matrix0.m_posit[0], &matrix0.m_front[0]);
	// allow the object to return but not to kick going forward
	NewtonUserJointSetRowMinimumFriction (m_joint, 0.0f);


	} else if (dist > m_maxLinearDist) {
	// get a point along the up vector and set a constraint  
	NewtonUserJointAddLinearRow (m_joint, &matrix0.m_posit[0], &matrix0.m_posit[0], &matrix0.m_front[0]);
	// allow the object to return but not to kick going forward
	NewtonUserJointSetRowMaximumFriction (m_joint, 0.0f);
	}
*/
	}

	if (m_limitsAngularOn) {
		_ASSERTE (0);
	}
}
