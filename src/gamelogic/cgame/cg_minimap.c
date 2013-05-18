/*
===========================================================================
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

#include "cg_local.h"

#define MINIMAP_DEFAULT_DISPLAY_SIZE 1024.0

//The minimap parser

/*
================
ParseFloats
================
*/
qboolean ParseFloats( float* res, int number, char **text )
{
    char* token;

    while( number --> 0 )
    {
        if( !*(token = COM_Parse( text )) )
        {
            return qfalse;
        }

        *res = atof(token);
        res ++;
    }

    return qtrue;
}

/*
================
CG_ParseMinimapZone
================
*/
qboolean CG_ParseMinimapZone( minimapZone_t* z, char **text )
{
    char* token;
    qboolean hasImage = qfalse;
    qboolean hasBounds = qfalse;

    z->scale = 1.0f;

    if( !*(token = COM_Parse( text )) || Q_stricmp( token, "{" ) )
    {
        CG_Printf( S_ERROR "expected a { at the beginning of a zones\n" );
        return qfalse;
    }

    while(1)
    {
        if( !*(token = COM_Parse( text )) )
        {
            break;
        }

        if( !Q_stricmp( token, "bounds" ) )
        {
            if( !ParseFloats( z->boundsMin, 3, text) || !ParseFloats( z->boundsMax, 3, text) )
            {
                CG_Printf( S_ERROR "error while parsing 'bounds'\n" );
                return qfalse;
            }

            hasBounds = qtrue;
        }
        else if( !Q_stricmp( token, "image") )
        {
            if( !*(token = COM_Parse( text )) )
            {
                CG_Printf( S_ERROR "error while parsing the image name while parsing 'image'\n" );
            }

            CG_Printf( "loading minimap image %s\n", token);

            z->image = trap_R_RegisterShader( token, RSF_DEFAULT );

            CG_Printf( "registershader returned %i\n", z->image );

            if( !ParseFloats( z->imageMin, 2, text) || !ParseFloats( z->imageMax, 2, text) )
            {
                CG_Printf( S_ERROR "error while parsing 'image'\n" );
                return qfalse;
            }

            hasImage = qtrue;
        }
        else if( !Q_stricmp( token, "scale" ))
        {
            if( !*(token = COM_Parse( text )) )
            {
                CG_Printf( S_ERROR "error while parsing the value  while parsing 'scale'\n" );
            }

            z->scale = atof( token );
        }
        else if( !Q_stricmp( token, "}" ) )
        {
            break;
        }
        else
        {
            Com_Printf( S_ERROR "unknown token '%s'\n", token );
        }
    }

    if( Q_stricmp( token, "}" ) )
    {
        CG_Printf( S_ERROR "expected a } at the end of a zone\n");
        return qfalse;
    }

    if( !hasBounds )
    {
        CG_Printf( S_ERROR "missing bounds in the zone\n" );
        return qfalse;
    }

    if( !hasImage )
    {
        CG_Printf( S_ERROR "missing image in the zone\n" );
        return qfalse;
    }

    return qtrue;
}

/*
================
CG_ParseMinimap
================
*/
qboolean CG_ParseMinimap( minimap_t* m, char* filename )
{
    char text_buffer[ 20000 ];
    char* text;
    char* token;

    m->nZones = 0;
    m->lastZone = -1;
    m->bgColor[3] = 1.0f;

    if( !BG_ReadWholeFile( filename, text_buffer, sizeof(text_buffer) ) )
    {
        return qfalse;
    }

    text = text_buffer;

    if( !*(token = COM_Parse( &text )) || Q_stricmp( token, "{" ) )
    {
        CG_Printf( S_ERROR "expected a { at the beginning of %s\n", filename );
        return qfalse;
    }

    while(1)
    {
        if( !*(token = COM_Parse( &text )) )
        {
            break;
        }

        if( !Q_stricmp( token, "zone" ) ){
            m->nZones ++;

            if( m->nZones > MAX_MINIMAP_ZONES )
            {
                CG_Printf( S_ERROR "reached the zone number limit (%i) in %s\n", MAX_MINIMAP_ZONES, filename );
                return qfalse;
            }

            if( !CG_ParseMinimapZone( &m->zones[m->nZones - 1], &text ) )
            {
                CG_Printf( S_ERROR "error while reading zone n°%i in %s\n", m->nZones, filename );
                return qfalse;
            }
        }
        else if( !Q_stricmp( token, "backgroundColor") )
        {
            if( !ParseFloats( m->bgColor, 3, &text) )
            {
                CG_Printf( S_ERROR "error while parsing 'backgroundColor' in %s\n", filename );
                return qfalse;
            }
        }
        else if( !Q_stricmp( token, "}" ) )
        {
            break;
        }
        else
        {
            Com_Printf( S_ERROR "%s: unknown token '%s'\n", filename, token );
        }
    }

    if( Q_stricmp( token, "}" ) )
    {
        CG_Printf( S_ERROR "expected a } at the end of %s\n", filename );
        return qfalse;
    }

    return qtrue;
}

//Helper functions for the minimap

/*
================
CG_IsInMinimapZone
================
*/
qboolean CG_IsInMinimapZone(minimapZone_t* z)
{
    return PointInBounds(cg.refdef.vieworg, z->boundsMin, z->boundsMax);
}

//The parameters for the current frame's minimap transform
static float transform[4];
static float transformVector[2];
static float transformAngle;
static float transformScale;

/*
================
CG_SetupMinimapTransform
================
*/
void CG_SetupMinimapTransform( const rectDef_t *rect, const minimap_t* minimap, const minimapZone_t* zone)
{
    float posx, posy, x, y, s, c, angle, scale;

    //The refdefview angle is the angle from the x axis
    //the 90 gets it back to the Y axis (we want the view to point up)
    //and the orientation change gives the -
    transformAngle = - (cg.refdefViewAngles[1] - 90.0);
    transformScale = zone->scale;

    angle = DEG2RAD(transformAngle);
    scale = transformScale * MINIMAP_DEFAULT_DISPLAY_SIZE / (zone->imageMax[0] - zone->imageMin[0]);
    s = sin(angle) * scale;
    c = cos(angle) * scale;

    //Simply a 2x2 rotoscale matrix
    transform[0] =  c;
    transform[1] =  s;
    transform[2] = -s;
    transform[3] =  c;

    //the minimap is shown with Z pointing to the viewer but OpenGL has Z pointing to the screen
    //thus the 2d axis don't have the same orientation
    posx = - cg.refdef.vieworg[0];
    posy = + cg.refdef.vieworg[1];

    //Compute the constant member of the transform
    x = transform[0] * posx + transform[1] * posy;
    y = transform[2] * posx + transform[3] * posy;

    transformVector[0] = x + rect->x + rect->w / 2;
    transformVector[1] = y + rect->y + rect->h / 2;
}

/*
================
CG_WorldToMinimap
================
*/
void CG_WorldToMinimap( const vec3_t worldPos, vec2_t minimapPos )
{
    minimapPos[0] = transform[0] * worldPos[0] + transform[1] * worldPos[1] +
                    transformVector[0];
    minimapPos[1] = transform[2] * worldPos[0] + transform[3] * worldPos[1] +
                    transformVector[1];
}

/*
================
CG_WorldToMinimapAngle
================
*/
float CG_WorldToMinimapAngle( void )
{
    return transformAngle;
}

float CG_WorldToMinimapScale( void )
{
    return transformScale;
}

//Entry points in the minimap code

/*
================
CG_InitMinimap
================
*/
void CG_InitMinimap( void )
{
    minimap_t* m = &cg.minimap;

    m->active = qtrue;

    if( !CG_ParseMinimap( m, va( "minimaps/%s.minimap", cgs.mapname ) ) )
    {
        m->active = qfalse;
        CG_Printf( S_WARNING "could not parse the minimap, defaulting to no minimap.\n" );
        return;
    }

    if( m->nZones == 0 )
    {
        m->active = qfalse;
        CG_Printf( S_ERROR "the minimap did not define any zone.\n" );
        return;
    }
}

/*
================
CG_DrawMinimap
================
*/
void CG_DrawMinimap( const rectDef_t* rect640 )
{
    minimap_t* m = &cg.minimap;
    minimapZone_t *z = NULL;
    float angle;
    float scale;
    float transform[4];
    rectDef_t rect = *rect640;

    //TODO: add a drawMinimap cvar and also
    //TODO: add a hasMinimap cvar for the hud to hide some elements if the map doesn't provide a minimap
    if( !m->active )
    {
        return;
    }

    //Chooses the current zone, tries the last zone first
    //More than providing a performance improvement it helps
    //the mapper make a nicer looking minimap: once you enter a zone I'll stay in it
    //until you reach the bounds
    if( m->lastZone < 0 || !CG_IsInMinimapZone( &m->zones[m->lastZone] ) )
    {
        int i;
        for( i = 0; i < m->nZones; i++ )
        {
            if( CG_IsInMinimapZone( &m->zones[i] ) )
            {
                break;
            }
        }

        //The mapper should make sure this never happens but we prevent crashes
        //could also be used to make a default zone
        if( i >= m->nZones )
        {
            i = m->nZones - 1;
        }

        z = &m->zones[i];
        m->lastZone = i;
    }
    else
    {
        z = &m->zones[m->lastZone];
    }


    //Setup the transform
    CG_AdjustFrom640( &rect.x, &rect.y, &rect.w, &rect.h );
    CG_SetupMinimapTransform( &rect, m, z );

    angle = CG_WorldToMinimapAngle();
    scale = CG_WorldToMinimapScale();

    //Testing code
    CG_SetScissor( rect.x, rect.y, rect.w, rect.h );
    CG_EnableScissor( qtrue );
    {
        CG_FillRect( rect640->x, rect640->y, rect640->w, rect640->h, m->bgColor );

        {
            vec2_t mapOffset;
            vec3_t origin = {0.0f, 0.0f, 0.0f};
            CG_WorldToMinimap(origin, mapOffset);

            trap_R_DrawRotatedPic(-512 * scale + mapOffset[0], -512 * scale + mapOffset[1], 1024 * scale, 1024 * scale, 0.0, 0.0, 1.0, 1.0, z->image, angle);
        }
        {
            float c[4] = {1.0, 0.0, 0.0, 1.0};
            CG_DrawRect( rect640->x, rect640->y, rect640->w, rect640->h, 1.0, c );
            CG_DrawRect( rect640->x + rect640->w/2.0 - 2.0, rect640->y + rect640->h/2.0 - 2.0, 4.0, 4.0, 1.0, c );
            CG_DrawRect( rect640->x + rect640->w/2.0 - 1.0, rect640->y + rect640->h/2.0 - 6.0, 2.0, 4.0, 1.0, c );
        }
    }
    CG_EnableScissor( qfalse );
}

