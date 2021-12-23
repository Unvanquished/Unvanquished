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

#include <iostream>
#include <vector>
#include <queue>
#include "engine/qcommon/qcommon.h"
#include "common/cm/cm_patch.h"
#include "common/cm/cm_polylib.h"
#include "common/FileSystem.h"
#include "navgen.h"
#include "sgame/sg_trapcalls.h"

static Log::Logger LOG("sgame.navgen", "");

class UnvContext : public rcContext
{
	/// Clears all log entries.
	void doResetLog() override
	{
	}

	/// Logs a message.
	///  @param[in]   category  The category of the message.
	///  @param[in]   msg     The formatted message.
	///  @param[in]   len     The length of the formatted message.
	void doLog(const rcLogCategory /*category*/, const char* msg, const int /*len*/) override
	{
		if ( m_logEnabled )
		{
			LOG.Notice(msg);
		}
	}

	/// Clears all timers. (Resets all to unused.)
	void doResetTimers() override
	{
	}

	/// Starts the specified performance timer.
	///  @param[in]   label The category of timer.
	void doStartTimer(const rcTimerLabel /*label*/) override
	{
	}

	/// Stops the specified performance timer.
	///  @param[in]   label The category of the timer.
	void doStopTimer(const rcTimerLabel /*label*/) override
	{
	}

	/// Returns the total accumulated time of the specified performance timer.
	///  @param[in]   label The category of the timer.
	///  @return The accumulated time of the timer, or -1 if timers are disabled or the timer has never been started.
	int doGetAccumulatedTime(const rcTimerLabel /*label*/) const override
	{
		return -1;
	}
};

Geometry geo;

float cellHeight = 2.0f;
float stepSize = STEPSIZE;
int tileSize = 64;

struct Character
{
	const char *name;   //appended to filename
	float radius; //radius of agents (BBox maxs[0] or BBox maxs[1])
	float height; //height of agents (BBox maxs[2] - BBox mins[2])
};

static const Character characterArray[] = {
	{ "builder",     20, 40 },
	{ "human_naked", 15, 56 },
	{ "human_bsuit", 15, 76 },
	{ "level0",      15, 30 },
	{ "level1",      15, 30 },
	{ "level2",      23, 36 },
	{ "level2upg",   25, 40 },
	{ "level3",      26, 55 },
	{ "level3upg",   29, 66 },
	{ "level4",      32, 92 }
};

//flag for excluding caulk surfaces
static bool excludeCaulk = true;

//flag for excluding surfaces with surfaceparm sky from navmesh generation
static bool excludeSky = true;

//flag for adding new walkable spans so bots can walk over small gaps
static bool filterGaps = true;

static void WriteNavMeshFile( Str::StringRef filename, const dtTileCache *tileCache, const dtNavMeshParams *params ) {
	int numTiles = 0;
	NavMeshSetHeader header;
	const int maxTiles = tileCache->getTileCount();

	for ( int i = 0; i < maxTiles; i++ )
	{
		const dtCompressedTile *tile = tileCache->getTile( i );
		if ( !tile || !tile->header || !tile->dataSize ) {
			continue;
		}
		numTiles++;
	}

	header.magic = NAVMESHSET_MAGIC;
	header.version = NAVMESHSET_VERSION;
	header.numTiles = numTiles;
	header.cacheParams = *tileCache->getParams();
	header.params = *params;

	SwapNavMeshSetHeader( header );

	qhandle_t file;
	trap_FS_FOpenFile( filename.c_str(), &file, fsMode_t::FS_WRITE );

	if ( !file ) {
		Sys::Drop( "Error opening %s", filename );
	}

	auto Write = [file](void* data, size_t len) {
		if (len != static_cast<size_t>(trap_FS_Write(data, len, file)))
			Sys::Drop("Error writing navmesh file");
	};

	Write( &header, sizeof( header ));

	for ( int i = 0; i < maxTiles; i++ )
	{
		const dtCompressedTile *tile = tileCache->getTile( i );

		if ( !tile || !tile->header || !tile->dataSize ) {
			continue;
		}

		NavMeshTileHeader tileHeader;
		tileHeader.tileRef = tileCache->getTileRef( tile );
		tileHeader.dataSize = tile->dataSize;

		SwapNavMeshTileHeader( tileHeader );
		Write( &tileHeader, sizeof( tileHeader ) );

		unsigned char* data = ( unsigned char * ) malloc( tile->dataSize );

		memcpy( data, tile->data, tile->dataSize );
		if ( LittleLong( 1 ) != 1 ) {
			dtTileCacheHeaderSwapEndian( data, tile->dataSize );
		}

		Write( data, tile->dataSize );

		free( data );
	}
	trap_FS_FCloseFile( file );
}

static std::string mapData;
static void LoadBSP(Str::StringRef name)
{
	if (!mapData.empty()) return;

	// copied from beginning of CM_LoadMap
	std::string mapFile = "maps/" + name + ".bsp";
	mapData = FS::PakPath::ReadFile(mapFile);
	dheader_t* header = reinterpret_cast<dheader_t*>(&mapData[0]);

	// hacky byte swapping for lumps of interest
	// assume all fields size 4 except the string at the beginning of dshader_t
	for (unsigned i = 0; i < sizeof(dheader_t) / 4; i++)
	{
		((int*)header)[i] = LittleLong(((int*)header)[i]);
	}
	for (int lump: {LUMP_MODELS, LUMP_BRUSHES, LUMP_SHADERS, LUMP_BRUSHSIDES, LUMP_PLANES})
	{
		unsigned nBytes = header->lumps[lump].fileofs;
		char *base = &mapData[0] + header->lumps[lump].filelen;
		for (unsigned i = 0; i < nBytes; i += 4)
		{
			if (lump == LUMP_SHADERS && i % sizeof(dshader_t) < offsetof(dshader_t, surfaceFlags))
				continue;
			int* p = reinterpret_cast<int*>(base + i);
			*p = LittleLong(*p);
		}
	}
}

//need this to get the windings for brushes
bool FixWinding( winding_t* w );

static void AddVert( std::vector<float> &verts, std::vector<int> &tris, vec3_t vert ) {
	vec3_t recastVert;
	VectorCopy( vert, recastVert );
	quake2recast( recastVert );
	int index = 0;
	for ( int i = 0; i < 3; i++ ) {
		verts.push_back( recastVert[i] );
	}
	index = ( verts.size() - 3 ) / 3;
	tris.push_back( index );
}

static void AddTri( std::vector<float> &verts, std::vector<int> &tris, vec3_t v1, vec3_t v2, vec3_t v3 ) {
	AddVert( verts, tris, v1 );
	AddVert( verts, tris, v2 );
	AddVert( verts, tris, v3 );
}

// TODO: Can this stuff be done using the already-loaded CM data structures
// (e.g. cbrush_t instead of dbrush_t) instead of reopening the BSP?
static void LoadBrushTris( std::vector<float> &verts, std::vector<int> &tris ) {
	int j;

	int solidFlags = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	int surfaceSkip = 0;

	if ( excludeSky ) {
		surfaceSkip = SURF_SKY;
	}

	const byte* const cmod_base = reinterpret_cast<const byte*>(mapData.data());
	auto& header = *reinterpret_cast<const dheader_t*>(mapData.data());

	// Each one of these lumps should be byte-swapped in LoadBSP
	const lump_t& modelsLump = header.lumps[LUMP_MODELS];
	const lump_t& brushesLump = header.lumps[LUMP_BRUSHES];
	const lump_t& shadersLump = header.lumps[LUMP_SHADERS];
	const lump_t& sidesLump = header.lumps[LUMP_BRUSHSIDES];
	const lump_t& planesLump = header.lumps[LUMP_PLANES];

	/* get model, index 0 is worldspawn entity */
	if ( modelsLump.filelen / sizeof(dmodel_t) <= 0 ) {
		Sys::Drop("No brushes found. Aborting.");
	}
	auto* model = reinterpret_cast<const dmodel_t*>(cmod_base + modelsLump.fileofs);
	auto* bspBrushes = reinterpret_cast<const dbrush_t*>(cmod_base + brushesLump.fileofs);
	auto* bspShaders = reinterpret_cast<const dshader_t*>(cmod_base + shadersLump.fileofs);
	auto* bspBrushSides = reinterpret_cast<const dbrushside_t*>(cmod_base + sidesLump.fileofs);
	auto* bspPlanes = reinterpret_cast<const dplane_t*>(cmod_base + planesLump.fileofs);

	//go through the brushes
	for ( int i = model->firstBrush, m = 0; m < model->numBrushes; i++, m++ ) {
		int numSides = bspBrushes[i].numSides;
		int firstSide = bspBrushes[i].firstSide;
		const dshader_t* brushShader = &bspShaders[bspBrushes[i].shaderNum];

		if ( !( brushShader->contentFlags & solidFlags ) ) {
			continue;
		}
		/* walk the list of brush sides */
		for ( int p = 0; p < numSides; p++ )
		{
			/* get side and plane */
			const dbrushside_t *side = &bspBrushSides[p + firstSide];
			const dplane_t *plane = &bspPlanes[side->planeNum];
			const dshader_t* shader = &bspShaders[side->shaderNum];

			if ( shader->surfaceFlags & surfaceSkip ) {
				continue;
			}

			if ( excludeCaulk && !Q_stricmp( shader->shader, "textures/common/caulk" ) ) {
				continue;
			}

			/* make huge winding */
			winding_t *w = BaseWindingForPlane( plane->normal, plane->dist );

			/* walk the list of brush sides */
			for ( j = 0; j < numSides && w != nullptr; j++ )
			{
				const dbrushside_t *chopSide = &bspBrushSides[j + firstSide];
				if ( chopSide == side ) {
					continue;
				}
				if ( chopSide->planeNum == ( side->planeNum ^ 1 ) ) {
					continue;       /* back side clipaway */

				}
				const dplane_t *chopPlane = &bspPlanes[chopSide->planeNum ^ 1];

				ChopWindingInPlace( &w, chopPlane->normal, chopPlane->dist, 0 );

				/* ydnar: fix broken windings that would generate trifans */
				FixWinding( w );
			}

			if ( w ) {
				for ( int j = 2; j < w->numpoints; j++ ) {
					AddTri( verts, tris, w->p[0], w->p[j - 1], w->p[j] );
				}
				FreeWinding( w );
			}
		}
	}
}

static void LoadPatchTris( std::vector<float> &verts, std::vector<int> &tris ) {
// Disabling this code for now. It should be straightforward to port, but I haven't
// bothered because I can't see any effect on the navmesh, even on parts with curved
// areas using patches. So maybe we are better of dropping this and saving some triangles.
	Q_UNUSED(verts);
	Q_UNUSED(tris);
#if 0
	vec3_t mins, maxs;
	int solidFlags = 0;
	int temp = 0;

	char surfaceparm[16];

	strcpy( surfaceparm, "default" );
	ApplySurfaceParm( surfaceparm, &solidFlags, NULL, NULL );

	strcpy( surfaceparm, "playerclip" );
	ApplySurfaceParm( surfaceparm, &temp, NULL, NULL );
	solidFlags |= temp;

	/*
	    Patches are not used during the bsp building process where
	    the generated portals are flooded through from all entity positions
	    if even one entity reaches the void, the map will not be compiled
	    Therefore, we can assume that any patch which lies outside the collective
	    bounds of the brushes cannot be reached by entitys
	 */

	// calculate bounds of all verts
	rcCalcBounds( &verts[ 0 ], verts.size() / 3, mins, maxs );

	// convert from recast to quake3 coordinates
	recast2quake( mins );
	recast2quake( maxs );

	vec3_t tmin, tmax;

	// need to recalculate mins and maxs because they no longer represent
	// the minimum and maximum vector components respectively
	ClearBounds( tmin, tmax );

	AddPointToBounds( mins, tmin, tmax );
	AddPointToBounds( maxs, tmin, tmax );

	VectorCopy( tmin, mins );
	VectorCopy( tmax, maxs );

	/* get model, index 0 is worldspawn entity */
	const bspModel_t *model = &bspModels[0];
	for ( int k = model->firstBSPSurface, n = 0; n < model->numBSPSurfaces; k++,n++ )
	{
		const bspDrawSurface_t *surface = &bspDrawSurfaces[k];

		if ( !( bspShaders[surface->shaderNum].contentFlags & solidFlags ) ) {
			continue;
		}

		if ( surface->surfaceType != MST_PATCH ) {
			continue;
		}

		if ( !surface->patchWidth ) {
			continue;
		}

		cGrid_t grid;
		grid.width = surface->patchWidth;
		grid.height = surface->patchHeight;
		grid.wrapHeight = qfalse;
		grid.wrapWidth = qfalse;

		bspDrawVert_t *curveVerts = &bspDrawVerts[surface->firstVert];

		// make sure the patch intersects the bounds of the brushes
		ClearBounds( tmin, tmax );

		for ( int x = 0; x < grid.width; x++ )
		{
			for ( int y = 0; y < grid.height; y++ )
			{
				AddPointToBounds( curveVerts[ y * grid.width + x ].xyz, tmin, tmax );
			}
		}

		if ( !BoundsIntersect( tmin, tmax, mins, maxs ) ) {
			// we can safely ignore this patch surface
			continue;
		}

		for ( int x = 0; x < grid.width; x++ )
		{
			for ( int y = 0; y < grid.height; y++ )
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

		for ( int x = 0; x < ( grid.width - 1 ); x++ )
		{
			for ( int y = 0; y < ( grid.height - 1 ); y++ )
			{
				/* set indexes */
				float *p1 = grid.points[ x ][ y ];
				float *p2 = grid.points[ x + 1 ][ y ];
				float *p3 = grid.points[ x + 1 ][ y + 1 ];
				AddTri( verts, tris, p1, p2, p3 );

				p1 = grid.points[ x + 1 ][ y + 1 ];
				p2 = grid.points[ x ][ y + 1 ];
				p3 = grid.points[ x ][ y ];
				AddTri( verts, tris, p1, p2, p3 );
			}
		}
	}
#endif
}

static void LoadGeometry(){
	std::vector<float> verts;
	std::vector<int> tris;

	LOG.Debug( "loading geometry..." );
	int numVerts, numTris;

	//count surfaces
	LoadBrushTris( verts, tris );
	LoadPatchTris( verts, tris );

	numTris = tris.size() / 3;
	numVerts = verts.size() / 3;

	LOG.Debug( "Using %d triangles", numTris );
	LOG.Debug( "Using %d vertices", numVerts );

	geo.init( &verts[ 0 ], numVerts, &tris[ 0 ], numTris );

	const float *mins = geo.getMins();
	const float *maxs = geo.getMaxs();

	LOG.Debug( "set recast world bounds to" );
	LOG.Debug( "min: %f %f %f", mins[0], mins[1], mins[2] );
	LOG.Debug( "max: %f %f %f", maxs[0], maxs[1], maxs[2] );
}

// Modified version of Recast's rcErodeWalkableArea that uses an AABB instead of a cylindrical radius
static bool rcErodeWalkableAreaByBox( rcContext* ctx, int boxRadius, rcCompactHeightfield& chf ){
	rcAssert( ctx );

	const int w = chf.width;
	const int h = chf.height;

	ctx->startTimer( RC_TIMER_ERODE_AREA );

	unsigned char* dist = (unsigned char*)rcAlloc( sizeof( unsigned char ) * chf.spanCount, RC_ALLOC_TEMP );
	if ( !dist ) {
		ctx->log( RC_LOG_ERROR, "erodeWalkableArea: Out of memory 'dist' (%d).", chf.spanCount );
		return false;
	}

	// Init distance.
	memset( dist, 0xff, sizeof( unsigned char ) * chf.spanCount );

	// Mark boundary cells.
	for ( int y = 0; y < h; ++y )
	{
		for ( int x = 0; x < w; ++x )
		{
			const rcCompactCell& c = chf.cells[x + y * w];
			for ( int i = (int)c.index, ni = (int)( c.index + c.count ); i < ni; ++i )
			{
				if ( chf.areas[i] == RC_NULL_AREA ) {
					dist[i] = 0;
				}
				else
				{
					const rcCompactSpan& s = chf.spans[i];
					int nc = 0;
					for ( int dir = 0; dir < 4; ++dir )
					{
						if ( rcGetCon( s, dir ) != RC_NOT_CONNECTED ) {
							const int nx = x + rcGetDirOffsetX( dir );
							const int ny = y + rcGetDirOffsetY( dir );
							const int nidx = (int)chf.cells[nx + ny * w].index + rcGetCon( s, dir );
							if ( chf.areas[nidx] != RC_NULL_AREA ) {
								nc++;
							}
						}
					}
					// At least one missing neighbour.
					if ( nc != 4 ) {
						dist[i] = 0;
					}
				}
			}
		}
	}

	unsigned char nd;

	// Pass 1
	for ( int y = 0; y < h; ++y )
	{
		for ( int x = 0; x < w; ++x )
		{
			const rcCompactCell& c = chf.cells[x + y * w];
			for ( int i = (int)c.index, ni = (int)( c.index + c.count ); i < ni; ++i )
			{
				const rcCompactSpan& s = chf.spans[i];

				if ( rcGetCon( s, 0 ) != RC_NOT_CONNECTED ) {
					// (-1,0)
					const int ax = x + rcGetDirOffsetX( 0 );
					const int ay = y + rcGetDirOffsetY( 0 );
					const int ai = (int)chf.cells[ax + ay * w].index + rcGetCon( s, 0 );
					const rcCompactSpan& as = chf.spans[ai];
					nd = (unsigned char)rcMin( (int)dist[ai] + 2, 255 );
					if ( nd < dist[i] ) {
						dist[i] = nd;
					}

					// (-1,-1)
					if ( rcGetCon( as, 3 ) != RC_NOT_CONNECTED ) {
						const int aax = ax + rcGetDirOffsetX( 3 );
						const int aay = ay + rcGetDirOffsetY( 3 );
						const int aai = (int)chf.cells[aax + aay * w].index + rcGetCon( as, 3 );
						nd = (unsigned char)rcMin( (int)dist[aai] + 2, 255 );
						if ( nd < dist[i] ) {
							dist[i] = nd;
						}
					}
				}
				if ( rcGetCon( s, 3 ) != RC_NOT_CONNECTED ) {
					// (0,-1)
					const int ax = x + rcGetDirOffsetX( 3 );
					const int ay = y + rcGetDirOffsetY( 3 );
					const int ai = (int)chf.cells[ax + ay * w].index + rcGetCon( s, 3 );
					const rcCompactSpan& as = chf.spans[ai];
					nd = (unsigned char)rcMin( (int)dist[ai] + 2, 255 );
					if ( nd < dist[i] ) {
						dist[i] = nd;
					}

					// (1,-1)
					if ( rcGetCon( as, 2 ) != RC_NOT_CONNECTED ) {
						const int aax = ax + rcGetDirOffsetX( 2 );
						const int aay = ay + rcGetDirOffsetY( 2 );
						const int aai = (int)chf.cells[aax + aay * w].index + rcGetCon( as, 2 );
						nd = (unsigned char)rcMin( (int)dist[aai] + 2, 255 );
						if ( nd < dist[i] ) {
							dist[i] = nd;
						}
					}
				}
			}
		}
	}

	// Pass 2
	for ( int y = h - 1; y >= 0; --y )
	{
		for ( int x = w - 1; x >= 0; --x )
		{
			const rcCompactCell& c = chf.cells[x + y * w];
			for ( int i = (int)c.index, ni = (int)( c.index + c.count ); i < ni; ++i )
			{
				const rcCompactSpan& s = chf.spans[i];

				if ( rcGetCon( s, 2 ) != RC_NOT_CONNECTED ) {
					// (1,0)
					const int ax = x + rcGetDirOffsetX( 2 );
					const int ay = y + rcGetDirOffsetY( 2 );
					const int ai = (int)chf.cells[ax + ay * w].index + rcGetCon( s, 2 );
					const rcCompactSpan& as = chf.spans[ai];
					nd = (unsigned char)rcMin( (int)dist[ai] + 2, 255 );
					if ( nd < dist[i] ) {
						dist[i] = nd;
					}

					// (1,1)
					if ( rcGetCon( as, 1 ) != RC_NOT_CONNECTED ) {
						const int aax = ax + rcGetDirOffsetX( 1 );
						const int aay = ay + rcGetDirOffsetY( 1 );
						const int aai = (int)chf.cells[aax + aay * w].index + rcGetCon( as, 1 );
						nd = (unsigned char)rcMin( (int)dist[aai] + 2, 255 );
						if ( nd < dist[i] ) {
							dist[i] = nd;
						}
					}
				}
				if ( rcGetCon( s, 1 ) != RC_NOT_CONNECTED ) {
					// (0,1)
					const int ax = x + rcGetDirOffsetX( 1 );
					const int ay = y + rcGetDirOffsetY( 1 );
					const int ai = (int)chf.cells[ax + ay * w].index + rcGetCon( s, 1 );
					const rcCompactSpan& as = chf.spans[ai];
					nd = (unsigned char)rcMin( (int)dist[ai] + 2, 255 );
					if ( nd < dist[i] ) {
						dist[i] = nd;
					}

					// (-1,1)
					if ( rcGetCon( as, 0 ) != RC_NOT_CONNECTED ) {
						const int aax = ax + rcGetDirOffsetX( 0 );
						const int aay = ay + rcGetDirOffsetY( 0 );
						const int aai = (int)chf.cells[aax + aay * w].index + rcGetCon( as, 0 );
						nd = (unsigned char)rcMin( (int)dist[aai] + 2, 255 );
						if ( nd < dist[i] ) {
							dist[i] = nd;
						}
					}
				}
			}
		}
	}

	const unsigned char thr = (unsigned char)( boxRadius * 2 );
	for ( int i = 0; i < chf.spanCount; ++i ) {
		if ( dist[i] < thr ) {
			chf.areas[i] = RC_NULL_AREA;
		}
	}

	rcFree( dist );

	ctx->stopTimer( RC_TIMER_ERODE_AREA );

	return true;
}

/*
   rcFilterGaps

   Does a super simple sampling of ledges to detect and fix
   any "gaps" in the heightfield that are narrow enough for us to walk over
   because of our AABB based collision system
 */
static void rcFilterGaps( rcContext *ctx, int walkableRadius, int walkableClimb, int walkableHeight, rcHeightfield &solid ) {
	const int h = solid.height;
	const int w = solid.width;
	const int MAX_HEIGHT = 0xffff;
	std::vector<int> spanData;
	spanData.reserve( 500 * 3 );
	std::vector<int> data;
	data.reserve( ( walkableRadius * 2 - 1 ) * 3 );

	//for every span in the heightfield
	for ( int y = 0; y < h; ++y ) {
		for ( int x = 0; x < w; ++x ) {
			//check each span in the column
			for ( rcSpan *s = solid.spans[x + y * w]; s; s = s->next ) {
				//bottom and top of the "base" span
				const int sbot = s->smax;

				//if the span is walkable
				if ( s->area != RC_NULL_AREA ) {
					//check all neighbor connections
					for ( int dir = 0; dir < 4; dir++ ) {
						const int dirx = rcGetDirOffsetX( dir );
						const int diry = rcGetDirOffsetY( dir );
						int dx = x;
						int dy = y;
						bool freeSpace = false;
						bool stop = false;

						if ( dx < 0 || dy < 0 || dx >= w || dy >= h ) {
							continue;
						}

						//keep going the direction for walkableRadius * 2 - 1 spans
						//because we can walk as long as at least part of our bbox is on a solid surface
						for ( int i = 1; i < walkableRadius * 2; i++ ) {
							dx = dx + dirx;
							dy = dy + diry;
							if ( dx < 0 || dy < 0 || dx >= w || dy >= h ) {
								i--;
								freeSpace = false;
								stop = false;
								break;
							}

							//tells if there is space here for us to stand
							freeSpace = false;

							//go through the column
							for ( rcSpan *ns = solid.spans[dx + dy * w]; ns; ns = ns->next ) {
								int nsbot = ns->smax;
								int nstop = ( ns->next ) ? ( ns->next->smin ) : MAX_HEIGHT;

								//if there is a span within walkableClimb of the base span, we have reached the end of the gap (if any)
								if ( rcAbs( sbot - nsbot ) <= walkableClimb && ns->area != RC_NULL_AREA ) {
									//set flag telling us to stop
									stop = true;

									//only add spans if we have gone for more than 1 iteration
									//if we stop at the first iteration, it means there was no gap to begin with
									if ( i > 1 ) {
										freeSpace = true;
									}
									break;
								}

								if ( nsbot < sbot && nstop >= sbot + walkableHeight ) {
									//tell that we found walkable space within reach of the previous span
									freeSpace = true;
									//add this span to a temporary storage location
									data.push_back( dx );
									data.push_back( dy );
									data.push_back( sbot );
									break;
								}
							}

							//stop if there is no more freespace, or we have reached end of gap
							if ( stop || !freeSpace ) {
								break;
							}
						}
						//move the spans from the temp location to the storage
						//we check freeSpace to make sure we don't add a
						//span when we stop at the first iteration (because there is no gap)
						//checking stop tells us if there was a span to complete the "bridge"
						if ( freeSpace && stop ) {
							const int N = data.size();
							for ( int i = 0; i < N; i++ )
								spanData.push_back( data[i] );
						}
						data.clear();
					}
				}
			}
		}
	}

	//add the new spans
	//we cant do this in the loop, because the loop would then iterate over those added spans
	for ( std::vector<int>::size_type i = 0; i < spanData.size(); i += 3 ) {
		rcAddSpan( ctx, solid, spanData[i], spanData[i + 1], spanData[i + 2] - 1, spanData[i + 2], RC_WALKABLE_AREA, walkableClimb );
	}
}

static int rasterizeTileLayers( rcContext &context, int tx, int ty, const rcConfig &mcfg, TileCacheData *data, int maxLayers ){
	rcConfig cfg;

	FastLZCompressor comp;
	RasterizationContext rc;

	const float tcs = mcfg.tileSize * mcfg.cs;

	memcpy( &cfg, &mcfg, sizeof( cfg ) );

	// find tile bounds
	// easy optimisation here: avoid recalculating things Y * X times
	cfg.bmin[ 0 ] = mcfg.bmin[ 0 ] + tx * tcs;
	cfg.bmin[ 1 ] = mcfg.bmin[ 1 ];
	cfg.bmin[ 2 ] = mcfg.bmin[ 2 ] + ty * tcs;

	cfg.bmax[ 0 ] = mcfg.bmin[ 0 ] + ( tx + 1 ) * tcs;
	cfg.bmax[ 1 ] = mcfg.bmax[ 1 ];
	cfg.bmax[ 2 ] = mcfg.bmin[ 2 ] + ( ty + 1 ) * tcs;

	// expand bounds by border size
	// easy optimisation here: borderSize and cs (cs: The xz-plane cell size to use for fields)
	// are constants (coming directly from mcfg, previously named cfg, you follow?).
	cfg.bmin[ 0 ] -= cfg.borderSize * cfg.cs;
	cfg.bmin[ 2 ] -= cfg.borderSize * cfg.cs;

	cfg.bmax[ 0 ] += cfg.borderSize * cfg.cs;
	cfg.bmax[ 2 ] += cfg.borderSize * cfg.cs;

	rc.solid = rcAllocHeightfield();

	if ( !rcCreateHeightfield( &context, *rc.solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch ) ) {
		Sys::Drop( "Failed to create heightfield for navigation mesh" );
	}

	//I understand that using std::vector prevents NiH's proudness.
	const float *verts = geo.getVerts();
	const int nverts = geo.getNumVerts();
	const rcChunkyTriMesh *chunkyMesh = geo.getChunkyMesh();

	rc.triareas = new unsigned char[ chunkyMesh->maxTrisPerChunk ];
	//you know what? This will never be reached, because new throws exceptions, by default.
	//really, should just use STL or C, not raw C++ with C's bugs.
	if ( !rc.triareas ) {
		Sys::Drop( "Out of memory rc.triareas" );
	}

	float tbmin[ 2 ], tbmax[ 2 ];

	tbmin[ 0 ] = cfg.bmin[ 0 ];
	tbmin[ 1 ] = cfg.bmin[ 2 ];
	tbmax[ 0 ] = cfg.bmax[ 0 ];
	tbmax[ 1 ] = cfg.bmax[ 2 ];

	int *cid = new int[ chunkyMesh->nnodes ];

	//do not search for this in libraries, you won't find it. It's in RecastDemo's code.
	//It " Creates partitioned triangle mesh (AABB tree), where each node contains at max trisPerChunk triangles."
	//why? No idea.
	const int ncid = rcGetChunksOverlappingRect( chunkyMesh, tbmin, tbmax, cid, chunkyMesh->nnodes );
	if ( !ncid ) {
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

	//makes them walkable (unlike the other filters, would probably kill some people to write meaningful code)
	rcFilterLowHangingWalkableObstacles( &context, cfg.walkableClimb, *rc.solid );

	//dont filter ledge spans since characters CAN walk on ledges due to using a bbox for movement collision
	//makes them un-walkable
	//rcFilterLedgeSpans (&context, cfg.walkableHeight, cfg.walkableClimb, *rc.solid);

	//makes them un-walkable
	rcFilterWalkableLowHeightSpans( &context, cfg.walkableHeight, *rc.solid );

	//by default, make gaps walkable (because using a BBox system makes agents cubes). In theory a great idea, but
	//it does not work correctly.
	if ( filterGaps ) {
		rcFilterGaps( &context, cfg.walkableRadius, cfg.walkableClimb, cfg.walkableHeight, *rc.solid );
	}

	rc.chf = rcAllocCompactHeightfield();

	if ( !rcBuildCompactHeightfield( &context, cfg.walkableHeight, cfg.walkableClimb, *rc.solid, *rc.chf ) ) {
		Sys::Drop( "Failed to create compact heightfield for navigation mesh" );
	}

	if ( !rcErodeWalkableAreaByBox( &context, cfg.walkableRadius, *rc.chf ) ) {
		Sys::Drop( "Unable to erode walkable surfaces" );
	}

	rc.lset = rcAllocHeightfieldLayerSet();

	if ( !rc.lset ) {
		Sys::Drop( "Out of memory heightfield layer set" );
	}

	if ( !rcBuildHeightfieldLayers( &context, *rc.chf, cfg.borderSize, cfg.walkableHeight, *rc.lset ) ) {
		Sys::Drop( "Could not build heightfield layers" );
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

		if ( dtStatusFailed( status ) ) {
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

static void BuildNavMesh( int characterNum ){
	const Character &agent = characterArray[ characterNum ];

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
	memset( &cfg, 0, sizeof( cfg ) );

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
	cfg.borderSize = cfg.walkableRadius * 2;
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

	if ( !tileCache ) {
		Sys::Drop( "Could not allocate tile cache" );
	}

	LinearAllocator alloc( 32000 );
	FastLZCompressor comp;
	MeshProcess proc;

	dtStatus status = tileCache->init( &tcparams, &alloc, &comp, &proc );

	if ( dtStatusFailed( status ) ) {
		if ( dtStatusDetail( status, DT_INVALID_PARAM ) ) {
			Sys::Drop( "Could not init tile cache: Invalid parameter" );
		}
		else
		{
			Sys::Drop( "Could not init tile cache" );
		}
	}

	UnvContext context;
	context.enableLog( true );

	//iterate over all tiles (number is determined by rcCalcGridSize)
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
				if ( dtStatusFailed( status ) ) {
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

	std::string mapname = Cvar::GetValue("mapname");
	std::string filename = Str::Format("maps/%s-%s.navMesh", mapname, agent.name);
	WriteNavMeshFile( filename, tileCache, &params );

	dtFreeTileCache( tileCache );
}

void GenerateNavmesh(Str::StringRef species)
{
	unsigned characterNum;
	for (characterNum = 0; characterNum < ARRAY_LEN(characterArray); characterNum++) {
		if (species == characterArray[characterNum].name) {
			break;
		}
	}
	if (characterNum == ARRAY_LEN(characterArray)) {
		LOG.Warn("class '%s' not valid for navmesh generation", species);
		return;
	}

	LOG.Notice( "Generating navmesh for %s", species );

	// TODO: flags? (cellHeight, stepSize, excludeCaulk, excludeSky, filterGaps)

	LoadBSP(Cvar::GetValue("mapname"));
	LoadGeometry();

	float height = rcAbs( geo.getMaxs()[1] ) + rcAbs( geo.getMins()[1] );
	if ( height / cellHeight > RC_SPAN_MAX_HEIGHT ) {
		LOG.Warn("Map geometry is too tall for specified cell height. Increasing cell height to compensate. This may cause a less accurate navmesh." );
		float prevCellHeight = cellHeight;
		float minCellHeight = height / RC_SPAN_MAX_HEIGHT;

		int divisor = ( int ) stepSize;

		while ( divisor && cellHeight < minCellHeight )
		{
			cellHeight = stepSize / divisor;
			divisor--;
		}

		if ( !divisor ) {
			Sys::Drop( "Map is too tall to generate a navigation mesh" );
		}

		LOG.Notice( "Previous cell height: %f", prevCellHeight );
		LOG.Notice( "New cell height: %f", cellHeight );
	}

	BuildNavMesh(characterNum);
	LOG.Notice("Finished generating navmesh for %s", species);
}
