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

// bg_parser.c -- parsers for configuration files

#include "../../engine/qcommon/q_shared.h"
#include "bg_public.h"

int                                trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void                               trap_FS_Read( void *buffer, int len, fileHandle_t f );
void                               trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void                               trap_FS_FCloseFile( fileHandle_t f );

#define PARSE(text, token) \
    (token) = COM_Parse( &(text) ); \
    if ( !*(token) ) \
    { \
        break; \
    }

/*
======================
BG_ReadWholeFile

Helper function that tries to read a whole file in a buffer. Should it be in bg_parse.c?
======================
*/

qboolean BG_ReadWholeFile( const char *filename, char *buffer, int size)
{
    fileHandle_t f;
    int len;

    len = trap_FS_FOpenFile( filename, &f, FS_READ );

    if ( len < 0 )
    {
        Com_Printf( S_COLOR_RED "ERROR: file %s doesn't exist\n", filename );
        return qfalse;
    }

    if ( len == 0 || len >= size - 1 )
    {
        trap_FS_FCloseFile( f );
        Com_Printf( S_COLOR_RED "ERROR: file %s is %s\n", filename,
                    len == 0 ? "empty" : "too long" );
        return qfalse;
    }

    trap_FS_Read( buffer, len, f );
    buffer[ len ] = 0;
    trap_FS_FCloseFile( f );

    return qtrue;
}

static int BG_StageFromNumber( int i )
{
    if( i == 1 )
    {
        return S1 | S2 | S3;
    }
    if( i == 2 )
    {
        return S2 | S3;
    }
    if( i == 3 )
    {
        return S3;
    }

    return 0;
}

static int BG_ParseTeam(char* token){
    if ( !Q_stricmp( token, "aliens" ) )
    {
        return TEAM_ALIENS;
    }
    else if ( !Q_stricmp( token, "humans" ) )
    {
        return TEAM_HUMANS;
    }
    else
    {
        Com_Printf( S_COLOR_RED "ERROR: unknown team value '%s'\n", token );
        return -1;
    }
}

/*
======================
BG_ParseBuildableAttributeFile

Parses a configuration file describing the attributes of a buildable
======================
*/

void BG_ParseBuildableAttributeFile( const char *filename, buildableAttributes_t *ba )
{
    char *token;
    char text_buffer[ 20000 ];
    char* text;
    int defined = 0;
    enum
    {
      HUMANNAME = 1 << 1,
      DESCRIPTION = 1 << 2,
      NORMAL = 1 << 3,
      BUILDPOINTS = 1 << 4,
      STAGE = 1 << 5,
      HEALTH = 1 << 6,
      DEATHMOD = 1 << 7,
      TEAM = 1 << 8,
      BUILDWEAPON = 1 << 9,
      BUILDTIME = 1 << 10,
      VALUE = 1 << 11,
      RADAR = 1 << 12,
    };

    if( !BG_ReadWholeFile( filename, text_buffer, sizeof(text_buffer) ) )
    {
        return;
    }

    text = text_buffer;

    while ( 1 )
    {
        PARSE(text, token);

        if ( !Q_stricmp( token, "humanName" ) )
        {
            PARSE(text, token);

            ba->humanName = strdup( token );

            defined |= HUMANNAME;
        }
        else if ( !Q_stricmp( token, "description" ) )
        {
            PARSE(text, token);

            ba->info = strdup( token );

            defined |= DESCRIPTION;
        }
        else if ( !Q_stricmp( token, "buildPoints" ) )
        {
            PARSE(text, token);

            ba->buildPoints = atoi(token);

            defined |= BUILDPOINTS;
        }
        else if ( !Q_stricmp( token, "stage" ) )
        {
            PARSE(text, token);

            //It depends heavily on the definition of S1 S2 S3
            ba->stages = BG_StageFromNumber( atoi(token) );

            defined |= STAGE;
        }
        else if ( !Q_stricmp( token, "health" ) )
        {
            PARSE(text, token);

            ba->health = atoi(token);

            defined |= HEALTH;
        }
        else if ( !Q_stricmp( token, "regen" ) )
        {
            PARSE(text, token);

            ba->regenRate = atoi(token);
        }
        else if ( !Q_stricmp( token, "splashDamage" ) )
        {
            PARSE(text, token);

            ba->splashDamage = atoi(token);
        }
        else if ( !Q_stricmp( token, "splashRadius" ) )
        {
            PARSE(text, token);

            ba->splashRadius = atoi(token);
        }
        else if ( !Q_stricmp( token, "meansOfDeath" ) )
        {
            PARSE(text, token);

            if ( !Q_stricmp( token, "alienBuildable" ) )
            {
                ba->meansOfDeath = MOD_ASPAWN;
            }
            else if ( !Q_stricmp( token, "humanBuildable" ) )
            {
                ba->meansOfDeath = MOD_HSPAWN;
            }
            else
            {
                Com_Printf( S_COLOR_RED "ERROR: unknown meanOfDeath value '%s'\n", token );
            }

            defined |= DEATHMOD;
        }
        else if ( !Q_stricmp( token, "team" ) )
        {
            PARSE(text, token);

            ba->team = BG_ParseTeam(token);

            defined |= TEAM;
        }
        else if ( !Q_stricmp( token, "buildWeapon" ) )
        {
            PARSE(text, token);

            if ( !Q_stricmp( token, "allAlien" ) )
            {
                ba->buildWeapon = ( 1 << WP_ABUILD ) | ( 1 << WP_ABUILD2 );
            }
            else if ( !Q_stricmp( token, "advAlien" ) )
            {
                ba->buildWeapon = ( 1 << WP_ABUILD2 );
            }
            else if ( !Q_stricmp( token, "human" ) )
            {
                ba->buildWeapon = ( 1 << WP_HBUILD );
            }
            else
            {
                Com_Printf( S_COLOR_RED "ERROR: unknown buildWeapon value '%s'\n", token );
            }

            defined |= BUILDWEAPON;
        }
        else if ( !Q_stricmp( token, "thinkPeriod" ) )
        {
            PARSE(text, token);

            ba->nextthink = atoi(token);
        }
        else if ( !Q_stricmp( token, "buildTime" ) )
        {
            PARSE(text, token);

            ba->buildTime = atoi(token);

            defined |= BUILDTIME;
        }
        else if ( !Q_stricmp( token, "usable" ) )
        {
            ba->usable = qtrue;
        }
        else if ( !Q_stricmp( token, "attackRange" ) )
        {
            PARSE(text, token);

            ba->turretRange = atoi(token);
        }
        else if ( !Q_stricmp( token, "attackSpeed" ) )
        {
            PARSE(text, token);

            ba->turretFireSpeed = atoi(token);
        }
        else if ( !Q_stricmp( token, "attackType" ) )
        {
            PARSE(text, token);

            if ( !Q_stricmp( token, "none" ) )
            {
                ba->turretProjType = WP_NONE;
            }
            else if ( !Q_stricmp( token, "lockBlob" ) )
            {
                ba->turretProjType = WP_LOCKBLOB_LAUNCHER;
            }
            else if ( !Q_stricmp( token, "hive" ) )
            {
                ba->turretProjType = WP_HIVE;
            }
            else if ( !Q_stricmp( token, "mgturret" ) )
            {
                ba->turretProjType = WP_MGTURRET;
            }
            else if ( !Q_stricmp( token, "tesla" ) )
            {
                ba->turretProjType = WP_TESLAGEN;
            }
            else
            {
                Com_Printf( S_COLOR_RED "ERROR: unknown attackType value '%s'\n", token );
            }
        }
        else if ( !Q_stricmp( token, "minNormal" ) )
        {
            PARSE(text, token);

            ba->minNormal = atof(token);
            defined |= NORMAL;
        }

        else if ( !Q_stricmp( token, "allowInvertNormal" ) )
        {
            ba->invertNormal = qtrue;
        }
        else if ( !Q_stricmp( token, "needsCreep" ) )
        {
            ba->creepTest = qtrue;
        }
        else if ( !Q_stricmp( token, "creepSize" ) )
        {
            PARSE(text, token);

            ba->creepSize = atoi(token);
        }
        else if ( !Q_stricmp( token, "dccTest" ) )
        {
            ba->dccTest = qtrue;
        }
        else if ( !Q_stricmp( token, "transparentTest" ) )
        {
            ba->transparentTest = qtrue;
        }
        else if ( !Q_stricmp( token, "unique" ) )
        {
            ba->uniqueTest = qtrue;
        }
        else if ( !Q_stricmp( token, "reward" ) )
        {
            PARSE(text, token);

            ba->value = atoi(token);

            defined |= VALUE;
        }
        else if ( !Q_stricmp( token, "radarFadeOut" ) )
        {
            PARSE(text, token);

            ba->value = atof(token);
            defined |= RADAR;
        }
        else
        {
            Com_Printf( S_COLOR_RED "ERROR: unknown token '%s'\n", token );
        }
    }

    if ( !( defined & HUMANNAME) ) { token = "humanName"; }
    else if ( !( defined & DESCRIPTION) ) { token = "description"; }
    else if ( !( defined & BUILDPOINTS) ) { token = "buildPoints"; }
    else if ( !( defined & STAGE) ) { token = "stage"; }
    else if ( !( defined & HEALTH) ) { token = "health"; }
    else if ( !( defined & DEATHMOD) ) { token = "meansOfDeath"; }
    else if ( !( defined & TEAM) ) { token = "team"; }
    else if ( !( defined & BUILDWEAPON) ) { token = "buildWeapon"; }
    else if ( !( defined & BUILDTIME) ) { token = "buildTime"; }
    else if ( !( defined & VALUE) ) { token = "reward"; }
    else if ( !( defined & RADAR) ) { token = "radarFadeOut"; }
    else if ( !( defined & NORMAL) ) { token = "minNormal"; }

    if ( strlen( token ) > 0 )
    {
        Com_Printf( S_COLOR_RED "ERROR: %s not defined in %s\n",
                    token, filename );
    }
}

/*
======================
BG_ParseBuildableModelFile

Parses a configuration file describing the model of a buildable
======================
*/
void BG_ParseBuildableModelFile( const char *filename, buildableModelConfig_t *bc )
{
    char *token;
    char text_buffer[ 20000 ];
    char* text;
    int defined = 0;
    enum
    {
      MODEL = 1 << 0,
      MODELSCALE = 1 << 1,
      MINS = 1 << 2,
      MAXS = 1 << 3,
      ZOFFSET = 1 << 4,
      OLDSCALE = 1 << 5,
      OLDOFFSET = 1 << 6
    };

    if( !BG_ReadWholeFile( filename, text_buffer, sizeof(text_buffer) ) )
    {
        return;
    }

    text = text_buffer;

    // read optional parameters
    while ( 1 )
    {
        PARSE(text, token);

        if ( !Q_stricmp( token, "model" ) )
        {
            int index = 0;

            PARSE(text, token);

            index = atoi( token );

            if ( index < 0 )
            {
                index = 0;
            }
            else if ( index >= MAX_BUILDABLE_MODELS )
            {
                index = MAX_BUILDABLE_MODELS - 1;
            }

            PARSE(text, token);

            Q_strncpyz( bc->models[ index ], token, sizeof( bc->models[ 0 ] ) );

            defined |= MODEL;
        }
        else if ( !Q_stricmp( token, "modelScale" ) )
        {
            float scale;

            PARSE(text, token);

            scale = atof( token );

            if ( scale < 0.0f )
            {
                scale = 0.0f;
            }

            bc->modelScale = scale;

            defined |= MODELSCALE;
        }
        else if ( !Q_stricmp( token, "mins" ) )
        {
            int i;

            for ( i = 0; i <= 2; i++ )
            {
                PARSE(text, token);

                bc->mins[ i ] = atof( token );
            }

            defined |= MINS;
        }
        else if ( !Q_stricmp( token, "maxs" ) )
        {
            int i;

            for ( i = 0; i <= 2; i++ )
            {
                PARSE(text, token);

                bc->maxs[ i ] = atof( token );
            }

            defined |= MAXS;
        }
        else if ( !Q_stricmp( token, "zOffset" ) )
        {
            PARSE(text, token);

            bc->zOffset = atof( token );

            defined |= ZOFFSET;
        }
        else if ( !Q_stricmp( token, "oldScale" ) )
        {
            PARSE(text, token);

            bc->oldScale = atof( token );

            defined |= OLDSCALE;
        }
        else if ( !Q_stricmp( token, "oldOffset" ) )
        {
            PARSE(text, token);

            bc->oldOffset = atof( token );

            defined |= OLDOFFSET;
        }
        else
        {
            Com_Printf( S_COLOR_RED "ERROR: unknown token '%s'\n", token );
        }
    }

    if ( !( defined & MODEL ) ) { token = "model"; }
    else if ( !( defined & MODELSCALE ) ) { token = "modelScale"; }
    else if ( !( defined & MINS ) ) { token = "mins"; }
    else if ( !( defined & MAXS ) ) { token = "maxs"; }
    else if ( !( defined & ZOFFSET ) ) { token = "zOffset"; }
    else { token = ""; }

    if ( strlen( token ) > 0 )
    {
        Com_Printf( S_COLOR_RED "ERROR: %s not defined in %s\n",
                    token, filename );
    }
}

/*
======================
BG_ParseClassAttributeFile

Parses a configuration file describing the attributes of a class
======================
*/

void BG_ParseClassAttributeFile( const char *filename, classAttributes_t *ca )
{
    char *token;
    char text_buffer[ 20000 ];
    char* text;
    int defined = 0;
    enum
    {
      INFO = 1 << 0,
      FOVCVAR = 1 << 1,
      STAGE = 1 << 2,
      HEALTH = 1 << 3,
      FALLDAMAGE = 1 << 4,
      REGEN = 1 << 5,
      FOV = 1 << 6,
      STEPTIME = 1 << 7,
      SPEED = 1 << 8,
      ACCELERATION = 1 << 9,
      AIRACCELERATION = 1 << 10,
      FRICTION = 1 << 11,
      STOPSPEED = 1 << 12,
      JUMPMAGNITUDE = 1 << 13,
      KNOCKBACKSCALE = 1 << 14,
      COST = 1 << 15,
      VALUE = 1 << 16,
      RADAR = 1 << 17,
    };

    if( !BG_ReadWholeFile( filename, text_buffer, sizeof(text_buffer) ) )
    {
        return;
    }

    text = text_buffer;

    // read optional parameters
    while ( 1 )
    {
        PARSE(text, token);

        if ( !Q_stricmp( token, "description" ) )
        {
            PARSE(text, token);

            if ( !Q_stricmp( token, "null" ) )
            {
                ca->info = "";
            }
            else
            {
                ca->info = strdup(token);
            }

            defined |= INFO;
        }
        else if ( !Q_stricmp( token, "fovCvar" ) )
        {
            PARSE(text, token);

            if ( !Q_stricmp( token, "null" ) )
            {
                ca->fovCvar = "";
            }
            else
            {
                ca->fovCvar = strdup(token);
            }

            defined |= FOVCVAR;
        }
        else if ( !Q_stricmp( token, "stage" ) )
        {
            PARSE(text, token);

            ca->stages = BG_StageFromNumber( atoi(token) );

            defined |= STAGE;
        }
        else if ( !Q_stricmp( token, "health" ) )
        {
            PARSE(text, token);

            ca->health = atoi( token );

            defined |= HEALTH;
        }
        else if ( !Q_stricmp( token, "fallDamage" ) )
        {
            PARSE(text, token);

            ca->fallDamage = atof( token );

            defined |= FALLDAMAGE;
        }
        else if ( !Q_stricmp( token, "regen" ) )
        {
            PARSE(text, token);

            ca->regenRate = atof( token );

            defined |= REGEN;
        }
        //Abilities ...
        else if ( !Q_stricmp( token, "wallClimber" ) )
        {
            ca->abilities |= SCA_WALLCLIMBER;
        }
        else if ( !Q_stricmp( token, "takesFallDamage" ) )
        {
            ca->abilities |= SCA_TAKESFALLDAMAGE;
        }
        else if ( !Q_stricmp( token, "fovWarps" ) )
        {
            ca->abilities |= SCA_FOVWARPS;
        }
        else if ( !Q_stricmp( token, "alienSense" ) )
        {
            ca->abilities |= SCA_ALIENSENSE;
        }
        else if ( !Q_stricmp( token, "canUseLadders" ) )
        {
            ca->abilities |= SCA_CANUSELADDERS;
        }
        else if ( !Q_stricmp( token, "wallJumper" ) )
        {
            ca->abilities |= SCA_WALLJUMPER;
        }
        else if ( !Q_stricmp( token, "buildDistance" ) )
        {
            PARSE(text, token);

            ca->buildDist = atof( token );
        }
        else if ( !Q_stricmp( token, "fov" ) )
        {
            PARSE(text, token);

            ca->fov = atoi( token );

            defined |= FOV;
        }
        else if ( !Q_stricmp( token, "bob" ) )
        {
            PARSE(text, token);

            ca->bob = atof( token );
        }
        else if ( !Q_stricmp( token, "bobCycle" ) )
        {
            PARSE(text, token);

            ca->bobCycle = atof( token );
        }
        else if ( !Q_stricmp( token, "stepTime" ) )
        {
            PARSE(text, token);

            ca->steptime = atoi( token );

            defined |= STEPTIME;
        }
        else if ( !Q_stricmp( token, "speed" ) )
        {
            PARSE(text, token);

            ca->speed = atof( token );

            defined |= SPEED;
        }
        else if ( !Q_stricmp( token, "acceleration" ) )
        {
            PARSE(text, token);

            ca->acceleration = atof( token );

            defined |= ACCELERATION;
        }
        else if ( !Q_stricmp( token, "airAcceleration" ) )
        {
            PARSE(text, token);

            ca->airAcceleration = atof( token );

            defined |= AIRACCELERATION;
        }
        else if ( !Q_stricmp( token, "friction" ) )
        {
            PARSE(text, token);

            ca->friction = atof( token );

            defined |= FRICTION;
        }
        else if ( !Q_stricmp( token, "stopSpeed" ) )
        {
            PARSE(text, token);

            ca->stopSpeed = atof( token );

            defined |= STOPSPEED;
        }
        else if ( !Q_stricmp( token, "jumpMagnitude" ) )
        {
            PARSE(text, token);

            ca->jumpMagnitude = atof( token );

            defined |= JUMPMAGNITUDE;
        }
        else if ( !Q_stricmp( token, "knockbackScale" ) )
        {
            PARSE(text, token);

            ca->knockbackScale = atof( token );

            defined |= KNOCKBACKSCALE;
        }
        else if ( !Q_stricmp( token, "cost" ) )
        {
            PARSE(text, token);

            ca->cost = atoi( token );

            defined |= COST;
        }
        else if ( !Q_stricmp( token, "value" ) )
        {
            PARSE(text, token);

            ca->value = atoi( token );

            defined |= VALUE;
        }
        else if ( !Q_stricmp( token, "radarFadeOut" ) )
        {
            PARSE(text, token);

            ca->radarFadeOut = atof( token );

            defined |= RADAR;
        }
        else
        {
            Com_Printf( S_COLOR_RED "ERROR: unknown token '%s'\n", token );
        }
    }

    if ( !( defined & INFO ) ) { token = "description"; }
    else if ( !( defined & FOVCVAR ) ) { token = "fovCvar"; }
    else if ( !( defined & STAGE ) ) { token = "stage"; }
    else if ( !( defined & HEALTH ) ) { token = "health"; }
    else if ( !( defined & FALLDAMAGE ) ) { token = "fallDamage"; }
    else if ( !( defined & REGEN ) ) { token = "regen"; }
    else if ( !( defined & FOV ) ) { token = "fov"; }
    else if ( !( defined & STEPTIME ) ) { token = "stepTime"; }
    else if ( !( defined & SPEED ) ) { token = "speed"; }
    else if ( !( defined & ACCELERATION ) ) { token = "acceleration"; }
    else if ( !( defined & AIRACCELERATION ) ) { token = "airAcceleration"; }
    else if ( !( defined & FRICTION ) ) { token = "friction"; }
    else if ( !( defined & STOPSPEED ) ) { token = "stopSpeed"; }
    else if ( !( defined & JUMPMAGNITUDE ) ) { token = "jumpMagnitude"; }
    else if ( !( defined & KNOCKBACKSCALE ) ) { token = "knockbackScale"; }
    else if ( !( defined & COST ) ) { token = "cost"; }
    else if ( !( defined & VALUE ) ) { token = "value"; }
    else if ( !( defined & RADAR ) ) { token = "radarFadeOut"; }
    else { token = ""; }

    if ( strlen( token ) > 0 )
    {
        Com_Printf( S_COLOR_RED "ERROR: %s not defined in %s\n",
                    token, filename );
    }
}

/*
======================
BG_ParseClassModelFile

Parses a configuration file describing the model of a class
======================
*/
void BG_ParseClassModelFile( const char *filename, classModelConfig_t *cc )
{
    char *token;
    char text_buffer[ 20000 ];
    char* text;
    int defined = 0;
    enum
    {
      MODEL = 1 << 0,
      SKIN = 1 << 1,
      HUD = 1 << 2,
      MODELSCALE = 1 << 3,
      SHADOWSCALE = 1 << 4,
      MINS = 1 << 5,
      MAXS = 1 << 6,
      DEADMINS = 1 << 7,
      DEADMAXS = 1 << 8,
      CROUCHMAXS = 1 << 9,
      VIEWHEIGHT = 1 << 10,
      CVIEWHEIGHT = 1 << 11,
      ZOFFSET = 1 << 12,
      NAME = 1 << 13,
      SHOULDEROFFSETS = 1 << 14
    };

    if( !BG_ReadWholeFile( filename, text_buffer, sizeof(text_buffer) ) )
    {
        return;
    }

    text = text_buffer;

    // read optional parameters
    while ( 1 )
    {
        PARSE(text, token);

        if ( !Q_stricmp( token, "model" ) )
        {
            PARSE(text, token);

            //Allow spectator to have an empty model
            if ( !Q_stricmp( token, "null" ) )
            {
                cc->modelName[0] = '\0';
            }
            else
            {
                Q_strncpyz( cc->modelName, token, sizeof( cc->modelName ) );
            }

            defined |= MODEL;
        }
        else if ( !Q_stricmp( token, "skin" ) )
        {
            PARSE(text, token);

            if ( !Q_stricmp( token, "null" ) )
            {
                cc->skinName[0] = '\0';
            }
            else
            {
                Q_strncpyz( cc->skinName, token, sizeof( cc->skinName ) );
            }

            defined |= SKIN;
        }
        else if ( !Q_stricmp( token, "hud" ) )
        {
            PARSE(text, token);

            if ( !Q_stricmp( token, "null" ) )
            {
                cc->hudName[0] = '\0';
            }
            else
            {
                Q_strncpyz( cc->hudName, token, sizeof( cc->hudName ) );
            }

            defined |= HUD;
        }
        else if ( !Q_stricmp( token, "modelScale" ) )
        {
            float scale;

            PARSE(text, token);

            scale = atof( token );

            if ( scale < 0.0f )
            {
                scale = 0.0f;
            }

            cc->modelScale = scale;

            defined |= MODELSCALE;
        }
        else if ( !Q_stricmp( token, "shadowScale" ) )
        {
            float scale;

            PARSE(text, token);

            scale = atof( token );

            if ( scale < 0.0f )
            {
                scale = 0.0f;
            }

            cc->shadowScale = scale;

            defined |= SHADOWSCALE;
        }
        else if ( !Q_stricmp( token, "mins" ) )
        {
            int i;

            for ( i = 0; i <= 2; i++ )
            {
                PARSE(text, token);

                cc->mins[ i ] = atof( token );
            }

            defined |= MINS;
        }
        else if ( !Q_stricmp( token, "maxs" ) )
        {
            int i;

            for ( i = 0; i <= 2; i++ )
            {
                PARSE(text, token);

                cc->maxs[ i ] = atof( token );
            }

            defined |= MAXS;
        }
        else if ( !Q_stricmp( token, "deadMins" ) )
        {
            int i;

            for ( i = 0; i <= 2; i++ )
            {
                PARSE(text, token);

                cc->deadMins[ i ] = atof( token );
            }

            defined |= DEADMINS;
        }
        else if ( !Q_stricmp( token, "deadMaxs" ) )
        {
            int i;

            for ( i = 0; i <= 2; i++ )
            {
                PARSE(text, token);

                cc->deadMaxs[ i ] = atof( token );
            }

            defined |= DEADMAXS;
        }
        else if ( !Q_stricmp( token, "crouchMaxs" ) )
        {
            int i;

            for ( i = 0; i <= 2; i++ )
            {
                PARSE(text, token);

                cc->crouchMaxs[ i ] = atof( token );
            }

            defined |= CROUCHMAXS;
        }
        else if ( !Q_stricmp( token, "viewheight" ) )
        {
            PARSE(text, token);

            cc->viewheight = atoi( token );

            defined |= VIEWHEIGHT;
        }
        else if ( !Q_stricmp( token, "crouchViewheight" ) )
        {
            PARSE(text, token);

            cc->crouchViewheight = atoi( token );

            defined |= CVIEWHEIGHT;
        }
        else if ( !Q_stricmp( token, "zOffset" ) )
        {
            PARSE(text, token);

            cc->zOffset = atof( token );

            defined |= ZOFFSET;
        }
        else if ( !Q_stricmp( token, "name" ) )
        {
            PARSE(text, token);

            cc->humanName = strdup( token );

            defined |= NAME;
        }
        else if ( !Q_stricmp( token, "shoulderOffsets" ) )
        {
            int i;

            for ( i = 0; i <= 2; i++ )
            {
                PARSE(text, token);

                cc->shoulderOffsets[ i ] = atof( token );
            }

            defined |= SHOULDEROFFSETS;
        }
        else
        {
            Com_Printf( S_COLOR_RED "ERROR: unknown token '%s'\n", token );
        }
    }

    if ( !( defined & MODEL ) ) { token = "model"; }
    else if ( !( defined & SKIN ) ) { token = "skin"; }
    else if ( !( defined & HUD ) ) { token = "hud"; }
    else if ( !( defined & MODELSCALE ) ) { token = "modelScale"; }
    else if ( !( defined & SHADOWSCALE ) ) { token = "shadowScale"; }
    else if ( !( defined & MINS ) ) { token = "mins"; }
    else if ( !( defined & MAXS ) ) { token = "maxs"; }
    else if ( !( defined & DEADMINS ) ) { token = "deadMins"; }
    else if ( !( defined & DEADMAXS ) ) { token = "deadMaxs"; }
    else if ( !( defined & CROUCHMAXS ) ) { token = "crouchMaxs"; }
    else if ( !( defined & VIEWHEIGHT ) ) { token = "viewheight"; }
    else if ( !( defined & CVIEWHEIGHT ) ) { token = "crouchViewheight"; }
    else if ( !( defined & ZOFFSET ) ) { token = "zOffset"; }
    else if ( !( defined & NAME ) ) { token = "name"; }
    else if ( !( defined & SHOULDEROFFSETS ) ) { token = "shoulderOffsets"; }
    else { token = ""; }

    if ( strlen( token ) > 0 )
    {
        Com_Printf( S_COLOR_RED "ERROR: %s not defined in %s\n",
                    token, filename );
    }
}

