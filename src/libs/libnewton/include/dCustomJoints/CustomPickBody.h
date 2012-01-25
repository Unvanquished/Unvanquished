//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************


// CustomPickBody.h: interface for the CustomPickBody class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __CustomPickBody_H__
#define __CustomPickBody_H__


#include "NewtonCustomJoint.h"

class JOINTLIBRARY_API CustomPickBody: public NewtonCustomJoint
{
	public:
	CustomPickBody (const NewtonBody* body, const dVector& handleInGlobalSpace);
	virtual ~CustomPickBody();

	void SetPickMode (int mode);
	void SetMaxLinearFriction(dFloat accel); 
	void SetMaxAngularFriction(dFloat alpha); 
	
	void SetTargetRotation (const dQuaternion& rotation); 
	void SetTargetPosit (const dVector& posit); 
	void SetTargetMatrix (const dMatrix& matrix); 

	dMatrix GetTargetMatrix () const;

	protected:
	virtual void SubmitConstrainst (dFloat timestep, int threadIndex);
	virtual void GetInfo (NewtonJointRecord* info) const;

	int m_pickMode;
	int m_autoSlepState;
	dFloat m_maxLinearFriction;
	dFloat m_maxAngularFriction;
	dVector m_localHandle;
	dVector m_targetPosit;
	dQuaternion m_targetRot;
};

#endif // !defined(AFX_CustomPickBody_H__EAE1E36C_6FDF_4D86_B4EE_855E3D1046F4__INCLUDED_)
