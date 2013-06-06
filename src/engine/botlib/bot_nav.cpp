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
extern "C"
{
	#include "../server/server.h"
}

#include "bot_local.h"

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
		qExtents[ j ] = MAX( fabsf( realMin[ j ] ), fabsf( realMax[ j ] ) );
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

extern "C" void BotDisableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs )
{
	BotSetPolyFlags( origin, mins, maxs, POLYFLAGS_DISABLED );
}

extern "C" void BotEnableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs )
{
	BotSetPolyFlags( origin, mins, maxs, POLYFLAGS_WALK );
}

extern "C" void BotSetNavMesh( int botClientNum, qhandle_t nav )
{
	if ( nav < 0 || nav >= numNavData )
	{
		Com_Printf( "^3ERROR: Navigation handle out of bounds\n" );
		return;
	}

	Bot_t *bot = &agents[ botClientNum ];

	bot->nav = &BotNavData[ nav ];
	bot->needReplan = qtrue;
}

void GetEntPosition( int num, rVec &pos )
{
	pos = qVec( SV_GentityNum( num )->s.origin );
}

void GetEntPosition( int num, qVec &pos )
{
	pos = SV_GentityNum( num )->s.origin;
}

extern "C" unsigned int BotFindRouteExt( int botClientNum, const botRouteTarget_t *target )
{
	rVec start;
	Bot_t *bot = &agents[ botClientNum ];

	GetEntPosition( botClientNum, start );

	return FindRoute( bot, start, *target );
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
	int corner = bot->numCorners - 1;
	const bool offMeshConnection = ( bot->cornerFlags[ corner ] & DT_STRAIGHTPATH_OFFMESH_CONNECTION ) ? true : false;

	if ( offMeshConnection )
	{
		return withinRadiusOfOffMeshConnection( bot, pos, &bot->cornerVerts[ corner * 3 ], bot->cornerPolys[ corner ] );
	}
	return false;
}

void UpdatePathCorridor( Bot_t *bot, rVec spos, botRouteTargetInternal target )
{
	bot->corridor.movePosition( spos, bot->nav->query, &bot->nav->filter );

	if ( target.type == BOT_TARGET_DYNAMIC )
	{
		bot->corridor.moveTargetPosition( target.pos, bot->nav->query, &bot->nav->filter );
	}

	if ( !bot->corridor.isValid( MAX_PATH_LOOKAHEAD, bot->nav->query, &bot->nav->filter ) )
	{
		bot->corridor.trimInvalidPath( bot->corridor.getFirstPoly(), spos, bot->nav->query, &bot->nav->filter );
		bot->needReplan = qtrue;
	}

	FindWaypoints( bot, bot->cornerVerts, bot->cornerFlags, bot->cornerPolys, &bot->numCorners, MAX_CORNERS );
}

extern "C" void BotUpdateCorridor( int botClientNum, const botRouteTarget_t *target, botNavCmd_t *cmd )
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

	bot->routePlanCounter = 0;

	UpdatePathCorridor( bot, spos, rtarget );

	if ( !bot->offMesh )
	{
		if ( bot->needReplan )
		{
			if ( ! ( FindRoute( bot, spos, rtarget ) & ( ROUTE_PARTIAL | ROUTE_FAILED ) ) )
			{
				bot->needReplan = qfalse;
			}
		}

		if ( overOffMeshConnectionStart( bot, spos ) )
		{
			dtPolyRef refs[ 2 ];
			rVec start;
			rVec end;
			int corner = bot->numCorners - 1;
			dtPolyRef con = bot->cornerPolys[ corner ];

			if ( bot->corridor.moveOverOffmeshConnection( con, refs, start, end, bot->nav->query ) )
			{
				bot->offMesh = true;
				bot->offMeshPoly = con;
				bot->offMeshEnd = end;
				bot->offMeshStart = start;
			}
		}

		dtPolyRef lastPoly = bot->corridor.getLastPoly();

		if ( rtarget.type == BOT_TARGET_DYNAMIC )
		{
			if ( !PointInPolyExtents( bot, lastPoly, epos, rtarget.polyExtents ) )
			{
				bot->needReplan = qtrue;
			}
		}

		rVec rdir;
		BotCalcSteerDir( bot, rdir );

		VectorCopy( rdir, cmd->dir );
		recast2quake( cmd->dir );

		cmd->directPathToGoal = static_cast<qboolean>( bot->numCorners == 1 );

		VectorCopy( bot->corridor.getPos(), cmd->pos );
		recast2quake( cmd->pos );
	}
	
	if ( bot->offMesh )
	{
		qVec pos, proj;
		qVec start = bot->offMeshStart;
		qVec end = bot->offMeshEnd;

		GetEntPosition( botClientNum, pos );

		ProjectPointOntoVectorBounded( pos, start, end, proj );
		
		VectorCopy( proj, cmd->pos );
		cmd->directPathToGoal = qfalse;
		VectorSubtract( end, start, cmd->dir );

		if ( withinRadiusOfOffMeshConnection( bot, spos, bot->offMeshEnd, bot->offMeshPoly ) )
		{
			bot->offMesh = false;
		}
	}
}

float frand()
{
	return ( float ) rand() / RAND_MAX;
}

extern "C" void BotFindRandomPoint( int botClientNum, vec3_t point )
{
	rVec randPoint;
	dtPolyRef randRef;
	Bot_t *bot = &agents[ botClientNum ];
	bot->nav->query->findRandomPoint( &bot->nav->filter, frand, &randRef, randPoint );
	
	VectorCopy( randPoint, point );
	recast2quake( point );
}

extern "C" qboolean BotFindRandomPointInRadius( int botClientNum, const vec3_t origin, vec3_t point, float radius )
{
	rVec rorigin = qVec( origin );
	rVec nearPoint;
	dtPolyRef nearPoly;

	VectorSet( point, 0, 0, 0 );

	Bot_t *bot = &agents[ botClientNum ];

	if ( !BotFindNearestPoly( bot, rorigin, &nearPoly, nearPoint ) )
	{
		return qfalse;
	}

	dtPolyRef randRef;
	dtStatus status = bot->nav->query->findRandomPointAroundCircle( nearPoly, rorigin, radius, &bot->nav->filter, frand, &randRef, nearPoint );
	
	if ( dtStatusFailed( status ) )
	{
		return qfalse;
	}

	VectorCopy( nearPoint, point );
	recast2quake( point );
	return qtrue;
}

extern "C" qboolean BotNavTrace( int botClientNum, botTrace_t *trace, const vec3_t start, const vec3_t end )
{
	dtPolyRef startRef;
	dtStatus status;
	rVec extents( 75, 96, 75 );
	rVec spos = qVec( start );
	rVec epos = qVec( end );

	memset( trace, 0, sizeof( *trace ) );

	Bot_t *bot = &agents[ botClientNum ];

	status = bot->nav->query->findNearestPoly( spos, extents, &bot->nav->filter, &startRef, NULL );
	if ( dtStatusFailed( status ) || startRef == 0 )
	{
		//try larger extents
		extents[ 1 ] += 500;
		status = bot->nav->query->findNearestPoly( spos, extents, &bot->nav->filter, &startRef, NULL );
		if ( dtStatusFailed( status ) || startRef == 0 )
		{
			return qfalse;
		}
	}

	status = bot->nav->query->raycast( startRef, spos, epos, &bot->nav->filter, &trace->frac, trace->normal, NULL, NULL, 0 );
	if ( dtStatusFailed( status ) )
	{
		return qfalse;
	}

	recast2quake( trace->normal );
	return qtrue;
}

extern "C" void BotAddObstacle( const vec3_t mins, const vec3_t maxs, qhandle_t *obstacleHandle )
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

		nav->cache->addObstacle( tempBox.mins, tempBox.maxs, &ref );
		*obstacleHandle = ref;
	}
}

extern "C" void BotRemoveObstacle( qhandle_t obstacleHandle )
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

extern "C" void BotUpdateObstacles()
{
	for ( int i = 0; i < numNavData; i++ )
	{
		NavData_t *nav = &BotNavData[ i ];
		nav->cache->update( 0, nav->mesh );
	}
}