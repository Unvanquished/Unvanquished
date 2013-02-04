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

extern "C" unsigned int BotFindRouteExt( int botClientNum, const vec3_t target )
{
	vec3_t start;
	vec3_t end;
	Bot_t *bot = &agents[ botClientNum ];
	VectorCopy( target, end );

	VectorCopy( SV_GentityNum( botClientNum )->s.origin, start );
	quake2recast( end );
	quake2recast( start );
	return FindRoute( bot, start, end );
}

extern "C" void BotUpdateCorridor( int botClientNum, vec3_t *corners, int *numCorners, int maxCorners, const vec3_t target )
{
	vec3_t spos;
	vec3_t epos;
	Bot_t *bot = &agents[ botClientNum ];

	sharedEntity_t * ent = SV_GentityNum( botClientNum );

	VectorCopy( ent->s.origin, spos );
	quake2recast( spos );

	VectorCopy( target, epos );
	quake2recast( epos );

	if ( bot->needReplan )
	{
		if ( ! ( FindRoute( bot, spos, epos ) & ( ROUTE_PARTIAL | ROUTE_FAILED ) ) )
		{
			bot->needReplan = qfalse;
		}
		else if ( !bot->corridor.getPathCount() )
		{
			return;
		}
	}

	bot->corridor.movePosition( spos, bot->nav->query, &bot->nav->filter );
	bot->corridor.moveTargetPosition( epos, bot->nav->query, &bot->nav->filter );

	if ( !bot->corridor.isValid( MAX_PATH_LOOKAHEAD, bot->nav->query, &bot->nav->filter ) )
	{
		bot->corridor.trimInvalidPath( bot->corridor.getFirstPoly(), spos, bot->nav->query, &bot->nav->filter );
		bot->needReplan = qtrue;
	}

	unsigned char *cornerFlags = ( unsigned char* ) dtAlloc( sizeof( unsigned char ) * maxCorners * 3, DT_ALLOC_TEMP ); 
	dtPolyRef *cornerPolys = ( dtPolyRef* ) dtAlloc( sizeof( dtPolyRef ) * maxCorners, DT_ALLOC_TEMP );
	FindWaypoints( bot, ( float * ) corners, cornerFlags, cornerPolys, numCorners, maxCorners );
	dtFree( cornerFlags );
	dtFree( cornerPolys );

	// convert points
	for ( int i = 0; i < *numCorners; i++ )
	{
		recast2quake( corners[ i ] );
	}

	dtPolyRef firstPoly = bot->corridor.getFirstPoly();
	dtPolyRef lastPoly = bot->corridor.getLastPoly();

	//check for replans caused by inaccurate bot movement or the target going off the path corridor
	if ( !PointInPoly( bot, firstPoly, spos )  )
	{
		bot->needReplan = qtrue;
	}

	if ( !PointInPoly( bot, lastPoly, epos ) )
	{
		bot->needReplan = qtrue;
	}
}


extern "C" void BotFindRandomPoint( int botClientNum, vec3_t point )
{
	int numTiles = 0;
	Bot_t *bot = &agents[ botClientNum ];
	const dtNavMesh *navMesh = bot->nav->mesh;
	numTiles = navMesh->getMaxTiles();
	const dtMeshTile *tile;
	vec3_t targetPos;

	//pick a random tile
	do
	{
		tile = navMesh->getTile( rand() % numTiles );
	}
	while ( !tile->header->vertCount );

	//pick a random vertex in the tile
	int vertStart = 3 * ( rand() % tile->header->vertCount );

	//convert from recast to quake3
	float *v = &tile->verts[ vertStart ];
	VectorCopy( v, targetPos );
	recast2quake( targetPos );

	VectorCopy( targetPos, point );
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