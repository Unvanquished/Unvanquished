/*
===========================================================================
Copyright (C) 2009-2010 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_models_psk.c -- Unreal Engine 3 .psk model loading and caching

#include "tr_local.h"
#include "tr_model_skel.h"

static void GetChunkHeader( memStream_t *s, axChunkHeader_t *chunkHeader )
{
	int i;

	for ( i = 0; i < 20; i++ )
	{
		chunkHeader->ident[ i ] = MemStreamGetC( s );
	}

	chunkHeader->flags = MemStreamGetLong( s );
	chunkHeader->dataSize = MemStreamGetLong( s );
	chunkHeader->numData = MemStreamGetLong( s );
}

static void GetBone( memStream_t *s, axBone_t *bone )
{
	int i;

	for ( i = 0; i < 4; i++ )
	{
		bone->quat[ i ] = MemStreamGetFloat( s );
	}

	for ( i = 0; i < 3; i++ )
	{
		bone->position[ i ] = MemStreamGetFloat( s );
	}

	bone->length = MemStreamGetFloat( s );

	bone->xSize = MemStreamGetFloat( s );
	bone->ySize = MemStreamGetFloat( s );
	bone->zSize = MemStreamGetFloat( s );
}

static int CompareTrianglesByMaterialIndex( const void *a, const void *b )
{
	axTriangle_t *t1, *t2;

	t1 = ( axTriangle_t * ) a;
	t2 = ( axTriangle_t * ) b;

	if ( t1->materialIndex < t2->materialIndex )
	{
		return -1;
	}

	if ( t1->materialIndex > t2->materialIndex )
	{
		return 1;
	}

	return 0;
}

/*
=================
R_LoadPSK
=================
*/
qboolean R_LoadPSK( model_t *mod, void *buffer, int bufferSize, const char *modName )
{
	int               i, j, k;
	memStream_t       *stream = NULL;

	axChunkHeader_t   chunkHeader;

	int               numPoints;
	axPoint_t         *point;
	axPoint_t         *points = NULL;

	int               numVertexes;
	axVertex_t        *vertex;
	axVertex_t        *vertexes = NULL;

	int               numTriangles;
	axTriangle_t      *triangle;
	axTriangle_t      *triangles = NULL;

	int               numMaterials;
	axMaterial_t      *material;
	axMaterial_t      *materials = NULL;

	int               numReferenceBones;
	axReferenceBone_t *refBone;
	axReferenceBone_t *refBones = NULL;

	int               numWeights;
	axBoneWeight_t    *axWeight;
	axBoneWeight_t    *axWeights = NULL;

	md5Model_t        *md5;
	md5Bone_t         *md5Bone;

	vec3_t            boneOrigin;
	quat_t            boneQuat;
	//matrix_t        boneMat;

	int               materialIndex, oldMaterialIndex;

	int               numRemaining;

	growList_t        sortedTriangles;
	growList_t        vboVertexes;
	growList_t        vboTriangles;
	growList_t        vboSurfaces;

	int               numBoneReferences;
	int               boneReferences[ MAX_BONES ];

	matrix_t          unrealToQuake;

#define DeallocAll() Com_Dealloc( materials ); \
	Com_Dealloc( points ); \
	Com_Dealloc( vertexes ); \
	Com_Dealloc( triangles ); \
	Com_Dealloc( refBones ); \
	Com_Dealloc( axWeights ); \
	FreeMemStream( stream );

	MatrixFromAngles( unrealToQuake, 0, 90, 0 );

	stream = AllocMemStream( (byte*) buffer, bufferSize );
	GetChunkHeader( stream, &chunkHeader );

	// check indent again
	if ( Q_strnicmp( chunkHeader.ident, "ACTRHEAD", 8 ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has wrong chunk indent ('%s' should be '%s')\n", modName, chunkHeader.ident, "ACTRHEAD" );
		DeallocAll();
		return qfalse;
	}

	mod->type = MOD_MD5;
	mod->dataSize += sizeof( md5Model_t );
	md5 = mod->md5 = (md5Model_t*) ri.Hunk_Alloc( sizeof( md5Model_t ), h_low );

	// read points
	GetChunkHeader( stream, &chunkHeader );

	if ( Q_strnicmp( chunkHeader.ident, "PNTS0000", 8 ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has wrong chunk indent ('%s' should be '%s')\n", modName, chunkHeader.ident, "PNTS0000" );
		DeallocAll();
		return qfalse;
	}

	if ( chunkHeader.dataSize != sizeof( axPoint_t ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has wrong chunk dataSize ('%i' should be '%i')\n", modName, chunkHeader.dataSize, ( int ) sizeof( axPoint_t ) );
		DeallocAll();
		return qfalse;
	}

	numPoints = chunkHeader.numData;
	points = (axPoint_t*) Com_Allocate( numPoints * sizeof( axPoint_t ) );

	for ( i = 0, point = points; i < numPoints; i++, point++ )
	{
		point->point[ 0 ] = MemStreamGetFloat( stream );
		point->point[ 1 ] = MemStreamGetFloat( stream );
		point->point[ 2 ] = MemStreamGetFloat( stream );
	}

	// read vertices
	GetChunkHeader( stream, &chunkHeader );

	if ( Q_strnicmp( chunkHeader.ident, "VTXW0000", 8 ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has wrong chunk indent ('%s' should be '%s')\n", modName, chunkHeader.ident, "VTXW0000" );
		DeallocAll();
		return qfalse;
	}

	if ( chunkHeader.dataSize != sizeof( axVertex_t ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has wrong chunk dataSize ('%i' should be '%i')\n", modName, chunkHeader.dataSize, ( int ) sizeof( axVertex_t ) );
		DeallocAll();
		return qfalse;
	}

	numVertexes = chunkHeader.numData;
	vertexes = (axVertex_t*) Com_Allocate( numVertexes * sizeof( axVertex_t ) );

	for ( i = 0, vertex = vertexes; i < numVertexes; i++, vertex++ )
	{
		vertex->pointIndex = MemStreamGetShort( stream );

		if ( vertex->pointIndex < 0 || vertex->pointIndex >= numPoints )
		{
			ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has vertex with point index out of range (%i while max %i)\n", modName, vertex->pointIndex, numPoints );
			DeallocAll();
			return qfalse;
		}

		vertex->unknownA = MemStreamGetShort( stream );
		vertex->st[ 0 ] = MemStreamGetFloat( stream );
		vertex->st[ 1 ] = MemStreamGetFloat( stream );
		vertex->materialIndex = MemStreamGetC( stream );
		vertex->reserved = MemStreamGetC( stream );
		vertex->unknownB = MemStreamGetShort( stream );
	}

	// read triangles
	GetChunkHeader( stream, &chunkHeader );

	if ( Q_strnicmp( chunkHeader.ident, "FACE0000", 8 ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has wrong chunk indent ('%s' should be '%s')\n", modName, chunkHeader.ident, "FACE0000" );
		DeallocAll();
		return qfalse;
	}

	if ( chunkHeader.dataSize != sizeof( axTriangle_t ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has wrong chunk dataSize ('%i' should be '%i')\n", modName, chunkHeader.dataSize, ( int ) sizeof( axTriangle_t ) );
		DeallocAll();
		return qfalse;
	}

	numTriangles = chunkHeader.numData;
	triangles = (axTriangle_t*) Com_Allocate( numTriangles * sizeof( axTriangle_t ) );

	for ( i = 0, triangle = triangles; i < numTriangles; i++, triangle++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			triangle->indexes[ j ] = MemStreamGetShort( stream );

			if ( triangle->indexes[ j ] >= numVertexes )
			{
				ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has triangle with vertex index out of range (%i while max %i)\n", modName, triangle->indexes[ j ], numVertexes );
				DeallocAll();
				return qfalse;
			}
		}

		triangle->materialIndex = MemStreamGetC( stream );
		triangle->materialIndex2 = MemStreamGetC( stream );
		triangle->smoothingGroups = MemStreamGetLong( stream );
	}

	// read materials
	GetChunkHeader( stream, &chunkHeader );

	if ( Q_strnicmp( chunkHeader.ident, "MATT0000", 8 ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has wrong chunk indent ('%s' should be '%s')\n", modName, chunkHeader.ident, "MATT0000" );
		DeallocAll();
		return qfalse;
	}

	if ( chunkHeader.dataSize != sizeof( axMaterial_t ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has wrong chunk dataSize ('%i' should be '%i')\n", modName, chunkHeader.dataSize, ( int ) sizeof( axMaterial_t ) );
		DeallocAll();
		return qfalse;
	}

	numMaterials = chunkHeader.numData;
	materials = (axMaterial_t*) Com_Allocate( numMaterials * sizeof( axMaterial_t ) );

	for ( i = 0, material = materials; i < numMaterials; i++, material++ )
	{
		MemStreamRead( stream, material->name, sizeof( material->name ) );

		ri.Printf( PRINT_ALL, "R_LoadPSK: material name: '%s'\n", material->name );

		material->shaderIndex = MemStreamGetLong( stream );
		material->polyFlags = MemStreamGetLong( stream );
		material->auxMaterial = MemStreamGetLong( stream );
		material->auxFlags = MemStreamGetLong( stream );
		material->lodBias = MemStreamGetLong( stream );
		material->lodStyle = MemStreamGetLong( stream );
	}

	for ( i = 0, vertex = vertexes; i < numVertexes; i++, vertex++ )
	{
		if ( vertex->materialIndex < 0 || vertex->materialIndex >= numMaterials )
		{
			ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has vertex with material index out of range (%i while max %i)\n", modName, vertex->materialIndex, numMaterials );
			DeallocAll();
			return qfalse;
		}
	}

	for ( i = 0, triangle = triangles; i < numTriangles; i++, triangle++ )
	{
		if ( triangle->materialIndex < 0 || triangle->materialIndex >= numMaterials )
		{
			ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has triangle with material index out of range (%i while max %i)\n", modName, triangle->materialIndex, numMaterials );
			DeallocAll();
			return qfalse;
		}
	}

	// read reference bones
	GetChunkHeader( stream, &chunkHeader );

	if ( Q_strnicmp( chunkHeader.ident, "REFSKELT", 8 ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has wrong chunk indent ('%s' should be '%s')\n", modName, chunkHeader.ident, "REFSKELT" );
		DeallocAll();
		return qfalse;
	}

	if ( chunkHeader.dataSize != sizeof( axReferenceBone_t ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has wrong chunk dataSize ('%i' should be '%i')\n", modName, chunkHeader.dataSize, ( int ) sizeof( axReferenceBone_t ) );
		DeallocAll();
		return qfalse;
	}

	numReferenceBones = chunkHeader.numData;
	refBones = (axReferenceBone_t*) Com_Allocate( numReferenceBones * sizeof( axReferenceBone_t ) );

	for ( i = 0, refBone = refBones; i < numReferenceBones; i++, refBone++ )
	{
		MemStreamRead( stream, refBone->name, sizeof( refBone->name ) );

		refBone->flags = MemStreamGetLong( stream );
		refBone->numChildren = MemStreamGetLong( stream );
		refBone->parentIndex = MemStreamGetLong( stream );

		GetBone( stream, &refBone->bone );
	}

	// read  bone weights
	GetChunkHeader( stream, &chunkHeader );

	if ( Q_strnicmp( chunkHeader.ident, "RAWWEIGHTS", 10 ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has wrong chunk indent ('%s' should be '%s')\n", modName, chunkHeader.ident, "RAWWEIGHTS" );
		DeallocAll();
		return qfalse;
	}

	if ( chunkHeader.dataSize != sizeof( axBoneWeight_t ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has wrong chunk dataSize ('%i' should be '%i')\n", modName, chunkHeader.dataSize, ( int ) sizeof( axBoneWeight_t ) );
		DeallocAll();
		return qfalse;
	}

	numWeights = chunkHeader.numData;
	axWeights = (axBoneWeight_t*) Com_Allocate( numWeights * sizeof( axBoneWeight_t ) );

	for ( i = 0, axWeight = axWeights; i < numWeights; i++, axWeight++ )
	{
		axWeight->weight = MemStreamGetFloat( stream );
		axWeight->pointIndex = MemStreamGetLong( stream );
		axWeight->boneIndex = MemStreamGetLong( stream );
	}

	//
	// convert the model to an internal MD5 representation
	//
	md5->numBones = numReferenceBones;

	// calc numMeshes <number>

	if ( md5->numBones < 1 )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has no bones\n", modName );
		DeallocAll();
		return qfalse;
	}

	if ( md5->numBones > MAX_BONES )
	{
		ri.Printf( PRINT_WARNING, "R_LoadPSK: '%s' has more than %i bones (%i)\n", modName, MAX_BONES, md5->numBones );
		DeallocAll();
		return qfalse;
	}

	// copy all reference bones
	md5->bones = (md5Bone_t*) ri.Hunk_Alloc( sizeof( *md5Bone ) * md5->numBones, h_low );

	for ( i = 0, md5Bone = md5->bones, refBone = refBones; i < md5->numBones; i++, md5Bone++, refBone++ )
	{
		Q_strncpyz( md5Bone->name, refBone->name, sizeof( md5Bone->name ) );

		if ( i == 0 )
		{
			md5Bone->parentIndex = refBone->parentIndex - 1;
		}
		else
		{
			md5Bone->parentIndex = refBone->parentIndex;
		}

		if ( md5Bone->parentIndex >= md5->numBones )
		{
			DeallocAll();
			ri.Error( ERR_DROP, "R_LoadPSK: '%s' has bone '%s' with bad parent index %i while numBones is %i", modName,
			          md5Bone->name, md5Bone->parentIndex, md5->numBones );
		}

		for ( j = 0; j < 3; j++ )
		{
			boneOrigin[ j ] = refBone->bone.position[ j ];
		}

		// Tr3B: I have really no idea why the .psk format stores the first quaternion with inverted quats.
		// Furthermore only the X and Z components of the first quat are inverted ?!?!
		if ( i == 0 )
		{
			boneQuat[ 0 ] = refBone->bone.quat[ 0 ];
			boneQuat[ 1 ] = -refBone->bone.quat[ 1 ];
			boneQuat[ 2 ] = refBone->bone.quat[ 2 ];
			boneQuat[ 3 ] = refBone->bone.quat[ 3 ];
		}
		else
		{
			boneQuat[ 0 ] = -refBone->bone.quat[ 0 ];
			boneQuat[ 1 ] = -refBone->bone.quat[ 1 ];
			boneQuat[ 2 ] = -refBone->bone.quat[ 2 ];
			boneQuat[ 3 ] = refBone->bone.quat[ 3 ];
		}

		VectorCopy( boneOrigin, md5Bone->origin );

		QuatCopy( boneQuat, md5Bone->rotation );

		if ( md5Bone->parentIndex >= 0 )
		{
			vec3_t    rotated;
			quat_t    quat;

			md5Bone_t *parent;

			parent = &md5->bones[ md5Bone->parentIndex ];

			QuatTransformVector( parent->rotation, md5Bone->origin, rotated );

			VectorAdd( parent->origin, rotated, md5Bone->origin );

			QuatMultiply1( parent->rotation, md5Bone->rotation, quat );
			QuatCopy( quat, md5Bone->rotation );
		}
	}

	Com_InitGrowList( &vboVertexes, 10000 );

	for ( i = 0, vertex = vertexes; i < numVertexes; i++, vertex++ )
	{
		md5Vertex_t *vboVert = (md5Vertex_t*) Com_Allocate( sizeof( *vboVert ) );

		for ( j = 0; j < 3; j++ )
		{
			vboVert->position[ j ] = points[ vertex->pointIndex ].point[ j ];
		}

		vboVert->texCoords[ 0 ] = vertex->st[ 0 ];
		vboVert->texCoords[ 1 ] = vertex->st[ 1 ];

		// find number of associated weights
		vboVert->numWeights = 0;

		for ( j = 0, axWeight = axWeights; j < numWeights; j++, axWeight++ )
		{
			if ( axWeight->pointIndex == vertex->pointIndex && axWeight->weight > 0.0f )
			{
				vboVert->numWeights++;
			}
		}

		if ( vboVert->numWeights > MAX_WEIGHTS )
		{
			DeallocAll();
			ri.Error( ERR_DROP, "R_LoadPSK: vertex %i requires more weights %i than the maximum of %i in model '%s'", i, vboVert->numWeights, MAX_WEIGHTS, modName );
		}

		for ( j = 0, axWeight = axWeights, k = 0; j < numWeights; j++, axWeight++ )
		{
			if ( axWeight->pointIndex == vertex->pointIndex && axWeight->weight > 0.0f )
			{
				vboVert->boneIndexes[ k ] = axWeight->boneIndex;
				vboVert->boneWeights[ k ] = axWeight->weight;
				k++;
			}
		}

		Com_AddToGrowList( &vboVertexes, vboVert );
	}

	ClearBounds( md5->bounds[ 0 ], md5->bounds[ 1 ] );

	for ( i = 0, vertex = vertexes; i < numVertexes; i++, vertex++ )
	{
		AddPointToBounds( points[ vertex->pointIndex ].point, md5->bounds[ 0 ], md5->bounds[ 1 ] );
	}

	// sort triangles
	qsort( triangles, numTriangles, sizeof( axTriangle_t ), CompareTrianglesByMaterialIndex );

	Com_InitGrowList( &sortedTriangles, 1000 );

	for ( i = 0, triangle = triangles; i < numTriangles; i++, triangle++ )
	{
		skelTriangle_t *sortTri = (skelTriangle_t*) Com_Allocate( sizeof( *sortTri ) );

		for ( j = 0; j < 3; j++ )
		{
			sortTri->indexes[ j ] = triangle->indexes[ j ];
			sortTri->vertexes[ j ] = (md5Vertex_t*) Com_GrowListElement( &vboVertexes, triangle->indexes[ j ] );
		}

		sortTri->referenced = qfalse;

		Com_AddToGrowList( &sortedTriangles, sortTri );
	}

	// calc tangent spaces
	{
		md5Vertex_t *v0, *v1, *v2;
		const float *p0, *p1, *p2;
		const float *t0, *t1, *t2;
		vec3_t      tangent;
		vec3_t      binormal;
		vec3_t      normal;

		for ( j = 0; j < vboVertexes.currentElements; j++ )
		{
			v0 = (md5Vertex_t*) Com_GrowListElement( &vboVertexes, j );

			VectorClear( v0->tangent );
			VectorClear( v0->binormal );
			VectorClear( v0->normal );
		}

		for ( j = 0; j < sortedTriangles.currentElements; j++ )
		{
			skelTriangle_t *tri = (skelTriangle_t*) Com_GrowListElement( &sortedTriangles, j );

			v0 = (md5Vertex_t*) Com_GrowListElement( &vboVertexes, tri->indexes[ 0 ] );
			v1 = (md5Vertex_t*) Com_GrowListElement( &vboVertexes, tri->indexes[ 1 ] );
			v2 = (md5Vertex_t*) Com_GrowListElement( &vboVertexes, tri->indexes[ 2 ] );

			p0 = v0->position;
			p1 = v1->position;
			p2 = v2->position;

			t0 = v0->texCoords;
			t1 = v1->texCoords;
			t2 = v2->texCoords;

			R_CalcTangentSpace( tangent, binormal, normal, p0, p1, p2, t0, t1, t2 );

			for ( k = 0; k < 3; k++ )
			{
				float *v;

				v0 = (md5Vertex_t*) Com_GrowListElement( &vboVertexes, tri->indexes[ k ] );

				v = v0->tangent;
				VectorAdd( v, tangent, v );

				v = v0->binormal;
				VectorAdd( v, binormal, v );

				v = v0->normal;
				VectorAdd( v, normal, v );
			}
		}

		for ( j = 0; j < vboVertexes.currentElements; j++ )
		{
			v0 = (md5Vertex_t*) Com_GrowListElement( &vboVertexes, j );

			VectorNormalize( v0->tangent );
			VectorNormalize( v0->binormal );
			VectorNormalize( v0->normal );
		}
	}

	// split the surfaces into VBO surfaces by the maximum number of GPU vertex skinning bones
	Com_InitGrowList( &vboSurfaces, 10 );

	materialIndex = oldMaterialIndex = -1;

	for ( i = 0; i < numTriangles; i++ )
	{
		triangle = &triangles[ i ];
		materialIndex = triangle->materialIndex;

		if ( materialIndex != oldMaterialIndex )
		{
			oldMaterialIndex = materialIndex;

			numRemaining = sortedTriangles.currentElements - i;

			while ( numRemaining )
			{
				numBoneReferences = 0;
				Com_Memset( boneReferences, 0, sizeof( boneReferences ) );

				Com_InitGrowList( &vboTriangles, 1000 );

				for ( j = i; j < sortedTriangles.currentElements; j++ )
				{
					skelTriangle_t *sortTri;

					triangle = &triangles[ j ];
					materialIndex = triangle->materialIndex;

					if ( materialIndex != oldMaterialIndex )
					{
						continue;
					}

					sortTri = (skelTriangle_t*) Com_GrowListElement( &sortedTriangles, j );

					if ( sortTri->referenced )
					{
						continue;
					}

					if ( AddTriangleToVBOTriangleList( &vboTriangles, sortTri, &numBoneReferences, boneReferences ) )
					{
						sortTri->referenced = qtrue;
					}
				}

				for ( j = 0; j < MAX_BONES; j++ )
				{
					if ( boneReferences[ j ] > 0 )
					{
						ri.Printf( PRINT_ALL, "R_LoadPSK: referenced bone: '%s'\n", ( j < numReferenceBones ) ? refBones[ j ].name : NULL );
					}
				}

				if ( !vboTriangles.currentElements )
				{
					ri.Printf( PRINT_WARNING, "R_LoadPSK: could not add triangles to a remaining VBO surface for model '%s'\n", modName );
					break;
				}

				// FIXME skinIndex
				AddSurfaceToVBOSurfacesList2( &vboSurfaces, &vboTriangles, &vboVertexes, md5, vboSurfaces.currentElements, materials[ oldMaterialIndex ].name, numBoneReferences, boneReferences );
				numRemaining -= vboTriangles.currentElements;

				Com_DestroyGrowList( &vboTriangles );
			}
		}
	}

	for ( j = 0; j < sortedTriangles.currentElements; j++ )
	{
		skelTriangle_t *sortTri = (skelTriangle_t*) Com_GrowListElement( &sortedTriangles, j );
		Com_Dealloc( sortTri );
	}

	Com_DestroyGrowList( &sortedTriangles );

	for ( j = 0; j < vboVertexes.currentElements; j++ )
	{
		md5Vertex_t *v = (md5Vertex_t*) Com_GrowListElement( &vboVertexes, j );
		Com_Dealloc( v );
	}

	Com_DestroyGrowList( &vboVertexes );

	// move VBO surfaces list to hunk
	md5->numVBOSurfaces = vboSurfaces.currentElements;
	md5->vboSurfaces = (srfVBOMD5Mesh_t**) ri.Hunk_Alloc( md5->numVBOSurfaces * sizeof( *md5->vboSurfaces ), h_low );

	for ( i = 0; i < md5->numVBOSurfaces; i++ )
	{
		md5->vboSurfaces[ i ] = ( srfVBOMD5Mesh_t * ) Com_GrowListElement( &vboSurfaces, i );
	}

	Com_DestroyGrowList( &vboSurfaces );

	FreeMemStream( stream );
	Com_Dealloc( points );
	Com_Dealloc( vertexes );
	Com_Dealloc( triangles );
	Com_Dealloc( materials );

	ri.Printf( PRINT_DEVELOPER, "%i VBO surfaces created for PSK model '%s'\n", md5->numVBOSurfaces, modName );

	return qtrue;
}
