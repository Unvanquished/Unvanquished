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
  F_VECTOR,
  F_VECTOR4,
  F_ANGLEHACK
} fieldtype_t;

typedef struct
{
	const char  *name;
	size_t      ofs;
	fieldtype_t type;
} field_t;

static const field_t fields[] =
{
	{ "acceleration",        FOFS( acceleration ),        F_VECTOR    },
	{ "alpha",               FOFS( pos1 ),                F_VECTOR    },
	{ "angle",               FOFS( s.angles ),            F_ANGLEHACK },
	{ "angles",              FOFS( s.angles ),            F_VECTOR    },
	{ "animation",           FOFS( animation ),           F_VECTOR4   },
	{ "bounce",              FOFS( physicsBounce ),       F_FLOAT     },
	{ "classname",           FOFS( classname ),           F_STRING    },
	{ "count",               FOFS( count ),               F_INT       },
	{ "dmg",                 FOFS( damage ),              F_INT       },
	{ "health",              FOFS( health ),              F_INT       },
	{ "message",             FOFS( message ),             F_STRING    },
	{ "model",               FOFS( model ),               F_STRING    },
	{ "model2",              FOFS( model2 ),              F_STRING    },
	{ "origin",              FOFS( s.origin ),            F_VECTOR    },
	{ "radius",              FOFS( pos2 ),                F_VECTOR    },
	{ "random",              FOFS( random ),              F_FLOAT     },
	{ "rotatorAngle",        FOFS( rotatorAngle ),        F_FLOAT     },
	{ "spawnflags",          FOFS( spawnflags ),          F_INT       },
	{ "speed",               FOFS( speed ),               F_FLOAT     },
	{ "target",				 FOFS( targets[ 0 ] ),		  F_STRING	  },
	{ "target2", 			 FOFS( targets[ 1 ] ),		  F_STRING	  },
	{ "target3",			 FOFS( targets[ 2 ] ),		  F_STRING	  },
	{ "target4",			 FOFS( targets[ 3 ] ),		  F_STRING	  },
	{ "targetname",			 FOFS( targetnames[ 0 ] ),	  F_STRING	  },
	{ "targetname2",		 FOFS( targetnames[ 1 ] ),	  F_STRING	  },
	{ "targetname3",		 FOFS( targetnames[ 2 ] ),	  F_STRING	  },
	{ "targetname4",		 FOFS( targetnames[ 3 ] ),	  F_STRING	  },
	{ "targetShaderName",    FOFS( targetShaderName ),    F_STRING    },
	{ "targetShaderNewName", FOFS( targetShaderNewName ), F_STRING    },
	{ "wait",                FOFS( wait ),                F_FLOAT     }
};

typedef struct
{
	const char *name;
	void      ( *spawn )( gentity_t *ent );
} spawn_t;

static const spawn_t spawns[] =
{
	{ "func_bobbing",             SP_func_bobbing             },
	{ "func_button",              SP_func_button              },
	{ "func_door",                SP_func_door                },
	{ "func_door_model",          SP_func_door_model          },
	{ "func_door_rotating",       SP_func_door_rotating       },
	{ "func_dynamic",             SP_func_dynamic             },
	{ "func_group",               SP_NULL                     },
	{ "func_pendulum",            SP_func_pendulum            },
	{ "func_plat",                SP_func_plat                },
	{ "func_rotating",            SP_func_rotating            },
	{ "func_static",              SP_func_static              },
	{ "func_timer",               SP_trigger_timer            }, //@Deprecated use trigger_timer instead
	{ "func_train",               SP_func_train               },

	/**
	 *
	 *	Information entities
	 *	====================
	 *	info entities don't do anything at all, but provide positional
	 *	information for things controlled by other processes
	 *
	 */
	{ "info_alien_intermission",  SP_info_alien_intermission  },
	{ "info_human_intermission",  SP_info_human_intermission  },
	{ "info_notnull",             SP_target_position          }, //@Deprecated use target_position instead
	{ "info_null",                SP_NULL                     }, //@Deprecated don't use it
	{ "info_player_deathmatch",   SP_info_player_deathmatch   },
	{ "info_player_intermission", SP_info_player_intermission },
	{ "info_player_start",        SP_info_player_start        },
	{ "light",                    SP_NULL                     },
	{ "misc_anim_model",          SP_misc_anim_model          },
	{ "misc_light_flare",         SP_misc_light_flare         },
	{ "misc_model",               SP_misc_model               },
	{ "misc_particle_system",     SP_misc_particle_system     },
	{ "misc_portal_camera",       SP_misc_portal_camera       },
	{ "misc_portal_surface",      SP_misc_portal_surface      },
	{ "misc_teleporter_dest",     SP_misc_teleporter_dest     },
	{ "path_corner",              SP_path_corner              },

	/**
	 *
	 * 	Target Entities
	 * 	===============
	 * 	Targets perform no action by themselves.
	 *	Instead they are targeted by other entities,
	 *	like being triggered by a trigger_ entity.
	 *
	 */
	{ "target_alien_win",         SP_target_alien_win         },
	{ "target_delay",             SP_target_relay             }, //@Deprecated use target_relay instead
	{ "target_human_win",         SP_target_human_win         },
	{ "target_hurt",              SP_target_hurt              },
	{ "target_kill",              SP_target_kill              },
	{ "target_location",          SP_target_location          },
	{ "target_position",          SP_target_position          },
	{ "target_print",             SP_target_print             },
	{ "target_push",              SP_target_push              },
	{ "target_relay",             SP_target_relay             },
	{ "target_rumble",            SP_target_rumble            },
	{ "target_score",             SP_target_score             },
	{ "target_speaker",           SP_target_speaker           },
	{ "target_teleporter",        SP_target_teleporter        },

	/**
	 *
	 *	Trigger
	 *	=======
	 *	Triggers are either special conditions or brush objects that
	 *	cause an effect when contacted by a living player,
	 *	usually involving firing targets.
	 *
	 *	While almost everything could be done with
	 *	a single trigger class and different targets, triggered effects
	 *	could not be client side predicted (push and teleport).
	 *
	 */
	{ "trigger_always",           SP_trigger_always           },
	{ "trigger_ammo",             SP_trigger_ammo             },
	{ "trigger_buildable",        SP_trigger_buildable        },
	{ "trigger_class",            SP_trigger_class            },
	{ "trigger_equipment",        SP_trigger_equipment        },
	{ "trigger_gravity",          SP_trigger_gravity          },
	{ "trigger_heal",             SP_trigger_heal             },
	{ "trigger_hurt",             SP_trigger_hurt             },
	{ "trigger_multiple",         SP_trigger_multiple         },
	{ "trigger_push",             SP_trigger_push             },
	{ "trigger_stage",            SP_trigger_stage            },
	{ "trigger_teleport",         SP_trigger_teleport         },
	{ "trigger_timer",            SP_trigger_timer            },
	{ "trigger_win",              SP_trigger_win              }
};

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean G_CallSpawn( gentity_t *ent )
{
	spawn_t     *s;
	buildable_t buildable;

	if ( !ent->classname )
	{
		G_Printf( "G_CallSpawn: NULL classname\n" );
		return qfalse;
	}

	//check buildable spawn functions
	buildable = BG_BuildableByEntityName( ent->classname )->number;

	if ( buildable != BA_NONE )
	{
		// don't spawn built-in buildings if we are using a custom layout
		if ( level.layout[ 0 ] && Q_stricmp( level.layout, "*BUILTIN*" ) )
		{
			return qfalse;
		}

		if ( buildable == BA_A_SPAWN || buildable == BA_H_SPAWN )
		{
			ent->s.angles[ YAW ] += 180.0f;
			AngleNormalize360( ent->s.angles[ YAW ] );
		}

		G_SpawnBuildable( ent, buildable );
		return qtrue;
	}

	// check the spawn functions for other classes
	s = bsearch( ent->classname, spawns, ARRAY_LEN( spawns ),
	             sizeof( spawn_t ), cmdcmp );

	if ( s )
	{
		// found it
		s->spawn( ent );
		return qtrue;
	}

	G_Printf( "%s doesn't have a spawn function\n", ent->classname );
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
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void G_ParseField( const char *key, const char *value, gentity_t *ent )
{
	field_t *f;
	byte    *b;
	float   v;
	vec3_t  vec;
	vec4_t  vec4;

	f = bsearch( key, fields, ARRAY_LEN( fields ),
	             sizeof( field_t ), cmdcmp );

	if ( !f )
	{
		return;
	}

	b = ( byte * ) ent;

	switch ( f->type )
	{
		case F_STRING:
			* ( char ** )( b + f->ofs ) = G_NewString( value );
			break;

		case F_VECTOR:
			sscanf( value, "%f %f %f", &vec[ 0 ], &vec[ 1 ], &vec[ 2 ] );

			( ( float * )( b + f->ofs ) ) [ 0 ] = vec[ 0 ];
			( ( float * )( b + f->ofs ) ) [ 1 ] = vec[ 1 ];
			( ( float * )( b + f->ofs ) ) [ 2 ] = vec[ 2 ];
			break;

		case F_VECTOR4:
			sscanf( value, "%f %f %f %f", &vec4[ 0 ], &vec4[ 1 ], &vec4[ 2 ], &vec4[ 3 ] );

			( ( float * )( b + f->ofs ) ) [ 0 ] = vec4[ 0 ];
			( ( float * )( b + f->ofs ) ) [ 1 ] = vec4[ 1 ];
			( ( float * )( b + f->ofs ) ) [ 2 ] = vec4[ 2 ];
			( ( float * )( b + f->ofs ) ) [ 3 ] = vec4[ 3 ];
			break;

		case F_INT:
			* ( int * )( b + f->ofs ) = atoi( value );
			break;

		case F_FLOAT:
			* ( float * )( b + f->ofs ) = atof( value );
			break;

		case F_ANGLEHACK:
			v = atof( value );
			( ( float * )( b + f->ofs ) ) [ 0 ] = 0;
			( ( float * )( b + f->ofs ) ) [ 1 ] = v;
			( ( float * )( b + f->ofs ) ) [ 2 ] = 0;
			break;
	}
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
	gentity_t *ent;

	// get the next free entity
	ent = G_Spawn();

	for ( i = 0; i < level.numSpawnVars; i++ )
	{
		G_ParseField( level.spawnVars[ i ][ 0 ], level.spawnVars[ i ][ 1 ], ent );
	}

	G_SpawnInt( "notq3a", "0", &i );

	if ( i )
	{
		G_FreeEntity( ent );
		return;
	}

	// move editor origin to pos
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	// don't leave any "gaps" between multiple targets
	j = 0;
	for (i = 0; i < MAX_TARGETS; ++i)
	{
		if (ent->targets[i])
			ent->targets[j++] = ent->targets[i];
	}
	ent->targets[ j ] = NULL;

	// don't leave any "gaps" between multiple targetnames
	j = 0;
	for (i = 0; i < MAX_TARGETNAMES; ++i)
	{
		if (ent->targetnames[i])
			ent->targetnames[j++] = ent->targetnames[i];
	}
	ent->targetnames[ j ] = NULL;

	// if we didn't get a classname, don't bother spawning anything
	if ( !G_CallSpawn( ent ) )
	{
		G_FreeEntity( ent );
	}
}

void G_Entitiy_Deprecation_Alias( gentity_t *entity, const char *replacement )
{
	G_Printf( "^3WARNING: ^7reference by deprecated classname ^5%s^7 found - use ^5%s^7 instead\n", entity->classname, replacement );
	entity->classname = replacement;
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
Every map should have exactly one worldspawn, which is used for global options.

message: Text to print during connection process. Used for the name of level.
music: path/name of looping .wav file used for level's music (eg. music/sonic5.wav).
gravity: level gravity [g_gravity (800)]

humanBuildPoints: maximum amount of power the humans can use. [g_humanBuildPoints]
humanRepeaterBuildPoints: maximum amount of power the humans can use around each repeater. [g_humanRepeaterBuildPoints]
alienBuildPoints: maximum amount of sentience available to the overmind. [g_alienBuildPoints]

humanMaxStage: The highest stage the humans are allowed to use (0/1/2). [g_alienMaxStage (2)]
alienMaxStage: The highest stage the aliens are allowed to use (0/1/2). [g_humanMaxStage (2)]

disabledEquipment: A comma delimited list of human weapons or upgrades to disable for this map. [g_disabledEquipment ()]
disabledClasses: A comma delimited list of alien classes to disable for this map. [g_disabledClasses ()]
disabledBuildables: A comma delimited list of buildables to disable for this map. [g_disabledBuildables ()]
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

	SP_target_init();
	SP_worldspawn();

	// parse ents
	while ( G_ParseSpawnVars() )
	{
		G_SpawnGEntityFromSpawnVars();
	}
}
