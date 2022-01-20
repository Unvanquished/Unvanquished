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

#include "common/Common.h"

#include <iostream>
#include <vector>
#include <queue>

// Filesystem APIs are stupidly declared in different headers
#ifdef BUILD_CGAME
#include "engine/client/cg_api.h"
#else
#include "sgame/sg_trapcalls.h"
#endif

#include "engine/qcommon/qcommon.h"
#include "common/cm/cm_patch.h"
#include "common/cm/cm_polylib.h"
#include "common/FileSystem.h"
#include "navgen.h"

static Log::Logger LOG( VM_STRING_PREFIX "navgen", "", Log::Level::NOTICE );

void UnvContext::doLog(const rcLogCategory /*category*/, const char* msg, const int /*len*/)
{
	if ( m_logEnabled )
	{
		LOG.Notice(msg);
	}
}

static NavgenStatus::Code CodeForFailedDtStatus(dtStatus status)
{
	if ( dtStatusDetail( status, DT_OUT_OF_MEMORY ) )
	{
		return NavgenStatus::TRANSIENT_FAILURE;
	}
	return NavgenStatus::PERMANENT_FAILURE;
}

static int tileSize = 64;

void NavmeshGenerator::WriteFile() {
	if ( d_->status.code == NavgenStatus::TRANSIENT_FAILURE )
	{
		return; // Don't write anything
	}

	// there are 22 bits to store a tile and its polys
	int tileBits = rcMin( ( int ) dtIlog2( dtNextPow2( d_->tcparams.maxTiles ) ), 14 );
	int polyBits = 22 - tileBits;

	dtNavMeshParams params;
	dtVcopy( params.orig, d_->tcparams.orig );
	params.tileHeight = d_->tcparams.height * d_->cfg.cs;
	params.tileWidth = d_->tcparams.width * d_->cfg.cs;
	params.maxTiles = 1 << tileBits;
	params.maxPolys = 1 << polyBits;

	std::string filename = NavmeshFilename( mapName_, BG_Class( d_->species )->name );

	NavMeshSetHeader header;
	int maxTiles;

	if ( d_->status.code == NavgenStatus::OK )
	{
		maxTiles = d_->tileCache->getTileCount();
		int numTiles = 0;

		for ( int i = 0; i < maxTiles; i++ )
		{
			const dtCompressedTile *tile = d_->tileCache->getTile( i );
			if ( !tile || !tile->header || !tile->dataSize ) {
				continue;
			}
			numTiles++;
		}
		header.numTiles = numTiles;
		header.cacheParams = *d_->tileCache->getParams();
		header.params = params;
	}
	else
	{
		header.params = {};
		header.cacheParams = {};
		header.params.tileHeight = PERMANENT_NAVGEN_ERROR;
		header.numTiles = 1;
	}

	header.magic = NAVMESHSET_MAGIC;
	header.version = NAVMESHSET_VERSION;

	SwapNavMeshSetHeader( header );

	qhandle_t file;
	trap_FS_FOpenFile( filename.c_str(), &file, fsMode_t::FS_WRITE );

	if ( !file ) {
		LOG.Warn( "Error opening %s", filename );
		return;
	}

	auto Write = [file, &filename](const void* data, size_t len) -> bool {
		if (len != static_cast<size_t>(trap_FS_Write(data, len, file)))
		{
			LOG.Warn( "Error writing navmesh file %s", filename );
			trap_FS_FCloseFile( file );
			std::error_code err;
			FS::HomePath::DeleteFile( filename, err );
			return false;
		}
		return true;
	};

	if ( !Write( &header, sizeof( header ) ) ) return;

	if ( d_->status.code == NavgenStatus::PERMANENT_FAILURE )
	{
		// Make old gamelogic fail versions with "Null tile in Navmesh" error
		// TODO(0.53): break navmesh header compatibility and clean this up
		NavMeshTileHeader tileHeader = {};
		if ( !Write( &tileHeader, sizeof(tileHeader) ) ) return;
		// rest of file is the error message
		if ( !Write( d_->status.message.data(), d_->status.message.size() ) ) return;
		trap_FS_FCloseFile( file );
		return;
	}
	ASSERT_EQ(d_->status.code, NavgenStatus::OK);

	for ( int i = 0; i < maxTiles; i++ )
	{
		const dtCompressedTile *tile = d_->tileCache->getTile( i );

		if ( !tile || !tile->header || !tile->dataSize ) {
			continue;
		}

		NavMeshTileHeader tileHeader;
		tileHeader.tileRef = d_->tileCache->getTileRef( tile );
		tileHeader.dataSize = tile->dataSize;

		SwapNavMeshTileHeader( tileHeader );
		if ( !Write( &tileHeader, sizeof( tileHeader ) ) ) return;

		std::unique_ptr<unsigned char[]> data( new unsigned char[tile->dataSize] );

		memcpy( data.get(), tile->data, tile->dataSize );
		if ( LittleLong( 1 ) != 1 ) {
			dtTileCacheHeaderSwapEndian( data.get(), tile->dataSize );
		}

		if ( !Write( data.get(), tile->dataSize ) ) return;
	}
	trap_FS_FCloseFile( file );
}

void NavmeshGenerator::LoadBSP()
{
	// copied from beginning of CM_LoadMap
	std::string mapFile = "maps/" + mapName_ + ".bsp";
	mapData_ = FS::PakPath::ReadFile(mapFile);
	dheader_t* header = reinterpret_cast<dheader_t*>(&mapData_[0]);

	// hacky byte swapping for lumps of interest
	// assume all fields size 4 except the string at the beginning of dshader_t
	for (unsigned i = 0; i < sizeof(dheader_t) / 4; i++)
	{
		((int*)header)[i] = LittleLong(((int*)header)[i]);
	}
	for (int lump: {LUMP_MODELS, LUMP_BRUSHES, LUMP_SHADERS, LUMP_BRUSHSIDES, LUMP_PLANES})
	{
		unsigned nBytes = header->lumps[lump].fileofs;
		char *base = &mapData_[0] + header->lumps[lump].filelen;
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
	std::swap( recastVert[1], recastVert[2] ); // convert Quake to Recast coordinates
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
void NavmeshGenerator::LoadBrushTris( std::vector<float> &verts, std::vector<int> &tris ) {
	int j;

	int solidFlags = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	int surfaceSkip = 0;

	if ( config_.excludeSky ) {
		surfaceSkip = SURF_SKY;
	}

	const byte* const cmod_base = reinterpret_cast<const byte*>(mapData_.data());
	auto& header = *reinterpret_cast<const dheader_t*>(mapData_.data());

	// Each one of these lumps should be byte-swapped in LoadBSP
	const lump_t& modelsLump = header.lumps[LUMP_MODELS];
	const lump_t& brushesLump = header.lumps[LUMP_BRUSHES];
	const lump_t& shadersLump = header.lumps[LUMP_SHADERS];
	const lump_t& sidesLump = header.lumps[LUMP_BRUSHSIDES];
	const lump_t& planesLump = header.lumps[LUMP_PLANES];

	/* get model, index 0 is worldspawn entity */
	if ( modelsLump.filelen / sizeof(dmodel_t) <= 0 ) {
		initStatus_ = { NavgenStatus::PERMANENT_FAILURE, "No brushes found" };
		return;
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

			if ( config_.excludeCaulk && !Q_stricmp( shader->shader, "textures/common/caulk" ) ) {
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

void NavmeshGenerator::LoadGeometry()
{
	std::vector<float> verts;
	std::vector<int> tris;

	LOG.Debug( "loading geometry..." );
	int numVerts, numTris;

	//count surfaces
	LoadBrushTris( verts, tris );
	if ( initStatus_.code != NavgenStatus::OK ) return;
	LoadPatchTris( verts, tris );

	numTris = tris.size() / 3;
	numVerts = verts.size() / 3;

	LOG.Debug( "Using %d triangles", numTris );
	LOG.Debug( "Using %d vertices", numVerts );

	geo_.init( &verts[ 0 ], numVerts, &tris[ 0 ], numTris );

	const float *mins = geo_.getMins();
	const float *maxs = geo_.getMaxs();

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

// Most Recast error returns here are translated as transient failures because inspection of the source shows
// that the only failure mode is insufficient memory
static NavgenStatus rasterizeTileLayers( Geometry& geo, rcContext &context, int tx, int ty, const rcConfig &mcfg,
                                         TileCacheData *data, int maxLayers, bool filterGaps, int *ntiles )
{
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
		return { NavgenStatus::TRANSIENT_FAILURE, "Failed to create heightfield for navigation mesh" };
	}

	//I understand that using std::vector prevents NiH's proudness.
	const float *verts = geo.getVerts();
	const int nverts = geo.getNumVerts();
	const rcChunkyTriMesh *chunkyMesh = geo.getChunkyMesh();

	rc.triareas = new unsigned char[ chunkyMesh->maxTrisPerChunk ];

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
	if ( !ncid ) { // TODO is this supposed to mean failure?
		delete[] cid;
		*ntiles = 0;
		return {};
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
		return { NavgenStatus::TRANSIENT_FAILURE, "Failed to create compact heightfield for navigation mesh" };
	}

	if ( !rcErodeWalkableAreaByBox( &context, cfg.walkableRadius, *rc.chf ) ) {
		return { NavgenStatus::TRANSIENT_FAILURE, "Unable to erode walkable surfaces" };
	}

	rc.lset = rcAllocHeightfieldLayerSet();

	if ( !rc.lset ) {
		return { NavgenStatus::TRANSIENT_FAILURE, "Out of memory heightfield layer set" };
	}

	if ( !rcBuildHeightfieldLayers( &context, *rc.chf, cfg.borderSize, cfg.walkableHeight, *rc.lset ) ) {
		// This could be an out-of-memory error or a real algorithm failure
		return { NavgenStatus::PERMANENT_FAILURE, "Could not build heightfield layers" };
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
			return { CodeForFailedDtStatus( status ), "dtBuildTileCacheLayer failed" };
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

	*ntiles = n;
	return {};
}

void NavmeshGenerator::StartGeneration( class_t species )
{
	LOG.Notice( "Generating navmesh for %s", BG_Class( species )->name );
	d_.reset( new PerClassData );
	d_->species = species;
	d_->status = initStatus_;
	if ( d_->status.code != NavgenStatus::OK )
	{
		return;
	}

	const float *bmin = geo_.getMins();
	const float *bmax = geo_.getMaxs();
	int gw = 0, gh = 0;

	const classModelConfig_t* dimensions = BG_ClassModelConfig(species);
	//radius of agent (BBox maxs[0] or BBox maxs[1])
	float radius = dimensions->maxs[0];
	//height of agent (BBox maxs[2] - BBox mins[2])
	float height = dimensions->maxs[2] - dimensions->mins[2];

	const float cellSize = radius / 4.0f;

	rcCalcGridSize( bmin, bmax, cellSize, &gw, &gh );

	const int ts = tileSize;
	d_->tw = ( gw + ts - 1 ) / ts;
	d_->th = ( gh + ts - 1 ) / ts;

	d_->cfg.cs = cellSize;
	d_->cfg.ch = config_.cellHeight;
	d_->cfg.walkableSlopeAngle = RAD2DEG( acosf( MIN_WALK_NORMAL ) );
	d_->cfg.walkableHeight = ( int ) ceilf( height / d_->cfg.ch );
	d_->cfg.walkableClimb = ( int ) floorf( config_.stepSize / d_->cfg.ch );
	d_->cfg.walkableRadius = ( int ) ceilf( radius / d_->cfg.cs );
	d_->cfg.maxEdgeLen = 0;
	d_->cfg.maxSimplificationError = 1.3f;
	d_->cfg.minRegionArea = rcSqr( 25 );
	d_->cfg.mergeRegionArea = rcSqr( 50 );
	d_->cfg.maxVertsPerPoly = 6;
	d_->cfg.tileSize = ts;
	d_->cfg.borderSize = d_->cfg.walkableRadius * 2;
	d_->cfg.width = d_->cfg.tileSize + d_->cfg.borderSize * 2;
	d_->cfg.height = d_->cfg.tileSize + d_->cfg.borderSize * 2;
	d_->cfg.detailSampleDist = d_->cfg.cs * 6.0f;
	d_->cfg.detailSampleMaxError = d_->cfg.ch * 1.0f;

	rcVcopy( d_->cfg.bmin, bmin );
	rcVcopy( d_->cfg.bmax, bmax );

	rcVcopy( d_->tcparams.orig, bmin );
	d_->tcparams.cs = cellSize;
	d_->tcparams.ch = config_.cellHeight;
	d_->tcparams.width = ts;
	d_->tcparams.height = ts;
	d_->tcparams.walkableHeight = height;
	d_->tcparams.walkableRadius = radius;
	d_->tcparams.walkableClimb = config_.stepSize;
	d_->tcparams.maxSimplificationError = 1.3;
	d_->tcparams.maxTiles = d_->tw * d_->th * EXPECTED_LAYERS_PER_TILE;
	d_->tcparams.maxObstacles = 256;

	d_->tileCache.reset(new dtTileCache);
	dtStatus status = d_->tileCache->init( &d_->tcparams, &d_->alloc, &d_->comp, &d_->proc );

	if ( dtStatusFailed( status ) ) {
		std::string message = dtStatusDetail( status, DT_INVALID_PARAM ) ? "Could not init tile cache: Invalid parameter" : "Could not init tile cache";
		d_->status = { CodeForFailedDtStatus( status ), message };
	}
}

bool NavmeshGenerator::Step()
{
	if ( d_->status.code != NavgenStatus::OK || d_->y >= d_->th || d_->tw == 0 )
	{
		if ( d_->status.code == NavgenStatus::OK )
		{
			LOG.Verbose( "Finished generating navmesh for %s", BG_ClassModelConfig( d_->species )->humanName );
		}
		else
		{
			LOG.Warn( "Navmesh generation for %s failed: %s", BG_ClassModelConfig( d_->species )->humanName, d_->status.message );
		}
		WriteFile();
		d_.reset();
		return true;
	}

	TileCacheData tiles[ MAX_LAYERS ];
	memset( tiles, 0, sizeof( tiles ) );

	int ntiles;
	NavgenStatus status = rasterizeTileLayers(geo_, recastContext_, d_->x, d_->y, d_->cfg, tiles, MAX_LAYERS, !!config_.filterGaps, &ntiles);
	if ( status.code != NavgenStatus::OK )
	{
		d_->status = status;
		return false;
	}

	for ( int i = 0; i < ntiles; i++ )
	{
		TileCacheData *tile = &tiles[ i ];
		dtStatus status = d_->tileCache->addTile( tile->data, tile->dataSize, DT_COMPRESSEDTILE_FREE_DATA, 0 );
		if ( dtStatusFailed( status ) ) {
			dtFree( tile->data );
			tile->data = 0;
			continue;
		}
	}

	//iterate over all tiles (number is determined by rcCalcGridSize)
	if ( ++d_->x == d_->tw )
	{
		d_->x = 0;
		++d_->y;
	}
	return false;
}

void NavmeshGenerator::Init(Str::StringRef mapName)
{
	if (mapName == mapName_) return;

	// TODO: some way of setting the config (former daemonmap flags)?
	config_.cellHeight = 2.0f;
	config_.stepSize = STEPSIZE;
	config_.excludeCaulk = 1;
	config_.excludeSky = 1;
	config_.filterGaps = 1;

	mapName_ = mapName;
	recastContext_.enableLog(true);
	initStatus_ = {};
	LoadBSP();
	LoadGeometry();
	if ( initStatus_.code != NavgenStatus::OK ) return;

	float height = rcAbs( geo_.getMaxs()[1] ) + rcAbs( geo_.getMins()[1] );
	if ( height / config_.cellHeight > RC_SPAN_MAX_HEIGHT ) {
		LOG.Warn("Map geometry is too tall for specified cell height. Increasing cell height to compensate. This may cause a less accurate navmesh." );
		float prevCellHeight = config_.cellHeight;
		float minCellHeight = height / RC_SPAN_MAX_HEIGHT;

		int divisor = ( int ) config_.stepSize;

		while ( divisor && config_.cellHeight < minCellHeight )
		{
			config_.cellHeight = config_.stepSize / divisor;
			divisor--;
		}

		if ( !divisor ) {
			initStatus_ = { NavgenStatus::PERMANENT_FAILURE, "Map is too tall to generate a navigation mesh" };
			return;
		}

		LOG.Verbose( "Previous cell height: %f", prevCellHeight );
		LOG.Verbose( "New cell height: %f", config_.cellHeight );
	}
}

float NavmeshGenerator::SpeciesFractionCompleted() const
{
	// Assume that writing the file at the end is 10%
	return 0.9f * float(d_->y * d_->tw + d_->x) / float(d_->tw * d_->th);
}
