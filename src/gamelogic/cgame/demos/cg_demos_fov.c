/*
===========================================================================
Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cg_demos.h" 

static void chasePointFree(demoChasePoint_t *point) {
	if (point->prev) 
		point->prev->next = point->next;
	else 
		demo.chase.points = point->next;
	if (point->next ) 
		point->next->prev = point->prev;
	free( point );
}

demoChasePoint_t *chasePointSynch(int time ) {
	demoChasePoint_t *point = demo.chase.points;
	if (!point)
		return 0;
	for( ;point->next && point->next->time <= time; point = point->next);
	return point;
}

static qboolean chasePointAdd(int time, const vec3_t angles, vec_t distance, int target, const vec3_t origin) {
	demoChasePoint_t *point = chasePointSynch( time );
	demoChasePoint_t *newPoint;
	if (!point || point->time > time) {
		newPoint = chasePointAlloc();
		newPoint->next = demo.chase.points;
		if (demo.chase.points)
			demo.chase.points->prev = newPoint;
		demo.chase.points = newPoint;
		newPoint->prev = 0;
	} else if (point->time == time) {
		newPoint = point;
	} else {
		newPoint = chasePointAlloc();
		newPoint->prev = point;
		newPoint->next = point->next;
		if (point->next)
			point->next->prev = newPoint;
		point->next = newPoint;
	}
	newPoint->target = target;
	newPoint->distance = distance;
	VectorCopy( angles, newPoint->angles );
	VectorCopy( origin, newPoint->origin );
	newPoint->time = time;
	return qtrue;
}

static qboolean chasePointDel( int time ) {
	demoChasePoint_t *point = chasePointSynch( time);

	if (!point || !point->time == time)
		return qfalse;
	chasePointFree( point );
	return qtrue;
}

static void chaseClear( void ) {
	demoChasePoint_t *next, *point = demo.chase.points;
	while ( point ) {
		next = point->next;
		chasePointFree( point );
		point = next;
	}
}


static void chasePointControl( demoChasePoint_t *point, int times[4], vec4_t origins[4], Quat_t quats[4]) {
	vec3_t tempAngles;

	if (!point->next)
		return;
	
	times[1] = point->time;
	origins[1][3] = point->distance;
	VectorCopy( point->origin, origins[1] );
	QuatFromAngles( point->angles, quats[1] );

	times[2] = point->next->time;
	origins[2][3] = point->next->distance;
	VectorCopy( point->next->origin, origins[2] );
	QuatFromAnglesClosest( point->next->angles, quats[1], quats[2] );
	if (!point->prev) {
		times[0] = times[1] - (times[2] - times[1]);
		origins[0][3] = origins[1][3] - (origins[2][3] - origins[1][3]);
		VectorSubtract( origins[2], origins[1], origins[0] );
		VectorSubtract( origins[1], origins[0], origins[0] );
		VectorSubtract( point->next->angles, point->angles, tempAngles );
		VectorAdd( tempAngles, point->angles, tempAngles );
		QuatFromAnglesClosest( tempAngles, quats[1], quats[0] );
	} else {
		times[0] = point->prev->time;
		origins[0][3] = point->prev->distance;
		VectorCopy( point->prev->origin, origins[0] );
		QuatFromAnglesClosest( point->prev->angles, quats[1], quats[0] );
	}
	if (!point->next->next) {
		times[3] = times[2] + (times[2] - times[1]);
		origins[3][3] = origins[2][3] + (origins[2][3] - origins[1][3]);
		VectorSubtract( origins[2], origins[1], origins[3] );
		VectorAdd( origins[2], origins[3], origins[3] );
		VectorSubtract( point->next->angles, point->angles, tempAngles );
		VectorAdd( tempAngles, point->next->angles, tempAngles );
		QuatFromAnglesClosest( tempAngles, quats[2], quats[3] );
	} else {
		times[3] = point->next->next->time;
		origins[3][3] = point->next->next->distance;
		VectorCopy( point->next->next->origin, origins[3] );
		QuatFromAnglesClosest( point->next->next->angles, quats[2], quats[3] );
	}
}

static void chasePointDraw( demoChasePoint_t *point ) {
	int times[4];
	vec4_t origins[4];
	Quat_t quats[4];

	float	lerp;
	vec4_t	lerpOrigin, lastOrigin;
	Quat_t	lerpQuat;
	vec3_t	axis[3], start, end;
	qboolean	firstOne = qtrue;

	chasePointControl( point, times, origins, quats );
	for (lerp = 0; lerp < 1.1f; lerp+=0.1f) {
		QuatTimeSpline( lerp, times, quats, lerpQuat );
		VectorTimeSpline( lerp, times, origins, lerpOrigin );
		QuatToAxis( lerpQuat, axis );
		if (firstOne) {
			demoDrawCross( lerpOrigin, colorRed );
			VectorCopy( lerpOrigin, start );
			VectorMA( start, -lerpOrigin[3], axis[0], end );
			demoDrawLine( start, end, colorBlue );
			demoDrawCross( end, colorBlue );
			firstOne = qfalse;
		} else {
			demoDrawLine( lerpOrigin, lastOrigin, colorRed );
		}
		Vector4Copy( lerpOrigin, lastOrigin );
	}
}

static int chasePointShift( int shift ) {
	demoChasePoint_t *startPoint, *endPoint;
	if ( demo.chase.start != demo.chase.end ) {
		if (demo.chase.start > demo.chase.end )
			return 0;
		startPoint = chasePointSynch( demo.chase.start );
		endPoint = chasePointSynch( demo.chase.end );
		if (!startPoint || !endPoint)
			return 0;
		if (startPoint->time != demo.chase.start || endPoint->time != demo.chase.end) {
			if (demo.chase.shiftWarn < demo.serverTime) {
				CG_DemosAddLog( "Failed to do shift, selection doesn't match points");
				demo.chase.shiftWarn = demo.serverTime + 1000;
			}
			return 0;
		}
	} else {
		startPoint = endPoint = chasePointSynch( demo.play.time );
		if (!startPoint)
			return 0;
		if (startPoint->time != demo.play.time) {
			if (demo.chase.shiftWarn < demo.serverTime) {
				CG_DemosAddLog( "Failed to do shift, no point at current time");
				demo.chase.shiftWarn = demo.serverTime + 1000;
			}
			return 0;
		}
	}

	if (!shift)
		return 0;

	if (shift < 0) {
		if (!startPoint->prev) {
			if (startPoint->time + shift < 0)
				shift = -startPoint->time;
		} else {
			int delta = (startPoint->prev->time - startPoint->time );
			if (shift <= delta)
				shift = delta + 1;
		}
	} else {
		if (endPoint->next) {
			int delta = (endPoint->next->time - endPoint->time );
			if (shift >= delta)
				shift = delta - 1;
		}
	}
	while (startPoint) {
		startPoint->time += shift;
		if (startPoint == endPoint)
			break;
		startPoint = startPoint->next;
	}
	if ( demo.chase.start != demo.chase.end ) {
		demo.chase.start += shift;
		demo.chase.end += shift;
	} else {
		demo.play.time += shift;
	}
	return shift;
}


static void chaseInterpolate( int time, float timeFraction, vec3_t origin, vec3_t angles, float *distance, int *target ) {
	float	lerp;
	int		times[4];
	vec4_t	origins[4], pr;
	Quat_t	quats[4], qr;
	centity_t	*targetCent;
	demoChasePoint_t *point = chasePointSynch( time );

	if (!point->next) {
		VectorCopy( point->origin, origin );
		VectorCopy( point->angles, angles );
		*distance = point->distance;
		*target = point->target;
		return;
	}
	chasePointControl( point, times, origins, quats );

	lerp = ((time - point->time) + timeFraction) / (point->next->time - point->time);
	QuatTimeSpline( lerp, times, quats, qr );
	VectorTimeSpline( lerp, times, origins, pr );
	/* Copy the results */
	targetCent = demoTargetEntity( demo.chase.target );
	if ( targetCent ) {
		chaseEntityOrigin( targetCent, demo.chase.origin );
	} else {
		VectorCopy( pr, demo.chase.origin );
	}
	demo.chase.target = point->target;
	demo.chase.distance = pr[3];
	QuatToAngles( qr, demo.chase.angles );
}

void chaseUpdate( int time, float timeFraction ) {
	if (!demo.chase.locked || !demo.chase.points ) {
		centity_t *cent = demoTargetEntity( demo.chase.target );
		if (cent) 
			chaseEntityOrigin( cent, demo.chase.origin );
		return;
	}
	chaseInterpolate( time, timeFraction, demo.chase.origin, demo.chase.angles, &demo.chase.distance, &demo.chase.target );
}

void chaseDraw( int time, float timeFraction ) {
	demoChasePoint_t *point = chasePointSynch( time - 10000 );
	while (point && point->next && point->time < (time + 10000)) {
		if (point->time > (time - 10000))
			chasePointDraw( point );
		point = point->next;
	}
	if (demo.chase.target < 0) 
		demoDrawCross( demo.chase.origin, colorWhite );
	else if (demo.chase.target < MAX_GENTITIES) {
		centity_t *cent = demoTargetEntity( demo.chase.target );
		if (cent) {
			vec3_t container;
			demoCentityBoxSize( cent, container );
			demoDrawBox( cent->lerpOrigin, container, colorWhite );
		}
	}
}

typedef struct  {
	int time, target;
	vec3_t origin, angles;
	float distance;
	qboolean hasTime, hasTarget, hasOrigin, hasAngles, hasDistance;
} parseChasePoint_t;

static qboolean chaseParseTime( BG_XMLParse_t *parse,const char *line, void *data) {
	parseChasePoint_t *point= (parseChasePoint_t *)data;
	if (!line[0]) 
		return qfalse;
	point->time = atoi( line );
	point->hasTime = qtrue;
	return qtrue;
}
static qboolean chaseParseDistance( BG_XMLParse_t *parse,const char *line, void *data) {
	parseChasePoint_t *point= (parseChasePoint_t *)data;
	if (!line[0]) 
		return qfalse;
	point->distance = atof( line );
	point->hasDistance = qtrue;
	return qtrue;
}
static qboolean chaseParseTarget( BG_XMLParse_t *parse,const char *line, void *data) {
	parseChasePoint_t *point= (parseChasePoint_t *)data;
	if (!line[0]) 
		return qfalse;
	point->target = atoi( line );
	point->hasTarget = qtrue;
	return qtrue;
}

static qboolean chaseParseAngles( BG_XMLParse_t *parse,const char *line, void *data) {
	parseChasePoint_t *point= (parseChasePoint_t *)data;
	if (!line[0]) 
		return qfalse;
	sscanf( line, "%f %f %f", &point->angles[0], &point->angles[1], &point->angles[2] );
	point->hasAngles = qtrue;
	return qtrue;
}
static qboolean chaseParseOrigin( BG_XMLParse_t *parse,const char *line, void *data) {
	parseChasePoint_t *point= (parseChasePoint_t *)data;
	if (!line[0]) 
		return qfalse;
	sscanf( line, "%f %f %f", &point->origin[0], &point->origin[1], &point->origin[2] );
	point->hasOrigin = qtrue;
	return qtrue;
}

static qboolean chaseParsePoint( BG_XMLParse_t *parse,const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	parseChasePoint_t pointLoad;
	static BG_XMLParseBlock_t chaseParseBlock[] = {
		{"origin", 0, chaseParseOrigin},
		{"angles", 0, chaseParseAngles},
		{"time", 0, chaseParseTime},
		{"distance", 0, chaseParseDistance},
		{"target", 0, chaseParseTarget},
		{0, 0, 0}
	};
	memset( &pointLoad, 0, sizeof( pointLoad ));
	if (!BG_XMLParse( parse, fromBlock, chaseParseBlock, &pointLoad )) {
		return qfalse;
	}
	if (!pointLoad.hasOrigin || !pointLoad.hasAngles || !pointLoad.hasTime || !pointLoad.hasDistance || !pointLoad.hasTarget ) 
		return BG_XMLError(parse, "Missing section in chase point");
	chasePointAdd( pointLoad.time, pointLoad.angles, pointLoad.distance, pointLoad.target, pointLoad.origin  );
	return qtrue;
}

static qboolean chaseParseLocked( BG_XMLParse_t *parse,const char *line, void *data) {
	if (!line[0]) 
		return qfalse;
	demo.chase.locked = atoi( line );
	return qtrue;
}

qboolean chaseParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	static BG_XMLParseBlock_t chaseParseBlock[] = {
		{"point",	chaseParsePoint,	0},
		{"locked",	0,					chaseParseLocked},
		{0, 0, 0}
	};

	chaseClear();
	if (!BG_XMLParse( parse, fromBlock, chaseParseBlock, data ))
		return qfalse;
	return qtrue;
}

void chaseSave( fileHandle_t fileHandle ) {
	demoChasePoint_t *point;

	point = demo.chase.points;
	demoSaveLine( fileHandle, "<chase>\n" );
	demoSaveLine( fileHandle, "\t<locked>%d</locked>\n", demo.chase.locked );
	while (point) {
		demoSaveLine( fileHandle, "\t<point>\n" );
		demoSaveLine( fileHandle, "\t\t<time>%10d</time>\n", point->time );
		demoSaveLine( fileHandle, "\t\t<distance>%9.4f</distance>\n", point->distance );
		demoSaveLine( fileHandle, "\t\t<target>%4d</target>\n", point->target );
		demoSaveLine( fileHandle, "\t\t<origin>%9.4f %9.4f %9.4f</origin>\n", point->origin[0], point->origin[1], point->origin[2] );
		demoSaveLine( fileHandle, "\t\t<angles>%9.4f %9.4f %9.4f</angles>\n", point->angles[0], point->angles[1], point->angles[2]);
		demoSaveLine( fileHandle, "\t</point>\n" );
		point = point->next;
	}
	demoSaveLine( fileHandle, "<\t/chase>\n" );
}

void demoChaseCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "lock")) {
		demo.chase.locked = !demo.chase.locked;
		if (demo.chase.locked) 
			CG_DemosAddLog("Chase view locked");
		else 
			CG_DemosAddLog("Chase view unlocked");
	} else if (!Q_stricmp(cmd, "add")) {
		if (chasePointAdd( demo.play.time, demo.chase.angles, demo.chase.distance, demo.chase.target, demo.chase.origin )) {
			CG_DemosAddLog("Added chase point" );
		} else {
			CG_DemosAddLog("Failed to add chase point" );
		}
	} else if (!Q_stricmp(cmd, "del")) {
		if (chasePointDel( demo.play.time) ) {
			CG_DemosAddLog("Deleted chase chase point" );
		} else {
			CG_DemosAddLog("Failed to add delete chase point" );
		}
	} else if (!Q_stricmp(cmd, "start")) { 
		demo.chase.start = demo.play.time;
		if (demo.chase.start > demo.chase.end)
			demo.chase.start = demo.chase.end;
		if (demo.chase.end == demo.chase.start)
			CG_DemosAddLog( "Cleared chase selection");
		else
			CG_DemosAddLog( "Chase selection start at %d.%03d", demo.chase.start / 1000, demo.chase.start % 1000 );
	} else if (!Q_stricmp(cmd, "end")) { 
		demo.chase.end = demo.play.time;
		if (demo.chase.end < demo.chase.start)
			demo.chase.end = demo.chase.start;
		if (demo.chase.end == demo.chase.start)
			CG_DemosAddLog( "Cleared chase selection");
		else
			CG_DemosAddLog( "Chase selection end at %d.%03d", demo.chase.end / 1000, demo.chase.end % 1000 );
	} else if (!Q_stricmp(cmd, "next")) {
		demoChasePoint_t *point = chasePointSynch( demo.play.time );
		if (!point)
			return;
		if (point->next)
			point = point->next;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	} else if (!Q_stricmp(cmd, "prev")) {
		demoChasePoint_t *point = chasePointSynch( demo.play.time );
		if (!point)
			return;
		if (point->prev)
			point = point->prev;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	} else if (!Q_stricmp(cmd, "clear")) {
		chaseClear();
	} else if (!Q_stricmp(cmd, "pos")) {
		float *oldOrigin;
		if (demo.chase.locked) {
			demoChasePoint_t *point = chasePointSynch( demo.play.time );
			if (!point || point->time != demo.play.time || demo.play.fraction ) {
				Com_Printf("Can't change position when not synched to a point\n");
				return;
			}
			oldOrigin = point->origin;
		} else {
			oldOrigin = demo.chase.origin;
		}
		demoCommandValue( CG_Argv(2), oldOrigin );
		demoCommandValue( CG_Argv(3), oldOrigin+1 );
		demoCommandValue( CG_Argv(4), oldOrigin+2 );
	} else if (!Q_stricmp(cmd, "angles")) {
		float *oldAngles;
		if (demo.chase.locked) {
			demoChasePoint_t *point = chasePointSynch( demo.play.time );
			if (!point || point->time != demo.play.time || demo.play.fraction ) {
				Com_Printf("Can't change angles when not synched to a point\n");
				return;
			}
			oldAngles = point->angles;
		} else {
			oldAngles = demo.chase.angles;
		}
		demoCommandValue( CG_Argv(2), oldAngles );
		demoCommandValue( CG_Argv(3), oldAngles+1 );
		demoCommandValue( CG_Argv(4), oldAngles+2 );
		AnglesNormalize180( oldAngles );
	} else if (!Q_stricmp(cmd, "dist") || !Q_stricmp(cmd, "distance")) {
		if (demo.chase.locked) {
			demoChasePoint_t *point = chasePointSynch( demo.play.time );
			if (!point || point->time != demo.play.time || demo.play.fraction ) {
				Com_Printf("Can't change distance when not synched to a point\n");
				return;
			}
			demoCommandValue( CG_Argv(2), &point->distance );
		} else {
			demoCommandValue( CG_Argv(2), &demo.chase.distance );
		}
	} else if (!Q_stricmp(cmd, "shift")) {
		int shift = 1000 * atof(CG_Argv(2));
		demo.chase.shiftWarn = 0;
		if (chasePointShift( shift )) {
			CG_DemosAddLog( "Shifted %d.%03d seconds", shift / 1000, shift % 1000);
		}
	} else {
		Com_Printf("chase usage:\n" );
		Com_Printf("chase add/del, add/del a chase point at current viewpoint and time.\n" );
		Com_Printf("chase clear, clear all chase points. (Use with caution...)\n" );
		Com_Printf("chase lock, lock the view to chase point or free moving.\n" );
		Com_Printf("chase shift time, move time indexes of chase points by certain amount.\n" );
		Com_Printf("chase next/prev, go to time index of next or previous point.\n" );
		Com_Printf("chase start/end, current time with be set as start/end of selection.\n" );
		Com_Printf("chase pos/angles (a)0 (a)0 (a)0, Directly control position/angles, optional a before number to add to current value\n" );
	}
}


void demoMoveChaseDirect( void ) {
	if (demo.chase.locked)
		return;
	if (!(demo.oldcmd.buttons & BUTTON_ATTACK)) {
		VectorClear( demo.chase.velocity );
	}
	VectorAdd( demo.chase.angles, demo.cmdDeltaAngles, demo.chase.angles );
	AnglesNormalize180( demo.chase.angles );
	demoMovePoint( demo.chase.origin, demo.chase.velocity, demo.chase.angles );
}

void demoMoveChase(void) {
	float *origin;
	float *distance;
	float *angles;
	int	*target;

	if (demo.chase.locked) {
		demoChasePoint_t * point = chasePointSynch( demo.play.time );
		if ( point && point->time == demo.play.time && !demo.play.fraction) {
			origin = point->origin;
			angles = point->angles;
			distance = &point->distance;
			target = &point->target;
		} else
			return;
	} else {
		origin = demo.chase.origin;
		angles = demo.chase.angles;
		distance = &demo.chase.distance;
		target = &demo.chase.target;
	}

	if (demo.cmd.buttons & BUTTON_ATTACK) {
		/* First clear some related values */
		if (!(demo.oldcmd.buttons & BUTTON_ATTACK)) {
			VectorClear( demo.chase.velocity );
		}
		if (demo.cmd.buttons & BUTTON_ATTACK2 && !(demo.oldcmd.buttons & BUTTON_ATTACK2)) {
			if (*target>=0) {
				CG_DemosAddLog("Cleared target %d", *target );
				*target = -1;
			} else {
				vec3_t forward;
				AngleVectors( demo.viewAngles, forward, 0, 0 );
				*target = demoHitEntities( demo.viewOrigin, forward );
				if (*target >= 0) {
					vec3_t targetAngles;
					chaseEntityOrigin( &cg_entities[*target], origin);
					*distance = VectorDistance( demo.viewOrigin, origin );
					VectorSubtract( origin, demo.viewOrigin, forward );
					vectoangles( forward, targetAngles );
					angles[PITCH] = targetAngles[PITCH];
					angles[YAW] = targetAngles[YAW];
					CG_DemosAddLog("Target set to %d", *target );
				} else {
					CG_DemosAddLog("Unable to hit any target" );
				}
			}
		}
		VectorAdd( angles, demo.cmdDeltaAngles, angles );
		AnglesNormalize180( angles );
		demoMovePoint( origin, demo.chase.velocity, angles );
	} else if (demo.cmd.buttons & BUTTON_NEGATIVE) {
		angles[ROLL] -= demo.cmdDeltaAngles[YAW];
		AnglesNormalize180( angles );
	} else if (demo.cmd.buttons & BUTTON_ATTACK2 && !(demo.oldcmd.buttons & BUTTON_ATTACK2)) {
		VectorCopy( demo.viewOrigin, origin );
		VectorCopy( demo.viewAngles, angles );
		CG_DemosAddLog("Chase position synced to current view");
	} else if (demo.cmd.rightmove < 0) {
		int shift;
		demo.chase.timeShift -= demo.serverDeltaTime * 1000 * demo.cmdDeltaAngles[YAW];
		shift = (int)demo.chase.timeShift;
		demo.chase.timeShift -= shift;
		chasePointShift( shift );
	} else if (demo.cmd.upmove > 0) {
		*distance -= demo.serverDeltaTime * 700 * demo.cmdDeltaAngles[YAW];
		if (*distance < 0)
			*distance = 0;
	}
}
