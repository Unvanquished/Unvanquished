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
// tr_model_md5.c -- Doom 3 .md5mesh model loading and caching

#include "tr_local.h"
#include "tr_model_skel.h"

/*
=================
R_LoadMD5
=================
*/
bool R_LoadMD5( model_t *mod, void *buffer, const char *modName )
{
	md5Model_t    *md5;
	md5Bone_t     *bone;
	md5Surface_t  *surf;
	srfTriangle_t *tri;
	md5Vertex_t   *v;
	md5Weight_t   *weight;
	int           version;
	shader_t      *sh;
	const char    *buf_p;
	char          *token;
	vec3_t        boneOrigin;
	quat_t        boneQuat;
	matrix_t      boneMat;

	int           numRemaining;
	growList_t    sortedTriangles;
	growList_t    vboTriangles;
	growList_t    vboSurfaces;

	int           numBoneReferences;
	int           boneReferences[ MAX_BONES ];

	buf_p = ( char * ) buffer;

	// skip MD5Version indent string
	COM_ParseExt2( &buf_p, false );

	// check version
	token = COM_ParseExt2( &buf_p, false );
	version = atoi( token );

	if ( version != MD5_VERSION )
	{
		ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: %s has wrong version (%i should be %i)\n", modName, version, MD5_VERSION );
		return false;
	}

	mod->type = modtype_t::MOD_MD5;
	mod->dataSize += sizeof( md5Model_t );
	md5 = mod->md5 = (md5Model_t*) ri.Hunk_Alloc( sizeof( md5Model_t ), ha_pref::h_low );

	// skip commandline <arguments string>
	token = COM_ParseExt2( &buf_p, true );
	token = COM_ParseExt2( &buf_p, true );

	// parse numJoints <number>
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "numJoints" ) )
	{
		ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected 'numJoints' found '%s' in model '%s'\n", token, modName );
		return false;
	}

	token = COM_ParseExt2( &buf_p, false );
	md5->numBones = atoi( token );

	// parse numMeshes <number>
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "numMeshes" ) )
	{
		ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected 'numMeshes' found '%s' in model '%s'\n", token, modName );
		return false;
	}

	token = COM_ParseExt2( &buf_p, false );
	md5->numSurfaces = atoi( token );

	if ( md5->numBones < 1 )
	{
		ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: '%s' has no bones\n", modName );
		return false;
	}

	if ( md5->numBones > MAX_BONES )
	{
		ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: '%s' has more than %i bones (%i)\n", modName, MAX_BONES, md5->numBones );
		return false;
	}

	// parse all the bones
	md5->bones = (md5Bone_t*) ri.Hunk_Alloc( sizeof( *bone ) * md5->numBones, ha_pref::h_low );

	// parse joints {
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "joints" ) )
	{
		ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected 'joints' found '%s' in model '%s'\n", token, modName );
		return false;
	}

	token = COM_ParseExt2( &buf_p, false );

	if ( Q_stricmp( token, "{" ) )
	{
		ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected '{' found '%s' in model '%s'\n", token, modName );
		return false;
	}

    bone = md5->bones;
	for ( int i = 0; i < md5->numBones; i++, bone++ )
	{
		token = COM_ParseExt2( &buf_p, true );
		Q_strncpyz( bone->name, token, sizeof( bone->name ) );

		token = COM_ParseExt2( &buf_p, false );
		bone->parentIndex = atoi( token );

		if ( bone->parentIndex >= md5->numBones )
		{
			ri.Error(errorParm_t::ERR_DROP, "R_LoadMD5: '%s' has bone '%s' with bad parent index %i while numBones is %i", modName,
			          bone->name, bone->parentIndex, md5->numBones );
		}

		// skip (
		token = COM_ParseExt2( &buf_p, false );

		if ( Q_stricmp( token, "(" ) )
		{
			ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName );
			return false;
		}

		for (unsigned j = 0; j < 3; j++ )
		{
			token = COM_ParseExt2( &buf_p, false );
			boneOrigin[ j ] = atof( token );
		}

		// skip )
		token = COM_ParseExt2( &buf_p, false );

		if ( Q_stricmp( token, ")" ) )
		{
			ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected ')' found '%s' in model '%s'\n", token, modName );
			return false;
		}

		// skip (
		token = COM_ParseExt2( &buf_p, false );

		if ( Q_stricmp( token, "(" ) )
		{
			ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName );
			return false;
		}

		for (unsigned j = 0; j < 3; j++ )
		{
			token = COM_ParseExt2( &buf_p, false );
			boneQuat[ j ] = atof( token );
		}

		QuatCalcW( boneQuat );
		MatrixFromQuat( boneMat, boneQuat );

		VectorCopy( boneOrigin, bone->origin );
		QuatCopy( boneQuat, bone->rotation );

		// skip )
		token = COM_ParseExt2( &buf_p, false );

		if ( Q_stricmp( token, ")" ) )
		{
			ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName );
			return false;
		}
	}

	// parse }
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "}" ) )
	{
		ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected '}' found '%s' in model '%s'\n", token, modName );
		return false;
	}

	// parse all the surfaces
	if ( md5->numSurfaces < 1 )
	{
		ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: '%s' has no surfaces\n", modName );
		return false;
	}

	md5->surfaces = (md5Surface_t*) ri.Hunk_Alloc( sizeof( *surf ) * md5->numSurfaces, ha_pref::h_low );

    surf = md5->surfaces;
	for ( unsigned i = 0; i < md5->numSurfaces; i++, surf++ )
	{
		// parse mesh {
		token = COM_ParseExt2( &buf_p, true );

		if ( Q_stricmp( token, "mesh" ) )
		{
			ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected 'mesh' found '%s' in model '%s'\n", token, modName );
			return false;
		}

		token = COM_ParseExt2( &buf_p, false );

		if ( Q_stricmp( token, "{" ) )
		{
			ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected '{' found '%s' in model '%s'\n", token, modName );
			return false;
		}

		// change to surface identifier
		surf->surfaceType = surfaceType_t::SF_MD5;

		// give pointer to model for Tess_SurfaceMD5
		surf->model = md5;

		// parse shader <name>
		token = COM_ParseExt2( &buf_p, true );

		if ( Q_stricmp( token, "shader" ) )
		{
			ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected 'shader' found '%s' in model '%s'\n", token, modName );
			return false;
		}

		token = COM_ParseExt2( &buf_p, false );
		Q_strncpyz( surf->shader, token, sizeof( surf->shader ) );

		// FIXME .md5mesh meshes don't have surface names
		// lowercase the surface name so skin compares are faster

		// register the shaders
		sh = R_FindShader( surf->shader, shaderType_t::SHADER_3D_DYNAMIC, RSF_DEFAULT );

		if ( sh->defaultShader )
		{
			surf->shaderIndex = 0;
		}
		else
		{
			surf->shaderIndex = sh->index;
		}

		// parse numVerts <number>
		token = COM_ParseExt2( &buf_p, true );

		if ( Q_stricmp( token, "numVerts" ) )
		{
			ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected 'numVerts' found '%s' in model '%s'\n", token, modName );
			return false;
		}

		token = COM_ParseExt2( &buf_p, false );
		surf->numVerts = atoi( token );

		if ( surf->numVerts > SHADER_MAX_VERTEXES )
		{
			ri.Error(errorParm_t::ERR_DROP, "R_LoadMD5: '%s' has more than %i verts on a surface (%i)",
			          modName, SHADER_MAX_VERTEXES, surf->numVerts );
		}

		surf->verts = (md5Vertex_t*) ri.Hunk_Alloc( sizeof( *v ) * surf->numVerts, ha_pref::h_low );
		ASSERT_EQ(((intptr_t) surf->verts & 15), 0);

        v = surf->verts;
		for (unsigned j = 0; j < surf->numVerts; j++, v++ )
		{
			// skip vert <number>
			token = COM_ParseExt2( &buf_p, true );

			if ( Q_stricmp( token, "vert" ) )
			{
				ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected 'vert' found '%s' in model '%s'\n", token, modName );
				return false;
			}

			COM_ParseExt2( &buf_p, false );

			// skip (
			token = COM_ParseExt2( &buf_p, false );

			if ( Q_stricmp( token, "(" ) )
			{
				ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName );
				return false;
			}

			for (unsigned k = 0; k < 2; k++ )
			{
				token = COM_ParseExt2( &buf_p, false );
				v->texCoords[ k ] = atof( token );
			}

			// skip )
			token = COM_ParseExt2( &buf_p, false );

			if ( Q_stricmp( token, ")" ) )
			{
				ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected ')' found '%s' in model '%s'\n", token, modName );
				return false;
			}

			token = COM_ParseExt2( &buf_p, false );
			v->firstWeight = atoi( token );

			token = COM_ParseExt2( &buf_p, false );
			v->numWeights = atoi( token );

			if ( v->numWeights > MAX_WEIGHTS )
			{
				ri.Error(errorParm_t::ERR_DROP, "R_LoadMD5: vertex %i requires more than %i weights on surface (%i) in model '%s'",
				          j, MAX_WEIGHTS, i, modName );
			}
		}

		// parse numTris <number>
		token = COM_ParseExt2( &buf_p, true );

		if ( Q_stricmp( token, "numTris" ) )
		{
			ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected 'numTris' found '%s' in model '%s'\n", token, modName );
			return false;
		}

		token = COM_ParseExt2( &buf_p, false );
		surf->numTriangles = atoi( token );

		if ( surf->numTriangles > SHADER_MAX_TRIANGLES )
		{
			ri.Error(errorParm_t::ERR_DROP, "R_LoadMD5: '%s' has more than %i triangles on a surface (%i)",
			          modName, SHADER_MAX_TRIANGLES, surf->numTriangles );
		}

		surf->triangles = (srfTriangle_t*) ri.Hunk_Alloc( sizeof( *tri ) * surf->numTriangles, ha_pref::h_low );

        tri = surf->triangles;
		for (unsigned j = 0; j < surf->numTriangles; j++, tri++ )
		{
			// skip tri <number>
			token = COM_ParseExt2( &buf_p, true );

			if ( Q_stricmp( token, "tri" ) )
			{
				ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected 'tri' found '%s' in model '%s'\n", token, modName );
				return false;
			}

			COM_ParseExt2( &buf_p, false );

			for (unsigned k = 0; k < 3; k++ )
			{
				token = COM_ParseExt2( &buf_p, false );
				tri->indexes[ k ] = atoi( token );
			}
		}

		// parse numWeights <number>
		token = COM_ParseExt2( &buf_p, true );

		if ( Q_stricmp( token, "numWeights" ) )
		{
			ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected 'numWeights' found '%s' in model '%s'\n", token, modName );
			return false;
		}

		token = COM_ParseExt2( &buf_p, false );
		surf->numWeights = atoi( token );

		surf->weights = (md5Weight_t*) ri.Hunk_Alloc( sizeof( *weight ) * surf->numWeights, ha_pref::h_low );

        weight = surf->weights;
		for (unsigned j = 0; j < surf->numWeights; j++, weight++ )
		{
			// skip weight <number>
			token = COM_ParseExt2( &buf_p, true );

			if ( Q_stricmp( token, "weight" ) )
			{
				ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected 'weight' found '%s' in model '%s'\n", token, modName );
				return false;
			}

			COM_ParseExt2( &buf_p, false );

			token = COM_ParseExt2( &buf_p, false );
			weight->boneIndex = atoi( token );

			token = COM_ParseExt2( &buf_p, false );
			weight->boneWeight = atof( token );

			// skip (
			token = COM_ParseExt2( &buf_p, false );

			if ( Q_stricmp( token, "(" ) )
			{
				ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName );
				return false;
			}

			for (unsigned k = 0; k < 3; k++ )
			{
				token = COM_ParseExt2( &buf_p, false );
				weight->offset[ k ] = atof( token );
			}

			// skip )
			token = COM_ParseExt2( &buf_p, false );

			if ( Q_stricmp( token, ")" ) )
			{
				ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected ')' found '%s' in model '%s'\n", token, modName );
				return false;
			}
		}

		// parse }
		token = COM_ParseExt2( &buf_p, true );

		if ( Q_stricmp( token, "}" ) )
		{
			ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: expected '}' found '%s' in model '%s'\n", token, modName );
			return false;
		}

		// loop through all vertices and set up the vertex weights and base positions
        v = surf->verts;
		for (unsigned j = 0; j < surf->numVerts; j++, v++ )
		{
			md5Weight_t *w = surf->weights + v->firstWeight;
			Vector4Set( v->position, 0, 0, 0, 1 );

			for (unsigned k = 0; k < v->numWeights; k++, w++ )
			{
				vec3_t offsetVec;

				v->boneIndexes[ k ] = w->boneIndex;
				v->boneWeights[ k ] = w->boneWeight;

				bone = &md5->bones[ w->boneIndex ];

				QuatTransformVector( bone->rotation, w->offset, offsetVec );
				VectorAdd( bone->origin, offsetVec, offsetVec );

				VectorMA( v->position, w->boneWeight, offsetVec, v->position );
			}
		}
	}

	// loading is done now calculate the bounding box and tangent spaces
	ClearBounds( md5->bounds[ 0 ], md5->bounds[ 1 ] );

    surf = md5->surfaces;
	for (unsigned i = 0; i < md5->numSurfaces; i++, surf++ )
	{
        v = surf->verts;
		for (unsigned j = 0; j < surf->numVerts; j++, v++ )
		{
			AddPointToBounds( v->position, md5->bounds[ 0 ], md5->bounds[ 1 ] );
		}

		// calc tangent spaces
		{
			const float *v0, *v1, *v2;
			const float *t0, *t1, *t2;
			vec3_t      tangent;
			vec3_t      binormal;
			vec3_t      normal;

            v = surf->verts;
			for (unsigned j = 0; j < surf->numVerts; j++, v++ )
			{
				VectorClear( v->tangent );
				VectorClear( v->binormal );
				VectorClear( v->normal );
			}

            tri = surf->triangles;
			for (unsigned j = 0; j < surf->numTriangles; j++, tri++ )
			{
				v0 = surf->verts[ tri->indexes[ 0 ] ].position;
				v1 = surf->verts[ tri->indexes[ 1 ] ].position;
				v2 = surf->verts[ tri->indexes[ 2 ] ].position;

				t0 = surf->verts[ tri->indexes[ 0 ] ].texCoords;
				t1 = surf->verts[ tri->indexes[ 1 ] ].texCoords;
				t2 = surf->verts[ tri->indexes[ 2 ] ].texCoords;

				R_CalcFaceNormal( normal, v0, v1, v2 );
				R_CalcTangents( tangent, binormal, v0, v1, v2, t0, t1, t2 );

				for (unsigned k = 0; k < 3; k++ )
				{
					float *v;

					v = surf->verts[ tri->indexes[ k ] ].tangent;
					VectorAdd( v, tangent, v );

					v = surf->verts[ tri->indexes[ k ] ].binormal;
					VectorAdd( v, binormal, v );

					v = surf->verts[ tri->indexes[ k ] ].normal;
					VectorAdd( v, normal, v );
				}
			}
            v = surf->verts;
			for (unsigned j = 0; j < surf->numVerts; j++, v++ )
			{
				VectorNormalize( v->tangent );
				v->tangent[ 3 ] = 0;
				VectorNormalize( v->binormal );
				v->binormal[ 3 ] = 0;
				VectorNormalize( v->normal );
				v->normal[ 3 ] = 0;
			}
		}
	}
	md5->internalScale = BoundsMaxExtent( md5->bounds[ 0 ], md5->bounds[ 1 ] );
	if( md5->internalScale > 0.0f ) {
		float invScale = 1.0f / md5->internalScale;

        surf = md5->surfaces;
		for (unsigned i = 0; i < md5->numSurfaces; i++, surf++ )
		{
            v = surf->verts;
			for (unsigned j = 0; j < surf->numVerts; j++, v++ )
			{
				VectorScale( v->position, invScale, v->position );
			}
		}
	}

	// split the surfaces into VBO surfaces by the maximum number of GPU vertex skinning bones
	Com_InitGrowList( &vboSurfaces, 10 );

    surf = md5->surfaces;
	for (unsigned i = 0; i < md5->numSurfaces; i++, surf++ )
	{
		// sort triangles
		Com_InitGrowList( &sortedTriangles, 1000 );

        tri = surf->triangles;
		for (unsigned j = 0; j < surf->numTriangles; j++, tri++ )
		{
			skelTriangle_t *sortTri = (skelTriangle_t*) Com_Allocate( sizeof( *sortTri ) );

			for (unsigned k = 0; k < 3; k++ )
			{
				sortTri->indexes[ k ] = tri->indexes[ k ];
				sortTri->vertexes[ k ] = &surf->verts[ tri->indexes[ k ] ];
			}

			sortTri->referenced = false;

			Com_AddToGrowList( &sortedTriangles, sortTri );
		}

		numRemaining = sortedTriangles.currentElements;

		while ( numRemaining )
		{
			numBoneReferences = 0;
			Com_Memset( boneReferences, 0, sizeof( boneReferences ) );

			Com_InitGrowList( &vboTriangles, 1000 );

			for (int j = 0; j < sortedTriangles.currentElements; j++ )
			{
				skelTriangle_t *sortTri = (skelTriangle_t*) Com_GrowListElement( &sortedTriangles, j );

				if ( sortTri->referenced )
				{
					continue;
				}

				if ( AddTriangleToVBOTriangleList( &vboTriangles, sortTri, &numBoneReferences, boneReferences ) )
				{
					sortTri->referenced = true;
				}
			}

			if ( !vboTriangles.currentElements )
			{
				ri.Printf(printParm_t::PRINT_WARNING, "R_LoadMD5: could not add triangles to a remaining VBO surfaces for model '%s'\n", modName );
				Com_DestroyGrowList( &vboTriangles );
				break;
			}

			AddSurfaceToVBOSurfacesList( &vboSurfaces, &vboTriangles, md5, surf, i, boneReferences );
			numRemaining -= vboTriangles.currentElements;

			Com_DestroyGrowList( &vboTriangles );
		}

		for (int j = 0; j < sortedTriangles.currentElements; j++ )
		{
			skelTriangle_t *sortTri = (skelTriangle_t*) Com_GrowListElement( &sortedTriangles, j );

			Com_Dealloc( sortTri );
		}

		Com_DestroyGrowList( &sortedTriangles );
	}

	// move VBO surfaces list to hunk
	md5->numVBOSurfaces = vboSurfaces.currentElements;
	md5->vboSurfaces = (srfVBOMD5Mesh_t**) ri.Hunk_Alloc( md5->numVBOSurfaces * sizeof( *md5->vboSurfaces ), ha_pref::h_low );

	for ( unsigned i = 0; i < md5->numVBOSurfaces; i++ )
	{
		md5->vboSurfaces[ i ] = ( srfVBOMD5Mesh_t * ) Com_GrowListElement( &vboSurfaces, i );
	}

	Com_DestroyGrowList( &vboSurfaces );

	return true;
}
