//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************


// CustomSlidingContact.h: interface for the CustomSlidingContact class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CustomSlidingContact_INCLUDED_)
#define AFX_CustomSlidingContact_INCLUDED_

#include "NewtonCustomJoint.h"

class JOINTLIBRARY_API CustomSlidingContact: public NewtonCustomJoint  
{
	public:
	CustomSlidingContact (const dMatrix& pinsAndPivoFrame, const NewtonBody* child, const NewtonBody* parent = NULL);
	virtual ~CustomSlidingContact();

	void EnableLinearLimits(bool state);
	void EnableAngularLimits(bool state);
	void SetLinearLimis(dFloat minAngle, dFloat maxAngle);
	void SetAngularLimis(dFloat minAngle, dFloat maxAngle);

protected:
	virtual void SubmitConstrainst (dFloat timestep, int threadIndex);
	virtual void GetInfo (NewtonJointRecord* info) const;

	dMatrix m_localMatrix0;
	dMatrix m_localMatrix1;

	bool m_limitsLinearOn;
	bool m_limitsAngularOn;
	dFloat m_minLinearDist;
	dFloat m_maxLinearDist;
	dFloat m_minAngularDist;
	dFloat m_maxAngularDist;

};

#endif // !defined(AFX_CustomSlidingContact_H__B631F556_468B_4331_B7D7_F85ECF3E9ADE__INCLUDED_)
