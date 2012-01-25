//********************************************************************
// Newton Game dynamics 
// copyright 2000
// By Julio Jerez
// VC: 6.0
// simple 4d matrix class
//********************************************************************
#include "dStdAfxMath.h"
#include "dMathDefines.h"
#include "dVector.h"
#include "dMatrix.h"
#include "dQuaternion.h"


dMatrix::dMatrix (const dQuaternion &rotation, const dVector &position)
{
	dFloat x2;
	dFloat y2;
	dFloat z2;
	dFloat w2;
	dFloat xy;
	dFloat xz;
	dFloat xw;
	dFloat yz;
	dFloat yw;
	dFloat zw;

	w2 = dFloat (2.0f) * rotation.m_q0 * rotation.m_q0;
	x2 = dFloat (2.0f) * rotation.m_q1 * rotation.m_q1;
	y2 = dFloat (2.0f) * rotation.m_q2 * rotation.m_q2;
	z2 = dFloat (2.0f) * rotation.m_q3 * rotation.m_q3;
	_ASSERTE (dAbs (w2 + x2 + y2 + z2 - dFloat(2.0f)) < dFloat (1.e-2f));

	xy = dFloat (2.0f) * rotation.m_q1 * rotation.m_q2;
	xz = dFloat (2.0f) * rotation.m_q1 * rotation.m_q3;
	xw = dFloat (2.0f) * rotation.m_q1 * rotation.m_q0;
	yz = dFloat (2.0f) * rotation.m_q2 * rotation.m_q3;
	yw = dFloat (2.0f) * rotation.m_q2 * rotation.m_q0;
	zw = dFloat (2.0f) * rotation.m_q3 * rotation.m_q0;

	m_front = dVector (dFloat(1.0f) - y2 - z2, xy + zw				 , xz - yw				  , dFloat(0.0f));
	m_up    = dVector (xy - zw				 , dFloat(1.0f) - x2 - z2, yz + xw				  , dFloat(0.0f));
	m_right = dVector (xz + yw				 , yz - xw				 , dFloat(1.0f) - x2 - y2 , dFloat(0.0f));

	m_posit.m_x = position.m_x;
	m_posit.m_y = position.m_y;
	m_posit.m_z = position.m_z;
	m_posit.m_w = dFloat(1.0f);
}


dVector dMatrix::GetXYZ_EulerAngles () const
{
#if 1
	dFloat yaw;
	dFloat roll;
	dFloat pitch;
	const dFloat minSin = 0.99995f;

	const dMatrix& matrix = *this;

	roll = dFloat(0.0f);
	pitch  = dFloat(0.0f);

	yaw = dAsin (- min (max (matrix[0][2], dFloat(-0.999999f)), dFloat(0.999999f)));
	if (matrix[0][2] < minSin) {
		if (matrix[0][2] > (-minSin)) {
			roll = dAtan2 (matrix[0][1], matrix[0][0]);
			pitch = dAtan2 (matrix[1][2], matrix[2][2]);
		} else {
			pitch = dAtan2 (matrix[1][0], matrix[1][1]);
		}
	} else {
		pitch = -dAtan2 (matrix[1][0], matrix[1][1]);
	}

#ifdef _DEBUG
	dMatrix m (dPitchMatrix (pitch) * dYawMatrix(yaw) * dRollMatrix(roll));
	for (int i = 0; i < 3; i ++) {
		for (int j = 0; j < 3; j ++) {
			dFloat error = dAbs (m[i][j] - matrix[i][j]);
			_ASSERTE (error < 5.0e-2f);
		}
	}
#endif
	return dVector (pitch, yaw, roll, dFloat(0.0f));

#else
	dQuaternion quat (*this);
	return quat.GetXYZ_EulerAngles();
#endif
}



dMatrix dMatrix::Inverse () const
{
	return dMatrix (dVector (m_front.m_x, m_up.m_x, m_right.m_x, 0.0f),
					dVector (m_front.m_y, m_up.m_y, m_right.m_y, 0.0f),
		            dVector (m_front.m_z, m_up.m_z, m_right.m_z, 0.0f),
		            dVector (- (m_posit % m_front), - (m_posit % m_up), - (m_posit % m_right), 1.0f));
}

dMatrix dMatrix::Transpose () const
{
	return dMatrix (dVector (m_front.m_x, m_up.m_x, m_right.m_x, 0.0f),
					dVector (m_front.m_y, m_up.m_y, m_right.m_y, 0.0f),
					dVector (m_front.m_z, m_up.m_z, m_right.m_z, 0.0f),
					dVector (0.0f, 0.0f, 0.0f, 1.0f));
}

dMatrix dMatrix::Transpose4X4 () const
{
	return dMatrix (dVector (m_front.m_x, m_up.m_x, m_right.m_x, m_posit.m_x),
					dVector (m_front.m_y, m_up.m_y, m_right.m_y, m_posit.m_y),
					dVector (m_front.m_z, m_up.m_z, m_right.m_z, m_posit.m_z),
					dVector (m_front.m_w, m_up.m_w, m_right.m_w, m_posit.m_w));
							
}

dVector dMatrix::RotateVector (const dVector &v) const
{
	return dVector (v.m_x * m_front.m_x + v.m_y * m_up.m_x + v.m_z * m_right.m_x,
					 v.m_x * m_front.m_y + v.m_y * m_up.m_y + v.m_z * m_right.m_y,
					 v.m_x * m_front.m_z + v.m_y * m_up.m_z + v.m_z * m_right.m_z);
}


dVector dMatrix::UnrotateVector (const dVector &v) const
{
	return dVector (v % m_front, v % m_up, v % m_right);
}


dVector dMatrix::TransformVector (const dVector &v) const
{
	return m_posit + RotateVector(v);
}

dVector dMatrix::UntransformVector (const dVector &v) const
{
	return UnrotateVector(v - m_posit);
}


void dMatrix::TransformTriplex (
	void* const dstPtr, 
	int dstStrideInBytes,
	void* const srcPtr, 
	int srcStrideInBytes, 
	int count) const
{
	int i;
	dFloat x;
	dFloat y;
	dFloat z;
	dFloat* dst;
	dFloat* src;

	dst = (dFloat*) dstPtr;
	src = (dFloat*) srcPtr;

	dstStrideInBytes /= sizeof (dFloat);
	srcStrideInBytes /= sizeof (dFloat);

	for (i = 0 ; i < count; i ++ ) {
		x = src[0];
		y = src[1];
		z = src[2];
		dst[0] = x * m_front.m_x + y * m_up.m_x + z * m_right.m_x + m_posit.m_x;
		dst[1] = x * m_front.m_y + y * m_up.m_y + z * m_right.m_y + m_posit.m_y;
		dst[2] = x * m_front.m_z + y * m_up.m_z + z * m_right.m_z + m_posit.m_z;
		dst += dstStrideInBytes;
		src += srcStrideInBytes;
	}
}


dMatrix dMatrix::operator* (const dMatrix &B) const
{
	const dMatrix& A = *this;
	return dMatrix (dVector (A[0][0] * B[0][0] + A[0][1] * B[1][0] + A[0][2] * B[2][0] + A[0][3] * B[3][0],
							 A[0][0] * B[0][1] + A[0][1] * B[1][1] + A[0][2] * B[2][1] + A[0][3] * B[3][1],
							 A[0][0] * B[0][2] + A[0][1] * B[1][2] + A[0][2] * B[2][2] + A[0][3] * B[3][2],
	                         A[0][0] * B[0][3] + A[0][1] * B[1][3] + A[0][2] * B[2][3] + A[0][3] * B[3][3]),
					dVector (A[1][0] * B[0][0] + A[1][1] * B[1][0] + A[1][2] * B[2][0] + A[1][3] * B[3][0],
						     A[1][0] * B[0][1] + A[1][1] * B[1][1] + A[1][2] * B[2][1] + A[1][3] * B[3][1],
							 A[1][0] * B[0][2] + A[1][1] * B[1][2] + A[1][2] * B[2][2] + A[1][3] * B[3][2],
							 A[1][0] * B[0][3] + A[1][1] * B[1][3] + A[1][2] * B[2][3] + A[1][3] * B[3][3]),
					dVector (A[2][0] * B[0][0] + A[2][1] * B[1][0] + A[2][2] * B[2][0] + A[2][3] * B[3][0],
							 A[2][0] * B[0][1] + A[2][1] * B[1][1] + A[2][2] * B[2][1] + A[2][3] * B[3][1],
							 A[2][0] * B[0][2] + A[2][1] * B[1][2] + A[2][2] * B[2][2] + A[2][3] * B[3][2],
							 A[2][0] * B[0][3] + A[2][1] * B[1][3] + A[2][2] * B[2][3] + A[2][3] * B[3][3]),
					dVector (A[3][0] * B[0][0] + A[3][1] * B[1][0] + A[3][2] * B[2][0] + A[3][3] * B[3][0],
							 A[3][0] * B[0][1] + A[3][1] * B[1][1] + A[3][2] * B[2][1] + A[3][3] * B[3][1],
							 A[3][0] * B[0][2] + A[3][1] * B[1][2] + A[3][2] * B[2][2] + A[3][3] * B[3][2],
							 A[3][0] * B[0][3] + A[3][1] * B[1][3] + A[3][2] * B[2][3] + A[3][3] * B[3][3]));
}





dVector dMatrix::TransformPlane (const dVector &localPlane) const
{
	dVector tmp (RotateVector (localPlane));  
	tmp.m_w = localPlane.m_w - (localPlane % UnrotateVector (m_posit));  
	return tmp;  
}

dVector dMatrix::UntransformPlane (const dVector &globalPlane) const
{
	dVector tmp (UnrotateVector (globalPlane));
	tmp.m_w = globalPlane % m_posit + globalPlane.m_w;
	return tmp;
}

bool dMatrix::SanityCheck() const
{
	dVector right (m_front * m_up);
	if (dAbs (right % m_right) < 0.9999f) {
		return false;
	}
	if (dAbs (m_right.m_w) > 0.0f) {
		return false;
	}
	if (dAbs (m_up.m_w) > 0.0f) {
		return false;
	}
	if (dAbs (m_right.m_w) > 0.0f) {
		return false;
	}

	if (dAbs (m_posit.m_w) != 1.0f) {
		return false;
	}

	return true;
}



// calculate an orthonormal matrix with the front vector pointing on the 
// dir direction, and the up and right are determined by using the GramSchidth procedure
dMatrix dgGrammSchmidt(const dVector& dir)
{
	dVector up;
	dVector right;
	dVector front (dir); 

	front = front.Scale(1.0f / dSqrt (front % front));
	if (dAbs (front.m_z) > 0.577f) {
		right = front * dVector (-front.m_y, front.m_z, 0.0f);
	} else {
	  	right = front * dVector (-front.m_y, front.m_x, 0.0f);
	}
  	right = right.Scale (1.0f / dSqrt (right % right));
  	up = right * front;

	front.m_w = 0.0f;
	up.m_w = 0.0f;
	right.m_w = 0.0f;
	return dMatrix (front, up, right, dVector (0.0f, 0.0f, 0.0f, 1.0f));
}


dMatrix dPitchMatrix(dFloat ang)
{
	dFloat cosAng;
	dFloat sinAng;
	sinAng = dSin (ang);
	cosAng = dCos (ang);
	return dMatrix (dVector (1.0f,    0.0f,    0.0f, 0.0f), 
					dVector (0.0f,  cosAng,  sinAng, 0.0f),
					dVector (0.0f, -sinAng,  cosAng, 0.0f), 
					dVector (0.0f,    0.0f,    0.0f, 1.0f)); 

}

dMatrix dYawMatrix(dFloat ang)
{
	dFloat cosAng;
	dFloat sinAng;
	sinAng = dSin (ang);
	cosAng = dCos (ang);
	return dMatrix (dVector (cosAng, 0.0f, -sinAng, 0.0f), 
					dVector (0.0f,   1.0f,    0.0f, 0.0f), 
					dVector (sinAng, 0.0f,  cosAng, 0.0f), 
					dVector (0.0f,   0.0f,    0.0f, 1.0f)); 
}

dMatrix dRollMatrix(dFloat ang)
{
	dFloat cosAng;
	dFloat sinAng;
	sinAng = dSin (ang);
	cosAng = dCos (ang);
	return dMatrix (dVector ( cosAng, sinAng, 0.0f, 0.0f), 
					dVector (-sinAng, cosAng, 0.0f, 0.0f),
					dVector (   0.0f,   0.0f, 1.0f, 0.0f), 
					dVector (   0.0f,   0.0f, 0.0f, 1.0f)); 
}																		 
