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
#include <thread>
#include <mutex>
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

struct ladder_t
{
	glm::vec3 bottom, up;
	float radius;
};

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

struct NavgenStatus
{
	enum Code {
		OK,
		TRANSIENT_FAILURE, // e.g. memory allocation failure
		PERMANENT_FAILURE,
		NOT_COMPLETE,
	};
	Code code;
	std::string message;

	bool ok() const { return code == Code::OK; }
	bool IsIncomplete() const { return code == Code::NOT_COMPLETE; }

	std::string String() const { return Str::Format("Code %d: %s", code, message); }
};

// Public interface to navgen. Rest of this file is internal details
class NavmeshGenerator : public rcContext {
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

		std::unique_ptr<std::thread> thread;

		bool IsDone() const {
			return y >= th;
		}

		float Progress() const {
			if (tw == 0 || th == 0) return 0.0f;
			// Assume that writing the file at the end is 10%
			return 0.9f * float(y * tw + x) / float(tw * th);
		}
	};

	NavgenConfig config_;
	// Custom computed config for the map
	float cellHeight_;
	// Map data
	std::string mapName_;
	std::string mapData_;
	Geometry geo_;

	std::mutex mu_;
	std::unordered_map<class_t, NavgenStatus> statuses_;
	std::unordered_map<class_t, PerClassData> tasks_;

	std::mutex trap_mu_;
	std::vector<std::function<void()>> funcs_;

	void doLog(const rcLogCategory /*category*/, const char* msg, const int /*len*/) override;

	void LoadBSP();
	NavgenStatus LoadGeometry();
	NavgenStatus LoadTris(std::vector<float>& verts, std::vector<int>& tris);

	NavgenStatus InitPerClassData(class_t species, PerClassData *data);
	NavgenStatus Step(PerClassData *data);
	NavgenStatus WriteFile(const PerClassData& data);
	NavgenStatus WriteError(const PerClassData& data, const NavgenStatus& status);

	void UpdateStatus(class_t species, NavgenStatus status);

	void Generate(class_t species);

	void AsyncRunOnMainThread(std::function<void()> func);


public:
	virtual ~NavmeshGenerator();
	// load the BSP if it has not been loaded already
	// in principle mapName could be different from the current map, if the necessary pak is loaded
	NavgenStatus Init(Str::StringRef mapName);

	void RunTasks();

	void StartGeneration(class_t species);

	float Progress();

	// Returns true when finished
};
