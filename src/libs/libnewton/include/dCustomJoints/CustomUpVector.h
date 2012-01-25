//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************


// CustomUpVector.h: interface for the CustomUpVector class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CUSTOMUPVECTOR_H__EAE1E36C_6FDF_4D86_B4EE_855E3D1046F4__INCLUDED_)
#define AFX_CUSTOMUPVECTOR_H__EAE1E36C_6FDF_4D86_B4EE_855E3D1046F4__INCLUDED_


#include "NewtonCustomJoint.h"

// This joint is useful to for implementing character controllers, and also precise object picking
class JOINTLIBRARY_API CustomUpVector: public NewtonCustomJoint
{
	public:
	CustomUpVector(const dVector& pin, NewtonBody* child);
	virtual ~CustomUpVector();

	void SetPinDir (const dVector& pin);

	protected:
	virtual void SubmitConstrainst (dFloat timestep, int threadIndex);
	virtual void GetInfo (NewtonJointRecord* info) const;


	dMatrix m_localMatrix0;
	dMatrix m_localMatrix1;
};

#endif // !defined(AFX_CUSTOMUPVECTOR_H__EAE1E36C_6FDF_4D86_B4EE_855E3D1046F4__INCLUDED_)
