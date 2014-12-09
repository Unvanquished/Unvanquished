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

#ifndef	__CG_DEMOS_H
#define	__CG_DEMOS_H

#include "../../shared/demos/bg_demos.h"
#include "../cg_local.h"
#include "cg_demos_math.h"

#define LOGLINES 8

typedef enum {
	editNone,
	editCamera,
	editChase,
	editLine,
	editDof,
	editScript,
	editLast,
} demoEditType_t;

typedef enum {
	viewCamera,
	viewChase,
	viewLast
} demoViewType_t;


typedef enum {
	findNone,
	findObituary,
} demofindType_t;

typedef struct demoLinePoint_s {
	struct			demoLinePoint_s *next, *prev;
	int				time, demoTime;
} demoLinePoint_t;

typedef struct demoListPoint_s {
	struct			demoListPoint_s *next, *prev;
	int				time;
	int				flags;
} demoListPoint_t;

typedef struct demoCameraPoint_s {
	struct			demoCameraPoint_s *next, *prev;
	vec3_t			origin, angles;
	float			fov;
	int				time, flags;
	float			len;
} demoCameraPoint_t;

typedef struct demoChasePoint_s {
	struct			demoChasePoint_s *next, *prev;
	vec_t			distance;
	vec3_t			angles;
	int				target;
	vec3_t			origin;
	int				time;
	float			len;
} demoChasePoint_t;

#define DEMO_SCRIPT_SIZE 80

typedef struct demoScriptPoint_s {
	struct			demoScriptPoint_s *next, *prev;
	int				time;
	char			init[DEMO_SCRIPT_SIZE];
	char			run[DEMO_SCRIPT_SIZE];
} demoScriptPoint_t;

typedef struct demoDofPoint_s {
	struct			demoDofPoint_s *next, *prev;
	float			focus, radius;
	int				time;
} demoDofPoint_t;

typedef struct {
	char lines[LOGLINES][1024];
	int	 times[LOGLINES];
	int	 lastline;
} demoLog_t;

typedef enum {
	captureNone,
	captureTGA,
	captureJPG,
	capturePNG,
	captureAVI
} demoCaptureType_t;

typedef struct demoMain_s {
	int				serverTime;
	float			serverDeltaTime;
	struct {
		int			start, end;
		qboolean	locked;
		float		speed;
		int			offset;
		float		timeShift;
		int			shiftWarn;
		int			time;
		demoLinePoint_t *points;
	} line;
	struct {
		int			start;
		float		range;
		int			index, total;
		int			lineDelay;
	} loop;
	struct {
		int			start, end;
		qboolean	locked;
		int			target;
		vec3_t		angles, origin, velocity;
		vec_t		distance;
		float		timeShift;
		int			shiftWarn;
		demoChasePoint_t *points;
		centity_t	*cent;
	} chase;
	struct {
		int			start, end;
		int			target, flags;
		int			shiftWarn;
		float		timeShift;
		float		fov;
		qboolean	locked;
		vec3_t		angles, origin, velocity;
		demoCameraPoint_t *points;
		posInterpolate_t	smoothPos;
		angleInterpolate_t	smoothAngles;
	} camera;
	struct {
		int			start, end;
		int			target;
		int			shiftWarn;
		float		timeShift;
		float		focus, radius;
		qboolean	locked;
		demoDofPoint_t *points;
	} dof;
	struct {
		int			start, end;
		qboolean	locked;
		float		timeShift;
		int			shiftWarn;
		demoScriptPoint_t *points;
	} script;
	struct {
		int			time;
		int			oldTime;
		int			lastTime;
		float		fraction;
		float		speed;
		qboolean	paused;
	} play;
	struct {
		float speed, acceleration, friction;
	} move;
	vec3_t			viewOrigin, viewAngles;
	demoViewType_t	viewType;
	vec_t			viewFov;
	int				viewTarget;
	float			viewFocus, viewFocusOld, viewRadius;
	demoEditType_t	editType;

	vec3_t		cmdDeltaAngles;
	usercmd_t	cmd, oldcmd;
	int			nextForwardTime, nextRightTime, nextUpTime;
	int			deltaForward, deltaRight, deltaUp;
	float		forwardSpeed, rightSpeed, upSpeed;
	struct	{
		int		start, end;
		qboolean active, locked;
	} capture;
	struct {
		qhandle_t additiveWhiteShader;
		qhandle_t mouseCursor;
		qhandle_t switchOn, switchOff;
	} media;
	demofindType_t find;
	qboolean	seekEnabled;
	qboolean	showHud;
	qboolean	initDone;
	qboolean	autoLoad;
	demoLog_t	log;
} demoMain_t;

extern demoMain_t demo;

void demoPlaybackInit(void);
void CG_DemosAddLog(const char *fmt, ...);
void demoCameraCommand_f(void);
void demoPointCommand_f(void);

demoCameraPoint_t *cameraPointSynch( int time );
void cameraMove( void );
void cameraMoveDirect( void );
void cameraUpdate( int time, float timeFraction );
void cameraDraw( int time, float timeFraction );
qboolean cameraParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
void cameraSave( fileHandle_t fileHandle );

void demoLineCommand_f(void);
demoLinePoint_t *linePointSynch(int playTime);
void lineAt(int playTime, float playFraction, int *demoTime, float *demoFraction, float *demoSpeed );
qboolean lineParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
void lineSave( fileHandle_t fileHandle );

void demoScriptCommand_f(void);
demoScriptPoint_t *scriptPointSynch( int time );
void scriptRun( qboolean hadSkip );

void demoMovePoint( vec3_t origin, vec3_t velocity, vec3_t angles);
void demoMoveDirection( vec3_t origin, vec3_t angles );
void demoMoveAddDeltaAngles( vec3_t angles );

void demoMoveGrid( void );
void demoMoveView( void );
void demoMoveChase( void );
void demoMoveChaseDirect( void );
void demoMoveLine( void );
void demoMoveUpdateAngles( void );
void demoMoveGetAngles( vec3_t moveAngles );
void demoMoveSynchAngles( const vec3_t targetAngles );
void demoMoveDeltaCmd( void );

void demoCaptureCommand_f( void );
void demoCaptureMove( void );
void demoLoadCommand_f( void );
void demoSaveCommand_f( void );
qboolean demoProjectLoad( const char *fileName );
qboolean demoProjectSave( const char *fileName );

void listPointFree( void *listStart[], void *point );
void *ListPointAdd( void *listStart[], int time, int flags, int size );

void demoSaveLine( fileHandle_t fileHandle, const char *fmt, ...);


void demoChaseCommand_f( void );
void chaseEntityOrigin( centity_t *cent, vec3_t origin );
demoChasePoint_t *chasePointSynch(int time );
qboolean chaseParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
void chaseSave( fileHandle_t fileHandle );

demoDofPoint_t *dofPointSynch( int time );
void dofMove(void);
void dofUpdate( int time, float timeFraction );
void dofDraw( int time, float timeFraction );
qboolean dofParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
void dofSave( fileHandle_t fileHandle );
void demoDofCommand_f(void);

qboolean demoCentityBoxSize( const centity_t *cent, vec3_t container );
int demoHitEntities( const vec3_t start, const vec3_t forward );
centity_t *demoTargetEntity( int num );
void chaseUpdate( int time, float timeFraction );
void chaseDraw( int time, float timeFraction );
void demoDrawCrosshair( void );

void hudInitTables(void);
void hudToggleInput(void);
void hudDraw(void);
const char *demoTimeString( int time );

void gridDraw(void);

#define FOV_MIN		10
#define FOV_MAX		170

#define CAM_ORIGIN	0x001
#define CAM_ANGLES	0x002
#define CAM_FOV		0x004
#define CAM_TIME	0x100

#define EFFECT_ORIGIN	0x0001
#define EFFECT_ANGLES	0x0002
#define EFFECT_COLOR	0x0004
#define EFFECT_SIZE		0x0008
#define EFFECT_ONCE		0x0010



#endif
