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

#ifndef ENTITIES_H_
#define ENTITIES_H_
//==================================================================

// gentity->flags
#define FL_GODMODE                 0x00000010
#define FL_NOTARGET                0x00000020
#define FL_GROUPSLAVE              0x00000400 // not the first on the group
#define FL_NO_KNOCKBACK            0x00000800
#define FL_NO_BOTS                 0x00002000 // spawn point not for bot use
#define FL_NO_HUMANS               0x00004000 // spawn point just for bots
#define FL_FORCE_GESTURE           0x00008000

/*
 * Constant entity classnames.
 * These might get referenced from several locations in the code.
 * At least once for spawning, and then potentially for searching or validation.
 */

#define S_FUNC_DOOR               "func_door"
#define S_DOOR_TRIGGER            "door_trigger"
#define S_PLAT_TRIGGER            "plat_trigger"

#define S_PATH_CORNER             "path_corner"
#define S_POS_PLAYER_SPAWN        "pos_player_spawn"
#define S_POS_ALIEN_INTERMISSION  "pos_alien_intermission"
#define S_POS_PLAYER_INTERMISSION "pos_player_intermission"
#define S_POS_HUMAN_INTERMISSION  "pos_human_intermission"

#define S_SENSOR_END              "sensor_end"
#define S_SENSOR_PLAYER           "sensor_player"
#define S_SENSOR_START            "sensor_start"
#define S_SENSOR_STAGE            "sensor_stage"


/**
 * The maximal available targets to aim at per entity.
 */
#define MAX_ENTITY_TARGETS           4


/**
 * The maximal available calltargets per entity
 */
#define MAX_ENTITY_CALLTARGETS       16

/**
 * The maximal available names or aliases per entity.
 *
 * If you increase these, then you also have to
 * change g_spawn.c to spawn additional targets and targetnames
 *
 * @see fields[] (where you should spawn additional ones)
 * @see G_SpawnGEntityFromSpawnVars()
 */
#define MAX_ENTITY_ALIASES 	3

// movers are things like doors, plats, buttons, etc
typedef enum
{
  MOVER_POS1,
  MOVER_POS2,
  MOVER_1TO2,
  MOVER_2TO1,

  ROTATOR_POS1,
  ROTATOR_POS2,
  ROTATOR_1TO2,
  ROTATOR_2TO1,

  MODEL_POS1,
  MODEL_POS2,
  MODEL_1TO2,
  MODEL_2TO1
} moverState_t;

typedef enum
{
	ECA_DEFAULT = 0,
	ECA_CUSTOM,

	ECA_FREE,
	ECA_PROPAGATE,

	ECA_ACT,
	ECA_USE,
	ECA_RESET,

	ECA_ENABLE,
	ECA_DISABLE,
	ECA_TOGGLE

} gentityCallActionType_t;

typedef enum
{
	ON_DEFAULT = 0,
	ON_CUSTOM,

	ON_FREE,

	ON_CALL,

	ON_ACT,
	ON_USE,
	ON_DIE,
	ON_REACH,
	ON_RESET,
	ON_TOUCH,

	ON_ENABLE,
	ON_DISABLE,

	ON_SPAWN

} gentityCallEvent_t;

//==================================================================
#endif /* ENTITIES_H_ */
