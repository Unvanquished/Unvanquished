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
void Tess_EndBegin( void )
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
	if ( glState.currentVBO != NULL && glState.currentIBO != NULL )
	{
		Tess_CheckVBOAndIBO( tess.vbo, tess.ibo );
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
		ri.Error( ERR_DROP, "Tess_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES );
	}

	if ( indexes >= SHADER_MAX_INDEXES )
	{
		ri.Error( ERR_DROP, "Tess_CheckOverflow: indexes > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES );
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
		VectorCopy( vert->xyz, tess.xyz[ tess.numVertexes + i ] );
		VectorCopy( vert->normal, tess.normals[ tess.numVertexes + i ] );
		VectorCopy( vert->binormal, tess.binormals[ tess.numVertexes + i ] );
		VectorCopy( vert->tangent, tess.tangents[ tess.numVertexes + i ] );

		tess.texCoords[ tess.numVertexes + i ][ 0 ] = vert->st[ 0 ];
		tess.texCoords[ tess.numVertexes + i ][ 1 ] = vert->st[ 1 ];

		tess.lightCoords[ tess.numVertexes + i ][ 0 ] = vert->lightmap[ 0 ];
		tess.lightCoords[ tess.numVertexes + i ][ 1 ] = vert->lightmap[ 1 ];

		Vector4Copy( vert->lightColor, tess.colors[ tess.numVertexes + i ] );

#if !defined( COMPAT_ET ) && !defined( COMPAT_Q3A )
		VectorCopy( vert->lightDirection, tess.lightDirections[ tess.numVertexes + i ] );
		Vector4Copy( vert->paintColor, tess.paintColors[ tess.numVertexes + i ] );
#endif
	}

	tess.numVertexes += numVerts;
	tess.attribsSet =  ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_COLOR | ATTR_NORMAL | ATTR_TANGENT | ATTR_BINORMAL;

#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )
	tess.attribsSet |= ATTR_PAINTCOLOR | ATTR_LIGHTDIRECTION;
#endif
}

static qboolean Tess_SurfaceVBO( VBO_t *vbo, IBO_t *ibo, int numVerts, int numIndexes, int firstIndex )
{
	if ( !vbo || !ibo )
	{
		return qfalse;
	}

	if ( tess.skipVBO || ShaderRequiresCPUDeforms( tess.surfaceShader ) || tess.stageIteratorFunc == &Tess_StageIteratorSky )
	{
		return qfalse;
	}

	Tess_CheckVBOAndIBO( vbo, ibo );

	tess.multiDrawIndexes[ tess.multiDrawPrimitives ] = ( glIndex_t * ) BUFFER_OFFSET( firstIndex * sizeof( glIndex_t ) );
	tess.multiDrawCounts[ tess.multiDrawPrimitives ] = numIndexes;

	tess.multiDrawPrimitives++;
	return qtrue;
}

/*
==============
Tess_AddQuadStampExt
==============
*/
void Tess_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, const vec4_t color, float s1, float t1, float s2, float t2 )
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

	tess.xyz[ ndx ][ 0 ] = origin[ 0 ] + left[ 0 ] + up[ 0 ];
	tess.xyz[ ndx ][ 1 ] = origin[ 1 ] + left[ 1 ] + up[ 1 ];
	tess.xyz[ ndx ][ 2 ] = origin[ 2 ] + left[ 2 ] + up[ 2 ];
	tess.xyz[ ndx ][ 3 ] = 1;

	tess.xyz[ ndx + 1 ][ 0 ] = origin[ 0 ] - left[ 0 ] + up[ 0 ];
	tess.xyz[ ndx + 1 ][ 1 ] = origin[ 1 ] - left[ 1 ] + up[ 1 ];
	tess.xyz[ ndx + 1 ][ 2 ] = origin[ 2 ] - left[ 2 ] + up[ 2 ];
	tess.xyz[ ndx + 1 ][ 3 ] = 1;

	tess.xyz[ ndx + 2 ][ 0 ] = origin[ 0 ] - left[ 0 ] - up[ 0 ];
	tess.xyz[ ndx + 2 ][ 1 ] = origin[ 1 ] - left[ 1 ] - up[ 1 ];
	tess.xyz[ ndx + 2 ][ 2 ] = origin[ 2 ] - left[ 2 ] - up[ 2 ];
	tess.xyz[ ndx + 2 ][ 3 ] = 1;

	tess.xyz[ ndx + 3 ][ 0 ] = origin[ 0 ] + left[ 0 ] - up[ 0 ];
	tess.xyz[ ndx + 3 ][ 1 ] = origin[ 1 ] + left[ 1 ] - up[ 1 ];
	tess.xyz[ ndx + 3 ][ 2 ] = origin[ 2 ] + left[ 2 ] - up[ 2 ];
	tess.xyz[ ndx + 3 ][ 3 ] = 1;

	// constant normal all the way around
	VectorSubtract( vec3_origin, backEnd.viewParms.orientation.axis[ 0 ], normal );

	tess.normals[ ndx ][ 0 ] = tess.normals[ ndx + 1 ][ 0 ] = tess.normals[ ndx + 2 ][ 0 ] = tess.normals[ ndx + 3 ][ 0 ] = normal[ 0 ];
	tess.normals[ ndx ][ 1 ] = tess.normals[ ndx + 1 ][ 1 ] = tess.normals[ ndx + 2 ][ 1 ] = tess.normals[ ndx + 3 ][ 1 ] = normal[ 1 ];
	tess.normals[ ndx ][ 2 ] = tess.normals[ ndx + 1 ][ 2 ] = tess.normals[ ndx + 2 ][ 2 ] = tess.normals[ ndx + 3 ][ 2 ] = normal[ 2 ];

	// standard square texture coordinates
	tess.texCoords[ ndx ][ 0 ] = s1;
	tess.texCoords[ ndx ][ 1 ] = t1;

	tess.texCoords[ ndx + 1 ][ 0 ] = s2;
	tess.texCoords[ ndx + 1 ][ 1 ] = t1;

	tess.texCoords[ ndx + 2 ][ 0 ] = s2;
	tess.texCoords[ ndx + 2 ][ 1 ] = t2;

	tess.texCoords[ ndx + 3 ][ 0 ] = s1;
	tess.texCoords[ ndx + 3 ][ 1 ] = t2;

	// constant color all the way around
	// should this be identity and let the shader specify from entity?

	for ( i = 0; i < 4; i++ )
	{
		Vector4Copy( color, tess.colors[ ndx + i ] );
	}

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.attribsSet |= ATTR_POSITION | ATTR_NORMAL | ATTR_COLOR | ATTR_TEXCOORD;
}

/*
==============
Tess_AddQuadStamp
==============
*/
void Tess_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, const vec4_t color )
{
	Tess_AddQuadStampExt( origin, left, up, color, 0, 0, 1, 1 );
}

/*
==============
Tess_AddQuadStampExt2
==============
*/
void Tess_AddQuadStampExt2( vec4_t quadVerts[ 4 ], const vec4_t color, float s1, float t1, float s2, float t2, qboolean calcNormals )
{
	int    i;
	vec4_t plane;
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

	Vector4Copy( quadVerts[ 0 ], tess.xyz[ ndx + 0 ] );
	Vector4Copy( quadVerts[ 1 ], tess.xyz[ ndx + 1 ] );
	Vector4Copy( quadVerts[ 2 ], tess.xyz[ ndx + 2 ] );
	Vector4Copy( quadVerts[ 3 ], tess.xyz[ ndx + 3 ] );

	tess.attribsSet |= ATTR_POSITION | ATTR_COLOR | ATTR_TEXCOORD | ATTR_NORMAL;

	// constant normal all the way around
	if ( calcNormals )
	{
		PlaneFromPoints( plane, quadVerts[ 0 ], quadVerts[ 1 ], quadVerts[ 2 ] );
	}
	else
	{
		VectorNegate( backEnd.viewParms.orientation.axis[ 0 ], plane );
	}

	tess.normals[ ndx ][ 0 ] = tess.normals[ ndx + 1 ][ 0 ] = tess.normals[ ndx + 2 ][ 0 ] = tess.normals[ ndx + 3 ][ 0 ] = plane[ 0 ];
	tess.normals[ ndx ][ 1 ] = tess.normals[ ndx + 1 ][ 1 ] = tess.normals[ ndx + 2 ][ 1 ] = tess.normals[ ndx + 3 ][ 1 ] = plane[ 1 ];
	tess.normals[ ndx ][ 2 ] = tess.normals[ ndx + 1 ][ 2 ] = tess.normals[ ndx + 2 ][ 2 ] = tess.normals[ ndx + 3 ][ 2 ] = plane[ 2 ];

	// standard square texture coordinates
	tess.texCoords[ ndx ][ 0 ] = s1;
	tess.texCoords[ ndx ][ 1 ] = t1;

	tess.texCoords[ ndx + 1 ][ 0 ] = s2;
	tess.texCoords[ ndx + 1 ][ 1 ] = t1;

	tess.texCoords[ ndx + 2 ][ 0 ] = s2;
	tess.texCoords[ ndx + 2 ][ 1 ] = t2;

	tess.texCoords[ ndx + 3 ][ 0 ] = s1;
	tess.texCoords[ ndx + 3 ][ 1 ] = t2;

	// constant color all the way around
	// should this be identity and let the shader specify from entity?

	for ( i = 0; i < 4; i++ )
	{
		Vector4Copy( color, tess.colors[ ndx + i ] );
	}

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

/*
==============
Tess_AddQuadStamp2
==============
*/
void Tess_AddQuadStamp2( vec4_t quadVerts[ 4 ], const vec4_t color )
{
	Tess_AddQuadStampExt2( quadVerts, color, 0, 0, 1, 1, qfalse );
}

void Tess_AddQuadStamp2WithNormals( vec4_t quadVerts[ 4 ], const vec4_t color )
{
	Tess_AddQuadStampExt2( quadVerts, color, 0, 0, 1, 1, qtrue );
}

void Tess_AddTetrahedron( vec4_t tetraVerts[ 4 ], const vec4_t color )
{
	int k;

	Tess_CheckOverflow( 12, 12 );

	// ground triangle
	for ( k = 0; k < 3; k++ )
	{
		Vector4Copy( tetraVerts[ k ], tess.xyz[ tess.numVertexes ] );
		Vector4Copy( color, tess.colors[ tess.numVertexes ] );
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;
	}

	// side triangles
	for ( k = 0; k < 3; k++ )
	{
		Vector4Copy( tetraVerts[ 3 ], tess.xyz[ tess.numVertexes ] );  // offset
		Vector4Copy( color, tess.colors[ tess.numVertexes ] );
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;

		Vector4Copy( tetraVerts[ k ], tess.xyz[ tess.numVertexes ] );
		Vector4Copy( color, tess.colors[ tess.numVertexes ] );
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;

		Vector4Copy( tetraVerts[( k + 1 ) % 3 ], tess.xyz[ tess.numVertexes ] );
		Vector4Copy( color, tess.colors[ tess.numVertexes ] );
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_COLOR;
}

void Tess_AddCube( const vec3_t position, const vec3_t minSize, const vec3_t maxSize, const vec4_t color )
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

void Tess_AddCubeWithNormals( const vec3_t position, const vec3_t minSize, const vec3_t maxSize, const vec4_t color )
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
Tess_UpdateVBOs

Tr3B: update the default VBO to replace the client side vertex arrays
==============
*/
void Tess_UpdateVBOs( uint32_t attribBits )
{
	if ( r_logFile->integer )
	{
		GLimp_LogComment( va( "--- Tess_UpdateVBOs( attribBits = %i ) ---\n", attribBits ) );
	}

	GL_CheckErrors();

	// update the default VBO
	if ( tess.numVertexes > 0 && tess.numVertexes <= SHADER_MAX_VERTEXES )
	{
		R_BindVBO( tess.vbo );

		GL_CheckErrors();

		assert( ( attribBits & ATTR_BITS ) != 0 );

		GL_VertexAttribsState( attribBits );

		if ( attribBits & ATTR_POSITION )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( va( "glBufferSubData( ATTR_POSITION, vbo = '%s', numVertexes = %i )\n", tess.vbo->name, tess.numVertexes ) );
			}

			glBufferSubData( GL_ARRAY_BUFFER, tess.vbo->attribs[ ATTR_INDEX_POSITION ].ofs, tess.numVertexes * sizeof( vec4_t ), tess.xyz );
		}

		if ( attribBits & ATTR_TEXCOORD )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( va( "glBufferSubData( ATTR_TEXCOORD, vbo = '%s', numVertexes = %i )\n", tess.vbo->name, tess.numVertexes ) );
			}

			glBufferSubData( GL_ARRAY_BUFFER, tess.vbo->attribs[ ATTR_INDEX_TEXCOORD ].ofs, tess.numVertexes * sizeof( vec2_t ), tess.texCoords );
		}

		if ( attribBits & ATTR_LIGHTCOORD )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( va( "glBufferSubData( ATTR_LIGHTCOORD, vbo = '%s', numVertexes = %i )\n", tess.vbo->name, tess.numVertexes ) );
			}

			glBufferSubData( GL_ARRAY_BUFFER, tess.vbo->attribs[ ATTR_INDEX_LIGHTCOORD ].ofs, tess.numVertexes * sizeof( vec2_t ), tess.lightCoords );
		}

		if ( attribBits & ATTR_TANGENT )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( va( "glBufferSubData( ATTR_TANGENT, vbo = '%s', numVertexes = %i )\n", tess.vbo->name, tess.numVertexes ) );
			}

			glBufferSubData( GL_ARRAY_BUFFER, tess.vbo->attribs[ ATTR_INDEX_TANGENT ].ofs, tess.numVertexes * sizeof( vec4_t ), tess.tangents );
		}

		if ( attribBits & ATTR_BINORMAL )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( va( "glBufferSubData( ATTR_BINORMAL, vbo = '%s', numVertexes = %i )\n", tess.vbo->name, tess.numVertexes ) );
			}

			glBufferSubData( GL_ARRAY_BUFFER, tess.vbo->attribs[ ATTR_INDEX_BINORMAL ].ofs, tess.numVertexes * sizeof( vec4_t ), tess.binormals );
		}

		if ( attribBits & ATTR_NORMAL )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( va( "glBufferSubData( ATTR_NORMAL, vbo = '%s', numVertexes = %i )\n", tess.vbo->name, tess.numVertexes ) );
			}

			glBufferSubData( GL_ARRAY_BUFFER, tess.vbo->attribs[ ATTR_INDEX_NORMAL ].ofs, tess.numVertexes * sizeof( vec4_t ), tess.normals );
		}

		if ( attribBits & ATTR_COLOR )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( va( "glBufferSubData( ATTR_COLOR, vbo = '%s', numVertexes = %i )\n", tess.vbo->name, tess.numVertexes ) );
			}

			glBufferSubData( GL_ARRAY_BUFFER, tess.vbo->attribs[ ATTR_INDEX_COLOR ].ofs, tess.numVertexes * sizeof( vec4_t ), tess.colors );
		}

#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )

		if ( attribBits & ATTR_PAINTCOLOR )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( va( "glBufferSubData( ATTR_PAINTCOLOR, vbo = '%s', numVertexes = %i )\n", tess.vbo->name, tess.numVertexes ) );
			}

			glBufferSubData( GL_ARRAY_BUFFER, tess.vbo->attribs[ ATTR_INDEX_PAINTCOLOR ].ofs, tess.numVertexes * sizeof( vec4_t ), tess.paintColors );
		}

#endif
		if ( attribBits & ATTR_AMBIENTLIGHT )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( va( "glBufferSubData( ATTR_AMBIENTLIGHT, vbo = '%s', numVertexes = %i )\n", tess.vbo->name, tess.numVertexes ) );
			}

			glBufferSubData( GL_ARRAY_BUFFER, tess.vbo->attribs[ ATTR_INDEX_AMBIENTLIGHT ].ofs, tess.numVertexes * sizeof( vec4_t ), tess.ambientLights );
		}

		if ( attribBits & ATTR_DIRECTEDLIGHT )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( va( "glBufferSubData( ATTR_DIRECTEDLIGHT, vbo = '%s', numVertexes = %i )\n", tess.vbo->name, tess.numVertexes ) );
			}

			glBufferSubData( GL_ARRAY_BUFFER, tess.vbo->attribs[ ATTR_INDEX_DIRECTEDLIGHT ].ofs, tess.numVertexes * sizeof( vec4_t ), tess.directedLights );
		}

		if ( attribBits & ATTR_LIGHTDIRECTION )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( va( "glBufferSubData( ATTR_LIGHTDIRECTION, vbo = '%s', numVertexes = %i )\n", tess.vbo->name, tess.numVertexes ) );
			}

			glBufferSubData( GL_ARRAY_BUFFER, tess.vbo->attribs[ ATTR_INDEX_LIGHTDIRECTION ].ofs, tess.numVertexes * sizeof( vec4_t ), tess.lightDirections );
		}

	}

	GL_CheckErrors();

	// update the default IBO
	if ( tess.numIndexes > 0 && tess.numIndexes <= SHADER_MAX_INDEXES )
	{
		R_BindIBO( tess.ibo );

		glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, 0, tess.numIndexes * sizeof( glIndex_t ), tess.indexes );
	}

	GL_CheckErrors();
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

	Vector4Copy( quadVerts[ 0 ], tess.xyz[ tess.numVertexes ] );
	tess.texCoords[ tess.numVertexes ][ 0 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 1 ] = 0;
	tess.colors[ tess.numVertexes ][ 0 ] = 1;
	tess.colors[ tess.numVertexes ][ 1 ] = 1;
	tess.colors[ tess.numVertexes ][ 2 ] = 1;
	tess.colors[ tess.numVertexes ][ 3 ] = 1;
	tess.numVertexes++;

	Vector4Copy( quadVerts[ 1 ], tess.xyz[ tess.numVertexes ] );
	tess.texCoords[ tess.numVertexes ][ 0 ] = 1;
	tess.texCoords[ tess.numVertexes ][ 1 ] = 0;
	tess.colors[ tess.numVertexes ][ 0 ] = 1;
	tess.colors[ tess.numVertexes ][ 1 ] = 1;
	tess.colors[ tess.numVertexes ][ 2 ] = 1;
	tess.colors[ tess.numVertexes ][ 3 ] = 1;
	tess.numVertexes++;

	Vector4Copy( quadVerts[ 2 ], tess.xyz[ tess.numVertexes ] );
	tess.texCoords[ tess.numVertexes ][ 0 ] = 1;
	tess.texCoords[ tess.numVertexes ][ 1 ] = 1;
	tess.colors[ tess.numVertexes ][ 0 ] = 1;
	tess.colors[ tess.numVertexes ][ 1 ] = 1;
	tess.colors[ tess.numVertexes ][ 2 ] = 1;
	tess.colors[ tess.numVertexes ][ 3 ] = 1;
	tess.numVertexes++;

	Vector4Copy( quadVerts[ 3 ], tess.xyz[ tess.numVertexes ] );
	tess.texCoords[ tess.numVertexes ][ 0 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 1 ] = 1;
	tess.colors[ tess.numVertexes ][ 0 ] = 1;
	tess.colors[ tess.numVertexes ][ 1 ] = 1;
	tess.colors[ tess.numVertexes ][ 2 ] = 1;
	tess.colors[ tess.numVertexes ][ 3 ] = 1;
	tess.numVertexes++;

	tess.indexes[ tess.numIndexes++ ] = 0;
	tess.indexes[ tess.numIndexes++ ] = 1;
	tess.indexes[ tess.numIndexes++ ] = 2;
	tess.indexes[ tess.numIndexes++ ] = 0;
	tess.indexes[ tess.numIndexes++ ] = 2;
	tess.indexes[ tess.numIndexes++ ] = 3;

	Tess_UpdateVBOs( ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR );

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
static void Tess_SurfaceSprite( void )
{
	vec3_t left, up;
	float  radius;
	vec4_t color;

	GLimp_LogComment( "--- Tess_SurfaceSprite ---\n" );

	// calculate the xyz locations for the four corners
	radius = backEnd.currentEntity->e.radius;

	if ( backEnd.currentEntity->e.rotation == 0 )
	{
		VectorScale( backEnd.viewParms.orientation.axis[ 1 ], radius, left );
		VectorScale( backEnd.viewParms.orientation.axis[ 2 ], radius, up );
	}
	else
	{
		float s, c;
		float ang;

		ang = M_PI * backEnd.currentEntity->e.rotation / 180;
		s = sin( ang );
		c = cos( ang );

		VectorScale( backEnd.viewParms.orientation.axis[ 1 ], c * radius, left );
		VectorMA( left, -s * radius, backEnd.viewParms.orientation.axis[ 2 ], left );

		VectorScale( backEnd.viewParms.orientation.axis[ 2 ], c * radius, up );
		VectorMA( up, s * radius, backEnd.viewParms.orientation.axis[ 1 ], up );
	}

	if ( backEnd.viewParms.isMirror )
	{
		VectorSubtract( vec3_origin, left, left );
	}

	color[ 0 ] = backEnd.currentEntity->e.shaderRGBA[ 0 ] * ( 1.0 / 255.0 );
	color[ 1 ] = backEnd.currentEntity->e.shaderRGBA[ 1 ] * ( 1.0 / 255.0 );
	color[ 2 ] = backEnd.currentEntity->e.shaderRGBA[ 2 ] * ( 1.0 / 255.0 );
	color[ 3 ] = backEnd.currentEntity->e.shaderRGBA[ 3 ] * ( 1.0 / 255.0 );

	Tess_AddQuadStamp( backEnd.currentEntity->e.origin, left, up, color );
}

/*
==============
VectorArrayNormalize

The inputs to this routing seem to always be close to length = 1.0 (about 0.6 to 2.0)
This means that we don't have to worry about zero length or enormously long vectors.
==============
*/
static void VectorArrayNormalize( vec4_t *normals, unsigned int count )
{
//    assert(count);

#if idppc
	{
		register float half = 0.5;
		register float one = 1.0;
		float          *components = ( float * ) normals;

		// Vanilla PPC code, but since PPC has a reciprocal square root estimate instruction,
		// runs *much* faster than calling sqrt().  We'll use a single Newton-Raphson
		// refinement step to get a little more precision.  This seems to yeild results
		// that are correct to 3 decimal places and usually correct to at least 4 (sometimes 5).
		// (That is, for the given input range of about 0.6 to 2.0).
		do
		{
			float x, y, z;
			float B, y0, y1;

			x = components[ 0 ];
			y = components[ 1 ];
			z = components[ 2 ];
			components += 4;
			B = x * x + y * y + z * z;

#ifdef __GNUC__
			asm( "frsqrte %0,%1" : "=f"( y0 ) : "f"( B ) );
#else
			y0 = __frsqrte( B );
#endif
			y1 = y0 + half * y0 * ( one - B * y0 * y0 );

			x = x * y1;
			y = y * y1;
			components[ -4 ] = x;
			z = z * y1;
			components[ -3 ] = y;
			components[ -2 ] = z;
		}
		while ( count-- );
	}
#else // No assembly version for this architecture, or C_ONLY defined

	// given the input, it's safe to call VectorNormalizeFast
	while ( count-- )
	{
		VectorNormalizeFast( normals[ 0 ] );
		normals++;
	}

#endif
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
		VectorCopy( p->verts[ i ].xyz, tess.xyz[ tess.numVertexes + i ] );
		tess.xyz[ tess.numVertexes + i ][ 3 ] = 1;

		tess.texCoords[ tess.numVertexes + i ][ 0 ] = p->verts[ i ].st[ 0 ];
		tess.texCoords[ tess.numVertexes + i ][ 1 ] = p->verts[ i ].st[ 1 ];

		tess.colors[ tess.numVertexes + i ][ 0 ] = p->verts[ i ].modulate[ 0 ] * ( 1.0 / 255.0 );
		tess.colors[ tess.numVertexes + i ][ 1 ] = p->verts[ i ].modulate[ 1 ] * ( 1.0 / 255.0 );
		tess.colors[ tess.numVertexes + i ][ 2 ] = p->verts[ i ].modulate[ 2 ] * ( 1.0 / 255.0 );
		tess.colors[ tess.numVertexes + i ][ 3 ] = p->verts[ i ].modulate[ 3 ] * ( 1.0 / 255.0 );

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
		const float *t0, *t1, *t2;
		vec3_t      tangent;
		vec3_t      binormal;
		vec3_t      normal;
		glIndex_t   *indices;

		for ( i = 0; i < numVertexes; i++ )
		{
			VectorClear( tess.tangents[ tess.numVertexes + i ] );
			VectorClear( tess.binormals[ tess.numVertexes + i ] );
			VectorClear( tess.normals[ tess.numVertexes + i ] );

			R_LightForPoint( tess.xyz[ tess.numVertexes + i ],
					 tess.ambientLights[ tess.numVertexes + i ],
					 tess.directedLights[ tess.numVertexes + i ],
					 tess.lightDirections[ tess.numVertexes + i ] );
		}

		for ( i = 0, indices = tess.indexes + tess.numIndexes; i < numIndexes; i += 3, indices += 3 )
		{
			v0 = tess.xyz[ indices[ 0 ] ];
			v1 = tess.xyz[ indices[ 1 ] ];
			v2 = tess.xyz[ indices[ 2 ] ];

			t0 = tess.texCoords[ indices[ 0 ] ];
			t1 = tess.texCoords[ indices[ 1 ] ];
			t2 = tess.texCoords[ indices[ 2 ] ];

			R_CalcTangentSpaceFast( tangent, binormal, normal, v0, v1, v2, t0, t1, t2 );

			for ( j = 0; j < 3; j++ )
			{
				v = tess.tangents[ indices[ j ] ];
				VectorAdd( v, tangent, v );
				v = tess.binormals[ indices[ j ] ];
				VectorAdd( v, binormal, v );
				v = tess.normals[ indices[ j ] ];
				VectorAdd( v, normal, v );
			}
		}

		VectorArrayNormalize( ( vec4_t * ) tess.tangents[ tess.numVertexes ], numVertexes );
		VectorArrayNormalize( ( vec4_t * ) tess.binormals[ tess.numVertexes ], numVertexes );
		VectorArrayNormalize( ( vec4_t * ) tess.normals[ tess.numVertexes ], numVertexes );

		tess.attribsSet |= ATTR_NORMAL | ATTR_BINORMAL | ATTR_TANGENT | ATTR_LIGHTDIRECTION | ATTR_AMBIENTLIGHT | ATTR_DIRECTEDLIGHT;
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

	numIndexes = MIN( surf->pPolyBuffer->numIndicies, MAX_PB_INDICIES );
	indices = surf->pPolyBuffer->indicies;

	for ( i = 0; i < numIndexes; i++ )
	{
		tess.indexes[ tess.numIndexes + i ] = tess.numVertexes + indices[ i ];
	}

	tess.numIndexes += numIndexes;

	numVertexes = MIN( surf->pPolyBuffer->numVerts, MAX_PB_VERTS );
	xyzw = &surf->pPolyBuffer->xyz[ 0 ][ 0 ];
	st = &surf->pPolyBuffer->st[ 0 ][ 0 ];
	color = &surf->pPolyBuffer->color[ 0 ][ 0 ];

	for ( i = 0; i < numVertexes; i++, xyzw += 4, st += 2, color += 4 )
	{
		VectorCopy( xyzw, tess.xyz[ tess.numVertexes + i ] );
		tess.xyz[ tess.numVertexes + i ][ 3 ] = 1;

		tess.texCoords[ tess.numVertexes + i ][ 0 ] = st[ 0 ];
		tess.texCoords[ tess.numVertexes + i ][ 1 ] = st[ 1 ];

		tess.colors[ tess.numVertexes + i ][ 0 ] = color[ 0 ] * ( 1.0 / 255.0 );
		tess.colors[ tess.numVertexes + i ][ 1 ] = color[ 1 ] * ( 1.0 / 255.0 );
		tess.colors[ tess.numVertexes + i ][ 2 ] = color[ 2 ] * ( 1.0 / 255.0 );
		tess.colors[ tess.numVertexes + i ][ 3 ] = color[ 3 ] * ( 1.0 / 255.0 );
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
		VectorCopy( srf->verts[ i ].xyz, tess.xyz[ tess.numVertexes + i ] );
		tess.xyz[ tess.numVertexes + i ][ 3 ] = 1;

		tess.texCoords[ tess.numVertexes + i ][ 0 ] = srf->verts[ i ].st[ 0 ];
		tess.texCoords[ tess.numVertexes + i ][ 1 ] = srf->verts[ i ].st[ 1 ];

		tess.colors[ tess.numVertexes + i ][ 0 ] = srf->verts[ i ].modulate[ 0 ] * ( 1.0 / 255.0 );
		tess.colors[ tess.numVertexes + i ][ 1 ] = srf->verts[ i ].modulate[ 1 ] * ( 1.0 / 255.0 );
		tess.colors[ tess.numVertexes + i ][ 2 ] = srf->verts[ i ].modulate[ 2 ] * ( 1.0 / 255.0 );
		tess.colors[ tess.numVertexes + i ][ 3 ] = srf->verts[ i ].modulate[ 3 ] * ( 1.0 / 255.0 );
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

	if ( !r_vboFaces->integer || !Tess_SurfaceVBO( srf->vbo, srf->ibo, srf->numVerts, srf->numTriangles * 3, srf->firstTriangle * 3 ) )
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

	if ( !r_vboCurves->integer || !Tess_SurfaceVBO( srf->vbo, srf->ibo, srf->numVerts, srf->numTriangles * 3, srf->firstTriangle * 3 ) )
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

	if ( !r_vboTriangles->integer || !Tess_SurfaceVBO( srf->vbo, srf->ibo, srf->numVerts, srf->numTriangles * 3, srf->firstTriangle * 3 ) )
	{
		Tess_SurfaceVertsAndTris( srf->verts, srf->triangles, srf->numVerts, srf->numTriangles );
	}
}

/*
==============
Tess_SurfaceBeam
==============
*/
static void Tess_SurfaceBeam( void )
{
#if 1

	GLimp_LogComment( "--- Tess_SurfaceBeam ---\n" );

	// TODO rewrite without glBegin/glEnd

#else
#define NUM_BEAM_SEGS 6
	refEntity_t *e;
	int         i;
	vec3_t      perpvec;
	vec3_t      direction, normalized_direction;
	vec3_t      start_points[ NUM_BEAM_SEGS ], end_points[ NUM_BEAM_SEGS ];
	vec3_t      oldorigin, origin;

	GLimp_LogComment( "--- Tess_SurfaceBeam ---\n" );

	if ( glState.currentVBO != tess.vbo || glState.currentIBO != tess.ibo )
	{
		Tess_EndBegin();

		R_BindVBO( tess.vbo );
		R_BindIBO( tess.ibo );
	}

	e = &backEnd.currentEntity->e;

	oldorigin[ 0 ] = e->oldorigin[ 0 ];
	oldorigin[ 1 ] = e->oldorigin[ 1 ];
	oldorigin[ 2 ] = e->oldorigin[ 2 ];

	origin[ 0 ] = e->origin[ 0 ];
	origin[ 1 ] = e->origin[ 1 ];
	origin[ 2 ] = e->origin[ 2 ];

	normalized_direction[ 0 ] = direction[ 0 ] = oldorigin[ 0 ] - origin[ 0 ];
	normalized_direction[ 1 ] = direction[ 1 ] = oldorigin[ 1 ] - origin[ 1 ];
	normalized_direction[ 2 ] = direction[ 2 ] = oldorigin[ 2 ] - origin[ 2 ];

	if ( VectorNormalize( normalized_direction ) == 0 )
	{
		return;
	}

	PerpendicularVector( perpvec, normalized_direction );

	VectorScale( perpvec, 4, perpvec );

	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		RotatePointAroundVector( start_points[ i ], normalized_direction, perpvec, ( 360.0 / NUM_BEAM_SEGS ) * i );
//      VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[ i ], direction, end_points[ i ] );
	}

	GL_BindProgram( 0 );
	GL_BindToTMU( 0 ,tr.whiteImage );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	glColor3f( 1, 0, 0 );

	glBegin( GL_TRIANGLE_STRIP );

	for ( i = 0; i <= NUM_BEAM_SEGS; i++ )
	{
		glVertex3fv( start_points[ i % NUM_BEAM_SEGS ] );
		glVertex3fv( end_points[ i % NUM_BEAM_SEGS ] );
	}

	glEnd();
#endif
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
//	mdvModel_t     *model;
	mdvXyz_t      *oldVert, *newVert;
	mdvSt_t       *st;
	srfTriangle_t *tri;
//	vec3_t          lightOrigin;
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

	newXyzScale = ( 1.0 - backlerp );
	oldXyzScale = backlerp;

	Tess_CheckOverflow( srf->numVerts, srf->numTriangles * 3 );

//	model = srf->model;

	numIndexes = srf->numTriangles * 3;

	for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
	{
		tess.indexes[ tess.numIndexes + i * 3 + 0 ] = tess.numVertexes + tri->indexes[ 0 ];
		tess.indexes[ tess.numIndexes + i * 3 + 1 ] = tess.numVertexes + tri->indexes[ 1 ];
		tess.indexes[ tess.numIndexes + i * 3 + 2 ] = tess.numVertexes + tri->indexes[ 2 ];
	}

	newVert = srf->verts + ( backEnd.currentEntity->e.frame * srf->numVerts );
	oldVert = srf->verts + ( backEnd.currentEntity->e.oldframe * srf->numVerts );
	st = srf->st;

	numVertexes = srf->numVerts;

	for ( j = 0; j < numVertexes; j++, newVert++, oldVert++, st++ )
	{
		vec3_t tmpVert;

		if ( backlerp == 0 )
		{
			// just copy
			tmpVert[ 0 ] = newVert->xyz[ 0 ] * newXyzScale;
			tmpVert[ 1 ] = newVert->xyz[ 1 ] * newXyzScale;
			tmpVert[ 2 ] = newVert->xyz[ 2 ] * newXyzScale;
		}
		else
		{
			// interpolate the xyz
			tmpVert[ 0 ] = oldVert->xyz[ 0 ] * oldXyzScale + newVert->xyz[ 0 ] * newXyzScale;
			tmpVert[ 1 ] = oldVert->xyz[ 1 ] * oldXyzScale + newVert->xyz[ 1 ] * newXyzScale;
			tmpVert[ 2 ] = oldVert->xyz[ 2 ] * oldXyzScale + newVert->xyz[ 2 ] * newXyzScale;
		}

		tess.xyz[ tess.numVertexes + j ][ 0 ] = tmpVert[ 0 ];
		tess.xyz[ tess.numVertexes + j ][ 1 ] = tmpVert[ 1 ];
		tess.xyz[ tess.numVertexes + j ][ 2 ] = tmpVert[ 2 ];
		tess.xyz[ tess.numVertexes + j ][ 3 ] = 1;

		tess.texCoords[ tess.numVertexes + j ][ 0 ] = st->st[ 0 ];
		tess.texCoords[ tess.numVertexes + j ][ 1 ] = st->st[ 1 ];
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD;

	// calc tangent spaces
	if ( !tess.skipTangentSpaces )
	{
		int         i;
		float       *v;
		const float *v0, *v1, *v2;
		const float *t0, *t1, *t2;
		vec3_t      tangent;
		vec3_t      binormal;
		vec3_t      normal;
		glIndex_t   *indices;

		tess.attribsSet |= ATTR_NORMAL | ATTR_BINORMAL | ATTR_TANGENT;

		for ( i = 0; i < numVertexes; i++ )
		{
			VectorClear( tess.tangents[ tess.numVertexes + i ] );
			VectorClear( tess.binormals[ tess.numVertexes + i ] );
			VectorClear( tess.normals[ tess.numVertexes + i ] );
		}

		for ( i = 0, indices = tess.indexes + tess.numIndexes; i < numIndexes; i += 3, indices += 3 )
		{
			v0 = tess.xyz[ indices[ 0 ] ];
			v1 = tess.xyz[ indices[ 1 ] ];
			v2 = tess.xyz[ indices[ 2 ] ];

			t0 = tess.texCoords[ indices[ 0 ] ];
			t1 = tess.texCoords[ indices[ 1 ] ];
			t2 = tess.texCoords[ indices[ 2 ] ];

			R_CalcTangentSpaceFast( tangent, binormal, normal, v0, v1, v2, t0, t1, t2 );

			for ( j = 0; j < 3; j++ )
			{
				v = tess.tangents[ indices[ j ] ];
				VectorAdd( v, tangent, v );

				v = tess.binormals[ indices[ j ] ];
				VectorAdd( v, binormal, v );

				v = tess.normals[ indices[ j ] ];
				VectorAdd( v, normal, v );
			}
		}

#if 1
		VectorArrayNormalize( ( vec4_t * ) tess.tangents[ tess.numVertexes ], numVertexes );
		VectorArrayNormalize( ( vec4_t * ) tess.binormals[ tess.numVertexes ], numVertexes );
		VectorArrayNormalize( ( vec4_t * ) tess.normals[ tess.numVertexes ], numVertexes );
#else

		for ( i = 0; i < numVertexes; i++ )
		{
			VectorNormalize( tess.tangents[ tess.numVertexes + i ] );
			VectorNormalize( tess.binormals[ tess.numVertexes + i ] );
			VectorNormalize( tess.normals[ tess.numVertexes + i ] );
		}

#endif

		// TEST

		/*
		for(i = 0; i < numVertexes; i++)
		{
		        VectorSet(tess.normals[tess.numVertexes + i], 0, 0, 1);
		}
		*/
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
	int             i, j, k;
	int             numIndexes = 0;
	int             numVertexes;
	md5Model_t      *model;
	md5Vertex_t     *v;
	srfTriangle_t   *tri;

	GLimp_LogComment( "--- Tess_SurfaceMD5 ---\n" );

	Tess_CheckOverflow( srf->numVerts, srf->numTriangles * 3 );

	model = srf->model;

	numIndexes = srf->numTriangles * 3;

	for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
	{
		tess.indexes[ tess.numIndexes + i * 3 + 0 ] = tess.numVertexes + tri->indexes[ 0 ];
		tess.indexes[ tess.numIndexes + i * 3 + 1 ] = tess.numVertexes + tri->indexes[ 1 ];
		tess.indexes[ tess.numIndexes + i * 3 + 2 ] = tess.numVertexes + tri->indexes[ 2 ];
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD;

	if ( tess.skipTangentSpaces )
	{
		// convert bones back to matrices
		for ( i = 0; i < model->numBones; i++ )
		{

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )

			if ( backEnd.currentEntity->e.skeleton.type == SK_ABSOLUTE )
			{
				refBone_t *bone = &backEnd.currentEntity->e.skeleton.bones[ i ];

				TransInitRotationQuat( model->bones[ i ].rotation, &bones[ i ] );
				TransAddTranslation( model->bones[ i ].origin, &bones[ i ] );
				TransInverse( &bones[ i ], &bones[ i ] );
				TransCombine( &bones[ i ], &bone->t, &bones[ i ] );
				TransAddScale( backEnd.currentEntity->e.skeleton.scale, &bones[ i ] );
			}
			else
#endif
			{
				TransInitRotationQuat( model->bones[ i ].rotation, &bones[i] );
				TransAddTranslation( model->bones[ i ].origin, &bones[ i ] );
			}
		}

		// deform the vertices by the lerped bones
		numVertexes = srf->numVerts;

		for ( j = 0, v = srf->verts; j < numVertexes; j++, v++ )
		{
			vec3_t tmp;

			VectorClear( tess.xyz[ tess.numVertexes + j ] );
			for ( k = 0; k < v->numWeights; k++ ) {
				TransformPoint( &bones[ v->boneIndexes[ k ] ],
						v->position, tmp );
				VectorMA( tess.xyz[ tess.numVertexes + j ],
					  v->boneWeights[ k ], tmp,
					  tess.xyz[ tess.numVertexes + j ] );
			}

			tess.texCoords[ tess.numVertexes + j ][ 0 ] = v->texCoords[ 0 ];
			tess.texCoords[ tess.numVertexes + j ][ 1 ] = v->texCoords[ 1 ];
		}
	}
	else
	{
		tess.attribsSet |= ATTR_NORMAL | ATTR_BINORMAL | ATTR_TANGENT;
		
		// convert bones back to matrices
		for ( i = 0; i < model->numBones; i++ )
		{

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )

			if ( backEnd.currentEntity->e.skeleton.type == SK_ABSOLUTE )
			{
				refBone_t *bone = &backEnd.currentEntity->e.skeleton.bones[ i ];
				TransInitRotationQuat( model->bones[ i ].rotation, &bones[ i ] );
				TransAddTranslation( model->bones[ i ].origin, &bones[ i ] );
				TransInverse( &bones[ i ], &bones[ i ] );

				TransCombine( &bones[ i ], &bone->t, &bones[ i ] );
				TransAddScale( backEnd.currentEntity->e.skeleton.scale, &bones[ i ] );
			}
			else
#endif
			{
				TransInitScale( backEnd.currentEntity->e.skeleton.scale, &bones[ i ] );
			}
		}

		// deform the vertices by the lerped bones
		numVertexes = srf->numVerts;

		for ( j = 0, v = srf->verts; j < numVertexes; j++, v++ )
		{
			vec3_t tmp;

			VectorClear( tess.xyz[ tess.numVertexes + j ] );
			VectorClear( tess.normals[ tess.numVertexes + j ] );
			VectorClear( tess.binormals[ tess.numVertexes + j ] );
			VectorClear( tess.tangents[ tess.numVertexes + j ] );

			for( k = 0; k < v->numWeights; k++ ) {
				TransformPoint( &bones[ v->boneIndexes[ k ] ],
						v->position, tmp );
				VectorMA( tess.xyz[ tess.numVertexes + j ],
					  v->boneWeights[ k ], tmp,
					  tess.xyz[ tess.numVertexes + j ] );

				TransformNormalVector( &bones[ v->boneIndexes[ k ] ],
						       v->normal, tmp );
				VectorMA( tess.normals[ tess.numVertexes + j ],
					  v->boneWeights[ k ], tmp,
					  tess.normals[ tess.numVertexes + j ] );

				TransformNormalVector( &bones[ v->boneIndexes[ k ] ],
						       v->tangent, tmp );
				VectorMA( tess.tangents[ tess.numVertexes + j ],
					  v->boneWeights[ k ], tmp,
					  tess.tangents[ tess.numVertexes + j ] );

				TransformNormalVector( &bones[ v->boneIndexes[ k ] ],
						       v->binormal, tmp );
				VectorMA( tess.binormals[ tess.numVertexes + j ],
					  v->boneWeights[ k ], tmp,
					  tess.binormals[ tess.numVertexes + j ] );
			}
			VectorNormalize( tess.normals[ tess.numVertexes + j ] );
			VectorNormalize( tess.tangents[ tess.numVertexes + j ] );
			VectorNormalize( tess.binormals[ tess.numVertexes + j ] );

			tess.texCoords[ tess.numVertexes + j ][ 0 ] = v->texCoords[ 0 ];
			tess.texCoords[ tess.numVertexes + j ][ 1 ] = v->texCoords[ 1 ];
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

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )

		if ( backEnd.currentEntity->e.skeleton.type == SK_ABSOLUTE )
		{
			refBone_t *bone = &backEnd.currentEntity->e.skeleton.bones[ i ];

			TransInverse( &model->joints[ i ], &bones[ i ] );
			TransCombine( &bones[ i ], &bone->t, &bones[ i ] );
		}
		else
#endif
		{
			TransInit( &bones[ i ] );
		}
		TransAddScale( backEnd.currentEntity->e.skeleton.scale, &bones[ i ] );
	}

	if( surf->vbo && surf->ibo ) {
		Com_Memcpy( tess.bones, bones, model->num_joints * sizeof(transform_t) );
		tess.numBones = model->num_joints;
		R_BindVBO( surf->vbo );
		R_BindIBO( surf->ibo );
		tess.vboVertexSkinning = qtrue;

		tess.multiDrawIndexes[ tess.multiDrawPrimitives ] = ((glIndex_t *)NULL) + surf->first_triangle * 3;
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

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD |
	                   ATTR_NORMAL | ATTR_BINORMAL | ATTR_TANGENT;

	// deform the vertices by the lerped bones
	for ( i = 0; i < surf->num_vertexes; i++ )
	{
		int    idxIn = surf->first_vertex + i;
		int    idxOut = tess.numVertexes + i;
		const float weightFactor = 1.0f / 255.0f;
		vec3_t tmp;

		VectorClear( tess.xyz[ idxOut ] );
		VectorClear( tess.normals[ idxOut ] );
		VectorClear( tess.tangents[ idxOut ] );
		VectorClear( tess.binormals[ idxOut ] );
		for ( j = 0; j < 4; j++ ) {
			int bone = model->blendIndexes[ 4 * idxIn + j ];
			float weight = weightFactor * model->blendWeights[ 4 * idxIn + j ];

			TransformPoint( &bones[ bone ],
					&model->positions[ 3 * idxIn ], tmp );
			VectorMA( tess.xyz[ idxOut ], weight, tmp,
				  tess.xyz[ idxOut ] );
			TransformNormalVector( &bones[ bone ],
					       &model->normals[ 3 * idxIn ], tmp );
			VectorMA( tess.normals[ idxOut ], weight, tmp,
				  tess.normals[ idxOut ] );
			TransformNormalVector( &bones[ bone ],
					       &model->tangents[ 3 * idxIn ], tmp );
			VectorMA( tess.tangents[ idxOut ], weight, tmp,
				  tess.tangents[ idxOut ] );
			TransformNormalVector( &bones[ bone ],
					       &model->bitangents[ 3 * idxIn ], tmp );
			VectorMA( tess.binormals[ idxOut ], weight, tmp,
				  tess.binormals[ idxOut ] );
		}
		VectorNormalize( tess.normals[ idxOut ] );
		VectorNormalize( tess.tangents[ idxOut ] );
		VectorNormalize( tess.binormals[ idxOut ] );

		tess.texCoords[ idxOut ][ 0 ] = model->texcoords[ 2 * idxIn + 0 ];
		tess.texCoords[ idxOut ][ 1 ] = model->texcoords[ 2 * idxIn + 1 ];
	}

	tess.numIndexes  += 3 * surf->num_triangles;
	tess.numVertexes += surf->num_vertexes;
}

/*
===========================================================================

NULL MODEL

===========================================================================
*/

/*
===================
Tess_SurfaceAxis

Draws x/y/z lines from the origin for orientation debugging
===================
*/
static void Tess_SurfaceAxis( void )
{
	//int             k;
	//vec4_t          verts[3];
	//vec3_t          forward, right, up;

	GLimp_LogComment( "--- Tess_SurfaceAxis ---\n" );

#if 0
	Tess_CheckOverflow( 9, 9 );

	MatrixToVectorsFRU( backEnd.orientation.transformMatrix, forward, right, up );

	VectorClear( verts[ 0 ] );
	VectorScale( forward, 1, verts[ 1 ] );
	VectorScale( up, 0.2, verts[ 2 ] );

	for ( k = 0; k < 3; k++ )
	{
		verts[ k ][ 3 ] = 1;
		Vector4Copy( verts[ k ], tess.xyz[ tess.numVertexes ] );
		Vector4Copy( colorRed, tess.colors[ tess.numVertexes ] );
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;
	}

	VectorScale( right, 1, verts[ 1 ] );
	VectorScale( up, 0.2, verts[ 2 ] );

	for ( k = 0; k < 3; k++ )
	{
		verts[ k ][ 3 ] = 1;
		Vector4Copy( verts[ k ], tess.xyz[ tess.numVertexes ] );
		Vector4Copy( colorGreen, tess.colors[ tess.numVertexes ] );
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;
	}

	VectorScale( up, 1, verts[ 1 ] );
	VectorScale( forward, 0.2, verts[ 2 ] );

	for ( k = 0; k < 3; k++ )
	{
		verts[ k ][ 3 ] = 1;
		Vector4Copy( verts[ k ], tess.xyz[ tess.numVertexes ] );
		Vector4Copy( colorBlue, tess.colors[ tess.numVertexes ] );
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
		tess.numVertexes++;
	}

#endif

	/*
	   GL_BindProgram(0);
	   GL_SelectTexture(0);
	   GL_Bind(tr.whiteImage);

	   glLineWidth(3);
	   glBegin(GL_LINES);
	   glColor3f(1, 0, 0);
	   glVertex3f(0, 0, 0);
	   glVertex3f(16, 0, 0);
	   glColor3f(0, 1, 0);
	   glVertex3f(0, 0, 0);
	   glVertex3f(0, 16, 0);
	   glColor3f(0, 0, 1);
	   glVertex3f(0, 0, 0);
	   glVertex3f(0, 0, 16);
	   glEnd();
	   glLineWidth(1);
	 */
}

//===========================================================================

/*
====================
Tess_SurfaceEntity

Entities that have a single procedurally generated surface
====================
*/
static void Tess_SurfaceEntity( surfaceType_t *surfType )
{
	GLimp_LogComment( "--- Tess_SurfaceEntity ---\n" );

	switch ( backEnd.currentEntity->e.reType )
	{
		case RT_SPRITE:
			Tess_SurfaceSprite();
			break;

		case RT_BEAM:
			Tess_SurfaceBeam();
			break;

		default:
			Tess_SurfaceAxis();
			break;
	}
}

static void Tess_SurfaceBad( surfaceType_t *surfType )
{
	GLimp_LogComment( "--- Tess_SurfaceBad ---\n" );

	ri.Printf( PRINT_ALL, "Bad surface tesselated.\n" );
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


	Tess_SurfaceVBO( srf->vbo, srf->ibo, srf->numVerts, srf->numIndexes, srf->firstIndex );
}

/*
==============
Tess_SurfaceVBOMDVMesh
==============
*/
void Tess_SurfaceVBOMDVMesh( srfVBOMDVMesh_t *surface )
{
//	int             i;
//	mdvModel_t     *mdvModel;
//	mdvSurface_t   *mdvSurface;
//	matrix_t        m, m2; //, m3
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

//	mdvModel = surface->mdvModel;
//	mdvSurface = surface->mdvSurface;

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

	//glState.vertexAttribPointersSet = 0;
	//GL_VertexAttribPointers(ATTR_BITS | ATTR_POSITION2 | ATTR_TANGENT2 | ATTR_BINORMAL2 | ATTR_NORMAL2);

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

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )

	if ( backEnd.currentEntity->e.skeleton.type == SK_ABSOLUTE )
	{
		tess.vboVertexSkinning = qtrue;
		tess.numBones = srf->numBoneRemap;

		for ( i = 0; i < srf->numBoneRemap; i++ )
		{
			refBone_t *bone = &backEnd.currentEntity->e.skeleton.bones[ srf->boneRemapInverse[ i ] ];

			TransInitRotationQuat( model->bones[ srf->boneRemapInverse[ i ] ].rotation, &tess.bones[ i ] );
			TransAddTranslation( model->bones[ srf->boneRemapInverse[ i ] ].origin, &tess.bones[ i ] );
			TransInverse( &tess.bones[ i ], &tess.bones[ i ] );
			TransCombine( &tess.bones[ i ], &bone->t, &tess.bones[ i ] );
			TransAddScale( backEnd.currentEntity->e.skeleton.scale, &tess.bones[ i ] );
		}
	}
	else
#endif
	{
		tess.vboVertexSkinning = qfalse;
	}

	//GL_VertexAttribPointers(ATTR_BITS | ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS);

	Tess_End();
}

static void Tess_SurfaceSkip( void *surf )
{
}

// *INDENT-OFF*
void ( *rb_surfaceTable[ SF_NUM_SURFACE_TYPES ] )( void * ) =
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
