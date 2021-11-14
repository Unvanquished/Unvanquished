/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
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
	polyVert_t verts[ 4 ];
	vec3_t     temp;

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
	if ( cgs.glconfig.vidWidth * 480 > cgs.glconfig.vidHeight * 640 )
	{
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
void CG_FillRect( float x, float y, float width, float height, const Color::Color& color )
{
	trap_R_SetColor( color );

	CG_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader );

	trap_R_ClearColor();
}

/*
================
CG_DrawSides

Coords are virtual 640x480
================
*/
void CG_DrawSides( float x, float y, float w, float h, float size )
{
	float sizeY;

	CG_AdjustFrom640( &x, &y, &w, &h );
	sizeY = size * cgs.screenYScale;
	size *= cgs.screenXScale;

	trap_R_DrawStretchPic( x, y + sizeY, size, h - ( sizeY * 2.0f ), 0, 0, 0, 0, cgs.media.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y + sizeY, size, h - ( sizeY * 2.0f ), 0, 0, 0, 0, cgs.media.whiteShader );
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
void CG_DrawRect( float x, float y, float width, float height, float size, const Color::Color& color )
{
	trap_R_SetColor( color );

	CG_DrawTopBottom( x, y, width, height, size );
	CG_DrawSides( x, y, width, height, size );

	trap_R_ClearColor();
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
CG_DrawRotatedPic

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawRotatedPic( float x, float y, float width, float height, qhandle_t hShader, float angle )
{
	CG_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawRotatedPic( x, y, width, height, 0, 0, 1, 1, hShader, angle );
}

/*
================
CG_DrawNoStretchPic

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawNoStretchPic( float x, float y, float width, float height, qhandle_t hShader )
{
	float ratio = cgs.glconfig.vidWidth / width;
	x *= ratio;
	y *= cgs.screenYScale;
	width *= ratio;
	height *= ratio;
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

/*
================
CG_SetClipRegion
=================
*/
void CG_SetClipRegion( float x, float y, float w, float h )
{
	vec4_t clip;

	CG_AdjustFrom640( &x, &y, &w, &h );

	clip[ 0 ] = x;
	clip[ 1 ] = y;
	clip[ 2 ] = x + w;
	clip[ 3 ] = y + h;

	trap_R_SetClipRegion( clip );
}

/*
================
CG_ClearClipRegion
=================
*/
void CG_ClearClipRegion()
{
	trap_R_ResetClipRegion();
}

/*
================
CG_EnableScissor

Enables the GL scissor test to be used for rotated images
DrawStretchPic seems to reset the scissor
=================
*/
void CG_EnableScissor( bool enable )
{
	trap_R_ScissorEnable(enable);
}

/*
================
CG_SetScissor
=================
*/
void CG_SetScissor( int x, int y, int w, int h )
{
	//Converts the Y axis
	trap_R_ScissorSet( x, cgs.glconfig.vidHeight - y - h, w, h );
}

/*
================
CG_DrawFadePic

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawFadePic( float x, float y, float width, float height, const Color::Color& fcolor,
                     const Color::Color& tcolor, float amount, qhandle_t hShader )
{

	CG_AdjustFrom640( &x, &y, &width, &height );

	Color::Color finalcolor = Color::Blend( fcolor, tcolor, amount / 100 );

	trap_R_SetColor( finalcolor );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
	trap_R_ClearColor();
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
void CG_TileClear()
{
	int top, bottom, left, right;
	int w, h;

	w = cgs.glconfig.vidWidth;
	h = cgs.glconfig.vidHeight;

	if ( cg.refdef.x == 0 && cg.refdef.y == 0 &&
	     cg.refdef.width == w && cg.refdef.height == h )
	{
		return; // full screen rendering
	}

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
Color::Color CG_FadeColor( int startMsec, int totalMsec )
{
	int           t;

	if ( startMsec == 0 )
	{
		return Color::White;
	}

	t = cg.time - startMsec;

	if ( t >= totalMsec )
	{
		return Color::White;
	}

	Color::Color color = Color::White;

	// fade out
	if ( totalMsec - t < FADE_TIME )
	{
		color.SetAlpha( ( totalMsec - t ) * 1.0 / FADE_TIME );
	}

	return color;
}

/*
=============
CG_FadeAlpha
=============
*/
float CG_FadeAlpha( int startMsec, int totalMsec )
{
	float fade = 1.0f;
	int   t = cg.time - startMsec;

	if ( startMsec == 0 )
	{
		return 0;
	}

	if ( t >= totalMsec )
	{
		return 0;
	}

	// fade out
	if ( totalMsec - t < FADE_TIME )
	{
		fade = static_cast<float>( totalMsec - t ) / FADE_TIME;
	}
	// fade in
	else if ( t < FADE_TIME )
	{
		fade = static_cast<float>(t) / FADE_TIME;
	}

	return fade;
}

/*
================
CG_WorldToScreen
================
*/
bool CG_WorldToScreen( vec3_t point, float *x, float *y )
{
	vec3_t trans;
	float  xc, yc;
	float  px, py;
	float  z;
	bool front = true;

	px = tanf( cg.refdef.fov_x * M_PI / 360.0f );
	py = tanf( cg.refdef.fov_y * M_PI / 360.0f );

	VectorSubtract( point, cg.refdef.vieworg, trans );

	xc = ( 640.0f * cg_viewsize.Get() ) / 200.0f;
	yc = ( 480.0f * cg_viewsize.Get() ) / 200.0f;

	z = DotProduct( trans, cg.refdef.viewaxis[ 0 ] );

	if ( z <= 0.001f )
	{
		front = false;
	}

	if ( x )
	{
		*x = 320.0f - DotProduct( trans, cg.refdef.viewaxis[ 1 ] ) * xc / ( z * px );
	}

	if ( y )
	{
		*y = 240.0f - DotProduct( trans, cg.refdef.viewaxis[ 2 ] ) * yc / ( z * py );
	}

	return front;
}

/*
=================
CG_GetColorCharForHealth
=================
*/
char CG_GetColorCharForHealth( int clientnum )
{
	char health_char = '2';
	int  healthPercent;
	int  maxHealth;
	int  curWeaponClass = cgs.clientinfo[ clientnum ].curWeaponClass;

	if ( cgs.clientinfo[ clientnum ].team == TEAM_ALIENS )
	{
		maxHealth = BG_Class( curWeaponClass )->health;
	}
	else
	{
		maxHealth = BG_Class( PCL_HUMAN_NAKED )->health;
	}

	healthPercent = ( int )( 100.0f * ( float ) cgs.clientinfo[ clientnum ].health /
	                         ( float ) maxHealth );

	if ( healthPercent < 33 )
	{
		health_char = '1';
	}
	else if ( healthPercent < 67 )
	{
		health_char = '3';
	}

	return health_char;
}

/*
================
CG_DrawSphere
================
*/
void CG_DrawSphere( const vec3_t center, float radius, int customShader, const Color::Color& shaderRGBA )
{
	refEntity_t re{};

	re.reType = refEntityType_t::RT_MODEL;
	re.hModel = cgs.media.sphereModel;
	re.customShader = customShader;
	re.renderfx = RF_NOSHADOW;

	re.shaderRGBA = shaderRGBA;

	VectorCopy( center, re.origin );

	radius *= 0.01f;
	VectorSet( re.axis[ 0 ], radius, 0, 0 );
	VectorSet( re.axis[ 1 ], 0, radius, 0 );
	VectorSet( re.axis[ 2 ], 0, 0, radius );
	re.nonNormalizedAxes = true;

	trap_R_AddRefEntityToScene( &re );
}

/*
================
CG_DrawSphericalCone
================
*/
void CG_DrawSphericalCone( const vec3_t tip, const vec3_t rotation, float radius,
                           bool a240, int customShader, const Color::Color& shaderRGBA )
{
	refEntity_t re{};

	re.reType = refEntityType_t::RT_MODEL;
	re.hModel = a240 ? cgs.media.sphericalCone240Model : cgs.media.sphericalCone64Model;
	re.customShader = customShader;
	re.renderfx = RF_NOSHADOW;

	re.shaderRGBA = shaderRGBA;

	VectorCopy( tip, re.origin );

	radius *= 0.01f;
	AnglesToAxis( rotation, re.axis );
	VectorScale( re.axis[ 0 ], radius, re.axis[ 0 ] );
	VectorScale( re.axis[ 1 ], radius, re.axis[ 1 ] );
	VectorScale( re.axis[ 2 ], radius, re.axis[ 2 ] );
	re.nonNormalizedAxes = true;

	trap_R_AddRefEntityToScene( &re );
}

/*
================
CG_DrawRangeMarker
================
*/
void CG_DrawRangeMarker( rangeMarker_t rmType, const vec3_t origin, float range, const vec3_t angles, Color::Color rgba )
{
	if ( cg_rangeMarkerDrawSurface.Get() )
	{
		qhandle_t pcsh;

		pcsh = cgs.media.plainColorShader;

		rgba.SetAlpha( rgba.Alpha() * cg_rangeMarkerSurfaceOpacity.Get() );

		switch( rmType )
		{
			case RM_SPHERE:
				CG_DrawSphere( origin, range, pcsh, rgba );
				break;
			case RM_SPHERICAL_CONE_64:
				CG_DrawSphericalCone( origin, angles, range, false, pcsh, rgba );
				break;
			case RM_SPHERICAL_CONE_240:
				CG_DrawSphericalCone( origin, angles, range, true, pcsh, rgba );
				break;
		}
	}

	if ( cg_rangeMarkerDrawIntersection.Get() || cg_rangeMarkerDrawFrontline.Get() )
	{
		float                       lineOpacity, lineThickness;
		const cgMediaBinaryShader_t *mbsh;
		cgBinaryShaderSetting_t     *bshs;

		if ( cg.numBinaryShadersUsed >= NUM_BINARY_SHADERS )
		{
			return;
		}

		lineOpacity = cg_rangeMarkerLineOpacity.Get();
		lineThickness = cg_rangeMarkerLineThickness.Get();
		if ( lineThickness < 0.0f )
			lineThickness = 0.0f;
		mbsh = &cgs.media.binaryShaders[ cg.numBinaryShadersUsed ];

		if ( rmType == RM_SPHERE )
		{
			if ( range > lineThickness / 2 )
			{
				if ( cg_rangeMarkerDrawIntersection.Get() )
				{
					CG_DrawSphere( origin, range - lineThickness / 2, mbsh->b1, Color::White );
				}

				CG_DrawSphere( origin, range - lineThickness / 2, mbsh->f2, Color::White );
			}

			if ( cg_rangeMarkerDrawIntersection.Get() )
			{
				CG_DrawSphere( origin, range + lineThickness / 2, mbsh->b2, Color::White );
			}

			CG_DrawSphere( origin, range + lineThickness / 2, mbsh->f1, Color::White );
		}
		else
		{
			bool t2;
			float    f, r;
			vec3_t   forward, tip;

			t2 = ( rmType == RM_SPHERICAL_CONE_240 );
			f = lineThickness * ( t2 ? 0.26f : 0.8f );
			r = f + lineThickness * ( t2 ? 0.23f : 0.43f );
			AngleVectors( angles, forward, nullptr, nullptr );

			if ( range > r )
			{
				VectorMA( origin, f, forward, tip );

				if ( cg_rangeMarkerDrawIntersection.Get() )
				{
					CG_DrawSphericalCone( tip, angles, range - r, t2, mbsh->b1, Color::White );
				}

				CG_DrawSphericalCone( tip, angles, range - r, t2, mbsh->f2, Color::White );
			}

			VectorMA( origin, -f, forward, tip );

			if ( cg_rangeMarkerDrawIntersection.Get() )
			{
				CG_DrawSphericalCone( tip, angles, range + r, t2, mbsh->b2, Color::White );
			}

			CG_DrawSphericalCone( tip, angles, range + r, t2, mbsh->f1, Color::White );
		}

		bshs = &cg.binaryShaderSettings[ cg.numBinaryShadersUsed ];

		bshs->color = rgba * lineOpacity;

		bshs->drawIntersection = cg_rangeMarkerDrawIntersection.Get();
		bshs->drawFrontline = cg_rangeMarkerDrawFrontline.Get();

		++cg.numBinaryShadersUsed;
	}
}
