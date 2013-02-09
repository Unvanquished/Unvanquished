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

#include "../recast/Recast.h"
#include "../recast/RecastAlloc.h"
#include "../recast/RecastAssert.h"
#include "../recast/ChunkyTriMesh.h"
#include "../detour/DetourCommon.h"
#include "../detour/DetourNavMesh.h"
#include "../detour/DetourNavMeshBuilder.h"
#include "../detour/DetourTileCache.h"
#include "../detour/DetourTileCacheBuilder.h"
#include "../../engine/botlib/nav.h"

static const int MAX_LAYERS = 32;
static const int EXPECTED_LAYERS_PER_TILE = 4;

struct TileCacheData
{
	unsigned char* data;
	int dataSize;
};

struct RasterizationContext
{
	rcHeightfield* solid;
	unsigned char* triareas;
	rcHeightfieldLayerSet* lset;
	rcCompactHeightfield* chf;
	TileCacheData tiles[MAX_LAYERS];
	int ntiles;

	RasterizationContext() :
		solid(0),
		triareas(0),
		lset(0),
		chf(0),
		ntiles(0)
	{
		memset( tiles, 0, sizeof(TileCacheData)*MAX_LAYERS );
	}

	~RasterizationContext()
	{
		rcFreeHeightField(solid);
		delete[] triareas;
		rcFreeHeightfieldLayerSet( lset );
		rcFreeCompactHeightfield( chf );
		for ( int i = 0; i < MAX_LAYERS; ++i )
		{
			dtFree(tiles[i].data);
			tiles[i].data = 0;
		}
	}
};

class Geometry
{
private:
	float            mins[ 3 ];
	float            maxs[ 3 ];
	float           *verts;
	int              nverts;
	rcChunkyTriMesh  mesh;

public:
	Geometry() : verts(0), nverts(0) {}
	~Geometry() { delete[] verts; }

	void init( const float *v, int nv, const int *tris, int ntris )
	{
		verts = new float[ nv * 3 ];
		memcpy( verts, v, sizeof( float ) * nv * 3 );

		nverts = nv;

		rcCreateChunkyTriMesh( verts, tris, ntris, 1024, &mesh );

		rcCalcBounds( verts, nverts, mins, maxs );
	}

	const float           *getMins(){ return mins; }
	const float           *getMaxs() { return maxs; }
	const float           *getVerts() { return verts; }
	const int              getNumVerts() { return nverts; }
	const rcChunkyTriMesh *getChunkyMesh() { return &mesh; }
};

static void quake2recast( float vec[ 3 ] )
{
	float temp = vec[ 1 ];
	vec[ 0 ] = -vec[ 0 ];
	vec[ 1 ] = vec[ 2 ];
	vec[ 2 ] = -temp;
}
