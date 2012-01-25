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
#include "CustomDryRollingFriction.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CustomDryRollingFriction::CustomDryRollingFriction(NewtonBody* child, dFloat radius, dFloat coeficient)
	:NewtonCustomJoint(1, child, NULL)
{
	dFloat mass;
	dFloat Ixx;
	dFloat Iyy;
	dFloat Izz;

	NewtonBodyGetMassMatrix (child, &mass, &Ixx, &Iyy, &Izz);

	m_frictionCoef = coeficient;
	m_frictionTorque = Ixx * radius;
}


CustomDryRollingFriction::~CustomDryRollingFriction()
{
}


// rolling friction works as follow: the idealization of the contact of a spherical object 
// with a another surface is a point that pass by the center of the sphere.
// in most cases this is enough to model the collision but in insufficient for modeling 
// the rolling friction. In reality contact with the sphere with the other surface is not 
// a point but a contact patch. A contact patch has the property the it generates a fix 
// constant rolling torque that opposes the movement of the sphere.
// we can model this torque by adding a clamped torque aligned to the instantaneously axis 
// of rotation of the ball. and with a magnitude of the stopping angular acceleration.
void CustomDryRollingFriction::SubmitConstrainst (dFloat timestep, int threadIndex)
{
	dVector omega;
	dFloat omegaMag;
	dFloat torqueFriction;

	// get the omega vector
	NewtonBodyGetOmega(m_body0, &omega[0]);

	omegaMag = dSqrt (omega % omega);
	if (omegaMag > 0.1f) {
		// tell newton to used this the friction of the omega vector to apply the rolling friction
		dVector pin (omega.Scale (1.0f / omegaMag));
		NewtonUserJointAddAngularRow (m_joint, 0.0f, &pin[0]);

		// calculate the acceleration to stop the ball in one time step
		NewtonUserJointSetRowAcceleration (m_joint, -omegaMag / timestep);

		// set the friction limit proportional the sphere Inertia
		torqueFriction = m_frictionTorque * m_frictionCoef;
		NewtonUserJointSetRowMinimumFriction (m_joint, -torqueFriction);
		NewtonUserJointSetRowMaximumFriction (m_joint, torqueFriction);

	} else {
		// when omega is too low sheath a little bit and damp the omega directly
		omega = omega.Scale (0.2f);
		NewtonBodySetOmega(m_body0, &omega[0]);
	}
}


void CustomDryRollingFriction::GetInfo (NewtonJointRecord* info) const
{
	strcpy (info->m_descriptionType, "dryRollingFriction");

	info->m_attachBody_0 = m_body0;
	info->m_attachBody_1 = m_body1;

	info->m_minLinearDof[0] = -1.0e10f;
	info->m_maxLinearDof[0] = 1.0e10f;

	info->m_minLinearDof[1] = -1.0e10f;
	info->m_maxLinearDof[1] = 1.0e10f;

	info->m_minLinearDof[2] = -1.0e10f;
	info->m_maxLinearDof[2] = 1.0e10f;

	info->m_minAngularDof[0] = -1.0e10f;
	info->m_maxAngularDof[0] =  1.0e10f;

	info->m_minAngularDof[1] = -1.0e10f;;
	info->m_maxAngularDof[1] =  1.0e10f;

	info->m_minAngularDof[2] = -1.0e10f;;
	info->m_maxAngularDof[2] =  1.0e10f;

	dMatrix matrix (GetIdentityMatrix());
	memcpy (info->m_attachmenMatrix_0, &matrix[0][0], sizeof (dMatrix));

	// note this is not a bug
	memcpy (info->m_attachmenMatrix_1, &matrix[0][0], sizeof (dMatrix));
}

