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
// tr_surface.c
#include "tr_local.h"

/*
==============================================================================
THIS ENTIRE FILE IS BACK END!

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
==============================================================================
*/

static ALIGNED( 16, transform_t bones[ MAX_BONES ] );

/*
==============
Tess_EndBegin
==============
*/
void Tess_EndBegin()
{
	Tess_End();
	Tess_Begin( tess.stageIteratorFunc, tess.stageIteratorFunc2, tess.surfaceShader, tess.lightShader, tess.skipTangentSpaces, tess.skipVBO,
	            tess.lightmapNum, tess.fogNum );
}

/*
==============
Tess_CheckVBOAndIBO
==============
*/
static void Tess_CheckVBOAndIBO( VBO_t *vbo, IBO_t *ibo )
{
	if ( glState.currentVBO != vbo || glState.currentIBO != ibo || tess.multiDrawPrimitives >= MAX_MULTIDRAW_PRIMITIVES )
	{
		Tess_EndBegin();

		R_BindVBO( vbo );
		R_BindIBO( ibo );
	}
}

/*
==============
Tess_CheckOverflow
==============
*/
void Tess_CheckOverflow( int verts, int indexes )
{
	// FIXME: need to check if a vbo is bound, otherwise we fail on startup
	if ( glState.currentVBO != nullptr && glState.currentIBO != nullptr )
	{
		Tess_CheckVBOAndIBO( tess.vbo, tess.ibo );
	}

	if ( tess.buildingVBO )
	{
		return;
	}

	if ( tess.numVertexes + verts < SHADER_MAX_VERTEXES && tess.numIndexes + indexes < SHADER_MAX_INDEXES )
	{
		return;
	}

	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va
		                  ( "--- Tess_CheckOverflow(%i + %i vertices, %i + %i triangles ) ---\n", tess.numVertexes, verts,
		                    ( tess.numIndexes / 3 ), indexes ) );
	}

	Tess_End();

	if ( verts >= SHADER_MAX_VERTEXES )
	{
		ri.Error(errorParm_t::ERR_DROP, "Tess_CheckOverflow: verts > std::max (%d > %d)", verts, SHADER_MAX_VERTEXES );
	}

	if ( indexes >= SHADER_MAX_INDEXES )
	{
		ri.Error(errorParm_t::ERR_DROP, "Tess_CheckOverflow: indexes > std::max (%d > %d)", indexes, SHADER_MAX_INDEXES );
	}

	Tess_Begin( tess.stageIteratorFunc, tess.stageIteratorFunc2, tess.surfaceShader, tess.lightShader, tess.skipTangentSpaces, tess.skipVBO,
	            tess.lightmapNum, tess.fogNum );
}

/*
==============
Tess_SurfaceVertsAndTris
==============
*/
static void Tess_SurfaceVertsAndTris( const srfVert_t *verts, const srfTriangle_t *triangles, int numVerts, int numTriangles )
{
	int i;
	const srfTriangle_t *tri = triangles;
	const srfVert_t *vert = verts;
	const int numIndexes = numTriangles * 3;

	Tess_CheckOverflow( numVerts, numIndexes );

	for ( i = 0; i < numIndexes; i+=3, tri++ )
	{
		tess.indexes[ tess.numIndexes + i + 0 ] = tess.numVertexes + tri->indexes[ 0 ];
		tess.indexes[ tess.numIndexes + i + 1 ] = tess.numVertexes + tri->indexes[ 1 ];
		tess.indexes[ tess.numIndexes + i + 2 ] = tess.numVertexes + tri->indexes[ 2 ];
	}

	tess.numIndexes += numIndexes;

	for ( i = 0; i < numVerts; i++, vert++ )
	{
		VectorCopy( vert->xyz, tess.verts[ tess.numVertexes + i ].xyz );
		Vector4Copy( vert->qtangent, tess.verts[ tess.numVertexes + i ].qtangents );

		tess.verts[ tess.numVertexes + i ].texCoords[ 0 ] = floatToHalf( vert->st[ 0 ] );
		tess.verts[ tess.numVertexes + i ].texCoords[ 1 ] = floatToHalf( vert->st[ 1 ] );

		tess.verts[ tess.numVertexes + i ].texCoords[ 2 ] = floatToHalf( vert->lightmap[ 0 ] );
		tess.verts[ tess.numVertexes + i ].texCoords[ 3 ] = floatToHalf( vert->lightmap[ 1 ] );

		tess.verts[ tess.numVertexes + i ].color = vert->lightColor;
	}

	tess.numVertexes += numVerts;
	tess.attribsSet =  ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR | ATTR_QTANGENT;
}

static bool Tess_SurfaceVBO( VBO_t *vbo, IBO_t *ibo, int numIndexes, int firstIndex )
{
	if ( !vbo || !ibo )
	{
		return false;
	}

	if ( tess.skipVBO || tess.stageIteratorFunc == &Tess_StageIteratorSky )
	{
		return false;
	}

	Tess_CheckVBOAndIBO( vbo, ibo );

	//lazy merge multidraws together
	bool mergeBack = false;
	bool mergeFront = false;

	glIndex_t *firstIndexOffset = ( glIndex_t* ) BUFFER_OFFSET( firstIndex * sizeof( glIndex_t ) );

	if ( tess.multiDrawPrimitives > 0 )
	{
		int lastPrimitive = tess.multiDrawPrimitives - 1;
		glIndex_t *lastIndexOffset = firstIndexOffset + numIndexes;
		glIndex_t *prevLastIndexOffset = tess.multiDrawIndexes[ lastPrimitive ] + tess.multiDrawCounts[ lastPrimitive ];

		if ( firstIndexOffset == prevLastIndexOffset )
		{
			mergeFront = true;
		}
		else if ( lastIndexOffset == tess.multiDrawIndexes[ lastPrimitive ] )
		{
			mergeBack = true;
		}
	}

	if ( mergeFront )
	{
		tess.multiDrawCounts[ tess.multiDrawPrimitives - 1 ] += numIndexes;
	}
	else if ( mergeBack )
	{
		tess.multiDrawIndexes[ tess.multiDrawPrimitives - 1 ] = firstIndexOffset;
		tess.multiDrawCounts[ tess.multiDrawPrimitives - 1 ] += numIndexes;
	}
	else
	{
		tess.multiDrawIndexes[ tess.multiDrawPrimitives ] = firstIndexOffset;
		tess.multiDrawCounts[ tess.multiDrawPrimitives ] = numIndexes;

		tess.multiDrawPrimitives++;
	}

	return true;
}

/*
==============
Tess_AddQuadStampExt
==============
*/
void Tess_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, const Color::Color& color, float s1, float t1, float s2, float t2 )
{
	int    i;
	vec3_t normal;
	int    ndx;

	GLimp_LogComment( "--- Tess_AddQuadStampExt ---\n" );

	Tess_CheckOverflow( 4, 6 );

	ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[ tess.numIndexes ] = ndx;
	tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 2 ] = ndx + 3;

	tess.indexes[ tess.numIndexes + 3 ] = ndx + 3;
	tess.indexes[ tess.numIndexes + 4 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 5 ] = ndx + 2;

	tess.verts[ ndx ].xyz[ 0 ] = origin[ 0 ] + left[ 0 ] + up[ 0 ];
	tess.verts[ ndx ].xyz[ 1 ] = origin[ 1 ] + left[ 1 ] + up[ 1 ];
	tess.verts[ ndx ].xyz[ 2 ] = origin[ 2 ] + left[ 2 ] + up[ 2 ];

	tess.verts[ ndx + 1 ].xyz[ 0 ] = origin[ 0 ] - left[ 0 ] + up[ 0 ];
	tess.verts[ ndx + 1 ].xyz[ 1 ] = origin[ 1 ] - left[ 1 ] + up[ 1 ];
	tess.verts[ ndx + 1 ].xyz[ 2 ] = origin[ 2 ] - left[ 2 ] + up[ 2 ];

	tess.verts[ ndx + 2 ].xyz[ 0 ] = origin[ 0 ] - left[ 0 ] - up[ 0 ];
	tess.verts[ ndx + 2 ].xyz[ 1 ] = origin[ 1 ] - left[ 1 ] - up[ 1 ];
	tess.verts[ ndx + 2 ].xyz[ 2 ] = origin[ 2 ] - left[ 2 ] - up[ 2 ];

	tess.verts[ ndx + 3 ].xyz[ 0 ] = origin[ 0 ] + left[ 0 ] - up[ 0 ];
	tess.verts[ ndx + 3 ].xyz[ 1 ] = origin[ 1 ] + left[ 1 ] - up[ 1 ];
	tess.verts[ ndx + 3 ].xyz[ 2 ] = origin[ 2 ] + left[ 2 ] - up[ 2 ];

	// constant normal all the way around
	VectorSubtract( vec3_origin, backEnd.viewParms.orientation.axis[ 0 ], normal );
	R_TBNtoQtangents( left, up, normal, tess.verts[ ndx ].qtangents );
	Vector4Copy( tess.verts[ ndx ].qtangents, tess.verts[ ndx + 1 ].qtangents );
	Vector4Copy( tess.verts[ ndx ].qtangents, tess.verts[ ndx + 2 ].qtangents );
	Vector4Copy( tess.verts[ ndx ].qtangents, tess.verts[ ndx + 3 ].qtangents );

	// standard square texture coordinates
	tess.verts[ ndx ].texCoords[ 0 ] = floatToHalf( s1 );
	tess.verts[ ndx ].texCoords[ 1 ] = floatToHalf( t1 );

	tess.verts[ ndx + 1 ].texCoords[ 0 ] = floatToHalf( s2 );
	tess.verts[ ndx + 1 ].texCoords[ 1 ] = floatToHalf( t1 );

	tess.verts[ ndx + 2 ].texCoords[ 0 ] = floatToHalf( s2 );
	tess.verts[ ndx + 2 ].texCoords[ 1 ] = floatToHalf( t2 );

	tess.verts[ ndx + 3 ].texCoords[ 0 ] = floatToHalf( s1 );
	tess.verts[ ndx + 3 ].texCoords[ 1 ] = floatToHalf( t2 );

	// constant color all the way around
	// should this be identity and let the shader specify from entity?

	Color::Color32Bit iColor = color;

	for ( i = 0; i < 4; i++ )
	{
		tess.verts[ ndx + i ].color = iColor;
	}

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.attribsSet |= ATTR_POSITION | ATTR_QTANGENT | ATTR_COLOR | ATTR_TEXCOORD;
}

/*
==============
Tess_AddQuadStamp
==============
*/
void Tess_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, const Color::Color& color )
{
	Tess_AddQuadStampExt( origin, left, up, color, 0, 0, 1, 1 );
}

/*
==============
Tess_AddQuadStampExt2
==============
*/
void Tess_AddQuadStampExt2( vec4_t quadVerts[ 4 ], const Color::Color& color, float s1, float t1, float s2, float t2 )
{
	int    i;
	vec3_t normal, tangent, binormal;
	int    ndx;

	GLimp_LogComment( "--- Tess_AddQuadStampExt2 ---\n" );

	Tess_CheckOverflow( 4, 6 );

	ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[ tess.numIndexes ] = ndx;
	tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 2 ] = ndx + 3;

	tess.indexes[ tess.numIndexes + 3 ] = ndx + 3;
	tess.indexes[ tess.numIndexes + 4 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 5 ] = ndx + 2;

	VectorCopy( quadVerts[ 0 ], tess.verts[ ndx + 0 ].xyz );
	VectorCopy( quadVerts[ 1 ], tess.verts[ ndx + 1 ].xyz );
	VectorCopy( quadVerts[ 2 ], tess.verts[ ndx + 2 ].xyz );
	VectorCopy( quadVerts[ 3 ], tess.verts[ ndx + 3 ].xyz );

	tess.attribsSet |= ATTR_POSITION | ATTR_COLOR | ATTR_TEXCOORD | ATTR_QTANGENT;

	// constant normal all the way around
	vec2_t st[ 3 ] = { { s1, t1 }, { s2, t1 }, { s2, t2 } };
	R_CalcFaceNormal( normal, quadVerts[ 0 ], quadVerts[ 1 ], quadVerts[ 2 ] );
	R_CalcTangents( tangent, binormal,
			quadVerts[ 0 ], quadVerts[ 1 ], quadVerts[ 2 ],
			st[ 0 ], st[ 1 ], st[ 2 ] );
	//if ( !calcNormals )
	//{
	//	VectorNegate( backEnd.viewParms.orientation.axis[ 0 ], normal );
	//}

	R_TBNtoQtangents( tangent, binormal, normal, tess.verts[ ndx ].qtangents );
	Vector4Copy( tess.verts[ ndx ].qtangents, tess.verts[ ndx + 1 ].qtangents );
	Vector4Copy( tess.verts[ ndx ].qtangents, tess.verts[ ndx + 2 ].qtangents );
	Vector4Copy( tess.verts[ ndx ].qtangents, tess.verts[ ndx + 3 ].qtangents );

	// standard square texture coordinates
	tess.verts[ ndx ].texCoords[ 0 ] = floatToHalf( s1 );
	tess.verts[ ndx ].texCoords[ 1 ] = floatToHalf( t1 );

	tess.verts[ ndx + 1 ].texCoords[ 0 ] = floatToHalf( s2 );
	tess.verts[ ndx + 1 ].texCoords[ 1 ] = floatToHalf( t1 );

	tess.verts[ ndx + 2 ].texCoords[ 0 ] = floatToHalf( s2 );
	tess.verts[ ndx + 2 ].texCoords[ 1 ] = floatToHalf( t2 );

	tess.verts[ ndx + 3 ].texCoords[ 0 ] = floatToHalf( s1 );
	tess.verts[ ndx + 3 ].texCoords[ 1 ] = floatToHalf( t2 );

	// constant color all the way around
	// should this be identity and let the shader specify from entity?

	Color::Color32Bit iColor = color;
	for ( i = 0; i < 4; i++ )
	{
		tess.verts[ ndx + i ].color = iColor;
	}

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

/*
==============
Tess_AddQuadStamp2
==============
*/
void Tess_AddQuadStamp2( vec4_t quadVerts[ 4 ], const Color::Color& color )
{
	Tess_AddQuadStampExt2( quadVerts, color, 0, 0, 1, 1 );
}

void Tess_AddQuadStamp2WithNormals( vec4_t quadVerts[ 4 ], const Color::Color& color )
{
	Tess_AddQuadStampExt2( quadVerts, color, 0, 0, 1, 1 );
}

void Tess_AddSprite( const vec3_t center, const Color::Color32Bit color, float radius, float rotation )
{
	int    i;
	int    ndx;

	GLimp_LogComment( "--- Tess_AddSprite ---\n" );

	Tess_CheckOverflow( 4, 6 );

	ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[ tess.numIndexes     ] = ndx;
	tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 2 ] = ndx + 3;

	tess.indexes[ tess.numIndexes + 3 ] = ndx + 3;
	tess.indexes[ tess.numIndexes + 4 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 5 ] = ndx + 2;

	for ( i = 0; i < 4; i++ )
	{
		vec4_t texCoord;
		vec4_t orientation;

		Vector4Set( texCoord, 0.5f * (i & 2), 0.5f * ( (i + 1) & 2 ),
			    (i & 2) - 1.0f, ( (i + 1) & 2 ) - 1.0f );

		VectorCopy( center, tess.verts[ ndx + i ].xyz );
		tess.verts[ ndx + i ].color = color;
		floatToHalf( texCoord, tess.verts[ ndx + i ].texCoords );
		Vector4Set( orientation, rotation, 0.0f, 0.0f, radius );
		floatToHalf( orientation, tess.verts[ ndx + i ].spriteOrientation );
	}

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.attribsSet |= ATTR_POSITION | ATTR_COLOR | ATTR_TEXCOORD | ATTR_ORIENTATION;
}

void Tess_AddTetrahedron( vec4_t tetraVerts[ 4 ], const Color::Color& colorf )
{
	int k;

	Tess_CheckOverflow( 12, 12 );

	Color::Color32Bit color = colorf;

	// ground triangle
	for ( k = 0; k < 3; k++ )
	{
		VectorCopy( tetraVerts[ k ], tess.verts[ tess.numVertexes ].xyz );
		tess.verts[ tess.numVertexes ].color = color;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;
	}

	// side triangles
	for ( k = 0; k < 3; k++ )
	{
		VectorCopy( tetraVerts[ 3 ], tess.verts[ tess.numVertexes ].xyz );  // offset
		tess.verts[ tess.numVertexes ].color = color;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;

		VectorCopy( tetraVerts[ k ], tess.verts[ tess.numVertexes ].xyz );
		tess.verts[ tess.numVertexes ].color = color;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;

		VectorCopy( tetraVerts[( k + 1 ) % 3 ], tess.verts[ tess.numVertexes ].xyz );
		tess.verts[ tess.numVertexes ].color = color;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_COLOR;
}

void Tess_AddCube( const vec3_t position, const vec3_t minSize, const vec3_t maxSize, const Color::Color& color )
{
	vec4_t quadVerts[ 4 ];
	vec3_t mins;
	vec3_t maxs;

	VectorAdd( position, minSize, mins );
	VectorAdd( position, maxSize, maxs );

	Vector4Set( quadVerts[ 0 ], mins[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], mins[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], mins[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], mins[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Tess_AddQuadStamp2( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], maxs[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], maxs[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], maxs[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], mins[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], mins[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], maxs[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Tess_AddQuadStamp2( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], maxs[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], maxs[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], mins[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], mins[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], mins[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], mins[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], maxs[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], maxs[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], maxs[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], mins[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], mins[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2( quadVerts, color );
}

void Tess_AddCubeWithNormals( const vec3_t position, const vec3_t minSize, const vec3_t maxSize, const Color::Color& color )
{
	vec4_t quadVerts[ 4 ];
	vec3_t mins;
	vec3_t maxs;

	VectorAdd( position, minSize, mins );
	VectorAdd( position, maxSize, maxs );

	Vector4Set( quadVerts[ 0 ], mins[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], mins[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], mins[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], mins[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Tess_AddQuadStamp2WithNormals( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], maxs[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], maxs[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], maxs[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2WithNormals( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], mins[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], mins[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], maxs[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Tess_AddQuadStamp2WithNormals( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], maxs[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], maxs[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], mins[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], mins[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2WithNormals( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], mins[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], mins[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], maxs[ 0 ], mins[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], maxs[ 0 ], mins[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2WithNormals( quadVerts, color );

	Vector4Set( quadVerts[ 0 ], maxs[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Vector4Set( quadVerts[ 1 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 2 ], mins[ 0 ], maxs[ 1 ], maxs[ 2 ], 1 );
	Vector4Set( quadVerts[ 3 ], mins[ 0 ], maxs[ 1 ], mins[ 2 ], 1 );
	Tess_AddQuadStamp2WithNormals( quadVerts, color );
}


/*
==============
Tess_InstantQuad
==============
*/
void Tess_InstantQuad( vec4_t quadVerts[ 4 ] )
{
	GLimp_LogComment( "--- Tess_InstantQuad ---\n" );

	tess.multiDrawPrimitives = 0;
	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.attribsSet = 0;

	Tess_MapVBOs( false );
	VectorCopy( quadVerts[ 0 ], tess.verts[ tess.numVertexes ].xyz );
	tess.verts[ tess.numVertexes ].color = Color::White;
	tess.verts[ tess.numVertexes ].texCoords[ 0 ] = floatToHalf( 0.0f );
	tess.verts[ tess.numVertexes ].texCoords[ 1 ] = floatToHalf( 0.0f );
	tess.numVertexes++;

	VectorCopy( quadVerts[ 1 ], tess.verts[ tess.numVertexes ].xyz );
	tess.verts[ tess.numVertexes ].color = Color::White;
	tess.verts[ tess.numVertexes ].texCoords[ 0 ] = floatToHalf( 1.0f );
	tess.verts[ tess.numVertexes ].texCoords[ 1 ] = floatToHalf( 0.0f );
	tess.numVertexes++;

	VectorCopy( quadVerts[ 2 ], tess.verts[ tess.numVertexes ].xyz );
	tess.verts[ tess.numVertexes ].color = Color::White;
	tess.verts[ tess.numVertexes ].texCoords[ 0 ] = floatToHalf( 1.0f );
	tess.verts[ tess.numVertexes ].texCoords[ 1 ] = floatToHalf( 1.0f );
	tess.numVertexes++;

	VectorCopy( quadVerts[ 3 ], tess.verts[ tess.numVertexes ].xyz );
	tess.verts[ tess.numVertexes ].color = Color::White;
	tess.verts[ tess.numVertexes ].texCoords[ 0 ] = floatToHalf( 0.0f );
	tess.verts[ tess.numVertexes ].texCoords[ 1 ] = floatToHalf( 1.0f );
	tess.numVertexes++;

	tess.indexes[ tess.numIndexes++ ] = 0;
	tess.indexes[ tess.numIndexes++ ] = 1;
	tess.indexes[ tess.numIndexes++ ] = 2;
	tess.indexes[ tess.numIndexes++ ] = 0;
	tess.indexes[ tess.numIndexes++ ] = 2;
	tess.indexes[ tess.numIndexes++ ] = 3;

	Tess_UpdateVBOs( );
	GL_VertexAttribsState( ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR );

	Tess_DrawElements();

	tess.multiDrawPrimitives = 0;
	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.attribsSet = 0;
	GL_CheckErrors();
}

/*
==============
Tess_SurfaceSprite
==============
*/
static const float NORMAL_EPSILON = 0.0001;

static void Tess_SurfaceSprite()
{
	vec3_t delta, left, up;
	float  radius;

	GLimp_LogComment( "--- Tess_SurfaceSprite ---\n" );

	radius = backEnd.currentEntity->e.radius;

	if( tess.surfaceShader->autoSpriteMode == 1 ) {
		// the calculations are done in GLSL shader

		Tess_AddSprite( backEnd.currentEntity->e.origin, 
				backEnd.currentEntity->e.shaderRGBA,
				radius, backEnd.currentEntity->e.rotation );
		return;
	}

	VectorSubtract( backEnd.currentEntity->e.origin, backEnd.viewParms.pvsOrigin, delta );

	if( VectorNormalize( delta ) < NORMAL_EPSILON )
		return;

	CrossProduct( backEnd.viewParms.orientation.axis[ 2 ], delta, left );

	if( VectorNormalize( left ) < NORMAL_EPSILON )
		VectorSet( left, 1, 0, 0 );

	if( backEnd.currentEntity->e.rotation != 0 )
		RotatePointAroundVector( left, delta, left, backEnd.currentEntity->e.rotation );

	CrossProduct( delta, left, up );

	VectorScale( left, radius, left );
	VectorScale( up, radius, up );

	if ( backEnd.viewParms.isMirror )
		VectorSubtract( vec3_origin, left, left );

	Tess_AddQuadStamp( backEnd.currentEntity->e.origin, left, up,
		backEnd.currentEntity->e.shaderRGBA );
}

/*
=============
Tess_SurfacePolychain
=============
*/
static void Tess_SurfacePolychain( srfPoly_t *p )
{
	int i, j;
	int numVertexes;
	int numIndexes;

	GLimp_LogComment( "--- Tess_SurfacePolychain ---\n" );

	Tess_CheckOverflow( p->numVerts, 3 * ( p->numVerts - 2 ) );

	// fan triangles into the tess array
	numVertexes = 0;

	for ( i = 0; i < p->numVerts; i++ )
	{
		VectorCopy( p->verts[ i ].xyz, tess.verts[ tess.numVertexes + i ].xyz );

		tess.verts[ tess.numVertexes + i ].texCoords[ 0 ] = floatToHalf( p->verts[ i ].st[ 0 ] );
		tess.verts[ tess.numVertexes + i ].texCoords[ 1 ] = floatToHalf( p->verts[ i ].st[ 1 ] );

		tess.verts[ tess.numVertexes + i ].color = Color::Adapt( p->verts[ i ].modulate );

		numVertexes++;
	}

	// generate fan indexes into the tess array
	numIndexes = 0;

	for ( i = 0; i < p->numVerts - 2; i++ )
	{
		tess.indexes[ tess.numIndexes + i * 3 + 0 ] = tess.numVertexes;
		tess.indexes[ tess.numIndexes + i * 3 + 1 ] = tess.numVertexes + i + 1;
		tess.indexes[ tess.numIndexes + i * 3 + 2 ] = tess.numVertexes + i + 2;
		numIndexes += 3;
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR;

	// calc tangent spaces
	if ( tess.surfaceShader->interactLight && !tess.skipTangentSpaces )
	{
		int         i;
		float       *v;
		const float *v0, *v1, *v2;
		const int16_t *t0, *t1, *t2;
		vec3_t      tangent, *tangents;
		vec3_t      binormal, *binormals;
		vec3_t      normal, *normals;
		glIndex_t   *indices;

		tangents = (vec3_t *)ri.Hunk_AllocateTempMemory( numVertexes * sizeof( vec3_t ) );
		binormals = (vec3_t *)ri.Hunk_AllocateTempMemory( numVertexes * sizeof( vec3_t ) );
		normals = (vec3_t *)ri.Hunk_AllocateTempMemory( numVertexes * sizeof( vec3_t ) );

		for ( i = 0; i < numVertexes; i++ )
		{
			VectorClear( tangents[ i ] );
			VectorClear( binormals[ i ] );
			VectorClear( normals[ i ] );
		}

		for ( i = 0, indices = tess.indexes + tess.numIndexes; i < numIndexes; i += 3, indices += 3 )
		{
			v0 = tess.verts[ indices[ 0 ] ].xyz;
			v1 = tess.verts[ indices[ 1 ] ].xyz;
			v2 = tess.verts[ indices[ 2 ] ].xyz;

			t0 = tess.verts[ indices[ 0 ] ].texCoords;
			t1 = tess.verts[ indices[ 1 ] ].texCoords;
			t2 = tess.verts[ indices[ 2 ] ].texCoords;

			R_CalcFaceNormal( normal, v0, v1, v2 );
			R_CalcTangents( tangent, binormal, v0, v1, v2, t0, t1, t2 );

			for ( j = 0; j < 3; j++ )
			{
				v = tangents[ indices[ j ] - tess.numVertexes ];
				VectorAdd( v, tangent, v );
				v = binormals[ indices[ j ] - tess.numVertexes ];
				VectorAdd( v, binormal, v );
				v = normals[ indices[ j ] - tess.numVertexes ];
				VectorAdd( v, normal, v );
			}
		}

		for ( i = 0; i < numVertexes; i++ )
		{
			VectorNormalizeFast( normals[ i ] );
			R_TBNtoQtangents( tangents[ i ], binormals[ i ],
					  normals[ i ], tess.verts[ tess.numVertexes + i ].qtangents );
		}

		ri.Hunk_FreeTempMemory( normals );
		ri.Hunk_FreeTempMemory( binormals );
		ri.Hunk_FreeTempMemory( tangents );

		tess.attribsSet |= ATTR_QTANGENT;
	}

	tess.numIndexes += numIndexes;
	tess.numVertexes += numVertexes;
}

void Tess_SurfacePolybuffer( srfPolyBuffer_t *surf )
{
	int       i;
	int       numIndexes;
	int       numVertexes;
	glIndex_t *indices;
	float     *xyzw;
	float     *st;
	byte      *color;

	GLimp_LogComment( "--- Tess_SurfacePolybuffer ---\n" );

	Tess_CheckOverflow( surf->pPolyBuffer->numVerts, surf->pPolyBuffer->numIndicies );

	numIndexes = std::min( surf->pPolyBuffer->numIndicies, MAX_PB_INDICIES );
	indices = surf->pPolyBuffer->indicies;

	for ( i = 0; i < numIndexes; i++ )
	{
		tess.indexes[ tess.numIndexes + i ] = tess.numVertexes + indices[ i ];
	}

	tess.numIndexes += numIndexes;

	numVertexes = std::min( surf->pPolyBuffer->numVerts, MAX_PB_VERTS );
	xyzw = &surf->pPolyBuffer->xyz[ 0 ][ 0 ];
	st = &surf->pPolyBuffer->st[ 0 ][ 0 ];
	color = &surf->pPolyBuffer->color[ 0 ][ 0 ];

	for ( i = 0; i < numVertexes; i++, xyzw += 4, st += 2, color += 4 )
	{
		VectorCopy( xyzw, tess.verts[ tess.numVertexes + i ].xyz );

		tess.verts[ tess.numVertexes + i ].texCoords[ 0 ] = floatToHalf( st[ 0 ] );
		tess.verts[ tess.numVertexes + i ].texCoords[ 1 ] = floatToHalf( st[ 1 ] );

		tess.verts[ tess.numVertexes + i ].color = Color::Adapt( color );
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_COLOR | ATTR_TEXCOORD;
	tess.numVertexes += numVertexes;
}

// ydnar: decal surfaces
void Tess_SurfaceDecal( srfDecal_t *srf )
{
	int i;

	GLimp_LogComment( "--- Tess_SurfaceDecal ---\n" );

	Tess_CheckOverflow( srf->numVerts, 3 * ( srf->numVerts - 2 ) );

	// fan triangles into the tess array
	for ( i = 0; i < srf->numVerts; i++ )
	{
		VectorCopy( srf->verts[ i ].xyz, tess.verts[ tess.numVertexes + i ].xyz );

		tess.verts[ tess.numVertexes + i ].texCoords[ 0 ] = floatToHalf( srf->verts[ i ].st[ 0 ] );
		tess.verts[ tess.numVertexes + i ].texCoords[ 1 ] = floatToHalf( srf->verts[ i ].st[ 1 ] );

		tess.verts[ tess.numVertexes + i ].color = Color::Adapt( srf->verts[ i ].modulate );
	}

	// generate fan indexes into the tess array
	for ( i = 0; i < srf->numVerts - 2; i++ )
	{
		tess.indexes[ tess.numIndexes + 0 ] = tess.numVertexes;
		tess.indexes[ tess.numIndexes + 1 ] = tess.numVertexes + i + 1;
		tess.indexes[ tess.numIndexes + 2 ] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_COLOR | ATTR_TEXCOORD;
	tess.numVertexes += srf->numVerts;
}

/*
==============
Tess_SurfaceFace
==============
*/
static void Tess_SurfaceFace( srfSurfaceFace_t *srf )
{
	GLimp_LogComment( "--- Tess_SurfaceFace ---\n" );

	if ( !r_vboFaces->integer || !Tess_SurfaceVBO( srf->vbo, srf->ibo, srf->numTriangles * 3, srf->firstTriangle * 3 ) )
	{
		Tess_SurfaceVertsAndTris( srf->verts, srf->triangles, srf->numVerts, srf->numTriangles );
	}
}

/*
=============
Tess_SurfaceGrid
=============
*/
static void Tess_SurfaceGrid( srfGridMesh_t *srf )
{
	GLimp_LogComment( "--- Tess_SurfaceGrid ---\n" );

	if ( !r_vboCurves->integer || !Tess_SurfaceVBO( srf->vbo, srf->ibo, srf->numTriangles * 3, srf->firstTriangle * 3 ) )
	{
		Tess_SurfaceVertsAndTris( srf->verts, srf->triangles, srf->numVerts, srf->numTriangles );
	}
}

/*
=============
Tess_SurfaceTriangles
=============
*/
static void Tess_SurfaceTriangles( srfTriangles_t *srf )
{
	GLimp_LogComment( "--- Tess_SurfaceTriangles ---\n" );

	if ( !r_vboTriangles->integer || !Tess_SurfaceVBO( srf->vbo, srf->ibo, srf->numTriangles * 3, srf->firstTriangle * 3 ) )
	{
		Tess_SurfaceVertsAndTris( srf->verts, srf->triangles, srf->numVerts, srf->numTriangles );
	}
}

//================================================================================

/*
=============
Tess_SurfaceMDV
=============
*/
static void Tess_SurfaceMDV( mdvSurface_t *srf )
{
	int           i, j;
	int           numIndexes = 0;
	int           numVertexes;
	mdvXyz_t      *oldVert, *newVert;
	mdvNormal_t   *oldNormal, *newNormal;
	mdvSt_t       *st;
	srfTriangle_t *tri;
	float         backlerp;
	float         oldXyzScale, newXyzScale;

	GLimp_LogComment( "--- Tess_SurfaceMDV ---\n" );

	if ( backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame )
	{
		backlerp = 0;
	}
	else
	{
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	newXyzScale = ( 1.0f - backlerp );
	oldXyzScale = backlerp;

	Tess_CheckOverflow( srf->numVerts, srf->numTriangles * 3 );

	numIndexes = srf->numTriangles * 3;

	for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
	{
		tess.indexes[ tess.numIndexes + i * 3 + 0 ] = tess.numVertexes + tri->indexes[ 0 ];
		tess.indexes[ tess.numIndexes + i * 3 + 1 ] = tess.numVertexes + tri->indexes[ 1 ];
		tess.indexes[ tess.numIndexes + i * 3 + 2 ] = tess.numVertexes + tri->indexes[ 2 ];
	}

	newVert = srf->verts + ( backEnd.currentEntity->e.frame * srf->numVerts );
	oldVert = srf->verts + ( backEnd.currentEntity->e.oldframe * srf->numVerts );
	newNormal = srf->normals + ( backEnd.currentEntity->e.frame * srf->numVerts );
	oldNormal = srf->normals + ( backEnd.currentEntity->e.oldframe * srf->numVerts );
	st = srf->st;

	numVertexes = srf->numVerts;

	for ( j = 0; j < numVertexes; j++, newVert++, oldVert++, st++ )
	{
		vec3_t tmpVert;

		if ( backlerp == 0 )
		{
			// just copy
			VectorCopy( newVert->xyz, tmpVert );
		}
		else
		{
			// interpolate the xyz
			VectorScale( oldVert->xyz, oldXyzScale, tmpVert );
			VectorMA( tmpVert, newXyzScale, newVert->xyz, tmpVert );
		}

		tess.verts[ tess.numVertexes + j ].xyz[ 0 ] = tmpVert[ 0 ];
		tess.verts[ tess.numVertexes + j ].xyz[ 1 ] = tmpVert[ 1 ];
		tess.verts[ tess.numVertexes + j ].xyz[ 2 ] = tmpVert[ 2 ];

		tess.verts[ tess.numVertexes + j ].texCoords[ 0 ] = floatToHalf( st->st[ 0 ] );
		tess.verts[ tess.numVertexes + j ].texCoords[ 1 ] = floatToHalf( st->st[ 1 ] );
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD;

	// calc tangent spaces
	if ( !tess.skipTangentSpaces )
	{
		int         i;
		float       *v;
		const float *v0, *v1, *v2;
		const int16_t *t0, *t1, *t2;
		vec3_t      tangent, *tangents;
		vec3_t      binormal, *binormals;
		vec3_t      *normals;
		glIndex_t   *indices;

		tess.attribsSet |= ATTR_QTANGENT;

		tangents = (vec3_t *)ri.Hunk_AllocateTempMemory( numVertexes * sizeof( vec3_t ) );
		binormals = (vec3_t *)ri.Hunk_AllocateTempMemory( numVertexes * sizeof( vec3_t ) );
		normals = (vec3_t *)ri.Hunk_AllocateTempMemory( numVertexes * sizeof( vec3_t ) );

		for ( i = 0; i < numVertexes; i++ )
		{
			VectorClear( tangents[ i ] );
			VectorClear( binormals[ i ] );

			if ( backlerp == 0 )
			{
				// just copy
				VectorCopy( newNormal->normal, normals[ i ] );
			}
			else
			{
				// interpolate the xyz
				VectorScale( oldNormal->normal, oldXyzScale, normals[ i ] );
				VectorMA( normals[ i ], newXyzScale, newNormal->normal, normals[ i ] );
				VectorNormalizeFast( normals[ i ] );
			}
		}

		for ( i = 0, indices = tess.indexes + tess.numIndexes; i < numIndexes; i += 3, indices += 3 )
		{
			v0 = tess.verts[ indices[ 0 ] ].xyz;
			v1 = tess.verts[ indices[ 1 ] ].xyz;
			v2 = tess.verts[ indices[ 2 ] ].xyz;

			t0 = tess.verts[ indices[ 0 ] ].texCoords;
			t1 = tess.verts[ indices[ 1 ] ].texCoords;
			t2 = tess.verts[ indices[ 2 ] ].texCoords;

			R_CalcTangents( tangent, binormal, v0, v1, v2, t0, t1, t2 );

			for ( j = 0; j < 3; j++ )
			{
				v = tangents[ indices[ j ] - tess.numVertexes ];
				VectorAdd( v, tangent, v );

				v = binormals[ indices[ j ] - tess.numVertexes ];
				VectorAdd( v, binormal, v );
			}
		}

		for ( i = 0; i < numVertexes; i++ )
		{
			R_TBNtoQtangents( tangents[ i ], binormals[ i ],
					  normals[ i ], tess.verts[ numVertexes + i ].qtangents );
		}

		ri.Hunk_FreeTempMemory( normals );
		ri.Hunk_FreeTempMemory( binormals );
		ri.Hunk_FreeTempMemory( tangents );
	}

	tess.numIndexes += numIndexes;
	tess.numVertexes += numVertexes;
}

/*
==============
Tess_SurfaceMD5
==============
*/
static void Tess_SurfaceMD5( md5Surface_t *srf )
{
	int             j;
	int             numIndexes = 0;
	int             numVertexes;
	md5Model_t      *model;
	md5Vertex_t     *v;
	srfTriangle_t   *tri;

	GLimp_LogComment( "--- Tess_SurfaceMD5 ---\n" );

	Tess_CheckOverflow( srf->numVerts, srf->numTriangles * 3 );

	model = srf->model;

	numIndexes = srf->numTriangles * 3;

    tri = srf->triangles;
	for (unsigned i = 0; i < srf->numTriangles; i++, tri++ )
	{
		tess.indexes[ tess.numIndexes + i * 3 + 0 ] = tess.numVertexes + tri->indexes[ 0 ];
		tess.indexes[ tess.numIndexes + i * 3 + 1 ] = tess.numVertexes + tri->indexes[ 1 ];
		tess.indexes[ tess.numIndexes + i * 3 + 2 ] = tess.numVertexes + tri->indexes[ 2 ];
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD;

	if ( tess.skipTangentSpaces )
	{
		// convert bones back to matrices
		for (unsigned i = 0; i < model->numBones; i++ )
		{
			if ( backEnd.currentEntity->e.skeleton.type == refSkeletonType_t::SK_ABSOLUTE )
			{
				refBone_t *bone = &backEnd.currentEntity->e.skeleton.bones[ i ];

				TransInitRotationQuat( model->bones[ i ].rotation, &bones[ i ] );
				TransAddTranslation( model->bones[ i ].origin, &bones[ i ] );
				TransInverse( &bones[ i ], &bones[ i ] );
				TransCombine( &bones[ i ], &bone->t, &bones[ i ] );
				TransAddScale( backEnd.currentEntity->e.skeleton.scale, &bones[ i ] );
			}
			else
			{
				TransInitRotationQuat( model->bones[ i ].rotation, &bones[i] );
				TransAddTranslation( model->bones[ i ].origin, &bones[ i ] );
			}
			TransInsScale( model->internalScale, &bones[ i ] );
		}

		// deform the vertices by the lerped bones
		numVertexes = srf->numVerts;

		for ( j = 0, v = srf->verts; j < numVertexes; j++, v++ )
		{
			vec3_t tmp;

			VectorClear( tess.verts[ tess.numVertexes + j ].xyz );
			for (unsigned k = 0; k < v->numWeights; k++ ) {
				TransformPoint( &bones[ v->boneIndexes[ k ] ],
						v->position, tmp );
				VectorMA( tess.verts[ tess.numVertexes + j ].xyz,
					  v->boneWeights[ k ], tmp,
					  tess.verts[ tess.numVertexes + j ].xyz );

			}

			tess.verts[ tess.numVertexes + j ].texCoords[ 0 ] = floatToHalf( v->texCoords[ 0 ] );
			tess.verts[ tess.numVertexes + j ].texCoords[ 1 ] = floatToHalf( v->texCoords[ 1 ] );
		}
	}
	else
	{
		tess.attribsSet |= ATTR_QTANGENT;

		// convert bones back to matrices
		for (unsigned i = 0; i < model->numBones; i++ )
		{
			if ( backEnd.currentEntity->e.skeleton.type == refSkeletonType_t::SK_ABSOLUTE )
			{
				refBone_t *bone = &backEnd.currentEntity->e.skeleton.bones[ i ];
				TransInitRotationQuat( model->bones[ i ].rotation, &bones[ i ] );
				TransAddTranslation( model->bones[ i ].origin, &bones[ i ] );
				TransInverse( &bones[ i ], &bones[ i ] );

				TransCombine( &bones[ i ], &bone->t, &bones[ i ] );
				TransAddScale( backEnd.currentEntity->e.skeleton.scale, &bones[ i ] );
			}
			else
			{
				TransInitScale( backEnd.currentEntity->e.skeleton.scale, &bones[ i ] );
			}
			TransInsScale( model->internalScale, &bones[ i ] );
		}

		// deform the vertices by the lerped bones
		numVertexes = srf->numVerts;

		for ( j = 0, v = srf->verts; j < numVertexes; j++, v++ )
		{
			vec3_t tangent, binormal, normal, tmp;

			VectorClear( tess.verts[ tess.numVertexes + j ].xyz );
			VectorClear( normal );
			VectorClear( binormal );
			VectorClear( tangent );

			for(unsigned k = 0; k < v->numWeights; k++ ) {
				TransformPoint( &bones[ v->boneIndexes[ k ] ],
						v->position, tmp );
				VectorMA( tess.verts[ tess.numVertexes + j ].xyz,
					  v->boneWeights[ k ], tmp,
					  tess.verts[ tess.numVertexes + j ].xyz );

				TransformNormalVector( &bones[ v->boneIndexes[ k ] ],
						       v->normal, tmp );
				VectorMA( normal, v->boneWeights[ k ], tmp, normal );

				TransformNormalVector( &bones[ v->boneIndexes[ k ] ],
						       v->tangent, tmp );
				VectorMA( tangent, v->boneWeights[ k ], tmp, tangent );

				TransformNormalVector( &bones[ v->boneIndexes[ k ] ],
						       v->binormal, tmp );
				VectorMA( binormal, v->boneWeights[ k ], tmp, binormal );
			}
			VectorNormalize( normal );
			VectorNormalize( tangent );
			VectorNormalize( binormal );

			R_TBNtoQtangents( tangent, binormal, normal, tess.verts[ tess.numVertexes + j ].qtangents );

			tess.verts[ tess.numVertexes + j ].texCoords[ 0 ] = floatToHalf( v->texCoords[ 0 ] );
			tess.verts[ tess.numVertexes + j ].texCoords[ 1 ] = floatToHalf( v->texCoords[ 1 ] );
		}
	}

	tess.numIndexes += numIndexes;
	tess.numVertexes += numVertexes;
}

/*
=================
Tess_SurfaceIQM

Compute vertices for this model surface
=================
*/
void Tess_SurfaceIQM( srfIQModel_t *surf ) {
	IQModel_t       *model = surf->data;
	int             i, j;
	int             offset = tess.numVertexes - surf->first_vertex;

	GLimp_LogComment( "--- RB_SurfaceIQM ---\n" );

	Tess_CheckOverflow( surf->num_vertexes, surf->num_triangles * 3 );

	// compute bones
	for ( i = 0; i < model->num_joints; i++ )
	{

		if ( backEnd.currentEntity->e.skeleton.type == refSkeletonType_t::SK_ABSOLUTE )
		{
			refBone_t *bone = &backEnd.currentEntity->e.skeleton.bones[ i ];

			TransInverse( &model->joints[ i ], &bones[ i ] );
			TransCombine( &bones[ i ], &bone->t, &bones[ i ] );
		}
		else
		{
			TransInit( &bones[ i ] );
		}
		TransAddScale( backEnd.currentEntity->e.skeleton.scale, &bones[ i ] );
		TransInsScale( model->internalScale, &bones[ i ] );
	}

	if( surf->vbo && surf->ibo ) {
		if( model->num_joints > 0 ) {
			Com_Memcpy( tess.bones, bones, model->num_joints * sizeof(transform_t) );
			tess.numBones = model->num_joints;
		} else {
			TransInitScale( model->internalScale * backEnd.currentEntity->e.skeleton.scale, &tess.bones[ 0 ] );
			tess.numBones = 1;
		}
		R_BindVBO( surf->vbo );
		R_BindIBO( surf->ibo );
		tess.vboVertexSkinning = true;

		tess.multiDrawIndexes[ tess.multiDrawPrimitives ] = ((glIndex_t *)nullptr) + surf->first_triangle * 3;
		tess.multiDrawCounts[ tess.multiDrawPrimitives ] = surf->num_triangles * 3;
		tess.multiDrawPrimitives++;

		Tess_End();
		return;
	}

	for ( i = 0; i < surf->num_triangles; i++ )
	{
		tess.indexes[ tess.numIndexes + i * 3 + 0 ] = offset + model->triangles[ 3 * ( surf->first_triangle + i ) + 0 ];
		tess.indexes[ tess.numIndexes + i * 3 + 1 ] = offset + model->triangles[ 3 * ( surf->first_triangle + i ) + 1 ];
		tess.indexes[ tess.numIndexes + i * 3 + 2 ] = offset + model->triangles[ 3 * ( surf->first_triangle + i ) + 2 ];
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT;

	if( model->num_joints > 0 && model->blendWeights && model->blendIndexes ) {
		// deform the vertices by the lerped bones
		for ( i = 0; i < surf->num_vertexes; i++ )
		{
			int    idxIn = surf->first_vertex + i;
			int    idxOut = tess.numVertexes + i;
			const float weightFactor = 1.0f / 255.0f;
			vec3_t tangent, binormal, normal, tmp;

			if( model->blendWeights[ 4 * idxIn + 0 ] == 0 &&
			    model->blendWeights[ 4 * idxIn + 1 ] == 0 &&
			    model->blendWeights[ 4 * idxIn + 2 ] == 0 &&
			    model->blendWeights[ 4 * idxIn + 3 ] == 0 )
				model->blendWeights[ 4 * idxIn + 0 ] = 255;

			VectorClear( tess.verts[ idxOut ].xyz );
			VectorClear( normal );
			VectorClear( tangent );
			VectorClear( binormal );
			for ( j = 0; j < 4; j++ ) {
				int bone = model->blendIndexes[ 4 * idxIn + j ];
				float weight = weightFactor * model->blendWeights[ 4 * idxIn + j ];

				TransformPoint( &bones[ bone ],
						&model->positions[ 3 * idxIn ], tmp );
				VectorMA( tess.verts[ idxOut ].xyz, weight, tmp,
					  tess.verts[ idxOut ].xyz );

				TransformNormalVector( &bones[ bone ],
						       &model->normals[ 3 * idxIn ], tmp );
				VectorMA( normal, weight, tmp, normal );
				TransformNormalVector( &bones[ bone ],
						       &model->tangents[ 3 * idxIn ], tmp );
				VectorMA( tangent, weight, tmp, tangent );
				TransformNormalVector( &bones[ bone ],
						       &model->bitangents[ 3 * idxIn ], tmp );
				VectorMA( binormal, weight, tmp, binormal );
			}
			VectorNormalize( normal );
			VectorNormalize( tangent );
			VectorNormalize( binormal );

			R_TBNtoQtangents( tangent, binormal, normal, tess.verts[ idxOut ].qtangents );

			tess.verts[ idxOut ].texCoords[ 0 ] = model->texcoords[ 2 * idxIn + 0 ];
			tess.verts[ idxOut ].texCoords[ 1 ] = model->texcoords[ 2 * idxIn + 1 ];
		}
	} else {
		for ( i = 0; i < surf->num_vertexes; i++ )
		{
			int    idxIn = surf->first_vertex + i;
			int    idxOut = tess.numVertexes + i;
			float  scale = model->internalScale * backEnd.currentEntity->e.skeleton.scale;

			VectorScale( &model->positions[ 3 * idxIn ], scale, tess.verts[ idxOut ].xyz );
			R_TBNtoQtangents( &model->tangents[ 3 * idxIn ],
					  &model->bitangents[ 3 * idxIn ],
					  &model->normals[ 3 * idxIn ],
					  tess.verts[ idxOut ].qtangents );

			tess.verts[ idxOut ].texCoords[ 0 ] = model->texcoords[ 2 * idxIn + 0 ];
			tess.verts[ idxOut ].texCoords[ 1 ] = model->texcoords[ 2 * idxIn + 1 ];
		}
	}

	tess.numIndexes  += 3 * surf->num_triangles;
	tess.numVertexes += surf->num_vertexes;
}

//===========================================================================

/*
====================
Tess_SurfaceEntity

Entities that have a single procedurally generated surface
====================
*/
static void Tess_SurfaceEntity( surfaceType_t* )
{
	GLimp_LogComment( "--- Tess_SurfaceEntity ---\n" );

	switch ( backEnd.currentEntity->e.reType )
	{
		case refEntityType_t::RT_SPRITE:
			Tess_SurfaceSprite();
			break;
		default:
			break;
	}
}

static void Tess_SurfaceBad( surfaceType_t* )
{
	GLimp_LogComment( "--- Tess_SurfaceBad ---\n" );

	Log::Notice("Bad surface tesselated." );
}

static void Tess_SurfaceFlare( srfFlare_t *surf )
{
	vec3_t dir;
	vec3_t origin;
	float  d;

	GLimp_LogComment( "--- Tess_SurfaceFlare ---\n" );

	Tess_CheckVBOAndIBO( tess.vbo, tess.ibo );

	VectorMA( surf->origin, 2.0, surf->normal, origin );
	VectorSubtract( origin, backEnd.viewParms.orientation.origin, dir );
	VectorNormalize( dir );
	d = -DotProduct( dir, surf->normal );
	VectorMA( origin, r_ignore->value, dir, origin );

	if ( d < 0 )
	{
		return;
	}

	RB_AddFlare( ( void * ) surf, tess.fogNum, origin, surf->color, surf->normal );
}

/*
==============
Tess_SurfaceVBOMesh
==============
*/
static void Tess_SurfaceVBOMesh( srfVBOMesh_t *srf )
{
	GLimp_LogComment( "--- Tess_SurfaceVBOMesh ---\n" );


	Tess_SurfaceVBO( srf->vbo, srf->ibo, srf->numIndexes, srf->firstIndex );
}

/*
==============
Tess_SurfaceVBOMDVMesh
==============
*/
void Tess_SurfaceVBOMDVMesh( srfVBOMDVMesh_t *surface )
{
	refEntity_t *refEnt;

	GLimp_LogComment( "--- Tess_SurfaceVBOMDVMesh ---\n" );

	if ( !surface->vbo || !surface->ibo )
	{
		return;
	}

	Tess_EndBegin();

	R_BindVBO( surface->vbo );
	R_BindIBO( surface->ibo );

	tess.numIndexes = surface->numIndexes;
	tess.numVertexes = surface->numVerts;
	tess.vboVertexAnimation = true;

	refEnt = &backEnd.currentEntity->e;

	if ( refEnt->oldframe == refEnt->frame )
	{
		glState.vertexAttribsInterpolation = 0;
	}
	else
	{
		glState.vertexAttribsInterpolation = ( 1.0 - refEnt->backlerp );
	}

	glState.vertexAttribsOldFrame = refEnt->oldframe;
	glState.vertexAttribsNewFrame = refEnt->frame;

	Tess_End();
}

/*
==============
Tess_SurfaceVBOMD5Mesh
==============
*/
static void Tess_SurfaceVBOMD5Mesh( srfVBOMD5Mesh_t *srf )
{
	int        i;
	md5Model_t *model;

	GLimp_LogComment( "--- Tess_SurfaceVBOMD5Mesh ---\n" );

	if ( !srf->vbo || !srf->ibo )
	{
		return;
	}

	Tess_EndBegin();

	R_BindVBO( srf->vbo );
	R_BindIBO( srf->ibo );

	tess.numIndexes = srf->numIndexes;
	tess.numVertexes = srf->numVerts;

	model = srf->md5Model;

	tess.vboVertexSkinning = true;
	tess.numBones = srf->numBoneRemap;

	for ( i = 0; i < srf->numBoneRemap; i++ )
	{
		refBone_t *bone = &backEnd.currentEntity->e.skeleton.bones[ srf->boneRemapInverse[ i ] ];

		if ( backEnd.currentEntity->e.skeleton.type == refSkeletonType_t::SK_ABSOLUTE )
		{
			TransInitRotationQuat( model->bones[ srf->boneRemapInverse[ i ] ].rotation, &tess.bones[ i ] );
			TransAddTranslation( model->bones[ srf->boneRemapInverse[ i ] ].origin, &tess.bones[ i ] );
			TransInverse( &tess.bones[ i ], &tess.bones[ i ] );
			TransCombine( &tess.bones[ i ], &bone->t, &tess.bones[ i ] );
		} else {
			TransInit( &tess.bones[ i ] );
		}
		TransAddScale( backEnd.currentEntity->e.skeleton.scale, &tess.bones[ i ] );
		TransInsScale( model->internalScale, &tess.bones[ i ] );
	}

	Tess_End();
}

static void Tess_SurfaceSkip( void* )
{
}

// *INDENT-OFF*
void ( *rb_surfaceTable[ Util::ordinal(surfaceType_t::SF_NUM_SURFACE_TYPES) ] )( void * ) =
{
	( void ( * )( void * ) ) Tess_SurfaceBad,  // SF_BAD,
	( void ( * )( void * ) ) Tess_SurfaceSkip,  // SF_SKIP,
	( void ( * )( void * ) ) Tess_SurfaceFace,  // SF_FACE,
	( void ( * )( void * ) ) Tess_SurfaceGrid,  // SF_GRID,
	( void ( * )( void * ) ) Tess_SurfaceTriangles,  // SF_TRIANGLES,
	( void ( * )( void * ) ) Tess_SurfacePolychain,  // SF_POLY,
	( void ( * )( void * ) ) Tess_SurfacePolybuffer,  // SF_POLYBUFFER,
	( void ( * )( void * ) ) Tess_SurfaceDecal,  // SF_DECAL
	( void ( * )( void * ) ) Tess_SurfaceMDV,  // SF_MDV,
	( void ( * )( void * ) ) Tess_SurfaceMD5,  // SF_MD5,
	( void ( * )( void * ) ) Tess_SurfaceIQM,  // SF_IQM,

	( void ( * )( void * ) ) Tess_SurfaceFlare,  // SF_FLARE,
	( void ( * )( void * ) ) Tess_SurfaceEntity,  // SF_ENTITY
	( void ( * )( void * ) ) Tess_SurfaceVBOMesh,  // SF_VBO_MESH
	( void ( * )( void * ) ) Tess_SurfaceVBOMD5Mesh,  // SF_VBO_MD5MESH
	( void ( * )( void * ) ) Tess_SurfaceVBOMDVMesh  // SF_VBO_MDVMESH
};
