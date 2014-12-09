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

static demoScriptPoint_t *scriptPointAlloc(void) {
	demoScriptPoint_t * point = (demoScriptPoint_t *)malloc(sizeof (demoScriptPoint_t));
	if (!point)
		CG_Error("Out of memory");
	memset( point, 0, sizeof( demoScriptPoint_t ));
	return point;
}

static void scriptPointFree(demoScriptPoint_t *point) {
	if (point->prev) 
		point->prev->next = point->next;
	else 
		demo.script.points = point->next;
	if (point->next ) 
		point->next->prev = point->prev;
	free( point );
}

demoScriptPoint_t *scriptPointSynch( int time ) {
	demoScriptPoint_t *point = demo.script.points;
	if (!point)
		return 0;
	for( ;point->next && point->next->time <= time; point = point->next);
	return point;
}

static demoScriptPoint_t *scriptPointAdd( int time ) {
	demoScriptPoint_t *point = scriptPointSynch( time );
	demoScriptPoint_t *newPoint;
	if (!point || point->time > time) {
		newPoint = scriptPointAlloc();
		newPoint->next = demo.script.points;
		if (demo.script.points)
			demo.script.points->prev = newPoint;
		demo.script.points = newPoint;
		newPoint->prev = 0;
	} else if (point->time == time) {
		newPoint = point;
	} else {
		newPoint = scriptPointAlloc();
		newPoint->prev = point;
		newPoint->next = point->next;
		if (point->next)
			point->next->prev = newPoint;
		point->next = newPoint;
	}
	newPoint->time = time;
	return newPoint;
}

static qboolean scriptPointDel( int playTime ) {
	demoScriptPoint_t *point = scriptPointSynch( playTime );

	if (!point || !point->time == playTime)
		return qfalse;
	scriptPointFree( point );
	return qtrue;
}

static void scriptClear( void ) {
	demoScriptPoint_t *next, *point = demo.script.points;
	while ( point ) {
		next = point->next;
		scriptPointFree( point );
		point = next;
	}
}

typedef struct {
	char init[DEMO_SCRIPT_SIZE];
	char run[DEMO_SCRIPT_SIZE];
	int time;
	int	flags;
} parseScriptPoint_t;

static qboolean scriptParseRun( BG_XMLParse_t *parse,const char *script, void *data) {
	parseScriptPoint_t *point=(parseScriptPoint_t *)data;
	Q_strncpyz( point->run, script, sizeof( point->run ));
	return qtrue;
}
static qboolean scriptParseInit( BG_XMLParse_t *parse,const char *script, void *data) {
	parseScriptPoint_t *point=(parseScriptPoint_t *)data;
	Q_strncpyz( point->init, script, sizeof( point->init ));
	return qtrue;
}

static qboolean scriptParseTime( BG_XMLParse_t *parse,const char *script, void *data) {
	parseScriptPoint_t *point=(parseScriptPoint_t *)data;

	point->time = atoi( script );
	return qtrue;
}
static qboolean scriptParsePoint( BG_XMLParse_t *parse,const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	parseScriptPoint_t pointLoad;
	static BG_XMLParseBlock_t scriptParseBlock[] = {
		{"time", 0, scriptParseTime},
		{"run", 0, scriptParseRun},
		{"init", 0, scriptParseInit},
		{0, 0, 0}
	};
	memset( &pointLoad, 0, sizeof( pointLoad ));
	if (!BG_XMLParse( parse, fromBlock, scriptParseBlock, &pointLoad )) {
		return qfalse;
	}
	scriptPointAdd( pointLoad.time );
	return qtrue;
}
static qboolean scriptParseLocked( BG_XMLParse_t *parse,const char *script, void *data) {
	parseScriptPoint_t *point= (parseScriptPoint_t *)data;
	if (!script[0]) 
		return qfalse;
	demo.script.locked = atoi( script );
	return qtrue;
}
static qboolean scriptParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	static BG_XMLParseBlock_t scriptParseBlock[] = {
		{"point",	scriptParsePoint,	0},
		{"locked",	0,					scriptParseLocked },
		{0, 0, 0}
	};

	scriptClear();
	if (!BG_XMLParse( parse, fromBlock, scriptParseBlock, data ))
		return qfalse;
	return qtrue;
}

void scriptSave( fileHandle_t fileHandle ) {
	demoScriptPoint_t *point;

	point = demo.script.points;
	demoSaveLine( fileHandle, "<script>\n" );
	demoSaveLine( fileHandle, "\t<locked>%d</locked>\n", demo.script.locked );
	while (point) {
		demoSaveLine( fileHandle, "\t<point>\n");
		demoSaveLine( fileHandle, "\t\t<time>%10d</time>\n", point->time );
		demoSaveLine( fileHandle, "\t\t<run>%s</run>", point->run );
		demoSaveLine( fileHandle, "\t\t<init>%s</init>", point->init );
		demoSaveLine( fileHandle, "\t</point>\n");
		point = point->next;
	}
	demoSaveLine( fileHandle, "</script>\n" );
}

static void scriptExecRun( demoScriptPoint_t *point ) {
	trap_SendConsoleCommand( point->run );
}

static void scriptExecInit( demoScriptPoint_t *point ) {
	trap_SendConsoleCommand( point->init );
}

void scriptRun( qboolean hadSkip ) {
	if ( !demo.script.locked )
		return;
	if ( hadSkip ) {
		demoScriptPoint_t *skipPoint = demo.script.points;
		while ( skipPoint && skipPoint->time <= demo.play.time ) {
			scriptExecInit( skipPoint );
			skipPoint = skipPoint->next;
		}
	} else {
		demoScriptPoint_t *point = scriptPointSynch( demo.play.time );
		if ( point && demo.play.oldTime < point->time && demo.play.time >= point->time ) {
			scriptExecInit( point );
			scriptExecRun( point );
		}
	}
}

void demoScriptCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "lock")) {
		demo.script.locked = !demo.script.locked;
		if (demo.script.locked) 
			CG_DemosAddLog("script locked");
		else 
			CG_DemosAddLog("script unlocked");
	} else if (!Q_stricmp(cmd, "add")) {
		if (scriptPointAdd( demo.play.time )) {
			CG_DemosAddLog("Added script point");
		} else {
			CG_DemosAddLog("Failed to add script point");
		}
	} else if (!Q_stricmp(cmd, "del")) {
		if (scriptPointDel( demo.play.time)) {
			CG_DemosAddLog("Deleted script point");
		} else {
			CG_DemosAddLog("Failed to delete script point");
		}
	} else if (!Q_stricmp(cmd, "clear")) {
		scriptClear();
	} else if (!Q_stricmp(cmd, "next")) {
		demoScriptPoint_t *point = scriptPointSynch( demo.play.time );
		if (!point)
			return;
		if (point->next)
			point = point->next;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	} else if (!Q_stricmp(cmd, "prev")) {
		demoScriptPoint_t *point = scriptPointSynch( demo.play.time );
		if (!point)
			return;
		if (point->prev)
			point = point->prev;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	}
}
