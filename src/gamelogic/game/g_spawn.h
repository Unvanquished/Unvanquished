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

#ifndef SPAWN_H_
#define SPAWN_H_

/*
 * g_spawn.c
 */

/** spawn string returns a temporary reference, you must CopyString() if you want to keep it */
qboolean G_SpawnString( const char *key, const char *defaultString, char **out );
qboolean G_SpawnBoolean( const char *key, qboolean defaultqboolean );
qboolean G_SpawnFloat( const char *key, const char *defaultString, float *out );
qboolean G_SpawnInt( const char *key, const char *defaultString, int *out );
qboolean G_SpawnVector( const char *key, const char *defaultString, float *out );


/*
 * spawn functions
 */
void    SP_func_plat( gentity_t *self );
void    SP_func_static( gentity_t *self );
void    SP_func_dynamic( gentity_t *self );
void    SP_func_rotating( gentity_t *self );
void    SP_func_bobbing( gentity_t *self );
void    SP_func_pendulum( gentity_t *self );
void    SP_func_button( gentity_t *self );
void    SP_func_door( gentity_t *self );
void    SP_func_door_rotating( gentity_t *self );
void    SP_func_door_model( gentity_t *self );
void    SP_func_train( gentity_t *self );

void    SP_func_destructable( gentity_t *self );
void    SP_func_spawn( gentity_t *self );

void    SP_ctrl_relay( gentity_t *self );
void    SP_ctrl_limited( gentity_t *self );

void    SP_game_score( gentity_t *self );
void    SP_game_kill( gentity_t *self );
void    SP_game_end( gentity_t *self );
void    SP_game_funds( gentity_t *self );

void    SP_path_corner( gentity_t *self );
void    SP_pos_player_spawn( gentity_t *self );
void    SP_pos_player_intermission( gentity_t *self );
void    SP_pos_target( gentity_t *self );
void    SP_pos_location( gentity_t *self );

void    SP_sensor_start( gentity_t *self );
void    SP_sensor_stage( gentity_t *self );
void    SP_sensor_end( gentity_t *self );
void    SP_sensor_player( gentity_t *self );
void    SP_sensor_power( gentity_t *self );
void    SP_sensor_support( gentity_t *self );
void    SP_sensor_creep( gentity_t *self );
void    SP_sensor_buildable( gentity_t *self );
void    SP_sensor_timer( gentity_t *self );

void    SP_target_print( gentity_t *self );
void    SP_target_teleporter( gentity_t *self );
void    SP_target_push( gentity_t *self );
void    SP_target_hurt( gentity_t *self );

void    SP_env_afx_push( gentity_t *self );
void    SP_env_afx_teleport( gentity_t *self );
void    SP_env_afx_hurt( gentity_t *self );
void    SP_env_afx_gravity( gentity_t *self );
void    SP_env_afx_heal( gentity_t *self );
void    SP_env_afx_ammo( gentity_t *self );

void    SP_fx_rumble( gentity_t *self );
void    SP_sfx_speaker( gentity_t *self );

void    SP_gfx_particle_system( gentity_t *self );
void    SP_gfx_animated_model( gentity_t *self );
void    SP_gfx_light_flare( gentity_t *self );
void    SP_gfx_portal_camera( gentity_t *self );
void    SP_gfx_portal_surface( gentity_t *self );
void    SP_gfx_shader_mod( gentity_t *self );

/*
 * everything around entity versioning and deprecation
 */
#define ENT_V_UNCLEAR  0
#define ENT_V_CURRENT  0
#define ENT_V_RENAMED  1
#define ENT_V_EXTENDED 2
#define ENT_V_COMBINED 3
#define ENT_V_STRIPPED 4
#define ENT_V_SPLITUP  5
#define ENT_V_REMOVED  8

#define ENT_V_TMPORARY 32
#define ENT_V_TMPNAME  33

/**
 * @return qtrue if a deprecated field was found, qfalse otherwise
 */
qboolean G_WarnAboutDeprecatedEntityField( gentity_t *entity, const char *expectedFieldname, const char *actualFieldname, const int typeOfDeprecation );

/**
 * shared entity functions
 */

void     think_fireDelayed( gentity_t *self );
void     think_fireOnActDelayed( gentity_t *self );
void     think_aimAtTarget( gentity_t *self );

void     SP_RemoveSelf( gentity_t *self );
void     SP_Nothing( gentity_t *self );

/*
 * Shared and standardized entity management
 */
void     SP_ConditionFields( gentity_t *self );
void     SP_WaitFields( gentity_t *self, float defaultWait, float defaultWaitVariance );

#endif /* SPAWN_H_ */
