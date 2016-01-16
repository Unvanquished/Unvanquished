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

const cSurfaceCollide_t *debugSurfaceCollide;
const cFacet_t          *debugFacet;
bool                debugBlock;
vec3_t                  debugBlockPoints[ 4 ];

/*
=================
CM_ClearLevelPatches
=================
*/
void CM_ClearLevelPatches()
{
	debugSurfaceCollide = nullptr;
	debugFacet = nullptr;
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
static bool CM_NeedsSubdivision( vec3_t a, vec3_t b, vec3_t c )
{
	vec3_t cmid;
	vec3_t lmid;
	vec3_t delta;
	float  dist;
	int    i;

	// calculate the linear midpoint
	for ( i = 0; i < 3; i++ )
	{
		lmid[ i ] = 0.5 * ( a[ i ] + c[ i ] );
	}

	// calculate the exact curve midpoint
	for ( i = 0; i < 3; i++ )
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

	for ( i = 0; i < 3; i++ )
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
	bool tempWrap;

	if ( grid->width > grid->height )
	{
		for ( i = 0; i < grid->height; i++ )
		{
			for ( j = i + 1; j < grid->width; j++ )
			{
				if ( j < grid->height )
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
		for ( i = 0; i < grid->width; i++ )
		{
			for ( j = i + 1; j < grid->height; j++ )
			{
				if ( j < grid->width )
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

If the left and right columns are exactly equal, set grid->wrapWidth true
===================
*/
static void CM_SetGridWrapWidth( cGrid_t *grid )
{
	int   i, j;
	float d;

	for ( i = 0; i < grid->height; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			d = grid->points[ 0 ][ i ][ j ] - grid->points[ grid->width - 1 ][ i ][ j ];

			if ( d < -WRAP_POINT_EPSILON || d > WRAP_POINT_EPSILON )
			{
				break;
			}
		}

		if ( j != 3 )
		{
			break;
		}
	}

	if ( i == grid->height )
	{
		grid->wrapWidth = true;
	}
	else
	{
		grid->wrapWidth = false;
	}
}

/*
=================
CM_SubdivideGridColumns

Adds columns as necessary to the grid until
all the approximating points are within SUBDIVIDE_DISTANCE
from the true curve
=================
*/
static void CM_SubdivideGridColumns( cGrid_t *grid )
{
	int i, j, k;

	for ( i = 0; i < grid->width - 2; )
	{
		// grid->points[i][x] is an interpolating control point
		// grid->points[i+1][x] is an approximating control point
		// grid->points[i+2][x] is an interpolating control point

		//
		// first see if we can collapse the approximating column away
		//
		for ( j = 0; j < grid->height; j++ )
		{
			if ( CM_NeedsSubdivision( grid->points[ i ][ j ], grid->points[ i + 1 ][ j ], grid->points[ i + 2 ][ j ] ) )
			{
				break;
			}
		}

		if ( j == grid->height )
		{
			// all of the points were close enough to the linear midpoints
			// that we can collapse the entire column away
			for ( j = 0; j < grid->height; j++ )
			{
				// remove the column
				for ( k = i + 2; k < grid->width; k++ )
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
		for ( j = 0; j < grid->height; j++ )
		{
			vec3_t prev, mid, next;

			// save the control points now
			VectorCopy( grid->points[ i ][ j ], prev );
			VectorCopy( grid->points[ i + 1 ][ j ], mid );
			VectorCopy( grid->points[ i + 2 ][ j ], next );

			// make room for two additional columns in the grid
			// columns i+1 will be replaced, column i+2 will become i+4
			// i+1, i+2, and i+3 will be generated
			for ( k = grid->width - 1; k > i + 1; k-- )
			{
				VectorCopy( grid->points[ k ][ j ], grid->points[ k + 2 ][ j ] );
			}

			// generate the subdivided points
			CM_Subdivide( prev, mid, next, grid->points[ i + 1 ][ j ], grid->points[ i + 2 ][ j ], grid->points[ i + 3 ][ j ] );
		}

		grid->width += 2;

		// the new approximating point at i+1 may need to be removed
		// or subdivided farther, so don't advance i
	}
}

/*
======================
CM_ComparePoints
======================
*/
static const float POINT_EPSILON = 0.1;
static bool CM_ComparePoints( float *a, float *b )
{
	float d;

	d = a[ 0 ] - b[ 0 ];

	if ( d < -POINT_EPSILON || d > POINT_EPSILON )
	{
		return false;
	}

	d = a[ 1 ] - b[ 1 ];

	if ( d < -POINT_EPSILON || d > POINT_EPSILON )
	{
		return false;
	}

	d = a[ 2 ] - b[ 2 ];

	if ( d < -POINT_EPSILON || d > POINT_EPSILON )
	{
		return false;
	}

	return true;
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

	for ( i = 0; i < grid->width - 1; i++ )
	{
		for ( j = 0; j < grid->height; j++ )
		{
			if ( !CM_ComparePoints( grid->points[ i ][ j ], grid->points[ i + 1 ][ j ] ) )
			{
				break;
			}
		}

		if ( j != grid->height )
		{
			continue; // not degenerate
		}

		for ( j = 0; j < grid->height; j++ )
		{
			// remove the column
			for ( k = i + 2; k < grid->width; k++ )
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

/*
==================
CM_GridPlane
==================
*/
static int CM_GridPlane( int gridPlanes[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ][ 2 ], int i, int j, int tri )
{
	int p;

	p = gridPlanes[ i ][ j ][ tri ];

	if ( p != -1 )
	{
		return p;
	}

	p = gridPlanes[ i ][ j ][ !tri ];

	if ( p != -1 )
	{
		return p;
	}

	// should never happen
	Log::Warn( "CM_GridPlane unresolvable" );
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

	switch ( k )
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

	Sys::Drop( "CM_EdgePlaneNum: bad k" );
}

/*
===================
CM_SetBorderInward
===================
*/
static void CM_SetBorderInward( cFacet_t *facet, cGrid_t *grid,
                                int i, int j, int which )
{
	int   k, l;
	float *points[ 4 ];
	int   numPoints;

	switch ( which )
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
			Sys::Error( "CM_SetBorderInward: bad parameter" );
	}

	for ( k = 0; k < facet->numBorders; k++ )
	{
		int front, back;

		front = 0;
		back = 0;

		for ( l = 0; l < numPoints; l++ )
		{
			planeSide_t side = CM_PointOnPlaneSide( points[ l ], facet->borderPlanes[ k ] );

			if ( side == planeSide_t::SIDE_FRONT )
			{
				front++;
			}

			if ( side == planeSide_t::SIDE_BACK )
			{
				back++;
			}
		}

		if ( front && !back )
		{
			facet->borderInward[ k ] = true;
		}
		else if ( back && !front )
		{
			facet->borderInward[ k ] = false;
		}
		else if ( !front && !back )
		{
			// flat side border
			facet->borderPlanes[ k ] = -1;
		}
		else
		{
			// bisecting side border
			cmLog.Debug( "WARNING: CM_SetBorderInward: mixed plane sides\n" );
			facet->borderInward[ k ] = false;

			if ( !debugBlock )
			{
				debugBlock = true;
				VectorCopy( grid->points[ i ][ j ], debugBlockPoints[ 0 ] );
				VectorCopy( grid->points[ i + 1 ][ j ], debugBlockPoints[ 1 ] );
				VectorCopy( grid->points[ i + 1 ][ j + 1 ], debugBlockPoints[ 2 ] );
				VectorCopy( grid->points[ i ][ j + 1 ], debugBlockPoints[ 3 ] );
			}
		}
	}
}

enum edgeName_e
{
  EN_TOP,
  EN_RIGHT,
  EN_BOTTOM,
  EN_LEFT
};

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

	CM_ResetPlaneCounts();

	// find the planes for each triangle of the grid
	for ( i = 0; i < grid->width - 1; i++ )
	{
		for ( j = 0; j < grid->height - 1; j++ )
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
	for ( i = 0; i < grid->width - 1; i++ )
	{
		for ( j = 0; j < grid->height - 1; j++ )
		{
			borders[ EN_TOP ] = -1;

			if ( j > 0 )
			{
				borders[ EN_TOP ] = gridPlanes[ i ][ j - 1 ][ 1 ];
			}
			else if ( grid->wrapHeight )
			{
				borders[ EN_TOP ] = gridPlanes[ i ][ grid->height - 2 ][ 1 ];
			}

			noAdjust[ EN_TOP ] = ( borders[ EN_TOP ] == gridPlanes[ i ][ j ][ 0 ] );

			if ( borders[ EN_TOP ] == -1 || noAdjust[ EN_TOP ] )
			{
				borders[ EN_TOP ] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 0 );
			}

			borders[ EN_BOTTOM ] = -1;

			if ( j < grid->height - 2 )
			{
				borders[ EN_BOTTOM ] = gridPlanes[ i ][ j + 1 ][ 0 ];
			}
			else if ( grid->wrapHeight )
			{
				borders[ EN_BOTTOM ] = gridPlanes[ i ][ 0 ][ 0 ];
			}

			noAdjust[ EN_BOTTOM ] = ( borders[ EN_BOTTOM ] == gridPlanes[ i ][ j ][ 1 ] );

			if ( borders[ EN_BOTTOM ] == -1 || noAdjust[ EN_BOTTOM ] )
			{
				borders[ EN_BOTTOM ] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 2 );
			}

			borders[ EN_LEFT ] = -1;

			if ( i > 0 )
			{
				borders[ EN_LEFT ] = gridPlanes[ i - 1 ][ j ][ 0 ];
			}
			else if ( grid->wrapWidth )
			{
				borders[ EN_LEFT ] = gridPlanes[ grid->width - 2 ][ j ][ 0 ];
			}

			noAdjust[ EN_LEFT ] = ( borders[ EN_LEFT ] == gridPlanes[ i ][ j ][ 1 ] );

			if ( borders[ EN_LEFT ] == -1 || noAdjust[ EN_LEFT ] )
			{
				borders[ EN_LEFT ] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 3 );
			}

			borders[ EN_RIGHT ] = -1;

			if ( i < grid->width - 2 )
			{
				borders[ EN_RIGHT ] = gridPlanes[ i + 1 ][ j ][ 1 ];
			}
			else if ( grid->wrapWidth )
			{
				borders[ EN_RIGHT ] = gridPlanes[ 0 ][ j ][ 1 ];
			}

			noAdjust[ EN_RIGHT ] = ( borders[ EN_RIGHT ] == gridPlanes[ i ][ j ][ 0 ] );

			if ( borders[ EN_RIGHT ] == -1 || noAdjust[ EN_RIGHT ] )
			{
				borders[ EN_RIGHT ] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 1 );
			}

			if ( numFacets == SHADER_MAX_TRIANGLES )
			{
				Sys::Drop( "MAX_FACETS" );
			}

			facet = &facets[ numFacets ];
			Com_Memset( facet, 0, sizeof( *facet ) );

			if ( gridPlanes[ i ][ j ][ 0 ] == gridPlanes[ i ][ j ][ 1 ] )
			{
				if ( gridPlanes[ i ][ j ][ 0 ] == -1 )
				{
					continue; // degenerate
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
				CM_SetBorderInward( facet, grid, i, j, -1 );

				if ( CM_ValidateFacet( facet ) )
				{
					CM_AddFacetBevels( facet );
					numFacets++;
				}
			}
			else
			{
				// two separate triangles
				facet->surfacePlane = gridPlanes[ i ][ j ][ 0 ];
				facet->numBorders = 3;
				facet->borderPlanes[ 0 ] = borders[ EN_TOP ];
				facet->borderNoAdjust[ 0 ] = noAdjust[ EN_TOP ];
				facet->borderPlanes[ 1 ] = borders[ EN_RIGHT ];
				facet->borderNoAdjust[ 1 ] = noAdjust[ EN_RIGHT ];
				facet->borderPlanes[ 2 ] = gridPlanes[ i ][ j ][ 1 ];

				if ( facet->borderPlanes[ 2 ] == -1 )
				{
					facet->borderPlanes[ 2 ] = borders[ EN_BOTTOM ];

					if ( facet->borderPlanes[ 2 ] == -1 )
					{
						facet->borderPlanes[ 2 ] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 4 );
					}
				}

				CM_SetBorderInward( facet, grid, i, j, 0 );

				if ( CM_ValidateFacet( facet ) )
				{
					CM_AddFacetBevels( facet );
					numFacets++;
				}

				if ( numFacets == SHADER_MAX_TRIANGLES )
				{
					Sys::Drop( "MAX_FACETS" );
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

				if ( facet->borderPlanes[ 2 ] == -1 )
				{
					facet->borderPlanes[ 2 ] = borders[ EN_TOP ];

					if ( facet->borderPlanes[ 2 ] == -1 )
					{
						facet->borderPlanes[ 2 ] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 5 );
					}
				}

				CM_SetBorderInward( facet, grid, i, j, 1 );

				if ( CM_ValidateFacet( facet ) )
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
	sc->facets = ( cFacet_t * ) CM_Alloc( numFacets * sizeof( *sc->facets ) );
	Com_Memcpy( sc->facets, facets, numFacets * sizeof( *sc->facets ) );
	sc->planes = ( cPlane_t * ) CM_Alloc( numPlanes * sizeof( *sc->planes ) );
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

	if ( width <= 2 || height <= 2 || !points )
	{
		Sys::Drop( "CM_GeneratePatchFacets: bad parameters: (%i, %i, %p)", width, height, ( void * ) points );
	}

	if ( !( width & 1 ) || !( height & 1 ) )
	{
		Sys::Drop( "CM_GeneratePatchFacets: even sizes are invalid for quadratic meshes" );
	}

	if ( width > MAX_GRID_SIZE || height > MAX_GRID_SIZE )
	{
		Sys::Drop( "CM_GeneratePatchFacets: source is > MAX_GRID_SIZE" );
	}

	// build a grid
	grid.width = width;
	grid.height = height;
	grid.wrapWidth = false;
	grid.wrapHeight = false;

	for ( i = 0; i < width; i++ )
	{
		for ( j = 0; j < height; j++ )
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
	// the approximate surface defined by these points will be
	// collided against
	sc = ( cSurfaceCollide_t * ) CM_Alloc( sizeof( *sc ) );
	ClearBounds( sc->bounds[ 0 ], sc->bounds[ 1 ] );

	for ( i = 0; i < grid.width; i++ )
	{
		for ( j = 0; j < grid.height; j++ )
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
