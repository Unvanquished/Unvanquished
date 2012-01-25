//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************


// CustomPathFollow.h: interface for the CustomPathFollow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CUSTOM_PATH_FOLLOW_H__INCLUDED_)
#define AFX_CUSTOM_PATH_FOLLOW_H__INCLUDED_

#include "NewtonCustomJoint.h"

class JOINTLIBRARY_API CustomPathFollow: public NewtonCustomJoint  
{
	public:
	CustomPathFollow (const dMatrix& pinsAndPivoFrame, NewtonBody* body);
	virtual ~CustomPathFollow();

	virtual dMatrix EvalueCurve (const dVector& posit);

	protected:
	virtual void SubmitConstrainst (dFloat timestep, int threadIndex);
	virtual void GetInfo (NewtonJointRecord* info) const;

	dMatrix m_localMatrix0;
};

#endif // !defined(AFX_CUSTOM_PATH_FOLLOW_H__INCLUDED_)
