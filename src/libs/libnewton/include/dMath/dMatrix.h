//********************************************************************
// Newton Game dynamics 
// copyright 2000
// By Julio Jerez
// VC: 6.0
// simple 4d matrix class
//********************************************************************

#ifndef __dMatrix__
#define __dMatrix__

#include <dVector.h>


class dQuaternion;

// small but fully operational 4x4 matrix class
class dQuaternion;

class dMatrix
{
	public:
	dMatrix ();
	dMatrix (const dVector &front, const dVector &up, const dVector &right, const dVector &posit);
	dMatrix (const dQuaternion &rotation, const dVector &position);

	dVector& operator[] (int i);
	const dVector& operator[] (int i) const;

	dMatrix Inverse () const;
	dMatrix Transpose () const;
	dMatrix Transpose4X4 () const;
	dVector GetXYZ_EulerAngles() const;
	dVector RotateVector (const dVector &v) const;
	dVector UnrotateVector (const dVector &v) const;
	dVector TransformVector (const dVector &v) const;
	dVector UntransformVector (const dVector &v) const;
	dVector TransformPlane (const dVector &localPlane) const;
	dVector UntransformPlane (const dVector &globalPlane) const;
	
	
	void TransformTriplex (void* const dst, int dstStrideInBytes,
						   void* const src, int srcStrideInBytes, int count) const;

	dMatrix operator* (const dMatrix &B) const;

	bool SanityCheck() const;

	dVector m_front;
	dVector m_up;
	dVector m_right;
	dVector m_posit;
};


inline dMatrix GetIdentityMatrix()
{
	return dMatrix (dVector (1.0f, 0.0f, 0.0f, 0.0f),
				    dVector (0.0f, 1.0f, 0.0f, 0.0f),
				    dVector (0.0f, 0.0f, 1.0f, 0.0f),
				    dVector (0.0f, 0.0f, 0.0f, 1.0f));
}

inline dMatrix GetZeroMatrix ()
{
	return dMatrix (dVector (0.0f, 0.0f, 0.0f, 0.0f),
					dVector (0.0f, 0.0f, 0.0f, 0.0f),
					dVector (0.0f, 0.0f, 0.0f, 0.0f),
					dVector (0.0f, 0.0f, 0.0f, 0.0f));
}


inline dMatrix::dMatrix ()
{
}

inline dMatrix::dMatrix (
	const dVector &front, 
	const dVector &up,
	const dVector &right,
	const dVector &posit)
	:m_front (front), m_up(up), m_right(right), m_posit(posit)
{
}

inline dVector& dMatrix::operator[] (int  i)
{
	return (&m_front)[i];
}

inline const dVector& dMatrix::operator[] (int  i) const
{
	return (&m_front)[i];
}

dMatrix dRollMatrix(dFloat ang);
dMatrix dYawMatrix(dFloat ang);
dMatrix dPitchMatrix(dFloat ang);
dMatrix dgGrammSchmidt(const dVector& dir);
#endif
