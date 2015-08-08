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

#define MINIMAP_MAP_DISPLAY_SIZE 1024.0f
#define MINIMAP_PLAYER_DISPLAY_SIZE 50.0f
#define MINIMAP_TEAMMATE_DISPLAY_SIZE 50.0f

//How big a region we want to show
#define MINIMAP_DEFAULT_SIZE 600.0f

//It is multiplied by msecs
#define MINIMAP_FADE_TIME (2.0f / 1000.0f)

//The minimap parser

/*
================
ParseFloats
================
*/
static bool ParseFloats( float* res, const int number, const char **text )
{
    char* token;
    int i = number;

    while( i --> 0 )
    {
        if( !*(token = COM_Parse( text )) )
        {
            return false;
        }

        *res = atof(token);
        res ++;
    }

    return true;
}

/*
================
CG_ParseMinimapZone
================
*/
static bool CG_ParseMinimapZone( minimapZone_t* z, const char **text )
{
    char* token;
    bool hasImage = false;
    bool hasBounds = false;

    z->scale = 1.0f;

    if( !*(token = COM_Parse( text )) || Q_stricmp( token, "{" ) )
    {
        CG_Printf( S_ERROR "expected a { at the beginning of a zones\n" );
        return false;
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
                return false;
            }

            hasBounds = true;
        }
        else if( !Q_stricmp( token, "image") )
        {
            if( !*(token = COM_Parse( text )) )
            {
                CG_Printf( S_ERROR "error while parsing the image name while parsing 'image'\n" );
            }

            z->image = trap_R_RegisterShader( token, RSF_DEFAULT );

            if( !ParseFloats( z->imageMin, 2, text) || !ParseFloats( z->imageMax, 2, text) )
            {
                CG_Printf( S_ERROR "error while parsing 'image'\n" );
                return false;
            }

            hasImage = true;
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
        return false;
    }

    if( !hasBounds )
    {
        CG_Printf( S_ERROR "missing bounds in the zone\n" );
        return false;
    }

    if( !hasImage )
    {
        CG_Printf( S_ERROR "missing image in the zone\n" );
        return false;
    }

    return true;
}

/*
================
CG_ParseMinimap
================
*/
static bool CG_ParseMinimap( minimap_t* m, const char* filename )
{
    char text_buffer[ 20000 ];
    const char* text;
    char* token;

    m->nZones = 0;
    m->lastZone = -1;
    m->scale = 1.0f;
    m->bgColor[3] = 0.333f; // black, approx. 1/3 opacity

    if( !BG_ReadWholeFile( filename, text_buffer, sizeof(text_buffer) ) )
    {
        return false;
    }

    text = text_buffer;

    if( !*(token = COM_Parse( &text )) || Q_stricmp( token, "{" ) )
    {
        CG_Printf( S_ERROR "expected a { at the beginning of %s\n", filename );
        return false;
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
                return false;
            }

            if( !CG_ParseMinimapZone( &m->zones[m->nZones - 1], &text ) )
            {
                CG_Printf( S_ERROR "error while reading zone n°%i in %s\n", m->nZones, filename );
                return false;
            }
        }
        else if( !Q_stricmp( token, "backgroundColor") )
        {
            if( !ParseFloats( m->bgColor, 4, &text) )
            {
                CG_Printf( S_ERROR "error while parsing 'backgroundColor' in %s\n", filename );
                return false;
            }
        }
        else if( !Q_stricmp( token, "globalScale") )
        {
            if( !ParseFloats( &m->scale, 1, &text) )
            {
                CG_Printf( S_ERROR "error while parsing 'globalScale' in %s\n", filename );
                return false;
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
        return false;
    }

    return true;
}

//Helper functions for the minimap

/*
================
CG_IsInMinimapZone
================
*/
static bool CG_IsInMinimapZone(const minimapZone_t* z)
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
static void CG_SetupMinimapTransform( const rectDef_t *rect, const minimap_t* minimap, const minimapZone_t* zone)
{
    float posx, posy, x, y, s, c, angle, scale;

    Q_UNUSED(minimap);

    //The refdefview angle is the angle from the x axis
    //the 90 gets it back to the Y axis (we want the view to point up)
    //and the orientation change gives the -
    transformAngle = - cg.refdefViewAngles[1];
    angle = DEG2RAD(transformAngle + 90.0);

    //Try to show the same region of the map for everyong
    transformScale = (rect->w + rect->h) / 2.0f / MINIMAP_DEFAULT_SIZE;

    scale = transformScale * MINIMAP_MAP_DISPLAY_SIZE / (zone->imageMax[0] - zone->imageMin[0]);

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
static void CG_WorldToMinimap( const vec3_t worldPos, vec2_t minimapPos )
{
    //Correct the orientation by inverting worldPos.y
    minimapPos[0] = transform[0] * worldPos[0] - transform[1] * worldPos[1] +
                    transformVector[0];
    minimapPos[1] = transform[2] * worldPos[0] - transform[3] * worldPos[1] +
                    transformVector[1];
}

/*
================
CG_WorldToMinimapAngle
================
*/
static float CG_WorldToMinimapAngle( const float angle )
{
    return angle + transformAngle;
}

/*
================
CG_WorldToMinimapScale
================
*/
static float CG_WorldToMinimapScale( const float scale )
{
    return scale * transformScale;
}


/*
================
CG_SetMinimapColor
================
*/
static vec4_t currentMinimapColor;
static void CG_SetMinimapColor( const vec4_t color)
{
    VectorCopy( color, currentMinimapColor );
}

/*
================
CG_DrawMinimapObject
================
*/
static void CG_DrawMinimapObject( const qhandle_t image, const vec3_t pos3d, const float angle, const float scale, const float texSize, const float alpha )
{
    vec2_t offset;
    float x, y, wh, realScale, realAngle;

    realAngle = CG_WorldToMinimapAngle( angle );
    realScale = CG_WorldToMinimapScale( scale );

    CG_WorldToMinimap( pos3d, offset );
    x = - texSize/2 * realScale + offset[0];
    y = - texSize/2 * realScale + offset[1];
    wh = texSize * realScale;

    //Handle teamcolor + transparency
    currentMinimapColor[3] = alpha;
    trap_R_SetColor( currentMinimapColor );

    trap_R_DrawRotatedPic( x, y, wh, wh, 0.0, 0.0, 1.0, 1.0, image, realAngle );
}

/*
================
CG_UpdateMinimapActive
================
*/
static void CG_UpdateMinimapActive(minimap_t* m)
{
    bool active = m->defined && cg_drawMinimap.integer;

    m->active = active;

    if ((cg_minimapActive.integer != 0) != active)
    {
        trap_Cvar_Set( "cg_minimapActive", va( "%d", active ) );
    }
}

//Other logical minimap functions

/*
================
CG_ChooseMinimapZone

Chooses the current zone, tries the last zone first
More than providing a performance improvement it helps
the mapper make a nicer looking minimap: once you enter a zone I'll stay in it
until you reach the bounds
================
*/
static minimapZone_t* CG_ChooseMinimapZone( minimap_t* m )
{
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

        m->lastZone = i;

        return &m->zones[i];
    }
    else
    {
        return &m->zones[m->lastZone];
    }
}

/*
================
CG_MinimapDrawMap
================
*/
static void CG_MinimapDrawMap( const minimap_t* m, const minimapZone_t* z )
{
    vec3_t origin = {0.0f, 0.0f, 0.0f};
    origin[0] = 0.5 * (z->imageMin[0] + z->imageMax[0]);
    origin[1] = 0.5 * (z->imageMin[1] + z->imageMax[1]);

    CG_DrawMinimapObject( z->image, origin, 90.0, m->scale * z->scale, MINIMAP_MAP_DISPLAY_SIZE, 1.0 );
}

/*
================
CG_MinimapDrawPlayer
================
*/
static void CG_MinimapDrawPlayer( const minimap_t* m )
{
    CG_DrawMinimapObject( m->gfx.playerArrow, cg.refdef.vieworg, cg.refdefViewAngles[1], 1.0, MINIMAP_PLAYER_DISPLAY_SIZE, 1.0 );
}

/*
================
CG_MinimapUpdateTeammateFadingAndPos

When the player leaves the PVS we cannot track its movement on the minimap
anymore so we fade its arrow by keeping in memory its last know pos and angle.
When he comes back in the PVS we don't want to have to manage two arrows or to
make the arrow warp. That's why we wait until the fadeout is finished before
fading it back in.
================
*/
/*
static void CG_MinimapUpdateTeammateFadingAndPos( centity_t* mate )
{
    playerEntity_t* state = &mate->pe;

    if( state->minimapFadingOut )
    {
        if( state->minimapFading != 0.0f )
        {
            state->minimapFading = MAX( 0.0f, state->minimapFading - cg.frametime * MINIMAP_FADE_TIME );
        }

        if( state->minimapFading == 0.0f )
        {
            state->minimapFadingOut = false;
        }
    }
    else
    {
        //The player is out of the PVS or is dead
        bool shouldStayVisible = mate->valid && ! ( mate->currentState.eFlags & EF_DEAD );

        if( !shouldStayVisible )
        {
            state->minimapFadingOut = true;
        }
        else
        {
            if( state->minimapFading != 1.0f )
            {
                state->minimapFading = MIN( 1.0f, state->minimapFading + cg.frametime * MINIMAP_FADE_TIME );
            }

            //Copy the current state so that we can use it once the player is out of the PVS
            VectorCopy( mate->lerpOrigin, state->lastMinimapPos );
            state->lastMinimapAngle = mate->lerpAngles[1];
        }
    }
}
*/

/*
================
CG_MinimapDrawTeammates
================
*/
/*
static void CG_MinimapDrawTeammates( const minimap_t* m )
{
    int ownTeam = cg.predictedPlayerState.persistant[ PERS_TEAM ];
    int i;

    for ( i = 0; i < MAX_GENTITIES; i++ )
    {
        centity_t* mate = &cg_entities[i];
        playerEntity_t* state = &mate->pe;

        int clientNum = mate->currentState.clientNum;

        bool isTeammate = mate->currentState.eType == ET_PLAYER && clientNum >= 0 && clientNum < MAX_CLIENTS && (mate->currentState.misc & 0x00FF) == ownTeam;

        if ( !isTeammate )
        {
            continue;
        }

        //We have a fading effect for teammates going out of the PVS
        CG_MinimapUpdateTeammateFadingAndPos( mate );

        //Draw the arrow for this teammate with the right fading
        if( state->minimapFading != 0.0f )
        {
            CG_DrawMinimapObject( m->gfx.teamArrow, state->lastMinimapPos, state->lastMinimapAngle, 1.0, MINIMAP_TEAMMATE_DISPLAY_SIZE, state->minimapFading );
        }
    }
}
*/

/*
================
CG_MinimapDrawBeacon
================
*/
static void CG_MinimapDrawBeacon( const cbeacon_t *b, float size, const vec2_t center, const vec2_t *bounds )
{
	vec2_t offset, pos2d, dir;
	bool clamped;
	vec4_t color;

	size *= b->scale;

	CG_WorldToMinimap( b->origin, offset );
	pos2d[ 0 ] = - size/2 + offset[ 0 ];
	pos2d[ 1 ] = - size/2 + offset[ 1 ];

	if( pos2d[ 0 ] < bounds[ 0 ][ 0 ] ||
	    pos2d[ 0 ] > bounds[ 1 ][ 0 ] ||
	    pos2d[ 1 ] < bounds[ 0 ][ 1 ] ||
	    pos2d[ 1 ] > bounds[ 1 ][ 1 ] )
	{
		clamped = true;

		if( b->type == BCT_TAG )
			return;

		Vector2Subtract( pos2d, center, dir );
		ProjectPointOntoRectangleOutwards( pos2d, center, dir, bounds );
	}
	else
		clamped = false;

	Vector4Copy( b->color, color );
	color[ 3 ] *= cgs.bc.minimapAlpha;
	trap_R_SetColor( color );

	trap_R_DrawRotatedPic( pos2d[ 0 ], pos2d[ 1 ], size, size, 0.0, 0.0, 1.0, 1.0, CG_BeaconIcon( b ), 0.0 );
	if( b->flags & EF_BC_DYING )
		trap_R_DrawStretchPic( pos2d[ 0 ] - size/2 * 0.3,
		                       pos2d[ 1 ] - size/2 * 0.3,
		                       size * 1.3, size * 1.3,
		                       0, 0, 1, 1,
		                       cgs.media.beaconNoTarget );
	if( clamped )
		trap_R_DrawRotatedPic( pos2d[ 0 ] - size * 0.25,
		                       pos2d[ 1 ] - size * 0.25,
		                       size * 1.5, size * 1.5,
		                       0.0, 0.0, 1.0, 1.0,
		                       cgs.media.beaconIconArrow,
		                       270.0 - atan2( dir[ 1 ], dir[ 0 ] ) * 180 / M_PI );
}

/*
================
CG_MinimapDrawBeacons

Precalculates some stuff for CG_MinimapDrawBeacon and calls the
function for all cg.beacons.
================
*/
#define BEACON_MINIMAP_SIZE 64
static void CG_MinimapDrawBeacons( const rectDef_t *rect )
{
  float size;
  vec2_t bounds[ 2 ], center;
	int i;

  size = CG_WorldToMinimapScale( BEACON_MINIMAP_SIZE ) * cgs.bc.minimapScale;

  bounds[ 0 ][ 0 ] = rect->x + size * 0.25;
  bounds[ 0 ][ 1 ] = rect->y + size * 0.25;
  bounds[ 1 ][ 0 ] = rect->x + rect->w - size * 1.25;
  bounds[ 1 ][ 1 ] = rect->y + rect->h - size * 1.25;
  center[ 0 ] = rect->x + rect->w / 2.0f;
  center[ 1 ] = rect->y + rect->h / 2.0f;

	for ( i = 0; i < cg.beaconCount; i++ )
		CG_MinimapDrawBeacon( cg.beacons[ i ], size, center, (const vec2_t*)bounds );
}

//Entry points in the minimap code

/*
================
CG_InitMinimap
================
*/
void CG_InitMinimap()
{
    minimap_t* m = &cg.minimap;

    m->defined = true;

    if( !CG_ParseMinimap( m, va( "minimaps/%s.minimap", cgs.mapname ) ) )
    {
        m->defined = false;
        CG_Printf( S_WARNING "could not parse the minimap, defaulting to no minimap.\n" );
    }
    else if( m->nZones == 0 )
    {
        m->defined = false;
        CG_Printf( S_ERROR "the minimap did not define any zone.\n" );
    }

    m->gfx.playerArrow = trap_R_RegisterShader( "gfx/2d/player-arrow", RSF_DEFAULT );
    m->gfx.teamArrow = trap_R_RegisterShader( "gfx/2d/team-arrow", RSF_DEFAULT );

    CG_UpdateMinimapActive( m );
}

/*
================
CG_DrawMinimap
================
*/
void CG_DrawMinimap( const rectDef_t* rect640, const vec4_t teamColor )
{
    minimap_t* m = &cg.minimap;
    minimapZone_t *z = nullptr;
    rectDef_t rect = *rect640;

    CG_UpdateMinimapActive( m );

    if( !m->active )
    {
        return;
    }

    z = CG_ChooseMinimapZone( m );

    //Setup the transform
    CG_AdjustFrom640( &rect.x, &rect.y, &rect.w, &rect.h );
    CG_SetupMinimapTransform( &rect, m, z );

    CG_SetMinimapColor( teamColor );

    //Add the backgound
    CG_FillRect( rect640->x, rect640->y, rect640->w, rect640->h, m->bgColor );

    //Draw things inside the rectangle we were given
    CG_SetScissor( rect.x, rect.y, rect.w, rect.h );
    CG_EnableScissor( true );
    {
        CG_MinimapDrawMap( m, z );
        CG_MinimapDrawPlayer( m );
        //CG_MinimapDrawTeammates( m );
    }
    CG_EnableScissor( false );

		//(experimental) Draw beacons without the scissor
    CG_MinimapDrawBeacons( &rect );

    //Reset the color for other hud elements
    trap_R_SetColor( nullptr );
}
