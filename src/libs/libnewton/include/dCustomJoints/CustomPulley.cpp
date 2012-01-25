//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************


// CustomPulley.cpp: implementation of the CustomPulley class.
//
//////////////////////////////////////////////////////////////////////
#include "CustomJointLibraryStdAfx.h"
#include "CustomPulley.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CustomPulley::CustomPulley(
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
	dMatrix pinAndPivot0 (dgGrammSchmidt (childPin));
//	CalculateLocalMatrix (pivot, childPin, m_localMatrix0, dommyMatrix);
	CalculateLocalMatrix (pinAndPivot0, m_localMatrix0, dommyMatrix);


	// calculate the local matrix for body body1  
//_ASSERTE (0);
	dMatrix pinAndPivot1 (dgGrammSchmidt (parentPin));
//	CalculateLocalMatrix (pivot, parentPin, dommyMatrix, m_localMatrix1);
	CalculateLocalMatrix (pinAndPivot1, dommyMatrix, m_localMatrix1);
}

CustomPulley::~CustomPulley()
{
	
}


void CustomPulley::SubmitConstrainst (dFloat timestep, int threadIndex)
{
	dFloat w0;
	dFloat w1;
	dFloat relAccel;
	dFloat relVeloc;
	dVector veloc0;
	dVector veloc1;
	dMatrix matrix0;
	dMatrix matrix1;
	dFloat jacobian0[6];
	dFloat jacobian1[6];

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (m_localMatrix0, m_localMatrix1, matrix0, matrix1);
	
	// calculate the angular velocity for both bodies
	NewtonBodyGetVelocity(m_body0, &veloc0[0]);
	NewtonBodyGetVelocity(m_body1, &veloc1[0]);

	// get angular velocity relative to the pin vector
	w0 = veloc0 % matrix0.m_front;
	w1 = veloc1 % matrix1.m_front;

	// establish the gear equation.
	relVeloc = w0 + m_gearRatio * w1;

	// calculate the relative angular acceleration by dividing by the time step
	// ideally relative acceleration should be zero, but is practice there will always 
	// be a small difference in velocity that need to be compensated. 
	// using the full acceleration will make the to over show a oscillate 
	// so use only fraction of the acceleration
	relAccel = - 0.3f * relVeloc / timestep;

	// set the linear part of Jacobian 0 to translational pin vector	
	jacobian0[0] = 	matrix0.m_front[0];
	jacobian0[1] = 	matrix0.m_front[1];
	jacobian0[2] = 	matrix0.m_front[2];

	// set the rotational part of Jacobian 0 to zero
	jacobian0[3] = 	0.0f;
	jacobian0[4] = 	0.0f;
	jacobian0[5] = 	0.0f;

	// set the linear part of Jacobian 1 to translational pin vector	
	jacobian1[0] = 	matrix1.m_front[0];
	jacobian1[1] = 	matrix1.m_front[1];
	jacobian1[2] = 	matrix1.m_front[2];

	// set the rotational part of Jacobian 1 to zero
	jacobian1[3] = 	0.0f;
	jacobian1[4] = 	0.0f;
	jacobian1[5] = 	0.0f;

	// add a angular constraint
	NewtonUserJointAddGeneralRow (m_joint, jacobian0, jacobian1);

	// set the desired angular acceleration between the two bodies
	NewtonUserJointSetRowAcceleration (m_joint, relAccel);
}


void CustomPulley::GetInfo (NewtonJointRecord* info) const
{
	strcpy (info->m_descriptionType, "pulley");

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