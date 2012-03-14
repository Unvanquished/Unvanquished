/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

// bg_local.h -- local definitions for the bg (both games) files

#define MIN_WALK_NORMAL  0.7 // can't walk on very steep slopes

#define STEPSIZE         18

#define JUMP_VELOCITY    270

#define TIMER_LAND       130
#define TIMER_GESTURE    ( 34 * 66 + 50 )

#define DOUBLE_TAP_DELAY 400

#define MAX_MG42_HEAT    1500.f

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server
typedef struct
{
	vec3_t   forward, right, up;
	float    frametime;

	int      msec;

	qboolean walking;
	qboolean groundPlane;
	trace_t  groundTrace;

	float    impactSpeed;

	vec3_t   previous_origin;
	vec3_t   previous_velocity;
	int      previous_waterlevel;

	// Ridah, ladders
	qboolean ladder;
} pml_t;

extern pmove_t *pm;
extern pml_t   pml;

// movement parameters
extern float   pm_stopspeed;

//extern    float   pm_duckScale;

//----(SA)  modified
extern float pm_waterSwimScale;
extern float pm_waterWadeScale;
extern float pm_slagSwimScale;
extern float pm_slagWadeScale;

extern float pm_accelerate;
extern float pm_airaccelerate;
extern float pm_wateraccelerate;
extern float pm_slagaccelerate;
extern float pm_flyaccelerate;

extern float pm_friction;
extern float pm_waterfriction;
extern float pm_slagfriction;
extern float pm_flightfriction;

//----(SA)  end

extern int c_pmove;

void       PM_AddTouchEnt ( int entityNum );
void       PM_AddEvent ( int newEvent );

qboolean   PM_SlideMove ( qboolean gravity );
void       PM_StepSlideMove ( qboolean gravity );

qboolean   PM_SlideMoveProne ( qboolean gravity );
void       PM_StepSlideMoveProne ( qboolean gravity );

void       PM_BeginWeaponChange ( int oldweapon, int newweapon, qboolean reload );
