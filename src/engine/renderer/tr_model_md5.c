/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of OpenWolf source code.

OpenWolf source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenWolf source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_model_md5.c -- Doom 3 .md5mesh model loading and caching

#include "tr_local.h"

/*
=================
R_LoadMD5
=================
*/
qboolean R_LoadMD5(model_t * mod, void *buffer, int bufferSize, const char *modName)
{
	int             i, j, k;
	md5Model_t     *md5;
	md5Bone_t      *bone;
	md5Surface_t   *surf;
	md5Triangle_t  *tri;
	md5Vertex_t    *v;
	md5Weight_t    *weight;
	int             version;
	shader_t       *sh;
	char           *buf_p;
	char           *token;
	vec3_t          boneOrigin;
	quat_t          boneQuat;
	matrix_t        boneMat;

	int				numRemaining;
	growList_t		sortedTriangles;
	growList_t      vboTriangles;
	growList_t      vboSurfaces;

	int				numBoneReferences;
	int				boneReferences[MAX_BONES];

	buf_p = (char *)buffer;

	// skip MD5Version indent string
	COM_ParseExt2(&buf_p, qfalse);

	// check version
	token = COM_ParseExt2(&buf_p, qfalse);
	version = atoi(token);
	if(version != MD5_VERSION)
	{
		ri.Printf(PRINT_WARNING, "R_LoadMD5: %s has wrong version (%i should be %i)\n", modName, version, MD5_VERSION);
		return qfalse;
	}

	mod->type = MOD_MD5;
	mod->dataSize += sizeof(md5Model_t);
	md5 = mod->model.md5 = ri.Hunk_Alloc(sizeof(md5Model_t), h_low);

	// skip commandline <arguments string>
	token = COM_ParseExt2(&buf_p, qtrue);
	token = COM_ParseExt2(&buf_p, qtrue);
//  ri.Printf(PRINT_ALL, "%s\n", token);

	// parse numJoints <number>
	token = COM_ParseExt2(&buf_p, qtrue);
	if(Q_stricmp(token, "numJoints"))
	{
		ri.Printf(PRINT_WARNING, "R_LoadMD5: expected 'numJoints' found '%s' in model '%s'\n", token, modName);
		return qfalse;
	}
	token = COM_ParseExt2(&buf_p, qfalse);
	md5->numBones = atoi(token);

	// parse numMeshes <number>
	token = COM_ParseExt2(&buf_p, qtrue);
	if(Q_stricmp(token, "numMeshes"))
	{
		ri.Printf(PRINT_WARNING, "R_LoadMD5: expected 'numMeshes' found '%s' in model '%s'\n", token, modName);
		return qfalse;
	}
	token = COM_ParseExt2(&buf_p, qfalse);
	md5->numSurfaces = atoi(token);
	//ri.Printf(PRINT_ALL, "R_LoadMD5: '%s' has %i surfaces\n", modName, md5->numSurfaces);


	if(md5->numBones < 1)
	{
		ri.Printf(PRINT_WARNING, "R_LoadMD5: '%s' has no bones\n", modName);
		return qfalse;
	}
	if(md5->numBones > MAX_BONES)
	{
		ri.Printf(PRINT_WARNING, "R_LoadMD5: '%s' has more than %i bones (%i)\n", modName, MAX_BONES, md5->numBones);
		return qfalse;
	}
	//ri.Printf(PRINT_ALL, "R_LoadMD5: '%s' has %i bones\n", modName, md5->numBones);

	// parse all the bones
	md5->bones = ri.Hunk_Alloc(sizeof(*bone) * md5->numBones, h_low);

	// parse joints {
	token = COM_ParseExt2(&buf_p, qtrue);
	if(Q_stricmp(token, "joints"))
	{
		ri.Printf(PRINT_WARNING, "R_LoadMD5: expected 'joints' found '%s' in model '%s'\n", token, modName);
		return qfalse;
	}
	token = COM_ParseExt2(&buf_p, qfalse);
	if(Q_stricmp(token, "{"))
	{
		ri.Printf(PRINT_WARNING, "R_LoadMD5: expected '{' found '%s' in model '%s'\n", token, modName);
		return qfalse;
	}

	for(i = 0, bone = md5->bones; i < md5->numBones; i++, bone++)
	{
		token = COM_ParseExt2(&buf_p, qtrue);
		Q_strncpyz(bone->name, token, sizeof(bone->name));

		//ri.Printf(PRINT_ALL, "R_LoadMD5: '%s' has bone '%s'\n", modName, bone->name);

		token = COM_ParseExt2(&buf_p, qfalse);
		bone->parentIndex = atoi(token);

		//ri.Printf(PRINT_ALL, "R_LoadMD5: '%s' has bone '%s' with parent index %i\n", modName, bone->name, bone->parentIndex);

		if(bone->parentIndex >= md5->numBones)
		{
			ri.Error(ERR_DROP, "R_LoadMD5: '%s' has bone '%s' with bad parent index %i while numBones is %i\n", modName,
					 bone->name, bone->parentIndex, md5->numBones);
		}

		// skip (
		token = COM_ParseExt2(&buf_p, qfalse);
		if(Q_stricmp(token, "("))
		{
			ri.Printf(PRINT_WARNING, "R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		for(j = 0; j < 3; j++)
		{
			token = COM_ParseExt2(&buf_p, qfalse);
			boneOrigin[j] = atof(token);
		}

		// skip )
		token = COM_ParseExt2(&buf_p, qfalse);
		if(Q_stricmp(token, ")"))
		{
			ri.Printf(PRINT_WARNING, "R_LoadMD5: expected ')' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		// skip (
		token = COM_ParseExt2(&buf_p, qfalse);
		if(Q_stricmp(token, "("))
		{
			ri.Printf(PRINT_WARNING, "R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		for(j = 0; j < 3; j++)
		{
			token = COM_ParseExt2(&buf_p, qfalse);
			boneQuat[j] = atof(token);
		}
		QuatCalcW(boneQuat);
		MatrixFromQuat(boneMat, boneQuat);

		VectorCopy(boneOrigin, bone->origin);
		QuatCopy(boneQuat, bone->rotation);

		MatrixSetupTransformFromQuat(bone->inverseTransform, boneQuat, boneOrigin);
		MatrixInverse(bone->inverseTransform);

		// skip )
		token = COM_ParseExt2(&buf_p, qfalse);
		if(Q_stricmp(token, ")"))
		{
			ri.Printf(PRINT_WARNING, "R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}
	}

	// parse }
	token = COM_ParseExt2(&buf_p, qtrue);
	if(Q_stricmp(token, "}"))
	{
		ri.Printf(PRINT_WARNING, "R_LoadMD5: expected '}' found '%s' in model '%s'\n", token, modName);
		return qfalse;
	}

	// parse all the surfaces
	if(md5->numSurfaces < 1)
	{
		ri.Printf(PRINT_WARNING, "R_LoadMD5: '%s' has no surfaces\n", modName);
		return qfalse;
	}
	//ri.Printf(PRINT_ALL, "R_LoadMD5: '%s' has %i surfaces\n", modName, md5->numSurfaces);

	md5->surfaces = ri.Hunk_Alloc(sizeof(*surf) * md5->numSurfaces, h_low);
	for(i = 0, surf = md5->surfaces; i < md5->numSurfaces; i++, surf++)
	{
		// parse mesh {
		token = COM_ParseExt2(&buf_p, qtrue);
		if(Q_stricmp(token, "mesh"))
		{
			ri.Printf(PRINT_WARNING, "R_LoadMD5: expected 'mesh' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}
		token = COM_ParseExt2(&buf_p, qfalse);
		if(Q_stricmp(token, "{"))
		{
			ri.Printf(PRINT_WARNING, "R_LoadMD5: expected '{' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		// change to surface identifier
		surf->surfaceType = SF_MD5;

		// give pointer to model for Tess_SurfaceMD5
		surf->model = md5;

		// parse shader <name>
		token = COM_ParseExt2(&buf_p, qtrue);
		if(Q_stricmp(token, "shader"))
		{
			Q_strncpyz(surf->shader, "<default>", sizeof(surf->shader));
			surf->shaderIndex = 0;
		} else {
			token = COM_ParseExt2(&buf_p, qfalse);
			Q_strncpyz(surf->shader, token, sizeof(surf->shader));

			//ri.Printf(PRINT_ALL, "R_LoadMD5: '%s' uses shader '%s'\n", modName, surf->shader);

			// FIXME .md5mesh meshes don't have surface names
			// lowercase the surface name so skin compares are faster
			//Q_strlwr(surf->name);
			//ri.Printf(PRINT_ALL, "R_LoadMD5: '%s' has surface '%s'\n", modName, surf->name);

			// register the shaders
			sh = R_FindShader(surf->shader, LIGHTMAP_NONE, qtrue);
			if(sh->defaultShader)
			{
				surf->shaderIndex = 0;
			}
			else
			{
				surf->shaderIndex = sh->index;
			}
			token = COM_ParseExt2(&buf_p, qtrue);
		}

		// parse numVerts <number>
		if(Q_stricmp(token, "numVerts"))
		{
			ri.Printf(PRINT_WARNING, "R_LoadMD5: expected 'numVerts' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}
		token = COM_ParseExt2(&buf_p, qfalse);
		surf->numVerts = atoi(token);

		if(surf->numVerts > SHADER_MAX_VERTEXES)
		{
			ri.Error(ERR_DROP, "R_LoadMD5: '%s' has more than %i verts on a surface (%i)",
					 modName, SHADER_MAX_VERTEXES, surf->numVerts);
		}

		surf->verts = ri.Hunk_Alloc(sizeof(*v) * surf->numVerts, h_low);
		for(j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
		{
			// skip vert <number>
			token = COM_ParseExt2(&buf_p, qtrue);
			if(Q_stricmp(token, "vert"))
			{
				ri.Printf(PRINT_WARNING, "R_LoadMD5: expected 'vert' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}
			COM_ParseExt2(&buf_p, qfalse);

			// skip (
			token = COM_ParseExt2(&buf_p, qfalse);
			if(Q_stricmp(token, "("))
			{
				ri.Printf(PRINT_WARNING, "R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}

			for(k = 0; k < 2; k++)
			{
				token = COM_ParseExt2(&buf_p, qfalse);
				v->texCoords[k] = atof(token);
			}

			// skip )
			token = COM_ParseExt2(&buf_p, qfalse);
			if(Q_stricmp(token, ")"))
			{
				ri.Printf(PRINT_WARNING, "R_LoadMD5: expected ')' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}

			token = COM_ParseExt2(&buf_p, qfalse);
			v->firstWeight = atoi(token);

			token = COM_ParseExt2(&buf_p, qfalse);
			v->numWeights = atoi(token);

			if(v->numWeights > MAX_WEIGHTS)
			{
				ri.Error(ERR_DROP, "R_LoadMD5: vertex %i requires more than %i weights on surface (%i) in model '%s'",
						 j, MAX_WEIGHTS, i, modName);
			}
		}

		// parse numTris <number>
		token = COM_ParseExt2(&buf_p, qtrue);
		if(Q_stricmp(token, "numTris"))
		{
			ri.Printf(PRINT_WARNING, "R_LoadMD5: expected 'numTris' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}
		token = COM_ParseExt2(&buf_p, qfalse);
		surf->numTriangles = atoi(token);

		if(surf->numTriangles > SHADER_MAX_TRIANGLES)
		{
			ri.Error(ERR_DROP, "R_LoadMD5: '%s' has more than %i triangles on a surface (%i)",
					 modName, SHADER_MAX_TRIANGLES, surf->numTriangles);
		}

		surf->triangles = ri.Hunk_Alloc(sizeof(*tri) * surf->numTriangles, h_low);
		for(j = 0, tri = surf->triangles; j < surf->numTriangles; j++, tri++)
		{
			// skip tri <number>
			token = COM_ParseExt2(&buf_p, qtrue);
			if(Q_stricmp(token, "tri"))
			{
				ri.Printf(PRINT_WARNING, "R_LoadMD5: expected 'tri' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}
			COM_ParseExt2(&buf_p, qfalse);

			for(k = 0; k < 3; k++)
			{
				token = COM_ParseExt2(&buf_p, qfalse);
				tri->indexes[k] = atoi(token);
			}
		}

		// parse numWeights <number>
		token = COM_ParseExt2(&buf_p, qtrue);
		if(Q_stricmp(token, "numWeights"))
		{
			ri.Printf(PRINT_WARNING, "R_LoadMD5: expected 'numWeights' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}
		token = COM_ParseExt2(&buf_p, qfalse);
		surf->numWeights = atoi(token);

		surf->weights = ri.Hunk_Alloc(sizeof(*weight) * surf->numWeights, h_low);
		for(j = 0, weight = surf->weights; j < surf->numWeights; j++, weight++)
		{
			// skip weight <number>
			token = COM_ParseExt2(&buf_p, qtrue);
			if(Q_stricmp(token, "weight"))
			{
				ri.Printf(PRINT_WARNING, "R_LoadMD5: expected 'weight' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}
			COM_ParseExt2(&buf_p, qfalse);

			token = COM_ParseExt2(&buf_p, qfalse);
			weight->boneIndex = atoi(token);

			token = COM_ParseExt2(&buf_p, qfalse);
			weight->boneWeight = atof(token);

			// skip (
			token = COM_ParseExt2(&buf_p, qfalse);
			if(Q_stricmp(token, "("))
			{
				ri.Printf(PRINT_WARNING, "R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}

			for(k = 0; k < 3; k++)
			{
				token = COM_ParseExt2(&buf_p, qfalse);
				weight->offset[k] = atof(token);
			}

			// skip )
			token = COM_ParseExt2(&buf_p, qfalse);
			if(Q_stricmp(token, ")"))
			{
				ri.Printf(PRINT_WARNING, "R_LoadMD5: expected ')' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}
		}

		// parse }
		token = COM_ParseExt2(&buf_p, qtrue);
		if(Q_stricmp(token, "}"))
		{
			ri.Printf(PRINT_WARNING, "R_LoadMD5: expected '}' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		// loop trough all vertices and set up the vertex weights
		for(j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
		{
			v->weights = ri.Hunk_Alloc(sizeof(*v->weights) * v->numWeights, h_low);

			for(k = 0; k < v->numWeights; k++)
			{
				v->weights[k] = surf->weights + (v->firstWeight + k);
			}
		}
	}

	// loading is done now calculate the bounding box and tangent spaces
	ClearBounds(md5->bounds[0], md5->bounds[1]);

	for(i = 0, surf = md5->surfaces; i < md5->numSurfaces; i++, surf++)
	{
		for(j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
		{
			vec3_t          tmpVert;
			md5Weight_t    *w;

			VectorClear(tmpVert);

			for(k = 0, w = v->weights[0]; k < v->numWeights; k++, w++)
			{
				vec3_t          offsetVec;

				bone = &md5->bones[w->boneIndex];

				QuatTransformVector(bone->rotation, w->offset, offsetVec);
				VectorAdd(bone->origin, offsetVec, offsetVec);

				VectorMA(tmpVert, w->boneWeight, offsetVec, tmpVert);
			}

			VectorCopy(tmpVert, v->position);
			AddPointToBounds(tmpVert, md5->bounds[0], md5->bounds[1]);
		}

		// calc normals
		{
			const float    *v0, *v1, *v2;
			const float    *t0, *t1, *t2;
			vec3_t          tangent;
			vec3_t          binormal;
			vec3_t          normal;

			for(j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
			{
				VectorClear(v->tangent);
				VectorClear(v->binormal);
				VectorClear(v->normal);
			}

			for(j = 0, tri = surf->triangles; j < surf->numTriangles; j++, tri++)
			{
				v0 = surf->verts[tri->indexes[0]].position;
				v1 = surf->verts[tri->indexes[1]].position;
				v2 = surf->verts[tri->indexes[2]].position;

				t0 = surf->verts[tri->indexes[0]].texCoords;
				t1 = surf->verts[tri->indexes[1]].texCoords;
				t2 = surf->verts[tri->indexes[2]].texCoords;

				R_CalcNormalForTriangle(normal, v0, v1, v2);

				for(k = 0; k < 3; k++)
				{
					float          *v;

					v = surf->verts[tri->indexes[k]].normal;
					VectorAdd(v, normal, v);
				}
			}

			for(j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
			{
				VectorNormalize(v->normal);
			}
		}

#if 0
		// do another extra smoothing for normals to avoid flat shading
		for(j = 0; j < surf->numVerts; j++)
		{
			for(k = 0; k < surf->numVerts; k++)
			{
				if(j == k)
					continue;

				if(VectorCompare(surf->verts[j].position, surf->verts[k].position))
				{
					VectorAdd(surf->verts[j].normal, surf->verts[k].normal, surf->verts[j].normal);
				}
			}

			VectorNormalize(surf->verts[j].normal);
		}
#endif
	}

	return qtrue;
}


