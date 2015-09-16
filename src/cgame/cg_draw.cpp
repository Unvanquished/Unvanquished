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

// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

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
	float charWidth, charHeight;

	if ( !( charWidth = cw ) )
	{
		charWidth = CHAR_WIDTH;
	}

	if ( !( charHeight = ch ) )
	{
		charHeight = CHAR_HEIGHT;
	}

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

void CG_MouseEvent( int x, int y )
{
	if ( rocketInfo.keyCatcher & KEYCATCH_UI)
	{
		Rocket_MouseMove( x, y );
	}
	else if ( ( cg.predictedPlayerState.pm_type == PM_NORMAL ||
	       cg.predictedPlayerState.pm_type == PM_SPECTATOR ) &&
	     cg.showScores == false )
	{
		CG_SetKeyCatcher( 0 );
		return;
	}


}

void CG_KeyEvent( int key, bool down )
{
	if ( rocketInfo.keyCatcher & KEYCATCH_UI )
	{
		Rocket_ProcessKeyInput( key, down);
	}
	else if ( cg.predictedPlayerState.pm_type == PM_NORMAL ||
	     ( cg.predictedPlayerState.pm_type == PM_SPECTATOR &&
	       cg.showScores == false ) )
	{
		CG_SetKeyCatcher( 0 );
	}
}

int CG_ClientNumFromName( const char *p )
{
	int i;

	for ( i = 0; i < cgs.maxclients; i++ )
	{
		if ( cgs.clientinfo[ i ].infoValid &&
		     Q_stricmp( cgs.clientinfo[ i ].name, p ) == 0 )
		{
			return i;
		}
	}

	return -1;
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
void CG_CenterPrint( const char *str, int, int )
{
	Q_strncpyz( cg.centerPrint, str, sizeof( cg.centerPrint ) );
	cg.centerPrintTime = cg.time;
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
	vec4_t color;

	// Don't draw clamped beacons for tags, except for enemy players.
	if( b->type == BCT_TAG && b->clamped && !( ( b->flags & EF_BC_ENEMY ) &&
	                                           ( b->flags & EF_BC_TAG_PLAYER ) ) )
		return;

	Vector4Copy( b->color, color );

	if( !( BG_Beacon( b->type )->flags & BCF_IMPORTANT ) )
		color[ 3 ] *= cgs.bc.hudAlpha;
	else
		color[ 3 ] *= cgs.bc.hudAlphaImportant;

	trap_R_SetColor( color );

	trap_R_DrawStretchPic( b->pos[ 0 ] - b->size/2,
	                       b->pos[ 1 ] - b->size/2,
	                       b->size, b->size,
	                       0, 0, 1, 1,
	                       CG_BeaconIcon( b ) );

	if( b->flags & EF_BC_DYING )
		trap_R_DrawStretchPic( b->pos[ 0 ] - b->size/2 * 1.3,
		                       b->pos[ 1 ] - b->size/2 * 1.3,
		                       b->size * 1.3, b->size * 1.3,
		                       0, 0, 1, 1,
		                       cgs.media.beaconNoTarget );

	if ( b->clamped )
		trap_R_DrawRotatedPic( b->pos[ 0 ] - b->size/2 * 1.5,
		                       b->pos[ 1 ] - b->size/2 * 1.5,
		                       b->size * 1.5, b->size * 1.5,
		                       0, 0, 1, 1,
		                       cgs.media.beaconIconArrow,
		                       270.0 - ( angle = atan2( b->clamp_dir[ 1 ], b->clamp_dir[ 0 ] ) ) * 180 / M_PI );

	if( b->type == BCT_TIMER )
	{
		int num;

		num = ( BEACON_TIMER_TIME + b->ctime - cg.time ) / 100;

		if( num > 0 )
		{
			float h, tw;
			const char *p;
			vec2_t pos, dir, rect[ 2 ];
			int i, l, frame;

			h = b->size * 0.4;
			p = va( "%d", num );
			l = strlen( p );
			tw = h * l;

			if( !b->clamped )
			{
				pos[ 0 ] = b->pos[ 0 ];
				pos[ 1 ] = b->pos[ 1 ] + b->size/2 + h/2;
			}
			else
			{
				rect[ 0 ][ 0 ] = b->pos[ 0 ] - b->size/2 - tw/2;
				rect[ 1 ][ 0 ] = b->pos[ 0 ] + b->size/2 + tw/2;
				rect[ 0 ][ 1 ] = b->pos[ 1 ] - b->size/2 - h/2;
				rect[ 1 ][ 1 ] = b->pos[ 1 ] + b->size/2 + h/2;

				for( i = 0; i < 2; i++ )
					dir[ i ] = - b->clamp_dir[ i ];

				ProjectPointOntoRectangleOutwards( pos, b->pos, dir, (const vec2_t*)rect );
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

	trap_R_SetColor( nullptr );
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

	if ( cg_draw2D.integer == 0 )
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

	if ( cg.zoomed )
	{
		vec4_t black = { 0.0f, 0.0f, 0.0f, 0.5f };
		trap_R_DrawStretchPic( ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ), 0, cgs.glconfig.vidHeight, cgs.glconfig.vidHeight, 0, 0, 1, 1, cgs.media.scopeShader );
		trap_R_SetColor( black );
		trap_R_DrawStretchPic( 0, 0, ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ), cgs.glconfig.vidHeight, 0, 0, 1, 1, cgs.media.whiteShader );
		trap_R_DrawStretchPic( cgs.glconfig.vidWidth - ( ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ) ), 0, ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ), cgs.glconfig.vidHeight, 0, 0, 1, 1, cgs.media.whiteShader );
		trap_R_SetColor( nullptr );
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

	*s1 *= cg_painBlendZoom.value;
	*t1 *= cg_painBlendZoom.value;
	*s2 *= cg_painBlendZoom.value;
	*t2 *= cg_painBlendZoom.value;

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
	vec4_t    color;
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

	damageAsFracOfMax = ( float ) damage / cg.snap->ps.stats[ STAT_MAX_HEALTH ];
	cg.lastHealth = cg.snap->ps.stats[ STAT_HEALTH ];

	cg.painBlendValue += damageAsFracOfMax * cg_painBlendScale.value;

	if ( cg.painBlendValue > 0.0f )
	{
		cg.painBlendValue -= ( cg.frametime / 1000.0f ) *
		                     cg_painBlendDownRate.value;
	}

	if ( cg.painBlendValue > 1.0f )
	{
		cg.painBlendValue = 1.0f;
	}
	else if ( cg.painBlendValue <= 0.0f )
	{
		cg.painBlendValue = 0.0f;
		return;
	}

	if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_ALIENS )
	{
		VectorSet( color, 0.43f, 0.8f, 0.37f );
	}
	else if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_HUMANS )
	{
		VectorSet( color, 0.8f, 0.0f, 0.0f );
	}

	if ( cg.painBlendValue > cg.painBlendTarget )
	{
		cg.painBlendTarget += ( cg.frametime / 1000.0f ) *
		                      cg_painBlendUpRate.value;
	}
	else if ( cg.painBlendValue < cg.painBlendTarget )
	{
		cg.painBlendTarget = cg.painBlendValue;
	}

	if ( cg.painBlendTarget > cg_painBlendMax.value )
	{
		cg.painBlendTarget = cg_painBlendMax.value;
	}

	color[ 3 ] = cg.painBlendTarget;

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

	trap_R_SetColor( nullptr );
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
	int        i, j, k;

	if ( !cg.numBinaryShadersUsed )
	{
		return;
	}

	ss = cg_binaryShaderScreenScale.value;

	if ( ss <= 0.0f )
	{
		cg.numBinaryShadersUsed = 0;
		return;
	}
	else if ( ss > 1.0f )
	{
		ss = 1.0f;
	}

	ss = sqrt( ss );

	trap_Cvar_VariableStringBuffer( "r_znear", str, sizeof( str ) );
	f = atof( str ) + 0.01;
	l = f * tan( DEG2RAD( cg.refdef.fov_x / 2 ) ) * ss;
	u = f * tan( DEG2RAD( cg.refdef.fov_y / 2 ) ) * ss;

	VectorMA( cg.refdef.vieworg, f, cg.refdef.viewaxis[ 0 ], verts[ 0 ].xyz );
	VectorMA( verts[ 0 ].xyz, l, cg.refdef.viewaxis[ 1 ], verts[ 0 ].xyz );
	VectorMA( verts[ 0 ].xyz, u, cg.refdef.viewaxis[ 2 ], verts[ 0 ].xyz );
	VectorMA( verts[ 0 ].xyz, -2 * l, cg.refdef.viewaxis[ 1 ], verts[ 1 ].xyz );
	VectorMA( verts[ 1 ].xyz, -2 * u, cg.refdef.viewaxis[ 2 ], verts[ 2 ].xyz );
	VectorMA( verts[ 0 ].xyz, -2 * u, cg.refdef.viewaxis[ 2 ], verts[ 3 ].xyz );

	trap_R_AddPolyToScene( cgs.media.binaryAlpha1Shader, 4, verts );

	for ( i = 0; i < cg.numBinaryShadersUsed; ++i )
	{
		for ( j = 0; j < 4; ++j )
		{
			for ( k = 0; k < 3; ++k )
			{
				verts[ j ].modulate[ k ] = cg.binaryShaderSettings[ i ].color[ k ];
			}
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
