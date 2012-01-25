//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************


// Custom6DOF.h: interface for the Custom6DOF class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_Custom6DOF_H__B631F556_B7D7_F85ECF3E9ADE__INCLUDED_)
#define AFX_Custom6DOF_H__B631F556_B7D7_F85ECF3E9ADE__INCLUDED_

#include "NewtonCustomJoint.h"

class JOINTLIBRARY_API Custom6DOF: public NewtonCustomJoint  
{
	public:
	Custom6DOF (const dMatrix& pinsAndPivoChildFrame, const dMatrix& pinsAndPivoParentFrame, const NewtonBody* child, const NewtonBody* parent = NULL);
	virtual ~Custom6DOF();

	void SetLinearLimits (const dVector& minLinearLimits, const dVector& maxLinearLimits);
	void SetAngularLimits (const dVector& minAngularLimits, const dVector& maxAngularLimits);
	void GetLinearLimits (dVector& minLinearLimits, dVector& maxLinearLimits);
	void GetAngularLimits (dVector& minAngularLimits, dVector& maxAngularLimits);


	void SetReverserUniversal (int order);

	protected:
	virtual void SubmitConstrainst (dFloat timestep, int threadIndex);
	virtual void GetInfo (NewtonJointRecord* info) const;

	void SubmitConstraints (const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);

	protected:
	dMatrix CalculateBasisAndJointAngle (const dMatrix& matrix0, const dMatrix& matrix1) const;
	dMatrix CalculateHinge_Angles (const dMatrix& matrix0, const dMatrix& matrix1, int x, int y, int z) const;
	dMatrix CalculateUniversal_Angles (const dMatrix& matrix0, const dMatrix& matrix1, int x, int y, int z) const;

	dMatrix m_localMatrix0;
	dMatrix m_localMatrix1;
	dVector m_minLinearLimits;
	dVector m_maxLinearLimits;
	dVector m_minAngularLimits;
	dVector m_maxAngularLimits;
	dVector m_maxMaxLinearErrorRamp;
	dVector m_maxMaxAngularErrorRamp;
	bool m_reverseUniversal;
};

#endif // !defined(AFX_Custom6DOF_H__B631F556_B7D7_F85ECF3E9ADE__INCLUDED_)
