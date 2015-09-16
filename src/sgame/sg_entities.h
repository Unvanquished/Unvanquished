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
 *  common entity spawn flags
 */
#define SPF_SPAWN_DISABLED          1
#define SPF_SPAWN_ENABLED           1
#define SPF_NEGATE_CONDITIONS       2

/*
 * Constant entity classnames.
 * These might get referenced from several locations in the code.
 * At least once for spawning, and then potentially for searching or validation.
 */

#define S_WORLDSPAWN              "worldspawn"
#define S_PLAYER_CLASSNAME        "player"

#define S_CTRL_LIMITED            "ctrl_limited"
#define S_CTRL_RELAY              "ctrl_relay"

#define S_env_afx_ammo            "env_afx_ammo"
#define S_env_afx_gravity         "env_afx_gravity"
#define S_env_afx_heal            "env_afx_heal"
#define S_env_afx_hurt            "env_afx_hurt"
#define S_env_afx_push            "env_afx_push"
#define S_env_afx_teleport        "env_afx_teleport"

#define S_fx_rumble               "fx_rumble"
#define S_sfx_speaker             "sfx_speaker"

#define S_gfx_animated_model      "gfx_animated_model"
#define S_gfx_light_flare         "gfx_light_flare"
#define S_gfx_particle_system     "gfx_particle_system"
#define S_gfx_portal_camera       "gfx_portal_camera"
#define S_gfx_portal_surface      "gfx_portal_surface"
#define S_gfx_shader_mod          "gfx_shader_mod"

#define S_FUNC_DOOR               "func_door"
#define S_DOOR_SENSOR             "door_sensor"
#define S_PLAT_SENSOR             "plat_sensor"

#define S_GAME_END                "game_end"
#define S_GAME_FUNDS              "game_funds"
#define S_GAME_KILL               "game_kill"
#define S_GAME_SCORE              "game_score"

#define S_PATH_CORNER             "path_corner"
#define S_POS_PLAYER_SPAWN        "pos_player_spawn"
#define S_POS_ALIEN_INTERMISSION  "pos_alien_intermission"
#define S_POS_PLAYER_INTERMISSION "pos_player_intermission"
#define S_POS_HUMAN_INTERMISSION  "pos_human_intermission"
#define S_POS_TARGET              "pos_target"
#define S_POS_LOCATION            "pos_location"

#define S_SENSOR_CREEP            "sensor_creep"
#define S_SENSOR_END              "sensor_end"
#define S_SENSOR_BUILDABLE        "sensor_buildable"
#define S_SENSOR_TIMER            "sensor_timer"
#define S_SENSOR_PLAYER           "sensor_player"
#define S_SENSOR_POWER            "sensor_power"
#define S_SENSOR_SUPPORT          "sensor_support"
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

/**
 * after which time shall a sensor that depends on polling for sensing poll again?
 */
#define SENSOR_POLL_PERIOD 100

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
	ECA_NOP = 0,
	ECA_DEFAULT,
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

typedef struct
{
	const char *event;
	gentityCallEvent_t eventType;

	char  *name;

	char  *action;
	gentityCallActionType_t actionType;
} gentityCallDefinition_t;

typedef struct
{
	gentityCallDefinition_t *definition;
	//struct gentity_s *recipient;
	struct gentity_s *caller;
	struct gentity_s *activator;
} gentityCall_t;

//
// g_entities.c
//
//lifecycle
void       G_InitGentityMinimal( gentity_t *e );
void       G_InitGentity( gentity_t *e );
gentity_t  *G_NewEntity();
gentity_t  *G_NewTempEntity( const vec3_t origin, int event );
void       G_FreeEntity( gentity_t *e );

//debug
const char *etos( const gentity_t *entity );
void       G_PrintEntityNameList( gentity_t *entity );

//search, select, iterate
gentity_t  *G_IterateEntities( gentity_t *entity, const char *classname, bool skipdisabled, size_t fieldofs, const char *match );
gentity_t  *G_IterateEntities( gentity_t *entity );
gentity_t  *G_IterateEntitiesOfClass( gentity_t *entity, const char *classname );
gentity_t  *G_IterateEntitiesWithField( gentity_t *entity, size_t fieldofs, const char *match );
gentity_t  *G_IterateEntitiesWithinRadius( gentity_t *entity, vec3_t origin, float radius );
gentity_t  *G_FindClosestEntity( vec3_t origin, gentity_t **entities, int numEntities );
gentity_t  *G_PickRandomEntity( const char *classname, size_t fieldofs, const char *match );
gentity_t  *G_PickRandomEntityOfClass( const char *classname );
gentity_t  *G_PickRandomEntityWithField( size_t fieldofs, const char *match );

//test
bool   G_MatchesName( gentity_t *entity, const char* name );
bool   G_IsVisible( gentity_t *ent1, gentity_t *ent2, int contents );

//chain
gentityCallEvent_t      G_GetCallEventTypeFor( const char* event );
gentityCallActionType_t G_GetCallActionTypeFor( const char* action );
gentity_t  *G_ResolveEntityKeyword( gentity_t *self, char *keyword );
gentity_t  *G_IterateTargets(gentity_t *entity, int *targetIndex, gentity_t *self);
gentity_t  *G_IterateCallEndpoints( gentity_t *entity, int *calltargetIndex, gentity_t *self );
gentity_t  *G_PickRandomTargetFor( gentity_t *self );
void       G_FireEntityRandomly( gentity_t *entity, gentity_t *activator );
void       G_FireEntity( gentity_t *ent, gentity_t *activator );
void       G_EventFireEntity( gentity_t *self, gentity_t *activator, gentityCallEvent_t eventType );

void       G_CallEntity(gentity_t *targetedEntity, gentityCall_t *call);
void       G_HandleActCall( gentity_t *entity, gentityCall_t *call );
void       G_ExecuteAct( gentity_t *entity, gentityCall_t *call );

//configure
void       G_SetMovedir( vec3_t angles, vec3_t movedir );
void       G_SetOrigin( gentity_t *ent, const vec3_t origin );

//
// g_spawn.c
//
void     G_SpawnEntitiesFromString();
void     G_SpawnFakeEntities();
void     G_ReorderCallTargets( gentity_t *ent );
char     *G_NewString( const char *string );

//
// g_spawn_mover.c
//
void G_RunMover( gentity_t *ent );
void door_trigger_touch( gentity_t *ent, gentity_t *other, trace_t *trace );
void manualTriggerSpectator( gentity_t *trigger, gentity_t *player );

//
// g_spawn_sensor.c
//
void G_notify_sensor_stage(team_t team, int previousStage, int newStage );
void G_notify_sensor_start( );
void G_notify_sensor_end( team_t winningTeam );

//
// g_spawn_shared.c
//
void     G_ResetIntField( int* target, bool fallbackIfNegativ, int instanceField, int classField, int fallbacke );
void     G_ResetFloatField( float* target, bool fallbackIfNegativ, float instanceField, float classField, float fallback );
void     G_ResetTimeField( variatingTime_t* result, variatingTime_t instanceField, variatingTime_t classField, variatingTime_t fallback );

//==================================================================
#endif /* ENTITIES_H_ */
