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

static const int PLANE_HASHES = 8192;
static cPlane_t *planeHashTable[ PLANE_HASHES ];

int      numPlanes;
cPlane_t planes[ SHADER_MAX_TRIANGLES ];

int      numFacets;
cFacet_t facets[ SHADER_MAX_TRIANGLES ];

/*
=================
CM_ResetPlaneCounts
=================
*/

void CM_ResetPlaneCounts()
{
	memset( planeHashTable, 0, sizeof( planeHashTable ) );
	numPlanes = 0;
	numFacets = 0;
}
/*
=================
CM_SignbitsForNormal
=================
*/
int CM_SignbitsForNormal( vec3_t normal )
{
	int bits, j;

	bits = 0;

	for ( j = 0; j < 3; j++ )
	{
		if ( normal[ j ] < 0 )
		{
			bits |= 1 << j;
		}
	}

	return bits;
}

/*
=====================
CM_PlaneFromPoints

Returns false if the triangle is degenerate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
bool CM_PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c )
{
	vec3_t d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane );

	if ( VectorNormalize( plane ) == 0 )
	{
		return false;
	}

	plane[ 3 ] = DotProduct( a, plane );
	return true;
}

static const double NORMAL_EPSILON    = 0.0001;
static const double DIST_EPSILON      = 0.02;

/*
==================
CM_PlaneEqual
==================
*/
int CM_PlaneEqual( cPlane_t *p, float plane[ 4 ], bool *flipped )
{
	float invplane[ 4 ];

	if ( fabs( p->plane[ 0 ] - plane[ 0 ] ) < NORMAL_EPSILON
	     && fabs( p->plane[ 1 ] - plane[ 1 ] ) < NORMAL_EPSILON
	     && fabs( p->plane[ 2 ] - plane[ 2 ] ) < NORMAL_EPSILON && fabs( p->plane[ 3 ] - plane[ 3 ] ) < DIST_EPSILON )
	{
		*flipped = false;
		return true;
	}

	VectorNegate( plane, invplane );
	invplane[ 3 ] = -plane[ 3 ];

	if ( fabs( p->plane[ 0 ] - invplane[ 0 ] ) < NORMAL_EPSILON
	     && fabs( p->plane[ 1 ] - invplane[ 1 ] ) < NORMAL_EPSILON
	     && fabs( p->plane[ 2 ] - invplane[ 2 ] ) < NORMAL_EPSILON && fabs( p->plane[ 3 ] - invplane[ 3 ] ) < DIST_EPSILON )
	{
		*flipped = true;
		return true;
	}

	return false;
}

/*
==================
CM_SnapVector
==================
*/
void CM_SnapVector( vec3_t normal )
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		if ( fabs( normal[ i ] - 1 ) < NORMAL_EPSILON )
		{
			VectorClear( normal );
			normal[ i ] = 1;
			break;
		}

		if ( fabs( normal[ i ] - -1 ) < NORMAL_EPSILON )
		{
			VectorClear( normal );
			normal[ i ] = -1;
			break;
		}
	}
}

/*
================
CM_GenerateHashValue

return a hash value for a plane
================
*/
static long CM_GenerateHashValue( vec4_t plane )
{
	long hash;

	hash = ( int ) fabs( plane[ 3 ] );
	hash &= ( PLANE_HASHES - 1 );

	return hash;
}

/*
================
CM_AddPlaneToHash
================
*/
static void CM_AddPlaneToHash( cPlane_t *p )
{
	long hash;

	hash = CM_GenerateHashValue( p->plane );

	p->hashChain = planeHashTable[ hash ];
	planeHashTable[ hash ] = p;
}

/*
================
CM_CreateNewFloatPlane
================
*/
static int CM_CreateNewFloatPlane( vec4_t plane )
{
	cPlane_t *p; //, temp;

	// create a new plane
	if ( numPlanes == SHADER_MAX_TRIANGLES )
	{
		Sys::Drop( "CM_FindPlane: SHADER_MAX_TRIANGLES" );
	}

	p = &planes[ numPlanes ];
	Vector4Copy( plane, p->plane );

	p->signbits = CM_SignbitsForNormal( plane );

	numPlanes++;

	CM_AddPlaneToHash( p );
	return numPlanes - 1;
}

/*
==================
CM_FindPlane2
==================
*/
int CM_FindPlane2( float plane[ 4 ], bool *flipped )
{
	int      i;
	cPlane_t *p;
	int      hash, h;

	//SnapPlane(normal, &dist);
	hash = CM_GenerateHashValue( plane );

	// search the border bins as well
	for ( i = -1; i <= 1; i++ )
	{
		h = ( hash + i ) & ( PLANE_HASHES - 1 );

		for ( p = planeHashTable[ h ]; p; p = p->hashChain )
		{
			if ( CM_PlaneEqual( p, plane, flipped ) )
			{
				return p - planes;
			}
		}
	}

	*flipped = false;
	return CM_CreateNewFloatPlane( plane );
}

/*
==================
CM_FindPlane
==================
*/
int CM_FindPlane( const float *p1, const float *p2, const float *p3 )
{
	float      plane[ 4 ];
	int        i;
	float      d;
	cPlane_t   *p;
	int        hash, h;

	if ( !CM_PlaneFromPoints( plane, p1, p2, p3 ) )
	{
		return -1;
	}

	hash = CM_GenerateHashValue( plane );

	// search the border bins as well
	for ( i = -1; i <= 1; i++ )
	{
		h = ( hash + i ) & ( PLANE_HASHES - 1 );

		for ( p = planeHashTable[ h ]; p; p = p->hashChain )
		{
			//check points on the plane
			if ( DotProduct( plane, p->plane ) < 0 )
			{
				continue; // allow backwards planes?
			}

			d = DotProduct( p1, p->plane ) - p->plane[ 3 ];

			if ( d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON )
			{
				continue;
			}

			d = DotProduct( p2, p->plane ) - p->plane[ 3 ];

			if ( d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON )
			{
				continue;
			}

			d = DotProduct( p3, p->plane ) - p->plane[ 3 ];

			if ( d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON )
			{
				continue;
			}

			return p - planes;
		}
	}

	return CM_CreateNewFloatPlane( plane );
}

/*
==================
CM_PointOnPlaneSide
==================
*/
planeSide_t CM_PointOnPlaneSide( float *p, int planeNum )
{
	float *plane;
	float d;

	if ( planeNum == -1 )
	{
		return planeSide_t::SIDE_ON;
	}

	plane = planes[ planeNum ].plane;

	d = DotProduct( p, plane ) - plane[ 3 ];

	if ( d > PLANE_TRI_EPSILON )
	{
		return planeSide_t::SIDE_FRONT;
	}

	if ( d < -PLANE_TRI_EPSILON )
	{
		return planeSide_t::SIDE_BACK;
	}

	return planeSide_t::SIDE_ON;
}

/*
==================
CM_ValidateFacet

If the facet isn't bounded by its borders, we screwed up.
==================
*/
bool CM_ValidateFacet( cFacet_t *facet )
{
	float     plane[ 4 ];
	int       j;
	winding_t *w;
	vec3_t    bounds[ 2 ];

	if ( facet->surfacePlane == -1 )
	{
		return false;
	}

	Vector4Copy( planes[ facet->surfacePlane ].plane, plane );
	w = BaseWindingForPlane( plane, plane[ 3 ] );

	for ( j = 0; j < facet->numBorders && w; j++ )
	{
		if ( facet->borderPlanes[ j ] == -1 )
		{
			FreeWinding( w );
			return false;
		}

		Vector4Copy( planes[ facet->borderPlanes[ j ] ].plane, plane );

		if ( !facet->borderInward[ j ] )
		{
			VectorSubtract( vec3_origin, plane, plane );
			plane[ 3 ] = -plane[ 3 ];
		}

		ChopWindingInPlace( &w, plane, plane[ 3 ], 0.1f );
	}

	if ( !w )
	{
		return false; // winding was completely chopped away
	}

	// see if the facet is unreasonably large
	WindingBounds( w, bounds[ 0 ], bounds[ 1 ] );
	FreeWinding( w );

	for ( j = 0; j < 3; j++ )
	{
		if ( bounds[ 1 ][ j ] - bounds[ 0 ][ j ] > MAX_WORLD_COORD )
		{
			return false; // we must be missing a plane
		}

		if ( bounds[ 0 ][ j ] >= MAX_WORLD_COORD )
		{
			return false;
		}

		if ( bounds[ 1 ][ j ] <= MIN_WORLD_COORD )
		{
			return false;
		}
	}

	return true; // winding is fine
}

/*
==================
CM_AddFacetBevels
==================
*/
void CM_AddFacetBevels( cFacet_t *facet )
{
	int       i, j, k, l;
	int       axis, dir, order;
	bool  flipped;
	float     plane[ 4 ], d, newplane[ 4 ];
	winding_t *w, *w2;
	vec3_t    mins, maxs, vec, vec2;

	Vector4Copy( planes[ facet->surfacePlane ].plane, plane );

	w = BaseWindingForPlane( plane, plane[ 3 ] );

	for ( j = 0; j < facet->numBorders && w; j++ )
	{
		if ( facet->borderPlanes[ j ] == facet->surfacePlane )
		{
			continue;
		}

		Vector4Copy( planes[ facet->borderPlanes[ j ] ].plane, plane );

		if ( !facet->borderInward[ j ] )
		{
			VectorSubtract( vec3_origin, plane, plane );
			plane[ 3 ] = -plane[ 3 ];
		}

		ChopWindingInPlace( &w, plane, plane[ 3 ], 0.1f );
	}

	if ( !w )
	{
		return;
	}

	WindingBounds( w, mins, maxs );

	// add the axial planes
	order = 0;

	for ( axis = 0; axis < 3; axis++ )
	{
		for ( dir = -1; dir <= 1; dir += 2, order++ )
		{
			VectorClear( plane );
			plane[ axis ] = dir;

			if ( dir == 1 )
			{
				plane[ 3 ] = maxs[ axis ];
			}
			else
			{
				plane[ 3 ] = -mins[ axis ];
			}

			//if it's the surface plane
			if ( CM_PlaneEqual( &planes[ facet->surfacePlane ], plane, &flipped ) )
			{
				continue;
			}

			// see if the plane is already present
			for ( i = 0; i < facet->numBorders; i++ )
			{
				if ( CM_PlaneEqual( &planes[ facet->borderPlanes[ i ] ], plane, &flipped ) )
				{
					break;
				}
			}

			if ( i == facet->numBorders )
			{
				if ( facet->numBorders > MAX_FACET_BEVELS )
				{
					Log::Warn( "too many bevels" );
				}

				facet->borderPlanes[ facet->numBorders ] = CM_FindPlane2( plane, &flipped );
				facet->borderNoAdjust[ facet->numBorders ] = false;
				facet->borderInward[ facet->numBorders ] = flipped;
				facet->numBorders++;
			}
		}
	}

	//
	// add the edge bevels
	//
	// test the non-axial plane edges
	for ( j = 0; j < w->numpoints; j++ )
	{
		k = ( j + 1 ) % w->numpoints;
		VectorSubtract( w->p[ j ], w->p[ k ], vec );

		//if it's a degenerate edge
		if ( VectorNormalize( vec ) < 0.5 )
		{
			continue;
		}

		CM_SnapVector( vec );

		for ( k = 0; k < 3; k++ )
		{
			if ( vec[ k ] == -1 || vec[ k ] == 1 )
			{
				break; // axial
			}
		}

		if ( k < 3 )
		{
			continue; // only test non-axial edges
		}

		// try the six possible slanted axials from this edge
		for ( axis = 0; axis < 3; axis++ )
		{
			for ( dir = -1; dir <= 1; dir += 2 )
			{
				// construct a plane
				VectorClear( vec2 );
				vec2[ axis ] = dir;
				CrossProduct( vec, vec2, plane );

				if ( VectorNormalize( plane ) < 0.5 )
				{
					continue;
				}

				plane[ 3 ] = DotProduct( w->p[ j ], plane );

				// if all the points of the facet winding are
				// behind this plane, it is a proper edge bevel
				for ( l = 0; l < w->numpoints; l++ )
				{
					d = DotProduct( w->p[ l ], plane ) - plane[ 3 ];

					if ( d > 0.1 )
					{
						break; // point in front
					}
				}

				if ( l < w->numpoints )
				{
					continue;
				}

				//if it's the surface plane
				if ( CM_PlaneEqual( &planes[ facet->surfacePlane ], plane, &flipped ) )
				{
					continue;
				}

				// see if the plane is already present
				for ( i = 0; i < facet->numBorders; i++ )
				{
					if ( CM_PlaneEqual( &planes[ facet->borderPlanes[ i ] ], plane, &flipped ) )
					{
						break;
					}
				}

				if ( i == facet->numBorders )
				{
					if ( facet->numBorders > MAX_FACET_BEVELS )
					{
						Log::Warn( "too many bevels" );
					}

					facet->borderPlanes[ facet->numBorders ] = CM_FindPlane2( plane, &flipped );

					for ( k = 0; k < facet->numBorders; k++ )
					{
						if ( facet->borderPlanes[ facet->numBorders ] == facet->borderPlanes[ k ] )
						{
							Log::Warn( "bevel plane already used" );
						}
					}

					facet->borderNoAdjust[ facet->numBorders ] = false;
					facet->borderInward[ facet->numBorders ] = flipped;
					//
					w2 = CopyWinding( w );
					Vector4Copy( planes[ facet->borderPlanes[ facet->numBorders ] ].plane, newplane );

					if ( !facet->borderInward[ facet->numBorders ] )
					{
						VectorNegate( newplane, newplane );
						newplane[ 3 ] = -newplane[ 3 ];
					}

					ChopWindingInPlace( &w2, newplane, newplane[ 3 ], 0.1f );

					if ( !w2 )
					{
						cmLog.Debug( "WARNING: CM_AddFacetBevels... invalid bevel" );
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
	facet->borderNoAdjust[ facet->numBorders ] = false;
	facet->borderInward[ facet->numBorders ] = true;
	facet->numBorders++;
}

/*
==================
CM_GenerateBoundaryForPoints
==================
*/
static int CM_GenerateBoundaryForPoints( const vec4_t triPlane, const vec3_t p1, const vec3_t p2 )
{
	vec3_t up;

	VectorMA( p1, 4, triPlane, up );

	return CM_FindPlane( p1, p2, up );
}

/*
=====================
CM_GenerateFacetFor3Points
=====================
*/
bool CM_GenerateFacetFor3Points( cFacet_t *facet, const vec3_t p1, const vec3_t p2, const vec3_t p3 )
{
	vec4_t          plane;

	// if we can't generate a valid plane for the points, ignore the facet
	//if(!PlaneFromPoints(f->surface, a, b, c, true))
	if ( facet->surfacePlane == -1 )
	{
		facet->numBorders = 0;
		return false;
	}

	Vector4Copy( planes[ facet->surfacePlane ].plane, plane );

	facet->numBorders = 3;

	facet->borderNoAdjust[ 0 ] = false;
	facet->borderNoAdjust[ 1 ] = false;
	facet->borderNoAdjust[ 2 ] = false;

	facet->borderPlanes[ 0 ] = CM_GenerateBoundaryForPoints( plane, p1, p2 );
	facet->borderPlanes[ 1 ] = CM_GenerateBoundaryForPoints( plane, p2, p3 );
	facet->borderPlanes[ 2 ] = CM_GenerateBoundaryForPoints( plane, p3, p1 );

	//VectorCopy(a->xyz, f->points[0]);
	//VectorCopy(b->xyz, f->points[1]);
	//VectorCopy(c->xyz, f->points[2]);

	return true;
}
