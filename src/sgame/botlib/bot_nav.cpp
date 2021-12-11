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

// TODO: move these functions here
bool BotFindRandomPointInRadius( int botClientNum, const vec3_t origin, vec3_t point, float radius );

Bot_t agents[ MAX_CLIENTS ];

/*
====================
bot_nav.cpp

All vectors used as inputs and outputs to functions here use the engine's coordinate system
====================
*/

void BotSetPolyFlags( qVec origin, qVec mins, qVec maxs, unsigned short flags )
{
	qVec qExtents;
	qVec realMin, realMax;
	qVec qCenter;

	if ( !numNavData )
	{
		return;
	}

	// support abs min/max by recalculating the origin and min/max
	VectorAdd( mins, origin, realMin );
	VectorAdd( maxs, origin, realMax );
	VectorAdd( realMin, realMax, qCenter );
	VectorScale( qCenter, 0.5, qCenter );
	VectorSubtract( realMax, qCenter, realMax );
	VectorSubtract( realMin, qCenter, realMin );

	// find extents
	for ( int j = 0; j < 3; j++ )
	{
		qExtents[ j ] = std::max( fabsf( realMin[ j ] ), fabsf( realMax[ j ] ) );
	}

	// convert to recast coordinates
	rVec center = qCenter;
	rVec extents = qExtents;

	// quake2recast conversion makes some components negative
	extents[ 0 ] = fabsf( extents[ 0 ] );
	extents[ 1 ] = fabsf( extents[ 1 ] );
	extents[ 2 ] = fabsf( extents[ 2 ] );

	// setup a filter so our queries include disabled polygons
	dtQueryFilter filter;
	filter.setIncludeFlags( POLYFLAGS_WALK | POLYFLAGS_DISABLED );
	filter.setExcludeFlags( 0 );

	for ( int i = 0; i < numNavData; i++ )
	{
		const int maxPolys = 20;
		int polyCount = 0;
		dtPolyRef polys[ maxPolys ];

		dtNavMeshQuery *query = BotNavData[ i ].query;
		dtNavMesh *mesh = BotNavData[ i ].mesh;

		query->queryPolygons( center, extents, &filter, polys, &polyCount, maxPolys );

		for ( int i = 0; i < polyCount; i++ )
		{
			mesh->setPolyFlags( polys[ i ], flags );
		}
	}
}

void G_BotDisableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs )
{
	BotSetPolyFlags( origin, mins, maxs, POLYFLAGS_DISABLED );
}

void G_BotEnableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs )
{
	BotSetPolyFlags( origin, mins, maxs, POLYFLAGS_WALK );
}

void G_BotSetNavMesh( int botClientNum, qhandle_t nav )
{
	if ( nav < 0 || nav >= numNavData )
	{
		Log::Warn( "Navigation handle out of bounds" );
		return;
	}

	Bot_t *bot = &agents[ botClientNum ];

	bot->nav = &BotNavData[ nav ];
	bot->needReplan = true;
}

void GetEntPosition( int num, rVec &pos )
{
	pos = qVec( g_entities[ num ].s.origin );
}

void GetEntPosition( int num, qVec &pos )
{
	pos = g_entities[ num ].s.origin;
}

bool G_BotFindRoute( int botClientNum, const botRouteTarget_t *target, bool allowPartial )
{
	rVec start;
	Bot_t *bot = &agents[ botClientNum ];

	GetEntPosition( botClientNum, start );
	return FindRoute( bot, start, *target, allowPartial );
}

static bool withinRadiusOfOffMeshConnection( const Bot_t *bot, rVec pos, rVec off, dtPolyRef conPoly )
{
	const dtOffMeshConnection *con = bot->nav->mesh->getOffMeshConnectionByRef( conPoly );
	if ( !con )
	{
		return false;
	}

	if ( dtVdist2DSqr( pos, off ) < con->rad * con->rad )
	{
		return true;
	}

	return false;
}

static bool overOffMeshConnectionStart( const Bot_t *bot, rVec pos )
{
	if ( bot->numCorners < 1 )
	{
		return false;
	}
	int corner = bot->numCorners - 1;
	const bool offMeshConnection = ( bot->cornerFlags[ corner ] & DT_STRAIGHTPATH_OFFMESH_CONNECTION ) ? true : false;

	if ( offMeshConnection )
	{
		return withinRadiusOfOffMeshConnection( bot, pos, &bot->cornerVerts[ corner * 3 ], bot->cornerPolys[ corner ] );
	}
	return false;
}

void G_UpdatePathCorridor( Bot_t *bot, rVec spos, botRouteTargetInternal target )
{
	bot->corridor.movePosition( spos, bot->nav->query, &bot->nav->filter );

	if ( target.type == botRouteTargetType_t::BOT_TARGET_DYNAMIC )
	{
		bot->corridor.moveTargetPosition( target.pos, bot->nav->query, &bot->nav->filter );
	}

	if ( !bot->corridor.isValid( MAX_PATH_LOOKAHEAD, bot->nav->query, &bot->nav->filter ) )
	{
		bot->corridor.trimInvalidPath( bot->corridor.getFirstPoly(), spos, bot->nav->query, &bot->nav->filter );
		bot->needReplan = true;
	}

	FindWaypoints( bot, bot->cornerVerts, bot->cornerFlags, bot->cornerPolys, &bot->numCorners, MAX_CORNERS );
}

void G_BotUpdatePath( int botClientNum, const botRouteTarget_t *target, botNavCmd_t *cmd )
{
	rVec spos;
	rVec epos;
	Bot_t *bot = &agents[ botClientNum ];
	botRouteTargetInternal rtarget;

	if ( !cmd || !target )
	{
		return;
	}

	GetEntPosition( botClientNum, spos );

	rtarget = *target;
	epos = rtarget.pos;

	G_UpdatePathCorridor( bot, spos, rtarget );

	if ( !bot->offMesh )
	{
		if ( bot->needReplan )
		{
			if ( FindRoute( bot, spos, rtarget, false ) )
			{
				bot->needReplan = false;
			}
		}

		cmd->havePath = !bot->needReplan;

		if ( overOffMeshConnectionStart( bot, spos ) )
		{
			dtPolyRef refs[ 2 ];
			rVec start;
			rVec end;
			// numCorners is guaranteed to be >= 1 here
			dtPolyRef con = bot->cornerPolys[ bot->numCorners - 1 ];

			if ( bot->corridor.moveOverOffmeshConnection( con, refs, start, end, bot->nav->query ) )
			{
				bot->offMesh = true;
				bot->offMeshPoly = con;
				bot->offMeshEnd = end;
				bot->offMeshStart = start;
			}
		}

		dtPolyRef firstPoly = bot->corridor.getFirstPoly();
		dtPolyRef lastPoly = bot->corridor.getLastPoly();

		if ( !PointInPoly( bot, firstPoly, spos ) ||
		   ( rtarget.type == botRouteTargetType_t::BOT_TARGET_DYNAMIC
		     && !PointInPolyExtents( bot, lastPoly, epos, rtarget.polyExtents ) ) )
		{
			bot->needReplan = true;
		}

		rVec rdir;
		BotCalcSteerDir( bot, rdir );

		VectorCopy( rdir, cmd->dir );
		recast2quake( cmd->dir );

		cmd->directPathToGoal = bot->numCorners <= 1;

		VectorCopy( bot->corridor.getPos(), cmd->pos );
		recast2quake( cmd->pos );

		// if there are no corners, we have reached the goal
		// FIXME: this must be done because of a weird bug where the target is not reachable even if 
		// the path was checked for a partial path beforehand
		if ( bot->numCorners == 0 )
		{
			VectorCopy( cmd->pos, cmd->tpos );
		}
		else
		{
			VectorCopy( bot->corridor.getTarget(), cmd->tpos );

			float height;
			if ( dtStatusSucceed( bot->nav->query->getPolyHeight( bot->corridor.getLastPoly(), cmd->tpos, &height ) ) )
			{
				cmd->tpos[ 1 ] = height;
			}
			recast2quake( cmd->tpos );
		}
	}
	
	if ( bot->offMesh )
	{
		qVec pos, proj;
		qVec start = bot->offMeshStart;
		qVec end = bot->offMeshEnd;

		GetEntPosition( botClientNum, pos );
		start[ 2 ] = pos[ 2 ];
		end[ 2 ] = pos[ 2 ];

		ProjectPointOntoVectorBounded( pos, start, end, proj );
		
		VectorCopy( proj, cmd->pos );
		cmd->directPathToGoal = false;
		VectorSubtract( end, pos, cmd->dir );
		VectorNormalize( cmd->dir );

		VectorCopy( bot->corridor.getTarget(), cmd->tpos );
		float height;
		if ( dtStatusSucceed( bot->nav->query->getPolyHeight( bot->corridor.getLastPoly(), cmd->tpos, &height ) ) )
		{
			cmd->tpos[ 1 ] = height;
		}
		recast2quake( cmd->tpos );

		cmd->havePath = true;

		if ( withinRadiusOfOffMeshConnection( bot, spos, bot->offMeshEnd, bot->offMeshPoly ) )
		{
			bot->offMesh = false;
		}
	}
}

float frand()
{
	return ( float ) rand() / ( float ) RAND_MAX;
}

void BotFindRandomPoint( int botClientNum, vec3_t point )
{
	qVec origin = g_entities[ botClientNum ].s.origin;

	if ( !BotFindRandomPointInRadius( botClientNum, origin, point, 2000 ) )
	{
		VectorCopy( origin, point );
	}
}

bool BotFindRandomPointInRadius( int botClientNum, const vec3_t origin, vec3_t point, float radius )
{
	rVec rorigin = qVec( origin );
	rVec nearPoint;
	dtPolyRef nearPoly;

	VectorSet( point, 0, 0, 0 );

	Bot_t *bot = &agents[ botClientNum ];

	if ( !BotFindNearestPoly( bot, rorigin, &nearPoly, nearPoint ) )
	{
		return false;
	}

	dtPolyRef randRef;
	dtStatus status = bot->nav->query->findRandomPointAroundCircle( nearPoly, rorigin, radius, &bot->nav->filter, frand, &randRef, nearPoint );
	
	if ( dtStatusFailed( status ) )
	{
		return false;
	}

	VectorCopy( nearPoint, point );
	recast2quake( point );
	return true;
}

bool G_BotNavTrace( int botClientNum, botTrace_t *trace, const vec3_t start, const vec3_t end )
{
	dtPolyRef startRef;
	dtStatus status;
	rVec extents( 75, 96, 75 );
	rVec spos = qVec( start );
	rVec epos = qVec( end );

	memset( trace, 0, sizeof( *trace ) );

	Bot_t *bot = &agents[ botClientNum ];

	status = bot->nav->query->findNearestPoly( spos, extents, &bot->nav->filter, &startRef, nullptr );
	if ( dtStatusFailed( status ) || startRef == 0 )
	{
		//try larger extents
		extents[ 1 ] += 500;
		status = bot->nav->query->findNearestPoly( spos, extents, &bot->nav->filter, &startRef, nullptr );
		if ( dtStatusFailed( status ) || startRef == 0 )
		{
			return false;
		}
	}

	status = bot->nav->query->raycast( startRef, spos, epos, &bot->nav->filter, &trace->frac, trace->normal, nullptr, nullptr, 0 );
	if ( dtStatusFailed( status ) )
	{
		return false;
	}

	recast2quake( trace->normal );
	return true;
}

void G_BotAddObstacle( const vec3_t mins, const vec3_t maxs, qhandle_t *obstacleHandle )
{
	qVec min = mins;
	qVec max = maxs;
	rBounds box( min, max );

	for ( int i = 0; i < numNavData; i++ )
	{
		dtObstacleRef ref;
		NavData_t *nav = &BotNavData[ i ];

		const dtTileCacheParams *params = nav->cache->getParams();
		float offset = params->walkableRadius;

		rBounds tempBox = box;

		// offset bbox by agent radius like the navigation mesh was originally made
		tempBox.mins[ 0 ] -= offset;
		tempBox.mins[ 2 ] -= offset;

		tempBox.maxs[ 0 ] += offset;
		tempBox.maxs[ 2 ] += offset;
		
		// offset mins down by agent height so obstacles placed on ledges are handled correctly
		tempBox.mins[ 1 ] -= params->walkableHeight;

		nav->cache->addBoxObstacle( tempBox.mins, tempBox.maxs, &ref );
		*obstacleHandle = ref;
	}
}

void G_BotRemoveObstacle( qhandle_t obstacleHandle )
{
	for ( int i = 0; i < numNavData; i++ )
	{
		NavData_t *nav = &BotNavData[ i ];
		if ( nav->cache->getObstacleCount() <= 0 )
		{
			continue;
		}
		nav->cache->removeObstacle( obstacleHandle );
	}
}

void G_BotUpdateObstacles()
{
	for ( int i = 0; i < numNavData; i++ )
	{
		NavData_t *nav = &BotNavData[ i ];
		nav->cache->update( 0, nav->mesh );
	}
}
