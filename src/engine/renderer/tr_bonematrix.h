/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code. If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef TR_BONEMATRIX_H
#define TR_BONEMATRIX_H
#include "../qcommon/q_shared.h"

typedef float boneMatrix_t[ 12 ];

// bone matrix functions
// bone matrices are row major with 3 rows and 4 columns

void BoneMatrixInvert( boneMatrix_t m );

static INLINE void BoneMatrixIdentity( boneMatrix_t m )
{
	m[ 0 ] = 1;
	m[ 1 ] = 0;
	m[ 2 ] = 0;
	m[ 3 ] = 0;
	m[ 4 ] = 0;
	m[ 5 ] = 1;
	m[ 6 ] = 0;
	m[ 7 ] = 0;
	m[ 8 ] = 0;
	m[ 9 ] = 0;
	m[ 10 ] = 1;
	m[ 11 ] = 0;
}

static INLINE void BoneMatrixSetupTransform( boneMatrix_t m, const quat_t rot, const vec3_t origin )
{
	float xx = 2.0f * rot[ 0 ] * rot[ 0 ];
	float yy = 2.0f * rot[ 1 ] * rot[ 1 ];
	float zz = 2.0f * rot[ 2 ] * rot[ 2 ];
	float xy = 2.0f * rot[ 0 ] * rot[ 1 ];
	float xz = 2.0f * rot[ 0 ] * rot[ 2 ];
	float yz = 2.0f * rot[ 1 ] * rot[ 2 ];
	float wx = 2.0f * rot[ 3 ] * rot[ 0 ];
	float wy = 2.0f * rot[ 3 ] * rot[ 1 ];
	float wz = 2.0f * rot[ 3 ] * rot[ 2 ];
	
	m[ 0 ] = ( 1.0f - ( yy + zz ) );
	m[ 1 ] = ( xy - wz );
	m[ 2 ] = ( xz + wy );
	m[ 3 ] = origin[ 0 ];
	m[ 4 ] = ( xy + wz );
	m[ 5 ] = ( 1.0f - ( xx + zz ) );
	m[ 6 ] = ( yz - wx );
	m[ 7 ] = origin[ 1 ];
	m[ 8 ] = ( xz - wy );
	m[ 9 ] = ( yz + wx );
	m[ 10 ] = ( 1.0f - ( xx + yy ) );
	m[ 11 ] = origin[ 2 ];
}

static INLINE void BoneMatrixSetupTransformWithScale( boneMatrix_t m, const quat_t rot, const vec3_t origin, const vec3_t scale )
{
	float xx = 2.0f * rot[ 0 ] * rot[ 0 ];
	float yy = 2.0f * rot[ 1 ] * rot[ 1 ];
	float zz = 2.0f * rot[ 2 ] * rot[ 2 ];
	float xy = 2.0f * rot[ 0 ] * rot[ 1 ];
	float xz = 2.0f * rot[ 0 ] * rot[ 2 ];
	float yz = 2.0f * rot[ 1 ] * rot[ 2 ];
	float wx = 2.0f * rot[ 3 ] * rot[ 0 ];
	float wy = 2.0f * rot[ 3 ] * rot[ 1 ];
	float wz = 2.0f * rot[ 3 ] * rot[ 2 ];

	m[ 0 ] = scale[ 0 ] * ( 1.0f - ( yy + zz ) );
	m[ 1 ] = scale[ 0 ] * ( xy - wz );
	m[ 2 ] = scale[ 0 ] * ( xz + wy );
	m[ 3 ] = origin[ 0 ];
	m[ 4 ] = scale[ 1 ] * ( xy + wz );
	m[ 5 ] = scale[ 1 ] * ( 1.0f - ( xx + zz ) );
	m[ 6 ] = scale[ 1 ] * ( yz - wx );
	m[ 7 ] = origin[ 1 ];
	m[ 8 ] = scale[ 2 ] * ( xz - wy );
	m[ 9 ] = scale[ 2 ] * ( yz + wx );
	m[ 10 ] = scale[ 2 ] * ( 1.0f - ( xx + yy ) );
	m[ 11 ] = origin[ 2 ];
}

// multiplied as if the matrices are 4x4 with the last row = ( 0, 0, 0, 1 )
static INLINE void BoneMatrixMultiply( const boneMatrix_t a, const boneMatrix_t b, boneMatrix_t out )
{
	out[ 0 ] = b[ 0 ] * a[ 0 ] + b[ 4 ] * a[ 1 ] + b[ 8 ] * a[ 2 ];
	out[ 1 ] = b[ 1 ] * a[ 0 ] + b[ 5 ] * a[ 1 ] + b[ 9 ] * a[ 2 ];
	out[ 2 ] = b[ 2 ] * a[ 0 ] + b[ 6 ] * a[ 1 ] + b[ 10 ] * a[ 2 ];
	out[ 3 ] = b[ 3 ] * a[ 0 ] + b[ 7 ] * a[ 1 ] + b[ 11 ] * a[ 2 ] + a[ 3 ];
	
	out[ 4 ] = b[ 0 ] * a[ 4 ] + b[ 4 ] * a[ 5 ] + b[ 8 ] * a[ 6 ];
	out[ 5 ] = b[ 1 ] * a[ 4 ] + b[ 5 ] * a[ 5 ] + b[ 9 ] * a[ 6 ];
	out[ 6 ] = b[ 2 ] * a[ 4 ] + b[ 6 ] * a[ 5 ] + b[ 10 ] * a[ 6 ];
	out[ 7 ] = b[ 3 ] * a[ 4 ] + b[ 7 ] * a[ 5 ] + b[ 11 ] * a[ 6 ] + a[ 7 ];
	
	out[ 8 ] = b[ 0 ] * a[ 8 ] + b[ 4 ] * a[ 9 ] + b[ 8 ] * a[ 10 ];
	out[ 9 ] = b[ 1 ] * a[ 8 ] + b[ 5 ] * a[ 9 ] + b[ 9 ] * a[ 10 ];
	out[ 10 ] = b[ 2 ] * a[ 8 ] + b[ 6 ] * a[ 9 ] + b[ 10 ] * a[ 10 ];
	out[ 11 ] = b[ 3 ] * a[ 8 ] + b[ 7 ] * a[ 9 ] + b[ 11 ] * a[ 10 ] + a[ 11 ];
}

static INLINE void BoneMatrixMad( boneMatrix_t a, float s, const boneMatrix_t b )
{
	a[ 0 ] += b[ 0 ] * s;
	a[ 1 ] += b[ 1 ] * s;
	a[ 2 ] += b[ 2 ] * s;
	a[ 3 ] += b[ 3 ] * s;
	
	a[ 4 ] += b[ 4 ] * s;
	a[ 5 ] += b[ 5 ] * s;
	a[ 6 ] += b[ 6 ] * s;
	a[ 7 ] += b[ 7 ] * s;
	
	a[ 8 ] += b[ 8 ] * s;
	a[ 9 ] += b[ 9 ] * s;
	a[ 10 ] += b[ 10 ] * s;
	a[ 11 ] += b[ 11 ] * s;
}

 static INLINE void BoneMatrixMul( boneMatrix_t a, float s, const boneMatrix_t b )
{
	a[ 0 ] = b[ 0 ] * s;
	a[ 1 ] = b[ 1 ] * s;
	a[ 2 ] = b[ 2 ] * s;
	a[ 3 ] = b[ 3 ] * s;
	
	a[ 4 ] = b[ 4 ] * s;
	a[ 5 ] = b[ 5 ] * s;
	a[ 6 ] = b[ 6 ] * s;
	a[ 7 ] = b[ 7 ] * s;
	
	a[ 8 ] = b[ 8 ] * s;
	a[ 9 ] = b[ 9 ] * s;
	a[ 10 ] = b[ 10 ] * s;
	a[ 11 ] = b[ 11 ] * s;
}

static INLINE void BoneMatrixTransformPoint( const boneMatrix_t m, const vec3_t p, vec3_t out )
{
	out[ 0 ] = m[ 0 ] * p[ 0 ] + m[ 1 ] * p[ 1 ] + m[ 2 ] * p[ 2 ] + m[ 3 ]; 
	out[ 1 ] = m[ 4 ] * p[ 0 ] + m[ 5 ] * p[ 1 ] + m[ 6 ] * p[ 2 ] + m[ 7 ];
	out[ 2 ] = m[ 8 ] * p[ 0 ] + m[ 9 ] * p[ 1 ] + m[ 10 ] * p[ 2 ] + m[ 11 ];
}

static INLINE void BoneMatrixAdjointTranspose( boneMatrix_t m )
{
	float a0 = m[ 0 ];
	float a1 = m[ 1 ];
	float a2 = m[ 2 ];

	float b0 = m[ 4 ];
	float b1 = m[ 5 ];
	float b2 = m[ 6 ];

	float c0 = m[ 8 ];
	float c1 = m[ 9 ];
	float c2 = m[ 10 ];

	// the 3x3 adjoint matrix is computed using 3 cross products
	m[ 0 ] = b1 * c2 - b2 * c1;
	m[ 1 ] = b2 * c0 - b0 * c2;
	m[ 2 ] = b0 * c1 - b1 * c0;
	m[ 3 ] = 0;

	m[ 4 ] = c1 * a2 - c2 * a1;
	m[ 5 ] = c2 * a0 - c0 * a2;
	m[ 6 ] = c0 * a1 - c1 * a0;
	m[ 7 ] = 0;

	m[ 8 ] = a1 * b2 - a2 * b1;
	m[ 9 ] = a2 * b0 - a0 * b2;
	m[ 10 ] = a0 * b1 - a1 * b0;
	m[ 11 ] = 0;
}

static INLINE void BoneMatrixTransformNormal( const boneMatrix_t m, const vec3_t p, vec3_t out )
{
	out[ 0 ] = m[ 0 ] * p[ 0 ] + m[ 1 ] * p[ 1 ] + m[ 2 ] * p[ 2 ];
	out[ 1 ] = m[ 4 ] * p[ 0 ] + m[ 5 ] * p[ 1 ] + m[ 6 ] * p[ 2 ];
	out[ 2 ] = m[ 8 ] * p[ 0 ] + m[ 9 ] * p[ 1 ] + m[ 10 ] * p[ 2 ];
}

#if id386_sse
#define assert_16_byte_aligned( x ) assert( ( (intptr_t)x & 15 ) == 0 )

static INLINE void BoneMatrixMulSSE( __m128 *oa, __m128 *ob, __m128 *oc, float s, const boneMatrix_t m )
{
	__m128 a, b, c, w;
	assert_16_byte_aligned( m );
	
	a = _mm_load_ps( m );
	b = _mm_load_ps( m + 4 );
	c = _mm_load_ps( m + 8 );
	w = _mm_load1_ps( &s );

	*oa = _mm_mul_ps( a, w );
	*ob = _mm_mul_ps( b, w );
	*oc = _mm_mul_ps( c, w );
}

static INLINE void BoneMatrixMadSSE(  __m128 *oa, __m128 *ob, __m128 *oc, float s, const boneMatrix_t m )
{
	__m128 a, b, c, w;
	assert_16_byte_aligned( m );

	a = _mm_load_ps( m );
	b = _mm_load_ps( m + 4 );
	c = _mm_load_ps( m + 8 );
	w = _mm_load1_ps( &s );

	*oa = _mm_add_ps( *oa, _mm_mul_ps( a, w ) );
	*ob = _mm_add_ps( *ob, _mm_mul_ps( b, w ) );
	*oc = _mm_add_ps( *oc, _mm_mul_ps( c, w ) );
}

// expects in[ 3 ] == 1
// outputs ( x, y, z, ? )
static INLINE void BoneMatrixTransformPointSSE( __m128 a, __m128 b, __m128 c, const vec4_t in, vec4_t out )
{
	__m128 p, x, y, z, s1, s2;
	assert_16_byte_aligned( in );
	assert_16_byte_aligned( out );

	p = _mm_load_ps( in );

	x = _mm_mul_ps( a, p );
	y = _mm_mul_ps( b, p );
	z = _mm_mul_ps( c, p );

	s1 = _mm_add_ps( _mm_unpacklo_ps( x, z ), _mm_unpackhi_ps( x, z ) );
	s2 = _mm_add_ps( _mm_unpacklo_ps( y, z ), _mm_unpackhi_ps( y, z ) );
	_mm_store_ps( out, _mm_add_ps( _mm_unpacklo_ps( s1, s2 ), _mm_unpackhi_ps( s1, s2 ) ) );
}

static INLINE void BoneMatrixAdjointTransposeSSE( __m128 *a, __m128 *b, __m128 *c )
{
	__m128 shufa = _mm_shuffle_ps( *a, *a, _MM_SHUFFLE( 3, 0, 2, 1 ) );
	__m128 shufa2 = _mm_shuffle_ps( *a, *a, _MM_SHUFFLE( 3, 1, 0, 2 ) );
	__m128 shufb = _mm_shuffle_ps( *b, *b, _MM_SHUFFLE( 3, 0, 2, 1 ) );
	__m128 shufb2 = _mm_shuffle_ps( *b, *b, _MM_SHUFFLE( 3, 1, 0, 2 ) );
	__m128 shufc = _mm_shuffle_ps( *c, *c, _MM_SHUFFLE( 3, 0, 2, 1 ) );
	__m128 shufc2 = _mm_shuffle_ps( *c, *c, _MM_SHUFFLE( 3, 1, 0, 2 ) );

	// b x c
	*a = _mm_sub_ps( _mm_mul_ps( shufb, shufc2 ), _mm_mul_ps( shufb2, shufc ) );

	// c x a
	*b = _mm_sub_ps( _mm_mul_ps( shufc, shufa2 ), _mm_mul_ps( shufc2, shufa ) );

	// a x b
	*c = _mm_sub_ps( _mm_mul_ps( shufa, shufb2 ), _mm_mul_ps( shufa2, shufb ) );
}

// outputs ( x, y, z, ? )
// does not normalize output
static INLINE void BoneMatrixTransformNormalSSE( __m128 a, __m128 b, __m128 c, const vec4_t in, vec4_t out )
{
	__m128 p, x, y, z;
	__m128 xy01, xy23;
	__m128 xyz0, xyz1, xyz2;
	assert_16_byte_aligned( in );
	assert_16_byte_aligned( out );

	p = _mm_load_ps( in );

	x = _mm_mul_ps( a, p );
	y = _mm_mul_ps( b, p );
	z = _mm_mul_ps( c, p );

	xy01 = _mm_unpacklo_ps( x, y );
	xyz0 = _mm_shuffle_ps( xy01, z, _MM_SHUFFLE( 3, 0, 1, 0 ) );
	xyz1 = _mm_shuffle_ps( xy01, z, _MM_SHUFFLE( 3, 1, 3, 2 ) );

	xy23 = _mm_unpackhi_ps( x, y );
	xyz2 = _mm_shuffle_ps( xy23, z, _MM_SHUFFLE( 3, 2, 1, 0 ) );

	_mm_store_ps( out, _mm_add_ps( _mm_add_ps( xyz0, xyz1 ), xyz2 ) );
}

// outputs ( x, y, z, ? )
// normalizes output
static INLINE void BoneMatrixTransformNormalNSSE( __m128 a, __m128 b, __m128 c, const vec4_t in, vec4_t out )
{
	__m128 p, x, y, z;
	__m128 xy01, xy23;
	__m128 xyz0, xyz1, xyz2;
	__m128 xyz;
	__m128 xyzsq;
	__m128 dotp;
	__m128 ilen;
	assert_16_byte_aligned( in );
	assert_16_byte_aligned( out );

	p = _mm_load_ps( in );

	x = _mm_mul_ps( a, p );
	y = _mm_mul_ps( b, p );
	z = _mm_mul_ps( c, p );

	xy01 = _mm_unpacklo_ps( x, y );
	xyz0 = _mm_shuffle_ps( xy01, z, _MM_SHUFFLE( 3, 0, 1, 0 ) );
	xyz1 = _mm_shuffle_ps( xy01, z, _MM_SHUFFLE( 3, 1, 3, 2 ) );

	xy23 = _mm_unpackhi_ps( x, y );
	xyz2 = _mm_shuffle_ps( xy23, z, _MM_SHUFFLE( 3, 2, 1, 0 ) );

	xyz = _mm_add_ps( _mm_add_ps( xyz0, xyz1 ), xyz2 );

	// normalize result
	xyzsq = _mm_mul_ps( xyz, xyz );

	dotp = _mm_movehl_ps( xyz2, xyzsq );
	dotp = _mm_add_ss( dotp, xyzsq );
	dotp = _mm_add_ss( dotp, _mm_shuffle_ps( xyzsq, xyzsq, _MM_SHUFFLE( 1, 1, 1, 1 ) ) );
	
	ilen = _mm_rsqrt_ss( dotp );
	ilen = _mm_shuffle_ps( ilen, ilen, _MM_SHUFFLE( 0, 0, 0, 0 ) );

	_mm_store_ps( out, _mm_mul_ps( xyz, ilen ) );
}
#endif
#endif
