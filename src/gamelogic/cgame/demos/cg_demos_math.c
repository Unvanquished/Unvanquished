/*
===========================================================================
Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../../shared/demos/bg_demos.h"
#include "cg_demos.h" 
#include <malloc.h>

#ifdef _MSC_VER
#define alloca _alloca
#endif

#define Qx 0
#define Qy 1
#define Qz 2
#define Qw 3

/*
=================
GetPerpendicularViewVector

  Used to find an "up" vector for drawing a sprite so that it always faces the view as best as possible
=================
*/
/*void GetPerpendicularViewVector( const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up )
{
	vec3_t	v1, v2;

	VectorSubtract( point, p1, v1 );
	VectorNormalize( v1 );

	VectorSubtract( point, p2, v2 );
	VectorNormalize( v2 );

	CrossProduct( v1, v2, up );
	VectorNormalize( up );
}*/

void AxisToAngles( const vec3_t axis[3], vec3_t angles ) {
	float	length1;
	float	yaw, pitch, roll = 0;
	
	if ( axis[0][1] == 0 && axis[0][0] == 0 ) {
		yaw = 0;
		if ( axis[0][2] > 0 ) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		if ( axis[0][0] ) {
			yaw = ( atan2 ( axis[0][1], axis[0][0] ) * 180 / M_PI );
		}
		else if ( axis[0][1] > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}

		length1 = sqrt ( axis[0][0]*axis[0][0] + axis[0][1]*axis[0][1] );
		pitch = ( atan2( axis[0][2], length1) * 180 / M_PI );
		if ( pitch < 0 ) {
			pitch += 360;
		}

		roll = ( atan2( axis[1][2], axis[2][2] ) * 180 / M_PI );
		if ( roll < 0 ) {
			roll += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = roll;
}


qboolean inline BoxTraceTestResult( int axis, float dist, const vec3_t start, const vec3_t forward, const vec3_t mins, const vec3_t maxs, vec3_t result ) {
	result[0] = start[0] + forward[0] * dist;
	result[1] = start[1] + forward[1] * dist;
	result[2] = start[2] + forward[2] * dist;

	if (axis != 0 && (result[0] < mins[0] || result[0] > maxs[0]))
		return qfalse;
	if (axis != 1 && (result[1] < mins[1] || result[1] > maxs[1]))
		return qfalse;
	if (axis != 2 && (result[2] < mins[2] || result[2] > maxs[2]))
		return qfalse;
	return qtrue;
}

qboolean inline BoxTraceTestSides( int axis, const vec3_t start, const vec3_t forward, const vec3_t mins, const vec3_t maxs, vec3_t result ) {
	if (forward[axis] > 0 && start[axis] <= mins[axis]) {
		float dist = ( mins[axis] - start[axis] ) / forward[axis];
		if (BoxTraceTestResult(axis, dist, start, forward, mins, maxs, result))
			return qtrue;
	}
	if (forward[axis] < 0 && start[axis] >= maxs[axis]) {
		float dist = ( maxs[axis] - start[axis] ) / forward[axis];
		if (BoxTraceTestResult(axis, dist, start, forward, mins, maxs, result))
			return qtrue;
	}
	return qfalse;
}

qboolean BoxTraceImpact(const vec3_t start, const vec3_t forward, const vec3_t container, vec3_t result ) {
	vec3_t mins, maxs;

	mins[0] = -container[2];
	mins[1] = -container[2];
	mins[2] = container[0];
	maxs[0] = container[2];
	maxs[1] = container[2];
	maxs[2] = container[1];

	if (BoxTraceTestSides(0, start, forward, mins, maxs, result))
		return qtrue;
	if (BoxTraceTestSides(1, start, forward, mins, maxs, result))
		return qtrue;
	if (BoxTraceTestSides(2, start, forward, mins, maxs, result))
		return qtrue;
	return qfalse;
}

qboolean CylinderTraceImpact( const vec3_t start, const vec3_t forward, const vec3_t container, vec3_t result ) {
	float a = forward[0] * forward[0] + forward[1]*forward[1];
	float b = 2*(forward[0]*start[0] + forward[1]*start[1]);
	float c = start[0]*start[0] + start[1]*start[1] - container[2]*container[2];
	float base = b*b - 4*a*c;
	float t,r;

	/* Test if we intersect with the top or bottom */
	if ( (forward[2] > 0 && start[2] <= container[0]) || (forward[2] < 0 && start[2] >= container[1] )) {
		t = (container[0] - start[2] / forward[2]);
		VectorMA( start, t, forward, result );
		if ( (result[0]*result[0] + result[1]*result[1]) < ( container[2] * container[2]))
			return qtrue;
	}

	if (base >=0 ) {
		t = (-b - sqrt( base )) / 2*a;
		VectorMA( start, t, forward, result );
		r = result[0] * result[0] + result[1]*result[1];
		if (result[2] >= container[0] && result[2]<= container[1])
			return qtrue;
	}
	return qfalse;
};

void QuatMultiplyMME( const QuatMME_t q1, const QuatMME_t q2, QuatMME_t qr) {
	qr[Qw] = q1[Qw]*q2[Qw] - q1[Qx]*q2[Qx] - q1[Qy]*q2[Qy] - q1[Qz]*q2[Qz];
	qr[Qx] = q1[Qw]*q2[Qx] + q1[Qx]*q2[Qw] + q1[Qy]*q2[Qz] - q1[Qz]*q2[Qy];
	qr[Qy] = q1[Qw]*q2[Qy] + q1[Qy]*q2[Qw] + q1[Qz]*q2[Qx] - q1[Qx]*q2[Qz];
	qr[Qz] = q1[Qw]*q2[Qz] + q1[Qz]*q2[Qw] + q1[Qx]*q2[Qy] - q1[Qy]*q2[Qx];
}

void QuatFromAxisAngleMME( const vec3_t axis, vec_t angle, QuatMME_t dst) {
	float halfAngle = 0.5 * DEG2RAD(angle);
	float fSin = sin(halfAngle);
    dst[Qw] = cos (halfAngle);
    dst[Qx] = fSin*axis[0];
    dst[Qy] = fSin*axis[1];
    dst[Qz] = fSin*axis[2];
}

void QuatFromAnglesMME( const vec3_t angles, QuatMME_t dst) {
	const float pitchAngle = 0.5 * DEG2RAD(angles[PITCH]);
	const float sinPitch = sin( pitchAngle );
	const float cosPitch = cos( pitchAngle );
	const float yawAngle = 0.5 * DEG2RAD(angles[YAW]);
	const float sinYaw = sin( yawAngle );
	const float cosYaw = cos( yawAngle );
	const float rollAngle = 0.5 * DEG2RAD(angles[ROLL]);
	const float sinRoll = sin( rollAngle );
	const float cosRoll = cos( rollAngle );

	dst[Qx] = sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw;
	dst[Qy] = cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw;
	dst[Qz] = cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw;
	dst[Qw] = cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw;
}

void QuatFromAnglesClosestMME( const vec3_t angles, const QuatMME_t qc, QuatMME_t dst) {
	QuatMME_t t0, t1;

	QuatFromAnglesMME( angles, dst );

	QuatAddMME( qc, dst, t0 );
	QuatSubMME( qc, dst, t1 );
	if (QuatLengthSquaredMME( t0 ) < QuatLengthSquaredMME( t1))
		QuatNegateMME( dst, dst );
}

void QuatToAxisAngleMME (const QuatMME_t q, vec3_t axis, float *angle) {
    // The quaternion representing the rotation is
    //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)
	float sqrLength = q[Qx]*q[Qx] + q[Qy]*q[Qy] + q[Qz]*q[Qz];
	if ( sqrLength > 0.00001 ) {
		float invLength = 1 / sqrt( sqrLength );
		*angle = 2 * acos( q[Qw] );
		axis[0] = q[Qx] * invLength;
		axis[1] = q[Qy] * invLength;
		axis[2] = q[Qz] * invLength;
    } else {
		*angle = 0;
		axis[0] = 1;
		axis[1] = 0;
		axis[2] = 0;
	}
}

void QuatToAxisMME( const QuatMME_t q, vec3_t axis[3]) {
	const float tx = 2 * q[Qx];
	const float ty = 2 * q[Qy];
	const float tz = 2 * q[Qz];
	const float twx = q[Qw] * tx;
	const float twy = q[Qw] * ty;
	const float twz = q[Qw] * tz;
	const float txx = q[Qx] * tx;
	const float txy = q[Qx] * ty;
	const float txz = q[Qx] * tz;
	const float tyy = q[Qy] * ty;
	const float tyz = q[Qy] * tz;
	const float tzz = q[Qz] * tz;

	axis[0][0] = 1 - tyy - tzz;
	axis[1][0] = txy - twz;
	axis[2][0] = txz + twy;
	axis[0][1] = txy + twz;
	axis[1][1] = 1 - txx - tzz;
	axis[2][1] = tyz - twx;
	axis[0][2] = txz - twy;
	axis[1][2] = tyz + twx;
	axis[2][2] = 1 - txx - tyy;
#if 0	
	axis[0][0] = 1 - tyy - tzz;
	axis[0][1] = txy - twz;
	axis[0][2] = txz + twy;
	axis[1][0] = txy + twz;
	axis[1][1] = 1 - txx - tzz;
	axis[1][2] = tyz - twx;
	axis[2][0] = txz - twy;
	axis[2][1] = tyz + twx;
	axis[2][2] = 1 - txx - tyy;
#endif
}

void QuatToAnglesMME( const QuatMME_t q, vec3_t angles) {
	vec3_t axis[3];
	QuatToAxisMME( q, axis );
	AxisToAngles( axis, angles );
}

void QuatLogMME(const QuatMME_t q0, QuatMME_t qr) { 
    if ( fabs(q0[Qw]) < 1 ) {
        float fAngle = acos(q0[Qw]);
		float fSin = sin(fAngle);
		if (fabs(fSin) >= 0.00005 ) {
            float fCoeff = fAngle/fSin;
			qr[Qw] = 0;
			qr[Qx] = fCoeff * q0[Qx];
			qr[Qy] = fCoeff * q0[Qy];
			qr[Qz] = fCoeff * q0[Qz];
			return;
        }
    }
	qr[Qw] = 0;
	qr[Qx] = q0[Qx];
	qr[Qy] = q0[Qy];
	qr[Qz] = q0[Qz];
}

void QuatExpMME(const QuatMME_t q0, QuatMME_t qr) {
	float fAngle = sqrt( q0[Qx]*q0[Qx] + q0[Qy]*q0[Qy] + q0[Qz]*q0[Qz]);
	float fSin = sin(fAngle);
	qr[Qw] = cos(fAngle);

    if ( fabs(fSin) >= 0.00005 ) {
		float fCoeff = fSin/fAngle;
		qr[Qx] = fCoeff * q0[Qx];
		qr[Qy] = fCoeff * q0[Qy];
		qr[Qz] = fCoeff * q0[Qz];
    } else {
		qr[Qx] = q0[Qx];
		qr[Qy] = q0[Qy];
		qr[Qz] = q0[Qz];
    }
}

static void QuatSquadPoint(const QuatMME_t q0, const QuatMME_t q1, const QuatMME_t q2, QuatMME_t qr) {
	QuatMME_t qi, qt, ql0, ql1;

	QuatConjugateMME( q1, qi );
	QuatMultiplyMME( qi, q0, qt);
	QuatLogMME( qt, ql0 );
	QuatMultiplyMME( qi, q2, qt);
	QuatLogMME( qt, ql1 );
	QuatAddMME( ql0, ql1, qt );
	QuatScaleMME( qt, -0.25f, ql0);
	QuatExpMME( ql0, qt);
	QuatMultiplyMME( q1, qt, qr );
}

void QuatSlerpMME(float t, const QuatMME_t q0, const QuatMME_t q1, QuatMME_t qr) { 
	float fCos = QuatDotMME( q0, q1 );
	float angle = acos( fCos );
	if ( fabs(angle) >= 0.00001 ) {
		float invSin = 1 / sin(angle);
        float m0 = sin((1-t)*angle)*invSin;
        float m1 = sin(t*angle)*invSin;
		QuatScaleMME( q0, m0, qr );
		QuatScaleAddMME( qr, q1, m1, qr );
    } else {
		QuatCopyMME( q0, qr );
    }
}

void QuatSquadMME( float t, const QuatMME_t q0, const QuatMME_t q1, const QuatMME_t q2, const QuatMME_t q3, QuatMME_t qr) {  
	QuatMME_t qp0, qp1, qt0, qt1;

	QuatSquadPoint( q0, q1, q2, qp0);
	QuatSquadPoint( q1, q2, q3, qp1);
	QuatSlerpMME( t, q1, q2, qt0 );
	QuatSlerpMME( t, qp0, qp1, qt1 );
	QuatSlerpMME( 2*t*(1 - t), qt0, qt1, qr );

	QuatNormalizeMME( qr );
}

void QuatTimeSplineMME( float t, const int times[4], const QuatMME_t quats[4], QuatMME_t qr) {
	QuatMME_t	qn, qt;
	QuatMME_t	qa, qb;
	QuatMME_t	qlog10, qlog21, qlog32;
	float fna, fnb; 

	QuatConjugateMME( quats[0], qn);
	QuatMultiplyMME( qn, quats[1], qt );
	QuatLogMME( qt, qlog10 );

	QuatConjugateMME( quats[1], qn);
	QuatMultiplyMME( qn, quats[2], qt );
	QuatLogMME( qt, qlog21 );

	QuatConjugateMME( quats[2], qn);
	QuatMultiplyMME( qn, quats[3], qt );
	QuatLogMME( qt, qlog32 );

	fna = ( times[1] - times[0]) / (0.5 * ( times[2] - times[0]));
	fnb = ( times[3] - times[2]) / (0.5 * ( times[3] - times[1]));

	/* Calc qa */
	QuatScaleMME( qlog10, 1 - 0.5 * fna, qt );
	QuatScaleAddMME( qt, qlog21, -0.5 * fna, qt );
	QuatScaleMME( qt, 0.5f, qt );
	QuatExpMME( qt, qt );
	QuatMultiplyMME( quats[1], qt, qa );

	/* Calc qb */
	QuatScaleMME( qlog21, 0.5*fnb, qt );
	QuatScaleAddMME( qt, qlog32, 0.5 * fnb - 1, qt );
	QuatScaleMME( qt, 0.5f, qt );
	QuatExpMME( qt, qt );
	QuatMultiplyMME( quats[2], qt, qb );

	/* Slerp the final result */
	QuatSlerpMME( t, quats[1], quats[2], qt );
	QuatSlerpMME( t, qa, qb, qn );
	QuatSlerpMME( 2*t*(1 - t), qt, qn, qr );

	QuatNormalizeMME( qr );
}

void QuatTimeSpline2( float t, int t0, int t1, int t2, int t3, const QuatMME_t q0, const QuatMME_t q1, const QuatMME_t q2, const QuatMME_t q3, QuatMME_t qr) {  
	QuatMME_t	qn, qt;
	QuatMME_t	qlog10, qlog21, qlog32;
	QuatMME_t	qa, qb;
	float scale;


	/* Create some logs */
	QuatConjugateMME( q0, qn);
	QuatMultiplyMME( qn, q1, qt );
	QuatLogMME( qt, qlog10 );

	QuatConjugateMME( q1, qn);
	QuatMultiplyMME( qn, q2, qt );
	QuatLogMME( qt, qlog21 );

	QuatConjugateMME( q2, qn);
	QuatMultiplyMME( qn, q3, qt );
	QuatLogMME( qt, qlog32 );
	
	/* Create Outgoing */
	scale = (float)( t2 - t1) / (float)( t2 - t0);
	QuatAddMME( qlog21, qlog10, qt );
	QuatScaleMME( qt, scale, qt );
	QuatSubMME( qt, qlog21, qt );
	QuatScaleMME( qt, 0.5f, qt );
	QuatExpMME( qt, qt );
	QuatMultiplyMME( q1, qt, qb );

	/* Create incoming */
	scale = (float)( t2 - t1) / (float)( t3 - t1);
	QuatAddMME( qlog32, qlog21, qt );
	QuatScaleMME( qt, scale, qt );
	QuatSubMME( qlog21, qt, qt );
	QuatScaleMME( qt, 0.5f, qt );
	QuatExpMME( qt, qt );
	QuatMultiplyMME( q2, qt, qa );

	/* Slerp the final result */
	QuatSlerpMME( t, q1, q2, qt );
	QuatSlerpMME( t, qa, qb, qn );
	QuatSlerpMME( 2*t*(1 - t), qt, qn, qr );

	QuatNormalizeMME( qr );
}


void VectorTimeSpline( float t, const int times[4], float *control, float *result, int dim) {
	int i;
	float diff10[8];
	float diff21[8];
	float diff32[8];
	float tOut[8];
	float tIn[8];
	float scaleIn, scaleOut;
	
	for (i = 0;i<dim;i++) {
		diff10[i] = control[i + 1*dim] - control[i + 0*dim];
		diff21[i] = control[i + 2*dim] - control[i + 1*dim];
		diff32[i] = control[i + 3*dim] - control[i + 2*dim];
	}
	scaleOut = (float)( times[2] - times[1]) / (float)( times[2] - times[0]);
	scaleIn = (float)( times[2] - times[1]) / (float)( times[3] - times[1]);
	for (i = 0;i<dim;i++) {
		tOut[i] = scaleOut * (diff10[i] + diff21[i]);
		tIn[i] = scaleIn * (diff21[i] + diff32[i]);
	}

	for (i = 0;i<dim;i++) {
		result[i] = ( -2 * diff21[i] + tOut[i] + tIn[i]);
		result[i] *= t;
		result[i] += -1*tIn[i] - 2*tOut[i] + 3 * diff21[i];
		result[i] *= t;
		result[i] += tOut[i];
		result[i] *= t;
		result[i] += control[ i + dim];
	}
}

void bezierInterpolate( float t, const vec3_t control[4], vec3_t out) {
	vec4_t matrix;
	matrix[0] = (((-t+3)*t-3)*t+1)/6;
	matrix[1] = (((3*t-6)*t)*t+4)/6;
	matrix[2] = (((-3*t+3)*t+3)*t+1)/6;
	matrix[3] = (t*t*t)/6;
	
	VectorScale( control[0], matrix[0], out );
	VectorMA( out, matrix[1], control[1], out );
	VectorMA( out, matrix[2], control[2], out );
	VectorMA( out, matrix[3], control[3], out );
}

void linearInterpolate( float t, const vec3_t control[4], vec3_t out) {
	vec3_t delta;
	
	VectorSubtract( control[2], control[1], delta );
	VectorMA( control[1], t, delta, out );
}



static float dsMax ( float x, float y ) {
  if ( y > x )
	  return y;
  else 
    return x;
}

static float dsMin ( float x, float y ) {
  if ( y < x )
	  return y;
  else 
    return x;
}

static float dsplineTangent( float h1, float h2, float d1, float d2) {
	float	hsum = h1 + h2;
	float	del1 = d1 / h1;
	float	del2 = d2 / h2;

	if (del1 * del2 == 0) {
		return 0;
	} else {
		float hsumt3 = 3 * hsum;
		float w1 = ( hsum + h1 ) / hsumt3;
		float w2 = ( hsum + h2 ) / hsumt3;
		float dmax = dsMax ( fabs ( del1 ), fabs ( del2 ) );
		float dmin = dsMin ( fabs ( del1 ), fabs ( del2 ) );
		float drat1 = del1 / dmax;
		float drat2 = del2 / dmax;
		return  dmin / ( w1 * drat1 + w2 * drat2 );
	}
}

float dsplineCalc(float x, vec3_t dx, vec3_t dy, float *deriv ) {
	float tan1, tan2;
	float c2, c3;
	float delta, del1, del2;
	
	tan1 = dsplineTangent( dx[0], dx[1], dy[0], dy[1] );
	tan2 = dsplineTangent( dx[1], dx[2], dy[1], dy[2] );

	delta = dy[1] / dx[1];
	del1 =  (tan1 - delta) / dx[1];
	del2 =  (tan2 - delta) / dx[1];
	c2 = - ( del1 + del1 + del2 );
	c3 = (del1 + del2) / dx[1];
	if (deriv) {
		*deriv = tan1 + 2*x*c2 + 3*x*x*c3;
	}
	return  x * ( tan1 + x * ( c2 + x * c3 ) );
}

/* Some line drawing functions */
void demoDrawSetupVerts( polyVert_t *verts, const vec4_t color ) {
	int i;
	for (i = 0; i<4;i++) {
		verts[i].modulate[0] = color[0] * 255;
		verts[i].modulate[1] = color[1] * 255;
		verts[i].modulate[2] = color[2] * 255;
		verts[i].modulate[3] = color[3] * 255;
		verts[i].st[0] = (i & 2) ? 0 : 1;
		verts[i].st[1] = (i & 1) ? 0 : 1;
	}
}

void demoDrawRawLine(const vec3_t start, const vec3_t end, float width, polyVert_t *verts ) {
	vec3_t up;
	vec3_t middle;

	VectorScale( start, 0.5, middle ) ;
	VectorMA( middle, 0.5, end, middle );
	if ( VectorDistance( middle, cg.refdef.vieworg ) < 100 )
		return;
	GetPerpendicularViewVector( cg.refdef.vieworg, start, end, up );
	VectorMA( start, width, up, verts[0].xyz);
	VectorMA( start, -width, up, verts[1].xyz);
	VectorMA( end, -width, up, verts[2].xyz);
	VectorMA( end, width, up, verts[3].xyz);
	trap_R_AddPolyToScene( demo.media.additiveWhiteShader, 4, verts );
}

void demoDrawLine( const vec3_t p0, const vec3_t p1, const vec4_t color) {
	polyVert_t verts[4];

	demoDrawSetupVerts( verts, color );
	demoDrawRawLine( p0, p1, 1, verts );
}

void demoDrawCross( const vec3_t origin, const vec4_t color ) {
	unsigned int i;
	vec3_t start, end;
	polyVert_t verts[4];

	demoDrawSetupVerts( verts, color );
	/* Create a few lines indicating the point */
	for(i = 0; i < 3; i++) {
		VectorCopy(origin, start);
		VectorCopy(origin, end );
		start[i] -= 10;
		end[i] += 10;
		demoDrawRawLine( start, end, 0.6f, verts );
	}
}

void demoDrawBox( const vec3_t origin, const vec3_t container, const vec4_t color ) {
	unsigned int i;
	vec3_t boxCorners[8];
	polyVert_t verts[4];
	static const float xMul[4] = {1,1,-1,-1};
	static const float yMul[4] = {1,-1,-1,1};


	demoDrawSetupVerts( verts, color );
	/* Create the box corners */
	for(i = 0; i < 8; i++) {
		boxCorners[i][0] = origin[0] + (((i+0) & 2) ? +container[2] : -container[2]);
		boxCorners[i][1] = origin[1] + (((i+1) & 2) ? +container[2] : -container[2]);
		boxCorners[i][2] = origin[2] + container[i >>2];
	}
	for (i = 0; i < 4;i++) {
		demoDrawRawLine( boxCorners[i], boxCorners[(i+1) & 3], 1.0f, verts );
		demoDrawRawLine( boxCorners[4+i], boxCorners[4+((i+1) & 3)], 1.0f, verts );
		demoDrawRawLine( boxCorners[i], boxCorners[4+i], 1.0f, verts );
	}
}

void demoDrawCrosshair( void ) {
	float		w, h;
	qhandle_t	hShader;
	float		x, y;
	int			ca;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	w = h = cg_crosshairSize.value;
	w *= cgs.aspectScale;
	CG_AdjustFrom640( &x, &y, &w, &h );
	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cg_weapons[ WP_MACHINEGUN ].crossHair;
	trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), 
		y + cg.refdef.y + 0.5 * (cg.refdef.height - h), 
		w, h, 0, 0, 1, 1, hShader );
}


static void drawGridLine( const vec3_t start, const vec3_t end, float width, polyVert_t *verts, qhandle_t shader ) {
	vec3_t up;
	GetPerpendicularViewVector( cg.refdef.vieworg, start, end, up );
	VectorMA( start, width, up, verts[0].xyz);
	VectorMA( start, -width, up, verts[1].xyz);
	VectorMA( end, -width, up, verts[2].xyz);
	VectorMA( end, width, up, verts[3].xyz);
	trap_R_AddPolyToScene( shader, 4, verts );
}

void demoDrawGrid( const vec3_t input, const vec4_t color, const vec3_t offset, float width, float step, float range, qhandle_t shader ) {
	int i;
	vec3_t min, max;
	vec3_t origin;
	float x, y, z, stepCount, stepScale;

	polyVert_t verts[4];

	demoDrawSetupVerts( verts, color );

	stepCount = range / step;
	if ( stepCount > 128 ) {
		stepCount = 128;
		range = step * stepCount;
	}

	stepScale = range / stepCount;
	stepCount *= 2;


	for ( i = 0; i < 3; i++ ) {
		origin[i] = offset[i] + input[i] - fmod( input[i], step );
		min[i] = origin[i] - range;
		max[i] = origin[i] + range;
	}
	verts[0].st[0] = 0;
	verts[1].st[0] = 0;
	verts[2].st[0] = stepCount + 0;
	verts[3].st[0] = stepCount + 0;

	//Draw the vertical bars	
	for ( x = min[0]; x < max[0]; x+= step ) {
		for ( y = min[1]; y < max[1]; y += step ) {
			vec3_t start, end;

			start[0] = x;
			start[1] = y;
			start[2] = min[2];

			end[0] = x;
			end[1] = y;
			end[2] = max[2];

			drawGridLine( start, end, width, verts, shader );
		}
	}
	//Draw the horizontal bars
	for ( z = min[2]; z < max[2]; z+= step ) {

		for ( x = min[0]; x < max[0]; x+= step ) {
			vec3_t start, end;

			start[0] = x;
			start[1] = min[1];
			start[2] = z;

			end[0] = x;
			end[1] = max[1];
			end[2] = z;

			drawGridLine( start, end, width, verts, shader );
		}
		for ( y = min[1]; y < max[1]; y+= step ) {
			vec3_t start, end;

			start[0] = min[0];
			start[1] = y;
			start[2] = z;

			end[0] = max[0];
			end[1] = y;
			end[2] = z;

			drawGridLine( start, end, width, verts, shader );
		}
	}
};


void demoNowTrajectory( const trajectory_t *tr, vec3_t result ) {
	float		deltaTime;
	float		phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( cg.time - tr->trTime ) * 0.001 + cg.timeFraction * 0.001;
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( cg.time - tr->trTime ) % tr->trDuration;
		deltaTime = ( deltaTime + cg.timeFraction ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( cg.time > tr->trTime + tr->trDuration ) {
			deltaTime = tr->trDuration * 0.001;
		} else {
			deltaTime = ( cg.time - tr->trTime ) * 0.001 + cg.timeFraction * 0.001;
		}
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( cg.time - tr->trTime ) * 0.001 + cg.timeFraction * 0.001;
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	case TR_BUOYANCY:
		deltaTime = ( cg.time - tr->trTime ) * 0.001 + cg.timeFraction * 0.001;
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[ 2 ] += 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime; // FIXME: local gravity...
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trType );
		break;
	}
}

void demoRotatePoint(vec3_t point, const vec3_t matrix[3]) { 
	vec3_t tvec;

	VectorCopy(point, tvec);
	point[0] = DotProduct(matrix[0], tvec);
	point[1] = DotProduct(matrix[1], tvec);
	point[2] = DotProduct(matrix[2], tvec);
}

void demoTransposeMatrix(const vec3_t matrix[3], vec3_t transpose[3]) {
	int i, j;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			transpose[i][j] = matrix[j][i];
		}
	}
}

void demoCreateRotationMatrix(const vec3_t angles, vec3_t matrix[3]) {
	AngleVectors(angles, matrix[0], matrix[1], matrix[2]);
	VectorInverse(matrix[1]);
}




