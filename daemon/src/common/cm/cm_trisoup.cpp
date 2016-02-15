/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>

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

#include "cm_local.h"

/*
================================================================================

PATCH COLLIDE GENERATION

================================================================================
*/

/*
==================
CM_TrianglePlane
==================
*/

/*static int CM_TrianglePlane(int trianglePlanes[SHADER_MAX_TRIANGLES], int tri)
{
        int             p;

        p = trianglePlanes[tri];
        if(p != -1)
        {
                //Log::Notice( "CM_TrianglePlane %i %i", tri, p);
                return p;
        }

        // should never happen
		Log::Warn( "CM_TrianglePlane unresolvable" );
        return -1;
}*/

/*
==================
CM_EdgePlaneNum
==================
*/

/*
static int CM_EdgePlaneNum(cTriangleSoup_t * triSoup, int tri, int edgeType)
{
        float          *p1, *p2;
        vec3_t          up;
        int             p;

//  Log::Notice( "CM_EdgePlaneNum: %i %i", tri, edgeType );

        switch (edgeType)
        {
                case 0:
                        p1 = triSoup->points[tri][0];
                        p2 = triSoup->points[tri][1];
                        p = CM_TrianglePlane(triSoup->trianglePlanes, tri);
                        if(p == -1)
                                break;
                        VectorMA(p1, 4, planes[p].plane, up);
                        return CM_FindPlane(p1, p2, up);

                case 1:
                        p1 = triSoup->points[tri][1];
                        p2 = triSoup->points[tri][2];
                        p = CM_TrianglePlane(triSoup->trianglePlanes, tri);
                        if(p == -1)
                                break;
                        VectorMA(p1, 4, planes[p].plane, up);
                        return CM_FindPlane(p1, p2, up);

                case 2:
                        p1 = triSoup->points[tri][2];
                        p2 = triSoup->points[tri][0];
                        p = CM_TrianglePlane(triSoup->trianglePlanes, tri);
                        if(p == -1)
                                break;
                        VectorMA(p1, 4, planes[p].plane, up);
                        return CM_FindPlane(p2, p1, up);

                default:
                        Sys::Drop("CM_EdgePlaneNum: bad edgeType=%i", edgeType);

        }

        return -1;
}
*/

/*
===================
CM_SetBorderInward
===================
*/
static void CM_SetBorderInward( cFacet_t *facet, cTriangleSoup_t *triSoup, int i, int which )
{
	int   k, l;
	float *points[ 4 ];
	int   numPoints;

	switch ( which )
	{
		case 0:
			points[ 0 ] = triSoup->points[ i ][ 0 ];
			points[ 1 ] = triSoup->points[ i ][ 1 ];
			points[ 2 ] = triSoup->points[ i ][ 2 ];
			numPoints = 3;
			break;

		case 1:
			points[ 0 ] = triSoup->points[ i ][ 2 ];
			points[ 1 ] = triSoup->points[ i ][ 1 ];
			points[ 2 ] = triSoup->points[ i ][ 0 ];
			numPoints = 3;
			break;

		default:
			Sys::Error( "CM_SetBorderInward: bad parameter %i", which );
	}

	for ( k = 0; k < facet->numBorders; k++ )
	{
		int front, back;

		front = 0;
		back = 0;

		for ( l = 0; l < numPoints; l++ )
		{
			auto side = CM_PointOnPlaneSide( points[ l ], facet->borderPlanes[ k ] );

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
			cmLog.Debug( "WARNING: CM_SetBorderInward: mixed plane sides" );
			facet->borderInward[ k ] = false;
		}
	}
}

/*
==================
CM_SurfaceCollideFromTriangleSoup
==================
*/
static void CM_SurfaceCollideFromTriangleSoup( cTriangleSoup_t *triSoup, cSurfaceCollide_t *sc )
{
	int             i;
	float          *p1, *p2, *p3;
//	int             i1, i2, i3;

	cFacet_t       *facet;

	CM_ResetPlaneCounts();

	// find the planes for each triangle of the grid
	for ( i = 0; i < triSoup->numTriangles; i++ )
	{
		p1 = triSoup->points[ i ][ 0 ];
		p2 = triSoup->points[ i ][ 1 ];
		p3 = triSoup->points[ i ][ 2 ];

		triSoup->trianglePlanes[ i ] = CM_FindPlane( p1, p2, p3 );

		//Log::Notive( "trianglePlane[%i] = %i", i, trianglePlanes[i]);
	}

	// create the borders for each triangle
	for ( i = 0; i < triSoup->numTriangles; i++ )
	{
		facet = &facets[ numFacets ];
		Com_Memset( facet, 0, sizeof( *facet ) );

		p1 = triSoup->points[ i ][ 0 ];
		p2 = triSoup->points[ i ][ 1 ];
		p3 = triSoup->points[ i ][ 2 ];

		facet->surfacePlane = triSoup->trianglePlanes[ i ]; //CM_FindPlane(p1, p2, p3);

		// try and make a quad out of two triangles
#if 0
		i1 = triSoup->indexes[ i * 3 + 0 ];
		i2 = triSoup->indexes[ i * 3 + 1 ];
		i3 = triSoup->indexes[ i * 3 + 2 ];

		if ( i != triSoup->numTriangles - 1 )
		{
			i4 = triSoup->indexes[ i * 3 + 3 ];
			i5 = triSoup->indexes[ i * 3 + 4 ];
			i6 = triSoup->indexes[ i * 3 + 5 ];

			if ( i4 == i3 && i5 == i2 )
			{
				p4 = triSoup->points[ i ][ 5 ]; // vertex at i6

				if ( CM_GenerateFacetFor4Points( facet, p1, p2, p4, p3 ) )  //test->facets[count], v1, v2, v4, v3))
				{
					CM_SetBorderInward( facet, triSoup, i, 0 );

					if ( CM_ValidateFacet( facet ) )
					{
						CM_AddFacetBevels( facet );
						numFacets++;

						i++; // skip next tri
						continue;
					}
				}
			}
		}

#endif

		if ( CM_GenerateFacetFor3Points( facet, p1, p2, p3 ) )
		{
			CM_SetBorderInward( facet, triSoup, i, 0 );

			if ( CM_ValidateFacet( facet ) )
			{
				CM_AddFacetBevels( facet );
				numFacets++;
			}
		}
	}

	// copy the results out
	sc->numPlanes = numPlanes;
	sc->planes = ( cPlane_t * ) CM_Alloc( numPlanes * sizeof( *sc->planes ) );
	Com_Memcpy( sc->planes, planes, numPlanes * sizeof( *sc->planes ) );

	sc->numFacets = numFacets;
	sc->facets = ( cFacet_t * ) CM_Alloc( numFacets * sizeof( *sc->facets ) );
	Com_Memcpy( sc->facets, facets, numFacets * sizeof( *sc->facets ) );
}

/*
===================
CM_GenerateTriangleSoupCollide

Creates an internal structure that will be used to perform
collision detection with a triangle soup mesh.

Points is packed as concatenated rows.
===================
*/
cSurfaceCollide_t *CM_GenerateTriangleSoupCollide( int numVertexes, vec3_t *vertexes, int numIndexes, int *indexes )
{
	cSurfaceCollide_t *sc;
	static cTriangleSoup_t triSoup;
	int             i, j;

	if ( numVertexes <= 2 || !vertexes || numIndexes <= 2 || !indexes )
	{
		Sys::Drop( "CM_GenerateTriangleSoupCollide: bad parameters: (%i, %p, %i, %p)", numVertexes, vertexes, numIndexes,
		           indexes );
	}

	if ( numIndexes > SHADER_MAX_INDEXES )
	{
		Sys::Drop( "CM_GenerateTriangleSoupCollide: source is > SHADER_MAX_TRIANGLES" );
	}

	// build a triangle soup
	triSoup.numTriangles = numIndexes / 3;

	for ( i = 0; i < triSoup.numTriangles; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			triSoup.indexes[ i * 3 + j ] = indexes[ i * 3 + j ];

			//VectorCopy(
			VectorCopy( vertexes[ indexes[ i * 3 + j ] ], triSoup.points[ i ][ j ] );
		}
	}

	//for(i = 0; i < triSoup.num

	sc = ( cSurfaceCollide_t * ) CM_Alloc( sizeof( *sc ) );
	ClearBounds( sc->bounds[ 0 ], sc->bounds[ 1 ] );

	for ( i = 0; i < triSoup.numTriangles; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			AddPointToBounds( triSoup.points[ i ][ j ], sc->bounds[ 0 ], sc->bounds[ 1 ] );
		}
	}

	// generate a bsp tree for the surface
	CM_SurfaceCollideFromTriangleSoup( &triSoup, sc );

	// expand by one unit for epsilon purposes
	sc->bounds[ 0 ][ 0 ] -= 1;
	sc->bounds[ 0 ][ 1 ] -= 1;
	sc->bounds[ 0 ][ 2 ] -= 1;

	sc->bounds[ 1 ][ 0 ] += 1;
	sc->bounds[ 1 ][ 1 ] += 1;
	sc->bounds[ 1 ][ 2 ] += 1;

	cmLog.Debug( "CM_GenerateTriangleSoupCollide: %i planes %i facets", sc->numPlanes, sc->numFacets );

	return sc;
}
