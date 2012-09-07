/* -------------------------------------------------------------------------------

Copyright (C) 1999-2007 id Software, Inc. and contributors.
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
#define MAP_C



/* dependencies */
#include "q3map2.h"



/* FIXME: remove these vars */

/* undefine to make plane finding use linear sort (note: really slow) */
#define	USE_HASHING
#define	PLANE_HASHES	8192

int             planehash[PLANE_HASHES];

int             c_boxbevels;
int             c_edgebevels;
int             c_areaportals;
int             c_detail;
int             c_structural;
int             c_mergedFuncStatics;


/*
PlaneEqual()
ydnar: replaced with variable epsilon for djbob
*/

qboolean PlaneEqual(plane_t * p, vec3_t normal, vec_t dist)
{
	float           ne, de;


	/* get local copies */
	ne = normalEpsilon;
	de = distanceEpsilon;

	/* compare */
	// We check equality of each component since we're using '<', not '<='
	// (the epsilons may be zero).  We want to use '<' intead of '<=' to be
	// consistent with the true meaning of "epsilon", and also because other
	// parts of the code uses this inequality.
	if((p->dist == dist || fabs(p->dist - dist) < de) &&
	   (p->normal[0] == normal[0] || fabs(p->normal[0] - normal[0]) < ne) &&
	   (p->normal[1] == normal[1] || fabs(p->normal[1] - normal[1]) < ne) &&
	   (p->normal[2] == normal[2] || fabs(p->normal[2] - normal[2]) < ne))
		return qtrue;

	/* different */
	return qfalse;
}



/*
AddPlaneToHash()
*/

void AddPlaneToHash(plane_t * p)
{
	int             hash;


	hash = (PLANE_HASHES - 1) & (int)fabs(p->dist);

	p->hash_chain = planehash[hash];
	planehash[hash] = p - mapplanes + 1;
}

/*
================
CreateNewFloatPlane
================
*/
int CreateNewFloatPlane(vec3_t normal, vec_t dist)
{
	plane_t        *p, temp;

	if(VectorLength(normal) < 0.5)
	{
		Sys_Printf("FloatPlane: bad normal\n");
		return -1;
	}

	// create a new plane
	AUTOEXPAND_BY_REALLOC(mapplanes, nummapplanes + 1, allocatedmapplanes, 1024);

	p = &mapplanes[nummapplanes];
	VectorCopy(normal, p->normal);
	p->dist = dist;
	p->type = (p + 1)->type = PlaneTypeForNormal(p->normal);

	VectorSubtract(vec3_origin, normal, (p + 1)->normal);
	(p + 1)->dist = -dist;

	nummapplanes += 2;

	// always put axial planes facing positive first
	if(p->type < 3)
	{
		if(p->normal[0] < 0 || p->normal[1] < 0 || p->normal[2] < 0)
		{
			// flip order
			temp = *p;
			*p = *(p + 1);
			*(p + 1) = temp;

			AddPlaneToHash(p);
			AddPlaneToHash(p + 1);
			return nummapplanes - 1;
		}
	}

	AddPlaneToHash(p);
	AddPlaneToHash(p + 1);
	return nummapplanes - 2;
}



/*
SnapNormal()
Snaps a near-axial normal vector.
Returns qtrue if and only if the normal was adjusted.
*/

qboolean SnapNormal(vec3_t normal)
{
#if Q3MAP2_EXPERIMENTAL_SNAP_NORMAL_FIX
	int             i;
	qboolean        adjusted = qfalse;

	// A change from the original SnapNormal() is that we snap each
	// component that's close to 0.  So for example if a normal is
	// (0.707, 0.707, 0.0000001), it will get snapped to lie perfectly in the
	// XY plane (its Z component will be set to 0 and its length will be
	// normalized).  The original SnapNormal() didn't snap such vectors - it
	// only snapped vectors that were near a perfect axis.

	for(i = 0; i < 3; i++)
	{
		if(normal[i] != 0.0 && -normalEpsilon < normal[i] && normal[i] < normalEpsilon)
		{
			normal[i] = 0.0;
			adjusted = qtrue;
		}
	}

	if(adjusted)
	{
		VectorNormalize(normal, normal);
		return qtrue;
	}
	return qfalse;
#else
	int             i;

	// I would suggest that you uncomment the following code and look at the
	// results:

	/*
	   Sys_Printf("normalEpsilon is %f\n", normalEpsilon);
	   for (i = 0;; i++)
	   {
	   normal[0] = 1.0;
	   normal[1] = 0.0;
	   normal[2] = i * 0.000001;
	   VectorNormalize(normal, normal);
	   if (1.0 - normal[0] >= normalEpsilon) {
	   Sys_Printf("(%f %f %f)\n", normal[0], normal[1], normal[2]);
	   Error("SnapNormal: test completed");
	   }
	   }
	 */

	// When the normalEpsilon is 0.00001, the loop will break out when normal is
	// (0.999990 0.000000 0.004469).  In other words, this is the vector closest
	// to axial that will NOT be snapped.  Anything closer will be snaped.  Now,
	// 0.004469 is close to 1/225.  The length of a circular quarter-arc of radius
	// 1 is PI/2, or about 1.57.  And 0.004469/1.57 is about 0.0028, or about
	// 1/350.  Expressed a different way, 1/350 is also about 0.26/90.
	// This means is that a normal with an angle that is within 1/4 of a degree
	// from axial will be "snapped".  My belief is that the person who wrote the
	// code below did not intend it this way.  I think the person intended that
	// the epsilon be measured against the vector components close to 0, not 1.0.
	// I think the logic should be: if 2 of the normal components are within
	// epsilon of 0, then the vector can be snapped to be perfectly axial.
	// We may consider adjusting the epsilon to a larger value when we make this
	// code fix.

	for(i = 0; i < 3; i++)
	{
		if(fabs(normal[i] - 1) < normalEpsilon)
		{
			VectorClear(normal);
			normal[i] = 1;
			return qtrue;
		}
		if(fabs(normal[i] - -1) < normalEpsilon)
		{
			VectorClear(normal);
			normal[i] = -1;
			return qtrue;
		}
	}
	return qfalse;
#endif
}



/*
SnapPlane()
snaps a plane to normal/distance epsilons
*/

void SnapPlane(vec3_t normal, vec_t * dist)
{
// SnapPlane disabled by LordHavoc because it often messes up collision
// brushes made from triangles of embedded models, and it has little effect
// on anything else (axial planes are usually derived from snapped points)
/*
  SnapPlane reenabled by namespace because of multiple reports of
  q3map2-crashes which were triggered by this patch.
*/
	SnapNormal(normal);

	// TODO: Rambetter has some serious comments here as well.  First off,
	// in the case where a normal is non-axial, there is nothing special
	// about integer distances.  I would think that snapping a distance might
	// make sense for axial normals, but I'm not so sure about snapping
	// non-axial normals.  A shift by 0.01 in a plane, multiplied by a clipping
	// against another plane that is 5 degrees off, and we introduce 0.1 error
	// easily.  A 0.1 error in a vertex is where problems start to happen, such
	// as disappearing triangles.

	// Second, assuming we have snapped the normal above, let's say that the
	// plane we just snapped was defined for some points that are actually
	// quite far away from normal * dist.  Well, snapping the normal in this
	// case means that we've just moved those points by potentially many units!
	// Therefore, if we are going to snap the normal, we need to know the
	// points we're snapping for so that the plane snaps with those points in
	// mind (points remain close to the plane).

	// I would like to know exactly which problems SnapPlane() is trying to
	// solve so that we can better engineer it (I'm not saying that SnapPlane()
	// should be removed altogether).  Fix all this snapping code at some point!

	if(fabs(*dist - Q_rint(*dist)) < distanceEpsilon)
		*dist = Q_rint(*dist);
}

/*
SnapPlaneImproved()
snaps a plane to normal/distance epsilons, improved code
*/
void SnapPlaneImproved(vec3_t normal, vec_t * dist, int numPoints, const vec3_t * points)
{
	int             i;
	vec3_t          center;
	vec_t           distNearestInt;

	if(SnapNormal(normal))
	{
		if(numPoints > 0)
		{
			// Adjust the dist so that the provided points don't drift away.
			VectorClear(center);
			for(i = 0; i < numPoints; i++)
			{
				VectorAdd(center, points[i], center);
			}
			for(i = 0; i < 3; i++)
			{
				center[i] = center[i] / numPoints;
			}
			*dist = DotProduct(normal, center);
		}
	}

	if(VectorIsOnAxis(normal))
	{
		// Only snap distance if the normal is an axis.  Otherwise there
		// is nothing "natural" about snapping the distance to an integer.
		distNearestInt = Q_rint(*dist);
		if(-distanceEpsilon < *dist - distNearestInt && *dist - distNearestInt < distanceEpsilon)
		{
			*dist = distNearestInt;
		}
	}
}



/*
FindFloatPlane()
ydnar: changed to allow a number of test points to be supplied that
must be within an epsilon distance of the plane
*/
// *INDENT-OFF*
int FindFloatPlane(vec3_t innormal, vec_t dist, int numPoints, vec3_t * points)	// NOTE: this has a side effect on the normal. Good or bad?
#ifdef USE_HASHING
{
	int             i, j, hash, h;
	int             pidx;
	plane_t        *p;
	vec_t           d;
	vec3_t          normal;

	VectorCopy(innormal, normal);
#if Q3MAP2_EXPERIMENTAL_SNAP_PLANE_FIX
	SnapPlaneImproved(normal, &dist, numPoints, (const vec3_t *)points);
#else
	SnapPlane(normal, &dist);
#endif
	/* hash the plane */
	hash = (PLANE_HASHES - 1) & (int)fabs(dist);

	/* search the border bins as well */
	for(i = -1; i <= 1; i++)
	{
		h = (hash + i) & (PLANE_HASHES - 1);
		for(pidx = planehash[h] - 1; pidx != -1; pidx = mapplanes[pidx].hash_chain - 1)
		{
			p = &mapplanes[pidx];

			/* do standard plane compare */
			if(!PlaneEqual(p, normal, dist))
				continue;

			/* ydnar: uncomment the following line for old-style plane finding */
			//% return p - mapplanes;

			/* ydnar: test supplied points against this plane */
			for(j = 0; j < numPoints; j++)
			{
				// NOTE: When dist approaches 2^16, the resolution of 32 bit floating
				// point number is greatly decreased.  The distanceEpsilon cannot be
				// very small when world coordinates extend to 2^16.  Making the
				// dot product here in 64 bit land will not really help the situation
				// because the error will already be carried in dist.
				d = DotProduct(points[j], p->normal) - p->dist;
				d = fabs(d);
				if(d != 0.0 && d >= distanceEpsilon)
					break;		// Point is too far from plane.
			}

			/* found a matching plane */
			if(j >= numPoints)
				return p - mapplanes;
		}
	}

	/* none found, so create a new one */
	return CreateNewFloatPlane(normal, dist);
}

#else
{
	int             i;
	plane_t        *p;
	vec3_t          normal;

	VectorCopy(innormal, normal);
#if Q3MAP2_EXPERIMENTAL_SNAP_PLANE_FIX
	SnapPlaneImproved(normal, &dist, numPoints, (const vec3_t *)points);
#else
	SnapPlane(normal, &dist);
#endif
	for(i = 0, p = mapplanes; i < nummapplanes; i++, p++)
	{
		if(!PlaneEqual(p, normal, dist))
			continue;

		/* ydnar: uncomment the following line for old-style plane finding */
		//% return i;

		/* ydnar: test supplied points against this plane */
		for(j = 0; j < numPoints; j++)
		{
			d = DotProduct(points[j], p->normal) - p->dist;
			if(fabs(d) > distanceEpsilon)
				break;
		}

		/* found a matching plane */
		if(j >= numPoints)
			return i;
		// TODO: Note that the non-USE_HASHING code does not compute epsilons
		// for the provided points.  It should do that.  I think this code
		// is unmaintained because nobody sets USE_HASHING to off.
	}

	return CreateNewFloatPlane(normal, dist);
}

#endif
// *INDENT-ON*


/*
MapPlaneFromPoints()
takes 3 points and finds the plane they lie in
*/

int MapPlaneFromPoints(vec3_t * p)
{
#if Q3MAP2_EXPERIMENTAL_HIGH_PRECISION_MATH_FIXES
	vec3_accu_t     paccu[3];
	vec3_accu_t     t1, t2, normalAccu;
	vec3_t          normal;
	vec_t           dist;

	VectorCopyRegularToAccu(p[0], paccu[0]);
	VectorCopyRegularToAccu(p[1], paccu[1]);
	VectorCopyRegularToAccu(p[2], paccu[2]);

	VectorSubtractAccu(paccu[0], paccu[1], t1);
	VectorSubtractAccu(paccu[2], paccu[1], t2);
	CrossProductAccu(t1, t2, normalAccu);
	VectorNormalizeAccu(normalAccu, normalAccu);
	// TODO: A 32 bit float for the plane distance isn't enough resolution
	// if the plane is 2^16 units away from the origin (the "epsilon" approaches
	// 0.01 in that case).
	dist = (vec_t) DotProductAccu(paccu[0], normalAccu);
	VectorCopyAccuToRegular(normalAccu, normal);

	return FindFloatPlane(normal, dist, 3, p);
#else
	vec3_t          t1, t2, normal;
	vec_t           dist;


	/* calc plane normal */
	VectorSubtract(p[0], p[1], t1);
	VectorSubtract(p[2], p[1], t2);
	CrossProduct(t1, t2, normal);
	VectorNormalize(normal);

	/* calc plane distance */
	dist = DotProduct(p[0], normal);

	/* store the plane */
	return FindFloatPlane(normal, dist, 3, p);
#endif
}

int MapPlaneFromEquation(vec4_t plane)
{
	vec3_t          normal;
	vec_t           dist;
	vec_t           mag;

	dist = -plane[3];

	mag = VectorNormalize2(plane, normal);

	if(mag)
		plane[3] /= mag;

	SnapPlane(normal, &dist);

	/* Tr3B: debug it */
//  Sys_FPrintf( SYS_VRB, "MapPlaneFromEquation: (%i %i %i %i)\n", (int)normal[0], (int)normal[1], (int)normal[2], (int)dist);

	return FindFloatPlane(normal, dist, 0, NULL);
}


/*
CompareBrushSides()
compare function for qsort()
*/

static int CompareBrushSides(const void *a, const void *b)
{
	side_t         *sidea, *sideb;

	sidea = (side_t *) a;
	sideb = (side_t *) b;

	/* move area portals to the front */
	if((sidea->compileFlags & C_AREAPORTAL) && !(sideb->compileFlags & C_AREAPORTAL))
		return -1;
	else if(!(sidea->compileFlags & C_AREAPORTAL) && (sideb->compileFlags & C_AREAPORTAL))
		return 1;

	/* functionally equivalent */
	return 0;
}

/*
SetBrushContents()
the content flags and compile flags on all sides of a brush should be the same
*/

void SetBrushContents(brush_t * b)
{
	int             contentFlags, compileFlags;
	side_t         *s;
	int             i;
	qboolean        mixed;

	/* Tr3B: sort brush sides because Doom 3 visportals don't need to be on the first brush side */
	qsort(b->sides, b->numsides, sizeof(b->sides[0]), CompareBrushSides);

	/* get initial compile flags from first side */
	s = &b->sides[0];
	contentFlags = s->contentFlags;
	compileFlags = s->compileFlags;
	b->contentShader = s->shaderInfo;
	mixed = qfalse;

	/* get the content/compile flags for every side in the brush */
	for(i = 1; i < b->numsides; i++, s++)
	{
		s = &b->sides[i];
		if(s->shaderInfo == NULL)
		{
			continue;
		}

		if(s->contentFlags != contentFlags || s->compileFlags != compileFlags)
			mixed = qtrue;

		contentFlags |= s->contentFlags;
		compileFlags |= s->compileFlags;
	}

	/* ydnar: getting rid of this stupid warning */
	//% if( mixed )
	//%     Sys_FPrintf( SYS_VRB,"Entity %i, Brush %i: mixed face contentFlags\n", b->entitynum, b->brushnum );

	/* check for detail & structural */
	if((compileFlags & C_DETAIL) && (compileFlags & C_STRUCTURAL))
	{
#if defined(USE_XML)
		xml_Select("Mixed detail and structural (defaulting to structural)", mapEnt->mapEntityNum, entitySourceBrushes, qfalse);
#else
		Sys_Printf("Entity %i, Brush %i: Mixed detail and structural (defaulting to structural)\n", mapEnt->mapEntityNum, entitySourceBrushes);
#endif
		compileFlags &= ~C_DETAIL;
	}

	/* the fulldetail compile flag will cause detail brush flags to be ignored */
	if(fulldetail)
		compileFlags &= ~C_DETAIL;

	/* all translucent brushes that aren't specifically made structural will be detail */
	if((compileFlags & C_TRANSLUCENT) && !(compileFlags & C_STRUCTURAL))
		compileFlags |= C_DETAIL;

	/* detail? */
	if(compileFlags & C_DETAIL)
	{
		c_detail++;
		b->detail = qtrue;
	}
	else
	{
		c_structural++;
		b->detail = qfalse;
	}

	/* opaque? */
	if(compileFlags & C_TRANSLUCENT)
		b->opaque = qfalse;
	else
		b->opaque = qtrue;

	/* areaportal? */
	if(compileFlags & C_AREAPORTAL)
		c_areaportals++;

	/* set brush flags */
	b->contentFlags = contentFlags;
	b->compileFlags = compileFlags;
}



/*
AddBrushBevels()
adds any additional planes necessary to allow the brush being
built to be expanded against axial bounding boxes
ydnar 2003-01-20: added mrelusive fixes
*/

void AddBrushBevels(void)
{
	int             axis, dir;
	int             i, j, k, l, order;
	side_t          sidetemp;
	side_t         *s, *s2;
	winding_t      *w, *w2;
	vec3_t          normal;
	float           dist;
	vec3_t          vec, vec2;
	float           d, minBack;

	//
	// add the axial planes
	//
	order = 0;
	for(axis = 0; axis < 3; axis++)
	{
		for(dir = -1; dir <= 1; dir += 2, order++)
		{
			// see if the plane is already present
			for(i = 0, s = buildBrush->sides; i < buildBrush->numsides; i++, s++)
			{
				/* ydnar: testing disabling of mre code */
#if 0
				if(dir > 0)
				{
					if(mapplanes[s->planenum].normal[axis] >= 0.9999f)
					{
						break;
					}
				}
				else
				{
					if(mapplanes[s->planenum].normal[axis] <= -0.9999f)
					{
						break;
					}
				}
#else
				if((dir > 0 && mapplanes[s->planenum].normal[axis] == 1.0f) ||
				   (dir < 0 && mapplanes[s->planenum].normal[axis] == -1.0f))
					break;
#endif
			}

			if(i == buildBrush->numsides)
			{
				// add a new side
				if(buildBrush->numsides == MAX_BUILD_SIDES)
				{
#if defined(USE_XML)
					xml_Select("MAX_BUILD_SIDES", buildBrush->entityNum, buildBrush->brushNum, qtrue);
#else
					Sys_Printf("Entity %i, Brush %i: MAX_BUILD_SIDES\n", buildBrush->entityNum, buildBrush->brushNum);
#endif
				}
				memset(s, 0, sizeof(*s));
				buildBrush->numsides++;
				VectorClear(normal);
				normal[axis] = dir;

				if(dir == 1)
				{
					/* ydnar: adding bevel plane snapping for fewer bsp planes */
					if(bevelSnap > 0)
						dist = floor(buildBrush->maxs[axis] / bevelSnap) * bevelSnap;
					else
						dist = buildBrush->maxs[axis];
				}
				else
				{
					/* ydnar: adding bevel plane snapping for fewer bsp planes */
					if(bevelSnap > 0)
						dist = -ceil(buildBrush->mins[axis] / bevelSnap) * bevelSnap;
					else
						dist = -buildBrush->mins[axis];
				}

				s->planenum = FindFloatPlane(normal, dist, 0, NULL);
				s->contentFlags = buildBrush->sides[0].contentFlags;
				s->bevel = qtrue;
				c_boxbevels++;
			}

			// if the plane is not in it canonical order, swap it
			if(i != order)
			{
				sidetemp = buildBrush->sides[order];
				buildBrush->sides[order] = buildBrush->sides[i];
				buildBrush->sides[i] = sidetemp;
			}
		}
	}

	//
	// add the edge bevels
	//
	if(buildBrush->numsides == 6)
	{
		return;					// pure axial
	}

	// test the non-axial plane edges
	for(i = 6; i < buildBrush->numsides; i++)
	{
		s = buildBrush->sides + i;
		w = s->winding;
		if(!w)
		{
			continue;
		}
		for(j = 0; j < w->numpoints; j++)
		{
			k = (j + 1) % w->numpoints;
			VectorSubtract(w->p[j], w->p[k], vec);
			if(VectorNormalize(vec) < 0.5f)
			{
				continue;
			}
			SnapNormal(vec);
			for(k = 0; k < 3; k++)
			{
				if(vec[k] == -1.0f || vec[k] == 1.0f || (vec[k] == 0.0f && vec[(k + 1) % 3] == 0.0f))
				{
					break;		// axial
				}
			}
			if(k != 3)
			{
				continue;		// only test non-axial edges
			}

			/* debug code */
			//% Sys_Printf( "-------------\n" );

			// try the six possible slanted axials from this edge
			for(axis = 0; axis < 3; axis++)
			{
				for(dir = -1; dir <= 1; dir += 2)
				{
					// construct a plane
					VectorClear(vec2);
					vec2[axis] = dir;
					CrossProduct(vec, vec2, normal);
					if(VectorNormalize(normal) < 0.5f)
					{
						continue;
					}
					dist = DotProduct(w->p[j], normal);

					// if all the points on all the sides are
					// behind this plane, it is a proper edge bevel
					for(k = 0; k < buildBrush->numsides; k++)
					{

						// if this plane has already been used, skip it
						if(PlaneEqual(&mapplanes[buildBrush->sides[k].planenum], normal, dist))
						{
							break;
						}

						w2 = buildBrush->sides[k].winding;
						if(!w2)
						{
							continue;
						}
						minBack = 0.0f;
						for(l = 0; l < w2->numpoints; l++)
						{
							d = DotProduct(w2->p[l], normal) - dist;
							if(d > 0.1f)
							{
								break;	// point in front
							}
							if(d < minBack)
							{
								minBack = d;
							}
						}
						// if some point was at the front
						if(l != w2->numpoints)
						{
							break;
						}

						// if no points at the back then the winding is on the bevel plane
						if(minBack > -0.1f)
						{
							//% Sys_Printf( "On bevel plane\n" );
							break;
						}
					}

					if(k != buildBrush->numsides)
					{
						continue;	// wasn't part of the outer hull
					}

					/* debug code */
					//% Sys_Printf( "n = %f %f %f\n", normal[ 0 ], normal[ 1 ], normal[ 2 ] );

					// add this plane
					if(buildBrush->numsides == MAX_BUILD_SIDES)
					{
#if defined(USE_XML)
						xml_Select("MAX_BUILD_SIDES", buildBrush->entityNum, buildBrush->brushNum, qtrue);
#else
						Sys_Printf("Entity %i, Brush %i: MAX_BUILD_SIDES\n", buildBrush->entityNum, buildBrush->brushNum);
#endif
					}
					s2 = &buildBrush->sides[buildBrush->numsides];
					buildBrush->numsides++;
					memset(s2, 0, sizeof(*s2));

					s2->planenum = FindFloatPlane(normal, dist, 1, &w->p[j]);
					s2->contentFlags = buildBrush->sides[0].contentFlags;
					s2->bevel = qtrue;
					c_edgebevels++;
				}
			}
		}
	}
}



/*
FinishBrush()
produces a final brush based on the buildBrush->sides array
and links it to the current entity
*/

static void MergeOrigin(entity_t * ent, vec3_t origin)
{
	vec3_t          adjustment;
	char            string[128];

	/* we have not parsed the brush completely yet... */
	GetVectorForKey(ent, "origin", ent->origin);

	VectorMA(origin, -1, ent->originbrush_origin, adjustment);
	VectorAdd(adjustment, ent->origin, ent->origin);
	VectorCopy(origin, ent->originbrush_origin);

	sprintf(string, "%f %f %f", ent->origin[0], ent->origin[1], ent->origin[2]);
	SetKeyValue(ent, "origin", string);
}

brush_t        *FinishBrush(void)
{
	brush_t        *b;


	/* create windings for sides and bounds for brush */
	if(!CreateBrushWindings(buildBrush))
		return NULL;

	/* origin brushes are removed, but they set the rotation origin for the rest of the brushes in the entity.
	   after the entire entity is parsed, the planenums and texinfos will be adjusted for the origin brush */
	if(buildBrush->compileFlags & C_ORIGIN)
	{
		vec3_t          origin;

		Sys_Printf("Entity %i, Brush %i: origin brush detected\n", mapEnt->mapEntityNum, entitySourceBrushes);

		if(numEntities == 1)
		{
			Sys_Printf("Entity %i, Brush %i: origin brushes not allowed in world\n", mapEnt->mapEntityNum, entitySourceBrushes);
			return NULL;
		}

		VectorAdd(buildBrush->mins, buildBrush->maxs, origin);
		VectorScale(origin, 0.5, origin);

		MergeOrigin(&entities[numEntities - 1], origin);

		/* don't keep this brush */
		return NULL;
	}

	/* determine if the brush is an area portal */
	if(buildBrush->compileFlags & C_AREAPORTAL)
	{
		if(numEntities != 1)
		{
			Sys_Printf("Entity %i, Brush %i: areaportals only allowed in world\n", numEntities - 1, entitySourceBrushes);
			return NULL;
		}
	}

	/* add bevel planes */
	if(convertType == CONVERT_NOTHING)
	{
		AddBrushBevels();
	}

	/* keep it */
	b = CopyBrush(buildBrush);

	/* set map entity and brush numbering */
	b->entityNum = mapEnt->mapEntityNum;
	b->brushNum = entitySourceBrushes;

	/* set original */
	b->original = b;

	/* link opaque brushes to head of list, translucent brushes to end */
	if(b->opaque || mapEnt->lastBrush == NULL)
	{
		b->next = mapEnt->brushes;
		mapEnt->brushes = b;
		if(mapEnt->lastBrush == NULL)
			mapEnt->lastBrush = b;
	}
	else
	{
		b->next = NULL;
		mapEnt->lastBrush->next = b;
		mapEnt->lastBrush = b;
	}

	/* link colorMod volume brushes to the entity directly */
	if(b->contentShader != NULL && b->contentShader->colorMod != NULL && b->contentShader->colorMod->type == CM_VOLUME)
	{
		b->nextColorModBrush = mapEnt->colorModBrushes;
		mapEnt->colorModBrushes = b;
	}

	/* return to sender */
	return b;
}



/*
TextureAxisFromPlane()
determines best orthagonal axis to project a texture onto a wall
(must be identical in radiant!)
*/

vec3_t          baseaxis[18] = {
	{0, 0, 1}
	, {1, 0, 0}
	, {0, -1, 0}
	,							// floor
	{0, 0, -1}
	, {1, 0, 0}
	, {0, -1, 0}
	,							// ceiling
	{1, 0, 0}
	, {0, 1, 0}
	, {0, 0, -1}
	,							// west wall
	{-1, 0, 0}
	, {0, 1, 0}
	, {0, 0, -1}
	,							// east wall
	{0, 1, 0}
	, {1, 0, 0}
	, {0, 0, -1}
	,							// south wall
	{0, -1, 0}
	, {1, 0, 0}
	, {0, 0, -1}				// north wall
};

void TextureAxisFromPlane(plane_t * pln, vec3_t xv, vec3_t yv)
{
	int             bestaxis;
	vec_t           dot, best;
	int             i;

	best = 0;
	bestaxis = 0;

	for(i = 0; i < 6; i++)
	{
		dot = DotProduct(pln->normal, baseaxis[i * 3]);
		if(dot > best + 0.0001f)	/* ydnar: bug 637 fix, suggested by jmonroe */
		{
			best = dot;
			bestaxis = i;
		}
	}

	VectorCopy(baseaxis[bestaxis * 3 + 1], xv);
	VectorCopy(baseaxis[bestaxis * 3 + 2], yv);
}



/*
QuakeTextureVecs()
creates world-to-texture mapping vecs for crappy quake plane arrangements
*/

void QuakeTextureVecs(plane_t * plane, vec_t shift[2], vec_t rotate, vec_t scale[2], vec_t mappingVecs[2][4])
{
	vec3_t          vecs[2];
	int             sv, tv;
	vec_t           ang, sinv, cosv;
	vec_t           ns, nt;
	int             i, j;


	TextureAxisFromPlane(plane, vecs[0], vecs[1]);

	if(!scale[0])
		scale[0] = 1;
	if(!scale[1])
		scale[1] = 1;

	// rotate axis
	if(rotate == 0)
	{
		sinv = 0;
		cosv = 1;
	}
	else if(rotate == 90)
	{
		sinv = 1;
		cosv = 0;
	}
	else if(rotate == 180)
	{
		sinv = 0;
		cosv = -1;
	}
	else if(rotate == 270)
	{
		sinv = -1;
		cosv = 0;
	}
	else
	{
		ang = rotate / 180 * Q_PI;
		sinv = sin(ang);
		cosv = cos(ang);
	}

	if(vecs[0][0])
		sv = 0;
	else if(vecs[0][1])
		sv = 1;
	else
		sv = 2;

	if(vecs[1][0])
		tv = 0;
	else if(vecs[1][1])
		tv = 1;
	else
		tv = 2;

	for(i = 0; i < 2; i++)
	{
		ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
		nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
		vecs[i][sv] = ns;
		vecs[i][tv] = nt;
	}

	for(i = 0; i < 2; i++)
		for(j = 0; j < 3; j++)
			mappingVecs[i][j] = vecs[i][j] / scale[i];

	mappingVecs[0][3] = shift[0];
	mappingVecs[1][3] = shift[1];
}



/*
ParseRawBrush()
parses the sides into buildBrush->sides[], nothing else.
no validation, back plane removal, etc.

Timo - 08/26/99
added brush epairs parsing ( ignoring actually )
Timo - 08/04/99
added exclusive brush primitive parsing
Timo - 08/08/99
support for old brush format back in
NOTE: it would be "cleaner" to have separate functions to parse between old and new brushes
*/

static void ParseRawBrush(qboolean onlyLights)
{
	side_t         *side;
	vec3_t          planePoints[3];
	vec4_t          planeEquation;
	int             planenum;
	shaderInfo_t   *si;
	vec_t           shift[2];
	vec_t           rotate;
	vec_t           scale[2];
	char            name[MAX_QPATH];
	char            shader[MAX_QPATH];
	int             flags;


	/* initial setup */
	buildBrush->numsides = 0;
	buildBrush->detail = qfalse;

	/* bp */
	if(g_bBrushPrimit >= BPRIMIT_NEWBRUSHES)
		MatchToken("{");

	/* parse sides */
	while(1)
	{
		if(!GetToken(qtrue))
			break;
		if(!strcmp(token, "}"))
			break;

		/* ttimo : bp: here we may have to jump over brush epairs (only used in editor) */
		if(g_bBrushPrimit >= BPRIMIT_NEWBRUSHES)
		{
			while(1)
			{
				if(strcmp(token, "("))
					GetToken(qfalse);
				else
					break;
				GetToken(qtrue);
			}
		}
		UnGetToken();

		/* test side count */
		if(buildBrush->numsides >= MAX_BUILD_SIDES)
#if defined(USE_XML)
			xml_Select("MAX_BUILD_SIDES", buildBrush->entityNum, buildBrush->brushNum, qtrue);
#else
			Sys_Printf("Entity %i, Brush %i: MAX_BUILD_SIDES\n", buildBrush->entityNum, buildBrush->brushNum);
#endif

		/* add side */
		side = &buildBrush->sides[buildBrush->numsides];
		memset(side, 0, sizeof(*side));
		buildBrush->numsides++;

		if(g_bBrushPrimit >= BPRIMIT_D3BRUSHES)
		{
			/* read the plane equation */
			Parse1DMatrix(4, planeEquation);
		}
		else
		{
			/* read the three point plane definition */
			Parse1DMatrix(3, planePoints[0]);
			Parse1DMatrix(3, planePoints[1]);
			Parse1DMatrix(3, planePoints[2]);
		}

		/* bp: read the texture matrix */
		if(g_bBrushPrimit >= BPRIMIT_NEWBRUSHES)
			Parse2DMatrix(2, 3, (float *)side->texMat);

		/* read shader name */
		GetToken(qfalse);
		strcpy(name, token);

		/* bp */
		if(g_bBrushPrimit == BPRIMIT_OLDBRUSHES)
		{
			GetToken(qfalse);
			shift[0] = atof(token);
			GetToken(qfalse);
			shift[1] = atof(token);
			GetToken(qfalse);
			rotate = atof(token);
			GetToken(qfalse);
			scale[0] = atof(token);
			GetToken(qfalse);
			scale[1] = atof(token);
		}

		/* set default flags and values */
		if(!Q_strncasecmp(name, "textures/", 9))
			sprintf(shader, "%s", name);
		else
			sprintf(shader, "textures/%s", name);

		if(onlyLights)
			si = &shaderInfo[0];
		else
			si = ShaderInfoForShader(shader);
		side->shaderInfo = si;
		side->surfaceFlags = si->surfaceFlags;
		side->contentFlags = si->contentFlags;
		side->compileFlags = si->compileFlags;
		side->value = si->value;

		/* ydnar: gs mods: bias texture shift */
		if(si->globalTexture == qfalse)
		{
			shift[0] -= (floor(shift[0] / si->shaderWidth) * si->shaderWidth);
			shift[1] -= (floor(shift[1] / si->shaderHeight) * si->shaderHeight);
		}

		/*
		   historically, there are 3 integer values at the end of a brushside line in a .map file.
		   in quake 3, the only thing that mattered was the first of these three values, which
		   was previously the content flags. and only then did a single bit matter, the detail
		   bit. because every game has its own special flags for specifying detail, the
		   traditionally game-specified CONTENTS_DETAIL flag was overridden for Q3Map 2.3.0
		   by C_DETAIL, defined in q3map2.h. the value is exactly as it was before, but
		   is stored in compileFlags, as opposed to contentFlags, for multiple-game
		   portability. :sigh:
		 */

		if(g_bBrushPrimit != BPRIMIT_Q4BRUSHES && TokenAvailable())
		{
			/* get detail bit from map content flags */
			GetToken(qfalse);
			flags = atoi(token);
			if(flags & C_DETAIL)
				side->compileFlags |= C_DETAIL;

			/* historical */
			GetToken(qfalse);
			//% td.flags = atoi( token );
			GetToken(qfalse);
			//% td.value = atoi( token );
		}

		/* find the plane number */
		if(g_bBrushPrimit >= BPRIMIT_D3BRUSHES)
			planenum = MapPlaneFromEquation(planeEquation);
		else
			planenum = MapPlaneFromPoints(planePoints);
		side->planenum = planenum;

		/* bp: get the texture mapping for this texturedef / plane combination */
		if(g_bBrushPrimit == BPRIMIT_OLDBRUSHES)
			QuakeTextureVecs(&mapplanes[planenum], shift, rotate, scale, side->vecs);
	}

	/* bp */
	if(g_bBrushPrimit >= BPRIMIT_NEWBRUSHES)
	{
		UnGetToken();
		MatchToken("}");
		MatchToken("}");
	}
}



/*
RemoveDuplicateBrushPlanes
returns false if the brush has a mirrored set of planes,
meaning it encloses no volume.
also removes planes without any normal
*/

qboolean RemoveDuplicateBrushPlanes(brush_t * b)
{
	int             i, j, k;
	side_t         *sides;

	sides = b->sides;

	for(i = 1; i < b->numsides; i++)
	{

		// check for a degenerate plane
		if(sides[i].planenum == -1)
		{
#if defined(USE_XML)
			xml_Select("degenerate plane", b->entityNum, b->brushNum, qfalse);
#else
			Sys_Printf("Entity %i, Brush %i: degenerate plane\n", b->entityNum, b->brushNum);
#endif

			// remove it
			for(k = i + 1; k < b->numsides; k++)
			{
				sides[k - 1] = sides[k];
			}
			b->numsides--;
			i--;
			continue;
		}

		// check for duplication and mirroring
		for(j = 0; j < i; j++)
		{
			if(sides[i].planenum == sides[j].planenum)
			{
#if defined(USE_XML)
				xml_Select("duplicate plane", b->entityNum, b->brushNum, qfalse);
#else
				Sys_Printf("Entity %i, Brush %i: duplicate plane\n", b->entityNum, b->brushNum);
#endif

				// remove the second duplicate
				for(k = i + 1; k < b->numsides; k++)
				{
					sides[k - 1] = sides[k];
				}
				b->numsides--;
				i--;
				break;
			}

			if(sides[i].planenum == (sides[j].planenum ^ 1))
			{
				// mirror plane, brush is invalid
#if defined(USE_XML)
				xml_Select("mirrored plane", b->entityNum, b->brushNum, qfalse);
#else
				Sys_Printf("Entity %i, Brush %i: mirrored plane\n", b->entityNum, b->brushNum);
#endif
				return qfalse;
			}
		}
	}
	return qtrue;
}



/*
ParseBrush()
parses a brush out of a map file and sets it up
*/

static void ParseBrush(qboolean onlyLights)
{
	brush_t        *b;


	/* parse the brush out of the map */
	ParseRawBrush(onlyLights);

	/* only go this far? */
	if(onlyLights)
		return;

	/* set some defaults */
	buildBrush->portalareas[0] = -1;
	buildBrush->portalareas[1] = -1;
	buildBrush->entityNum = numMapEntities - 1;
	buildBrush->brushNum = entitySourceBrushes;

	/* if there are mirrored planes, the entire brush is invalid */
	if(!RemoveDuplicateBrushPlanes(buildBrush))
		return;

	/* get the content for the entire brush */
	SetBrushContents(buildBrush);

	/* allow detail brushes to be removed */
	if(nodetail && (buildBrush->compileFlags & C_DETAIL))
	{
		//% FreeBrush( buildBrush );
		return;
	}

	/* allow liquid brushes to be removed */
	if(nowater && (buildBrush->compileFlags & C_LIQUID))
	{
		//% FreeBrush( buildBrush );
		return;
	}

	/* ydnar: allow hint brushes to be removed */
	if(noHint && (buildBrush->compileFlags & C_HINT))
	{
		//% FreeBrush( buildBrush );
		return;
	}

	/* finish the brush */
	b = FinishBrush();
}



/*
MoveBrushesToWorld()
takes all of the brushes from the current entity and
adds them to the world's brush list
(used by func_group)
*/

void MoveBrushesToWorld(entity_t * ent)
{
	brush_t        *b, *next;

	/* move brushes */
	for(b = ent->brushes; b != NULL; b = next)
	{
		/* get next brush */
		next = b->next;

		/* link opaque brushes to head of list, translucent brushes to end */
		if(b->opaque || entities[0].lastBrush == NULL)
		{
			b->next = entities[0].brushes;
			entities[0].brushes = b;
			if(entities[0].lastBrush == NULL)
				entities[0].lastBrush = b;
		}
		else
		{
			b->next = NULL;
			entities[0].lastBrush->next = b;
			entities[0].lastBrush = b;
		}
	}
	ent->brushes = NULL;

	/* ydnar: move colormod brushes */
	if(ent->colorModBrushes != NULL)
	{
		for(b = ent->colorModBrushes; b->nextColorModBrush != NULL; b = b->nextColorModBrush);

		b->nextColorModBrush = entities[0].colorModBrushes;
		entities[0].colorModBrushes = ent->colorModBrushes;

		ent->colorModBrushes = NULL;
	}
}

void MovePatchesToWorld(entity_t * ent)
{
	parseMesh_t    *pm;

	/* move patches */
	if(ent->patches != NULL)
	{
		for(pm = ent->patches; pm->next != NULL; pm = pm->next);

		pm->next = entities[0].patches;
		entities[0].patches = ent->patches;

		ent->patches = NULL;
	}
}



/*
AdjustBrushesForOrigin()
*/

void AdjustBrushesForOrigin(entity_t * ent, vec3_t origin)
{
	int             i;
	side_t         *s;
	vec_t           newdist;
	brush_t        *b;

	/* walk brush list */
	for(b = ent->brushes; b != NULL; b = b->next)
	{
		/* offset brush planes */
		for(i = 0; i < b->numsides; i++)
		{
			/* get brush side */
			s = &b->sides[i];

			/* offset side plane */
			newdist = mapplanes[s->planenum].dist - DotProduct(mapplanes[s->planenum].normal, origin);

			/* find a new plane */
			s->planenum = FindFloatPlane(mapplanes[s->planenum].normal, newdist, 0, NULL);
		}

		/* rebuild brush windings (ydnar: just offsetting the winding above should be fine) */
		CreateBrushWindings(b);
	}
}

/*
AdjustPatchesForOrigin()
*/

void AdjustPatchesForOrigin(entity_t * ent, vec3_t origin)
{
	int             i;
	parseMesh_t    *p;

	/* walk patch list */
	for(p = ent->patches; p != NULL; p = p->next)
	{
		for(i = 0; i < (p->mesh.width * p->mesh.height); i++)
			VectorSubtract(p->mesh.verts[i].xyz, origin, p->mesh.verts[i].xyz);
	}
}



/*
SetEntityBounds() - ydnar
finds the bounds of an entity's brushes (necessary for terrain-style generic metashaders)
*/

void SetEntityBounds(entity_t * e)
{
	int             i;
	brush_t        *b;
	parseMesh_t    *p;
	vec3_t          mins, maxs;
	const char     *value;

	/* walk the entity's brushes/patches and determine bounds */
	ClearBounds(mins, maxs);
	for(b = e->brushes; b; b = b->next)
	{
		AddPointToBounds(b->mins, mins, maxs);
		AddPointToBounds(b->maxs, mins, maxs);
	}
	for(p = e->patches; p; p = p->next)
	{
		for(i = 0; i < (p->mesh.width * p->mesh.height); i++)
			AddPointToBounds(p->mesh.verts[i].xyz, mins, maxs);
	}

	/* try to find explicit min/max key */
	value = ValueForKey(e, "min");
	if(value[0] != '\0')
		GetVectorForKey(e, "min", mins);
	value = ValueForKey(e, "max");
	if(value[0] != '\0')
		GetVectorForKey(e, "max", maxs);

	/* store the bounds */
	for(b = e->brushes; b; b = b->next)
	{
		VectorCopy(mins, b->eMins);
		VectorCopy(maxs, b->eMaxs);
	}
	for(p = e->patches; p; p = p->next)
	{
		VectorCopy(mins, p->eMins);
		VectorCopy(maxs, p->eMaxs);
	}
}



/*
LoadEntityIndexMap() - ydnar
based on LoadAlphaMap() from terrain.c, a little more generic
*/

void LoadEntityIndexMap(entity_t * e)
{
	int             i, size, numLayers, w, h;
	const char     *value, *indexMapFilename, *shader;
	char            ext[MAX_QPATH], offset[4096], *search, *space;
	byte           *pixels;
	unsigned int   *pixels32;
	indexMap_t     *im;
	brush_t        *b;
	parseMesh_t    *p;


	/* this only works with bmodel ents */
	if(e->brushes == NULL && e->patches == NULL)
		return;

	/* determine if there is an index map (support legacy "alphamap" key as well) */
	value = ValueForKey(e, "_indexmap");
	if(value[0] == '\0')
		value = ValueForKey(e, "alphamap");
	if(value[0] == '\0')
		return;
	indexMapFilename = value;

	/* get number of layers (support legacy "layers" key as well) */
	value = ValueForKey(e, "_layers");
	if(value[0] == '\0')
		value = ValueForKey(e, "layers");
	if(value[0] == '\0')
	{
		Sys_Printf("WARNING: Entity with index/alpha map \"%s\" has missing \"_layers\" or \"layers\" key\n", indexMapFilename);
		Sys_Printf("Entity will not be textured properly. Check your keys/values.\n");
		return;
	}
	numLayers = atoi(value);
	if(numLayers < 1)
	{
		Sys_Printf("WARNING: Entity with index/alpha map \"%s\" has < 1 layer (%d)\n", indexMapFilename, numLayers);
		Sys_Printf("Entity will not be textured properly. Check your keys/values.\n");
		return;
	}

	/* get base shader name (support legacy "shader" key as well) */
	value = ValueForKey(mapEnt, "_shader");
	if(value[0] == '\0')
		value = ValueForKey(e, "shader");
	if(value[0] == '\0')
	{
		Sys_Printf("WARNING: Entity with index/alpha map \"%s\" has missing \"_shader\" or \"shader\" key\n", indexMapFilename);
		Sys_Printf("Entity will not be textured properly. Check your keys/values.\n");
		return;
	}
	shader = value;

	/* note it */
	Sys_FPrintf(SYS_VRB, "Entity %d (%s) has shader index map \"%s\"\n", mapEnt->mapEntityNum, ValueForKey(e, "classname"),
				indexMapFilename);

	/* get index map file extension */
	ExtractFileExtension(indexMapFilename, ext);

	/* handle tga image */
	if(!Q_stricmp(ext, "tga"))
	{
		/* load it */
		Load32BitImage(indexMapFilename, &pixels32, &w, &h);

		/* convert to bytes */
		size = w * h;
		pixels = safe_malloc(size);
		for(i = 0; i < size; i++)
		{
			pixels[i] = ((pixels32[i] & 0xFF) * numLayers) / 256;
			if(pixels[i] >= numLayers)
				pixels[i] = numLayers - 1;
		}

		/* free the 32 bit image */
		free(pixels32);
	}
	else
	{
		/* load it */
		Load256Image(indexMapFilename, &pixels, NULL, &w, &h);

		/* debug code */
		//% Sys_Printf( "-------------------------------" );

		/* fix up out-of-range values */
		size = w * h;
		for(i = 0; i < size; i++)
		{
			if(pixels[i] >= numLayers)
				pixels[i] = numLayers - 1;

			/* debug code */
			//% if( (i % w) == 0 )
			//%     Sys_Printf( "\n" );
			//% Sys_Printf( "%c", pixels[ i ] + '0' );
		}

		/* debug code */
		//% Sys_Printf( "\n-------------------------------\n" );
	}

	/* the index map must be at least 2x2 pixels */
	if(w < 2 || h < 2)
	{
		Sys_Printf("WARNING: Entity with index/alpha map \"%s\" is smaller than 2x2 pixels\n", indexMapFilename);
		Sys_Printf("Entity will not be textured properly. Check your keys/values.\n");
		free(pixels);
		return;
	}

	/* create a new index map */
	im = safe_malloc(sizeof(*im));
	memset(im, 0, sizeof(*im));

	/* set it up */
	im->w = w;
	im->h = h;
	im->numLayers = numLayers;
	strcpy(im->name, indexMapFilename);
	strcpy(im->shader, shader);
	im->pixels = pixels;

	/* get height offsets */
	value = ValueForKey(mapEnt, "_offsets");
	if(value[0] == '\0')
		value = ValueForKey(e, "offsets");
	if(value[0] != '\0')
	{
		/* value is a space-separated set of numbers */
		strcpy(offset, value);
		search = offset;

		/* get each value */
		for(i = 0; i < 256 && *search != '\0'; i++)
		{
			space = strstr(search, " ");
			if(space != NULL)
				*space = '\0';
			im->offsets[i] = atof(search);
			if(space == NULL)
				break;
			search = space + 1;
		}
	}

	/* store the index map in every brush/patch in the entity */
	for(b = e->brushes; b != NULL; b = b->next)
		b->im = im;
	for(p = e->patches; p != NULL; p = p->next)
		p->im = im;
}







/*
ParseMapEntity()
parses a single entity out of a map file
*/

static qboolean ParseMapEntity(qboolean onlyLights)
{
	int             i;
	epair_t        *ep;
	const char     *targetname;
	const char     *classname, *value, *name, *model;
	const char     *name2;
	entity_t       *otherEnt;
	float           lightmapScale, shadeAngle;
	int             lightmapSampleSize;
//	char            shader[MAX_QPATH];
	shaderInfo_t   *celShader = NULL;
	brush_t        *brush;
	parseMesh_t    *patch;
	qboolean        funcGroup;
	int             castShadows, recvShadows;

	/* eof check */
	if(!GetToken(qtrue))
		return qfalse;

	/* Tr3B: Version check for newer .map formats */
	if(!strcmp(token, "Version"))
	{
		if(!GetToken(qfalse))
			return qfalse;

		if(atoi(token) == 2)
		{
			g_bBrushPrimit = BPRIMIT_D3BRUSHES;
		}
		else if(atoi(token) == 3)
		{
			g_bBrushPrimit = BPRIMIT_Q4BRUSHES;
		}
		else
		{
			Sys_Printf("WARNING: ParseEntity: bad version number.\n");
			return qfalse;
		}

		return qtrue;
	}

	/* conformance check */
	if(strcmp(token, "{"))
	{
		Sys_Printf("WARNING: ParseEntity: { not found, found %s - last entity was at: <%4.2f, %4.2f, %4.2f>...\n"
				   "Continuing to process map, but resulting BSP may be invalid.\n",
				   token, entities[numEntities].origin[0], entities[numEntities].origin[1], entities[numEntities].origin[2]);
		return qfalse;
	}

	/* range check */
	if(numEntities >= MAX_MAP_ENTITIES)
		Error("numEntities == MAX_MAP_ENTITIES");

	/* setup */
	entitySourceBrushes = 0;
	mapEnt = &entities[numEntities];
	numEntities++;
	memset(mapEnt, 0, sizeof(*mapEnt));

	/* ydnar: true entity numbering */
	mapEnt->mapEntityNum = numMapEntities;
	numMapEntities++;

	/* loop */
	while(1)
	{
		/* get initial token */
		if(!GetToken(qtrue))
		{
			Sys_Printf("WARNING: ParseEntity: EOF without closing brace\n"
					   "Continuing to process map, but resulting BSP may be invalid.\n");
			return qfalse;
		}

		if(!strcmp(token, "}"))
			break;

		if(!strcmp(token, "{"))
		{
			/* parse a brush or patch */
			if(!GetToken(qtrue))
				break;

			/* check */
			if(!strcmp(token, "patchDef2"))
			{
				numMapPatches++;
				ParsePatch(onlyLights, qfalse);
			}
			else if(!strcmp(token, "patchDef3"))
			{
				numMapPatches++;
				ParsePatch(onlyLights, qtrue);
			}
			else if(!strcmp(token, "terrainDef"))
			{
				//% ParseTerrain();
				Sys_Printf("WARNING: Terrain entity parsing not supported in this build.\n");	/* ydnar */
			}
			else if(!strcmp(token, "brushDef"))
			{
				if(g_bBrushPrimit == BPRIMIT_OLDBRUSHES)
					Error("Old brush format not allowed in new brush format map");
				g_bBrushPrimit = BPRIMIT_NEWBRUSHES;

				/* parse brush primitive */
				ParseBrush(onlyLights);
			}
			else if(!strcmp(token, "brushDef3"))
			{
				if(g_bBrushPrimit == BPRIMIT_OLDBRUSHES)
					Error("Old brush format not allowed in new brush format map");

				/* parse brush primitive */
				ParseBrush(onlyLights);
			}
			else
			{
				if(g_bBrushPrimit == BPRIMIT_NEWBRUSHES)
					Error("New brush format not allowed in old brush format map");
				g_bBrushPrimit = BPRIMIT_OLDBRUSHES;

				/* parse old brush format */
				UnGetToken();
				ParseBrush(onlyLights);
			}
			entitySourceBrushes++;
		}
		else
		{
			/* parse a key / value pair */
			ep = ParseEPair();

			/* ydnar: 2002-07-06 fixed wolf bug with empty epairs */
			if(ep->key[0] != '\0' && ep->value[0] != '\0')
			{
				ep->next = mapEnt->epairs;
				mapEnt->epairs = ep;
			}
		}
	}

	/* ydnar: get classname */
	classname = ValueForKey(mapEnt, "classname");
	name = ValueForKey(mapEnt, "name");
	model = ValueForKey(mapEnt, "model");
	GetVectorForKey(mapEnt, "origin", mapEnt->origin);

	/* ydnar: determine if this is a func_group */
	if(!Q_stricmp("func_group", classname))
		funcGroup = qtrue;
	else
		funcGroup = qfalse;

	if(convertType == CONVERT_QUAKE3)
	{
		// Tr3B: rename misc_model to func_static
#if 0
		if(!Q_stricmp("misc_model", classname) && model[0] != '\0')
		{
			SetKeyValue(mapEnt, "classname", "func_static");
		}
#endif

		/* Tr3B: rename targetname to name if necessary */
		targetname = ValueForKey(mapEnt, "targetname");
		if(targetname[0])
		{
			SetKeyValue(mapEnt, "name", targetname);
			name = ValueForKey(mapEnt, "name");
			RemoveKey(mapEnt, "targetname");
		}

		/* Tr3B: check for empty name */
#if 1
		if(!name[0] && numEntities != 1)
		{
			name = UniqueEntityName(mapEnt, classname);
			if(!name[0])
			{
#if defined(USE_XML)
				xml_Select("UniqueEntityName failed", mapEnt->mapEntityNum, 0, qtrue);
#else
				Error("Entity %i, Brush %i: UniqueEntityName failed", mapEnt->mapEntityNum, 0);
#endif
			}

			SetKeyValue(mapEnt, "name", name);
			SetKeyValue(mapEnt, "_owmap_autogeneratedName", "1");
			name = ValueForKey(mapEnt, "name");
		}
#endif

		/* Tr3B: check for bad duplicated names */
#if 1
		for(i = 0; i < numEntities; i++)
		{
			otherEnt = &entities[i];

			if(mapEnt == otherEnt)
				continue;

			name2 = ValueForKey(otherEnt, "name");

			if(!Q_stricmp(name, name2))
			{
				const char     *newName;

				/*
#if defined(USE_XML)
				xml_Select("duplicated entity name", mapEnt->mapEntityNum, 0, qfalse);
#else
				Sys_Printf("Entity %i, Brush %i: duplicated entity name\n", mapEnt->mapEntityNum, 0);
#endif
				*/
				newName = UniqueEntityName(mapEnt, name);
				if(!newName[0])
				{
#if defined(USE_XML)
					xml_Select("UniqueEntityName failed", mapEnt->mapEntityNum, 0, qtrue);
#else
					Error("Entity %i, UniqueEntityName failed", mapEnt->mapEntityNum);
#endif
				}

				Sys_Printf("Entity %i, class ='%s', old name = '%s', new name = '%s' : duplicated entity name\n", mapEnt->mapEntityNum, classname, name, newName);

				SetKeyValue(mapEnt, "_owmap_oldName", name);
				SetKeyValue(mapEnt, "name", newName);
				name = ValueForKey(mapEnt, "name");
				SetKeyValue(mapEnt, "_owmap_fixedDuplicatedName", "1");
				break;
			}
		}
#endif

		// Tr3B
		if(!model[0] && Q_stricmp("func_static", classname) && (mapEnt->brushes || mapEnt->patches) && numEntities != 1)
		{
			SetKeyValue(mapEnt, "model", name);
			model = ValueForKey(mapEnt, "model");
		}

		// Tr3B: move modelscale to rotation key because DarkRadiant does not support modelscale
		#if 1
		if(!Q_stricmp("misc_model", classname))
		{
			vec_t			temp;
			vec3_t          angles, scale;
			matrix_t        rotation, rotationScaled;

			/* get "angle" (yaw) or "angles" (pitch yaw roll) */
			MatrixIdentity(rotation);
			angles[0] = angles[1] = angles[2] = 0.0f;

			value = ValueForKey(mapEnt, "angle");
			if(value[0] != '\0')
			{
				angles[1] = atof(value);
				MatrixFromAngles(rotation, angles[PITCH], angles[YAW], angles[ROLL]);
				RemoveKey(mapEnt, "angle");
			}

			value = ValueForKey(mapEnt, "angles");
			if(value[0] != '\0')
			{
				sscanf(value, "%f %f %f", &angles[0], &angles[1], &angles[2]);
				MatrixFromAngles(rotation, angles[PITCH], angles[YAW], angles[ROLL]);
				RemoveKey(mapEnt, "angles");
			}

			value = ValueForKey(mapEnt, "rotation");
			if(value[0] != '\0')
			{
				sscanf(value, "%f %f %f %f %f %f %f %f %f", &rotation[0], &rotation[1], &rotation[2],
					   &rotation[4], &rotation[5], &rotation[6], &rotation[8], &rotation[9], &rotation[10]);
			}

			/* get scale */
			MatrixCopy(rotation, rotationScaled);

			scale[0] = scale[1] = scale[2] = 1.0f;
			temp = FloatForKey(mapEnt, "modelscale");
			if(temp != 0.0f)
			{
				scale[0] = scale[1] = scale[2] = temp;
				RemoveKey(mapEnt, "modelscale");
			}
			value = ValueForKey(mapEnt, "modelscale_vec");
			if(value[0] != '\0')
			{
				sscanf(value, "%f %f %f", &scale[0], &scale[1], &scale[2]);
				RemoveKey(mapEnt, "modelscale_vec");
			}

			MatrixMultiplyScale(rotationScaled, scale[0], scale[1], scale[2]);

			SetKeyValue(mapEnt, "rotation", va("%f %f %f %f %f %f %f %f %f", rotationScaled[0], rotationScaled[1], rotationScaled[2],
				   rotationScaled[4], rotationScaled[5], rotationScaled[6], rotationScaled[8], rotationScaled[9], rotationScaled[10]));
		}
		#endif

		// Tr3B: move detail brushes from world spawn into separate func_static
		// or the detail brush flags will be lost and the brushes will generate really many bsp splits
		if(numEntities == 1 || funcGroup)
		{
			brush_t        *b, *prev, *next;
			GSList         *structuralBrushes = NULL;
			GSList         *detailBrushes = NULL;
			guint			numDetailBrushes;

			memset(&convertDetailBrushesEntity, 0, sizeof(convertDetailBrushesEntity));

			if(funcGroup)
			{
				epair_t        *ep;
				const char     *name;

				// duplicate epairs
				for(ep = mapEnt->epairs; ep != NULL; ep = ep->next)
				{
					SetKeyValue(&convertDetailBrushesEntity, ep->key, ep->value);
				}

				name = ValueForKey(mapEnt, "name");
				SetKeyValue(&convertDetailBrushesEntity, "classname", "func_static");
				SetKeyValue(&convertDetailBrushesEntity, "name", va("%s_detail", name));
				SetKeyValue(&convertDetailBrushesEntity, "model", va("%s_detail", name));
			}
			else
			{
				// processing worldspawn entity
				SetKeyValue(&convertDetailBrushesEntity, "classname", "func_static");
				SetKeyValue(&convertDetailBrushesEntity, "name", "worldspawn_detail_brushes_0");
				SetKeyValue(&convertDetailBrushesEntity, "model", "worldspawn_detail_brushes_0");
				SetKeyValue(&convertDetailBrushesEntity, "description", "Generated by OWMap (OpenWolf)");
			}

			// move brushes
			for(b = mapEnt->brushes, prev = NULL; b != NULL; b = next)
			{
				// get next brush
				next = b->next;

				if(b->compileFlags & C_DETAIL)
				{
					detailBrushes = g_slist_append(detailBrushes, b);
				}
				else
				{
					structuralBrushes = g_slist_append(structuralBrushes, b);
				}

				numDetailBrushes = g_slist_length(detailBrushes);
			}

			// add detail brushes to new func_static entity
			while(detailBrushes)
			{
				b = (brush_t*) detailBrushes->data;

				// link opaque brushes to head of list, translucent brushes to end
				if(b->opaque || convertDetailBrushesEntity.lastBrush == NULL)
				{
					b->next = convertDetailBrushesEntity.brushes;
					convertDetailBrushesEntity.brushes = b;
					if(convertDetailBrushesEntity.lastBrush == NULL)
						convertDetailBrushesEntity.lastBrush = b;
				}
				else
				{
					b->next = NULL;
					convertDetailBrushesEntity.lastBrush->next = b;
					convertDetailBrushesEntity.lastBrush = b;
				}

				// link colorMod volume brushes to the entity directly
				if(b->contentShader != NULL && b->contentShader->colorMod != NULL && b->contentShader->colorMod->type == CM_VOLUME)
				{
					b->nextColorModBrush = convertDetailBrushesEntity.colorModBrushes;
					convertDetailBrushesEntity.colorModBrushes = b;
				}

				detailBrushes = g_slist_remove(detailBrushes, detailBrushes->data);
			}

			mapEnt->brushes = NULL;
			mapEnt->colorModBrushes = NULL;

			// readd structural brushes to current entity
			while(structuralBrushes)
			{
				b = (brush_t*) structuralBrushes->data;

				// link opaque brushes to head of list, translucent brushes to end
				if(b->opaque || mapEnt->lastBrush == NULL)
				{
					b->next = mapEnt->brushes;
					mapEnt->brushes = b;
					if(mapEnt->lastBrush == NULL)
						mapEnt->lastBrush = b;
				}
				else
				{
					b->next = NULL;
					mapEnt->lastBrush->next = b;
					mapEnt->lastBrush = b;
				}

				// link colorMod volume brushes to the entity directly
				if(b->contentShader != NULL && b->contentShader->colorMod != NULL && b->contentShader->colorMod->type == CM_VOLUME)
				{
					b->nextColorModBrush = mapEnt->colorModBrushes;
					mapEnt->colorModBrushes = b;
				}

				structuralBrushes = g_slist_remove(structuralBrushes, structuralBrushes->data);
			}
		}
	}


	if(model[0] && !Q_stricmp("func_group", classname) && !mapEnt->brushes && !mapEnt->patches)
	{
		RemoveKey(mapEnt, "model");
	}


	/* ydnar: only lights? */
	if(onlyLights && Q_strncasecmp(classname, "light", 5))
	{
		numEntities--;
		return qtrue;
	}

	/* worldspawn (and func_groups) default to cast/recv shadows in worldspawn group */
	// Tr3B: func_statics should receive shadows too
	if(funcGroup || !Q_stricmp("func_static", classname) || mapEnt->mapEntityNum == 0)
	{
		//% Sys_Printf( "World:  %d\n", mapEnt->mapEntityNum );
		castShadows = WORLDSPAWN_CAST_SHADOWS;
		recvShadows = WORLDSPAWN_RECV_SHADOWS;
	}

	/* other entities don't cast any shadows, but recv worldspawn shadows */
	else
	{
		//% Sys_Printf( "Entity: %d\n", mapEnt->mapEntityNum );
		castShadows = ENTITY_CAST_SHADOWS;
		recvShadows = ENTITY_RECV_SHADOWS;
	}

	/* get explicit shadow flags */
	GetEntityShadowFlags(mapEnt, NULL, &castShadows, &recvShadows);

	/* vortex: added _ls key (short name of lightmapscale) */
	/* ydnar: get lightmap scaling value for this entity */
	lightmapScale = 0.0f;
	if(strcmp("", ValueForKey(mapEnt, "lightmapscale")) ||
	   strcmp("", ValueForKey(mapEnt, "_lightmapscale")) || strcmp("", ValueForKey(mapEnt, "_ls")))
	{
		/* get lightmap scale from entity */
		lightmapScale = FloatForKey(mapEnt, "lightmapscale");
		if(lightmapScale <= 0.0f)
			lightmapScale = FloatForKey(mapEnt, "_lightmapscale");
		if(lightmapScale <= 0.0f)
			lightmapScale = FloatForKey(mapEnt, "_ls");
		if(lightmapScale < 0.0f)
			lightmapScale = 0.0f;
		if(lightmapScale > 0.0f)
			Sys_Printf("Entity %d (%s) has lightmap scale of %.4f\n", mapEnt->mapEntityNum, classname, lightmapScale);
	}

	// Tr3B: removed cel shader support
	celShader = NULL;

	/* jal : entity based _shadeangle */
	shadeAngle = 0.0f;
	if(strcmp("", ValueForKey(mapEnt, "_shadeangle")))
		shadeAngle = FloatForKey(mapEnt, "_shadeangle");
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
		Sys_Printf("Entity %d (%s) has shading angle of %.4f\n", mapEnt->mapEntityNum, classname, shadeAngle);

	/* jal : entity based _samplesize */
	lightmapSampleSize = 0;
	if(strcmp("", ValueForKey(mapEnt, "_lightmapsamplesize")))
		lightmapSampleSize = IntForKey(mapEnt, "_lightmapsamplesize");
	else if(strcmp("", ValueForKey(mapEnt, "_samplesize")))
		lightmapSampleSize = IntForKey(mapEnt, "_samplesize");

	if(lightmapSampleSize < 0)
		lightmapSampleSize = 0;

	if(lightmapSampleSize > 0)
		Sys_Printf("Entity %d (%s) has lightmap sample size of %d\n", mapEnt->mapEntityNum, classname, lightmapSampleSize);

	/* attach stuff to everything in the entity */
	for(brush = mapEnt->brushes; brush != NULL; brush = brush->next)
	{
		brush->entityNum = mapEnt->mapEntityNum;
		brush->castShadows = castShadows;
		brush->recvShadows = recvShadows;
		brush->lightmapSampleSize = lightmapSampleSize;
		brush->lightmapScale = lightmapScale;
		brush->celShader = celShader;
		brush->shadeAngleDegrees = shadeAngle;
	}

	for(patch = mapEnt->patches; patch != NULL; patch = patch->next)
	{
		patch->entityNum = mapEnt->mapEntityNum;
		patch->castShadows = castShadows;
		patch->recvShadows = recvShadows;
		patch->lightmapSampleSize = lightmapSampleSize;
		patch->lightmapScale = lightmapScale;
		patch->celShader = celShader;
	}

	/* Tr3B: convert Doom3's func_static entities with custom models into misc_models */
	if(convertType == CONVERT_NOTHING && !Q_stricmp("func_static", classname) && !mapEnt->brushes && !mapEnt->patches && model[0] != '\0')
	{
		SetKeyValue(mapEnt, "classname", "misc_model");
	}

#if 0
	// HACK: we should support Doom3 style doors in engine code but get rid of them for now
	if(!Q_stricmp("func_door", classname) && !mapEnt->brushes && !mapEnt->patches && model[0] != '\0')
	{
		numEntities--;
		return qtrue;
	}
#endif

#if 0
	// HACK:
	if(!Q_stricmp("func_rotating", classname))// && !mapEnt->brushes && !mapEnt->patches && model[0] != '\0')
	{
		Sys_Printf(".\n");
		//numEntities--;
		//return qtrue;
	}
#endif

	/* ydnar: gs mods: set entity bounds */
	SetEntityBounds(mapEnt);

	/* ydnar: gs mods: load shader index map (equivalent to old terrain alphamap) */
	// Tr3B: moved up to here before func_static are merged into worldspawn
	LoadEntityIndexMap(mapEnt);

	// HACK: determine if this is a func_static that can be merged into worldspawn
	if(convertType == CONVERT_NOTHING && !Q_stricmp("func_static", classname) && name[0] != '\0' && model[0] != '\0' && !Q_stricmp(name, model))
	{
		brush_t        *brush;
		vec3_t          originNeg;

#if 1
		VectorNegate(mapEnt->origin, originNeg);
		AdjustBrushesForOrigin(mapEnt, originNeg);
#else
		AdjustBrushesForOrigin(mapEnt, mapEnt->origin);
#endif

		// NOTE: func_static entities should always contain detail brushes
		for(brush = mapEnt->brushes; brush != NULL; brush = brush->next)
		{
			if(!(brush->compileFlags & C_DETAIL) || !brush->detail)
			{
				c_detail++;
				c_structural--;
				brush->detail = qtrue;
				brush->compileFlags |= C_DETAIL;
			}
		}

#if 0
		if(!strcmp("0", ValueForKey(mapEnt, "solid")))
		{
			side_t         *s;

			for(brush = mapEnt->brushes; brush != NULL; brush = brush->next)
			{
				ApplySurfaceParm("nonsolid", &brush->contentFlags, NULL, &brush->compileFlags);

				/* set the content/compile flags for every side in the brush */
				for(i = 0, s = &brush->sides[0]; i < brush->numsides; i++, s++)
				{
					s = &brush->sides[i];
					if(s->shaderInfo == NULL)
						continue;

					ApplySurfaceParm("nonsolid", &s->contentFlags, &s->surfaceFlags, &s->compileFlags);
				}
			}
		}
#endif

		MoveBrushesToWorld(mapEnt);
		MovePatchesToWorld(mapEnt);

		c_mergedFuncStatics++;
		numEntities--;
		return qtrue;
	}

	// Tr3B: moved this up before SetEntityBounds() is called
	// move brushes and patches from world space into entity space
	if(mapEnt->origin[0] || mapEnt->origin[1] || mapEnt->origin[2])
	{
		if(mapEnt->brushes || mapEnt->patches)
		{
			if(convertType != CONVERT_QUAKE3 && (name[0] != '\0' && model[0] != '\0' && !Q_stricmp(name, model)))
			{
				AdjustBrushesForOrigin(mapEnt, vec3_origin);
				AdjustPatchesForOrigin(mapEnt, mapEnt->origin);
			}
			else
			{
				AdjustBrushesForOrigin(mapEnt, mapEnt->origin);
				AdjustPatchesForOrigin(mapEnt, mapEnt->origin);
			}
		}
	}

	/* group_info entities are just for editor grouping (fixme: leak!) */
	if(convertType == CONVERT_NOTHING && !Q_stricmp("group_info", classname))
	{
		numEntities--;
		return qtrue;
	}

	/* group entities are just for editor convenience, toss all brushes into worldspawn */
	if(convertType == CONVERT_NOTHING && funcGroup)
	{
		vec3_t          originNeg;

		// HACK: this is needed for Quake4 maps
		VectorNegate(mapEnt->origin, originNeg);
		AdjustBrushesForOrigin(mapEnt, originNeg);
		AdjustPatchesForOrigin(mapEnt, originNeg);

		// group entities should always contain detail brushes
#if 0
		if(!strcmp("1", ValueForKey(mapEnt, "detail")))
		{
			for(brush = mapEnt->brushes; brush != NULL; brush = brush->next)
			{
				if(!(brush->compileFlags & C_DETAIL) || !brush->detail)
				{
					c_detail++;
					c_structural--;
					brush->detail = qtrue;
					brush->compileFlags |= C_DETAIL;
				}
			}
		}
#endif

		MoveBrushesToWorld(mapEnt);
		MovePatchesToWorld(mapEnt);
		numEntities--;
		return qtrue;
	}

	// Tr3B: append temporary func_group containing the detail brushes
	if(convertDetailBrushesEntity.lastBrush != NULL)
	{
		mapEnt = &entities[numEntities];
		numEntities++;
		memcpy(mapEnt, &convertDetailBrushesEntity, sizeof(*mapEnt));

		memset(&convertDetailBrushesEntity, 0, sizeof(convertDetailBrushesEntity));

		// true entity numbering
		mapEnt->mapEntityNum = numMapEntities;
		numMapEntities++;
	}

	/* done */
	return qtrue;
}



/*
LoadMapFile()
loads a map file into a list of entities
*/

void LoadMapFile(char *filename, qboolean onlyLights)
{
	FILE           *file;
	brush_t        *b;
	int             oldNumEntities, numMapBrushes;


	/* note it */
	Sys_FPrintf(SYS_VRB, "--- LoadMapFile ---\n");
	Sys_Printf("Loading %s\n", filename);

	/* hack */
	file = SafeOpenRead(filename);
	fclose(file);

	/* load the map file */
	LoadScriptFile(filename, -1);

	/* setup */
	if(onlyLights)
		oldNumEntities = numEntities;
	else
		numEntities = 0;

	/* initial setup */
	numMapDrawSurfs = 0;
	c_detail = 0;
	g_bBrushPrimit = BPRIMIT_UNDEFINED;

	/* allocate a very large temporary brush for building the brushes as they are loaded */
	buildBrush = AllocBrush(MAX_BUILD_SIDES);

	/* parse the map file */
	while(ParseMapEntity(onlyLights));

	/* light loading */
	if(onlyLights)
	{
		/* emit some statistics */
		Sys_FPrintf(SYS_VRB, "%9d light entities\n", numEntities - oldNumEntities);
	}
	else
	{
		/* set map bounds */
		ClearBounds(mapMins, mapMaxs);
		for(b = entities[0].brushes; b; b = b->next)
		{
			AddPointToBounds(b->mins, mapMins, mapMaxs);
			AddPointToBounds(b->maxs, mapMins, mapMaxs);
		}

		/* get brush counts */
		numMapBrushes = CountBrushList(entities[0].brushes);
		if((float)c_detail / (float)numMapBrushes < 0.10f && numMapBrushes > 500)
			Sys_Printf("WARNING: Over 90 percent structural map detected. Compile time may be adversely affected.\n");

		/* emit some statistics */
		Sys_FPrintf(SYS_VRB, "%9d total world brushes\n", numMapBrushes);
		Sys_FPrintf(SYS_VRB, "%9d detail brushes\n", c_detail);
		Sys_FPrintf(SYS_VRB, "%9d patches\n", numMapPatches);
		Sys_FPrintf(SYS_VRB, "%9d boxbevels\n", c_boxbevels);
		Sys_FPrintf(SYS_VRB, "%9d edgebevels\n", c_edgebevels);
		Sys_FPrintf(SYS_VRB, "%9d merged func_static entities\n", c_mergedFuncStatics);
		Sys_FPrintf(SYS_VRB, "%9d entities\n", numEntities);
		Sys_FPrintf(SYS_VRB, "%9d planes\n", nummapplanes);
		Sys_Printf("%9d areaportals\n", c_areaportals);
		Sys_Printf("Size: %5.0f, %5.0f, %5.0f to %5.0f, %5.0f, %5.0f\n",
				   mapMins[0], mapMins[1], mapMins[2], mapMaxs[0], mapMaxs[1], mapMaxs[2]);

		/* write bogus map */
		if(fakemap)
			WriteBSPBrushMap("fakemap.map", entities[0].brushes);
	}
}
