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

#include "bot_types.h"
#ifdef __cplusplus
extern "C"
{
#endif
qboolean     BotSetupNav( const botClass_t *botClass, qhandle_t *navHandle );
void         BotShutdownNav( void );

void         BotDisableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs );
void         BotEnableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs );
void         BotSetNavMesh( int botClientNum, qhandle_t nav );
unsigned int BotFindRouteExt( int botClientNum, const botRouteTarget_t *target );
qboolean     BotUpdateCorridor( int botClientNum, const botRouteTarget_t *target, vec3_t dir, qboolean *directPathToGoal );
void         BotFindRandomPoint( int botClientNum, vec3_t point );
qboolean     BotFindRandomPointInRadius( int botClientNum, const vec3_t origin, vec3_t point, float radius );
qboolean     BotNavTrace( int botClientNum, botTrace_t *trace, const vec3_t start, const vec3_t end );
void         BotAddObstacle( const vec3_t mins, const vec3_t maxs, qhandle_t *obstacleHandle );
void         BotRemoveObstacle( qhandle_t obstacleHandle );
void         BotUpdateObstacles();
#ifdef __cplusplus
}
#endif