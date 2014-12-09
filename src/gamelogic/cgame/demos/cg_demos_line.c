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

#define SPEED_SHIFT 14

static demoLinePoint_t *linePointAlloc(void) {
	demoLinePoint_t * point = (demoLinePoint_t *)malloc(sizeof (demoLinePoint_t));
	if (!point)
		CG_Error("Out of memory");
	memset( point, 0, sizeof( demoLinePoint_t ));
	return point;
}

static void linePointFree(demoLinePoint_t *point) {
	if (point->prev) 
		point->prev->next = point->next;
	else 
		demo.line.points = point->next;
	if (point->next ) 
		point->next->prev = point->prev;
	free( point );
}

demoLinePoint_t *linePointSynch(int playTime ) {
	demoLinePoint_t *point = demo.line.points;
	if (!point)
		return 0;
	for( ;point->next && point->next->time <= playTime; point = point->next);
	return point;
}

static qboolean linePointAdd(int playTime, int demoTime ) {
	demoLinePoint_t *point = linePointSynch( playTime );
	demoLinePoint_t *newPoint;
	if (!point || point->time > playTime) {
		if (demo.line.points && demo.line.points->demoTime < demoTime)
			return qfalse;
		newPoint = linePointAlloc();
		newPoint->next = demo.line.points;
		if (demo.line.points)
			demo.line.points->prev = newPoint;
		demo.line.points = newPoint;
		newPoint->prev = 0;
	} else if (point->time == playTime) {
		newPoint = point;
	} else {
		newPoint = linePointAlloc();
		newPoint->prev = point;
		newPoint->next = point->next;
		if (point->next)
			point->next->prev = newPoint;
		point->next = newPoint;
	}
	if (newPoint->prev && newPoint->prev->demoTime > demoTime || 
		newPoint->next && newPoint->next->demoTime < demoTime) 
	{
		linePointFree( newPoint );
		return qfalse;
	}
	newPoint->time = playTime;
	newPoint->demoTime = demoTime;
	return qtrue;
}

static qboolean linePointDel( int playTime ) {
	demoLinePoint_t *point = linePointSynch( playTime );

	if (!point || !point->time == playTime)
		return qfalse;
	linePointFree( point );
	return qtrue;
}

static void lineClear( void ) {
	demoLinePoint_t *next, *point = demo.line.points;
	while ( point ) {
		next = point->next;
		linePointFree( point );
		point = next;
	}
}


static void lineInterpolate( int playTime, float playTimeFraction, int *demoTime, float *demoTimeFraction, float *demoSpeed ) {
	vec3_t dx, dy;
	demoLinePoint_t *point = linePointSynch( playTime );
	if (!point || !point->next) {
		int calcTimeLow, calcTimeHigh;
		int speed = (1 << SPEED_SHIFT) * demo.line.speed;
		if (point) 
			playTime -= point->time;
		calcTimeHigh = (playTime >> 16) * speed;
		calcTimeLow = (playTime & 0xffff) * speed;
		calcTimeLow += playTimeFraction * speed;
		*demoTime = (calcTimeHigh << (16-SPEED_SHIFT)) + (calcTimeLow >> SPEED_SHIFT);
		*demoTimeFraction = (float)(calcTimeLow & ((1 << SPEED_SHIFT) -1)) / (1 << SPEED_SHIFT);
		if (point)
			*demoTime += point->demoTime;
		*demoTime += demo.line.offset;
		*demoSpeed = demo.line.speed;
		return;
	}
	dx[1] = point->next->time - point->time;
	dy[1] = point->next->demoTime - point->demoTime;
	if (point->prev) {
		dx[0] = point->time - point->prev->time;
		dy[0] = point->demoTime - point->prev->demoTime;
	} else {
		dx[0] = dx[1];
		dy[0] = dy[1];
	}
	if (point->next->next) {
		dx[2] = point->next->next->time - point->next->time;;
		dy[2] = point->next->next->demoTime - point->next->demoTime;;
	} else {
		dx[2] = dx[1];
		dy[2] = dy[1];
	}
	*demoTimeFraction = dsplineCalc( (playTime - point->time) + playTimeFraction, dx, dy, demoSpeed );
	*demoTime = (int)*demoTimeFraction;
	*demoTimeFraction -= *demoTime;
	*demoTime += point->demoTime;
}


void lineAt(int playTime, float playTimeFraction, int *demoTime, float *demoTimeFraction, float *demoSpeed ) {
	if (!demo.line.locked) {
		int calcTimeLow, calcTimeHigh;
		int speed = (1 << SPEED_SHIFT) * demo.line.speed;

		calcTimeHigh = (playTime >> 16) * speed;
		calcTimeLow = (playTime & 0xffff) * speed;
		calcTimeLow += playTimeFraction * speed;
		*demoTime = (calcTimeHigh << (16-SPEED_SHIFT)) + (calcTimeLow >> SPEED_SHIFT);
		*demoTimeFraction = (float)(calcTimeLow & ((1 << SPEED_SHIFT) -1)) / (1 << SPEED_SHIFT);
		*demoTime += demo.line.offset;
		*demoSpeed = demo.line.speed;
	} else {
		lineInterpolate( playTime, playTimeFraction, demoTime, demoTimeFraction, demoSpeed );
	}
	if (*demoTime < 0 || *demoTimeFraction < 0) {
		*demoTimeFraction = 0;
		*demoTime = 0;
	}
}

static int linePointShift( qboolean demoTime, int shift ) {
	demoLinePoint_t *startPoint, *endPoint;
	if ( demo.line.start != demo.line.end ) {
		if (demo.line.start > demo.line.end )
			return 0;
		startPoint = linePointSynch( demo.line.start );
		endPoint = linePointSynch( demo.line.end );
		if (!startPoint || !endPoint)
			return 0;
		if (startPoint->time != demo.line.start || endPoint->time != demo.line.end) {
			if (demo.line.shiftWarn < demo.serverTime) {
				CG_DemosAddLog( "Failed to do shift, selection doesn't match points");
				demo.line.shiftWarn = demo.serverTime + 1000;
			}
			return 0;
		}
	} else {
		startPoint = endPoint = linePointSynch( demo.play.time );
		if (!startPoint)
			return 0;
		if (startPoint->time != demo.play.time) {
			if (demo.line.shiftWarn < demo.serverTime) {
				CG_DemosAddLog( "Failed to do shift, no point at current time");
				demo.line.shiftWarn = demo.serverTime + 1000;
			}
			return 0;
		}
	}

	if (!shift)
		return 0;

	if (shift < 0) {
		if (!startPoint->prev) {
			if (!demoTime && startPoint->time + shift < 0)
				shift = -startPoint->time;
		} else {
			int delta;
			if ( demoTime )
				delta = (startPoint->prev->demoTime - startPoint->demoTime );
			else
				delta = (startPoint->prev->time - startPoint->time );
			if (shift <= delta)
				shift = delta + 1;
		}
	} else {
		if (endPoint->next) {
			int delta;
			if ( demoTime )
				delta = (endPoint->next->demoTime - endPoint->demoTime );
			else
				delta = (endPoint->next->time - endPoint->time );
			if (shift >= delta)
				shift = delta - 1;
		}
	}
	while (startPoint) {
		if (demoTime)
			startPoint->demoTime += shift;
		else
			startPoint->time += shift;
		if (startPoint == endPoint)
			break;
		startPoint = startPoint->next;
	}
	if ( !demoTime ) {
		if ( demo.line.start != demo.line.end ) {
			demo.line.start += shift;
			demo.line.end += shift;
		} else {
			demo.play.time += shift;
		}
	}
	return shift;
}



void demoMoveLine(void) {
	if (usercmdButtonPressed(demo.cmd.buttons, BUTTON_ATTACK)) {
		switch (demo.viewType) {
		case viewCamera:
			cameraMoveDirect();
			break;
		case viewChase:
			demoMoveChaseDirect();
			break;
		}
	} else if (demo.cmd.rightmove < 0) {
		int shift;
		demo.line.timeShift -= demo.serverDeltaTime * 1000 * demo.cmdDeltaAngles[YAW];
		shift = (int)demo.line.timeShift;
		demo.line.timeShift -= shift;
		linePointShift( qfalse, shift );
	} else if (demo.cmd.rightmove > 0) {
		int shift;
		demo.line.timeShift -= demo.serverDeltaTime * 1000 * demo.cmdDeltaAngles[YAW];
		shift = (int)demo.line.timeShift;
		demo.line.timeShift -= shift;
		linePointShift( qtrue, shift );
	} 
}

typedef struct  {
	int time, demoTime;
	qboolean hasTime, hasDemoTime;
} ParseLinePoint_t;

static qboolean lineParseTime( BG_XMLParse_t *parse,const char *line, void *data) {
	ParseLinePoint_t *point= (ParseLinePoint_t *)data;
	if (!line[0]) 
		return qfalse;
	point->time = atoi( line );
	point->hasTime = qtrue;
	return qtrue;
}
static qboolean lineParseDemoTime( BG_XMLParse_t *parse,const char *line, void *data) {
	ParseLinePoint_t *point= (ParseLinePoint_t *)data;
	if (!line[0]) 
		return qfalse;
	point->demoTime = atoi( line );
	point->hasDemoTime = qtrue;
	return qtrue;
}
static qboolean lineParsePoint( BG_XMLParse_t *parse,const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	ParseLinePoint_t pointLoad;
	static BG_XMLParseBlock_t lineParseBlock[] = {
		{"time", 0, lineParseTime},
		{"demotime", 0, lineParseDemoTime},
		{0, 0, 0}
	};
	memset( &pointLoad, 0, sizeof( pointLoad ));
	if (!BG_XMLParse( parse, fromBlock, lineParseBlock, &pointLoad )) {
		return qfalse;
	}
	if (!pointLoad.hasTime || !pointLoad.hasDemoTime ) 
		return BG_XMLError(parse, "Missing section in line point");
	linePointAdd( pointLoad.time, pointLoad.demoTime  );
	return qtrue;
}
static qboolean lineParseOffset( BG_XMLParse_t *parse,const char *line, void *data) {
	ParseLinePoint_t *point= (ParseLinePoint_t *)data;
	if (!line[0]) 
		return qfalse;
	demo.line.offset = atoi( line );
	return qtrue;
}
static qboolean lineParseSpeed( BG_XMLParse_t *parse,const char *line, void *data) {
	ParseLinePoint_t *point= (ParseLinePoint_t *)data;
	if (!line[0]) 
		return qfalse;
	demo.line.speed = atof( line );
	return qtrue;
}
static qboolean lineParseLocked( BG_XMLParse_t *parse,const char *line, void *data) {
	ParseLinePoint_t *point= (ParseLinePoint_t *)data;
	if (!line[0]) 
		return qfalse;
	demo.line.locked = atoi( line );
	return qtrue;
}
qboolean lineParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	static BG_XMLParseBlock_t lineParseBlock[] = {
		{"point",	lineParsePoint,	0},
		{"offset",	0,				lineParseOffset },
		{"speed",	0,				lineParseSpeed },
		{"locked",	0,				lineParseLocked },

		{0, 0, 0}
	};

	lineClear();
	if (!BG_XMLParse( parse, fromBlock, lineParseBlock, data ))
		return qfalse;
	return qtrue;
}

void lineSave( fileHandle_t fileHandle ) {
	demoLinePoint_t *point;

	point = demo.line.points;
	demoSaveLine( fileHandle, "<line>\n" );
	demoSaveLine( fileHandle, "\t<offset>%d</offset>\n", demo.line.offset );
	demoSaveLine( fileHandle, "\t<speed>%9.4f</speed>\n", demo.line.speed );
	demoSaveLine( fileHandle, "\t<locked>%d</locked>\n", demo.line.locked );
	while (point) {
		demoSaveLine( fileHandle, "\t<point>\n");
		demoSaveLine( fileHandle, "\t\t<time>%10d</time>\n", point->time );
		demoSaveLine( fileHandle, "\t\t<demotime>%10d</demotime>\n", point->demoTime );
		demoSaveLine( fileHandle, "\t</point>\n");
		point = point->next;
	}
	demoSaveLine( fileHandle, "</line>\n" );
}


void demoLineCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "lock")) {
		demo.line.locked = !demo.line.locked;
		if (demo.line.locked) 
			CG_DemosAddLog("Timeline locked");
		else 
			CG_DemosAddLog("Timeline unlocked");
	} else if (!Q_stricmp(cmd, "add")) {
		if (linePointAdd( demo.play.time, demo.line.time )) {
			CG_DemosAddLog("Added timeline point");
		} else {
			CG_DemosAddLog("Failed to add timeline point");
		}
	} else if (!Q_stricmp(cmd, "del")) {
		if (linePointDel( demo.play.time)) {
			CG_DemosAddLog("Deleted timeline point");
		} else {
			CG_DemosAddLog("Failed to delete timeline point");
		}
	} else if (!Q_stricmp(cmd, "clear")) {
		lineClear();
	} else if (!Q_stricmp(cmd, "sync")) {
		demo.line.offset = demo.play.time;
		demo.line.offset -= demo.line.offset * demo.line.speed;
		CG_DemosAddLog("Timeline synched", demo.line.speed);
	} else if (!Q_stricmp(cmd, "speed")) {
		cmd = CG_Argv(2);
		if (cmd[0]) 
			demo.line.speed = atof( cmd );
		CG_DemosAddLog("Timeline speed %.03f", demo.line.speed);
	} else if (!Q_stricmp(cmd, "offset")) {
		cmd = CG_Argv(2);
		if (cmd[0]) 
			demo.line.offset = 1000 * atof( cmd );
		CG_DemosAddLog("Timeline offset %.03f", demo.line.offset / 1000.0f);
	} else if (!Q_stricmp(cmd, "next")) {
		demoLinePoint_t *point = linePointSynch( demo.play.time );
		if (!point)
			return;
		if (point->next)
			point = point->next;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	} else if (!Q_stricmp(cmd, "prev")) {
		demoLinePoint_t *point = linePointSynch( demo.play.time );
		if (!point)
			return;
		if (point->prev)
			point = point->prev;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	} else if (!Q_stricmp(cmd, "start")) {
		if ( demo.seekEnabled ) {
			demo.play.time = demo.capture.start;
			demo.play.fraction = 0;
			CG_DemosAddLog( "Seek to capture start at %s", demoTimeString( demo.play.time ));
		} else {
			demo.capture.start = demo.play.time;
			CG_DemosAddLog( "capture start set at %s", demoTimeString( demo.play.time ));
		}
	} else if (!Q_stricmp(cmd, "end")) {
		if ( demo.seekEnabled ) {
			demo.play.time = demo.capture.end;
			demo.play.fraction = 0;
			CG_DemosAddLog( "Seek to capture end at %s", demoTimeString( demo.play.time ));
		} else {
			demo.capture.end = demo.play.time;
			CG_DemosAddLog( "capture end set at %s", demoTimeString( demo.play.time ) );
		}	
	} else {
		Com_Printf("line usage:\n" );
		Com_Printf("line lock, lock timeline to use the keypoints\n");
		Com_Printf("line add, Add timeline point for this demo/line time como\n");
		Com_Printf("line del, Delete timeline keypoint\n" );
		Com_Printf("line clear, Clear all keypoints\n" );
		Com_Printf("line speed value, Demo speed when timeline unlocked\n" );
		Com_Printf("line offset seconds, Shift the demotime along the main time\n" );
		Com_Printf("start/end, set start/end parts of capture range\n");
	}
}
