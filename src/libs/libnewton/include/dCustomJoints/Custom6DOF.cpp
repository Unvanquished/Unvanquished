//********************************************************************
// Newton Game dynamics 
// copyright 2000-2007
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************


// Custom6DOF.cpp: implementation of the Custom6DOF class.
//
//////////////////////////////////////////////////////////////////////
#include "CustomJointLibraryStdAfx.h"
#include "Custom6DOF.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

enum d6DOF_AngularGeometry
{
	m_ball,
	m_hinge_x,
	m_hinge_y,
	m_hinge_z,
	m_universal_xy,
	m_universal_yz,
	m_universal_zx,
	m_fixed,
	m_free,
};


Custom6DOF::Custom6DOF (const dMatrix& pinsAndPivoChildFrame, const dMatrix& pinsAndPivoParentFrame, const NewtonBody* child, const NewtonBody* parent)
	:NewtonCustomJoint(6, child, parent),
	 m_minLinearLimits(0.0f, 0.0f, 0.0f, 0.0f), m_maxLinearLimits(0.0f, 0.0f, 0.0f, 0.0f),
	 m_minAngularLimits(0.0f, 0.0f, 0.0f, 0.0f), m_maxAngularLimits(0.0f, 0.0f, 0.0f, 0.0f)
{
	CalculateLocalMatrix (pinsAndPivoChildFrame, m_localMatrix0, m_localMatrix1);

	m_reverseUniversal = false;
	// set default max constraint violations correction
	m_maxMaxLinearErrorRamp = dVector (0.2f, 0.2f,0.2f, 0.0f);
	m_maxMaxAngularErrorRamp = dVector (1.0f * 3.141592f / 180.0f, 1.0f * 3.141592f / 180.0f, 1.0f * 3.141592f / 180.0f, 0.0f);;
}

Custom6DOF::~Custom6DOF()
{
}


void Custom6DOF::SetLinearLimits (const dVector& minLinearLimits, const dVector& maxLinearLimits)
{
	int i;
	for (i = 0; i < 3; i ++) {
		m_minLinearLimits[i] =  (dAbs (minLinearLimits[i]) < dFloat (1.0e-5f)) ? 0.0f : minLinearLimits[i];
		m_maxLinearLimits[i] =  (dAbs (maxLinearLimits[i]) < dFloat (1.0e-5f)) ? 0.0f : maxLinearLimits[i];
	}

}

void Custom6DOF::SetAngularLimits (const dVector& minAngularLimits, const dVector& maxAngularLimits)
{
	int i;
	for (i = 0; i < 3; i ++) {
		m_minAngularLimits[i] =  (dAbs (minAngularLimits[i]) < dFloat (1.0e-5f)) ? 0.0f : minAngularLimits[i];
		m_maxAngularLimits[i] =  (dAbs (maxAngularLimits[i]) < dFloat (1.0e-5f)) ? 0.0f : maxAngularLimits[i];
	}
}

void Custom6DOF::GetLinearLimits (dVector& minLinearLimits, dVector& maxLinearLimits)
{
	minLinearLimits =  m_minLinearLimits;
	maxLinearLimits =  m_maxLinearLimits;
}

void Custom6DOF::GetAngularLimits (dVector& minAngularLimits, dVector& maxAngularLimits)
{
	minAngularLimits =  m_minAngularLimits;
	maxAngularLimits =  m_maxAngularLimits;
}


void Custom6DOF::SetReverserUniversal (int order)
{
	m_reverseUniversal = order ? true : false;
}

void Custom6DOF::GetInfo (NewtonJointRecord* info) const
{
	int i;
	dFloat dist;
	dMatrix matrix0;
	dMatrix matrix1;

	strcpy (info->m_descriptionType, "generic6dof");

	info->m_attachBody_0 = m_body0;
	info->m_attachBody_1 = m_body1;

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (m_localMatrix0, m_localMatrix1, matrix0, matrix1);

	dVector p0 (matrix0.m_posit);
	dVector p1 (matrix1.m_posit);
	dVector dp (p0 - p1);
	for (i = 0; i < 3; i ++) {
		if (!((m_minLinearLimits[i] == 0.0f) && (m_maxLinearLimits[i] == 0.0f))) {
			p1 += matrix1[i].Scale (dp % matrix1[i]);
		}
	}

	for (i = 0; i < 3; i ++) {
		if ((m_minLinearLimits[i] == 0.0f) && (m_maxLinearLimits[i] == 0.0f)) {
			info->m_minLinearDof[i] = 0.0f;
			info->m_maxLinearDof[i] = 0.0f;
		} else {
			dist = dp % matrix1[i];
			info->m_maxLinearDof[i] = m_maxLinearLimits[i] - dist;
			info->m_minLinearDof[i] = m_minLinearLimits[i] - dist;
		}
	}

	dMatrix eulerAngles (CalculateBasisAndJointAngle (matrix0, matrix1));
	for (i = 0; i < 3; i ++) {
		if ((m_minAngularLimits[i] == 0.0f) && (m_maxAngularLimits[i] == 0.0f)) {
			info->m_minAngularDof[i] = 0.0f;
			info->m_maxAngularDof[i] = 0.0f;
		} else {
			info->m_maxAngularDof[i] = (m_maxAngularLimits[i] - eulerAngles.m_posit[i]) * 180.0f / 3.141592f;
			info->m_minAngularDof[i] = (m_minAngularLimits[i] - eulerAngles.m_posit[i]) * 180.0f / 3.141592f;
		}
	}

	info->m_bodiesCollisionOn = GetBodiesCollisionState();

	memcpy (info->m_attachmenMatrix_0, &m_localMatrix0, sizeof (dMatrix));
	memcpy (info->m_attachmenMatrix_1, &m_localMatrix1, sizeof (dMatrix));
}


void Custom6DOF::SubmitConstrainst (dFloat timestep, int threadIndex)
{
	dMatrix matrix0;
	dMatrix matrix1;

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (m_localMatrix0, m_localMatrix1, matrix0, matrix1);
	SubmitConstraints (matrix0, matrix1, timestep);
}

void Custom6DOF::SubmitConstraints (const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep)
{
	int i;
	dFloat dist;

	// add the linear limits
	const dVector& p0 = matrix0.m_posit;
	dVector p1 (matrix1.m_posit);
	dVector dp (p0 - p1);
	for (i = 0; i < 3; i ++) {
		if (!((m_minLinearLimits[i] == 0.0f) && (m_maxLinearLimits[i] == 0.0f))) {
			p1 += matrix1[i].Scale (dp % matrix1[i]);
		}
	}

	for (i = 0; i < 3; i ++) {
		if ((m_minLinearLimits[i] == 0.0f) && (m_maxLinearLimits[i] == 0.0f)) {
			NewtonUserJointAddLinearRow (m_joint, &p0[0], &p1[0], &matrix0[i][0]);
			NewtonUserJointSetRowStiffness (m_joint, 1.0f);
		} else {
			// it is a limited linear dof, check if it pass the limits
			dist = dp % matrix1[i];
			if (dist > m_maxLinearLimits[i]) {
				// clamp the error, so the not too much energy is added when constraint violation occurs
				if (dist > m_maxMaxLinearErrorRamp[i]) {
					dist = m_maxMaxLinearErrorRamp[i];
				}

				dVector q1 (p1 + matrix1[i].Scale (m_maxLinearLimits[i] - dist));
				NewtonUserJointAddLinearRow (m_joint, &p0[0], &q1[0], &matrix0[i][0]);

				NewtonUserJointSetRowStiffness (m_joint, 1.0f);
				// allow the object to return but not to kick going forward
				NewtonUserJointSetRowMaximumFriction (m_joint, 0.0f);

			} else if (dist < m_minLinearLimits[i]) {
				// clamp the error, so the not too much energy is added when constraint violation occurs
				if (dist < -m_maxMaxLinearErrorRamp[i]) {
					dist = -m_maxMaxLinearErrorRamp[i];
				}

				dVector q1 (p1 + matrix1[i].Scale (m_minLinearLimits[i] - dist));
				NewtonUserJointAddLinearRow (m_joint, &p0[0], &q1[0], &matrix0[i][0]);

				NewtonUserJointSetRowStiffness (m_joint, 1.0f);
				// allow the object to return but not to kick going forward
				NewtonUserJointSetRowMinimumFriction (m_joint, 0.0f);
			}
		}
	}

	dMatrix basisAndEulerAngles (CalculateBasisAndJointAngle (matrix0, matrix1));
	const dVector& eulerAngles = basisAndEulerAngles.m_posit;
	for (i = 0; i < 3; i ++) {
		if ((m_minAngularLimits[i] == 0.0f) && (m_maxAngularLimits[i] == 0.0f)) {
			NewtonUserJointAddAngularRow (m_joint, eulerAngles[i], &basisAndEulerAngles[i][0]);
			NewtonUserJointSetRowStiffness (m_joint, 1.0f);
		} else {
			// it is a limited linear dof, check if it pass the limits
			if (eulerAngles[i] > m_maxAngularLimits[i]) {
				dist = eulerAngles[i] - m_maxAngularLimits[i];
				// clamp the error, so the not too much energy is added when constraint violation occurs
				if (dist > m_maxMaxAngularErrorRamp[i]) {
					dist = m_maxMaxAngularErrorRamp[i];
				}

				// tell joint error will minimize the exceeded angle error
				NewtonUserJointAddAngularRow (m_joint, dist, &basisAndEulerAngles[i][0]);

				// need high stiffness here
				NewtonUserJointSetRowStiffness (m_joint, 1.0f);

				// allow the joint to move back freely
				NewtonUserJointSetRowMinimumFriction (m_joint, 0.0f);

			} else if (eulerAngles[i] < m_minAngularLimits[i]) {
				dist = eulerAngles[i] - m_minAngularLimits[i];
				// clamp the error, so the not too much energy is added when constraint violation occurs
				if (dist < -m_maxMaxAngularErrorRamp[i]) {
					dist = -m_maxMaxAngularErrorRamp[i];
				}

				// tell joint error will minimize the exceeded angle error
				NewtonUserJointAddAngularRow (m_joint, dist, &basisAndEulerAngles[i][0]);

				// need high stiffness here
				NewtonUserJointSetRowStiffness (m_joint, 1.0f);

				// allow the joint to move back freely 
				NewtonUserJointSetRowMaximumFriction (m_joint, 0.0f);
			}
		}
	}
}

dMatrix Custom6DOF::CalculateBasisAndJointAngle (const dMatrix& matrix0, const dMatrix& matrix1) const
{

	d6DOF_AngularGeometry type;

	type = m_fixed;
	if ((m_minAngularLimits[2] == 0.0f) && (m_maxAngularLimits[2] == 0.0f)) {
		if ((m_minAngularLimits[1] == 0.0f) && (m_maxAngularLimits[1] == 0.0f)) {
			if ((m_minAngularLimits[0] == 0.0f) && (m_maxAngularLimits[0] == 0.0f)) {
				// this is a fix joint
				type = m_fixed;
			} else {
				//this is a hinge
				type = m_hinge_x;
			}
		} else {
			if ((m_minAngularLimits[0] == 0.0f) && (m_maxAngularLimits[0] == 0.0f)) {
				type = m_hinge_y;
			} else {
				type = m_universal_xy;
			}
		}

	} else {
		if ((m_minAngularLimits[1] == 0.0f) && (m_maxAngularLimits[1] == 0.0f)) {
			if ((m_minAngularLimits[0] == 0.0f) && (m_maxAngularLimits[0] == 0.0f)) {
				type = m_hinge_z;
			} else {
				type = m_universal_zx;
			}
		} else {
			if ((m_minAngularLimits[0] == 0.0f) && (m_maxAngularLimits[0] == 0.0f)) {
				type = m_universal_yz;
			} else {
				type = m_free;
			}
		}
	}

	switch (type)
	{
		case m_fixed:
		{
			//return CalculateHinge_X_Angles (matrix0, matrix1);
			//_ASSERTE (0);
			return CalculateHinge_Angles (matrix0, matrix1, 0, 1, 2);
			break;
		}

		case m_ball:
		{
			_ASSERTE (0);
			return (GetIdentityMatrix());
			break;
		}

		case m_hinge_x:
		{
			//return CalculateHinge_X_Angles (matrix0, matrix1);
			return CalculateHinge_Angles (matrix0, matrix1, 0, 1, 2);
			break;
		}

		case m_hinge_y:
		{
			//return CalculateHinge_Y_Angles (matrix0, matrix1);
			return CalculateHinge_Angles (matrix0, matrix1, 1, 2, 0);
			break;
		}

		case m_hinge_z:
		{
			//return CalculateHinge_Z_Angles (matrix0, matrix1);
			return CalculateHinge_Angles (matrix0, matrix1, 2, 0, 1);
			break;
		}


		case m_universal_xy:
		{
			return CalculateUniversal_Angles (matrix0, matrix1, 0, 1, 2);
			break;
		}

		case m_universal_yz:
		{
			//return CalculateUniversal_Angles (matrix0, matrix1, 1, 2, 0);
			return CalculateUniversal_Angles (matrix0, matrix1, 2, 1, 0);
			break;
		}

		case m_universal_zx:
		{
			//return CalculateUniversal_Angles (matrix0, matrix1, 2, 0, 1);
			return CalculateUniversal_Angles (matrix0, matrix1, 0, 2, 1);
			break;
		}


		case m_free:
		{
			//return CalculateFree_XYZ_Angles (matrix0, matrix1);
//			_ASSERTE (0);
//			return (GetIdentityMatrix());
			return CalculateUniversal_Angles (matrix0, matrix1, 0, 1, 2);
		}

	}

	return (GetIdentityMatrix());
}



dMatrix Custom6DOF::CalculateHinge_Angles (const dMatrix& matrix0, const dMatrix& matrix1, int x, int y, int z) const
{
	dFloat sinAngle;
	dFloat cosAngle;
	dVector angles;

	// get a point along the pin axis at some reasonable large distance from the pivot
	//	dVector q0 (matrix0.m_posit + matrix0.m_front.Scale(MIN_JOINT_PIN_LENGTH));
	//	dVector q1 (matrix1.m_posit + matrix1.m_front.Scale(MIN_JOINT_PIN_LENGTH));
	// two constraints row perpendicular to the pin vector
	//	NewtonUserJointAddLinearRow (m_joint, &q0[0], &q1[0], &matrix0.m_up[0]);
	//	NewtonUserJointAddLinearRow (m_joint, &q0[0], &q1[0], &matrix0.m_right[0]);


	// assume that angles relative to y and z and error angles and therefore small,
	// we only concern wit the joint angle relative to the x axis in parent space.
	// this is possible because for a one DoF joint the error angle are small, and if tow angel are small
	// the the order of multiplication is irrelevant.
	// this code will not work of more than one dof joint or in the rigid angel error is too large.
	cosAngle = matrix0[x] % matrix1[x];
	dVector sinDir (matrix0[x] * matrix1[x]);

	sinAngle = sinDir % matrix1[y];
	angles[y] = dAtan2 (sinAngle, cosAngle);

	sinAngle = sinDir % matrix1[z];
	angles[z] = dAtan2 (sinAngle, cosAngle);

	cosAngle = matrix0[y] % matrix1[y];
	sinAngle = (matrix0[y] * matrix1[y]) % matrix1[x];
	angles[x] = dAtan2 (sinAngle, cosAngle);

	angles[3] = 1.0f;

//	_ASSERTE (dAbs (angles[y]) < 0.3f);
//	_ASSERTE (dAbs (angles[z]) < 0.3f);

	return dMatrix (matrix1[0], matrix1[1], matrix1[2], angles);
}

dMatrix Custom6DOF::CalculateUniversal_Angles (const dMatrix& matrix0, const dMatrix& matrix1, int x, int y, int z) const
{
	dFloat sinAngle;
	dFloat cosAngle;

	if (m_reverseUniversal) {
		int tmp;
		tmp = x;
		x = y;
		y = tmp;
	}

	// this assumes calculation assumes that the angle relative to the z axis is alway small
	// because it is the unconstitutionally strong angle
	dMatrix basisAndEulerAngles;
	basisAndEulerAngles[x] = matrix0[x];
	basisAndEulerAngles[y] = matrix1[y];
	basisAndEulerAngles[z] = basisAndEulerAngles[x] * basisAndEulerAngles[y];
	basisAndEulerAngles[z] = basisAndEulerAngles[z].Scale (1.0f / dSqrt (basisAndEulerAngles[z] % basisAndEulerAngles[z]));

	cosAngle = matrix0[y] % matrix1[y];
	sinAngle = (matrix0[y] * matrix1[y]) % basisAndEulerAngles[x];
	basisAndEulerAngles[3][x] = dAtan2 (sinAngle, cosAngle);

	cosAngle = matrix0[x] % matrix1[x];
	sinAngle = (matrix0[x] * matrix1[x]) % matrix1[y];
	basisAndEulerAngles[3][y] = dAtan2 (sinAngle, cosAngle);

	dVector dir3 (basisAndEulerAngles[z] * basisAndEulerAngles[x]);
	dir3 = dir3.Scale (1.0f / dSqrt (dir3 % dir3));
	cosAngle = dir3 % basisAndEulerAngles[y];
	sinAngle = (dir3 * basisAndEulerAngles[y]) % basisAndEulerAngles[z];
	basisAndEulerAngles[3][z] = dAtan2 (sinAngle, cosAngle);
	//	_ASSERTE (dAbs (angle_z) < 0.1f);
	basisAndEulerAngles[3][3] = 1.0f;

//	_ASSERTE (dAbs (basisAndEulerAngles[3][z]) < 0.3f);
	return basisAndEulerAngles;
}







