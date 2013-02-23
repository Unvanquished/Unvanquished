/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

void    SP_info_player_start( gentity_t *ent );
void    SP_info_player_deathmatch( gentity_t *ent );
void    SP_info_player_intermission( gentity_t *ent );

void    SP_func_plat( gentity_t *ent );
void    SP_func_static( gentity_t *ent );
void    SP_func_dynamic( gentity_t *ent );
void    SP_func_rotating( gentity_t *ent );
void    SP_func_bobbing( gentity_t *ent );
void    SP_func_pendulum( gentity_t *ent );
void    SP_func_button( gentity_t *ent );
void    SP_func_door( gentity_t *ent );
void    SP_func_door_rotating( gentity_t *ent );
void    SP_func_door_model( gentity_t *ent );
void    SP_func_train( gentity_t *ent );

void    SP_func_destructable( gentity_t *self );
void    SP_func_spawn( gentity_t *self );

void    SP_ctrl_relay( gentity_t *ent );
void    SP_ctrl_limited( gentity_t *ent );

void    SP_game_score( gentity_t *ent );
void    SP_game_end( gentity_t *ent );

void    SP_sensor_start( gentity_t *ent );
void    SP_sensor_stage( gentity_t *ent );
void    SP_sensor_end( gentity_t *ent );
void    SP_sensor_touch( gentity_t *ent );
void    SP_sensor_touch_compat( gentity_t *ent );
void    SP_sensor_timer( gentity_t *self );

void    SP_trigger_multiple( gentity_t *ent );
void    SP_trigger_push( gentity_t *ent );
void    SP_trigger_teleport( gentity_t *ent );
void    SP_trigger_hurt( gentity_t *ent );
void    SP_trigger_gravity( gentity_t *ent );
void    SP_trigger_heal( gentity_t *ent );
void    SP_trigger_ammo( gentity_t *ent );

void    SP_target_print( gentity_t *ent );
void    SP_target_teleporter( gentity_t *ent );
void    SP_target_kill( gentity_t *ent );
void    SP_target_position( gentity_t *ent );
void    SP_target_location( gentity_t *ent );
void    SP_target_push( gentity_t *ent );
void    SP_target_alien_win( gentity_t *ent );
void    SP_target_human_win( gentity_t *ent );
void    SP_target_hurt( gentity_t *ent );

void    SP_NULL( gentity_t *self );
void    SP_path_corner( gentity_t *self );

void    SP_env_portal_camera( gentity_t *ent );
void    SP_env_portal_surface( gentity_t *ent );

void    SP_env_rumble( gentity_t *ent );
void    SP_env_speaker( gentity_t *ent );
void    SP_env_particle_system( gentity_t *ent );
void    SP_env_animated_model( gentity_t *ent );
void    SP_env_lens_flare( gentity_t *ent );

/*
 * everything around entity versioning and deprecation
 */
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

/*
 * Standardized entity management
 */

/**
 * predefined field-interpretation
 */
void     entity_SetNextthink( gentity_t *self );
void     entity_ParseConditions( gentity_t *self );


// Init functions
void    SP_target_init( void );
