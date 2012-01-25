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

#if !defined(AFX_DRY_ROLLOING_FRICTION3D1046F4__INCLUDED_)
#define AFX_DRY_ROLLOING_FRICTION3D1046F4__INCLUDED_


#include "NewtonCustomJoint.h"


// this joint is usefully to simulate the rolling friction of a rolling ball over 
// a flat surface.
// normally this is not important for non spherical objects, but for games like 
// poll, pinball, bolling, golf or any other where the movement of balls is the main objective
// the rolling friction is a real big problem.
class JOINTLIBRARY_API CustomDryRollingFriction: public NewtonCustomJoint  
{
	public:
	CustomDryRollingFriction(NewtonBody* child, dFloat radius, dFloat coeficient);
	virtual ~CustomDryRollingFriction();

	protected:
	virtual void SubmitConstrainst (dFloat timestep, int threadIndex);
	virtual void GetInfo (NewtonJointRecord* info) const;

	dFloat m_frictionCoef;
	dFloat m_frictionTorque;
	
};



#endif // !defined(AFX_DRY_ROLLOING_FRICTION3D1046F4__INCLUDED_)
