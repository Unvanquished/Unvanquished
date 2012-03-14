/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

/*
 * name:    cg_spawn.c
 *
 * desc:    Client sided only map entities
*/

#include "cg_local.h"

qboolean CG_SpawnString( const char *key, const char *defaultString, char **out )
{
	int i;

	if( !cg.spawning )
	{
		*out = ( char * ) defaultString;
		CG_Error( "CG_SpawnString() called while not spawning" );
	}

	for( i = 0; i < cg.numSpawnVars; i++ )
	{
		if( !strcmp( key, cg.spawnVars[ i ][ 0 ] ) )
		{
			*out = cg.spawnVars[ i ][ 1 ];
			return qtrue;
		}
	}

	*out = ( char * ) defaultString;
	return qfalse;
}

qboolean CG_SpawnFloat( const char *key, const char *defaultString, float *out )
{
	char     *s;
	qboolean present;

	present = CG_SpawnString( key, defaultString, &s );
	*out = atof( s );
	return present;
}

qboolean CG_SpawnInt( const char *key, const char *defaultString, int *out )
{
	char     *s;
	qboolean present;

	present = CG_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

qboolean CG_SpawnVector( const char *key, const char *defaultString, float *out )
{
	char     *s;
	qboolean present;

	present = CG_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[ 0 ], &out[ 1 ], &out[ 2 ] );
	return present;
}

qboolean CG_SpawnVector2D( const char *key, const char *defaultString, float *out )
{
	char     *s;
	qboolean present;

	present = CG_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f", &out[ 0 ], &out[ 1 ] );
	return present;
}

/*
=============
VectorToString

This is just a convenience function
for printing vectors
=============
*/
char           *vtos( const vec3_t v )
{
	static int  index;
	static char str[ 8 ][ 32 ];
	char        *s;

	// use an array so that multiple vtos won't collide
	s = str[ index ];
	index = ( index + 1 ) & 7;

	Com_sprintf( s, 32, "(%i %i %i)", ( int ) v[ 0 ], ( int ) v[ 1 ], ( int ) v[ 2 ] );

	return s;
}

void SP_path_corner_2( void )
{
	char   *targetname;
	vec3_t origin;

	CG_SpawnString( "targetname", "", &targetname );
	CG_SpawnVector( "origin", "0 0 0", origin );

	if( !*targetname )
	{
		// XreaL BEGIN
		if( !CG_SpawnString( "name", "", &targetname ) )
			// XreaL END
		{
			CG_Error( "path_corner_2 with no targetname at %s\n", vtos( origin ) );
			return;
		}
	}

	if( numPathCorners >= MAX_PATH_CORNERS )
	{
		CG_Error( "Maximum path_corners hit\n" );
		return;
	}

	BG_AddPathCorner( targetname, origin );
}

void SP_info_train_spline_main( void )
{
	char         *targetname;
	char         *target;
	char         *control;
	vec3_t       origin;
	int          i;
	char         *end;
	splinePath_t *spline;

	if( !CG_SpawnVector( "origin", "0 0 0", origin ) )
	{
		CG_Error( "info_train_spline_main with no origin\n" );
	}

	if( !CG_SpawnString( "targetname", "", &targetname ) )
	{
		// XreaL BEGIN
		if( !CG_SpawnString( "name", "", &targetname ) )
			// XreaL END
		{
			CG_Error( "info_train_spline_main with no targetname at %s\n", vtos( origin ) );
		}
	}

	CG_SpawnString( "target", "", &target );

	spline = BG_AddSplinePath( targetname, target, origin );

	if( CG_SpawnString( "end", "", &end ) )
	{
		spline->isEnd = qtrue;
	}
	else if( CG_SpawnString( "start", "", &end ) )
	{
		spline->isStart = qtrue;
	}

	for( i = 1;; i++ )
	{
		if( !CG_SpawnString( i == 1 ? va( "control" ) : va( "control%i", i ), "", &control ) )
		{
			break;
		}

		BG_AddSplineControl( spline, control );
	}
}

void SP_misc_gamemodel( void )
{
	char           *model;
	vec_t          angle;
	vec3_t         angles;

	vec_t          scale;
	vec3_t         vScale;

	vec3_t         org;

	cg_gamemodel_t *gamemodel;

	int            i;

	if( CG_SpawnString( "targetname", "", &model ) || CG_SpawnString( "scriptname", "", &model ) ||
	    CG_SpawnString( "spawnflags", "", &model ) )
	{
		// Gordon: this model may not be static, so let the server handle it
		return;
	}

	if( cg.numMiscGameModels >= MAX_STATIC_GAMEMODELS )
	{
		CG_Error( "^1MAX_STATIC_GAMEMODELS(%i) hit", MAX_STATIC_GAMEMODELS );
	}

	CG_SpawnString( "model", "", &model );

	CG_SpawnVector( "origin", "0 0 0", org );

	if( !CG_SpawnVector( "angles", "0 0 0", angles ) )
	{
		if( CG_SpawnFloat( "angle", "0", &angle ) )
		{
			angles[ YAW ] = angle;
		}
	}

	if( !CG_SpawnVector( "modelscale_vec", "1 1 1", vScale ) )
	{
		if( CG_SpawnFloat( "modelscale", "1", &scale ) )
		{
			VectorSet( vScale, scale, scale, scale );
		}
	}

	gamemodel = &cgs.miscGameModels[ cg.numMiscGameModels++ ];
	gamemodel->model = trap_R_RegisterModel( model );
	AnglesToAxis( angles, gamemodel->axes );

	for( i = 0; i < 3; i++ )
	{
		VectorScale( gamemodel->axes[ i ], vScale[ i ], gamemodel->axes[ i ] );
	}

	VectorCopy( org, gamemodel->org );

	if( gamemodel->model )
	{
		vec3_t mins, maxs;

		trap_R_ModelBounds( gamemodel->model, mins, maxs );

		for( i = 0; i < 3; i++ )
		{
			mins[ i ] *= vScale[ i ];
			maxs[ i ] *= vScale[ i ];
		}

		gamemodel->radius = RadiusFromBounds( mins, maxs );
	}
	else
	{
		gamemodel->radius = 0;
	}
}

void SP_trigger_objective_info( void )
{
	char *temp;

	CG_SpawnString( "infoAllied", "^1No Text Supplied", &temp );
	Q_strncpyz( cg.oidTriggerInfoAllies[ cg.numOIDtriggers2 ], temp, 256 );

	CG_SpawnString( "infoAxis", "^1No Text Supplied", &temp );
	Q_strncpyz( cg.oidTriggerInfoAxis[ cg.numOIDtriggers2 ], temp, 256 );

	cg.numOIDtriggers2++;
}

typedef struct
{
	char *name;
	void ( *spawn )( void );
} spawn_t;

spawn_t spawns[] =
{
	{ 0,                           0                                              },
	{ "path_corner_2",             SP_path_corner_2                               },
	{ "info_train_spline_main",    SP_info_train_spline_main                      },
	{ "info_train_spline_control", SP_path_corner_2                               },

	{ "trigger_objective_info",    SP_trigger_objective_info                      },
	{ "misc_gamemodel",            SP_misc_gamemodel                              },
};

#define NUMSPAWNS ( sizeof( spawns ) / sizeof( spawn_t ) )

/*
===================
CG_ParseEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
cg.spawnVars[], then call the class specfic spawn function
===================
*/
void CG_ParseEntityFromSpawnVars( void )
{
	int  i;
	char *classname;

	// check for "notteam" / "notfree" flags
	CG_SpawnInt( "notteam", "0", &i );

	if( i )
	{
		return;
	}

	if( CG_SpawnString( "classname", "", &classname ) )
	{
		for( i = 0; i < NUMSPAWNS; i++ )
		{
			if( !Q_stricmp( spawns[ i ].name, classname ) )
			{
				spawns[ i ].spawn();
				break;
			}
		}
	}
}

/*
====================
CG_AddSpawnVarToken
====================
*/
char           *CG_AddSpawnVarToken( const char *string )
{
	int  l;
	char *dest;

	l = strlen( string );

	if( cg.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS )
	{
		CG_Error( "CG_AddSpawnVarToken: MAX_SPAWN_VARS" );
	}

	dest = cg.spawnVarChars + cg.numSpawnVarChars;
	memcpy( dest, string, l + 1 );

	cg.numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
CG_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into cg.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean CG_ParseSpawnVars( void )
{
	char keyname[ MAX_TOKEN_CHARS ];
	char com_token[ MAX_TOKEN_CHARS ];

	cg.numSpawnVars = 0;
	cg.numSpawnVarChars = 0;

	// parse the opening brace
	if( !trap_GetEntityToken( com_token, sizeof( com_token ) ) )
	{
		// end of spawn string
		return qfalse;
	}

	if( com_token[ 0 ] != '{' )
	{
		CG_Error( "CG_ParseSpawnVars: found %s when expecting {", com_token );
	}

	// go through all the key / value pairs
	while( 1 )
	{
		// parse key
		if( !trap_GetEntityToken( keyname, sizeof( keyname ) ) )
		{
			CG_Error( "CG_ParseSpawnVars: EOF without closing brace" );
		}

		if( keyname[ 0 ] == '}' )
		{
			break;
		}

		// parse value
		if( !trap_GetEntityToken( com_token, sizeof( com_token ) ) )
		{
			CG_Error( "CG_ParseSpawnVars: EOF without closing brace" );
		}

		if( com_token[ 0 ] == '}' )
		{
			CG_Error( "CG_ParseSpawnVars: closing brace without data" );
		}

		if( cg.numSpawnVars == MAX_SPAWN_VARS )
		{
			CG_Error( "CG_ParseSpawnVars: MAX_SPAWN_VARS" );
		}

		cg.spawnVars[ cg.numSpawnVars ][ 0 ] = CG_AddSpawnVarToken( keyname );
		cg.spawnVars[ cg.numSpawnVars ][ 1 ] = CG_AddSpawnVarToken( com_token );
		cg.numSpawnVars++;
	}

	return qtrue;
}

void SP_worldspawn( void )
{
	char *s;
	int  i;

	CG_SpawnString( "classname", "", &s );

	if( Q_stricmp( s, "worldspawn" ) )
	{
		CG_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	cgs.ccLayers = 0;

	if( CG_SpawnVector2D( "mapcoordsmins", "-128 128", cg.mapcoordsMins ) &&   // top left
	    CG_SpawnVector2D( "mapcoordsmaxs", "128 -128", cg.mapcoordsMaxs ) )
	{
		// bottom right
		cg.mapcoordsValid = qtrue;
	}
	else
	{
		cg.mapcoordsValid = qfalse;
	}

	CG_ParseSpawns();

	CG_SpawnString( "cclayers", "0", &s );
	cgs.ccLayers = atoi( s );

	// CHRUKER: b043 - Make sure the maximum commandmaps, doesn't overflow the maximum allowed command map layers
	if( cgs.ccLayers > MAX_COMMANDMAP_LAYERS )
	{
		cgs.ccLayers = MAX_COMMANDMAP_LAYERS;
		CG_Printf( "^3Warning: The maximum number (%i) of command map layers is exceeded.\n", MAX_COMMANDMAP_LAYERS );
	}

	for( i = 0; i < cgs.ccLayers; i++ )
	{
		CG_SpawnString( va( "cclayerceil%i", i ), "0", &s );
		cgs.ccLayerCeils[ i ] = atoi( s );
	}

	cg.mapcoordsScale[ 0 ] = 1 / ( cg.mapcoordsMaxs[ 0 ] - cg.mapcoordsMins[ 0 ] );
	cg.mapcoordsScale[ 1 ] = 1 / ( cg.mapcoordsMaxs[ 1 ] - cg.mapcoordsMins[ 1 ] );

	BG_InitLocations( cg.mapcoordsMins, cg.mapcoordsMaxs );

	CG_SpawnString( "atmosphere", "", &s );
	CG_EffectParse( s );

	cg.fiveMinuteSound_g[ 0 ] =
	  cg.fiveMinuteSound_a[ 0 ] =
	    cg.twoMinuteSound_g[ 0 ] = cg.twoMinuteSound_a[ 0 ] = cg.thirtySecondSound_g[ 0 ] = cg.thirtySecondSound_a[ 0 ] = '\0';

	// CHRUKER: b092 - Custom 5 minute warning is the same as 2 minute warning
	CG_SpawnString( "fiveMinuteSound_axis", "axis_hq_5minutes", &s );
	Q_strncpyz( cg.fiveMinuteSound_g, s, sizeof( cg.fiveMinuteSound_g ) );
	CG_SpawnString( "fiveMinuteSound_allied", "allies_hq_5minutes", &s );
	Q_strncpyz( cg.fiveMinuteSound_a, s, sizeof( cg.fiveMinuteSound_a ) );
	// b092

	CG_SpawnString( "twoMinuteSound_axis", "axis_hq_2minutes", &s );
	Q_strncpyz( cg.twoMinuteSound_g, s, sizeof( cg.twoMinuteSound_g ) );
	CG_SpawnString( "twoMinuteSound_allied", "allies_hq_2minutes", &s );
	Q_strncpyz( cg.twoMinuteSound_a, s, sizeof( cg.twoMinuteSound_a ) );

	CG_SpawnString( "thirtySecondSound_axis", "axis_hq_30seconds", &s );
	Q_strncpyz( cg.thirtySecondSound_g, s, sizeof( cg.thirtySecondSound_g ) );
	CG_SpawnString( "thirtySecondSound_allied", "allies_hq_30seconds", &s );
	Q_strncpyz( cg.thirtySecondSound_a, s, sizeof( cg.thirtySecondSound_a ) );

	// 5 minute axis
	if( !*cg.fiveMinuteSound_g )
	{
		cgs.media.fiveMinuteSound_g = 0;
	}
	else if( strstr( cg.fiveMinuteSound_g, ".wav" ) )
	{
		cgs.media.fiveMinuteSound_g = trap_S_RegisterSound( cg.fiveMinuteSound_g, qtrue );
	}
	else
	{
		cgs.media.fiveMinuteSound_g = -1;
	}

	// 5 minute allied
	if( !*cg.fiveMinuteSound_a )
	{
		cgs.media.fiveMinuteSound_a = 0;
	}
	else if( strstr( cg.fiveMinuteSound_a, ".wav" ) )
	{
		cgs.media.fiveMinuteSound_a = trap_S_RegisterSound( cg.fiveMinuteSound_a, qtrue );
	}
	else
	{
		cgs.media.fiveMinuteSound_a = -1;
	}

	// 2 minute axis
	if( !*cg.twoMinuteSound_g )
	{
		cgs.media.twoMinuteSound_g = 0;
	}
	else if( strstr( cg.twoMinuteSound_g, ".wav" ) )
	{
		cgs.media.twoMinuteSound_g = trap_S_RegisterSound( cg.twoMinuteSound_g, qtrue );
	}
	else
	{
		cgs.media.twoMinuteSound_g = -1;
	}

	// 2 minute allied
	if( !*cg.twoMinuteSound_a )
	{
		cgs.media.twoMinuteSound_a = 0;
	}
	else if( strstr( cg.twoMinuteSound_a, ".wav" ) )
	{
		cgs.media.twoMinuteSound_a = trap_S_RegisterSound( cg.twoMinuteSound_a, qtrue );
	}
	else
	{
		cgs.media.twoMinuteSound_a = -1;
	}

	// 30 seconds axis
	if( !*cg.thirtySecondSound_g )
	{
		cgs.media.thirtySecondSound_g = 0;
	}
	else if( strstr( cg.thirtySecondSound_g, ".wav" ) )
	{
		cgs.media.thirtySecondSound_g = trap_S_RegisterSound( cg.thirtySecondSound_g, qtrue );
	}
	else
	{
		cgs.media.thirtySecondSound_g = -1;
	}

	// 30 seconds allied
	if( !*cg.thirtySecondSound_a )
	{
		cgs.media.thirtySecondSound_a = 0;
	}
	else if( strstr( cg.thirtySecondSound_a, ".wav" ) )
	{
		cgs.media.thirtySecondSound_a = trap_S_RegisterSound( cg.thirtySecondSound_a, qtrue );
	}
	else
	{
		cgs.media.thirtySecondSound_a = -1;
	}
}

/*
==============
CG_ParseEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void CG_ParseEntitiesFromString( void )
{
	// allow calls to CG_Spawn*()
	cg.spawning = qtrue;
	cg.numSpawnVars = 0;
	cg.numMiscGameModels = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if( !CG_ParseSpawnVars() )
	{
		CG_Error( "ParseEntities: no entities" );
	}

	SP_worldspawn();

	// parse ents
	while( CG_ParseSpawnVars() )
	{
		CG_ParseEntityFromSpawnVars();
	}

	cg.spawning = qfalse; // any future calls to CG_Spawn*() will be errors
}
