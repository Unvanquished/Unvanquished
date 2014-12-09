// cg_demos_dof.c -- depth of field mme feature

#include "cg_demos.h" 

#define DOF_MAX_FOCUS	4096.0f
#define DOF_MAX_RADIUS	64.0f
#define DOF_MAX_LINES	42

demoDofPoint_t *dofPointSynch( int time ) {
	demoDofPoint_t *point = demo.dof.points;
	if (!point)
		return 0;
	for( ;point->next && point->next->time <= time; point = point->next);
	return point;
}

static demoDofPoint_t *dofPointAlloc(void) {
	demoDofPoint_t * point = (demoDofPoint_t *)malloc(sizeof (demoDofPoint_t));
	if (!point)
		CG_Error("Out of memory");
	memset( point, 0, sizeof( demoDofPoint_t ));
	return point;
}

static void dofPointFree(demoDofPoint_t *point) {
	if (point->prev) 
		point->prev->next = point->next;
	else 
		demo.dof.points = point->next;
	if (point->next ) 
		point->next->prev = point->prev;
	free( point );
}

static qboolean dofPointAdd( int time, float focus, float radius ) {
	demoDofPoint_t *point = dofPointSynch( time );
	demoDofPoint_t *newPoint;
	if (!point || point->time > time) {
		newPoint = dofPointAlloc();
		newPoint->next = demo.dof.points;
		if (demo.dof.points)
			demo.dof.points->prev = newPoint;
		demo.dof.points = newPoint;
		newPoint->prev = 0;
	} else if (point->time == time) {
		newPoint = point;
	} else {
		newPoint = dofPointAlloc();
		newPoint->prev = point;
		newPoint->next = point->next;
		if (point->next)
			point->next->prev = newPoint;
		point->next = newPoint;
	}
	newPoint->time = time;
	newPoint->focus = focus;
	newPoint->radius = radius;
	return qtrue;
}

static qboolean dofPointDel( int time ) {
	demoDofPoint_t *point = dofPointSynch( time );
	if (!point)
		return qfalse;
	if (point->time != time)
		return qfalse;
	dofPointFree( point );
	return qtrue;
}

static void dofClear( void ) {
	demoDofPoint_t *next, *point = demo.dof.points;
	while ( point ) {
		next = point->next;
		dofPointFree( point );
		point = next;
	}
}

static int dofPointShift( int shift ) {
	demoDofPoint_t *startPoint, *endPoint;
	if ( demo.dof.start != demo.dof.end ) {
		if (demo.dof.start > demo.dof.end )
			return 0;
		startPoint = dofPointSynch( demo.dof.start );
		endPoint = dofPointSynch( demo.dof.end );
		if (!startPoint || !endPoint)
			return 0;
		if (startPoint->time != demo.dof.start || endPoint->time != demo.dof.end) {
			if (demo.dof.shiftWarn < demo.serverTime) {
				CG_DemosAddLog( "Failed to do shift, selection doesn't match points");
				demo.dof.shiftWarn = demo.serverTime + 1000;
			}
			return 0;
		}
	} else {
		startPoint = endPoint = dofPointSynch( demo.play.time );
		if (!startPoint)
			return 0;
		if (startPoint->time != demo.play.time) {
			if (demo.dof.shiftWarn < demo.serverTime) {
				CG_DemosAddLog( "Failed to do shift, no point at current time");
				demo.dof.shiftWarn = demo.serverTime + 1000;
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
	if ( demo.dof.start != demo.dof.end ) {
		demo.dof.start += shift;
		demo.dof.end += shift;
	} else {
		demo.play.time += shift;
	}
	return shift;
}

static void dofPointControl( demoDofPoint_t *point, int times[4], float focuses[4], float radiuses[4]) {
	if (!point->next)
		return;
	
	times[1] = point->time;
	focuses[1] = point->focus;
	radiuses[1] = point->radius;

	times[2] = point->next->time;
	focuses[2] = point->next->focus;
	radiuses[2] = point->next->radius;
	if (!point->prev) {
		times[0] = times[1] - (times[2] - times[1]);
		focuses[0] = focuses[1] - (focuses[2] - focuses[1]);
		radiuses[0] = radiuses[1] - (radiuses[2] - radiuses[1]);
	} else {
		times[0] = point->prev->time;
		focuses[0] = point->prev->focus;
		radiuses[0] = point->prev->radius;
	}
	if (!point->next->next) {
		times[3] = times[2] + (times[2] - times[1]);
		focuses[3] = focuses[2] + (focuses[2] - focuses[1]);
		radiuses[3] = radiuses[2] + (radiuses[2] - radiuses[1]);
	} else {
		times[3] = point->next->next->time;
		focuses[3] = point->next->next->focus;
		radiuses[3] = point->next->next->radius;
	}
}

static void dofInterpolate( int time, float timeFraction, float *focus, float *radius ) {
	float	lerp, focuses[4], radiuses[4];
	int		times[4];
	demoDofPoint_t *point = dofPointSynch( time );

	if (!point->next) {
		*focus = point->focus;
		*radius = point->radius;
		return;
	}
	dofPointControl( point, times, focuses, radiuses );

	lerp = ((time - point->time) + timeFraction) / (point->next->time - point->time);
	VectorTimeSpline( lerp, times, (float *)focuses, focus,1 );
	VectorTimeSpline( lerp, times, (float *)radiuses, radius,1 );
}

static void dofDrawVerticalLine(vec3_t centre, vec3_t axis[3], float scale, float focus, vec4_t color) {
	vec3_t start, end, newCentre, up, middle;
	polyVert_t verts[4];

	demoDrawSetupVerts( verts, color );
	
	VectorMA(centre, 8096.0f, axis[2], start);
	VectorMA(centre, -8096.0f, axis[2], end);

	VectorScale( start, 0.5, middle ) ;
	VectorMA( middle, 0.5, end, middle );

	VectorMA( centre, -focus, axis[0], newCentre );
	GetPerpendicularViewVector( newCentre, start, end, up );
	VectorMA( start, scale, up, verts[0].xyz);
	VectorMA( start, -scale, up, verts[1].xyz);
	VectorMA( end, -scale, up, verts[2].xyz);
	VectorMA( end, scale, up, verts[3].xyz);
	trap_R_AddPolyToScene( demo.media.additiveWhiteShader, 4, verts );
}
static void dofDrawHorizontalLine(vec3_t centre, vec3_t axis[3], float scale, float focus, vec4_t color) {
	vec3_t start, end, newCentre, up, middle;
	polyVert_t verts[4];

	demoDrawSetupVerts( verts, color );
	
	VectorMA(centre, 8096.0f, axis[1], start);
	VectorMA(centre, -8096.0f, axis[1], end);
	
	VectorScale( start, 0.5, middle ) ;
	VectorMA( middle, 0.5, end, middle );

	VectorMA( centre, -focus, axis[0], newCentre );
	GetPerpendicularViewVector( newCentre, start, end, up );
	VectorMA( start, scale, up, verts[0].xyz);
	VectorMA( start, -scale, up, verts[1].xyz);
	VectorMA( end, -scale, up, verts[2].xyz);
	VectorMA( end, scale, up, verts[3].xyz);
	trap_R_AddPolyToScene( demo.media.additiveWhiteShader, 4, verts );
}

qboolean dofWorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y) {
    vec3_t trans;
    vec_t xc, yc;
    vec_t px, py;
    vec_t z;

    px = tan(cg.refdef.fov_x * (M_PI / 360) );
    py = tan(cg.refdef.fov_y * (M_PI / 360) );
	
    VectorSubtract(worldCoord, cg.refdef.vieworg, trans);
   
    xc = SCREEN_WIDTH / 2.0f;
    yc = SCREEN_HEIGHT / 2.0f;
    
	// z = how far is the object in our forward direction
    z = DotProduct(trans, cg.refdef.viewaxis[0]);
    if (z <= 0.001)
        return qfalse;

    *x = xc - DotProduct(trans, cg.refdef.viewaxis[1])*xc/(z*px);
    *y = yc - DotProduct(trans, cg.refdef.viewaxis[2])*yc/(z*py);

    return qtrue;
}

static void dofDrawPlane(vec3_t origin, vec3_t angles, float focus, float radius) {
	float i;
	int lines;
	const float step = DOF_MAX_RADIUS + 2.0f * DOF_MAX_RADIUS; // 2nd DOF_MAX_RADIUS is padding
	vec3_t axis[3], centre;
	
	AnglesNormalize180( angles );
	AnglesToAxis(angles, axis);
	VectorMA(origin, focus, axis[0], centre);
//	RotateAroundDirection(axis, 45.0f);
	
	dofDrawVerticalLine(centre, axis, radius, focus, colorRed);	
	for (i = step, lines = 0;; i += step, lines++) {
		float x, y;
		vec3_t newCentre;
		VectorMA(centre, i, axis[1], newCentre);
		dofWorldCoordToScreenCoordFloat(newCentre, &x, &y);
		if (x < 0.0f - DOF_MAX_RADIUS)
			break;
		if (lines > DOF_MAX_LINES)
			break;
		dofDrawVerticalLine(newCentre, axis, radius, focus, colorRed);
	}
	for (i = -step, lines = 0;; i -= step, lines++) {
		float x, y;
		vec3_t newCentre;
		VectorMA(centre, i, axis[1], newCentre);
		dofWorldCoordToScreenCoordFloat(newCentre, &x, &y);
		if (x > (float)SCREEN_WIDTH + DOF_MAX_RADIUS)
			break;
		if (lines > DOF_MAX_LINES)
			break;
		dofDrawVerticalLine(newCentre, axis, radius, focus, colorRed);
	}
	
	dofDrawHorizontalLine(centre, axis, radius, focus, colorRed);	
	for (i = step, lines = 0;; i += step, lines++) {
		float x, y;
		vec3_t newCentre;
		VectorMA(centre, i, axis[2], newCentre);
		dofWorldCoordToScreenCoordFloat(newCentre, &x, &y);
		if (y < 0.0f - DOF_MAX_RADIUS)
			break;
		if (lines > DOF_MAX_LINES)
			break;
		dofDrawHorizontalLine(newCentre, axis, radius, focus, colorRed);
	}
	for (i = -step, lines = 0;; i -= step, lines++) {
		float x, y;
		vec3_t newCentre;
		VectorMA(centre, i, axis[2], newCentre);
		dofWorldCoordToScreenCoordFloat(newCentre, &x, &y);
		if (y > (float)SCREEN_HEIGHT + DOF_MAX_RADIUS)
			break;
		if (lines > DOF_MAX_LINES)
			break;
		dofDrawHorizontalLine(newCentre, axis, radius, focus, colorRed);
	}
}

void dofUpdate( int time, float timeFraction ) {
	if (demo.dof.locked && demo.dof.points ) {
		dofInterpolate( time, timeFraction, &demo.dof.focus, &demo.dof.radius );
	}
}

void dofDraw( int time, float timeFraction ) {
	centity_t *targetCent;
	float focus, radius;
	if (demo.dof.target >= 0 && demo.dof.focus < 0.001f)
		return;
	if (demo.dof.locked && demo.dof.points ) {
		dofInterpolate( time, timeFraction, &focus, &radius );
		if (demo.dof.target >= 0) // we can only animate radius when focus is locked on a target
			focus = demo.dof.focus;
	} else {
		focus = demo.dof.focus;
		radius = demo.dof.radius;
	}
	dofDrawPlane(demo.viewOrigin, demo.viewAngles, focus, radius);
	/* Draw a box around an optional target */
	targetCent = demoTargetEntity( demo.dof.target );
	if (targetCent) {
		vec3_t container;
		demoCentityBoxSize( targetCent, container );
		demoDrawBox( targetCent->lerpOrigin, container, colorWhite );
	}
}

void dofMove(void) {
	float *focus, *radius;
	demoDofPoint_t *point;

	if (!demo.dof.locked) {
		focus = &demo.dof.focus;
		radius = &demo.dof.radius;
		point = 0;
	} else {
		point = dofPointSynch( demo.play.time );
		if (!point || point->time != demo.play.time || demo.play.fraction )
			return;
		focus =  &point->focus;
		radius =  &point->radius;
	}
	
	if (demo.cmd.forwardmove > 0 && !(usercmdButtonPressed(demo.cmd.buttons, BUTTON_ATTACK))) {
		focus[0] -= demo.cmdDeltaAngles[YAW];
		if (focus[0] < 0.001f)
			focus[0] = 0.001f;
		else if (focus[0] > DOF_MAX_FOCUS)
			focus[0] = DOF_MAX_FOCUS;
	} else if (demo.cmd.upmove > 0 && !(usercmdButtonPressed(demo.cmd.buttons, BUTTON_ATTACK))) {
		radius[0] -= demo.cmdDeltaAngles[YAW];
		if (radius[0] < 0.0f)
			radius[0] = 0.0f;
		else if (radius[0] > DOF_MAX_RADIUS)
			radius[0] = DOF_MAX_RADIUS;
	}
}

typedef struct  {
	int		time;
	float	focus, radius;
	qboolean hasTime, hasFocus, hasRadius;
} parseDofPoint_t;

static qboolean dofParseTime( BG_XMLParse_t *parse,const char *line, void *data) {
	parseDofPoint_t *point= (parseDofPoint_t *)data;

	point->time = atoi( line );
	point->hasTime = qtrue;
	return qtrue;
}
static qboolean dofParseFocus( BG_XMLParse_t *parse,const char *line, void *data) {
	parseDofPoint_t *point= (parseDofPoint_t *)data;

	point->focus = atof( line );
	point->hasFocus = qtrue;
	return qtrue;
}
static qboolean dofParseRadius( BG_XMLParse_t *parse,const char *line, void *data) {
	parseDofPoint_t *point= (parseDofPoint_t *)data;

	point->radius = atof( line );
	point->hasRadius = qtrue;
	return qtrue;
}
static qboolean dofParsePoint( BG_XMLParse_t *parse,const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	parseDofPoint_t pointLoad;
	static BG_XMLParseBlock_t dofParseBlock[] = {
		{"time",	0, dofParseTime },
		{"focus",	0, dofParseFocus },
		{"radius",	0, dofParseRadius },
		{0, 0, 0}
	};
	memset( &pointLoad, 0, sizeof( pointLoad ));
	if (!BG_XMLParse( parse, fromBlock, dofParseBlock, &pointLoad )) {
		return qfalse;
	}
	if (!pointLoad.hasTime || !pointLoad.hasFocus || !pointLoad.hasRadius ) 
		return BG_XMLError(parse, "Missing section in dof point");
	dofPointAdd( pointLoad.time, pointLoad.focus, pointLoad.radius );
	return qtrue;
}
static qboolean dofParseLocked( BG_XMLParse_t *parse,const char *line, void *data) {
	demo.dof.locked = atoi( line );
	return qtrue;
}
static qboolean dofParseTarget( BG_XMLParse_t *parse,const char *line, void *data) {
	demo.dof.target = atoi( line );
	return qtrue;
}

qboolean dofParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	static BG_XMLParseBlock_t dofParseBlock[] = {
		{"point",	dofParsePoint,	0 },
		{"locked",	0,				dofParseLocked },
		{"target",	0,				dofParseTarget },
		{0, 0, 0}
	};

	dofClear();
	if (!BG_XMLParse( parse, fromBlock, dofParseBlock, data ))
		return qfalse;

	return qtrue;
}

void dofSave( fileHandle_t fileHandle ) {
	demoDofPoint_t *point;

	point = demo.dof.points;
	demoSaveLine( fileHandle, "<dof>\n" );
	demoSaveLine( fileHandle, "\t<locked>%d</locked>\n", demo.dof.locked );
	demoSaveLine( fileHandle, "\t<target>%d</target>\n", demo.dof.target );
	while (point) {
		demoSaveLine( fileHandle, "\t<point>\n");
		demoSaveLine( fileHandle, "\t\t<time>%10d</time>\n", point->time );
		demoSaveLine( fileHandle, "\t\t<focus>%9.4f</focus>\n", point->focus );
		demoSaveLine( fileHandle, "\t\t<radius>%9.4f</radius>\n", point->radius );
		demoSaveLine( fileHandle, "\t</point>\n");
		point = point->next;
	}
	demoSaveLine( fileHandle, "</dof>\n" );
}

void demoDofCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "add")) {
		if (dofPointAdd( demo.play.time, demo.dof.focus, demo.dof.radius )) {
			CG_DemosAddLog( "Added a dof point");
		} else {
			CG_DemosAddLog( "Failed to add a dof point");
		}
	}  else if (!Q_stricmp(cmd, "del") || !Q_stricmp(cmd, "delete" )) { 
		if (dofPointDel( demo.play.time )) {
			CG_DemosAddLog( "Deleted a dof point");
		} else {
			CG_DemosAddLog( "Failed to delete a dof point");
		}
	} else if (!Q_stricmp(cmd, "start")) { 
		demo.dof.start = demo.play.time;
		if (demo.dof.start > demo.dof.end)
			demo.dof.start = demo.dof.end;
		if (demo.dof.end == demo.dof.start)
			CG_DemosAddLog( "Cleared dof selection");
		else
			CG_DemosAddLog( "Dof selection start at %d.%03d", demo.dof.start / 1000, demo.dof.start % 1000 );
	} else if (!Q_stricmp(cmd, "end")) { 
		demo.dof.end = demo.play.time;
		if (demo.dof.end < demo.dof.start)
			demo.dof.end = demo.dof.start;
		if (demo.dof.end == demo.dof.start)
			CG_DemosAddLog( "Cleared dof selection");
		else
			CG_DemosAddLog( "Dof selection end at %d.%03d", demo.dof.end / 1000, demo.dof.end % 1000 );
	} else if (!Q_stricmp(cmd, "next")) { 
		demoDofPoint_t *point = dofPointSynch( demo.play.time );
		if (!point)
			return;
		if (point->next)
			point = point->next;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	} else if (!Q_stricmp(cmd, "prev")) {
		demoDofPoint_t *point = dofPointSynch( demo.play.time );
		if (!point)
			return;
		if (point->prev)
			point = point->prev;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	} else if (!Q_stricmp(cmd, "lock")) {
		demo.dof.locked = !demo.dof.locked;
		if (demo.dof.locked) 
			CG_DemosAddLog("Depth of field locked");
		else 
			CG_DemosAddLog("Depth of field unlocked");
	} else if (!Q_stricmp(cmd, "target")) {
		if ( demo.dof.locked ) {
			CG_DemosAddLog("Can't target while locked" );
		}
		if ( usercmdButtonPressed(demo.cmd.buttons, BUTTON_ATTACK) ) {
			if (demo.dof.target>=0) {			
				CG_DemosAddLog("Cleared target %d", demo.dof.target );
				demo.dof.target = -1;
			} else {
				vec3_t forward;
				AngleVectors( demo.viewAngles, forward, 0, 0 );
				demo.dof.target = demoHitEntities( demo.viewOrigin, forward );
				if (demo.dof.target >= 0) {
					CG_DemosAddLog("Target set to %d", demo.dof.target );
				} else {
					CG_DemosAddLog("Unable to hit any target" );
				}
			}
		}
	} else if (!Q_stricmp(cmd, "shift")) {
		int shift = 1000 * atof(CG_Argv(2));
		demo.dof.shiftWarn = 0;
		if (dofPointShift( shift )) {
			CG_DemosAddLog( "Shifted %d.%03d seconds", shift / 1000, shift % 1000);
		}
	} else if (!Q_stricmp(cmd, "clear")) {
		dofClear( );
		CG_DemosAddLog( "Dof points cleared" );
	} else if (!Q_stricmp(cmd, "focus")) {
		float *oldFocus;
		if (demo.dof.locked) {
			demoDofPoint_t *point = dofPointSynch( demo.play.time );
			if (!point || point->time != demo.play.time || demo.play.fraction ) {
				Com_Printf("Can't change focus when not synched to a point\n");
				return;
			}
			oldFocus = &point->focus;
		} else {
			oldFocus = &demo.dof.focus;
		}
		demoCommandValue( CG_Argv(2), oldFocus );
		if (*oldFocus < 0.001f)
			*oldFocus = 0.001f;
		else if (*oldFocus > DOF_MAX_FOCUS)
			*oldFocus = DOF_MAX_FOCUS;
		CG_DemosAddLog( "Dof focus is set to %.4f", *oldFocus);
	} else if (!Q_stricmp(cmd, "radius")) {
		float *oldRadius;
		if (demo.dof.locked) {
			demoDofPoint_t *point = dofPointSynch( demo.play.time );
			if (!point || point->time != demo.play.time || demo.play.fraction ) {
				Com_Printf("Can't change radius when not synched to a point\n");
				return;
			}
			oldRadius = &point->radius;
		} else {
			oldRadius = &demo.dof.radius;
		}
		demoCommandValue( CG_Argv(2), oldRadius );
		if (*oldRadius < 0)
			*oldRadius = 0;
		else if (*oldRadius > DOF_MAX_RADIUS)
			*oldRadius = DOF_MAX_RADIUS;
		CG_DemosAddLog( "Dof radius is set to %.4f", *oldRadius);
	} else {
		Com_Printf("dof usage:\n" );
		Com_Printf("dof add/del, add/del a dof point at current viewpoint and time.\n" );
		Com_Printf("dof clear, clear all dof points. (Use with caution...)\n" );
		Com_Printf("dof lock, lock dof to use the keypoints.\n" );
		Com_Printf("dof shift time, move time indexes of dof points by certain amount.\n" );
		Com_Printf("dof next/prev, go to time index of next or previous point.\n" );
		Com_Printf("dof start/end, current time with be set as start/end of selection.\n" );
		Com_Printf("dof focus (a)0, Directly control focus, optional a before number to add to current value.\n" );
		Com_Printf("dof radius (a)0, Directly control radius, optional a before number to add to current value.\n" );
		Com_Printf("dof target, Clear/Set the target currently being focused at.\n" );
	}
}
