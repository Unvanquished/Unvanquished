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



#include "g_local.h"
#include "g_bot.h"
#include "../../../../engine/botlib/nav.h"
#include "../../../../libs/detour/DetourNavMeshBuilder.h"

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
qboolean G_NavLoad(dtNavMeshCreateParams *navParams, class_t classt) {
	char filename[MAX_QPATH];
	char mapname[MAX_QPATH];
	long len;
	char text[Square(MAX_STRING_CHARS)];
	char *text_p = NULL;
	fileHandle_t f;
	NavMeshHeader_t navHeader;
	char gameName[MAX_STRING_CHARS];

	memset(&navHeader,0,sizeof(NavMeshHeader_t));

	trap_Cvar_VariableStringBuffer("mapname", mapname, sizeof(mapname));
	trap_Cvar_VariableStringBuffer("fs_game", gameName, sizeof(gameName));
	Com_sprintf(filename, sizeof(filename), "maps/%s-%s.navMesh", mapname, BG_Class(classt)->name);
	Com_Printf(" loading navigation mesh file '%s'...", filename);

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if( len < 0 ) {
		Com_Printf(S_COLOR_RED "ERROR: Negative Length for Navigation Mesh file %s\n", filename);
		return qfalse;
	}

	if( len == 0 || len >= sizeof( text ) - 1 )
	{
		trap_FS_FCloseFile( f );
		Com_Printf( S_COLOR_RED "ERROR: Navigation Mesh file %s is %s\n", filename,
			len == 0 ? "empty" : "too long" );
		return qfalse;
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	//header
	navHeader.version = atoi(COM_Parse(&text_p));

	//wrong version!!!
	if(navHeader.version != 1) {
		Com_Printf( S_COLOR_RED "ERROR: The Navigation Mesh is the wrong version!\n");
		return qfalse;
	}

	navHeader.numVerts = atoi(COM_Parse(&text_p));
	navHeader.numPolys = atoi(COM_Parse(&text_p));
	navHeader.numVertsPerPoly = atoi(COM_Parse(&text_p));
	for(int i=0;i<3;i++) {
		navHeader.mins[i] = atof(COM_Parse(&text_p));
	}
	for(int i=0;i<3;i++) {
		navHeader.maxs[i] = atof(COM_Parse(&text_p));
	}
	navHeader.dNumMeshes = atoi(COM_Parse(&text_p));
	navHeader.dNumVerts = atoi(COM_Parse(&text_p));
	navHeader.dNumTris = atoi(COM_Parse(&text_p));
	navHeader.cellSize = atof(COM_Parse(&text_p));
	navHeader.cellHeight = atof(COM_Parse(&text_p));

	//load verts
	int vertsSize = navHeader.numVerts * sizeof (unsigned short) * 3;
	navHeader.data.verts = (unsigned short *)malloc(vertsSize);
	for(int i=0,j=0;i<navHeader.numVerts;i++,j+=3) {
		for(int n=0;n<3;n++) {
			navHeader.data.verts[j + n] = (unsigned short)atoi(COM_Parse(&text_p));
		}
	}
	navParams->vertCount = navHeader.numVerts;
	navParams->verts = navHeader.data.verts;

	//load polys
	int polysSize = navHeader.numPolys * sizeof (unsigned short) * navHeader.numVertsPerPoly * 2;
	navHeader.data.polys = (unsigned short *)malloc(polysSize);
	for(int i=0, k=0;i<navHeader.numPolys;i++,k+=navHeader.numVertsPerPoly * 2) {
		for(int n=0;n<navHeader.numVertsPerPoly * 2;n++) {
			navHeader.data.polys[k + n] = (unsigned short)atoi(COM_Parse(&text_p));
		}
	}
	navParams->polyCount = navHeader.numPolys;
	navParams->nvp = navHeader.numVertsPerPoly;
	navParams->polys = navHeader.data.polys;

	//load areas
	int areasSize = navHeader.numPolys * sizeof (unsigned char);
	navHeader.data.areas = (unsigned char *)malloc(areasSize);
	for(int i=0;i<navHeader.numPolys;i++) {
		navHeader.data.areas[i] = (unsigned char)atoi(COM_Parse(&text_p));
	}
	navParams->polyAreas = navHeader.data.areas;

	//load flags
	int flagsSize = navHeader.numPolys * sizeof (unsigned short);
	navHeader.data.flags = (unsigned short *)malloc(flagsSize);
	for(int i=0;i<navHeader.numPolys;i++) {
		navHeader.data.flags[i] = (unsigned short)atoi(COM_Parse(&text_p));
	}
	navParams->polyFlags = navHeader.data.flags;

	//load detailed meshes
	int dMeshesSize = navHeader.dNumMeshes * sizeof (unsigned int) * 4;
	navHeader.data.dMeshes = (unsigned int *)malloc(dMeshesSize);
	for(int i=0,j=0;i<navHeader.dNumMeshes;i++,j+=4) {
		for(int n=0;n<4;n++) {
			navHeader.data.dMeshes[j + n] = (unsigned int)atoi(COM_Parse(&text_p));
		}
	}
	navParams->detailMeshes = navHeader.data.dMeshes;

	//load detailed verticies
	int dVertsSize = navHeader.dNumVerts * sizeof (float) * 3;
	navHeader.data.dVerts = (float *)malloc(dVertsSize);
	for(int i=0,j=0;i<navHeader.dNumVerts;i++,j+=3) {
		for(int n=0;n<3;n++) {
			navHeader.data.dVerts[j + n] = atof(COM_Parse(&text_p));
		}
	}
	navParams->detailVertsCount = navHeader.dNumVerts;
	navParams->detailVerts = navHeader.data.dVerts;

	//load detailed tris
	int dTrisSize = navHeader.dNumTris * sizeof (unsigned char) * 4;
	navHeader.data.dTris = (unsigned char *)malloc(dTrisSize);
	for(int i=0,j=0;i<navHeader.dNumTris;i++,j+=4) {
		for(int n=0;n<4;n++) {
			navHeader.data.dTris[j + n] = (unsigned char)atoi(COM_Parse(&text_p));
		}
	}
	navParams->detailTriCount = navHeader.dNumTris;
	navParams->detailTris = navHeader.data.dTris;

	//other parameters
	navParams->walkableHeight = 48.0f;
	navParams->walkableRadius = 15.0f;
	navParams->walkableClimb = STEPSIZE;
	VectorCopy (navHeader.mins, navParams->bmin);
	VectorCopy (navHeader.maxs, navParams->bmax);
	navParams->cs = navHeader.cellSize;
	navParams->ch = navHeader.cellHeight;
	navParams->offMeshConCount = 0;
	navParams->buildBvTree = true;

	Com_Printf(" done.\n");
	return qtrue;
}
extern "C" void G_NavMeshInit() {
	Com_Printf("==== Bot Navigation Initialization ==== \n");
	memset(navMeshes,0,sizeof(*navMeshes));
	memset(navQuerys,0,sizeof(*navQuerys));
	for(int i=PCL_NONE+1;i<PCL_NUM_CLASSES;i++) {
		dtNavMeshCreateParams navParams;
		unsigned char *navData = NULL;
		int navDataSize = 0;
		memset(&navParams,0,sizeof(dtNavMeshCreateParams));

		if(!G_NavLoad(&navParams, (class_t)i)) {

			return;
		}

		if ( !dtCreateNavMeshData (&navParams, &navData, &navDataSize) )
		{
			Com_Printf ("Could not build Detour Navigation Mesh Data for class %s\n",BG_Class((class_t)i)->name);
			return;
		}

		navMeshes[i] = dtAllocNavMesh();

		if (!navMeshes[i])
		{
			dtFree(navData);
			navData = NULL;
			Com_Printf ("Could not allocate Detour Navigation Mesh for class %s\n",BG_Class((class_t)i)->name);
			return;
		}

		if (dtStatusFailed(navMeshes[i]->init(navData, navDataSize, DT_TILE_FREE_DATA)))
		{
			dtFree(navData);
			navData = NULL;
			dtFreeNavMesh(navMeshes[i]);
			navMeshes[i] = NULL;
			Com_Printf ("Could not init Detour Navigation Mesh for class %s\n", BG_Class((class_t)i)->name);
			return;
		}

		navQuerys[i] = dtAllocNavMeshQuery();
		if(!navQuerys[i]) {
			dtFree(navData);
			navData = NULL;
			dtFreeNavMesh(navMeshes[i]);
			navMeshes[i] = NULL;
			Com_Printf("Could not allocate Detour Navigation Mesh Query for class %s\n",BG_Class((class_t)i)->name);
			return;
		}
		if (dtStatusFailed(navQuerys[i]->init(navMeshes[i], 65536)))
		{
			dtFreeNavMeshQuery(navQuerys[i]);
			navQuerys[i] = NULL;
			dtFree(navData);
			navData = NULL;
			dtFreeNavMesh(navMeshes[i]);
			navMeshes[i] = NULL;
			Com_Printf("Could not init Detour Navigation Mesh Query for class %s",BG_Class((class_t)i)->name);
			return;
		}
		navFilters[i].setIncludeFlags(POLYFLAGS_WALK);
		navFilters[i].setExcludeFlags(0);
	}
	for(int i=0;i<MAX_CLIENTS;i++) {
		if(!pathCorridor[i].init(MAX_PATH_POLYS)) {
			Com_Printf("Could not init the path corridor\n");
		}
	}
	navMeshLoaded = qtrue;
}
extern "C" void G_NavMeshCleanup(void) {
	for(int i=PCL_NONE+1;i<PCL_NUM_CLASSES;i++) {
		if(navQuerys[i]) {
			dtFreeNavMeshQuery(navQuerys[i]);
			navQuerys[i] = NULL;
		}
		if(navMeshes[i]) {
			dtFreeNavMesh(navMeshes[i]);
			navMeshes[i] = NULL;
		}
	}
	for(int i=0;i<MAX_CLIENTS;i++) {
		pathCorridor[i].~dtPathCorridor();
		pathCorridor[i] = dtPathCorridor();
	}
	navMeshLoaded = qfalse;
}
/*
========================
Bot Navigation Querys
========================
*/

void BotGetAgentExtents(gentity_t *ent, vec3_t extents) {
	VectorSet(extents, ent->r.maxs[0] * 5,  2 * (ent->r.maxs[2] - ent->r.mins[2]), ent->r.maxs[1] * 5);
}
int DistanceToGoal(gentity_t *self) {
	vec3_t targetPos;
	vec3_t selfPos;
	//safety check for morons who use this incorrectly
	if(!(self->botMind)) {
		return -1;
	}
	BotGetTargetPos(self->botMind->goal,targetPos);
	VectorCopy(self->s.origin,selfPos);
	return Distance(selfPos,targetPos);
}
int DistanceToGoalSquared(gentity_t *self) {
	vec3_t targetPos;
	vec3_t selfPos;
	//safety check for morons who use this incorrectly
	if(!(self->botMind)) {
		return -1;
	}
	BotGetTargetPos(self->botMind->goal,targetPos);
	VectorCopy(self->s.origin,selfPos);
	return DistanceSquared(selfPos,targetPos);
}
qboolean BotNav_Trace(dtNavMeshQuery* navQuery, dtQueryFilter* navFilter, vec3_t start, vec3_t end, float *hit, vec3_t normal, dtPolyRef *pathPolys, int *numHit, int maxPolies) {
	vec3_t nearPoint;
	dtPolyRef startRef;
	dtStatus status;
	vec3_t extents = {75,96,75};
	quake2recast(start);
	quake2recast(end);
	status = navQuery->findNearestPoly(start,extents,navFilter,&startRef,nearPoint);
	if(dtStatusFailed(status) || startRef == 0) {
		//try larger extents
		extents[1] += 500;
		status = navQuery->findNearestPoly(start,extents,navFilter,&startRef,nearPoint);
		if(dtStatusFailed(status) || startRef == 0) {
			*numHit = 0;
			*hit = 0;
			VectorSet(normal,0,0,0);
			if(maxPolies > 0) {
				pathPolys[0] = startRef;
				*numHit = 1;
			}
			return qfalse;
		}
	}
	status = navQuery->raycast(startRef,start,end,navFilter, hit, normal, pathPolys, numHit, maxPolies);
	if(dtStatusFailed(status)) {
		return qfalse;
	} else {
		return qtrue;
	}
}
qboolean BotPathIsWalkable(gentity_t *self, botTarget_t target) {
	dtPolyRef pathPolys[MAX_PATH_POLYS];
	vec3_t selfPos, targetPos;
	float hit;
	int numPolys;
	vec3_t hitNormal;
	vec3_t viewNormal;
	BG_GetClientNormal(&self->client->ps,viewNormal);
	VectorMA(self->s.origin,self->r.mins[2],viewNormal,selfPos);
	BotGetTargetPos(target, targetPos);
	//quake2recast(selfPos);
	//quake2recast(targetPos);

	if(!BotNav_Trace(self->botMind->navQuery, self->botMind->navFilter, selfPos,targetPos,&hit,hitNormal,pathPolys,&numPolys,MAX_PATH_POLYS))
		return qfalse;

	if(hit == FLT_MAX)
		return qtrue;
	else
		return qfalse;
}
qboolean BotFindNearestPoly(gentity_t *self, gentity_t *ent, dtPolyRef *nearestPoly, vec3_t nearPoint) {
	vec3_t extents;
	vec3_t start;
	vec3_t viewNormal;
	dtStatus status;
	dtNavMeshQuery* navQuery;
	dtQueryFilter* navFilter;
	if(ent->client) {
		BG_GetClientNormal(&ent->client->ps,viewNormal);
		navQuery = self->botMind->navQuery;
		navFilter = self->botMind->navFilter;
	} else {
		VectorSet(viewNormal,0,0,1);
		navQuery = self->botMind->navQuery;
		navFilter = self->botMind->navFilter;
	}
	VectorMA(ent->s.origin,ent->r.mins[2],viewNormal,start);
	quake2recast(start);
	BotGetAgentExtents(ent,extents);
	status = navQuery->findNearestPoly(start,extents,navFilter,nearestPoly,nearPoint);
	if(dtStatusFailed(status) || *nearestPoly == 0) {
		//try larger extents
		extents[1] += 900;
		status = navQuery->findNearestPoly(start,extents,navFilter,nearestPoly,nearPoint);
		if(dtStatusFailed(status) || *nearestPoly == 0) {
			return qfalse; // failed
		}
	}
	recast2quake(nearPoint);
	return qtrue;

}
qboolean BotFindNearestPoly(gentity_t *self, vec3_t coord, dtPolyRef *nearestPoly, vec3_t nearPoint) {
	vec3_t start,extents;
	dtStatus status;
	dtNavMeshQuery* navQuery = self->botMind->navQuery;
	dtQueryFilter* navFilter = self->botMind->navFilter;
	VectorSet(extents,640,96,640);
	VectorCopy(coord, start);
	quake2recast(start);
	status = navQuery->findNearestPoly(start,extents,navFilter,nearestPoly,nearPoint);
	if(dtStatusFailed(status) || *nearestPoly == 0) {
		//try larger extents
		extents[1] += 900;
		status = navQuery->findNearestPoly(start,extents,navFilter,nearestPoly,nearPoint);
		if(dtStatusFailed(status) || *nearestPoly == 0) {
			return qfalse; // failed
		}
	}
	recast2quake(nearPoint);
	return qtrue;
}
/*
========================
Local Bot Navigation
========================
*/
void BotDodge(gentity_t *self, usercmd_t *botCmdBuffer) {
	if(self->client->time1000 >= 500)
		botCmdBuffer->rightmove = 127;
	else
		botCmdBuffer->rightmove = -127;

	if((self->client->time10000 % 2000) < 1000)
		botCmdBuffer->rightmove *= -1;

	if((self->client->time1000 % 300) >= 100 && (self->client->time10000 % 3000) > 2000)
		botCmdBuffer->rightmove = 0;
}
gentity_t* BotGetPathBlocker(gentity_t *self) {
	vec3_t playerMins,playerMaxs;
	vec3_t end;
	vec3_t forward;
	trace_t trace;
	vec3_t moveDir;
	const int TRACE_LENGTH = BOT_OBSTACLE_AVOID_RANGE;

	if(!(self && self->client))
		return NULL;

	//get the forward vector, ignoring pitch
	forward[0] = cos(DEG2RAD(self->client->ps.viewangles[YAW]));
	forward[1] = sin(DEG2RAD(self->client->ps.viewangles[YAW]));
	forward[2] = 0;
	//VectorSubtract(target,self->s.origin, moveDir);
	//moveDir[2] = 0;
	//VectorNormalize(moveDir);
	//already normalized
	VectorCopy(forward,moveDir);

	BG_ClassBoundingBox((class_t) self->client->ps.stats[STAT_CLASS], playerMins,playerMaxs,NULL,NULL,NULL);

	//account for how large we can step
	playerMins[2] += STEPSIZE;
	playerMaxs[2] += STEPSIZE;

	VectorMA(self->s.origin, TRACE_LENGTH, moveDir, end);

	trap_Trace(&trace, self->s.origin, playerMins, playerMaxs, end, self->s.number, CONTENTS_BODY);
	if(trace.entityNum != ENTITYNUM_NONE && trace.fraction < 1.0f) {
		return &g_entities[trace.entityNum];
	} else {
		return NULL;
	}
}

qboolean BotShouldJump(gentity_t *self, gentity_t *blocker) {
	vec3_t playerMins;
	vec3_t playerMaxs;
	vec3_t moveDir;
	float jumpMagnitude;
	trace_t trace;
	const int TRACE_LENGTH = BOT_OBSTACLE_AVOID_RANGE;
	vec3_t forward;
	vec3_t end;

	//blocker is not on our team, so ignore
	if(BotGetTeam(self) != BotGetTeam(blocker))
		return qfalse;

	VectorSubtract(self->botMind->route[0], self->s.origin, forward);
	//get the forward vector, ignoring z axis
	/*forward[0] = cos(DEG2RAD(self->client->ps.viewangles[YAW]));
	forward[1] = sin(DEG2RAD(self->client->ps.viewangles[YAW]));
	forward[2] = 0;*/
	forward[2] = 0;
	VectorNormalize(forward);

	//already normalized
	VectorCopy(forward,moveDir);
	BG_ClassBoundingBox((class_t) self->client->ps.stats[STAT_CLASS], playerMins,playerMaxs,NULL,NULL,NULL);

	playerMins[2] += STEPSIZE;
	playerMaxs[2] += STEPSIZE;


	//trap_Print(vtos(self->movedir));
	VectorMA(self->s.origin, TRACE_LENGTH, moveDir, end);

	//make sure we are moving into a block
	trap_Trace(&trace,self->s.origin,playerMins,playerMaxs,end,self->s.number,MASK_SHOT);
	if(trace.fraction >= 1.0f || blocker != &g_entities[trace.entityNum]) {
	  return qfalse;
	}

	jumpMagnitude = BG_Class((class_t)self->client->ps.stats[STAT_CLASS])->jumpMagnitude;

	//find the actual height of our jump
	jumpMagnitude = Square(jumpMagnitude) / (self->client->ps.gravity * 2);

	//prepare for trace
	playerMins[2] += jumpMagnitude;
	playerMaxs[2] += jumpMagnitude;

	//check if jumping will clear us of entity
	trap_Trace(&trace, self->s.origin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT);

	//if we can jump over it, then jump
	//note that we also test for a blocking barricade because barricades will collapse to let us through
	if(blocker->s.modelindex == BA_A_BARRICADE || trace.fraction == 1.0f)
		return qtrue;
	else
		return qfalse;
}
int BotGetStrafeDirection(void) {
	if(level.time % 8000 < 4000) {
		return 127;
	} else {
		return -127;
	}
}
qboolean BotFindSteerTarget(gentity_t *self, vec3_t aimPos) {
	vec3_t forward;
	vec3_t testPoint1,testPoint2;
	vec3_t playerMins,playerMaxs;
	float yaw1, yaw2;
	trace_t trace1, trace2;

	if(!(self && self->client))
		return qfalse;

	//get bbox
	BG_ClassBoundingBox((class_t) self->client->ps.stats[STAT_CLASS], playerMins,playerMaxs,NULL,NULL,NULL);

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
	for(int i=0;i<5;i++,yaw1 -= 15 ,yaw2 += 15) {
		//compute forward for right
		forward[0] = cos(DEG2RAD(yaw1));
		forward[1] = sin(DEG2RAD(yaw1));
		//forward is already normalized
		//try the right
		VectorMA(self->s.origin,BOT_OBSTACLE_AVOID_RANGE,forward,testPoint1);

		//test it
		trap_Trace(&trace1,self->s.origin,playerMins,playerMaxs,testPoint1,self->s.number,MASK_SHOT);

		//check if unobstructed
		if(trace1.fraction >= 1.0f) {
			VectorCopy(testPoint1,aimPos);
			aimPos[2] += self->client->ps.viewheight;
			return qtrue;
		}

		//compute forward for left
		forward[0] = cos(DEG2RAD(yaw2));
		forward[1] = sin(DEG2RAD(yaw2));
		//forward is already normalized
		//try the left
		VectorMA(self->s.origin,BOT_OBSTACLE_AVOID_RANGE,forward,testPoint2);

		//test it
		trap_Trace(&trace2, self->s.origin,playerMins,playerMaxs,testPoint2,self->s.number,MASK_SHOT);

		//check if unobstructed
		if(trace2.fraction >= 1.0f) {
			VectorCopy(testPoint2,aimPos);
			aimPos[2] += self->client->ps.viewheight;
			return qtrue;
		}
	}

	//we couldnt find a new position
	return qfalse;
}
qboolean BotAvoidObstacles(gentity_t *self, vec3_t rVec, usercmd_t *botCmdBuffer) {
	if(!(self && self->client))
		return qfalse;
	if(BotGetTeam(self) == TEAM_NONE)
		return qfalse;
	gentity_t *blocker = BotGetPathBlocker(self);
	if(blocker) {
		if(BotShouldJump(self, blocker)) {
			//jump if we have enough stamina
			if(self->client->ps.stats[STAT_STAMINA] >= STAMINA_SLOW_LEVEL + STAMINA_JUMP_TAKE) {
				botCmdBuffer->upmove = 127;
			} else if(BotGetTeam(self) == BotGetTeam(blocker) || blocker->s.number == ENTITYNUM_WORLD){
				botCmdBuffer->forwardmove = 0;
				botCmdBuffer->rightmove = 0;
			}
			return qfalse;
		} else {
			vec3_t newAimPos;
				//we should not jump, try to get around the obstruction
			if(BotFindSteerTarget(self, newAimPos)) {
				//found a steer target
				VectorCopy(newAimPos,rVec);
				return qtrue;
			} else {
				botCmdBuffer->rightmove = BotGetStrafeDirection();
				botCmdBuffer->forwardmove = -127; //backup
			}
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

	if( !BG_ClassHasAbility( (class_t) self->client->ps.stats[ STAT_CLASS ], SCA_CANUSELADDERS ) )
		return qfalse;

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL);

	forward[ 2 ] = 0.0f;
	BG_ClassBoundingBox( (class_t) self->client->ps.stats[ STAT_CLASS ], mins, maxs, NULL, NULL, NULL );
	VectorMA( self->s.origin, 1.0f, forward, end );

	trap_Trace( &trace, self->s.origin, mins, maxs, end, self->s.number, MASK_PLAYERSOLID );

	if( trace.fraction < 1.0f && trace.surfaceFlags & SURF_LADDER )
		return qtrue;
	else
		return qfalse;
}
/*
BotSteer
Slows down aiming for waypoints
Based on Code by Mikko Mononen, but adapted for use in tremz
*/
void BotSteer(gentity_t *self, vec3_t target) {
	vec3_t viewBase;
	vec3_t aimVec;
	vec3_t skilledVec;

	if( !(self && self->client) )
		return;
	BG_GetClientViewOrigin(&self->client->ps,viewBase);
	//get the Vector from the bot to the enemy (aim Vector)
	VectorSubtract( target, viewBase, aimVec );
	float length = VectorNormalize(aimVec);

	const int ip0 = 0;
	const int ip1 = MIN(1,self->botMind->numCorners - 1);
	vec3_t p0,p1;
	vec3_t selfPos;
	VectorCopy(self->s.origin,selfPos);
	selfPos[2] += self->r.mins[2];
	VectorCopy(self->botMind->route[ip0],p0);
	VectorCopy(self->botMind->route[ip1],p1);
	vec3_t dir0,dir1;
	VectorSubtract(p0,selfPos,dir0);
	VectorSubtract(p1,selfPos,dir1);
	dir0[2] = 0;
	dir1[2] = 0;
	float len0 = VectorLength(dir0);
	float len1 = VectorLength(dir1);
	if(len1 > 0.001f)
		VectorScale(dir1,1.0/len1,dir1);

	skilledVec[0] = dir0[0] - dir1[0]*len0*0.5f;
	skilledVec[1] = dir0[1] - dir1[1]*len0*0.5f;
	skilledVec[2] = aimVec[2];

	VectorNormalize(skilledVec);
	VectorScale(skilledVec, length, skilledVec);
	VectorAdd(viewBase, skilledVec, target);
}
/**BotGoto
* Used to make the bot travel between waypoints or to the target from the last waypoint
* Also can be used to make the bot travel other short distances
*/
void BotGoto(gentity_t *self, botTarget_t target, usercmd_t *botCmdBuffer) {

	vec3_t tmpVec;
	botCmdBuffer->forwardmove = 127; //max forward speed
	BotGetIdealAimLocation(self, target, tmpVec);
	if(BotAvoidObstacles(self, tmpVec, botCmdBuffer)) { //make whatever adjustments we need to make
		BotSlowAim(self, tmpVec, 0.6);
	} else {
		BotSteer(self, tmpVec); //steer to stay on path
	}

	BotAimAtLocation(self, tmpVec, botCmdBuffer);

	//dont sprint or dodge if we dont have enough stamina and are about to slow
	if(BotGetTeam(self) == TEAM_HUMANS && self->client->ps.stats[STAT_STAMINA] < STAMINA_SLOW_LEVEL) {
		usercmdReleaseButton(botCmdBuffer->buttons, BUTTON_SPRINT);
		usercmdReleaseButton(botCmdBuffer->buttons, BUTTON_DODGE);
	}
}

/*
=========================
Global Bot Navigation
=========================
*/

void FindWaypoints(gentity_t *self) {
	float corners[MAX_ROUTE_NODES*3];
	unsigned char cornerFlags[MAX_ROUTE_NODES];
	dtPolyRef cornerPolys[MAX_ROUTE_NODES];

	if(!self->botMind->pathCorridor->getPathCount()) {
		BotDPrintf(S_COLOR_RED "ERROR: FindWaypoints Called without a path!\n");
		self->botMind->numCorners = 0;
		return;
	}

	//trap_Print("finding Corners\n");
	int numCorners = self->botMind->pathCorridor->findCorners(corners, cornerFlags, cornerPolys, MAX_ROUTE_NODES,self->botMind->navQuery, self->botMind->navFilter);
	//trap_Print("found Corners\n");
	//copy the points to the route array, converting each point into quake coordinates too
	for(int i=0;i<numCorners;i++) {
		float *vert = &corners[i*3];
		recast2quake(vert);
		VectorCopy(vert,self->botMind->route[i]);
	}

	self->botMind->numCorners = numCorners;
}
float Distance2D(vec3_t pos1, vec3_t pos2) {
	return sqrt(pos1[0] * pos2[0] + pos1[1] * pos2[1]);
}
void UpdatePathCorridor(gentity_t *self) {
	vec3_t selfPos, targetPos;
	if(!self->botMind->pathCorridor->getPathCount()) {
		BotDPrintf(S_COLOR_RED "ERROR: UpdatePathCorridor called without a path!\n");
		return;
	}
	if(BotTargetIsEntity(self->botMind->goal)) {
		if(self->botMind->goal.ent->s.groundEntityNum == -1 || self->botMind->goal.ent->s.groundEntityNum == ENTITYNUM_NONE)
			PlantEntityOnGround(self->botMind->goal.ent,targetPos);
		else
			BotGetTargetPos(self->botMind->goal, targetPos);
	} else
		BotGetTargetPos(self->botMind->goal, targetPos);

	if(self->s.groundEntityNum == -1 || self->s.groundEntityNum == ENTITYNUM_NONE)
		PlantEntityOnGround(self,selfPos);
	else
		VectorCopy(self->s.origin,selfPos);

	quake2recast(selfPos);
	quake2recast(targetPos);

	self->botMind->pathCorridor->movePosition(selfPos, self->botMind->navQuery, self->botMind->navFilter);
	self->botMind->pathCorridor->moveTargetPosition(targetPos,self->botMind->navQuery, self->botMind->navFilter);


	FindWaypoints(self);
	dtPolyRef check;
	vec3_t pos;

	//check for replans
	//THIS STILL DOESNT WORK 100% OF THE TIME GRRRRRRR
	if(self->client->time1000 % 500 == 0) {
		if(BotFindNearestPoly(self, self,&check,pos) && self->client->ps.groundEntityNum != ENTITYNUM_NONE) {
			if(check != self->botMind->pathCorridor->getFirstPoly())
				FindRouteToTarget(self, self->botMind->goal);
		}
		if(BotTargetIsPlayer(self->botMind->goal)) {
			if(self->botMind->goal.ent->client->ps.groundEntityNum != ENTITYNUM_NONE) {
				if(BotFindNearestPoly(self, self->botMind->goal.ent,&check,pos)) {
					if(check != self->botMind->pathCorridor->getLastPoly())
						FindRouteToTarget(self, self->botMind->goal);
				}
			}
		}
	}
}
qboolean BotMoveToGoal( gentity_t *self, usercmd_t *botCmdBuffer ) {
	botTarget_t target;

	if(!(self && self->client))
		return qfalse;

	UpdatePathCorridor(self);

	if(self->botMind->numCorners > 0) {
		BotSetTarget(&target,NULL, &self->botMind->route[0]);
		BotGoto( self, target, botCmdBuffer );
		return qfalse;
	}
	return qtrue;
}

int FindRouteToTarget( gentity_t *self, botTarget_t target) {
	vec3_t selfPos;
	vec3_t targetPos;
	vec3_t start;
	vec3_t end;
	dtPolyRef startRef, endRef = 1;
	dtPolyRef pathPolys[MAX_PATH_POLYS];
	dtStatus status;
	int pathNumPolys = 512;
	qboolean result;
	//int straightNum;

	//dont pathfind too much
	if(level.time - self->botMind->timeFoundRoute < 200)
		return STATUS_FAILED;

	if(!self->botMind->navQuery) {
		BotDPrintf("Cannot query the Navmesh!\n");
		return STATUS_FAILED;
	}

	self->botMind->timeFoundRoute = level.time;

	if(BotTargetIsEntity(target)) {
		PlantEntityOnGround(target.ent,targetPos);
	} else {
		BotGetTargetPos(target,targetPos);
	}

	if(!BotFindNearestPoly(self, self, &startRef, start)) {
		BotDPrintf("Failed to find a polygon near the bot\n");
		return STATUS_FAILED | STATUS_NOPOLYNEARSELF;
	}

	if(BotTargetIsEntity(target)) {
		result = BotFindNearestPoly(self, target.ent,&endRef,end);
	} else {
		result = BotFindNearestPoly(self, targetPos,&endRef,end);
	}

	if(!result) {
		BotDPrintf("Failed to find a polygon near the target\n");
		return STATUS_FAILED | STATUS_NOPOLYNEARTARGET;
	}
	quake2recast(selfPos);
	quake2recast(targetPos);
	self->botMind->numCorners = 0;

	quake2recast(start);
	quake2recast(end);
	//trap_Print("Finding path\n");
	status = self->botMind->navQuery->findPath(startRef, endRef, start, end, self->botMind->navFilter, pathPolys, &pathNumPolys, MAX_PATH_POLYS);

	if(dtStatusFailed(status)) {
		BotDPrintf("Could not find path\n");
		return STATUS_FAILED;
	}
	self->botMind->pathCorridor->reset(startRef, selfPos);
	self->botMind->pathCorridor->setCorridor(targetPos, pathPolys, pathNumPolys);

	FindWaypoints(self);
	if(status & DT_PARTIAL_RESULT) {
		BotDPrintf("Found a partial path\n");
		return STATUS_SUCCEED | STATUS_PARTIAL;
	}

	return STATUS_SUCCEED;
}

/*
========================
Misc Bot Nav
========================
*/
extern "C" qboolean addOffMeshConnection(vec3_t start, vec3_t end, const int type) {
	quake2recast(start);
	quake2recast(end);
	//is the type a power of 2?
	//64 == max flag number
	if((!(type & (type - 1)) && type && type <= 64)) {

		//TODO: add connection

		return qtrue;
	}
	return qfalse;

}
void PlantEntityOnGround(gentity_t *ent, vec3_t groundPos) {
	vec3_t mins,maxs;
	trace_t trace;
	vec3_t realPos;
	const int traceLength = 1000;
	vec3_t endPos;
	vec3_t traceDir;
	if(ent->client) {
		BG_ClassBoundingBox((class_t)ent->client->ps.stats[STAT_CLASS],mins,maxs,NULL,NULL,NULL);
	} else if(ent->s.eType == ET_BUILDABLE) {
		BG_BuildableBoundingBox((buildable_t)ent->s.modelindex, mins,maxs);
	} else {
		VectorCopy(ent->r.mins,mins);
		VectorCopy(ent->r.maxs,maxs);
	}

	VectorSet(traceDir,0,0,-1);
	VectorCopy(ent->s.origin, realPos);
	VectorMA(realPos,traceLength,traceDir,endPos);
	trap_Trace(&trace,ent->s.origin,mins,maxs,endPos,ent->s.number,CONTENTS_SOLID);
	if(trace.fraction != 1.0f) {
		VectorCopy(trace.endpos,groundPos);
	} else {
		VectorCopy(realPos,groundPos);
		groundPos[2] += mins[2];

	}
}
