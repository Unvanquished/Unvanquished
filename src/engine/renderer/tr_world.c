/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the OpenWolf GPL Source Code (OpenWolf Source Code).  

OpenWolf Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenWolf Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the OpenWolf Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the OpenWolf 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

#include "tr_local.h"



/*
=================
R_CullGenericSurface - ydnar
based on R_CullTriSurf()
this culls the basic subset of all triangle/mesh map drawsurfaces
=================
*/

#if 0

#define SPHERE_CULL

#ifdef SPHERE_CULL

/*static qboolean	R_CullGenericSurface( srfGeneric_t *surface )
{
	int     cull;


	// allow disabling of foliage
	if( surface->surfaceType == SF_FOLIAGE && !r_drawfoliage->integer )
		return qtrue;

	// allow disabling of foliage
	if( surface->surfaceType == SF_GRID && r_nocurves->integer )
		return qtrue;

	// try sphere cull
	if( tr.currentEntityNum != ENTITYNUM_WORLD )
		cull = R_CullLocalPointAndRadius( surface->origin, surface->radius );
	else
		cull = R_CullPointAndRadius( surface->origin, surface->radius );
	if( cull == CULL_OUT )
		return qtrue;

	// it's ok
	return qfalse;
}*/

#else

static qboolean R_CullTriSurf(srfTriangles_t * cv)
{
	int             boxCull;

	boxCull = R_CullLocalBox(cv->bounds);

	if(boxCull == CULL_OUT)
	{
		return qtrue;
	}
	return qfalse;
}


/*
=================
R_CullGrid

Returns true if the grid is completely culled away.
Also sets the clipped hint bit in tess
=================
*/
static qboolean R_CullGrid(srfGridMesh_t * cv)
{
	int             boxCull;
	int             sphereCull;

	if(r_nocurves->integer)
	{
		return qtrue;
	}

	if(tr.currentEntityNum != ENTITYNUM_WORLD)
	{
		sphereCull = R_CullLocalPointAndRadius(cv->localOrigin, cv->meshRadius);
	}
	else
	{
		sphereCull = R_CullPointAndRadius(cv->localOrigin, cv->meshRadius);
	}
	boxCull = CULL_OUT;

	// check for trivial reject
	if(sphereCull == CULL_OUT)
	{
		tr.pc.c_sphere_cull_patch_out++;
		return qtrue;
	}
	// check bounding box if necessary
	else if(sphereCull == CULL_CLIP)
	{
		tr.pc.c_sphere_cull_patch_clip++;

		boxCull = R_CullLocalBox(cv->meshBounds);

		if(boxCull == CULL_OUT)
		{
			tr.pc.c_box_cull_patch_out++;
			return qtrue;
		}
		else if(boxCull == CULL_IN)
		{
			tr.pc.c_box_cull_patch_in++;
		}
		else
		{
			tr.pc.c_box_cull_patch_clip++;
		}
	}
	else
	{
		tr.pc.c_sphere_cull_patch_in++;
	}

	return qfalse;
}

#endif

#endif							// 0

/*
================
R_CullSurface

Tries to back face cull surfaces before they are lighted or
added to the sorting list.

This will also allow mirrors on both sides of a model without recursion.
================
*/
static qboolean R_CullSurface(surfaceType_t * surface, shader_t * shader, int *frontFace)
{
	srfGeneric_t   *gen;
	int             cull;
	float           d;


	// force to non-front facing
	*frontFace = 0;

	// allow culling to be disabled
	if(r_nocull->integer)
	{
		return qfalse;
	}

	// ydnar: made surface culling generic, inline with q3map2 surface classification
	switch (*surface)
	{
		case SF_FACE:
		case SF_TRIANGLES:
			break;
		case SF_GRID:
			if(r_nocurves->integer)
			{
				return qtrue;
			}
			break;
		case SF_FOLIAGE:
			if(!r_drawfoliage->value)
			{
				return qtrue;
			}
			break;

		default:
			return qtrue;
	}

	// get generic surface
	gen = (srfGeneric_t *) surface;

	// plane cull
	if(gen->plane.type != PLANE_NON_PLANAR && r_facePlaneCull->integer)
	{
		d = DotProduct(tr.orientation.viewOrigin, gen->plane.normal) - gen->plane.dist;
		if(d > 0.0f)
		{
			*frontFace = 1;
		}

		// don't cull exactly on the plane, because there are levels of rounding
		// through the BSP, ICD, and hardware that may cause pixel gaps if an
		// epsilon isn't allowed here
		if(shader->cullType == CT_FRONT_SIDED)
		{
			if(d < -8.0f)
			{
				tr.pc.c_plane_cull_out++;
				return qtrue;
			}
		}
		else if(shader->cullType == CT_BACK_SIDED)
		{
			if(d > 8.0f)
			{
				tr.pc.c_plane_cull_out++;
				return qtrue;
			}
		}

		tr.pc.c_plane_cull_in++;
	}

	{
		// try sphere cull
		if(tr.currentEntityNum != ENTITYNUM_WORLD)
		{
			cull = R_CullLocalPointAndRadius(gen->origin, gen->radius);
		}
		else
		{
			cull = R_CullPointAndRadius(gen->origin, gen->radius);
		}
		if(cull == CULL_OUT)
		{
			tr.pc.c_sphere_cull_out++;
			return qtrue;
		}

		tr.pc.c_sphere_cull_in++;
	}

	// must be visible
	return qfalse;
}

#if 0
static int R_DlightFace(srfSurfaceFace_t * face, int dlightBits)
{
	float           d;
	int             i;
	dlight_t       *dl;


	// ydnar: quick hack, need to rewrite for generic surfaces
	return dlightBits;

	for(i = 0; i < tr.refdef.num_dlights; i++)
	{
		if(!(dlightBits & (1 << i)))
		{
			continue;
		}
		dl = &tr.refdef.dlights[i];
		d = DotProduct(dl->origin, face->plane.normal) - face->plane.dist;
		if(d < -dl->radius || d > dl->radius)
		{
			// dlight doesn't reach the plane
			dlightBits &= ~(1 << i);
		}
	}

	if(!dlightBits)
	{
		tr.pc.c_dlightSurfacesCulled++;
	}

	face->dlightBits[tr.smpFrame] = dlightBits;
	return dlightBits;
}

/*static int R_DlightGrid( srfGridMesh_t *grid, int dlightBits ) {
	int			i;
	dlight_t	*dl;

	for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
		if ( ! ( dlightBits & ( 1 << i ) ) ) {
			continue;
		}
		dl = &tr.refdef.dlights[i];
		if ( dl->origin[0] - dl->radius > grid->bounds[1][0]
			|| dl->origin[0] + dl->radius < grid->bounds[0][0]
			|| dl->origin[1] - dl->radius > grid->bounds[1][1]
			|| dl->origin[1] + dl->radius < grid->bounds[0][1]
			|| dl->origin[2] - dl->radius > grid->bounds[1][2]
			|| dl->origin[2] + dl->radius < grid->bounds[0][2] ) {
			// dlight doesn't reach the bounds
			dlightBits &= ~( 1 << i );
		}
	}

	if ( !dlightBits ) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	grid->dlightBits[ tr.smpFrame ] = dlightBits;
	return dlightBits;
}*/


// ydnar: fixed this function (can be used for trisurfs and foliage)

static int R_DlightTrisurf(srfTriangles_t * srf, int dlightBits)
#if 0
{
	// FIXME: more dlight culling to trisurfs...
	surf->dlightBits[tr.smpFrame] = dlightBits;
	return dlightBits;
}
#else
{
	int             i;
	dlight_t       *dl;
	vec3_t          origin, delta;
	float           radius2, dist2;


	VectorCopy(srf->origin, origin);
	radius2 = srf->radius * srf->radius;

	for(i = 0; i < tr.refdef.num_dlights; i++)
	{
		if(!(dlightBits & (1 << i)))
		{
			continue;
		}

		dl = &tr.refdef.dlights[i];

		VectorSubtract(dl->origin, srf->origin, delta);
		dist2 = DotProduct(delta, delta) - (dl->radius * dl->radius);
		if(dist2 > radius2)
		{
			dlightBits &= ~(1 << i);
		}
	}

	// Com_Printf( "Surf: 0x%08X dlightBits: 0x%08X\n", srf, dlightBits );

	if(!dlightBits)
	{
		tr.pc.c_dlightSurfacesCulled++;
	}

	srf->dlightBits[tr.smpFrame] = dlightBits;
	return dlightBits;
}
#endif
#endif							// 0

/*
====================
R_DlightSurface

The given surface is going to be drawn, and it touches a leaf
that is touched by one or more dlights, so try to throw out
more dlights if possible.
====================
*/

#if 0

static int R_DlightSurface(msurface_t * surf, int dlightBits)
{
	if(*surf->data == SF_FACE)
	{
		dlightBits = R_DlightFace((srfSurfaceFace_t *) surf->data, dlightBits);
	}
	else if(*surf->data == SF_GRID)
	{
		dlightBits = R_DlightGrid((srfGridMesh_t *) surf->data, dlightBits);
	}
	else if(*surf->data == SF_TRIANGLES || *surf->data == SF_FOLIAGE)
	{							// ydnar
		dlightBits = R_DlightTrisurf((srfTriangles_t *) surf->data, dlightBits);
	}
	else
	{
		dlightBits = 0;
	}

	if(dlightBits)
	{
		tr.pc.c_dlightSurfaces++;
	}

	return dlightBits;
}

#else

// ydnar: made this use generic surface

static int R_DlightSurface(msurface_t * surface, int dlightBits)
{
	int             i;
	vec3_t          origin;
	float           radius;
	srfGeneric_t   *gen;


	// get generic surface
	gen = (srfGeneric_t *) surface->data;

	// ydnar: made surface dlighting generic, inline with q3map2 surface classification
	switch ((surfaceType_t) * surface->data)
	{
		case SF_FACE:
		case SF_TRIANGLES:
		case SF_GRID:
		case SF_FOLIAGE:
			break;

		default:
			gen->dlightBits[tr.smpFrame] = 0;
			return 0;
	}

	// debug code
	//% gen->dlightBits[ tr.smpFrame ] = dlightBits;
	//% return dlightBits;

	// try to cull out dlights
	for(i = 0; i < tr.refdef.num_dlights; i++)
	{
		if(!(dlightBits & (1 << i)))
		{
			continue;
		}

		// junior dlights don't affect world surfaces
		if(tr.refdef.dlights[i].flags & REF_JUNIOR_DLIGHT)
		{
			dlightBits &= ~(1 << i);
			continue;
		}

		// lightning dlights affect all surfaces
		if(tr.refdef.dlights[i].flags & REF_DIRECTED_DLIGHT)
		{
			continue;
		}

		// test surface bounding sphere against dlight bounding sphere
		VectorCopy(tr.refdef.dlights[i].transformed, origin);
		radius = tr.refdef.dlights[i].radius;

		if((gen->origin[0] + gen->radius) < (origin[0] - radius) ||
		   (gen->origin[0] - gen->radius) > (origin[0] + radius) ||
		   (gen->origin[1] + gen->radius) < (origin[1] - radius) ||
		   (gen->origin[1] - gen->radius) > (origin[1] + radius) ||
		   (gen->origin[2] + gen->radius) < (origin[2] - radius) || (gen->origin[2] - gen->radius) > (origin[2] + radius))
		{
			dlightBits &= ~(1 << i);
		}
	}

	// Com_Printf( "Surf: 0x%08X dlightBits: 0x%08X\n", srf, dlightBits );

	// set counters
	if(dlightBits == 0)
	{
		tr.pc.c_dlightSurfacesCulled++;
	}
	else
	{
		tr.pc.c_dlightSurfaces++;
	}

	// set surface dlight bits and return
	gen->dlightBits[tr.smpFrame] = dlightBits;
	return dlightBits;
}

#endif



/*
======================
R_AddWorldSurface
======================
*/
static void R_AddWorldSurface(msurface_t * surf, shader_t * shader, int dlightMap, int decalBits)
{
	int             i, frontFace;


	if(surf->viewCount == tr.viewCount)
	{
		return;					// already in this view

	}
	surf->viewCount = tr.viewCount;
	// FIXME: bmodel fog?

	// try to cull before dlighting or adding
	if(R_CullSurface(surf->data, shader, &frontFace))
	{
		return;
	}

	// check for dlighting
	if(dlightMap)
	{
		dlightMap = R_DlightSurface(surf, dlightMap);
		dlightMap = (dlightMap != 0);
	}

	// add decals
	if(decalBits)
	{
		// ydnar: project any decals
		for(i = 0; i < tr.refdef.numDecalProjectors; i++)
		{
			if(decalBits & (1 << i))
			{
				R_ProjectDecalOntoSurface(&tr.refdef.decalProjectors[i], surf, tr.currentBModel);
			}
		}
	}

	R_AddDrawSurf(surf->data, shader, surf->fogIndex, frontFace, dlightMap);
}

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

//----(SA) added

/*
=================
R_BmodelFogNum

See if a sprite is inside a fog volume
Return positive with /any part/ of the brush falling within a fog volume
=================
*/

// ydnar: the original implementation of this function is a bit flaky...
int R_BmodelFogNum(trRefEntity_t * re, bmodel_t * bmodel)
#if 1
{
	int             i, j;
	fog_t          *fog;

	for(i = 1; i < tr.world->numfogs; i++)
	{
		fog = &tr.world->fogs[i];
		for(j = 0; j < 3; j++)
		{
			if(re->e.origin[j] + bmodel->bounds[0][j] >= fog->bounds[1][j])
			{
				break;
			}
			if(re->e.origin[j] + bmodel->bounds[1][j] <= fog->bounds[0][j])
			{
				break;
			}
		}
		if(j == 3)
		{
			return i;
		}
	}

	return 0;
}

#else
{
	int             i, j;
	fog_t          *fog;

	for(i = 1; i < tr.world->numfogs; i++)
	{
		fog = &tr.world->fogs[i];
		for(j = 0; j < 3; j++)
		{
			if(re->e.origin[j] + bmodel->bounds[0][j] > fog->bounds[1][j])
			{
				break;
			}
			if(re->e.origin[j] + bmodel->bounds[0][j] < fog->bounds[0][j])
			{
				break;
			}
		}
		if(j == 3)
		{
			return i;
		}
		for(j = 0; j < 3; j++)
		{
			if(re->e.origin[j] + bmodel->bounds[1][j] > fog->bounds[1][j])
			{
				break;
			}
			if(bmodel->bounds[1][j] < fog->bounds[0][j])
			{
				break;
			}
		}
		if(j == 3)
		{
			return i;
		}
	}

	return 0;
}

#endif

//----(SA) done



/*
=================
R_AddBrushModelSurfaces
=================
*/
void R_AddBrushModelSurfaces(trRefEntity_t * ent)
{
	int             i, clip, fognum, decalBits;
	vec3_t          mins, maxs;
	model_t        *pModel;
	bmodel_t       *bmodel;
	int             savedNumDecalProjectors, numLocalProjectors;
	decalProjector_t *savedDecalProjectors, localProjectors[MAX_DECAL_PROJECTORS];


	pModel = R_GetModelByHandle(ent->e.hModel);

	bmodel = pModel->model.bmodel;

	clip = R_CullLocalBox(bmodel->bounds);
	if(clip == CULL_OUT)
	{
		return;
	}

	// ydnar: set current brush model to world
	tr.currentBModel = bmodel;

	// ydnar: set model state for decals and dynamic fog
	VectorCopy(ent->e.origin, bmodel->orientation[tr.smpFrame].origin);
	VectorCopy(ent->e.axis[0], bmodel->orientation[tr.smpFrame].axis[0]);
	VectorCopy(ent->e.axis[1], bmodel->orientation[tr.smpFrame].axis[1]);
	VectorCopy(ent->e.axis[2], bmodel->orientation[tr.smpFrame].axis[2]);
	bmodel->visible[tr.smpFrame] = qtrue;
	bmodel->entityNum[tr.smpFrame] = tr.currentEntityNum;

	R_DlightBmodel(bmodel);

	// determine if in fog
	fognum = R_BmodelFogNum(ent, bmodel);

	// ydnar: project any decals
	decalBits = 0;
	numLocalProjectors = 0;
	for(i = 0; i < tr.refdef.numDecalProjectors; i++)
	{
		// early out
		if(tr.refdef.decalProjectors[i].shader == NULL)
		{
			continue;
		}

		// transform entity bbox (fixme: rotated entities have invalid bounding boxes)
		VectorAdd(bmodel->bounds[0], tr.orientation.origin, mins);
		VectorAdd(bmodel->bounds[1], tr.orientation.origin, maxs);

		// set bit
		if(R_TestDecalBoundingBox(&tr.refdef.decalProjectors[i], mins, maxs))
		{
			R_TransformDecalProjector(&tr.refdef.decalProjectors[i], tr.orientation.axis, tr.orientation.origin,
									  &localProjectors[numLocalProjectors]);
			numLocalProjectors++;
			decalBits <<= 1;
			decalBits |= 1;
		}
	}

	// ydnar: save old decal projectors
	savedNumDecalProjectors = tr.refdef.numDecalProjectors;
	savedDecalProjectors = tr.refdef.decalProjectors;

	// set local decal projectors
	tr.refdef.numDecalProjectors = numLocalProjectors;
	tr.refdef.decalProjectors = localProjectors;

	// add model surfaces
	for(i = 0; i < bmodel->numSurfaces; i++)
	{
		(bmodel->firstSurface + i)->fogIndex = fognum;
		// Arnout: custom shader support for brushmodels
		if(ent->e.customShader)
		{
			R_AddWorldSurface(bmodel->firstSurface + i, R_GetShaderByHandle(ent->e.customShader), tr.currentEntity->needDlights,
							  decalBits);
		}
		else
		{
			R_AddWorldSurface(bmodel->firstSurface + i, ((msurface_t *) (bmodel->firstSurface + i))->shader,
							  tr.currentEntity->needDlights, decalBits);
		}
	}

	// ydnar: restore old decal projectors
	tr.refdef.numDecalProjectors = savedNumDecalProjectors;
	tr.refdef.decalProjectors = savedDecalProjectors;

	// ydnar: add decal surfaces
	R_AddDecalSurfaces(bmodel);

	// ydnar: clear current brush model
	tr.currentBModel = NULL;
}


/*
=============================================================

	WORLD MODEL

=============================================================
*/


/*
R_AddLeafSurfaces() - ydnar
adds a leaf's drawsurfaces
*/

static void R_AddLeafSurfaces(mnode_t * node, int dlightBits, int decalBits)
{
	int             c;
	msurface_t     *surf, **mark;


	// add to count
	tr.pc.c_leafs++;

	// add to z buffer bounds
	if(node->mins[0] < tr.viewParms.visBounds[0][0])
	{
		tr.viewParms.visBounds[0][0] = node->mins[0];
	}
	if(node->mins[1] < tr.viewParms.visBounds[0][1])
	{
		tr.viewParms.visBounds[0][1] = node->mins[1];
	}
	if(node->mins[2] < tr.viewParms.visBounds[0][2])
	{
		tr.viewParms.visBounds[0][2] = node->mins[2];
	}

	if(node->maxs[0] > tr.viewParms.visBounds[1][0])
	{
		tr.viewParms.visBounds[1][0] = node->maxs[0];
	}
	if(node->maxs[1] > tr.viewParms.visBounds[1][1])
	{
		tr.viewParms.visBounds[1][1] = node->maxs[1];
	}
	if(node->maxs[2] > tr.viewParms.visBounds[1][2])
	{
		tr.viewParms.visBounds[1][2] = node->maxs[2];
	}

	// add the individual surfaces
	mark = node->firstmarksurface;
	c = node->nummarksurfaces;
	while(c--)
	{
		// the surface may have already been added if it
		// spans multiple leafs
		surf = *mark;
		R_AddWorldSurface(surf, surf->shader, dlightBits, decalBits);
		mark++;
	}
}


/*
================
R_RecursiveWorldNode
================
*/
static void R_RecursiveWorldNode(mnode_t * node, int planeBits, int dlightBits, int decalBits)
{
	int             i, r;
	dlight_t       *dl;


	do
	{
		// if the node wasn't marked as potentially visible, exit
		if(node->visframe != tr.visCount)
		{
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?

		if(!r_nocull->integer)
		{
			if(planeBits & 1)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[0]);
				if(r == 2)
				{
					return;		// culled
				}
				if(r == 1)
				{
					planeBits &= ~1;	// all descendants will also be in front
				}
			}

			if(planeBits & 2)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[1]);
				if(r == 2)
				{
					return;		// culled
				}
				if(r == 1)
				{
					planeBits &= ~2;	// all descendants will also be in front
				}
			}

			if(planeBits & 4)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[2]);
				if(r == 2)
				{
					return;		// culled
				}
				if(r == 1)
				{
					planeBits &= ~4;	// all descendants will also be in front
				}
			}

			if(planeBits & 8)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[3]);
				if(r == 2)
				{
					return;		// culled
				}
				if(r == 1)
				{
					planeBits &= ~8;	// all descendants will also be in front
				}
			}

			// ydnar: farplane culling
			if(planeBits & 16)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[4]);
				if(r == 2)
				{
					return;		// culled
				}
				if(r == 1)
				{
					planeBits &= ~8;	// all descendants will also be in front
				}
			}

		}

		// ydnar: cull dlights
		if(dlightBits)
		{						//%    && node->contents != -1 )
			for(i = 0; i < tr.refdef.num_dlights; i++)
			{
				if(dlightBits & (1 << i))
				{
					// directional dlights don't get culled
					if(tr.refdef.dlights[i].flags & REF_DIRECTED_DLIGHT)
					{
						continue;
					}

					// test dlight bounds against node surface bounds
					dl = &tr.refdef.dlights[i];
					if(node->surfMins[0] >= (dl->origin[0] + dl->radius) || node->surfMaxs[0] <= (dl->origin[0] - dl->radius) ||
					   node->surfMins[1] >= (dl->origin[1] + dl->radius) || node->surfMaxs[1] <= (dl->origin[1] - dl->radius) ||
					   node->surfMins[2] >= (dl->origin[2] + dl->radius) || node->surfMaxs[2] <= (dl->origin[2] - dl->radius))
					{
						dlightBits &= ~(1 << i);
					}
				}
			}
		}

		// ydnar: cull decals
		if(decalBits)
		{
			for(i = 0; i < tr.refdef.numDecalProjectors; i++)
			{
				if(decalBits & (1 << i))
				{
					// test decal bounds against node surface bounds
					if(tr.refdef.decalProjectors[i].shader == NULL ||
					   !R_TestDecalBoundingBox(&tr.refdef.decalProjectors[i], node->surfMins, node->surfMaxs))
					{
						decalBits &= ~(1 << i);
					}
				}
			}
		}

		// handle leaf nodes
		if(node->contents != -1)
		{
			break;
		}

		// recurse down the children, front side first
		R_RecursiveWorldNode(node->children[0], planeBits, dlightBits, decalBits);

		// tail recurse
		node = node->children[1];
	} while(1);

	// short circuit
	if(node->nummarksurfaces == 0)
	{
		return;
	}

	// ydnar: moved off to separate function
	R_AddLeafSurfaces(node, dlightBits, decalBits);
}


/*
===============
R_PointInLeaf
===============
*/
static mnode_t *R_PointInLeaf(const vec3_t p)
{
	mnode_t        *node;
	float           d;
	cplane_t       *plane;

	if(!tr.world)
	{
		ri.Error(ERR_DROP, "R_PointInLeaf: bad model");
	}

	node = tr.world->nodes;
	while(1)
	{
		if(node->contents != -1)
		{
			break;
		}
		plane = node->plane;
		d = DotProduct(p, plane->normal) - plane->dist;
		if(d > 0)
		{
			node = node->children[0];
		}
		else
		{
			node = node->children[1];
		}
	}

	return node;
}

/*
==============
R_ClusterPVS
==============
*/
static const byte *R_ClusterPVS(int cluster)
{
	if(!tr.world || !tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters)
	{
		return tr.world->novis;
	}

	return tr.world->vis + cluster * tr.world->clusterBytes;
}

/*
=================
R_inPVS
=================
*/
qboolean R_inPVS(const vec3_t p1, const vec3_t p2)
{
	mnode_t        *leaf;
	const byte     *vis;

	leaf = R_PointInLeaf(p1);
	vis = R_ClusterPVS(leaf->cluster);
	leaf = R_PointInLeaf(p2);

	if(!(vis[leaf->cluster >> 3] & (1 << (leaf->cluster & 7))))
	{
		return qfalse;
	}
	return qtrue;
}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
static void R_MarkLeaves(void)
{
	const byte     *vis;
	mnode_t        *leaf, *parent;
	int             i;
	int             cluster;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if(r_lockpvs->integer)
	{
		return;
	}

	// current viewcluster
	leaf = R_PointInLeaf(tr.viewParms.pvsOrigin);
	cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	// if r_showcluster was just turned on, remark everything
	if(tr.viewCluster == cluster && !tr.refdef.areamaskModified && !r_showcluster->modified)
	{
		return;
	}

	if(r_showcluster->modified || r_showcluster->integer)
	{
		r_showcluster->modified = qfalse;
		if(r_showcluster->integer)
		{
			ri.Printf(PRINT_ALL, "cluster:%i  area:%i\n", cluster, leaf->area);
		}
	}

	tr.visCount++;
	tr.viewCluster = cluster;

	if(r_novis->integer || tr.viewCluster == -1)
	{
		for(i = 0; i < tr.world->numnodes; i++)
		{
			if(tr.world->nodes[i].contents != CONTENTS_SOLID)
			{
				tr.world->nodes[i].visframe = tr.visCount;
			}
		}
		return;
	}

	vis = R_ClusterPVS(tr.viewCluster);

	for(i = 0, leaf = tr.world->nodes; i < tr.world->numnodes; i++, leaf++)
	{
		cluster = leaf->cluster;
		if(cluster < 0 || cluster >= tr.world->numClusters)
		{
			continue;
		}

		// check general pvs
		if(!(vis[cluster >> 3] & (1 << (cluster & 7))))
		{
			continue;
		}

		// check for door connection
		if((tr.refdef.areamask[leaf->area >> 3] & (1 << (leaf->area & 7))))
		{
			continue;			// not visible
		}

		// ydnar: don't want to walk the entire bsp to add skybox surfaces
		if(tr.refdef.rdflags & RDF_SKYBOXPORTAL)
		{
			// this only happens once, as game/cgame know the origin of the skybox
			// this also means the skybox portal cannot move, as this list is calculated once and never again
			if(tr.world->numSkyNodes < WORLD_MAX_SKY_NODES)
			{
				tr.world->skyNodes[tr.world->numSkyNodes++] = leaf;
			}
			R_AddLeafSurfaces(leaf, 0, 0);
			continue;
		}

		parent = leaf;
		do
		{
			if(parent->visframe == tr.visCount)
			{
				break;
			}
			parent->visframe = tr.visCount;
			parent = parent->parent;
		} while(parent);
	}
}


/*
=============
R_AddWorldSurfaces
=============
*/
void R_AddWorldSurfaces(void)
{
	if(!r_drawworld->integer)
	{
		return;
	}

	if(tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	tr.currentEntityNum = ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	// ydnar: set current brush model to world
	tr.currentBModel = &tr.world->bmodels[0];

	// clear out the visible min/max
	ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);

	// render sky or world?
	if(tr.refdef.rdflags & RDF_SKYBOXPORTAL && tr.world->numSkyNodes > 0)
	{
		int             i;
		mnode_t       **node;

		for(i = 0, node = tr.world->skyNodes; i < tr.world->numSkyNodes; i++, node++)
			R_AddLeafSurfaces(*node, tr.refdef.dlightBits, 0);	// no decals on skybox nodes
	}
	else
	{
		// determine which leaves are in the PVS / areamask
		R_MarkLeaves();

		// perform frustum culling and add all the potentially visible surfaces
		R_RecursiveWorldNode(tr.world->nodes, 255, tr.refdef.dlightBits, tr.refdef.decalBits);

		// ydnar: add decal surfaces
		R_AddDecalSurfaces(tr.world->bmodels);
	}

	// clear brush model
	tr.currentBModel = NULL;
}
