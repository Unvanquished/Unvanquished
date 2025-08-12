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

#include "common/Common.h"

#include "common/cm/cm_public.h"

#include "sg_local.h"
#include "sg_spawn.h"

bool G_SpawnString( const char *key, const char *defaultString, char **out )
{
	int i;

	if ( !level.spawning )
	{
		*out = ( char * ) defaultString;
//    Sys::Drop( "G_SpawnString() called while not spawning" );
		return false;
	}

	for ( i = 0; i < level.numSpawnVars; i++ )
	{
		if ( !Q_stricmp( key, level.spawnVars[ i ][ 0 ] ) )
		{
			*out = level.spawnVars[ i ][ 1 ];
			return true;
		}
	}

	*out = ( char * ) defaultString;
	return false;
}

/**
 * spawns a string and sets it as a cvar, or reset the cvar to its default
 * value if not set.
 * This allows loading values from the map, without keeping the garbage from
 * the previous map if nothing is set.
 */
static void G_SpawnStringIntoCVar( const char *key, Cvar::CvarProxy& cvar )
{
	char *str = nullptr;
	if ( G_SpawnString( key, nullptr, &str ) )
	{
		Cvar::SetValue( cvar.Name(), str );
	}
	else
	{
		cvar.Reset();
	}
}

bool G_SpawnBoolean( const char *key, bool defaultqboolean )
{
	char     *string;
	int     out;

	if(G_SpawnString( key, "", &string ))
	{
		if(Q_strtoi(string, &out))
		{
			if(out == 1)
			{
				return true;
			}
			else if(out == 0)
			{
				return false;
			}
			return defaultqboolean;
		}
		else
		{
			if(!Q_stricmp(string, "true"))
			{
				return true;
			}
			else if(!Q_stricmp(string, "false"))
			{
				return false;
			}
			return defaultqboolean;
		}
	}
	else
	{
		return defaultqboolean;
	}
}

bool  G_SpawnFloat( const char *key, const char *defaultString, float *out )
{
	char     *s;
	bool present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atof( s );
	return present;
}

bool G_SpawnInt( const char *key, const char *defaultString, int *out )
{
	char     *s;
	bool present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

bool G_SpawnVector( const char *key, const char *defaultString, vec3_t out )
{
	char     *s;

	if ( G_SpawnString( key, defaultString, &s ) )
	{
		if ( 3 == sscanf( s, "%f %f %f", &out[ 0 ], &out[ 1 ], &out[ 2 ] ) )
		{
			return true;
		}

		Log::Warn( "G_SpawnVector: invalid string '%s'", s );
	}

	if ( 3 != sscanf( defaultString, "%f %f %f", &out[ 0 ], &out[ 1 ], &out[ 2 ] ) )
	{
		ASSERT_UNREACHABLE();
	}

	return false;
}

//
// fields are needed for spawning from the entity string
//
enum fieldType_t
{
  F_INT,
  F_FLOAT,
  F_STRING,
  F_TARGET,
  F_CALLTARGET,
  F_TIME,
  F_3D_VECTOR,
  F_4D_VECTOR,
  F_YAW,
  F_SOUNDINDEX
};

struct fieldDescriptor_t
{
	const char  *name;
	size_t      offset;
	fieldType_t type;
	int   versionState;
	const char  *replacement;
};

static const fieldDescriptor_t fields[] =
{
	{ "acceleration",        FOFS( acceleration ),                   F_3D_VECTOR,  ENT_V_UNCLEAR, nullptr },
	{ "alias",               FOFS( mapEntity.names[ 2 ] ),           F_STRING,     ENT_V_UNCLEAR, nullptr },
	{ "alpha",               FOFS( mapEntity.restingPosition ),      F_3D_VECTOR,  ENT_V_UNCLEAR, nullptr }, // What's with the variable abuse everytime?
	{ "amount",              FOFS( mapEntity.config.amount ),        F_INT,        ENT_V_UNCLEAR, nullptr },
	{ "angle",               FOFS( s.angles ),                       F_YAW,        ENT_V_TMPNAME, "yaw" }, //radiants ui sadly strongly encourages the "angle" keyword
	{ "angles",              FOFS( s.angles ),                       F_3D_VECTOR,  ENT_V_UNCLEAR, nullptr },
	{ "animation",           FOFS( mapEntity.animation ),            F_4D_VECTOR,  ENT_V_UNCLEAR, nullptr },
	{ "bounce",              FOFS( physicsBounce ),                  F_FLOAT,      ENT_V_UNCLEAR, nullptr },
	{ "classname",           FOFS( classname ),                      F_STRING,     ENT_V_UNCLEAR, nullptr },
	{ "delay",               FOFS( mapEntity.config.delay ),         F_TIME,       ENT_V_UNCLEAR, nullptr },
	{ "dmg",                 FOFS( mapEntity.config.damage ),        F_INT,        ENT_V_UNCLEAR, nullptr },
	{ "gravity",             FOFS( mapEntity.config.amount ),        F_INT,        ENT_V_UNCLEAR, "amount" },
	{ "health",              FOFS( mapEntity.config.health ),        F_INT,        ENT_V_UNCLEAR, nullptr },
	{ "id",                  FOFS( id ),                             F_STRING,     ENT_V_UNCLEAR, nullptr },
	{ "message",             FOFS( mapEntity.message ),              F_STRING,     ENT_V_UNCLEAR, nullptr },
	{ "model",               FOFS( mapEntity.model ),                F_STRING,     ENT_V_UNCLEAR, nullptr },
	{ "model2",              FOFS( mapEntity.model2 ),               F_STRING,     ENT_V_UNCLEAR, nullptr },
	{ "name",                FOFS( mapEntity.names[ 0 ] ),           F_STRING,     ENT_V_UNCLEAR, nullptr },
	{ "noise",               FOFS( mapEntity.soundIndex ),           F_SOUNDINDEX, ENT_V_UNCLEAR, nullptr },
	{ "onAct",               FOFS( mapEntity.calltargets ),          F_CALLTARGET, ENT_V_UNCLEAR, nullptr },
	{ "onDie",               FOFS( mapEntity.calltargets ),          F_CALLTARGET, ENT_V_UNCLEAR, nullptr },
	{ "onDisable",           FOFS( mapEntity.calltargets ),          F_CALLTARGET, ENT_V_UNCLEAR, nullptr },
	{ "onEnable",            FOFS( mapEntity.calltargets ),          F_CALLTARGET, ENT_V_UNCLEAR, nullptr },
	{ "onFree",              FOFS( mapEntity.calltargets ),          F_CALLTARGET, ENT_V_UNCLEAR, nullptr },
	{ "onReach",             FOFS( mapEntity.calltargets ),          F_CALLTARGET, ENT_V_UNCLEAR, nullptr },
	{ "onReset",             FOFS( mapEntity.calltargets ),          F_CALLTARGET, ENT_V_UNCLEAR, nullptr },
	{ "onSpawn",             FOFS( mapEntity.calltargets ),          F_CALLTARGET, ENT_V_UNCLEAR, nullptr },
	{ "onTouch",             FOFS( mapEntity.calltargets ),          F_CALLTARGET, ENT_V_UNCLEAR, nullptr },
	{ "onUse",               FOFS( mapEntity.calltargets ),          F_CALLTARGET, ENT_V_UNCLEAR, nullptr },
	{ "origin",              FOFS( s.origin ),                       F_3D_VECTOR,  ENT_V_UNCLEAR, nullptr },
	{ "period",              FOFS( mapEntity.config.period ),        F_TIME,       ENT_V_UNCLEAR, nullptr },
	{ "radius",              FOFS( mapEntity.activatedPosition ),    F_3D_VECTOR,  ENT_V_UNCLEAR, nullptr }, // What's with the variable abuse everytime?
	{ "random",              FOFS( mapEntity.config.wait.variance ), F_FLOAT,      ENT_V_TMPNAME, "wait" },
	{ "replacement",         FOFS( mapEntity.shaderReplacement ),    F_STRING,     ENT_V_UNCLEAR, nullptr },
	{ "shader",              FOFS( mapEntity.shaderKey ),            F_STRING,     ENT_V_UNCLEAR, nullptr },
	{ "sound1to2",           FOFS( mapEntity.sound1to2 ),            F_SOUNDINDEX, ENT_V_UNCLEAR, nullptr },
	{ "sound2to1",           FOFS( mapEntity.sound2to1 ),            F_SOUNDINDEX, ENT_V_UNCLEAR, nullptr },
	{ "soundPos1",           FOFS( mapEntity.soundPos1 ),            F_SOUNDINDEX, ENT_V_UNCLEAR, nullptr },
	{ "soundPos2",           FOFS( mapEntity.soundPos2 ),            F_SOUNDINDEX, ENT_V_UNCLEAR, nullptr },
	{ "spawnflags",          FOFS( mapEntity.spawnflags ),           F_INT,        ENT_V_UNCLEAR, nullptr },
	{ "speed",               FOFS( mapEntity.config.speed ),         F_FLOAT,      ENT_V_UNCLEAR, nullptr },
	{ "stage",               FOFS( mapEntity.conditions.stage ),     F_INT,        ENT_V_UNCLEAR, nullptr },
	{ "target",              FOFS( mapEntity.targets ),              F_TARGET,     ENT_V_UNCLEAR, nullptr },
	{ "target2",             FOFS( mapEntity.targets ),              F_TARGET,     ENT_V_UNCLEAR, nullptr }, // backwardcompatibility with AMP and to use the blackout map for testing
	{ "target3",             FOFS( mapEntity.targets ),              F_TARGET,     ENT_V_UNCLEAR, nullptr }, // backwardcompatibility with AMP and to use the blackout map for testing
	{ "target4",             FOFS( mapEntity.targets ),              F_TARGET,     ENT_V_UNCLEAR, nullptr }, // backwardcompatibility with AMP and to use the blackout map for testing
	{ "targetname",          FOFS( mapEntity.names[ 1 ] ),           F_STRING,     ENT_V_TMPNAME, "name" }, //radiants ui sadly strongly encourages the "targetname" keyword
	{ "targetname2",         FOFS( mapEntity.names[ 2 ] ),           F_STRING,     ENT_V_RENAMED, "name" }, // backwardcompatibility with AMP and to use the blackout map for testing
	{ "targetShaderName",    FOFS( mapEntity.shaderKey ),            F_STRING,     ENT_V_RENAMED, "shader" },
	{ "targetShaderNewName", FOFS( mapEntity.shaderReplacement ),    F_STRING,     ENT_V_RENAMED, "replacement" },
	{ "team",                FOFS( mapEntity.conditions.team ),      F_INT,        ENT_V_UNCLEAR, nullptr },
	{ "wait",                FOFS( mapEntity.config.wait ),          F_TIME,       ENT_V_UNCLEAR, nullptr },
	{ "yaw",                 FOFS( s.angles ),                       F_YAW,        ENT_V_UNCLEAR, nullptr },
};

enum entityChainType_t
{
	/*
	 * self sufficient, it might possibly be fired at, but it can do as well on its own, so won't be freed automatically
	 */
	CHAIN_AUTONOMOUS,
	/*
	 * needs something to fire at, or it has no reason to exist in this <world> and will be freed
	 */
	CHAIN_ACTIVE,
	/*
	 * needs something to fire at it in order to fullfill any task given to it, or will be freed otherwise
	 */
	CHAIN_PASSIV,
	/*
	 * needs something to fire at it, and something to relay that fire to (under whatever conditions given), or will be freed otherwise
	 */
	CHAIN_RELAY,
	/*
	 * will be aimed at by something, but no firing needs to be involved at all, if not aimed at, it will be freed
	 */
	CHAIN_TARGET,
};

struct entityClassDescriptor_t
{
	const char *name;
	void ( *spawn )( gentity_t *entityToSpawn );
	const entityChainType_t chainType;

	//optional spawn-time data
	int	versionState;
	const char  *replacement;
};


static const entityClassDescriptor_t entityClassDescriptions[] =
{
	/**
	 *
	 *	Control entities
	 *	================
	 *	Entities state control and optionally store states.
	 *	Act essentially as Statemachines.
	 *
	 */
	{ S_CTRL_LIMITED,             SP_ctrl_limited,           CHAIN_RELAY, ENT_V_UNCLEAR, nullptr },
	{ S_CTRL_RELAY,               SP_ctrl_relay,             CHAIN_RELAY, ENT_V_UNCLEAR, nullptr },

	/**
	 *
	 *	Environment area effect entities
	 *	====================
	 *	Entities that represent some form of area effect in the world.
	 *	Many are client predictable or even completly handled clientside.
	 *
	 */
	{ S_env_afx_ammo,             SP_env_afx_ammo,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_env_afx_gravity,          SP_env_afx_gravity,        CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_env_afx_heal,             SP_env_afx_heal,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_env_afx_hurt,             SP_env_afx_hurt,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_env_afx_push,             SP_env_afx_push,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_env_afx_teleport,         SP_env_afx_teleport,       CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },

	/**
	 *	Functional entities
	 *	====================
	 */
	{ "func_bobbing",             SP_func_bobbing,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ "func_button",              SP_func_button,            CHAIN_ACTIVE,     ENT_V_UNCLEAR, nullptr },
	{ "func_destructable",        SP_func_destructable,      CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_FUNC_DOOR,                SP_func_door,              CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ "func_door_model",          SP_func_door_model,        CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ "func_door_rotating",       SP_func_door_rotating,     CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ "func_dynamic",             SP_func_dynamic,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ "func_group",               SP_RemoveSelf,             (entityChainType_t) 0, ENT_V_UNCLEAR, nullptr },
	{ "func_pendulum",            SP_func_pendulum,          CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ "func_plat",                SP_func_plat,              CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ "func_rotating",            SP_func_rotating,          CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ "func_spawn",               SP_func_spawn,             CHAIN_PASSIV,     ENT_V_UNCLEAR, nullptr },
	{ "func_static",              SP_func_static,            CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ "func_timer",               SP_sensor_timer,			 CHAIN_ACTIVE,     ENT_V_TMPNAME, S_SENSOR_TIMER },
	{ "func_train",               SP_func_train,             CHAIN_ACTIVE,     ENT_V_UNCLEAR, nullptr },

	/**
	 *
	 *	Effects entities
	 *	====================
	 *	Entities that represent some form of Effect in the world.
	 *	Some might be client predictable or even completly handled clientside.
	 */
	{ S_fx_rumble,                SP_fx_rumble,              CHAIN_PASSIV,     ENT_V_UNCLEAR, nullptr },

	/**
	 *
	 *	Game entities
	 *	=============
	 *  Entities that have an major influence on the gameplay.
	 *  These are actions and effects against the whole match, a team or a player.
	 */
	{ S_GAME_END,                 SP_game_end,               CHAIN_PASSIV, ENT_V_UNCLEAR, nullptr },
	{ S_GAME_FUNDS,               SP_game_funds,             CHAIN_PASSIV, ENT_V_UNCLEAR, nullptr },
	{ S_GAME_KILL,                SP_game_kill,              CHAIN_PASSIV, ENT_V_UNCLEAR, nullptr },
	{ S_GAME_SCORE,               SP_game_score,             CHAIN_PASSIV, ENT_V_UNCLEAR, nullptr },

	/**
	 *
	 *	Graphic Effects
	 *	====================
	 *	Entities that represent some form of Graphical or visual Effect in the world.
	 *	They will be handled by cgame and mostly the renderer.
	 */
	{ S_gfx_animated_model,       SP_gfx_animated_model,     CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_gfx_light_flare,          SP_gfx_light_flare,        CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_gfx_particle_system,      SP_gfx_particle_system,    CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_gfx_portal_camera,        SP_gfx_portal_camera,      CHAIN_TARGET,     ENT_V_UNCLEAR, nullptr },
	{ S_gfx_portal_surface,       SP_gfx_portal_surface,     CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_gfx_shader_mod,           SP_gfx_shader_mod,         CHAIN_PASSIV,     ENT_V_UNCLEAR, nullptr },


	/**
	 * former information and misc entities, now deprecated
	 */
	{ "info_alien_intermission",  SP_Nothing,                CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_POS_ALIEN_INTERMISSION  },
	{ "info_human_intermission",  SP_Nothing,                CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_POS_HUMAN_INTERMISSION  },
	{ "info_notnull",             SP_pos_target,             CHAIN_TARGET,     ENT_V_RENAMED, S_POS_TARGET },
	{ "info_null",                SP_RemoveSelf,             (entityChainType_t) 0, ENT_V_UNCLEAR, nullptr },
	{ "info_player_deathmatch",   SP_pos_player_spawn,       CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_POS_PLAYER_SPAWN },
	{ "info_player_intermission", SP_Nothing,                CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_POS_PLAYER_INTERMISSION },
	{ "info_player_start",        SP_pos_player_spawn,       CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_POS_PLAYER_SPAWN },
	{ "light",                    SP_RemoveSelf,             (entityChainType_t) 0, ENT_V_UNCLEAR, nullptr },
	{ "misc_anim_model",          SP_gfx_animated_model,     CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_gfx_animated_model },
	{ "misc_light_flare",         SP_gfx_light_flare,        CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_gfx_light_flare },
	{ "misc_model",               SP_RemoveSelf,             (entityChainType_t) 0, ENT_V_UNCLEAR, nullptr },
	{ "misc_particle_system",     SP_gfx_particle_system,    CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_gfx_particle_system},
	{ "misc_portal_camera",       SP_gfx_portal_camera,      CHAIN_TARGET,     ENT_V_TMPNAME, S_gfx_portal_camera },
	{ "misc_portal_surface",      SP_gfx_portal_surface,     CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_gfx_portal_surface },
	{ "misc_teleporter_dest",     SP_pos_target,             CHAIN_TARGET,     ENT_V_RENAMED, S_POS_TARGET },

	/**
	 *  Position entities
	 *  =================
	 *  position entities may get used by other entities or other processes as provider for positional data
	 *
	 *  positions may or may not have an additional direction attached to them
	 *  they may also target to another position to indicate that direction,
	 *  possibly even modeling a form of path
	 */
	{ S_PATH_CORNER,              SP_Nothing,                CHAIN_TARGET,     ENT_V_UNCLEAR, nullptr },
	{ S_POS_ALIEN_INTERMISSION,   SP_Nothing,                CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_POS_HUMAN_INTERMISSION,   SP_Nothing,                CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_POS_LOCATION,             SP_pos_location,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_POS_PLAYER_INTERMISSION,  SP_Nothing,                CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_POS_PLAYER_SPAWN,         SP_pos_player_spawn,       CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },
	{ S_POS_TARGET,               SP_pos_target,             CHAIN_TARGET, ENT_V_UNCLEAR, nullptr },

	/**
	 *  Sensors
	 *  =======
	 *  Sensor fire an event against call-receiving entities, when aware
	 *  of another entity, event, or gamestate (timer and start being aware of the game start).
	 *  Enabling/Disabling Sensors generally changes their ability of perceiving other entities.
	 */
	{ S_SENSOR_BUILDABLE,         SP_sensor_buildable,       CHAIN_ACTIVE, ENT_V_UNCLEAR, nullptr },
	{ S_SENSOR_CREEP,             SP_sensor_creep,           CHAIN_ACTIVE, ENT_V_UNCLEAR, nullptr },
	{ S_SENSOR_END,               SP_sensor_end,             CHAIN_ACTIVE, ENT_V_UNCLEAR, nullptr },
	{ S_SENSOR_PLAYER,            SP_sensor_player,          CHAIN_ACTIVE, ENT_V_UNCLEAR, nullptr },
	{ S_SENSOR_POWER,             SP_sensor_power,           CHAIN_ACTIVE, ENT_V_UNCLEAR, nullptr },
	{ S_SENSOR_STAGE,             SP_sensor_stage,           CHAIN_ACTIVE, ENT_V_UNCLEAR, nullptr },
	{ S_SENSOR_START,             SP_sensor_start,           CHAIN_ACTIVE, ENT_V_UNCLEAR, nullptr },
	{ S_SENSOR_SUPPORT,           SP_sensor_support,         CHAIN_ACTIVE, ENT_V_UNCLEAR, nullptr },
	{ S_SENSOR_TIMER,             SP_sensor_timer,           CHAIN_ACTIVE,     ENT_V_UNCLEAR, nullptr },

   /**
	*
	*	Sound Effects
	*	====================
	*	Entities that represent some form of auditive effect in the world.
	*	They will be handled by cgame and even more the sound backend.
	*/
	{ S_sfx_speaker,              SP_sfx_speaker,            CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, nullptr },



	/*
	 * former target and trigger entities, now deprecated or soon to be deprecated
	 */
	{ "target_alien_win",         SP_game_end,               CHAIN_PASSIV,     ENT_V_TMPNAME, S_GAME_END },
	{ "target_delay",             SP_ctrl_relay,             CHAIN_RELAY,      ENT_V_TMPNAME, S_CTRL_RELAY },
	{ "target_human_win",         SP_game_end,               CHAIN_PASSIV,     ENT_V_TMPNAME, S_GAME_END },
	{ "target_hurt",              SP_target_hurt,            CHAIN_PASSIV,     ENT_V_UNCLEAR, nullptr },
	{ "target_kill",              SP_game_kill,              CHAIN_PASSIV,     ENT_V_TMPNAME, S_GAME_KILL },
	{ "target_location",          SP_pos_location,           CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_POS_LOCATION },
	{ "target_position",          SP_pos_target,             CHAIN_TARGET,     ENT_V_RENAMED, S_POS_TARGET },
	{ "target_print",             SP_target_print,           CHAIN_PASSIV,     ENT_V_UNCLEAR, nullptr },
	{ "target_push",              SP_target_push,            CHAIN_PASSIV,     ENT_V_UNCLEAR, nullptr },
	{ "target_relay",             SP_ctrl_relay,             CHAIN_RELAY,      ENT_V_TMPNAME, S_CTRL_RELAY },
	{ "target_rumble",            SP_fx_rumble,              CHAIN_PASSIV,     ENT_V_TMPNAME, S_fx_rumble },
	{ "target_score",             SP_game_score,             CHAIN_PASSIV,     ENT_V_TMPNAME, S_GAME_SCORE },
	{ "target_speaker",           SP_sfx_speaker,            CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_sfx_speaker },
	{ "target_teleporter",        SP_target_teleporter,      CHAIN_PASSIV,     ENT_V_UNCLEAR, nullptr },
	{ "trigger_always",           SP_sensor_start,           CHAIN_ACTIVE,     ENT_V_RENAMED, S_SENSOR_START },
	{ "trigger_ammo",             SP_env_afx_ammo,           CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_env_afx_ammo },
	{ "trigger_buildable",        SP_sensor_buildable,       CHAIN_ACTIVE,     ENT_V_TMPNAME, S_SENSOR_BUILDABLE },
	{ "trigger_class",            SP_sensor_player,          CHAIN_ACTIVE,     ENT_V_TMPNAME, S_SENSOR_PLAYER },
	{ "trigger_equipment",        SP_sensor_player,          CHAIN_ACTIVE,     ENT_V_TMPNAME, S_SENSOR_PLAYER },
	{ "trigger_gravity",          SP_env_afx_gravity,        CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_env_afx_gravity },
	{ "trigger_heal",             SP_env_afx_heal,           CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_env_afx_heal },
	{ "trigger_hurt",             SP_env_afx_hurt,           CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_env_afx_hurt },
	{ "trigger_multiple",         SP_sensor_player,          CHAIN_ACTIVE,     ENT_V_TMPNAME, S_SENSOR_PLAYER },
	{ "trigger_push",             SP_env_afx_push,           CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_env_afx_push },
	{ "trigger_stage",            SP_sensor_stage,           CHAIN_ACTIVE,     ENT_V_RENAMED, S_SENSOR_STAGE },
	{ "trigger_teleport",         SP_env_afx_teleport,       CHAIN_AUTONOMOUS, ENT_V_TMPNAME, S_env_afx_teleport },
	{ "trigger_win",              SP_sensor_end,             CHAIN_ACTIVE,     ENT_V_TMPNAME, S_SENSOR_END }
};

static bool G_HandleEntityVersions( entityClassDescriptor_t *spawnDescription, gentity_t *entity )
{
	if ( spawnDescription->versionState == ENT_V_CURRENT ) // we don't need to handle anything
		return true;

	if ( !spawnDescription->replacement || !Q_stricmp(entity->classname, spawnDescription->replacement))
	{
		if ( g_debugEntities.Get() > -2 )
			Log::Warn("Class %s has been marked deprecated but no replacement has been supplied", etos( entity ) );

		return false;
	}

	if ( g_debugEntities.Get() >= 0 ) //dont't warn about anything with -1 or lower
	{
		if( spawnDescription->versionState < ENT_V_TMPORARY
		|| ( g_debugEntities.Get() >= 1 && spawnDescription->versionState >= ENT_V_TMPORARY) )
		{
			Log::Warn("Entity %s uses a deprecated classtype — use the class ^5%s^* instead", etos( entity ), spawnDescription->replacement );
		}
	}
	entity->classname = BG_strdup( spawnDescription->replacement );
	return true;
}

static bool G_ValidateEntity( entityClassDescriptor_t *entityClass, gentity_t *entity )
{
	bool hasTarget = entity->mapEntity.callTargetCount ||
		( entity->mapEntity.shaderKey && entity->mapEntity.shaderReplacement );

	switch (entityClass->chainType) {
		case CHAIN_ACTIVE:
			if ( !hasTarget ) //check target usage for backward compatibility
			{
				if( g_debugEntities.Get() > -2 )
					Log::Warn("Entity %s needs to call or target to something — Removing it.", etos( entity ) );
				return false;
			}
			break;
		case CHAIN_TARGET:
		case CHAIN_PASSIV:
			if(!entity->mapEntity.names[0])
			{
				if( g_debugEntities.Get() > -2 )
					Log::Warn("Entity %s needs a name, so other entities can target it — Removing it.", etos( entity ) );
				return false;
			}
			break;
		case CHAIN_RELAY:
			if ( !hasTarget //check target usage for backward compatibility
					|| !entity->mapEntity.names[0] )
			{
				if( g_debugEntities.Get() > -2 )
					Log::Warn("Entity %s needs a name as well as a target to conditionally relay the firing — Removing it.", etos( entity ) );
				return false;
			}
			break;
		default:
			break;
	}

	return true;
}

static entityClass_t entityClasses[ARRAY_LEN(entityClassDescriptions)];

/*
===============
G_CallSpawnFunction

Finds the spawn function for the entity and calls it,
returning false if not found
===============
*/
static bool G_CallSpawnFunction( gentity_t *spawnedEntity )
{
	entityClassDescriptor_t     *spawnedClass;
	buildable_t buildable;

	if ( !spawnedEntity->classname )
	{
		//don't even warn about spawning-errors with -2 (maps might still work at least partly if we ignore these willingly)
		if ( g_debugEntities.Get() > -2 )
			Log::Warn("Entity ^5#%i^* is missing classname – we are unable to spawn it.", spawnedEntity->num() );
		return false;
	}

	//check buildable spawn functions
	buildable = BG_BuildableByEntityName( spawnedEntity->classname )->number;

	if ( buildable != BA_NONE )
	{
		const buildableAttributes_t *attr = BG_Buildable( buildable );

		// don't spawn built-in buildings if we are using a custom layout
		if ( level.layout[ 0 ] && Q_stricmp( level.layout, S_BUILTIN_LAYOUT ) )
		{
			return false;
		}

		if ( buildable == BA_A_SPAWN || buildable == BA_H_SPAWN )
		{
			spawnedEntity->s.angles[ YAW ] += 180.0f;
			AngleNormalize360( spawnedEntity->s.angles[ YAW ] );
		}

		G_SpawnBuildable( spawnedEntity, buildable );
		level.team[ attr->team ].layoutBuildPoints += attr->buildPoints;

		return true;
	}

	// check the spawn functions for other classes
	spawnedClass = (entityClassDescriptor_t*) bsearch( spawnedEntity->classname, entityClassDescriptions, ARRAY_LEN( entityClassDescriptions ),
	             sizeof( entityClassDescriptor_t ), cmdcmp );

	if ( spawnedClass )
	{ // found it

		spawnedEntity->mapEntity.eclass = &entityClasses[(int) (spawnedClass-entityClassDescriptions)];
		spawnedEntity->mapEntity.eclass->instanceCounter++;

		if(!G_ValidateEntity( spawnedClass, spawnedEntity ))
			return false; // results in freeing the entity

		spawnedClass->spawn( spawnedEntity );
		spawnedEntity->spawned = true;

		if ( g_debugEntities.Get() > 2 )
		{
			std::string count = spawnedEntity->mapEntity.eclass ? std::to_string(spawnedEntity->mapEntity.eclass->instanceCounter) : "??";
			Log::Notice("Successfully spawned entity ^5#%i^* as ^3#%s^*th instance of ^5%s",
			            spawnedEntity->num(), count, spawnedClass->name);
		}

		/*
		 *  to allow each spawn function to test and handle for itself,
		 *  we handle it automatically *after* the spawn (but before it's use/reset)
		 */
		return G_HandleEntityVersions( spawnedClass, spawnedEntity );
	}

	//don't even warn about spawning-errors with -2 (maps might still work at least partly if we ignore these willingly)
	if ( g_debugEntities.Get() > -2 )
	{
		if (!Q_stricmp(S_WORLDSPAWN, spawnedEntity->classname))
		{
			Log::Warn("a ^5" S_WORLDSPAWN "^* class was misplaced into position ^5#%i^* of the spawn string – Ignoring", spawnedEntity->num() );
		}
		else
		{
			Log::Warn("Unknown entity class \"^5%s^*\".", spawnedEntity->classname );
		}
	}

	return false;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *G_NewString( const char *string )
{
	char *newb, *new_p;
	size_t l = strlen( string ) + 1;

	newb = (char*) BG_Alloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for ( size_t i = 0; i < l; i++ )
	{
		if ( string[ i ] == '\\' && i < l - 1 )
		{
			i++;

			if ( string[ i ] == 'n' )
			{
				*new_p++ = '\n';
			}
			else
			{
				*new_p++ = '\\';
			}
		}
		else
		{
			*new_p++ = string[ i ];
		}
	}

	return newb;
}

/*
=============
G_NewTarget
=============
*/
static gentityCallDefinition_t G_NewCallDefinition( const char *eventKey, const char *string )
{
	char *stringPointer;
	gentityCallDefinition_t newCallDefinition = { nullptr, ON_DEFAULT, nullptr, nullptr, ECA_NOP };

	size_t stringLength = strlen( string ) + 1;
	if ( stringLength == 1 )
		return newCallDefinition;

	stringPointer = (char*) BG_Alloc( stringLength );
	newCallDefinition.name = stringPointer;

	for ( size_t i = 0; i < stringLength; i++ )
	{
		if ( string[ i ] == ':' && !newCallDefinition.action )
		{
			*stringPointer++ = '\0';
			newCallDefinition.action = stringPointer;
			continue;
		}
		*stringPointer++ = string[ i ];
	}
	newCallDefinition.actionType = G_GetCallActionTypeFor( newCallDefinition.action );

	newCallDefinition.event = eventKey;
	newCallDefinition.eventType = G_GetCallEventTypeFor( newCallDefinition.event );
	return newCallDefinition;
}

/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
static void G_ParseField( const char *key, const char *rawString, gentity_t *entity )
{
	fieldDescriptor_t *fieldDescriptor;
	byte    *entityDataField;
	vec4_t tmpFloatData = {};
	variatingTime_t varTime = {0, 0};

	fieldDescriptor = (fieldDescriptor_t*) bsearch( key, fields, ARRAY_LEN( fields ), sizeof( fieldDescriptor_t ), cmdcmp );

	if ( !fieldDescriptor )
	{
		return;
	}

	entityDataField = ( byte * ) entity + fieldDescriptor->offset;

	switch ( fieldDescriptor->type )
	{
		case F_STRING:
			* ( char ** ) entityDataField = G_NewString( rawString );
			break;

		case F_TARGET:
			if(entity->mapEntity.targetCount >= MAX_ENTITY_TARGETS)
				Sys::Drop("Maximal number of %i targets reached.", MAX_ENTITY_TARGETS);

			( ( char ** ) entityDataField ) [ entity->mapEntity.targetCount++ ] = G_NewString( rawString );
			break;

		case F_CALLTARGET:
			if(entity->mapEntity.callTargetCount >= MAX_ENTITY_CALLTARGETS)
				Sys::Drop("Maximal number of %i calltargets reached. You can solve this by using a Relay.", MAX_ENTITY_CALLTARGETS);

			( ( gentityCallDefinition_t * ) entityDataField ) [ entity->mapEntity.callTargetCount++ ] = G_NewCallDefinition( fieldDescriptor->replacement ? fieldDescriptor->replacement : fieldDescriptor->name, rawString );
			break;

		case F_TIME:
			sscanf( rawString, "%f %f", &varTime.time, &varTime.variance );
			* ( variatingTime_t * ) entityDataField = varTime;
			break;

		case F_3D_VECTOR:
			sscanf( rawString, "%f %f %f", &tmpFloatData[ 0 ], &tmpFloatData[ 1 ], &tmpFloatData[ 2 ] );

			( ( float * ) entityDataField ) [ 0 ] = tmpFloatData[ 0 ];
			( ( float * ) entityDataField ) [ 1 ] = tmpFloatData[ 1 ];
			( ( float * ) entityDataField ) [ 2 ] = tmpFloatData[ 2 ];
			break;

		case F_4D_VECTOR:
			sscanf( rawString, "%f %f %f %f", &tmpFloatData[ 0 ], &tmpFloatData[ 1 ], &tmpFloatData[ 2 ], &tmpFloatData[ 3 ] );

			( ( float * ) entityDataField ) [ 0 ] = tmpFloatData[ 0 ];
			( ( float * ) entityDataField ) [ 1 ] = tmpFloatData[ 1 ];
			( ( float * ) entityDataField ) [ 2 ] = tmpFloatData[ 2 ];
			( ( float * ) entityDataField ) [ 3 ] = tmpFloatData[ 3 ];
			break;

		case F_INT:
			* ( int * )   entityDataField = atoi( rawString );
			break;

		case F_FLOAT:
			* ( float * ) entityDataField = atof( rawString );
			break;

		case F_YAW:
			( ( float * ) entityDataField ) [ PITCH ] = 0;
			( ( float * ) entityDataField ) [ YAW   ] = atof( rawString );
			( ( float * ) entityDataField ) [ ROLL  ] = 0;
			break;

		case F_SOUNDINDEX:
			if ( strlen( rawString ) >= MAX_QPATH )
			{
				Sys::Drop( "Sound filename %s in field %s of %s exceeds MAX_QPATH", rawString, fieldDescriptor->name, etos( entity ) );
			}

			* ( int * ) entityDataField  = G_SoundIndex( rawString );
			break;

		default:
			Log::Warn("unknown datatype %i for field %s", fieldDescriptor->type, fieldDescriptor->name );
			break;
	}

	if ( fieldDescriptor->replacement && fieldDescriptor->versionState )
		G_WarnAboutDeprecatedEntityField(entity, fieldDescriptor->replacement, key, fieldDescriptor->versionState );
}

static std::unordered_map< std::string, int > autoIdCounters;

static void ResetAutomaticEntityIdState()
{
	autoIdCounters.clear();
}

static void SetAutomaticEntityId( gentity_t *spawningEntity )
{
	// generate automatic IDs for some map entities, in case the map does not set IDs.
	// these IDs are persistent, they do not change when loading a map with or
	// without a layout.
	// we are using a simple scheme for persistence. for instance, consider
	// a map with 3 func_door entities, each without an ID assigned by the map.
	// they will receive the automatic IDs: func_door_0, func_door_1, func_door_2
	// their order in the entity lump / .ent file determines which entity comes first
	// in this numbering.
	// if we want to add another func_door entity to this map, and keep persistence,
	// we have to add it after the already existing func_door entities in
	// the entity lump / .ent file.
	// ideas for a method that would keep persistence in more cases:
	// - method 1:
	//   sort the entities by their positions in space
	//   use the average of vertices for entities with models in the BSP
	//   break ties by sorting by all properties
	//   (overlapping entities of the same kind have been observed in several maps)
	// - method 2:
	//   calculate a hash from the entity description text
	//   convert the hash to something human readable, use this as ID postfix
	// the more complicated methods are probably not worth it, as we can always assign
	// IDs of our own choosing in the entity lump / .ent file.
	std::string classname = spawningEntity->classname;
	int counter = autoIdCounters[ classname ]++;
	if ( spawningEntity->id == nullptr )
	{
		std::string autoId = Str::Format( "%s_%d", classname, counter );
		spawningEntity->id = G_NewString( autoId.c_str() );
	}
}

/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
static void G_SpawnGEntityFromSpawnVars()
{
	int       i, j;
	gentity_t *spawningEntity;

	// get the next free entity
	spawningEntity = G_NewEntity( NO_CBSE );

	for ( i = 0; i < level.numSpawnVars; i++ )
	{
		G_ParseField( level.spawnVars[ i ][ 0 ], level.spawnVars[ i ][ 1 ], spawningEntity );
	}

	if(G_SpawnBoolean( "nop", false ) || G_SpawnBoolean( "notunv", false ))
	{
		G_FreeEntity( spawningEntity );
		return;
	}

	/*
	 * will have only the classname or missing it…
	 * both aren't helping us and might even create a error later
	 * in the server, where we dont know as much anymore about it,
	 * so we fail rather here, so mappers have a chance to remove it
	 */
	if( level.numSpawnVars <= 1 )
	{
		if ( level.numSpawnVars == 1 )
		{
			Log::Warn("encountered ghost-entity #%i with only one field: %s = %s", spawningEntity->num(), level.spawnVars[ 0 ][ 0 ], level.spawnVars[ 0 ][ 1 ] );
		}
		else
		{
			Log::Warn("encountered ghost entity #%i with no fields", spawningEntity->num());
		}
		G_FreeEntity( spawningEntity );
		return;
	}

	// move editor origin to pos
	VectorCopy( spawningEntity->s.origin, spawningEntity->s.pos.trBase );
	VectorCopy( spawningEntity->s.origin, spawningEntity->r.currentOrigin );

	// don't leave any "gaps" between multiple names
	j = 0;
	for (i = 0; i < MAX_ENTITY_ALIASES; ++i)
	{
		if (spawningEntity->mapEntity.names[i])
			spawningEntity->mapEntity.names[j++] = spawningEntity->mapEntity.names[i];
	}
	spawningEntity->mapEntity.names[ j ] = nullptr;

	/*
	 * for backward compatbility, since before targets were used for calling,
	 * we'll have to copy them over to the called-targets as well for now
	 */
	if(!spawningEntity->mapEntity.callTargetCount)
	{
		for (i = 0; i < MAX_ENTITY_TARGETS && i < MAX_ENTITY_CALLTARGETS; i++)
		{
			if (!spawningEntity->mapEntity.targets[i])
				continue;

			spawningEntity->mapEntity.calltargets[i].event = "target";
			spawningEntity->mapEntity.calltargets[i].eventType = ON_DEFAULT;
			spawningEntity->mapEntity.calltargets[i].actionType = ECA_DEFAULT;
			spawningEntity->mapEntity.calltargets[i].name = spawningEntity->mapEntity.targets[i];
			spawningEntity->mapEntity.callTargetCount++;
		}
	}

	// don't leave any "gaps" between multiple targets
	j = 0;
	for (i = 0; i < MAX_ENTITY_TARGETS; ++i)
	{
		if (spawningEntity->mapEntity.targets[i])
			spawningEntity->mapEntity.targets[j++] = spawningEntity->mapEntity.targets[i];
	}
	spawningEntity->mapEntity.targets[ j ] = nullptr;

	// if we didn't get necessary fields (like the classname), don't bother spawning anything
	if ( !G_CallSpawnFunction( spawningEntity ) )
	{
		G_FreeEntity( spawningEntity );
		return;
	}

	// if the entitiy removed itself, we are done
	if ( !spawningEntity->inuse )
	{
		return;
	}

	SetAutomaticEntityId( spawningEntity );

	if ( spawningEntity->id != nullptr )
	{
		G_RegisterEntityId( spawningEntity->num(), spawningEntity->id );
	}
}

bool G_WarnAboutDeprecatedEntityField( gentity_t *entity, const char *expectedFieldname, const char *actualFieldname, const int typeOfDeprecation  )
{
	if ( !Q_stricmp(expectedFieldname, actualFieldname) || typeOfDeprecation == ENT_V_CURRENT )
		return false;

	if ( g_debugEntities.Get() >= 0 ) //dont't warn about anything with -1 or lower
	{
		if ( typeOfDeprecation < ENT_V_TMPORARY || g_debugEntities.Get() >= 1 )
		{
			Log::Warn("Entity ^5#%i^* contains deprecated field ^5%s^* — use ^5%s^* instead", entity->num(), actualFieldname, expectedFieldname );
		}
	}

	return true;
}

/*
====================
G_AddSpawnVarToken
====================
*/
static char *G_AddSpawnVarToken( const char *string )
{
	int  l;
	char *dest;

	l = strlen( string );

	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS )
	{
		Sys::Drop( "G_AddSpawnVarToken: MAX_SPAWN_VARS_CHARS" );
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy( dest, string, l + 1 );

	level.numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
static bool G_ParseSpawnVars( const char** entString )
{
	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	const char* entityString = *entString;
	const char* token;

	// parse the opening brace
	token = COM_Parse( &entityString );
	if ( !token || token[0] == '\0' )
	{
		// end of spawn string
		return false;
	}

	if ( token[ 0 ] != '{' )
	{
		Sys::Drop( "G_ParseSpawnVars: found %s when expecting {", token );
	}

	// go through all the key / value pairs
	while ( 1 )
	{
		// parse key
		token = COM_Parse( &entityString );
		if ( !token )
		{
			Sys::Drop( "G_ParseSpawnVars: EOF without closing brace" );
		}

		const std::string key { token };
		if ( key[ 0 ] == '}' )
		{
			break;
		}

		// parse value
		token = COM_Parse( &entityString );
		if ( !token )
		{
			Sys::Drop( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( token[ 0 ] == '}' )
		{
			Sys::Drop( "G_ParseSpawnVars: closing brace without data" );
		}

		if ( level.numSpawnVars == MAX_SPAWN_VARS )
		{
			Sys::Drop( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}

		level.spawnVars[ level.numSpawnVars ][ 0 ] = G_AddSpawnVarToken( key.c_str() );
		level.spawnVars[ level.numSpawnVars ][ 1 ] = G_AddSpawnVarToken( token );
		level.numSpawnVars++;
	}

	*entString = entityString;
	return true;
}

// The callbacks don't work until BG_InitAllConfigs()
static void InitDisabledItemCvars()
{
	static Cvar::Callback<Cvar::Cvar<std::string>> g_disabledEquipment(
		"g_disabledEquipment",
		"Forbidden weapons and gear humans can buy, example: " QQ("lcannon, flamer, gren, firebomb, bsuit, larmour"),
		Cvar::SERVERINFO,
		"", // everything is allowed by default
		BG_SetForbiddenEquipment
		);
	static Cvar::Callback<Cvar::Cvar<std::string>> g_disabledClasses(
		"g_disabledClasses",
		"Forbidden alien classes, like " QQ("level3,level3upg,builder"),
		Cvar::SERVERINFO,
		"", // everything is allowed by default
		BG_SetForbiddenClasses
		);
	static Cvar::Callback<Cvar::Cvar<std::string>> g_disabledBuildables(
		"g_disabledBuildables",
		"Forbidden (human and alien) buildings, like " QQ("acid_tube, barricade, medistat, drill, mgturret, rocketpod"),
		Cvar::SERVERINFO,
		"", // everything is allowed by default
		BG_SetForbiddenBuildables
		);

	G_SpawnStringIntoCVar( "disabledEquipment", g_disabledEquipment );
	G_SpawnStringIntoCVar( "disabledClasses", g_disabledClasses );
	G_SpawnStringIntoCVar( "disabledBuildables", g_disabledBuildables );
}

static std::string defaultTacticBehaviors = "default, defend, attack, stay_here, follow";
static void InitTacticBehaviorsCvar()
{
	static Cvar::Callback<Cvar::Cvar<std::string>> g_tacticBehaviors(
	   "g_tacticBehaviors",
		"Allowed behaviors for the tactic command, example: " QQ("default, camper, reckless"),
		Cvar::NONE,
		defaultTacticBehaviors.c_str(),
		BG_SetTacticBehaviors
		);
	static bool alreadyInitialized = false;
	if ( !alreadyInitialized )
	{
		BG_SetTacticBehaviors( defaultTacticBehaviors.c_str() );
		alreadyInitialized = true;
	}
}

/**
 * Warning: The following comment contains information, that might be parsed and used by radiant based mapeditors.
 */
/*QUAKED worldspawn (0 0 0) ?
Used for game-world global options.
Every map should have exactly one.

=== KEYS ===
; message: Text to print during connection process. Used for the name of level.
; music: path/name of looping sound file used for level's music (eg. music/sonic5).
; gravity: level gravity [g_gravity (800)]

; humanBuildPoints: maximum amount of power the humans can use. [g_humanBuildPoints]
; alienBuildPoints: maximum amount of sentience available to the overmind. [g_alienBuildPoints]

; disabledEquipment: A comma delimited list of human weapons or upgrades to disable for this map. [g_disabledEquipment ()]
; disabledClasses: A comma delimited list of alien classes to disable for this map. [g_disabledClasses ()]
; disabledBuildables: A comma delimited list of buildables to disable for this map. [g_disabledBuildables ()]
*/
static void SP_worldspawn()
{
	char *s;

	G_SpawnString( "classname", "", &s );

	if ( Q_stricmp( s, S_WORLDSPAWN ) )
	{
		Sys::Drop( "SP_worldspawn: The first entry in the spawn string isn't of expected type '" S_WORLDSPAWN "'" );
	}

	// make some data visible to connecting client
	trap_SetConfigstring( CS_GAME_VERSION, GAME_VERSION );

	trap_SetConfigstring( CS_LEVEL_START_TIME, va( "%i", level.startTime ) );

	G_SpawnString( "music", "", &s );
	trap_SetConfigstring( CS_MUSIC, s );


	G_SpawnString( "message", "", &s );
	trap_SetConfigstring( CS_MESSAGE, s );  // map specific message

	if(G_SpawnString( "gradingTexture", "", &s ))
		trap_SetConfigstring( CS_GRADING_TEXTURES, va( "%i %f %s", -1, 0.0f, s ) );

	if(G_SpawnString( "colorGrade", "", &s )) {
		Log::Warn("\"colorGrade\" deprecated. Please use \"gradingTexture\"");
		trap_SetConfigstring( CS_GRADING_TEXTURES, va( "%i %f %s", -1, 0.0f, s ) );
	}

	if ( G_SpawnString( "reverbEffect", "", &s ) ) {
		float reverbIntensity;
		G_SpawnFloat( "reverbIntensity", "1", &reverbIntensity );
		reverbIntensity = Math::Clamp( reverbIntensity, 0.0f, 2.0f );
		
		trap_SetConfigstring( CS_REVERB_EFFECTS, va( "%i %f %s %f", 0, 0.0f, s, reverbIntensity ) );
	}

	trap_SetConfigstring( CS_MOTD, g_motd.Get().c_str() );  // message of the day

	G_SpawnStringIntoCVar( "gravity", g_gravity );

	G_SpawnStringIntoCVar( "humanAllowBuilding", g_humanAllowBuilding );
	G_SpawnStringIntoCVar( "alienAllowBuilding", g_alienAllowBuilding );

	G_SpawnStringIntoCVar( "BPInitialBudget", g_buildPointInitialBudget );
	G_SpawnStringIntoCVar( "BPInitialBudgetHumans", g_BPInitialBudgetHumans );
	G_SpawnStringIntoCVar( "BPInitialBudgetAliens", g_BPInitialBudgetAliens );
	G_SpawnStringIntoCVar( "BPBudgetPerMiner", g_buildPointBudgetPerMiner );
	G_SpawnStringIntoCVar( "BPRecoveryRateHalfLife", g_buildPointRecoveryRateHalfLife );

	InitDisabledItemCvars();
	InitTacticBehaviorsCvar();

	g_entities[ ENTITYNUM_WORLD ].s.number = ENTITYNUM_WORLD;
	g_entities[ ENTITYNUM_WORLD ].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ ENTITYNUM_WORLD ].classname = BG_strdup( S_WORLDSPAWN );

	g_entities[ ENTITYNUM_NONE ].s.number = ENTITYNUM_NONE;
	g_entities[ ENTITYNUM_NONE ].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ ENTITYNUM_NONE ].classname = BG_strdup( "nothing" );

	// see if we want a warmup time
	trap_SetConfigstring( CS_WARMUP, "-1" );

	if ( g_doWarmup.Get() )
	{
		level.warmupTime = level.matchTime + ( g_warmup.Get() * 1000 );
		trap_SetConfigstring( CS_WARMUP, va( "%i", level.warmupTime ) );
		G_LogPrintf( "Warmup: %i", g_warmup.Get() );
	}

	level.timelimit = g_timelimit.Get();
}

/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString()
{
	level.numSpawnVars = 0;
	ResetAutomaticEntityIdState();

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	const char* entityString = CM_EntityString();
	if ( !G_ParseSpawnVars( &entityString ) )
	{
		Sys::Drop( "SpawnEntities: no entities" );
	}

	SP_worldspawn();

	// parse ents
	while ( G_ParseSpawnVars( &entityString ) )
	{
		G_SpawnGEntityFromSpawnVars();
	}
}

void G_SpawnFakeEntities()
{
	/* The previous outOfRange value of 1.7e19f was doing an overflow
	when calling VectorLengthSquared( eloc->r.currentOrigin )
	in GetCloseLocationEntity() from sg_team.cpp. */
	float outOfRange = HUGE_DISTANCE;

	level.fakeLocation = G_NewEntity( NO_CBSE );
	level.fakeLocation->s.origin[ 0 ] =
	level.fakeLocation->s.origin[ 1 ] =
	level.fakeLocation->s.origin[ 2 ] = outOfRange;
	level.fakeLocation->mapEntity.message = nullptr;

	level.fakeLocation->s.eType = entityType_t::ET_LOCATION;
	level.fakeLocation->r.svFlags = SVF_BROADCAST;

	level.fakeLocation->nextPathSegment = level.locationHead;
	level.fakeLocation->s.generic1 = 0;
	level.locationHead = level.fakeLocation;

	G_SetOrigin( level.fakeLocation, VEC2GLM( level.fakeLocation->s.origin ) );
}
