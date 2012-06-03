/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// mathlib.c -- math primitives

#include "cmdlib.h"
#include "mathlib.h"

#ifdef _MSC_VER
//Improve floating-point consistency.
//without this option weird floating point issues occur
#pragma optimize( "p", on )
#endif


vec3_t          vec3_origin = { 0, 0, 0 };

/*
** NormalToLatLong
**
** We use two byte encoded normals in some space critical applications.
** Lat = 0 at (1,0,0) to 360 (-1,0,0), encoded in 8-bit sine table format
** Lng = 0 at (0,0,1) to 180 (0,0,-1), encoded in 8-bit sine table format
**
*/
void NormalToLatLong(const vec3_t normal, byte bytes[2])
{
	// check for singularities
	if(normal[0] == 0 && normal[1] == 0)
	{
		if(normal[2] > 0)
		{
			bytes[0] = 0;
			bytes[1] = 0;		// lat = 0, long = 0
		}
		else
		{
			bytes[0] = 128;
			bytes[1] = 0;		// lat = 0, long = 128
		}
	}
	else
	{
		int             a, b;

		a = RAD2DEG(atan2(normal[1], normal[0])) * (255.0f / 360.0f);
		a &= 0xff;

		b = RAD2DEG(acos(normal[2])) * (255.0f / 360.0f);
		b &= 0xff;

		bytes[0] = b;			// longitude
		bytes[1] = a;			// lattitude
	}
}

/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points if cw == qtrue
=====================
*/
qboolean PlaneFromPoints(vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c, qboolean cw)
{
	vec3_t          d1, d2;

	VectorSubtract(b, a, d1);
	VectorSubtract(c, a, d2);

	if(cw)
	{
		CrossProduct(d2, d1, plane);
	}
	else
	{
		CrossProduct(d1, d2, plane);
	}

	if(VectorNormalize(plane) == 0)
	{
		return qfalse;
	}

	plane[3] = DotProduct(a, plane);
	return qtrue;
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors(vec3_t forward, vec3_t right, vec3_t up)
{
	float           d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct(right, forward);
	VectorMA(right, -d, forward, right);
	VectorNormalize(right);
	CrossProduct(right, forward, up);
}


void Vec10Copy(vec_t * in, vec_t * out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = in[3];
	out[4] = in[4];
	out[5] = in[5];
	out[6] = in[6];
	out[7] = in[7];
	out[8] = in[8];
	out[9] = in[9];
}


/*
================
VectorIsOnAxis
================
*/
qboolean VectorIsOnAxis(vec3_t v)
{
	int	i, zeroComponentCount;

	zeroComponentCount = 0;
	for (i = 0; i < 3; i++)
	{
		if (v[i] == 0.0)
		{
			zeroComponentCount++;
		}
	}

	if (zeroComponentCount > 1)
	{
		// The zero vector will be on axis.
		return qtrue;
	}

	return qfalse;
}

/*
================
VectorIsOnAxialPlane
================
*/
qboolean VectorIsOnAxialPlane(vec3_t v)
{
	int	i;

	for (i = 0; i < 3; i++)
	{
		if (v[i] == 0.0)
		{
			// The zero vector will be on axial plane.
			return qtrue;
		}
	}

	return qfalse;
}

void VectorRotate3x3(vec3_t v, float r[3][3], vec3_t d)
{
	d[0] = v[0] * r[0][0] + v[1] * r[1][0] + v[2] * r[2][0];
	d[1] = v[0] * r[0][1] + v[1] * r[1][1] + v[2] * r[2][1];
	d[2] = v[0] * r[0][2] + v[1] * r[1][2] + v[2] * r[2][2];
}

double VectorLength(const vec3_t v)
{
	int             i;
	double          length;

	length = 0;
	for(i = 0; i < 3; i++)
		length += v[i] * v[i];
	length = sqrt(length);		// FIXME

	return length;
}

int VectorCompare(const vec3_t v1, const vec3_t v2)
{
	int             i;

	for(i = 0; i < 3; i++)
		if(fabs(v1[i] - v2[i]) > EQUAL_EPSILON)
			return 0;

	return 1;
}

vec_t Q_rint(vec_t in)
{
	return floor(in + 0.5);
}

void CrossProduct(const vec3_t v1, const vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

vec_t _DotProduct(vec3_t v1, vec3_t v2)
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void _VectorSubtract(vec3_t va, vec3_t vb, vec3_t out)
{
	out[0] = va[0] - vb[0];
	out[1] = va[1] - vb[1];
	out[2] = va[2] - vb[2];
}

void _VectorAdd(vec3_t va, vec3_t vb, vec3_t out)
{
	out[0] = va[0] + vb[0];
	out[1] = va[1] + vb[1];
	out[2] = va[2] + vb[2];
}

void _VectorCopy(vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void _VectorScale(vec3_t v, vec_t scale, vec3_t out)
{
	out[0] = v[0] * scale;
	out[1] = v[1] * scale;
	out[2] = v[2] * scale;
}

void _VectorMA(const vec3_t va, double scale, const vec3_t vb, vec3_t vc)
{
	vc[0] = va[0] + scale * vb[0];
	vc[1] = va[1] + scale * vb[1];
	vc[2] = va[2] + scale * vb[2];
}

vec_t VectorNormalize(vec3_t v)
{
	vec_t           length, ilength;

	length = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if(length)
	{
		ilength = 1.0 / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;
}

vec_t VectorNormalize2(const vec3_t in, vec3_t out)
{
	vec_t           length, ilength;

	length = sqrt(in[0] * in[0] + in[1] * in[1] + in[2] * in[2]);
	if(length == 0)
	{
		VectorClear(out);
		return 0;
	}

	ilength = 1.0 / length;
	out[0] = in[0] * ilength;
	out[1] = in[1] * ilength;
	out[2] = in[2] * ilength;

	return length;
}

vec_t ColorNormalize(const vec3_t in, vec3_t out)
{
	float           max, scale;

	max = in[0];
	if(in[1] > max)
		max = in[1];
	if(in[2] > max)
		max = in[2];

	if(max == 0)
	{
		out[0] = out[1] = out[2] = 1.0;
		return 0;
	}

	scale = 1.0 / max;

	VectorScale(in, scale, out);

	return max;
}



void VectorInverse(vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void ClearBounds(vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs)
{
	int             i;
	vec_t           val;

	for(i = 0; i < 3; i++)
	{
		val = v[i];
		if(val < mins[i])
			mins[i] = val;
		if(val > maxs[i])
			maxs[i] = val;
	}
}

void BoundsAdd(vec3_t mins, vec3_t maxs, const vec3_t mins2, const vec3_t maxs2)
{
	if(mins2[0] < mins[0])
	{
		mins[0] = mins2[0];
	}
	if(mins2[1] < mins[1])
	{
		mins[1] = mins2[1];
	}
	if(mins2[2] < mins[2])
	{
		mins[2] = mins2[2];
	}

	if(maxs2[0] > maxs[0])
	{
		maxs[0] = maxs2[0];
	}
	if(maxs2[1] > maxs[1])
	{
		maxs[1] = maxs2[1];
	}
	if(maxs2[2] > maxs[2])
	{
		maxs[2] = maxs2[2];
	}
}


/*
=================
PlaneTypeForNormal
=================
*/
int PlaneTypeForNormal(vec3_t normal)
{
	if(normal[0] == 1.0 || normal[0] == -1.0)
		return PLANE_X;
	if(normal[1] == 1.0 || normal[1] == -1.0)
		return PLANE_Y;
	if(normal[2] == 1.0 || normal[2] == -1.0)
		return PLANE_Z;

	return PLANE_NON_AXIAL;
}

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal)
{
	float           d;
	vec3_t          n;
	float           inv_denom;

	inv_denom = 1.0F / DotProduct(normal, normal);

	d = DotProduct(normal, p) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector(vec3_t dst, const vec3_t src)
{
	int             pos;
	int             i;
	float           minelem = 1.0F;
	vec3_t          tempvec;

	/*
	 ** find the smallest magnitude axially aligned vector
	 */
	for(pos = 0, i = 0; i < 3; i++)
	{
		if(fabs(src[i]) < minelem)
		{
			pos = i;
			minelem = fabs(src[i]);
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	 ** project the point onto the plane defined by src
	 */
	ProjectPointOnPlane(dst, tempvec, src);

	/*
	 ** normalize the result
	 */
	VectorNormalize(dst);
}


void AxisMultiply(float in1[3][3], float in2[3][3], float out[3][3])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}

/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees)
{
	float           m[3][3];
	float           im[3][3];
	float           zrot[3][3];
	float           tmpmat[3][3];
	float           rot[3][3];
	int             i;
	vec3_t          vr, vup, vf;
	float           rad;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector(vr, dir);
	CrossProduct(vr, vf, vup);

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy(im, m, sizeof(im));

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset(zrot, 0, sizeof(zrot));
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	rad = DEG2RAD(degrees);
	zrot[0][0] = cos(rad);
	zrot[0][1] = sin(rad);
	zrot[1][0] = -sin(rad);
	zrot[1][1] = cos(rad);

	AxisMultiply(m, zrot, tmpmat);
	AxisMultiply(tmpmat, im, rot);

	for(i = 0; i < 3; i++)
	{
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

// *INDENT-OFF*
void MatrixIdentity(matrix_t m)
{
	m[ 0] = 1;      m[ 4] = 0;      m[ 8] = 0;      m[12] = 0;
	m[ 1] = 0;      m[ 5] = 1;      m[ 9] = 0;      m[13] = 0;
	m[ 2] = 0;      m[ 6] = 0;      m[10] = 1;      m[14] = 0;
	m[ 3] = 0;      m[ 7] = 0;      m[11] = 0;      m[15] = 1;
}

void MatrixClear(matrix_t m)
{
    m[ 0] = 0;      m[ 4] = 0;      m[ 8] = 0;      m[12] = 0;
	m[ 1] = 0;      m[ 5] = 0;      m[ 9] = 0;      m[13] = 0;
	m[ 2] = 0;      m[ 6] = 0;      m[10] = 0;      m[14] = 0;
	m[ 3] = 0;      m[ 7] = 0;      m[11] = 0;      m[15] = 0;
}

void MatrixCopy(const matrix_t in, matrix_t out)
{
	out[ 0] = in[ 0];       out[ 4] = in[ 4];       out[ 8] = in[ 8];       out[12] = in[12];
	out[ 1] = in[ 1];       out[ 5] = in[ 5];       out[ 9] = in[ 9];       out[13] = in[13];
	out[ 2] = in[ 2];       out[ 6] = in[ 6];       out[10] = in[10];       out[14] = in[14];
	out[ 3] = in[ 3];       out[ 7] = in[ 7];       out[11] = in[11];       out[15] = in[15];
}

void MatrixTranspose(const matrix_t in, matrix_t out)
{
	out[ 0] = in[ 0];       out[ 1] = in[ 4];       out[ 2] = in[ 8];       out[ 3] = in[12];
	out[ 4] = in[ 1];       out[ 5] = in[ 5];       out[ 6] = in[ 9];       out[ 7] = in[13];
	out[ 8] = in[ 2];       out[ 9] = in[ 6];       out[10] = in[10];       out[11] = in[14];
	out[12] = in[ 3];       out[13] = in[ 7];       out[14] = in[11];       out[15] = in[15];
}

// helper functions for MatrixInverse from GtkRadiant C mathlib
typedef vec_t   matrix3x3_t[9];

static float m3_det( matrix3x3_t mat )
{
  float det;

  det = mat[0] * ( mat[4]*mat[8] - mat[7]*mat[5] )
    - mat[1] * ( mat[3]*mat[8] - mat[6]*mat[5] )
    + mat[2] * ( mat[3]*mat[7] - mat[6]*mat[4] );

  return( det );
}

static int m3_inverse( matrix3x3_t mr, matrix3x3_t ma )
{
  float det = m3_det( ma );

  if (det == 0 )
  {
    return 1;
  }


  mr[0] =    ma[4]*ma[8] - ma[5]*ma[7]   / det;
  mr[1] = -( ma[1]*ma[8] - ma[7]*ma[2] ) / det;
  mr[2] =    ma[1]*ma[5] - ma[4]*ma[2]   / det;

  mr[3] = -( ma[3]*ma[8] - ma[5]*ma[6] ) / det;
  mr[4] =    ma[0]*ma[8] - ma[6]*ma[2]   / det;
  mr[5] = -( ma[0]*ma[5] - ma[3]*ma[2] ) / det;

  mr[6] =    ma[3]*ma[7] - ma[6]*ma[4]   / det;
  mr[7] = -( ma[0]*ma[7] - ma[6]*ma[1] ) / det;
  mr[8] =    ma[0]*ma[4] - ma[1]*ma[3]   / det;

  return 0;
}

static void m4_submat( matrix_t mr, matrix3x3_t mb, int i, int j )
{
  int ti, tj, idst, jdst;

  for ( ti = 0; ti < 4; ti++ )
  {
    if ( ti < i )
      idst = ti;
    else
      if ( ti > i )
        idst = ti-1;

      for ( tj = 0; tj < 4; tj++ )
      {
        if ( tj < j )
          jdst = tj;
        else
          if ( tj > j )
            jdst = tj-1;

          if ( ti != i && tj != j )
            mb[idst*3 + jdst] = mr[ti*4 + tj ];
      }
  }
}

float MatrixDet(matrix_t mr)
{
  float  det, result = 0, i = 1;
  matrix3x3_t msub3;
  int     n;

  for ( n = 0; n < 4; n++, i *= -1 )
  {
    m4_submat( mr, msub3, 0, n );

    det     = m3_det( msub3 );
    result += mr[n] * det * i;
  }

  return result;
}

qboolean MatrixInverse(matrix_t matrix)
{
  float  mdet = MatrixDet(matrix);
  matrix3x3_t mtemp;
  int     i, j, sign;
  matrix_t m4x4_temp;

#if 0
  if ( fabs( mdet ) < 0.0000000001 )
    return qtrue;
#endif

  MatrixCopy(matrix, m4x4_temp);

  for ( i = 0; i < 4; i++ )
    for ( j = 0; j < 4; j++ )
    {
      sign = 1 - ( (i +j) % 2 ) * 2;

      m4_submat( m4x4_temp, mtemp, i, j );

	  // FIXME: try using * inverse det and see if speed/accuracy are good enough
      matrix[i+j*4] = ( m3_det( mtemp ) * sign ) / mdet;
    }

  return qfalse;
}

void MatrixSetupXRotation(matrix_t m, vec_t degrees)
{
	vec_t a = DEG2RAD(degrees);

	m[ 0] = 1;      m[ 4] = 0;              m[ 8] = 0;              m[12] = 0;
	m[ 1] = 0;      m[ 5] = cos(a);         m[ 9] =-sin(a);         m[13] = 0;
	m[ 2] = 0;      m[ 6] = sin(a);         m[10] = cos(a);         m[14] = 0;
	m[ 3] = 0;      m[ 7] = 0;              m[11] = 0;              m[15] = 1;
}

void MatrixSetupYRotation(matrix_t m, vec_t degrees)
{
	vec_t a = DEG2RAD(degrees);

	m[ 0] = cos(a);         m[ 4] = 0;      m[ 8] = sin(a);         m[12] = 0;
	m[ 1] = 0;              m[ 5] = 1;      m[ 9] = 0;              m[13] = 0;
	m[ 2] =-sin(a);         m[ 6] = 0;      m[10] = cos(a);         m[14] = 0;
	m[ 3] = 0;              m[ 7] = 0;      m[11] = 0;              m[15] = 1;
}

void MatrixSetupZRotation(matrix_t m, vec_t degrees)
{
	vec_t a = DEG2RAD(degrees);

	m[ 0] = cos(a);         m[ 4] =-sin(a);         m[ 8] = 0;      m[12] = 0;
	m[ 1] = sin(a);         m[ 5] = cos(a);         m[ 9] = 0;      m[13] = 0;
	m[ 2] = 0;              m[ 6] = 0;              m[10] = 1;      m[14] = 0;
	m[ 3] = 0;              m[ 7] = 0;              m[11] = 0;      m[15] = 1;
}

void MatrixSetupTranslation(matrix_t m, vec_t x, vec_t y, vec_t z)
{
	m[ 0] = 1;      m[ 4] = 0;      m[ 8] = 0;      m[12] = x;
	m[ 1] = 0;      m[ 5] = 1;      m[ 9] = 0;      m[13] = y;
	m[ 2] = 0;      m[ 6] = 0;      m[10] = 1;      m[14] = z;
	m[ 3] = 0;      m[ 7] = 0;      m[11] = 0;      m[15] = 1;
}

void MatrixSetupScale(matrix_t m, vec_t x, vec_t y, vec_t z)
{
	m[ 0] = x;      m[ 4] = 0;      m[ 8] = 0;      m[12] = 0;
	m[ 1] = 0;      m[ 5] = y;      m[ 9] = 0;      m[13] = 0;
	m[ 2] = 0;      m[ 6] = 0;      m[10] = z;      m[14] = 0;
	m[ 3] = 0;      m[ 7] = 0;      m[11] = 0;      m[15] = 1;
}

void MatrixMultiply(const matrix_t a, const matrix_t b, matrix_t out)
{
	out[ 0] = b[ 0]*a[ 0] + b[ 1]*a[ 4] + b[ 2]*a[ 8] + b[ 3]*a[12];
	out[ 1] = b[ 0]*a[ 1] + b[ 1]*a[ 5] + b[ 2]*a[ 9] + b[ 3]*a[13];
	out[ 2] = b[ 0]*a[ 2] + b[ 1]*a[ 6] + b[ 2]*a[10] + b[ 3]*a[14];
	out[ 3] = b[ 0]*a[ 3] + b[ 1]*a[ 7] + b[ 2]*a[11] + b[ 3]*a[15];

	out[ 4] = b[ 4]*a[ 0] + b[ 5]*a[ 4] + b[ 6]*a[ 8] + b[ 7]*a[12];
	out[ 5] = b[ 4]*a[ 1] + b[ 5]*a[ 5] + b[ 6]*a[ 9] + b[ 7]*a[13];
	out[ 6] = b[ 4]*a[ 2] + b[ 5]*a[ 6] + b[ 6]*a[10] + b[ 7]*a[14];
	out[ 7] = b[ 4]*a[ 3] + b[ 5]*a[ 7] + b[ 6]*a[11] + b[ 7]*a[15];

	out[ 8] = b[ 8]*a[ 0] + b[ 9]*a[ 4] + b[10]*a[ 8] + b[11]*a[12];
	out[ 9] = b[ 8]*a[ 1] + b[ 9]*a[ 5] + b[10]*a[ 9] + b[11]*a[13];
	out[10] = b[ 8]*a[ 2] + b[ 9]*a[ 6] + b[10]*a[10] + b[11]*a[14];
	out[11] = b[ 8]*a[ 3] + b[ 9]*a[ 7] + b[10]*a[11] + b[11]*a[15];

	out[12] = b[12]*a[ 0] + b[13]*a[ 4] + b[14]*a[ 8] + b[15]*a[12];
	out[13] = b[12]*a[ 1] + b[13]*a[ 5] + b[14]*a[ 9] + b[15]*a[13];
	out[14] = b[12]*a[ 2] + b[13]*a[ 6] + b[14]*a[10] + b[15]*a[14];
	out[15] = b[12]*a[ 3] + b[13]*a[ 7] + b[14]*a[11] + b[15]*a[15];
}

void MatrixMultiply2(matrix_t m, const matrix_t m2)
{
	matrix_t        tmp;

	MatrixCopy(m, tmp);
	MatrixMultiply(tmp, m2, m);
}

void MatrixMultiplyRotation(matrix_t m, vec_t pitch, vec_t yaw, vec_t roll)
{
	matrix_t        tmp, rot;

	MatrixCopy(m, tmp);
	MatrixFromAngles(rot, pitch, yaw, roll);

	MatrixMultiply(rot, tmp, m);
}

void MatrixMultiplyTranslation(matrix_t m, vec_t x, vec_t y, vec_t z)
{
	matrix_t        tmp, trans;

	MatrixCopy(m, tmp);
	MatrixSetupTranslation(trans, x, y, z);
	MatrixMultiply(trans, tmp, m);
}

void MatrixMultiplyScale(matrix_t m, vec_t x, vec_t y, vec_t z)
{
	matrix_t        tmp, scale;

	MatrixCopy(m, tmp);
	MatrixSetupScale(scale, x, y, z);
	MatrixMultiply(scale, tmp, m);
}

void MatrixFromAngles(matrix_t m, vec_t pitch, vec_t yaw, vec_t roll)
{
	static float    sr, sp, sy, cr, cp, cy;

	// static to help MS compiler fp bugs
	sp = sin(DEG2RAD(pitch));
	cp = cos(DEG2RAD(pitch));

	sy = sin(DEG2RAD(yaw));
	cy = cos(DEG2RAD(yaw));

	sr = sin(DEG2RAD(roll));
	cr = cos(DEG2RAD(roll));

	m[ 0] = cp*cy;  m[ 4] = (sr*sp*cy+cr*-sy);      m[ 8] = (cr*sp*cy+-sr*-sy);     m[12] = 0;
	m[ 1] = cp*sy;  m[ 5] = (sr*sp*sy+cr*cy);       m[ 9] = (cr*sp*sy+-sr*cy);      m[13] = 0;
	m[ 2] = -sp;    m[ 6] = sr*cp;                  m[10] = cr*cp;                  m[14] = 0;
	m[ 3] = 0;      m[ 7] = 0;                      m[11] = 0;                      m[15] = 1;
}

void MatrixFromVectorsFLU(matrix_t m, const vec3_t forward, const vec3_t left, const vec3_t up)
{
	m[ 0] = forward[0];     m[ 4] = left[0];        m[ 8] = up[0];  m[12] = 0;
	m[ 1] = forward[1];     m[ 5] = left[1];        m[ 9] = up[1];  m[13] = 0;
	m[ 2] = forward[2];     m[ 6] = left[2];        m[10] = up[2];  m[14] = 0;
	m[ 3] = 0;              m[ 7] = 0;              m[11] = 0;      m[15] = 1;
}

void MatrixFromVectorsFRU(matrix_t m, const vec3_t forward, const vec3_t right, const vec3_t up)
{
    m[ 0] = forward[0];     m[ 4] =-right[0];       m[ 8] = up[0];  m[12] = 0;
	m[ 1] = forward[1];     m[ 5] =-right[1];       m[ 9] = up[1];  m[13] = 0;
	m[ 2] = forward[2];     m[ 6] =-right[2];       m[10] = up[2];  m[14] = 0;
	m[ 3] = 0;              m[ 7] = 0;              m[11] = 0;      m[15] = 1;
}

void MatrixToVectorsFLU(const matrix_t m, vec3_t forward, vec3_t left, vec3_t up)
{
	if(forward)
	{
		forward[0] = m[ 0];     // cp*cy;
		forward[1] = m[ 1];     // cp*sy;
		forward[2] = m[ 2];     //-sp;
	}

    if(left)
    {
		left[0] = m[ 4];        // sr*sp*cy+cr*-sy;
		left[1] = m[ 5];        // sr*sp*sy+cr*cy;
		left[2] = m[ 6];        // sr*cp;
    }

	if(up)
	{
		up[0] = m[ 8];  // cr*sp*cy+-sr*-sy;
		up[1] = m[ 9];  // cr*sp*sy+-sr*cy;
		up[2] = m[10];  // cr*cp;
	}
}

void MatrixToVectorsFRU(const matrix_t m, vec3_t forward, vec3_t right, vec3_t up)
{
	if(forward)
	{
		forward[0] = m[ 0];
		forward[1] = m[ 1];
		forward[2] = m[ 2];
	}

	if(right)
	{
		right[0] =-m[ 4];
		right[1] =-m[ 5];
		right[2] =-m[ 6];
	}

	if(up)
	{
		up[0] = m[ 8];
		up[1] = m[ 9];
		up[2] = m[10];
	}
}

void MatrixSetupTransform(matrix_t m, const vec3_t forward, const vec3_t left, const vec3_t up, const vec3_t origin)
{
	m[ 0] = forward[0];     m[ 4] = left[0];        m[ 8] = up[0];  m[12] = origin[0];
	m[ 1] = forward[1];     m[ 5] = left[1];        m[ 9] = up[1];  m[13] = origin[1];
	m[ 2] = forward[2];     m[ 6] = left[2];        m[10] = up[2];  m[14] = origin[2];
	m[ 3] = 0;              m[ 7] = 0;              m[11] = 0;      m[15] = 1;
}

void MatrixSetupTransformFromRotation(matrix_t m, const matrix_t rot, const vec3_t origin)
{
	m[ 0] = rot[ 0];     m[ 4] = rot[ 4];        m[ 8] = rot[ 8];  m[12] = origin[0];
	m[ 1] = rot[ 1];     m[ 5] = rot[ 5];        m[ 9] = rot[ 9];  m[13] = origin[1];
	m[ 2] = rot[ 2];     m[ 6] = rot[ 6];        m[10] = rot[10];  m[14] = origin[2];
	m[ 3] = 0;           m[ 7] = 0;              m[11] = 0;        m[15] = 1;
}

void MatrixAffineInverse(const matrix_t in, matrix_t out)
{
	out[ 0] = in[ 0];       out[ 4] = in[ 1];       out[ 8] = in[ 2];
	out[ 1] = in[ 4];       out[ 5] = in[ 5];       out[ 9] = in[ 6];
	out[ 2] = in[ 8];       out[ 6] = in[ 9];       out[10] = in[10];
	out[ 3] = 0;            out[ 7] = 0;            out[11] = 0;            out[15] = 1;

	out[12] = -( in[12] * out[ 0] + in[13] * out[ 4] + in[14] * out[ 8] );
	out[13] = -( in[12] * out[ 1] + in[13] * out[ 5] + in[14] * out[ 9] );
	out[14] = -( in[12] * out[ 2] + in[13] * out[ 6] + in[14] * out[10] );
}

void MatrixTransformNormal(const matrix_t m, const vec3_t in, vec3_t out)
{
	out[ 0] = m[ 0]*in[ 0] + m[ 4]*in[ 1] + m[ 8]*in[ 2];
	out[ 1] = m[ 1]*in[ 0] + m[ 5]*in[ 1] + m[ 9]*in[ 2];
	out[ 2] = m[ 2]*in[ 0] + m[ 6]*in[ 1] + m[10]*in[ 2];
}

void MatrixTransformNormal2(const matrix_t m, vec3_t inout)
{
	vec3_t          tmp;

	tmp[ 0] = m[ 0]*inout[ 0] + m[ 4]*inout[ 1] + m[ 8]*inout[ 2];
	tmp[ 1] = m[ 1]*inout[ 0] + m[ 5]*inout[ 1] + m[ 9]*inout[ 2];
	tmp[ 2] = m[ 2]*inout[ 0] + m[ 6]*inout[ 1] + m[10]*inout[ 2];

	VectorCopy(tmp, inout);
}

void MatrixTransformPoint(const matrix_t m, const vec3_t in, vec3_t out)
{
	out[ 0] = m[ 0]*in[ 0] + m[ 4]*in[ 1] + m[ 8]*in[ 2] + m[12];
	out[ 1] = m[ 1]*in[ 0] + m[ 5]*in[ 1] + m[ 9]*in[ 2] + m[13];
	out[ 2] = m[ 2]*in[ 0] + m[ 6]*in[ 1] + m[10]*in[ 2] + m[14];
}

void MatrixTransformPoint2(const matrix_t m, vec3_t inout)
{
	vec3_t          tmp;

	tmp[ 0] = m[ 0]*inout[ 0] + m[ 4]*inout[ 1] + m[ 8]*inout[ 2] + m[12];
	tmp[ 1] = m[ 1]*inout[ 0] + m[ 5]*inout[ 1] + m[ 9]*inout[ 2] + m[13];
	tmp[ 2] = m[ 2]*inout[ 0] + m[ 6]*inout[ 1] + m[10]*inout[ 2] + m[14];

	VectorCopy(tmp, inout);
}

void MatrixTransformVec4(const matrix_t matrix, vec4_t vector)
{
	float out1, out2, out3, out4;

	out1 =  matrix[0]  * vector[0] + matrix[4]  * vector[1] + matrix[8]  * vector[2] + matrix[12] * vector[3];
	out2 =  matrix[1]  * vector[0] + matrix[5]  * vector[1] + matrix[9]  * vector[2] + matrix[13] * vector[3];
	out3 =  matrix[2]  * vector[0] + matrix[6]  * vector[1] + matrix[10] * vector[2] + matrix[14] * vector[3];
	out4 =  matrix[3]  * vector[0] + matrix[7]  * vector[1] + matrix[11] * vector[2] + matrix[15] * vector[3];

	vector[0] = out1;
	vector[1] = out2;
	vector[2] = out3;
	vector[3] = out4;
}

// *INDENT-ON*
