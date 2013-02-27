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
void                               trap_FS_Seek( fileHandle_t f, long offset, fsOrigin_t origin );  // fsOrigin_t
int                                trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
void                               trap_QuoteString( const char *, char *, int );

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

/*
======================
BG_ParseBuildableAttributeFile

Parses a configuration file describing the attributes of a buildable
======================
*/

qboolean BG_ParseBuildableAttributeFile( const char *filename, buildableAttributes_t *ba )
{
    int i;
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
        return qfalse;
    }

    text = text_buffer;

    while ( 1 )
    {
        PARSE(text, token);

        if ( !Q_stricmp( token, "humanName" ) )
        {
            PARSE(text, token);

            Q_strncpyz( ba->humanName, token, sizeof( ba->humanName ) );

            defined |= HUMANNAME;
        }
        else if ( !Q_stricmp( token, "description" ) )
        {
            PARSE(text, token);

            Q_strncpyz( ba->info, token, sizeof( ba->info ) );

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
            ba->stages = (1 << (S1 + atoi(token))) - 1;

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
                break;
            }

            defined |= DEATHMOD;
        }
        else if ( !Q_stricmp( token, "team" ) )
        {
            PARSE(text, token);

            if ( !Q_stricmp( token, "aliens" ) )
            {
                ba->team = TEAM_ALIENS;
            }
            else if ( !Q_stricmp( token, "humans" ) )
            {
                ba->team = TEAM_HUMANS;
            }
            else
            {
                Com_Printf( S_COLOR_RED "ERROR: unknown team value '%s'\n", token );
                break;
            }

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
                break;
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
                break;
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
            return qfalse;
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
        return qfalse;
    }

    return qtrue;
}

/*
======================
BG_ParseBuildableModelFile

Parses a configuration file describing the model of a buildable
======================
*/
qboolean BG_ParseBuildableModelFile( const char *filename, buildableModelConfig_t *bc )
{
    int i;
    char *token;
    char text_buffer[ 20000 ];
    char* text;
    int defined = 0;
    float scale;
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
        return qfalse;
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
            continue;
        }
        else if ( !Q_stricmp( token, "modelScale" ) )
        {
            PARSE(text, token);

            scale = atof( token );

            if ( scale < 0.0f )
            {
                scale = 0.0f;
            }

            bc->modelScale = scale;

            defined |= MODELSCALE;
            continue;
        }
        else if ( !Q_stricmp( token, "mins" ) )
        {
            for ( i = 0; i <= 2; i++ )
            {
                PARSE(text, token);

                bc->mins[ i ] = atof( token );
            }

            defined |= MINS;
            continue;
        }
        else if ( !Q_stricmp( token, "maxs" ) )
        {
            for ( i = 0; i <= 2; i++ )
            {
                PARSE(text, token);

                bc->maxs[ i ] = atof( token );
            }

            defined |= MAXS;
            continue;
        }
        else if ( !Q_stricmp( token, "zOffset" ) )
        {
            float offset;

            PARSE(text, token);

            offset = atof( token );

            bc->zOffset = offset;

            defined |= ZOFFSET;
            continue;
        }
        else if ( !Q_stricmp( token, "oldScale" ) )
        {
            float scale;

            PARSE(text, token);

            scale = atof( token );

            bc->oldScale = scale;

            defined |= OLDSCALE;
            continue;
        }
        else if ( !Q_stricmp( token, "oldOffset" ) )
        {
            float offset;

            PARSE(text, token);

            offset = atof( token );

            bc->oldOffset = offset;

            defined |= OLDOFFSET;
            continue;
        }

        Com_Printf( S_COLOR_RED "ERROR: unknown token '%s'\n", token );
        return qfalse;
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
        return qfalse;
    }

    return qtrue;
}
