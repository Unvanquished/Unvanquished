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

void BotSetPolyFlags( const vec3_t origin, const vec3_t mins, const vec3_t maxs, unsigned short flags )
{
	vec3_t extents;
	vec3_t realMin, realMax, center;

	if ( !numNavData )
	{
		return;
	}

	// support abs min/max by recalculating the origin and min/max
	VectorAdd( mins, origin, realMin );
	VectorAdd( maxs, origin, realMax );
	VectorAdd( realMin, realMax, center );
	VectorScale( center, 0.5, center );
	VectorSubtract( realMax, center, realMax );
	VectorSubtract( realMin, center, realMin );

	// find extents
	for ( int j = 0; j < 3; j++ )
	{
		extents[ j ] = MAX( fabsf( realMin[ j ] ), fabsf( realMax[ j ] ) );
	}

	// convert to recast coordinates
	quake2recast( center );
	quake2recast( extents );

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

extern "C" unsigned int BotFindRouteExt( int botClientNum, const botRouteTarget_t *target )
{
	vec3_t start;
	botRouteTarget_t rtarget;
	Bot_t *bot = &agents[ botClientNum ];
	rtarget = *target;

	VectorCopy( SV_GentityNum( botClientNum )->s.origin, start );
	quake2recast( start );
	quake2recastTarget( &rtarget );
	return FindRoute( bot, start, &rtarget );
}

extern "C" qboolean BotUpdateCorridor( int botClientNum, const botRouteTarget_t *target, vec3_t dir, qboolean *directPathToGoal )
{
	vec3_t spos;
	vec3_t epos;
	Bot_t *bot = &agents[ botClientNum ];
	botRouteTarget_t rtarget;

	sharedEntity_t * ent = SV_GentityNum( botClientNum );

	VectorCopy( ent->s.origin, spos );
	quake2recast( spos );

	rtarget = *target;
	quake2recastTarget( &rtarget );

	VectorCopy( rtarget.pos, epos );

	if ( directPathToGoal )
	{
		*directPathToGoal = qfalse;
	}

	if ( bot->needReplan )
	{
		if ( ! ( FindRoute( bot, spos, &rtarget ) & ( ROUTE_PARTIAL | ROUTE_FAILED ) ) )
		{
			bot->needReplan = qfalse;
		}
		else if ( !bot->corridor.getPathCount() )
		{
			return qfalse;
		}
	}

	bot->corridor.movePosition( spos, bot->nav->query, &bot->nav->filter );

	if ( rtarget.type == BOT_TARGET_DYNAMIC )
	{
		bot->corridor.moveTargetPosition( epos, bot->nav->query, &bot->nav->filter );
	}

	if ( !bot->corridor.isValid( MAX_PATH_LOOKAHEAD, bot->nav->query, &bot->nav->filter ) )
	{
		bot->corridor.trimInvalidPath( bot->corridor.getFirstPoly(), spos, bot->nav->query, &bot->nav->filter );
		bot->needReplan = qtrue;
	}

	FindWaypoints( bot, bot->cornerVerts, bot->cornerFlags, bot->cornerPolys, &bot->numCorners, MAX_CORNERS );

	dtPolyRef firstPoly = bot->corridor.getFirstPoly();
	dtPolyRef lastPoly = bot->corridor.getLastPoly();

	//check for replans caused by inaccurate bot movement or the target going off the path corridor
	if ( !PointInPoly( bot, firstPoly, spos )  )
	{
		bot->needReplan = qtrue;
	}

	if ( rtarget.type == BOT_TARGET_DYNAMIC )
	{
		if ( !PointInPolyExtents( bot, lastPoly, epos, rtarget.polyExtents ) )
		{
			bot->needReplan = qtrue;
		}
	}

	if ( dir )
	{
		vec3_t rdir;
		BotCalcSteerDir( bot, rdir );
		recast2quake( rdir );
		VectorCopy( rdir, dir );
	}

	if ( directPathToGoal )
	{
		*directPathToGoal = ( qboolean )( ( int ) bot->numCorners == 1 );
	}

	return qtrue;
}

float frand()
{
	return ( float ) rand() / RAND_MAX;
}

extern "C" void BotFindRandomPoint( int botClientNum, vec3_t point )
{
	int numTiles = 0;
	vec3_t randPoint;
	dtPolyRef randRef;
	Bot_t *bot = &agents[ botClientNum ];
	bot->nav->query->findRandomPoint( &bot->nav->filter, frand, &randRef, randPoint );
	
	VectorCopy( randPoint, point );
	recast2quake( point );
}

extern "C" qboolean BotFindRandomPointInRadius( int botClientNum, const vec3_t origin, vec3_t point, float radius )
{
	vec3_t rorigin;
	dtPolyRef nearPoly;
	VectorCopy( origin, rorigin );
	quake2recast( rorigin );
	VectorSet( point, 0, 0, 0 );

	Bot_t *bot = &agents[ botClientNum ];

	if ( !BotFindNearestPoly( bot, rorigin, &nearPoly, point ) )
	{
		return qfalse;
	}

	dtPolyRef randRef;
	dtStatus status = bot->nav->query->findRandomPointAroundCircle( nearPoly, rorigin, radius, &bot->nav->filter, frand, &randRef, point );
	
	if ( dtStatusFailed( status ) )
	{
		return qfalse;
	}

	recast2quake( point );
	return qtrue;
}

extern "C" qboolean BotNavTrace( int botClientNum, botTrace_t *trace, const vec3_t start, const vec3_t end )
{
	vec3_t nearPoint;
	dtPolyRef startRef;
	dtStatus status;
	vec3_t extents = {75, 96, 75};
	vec3_t spos, epos;
	VectorCopy( start, spos );
	VectorCopy( end, epos );
	quake2recast( spos );
	quake2recast( epos );
	memset( trace, 0, sizeof( *trace ) );

	Bot_t *bot = &agents[ botClientNum ];

	status = bot->nav->query->findNearestPoly( spos, extents, &bot->nav->filter, &startRef, nearPoint );
	if ( dtStatusFailed( status ) || startRef == 0 )
	{
		//try larger extents
		extents[1] += 500;
		status = bot->nav->query->findNearestPoly( spos, extents, &bot->nav->filter, &startRef, nearPoint );
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
	else
	{
		return qtrue;
	}
}

extern "C" void BotAddObstacle( const vec3_t mins, const vec3_t maxs, qhandle_t *obstacleHandle )
{
	vec3_t bmin, bmax;
	VectorCopy( mins, bmin );
	VectorCopy( maxs, bmax );

	quake2recastExtents( bmin, bmax );

	for ( int i = 0; i < numNavData; i++ )
	{
		dtObstacleRef ref;
		NavData_t *nav = &BotNavData[ i ];

		vec3_t realBmin, realBmax;

		VectorCopy( bmin, realBmin );
		VectorCopy( bmax, realBmax );

		const dtTileCacheParams *params = nav->cache->getParams();
		float offset = params->walkableRadius;

		// offset bbox by agent radius like the navigation mesh was originally made
		realBmin[ 0 ] -= offset;
		realBmin[ 2 ] -= offset;

		realBmax[ 0 ] += offset;
		realBmax[ 2 ] += offset;
		
		// offset mins down by agent height so obstacles placed on ledges are handled correctly
		realBmin[ 1 ] -= params->walkableHeight;

		nav->cache->addObstacle( realBmin, realBmax, &ref );
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