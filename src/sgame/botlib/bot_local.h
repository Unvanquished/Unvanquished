/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2013 Daemon Developers
All rights reserved.

This file is part of the Daemon BSD Source Code (Daemon Source Code).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS

===========================================================================
*/

#ifndef BOTLIB_LOCAL_H_
#define BOTLIB_LOCAL_H_

#include "engine/qcommon/q_shared.h"

#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "DetourPathCorridor.h"
#include "DetourCommon.h"
#include "DetourTileCache.h"
#include "DetourTileCacheBuilder.h"

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
