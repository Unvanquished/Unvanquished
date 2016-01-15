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

#include "cm_public.h"
#include "cm_polylib.h"

#include "common/Cvar.h"
#include "common/Log.h"
#include "engine/qcommon/qcommon.h"

// fake submodel handles
#define CAPSULE_MODEL_HANDLE ( MAX_SUBMODELS )
#define BOX_MODEL_HANDLE     ( MAX_SUBMODELS + 1)

struct cbrushedge_t
{
	vec3_t p0;
	vec3_t p1;
};

struct cNode_t
{
	cplane_t  *plane;
	int       planeNum;
	int       children[ 2 ]; // negative numbers are leafs
	winding_t *winding;
};

struct cLeaf_t
{
	int cluster;
	int area;

	int firstLeafBrush;
	int numLeafBrushes;

	int firstLeafSurface;
	int numLeafSurfaces;
};

struct cmodel_t
{
	vec3_t  mins, maxs;
	cLeaf_t leaf; // submodels don't reference the main tree
};

struct cbrushside_t
{
	cplane_t  *plane;
	int       planeNum;
	int       surfaceFlags;
	winding_t *winding;
};

struct cbrush_t
{
	int          contents;
	vec3_t       bounds[ 2 ];
	int          numsides;
	cbrushside_t *sides;
	int          checkcount; // to avoid repeated testings
	bool     collided; // marker for optimisation
	cbrushedge_t *edges;
	int          numEdges;
};

struct cPlane_t
{
	float           plane[ 4 ];
	int             signbits; // signx + (signy<<1) + (signz<<2), used as lookup during collision
	cPlane_t *hashChain;
};

// 3 or four + 6 axial bevels + 4 or 3 * 4 edge bevels
#define MAX_FACET_BEVELS ( 4 + 6 + 16 )

// a facet is a subdivided element of a patch approximation or model
struct cFacet_t
{
	int      surfacePlane;
	int      numBorders;
	int      borderPlanes[ MAX_FACET_BEVELS ];
	bool     borderInward[ MAX_FACET_BEVELS ];
	bool borderNoAdjust[ MAX_FACET_BEVELS ];
};

struct cSurfaceCollide_t
{
	vec3_t   bounds[ 2 ];
	int      numPlanes; // surface planes plus edge planes
	cPlane_t *planes;

	int      numFacets;
	cFacet_t *facets;
};

struct cSurface_t
{
	int               checkcount; // to avoid repeated testings
	int               surfaceFlags;
	int               contents;
	cSurfaceCollide_t *sc;
	int               type;
};

struct cArea_t
{
	int floodnum;
	int floodvalid;
};

struct clipMap_t
{
	int          numShaders;
	dshader_t    *shaders;

	int          numBrushSides;
	cbrushside_t *brushsides;

	int          numPlanes;
	cplane_t     *planes;

	int          numNodes;
	cNode_t      *nodes;

	int          numLeafs;
	cLeaf_t      *leafs;

	int          numLeafBrushes;
	int          *leafbrushes;

	int          numLeafSurfaces;
	int          *leafsurfaces;

	int          numSubModels;
	cmodel_t     *cmodels;

	int          numBrushes;
	cbrush_t     *brushes;

	int          numClusters;
	int          clusterBytes;
	byte         *visibility;
	bool     vised; // if false, visibility is just a single cluster of ffs

	int          numEntityChars;
	char         *entityString;

	int          numAreas;
	cArea_t      *areas;
	int          *areaPortals; // [ numAreas*numAreas ] reference counts

	int          numSurfaces;
	cSurface_t   **surfaces; // non-patches will be nullptr

	int          floodvalid;
	int          checkcount; // incremented on each trace
	bool     perPolyCollision;
};

// keep 1/8 unit away to keep the position valid before network snapping
// and to avoid various numeric issues
#define SURFACE_CLIP_EPSILON ( 0.125 )

extern clipMap_t cm;
extern int       c_pointcontents;
extern int       c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces;
extern Cvar::Cvar<bool> cm_forceTriangles;
extern Log::Logger cmLog;

// cm_test.c

struct biSphere_t
{
	float startRadius;
	float endRadius;
};

// Used for oriented capsule collision detection
struct sphere_t
{
	float  radius;
	float  halfheight;
	vec3_t offset;
};

struct traceWork_t
{
	traceType_t type;
	vec3_t      start;
	vec3_t      end;
	vec3_t      size[ 2 ]; // size of the box being swept through the model
	vec3_t      offsets[ 8 ]; // [signbits][x] = either size[0][x] or size[1][x]
	float       maxOffset; // longest corner length from origin
	vec3_t      extents; // greatest of abs(size[0]) and abs(size[1])
	vec3_t      bounds[ 2 ]; // enclosing box of start and end surrounding by size
	vec3_t      modelOrigin; // origin of the model tracing through
	int         contents; // ored contents of the model tracing through
	int         skipContents; // ored contents that shall be ignored
	bool    isPoint; // optimized case
	trace_t     trace; // returned from trace call
	sphere_t    sphere; // sphere for oriendted capsule collision
	biSphere_t  biSphere;
	bool    testLateralCollision; // whether or not to test for lateral collision
};

struct leafList_t
{
	int      count;
	int      maxcount;
	bool overflowed;
	int      *list;
	vec3_t   bounds[ 2 ];
	int      lastLeaf; // for overflows where each leaf can't be stored individually
	void ( *storeLeafs )( leafList_t *ll, int nodenum );
};

#define SUBDIVIDE_DISTANCE 16 //4 // never more than this units away from curve
#define PLANE_TRI_EPSILON  0.1
#define WRAP_POINT_EPSILON 0.1

cSurfaceCollide_t *CM_GeneratePatchCollide( int width, int height, vec3_t *points );
void              CM_ClearLevelPatches();

// cm_trisoup.c

struct cTriangleSoup_t
{
	int    numTriangles;
	int    indexes[ SHADER_MAX_INDEXES ];

	int    trianglePlanes[ SHADER_MAX_TRIANGLES ];

	vec3_t points[ SHADER_MAX_TRIANGLES ][ 3 ];
};

cSurfaceCollide_t              *CM_GenerateTriangleSoupCollide( int numVertexes, vec3_t *vertexes, int numIndexes, int *indexes );


void* CM_Alloc( int size );

// cm_plane.c

extern int numPlanes;
extern cPlane_t planes[];

extern int numFacets;
extern cFacet_t facets[];

void     CM_ResetPlaneCounts();
int      CM_FindPlane2( float plane[ 4 ], bool *flipped );
int      CM_FindPlane( const float *p1, const float *p2, const float *p3 );
int      CM_PointOnPlaneSide( float *p, int planeNum );
bool CM_ValidateFacet( cFacet_t *facet );
void     CM_AddFacetBevels( cFacet_t *facet );
bool CM_GenerateFacetFor3Points( cFacet_t *facet, const vec3_t p1, const vec3_t p2, const vec3_t p3 );
bool CM_GenerateFacetFor4Points( cFacet_t *facet, const vec3_t p1, const vec3_t p2, const vec3_t p3, const vec3_t p4 );


// cm_test.c
extern const cSurfaceCollide_t *debugSurfaceCollide;
extern const cFacet_t          *debugFacet;
extern bool                debugBlock;
extern vec3_t                  debugBlockPoints[ 4 ];

void                           CM_StoreLeafs( leafList_t *ll, int nodenum );
void                           CM_StoreBrushes( leafList_t *ll, int nodenum );

void                           CM_BoxLeafnums_r( leafList_t *ll, int nodenum );

cmodel_t                       *CM_ClipHandleToModel( clipHandle_t handle );

// XreaL BEGIN
bool                       CM_BoundsIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 );
bool                       CM_BoundsIntersectPoint( const vec3_t mins, const vec3_t maxs, const vec3_t point );

// XreaL END
