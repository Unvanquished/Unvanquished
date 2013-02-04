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

#include "bot_local.h"

/*
====================
bot_local.cpp

All vectors used as inputs and outputs to functions here are assumed to 
use detour/recast's coordinate system

Callers should use recast2quake() and quake2recast() where appropriate to
make sure the vectors use the same coordinate system
====================
*/

void FindWaypoints( Bot_t *bot, float *corners, unsigned char *cornerFlags, dtPolyRef *cornerPolys, int *numCorners, int maxCorners )
{
	if ( !bot->corridor.getPathCount() )
	{
		Com_Printf( S_COLOR_RED "ERROR: FindWaypoints Called without a path!\n" );
		*numCorners = 0;
		return;
	}

	*numCorners = bot->corridor.findCorners( corners, cornerFlags, cornerPolys, maxCorners, bot->nav->query, &bot->nav->filter );
}

qboolean PointInPoly( Bot_t *bot, dtPolyRef ref, const vec3_t point )
{
	vec3_t closest;

	if ( dtStatusFailed( bot->nav->query->closestPointOnPolyBoundary( ref, point, closest ) ) )
	{
		return qfalse;
	}

	// use the bot's bbox as an epsilon because the navmesh is always at least that far from a boundry
	sharedEntity_t *ent = SV_GentityNum( bot->clientNum );
	float maxRad = MAX( ent->r.maxs[ 0 ], ent->r.maxs[ 1 ] );

	if ( fabsf( point[ 0 ] - closest[ 0 ] ) > maxRad )
	{
		return qfalse;
	}

	if ( fabsf( point[ 2 ] - closest[ 2 ] ) > maxRad )
	{
		return qfalse;
	}

	return qtrue;
}

qboolean BotFindNearestPoly( Bot_t *bot, const vec3_t coord, dtPolyRef *nearestPoly, vec3_t nearPoint )
{
	vec3_t start, extents;
	dtStatus status;
	dtNavMeshQuery* navQuery = bot->nav->query;
	dtQueryFilter* navFilter = &bot->nav->filter;

	VectorSet( extents, 640, 96, 640 );
	VectorCopy( coord, start );

	status = navQuery->findNearestPoly( start, extents, navFilter, nearestPoly, nearPoint );
	if ( dtStatusFailed( status ) || *nearestPoly == 0 )
	{
		//try larger extents
		extents[ 1 ] += 900;
		status = navQuery->findNearestPoly( start, extents, navFilter, nearestPoly, nearPoint );
		if ( dtStatusFailed( status ) || *nearestPoly == 0 )
		{
			return qfalse; // failed
		}
	}
	return qtrue;
}

unsigned int FindRoute( Bot_t *bot, const vec3_t s, const vec3_t e )
{
	vec3_t start;
	vec3_t end;
	dtPolyRef startRef, endRef = 1;
	dtPolyRef pathPolys[ MAX_BOT_PATH ];
	dtStatus status;
	int pathNumPolys;
	qboolean result;
	int time = Sys_Milliseconds();

	//dont pathfind too much
	if ( time - bot->lastRouteTime < 200 )
	{
		return ROUTE_FAILED;
	}

	bot->lastRouteTime = time;

	result = BotFindNearestPoly( bot, s, &startRef, start );

	if ( !result )
	{
		return ROUTE_FAILED;
	}

	result = BotFindNearestPoly( bot, e, &endRef, end );

	if ( !result )
	{
		return ROUTE_FAILED;
	}
	
	status = bot->nav->query->findPath( startRef, endRef, start, end, &bot->nav->filter, pathPolys, &pathNumPolys, MAX_BOT_PATH );

	if ( dtStatusFailed( status ) )
	{
		return ROUTE_FAILED;
	}

	bot->corridor.reset( startRef, start );
	bot->corridor.setCorridor( end, pathPolys, pathNumPolys );

	if ( status & DT_PARTIAL_RESULT )
	{
		return ROUTE_SUCCEED | ROUTE_PARTIAL;
	}

	bot->needReplan = qfalse;
	return ROUTE_SUCCEED;
}