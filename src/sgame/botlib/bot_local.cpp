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

#include "bot_local.h"
#include "sgame/sg_local.h"

/*
====================
bot_local.cpp

All vectors used as inputs and outputs to functions here are assumed to 
use detour/recast's coordinate system

Callers should use recast2quake() and quake2recast() where appropriate to
make sure the vectors use the same coordinate system
====================
*/

void BotCalcSteerDir( Bot_t *bot, rVec &dir )
{
	const int ip0 = 0;
	//FIXME: p1 will result in either -1, 0 or 1. It's very unlikely that -1 is a good idea.
	//old code follow, for reference, because I'm not sure at all that the fix is valid.
	//const int ip1 = std::min( 1, bot->numCorners - 1 );
	const int ip1 = std::min( 1, bot->numCorners );
	const float* p0 = &bot->cornerVerts[ ip0 * 3 ];
	const float* p1 = &bot->cornerVerts[ ip1 * 3 ];
	rVec dir0, dir1;
	float len0, len1;
	rVec spos;

	VectorCopy( bot->corridor.getPos(), spos );

	VectorSubtract( p0, spos, dir0 );
	VectorSubtract( p1, spos, dir1 );
	dir0[ 1 ] = 0;
	dir1[ 1 ] = 0;
	
	len0 = VectorLength( dir0 );
	len1 = VectorLength( dir1 );
	if ( len1 > 0.001f )
	{
		VectorScale(dir1, 1.0f / len1, dir1);
	}
	
	dir[ 0 ] = dir0[ 0 ] - dir1[ 0 ]*len0*0.5f;
	dir[ 1 ] = 0;
	dir[ 2 ] = dir0[ 2 ] - dir1[ 2 ]*len0*0.5f;

	VectorNormalize( dir );
}

void FindWaypoints( Bot_t *bot, float *corners, unsigned char *cornerFlags, dtPolyRef *cornerPolys, int *numCorners, int maxCorners )
{
	if ( !bot->corridor.getPathCount() )
	{
		Log::Warn("FindWaypoints Called without a path!" );
		*numCorners = 0;
		return;
	}

	*numCorners = bot->corridor.findCorners( corners, cornerFlags, cornerPolys, maxCorners, bot->nav->query, &bot->nav->filter );
}

bool PointInPolyExtents( Bot_t *bot, dtPolyRef ref, rVec point, rVec extents )
{
	rVec closest;

	if ( dtStatusFailed( bot->nav->query->closestPointOnPolyBoundary( ref, point, closest ) ) )
	{
		return false;
	}

	// use the bot's bbox as an epsilon because the navmesh is always at least that far from a boundary
	float maxRad = std::max( extents[ 0 ], extents[ 1 ] ) + 1;

	if ( fabsf( point[ 0 ] - closest[ 0 ] ) > maxRad )
	{
		return false;
	}

	if ( fabsf( point[ 2 ] - closest[ 2 ] ) > maxRad )
	{
		return false;
	}

	return true;
}

bool PointInPoly( Bot_t *bot, dtPolyRef ref, rVec point )
{
	gentity_t *ent = g_entities + bot->clientNum;

	return PointInPolyExtents( bot, ref, point, ent->r.maxs );
}

bool BotFindNearestPoly( Bot_t *bot, rVec coord, dtPolyRef *nearestPoly, rVec &nearPoint )
{
	rVec start( coord );
	rVec extents( 640, 96, 640 );
	dtStatus status;
	dtNavMeshQuery* navQuery = bot->nav->query;
	dtQueryFilter* navFilter = &bot->nav->filter;

	status = navQuery->findNearestPoly( start, extents, navFilter, nearestPoly, nearPoint );
	if ( dtStatusFailed( status ) || *nearestPoly == 0 )
	{
		//try larger extents
		extents[ 1 ] += 900;
		status = navQuery->findNearestPoly( start, extents, navFilter, nearestPoly, nearPoint );
		if ( dtStatusFailed( status ) || *nearestPoly == 0 )
		{
			return false; // failed
		}
	}
	return true;
}

static void InvalidateRouteResults( Bot_t *bot )
{
	for ( int i = 0; i < MAX_ROUTE_CACHE; i++ )
	{
		dtRouteResult &res = bot->routeResults[ i ];

		if ( res.invalid )
		{
			continue;
		}

		if ( level.time - res.time > ROUTE_CACHE_TIME )
		{
			res.invalid = true;
			continue;
		}

		if ( !bot->nav->query->isValidPolyRef( res.startRef, &bot->nav->filter ) )
		{
			res.invalid = true;
			continue;
		}

		if ( !bot->nav->query->isValidPolyRef( res.endRef, &bot->nav->filter ) )
		{
			res.invalid = true;
			continue;
		}
	}
}

static dtRouteResult *FindRouteResult( Bot_t *bot, dtPolyRef start )
{
	if ( bot->needReplan )
	{
		return nullptr; // force replan
	}

	for ( int i = 0; i < MAX_ROUTE_CACHE; i++ )
	{
		dtRouteResult &res = bot->routeResults[ i ];

		if ( res.invalid )
		{
			continue;
		}

		if ( res.startRef != start )
		{
			continue;
		}

		if ( res.endRef != start )
		{
			continue;
		}

		return &res;
	}

	return nullptr;
}

static void AddRouteResult( Bot_t *bot, dtPolyRef start, dtPolyRef end, dtStatus status )
{
	// only store route failures or partial results
	if ( !dtStatusFailed( status ) && !dtStatusDetail( status, DT_PARTIAL_RESULT ) )
	{
		return;
	}

	dtRouteResult *bestPos = nullptr;

	for ( int i = 0; i < MAX_ROUTE_CACHE; i++ )
	{
		dtRouteResult &res = bot->routeResults[ i ];

		if ( !bestPos || res.time < bestPos->time || res.invalid )
		{
			bestPos = &res;
		}
	}

	// add result
	bestPos->endRef = end;
	bestPos->startRef = start;
	bestPos->invalid = false;
	bestPos->time = level.time;
	bestPos->status = status;
}

bool FindRoute( Bot_t *bot, rVec s, botRouteTargetInternal rtarget, bool allowPartial )
{
	rVec start;
	rVec end;
	dtPolyRef startRef, endRef = 1;
	dtPolyRef pathPolys[ MAX_BOT_PATH ];
	dtStatus status;
	int pathNumPolys;

	InvalidateRouteResults( bot );

	if ( !BotFindNearestPoly( bot, s, &startRef, start ) )
	{
		return false;
	}

	status = bot->nav->query->findNearestPoly( rtarget.pos, rtarget.polyExtents, 
	                                           &bot->nav->filter, &endRef, end ); 

	if ( dtStatusFailed( status ) || !endRef )
	{
		return false;
	}

	// cache failed results
	dtRouteResult *res = FindRouteResult( bot, startRef );

	if ( res )
	{
		if ( dtStatusFailed( res->status ) )
		{
			return false;
		}

		if ( dtStatusDetail( res->status, DT_PARTIAL_RESULT ) && !allowPartial )
		{
			return false;
		}
	}
	
	status = bot->nav->query->findPath( startRef, endRef, start, end, &bot->nav->filter, pathPolys, &pathNumPolys, MAX_BOT_PATH );

	AddRouteResult( bot, startRef, endRef, status );

	if ( dtStatusFailed( status ) )
	{
		return false;
	}

	if ( dtStatusDetail( status, DT_PARTIAL_RESULT ) && !allowPartial )
	{
		return false;
	}

	bot->corridor.reset( startRef, start );
	bot->corridor.setCorridor( end, pathPolys, pathNumPolys );

	bot->needReplan = false;
	bot->offMesh = false;
	return true;
}
