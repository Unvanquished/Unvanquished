/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_main.c -- main control flow for each frame
#include "tr_local.h"

trGlobals_t tr;

// convert from our coordinate system (looking down X)
// to OpenGL's coordinate system (looking down -Z)
const matrix_t quakeToOpenGLMatrix =
{
	0,  0, -1, 0,
	-1, 0, 0,  0,
	0,  1, 0,  0,
	0,  0, 0,  1
};

// inverse of quakeToOpenGL matrix
const matrix_t openGLToQuakeMatrix =
{
	0,  -1, 0, 0,
	0,  0,  1, 0,
	-1, 0,  0, 0,
	0,  0,  0, 1
};

// convert from our right handed coordinate system (looking down X)
// to D3D's left handed coordinate system (looking down Z)
const matrix_t quakeToD3DMatrix =
{
	0,  0, 1, 0,
	-1, 0, 0, 0,
	0,  1, 0, 0,
	0,  0, 0, 1
};

const matrix_t flipZMatrix =
{
	1, 0, 0,  0,
	0, 1, 0,  0,
	0, 0, -1, 0,
	0, 0, 0,  1
};

const GLenum   geometricRenderTargets[] =
{
	GL_COLOR_ATTACHMENT0_EXT,
	GL_COLOR_ATTACHMENT1_EXT,
	GL_COLOR_ATTACHMENT2_EXT,
	GL_COLOR_ATTACHMENT3_EXT
};

int            shadowMapResolutions[ 5 ] = { 2048, 1024, 512, 256, 128 };
int            sunShadowMapResolutions[ 5 ] = { 2048, 2048, 1024, 1024, 1024 };

refimport_t    ri;

// entities that will have procedurally generated surfaces will just
// point at this for their sorting surface
surfaceType_t entitySurface = SF_ENTITY;

/*
================
R_CompareVert
================
*/
qboolean R_CompareVert( srfVert_t *v1, srfVert_t *v2, qboolean checkST )
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		if ( floor( v1->xyz[ i ] + 0.1 ) != floor( v2->xyz[ i ] + 0.1 ) )
		{
			return qfalse;
		}

		if ( checkST && ( ( v1->st[ 0 ] != v2->st[ 0 ] ) || ( v1->st[ 1 ] != v2->st[ 1 ] ) ) )
		{
			return qfalse;
		}
	}

	return qtrue;
}

/*
=============
R_CalcNormalForTriangle
=============
*/
void R_CalcNormalForTriangle( vec3_t normal, const vec3_t v0, const vec3_t v1, const vec3_t v2 )
{
	vec3_t udir, vdir;

	// compute the face normal based on vertex points
	VectorSubtract( v2, v0, udir );
	VectorSubtract( v1, v0, vdir );
	CrossProduct( udir, vdir, normal );

	VectorNormalize( normal );
}

/*
=============
R_CalcTangentsForTriangle
http://members.rogers.com/deseric/tangentspace.htm
=============
*/
void R_CalcTangentsForTriangle( vec3_t tangent, vec3_t binormal,
                                const vec3_t v0, const vec3_t v1, const vec3_t v2,
                                const vec2_t t0, const vec2_t t1, const vec2_t t2 )
{
	int    i;
	vec3_t planes[ 3 ];
	vec3_t u, v;

	for ( i = 0; i < 3; i++ )
	{
		VectorSet( u, v1[ i ] - v0[ i ], t1[ 0 ] - t0[ 0 ], t1[ 1 ] - t0[ 1 ] );
		VectorSet( v, v2[ i ] - v0[ i ], t2[ 0 ] - t0[ 0 ], t2[ 1 ] - t0[ 1 ] );

		VectorNormalize( u );
		VectorNormalize( v );

		CrossProduct( u, v, planes[ i ] );
	}

	//So your tangent space will be defined by this :
	//Normal = Normal of the triangle or Tangent X Binormal (careful with the cross product,
	// you have to make sure the normal points in the right direction)
	//Tangent = ( dp(Fx(s,t)) / ds,  dp(Fy(s,t)) / ds, dp(Fz(s,t)) / ds )   or     ( -Bx/Ax, -By/Ay, - Bz/Az )
	//Binormal =  ( dp(Fx(s,t)) / dt,  dp(Fy(s,t)) / dt, dp(Fz(s,t)) / dt )  or     ( -Cx/Ax, -Cy/Ay, -Cz/Az )

	// tangent...
	tangent[ 0 ] = -planes[ 0 ][ 1 ] / planes[ 0 ][ 0 ];
	tangent[ 1 ] = -planes[ 1 ][ 1 ] / planes[ 1 ][ 0 ];
	tangent[ 2 ] = -planes[ 2 ][ 1 ] / planes[ 2 ][ 0 ];
	VectorNormalize( tangent );

	// binormal...
	binormal[ 0 ] = -planes[ 0 ][ 2 ] / planes[ 0 ][ 0 ];
	binormal[ 1 ] = -planes[ 1 ][ 2 ] / planes[ 1 ][ 0 ];
	binormal[ 2 ] = -planes[ 2 ][ 2 ] / planes[ 2 ][ 0 ];
	VectorNormalize( binormal );
}

/*
=============
R_CalcTangentSpace
=============
*/
void R_CalcTangentSpace( vec3_t tangent, vec3_t binormal, vec3_t normal,
                         const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec2_t t0, const vec2_t t1, const vec2_t t2 )
{
	vec3_t cp, u, v;
	vec3_t faceNormal;

	VectorSet( u, v1[ 0 ] - v0[ 0 ], t1[ 0 ] - t0[ 0 ], t1[ 1 ] - t0[ 1 ] );
	VectorSet( v, v2[ 0 ] - v0[ 0 ], t2[ 0 ] - t0[ 0 ], t2[ 1 ] - t0[ 1 ] );

	CrossProduct( u, v, cp );

	if ( fabs( cp[ 0 ] ) > 10e-6 )
	{
		tangent[ 0 ] = -cp[ 1 ] / cp[ 0 ];
		binormal[ 0 ] = -cp[ 2 ] / cp[ 0 ];
	}

	u[ 0 ] = v1[ 1 ] - v0[ 1 ];
	v[ 0 ] = v2[ 1 ] - v0[ 1 ];

	CrossProduct( u, v, cp );

	if ( fabs( cp[ 0 ] ) > 10e-6 )
	{
		tangent[ 1 ] = -cp[ 1 ] / cp[ 0 ];
		binormal[ 1 ] = -cp[ 2 ] / cp[ 0 ];
	}

	u[ 0 ] = v1[ 2 ] - v0[ 2 ];
	v[ 0 ] = v2[ 2 ] - v0[ 2 ];

	CrossProduct( u, v, cp );

	if ( fabs( cp[ 0 ] ) > 10e-6 )
	{
		tangent[ 2 ] = -cp[ 1 ] / cp[ 0 ];
		binormal[ 2 ] = -cp[ 2 ] / cp[ 0 ];
	}

	VectorNormalize( tangent );
	VectorNormalize( binormal );

	// compute the face normal based on vertex points
	VectorSubtract( v2, v0, u );
	VectorSubtract( v1, v0, v );
	CrossProduct( u, v, faceNormal );

	VectorNormalize( faceNormal );

#if 1
	// Gram-Schmidt orthogonalize
	//tangent[a] = (t - n * Dot(n, t)).Normalize();
	VectorMA( tangent, -DotProduct( faceNormal, tangent ), faceNormal, tangent );
	VectorNormalize( tangent );

	// compute the cross product B=NxT
	//CrossProduct(normal, tangent, binormal);
#else
	// normal, compute the cross product N=TxB
	CrossProduct( tangent, binormal, normal );
	VectorNormalize( normal );

	if ( DotProduct( normal, faceNormal ) < 0 )
	{
		//VectorInverse(normal);
		//VectorInverse(tangent);
		//VectorInverse(binormal);

		// compute the cross product T=BxN
		CrossProduct( binormal, faceNormal, tangent );

		// compute the cross product B=NxT
		//CrossProduct(normal, tangent, binormal);
	}

#endif

	VectorCopy( faceNormal, normal );
}

void R_CalcTangentSpaceFast( vec3_t tangent, vec3_t binormal, vec3_t normal,
                             const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec2_t t0, const vec2_t t1, const vec2_t t2 )
{
	vec3_t cp, u, v;
	vec3_t faceNormal;

	VectorSet( u, v1[ 0 ] - v0[ 0 ], t1[ 0 ] - t0[ 0 ], t1[ 1 ] - t0[ 1 ] );
	VectorSet( v, v2[ 0 ] - v0[ 0 ], t2[ 0 ] - t0[ 0 ], t2[ 1 ] - t0[ 1 ] );

	CrossProduct( u, v, cp );

	if ( fabs( cp[ 0 ] ) > 10e-6 )
	{
		tangent[ 0 ] = -cp[ 1 ] / cp[ 0 ];
		binormal[ 0 ] = -cp[ 2 ] / cp[ 0 ];
	}

	u[ 0 ] = v1[ 1 ] - v0[ 1 ];
	v[ 0 ] = v2[ 1 ] - v0[ 1 ];

	CrossProduct( u, v, cp );

	if ( fabs( cp[ 0 ] ) > 10e-6 )
	{
		tangent[ 1 ] = -cp[ 1 ] / cp[ 0 ];
		binormal[ 1 ] = -cp[ 2 ] / cp[ 0 ];
	}

	u[ 0 ] = v1[ 2 ] - v0[ 2 ];
	v[ 0 ] = v2[ 2 ] - v0[ 2 ];

	CrossProduct( u, v, cp );

	if ( fabs( cp[ 0 ] ) > 10e-6 )
	{
		tangent[ 2 ] = -cp[ 1 ] / cp[ 0 ];
		binormal[ 2 ] = -cp[ 2 ] / cp[ 0 ];
	}

	VectorNormalizeFast( tangent );
	VectorNormalizeFast( binormal );

	// compute the face normal based on vertex points
	VectorSubtract( v2, v0, u );
	VectorSubtract( v1, v0, v );
	CrossProduct( u, v, faceNormal );

	VectorNormalizeFast( faceNormal );

#if 0
	// normal, compute the cross product N=TxB
	CrossProduct( tangent, binormal, normal );
	VectorNormalizeFast( normal );

	if ( DotProduct( normal, faceNormal ) < 0 )
	{
		VectorInverse( normal );
		//VectorInverse(tangent);
		//VectorInverse(binormal);

		CrossProduct( normal, tangent, binormal );
	}

	VectorCopy( faceNormal, normal );
#else
	// Gram-Schmidt orthogonalize
	//tangent[a] = (t - n * Dot(n, t)).Normalize();
	VectorMA( tangent, -DotProduct( faceNormal, tangent ), faceNormal, tangent );
	VectorNormalizeFast( tangent );
#endif

	VectorCopy( faceNormal, normal );
}

/*
http://www.terathon.com/code/tangent.html
*/
void R_CalcTBN( vec3_t tangent, vec3_t bitangent, vec3_t normal,
                const vec3_t v1, const vec3_t v2, const vec3_t v3, const vec2_t w1, const vec2_t w2, const vec2_t w3 )
{
	vec3_t u, v;
	float  x1, x2, y1, y2, z1, z2;
	float  s1, s2, t1, t2;
	float  r, dot;

	x1 = v2[ 0 ] - v1[ 0 ];
	x2 = v3[ 0 ] - v1[ 0 ];
	y1 = v2[ 1 ] - v1[ 1 ];
	y2 = v3[ 1 ] - v1[ 1 ];
	z1 = v2[ 2 ] - v1[ 2 ];
	z2 = v3[ 2 ] - v1[ 2 ];

	s1 = w2[ 0 ] - w1[ 0 ];
	s2 = w3[ 0 ] - w1[ 0 ];
	t1 = w2[ 1 ] - w1[ 1 ];
	t2 = w3[ 1 ] - w1[ 1 ];

	r = 1.0f / ( s1 * t2 - s2 * t1 );

	VectorSet( tangent, ( t2 * x1 - t1 * x2 ) * r, ( t2 * y1 - t1 * y2 ) * r, ( t2 * z1 - t1 * z2 ) * r );
	VectorSet( bitangent, ( s1 * x2 - s2 * x1 ) * r, ( s1 * y2 - s2 * y1 ) * r, ( s1 * z2 - s2 * z1 ) * r );

	// compute the face normal based on vertex points
	VectorSubtract( v3, v1, u );
	VectorSubtract( v2, v1, v );
	CrossProduct( u, v, normal );

	VectorNormalize( normal );

	// Gram-Schmidt orthogonalize
	//tangent[a] = (t - n * Dot(n, t)).Normalize();
	dot = DotProduct( normal, tangent );
	VectorMA( tangent, -dot, normal, tangent );
	VectorNormalize( tangent );

	// B=NxT
	//CrossProduct(normal, tangent, bitangent);
}

void R_CalcTBN2( vec3_t tangent, vec3_t binormal, vec3_t normal,
                 const vec3_t v1, const vec3_t v2, const vec3_t v3, const vec2_t t1, const vec2_t t2, const vec2_t t3 )
{
	vec3_t v2v1;
	vec3_t v3v1;

	float  c2c1_T;
	float  c2c1_B;

	float  c3c1_T;
	float  c3c1_B;

	float  denominator;
	float  scale1, scale2;

	vec3_t T, B, N, C;

	// Calculate the tangent basis for each vertex of the triangle
	// UPDATE: In the 3rd edition of the accompanying article, the for-loop located here has
	// been removed as it was redundant (the entire TBN matrix was calculated three times
	// instead of just one).
	//
	// Please note, that this function relies on the fact that the input geometry are triangles
	// and the tangent basis for each vertex thus is identical!
	//

	// Calculate the vectors from the current vertex to the two other vertices in the triangle
	VectorSubtract( v2, v1, v2v1 );
	VectorSubtract( v3, v1, v3v1 );

	// The equation presented in the article states that:
	// c2c1_T = V2.texcoord.x − V1.texcoord.x
	// c2c1_B = V2.texcoord.y − V1.texcoord.y
	// c3c1_T = V3.texcoord.x − V1.texcoord.x
	// c3c1_B = V3.texcoord.y − V1.texcoord.y

	// Calculate c2c1_T and c2c1_B
	c2c1_T = t2[ 0 ] - t1[ 0 ];
	c2c1_B = t2[ 1 ] - t2[ 1 ];

	// Calculate c3c1_T and c3c1_B
	c3c1_T = t3[ 0 ] - t1[ 0 ];
	c3c1_B = t3[ 1 ] - t1[ 1 ];

	denominator = c2c1_T * c3c1_B - c3c1_T * c2c1_B;

	//if(ROUNDOFF(fDenominator) == 0.0f)
	if ( denominator == 0.0f )
	{
		// We won't risk a divide by zero, so set the tangent matrix to the identity matrix
		VectorSet( tangent, 1, 0, 0 );
		VectorSet( binormal, 0, 1, 0 );
		VectorSet( normal, 0, 0, 1 );
	}
	else
	{
		// Calculate the reciprocal value once and for all (to achieve speed)
		scale1 = 1.0f / denominator;

		// T and B are calculated just as the equation in the article states
		VectorSet( T, ( c3c1_B * v2v1[ 0 ] - c2c1_B * v3v1[ 0 ] ) * scale1,
		           ( c3c1_B * v2v1[ 1 ] - c2c1_B * v3v1[ 1 ] ) * scale1,
		           ( c3c1_B * v2v1[ 2 ] - c2c1_B * v3v1[ 2 ] ) * scale1 );

		VectorSet( B, ( -c3c1_T * v2v1[ 0 ] + c2c1_T * v3v1[ 0 ] ) * scale1,
		           ( -c3c1_T * v2v1[ 1 ] + c2c1_T * v3v1[ 1 ] ) * scale1,
		           ( -c3c1_T * v2v1[ 2 ] + c2c1_T * v3v1[ 2 ] ) * scale1 );

		// The normal N is calculated as the cross product between T and B
		CrossProduct( T, B, N );

#if 0
		VectorCopy( T, tangent );
		VectorCopy( B, binormal );
		VectorCopy( N, normal );
#else
		// Calculate the reciprocal value once and for all (to achieve speed)
		scale2 = 1.0f / ( ( T[ 0 ] * B[ 1 ] * N[ 2 ] - T[ 2 ] * B[ 1 ] * N[ 0 ] ) +
		                  ( B[ 0 ] * N[ 1 ] * T[ 2 ] - B[ 2 ] * N[ 1 ] * T[ 0 ] ) +
		                  ( N[ 0 ] * T[ 1 ] * B[ 2 ] - N[ 2 ] * T[ 1 ] * B[ 0 ] ) );

		// Calculate the inverse if the TBN matrix using the formula described in the article.
		// We store the basis vectors directly in the provided TBN matrix: pvTBNMatrix
		CrossProduct( B, N, C );
		tangent[ 0 ] = C[ 0 ] * scale2;
		CrossProduct( N, T, C );
		tangent[ 1 ] = -C[ 0 ] * scale2;
		CrossProduct( T, B, C );
		tangent[ 2 ] = C[ 0 ] * scale2;
		VectorNormalize( tangent );

		CrossProduct( B, N, C );
		binormal[ 0 ] = -C[ 1 ] * scale2;
		CrossProduct( N, T, C );
		binormal[ 1 ] = C[ 1 ] * scale2;
		CrossProduct( T, B, C );
		binormal[ 2 ] = -C[ 1 ] * scale2;
		VectorNormalize( binormal );

		CrossProduct( B, N, C );
		normal[ 0 ] = C[ 2 ] * scale2;
		CrossProduct( N, T, C );
		normal[ 1 ] = -C[ 2 ] * scale2;
		CrossProduct( T, B, C );
		normal[ 2 ] = C[ 2 ] * scale2;
		VectorNormalize( normal );
#endif
	}
}

qboolean R_CalcTangentVectors( srfVert_t *dv[ 3 ] )
{
	int    i;
	float  bb, s, t;
	vec3_t bary;

	/* calculate barycentric basis for the triangle */
	bb = ( dv[ 1 ]->st[ 0 ] - dv[ 0 ]->st[ 0 ] ) * ( dv[ 2 ]->st[ 1 ] - dv[ 0 ]->st[ 1 ] ) - ( dv[ 2 ]->st[ 0 ] - dv[ 0 ]->st[ 0 ] ) * ( dv[ 1 ]->st[ 1 ] - dv[ 0 ]->st[ 1 ] );

	if ( fabs( bb ) < 0.00000001f )
	{
		return qfalse;
	}

	/* do each vertex */
	for ( i = 0; i < 3; i++ )
	{
		// calculate s tangent vector
		s = dv[ i ]->st[ 0 ] + 10.0f;
		t = dv[ i ]->st[ 1 ];
		bary[ 0 ] = ( ( dv[ 1 ]->st[ 0 ] - s ) * ( dv[ 2 ]->st[ 1 ] - t ) - ( dv[ 2 ]->st[ 0 ] - s ) * ( dv[ 1 ]->st[ 1 ] - t ) ) / bb;
		bary[ 1 ] = ( ( dv[ 2 ]->st[ 0 ] - s ) * ( dv[ 0 ]->st[ 1 ] - t ) - ( dv[ 0 ]->st[ 0 ] - s ) * ( dv[ 2 ]->st[ 1 ] - t ) ) / bb;
		bary[ 2 ] = ( ( dv[ 0 ]->st[ 0 ] - s ) * ( dv[ 1 ]->st[ 1 ] - t ) - ( dv[ 1 ]->st[ 0 ] - s ) * ( dv[ 0 ]->st[ 1 ] - t ) ) / bb;

		dv[ i ]->tangent[ 0 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 0 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 0 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 0 ];
		dv[ i ]->tangent[ 1 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 1 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 1 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 1 ];
		dv[ i ]->tangent[ 2 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 2 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 2 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 2 ];

		VectorSubtract( dv[ i ]->tangent, dv[ i ]->xyz, dv[ i ]->tangent );
		VectorNormalize( dv[ i ]->tangent );

		// calculate t tangent vector
		s = dv[ i ]->st[ 0 ];
		t = dv[ i ]->st[ 1 ] + 10.0f;
		bary[ 0 ] = ( ( dv[ 1 ]->st[ 0 ] - s ) * ( dv[ 2 ]->st[ 1 ] - t ) - ( dv[ 2 ]->st[ 0 ] - s ) * ( dv[ 1 ]->st[ 1 ] - t ) ) / bb;
		bary[ 1 ] = ( ( dv[ 2 ]->st[ 0 ] - s ) * ( dv[ 0 ]->st[ 1 ] - t ) - ( dv[ 0 ]->st[ 0 ] - s ) * ( dv[ 2 ]->st[ 1 ] - t ) ) / bb;
		bary[ 2 ] = ( ( dv[ 0 ]->st[ 0 ] - s ) * ( dv[ 1 ]->st[ 1 ] - t ) - ( dv[ 1 ]->st[ 0 ] - s ) * ( dv[ 0 ]->st[ 1 ] - t ) ) / bb;

		dv[ i ]->binormal[ 0 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 0 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 0 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 0 ];
		dv[ i ]->binormal[ 1 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 1 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 1 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 1 ];
		dv[ i ]->binormal[ 2 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 2 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 2 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 2 ];

		VectorSubtract( dv[ i ]->binormal, dv[ i ]->xyz, dv[ i ]->binormal );
		VectorNormalize( dv[ i ]->binormal );

		// debug code
		//% Sys_FPrintf( SYS_VRB, "%d S: (%f %f %f) T: (%f %f %f)\n", i,
		//%     stv[ i ][ 0 ], stv[ i ][ 1 ], stv[ i ][ 2 ], ttv[ i ][ 0 ], ttv[ i ][ 1 ], ttv[ i ][ 2 ] );
	}

	return qtrue;
}

/*
Tr3B: this function breaks the VC9 compiler for some unknown reason ...

float R_CalcFov(float fovX, float width, float height)
{
        static float  x;
        static float  fovY;

        x = width / tan(fovX / 360.0f * M_PI);
        fovY = atan2(height, x);
        fovY = fovY * 360.0f / M_PI;

        return fovY;
}
*/

/*
=================
R_CullLocalBox

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
cullResult_t R_CullLocalBox( vec3_t localBounds[ 2 ] )
{
#if 0
	int      i, j;
	vec3_t   transformed[ 8 ];
	float    dists[ 8 ];
	vec3_t   v;
	cplane_t *frust;
	int      anyBack;
	int      front, back;

	if ( r_nocull->integer )
	{
		return CULL_CLIP;
	}

	// transform into world space
	for ( i = 0; i < 8; i++ )
	{
		v[ 0 ] = localBounds[ i & 1 ][ 0 ];
		v[ 1 ] = localBounds[( i >> 1 ) & 1 ][ 1 ];
		v[ 2 ] = localBounds[( i >> 2 ) & 1 ][ 2 ];

		R_LocalPointToWorld( v, transformed[ i ] );
	}

	// check against frustum planes
	anyBack = 0;

	for ( i = 0; i < FRUSTUM_PLANES; i++ )
	{
		frust = &tr.viewParms.frustums[ 0 ][ i ];

		front = back = 0;

		for ( j = 0; j < 8; j++ )
		{
			dists[ j ] = DotProduct( transformed[ j ], frust->normal );

			if ( dists[ j ] > frust->dist )
			{
				front = 1;

				if ( back )
				{
					break; // a point is in front
				}
			}
			else
			{
				back = 1;
			}
		}

		if ( !front )
		{
			// all points were behind one of the planes
			return CULL_OUT;
		}

		anyBack |= back;
	}

	if ( !anyBack )
	{
		return CULL_IN; // completely inside frustum
	}

	return CULL_CLIP; // partially clipped
#else
	int      i, j;
	vec3_t   transformed;
	vec3_t   v;
	cplane_t *frust;
	qboolean anyClip;
	int      r;
	vec3_t   worldBounds[ 2 ];

	if ( r_nocull->integer )
	{
		return CULL_CLIP;
	}

	// transform into world space
	ClearBounds( worldBounds[ 0 ], worldBounds[ 1 ] );

	for ( j = 0; j < 8; j++ )
	{
		v[ 0 ] = localBounds[ j & 1 ][ 0 ];
		v[ 1 ] = localBounds[( j >> 1 ) & 1 ][ 1 ];
		v[ 2 ] = localBounds[( j >> 2 ) & 1 ][ 2 ];

		R_LocalPointToWorld( v, transformed );

		AddPointToBounds( transformed, worldBounds[ 0 ], worldBounds[ 1 ] );
	}

	// check against frustum planes
	anyClip = qfalse;

	for ( i = 0; i < FRUSTUM_PLANES; i++ )
	{
		frust = &tr.viewParms.frustums[ 0 ][ i ];

		r = BoxOnPlaneSide( worldBounds[ 0 ], worldBounds[ 1 ], frust );

		if ( r == 2 )
		{
			// completely outside frustum
			return CULL_OUT;
		}

		if ( r == 3 )
		{
			anyClip = qtrue;
		}
	}

	if ( !anyClip )
	{
		// completely inside frustum
		return CULL_IN;
	}

	// partially clipped
	return CULL_CLIP;
#endif
}

/*
=================
R_CullLocalPointAndRadius
=================
*/
int R_CullLocalPointAndRadius( vec3_t pt, float radius )
{
	vec3_t transformed;

	R_LocalPointToWorld( pt, transformed );

	return R_CullPointAndRadius( transformed, radius );
}

/*
=================
R_CullPointAndRadius
=================
*/
int R_CullPointAndRadius( vec3_t pt, float radius )
{
	int      i;
	float    dist;
	cplane_t *frust;
	qboolean mightBeClipped = qfalse;

	if ( r_nocull->integer )
	{
		return CULL_CLIP;
	}

	// check against frustum planes
	for ( i = 0; i < FRUSTUM_PLANES; i++ )
	{
		frust = &tr.viewParms.frustums[ 0 ][ i ];

		dist = DotProduct( pt, frust->normal ) - frust->dist;

		if ( dist < -radius )
		{
			return CULL_OUT;
		}
		else if ( dist <= radius )
		{
			mightBeClipped = qtrue;
		}
	}

	if ( mightBeClipped )
	{
		return CULL_CLIP;
	}

	return CULL_IN; // completely inside frustum
}

/*
=================
R_FogLocalPointAndRadius
=================
*/
int R_FogLocalPointAndRadius( const vec3_t pt, float radius )
{
	vec3_t transformed;

	R_LocalPointToWorld( pt, transformed );

	return R_FogPointAndRadius( transformed, radius );
}

/*
=================
R_FogPointAndRadius
=================
*/
int R_FogPointAndRadius( const vec3_t pt, float radius )
{
	int   i, j;
	fog_t *fog;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		return 0;
	}

	// FIXME: non-normalized axis issues
	for ( i = 1; i < tr.world->numFogs; i++ )
	{
		fog = &tr.world->fogs[ i ];

		for ( j = 0; j < 3; j++ )
		{
			if ( pt[ j ] - radius >= fog->bounds[ 1 ][ j ] )
			{
				break;
			}

			if ( pt[ j ] + radius <= fog->bounds[ 0 ][ j ] )
			{
				break;
			}
		}

		if ( j == 3 )
		{
			return i;
		}
	}

	return 0;
}

/*
=================
R_FogWorldBox
=================
*/
int R_FogWorldBox( vec3_t bounds[ 2 ] )
{
	int   i, j;
	fog_t *fog;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		return 0;
	}

	for ( i = 1; i < tr.world->numFogs; i++ )
	{
		fog = &tr.world->fogs[ i ];

		for ( j = 0; j < 3; j++ )
		{
			if ( bounds[ 0 ][ j ] >= fog->bounds[ 1 ][ j ] )
			{
				break;
			}

			if ( bounds[ 1 ][ j ] <= fog->bounds[ 0 ][ j ] )
			{
				break;
			}
		}

		if ( j == 3 )
		{
			return i;
		}
	}

	return 0;
}

/*
=================
R_LocalNormalToWorld
=================
*/
void R_LocalNormalToWorld( const vec3_t local, vec3_t world )
{
	MatrixTransformNormal( tr.orientation.transformMatrix, local, world );
}

/*
=================
R_LocalPointToWorld
=================
*/
void R_LocalPointToWorld( const vec3_t local, vec3_t world )
{
	MatrixTransformPoint( tr.orientation.transformMatrix, local, world );
}

/*
==========================
R_TransformWorldToClip
==========================
*/
void R_TransformWorldToClip( const vec3_t src, const float *cameraViewMatrix, const float *projectionMatrix, vec4_t eye,
                             vec4_t dst )
{
	vec4_t src2;

	VectorCopy( src, src2 );
	src2[ 3 ] = 1;

	MatrixTransform4( cameraViewMatrix, src2, eye );
	MatrixTransform4( projectionMatrix, eye, dst );
}

/*
==========================
R_TransformModelToClip
==========================
*/
void R_TransformModelToClip( const vec3_t src, const float *modelViewMatrix, const float *projectionMatrix, vec4_t eye, vec4_t dst )
{
	vec4_t src2;

	VectorCopy( src, src2 );
	src2[ 3 ] = 1;

	MatrixTransform4( modelViewMatrix, src2, eye );
	MatrixTransform4( projectionMatrix, eye, dst );
}

/*
==========================
R_TransformClipToWindow
==========================
*/
void R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window )
{
	normalized[ 0 ] = clip[ 0 ] / clip[ 3 ];
	normalized[ 1 ] = clip[ 1 ] / clip[ 3 ];
	normalized[ 2 ] = ( clip[ 2 ] + clip[ 3 ] ) / ( 2 * clip[ 3 ] );

	window[ 0 ] = view->viewportX + ( 0.5f * ( 1.0f + normalized[ 0 ] ) * view->viewportWidth );
	window[ 1 ] = view->viewportY + ( 0.5f * ( 1.0f + normalized[ 1 ] ) * view->viewportHeight );
	window[ 2 ] = normalized[ 2 ];

	window[ 0 ] = ( int )( window[ 0 ] + 0.5 );
	window[ 1 ] = ( int )( window[ 1 ] + 0.5 );
}

/*
================
R_ProjectRadius
================
*/
float R_ProjectRadius( float r, vec3_t location )
{
	float  pr;
	float  dist;
	float  c;
	vec3_t p;
	float  projected[ 4 ];

	c = DotProduct( tr.viewParms.orientation.axis[ 0 ], tr.viewParms.orientation.origin );
	dist = DotProduct( tr.viewParms.orientation.axis[ 0 ], location ) - c;

	if ( dist <= 0 )
	{
		return 0;
	}

	p[ 0 ] = 0;
	p[ 1 ] = fabs( r );
	p[ 2 ] = -dist;

	projected[ 0 ] = p[ 0 ] * tr.viewParms.projectionMatrix[ 0 ] +
	                 p[ 1 ] * tr.viewParms.projectionMatrix[ 4 ] + p[ 2 ] * tr.viewParms.projectionMatrix[ 8 ] + tr.viewParms.projectionMatrix[ 12 ];

	projected[ 1 ] = p[ 0 ] * tr.viewParms.projectionMatrix[ 1 ] +
	                 p[ 1 ] * tr.viewParms.projectionMatrix[ 5 ] + p[ 2 ] * tr.viewParms.projectionMatrix[ 9 ] + tr.viewParms.projectionMatrix[ 13 ];

	projected[ 2 ] = p[ 0 ] * tr.viewParms.projectionMatrix[ 2 ] +
	                 p[ 1 ] * tr.viewParms.projectionMatrix[ 6 ] + p[ 2 ] * tr.viewParms.projectionMatrix[ 10 ] + tr.viewParms.projectionMatrix[ 14 ];

	projected[ 3 ] = p[ 0 ] * tr.viewParms.projectionMatrix[ 3 ] +
	                 p[ 1 ] * tr.viewParms.projectionMatrix[ 7 ] + p[ 2 ] * tr.viewParms.projectionMatrix[ 11 ] + tr.viewParms.projectionMatrix[ 15 ];

	pr = projected[ 1 ] / projected[ 3 ];

	if ( pr > 1.0f )
	{
		pr = 1.0f;
	}

	return pr;
}

/*
=================
R_SetupEntityWorldBounds
Tr3B - needs R_RotateEntityForViewParms
=================
*/
void R_SetupEntityWorldBounds( trRefEntity_t *ent )
{
	int    j;
	vec3_t v;

	ClearBounds( ent->worldBounds[ 0 ], ent->worldBounds[ 1 ] );

	for ( j = 0; j < 8; j++ )
	{
		v[ 0 ] = ent->localBounds[ j & 1 ][ 0 ];
		v[ 1 ] = ent->localBounds[( j >> 1 ) & 1 ][ 1 ];
		v[ 2 ] = ent->localBounds[( j >> 2 ) & 1 ][ 2 ];

		// transform local bounds vertices into world space
		R_LocalPointToWorld( v, ent->worldCorners[ j ] );

		AddPointToBounds( ent->worldCorners[ j ], ent->worldBounds[ 0 ], ent->worldBounds[ 1 ] );
	}
}

/*
=================
R_RotateEntityForViewParms

Generates an orientation for an entity and viewParms
Does NOT produce any GL calls
Called by both the front end and the back end
=================
*/
void R_RotateEntityForViewParms( const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t * or )
{
	vec3_t delta;
	float  axisLength;

	if ( ent->e.reType != RT_MODEL )
	{
		* or = viewParms->world;
		return;
	}

	VectorCopy( ent->e.origin, or ->origin );

	VectorCopy( ent->e.axis[ 0 ], or ->axis[ 0 ] );
	VectorCopy( ent->e.axis[ 1 ], or ->axis[ 1 ] );
	VectorCopy( ent->e.axis[ 2 ], or ->axis[ 2 ] );

	MatrixSetupTransformFromVectorsFLU( or ->transformMatrix, or ->axis[ 0 ], or ->axis[ 1 ], or ->axis[ 2 ], or ->origin );
	MatrixAffineInverse( or ->transformMatrix, or ->viewMatrix );
	MatrixMultiply( viewParms->world.viewMatrix, or ->transformMatrix, or ->modelViewMatrix );

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
	VectorSubtract( viewParms->orientation.origin, or ->origin, delta );

	// compensate for scale in the axes if necessary
	if ( ent->e.nonNormalizedAxes )
	{
		axisLength = VectorLength( ent->e.axis[ 0 ] );

		if ( !axisLength )
		{
			axisLength = 0;
		}
		else
		{
			axisLength = 1.0f / axisLength;
		}
	}
	else
	{
		axisLength = 1.0f;
	}

	or ->viewOrigin[ 0 ] = DotProduct( delta, or ->axis[ 0 ] ) * axisLength;
	or ->viewOrigin[ 1 ] = DotProduct( delta, or ->axis[ 1 ] ) * axisLength;
	or ->viewOrigin[ 2 ] = DotProduct( delta, or ->axis[ 2 ] ) * axisLength;
}

/*
=================
R_RotateEntityForLight

Generates an orientation for an entity and light
Does NOT produce any GL calls
Called by both the front end and the back end
=================
*/
void R_RotateEntityForLight( const trRefEntity_t *ent, const trRefLight_t *light, orientationr_t * or )
{
	vec3_t delta;
	float  axisLength;

	if ( ent->e.reType != RT_MODEL )
	{
		Com_Memset( or , 0, sizeof( * or ) );

		or ->axis[ 0 ][ 0 ] = 1;
		or ->axis[ 1 ][ 1 ] = 1;
		or ->axis[ 2 ][ 2 ] = 1;

		VectorCopy( light->l.origin, or ->viewOrigin );

		MatrixIdentity( or ->transformMatrix );
		//MatrixAffineInverse(or->transformMatrix, or->viewMatrix);
		MatrixMultiply( light->viewMatrix, or ->transformMatrix, or ->viewMatrix );
		MatrixCopy( or ->viewMatrix, or ->modelViewMatrix );
		return;
	}

	VectorCopy( ent->e.origin, or ->origin );

	VectorCopy( ent->e.axis[ 0 ], or ->axis[ 0 ] );
	VectorCopy( ent->e.axis[ 1 ], or ->axis[ 1 ] );
	VectorCopy( ent->e.axis[ 2 ], or ->axis[ 2 ] );

	MatrixSetupTransformFromVectorsFLU( or ->transformMatrix, or ->axis[ 0 ], or ->axis[ 1 ], or ->axis[ 2 ], or ->origin );
	MatrixAffineInverse( or ->transformMatrix, or ->viewMatrix );

	MatrixMultiply( light->viewMatrix, or ->transformMatrix, or ->modelViewMatrix );

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
	VectorSubtract( light->l.origin, or ->origin, delta );

	// compensate for scale in the axes if necessary
	if ( ent->e.nonNormalizedAxes )
	{
		axisLength = VectorLength( ent->e.axis[ 0 ] );

		if ( !axisLength )
		{
			axisLength = 0;
		}
		else
		{
			axisLength = 1.0f / axisLength;
		}
	}
	else
	{
		axisLength = 1.0f;
	}

	or ->viewOrigin[ 0 ] = DotProduct( delta, or ->axis[ 0 ] ) * axisLength;
	or ->viewOrigin[ 1 ] = DotProduct( delta, or ->axis[ 1 ] ) * axisLength;
	or ->viewOrigin[ 2 ] = DotProduct( delta, or ->axis[ 2 ] ) * axisLength;
}

/*
=================
R_RotateLightForViewParms
=================
*/
void R_RotateLightForViewParms( const trRefLight_t *light, const viewParms_t *viewParms, orientationr_t * or )
{
	vec3_t delta;

	VectorCopy( light->l.origin, or ->origin );

	QuatToAxis( light->l.rotation, or ->axis );

	MatrixSetupTransformFromVectorsFLU( or ->transformMatrix, or ->axis[ 0 ], or ->axis[ 1 ], or ->axis[ 2 ], or ->origin );
	MatrixAffineInverse( or ->transformMatrix, or ->viewMatrix );
	MatrixMultiply( viewParms->world.viewMatrix, or ->transformMatrix, or ->modelViewMatrix );

	// calculate the viewer origin in the light's space
	// needed for fog, specular, and environment mapping
	VectorSubtract( viewParms->orientation.origin, or ->origin, delta );

	or ->viewOrigin[ 0 ] = DotProduct( delta, or ->axis[ 0 ] );
	or ->viewOrigin[ 1 ] = DotProduct( delta, or ->axis[ 1 ] );
	or ->viewOrigin[ 2 ] = DotProduct( delta, or ->axis[ 2 ] );
}

/*
=================
R_RotateForViewer

Sets up the modelview matrix for a given viewParm
=================
*/
void R_RotateForViewer( void )
{
	matrix_t transformMatrix;
//	axis_t     viewAxis;
//	vec3_t     forward, right, up;
	matrix_t viewMatrix;

	Com_Memset( &tr.orientation, 0, sizeof( tr.orientation ) );
	tr.orientation.axis[ 0 ][ 0 ] = 1;
	tr.orientation.axis[ 1 ][ 1 ] = 1;
	tr.orientation.axis[ 2 ][ 2 ] = 1;
	VectorCopy( tr.viewParms.orientation.origin, tr.orientation.viewOrigin );

	MatrixIdentity( tr.orientation.transformMatrix );

#if 0
	ri.Printf( PRINT_ALL, "transform forward = (%5.3f, %5.3f, %5.3f), left = (%5.3f, %5.3f, %5.3f), up = (%5.3f, %5.3f, %5.3f)\n",
	           tr.viewParms.orientation.axis[ 0 ][ 0 ], tr.viewParms.orientation.axis[ 0 ][ 1 ], tr.viewParms.orientation.axis[ 0 ][ 2 ],
	           tr.viewParms.orientation.axis[ 1 ][ 0 ], tr.viewParms.orientation.axis[ 1 ][ 1 ], tr.viewParms.orientation.axis[ 1 ][ 2 ],
	           tr.viewParms.orientation.axis[ 2 ][ 0 ], tr.viewParms.orientation.axis[ 2 ][ 1 ], tr.viewParms.orientation.axis[ 2 ][ 2 ] );
#endif

	// transform by the camera placement
	MatrixSetupTransformFromVectorsFLU( transformMatrix,
	                                    tr.viewParms.orientation.axis[ 0 ], tr.viewParms.orientation.axis[ 1 ], tr.viewParms.orientation.axis[ 2 ], tr.viewParms.orientation.origin );

	MatrixAffineInverse( transformMatrix, tr.orientation.viewMatrix2 );
//  MatrixAffineInverse(transformMatrix, tr.orientation.viewMatrix);

#if 0
	// convert from our right handed coordinate system (looking down X)
	// to D3D's left handed coordinate system (looking down Z)
	MatrixMultiply( quakeToD3DMatrix, tr.orientation.viewMatrix2, viewMatrix );
#elif 1
	// convert from our right handed coordinate system (looking down X)
	// to OpenGL's right handed coordinate system (looking down -Z)
	MatrixMultiply( quakeToOpenGLMatrix, tr.orientation.viewMatrix2, viewMatrix );
#else

	// Tr3B:  !!! THIS BREAKS MIRRORS !!!

	// http://redmine.xreal-project.net/issues/41
	// Bug #41 - Mirrors(and possibly portal cameras) are broken

	// convert from our right handed coordinate system (looking down X)
	// to OpenGL's right handed coordinate system (looking down -Z)
	MatrixLookAtRH( viewMatrix, tr.viewParms.orientation.origin, tr.viewParms.orientation.axis[ 0 ], tr.viewParms.orientation.axis[ 2 ] );
#endif

#if 0
	MatrixToVectorsFLU( viewMatrix, viewAxis[ 0 ], viewAxis[ 1 ], viewAxis[ 2 ] );

	ri.Printf( PRINT_ALL, "view forward = (%5.3f, %5.3f, %5.3f), left = (%5.3f, %5.3f, %5.3f), up = (%5.3f, %5.3f, %5.3f)\n",
	           viewAxis[ 0 ][ 0 ], viewAxis[ 0 ][ 1 ], viewAxis[ 0 ][ 2 ],
	           viewAxis[ 1 ][ 0 ], viewAxis[ 1 ][ 1 ], viewAxis[ 1 ][ 2 ],
	           viewAxis[ 2 ][ 0 ], viewAxis[ 2 ][ 1 ], viewAxis[ 2 ][ 2 ] );

	VectorSet( forward, 1, 0, 0 );
	VectorSet( right, 0, 1, 0 );
	VectorSet( up, 0, 0, 1 );

	MatrixTransformNormal2( viewMatrix, forward );
	MatrixTransformNormal2( viewMatrix, right );
	MatrixTransformNormal2( viewMatrix, up );

	ri.Printf( PRINT_ALL, "transformed forward = (%5.3f, %5.3f, %5.3f), right = (%5.3f, %5.3f, %5.3f), up = (%5.3f, %5.3f, %5.3f)\n",
	           forward[ 0 ], forward[ 1 ], forward[ 2 ],
	           right[ 0 ], right[ 1 ], right[ 2 ],
	           up[ 0 ], up[ 1 ], up[ 2 ] );
#endif

#if 0

	// Tr3B: support mirrors
	if ( tr.viewParms.isMirror )
	{
		vec4_t   plane;
		vec4_t   plane2;
		matrix_t mirrorMatrix, mirrorMatrix2;

		// clipping plane in world space
		plane[ 0 ] = tr.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = tr.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = tr.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = tr.viewParms.portalPlane.dist;

#if 1
		MatrixPlaneReflection( mirrorMatrix, plane );
		//MatrixInverse(mirrorMatrix);
		MatrixMultiply( mirrorMatrix, viewMatrix, tr.orientation.viewMatrix );
		//MatrixMultiply(viewMatrix, mirrorMatrix, tr.orientation.viewMatrix);
#else

		// clipping plane in view space
		plane2[ 0 ] = DotProduct( tr.viewParms.orientation.axis[ 0 ], plane );
		plane2[ 1 ] = DotProduct( tr.viewParms.orientation.axis[ 1 ], plane );
		plane2[ 2 ] = DotProduct( tr.viewParms.orientation.axis[ 2 ], plane );
		plane2[ 3 ] = ( DotProduct( plane, tr.viewParms.orientation.origin ) - plane[ 3 ] );

		//MatrixTransformPlane(viewMatrix, plane, plane2);
		//plane2[3] = -plane2[3];

		MatrixPlaneReflection( mirrorMatrix, plane2 );

		//MatrixMultiply(mirrorMatrix, quakeToOpenGLMatrix, mirrorMatrix2);
		//MatrixMultiply(quakeToOpenGLMatrix, mirrorMatrix, mirrorMatrix2);

		MatrixMultiply( mirrorMatrix, viewMatrix, tr.orientation.viewMatrix );
		//MatrixMultiply(viewMatrix, mirrorMatrix, tr.orientation.viewMatrix);
#endif
	}
	else
#endif
	{
		MatrixCopy( viewMatrix, tr.orientation.viewMatrix );
	}

	MatrixCopy( tr.orientation.viewMatrix, tr.orientation.modelViewMatrix );

	tr.viewParms.world = tr.orientation;
}

/*
** SetFarClip
*/
static void SetFarClip( void )
{
	float farthestCornerDistance;
	int   i;
//	vec3_t          v;
//	vec3_t          eye;
//	float          *modelMatrix;

	// if not rendering the world (icons, menus, etc)
	// set a 2k far clip plane
	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		tr.viewParms.zFar = 2048;
		return;
	}

	//
	// set far clipping planes dynamically
	//
/// Berserker: setup the more precision zFar - convert corners from world to eye coordinates, and take the farthest X local coordinate
#if 0
	modelMatrix = tr.viewParms.world.viewMatrix2;
	farthestCornerDistance = 0;

	// check visBounds
	for ( i = 0; i < 8; i++ )
	{
		int j;

		if ( i & 1 )
		{
			v[ 0 ] = tr.viewParms.visBounds[ 0 ][ 0 ];
		}
		else
		{
			v[ 0 ] = tr.viewParms.visBounds[ 1 ][ 0 ];
		}

		if ( i & 2 )
		{
			v[ 1 ] = tr.viewParms.visBounds[ 0 ][ 1 ];
		}
		else
		{
			v[ 1 ] = tr.viewParms.visBounds[ 1 ][ 1 ];
		}

		if ( i & 4 )
		{
			v[ 2 ] = tr.viewParms.visBounds[ 0 ][ 2 ];
		}
		else
		{
			v[ 2 ] = tr.viewParms.visBounds[ 1 ][ 2 ];
		}

		for ( j = 0; j < 3; j++ )
		{
			eye[ j ] =
			  v[ 0 ] * modelMatrix[ j + 0 * 4 ] +
			  v[ 1 ] * modelMatrix[ j + 1 * 4 ] + v[ 2 ] * modelMatrix[ j + 2 * 4 ] + modelMatrix[ j + 3 * 4 ];
		}

		farthestCornerDistance = max( farthestCornerDistance, eye[ 0 ] * eye[ 0 ] + eye[ 1 ] * eye[ 1 ] + eye[ 2 ] * eye[ 2 ] );
	}

#if 0

	// check lightBounds
	if ( tr.refdef.numLights )
	{
		for ( i = 0; i < 8; i++ )
		{
			int j;

			if ( i & 1 )
			{
				v[ 0 ] = tr.viewParms.lightBounds[ 0 ][ 0 ];
			}
			else
			{
				v[ 0 ] = tr.viewParms.lightBounds[ 1 ][ 0 ];
			}

			if ( i & 2 )
			{
				v[ 1 ] = tr.viewParms.lightBounds[ 0 ][ 1 ];
			}
			else
			{
				v[ 1 ] = tr.viewParms.lightBounds[ 1 ][ 1 ];
			}

			if ( i & 4 )
			{
				v[ 2 ] = tr.viewParms.lightBounds[ 0 ][ 2 ];
			}
			else
			{
				v[ 2 ] = tr.viewParms.lightBounds[ 1 ][ 2 ];
			}

			for ( j = 0; j < 3; j++ )
			{
				eye[ j ] =
				  v[ 0 ] * modelMatrix[ j + 0 * 4 ] +
				  v[ 1 ] * modelMatrix[ j + 1 * 4 ] + v[ 2 ] * modelMatrix[ j + 2 * 4 ] + modelMatrix[ j + 3 * 4 ];
			}

			farthestCornerDistance = max( farthestCornerDistance, eye[ 0 ] * eye[ 0 ] + eye[ 1 ] * eye[ 1 ] + eye[ 2 ] * eye[ 2 ] );
		}
	}

#endif

#else
	farthestCornerDistance = 0;

	// check visBounds
	for ( i = 0; i < 8; i++ )
	{
		vec3_t v;
		float  distance;

		if ( i & 1 )
		{
			v[ 0 ] = tr.viewParms.visBounds[ 0 ][ 0 ];
		}
		else
		{
			v[ 0 ] = tr.viewParms.visBounds[ 1 ][ 0 ];
		}

		if ( i & 2 )
		{
			v[ 1 ] = tr.viewParms.visBounds[ 0 ][ 1 ];
		}
		else
		{
			v[ 1 ] = tr.viewParms.visBounds[ 1 ][ 1 ];
		}

		if ( i & 4 )
		{
			v[ 2 ] = tr.viewParms.visBounds[ 0 ][ 2 ];
		}
		else
		{
			v[ 2 ] = tr.viewParms.visBounds[ 1 ][ 2 ];
		}

		distance = DistanceSquared( v, tr.viewParms.orientation.origin );

		if ( distance > farthestCornerDistance )
		{
			farthestCornerDistance = distance;
		}
	}

#if 0

	// check lightBounds
	for ( i = 0; i < 8; i++ )
	{
		vec3_t v;
		vec3_t vecTo;
		float  distance;

		if ( i & 1 )
		{
			v[ 0 ] = tr.viewParms.lightBounds[ 0 ][ 0 ];
		}
		else
		{
			v[ 0 ] = tr.viewParms.lightBounds[ 1 ][ 0 ];
		}

		if ( i & 2 )
		{
			v[ 1 ] = tr.viewParms.lightBounds[ 0 ][ 1 ];
		}
		else
		{
			v[ 1 ] = tr.viewParms.lightBounds[ 1 ][ 1 ];
		}

		if ( i & 4 )
		{
			v[ 2 ] = tr.viewParms.lightBounds[ 0 ][ 2 ];
		}
		else
		{
			v[ 2 ] = tr.viewParms.lightBounds[ 1 ][ 2 ];
		}

		VectorSubtract( v, tr.viewParms.orientation.origin, vecTo );

		distance = vecTo[ 0 ] * vecTo[ 0 ] + vecTo[ 1 ] * vecTo[ 1 ] + vecTo[ 2 ] * vecTo[ 2 ];

		if ( distance > farthestCornerDistance )
		{
			farthestCornerDistance = distance;
		}
	}

#endif

#endif
	tr.viewParms.zFar = sqrt( farthestCornerDistance );

	// ydnar: add global q3 fog
#if 0

	if ( tr.world != NULL && tr.world->globalFog >= 0 &&
	     tr.world->fogs[ tr.world->globalFog ].fogParms.depthForOpaque < tr.viewParms.zFar )
	{
		tr.viewParms.zFar = tr.world->fogs[ tr.world->globalFog ].fogParms.depthForOpaque;
	}

#endif
}

/*
===============
R_SetupProjection
===============
*/
// *INDENT-OFF*
static void R_SetupProjection( qboolean infiniteFarClip )
{
	float zNear, zFar;
	float *proj = tr.viewParms.projectionMatrix;

	// dynamically compute far clip plane distance
	SetFarClip();

	zNear = tr.viewParms.zNear = r_znear->value;

	if ( r_zfar->value )
	{
		zFar = tr.viewParms.zFar = MAX( tr.viewParms.zFar, r_zfar->value );
	}
	else if ( infiniteFarClip )
	{
		zFar = tr.viewParms.zFar = 0;
	}
	else
	{
		zFar = tr.viewParms.zFar;
	}

#if 0
	{
		float xMin, yMin;
		float xMax, yMax;
		float width, height, depth;

		yMax = zNear * tan( tr.refdef.fov_y * M_PI / 360.0f );
		xMax = zNear * tan( tr.refdef.fov_x * M_PI / 360.0f );
		yMin = -yMax;
		xMin = -xMax;
		width = xMax - xMin;
		height = yMax - yMin;
		depth = zFar - zNear;

		if ( zFar <= 0 || infiniteFarClip )
		{
			// Tr3B: far plane at infinity, see RobustShadowVolumes.pdf by Nvidia
			proj[ 0 ] = 2 * zNear / width;
			proj[ 4 ] = 0;
			proj[ 8 ] = ( xMax + xMin ) / width;
			proj[ 12 ] = 0;
			proj[ 1 ] = 0;
			proj[ 5 ] = 2 * zNear / height;
			proj[ 9 ] = ( yMax + yMin ) / height;
			proj[ 13 ] = 0;
			proj[ 2 ] = 0;
			proj[ 6 ] = 0;
			proj[ 10 ] = -1;
			proj[ 14 ] = -2 * zNear;
			proj[ 3 ] = 0;
			proj[ 7 ] = 0;
			proj[ 11 ] = -1;
			proj[ 15 ] = 0;
		}
		else
		{
			// standard OpenGL projection matrix
			proj[ 0 ] = 2 * zNear / width;
			proj[ 4 ] = 0;
			proj[ 8 ] = ( xMax + xMin ) / width;
			proj[ 12 ] = 0;
			proj[ 1 ] = 0;
			proj[ 5 ] = 2 * zNear / height;
			proj[ 9 ] = ( yMax + yMin ) / height;
			proj[ 13 ] = 0;
			proj[ 2 ] = 0;
			proj[ 6 ] = 0;
			proj[ 10 ] = - ( zFar + zNear ) / depth;
			proj[ 14 ] = -2 * zFar * zNear / depth;
			proj[ 3 ] = 0;
			proj[ 7 ] = 0;
			proj[ 11 ] = -1;
			proj[ 15 ] = 0;
		}
	}
#else

	if ( zFar <= 0 || infiniteFarClip ) // || r_showBspNodes->integer)
	{
		MatrixPerspectiveProjectionFovXYInfiniteRH( proj, tr.refdef.fov_x, tr.refdef.fov_y, zNear );
	}
	else
	{
		MatrixPerspectiveProjectionFovXYRH( proj, tr.refdef.fov_x, tr.refdef.fov_y, zNear, zFar );
	}

#endif

#if 0
	int      i;
	matrix_t mvp;
	vec4_t   axis[ 3 ];
	vec4_t   trans;

	MatrixMultiply( tr.viewParms.projectionMatrix, tr.orientation.modelViewMatrix, mvp );
	MatrixCopy( tr.orientation.modelViewMatrix, mvp );

	Vector4Set( axis[ 0 ], 1, 0, 0, 1 );
	Vector4Set( axis[ 1 ], 0, 1, 0, 1 );
	Vector4Set( axis[ 2 ], 0, 0, 1, 1 );

	for ( i = 0; i < 3; i++ )
	{
		MatrixTransform4( mvp, axis[ i ], trans );

		trans[ 0 ] /= trans[ 3 ];
		trans[ 1 ] /= trans[ 3 ];
		trans[ 2 ] /= trans[ 3 ];

		ri.Printf( PRINT_ALL, "transformed[%i] = (%5.3f, %5.3f, %5.3f)\n", i, trans[ 0 ], trans[ 1 ], trans[ 2 ] );
	}

#endif
}

// *INDENT-ON*

/*
=================
R_SetupUnprojection
create a matrix with similar functionality like gluUnproject, project from window space to world space
=================
*/
static void R_SetupUnprojection( void )
{
	float *unprojectMatrix = tr.viewParms.unprojectionMatrix;

	MatrixCopy( tr.viewParms.projectionMatrix, unprojectMatrix );
	MatrixMultiply2( unprojectMatrix, quakeToOpenGLMatrix );
	MatrixMultiply2( unprojectMatrix, tr.viewParms.world.viewMatrix2 );
	MatrixInverse( unprojectMatrix );

	// FIXME ?
	// MatrixMultiplyTranslation(unprojectMatrix, -(float)glConfig.vidWidth / (float)tr.viewParms.viewportWidth,
	// -(float)glConfig.vidHeight / (float)tr.viewParms.viewportHeight, -1.0);

	MatrixMultiplyTranslation( unprojectMatrix, -1.0, -1.0, -1.0 );
	MatrixMultiplyScale( unprojectMatrix, 2.0 * Q_recip( ( float ) glConfig.vidWidth ), 2.0 * Q_recip( ( float ) glConfig.vidHeight ), 2.0 );
}

/*
=================
R_SetupFrustum

Setup that culling frustum planes for the current view
=================
*/
static void R_SetupFrustum( void )
{
	int    i;
	float  xs, xc;
	float  ang;
	vec3_t planeOrigin;

	ang = tr.viewParms.fovX / 180 * M_PI * 0.5f;
	xs = sin( ang );
	xc = cos( ang );

	VectorScale( tr.viewParms.orientation.axis[ 0 ], xs, tr.viewParms.frustums[ 0 ][ 0 ].normal );
	VectorMA( tr.viewParms.frustums[ 0 ][ 0 ].normal, xc, tr.viewParms.orientation.axis[ 1 ], tr.viewParms.frustums[ 0 ][ 0 ].normal );

	VectorScale( tr.viewParms.orientation.axis[ 0 ], xs, tr.viewParms.frustums[ 0 ][ 1 ].normal );
	VectorMA( tr.viewParms.frustums[ 0 ][ 1 ].normal, -xc, tr.viewParms.orientation.axis[ 1 ], tr.viewParms.frustums[ 0 ][ 1 ].normal );

	ang = tr.viewParms.fovY / 180 * M_PI * 0.5f;
	xs = sin( ang );
	xc = cos( ang );

	VectorScale( tr.viewParms.orientation.axis[ 0 ], xs, tr.viewParms.frustums[ 0 ][ 2 ].normal );
	VectorMA( tr.viewParms.frustums[ 0 ][ 2 ].normal, xc, tr.viewParms.orientation.axis[ 2 ], tr.viewParms.frustums[ 0 ][ 2 ].normal );

	VectorScale( tr.viewParms.orientation.axis[ 0 ], xs, tr.viewParms.frustums[ 0 ][ 3 ].normal );
	VectorMA( tr.viewParms.frustums[ 0 ][ 3 ].normal, -xc, tr.viewParms.orientation.axis[ 2 ], tr.viewParms.frustums[ 0 ][ 3 ].normal );

	for ( i = 0; i < 4; i++ )
	{
		tr.viewParms.frustums[ 0 ][ i ].type = PLANE_NON_AXIAL;
		tr.viewParms.frustums[ 0 ][ i ].dist = DotProduct( tr.viewParms.orientation.origin, tr.viewParms.frustums[ 0 ][ i ].normal );
		SetPlaneSignbits( &tr.viewParms.frustums[ 0 ][ i ] );
	}

	// Tr3B: set extra near plane which is required by the dynamic occlusion culling
	tr.viewParms.frustums[ 0 ][ FRUSTUM_NEAR ].type = PLANE_NON_AXIAL;
	VectorCopy( tr.viewParms.orientation.axis[ 0 ], tr.viewParms.frustums[ 0 ][ FRUSTUM_NEAR ].normal );

	VectorMA( tr.viewParms.orientation.origin, r_znear->value, tr.viewParms.frustums[ 0 ][ FRUSTUM_NEAR ].normal, planeOrigin );
	tr.viewParms.frustums[ 0 ][ FRUSTUM_NEAR ].dist = DotProduct( planeOrigin, tr.viewParms.frustums[ 0 ][ FRUSTUM_NEAR ].normal );
	SetPlaneSignbits( &tr.viewParms.frustums[ 0 ][ FRUSTUM_NEAR ] );
}

/*
=================
R_SetupFrustum

Setup that culling frustum planes for the current view
=================
*/
// *INDENT-OFF*
void R_SetupFrustum2( frustum_t frustum, const matrix_t mvp )
{
	// http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf
	int i;

	// left
	frustum[ FRUSTUM_LEFT ].normal[ 0 ] = mvp[ 3 ] + mvp[ 0 ];
	frustum[ FRUSTUM_LEFT ].normal[ 1 ] = mvp[ 7 ] + mvp[ 4 ];
	frustum[ FRUSTUM_LEFT ].normal[ 2 ] = mvp[ 11 ] + mvp[ 8 ];
	frustum[ FRUSTUM_LEFT ].dist = - ( mvp[ 15 ] + mvp[ 12 ] );

	// right
	frustum[ FRUSTUM_RIGHT ].normal[ 0 ] = mvp[ 3 ] - mvp[ 0 ];
	frustum[ FRUSTUM_RIGHT ].normal[ 1 ] = mvp[ 7 ] - mvp[ 4 ];
	frustum[ FRUSTUM_RIGHT ].normal[ 2 ] = mvp[ 11 ] - mvp[ 8 ];
	frustum[ FRUSTUM_RIGHT ].dist = - ( mvp[ 15 ] - mvp[ 12 ] );

	// bottom
	frustum[ FRUSTUM_BOTTOM ].normal[ 0 ] = mvp[ 3 ] + mvp[ 1 ];
	frustum[ FRUSTUM_BOTTOM ].normal[ 1 ] = mvp[ 7 ] + mvp[ 5 ];
	frustum[ FRUSTUM_BOTTOM ].normal[ 2 ] = mvp[ 11 ] + mvp[ 9 ];
	frustum[ FRUSTUM_BOTTOM ].dist = - ( mvp[ 15 ] + mvp[ 13 ] );

	// top
	frustum[ FRUSTUM_TOP ].normal[ 0 ] = mvp[ 3 ] - mvp[ 1 ];
	frustum[ FRUSTUM_TOP ].normal[ 1 ] = mvp[ 7 ] - mvp[ 5 ];
	frustum[ FRUSTUM_TOP ].normal[ 2 ] = mvp[ 11 ] - mvp[ 9 ];
	frustum[ FRUSTUM_TOP ].dist = - ( mvp[ 15 ] - mvp[ 13 ] );

	// near
	frustum[ FRUSTUM_NEAR ].normal[ 0 ] = mvp[ 3 ] + mvp[ 2 ];
	frustum[ FRUSTUM_NEAR ].normal[ 1 ] = mvp[ 7 ] + mvp[ 6 ];
	frustum[ FRUSTUM_NEAR ].normal[ 2 ] = mvp[ 11 ] + mvp[ 10 ];
	frustum[ FRUSTUM_NEAR ].dist = - ( mvp[ 15 ] + mvp[ 14 ] );

	// far
	frustum[ FRUSTUM_FAR ].normal[ 0 ] = mvp[ 3 ] - mvp[ 2 ];
	frustum[ FRUSTUM_FAR ].normal[ 1 ] = mvp[ 7 ] - mvp[ 6 ];
	frustum[ FRUSTUM_FAR ].normal[ 2 ] = mvp[ 11 ] - mvp[ 10 ];
	frustum[ FRUSTUM_FAR ].dist = - ( mvp[ 15 ] - mvp[ 14 ] );

	for ( i = 0; i < 6; i++ )
	{
		vec_t length, ilength;

		frustum[ i ].type = PLANE_NON_AXIAL;

		// normalize
		length = VectorLength( frustum[ i ].normal );

		if ( length )
		{
			ilength = 1.0 / length;
			frustum[ i ].normal[ 0 ] *= ilength;
			frustum[ i ].normal[ 1 ] *= ilength;
			frustum[ i ].normal[ 2 ] *= ilength;
			frustum[ i ].dist *= ilength;
		}

		SetPlaneSignbits( &frustum[ i ] );
	}
}

// *INDENT-ON*

static void CopyPlane( const cplane_t *in, cplane_t *out )
{
	VectorCopy( in->normal, out->normal );
	out->dist = in->dist;
	out->type = in->type;
	out->signbits = in->signbits;
	out->pad[ 0 ] = in->pad[ 0 ];
	out->pad[ 1 ] = in->pad[ 1 ];
}

static void R_SetupSplitFrustums( void )
{
	int    i, j;
	float  lambda;
	float  ratio;
	vec3_t planeOrigin;
	float  zNear, zFar;

	lambda = r_parallelShadowSplitWeight->value;
	ratio = tr.viewParms.zFar / tr.viewParms.zNear;

	for ( j = 0; j < 5; j++ )
	{
		CopyPlane( &tr.viewParms.frustums[ 0 ][ j ], &tr.viewParms.frustums[ 1 ][ j ] );
	}

	for ( i = 1; i <= ( r_parallelShadowSplits->integer + 1 ); i++ )
	{
		float si = i / ( float )( r_parallelShadowSplits->integer + 1 );

		zFar = 1.005f * lambda * ( tr.viewParms.zNear * powf( ratio, si ) ) + ( 1 - lambda ) * ( tr.viewParms.zNear + ( tr.viewParms.zFar - tr.viewParms.zNear ) * si );

		if ( i <= r_parallelShadowSplits->integer )
		{
			tr.viewParms.parallelSplitDistances[ i - 1 ] = zFar;
		}

		tr.viewParms.frustums[ i ][ FRUSTUM_FAR ].type = PLANE_NON_AXIAL;
		VectorNegate( tr.viewParms.orientation.axis[ 0 ], tr.viewParms.frustums[ i ][ FRUSTUM_FAR ].normal );

		VectorMA( tr.viewParms.orientation.origin, zFar, tr.viewParms.orientation.axis[ 0 ], planeOrigin );
		tr.viewParms.frustums[ i ][ FRUSTUM_FAR ].dist = DotProduct( planeOrigin, tr.viewParms.frustums[ i ][ FRUSTUM_FAR ].normal );
		SetPlaneSignbits( &tr.viewParms.frustums[ i ][ FRUSTUM_FAR ] );

		if ( i <= ( r_parallelShadowSplits->integer ) )
		{
			zNear = zFar - ( zFar * 0.005f );
			tr.viewParms.frustums[ i + 1 ][ FRUSTUM_NEAR ].type = PLANE_NON_AXIAL;
			VectorCopy( tr.viewParms.orientation.axis[ 0 ], tr.viewParms.frustums[ i + 1 ][ FRUSTUM_NEAR ].normal );

			VectorMA( tr.viewParms.orientation.origin, zNear, tr.viewParms.orientation.axis[ 0 ], planeOrigin );
			tr.viewParms.frustums[ i + 1 ][ FRUSTUM_NEAR ].dist = DotProduct( planeOrigin, tr.viewParms.frustums[ i + 1 ][ FRUSTUM_NEAR ].normal );
			SetPlaneSignbits( &tr.viewParms.frustums[ i + 1 ][ FRUSTUM_NEAR ] );
		}

		for ( j = 0; j < 4; j++ )
		{
			CopyPlane( &tr.viewParms.frustums[ 0 ][ j ], &tr.viewParms.frustums[ i ][ j ] );
		}
	}
}

/*
=================
R_MirrorPoint
=================
*/
void R_MirrorPoint( vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out )
{
	int    i;
	vec3_t local;
	vec3_t transformed;
	float  d;

	VectorSubtract( in, surface->origin, local );

	VectorClear( transformed );

	for ( i = 0; i < 3; i++ )
	{
		d = DotProduct( local, surface->axis[ i ] );
		VectorMA( transformed, d, camera->axis[ i ], transformed );
	}

	VectorAdd( transformed, camera->origin, out );
}

void R_MirrorVector( vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out )
{
	int   i;
	float d;

	VectorClear( out );

	for ( i = 0; i < 3; i++ )
	{
		d = DotProduct( in, surface->axis[ i ] );
		VectorMA( out, d, camera->axis[ i ], out );
	}
}

/*
=============
R_PlaneForSurface
=============
*/
void R_PlaneForSurface( surfaceType_t *surfType, cplane_t *plane )
{
	srfTriangles_t *tri;
	srfPoly_t      *poly;
	srfVert_t      *v1, *v2, *v3;
	vec4_t         plane4;

	if ( !surfType )
	{
		Com_Memset( plane, 0, sizeof( *plane ) );
		plane->normal[ 0 ] = 1;
		return;
	}

	switch ( *surfType )
	{
		case SF_FACE:
			*plane = ( ( srfSurfaceFace_t * ) surfType )->plane;
			return;

		case SF_TRIANGLES:
			tri = ( srfTriangles_t * ) surfType;
			v1 = tri->verts + tri->triangles[ 0 ].indexes[ 0 ];
			v2 = tri->verts + tri->triangles[ 0 ].indexes[ 1 ];
			v3 = tri->verts + tri->triangles[ 0 ].indexes[ 2 ];
			PlaneFromPoints( plane4, v1->xyz, v2->xyz, v3->xyz );
			VectorCopy( plane4, plane->normal );
			plane->dist = plane4[ 3 ];
			return;

		case SF_POLY:
			poly = ( srfPoly_t * ) surfType;
			PlaneFromPoints( plane4, poly->verts[ 0 ].xyz, poly->verts[ 1 ].xyz, poly->verts[ 2 ].xyz );
			VectorCopy( plane4, plane->normal );
			plane->dist = plane4[ 3 ];
			return;

		default:
			Com_Memset( plane, 0, sizeof( *plane ) );
			plane->normal[ 0 ] = 1;
			return;
	}
}

/*
=================
R_GetPortalOrientation

entityNum is the entity that the portal surface is a part of, which may
be moving and rotating.

Returns qtrue if it should be mirrored
=================
*/
static qboolean R_GetPortalOrientations( drawSurf_t *drawSurf, orientation_t *surface, orientation_t *camera, vec3_t pvsOrigin,
    qboolean *mirror )
{
	int           i;
	cplane_t      originalPlane, plane;
	trRefEntity_t *e;
	float         d;
	vec3_t        transformed;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	//ri.Printf(PRINT_ALL, "R_GetPortalOrientations: original plane = (%5.3f, %5.3f, %5.3f)\n", originalPlane.normal[0], originalPlane.normal[1], originalPlane.normal[2]);

	// rotate the plane if necessary
	if ( drawSurf->entity != &tr.worldEntity )
	{
		tr.currentEntity = drawSurf->entity;

		// get the orientation of the entity
		R_RotateEntityForViewParms( tr.currentEntity, &tr.viewParms, &tr.orientation );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.orientation.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.orientation.origin );
	}
	else
	{
		plane = originalPlane;
	}

	VectorCopy( plane.normal, surface->axis[ 0 ] );
	PerpendicularVector( surface->axis[ 1 ], surface->axis[ 0 ] );
	CrossProduct( surface->axis[ 0 ], surface->axis[ 1 ], surface->axis[ 2 ] );

#if 0
	// Doom3 style mirror support
	shader = tr.sortedShaders[ drawSurf->shaderNum ];

	if ( shader->isMirror )
	{
		//ri.Printf(PRINT_ALL, "Portal surface with a mirror\n");

		VectorScale( plane.normal, plane.dist, surface->origin );
		VectorCopy( surface->origin, camera->origin );
		VectorSubtract( vec3_origin, surface->axis[ 0 ], camera->axis[ 0 ] );
		VectorCopy( surface->axis[ 1 ], camera->axis[ 1 ] );
		VectorCopy( surface->axis[ 2 ], camera->axis[ 2 ] );

		*mirror = qtrue;
		return qtrue;
	}

#endif

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( i = 0; i < tr.refdef.numEntities; i++ )
	{
		e = &tr.refdef.entities[ i ];

		if ( e->e.reType != RT_PORTALSURFACE )
		{
			continue;
		}

		d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;

		if ( d > 64 || d < -64 )
		{
			continue;
		}

		// get the pvsOrigin from the entity
		VectorCopy( e->e.oldorigin, pvsOrigin );

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[ 0 ] == e->e.origin[ 0 ] && e->e.oldorigin[ 1 ] == e->e.origin[ 1 ] && e->e.oldorigin[ 2 ] == e->e.origin[ 2 ] )
		{
			//ri.Printf(PRINT_ALL, "Portal surface with a mirror entity\n");

			VectorScale( plane.normal, plane.dist, surface->origin );
			VectorCopy( surface->origin, camera->origin );
			VectorSubtract( vec3_origin, surface->axis[ 0 ], camera->axis[ 0 ] );
			VectorCopy( surface->axis[ 1 ], camera->axis[ 1 ] );
			VectorCopy( surface->axis[ 2 ], camera->axis[ 2 ] );

			*mirror = qtrue;
			return qtrue;
		}

		// project the origin onto the surface plane to get
		// an origin point we can rotate around
		d = DotProduct( e->e.origin, plane.normal ) - plane.dist;
		VectorMA( e->e.origin, -d, surface->axis[ 0 ], surface->origin );

		// now get the camera origin and orientation
		VectorCopy( e->e.oldorigin, camera->origin );
		AxisCopy( e->e.axis, camera->axis );
		VectorSubtract( vec3_origin, camera->axis[ 0 ], camera->axis[ 0 ] );
		VectorSubtract( vec3_origin, camera->axis[ 1 ], camera->axis[ 1 ] );

		// optionally rotate
		if ( e->e.oldframe )
		{
			// if a speed is specified
			if ( e->e.frame )
			{
				// continuous rotate
				d = ( tr.refdef.time / 1000.0f ) * e->e.frame;
				VectorCopy( camera->axis[ 1 ], transformed );
				RotatePointAroundVector( camera->axis[ 1 ], camera->axis[ 0 ], transformed, d );
				CrossProduct( camera->axis[ 0 ], camera->axis[ 1 ], camera->axis[ 2 ] );
			}
			else
			{
				// bobbing rotate, with skinNum being the rotation offset
				d = sin( tr.refdef.time * 0.003f );
				d = e->e.skinNum + d * 4;
				VectorCopy( camera->axis[ 1 ], transformed );
				RotatePointAroundVector( camera->axis[ 1 ], camera->axis[ 0 ], transformed, d );
				CrossProduct( camera->axis[ 0 ], camera->axis[ 1 ], camera->axis[ 2 ] );
			}
		}
		else if ( e->e.skinNum )
		{
			d = e->e.skinNum;
			VectorCopy( camera->axis[ 1 ], transformed );
			RotatePointAroundVector( camera->axis[ 1 ], camera->axis[ 0 ], transformed, d );
			CrossProduct( camera->axis[ 0 ], camera->axis[ 1 ], camera->axis[ 2 ] );
		}

		//ri.Printf(PRINT_ALL, "Portal surface with a portal entity\n");

		*mirror = qfalse;
		return qtrue;
	}

	// if we didn't locate a portal entity, don't render anything.
	// We don't want to just treat it as a mirror, because without a
	// portal entity the server won't have communicated a proper entity set
	// in the snapshot

	// unfortunately, with local movement prediction it is easily possible
	// to see a surface before the server has communicated the matching
	// portal surface entity, so we don't want to print anything here...

	//ri.Printf(PRINT_ALL, "Portal surface without a portal entity\n");

	return qfalse;
}

static qboolean IsMirror( const drawSurf_t *drawSurf )
{
	int           i;
	cplane_t      originalPlane, plane;
	trRefEntity_t *e;
	float         d;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	if ( tr.currentEntity != &tr.worldEntity )
	{
		// get the orientation of the entity
		R_RotateEntityForViewParms( tr.currentEntity, &tr.viewParms, &tr.orientation );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.orientation.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.orientation.origin );
	}
	else
	{
		plane = originalPlane;
	}

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( i = 0; i < tr.refdef.numEntities; i++ )
	{
		e = &tr.refdef.entities[ i ];

		if ( e->e.reType != RT_PORTALSURFACE )
		{
			continue;
		}

		d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;

		if ( d > 64 || d < -64 )
		{
			continue;
		}

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[ 0 ] == e->e.origin[ 0 ] && e->e.oldorigin[ 1 ] == e->e.origin[ 1 ] && e->e.oldorigin[ 2 ] == e->e.origin[ 2 ] )
		{
			return qtrue;
		}

		return qfalse;
	}

	return qfalse;
}

/*
** SurfIsOffscreen
**
** Determines if a surface is completely offscreen.
*/
static qboolean SurfIsOffscreen( const drawSurf_t *drawSurf, vec4_t clipDest[ 128 ] )
{
	float        shortest = 100000000;
	shader_t     *shader;
	int          numTriangles;
	vec4_t       clip, eye;
	int          i;
	unsigned int pointOr = 0;
	unsigned int pointAnd = ( unsigned int ) ~0;

	if ( glConfig.smpActive )
	{
		// FIXME!  we can't do Tess_Begin/Tess_End stuff with smp!
		return qfalse;
	}

	tr.currentEntity = drawSurf->entity;
	shader = tr.sortedShaders[ drawSurf->shaderNum ];

	// rotate if necessary
	if ( tr.currentEntity != &tr.worldEntity )
	{
		//ri.Printf(PRINT_ALL, "entity portal surface\n");
		R_RotateEntityForViewParms( tr.currentEntity, &tr.viewParms, &tr.orientation );
	}
	else
	{
		//ri.Printf(PRINT_ALL, "world portal surface\n");
		tr.orientation = tr.viewParms.world;
	}

	Tess_Begin( Tess_StageIteratorGeneric, NULL, shader, NULL, qtrue, qtrue, -1, 0 );
	rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );

	// Tr3B: former assertion
	if ( tess.numVertexes >= 128 )
	{
		return qfalse;
	}

	for ( i = 0; i < tess.numVertexes; i++ )
	{
		int          j;
		unsigned int pointFlags = 0;

		R_TransformModelToClip( tess.xyz[ i ], tr.orientation.modelViewMatrix, tr.viewParms.projectionMatrix, eye, clip );

		for ( j = 0; j < 3; j++ )
		{
			if ( clip[ j ] >= clip[ 3 ] )
			{
				pointFlags |= ( 1 << ( j * 2 ) );
			}
			else if ( clip[ j ] <= -clip[ 3 ] )
			{
				pointFlags |= ( 1 << ( j * 2 + 1 ) );
			}
		}

		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	// trivially reject
	if ( pointAnd )
	{
		return qtrue;
	}

	// determine if this surface is backfaced and also determine the distance
	// to the nearest vertex so we can cull based on portal range.  Culling
	// based on vertex distance isn't 100% correct (we should be checking for
	// range to the surface), but it's good enough for the types of portals
	// we have in the game right now.
	numTriangles = tess.numIndexes / 3;

	for ( i = 0; i < tess.numIndexes; i += 3 )
	{
		vec3_t normal;
		float  dot;
		float  len;

		VectorSubtract( tess.xyz[ tess.indexes[ i ] ], tr.orientation.viewOrigin, normal );

		len = VectorLengthSquared( normal );  // lose the sqrt

		if ( len < shortest )
		{
			shortest = len;
		}

		if ( ( dot = DotProduct( normal, tess.normals[ tess.indexes[ i ] ] ) ) >= 0 )
		{
			numTriangles--;
		}
	}

	if ( !numTriangles )
	{
		//ri.Printf(PRINT_ALL, "entity portal surface triangles culled\n");
		return qtrue;
	}

	// mirrors can early out at this point, since we don't do a fade over distance
	// with them (although we could)
	if ( IsMirror( drawSurf ) )
	{
		return qfalse;
	}

	if ( shortest > ( tess.surfaceShader->portalRange * tess.surfaceShader->portalRange ) )
	{
		return qtrue;
	}

	return qfalse;
}

/*
========================
R_MirrorViewBySurface

Returns qtrue if another view has been rendered
========================
*/
static qboolean R_MirrorViewBySurface( drawSurf_t *drawSurf )
{
	vec4_t        clipDest[ 128 ];
	viewParms_t   newParms;
	viewParms_t   oldParms;
	orientation_t surface, camera;

	// don't recursively mirror
	if ( tr.viewParms.isPortal )
	{
		ri.Printf( PRINT_DEVELOPER, "WARNING: recursive mirror/portal found\n" );
		return qfalse;
	}

	if ( r_noportals->integer )
	{
		return qfalse;
	}

	// trivially reject portal/mirror
	if ( SurfIsOffscreen( drawSurf, clipDest ) )
	{
		//ri.Printf(PRINT_ALL, "WARNING: offscreen mirror/portal surface\n");
		return qfalse;
	}

	// save old viewParms so we can return to it after the mirror view
	oldParms = tr.viewParms;

	newParms = tr.viewParms;
	newParms.isPortal = qtrue;

	if ( !R_GetPortalOrientations( drawSurf, &surface, &camera, newParms.pvsOrigin, &newParms.isMirror ) )
	{
		return qfalse; // bad portal, no portalentity
	}

	R_MirrorPoint( oldParms.orientation.origin, &surface, &camera, newParms.orientation.origin );

#if 1
	VectorSubtract( vec3_origin, camera.axis[ 0 ], newParms.portalPlane.normal );
#else
	VectorCopy( camera.axis[ 0 ], newParms.portalPlane.normal );
#endif
	newParms.portalPlane.dist = DotProduct( camera.origin, newParms.portalPlane.normal );

	R_MirrorVector( oldParms.orientation.axis[ 0 ], &surface, &camera, newParms.orientation.axis[ 0 ] );
	R_MirrorVector( oldParms.orientation.axis[ 1 ], &surface, &camera, newParms.orientation.axis[ 1 ] );
	R_MirrorVector( oldParms.orientation.axis[ 2 ], &surface, &camera, newParms.orientation.axis[ 2 ] );

	// OPTIMIZE: restrict the viewport on the mirrored view

	// render the mirror view
	R_RenderView( &newParms );

	tr.viewParms = oldParms;

	return qtrue;
}

/*
=================
R_SpriteFogNum

See if a sprite is inside a fog volume
=================
*/
int R_SpriteFogNum( trRefEntity_t *ent )
{
	int   i, j;
	fog_t *fog;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		return 0;
	}

	for ( i = 1; i < tr.world->numFogs; i++ )
	{
		fog = &tr.world->fogs[ i ];

		for ( j = 0; j < 3; j++ )
		{
			if ( ent->e.origin[ j ] - ent->e.radius >= fog->bounds[ 1 ][ j ] )
			{
				break;
			}

			if ( ent->e.origin[ j ] + ent->e.radius <= fog->bounds[ 0 ][ j ] )
			{
				break;
			}
		}

		if ( j == 3 )
		{
			return i;
		}
	}

	return 0;
}

/*
=================
R_AddDrawSurf
=================
*/
void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int lightmapNum, int fogNum )
{
	int        index;
	drawSurf_t *drawSurf;

	// instead of checking for overflow, we just mask the index
	// so it wraps around
	index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;

	drawSurf = &tr.refdef.drawSurfs[ index ];

	drawSurf->entity = tr.currentEntity;
	drawSurf->surface = surface;
	drawSurf->shaderNum = shader->sortedIndex;
	drawSurf->lightmapNum = lightmapNum;
	drawSurf->fogNum = fogNum;

	tr.refdef.numDrawSurfs++;
}

/*
=================
DrawSurfCompare
compare function for qsort()
=================
*/
static int DrawSurfCompare( const void *a, const void *b )
{
#if 1

	// by shader
	if ( ( ( drawSurf_t * ) a )->shaderNum < ( ( drawSurf_t * ) b )->shaderNum )
	{
		return -1;
	}

	else if ( ( ( drawSurf_t * ) a )->shaderNum > ( ( drawSurf_t * ) b )->shaderNum )
	{
		return 1;
	}

#endif

#if 1

	// by lightmap
	if ( ( ( drawSurf_t * ) a )->lightmapNum < ( ( drawSurf_t * ) b )->lightmapNum )
	{
		return -1;
	}

	else if ( ( ( drawSurf_t * ) a )->lightmapNum > ( ( drawSurf_t * ) b )->lightmapNum )
	{
		return 1;
	}

#endif

#if 1

	// by entity
	if ( ( ( drawSurf_t * ) a )->entity == &tr.worldEntity && ( ( drawSurf_t * ) b )->entity != &tr.worldEntity )
	{
		return -1;
	}

	else if ( ( ( drawSurf_t * ) a )->entity != &tr.worldEntity && ( ( drawSurf_t * ) b )->entity == &tr.worldEntity )
	{
		return 1;
	}

	else if ( ( ( drawSurf_t * ) a )->entity < ( ( drawSurf_t * ) b )->entity )
	{
		return -1;
	}

	else if ( ( ( drawSurf_t * ) a )->entity > ( ( drawSurf_t * ) b )->entity )
	{
		return 1;
	}

#endif

#if 1

	// by fog
	if ( ( ( drawSurf_t * ) a )->fogNum < ( ( drawSurf_t * ) b )->fogNum )
	{
		return -1;
	}

	else if ( ( ( drawSurf_t * ) a )->fogNum > ( ( drawSurf_t * ) b )->fogNum )
	{
		return 1;
	}

#endif

	return 0;
}

/*
=================
R_SortDrawSurfs
=================
*/
static void R_SortDrawSurfs( void )
{
	drawSurf_t *drawSurf;
	shader_t   *shader;
	int        i;

	// it is possible for some views to not have any surfaces
	if ( tr.viewParms.numDrawSurfs < 1 )
	{
		// we still need to add it for hyperspace cases
		R_AddDrawViewCmd();
		return;
	}

	// if we overflowed MAX_DRAWSURFS, the drawsurfs
	// wrapped around in the buffer and we will be missing
	// the first surfaces, not the last ones
	if ( tr.viewParms.numDrawSurfs > MAX_DRAWSURFS )
	{
		tr.viewParms.numDrawSurfs = MAX_DRAWSURFS;
	}

	// if we overflowed MAX_INTERACTIONS, the interactions
	// wrapped around in the buffer and we will be missing
	// the first interactions, not the last ones
	if ( tr.viewParms.numInteractions > MAX_INTERACTIONS )
	{
		interaction_t *ia;

		tr.viewParms.numInteractions = MAX_INTERACTIONS;

		// reset last interaction's next pointer
		ia = &tr.viewParms.interactions[ tr.viewParms.numInteractions - 1 ];
		ia->next = NULL;
	}

	// sort the drawsurfs by sort type, then orientation, then shader
//  qsortFast(drawSurfs, numDrawSurfs, sizeof(drawSurf_t));
	qsort( tr.viewParms.drawSurfs, tr.viewParms.numDrawSurfs, sizeof( drawSurf_t ), DrawSurfCompare );

	// check for any pass through drawing, which
	// may cause another view to be rendered first
	for ( i = 0, drawSurf = tr.viewParms.drawSurfs; i < tr.viewParms.numDrawSurfs; i++, drawSurf++ )
	{
		shader = tr.sortedShaders[ drawSurf->shaderNum ];

#if 1

		if ( shader->sort > SS_PORTAL )
		{
			break;
		}

#else

		if ( !shader->isPortal )
		{
			continue;
		}

#endif

		//ri.Printf(PRINT_ALL, "portal or mirror surface\n");

		// no shader should ever have this sort type
		if ( shader->sort == SS_BAD )
		{
			ri.Error( ERR_DROP, "Shader '%s'with sort == SS_BAD", shader->name );
		}

		// if the mirror was completely clipped away, we may need to check another surface
		if ( R_MirrorViewBySurface( drawSurf ) )
		{
			// this is a debug option to see exactly what is being mirrored
			if ( r_portalOnly->integer )
			{
				return;
			}

			break; // only one mirror view at a time
		}
	}

	// tell renderer backend to render this view
	R_AddDrawViewCmd();
}

/*
=============
R_AddEntitySurfaces
=============
*/
void R_AddEntitySurfaces( void )
{
	int           i;
	trRefEntity_t *ent;
	shader_t      *shader;

	if ( !r_drawentities->integer )
	{
		return;
	}

	for ( i = 0; i < tr.refdef.numEntities; i++ )
	{
		ent = tr.currentEntity = &tr.refdef.entities[ i ];

		//
		// the weapon model must be handled special --
		// we don't want the hacked weapon position showing in
		// mirrors, because the true body position will already be drawn
		//
		if ( ( ent->e.renderfx & RF_FIRST_PERSON ) && ( tr.viewParms.isPortal || tr.viewParms.isMirror ) )
		{
			continue;
		}

		// simple generated models, like sprites and beams, are not culled
		switch ( ent->e.reType )
		{
			case RT_PORTALSURFACE:
				break; // don't draw anything

			case RT_SPRITE:
			case RT_SPLASH:
			case RT_BEAM:
			case RT_LIGHTNING:
			case RT_RAIL_CORE:
			case RT_RAIL_CORE_TAPER:
			case RT_RAIL_RINGS:

				// self blood sprites, talk balloons, etc should not be drawn in the primary
				// view.  We can't just do this check for all entities, because md3
				// entities may still want to cast shadows from them
				if ( ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal )
				{
					continue;
				}

				shader = R_GetShaderByHandle( ent->e.customShader );
				R_AddDrawSurf( &entitySurface, shader, -1, R_SpriteFogNum( ent ) );
				break;

			case RT_MODEL:
				// we must set up parts of tr.or for model culling
				R_RotateEntityForViewParms( ent, &tr.viewParms, &tr.orientation );

				tr.currentModel = R_GetModelByHandle( ent->e.hModel );

				if ( !tr.currentModel )
				{
					R_AddDrawSurf( &entitySurface, tr.defaultShader, -1, 0 );
				}
				else
				{
					switch ( tr.currentModel->type )
					{
						case MOD_MESH:
							R_AddMDVSurfaces( ent );
							break;
#if defined( COMPAT_ET )

						case MOD_MDX:
							// not a model, just a skeleton
							break;

						case MOD_MDM:
							R_MDM_AddAnimSurfaces( ent );
							break;
#endif

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )

						case MOD_MD5:
							R_AddMD5Surfaces( ent );
							break;
#endif

						case MOD_BSP:
							R_AddBSPModelSurfaces( ent );
							break;

						case MOD_BAD: // null model axis
							if ( ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal )
							{
								break;
							}

							VectorClear( ent->localBounds[ 0 ] );
							VectorClear( ent->localBounds[ 1 ] );
							VectorClear( ent->worldBounds[ 0 ] );
							VectorClear( ent->worldBounds[ 1 ] );
							shader = R_GetShaderByHandle( ent->e.customShader );
							R_AddDrawSurf( &entitySurface, tr.defaultShader, -1, 0 );
							break;

						default:
							ri.Error( ERR_DROP, "R_AddEntitySurfaces: Bad modeltype" );
					}
				}

				break;

			default:
				ri.Error( ERR_DROP, "R_AddEntitySurfaces: Bad reType" );
		}
	}
}

/*
=============
R_AddEntityInteractions
=============
*/
void R_AddEntityInteractions( trRefLight_t *light )
{
	int           i;
	trRefEntity_t *ent;

	if ( !r_drawentities->integer )
	{
		return;
	}

	for ( i = 0; i < tr.refdef.numEntities; i++ )
	{
		ent = tr.currentEntity = &tr.refdef.entities[ i ];

		//
		// the weapon model must be handled special --
		// we don't want the hacked weapon position showing in
		// mirrors, because the true body position will already be drawn
		//
		if ( ( ent->e.renderfx & RF_FIRST_PERSON ) && ( tr.viewParms.isPortal || tr.viewParms.isMirror ) )
		{
			continue;
		}

		// simple generated models, like sprites and beams, are not culled
		switch ( ent->e.reType )
		{
			case RT_PORTALSURFACE:
				break; // don't draw anything

			case RT_SPRITE:
			case RT_SPLASH:
			case RT_BEAM:
			case RT_LIGHTNING:
			case RT_RAIL_CORE:
			case RT_RAIL_CORE_TAPER:
			case RT_RAIL_RINGS:
				break;

			case RT_MODEL:
				tr.currentModel = R_GetModelByHandle( ent->e.hModel );

				if ( !tr.currentModel )
				{
					//R_AddDrawSurf(&entitySurface, tr.defaultShader, 0);
				}
				else
				{
					switch ( tr.currentModel->type )
					{
						case MOD_MESH:
							R_AddMDVInteractions( ent, light );
							break;

#if defined( COMPAT_ET )

						case MOD_MDX:
							// not a model, just a skeleton
							break;

						case MOD_MDM:
							R_AddMDMInteractions( ent, light );
							break;
#endif

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )

						case MOD_MD5:
							R_AddMD5Interactions( ent, light );
							break;
#endif

						case MOD_BSP:
							R_AddBrushModelInteractions( ent, light );
							break;

						case MOD_BAD: // null model axis
							break;

						default:
							ri.Error( ERR_DROP, "R_AddEntityInteractions: Bad modeltype" );
					}
				}

				break;

			default:
				ri.Error( ERR_DROP, "R_AddEntityInteractions: Bad reType" );
		}
	}
}

/*
=====================
R_AddPolygonInteractions
=====================
*/
void R_AddPolygonInteractions( trRefLight_t *light )
{
	int       i, j;
	shader_t  *shader;
	srfPoly_t *poly;
	vec3_t    worldBounds[ 2 ];

	if ( light->l.inverseShadows )
	{
		return;
	}

	tr.currentEntity = &tr.worldEntity;

	for ( i = 0, poly = tr.refdef.polys; i < tr.refdef.numPolys; i++, poly++ )
	{
		shader = R_GetShaderByHandle( poly->hShader );

		if ( !shader->interactLight )
		{
			continue;
		}

		// calc AABB of the triangle
		ClearBounds( worldBounds[ 0 ], worldBounds[ 1 ] );

		for ( j = 0; j < poly->numVerts; j++ )
		{
			AddPointToBounds( poly->verts[ j ].xyz, worldBounds[ 0 ], worldBounds[ 1 ] );
		}

		// do a quick AABB cull
		if ( !BoundsIntersect( light->worldBounds[ 0 ], light->worldBounds[ 1 ], worldBounds[ 0 ], worldBounds[ 1 ] ) )
		{
			continue;
		}

		R_AddLightInteraction( light, ( void * ) poly, shader, CUBESIDE_CLIPALL, IA_LIGHTONLY );
	}
}

/*
=============
R_AddLightInteractions
=============
*/
void R_AddLightInteractions( void )
{
	int          i;
	trRefLight_t *light;
	//bspNode_t     **leafs; //unused
	bspNode_t    *leaf;
	link_t       *l; //, *sentinel; //unused

	for ( i = 0; i < tr.refdef.numLights; i++ )
	{
		light = tr.currentLight = &tr.refdef.lights[ i ];

		if ( light->isStatic )
		{
			if ( !r_staticLight->integer || ( ( r_precomputedLighting->integer || r_vertexLighting->integer ) && !light->noRadiosity ) )
			{
				light->cull = CULL_OUT;
				continue;
			}
		}
		else
		{
			if ( !r_dynamicLight->integer )
			{
				light->cull = CULL_OUT;
				continue;
			}
		}

		// we must set up parts of tr.or for light culling
		R_RotateLightForViewParms( light, &tr.viewParms, &tr.orientation );

		// calc local bounds for culling
		if ( light->isStatic )
		{
#if 1

			// ignore if not in PVS
			if ( !r_noLightVisCull->integer )
			{
				if ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicBspOcclusionCulling->integer )
				{
					int numVisibleLeafs = 0;

					for ( l = light->leafs.next; l != &light->leafs; l = l->next )
					{
						if ( !l || !l->data )
						{
							// something odd happens with the prev/next pointers if ri.Hunk_Alloc was used
							break;
						}

						leaf = ( bspNode_t * ) l->data;

						//ri.Printf(PRINT_ALL, "leaf %i: visible = %i, %i\n", leaf - tr.world->nodes, leaf->visible[tr.viewCount], tr.viewCount);

						if ( leaf->visible[ tr.viewCount ] && ( tr.frameCount - leaf->lastVisited[ tr.viewCount ] ) <= r_chcMaxVisibleFrames->integer )
						{
							numVisibleLeafs++;
						}
					}

					if ( numVisibleLeafs == 0 )
					{
						tr.pc.c_pvs_cull_light_out++;
						light->cull = CULL_OUT;
						continue;
					}
				}
				else
				{
					for ( l = light->leafs.next; l != &light->leafs; l = l->next )
					{
						if ( !l || !l->data )
						{
							// something odd happens with the prev/next pointers if ri.Hunk_Alloc was used
							break;
						}

						leaf = ( bspNode_t * ) l->data;

						if ( leaf->visCounts[ tr.visIndex ] == tr.visCounts[ tr.visIndex ] )
						{
							light->visCounts[ tr.visIndex ] = tr.visCounts[ tr.visIndex ];
						}
					}

					if ( light->visCounts[ tr.visIndex ] != tr.visCounts[ tr.visIndex ] )
					{
						tr.pc.c_pvs_cull_light_out++;
						light->cull = CULL_OUT;
						continue;
					}
				}
			}

			// look if we have to draw the light including its interactions
			switch ( R_CullLocalBox( light->localBounds ) )
			{
				case CULL_IN:
				default:
					tr.pc.c_box_cull_light_in++;
					light->cull = CULL_IN;
					break;

				case CULL_CLIP:
					tr.pc.c_box_cull_light_clip++;
					light->cull = CULL_CLIP;
					break;

				case CULL_OUT:
					// light is not visible so skip other light setup stuff to save speed
					tr.pc.c_box_cull_light_out++;
					light->cull = CULL_OUT;
					continue;
			}

#endif
		}
		else
		{
			// set up light transform matrix
			MatrixSetupTransformFromQuat( light->transformMatrix, light->l.rotation, light->l.origin );

			// set up light origin for lighting and shadowing
			R_SetupLightOrigin( light );

			// set up model to light view matrix
			R_SetupLightView( light );

			// set up projection
			R_SetupLightProjection( light );

			// calc local bounds for culling
			R_SetupLightLocalBounds( light );

			// look if we have to draw the light including its interactions
			switch ( R_CullLocalBox( light->localBounds ) )
			{
				case CULL_IN:
				default:
					tr.pc.c_box_cull_light_in++;
					light->cull = CULL_IN;
					break;

				case CULL_CLIP:
					tr.pc.c_box_cull_light_clip++;
					light->cull = CULL_CLIP;
					break;

				case CULL_OUT:
					// light is not visible so skip other light setup stuff to save speed
					tr.pc.c_box_cull_light_out++;
					light->cull = CULL_OUT;
					continue;
			}

			// setup world bounds for intersection tests
			R_SetupLightWorldBounds( light );

			// setup frustum planes for intersection tests
			R_SetupLightFrustum( light );

			// ignore if not in visible bounds
			if ( !BoundsIntersect
			     ( light->worldBounds[ 0 ], light->worldBounds[ 1 ], tr.viewParms.visBounds[ 0 ], tr.viewParms.visBounds[ 1 ] ) )
			{
				light->cull = CULL_OUT;
				continue;
			}
		}

		// set up view dependent light scissor
		R_SetupLightScissor( light );

		// set up view dependent light Level of Detail
		R_SetupLightLOD( light );

		// look for proper attenuation shader
		R_SetupLightShader( light );

		// setup interactions
		light->firstInteraction = NULL;
		light->lastInteraction = NULL;

		light->numInteractions = 0;
		light->numShadowOnlyInteractions = 0;
		light->numLightOnlyInteractions = 0;
		light->noSort = qfalse;

		if ( r_deferredShading->integer && r_shadows->integer < SHADOWING_ESM16 )
		{
			// add one fake interaction for this light
			// because the renderer backend only loops through interactions
			R_AddLightInteraction( light, NULL, NULL, CUBESIDE_CLIPALL, IA_DEFAULT );
		}
		else
		{
			if ( light->isStatic )
			{
				R_AddPrecachedWorldInteractions( light );
			}
			else
			{
				R_AddWorldInteractions( light );
			}

			R_AddEntityInteractions( light );

			// Tr3B: fun but slow
			//R_AddPolygonInteractions(light);
		}

		if ( light->numInteractions && light->numInteractions != light->numShadowOnlyInteractions )
		{
			R_SortInteractions( light );

			if ( light->isStatic )
			{
				tr.pc.c_slights++;
			}
			else
			{
				tr.pc.c_dlights++;
			}
		}
		else
		{
			// skip all interactions of this light because it caused only shadow volumes
			// but no lighting
			tr.refdef.numInteractions -= light->numInteractions;
			light->cull = CULL_OUT;
		}
	}
}

void R_AddLightBoundsToVisBounds( void )
{
	int          i;
	trRefLight_t *light;
	//bspNode_t     **leafs; //unused
	bspNode_t    *leaf;
	link_t       *l; //, *sentinel; //unused

	for ( i = 0; i < tr.refdef.numLights; i++ )
	{
		light = tr.currentLight = &tr.refdef.lights[ i ];

		if ( light->isStatic )
		{
			if ( !r_staticLight->integer || ( ( r_precomputedLighting->integer || r_vertexLighting->integer ) && !light->noRadiosity ) )
			{
				//light->cull = CULL_OUT;
				continue;
			}
		}
		else
		{
			if ( !r_dynamicLight->integer )
			{
				//light->cull = CULL_OUT;
				continue;
			}
		}

		// we must set up parts of tr.or for light culling
		R_RotateLightForViewParms( light, &tr.viewParms, &tr.orientation );

		// calc local bounds for culling
		if ( light->isStatic )
		{
			// ignore if not in PVS
			if ( !r_noLightVisCull->integer )
			{
				if ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicBspOcclusionCulling->integer )
				{
					int numVisibleLeafs = 0;

					for ( l = light->leafs.next; l != &light->leafs; l = l->next )
					{
						if ( !l || !l->data )
						{
							// something odd happens with the prev/next pointers if ri.Hunk_Alloc was used
							break;
						}

						leaf = ( bspNode_t * ) l->data;

						//ri.Printf(PRINT_ALL, "leaf %i: visible = %i, %i\n", leaf - tr.world->nodes, leaf->visible[tr.viewCount], tr.viewCount);

						if ( leaf->visible[ tr.viewCount ] && ( tr.frameCount - leaf->lastVisited[ tr.viewCount ] ) <= r_chcMaxVisibleFrames->integer )
						{
							numVisibleLeafs++;
						}
					}

					if ( numVisibleLeafs == 0 )
					{
						//tr.pc.c_pvs_cull_light_out++;
						//light->cull = CULL_OUT;
						continue;
					}
				}
				else
				{
					for ( l = light->leafs.next; l != &light->leafs; l = l->next )
					{
						if ( !l || !l->data )
						{
							// something odd happens with the prev/next pointers if ri.Hunk_Alloc was used
							break;
						}

						leaf = ( bspNode_t * ) l->data;

						if ( leaf->visCounts[ tr.visIndex ] == tr.visCounts[ tr.visIndex ] )
						{
							light->visCounts[ tr.visIndex ] = tr.visCounts[ tr.visIndex ];
						}
					}

					if ( light->visCounts[ tr.visIndex ] != tr.visCounts[ tr.visIndex ] )
					{
						//tr.pc.c_pvs_cull_light_out++;
						//light->cull = CULL_OUT;
						continue;
					}
				}
			}

			// look if we have to draw the light including its interactions
			switch ( R_CullLocalBox( light->localBounds ) )
			{
				case CULL_IN:
				default:
					break;

				case CULL_CLIP:
					break;

				case CULL_OUT:
					continue;
			}
		}
		else
		{
			// set up light transform matrix
			MatrixSetupTransformFromQuat( light->transformMatrix, light->l.rotation, light->l.origin );

			// set up light origin for lighting and shadowing
			R_SetupLightOrigin( light );

			// set up model to light view matrix
			R_SetupLightView( light );

			// set up projection
			R_SetupLightProjection( light );

			// calc local bounds for culling
			R_SetupLightLocalBounds( light );

			// look if we have to draw the light including its interactions
			switch ( R_CullLocalBox( light->localBounds ) )
			{
				case CULL_IN:
				default:
					break;

				case CULL_CLIP:
					break;

				case CULL_OUT:
					continue;
			}

			// setup world bounds for intersection tests
			R_SetupLightWorldBounds( light );
		}

		// add to z buffer bounds
		if ( light->worldBounds[ 0 ][ 0 ] < tr.viewParms.visBounds[ 0 ][ 0 ] )
		{
			tr.viewParms.visBounds[ 0 ][ 0 ] = light->worldBounds[ 0 ][ 0 ];
		}

		if ( light->worldBounds[ 0 ][ 1 ] < tr.viewParms.visBounds[ 0 ][ 1 ] )
		{
			tr.viewParms.visBounds[ 0 ][ 1 ] = light->worldBounds[ 0 ][ 1 ];
		}

		if ( light->worldBounds[ 0 ][ 2 ] < tr.viewParms.visBounds[ 0 ][ 2 ] )
		{
			tr.viewParms.visBounds[ 0 ][ 2 ] = light->worldBounds[ 0 ][ 2 ];
		}

		if ( light->worldBounds[ 1 ][ 0 ] > tr.viewParms.visBounds[ 1 ][ 0 ] )
		{
			tr.viewParms.visBounds[ 1 ][ 0 ] = light->worldBounds[ 1 ][ 0 ];
		}

		if ( light->worldBounds[ 1 ][ 1 ] > tr.viewParms.visBounds[ 1 ][ 1 ] )
		{
			tr.viewParms.visBounds[ 1 ][ 1 ] = light->worldBounds[ 1 ][ 1 ];
		}

		if ( light->worldBounds[ 1 ][ 2 ] > tr.viewParms.visBounds[ 1 ][ 2 ] )
		{
			tr.viewParms.visBounds[ 1 ][ 2 ] = light->worldBounds[ 1 ][ 2 ];
		}
	}
}

void R_DebugAxis( const vec3_t origin, const matrix_t transformMatrix )
{
#if 0
	vec3_t forward, left, up;

	MatrixToVectorsFLU( transformMatrix, forward, left, up );
	VectorMA( origin, 16, forward, forward );
	VectorMA( origin, 16, left, left );
	VectorMA( origin, 16, up, up );

	// draw axis
	glLineWidth( 3 );
	glBegin( GL_LINES );

	glColor3f( 1, 0, 0 );
	glVertex3fv( origin );
	glVertex3fv( forward );

	glColor3f( 0, 1, 0 );
	glVertex3fv( origin );
	glVertex3fv( left );

	glColor3f( 0, 0, 1 );
	glVertex3fv( origin );
	glVertex3fv( up );

	glEnd();
	glLineWidth( 1 );
#endif
}

// Tr3B - from botlib
void R_DebugBoundingBox( const vec3_t origin, const vec3_t mins, const vec3_t maxs, vec4_t color )
{
#if 0
	vec3_t corners[ 8 ];
	int    i;

	// upper corners
	corners[ 0 ][ 0 ] = origin[ 0 ] + maxs[ 0 ];
	corners[ 0 ][ 1 ] = origin[ 1 ] + maxs[ 1 ];
	corners[ 0 ][ 2 ] = origin[ 2 ] + maxs[ 2 ];

	corners[ 1 ][ 0 ] = origin[ 0 ] + mins[ 0 ];
	corners[ 1 ][ 1 ] = origin[ 1 ] + maxs[ 1 ];
	corners[ 1 ][ 2 ] = origin[ 2 ] + maxs[ 2 ];

	corners[ 2 ][ 0 ] = origin[ 0 ] + mins[ 0 ];
	corners[ 2 ][ 1 ] = origin[ 1 ] + mins[ 1 ];
	corners[ 2 ][ 2 ] = origin[ 2 ] + maxs[ 2 ];

	corners[ 3 ][ 0 ] = origin[ 0 ] + maxs[ 0 ];
	corners[ 3 ][ 1 ] = origin[ 1 ] + mins[ 1 ];
	corners[ 3 ][ 2 ] = origin[ 2 ] + maxs[ 2 ];

	// lower corners
	Com_Memcpy( corners[ 4 ], corners[ 0 ], sizeof( vec3_t ) * 4 );

	for ( i = 0; i < 4; i++ )
	{
		corners[ 4 + i ][ 2 ] = origin[ 2 ] + mins[ 2 ];
	}

	// draw bounding box
	glBegin( GL_LINES );
	glVertexAttrib4fv( ATTR_INDEX_COLOR, color );

	for ( i = 0; i < 4; i++ )
	{
		// top plane
		glVertex3fv( corners[ i ] );
		glVertex3fv( corners[( i + 1 ) & 3 ] );

		// bottom plane
		glVertex3fv( corners[ 4 + i ] );
		glVertex3fv( corners[ 4 + ( ( i + 1 ) & 3 ) ] );

		// vertical lines
		glVertex3fv( corners[ i ] );
		glVertex3fv( corners[ 4 + i ] );
	}

	glEnd();
#endif
}

/*
================
R_DebugPolygon
================
*/
void R_DebugPolygon( int color, int numPoints, float *points )
{
#if 0
	int i;

	GL_BindProgram( 0 );
	GL_VertexAttribsState( ATTR_DEFAULT );
	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	// draw solid shade
	glColor3f( color & 1, ( color >> 1 ) & 1, ( color >> 2 ) & 1 );
	glBegin( GL_POLYGON );

	for ( i = 0; i < numPoints; i++ )
	{
		glVertex3fv( points + i * 3 );
	}

	glEnd();

	// draw wireframe outline
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	glDepthRange( 0, 0 );
	glColor3f( 1, 1, 1 );
	glBegin( GL_POLYGON );

	for ( i = 0; i < numPoints; i++ )
	{
		glVertex3fv( points + i * 3 );
	}

	glEnd();
	glDepthRange( 0, 1 );
#endif
}

/*
================
R_DebugText
================
*/
void R_DebugText( const vec3_t org, float r, float g, float b, const char *text, qboolean neverOcclude )
{
#if 0

	if ( neverOcclude )
	{
		glDepthRange( 0, 0 );  // never occluded
	}

	glColor3f( r, g, b );
	glRasterPos3fv( org );
	glPushAttrib( GL_LIST_BIT );
	glListBase( gl_NormalFontBase );
	glCallLists( strlen( text ), GL_UNSIGNED_BYTE, text );
	glListBase( 0 );
	glPopAttrib();

	if ( neverOcclude )
	{
		glDepthRange( 0, 1 );
	}

#endif
}

/*
====================
R_DebugGraphics

Visualization aid for movement clipping debugging
====================
*/
static void R_DebugGraphics( void )
{
#if defined( USE_D3D10 )
	// TODO
#else

	if ( r_debugSurface->integer )
	{
		// the render thread can't make callbacks to the main thread
		R_SyncRenderThread();

		GL_BindProgram( 0 );
		GL_SelectTexture( 0 );
		GL_Bind( tr.whiteImage );
		GL_Cull( CT_FRONT_SIDED );
		ri.CM_DrawDebugSurface( R_DebugPolygon );
	}

#endif
}

/*
================
R_RenderView

A view may be either the actual camera view,
or a mirror / remote location
================
*/
void R_RenderView( viewParms_t *parms )
{
	int      firstDrawSurf;
	int      firstInteraction;
	matrix_t mvp;

	if ( parms->viewportWidth <= 0 || parms->viewportHeight <= 0 )
	{
		return;
	}

	tr.viewCount++;
	tr.viewCountNoReset++;

	if ( tr.viewCount >= MAX_VIEWS )
	{
		ri.Printf( PRINT_ALL, "MAX_VIEWS (%i) hit. Don't add more mirrors or portals. Skipping view ...\n", MAX_VIEWS );
		return;
	}

	tr.viewParms = *parms;
	tr.viewParms.frameSceneNum = tr.frameSceneNum;
	tr.viewParms.frameCount = tr.frameCount;
	tr.viewParms.viewCount = tr.viewCount; // % MAX_VIEWS;

	firstDrawSurf = tr.refdef.numDrawSurfs;
	firstInteraction = tr.refdef.numInteractions;

	// set viewParms.world
	R_RotateForViewer();

	// set the projection matrix with the far clip plane set at infinity
	// this required for the CHC++ algorithm
	R_SetupProjection( qtrue );

	R_SetupFrustum();

	// RB: cull decal projects before calling R_AddWorldSurfaces
	// because it requires the decalBits
	R_CullDecalProjectors();

	R_AddWorldSurfaces();

	R_AddPolygonSurfaces();

	R_AddPolygonBufferSurfaces();

	// we have tr.viewParms.visBounds set and now we need to add the light bounds
	// or we get wrong occlusion query results
	R_AddLightBoundsToVisBounds();

	// set the projection matrix now that we have the world bounded
	// this needs to be done before entities are
	// added, because they use the projection
	// matrix for lod calculation
	R_SetupProjection( qfalse );

#if defined( COMPAT_ET )
	R_SetFrameFog();
#endif

	R_SetupUnprojection();

	// set camera frustum planes in world space again, but this time including the far plane
	tr.orientation = tr.viewParms.world;
	MatrixMultiply( tr.viewParms.projectionMatrix, tr.orientation.modelViewMatrix, mvp );
	R_SetupFrustum2( tr.viewParms.frustums[ 0 ], mvp );

	// for parallel split shadow mapping
	R_SetupSplitFrustums();

	//R_AddWorldSurfaces();

	R_AddEntitySurfaces();

	R_AddLightInteractions();

	if( tr.refdef.blurVec[0] != 0.0f ||
	    tr.refdef.blurVec[1] != 0.0f ||
	    tr.refdef.blurVec[2] != 0.0f ) {
		MatrixTransformNormal2( tr.orientation.viewMatrix,
					tr.refdef.blurVec );
	}

	tr.viewParms.drawSurfs = tr.refdef.drawSurfs + firstDrawSurf;
	tr.viewParms.numDrawSurfs = tr.refdef.numDrawSurfs - firstDrawSurf;

	tr.viewParms.interactions = tr.refdef.interactions + firstInteraction;
	tr.viewParms.numInteractions = tr.refdef.numInteractions - firstInteraction;

	R_SortDrawSurfs();

	// draw main system development information (surface outlines, etc)
	R_DebugGraphics();
}
