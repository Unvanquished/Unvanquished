//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************

// CustomHinge.h: interface for the CustomHinge class.
//
//////////////////////////////////////////////////////////////////////


#if !defined(AFX_CUSTOMHINGE_H__B631F556_468B_4331_B7D7_F85ECF3E9ADE__INCLUDED_)
#define AFX_CUSTOMHINGE_H__B631F556_468B_4331_B7D7_F85ECF3E9ADE__INCLUDED_

#include "NewtonCustomJoint.h"

class JOINTLIBRARY_API CustomHinge: public NewtonCustomJoint  
{
	public:
	CustomHinge (const dMatrix& pinsAndPivoFrame, NewtonBody* child, NewtonBody* parent = NULL);
	virtual ~CustomHinge();

	void EnableLimits(bool state);
	void SetLimis(dFloat minAngle, dFloat maxAngle);

	protected:
	virtual void SubmitConstrainst (dFloat timestep, int threadIndex);
	virtual void GetInfo (NewtonJointRecord* info) const;

	dMatrix m_localMatrix0;
	dMatrix m_localMatrix1;

	bool m_limitsOn;
	dFloat m_minAngle;
	dFloat m_maxAngle;
};

#endif // !defined(AFX_CUSTOMHINGE_H__B631F556_468B_4331_B7D7_F85ECF3E9ADE__INCLUDED_)
