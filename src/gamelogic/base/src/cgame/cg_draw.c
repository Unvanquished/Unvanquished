/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

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

// used for scoreboard
extern    displayContextDef_t cgDC;
menuDef_t                     *menuScoreboard = NULL;

int                           drawTeamOverlayModificationCount = -1;

int                           sortedTeamPlayers[ TEAM_MAXOVERLAY ];
int                           numSortedTeamPlayers;
char                          systemChat[ 256 ];
char                          teamChat1[ 256 ];
char                          teamChat2[ 256 ];

//TA UI
int CG_Text_Width( const char *text, float scale, int limit )
{
	int         count, len;
	float       out;
	glyphInfo_t *glyph;
	float       useScale;
// FIXME: see ui_main.c, same problem
//  const unsigned char *s = text;
	const char  *s = text;
	fontInfo_t  *font = &cgDC.Assets.textFont;

	if( scale <= cg_smallFont.value )
	{
		font = &cgDC.Assets.smallFont;
	}
	else if( scale > cg_bigFont.value )
	{
		font = &cgDC.Assets.bigFont;
	}

	useScale = scale * font->glyphScale;
	out = 0;

	if( text )
	{
		len = strlen( text );

		if( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while( s && *s && count < len )
		{
			if( Q_IsColorString( s ) )
			{
				s += 2;
				continue;
			}
			else
			{
				glyph = &font->glyphs[( int ) * s ];
				//TTimo: FIXME: getting nasty warnings without the cast,
				//hopefully this doesn't break the VM build
				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}

	return out * useScale;
}

int CG_Text_Height( const char *text, float scale, int limit )
{
	int         len, count;
	float       max;
	glyphInfo_t *glyph;
	float       useScale;
// TTimo: FIXME
//  const unsigned char *s = text;
	const char  *s = text;
	fontInfo_t  *font = &cgDC.Assets.textFont;

	if( scale <= cg_smallFont.value )
	{
		font = &cgDC.Assets.smallFont;
	}
	else if( scale > cg_bigFont.value )
	{
		font = &cgDC.Assets.bigFont;
	}

	useScale = scale * font->glyphScale;
	max = 0;

	if( text )
	{
		len = strlen( text );

		if( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while( s && *s && count < len )
		{
			if( Q_IsColorString( s ) )
			{
				s += 2;
				continue;
			}
			else
			{
				glyph = &font->glyphs[( int ) * s ];

				//TTimo: FIXME: getting nasty warnings without the cast,
				//hopefully this doesn't break the VM build
				if( max < glyph->height )
				{
					max = glyph->height;
				}

				s++;
				count++;
			}
		}
	}

	return max * useScale;
}

void CG_Text_PaintChar( float x, float y, float width, float height, float scale,
                        float s, float t, float s2, float t2, qhandle_t hShader )
{
	float w, h;
	w = width * scale;
	h = height * scale;
	CG_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint( float x, float y, float scale, vec4_t color, const char *text,
                    float adjust, int limit, int style )
{
	int         len, count;
	vec4_t      newColor;
	glyphInfo_t *glyph;
	float       useScale;
	fontInfo_t  *font = &cgDC.Assets.textFont;

	if( scale <= cg_smallFont.value )
	{
		font = &cgDC.Assets.smallFont;
	}
	else if( scale > cg_bigFont.value )
	{
		font = &cgDC.Assets.bigFont;
	}

	useScale = scale * font->glyphScale;

	if( text )
	{
// TTimo: FIXME
//    const unsigned char *s = text;
		const char *s = text;

		trap_R_SetColor( color );
		memcpy( &newColor[ 0 ], &color[ 0 ], sizeof( vec4_t ) );
		len = strlen( text );

		if( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while( s && *s && count < len )
		{
			glyph = &font->glyphs[( int ) * s ];
			//TTimo: FIXME: getting nasty warnings without the cast,
			//hopefully this doesn't break the VM build

			if( Q_IsColorString( s ) )
			{
				memcpy( newColor, g_color_table[ ColorIndex( * ( s + 1 ) ) ], sizeof( newColor ) );
				newColor[ 3 ] = color[ 3 ];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			}
			else
			{
				float yadj = useScale * glyph->top;

				if( style == ITEM_TEXTSTYLE_SHADOWED ||
				    style == ITEM_TEXTSTYLE_SHADOWEDMORE )
				{
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[ 3 ] = newColor[ 3 ];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar( x + ofs, y - yadj + ofs,
					                   glyph->imageWidth,
					                   glyph->imageHeight,
					                   useScale,
					                   glyph->s,
					                   glyph->t,
					                   glyph->s2,
					                   glyph->t2,
					                   glyph->glyph );

					colorBlack[ 3 ] = 1.0;
					trap_R_SetColor( newColor );
				}
				else if( style == ITEM_TEXTSTYLE_NEON )
				{
					vec4_t glow, outer, inner, white;

					glow[ 0 ] = newColor[ 0 ] * 0.5;
					glow[ 1 ] = newColor[ 1 ] * 0.5;
					glow[ 2 ] = newColor[ 2 ] * 0.5;
					glow[ 3 ] = newColor[ 3 ] * 0.2;

					outer[ 0 ] = newColor[ 0 ];
					outer[ 1 ] = newColor[ 1 ];
					outer[ 2 ] = newColor[ 2 ];
					outer[ 3 ] = newColor[ 3 ];

					inner[ 0 ] = newColor[ 0 ] * 1.5 > 1.0f ? 1.0f : newColor[ 0 ] * 1.5;
					inner[ 1 ] = newColor[ 1 ] * 1.5 > 1.0f ? 1.0f : newColor[ 1 ] * 1.5;
					inner[ 2 ] = newColor[ 2 ] * 1.5 > 1.0f ? 1.0f : newColor[ 2 ] * 1.5;
					inner[ 3 ] = newColor[ 3 ];

					white[ 0 ] = white[ 1 ] = white[ 2 ] = white[ 3 ] = 1.0f;

					trap_R_SetColor( glow );
					CG_Text_PaintChar( x - 3, y - yadj - 3,
					                   glyph->imageWidth + 6,
					                   glyph->imageHeight + 6,
					                   useScale,
					                   glyph->s,
					                   glyph->t,
					                   glyph->s2,
					                   glyph->t2,
					                   glyph->glyph );

					trap_R_SetColor( outer );
					CG_Text_PaintChar( x - 1, y - yadj - 1,
					                   glyph->imageWidth + 2,
					                   glyph->imageHeight + 2,
					                   useScale,
					                   glyph->s,
					                   glyph->t,
					                   glyph->s2,
					                   glyph->t2,
					                   glyph->glyph );

					trap_R_SetColor( inner );
					CG_Text_PaintChar( x - 0.5, y - yadj - 0.5,
					                   glyph->imageWidth + 1,
					                   glyph->imageHeight + 1,
					                   useScale,
					                   glyph->s,
					                   glyph->t,
					                   glyph->s2,
					                   glyph->t2,
					                   glyph->glyph );

					trap_R_SetColor( white );
				}

				CG_Text_PaintChar( x, y - yadj,
				                   glyph->imageWidth,
				                   glyph->imageHeight,
				                   useScale,
				                   glyph->s,
				                   glyph->t,
				                   glyph->s2,
				                   glyph->t2,
				                   glyph->glyph );

				x += ( glyph->xSkip * useScale ) + adjust;
				s++;
				count++;
			}
		}

		trap_R_SetColor( NULL );
	}
}

/*
==============
CG_DrawFieldPadded

Draws large numbers for status bar and powerups
==============
*/
static void CG_DrawFieldPadded( int x, int y, int width, int cw, int ch, int value )
{
	char num[ 16 ], *ptr;
	int  l, orgL;
	int  frame;
	int  charWidth, charHeight;

	if( !( charWidth = cw ) )
	{
		charWidth = CHAR_WIDTH;
	}

	if( !( charHeight = ch ) )
	{
		charWidth = CHAR_HEIGHT;
	}

	if( width < 1 )
	{
		return;
	}

	// draw number string
	if( width > 4 )
	{
		width = 4;
	}

	switch( width )
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

	if( l > width )
	{
		l = width;
	}

	orgL = l;

	x += 2;

	ptr = num;

	while( *ptr && l )
	{
		if( width > orgL )
		{
			CG_DrawPic( x, y, charWidth, charHeight, cgs.media.numberShaders[ 0 ] );
			width--;
			x += charWidth;
			continue;
		}

		if( *ptr == '-' )
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

Draws large numbers for status bar and powerups
==============
*/
static void CG_DrawField( int x, int y, int width, int cw, int ch, int value )
{
	char num[ 16 ], *ptr;
	int  l;
	int  frame;
	int  charWidth, charHeight;

	if( !( charWidth = cw ) )
	{
		charWidth = CHAR_WIDTH;
	}

	if( !( charHeight = ch ) )
	{
		charWidth = CHAR_HEIGHT;
	}

	if( width < 1 )
	{
		return;
	}

	// draw number string
	if( width > 4 )
	{
		width = 4;
	}

	switch( width )
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

	if( l > width )
	{
		l = width;
	}

	x += 2 + charWidth * ( width - l );

	ptr = num;

	while( *ptr && l )
	{
		if( *ptr == '-' )
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
                                int align, int textStyle, int special, float progress )
{
	float rimWidth = rect->h / 20.0f;
	float doneWidth, leftWidth;
	float tx, ty, tw, th;
	char  textBuffer[ 8 ];

	if( rimWidth < 0.6f )
	{
		rimWidth = 0.6f;
	}

	if( special >= 0.0f )
	{
		rimWidth = special;
	}

	if( progress < 0.0f )
	{
		progress = 0.0f;
	}
	else if( progress > 1.0f )
	{
		progress = 1.0f;
	}

	doneWidth = ( rect->w - 2 * rimWidth ) * progress;
	leftWidth = ( rect->w - 2 * rimWidth ) - doneWidth;

	trap_R_SetColor( color );

	//draw rim and bar
	if( align == ITEM_ALIGN_RIGHT )
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
	if( scale > 0.0 )
	{
		Com_sprintf( textBuffer, sizeof( textBuffer ), "%d%%", ( int )( progress * 100 ) );
		tw = CG_Text_Width( textBuffer, scale, 0 );
		th = scale * 40.0f;

		switch( align )
		{
			case ITEM_ALIGN_LEFT:
				tx = rect->x + ( rect->w / 10.0f );
				ty = rect->y + ( rect->h / 2.0f ) + ( th / 2.0f );
				break;

			case ITEM_ALIGN_RIGHT:
				tx = rect->x + rect->w - ( rect->w / 10.0f ) - tw;
				ty = rect->y + ( rect->h / 2.0f ) + ( th / 2.0f );
				break;

			case ITEM_ALIGN_CENTER:
				tx = rect->x + ( rect->w / 2.0f ) - ( tw / 2.0f );
				ty = rect->y + ( rect->h / 2.0f ) + ( th / 2.0f );
				break;

			default:
				tx = ty = 0.0f;
		}

		CG_Text_Paint( tx, ty, scale, color, textBuffer, 0, 0, textStyle );
	}
}

//=============== TA: was cg_newdraw.c

void CG_InitTeamChat( void )
{
	memset( teamChat1,  0, sizeof( teamChat1 ) );
	memset( teamChat2,  0, sizeof( teamChat2 ) );
	memset( systemChat, 0, sizeof( systemChat ) );
}

void CG_SetPrintString( int type, const char *p )
{
	if( type == SYSTEM_PRINT )
	{
		strcpy( systemChat, p );
	}
	else
	{
		strcpy( teamChat2, teamChat1 );
		strcpy( teamChat1, p );
	}
}

#define NO_CREDITS_TIME 2000

static void CG_DrawPlayerCreditsValue( rectDef_t *rect, vec4_t color, qboolean padding )
{
	int           value;
	playerState_t *ps;
	centity_t     *cent;

	cent = &cg_entities[ cg.snap->ps.clientNum ];
	ps = &cg.snap->ps;

	//if the build timer pie is showing don't show this
	if( ( cent->currentState.weapon == WP_ABUILD ||
	      cent->currentState.weapon == WP_ABUILD2 ) && ps->stats[ STAT_MISC ] )
	{
		return;
	}

	value = ps->persistant[ PERS_CREDIT ];

	if( value > -1 )
	{
		if( cg.predictedPlayerState.stats[ STAT_PTEAM ] == PTE_ALIENS &&
		    !CG_AtHighestClass() )
		{
			if( cg.time - cg.lastEvolveAttempt <= NO_CREDITS_TIME )
			{
				if( ( ( cg.time - cg.lastEvolveAttempt ) / 300 ) % 2 )
				{
					color[ 3 ] = 0.0f;
				}
			}
		}

		trap_R_SetColor( color );

		if( padding )
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

static void CG_DrawPlayerBankValue( rectDef_t *rect, vec4_t color, qboolean padding )
{
	int           value;
	playerState_t *ps;

	ps = &cg.snap->ps;

	value = ps->persistant[ PERS_BANK ];

	if( value > -1 )
	{
		trap_R_SetColor( color );

		if( padding )
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

#define HH_MIN_ALPHA  0.2f
#define HH_MAX_ALPHA  0.8f
#define HH_ALPHA_DIFF ( HH_MAX_ALPHA - HH_MIN_ALPHA )

#define AH_MIN_ALPHA  0.2f
#define AH_MAX_ALPHA  0.8f
#define AH_ALPHA_DIFF ( AH_MAX_ALPHA - AH_MIN_ALPHA )

/*
==============
CG_DrawPlayerStamina1
==============
*/
static void CG_DrawPlayerStamina1( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	float         stamina = ps->stats[ STAT_STAMINA ];
	float         maxStaminaBy3 = ( float ) MAX_STAMINA / 3.0f;
	float         progress;

	stamina -= ( 2 * ( int ) maxStaminaBy3 );
	progress = stamina / maxStaminaBy3;

	if( progress > 1.0f )
	{
		progress = 1.0f;
	}
	else if( progress < 0.0f )
	{
		progress = 0.0f;
	}

	color[ 3 ] = HH_MIN_ALPHA + ( progress * HH_ALPHA_DIFF );

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerStamina2
==============
*/
static void CG_DrawPlayerStamina2( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	float         stamina = ps->stats[ STAT_STAMINA ];
	float         maxStaminaBy3 = ( float ) MAX_STAMINA / 3.0f;
	float         progress;

	stamina -= ( int ) maxStaminaBy3;
	progress = stamina / maxStaminaBy3;

	if( progress > 1.0f )
	{
		progress = 1.0f;
	}
	else if( progress < 0.0f )
	{
		progress = 0.0f;
	}

	color[ 3 ] = HH_MIN_ALPHA + ( progress * HH_ALPHA_DIFF );

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerStamina3
==============
*/
static void CG_DrawPlayerStamina3( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	float         stamina = ps->stats[ STAT_STAMINA ];
	float         maxStaminaBy3 = ( float ) MAX_STAMINA / 3.0f;
	float         progress;

	progress = stamina / maxStaminaBy3;

	if( progress > 1.0f )
	{
		progress = 1.0f;
	}
	else if( progress < 0.0f )
	{
		progress = 0.0f;
	}

	color[ 3 ] = HH_MIN_ALPHA + ( progress * HH_ALPHA_DIFF );

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerStamina4
==============
*/
static void CG_DrawPlayerStamina4( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	float         stamina = ps->stats[ STAT_STAMINA ];
	float         progress;

	stamina += ( float ) MAX_STAMINA;
	progress = stamina / ( float ) MAX_STAMINA;

	if( progress > 1.0f )
	{
		progress = 1.0f;
	}
	else if( progress < 0.0f )
	{
		progress = 0.0f;
	}

	color[ 3 ] = HH_MIN_ALPHA + ( progress * HH_ALPHA_DIFF );

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerStaminaBolt
==============
*/
static void CG_DrawPlayerStaminaBolt( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	float         stamina = ps->stats[ STAT_STAMINA ];

	if( stamina < 0 )
	{
		color[ 3 ] = HH_MIN_ALPHA;
	}
	else
	{
		color[ 3 ] = HH_MAX_ALPHA;
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerClipsRing
==============
*/
static void CG_DrawPlayerClipsRing( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	centity_t     *cent;
	float         buildTime = ps->stats[ STAT_MISC ];
	float         progress;
	float         maxDelay;

	cent = &cg_entities[ cg.snap->ps.clientNum ];

	switch( cent->currentState.weapon )
	{
		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
		case WP_HBUILD2:
			maxDelay = ( float ) BG_FindBuildDelayForWeapon( cent->currentState.weapon );

			if( buildTime > maxDelay )
			{
				buildTime = maxDelay;
			}

			progress = ( maxDelay - buildTime ) / maxDelay;

			color[ 3 ] = HH_MIN_ALPHA + ( progress * HH_ALPHA_DIFF );
			break;

		default:
			if( ps->weaponstate == WEAPON_RELOADING )
			{
				maxDelay = ( float ) BG_FindReloadTimeForWeapon( cent->currentState.weapon );
				progress = ( maxDelay - ( float ) ps->weaponTime ) / maxDelay;

				color[ 3 ] = HH_MIN_ALPHA + ( progress * HH_ALPHA_DIFF );
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
static void CG_DrawPlayerBuildTimerRing( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	centity_t     *cent;
	float         buildTime = ps->stats[ STAT_MISC ];
	float         progress;
	float         maxDelay;

	cent = &cg_entities[ cg.snap->ps.clientNum ];

	maxDelay = ( float ) BG_FindBuildDelayForWeapon( cent->currentState.weapon );

	if( buildTime > maxDelay )
	{
		buildTime = maxDelay;
	}

	progress = ( maxDelay - buildTime ) / maxDelay;

	color[ 3 ] = AH_MIN_ALPHA + ( progress * AH_ALPHA_DIFF );

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerBoosted
==============
*/
static void CG_DrawPlayerBoosted( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	qboolean      boosted = ps->stats[ STAT_STATE ] & SS_BOOSTED;

	if( boosted )
	{
		color[ 3 ] = AH_MAX_ALPHA;
	}
	else
	{
		color[ 3 ] = AH_MIN_ALPHA;
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerBoosterBolt
==============
*/
static void CG_DrawPlayerBoosterBolt( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	qboolean      boosted = ps->stats[ STAT_STATE ] & SS_BOOSTED;
	vec4_t        localColor;

	Vector4Copy( color, localColor );

	if( boosted )
	{
		if( ps->stats[ STAT_BOOSTTIME ] > BOOST_TIME - 3000 )
		{
			qboolean flash = ( ps->stats[ STAT_BOOSTTIME ] / 500 ) % 2;

			if( flash )
			{
				localColor[ 3 ] = 1.0f;
			}
		}
	}

	trap_R_SetColor( localColor );
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
	playerState_t *ps = &cg.snap->ps;
	int           x = rect->x;
	int           y = rect->y;
	int           width = rect->w;
	int           height = rect->h;
	qboolean      vertical;
	int           iconsize, numBarbs, i;

	BG_UnpackAmmoArray( ps->weapon, ps->ammo, ps->powerups, &numBarbs, NULL );

	if( height > width )
	{
		vertical = qtrue;
		iconsize = width;
	}
	else if( height <= width )
	{
		vertical = qfalse;
		iconsize = height;
	}

	if( color[ 3 ] != 0.0 )
	{
		trap_R_SetColor( color );
	}

	for( i = 0; i < numBarbs; i++ )
	{
		if( vertical )
		{
			y += iconsize;
		}
		else
		{
			x += iconsize;
		}

		CG_DrawPic( x, y, iconsize, iconsize, shader );
	}

	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawPlayerWallclimbing
==============
*/
static void CG_DrawPlayerWallclimbing( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	qboolean      ww = ps->stats[ STAT_STATE ] & SS_WALLCLIMBING;

	if( ww )
	{
		color[ 3 ] = AH_MAX_ALPHA;
	}
	else
	{
		color[ 3 ] = AH_MIN_ALPHA;
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

static void CG_DrawPlayerStamina( rectDef_t *rect, vec4_t color, float scale,
                                  int align, int textStyle, int special )
{
	playerState_t *ps = &cg.snap->ps;
	int           stamina = ps->stats[ STAT_STAMINA ];
	float         progress = ( ( float ) stamina + ( float ) MAX_STAMINA ) / ( ( float ) MAX_STAMINA * 2.0f );

	CG_DrawProgressBar( rect, color, scale, align, textStyle, special, progress );
}

static void CG_DrawPlayerAmmoValue( rectDef_t *rect, vec4_t color )
{
	int           value;
	centity_t     *cent;
	playerState_t *ps;

	cent = &cg_entities[ cg.snap->ps.clientNum ];
	ps = &cg.snap->ps;

	if( cent->currentState.weapon )
	{
		switch( cent->currentState.weapon )
		{
			case WP_ABUILD:
			case WP_ABUILD2:
				//percentage of BP remaining
				value = cgs.alienBuildPoints;
				break;

			case WP_HBUILD:
			case WP_HBUILD2:
				//percentage of BP remaining
				value = cgs.humanBuildPoints;
				break;

			default:
				BG_UnpackAmmoArray( cent->currentState.weapon, ps->ammo, ps->powerups, &value, NULL );
				break;
		}

		if( value > 999 )
		{
			value = 999;
		}

		if( value > -1 )
		{
			trap_R_SetColor( color );
			CG_DrawField( rect->x, rect->y, 4, rect->w / 4, rect->h, value );
			trap_R_SetColor( NULL );
		}
	}
}

/*
==============
CG_DrawAlienSense
==============
*/
static void CG_DrawAlienSense( rectDef_t *rect )
{
	if( BG_ClassHasAbility( cg.snap->ps.stats[ STAT_PCLASS ], SCA_ALIENSENSE ) )
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
	if( BG_InventoryContainsUpgrade( UP_HELMET, cg.snap->ps.stats ) )
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

	if( es->eType == ET_BUILDABLE && BG_FindUsableForBuildable( es->modelindex ) &&
	    cg.predictedPlayerState.stats[ STAT_PTEAM ] == BG_FindTeamForBuildable( es->modelindex ) )
	{
		//hack to prevent showing the usable buildable when you aren't carrying an energy weapon
		if( ( es->modelindex == BA_H_REACTOR || es->modelindex == BA_H_REPEATER ) &&
		    ( !BG_FindUsesEnergyForWeapon( cg.snap->ps.weapon ) ||
		      BG_FindInfinteAmmoForWeapon( cg.snap->ps.weapon ) ) )
		{
			return;
		}

		trap_R_SetColor( color );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
		trap_R_SetColor( NULL );
	}
}

#define BUILD_DELAY_TIME 2000

static void CG_DrawPlayerBuildTimer( rectDef_t *rect, vec4_t color )
{
	float         progress;
	int           index;
	centity_t     *cent;
	playerState_t *ps;

	cent = &cg_entities[ cg.snap->ps.clientNum ];
	ps = &cg.snap->ps;

	if( cent->currentState.weapon )
	{
		switch( cent->currentState.weapon )
		{
			case WP_ABUILD:
				progress = ( float ) ps->stats[ STAT_MISC ] / ( float ) ABUILDER_BASE_DELAY;
				break;

			case WP_ABUILD2:
				progress = ( float ) ps->stats[ STAT_MISC ] / ( float ) ABUILDER_ADV_DELAY;
				break;

			case WP_HBUILD:
				progress = ( float ) ps->stats[ STAT_MISC ] / ( float ) HBUILD_DELAY;
				break;

			case WP_HBUILD2:
				progress = ( float ) ps->stats[ STAT_MISC ] / ( float ) HBUILD2_DELAY;
				break;

			default:
				return;
				break;
		}

		if( !ps->stats[ STAT_MISC ] )
		{
			return;
		}

		index = ( int )( progress * 8.0f );

		if( index > 7 )
		{
			index = 7;
		}
		else if( index < 0 )
		{
			index = 0;
		}

		if( cg.time - cg.lastBuildAttempt <= BUILD_DELAY_TIME )
		{
			if( ( ( cg.time - cg.lastBuildAttempt ) / 300 ) % 2 )
			{
				color[ 0 ] = 1.0f;
				color[ 1 ] = color[ 2 ] = 0.0f;
				color[ 3 ] = 1.0f;
			}
		}

		trap_R_SetColor( color );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h,
		            cgs.media.buildWeaponTimerPie[ index ] );
		trap_R_SetColor( NULL );
	}
}

static void CG_DrawPlayerClipsValue( rectDef_t *rect, vec4_t color )
{
	int           value;
	centity_t     *cent;
	playerState_t *ps;

	cent = &cg_entities[ cg.snap->ps.clientNum ];
	ps = &cg.snap->ps;

	if( cent->currentState.weapon )
	{
		switch( cent->currentState.weapon )
		{
			case WP_ABUILD:
			case WP_ABUILD2:
			case WP_HBUILD:
			case WP_HBUILD2:
				break;

			default:
				BG_UnpackAmmoArray( cent->currentState.weapon, ps->ammo, ps->powerups, NULL, &value );

				if( value > -1 )
				{
					trap_R_SetColor( color );
					CG_DrawField( rect->x, rect->y, 4, rect->w / 4, rect->h, value );
					trap_R_SetColor( NULL );
				}

				break;
		}
	}
}

static void CG_DrawPlayerHealthValue( rectDef_t *rect, vec4_t color )
{
	playerState_t *ps;
	int           value;

	ps = &cg.snap->ps;

	value = ps->stats[ STAT_HEALTH ];

	trap_R_SetColor( color );
	CG_DrawField( rect->x, rect->y, 4, rect->w / 4, rect->h, value );
	trap_R_SetColor( NULL );
}

static void CG_DrawPlayerHealthBar( rectDef_t *rect, vec4_t color, float scale,
                                    int align, int textStyle, int special )
{
	playerState_t *ps;
	float         total;

	ps = &cg.snap->ps;

	total = ( ( float ) ps->stats[ STAT_HEALTH ] / ( float ) ps->stats[ STAT_MAX_HEALTH ] );
	CG_DrawProgressBar( rect, color, scale, align, textStyle, special, total );
}

/*
==============
CG_DrawPlayerHealthCross
==============
*/
static void CG_DrawPlayerHealthCross( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	playerState_t *ps = &cg.snap->ps;
	int           health = ps->stats[ STAT_HEALTH ];

	if( health < 10 )
	{
		color[ 0 ] = 1.0f;
		color[ 1 ] = color[ 2 ] = 0.0f;
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

static void CG_DrawProgressLabel( rectDef_t *rect, float text_x, float text_y, vec4_t color,
                                  float scale, int align, const char *s, float fraction )
{
	vec4_t white = { 1.0f, 1.0f, 1.0f, 1.0f };
	float  tx, tw = CG_Text_Width( s, scale, 0 );

	switch( align )
	{
		case ITEM_ALIGN_LEFT:
			tx = 0.0f;
			break;

		case ITEM_ALIGN_RIGHT:
			tx = rect->w - tw;
			break;

		case ITEM_ALIGN_CENTER:
			tx = ( rect->w / 2.0f ) - ( tw / 2.0f );
			break;

		default:
			tx = 0.0f;
	}

	if( fraction < 1.0f )
	{
		CG_Text_Paint( rect->x + text_x + tx, rect->y + text_y, scale, white,
		               s, 0, 0, ITEM_TEXTSTYLE_NORMAL );
	}
	else
	{
		CG_Text_Paint( rect->x + text_x + tx, rect->y + text_y, scale, color,
		               s, 0, 0, ITEM_TEXTSTYLE_NEON );
	}
}

static void CG_DrawMediaProgress( rectDef_t *rect, vec4_t color, float scale,
                                  int align, int textStyle, int special )
{
	CG_DrawProgressBar( rect, color, scale, align, textStyle, special, cg.mediaFraction );
}

static void CG_DrawMediaProgressLabel( rectDef_t *rect, float text_x, float text_y,
                                       vec4_t color, float scale, int align )
{
	CG_DrawProgressLabel( rect, text_x, text_y, color, scale, align, "Map and Textures", cg.mediaFraction );
}

static void CG_DrawBuildablesProgress( rectDef_t *rect, vec4_t color, float scale,
                                       int align, int textStyle, int special )
{
	CG_DrawProgressBar( rect, color, scale, align, textStyle, special, cg.buildablesFraction );
}

static void CG_DrawBuildablesProgressLabel( rectDef_t *rect, float text_x, float text_y,
    vec4_t color, float scale, int align )
{
	CG_DrawProgressLabel( rect, text_x, text_y, color, scale, align, "Buildable Models", cg.buildablesFraction );
}

static void CG_DrawCharModelProgress( rectDef_t *rect, vec4_t color, float scale,
                                      int align, int textStyle, int special )
{
	CG_DrawProgressBar( rect, color, scale, align, textStyle, special, cg.charModelFraction );
}

static void CG_DrawCharModelProgressLabel( rectDef_t *rect, float text_x, float text_y,
    vec4_t color, float scale, int align )
{
	CG_DrawProgressLabel( rect, text_x, text_y, color, scale, align, "Character Models", cg.charModelFraction );
}

static void CG_DrawOverallProgress( rectDef_t *rect, vec4_t color, float scale,
                                    int align, int textStyle, int special )
{
	float total;

	total = ( cg.charModelFraction + cg.buildablesFraction + cg.mediaFraction ) / 3.0f;
	CG_DrawProgressBar( rect, color, scale, align, textStyle, special, total );
}

static void CG_DrawLevelShot( rectDef_t *rect )
{
	const char *s;
	const char *info;
	qhandle_t  levelshot;
	qhandle_t  detail;

	info = CG_ConfigString( CS_SERVERINFO );
	s = Info_ValueForKey( info, "mapname" );
	levelshot = trap_R_RegisterShaderNoMip( va( "levelshots/%s.tga", s ) );

	if( !levelshot )
	{
		levelshot = trap_R_RegisterShaderNoMip( "gfx/2d/load_screen" );
	}

	trap_R_SetColor( NULL );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, levelshot );

	// blend a detail texture over it
	detail = trap_R_RegisterShader( "gfx/misc/detail" );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, detail );
}

static void CG_DrawLoadingString( rectDef_t *rect, float text_x, float text_y, vec4_t color,
                                  float scale, int align, int textStyle, const char *s )
{
	float tw, th, tx;
	int   pos, i;
	char  buffer[ 1024 ];
	char  *end;

	if( !s[ 0 ] )
	{
		return;
	}

	strcpy( buffer, s );
	tw = CG_Text_Width( s, scale, 0 );
	th = scale * 40.0f;

	pos = i = 0;

	while( pos < strlen( s ) )
	{
		strcpy( buffer, &s[ pos ] );
		tw = CG_Text_Width( buffer, scale, 0 );

		while( tw > rect->w )
		{
			end = strrchr( buffer, ' ' );

			if( end == NULL )
			{
				break;
			}

			*end = '\0';
			tw = CG_Text_Width( buffer, scale, 0 );
		}

		switch( align )
		{
			case ITEM_ALIGN_LEFT:
				tx = rect->x;
				break;

			case ITEM_ALIGN_RIGHT:
				tx = rect->x + rect->w - tw;
				break;

			case ITEM_ALIGN_CENTER:
				tx = rect->x + ( rect->w / 2.0f ) - ( tw / 2.0f );
				break;

			default:
				tx = 0.0f;
		}

		CG_Text_Paint( tx + text_x, rect->y + text_y + i * ( th + 3 ), scale, color,
		               buffer, 0, 0, textStyle );

		pos += strlen( buffer ) + 1;
		i++;
	}
}

static void CG_DrawLevelName( rectDef_t *rect, float text_x, float text_y,
                              vec4_t color, float scale, int align, int textStyle )
{
	const char *s;

	s = CG_ConfigString( CS_MESSAGE );

	CG_DrawLoadingString( rect, text_x, text_y, color, scale, align, textStyle, s );
}

static void CG_DrawMOTD( rectDef_t *rect, float text_x, float text_y,
                         vec4_t color, float scale, int align, int textStyle )
{
	const char *s;

	s = CG_ConfigString( CS_MOTD );

	CG_DrawLoadingString( rect, text_x, text_y, color, scale, align, textStyle, s );
}

static void CG_DrawHostname( rectDef_t *rect, float text_x, float text_y,
                             vec4_t color, float scale, int align, int textStyle )
{
	char       buffer[ 1024 ];
	const char *info;

	info = CG_ConfigString( CS_SERVERINFO );

	Q_strncpyz( buffer, Info_ValueForKey( info, "sv_hostname" ), 1024 );
	Q_CleanStr( buffer );

	CG_DrawLoadingString( rect, text_x, text_y, color, scale, align, textStyle, buffer );
}

/*
==============
CG_DrawDemoPlayback
==============
*/
static void CG_DrawDemoPlayback( rectDef_t *rect, vec4_t color, qhandle_t shader )
{
	if( !cg_drawDemoState.integer )
	{
		return;
	}

	if( trap_GetDemoState() != DS_PLAYBACK )
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
	if( !cg_drawDemoState.integer )
	{
		return;
	}

	if( trap_GetDemoState() != DS_RECORDING )
	{
		return;
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
======================
CG_UpdateMediaFraction

======================
*/
void CG_UpdateMediaFraction( float newFract )
{
	cg.mediaFraction = newFract;

	trap_UpdateScreen();
}

/*
====================
CG_DrawLoadingScreen

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawLoadingScreen( void )
{
	Menu_Paint( Menus_FindByName( "Loading" ), qtrue );
}

float CG_GetValue( int ownerDraw )
{
	centity_t     *cent;
	playerState_t *ps;

	cent = &cg_entities[ cg.snap->ps.clientNum ];
	ps = &cg.snap->ps;

	switch( ownerDraw )
	{
		case CG_PLAYER_AMMO_VALUE:
			if( cent->currentState.weapon )
			{
				int value;

				BG_UnpackAmmoArray( cent->currentState.weapon, ps->ammo, ps->powerups,
				                    &value, NULL );

				return value;
			}

			break;

		case CG_PLAYER_CLIPS_VALUE:
			if( cent->currentState.weapon )
			{
				int value;

				BG_UnpackAmmoArray( cent->currentState.weapon, ps->ammo, ps->powerups,
				                    NULL, &value );

				return value;
			}

			break;

		case CG_PLAYER_HEALTH:
			return ps->stats[ STAT_HEALTH ];
			break;

		default:
			break;
	}

	return -1;
}

static void CG_DrawAreaSystemChat( rectDef_t *rect, float scale, vec4_t color, qhandle_t shader )
{
	CG_Text_Paint( rect->x, rect->y + rect->h, scale, color, systemChat, 0, 0, 0 );
}

static void CG_DrawAreaTeamChat( rectDef_t *rect, float scale, vec4_t color, qhandle_t shader )
{
	CG_Text_Paint( rect->x, rect->y + rect->h, scale, color, teamChat1, 0, 0, 0 );
}

static void CG_DrawAreaChat( rectDef_t *rect, float scale, vec4_t color, qhandle_t shader )
{
	CG_Text_Paint( rect->x, rect->y + rect->h, scale, color, teamChat2, 0, 0, 0 );
}

const char *CG_GetKillerText()
{
	const char *s = "";

	if( cg.killerName[ 0 ] )
	{
		s = va( "Fragged by %s", cg.killerName );
	}

	return s;
}

static void CG_DrawKiller( rectDef_t *rect, float scale, vec4_t color,
                           qhandle_t shader, int textStyle )
{
	// fragged by ... line
	if( cg.killerName[ 0 ] )
	{
		int x = rect->x + rect->w / 2;
		CG_Text_Paint( x - CG_Text_Width( CG_GetKillerText(), scale, 0 ) / 2,
		               rect->y + rect->h, scale, color, CG_GetKillerText(), 0, 0, textStyle );
	}
}

static void CG_Text_Paint_Limit( float *maxX, float x, float y, float scale,
                                 vec4_t color, const char *text, float adjust, int limit )
{
	int         len, count;
	vec4_t      newColor;
	glyphInfo_t *glyph;

	if( text )
	{
// TTimo: FIXME
//    const unsigned char *s = text; // bk001206 - unsigned
		const char *s = text;
		float      max = *maxX;
		float      useScale;
		fontInfo_t *font = &cgDC.Assets.textFont;

		if( scale <= cg_smallFont.value )
		{
			font = &cgDC.Assets.smallFont;
		}
		else if( scale > cg_bigFont.value )
		{
			font = &cgDC.Assets.bigFont;
		}

		useScale = scale * font->glyphScale;
		trap_R_SetColor( color );
		len = strlen( text );

		if( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while( s && *s && count < len )
		{
			glyph = &font->glyphs[( int ) * s ];
			//TTimo: FIXME: getting nasty warnings without the cast,
			//hopefully this doesn't break the VM build

			if( Q_IsColorString( s ) )
			{
				memcpy( newColor, g_color_table[ ColorIndex( * ( s + 1 ) ) ], sizeof( newColor ) );
				newColor[ 3 ] = color[ 3 ];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			}
			else
			{
				float yadj = useScale * glyph->top;

				if( CG_Text_Width( s, useScale, 1 ) + x > max )
				{
					*maxX = 0;
					break;
				}

				CG_Text_PaintChar( x, y - yadj,
				                   glyph->imageWidth,
				                   glyph->imageHeight,
				                   useScale,
				                   glyph->s,
				                   glyph->t,
				                   glyph->s2,
				                   glyph->t2,
				                   glyph->glyph );
				x += ( glyph->xSkip * useScale ) + adjust;
				*maxX = x;
				count++;
				s++;
			}
		}

		trap_R_SetColor( NULL );
	}
}

static void CG_DrawTeamSpectators( rectDef_t *rect, float scale, vec4_t color, qhandle_t shader )
{
	if( cg.spectatorLen )
	{
		float maxX;

		if( cg.spectatorWidth == -1 )
		{
			cg.spectatorWidth = 0;
			cg.spectatorPaintX = rect->x + 1;
			cg.spectatorPaintX2 = -1;
		}

		if( cg.spectatorOffset > cg.spectatorLen )
		{
			cg.spectatorOffset = 0;
			cg.spectatorPaintX = rect->x + 1;
			cg.spectatorPaintX2 = -1;
		}

		if( cg.time > cg.spectatorTime )
		{
			cg.spectatorTime = cg.time + 10;

			if( cg.spectatorPaintX <= rect->x + 2 )
			{
				if( cg.spectatorOffset < cg.spectatorLen )
				{
					//TA: skip colour directives
					if( Q_IsColorString( &cg.spectatorList[ cg.spectatorOffset ] ) )
					{
						cg.spectatorOffset += 2;
					}
					else
					{
						cg.spectatorPaintX += CG_Text_Width( &cg.spectatorList[ cg.spectatorOffset ], scale, 1 ) - 1;
						cg.spectatorOffset++;
					}
				}
				else
				{
					cg.spectatorOffset = 0;

					if( cg.spectatorPaintX2 >= 0 )
					{
						cg.spectatorPaintX = cg.spectatorPaintX2;
					}
					else
					{
						cg.spectatorPaintX = rect->x + rect->w - 2;
					}

					cg.spectatorPaintX2 = -1;
				}
			}
			else
			{
				cg.spectatorPaintX--;

				if( cg.spectatorPaintX2 >= 0 )
				{
					cg.spectatorPaintX2--;
				}
			}
		}

		maxX = rect->x + rect->w - 2;

		CG_Text_Paint_Limit( &maxX, cg.spectatorPaintX, rect->y + rect->h - 3, scale, color,
		                     &cg.spectatorList[ cg.spectatorOffset ], 0, 0 );

		if( cg.spectatorPaintX2 >= 0 )
		{
			float maxX2 = rect->x + rect->w - 2;
			CG_Text_Paint_Limit( &maxX2, cg.spectatorPaintX2, rect->y + rect->h - 3, scale,
			                     color, cg.spectatorList, 0, cg.spectatorOffset );
		}

		if( cg.spectatorOffset && maxX > 0 )
		{
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if( cg.spectatorPaintX2 == -1 )
			{
				cg.spectatorPaintX2 = rect->x + rect->w - 2;
			}
		}
		else
		{
			cg.spectatorPaintX2 = -1;
		}
	}
}

/*
==================
CG_DrawStageReport
==================
*/
static void CG_DrawStageReport( rectDef_t *rect, float text_x, float text_y,
                                vec4_t color, float scale, int align, int textStyle )
{
	char s[ MAX_TOKEN_CHARS ];
	int  tx, w, kills;

	if( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_SPECTATOR && !cg.intermissionStarted )
	{
		return;
	}

	if( cg.intermissionStarted )
	{
		Com_sprintf( s, MAX_TOKEN_CHARS,
		             "Stage %d" //PH34R MY MAD-LEET CODING SKILLZ
		             "                                                       "
		             "Stage %d",
		             cgs.alienStage + 1, cgs.humanStage + 1 );
	}
	else if( cg.snap->ps.stats[ STAT_PTEAM ] == PTE_ALIENS )
	{
		kills = cgs.alienNextStageThreshold - cgs.alienKills;

		if( cgs.alienNextStageThreshold < 0 )
		{
			Com_sprintf( s, MAX_TOKEN_CHARS, "Stage %d", cgs.alienStage + 1 );
		}
		else if( kills == 1 )
		{
			Com_sprintf( s, MAX_TOKEN_CHARS, "Stage %d, %d kill for next stage",
			             cgs.alienStage + 1, kills );
		}
		else
		{
			Com_sprintf( s, MAX_TOKEN_CHARS, "Stage %d, %d kills for next stage",
			             cgs.alienStage + 1, kills );
		}
	}
	else if( cg.snap->ps.stats[ STAT_PTEAM ] == PTE_HUMANS )
	{
		kills = cgs.humanNextStageThreshold - cgs.humanKills;

		if( cgs.humanNextStageThreshold < 0 )
		{
			Com_sprintf( s, MAX_TOKEN_CHARS, "Stage %d", cgs.humanStage + 1 );
		}
		else if( kills == 1 )
		{
			Com_sprintf( s, MAX_TOKEN_CHARS, "Stage %d, %d kill for next stage",
			             cgs.humanStage + 1, kills );
		}
		else
		{
			Com_sprintf( s, MAX_TOKEN_CHARS, "Stage %d, %d kills for next stage",
			             cgs.humanStage + 1, kills );
		}
	}

	w = CG_Text_Width( s, scale, 0 );

	switch( align )
	{
		case ITEM_ALIGN_LEFT:
			tx = rect->x;
			break;

		case ITEM_ALIGN_RIGHT:
			tx = rect->x + rect->w - w;
			break;

		case ITEM_ALIGN_CENTER:
			tx = rect->x + ( rect->w / 2.0f ) - ( w / 2.0f );
			break;

		default:
			tx = 0.0f;
	}

	CG_Text_Paint( text_x + tx, rect->y + text_y, scale, color, s, 0, 0, textStyle );
}

/*
==================
CG_DrawFPS
==================
*/
//TA: personally i think this should be longer - it should really be a cvar
#define FPS_FRAMES 20
#define FPS_STRING "fps"
static void CG_DrawFPS( rectDef_t *rect, float text_x, float text_y,
                        float scale, vec4_t color, int align, int textStyle,
                        qboolean scalableText )
{
	char       *s;
	int        tx, w, totalWidth, strLength;
	static int previousTimes[ FPS_FRAMES ];
	static int index;
	int        i, total;
	int        fps;
	static int previous;
	int        t, frameTime;

	if( !cg_drawFPS.integer )
	{
		return;
	}

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[ index % FPS_FRAMES ] = frameTime;
	index++;

	if( index > FPS_FRAMES )
	{
		// average multiple frames together to smooth changes out a bit
		total = 0;

		for( i = 0; i < FPS_FRAMES; i++ )
		{
			total += previousTimes[ i ];
		}

		if( !total )
		{
			total = 1;
		}

		fps = 1000 * FPS_FRAMES / total;

		s = va( "%d", fps );
		w = CG_Text_Width( "0", scale, 0 );
		strLength = CG_DrawStrlen( s );
		totalWidth = CG_Text_Width( FPS_STRING, scale, 0 ) + w * strLength;

		switch( align )
		{
			case ITEM_ALIGN_LEFT:
				tx = rect->x;
				break;

			case ITEM_ALIGN_RIGHT:
				tx = rect->x + rect->w - totalWidth;
				break;

			case ITEM_ALIGN_CENTER:
				tx = rect->x + ( rect->w / 2.0f ) - ( totalWidth / 2.0f );
				break;

			default:
				tx = 0.0f;
		}

		if( scalableText )
		{
			for( i = 0; i < strLength; i++ )
			{
				char c[ 2 ];

				c[ 0 ] = s[ i ];
				c[ 1 ] = '\0';

				CG_Text_Paint( text_x + tx + i * w, rect->y + text_y, scale, color, c, 0, 0, textStyle );
			}
		}
		else
		{
			trap_R_SetColor( color );
			CG_DrawField( rect->x, rect->y, 3, rect->w / 3, rect->h, fps );
			trap_R_SetColor( NULL );
		}

		if( scalableText )
		{
			CG_Text_Paint( text_x + tx + i * w, rect->y + text_y, scale, color, FPS_STRING, 0, 0, textStyle );
		}
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

	if( !cg_drawTimer.integer )
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

	if( !cg_drawTimer.integer )
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
static void CG_DrawTimer( rectDef_t *rect, float text_x, float text_y,
                          float scale, vec4_t color, int align, int textStyle )
{
	char *s;
	int  i, tx, w, totalWidth, strLength;
	int  mins, seconds, tens;
	int  msec;

	if( !cg_drawTimer.integer )
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
	w = CG_Text_Width( "0", scale, 0 );
	strLength = CG_DrawStrlen( s );
	totalWidth = w * strLength;

	switch( align )
	{
		case ITEM_ALIGN_LEFT:
			tx = rect->x;
			break;

		case ITEM_ALIGN_RIGHT:
			tx = rect->x + rect->w - totalWidth;
			break;

		case ITEM_ALIGN_CENTER:
			tx = rect->x + ( rect->w / 2.0f ) - ( totalWidth / 2.0f );
			break;

		default:
			tx = 0.0f;
	}

	for( i = 0; i < strLength; i++ )
	{
		char c[ 2 ];

		c[ 0 ] = s[ i ];
		c[ 1 ] = '\0';

		CG_Text_Paint( text_x + tx + i * w, rect->y + text_y, scale, color, c, 0, 0, textStyle );
	}
}

/*
==================
CG_DrawSnapshot
==================
*/
static void CG_DrawSnapshot( rectDef_t *rect, float text_x, float text_y,
                             float scale, vec4_t color, int align, int textStyle )
{
	char *s;
	int  w, tx;

	if( !cg_drawSnapshot.integer )
	{
		return;
	}

	s = va( "time:%d snap:%d cmd:%d", cg.snap->serverTime,
	        cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_Text_Width( s, scale, 0 );

	switch( align )
	{
		case ITEM_ALIGN_LEFT:
			tx = rect->x;
			break;

		case ITEM_ALIGN_RIGHT:
			tx = rect->x + rect->w - w;
			break;

		case ITEM_ALIGN_CENTER:
			tx = rect->x + ( rect->w / 2.0f ) - ( w / 2.0f );
			break;

		default:
			tx = 0.0f;
	}

	CG_Text_Paint( text_x + tx, rect->y + text_y, scale, color, s, 0, 0, textStyle );
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
void CG_AddLagometerSnapshotInfo( snapshot_t *snap )
{
	// dropped packet
	if( !snap )
	{
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->snapFlags;
	lagometer.snapshotCount++;
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
	if( cmd.serverTime <= cg.snap->ps.commandTime || cmd.serverTime > cg.time )
	{
		return;
	}

	// also add text in center of screen
	s = "Connection Interrupted";
	w = CG_Text_Width( s, 0.7f, 0 );
	CG_Text_Paint( 320 - w / 2, 100, 0.7f, color, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED );

	// blink the icon
	if( ( cg.time >> 9 ) & 1 )
	{
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader( "gfx/2d/net.tga" ) );
}

#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

#define PING_FRAMES         40

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
	vec4_t white = { 1.0f, 1.0f, 1.0f, 1.0f };

	if( cg.snap->ps.pm_type == PM_INTERMISSION )
	{
		return;
	}

	if( !cg_lagometer.integer )
	{
		return;
	}

	if( cg.demoPlayback )
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
	for( a = 0; a < aw; a++ )
	{
		i = ( lagometer.frameCount - 1 - a ) & ( LAG_SAMPLES - 1 );
		v = lagometer.frameSamples[ i ];
		v *= vscale;

		if( v > 0 )
		{
			if( color != 1 )
			{
				color = 1;
				trap_R_SetColor( g_color_table[ ColorIndex( COLOR_YELLOW ) ] );
			}

			if( v > range )
			{
				v = range;
			}

			trap_R_DrawStretchPic( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
		else if( v < 0 )
		{
			if( color != 2 )
			{
				color = 2;
				trap_R_SetColor( g_color_table[ ColorIndex( COLOR_BLUE ) ] );
			}

			v = -v;

			if( v > range )
			{
				v = range;
			}

			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for( a = 0; a < aw; a++ )
	{
		i = ( lagometer.snapshotCount - 1 - a ) & ( LAG_SAMPLES - 1 );
		v = lagometer.snapshotSamples[ i ];

		if( v > 0 )
		{
			if( lagometer.snapshotFlags[ i ] & SNAPFLAG_RATE_DELAYED )
			{
				if( color != 5 )
				{
					color = 5; // YELLOW for rate delay
					trap_R_SetColor( g_color_table[ ColorIndex( COLOR_YELLOW ) ] );
				}
			}
			else
			{
				if( color != 3 )
				{
					color = 3;

					trap_R_SetColor( g_color_table[ ColorIndex( COLOR_GREEN ) ] );
				}
			}

			v = v * vscale;

			if( v > range )
			{
				v = range;
			}

			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
		else if( v < 0 )
		{
			if( color != 4 )
			{
				color = 4; // RED for dropped snapshots
				trap_R_SetColor( g_color_table[ ColorIndex( COLOR_RED ) ] );
			}

			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if( cg_nopredict.integer || cg_synchronousClients.integer )
	{
		CG_Text_Paint( ax, ay, 0.5, white, "snc", 0, 0, ITEM_TEXTSTYLE_NORMAL );
	}
	else
	{
		static int previousPings[ PING_FRAMES ];
		static int index;
		int        i, ping = 0;
		char       *s;

		previousPings[ index++ ] = cg.snap->ping;
		index = index % PING_FRAMES;

		for( i = 0; i < PING_FRAMES; i++ )
		{
			ping += previousPings[ i ];
		}

		ping /= PING_FRAMES;

		s = va( "%d", ping );
		ax = rect->x + ( rect->w / 2.0f ) - ( CG_Text_Width( s, scale, 0 ) / 2.0f ) + text_x;
		ay = rect->y + ( rect->h / 2.0f ) + ( CG_Text_Height( s, scale, 0 ) / 2.0f ) + text_y;

		Vector4Copy( textColor, adjustedColor );
		adjustedColor[ 3 ] = 0.5f;
		CG_Text_Paint( ax, ay, scale, adjustedColor, s, 0, 0, ITEM_TEXTSTYLE_NORMAL );
	}

	CG_DrawDisconnect();
}

/*
==============
CG_DrawTextBlock
==============
*/
static void CG_DrawTextBlock( rectDef_t *rect, float text_x, float text_y, vec4_t color,
                              float scale, int align, int textStyle, const char *text,
                              menuDef_t *parent, itemDef_t *textItem )
{
	float x, y, w, h;

	//offset the text
	x = rect->x;
	y = rect->y;
	w = rect->w - ( 16 + ( 2 * text_x ) ); //16 to ensure text within frame
	h = rect->h;

	textItem->text = text;

	textItem->parent = parent;
	memcpy( textItem->window.foreColor, color, sizeof( vec4_t ) );
	textItem->window.flags = 0;

	switch( align )
	{
		case ITEM_ALIGN_LEFT:
			textItem->window.rect.x = x;
			break;

		case ITEM_ALIGN_RIGHT:
			textItem->window.rect.x = x + w;
			break;

		case ITEM_ALIGN_CENTER:
			textItem->window.rect.x = x + ( w / 2 );
			break;

		default:
			textItem->window.rect.x = x;
			break;
	}

	textItem->window.rect.y = y;
	textItem->window.rect.w = w;
	textItem->window.rect.h = h;
	textItem->window.borderSize = 0;
	textItem->textRect.x = 0;
	textItem->textRect.y = 0;
	textItem->textRect.w = 0;
	textItem->textRect.h = 0;
	textItem->textalignment = align;
	textItem->textalignx = text_x;
	textItem->textaligny = text_y;
	textItem->textscale = scale;
	textItem->textStyle = textStyle;

	//hack to utilise existing autowrap code
	Item_Text_AutoWrapped_Paint( textItem );
}

/*
===================
CG_DrawConsole
===================
*/
static void CG_DrawConsole( rectDef_t *rect, float text_x, float text_y, vec4_t color,
                            float scale, int align, int textStyle )
{
	static menuDef_t dummyParent;
	static itemDef_t textItem;

	CG_DrawTextBlock( rect, text_x, text_y, color, scale, align, textStyle,
	                  cg.consoleText, &dummyParent, &textItem );
}

/*
===================
CG_DrawTutorial
===================
*/
static void CG_DrawTutorial( rectDef_t *rect, float text_x, float text_y, vec4_t color,
                             float scale, int align, int textStyle )
{
	static menuDef_t dummyParent;
	static itemDef_t textItem;

	if( !cg_tutorial.integer )
	{
		return;
	}

	CG_DrawTextBlock( rect, text_x, text_y, color, scale, align, textStyle,
	                  CG_TutorialText(), &dummyParent, &textItem );
}

/*
===================
CG_DrawWeaponIcon
===================
*/
void CG_DrawWeaponIcon( rectDef_t *rect, vec4_t color )
{
	int           ammo, clips, maxAmmo;
	centity_t     *cent;
	playerState_t *ps;

	cent = &cg_entities[ cg.snap->ps.clientNum ];
	ps = &cg.snap->ps;

	BG_UnpackAmmoArray( cent->currentState.weapon, ps->ammo, ps->powerups, &ammo, &clips );
	BG_FindAmmoForWeapon( cent->currentState.weapon, &maxAmmo, NULL );

	// don't display if dead
	if( cg.predictedPlayerState.stats[ STAT_HEALTH ] <= 0 )
	{
		return;
	}

	if( cent->currentState.weapon == 0 )
	{
		return;
	}

	CG_RegisterWeapon( cent->currentState.weapon );

	if( clips == 0 && !BG_FindInfinteAmmoForWeapon( cent->currentState.weapon ) )
	{
		float ammoPercent = ( float ) ammo / ( float ) maxAmmo;

		if( ammoPercent < 0.33f )
		{
			color[ 0 ] = 1.0f;
			color[ 1 ] = color[ 2 ] = 0.0f;
		}
	}

	if( cg.predictedPlayerState.stats[ STAT_PTEAM ] == PTE_ALIENS && CG_AtHighestClass() )
	{
		if( cg.time - cg.lastEvolveAttempt <= NO_CREDITS_TIME )
		{
			if( ( ( cg.time - cg.lastEvolveAttempt ) / 300 ) % 2 )
			{
				color[ 3 ] = 0.0f;
			}
		}
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_weapons[ cent->currentState.weapon ].weaponIcon );
	trap_R_SetColor( NULL );
}

/*
================================================================================

CROSSHAIR

================================================================================
*/

/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair( void )
{
	float        w, h;
	qhandle_t    hShader;
	float        x, y;
	weaponInfo_t *wi;

	if( !cg_drawCrosshair.integer )
	{
		return;
	}

	if( ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_SPECTATOR ) ||
	    ( cg.snap->ps.stats[ STAT_STATE ] & SS_INFESTING ) ||
	    ( cg.snap->ps.stats[ STAT_STATE ] & SS_HOVELING ) )
	{
		return;
	}

	if( cg.renderingThirdPerson )
	{
		return;
	}

	wi = &cg_weapons[ cg.snap->ps.weapon ];

	w = h = wi->crossHairSize;

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	CG_AdjustFrom640( &x, &y, &w, &h );

	hShader = wi->crossHair;

	if( hShader != 0 )
	{
		trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * ( cg.refdef.width - w ),
		                       y + cg.refdef.y + 0.5 * ( cg.refdef.height - h ),
		                       w, h, 0, 0, 1, 1, hShader );
	}
}

/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void )
{
	trace_t trace;
	vec3_t  start, end;
	int     content;
	pTeam_t team;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[ 0 ], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
	          cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY );

	if( trace.entityNum >= MAX_CLIENTS )
	{
		return;
	}

	// if the player is in fog, don't show it
	content = trap_CM_PointContents( trace.endpos, 0 );

	if( content & CONTENTS_FOG )
	{
		return;
	}

	team = cgs.clientinfo[ trace.entityNum ].team;

	if( cg.snap->ps.persistant[ PERS_TEAM ] != TEAM_SPECTATOR )
	{
		//only display team names of those on the same team as this player
		if( team != cg.snap->ps.stats[ STAT_PTEAM ] )
		{
			return;
		}
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
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

	if( !cg_drawCrosshair.integer )
	{
		return;
	}

	if( !cg_drawCrosshairNames.integer )
	{
		return;
	}

	if( cg.renderingThirdPerson )
	{
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );

	if( !color )
	{
		trap_R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;
	w = CG_Text_Width( name, scale, 0 );
	x = rect->x + rect->w / 2;
	CG_Text_Paint( x - w / 2, rect->y + rect->h, scale, color, name, 0, 0, textStyle );
	trap_R_SetColor( NULL );
}

/*
===============
CG_OwnerDraw

Draw an owner drawn item
===============
*/
void CG_OwnerDraw( float x, float y, float w, float h, float text_x,
                   float text_y, int ownerDraw, int ownerDrawFlags,
                   int align, float special, float scale, vec4_t color,
                   qhandle_t shader, int textStyle )
{
	rectDef_t rect;

	if( cg_drawStatus.integer == 0 )
	{
		return;
	}

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	switch( ownerDraw )
	{
		case CG_PLAYER_CREDITS_VALUE:
			CG_DrawPlayerCreditsValue( &rect, color, qtrue );
			break;

		case CG_PLAYER_BANK_VALUE:
			CG_DrawPlayerBankValue( &rect, color, qtrue );
			break;

		case CG_PLAYER_CREDITS_VALUE_NOPAD:
			CG_DrawPlayerCreditsValue( &rect, color, qfalse );
			break;

		case CG_PLAYER_BANK_VALUE_NOPAD:
			CG_DrawPlayerBankValue( &rect, color, qfalse );
			break;

		case CG_PLAYER_STAMINA:
			CG_DrawPlayerStamina( &rect, color, scale, align, textStyle, special );
			break;

		case CG_PLAYER_STAMINA_1:
			CG_DrawPlayerStamina1( &rect, color, shader );
			break;

		case CG_PLAYER_STAMINA_2:
			CG_DrawPlayerStamina2( &rect, color, shader );
			break;

		case CG_PLAYER_STAMINA_3:
			CG_DrawPlayerStamina3( &rect, color, shader );
			break;

		case CG_PLAYER_STAMINA_4:
			CG_DrawPlayerStamina4( &rect, color, shader );
			break;

		case CG_PLAYER_STAMINA_BOLT:
			CG_DrawPlayerStaminaBolt( &rect, color, shader );
			break;

		case CG_PLAYER_AMMO_VALUE:
			CG_DrawPlayerAmmoValue( &rect, color );
			break;

		case CG_PLAYER_CLIPS_VALUE:
			CG_DrawPlayerClipsValue( &rect, color );
			break;

		case CG_PLAYER_BUILD_TIMER:
			CG_DrawPlayerBuildTimer( &rect, color );
			break;

		case CG_PLAYER_HEALTH:
			CG_DrawPlayerHealthValue( &rect, color );
			break;

		case CG_PLAYER_HEALTH_BAR:
			CG_DrawPlayerHealthBar( &rect, color, scale, align, textStyle, special );
			break;

		case CG_PLAYER_HEALTH_CROSS:
			CG_DrawPlayerHealthCross( &rect, color, shader );
			break;

		case CG_PLAYER_CLIPS_RING:
			CG_DrawPlayerClipsRing( &rect, color, shader );
			break;

		case CG_PLAYER_BUILD_TIMER_RING:
			CG_DrawPlayerBuildTimerRing( &rect, color, shader );
			break;

		case CG_PLAYER_WALLCLIMBING:
			CG_DrawPlayerWallclimbing( &rect, color, shader );
			break;

		case CG_PLAYER_BOOSTED:
			CG_DrawPlayerBoosted( &rect, color, shader );
			break;

		case CG_PLAYER_BOOST_BOLT:
			CG_DrawPlayerBoosterBolt( &rect, color, shader );
			break;

		case CG_PLAYER_POISON_BARBS:
			CG_DrawPlayerPoisonBarbs( &rect, color, shader );
			break;

		case CG_PLAYER_ALIEN_SENSE:
			CG_DrawAlienSense( &rect );
			break;

		case CG_PLAYER_HUMAN_SCANNER:
			CG_DrawHumanScanner( &rect, shader, color );
			break;

		case CG_PLAYER_USABLE_BUILDABLE:
			CG_DrawUsableBuildable( &rect, shader, color );
			break;

		case CG_AREA_SYSTEMCHAT:
			CG_DrawAreaSystemChat( &rect, scale, color, shader );
			break;

		case CG_AREA_TEAMCHAT:
			CG_DrawAreaTeamChat( &rect, scale, color, shader );
			break;

		case CG_AREA_CHAT:
			CG_DrawAreaChat( &rect, scale, color, shader );
			break;

		case CG_KILLER:
			CG_DrawKiller( &rect, scale, color, shader, textStyle );
			break;

		case CG_PLAYER_SELECT:
			CG_DrawItemSelect( &rect, color );
			break;

		case CG_PLAYER_WEAPONICON:
			CG_DrawWeaponIcon( &rect, color );
			break;

		case CG_PLAYER_SELECTTEXT:
			CG_DrawItemSelectText( &rect, scale, textStyle );
			break;

		case CG_SPECTATORS:
			CG_DrawTeamSpectators( &rect, scale, color, shader );
			break;

		case CG_PLAYER_CROSSHAIRNAMES:
			CG_DrawCrosshairNames( &rect, scale, textStyle );
			break;

		case CG_STAGE_REPORT_TEXT:
			CG_DrawStageReport( &rect, text_x, text_y, color, scale, align, textStyle );
			break;

			//loading screen
		case CG_LOAD_LEVELSHOT:
			CG_DrawLevelShot( &rect );
			break;

		case CG_LOAD_MEDIA:
			CG_DrawMediaProgress( &rect, color, scale, align, textStyle, special );
			break;

		case CG_LOAD_MEDIA_LABEL:
			CG_DrawMediaProgressLabel( &rect, text_x, text_y, color, scale, align );
			break;

		case CG_LOAD_BUILDABLES:
			CG_DrawBuildablesProgress( &rect, color, scale, align, textStyle, special );
			break;

		case CG_LOAD_BUILDABLES_LABEL:
			CG_DrawBuildablesProgressLabel( &rect, text_x, text_y, color, scale, align );
			break;

		case CG_LOAD_CHARMODEL:
			CG_DrawCharModelProgress( &rect, color, scale, align, textStyle, special );
			break;

		case CG_LOAD_CHARMODEL_LABEL:
			CG_DrawCharModelProgressLabel( &rect, text_x, text_y, color, scale, align );
			break;

		case CG_LOAD_OVERALL:
			CG_DrawOverallProgress( &rect, color, scale, align, textStyle, special );
			break;

		case CG_LOAD_LEVELNAME:
			CG_DrawLevelName( &rect, text_x, text_y, color, scale, align, textStyle );
			break;

		case CG_LOAD_MOTD:
			CG_DrawMOTD( &rect, text_x, text_y, color, scale, align, textStyle );
			break;

		case CG_LOAD_HOSTNAME:
			CG_DrawHostname( &rect, text_x, text_y, color, scale, align, textStyle );
			break;

		case CG_FPS:
			CG_DrawFPS( &rect, text_x, text_y, scale, color, align, textStyle, qtrue );
			break;

		case CG_FPS_FIXED:
			CG_DrawFPS( &rect, text_x, text_y, scale, color, align, textStyle, qfalse );
			break;

		case CG_TIMER:
			CG_DrawTimer( &rect, text_x, text_y, scale, color, align, textStyle );
			break;

		case CG_TIMER_MINS:
			CG_DrawTimerMins( &rect, color );
			break;

		case CG_TIMER_SECS:
			CG_DrawTimerSecs( &rect, color );
			break;

		case CG_SNAPSHOT:
			CG_DrawSnapshot( &rect, text_x, text_y, scale, color, align, textStyle );
			break;

		case CG_LAGOMETER:
			CG_DrawLagometer( &rect, text_x, text_y, scale, color );
			break;

		case CG_DEMO_PLAYBACK:
			CG_DrawDemoPlayback( &rect, color, shader );
			break;

		case CG_DEMO_RECORDING:
			CG_DrawDemoRecording( &rect, color, shader );
			break;

		case CG_CONSOLE:
			CG_DrawConsole( &rect, text_x, text_y, color, scale, align, textStyle );
			break;

		case CG_TUTORIAL:
			CG_DrawTutorial( &rect, text_x, text_y, color, scale, align, textStyle );
			break;

		default:
			break;
	}
}

void CG_MouseEvent( int x, int y )
{
	int n;

	if( ( cg.predictedPlayerState.pm_type == PM_NORMAL ||
	      cg.predictedPlayerState.pm_type == PM_SPECTATOR ) &&
	    cg.showScores == qfalse )
	{
		trap_Key_SetCatcher( 0 );
		return;
	}

	cgs.cursorX += x;

	if( cgs.cursorX < 0 )
	{
		cgs.cursorX = 0;
	}
	else if( cgs.cursorX > 640 )
	{
		cgs.cursorX = 640;
	}

	cgs.cursorY += y;

	if( cgs.cursorY < 0 )
	{
		cgs.cursorY = 0;
	}
	else if( cgs.cursorY > 480 )
	{
		cgs.cursorY = 480;
	}

	n = Display_CursorType( cgs.cursorX, cgs.cursorY );
	cgs.activeCursor = 0;

	if( n == CURSOR_ARROW )
	{
		cgs.activeCursor = cgs.media.selectCursor;
	}
	else if( n == CURSOR_SIZER )
	{
		cgs.activeCursor = cgs.media.sizeCursor;
	}

	if( cgs.capturedItem )
	{
		Display_MouseMove( cgs.capturedItem, x, y );
	}
	else
	{
		Display_MouseMove( NULL, cgs.cursorX, cgs.cursorY );
	}
}

/*
==================
CG_HideTeamMenus
==================

*/
void CG_HideTeamMenu( void )
{
	Menus_CloseByName( "teamMenu" );
	Menus_CloseByName( "getMenu" );
}

/*
==================
CG_ShowTeamMenus
==================

*/
void CG_ShowTeamMenu( void )
{
	Menus_OpenByName( "teamMenu" );
}

/*
==================
CG_EventHandling
==================
 type 0 - no event handling
      1 - team menu
      2 - hud editor

*/
void CG_EventHandling( int type )
{
	cgs.eventHandling = type;

	if( type == CGAME_EVENT_NONE )
	{
		CG_HideTeamMenu();
	}
}

void CG_KeyEvent( int key, qboolean down )
{
	if( !down )
	{
		return;
	}

	if( cg.predictedPlayerState.pm_type == PM_NORMAL ||
	    ( cg.predictedPlayerState.pm_type == PM_SPECTATOR &&
	      cg.showScores == qfalse ) )
	{
		CG_EventHandling( CGAME_EVENT_NONE );
		trap_Key_SetCatcher( 0 );
		return;
	}

	Display_HandleKey( key, down, cgs.cursorX, cgs.cursorY );

	if( cgs.capturedItem )
	{
		cgs.capturedItem = NULL;
	}
	else
	{
		if( key == K_MOUSE2 && down )
		{
			cgs.capturedItem = Display_CaptureItem( cgs.cursorX, cgs.cursorY );
		}
	}
}

int CG_ClientNumFromName( const char *p )
{
	int i;

	for( i = 0; i < cgs.maxclients; i++ )
	{
		if( cgs.clientinfo[ i ].infoValid &&
		    Q_stricmp( cgs.clientinfo[ i ].name, p ) == 0 )
		{
			return i;
		}
	}

	return -1;
}

void CG_RunMenuScript( char **args )
{
}

void CG_GetTeamColor( vec4_t *color )
{
	( *color ) [ 0 ] = ( *color ) [ 2 ] = 0.0f;
	( *color ) [ 1 ] = 0.17f;
	( *color ) [ 3 ] = 0.25f;
}

//END TA UI

/*
================
CG_DrawLighting

================
*/
static void CG_DrawLighting( void )
{
	centity_t *cent;

	cent = &cg_entities[ cg.snap->ps.clientNum ];

	//fade to black if stamina is low
	if( ( cg.snap->ps.stats[ STAT_STAMINA ] < -800 ) &&
	    ( cg.snap->ps.stats[ STAT_PTEAM ] == PTE_HUMANS ) )
	{
		vec4_t black = { 0, 0, 0, 0 };
		black[ 3 ] = 1.0 - ( ( float )( cg.snap->ps.stats[ STAT_STAMINA ] + 1000 ) / 200.0f );
		trap_R_SetColor( black );
		CG_DrawPic( 0, 0, 640, 480, cgs.media.whiteShader );
		trap_R_SetColor( NULL );
	}
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
	char *s;

	Q_strncpyz( cg.centerPrint, str, sizeof( cg.centerPrint ) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;

	while( *s )
	{
		if( *s == '\n' )
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

	if( !cg.centerPrintTime )
	{
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );

	if( !color )
	{
		return;
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while( 1 )
	{
		char linebuffer[ 1024 ];

		for( l = 0; l < 50; l++ )
		{
			if( !start[ l ] || start[ l ] == '\n' )
			{
				break;
			}

			linebuffer[ l ] = start[ l ];
		}

		linebuffer[ l ] = 0;

		w = CG_Text_Width( linebuffer, 0.5, 0 );
		h = CG_Text_Height( linebuffer, 0.5, 0 );
		x = ( SCREEN_WIDTH - w ) / 2;
		CG_Text_Paint( x, y + h, 0.5, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );
		y += h + 6;

		while( *start && ( *start != '\n' ) )
		{
			start++;
		}

		if( !*start )
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
static void CG_DrawVote( void )
{
	char   *s;
	int    sec;
	vec4_t white = { 1.0f, 1.0f, 1.0f, 1.0f };

	if( !cgs.voteTime )
	{
		return;
	}

	// play a talk beep whenever it is modified
	if( cgs.voteModified )
	{
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;

	if( sec < 0 )
	{
		sec = 0;
	}

	s = va( "VOTE(%i): \"%s\"  Yes:%i No:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo );
	CG_Text_Paint( 8, 340, 0.3f, white, s, 0, 0, ITEM_TEXTSTYLE_NORMAL );
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote( void )
{
	char   *s;
	int    sec, cs_offset;
	vec4_t white = { 1.0f, 1.0f, 1.0f, 1.0f };

	if( cg.predictedPlayerState.stats[ STAT_PTEAM ] == PTE_HUMANS )
	{
		cs_offset = 0;
	}
	else if( cg.predictedPlayerState.stats[ STAT_PTEAM ] == PTE_ALIENS )
	{
		cs_offset = 1;
	}
	else
	{
		return;
	}

	if( !cgs.teamVoteTime[ cs_offset ] )
	{
		return;
	}

	// play a talk beep whenever it is modified
	if( cgs.teamVoteModified[ cs_offset ] )
	{
		cgs.teamVoteModified[ cs_offset ] = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[ cs_offset ] ) ) / 1000;

	if( sec < 0 )
	{
		sec = 0;
	}

	s = va( "TEAMVOTE(%i): \"%s\"  Yes:%i No:%i", sec, cgs.teamVoteString[ cs_offset ],
	        cgs.teamVoteYes[ cs_offset ], cgs.teamVoteNo[ cs_offset ] );

	CG_Text_Paint( 8, 360, 0.3f, white, s, 0, 0, ITEM_TEXTSTYLE_NORMAL );
}

static qboolean CG_DrawScoreboard( void )
{
	static qboolean firstTime = qtrue;
	float           fade, *fadeColor;

	if( menuScoreboard )
	{
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}

	if( cg_paused.integer )
	{
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	if( cg.showScores ||
	    cg.predictedPlayerState.pm_type == PM_INTERMISSION )
	{
		fade = 1.0;
		fadeColor = colorWhite;
	}
	else
	{
		cg.deferredPlayerLoading = 0;
		cg.killerName[ 0 ] = 0;
		firstTime = qtrue;
		return qfalse;
	}

	if( menuScoreboard == NULL )
	{
		menuScoreboard = Menus_FindByName( "teamscore_menu" );
	}

	if( menuScoreboard )
	{
		if( firstTime )
		{
			CG_SetScoreSelection( menuScoreboard );
			firstTime = qfalse;
		}

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
	if( cg_drawStatus.integer )
	{
		Menu_Paint( Menus_FindByName( "default_hud" ), qtrue );
	}

	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

#define FOLLOWING_STRING "following "

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void )
{
	float  w;
	vec4_t color;
	char   buffer[ MAX_STRING_CHARS ];

	if( !( cg.snap->ps.pm_flags & PMF_FOLLOW ) )
	{
		return qfalse;
	}

	color[ 0 ] = 1;
	color[ 1 ] = 1;
	color[ 2 ] = 1;
	color[ 3 ] = 1;

	strcpy( buffer, FOLLOWING_STRING );
	strcat( buffer, cgs.clientinfo[ cg.snap->ps.clientNum ].name );

	w = CG_Text_Width( buffer, 0.7f, 0 );
	CG_Text_Paint( 320 - w / 2, 400, 0.7f, color, buffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED );

	return qtrue;
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
	char   buffer[ MAX_STRING_CHARS ];

	if( !( cg.snap->ps.pm_flags & PMF_QUEUED ) )
	{
		return qfalse;
	}

	color[ 0 ] = 1;
	color[ 1 ] = 1;
	color[ 2 ] = 1;
	color[ 3 ] = 1;

	Com_sprintf( buffer, MAX_STRING_CHARS, "You are in position %d of the spawn queue.",
	             cg.snap->ps.persistant[ PERS_QUEUEPOS ] + 1 );

	w = CG_Text_Width( buffer, 0.7f, 0 );
	CG_Text_Paint( 320 - w / 2, 360, 0.7f, color, buffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED );

	if( cg.snap->ps.stats[ STAT_PTEAM ] == PTE_ALIENS )
	{
		if( cgs.numAlienSpawns == 1 )
		{
			Com_sprintf( buffer, MAX_STRING_CHARS, "There is 1 spawn remaining." );
		}
		else
		{
			Com_sprintf( buffer, MAX_STRING_CHARS, "There are %d spawns remaining.",
			             cgs.numAlienSpawns );
		}
	}
	else if( cg.snap->ps.stats[ STAT_PTEAM ] == PTE_HUMANS )
	{
		if( cgs.numHumanSpawns == 1 )
		{
			Com_sprintf( buffer, MAX_STRING_CHARS, "There is 1 spawn remaining." );
		}
		else
		{
			Com_sprintf( buffer, MAX_STRING_CHARS, "There are %d spawns remaining.",
			             cgs.numHumanSpawns );
		}
	}

	w = CG_Text_Width( buffer, 0.7f, 0 );
	CG_Text_Paint( 320 - w / 2, 400, 0.7f, color, buffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED );

	return qtrue;
}

//==================================================================================

#define SPECTATOR_STRING "SPECTATOR"

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D( void )
{
	vec4_t    color;
	float     w;
	menuDef_t *menu = NULL, *defaultMenu;

	color[ 0 ] = color[ 1 ] = color[ 2 ] = color[ 3 ] = 1.0f;

	if( cg_draw2D.integer == 0 )
	{
		return;
	}

	if( cg.snap->ps.pm_type == PM_INTERMISSION )
	{
		CG_DrawIntermission();
		return;
	}

	//TA: draw the lighting effects e.g. nvg
	CG_DrawLighting();

	defaultMenu = Menus_FindByName( "default_hud" );

	if( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_SPECTATOR )
	{
		w = CG_Text_Width( SPECTATOR_STRING, 0.7f, 0 );
		CG_Text_Paint( 320 - w / 2, 440, 0.7f, color, SPECTATOR_STRING, 0, 0, ITEM_TEXTSTYLE_SHADOWED );
	}
	else
	{
		menu = Menus_FindByName( BG_FindHudNameForClass( cg.predictedPlayerState.stats[ STAT_PCLASS ] ) );
	}

	if( !( cg.snap->ps.stats[ STAT_STATE ] & SS_INFESTING ) &&
	    !( cg.snap->ps.stats[ STAT_STATE ] & SS_HOVELING ) && menu &&
	    ( cg.snap->ps.stats[ STAT_HEALTH ] > 0 ) )
	{
		if( cg_drawStatus.integer )
		{
			Menu_Paint( menu, qtrue );
		}

		CG_DrawCrosshair();
	}
	else if( cg_drawStatus.integer )
	{
		Menu_Paint( defaultMenu, qtrue );
	}

	CG_DrawVote();
	CG_DrawTeamVote();
	CG_DrawFollow();
	CG_DrawQueue();

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();

	if( !cg.scoreBoardShowing )
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

	if( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_SPECTATOR )
	{
		return;
	}

	damage = cg.lastHealth - cg.snap->ps.stats[ STAT_HEALTH ];

	if( damage < 0 )
	{
		damage = 0;
	}

	damageAsFracOfMax = ( float ) damage / cg.snap->ps.stats[ STAT_MAX_HEALTH ];
	cg.lastHealth = cg.snap->ps.stats[ STAT_HEALTH ];

	cg.painBlendValue += damageAsFracOfMax * cg_painBlendScale.value;

	if( cg.painBlendValue > 0.0f )
	{
		cg.painBlendValue -= ( cg.frametime / 1000.0f ) *
		                     cg_painBlendDownRate.value;
	}

	if( cg.painBlendValue > 1.0f )
	{
		cg.painBlendValue = 1.0f;
	}
	else if( cg.painBlendValue <= 0.0f )
	{
		cg.painBlendValue = 0.0f;
		return;
	}

	if( cg.snap->ps.stats[ STAT_PTEAM ] == PTE_ALIENS )
	{
		VectorSet( color, 0.43f, 0.8f, 0.37f );
	}
	else if( cg.snap->ps.stats[ STAT_PTEAM ] == PTE_HUMANS )
	{
		VectorSet( color, 0.8f, 0.0f, 0.0f );
	}

	if( cg.painBlendValue > cg.painBlendTarget )
	{
		cg.painBlendTarget += ( cg.frametime / 1000.0f ) *
		                      cg_painBlendUpRate.value;
	}
	else if( cg.painBlendValue < cg.painBlendTarget )
	{
		cg.painBlendTarget = cg.painBlendValue;
	}

	if( cg.painBlendTarget > cg_painBlendMax.value )
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
	if( !cg.snap )
	{
		return;
	}

	switch( stereoView )
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

	if( separation != 0 )
	{
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[ 1 ],
		          cg.refdef.vieworg );
	}

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// restore original viewpoint if running stereo
	if( separation != 0 )
	{
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}

	// first person blend blobs, done after AnglesToAxis
	if( !cg.renderingThirdPerson )
	{
		CG_PainBlend();
	}

	// draw status bar and other floating elements
	CG_Draw2D();
}
