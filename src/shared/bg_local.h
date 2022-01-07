/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

#ifndef BG_LOCAL_H_
#define BG_LOCAL_H_
// bg_local.h -- local definitions for the bg (both games) files
//==================================================================

#define MIN_WALK_NORMAL   0.7f // can't walk on very steep slopes

#define STEPSIZE          18

#define TIMER_LAND        130
#define TIMER_GESTURE     ( 34 * 66 + 50 )
#define TIMER_ATTACK      500 //nonsegmented models

#define FALLING_THRESHOLD -900.0f //what vertical speed to start falling sound at

extern  pmove_t *pm;

// movement parameters
#define pm_duckScale         (0.25f)
#define pm_swimScale         (0.50f)

#define pm_accelerate        (10.0f)
#define pm_wateraccelerate   (4.0f)
#define pm_flyaccelerate     (4.0f)

#define pm_waterfriction     (1.125f)
#define pm_spectatorfriction (5.0f)

extern  int     c_pmove;

void            PM_ClipVelocity( const vec3_t in, const vec3_t normal, vec3_t out );
void            PM_AddTouchEnt( int entityNum );

//==================================================================
#endif /* BG_LOCAL_H_ */
