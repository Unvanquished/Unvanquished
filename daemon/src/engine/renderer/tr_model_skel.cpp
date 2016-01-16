/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2010 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_models.c -- model loading and caching
#include "tr_local.h"

bool AddTriangleToVBOTriangleList( growList_t *vboTriangles, skelTriangle_t *tri, int *numBoneReferences, int boneReferences[ MAX_BONES ] )
{
	md5Vertex_t *v;
	int         boneIndex;
	int         numNewReferences;
	int         newReferences[ MAX_WEIGHTS * 3 ]; // a single triangle can have up to 12 new bone references !
	bool    hasWeights;

	hasWeights = false;

	numNewReferences = 0;
	Com_Memset( newReferences, -1, sizeof( newReferences ) );

	for (unsigned i = 0; i < 3; i++ )
	{
		v = tri->vertexes[ i ];

		// can the bones be referenced?
		for (unsigned j = 0; j < MAX_WEIGHTS; j++ )
		{
			if ( j < v->numWeights )
			{
				boneIndex = v->boneIndexes[ j ];
				hasWeights = true;

				// is the bone already referenced?
				if ( !boneReferences[ boneIndex ] )
				{
					// the bone isn't yet and we have to test if we can give the mesh this bone at all
					if ( ( *numBoneReferences + numNewReferences ) >= glConfig2.maxVertexSkinningBones )
					{
						return false;
					}
					else
					{
                        unsigned k;
						for ( k = 0; k < ( MAX_WEIGHTS * 3 ); k++ )
						{
							if ( newReferences[ k ] == boneIndex )
							{
								break;
							}
						}

						if ( k == ( MAX_WEIGHTS * 3 ) )
						{
							newReferences[ numNewReferences ] = boneIndex;
							numNewReferences++;
						}
					}
				}
			}
		}
	}

	// reference them!
	for (int j = 0; j < numNewReferences; j++ )
	{
		boneIndex = newReferences[ j ];

		boneReferences[ boneIndex ]++;

		*numBoneReferences = *numBoneReferences + 1;
	}

	if ( hasWeights )
	{
		Com_AddToGrowList( vboTriangles, tri );
		return true;
	}

	return false;
}

void AddSurfaceToVBOSurfacesList( growList_t *vboSurfaces, growList_t *vboTriangles, md5Model_t *md5, md5Surface_t *surf, int skinIndex, int boneReferences[ MAX_BONES ] )
{
	int             j;

	int             vertexesNum;
	vboData_t       data;

	int             indexesNum;
	glIndex_t       *indexes;

	skelTriangle_t  *tri;

	srfVBOMD5Mesh_t *vboSurf;

	vertexesNum = surf->numVerts;
	indexesNum = vboTriangles->currentElements * 3;

	// create surface
	vboSurf = (srfVBOMD5Mesh_t*) ri.Hunk_Alloc( sizeof( *vboSurf ), ha_pref::h_low );
	Com_AddToGrowList( vboSurfaces, vboSurf );

	vboSurf->surfaceType = surfaceType_t::SF_VBO_MD5MESH;
	vboSurf->md5Model = md5;
	vboSurf->shader = R_GetShaderByHandle( surf->shaderIndex );
	vboSurf->skinIndex = skinIndex;
	vboSurf->numIndexes = indexesNum;
	vboSurf->numVerts = vertexesNum;

	memset( &data, 0, sizeof( data ) );

	data.xyz = ( vec3_t * ) ri.Hunk_AllocateTempMemory( sizeof( *data.xyz ) * vertexesNum );
	data.qtangent = ( i16vec4_t * ) ri.Hunk_AllocateTempMemory( sizeof( i16vec4_t ) * vertexesNum );
	data.boneIndexes = ( int (*)[ 4 ] ) ri.Hunk_AllocateTempMemory( sizeof( *data.boneIndexes ) * vertexesNum );
	data.boneWeights = ( vec4_t * ) ri.Hunk_AllocateTempMemory( sizeof( *data.boneWeights ) * vertexesNum );
	data.st = ( i16vec2_t * ) ri.Hunk_AllocateTempMemory( sizeof( i16vec2_t ) * vertexesNum );
	data.noLightCoords = true;
	data.numVerts = vertexesNum;

	indexes = ( glIndex_t * ) ri.Hunk_AllocateTempMemory( indexesNum * sizeof( glIndex_t ) );

	vboSurf->numBoneRemap = 0;
	Com_Memset( vboSurf->boneRemap, 0, sizeof( vboSurf->boneRemap ) );
	Com_Memset( vboSurf->boneRemapInverse, 0, sizeof( vboSurf->boneRemapInverse ) );

	for ( j = 0; j < MAX_BONES; j++ )
	{
		if ( boneReferences[ j ] > 0 )
		{
			vboSurf->boneRemap[ j ] = vboSurf->numBoneRemap;
			vboSurf->boneRemapInverse[ vboSurf->numBoneRemap ] = j;

			vboSurf->numBoneRemap++;
		}
	}

	for ( j = 0; j < vboTriangles->currentElements; j++ )
	{
		tri = ( skelTriangle_t * ) Com_GrowListElement( vboTriangles, j );

		for (unsigned k = 0; k < 3; k++ )
		{
			indexes[ j * 3 + k ] = tri->indexes[ k ];
		}
	}

	for ( j = 0; j < vertexesNum; j++ )
	{
		VectorCopy( surf->verts[ j ].position, data.xyz[ j ] );
		R_TBNtoQtangents( surf->verts[ j ].tangent, surf->verts[ j ].binormal,
				  surf->verts[ j ].normal, data.qtangent[ j ] );
		
		data.st[ j ][ 0 ] = floatToHalf( surf->verts[ j ].texCoords[ 0 ] );
		data.st[ j ][ 1 ] = floatToHalf( surf->verts[ j ].texCoords[ 1 ] );

		for (unsigned k = 0; k < MAX_WEIGHTS; k++ )
		{
			if ( k < surf->verts[ j ].numWeights )
			{
				data.boneIndexes[ j ][ k ] = vboSurf->boneRemap[ surf->verts[ j ].boneIndexes[ k ] ];
				data.boneWeights[ j ][ k ] = surf->verts[ j ].boneWeights[ k ];
			}
			else
			{
				data.boneWeights[ j ][ k ] = 0;
				data.boneIndexes[ j ][ k ] = 0;
			}
		}
	}

	vboSurf->vbo = R_CreateStaticVBO( va( "staticMD5Mesh_VBO %i", vboSurfaces->currentElements ), data, vboLayout_t::VBO_LAYOUT_SKELETAL );

	vboSurf->ibo = R_CreateStaticIBO( va( "staticMD5Mesh_IBO %i", vboSurfaces->currentElements ), indexes, indexesNum );

	ri.Hunk_FreeTempMemory( indexes );
	ri.Hunk_FreeTempMemory( data.st );
	ri.Hunk_FreeTempMemory( data.boneWeights );
	ri.Hunk_FreeTempMemory( data.boneIndexes );
	ri.Hunk_FreeTempMemory( data.qtangent );
	ri.Hunk_FreeTempMemory( data.xyz );
}
