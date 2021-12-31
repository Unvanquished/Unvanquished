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

#include <memory>
#include "Recast.h"
#include "RecastAlloc.h"
#include "RecastAssert.h"
#include "ChunkyTriMesh.h"
#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourTileCache.h"
#include "DetourTileCacheBuilder.h"
#include "shared/bot_nav_shared.h"
#include "shared/bg_public.h"

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
		solid( 0 ),
		triareas( 0 ),
		lset( 0 ),
		chf( 0 ),
		ntiles( 0 ){
		memset( tiles, 0, sizeof( TileCacheData ) * MAX_LAYERS );
	}

	~RasterizationContext(){
		rcFreeHeightField( solid );
		delete[] triareas;
		rcFreeHeightfieldLayerSet( lset );
		rcFreeCompactHeightfield( chf );
		for ( int i = 0; i < MAX_LAYERS; ++i )
		{
			dtFree( tiles[i].data );
			tiles[i].data = 0;
		}
	}
};

class Geometry
{
private:
float mins[ 3 ];
float maxs[ 3 ];
float           *verts;
int nverts;
rcChunkyTriMesh mesh;

public:
Geometry() : verts( 0 ), nverts( 0 ) {}
~Geometry() { delete[] verts; }

void init( const float *v, int nv, const int *tris, int ntris ){
	verts = new float[ nv * 3 ];
	memcpy( verts, v, sizeof( float ) * nv * 3 );

	nverts = nv;

	rcCreateChunkyTriMesh( verts, tris, ntris, 1024, &mesh );

	rcCalcBounds( verts, nverts, mins, maxs );
}

const float           *getMins(){ return mins; }
const float           *getMaxs() { return maxs; }
const float           *getVerts() { return verts; }
int                    getNumVerts() { return nverts; }
const rcChunkyTriMesh *getChunkyMesh() { return &mesh; }
};

class UnvContext : public rcContext
{
	void doLog(const rcLogCategory /*category*/, const char* msg, const int /*len*/) override;
};

struct NavgenStatus
{
	enum Code {
		OK,
		TRANSIENT_FAILURE, // e.g. memory allocation failure
		PERMANENT_FAILURE,
	};
	Code code;
	std::string message;
};

// Public interface to navgen. Rest of this file is internal details
class NavmeshGenerator {
private:
	struct PerClassData {
		class_t species;
		rcConfig cfg = {};
		dtTileCacheParams tcparams = {};
		std::unique_ptr<dtTileCache> tileCache;
		LinearAllocator alloc = LinearAllocator(32000);
		FastLZCompressor comp;
		BasicMeshProcess proc;
		int tw;
		int th;
		int x = 0;
		int y = 0;
		NavgenStatus status;
	};

	UnvContext recastContext_;
	NavgenConfig config_;
	// Map data
	std::string mapName_;
	std::string mapData_;
	Geometry geo_;
	NavgenStatus initStatus_;
	// Data for generating current class
	std::unique_ptr<PerClassData> d_;

	void LoadBSP();
	void LoadGeometry();
	void LoadBrushTris(std::vector<float>& verts, std::vector<int>& tris);
	void WriteFile();

public:
	// load the BSP if it has not been loaded already
	// in principle mapName could be different from the current map, if the necessary pak is loaded
	void Init(Str::StringRef mapName);

	void StartGeneration(class_t species);

	float SpeciesFractionCompleted() const;

	// Returns true when finished
	bool Step();
};
