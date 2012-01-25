//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************


// CustomUpVector.cpp: implementation of the CustomUpVector class.
//
//////////////////////////////////////////////////////////////////////
#include "CustomJointLibraryStdAfx.h"
#include "CustomUpVector.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CustomUpVector::CustomUpVector(const dVector& pin, NewtonBody* child)
	:NewtonCustomJoint(2, child, NULL)
{
	dMatrix pivot;

	NewtonBodyGetMatrix(child, &pivot[0][0]);

	dMatrix matrix (dgGrammSchmidt(pin));
	matrix.m_posit = pivot.m_posit;

	CalculateLocalMatrix (matrix, m_localMatrix0, m_localMatrix1);
}

CustomUpVector::~CustomUpVector()
{
}


// bu animating the orientation of the pin vector the application can change the orientation of the picked object
void CustomUpVector::SetPinDir (const dVector& pin)
{
	m_localMatrix1 = dgGrammSchmidt(pin);
}


void CustomUpVector::SubmitConstrainst (dFloat timestep, int threadIndex)
{
	dFloat mag; 
	dFloat angle; 
	dMatrix matrix0;
	dMatrix matrix1;

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (m_localMatrix0, m_localMatrix1, matrix0, matrix1);
  
	// if the body ha rotated by some amount, the there will be a plane of rotation
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
}

void CustomUpVector::GetInfo (NewtonJointRecord* info) const
{
	strcpy (info->m_descriptionType, "upVector");

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
}