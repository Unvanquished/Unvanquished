/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2010 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/
// tr_model_mdm.c -- Enemy Territory .mdm model loading and caching

#include "tr_local.h"
#include "tr_model_skel.h"

#define	LL(x) x=LittleLong(x)
#define	LF(x) x=LittleFloat(x)

const float mdmLODResolutions[MD3_MAX_LODS] = {1.0f, 0.75f, 0.5f, 0.35f};

static void AddSurfaceToVBOSurfacesListMDM(growList_t * vboSurfaces, growList_t * vboTriangles, mdmModel_t * mdm, mdmSurfaceIntern_t * surf, int skinIndex, int numBoneReferences, int boneReferences[MAX_BONES])
{
	int				j, k, lod;

	int             vertexesNum;
	byte           *data;
	int             dataSize;
	int             dataOfs;

	GLuint          ofsTexCoords;
	GLuint          ofsTangents;
	GLuint          ofsBinormals;
	GLuint          ofsNormals;
	GLuint          ofsBoneIndexes;
	GLuint          ofsBoneWeights;

	int             indexesNum;
	byte           *indexes;
	int             indexesSize;
	int             indexesOfs;

	skelTriangle_t  *tri;

	vec4_t          tmp;
	int             index;

	srfVBOMDMMesh_t *vboSurf;
	md5Vertex_t     *v;

//	vec4_t          tmpColor = { 1, 1, 1, 1 };

	static int32_t  collapse[MDM_MAX_VERTS];


	vertexesNum = surf->numVerts;
	indexesNum = vboTriangles->currentElements * 3;

	// create surface
	vboSurf = ri.Hunk_Alloc(sizeof(*vboSurf), h_low);
	Com_AddToGrowList(vboSurfaces, vboSurf);

	vboSurf->surfaceType = SF_VBO_MDMMESH;
	vboSurf->mdmModel = mdm;
	vboSurf->mdmSurface = surf;
	vboSurf->shader = R_GetShaderByHandle(surf->shaderIndex);
	vboSurf->skinIndex = skinIndex;
	vboSurf->numIndexes = indexesNum;
	vboSurf->numVerts = vertexesNum;

	dataSize = vertexesNum * (sizeof(vec4_t) * 7);
	data = ri.Hunk_AllocateTempMemory(dataSize);
	dataOfs = 0;



	//ri.Printf(PRINT_ALL, "AddSurfaceToVBOSurfacesList( %i verts, %i tris )\n", surf->numVerts, vboTriangles->currentElements);

	vboSurf->numBoneRemap = 0;
	Com_Memset(vboSurf->boneRemap, 0, sizeof(vboSurf->boneRemap));
	Com_Memset(vboSurf->boneRemapInverse, 0, sizeof(vboSurf->boneRemapInverse));

	//ri.Printf(PRINT_ALL, "original referenced bones: [ ");
	//for(j = 0; j < surf->numBoneReferences; j++)
	//{
	//	ri.Printf(PRINT_ALL, "%i, ", surf->boneReferences[j]);
	//}
	//ri.Printf(PRINT_ALL, "]\n");

	//ri.Printf(PRINT_ALL, "new referenced bones: ");
	for(j = 0; j < MAX_BONES; j++)
	{
		if(boneReferences[j] > 0)
		{
			vboSurf->boneRemap[j] = vboSurf->numBoneRemap;
			vboSurf->boneRemapInverse[vboSurf->numBoneRemap] = j;

			vboSurf->numBoneRemap++;

			//ri.Printf(PRINT_ALL, "(%i -> %i) ", j, vboSurf->boneRemap[j]);
		}
	}
	//ri.Printf(PRINT_ALL, "\n");

	// feed vertex XYZ
	for(j = 0; j < vertexesNum; j++)
	{
		for(k = 0; k < 3; k++)
		{
			tmp[k] = surf->verts[j].position[k];
		}
		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, (vec_t *) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed vertex texcoords
	ofsTexCoords = dataOfs;
	for(j = 0; j < vertexesNum; j++)
	{
		for(k = 0; k < 2; k++)
		{
			tmp[k] = surf->verts[j].texCoords[k];
		}
		tmp[2] = 0;
		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, (vec_t *) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed vertex tangents
	ofsTangents = dataOfs;
	for(j = 0; j < vertexesNum; j++)
	{
		for(k = 0; k < 3; k++)
		{
			tmp[k] = surf->verts[j].tangent[k];
		}
		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, (vec_t *) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed vertex binormals
	ofsBinormals = dataOfs;
	for(j = 0; j < vertexesNum; j++)
	{
		for(k = 0; k < 3; k++)
		{
			tmp[k] = surf->verts[j].binormal[k];
		}
		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, (vec_t *) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed vertex normals
	ofsNormals = dataOfs;
	for(j = 0; j < vertexesNum; j++)
	{
		for(k = 0; k < 3; k++)
		{
			tmp[k] = surf->verts[j].normal[k];
		}
		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, (vec_t *) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed bone indices
	ofsBoneIndexes = dataOfs;
	for(j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
	{
		for(k = 0; k < MAX_WEIGHTS; k++)
		{
			if(k < v->numWeights)
				index = vboSurf->boneRemap[v->weights[k]->boneIndex];
			else
				index = 0;

			Com_Memcpy(data + dataOfs, &index, sizeof(int));
			dataOfs += sizeof(int);
		}
	}

	// feed bone weights
	ofsBoneWeights = dataOfs;
	for(j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
	{
		for(k = 0; k < MAX_WEIGHTS; k++)
		{
			if(k < v->numWeights)
				tmp[k] = v->weights[k]->boneWeight;
			else
				tmp[k] = 0;
		}
		Com_Memcpy(data + dataOfs, (vec_t *) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	vboSurf->vbo = R_CreateVBO(va("staticMDMMesh_VBO %i", vboSurfaces->currentElements), data, dataSize, VBO_USAGE_STATIC);
	vboSurf->vbo->ofsXYZ = 0;
	vboSurf->vbo->ofsTexCoords = ofsTexCoords;
	vboSurf->vbo->ofsLightCoords = ofsTexCoords;
	vboSurf->vbo->ofsTangents = ofsTangents;
	vboSurf->vbo->ofsBinormals = ofsBinormals;
	vboSurf->vbo->ofsNormals = ofsNormals;
	vboSurf->vbo->ofsColors = ofsNormals;
	vboSurf->vbo->ofsLightCoords = 0;		// not required anyway
	vboSurf->vbo->ofsLightDirections = 0;	// not required anyway
	vboSurf->vbo->ofsBoneIndexes = ofsBoneIndexes;
	vboSurf->vbo->ofsBoneWeights = ofsBoneWeights;

	// calculate LOD IBOs
	lod = 0;
	do
	{
		float		flod;
		int			renderCount;

		flod = mdmLODResolutions[lod];

		renderCount = Q_min((int)((float)surf->numVerts * flod), surf->numVerts);

		if(renderCount < surf->minLod)
		{
			renderCount = surf->minLod;
			flod = (float)renderCount / surf->numVerts;
		}

		if(renderCount == surf->numVerts)
		{
			indexesNum = vboTriangles->currentElements * 3;
			indexesSize = indexesNum * sizeof(int);
			indexes = ri.Hunk_AllocateTempMemory(indexesSize);
			indexesOfs = 0;

			for(j = 0; j < vboTriangles->currentElements; j++)
			{
				tri = Com_GrowListElement(vboTriangles, j);

				for(k = 0; k < 3; k++)
				{
					index = tri->indexes[k];

					Com_Memcpy(indexes + indexesOfs, &index, sizeof(int));
					indexesOfs += sizeof(int);
				}
			}
		}
		else
		{
			int				ci[3];
			int32_t        *pCollapseMap;
			int32_t        *pCollapse;

			pCollapse = collapse;
			for(j = 0; j < renderCount; pCollapse++, j++)
			{
				*pCollapse = j;
			}

			pCollapseMap = &surf->collapseMap[renderCount];
			for(j = renderCount; j < surf->numVerts; j++, pCollapse++, pCollapseMap++)
			{
				int32_t collapseValue = *pCollapseMap;
				*pCollapse = collapse[collapseValue];
			}


			indexesNum = 0;
			for(j = 0; j < vboTriangles->currentElements; j++)
			{
				tri = Com_GrowListElement(vboTriangles, j);

				ci[0] = collapse[tri->indexes[0]];
				ci[1] = collapse[tri->indexes[1]];
				ci[2] = collapse[tri->indexes[2]];

				// FIXME
				// note:  serious optimization opportunity here,
				//  by sorting the triangles the following "continue"
				//  could have been made into a "break" statement.
				if(ci[0] == ci[1] || ci[1] == ci[2] || ci[2] == ci[0])
				{
					continue;
				}

				indexesNum += 3;
			}


			indexesSize = indexesNum * sizeof(int);
			indexes = ri.Hunk_AllocateTempMemory(indexesSize);
			indexesOfs = 0;

			for(j = 0; j < vboTriangles->currentElements; j++)
			{
				tri = Com_GrowListElement(vboTriangles, j);

				ci[0] = collapse[tri->indexes[0]];
				ci[1] = collapse[tri->indexes[1]];
				ci[2] = collapse[tri->indexes[2]];

				// FIXME
				// note:  serious optimization opportunity here,
				//  by sorting the triangles the following "continue"
				//  could have been made into a "break" statement.
				if(ci[0] == ci[1] || ci[1] == ci[2] || ci[2] == ci[0])
				{
					continue;
				}

				for(k = 0; k < 3; k++)
				{
					index = ci[k];

					Com_Memcpy(indexes + indexesOfs, &index, sizeof(int));
					indexesOfs += sizeof(int);
				}
			}
		}

		vboSurf->ibo[lod] = R_CreateIBO(va("staticMDMMesh_IBO_LOD_%f %i", flod, indexesNum / 3), indexes, indexesSize, VBO_USAGE_STATIC);
		vboSurf->ibo[lod]->indexesNum = indexesNum;

		ri.Hunk_FreeTempMemory(indexes);

		if(vboTriangles->currentElements != surf->numTriangles)
		{
			ri.Printf(PRINT_WARNING, "Can't calculate LOD IBOs\n");
			break;
		}

		lod++;
	}
	while(lod < MD3_MAX_LODS);

	ri.Hunk_FreeTempMemory(data);

	// megs
	/*
	   ri.Printf(PRINT_ALL, "md5 mesh data VBO size: %d.%02d MB\n", dataSize / (1024 * 1024),
	   (dataSize % (1024 * 1024)) * 100 / (1024 * 1024));
	   ri.Printf(PRINT_ALL, "md5 mesh tris VBO size: %d.%02d MB\n", indexesSize / (1024 * 1024),
	   (indexesSize % (1024 * 1024)) * 100 / (1024 * 1024));
	 */
}


/*
=================
R_LoadMDM
=================
*/
qboolean R_LoadMDM(model_t * mod, void *buffer, const char *modName)
{
	int             i, j, k;

	mdmHeader_t    *mdm;
//    mdmFrame_t            *frame;
	mdmSurface_t   *mdmSurf;
	mdmTriangle_t  *mdmTri;
	mdmVertex_t    *mdmVertex;
	mdmTag_t       *mdmTag;
	int             version;
//	int             size;
	shader_t       *sh;
	int32_t        *collapseMap, *collapseMapOut, *boneref, *bonerefOut;


	mdmModel_t     *mdmModel;
	mdmTagIntern_t			*tag;
	mdmSurfaceIntern_t		*surf;
	srfTriangle_t			*tri;
	md5Vertex_t             *v;

	mdm = (mdmHeader_t *) buffer;

	version = LittleLong(mdm->version);
	if(version != MDM_VERSION)
	{
		ri.Printf(PRINT_WARNING, "R_LoadMDM: %s has wrong version (%i should be %i)\n", modName, version, MDM_VERSION);
		return qfalse;
	}

	mod->type = MOD_MDM;
//	size = LittleLong(mdm->ofsEnd);
	mod->dataSize += sizeof(mdmModel_t);

	//mdm = mod->mdm = ri.Hunk_Alloc(size, h_low);
	//memcpy(mdm, buffer, LittleLong(pinmodel->ofsEnd));

	mdmModel = mod->mdm = ri.Hunk_Alloc(sizeof(mdmModel_t), h_low);

	LL(mdm->ident);
	LL(mdm->version);
//    LL(mdm->numFrames);
	LL(mdm->numTags);
	LL(mdm->numSurfaces);
//    LL(mdm->ofsFrames);
	LL(mdm->ofsTags);
	LL(mdm->ofsEnd);
	LL(mdm->ofsSurfaces);

	mdmModel->lodBias = LittleFloat(mdm->lodBias);
	mdmModel->lodScale = LittleFloat(mdm->lodScale);

/*	mdm->skel = RE_RegisterModel(mdm->bonesfile);
	if ( !mdm->skel ) {
		ri.Error (ERR_DROP, "R_LoadMDM: %s skeleton not found\n", mdm->bonesfile );
	}

	if ( mdm->numFrames < 1 ) {
		ri.Printf( PRINT_WARNING, "R_LoadMDM: %s has no frames\n", modName );
		return qfalse;
	}*/



	// swap all the frames
	/*frameSize = (int) ( sizeof( mdmFrame_t ) );
	   for ( i = 0 ; i < mdm->numFrames ; i++, frame++) {
	   frame = (mdmFrame_t *) ( (byte *)mdm + mdm->ofsFrames + i * frameSize );
	   frame->radius = LittleFloat( frame->radius );
	   for ( j = 0 ; j < 3 ; j++ ) {
	   frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
	   frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
	   frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
	   frame->parentOffset[j] = LittleFloat( frame->parentOffset[j] );
	   }
	   } */

	// swap all the tags
	mdmModel->numTags = mdm->numTags;
	mdmModel->tags = tag = ri.Hunk_Alloc(sizeof(*tag) * mdm->numTags, h_low);

	mdmTag = (mdmTag_t *) ((byte *) mdm + mdm->ofsTags);
	for(i = 0; i < mdm->numTags; i++, tag++)
	{
		int             ii;

		Q_strncpyz(tag->name, mdmTag->name, sizeof(tag->name));

		for(ii = 0; ii < 3; ii++)
		{
			tag->axis[ii][0] = LittleFloat(mdmTag->axis[ii][0]);
			tag->axis[ii][1] = LittleFloat(mdmTag->axis[ii][1]);
			tag->axis[ii][2] = LittleFloat(mdmTag->axis[ii][2]);
		}

		tag->boneIndex = LittleLong(mdmTag->boneIndex);
		//tag->torsoWeight = LittleFloat( tag->torsoWeight );
		tag->offset[0] = LittleFloat(mdmTag->offset[0]);
		tag->offset[1] = LittleFloat(mdmTag->offset[1]);
		tag->offset[2] = LittleFloat(mdmTag->offset[2]);


		LL(mdmTag->numBoneReferences);
		LL(mdmTag->ofsBoneReferences);
		LL(mdmTag->ofsEnd);

		tag->numBoneReferences = mdmTag->numBoneReferences;
		tag->boneReferences = ri.Hunk_Alloc(sizeof(*bonerefOut) * mdmTag->numBoneReferences, h_low);

		// swap the bone references
		boneref = (int32_t *)((byte *) mdmTag + mdmTag->ofsBoneReferences);
		for(j = 0, bonerefOut = tag->boneReferences; j < mdmTag->numBoneReferences; j++, boneref++, bonerefOut++)
		{
			*bonerefOut = LittleLong(*boneref);
		}


		// find the next tag
		mdmTag = (mdmTag_t *) ((byte *) mdmTag + mdmTag->ofsEnd);
	}


	// swap all the surfaces
	mdmModel->numSurfaces = mdm->numSurfaces;
	mdmModel->surfaces = ri.Hunk_Alloc(sizeof(*surf) * mdmModel->numSurfaces, h_low);

	mdmSurf = (mdmSurface_t *) ((byte *) mdm + mdm->ofsSurfaces);
	for(i = 0, surf = mdmModel->surfaces; i < mdm->numSurfaces; i++, surf++)
	{
		LL(mdmSurf->shaderIndex);
		LL(mdmSurf->ofsHeader);
		LL(mdmSurf->ofsCollapseMap);
		LL(mdmSurf->numTriangles);
		LL(mdmSurf->ofsTriangles);
		LL(mdmSurf->numVerts);
		LL(mdmSurf->ofsVerts);
		LL(mdmSurf->numBoneReferences);
		LL(mdmSurf->ofsBoneReferences);
		LL(mdmSurf->ofsEnd);

		surf->minLod = LittleLong(mdmSurf->minLod);

		// change to surface identifier
		surf->surfaceType = SF_MDM;
		surf->model = mdmModel;

		Q_strncpyz(surf->name, mdmSurf->name, sizeof(surf->name));

		if(mdmSurf->numVerts > SHADER_MAX_VERTEXES)
		{
			ri.Error(ERR_DROP, "R_LoadMDM: %s has more than %i verts on a surface (%i)",
					 modName, SHADER_MAX_VERTEXES, mdmSurf->numVerts);
		}

		if(mdmSurf->numTriangles > SHADER_MAX_TRIANGLES)
		{
			ri.Error(ERR_DROP, "R_LoadMDM: %s has more than %i triangles on a surface (%i)",
						modName, SHADER_MAX_TRIANGLES, mdmSurf->numTriangles);
		}

		// register the shaders
		if(mdmSurf->shader[0])
		{
			Q_strncpyz(surf->shader, mdmSurf->shader, sizeof(surf->shader));

			sh = R_FindShader(surf->shader, SHADER_3D_DYNAMIC, qtrue);
			if(sh->defaultShader)
			{
				surf->shaderIndex = 0;
			}
			else
			{
				surf->shaderIndex = sh->index;
			}
		}
		else
		{
			surf->shaderIndex = 0;
		}


		// swap all the triangles
		surf->numTriangles = mdmSurf->numTriangles;
		surf->triangles = ri.Hunk_Alloc(sizeof(*tri) * surf->numTriangles, h_low);

		mdmTri = (mdmTriangle_t *) ((byte *) mdmSurf + mdmSurf->ofsTriangles);
		for(j = 0, tri = surf->triangles; j < surf->numTriangles; j++, mdmTri++, tri++)
		{
			tri->indexes[0] = LittleLong(mdmTri->indexes[0]);
			tri->indexes[1] = LittleLong(mdmTri->indexes[1]);
			tri->indexes[2] = LittleLong(mdmTri->indexes[2]);
		}

		// swap all the vertexes
		surf->numVerts = mdmSurf->numVerts;
		surf->verts = ri.Hunk_Alloc(sizeof(*v) * surf->numVerts, h_low);

		mdmVertex = (mdmVertex_t *) ((byte *) mdmSurf + mdmSurf->ofsVerts);
		for(j = 0, v = surf->verts; j < mdmSurf->numVerts; j++, v++)
		{
			v->normal[0] = LittleFloat(mdmVertex->normal[0]);
			v->normal[1] = LittleFloat(mdmVertex->normal[1]);
			v->normal[2] = LittleFloat(mdmVertex->normal[2]);

			v->texCoords[0] = LittleFloat(mdmVertex->texCoords[0]);
			v->texCoords[1] = LittleFloat(mdmVertex->texCoords[1]);

			v->numWeights = LittleLong(mdmVertex->numWeights);

			if(v->numWeights > MAX_WEIGHTS)
			{
#if 0
				ri.Error(ERR_DROP, "R_LoadMDM: vertex %i requires %i instead of maximum %i weights on surface (%i) in model '%s'",
					j, v->numWeights, MAX_WEIGHTS, i, modName);
#else
				ri.Printf(PRINT_WARNING, "WARNING: R_LoadMDM: vertex %i requires %i instead of maximum %i weights on surface (%i) in model '%s'\n",
					j, v->numWeights, MAX_WEIGHTS, i, modName);
#endif
			}

			v->weights = ri.Hunk_Alloc(sizeof(*v->weights) * v->numWeights, h_low);
			for(k = 0; k < v->numWeights; k++)
			{
				md5Weight_t *weight = ri.Hunk_Alloc(sizeof(*weight), h_low);

				weight->boneIndex = LittleLong(mdmVertex->weights[k].boneIndex);
				weight->boneWeight = LittleFloat(mdmVertex->weights[k].boneWeight);
				weight->offset[0] = LittleFloat(mdmVertex->weights[k].offset[0]);
				weight->offset[1] = LittleFloat(mdmVertex->weights[k].offset[1]);
				weight->offset[2] = LittleFloat(mdmVertex->weights[k].offset[2]);

				v->weights[k] = weight;
			}

			mdmVertex = (mdmVertex_t *) &mdmVertex->weights[v->numWeights];
		}

		// swap the collapse map
		surf->collapseMap = ri.Hunk_Alloc(sizeof(*collapseMapOut) * mdmSurf->numVerts, h_low);

		collapseMap = (int32_t *)((byte *) mdmSurf + mdmSurf->ofsCollapseMap);
		//ri.Printf(PRINT_ALL, "collapse map for mdm surface '%s': ", surf->name);
		for(j = 0, collapseMapOut = surf->collapseMap; j < mdmSurf->numVerts; j++, collapseMap++, collapseMapOut++)
		{
			int32_t value = LittleLong(*collapseMap);
			//surf->collapseMap[j] = value;
			*collapseMapOut = value;

			//ri.Printf(PRINT_ALL, "(%i -> %i) ", j, value);
		}
		//ri.Printf(PRINT_ALL, "\n");

#if 0
		ri.Printf(PRINT_ALL, "collapse map for mdm surface '%s': ", surf->name);
		for(j = 0, collapseMap = surf->collapseMap; j < mdmSurf->numVerts; j++, collapseMap++)
		{
			ri.Printf(PRINT_ALL, "(%i -> %i) ", j, *collapseMap);
		}
		ri.Printf(PRINT_ALL, "\n");
#endif

		// swap the bone references
		surf->numBoneReferences = mdmSurf->numBoneReferences;
		surf->boneReferences = ri.Hunk_Alloc(sizeof(*bonerefOut) * mdmSurf->numBoneReferences, h_low);

		boneref = (int32_t *)((byte *) mdmSurf + mdmSurf->ofsBoneReferences);
		for(j = 0, bonerefOut = surf->boneReferences; j < surf->numBoneReferences; j++, boneref++, bonerefOut++)
		{
			*bonerefOut = LittleLong(*boneref);
		}

		// find the next surface
		mdmSurf = (mdmSurface_t *) ((byte *) mdmSurf + mdmSurf->ofsEnd);
	}

	// loading is done now calculate the bounding box and tangent spaces
	ClearBounds(mdmModel->bounds[0], mdmModel->bounds[1]);

	for(i = 0, surf = mdmModel->surfaces; i < mdmModel->numSurfaces; i++, surf++)
	{
		for(j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
		{
			vec3_t          tmpVert;
			md5Weight_t    *w;

			VectorClear(tmpVert);

			for(k = 0, w = v->weights[0]; k < v->numWeights; k++, w++)
			{
				//vec3_t          offsetVec;

				//VectorClear(offsetVec);

				//bone = &md5->bones[w->boneIndex];

				//QuatTransformVector(bone->rotation, w->offset, offsetVec);
				//VectorAdd(bone->origin, offsetVec, offsetVec);

				VectorMA(tmpVert, w->boneWeight, w->offset, tmpVert);
			}

			VectorCopy(tmpVert, v->position);
			AddPointToBounds(tmpVert, mdmModel->bounds[0], mdmModel->bounds[1]);
		}

		// calc tangent spaces
#if 0
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

#if 1
				R_CalcTangentSpace(tangent, binormal, normal, v0, v1, v2, t0, t1, t2);
#else
				R_CalcNormalForTriangle(normal, v0, v1, v2);
				R_CalcTangentsForTriangle(tangent, binormal, v0, v1, v2, t0, t1, t2);
#endif

				for(k = 0; k < 3; k++)
				{
					float          *v;

					v = surf->verts[tri->indexes[k]].tangent;
					VectorAdd(v, tangent, v);

					v = surf->verts[tri->indexes[k]].binormal;
					VectorAdd(v, binormal, v);

					v = surf->verts[tri->indexes[k]].normal;
					VectorAdd(v, normal, v);
				}
			}

			for(j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
			{
				VectorNormalize(v->tangent);
				VectorNormalize(v->binormal);
				VectorNormalize(v->normal);
			}
		}
#else
		{
			int             k;
			float           bb, s, t;
			vec3_t          bary;
			vec3_t			faceNormal;
			md5Vertex_t    *dv[3];

			for(j = 0, tri = surf->triangles; j < surf->numTriangles; j++, tri++)
			{
				dv[0] = &surf->verts[tri->indexes[0]];
				dv[1] = &surf->verts[tri->indexes[1]];
				dv[2] = &surf->verts[tri->indexes[2]];

				R_CalcNormalForTriangle(faceNormal, dv[0]->position, dv[1]->position, dv[2]->position);

				// calculate barycentric basis for the triangle
				bb = (dv[1]->texCoords[0] - dv[0]->texCoords[0]) * (dv[2]->texCoords[1] - dv[0]->texCoords[1]) - (dv[2]->texCoords[0] - dv[0]->texCoords[0]) * (dv[1]->texCoords[1] -
																													  dv[0]->texCoords[1]);
				if(fabs(bb) < 0.00000001f)
					continue;

				// do each vertex
				for(k = 0; k < 3; k++)
				{
					// calculate s tangent vector
					s = dv[k]->texCoords[0] + 10.0f;
					t = dv[k]->texCoords[1];
					bary[0] = ((dv[1]->texCoords[0] - s) * (dv[2]->texCoords[1] - t) - (dv[2]->texCoords[0] - s) * (dv[1]->texCoords[1] - t)) / bb;
					bary[1] = ((dv[2]->texCoords[0] - s) * (dv[0]->texCoords[1] - t) - (dv[0]->texCoords[0] - s) * (dv[2]->texCoords[1] - t)) / bb;
					bary[2] = ((dv[0]->texCoords[0] - s) * (dv[1]->texCoords[1] - t) - (dv[1]->texCoords[0] - s) * (dv[0]->texCoords[1] - t)) / bb;

					dv[k]->tangent[0] = bary[0] * dv[0]->position[0] + bary[1] * dv[1]->position[0] + bary[2] * dv[2]->position[0];
					dv[k]->tangent[1] = bary[0] * dv[0]->position[1] + bary[1] * dv[1]->position[1] + bary[2] * dv[2]->position[1];
					dv[k]->tangent[2] = bary[0] * dv[0]->position[2] + bary[1] * dv[1]->position[2] + bary[2] * dv[2]->position[2];

					VectorSubtract(dv[k]->tangent, dv[k]->position, dv[k]->tangent);
					VectorNormalize(dv[k]->tangent);

					// calculate t tangent vector (binormal)
					s = dv[k]->texCoords[0];
					t = dv[k]->texCoords[1] + 10.0f;
					bary[0] = ((dv[1]->texCoords[0] - s) * (dv[2]->texCoords[1] - t) - (dv[2]->texCoords[0] - s) * (dv[1]->texCoords[1] - t)) / bb;
					bary[1] = ((dv[2]->texCoords[0] - s) * (dv[0]->texCoords[1] - t) - (dv[0]->texCoords[0] - s) * (dv[2]->texCoords[1] - t)) / bb;
					bary[2] = ((dv[0]->texCoords[0] - s) * (dv[1]->texCoords[1] - t) - (dv[1]->texCoords[0] - s) * (dv[0]->texCoords[1] - t)) / bb;

					dv[k]->binormal[0] = bary[0] * dv[0]->position[0] + bary[1] * dv[1]->position[0] + bary[2] * dv[2]->position[0];
					dv[k]->binormal[1] = bary[0] * dv[0]->position[1] + bary[1] * dv[1]->position[1] + bary[2] * dv[2]->position[1];
					dv[k]->binormal[2] = bary[0] * dv[0]->position[2] + bary[1] * dv[1]->position[2] + bary[2] * dv[2]->position[2];

					VectorSubtract(dv[k]->binormal, dv[k]->position, dv[k]->binormal);
					VectorNormalize(dv[k]->binormal);

					// calculate the normal as cross product N=TxB
#if 0
					CrossProduct(dv[k]->tangent, dv[k]->binormal, dv[k]->normal);
					VectorNormalize(dv[k]->normal);

					// Gram-Schmidt orthogonalization process for B
					// compute the cross product B=NxT to obtain
					// an orthogonal basis
					CrossProduct(dv[k]->normal, dv[k]->tangent, dv[k]->binormal);

					if(DotProduct(dv[k]->normal, faceNormal) < 0)
					{
						VectorInverse(dv[k]->normal);
						//VectorInverse(dv[k]->tangent);
						//VectorInverse(dv[k]->binormal);
					}
#else
					//VectorAdd(dv[k]->normal, faceNormal, dv[k]->normal);
#endif
				}
			}

#if 1
			for(j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
			{
				//VectorNormalize(v->tangent);
				//VectorNormalize(v->binormal);
				//VectorNormalize(v->normal);
			}
#endif
		}
#endif

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

	// split the surfaces into VBO surfaces by the maximum number of GPU vertex skinning bones
	{
		int				numRemaining;
		growList_t		sortedTriangles;
		growList_t      vboTriangles;
		growList_t      vboSurfaces;

		int				numBoneReferences;
		int				boneReferences[MAX_BONES];

		Com_InitGrowList(&vboSurfaces, 10);

		for(i = 0, surf = mdmModel->surfaces; i < mdmModel->numSurfaces; i++, surf++)
		{
			// sort triangles
			Com_InitGrowList(&sortedTriangles, 1000);

			for(j = 0, tri = surf->triangles; j < surf->numTriangles; j++, tri++)
			{
				skelTriangle_t *sortTri = Com_Allocate(sizeof(*sortTri));

				for(k = 0; k < 3; k++)
				{
					sortTri->indexes[k] = tri->indexes[k];
					sortTri->vertexes[k] = &surf->verts[tri->indexes[k]];
				}
				sortTri->referenced = qfalse;

				Com_AddToGrowList(&sortedTriangles, sortTri);
			}

			//qsort(sortedTriangles.elements, sortedTriangles.currentElements, sizeof(void *), CompareTrianglesByBoneReferences);

	#if 0
			for(j = 0; j < sortedTriangles.currentElements; j++)
			{
				int		b[MAX_WEIGHTS * 3];

				skelTriangle_t *sortTri = Com_GrowListElement(&sortedTriangles, j);

				for(k = 0; k < 3; k++)
				{
					v = sortTri->vertexes[k];

					for(l = 0; l < MAX_WEIGHTS; l++)
					{
						b[k * 3 + l] = (l < v->numWeights) ? v->weights[l]->boneIndex : 9999;
					}

					qsort(b, MAX_WEIGHTS * 3, sizeof(int), CompareBoneIndices);
					//ri.Printf(PRINT_ALL, "bone indices: %i %i %i %i\n", b[k * 3 + 0], b[k * 3 + 1], b[k * 3 + 2], b[k * 3 + 3]);
				}
			}
	#endif

			numRemaining = sortedTriangles.currentElements;
			while(numRemaining)
			{
				numBoneReferences = 0;
				Com_Memset(boneReferences, 0, sizeof(boneReferences));

				Com_InitGrowList(&vboTriangles, 1000);

				for(j = 0; j < sortedTriangles.currentElements; j++)
				{
					skelTriangle_t *sortTri = Com_GrowListElement(&sortedTriangles, j);

					if(sortTri->referenced)
						continue;

					if(AddTriangleToVBOTriangleList(&vboTriangles, sortTri, &numBoneReferences, boneReferences))
					{
						sortTri->referenced = qtrue;
					}
				}

				if(!vboTriangles.currentElements)
				{
					ri.Printf(PRINT_WARNING, "R_LoadMDM: could not add triangles to a remaining VBO surface for model '%s'\n", modName);
					break;
				}

				AddSurfaceToVBOSurfacesListMDM(&vboSurfaces, &vboTriangles, mdmModel, surf, i, numBoneReferences, boneReferences);
				numRemaining -= vboTriangles.currentElements;

				Com_DestroyGrowList(&vboTriangles);
			}

			for(j = 0; j < sortedTriangles.currentElements; j++)
			{
				skelTriangle_t *sortTri = Com_GrowListElement(&sortedTriangles, j);

				Com_Dealloc(sortTri);
			}
			Com_DestroyGrowList(&sortedTriangles);
		}

		// move VBO surfaces list to hunk
		mdmModel->numVBOSurfaces = vboSurfaces.currentElements;
		mdmModel->vboSurfaces = ri.Hunk_Alloc(mdmModel->numVBOSurfaces * sizeof(*mdmModel->vboSurfaces), h_low);

		for(i = 0; i < mdmModel->numVBOSurfaces; i++)
		{
			mdmModel->vboSurfaces[i] = (srfVBOMDMMesh_t *) Com_GrowListElement(&vboSurfaces, i);
		}

		Com_DestroyGrowList(&vboSurfaces);
	}

	return qtrue;
}
