/* -------------------------------------------------------------------------------

Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

----------------------------------------------------------------------------------

This code has been altered significantly from its original form, to support
several games based on the Quake III Arena engine, in the form of "Q3Map2."

------------------------------------------------------------------------------- */



/* marker */
#define MODEL_C



/* dependencies */
#include "q3map2.h"



/*
PicoPrintFunc()
callback for picomodel.lib
*/

void PicoPrintFunc(int level, const char *str)
{
	if(str == NULL)
		return;
	switch (level)
	{
		case PICO_NORMAL:
			Sys_Printf("%s\n", str);
			break;

		case PICO_VERBOSE:
			Sys_FPrintf(SYS_VRB, "%s\n", str);
			break;

		case PICO_WARNING:
			Sys_Printf("WARNING: %s\n", str);
			break;

		case PICO_ERROR:
			Sys_Printf("ERROR: %s\n", str);
			break;

		case PICO_FATAL:
			Error("ERROR: %s", str);
	}
}



/*
PicoLoadFileFunc()
callback for picomodel.lib
*/

void PicoLoadFileFunc(char *name, byte ** buffer, int *bufSize)
{
	*bufSize = vfsLoadFile((const char *)name, (void **)buffer, 0);
}



/*
FindModel() - ydnar
finds an existing picoModel and returns a pointer to the picoModel_t struct or NULL if not found
*/

picoModel_t    *FindModel(char *name, int frame)
{
	int             i;


	/* init */
	if(numPicoModels <= 0)
		memset(picoModels, 0, sizeof(picoModels));

	/* dummy check */
	if(name == NULL || name[0] == '\0')
		return NULL;

	/* search list */
	for(i = 0; i < MAX_MODELS; i++)
	{
		if(picoModels[i] != NULL &&
		   !strcmp(PicoGetModelName(picoModels[i]), name) && PicoGetModelFrameNum(picoModels[i]) == frame)
			return picoModels[i];
	}

	/* no matching picoModel found */
	return NULL;
}



/*
LoadModel() - ydnar
loads a picoModel and returns a pointer to the picoModel_t struct or NULL if not found
*/

picoModel_t    *LoadModel(char *name, int frame)
{
	int             i;
	picoModel_t    *model, **pm;


	/* init */
	if(numPicoModels <= 0)
		memset(picoModels, 0, sizeof(picoModels));

	/* dummy check */
	if(name == NULL || name[0] == '\0')
		return NULL;

	/* try to find existing picoModel */
	model = FindModel(name, frame);
	if(model != NULL)
		return model;

	/* none found, so find first non-null picoModel */
	pm = NULL;
	for(i = 0; i < MAX_MODELS; i++)
	{
		if(picoModels[i] == NULL)
		{
			pm = &picoModels[i];
			break;
		}
	}

	/* too many picoModels? */
	if(pm == NULL)
		Error("MAX_MODELS (%d) exceeded, there are too many model files referenced by the map.", MAX_MODELS);

	/* attempt to parse model */
	*pm = PicoLoadModel((char *)name, frame);

	/* if loading failed, make a bogus model to silence the rest of the warnings */
	if(*pm == NULL)
	{
		/* allocate a new model */
		*pm = PicoNewModel();
		if(*pm == NULL)
			return NULL;

		/* set data */
		PicoSetModelName(*pm, name);
		PicoSetModelFrameNum(*pm, frame);
	}

	/* debug code */
#if 0
	{
		int             numSurfaces, numVertexes;
		picoSurface_t  *ps;


		Sys_Printf("Model %s\n", name);
		numSurfaces = PicoGetModelNumSurfaces(*pm);
		for(i = 0; i < numSurfaces; i++)
		{
			ps = PicoGetModelSurface(*pm, i);
			numVertexes = PicoGetSurfaceNumVertexes(ps);
			Sys_Printf("Surface %d has %d vertexes\n", i, numVertexes);
		}
	}
#endif

	/* set count */
	if(*pm != NULL)
		numPicoModels++;

	/* return the picoModel */
	return *pm;
}



/*
InsertModel() - ydnar
adds a picomodel into the bsp
*/

void InsertModel(char *name, int skin, int frame, matrix_t transform, matrix_t nTransform, remap_t * remap, shaderInfo_t * celShader, int eNum, int castShadows,
				 int recvShadows, int spawnFlags, float lightmapScale, int lightmapSampleSize, float shadeAngle)
{
	int             i, j, k, s, numSurfaces;
	matrix_t        identity;
	picoModel_t    *model;
	picoShader_t   *shader;
	picoSurface_t  *surface;
	shaderInfo_t   *si;
	mapDrawSurface_t *ds;
	bspDrawVert_t  *dv;
	char           *picoShaderName;
	char            shaderName[MAX_QPATH];
	picoVec_t      *xyz, *normal, *st;
	byte           *color;
	picoIndex_t    *indexes;
	remap_t        *rm, *glob;
	skinfile_t     *sf, *sf2;
	double          normalEpsilon_save;
	double          distanceEpsilon_save;
	char            skinfilename[MAX_QPATH];
	char           *skinfilecontent;
	int             skinfilesize;
	char           *skinfileptr, *skinfilenextptr;


	/* get model */
	model = LoadModel(name, frame);
	if(model == NULL)
		return;

	/* load skin file */
	Com_sprintf(skinfilename, sizeof(skinfilename), "%s_%d.skin", name, skin);
	skinfilename[sizeof(skinfilename) - 1] = 0;
	skinfilesize = vfsLoadFile(skinfilename, (void **)&skinfilecontent, 0);
	if(skinfilesize < 0 && skin != 0)
	{
		/* fallback to skin 0 if invalid */
		Com_sprintf(skinfilename, sizeof(skinfilename), "%s_0.skin", name);
		skinfilename[sizeof(skinfilename) - 1] = 0;
		skinfilesize = vfsLoadFile(skinfilename, (void **)&skinfilecontent, 0);
		if(skinfilesize >= 0)
			Sys_Printf("Skin %d of %s does not exist, using 0 instead\n", skin, name);
	}
	sf = NULL;
	if(skinfilesize >= 0)
	{
		int             pos;
		Sys_Printf("Using skin %d of %s\n", skin, name);

		for(skinfileptr = skinfilecontent; *skinfileptr; skinfileptr = skinfilenextptr)
		{
			// for fscanf
			char            format[64];

			skinfilenextptr = strchr(skinfileptr, '\r');
			if(skinfilenextptr)
			{
				*skinfilenextptr++ = 0;
			}
			else
			{
				skinfilenextptr = strchr(skinfileptr, '\n');
				if(skinfilenextptr)
					*skinfilenextptr++ = 0;
				else
					skinfilenextptr = skinfileptr + strlen(skinfileptr);
			}

			/* create new item */
			sf2 = sf;
			sf = safe_malloc(sizeof(*sf));
			sf->next = sf2;

			Com_sprintf(format, sizeof(format), "replace %%%ds %%%ds", (int)sizeof(sf->name) - 1, (int)sizeof(sf->to) - 1);
			if(sscanf(skinfileptr, format, sf->name, sf->to) == 2)
				continue;
			Com_sprintf(format, sizeof(format), " %%%d[^, 	] ,%%%ds", (int)sizeof(sf->name) - 1, (int)sizeof(sf->to) - 1);
			if((pos = sscanf(skinfileptr, format, sf->name, sf->to)) == 2)
				continue;

			/* invalid input line -> discard sf struct */
			Sys_Printf("Discarding skin directive in %s: %s\n", skinfilename, skinfileptr);
			free(sf);
			sf = sf2;
		}
		free(skinfilecontent);
	}

	/* handle null matrix */
	if(transform == NULL)
	{
		MatrixIdentity(identity);
		transform = identity;
	}

	/* create transform matrix for normals */
#if 0
	MatrixCopy(transform, nTransform);
	if(MatrixInverse(nTransform))
	{
		Sys_FPrintf(SYS_VRB, "WARNING: Can't invert model transform matrix, using transpose instead\n");
		MatrixTranspose(transform, nTransform);
	}
#endif

	/* fix bogus lightmap scale */
	if(lightmapScale <= 0.0f)
		lightmapScale = 1.0f;

	/* fix bogus shade angle */
	if(shadeAngle <= 0.0f)
		shadeAngle = 0.0f;

	/* each surface on the model will become a new map drawsurface */
	numSurfaces = PicoGetModelNumSurfaces(model);
	//% Sys_FPrintf( SYS_VRB, "Model %s has %d surfaces\n", name, numSurfaces );
	for(s = 0; s < numSurfaces; s++)
	{
		/* get surface */
		surface = PicoGetModelSurface(model, s);
		if(surface == NULL)
			continue;

		/* only handle triangle surfaces initially (fixme: support patches) */
		if(PicoGetSurfaceType(surface) != PICO_TRIANGLES)
			continue;

		/* fix the surface's normals */
		PicoFixSurfaceNormals(surface);

		/* get shader name */
		shader = PicoGetSurfaceShader(surface);
		if(shader == NULL)
			picoShaderName = "";
		else
			picoShaderName = PicoGetShaderName(shader);

		/* handle .skin file */
		if(sf)
		{
			picoShaderName = NULL;
			for(sf2 = sf; sf2 != NULL; sf2 = sf2->next)
			{
				if(!Q_stricmp(surface->name, sf2->name))
				{
					Sys_FPrintf(SYS_VRB, "Skin file: mapping %s to %s\n", surface->name, sf2->to);
					picoShaderName = sf2->to;
					break;
				}
			}
			if(!picoShaderName)
			{
				Sys_FPrintf(SYS_VRB, "Skin file: not mapping %s\n", surface->name);
				continue;
			}
		}

		/* handle shader remapping */
		glob = NULL;
		for(rm = remap; rm != NULL; rm = rm->next)
		{
			if(rm->from[0] == '*' && rm->from[1] == '\0')
				glob = rm;
			else if(!Q_stricmp(picoShaderName, rm->from))
			{
				Sys_FPrintf(SYS_VRB, "Remapping %s to %s\n", picoShaderName, rm->to);
				picoShaderName = rm->to;
				glob = NULL;
				break;
			}
		}

		if(glob != NULL)
		{
			Sys_FPrintf(SYS_VRB, "Globbing %s to %s\n", picoShaderName, glob->to);
			picoShaderName = glob->to;
		}

		/* shader renaming for sof2 */
		if(renameModelShaders)
		{
			strcpy(shaderName, picoShaderName);
			StripExtension(shaderName);
			if(spawnFlags & 1)
				strcat(shaderName, "_RMG_BSP");
			else
				strcat(shaderName, "_BSP");
			si = ShaderInfoForShader(shaderName);
		}
		else
		{
			si = ShaderInfoForShader(picoShaderName);

			// Tr3B: HACK to support the messy Doom 3 materials provided by .ASE files
			if(!si->explicitDef)
			{
				picoShaderName = PicoGetShaderMapName(shader);

				Q_strncpyz(shaderName, picoShaderName, sizeof(shaderName));
				StripExtension(shaderName);

				i = 0;
				while(shaderName[i])
				{
					if(shaderName[i] == '\\')
						shaderName[i] = '/';
					i++;
				}

				if(strstr(shaderName, "base/"))
				{
					si = ShaderInfoForShader(strstr(shaderName, "base/") + strlen("base/"));
					Sys_FPrintf(SYS_WRN, "WARNING: Applied .ASE material loader HACK to '%s' -> '%s'\n", picoShaderName, si->shader);
				}

			}
		}

		/* allocate a surface (ydnar: gs mods) */
		ds = AllocDrawSurface(SURFACE_TRIANGLES);
		ds->entityNum = eNum;
		ds->castShadows = castShadows;
		ds->recvShadows = recvShadows;

		/* set shader */
		ds->shaderInfo = si;

		/* force to meta? */
		if((si != NULL && si->forceMeta) || (spawnFlags & 4))	/* 3rd bit */
			ds->type = SURFACE_FORCED_META;

		/* fix the surface's normals (jal: conditioned by shader info) */
		//if(!(spawnFlags & 64) && (shadeAngle == 0.0f || ds->type != SURFACE_FORCED_META))
		//	PicoFixSurfaceNormals(surface);

		/* set sample size */
		if(lightmapSampleSize > 0.0f)
			ds->sampleSize = lightmapSampleSize;

		/* set lightmap scale */
		if(lightmapScale > 0.0f)
			ds->lightmapScale = lightmapScale;

		/* set shading angle */
		if(shadeAngle > 0.0f)
			ds->shadeAngleDegrees = shadeAngle;

		/* set particulars */
		ds->numVerts = PicoGetSurfaceNumVertexes(surface);
		ds->verts = safe_malloc(ds->numVerts * sizeof(ds->verts[0]));
		memset(ds->verts, 0, ds->numVerts * sizeof(ds->verts[0]));

		ds->numIndexes = PicoGetSurfaceNumIndexes(surface);
		ds->indexes = safe_malloc(ds->numIndexes * sizeof(ds->indexes[0]));
		memset(ds->indexes, 0, ds->numIndexes * sizeof(ds->indexes[0]));

		/* copy vertexes */
		for(i = 0; i < ds->numVerts; i++)
		{
			/* get vertex */
			dv = &ds->verts[i];

			/* xyz and normal */
			xyz = PicoGetSurfaceXYZ(surface, i);
			VectorCopy(xyz, dv->xyz);
			MatrixTransformPoint2(transform, dv->xyz);

			normal = PicoGetSurfaceNormal(surface, i);
			VectorCopy(normal, dv->normal);
			MatrixTransformNormal2(nTransform, dv->normal);
			VectorNormalize2(dv->normal, dv->normal);

			/* ydnar: tek-fu celshading support for flat shaded shit */
			if(flat)
			{
				dv->st[0] = si->stFlat[0];
				dv->st[1] = si->stFlat[1];
			}

			/* ydnar: gs mods: added support for explicit shader texcoord generation */
			else if(si->tcGen)
			{
				/* project the texture */
				dv->st[0] = DotProduct(si->vecs[0], dv->xyz);
				dv->st[1] = DotProduct(si->vecs[1], dv->xyz);
			}

			/* direct texture coordinates */
			else
			{
				st = PicoGetSurfaceST(surface, 0, i);
				dv->st[0] = st[0];
				dv->st[1] = st[1];
			}

			/* set lightmap/color bits */
			color = PicoGetSurfaceColor(surface, 0, i);

			dv->paintColor[0] = color[0] / 255.0f;
			dv->paintColor[1] = color[1] / 255.0f;
			dv->paintColor[2] = color[2] / 255.0f;
			dv->paintColor[3] = color[3] / 255.0f;

			for(j = 0; j < MAX_LIGHTMAPS; j++)
			{
				dv->lightmap[j][0] = 0.0f;
				dv->lightmap[j][1] = 0.0f;

				dv->lightColor[j][0] = 255;
				dv->lightColor[j][1] = 255;
				dv->lightColor[j][2] = 255;
				dv->lightColor[j][3] = 255;
			}
		}

		/* copy indexes */
		indexes = PicoGetSurfaceIndexes(surface, 0);
		for(i = 0; i < ds->numIndexes; i++)
			ds->indexes[i] = indexes[i];

		/* set cel shader */
		ds->celShader = celShader;

		/* ydnar: giant hack land: generate clipping brushes for model triangles */
		if(si->clipModel || (spawnFlags & 2))	/* 2nd bit */
		{
			vec3_t          points[4], backs[3];
			vec4_t          plane, reverse, pa, pb, pc;


			/* temp hack */
			if(!si->clipModel &&
			   (((si->compileFlags & C_TRANSLUCENT) && !(si->compileFlags & C_COLLISION)) || !(si->compileFlags & C_SOLID)))
				continue;

			/* walk triangle list */
			for(i = 0; i < ds->numIndexes; i += 3)
			{
				/* overflow hack */
				AUTOEXPAND_BY_REALLOC(mapplanes, (nummapplanes + 64) << 1, allocatedmapplanes, 1024);

				/* make points and back points */
				for(j = 0; j < 3; j++)
				{
					/* get vertex */
					dv = &ds->verts[ds->indexes[i + j]];

					/* copy xyz */
					VectorCopy(dv->xyz, points[j]);
					VectorCopy(dv->xyz, backs[j]);

					/* find nearest axial to normal and push back points opposite */
					/* note: this doesn't work as well as simply using the plane of the triangle, below */
					for(k = 0; k < 3; k++)
					{
						if(fabs(dv->normal[k]) >= fabs(dv->normal[(k + 1) % 3]) &&
						   fabs(dv->normal[k]) >= fabs(dv->normal[(k + 2) % 3]))
						{
							backs[j][k] += dv->normal[k] < 0.0f ? 64.0f : -64.0f;
							break;
						}
					}
				}

				VectorCopy(points[0], points[3]);	// for cyclic usage

				/* make plane for triangle */
				// div0: add some extra spawnflags:
				//   0: snap normals to axial planes for extrusion
				//   8: extrude with the original normals
				//  16: extrude only with up/down normals (ideal for terrain)
				//  24: extrude by distance zero (may need engine changes)
				if(PlaneFromPoints(plane, points[0], points[1], points[2], qtrue))
				{
					vec3_t          bestNormal;
					float           backPlaneDistance = 2;

					if(spawnFlags & 8)	// use a DOWN normal
					{
						if(spawnFlags & 16)
						{
							// 24: normal as is, and zero width (broken)
							VectorCopy(plane, bestNormal);
						}
						else
						{
							// 8: normal as is
							VectorCopy(plane, bestNormal);
						}
					}
					else
					{
						if(spawnFlags & 16)
						{
							// 16: UP/DOWN normal
							VectorSet(bestNormal, 0, 0, (plane[2] >= 0 ? 1 : -1));
						}
						else
						{
							// 0: axial normal
							if(fabs(plane[0]) > fabs(plane[1]))	// x>y
								if(fabs(plane[1]) > fabs(plane[2]))	// x>y, y>z
									VectorSet(bestNormal, (plane[0] >= 0 ? 1 : -1), 0, 0);
								else	// x>y, z>=y
								if(fabs(plane[0]) > fabs(plane[2]))	// x>z, z>=y
									VectorSet(bestNormal, (plane[0] >= 0 ? 1 : -1), 0, 0);
								else	// z>=x, x>y
									VectorSet(bestNormal, 0, 0, (plane[2] >= 0 ? 1 : -1));
							else	// y>=x
							if(fabs(plane[1]) > fabs(plane[2]))	// y>z, y>=x
								VectorSet(bestNormal, 0, (plane[1] >= 0 ? 1 : -1), 0);
							else	// z>=y, y>=x
								VectorSet(bestNormal, 0, 0, (plane[2] >= 0 ? 1 : -1));
						}
					}

					/* build a brush */
					buildBrush = AllocBrush(48);
					buildBrush->entityNum = mapEntityNum;
					buildBrush->original = buildBrush;
					buildBrush->contentShader = si;
					buildBrush->compileFlags = si->compileFlags;
					buildBrush->contentFlags = si->contentFlags;

					buildBrush->generatedClipBrush = qtrue;

					normalEpsilon_save = normalEpsilon;
					distanceEpsilon_save = distanceEpsilon;

					if(si->compileFlags & C_STRUCTURAL)	// allow forced structural brushes here
					{
						buildBrush->detail = qfalse;

						// only allow EXACT matches when snapping for these (this is mostly for caulk brushes inside a model)
						if(normalEpsilon > 0)
							normalEpsilon = 0;
						if(distanceEpsilon > 0)
							distanceEpsilon = 0;
					}
					else
						buildBrush->detail = qtrue;

					/* regenerate back points */
					for(j = 0; j < 3; j++)
					{
						/* get vertex */
						dv = &ds->verts[ds->indexes[i + j]];

						// shift by some units
						VectorMA(dv->xyz, -64.0f, bestNormal, backs[j]);	// 64 prevents roundoff errors a bit
					}

					/* make back plane */
					VectorScale(plane, -1.0f, reverse);
					reverse[3] = -plane[3];
					if((spawnFlags & 24) != 24)
						reverse[3] += DotProduct(bestNormal, plane) * backPlaneDistance;
					// that's at least sqrt(1/3) backPlaneDistance, unless in DOWN mode; in DOWN mode, we are screwed anyway if we encounter a plane that's perpendicular to the xy plane)

					if(PlaneFromPoints(pa, points[2], points[1], backs[1], qtrue) &&
					   PlaneFromPoints(pb, points[1], points[0], backs[0], qtrue) && PlaneFromPoints(pc, points[0], points[2], backs[2], qtrue))
					{
						/* set up brush sides */
						buildBrush->numsides = 5;
						buildBrush->sides[0].shaderInfo = si;
						for(j = 1; j < buildBrush->numsides; j++)
							buildBrush->sides[j].shaderInfo = NULL;	// don't emit these faces as draw surfaces, should make smaller BSPs; hope this works

						buildBrush->sides[0].planenum = FindFloatPlane(plane, plane[3], 3, points);
						buildBrush->sides[1].planenum = FindFloatPlane(pa, pa[3], 2, &points[1]);	// pa contains points[1] and points[2]
						buildBrush->sides[2].planenum = FindFloatPlane(pb, pb[3], 2, &points[0]);	// pb contains points[0] and points[1]
						buildBrush->sides[3].planenum = FindFloatPlane(pc, pc[3], 2, &points[2]);	// pc contains points[2] and points[0] (copied to points[3]
						buildBrush->sides[4].planenum = FindFloatPlane(reverse, reverse[3], 3, backs);
					}
					else
					{
						free(buildBrush);
						continue;
					}

					normalEpsilon = normalEpsilon_save;
					distanceEpsilon = distanceEpsilon_save;

					/* add to entity */
					if(CreateBrushWindings(buildBrush))
					{
						AddBrushBevels();
						//% EmitBrushes( buildBrush, NULL, NULL );
						buildBrush->next = entities[mapEntityNum].brushes;
						entities[mapEntityNum].brushes = buildBrush;
						entities[mapEntityNum].numBrushes++;
					}
					else
						free(buildBrush);
				}
			}
		}
	}
}


/*
AddTriangleModel()
*/

void AddTriangleModel(entity_t * e)
{
	int             frame, skin, castShadows, recvShadows, spawnFlags;
	const char     *name, *model, *value;
	char            shader[MAX_QPATH];
	shaderInfo_t   *celShader;
	float           temp, lightmapScale;
	float           shadeAngle;
	int             lightmapSampleSize;
	vec3_t          scale;
	matrix_t        rotation, transform;
	epair_t        *ep;
	remap_t        *remap, *remap2;
	char           *split;

	/* note it */
	Sys_FPrintf(SYS_VRB, "--- AddTriangleModel ---\n");

	/* get current brush entity name */
	name = ValueForKey(e, "name");

	/* misc_model entities target non-worldspawn brush model entities */
	if(name[0] == '\0')
		return;

	/* get model name */
	model = ValueForKey(e, "model");
	if(model[0] == '\0')
		return;

	/* Tr3B: skip triggers and other entities */
	if(!Q_stricmp(name, model))
		return;

	/* get model frame */
	frame = IntForKey(e, "_frame");

	/* worldspawn (and func_groups) default to cast/recv shadows in worldspawn group */
	if(e == entities)
	{
		castShadows = WORLDSPAWN_CAST_SHADOWS;
		recvShadows = WORLDSPAWN_RECV_SHADOWS;
	}

	/* other entities don't cast any shadows, but recv worldspawn shadows */
	else
	{
		castShadows = ENTITY_CAST_SHADOWS;
		recvShadows = ENTITY_RECV_SHADOWS;
	}

	/* get explicit shadow flags */
	GetEntityShadowFlags(NULL, e, &castShadows, &recvShadows);

	/* get spawnflags */
	spawnFlags = IntForKey(e, "spawnflags");

	/* Tr3B: added clipModel option */
	spawnFlags |= (IntForKey(e, "clipModel") > 0) ? 2 : 0;

	/* Tr3B: added forceMeta option */
	spawnFlags |= (IntForKey(e, "forceMeta") > 0) ? 4 : 0;

	/* get scale */
	scale[0] = scale[1] = scale[2] = 1.0f;
	temp = FloatForKey(e, "modelscale");
	if(temp != 0.0f)
		scale[0] = scale[1] = scale[2] = temp;
	value = ValueForKey(e, "modelscale_vec");
	if(value[0] != '\0')
		sscanf(value, "%f %f %f", &scale[0], &scale[1], &scale[2]);

	/* set transform matrix */
	MatrixIdentity(transform);
	MatrixMultiplyScale(transform, scale[0], scale[1], scale[2]);
	MatrixIdentity(rotation);

	/* get shader remappings */
	remap = NULL;
	for(ep = e->epairs; ep != NULL; ep = ep->next)
	{
		/* look for keys prefixed with "_remap" */
		if(ep->key != NULL && ep->value != NULL &&
		   ep->key[0] != '\0' && ep->value[0] != '\0' && !Q_strncasecmp(ep->key, "_remap", 6))
		{
			/* create new remapping */
			remap2 = remap;
			remap = safe_malloc(sizeof(*remap));
			remap->next = remap2;
			strcpy(remap->from, ep->value);

			/* split the string */
			split = strchr(remap->from, ';');
			if(split == NULL)
			{
				Sys_Printf("WARNING: Shader _remap key found in misc_model without a ; character\n");
				free(remap);
				remap = remap2;
				continue;
			}

			/* store the split */
			*split = '\0';
			strcpy(remap->to, (split + 1));

			/* note it */
			//% Sys_FPrintf( SYS_VRB, "Remapping %s to %s\n", remap->from, remap->to );
		}
	}

	/* ydnar: cel shader support */
	value = ValueForKey(e, "_celshader");
	if(value[0] == '\0')
		value = ValueForKey(&entities[0], "_celshader");
	if(value[0] != '\0')
	{
		sprintf(shader, "textures/%s", value);
		celShader = ShaderInfoForShader(shader);
	}
	else
		celShader = NULL;

	/* get lightmap scale */
	/* jal : entity based _samplesize */
	lightmapSampleSize = 0;
	if(strcmp("", ValueForKey(e, "_lightmapsamplesize")))
		lightmapSampleSize = IntForKey(e, "_lightmapsamplesize");
	else if(strcmp("", ValueForKey(e, "_samplesize")))
		lightmapSampleSize = IntForKey(e, "_samplesize");

	if(lightmapSampleSize < 0)
		lightmapSampleSize = 0;

	if(lightmapSampleSize > 0.0f)
		Sys_Printf(" has lightmap sample size of %.d\n", lightmapSampleSize);

	/* get lightmap scale */
	/* vortex: added _ls key (short name of lightmapscale) */
	lightmapScale = 0.0f;
	if(strcmp("", ValueForKey(e, "lightmapscale")) ||
	   strcmp("", ValueForKey(e, "_lightmapscale")) || strcmp("", ValueForKey(e, "_ls")))
	{
		lightmapScale = FloatForKey(e, "lightmapscale");
		if(lightmapScale <= 0.0f)
			lightmapScale = FloatForKey(e, "_lightmapscale");
		if(lightmapScale <= 0.0f)
			lightmapScale = FloatForKey(e, "_ls");
		if(lightmapScale < 0.0f)
			lightmapScale = 0.0f;
		if(lightmapScale > 0.0f)
			Sys_Printf("misc_model has lightmap scale of %.4f\n", lightmapScale);
	}

	/* jal : entity based _shadeangle */
	shadeAngle = 0.0f;
	if(strcmp("", ValueForKey(e, "_shadeangle")))
		shadeAngle = FloatForKey(e, "_shadeangle");
	/* vortex' aliases */
	else if(strcmp("", ValueForKey(e, "_smoothnormals")))
		shadeAngle = FloatForKey(e, "_smoothnormals");
	else if(strcmp("", ValueForKey(e, "_sn")))
		shadeAngle = FloatForKey(e, "_sn");
	else if(strcmp("", ValueForKey(e, "_smooth")))
		shadeAngle = FloatForKey(e, "_smooth");

	if(shadeAngle < 0.0f)
		shadeAngle = 0.0f;

	if(shadeAngle > 0.0f)
		Sys_Printf("misc_model has shading angle of %.4f\n", shadeAngle);

	skin = 0;
	if(strcmp("", ValueForKey(e, "_skin")))
		skin = IntForKey(e, "_skin");
	else if(strcmp("", ValueForKey(e, "skin")))
		skin = IntForKey(e, "skin");

	/* insert the model */
	InsertModel((char *)model, skin, frame, transform, rotation, remap, celShader, mapEntityNum, castShadows, recvShadows, spawnFlags,
				lightmapScale, lightmapSampleSize, shadeAngle);

	/* free shader remappings */
	while(remap != NULL)
	{
		remap2 = remap->next;
		free(remap);
		remap = remap2;
	}
}


/*
AddTriangleModels()
adds misc_model surfaces to the bsp
*/

void AddTriangleModels(entity_t * e)
{
	int             num, frame, skin, castShadows, recvShadows, spawnFlags;
	entity_t       *e2;
	const char     *targetName;
	const char     *target, *model, *value;
	char            shader[MAX_QPATH];
	shaderInfo_t   *celShader;
	float           temp, baseLightmapScale, lightmapScale;
	float           shadeAngle;
	int             lightmapSampleSize;
	vec3_t          origin, scale, angles;
	matrix_t        rotation, rotationScaled, transform;
	epair_t        *ep;
	remap_t        *remap, *remap2;
	char           *split;


	/* note it */
	Sys_FPrintf(SYS_VRB, "--- AddTriangleModels ---\n");

	/* get current brush entity targetname */
	if(e == entities)
		targetName = "";
	else
	{
		targetName = ValueForKey(e, "name");

		/* misc_model entities target non-worldspawn brush model entities */
		if(targetName[0] == '\0')
			return;
	}

	/* get lightmap scale */
	/* vortex: added _ls key (short name of lightmapscale) */
	baseLightmapScale = 0.0f;
	if(strcmp("", ValueForKey(e, "lightmapscale")) ||
	   strcmp("", ValueForKey(e, "_lightmapscale")) || strcmp("", ValueForKey(e, "_ls")))
	{
		baseLightmapScale = FloatForKey(e, "lightmapscale");
		if(baseLightmapScale <= 0.0f)
			baseLightmapScale = FloatForKey(e, "_lightmapscale");
		if(baseLightmapScale <= 0.0f)
			baseLightmapScale = FloatForKey(e, "_ls");
		if(baseLightmapScale < 0.0f)
			baseLightmapScale = 0.0f;
		if(baseLightmapScale > 0.0f)
			Sys_Printf("World Entity has lightmap scale of %.4f\n", baseLightmapScale);
	}

	/* walk the entity list */
	for(num = 1; num < numEntities; num++)
	{
		/* get e2 */
		e2 = &entities[num];

		/* convert misc_models into raw geometry */
		if(Q_stricmp("misc_model", ValueForKey(e2, "classname")))
			continue;

		/* ydnar: added support for md3 models on non-worldspawn models */
		target = ValueForKey(e2, "target");
		if(strcmp(target, targetName))
			continue;

		/* get model name */
		model = ValueForKey(e2, "model");
		if(model[0] == '\0')
		{
			Sys_Printf("WARNING: misc_model at %i %i %i without a model key\n", (int)origin[0], (int)origin[1], (int)origin[2]);
			continue;
		}

		/* get model frame */
		frame = IntForKey(e2, "_frame");

		/* worldspawn (and func_groups) default to cast/recv shadows in worldspawn group */
		if(e == entities)
		{
			castShadows = WORLDSPAWN_CAST_SHADOWS;
			recvShadows = WORLDSPAWN_RECV_SHADOWS;
		}

		/* other entities don't cast any shadows, but recv worldspawn shadows */
		else
		{
			castShadows = ENTITY_CAST_SHADOWS;
			recvShadows = ENTITY_RECV_SHADOWS;
		}

		/* get explicit shadow flags */
		GetEntityShadowFlags(e2, e, &castShadows, &recvShadows);

		/* get spawnflags */
		spawnFlags = IntForKey(e2, "spawnflags");

		/* Tr3B: added clipModel option */
		spawnFlags |= (IntForKey(e2, "clipModel") > 0) ? 2 : 0;

		/* Tr3B: added forceMeta option */
		spawnFlags |= (IntForKey(e2, "forceMeta") > 0) ? 4 : 0;

		/* get origin */
		GetVectorForKey(e2, "origin", origin);
		VectorSubtract(origin, e->origin, origin);	/* offset by parent */

		/* get "angle" (yaw) or "angles" (pitch yaw roll) */
		MatrixIdentity(rotation);
		angles[0] = angles[1] = angles[2] = 0.0f;

		value = ValueForKey(e2, "angle");
		if(value[0] != '\0')
		{
			angles[1] = atof(value);
			MatrixFromAngles(rotation, angles[PITCH], angles[YAW], angles[ROLL]);
		}

		value = ValueForKey(e2, "angles");
		if(value[0] != '\0')
		{
			sscanf(value, "%f %f %f", &angles[0], &angles[1], &angles[2]);
			MatrixFromAngles(rotation, angles[PITCH], angles[YAW], angles[ROLL]);
		}

		value = ValueForKey(e2, "rotation");
		if(value[0] != '\0')
		{
			sscanf(value, "%f %f %f %f %f %f %f %f %f", &rotation[0], &rotation[1], &rotation[2],
				   &rotation[4], &rotation[5], &rotation[6], &rotation[8], &rotation[9], &rotation[10]);
		}

		/* get scale */
		scale[0] = scale[1] = scale[2] = 1.0f;
		temp = FloatForKey(e2, "modelscale");
		if(temp != 0.0f)
			scale[0] = scale[1] = scale[2] = temp;
		value = ValueForKey(e2, "modelscale_vec");
		if(value[0] != '\0')
			sscanf(value, "%f %f %f", &scale[0], &scale[1], &scale[2]);

		MatrixCopy(rotation, rotationScaled);
		MatrixMultiplyScale(rotationScaled, scale[0], scale[1], scale[2]);

		/* set transform matrix */
		MatrixIdentity(transform);
		MatrixSetupTransformFromRotation(transform, rotationScaled, origin);

		/* get shader remappings */
		remap = NULL;
		for(ep = e2->epairs; ep != NULL; ep = ep->next)
		{
			/* look for keys prefixed with "_remap" */
			if(ep->key != NULL && ep->value != NULL &&
			   ep->key[0] != '\0' && ep->value[0] != '\0' && !Q_strncasecmp(ep->key, "_remap", 6))
			{
				/* create new remapping */
				remap2 = remap;
				remap = safe_malloc(sizeof(*remap));
				remap->next = remap2;
				strcpy(remap->from, ep->value);

				/* split the string */
				split = strchr(remap->from, ';');
				if(split == NULL)
				{
					Sys_Printf("WARNING: Shader _remap key found in misc_model without a ; character\n");
					free(remap);
					remap = remap2;
					continue;
				}

				/* store the split */
				*split = '\0';
				strcpy(remap->to, (split + 1));

				/* note it */
				//% Sys_FPrintf( SYS_VRB, "Remapping %s to %s\n", remap->from, remap->to );
			}
		}

		/* ydnar: cel shader support */
		value = ValueForKey(e2, "_celshader");
		if(value[0] == '\0')
			value = ValueForKey(&entities[0], "_celshader");
		if(value[0] != '\0')
		{
			sprintf(shader, "textures/%s", value);
			celShader = ShaderInfoForShader(shader);
		}
		else
			celShader = NULL;

		/* jal : entity based _samplesize */
		lightmapSampleSize = 0;
		if(strcmp("", ValueForKey(e2, "_lightmapsamplesize")))
			lightmapSampleSize = IntForKey(e2, "_lightmapsamplesize");
		else if(strcmp("", ValueForKey(e2, "_samplesize")))
			lightmapSampleSize = IntForKey(e2, "_samplesize");

		if(lightmapSampleSize < 0)
			lightmapSampleSize = 0;

		if(lightmapSampleSize > 0.0f)
			Sys_Printf("misc_model has lightmap sample size of %.d\n", lightmapSampleSize);

		/* get lightmap scale */
		/* vortex: added _ls key (short name of lightmapscale) */
		lightmapScale = 0.0f;
		if(strcmp("", ValueForKey(e2, "lightmapscale")) ||
		   strcmp("", ValueForKey(e2, "_lightmapscale")) || strcmp("", ValueForKey(e2, "_ls")))
		{
			lightmapScale = FloatForKey(e2, "lightmapscale");
			if(lightmapScale <= 0.0f)
				lightmapScale = FloatForKey(e2, "_lightmapscale");
			if(lightmapScale <= 0.0f)
				lightmapScale = FloatForKey(e2, "_ls");
			if(lightmapScale < 0.0f)
				lightmapScale = 0.0f;
			if(lightmapScale > 0.0f)
				Sys_Printf("misc_model has lightmap scale of %.4f\n", lightmapScale);
		}

		/* jal : entity based _shadeangle */
		shadeAngle = 0.0f;
		if(strcmp("", ValueForKey(e2, "_shadeangle")))
			shadeAngle = FloatForKey(e2, "_shadeangle");
		/* vortex' aliases */
		else if(strcmp("", ValueForKey(mapEnt, "_smoothnormals")))
			shadeAngle = FloatForKey(mapEnt, "_smoothnormals");
		else if(strcmp("", ValueForKey(mapEnt, "_sn")))
			shadeAngle = FloatForKey(mapEnt, "_sn");
		else if(strcmp("", ValueForKey(mapEnt, "_smooth")))
			shadeAngle = FloatForKey(mapEnt, "_smooth");

		if(shadeAngle < 0.0f)
			shadeAngle = 0.0f;

		if(shadeAngle > 0.0f)
			Sys_Printf("misc_model has shading angle of %.4f\n", shadeAngle);

		skin = 0;
		if(strcmp("", ValueForKey(e2, "_skin")))
			skin = IntForKey(e2, "_skin");
		else if(strcmp("", ValueForKey(e2, "skin")))
			skin = IntForKey(e2, "skin");

		/* insert the model */
		InsertModel((char *)model, skin, frame, transform, rotation, remap, celShader, mapEntityNum, castShadows, recvShadows, spawnFlags,
					lightmapScale, lightmapSampleSize, shadeAngle);

		/* free shader remappings */
		while(remap != NULL)
		{
			remap2 = remap->next;
			free(remap);
			remap = remap2;
		}
	}
}
