//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************


// CustomGear.cpp: implementation of the CustomGear class.
//
//////////////////////////////////////////////////////////////////////
#include "CustomJointLibraryStdAfx.h"
#include "CustomGear.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CustomGear::CustomGear(
	dFloat gearRatio, 
	const dVector& childPin, 
	const dVector& parentPin, 
	NewtonBody* child, 
	NewtonBody* parent)
	:NewtonCustomJoint(1, child, parent)
{
	m_gearRatio = gearRatio;

	// calculate the two local matrix of the pivot point
//	dVector pivot (0.0f, 0.0f, 0.0f);

	dMatrix dommyMatrix;
	// calculate the local matrix for body body0
	dMatrix pinAndPivot0 (dgGrammSchmidt(childPin));
//	CalculateLocalMatrix (pivot, childPin, m_localMatrix0, dommyMatrix);
	CalculateLocalMatrix (pinAndPivot0, m_localMatrix0, dommyMatrix);

	// calculate the local matrix for body body1  
	dMatrix pinAndPivot1 (dgGrammSchmidt(parentPin));
//	CalculateLocalMatrix (pivot, parentPin, dommyMatrix, m_localMatrix1);
	CalculateLocalMatrix (pinAndPivot1, dommyMatrix, m_localMatrix1);

}

CustomGear::~CustomGear()
{
	
}


void CustomGear::SubmitConstrainst (dFloat timestep, int threadIndex)
{
	dFloat w0;
	dFloat w1;
	dFloat relAccel;
	dFloat relOmega;
	dVector omega0;
	dVector omega1;
	dMatrix matrix0;
	dMatrix matrix1;
	dFloat jacobian0[6];
	dFloat jacobian1[6];

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (m_localMatrix0, m_localMatrix1, matrix0, matrix1);

	
	// calculate the angular velocity for both bodies
	NewtonBodyGetOmega(m_body0, &omega0[0]);
	NewtonBodyGetOmega(m_body1, &omega1[0]);

	// get angular velocity relative to the pin vector
	w0 = omega0 % matrix0.m_front;
	w1 = omega1 % matrix1.m_front;

	// establish the gear equation.
	relOmega = w0 + m_gearRatio * w1;

	// calculate the relative angular acceleration by dividing by the time step
	// ideally relative acceleration should be zero, but is practice there will always 
	// be a small difference in velocity that need to be compensated. 
	// using the full acceleration will make the to over show a oscillate 
	// so use only fraction of the acceleration
	relAccel = - 0.3f * relOmega / timestep;


	// set the linear part of Jacobian 0 to zero	
	jacobian0[0] = 	0.0f;
	jacobian0[1] = 	0.0f;
	jacobian0[2] = 	0.0f;

	// set the angular part of Jacobian 0 pin vector		
	jacobian0[3] = 	matrix0.m_front[0];
	jacobian0[4] = 	matrix0.m_front[1];
	jacobian0[5] = 	matrix0.m_front[2];

	// set the linear part of Jacobian 1 to zero
	jacobian1[0] = 	0.0f;
	jacobian1[1] = 	0.0f;
	jacobian1[2] = 	0.0f;

	// set the angular part of Jacobian 1 pin vector	
	jacobian1[3] = 	matrix1.m_front[0];
	jacobian1[4] = 	matrix1.m_front[1];
	jacobian1[5] = 	matrix1.m_front[2];

	// add a angular constraint
	NewtonUserJointAddGeneralRow (m_joint, jacobian0, jacobian1);

	// set the desired angular acceleration between the two bodies
	NewtonUserJointSetRowAcceleration (m_joint, relAccel);
}


void CustomGear::GetInfo (NewtonJointRecord* info) const
{
	strcpy (info->m_descriptionType, "gear");

	info->m_extraParameters[0] = m_gearRatio;

	info->m_attachBody_0 = m_body0;
	info->m_attachBody_1 = m_body1;

	info->m_minLinearDof[0] = -FLT_MAX;
	info->m_maxLinearDof[0] = FLT_MAX;

	info->m_minLinearDof[1] = -FLT_MAX;
	info->m_maxLinearDof[1] = FLT_MAX;

	info->m_minLinearDof[2] = -FLT_MAX;
	info->m_maxLinearDof[2] = FLT_MAX;

	info->m_minAngularDof[0] = -FLT_MAX;
	info->m_maxAngularDof[0] =  FLT_MAX;

	info->m_minAngularDof[1] = -FLT_MAX;;
	info->m_maxAngularDof[1] =  FLT_MAX;

	info->m_minAngularDof[2] = -FLT_MAX;;
	info->m_maxAngularDof[2] =  FLT_MAX;

	

	memcpy (info->m_attachmenMatrix_0, &m_localMatrix0, sizeof (dMatrix));
	memcpy (info->m_attachmenMatrix_1, &m_localMatrix1, sizeof (dMatrix));

}