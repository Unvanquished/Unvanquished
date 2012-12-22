/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
Copyright (C) 2009 Peter McNeill <n27@bigpond.net.au>

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

#include "../recast/Recast.h"
#include "../recast/RecastAlloc.h"
#include "../recast/RecastAssert.h"
#include <vector>
#include <queue>
#include "../../engine/botlib/nav.h"

vec3_t mapmins;
vec3_t mapmaxs;

// Load triangles
std::vector<int> tris;
int numtris = 0;

// Load vertices
std::vector<float> verts;
int numverts = 0;

float cellSize = 6;
float cellHeight = 0.5;
float stepSize = STEPSIZE;

typedef struct {
	char* name;   //appended to filename
	short radius; //radius of agents (BBox maxs[0] or BBox maxs[1])
	short height; //height of agents (BBox maxs[2] - BBox mins[2])
} tremClass_t;

const tremClass_t tremClasses[] = {
	{
		"builder",
		20,
		40,
	},
	{
		"builderupg",
		20,
		40,
	},
	{
		"human_base",
		15,
		56,
	},
	{
		"human_bsuit",
		15,
		76,
	},
	{
		"level0",
		15,
		30,
	},
	{
		"level1",
		18,
		36
	},
	{
		"level1upg",
		21,
		42
	},
	{
		"level2",
		23,
		36
	},
	{
		"level2upg",
		25,
		40
	},
	{
		"level3",
		26,
		55
	},
	{
		"level3upg",
		26,
		66
	},
	{
		"level4",
		32,
		92
	}
};


//flag for optional median filter of walkable surfaces
static qboolean median = qfalse;

//flag for optimistic walkableclimb projection
static qboolean optimistic = qfalse;

//flag for excluding caulk surfaces
static qboolean excludeCaulk = qtrue;

//flag for adding new walkable spans so bots can walk over small gaps
static qboolean filterGaps;

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

struct spanStruct {
	int idx;
	int x;
	int y;
};

static void quake2recast(vec3_t vec) {
	vec_t temp = vec[1];
	vec[0] = -vec[0];
	vec[1] = vec[2];
	vec[2] = -temp;
}

static void WriteRecastData (const char* agentname, const rcPolyMesh *polyMesh, const rcPolyMeshDetail *detailedPolyMesh, const rcConfig *cfg )
{
	FILE *file;
	NavMeshHeader_t navHeader;
	char filename[1024];
	StripExtension(source);
	strcpy(filename,source);
	sprintf(filename,"%s-%s",filename,agentname);
	DefaultExtension(filename, ".navMesh");

	file = fopen(filename, "w");
	if(!file) {
		Error("Error opening %s: %s", filename, strerror(errno));
	}
	memset(&navHeader,0, sizeof(NavMeshHeader_t));

	//print header info
	navHeader.version = 1;

	navHeader.numVerts = polyMesh->nverts;

	navHeader.numPolys = polyMesh->npolys;

	navHeader.numVertsPerPoly = polyMesh->nvp;

	VectorCopy (polyMesh->bmin, navHeader.mins);

	VectorCopy (polyMesh->bmax, navHeader.maxs);

	navHeader.dNumMeshes = detailedPolyMesh->nmeshes;

	navHeader.dNumVerts = detailedPolyMesh->nverts;

	navHeader.dNumTris = detailedPolyMesh->ntris;

	navHeader.cellSize = cfg->cs;

	navHeader.cellHeight = cfg->ch;

	//write header
	fprintf(file, "%d ", navHeader.version);
	fprintf(file, "%d %d %d ", navHeader.numVerts,navHeader.numPolys,navHeader.numVertsPerPoly);
	fprintf(file, "%f %f %f ", navHeader.mins[0],navHeader.mins[1],navHeader.mins[2]);
	fprintf(file, "%f %f %f ", navHeader.maxs[0], navHeader.maxs[1], navHeader.maxs[2]);
	fprintf(file, "%d %d %d ",navHeader.dNumMeshes, navHeader.dNumVerts, navHeader.dNumTris);
	fprintf(file, "%f %f\n", navHeader.cellSize, navHeader.cellHeight);


	//write verts
	for ( int i = 0, j = 0; i < polyMesh->nverts; i++, j += 3 )
	{
		fprintf(file, "%d %d %d\n", polyMesh->verts[j], polyMesh->verts[j+1], polyMesh->verts[j+2]);
	}

	//write polys
	for ( int i = 0, j = 0; i < polyMesh->npolys; i++, j += polyMesh->nvp * 2 )
	{
		fprintf(file, "%d %d %d %d %d %d %d %d %d %d %d %d\n", polyMesh->polys[j], polyMesh->polys[j+1], polyMesh->polys[j+2],
			polyMesh->polys[j+3], polyMesh->polys[j+4], polyMesh->polys[j+5],
			polyMesh->polys[j+6], polyMesh->polys[j+7], polyMesh->polys[j+8],
			polyMesh->polys[j+9], polyMesh->polys[j+10], polyMesh->polys[j+11]);
	}

	//write areas
	for( int i=0;i<polyMesh->npolys;i++) {
		fprintf(file, "%d ", polyMesh->areas[i]);
	}
	fprintf(file, "\n");

	//write flags
	for ( int i = 0; i < polyMesh->npolys; i++ )
	{
		fprintf(file, "%d ", polyMesh->flags[i]);
	}
	fprintf(file, "\n");

	//write detail meshes
	for ( int i = 0, j = 0; i < detailedPolyMesh->nmeshes; i++, j += 4 )
	{
		fprintf(file, "%d %d %d %d\n", detailedPolyMesh->meshes[j], detailedPolyMesh->meshes[j+1],
			detailedPolyMesh->meshes[j+2], detailedPolyMesh->meshes[j+3]);
	}

	//write detail verts
	for ( int i = 0, j = 0; i < detailedPolyMesh->nverts; i++, j += 3 )
	{
		fprintf(file, "%d %d %d\n", (int)detailedPolyMesh->verts[j], (int)detailedPolyMesh->verts[j+1],
			(int)detailedPolyMesh->verts[j+2]);
	}

	//write detail tris
	for ( int i = 0, j = 0; i < detailedPolyMesh->ntris; i++, j += 4 )
	{
		fprintf(file, "%d %d %d %d\n", detailedPolyMesh->tris[j], detailedPolyMesh->tris[j+1],
			detailedPolyMesh->tris[j+2], detailedPolyMesh->tris[j+3]);
	}

	fclose(file);
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
	Sys_Printf("loading geometry...\n");

	//count surfaces
	LoadBrushTris( verts, tris );
	LoadPatchTris( verts, tris );

	numtris = tris.size() / 3;
	numverts = verts.size() / 3;
	Sys_Printf("Using %d triangles\n", numtris);
	Sys_Printf("Using %d vertices\n", numverts);

	// find bounds
	ClearBounds( mapmins, mapmaxs );
	for(int i=0;i<numverts;i++) {
		vec3_t vert;
		VectorSet(vert,verts[i*3],verts[i*3+1],verts[i*3+2]);
		AddPointToBounds(vert, mapmins, mapmaxs);
	}

	Sys_Printf("set recast world bounds to\n");
	Sys_Printf("min: %f %f %f\n", mapmins[0], mapmins[1], mapmins[2]);
	Sys_Printf("max: %f %f %f\n", mapmaxs[0], mapmaxs[1], mapmaxs[2]);
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
FloodSpans

Does a flood fill through spans that are open to one another
*/
static void FloodSpans(int cx, int cy, int spanIdx, int radius, unsigned char area, rcCompactHeightfield &chf) {
	const int w =chf.width;
	const int h = chf.height;
	const int mins[2] = {rcMax(cx - radius,0), rcMax(cy - radius,0)};
	const int maxs[2] = {rcMin(cx + radius,w-1), rcMin(cy + radius,h-1)};

	std::queue<spanStruct> queue;
	spanStruct span;
	span.idx = spanIdx;
	span.x = cx;
	span.y = cy;
	queue.push(span);

	std::vector<int> closed;
	closed.reserve(radius*radius);
	chf.areas[spanIdx] = UCHAR_MAX;

	while(!queue.empty()) {
		const spanStruct span = queue.front();
		queue.pop();
		const rcCompactSpan &s = chf.spans[span.idx];

		for(int dir=0;dir<4;dir++) {
			spanStruct ns;
			ns.x = span.x + rcGetDirOffsetX(dir);
			ns.y = span.y + rcGetDirOffsetY(dir);

			//skip span if past bounds
			if(ns.x < mins[0] || ns.y < mins[1])
				continue;
			if(ns.x > maxs[0] || ns.y > maxs[1])
				continue;

			rcCompactCell &c = chf.cells[ns.x + ns.y*w];
			for(int i=(int)c.index,ni = (int)(c.index+c.count);i<ni;i++) {
				rcCompactSpan &nns = chf.spans[i];

				//skip span if it is not within current span's z bounds
				//if(nns.y < s.y && (nns.y+nns.h) > (s.y + s.h))
					//continue;
				//if(nns.y > s.y && nns.y > (s.y + s.h))
				if(nns.y > (s.y + s.h))
					continue;
				if((nns.y + nns.h) < s.y)
					continue;
				ns.idx = i;
				if(chf.areas[ns.idx] != UCHAR_MAX) {
					chf.areas[ns.idx] = UCHAR_MAX;
					queue.push(ns);
				}
			}
		}
		closed.push_back(span.idx);
	}
	for(int i=0;i<closed.size();i++) {
		chf.areas[closed[i]] = area;
	}
}

char floodEntities[][17] = {"team_human_spawn", "team_alien_spawn"};

static bool inEntityList(entity_t *e) {
	for(int i=0;i<sizeof(floodEntities)/sizeof(floodEntities[0]);i++) {
		if(!Q_stricmp(ValueForKey(e,"classname"), floodEntities[i]))
			return true;
	}
	return false;
}

static void RemoveUnreachableSpans(rcCompactHeightfield &chf) {
	int radius = rcMax(chf.width, chf.height);

	//copy over current span areas
	unsigned char *prevAreas = new unsigned char[chf.spanCount];
	memcpy(prevAreas, chf.areas, chf.spanCount);

	//go over the entities and flood fill from them in the compact heightfield
	for(int i=1;i<numEntities;i++) {
		entity_t *e = &entities[i];
		if(!inEntityList(e))
			continue;
		vec3_t origin;
		GetVectorForKey(e, "origin", origin);
		quake2recast(origin);

		//find voxel cordinate for the origin
		int x = (origin[0] - chf.bmin[0]) / chf.cs;
		int y = (origin[1] - chf.bmin[1]) / chf.ch;
		int z = (origin[2] - chf.bmin[2]) / chf.cs;

		//now find a walkableSpan closest to the origin
		rcCompactCell &c = chf.cells[x+z*chf.width];
		int bestDist = INT_MAX;
		int spanIdx = -1;
		for(int i = (int)c.index, ni = (int)(c.index + c.count); i < ni; i++) {
			if(chf.areas[i] == RC_NULL_AREA)
				continue;
			rcCompactSpan &s = chf.spans[i];
			if(rcAbs(s.y - y) < bestDist) {
				spanIdx = i;
				bestDist = rcAbs(s.y - y);
			}
		}

		//flood fill spans that are open to one another
		if(spanIdx > -1)
			FloodSpans(x, z, spanIdx, radius, UCHAR_MAX, chf);
	}

	//now we go through the areas and only use the ones that we flooded to
	for(int i=0;i<chf.spanCount;i++) {
		if(chf.areas[i] == UCHAR_MAX)
			chf.areas[i] = prevAreas[i];
		else
			chf.areas[i] = RC_NULL_AREA;
	}
	delete[] prevAreas;
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

						//keep going the direction for walkableRadius * 2 - 1 spans
						//because we can walk as long as at least part of our bbox is on a solid surface
						for(int i=1;i<walkableRadius*2;i++) {
							dx = dx + dirx;
							dy = dy + diry;
							if(dx < 0 || dy < 0 || dx >= w || dy >= h) {
								break;
							}

							//tells if there is space here for us to stand
							freeSpace = false;

							//go through the column
							for(rcSpan *ns = solid.spans[dx + dy*w];ns;ns = ns->next) {
								int nsbot = ns->smax;
								int nstop = (ns->next) ? (ns->next->smin) : MAX_HEIGHT;

								//if there is a span within walkableClimb of the base span, we have reached the end of the gap (if any)
								if(rcAbs(sbot - nsbot) <= walkableClimb) {
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

static void buildPolyMesh( int characterNum, vec3_t mapmins, vec3_t mapmaxs, rcPolyMesh *&polyMesh, rcPolyMeshDetail *&detailedPolyMesh, rcConfig &cfg )
{
	float agentHeight = tremClasses[ characterNum ].height;
	float agentRadius = tremClasses[ characterNum ].radius;
	rcHeightfield *heightField;
	rcCompactHeightfield *compHeightField;
	rcContourSet *contours;

	memset (&cfg, 0, sizeof (cfg));
	VectorCopy (mapmaxs, cfg.bmax);
	VectorCopy (mapmins, cfg.bmin);

	cfg.cs = cellSize;
	cfg.ch = cellHeight;
	cfg.walkableSlopeAngle = 46; //max slope is 45, but recast checks for < 45 so we need 46
	cfg.maxEdgeLen = 64;
	cfg.maxSimplificationError = 1;
	cfg.maxVertsPerPoly = 6;
	cfg.detailSampleDist = cfg.cs * 6.0f;
	cfg.detailSampleMaxError = cfg.ch * 1.0f;
	cfg.minRegionArea = rcSqr(8);
	cfg.mergeRegionArea = rcSqr(30);
	cfg.walkableHeight = (int) ceilf(agentHeight / cfg.ch);
	cfg.walkableClimb = (int) floorf(stepSize/cfg.ch);
	cfg.walkableRadius = (int) ceilf(agentRadius / cfg.cs);

	rcCalcGridSize (cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

	class rcContext context(false);

	heightField = rcAllocHeightfield();

	if ( !rcCreateHeightfield (&context, *heightField, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch) )
	{
		Error ("Failed to create heightfield for navigation mesh.\n");
	}

	unsigned char *triareas = new unsigned char[numtris];
	memset (triareas, 0, numtris);

	rcMarkWalkableTriangles (&context, cfg.walkableSlopeAngle, &verts[ 0 ], numverts, &tris[ 0 ], numtris, triareas);
	rcRasterizeTriangles (&context, &verts[ 0 ], triareas, numtris, *heightField, cfg.walkableClimb);
	delete[] triareas;
	triareas = NULL;

	rcFilterLowHangingWalkableObstacles (&context, cfg.walkableClimb, *heightField);

	//dont filter ledge spans since characters CAN walk on ledges due to using a bbox for movement collision
	//rcFilterLedgeSpans (&context, cfg.walkableHeight, cfg.walkableClimb, *heightField);

	rcFilterWalkableLowHeightSpans (&context, cfg.walkableHeight, *heightField);
	
	if(filterGaps)
	{
		rcFilterGaps(&context, cfg.walkableRadius, cfg.walkableClimb, cfg.walkableHeight, *heightField);
	}
	
	compHeightField = rcAllocCompactHeightfield();
	if ( !rcBuildCompactHeightfield (&context, cfg.walkableHeight, cfg.walkableClimb, *heightField, *compHeightField) )
	{
		Error ("Failed to create compact heightfield for navigation mesh.\n");
	}

	rcFreeHeightField( heightField );

	if( median ) {
		if(!rcMedianFilterWalkableArea(&context, *compHeightField))
		{
			Error ("Failed to apply Median filter to walkable areas.\n");
		}
	}

	if ( !rcErodeWalkableAreaByBox(&context, cfg.walkableRadius, *compHeightField) )
	{
		Error ("Unable to erode walkable surfaces.\n");
	}

	//remove unreachable spans ( examples: roof of map, inside closed spaces such as boxes ) so we don't have to build a navmesh for them
	RemoveUnreachableSpans( *compHeightField );

	if ( !rcBuildDistanceField (&context, *compHeightField) )
	{
		Error ("Failed to build distance field for navigation mesh.\n");
	}

	if ( !rcBuildRegions (&context, *compHeightField, 0, cfg.minRegionArea, cfg.mergeRegionArea) )
	{
		Error ("Failed to build regions for navigation mesh.\n");
	}

	contours = rcAllocContourSet();
	if ( !rcBuildContours (&context, *compHeightField, cfg.maxSimplificationError, cfg.maxEdgeLen, *contours) )
	{
		Error ("Failed to create contour set for navigation mesh.\n");
	}

	polyMesh = rcAllocPolyMesh();
	if ( !rcBuildPolyMesh (&context, *contours, cfg.maxVertsPerPoly, *polyMesh) )
	{
		Error ("Failed to triangulate contours.\n");
	}
	rcFreeContourSet (contours);

	detailedPolyMesh = rcAllocPolyMeshDetail();
	if ( !rcBuildPolyMeshDetail (&context, *polyMesh, *compHeightField, cfg.detailSampleDist, cfg.detailSampleMaxError, *detailedPolyMesh) )
	{
		Error ("Failed to create detail mesh for navigation mesh.\n");
	}

	rcFreeCompactHeightfield (compHeightField);

	// Update poly flags from areas.
	for (int i = 0; i < polyMesh->npolys; ++i)
	{
		if (polyMesh->areas[i] == RC_WALKABLE_AREA) {
			polyMesh->areas[i] = POLYAREA_GROUND;
			polyMesh->flags[i] = POLYFLAGS_WALK;
		} else if (polyMesh->areas[i] == POLYAREA_WATER) {
			polyMesh->flags[i] = POLYFLAGS_SWIM;
		}
		else if (polyMesh->areas[i] == POLYAREA_DOOR) {
			polyMesh->flags[i] = POLYFLAGS_WALK | POLYFLAGS_DOOR;
		}
	}
}

static void BuildSoloMesh(int characterNum) {
	rcConfig cfg;
	rcPolyMesh *polyMesh = rcAllocPolyMesh();
	rcPolyMeshDetail *detailedPolyMesh = rcAllocPolyMeshDetail();

	buildPolyMesh( characterNum, mapmins, mapmaxs, polyMesh, detailedPolyMesh, cfg );
	WriteRecastData( tremClasses[characterNum].name, polyMesh, detailedPolyMesh, &cfg );

	rcFreePolyMesh( polyMesh );
	rcFreePolyMeshDetail( detailedPolyMesh );
}

/*
===========
NavMain
===========
*/
extern "C" int NavMain(int argc, char **argv)
{
	float temp;


	/* note it */
	Sys_Printf("--- Nav ---\n");
	int i;
	/* process arguments */
	for(i = 1; i < (argc - 1); i++)
	{
		if(!Q_stricmp(argv[i],"-cellsize")) {
			i++;
			if(i<(argc - 1)) {
				temp = atof(argv[i]);
				if(temp > 0) {
					cellSize = temp;
				}
			}

		} else if(!Q_stricmp(argv[i],"-cellheight")) {
			i++;
			if(i<(argc - 1)) {
				temp = atof(argv[i]);
				if(temp > 0) {
					cellHeight = temp;
				}
			}

		} else if (!Q_stricmp(argv[i], "-optimistic")){
			stepSize = 20;
		} else if(!Q_stricmp(argv[i], "-median")) {
			median = qtrue;
		} else if(!Q_stricmp(argv[i], "-nogaps")) {
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

	/* parse bsp entities */
	ParseEntities();

	/* get the data into recast */
	LoadGeometry();

	float height = rcAbs(mapmaxs[1]) + rcAbs(mapmins[1]);
	if(height / cellHeight > RC_SPAN_MAX_HEIGHT) {
		Sys_Printf("WARNING: Map geometry is too tall for specified cell height. Increasing cell height to compensate. This may cause a less accurate navmesh.\n");
		float prevCellHeight = cellHeight;
		cellHeight = height / RC_SPAN_MAX_HEIGHT + 0.1;
		Sys_Printf("Previous cellheight: %f\n", prevCellHeight);
		Sys_Printf("New cellheight: %f\n", cellHeight);
	}

	RunThreadsOnIndividual( sizeof( tremClasses ) / sizeof( tremClasses[ 0 ] ), qtrue, BuildSoloMesh);

	return 0;
}