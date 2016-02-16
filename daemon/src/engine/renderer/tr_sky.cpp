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
// tr_sky.c
#include "tr_local.h"
#include "gl_shader.h"

static const int SKY_SUBDIVISIONS      = 8;
static const int HALF_SKY_SUBDIVISIONS = ( SKY_SUBDIVISIONS / 2 );

static float s_cloudTexCoords[ 6 ][ SKY_SUBDIVISIONS + 1 ][ SKY_SUBDIVISIONS + 1 ][ 2 ];
static float s_cloudTexP[ 6 ][ SKY_SUBDIVISIONS + 1 ][ SKY_SUBDIVISIONS + 1 ];

/*
===================================================================================

POLYGON TO BOX SIDE PROJECTION

===================================================================================
*/

static const vec3_t sky_clip[ 6 ] =
{
	{ 1,  1,  0 },
	{ 1,  -1, 0 },
	{ 0,  -1, 1 },
	{ 0,  1,  1 },
	{ 1,  0,  1 },
	{ -1, 0,  1 }
};

static float  sky_mins[ 2 ][ 6 ], sky_maxs[ 2 ][ 6 ];
static float  sky_min, sky_max;

/*
================
AddSkyPolygon
================
*/
static void AddSkyPolygon( int nump, vec3_t vecs )
{
	int        i, j;
	vec3_t     v, av;
	float      s, t, dv;
	int        axis;
	float      *vp;

	static const int vec_to_st[ 6 ][ 3 ] =
	{
		{ -2, 3,  1  },
		{ 2,  3,  -1 },

		{ 1,  3,  2  },
		{ -1, 3,  -2 },

		{ -2, -1, 3  },
		{ -2, 1,  -3 }

		//  {-1,2,3},
		//  {1,2,-3}
	};

	// decide which face it maps to
	VectorCopy( vec3_origin, v );

	for ( i = 0, vp = vecs; i < nump; i++, vp += 3 )
	{
		VectorAdd( vp, v, v );
	}

	av[ 0 ] = fabs( v[ 0 ] );
	av[ 1 ] = fabs( v[ 1 ] );
	av[ 2 ] = fabs( v[ 2 ] );

	if ( av[ 0 ] > av[ 1 ] && av[ 0 ] > av[ 2 ] )
	{
		if ( v[ 0 ] < 0 )
		{
			axis = 1;
		}
		else
		{
			axis = 0;
		}
	}
	else if ( av[ 1 ] > av[ 2 ] && av[ 1 ] > av[ 0 ] )
	{
		if ( v[ 1 ] < 0 )
		{
			axis = 3;
		}
		else
		{
			axis = 2;
		}
	}
	else
	{
		if ( v[ 2 ] < 0 )
		{
			axis = 5;
		}
		else
		{
			axis = 4;
		}
	}

	// project new texture coords
	for ( i = 0; i < nump; i++, vecs += 3 )
	{
		j = vec_to_st[ axis ][ 2 ];

		if ( j > 0 )
		{
			dv = vecs[ j - 1 ];
		}
		else
		{
			dv = -vecs[ -j - 1 ];
		}

		if ( dv < 0.001 )
		{
			continue; // don't divide by zero
		}

		j = vec_to_st[ axis ][ 0 ];

		if ( j < 0 )
		{
			s = -vecs[ -j - 1 ] / dv;
		}
		else
		{
			s = vecs[ j - 1 ] / dv;
		}

		j = vec_to_st[ axis ][ 1 ];

		if ( j < 0 )
		{
			t = -vecs[ -j - 1 ] / dv;
		}
		else
		{
			t = vecs[ j - 1 ] / dv;
		}

		if ( s < sky_mins[ 0 ][ axis ] )
		{
			sky_mins[ 0 ][ axis ] = s;
		}

		if ( t < sky_mins[ 1 ][ axis ] )
		{
			sky_mins[ 1 ][ axis ] = t;
		}

		if ( s > sky_maxs[ 0 ][ axis ] )
		{
			sky_maxs[ 0 ][ axis ] = s;
		}

		if ( t > sky_maxs[ 1 ][ axis ] )
		{
			sky_maxs[ 1 ][ axis ] = t;
		}
	}
}

static const float ON_EPSILON     = 0.1f; // point on plane side epsilon
static const int MAX_CLIP_VERTS = 64;

/*
================
ClipSkyPolygon
================
*/
static void ClipSkyPolygon( int nump, vec3_t vecs, int stage )
{
	const float *norm;
	float    *v;
	bool front, back;
	float    d, e;
	float    dists[ MAX_CLIP_VERTS ];
	planeSide_t sides[ MAX_CLIP_VERTS ];
	vec3_t   newv[ 2 ][ MAX_CLIP_VERTS ];
	int      newc[ 2 ];
	int      i, j;

	if ( nump > MAX_CLIP_VERTS - 2 )
	{
		ri.Error(errorParm_t::ERR_DROP, "ClipSkyPolygon: MAX_CLIP_VERTS" );
	}

	if ( stage == 6 )
	{
		// fully clipped, so draw it
		AddSkyPolygon( nump, vecs );
		return;
	}

	front = back = false;
	norm = sky_clip[ stage ];

	for ( i = 0, v = vecs; i < nump; i++, v += 3 )
	{
		d = DotProduct( v, norm );

		if ( d > ON_EPSILON )
		{
			front = true;
			sides[ i ] = planeSide_t::SIDE_FRONT;
		}
		else if ( d < -ON_EPSILON )
		{
			back = true;
			sides[ i ] = planeSide_t::SIDE_BACK;
		}
		else
		{
			sides[ i ] = planeSide_t::SIDE_ON;
		}

		dists[ i ] = d;
	}

	if ( !front || !back )
	{
		// not clipped
		ClipSkyPolygon( nump, vecs, stage + 1 );
		return;
	}

	// clip it
	sides[ i ] = sides[ 0 ];
	dists[ i ] = dists[ 0 ];
	VectorCopy( vecs, ( vecs + ( i * 3 ) ) );
	newc[ 0 ] = newc[ 1 ] = 0;

	for ( i = 0, v = vecs; i < nump; i++, v += 3 )
	{
		switch ( sides[ i ] )
		{
			case planeSide_t::SIDE_CROSS:
				break;
			case planeSide_t::SIDE_FRONT:
				VectorCopy( v, newv[ 0 ][ newc[ 0 ] ] );
				newc[ 0 ]++;
				break;

			case planeSide_t::SIDE_BACK:
				VectorCopy( v, newv[ 1 ][ newc[ 1 ] ] );
				newc[ 1 ]++;
				break;

			case planeSide_t::SIDE_ON:
				VectorCopy( v, newv[ 0 ][ newc[ 0 ] ] );
				newc[ 0 ]++;
				VectorCopy( v, newv[ 1 ][ newc[ 1 ] ] );
				newc[ 1 ]++;
				break;
		}

		if ( sides[ i ] == planeSide_t::SIDE_ON || sides[ i + 1 ] == planeSide_t::SIDE_ON || sides[ i + 1 ] == sides[ i ] )
		{
			continue;
		}

		d = dists[ i ] / ( dists[ i ] - dists[ i + 1 ] );

		for ( j = 0; j < 3; j++ )
		{
			e = v[ j ] + d * ( v[ j + 3 ] - v[ j ] );
			newv[ 0 ][ newc[ 0 ] ][ j ] = e;
			newv[ 1 ][ newc[ 1 ] ][ j ] = e;
		}

		newc[ 0 ]++;
		newc[ 1 ]++;
	}

	// continue
	ClipSkyPolygon( newc[ 0 ], newv[ 0 ][ 0 ], stage + 1 );
	ClipSkyPolygon( newc[ 1 ], newv[ 1 ][ 0 ], stage + 1 );
}

/*
==============
ClearSkyBox
==============
*/
static void ClearSkyBox()
{
	int i;

	for ( i = 0; i < 6; i++ )
	{
		sky_mins[ 0 ][ i ] = sky_mins[ 1 ][ i ] = 9999;
		sky_maxs[ 0 ][ i ] = sky_maxs[ 1 ][ i ] = -9999;
	}
}

/*
================
Tess_ClipSkyPolygons
================
*/
void Tess_ClipSkyPolygons()
{
	vec3_t p[ 5 ]; // need one extra point for clipping
	unsigned int i, j;

	ClearSkyBox();

	for ( i = 0; i < tess.numIndexes; i += 3 )
	{
		for ( j = 0; j < 3; j++ )
		{
			VectorSubtract( tess.verts[ tess.indexes[ i + j ] ].xyz, backEnd.viewParms.orientation.origin, p[ j ] );
		}

		ClipSkyPolygon( 3, p[ 0 ], 0 );
	}
}

/*
===================================================================================

CLOUD VERTEX GENERATION

===================================================================================
*/

/*
** MakeSkyVec
**
** Parms: s, t range from -1 to 1
*/
static void MakeSkyVec( float s, float t, int axis, vec2_t outSt, vec4_t outXYZ )
{
	// 1 = s, 2 = t, 3 = 2048
	static const int st_to_vec[ 6 ][ 3 ] =
	{
		{ 3,  -1, 2 },
		{ -3, 1,  2 },

		{ 1,  3,  2 },
		{ -1, -3, 2 },

		{ -2, -1, 3 }, // 0 degrees yaw, look straight up
		{ 2,  -1, -3} // look straight down
	};

	vec3_t     b;
	int        j, k;
	float      boxSize;

	boxSize = backEnd.viewParms.zFar / 1.75; // div sqrt(3)
	b[ 0 ] = s * boxSize;
	b[ 1 ] = t * boxSize;
	b[ 2 ] = boxSize;

	for ( j = 0; j < 3; j++ )
	{
		k = st_to_vec[ axis ][ j ];

		if ( k < 0 )
		{
			outXYZ[ j ] = -b[ -k - 1 ];
		}
		else
		{
			outXYZ[ j ] = b[ k - 1 ];
		}
	}

	outXYZ[ 3 ] = 1;

	// avoid bilerp seam
	s = ( s + 1 ) * 0.5;
	t = ( t + 1 ) * 0.5;

	if ( s < sky_min )
	{
		s = sky_min;
	}
	else if ( s > sky_max )
	{
		s = sky_max;
	}

	if ( t < sky_min )
	{
		t = sky_min;
	}
	else if ( t > sky_max )
	{
		t = sky_max;
	}

	t = 1.0 - t;

	if ( outSt )
	{
		outSt[ 0 ] = s;
		outSt[ 1 ] = t;
	}
}

static vec4_t s_skyPoints[ SKY_SUBDIVISIONS + 1 ][ SKY_SUBDIVISIONS + 1 ];
static vec2_t s_skyTexCoords[ SKY_SUBDIVISIONS + 1 ][ SKY_SUBDIVISIONS + 1 ];

static void FillCloudySkySide( const int mins[ 2 ], const int maxs[ 2 ], bool addIndexes )
{
	int s, t;
	int vertexStart = tess.numVertexes;
	int tHeight, sWidth;

	tHeight = maxs[ 1 ] - mins[ 1 ] + 1;
	sWidth = maxs[ 0 ] - mins[ 0 ] + 1;

	for ( t = mins[ 1 ] + HALF_SKY_SUBDIVISIONS; t <= maxs[ 1 ] + HALF_SKY_SUBDIVISIONS; t++ )
	{
		for ( s = mins[ 0 ] + HALF_SKY_SUBDIVISIONS; s <= maxs[ 0 ] + HALF_SKY_SUBDIVISIONS; s++ )
		{
			VectorAdd( s_skyPoints[ t ][ s ], backEnd.viewParms.orientation.origin, tess.verts[ tess.numVertexes ].xyz );

			tess.verts[ tess.numVertexes ].texCoords[ 0 ] = floatToHalf( s_skyTexCoords[ t ][ s ][ 0 ] );
			tess.verts[ tess.numVertexes ].texCoords[ 1 ] = floatToHalf( s_skyTexCoords[ t ][ s ][ 1 ] );

			tess.numVertexes++;

			if ( tess.numVertexes >= SHADER_MAX_VERTEXES )
			{
				ri.Error(errorParm_t::ERR_DROP, "SHADER_MAX_VERTEXES hit in FillCloudySkySide()" );
			}
		}
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD;

	// only add indexes for one pass, otherwise it would draw multiple times for each pass
	if ( addIndexes )
	{
		for ( t = 0; t < tHeight - 1; t++ )
		{
			for ( s = 0; s < sWidth - 1; s++ )
			{
				tess.indexes[ tess.numIndexes ] = vertexStart + s + t * ( sWidth );
				tess.numIndexes++;
				tess.indexes[ tess.numIndexes ] = vertexStart + s + ( t + 1 ) * ( sWidth );
				tess.numIndexes++;
				tess.indexes[ tess.numIndexes ] = vertexStart + s + 1 + t * ( sWidth );
				tess.numIndexes++;

				tess.indexes[ tess.numIndexes ] = vertexStart + s + ( t + 1 ) * ( sWidth );
				tess.numIndexes++;
				tess.indexes[ tess.numIndexes ] = vertexStart + s + 1 + ( t + 1 ) * ( sWidth );
				tess.numIndexes++;
				tess.indexes[ tess.numIndexes ] = vertexStart + s + 1 + t * ( sWidth );
				tess.numIndexes++;
			}
		}
	}
}

static void DrawSkyBox()
{
	int i;

	sky_min = 0;
	sky_max = 1;

	Com_Memset( s_skyTexCoords, 0, sizeof( s_skyTexCoords ) );

	// set up for drawing
	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.attribsSet = 0;

	GL_State( GLS_DEFAULT );

	Tess_MapVBOs( false );

	for ( i = 0; i < 6; i++ )
	{
		int sky_mins_subd[ 2 ], sky_maxs_subd[ 2 ];
		int s, t;

		sky_mins[ 0 ][ i ] = floor( sky_mins[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_mins[ 1 ][ i ] = floor( sky_mins[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[ 0 ][ i ] = ceil( sky_maxs[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[ 1 ][ i ] = ceil( sky_maxs[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;

		if ( ( sky_mins[ 0 ][ i ] >= sky_maxs[ 0 ][ i ] ) || ( sky_mins[ 1 ][ i ] >= sky_maxs[ 1 ][ i ] ) )
		{
			continue;
		}

		sky_mins_subd[ 0 ] = sky_mins[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS;
		sky_mins_subd[ 1 ] = sky_mins[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[ 0 ] = sky_maxs[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[ 1 ] = sky_maxs[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS;

		if ( sky_mins_subd[ 0 ] < -HALF_SKY_SUBDIVISIONS )
		{
			sky_mins_subd[ 0 ] = -HALF_SKY_SUBDIVISIONS;
		}
		else if ( sky_mins_subd[ 0 ] > HALF_SKY_SUBDIVISIONS )
		{
			sky_mins_subd[ 0 ] = HALF_SKY_SUBDIVISIONS;
		}

		if ( sky_mins_subd[ 1 ] < -HALF_SKY_SUBDIVISIONS )
		{
			sky_mins_subd[ 1 ] = -HALF_SKY_SUBDIVISIONS;
		}
		else if ( sky_mins_subd[ 1 ] > HALF_SKY_SUBDIVISIONS )
		{
			sky_mins_subd[ 1 ] = HALF_SKY_SUBDIVISIONS;
		}

		if ( sky_maxs_subd[ 0 ] < -HALF_SKY_SUBDIVISIONS )
		{
			sky_maxs_subd[ 0 ] = -HALF_SKY_SUBDIVISIONS;
		}
		else if ( sky_maxs_subd[ 0 ] > HALF_SKY_SUBDIVISIONS )
		{
			sky_maxs_subd[ 0 ] = HALF_SKY_SUBDIVISIONS;
		}

		if ( sky_maxs_subd[ 1 ] < -HALF_SKY_SUBDIVISIONS )
		{
			sky_maxs_subd[ 1 ] = -HALF_SKY_SUBDIVISIONS;
		}
		else if ( sky_maxs_subd[ 1 ] > HALF_SKY_SUBDIVISIONS )
		{
			sky_maxs_subd[ 1 ] = HALF_SKY_SUBDIVISIONS;
		}

		// iterate through the subdivisions
		for ( t = sky_mins_subd[ 1 ] + HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[ 1 ] + HALF_SKY_SUBDIVISIONS; t++ )
		{
			for ( s = sky_mins_subd[ 0 ] + HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[ 0 ] + HALF_SKY_SUBDIVISIONS; s++ )
			{
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS,
				            ( t - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS,
				            i, s_skyTexCoords[ t ][ s ], s_skyPoints[ t ][ s ] );
			}
		}

		// only add indexes for first stage
		FillCloudySkySide( sky_mins_subd, sky_maxs_subd, true );
	}
	Tess_UpdateVBOs( );
	GL_VertexAttribsState( tess.attribsSet );

	Tess_DrawElements();
}

static void FillCloudBox( int stage )
{
	int i;

	for ( i = 0; i < 6; i++ )
	{
		int   sky_mins_subd[ 2 ], sky_maxs_subd[ 2 ];
		int   s, t;
		float MIN_T;

		{
			MIN_T = -HALF_SKY_SUBDIVISIONS;

			// still don't want to draw the bottom, even if fullClouds
			if ( i == 5 )
			{
				continue;
			}
		}

		sky_mins[ 0 ][ i ] = floor( sky_mins[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_mins[ 1 ][ i ] = floor( sky_mins[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[ 0 ][ i ] = ceil( sky_maxs[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[ 1 ][ i ] = ceil( sky_maxs[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;

		if ( ( sky_mins[ 0 ][ i ] >= sky_maxs[ 0 ][ i ] ) || ( sky_mins[ 1 ][ i ] >= sky_maxs[ 1 ][ i ] ) )
		{
			continue;
		}

		sky_mins_subd[ 0 ] = Q_ftol( sky_mins[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS );
		sky_mins_subd[ 1 ] = Q_ftol( sky_mins[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS );
		sky_maxs_subd[ 0 ] = Q_ftol( sky_maxs[ 0 ][ i ] * HALF_SKY_SUBDIVISIONS );
		sky_maxs_subd[ 1 ] = Q_ftol( sky_maxs[ 1 ][ i ] * HALF_SKY_SUBDIVISIONS );

		if ( sky_mins_subd[ 0 ] < -HALF_SKY_SUBDIVISIONS )
		{
			sky_mins_subd[ 0 ] = -HALF_SKY_SUBDIVISIONS;
		}
		else if ( sky_mins_subd[ 0 ] > HALF_SKY_SUBDIVISIONS )
		{
			sky_mins_subd[ 0 ] = HALF_SKY_SUBDIVISIONS;
		}

		if ( sky_mins_subd[ 1 ] < MIN_T )
		{
			sky_mins_subd[ 1 ] = MIN_T;
		}
		else if ( sky_mins_subd[ 1 ] > HALF_SKY_SUBDIVISIONS )
		{
			sky_mins_subd[ 1 ] = HALF_SKY_SUBDIVISIONS;
		}

		if ( sky_maxs_subd[ 0 ] < -HALF_SKY_SUBDIVISIONS )
		{
			sky_maxs_subd[ 0 ] = -HALF_SKY_SUBDIVISIONS;
		}
		else if ( sky_maxs_subd[ 0 ] > HALF_SKY_SUBDIVISIONS )
		{
			sky_maxs_subd[ 0 ] = HALF_SKY_SUBDIVISIONS;
		}

		if ( sky_maxs_subd[ 1 ] < MIN_T )
		{
			sky_maxs_subd[ 1 ] = MIN_T;
		}
		else if ( sky_maxs_subd[ 1 ] > HALF_SKY_SUBDIVISIONS )
		{
			sky_maxs_subd[ 1 ] = HALF_SKY_SUBDIVISIONS;
		}

		// iterate through the subdivisions
		for ( t = sky_mins_subd[ 1 ] + HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[ 1 ] + HALF_SKY_SUBDIVISIONS; t++ )
		{
			for ( s = sky_mins_subd[ 0 ] + HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[ 0 ] + HALF_SKY_SUBDIVISIONS; s++ )
			{
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS,
				            ( t - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, i, nullptr, s_skyPoints[ t ][ s ] );

				s_skyTexCoords[ t ][ s ][ 0 ] = s_cloudTexCoords[ i ][ t ][ s ][ 0 ];
				s_skyTexCoords[ t ][ s ][ 1 ] = s_cloudTexCoords[ i ][ t ][ s ][ 1 ];
			}
		}

		// only add indexes for first stage
		FillCloudySkySide( sky_mins_subd, sky_maxs_subd, ( bool )( stage == 0 ) );
	}
}

static void BuildCloudData()
{
	int      i;

	ASSERT(tess.surfaceShader->isSky);

	sky_min = 1.0 / 256.0f; // FIXME: not correct?
	sky_max = 255.0 / 256.0f;

	// set up for drawing
	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.attribsSet = 0;

	Tess_MapVBOs( false );

	if ( tess.surfaceShader->sky.cloudHeight )
	{
		for ( i = 0; i < MAX_SHADER_STAGES; i++ )
		{
			if ( !tess.surfaceStages[ i ] )
			{
				break;
			}

			FillCloudBox( i );
		}
	}
}

/*
** R_InitSkyTexCoords
** Called when a sky shader is parsed
*/
void R_InitSkyTexCoords( float heightCloud )
{
	int    i, s, t;
	float  radiusWorld = 4096;
	float  p;
	float  sRad, tRad;
	vec4_t skyVec;
	vec3_t v;

	// init zfar so MakeSkyVec works even though
	// a world hasn't been bounded
	backEnd.viewParms.zFar = 1024;

	for ( i = 0; i < 6; i++ )
	{
		for ( t = 0; t <= SKY_SUBDIVISIONS; t++ )
		{
			for ( s = 0; s <= SKY_SUBDIVISIONS; s++ )
			{
				// compute vector from view origin to sky side integral point
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS,
				            ( t - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, i, nullptr, skyVec );

				// compute parametric value 'p' that intersects with cloud layer
				p = ( 1.0f / ( 2 * DotProduct( skyVec, skyVec ) ) ) *
				    ( -2 * skyVec[ 2 ] * radiusWorld +
				      2 * sqrt( Square( skyVec[ 2 ] ) * Square( radiusWorld ) +
				                2 * Square( skyVec[ 0 ] ) * radiusWorld * heightCloud +
				                Square( skyVec[ 0 ] ) * Square( heightCloud ) +
				                2 * Square( skyVec[ 1 ] ) * radiusWorld * heightCloud +
				                Square( skyVec[ 1 ] ) * Square( heightCloud ) +
				                2 * Square( skyVec[ 2 ] ) * radiusWorld * heightCloud + Square( skyVec[ 2 ] ) * Square( heightCloud ) ) );

				s_cloudTexP[ i ][ t ][ s ] = p;

				// compute intersection point based on p
				VectorScale( skyVec, p, v );
				v[ 2 ] += radiusWorld;

				// compute vector from world origin to intersection point 'v'
				VectorNormalize( v );

				sRad = acos( v[ 0 ] );
				tRad = acos( v[ 1 ] );

				s_cloudTexCoords[ i ][ t ][ s ][ 0 ] = sRad;
				s_cloudTexCoords[ i ][ t ][ s ][ 1 ] = tRad;
			}
		}
	}
}

//======================================================================================

/*
================
Tess_StageIteratorSky

All of the visible sky triangles are in tess

Other things could be stuck in here, like birds in the sky, etc
================
*/
void Tess_StageIteratorSky()
{
	// log this call
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va
		                  ( "--- Tess_StageIteratorSky( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
		                    tess.numVertexes, tess.numIndexes / 3 ) );
	}

	if ( r_fastsky->integer )
	{
		return;
	}

	// trebor: HACK why does this happen with cg_draw2D 0 ?
	if ( tess.stageIteratorFunc2 == nullptr )
	{
		//tess.stageIteratorFunc2 = Tess_StageIteratorGeneric;
		ri.Error( errorParm_t::ERR_FATAL, "tess.stageIteratorFunc == NULL" );
	}

	GL_Cull(cullType_t::CT_TWO_SIDED);

	if ( tess.stageIteratorFunc2 == &Tess_StageIteratorDepthFill )
	{
		// go through all the polygons and project them onto
		// the sky box to see which blocks on each side need
		// to be drawn
		Tess_ClipSkyPolygons();

		// generate the vertexes for all the clouds, which will be drawn
		// by the generic shader routine
		BuildCloudData();

		if ( tess.numVertexes || tess.multiDrawPrimitives )
		{
			tess.stageIteratorFunc2();
		}
	}
	else
	{
		// go through all the polygons and project them onto
		// the sky box to see which blocks on each side need
		// to be drawn
		Tess_ClipSkyPolygons();

		// r_showSky will let all the sky blocks be drawn in
		// front of everything to allow developers to see how
		// much sky is getting sucked in

		if ( r_showSky->integer )
		{
			glDepthRange( 0.0, 0.0 );
		}
		else
		{
			glDepthRange( 1.0, 1.0 );
		}

		// draw the outer skybox
		if ( tess.surfaceShader->sky.outerbox && tess.surfaceShader->sky.outerbox != tr.blackCubeImage )
		{
			R_BindVBO( tess.vbo );
			R_BindIBO( tess.ibo );

			gl_skyboxShader->BindProgram( 0 );

			gl_skyboxShader->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space

			gl_skyboxShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
			gl_skyboxShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

			gl_skyboxShader->SetRequiredVertexPointers();

			// bind u_ColorMap
			GL_BindToTMU( 0, tess.surfaceShader->sky.outerbox );

			DrawSkyBox();
		}

		// generate the vertexes for all the clouds, which will be drawn
		// by the generic shader routine
		BuildCloudData();

		if ( tess.numVertexes || tess.multiDrawPrimitives )
		{
			tess.stageIteratorFunc2();
		}

		if ( tess.stageIteratorFunc2 != Tess_StageIteratorDepthFill )
		{
			// back to standard depth range
			glDepthRange( 0.0, 1.0 );

			// note that sky was drawn so we will draw a sun later
			backEnd.skyRenderedThisView = true;
		}
	}
}
