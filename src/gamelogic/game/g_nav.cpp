/*
===========================================================================
This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "g_bot.h"
#include "../../libs/detour/DetourNavMeshBuilder.h"
#include "../../libs/detour/DetourNavMeshQuery.h"
#include "../../libs/detour/DetourPathCorridor.h"
#include "../../libs/detour/DetourCommon.h"
#include "../../engine/botlib/nav.h"

dtPathCorridor pathCorridor[MAX_CLIENTS];
dtNavMeshQuery* navQuerys[PCL_NUM_CLASSES];
dtQueryFilter navFilters[PCL_NUM_CLASSES];
dtNavMesh *navMeshes[PCL_NUM_CLASSES];

//tells if all navmeshes loaded successfully
qboolean navMeshLoaded = qfalse;

/*
========================
Navigation Mesh Loading
========================
*/
qboolean BotLoadNavMesh( const char *filename, dtNavMesh **navMesh )
{
	char mapname[ MAX_QPATH ];
	char filePath[ MAX_QPATH ];
	char gameName[ MAX_STRING_CHARS ];
	fileHandle_t f = 0;
	dtNavMesh *mesh = NULL;

	trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );
	trap_Cvar_VariableStringBuffer( "fs_game", gameName, sizeof( gameName ) );
	Com_sprintf( filePath, sizeof( filePath ), "maps/%s-%s.navMesh", mapname, filename );
	Com_Printf( " loading navigation mesh file '%s'...\n", filePath );

	int len = trap_FS_FOpenFile( filePath, &f, FS_READ );

	if ( !f )
	{
		Com_Printf( S_COLOR_RED "ERROR: Cannot open Navigaton Mesh file\n" );
		return qfalse;
	}

	if ( len < 0 )
	{
		Com_Printf( S_COLOR_RED "ERROR: Negative Length for Navigation Mesh file\n" );
		return qfalse;
	}

	NavMeshSetHeader header;
	
	trap_FS_Read( &header, sizeof( header ), f );

	qboolean swapEndian = qfalse;

	if ( header.magic != NAVMESHSET_MAGIC )
	{
		swapEndian = qtrue;
		int i;
		for ( i = 0; i < sizeof( header ) / sizeof( int ); i ++ )
		{
			dtSwapEndian( ( ( int * ) &header ) + i );
		}
	}

	if ( header.version != NAVMESHSET_VERSION )
	{
		Com_Printf( S_COLOR_RED "ERROR: File is wrong version found: %d want: %d\n", header.version, NAVMESHSET_VERSION );
		trap_FS_FCloseFile( f );
		return qfalse;
	}

	mesh = dtAllocNavMesh();

	if ( !mesh )
	{
		Com_Printf( S_COLOR_RED "ERROR: Unable to allocate nav mesh\n" );
		trap_FS_FCloseFile( f );
		return qfalse;
	}

	*navMesh = mesh;
	dtStatus status = mesh->init( &header.params );

	if ( dtStatusFailed( status ) )
	{
		Com_Printf( S_COLOR_RED "ERROR: Could not init navmesh\n" );
		dtFreeNavMesh( *navMesh );
		*navMesh = 0;
		trap_FS_FCloseFile( f );
		return qfalse;
	}

	for ( int i = 0; i < header.numTiles; i++ )
	{
		NavMeshTileHeader tileHeader;

		trap_FS_Read( &tileHeader, sizeof( tileHeader ), f );

		if ( swapEndian )
		{
			dtSwapEndian( &tileHeader.dataSize );
			dtSwapEndian( &tileHeader.tileRef );
		}

		if ( !tileHeader.tileRef || !tileHeader.dataSize )
		{
			Com_Printf( S_COLOR_RED "ERROR: NUll Tile in navmesh\n" );
			dtFreeNavMesh( *navMesh );
			*navMesh = 0;
			trap_FS_FCloseFile( f );
			return qfalse;
		}

		unsigned char *data = ( unsigned char * ) dtAlloc( tileHeader.dataSize, DT_ALLOC_PERM );

		if ( !data )
		{
			Com_Printf( S_COLOR_RED "ERROR: Failed to allocate memory for tile data\n" );
			dtFreeNavMesh( *navMesh );
			*navMesh = 0;
			trap_FS_FCloseFile( f );
			return qfalse;
		}

		memset( data, 0, tileHeader.dataSize );

		trap_FS_Read( data, tileHeader.dataSize, f );

		if ( swapEndian )
		{
			dtNavMeshHeaderSwapEndian( data, tileHeader.dataSize );
			dtNavMeshDataSwapEndian( data, tileHeader.dataSize );
		}

		dtStatus status = mesh->addTile( data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0 );

		if ( dtStatusFailed( status ) )
		{
			Com_Printf( S_COLOR_RED "ERROR: Failed to add tile to navmesh\n" );
			dtFree( data );
			dtFreeNavMesh( *navMesh );
			*navMesh = 0;
			trap_FS_FCloseFile( f );
			return qfalse;
		}
	}

	trap_FS_FCloseFile( f );
	return qtrue;
}

void G_BotNavInit()
{

	Com_Printf( "==== Bot Navigation Initialization ==== \n" );

	memset( navMeshes, 0, sizeof( *navMeshes ) );
	memset( navQuerys, 0, sizeof( *navQuerys ) );

	for ( int i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		if ( !BotLoadNavMesh( BG_Class( ( class_t ) i )->name, &navMeshes[ i ] ) )
		{
			G_BotNavCleanup();
			return;
		}

		navQuerys[i] = dtAllocNavMeshQuery();
		if ( !navQuerys[i] )
		{
			G_BotNavCleanup();
			Com_Printf( "Could not allocate Detour Navigation Mesh Query for class %s\n", BG_Class( ( class_t )i )->name );
			return;
		}

		if ( dtStatusFailed( navQuerys[i]->init( navMeshes[i], 65536 ) ) )
		{
			G_BotNavCleanup();
			Com_Printf( "Could not init Detour Navigation Mesh Query for class %s", BG_Class( ( class_t )i )->name );
			return;
		}

		navFilters[i].setIncludeFlags( POLYFLAGS_WALK );
		navFilters[i].setExcludeFlags( POLYFLAGS_DISABLED );
	}
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( !pathCorridor[i].init( MAX_PATH_POLYS ) )
		{
			G_BotNavCleanup();
			Com_Printf( "Could not init the path corridor\n" );
			return;
		}
	}
	navMeshLoaded = qtrue;
}

void G_BotNavCleanup( void )
{
	for ( int i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		if ( navQuerys[i] )
		{
			dtFreeNavMeshQuery( navQuerys[i] );
			navQuerys[i] = NULL;
		}
		if ( navMeshes[i] )
		{
			dtFreeNavMesh( navMeshes[i] );
			navMeshes[i] = NULL;
		}
	}
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		pathCorridor[i].~dtPathCorridor();
		pathCorridor[i] = dtPathCorridor();
	}
	navMeshLoaded = qfalse;
}

void BotSetPolyFlags( vec3_t origin, vec3_t mins, vec3_t maxs, unsigned short flags )
{
	vec3_t extents;
	vec3_t realMin, realMax, center;

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

	if ( navMeshLoaded )
	{
		for ( int i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
		{
			const int maxPolys = 20;
			int polyCount = 0;
			dtPolyRef polys[ maxPolys ];

			dtNavMeshQuery *query = navQuerys[ i ];
			dtNavMesh *mesh = navMeshes[ i ];

			query->queryPolygons( center, extents, &filter, polys, &polyCount, maxPolys );

			for ( int i = 0; i < polyCount; i++ )
			{
				mesh->setPolyFlags( polys[ i ], flags );
			}
		}
	}
}

extern "C" void G_BotDisableArea( vec3_t origin, vec3_t mins, vec3_t maxs )
{
	BotSetPolyFlags( origin, mins, maxs, POLYFLAGS_DISABLED );
}

extern "C" void G_BotEnableArea( vec3_t origin, vec3_t mins, vec3_t maxs )
{
	BotSetPolyFlags( origin, mins, maxs, POLYFLAGS_WALK );
}

void BotGetAgentExtents( gentity_t *ent, vec3_t extents )
{
	VectorSet( extents, ent->r.maxs[0] * 5,  2 * ( ent->r.maxs[2] - ent->r.mins[2] ), ent->r.maxs[1] * 5 );
}

extern "C" void BotSetNavmesh( gentity_t  *self, class_t newClass )
{
	if ( newClass == PCL_NONE )
	{
		return;
	}

	self->botMind->navQuery = navQuerys[ newClass ];
	self->botMind->navFilter = &navFilters[ newClass ];
	self->botMind->pathCorridor = &pathCorridor[ self->s.number ];
	self->botMind->needPathReset = qtrue;
}

qboolean BotFindNearestPoly( gentity_t *self, gentity_t *ent, dtPolyRef *nearestPoly, vec3_t nearPoint );
extern "C" void BotCheckCorridor( gentity_t *self )
{
	vec3_t point;
	dtPolyRef ref;
	if ( self->botMind->needPathReset && BotFindNearestPoly( self, self, &ref, point ) )
	{
		quake2recast( point );
		self->botMind->pathCorridor->reset( ref, point );

		if ( self->botMind->goal.inuse )
		{
			if ( !( FindRouteToTarget( self, self->botMind->goal ) & ( ROUTE_FAILURE | ROUTE_PARTIAL ) ) )
			{
				self->botMind->needPathReset = qfalse;
			}
		}
		else
		{
			self->botMind->needPathReset = qfalse;
		}
	}
}

/*
========================
Bot Navigation Querys
========================
*/

int DistanceToGoal( gentity_t *self )
{
	vec3_t targetPos;
	vec3_t selfPos;
	//safety check for morons who use this incorrectly
	if ( !( self->botMind ) )
	{
		return -1;
	}
	BotGetTargetPos( self->botMind->goal, targetPos );
	VectorCopy( self->s.origin, selfPos );
	return Distance( selfPos, targetPos );
}

int DistanceToGoalSquared( gentity_t *self )
{
	vec3_t targetPos;
	vec3_t selfPos;
	//safety check for morons who use this incorrectly
	if ( !( self->botMind ) )
	{
		return -1;
	}
	BotGetTargetPos( self->botMind->goal, targetPos );
	VectorCopy( self->s.origin, selfPos );
	return DistanceSquared( selfPos, targetPos );
}

qboolean BotNav_Trace( dtNavMeshQuery* navQuery, dtQueryFilter* navFilter, vec3_t start, vec3_t end, float *hit, vec3_t normal, dtPolyRef *pathPolys, int *numHit, int maxPolies )
{
	vec3_t nearPoint;
	dtPolyRef startRef;
	dtStatus status;
	vec3_t extents = {75, 96, 75};
	quake2recast( start );
	quake2recast( end );
	status = navQuery->findNearestPoly( start, extents, navFilter, &startRef, nearPoint );
	if ( dtStatusFailed( status ) || startRef == 0 )
	{
		//try larger extents
		extents[1] += 500;
		status = navQuery->findNearestPoly( start, extents, navFilter, &startRef, nearPoint );
		if ( dtStatusFailed( status ) || startRef == 0 )
		{
			if ( numHit )
			{
				*numHit = 0;
				*hit = 0;
			}
			VectorSet( normal, 0, 0, 0 );
			if ( maxPolies > 0 )
			{
				pathPolys[0] = startRef;

				if ( numHit )
				{
					*numHit = 1;
				}
			}
			return qfalse;
		}
	}
	status = navQuery->raycast( startRef, start, end, navFilter, hit, normal, pathPolys, numHit, maxPolies );
	if ( dtStatusFailed( status ) )
	{
		return qfalse;
	}
	else
	{
		return qtrue;
	}
}
extern "C" qboolean BotPathIsWalkable( gentity_t *self, botTarget_t target )
{
	dtPolyRef pathPolys[MAX_PATH_POLYS];
	vec3_t selfPos, targetPos;
	float hit;
	int numPolys;
	vec3_t hitNormal;
	vec3_t viewNormal;
	BG_GetClientNormal( &self->client->ps, viewNormal );
	VectorMA( self->s.origin, self->r.mins[2], viewNormal, selfPos );
	BotGetTargetPos( target, targetPos );
	//quake2recast(selfPos);
	//quake2recast(targetPos);

	if ( !BotNav_Trace( self->botMind->navQuery, self->botMind->navFilter, selfPos, targetPos, &hit, hitNormal, pathPolys, &numPolys, MAX_PATH_POLYS ) )
	{
		return qfalse;
	}

	if ( hit == FLT_MAX )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

extern "C" void BotFindRandomPointOnMesh( gentity_t *self, vec3_t point )
{
	int numTiles = 0;
	const dtNavMesh *navMesh = self->botMind->navQuery->getAttachedNavMesh();
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
	float *v = &tile->verts[vertStart];
	VectorCopy( v, targetPos );
	recast2quake( targetPos );

	VectorCopy( targetPos, point );
}

qboolean BotFindNearestPoly( gentity_t *self, gentity_t *ent, dtPolyRef *nearestPoly, vec3_t nearPoint )
{
	vec3_t extents;
	vec3_t start;
	vec3_t viewNormal;
	dtStatus status;
	dtNavMeshQuery* navQuery;
	dtQueryFilter* navFilter;
	if ( ent->client )
	{
		BG_GetClientNormal( &ent->client->ps, viewNormal );
		navQuery = self->botMind->navQuery;
		navFilter = self->botMind->navFilter;
	}
	else
	{
		VectorSet( viewNormal, 0, 0, 1 );
		navQuery = self->botMind->navQuery;
		navFilter = self->botMind->navFilter;
	}
	VectorMA( ent->s.origin, ent->r.mins[2], viewNormal, start );
	quake2recast( start );
	BotGetAgentExtents( ent, extents );
	status = navQuery->findNearestPoly( start, extents, navFilter, nearestPoly, nearPoint );
	if ( dtStatusFailed( status ) || *nearestPoly == 0 )
	{
		//try larger extents
		extents[1] += 900;
		status = navQuery->findNearestPoly( start, extents, navFilter, nearestPoly, nearPoint );
		if ( dtStatusFailed( status ) || *nearestPoly == 0 )
		{
			return qfalse; // failed
		}
	}
	recast2quake( nearPoint );
	return qtrue;

}
qboolean BotFindNearestPoly( gentity_t *self, vec3_t coord, dtPolyRef *nearestPoly, vec3_t nearPoint )
{
	vec3_t start, extents;
	dtStatus status;
	dtNavMeshQuery* navQuery = self->botMind->navQuery;
	dtQueryFilter* navFilter = self->botMind->navFilter;
	VectorSet( extents, 640, 96, 640 );
	VectorCopy( coord, start );
	quake2recast( start );
	status = navQuery->findNearestPoly( start, extents, navFilter, nearestPoly, nearPoint );
	if ( dtStatusFailed( status ) || *nearestPoly == 0 )
	{
		//try larger extents
		extents[1] += 900;
		status = navQuery->findNearestPoly( start, extents, navFilter, nearestPoly, nearPoint );
		if ( dtStatusFailed( status ) || *nearestPoly == 0 )
		{
			return qfalse; // failed
		}
	}
	recast2quake( nearPoint );
	return qtrue;
}
/*
========================
Local Bot Navigation
========================
*/

void BotStrafeDodge( gentity_t *self )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	if ( self->client->time1000 >= 500 )
	{
		botCmdBuffer->rightmove = 127;
	}
	else
	{
		botCmdBuffer->rightmove = -127;
	}

	if ( ( self->client->time10000 % 2000 ) < 1000 )
	{
		botCmdBuffer->rightmove *= -1;
	}

	if ( ( self->client->time1000 % 300 ) >= 100 && ( self->client->time10000 % 3000 ) > 2000 )
	{
		botCmdBuffer->rightmove = 0;
	}
}

void BotMoveInDir( gentity_t *self, uint32_t dir )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;
	if ( dir & MOVE_FORWARD )
	{
		botCmdBuffer->forwardmove = 127;
	}
	else if ( dir & MOVE_BACKWARD )
	{
		botCmdBuffer->forwardmove = -127;
	}

	if ( dir & MOVE_RIGHT )
	{
		botCmdBuffer->rightmove = 127;
	}
	else if ( dir & MOVE_LEFT )
	{
		botCmdBuffer->rightmove = -127;
	}
}

void BotAlternateStrafe( gentity_t *self )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	if ( level.time % 8000 < 4000 )
	{
		botCmdBuffer->rightmove = 127;
	}
	else
	{
		botCmdBuffer->rightmove = -127;
	}
}

void BotStandStill( gentity_t *self )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	botCmdBuffer->forwardmove = 0;
	botCmdBuffer->rightmove = 0;
	botCmdBuffer->upmove = 0;
}

qboolean BotJump( gentity_t *self )
{
	if ( self->client->ps.stats[STAT_TEAM] == TEAM_HUMANS && self->client->ps.stats[STAT_STAMINA] < STAMINA_SLOW_LEVEL + STAMINA_JUMP_TAKE )
	{
		return qfalse;
	}

	self->botMind->cmdBuffer.upmove = 127;
	return qtrue;
}

qboolean BotSprint( gentity_t *self, qboolean enable )
{

	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	if ( !enable )
	{
		usercmdReleaseButton( botCmdBuffer->buttons, BUTTON_SPRINT );
		return qfalse;
	}

	if ( self->client->ps.stats[STAT_TEAM] == TEAM_HUMANS && self->client->ps.stats[STAT_STAMINA] > STAMINA_SLOW_LEVEL + STAMINA_JUMP_TAKE && self->botMind->botSkill.level >= 5 )
	{
		usercmdPressButton( botCmdBuffer->buttons, BUTTON_SPRINT );
		usercmdReleaseButton( botCmdBuffer->buttons, BUTTON_WALKING );
		return qtrue;
	}
	else
	{
		usercmdReleaseButton( botCmdBuffer->buttons, BUTTON_SPRINT );
		return qfalse;
	}
}


qboolean BotDodge( gentity_t *self )
{
	vec3_t backward, right, left;
	vec3_t end;
	float fracback, fracleft, fracright;
	float jumpMag;
	vec3_t normal;

	//see: bg_pmove.c, these conditions prevent use from using dodge
	if ( self->client->ps.stats[STAT_TEAM] != TEAM_HUMANS )
	{
		usercmdReleaseButton( self->botMind->cmdBuffer.buttons, BUTTON_DODGE );
		return qfalse;
	}

	if ( self->client->ps.pm_type != PM_NORMAL || self->client->ps.stats[STAT_STAMINA] > STAMINA_SLOW_LEVEL + STAMINA_DODGE_TAKE ||
	        ( self->client->ps.pm_flags & PMF_DUCKED ) )
	{
		usercmdReleaseButton( self->botMind->cmdBuffer.buttons, BUTTON_DODGE );
		return qfalse;
	}

	if ( !( self->client->ps.pm_flags & ( PMF_TIME_LAND | PMF_CHARGE ) ) && self->client->ps.groundEntityNum != ENTITYNUM_NONE )
	{
		usercmdReleaseButton( self->botMind->cmdBuffer.buttons, BUTTON_DODGE );
		return qfalse;
	}

	//skill level required to use dodge
	if ( self->botMind->botSkill.level < 7 )
	{
		usercmdReleaseButton( self->botMind->cmdBuffer.buttons, BUTTON_DODGE );
		return qfalse;
	}

	//find the best direction to dodge in
	AngleVectors( self->client->ps.viewangles, backward, right, NULL );
	VectorInverse( backward );
	backward[2] = 0;
	VectorNormalize( backward );
	right[2] = 0;
	VectorNormalize( right );
	VectorNegate( right, left );

	jumpMag = BG_Class( ( class_t ) self->client->ps.stats[STAT_CLASS] )->jumpMagnitude;

	//test each direction for navigation mesh collisions
	//FIXME: this code does not guarentee that we will land safely and on the navigation mesh!
	VectorMA( self->s.origin, jumpMag, backward, end );
	BotNav_Trace( self->botMind->navQuery, self->botMind->navFilter, self->s.origin, end, &fracback, normal, NULL, NULL, 0 );
	VectorMA( self->s.origin, jumpMag, right, end );
	BotNav_Trace( self->botMind->navQuery, self->botMind->navFilter, self->s.origin, end, &fracright, normal, NULL, NULL, 0 );
	VectorMA( self->s.origin, jumpMag, left, end );
	BotNav_Trace( self->botMind->navQuery, self->botMind->navFilter, self->s.origin, end, &fracleft, normal, NULL, NULL, 0 );

	//set the direction to dodge
	BotStandStill( self );

	if ( fracback > fracleft && fracback > fracright )
	{
		self->botMind->cmdBuffer.forwardmove = -127;
	}
	else if ( fracleft > fracright && fracleft > fracback )
	{
		self->botMind->cmdBuffer.rightmove = -127;
	}
	else
	{
		self->botMind->cmdBuffer.rightmove = 127;
	}

	// dodge
	usercmdPressButton( self->botMind->cmdBuffer.buttons, BUTTON_DODGE );
	return qtrue;
}

gentity_t* BotGetPathBlocker( gentity_t *self )
{
	vec3_t playerMins, playerMaxs;
	vec3_t end;
	vec3_t forward;
	trace_t trace;
	vec3_t moveDir;
	const int TRACE_LENGTH = BOT_OBSTACLE_AVOID_RANGE;

	if ( !( self && self->client ) )
	{
		return NULL;
	}

	//get the forward vector, ignoring pitch
	forward[0] = cos( DEG2RAD( self->client->ps.viewangles[YAW] ) );
	forward[1] = sin( DEG2RAD( self->client->ps.viewangles[YAW] ) );
	forward[2] = 0;
	//VectorSubtract(target,self->s.origin, moveDir);
	//moveDir[2] = 0;
	//VectorNormalize(moveDir);
	//already normalized
	VectorCopy( forward, moveDir );

	BG_ClassBoundingBox( ( class_t ) self->client->ps.stats[STAT_CLASS], playerMins, playerMaxs, NULL, NULL, NULL );

	//account for how large we can step
	playerMins[2] += STEPSIZE;
	playerMaxs[2] += STEPSIZE;

	VectorMA( self->s.origin, TRACE_LENGTH, moveDir, end );

	trap_Trace( &trace, self->s.origin, playerMins, playerMaxs, end, self->s.number, CONTENTS_BODY );
	if ( trace.entityNum != ENTITYNUM_NONE && trace.fraction < 1.0f )
	{
		return &g_entities[trace.entityNum];
	}
	else
	{
		return NULL;
	}
}

qboolean BotShouldJump( gentity_t *self, gentity_t *blocker )
{
	vec3_t playerMins;
	vec3_t playerMaxs;
	vec3_t moveDir;
	float jumpMagnitude;
	trace_t trace;
	const int TRACE_LENGTH = BOT_OBSTACLE_AVOID_RANGE;
	vec3_t forward;
	vec3_t end;

	//blocker is not on our team, so ignore
	if ( BotGetEntityTeam( self ) != BotGetEntityTeam( blocker ) )
	{
		return qfalse;
	}

	VectorSubtract( self->botMind->route[0], self->s.origin, forward );
	//get the forward vector, ignoring z axis
	/*forward[0] = cos(DEG2RAD(self->client->ps.viewangles[YAW]));
	forward[1] = sin(DEG2RAD(self->client->ps.viewangles[YAW]));
	forward[2] = 0;*/
	forward[2] = 0;
	VectorNormalize( forward );

	//already normalized
	VectorCopy( forward, moveDir );
	BG_ClassBoundingBox( ( class_t ) self->client->ps.stats[STAT_CLASS], playerMins, playerMaxs, NULL, NULL, NULL );

	playerMins[2] += STEPSIZE;
	playerMaxs[2] += STEPSIZE;


	//trap_Print(vtos(self->movedir));
	VectorMA( self->s.origin, TRACE_LENGTH, moveDir, end );

	//make sure we are moving into a block
	trap_Trace( &trace, self->s.origin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT );
	if ( trace.fraction >= 1.0f || blocker != &g_entities[trace.entityNum] )
	{
		return qfalse;
	}

	jumpMagnitude = BG_Class( ( class_t )self->client->ps.stats[STAT_CLASS] )->jumpMagnitude;

	//find the actual height of our jump
	jumpMagnitude = Square( jumpMagnitude ) / ( self->client->ps.gravity * 2 );

	//prepare for trace
	playerMins[2] += jumpMagnitude;
	playerMaxs[2] += jumpMagnitude;

	//check if jumping will clear us of entity
	trap_Trace( &trace, self->s.origin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT );

	//if we can jump over it, then jump
	//note that we also test for a blocking barricade because barricades will collapse to let us through
	if ( blocker->s.modelindex == BA_A_BARRICADE || trace.fraction == 1.0f )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

qboolean BotFindSteerTarget( gentity_t *self, vec3_t aimPos )
{
	vec3_t forward;
	vec3_t testPoint1, testPoint2;
	vec3_t playerMins, playerMaxs;
	float yaw1, yaw2;
	trace_t trace1, trace2;

	if ( !( self && self->client ) )
	{
		return qfalse;
	}

	//get bbox
	BG_ClassBoundingBox( ( class_t ) self->client->ps.stats[STAT_CLASS], playerMins, playerMaxs, NULL, NULL, NULL );

	//account for stepsize
	playerMins[2] += STEPSIZE;
	playerMaxs[2] += STEPSIZE;

	//get the yaw (left/right) we dont care about up/down
	yaw1 = self->client->ps.viewangles[YAW];
	yaw2 = self->client->ps.viewangles[YAW];

	//directly infront of us is blocked, so dont test it
	yaw1 -= 15;
	yaw2 += 15;

	//forward vector is 2D
	forward[2] = 0;

	//find an unobstructed position
	//we check the full 180 degrees in front of us
	for ( int i = 0; i < 5; i++, yaw1 -= 15 , yaw2 += 15 )
	{
		//compute forward for right
		forward[0] = cos( DEG2RAD( yaw1 ) );
		forward[1] = sin( DEG2RAD( yaw1 ) );
		//forward is already normalized
		//try the right
		VectorMA( self->s.origin, BOT_OBSTACLE_AVOID_RANGE, forward, testPoint1 );

		//test it
		trap_Trace( &trace1, self->s.origin, playerMins, playerMaxs, testPoint1, self->s.number, MASK_SHOT );

		//check if unobstructed
		if ( trace1.fraction >= 1.0f )
		{
			VectorCopy( testPoint1, aimPos );
			aimPos[2] += self->client->ps.viewheight;
			return qtrue;
		}

		//compute forward for left
		forward[0] = cos( DEG2RAD( yaw2 ) );
		forward[1] = sin( DEG2RAD( yaw2 ) );
		//forward is already normalized
		//try the left
		VectorMA( self->s.origin, BOT_OBSTACLE_AVOID_RANGE, forward, testPoint2 );

		//test it
		trap_Trace( &trace2, self->s.origin, playerMins, playerMaxs, testPoint2, self->s.number, MASK_SHOT );

		//check if unobstructed
		if ( trace2.fraction >= 1.0f )
		{
			VectorCopy( testPoint2, aimPos );
			aimPos[2] += self->client->ps.viewheight;
			return qtrue;
		}
	}

	//we couldnt find a new position
	return qfalse;
}
qboolean BotAvoidObstacles( gentity_t *self, vec3_t rVec )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	if ( !( self && self->client ) )
	{
		return qfalse;
	}
	if ( BotGetEntityTeam( self ) == TEAM_NONE )
	{
		return qfalse;
	}

	gentity_t *blocker = BotGetPathBlocker( self );
	if ( blocker )
	{
		vec3_t newAimPos;
		if ( BotShouldJump( self, blocker ) )
		{
			BotJump( self );

			return qfalse;
		}
		else if ( BotFindSteerTarget( self, newAimPos ) )
		{
			//found a steer target
			VectorCopy( newAimPos, rVec );
			return qfalse;
		}
		else
		{
			BotAlternateStrafe( self );
			BotMoveInDir( self, MOVE_BACKWARD );

			return qtrue;
		}
	}
	return qfalse;
}

//copy of PM_CheckLadder in bg_pmove.c
qboolean BotOnLadder( gentity_t *self )
{
	vec3_t forward, end;
	vec3_t mins, maxs;
	trace_t trace;

	if ( !BG_ClassHasAbility( ( class_t ) self->client->ps.stats[ STAT_CLASS ], SCA_CANUSELADDERS ) )
	{
		return qfalse;
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );

	forward[ 2 ] = 0.0f;
	BG_ClassBoundingBox( ( class_t ) self->client->ps.stats[ STAT_CLASS ], mins, maxs, NULL, NULL, NULL );
	VectorMA( self->s.origin, 1.0f, forward, end );

	trap_Trace( &trace, self->s.origin, mins, maxs, end, self->s.number, MASK_PLAYERSOLID );

	if ( trace.fraction < 1.0f && trace.surfaceFlags & SURF_LADDER )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

void BotDirectionToUsercmd( gentity_t *self, vec3_t dir, usercmd_t *cmd )
{
	vec3_t forward;
	vec3_t right;

	float forwardmove;
	float rightmove;
	AngleVectors( self->client->ps.viewangles, forward, right, NULL );
	forward[2] = 0;
	VectorNormalize( forward );
	right[2] = 0;
	VectorNormalize( right );

	// get direction and non-optimal magnitude
	forwardmove = 127 * DotProduct( forward, dir );
	rightmove = 127 * DotProduct( right, dir );

	// find optimal magnitude to make speed as high as possible
	if ( fabsf( forwardmove ) > fabsf( rightmove ) )
	{
		float highestforward = forwardmove < 0 ? -127 : 127;

		float highestright = highestforward * rightmove / forwardmove;

		cmd->forwardmove = ClampChar( highestforward );
		cmd->rightmove = ClampChar( highestright );
	}
	else
	{
		float highestright = rightmove < 0 ? -127 : 127;

		float highestforward = highestright * forwardmove / rightmove;

		cmd->forwardmove = ClampChar( highestforward );
		cmd->rightmove = ClampChar( highestright );
	}
}

void BotSeek( gentity_t *self, vec3_t seekPos )
{
	vec3_t direction;
	vec3_t viewOrigin;

	BG_GetClientViewOrigin( &self->client->ps, viewOrigin );
	VectorSubtract( seekPos, viewOrigin, direction );

	VectorNormalize( direction );

	// move directly toward the target
	BotDirectionToUsercmd( self, direction, &self->botMind->cmdBuffer );

	// slowly change our aim to point to the target
	BotSlowAim( self, seekPos, 0.6 );
	BotAimAtLocation( self, seekPos );
}

/**BotGoto
* Used to make the bot travel between waypoints or to the target from the last waypoint
* Also can be used to make the bot travel other short distances
*/
void BotGoto( gentity_t *self, botTarget_t target )
{

	vec3_t tmpVec;

	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	BotGetIdealAimLocation( self, target, tmpVec );

	if ( !BotAvoidObstacles( self, tmpVec ) )
	{
		BotSeek( self, tmpVec );
	}
	else
	{
		BotSlowAim( self, tmpVec, 0.6 );
		BotAimAtLocation( self, tmpVec );
	}

	//dont sprint or dodge if we dont have enough stamina and are about to slow
	if ( self->client->ps.stats[ STAT_TEAM ] == TEAM_HUMANS && self->client->ps.stats[STAT_STAMINA] < STAMINA_SLOW_LEVEL + STAMINA_JUMP_TAKE )
	{
		usercmdReleaseButton( botCmdBuffer->buttons, BUTTON_SPRINT );
		usercmdReleaseButton( botCmdBuffer->buttons, BUTTON_DODGE );
	}
}

/*
=========================
Global Bot Navigation
=========================
*/

void FindWaypoints( gentity_t *self )
{
	float corners[MAX_ROUTE_NODES * 3];
	unsigned char cornerFlags[MAX_ROUTE_NODES];
	dtPolyRef cornerPolys[MAX_ROUTE_NODES];

	if ( !self->botMind->pathCorridor->getPathCount() )
	{
		BotDPrintf( S_COLOR_RED "ERROR: FindWaypoints Called without a path!\n" );
		self->botMind->numCorners = 0;
		return;
	}

	//trap_Print("finding Corners\n");
	int numCorners = self->botMind->pathCorridor->findCorners( corners, cornerFlags, cornerPolys, MAX_ROUTE_NODES, self->botMind->navQuery, self->botMind->navFilter );
	//trap_Print("found Corners\n");
	//copy the points to the route array, converting each point into quake coordinates too
	for ( int i = 0; i < numCorners; i++ )
	{
		float *vert = &corners[i * 3];
		recast2quake( vert );
		VectorCopy( vert, self->botMind->route[i] );
	}

	self->botMind->numCorners = numCorners;
}

qboolean PointInPoly( gentity_t *self, dtPolyRef ref, vec3_t point )
{
	const dtPolyRef curRef = ref;
	const dtMeshTile* curTile = 0;
	const dtPoly* curPoly = 0;
	float verts[DT_VERTS_PER_POLYGON*3];

	if ( dtStatusFailed( self->botMind->navQuery->getAttachedNavMesh()->getTileAndPolyByRef(curRef, &curTile, &curPoly) ) )
	{
		return qfalse;
	}
		
	const int nverts = curPoly->vertCount;
	for ( int i = 0; i < nverts; ++i )
	{
		VectorCopy( &curTile->verts[curPoly->verts[i]*3], &verts[i*3] );
	}

	if ( dtPointInPolygon( point, verts, nverts ) )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

void UpdatePathCorridor( gentity_t *self )
{
	vec3_t selfPos, targetPos;
	if ( !self->botMind->pathCorridor->getPathCount() )
	{
		BotDPrintf( S_COLOR_RED "ERROR: UpdatePathCorridor called without a path!\n" );
		return;
	}

	BotGetTargetPos( self->botMind->goal, targetPos );
	VectorCopy( self->s.origin, selfPos );

	quake2recast( selfPos );
	quake2recast( targetPos );

	self->botMind->pathCorridor->movePosition( selfPos, self->botMind->navQuery, self->botMind->navFilter );
	self->botMind->pathCorridor->moveTargetPosition( targetPos, self->botMind->navQuery, self->botMind->navFilter );

	if ( !self->botMind->pathCorridor->isValid( MAX_PATH_LOOK_AHEAD, self->botMind->navQuery, self->botMind->navFilter ) )
	{
		self->botMind->needPathReset = qtrue;
	}

	FindWaypoints( self );
	dtPolyRef check;
	vec3_t pos;
	dtPolyRef firstPoly = self->botMind->pathCorridor->getFirstPoly();
	dtPolyRef lastPoly = self->botMind->pathCorridor->getLastPoly();

	//check for replans caused by inaccurate bot movement or the target going off the path corridor

	if ( !PointInPoly( self, firstPoly, selfPos )  )
	{
		self->botMind->needPathReset = qtrue;
	}
		
	if ( BotTargetIsPlayer( self->botMind->goal ) )
	{
		if ( self->botMind->goal.ent->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{
			if ( !PointInPoly( self, lastPoly, targetPos ) )
			{
				self->botMind->needPathReset = qtrue;
			}
		}
	}
}

qboolean BotMoveToGoal( gentity_t *self )
{
	botTarget_t target;

	if ( !( self && self->client ) )
	{
		return qfalse;
	}

	UpdatePathCorridor( self );

	if ( self->botMind->numCorners > 0 )
	{
		BotSetTarget( &target, NULL, &self->botMind->route[0] );
		BotGoto( self, target );
		return qfalse;
	}
	return qtrue;
}

int FindRouteToTarget( gentity_t *self, botTarget_t target )
{
	vec3_t targetPos;
	vec3_t start;
	vec3_t end;
	dtPolyRef startRef, endRef = 1;
	dtPolyRef pathPolys[MAX_PATH_POLYS];
	dtStatus status;
	int pathNumPolys = 512;
	qboolean result;

	//dont pathfind too much
	if ( level.time - self->botMind->timeFoundRoute < 200 )
	{
		return ROUTE_FAILURE;
	}

	if ( !self->botMind->navQuery )
	{
		BotDPrintf( "Cannot query the Navmesh!\n" );
		return ROUTE_FAILURE;
	}

	self->botMind->timeFoundRoute = level.time;

	if ( BotTargetIsEntity( target ) )
	{
		PlantEntityOnGround( target.ent, targetPos );
	}
	else
	{
		BotGetTargetPos( target, targetPos );
	}

	startRef = self->botMind->pathCorridor->getFirstPoly();
	VectorCopy( self->botMind->pathCorridor->getPos(), start );

	if ( BotTargetIsEntity( target ) )
	{
		result = BotFindNearestPoly( self, target.ent, &endRef, end );
	}
	else
	{
		result = BotFindNearestPoly( self, targetPos, &endRef, end );
	}

	if ( !result )
	{
		BotDPrintf( "Failed to find a polygon near the target\n" );
		return ROUTE_FAILURE | ROUTE_NOPOLYNEARTARGET;
	}

	self->botMind->numCorners = 0;

	quake2recast( end );
	
	status = self->botMind->navQuery->findPath( startRef, endRef, start, end, self->botMind->navFilter, pathPolys, &pathNumPolys, MAX_PATH_POLYS );

	if ( dtStatusFailed( status ) )
	{
		BotDPrintf( "Could not find path\n" );
		return ROUTE_FAILURE;
	}

	self->botMind->pathCorridor->reset( startRef, start );
	self->botMind->pathCorridor->setCorridor( end, pathPolys, pathNumPolys );

	FindWaypoints( self );
	if ( status & DT_PARTIAL_RESULT )
	{
		BotDPrintf( "Found a partial path\n" );
		return ROUTE_SUCCESS | ROUTE_PARTIAL;
	}

	return ROUTE_SUCCESS;
}

/*
========================
Misc Bot Nav
========================
*/

void PlantEntityOnGround( gentity_t *ent, vec3_t groundPos )
{
	vec3_t mins, maxs;
	trace_t trace;
	vec3_t realPos;
	const int traceLength = 1000;
	vec3_t endPos;
	vec3_t traceDir;
	if ( ent->client )
	{
		BG_ClassBoundingBox( ( class_t )ent->client->ps.stats[STAT_CLASS], mins, maxs, NULL, NULL, NULL );
	}
	else if ( ent->s.eType == ET_BUILDABLE )
	{
		BG_BuildableBoundingBox( ( buildable_t )ent->s.modelindex, mins, maxs );
	}
	else
	{
		VectorCopy( ent->r.mins, mins );
		VectorCopy( ent->r.maxs, maxs );
	}

	VectorSet( traceDir, 0, 0, -1 );
	VectorCopy( ent->s.origin, realPos );
	VectorMA( realPos, traceLength, traceDir, endPos );
	trap_Trace( &trace, ent->s.origin, mins, maxs, endPos, ent->s.number, CONTENTS_SOLID );
	if ( trace.fraction != 1.0f )
	{
		VectorCopy( trace.endpos, groundPos );
	}
	else
	{
		VectorCopy( realPos, groundPos );
		groundPos[2] += mins[2];

	}
}
