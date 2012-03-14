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

#include "cm_local.h"
#include "cm_patch.h"

int                     c_totalPatchBlocks;
int                     c_totalPatchSurfaces;
int                     c_totalPatchEdges;

const cSurfaceCollide_t *debugSurfaceCollide;
const cFacet_t          *debugFacet;
qboolean                debugBlock;
vec3_t                  debugBlockPoints[ 4 ];

/*
=================
CM_ClearLevelPatches
=================
*/
void CM_ClearLevelPatches( void )
{
	debugSurfaceCollide = NULL;
	debugFacet = NULL;
}

/*
=================
CM_SignbitsForNormal
=================
*/
static int CM_SignbitsForNormal( vec3_t normal )
{
	int bits, j;

	bits = 0;

	for( j = 0; j < 3; j++ )
	{
		if( normal[ j ] < 0 )
		{
			bits |= 1 << j;
		}
	}

	return bits;
}

/*
=====================
CM_PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
static qboolean CM_PlaneFromPoints( vec4_t plane, vec3_t a, vec3_t b, vec3_t c )
{
	vec3_t d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane );

	if( VectorNormalize( plane ) == 0 )
	{
		return qfalse;
	}

	plane[ 3 ] = DotProduct( a, plane );
	return qtrue;
}

/*
================================================================================

GRID SUBDIVISION

================================================================================
*/

/*
=================
CM_NeedsSubdivision

Returns true if the given quadratic curve is not flat enough for our
collision detection purposes
=================
*/
static qboolean CM_NeedsSubdivision( vec3_t a, vec3_t b, vec3_t c )
{
	vec3_t cmid;
	vec3_t lmid;
	vec3_t delta;
	float  dist;
	int    i;

	// calculate the linear midpoint
	for( i = 0; i < 3; i++ )
	{
		lmid[ i ] = 0.5 * ( a[ i ] + c[ i ] );
	}

	// calculate the exact curve midpoint
	for( i = 0; i < 3; i++ )
	{
		cmid[ i ] = 0.5 * ( 0.5 * ( a[ i ] + b[ i ] ) + 0.5 * ( b[ i ] + c[ i ] ) );
	}

	// see if the curve is far enough away from the linear mid
	VectorSubtract( cmid, lmid, delta );
	dist = VectorLength( delta );

	return dist >= SUBDIVIDE_DISTANCE;
}

/*
===============
CM_Subdivide

a, b, and c are control points.
the subdivided sequence will be: a, out1, out2, out3, c
===============
*/
static void CM_Subdivide( vec3_t a, vec3_t b, vec3_t c, vec3_t out1, vec3_t out2, vec3_t out3 )
{
	int i;

	for( i = 0; i < 3; i++ )
	{
		out1[ i ] = 0.5 * ( a[ i ] + b[ i ] );
		out3[ i ] = 0.5 * ( b[ i ] + c[ i ] );
		out2[ i ] = 0.5 * ( out1[ i ] + out3[ i ] );
	}
}

/*
=================
CM_TransposeGrid

Swaps the rows and columns in place
=================
*/
static void CM_TransposeGrid( cGrid_t *grid )
{
	int      i, j, l;
	vec3_t   temp;
	qboolean tempWrap;

	if( grid->width > grid->height )
	{
		for( i = 0; i < grid->height; i++ )
		{
			for( j = i + 1; j < grid->width; j++ )
			{
				if( j < grid->height )
				{
					// swap the value
					VectorCopy( grid->points[ i ][ j ], temp );
					VectorCopy( grid->points[ j ][ i ], grid->points[ i ][ j ] );
					VectorCopy( temp, grid->points[ j ][ i ] );
				}
				else
				{
					// just copy
					VectorCopy( grid->points[ j ][ i ], grid->points[ i ][ j ] );
				}
			}
		}
	}
	else
	{
		for( i = 0; i < grid->width; i++ )
		{
			for( j = i + 1; j < grid->height; j++ )
			{
				if( j < grid->width )
				{
					// swap the value
					VectorCopy( grid->points[ j ][ i ], temp );
					VectorCopy( grid->points[ i ][ j ], grid->points[ j ][ i ] );
					VectorCopy( temp, grid->points[ i ][ j ] );
				}
				else
				{
					// just copy
					VectorCopy( grid->points[ i ][ j ], grid->points[ j ][ i ] );
				}
			}
		}
	}

	l = grid->width;
	grid->width = grid->height;
	grid->height = l;

	tempWrap = grid->wrapWidth;
	grid->wrapWidth = grid->wrapHeight;
	grid->wrapHeight = tempWrap;
}

/*
===================
CM_SetGridWrapWidth

If the left and right columns are exactly equal, set grid->wrapWidth qtrue
===================
*/
static void CM_SetGridWrapWidth( cGrid_t *grid )
{
	int   i, j;
	float d;

	for( i = 0; i < grid->height; i++ )
	{
		for( j = 0; j < 3; j++ )
		{
			d = grid->points[ 0 ][ i ][ j ] - grid->points[ grid->width - 1 ][ i ][ j ];

			if( d < -WRAP_POINT_EPSILON || d > WRAP_POINT_EPSILON )
			{
				break;
			}
		}

		if( j != 3 )
		{
			break;
		}
	}

	if( i == grid->height )
	{
		grid->wrapWidth = qtrue;
	}
	else
	{
		grid->wrapWidth = qfalse;
	}
}

/*
=================
CM_SubdivideGridColumns

Adds columns as necessary to the grid until
all the aproximating points are within SUBDIVIDE_DISTANCE
from the true curve
=================
*/
static void CM_SubdivideGridColumns( cGrid_t *grid )
{
	int i, j, k;

	for( i = 0; i < grid->width - 2; )
	{
		// grid->points[i][x] is an interpolating control point
		// grid->points[i+1][x] is an aproximating control point
		// grid->points[i+2][x] is an interpolating control point

		//
		// first see if we can collapse the aproximating collumn away
		//
		for( j = 0; j < grid->height; j++ )
		{
			if( CM_NeedsSubdivision( grid->points[ i ][ j ], grid->points[ i + 1 ][ j ], grid->points[ i + 2 ][ j ] ) )
			{
				break;
			}
		}

		if( j == grid->height )
		{
			// all of the points were close enough to the linear midpoints
			// that we can collapse the entire column away
			for( j = 0; j < grid->height; j++ )
			{
				// remove the column
				for( k = i + 2; k < grid->width; k++ )
				{
					VectorCopy( grid->points[ k ][ j ], grid->points[ k - 1 ][ j ] );
				}
			}

			grid->width--;

			// go to the next curve segment
			i++;
			continue;
		}

		//
		// we need to subdivide the curve
		//
		for( j = 0; j < grid->height; j++ )
		{
			vec3_t prev, mid, next;

			// save the control points now
			VectorCopy( grid->points[ i ][ j ], prev );
			VectorCopy( grid->points[ i + 1 ][ j ], mid );
			VectorCopy( grid->points[ i + 2 ][ j ], next );

			// make room for two additional columns in the grid
			// columns i+1 will be replaced, column i+2 will become i+4
			// i+1, i+2, and i+3 will be generated
			for( k = grid->width - 1; k > i + 1; k-- )
			{
				VectorCopy( grid->points[ k ][ j ], grid->points[ k + 2 ][ j ] );
			}

			// generate the subdivided points
			CM_Subdivide( prev, mid, next, grid->points[ i + 1 ][ j ], grid->points[ i + 2 ][ j ], grid->points[ i + 3 ][ j ] );
		}

		grid->width += 2;

		// the new aproximating point at i+1 may need to be removed
		// or subdivided farther, so don't advance i
	}
}

/*
======================
CM_ComparePoints
======================
*/
#define POINT_EPSILON 0.1
static qboolean CM_ComparePoints( float *a, float *b )
{
	float d;

	d = a[ 0 ] - b[ 0 ];

	if( d < -POINT_EPSILON || d > POINT_EPSILON )
	{
		return qfalse;
	}

	d = a[ 1 ] - b[ 1 ];

	if( d < -POINT_EPSILON || d > POINT_EPSILON )
	{
		return qfalse;
	}

	d = a[ 2 ] - b[ 2 ];

	if( d < -POINT_EPSILON || d > POINT_EPSILON )
	{
		return qfalse;
	}

	return qtrue;
}

/*
=================
CM_RemoveDegenerateColumns

If there are any identical columns, remove them
=================
*/
static void CM_RemoveDegenerateColumns( cGrid_t *grid )
{
	int i, j, k;

	for( i = 0; i < grid->width - 1; i++ )
	{
		for( j = 0; j < grid->height; j++ )
		{
			if( !CM_ComparePoints( grid->points[ i ][ j ], grid->points[ i + 1 ][ j ] ) )
			{
				break;
			}
		}

		if( j != grid->height )
		{
			continue; // not degenerate
		}

		for( j = 0; j < grid->height; j++ )
		{
			// remove the column
			for( k = i + 2; k < grid->width; k++ )
			{
				VectorCopy( grid->points[ k ][ j ], grid->points[ k - 1 ][ j ] );
			}
		}

		grid->width--;

		// check against the next column
		i--;
	}
}

/*
================================================================================

PATCH COLLIDE GENERATION

================================================================================
*/

static int      numPlanes;
static cPlane_t planes[ MAX_PATCH_PLANES ];

static int      numFacets;
static cFacet_t facets[ MAX_PATCH_PLANES ]; //maybe MAX_FACETS ??

#define NORMAL_EPSILON 0.0001
#define DIST_EPSILON   0.02

/*
==================
CM_PlaneEqual
==================
*/
static int CM_PlaneEqual( cPlane_t *p, float plane[ 4 ], int *flipped )
{
	float invplane[ 4 ];

	if( fabs( p->plane[ 0 ] - plane[ 0 ] ) < NORMAL_EPSILON
	    && fabs( p->plane[ 1 ] - plane[ 1 ] ) < NORMAL_EPSILON
	    && fabs( p->plane[ 2 ] - plane[ 2 ] ) < NORMAL_EPSILON && fabs( p->plane[ 3 ] - plane[ 3 ] ) < DIST_EPSILON )
	{
		*flipped = qfalse;
		return qtrue;
	}

	VectorNegate( plane, invplane );
	invplane[ 3 ] = -plane[ 3 ];

	if( fabs( p->plane[ 0 ] - invplane[ 0 ] ) < NORMAL_EPSILON
	    && fabs( p->plane[ 1 ] - invplane[ 1 ] ) < NORMAL_EPSILON
	    && fabs( p->plane[ 2 ] - invplane[ 2 ] ) < NORMAL_EPSILON && fabs( p->plane[ 3 ] - invplane[ 3 ] ) < DIST_EPSILON )
	{
		*flipped = qtrue;
		return qtrue;
	}

	return qfalse;
}

/*
==================
CM_SnapVector
==================
*/
static void CM_SnapVector( vec3_t normal )
{
	int i;

	for( i = 0; i < 3; i++ )
	{
		if( fabs( normal[ i ] - 1 ) < NORMAL_EPSILON )
		{
			VectorClear( normal );
			normal[ i ] = 1;
			break;
		}

		if( fabs( normal[ i ] - -1 ) < NORMAL_EPSILON )
		{
			VectorClear( normal );
			normal[ i ] = -1;
			break;
		}
	}
}

/*
==================
CM_FindPlane2
==================
*/
static int CM_FindPlane2( float plane[ 4 ], int *flipped )
{
	int i;

	// see if the points are close enough to an existing plane
	for( i = 0; i < numPlanes; i++ )
	{
		if( CM_PlaneEqual( &planes[ i ], plane, flipped ) )
		{
			return i;
		}
	}

	// add a new plane
	if( numPlanes == MAX_PATCH_PLANES )
	{
		Com_Error( ERR_DROP, "CM_FindPlane2: MAX_PATCH_PLANES" );
	}

	Vector4Copy( plane, planes[ numPlanes ].plane );
	planes[ numPlanes ].signbits = CM_SignbitsForNormal( plane );

	numPlanes++;

	*flipped = qfalse;

	return numPlanes - 1;
}

/*
==================
CM_FindPlane
==================
*/
static int CM_FindPlane( float *p1, float *p2, float *p3 )
{
	float plane[ 4 ];
	int   i;
	float d;

	if( !CM_PlaneFromPoints( plane, p1, p2, p3 ) )
	{
		return -1;
	}

	// see if the points are close enough to an existing plane
	for( i = 0; i < numPlanes; i++ )
	{
		if( DotProduct( plane, planes[ i ].plane ) < 0 )
		{
			continue; // allow backwards planes?
		}

		d = DotProduct( p1, planes[ i ].plane ) - planes[ i ].plane[ 3 ];

		if( d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON )
		{
			continue;
		}

		d = DotProduct( p2, planes[ i ].plane ) - planes[ i ].plane[ 3 ];

		if( d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON )
		{
			continue;
		}

		d = DotProduct( p3, planes[ i ].plane ) - planes[ i ].plane[ 3 ];

		if( d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON )
		{
			continue;
		}

		// found it
		return i;
	}

	// add a new plane
	if( numPlanes == MAX_PATCH_PLANES )
	{
		Com_Error( ERR_DROP, "MAX_PATCH_PLANES" );
	}

	Vector4Copy( plane, planes[ numPlanes ].plane );
	planes[ numPlanes ].signbits = CM_SignbitsForNormal( plane );

	numPlanes++;

	return numPlanes - 1;
}

/*
==================
CM_PointOnPlaneSide
==================
*/
static int CM_PointOnPlaneSide( float *p, int planeNum )
{
	float *plane;
	float d;

	if( planeNum == -1 )
	{
		return SIDE_ON;
	}

	plane = planes[ planeNum ].plane;

	d = DotProduct( p, plane ) - plane[ 3 ];

	if( d > PLANE_TRI_EPSILON )
	{
		return SIDE_FRONT;
	}

	if( d < -PLANE_TRI_EPSILON )
	{
		return SIDE_BACK;
	}

	return SIDE_ON;
}

/*
==================
CM_GridPlane
==================
*/
static int CM_GridPlane( int gridPlanes[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ][ 2 ], int i, int j, int tri )
{
	int p;

	p = gridPlanes[ i ][ j ][ tri ];

	if( p != -1 )
	{
		return p;
	}

	p = gridPlanes[ i ][ j ][ !tri ];

	if( p != -1 )
	{
		return p;
	}

	// should never happen
	Com_Printf( "WARNING: CM_GridPlane unresolvable\n" );
	return -1;
}

/*
==================
CM_EdgePlaneNum
==================
*/
static int CM_EdgePlaneNum( cGrid_t *grid, int gridPlanes[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ][ 2 ], int i, int j, int k )
{
	float  *p1, *p2;
	vec3_t up;
	int    p;

	switch( k )
	{
		case 0: // top border
			p1 = grid->points[ i ][ j ];
			p2 = grid->points[ i + 1 ][ j ];
			p = CM_GridPlane( gridPlanes, i, j, 0 );
			VectorMA( p1, 4, planes[ p ].plane, up );
			return CM_FindPlane( p1, p2, up );

		case 2: // bottom border
			p1 = grid->points[ i ][ j + 1 ];
			p2 = grid->points[ i + 1 ][ j + 1 ];
			p = CM_GridPlane( gridPlanes, i, j, 1 );
			VectorMA( p1, 4, planes[ p ].plane, up );
			return CM_FindPlane( p2, p1, up );

		case 3: // left border
			p1 = grid->points[ i ][ j ];
			p2 = grid->points[ i ][ j + 1 ];
			p = CM_GridPlane( gridPlanes, i, j, 1 );
			VectorMA( p1, 4, planes[ p ].plane, up );
			return CM_FindPlane( p2, p1, up );

		case 1: // right border
			p1 = grid->points[ i + 1 ][ j ];
			p2 = grid->points[ i + 1 ][ j + 1 ];
			p = CM_GridPlane( gridPlanes, i, j, 0 );
			VectorMA( p1, 4, planes[ p ].plane, up );
			return CM_FindPlane( p1, p2, up );

		case 4: // diagonal out of triangle 0
			p1 = grid->points[ i + 1 ][ j + 1 ];
			p2 = grid->points[ i ][ j ];
			p = CM_GridPlane( gridPlanes, i, j, 0 );
			VectorMA( p1, 4, planes[ p ].plane, up );
			return CM_FindPlane( p1, p2, up );

		case 5: // diagonal out of triangle 1
			p1 = grid->points[ i ][ j ];
			p2 = grid->points[ i + 1 ][ j + 1 ];
			p = CM_GridPlane( gridPlanes, i, j, 1 );
			VectorMA( p1, 4, planes[ p ].plane, up );
			return CM_FindPlane( p1, p2, up );
	}

	Com_Error( ERR_DROP, "CM_EdgePlaneNum: bad k" );
	return -1;
}

/*
===================
CM_SetBorderInward
===================
*/
static void CM_SetBorderInward( cFacet_t *facet, cGrid_t *grid, int gridPlanes[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ][ 2 ],
                                int i, int j, int which )
{
	int   k, l;
	float *points[ 4 ];
	int   numPoints;

	switch( which )
	{
		case -1:
			points[ 0 ] = grid->points[ i ][ j ];
			points[ 1 ] = grid->points[ i + 1 ][ j ];
			points[ 2 ] = grid->points[ i + 1 ][ j + 1 ];
			points[ 3 ] = grid->points[ i ][ j + 1 ];
			numPoints = 4;
			break;

		case 0:
			points[ 0 ] = grid->points[ i ][ j ];
			points[ 1 ] = grid->points[ i + 1 ][ j ];
			points[ 2 ] = grid->points[ i + 1 ][ j + 1 ];
			numPoints = 3;
			break;

		case 1:
			points[ 0 ] = grid->points[ i + 1 ][ j + 1 ];
			points[ 1 ] = grid->points[ i ][ j + 1 ];
			points[ 2 ] = grid->points[ i ][ j ];
			numPoints = 3;
			break;

		default:
			Com_Error( ERR_FATAL, "CM_SetBorderInward: bad parameter" );
			numPoints = 0;
			break;
	}

	for( k = 0; k < facet->numBorders; k++ )
	{
		int front, back;

		front = 0;
		back = 0;

		for( l = 0; l < numPoints; l++ )
		{
			int side;

			side = CM_PointOnPlaneSide( points[ l ], facet->borderPlanes[ k ] );

			if( side == SIDE_FRONT )
			{
				front++;
			}

			if( side == SIDE_BACK )
			{
				back++;
			}
		}

		if( front && !back )
		{
			facet->borderInward[ k ] = qtrue;
		}
		else if( back && !front )
		{
			facet->borderInward[ k ] = qfalse;
		}
		else if( !front && !back )
		{
			// flat side border
			facet->borderPlanes[ k ] = -1;
		}
		else
		{
			// bisecting side border
			Com_DPrintf( "WARNING: CM_SetBorderInward: mixed plane sides\n" );
			facet->borderInward[ k ] = qfalse;

			if( !debugBlock )
			{
				debugBlock = qtrue;
				VectorCopy( grid->points[ i ][ j ], debugBlockPoints[ 0 ] );
				VectorCopy( grid->points[ i + 1 ][ j ], debugBlockPoints[ 1 ] );
				VectorCopy( grid->points[ i + 1 ][ j + 1 ], debugBlockPoints[ 2 ] );
				VectorCopy( grid->points[ i ][ j + 1 ], debugBlockPoints[ 3 ] );
			}
		}
	}
}

/*
==================
CM_ValidateFacet

If the facet isn't bounded by its borders, we screwed up.
==================
*/
static qboolean CM_ValidateFacet( cFacet_t *facet )
{
	float     plane[ 4 ];
	int       j;
	winding_t *w;
	vec3_t    bounds[ 2 ];

	if( facet->surfacePlane == -1 )
	{
		return qfalse;
	}

	Vector4Copy( planes[ facet->surfacePlane ].plane, plane );
	w = BaseWindingForPlane( plane, plane[ 3 ] );

	for( j = 0; j < facet->numBorders && w; j++ )
	{
		if( facet->borderPlanes[ j ] == -1 )
		{
			FreeWinding( w );
			return qfalse;
		}

		Vector4Copy( planes[ facet->borderPlanes[ j ] ].plane, plane );

		if( !facet->borderInward[ j ] )
		{
			VectorSubtract( vec3_origin, plane, plane );
			plane[ 3 ] = -plane[ 3 ];
		}

		ChopWindingInPlace( &w, plane, plane[ 3 ], 0.1f );
	}

	if( !w )
	{
		return qfalse; // winding was completely chopped away
	}

	// see if the facet is unreasonably large
	WindingBounds( w, bounds[ 0 ], bounds[ 1 ] );
	FreeWinding( w );

	for( j = 0; j < 3; j++ )
	{
		if( bounds[ 1 ][ j ] - bounds[ 0 ][ j ] > MAX_WORLD_COORD )
		{
			return qfalse; // we must be missing a plane
		}

		if( bounds[ 0 ][ j ] >= MAX_WORLD_COORD )
		{
			return qfalse;
		}

		if( bounds[ 1 ][ j ] <= MIN_WORLD_COORD )
		{
			return qfalse;
		}
	}

	return qtrue; // winding is fine
}

/*
==================
CM_AddFacetBevels
==================
*/
static void CM_AddFacetBevels( cFacet_t *facet )
{
	int       i, j, k, l;
	int       axis, dir, order, flipped;
	float     plane[ 4 ], d, newplane[ 4 ];
	winding_t *w, *w2;
	vec3_t    mins, maxs, vec, vec2;

	Vector4Copy( planes[ facet->surfacePlane ].plane, plane );

	w = BaseWindingForPlane( plane, plane[ 3 ] );

	for( j = 0; j < facet->numBorders && w; j++ )
	{
		if( facet->borderPlanes[ j ] == facet->surfacePlane )
		{
			continue;
		}

		Vector4Copy( planes[ facet->borderPlanes[ j ] ].plane, plane );

		if( !facet->borderInward[ j ] )
		{
			VectorSubtract( vec3_origin, plane, plane );
			plane[ 3 ] = -plane[ 3 ];
		}

		ChopWindingInPlace( &w, plane, plane[ 3 ], 0.1f );
	}

	if( !w )
	{
		return;
	}

	WindingBounds( w, mins, maxs );

	// add the axial planes
	order = 0;

	for( axis = 0; axis < 3; axis++ )
	{
		for( dir = -1; dir <= 1; dir += 2, order++ )
		{
			VectorClear( plane );
			plane[ axis ] = dir;

			if( dir == 1 )
			{
				plane[ 3 ] = maxs[ axis ];
			}
			else
			{
				plane[ 3 ] = -mins[ axis ];
			}

			//if it's the surface plane
			if( CM_PlaneEqual( &planes[ facet->surfacePlane ], plane, &flipped ) )
			{
				continue;
			}

			// see if the plane is allready present
			for( i = 0; i < facet->numBorders; i++ )
			{
				if( CM_PlaneEqual( &planes[ facet->borderPlanes[ i ] ], plane, &flipped ) )
				{
					break;
				}
			}

			if( i == facet->numBorders )
			{
				if( facet->numBorders > MAX_FACET_BEVELS )
				{
					Com_Printf( "ERROR: too many bevels\n" );
				}

				facet->borderPlanes[ facet->numBorders ] = CM_FindPlane2( plane, &flipped );
				facet->borderNoAdjust[ facet->numBorders ] = 0;
				facet->borderInward[ facet->numBorders ] = flipped;
				facet->numBorders++;
			}
		}
	}

	//
	// add the edge bevels
	//
	// test the non-axial plane edges
	for( j = 0; j < w->numpoints; j++ )
	{
		k = ( j + 1 ) % w->numpoints;
		VectorSubtract( w->p[ j ], w->p[ k ], vec );

		//if it's a degenerate edge
		if( VectorNormalize( vec ) < 0.5 )
		{
			continue;
		}

		CM_SnapVector( vec );

		for( k = 0; k < 3; k++ )
		{
			if( vec[ k ] == -1 || vec[ k ] == 1 )
			{
				break; // axial
			}
		}

		if( k < 3 )
		{
			continue; // only test non-axial edges
		}

		// try the six possible slanted axials from this edge
		for( axis = 0; axis < 3; axis++ )
		{
			for( dir = -1; dir <= 1; dir += 2 )
			{
				// construct a plane
				VectorClear( vec2 );
				vec2[ axis ] = dir;
				CrossProduct( vec, vec2, plane );

				if( VectorNormalize( plane ) < 0.5 )
				{
					continue;
				}

				plane[ 3 ] = DotProduct( w->p[ j ], plane );

				// if all the points of the facet winding are
				// behind this plane, it is a proper edge bevel
				for( l = 0; l < w->numpoints; l++ )
				{
					d = DotProduct( w->p[ l ], plane ) - plane[ 3 ];

					if( d > 0.1 )
					{
						break; // point in front
					}
				}

				if( l < w->numpoints )
				{
					continue;
				}

				//if it's the surface plane
				if( CM_PlaneEqual( &planes[ facet->surfacePlane ], plane, &flipped ) )
				{
					continue;
				}

				// see if the plane is allready present
				for( i = 0; i < facet->numBorders; i++ )
				{
					if( CM_PlaneEqual( &planes[ facet->borderPlanes[ i ] ], plane, &flipped ) )
					{
						break;
					}
				}

				if( i == facet->numBorders )
				{
					if( facet->numBorders > MAX_FACET_BEVELS )
					{
						Com_Printf( "ERROR: too many bevels\n" );
					}

					facet->borderPlanes[ facet->numBorders ] = CM_FindPlane2( plane, &flipped );

					for( k = 0; k < facet->numBorders; k++ )
					{
						if( facet->borderPlanes[ facet->numBorders ] == facet->borderPlanes[ k ] )
						{
							Com_Printf( "WARNING: bevel plane already used\n" );
						}
					}

					facet->borderNoAdjust[ facet->numBorders ] = 0;
					facet->borderInward[ facet->numBorders ] = flipped;
					//
					w2 = CopyWinding( w );
					Vector4Copy( planes[ facet->borderPlanes[ facet->numBorders ] ].plane, newplane );

					if( !facet->borderInward[ facet->numBorders ] )
					{
						VectorNegate( newplane, newplane );
						newplane[ 3 ] = -newplane[ 3 ];
					}

					ChopWindingInPlace( &w2, newplane, newplane[ 3 ], 0.1f );

					if( !w2 )
					{
						Com_DPrintf( "WARNING: CM_AddFacetBevels... invalid bevel\n" );
						continue;
					}
					else
					{
						FreeWinding( w2 );
					}

					//
					facet->numBorders++;
					//already got a bevel
//                  break;
				}
			}
		}
	}

	FreeWinding( w );

	// add opposite plane
	facet->borderPlanes[ facet->numBorders ] = facet->surfacePlane;
	facet->borderNoAdjust[ facet->numBorders ] = 0;
	facet->borderInward[ facet->numBorders ] = qtrue;
	facet->numBorders++;
}

typedef enum
{
  EN_TOP,
  EN_RIGHT,
  EN_BOTTOM,
  EN_LEFT
} edgeName_t;

/*
==================
CM_PatchCollideFromGrid
==================
*/
static void CM_SurfaceCollideFromGrid( cGrid_t *grid, cSurfaceCollide_t *sc )
{
	int             i, j;
	float          *p1, *p2, *p3;
	int             gridPlanes[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ][ 2 ];
	cFacet_t       *facet;
	int             borders[ 4 ];
	int             noAdjust[ 4 ];

	numPlanes = 0;
	numFacets = 0;

	// find the planes for each triangle of the grid
	for( i = 0; i < grid->width - 1; i++ )
	{
		for( j = 0; j < grid->height - 1; j++ )
		{
			p1 = grid->points[ i ][ j ];
			p2 = grid->points[ i + 1 ][ j ];
			p3 = grid->points[ i + 1 ][ j + 1 ];
			gridPlanes[ i ][ j ][ 0 ] = CM_FindPlane( p1, p2, p3 );

			p1 = grid->points[ i + 1 ][ j + 1 ];
			p2 = grid->points[ i ][ j + 1 ];
			p3 = grid->points[ i ][ j ];
			gridPlanes[ i ][ j ][ 1 ] = CM_FindPlane( p1, p2, p3 );
		}
	}

	// create the borders for each facet
	for( i = 0; i < grid->width - 1; i++ )
	{
		for( j = 0; j < grid->height - 1; j++ )
		{
			borders[ EN_TOP ] = -1;

			if( j > 0 )
			{
				borders[ EN_TOP ] = gridPlanes[ i ][ j - 1 ][ 1 ];
			}
			else if( grid->wrapHeight )
			{
				borders[ EN_TOP ] = gridPlanes[ i ][ grid->height - 2 ][ 1 ];
			}

			noAdjust[ EN_TOP ] = ( borders[ EN_TOP ] == gridPlanes[ i ][ j ][ 0 ] );

			if( borders[ EN_TOP ] == -1 || noAdjust[ EN_TOP ] )
			{
				borders[ EN_TOP ] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 0 );
			}

			borders[ EN_BOTTOM ] = -1;

			if( j < grid->height - 2 )
			{
				borders[ EN_BOTTOM ] = gridPlanes[ i ][ j + 1 ][ 0 ];
			}
			else if( grid->wrapHeight )
			{
				borders[ EN_BOTTOM ] = gridPlanes[ i ][ 0 ][ 0 ];
			}

			noAdjust[ EN_BOTTOM ] = ( borders[ EN_BOTTOM ] == gridPlanes[ i ][ j ][ 1 ] );

			if( borders[ EN_BOTTOM ] == -1 || noAdjust[ EN_BOTTOM ] )
			{
				borders[ EN_BOTTOM ] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 2 );
			}

			borders[ EN_LEFT ] = -1;

			if( i > 0 )
			{
				borders[ EN_LEFT ] = gridPlanes[ i - 1 ][ j ][ 0 ];
			}
			else if( grid->wrapWidth )
			{
				borders[ EN_LEFT ] = gridPlanes[ grid->width - 2 ][ j ][ 0 ];
			}

			noAdjust[ EN_LEFT ] = ( borders[ EN_LEFT ] == gridPlanes[ i ][ j ][ 1 ] );

			if( borders[ EN_LEFT ] == -1 || noAdjust[ EN_LEFT ] )
			{
				borders[ EN_LEFT ] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 3 );
			}

			borders[ EN_RIGHT ] = -1;

			if( i < grid->width - 2 )
			{
				borders[ EN_RIGHT ] = gridPlanes[ i + 1 ][ j ][ 1 ];
			}
			else if( grid->wrapWidth )
			{
				borders[ EN_RIGHT ] = gridPlanes[ 0 ][ j ][ 1 ];
			}

			noAdjust[ EN_RIGHT ] = ( borders[ EN_RIGHT ] == gridPlanes[ i ][ j ][ 0 ] );

			if( borders[ EN_RIGHT ] == -1 || noAdjust[ EN_RIGHT ] )
			{
				borders[ EN_RIGHT ] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 1 );
			}

			if( numFacets == MAX_FACETS )
			{
				Com_Error( ERR_DROP, "MAX_FACETS" );
			}

			facet = &facets[ numFacets ];
			Com_Memset( facet, 0, sizeof( *facet ) );

			if( gridPlanes[ i ][ j ][ 0 ] == gridPlanes[ i ][ j ][ 1 ] )
			{
				if( gridPlanes[ i ][ j ][ 0 ] == -1 )
				{
					continue; // degenrate
				}

				facet->surfacePlane = gridPlanes[ i ][ j ][ 0 ];
				facet->numBorders = 4;
				facet->borderPlanes[ 0 ] = borders[ EN_TOP ];
				facet->borderNoAdjust[ 0 ] = noAdjust[ EN_TOP ];
				facet->borderPlanes[ 1 ] = borders[ EN_RIGHT ];
				facet->borderNoAdjust[ 1 ] = noAdjust[ EN_RIGHT ];
				facet->borderPlanes[ 2 ] = borders[ EN_BOTTOM ];
				facet->borderNoAdjust[ 2 ] = noAdjust[ EN_BOTTOM ];
				facet->borderPlanes[ 3 ] = borders[ EN_LEFT ];
				facet->borderNoAdjust[ 3 ] = noAdjust[ EN_LEFT ];
				CM_SetBorderInward( facet, grid, gridPlanes, i, j, -1 );

				if( CM_ValidateFacet( facet ) )
				{
					CM_AddFacetBevels( facet );
					numFacets++;
				}
			}
			else
			{
				// two seperate triangles
				facet->surfacePlane = gridPlanes[ i ][ j ][ 0 ];
				facet->numBorders = 3;
				facet->borderPlanes[ 0 ] = borders[ EN_TOP ];
				facet->borderNoAdjust[ 0 ] = noAdjust[ EN_TOP ];
				facet->borderPlanes[ 1 ] = borders[ EN_RIGHT ];
				facet->borderNoAdjust[ 1 ] = noAdjust[ EN_RIGHT ];
				facet->borderPlanes[ 2 ] = gridPlanes[ i ][ j ][ 1 ];

				if( facet->borderPlanes[ 2 ] == -1 )
				{
					facet->borderPlanes[ 2 ] = borders[ EN_BOTTOM ];

					if( facet->borderPlanes[ 2 ] == -1 )
					{
						facet->borderPlanes[ 2 ] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 4 );
					}
				}

				CM_SetBorderInward( facet, grid, gridPlanes, i, j, 0 );

				if( CM_ValidateFacet( facet ) )
				{
					CM_AddFacetBevels( facet );
					numFacets++;
				}

				if( numFacets == MAX_FACETS )
				{
					Com_Error( ERR_DROP, "MAX_FACETS" );
				}

				facet = &facets[ numFacets ];
				Com_Memset( facet, 0, sizeof( *facet ) );

				facet->surfacePlane = gridPlanes[ i ][ j ][ 1 ];
				facet->numBorders = 3;
				facet->borderPlanes[ 0 ] = borders[ EN_BOTTOM ];
				facet->borderNoAdjust[ 0 ] = noAdjust[ EN_BOTTOM ];
				facet->borderPlanes[ 1 ] = borders[ EN_LEFT ];
				facet->borderNoAdjust[ 1 ] = noAdjust[ EN_LEFT ];
				facet->borderPlanes[ 2 ] = gridPlanes[ i ][ j ][ 0 ];

				if( facet->borderPlanes[ 2 ] == -1 )
				{
					facet->borderPlanes[ 2 ] = borders[ EN_TOP ];

					if( facet->borderPlanes[ 2 ] == -1 )
					{
						facet->borderPlanes[ 2 ] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 5 );
					}
				}

				CM_SetBorderInward( facet, grid, gridPlanes, i, j, 1 );

				if( CM_ValidateFacet( facet ) )
				{
					CM_AddFacetBevels( facet );
					numFacets++;
				}
			}
		}
	}

	// copy the results out
	sc->numPlanes = numPlanes;
	sc->numFacets = numFacets;
	sc->facets = Hunk_Alloc( numFacets * sizeof( *sc->facets ), h_high );
	Com_Memcpy( sc->facets, facets, numFacets * sizeof( *sc->facets ) );
	sc->planes = Hunk_Alloc( numPlanes * sizeof( *sc->planes ), h_high );
	Com_Memcpy( sc->planes, planes, numPlanes * sizeof( *sc->planes ) );
}

/*
===================
CM_GeneratePatchCollide

Creates an internal structure that will be used to perform
collision detection with a patch mesh.

Points is packed as concatenated rows.
===================
*/
cSurfaceCollide_t *CM_GeneratePatchCollide( int width, int height, vec3_t *points )
{
	cSurfaceCollide_t *sc;
	cGrid_t         grid;
	int             i, j;

	if( width <= 2 || height <= 2 || !points )
	{
		Com_Error( ERR_DROP, "CM_GeneratePatchFacets: bad parameters: (%i, %i, %p)", width, height, ( void * ) points );
	}

	if( !( width & 1 ) || !( height & 1 ) )
	{
		Com_Error( ERR_DROP, "CM_GeneratePatchFacets: even sizes are invalid for quadratic meshes" );
	}

	if( width > MAX_GRID_SIZE || height > MAX_GRID_SIZE )
	{
		Com_Error( ERR_DROP, "CM_GeneratePatchFacets: source is > MAX_GRID_SIZE" );
	}

	// build a grid
	grid.width = width;
	grid.height = height;
	grid.wrapWidth = qfalse;
	grid.wrapHeight = qfalse;

	for( i = 0; i < width; i++ )
	{
		for( j = 0; j < height; j++ )
		{
			VectorCopy( points[ j * width + i ], grid.points[ i ][ j ] );
		}
	}

	// subdivide the grid
	CM_SetGridWrapWidth( &grid );
	CM_SubdivideGridColumns( &grid );
	CM_RemoveDegenerateColumns( &grid );

	CM_TransposeGrid( &grid );

	CM_SetGridWrapWidth( &grid );
	CM_SubdivideGridColumns( &grid );
	CM_RemoveDegenerateColumns( &grid );

	// we now have a grid of points exactly on the curve
	// the aproximate surface defined by these points will be
	// collided against
	sc = Hunk_Alloc( sizeof( *sc ), h_high );
	ClearBounds( sc->bounds[ 0 ], sc->bounds[ 1 ] );

	for( i = 0; i < grid.width; i++ )
	{
		for( j = 0; j < grid.height; j++ )
		{
			AddPointToBounds( grid.points[ i ][ j ], sc->bounds[ 0 ], sc->bounds[ 1 ] );
		}
	}

	c_totalPatchBlocks += ( grid.width - 1 ) * ( grid.height - 1 );

	// generate a bsp tree for the surface
	CM_SurfaceCollideFromGrid( &grid, sc );

	// expand by one unit for epsilon purposes
	sc->bounds[ 0 ][ 0 ] -= 1;
	sc->bounds[ 0 ][ 1 ] -= 1;
	sc->bounds[ 0 ][ 2 ] -= 1;

	sc->bounds[ 1 ][ 0 ] += 1;
	sc->bounds[ 1 ][ 1 ] += 1;
	sc->bounds[ 1 ][ 2 ] += 1;

	return sc;
}
