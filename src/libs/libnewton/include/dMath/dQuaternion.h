//********************************************************************
// Newton Game dynamics 
// copyright 2000
// By Julio Jerez
// VC: 6.0
// simple 4d matrix class
//********************************************************************


#ifndef __dQuaternion__
#define __dQuaternion__

#include <dVector.h>
class dMatrix;

class dQuaternion
{
	public:
	dQuaternion (); 
	dQuaternion (const dMatrix &matrix);
	dQuaternion (dFloat q0, dFloat q1, dFloat q2, dFloat q3); 
	dQuaternion (const dVector &unit_Axis, dFloat Angle = 0.0f);
	
	void Scale (dFloat scale); 
	void Normalize (); 
	inline dFloat DotProduct (const dQuaternion &QB) const;
	dQuaternion Inverse () const; 

	dVector RotateVector (const dVector& point) const;
	dVector UnrotateVector (const dVector& point) const;

	dVector GetXYZ_EulerAngles() const;
	dQuaternion Slerp (const dQuaternion &q1, dFloat t) const;
	dVector CalcAverageOmega (const dQuaternion &q1, dFloat dt) const;

	dQuaternion operator* (const dQuaternion &B) const;
	dQuaternion operator+ (const dQuaternion &B) const; 
	dQuaternion operator- (const dQuaternion &B) const; 

	dFloat m_q0;
	dFloat m_q1;
	dFloat m_q2;
	dFloat m_q3;
};




inline dQuaternion::dQuaternion () 
{
	m_q0 = 1.0f;
	m_q1 = 0.0f;
	m_q2 = 0.0f;
	m_q3 = 0.0f;
}

inline dQuaternion::dQuaternion (dFloat Q0, dFloat Q1, dFloat Q2, dFloat Q3) 
{
	m_q0 = Q0;
	m_q1 = Q1;
	m_q2 = Q2;
	m_q3 = Q3;
//	_ASSERTE (dAbs (DotProduct (*this) - 1.0f) < 1.0e-4f);
}



inline void dQuaternion::Scale (dFloat scale) 
{
	m_q0 *= scale;
	m_q1 *= scale;
	m_q2 *= scale;
	m_q3 *= scale;
}

inline void dQuaternion::Normalize () 
{
	Scale (1.0f / dSqrt (DotProduct (*this)));
}

inline dFloat dQuaternion::DotProduct (const dQuaternion &QB) const
{
	return m_q0 * QB.m_q0 + m_q1 * QB.m_q1 + m_q2 * QB.m_q2 + m_q3 * QB.m_q3;
}

inline dQuaternion dQuaternion::Inverse () const 
{
	return dQuaternion (m_q0, -m_q1, -m_q2, -m_q3);
}

/*
inline dQuaternion operator+ (const dQuaternion &A, const dQuaternion &B) 
{
	return dQuaternion (A.m_q0 + B.m_q0, A.m_q1 + B.m_q1, A.m_q2 + B.m_q2, A.m_q3 + B.m_q3);
}

inline dQuaternion operator- (const dQuaternion &A, const dQuaternion &B) 
{
	return dQuaternion (A.m_q0 - B.m_q0, A.m_q1 - B.m_q1, A.m_q2 - B.m_q2, A.m_q3 - B.m_q3);
}
*/

inline dQuaternion dQuaternion::operator+ (const dQuaternion &B) const
{
	return dQuaternion (m_q0 + B.m_q0, m_q1 + B.m_q1, m_q2 + B.m_q2, m_q3 + B.m_q3);
}

inline dQuaternion dQuaternion::operator- (const dQuaternion &B) const
{
	return dQuaternion (m_q0 - B.m_q0, m_q1 - B.m_q1, m_q2 - B.m_q2, m_q3 - B.m_q3);
}


inline dQuaternion dQuaternion::operator* (const dQuaternion &B) const
{
	return dQuaternion (B.m_q0 * m_q0 - B.m_q1 * m_q1 - B.m_q2 * m_q2 - B.m_q3 * m_q3, 
				 		B.m_q1 * m_q0 + B.m_q0 * m_q1 - B.m_q3 * m_q2 + B.m_q2 * m_q3, 
						B.m_q2 * m_q0 + B.m_q3 * m_q1 + B.m_q0 * m_q2 - B.m_q1 * m_q3, 
						B.m_q3 * m_q0 - B.m_q2 * m_q1 + B.m_q1 * m_q2 + B.m_q0 * m_q3); 
}

inline dVector dQuaternion::GetXYZ_EulerAngles() const
{
	dFloat val;
	dFloat pitch;
	dFloat yaw;
	dFloat roll;

	val = 2.0f * (m_q2 * m_q0 - m_q3 * m_q1);
	if (val  >= 0.99995f) {
		pitch = 2.0f * atan2(m_q1, m_q0);
		yaw = (0.5f * 3.141592f);
		roll = 0.0f;
	} else if (val  <= -0.99995f) {
		pitch = 2.0f * atan2(m_q1, m_q0);
		yaw = -(0.5f * 3.141592f);
		roll = 0.0f;
	} else {
		yaw = dAsin (val);
		pitch = dAtan2 (2.0f * (m_q1 * m_q0 + m_q3 * m_q2), 1.0f - 2.0f * (m_q1 * m_q1 + m_q2 * m_q2));
		roll = dAtan2 (2.0f * (m_q3 * m_q0 + m_q1 * m_q2), 1.0f - 2.0f * (m_q2 * m_q2 + m_q3 * m_q3));
	}
	return dVector (pitch, yaw, roll);
}


#endif 
