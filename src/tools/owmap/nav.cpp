/*
===========================================================================
Copyright (C) 2012 Unvanquished Developers

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
// nav.cpp -- navigation mesh generator interface

extern "C" {
#include "q3map2.h"
#include "../common/surfaceflags.h"
}

#include <vector>
#include <queue>
#include "navgen.h"

Geometry geo;

float cellHeight = 2.0f;
float stepSize = STEPSIZE;
int   tileSize = 128;

struct Character
{
	char *name;   //appended to filename
	float radius; //radius of agents (BBox maxs[0] or BBox maxs[1])
	float height; //height of agents (BBox maxs[2] - BBox mins[2])
};

const Character characters[] = {
	{ "builder",     20, 40 },
	{ "builderupg",  20, 40 },
	{ "human_base",  15, 56 },
	{ "human_bsuit", 15, 76 },
	{ "level0",      15, 30 },
	{ "level1",      18, 36 },
	{ "level1upg",   21, 42 },
	{ "level2",      23, 36 },
	{ "level2upg",   25, 40 },
	{ "level3",      26, 55 },
	{ "level3upg",   26, 66 },
	{ "level4",      32, 92 }
};

//flag for excluding caulk surfaces
static qboolean excludeCaulk = qtrue;

//flag for adding new walkable spans so bots can walk over small gaps
static qboolean filterGaps = qtrue;

//copied from cm_patch.c
static const int MAX_GRID_SIZE = 129;
#define POINT_EPSILON 0.1
#define WRAP_POINT_EPSILON 0.1
static const int SUBDIVIDE_DISTANCE = 16;

typedef struct
{
	int      width;
	int      height;
	qboolean wrapWidth;
	qboolean wrapHeight;
	vec3_t   points[ MAX_GRID_SIZE ][ MAX_GRID_SIZE ]; // [width][height]
} cGrid_t;

static void WriteNavMeshFile( const char* agentname, const dtTileCache *tileCache, const dtNavMeshParams *params ) {
	int numTiles = 0;
	FILE *file = NULL;
	char filename[ 1024 ];
	NavMeshSetHeader header;
	qboolean swapEndian = qfalse;
	const int maxTiles = tileCache->getTileCount();

	for( int i = 0; i < maxTiles; i++ )
	{
		const dtCompressedTile *tile = tileCache->getTile( i );
		if ( !tile || !tile->header || !tile->dataSize ) 
		{
			continue;
		}
		numTiles++;
	}

	if ( NAVMESHSET_MAGIC != LittleLong( NAVMESHSET_MAGIC ) )
	{
		swapEndian = qtrue;
	}

	header.magic = NAVMESHSET_MAGIC;
	header.version = NAVMESHSET_VERSION;
	header.numTiles = numTiles;
	memcpy( &header.cacheParams, tileCache->getParams(), sizeof( header.cacheParams ) );
	memcpy( &header.params, params, sizeof( header.params ) );

	SwapBlock( ( int * ) &header, sizeof( header ) );

	strcpy( filename, source );
	StripExtension( filename );
	sprintf( filename, "%s-%s", filename, agentname );
	DefaultExtension( filename, ".navMesh" );
	file = fopen( filename, "wb" );

	if( !file )
	{
		Error( "Error opening %s: %s\n", filename, strerror( errno ) );
	}

	fwrite( &header, sizeof( header ), 1, file );

	for( int i = 0; i < maxTiles; i++ )
	{
		const dtCompressedTile *tile = tileCache->getTile( i );

		if ( !tile || !tile->header || !tile->dataSize )
		{
			continue;
		}

		NavMeshTileHeader tileHeader;
		tileHeader.tileRef = tileCache->getTileRef( tile );
		tileHeader.dataSize = tile->dataSize;

		SwapBlock( ( int * ) &tileHeader, sizeof( tileHeader ) );

		fwrite( &tileHeader, sizeof( tileHeader ), 1, file );

		unsigned char* data = ( unsigned char * ) malloc( tile->dataSize );

		memcpy( data, tile->data, tile->dataSize );
		if ( swapEndian )
		{
			dtTileCacheHeaderSwapEndian( data, tile->dataSize );
		}

		fwrite( data, tile->dataSize, 1, file );

		free( data );
	}
	fclose( file );
}

//need this to get the windings for brushes
extern "C" qboolean FixWinding( winding_t* w );

static void AddVert(std::vector<float> &verts, std::vector<int> &tris, vec3_t vert) {
	vec3_t recastVert;
	VectorCopy(vert, recastVert);
	quake2recast(recastVert);
	int index = 0;
	for(int i=0;i<3;i++) {
		verts.push_back(recastVert[i]);
	}
	index = (verts.size() - 3)/3;
	tris.push_back(index);
}

static void AddTri(std::vector<float> &verts, std::vector<int> &tris, vec3_t v1, vec3_t v2, vec3_t v3) {
	AddVert(verts, tris, v1);
	AddVert(verts, tris, v2);
	AddVert(verts, tris, v3);
}

static void LoadBrushTris(std::vector<float> &verts, std::vector<int> &tris) {
	int             j;
	/* get model, index 0 is worldspawn entity */
	bspModel_t *model = &bspModels[0];

	//go through the brushes
	for(int i=model->firstBSPBrush,m=0;m<model->numBSPBrushes;i++,m++) {
		int numSides = bspBrushes[i].numSides;
		int firstSide = bspBrushes[i].firstSide;
		bspShader_t *brushShader = &bspShaders[bspBrushes[i].shaderNum];

		if(!(brushShader->contentFlags & (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY | CONTENTS_BOTCLIP)))
				continue;
		/* walk the list of brush sides */
		for(int p = 0; p < numSides; p++)
		{
			/* get side and plane */
			bspBrushSide_t *side = &bspBrushSides[p+firstSide];
			bspPlane_t *plane = &bspPlanes[side->planeNum];
			bspShader_t *shader = &bspShaders[side->shaderNum];

			if(excludeCaulk && !Q_stricmp(shader->shader, "textures/common/caulk"))
				continue;

			/* make huge winding */
			winding_t *w = BaseWindingForPlane(plane->normal, plane->dist);

			/* walk the list of brush sides */
			for(j = 0; j < numSides && w != NULL; j++)
			{
				bspBrushSide_t *chopSide = &bspBrushSides[j+firstSide];
				if(chopSide == side)
					continue;
				if(chopSide->planeNum == (side->planeNum ^ 1))
					continue;		/* back side clipaway */

				bspPlane_t *chopPlane = &bspPlanes[chopSide->planeNum ^ 1];

				ChopWindingInPlace(&w, chopPlane->normal, chopPlane->dist, 0);

				/* ydnar: fix broken windings that would generate trifans */
				FixWinding(w);
			}

			if(w) {
				for(int j=2;j<w->numpoints;j++) {
					AddTri(verts, tris, w->p[0], w->p[j-1], w->p[j]);
				}
				FreeWinding(w);
			}
		}
	}
}

//We need these functions (taken straight from the engine's collision system)
//To generate accurate verts and tris for patch surfaces
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

	return (qboolean)(int)(dist >= SUBDIVIDE_DISTANCE);
}

static qboolean CM_ComparePoints( float *a, float *b )
{
	float d;

	d = a[ 0 ] - b[ 0 ];

	if ( d < -POINT_EPSILON || d > POINT_EPSILON )
	{
		return qfalse;
	}

	d = a[ 1 ] - b[ 1 ];

	if ( d < -POINT_EPSILON || d > POINT_EPSILON )
	{
		return qfalse;
	}

	d = a[ 2 ] - b[ 2 ];

	if ( d < -POINT_EPSILON || d > POINT_EPSILON )
	{
		return qfalse;
	}

	return qtrue;
}

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
	qboolean tempWrap;

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

If the left and right columns are exactly equal, set grid->wrapWidth qtrue
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

	for ( i = 0; i < grid->width - 2; )
	{
		// grid->points[i][x] is an interpolating control point
		// grid->points[i+1][x] is an aproximating control point
		// grid->points[i+2][x] is an interpolating control point

		//
		// first see if we can collapse the aproximating collumn away
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

		// the new aproximating point at i+1 may need to be removed
		// or subdivided farther, so don't advance i
	}
}
static void LoadPatchTris(std::vector<float> &verts, std::vector<int> &tris) {
	/* get model, index 0 is worldspawn entity */
	const bspModel_t *model = &bspModels[0];
	for ( int k = model->firstBSPSurface, n = 0; n < model->numBSPSurfaces; k++,n++)
	{
		const bspDrawSurface_t *surface = &bspDrawSurfaces[k];

		if ( !( bspShaders[surface->shaderNum].contentFlags & ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP ) ) ) {
			continue;
		}

		if ( surface->surfaceType != MST_PATCH)
		{
			continue;
		}

		if( !surface->patchWidth ) {
			continue;
		}

		cGrid_t grid;
		grid.width = surface->patchWidth;
		grid.height = surface->patchHeight;
		grid.wrapHeight = qfalse;
		grid.wrapWidth = qfalse;

		bspDrawVert_t *curveVerts = &bspDrawVerts[surface->firstVert];
		for (int x = 0; x < grid.width; x++ )
		{
			for ( int y=0;y < grid.height;y++)
			{
				VectorCopy( curveVerts[ y * grid.width + x ].xyz, grid.points[ x ][ y ] );
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

		for(int x = 0; x < (grid.width - 1); x++)
		{
			for(int y = 0; y < (grid.height- 1); y++)
			{
				/* set indexes */
				float *p1 = grid.points[ x ][ y ];
				float *p2 = grid.points[ x + 1 ][ y ];
				float *p3 = grid.points[ x + 1 ][ y + 1 ];
				AddTri(verts, tris, p1, p2, p3);

				p1 = grid.points[ x + 1 ][ y + 1 ];
				p2 = grid.points[ x ][ y + 1 ];
				p3 = grid.points[ x ][ y ];
				AddTri(verts, tris, p1, p2, p3);
			}
		}
	}
}
static void LoadGeometry()
{
	std::vector<float> verts;
	std::vector<int> tris;

	Sys_Printf("loading geometry...\n");
	int numVerts, numTris;

	//count surfaces
	LoadBrushTris( verts, tris );
	LoadPatchTris( verts, tris );

	numTris = tris.size() / 3;
	numVerts = verts.size() / 3;

	Sys_Printf("Using %d triangles\n", numTris);
	Sys_Printf("Using %d vertices\n", numVerts);

	geo.init( &verts[ 0 ], numVerts, &tris[ 0 ], numTris );

	const float *mins = geo.getMins();
	const float *maxs = geo.getMaxs();

	Sys_Printf("set recast world bounds to\n");
	Sys_Printf("min: %f %f %f\n", mins[0], mins[1], mins[2]);
	Sys_Printf("max: %f %f %f\n", maxs[0], maxs[1], maxs[2]);
}

// Modified version of Recast's rcErodeWalkableArea that uses an AABB instead of a cylindrical radius
static bool rcErodeWalkableAreaByBox(rcContext* ctx, int boxRadius, rcCompactHeightfield& chf)
{
	rcAssert(ctx);

	const int w = chf.width;
	const int h = chf.height;

	ctx->startTimer(RC_TIMER_ERODE_AREA);

	unsigned char* dist = (unsigned char*)rcAlloc(sizeof(unsigned char)*chf.spanCount, RC_ALLOC_TEMP);
	if (!dist)
	{
		ctx->log(RC_LOG_ERROR, "erodeWalkableArea: Out of memory 'dist' (%d).", chf.spanCount);
		return false;
	}

	// Init distance.
	memset(dist, 0xff, sizeof(unsigned char)*chf.spanCount);

	// Mark boundary cells.
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const rcCompactCell& c = chf.cells[x+y*w];
			for (int i = (int)c.index, ni = (int)(c.index+c.count); i < ni; ++i)
			{
				if (chf.areas[i] == RC_NULL_AREA)
				{
					dist[i] = 0;
				}
				else
				{
					const rcCompactSpan& s = chf.spans[i];
					int nc = 0;
					for (int dir = 0; dir < 4; ++dir)
					{
						if (rcGetCon(s, dir) != RC_NOT_CONNECTED)
						{
							const int nx = x + rcGetDirOffsetX(dir);
							const int ny = y + rcGetDirOffsetY(dir);
							const int nidx = (int)chf.cells[nx+ny*w].index + rcGetCon(s, dir);
							if (chf.areas[nidx] != RC_NULL_AREA)
							{
								nc++;
							}
						}
					}
					// At least one missing neighbour.
					if (nc != 4)
						dist[i] = 0;
				}
			}
		}
	}

	unsigned char nd;

	// Pass 1
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const rcCompactCell& c = chf.cells[x+y*w];
			for (int i = (int)c.index, ni = (int)(c.index+c.count); i < ni; ++i)
			{
				const rcCompactSpan& s = chf.spans[i];

				if (rcGetCon(s, 0) != RC_NOT_CONNECTED)
				{
					// (-1,0)
					const int ax = x + rcGetDirOffsetX(0);
					const int ay = y + rcGetDirOffsetY(0);
					const int ai = (int)chf.cells[ax+ay*w].index + rcGetCon(s, 0);
					const rcCompactSpan& as = chf.spans[ai];
					nd = (unsigned char)rcMin((int)dist[ai]+2, 255);
					if (nd < dist[i])
						dist[i] = nd;

					// (-1,-1)
					if (rcGetCon(as, 3) != RC_NOT_CONNECTED)
					{
						const int aax = ax + rcGetDirOffsetX(3);
						const int aay = ay + rcGetDirOffsetY(3);
						const int aai = (int)chf.cells[aax+aay*w].index + rcGetCon(as, 3);
						nd = (unsigned char)rcMin((int)dist[aai]+2, 255);
						if (nd < dist[i])
							dist[i] = nd;
					}
				}
				if (rcGetCon(s, 3) != RC_NOT_CONNECTED)
				{
					// (0,-1)
					const int ax = x + rcGetDirOffsetX(3);
					const int ay = y + rcGetDirOffsetY(3);
					const int ai = (int)chf.cells[ax+ay*w].index + rcGetCon(s, 3);
					const rcCompactSpan& as = chf.spans[ai];
					nd = (unsigned char)rcMin((int)dist[ai]+2, 255);
					if (nd < dist[i])
						dist[i] = nd;

					// (1,-1)
					if (rcGetCon(as, 2) != RC_NOT_CONNECTED)
					{
						const int aax = ax + rcGetDirOffsetX(2);
						const int aay = ay + rcGetDirOffsetY(2);
						const int aai = (int)chf.cells[aax+aay*w].index + rcGetCon(as, 2);
						nd = (unsigned char)rcMin((int)dist[aai]+2, 255);
						if (nd < dist[i])
							dist[i] = nd;
					}
				}
			}
		}
	}

	// Pass 2
	for (int y = h-1; y >= 0; --y)
	{
		for (int x = w-1; x >= 0; --x)
		{
			const rcCompactCell& c = chf.cells[x+y*w];
			for (int i = (int)c.index, ni = (int)(c.index+c.count); i < ni; ++i)
			{
				const rcCompactSpan& s = chf.spans[i];

				if (rcGetCon(s, 2) != RC_NOT_CONNECTED)
				{
					// (1,0)
					const int ax = x + rcGetDirOffsetX(2);
					const int ay = y + rcGetDirOffsetY(2);
					const int ai = (int)chf.cells[ax+ay*w].index + rcGetCon(s, 2);
					const rcCompactSpan& as = chf.spans[ai];
					nd = (unsigned char)rcMin((int)dist[ai]+2, 255);
					if (nd < dist[i])
						dist[i] = nd;

					// (1,1)
					if (rcGetCon(as, 1) != RC_NOT_CONNECTED)
					{
						const int aax = ax + rcGetDirOffsetX(1);
						const int aay = ay + rcGetDirOffsetY(1);
						const int aai = (int)chf.cells[aax+aay*w].index + rcGetCon(as, 1);
						nd = (unsigned char)rcMin((int)dist[aai]+2, 255);
						if (nd < dist[i])
							dist[i] = nd;
					}
				}
				if (rcGetCon(s, 1) != RC_NOT_CONNECTED)
				{
					// (0,1)
					const int ax = x + rcGetDirOffsetX(1);
					const int ay = y + rcGetDirOffsetY(1);
					const int ai = (int)chf.cells[ax+ay*w].index + rcGetCon(s, 1);
					const rcCompactSpan& as = chf.spans[ai];
					nd = (unsigned char)rcMin((int)dist[ai]+2, 255);
					if (nd < dist[i])
						dist[i] = nd;

					// (-1,1)
					if (rcGetCon(as, 0) != RC_NOT_CONNECTED)
					{
						const int aax = ax + rcGetDirOffsetX(0);
						const int aay = ay + rcGetDirOffsetY(0);
						const int aai = (int)chf.cells[aax+aay*w].index + rcGetCon(as, 0);
						nd = (unsigned char)rcMin((int)dist[aai]+2, 255);
						if (nd < dist[i])
							dist[i] = nd;
					}
				}
			}
		}
	}

	const unsigned char thr = (unsigned char)(boxRadius*2);
	for (int i = 0; i < chf.spanCount; ++i) {
		if (dist[i] < thr)
			chf.areas[i] = RC_NULL_AREA;
	}

	rcFree(dist);

	ctx->stopTimer(RC_TIMER_ERODE_AREA);

	return true;
}

/*
rcFilterGaps

Does a super simple sampling of ledges to detect and fix
any "gaps" in the heightfield that are narrow enough for us to walk over
because of our AABB based collision system
*/
static void rcFilterGaps(rcContext *ctx, int walkableRadius, int walkableClimb, int walkableHeight, rcHeightfield &solid) {
	const int h = solid.height;
	const int w = solid.width;
	const int MAX_HEIGHT = 0xffff;
	std::vector<int> spanData;
	spanData.reserve(500*3);
	std::vector<int> data;
	data.reserve((walkableRadius*2-1)*3);

	//for every span in the heightfield
	for(int y=0;y<h;++y) {
		for(int x=0;x<w;++x) {
			//check each span in the column
			for(rcSpan *s = solid.spans[x + y*w]; s;s = s->next) {
				//bottom and top of the "base" span
				const int sbot = s->smax;
				const int stop = (s->next) ? (s->next->smin) : MAX_HEIGHT;

				//if the span is walkable
				if(s->area != RC_NULL_AREA) {
					//check all neighbor connections
					for(int dir=0;dir<4;dir++) {
						const int dirx = rcGetDirOffsetX(dir);
						const int diry = rcGetDirOffsetY(dir);
						int dx = x;
						int dy = y;
						bool freeSpace = false;
						bool stop = false;

						if ( dx < 0 || dy < 0 || dx >= w || dy >= h )
						{
							continue;
						}

						//keep going the direction for walkableRadius * 2 - 1 spans
						//because we can walk as long as at least part of our bbox is on a solid surface
						for(int i=1;i<walkableRadius*2;i++) {
							dx = dx + dirx;
							dy = dy + diry;
							if(dx < 0 || dy < 0 || dx >= w || dy >= h)
							{
								i--;
								freeSpace = false;
								stop = false;
								break;
							}

							//tells if there is space here for us to stand
							freeSpace = false;

							//go through the column
							for(rcSpan *ns = solid.spans[dx + dy*w];ns;ns = ns->next) {
								int nsbot = ns->smax;
								int nstop = (ns->next) ? (ns->next->smin) : MAX_HEIGHT;

								//if there is a span within walkableClimb of the base span, we have reached the end of the gap (if any)
								if(rcAbs(sbot - nsbot) <= walkableClimb && ns->area != RC_NULL_AREA) {
									//set flag telling us to stop
									stop = true;

									//only add spans if we have gone for more than 1 iteration
									//if we stop at the first iteration, it means there was no gap to begin with
									if(i>1)
										freeSpace = true;
									break;
								}

								if(nsbot < sbot && nstop >= sbot + walkableHeight) {
									//tell that we found walkable space within reach of the previous span
									freeSpace = true;
									//add this span to a temporary storage location
									data.push_back(dx);
									data.push_back(dy);
									data.push_back(sbot);
									break;
								}
							}

							//stop if there is no more freespace, or we have reached end of gap
							if(stop || !freeSpace) {
								break;
							}
						}
						//move the spans from the temp location to the storage
						//we check freeSpace to make sure we don't add a
						//span when we stop at the first iteration (because there is no gap)
						//checking stop tells us if there was a span to complete the "bridge"
						if(freeSpace && stop) {
							const int N = data.size();
							for(int i=0;i<N;i++)
								spanData.push_back(data[i]);
						}
						data.clear();
					}
				}
			}
		}
	}

	//add the new spans
	//we cant do this in the loop, because the loop would then iterate over those added spans
	for(int i=0;i<spanData.size();i+=3) {
		rcAddSpan(ctx, solid, spanData[i], spanData[i+1], spanData[i+2]-1, spanData[i+2], RC_WALKABLE_AREA, walkableClimb);
	}
}

static int rasterizeTileLayers( rcContext &context, int tx, int ty, const rcConfig &mcfg, TileCacheData *data, int maxLayers )
{
	rcConfig cfg;

	FastLZCompressor comp;
	RasterizationContext rc;

	const float tcs = mcfg.tileSize * mcfg.cs;

	memcpy( &cfg, &mcfg, sizeof( cfg ) );

	// find tile bounds
	cfg.bmin[ 0 ] = mcfg.bmin[ 0 ] + tx * tcs;
	cfg.bmin[ 1 ] = mcfg.bmin[ 1 ];
	cfg.bmin[ 2 ] = mcfg.bmin[ 2 ] + ty * tcs;

	cfg.bmax[ 0 ] = mcfg.bmin[ 0 ] + ( tx + 1 ) * tcs;
	cfg.bmax[ 1 ] = mcfg.bmax[ 1 ];
	cfg.bmax[ 2 ] = mcfg.bmin[ 2 ] + ( ty + 1 ) * tcs;

	// expand bounds by border size
	cfg.bmin[ 0 ] -= cfg.borderSize * cfg.cs;
	cfg.bmin[ 2 ] -= cfg.borderSize * cfg.cs;

	cfg.bmax[ 0 ] += cfg.borderSize * cfg.cs;
	cfg.bmax[ 2 ] += cfg.borderSize * cfg.cs;

	rc.solid = rcAllocHeightfield();

	if ( !rcCreateHeightfield( &context, *rc.solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch ) )
	{
		Error ("Failed to create heightfield for navigation mesh.\n");
	}

	const float *verts = geo.getVerts();
	const int nverts = geo.getNumVerts();
	const rcChunkyTriMesh *chunkyMesh = geo.getChunkyMesh();

	rc.triareas = new unsigned char[ chunkyMesh->maxTrisPerChunk ];
	if ( !rc.triareas )
	{
		Error( "Out of memory rc.triareas\n" );
	}

	float tbmin[ 2 ], tbmax[ 2 ];

	tbmin[ 0 ] = cfg.bmin[ 0 ];
	tbmin[ 1 ] = cfg.bmin[ 2 ];
	tbmax[ 0 ] = cfg.bmax[ 0 ];
	tbmax[ 1 ] = cfg.bmax[ 2 ];

	int *cid = new int[ chunkyMesh->nnodes ];

	const int ncid = rcGetChunksOverlappingRect( chunkyMesh, tbmin, tbmax, cid, chunkyMesh->nnodes );
	if ( !ncid )
	{
		delete[] cid;
		return 0;
	}

	for ( int i = 0; i < ncid; i++ )
	{
		const rcChunkyTriMeshNode &node = chunkyMesh->nodes[ cid[ i ] ];
		const int *tris = &chunkyMesh->tris[ node.i * 3 ];
		const int ntris = node.n;

		memset( rc.triareas, 0, ntris * sizeof( unsigned char ) );

		rcMarkWalkableTriangles( &context, cfg.walkableSlopeAngle, verts, nverts, tris, ntris, rc.triareas );
		rcRasterizeTriangles( &context, verts, nverts, tris, rc.triareas, ntris, *rc.solid, cfg.walkableClimb );
	}

	delete[] cid;

	rcFilterLowHangingWalkableObstacles( &context, cfg.walkableClimb, *rc.solid );

	//dont filter ledge spans since characters CAN walk on ledges due to using a bbox for movement collision
	//rcFilterLedgeSpans (&context, cfg.walkableHeight, cfg.walkableClimb, *rc.solid);

	rcFilterWalkableLowHeightSpans( &context, cfg.walkableHeight, *rc.solid );
	
	if(filterGaps)
	{
		rcFilterGaps( &context, cfg.walkableRadius, cfg.walkableClimb, cfg.walkableHeight, *rc.solid );
	}

	rc.chf = rcAllocCompactHeightfield();

	if ( !rcBuildCompactHeightfield( &context, cfg.walkableHeight, cfg.walkableClimb, *rc.solid, *rc.chf ) )
	{
		Error ("Failed to create compact heightfield for navigation mesh.\n");
	}

	if ( !rcErodeWalkableAreaByBox(&context, cfg.walkableRadius, *rc.chf) )
	{
		Error ("Unable to erode walkable surfaces.\n");
	}

	rc.lset = rcAllocHeightfieldLayerSet();

	if ( !rc.lset )
	{
		Error( "Out of memory heightfield layer set\n" );
	}

	if ( !rcBuildHeightfieldLayers( &context, *rc.chf, cfg.borderSize, cfg.walkableHeight, *rc.lset ) )
	{
		Error( "Could not build heightfield layers\n" );
	}

	rc.ntiles = 0;

	for ( int i = 0; i < rcMin( rc.lset->nlayers, MAX_LAYERS ); i++ )
	{
		TileCacheData *tile = &rc.tiles[ rc.ntiles++ ];
		const rcHeightfieldLayer *layer = &rc.lset->layers[ i ];

		dtTileCacheLayerHeader header;
		header.magic = DT_TILECACHE_MAGIC;
		header.version = DT_TILECACHE_VERSION;

		header.tx = tx;
		header.ty = ty;
		header.tlayer = i;
		dtVcopy( header.bmin, layer->bmin );
		dtVcopy( header.bmax, layer->bmax );

		header.width = ( unsigned char ) layer->width;
		header.height = ( unsigned char ) layer->height;
		header.minx = ( unsigned char ) layer->minx;
		header.maxx = ( unsigned char ) layer->maxx;
		header.miny = ( unsigned char ) layer->miny;
		header.maxy = ( unsigned char ) layer->maxy;
		header.hmin = ( unsigned short ) layer->hmin;
		header.hmax = ( unsigned short ) layer->hmax;

		dtStatus status = dtBuildTileCacheLayer( &comp, &header, layer->heights, layer->areas, layer->cons, &tile->data, &tile->dataSize );

		if ( dtStatusFailed( status ) )
		{
			return 0;
		}
	}

	// transfer tile data over to caller
	int n = 0;
	for ( int i = 0; i < rcMin( rc.ntiles, maxLayers ); i++ )
	{
		data[ n++ ] = rc.tiles[ i ];
		rc.tiles[ i ].data = 0;
		rc.tiles[ i ].dataSize = 0;
	}

	return n;
}

static void BuildNavMesh( int characterNum )
{
	const Character &agent = characters[ characterNum ];

	dtTileCache *tileCache;
	const float *bmin = geo.getMins();
	const float *bmax = geo.getMaxs();
	int gw = 0, gh = 0;
	const float cellSize = agent.radius / 4.0f;

	rcCalcGridSize( bmin, bmax, cellSize, &gw, &gh );

	const int ts = tileSize;
	const int tw = ( gw + ts - 1 ) / ts;
	const int th = ( gh + ts - 1 ) / ts;

	rcConfig cfg;
	memset( &cfg, 0, sizeof ( cfg ) );

	cfg.cs = cellSize;
	cfg.ch = cellHeight;
	cfg.walkableSlopeAngle = RAD2DEG( acosf( MIN_WALK_NORMAL ) );
	cfg.walkableHeight = ( int ) ceilf( agent.height / cfg.ch );
	cfg.walkableClimb = ( int ) floorf( stepSize / cfg.ch );
	cfg.walkableRadius = ( int ) ceilf( agent.radius / cfg.cs );
	cfg.maxEdgeLen = 0;
	cfg.maxSimplificationError = 1.3f;
	cfg.minRegionArea = rcSqr( 25 );
	cfg.mergeRegionArea = rcSqr( 50 );
	cfg.maxVertsPerPoly = 6;
	cfg.tileSize = ts;
	cfg.borderSize = cfg.walkableRadius + 3;
	cfg.width = cfg.tileSize + cfg.borderSize * 2;
	cfg.height = cfg.tileSize + cfg.borderSize * 2;
	cfg.detailSampleDist = cfg.cs * 6.0f;
	cfg.detailSampleMaxError = cfg.ch * 1.0f;

	rcVcopy( cfg.bmin, bmin );
	rcVcopy( cfg.bmax, bmax );

	dtTileCacheParams tcparams;
	memset( &tcparams, 0, sizeof( tcparams ) );
	rcVcopy( tcparams.orig, bmin );
	tcparams.cs = cellSize;
	tcparams.ch = cellHeight;
	tcparams.width = ts;
	tcparams.height = ts;
	tcparams.walkableHeight = agent.height;
	tcparams.walkableRadius = agent.radius;
	tcparams.walkableClimb = stepSize;
	tcparams.maxSimplificationError = 1.3;
	tcparams.maxTiles = tw * th * EXPECTED_LAYERS_PER_TILE;
	tcparams.maxObstacles = 256;
	
	tileCache = dtAllocTileCache();

	if ( !tileCache )
	{
		Error( "Could not allocate tile cache\n" );
	}

	LinearAllocator alloc( 32000 );
	FastLZCompressor comp;
	MeshProcess proc;

	dtStatus status = tileCache->init( &tcparams, &alloc, &comp, &proc );

	if ( dtStatusFailed( status ) )
	{
		if ( dtStatusDetail( status, DT_INVALID_PARAM ) )
		{
			Error( "Could not init tile cache: Invalid parameter\n" );
		}
		else
		{
			Error( "Could not init tile cache\n" );
		}
	}

	rcContext context( false );

	for ( int y = 0; y < th; y++ )
	{
		for ( int x = 0; x < tw; x++ )
		{
			TileCacheData tiles[ MAX_LAYERS ];
			memset( tiles, 0, sizeof( tiles ) );

			int ntiles = rasterizeTileLayers( context, x, y, cfg, tiles, MAX_LAYERS );
			
			for ( int i = 0; i < ntiles; i++ )
			{
				TileCacheData *tile = &tiles[ i ];
				status = tileCache->addTile( tile->data, tile->dataSize, DT_COMPRESSEDTILE_FREE_DATA, 0 );
				if ( dtStatusFailed( status ) )
				{
					dtFree( tile->data );
					tile->data = 0;
					continue;
				}
			}
		}
	}

	// there are 22 bits to store a tile and its polys
	int tileBits = rcMin( ( int ) dtIlog2( dtNextPow2( tcparams.maxTiles ) ), 14 );
	int polyBits = 22 - tileBits;

	dtNavMeshParams params;
	dtVcopy( params.orig, tcparams.orig );
	params.tileHeight = ts * cfg.cs;
	params.tileWidth = ts * cfg.cs;
	params.maxTiles = 1 << tileBits;
	params.maxPolys = 1 << polyBits;

	WriteNavMeshFile( agent.name, tileCache, &params );
	dtFreeTileCache( tileCache );
}

/*
===========
NavMain
===========
*/
extern "C" int NavMain(int argc, char **argv)
{
	float temp;
	int i;

	/* note it */
	Sys_Printf("--- Nav ---\n");

	if(argc < 2)
	{
		Sys_Printf("Usage: owmap -nav [-cellheight f] [-stepsize f] [-includecaulk] [-nogapfilter] <mapname>\n");
		return 0;
	}

	/* process arguments */
	for(i = 1; i < (argc - 1); i++)
	{
		if(!Q_stricmp(argv[i],"-cellheight")) {
			i++;
			if(i<(argc - 1)) {
				temp = atof(argv[i]);
				if(temp > 0) {
					cellHeight = temp;
				}
			}
		} else if (!Q_stricmp(argv[i], "-stepsize")) {
			i++;
			if(i<(argc - 1)) {
				temp = atof(argv[i]);
				if(temp > 0) {
					stepSize = temp;
				}
			}
		} else if(!Q_stricmp(argv[i], "-includecaulk")) {
			excludeCaulk = qfalse;
		} else if(!Q_stricmp(argv[i], "-nogapfilter")) {
			filterGaps = qfalse;
		} else {
			Sys_Printf("WARNING: Unknown option \"%s\"\n", argv[i]);
		}
	}

	/* load the bsp */
	sprintf(source, "%s%s", inbase, ExpandArg(argv[i]));
	StripExtension(source);
	strcat(source, ".bsp");
	//LoadShaderInfo();

	Sys_Printf("Loading %s\n", source);

	LoadBSPFile(source);

	ParseEntities();

	LoadGeometry();

	float height = rcAbs(geo.getMaxs()[1]) + rcAbs(geo.getMins()[1]);
	if(height / cellHeight > RC_SPAN_MAX_HEIGHT) {
		Sys_Printf("WARNING: Map geometry is too tall for specified cell height. Increasing cell height to compensate. This may cause a less accurate navmesh.\n");
		float prevCellHeight = cellHeight;
		float minCellHeight = height / RC_SPAN_MAX_HEIGHT;

		int divisor = ( int ) stepSize;

		while ( divisor && cellHeight < minCellHeight )
		{
			cellHeight = stepSize / divisor;
			divisor--;
		}

		if ( !divisor )
		{
			Error( "ERROR: Map is too tall to generate a navigation mesh\n" );
		}

		Sys_Printf("Previous cellheight: %f\n", prevCellHeight);
		Sys_Printf("New cellheight: %f\n", cellHeight);
	}

	RunThreadsOnIndividual( sizeof( characters ) / sizeof( characters[ 0 ] ), qtrue, BuildNavMesh );

	return 0;
}
