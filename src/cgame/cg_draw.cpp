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

// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "common/Common.h"
#include "cg_local.h"
#include "rocket/rocket.h"

// Moves `point` to (if !allowInterior) an edge of the rectangle, or (if allowInterior) to
// any point on/inside the rectangle. Returns whether point was clamped.
//
// If `allowInterior` is false OR `point` is outside the bounds, set `point` to the intersection
// of the bounding rectangle and the ray from `center` toward `point`. Otherwise, return false
// and leave `point` unchanged.
// `center` must be inside the bounds but doesn't have to be the true center.
bool CG_ClampToRectangleAlongLine(
	const glm::vec2 &mins, const glm::vec2 &maxs, const glm::vec2 &center, bool allowInterior, glm::vec2 &point )
{
	ASSERT( center.x >= mins.x && center.x <= maxs.x );
	ASSERT( center.y >= mins.y && center.y <= maxs.y );

	glm::vec2 dir = point - center;
	float fraction = std::numeric_limits<float>::max();

	if ( dir.x > 0 )
	{
		fraction = ( maxs.x - center.x ) / dir.x;
	}
	else if ( dir.x < 0 )
	{
		fraction = ( mins.x - center.x ) / dir.x;
	}

	if ( dir.y > 0 )
	{
		fraction = std::min( fraction, ( maxs.y - center.y ) / dir.y );
	}
	else if ( dir.y < 0 )
	{
		fraction = std::min( fraction, ( mins.y - center.y ) / dir.y );
	}

	ASSERT_GT( fraction, 0.0f );

	if ( allowInterior )
	{
		if ( fraction >= 1.0f )
		{
			return false;
		}
	}
	else
	{
		if ( fraction > 1.0e6f )
		{
			// ill-conditioned, return an arbitrary boundary point
			point.y = maxs.y;
			return true;
		}
	}

	point = center + dir * fraction;
	return true;
}

/*
==============
CG_DrawField

Draws large numbers for status bar
==============
*/
void CG_DrawField( float x, float y, int width, float cw, float ch, int value )
{
	char  num[ 16 ], *ptr;
	int   l;
	int   frame;
	float charWidth = (cw != 0.0f ? cw : CGAME_CHAR_WIDTH);
	float charHeight = (ch != 0.0f ? ch : CGAME_CHAR_HEIGHT);

	if ( width < 1 )
	{
		return;
	}

	// draw number string
	if ( width > 4 )
	{
		width = 4;
	}

	switch ( width )
	{
		case 1:
			value = value > 9 ? 9 : value;
			value = value < 0 ? 0 : value;
			break;

		case 2:
			value = value > 99 ? 99 : value;
			value = value < -9 ? -9 : value;
			break;

		case 3:
			value = value > 999 ? 999 : value;
			value = value < -99 ? -99 : value;
			break;

		case 4:
			value = value > 9999 ? 9999 : value;
			value = value < -999 ? -999 : value;
			break;
	}

	Com_sprintf( num, sizeof( num ), "%d", value );
	l = strlen( num );

	if ( l > width )
	{
		l = width;
	}

	x += ( 2.0f * cgs.aspectScale ) + charWidth * ( width - l );

	ptr = num;

	while ( *ptr && l )
	{
		if ( *ptr == '-' )
		{
			frame = STAT_MINUS;
		}
		else
		{
			frame = *ptr - '0';
		}

		CG_DrawPic( x, y, charWidth, charHeight, cgs.media.numberShaders[ frame ] );
		x += charWidth;
		ptr++;
		l--;
	}
}

void CG_MouseEvent( int /*dx*/, int /*dy*/ )
{
}

void CG_MousePosEvent( int x, int y )
{
	rocketInfo.cursor_pos.x = x;
	rocketInfo.cursor_pos.y = y;
	if ( rocketInfo.keyCatcher & KEYCATCH_UI_MOUSE )
	{
		Rocket_MouseMove( x, y );
	}
}

bool CG_KeyEvent( Keyboard::Key key, bool down )
{
	if ( down && key == Keyboard::Key(K_ESCAPE) && !CG_AnyMenuOpen() )
	{
		// Open the main menu if no menus are open
		trap_SendConsoleCommand( "toggleMenu" );

		return true;
	}

	return Rocket_ProcessKeyInput( key, down );
}

void CG_RunMenuScript( char **args )
{
	Q_UNUSED(args);
}

/*

CENTER PRINTING

===============================================================================
*/

/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, float sizeFactor )
{
	if ( !*str )
	{
		return;
	}

	Q_strncpyz( cg.centerPrint, str, sizeof( cg.centerPrint ) );
	cg.centerPrintTime = cg.time;
	cg.centerPrintSizeFactor = sizeFactor;
}

//==============================================================================

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission()
{
// TODO
}

/*
==================
CG_DrawBeacon

Draw a beacon on the HUD
==================
*/

static void CG_DrawBeacon( cbeacon_t *b )
{
	float angle;

	// Don't draw clamped beacons for tags, except for enemy players.
	if( b->type == BCT_TAG && b->clamped && !( ( b->flags & EF_BC_ENEMY ) &&
	                                           ( b->flags & EF_BC_TAG_PLAYER ) ) )
		return;

	Color::Color color = b->color;

	if( !( BG_Beacon( b->type )->flags & BCF_IMPORTANT ) )
		color.SetAlpha( color.Alpha() * cgs.bc.hudAlpha );
	else
		color.SetAlpha( color.Alpha() * cgs.bc.hudAlphaImportant );

	trap_R_SetColor( color );

	trap_R_DrawStretchPic( b->pos[ 0 ] - b->size/2,
	                       b->pos[ 1 ] - b->size/2,
	                       b->size, b->size,
	                       0, 0, 1, 1,
	                       CG_BeaconIcon( b ) );

	if( b->flags & EF_BC_DYING )
		trap_R_DrawStretchPic( b->pos[ 0 ] - b->size/2 * 1.3f,
		                       b->pos[ 1 ] - b->size/2 * 1.3f,
		                       b->size * 1.3f, b->size * 1.3f,
		                       0, 0, 1, 1,
		                       cgs.media.beaconNoTarget );

	if ( b->clamped )
		trap_R_DrawRotatedPic( b->pos[ 0 ] - b->size/2 * 1.5f,
		                       b->pos[ 1 ] - b->size/2 * 1.5f,
		                       b->size * 1.5f, b->size * 1.5f,
		                       0, 0, 1, 1,
		                       cgs.media.beaconIconArrow,
		                       270.0f - ( angle = atan2f( b->clamp_dir[ 1 ], b->clamp_dir[ 0 ] ) ) * 180 / M_PI );

	if( b->type == BCT_TIMER )
	{
		int num;

		num = ( BEACON_TIMER_TIME + b->ctime - cg.time ) / 100;

		if( num > 0 )
		{
			float h, tw;
			const char *p;
			int i, l, frame;

			h = b->size * 0.4f;
			p = va( "%d", num );
			l = strlen( p );
			tw = h * l;

			glm::vec2 pos;

			if( !b->clamped )
			{
				pos[ 0 ] = b->pos[ 0 ];
				pos[ 1 ] = b->pos[ 1 ] + b->size/2 + h/2;
			}
			else
			{
				glm::vec2 pos0 = VEC2GLM2( b->pos );
				glm::vec2 extent(0.5f * ( b->size + tw ), 0.5f * ( b->size + h ));
				glm::vec2 rectMins = pos0 - extent;
				glm::vec2 rectMaxs = pos0 + extent;
				pos = VEC2GLM2( cgs.bc.hudCenter );
				CG_ClampToRectangleAlongLine( rectMins, rectMaxs, pos0, false, pos );
			}

			pos[ 0 ] -= tw/2;
			pos[ 1 ] -= h/2;

			for( i = 0; i < l; i++ )
			{
				if( p[ i ] >= '0' && p[ i ] <= '9' )
					frame = p[ i ] - '0';
				else if( p[ i ] == '-' )
					frame = STAT_MINUS;
				else
					frame = -1;

				if( frame != -1 )
					trap_R_DrawStretchPic( pos[ 0 ], pos[ 1 ], h, h, 0, 0, 1, 1, cgs.media.numberShaders[ frame ] );

				pos[ 0 ] += h;
			}
		}
	}

	trap_R_ClearColor();
}

//==================================================================================

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D()
{
	int i;

	if ( !cg_draw2D.Get() )
	{
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION )
	{
		CG_DrawIntermission();
		return;
	}

	if ( cg.snap->ps.persistant[ PERS_SPECSTATE ] == SPECTATOR_NOT &&
	     cg.snap->ps.stats[ STAT_HEALTH ] > 0 && !cg.zoomed )
	{
		CG_DrawBuildableStatus();
	}

	// get an up-to-date list of beacons
	CG_RunBeacons();

	// draw beacons on HUD
	for( i = 0; i < cg.beaconCount; i++ )
		CG_DrawBeacon( cg.beacons[ i ] );

	CombatFeedback::DrawDamageIndicators();

	if ( cg.zoomed )
	{
		Color::Color black = { 0.f, 0.f, 0.f, 0.5f };
		qhandle_t shader;
		switch ( cg.predictedPlayerState.weapon )
		{
			case WP_MASS_DRIVER:
				shader = cgs.media.sniperScopeShader;
				break;
			case WP_LAS_GUN:
				shader = cgs.media.lgunScopeShader;
				break;
			default:
				Log::Warn( "No shader for gun: %s",
				           BG_Weapon( cg.predictedPlayerState.weapon )->name );
				return;
		}
		trap_R_DrawStretchPic( ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ), 0,
		                       cgs.glconfig.vidHeight, cgs.glconfig.vidHeight, 0, 0, 1, 1, shader );
		trap_R_SetColor( black );
		trap_R_DrawStretchPic( 0, 0, ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ), cgs.glconfig.vidHeight, 0, 0, 1, 1, cgs.media.whiteShader );
		trap_R_DrawStretchPic( cgs.glconfig.vidWidth - ( ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ) ), 0, ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ), cgs.glconfig.vidHeight, 0, 0, 1, 1, cgs.media.whiteShader );
		trap_R_ClearColor();
	}
}

/*
===============
CG_ScalePainBlendTCs
===============
*/
static void CG_ScalePainBlendTCs( float *s1, float *t1, float *s2, float *t2 )
{
	*s1 -= 0.5f;
	*t1 -= 0.5f;
	*s2 -= 0.5f;
	*t2 -= 0.5f;

	*s1 *= cg_painBlendZoom.Get();
	*t1 *= cg_painBlendZoom.Get();
	*s2 *= cg_painBlendZoom.Get();
	*t2 *= cg_painBlendZoom.Get();

	*s1 += 0.5f;
	*t1 += 0.5f;
	*s2 += 0.5f;
	*t2 += 0.5f;
}

#define PAINBLEND_BORDER 0.15f

/*
===============
CG_PainBlend
===============
*/
static void CG_PainBlend()
{
	int       damage;
	float     damageAsFracOfMax;
	qhandle_t shader = cgs.media.viewBloodShader;
	float     x, y, w, h;
	float     s1, t1, s2, t2;

	if ( cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT || cg.intermissionStarted )
	{
		return;
	}

	damage = cg.lastHealth - cg.snap->ps.stats[ STAT_HEALTH ];

	if ( damage < 0 )
	{
		damage = 0;
	}
	damageAsFracOfMax = static_cast<float>( damage ) / BG_Class( cg.snap->ps.stats[ STAT_CLASS ] )->health;
	cg.lastHealth = cg.snap->ps.stats[ STAT_HEALTH ];

	cg.painBlendValue += damageAsFracOfMax * cg_painBlendScale.Get();

	if ( cg.painBlendValue > 0.0f )
	{
		cg.painBlendValue -= ( cg.frametime / 1000.0f ) *
		                     cg_painBlendDownRate.Get();
	}

	cg.painBlendValue = Math::Clamp( cg.painBlendValue, 0.0f, 1.0f );

	// no work to do
	if ( cg.painBlendValue == 0.0f )
	{
		return;
	}

	Color::Color color;
	if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_ALIENS )
	{
		color = { 0.43f, 0.8f, 0.37f };
	}
	else if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_HUMANS )
	{
		color = { 0.8f, 0.0f, 0.0f };
	}

	if ( cg.painBlendValue > cg.painBlendTarget )
	{
		cg.painBlendTarget += ( cg.frametime / 1000.0f ) *
		                      cg_painBlendUpRate.Get();
	}
	else if ( cg.painBlendValue < cg.painBlendTarget )
	{
		cg.painBlendTarget = cg.painBlendValue;
	}

	if ( cg.painBlendTarget > cg_painBlendMax.Get() )
	{
		cg.painBlendTarget = cg_painBlendMax.Get();
	}

	color.SetAlpha( cg.painBlendTarget );

	trap_R_SetColor( color );

	//left
	x = 0.0f;
	y = 0.0f;
	w = PAINBLEND_BORDER * 640.0f;
	h = 480.0f;
	CG_AdjustFrom640( &x, &y, &w, &h );
	s1 = 0.0f;
	t1 = 0.0f;
	s2 = PAINBLEND_BORDER;
	t2 = 1.0f;
	CG_ScalePainBlendTCs( &s1, &t1, &s2, &t2 );
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, shader );

	//right
	x = 640.0f - ( PAINBLEND_BORDER * 640.0f );
	y = 0.0f;
	w = PAINBLEND_BORDER * 640.0f;
	h = 480.0f;
	CG_AdjustFrom640( &x, &y, &w, &h );
	s1 = 1.0f - PAINBLEND_BORDER;
	t1 = 0.0f;
	s2 = 1.0f;
	t2 = 1.0f;
	CG_ScalePainBlendTCs( &s1, &t1, &s2, &t2 );
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, shader );

	//top
	x = PAINBLEND_BORDER * 640.0f;
	y = 0.0f;
	w = 640.0f - ( 2 * PAINBLEND_BORDER * 640.0f );
	h = PAINBLEND_BORDER * 480.0f;
	CG_AdjustFrom640( &x, &y, &w, &h );
	s1 = PAINBLEND_BORDER;
	t1 = 0.0f;
	s2 = 1.0f - PAINBLEND_BORDER;
	t2 = PAINBLEND_BORDER;
	CG_ScalePainBlendTCs( &s1, &t1, &s2, &t2 );
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, shader );

	//bottom
	x = PAINBLEND_BORDER * 640.0f;
	y = 480.0f - ( PAINBLEND_BORDER * 480.0f );
	w = 640.0f - ( 2 * PAINBLEND_BORDER * 640.0f );
	h = PAINBLEND_BORDER * 480.0f;
	CG_AdjustFrom640( &x, &y, &w, &h );
	s1 = PAINBLEND_BORDER;
	t1 = 1.0f - PAINBLEND_BORDER;
	s2 = 1.0f - PAINBLEND_BORDER;
	t2 = 1.0f;
	CG_ScalePainBlendTCs( &s1, &t1, &s2, &t2 );
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, shader );

	trap_R_ClearColor();
}

/*
=====================
CG_ResetPainBlend
=====================
*/
void CG_ResetPainBlend()
{
	cg.painBlendValue = 0.0f;
	cg.painBlendTarget = 0.0f;
	cg.lastHealth = cg.snap->ps.stats[ STAT_HEALTH ];
}

/*
================
CG_DrawBinaryShadersFinalPhases
================
*/
static void CG_DrawBinaryShadersFinalPhases()
{
	float      ss;
	char       str[ 20 ];
	float      f, l, u;
	polyVert_t verts[ 4 ] =
	{
		{ { 0, 0, 0 }, { 0, 0 }, { 255, 255, 255, 255 } },
		{ { 0, 0, 0 }, { 1, 0 }, { 255, 255, 255, 255 } },
		{ { 0, 0, 0 }, { 1, 1 }, { 255, 255, 255, 255 } },
		{ { 0, 0, 0 }, { 0, 1 }, { 255, 255, 255, 255 } }
	};

	if ( !cg.numBinaryShadersUsed )
	{
		return;
	}

	ss = cg_binaryShaderScreenScale.Get();

	// no work to do
	if ( ss == 0.0f )
	{
		cg.numBinaryShadersUsed = 0;
		return;
	}

	ss = sqrtf( ss );

	trap_Cvar_VariableStringBuffer( "r_znear", str, sizeof( str ) );
	f = atof( str ) + 0.01;
	l = f * tanf( DEG2RAD( cg.refdef.fov_x / 2 ) ) * ss;
	u = f * tanf( DEG2RAD( cg.refdef.fov_y / 2 ) ) * ss;

	VectorMA( cg.refdef.vieworg, f, cg.refdef.viewaxis[ 0 ], verts[ 0 ].xyz );
	VectorMA( verts[ 0 ].xyz, l, cg.refdef.viewaxis[ 1 ], verts[ 0 ].xyz );
	VectorMA( verts[ 0 ].xyz, u, cg.refdef.viewaxis[ 2 ], verts[ 0 ].xyz );
	VectorMA( verts[ 0 ].xyz, -2 * l, cg.refdef.viewaxis[ 1 ], verts[ 1 ].xyz );
	VectorMA( verts[ 1 ].xyz, -2 * u, cg.refdef.viewaxis[ 2 ], verts[ 2 ].xyz );
	VectorMA( verts[ 0 ].xyz, -2 * u, cg.refdef.viewaxis[ 2 ], verts[ 3 ].xyz );

	trap_R_AddPolyToScene( cgs.media.binaryAlpha1Shader, 4, verts );

	for ( int i = 0; i < cg.numBinaryShadersUsed; ++i )
	{
		for ( int j = 0; j < 4; ++j )
		{
			auto alpha = verts[ j ].modulate[ 3 ];
			cg.binaryShaderSettings[ i ].color.ToArray( verts[ j ].modulate );
			verts[ j ].modulate[ 3 ] = alpha;
		}

		if ( cg.binaryShaderSettings[ i ].drawFrontline )
		{
			trap_R_AddPolyToScene( cgs.media.binaryShaders[ i ].f3, 4, verts );
		}

		if ( cg.binaryShaderSettings[ i ].drawIntersection )
		{
			trap_R_AddPolyToScene( cgs.media.binaryShaders[ i ].b3, 4, verts );
		}
	}

	cg.numBinaryShadersUsed = 0;
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive()
{
	// optionally draw the info screen instead
	if ( !cg.snap )
	{
		return;
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	CG_DrawBinaryShadersFinalPhases();

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// first person blend blobs, done after AnglesToAxis
	if ( !cg.renderingThirdPerson )
	{
		CG_PainBlend();
	}

	// draw status bar and other floating elements
	CG_Draw2D();
}
