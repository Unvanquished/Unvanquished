/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
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

#include "sg_map_entity.h"

// gentity->flags
#define FL_GODMODE                 0x00000010
#define FL_NOTARGET                0x00000020
#define FL_GROUPSLAVE              0x00000400 // not the first on the group

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
 * after which time shall a sensor that depends on polling for sensing poll again?
 */
#define SENSOR_POLL_PERIOD 100

enum initEntityStyle_t
{
	NO_CBSE,
	HAS_CBSE,
};

struct gentityCall_t
{
	gentityCallDefinition_t *definition;
	gentity_t *caller;
	gentity_t *activator;
};

//
// g_entities.c
//
//lifecycle
void       G_InitGentityMinimal( gentity_t *e );
void       G_InitGentity( gentity_t *e );
gentity_t  *G_NewEntity( initEntityStyle_t style );
gentity_t  *G_NewTempEntity( glm::vec3 origin, int event );
void       G_FreeEntity( gentity_t *e );

//debug
const char *etos( const gentity_t *entity );
void       G_PrintEntityNameList( gentity_t *entity );

//search, select, iterate
gentity_t  *G_IterateEntities( gentity_t *entity, const char *classname, bool skipdisabled, size_t fieldofs, const char *match );
gentity_t  *G_IterateEntities( gentity_t *entity );
gentity_t  *G_IterateEntitiesOfClass( gentity_t *entity, const char *classname );
gentity_t  *G_IterateEntitiesWithField( gentity_t *entity, size_t fieldofs, const char *match );
gentity_t  *G_IterateEntitiesWithinRadius( gentity_t *entity, const glm::vec3& origin, float radius );
gentity_t  *G_PickRandomEntity( const char *classname, size_t fieldofs, const char *match );
gentity_t  *G_PickRandomEntityOfClass( const char *classname );
gentity_t  *G_PickRandomEntityWithField( size_t fieldofs, const char *match );

//test
bool   G_MatchesName( gentity_t *entity, const char* name );
bool   G_IsVisible( gentity_t *ent1, gentity_t *ent2, int contents );

//chain
gentityCallActionType_t G_GetCallActionTypeFor( const char* action );
gentity_t  *G_ResolveEntityKeyword( gentity_t *self, char *keyword );
gentity_t  *G_IterateTargets(gentity_t *entity, int *targetIndex, gentity_t *self);
gentity_t  *G_IterateCallEndpoints( gentity_t *entity, int *calltargetIndex, gentity_t *self );
gentity_t  *G_PickRandomTargetFor( gentity_t *self );
void       G_FireEntityRandomly( gentity_t *entity, gentity_t *activator );
void       G_FireEntity( gentity_t *ent, gentity_t *activator );

void       G_CallEntity(gentity_t *targetedEntity, gentityCall_t *call);
void       G_HandleActCall( gentity_t *entity, gentityCall_t *call );
void       G_ExecuteAct( gentity_t *entity, gentityCall_t *call );

//configure
void       G_SetMovedir( glm::vec3& angles, glm::vec3& movedir );
void       G_SetOrigin( gentity_t *ent, const glm::vec3& origin );

//
// g_spawn.c
//
void     G_SpawnEntitiesFromString();
void     G_SpawnFakeEntities();

//
// g_spawn_mover.c
//
void G_RunMover( gentity_t *ent );

//
// g_spawn_sensor.c
//
void G_notify_sensor_stage(team_t team, int previousStage, int newStage );
void G_notify_sensor_start( );
void G_notify_sensor_end( team_t winningTeam );

gentityCallEvent_t      G_GetCallEventTypeFor( const char* event );
void       G_EventFireEntity( gentity_t *self, gentity_t *activator, gentityCallEvent_t eventType );


//==================================================================
#endif /* ENTITIES_H_ */
