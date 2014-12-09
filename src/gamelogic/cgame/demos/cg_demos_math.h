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

#ifndef	__CG_DEMOS_MATH_H
#define	__CG_DEMOS_MATH_H

typedef enum {
	posLinear,
	posCatmullRom,
	posBezier,
	posLast,
} posInterpolate_t;

typedef enum {
	angleLinear,
	angleQuat,
	angleLast,
} angleInterpolate_t;

typedef float QuatMME_t[4];
void QuatFromAnglesMME( const vec3_t angles, QuatMME_t dst );
void QuatFromAnglesClosestMME( const vec3_t angles, const QuatMME_t qc, QuatMME_t dst);
void QuatFromAxisAngleMME( const vec3_t axis, vec_t angle, QuatMME_t dst);
void QuatToAnglesMME( const QuatMME_t q, vec3_t angles);
void QuatToAxisMME( const QuatMME_t q, vec3_t axis[3]);
void QuatToAxisAngleMME (const QuatMME_t q, vec3_t axis, float *angle);
void QuatMultiplyMME( const QuatMME_t q1, const QuatMME_t q2, QuatMME_t qr);
void QuatSlerpMME(float t,const QuatMME_t q0,const QuatMME_t q1, QuatMME_t qr);
void QuatSquadMME( float t, const QuatMME_t q0, const QuatMME_t q1, const QuatMME_t q2, const QuatMME_t q3, QuatMME_t qr);
void QuatTimeSplineMME( float t, const int times[4], const QuatMME_t quats[4], QuatMME_t qr);
void QuatLogMME( const QuatMME_t q0, QuatMME_t qr );
void QuatExpMME( const QuatMME_t q0, QuatMME_t qr );

void VectorTimeSpline( float t, const int times[4], float *control, float *result, int dim);

#define Vector4Set(v, x, y, z, n)	((v)[0]=(x),(v)[1]=(y),(v)[2]=(z),(v)[3]=(n))
#define Vector4Copy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])
#define	Vector4Scale(v, s, o)		((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s),(o)[3]=(v)[3]*(s))
#define	Vector4MA(v, s, b, o)		((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s),(o)[3]=(v)[3]+(b)[3]*(s))
#define	Vector4Average(v, b, s, o)	((o)[0]=((v)[0]*(1-(s)))+((b)[0]*(s)),(o)[1]=((v)[1]*(1-(s)))+((b)[1]*(s)),(o)[2]=((v)[2]*(1-(s)))+((b)[2]*(s)),(o)[3]=((v)[3]*(1-(s)))+((b)[3]*(s)))
#define Vector4Subtract(a,b,c)		((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2],(c)[3]=(a)[3]-(b)[3])
#define Vector4Add(a,b,c)			((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2],(c)[3]=(a)[3]+(b)[3])

static inline void AngleForward( const vec3_t angles, vec3_t forward ) {
	float		angle;
	float		sp, sy, cp, cy;

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);

	forward[0] = cp*cy;
	forward[1] = cp*sy;
	forward[2] = -sp;
}	

static inline float QuatDotMME( const QuatMME_t q0, const QuatMME_t q1) {
	return q0[0]*q1[0] + q0[1]*q1[1] + q0[2]*q1[2] + q0[3]*q1[3];
}
static inline void QuatCopyMME( const QuatMME_t src, QuatMME_t dst) {
	dst[0] = src[0];dst[1] = src[1];dst[2] = src[2];dst[3] = src[3];
}
static inline void QuatNegateMME( const QuatMME_t src, QuatMME_t dst) {
	dst[0] = -src[0];dst[1] = -src[1];dst[2] = -src[2];dst[3] = -src[3];
}
static inline void QuatConjugateMME( const QuatMME_t src, QuatMME_t dst) {
	dst[0] = -src[0];dst[1] = -src[1];dst[2] = -src[2];dst[3] = src[3];
}
static inline void QuatAddMME( const QuatMME_t q0, const QuatMME_t q1, QuatMME_t qr) {
	qr[0] = q0[0] + q1[0];
	qr[1] = q0[1] + q1[1];
	qr[2] = q0[2] + q1[2];
	qr[3] = q0[3] + q1[3];
}
static inline void QuatSubMME( const QuatMME_t q0, const QuatMME_t q1, QuatMME_t qr) {
	qr[0] = q0[0] - q1[0];
	qr[1] = q0[1] - q1[1];
	qr[2] = q0[2] - q1[2];
	qr[3] = q0[3] - q1[3];
}
static inline void QuatScaleMME( const QuatMME_t q0, float scale,QuatMME_t qr) {
	qr[0] = scale * q0[0];
	qr[1] = scale * q0[1];
	qr[2] = scale * q0[2];
	qr[3] = scale * q0[3];
}
static inline void QuatScaleAddMME( const QuatMME_t q0, const QuatMME_t q1, float scale, QuatMME_t qr) {
	qr[0] = q0[0] + scale * q1[0];
	qr[1] = q0[1] + scale * q1[1];
	qr[2] = q0[2] + scale * q1[2];
	qr[3] = q0[3] + scale * q1[3];
}
static inline float QuatLengthSquaredMME( const QuatMME_t q ) {
	return q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
}
static inline float QuatLengthMME( const QuatMME_t q ) {
	return sqrt( q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
}
static inline void QuatClearMME( QuatMME_t q ) {
	q[0] = q[1] = q[2] = q[3] = 0;
}

static inline float QuatNormalizeMME( QuatMME_t q ) {
	float ilen, len = QuatLengthMME( q );
	if (len) {
		ilen = 1 / len;
		q[0] *= ilen;
		q[1] *= ilen;
		q[2] *= ilen;
		q[3] *= ilen;
	} else {
		QuatClearMME( q );
	}
	return len;
}

static inline void posMatrix( float t, posInterpolate_t type, vec4_t matrix) {
	switch ( type ) {
	case posLinear:
		matrix[0] = 0;
		matrix[1] = 1-t;
		matrix[2] = t;
		matrix[3] = 0;
		break;
	case posCatmullRom:
		matrix[0] = 0.5f * ( -t + 2*t*t - t*t*t );
		matrix[1] = 0.5f * ( 2 - 5*t*t + 3*t*t*t );
		matrix[2] = 0.5f * ( t + 4*t*t - 3*t*t*t );
		matrix[3] = 0.5f * (-t*t + t*t*t);
		break;
	default:
	case posBezier:
		matrix[0] = (((-t+3)*t-3)*t+1)/6;
		matrix[1] = (((3*t-6)*t)*t+4)/6;
		matrix[2] = (((-3*t+3)*t+3)*t+1)/6;
		matrix[3] = (t*t*t)/6;
		break;
	}
}

static inline void posGet( float t, posInterpolate_t type, const vec3_t control[4], vec3_t out) {
	vec4_t matrix;

	posMatrix( t, type, matrix );
	VectorScale( control[0], matrix[0], out );
	VectorMA( out, matrix[1], control[1], out );
	VectorMA( out, matrix[2], control[2], out );
	VectorMA( out, matrix[3], control[3], out );
}

static inline vec_t VectorDistanceSquared ( const vec3_t v1, const vec3_t v2 ) {
	vec3_t v;
	VectorSubtract( v1, v2, v );
	return (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}
static inline vec_t VectorDistance( const vec3_t v1, const vec3_t v2 ) {
	vec3_t v;
	VectorSubtract( v1, v2, v );
	return (vec_t)sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}
static inline void AnglesNormalize360( vec3_t angles) {
	angles[0] = AngleNormalize360(angles[0]);
	angles[1] = AngleNormalize360(angles[1]);
	angles[2] = AngleNormalize360(angles[2]);
}
static inline void AnglesNormalize180( vec3_t angles) {
	angles[0] = AngleNormalize180(angles[0]);
	angles[1] = AngleNormalize180(angles[1]);
	angles[2] = AngleNormalize180(angles[2]);
}

#define VectorAddDelta(a,b,c)	((c)[0]=(b)[0]+((b)[0]-(a)[0]),(c)[1]=(b)[1]+((b)[1]-(a)[1]),(c)[2]=(b)[2]+((b)[2]-(a)[2]))
#define VectorSubDelta(a,b,c)	((c)[0]=(a)[0]-((b)[0]-(a)[0]),(c)[1]=(a)[1]-((b)[1]-(a)[1]),(c)[2]=(a)[2]-((b)[2]-(a)[2]))

qboolean CylinderTraceImpact( const vec3_t start, const vec3_t forward, const vec3_t container, vec3_t result );
qboolean BoxTraceImpact(const vec3_t start, const vec3_t forward, const vec3_t container, vec3_t result );

//void GetPerpendicularViewVector( const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up );

float dsplineCalc(float x, vec3_t dx, vec3_t dy, float*deriv );

void demoDrawCross( const vec3_t origin, const vec4_t color );
void demoDrawBox( const vec3_t origin, const vec3_t container, const vec4_t color );
void demoDrawLine( const vec3_t p0, const vec3_t p1, const vec4_t color);
void demoDrawRawLine(const vec3_t start, const vec3_t end, float width, polyVert_t *verts );
void demoDrawSetupVerts( polyVert_t *verts, const vec4_t color );
void demoDrawGrid( const vec3_t origin, const vec4_t color, const vec3_t offset, float width, float step, float range, qhandle_t shader );

void demoRotatePoint(vec3_t point, const vec3_t matrix[3]);
void demoTransposeMatrix(const vec3_t matrix[3], vec3_t transpose[3]);
void demoCreateRotationMatrix(const vec3_t angles, vec3_t matrix[3]);
void demoJitterCreate( float *jitarr, int num );
#endif
