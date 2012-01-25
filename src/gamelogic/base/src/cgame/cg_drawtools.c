/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

This file is part of OpenWolf.

OpenWolf is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenWolf is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// cg_drawtools.c -- helper functions called by cg_draw, cg_scoreboard, cg_info, etc


#include "cg_local.h"

/*
===============
CG_DrawPlane

Draw a quad in 3 space - basically CG_DrawPic in 3 space
===============
*/
void CG_DrawPlane( vec3_t origin, vec3_t down, vec3_t right, qhandle_t shader )
{
  polyVert_t  verts[ 4 ];
  vec3_t      temp;

  VectorCopy( origin, verts[ 0 ].xyz );
  verts[ 0 ].st[ 0 ] = 0;
  verts[ 0 ].st[ 1 ] = 0;
  verts[ 0 ].modulate[ 0 ] = 255;
  verts[ 0 ].modulate[ 1 ] = 255;
  verts[ 0 ].modulate[ 2 ] = 255;
  verts[ 0 ].modulate[ 3 ] = 255;

  VectorAdd( origin, right, temp );
  VectorCopy( temp, verts[ 1 ].xyz );
  verts[ 1 ].st[ 0 ] = 1;
  verts[ 1 ].st[ 1 ] = 0;
  verts[ 1 ].modulate[ 0 ] = 255;
  verts[ 1 ].modulate[ 1 ] = 255;
  verts[ 1 ].modulate[ 2 ] = 255;
  verts[ 1 ].modulate[ 3 ] = 255;

  VectorAdd( origin, right, temp );
  VectorAdd( temp, down, temp );
  VectorCopy( temp, verts[ 2 ].xyz );
  verts[ 2 ].st[ 0 ] = 1;
  verts[ 2 ].st[ 1 ] = 1;
  verts[ 2 ].modulate[ 0 ] = 255;
  verts[ 2 ].modulate[ 1 ] = 255;
  verts[ 2 ].modulate[ 2 ] = 255;
  verts[ 2 ].modulate[ 3 ] = 255;

  VectorAdd( origin, down, temp );
  VectorCopy( temp, verts[ 3 ].xyz );
  verts[ 3 ].st[ 0 ] = 0;
  verts[ 3 ].st[ 1 ] = 1;
  verts[ 3 ].modulate[ 0 ] = 255;
  verts[ 3 ].modulate[ 1 ] = 255;
  verts[ 3 ].modulate[ 2 ] = 255;
  verts[ 3 ].modulate[ 3 ] = 255;

  trap_R_AddPolyToScene( shader, 4, verts );
}

/*
================
CG_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void CG_AdjustFrom640( float *x, float *y, float *w, float *h )
{
#if 0
  // adjust for wide screens
  if ( cgs.glconfig.vidWidth * 480 > cgs.glconfig.vidHeight * 640 ) {
    *x += 0.5 * ( cgs.glconfig.vidWidth - ( cgs.glconfig.vidHeight * 640 / 480 ) );
  }
#endif
  // scale for screen sizes
  *x *= cgs.screenXScale;
  *y *= cgs.screenYScale;
  *w *= cgs.screenXScale;
  *h *= cgs.screenYScale;
}

/*
================
CG_FillRect

Coordinates are 640*480 virtual values
=================
*/
void CG_FillRect( float x, float y, float width, float height, const float *color )
{
  trap_R_SetColor( color );

  CG_AdjustFrom640( &x, &y, &width, &height );
  trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader );

  trap_R_SetColor( NULL );
}


/*
================
CG_DrawSides

Coords are virtual 640x480
================
*/
void CG_DrawSides( float x, float y, float w, float h, float size )
{
  CG_AdjustFrom640( &x, &y, &w, &h );
  size *= cgs.screenXScale;
  trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
  trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
}

void CG_DrawTopBottom( float x, float y, float w, float h, float size )
{
  CG_AdjustFrom640( &x, &y, &w, &h );
  size *= cgs.screenYScale;
  trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
  trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
}


/*
================
CG_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color )
{
  trap_R_SetColor( color );

  CG_DrawTopBottom( x, y, width, height, size );
  CG_DrawSides( x, y, width, height, size );

  trap_R_SetColor( NULL );
}


/*
================
CG_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader )
{
  CG_AdjustFrom640( &x, &y, &width, &height );
  trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}



/*
================
CG_DrawFadePic

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawFadePic( float x, float y, float width, float height, vec4_t fcolor,
                     vec4_t tcolor, float amount, qhandle_t hShader )
{
  vec4_t  finalcolor;
  float   inverse;

  inverse = 100 - amount;

  CG_AdjustFrom640( &x, &y, &width, &height );

  finalcolor[ 0 ] = ( ( inverse * fcolor[ 0 ] ) + ( amount * tcolor[ 0 ] ) ) / 100;
  finalcolor[ 1 ] = ( ( inverse * fcolor[ 1 ] ) + ( amount * tcolor[ 1 ] ) ) / 100;
  finalcolor[ 2 ] = ( ( inverse * fcolor[ 2 ] ) + ( amount * tcolor[ 2 ] ) ) / 100;
  finalcolor[ 3 ] = ( ( inverse * fcolor[ 3 ] ) + ( amount * tcolor[ 3 ] ) ) / 100;

  trap_R_SetColor( finalcolor );
  trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
  trap_R_SetColor( NULL );
}

/*
=================
CG_DrawStrlen

Returns character count, skiping color escape codes
=================
*/
int CG_DrawStrlen( const char *str )
{
  const char  *s = str;
  int         count = 0;

  while( *s )
  {
    if( Q_IsColorString( s ) )
      s += 2;
    else
    {
      count++;
      s++;
    }
  }

  return count;
}

/*
=============
CG_TileClearBox

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
static void CG_TileClearBox( int x, int y, int w, int h, qhandle_t hShader )
{
  float s1, t1, s2, t2;

  s1 = x / 64.0;
  t1 = y / 64.0;
  s2 = ( x + w ) / 64.0;
  t2 = ( y + h ) / 64.0;
  trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}



/*
==============
CG_TileClear

Clear around a sized down screen
==============
*/
void CG_TileClear( void )
{
  int   top, bottom, left, right;
  int   w, h;

  w = cgs.glconfig.vidWidth;
  h = cgs.glconfig.vidHeight;

  if( cg.refdef.x == 0 && cg.refdef.y == 0 &&
      cg.refdef.width == w && cg.refdef.height == h )
    return;   // full screen rendering

  top = cg.refdef.y;
  bottom = top + cg.refdef.height - 1;
  left = cg.refdef.x;
  right = left + cg.refdef.width - 1;

  // clear above view screen
  CG_TileClearBox( 0, 0, w, top, cgs.media.backTileShader );

  // clear below view screen
  CG_TileClearBox( 0, bottom, w, h - bottom, cgs.media.backTileShader );

  // clear left of view screen
  CG_TileClearBox( 0, top, left, bottom - top + 1, cgs.media.backTileShader );

  // clear right of view screen
  CG_TileClearBox( right, top, w - right, bottom - top + 1, cgs.media.backTileShader );
}



/*
================
CG_FadeColor
================
*/
float *CG_FadeColor( int startMsec, int totalMsec )
{
  static vec4_t   color;
  int     t;

  if( startMsec == 0 )
    return NULL;

  t = cg.time - startMsec;

  if( t >= totalMsec )
    return NULL;

  // fade out
  if( totalMsec - t < FADE_TIME )
    color[ 3 ] = ( totalMsec - t ) * 1.0 / FADE_TIME;
  else
    color[ 3 ] = 1.0;

  color[ 0 ] = color[ 1 ] = color[ 2 ] = 1;

  return color;
}
