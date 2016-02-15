/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

#ifndef __BOT_LOCAL_H
#define __BOT_LOCAL_H

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"

#include "detour/DetourNavMeshBuilder.h"
#include "detour/DetourNavMeshQuery.h"
#include "detour/DetourPathCorridor.h"
#include "detour/DetourCommon.h"
#include "detour/DetourTileCache.h"
#include "detour/DetourTileCacheBuilder.h"

#include "bot_types.h"
#include "bot_api.h"
#include "nav.h"
#include "bot_convert.h"

const int MAX_NAV_DATA = 16;
const int MAX_BOT_PATH = 512;
const int MAX_PATH_LOOKAHEAD = 5;
const int MAX_CORNERS = 5;
const int MAX_ROUTE_PLANS = 2;
const int MAX_ROUTE_CACHE = 20;
const int ROUTE_CACHE_TIME = 200;

struct dtRouteResult
{
	dtPolyRef startRef;
	dtPolyRef endRef;
	int       time;
	dtStatus  status;
	bool      invalid;
};

struct NavData_t
{
	dtTileCache      *cache;
	dtNavMesh        *mesh;
	dtNavMeshQuery   *query;
	dtQueryFilter    filter;
	MeshProcess      process;
	char             name[ 64 ];
};

struct Bot_t
{
	NavData_t         *nav;
	dtPathCorridor    corridor;
	int               clientNum;
	bool              needReplan;
	float             cornerVerts[ MAX_CORNERS * 3 ];
	unsigned char     cornerFlags[ MAX_CORNERS ];
	dtPolyRef         cornerPolys[ MAX_CORNERS ];
	int               numCorners;
	bool              offMesh;
	rVec              offMeshStart;
	rVec              offMeshEnd;
	dtPolyRef         offMeshPoly;
	dtRouteResult     routeResults[ MAX_ROUTE_CACHE ];
};

extern int numNavData;
extern NavData_t BotNavData[ MAX_NAV_DATA ];
extern Bot_t agents[ MAX_CLIENTS ];

void NavEditInit();
void NavEditShutdown();
void BotSaveOffMeshConnections( NavData_t *nav );

void         BotCalcSteerDir( Bot_t *bot, rVec &dir );
void         FindWaypoints( Bot_t *bot, float *corners, unsigned char *cornerFlags, dtPolyRef *cornerPolys, int *numCorners, int maxCorners );
bool         PointInPolyExtents( Bot_t *bot, dtPolyRef ref, rVec point, rVec extents );
bool         PointInPoly( Bot_t *bot, dtPolyRef ref, rVec point );
bool         BotFindNearestPoly( Bot_t *bot, rVec coord, dtPolyRef *nearestPoly, rVec &nearPoint );
bool         FindRoute( Bot_t *bot, rVec s, botRouteTargetInternal target, bool allowPartial );
#endif
