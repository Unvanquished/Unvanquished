/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

// tr_decal.c - ydnar
// handles projection of decals (nee marks) onto brush model surfaces

#include "tr_local.h"

extern int r_numDecalProjectors;

struct decalVert_t
{
	vec3_t xyz;
	float  st[ 2 ];
};

/*
MakeTextureMatrix()
generates a texture projection matrix for a triangle
returns false if a texture matrix cannot be created
*/

using dvec3_t = double[3];

static bool MakeTextureMatrix( vec4_t texMat[ 2 ], vec4_t projection, decalVert_t *a, decalVert_t *b, decalVert_t *c )
{
	int     i, j;
	double  bb, s, t, d;
	dvec3_t pa, pb, pc;
	dvec3_t bary, origin, xyz;
	vec3_t  vecs[ 3 ], axis[ 3 ], lengths;

	/* project triangle onto plane of projection */
	d = DotProduct( a->xyz, projection ) - projection[ 3 ];
	VectorMA( a->xyz, -d, projection, pa );
	d = DotProduct( b->xyz, projection ) - projection[ 3 ];
	VectorMA( b->xyz, -d, projection, pb );
	d = DotProduct( c->xyz, projection ) - projection[ 3 ];
	VectorMA( c->xyz, -d, projection, pc );

	/* calculate barycentric basis for the triangle */
	bb = ( b->st[ 0 ] - a->st[ 0 ] ) * ( c->st[ 1 ] - a->st[ 1 ] ) - ( c->st[ 0 ] - a->st[ 0 ] ) * ( b->st[ 1 ] - a->st[ 1 ] );

	if ( fabs( bb ) < 0.00000001f )
	{
		return false;
	}

	/* calculate texture origin */
	s = 0.0f;
	t = 0.0f;
	bary[ 0 ] = ( ( b->st[ 0 ] - s ) * ( c->st[ 1 ] - t ) - ( c->st[ 0 ] - s ) * ( b->st[ 1 ] - t ) ) / bb;
	bary[ 1 ] = ( ( c->st[ 0 ] - s ) * ( a->st[ 1 ] - t ) - ( a->st[ 0 ] - s ) * ( c->st[ 1 ] - t ) ) / bb;
	bary[ 2 ] = ( ( a->st[ 0 ] - s ) * ( b->st[ 1 ] - t ) - ( b->st[ 0 ] - s ) * ( a->st[ 1 ] - t ) ) / bb;

	origin[ 0 ] = bary[ 0 ] * pa[ 0 ] + bary[ 1 ] * pb[ 0 ] + bary[ 2 ] * pc[ 0 ];
	origin[ 1 ] = bary[ 0 ] * pa[ 1 ] + bary[ 1 ] * pb[ 1 ] + bary[ 2 ] * pc[ 1 ];
	origin[ 2 ] = bary[ 0 ] * pa[ 2 ] + bary[ 1 ] * pb[ 2 ] + bary[ 2 ] * pc[ 2 ];

	/* calculate s vector */
	s = 1.0f;
	t = 0.0f;
	bary[ 0 ] = ( ( b->st[ 0 ] - s ) * ( c->st[ 1 ] - t ) - ( c->st[ 0 ] - s ) * ( b->st[ 1 ] - t ) ) / bb;
	bary[ 1 ] = ( ( c->st[ 0 ] - s ) * ( a->st[ 1 ] - t ) - ( a->st[ 0 ] - s ) * ( c->st[ 1 ] - t ) ) / bb;
	bary[ 2 ] = ( ( a->st[ 0 ] - s ) * ( b->st[ 1 ] - t ) - ( b->st[ 0 ] - s ) * ( a->st[ 1 ] - t ) ) / bb;

	xyz[ 0 ] = bary[ 0 ] * pa[ 0 ] + bary[ 1 ] * pb[ 0 ] + bary[ 2 ] * pc[ 0 ];
	xyz[ 1 ] = bary[ 0 ] * pa[ 1 ] + bary[ 1 ] * pb[ 1 ] + bary[ 2 ] * pc[ 1 ];
	xyz[ 2 ] = bary[ 0 ] * pa[ 2 ] + bary[ 1 ] * pb[ 2 ] + bary[ 2 ] * pc[ 2 ];

	VectorSubtract( xyz, origin, vecs[ 0 ] );

	/* calculate t vector */
	s = 0.0f;
	t = 1.0f;
	bary[ 0 ] = ( ( b->st[ 0 ] - s ) * ( c->st[ 1 ] - t ) - ( c->st[ 0 ] - s ) * ( b->st[ 1 ] - t ) ) / bb;
	bary[ 1 ] = ( ( c->st[ 0 ] - s ) * ( a->st[ 1 ] - t ) - ( a->st[ 0 ] - s ) * ( c->st[ 1 ] - t ) ) / bb;
	bary[ 2 ] = ( ( a->st[ 0 ] - s ) * ( b->st[ 1 ] - t ) - ( b->st[ 0 ] - s ) * ( a->st[ 1 ] - t ) ) / bb;

	xyz[ 0 ] = bary[ 0 ] * pa[ 0 ] + bary[ 1 ] * pb[ 0 ] + bary[ 2 ] * pc[ 0 ];
	xyz[ 1 ] = bary[ 0 ] * pa[ 1 ] + bary[ 1 ] * pb[ 1 ] + bary[ 2 ] * pc[ 1 ];
	xyz[ 2 ] = bary[ 0 ] * pa[ 2 ] + bary[ 1 ] * pb[ 2 ] + bary[ 2 ] * pc[ 2 ];

	VectorSubtract( xyz, origin, vecs[ 1 ] );

	/* calculate r vector */
	VectorScale( projection, -1.0f, vecs[ 2 ] );

	/* calculate transform axis */
	for ( i = 0; i < 3; i++ )
	{
		lengths[ i ] = VectorNormalize2( vecs[ i ], axis[ i ] );
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			texMat[ i ][ j ] = lengths[ i ] > 0.0f ? ( axis[ i ][ j ] / lengths[ i ] ) : 0.0f;
		}
	}

	texMat[ 0 ][ 3 ] = a->st[ 0 ] - DotProduct( pa, texMat[ 0 ] );
	texMat[ 1 ][ 3 ] = a->st[ 1 ] - DotProduct( pa, texMat[ 1 ] );

	/* disco */
	return true;
}

/*
RE_ProjectDecal()
creates a new decal projector from a triangle
projected polygons should be 3 or 4 points
if a single point is passed in (numPoints == 1) then the decal will be omnidirectional
omnidirectional decals use points[ 0 ] as center and projection[ 3 ] as radius
pass in lifeTime < 0 for a temporary mark
*/

void RE_ProjectDecal( qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, const Color::Color& color, int lifeTime,
                      int fadeTime )
{
	int              i;
	float            radius, iDist;
	vec3_t           xyz;
	vec4_t           omniProjection;
	decalVert_t      dv[ 4 ];
	decalProjector_t *dp, temp;

	/* first frame rendered does not have a valid decals list */
	if ( tr.refdef.decalProjectors == nullptr )
	{
		return;
	}

	/* dummy check */
	if ( numPoints != 1 && numPoints != 3 && numPoints != 4 )
	{
		ri.Printf( printParm_t::PRINT_WARNING, "WARNING: Invalid number of decal points (%d)\n", numPoints );
		return;
	}

	/* early outs */
	if ( lifeTime == 0 )
	{
		return;
	}

	if ( projection[ 3 ] <= 0.0f )
	{
		return;
	}

	/* set times properly */
	if ( lifeTime < 0 || fadeTime < 0 )
	{
		lifeTime = 0;
		fadeTime = 0;
	}

	/* basic setup */
	temp.shader = R_GetShaderByHandle( hShader );  /* debug code */
	temp.numPlanes = temp.shader->entityMergable;
	temp.color = color;
	temp.numPlanes = numPoints + 2;
	temp.fadeStartTime = tr.refdef.time + lifeTime - fadeTime;
	temp.fadeEndTime = temp.fadeStartTime + fadeTime;

	/* set up decal texcoords (fixme: support arbitrary projector st coordinates in trapcall) */
	dv[ 0 ].st[ 0 ] = 0.0f;
	dv[ 0 ].st[ 1 ] = 0.0f;
	dv[ 1 ].st[ 0 ] = 0.0f;
	dv[ 1 ].st[ 1 ] = 1.0f;
	dv[ 2 ].st[ 0 ] = 1.0f;
	dv[ 2 ].st[ 1 ] = 1.0f;
	dv[ 3 ].st[ 0 ] = 1.0f;
	dv[ 3 ].st[ 1 ] = 0.0f;

	/* omnidirectional? */
	if ( numPoints == 1 )
	{
		/* set up omnidirectional */
		numPoints = 4;
		temp.numPlanes = 6;
		temp.omnidirectional = true;
		radius = projection[ 3 ];
		Vector4Set( omniProjection, 0.0f, 0.0f, -1.0f, radius * 2.0f );
		projection = omniProjection;
		iDist = 1.0f / ( radius * 2.0f );

		/* set corner */
		VectorSet( xyz, points[ 0 ][ 0 ] - radius, points[ 0 ][ 1 ] - radius, points[ 0 ][ 2 ] + radius );

		/* make x axis texture matrix (yz) */
		VectorSet( temp.texMat[ 0 ][ 0 ], 0.0f, iDist, 0.0f );
		temp.texMat[ 0 ][ 0 ][ 3 ] = -DotProduct( temp.texMat[ 0 ][ 0 ], xyz );
		VectorSet( temp.texMat[ 0 ][ 1 ], 0.0f, 0.0f, iDist );
		temp.texMat[ 0 ][ 1 ][ 3 ] = -DotProduct( temp.texMat[ 0 ][ 1 ], xyz );

		/* make y axis texture matrix (xz) */
		VectorSet( temp.texMat[ 1 ][ 0 ], iDist, 0.0f, 0.0f );
		temp.texMat[ 1 ][ 0 ][ 3 ] = -DotProduct( temp.texMat[ 1 ][ 0 ], xyz );
		VectorSet( temp.texMat[ 1 ][ 1 ], 0.0f, 0.0f, iDist );
		temp.texMat[ 1 ][ 1 ][ 3 ] = -DotProduct( temp.texMat[ 1 ][ 1 ], xyz );

		/* make z axis texture matrix (xy) */
		VectorSet( temp.texMat[ 2 ][ 0 ], iDist, 0.0f, 0.0f );
		temp.texMat[ 2 ][ 0 ][ 3 ] = -DotProduct( temp.texMat[ 2 ][ 0 ], xyz );
		VectorSet( temp.texMat[ 2 ][ 1 ], 0.0f, iDist, 0.0f );
		temp.texMat[ 2 ][ 1 ][ 3 ] = -DotProduct( temp.texMat[ 2 ][ 1 ], xyz );

		/* setup decal points */
		VectorSet( dv[ 0 ].xyz, points[ 0 ][ 0 ] - radius, points[ 0 ][ 1 ] - radius, points[ 0 ][ 2 ] + radius );
		VectorSet( dv[ 1 ].xyz, points[ 0 ][ 0 ] - radius, points[ 0 ][ 1 ] + radius, points[ 0 ][ 2 ] + radius );
		VectorSet( dv[ 2 ].xyz, points[ 0 ][ 0 ] + radius, points[ 0 ][ 1 ] + radius, points[ 0 ][ 2 ] + radius );
		VectorSet( dv[ 3 ].xyz, points[ 0 ][ 0 ] + radius, points[ 0 ][ 1 ] - radius, points[ 0 ][ 2 ] + radius );
	}
	else
	{
		/* set up unidirectional */
		temp.omnidirectional = false;

		/* set up decal points */
		VectorCopy( points[ 0 ], dv[ 0 ].xyz );
		VectorCopy( points[ 1 ], dv[ 1 ].xyz );
		VectorCopy( points[ 2 ], dv[ 2 ].xyz );
		VectorCopy( points[ 3 ], dv[ 3 ].xyz );

		/* make texture matrix */
		if ( !MakeTextureMatrix( temp.texMat[ 0 ], projection, &dv[ 0 ], &dv[ 1 ], &dv[ 2 ] ) )
		{
			return;
		}
	}

	/* bound the projector */
	ClearBounds( temp.mins, temp.maxs );

	for ( i = 0; i < numPoints; i++ )
	{
		AddPointToBounds( dv[ i ].xyz, temp.mins, temp.maxs );
		VectorMA( dv[ i ].xyz, projection[ 3 ], projection, xyz );
		AddPointToBounds( xyz, temp.mins, temp.maxs );
	}

	/* make bounding sphere */
	VectorAdd( temp.mins, temp.maxs, temp.center );
	VectorScale( temp.center, 0.5f, temp.center );
	VectorSubtract( temp.maxs, temp.center, xyz );
	temp.radius = VectorLength( xyz );
	temp.radius2 = temp.radius * temp.radius;

	/* frustum cull the projector (fixme: this uses a stale frustum!) */
	if ( R_CullPointAndRadius( temp.center, temp.radius ) == cullResult_t::CULL_OUT )
	{
		return;
	}

	/* make the front plane */
	if ( !PlaneFromPoints( temp.planes[ 0 ], dv[ 0 ].xyz, dv[ 1 ].xyz, dv[ 2 ].xyz ) )
	{
		return;
	}

	/* make the back plane */
	VectorSubtract( vec3_origin, temp.planes[ 0 ], temp.planes[ 1 ] );
	VectorMA( dv[ 0 ].xyz, projection[ 3 ], projection, xyz );
	temp.planes[ 1 ][ 3 ] = DotProduct( xyz, temp.planes[ 1 ] );

	/* make the side planes */
	for ( i = 0; i < numPoints; i++ )
	{
		VectorMA( dv[ i ].xyz, projection[ 3 ], projection, xyz );

		if ( !PlaneFromPoints( temp.planes[ i + 2 ], dv[( i + 1 ) % numPoints ].xyz, dv[ i ].xyz, xyz ) )
		{
			return;
		}
	}

	/* create a new projector */
	dp = &tr.refdef.decalProjectors[ r_numDecalProjectors & DECAL_PROJECTOR_MASK ];
	Com_Memcpy( dp, &temp, sizeof( *dp ) );

	/* we have a winner */
	r_numDecalProjectors++;
}

/*
RE_ClearDecals()
clears decals from the world and entities
*/

void RE_ClearDecals()
{
	int i, j;

	/* dummy check */
	if ( tr.world == nullptr || tr.world->numModels <= 0 )
	{
		return;
	}

	/* clear world decals */
	for ( j = 0; j < MAX_WORLD_DECALS; j++ )
	{
		tr.world->models[ 0 ].decals[ j ].shader = nullptr;
	}

	/* clear entity decals */
	for ( i = 0; i < tr.world->numModels; i++ )
	{
		for ( j = 0; j < MAX_ENTITY_DECALS; j++ )
		{
			tr.world->models[ i ].decals[ j ].shader = nullptr;
		}
	}
}

/*
R_TestDecalBoundingBox()
return true if the decal projector intersects the bounding box
*/

bool R_TestDecalBoundingBox( decalProjector_t *dp, vec3_t mins, vec3_t maxs )
{
	if ( mins[ 0 ] >= ( dp->center[ 0 ] + dp->radius ) || maxs[ 0 ] <= ( dp->center[ 0 ] - dp->radius ) ||
	     mins[ 1 ] >= ( dp->center[ 1 ] + dp->radius ) || maxs[ 1 ] <= ( dp->center[ 1 ] - dp->radius ) ||
	     mins[ 2 ] >= ( dp->center[ 2 ] + dp->radius ) || maxs[ 2 ] <= ( dp->center[ 2 ] - dp->radius ) )
	{
		return false;
	}

	return true;
}

/*
R_TestDecalBoundingSphere()
return true if the decal projector intersects the bounding sphere
*/

bool R_TestDecalBoundingSphere( decalProjector_t *dp, vec3_t center, float radius2 )
{
	vec3_t delta;
	float  distance2;

	VectorSubtract( center, dp->center, delta );
	distance2 = DotProduct( delta, delta );

	if ( distance2 >= ( radius2 + dp->radius2 ) )
	{
		return false;
	}

	return true;
}

/*
ChopWindingBehindPlane()
clips a winding to the fragment behind the plane
*/

#define SIDE_FRONT 0
#define SIDE_BACK  1
#define SIDE_ON    2

static void ChopWindingBehindPlane( int numInPoints, vec3_t inPoints[ MAX_DECAL_VERTS ],
                                    int *numOutPoints, vec3_t outPoints[ MAX_DECAL_VERTS ], vec4_t plane, vec_t epsilon )
{
	int   i, j;
	float dot;
	float *p1, *p2, *clip;
	float d;
	float dists[ MAX_DECAL_VERTS + 4 ];
	int   sides[ MAX_DECAL_VERTS + 4 ];
	int   counts[ 3 ];

	/* set initial count */
	*numOutPoints = 0;

	/* don't clip if it might overflow */
	if ( numInPoints >= MAX_DECAL_VERTS - 1 )
	{
		return;
	}

	/* determine sides for each point */
	counts[ SIDE_FRONT ] = 0;
	counts[ SIDE_BACK ] = 0;
	counts[ SIDE_ON ] = 0;

	for ( i = 0; i < numInPoints; i++ )
	{
		dists[ i ] = DotProduct( inPoints[ i ], plane ) - plane[ 3 ];

		if ( dists[ i ] > epsilon )
		{
			sides[ i ] = SIDE_FRONT;
		}
		else if ( dists[ i ] < -epsilon )
		{
			sides[ i ] = SIDE_BACK;
		}
		else
		{
			sides[ i ] = SIDE_ON;
		}

		counts[ sides[ i ] ]++;
	}

	sides[ i ] = sides[ 0 ];
	dists[ i ] = dists[ 0 ];

	/* all points on front */
	if ( counts[ SIDE_BACK ] == 0 )
	{
		return;
	}

	/* all points on back */
	if ( counts[ SIDE_FRONT ] == 0 )
	{
		*numOutPoints = numInPoints;
		Com_Memcpy( outPoints, inPoints, numInPoints * sizeof( vec3_t ) );
		return;
	}

	/* clip the winding */
	for ( i = 0; i < numInPoints; i++ )
	{
		p1 = inPoints[ i ];
		clip = outPoints[ *numOutPoints ];

		if ( sides[ i ] == SIDE_ON || sides[ i ] == SIDE_BACK )
		{
			VectorCopy( p1, clip );
			( *numOutPoints ) ++;
		}

		if ( sides[ i + 1 ] == SIDE_ON || sides[ i + 1 ] == sides[ i ] )
		{
			continue;
		}

		/* generate a split point */
		p2 = inPoints[( i + 1 ) % numInPoints ];

		d = dists[ i ] - dists[ i + 1 ];

		if ( d == 0 )
		{
			dot = 0;
		}
		else
		{
			dot = dists[ i ] / d;
		}

		/* clip xyz */
		clip = outPoints[ *numOutPoints ];

		for ( j = 0; j < 3; j++ )
		{
			clip[ j ] = p1[ j ] + dot * ( p2[ j ] - p1[ j ] );
		}

		( *numOutPoints ) ++;
	}
}

/*
ProjectDecalOntoWinding()
projects decal onto a polygon
*/

static void ProjectDecalOntoWinding( decalProjector_t *dp, int numPoints, vec3_t points[ 2 ][ MAX_DECAL_VERTS ], bspSurface_t *surf,
                                     bspModel_t *bmodel )
{
	int        i, pingPong, count, axis;
	float      pd, d, d2, alpha = 1.f;
	vec4_t     plane;
	vec3_t     absNormal;
	decal_t    *decal, *oldest;
	polyVert_t *vert;

	/* make a plane from the winding */
	if ( !PlaneFromPoints( plane, points[ 0 ][ 0 ], points[ 0 ][ 1 ], points[ 0 ][ 2 ] ) )
	{
		return;
	}

	/* omnidirectional projectors need plane type */
	if ( dp->omnidirectional )
	{
		/* compiler warnings be gone */
		pd = 1.0f;

		/* fade by distance from plane */
		d = DotProduct( dp->center, plane ) - plane[ 3 ];
		alpha = 1.0f - ( fabs( d ) / dp->radius );

		if ( alpha < 0.0f )
		{
			return;
		}

		if ( alpha > 1.0f )
		{
			alpha = 1.0f;
		}

		/* set projection axis */
		absNormal[ 0 ] = fabs( plane[ 0 ] );
		absNormal[ 1 ] = fabs( plane[ 1 ] );
		absNormal[ 2 ] = fabs( plane[ 2 ] );

		if ( absNormal[ 2 ] >= absNormal[ 0 ] && absNormal[ 2 ] >= absNormal[ 1 ] )
		{
			axis = 2;
		}
		else if ( absNormal[ 0 ] >= absNormal[ 1 ] && absNormal[ 0 ] >= absNormal[ 2 ] )
		{
			axis = 0;
		}
		else
		{
			axis = 1;
		}
	}
	else
	{
		/* backface check */
		pd = DotProduct( dp->planes[ 0 ], plane );

		if ( pd < -0.0001f )
		{
			return;
		}

		/* directional decals use first texture matrix */
		axis = 0;
	}

	/* chop the winding by all the projector planes */
	pingPong = 0;

	for ( i = 0; i < dp->numPlanes; i++ ) //%    dp->numPlanes
	{
		ChopWindingBehindPlane( numPoints, points[ pingPong ], &numPoints, points[ !pingPong ], dp->planes[ i ], 0.0f );
		pingPong ^= 1;

		if ( numPoints < 3 )
		{
			return;
		}

		if ( numPoints == MAX_DECAL_VERTS )
		{
			break;
		}
	}

	/* find first free decal (fixme: optimize this) */
	count = ( bmodel == tr.world->models ? MAX_WORLD_DECALS : MAX_ENTITY_DECALS );
	oldest = &bmodel->decals[ 0 ];
	decal = bmodel->decals;

	for ( i = 0; i < count; i++, decal++ )
	{
		/* try to find an empty decal slot */
		if ( decal->shader == nullptr )
		{
			break;
		}

		/* find oldest decal */
		if ( decal->fadeEndTime < oldest->fadeEndTime )
		{
			oldest = decal;
		}
	}

	/* guess we have to use the oldest decal */
	if ( i >= count )
	{
		decal = oldest;
	}

	/* r_speeds useful info */
	tr.pc.c_decalSurfacesCreated++;

	/* set it up (fixme: get the shader before this happens) */
	decal->parent = surf;
	decal->shader = dp->shader;
	decal->fadeStartTime = dp->fadeStartTime;
	decal->fadeEndTime = dp->fadeEndTime;
	decal->fogIndex = surf->fogIndex;

	/* add points */
	decal->numVerts = numPoints;
	vert = decal->verts;

	for ( i = 0; i < numPoints; i++, vert++ )
	{
		/* set xyz */
		VectorCopy( points[ pingPong ][ i ], vert->xyz );

		/* set st */
		vert->st[ 0 ] = DotProduct( vert->xyz, dp->texMat[ axis ][ 0 ] ) + dp->texMat[ axis ][ 0 ][ 3 ];
		vert->st[ 1 ] = DotProduct( vert->xyz, dp->texMat[ axis ][ 1 ] ) + dp->texMat[ axis ][ 1 ][ 3 ];

		/* unidirectional decals fade by half distance from front->back planes */
		if ( !dp->omnidirectional )
		{
			/* set alpha */
			d = DotProduct( vert->xyz, dp->planes[ 0 ] ) - dp->planes[ 0 ][ 3 ];
			d2 = DotProduct( vert->xyz, dp->planes[ 1 ] ) - dp->planes[ 1 ][ 3 ];
			alpha = 2.0f * d2 / ( d + d2 );

			if ( alpha > 1.0f )
			{
				alpha = 1.0f;
			}
			else if ( alpha < 0.0f )
			{
				alpha = 0.0f;
			}
		}

		/* set color */
		( dp->color * pd * alpha ).ToArray( vert->modulate );
		vert->modulate[ 3 ] = alpha * dp->color.Alpha();
	}
}

/*
ProjectDecalOntoTriangles()
projects a decal onto a triangle surface (brush faces, misc_models, metasurfaces)
*/

static void ProjectDecalOntoTriangles( decalProjector_t *dp, bspSurface_t *surf, bspModel_t *bmodel )
{
	int           i;
	srfTriangle_t *tri;
	vec3_t        points[ 2 ][ MAX_DECAL_VERTS ];

	if ( *surf->data == surfaceType_t::SF_FACE )
	{
		/* get surface */
		srfSurfaceFace_t *srf = ( srfSurfaceFace_t * ) surf->data;

		/* walk triangle list */
		for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
		{
			/* make triangle */
			VectorCopy( srf->verts[ tri->indexes[ 0 ] ].xyz, points[ 0 ][ 0 ] );
			VectorCopy( srf->verts[ tri->indexes[ 1 ] ].xyz, points[ 0 ][ 1 ] );
			VectorCopy( srf->verts[ tri->indexes[ 2 ] ].xyz, points[ 0 ][ 2 ] );

			/* chop it */
			ProjectDecalOntoWinding( dp, 3, points, surf, bmodel );
		}
	}
	else if ( *surf->data == surfaceType_t::SF_TRIANGLES )
	{
		/* get surface */
		srfTriangles_t *srf = ( srfTriangles_t * ) surf->data;

		/* walk triangle list */
		for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
		{
			/* make triangle */
			VectorCopy( srf->verts[ tri->indexes[ 0 ] ].xyz, points[ 0 ][ 0 ] );
			VectorCopy( srf->verts[ tri->indexes[ 1 ] ].xyz, points[ 0 ][ 1 ] );
			VectorCopy( srf->verts[ tri->indexes[ 2 ] ].xyz, points[ 0 ][ 2 ] );

			/* chop it */
			ProjectDecalOntoWinding( dp, 3, points, surf, bmodel );
		}
	}
}

/*
ProjectDecalOntoGrid()
projects a decal onto a grid (patch) surface
*/

static void ProjectDecalOntoGrid( decalProjector_t *dp, bspSurface_t *surf, bspModel_t *bmodel )
{
	int           x, y;
	srfGridMesh_t *srf;
	srfVert_t     *dv;
	vec3_t        points[ 2 ][ MAX_DECAL_VERTS ];

	/* get surface */
	srf = ( srfGridMesh_t * ) surf->data;

	/* walk mesh rows */
	for ( y = 0; y < ( srf->height - 1 ); y++ )
	{
		/* walk mesh cols */
		for ( x = 0; x < ( srf->width - 1 ); x++ )
		{
			/* get vertex */
			dv = srf->verts + y * srf->width + x;

			/* first triangle */
			VectorCopy( dv[ 0 ].xyz, points[ 0 ][ 0 ] );
			VectorCopy( dv[ srf->width ].xyz, points[ 0 ][ 1 ] );
			VectorCopy( dv[ 1 ].xyz, points[ 0 ][ 2 ] );
			ProjectDecalOntoWinding( dp, 3, points, surf, bmodel );

			/* second triangle */
			VectorCopy( dv[ 1 ].xyz, points[ 0 ][ 0 ] );
			VectorCopy( dv[ srf->width ].xyz, points[ 0 ][ 1 ] );
			VectorCopy( dv[ srf->width + 1 ].xyz, points[ 0 ][ 2 ] );
			ProjectDecalOntoWinding( dp, 3, points, surf, bmodel );
		}
	}
}

/*
R_ProjectDecalOntoSurface()
projects a decal onto a world surface
*/

void R_ProjectDecalOntoSurface( decalProjector_t *dp, bspSurface_t *surf, bspModel_t *bmodel )
{
	float        d;
	srfGeneric_t *gen;

	/* early outs */
	if ( dp->shader == nullptr )
	{
		return;
	}

	//% if( surf->viewCount == tr.viewCount )
	//%     return;
	if ( ( surf->shader->surfaceFlags & ( SURF_NOIMPACT | SURF_NOMARKS ) ) || ( surf->shader->contentFlags & CONTENTS_FOG ) )
	{
		return;
	}

	/* add to counts */
	tr.pc.c_decalTestSurfaces++;

	/* get generic surface */
	gen = ( srfGeneric_t * ) surf->data;

	/* ignore certain surfacetypes */
	if ( gen->surfaceType != surfaceType_t::SF_FACE && gen->surfaceType != surfaceType_t::SF_TRIANGLES && gen->surfaceType != surfaceType_t::SF_GRID )
	{
		return;
	}

	/* test bounding sphere */
	if ( !R_TestDecalBoundingSphere( dp, gen->origin, ( gen->radius * gen->radius ) ) )
	{
		return;
	}

	/* planar surface */
	if ( gen->surfaceType == surfaceType_t::SF_FACE )
	{
		srfSurfaceFace_t *srf = ( srfSurfaceFace_t * )surf->data;

		if ( srf->plane.normal[ 0 ] || srf->plane.normal[ 1 ] || srf->plane.normal[ 2 ] )
		{
			/* backface check */
			d = DotProduct( dp->planes[ 0 ], srf->plane.normal );

			if ( d < -0.0001 )
			{
				return;
			}

			/* plane-sphere check */
			d = DotProduct( dp->center, srf->plane.normal ) - srf->plane.dist;

			if ( fabs( d ) >= dp->radius )
			{
				return;
			}
		}
	}

	/* add to counts */
	tr.pc.c_decalClipSurfaces++;

	/* switch on type */
	switch ( gen->surfaceType )
	{
		case surfaceType_t::SF_FACE:
		case surfaceType_t::SF_TRIANGLES:
			ProjectDecalOntoTriangles( dp, surf, bmodel );
			break;

		case surfaceType_t::SF_GRID:
			ProjectDecalOntoGrid( dp, surf, bmodel );
			break;

		default:
			break;
	}
}

/*
AddDecalSurface()
adds a decal surface to the scene
*/

void R_AddDecalSurface( decal_t *decal )
{
	int        i;
	float      fade;
	srfDecal_t *srf;

	/* early outs */
	if ( decal->shader == nullptr || decal->parent->viewCount != tr.viewCountNoReset || tr.refdef.numDecals >= MAX_DECALS )
	{
		return;
	}

	/* get decal surface */
	srf = &tr.refdef.decals[ tr.refdef.numDecals ];
	tr.refdef.numDecals++;

	/* set it up */
	srf->surfaceType = surfaceType_t::SF_DECAL;
	srf->numVerts = decal->numVerts;
	Com_Memcpy( srf->verts, decal->verts, srf->numVerts * sizeof( *srf->verts ) );

	/* fade colors */
	if ( decal->fadeStartTime < tr.refdef.time && decal->fadeStartTime < decal->fadeEndTime )
	{
		fade = ( float )( decal->fadeEndTime - tr.refdef.time ) / ( float )( decal->fadeEndTime - decal->fadeStartTime );

		for ( i = 0; i < decal->numVerts; i++ )
		{
			decal->verts[ i ].modulate[ 0 ] *= fade;
			decal->verts[ i ].modulate[ 1 ] *= fade;
			decal->verts[ i ].modulate[ 2 ] *= fade;
			decal->verts[ i ].modulate[ 3 ] *= fade;
		}
	}

	/* add surface to scene */
	R_AddDrawSurf( ( surfaceType_t * ) srf, decal->shader, -1, decal->fogIndex );
	tr.pc.c_decalSurfaces++;

	/* free temporary decal */
	if ( decal->fadeEndTime <= tr.refdef.time )
	{
		decal->shader = nullptr;
	}
}

/*
R_AddDecalSurfaces()
adds decal surfaces to the scene
*/

void R_AddDecalSurfaces( bspModel_t *bmodel )
{
	int     i, count;
	decal_t *decal;

	/* get decal count */
	count = ( bmodel == tr.world->models ? MAX_WORLD_DECALS : MAX_ENTITY_DECALS );

	/* iterate through decals */
	decal = bmodel->decals;

	for ( i = 0; i < count; i++, decal++ )
	{
		R_AddDecalSurface( decal );
	}
}

/*
R_CullDecalProjectors()
frustum culls decal projector list
*/

void R_CullDecalProjectors()
{
	int              i, numDecalProjectors, decalBits;
	decalProjector_t *dp;

	/* limit */
	if ( tr.refdef.numDecalProjectors > MAX_DECAL_PROJECTORS )
	{
		tr.refdef.numDecalProjectors = MAX_DECAL_PROJECTORS;
	}

	/* walk decal projector list */
	numDecalProjectors = 0;
	decalBits = 0;

	for ( i = 0, dp = tr.refdef.decalProjectors; i < tr.refdef.numDecalProjectors; i++, dp++ )
	{
		if ( R_CullPointAndRadius( dp->center, dp->radius ) == cullResult_t::CULL_OUT )
		{
			dp->shader = nullptr;
		}
		else
		{
			numDecalProjectors = i + 1;
			decalBits |= ( 1 << i );
		}
	}

	/* reset count */
	tr.refdef.numDecalProjectors = numDecalProjectors;
	tr.pc.c_decalProjectors = numDecalProjectors;

	/* set bits */
	tr.refdef.decalBits = decalBits;
}
