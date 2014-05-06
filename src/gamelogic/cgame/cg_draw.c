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
#include "../ui/ui_shared.h"

menuDef_t *menuScoreboard = NULL;

void CG_AlignText( rectDef_t *rect, const char *text, float scale,
                   float w, float h,
                   int align, int valign,
                   float *x, float *y )
{
	float tx, ty;

	if ( scale > 0.0f )
	{
		w = UI_Text_Width( text, scale );
		h = UI_Text_Height( text, scale );
	}

	switch ( align )
	{
		default:
		case ALIGN_LEFT:
			tx = 0.0f;
			break;

		case ALIGN_RIGHT:
			tx = rect->w - w;
			break;

		case ALIGN_CENTER:
			tx = ( rect->w - w ) / 2.0f;
			break;

		case ALIGN_NONE:
			tx = 0;
			break;
	}

	switch ( valign )
	{
		default:
		case VALIGN_BOTTOM:
			ty = rect->h;
			break;

		case VALIGN_TOP:
			ty = h;
			break;

		case VALIGN_CENTER:
			ty = h + ( ( rect->h - h ) / 2.0f );
			break;

		case VALIGN_NONE:
			ty = 0;
			break;
	}

	if ( x )
	{
		*x = rect->x + tx;
	}

	if ( y )
	{
		*y = rect->y + ty;
	}
}

/*
==============
CG_DrawFieldPadded

Draws large numbers for status bar
==============
*/
static void CG_DrawFieldPadded( int x, int y, int width, int cw, int ch, int value )
{
	char num[ 16 ], *ptr;
	int  l, orgL;
	int  frame;
	int  charWidth, charHeight;

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

	orgL = l;

	x += ( 2.0f * cgDC.aspectScale );

	ptr = num;

	while ( *ptr && l )
	{
		if ( width > orgL )
		{
			CG_DrawPic( x, y, charWidth, charHeight, cgs.media.numberShaders[ 0 ] );
			width--;
			x += charWidth;
			continue;
		}

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

	x += ( 2.0f * cgDC.aspectScale ) + charWidth * ( width - l );

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

static void CG_DrawProgressBar( rectDef_t *rect, vec4_t color, float scale,
                                int align, int textalign, int textStyle,
                                float borderSize, float progress )
{
	float rimWidth;
	float doneWidth, leftWidth;
	float tx, ty;
	char  textBuffer[ 8 ];

	if ( borderSize >= 0.0f )
	{
		rimWidth = borderSize;
	}
	else
	{
		rimWidth = rect->h / 20.0f;

		if ( rimWidth < 0.6f )
		{
			rimWidth = 0.6f;
		}
	}

	if ( progress < 0.0f )
	{
		progress = 0.0f;
	}
	else if ( progress > 1.0f )
	{
		progress = 1.0f;
	}

	doneWidth = ( rect->w - 2 * rimWidth ) * progress;
	leftWidth = ( rect->w - 2 * rimWidth ) - doneWidth;

	trap_R_SetColor( color );

	//draw rim and bar
	if ( align == ALIGN_RIGHT )
	{
		CG_DrawPic( rect->x, rect->y, rimWidth, rect->h, cgs.media.whiteShader );
		CG_DrawPic( rect->x + rimWidth, rect->y,
		            leftWidth, rimWidth, cgs.media.whiteShader );
		CG_DrawPic( rect->x + rimWidth, rect->y + rect->h - rimWidth,
		            leftWidth, rimWidth, cgs.media.whiteShader );
		CG_DrawPic( rect->x + rimWidth + leftWidth, rect->y,
		            rimWidth + doneWidth, rect->h, cgs.media.whiteShader );
	}
	else
	{
		CG_DrawPic( rect->x, rect->y, rimWidth + doneWidth, rect->h, cgs.media.whiteShader );
		CG_DrawPic( rimWidth + rect->x + doneWidth, rect->y,
		            leftWidth, rimWidth, cgs.media.whiteShader );
		CG_DrawPic( rimWidth + rect->x + doneWidth, rect->y + rect->h - rimWidth,
		            leftWidth, rimWidth, cgs.media.whiteShader );
		CG_DrawPic( rect->x + rect->w - rimWidth, rect->y, rimWidth, rect->h, cgs.media.whiteShader );
	}

	trap_R_SetColor( NULL );

	//draw text
	if ( scale > 0.0 )
	{
		Com_sprintf( textBuffer, sizeof( textBuffer ), "%d%%", ( int )( progress * 100 ) );
		CG_AlignText( rect, textBuffer, scale, 0.0f, 0.0f, textalign, VALIGN_CENTER, &tx, &ty );

		UI_Text_Paint( tx, ty, scale, color, textBuffer, 0, textStyle );
	}
}

//=============== TA: was cg_newdraw.c

#define NO_CREDITS_TIME 2000

static void CG_DrawPlayerCreditsValue( rectDef_t *rect, vec4_t color, qboolean padding )
{
	int           value;
	playerState_t *ps;
	centity_t     *cent;
	vec4_t        localColor;

	cent = &cg_entities[ cg.snap->ps.clientNum ];
	ps = &cg.snap->ps;

	//if the build timer pie is showing don't show this
	if ( ( cent->currentState.weapon == WP_ABUILD ||
	       cent->currentState.weapon == WP_ABUILD2 ) && ps->stats[ STAT_MISC ] )
	{
		return;
	}

	value = ps->persistant[ PERS_CREDIT ];

	if ( value > -1 )
	{
		Vector4Copy( color, localColor );

		if ( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_ALIENS )
		{
			if ( !BG_AlienCanEvolve( cg.predictedPlayerState.stats[ STAT_CLASS ], value ) &&
			     cg.time - cg.lastEvolveAttempt <= NO_CREDITS_TIME &&
			     ( ( cg.time - cg.lastEvolveAttempt ) / 300 ) & 1 )
			{
				localColor[ 3 ] = 0.0f;
			}

			value /= CREDITS_PER_EVO;
		}

		trap_R_SetColor( localColor );

		if ( padding )
		{
			CG_DrawFieldPadded( rect->x, rect->y, 4, rect->w / 4, rect->h, value );
		}
		else
		{
			CG_DrawField( rect->x, rect->y, 1, rect->w, rect->h, value );
		}

		trap_R_SetColor( NULL );
	}
}

static void CG_DrawPlayerCreditsFraction( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	float fraction;
	float height;
	rectDef_t aRect;

	if ( cg.predictedPlayerState.persistant[ PERS_TEAM ] != TEAM_ALIENS )
	{
		return;
	}

	fraction = ( ( float )( cg.predictedPlayerState.persistant[ PERS_CREDIT ] %
	             CREDITS_PER_EVO ) ) / CREDITS_PER_EVO;

	aRect = *rect;
	CG_AdjustFrom640( &aRect.x, &aRect.y, &aRect.w, &aRect.h );
	height = aRect.h * fraction;

	trap_R_SetColor( color );
	trap_R_DrawStretchPic( aRect.x, aRect.y - height + aRect.h, aRect.w,
	                       height, 0.0f, 1.0f - fraction, 1.0f, 1.0f, shader );
	trap_R_SetColor( NULL );
}

static void CG_DrawPlayerAlienEvos( rectDef_t *rect, float text_x, float text_y,
                                    vec4_t color, float scale, int textalign, int textvalign, int textStyle )
{
	float         value;
	float         tx, ty;
	playerState_t *ps;
	char           *s;
	vec4_t        localColor;

	ps = &cg.snap->ps;

	value = ( float ) ps->persistant[ PERS_CREDIT ];

	if ( value > -1 )
	{
		Vector4Copy( color, localColor );

		if ( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_ALIENS )
		{
			if ( !BG_AlienCanEvolve( cg.predictedPlayerState.stats[ STAT_CLASS ], value ) &&
			     cg.time - cg.lastEvolveAttempt <= NO_CREDITS_TIME &&
			     ( ( cg.time - cg.lastEvolveAttempt ) / 300 ) & 1 )
			{
				localColor[ 3 ] = 0.0f;
			}

			value /= ( float ) CREDITS_PER_EVO;
		}

		s = va( "%0.1f", floor( value * 10 ) / 10 );
		CG_AlignText( rect, s, scale, 0.0f, 0.0f, textalign, textvalign, &tx, &ty );

		UI_Text_Paint( text_x + tx, text_y + ty, scale, localColor, s, 0, textStyle );
	}
}

/*
==============
CG_DrawPlayerClipsRing
==============
*/
static void CG_DrawPlayerClipsRing( rectDef_t *rect, vec4_t backColor,
                                    vec4_t foreColor, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	centity_t     *cent;
	float         buildTime = ps->stats[ STAT_MISC ];
	float         progress;
	float         maxDelay;
	weapon_t      weapon;
	vec4_t        color;

	cent = &cg_entities[ cg.snap->ps.clientNum ];
	weapon = BG_GetPlayerWeapon( ps );

	switch ( weapon )
	{
		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
			if ( buildTime > MAXIMUM_BUILD_TIME )
			{
				buildTime = MAXIMUM_BUILD_TIME;
			}

			progress = ( MAXIMUM_BUILD_TIME - buildTime ) / MAXIMUM_BUILD_TIME;

			Vector4Lerp( progress, backColor, foreColor, color );
			break;

		default:
			if ( ps->weaponstate == WEAPON_RELOADING )
			{
				maxDelay = ( float ) BG_Weapon( cent->currentState.weapon )->reloadTime;
				progress = ( maxDelay - ( float ) ps->weaponTime ) / maxDelay;

				Vector4Lerp( progress, backColor, foreColor, color );
			}
			else
			{
				Com_Memcpy( color, foreColor, sizeof( color ) );
			}

			break;
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerBuildTimerRing
==============
*/
static void CG_DrawPlayerBuildTimerRing( rectDef_t *rect, vec4_t backColor,
    vec4_t foreColor, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	float         buildTime = ps->stats[ STAT_MISC ];
	float         progress;
	vec4_t        color;

	if ( buildTime > MAXIMUM_BUILD_TIME )
	{
		buildTime = MAXIMUM_BUILD_TIME;
	}

	progress = ( MAXIMUM_BUILD_TIME - buildTime ) / MAXIMUM_BUILD_TIME;

	Vector4Lerp( progress, backColor, foreColor, color );

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerBoosted
==============
*/
static void CG_DrawPlayerBoosted( rectDef_t *rect, vec4_t backColor,
                                  vec4_t foreColor, qhandle_t shader )
{
	if ( cg.snap->ps.stats[ STAT_STATE ] & SS_BOOSTED )
	{
		trap_R_SetColor( foreColor );
	}
	else
	{
		trap_R_SetColor( backColor );
	}

	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerBoosterBolt
==============
*/
static void CG_DrawPlayerBoosterBolt( rectDef_t *rect, vec4_t backColor,
                                      vec4_t foreColor, qhandle_t shader )
{
	vec4_t color;

	// Flash bolts when the boost is almost out
	if ( ( cg.snap->ps.stats[ STAT_STATE ] & SS_BOOSTED ) &&
	     ( cg.snap->ps.stats[ STAT_STATE ] & SS_BOOSTEDWARNING ) )
	{
		Vector4Lerp( ( sin( cg.time / 100.0f ) + 1 ) / 2,
		             backColor, foreColor, color );
	}
	else
	{
		Vector4Copy( foreColor, color );
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerPoisonBarbs
==============
*/
static void CG_DrawPlayerPoisonBarbs( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	qboolean vertical;
	float    x = rect->x, y = rect->y;
	float    width = rect->w, height = rect->h;
	float    diff;
	int      iconsize, numBarbs, maxBarbs;

	maxBarbs = BG_Weapon( cg.snap->ps.weapon )->maxAmmo;
	numBarbs = cg.snap->ps.ammo;

	if ( maxBarbs <= 0 || numBarbs <= 0 )
	{
		return;
	}

	// adjust these first to ensure the aspect ratio of the barb image is
	// preserved
	CG_AdjustFrom640( &x, &y, &width, &height );

	if ( height > width )
	{
		vertical = qtrue;
		iconsize = width;

		if ( maxBarbs != 1 ) // avoid division by zero
		{
			diff = ( height - iconsize ) / ( float )( maxBarbs - 1 );
		}
		else
		{
			diff = 0; // doesn't matter, won't be used
		}
	}
	else
	{
		vertical = qfalse;
		iconsize = height;

		if ( maxBarbs != 1 )
		{
			diff = ( width - iconsize ) / ( float )( maxBarbs - 1 );
		}
		else
		{
			diff = 0;
		}
	}

	trap_R_SetColor( color );

	for ( ; numBarbs > 0; numBarbs-- )
	{
		trap_R_DrawStretchPic( x, y, iconsize, iconsize, 0, 0, 1, 1, shader );

		if ( vertical )
		{
			y += diff;
		}
		else
		{
			x += diff;
		}
	}

	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerWallclimbing
==============
*/
static void CG_DrawPlayerWallclimbing( rectDef_t *rect, vec4_t backColor, vec4_t foreColor, qhandle_t shader )
{
	if ( cg.snap->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING )
	{
		trap_R_SetColor( foreColor );
	}
	else
	{
		trap_R_SetColor( backColor );
	}

	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

static void CG_DrawPlayerAmmoValue( rectDef_t *rect, vec4_t color )
{
	int      value;
	int      valueMarked = -1;
	qboolean bp = qfalse;

	switch ( BG_PrimaryWeapon( cg.snap->ps.stats ) )
	{
		case WP_NONE:
		case WP_BLASTER:
			return;

		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
			value = cg.snap->ps.persistant[ PERS_BP ];
			valueMarked = cg.snap->ps.persistant[ PERS_MARKEDBP ];
			bp = qtrue;
			break;

		default:
			value = cg.snap->ps.ammo;
			break;
	}

	if ( value > 999 )
	{
		value = 999;
	}

	if ( valueMarked > 999 )
	{
		valueMarked = 999;
	}

	if ( value > -1 )
	{
		float tx, ty;
		char  *text;
		float scale;
		int   len;

		trap_R_SetColor( color );

		if ( !bp )
		{
			CG_DrawField( rect->x - 5, rect->y, 4, rect->w / 4, rect->h, value );
			trap_R_SetColor( NULL );
			return;
		}

		if ( valueMarked > 0 )
		{
			text = va( "%d+(%d)", value, valueMarked );
		}
		else
		{
			text = va( "%d", value );
		}

		len = strlen( text );

		if ( len <= 4 )
		{
			scale = 0.50;
		}
		else if ( len <= 6 )
		{
			scale = 0.43;
		}
		else if ( len == 7 )
		{
			scale = 0.36;
		}
		else if ( len == 8 )
		{
			scale = 0.33;
		}
		else
		{
			scale = 0.31;
		}

		CG_AlignText( rect, text, scale, 0.0f, 0.0f, ALIGN_RIGHT, VALIGN_CENTER, &tx, &ty );
		UI_Text_Paint( tx + 1, ty, scale, color, text, 0, ITEM_TEXTSTYLE_PLAIN );
		trap_R_SetColor( NULL );
	}
}

static void CG_DrawPlayerTotalAmmoValue( rectDef_t *rect, vec4_t color )
{
	int      value;
	int      valueMarked = -1;
	qboolean bp = qfalse;
	weapon_t weapon;

	switch ( weapon = BG_PrimaryWeapon( cg.snap->ps.stats ) )
	{
		case WP_NONE:
		case WP_BLASTER:
			return;

		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
			value = cg.snap->ps.persistant[ PERS_BP ];
			valueMarked = cg.snap->ps.persistant[ PERS_MARKEDBP ];
			bp = qtrue;
			break;

		default:
			value = cg.snap->ps.ammo + ( cg.snap->ps.clips * BG_Weapon( weapon )->maxAmmo );
			break;
	}

	if ( value > 999 )
	{
		value = 999;
	}

	if ( valueMarked > 999 )
	{
		valueMarked = 999;
	}

	if ( value > -1 )
	{
		float tx, ty;
		char  *text;
		float scale;
		int   len;

		trap_R_SetColor( color );

		if ( !bp )
		{
			CG_DrawField( rect->x - 5, rect->y, 4, rect->w / 4, rect->h, value );
			trap_R_SetColor( NULL );
			return;
		}

		if ( valueMarked > 0 )
		{
			text = va( "%d+(%d)", value, valueMarked );
		}
		else
		{
			text = va( "%d", value );
		}

		len = strlen( text );

		if ( len <= 4 )
		{
			scale = 0.50;
		}
		else if ( len <= 6 )
		{
			scale = 0.43;
		}
		else if ( len == 7 )
		{
			scale = 0.36;
		}
		else if ( len == 8 )
		{
			scale = 0.33;
		}
		else
		{
			scale = 0.31;
		}

		CG_AlignText( rect, text, scale, 0.0f, 0.0f, ALIGN_RIGHT, VALIGN_CENTER, &tx, &ty );
		UI_Text_Paint( tx + 1, ty, scale, color, text, 0, ITEM_TEXTSTYLE_PLAIN );
		trap_R_SetColor( NULL );
	}
}

/*
==============
CG_DrawAlienSense
==============
*/
static void CG_DrawAlienSense( rectDef_t *rect )
{
	if ( BG_ClassHasAbility( cg.snap->ps.stats[ STAT_CLASS ], SCA_ALIENSENSE ) )
	{
		CG_AlienSense( rect );
	}
}

/*
==============
CG_DrawHumanScanner
==============
*/
static void CG_DrawHumanScanner( rectDef_t *rect, qhandle_t shader, vec4_t color )
{
	if ( BG_InventoryContainsUpgrade( UP_RADAR, cg.snap->ps.stats ) )
	{
		CG_Scanner( rect, shader, color );
	}
}

/*
==============
CG_DrawUsableBuildable
==============
*/
static void CG_DrawUsableBuildable( rectDef_t *rect, qhandle_t shader, vec4_t color )
{
	vec3_t        view, point;
	trace_t       trace;
	entityState_t *es;

	AngleVectors( cg.refdefViewAngles, view, NULL, NULL );
	VectorMA( cg.refdef.vieworg, 64, view, point );
	CG_Trace( &trace, cg.refdef.vieworg, NULL, NULL,
	          point, cg.predictedPlayerState.clientNum, MASK_SHOT );

	es = &cg_entities[ trace.entityNum ].currentState;

	if ( es->eType == ET_BUILDABLE && BG_Buildable( es->modelindex )->usable &&
	     cg.predictedPlayerState.persistant[ PERS_TEAM ] == BG_Buildable( es->modelindex )->team )
	{
		//hack to prevent showing the usable buildable when you aren't carrying an energy weapon
		if ( ( es->modelindex == BA_H_REACTOR || es->modelindex == BA_H_REPEATER ) &&
		     ( !BG_Weapon( cg.snap->ps.weapon )->usesEnergy ||
		       BG_Weapon( cg.snap->ps.weapon )->infiniteAmmo ) )
		{
			cg.nearUsableBuildable = BA_NONE;
			return;
		}

		trap_R_SetColor( color );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
		trap_R_SetColor( NULL );
		cg.nearUsableBuildable = es->modelindex;
	}
	else
	{
		cg.nearUsableBuildable = BA_NONE;
	}
}

#define BUILD_DELAY_TIME 2000

static void CG_DrawPlayerBuildTimer( rectDef_t *rect, vec4_t color )
{
	int           index;
	playerState_t *ps;
	vec4_t        localColor;

	ps = &cg.snap->ps;

	if ( ps->stats[ STAT_MISC ] <= 0 )
	{
		return;
	}

	switch ( BG_PrimaryWeapon( ps->stats ) )
	{
		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
			break;

		default:
			return;
	}

	index = 8 * ( ps->stats[ STAT_MISC ] - 1 ) / MAXIMUM_BUILD_TIME;

	if ( index > 7 )
	{
		index = 7;
	}
	else if ( index < 0 )
	{
		index = 0;
	}

	Vector4Copy( color, localColor );

	if ( cg.time - cg.lastBuildAttempt <= BUILD_DELAY_TIME &&
	     ( ( cg.time - cg.lastBuildAttempt ) / 300 ) % 2 )
	{
		localColor[ 0 ] = 1.0f;
		localColor[ 1 ] = localColor[ 2 ] = 0.0f;
		localColor[ 3 ] = 1.0f;
	}

	trap_R_SetColor( localColor );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h,
	            cgs.media.buildWeaponTimerPie[ index ] );
	trap_R_SetColor( NULL );
}

static void CG_DrawPlayerClipsValue( rectDef_t *rect, vec4_t color )
{
	int           value;
	playerState_t *ps = &cg.snap->ps;

	switch ( BG_PrimaryWeapon( ps->stats ) )
	{
		case WP_NONE:
		case WP_BLASTER:
		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
			return;

		default:
			value = ps->clips;

			if ( value > -1 )
			{
				trap_R_SetColor( color );
				CG_DrawField( rect->x, rect->y, 4, rect->w / 4, rect->h, value );
				trap_R_SetColor( NULL );
			}

			break;
	}
}

static void CG_DrawPlayerHealthValue( rectDef_t *rect, vec4_t color )
{
	trap_R_SetColor( color );
	CG_DrawField( rect->x, rect->y, 4, rect->w / 4, rect->h, cg.snap->ps.stats[ STAT_HEALTH ] );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerHealthCross
==============
*/
static void CG_DrawPlayerHealthCross( rectDef_t *rect, vec4_t ref_color )
{
	qhandle_t shader;
	vec4_t    color;
	float     ref_alpha;

	// Pick the current icon
	shader = cgs.media.healthCross;

	if ( cg.snap->ps.stats[ STAT_STATE ] & SS_HEALING_8X )
	{
		shader = cgs.media.healthCross3X;
	}
	else if ( cg.snap->ps.stats[ STAT_STATE ] & SS_HEALING_4X )
	{
		if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_ALIENS )
		{
			shader = cgs.media.healthCross2X;
		}
		else
		{
			shader = cgs.media.healthCrossMedkit;
		}
	}
	else if ( cg.snap->ps.stats[ STAT_STATE ] & SS_POISONED )
	{
		shader = cgs.media.healthCrossPoisoned;
	}

	// Pick the alpha value
	Vector4Copy( ref_color, color );

	if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_HUMANS &&
	     cg.snap->ps.stats[ STAT_HEALTH ] < 10 )
	{
		color[ 0 ] = 1.0f;
		color[ 1 ] = color[ 2 ] = 0.0f;
	}

	ref_alpha = ref_color[ 3 ];

	if ( cg.snap->ps.stats[ STAT_STATE ] & SS_HEALING_2X )
	{
		ref_alpha = 1.0f;
	}

	// Don't fade from nothing
	if ( !cg.lastHealthCross )
	{
		cg.lastHealthCross = shader;
	}

	// Fade the icon during transition
	if ( cg.lastHealthCross != shader )
	{
		cg.healthCrossFade += cg.frametime / 500.0f;

		if ( cg.healthCrossFade > 1.0f )
		{
			cg.healthCrossFade = 0.0f;
			cg.lastHealthCross = shader;
		}
		else
		{
			// Fading between two icons
			color[ 3 ] = ref_alpha * cg.healthCrossFade;
			trap_R_SetColor( color );
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
			color[ 3 ] = ref_alpha * ( 1.0f - cg.healthCrossFade );
			trap_R_SetColor( color );
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg.lastHealthCross );
			trap_R_SetColor( NULL );
			return;
		}
	}

	// Not fading, draw a single icon
	color[ 3 ] = ref_alpha;
	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

float CG_ChargeProgress( void )
{
	float progress;
	int   min = 0, max = 0;

	if ( cg.snap->ps.weapon == WP_ALEVEL1 )
	{
		min = 0;
		max = MAX( LEVEL1_POUNCE_COOLDOWN, LEVEL1_SIDEPOUNCE_COOLDOWN );
	}
	else if ( cg.snap->ps.weapon == WP_ALEVEL3 )
	{
		min = LEVEL3_POUNCE_TIME_MIN;
		max = LEVEL3_POUNCE_TIME;
	}
	else if ( cg.snap->ps.weapon == WP_ALEVEL3_UPG )
	{
		min = LEVEL3_POUNCE_TIME_MIN;
		max = LEVEL3_POUNCE_TIME_UPG;
	}
	else if ( cg.snap->ps.weapon == WP_ALEVEL4 )
	{
		if ( cg.predictedPlayerState.stats[ STAT_STATE ] & SS_CHARGING )
		{
			min = 0;
			max = LEVEL4_TRAMPLE_DURATION;
		}
		else
		{
			min = LEVEL4_TRAMPLE_CHARGE_MIN;
			max = LEVEL4_TRAMPLE_CHARGE_MAX;
		}
	}
	else if ( cg.snap->ps.weapon == WP_LUCIFER_CANNON )
	{
		min = LCANNON_CHARGE_TIME_MIN;
		max = LCANNON_CHARGE_TIME_MAX;
	}

	if ( max - min <= 0.0f )
	{
		return 0.0f;
	}

	progress = ( ( float ) cg.predictedPlayerState.stats[ STAT_MISC ] - min ) /
	           ( max - min );

	if ( progress > 1.0f )
	{
		return 1.0f;
	}

	if ( progress < 0.0f )
	{
		return 0.0f;
	}

	return progress;
}

#define CHARGE_BAR_FADE_RATE 0.002f

static void CG_DrawPlayerChargeBarBG( rectDef_t *rect, vec4_t ref_color,
                                      qhandle_t shader )
{
	vec4_t color;

	if ( !cg_drawChargeBar.integer || cg.chargeMeterAlpha <= 0.0f )
	{
		return;
	}

	color[ 0 ] = ref_color[ 0 ];
	color[ 1 ] = ref_color[ 1 ];
	color[ 2 ] = ref_color[ 2 ];
	color[ 3 ] = ref_color[ 3 ] * cg.chargeMeterAlpha;

	// Draw meter background
	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

// FIXME: This should come from the element info
#define CHARGE_BAR_CAP_SIZE 3

static void CG_DrawPlayerProgressBar( rectDef_t *rect, vec4_t ref_color, float progress, float warning,
                                      qhandle_t shader )
{
	vec4_t color;
	float  x, y, width, height, cap_size;

	Vector4Copy( ref_color, color );

	// Flash red for warning
	if ( ( ( warning < 0 && progress < -warning ) || ( warning > 0 && progress > warning ) )  &&
	     ( cg.time & 128 ) )
	{
		color[ 0 ] = 1.0f;
		color[ 1 ] = 0.0f;
		color[ 2 ] = 0.0f;
	}

	x = rect->x;
	y = rect->y;

	// Horizontal charge bar
	if ( rect->w >= rect->h )
	{
		width = ( rect->w - CHARGE_BAR_CAP_SIZE * 2 ) * progress;
		height = rect->h;
		CG_AdjustFrom640( &x, &y, &width, &height );
		cap_size = CHARGE_BAR_CAP_SIZE * cgs.screenXScale;

		// Draw the meter
		trap_R_SetColor( color );
		trap_R_DrawStretchPic( x, y, cap_size, height, 0, 0, 1, 1, shader );
		trap_R_DrawStretchPic( x + width + cap_size, y, cap_size, height,
		                       1, 0, 0, 1, shader );
		trap_R_DrawStretchPic( x + cap_size, y, width, height, 1, 0, 1, 1, shader );
		trap_R_SetColor( NULL );
	}

	// Vertical charge bar
	else
	{
		y += rect->h;
		width = rect->w;
		height = ( rect->h - CHARGE_BAR_CAP_SIZE * 2 ) * progress;
		CG_AdjustFrom640( &x, &y, &width, &height );
		cap_size = CHARGE_BAR_CAP_SIZE * cgs.screenYScale;

		// Draw the meter
		trap_R_SetColor( color );
		trap_R_DrawStretchPic( x, y - cap_size, width, cap_size,
		                       0, 1, 1, 0, shader );
		trap_R_DrawStretchPic( x, y - height - cap_size * 2, width,
		                       cap_size, 0, 0, 1, 1, shader );
		trap_R_DrawStretchPic( x, y - height - cap_size, width, height,
		                       0, 1, 1, 1, shader );
		trap_R_SetColor( NULL );
	}
}

static void CG_DrawPlayerChargeBar( rectDef_t *rect, vec4_t ref_color,
                                    qhandle_t shader )
{
	vec4_t color;
	float  x, y, width, height, cap_size, progress;

	if ( !cg_drawChargeBar.integer )
	{
		return;
	}

	// Get progress proportion and pump fade
	progress = CG_ChargeProgress();

	if ( progress <= 0.0f )
	{
		cg.chargeMeterAlpha -= CHARGE_BAR_FADE_RATE * cg.frametime;

		if ( cg.chargeMeterAlpha <= 0.0f )
		{
			cg.chargeMeterAlpha = 0.0f;
			return;
		}
	}
	else
	{
		cg.chargeMeterValue = progress;
		cg.chargeMeterAlpha += CHARGE_BAR_FADE_RATE * cg.frametime;

		if ( cg.chargeMeterAlpha > 1.0f )
		{
			cg.chargeMeterAlpha = 1.0f;
		}
	}

	color[ 0 ] = ref_color[ 0 ];
	color[ 1 ] = ref_color[ 1 ];
	color[ 2 ] = ref_color[ 2 ];
	color[ 3 ] = ref_color[ 3 ] * cg.chargeMeterAlpha;

	// Flash red for Lucifer Cannon warning
	if ( cg.snap->ps.weapon == WP_LUCIFER_CANNON &&
	     cg.snap->ps.stats[ STAT_MISC ] >= LCANNON_CHARGE_TIME_WARN &&
	     ( cg.time & 128 ) )
	{
		color[ 0 ] = 1.0f;
		color[ 1 ] = 0.0f;
		color[ 2 ] = 0.0f;
	}

	x = rect->x;
	y = rect->y;

	// Horizontal charge bar
	if ( rect->w >= rect->h )
	{
		width = ( rect->w - CHARGE_BAR_CAP_SIZE * 2 ) * cg.chargeMeterValue;
		height = rect->h;
		CG_AdjustFrom640( &x, &y, &width, &height );
		cap_size = CHARGE_BAR_CAP_SIZE * cgs.screenXScale;

		// Draw the meter
		trap_R_SetColor( color );
		trap_R_DrawStretchPic( x, y, cap_size, height, 0, 0, 1, 1, shader );
		trap_R_DrawStretchPic( x + width + cap_size, y, cap_size, height,
		                       1, 0, 0, 1, shader );
		trap_R_DrawStretchPic( x + cap_size, y, width, height, 1, 0, 1, 1, shader );
		trap_R_SetColor( NULL );
	}

	// Vertical charge bar
	else
	{
		y += rect->h;
		width = rect->w;
		height = ( rect->h - CHARGE_BAR_CAP_SIZE * 2 ) * cg.chargeMeterValue;
		CG_AdjustFrom640( &x, &y, &width, &height );
		cap_size = CHARGE_BAR_CAP_SIZE * cgs.screenYScale;

		// Draw the meter
		trap_R_SetColor( color );
		trap_R_DrawStretchPic( x, y - cap_size, width, cap_size,
		                       0, 1, 1, 0, shader );
		trap_R_DrawStretchPic( x, y - height - cap_size * 2, width,
		                       cap_size, 0, 0, 1, 1, shader );
		trap_R_DrawStretchPic( x, y - height - cap_size, width, height,
		                       0, 1, 1, 1, shader );
		trap_R_SetColor( NULL );
	}
}

#define MOMENTUM_BAR_MARKWIDTH 0.5f
#define MOMENTUM_BAR_GLOWTIME  2000

static void CG_DrawPlayerMomentumBar( rectDef_t *rect, vec4_t foreColor, vec4_t backColor, float borderSize )
{
	// data
	playerState_t *ps;
	float         momentum, rawFraction, fraction, glowFraction, glowOffset;
	int           threshold;
	team_t        team;
	qboolean      unlocked;

	momentumThresholdIterator_t unlockableIter = { -1 };

	// display
	vec4_t        color;
	float         x, y, w, h, b, glowStrength;
	qboolean      vertical;

	ps = &cg.predictedPlayerState;

	team       = (team_t) ps->persistant[ PERS_TEAM ];
	momentum = ps->persistant[ PERS_MOMENTUM ] / 10.0f;

	x = rect->x;
	y = rect->y;
	w = rect->w;
	h = rect->h;
	b = borderSize;

	vertical = ( h > w );

	// draw border
	CG_DrawRect( x, y, w, h, borderSize, backColor );

	// adjust rect to draw inside border
	x += b;
	y += b;
	w -= 2.0f * b;
	h -= 2.0f * b;

	// draw background
	Vector4Copy( backColor, color );
	color[ 3 ] *= 0.5f;
	CG_FillRect( x, y, w, h, color );

	// draw momentum bar
	fraction = rawFraction = momentum / MOMENTUM_MAX;

	if ( fraction < 0.0f )
	{
		fraction = 0.0f;
	}
	else if ( fraction > 1.0f )
	{
		fraction = 1.0f;
	}

	if ( vertical )
	{
		CG_FillRect( x, y + h * ( 1.0f - fraction ), w, h * fraction, foreColor );
	}
	else
	{
		CG_FillRect( x, y, w * fraction, h, foreColor );
	}

	// draw glow on momentum event
	if ( cg.momentumGainedTime + MOMENTUM_BAR_GLOWTIME > cg.time )
	{
		glowFraction = fabs( cg.momentumGained / MOMENTUM_MAX );
		glowStrength = ( MOMENTUM_BAR_GLOWTIME - ( cg.time - cg.momentumGainedTime ) ) /
		               ( float )MOMENTUM_BAR_GLOWTIME;

		if ( cg.momentumGained > 0.0f )
		{
			glowOffset = vertical ? 0 : glowFraction;
		}
		else
		{
			glowOffset = vertical ? glowFraction : 0;
		}

		CG_SetClipRegion( x, y, w, h );

		color[ 0 ] = 1.0f;
		color[ 1 ] = 1.0f;
		color[ 2 ] = 1.0f;
		color[ 3 ] = 0.5f * glowStrength;

		if ( vertical )
		{
			CG_FillRect( x, y + h * ( 1.0f - ( rawFraction + glowOffset ) ), w, h * glowFraction, color );
		}
		else
		{
			CG_FillRect( x + w * ( rawFraction - glowOffset ), y, w * glowFraction, h, color );
		}

		CG_ClearClipRegion();
	}

	// draw threshold markers
	while ( ( unlockableIter = BG_IterateMomentumThresholds( unlockableIter, team, &threshold, &unlocked ) ),
	        ( unlockableIter.num >= 0 ) )
	{
		fraction = threshold / MOMENTUM_MAX;

		if ( fraction > 1.0f )
		{
			fraction = 1.0f;
		}

		if ( unlocked )
		{
			color[ 0 ] = 1.0f;
			color[ 1 ] = 1.0f;
			color[ 2 ] = 0.0f;
			color[ 3 ] = 1.0f;
		}
		else
		{
			color[ 0 ] = 0.0f;
			color[ 1 ] = 1.0f;
			color[ 2 ] = 0.0f;
			color[ 3 ] = 1.0f;
		}

		if ( vertical )
		{
			CG_FillRect( x, y + h * ( 1.0f - fraction ), w, MOMENTUM_BAR_MARKWIDTH, color );
		}
		else
		{
			CG_FillRect( x + w * fraction, y, MOMENTUM_BAR_MARKWIDTH, h, color );
		}
	}

	trap_R_SetColor( NULL );
}

static INLINE qhandle_t CG_GetUnlockableIcon( int num )
{
	int index = BG_UnlockableTypeIndex( num );

	switch ( BG_UnlockableType( num ) )
	{
		case UNLT_WEAPON:    return cg_weapons[ index ].weaponIcon;
		case UNLT_UPGRADE:   return cg_upgrades[ index ].upgradeIcon;
		case UNLT_BUILDABLE: return cg_buildables[ index ].buildableIcon;
		case UNLT_CLASS:     return cg_classes[ index ].classIcon;
		default:             return 0;
	}
}

static void CG_DrawPlayerUnlockedItems( rectDef_t *rect, vec4_t foreColour, vec4_t backColour, float borderSize )
{
	momentumThresholdIterator_t unlockableIter = { -1, 1 }, previousIter;

	// data
	team_t    team;

	// display
	float     x, y, w, h, iw, ih;
	qboolean  vertical;

	int       icons, counts;
	int       count[ 32 ] = { 0 };
	struct {
		qhandle_t shader;
		qboolean  unlocked;
	} icon[ NUM_UNLOCKABLES ]; // more than enough(!)

	team = (team_t) cg.predictedPlayerState.persistant[ PERS_TEAM ];

	w = rect->w - 2 * borderSize;
	h = rect->h - 2 * borderSize;

	vertical = ( h > w );

	ih = vertical ? w : h;
	iw = ih * cgDC.aspectScale;

	x = rect->x + borderSize;
	y = rect->y + borderSize + ( h - ih ) * vertical;

	icons = counts = 0;

	for (;;)
	{
		qhandle_t shader;
		int       threshold;
		qboolean  unlocked;

		previousIter = unlockableIter;
		unlockableIter = BG_IterateMomentumThresholds( unlockableIter, team, &threshold, &unlocked );

		if ( previousIter.threshold != unlockableIter.threshold && icons )
		{
			count[ counts++ ] = icons;
		}

		// maybe exit the loop?
		if ( unlockableIter.num < 0 )
		{
			break;
		}

		// okay, next icon
		shader = CG_GetUnlockableIcon( unlockableIter.num );

		if ( shader )
		{
			icon[ icons ].shader = shader;
			icon[ icons].unlocked = unlocked;
			++icons;
		}
	}

	{
		float gap;
		int i, j;
		vec4_t unlockedBg, lockedBg;

		Vector4Copy( foreColour, unlockedBg );
		unlockedBg[ 3 ] *= 0.5f;
		Vector4Copy( backColour, lockedBg );
		lockedBg[ 3 ] *= 0.5f;

		gap = vertical ? ( h - icons * ih ) : ( w - icons * iw );

		if ( counts > 2 )
		{
			gap /= counts - 1;
		}

		for ( i = 0, j = 0; count[ i ]; ++i )
		{
			if ( vertical )
			{
				float yb = y - count[ i ] * ih - i * gap;
				float hb = ( count[ i ] - j ) * ih;
				CG_DrawRect( x - borderSize, yb - borderSize, iw + 2 * borderSize, hb, borderSize, backColour );
				CG_FillRect( x, yb, iw, hb - 2 * borderSize, icon[ j ].unlocked ? unlockedBg : lockedBg );
			}
			else
			{
				float xb = x + j * iw + i * gap;
				float wb = ( count[ i ] - j ) * iw + 2;
				CG_DrawRect( xb - borderSize, y - borderSize, wb, ih + 2 * borderSize, borderSize, backColour );
				CG_FillRect( xb, y, wb - 2 * borderSize, ih, icon[ j ].unlocked ? unlockedBg : lockedBg );
			}
			j = count[ i ];
		}

		for ( i = 0, j = 0; i < icons; ++i )
		{
			trap_R_SetColor( icon[ i ].unlocked ? foreColour : backColour );

			if ( i == count[ j ] )
			{
				++j;
				if ( vertical ) { y -= gap; } else { x += gap; }
			}

			CG_DrawPic( x, y, iw, ih, icon[ i ].shader );

			if ( vertical ) { y -= ih; } else { x += iw; }
		}
	}

	trap_R_SetColor( NULL );
}

static void CG_DrawPlayerBuildTimerBar( rectDef_t *rect, vec4_t foreColor, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	float         progress;
	weapon_t      weapon;
	static int    misc = 0;
	static int    max;

	weapon = BG_GetPlayerWeapon( ps );

	// Check if player is a builder
	if( weapon != WP_HBUILD && weapon != WP_ABUILD && weapon != WP_ABUILD2 )
	{
		return;
	}

	// Not building anything
	if( ps->stats[ STAT_MISC ] <= 0 )
	{
		misc = ps->stats[ STAT_MISC ];
		return;
	}

	// We are building something new. Change the max value
	if( ps->stats[ STAT_MISC ] > 0 && misc <= 0 )
	{
		max = ps->stats[ STAT_MISC ];
	}

	misc = ps->stats[ STAT_MISC ];
	progress = (float)misc/(float)max;

	CG_DrawPlayerProgressBar( rect, foreColor, 1-progress, 0.9, shader );
}

static void CG_DrawPlayerHealthBar( rectDef_t *rect, vec4_t foreColor, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;

	float progress = (float)ps->stats[ STAT_HEALTH ] / (float)BG_Class( ps->stats[ STAT_CLASS ] )->health;
	CG_DrawPlayerProgressBar( rect, foreColor, progress, -0.3, shader );
}

static void CG_DrawPlayerMeter( rectDef_t *rect, int align, float fraction, vec4_t color, qhandle_t shader )
{
	rectDef_t aRect = *rect;

	CG_AdjustFrom640( &aRect.x, &aRect.y, &aRect.w, &aRect.h );

	// Vertical meter
	if( aRect.h >= aRect.w )
	{
		float height;
		height = aRect.h * fraction;

		// Meter decreases down
		if ( align == ALIGN_LEFT )
		{
			trap_R_SetColor( color );
			trap_R_DrawStretchPic( aRect.x, aRect.y, aRect.w, height,
								   0.0f, 0.0f, 1.0f, fraction, shader );
			trap_R_SetColor( NULL );
		}
		else
		{
			trap_R_SetColor( color );
			trap_R_DrawStretchPic( aRect.x, aRect.y - height + aRect.h, aRect.w,
								   height, 0.0f, 1.0f - fraction, 1.0f, 1.0f, shader );
			trap_R_SetColor( NULL );
		}
	}

	// Horizontal meter
	else
	{
		float width;

		width = aRect.w * fraction;

		// Meter decreases to the left
		if ( align == ALIGN_LEFT )
		{
			trap_R_SetColor( color );
			trap_R_DrawStretchPic( aRect.x, aRect.y, width,
			                       aRect.h, 0.0f, 0.0f, fraction, 1.0f, shader );
			trap_R_SetColor( NULL );
		}
		else
		{
			trap_R_SetColor( color );
			trap_R_DrawStretchPic( aRect.x - width + aRect.w, aRect.y, width,
			                       aRect.h, 1.0f - fraction, 0.0f, 1.0f, 1.0f, shader );
			trap_R_SetColor( NULL );
		}
	}
}

static void CG_DrawPlayerClipMeter( rectDef_t *rect, int align, vec4_t color, qhandle_t shader )
{
	float    fraction;
	int      maxAmmo;
	weapon_t weapon;

	if ( cg.predictedPlayerState.persistant[ PERS_TEAM ] != TEAM_HUMANS )
	{
		return;
	}

	weapon = BG_PrimaryWeapon( cg.snap->ps.stats );
	maxAmmo = BG_Weapon( weapon )->maxAmmo;

	if ( maxAmmo <= 0 ) { return; }

	fraction = (float)cg.snap->ps.ammo / (float)maxAmmo;

	CG_DrawPlayerMeter( rect, align, fraction, color, shader );
}

static void CG_DrawPlayerHealthMeter( rectDef_t *rect, int align, vec4_t color, qhandle_t shader )
{
	float fraction;

	fraction = (float)cg.snap->ps.stats[ STAT_HEALTH ] / (float)BG_Class( cg.snap->ps.stats[ STAT_CLASS ] )->health;

	CG_DrawPlayerMeter( rect, align, fraction, color, shader );
}

static void CG_DrawPlayerBoostedMeter( rectDef_t *rect, int align, vec4_t foreColor, qhandle_t shader )
{
	static int time = -1;

	if( cg.snap->ps.stats[ STAT_STATE ] & SS_BOOSTED )
	{
		float      progress;

		if( time == -1 || cg.snap->ps.stats[ STAT_STATE ] & SS_BOOSTEDNEW )
		{
			time = cg.time;
		}

		progress = ( (float)cg.time - time ) / BOOST_TIME;

		CG_DrawPlayerMeter( rect, align, 1-progress, foreColor, shader );

	}
	else
	{
		time = -1;
		return;
	}

}

static void CG_DrawPlayerStaminaBar( rectDef_t *rect, vec4_t foreColor, qhandle_t shader )
{
	int   stamina;
	float progress;
	float warning;
	int   staminaJumpCost;

	stamina  = cg.snap->ps.stats[ STAT_STAMINA ];
	staminaJumpCost = BG_Class( cg.snap->ps.stats[ STAT_CLASS ] )->staminaJumpCost;
	progress = ( float )stamina / ( float )STAMINA_MAX;
	warning  = -1.0f * ( float )staminaJumpCost / ( float )STAMINA_MAX;

	CG_DrawPlayerProgressBar( rect, foreColor, progress, warning, shader );
}

static void CG_DrawPlayerStaminaValue( rectDef_t *rect, vec4_t color )
{
	int stamina, percent;

	stamina = cg.snap->ps.stats[ STAT_STAMINA ];
	percent = ( int )( 100.0f * ( float )stamina / ( float )STAMINA_MAX );

	trap_R_SetColor( color );
	CG_DrawField( rect->x, rect->y, 4, rect->w / 4, rect->h, percent );
	trap_R_SetColor( NULL );
}

static void CG_DrawPlayerStaminaBolt( rectDef_t *rect, vec4_t backColor,
                                      vec4_t foreColor, qhandle_t shader )
{
	int      stamina;
	vec4_t   color;
	qboolean sprinting;
	int      staminaJumpCost;

	stamina   = cg.snap->ps.stats[ STAT_STAMINA ];
	staminaJumpCost = BG_Class( cg.snap->ps.stats[ STAT_CLASS ] )->staminaJumpCost;
	sprinting = ( cg.predictedPlayerState.stats[ STAT_STATE ] & SS_SPEEDBOOST );

	if ( stamina < staminaJumpCost )
	{
		Vector4Copy( backColor, color );
	}
	else if ( sprinting )
	{
		Vector4Lerp( ( sin( cg.time / 150.0f ) + 1.0f ) / 2.0f, backColor, foreColor, color );
	}
	else
	{
		Vector4Copy( foreColor, color );
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

static void CG_DrawPlayerFuelBar( rectDef_t *rect, vec4_t foreColor, qhandle_t shader )
{
	int   fuel;
	float progress, warning;

	if ( !BG_InventoryContainsUpgrade( UP_JETPACK, cg.snap->ps.stats ) )
	{
		return;
	}

	fuel     = cg.snap->ps.stats[ STAT_FUEL ];
	progress = ( float )fuel / ( float )JETPACK_FUEL_MAX;
	warning  = -1.0f * ( float )JETPACK_FUEL_LOW / ( float )JETPACK_FUEL_MAX;

	CG_DrawPlayerProgressBar( rect, foreColor, progress, warning, shader );
}

static void CG_DrawPlayerFuelValue( rectDef_t *rect, vec4_t color )
{
	int fuel, percent;

	if ( !BG_InventoryContainsUpgrade( UP_JETPACK, cg.snap->ps.stats ) )
	{
		return;
	}

	fuel    = cg.snap->ps.stats[ STAT_FUEL ];
	percent = ( int )( 100.0f * ( float )fuel / ( float )JETPACK_FUEL_MAX );

	trap_R_SetColor( color );
	CG_DrawField( rect->x, rect->y, 4, rect->w / 4, rect->h, percent );
	trap_R_SetColor( NULL );
}

static void CG_DrawPlayerFuelIcon( rectDef_t *rect, vec4_t backColor,
                                   vec4_t foreColor, qhandle_t shader )
{
	vec4_t   color;
	int      fuel;
	qboolean pmNormal, active;

	if ( !BG_InventoryContainsUpgrade( UP_JETPACK, cg.snap->ps.stats ) )
	{
		return;
	}

	fuel     = cg.snap->ps.stats[ STAT_FUEL ];
	pmNormal = ( cg.snap->ps.pm_type == PM_NORMAL );
	active   = ( cg.snap->ps.stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE );

	if ( fuel < JETPACK_FUEL_LOW || !pmNormal )
	{
		Vector4Copy( backColor, color );
	}
	else if ( active )
	{
		Vector4Lerp( ( sin( cg.time / 150.0f ) + 1.0f ) / 2.0f, backColor, foreColor, color );
	}
	else
	{
		Vector4Copy( foreColor, color );
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

static void CG_DrawMineRate( rectDef_t *rect, float text_x, float text_y,
                             vec4_t color, float scale, int textalign, int textvalign, int textStyle )
{
	char s[ MAX_TOKEN_CHARS ];
	float tx, ty, levelRate, rate;
	int efficiency;

	// check if builder
	switch ( BG_GetPlayerWeapon( &cg.snap->ps ) )
	{
		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
			break;

		default:
			return;
	}

	levelRate  = cg.predictedPlayerState.persistant[ PERS_MINERATE ] / 10.0f;
	efficiency = cg.predictedPlayerState.persistant[ PERS_RGS_EFFICIENCY ];
	rate       = ( ( efficiency / 100.0f ) * levelRate );

	Com_sprintf( s, MAX_TOKEN_CHARS, _("%.1f BP/min (%d%% × %.1f)"), rate, efficiency, levelRate );

	CG_AlignText( rect, s, scale, 0.0f, 0.0f, textalign, textvalign, &tx, &ty );
	UI_Text_Paint( text_x + tx, text_y + ty, scale, color, s, 0, textStyle );
}


static void CG_DrawProgressLabel( rectDef_t *rect, float text_x, float text_y, vec4_t color,
                                  float scale, int textalign, int textvalign,
                                  const char *s, float fraction )
{
	vec4_t white = { 1.0f, 1.0f, 1.0f, 1.0f };
	float  tx, ty;

	CG_AlignText( rect, s, scale, 0.0f, 0.0f, textalign, textvalign, &tx, &ty );

	if ( fraction < 1.0f )
	{
		UI_Text_Paint( text_x + tx, text_y + ty, scale, white,
		               s, 0, ITEM_TEXTSTYLE_PLAIN );
	}
	else
	{
		UI_Text_Paint( text_x + tx, text_y + ty, scale, color,
		               s, 0, ITEM_TEXTSTYLE_NEON );
	}
}

static void CG_DrawMediaProgress( rectDef_t *rect, vec4_t color, float scale,
                                  int align, int textalign, int textStyle,
                                  float borderSize )
{
	CG_DrawProgressBar( rect, color, scale, align, textalign, textStyle,
	                    borderSize, cg.mediaFraction );
}

static void CG_DrawMediaProgressLabel( rectDef_t *rect, float text_x, float text_y,
                                       vec4_t color, float scale, int textalign, int textvalign )
{
	CG_DrawProgressLabel( rect, text_x, text_y, color, scale, textalign, textvalign,
	                      "Map and Textures", cg.mediaFraction );
}

static void CG_DrawBuildablesProgress( rectDef_t *rect, vec4_t color,
                                       float scale, int align, int textalign,
                                       int textStyle, float borderSize )
{
	CG_DrawProgressBar( rect, color, scale, align, textalign, textStyle,
	                    borderSize, cg.buildablesFraction );
}

static void CG_DrawBuildablesProgressLabel( rectDef_t *rect, float text_x, float text_y,
    vec4_t color, float scale, int textalign, int textvalign )
{
	CG_DrawProgressLabel( rect, text_x, text_y, color, scale, textalign, textvalign,
	                      "Buildable Models", cg.buildablesFraction );
}

static void CG_DrawCharModelProgress( rectDef_t *rect, vec4_t color,
                                      float scale, int align, int textalign,
                                      int textStyle, float borderSize )
{
	CG_DrawProgressBar( rect, color, scale, align, textalign, textStyle,
	                    borderSize, cg.charModelFraction );
}

static void CG_DrawCharModelProgressLabel( rectDef_t *rect, float text_x, float text_y,
    vec4_t color, float scale, int textalign, int textvalign )
{
	CG_DrawProgressLabel( rect, text_x, text_y, color, scale, textalign, textvalign,
	                      "Character Models", cg.charModelFraction );
}

static void CG_DrawOverallProgress( rectDef_t *rect, vec4_t color, float scale,
                                    int align, int textalign, int textStyle,
                                    float borderSize )
{
	float total;

	total = cg.charModelFraction + cg.buildablesFraction + cg.mediaFraction;
	total /= 3.0f;

	CG_DrawProgressBar( rect, color, scale, align, textalign, textStyle,
	                    borderSize, total );
}

static void CG_DrawOverallProgressLabel( rectDef_t *rect, float text_x, float text_y,
    vec4_t color, float scale, int textalign, int textvalign )
{
	CG_DrawProgressLabel( rect, text_x, text_y, color, scale, textalign, textvalign,
	                     cg.currentLoadingLabel, cg.charModelFraction );
}

static void CG_DrawLevelShot( rectDef_t *rect )
{
	const char *s;
	const char *info;
	qhandle_t  levelshot;
	qhandle_t  detail;

	info = CG_ConfigString( CS_SERVERINFO );
	s = Info_ValueForKey( info, "mapname" );
	levelshot = trap_R_RegisterShader(va("levelshots/%s", s),
					  RSF_NOMIP);

	if ( !levelshot )
	{
		levelshot = trap_R_RegisterShader("gfx/2d/load_screen",
						  RSF_NOMIP);
	}

	trap_R_SetColor( NULL );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, levelshot );

	// blend a detail texture over it
	detail = trap_R_RegisterShader("gfx/misc/detail", RSF_DEFAULT);
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, detail );
}

static void CG_DrawLevelName( rectDef_t *rect, float text_x, float text_y,
                              vec4_t color, float scale,
                              int textalign, int textvalign, int textStyle )
{
	const char *s;

	s = CG_ConfigString( CS_MESSAGE );

	UI_DrawTextBlock( rect, text_x, text_y, color, scale, textalign, textvalign, textStyle, s );
}

static void CG_DrawMOTD( rectDef_t *rect, float text_x, float text_y,
                         vec4_t color, float scale,
                         int textalign, int textvalign, int textStyle )
{
	const char *s;
	char       parsed[ MAX_STRING_CHARS ];

	s = CG_ConfigString( CS_MOTD );

	Q_ParseNewlines( parsed, s, sizeof( parsed ) );

	UI_DrawTextBlock( rect, text_x, text_y, color, scale, textalign, textvalign, textStyle, parsed );
}

static void CG_DrawHostname( rectDef_t *rect, float text_x, float text_y,
                             vec4_t color, float scale,
                             int textalign, int textvalign, int textStyle )
{
	char       buffer[ 1024 ];
	const char *info;

	info = CG_ConfigString( CS_SERVERINFO );

	UI_EscapeEmoticons( buffer, Info_ValueForKey( info, "sv_hostname" ), sizeof( buffer ) );
	Q_CleanStr( buffer );

	UI_DrawTextBlock( rect, text_x, text_y, color, scale, textalign, textvalign, textStyle, buffer );
}

/*
==============
CG_DrawDemoPlayback
==============
*/
static void CG_DrawDemoPlayback( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	if ( !cg_drawDemoState.integer )
	{
		return;
	}

	if ( trap_GetDemoState() != DS_PLAYBACK )
	{
		return;
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawDemoRecording
==============
*/
static void CG_DrawDemoRecording( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	if ( !cg_drawDemoState.integer )
	{
		return;
	}

	if ( trap_GetDemoState() != DS_RECORDING )
	{
		return;
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
====================
CG_DrawLoadingScreen

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawLoadingScreen( void )
{
	menuDef_t *menu = Menus_FindByName( "Loading" );

	Menu_Update( menu );
	Menu_Paint( menu, qtrue );
}

float CG_GetValue( int ownerDraw )
{
	playerState_t *ps;
	weapon_t      weapon;

	ps = &cg.snap->ps;
	weapon = BG_GetPlayerWeapon( ps );

	switch ( ownerDraw )
	{
		case CG_PLAYER_AMMO_VALUE:
			if ( weapon )
			{
				return ps->ammo;
			}

			break;

		case CG_PLAYER_CLIPS_VALUE:
			if ( weapon )
			{
				return ps->clips;
			}

			break;

		case CG_PLAYER_HEALTH:
			return ps->stats[ STAT_HEALTH ];

		default:
			break;
	}

	return -1;
}

const char *CG_GetKillerText( void )
{
	const char *s = "";

	if ( cg.killerName[ 0 ] )
	{
		s = va( "Fragged by %s", cg.killerName );
	}

	return s;
}

static void CG_DrawKiller( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	// fragged by ... line
	if ( cg.killerName[ 0 ] )
	{
		int x = rect->x + rect->w / 2;
		UI_Text_Paint( x - UI_Text_Width( CG_GetKillerText(), scale ) / 2,
		               rect->y + rect->h, scale, color, CG_GetKillerText(), 0, textStyle );
	}
}

#define SPECTATORS_PIXELS_PER_SECOND 30.0f

/*
==================
CG_DrawTeamSpectators
==================
*/
static void CG_DrawTeamSpectators( rectDef_t *rect, float scale, int textvalign, vec4_t color )
{
	float y;
	char  *text = cg.spectatorList;
	float textWidth = UI_Text_Width( text, scale );

	CG_AlignText( rect, text, scale, 0.0f, 0.0f, ALIGN_LEFT, textvalign, NULL, &y );

	if ( textWidth > rect->w )
	{
		// The text is too wide to fit, so scroll it
		int now = trap_Milliseconds();
		int delta = now - cg.spectatorTime;

		CG_SetClipRegion( rect->x, rect->y, rect->w, rect->h );

		UI_Text_Paint( rect->x - cg.spectatorOffset, y, scale, color, text, 0, 0 );
		UI_Text_Paint( rect->x + textWidth - cg.spectatorOffset, y, scale, color, text, 0, 0 );

		CG_ClearClipRegion();

		cg.spectatorOffset += ( delta / 1000.0f ) * SPECTATORS_PIXELS_PER_SECOND;

		while ( cg.spectatorOffset > textWidth )
		{
			cg.spectatorOffset -= textWidth;
		}

		cg.spectatorTime = now;
	}
	else
	{
		UI_Text_Paint( rect->x, y, scale, color, text, 0, 0 );
	}
}

#define FOLLOWING_STRING "following "
#define CHASING_STRING   "chasing "

/*
==================
CG_DrawFollow
==================
*/
static void CG_DrawFollow( rectDef_t *rect, float text_x, float text_y,
                           vec4_t color, float scale, int textalign, int textvalign, int textStyle )
{
	float tx, ty;

	if ( cg.snap && cg.snap->ps.pm_flags & PMF_FOLLOW )
	{
		char buffer[ MAX_STRING_CHARS ];

		if ( !cg.chaseFollow )
		{
			strcpy( buffer, FOLLOWING_STRING );
		}
		else
		{
			strcpy( buffer, CHASING_STRING );
		}

		strcat( buffer, cgs.clientinfo[ cg.snap->ps.clientNum ].name );

		CG_AlignText( rect, buffer, scale, 0, 0, textalign, textvalign, &tx, &ty );
		UI_Text_Paint( text_x + tx, text_y + ty, scale, color, buffer, 0, textStyle );
	}
}

/*
==================
CG_DrawTeamLabel
==================
*/
static void CG_DrawTeamLabel( rectDef_t *rect, team_t team, float text_x, float text_y,
                              vec4_t color, float scale, int textalign, int textvalign, int textStyle )
{
	const char *teamName;
	float tx, ty;

	switch ( team )
	{
		case TEAM_ALIENS:
			teamName = _("Aliens");
			break;

		case TEAM_HUMANS:
			teamName = _("Humans");
			break;

		default:
			teamName = "";
			break;
	}

	CG_AlignText( rect, teamName, scale, 0.0f, 0.0f, textalign, textvalign, &tx, &ty );
	UI_Text_Paint( text_x + tx, text_y + ty, scale, color, teamName, 0, textStyle );
}

/*
==================
CG_DrawMomentum
==================
*/
static void CG_DrawMomentum( rectDef_t *rect, float text_x, float text_y,
                               vec4_t color, float scale, int textalign, int textvalign, int textStyle )
{
	char   s[ MAX_TOKEN_CHARS ];
	float  tx, ty, momentum;
	team_t team;

	if ( cg.intermissionStarted )
	{
		return;
	}

	team = (team_t) cg.snap->ps.persistant[ PERS_TEAM ];

	if ( team <= TEAM_NONE || team >= NUM_TEAMS )
	{
		return;
	}

	momentum = cg.predictedPlayerState.persistant[ PERS_MOMENTUM ] / 10.0f;

	Com_sprintf( s, MAX_TOKEN_CHARS, _("%.1f momentum"), momentum );

	CG_AlignText( rect, s, scale, 0.0f, 0.0f, textalign, textvalign, &tx, &ty );

	UI_Text_Paint( text_x + tx, text_y + ty, scale, color, s, 0, textStyle );
}

/*
==================
CG_DrawFPS
==================
*/
#define FPS_FRAMES 20
#define FPS_STRING "fps"
static void CG_DrawFPS( rectDef_t *rect, float scale, vec4_t color,
                        int textalign, int textvalign, int textStyle,
                        qboolean scalableText )
{
	char       *s;
	float      tx = rect->x, ty = rect->y;
	static int previousTimes[ FPS_FRAMES ];
	static int index;
	int        i, total;
	int        fps;
	static int previous;
	int        t, frameTime;
	float      maxX;

	if ( !cg_drawFPS.integer )
	{
		return;
	}

	// don't use serverTime, because that will be drifting to
	// correct for Internet lag changes, timescales, timedemos, etc.
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[ index % FPS_FRAMES ] = frameTime;
	index++;

	if ( index > FPS_FRAMES )
	{
		// average multiple frames together to smooth changes out a bit
		total = 0;

		for ( i = 0; i < FPS_FRAMES; i++ )
		{
			total += previousTimes[ i ];
		}

		if ( !total )
		{
			total = 1;
		}

		fps = 1000 * FPS_FRAMES / total;
	}
	else
		fps = 0;

	s = va( "%d %s", fps, FPS_STRING );
	maxX = rect->x + rect->w;

	if ( UI_Text_Width( s, scale ) < rect->w && scalableText )
	{
		CG_AlignText( rect, s, scale, 0, 0, textalign, textvalign, &tx, &ty );
		UI_Text_Paint( tx, ty, scale, color, s, 0, textStyle );
	}
	else if ( UI_Text_Width( s, scale ) >= rect->w && scalableText )
	{
		CG_AlignText( rect, s, scale, 0, 0, textalign, textvalign, &tx, &ty );
		UI_Text_Paint_Limit( &maxX, tx, ty, scale, color, s, 0 );
	}
	else
	{
		trap_R_SetColor( color );
		CG_DrawField( rect->x, rect->y, 3, rect->w / 3, rect->h, fps );
		trap_R_SetColor( NULL );
	}
}

/*
=================
CG_DrawTimerMins
=================
*/
static void CG_DrawTimerMins( rectDef_t *rect, vec4_t color )
{
	int mins, seconds;
	int msec;

	if ( !cg_drawTimer.integer )
	{
		return;
	}

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;

	trap_R_SetColor( color );
	CG_DrawField( rect->x, rect->y, 3, rect->w / 3, rect->h, mins );
	trap_R_SetColor( NULL );
}

/*
=================
CG_DrawTimerSecs
=================
*/
static void CG_DrawTimerSecs( rectDef_t *rect, vec4_t color )
{
	int mins, seconds;
	int msec;

	if ( !cg_drawTimer.integer )
	{
		return;
	}

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;

	trap_R_SetColor( color );
	CG_DrawFieldPadded( rect->x, rect->y, 2, rect->w / 2, rect->h, seconds );
	trap_R_SetColor( NULL );
}

/*
=================
CG_DrawTimer
=================
*/
static void CG_DrawTimer( rectDef_t *rect, float scale, vec4_t color,
                          int textalign, int textvalign, int textStyle )
{
	char  *s;
	float tx = rect->x, ty = rect->y;
	int   mins, seconds, tens;
	int   msec;
	float maxX;

	if ( !cg_drawTimer.integer )
	{
		return;
	}

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%d:%d%d", mins, tens, seconds );

	if ( UI_Text_Width( s, scale ) < rect->w )
	{
		CG_AlignText( rect, s, scale, 0, 0, textalign, textvalign, &tx, &ty );
		UI_Text_Paint( tx, ty, scale, color, s, 0, textStyle );
	}
	else
	{
		CG_AlignText( rect, s, scale, 0, 0, textalign, textvalign, &tx, &ty );
		UI_Text_Paint_Limit( &maxX, tx, ty, scale, color, s, 0 );
	}
}

/*
=================
CG_DrawTeamOverlay
=================
*/

typedef enum
{
  TEAMOVERLAY_OFF,
  TEAMOVERLAY_ALL,
  TEAMOVERLAY_SUPPORT,
  TEAMOVERLAY_NEARBY,
} teamOverlayMode_t;

typedef enum
{
  TEAMOVERLAY_SORT_NONE,
  TEAMOVERLAY_SORT_SCORE,
  TEAMOVERLAY_SORT_WEAPONCLASS,
} teamOverlaySort_t;

static int QDECL SortScore( const void *a, const void *b )
{
	int na = * ( int * ) a;
	int nb = * ( int * ) b;

	return ( cgs.clientinfo[ nb ].score - cgs.clientinfo[ na ].score );
}

static int QDECL SortWeaponClass( const void *a, const void *b )
{
	int          out;
	clientInfo_t *ca = cgs.clientinfo + * ( int * ) a;
	clientInfo_t *cb = cgs.clientinfo + * ( int * ) b;

	out = cb->curWeaponClass - ca->curWeaponClass;

	// We want grangers on top. ckits are already on top without the special case.
	if ( ca->team == TEAM_ALIENS )
	{
		if ( ca->curWeaponClass == PCL_ALIEN_BUILDER0_UPG ||
		     cb->curWeaponClass == PCL_ALIEN_BUILDER0_UPG ||
		     ca->curWeaponClass == PCL_ALIEN_BUILDER0 ||
		     cb->curWeaponClass == PCL_ALIEN_BUILDER0 )
		{
			out = -out;
		}
	}

	return ( out );
}

static void CG_DrawTeamOverlay( rectDef_t *rect, float scale, vec4_t color )
{
	char              *s;
	int               i;
	float             x = rect->x;
	float             y;
	clientInfo_t      *ci, *pci;
	vec4_t            tcolor;
	float             iconSize = rect->h / 8.0f;
	float             leftMargin = 4.0f;
	float             iconTopMargin = 2.0f;
	float             midSep = 2.0f;
	float             backgroundWidth = rect->w;
	float             fontScale = 0.30f;
	float             vPad = 0.0f;
	float             nameWidth = 0.5f * rect->w;
	char              name[ MAX_NAME_LENGTH + 2 ];
	int               maxDisplayCount = 0;
	int               displayCount = 0;
	float             nameMaxX, nameMaxXCp;
	float             maxX = rect->x + rect->w;
	float             maxXCp = maxX;
	weapon_t          curWeapon = WP_NONE;
	teamOverlayMode_t mode = (teamOverlayMode_t) cg_drawTeamOverlay.integer;
	teamOverlaySort_t sort = (teamOverlaySort_t) cg_teamOverlaySortMode.integer;
	int               displayClients[ MAX_CLIENTS ];

	if ( cg.predictedPlayerState.pm_type == PM_SPECTATOR )
	{
		return;
	}

	if ( mode == TEAMOVERLAY_OFF || !cg_teamOverlayMaxPlayers.integer )
	{
		return;
	}

	if ( !cgs.teamInfoReceived )
	{
		return;
	}

	if ( cg.showScores ||
	     cg.predictedPlayerState.pm_type == PM_INTERMISSION )
	{
		return;
	}

	pci = cgs.clientinfo + cg.snap->ps.clientNum;

	if ( mode == TEAMOVERLAY_ALL || mode == TEAMOVERLAY_SUPPORT )
	{
		for ( i = 0; i < MAX_CLIENTS; i++ )
		{
			ci = cgs.clientinfo + i;

			if ( ci->infoValid && pci != ci && ci->team == pci->team )
			{
				if ( mode == TEAMOVERLAY_ALL )
				{
					displayClients[ maxDisplayCount++ ] = i;
				}
				else
				{
					if ( ci->curWeaponClass == PCL_ALIEN_BUILDER0 ||
					     ci->curWeaponClass == PCL_ALIEN_BUILDER0_UPG ||
					     ci->curWeaponClass == PCL_ALIEN_LEVEL1 ||
					     ci->curWeaponClass == WP_HBUILD )
					{
						displayClients[ maxDisplayCount++ ] = i;
					}
				}
			}
		}
	}
	else // find nearby
	{
		for ( i = 0; i < cg.snap->numEntities; i++ )
		{
			centity_t *cent = &cg_entities[ cg.snap->entities[ i ].number ];
			vec3_t    relOrigin = { 0.0f, 0.0f, 0.0f };
			int       team = cent->currentState.misc & 0x00FF;

			if ( cent->currentState.eType != ET_PLAYER ||
			     team != pci->team ||
			     cent->currentState.eFlags & EF_DEAD )
			{
				continue;
			}

			VectorSubtract( cent->lerpOrigin, cg.predictedPlayerState.origin, relOrigin );

			if ( VectorLength( relOrigin ) < RADAR_RANGE )
			{
				displayClients[ maxDisplayCount++ ] = cg.snap->entities[ i ].number;
			}
		}
	}

	// Sort
	if ( sort == TEAMOVERLAY_SORT_SCORE )
	{
		qsort( displayClients, maxDisplayCount,
		       sizeof( displayClients[ 0 ] ), SortScore );
	}
	else if ( sort == TEAMOVERLAY_SORT_WEAPONCLASS )
	{
		qsort( displayClients, maxDisplayCount,
		       sizeof( displayClients[ 0 ] ), SortWeaponClass );
	}

	if ( maxDisplayCount > cg_teamOverlayMaxPlayers.integer )
	{
		maxDisplayCount = cg_teamOverlayMaxPlayers.integer;
	}

	iconSize *= scale;
	leftMargin *= scale;
	iconTopMargin *= scale;
	midSep *= scale;
	backgroundWidth *= scale;
	fontScale *= scale;
	nameWidth *= scale;

	vPad = ( rect->h - ( ( float ) maxDisplayCount * iconSize ) ) / 2.0f;
	y = rect->y + vPad;

	tcolor[ 0 ] = 1.0f;
	tcolor[ 1 ] = 1.0f;
	tcolor[ 2 ] = 1.0f;
	tcolor[ 3 ] = color[ 3 ];

	for ( i = 0; i < MAX_CLIENTS && displayCount < maxDisplayCount; i++ )
	{
		ci = cgs.clientinfo + displayClients[ i ];

		if ( !ci->infoValid || pci == ci || ci->team != pci->team )
		{
			continue;
		}

		Com_sprintf( name, sizeof( name ), "%s^7", ci->name );

		trap_R_SetColor( color );
		CG_DrawPic( x, y, backgroundWidth,
		            iconSize, cgs.media.teamOverlayShader );
		trap_R_SetColor( tcolor );

		if ( ci->health <= 0 || !ci->curWeaponClass )
		{
			s = "";
		}
		else
		{
			if ( ci->team == TEAM_HUMANS )
			{
				curWeapon = (weapon_t) ci->curWeaponClass;
			}
			else if ( ci->team == TEAM_ALIENS )
			{
				curWeapon = BG_Class( ci->curWeaponClass )->startWeapon;
			}

			CG_DrawPic( x + leftMargin, y, iconSize, iconSize,
			            cg_weapons[ curWeapon ].weaponIcon );

			if ( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_HUMANS )
			{
				if ( ci->upgrade != UP_NONE )
				{
					CG_DrawPic( x + iconSize + leftMargin, y, iconSize,
					            iconSize, cg_upgrades[ ci->upgrade ].upgradeIcon );
				}
			}

			s = va( " [^%c%s%d^7] %s ^7%s",
			        CG_GetColorCharForHealth( displayClients[ i ] ),
			        &"  "[ci->health >= 100 ? 6 : ci->health >= 10 ? 3 : 0], // these are figure spaces, 3 bytes each
			        ci->health,
			        ( ci->team == TEAM_ALIENS )
			          ? va( "₠%.1f", (float) ci->credit / CREDITS_PER_EVO )
			          : va( "₢%d", ci->credit ),
			        CG_ConfigString( CS_LOCATIONS + ci->location ) );
		}

		trap_R_SetColor( NULL );
		nameMaxX = nameMaxXCp = x + 2.0f * iconSize +
		                        leftMargin + midSep + nameWidth;
		UI_Text_Paint_Limit( &nameMaxXCp, x + 2.0f * iconSize + leftMargin + midSep,
		                     y + iconSize - iconTopMargin, fontScale, tcolor, name,
		                     0 );

		maxXCp = maxX;

		UI_Text_Paint_Limit( &maxXCp, nameMaxX, y + iconSize - iconTopMargin,
		                     fontScale, tcolor, s, 0 );
		y += iconSize;
		displayCount++;
	}
}

/*
=================
CG_DrawClock
=================
*/
static void CG_DrawClock( rectDef_t *rect, float scale, vec4_t color,
                          int textalign, int textvalign, int textStyle )
{
	char    *s;
	float   tx, ty;
	float   maxX;
	qtime_t qt;

	if ( !cg_drawClock.integer )
	{
		return;
	}

	trap_RealTime( &qt );

	if ( cg_drawClock.integer == 2 )
	{
		s = va( "%02d%s%02d", qt.tm_hour, ( qt.tm_sec % 2 ) ? ":" : " ",
		        qt.tm_min );
	}
	else
	{
		char *pm = "am";
		int  h = qt.tm_hour;

		if ( h == 0 )
		{
			h = 12;
		}
		else if ( h == 12 )
		{
			pm = "pm";
		}
		else if ( h > 12 )
		{
			h -= 12;
			pm = "pm";
		}

		s = va( "%d%s%02d%s", h, ( qt.tm_sec % 2 ) ? ":" : " ", qt.tm_min, pm );
	}

	if ( UI_Text_Width( s, scale ) < rect->w )
	{
		CG_AlignText( rect, s, scale, 0, 0, textalign, textvalign, &tx, &ty );
		UI_Text_Paint( tx, ty, scale, color, s, 0, textStyle );
	}
	else
	{
		CG_AlignText( rect, s, scale, 0, 0, textalign, textvalign, &tx, &ty );
		UI_Text_Paint_Limit( &maxX, tx, ty, scale, color, s, 0 );
	}
}

/*
==================
CG_DrawSnapshot
==================
*/
static void CG_DrawSnapshot( rectDef_t *rect, float text_x, float text_y,
                             float scale, vec4_t color,
                             int textalign, int textvalign, int textStyle )
{
	char  *s;
	float tx, ty;

	if ( !cg_drawSnapshot.integer )
	{
		return;
	}

	s = va( "time:%d snap:%d cmd:%d", cg.snap->serverTime,
	        cg.latestSnapshotNum, cgs.serverCommandSequence );

	CG_AlignText( rect, s, scale, 0.0f, 0.0f, textalign, textvalign, &tx, &ty );

	UI_Text_Paint( text_x + tx, text_y + ty, scale, color, s, 0, textStyle );
}

/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define LAG_SAMPLES 128

typedef struct
{
	int frameSamples[ LAG_SAMPLES ];
	int frameCount;
	int snapshotFlags[ LAG_SAMPLES ];
	int snapshotSamples[ LAG_SAMPLES ];
	int snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void )
{
	int offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1 ) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
#define PING_FRAMES 40
void CG_AddLagometerSnapshotInfo( snapshot_t *snap )
{
	static int previousPings[ PING_FRAMES ];
	static int index;
	int        i;

	// dropped packet
	if ( !snap )
	{
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->snapFlags;
	lagometer.snapshotCount++;

	cg.ping = 0;

	if ( cg.snap )
	{
		previousPings[ index++ ] = cg.snap->ping;
		index = index % PING_FRAMES;

		for ( i = 0; i < PING_FRAMES; i++ )
		{
			cg.ping += previousPings[ i ];
		}

		cg.ping /= PING_FRAMES;
	}
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void )
{
	float      x, y;
	int        cmdNum;
	usercmd_t  cmd;
	const char *s;
	int        w;
	vec4_t     color = { 1.0f, 1.0f, 1.0f, 1.0f };

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );

	// special check for map_restart
	if ( cmd.serverTime <= cg.snap->ps.commandTime || cmd.serverTime > cg.time )
	{
		return;
	}

	// also add text in center of screen
	s = _("Connection Interrupted");
	w = UI_Text_Width( s, 0.7f );
	UI_Text_Paint( 320 - w / 2, 100, 0.7f, color, s, 0, ITEM_TEXTSTYLE_SHADOWED );

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 )
	{
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net",
							RSF_DEFAULT));
}

#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( rectDef_t *rect, float text_x, float text_y,
                              float scale, vec4_t textColor )
{
	int    a, x, y, i;
	float  v;
	float  ax, ay, aw, ah, mid, range;
	int    color;
	vec4_t adjustedColor;
	float  vscale;
	char   *ping;

	if ( cg.snap->ps.pm_type == PM_INTERMISSION )
	{
		return;
	}

	if ( !cg_lagometer.integer )
	{
		return;
	}

	if ( cg.demoPlayback )
	{
		return;
	}

	Vector4Copy( textColor, adjustedColor );
	adjustedColor[ 3 ] = 0.25f;

	trap_R_SetColor( adjustedColor );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.whiteShader );
	trap_R_SetColor( NULL );

	//
	// draw the graph
	//
	ax = x = rect->x;
	ay = y = rect->y;
	aw = rect->w;
	ah = rect->h;

	trap_R_SetColor( NULL );

	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0; a < aw; a++ )
	{
		i = ( lagometer.frameCount - 1 - a ) & ( LAG_SAMPLES - 1 );
		v = lagometer.frameSamples[ i ];
		v *= vscale;

		if ( v > 0 )
		{
			if ( color != 1 )
			{
				color = 1;
				trap_R_SetColor( g_color_table[ ColorIndex( COLOR_YELLOW ) ] );
			}

			if ( v > range )
			{
				v = range;
			}

			trap_R_DrawStretchPic( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
		else if ( v < 0 )
		{
			if ( color != 2 )
			{
				color = 2;
				trap_R_SetColor( g_color_table[ ColorIndex( COLOR_BLUE ) ] );
			}

			v = -v;

			if ( v > range )
			{
				v = range;
			}

			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0; a < aw; a++ )
	{
		i = ( lagometer.snapshotCount - 1 - a ) & ( LAG_SAMPLES - 1 );
		v = lagometer.snapshotSamples[ i ];

		if ( v > 0 )
		{
			if ( lagometer.snapshotFlags[ i ] & SNAPFLAG_RATE_DELAYED )
			{
				if ( color != 5 )
				{
					color = 5; // YELLOW for rate delay
					trap_R_SetColor( g_color_table[ ColorIndex( COLOR_YELLOW ) ] );
				}
			}
			else
			{
				if ( color != 3 )
				{
					color = 3;

					trap_R_SetColor( g_color_table[ ColorIndex( COLOR_GREEN ) ] );
				}
			}

			v = v * vscale;

			if ( v > range )
			{
				v = range;
			}

			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
		else if ( v < 0 )
		{
			if ( color != 4 )
			{
				color = 4; // RED for dropped snapshots
				trap_R_SetColor( g_color_table[ ColorIndex( COLOR_RED ) ] );
			}

			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer )
	{
		ping = "snc";
	}
	else
	{
		ping = va( "%d", cg.ping );
	}

	ax = rect->x + ( rect->w / 2.0f ) -
	     ( UI_Text_Width( ping, scale ) / 2.0f ) + text_x;
	ay = rect->y + ( rect->h / 2.0f ) +
	     ( UI_Text_Height( ping, scale ) / 2.0f ) + text_y;

	Vector4Copy( textColor, adjustedColor );
	adjustedColor[ 3 ] = 0.5f;
	UI_Text_Paint( ax, ay, scale, adjustedColor, ping, 0, ITEM_TEXTSTYLE_PLAIN );

	CG_DrawDisconnect();
}

#define SPEEDOMETER_NUM_SAMPLES 4096
#define SPEEDOMETER_NUM_DISPLAYED_SAMPLES 160
#define SPEEDOMETER_DRAW_TEXT   0x1
#define SPEEDOMETER_DRAW_GRAPH  0x2
#define SPEEDOMETER_IGNORE_Z    0x4
float speedSamples[ SPEEDOMETER_NUM_SAMPLES ];
int speedSampleTimes[ SPEEDOMETER_NUM_SAMPLES ];
// array indices
int   oldestSpeedSample = 0;
int   maxSpeedSample = 0;
int   maxSpeedSampleInWindow = 0;

/*
===================
CG_AddSpeed

append a speed to the sample history
===================
*/
void CG_AddSpeed( void )
{
	float  speed;
	vec3_t vel;
	int    windowTime;
	qboolean newSpeedGteMaxSpeed, newSpeedGteMaxSpeedInWindow;

	VectorCopy( cg.snap->ps.velocity, vel );

	if ( cg_drawSpeed.integer & SPEEDOMETER_IGNORE_Z )
	{
		vel[ 2 ] = 0;
	}

	speed = VectorLength( vel );

	windowTime = cg_maxSpeedTimeWindow.integer;
	if ( windowTime < 0 )
	{
		windowTime = 0;
	}
	else if ( windowTime > SPEEDOMETER_NUM_SAMPLES * 1000 )
	{
		windowTime = SPEEDOMETER_NUM_SAMPLES * 1000;
	}

	if ( ( newSpeedGteMaxSpeed = ( speed >= speedSamples[ maxSpeedSample ] ) ) )
	{
		maxSpeedSample = oldestSpeedSample;
	}

	if ( ( newSpeedGteMaxSpeedInWindow = ( speed >= speedSamples[ maxSpeedSampleInWindow ] ) ) )
	{
		maxSpeedSampleInWindow = oldestSpeedSample;
	}

	speedSamples[ oldestSpeedSample ] = speed;

	speedSampleTimes[ oldestSpeedSample ] = cg.time;

	if ( !newSpeedGteMaxSpeed && maxSpeedSample == oldestSpeedSample )
	{
		// if old max was overwritten find a new one
		int i;

		for ( maxSpeedSample = 0, i = 1; i < SPEEDOMETER_NUM_SAMPLES; i++ )
		{
			if ( speedSamples[ i ] > speedSamples[ maxSpeedSample ] )
			{
				maxSpeedSample = i;
			}
		}
	}

	if ( !newSpeedGteMaxSpeedInWindow && ( maxSpeedSampleInWindow == oldestSpeedSample ||
	     cg.time - speedSampleTimes[ maxSpeedSampleInWindow ] > windowTime ) )
	{
		int i;
		do
		{
			maxSpeedSampleInWindow = ( maxSpeedSampleInWindow + 1 ) % SPEEDOMETER_NUM_SAMPLES;
		} while( cg.time - speedSampleTimes[ maxSpeedSampleInWindow ] > windowTime );
		for ( i = maxSpeedSampleInWindow; ; i = ( i + 1 ) % SPEEDOMETER_NUM_SAMPLES )
		{
			if ( speedSamples[ i ] > speedSamples[ maxSpeedSampleInWindow ] )
			{
				maxSpeedSampleInWindow = i;
			}
			if ( i == oldestSpeedSample )
			{
				break;
			}
		}
	}

	oldestSpeedSample = ( oldestSpeedSample + 1 ) % SPEEDOMETER_NUM_SAMPLES;
}

#define SPEEDOMETER_MIN_RANGE 900
#define SPEED_MED             1000.f
#define SPEED_FAST            1600.f

/*
===================
CG_DrawSpeedGraph
===================
*/
static void CG_DrawSpeedGraph( rectDef_t *rect, vec4_t foreColor,
                               vec4_t backColor )
{
	int          i;
	float        val, max, top;
	// colour of graph is interpolated between these values
	const vec3_t slow = { 0.0, 0.0, 1.0 };
	const vec3_t medium = { 0.0, 1.0, 0.0 };
	const vec3_t fast = { 1.0, 0.0, 0.0 };
	vec4_t       color;

	max = speedSamples[ maxSpeedSample ];

	if ( max < SPEEDOMETER_MIN_RANGE )
	{
		max = SPEEDOMETER_MIN_RANGE;
	}

	trap_R_SetColor( backColor );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.whiteShader );

	Vector4Copy( foreColor, color );

	for ( i = 1; i < SPEEDOMETER_NUM_DISPLAYED_SAMPLES; i++ )
	{
		val = speedSamples[ ( oldestSpeedSample + i + SPEEDOMETER_NUM_SAMPLES -
		                      SPEEDOMETER_NUM_DISPLAYED_SAMPLES ) % SPEEDOMETER_NUM_SAMPLES ];

		if ( val < SPEED_MED )
		{
			VectorLerpTrem( val / SPEED_MED, slow, medium, color );
		}
		else if ( val < SPEED_FAST )
		{
			VectorLerpTrem( ( val - SPEED_MED ) / ( SPEED_FAST - SPEED_MED ),
			                medium, fast, color );
		}
		else
		{
			VectorCopy( fast, color );
		}

		trap_R_SetColor( color );
		top = rect->y + ( 1 - val / max ) * rect->h;
		CG_DrawPic( rect->x + ( i / ( float ) SPEEDOMETER_NUM_DISPLAYED_SAMPLES ) * rect->w, top,
		            rect->w / ( float ) SPEEDOMETER_NUM_DISPLAYED_SAMPLES, val * rect->h / max,
		            cgs.media.whiteShader );
	}

	trap_R_SetColor( NULL );
}

/*
===================
CG_DrawSpeedText
===================
*/
static void CG_DrawSpeedText( rectDef_t *rect, float scale, vec4_t foreColor )
{
	char   speedstr[ 16 ];
	float  val;
	vec4_t color;

	VectorCopy( foreColor, color );
	color[ 3 ] = 1;

	if ( cg.predictedPlayerState.clientNum == cg.clientNum )
	{
		vec3_t vel;
		VectorCopy( cg.predictedPlayerState.velocity, vel );

		if ( cg_drawSpeed.integer & SPEEDOMETER_IGNORE_Z )
		{
			vel[ 2 ] = 0;
		}

		val = VectorLength( vel );
	}
	else
	{
		val = speedSamples[ ( oldestSpeedSample - 1 + SPEEDOMETER_NUM_SAMPLES ) % SPEEDOMETER_NUM_SAMPLES ];
	}

	Com_sprintf( speedstr, sizeof( speedstr ), "%d %d", ( int ) speedSamples[ maxSpeedSampleInWindow ], ( int ) val );

	UI_Text_Paint(
	  rect->x + ( rect->w - UI_Text_Width( speedstr, scale ) ) / 2.0f,
	  rect->y + ( rect->h + UI_Text_Height( speedstr, scale ) ) / 2.0f,
	  scale, color, speedstr, 0, ITEM_TEXTSTYLE_PLAIN );
}

/*
===================
CG_DrawSpeed
===================
*/
static void CG_DrawSpeed( rectDef_t *rect, float scale, vec4_t foreColor, vec4_t backColor )
{
	if ( cg_drawSpeed.integer & SPEEDOMETER_DRAW_GRAPH )
	{
		CG_DrawSpeedGraph( rect, foreColor, backColor );
	}

	if ( cg_drawSpeed.integer & SPEEDOMETER_DRAW_TEXT )
	{
		CG_DrawSpeedText( rect, scale, foreColor );
	}
}

/*
===================
CG_DrawConsole
===================
*/
static void CG_DrawConsole( rectDef_t *rect, float text_x, float text_y, vec4_t color,
                            float scale, int textalign, int textvalign, int textStyle )
{
	UI_DrawTextBlock( rect, text_x, text_y, color, scale, textalign, textvalign, textStyle, cg.consoleText );
}

/*
===================
CG_DrawTutorial
===================
*/
static void CG_DrawTutorial( rectDef_t *rect, float text_x, float text_y, vec4_t color,
                             float scale, int textalign, int textvalign, int textStyle )
{
	if ( !cg_tutorial.integer )
	{
		return;
	}

	UI_DrawTextBlock( rect, text_x, text_y, color, scale, textalign, textvalign, textStyle, CG_TutorialText() );
}

/*
===================
CG_DrawWeaponIcon
===================
*/
void CG_DrawWeaponIcon( rectDef_t *rect, vec4_t color )
{
	int           maxAmmo;
	playerState_t *ps;
	weapon_t      weapon;
	vec4_t        localColor;

	ps = &cg.snap->ps;
	weapon = BG_GetPlayerWeapon( ps );

	maxAmmo = BG_Weapon( weapon )->maxAmmo;

	// don't display if dead or no weapon
	if ( cg.predictedPlayerState.stats[ STAT_HEALTH ] <= 0 || weapon == WP_NONE )
	{
		return;
	}


	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS )
	{
		CG_Error( "CG_DrawWeaponIcon: weapon out of range: %d", weapon );
	}

	if ( !cg_weapons[ weapon ].registered )
	{
		Com_Printf( S_WARNING "CG_DrawWeaponIcon: weapon %d (%s) "
		            "is not registered\n", weapon, BG_Weapon( weapon )->name );
		return;
	}

	Vector4Copy( color, localColor );

	if ( ps->clips == 0 && !BG_Weapon( weapon )->infiniteAmmo )
	{
		float ammoPercent = ( float ) ps->ammo / ( float ) maxAmmo;

		if ( ammoPercent < 0.33f )
		{
			localColor[ 0 ] = 1.0f;
			localColor[ 1 ] = localColor[ 2 ] = 0.0f;
		}
	}

	if ( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_ALIENS &&
	     !BG_AlienCanEvolve( cg.predictedPlayerState.stats[ STAT_CLASS ], ps->persistant[ PERS_CREDIT ] ) )
	{
		if ( cg.time - cg.lastEvolveAttempt <= NO_CREDITS_TIME )
		{
			if ( ( ( cg.time - cg.lastEvolveAttempt ) / 300 ) % 2 )
			{
				localColor[ 3 ] = 0.0f;
			}
		}
	}

	trap_R_SetColor( localColor );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h,
	            cg_weapons[ weapon ].weaponIcon );
	trap_R_SetColor( NULL );
}

/*
================================================================================

CROSSHAIR

================================================================================
*/

#define CROSSHAIR_INDICATOR_HITFADE 500

static void CG_DrawCrosshairIndicator( rectDef_t *rect, vec4_t color )
{
	float        x, y, w, h, dim;
	qhandle_t    indicator;
	vec4_t       drawColor, baseColor;
	weapon_t     weapon;
	weaponInfo_t *wi;
	qboolean     onRelevantEntity;

	if ( ( !cg_drawCrosshairHit.integer && !cg_drawCrosshairFriendFoe.integer ) ||
	     cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT ||
	     cg.snap->ps.pm_type == PM_INTERMISSION ||
	     cg.renderingThirdPerson )
	{
		return;
	}

	weapon = BG_GetPlayerWeapon( &cg.snap->ps );
	wi = &cg_weapons[ weapon ];
	indicator = wi->crossHairIndicator;

	if ( !indicator )
	{
		return;
	}

	// set base color (friend/foe detection)
	if ( cg_drawCrosshairFriendFoe.integer >= CROSSHAIR_ALWAYSON ||
	     ( cg_drawCrosshairFriendFoe.integer >= CROSSHAIR_RANGEDONLY && BG_Weapon( weapon )->longRanged ) )
	{
		if ( cg.crosshairFoe )
		{
			Vector4Copy( colorRed, baseColor );
			baseColor[ 3 ] = color[ 3 ] * 0.75f;
			onRelevantEntity = qtrue;
		}
		else if ( cg.crosshairFriend )
		{
			Vector4Copy( colorGreen, baseColor );
			baseColor[ 3 ] = color[ 3 ] * 0.75f;
			onRelevantEntity = qtrue;
		}
		else
		{
			Vector4Set( baseColor, 1.0f, 1.0f, 1.0f, 0.0f );
			onRelevantEntity = qfalse;
		}
	}
	else
	{
		Vector4Set( baseColor, 1.0f, 1.0f, 1.0f, 0.0f );
		onRelevantEntity = qfalse;
	}

	// add hit color
	if ( cg_drawCrosshairHit.integer && cg.hitTime + CROSSHAIR_INDICATOR_HITFADE > cg.time )
	{
		dim = ( ( cg.hitTime + CROSSHAIR_INDICATOR_HITFADE ) - cg.time ) / ( float )CROSSHAIR_INDICATOR_HITFADE;

		Vector4Lerp( dim, baseColor, colorWhite, drawColor );
	}
	else if ( !onRelevantEntity )
	{
		return;
	}
	else
	{
		Vector4Copy( baseColor, drawColor );
	}

	// set size
	w = h = wi->crossHairSize * cg_crosshairSize.value;
	w *= cgDC.aspectScale;

	// HACK: This ignores the width/height of the rect (does it?)
	x = rect->x + ( rect->w / 2 ) - ( w / 2 );
	y = rect->y + ( rect->h / 2 ) - ( h / 2 );

	// draw
	trap_R_SetColor( drawColor );
	CG_DrawPic( x, y, w, h, indicator );
	trap_R_SetColor( NULL );
}

/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair( rectDef_t *rect, vec4_t color )
{
	float        w, h;
	qhandle_t    crosshair;
	float        x, y;
	weaponInfo_t *wi;
	weapon_t     weapon;

	vec4_t       localColor;

	weapon = BG_GetPlayerWeapon( &cg.snap->ps );

	if ( !cg_drawCrosshair.integer )
	{
		return;
	}

	if ( cg_drawCrosshair.integer <= CROSSHAIR_RANGEDONLY &&
	     !BG_Weapon( weapon )->longRanged )
	{
		return;
	}

	if ( cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
	{
		return;
	}

	if ( cg.renderingThirdPerson )
	{
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION )
	{
		return;
	}

	wi = &cg_weapons[ weapon ];

	w = h = wi->crossHairSize * cg_crosshairSize.value;
	w *= cgDC.aspectScale;

	// HACK: This ignores the width/height of the rect (does it?)
	x = rect->x + ( rect->w / 2 ) - ( w / 2 );
	y = rect->y + ( rect->h / 2 ) - ( h / 2 );

	crosshair = wi->crossHair;

	Vector4Copy( color, localColor );

	if ( crosshair )
	{
		trap_R_SetColor( localColor );
		CG_DrawPic( x, y, w, h, crosshair );
		trap_R_SetColor( NULL );
	}
}

/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void )
{
	trace_t       trace;
	vec3_t        start, end;
	team_t        ownTeam, targetTeam;
	entityState_t *targetState;

	cg.crosshairFriend = qfalse;
	cg.crosshairFoe    = qfalse;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[ 0 ], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
	          cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY );

	// ignore special entities
	if ( trace.entityNum > ENTITYNUM_MAX_NORMAL )
	{
		return;
	}

	// ignore targets in fog
	if ( trap_CM_PointContents( trace.endpos, 0 ) & CONTENTS_FOG )
	{
		return;
	}

	ownTeam = (team_t) cg.snap->ps.persistant[ PERS_TEAM ];
	targetState = &cg_entities[ trace.entityNum ].currentState;

	if ( trace.entityNum >= MAX_CLIENTS )
	{
		// we have a non-client entity

		// set friend/foe if it's a living buildable
		if ( targetState->eType == ET_BUILDABLE && targetState->generic1 > 0 )
		{
			targetTeam = BG_Buildable( targetState->modelindex )->team;

			if ( targetTeam == ownTeam )
			{
				cg.crosshairFriend = qtrue;
			}
			else if ( targetTeam != TEAM_NONE )
			{
				cg.crosshairFoe = qtrue;
			}
		}

		// set more stuff if requested
		if ( cg_drawEntityInfo.integer && targetState->eType )
		{
			cg.crosshairClientNum = trace.entityNum;
			cg.crosshairClientTime = cg.time;
		}
	}
	else
	{
		// we have a client entitiy
		targetTeam = cgs.clientinfo[ trace.entityNum ].team;

		// only react to living clients
		if ( targetState->generic1 > 0 )
		{
			// set friend/foe
			if ( targetTeam == ownTeam && ownTeam != TEAM_NONE )
			{
				cg.crosshairFriend = qtrue;

				// only set this for friendly clients as it triggers name display
				cg.crosshairClientNum = trace.entityNum;
				cg.crosshairClientTime = cg.time;
			}
			else if ( targetTeam != TEAM_NONE )
			{
				cg.crosshairFoe = qtrue;

				if ( ownTeam == TEAM_NONE )
				{
					// spectating, so show the name
					cg.crosshairClientNum = trace.entityNum;
					cg.crosshairClientTime = cg.time;
				}
			}
		}
	}
}

/*
=====================
CG_DrawLocation
=====================
*/
static void CG_DrawLocation( rectDef_t *rect, float scale, int textalign, vec4_t color )
{
	const char *location;
	centity_t  *locent;
	float      maxX;
	float      tx = rect->x, ty = rect->y;

	if ( cg.intermissionStarted )
	{
		return;
	}

	maxX = rect->x + rect->w;

	locent = CG_GetPlayerLocation();

	if ( locent )
	{
		location = CG_ConfigString( CS_LOCATIONS + locent->currentState.generic1 );
	}
	else
	{
		location = CG_ConfigString( CS_LOCATIONS );
	}

	// need to skip horiz. align if it's too long, but valign must be run either way
	if ( UI_Text_Width( location, scale ) < rect->w )
	{
		CG_AlignText( rect, location, scale, 0.0f, 0.0f, textalign, VALIGN_CENTER, &tx, &ty );
		UI_Text_Paint( tx, ty, scale, color, location, 0, ITEM_TEXTSTYLE_PLAIN );
	}
	else
	{
		CG_AlignText( rect, location, scale, 0.0f, 0.0f, ALIGN_NONE, VALIGN_CENTER, &tx, &ty );
		UI_Text_Paint_Limit( &maxX, tx, ty, scale, color, location, 0 );
	}

	trap_R_SetColor( NULL );
}

/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( rectDef_t *rect, float scale, int textStyle )
{
	float *color;
	char  *name;
	float w, x;

	if ( !cg_drawCrosshairNames.integer )
	{
		return;
	}

	if ( cg.renderingThirdPerson )
	{
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, CROSSHAIR_CLIENT_TIMEOUT );

	if ( !color )
	{
		trap_R_SetColor( NULL );
		return;
	}

	if( cg_drawEntityInfo.integer )
	{
		name = va( "(" S_COLOR_CYAN "%s" S_COLOR_WHITE "|" S_COLOR_CYAN "#%d" S_COLOR_WHITE ")",
				Com_EntityTypeName( cg_entities[cg.crosshairClientNum].currentState.eType ), cg.crosshairClientNum );
	}
	else if ( cg_drawCrosshairNames.integer >= 2 )
	{
		name = va( "%2i: %s", cg.crosshairClientNum, cgs.clientinfo[ cg.crosshairClientNum ].name );
	}
	else
	{
		name = cgs.clientinfo[ cg.crosshairClientNum ].name;
	}

	// add health from overlay info to the crosshair client name
	if ( cg_teamOverlayUserinfo.integer &&
	     cg.snap->ps.persistant[ PERS_TEAM ] != TEAM_NONE &&
	     cgs.teamInfoReceived &&
	     cgs.clientinfo[ cg.crosshairClientNum ].health > 0 )
	{
		name = va( "%s ^7[^%c%d^7]", name,
		           CG_GetColorCharForHealth( cg.crosshairClientNum ),
		           cgs.clientinfo[ cg.crosshairClientNum ].health );
	}

	w = UI_Text_Width( name, scale );
	x = rect->x + rect->w / 2.0f;
	UI_Text_Paint( x - w / 2.0f, rect->y + rect->h, scale, color, name, 0, textStyle );
	trap_R_SetColor( NULL );
}

/*
============
CG_DrawStack

Helper function: draws a "stack" of <val> boxes in a row or column big
enough to fit <max> boxes. <fill> sets the proportion of space in the stack
with box drawn on it - i.e. lower values will result in thinner boxes.
If the boxes are too thin, or if fill is specified as 0, they will be merged
into a single progress bar.
============
*/
static void CG_DrawStack( rectDef_t *rect, vec4_t color, float fill,
                          int align, int valign, float val, int max )
{
	int      i;
	float    each, frac;
	float    nudge;
	float    fmax = max; // we don't want integer division
	qboolean vertical; // a stack taller than it is wide is drawn vertically
	vec4_t   localColor;

	// so that the vertical and horizontal bars can share code, abstract the
	// longer dimension and the alignment parameter
	int length, lalign;
#define LALIGN_TOPLEFT     0
#define LALIGN_CENTER      1
#define LALIGN_BOTTOMRIGHT 2

	if ( val <= 0 || max <= 0 )
	{
		return; // nothing to draw
	}

	trap_R_SetColor( color );

	if ( rect->h >= rect->w )
	{
		vertical = qtrue;
		length = rect->h;

		switch ( valign )
		{
			case VALIGN_TOP:
				lalign = LALIGN_TOPLEFT;
				break;

			case VALIGN_CENTER:
				lalign = LALIGN_CENTER;
				break;

			case VALIGN_BOTTOM:
				lalign = LALIGN_BOTTOMRIGHT;
				break;

			default:
				CG_Error( "CG_DrawStack: valign value %d not recognised", valign );
		}
	}
	else
	{
		vertical = qfalse;
		length = rect->w;

		switch ( align )
		{
			case ALIGN_LEFT:
				lalign = LALIGN_TOPLEFT;
				break;

			case ALIGN_CENTER:
				lalign = LALIGN_CENTER;
				break;

			case ALIGN_RIGHT:
				lalign = LALIGN_BOTTOMRIGHT;
				break;

			default:
				CG_Error( "CG_DrawStack: align value %d not recognised", align );
		}
	}

	// the filled proportion of the length, divided into max bits
	each = fill * length / fmax;

	// if the scaled length of each segment is too small, do a single bar
	if ( each * ( vertical ? cgs.screenYScale : cgs.screenXScale ) < 4.f )
	{
		float loff;

		switch ( lalign )
		{
			case LALIGN_TOPLEFT:
			default:
				loff = 0;
				break;

			case LALIGN_CENTER:
				loff = length * ( 1 - val / fmax ) / 2;
				break;

			case LALIGN_BOTTOMRIGHT:
				loff = length * ( 1 - val / fmax );
		}

		if ( vertical )
		{
			CG_DrawPic( rect->x, rect->y + loff, rect->w, rect->h * val / fmax,
			            cgs.media.whiteShader );
		}
		else
		{
			CG_DrawPic( rect->x + loff, rect->y, rect->w * val / fmax, rect->h,
			            cgs.media.whiteShader );
		}

		trap_R_SetColor( NULL );
		return;
	}

	// the code would normally leave a bit of space after every square, but this
	// leaves a space on the end, too: nudge divides that space among the blocks
	// so that the beginning and end line up perfectly
	if ( fmax > 1 )
	{
		nudge = ( 1 - fill ) / ( fmax - 1 );
	}
	else
	{
		nudge = 0;
	}

	frac = val - ( int ) val;

	for ( i = ( int ) val - 1; i >= 0; i-- )
	{
		float start;

		switch ( lalign )
		{
			case LALIGN_TOPLEFT:
				start = ( i * ( 1 + nudge ) + frac ) / fmax;
				break;

			case LALIGN_CENTER:

				// TODO (fallthrough for now)
			default:
			case LALIGN_BOTTOMRIGHT:
				start = 1 - ( val - i - ( i + fmax - val ) * nudge ) / fmax;
				break;
		}

		if ( vertical )
		{
			CG_DrawPic( rect->x, rect->y + rect->h * start, rect->w, each,
			            cgs.media.whiteShader );
		}
		else
		{
			CG_DrawPic( rect->x + rect->w * start, rect->y, each, rect->h,
			            cgs.media.whiteShader );
		}
	}

	// if there is a partial square, draw it dropping off the end of the stack
	if ( frac <= 0.f )
	{
		trap_R_SetColor( NULL );
		return; // no partial square, we're done here
	}

	Vector4Copy( color, localColor );
	localColor[ 3 ] *= frac;
	trap_R_SetColor( localColor );

	switch ( lalign )
	{
		case LALIGN_TOPLEFT:
			if ( vertical )
			{
				CG_DrawPic( rect->x, rect->y - rect->h * ( 1 - frac ) / fmax,
				            rect->w, each, cgs.media.whiteShader );
			}
			else
			{
				CG_DrawPic( rect->x - rect->w * ( 1 - frac ) / fmax, rect->y,
				            each, rect->h, cgs.media.whiteShader );
			}

			break;

		case LALIGN_CENTER:

			// fallthrough
		default:
		case LALIGN_BOTTOMRIGHT:
			if ( vertical )
			{
				CG_DrawPic( rect->x, rect->y + rect->h *
				            ( 1 + ( ( 1 - fill ) / fmax ) - frac / fmax ),
				            rect->w, each, cgs.media.whiteShader );
			}
			else
			{
				CG_DrawPic( rect->x + rect->w *
				            ( 1 + ( ( 1 - fill ) / fmax ) - frac / fmax ), rect->y,
				            each, rect->h, cgs.media.whiteShader );
			}
	}

	trap_R_SetColor( NULL );
}

static void CG_DrawPlayerAmmoStack( rectDef_t *rect,
                                    vec4_t backColor, vec4_t foreColor,
                                    int textalign, int textvalign )
{
	float         val;
	int           maxVal;
	static int    lastws, maxwt, lastval, valdiff;
	playerState_t *ps = &cg.snap->ps;
	weapon_t      primary = BG_PrimaryWeapon( ps->stats );
	vec4_t        localColor;

	maxVal = BG_Weapon( primary )->maxAmmo;

	if ( maxVal <= 0 )
	{
		return; // not an ammo-carrying weapon
	}

	val = ps->ammo;

	// draw background if required
	if ( backColor[ 3 ] > 0.f )
	{
		trap_R_SetColor( backColor );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.whiteShader );
		trap_R_SetColor( NULL );
	}

	// smoothing effects (only if weaponTime etc. apply to primary weapon)
	if ( ps->weapon == primary && ps->weaponTime > 0 &&
	     ( ps->weaponstate == WEAPON_FIRING ||
	       ps->weaponstate == WEAPON_RELOADING ) )
	{
		// track the weaponTime we're coming down from
		// if weaponstate changed, this value is invalid
		if ( lastws != ps->weaponstate || ps->weaponTime > maxwt )
		{
			maxwt = ps->weaponTime;
			lastws = ps->weaponstate;
		}

		// if reloading, move towards max ammo value
		if ( ps->weaponstate == WEAPON_RELOADING )
		{
			val = maxVal;
		}

		// track size of change in ammo
		if ( lastval != val )
		{
			valdiff = lastval - val; // can be negative
			lastval = val;
		}

		if ( maxwt > 0 )
		{
			float f = ps->weaponTime / ( float ) maxwt;
			// move from last ammo value to current
			val += valdiff * f * f;
		}
	}
	else
	{
		// reset counters
		lastval = val;
		valdiff = 0;
		lastws = ps->weaponstate;
	}

	if ( val == 0 )
	{
		return; // nothing to draw
	}

	if ( val * 3 < maxVal )
	{
		// low on ammo
		// FIXME: don't hardcode this colour
		vec4_t lowAmmoColor = { 1.f, 0.f, 0.f, 0.f };
		// don't lerp alpha
		VectorLerpTrem( ( cg.time & 128 ), foreColor, lowAmmoColor, localColor );
		localColor[ 3 ] = foreColor[ 3 ];
	}
	else
	{
		Vector4Copy( foreColor, localColor );
	}

	CG_DrawStack( rect, localColor, 0.8, textalign, textvalign,
	              val, maxVal );
}

static void CG_DrawPlayerClipsStack( rectDef_t *rect,
                                     vec4_t backColor, vec4_t foreColor,
                                     int textalign, int textvalign )
{
	float         val;
	int           maxVal;
	static int    lastws, maxwt;
	playerState_t *ps = &cg.snap->ps;

	maxVal = BG_Weapon( BG_PrimaryWeapon( ps->stats ) )->maxClips;

	if ( !maxVal )
	{
		return; // not a clips weapon
	}

	val = ps->clips;

	// draw background if required
	if ( backColor[ 3 ] > 0.f )
	{
		trap_R_SetColor( backColor );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.whiteShader );
		trap_R_SetColor( NULL );
	}

	// if reloading, do fancy interpolation effects
	if ( ps->weaponstate == WEAPON_RELOADING )
	{
		float frac;

		// if we just started a reload, note the weaponTime we're coming down from
		if ( lastws != ps->weaponstate || ps->weaponTime > maxwt )
		{
			maxwt = ps->weaponTime;
			lastws = ps->weaponstate;
		}

		// just in case, don't divide by zero
		if ( maxwt != 0 )
		{
			frac = ps->weaponTime / ( float ) maxwt;
			val -= 1 - frac * frac; // speed is proportional to distance from target
		}
	}

	CG_DrawStack( rect, foreColor, 0.8, textalign, textvalign,
	              val, maxVal );
}

/*
===============
CG_OwnerDraw

Draw an owner drawn item
===============
*/
void CG_OwnerDraw( rectDef_t *rect, float text_x,
                   float text_y, int ownerDraw, int ownerDrawFlags,
                   int align, int textalign, int textvalign, float borderSize,
                   float scale, vec4_t foreColor, vec4_t backColor,
                   qhandle_t shader, int textStyle )
{
	Q_UNUSED(ownerDrawFlags);
	switch ( ownerDraw )
	{
		case CG_PLAYER_CREDITS_VALUE:
			CG_DrawPlayerCreditsValue( rect, foreColor, qtrue );
			break;

		case CG_PLAYER_CREDITS_FRACTION:
			CG_DrawPlayerCreditsFraction( rect, foreColor, shader );
			break;

		case CG_PLAYER_CREDITS_VALUE_NOPAD:
			CG_DrawPlayerCreditsValue( rect, foreColor, qfalse );
			break;

		case CG_PLAYER_ALIEN_EVOS:
			CG_DrawPlayerAlienEvos( rect, text_x, text_y, foreColor, scale, textalign, textvalign, textStyle );
			break;

		case CG_PLAYER_STAMINA_BAR:
			CG_DrawPlayerStaminaBar( rect, foreColor, shader );
			break;

		case CG_PLAYER_STAMINA:
			CG_DrawPlayerStaminaValue( rect, foreColor );
			break;

		case CG_PLAYER_STAMINA_BOLT:
			CG_DrawPlayerStaminaBolt( rect, backColor, foreColor, shader );
			break;

		case CG_PLAYER_FUEL_BAR:
			CG_DrawPlayerFuelBar( rect, foreColor, shader );
			break;

		case CG_PLAYER_FUEL_VALUE:
			CG_DrawPlayerFuelValue( rect, foreColor );
			break;

		case CG_PLAYER_FUEL_ICON:
			CG_DrawPlayerFuelIcon( rect, backColor, foreColor, shader );
			break;

		case CG_PLAYER_AMMO_VALUE:
			CG_DrawPlayerAmmoValue( rect, foreColor );
			break;

		case CG_PLAYER_TOTAL_AMMO_VALUE:
			CG_DrawPlayerTotalAmmoValue( rect, foreColor );
			break;

		case CG_PLAYER_CLIPS_VALUE:
			CG_DrawPlayerClipsValue( rect, foreColor );
			break;

		case CG_PLAYER_CLIPS_METER:
			CG_DrawPlayerClipMeter( rect, align, foreColor, shader );
			break;

		case CG_PLAYER_AMMO_STACK:
			CG_DrawPlayerAmmoStack( rect, backColor, foreColor, textalign,
			                        textvalign );
			break;

		case CG_PLAYER_CLIPS_STACK:
			CG_DrawPlayerClipsStack( rect, backColor, foreColor, textalign,
			                         textvalign );
			break;

		case CG_PLAYER_BUILD_TIMER:
			CG_DrawPlayerBuildTimer( rect, foreColor );
			break;

		case CG_PLAYER_BUILD_TIMER_BAR:
			CG_DrawPlayerBuildTimerBar( rect, foreColor, shader );
			break;
		case CG_PLAYER_HEALTH:
			CG_DrawPlayerHealthValue( rect, foreColor );
			break;
		case CG_PLAYER_HEALTH_BAR:
			CG_DrawPlayerHealthBar( rect, foreColor, shader );
			break;
		case CG_PLAYER_HEALTH_CROSS:
			CG_DrawPlayerHealthCross( rect, foreColor );
			break;

		case CG_PLAYER_HEALTH_METER:
			CG_DrawPlayerHealthMeter( rect, align, foreColor, shader );
			break;

		case CG_PLAYER_CHARGE_BAR_BG:
			CG_DrawPlayerChargeBarBG( rect, foreColor, shader );
			break;

		case CG_PLAYER_CHARGE_BAR:
			CG_DrawPlayerChargeBar( rect, foreColor, shader );
			break;

		case CG_PLAYER_CLIPS_RING:
			CG_DrawPlayerClipsRing( rect, backColor, foreColor, shader );
			break;

		case CG_PLAYER_BUILD_TIMER_RING:
			CG_DrawPlayerBuildTimerRing( rect, backColor, foreColor, shader );
			break;

		case CG_PLAYER_WALLCLIMBING:
			CG_DrawPlayerWallclimbing( rect, backColor, foreColor, shader );
			break;

		case CG_PLAYER_BOOSTED:
			CG_DrawPlayerBoosted( rect, backColor, foreColor, shader );
			break;

		case CG_PLAYER_BOOST_BOLT:
			CG_DrawPlayerBoosterBolt( rect, backColor, foreColor, shader );
			break;

		case CG_PLAYER_BOOSTED_METER:
			CG_DrawPlayerBoostedMeter( rect, align, foreColor, shader );
			break;

		case CG_PLAYER_POISON_BARBS:
			CG_DrawPlayerPoisonBarbs( rect, foreColor, shader );
			break;

		case CG_PLAYER_ALIEN_SENSE:
			CG_DrawAlienSense( rect );
			break;

		case CG_PLAYER_HUMAN_SCANNER:
			CG_DrawHumanScanner( rect, shader, foreColor );
			break;

		case CG_PLAYER_USABLE_BUILDABLE:
			CG_DrawUsableBuildable( rect, shader, foreColor );
			break;

		case CG_KILLER:
			CG_DrawKiller( rect, scale, foreColor, textStyle );
			break;

		case CG_PLAYER_HUMAN_INVENTORY:
			CG_DrawHumanInventory( rect, backColor, foreColor );
			break;

		case CG_PLAYER_WEAPONICON:
			CG_DrawWeaponIcon( rect, foreColor );
			break;

		case CG_PLAYER_SELECTTEXT:
			CG_DrawItemSelectText( rect, scale, textStyle );
			break;

		case CG_SPECTATORS:
			CG_DrawTeamSpectators( rect, scale, textvalign, foreColor );
			break;

		case CG_PLAYER_LOCATION:
			CG_DrawLocation( rect, scale, textalign, foreColor );
			break;

		case CG_FOLLOW:
			CG_DrawFollow( rect, text_x, text_y, foreColor, scale,
			               textalign, textvalign, textStyle );
			break;

		case CG_PLAYER_CROSSHAIRNAMES:
			CG_DrawCrosshairNames( rect, scale, textStyle );
			break;

		case CG_PLAYER_CROSSHAIR:
			CG_DrawCrosshair( rect, foreColor );
			break;

		case CG_PLAYER_CROSSHAIR_INDICATOR:
			CG_DrawCrosshairIndicator( rect, foreColor );
			break;

		case CG_MOMENTUM_TEXT:
			CG_DrawMomentum( rect, text_x, text_y, foreColor, scale, textalign, textvalign, textStyle );
			break;

		case CG_MOMENTUM_BAR:
			CG_DrawPlayerMomentumBar( rect, foreColor, backColor, borderSize );
			break;

		case CG_UNLOCKED_ITEMS:
			CG_DrawPlayerUnlockedItems( rect, foreColor, backColor, borderSize );
			break;

		case CG_ALIENS_SCORE_LABEL:
			CG_DrawTeamLabel( rect, TEAM_ALIENS, text_x, text_y, foreColor, scale, textalign, textvalign, textStyle );
			break;

		case CG_HUMANS_SCORE_LABEL:
			CG_DrawTeamLabel( rect, TEAM_HUMANS, text_x, text_y, foreColor, scale, textalign, textvalign, textStyle );
			break;

		case CG_MINE_RATE:
			CG_DrawMineRate( rect, text_x, text_y, foreColor, scale, textalign, textvalign, textStyle );
			break;

			//loading screen
		case CG_LOAD_LEVELSHOT:
			CG_DrawLevelShot( rect );
			break;

		case CG_LOAD_MEDIA:
			CG_DrawMediaProgress( rect, foreColor, scale, align, textalign, textStyle,
			                      borderSize );
			break;

		case CG_LOAD_MEDIA_LABEL:
			CG_DrawMediaProgressLabel( rect, text_x, text_y, foreColor, scale, textalign, textvalign );
			break;

		case CG_LOAD_BUILDABLES:
			CG_DrawBuildablesProgress( rect, foreColor, scale, align, textalign,
			                           textStyle, borderSize );
			break;

		case CG_LOAD_BUILDABLES_LABEL:
			CG_DrawBuildablesProgressLabel( rect, text_x, text_y, foreColor, scale, textalign, textvalign );
			break;

		case CG_LOAD_CHARMODEL:
			CG_DrawCharModelProgress( rect, foreColor, scale, align, textalign,
			                          textStyle, borderSize );
			break;

		case CG_LOAD_CHARMODEL_LABEL:
			CG_DrawCharModelProgressLabel( rect, text_x, text_y, foreColor, scale, textalign, textvalign );
			break;

		case CG_LOAD_OVERALL:
			CG_DrawOverallProgress( rect, foreColor, scale, align, textalign, textStyle,
			                        borderSize );
			break;

		case CG_LOAD_OVERALL_LABEL:
			CG_DrawOverallProgressLabel( rect, text_x, text_y, foreColor, scale, textalign, textvalign );
			break;

		case CG_LOAD_LEVELNAME:
			CG_DrawLevelName( rect, text_x, text_y, foreColor, scale, textalign, textvalign, textStyle );
			break;

		case CG_LOAD_MOTD:
			CG_DrawMOTD( rect, text_x, text_y, foreColor, scale, textalign, textvalign, textStyle );
			break;

		case CG_LOAD_HOSTNAME:
			CG_DrawHostname( rect, text_x, text_y, foreColor, scale, textalign, textvalign, textStyle );
			break;

		case CG_FPS:
			CG_DrawFPS( rect, scale, foreColor, textalign, textvalign, textStyle, qtrue );
			break;

		case CG_FPS_FIXED:
			CG_DrawFPS( rect, scale, foreColor, textalign, textvalign, textStyle, qfalse );
			break;

		case CG_TIMER:
			CG_DrawTimer( rect, scale, foreColor, textalign, textvalign, textStyle );
			break;

		case CG_CLOCK:
			CG_DrawClock( rect, scale, foreColor, textalign, textvalign, textStyle );
			break;

		case CG_TIMER_MINS:
			CG_DrawTimerMins( rect, foreColor );
			break;

		case CG_TIMER_SECS:
			CG_DrawTimerSecs( rect, foreColor );
			break;

		case CG_SNAPSHOT:
			CG_DrawSnapshot( rect, text_x, text_y, scale, foreColor, textalign, textvalign, textStyle );
			break;

		case CG_LAGOMETER:
			CG_DrawLagometer( rect, text_x, text_y, scale, foreColor );
			break;

		case CG_TEAMOVERLAY:
			CG_DrawTeamOverlay( rect, scale, foreColor );
			break;

		case CG_SPEEDOMETER:
			CG_DrawSpeed( rect, scale, foreColor, backColor );
			break;

		case CG_DEMO_PLAYBACK:
			CG_DrawDemoPlayback( rect, foreColor, shader );
			break;

		case CG_DEMO_RECORDING:
			CG_DrawDemoRecording( rect, foreColor, shader );
			break;

		case CG_CONSOLE:
			CG_DrawConsole( rect, text_x, text_y, foreColor, scale, textalign, textvalign, textStyle );
			break;

		case CG_TUTORIAL:
			CG_DrawTutorial( rect, text_x, text_y, foreColor, scale, textalign, textvalign, textStyle );
			break;

		case CG_MINIMAP:
			CG_DrawMinimap( rect, foreColor );
			break;

		default:
			break;
	}
}

void CG_MouseEvent( int x, int y )
{
	int n;

	if ( ( cg.predictedPlayerState.pm_type == PM_NORMAL ||
	       cg.predictedPlayerState.pm_type == PM_SPECTATOR ) &&
	     cg.showScores == qfalse )
	{
		trap_Key_SetCatcher( 0 );
		return;
	}

	cgs.cursorX += x;

	if ( cgs.cursorX < 0 )
	{
		cgs.cursorX = 0;
	}
	else if ( cgs.cursorX > 640 )
	{
		cgs.cursorX = 640;
	}

	cgs.cursorY += y;

	if ( cgs.cursorY < 0 )
	{
		cgs.cursorY = 0;
	}
	else if ( cgs.cursorY > 480 )
	{
		cgs.cursorY = 480;
	}

	n = Display_CursorType( cgs.cursorX, cgs.cursorY );
	cgs.activeCursor = 0;

	if ( n == CURSOR_ARROW )
	{
		cgs.activeCursor = cgs.media.selectCursor;
	}
	else if ( n == CURSOR_SIZER )
	{
		cgs.activeCursor = cgs.media.sizeCursor;
	}

	if ( cgs.capturedItem )
	{
		Display_MouseMove( cgs.capturedItem, x, y );
	}
	else
	{
		Display_MouseMove( NULL, cgs.cursorX, cgs.cursorY );
	}
}

void CG_KeyEvent( int key, int chr, int flags )
{
	if ( !( flags & KEYEVSTATE_DOWN ) )
	{
		return;
	}

	if ( cg.predictedPlayerState.pm_type == PM_NORMAL ||
	     ( cg.predictedPlayerState.pm_type == PM_SPECTATOR &&
	       cg.showScores == qfalse ) )
	{
		trap_Key_SetCatcher( 0 );
		return;
	}

	Display_HandleKey( key, chr, qtrue, cgs.cursorX, cgs.cursorY );

	if ( cgs.capturedItem )
	{
		cgs.capturedItem = NULL;
	}
	else
	{
		if ( key == K_MOUSE2 /*&& down*/ )
		{
			cgs.capturedItem = Display_CaptureItem( cgs.cursorX, cgs.cursorY );
		}
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
===============================================================================

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
void CG_CenterPrint( const char *str, int y, int charWidth )
{
	char       *s;
	char       newlineParsed[ MAX_STRING_CHARS ];
	const char *wrapped;
	static int maxWidth = ( int )( ( 2.0f / 3.0f ) * ( float ) SCREEN_WIDTH );

	Q_ParseNewlines( newlineParsed, str, sizeof( newlineParsed ) );

	wrapped = Item_Text_Wrap( newlineParsed, 0.5f, maxWidth );

	Q_strncpyz( cg.centerPrint, wrapped, sizeof( cg.centerPrint ) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;

	while ( *s )
	{
		if ( *s == '\n' )
		{
			cg.centerPrintLines++;
		}

		s++;
	}
}

/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void )
{
	char  *start;
	int   l;
	int   x, y, w;
	int   h;
	float *color;

	if ( !cg.centerPrintTime )
	{
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );

	if ( !color )
	{
		return;
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 )
	{
		char linebuffer[ MAX_STRING_CHARS ];

		for ( l = 0; l + 1 < (int) sizeof( linebuffer ); l++ )
		{
			if ( !start[ l ] || start[ l ] == '\n' )
			{
				break;
			}

			linebuffer[ l ] = start[ l ];
		}

		linebuffer[ l ] = 0;

		w = UI_Text_Width( linebuffer, 0.5 );
		h = UI_Text_Height( linebuffer, 0.5 );
		x = ( SCREEN_WIDTH - w ) / 2;
		UI_Text_Paint( x, y + h, 0.5, color, linebuffer, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );
		y += h + 6;

		while ( *start && ( *start != '\n' ) )
		{
			start++;
		}

		if ( !*start )
		{
			break;
		}

		start++;
	}

	trap_R_SetColor( NULL );
}

//==============================================================================

//FIXME: both vote notes are hardcoded, change to ownerdrawn?

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote( team_t team )
{
	char   *s;
	int    sec;
	int    offset = 0;
	vec4_t white = { 1.0f, 1.0f, 1.0f, 1.0f };
	char   yeskey[ 32 ] = "", nokey[ 32 ] = "";

	if ( !cgs.voteTime[ team ] )
	{
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified[ team ] )
	{
		cgs.voteModified[ team ] = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime[ team ] ) ) / 1000;

	if ( sec < 0 )
	{
		sec = 0;
	}

	if ( cg_tutorial.integer )
	{
		Com_sprintf( yeskey, sizeof( yeskey ), "[%s]",
		             CG_KeyBinding( va( "%svote yes", team == TEAM_NONE ? "" : "team" ), team ) );
		Com_sprintf( nokey, sizeof( nokey ), "[%s]",
		             CG_KeyBinding( va( "%svote no", team == TEAM_NONE ? "" : "team" ), team ) );
	}

	if ( team != TEAM_NONE )
	{
		offset = 80;
	}

	s = va( "%sVOTE(%i): %s",
	        team == TEAM_NONE ? "" : "TEAM", sec, cgs.voteString[ team ] );

	UI_Text_Paint( 8, 300 + offset, 0.3f, white, s, 0, ITEM_TEXTSTYLE_PLAIN );

	s = va( "  Called by: \"%s\"", cgs.voteCaller[ team ] );

	UI_Text_Paint( 8, 320 + offset, 0.3f, white, s, 0, ITEM_TEXTSTYLE_PLAIN );

	s = va( "  %s[check]:%i %s[cross]:%i",
	        yeskey, cgs.voteYes[ team ], nokey, cgs.voteNo[ team ] );

	UI_Text_Paint( 8, 340 + offset, 0.3f, white, s, 0, ITEM_TEXTSTYLE_PLAIN );
}

static qboolean CG_DrawScoreboard( void )
{
	static qboolean firstTime = qtrue;

	if ( menuScoreboard )
	{
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}

	if ( cg_paused.integer )
	{
		firstTime = qtrue;
		return qfalse;
	}

	if ( !cg.showScores &&
	     cg.predictedPlayerState.pm_type != PM_INTERMISSION )
	{
		cg.killerName[ 0 ] = 0;
		firstTime = qtrue;
		return qfalse;
	}

	CG_RequestScores();

	if ( menuScoreboard == NULL )
	{
		menuScoreboard = Menus_FindByName( "teamscore_menu" );
	}

	if ( menuScoreboard )
	{
		if ( firstTime )
		{
			cg.spectatorTime = trap_Milliseconds();
			CG_SetScoreSelection( menuScoreboard );
			firstTime = qfalse;
		}

		Menu_Update( menuScoreboard );
		Menu_Paint( menuScoreboard, qtrue );
	}

	return qtrue;
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void )
{
	menuDef_t *menu = Menus_FindByName( "default_hud" );

	Menu_Update( menu );
	Menu_Paint( menu, qtrue );

	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawQueue
=================
*/
static qboolean CG_DrawQueue( void )
{
	float  w;
	vec4_t color;
	int    position, spawns;
	char   buffer[ MAX_STRING_CHARS ];

	if ( !( cg.snap->ps.pm_flags & PMF_QUEUED ) )
	{
		return qfalse;
	}

	color[ 0 ] = 1;
	color[ 1 ] = 1;
	color[ 2 ] = 1;
	color[ 3 ] = 1;

	spawns   = cg.snap->ps.persistant[ PERS_SPAWNQUEUE ] & 0x000000ff;
	position = cg.snap->ps.persistant[ PERS_SPAWNQUEUE ] >> 8;

	if ( position < 1 )
	{
		return qfalse;
	}

	if ( position == 1 )
	{
		Com_sprintf( buffer, MAX_STRING_CHARS, _("You are at the front of the spawn queue") );
	}
	else
	{
		Com_sprintf( buffer, MAX_STRING_CHARS, _("You are at position %d in the spawn queue"), position );
	}

	w = UI_Text_Width( buffer, 0.7f );
	UI_Text_Paint( 320 - w / 2, 360, 0.7f, color, buffer, 0, ITEM_TEXTSTYLE_SHADOWED );

	if ( spawns == 0 )
	{
		Com_sprintf( buffer, MAX_STRING_CHARS, _("There are no spawns remaining") );
	}
	else
	{
		Com_sprintf( buffer, MAX_STRING_CHARS,
		             P_( "There is 1 spawn remaining", "There are %d spawns remaining", spawns ), spawns );
	}

	w = UI_Text_Width( buffer, 0.7f );
	UI_Text_Paint( 320 - w / 2, 400, 0.7f, color, buffer, 0, ITEM_TEXTSTYLE_SHADOWED );

	return qtrue;
}

/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void )
{
	int   sec = 0;
	int   w;
	int   h;
	float size = 0.5f;
	char  text[ MAX_STRING_CHARS ];

	if ( !cg.warmupTime )
	{
		return;
	}

	sec = ( cg.warmupTime - cg.time ) / 1000;

	if ( sec < 0 )
	{
		return;
	}

	Q_strncpyz( text, _( "Warmup Time:" ), sizeof( text ) );
	w = UI_Text_Width( text, size );
	h = UI_Text_Height( text, size );
	UI_Text_Paint( 320 - w / 2, 200, size, colorWhite, text, 0, ITEM_TEXTSTYLE_SHADOWED );

	Com_sprintf( text, sizeof( text ), "%s", sec ? va( "%d", sec ) : _("FIGHT!") );

	w = UI_Text_Width( text, size );
	UI_Text_Paint( 320 - w / 2, 200 + 1.5f * h, size, colorWhite, text, 0, ITEM_TEXTSTYLE_SHADOWED );
}

//==================================================================================

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D( void )
{
	menuDef_t *menu = NULL;

	if ( cg_draw2D.integer == 0 )
	{
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION )
	{
		CG_DrawVote( TEAM_NONE );
		CG_DrawVote( (team_t) cg.predictedPlayerState.persistant[ PERS_TEAM ] );
		CG_DrawIntermission();
		return;
	}

	if ( cg.snap->ps.persistant[ PERS_SPECSTATE ] == SPECTATOR_NOT &&
	     cg.snap->ps.stats[ STAT_HEALTH ] > 0 && !cg.zoomed )
	{
		menu = Menus_FindByName( BG_ClassModelConfig(
		                           cg.predictedPlayerState.stats[ STAT_CLASS ] )->hudName );

		CG_DrawBuildableStatus();
	}

	if ( !menu )
	{
		menu = Menus_FindByName( "default_hud" );

		if ( !menu ) // still couldn't find it
		{
			CG_Error( "Default HUD could not be found" );
		}
	}

	if ( cg.zoomed )
	{
		vec4_t black = { 0.0f, 0.0f, 0.0f, 0.5f };
		trap_R_DrawStretchPic( ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ), 0, cgs.glconfig.vidHeight, cgs.glconfig.vidHeight, 0, 0, 1, 1, cgs.media.scopeShader );
		trap_R_SetColor( black );
		trap_R_DrawStretchPic( 0, 0, ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ), cgs.glconfig.vidHeight, 0, 0, 1, 1, cgs.media.whiteShader );
		trap_R_DrawStretchPic( cgs.glconfig.vidWidth - ( ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ) ), 0, ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ), cgs.glconfig.vidHeight, 0, 0, 1, 1, cgs.media.whiteShader );
		trap_R_SetColor( NULL );
	}
	else
	{
		Menu_Update( menu );
		Menu_Paint( menu, qtrue );
	}

	CG_DrawVote( TEAM_NONE );
	CG_DrawVote( (team_t) cg.predictedPlayerState.persistant[ PERS_TEAM ] );
	CG_DrawWarmup();
	CG_DrawQueue();

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();

	if ( !cg.scoreBoardShowing )
	{
		CG_DrawCenterString();
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
static void CG_PainBlend( void )
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

	trap_R_SetColor( NULL );
}

/*
=====================
CG_ResetPainBlend
=====================
*/
void CG_ResetPainBlend( void )
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
static void CG_DrawBinaryShadersFinalPhases( void )
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
void CG_DrawActive( stereoFrame_t stereoView )
{
	float  separation;
	vec3_t baseOrg;

	// optionally draw the info screen instead
	if ( !cg.snap )
	{
		return;
	}

	switch ( stereoView )
	{
		case STEREO_CENTER:
			separation = 0;
			break;

		case STEREO_LEFT:
			separation = -cg_stereoSeparation.value / 2;
			break;

		case STEREO_RIGHT:
			separation = cg_stereoSeparation.value / 2;
			break;

		default:
			separation = 0;
			CG_Error( "CG_DrawActive: Undefined stereoView" );
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy( cg.refdef.vieworg, baseOrg );

	if ( separation != 0 )
	{
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[ 1 ],
		          cg.refdef.vieworg );
	}

	CG_DrawBinaryShadersFinalPhases();

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// restore original viewpoint if running stereo
	if ( separation != 0 )
	{
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}

	// first person blend blobs, done after AnglesToAxis
	if ( !cg.renderingThirdPerson )
	{
		CG_PainBlend();
	}

	// draw status bar and other floating elements
	CG_Draw2D();
}
