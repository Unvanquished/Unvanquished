/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_curve.c
#include "tr_local.h"

/*
======================================================================================
This file does all of the processing necessary to turn a raw grid of points
read from the map file into a srfGridMesh_t ready for rendering.

The level of detail solution is direction independent, based only on subdivided
distance from the true curve.

Only a single entry point:
R_SubdividePatchToGrid(int width, int height, srfVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE])
======================================================================================
*/

/*
============
LerpDrawVert
============
*/
static void LerpSurfaceVert( srfVert_t *a, srfVert_t *b, srfVert_t *out )
{
	out->xyz[ 0 ]            = 0.5f * ( a->xyz[ 0 ] + b->xyz[ 0 ] );
	out->xyz[ 1 ]            = 0.5f * ( a->xyz[ 1 ] + b->xyz[ 1 ] );
	out->xyz[ 2 ]            = 0.5f * ( a->xyz[ 2 ] + b->xyz[ 2 ] );

	out->st[ 0 ]             = 0.5f * ( a->st[ 0 ] + b->st[ 0 ] );
	out->st[ 1 ]             = 0.5f * ( a->st[ 1 ] + b->st[ 1 ] );

	out->lightmap[ 0 ]       = 0.5f * ( a->lightmap[ 0 ] + b->lightmap[ 0 ] );
	out->lightmap[ 1 ]       = 0.5f * ( a->lightmap[ 1 ] + b->lightmap[ 1 ] );

	out->paintColor[ 0 ]     = ( a->paintColor[ 0 ] + b->paintColor[ 0 ] ) * 0.5f;
	out->paintColor[ 1 ]     = ( a->paintColor[ 1 ] + b->paintColor[ 1 ] ) * 0.5f;
	out->paintColor[ 2 ]     = ( a->paintColor[ 2 ] + b->paintColor[ 2 ] ) * 0.5f;
	out->paintColor[ 3 ]     = ( a->paintColor[ 3 ] + b->paintColor[ 3 ] ) * 0.5f;

	out->lightColor[ 0 ]     = ( a->lightColor[ 0 ] + b->lightColor[ 0 ] ) * 0.5f;
	out->lightColor[ 1 ]     = ( a->lightColor[ 1 ] + b->lightColor[ 1 ] ) * 0.5f;
	out->lightColor[ 2 ]     = ( a->lightColor[ 2 ] + b->lightColor[ 2 ] ) * 0.5f;
	out->lightColor[ 3 ]     = ( a->lightColor[ 3 ] + b->lightColor[ 3 ] ) * 0.5f;

	out->lightDirection[ 0 ] = ( a->lightDirection[ 0 ] + b->lightDirection[ 0 ] ) * 0.5f;
	out->lightDirection[ 1 ] = ( a->lightDirection[ 1 ] + b->lightDirection[ 1 ] ) * 0.5f;
	out->lightDirection[ 2 ] = ( a->lightDirection[ 2 ] + b->lightDirection[ 2 ] ) * 0.5f;
}

/*
============
Transpose
============
*/
static void Transpose( int width, int height, srfVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ] )
{
	int       i, j;
	srfVert_t temp;

	if ( width > height )
	{
		for ( i = 0; i < height; i++ )
		{
			for ( j = i + 1; j < width; j++ )
			{
				if ( j < height )
				{
					// swap the value
					temp           = ctrl[ j ][ i ];
					ctrl[ j ][ i ] = ctrl[ i ][ j ];
					ctrl[ i ][ j ] = temp;
				}
				else
				{
					// just copy
					ctrl[ j ][ i ] = ctrl[ i ][ j ];
				}
			}
		}
	}
	else
	{
		for ( i = 0; i < width; i++ )
		{
			for ( j = i + 1; j < height; j++ )
			{
				if ( j < width )
				{
					// swap the value
					temp           = ctrl[ i ][ j ];
					ctrl[ i ][ j ] = ctrl[ j ][ i ];
					ctrl[ j ][ i ] = temp;
				}
				else
				{
					// just copy
					ctrl[ i ][ j ] = ctrl[ j ][ i ];
				}
			}
		}
	}
}

/*
=================
MakeMeshNormals

Handles all the complicated wrapping and degenerate cases
=================
*/
static void MakeMeshNormals( int width, int height, srfVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ] )
{
	int        i, j, k, dist;
	vec3_t     normal;
	vec3_t     sum;
	int        count;
	vec3_t     base;
	vec3_t     delta;
	int        x, y;
	srfVert_t  *dv;
	vec3_t     around[ 8 ], temp;
	qboolean   good[ 8 ];
	qboolean   wrapWidth, wrapHeight;
	float      len;
	static int neighbors[ 8 ][ 2 ] =
	{
		{ 0, 1 }, { 1, 1 }, { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, -1 }, { -1, 0 }, { -1, 1 }
	};

	wrapWidth = qfalse;

	for ( i = 0; i < height; i++ )
	{
		VectorSubtract( ctrl[ i ][ 0 ].xyz, ctrl[ i ][ width - 1 ].xyz, delta );
		len = VectorLengthSquared( delta );

		if ( len > 1.0 )
		{
			break;
		}
	}

	if ( i == height )
	{
		wrapWidth = qtrue;
	}

	wrapHeight = qfalse;

	for ( i = 0; i < width; i++ )
	{
		VectorSubtract( ctrl[ 0 ][ i ].xyz, ctrl[ height - 1 ][ i ].xyz, delta );
		len = VectorLengthSquared( delta );

		if ( len > 1.0 )
		{
			break;
		}
	}

	if ( i == width )
	{
		wrapHeight = qtrue;
	}

	for ( i = 0; i < width; i++ )
	{
		for ( j = 0; j < height; j++ )
		{
			count = 0;
			dv    = &ctrl[ j ][ i ];
			VectorCopy( dv->xyz, base );

			for ( k = 0; k < 8; k++ )
			{
				VectorClear( around[ k ] );
				good[ k ] = qfalse;

				for ( dist = 1; dist <= 3; dist++ )
				{
					x = i + neighbors[ k ][ 0 ] * dist;
					y = j + neighbors[ k ][ 1 ] * dist;

					if ( wrapWidth )
					{
						if ( x < 0 )
						{
							x = width - 1 + x;
						}
						else if ( x >= width )
						{
							x = 1 + x - width;
						}
					}

					if ( wrapHeight )
					{
						if ( y < 0 )
						{
							y = height - 1 + y;
						}
						else if ( y >= height )
						{
							y = 1 + y - height;
						}
					}

					if ( x < 0 || x >= width || y < 0 || y >= height )
					{
						break;  // edge of patch
					}

					VectorSubtract( ctrl[ y ][ x ].xyz, base, temp );

					if ( VectorNormalize2( temp, temp ) == 0 )
					{
						continue;       // degenerate edge, get more dist
					}
					else
					{
						good[ k ] = qtrue;
						VectorCopy( temp, around[ k ] );
						break;  // good edge
					}
				}
			}

			VectorClear( sum );

			for ( k = 0; k < 8; k++ )
			{
				if ( !good[ k ] || !good[ ( k + 1 ) & 7 ] )
				{
					continue;       // didn't get two points
				}

				CrossProduct( around[ ( k + 1 ) & 7 ], around[ k ], normal );

				if ( VectorNormalize2( normal, normal ) == 0 )
				{
					continue;
				}

				VectorAdd( normal, sum, sum );
				count++;
			}

			if ( count == 0 )
			{
//printf("bad normal\n");
				count = 1;
			}

			VectorNormalize2( sum, dv->normal );
		}
	}
}

static void MakeMeshTangentVectors( int width, int height, srfVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ], int numTriangles,
                                    srfTriangle_t triangles[ SHADER_MAX_TRIANGLES ] )
{
	int              i, j;
	srfVert_t        *dv[ 3 ];
	static srfVert_t ctrl2[ MAX_GRID_SIZE * MAX_GRID_SIZE ];
	srfTriangle_t    *tri;

	// FIXME: use more elegant way
	for ( i = 0; i < width; i++ )
	{
		for ( j = 0; j < height; j++ )
		{
			dv[ 0 ]  = &ctrl2[ j * width + i ];
			*dv[ 0 ] = ctrl[ j ][ i ];
		}
	}

	for ( i = 0, tri = triangles; i < numTriangles; i++, tri++ )
	{
		dv[ 0 ] = &ctrl2[ tri->indexes[ 0 ] ];
		dv[ 1 ] = &ctrl2[ tri->indexes[ 1 ] ];
		dv[ 2 ] = &ctrl2[ tri->indexes[ 2 ] ];

		R_CalcTangentVectors( dv );
	}

#if 0

	for ( i = 0; i < ( width * height ); i++ )
	{
		dv0 = &ctrl2[ i ];

		VectorNormalize( dv0->normal );
#if 0
		VectorNormalize( dv0->tangent );
		VectorNormalize( dv0->binormal );
#else
		d = DotProduct( dv0->tangent, dv0->normal );
		VectorMA( dv0->tangent, -d, dv0->normal, dv0->tangent );
		VectorNormalize( dv0->tangent );

		d = DotProduct( dv0->binormal, dv0->normal );
		VectorMA( dv0->binormal, -d, dv0->normal, dv0->binormal );
		VectorNormalize( dv0->binormal );
#endif
	}

#endif

#if 0

	// do another extra smoothing for normals to avoid flat shading
	for ( i = 0; i < ( width * height ); i++ )
	{
		for ( j = 0; j < ( width * height ); j++ )
		{
			if ( R_CompareVert( &ctrl2[ i ], &ctrl2[ j ], qfalse ) )
			{
				VectorAdd( ctrl2[ i ].normal, ctrl2[ j ].normal, ctrl2[ i ].normal );
			}
		}

		VectorNormalize( ctrl2[ i ].normal );
	}

#endif

	for ( i = 0; i < width; i++ )
	{
		for ( j = 0; j < height; j++ )
		{
			dv[ 0 ] = &ctrl2[ j * width + i ];
			dv[ 1 ] = &ctrl[ j ][ i ];

			VectorCopy( dv[ 0 ]->tangent, dv[ 1 ]->tangent );
			VectorCopy( dv[ 0 ]->binormal, dv[ 1 ]->binormal );
		}
	}
}

static int MakeMeshTriangles( int width, int height, srfVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ],
                              srfTriangle_t triangles[ SHADER_MAX_TRIANGLES ] )
{
	int              i, j;
	int              numTriangles;
	int              w, h;
	srfVert_t        *dv;
	static srfVert_t ctrl2[ MAX_GRID_SIZE * MAX_GRID_SIZE ];

	h            = height - 1;
	w            = width - 1;
	numTriangles = 0;

	for ( i = 0; i < h; i++ )
	{
		for ( j = 0; j < w; j++ )
		{
			int v1, v2, v3, v4;

			// vertex order to be reckognized as tristrips
			v1                                     = i * width + j + 1;
			v2                                     = v1 - 1;
			v3                                     = v2 + width;
			v4                                     = v3 + 1;

			triangles[ numTriangles ].indexes[ 0 ] = v2;
			triangles[ numTriangles ].indexes[ 1 ] = v3;
			triangles[ numTriangles ].indexes[ 2 ] = v1;
			numTriangles++;

			triangles[ numTriangles ].indexes[ 0 ] = v1;
			triangles[ numTriangles ].indexes[ 1 ] = v3;
			triangles[ numTriangles ].indexes[ 2 ] = v4;
			numTriangles++;
		}
	}

	R_CalcSurfaceTriangleNeighbors( numTriangles, triangles );

	// FIXME: use more elegant way
	for ( i = 0; i < width; i++ )
	{
		for ( j = 0; j < height; j++ )
		{
			dv  = &ctrl2[ j * width + i ];
			*dv = ctrl[ j ][ i ];
		}
	}

	R_CalcSurfaceTrianglePlanes( numTriangles, triangles, ctrl2 );

	return numTriangles;
}

/*static void MakeTangentSpaces(int width, int height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE], int numTriangles,
                                                          srfTriangle_t triangles[SHADER_MAX_TRIANGLES])
{
        int             i, j;
        float          *v;
        const float    *v0, *v1, *v2;
        const float    *t0, *t1, *t2;
        vec3_t          tangent;
        vec3_t          binormal;
        vec3_t          normal;
        vec_t           d;
        srfVert_t      *dv0, *dv1, *dv2;
        srfVert_t       ctrl2[MAX_GRID_SIZE * MAX_GRID_SIZE];
        srfTriangle_t  *tri;

        // FIXME: use more elegant way
        for(i = 0; i < width; i++)
        {
                for(j = 0; j < height; j++)
                {
                        dv0 = &ctrl2[j * width + i];
                        *dv0 = ctrl[j][i];
                }
        }

        for(i = 0; i < (width * height); i++)
        {
                dv0 = &ctrl2[i];

                VectorClear(dv0->tangent);
                VectorClear(dv0->binormal);
                VectorClear(dv0->normal);
        }

        for(i = 0, tri = triangles; i < numTriangles; i++, tri++)
        {
                dv0 = &ctrl2[tri->indexes[0]];
                dv1 = &ctrl2[tri->indexes[1]];
                dv2 = &ctrl2[tri->indexes[2]];

                v0 = dv0->xyz;
                v1 = dv1->xyz;
                v2 = dv2->xyz;

                t0 = dv0->st;
                t1 = dv1->st;
                t2 = dv2->st;

                R_CalcTangentSpace2(tangent, binormal, normal, v0, v1, v2, t0, t1, t2);

                for(j = 0; j < 3; j++)
                {
                        dv0 = &ctrl2[tri->indexes[j]];

                        v = dv0->tangent;
                        VectorAdd(v, tangent, v);

                        v = dv0->binormal;
                        VectorAdd(v, binormal, v);

                        v = dv0->normal;
                        VectorAdd(v, normal, v);
                }
        }

        for(i = 0; i < (width * height); i++)
        {
                dv0 = &ctrl2[i];

                VectorNormalize(dv0->normal);
#if 0
                VectorNormalize(dv0->tangent);
                VectorNormalize(dv0->binormal);
#else
                d = DotProduct(dv0->tangent, dv0->normal);
                VectorMA(dv0->tangent, -d, dv0->normal, dv0->tangent);
                VectorNormalize(dv0->tangent);

                d = DotProduct(dv0->binormal, dv0->normal);
                VectorMA(dv0->binormal, -d, dv0->normal, dv0->binormal);
                VectorNormalize(dv0->binormal);
#endif
        }

        // do another extra smoothing for normals to avoid flat shading
        for(i = 0; i < (width * height); i++)
        {
                for(j = 0; j < (width * height); j++)
                {
                        if(R_CompareVert(&ctrl2[i], &ctrl2[j], qfalse))
                        {
                                VectorAdd(ctrl2[i].normal, ctrl2[j].normal, ctrl2[i].normal);
                        }
                }

                VectorNormalize(ctrl2[i].normal);
        }

        for(i = 0; i < width; i++)
        {
                for(j = 0; j < height; j++)
                {
                        dv0 = &ctrl2[j * width + i];
                        dv1 = &ctrl[j][i];

                        VectorCopy(dv0->tangent, dv1->tangent);
                        VectorCopy(dv0->binormal, dv1->binormal);
                        VectorCopy(dv0->normal, dv1->normal);
                }
        }
}*/

/*
============
InvertCtrl
============
*/
static void InvertCtrl( int width, int height, srfVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ] )
{
	int       i, j;
	srfVert_t temp;

	for ( i = 0; i < height; i++ )
	{
		for ( j = 0; j < width / 2; j++ )
		{
			temp                       = ctrl[ i ][ j ];
			ctrl[ i ][ j ]             = ctrl[ i ][ width - 1 - j ];
			ctrl[ i ][ width - 1 - j ] = temp;
		}
	}
}

/*
=================
InvertErrorTable
=================
*/
static void InvertErrorTable( float errorTable[ 2 ][ MAX_GRID_SIZE ], int width, int height )
{
	int   i;
	float copy[ 2 ][ MAX_GRID_SIZE ];

	Com_Memcpy( copy, errorTable, sizeof( copy ) );

	for ( i = 0; i < width; i++ )
	{
		errorTable[ 1 ][ i ] = copy[ 0 ][ i ];  //[width-1-i];
	}

	for ( i = 0; i < height; i++ )
	{
		errorTable[ 0 ][ i ] = copy[ 1 ][ height - 1 - i ];
	}
}

/*
==================
PutPointsOnCurve
==================
*/
static void PutPointsOnCurve( srfVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ], int width, int height )
{
	int       i, j;
	srfVert_t prev, next;

	for ( i = 0; i < width; i++ )
	{
		for ( j = 1; j < height; j += 2 )
		{
			LerpSurfaceVert( &ctrl[ j ][ i ], &ctrl[ j + 1 ][ i ], &prev );
			LerpSurfaceVert( &ctrl[ j ][ i ], &ctrl[ j - 1 ][ i ], &next );
			LerpSurfaceVert( &prev, &next, &ctrl[ j ][ i ] );
		}
	}

	for ( j = 0; j < height; j++ )
	{
		for ( i = 1; i < width; i += 2 )
		{
			LerpSurfaceVert( &ctrl[ j ][ i ], &ctrl[ j ][ i + 1 ], &prev );
			LerpSurfaceVert( &ctrl[ j ][ i ], &ctrl[ j ][ i - 1 ], &next );
			LerpSurfaceVert( &prev, &next, &ctrl[ j ][ i ] );
		}
	}
}

/*
=================
R_CreateSurfaceGridMesh
=================
*/
static srfGridMesh_t *R_CreateSurfaceGridMesh( int width, int height,
    srfVert_t ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ],
    float errorTable[ 2 ][ MAX_GRID_SIZE ],
    int numTriangles, srfTriangle_t triangles[ SHADER_MAX_TRIANGLES ] )
{
	int           i, j, size;
	srfVert_t     *vert;
	vec3_t        tmpVec;
	srfGridMesh_t *grid;

	// copy the results out to a grid
	size = sizeof( *grid );

	if ( r_stitchCurves->integer )
	{
		grid                 = /*ri.Hunk_Alloc */ Com_Allocate( size );
		Com_Memset( grid, 0, size );

		grid->widthLodError  = /*ri.Hunk_Alloc */ Com_Allocate( width * 4 );
		Com_Memcpy( grid->widthLodError, errorTable[ 0 ], width * 4 );

		grid->heightLodError = /*ri.Hunk_Alloc */ Com_Allocate( height * 4 );
		Com_Memcpy( grid->heightLodError, errorTable[ 1 ], height * 4 );

		grid->numTriangles   = numTriangles;
		grid->triangles      = Com_Allocate( grid->numTriangles * sizeof( srfTriangle_t ) );
		Com_Memcpy( grid->triangles, triangles, numTriangles * sizeof( srfTriangle_t ) );

		grid->numVerts       = ( width * height );
		grid->verts          = Com_Allocate( grid->numVerts * sizeof( srfVert_t ) );
	}
	else
	{
		grid                 = ri.Hunk_Alloc( size, h_low );
		Com_Memset( grid, 0, size );

		grid->widthLodError  = ri.Hunk_Alloc( width * 4, h_low );
		Com_Memcpy( grid->widthLodError, errorTable[ 0 ], width * 4 );

		grid->heightLodError = ri.Hunk_Alloc( height * 4, h_low );
		Com_Memcpy( grid->heightLodError, errorTable[ 1 ], height * 4 );

		grid->numTriangles   = numTriangles;
		grid->triangles      = ri.Hunk_Alloc( grid->numTriangles * sizeof( srfTriangle_t ), h_low );
		Com_Memcpy( grid->triangles, triangles, numTriangles * sizeof( srfTriangle_t ) );

		grid->numVerts       = ( width * height );
		grid->verts          = ri.Hunk_Alloc( grid->numVerts * sizeof( srfVert_t ), h_low );
	}

	grid->width       = width;
	grid->height      = height;
	grid->surfaceType = SF_GRID;
	ClearBounds( grid->bounds[ 0 ], grid->bounds[ 1 ] );

	for ( i = 0; i < width; i++ )
	{
		for ( j = 0; j < height; j++ )
		{
			vert  = &grid->verts[ j * width + i ];
			*vert = ctrl[ j ][ i ];
			AddPointToBounds( vert->xyz, grid->bounds[ 0 ], grid->bounds[ 1 ] );
		}
	}

	// compute local origin and bounds
	VectorAdd( grid->bounds[ 0 ], grid->bounds[ 1 ], grid->origin );
	VectorScale( grid->origin, 0.5f, grid->origin );
	VectorSubtract( grid->bounds[ 0 ], grid->origin, tmpVec );
	grid->radius    = VectorLength( tmpVec );

	VectorCopy( grid->origin, grid->lodOrigin );
	grid->lodRadius = grid->radius;
	//
	return grid;
}

/*
=================
R_FreeSurfaceGridMesh
=================
*/
void R_FreeSurfaceGridMesh( srfGridMesh_t *grid )
{
	Com_Dealloc( grid->widthLodError );
	Com_Dealloc( grid->heightLodError );
	Com_Dealloc( grid->triangles );
	Com_Dealloc( grid->verts );
	Com_Dealloc( grid );
}

/*
=================
R_SubdividePatchToGrid
=================
*/
srfGridMesh_t  *R_SubdividePatchToGrid( int width, int height, srfVert_t points[ MAX_PATCH_SIZE *MAX_PATCH_SIZE ] )
{
	int                  i, j, k, l;
	srfVert_t            prev, next, mid;
	float                len, maxLen;
	int                  dir;
	int                  t;
	static srfVert_t     ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ];
	float                errorTable[ 2 ][ MAX_GRID_SIZE ];
	int                  numTriangles;
	static srfTriangle_t triangles[ SHADER_MAX_TRIANGLES ];

	for ( i = 0; i < width; i++ )
	{
		for ( j = 0; j < height; j++ )
		{
			ctrl[ j ][ i ] = points[ j * width + i ];
		}
	}

	for ( dir = 0; dir < 2; dir++ )
	{
		for ( j = 0; j < MAX_GRID_SIZE; j++ )
		{
			errorTable[ dir ][ j ] = 0;
		}

		// horizontal subdivisions
		for ( j = 0; j + 2 < width; j += 2 )
		{
			// check subdivided midpoints against control points

			// FIXME: also check midpoints of adjacent patches against the control points
			// this would basically stitch all patches in the same LOD group together.

			maxLen = 0;

			for ( i = 0; i < height; i++ )
			{
				vec3_t midxyz;
				vec3_t midxyz2;
				vec3_t dir;
				vec3_t projected;
				float  d;

				// calculate the point on the curve
				for ( l = 0; l < 3; l++ )
				{
					midxyz[ l ] = ( ctrl[ i ][ j ].xyz[ l ] + ctrl[ i ][ j + 1 ].xyz[ l ] * 2 + ctrl[ i ][ j + 2 ].xyz[ l ] ) * 0.25f;
				}

				// see how far off the line it is
				// using dist-from-line will not account for internal
				// texture warping, but it gives a lot less polygons than
				// dist-from-midpoint
				VectorSubtract( midxyz, ctrl[ i ][ j ].xyz, midxyz );
				VectorSubtract( ctrl[ i ][ j + 2 ].xyz, ctrl[ i ][ j ].xyz, dir );
				VectorNormalize( dir );

				d   = DotProduct( midxyz, dir );
				VectorScale( dir, d, projected );
				VectorSubtract( midxyz, projected, midxyz2 );
				len = VectorLengthSquared( midxyz2 );   // we will do the sqrt later

				if ( len > maxLen )
				{
					maxLen = len;
				}
			}

			maxLen = sqrt( maxLen );

			// if all the points are on the lines, remove the entire columns
			if ( maxLen < 0.1f )
			{
				errorTable[ dir ][ j + 1 ] = 999;
				continue;
			}

			// see if we want to insert subdivided columns
			if ( width + 2 > MAX_GRID_SIZE )
			{
				errorTable[ dir ][ j + 1 ] = 1.0f / maxLen;
				continue;               // can't subdivide any more
			}

			if ( maxLen <= r_subdivisions->value )
			{
				errorTable[ dir ][ j + 1 ] = 1.0f / maxLen;
				continue;               // didn't need subdivision
			}

			errorTable[ dir ][ j + 2 ] = 1.0f / maxLen;

			// insert two columns and replace the peak
			width                     += 2;

			for ( i = 0; i < height; i++ )
			{
				LerpSurfaceVert( &ctrl[ i ][ j ], &ctrl[ i ][ j + 1 ], &prev );
				LerpSurfaceVert( &ctrl[ i ][ j + 1 ], &ctrl[ i ][ j + 2 ], &next );
				LerpSurfaceVert( &prev, &next, &mid );

				for ( k = width - 1; k > j + 3; k-- )
				{
					ctrl[ i ][ k ] = ctrl[ i ][ k - 2 ];
				}

				ctrl[ i ][ j + 1 ] = prev;
				ctrl[ i ][ j + 2 ] = mid;
				ctrl[ i ][ j + 3 ] = next;
			}

			// back up and recheck this set again, it may need more subdivision
			j -= 2;
		}

		Transpose( width, height, ctrl );
		t      = width;
		width  = height;
		height = t;
	}

	// put all the aproximating points on the curve
	PutPointsOnCurve( ctrl, width, height );

	// cull out any rows or columns that are colinear
	for ( i = 1; i < width - 1; i++ )
	{
		if ( errorTable[ 0 ][ i ] != 999 )
		{
			continue;
		}

		for ( j = i + 1; j < width; j++ )
		{
			for ( k = 0; k < height; k++ )
			{
				ctrl[ k ][ j - 1 ] = ctrl[ k ][ j ];
			}

			errorTable[ 0 ][ j - 1 ] = errorTable[ 0 ][ j ];
		}

		width--;
	}

	for ( i = 1; i < height - 1; i++ )
	{
		if ( errorTable[ 1 ][ i ] != 999 )
		{
			continue;
		}

		for ( j = i + 1; j < height; j++ )
		{
			for ( k = 0; k < width; k++ )
			{
				ctrl[ j - 1 ][ k ] = ctrl[ j ][ k ];
			}

			errorTable[ 1 ][ j - 1 ] = errorTable[ 1 ][ j ];
		}

		height--;
	}

#if 1

	// flip for longest tristrips as an optimization
	// the results should be visually identical with or
	// without this step
	if ( height > width )
	{
		Transpose( width, height, ctrl );
		InvertErrorTable( errorTable, width, height );
		t      = width;
		width  = height;
		height = t;
		InvertCtrl( width, height, ctrl );
	}

#endif

	// calculate triangles
	numTriangles = MakeMeshTriangles( width, height, ctrl, triangles );

	// calculate normals
	MakeMeshNormals( width, height, ctrl );
	MakeMeshTangentVectors( width, height, ctrl, numTriangles, triangles );

	// calculate tangent spaces
	//MakeTangentSpaces(width, height, ctrl, numTriangles, triangles);

	return R_CreateSurfaceGridMesh( width, height, ctrl, errorTable, numTriangles, triangles );
}

/*
===============
R_GridInsertColumn
===============
*/
srfGridMesh_t  *R_GridInsertColumn( srfGridMesh_t *grid, int column, int row, vec3_t point, float loderror )
{
	int                  i, j;
	int                  width, height, oldwidth;
	static srfVert_t     ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ];
	float                errorTable[ 2 ][ MAX_GRID_SIZE ];
	float                lodRadius;
	vec3_t               lodOrigin;
	int                  numTriangles;
	static srfTriangle_t triangles[ SHADER_MAX_TRIANGLES ];

	oldwidth = 0;
	width    = grid->width + 1;

	if ( width > MAX_GRID_SIZE )
	{
		return NULL;
	}

	height   = grid->height;

	for ( i = 0; i < width; i++ )
	{
		if ( i == column )
		{
			//insert new column
			for ( j = 0; j < grid->height; j++ )
			{
				LerpSurfaceVert( &grid->verts[ j * grid->width + i - 1 ], &grid->verts[ j * grid->width + i ], &ctrl[ j ][ i ] );

				if ( j == row )
				{
					VectorCopy( point, ctrl[ j ][ i ].xyz );
				}
			}

			errorTable[ 0 ][ i ] = loderror;
			continue;
		}

		errorTable[ 0 ][ i ] = grid->widthLodError[ oldwidth ];

		for ( j = 0; j < grid->height; j++ )
		{
			ctrl[ j ][ i ] = grid->verts[ j * grid->width + oldwidth ];
		}

		oldwidth++;
	}

	for ( j = 0; j < grid->height; j++ )
	{
		errorTable[ 1 ][ j ] = grid->heightLodError[ j ];
	}

	// put all the aproximating points on the curve
	//PutPointsOnCurve( ctrl, width, height );

	// calculate triangles
	numTriangles = MakeMeshTriangles( width, height, ctrl, triangles );

	// calculate normals
	MakeMeshNormals( width, height, ctrl );
	MakeMeshTangentVectors( width, height, ctrl, numTriangles, triangles );

	// calculate tangent spaces
	//MakeTangentSpaces(width, height, ctrl, numTriangles, triangles);

	VectorCopy( grid->lodOrigin, lodOrigin );
	lodRadius = grid->lodRadius;
	// free the old grid
	R_FreeSurfaceGridMesh( grid );
	// create a new grid
	grid            = R_CreateSurfaceGridMesh( width, height, ctrl, errorTable, numTriangles, triangles );
	grid->lodRadius = lodRadius;
	VectorCopy( lodOrigin, grid->lodOrigin );
	return grid;
}

/*
===============
R_GridInsertRow
===============
*/
srfGridMesh_t  *R_GridInsertRow( srfGridMesh_t *grid, int row, int column, vec3_t point, float loderror )
{
	int                  i, j;
	int                  width, height, oldheight;
	static srfVert_t     ctrl[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ];
	float                errorTable[ 2 ][ MAX_GRID_SIZE ];
	float                lodRadius;
	vec3_t               lodOrigin;
	int                  numTriangles;
	static srfTriangle_t triangles[ SHADER_MAX_TRIANGLES ];

	oldheight = 0;
	width     = grid->width;
	height    = grid->height + 1;

	if ( height > MAX_GRID_SIZE )
	{
		return NULL;
	}

	for ( i = 0; i < height; i++ )
	{
		if ( i == row )
		{
			//insert new row
			for ( j = 0; j < grid->width; j++ )
			{
				LerpSurfaceVert( &grid->verts[ ( i - 1 ) * grid->width + j ], &grid->verts[ i * grid->width + j ], &ctrl[ i ][ j ] );

				if ( j == column )
				{
					VectorCopy( point, ctrl[ i ][ j ].xyz );
				}
			}

			errorTable[ 1 ][ i ] = loderror;
			continue;
		}

		errorTable[ 1 ][ i ] = grid->heightLodError[ oldheight ];

		for ( j = 0; j < grid->width; j++ )
		{
			ctrl[ i ][ j ] = grid->verts[ oldheight * grid->width + j ];
		}

		oldheight++;
	}

	for ( j = 0; j < grid->width; j++ )
	{
		errorTable[ 0 ][ j ] = grid->widthLodError[ j ];
	}

	// put all the aproximating points on the curve
	//PutPointsOnCurve( ctrl, width, height );

	// calculate triangles
	numTriangles = MakeMeshTriangles( width, height, ctrl, triangles );

	// calculate normals
	MakeMeshNormals( width, height, ctrl );
	MakeMeshTangentVectors( width, height, ctrl, numTriangles, triangles );

	// calculate tangent spaces
	//MakeTangentSpaces(width, height, ctrl, numTriangles, triangles);

	VectorCopy( grid->lodOrigin, lodOrigin );
	lodRadius = grid->lodRadius;
	// free the old grid
	R_FreeSurfaceGridMesh( grid );
	// create a new grid
	grid            = R_CreateSurfaceGridMesh( width, height, ctrl, errorTable, numTriangles, triangles );
	grid->lodRadius = lodRadius;
	VectorCopy( lodOrigin, grid->lodOrigin );
	return grid;
}
