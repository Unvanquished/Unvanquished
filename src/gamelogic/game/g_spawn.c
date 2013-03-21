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

#include "g_local.h"
#include "g_spawn.h"

qboolean G_SpawnString( const char *key, const char *defaultString, char **out )
{
	int i;

	if ( !level.spawning )
	{
		*out = ( char * ) defaultString;
//    G_Error( "G_SpawnString() called while not spawning" );
		return qfalse;
	}

	for ( i = 0; i < level.numSpawnVars; i++ )
	{
		if ( !Q_stricmp( key, level.spawnVars[ i ][ 0 ] ) )
		{
			*out = level.spawnVars[ i ][ 1 ];
			return qtrue;
		}
	}

	*out = ( char * ) defaultString;
	return qfalse;
}

qboolean  G_SpawnFloat( const char *key, const char *defaultString, float *out )
{
	char     *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atof( s );
	return present;
}

qboolean G_SpawnInt( const char *key, const char *defaultString, int *out )
{
	char     *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

qboolean  G_SpawnVector( const char *key, const char *defaultString, float *out )
{
	char     *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[ 0 ], &out[ 1 ], &out[ 2 ] );
	return present;
}

qboolean  G_SpawnVector4( const char *key, const char *defaultString, float *out )
{
	char     *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f %f", &out[ 0 ], &out[ 1 ], &out[ 2 ], &out[ 3 ] );
	return present;
}

//
// fields are needed for spawning from the entity string
//
typedef enum
{
  F_INT,
  F_FLOAT,
  F_STRING,
  F_CALLTARGET,
  F_TIME,
  F_3D_VECTOR,
  F_4D_VECTOR,
  F_YAW
} fieldtype_t;

typedef struct
{
	char  *name;
	size_t      offset;
	fieldtype_t type;
	int   versionState;
	char  *replacement;
} field_t;

static const field_t fields[] =
{
	{ "acceleration",        FOFS( acceleration ),        F_3D_VECTOR },
	{ "alias",               FOFS( names[ 2 ] ),          F_STRING    },
	{ "alpha",               FOFS( restingPosition ),     F_3D_VECTOR }, // What's with the variable abuse everytime?
	{ "amount",              FOFS( config.amount ),       F_INT       },
	{ "angle",               FOFS( s.angles ),            F_YAW,      ENT_V_TMPNAME, "yaw"}, //radiants ui sadly strongly encourages the "angle" keyword
	{ "angles",              FOFS( s.angles ),            F_3D_VECTOR },
	{ "animation",           FOFS( animation ),           F_4D_VECTOR },
	{ "bounce",              FOFS( physicsBounce ),       F_FLOAT     },
	{ "classname",           FOFS( classname ),           F_STRING    },
	{ "dmg",                 FOFS( config.damage ),       F_INT       },
	{ "health",              FOFS( config.health ),       F_INT       },
	{ "message",             FOFS( message ),             F_STRING    },
	{ "model",               FOFS( model ),               F_STRING    },
	{ "model2",              FOFS( model2 ),              F_STRING    },
	{ "name",	        	 FOFS( names[ 0 ] ),          F_STRING	  },
	{ "origin",              FOFS( s.origin ),            F_3D_VECTOR },
	{ "radius",              FOFS( activatedPosition ),   F_3D_VECTOR }, // What's with the variable abuse everytime?
	{ "random",              FOFS( config.wait.variance ),F_FLOAT,    ENT_V_COMBINED, "wait" },
	{ "spawnflags",          FOFS( spawnflags ),          F_INT       },
	{ "speed",               FOFS( config.speed ),        F_FLOAT     },
	{ "stage",               FOFS( conditions.stage ),    F_INT       },
	{ "target",              FOFS( calltargets ),         F_CALLTARGET },
	{ "target2",             FOFS( calltargets ),         F_CALLTARGET },
	{ "target3",             FOFS( calltargets ),         F_CALLTARGET },
	{ "target4",             FOFS( calltargets ),         F_CALLTARGET },
	{ "targetname",			 FOFS( names[ 1 ] ),          F_STRING,	  ENT_V_RENAMED, "name"},
	{ "targetShaderName",    FOFS( targetShaderName ),    F_STRING    },
	{ "targetShaderNewName", FOFS( targetShaderNewName ), F_STRING    },
	{ "team",                FOFS( conditions.team ),     F_INT       },
	{ "wait",                FOFS( config.wait ),         F_TIME      },
	{ "yaw",                 FOFS( s.angles ),            F_YAW       },
};

typedef enum
{
	/*
	 * self sufficent, it might possibly be fired at, but it can do as well on its own, so won't be freed automaticly
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
} entityChainType_t;

typedef struct
{
	const char *name;
	void ( *spawn )( gentity_t *entityToSpawn );
	const entityChainType_t chainType;

	//optional spawn-time data
	const int	versionState;
	const char  *replacement;
} entityClassDescriptor_t;


static const entityClassDescriptor_t entityClassDescriptions[] =
{
	/**
	 *
	 *	Control entities
	 *	================
	 *
	 */
	{ "ctrl_limited",             SP_ctrl_limited,           CHAIN_RELAY,      ENT_V_CURRENT, NULL  },
	{ "ctrl_relay",               SP_ctrl_relay,             CHAIN_RELAY,      ENT_V_CURRENT, NULL  },

	/**
	 *
	 *	Environment entities
	 *	====================
	 *  the afx subgroup describes environment area effects, which in most cases should be client predictable
	 */
	{ "env_afx_ammo",             SP_env_afx_ammo,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "env_afx_gravity",          SP_env_afx_gravity,        CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "env_afx_heal",             SP_env_afx_heal,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "env_afx_hurt",             SP_env_afx_hurt,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "env_afx_push",             SP_env_afx_push,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "env_afx_teleport",         SP_env_afx_teleport,       CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "env_animated_model",       SP_env_animated_model,     CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "env_lens_flare",           SP_env_lens_flare,         CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "env_particle_system",      SP_env_particle_system,    CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "env_portal_camera",        SP_env_portal_camera,      CHAIN_TARGET,     ENT_V_UNCLEAR, NULL },
	{ "env_portal_surface",       SP_env_portal_surface,     CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "env_rumble",               SP_env_rumble,             CHAIN_PASSIV,     ENT_V_UNCLEAR, NULL },
	{ "env_speaker",              SP_env_speaker,            CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },

	/**
	 *
	 *	Functional entities
	 *	====================
	 *
	 */
	{ "func_bobbing",             SP_func_bobbing,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "func_button",              SP_func_button,            CHAIN_ACTIVE,     ENT_V_UNCLEAR, NULL },
	{ "func_destructable",        SP_func_destructable,      CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "func_door",                SP_func_door,              CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "func_door_model",          SP_func_door_model,        CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "func_door_rotating",       SP_func_door_rotating,     CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "func_dynamic",             SP_func_dynamic,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "func_group",               SP_RemoveSelf,             0,                ENT_V_UNCLEAR, NULL },
	{ "func_pendulum",            SP_func_pendulum,          CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "func_plat",                SP_func_plat,              CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "func_rotating",            SP_func_rotating,          CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "func_spawn",               SP_func_spawn,             CHAIN_PASSIV,     ENT_V_UNCLEAR, NULL },
	{ "func_static",              SP_func_static,            CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "func_timer",               SP_sensor_timer,			 CHAIN_ACTIVE,     ENT_V_TMPNAME, "sensor_timer" },
	{ "func_train",               SP_func_train,             CHAIN_ACTIVE,     ENT_V_UNCLEAR, NULL },

	/**
	 *
	 *	Game entities
	 *	=============
	 *
	 */
	{ "game_score",               SP_game_score,             CHAIN_PASSIV,     ENT_V_CURRENT, NULL },
	{ "game_end",                 SP_game_end,               CHAIN_PASSIV,     ENT_V_CURRENT, NULL },

	/**
	 * former information and misc entities
	 */
	{ "info_alien_intermission",  SP_Nothing,                CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "pos_alien_intermission"  },
	{ "info_human_intermission",  SP_Nothing,                CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "pos_human_intermission"  },
	{ "info_notnull",             SP_pos_target,             CHAIN_TARGET,     ENT_V_TMPNAME, "pos_target" },
	{ "info_null",                SP_RemoveSelf,             0,                ENT_V_UNCLEAR, NULL },
	{ "info_player_deathmatch",   SP_pos_player_spawn,       CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "pos_player_spawn" },
	{ "info_player_intermission", SP_Nothing,                CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "pos_player_intermission" },
	{ "info_player_start",        SP_pos_player_spawn,       CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "pos_player_spawn" },
	{ "light",                    SP_RemoveSelf,             0,                ENT_V_UNCLEAR, NULL },
	{ "misc_anim_model",          SP_env_animated_model,     CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "env_animated_model" },
	{ "misc_light_flare",         SP_env_lens_flare,         CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "env_lens_flare"},
	{ "misc_model",               SP_RemoveSelf,             0,                ENT_V_UNCLEAR, NULL },
	{ "misc_particle_system",     SP_env_particle_system,    CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "env_particle_system"},
	{ "misc_portal_camera",       SP_env_portal_camera,      CHAIN_TARGET,     ENT_V_TMPNAME, "env_portal_camera" },
	{ "misc_portal_surface",      SP_env_portal_surface,     CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "env_portal_surface" },
	{ "misc_teleporter_dest",     SP_pos_target,             CHAIN_TARGET,     ENT_V_TMPNAME, "pos_target" },

	/**
	 *  Position entities
	 *  =================
	 *  position entities may get used by other entities or other processes as provider for positional data
	 *
	 *  positions may or may not have an additional direction attached to them
	 *  they may also target to another position to indicate that direction
	 */
	{ "path_corner",              SP_Nothing,                CHAIN_TARGET,     ENT_V_UNCLEAR, NULL },
	{ "pos_alien_intermission",   SP_Nothing,                CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "pos_human_intermission",   SP_Nothing,                CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "pos_location",             SP_pos_location,           CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "pos_player_intermission",  SP_Nothing,                CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "pos_player_spawn",         SP_pos_player_spawn,       CHAIN_AUTONOMOUS, ENT_V_UNCLEAR, NULL },
	{ "pos_target",               SP_pos_target,             CHAIN_TARGET,     ENT_V_UNCLEAR, NULL },

	/**
	 *  Sensors
	 *  =======
	 *  Sensor fire an event (usually towards targets) when aware
	 *  of another entity, event, or gamestate (timer and start being aware of the game start).
	 *  Sensors often can be targeted to toggle (activate or deactivate)
	 *  their function of perceiving other entities.
	 */

	{ "sensor_buildable",         SP_sensor_buildable,       CHAIN_ACTIVE,     ENT_V_UNCLEAR, NULL },
	{ "sensor_end",               SP_sensor_end,             CHAIN_ACTIVE,     ENT_V_CURRENT, NULL },
	{ "sensor_player",            SP_sensor_player,          CHAIN_ACTIVE,     ENT_V_UNCLEAR, NULL },
	{ "sensor_stage",             SP_sensor_stage,           CHAIN_ACTIVE,     ENT_V_CURRENT, NULL },
	{ "sensor_start",             SP_sensor_start,           CHAIN_ACTIVE,     ENT_V_CURRENT, NULL },
	{ "sensor_timer",             SP_sensor_timer,           CHAIN_ACTIVE,     ENT_V_UNCLEAR, NULL },

	/**
	 *
	 * 	Target Entities
	 * 	===============
	 * 	Targets perform no action by themselves.
	 *	Instead they are targeted by other entities,
	 *	like being triggered by a trigger_ entity.
	 *
	 */
	{ "target_alien_win",         SP_target_alien_win,       CHAIN_PASSIV,     ENT_V_TMPNAME, "game_end" },
	{ "target_delay",             SP_ctrl_relay,             CHAIN_RELAY,      ENT_V_TMPNAME, "ctrl_relay" },
	{ "target_human_win",         SP_target_human_win,       CHAIN_PASSIV,     ENT_V_TMPNAME, "game_end" },
	{ "target_hurt",              SP_target_hurt,            CHAIN_PASSIV,     ENT_V_UNCLEAR, NULL },
	{ "target_kill",              SP_target_kill,            CHAIN_PASSIV,     ENT_V_UNCLEAR, NULL },
	{ "target_location",          SP_pos_location,           CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "pos_location" },
	{ "target_position",          SP_pos_target,             CHAIN_TARGET,     ENT_V_TMPNAME, "pos_target" },
	{ "target_print",             SP_target_print,           CHAIN_PASSIV,     ENT_V_UNCLEAR, NULL },
	{ "target_push",              SP_target_push,            CHAIN_PASSIV,     ENT_V_UNCLEAR, NULL },
	{ "target_relay",             SP_ctrl_relay,             CHAIN_RELAY,      ENT_V_TMPNAME, "ctrl_relay" },
	{ "target_rumble",            SP_env_rumble,             CHAIN_PASSIV,     ENT_V_TMPNAME, "env_rumble" },
	{ "target_score",             SP_game_score,             CHAIN_PASSIV,     ENT_V_TMPNAME, "game_score" },
	{ "target_speaker",           SP_env_speaker,            CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "env_speaker" },
	{ "target_teleporter",        SP_target_teleporter,      CHAIN_PASSIV,     ENT_V_UNCLEAR, NULL },

	/**
	 * former trigger
	 */
	{ "trigger_always",           SP_sensor_start,           CHAIN_ACTIVE,     ENT_V_RENAMED, "sensor_start" },
	{ "trigger_ammo",             SP_env_afx_ammo,           CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "env_afx_ammo" },
	{ "trigger_buildable",        SP_sensor_buildable,       CHAIN_ACTIVE,     ENT_V_TMPNAME, "sensor_buildable" },
	{ "trigger_class",            SP_sensor_player,          CHAIN_ACTIVE,     ENT_V_TMPNAME, "sensor_player" },
	{ "trigger_equipment",        SP_sensor_player,          CHAIN_ACTIVE,     ENT_V_TMPNAME, "sensor_player" },
	{ "trigger_gravity",          SP_env_afx_gravity,        CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "env_afx_gravity" },
	{ "trigger_heal",             SP_env_afx_heal,           CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "env_afx_heal" },
	{ "trigger_hurt",             SP_env_afx_hurt,           CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "env_afx_hurt" },
	{ "trigger_multiple",         SP_sensor_player,          CHAIN_ACTIVE,     ENT_V_TMPNAME, "sensor_player" },
	{ "trigger_push",             SP_env_afx_push,           CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "env_afx_push" },
	{ "trigger_stage",            SP_sensor_stage,           CHAIN_ACTIVE,     ENT_V_RENAMED, "sensor_stage" },
	{ "trigger_teleport",         SP_env_afx_teleport,       CHAIN_AUTONOMOUS, ENT_V_TMPNAME, "env_afx_teleport" },
	{ "trigger_win",              SP_sensor_end,             CHAIN_ACTIVE,     ENT_V_TMPNAME, "sensor_end" }
};

qboolean G_HandleEntityVersions( entityClassDescriptor_t *spawnDescription, gentity_t *entity )
{
	if ( spawnDescription->versionState == ENT_V_CURRENT ) // we don't need to handle anything
		return qtrue;

	if ( !spawnDescription->replacement || !Q_stricmp(entity->classname, spawnDescription->replacement))
	{
		if ( g_debugEntities.integer > -2 )
			G_Printf( "^1ERROR: ^7Entity ^5#%i ^7 of type ^5%s ^7has been marked deprecated but no replacement has been supplied\n", entity->s.number, entity->classname );

		return qfalse;
	}

	if ( g_debugEntities.integer >= 0 ) //dont't warn about anything with -1 or lower
	{
		if( spawnDescription->versionState < ENT_V_TMPORARY
		|| ( g_debugEntities.integer >= 1 && spawnDescription->versionState >= ENT_V_TMPORARY) )
		{
			G_Printf( "^3WARNING: ^7Entity ^5#%i ^7is of deprecated type ^5%s^7 — use ^5%s^7 instead\n", entity->s.number, entity->classname, spawnDescription->replacement );
		}
	}
	entity->classname = spawnDescription->replacement;
	return qtrue;
}

qboolean G_ValidateEntity( entityClassDescriptor_t *entityClass, gentity_t *entity )
{
	switch (entityClass->chainType) {
		case CHAIN_ACTIVE:
			if(!entity->callTargetCount)
			{
				if( g_debugEntities.integer > -2 )
					G_Printf( "^3WARNING: ^7Entity ^5#%i^7 of type ^5%s^7 needs to target to something — Removing it.\n", entity->s.number, entity->classname );
				return qfalse;
			}
			break;
		case CHAIN_TARGET:
		case CHAIN_PASSIV:
			if(!entity->names[0])
			{
				if( g_debugEntities.integer > -2 )
					G_Printf( "^3WARNING: ^7Entity ^5#%i^7 of type ^5%s^7 needs a name, so other entities can target it — Removing it.\n", entity->s.number, entity->classname );
				return qfalse;
			}
			break;
		case CHAIN_RELAY:
			if(!entity->callTargetCount || !entity->names[0])
			{
				if( g_debugEntities.integer > -2 )
					G_Printf( "^3WARNING: ^7Entity ^5#%i^7 of type ^5%s^7 needs a name as well as a target to conditionally relay the firing — Removing it.\n", entity->s.number, entity->classname );
				return qfalse;
			}
			break;
		default:
			break;
	}

	return qtrue;
}

static entityClass_t entityClasses[ARRAY_LEN(entityClassDescriptions)];

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean G_CallSpawn( gentity_t *spawnedEntity )
{
	entityClassDescriptor_t     *spawnedClass;
	buildable_t buildable;

	if ( !spawnedEntity->classname )
	{
		//don't even warn about spawning-errors with -2 (maps might still work at least partly if we ignore these willingly)
		if ( g_debugEntities.integer > -2 )
			G_Printf( "^1ERROR: Entity ^5#%i^7 is missing classname – we are unable to spawn it.\n", spawnedEntity->s.number );
		return qfalse;
	}

	//check buildable spawn functions
	buildable = BG_BuildableByEntityName( spawnedEntity->classname )->number;

	if ( buildable != BA_NONE )
	{
		// don't spawn built-in buildings if we are using a custom layout
		if ( level.layout[ 0 ] && Q_stricmp( level.layout, "*BUILTIN*" ) )
		{
			return qfalse;
		}

		if ( buildable == BA_A_SPAWN || buildable == BA_H_SPAWN )
		{
			spawnedEntity->s.angles[ YAW ] += 180.0f;
			AngleNormalize360( spawnedEntity->s.angles[ YAW ] );
		}

		G_SpawnBuildable( spawnedEntity, buildable );
		return qtrue;
	}

	// check the spawn functions for other classes
	spawnedClass = bsearch( spawnedEntity->classname, entityClassDescriptions, ARRAY_LEN( entityClassDescriptions ),
	             sizeof( entityClassDescriptor_t ), cmdcmp );

	if ( spawnedClass )
	{ // found it

		spawnedEntity->eclass = &entityClasses[(int) (spawnedClass-entityClassDescriptions)];
		spawnedEntity->eclass->instanceCounter++;

		if(!G_ValidateEntity( spawnedClass, spawnedEntity ))
			return qfalse; // results in freeing the entity

		spawnedClass->spawn( spawnedEntity );
		spawnedEntity->spawned = qtrue;

		if ( g_debugEntities.integer > 2 )
			G_Printf("Debug: Successfully spawned entity ^5#%i^7 as ^3%i^7th instance of ^5%s\n",
					spawnedEntity->s.number, spawnedEntity->eclass->instanceCounter, spawnedClass->name);

		/*
		 *  to allow each spawn function to test and handle for itself,
		 *  we handle it automatically *after* the spawn (but before it's use/reset)
		 */
		if(!G_HandleEntityVersions( spawnedClass, spawnedEntity ))
			return qfalse;


		return qtrue;
	}

	//don't even warn about spawning-errors with -2 (maps might still work at least partly if we ignore these willingly)
	if ( g_debugEntities.integer > -2 )
	{
		if (!Q_stricmp("worldspawn", spawnedEntity->classname))
		{
			G_Printf( "^1ERROR: ^5%s ^7is not the first but the ^5#%i^7 entry in the spawn string – Some map configurations will not be set.\n", spawnedEntity->classname, spawnedEntity->s.number );
		}
		else
		{
			G_Printf( "^1ERROR: Unknown entity class \"^5%s^7\".\n", spawnedEntity->classname );
		}
	}

	return qfalse;
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
	int  i, l;

	l = strlen( string ) + 1;

	newb = BG_Alloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for ( i = 0; i < l; i++ )
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
gentityCallDefinition_t G_NewCallDefinition( char *eventKey, const char *string )
{
	char *stringPointer;
	int  i, stringLength;
	gentityCallDefinition_t newTarget = { NULL, NULL, NULL, ECA_DEFAULT };

	stringLength = strlen( string ) + 1;
	if(stringLength == 1)
		return newTarget;

	stringPointer = BG_Alloc( stringLength );
	newTarget.name = stringPointer;

	for ( i = 0; i < stringLength; i++ )
	{
		if ( string[ i ] == ':' && !newTarget.action )
		{
			*stringPointer++ = '\0';
			newTarget.action = stringPointer;
			continue;
		}
		*stringPointer++ = string[ i ];
	}
	newTarget.actionType = G_GetCallActionFor( newTarget.action );
	newTarget.event = eventKey;
	return newTarget;
}

/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void G_ParseField( const char *key, const char *rawString, gentity_t *entity )
{
	field_t *resultingField;
	byte    *entityData;
	vec4_t  tmpFloatData;
	variatingTime_t varTime = {0, 0};

	resultingField = bsearch( key, fields, ARRAY_LEN( fields ), sizeof( field_t ), cmdcmp );

	if ( !resultingField )
	{
		return;
	}

	entityData = ( byte * ) entity;

	switch ( resultingField->type )
	{
		case F_STRING:
			* ( char ** )( entityData + resultingField->offset ) = G_NewString( rawString );
			break;

		case F_CALLTARGET:
			if(entity->callTargetCount >= MAX_ENTITY_CALLTARGETS)
				G_Error("Maximal number of %i calltargets reached. You can solve this by using a Relay.", MAX_ENTITY_CALLTARGETS);

			( ( gentityCallDefinition_t * )( entityData + resultingField->offset ) ) [ entity->callTargetCount++ ] = G_NewCallDefinition( resultingField->replacement ? resultingField->replacement : resultingField->name, rawString );
			break;

		case F_TIME:
			sscanf( rawString, "%f %f", &varTime.time, &varTime.variance );
			* ( variatingTime_t * )( entityData + resultingField->offset ) = varTime;
			break;

		case F_3D_VECTOR:
			sscanf( rawString, "%f %f %f", &tmpFloatData[ 0 ], &tmpFloatData[ 1 ], &tmpFloatData[ 2 ] );

			( ( float * )( entityData + resultingField->offset ) ) [ 0 ] = tmpFloatData[ 0 ];
			( ( float * )( entityData + resultingField->offset ) ) [ 1 ] = tmpFloatData[ 1 ];
			( ( float * )( entityData + resultingField->offset ) ) [ 2 ] = tmpFloatData[ 2 ];
			break;

		case F_4D_VECTOR:
			sscanf( rawString, "%f %f %f %f", &tmpFloatData[ 0 ], &tmpFloatData[ 1 ], &tmpFloatData[ 2 ], &tmpFloatData[ 3 ] );

			( ( float * )( entityData + resultingField->offset ) ) [ 0 ] = tmpFloatData[ 0 ];
			( ( float * )( entityData + resultingField->offset ) ) [ 1 ] = tmpFloatData[ 1 ];
			( ( float * )( entityData + resultingField->offset ) ) [ 2 ] = tmpFloatData[ 2 ];
			( ( float * )( entityData + resultingField->offset ) ) [ 3 ] = tmpFloatData[ 3 ];
			break;

		case F_INT:
			* ( int * )( entityData + resultingField->offset ) = atoi( rawString );
			break;

		case F_FLOAT:
			* ( float * )( entityData + resultingField->offset ) = atof( rawString );
			break;

		case F_YAW:
			( ( float * )( entityData + resultingField->offset ) ) [ 0 ] = 0;
			( ( float * )( entityData + resultingField->offset ) ) [ 1 ] = atof( rawString );
			( ( float * )( entityData + resultingField->offset ) ) [ 2 ] = 0;
			break;

		default:
			G_Printf( "^3ERROR: unknown datatype %i for field %s\n", resultingField->type, resultingField->name );
			break;
	}

	if ( resultingField->replacement && resultingField->versionState )
		G_WarnAboutDeprecatedEntityField(entity, resultingField->replacement, key, resultingField->versionState );
}

/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
void G_SpawnGEntityFromSpawnVars( void )
{
	int       i, j;
	gentity_t *spawningEntity;

	// get the next free entity
	spawningEntity = G_Spawn();

	for ( i = 0; i < level.numSpawnVars; i++ )
	{
		G_ParseField( level.spawnVars[ i ][ 0 ], level.spawnVars[ i ][ 1 ], spawningEntity );
	}

	G_SpawnInt( "notq3a", "0", &i );

	if ( i )
	{
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
		if (spawningEntity->names[i])
			spawningEntity->names[j++] = spawningEntity->names[i];
	}
	spawningEntity->names[ j ] = NULL;

	// if we didn't get necessary fields (like the classname), don't bother spawning anything
	if ( !G_CallSpawn( spawningEntity ) )
	{
		G_FreeEntity( spawningEntity );
	}
}

void G_ReorderCallTargets( gentity_t *ent )
{
	int i, j;
	// don't leave any "gaps" between multiple targets
	j = 0;
	for (i = 0; i < MAX_ENTITY_CALLTARGETS; ++i)
	{
		if (ent->calltargets[i].name) {
			ent->calltargets[j] = ent->calltargets[i];
			ent->calltargets[j].actionType = G_GetCallActionFor(ent->calltargets[i].action);
			j++;
		}
	}
	ent->calltargets[ j ].name = NULL;
	ent->calltargets[ j ].action = NULL;
	ent->calltargets[ j ].actionType = ECA_DEFAULT;
	ent->callTargetCount = j;
}

qboolean G_WarnAboutDeprecatedEntityField( gentity_t *entity, const char *expectedFieldname, const char *actualFieldname, const int typeOfDeprecation  )
{
	if ( !Q_stricmp(expectedFieldname, actualFieldname) || typeOfDeprecation == ENT_V_CURRENT )
		return qfalse;

	if ( g_debugEntities.integer >= 0 ) //dont't warn about anything with -1 or lower
	{
		if( typeOfDeprecation < ENT_V_TMPORARY
		|| ( g_debugEntities.integer >= 1 && typeOfDeprecation >= ENT_V_TMPORARY) )
		{
			G_Printf( "^3WARNING: ^7Entity ^5#%i^7 contains deprecated field ^5%s^7 — use ^5%s^7 instead\n", entity->s.number, actualFieldname, expectedFieldname );
		}
	}

	return qtrue;
}

/*
====================
G_AddSpawnVarToken
====================
*/
char *G_AddSpawnVarToken( const char *string )
{
	int  l;
	char *dest;

	l = strlen( string );

	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS )
	{
		G_Error( "G_AddSpawnVarToken: MAX_SPAWN_VARS_CHARS" );
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
qboolean G_ParseSpawnVars( void )
{
	char keyname[ MAX_TOKEN_CHARS ];
	char com_token[ MAX_TOKEN_CHARS ];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) )
	{
		// end of spawn string
		return qfalse;
	}

	if ( com_token[ 0 ] != '{' )
	{
		G_Error( "G_ParseSpawnVars: found %s when expecting {", com_token );
	}

	// go through all the key / value pairs
	while ( 1 )
	{
		// parse key
		if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) )
		{
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[ 0 ] == '}' )
		{
			break;
		}

		// parse value
		if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) )
		{
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[ 0 ] == '}' )
		{
			G_Error( "G_ParseSpawnVars: closing brace without data" );
		}

		if ( level.numSpawnVars == MAX_SPAWN_VARS )
		{
			G_Error( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}

		level.spawnVars[ level.numSpawnVars ][ 0 ] = G_AddSpawnVarToken( keyname );
		level.spawnVars[ level.numSpawnVars ][ 1 ] = G_AddSpawnVarToken( com_token );
		level.numSpawnVars++;
	}

	return qtrue;
}

/**
 * Warning: The following comment contains information, that might be parsed and used by radiant based mapeditors.
 */
/*QUAKED worldspawn (0 0 0) ?
Used for game-world global options.
Every map should have exactly one.

=== KEYS ===
; message: Text to print during connection process. Used for the name of level.
; music: path/name of looping .wav file used for level's music (eg. music/sonic5.wav).
; gravity: level gravity [g_gravity (800)]

; humanBuildPoints: maximum amount of power the humans can use. [g_humanBuildPoints]
; humanRepeaterBuildPoints: maximum amount of power the humans can use around each repeater. [g_humanRepeaterBuildPoints]
; alienBuildPoints: maximum amount of sentience available to the overmind. [g_alienBuildPoints]

; humanMaxStage: The highest stage the humans are allowed to use (0/1/2). [g_alienMaxStage (2)]
; alienMaxStage: The highest stage the aliens are allowed to use (0/1/2). [g_humanMaxStage (2)]

; disabledEquipment: A comma delimited list of human weapons or upgrades to disable for this map. [g_disabledEquipment ()]
; disabledClasses: A comma delimited list of alien classes to disable for this map. [g_disabledClasses ()]
; disabledBuildables: A comma delimited list of buildables to disable for this map. [g_disabledBuildables ()]
*/
void SP_worldspawn( void )
{
	char *s;

	G_SpawnString( "classname", "", &s );

	if ( Q_stricmp( s, "worldspawn" ) )
	{
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	// make some data visible to connecting client
	trap_SetConfigstring( CS_GAME_VERSION, GAME_VERSION );

	trap_SetConfigstring( CS_LEVEL_START_TIME, va( "%i", level.startTime ) );

	G_SpawnString( "music", "", &s );
	trap_SetConfigstring( CS_MUSIC, s );

	G_SpawnString( "message", "", &s );
	trap_SetConfigstring( CS_MESSAGE, s );  // map specific message

	trap_SetConfigstring( CS_MOTD, g_motd.string );  // message of the day

	if ( G_SpawnString( "gravity", "", &s ) )
	{
		trap_Cvar_Set( "g_gravity", s );
	}

	if ( G_SpawnString( "humanMaxStage", "", &s ) )
	{
		trap_Cvar_Set( "g_humanMaxStage", s );
	}

	if ( G_SpawnString( "alienMaxStage", "", &s ) )
	{
		trap_Cvar_Set( "g_alienMaxStage", s );
	}

	if ( G_SpawnString( "humanRepeaterBuildPoints", "", &s ) )
		trap_Cvar_Set( "g_humanRepeaterBuildPoints", s );

	if ( G_SpawnString( "humanBuildPoints", "", &s ) )
		trap_Cvar_Set( "g_humanBuildPoints", s );

	if ( G_SpawnString( "alienBuildPoints", "", &s ) )
		trap_Cvar_Set( "g_alienBuildPoints", s );

	G_SpawnString( "disabledEquipment", "", &s );
	trap_Cvar_Set( "g_disabledEquipment", s );

	G_SpawnString( "disabledClasses", "", &s );
	trap_Cvar_Set( "g_disabledClasses", s );

	G_SpawnString( "disabledBuildables", "", &s );
	trap_Cvar_Set( "g_disabledBuildables", s );

	g_entities[ ENTITYNUM_WORLD ].s.number = ENTITYNUM_WORLD;
	g_entities[ ENTITYNUM_WORLD ].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ ENTITYNUM_WORLD ].classname = "worldspawn";

	g_entities[ ENTITYNUM_NONE ].s.number = ENTITYNUM_NONE;
	g_entities[ ENTITYNUM_NONE ].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ ENTITYNUM_NONE ].classname = "nothing";

	// see if we want a warmup time
	trap_SetConfigstring( CS_WARMUP, "-1" );

	if ( g_doWarmup.integer )
	{
		level.warmupTime = level.time - level.startTime + ( g_warmup.integer * 1000 );
		trap_SetConfigstring( CS_WARMUP, va( "%i", level.warmupTime ) );
		G_LogPrintf( "Warmup: %i\n", g_warmup.integer );
	}

	level.timelimit = g_timelimit.integer;
}

/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString( void )
{
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars() )
	{
		G_Error( "SpawnEntities: no entities" );
	}

	SP_position_init();
	SP_worldspawn();

	// parse ents
	while ( G_ParseSpawnVars() )
	{
		G_SpawnGEntityFromSpawnVars();
	}
}
