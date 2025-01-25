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

#include <bitset>
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
static const int EXPECTED_LAYERS_PER_TILE = 12;

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
		tiles{},
		ntiles( 0 )
	{}

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
	std::copy_n( v, nv * 3, verts );

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
	std::vector<std::function<void()>> mainThreadTasks_;

public:
	void doLog(rcLogCategory category, const char* msg, int len) override;

	void RunOnMainThread(std::function<void()> f);

	// Do this once at the end so that logs from the same class_t are together
	void DoMainThreadTasks();
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

// for one class_t
struct NavgenTask {
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
	UnvContext context;
};


// There are 3 threading modes of navmesh generation:
// - Main thread does a little bit work once per frame (sgame background  generation with threads disabled)
// - Background threads work on navgen while main thread does other things (sgame background with threads enabled)
// - Main thread blocks while background threads run until navgen completed (/navgen and cgame)
// NavgenPool is used for the 2nd and 3rd cases.

// Public interface to navgen. Rest of this file is internal details
class NavmeshGenerator {
private:
	NavgenConfig config_;
	// Custom computed config for the map
	float cellHeight_;
	// Map data
	std::string mapName_;
	std::string mapData_;
	Geometry geo_;
	NavgenStatus initStatus_;

	std::vector<std::unique_ptr<NavgenTask>> taskQueue_;

	std::atomic<int> fractionCompleteNumerator_{0};
	int fractionCompleteDenominator_;

	// Map geometry loading
	void LoadBSP();
	void LoadGeometry();
	void LoadTris(std::vector<float>& verts, std::vector<int>& tris);
	// in principle mapName could be different from the current map, if the necessary pak is loaded
	void LoadMap(Str::StringRef mapName);

	void WriteFile(const NavgenTask& t);

public:
	~NavmeshGenerator();

	std::unique_ptr<NavgenTask> StartGeneration(class_t species);

	void LoadMapAndEnqueueTasks(Str::StringRef mapName, std::bitset<PCL_NUM_CLASSES> classes);

	// Only intended to be meaningful if no tasks have failed
	float FractionComplete() const;

	/*
	 * Optional multi-threading functionality
	 */
private:
	std::vector<std::thread> threads_;
	int numActiveThreads_;
	std::atomic<bool> canceled_{false};
	std::vector<std::unique_ptr<NavgenTask>> finishedTasks_;

	// guards taskQueue_, finishedTasks_, numActiveThreads_ during multithreading
	std::mutex taskQueueMutex_;

public:
	void StartBackgroundThreads(int numBackgroundThreads);
	void WaitInMainThread(std::function<void(float)> progressCallback);
	bool ThreadsDone();
	bool HandleFinishedTasks();

	/*
	 * METHODS THAT MAY BE CALLED FROM A NON-MAIN THREAD
	 * Must not use any trap calls, that includes logging!!!
	 */
public:
	std::unique_ptr<NavgenTask> PopTask(); // caller must hold mutex if applicable
	// Returns true when finished
	bool Step(NavgenTask& t);
private:
	void BackgroundThreadMain();
};
