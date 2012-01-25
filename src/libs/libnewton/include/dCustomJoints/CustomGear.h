//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************

// CustomGear.h: interface for the CustomGear class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CustomGear_H__B631F556_B7D7_F85ECF3E9ADE__INCLUDED_)
#define AFX_CustomGear_H__B631F556_B7D7_F85ECF3E9ADE__INCLUDED_

#include "NewtonCustomJoint.h"

// this joint is for used in conjunction with Hinge of other spherical joints
// is is usefully for establishing synchronization between the phase angle other the 
// relative angular velocity of two spinning disk according to the law of gears
// velErro = -(W0 * r0 + W1 *  r1)
// where w0 and W1 are the angular velocity
// r0 and r1 are the radius of the spinning disk
class JOINTLIBRARY_API CustomGear: public NewtonCustomJoint  
{
	public:
	CustomGear(dFloat gearRatio, 
			   const dVector& childPin, const dVector& parentPin, 
			   NewtonBody* parenPin, NewtonBody* parent);
	virtual ~CustomGear();


	protected:
	virtual void SubmitConstrainst (dFloat timestep, int threadIndex);
	virtual void GetInfo (NewtonJointRecord* info) const;

	dMatrix m_localMatrix0;
	dMatrix m_localMatrix1;

	dFloat m_gearRatio;
};

#endif // !defined(AFX_CustomGear_H__B631F556_468B_4331_B7D7_F85ECF3E9ADE__INCLUDED_)
