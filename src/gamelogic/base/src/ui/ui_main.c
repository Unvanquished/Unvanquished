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

/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/

// use this to get a demo build without an explicit demo build, i.e. to get the demo ui files to build
//#define PRE_RELEASE_TADEMO

#include "ui_local.h"

uiInfo_t                    uiInfo;

static const char           *skillLevels[] =
{
	"I Can Win",
	"Bring It On",
	"Hurt Me Plenty",
	"Hardcore",
	"Nightmare"
};

static const int            numSkillLevels = sizeof ( skillLevels ) / sizeof ( const char * );

static const char           *netSources[] =
{
	"Local",
	"Internet",
	"Favorites"
};
static const int            numNetSources = sizeof ( netSources ) / sizeof ( const char * );

static const serverFilter_t serverFilters[] =
{
	{ "All",                   ""            },
	{ "Quake 3 Arena",         ""            },
	{ "Team Arena",            "missionpack" },
	{ "Rocket Arena",          "arena"       },
	{ "Alliance",              "alliance20"  },
	{ "Weapons Factory Arena", "wfa"         },
	{ "OSP",                   "osp"         },
};

static const int            numServerFilters = sizeof ( serverFilters ) / sizeof ( serverFilter_t );

static char                  *netnames[] =
{
	"???",
	"UDP",
	"IPX",
	NULL
};

static int                  gamecodetoui[] = { 4, 2, 3, 0, 5, 1, 6 };

static void                 UI_StartServerRefresh ( qboolean full );
static void                 UI_StopServerRefresh ( void );
static void                 UI_DoServerRefresh ( void );
static void                 UI_FeederSelection ( float feederID, int index );
static void                 UI_BuildServerDisplayList ( qboolean force );
static void                 UI_BuildServerStatus ( qboolean force );
static void                 UI_BuildFindPlayerList ( qboolean force );
static int QDECL            UI_ServersQsortCompare ( const void *arg1, const void *arg2 );
static int                  UI_MapCountByGameType ( qboolean singlePlayer );
static int                  UI_HeadCountByTeam ( void );
static const char           *UI_SelectedMap ( int index, int *actual );
static const char           *UI_SelectedHead ( int index, int *actual );
static int                  UI_GetIndexFromSelection ( int actual );

int                         ProcessNewUI ( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6 );

extern displayContextDef_t  *DC;

static char                 translated_yes[ 4 ], translated_no[ 4 ];

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
vmCvar_t ui_new;
vmCvar_t ui_debug;
vmCvar_t ui_initialized;
vmCvar_t ui_teamArenaFirstRun;

void     _UI_Init ( qboolean );
void     _UI_Shutdown ( void );
void     _UI_KeyEvent ( int key, qboolean down );
void     _UI_MouseEvent ( int dx, int dy );
void     _UI_Refresh ( int realtime );
qboolean _UI_IsFullscreen ( void );

intptr_t vmMain ( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11  )
{
	switch ( command )
	{
		case UI_GETAPIVERSION:
			return UI_API_VERSION;

		case UI_INIT:
			_UI_Init ( arg0 );
			return 0;

		case UI_SHUTDOWN:
			_UI_Shutdown();
			return 0;

		case UI_KEY_EVENT:
			_UI_KeyEvent ( arg0, arg1 );
			return 0;

		case UI_MOUSE_EVENT:
			_UI_MouseEvent ( arg0, arg1 );
			return 0;

		case UI_REFRESH:
			_UI_Refresh ( arg0 );
			return 0;

		case UI_IS_FULLSCREEN:
			return _UI_IsFullscreen();

		case UI_SET_ACTIVE_MENU:
			_UI_SetActiveMenu ( arg0 );
			return 0;

		case UI_CONSOLE_COMMAND:
			return UI_ConsoleCommand ( arg0 );

		case UI_DRAW_CONNECT_SCREEN:
			UI_DrawConnectScreen ( arg0 );
			return 0;
	}

	return -1;
}

void AssetCache ( void )
{
	uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip ( ASSET_GRADIENTBAR );
	uiInfo.uiDC.Assets.scrollBar = trap_R_RegisterShaderNoMip ( ASSET_SCROLLBAR );
	uiInfo.uiDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip ( ASSET_SCROLLBAR_ARROWDOWN );
	uiInfo.uiDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip ( ASSET_SCROLLBAR_ARROWUP );
	uiInfo.uiDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip ( ASSET_SCROLLBAR_ARROWLEFT );
	uiInfo.uiDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip ( ASSET_SCROLLBAR_ARROWRIGHT );
	uiInfo.uiDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip ( ASSET_SCROLL_THUMB );
	uiInfo.uiDC.Assets.sliderBar = trap_R_RegisterShaderNoMip ( ASSET_SLIDER_BAR );
	uiInfo.uiDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip ( ASSET_SLIDER_THUMB );
}

void _UI_DrawSides ( float x, float y, float w, float h, float size )
{
	UI_AdjustFrom640 ( &x, &y, &w, &h );
	size *= uiInfo.uiDC.xscale;
	trap_R_DrawStretchPic ( x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic ( x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void _UI_DrawTopBottom ( float x, float y, float w, float h, float size )
{
	UI_AdjustFrom640 ( &x, &y, &w, &h );
	size *= uiInfo.uiDC.yscale;
	trap_R_DrawStretchPic ( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic ( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void _UI_DrawRect ( float x, float y, float width, float height, float size, const float *color )
{
	trap_R_SetColor ( color );

	_UI_DrawTopBottom ( x, y, width, height, size );
	_UI_DrawSides ( x, y, width, height, size );

	trap_R_SetColor ( NULL );
}

int Text_Width ( const char *text, float scale, int limit )
{
	int         count, len;
	float       out;
	glyphInfo_t *glyph;
	float       useScale;
	const char  *s = text;
	fontInfo_t  *font = &uiInfo.uiDC.Assets.textFont;

	if ( scale <= ui_smallFont.value )
	{
		font = &uiInfo.uiDC.Assets.smallFont;
	}
	else if ( scale >= ui_bigFont.value )
	{
		font = &uiInfo.uiDC.Assets.bigFont;
	}

	useScale = scale * font->glyphScale;
	out = 0;

	if ( text )
	{
		len = strlen ( text );

		if ( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while ( s && *s && count < len )
		{
			if ( Q_IsColorString ( s ) )
			{
				s += 2;
				continue;
			}
			else
			{
				glyph = &font->glyphs[ ( int ) * s ];
				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}

	return out * useScale;
}

int Text_Height ( const char *text, float scale, int limit )
{
	int         len, count;
	float       max;
	glyphInfo_t *glyph;
	float       useScale;
	const char  *s = text; // bk001206 - unsigned
	fontInfo_t  *font = &uiInfo.uiDC.Assets.textFont;

	if ( scale <= ui_smallFont.value )
	{
		font = &uiInfo.uiDC.Assets.smallFont;
	}
	else if ( scale >= ui_bigFont.value )
	{
		font = &uiInfo.uiDC.Assets.bigFont;
	}

	useScale = scale * font->glyphScale;
	max = 0;

	if ( text )
	{
		len = strlen ( text );

		if ( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while ( s && *s && count < len )
		{
			if ( Q_IsColorString ( s ) )
			{
				s += 2;
				continue;
			}
			else
			{
				glyph = &font->glyphs[ ( int ) * s ];

				if ( max < glyph->height )
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

void Text_PaintChar ( float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader )
{
	float w, h;
	w = width * scale;
	h = height * scale;
	UI_AdjustFrom640 ( &x, &y, &w, &h );
	trap_R_DrawStretchPic ( x, y, w, h, s, t, s2, t2, hShader );
}

void Text_Paint ( float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style )
{
	int         len, count;
	vec4_t      newColor;
	glyphInfo_t *glyph;
	float       useScale;
	fontInfo_t  *font = &uiInfo.uiDC.Assets.textFont;

	if ( scale <= ui_smallFont.value )
	{
		font = &uiInfo.uiDC.Assets.smallFont;
	}
	else if ( scale >= ui_bigFont.value )
	{
		font = &uiInfo.uiDC.Assets.bigFont;
	}

	useScale = scale * font->glyphScale;

	if ( text )
	{
		const char *s = text; // bk001206 - unsigned
		trap_R_SetColor ( color );
		memcpy ( &newColor[ 0 ], &color[ 0 ], sizeof ( vec4_t ) );
		len = strlen ( text );

		if ( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while ( s && *s && count < len )
		{
			glyph = &font->glyphs[ ( int ) * s ];

			//int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
			//float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString ( s ) )
			{
				memcpy ( newColor, g_color_table[ ColorIndex ( * ( s + 1 ) ) ], sizeof ( newColor ) );
				newColor[ 3 ] = color[ 3 ];
				trap_R_SetColor ( newColor );
				s += 2;
				continue;
			}
			else
			{
				float yadj = useScale * glyph->top;

				if ( style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE )
				{
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[ 3 ] = newColor[ 3 ];
					trap_R_SetColor ( colorBlack );
					Text_PaintChar ( x + ofs, y - yadj + ofs,
					                 glyph->imageWidth,
					                 glyph->imageHeight,
					                 useScale,
					                 glyph->s,
					                 glyph->t,
					                 glyph->s2,
					                 glyph->t2,
					                 glyph->glyph );
					trap_R_SetColor ( newColor );
					colorBlack[ 3 ] = 1.0;
				}
				else if ( style == ITEM_TEXTSTYLE_NEON )
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

					trap_R_SetColor ( glow );
					Text_PaintChar ( x - 1.5, y - yadj - 1.5,
					                 glyph->imageWidth + 3,
					                 glyph->imageHeight + 3,
					                 useScale,
					                 glyph->s,
					                 glyph->t,
					                 glyph->s2,
					                 glyph->t2,
					                 glyph->glyph );

					trap_R_SetColor ( outer );
					Text_PaintChar ( x - 1, y - yadj - 1,
					                 glyph->imageWidth + 2,
					                 glyph->imageHeight + 2,
					                 useScale,
					                 glyph->s,
					                 glyph->t,
					                 glyph->s2,
					                 glyph->t2,
					                 glyph->glyph );

					trap_R_SetColor ( inner );
					Text_PaintChar ( x - 0.5, y - yadj - 0.5,
					                 glyph->imageWidth + 1,
					                 glyph->imageHeight + 1,
					                 useScale,
					                 glyph->s,
					                 glyph->t,
					                 glyph->s2,
					                 glyph->t2,
					                 glyph->glyph );

					trap_R_SetColor ( white );
				}

				Text_PaintChar ( x, y - yadj,
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

		trap_R_SetColor ( NULL );
	}
}

void Text_PaintWithCursor ( float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style )
{
	int         len, count;
	vec4_t      newColor;
	glyphInfo_t *glyph, *glyph2;
	float       yadj;
	float       useScale;
	fontInfo_t  *font = &uiInfo.uiDC.Assets.textFont;

	if ( scale <= ui_smallFont.value )
	{
		font = &uiInfo.uiDC.Assets.smallFont;
	}
	else if ( scale >= ui_bigFont.value )
	{
		font = &uiInfo.uiDC.Assets.bigFont;
	}

	useScale = scale * font->glyphScale;

	if ( text )
	{
		const char *s = text; // bk001206 - unsigned
		trap_R_SetColor ( color );
		memcpy ( &newColor[ 0 ], &color[ 0 ], sizeof ( vec4_t ) );
		len = strlen ( text );

		if ( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;
		glyph2 = &font->glyphs[ ( int ) cursor ]; // bk001206 - possible signed char

		while ( s && *s && count < len )
		{
			glyph = &font->glyphs[ ( int ) * s ];

			if ( Q_IsColorString ( s ) )
			{
				memcpy ( newColor, g_color_table[ ColorIndex ( * ( s + 1 ) ) ], sizeof ( newColor ) );
				newColor[ 3 ] = color[ 3 ];
				trap_R_SetColor ( newColor );
				s += 2;
				continue;
			}
			else
			{
				yadj = useScale * glyph->top;

				if ( style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE )
				{
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[ 3 ] = newColor[ 3 ];
					trap_R_SetColor ( colorBlack );
					Text_PaintChar ( x + ofs, y - yadj + ofs,
					                 glyph->imageWidth,
					                 glyph->imageHeight,
					                 useScale,
					                 glyph->s,
					                 glyph->t,
					                 glyph->s2,
					                 glyph->t2,
					                 glyph->glyph );
					colorBlack[ 3 ] = 1.0;
					trap_R_SetColor ( newColor );
				}
				else if ( style == ITEM_TEXTSTYLE_NEON )
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

					trap_R_SetColor ( glow );
					Text_PaintChar ( x - 1.5, y - yadj - 1.5,
					                 glyph->imageWidth + 3,
					                 glyph->imageHeight + 3,
					                 useScale,
					                 glyph->s,
					                 glyph->t,
					                 glyph->s2,
					                 glyph->t2,
					                 glyph->glyph );

					trap_R_SetColor ( outer );
					Text_PaintChar ( x - 1, y - yadj - 1,
					                 glyph->imageWidth + 2,
					                 glyph->imageHeight + 2,
					                 useScale,
					                 glyph->s,
					                 glyph->t,
					                 glyph->s2,
					                 glyph->t2,
					                 glyph->glyph );

					trap_R_SetColor ( inner );
					Text_PaintChar ( x - 0.5, y - yadj - 0.5,
					                 glyph->imageWidth + 1,
					                 glyph->imageHeight + 1,
					                 useScale,
					                 glyph->s,
					                 glyph->t,
					                 glyph->s2,
					                 glyph->t2,
					                 glyph->glyph );

					trap_R_SetColor ( white );
				}

				Text_PaintChar ( x, y - yadj,
				                 glyph->imageWidth,
				                 glyph->imageHeight,
				                 useScale,
				                 glyph->s,
				                 glyph->t,
				                 glyph->s2,
				                 glyph->t2,
				                 glyph->glyph );

				// CG_DrawPic(x, y - yadj, scale * uiDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * uiDC.Assets.textFont.glyphs[text[i]].imageHeight, uiDC.Assets.textFont.glyphs[text[i]].glyph);
				yadj = useScale * glyph2->top;

				if ( count == cursorPos && ! ( ( uiInfo.uiDC.realTime / BLINK_DIVISOR ) & 1 ) )
				{
					Text_PaintChar ( x, y - yadj,
					                 glyph2->imageWidth,
					                 glyph2->imageHeight,
					                 useScale,
					                 glyph2->s,
					                 glyph2->t,
					                 glyph2->s2,
					                 glyph2->t2,
					                 glyph2->glyph );
				}

				x += ( glyph->xSkip * useScale );
				s++;
				count++;
			}
		}

		// need to paint cursor at end of text
		if ( cursorPos == len && ! ( ( uiInfo.uiDC.realTime / BLINK_DIVISOR ) & 1 ) )
		{
			yadj = useScale * glyph2->top;
			Text_PaintChar ( x, y - yadj,
			                 glyph2->imageWidth,
			                 glyph2->imageHeight,
			                 useScale,
			                 glyph2->s,
			                 glyph2->t,
			                 glyph2->s2,
			                 glyph2->t2,
			                 glyph2->glyph );
		}

		trap_R_SetColor ( NULL );
	}
}

static void Text_Paint_Limit ( float *maxX, float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit )
{
	int         len, count;
	vec4_t      newColor;
	glyphInfo_t *glyph;

	if ( text )
	{
		const char *s = text; // bk001206 - unsigned
		float      max = *maxX;
		float      useScale;
		fontInfo_t *font = &uiInfo.uiDC.Assets.textFont;

		if ( scale <= ui_smallFont.value )
		{
			font = &uiInfo.uiDC.Assets.smallFont;
		}
		else if ( scale > ui_bigFont.value )
		{
			font = &uiInfo.uiDC.Assets.bigFont;
		}

		useScale = scale * font->glyphScale;
		trap_R_SetColor ( color );
		len = strlen ( text );

		if ( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while ( s && *s && count < len )
		{
			glyph = &font->glyphs[ ( int ) * s ];

			if ( Q_IsColorString ( s ) )
			{
				memcpy ( newColor, g_color_table[ ColorIndex ( * ( s + 1 ) ) ], sizeof ( newColor ) );
				newColor[ 3 ] = color[ 3 ];
				trap_R_SetColor ( newColor );
				s += 2;
				continue;
			}
			else
			{
				float yadj = useScale * glyph->top;

				if ( Text_Width ( s, useScale, 1 ) + x > max )
				{
					*maxX = 0;
					break;
				}

				Text_PaintChar ( x, y - yadj,
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

		trap_R_SetColor ( NULL );
	}
}

void UI_ShowPostGame ( qboolean newHigh )
{
	trap_Cvar_Set ( "cg_cameraOrbit", "0" );
	trap_Cvar_Set ( "cg_thirdPerson", "0" );
	trap_Cvar_Set ( "sv_killserver", "1" );
	uiInfo.soundHighScore = newHigh;
	_UI_SetActiveMenu ( UIMENU_POSTGAME );
}

/*
=================
_UI_Refresh
=================
*/

void UI_DrawCenteredPic ( qhandle_t image, int w, int h )
{
	int x, y;
	x = ( SCREEN_WIDTH - w ) / 2;
	y = ( SCREEN_HEIGHT - h ) / 2;
	UI_DrawHandlePic ( x, y, w, h, image );
}

int frameCount = 0;
int startTime;

#define UI_FPS_FRAMES 4
void _UI_Refresh ( int realtime )
{
	static int index;
	static int previousTimes[ UI_FPS_FRAMES ];

	//if ( !( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
	//  return;
	//}

	uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realtime;

	previousTimes[ index % UI_FPS_FRAMES ] = uiInfo.uiDC.frameTime;
	index++;

	if ( index > UI_FPS_FRAMES )
	{
		int i, total;
		// average multiple frames together to smooth changes out a bit
		total = 0;

		for ( i = 0; i < UI_FPS_FRAMES; i++ )
		{
			total += previousTimes[ i ];
		}

		if ( !total )
		{
			total = 1;
		}

		uiInfo.uiDC.FPS = 1000 * UI_FPS_FRAMES / total;
	}

	UI_UpdateCvars();

	if ( Menu_Count() > 0 )
	{
		// paint all the menus
		Menu_PaintAll();
		// refresh server browser list
		UI_DoServerRefresh();
		// refresh server status
		UI_BuildServerStatus ( qfalse );
		// refresh find player list
		UI_BuildFindPlayerList ( qfalse );
	}

	// draw cursor
	UI_SetColor ( NULL );

	//TA: don't draw the cursor whilst loading
	if ( Menu_Count() > 0 && !trap_Cvar_VariableValue ( "ui_loading" ) )
	{
		UI_DrawHandlePic ( uiInfo.uiDC.cursorx - 16, uiInfo.uiDC.cursory - 16, 32, 32, uiInfo.uiDC.Assets.cursor );
	}

#ifndef NDEBUG

	if ( uiInfo.uiDC.debug )
	{
		// cursor coordinates
		//FIXME
		//UI_DrawString( 0, 0, va("(%d,%d)",uis.cursorx,uis.cursory), UI_LEFT|UI_SMALLFONT, colorRed );
	}

#endif
}

/*
=================
_UI_Shutdown
=================
*/
void _UI_Shutdown ( void )
{
	trap_LAN_SaveCachedServers();
}

char *defaultMenu = NULL;

char *GetMenuBuffer ( const char *filename )
{
	int          len;
	fileHandle_t f;
	static char  buf[ MAX_MENUFILE ];

	len = trap_FS_FOpenFile ( filename, &f, FS_READ );

	if ( !f )
	{
		trap_Print ( va ( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return defaultMenu;
	}

	if ( len >= MAX_MENUFILE )
	{
		trap_Print ( va ( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", filename, len, MAX_MENUFILE ) );
		trap_FS_FCloseFile ( f );
		return defaultMenu;
	}

	trap_FS_Read ( buf, len, f );
	buf[ len ] = 0;
	trap_FS_FCloseFile ( f );
	//COM_Compress(buf);
	return buf;
}

qboolean Asset_Parse ( int handle )
{
	pc_token_t token;
	const char *tempStr;

	if ( !trap_PC_ReadToken ( handle, &token ) )
	{
		return qfalse;
	}

	if ( Q_stricmp ( token.string, "{" ) != 0 )
	{
		return qfalse;
	}

	while ( 1 )
	{
		memset ( &token, 0, sizeof ( pc_token_t ) );

		if ( !trap_PC_ReadToken ( handle, &token ) )
		{
			return qfalse;
		}

		if ( Q_stricmp ( token.string, "}" ) == 0 )
		{
			return qtrue;
		}

		// font
		if ( Q_stricmp ( token.string, "font" ) == 0 )
		{
			int pointSize;

			if ( !PC_String_Parse ( handle, &tempStr ) || !PC_Int_Parse ( handle, &pointSize ) )
			{
				return qfalse;
			}

			trap_R_RegisterFont ( tempStr, pointSize, &uiInfo.uiDC.Assets.textFont );
			uiInfo.uiDC.Assets.fontRegistered = qtrue;
			continue;
		}

		if ( Q_stricmp ( token.string, "smallFont" ) == 0 )
		{
			int pointSize;

			if ( !PC_String_Parse ( handle, &tempStr ) || !PC_Int_Parse ( handle, &pointSize ) )
			{
				return qfalse;
			}

			trap_R_RegisterFont ( tempStr, pointSize, &uiInfo.uiDC.Assets.smallFont );
			continue;
		}

		if ( Q_stricmp ( token.string, "bigFont" ) == 0 )
		{
			int pointSize;

			if ( !PC_String_Parse ( handle, &tempStr ) || !PC_Int_Parse ( handle, &pointSize ) )
			{
				return qfalse;
			}

			trap_R_RegisterFont ( tempStr, pointSize, &uiInfo.uiDC.Assets.bigFont );
			continue;
		}

		// gradientbar
		if ( Q_stricmp ( token.string, "gradientbar" ) == 0 )
		{
			if ( !PC_String_Parse ( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip ( tempStr );
			continue;
		}

		// enterMenuSound
		if ( Q_stricmp ( token.string, "menuEnterSound" ) == 0 )
		{
			if ( !PC_String_Parse ( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.menuEnterSound = trap_S_RegisterSound ( tempStr, qfalse );
			continue;
		}

		// exitMenuSound
		if ( Q_stricmp ( token.string, "menuExitSound" ) == 0 )
		{
			if ( !PC_String_Parse ( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.menuExitSound = trap_S_RegisterSound ( tempStr, qfalse );
			continue;
		}

		// itemFocusSound
		if ( Q_stricmp ( token.string, "itemFocusSound" ) == 0 )
		{
			if ( !PC_String_Parse ( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.itemFocusSound = trap_S_RegisterSound ( tempStr, qfalse );
			continue;
		}

		// menuBuzzSound
		if ( Q_stricmp ( token.string, "menuBuzzSound" ) == 0 )
		{
			if ( !PC_String_Parse ( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.menuBuzzSound = trap_S_RegisterSound ( tempStr, qfalse );
			continue;
		}

		if ( Q_stricmp ( token.string, "cursor" ) == 0 )
		{
			if ( !PC_String_Parse ( handle, &uiInfo.uiDC.Assets.cursorStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.cursor = trap_R_RegisterShaderNoMip ( uiInfo.uiDC.Assets.cursorStr );
			continue;
		}

		if ( Q_stricmp ( token.string, "fadeClamp" ) == 0 )
		{
			if ( !PC_Float_Parse ( handle, &uiInfo.uiDC.Assets.fadeClamp ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp ( token.string, "fadeCycle" ) == 0 )
		{
			if ( !PC_Int_Parse ( handle, &uiInfo.uiDC.Assets.fadeCycle ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp ( token.string, "fadeAmount" ) == 0 )
		{
			if ( !PC_Float_Parse ( handle, &uiInfo.uiDC.Assets.fadeAmount ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp ( token.string, "shadowX" ) == 0 )
		{
			if ( !PC_Float_Parse ( handle, &uiInfo.uiDC.Assets.shadowX ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp ( token.string, "shadowY" ) == 0 )
		{
			if ( !PC_Float_Parse ( handle, &uiInfo.uiDC.Assets.shadowY ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp ( token.string, "shadowColor" ) == 0 )
		{
			if ( !PC_Color_Parse ( handle, &uiInfo.uiDC.Assets.shadowColor ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.shadowFadeClamp = uiInfo.uiDC.Assets.shadowColor[ 3 ];
			continue;
		}
	}

	return qfalse;
}

void Font_Report ( void )
{
	int i;
	Com_Printf ( "Font Info\n" );
	Com_Printf ( "=========\n" );

	for ( i = 32; i < 96; i++ )
	{
		Com_Printf ( "Glyph handle %i: %i\n", i, uiInfo.uiDC.Assets.textFont.glyphs[ i ].glyph );
	}
}

void UI_Report ( void )
{
	String_Report();
	//Font_Report();
}

qboolean UI_ParseMenu ( const char *menuFile )
{
	int        handle;
	pc_token_t token;

	/*Com_Printf("Parsing menu file:%s\n", menuFile);*/

	handle = trap_PC_LoadSource ( menuFile );

	if ( !handle )
	{
		return qfalse;
	}

	while ( 1 )
	{
		memset ( &token, 0, sizeof ( pc_token_t ) );

		if ( !trap_PC_ReadToken ( handle, &token ) )
		{
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//  Com_Printf( "Missing { in menu file\n" );
		//  break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//  Com_Printf( "Too many menus!\n" );
		//  break;
		//}

		if ( token.string[ 0 ] == '}' )
		{
			break;
		}

		if ( Q_stricmp ( token.string, "assetGlobalDef" ) == 0 )
		{
			if ( Asset_Parse ( handle ) )
			{
				continue;
			}
			else
			{
				break;
			}
		}

		if ( Q_stricmp ( token.string, "menudef" ) == 0 )
		{
			// start a new menu
			Menu_New ( handle );
		}
	}

	trap_PC_FreeSource ( handle );
	return qtrue;
}

/*
===============
UI_FindInfoPaneByName
===============
*/
tremInfoPane_t *UI_FindInfoPaneByName ( const char *name )
{
	int i;

	for ( i = 0; i < uiInfo.tremInfoPaneCount; i++ )
	{
		if ( !Q_stricmp ( uiInfo.tremInfoPanes[ i ].name, name ) )
		{
			return &uiInfo.tremInfoPanes[ i ];
		}
	}

	//create a dummy infopane demanding the user write the infopane
	uiInfo.tremInfoPanes[ i ].name = String_Alloc ( name );
	strncpy ( uiInfo.tremInfoPanes[ i ].text, "Not implemented.\n\nui/infopanes.def\n", MAX_INFOPANE_TEXT );
	Q_strcat ( uiInfo.tremInfoPanes[ i ].text, MAX_INFOPANE_TEXT, String_Alloc ( name ) );

	uiInfo.tremInfoPaneCount++;

	return &uiInfo.tremInfoPanes[ i ];
}

/*
===============
UI_LoadInfoPane
===============
*/
qboolean UI_LoadInfoPane ( int handle )
{
	pc_token_t token;
	qboolean   valid = qfalse;

	while ( 1 )
	{
		memset ( &token, 0, sizeof ( pc_token_t ) );

		if ( !trap_PC_ReadToken ( handle, &token ) )
		{
			break;
		}

		if ( !Q_stricmp ( token.string, "name" ) )
		{
			memset ( &token, 0, sizeof ( pc_token_t ) );

			if ( !trap_PC_ReadToken ( handle, &token ) )
			{
				break;
			}

			uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].name = String_Alloc ( token.string );
			valid = qtrue;
		}
		else if ( !Q_stricmp ( token.string, "graphic" ) )
		{
			int *graphic;

			memset ( &token, 0, sizeof ( pc_token_t ) );

			if ( !trap_PC_ReadToken ( handle, &token ) )
			{
				break;
			}

			graphic = &uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].numGraphics;

			if ( !Q_stricmp ( token.string, "top" ) )
			{
				uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].graphics[ *graphic ].side = INFOPANE_TOP;
			}
			else if ( !Q_stricmp ( token.string, "bottom" ) )
			{
				uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].graphics[ *graphic ].side = INFOPANE_BOTTOM;
			}
			else if ( !Q_stricmp ( token.string, "left" ) )
			{
				uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].graphics[ *graphic ].side = INFOPANE_LEFT;
			}
			else if ( !Q_stricmp ( token.string, "right" ) )
			{
				uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].graphics[ *graphic ].side = INFOPANE_RIGHT;
			}
			else
			{
				break;
			}

			memset ( &token, 0, sizeof ( pc_token_t ) );

			if ( !trap_PC_ReadToken ( handle, &token ) )
			{
				break;
			}

			if ( !Q_stricmp ( token.string, "center" ) )
			{
				uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].graphics[ *graphic ].offset = -1;
			}
			else
			{
				uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].graphics[ *graphic ].offset = token.intvalue;
			}

			memset ( &token, 0, sizeof ( pc_token_t ) );

			if ( !trap_PC_ReadToken ( handle, &token ) )
			{
				break;
			}

			uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].graphics[ *graphic ].graphic =
			  trap_R_RegisterShaderNoMip ( token.string );

			memset ( &token, 0, sizeof ( pc_token_t ) );

			if ( !trap_PC_ReadToken ( handle, &token ) )
			{
				break;
			}

			uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].graphics[ *graphic ].width = token.intvalue;

			memset ( &token, 0, sizeof ( pc_token_t ) );

			if ( !trap_PC_ReadToken ( handle, &token ) )
			{
				break;
			}

			uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].graphics[ *graphic ].height = token.intvalue;

			//increment graphics
			( *graphic ) ++;

			if ( *graphic == MAX_INFOPANE_GRAPHICS )
			{
				break;
			}
		}
		else if ( !Q_stricmp ( token.string, "text" ) )
		{
			memset ( &token, 0, sizeof ( pc_token_t ) );

			if ( !trap_PC_ReadToken ( handle, &token ) )
			{
				break;
			}

			Q_strcat ( uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].text, MAX_INFOPANE_TEXT, token.string );
		}
		else if ( !Q_stricmp ( token.string, "align" ) )
		{
			memset ( &token, 0, sizeof ( pc_token_t ) );

			if ( !trap_PC_ReadToken ( handle, &token ) )
			{
				break;
			}

			if ( !Q_stricmp ( token.string, "left" ) )
			{
				uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].align = ITEM_ALIGN_LEFT;
			}
			else if ( !Q_stricmp ( token.string, "right" ) )
			{
				uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].align = ITEM_ALIGN_RIGHT;
			}
			else if ( !Q_stricmp ( token.string, "center" ) )
			{
				uiInfo.tremInfoPanes[ uiInfo.tremInfoPaneCount ].align = ITEM_ALIGN_CENTER;
			}
		}
		else if ( token.string[ 0 ] == '}' )
		{
			//reached the end, break
			break;
		}
		else
		{
			break;
		}
	}

	if ( valid )
	{
		uiInfo.tremInfoPaneCount++;
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*
===============
UI_LoadInfoPanes
===============
*/
void UI_LoadInfoPanes ( const char *file )
{
	pc_token_t token;
	int        handle;
	int        count;

	uiInfo.tremInfoPaneCount = count = 0;

	handle = trap_PC_LoadSource ( file );

	if ( !handle )
	{
		trap_Error ( va ( S_COLOR_YELLOW "infopane file not found: %s\n", file ) );
		return;
	}

	while ( 1 )
	{
		if ( !trap_PC_ReadToken ( handle, &token ) )
		{
			break;
		}

		if ( token.string[ 0 ] == 0 )
		{
			break;
		}

		if ( token.string[ 0 ] == '{' )
		{
			if ( UI_LoadInfoPane ( handle ) )
			{
				count++;
			}

			if ( count == MAX_INFOPANES )
			{
				break;
			}
		}
	}

	trap_PC_FreeSource ( handle );
}

qboolean Load_Menu ( int handle )
{
	pc_token_t token;
	// Dushan - engine support localization support
	//        - it *should* be initialized with cvar
#ifdef LOCALIZATION_SUPPORT
	int cl_lang;
#endif

	if ( !trap_PC_ReadToken ( handle, &token ) )
	{
		return qfalse;
	}

	if ( token.string[ 0 ] != '{' )
	{
		return qfalse;
	}

	while ( 1 )
	{
		if ( !trap_PC_ReadToken ( handle, &token ) )
		{
			return qfalse;
		}

		if ( token.string[ 0 ] == 0 )
		{
			return qfalse;
		}

		if ( token.string[ 0 ] == '}' )
		{
			return qtrue;
		}

#ifdef LOCALIZATION_SUPPORT
		// Dushan - check cl_language cvar
		cl_lang = atoi ( UI_Cvar_VariableString ( "cl_language" ) );

		if ( cl_lang )
		{
			const char *s = NULL;
			const char *fname;
			char       out[ 256 ];

			COM_StripFilename ( token.string, out );

			fname = COM_SkipPath ( token.string );

			// NOTE : cl_language 0 - English

			// Dushan - if cl_language is 1
			//        - load French
			if ( cl_lang == 1 )
			{
				s = va ( "%s%s", out, "french/" );
			}
			// Dushan - if cl_language is 2
			//        - load German
			else if ( cl_lang == 2 )
			{
				s = va ( "%s%s", out, "german/" );
			}
			// Dushan - if cl_language is 1
			//        - load Italian
			else if ( cl_lang == 3 )
			{
				s = va ( "%s%s", out, "italian/" );
			}
			// Dushan - if cl_language is 1
			//        - load Spanish
			else if ( cl_lang == 4 )
			{
				s = va ( "%s%s", out, "spanish/" );
			}

			if ( UI_ParseMenu ( va ( "%s%s", s, fname ) ) )
			{
				continue;
			}
		}

#endif // LOCALIZATION_SUPPORT

		UI_ParseMenu ( token.string );
	}

	return qfalse;
}

void UI_LoadMenus ( const char *menuFile, qboolean reset )
{
	pc_token_t token;
	int        handle;
	int        start;

	start = trap_Milliseconds();

	handle = trap_PC_LoadSource ( menuFile );

	if ( !handle )
	{
		Com_Printf ( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile );
		handle = trap_PC_LoadSource ( "ui/menus.txt" );

		if ( !handle )
		{
			trap_Error ( S_COLOR_RED "default menu file not found: ui/menus.txt, unable to continue!\n" );
		}
	}

	ui_new.integer = 1;

	if ( reset )
	{
		Menu_Reset();
	}

	while ( 1 )
	{
		if ( !trap_PC_ReadToken ( handle, &token ) )
		{
			break;
		}

		if ( token.string[ 0 ] == 0 || token.string[ 0 ] == '}' )
		{
			break;
		}

		if ( token.string[ 0 ] == '}' )
		{
			break;
		}

		if ( Q_stricmp ( token.string, "loadmenu" ) == 0 )
		{
			if ( Load_Menu ( handle ) )
			{
				continue;
			}
			else
			{
				break;
			}
		}
	}

	Com_Printf ( "UI menu load time = %d milli seconds\n", trap_Milliseconds() - start );

	trap_PC_FreeSource ( handle );
}

void UI_Load ( void )
{
	char      lastName[ 1024 ];
	menuDef_t *menu = Menu_GetFocused();
	char      *menuSet = UI_Cvar_VariableString ( "ui_menuFiles" );

	if ( menu && menu->window.name )
	{
		strcpy ( lastName, menu->window.name );
	}

	if ( menuSet == NULL || menuSet[ 0 ] == '\0' )
	{
		menuSet = "ui/menus.txt";
	}

	String_Init();

	/*  UI_ParseGameInfo("gameinfo.txt");
	  UI_LoadArenas();*/

	UI_LoadMenus ( menuSet, qtrue );
	Menus_CloseAll();
	Menus_ActivateByName ( lastName );
}

static const char *handicapValues[] = { "None", "95", "90", "85", "80", "75", "70", "65", "60", "55", "50", "45", "40", "35", "30", "25", "20", "15", "10", "5", NULL };

static void UI_DrawHandicap ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	int i, h;

	h = Com_Clamp ( 5, 100, trap_Cvar_VariableValue ( "handicap" ) );
	i = 20 - h / 5;

	Text_Paint ( rect->x, rect->y, scale, color, handicapValues[ i ], 0, 0, textStyle );
}

static void UI_DrawClanName ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	Text_Paint ( rect->x, rect->y, scale, color, UI_Cvar_VariableString ( "ui_teamName" ), 0, 0, textStyle );
}

static void UI_SetCapFragLimits ( qboolean uiVars )
{
	int cap = 5;
	int frag = 10;

	if ( uiVars )
	{
		trap_Cvar_Set ( "ui_captureLimit", va ( "%d", cap ) );
		trap_Cvar_Set ( "ui_fragLimit", va ( "%d", frag ) );
	}
	else
	{
		trap_Cvar_Set ( "capturelimit", va ( "%d", cap ) );
		trap_Cvar_Set ( "fraglimit", va ( "%d", frag ) );
	}
}

// ui_gameType assumes gametype 0 is -1 ALL and will not show
static void UI_DrawGameType ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	Text_Paint ( rect->x, rect->y, scale, color, uiInfo.gameTypes[ ui_gameType.integer ].gameType, 0, 0, textStyle );
}

static void UI_DrawNetGameType ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	if ( ui_netGameType.integer < 0 || ui_netGameType.integer > uiInfo.numGameTypes )
	{
		trap_Cvar_Set ( "ui_netGameType", "0" );
		trap_Cvar_Set ( "ui_actualNetGameType", "0" );
	}

	Text_Paint ( rect->x, rect->y, scale, color, uiInfo.gameTypes[ ui_netGameType.integer ].gameType, 0, 0, textStyle );
}

static void UI_DrawJoinGameType ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	if ( ui_joinGameType.integer < 0 || ui_joinGameType.integer > uiInfo.numJoinGameTypes )
	{
		trap_Cvar_Set ( "ui_joinGameType", "0" );
	}

	Text_Paint ( rect->x, rect->y, scale, color, uiInfo.joinGameTypes[ ui_joinGameType.integer ].gameType, 0, 0, textStyle );
}

static int UI_TeamIndexFromName ( const char *name )
{
	int i;

	if ( name && *name )
	{
		for ( i = 0; i < uiInfo.teamCount; i++ )
		{
			if ( Q_stricmp ( name, uiInfo.teamList[ i ].teamName ) == 0 )
			{
				return i;
			}
		}
	}

	return 0;
}

static void UI_DrawClanLogo ( rectDef_t *rect, float scale, vec4_t color )
{
	int i;
	i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_teamName" ) );

	if ( i >= 0 && i < uiInfo.teamCount )
	{
		trap_R_SetColor ( color );

		if ( uiInfo.teamList[ i ].teamIcon == -1 )
		{
			uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip ( uiInfo.teamList[ i ].imageName );
			uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip ( va ( "%s_metal", uiInfo.teamList[ i ].imageName ) );
			uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip ( va ( "%s_name", uiInfo.teamList[ i ].imageName ) );
		}

		UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon );
		trap_R_SetColor ( NULL );
	}
}

static void UI_DrawClanCinematic ( rectDef_t *rect, float scale, vec4_t color )
{
	int i;
	i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_teamName" ) );

	if ( i >= 0 && i < uiInfo.teamCount )
	{
		if ( uiInfo.teamList[ i ].cinematic >= -2 )
		{
			if ( uiInfo.teamList[ i ].cinematic == -1 )
			{
				uiInfo.teamList[ i ].cinematic = trap_CIN_PlayCinematic ( va ( "%s.roq", uiInfo.teamList[ i ].imageName ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
			}

			if ( uiInfo.teamList[ i ].cinematic >= 0 )
			{
				trap_CIN_RunCinematic ( uiInfo.teamList[ i ].cinematic );
				trap_CIN_SetExtents ( uiInfo.teamList[ i ].cinematic, rect->x, rect->y, rect->w, rect->h );
				trap_CIN_DrawCinematic ( uiInfo.teamList[ i ].cinematic );
			}
			else
			{
				trap_R_SetColor ( color );
				UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon_Metal );
				trap_R_SetColor ( NULL );
				uiInfo.teamList[ i ].cinematic = -2;
			}
		}
		else
		{
			trap_R_SetColor ( color );
			UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon );
			trap_R_SetColor ( NULL );
		}
	}
}

static void UI_DrawPreviewCinematic ( rectDef_t *rect, float scale, vec4_t color )
{
	if ( uiInfo.previewMovie > -2 )
	{
		uiInfo.previewMovie = trap_CIN_PlayCinematic ( va ( "%s.roq", uiInfo.movieList[ uiInfo.movieIndex ] ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );

		if ( uiInfo.previewMovie >= 0 )
		{
			trap_CIN_RunCinematic ( uiInfo.previewMovie );
			trap_CIN_SetExtents ( uiInfo.previewMovie, rect->x, rect->y, rect->w, rect->h );
			trap_CIN_DrawCinematic ( uiInfo.previewMovie );
		}
		else
		{
			uiInfo.previewMovie = -2;
		}
	}
}

#define GRAPHIC_BWIDTH 8.0f

/*
===============
UI_DrawInfoPane
===============
*/
static void UI_DrawInfoPane ( tremInfoPane_t *pane, rectDef_t *rect, float text_x, float text_y,
                              float scale, vec4_t color, int textStyle )
{
	int       i;
	float     maxLeft = 0, maxTop = 0;
	float     maxRight = 0, maxBottom = 0;
	float     x = rect->x - text_x, y = rect->y - text_y, w, h;
	float     xoffset = 0, yoffset = 0;
	menuDef_t dummyParent;
	itemDef_t textItem;

	//iterate through graphics
	for ( i = 0; i < pane->numGraphics; i++ )
	{
		float     width = pane->graphics[ i ].width;
		float     height = pane->graphics[ i ].height;
		qhandle_t graphic = pane->graphics[ i ].graphic;

		if ( pane->graphics[ i ].side == INFOPANE_TOP || pane->graphics[ i ].side == INFOPANE_BOTTOM )
		{
			//set horizontal offset of graphic
			if ( pane->graphics[ i ].offset < 0 )
			{
				xoffset = ( rect->w / 2 ) - ( pane->graphics[ i ].width / 2 );
			}
			else
			{
				xoffset = pane->graphics[ i ].offset + GRAPHIC_BWIDTH;
			}
		}
		else if ( pane->graphics[ i ].side == INFOPANE_LEFT || pane->graphics[ i ].side == INFOPANE_RIGHT )
		{
			//set vertical offset of graphic
			if ( pane->graphics[ i ].offset < 0 )
			{
				yoffset = ( rect->h / 2 ) - ( pane->graphics[ i ].height / 2 );
			}
			else
			{
				yoffset = pane->graphics[ i ].offset + GRAPHIC_BWIDTH;
			}
		}

		if ( pane->graphics[ i ].side == INFOPANE_LEFT )
		{
			//set the horizontal offset of the text
			if ( pane->graphics[ i ].width > maxLeft )
			{
				maxLeft = pane->graphics[ i ].width + GRAPHIC_BWIDTH;
			}

			xoffset = GRAPHIC_BWIDTH;
		}
		else if ( pane->graphics[ i ].side == INFOPANE_RIGHT )
		{
			if ( pane->graphics[ i ].width > maxRight )
			{
				maxRight = pane->graphics[ i ].width + GRAPHIC_BWIDTH;
			}

			xoffset = rect->w - width - GRAPHIC_BWIDTH;
		}
		else if ( pane->graphics[ i ].side == INFOPANE_TOP )
		{
			//set the vertical offset of the text
			if ( pane->graphics[ i ].height > maxTop )
			{
				maxTop = pane->graphics[ i ].height + GRAPHIC_BWIDTH;
			}

			yoffset = GRAPHIC_BWIDTH;
		}
		else if ( pane->graphics[ i ].side == INFOPANE_BOTTOM )
		{
			if ( pane->graphics[ i ].height > maxBottom )
			{
				maxBottom = pane->graphics[ i ].height + GRAPHIC_BWIDTH;
			}

			yoffset = rect->h - height - GRAPHIC_BWIDTH;
		}

		//draw the graphic
		UI_DrawHandlePic ( x + xoffset, y + yoffset, width, height, graphic );
	}

	//offset the text
	x = rect->x + maxLeft;
	y = rect->y + maxTop;
	w = rect->w - ( maxLeft + maxRight + 16 + ( 2 * text_x ) ); //16 to ensure text within frame
	h = rect->h - ( maxTop + maxBottom );

	textItem.text = pane->text;

	textItem.parent = &dummyParent;
	memcpy ( textItem.window.foreColor, color, sizeof ( vec4_t ) );
	textItem.window.flags = 0;

	switch ( pane->align )
	{
		case ITEM_ALIGN_LEFT:
			textItem.window.rect.x = x;
			break;

		case ITEM_ALIGN_RIGHT:
			textItem.window.rect.x = x + w;
			break;

		case ITEM_ALIGN_CENTER:
			textItem.window.rect.x = x + ( w / 2 );
			break;

		default:
			textItem.window.rect.x = x;
			break;
	}

	textItem.window.rect.y = y;
	textItem.window.rect.w = w;
	textItem.window.rect.h = h;
	textItem.window.borderSize = 0;
	textItem.textRect.x = 0;
	textItem.textRect.y = 0;
	textItem.textRect.w = 0;
	textItem.textRect.h = 0;
	textItem.textalignment = pane->align;
	textItem.textalignx = text_x;
	textItem.textaligny = text_y;
	textItem.textscale = scale;
	textItem.textStyle = textStyle;

	textItem.enableCvar = NULL;
	textItem.cvarTest = NULL;

	//hack to utilise existing autowrap code
	Item_Text_AutoWrapped_Paint ( &textItem );
}

static void UI_DrawSkill ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	int i;
	i = trap_Cvar_VariableValue ( "g_spSkill" );

	if ( i < 1 || i > numSkillLevels )
	{
		i = 1;
	}

	Text_Paint ( rect->x, rect->y, scale, color, skillLevels[ i - 1 ], 0, 0, textStyle );
}

static void UI_DrawTeamName ( rectDef_t *rect, float scale, vec4_t color, qboolean blue, int textStyle )
{
	int i;
	i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( ( blue ) ? "ui_blueTeam" : "ui_redTeam" ) );

	if ( i >= 0 && i < uiInfo.teamCount )
	{
		Text_Paint ( rect->x, rect->y, scale, color, va ( "%s: %s", ( blue ) ? "Blue" : "Red", uiInfo.teamList[ i ].teamName ), 0, 0, textStyle );
	}
}

static void UI_DrawTeamMember ( rectDef_t *rect, float scale, vec4_t color, qboolean blue, int num, int textStyle )
{
	// 0 - None
	// 1 - Human
	// 2..NumCharacters - Bot
	int        value = trap_Cvar_VariableValue ( va ( blue ? "ui_blueteam%i" : "ui_redteam%i", num ) );
	const char *text;

	if ( value <= 0 )
	{
		text = "Closed";
	}
	else if ( value == 1 )
	{
		text = "Human";
	}
	else
	{
		value -= 2;

		if ( value >= UI_GetNumBots() )
		{
			value = 0;
		}

		text = UI_GetBotNameByNumber ( value );
	}

	Text_Paint ( rect->x, rect->y, scale, color, text, 0, 0, textStyle );
}

static void UI_DrawMapPreview ( rectDef_t *rect, float scale, vec4_t color, qboolean net )
{
	int map = ( net ) ? ui_currentNetMap.integer : ui_currentMap.integer;

	if ( map < 0 || map > uiInfo.mapCount )
	{
		if ( net )
		{
			ui_currentNetMap.integer = 0;
			trap_Cvar_Set ( "ui_currentNetMap", "0" );
		}
		else
		{
			ui_currentMap.integer = 0;
			trap_Cvar_Set ( "ui_currentMap", "0" );
		}

		map = 0;
	}

	if ( uiInfo.mapList[ map ].levelShot == -1 )
	{
		uiInfo.mapList[ map ].levelShot = trap_R_RegisterShaderNoMip ( uiInfo.mapList[ map ].imageName );
	}

	if ( uiInfo.mapList[ map ].levelShot > 0 )
	{
		UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, uiInfo.mapList[ map ].levelShot );
	}
	else
	{
		UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip ( "gfx/2d/load_screen" ) );
	}
}

static void UI_DrawMapTimeToBeat ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	int minutes, seconds, time;

	if ( ui_currentMap.integer < 0 || ui_currentMap.integer > uiInfo.mapCount )
	{
		ui_currentMap.integer = 0;
		trap_Cvar_Set ( "ui_currentMap", "0" );
	}

	time = uiInfo.mapList[ ui_currentMap.integer ].timeToBeat[ uiInfo.gameTypes[ ui_gameType.integer ].gtEnum ];

	minutes = time / 60;
	seconds = time % 60;

	Text_Paint ( rect->x, rect->y, scale, color, va ( "%02i:%02i", minutes, seconds ), 0, 0, textStyle );
}

static void UI_DrawMapCinematic ( rectDef_t *rect, float scale, vec4_t color, qboolean net )
{
	int map = ( net ) ? ui_currentNetMap.integer : ui_currentMap.integer;

	if ( map < 0 || map > uiInfo.mapCount )
	{
		if ( net )
		{
			ui_currentNetMap.integer = 0;
			trap_Cvar_Set ( "ui_currentNetMap", "0" );
		}
		else
		{
			ui_currentMap.integer = 0;
			trap_Cvar_Set ( "ui_currentMap", "0" );
		}

		map = 0;
	}

	if ( uiInfo.mapList[ map ].cinematic >= -1 )
	{
		if ( uiInfo.mapList[ map ].cinematic == -1 )
		{
			uiInfo.mapList[ map ].cinematic = trap_CIN_PlayCinematic ( va ( "%s.roq", uiInfo.mapList[ map ].mapLoadName ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
		}

		if ( uiInfo.mapList[ map ].cinematic >= 0 )
		{
			trap_CIN_RunCinematic ( uiInfo.mapList[ map ].cinematic );
			trap_CIN_SetExtents ( uiInfo.mapList[ map ].cinematic, rect->x, rect->y, rect->w, rect->h );
			trap_CIN_DrawCinematic ( uiInfo.mapList[ map ].cinematic );
		}
		else
		{
			uiInfo.mapList[ map ].cinematic = -2;
		}
	}
	else
	{
		UI_DrawMapPreview ( rect, scale, color, net );
	}
}

static qboolean updateModel = qtrue;
static qboolean q3Model = qfalse;

static void UI_DrawPlayerModel ( rectDef_t *rect )
{
	static playerInfo_t info;
	char                model[ MAX_QPATH ];
	char                team[ 256 ];
	char                head[ 256 ];
	vec3_t              viewangles;
	vec3_t              moveangles;

	if ( trap_Cvar_VariableValue ( "ui_Q3Model" ) )
	{
		strcpy ( model, UI_Cvar_VariableString ( "model" ) );
		strcpy ( head, UI_Cvar_VariableString ( "headmodel" ) );

		if ( !q3Model )
		{
			q3Model = qtrue;
			updateModel = qtrue;
		}

		team[ 0 ] = '\0';
	}
	else
	{
		strcpy ( team, UI_Cvar_VariableString ( "ui_teamName" ) );
		strcpy ( model, UI_Cvar_VariableString ( "team_model" ) );
		strcpy ( head, UI_Cvar_VariableString ( "team_headmodel" ) );

		if ( q3Model )
		{
			q3Model = qfalse;
			updateModel = qtrue;
		}
	}

	if ( updateModel )
	{
		memset ( &info, 0, sizeof ( playerInfo_t ) );
		viewangles[ YAW ] = 180 - 10;
		viewangles[ PITCH ] = 0;
		viewangles[ ROLL ] = 0;
		VectorClear ( moveangles );
		UI_PlayerInfo_SetModel ( &info, model, head, team );
		UI_PlayerInfo_SetInfo ( &info, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_MACHINEGUN, qfalse );
//    UI_RegisterClientModelname( &info, model, head, team);
		updateModel = qfalse;
	}

	UI_DrawPlayer ( rect->x, rect->y, rect->w, rect->h, &info, uiInfo.uiDC.realTime / 2 );
}

static void UI_DrawNetSource ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	if ( ui_netSource.integer < 0 || ui_netSource.integer > numNetSources )
	{
		ui_netSource.integer = 0;
	}

	Text_Paint ( rect->x, rect->y, scale, color, va ( "Source: %s", netSources[ ui_netSource.integer ] ), 0, 0, textStyle );
}

static void UI_DrawNetMapPreview ( rectDef_t *rect, float scale, vec4_t color )
{
	if ( uiInfo.serverStatus.currentServerPreview > 0 )
	{
		UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, uiInfo.serverStatus.currentServerPreview );
	}
	else
	{
		UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip ( "gfx/2d/load_screen" ) );
	}
}

static void UI_DrawNetMapCinematic ( rectDef_t *rect, float scale, vec4_t color )
{
	if ( ui_currentNetMap.integer < 0 || ui_currentNetMap.integer > uiInfo.mapCount )
	{
		ui_currentNetMap.integer = 0;
		trap_Cvar_Set ( "ui_currentNetMap", "0" );
	}

	if ( uiInfo.serverStatus.currentServerCinematic >= 0 )
	{
		trap_CIN_RunCinematic ( uiInfo.serverStatus.currentServerCinematic );
		trap_CIN_SetExtents ( uiInfo.serverStatus.currentServerCinematic, rect->x, rect->y, rect->w, rect->h );
		trap_CIN_DrawCinematic ( uiInfo.serverStatus.currentServerCinematic );
	}
	else
	{
		UI_DrawNetMapPreview ( rect, scale, color );
	}
}

static void UI_DrawNetFilter ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	if ( ui_serverFilterType.integer < 0 || ui_serverFilterType.integer > numServerFilters )
	{
		ui_serverFilterType.integer = 0;
	}

	Text_Paint ( rect->x, rect->y, scale, color, va ( "Filter: %s", serverFilters[ ui_serverFilterType.integer ].description ), 0, 0, textStyle );
}

static void UI_DrawTier ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	int i;
	i = trap_Cvar_VariableValue ( "ui_currentTier" );

	if ( i < 0 || i >= uiInfo.tierCount )
	{
		i = 0;
	}

	Text_Paint ( rect->x, rect->y, scale, color, va ( "Tier: %s", uiInfo.tierList[ i ].tierName ), 0, 0, textStyle );
}

static void UI_DrawTierMap ( rectDef_t *rect, int index )
{
	int i;
	i = trap_Cvar_VariableValue ( "ui_currentTier" );

	if ( i < 0 || i >= uiInfo.tierCount )
	{
		i = 0;
	}

	if ( uiInfo.tierList[ i ].mapHandles[ index ] == -1 )
	{
		uiInfo.tierList[ i ].mapHandles[ index ] = trap_R_RegisterShaderNoMip ( va ( "levelshots/%s", uiInfo.tierList[ i ].maps[ index ] ) );
	}

	UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, uiInfo.tierList[ i ].mapHandles[ index ] );
}

static const char *UI_EnglishMapName ( const char *map )
{
	int i;

	for ( i = 0; i < uiInfo.mapCount; i++ )
	{
		if ( Q_stricmp ( map, uiInfo.mapList[ i ].mapLoadName ) == 0 )
		{
			return uiInfo.mapList[ i ].mapName;
		}
	}

	return "";
}

static void UI_DrawTierMapName ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	int i, j;
	i = trap_Cvar_VariableValue ( "ui_currentTier" );

	if ( i < 0 || i >= uiInfo.tierCount )
	{
		i = 0;
	}

	j = trap_Cvar_VariableValue ( "ui_currentMap" );

	if ( j < 0 || j > MAPS_PER_TIER )
	{
		j = 0;
	}

	Text_Paint ( rect->x, rect->y, scale, color, UI_EnglishMapName ( uiInfo.tierList[ i ].maps[ j ] ), 0, 0, textStyle );
}

static void UI_DrawTierGameType ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	int i, j;
	i = trap_Cvar_VariableValue ( "ui_currentTier" );

	if ( i < 0 || i >= uiInfo.tierCount )
	{
		i = 0;
	}

	j = trap_Cvar_VariableValue ( "ui_currentMap" );

	if ( j < 0 || j > MAPS_PER_TIER )
	{
		j = 0;
	}

	Text_Paint ( rect->x, rect->y, scale, color, uiInfo.gameTypes[ uiInfo.tierList[ i ].gameTypes[ j ] ].gameType, 0, 0, textStyle );
}

static const char *UI_AIFromName ( const char *name )
{
	int j;

	for ( j = 0; j < uiInfo.aliasCount; j++ )
	{
		if ( Q_stricmp ( uiInfo.aliasList[ j ].name, name ) == 0 )
		{
			return uiInfo.aliasList[ j ].ai;
		}
	}

	return "James";
}

static qboolean updateOpponentModel = qtrue;
static void UI_DrawOpponent ( rectDef_t *rect )
{
	static playerInfo_t info2;
	char                model[ MAX_QPATH ];
	char                headmodel[ MAX_QPATH ];
	char                team[ 256 ];
	vec3_t              viewangles;
	vec3_t              moveangles;

	if ( updateOpponentModel )
	{
		strcpy ( model, UI_Cvar_VariableString ( "ui_opponentModel" ) );
		strcpy ( headmodel, UI_Cvar_VariableString ( "ui_opponentModel" ) );
		team[ 0 ] = '\0';

		memset ( &info2, 0, sizeof ( playerInfo_t ) );
		viewangles[ YAW ] = 180 - 10;
		viewangles[ PITCH ] = 0;
		viewangles[ ROLL ] = 0;
		VectorClear ( moveangles );
		UI_PlayerInfo_SetModel ( &info2, model, headmodel, "" );
		UI_PlayerInfo_SetInfo ( &info2, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_MACHINEGUN, qfalse );
		UI_RegisterClientModelname ( &info2, model, headmodel, team );
		updateOpponentModel = qfalse;
	}

	UI_DrawPlayer ( rect->x, rect->y, rect->w, rect->h, &info2, uiInfo.uiDC.realTime / 2 );
}

static void UI_NextOpponent ( void )
{
	int i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_opponentName" ) );
	int j = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_teamName" ) );
	i++;

	if ( i >= uiInfo.teamCount )
	{
		i = 0;
	}

	if ( i == j )
	{
		i++;

		if ( i >= uiInfo.teamCount )
		{
			i = 0;
		}
	}

	trap_Cvar_Set ( "ui_opponentName", uiInfo.teamList[ i ].teamName );
}

static void UI_PriorOpponent ( void )
{
	int i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_opponentName" ) );
	int j = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_teamName" ) );
	i--;

	if ( i < 0 )
	{
		i = uiInfo.teamCount - 1;
	}

	if ( i == j )
	{
		i--;

		if ( i < 0 )
		{
			i = uiInfo.teamCount - 1;
		}
	}

	trap_Cvar_Set ( "ui_opponentName", uiInfo.teamList[ i ].teamName );
}

static void UI_DrawPlayerLogo ( rectDef_t *rect, vec3_t color )
{
	int i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_teamName" ) );

	if ( uiInfo.teamList[ i ].teamIcon == -1 )
	{
		uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip ( uiInfo.teamList[ i ].imageName );
		uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip ( va ( "%s_metal", uiInfo.teamList[ i ].imageName ) );
		uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip ( va ( "%s_name", uiInfo.teamList[ i ].imageName ) );
	}

	trap_R_SetColor ( color );
	UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon );
	trap_R_SetColor ( NULL );
}

static void UI_DrawPlayerLogoMetal ( rectDef_t *rect, vec3_t color )
{
	int i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_teamName" ) );

	if ( uiInfo.teamList[ i ].teamIcon == -1 )
	{
		uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip ( uiInfo.teamList[ i ].imageName );
		uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip ( va ( "%s_metal", uiInfo.teamList[ i ].imageName ) );
		uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip ( va ( "%s_name", uiInfo.teamList[ i ].imageName ) );
	}

	trap_R_SetColor ( color );
	UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon_Metal );
	trap_R_SetColor ( NULL );
}

static void UI_DrawPlayerLogoName ( rectDef_t *rect, vec3_t color )
{
	int i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_teamName" ) );

	if ( uiInfo.teamList[ i ].teamIcon == -1 )
	{
		uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip ( uiInfo.teamList[ i ].imageName );
		uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip ( va ( "%s_metal", uiInfo.teamList[ i ].imageName ) );
		uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip ( va ( "%s_name", uiInfo.teamList[ i ].imageName ) );
	}

	trap_R_SetColor ( color );
	UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon_Name );
	trap_R_SetColor ( NULL );
}

static void UI_DrawOpponentLogo ( rectDef_t *rect, vec3_t color )
{
	int i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_opponentName" ) );

	if ( uiInfo.teamList[ i ].teamIcon == -1 )
	{
		uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip ( uiInfo.teamList[ i ].imageName );
		uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip ( va ( "%s_metal", uiInfo.teamList[ i ].imageName ) );
		uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip ( va ( "%s_name", uiInfo.teamList[ i ].imageName ) );
	}

	trap_R_SetColor ( color );
	UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon );
	trap_R_SetColor ( NULL );
}

static void UI_DrawOpponentLogoMetal ( rectDef_t *rect, vec3_t color )
{
	int i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_opponentName" ) );

	if ( uiInfo.teamList[ i ].teamIcon == -1 )
	{
		uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip ( uiInfo.teamList[ i ].imageName );
		uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip ( va ( "%s_metal", uiInfo.teamList[ i ].imageName ) );
		uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip ( va ( "%s_name", uiInfo.teamList[ i ].imageName ) );
	}

	trap_R_SetColor ( color );
	UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon_Metal );
	trap_R_SetColor ( NULL );
}

static void UI_DrawOpponentLogoName ( rectDef_t *rect, vec3_t color )
{
	int i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_opponentName" ) );

	if ( uiInfo.teamList[ i ].teamIcon == -1 )
	{
		uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip ( uiInfo.teamList[ i ].imageName );
		uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip ( va ( "%s_metal", uiInfo.teamList[ i ].imageName ) );
		uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip ( va ( "%s_name", uiInfo.teamList[ i ].imageName ) );
	}

	trap_R_SetColor ( color );
	UI_DrawHandlePic ( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon_Name );
	trap_R_SetColor ( NULL );
}

static void UI_DrawAllMapsSelection ( rectDef_t *rect, float scale, vec4_t color, int textStyle, qboolean net )
{
	int map = ( net ) ? ui_currentNetMap.integer : ui_currentMap.integer;

	if ( map >= 0 && map < uiInfo.mapCount )
	{
		Text_Paint ( rect->x, rect->y, scale, color, uiInfo.mapList[ map ].mapName, 0, 0, textStyle );
	}
}

static void UI_DrawOpponentName ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	Text_Paint ( rect->x, rect->y, scale, color, UI_Cvar_VariableString ( "ui_opponentName" ), 0, 0, textStyle );
}

static int UI_OwnerDrawWidth ( int ownerDraw, float scale )
{
	int        i, h, value;
	const char *text;
	const char *s = NULL;

	switch ( ownerDraw )
	{
		case UI_HANDICAP:
			h = Com_Clamp ( 5, 100, trap_Cvar_VariableValue ( "handicap" ) );
			i = 20 - h / 5;
			s = handicapValues[ i ];
			break;

		case UI_CLANNAME:
			s = UI_Cvar_VariableString ( "ui_teamName" );
			break;

		case UI_GAMETYPE:
			s = uiInfo.gameTypes[ ui_gameType.integer ].gameType;
			break;

		case UI_SKILL:
			i = trap_Cvar_VariableValue ( "g_spSkill" );

			if ( i < 1 || i > numSkillLevels )
			{
				i = 1;
			}

			s = skillLevels[ i - 1 ];
			break;

		case UI_BLUETEAMNAME:
			i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_blueTeam" ) );

			if ( i >= 0 && i < uiInfo.teamCount )
			{
				s = va ( "%s: %s", "Blue", uiInfo.teamList[ i ].teamName );
			}

			break;

		case UI_REDTEAMNAME:
			i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_redTeam" ) );

			if ( i >= 0 && i < uiInfo.teamCount )
			{
				s = va ( "%s: %s", "Red", uiInfo.teamList[ i ].teamName );
			}

			break;

		case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
			value = trap_Cvar_VariableValue ( va ( "ui_blueteam%i", ownerDraw - UI_BLUETEAM1 + 1 ) );

			if ( value <= 0 )
			{
				text = "Closed";
			}
			else if ( value == 1 )
			{
				text = "Human";
			}
			else
			{
				value -= 2;

				if ( value >= uiInfo.aliasCount )
				{
					value = 0;
				}

				text = uiInfo.aliasList[ value ].name;
			}

			s = va ( "%i. %s", ownerDraw - UI_BLUETEAM1 + 1, text );
			break;

		case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
			value = trap_Cvar_VariableValue ( va ( "ui_redteam%i", ownerDraw - UI_REDTEAM1 + 1 ) );

			if ( value <= 0 )
			{
				text = "Closed";
			}
			else if ( value == 1 )
			{
				text = "Human";
			}
			else
			{
				value -= 2;

				if ( value >= uiInfo.aliasCount )
				{
					value = 0;
				}

				text = uiInfo.aliasList[ value ].name;
			}

			s = va ( "%i. %s", ownerDraw - UI_REDTEAM1 + 1, text );
			break;

		case UI_NETSOURCE:
			if ( ui_netSource.integer < 0 || ui_netSource.integer > uiInfo.numJoinGameTypes )
			{
				ui_netSource.integer = 0;
			}

			s = va ( "Source: %s", netSources[ ui_netSource.integer ] );
			break;

		case UI_NETFILTER:
			if ( ui_serverFilterType.integer < 0 || ui_serverFilterType.integer > numServerFilters )
			{
				ui_serverFilterType.integer = 0;
			}

			s = va ( "Filter: %s", serverFilters[ ui_serverFilterType.integer ].description );
			break;

		case UI_TIER:
			break;

		case UI_TIER_MAPNAME:
			break;

		case UI_TIER_GAMETYPE:
			break;

		case UI_ALLMAPS_SELECTION:
			break;

		case UI_OPPONENT_NAME:
			break;

		case UI_KEYBINDSTATUS:
			if ( Display_KeyBindPending() )
			{
				s = trap_TranslateString ( "Waiting for new key... Press ESCAPE to cancel" );
			}
			else
			{
				s = trap_TranslateString ( "Press ENTER or CLICK to change, Press BACKSPACE to clear" );
			}

			break;

		case UI_SERVERREFRESHDATE:
			s = UI_Cvar_VariableString ( va ( "ui_lastServerRefresh_%i", ui_netSource.integer ) );
			break;

		default:
			break;
	}

	if ( s )
	{
		return Text_Width ( s, scale, 0 );
	}

	return 0;
}

static void UI_DrawBotName ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	int        value = uiInfo.botIndex;
	const char *text = "";

	if ( value >= UI_GetNumBots() )
	{
		value = 0;
	}

	text = UI_GetBotNameByNumber ( value );

	Text_Paint ( rect->x, rect->y, scale, color, text, 0, 0, textStyle );
}

static void UI_DrawBotSkill ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	if ( uiInfo.skillIndex >= 0 && uiInfo.skillIndex < numSkillLevels )
	{
		Text_Paint ( rect->x, rect->y, scale, color, skillLevels[ uiInfo.skillIndex ], 0, 0, textStyle );
	}
}

static void UI_DrawRedBlue ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	Text_Paint ( rect->x, rect->y, scale, color, ( uiInfo.redBlue == 0 ) ? "Red" : "Blue", 0, 0, textStyle );
}

/*
===============
UI_BuildPlayerList
===============
*/
static void UI_BuildPlayerList ( void )
{
	uiClientState_t cs;
	int             n, count, team, team2, playerTeamNumber;
	char            info[ MAX_INFO_STRING ];

	trap_GetClientState ( &cs );
	trap_GetConfigString ( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
	uiInfo.playerNumber = cs.clientNum;
	uiInfo.teamLeader = atoi ( Info_ValueForKey ( info, "tl" ) );
	team = atoi ( Info_ValueForKey ( info, "t" ) );
	trap_GetConfigString ( CS_SERVERINFO, info, sizeof ( info ) );
	count = atoi ( Info_ValueForKey ( info, "sv_maxclients" ) );
	uiInfo.playerCount = 0;
	uiInfo.myTeamCount = 0;
	playerTeamNumber = 0;

	for ( n = 0; n < count; n++ )
	{
		trap_GetConfigString ( CS_PLAYERS + n, info, MAX_INFO_STRING );

		if ( info[ 0 ] )
		{
			Q_strncpyz ( uiInfo.playerNames[ uiInfo.playerCount ], Info_ValueForKey ( info, "n" ), MAX_NAME_LENGTH );
			Q_CleanStr ( uiInfo.playerNames[ uiInfo.playerCount ] );
			uiInfo.playerCount++;
			team2 = atoi ( Info_ValueForKey ( info, "t" ) );

			if ( team2 == team )
			{
				Q_strncpyz ( uiInfo.teamNames[ uiInfo.myTeamCount ], Info_ValueForKey ( info, "n" ), MAX_NAME_LENGTH );
				Q_CleanStr ( uiInfo.teamNames[ uiInfo.myTeamCount ] );
				uiInfo.teamClientNums[ uiInfo.myTeamCount ] = n;

				if ( uiInfo.playerNumber == n )
				{
					playerTeamNumber = uiInfo.myTeamCount;
				}

				uiInfo.myTeamCount++;
			}
		}
	}

	if ( !uiInfo.teamLeader )
	{
		trap_Cvar_Set ( "cg_selectedPlayer", va ( "%d", playerTeamNumber ) );
	}

	n = trap_Cvar_VariableValue ( "cg_selectedPlayer" );

	if ( n < 0 || n > uiInfo.myTeamCount )
	{
		n = 0;
	}

	if ( n < uiInfo.myTeamCount )
	{
		trap_Cvar_Set ( "cg_selectedPlayerName", uiInfo.teamNames[ n ] );
	}
}

static void UI_DrawSelectedPlayer ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	if ( uiInfo.uiDC.realTime > uiInfo.playerRefresh )
	{
		uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
		UI_BuildPlayerList();
	}

	Text_Paint ( rect->x, rect->y, scale, color, ( uiInfo.teamLeader ) ? UI_Cvar_VariableString ( "cg_selectedPlayerName" ) : UI_Cvar_VariableString ( "name" ), 0, 0, textStyle );
}

static void UI_DrawServerRefreshDate ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	if ( uiInfo.serverStatus.refreshActive )
	{
		vec4_t lowLight, newColor;
		lowLight[ 0 ] = 0.8 * color[ 0 ];
		lowLight[ 1 ] = 0.8 * color[ 1 ];
		lowLight[ 2 ] = 0.8 * color[ 2 ];
		lowLight[ 3 ] = 0.8 * color[ 3 ];
		LerpColor ( color, lowLight, newColor, 0.5 + 0.5 * sin ( uiInfo.uiDC.realTime / PULSE_DIVISOR ) );
		Text_Paint ( rect->x, rect->y, scale, newColor, va ( trap_TranslateString ( "Getting info for %d servers (ESC to cancel)" ),
		             trap_LAN_GetServerCount ( ui_netSource.integer ) ), 0, 0, textStyle );
	}
	else
	{
		char buff[ 64 ];
		Q_strncpyz ( buff, UI_Cvar_VariableString ( va ( "ui_lastServerRefresh_%i", ui_netSource.integer ) ), 64 );
		Text_Paint ( rect->x, rect->y, scale, color, va ( trap_TranslateString ( "Refresh Time: %s" ), buff ), 0, 0, textStyle );
	}
}

static void UI_DrawServerMOTD ( rectDef_t *rect, float scale, vec4_t color )
{
	if ( uiInfo.serverStatus.motdLen )
	{
		float maxX;

		if ( uiInfo.serverStatus.motdWidth == -1 )
		{
			uiInfo.serverStatus.motdWidth = 0;
			uiInfo.serverStatus.motdPaintX = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if ( uiInfo.serverStatus.motdOffset > uiInfo.serverStatus.motdLen )
		{
			uiInfo.serverStatus.motdOffset = 0;
			uiInfo.serverStatus.motdPaintX = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if ( uiInfo.uiDC.realTime > uiInfo.serverStatus.motdTime )
		{
			uiInfo.serverStatus.motdTime = uiInfo.uiDC.realTime + 10;

			if ( uiInfo.serverStatus.motdPaintX <= rect->x + 2 )
			{
				if ( uiInfo.serverStatus.motdOffset < uiInfo.serverStatus.motdLen )
				{
					uiInfo.serverStatus.motdPaintX += Text_Width ( &uiInfo.serverStatus.motd[ uiInfo.serverStatus.motdOffset ], scale, 1 ) - 1;
					uiInfo.serverStatus.motdOffset++;
				}
				else
				{
					uiInfo.serverStatus.motdOffset = 0;

					if ( uiInfo.serverStatus.motdPaintX2 >= 0 )
					{
						uiInfo.serverStatus.motdPaintX = uiInfo.serverStatus.motdPaintX2;
					}
					else
					{
						uiInfo.serverStatus.motdPaintX = rect->x + rect->w - 2;
					}

					uiInfo.serverStatus.motdPaintX2 = -1;
				}
			}
			else
			{
				//serverStatus.motdPaintX--;
				uiInfo.serverStatus.motdPaintX -= 2;

				if ( uiInfo.serverStatus.motdPaintX2 >= 0 )
				{
					//serverStatus.motdPaintX2--;
					uiInfo.serverStatus.motdPaintX2 -= 2;
				}
			}
		}

		maxX = rect->x + rect->w - 2;
		Text_Paint_Limit ( &maxX, uiInfo.serverStatus.motdPaintX, rect->y + rect->h - 3, scale, color, &uiInfo.serverStatus.motd[ uiInfo.serverStatus.motdOffset ], 0, 0 );

		if ( uiInfo.serverStatus.motdPaintX2 >= 0 )
		{
			float maxX2 = rect->x + rect->w - 2;
			Text_Paint_Limit ( &maxX2, uiInfo.serverStatus.motdPaintX2, rect->y + rect->h - 3, scale, color, uiInfo.serverStatus.motd, 0, uiInfo.serverStatus.motdOffset );
		}

		if ( uiInfo.serverStatus.motdOffset && maxX > 0 )
		{
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if ( uiInfo.serverStatus.motdPaintX2 == -1 )
			{
				uiInfo.serverStatus.motdPaintX2 = rect->x + rect->w - 2;
			}
		}
		else
		{
			uiInfo.serverStatus.motdPaintX2 = -1;
		}
	}
}

static void UI_DrawKeyBindStatus ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
//  int ofs = 0; TTimo: unused
	if ( Display_KeyBindPending() )
	{
		Text_Paint ( rect->x, rect->y, scale, color, trap_TranslateString ( "Waiting for new key... Press ESCAPE to cancel" ), 0, 0, textStyle );
	}
	else
	{
		Text_Paint ( rect->x, rect->y, scale, color, trap_TranslateString ( "Press ENTER or CLICK to change, Press BACKSPACE to clear" ), 0, 0, textStyle );
	}
}

static void UI_DrawGLInfo ( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	char        *eptr;
	char       buff[ 1024 ];
	const char *lines[ 64 ];
	int        y, numLines, i;

	Text_Paint ( rect->x + 2, rect->y, scale, color, va ( "VENDOR: %s", uiInfo.uiDC.glconfig.vendor_string ), 0, 30, textStyle );
	Text_Paint ( rect->x + 2, rect->y + 15, scale, color, va ( "VERSION: %s: %s", uiInfo.uiDC.glconfig.version_string, uiInfo.uiDC.glconfig.renderer_string ), 0, 30, textStyle );
	Text_Paint ( rect->x + 2, rect->y + 30, scale, color, va ( "PIXELFORMAT: color(%d-bits) Z(%d-bits) stencil(%d-bits)", uiInfo.uiDC.glconfig.colorBits, uiInfo.uiDC.glconfig.depthBits, uiInfo.uiDC.glconfig.stencilBits ), 0, 30, textStyle );

	// build null terminated extension strings
	// TTimo: https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=399
	// in TA this was not directly crashing, but displaying a nasty broken shader right in the middle
	// brought down the string size to 1024, there's not much that can be shown on the screen anyway
	Q_strncpyz ( buff, uiInfo.uiDC.glconfig.extensions_string, 1024 );
	eptr = buff;
	y = rect->y + 45;
	numLines = 0;

	while ( y < rect->y + rect->h && *eptr )
	{
		while ( *eptr && *eptr == ' ' )
		{
			*eptr++ = '\0';
		}

		// track start of valid string
		if ( *eptr && *eptr != ' ' )
		{
			lines[ numLines++ ] = eptr;
		}

		while ( *eptr && *eptr != ' ' )
		{
			eptr++;
		}
	}

	i = 0;

	while ( i < numLines )
	{
		Text_Paint ( rect->x + 2, y, scale, color, lines[ i++ ], 0, 20, textStyle );

		if ( i < numLines )
		{
			Text_Paint ( rect->x + rect->w / 2, y, scale, color, lines[ i++ ], 0, 20, textStyle );
		}

		y += 10;

		if ( y > rect->y + rect->h - 11 )
		{
			break;
		}
	}
}

// FIXME: table drive
//
static void UI_OwnerDraw ( float x, float y, float w, float h,
                           float text_x, float text_y, int ownerDraw,
                           int ownerDrawFlags, int align, float special,
                           float scale, vec4_t color, qhandle_t shader, int textStyle )
{
	rectDef_t      rect;
	tremInfoPane_t *pane = NULL;

	rect.x = x + text_x;
	rect.y = y + text_y;
	rect.w = w;
	rect.h = h;

	switch ( ownerDraw )
	{
		case UI_TEAMINFOPANE:
			if ( ( pane = uiInfo.tremTeamList[ uiInfo.tremTeamIndex ].infopane ) )
			{
				UI_DrawInfoPane ( pane, &rect, text_x, text_y, scale, color, textStyle );
			}

			break;

		case UI_ACLASSINFOPANE:
			if ( ( pane = uiInfo.tremAlienClassList[ uiInfo.tremAlienClassIndex ].infopane ) )
			{
				UI_DrawInfoPane ( pane, &rect, text_x, text_y, scale, color, textStyle );
			}

			break;

		case UI_AUPGRADEINFOPANE:
			if ( ( pane = uiInfo.tremAlienUpgradeList[ uiInfo.tremAlienUpgradeIndex ].infopane ) )
			{
				UI_DrawInfoPane ( pane, &rect, text_x, text_y, scale, color, textStyle );
			}

			break;

		case UI_HITEMINFOPANE:
			if ( ( pane = uiInfo.tremHumanItemList[ uiInfo.tremHumanItemIndex ].infopane ) )
			{
				UI_DrawInfoPane ( pane, &rect, text_x, text_y, scale, color, textStyle );
			}

			break;

		case UI_HBUYINFOPANE:
			if ( ( pane = uiInfo.tremHumanArmouryBuyList[ uiInfo.tremHumanArmouryBuyIndex ].infopane ) )
			{
				UI_DrawInfoPane ( pane, &rect, text_x, text_y, scale, color, textStyle );
			}

			break;

		case UI_HSELLINFOPANE:
			if ( ( pane = uiInfo.tremHumanArmourySellList[ uiInfo.tremHumanArmourySellIndex ].infopane ) )
			{
				UI_DrawInfoPane ( pane, &rect, text_x, text_y, scale, color, textStyle );
			}

			break;

		case UI_ABUILDINFOPANE:
			if ( ( pane = uiInfo.tremAlienBuildList[ uiInfo.tremAlienBuildIndex ].infopane ) )
			{
				UI_DrawInfoPane ( pane, &rect, text_x, text_y, scale, color, textStyle );
			}

			break;

		case UI_HBUILDINFOPANE:
			if ( ( pane = uiInfo.tremHumanBuildList[ uiInfo.tremHumanBuildIndex ].infopane ) )
			{
				UI_DrawInfoPane ( pane, &rect, text_x, text_y, scale, color, textStyle );
			}

			break;

		case UI_HANDICAP:
			UI_DrawHandicap ( &rect, scale, color, textStyle );
			break;

		case UI_PLAYERMODEL:
			UI_DrawPlayerModel ( &rect );
			break;

		case UI_CLANNAME:
			UI_DrawClanName ( &rect, scale, color, textStyle );
			break;

		case UI_CLANLOGO:
			UI_DrawClanLogo ( &rect, scale, color );
			break;

		case UI_CLANCINEMATIC:
			UI_DrawClanCinematic ( &rect, scale, color );
			break;

		case UI_PREVIEWCINEMATIC:
			UI_DrawPreviewCinematic ( &rect, scale, color );
			break;

		case UI_GAMETYPE:
			UI_DrawGameType ( &rect, scale, color, textStyle );
			break;

		case UI_NETGAMETYPE:
			UI_DrawNetGameType ( &rect, scale, color, textStyle );
			break;

		case UI_JOINGAMETYPE:
			UI_DrawJoinGameType ( &rect, scale, color, textStyle );
			break;

		case UI_MAPPREVIEW:
			UI_DrawMapPreview ( &rect, scale, color, qtrue );
			break;

		case UI_MAP_TIMETOBEAT:
			UI_DrawMapTimeToBeat ( &rect, scale, color, textStyle );
			break;

		case UI_MAPCINEMATIC:
			UI_DrawMapCinematic ( &rect, scale, color, qfalse );
			break;

		case UI_STARTMAPCINEMATIC:
			UI_DrawMapCinematic ( &rect, scale, color, qtrue );
			break;

		case UI_SKILL:
			UI_DrawSkill ( &rect, scale, color, textStyle );
			break;

		case UI_BLUETEAMNAME:
			UI_DrawTeamName ( &rect, scale, color, qtrue, textStyle );
			break;

		case UI_REDTEAMNAME:
			UI_DrawTeamName ( &rect, scale, color, qfalse, textStyle );
			break;

		case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
			UI_DrawTeamMember ( &rect, scale, color, qtrue, ownerDraw - UI_BLUETEAM1 + 1, textStyle );
			break;

		case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
			UI_DrawTeamMember ( &rect, scale, color, qfalse, ownerDraw - UI_REDTEAM1 + 1, textStyle );
			break;

		case UI_NETSOURCE:
			UI_DrawNetSource ( &rect, scale, color, textStyle );
			break;

		case UI_NETMAPPREVIEW:
			UI_DrawNetMapPreview ( &rect, scale, color );
			break;

		case UI_NETMAPCINEMATIC:
			UI_DrawNetMapCinematic ( &rect, scale, color );
			break;

		case UI_NETFILTER:
			UI_DrawNetFilter ( &rect, scale, color, textStyle );
			break;

		case UI_TIER:
			UI_DrawTier ( &rect, scale, color, textStyle );
			break;

		case UI_OPPONENTMODEL:
			UI_DrawOpponent ( &rect );
			break;

		case UI_TIERMAP1:
			UI_DrawTierMap ( &rect, 0 );
			break;

		case UI_TIERMAP2:
			UI_DrawTierMap ( &rect, 1 );
			break;

		case UI_TIERMAP3:
			UI_DrawTierMap ( &rect, 2 );
			break;

		case UI_PLAYERLOGO:
			UI_DrawPlayerLogo ( &rect, color );
			break;

		case UI_PLAYERLOGO_METAL:
			UI_DrawPlayerLogoMetal ( &rect, color );
			break;

		case UI_PLAYERLOGO_NAME:
			UI_DrawPlayerLogoName ( &rect, color );
			break;

		case UI_OPPONENTLOGO:
			UI_DrawOpponentLogo ( &rect, color );
			break;

		case UI_OPPONENTLOGO_METAL:
			UI_DrawOpponentLogoMetal ( &rect, color );
			break;

		case UI_OPPONENTLOGO_NAME:
			UI_DrawOpponentLogoName ( &rect, color );
			break;

		case UI_TIER_MAPNAME:
			UI_DrawTierMapName ( &rect, scale, color, textStyle );
			break;

		case UI_TIER_GAMETYPE:
			UI_DrawTierGameType ( &rect, scale, color, textStyle );
			break;

		case UI_ALLMAPS_SELECTION:
			UI_DrawAllMapsSelection ( &rect, scale, color, textStyle, qtrue );
			break;

		case UI_MAPS_SELECTION:
			UI_DrawAllMapsSelection ( &rect, scale, color, textStyle, qfalse );
			break;

		case UI_OPPONENT_NAME:
			UI_DrawOpponentName ( &rect, scale, color, textStyle );
			break;

		case UI_BOTNAME:
			UI_DrawBotName ( &rect, scale, color, textStyle );
			break;

		case UI_BOTSKILL:
			UI_DrawBotSkill ( &rect, scale, color, textStyle );
			break;

		case UI_REDBLUE:
			UI_DrawRedBlue ( &rect, scale, color, textStyle );
			break;

		case UI_SELECTEDPLAYER:
			UI_DrawSelectedPlayer ( &rect, scale, color, textStyle );
			break;

		case UI_SERVERREFRESHDATE:
			UI_DrawServerRefreshDate ( &rect, scale, color, textStyle );
			break;

		case UI_SERVERMOTD:
			UI_DrawServerMOTD ( &rect, scale, color );
			break;

		case UI_GLINFO:
			UI_DrawGLInfo ( &rect, scale, color, textStyle );
			break;

		case UI_KEYBINDSTATUS:
			UI_DrawKeyBindStatus ( &rect, scale, color, textStyle );
			break;

		default:
			break;
	}
}

static qboolean UI_OwnerDrawVisible ( int flags )
{
	qboolean        vis = qtrue;
	uiClientState_t cs;
	pTeam_t         team;
	char            info[ MAX_INFO_STRING ];

	trap_GetClientState ( &cs );
	trap_GetConfigString ( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
	team = atoi ( Info_ValueForKey ( info, "t" ) );

	while ( flags )
	{
		if ( flags & UI_SHOW_NOTSPECTATING )
		{
			if ( team == PTE_NONE )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_NOTSPECTATING;
		}

		if ( flags & UI_SHOW_VOTEACTIVE )
		{
			if ( !trap_Cvar_VariableValue ( "ui_voteActive" ) )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_VOTEACTIVE;
		}

		if ( flags & UI_SHOW_CANVOTE )
		{
			if ( trap_Cvar_VariableValue ( "ui_voteActive" ) )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_CANVOTE;
		}

		if ( flags & UI_SHOW_TEAMVOTEACTIVE )
		{
			if ( team == PTE_ALIENS )
			{
				if ( !trap_Cvar_VariableValue ( "ui_alienTeamVoteActive" ) )
				{
					vis = qfalse;
				}
			}
			else if ( team == PTE_HUMANS )
			{
				if ( !trap_Cvar_VariableValue ( "ui_humanTeamVoteActive" ) )
				{
					vis = qfalse;
				}
			}

			flags &= ~UI_SHOW_TEAMVOTEACTIVE;
		}

		if ( flags & UI_SHOW_CANTEAMVOTE )
		{
			if ( team == PTE_ALIENS )
			{
				if ( trap_Cvar_VariableValue ( "ui_alienTeamVoteActive" ) )
				{
					vis = qfalse;
				}
			}
			else if ( team == PTE_HUMANS )
			{
				if ( trap_Cvar_VariableValue ( "ui_humanTeamVoteActive" ) )
				{
					vis = qfalse;
				}
			}

			flags &= ~UI_SHOW_CANTEAMVOTE;
		}

		if ( flags & UI_SHOW_LEADER )
		{
			// these need to show when this client can give orders to a player or a group
			if ( !uiInfo.teamLeader )
			{
				vis = qfalse;
			}
			else
			{
				// if showing yourself
				if ( ui_selectedPlayer.integer < uiInfo.myTeamCount && uiInfo.teamClientNums[ ui_selectedPlayer.integer ] == uiInfo.playerNumber )
				{
					vis = qfalse;
				}
			}

			flags &= ~UI_SHOW_LEADER;
		}

		if ( flags & UI_SHOW_NOTLEADER )
		{
			// these need to show when this client is assigning their own status or they are NOT the leader
			if ( uiInfo.teamLeader )
			{
				// if not showing yourself
				if ( ! ( ui_selectedPlayer.integer < uiInfo.myTeamCount && uiInfo.teamClientNums[ ui_selectedPlayer.integer ] == uiInfo.playerNumber ) )
				{
					vis = qfalse;
				}

				// these need to show when this client can give orders to a player or a group
			}

			flags &= ~UI_SHOW_NOTLEADER;
		}

		if ( flags & UI_SHOW_FAVORITESERVERS )
		{
			// this assumes you only put this type of display flag on something showing in the proper context
			if ( ui_netSource.integer != AS_FAVORITES )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_FAVORITESERVERS;
		}

		if ( flags & UI_SHOW_NOTFAVORITESERVERS )
		{
			// this assumes you only put this type of display flag on something showing in the proper context
			if ( ui_netSource.integer == AS_FAVORITES )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_NOTFAVORITESERVERS;
		}

		if ( flags & UI_SHOW_NEWHIGHSCORE )
		{
			if ( uiInfo.newHighScoreTime < uiInfo.uiDC.realTime )
			{
				vis = qfalse;
			}
			else
			{
				if ( uiInfo.soundHighScore )
				{
					if ( trap_Cvar_VariableValue ( "sv_killserver" ) == 0 )
					{
						// wait on server to go down before playing sound
						trap_S_StartLocalSound ( uiInfo.newHighScoreSound, CHAN_ANNOUNCER );
						uiInfo.soundHighScore = qfalse;
					}
				}
			}

			flags &= ~UI_SHOW_NEWHIGHSCORE;
		}

		if ( flags & UI_SHOW_NEWBESTTIME )
		{
			if ( uiInfo.newBestTime < uiInfo.uiDC.realTime )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_NEWBESTTIME;
		}

		if ( flags & UI_SHOW_DEMOAVAILABLE )
		{
			if ( !uiInfo.demoAvailable )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_DEMOAVAILABLE;
		}
		else
		{
			flags = 0;
		}
	}

	return vis;
}

static qboolean UI_Handicap_HandleKey ( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		int h;
		h = Com_Clamp ( 5, 100, trap_Cvar_VariableValue ( "handicap" ) );

		if ( key == K_MOUSE2 )
		{
			h -= 5;
		}
		else
		{
			h += 5;
		}

		if ( h > 100 )
		{
			h = 5;
		}
		else if ( h < 0 )
		{
			h = 100;
		}

		trap_Cvar_Set ( "handicap", va ( "%i", h ) );
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_ClanName_HandleKey ( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		int i;
		i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_teamName" ) );

		if ( uiInfo.teamList[ i ].cinematic >= 0 )
		{
			trap_CIN_StopCinematic ( uiInfo.teamList[ i ].cinematic );
			uiInfo.teamList[ i ].cinematic = -1;
		}

		if ( key == K_MOUSE2 )
		{
			i--;
		}
		else
		{
			i++;
		}

		if ( i >= uiInfo.teamCount )
		{
			i = 0;
		}
		else if ( i < 0 )
		{
			i = uiInfo.teamCount - 1;
		}

		trap_Cvar_Set ( "ui_teamName", uiInfo.teamList[ i ].teamName );
		UI_HeadCountByTeam();
		UI_FeederSelection ( FEEDER_HEADS, 0 );
		updateModel = qtrue;
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_GameType_HandleKey ( int flags, float *special, int key, qboolean resetMap )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		int oldCount = UI_MapCountByGameType ( qtrue );

		// hard coded mess here
		if ( key == K_MOUSE2 )
		{
			ui_gameType.integer--;

			if ( ui_gameType.integer == 2 )
			{
				ui_gameType.integer = 1;
			}
			else if ( ui_gameType.integer < 2 )
			{
				ui_gameType.integer = uiInfo.numGameTypes - 1;
			}
		}
		else
		{
			ui_gameType.integer++;

			if ( ui_gameType.integer >= uiInfo.numGameTypes )
			{
				ui_gameType.integer = 1;
			}
			else if ( ui_gameType.integer == 2 )
			{
				ui_gameType.integer = 3;
			}
		}

		trap_Cvar_Set ( "ui_Q3Model", "0" );

		trap_Cvar_Set ( "ui_gameType", va ( "%d", ui_gameType.integer ) );
		UI_SetCapFragLimits ( qtrue );
		UI_LoadBestScores ( uiInfo.mapList[ ui_currentMap.integer ].mapLoadName, uiInfo.gameTypes[ ui_gameType.integer ].gtEnum );

		if ( resetMap && oldCount != UI_MapCountByGameType ( qtrue ) )
		{
			trap_Cvar_Set ( "ui_currentMap", "0" );
			Menu_SetFeederSelection ( NULL, FEEDER_MAPS, 0, NULL );
		}

		return qtrue;
	}

	return qfalse;
}

static qboolean UI_NetGameType_HandleKey ( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		if ( key == K_MOUSE2 )
		{
			ui_netGameType.integer--;
		}
		else
		{
			ui_netGameType.integer++;
		}

		if ( ui_netGameType.integer < 0 )
		{
			ui_netGameType.integer = uiInfo.numGameTypes - 1;
		}
		else if ( ui_netGameType.integer >= uiInfo.numGameTypes )
		{
			ui_netGameType.integer = 0;
		}

		trap_Cvar_Set ( "ui_netGameType", va ( "%d", ui_netGameType.integer ) );
		trap_Cvar_Set ( "ui_actualnetGameType", va ( "%d", uiInfo.gameTypes[ ui_netGameType.integer ].gtEnum ) );
		trap_Cvar_Set ( "ui_currentNetMap", "0" );
		UI_MapCountByGameType ( qfalse );
		Menu_SetFeederSelection ( NULL, FEEDER_ALLMAPS, 0, NULL );
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_JoinGameType_HandleKey ( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		if ( key == K_MOUSE2 )
		{
			ui_joinGameType.integer--;
		}
		else
		{
			ui_joinGameType.integer++;
		}

		if ( ui_joinGameType.integer < 0 )
		{
			ui_joinGameType.integer = uiInfo.numJoinGameTypes - 1;
		}
		else if ( ui_joinGameType.integer >= uiInfo.numJoinGameTypes )
		{
			ui_joinGameType.integer = 0;
		}

		trap_Cvar_Set ( "ui_joinGameType", va ( "%d", ui_joinGameType.integer ) );
		UI_BuildServerDisplayList ( qtrue );
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_Skill_HandleKey ( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		int i = trap_Cvar_VariableValue ( "g_spSkill" );

		if ( key == K_MOUSE2 )
		{
			i--;
		}
		else
		{
			i++;
		}

		if ( i < 1 )
		{
			i = numSkillLevels;
		}
		else if ( i > numSkillLevels )
		{
			i = 1;
		}

		trap_Cvar_Set ( "g_spSkill", va ( "%i", i ) );
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_TeamName_HandleKey ( int flags, float *special, int key, qboolean blue )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		int i;
		i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( ( blue ) ? "ui_blueTeam" : "ui_redTeam" ) );

		if ( key == K_MOUSE2 )
		{
			i--;
		}
		else
		{
			i++;
		}

		if ( i >= uiInfo.teamCount )
		{
			i = 0;
		}
		else if ( i < 0 )
		{
			i = uiInfo.teamCount - 1;
		}

		trap_Cvar_Set ( ( blue ) ? "ui_blueTeam" : "ui_redTeam", uiInfo.teamList[ i ].teamName );

		return qtrue;
	}

	return qfalse;
}

static qboolean UI_TeamMember_HandleKey ( int flags, float *special, int key, qboolean blue, int num )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		// 0 - None
		// 1 - Human
		// 2..NumCharacters - Bot
		char *cvar = va ( blue ? "ui_blueteam%i" : "ui_redteam%i", num );
		int  value = trap_Cvar_VariableValue ( cvar );

		if ( key == K_MOUSE2 )
		{
			value--;
		}
		else
		{
			value++;
		}

		if ( value >= UI_GetNumBots() + 2 )
		{
			value = 0;
		}
		else if ( value < 0 )
		{
			value = UI_GetNumBots() + 2 - 1;
		}

		trap_Cvar_Set ( cvar, va ( "%i", value ) );
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_NetSource_HandleKey ( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		if ( key == K_MOUSE2 )
		{
			ui_netSource.integer--;
		}
		else
		{
			ui_netSource.integer++;
		}

		if ( ui_netSource.integer >= numNetSources )
		{
			ui_netSource.integer = 0;
		}
		else if ( ui_netSource.integer < 0 )
		{
			ui_netSource.integer = numNetSources - 1;
		}

		UI_BuildServerDisplayList ( qtrue );

		if ( ui_netSource.integer != AS_GLOBAL )
		{
			UI_StartServerRefresh ( qtrue );
		}

		trap_Cvar_Set ( "ui_netSource", va ( "%d", ui_netSource.integer ) );
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_NetFilter_HandleKey ( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		if ( key == K_MOUSE2 )
		{
			ui_serverFilterType.integer--;
		}
		else
		{
			ui_serverFilterType.integer++;
		}

		if ( ui_serverFilterType.integer >= numServerFilters )
		{
			ui_serverFilterType.integer = 0;
		}
		else if ( ui_serverFilterType.integer < 0 )
		{
			ui_serverFilterType.integer = numServerFilters - 1;
		}

		UI_BuildServerDisplayList ( qtrue );
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_OpponentName_HandleKey ( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		if ( key == K_MOUSE2 )
		{
			UI_PriorOpponent();
		}
		else
		{
			UI_NextOpponent();
		}

		return qtrue;
	}

	return qfalse;
}

static qboolean UI_BotName_HandleKey ( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		int value = uiInfo.botIndex;

		if ( key == K_MOUSE2 )
		{
			value--;
		}
		else
		{
			value++;
		}

		if ( value >= UI_GetNumBots() + 2 )
		{
			value = 0;
		}
		else if ( value < 0 )
		{
			value = UI_GetNumBots() + 2 - 1;
		}

		uiInfo.botIndex = value;
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_BotSkill_HandleKey ( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		if ( key == K_MOUSE2 )
		{
			uiInfo.skillIndex--;
		}
		else
		{
			uiInfo.skillIndex++;
		}

		if ( uiInfo.skillIndex >= numSkillLevels )
		{
			uiInfo.skillIndex = 0;
		}
		else if ( uiInfo.skillIndex < 0 )
		{
			uiInfo.skillIndex = numSkillLevels - 1;
		}

		return qtrue;
	}

	return qfalse;
}

static qboolean UI_RedBlue_HandleKey ( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		uiInfo.redBlue ^= 1;
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_SelectedPlayer_HandleKey ( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		int selected;

		UI_BuildPlayerList();

		if ( !uiInfo.teamLeader )
		{
			return qfalse;
		}

		selected = trap_Cvar_VariableValue ( "cg_selectedPlayer" );

		if ( key == K_MOUSE2 )
		{
			selected--;
		}
		else
		{
			selected++;
		}

		if ( selected > uiInfo.myTeamCount )
		{
			selected = 0;
		}
		else if ( selected < 0 )
		{
			selected = uiInfo.myTeamCount;
		}

		if ( selected == uiInfo.myTeamCount )
		{
			trap_Cvar_Set ( "cg_selectedPlayerName", "Everyone" );
		}
		else
		{
			trap_Cvar_Set ( "cg_selectedPlayerName", uiInfo.teamNames[ selected ] );
		}

		trap_Cvar_Set ( "cg_selectedPlayer", va ( "%d", selected ) );
	}

	return qfalse;
}

static qboolean UI_OwnerDrawHandleKey ( int ownerDraw, int flags, float *special, int key )
{
	switch ( ownerDraw )
	{
		case UI_HANDICAP:
			return UI_Handicap_HandleKey ( flags, special, key );
			break;

		case UI_CLANNAME:
			return UI_ClanName_HandleKey ( flags, special, key );
			break;

		case UI_GAMETYPE:
			return UI_GameType_HandleKey ( flags, special, key, qtrue );
			break;

		case UI_NETGAMETYPE:
			return UI_NetGameType_HandleKey ( flags, special, key );
			break;

		case UI_JOINGAMETYPE:
			return UI_JoinGameType_HandleKey ( flags, special, key );
			break;

		case UI_SKILL:
			return UI_Skill_HandleKey ( flags, special, key );
			break;

		case UI_BLUETEAMNAME:
			return UI_TeamName_HandleKey ( flags, special, key, qtrue );
			break;

		case UI_REDTEAMNAME:
			return UI_TeamName_HandleKey ( flags, special, key, qfalse );
			break;

		case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
			UI_TeamMember_HandleKey ( flags, special, key, qtrue, ownerDraw - UI_BLUETEAM1 + 1 );
			break;

		case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
			UI_TeamMember_HandleKey ( flags, special, key, qfalse, ownerDraw - UI_REDTEAM1 + 1 );
			break;

		case UI_NETSOURCE:
			UI_NetSource_HandleKey ( flags, special, key );
			break;

		case UI_NETFILTER:
			UI_NetFilter_HandleKey ( flags, special, key );
			break;

		case UI_OPPONENT_NAME:
			UI_OpponentName_HandleKey ( flags, special, key );
			break;

		case UI_BOTNAME:
			return UI_BotName_HandleKey ( flags, special, key );
			break;

		case UI_BOTSKILL:
			return UI_BotSkill_HandleKey ( flags, special, key );
			break;

		case UI_REDBLUE:
			UI_RedBlue_HandleKey ( flags, special, key );
			break;

		case UI_SELECTEDPLAYER:
			UI_SelectedPlayer_HandleKey ( flags, special, key );
			break;

		default:
			break;
	}

	return qfalse;
}

static float UI_GetValue ( int ownerDraw )
{
	return 0;
}

/*
=================
UI_ServersQsortCompare
=================
*/
static int QDECL UI_ServersQsortCompare ( const void *arg1, const void *arg2 )
{
	return trap_LAN_CompareServers ( ui_netSource.integer, uiInfo.serverStatus.sortKey, uiInfo.serverStatus.sortDir, * ( int * ) arg1, * ( int * ) arg2 );
}

/*
=================
UI_ServersSort
=================
*/
void UI_ServersSort ( int column, qboolean force )
{
	if ( !force )
	{
		if ( uiInfo.serverStatus.sortKey == column )
		{
			return;
		}
	}

	uiInfo.serverStatus.sortKey = column;
	qsort ( &uiInfo.serverStatus.displayServers[ 0 ], uiInfo.serverStatus.numDisplayServers, sizeof ( int ), UI_ServersQsortCompare );
}

/*
===============
UI_GetCurrentAlienStage
===============
*/
static stage_t UI_GetCurrentAlienStage ( void )
{
	char    buffer[ MAX_TOKEN_CHARS ];
	stage_t stage, dummy;

	trap_Cvar_VariableStringBuffer ( "ui_stages", buffer, sizeof ( buffer ) );
	sscanf ( buffer, "%d %d", ( int * ) &stage, ( int * ) &dummy );

	return stage;
}

/*
===============
UI_GetCurrentHumanStage
===============
*/
static stage_t UI_GetCurrentHumanStage ( void )
{
	char    buffer[ MAX_TOKEN_CHARS ];
	stage_t stage, dummy;

	trap_Cvar_VariableStringBuffer ( "ui_stages", buffer, sizeof ( buffer ) );
	sscanf ( buffer, "%d %d", ( int * ) &dummy, ( int * ) &stage );

	return stage;
}

/*
===============
UI_LoadTremTeams
===============
*/
static void UI_LoadTremTeams ( void )
{
	uiInfo.tremTeamCount = 4;

	uiInfo.tremTeamList[ 0 ].text = String_Alloc ( "Aliens" );
	uiInfo.tremTeamList[ 0 ].cmd = String_Alloc ( "cmd team aliens\n" );
	uiInfo.tremTeamList[ 0 ].infopane = UI_FindInfoPaneByName ( "alienteam" );

	uiInfo.tremTeamList[ 1 ].text = String_Alloc ( "Humans" );
	uiInfo.tremTeamList[ 1 ].cmd = String_Alloc ( "cmd team humans\n" );
	uiInfo.tremTeamList[ 1 ].infopane = UI_FindInfoPaneByName ( "humanteam" );

	uiInfo.tremTeamList[ 2 ].text = String_Alloc ( "Spectate" );
	uiInfo.tremTeamList[ 2 ].cmd = String_Alloc ( "cmd team spectate\n" );
	uiInfo.tremTeamList[ 2 ].infopane = UI_FindInfoPaneByName ( "spectateteam" );

	uiInfo.tremTeamList[ 3 ].text = String_Alloc ( "Auto select" );
	uiInfo.tremTeamList[ 3 ].cmd = String_Alloc ( "cmd team auto\n" );
	uiInfo.tremTeamList[ 3 ].infopane = UI_FindInfoPaneByName ( "autoteam" );
}

/*
===============
UI_AddClass
===============
*/
static void UI_AddClass ( pClass_t class )
{
	uiInfo.tremAlienClassList[ uiInfo.tremAlienClassCount ].text =
	  String_Alloc ( BG_FindHumanNameForClassNum ( class ) );
	uiInfo.tremAlienClassList[ uiInfo.tremAlienClassCount ].cmd =
	  String_Alloc ( va ( "cmd class %s\n", BG_FindNameForClassNum ( class ) ) );
	uiInfo.tremAlienClassList[ uiInfo.tremAlienClassCount ].infopane =
	  UI_FindInfoPaneByName ( va ( "%sclass", BG_FindNameForClassNum ( class ) ) );

	uiInfo.tremAlienClassCount++;
}

/*
===============
UI_LoadTremAlienClasses
===============
*/
static void UI_LoadTremAlienClasses ( void )
{
	uiInfo.tremAlienClassCount = 0;

	if ( BG_ClassIsAllowed ( PCL_ALIEN_LEVEL0 ) )
	{
		UI_AddClass ( PCL_ALIEN_LEVEL0 );
	}

	if ( BG_ClassIsAllowed ( PCL_ALIEN_BUILDER0_UPG ) &&
	     BG_FindStagesForClass ( PCL_ALIEN_BUILDER0_UPG, UI_GetCurrentAlienStage() ) )
	{
		UI_AddClass ( PCL_ALIEN_BUILDER0_UPG );
	}
	else if ( BG_ClassIsAllowed ( PCL_ALIEN_BUILDER0 ) )
	{
		UI_AddClass ( PCL_ALIEN_BUILDER0 );
	}
}

/*
===============
UI_AddItem
===============
*/
static void UI_AddItem ( weapon_t weapon )
{
	uiInfo.tremHumanItemList[ uiInfo.tremHumanItemCount ].text =
	  String_Alloc ( BG_FindHumanNameForWeapon ( weapon ) );
	uiInfo.tremHumanItemList[ uiInfo.tremHumanItemCount ].cmd =
	  String_Alloc ( va ( "cmd class %s\n", BG_FindNameForWeapon ( weapon ) ) );
	uiInfo.tremHumanItemList[ uiInfo.tremHumanItemCount ].infopane =
	  UI_FindInfoPaneByName ( va ( "%sitem", BG_FindNameForWeapon ( weapon ) ) );

	uiInfo.tremHumanItemCount++;
}

/*
===============
UI_LoadTremHumanItems
===============
*/
static void UI_LoadTremHumanItems ( void )
{
	uiInfo.tremHumanItemCount = 0;

	if ( BG_WeaponIsAllowed ( WP_MACHINEGUN ) )
	{
		UI_AddItem ( WP_MACHINEGUN );
	}

	if ( BG_WeaponIsAllowed ( WP_HBUILD2 ) &&
	     BG_FindStagesForWeapon ( WP_HBUILD2, UI_GetCurrentHumanStage() ) )
	{
		UI_AddItem ( WP_HBUILD2 );
	}
	else if ( BG_WeaponIsAllowed ( WP_HBUILD ) )
	{
		UI_AddItem ( WP_HBUILD );
	}
}

/*
===============
UI_ParseCarriageList
===============
*/
static void UI_ParseCarriageList ( int *weapons, int *upgrades )
{
	int  i;
	char carriageCvar[ MAX_TOKEN_CHARS ];
	char *iterator;
	char buffer[ MAX_TOKEN_CHARS ];
	char *bufPointer;

	trap_Cvar_VariableStringBuffer ( "ui_carriage", carriageCvar, sizeof ( carriageCvar ) );
	iterator = carriageCvar;

	if ( weapons )
	{
		*weapons = 0;
	}

	if ( upgrades )
	{
		*upgrades = 0;
	}

	//simple parser to give rise to weapon/upgrade list
	while ( iterator && iterator[ 0 ] != '$' )
	{
		bufPointer = buffer;

		if ( iterator[ 0 ] == 'W' )
		{
			iterator++;

			while ( iterator[ 0 ] != ' ' )
			{
				*bufPointer++ = *iterator++;
			}

			*bufPointer++ = '\n';

			i = atoi ( buffer );

			if ( weapons )
			{
				*weapons |= ( 1 << i );
			}
		}
		else if ( iterator[ 0 ] == 'U' )
		{
			iterator++;

			while ( iterator[ 0 ] != ' ' )
			{
				*bufPointer++ = *iterator++;
			}

			*bufPointer++ = '\n';

			i = atoi ( buffer );

			if ( upgrades )
			{
				*upgrades |= ( 1 << i );
			}
		}

		iterator++;
	}
}

/*
===============
UI_LoadTremHumanArmouryBuys
===============
*/
static void UI_LoadTremHumanArmouryBuys ( void )
{
	int     i, j = 0;
	stage_t stage = UI_GetCurrentHumanStage();
	int     weapons, upgrades;
	int     slots = 0;

	UI_ParseCarriageList ( &weapons, &upgrades );

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		if ( weapons & ( 1 << i ) )
		{
			slots |= BG_FindSlotsForWeapon ( i );
		}
	}

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( upgrades & ( 1 << i ) )
		{
			slots |= BG_FindSlotsForUpgrade ( i );
		}
	}

	uiInfo.tremHumanArmouryBuyCount = 0;

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		if ( BG_FindTeamForWeapon ( i ) == WUT_HUMANS &&
		     BG_FindPurchasableForWeapon ( i ) &&
		     BG_FindStagesForWeapon ( i, stage ) &&
		     BG_WeaponIsAllowed ( i ) &&
		     ! ( BG_FindSlotsForWeapon ( i ) & slots ) &&
		     ! ( weapons & ( 1 << i ) ) )
		{
			uiInfo.tremHumanArmouryBuyList[ j ].text =
			  String_Alloc ( BG_FindHumanNameForWeapon ( i ) );
			uiInfo.tremHumanArmouryBuyList[ j ].cmd =
			  String_Alloc ( va ( "cmd buy %s retrigger\n", BG_FindNameForWeapon ( i ) ) );
			uiInfo.tremHumanArmouryBuyList[ j ].infopane =
			  UI_FindInfoPaneByName ( va ( "%sitem", BG_FindNameForWeapon ( i ) ) );

			j++;

			uiInfo.tremHumanArmouryBuyCount++;
		}
	}

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( BG_FindTeamForUpgrade ( i ) == WUT_HUMANS &&
		     BG_FindPurchasableForUpgrade ( i ) &&
		     BG_FindStagesForUpgrade ( i, stage ) &&
		     BG_UpgradeIsAllowed ( i ) &&
		     ! ( BG_FindSlotsForUpgrade ( i ) & slots ) &&
		     ! ( upgrades & ( 1 << i ) ) )
		{
			uiInfo.tremHumanArmouryBuyList[ j ].text =
			  String_Alloc ( BG_FindHumanNameForUpgrade ( i ) );
			uiInfo.tremHumanArmouryBuyList[ j ].cmd =
			  String_Alloc ( va ( "cmd buy %s retrigger\n", BG_FindNameForUpgrade ( i ) ) );
			uiInfo.tremHumanArmouryBuyList[ j ].infopane =
			  UI_FindInfoPaneByName ( va ( "%sitem", BG_FindNameForUpgrade ( i ) ) );

			j++;

			uiInfo.tremHumanArmouryBuyCount++;
		}
	}
}

/*
===============
UI_LoadTremHumanArmourySells
===============
*/
static void UI_LoadTremHumanArmourySells ( void )
{
	int weapons, upgrades;
	int i, j = 0;

	uiInfo.tremHumanArmourySellCount = 0;
	UI_ParseCarriageList ( &weapons, &upgrades );

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		if ( weapons & ( 1 << i ) )
		{
			uiInfo.tremHumanArmourySellList[ j ].text = String_Alloc ( BG_FindHumanNameForWeapon ( i ) );
			uiInfo.tremHumanArmourySellList[ j ].cmd =
			  String_Alloc ( va ( "cmd sell %s retrigger\n", BG_FindNameForWeapon ( i ) ) );
			uiInfo.tremHumanArmourySellList[ j ].infopane =
			  UI_FindInfoPaneByName ( va ( "%sitem", BG_FindNameForWeapon ( i ) ) );

			j++;

			uiInfo.tremHumanArmourySellCount++;
		}
	}

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( upgrades & ( 1 << i ) )
		{
			uiInfo.tremHumanArmourySellList[ j ].text = String_Alloc ( BG_FindHumanNameForUpgrade ( i ) );
			uiInfo.tremHumanArmourySellList[ j ].cmd =
			  String_Alloc ( va ( "cmd sell %s retrigger\n", BG_FindNameForUpgrade ( i ) ) );
			uiInfo.tremHumanArmourySellList[ j ].infopane =
			  UI_FindInfoPaneByName ( va ( "%sitem", BG_FindNameForUpgrade ( i ) ) );

			j++;

			uiInfo.tremHumanArmourySellCount++;
		}
	}
}

/*
===============
UI_LoadTremAlienUpgrades
===============
*/
static void UI_LoadTremAlienUpgrades ( void )
{
	int     i, j = 0;
	int     class, credits;
	char    ui_currentClass[ MAX_STRING_CHARS ];
	stage_t stage = UI_GetCurrentAlienStage();

	trap_Cvar_VariableStringBuffer ( "ui_currentClass", ui_currentClass, MAX_STRING_CHARS );
	sscanf ( ui_currentClass, "%d %d", &class, &credits );

	uiInfo.tremAlienUpgradeCount = 0;

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		if ( BG_ClassCanEvolveFromTo ( class, i, credits, 0 ) >= 0 &&
		     BG_FindStagesForClass ( i, stage ) &&
		     BG_ClassIsAllowed ( i ) )
		{
			uiInfo.tremAlienUpgradeList[ j ].text = String_Alloc ( BG_FindHumanNameForClassNum ( i ) );
			uiInfo.tremAlienUpgradeList[ j ].cmd =
			  String_Alloc ( va ( "cmd class %s\n", BG_FindNameForClassNum ( i ) ) );
			uiInfo.tremAlienUpgradeList[ j ].infopane =
			  UI_FindInfoPaneByName ( va ( "%sclass", BG_FindNameForClassNum ( i ) ) );

			j++;

			uiInfo.tremAlienUpgradeCount++;
		}
	}
}

/*
===============
UI_LoadTremAlienBuilds
===============
*/
static void UI_LoadTremAlienBuilds ( void )
{
	int     weapons;
	int     i, j = 0;
	stage_t stage;

	UI_ParseCarriageList ( &weapons, NULL );
	stage = UI_GetCurrentAlienStage();

	uiInfo.tremAlienBuildCount = 0;

	for ( i = BA_NONE + 1; i < BA_NUM_BUILDABLES; i++ )
	{
		if ( BG_FindTeamForBuildable ( i ) == BIT_ALIENS &&
		     BG_FindBuildWeaponForBuildable ( i ) & weapons &&
		     BG_FindStagesForBuildable ( i, stage ) &&
		     BG_BuildableIsAllowed ( i ) )
		{
			uiInfo.tremAlienBuildList[ j ].text =
			  String_Alloc ( BG_FindHumanNameForBuildable ( i ) );
			uiInfo.tremAlienBuildList[ j ].cmd =
			  String_Alloc ( va ( "cmd build %s\n", BG_FindNameForBuildable ( i ) ) );
			uiInfo.tremAlienBuildList[ j ].infopane =
			  UI_FindInfoPaneByName ( va ( "%sbuild", BG_FindNameForBuildable ( i ) ) );

			j++;

			uiInfo.tremAlienBuildCount++;
		}
	}
}

/*
===============
UI_LoadTremHumanBuilds
===============
*/
static void UI_LoadTremHumanBuilds ( void )
{
	int     weapons;
	int     i, j = 0;
	stage_t stage;

	UI_ParseCarriageList ( &weapons, NULL );
	stage = UI_GetCurrentHumanStage();

	uiInfo.tremHumanBuildCount = 0;

	for ( i = BA_NONE + 1; i < BA_NUM_BUILDABLES; i++ )
	{
		if ( BG_FindTeamForBuildable ( i ) == BIT_HUMANS &&
		     BG_FindBuildWeaponForBuildable ( i ) & weapons &&
		     BG_FindStagesForBuildable ( i, stage ) &&
		     BG_BuildableIsAllowed ( i ) )
		{
			uiInfo.tremHumanBuildList[ j ].text =
			  String_Alloc ( BG_FindHumanNameForBuildable ( i ) );
			uiInfo.tremHumanBuildList[ j ].cmd =
			  String_Alloc ( va ( "cmd build %s\n", BG_FindNameForBuildable ( i ) ) );
			uiInfo.tremHumanBuildList[ j ].infopane =
			  UI_FindInfoPaneByName ( va ( "%sbuild", BG_FindNameForBuildable ( i ) ) );

			j++;

			uiInfo.tremHumanBuildCount++;
		}
	}
}

/*
===============
UI_LoadMods
===============
*/
static void UI_LoadMods ( void )
{
	int  numdirs;
	char dirlist[ 2048 ];
	char *dirptr;
	char *descptr;
	int  i;
	int  dirlen;

	uiInfo.modCount = 0;
	numdirs = trap_FS_GetFileList ( "$modlist", "", dirlist, sizeof ( dirlist ) );
	dirptr = dirlist;

	for ( i = 0; i < numdirs; i++ )
	{
		dirlen = strlen ( dirptr ) + 1;
		descptr = dirptr + dirlen;
		uiInfo.modList[ uiInfo.modCount ].modName = String_Alloc ( dirptr );
		uiInfo.modList[ uiInfo.modCount ].modDescr = String_Alloc ( descptr );
		dirptr += dirlen + strlen ( descptr ) + 1;
		uiInfo.modCount++;

		if ( uiInfo.modCount >= MAX_MODS )
		{
			break;
		}
	}
}

/*
===============
UI_LoadMovies
===============
*/
static void UI_LoadMovies ( void )
{
	char movielist[ 4096 ];
	char *moviename;
	int  i, len;

	uiInfo.movieCount = trap_FS_GetFileList ( "video", "roq", movielist, 4096 );

	if ( uiInfo.movieCount )
	{
		if ( uiInfo.movieCount > MAX_MOVIES )
		{
			uiInfo.movieCount = MAX_MOVIES;
		}

		moviename = movielist;

		for ( i = 0; i < uiInfo.movieCount; i++ )
		{
			len = strlen ( moviename );

			if ( !Q_stricmp ( moviename +  len - 4, ".roq" ) )
			{
				moviename[ len - 4 ] = '\0';
			}

			Q_strupr ( moviename );
			uiInfo.movieList[ i ] = String_Alloc ( moviename );
			moviename += len + 1;
		}
	}
}

/*
===============
UI_LoadDemos
===============
*/
static void UI_LoadDemos ( void )
{
	char demolist[ 4096 ];
	char demoExt[ 32 ];
	char *demoname;
	int  i, len;

	Com_sprintf ( demoExt, sizeof ( demoExt ), "dm_%d", ( int ) trap_Cvar_VariableValue ( "protocol" ) );

	uiInfo.demoCount = trap_FS_GetFileList ( "demos", demoExt, demolist, 4096 );

	Com_sprintf ( demoExt, sizeof ( demoExt ), ".dm_%d", ( int ) trap_Cvar_VariableValue ( "protocol" ) );

	if ( uiInfo.demoCount )
	{
		if ( uiInfo.demoCount > MAX_DEMOS )
		{
			uiInfo.demoCount = MAX_DEMOS;
		}

		demoname = demolist;

		for ( i = 0; i < uiInfo.demoCount; i++ )
		{
			len = strlen ( demoname );

			if ( !Q_stricmp ( demoname +  len - strlen ( demoExt ), demoExt ) )
			{
				demoname[ len - strlen ( demoExt ) ] = '\0';
			}

			Q_strupr ( demoname );
			uiInfo.demoList[ i ] = String_Alloc ( demoname );
			demoname += len + 1;
		}
	}
}

static qboolean UI_SetNextMap ( int actual, int index )
{
	int i;

	for ( i = actual + 1; i < uiInfo.mapCount; i++ )
	{
		if ( uiInfo.mapList[ i ].active )
		{
			Menu_SetFeederSelection ( NULL, FEEDER_MAPS, index + 1, "skirmish" );
			return qtrue;
		}
	}

	return qfalse;
}

static void UI_StartSkirmish ( qboolean next )
{
	int   i, k, g, delay, temp;
	float skill;
	char  buff[ MAX_STRING_CHARS ];

	if ( next )
	{
		int actual;
		int index = trap_Cvar_VariableValue ( "ui_mapIndex" );
		UI_MapCountByGameType ( qtrue );
		UI_SelectedMap ( index, &actual );

		if ( UI_SetNextMap ( actual, index ) )
		{
		}
		else
		{
			UI_GameType_HandleKey ( 0, NULL, K_MOUSE1, qfalse );
			UI_MapCountByGameType ( qtrue );
			Menu_SetFeederSelection ( NULL, FEEDER_MAPS, 0, "skirmish" );
		}
	}

	g = uiInfo.gameTypes[ ui_gameType.integer ].gtEnum;
	trap_Cvar_SetValue ( "g_gametype", g );
	trap_Cmd_ExecuteText ( EXEC_APPEND, va ( "wait ; wait ; map %s\n", uiInfo.mapList[ ui_currentMap.integer ].mapLoadName ) );
	skill = trap_Cvar_VariableValue ( "g_spSkill" );
	trap_Cvar_Set ( "ui_scoreMap", uiInfo.mapList[ ui_currentMap.integer ].mapName );

	k = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_opponentName" ) );

	trap_Cvar_Set ( "ui_singlePlayerActive", "1" );

	// set up sp overrides, will be replaced on postgame
	temp = trap_Cvar_VariableValue ( "capturelimit" );
	trap_Cvar_Set ( "ui_saveCaptureLimit", va ( "%i", temp ) );
	temp = trap_Cvar_VariableValue ( "fraglimit" );
	trap_Cvar_Set ( "ui_saveFragLimit", va ( "%i", temp ) );

	UI_SetCapFragLimits ( qfalse );

	temp = trap_Cvar_VariableValue ( "cg_drawTimer" );
	trap_Cvar_Set ( "ui_drawTimer", va ( "%i", temp ) );
	temp = trap_Cvar_VariableValue ( "g_doWarmup" );
	trap_Cvar_Set ( "ui_doWarmup", va ( "%i", temp ) );
	temp = trap_Cvar_VariableValue ( "g_friendlyFire" );
	trap_Cvar_Set ( "ui_friendlyFire", va ( "%i", temp ) );
	temp = trap_Cvar_VariableValue ( "sv_maxClients" );
	trap_Cvar_Set ( "ui_maxClients", va ( "%i", temp ) );
	temp = trap_Cvar_VariableValue ( "g_warmup" );
	trap_Cvar_Set ( "ui_Warmup", va ( "%i", temp ) );
	temp = trap_Cvar_VariableValue ( "sv_pure" );
	trap_Cvar_Set ( "ui_pure", va ( "%i", temp ) );

	trap_Cvar_Set ( "cg_cameraOrbit", "0" );
	trap_Cvar_Set ( "cg_thirdPerson", "0" );
	trap_Cvar_Set ( "cg_drawTimer", "1" );
	trap_Cvar_Set ( "g_doWarmup", "1" );
	trap_Cvar_Set ( "g_warmup", "15" );
	trap_Cvar_Set ( "sv_pure", "0" );
	trap_Cvar_Set ( "g_friendlyFire", "0" );
	trap_Cvar_Set ( "g_redTeam", UI_Cvar_VariableString ( "ui_teamName" ) );
	trap_Cvar_Set ( "g_blueTeam", UI_Cvar_VariableString ( "ui_opponentName" ) );

	if ( trap_Cvar_VariableValue ( "ui_recordSPDemo" ) )
	{
		Com_sprintf ( buff, MAX_STRING_CHARS, "%s_%i", uiInfo.mapList[ ui_currentMap.integer ].mapLoadName, g );
		trap_Cvar_Set ( "ui_recordSPDemoName", buff );
	}

	delay = 500;

	{
		temp = uiInfo.mapList[ ui_currentMap.integer ].teamMembers * 2;
		trap_Cvar_Set ( "sv_maxClients", va ( "%d", temp ) );

		for ( i = 0; i < uiInfo.mapList[ ui_currentMap.integer ].teamMembers; i++ )
		{
			Com_sprintf ( buff, sizeof ( buff ), "addbot %s %f %s %i %s\n", UI_AIFromName ( uiInfo.teamList[ k ].teamMembers[ i ] ), skill, "", delay, uiInfo.teamList[ k ].teamMembers[ i ] );
			trap_Cmd_ExecuteText ( EXEC_APPEND, buff );
			delay += 500;
		}

		k = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_teamName" ) );

		for ( i = 0; i < uiInfo.mapList[ ui_currentMap.integer ].teamMembers - 1; i++ )
		{
			Com_sprintf ( buff, sizeof ( buff ), "addbot %s %f %s %i %s\n", UI_AIFromName ( uiInfo.teamList[ k ].teamMembers[ i ] ), skill, "", delay, uiInfo.teamList[ k ].teamMembers[ i ] );
			trap_Cmd_ExecuteText ( EXEC_APPEND, buff );
			delay += 500;
		}
	}
}

static void UI_Update ( const char *name )
{
	int val = trap_Cvar_VariableValue ( name );

	if ( Q_stricmp ( name, "ui_SetName" ) == 0 )
	{
		trap_Cvar_Set ( "name", UI_Cvar_VariableString ( "ui_Name" ) );
	}
	else if ( Q_stricmp ( name, "ui_setRate" ) == 0 )
	{
		float rate = trap_Cvar_VariableValue ( "rate" );

		if ( rate >= 5000 )
		{
			trap_Cvar_Set ( "cl_maxpackets", "30" );
			trap_Cvar_Set ( "cl_packetdup", "1" );
		}
		else if ( rate >= 4000 )
		{
			trap_Cvar_Set ( "cl_maxpackets", "15" );
			trap_Cvar_Set ( "cl_packetdup", "2" ); // favor less prediction errors when there's packet loss
		}
		else
		{
			trap_Cvar_Set ( "cl_maxpackets", "15" );
			trap_Cvar_Set ( "cl_packetdup", "1" ); // favor lower bandwidth
		}
	}
	else if ( Q_stricmp ( name, "ui_GetName" ) == 0 )
	{
		trap_Cvar_Set ( "ui_Name", UI_Cvar_VariableString ( "name" ) );
	}
	else if ( Q_stricmp ( name, "r_colorbits" ) == 0 )
	{
		switch ( val )
		{
			case 0:
				trap_Cvar_SetValue ( "r_depthbits", 0 );
				trap_Cvar_SetValue ( "r_stencilbits", 0 );
				break;

			case 16:
				trap_Cvar_SetValue ( "r_depthbits", 16 );
				trap_Cvar_SetValue ( "r_stencilbits", 0 );
				break;

			case 32:
				trap_Cvar_SetValue ( "r_depthbits", 24 );
				break;
		}
	}
	else if ( Q_stricmp ( name, "r_lodbias" ) == 0 )
	{
		switch ( val )
		{
			case 0:
				trap_Cvar_SetValue ( "r_subdivisions", 4 );
				break;

			case 1:
				trap_Cvar_SetValue ( "r_subdivisions", 12 );
				break;

			case 2:
				trap_Cvar_SetValue ( "r_subdivisions", 20 );
				break;
		}
	}
	else if ( Q_stricmp ( name, "ui_glCustom" ) == 0 )
	{
		switch ( val )
		{
			case 0: // high quality
				trap_Cvar_SetValue ( "r_fullScreen", 1 );
				trap_Cvar_SetValue ( "r_subdivisions", 4 );
				trap_Cvar_SetValue ( "r_vertexlight", 0 );
				trap_Cvar_SetValue ( "r_lodbias", 0 );
				trap_Cvar_SetValue ( "r_colorbits", 32 );
				trap_Cvar_SetValue ( "r_depthbits", 24 );
				trap_Cvar_SetValue ( "r_picmip", 0 );
				trap_Cvar_SetValue ( "r_mode", 4 );
				trap_Cvar_SetValue ( "r_texturebits", 32 );
				trap_Cvar_SetValue ( "r_fastSky", 0 );
				trap_Cvar_SetValue ( "r_inGameVideo", 1 );
				trap_Cvar_SetValue ( "cg_shadows", 1 );
				trap_Cvar_SetValue ( "cg_brassTime", 2500 );
				trap_Cvar_Set ( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
				break;

			case 1: // normal
				trap_Cvar_SetValue ( "r_fullScreen", 1 );
				trap_Cvar_SetValue ( "r_subdivisions", 12 );
				trap_Cvar_SetValue ( "r_vertexlight", 0 );
				trap_Cvar_SetValue ( "r_lodbias", 0 );
				trap_Cvar_SetValue ( "r_colorbits", 0 );
				trap_Cvar_SetValue ( "r_depthbits", 24 );
				trap_Cvar_SetValue ( "r_picmip", 1 );
				trap_Cvar_SetValue ( "r_mode", 3 );
				trap_Cvar_SetValue ( "r_texturebits", 0 );
				trap_Cvar_SetValue ( "r_fastSky", 0 );
				trap_Cvar_SetValue ( "r_inGameVideo", 1 );
				trap_Cvar_SetValue ( "cg_brassTime", 2500 );
				trap_Cvar_Set ( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
				trap_Cvar_SetValue ( "cg_shadows", 0 );
				break;

			case 2: // fast
				trap_Cvar_SetValue ( "r_fullScreen", 1 );
				trap_Cvar_SetValue ( "r_subdivisions", 8 );
				trap_Cvar_SetValue ( "r_vertexlight", 0 );
				trap_Cvar_SetValue ( "r_lodbias", 1 );
				trap_Cvar_SetValue ( "r_colorbits", 0 );
				trap_Cvar_SetValue ( "r_depthbits", 0 );
				trap_Cvar_SetValue ( "r_picmip", 1 );
				trap_Cvar_SetValue ( "r_mode", 3 );
				trap_Cvar_SetValue ( "r_texturebits", 0 );
				trap_Cvar_SetValue ( "cg_shadows", 0 );
				trap_Cvar_SetValue ( "r_fastSky", 1 );
				trap_Cvar_SetValue ( "r_inGameVideo", 0 );
				trap_Cvar_SetValue ( "cg_brassTime", 0 );
				trap_Cvar_Set ( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
				break;

			case 3: // fastest
				trap_Cvar_SetValue ( "r_fullScreen", 1 );
				trap_Cvar_SetValue ( "r_subdivisions", 20 );
				trap_Cvar_SetValue ( "r_vertexlight", 1 );
				trap_Cvar_SetValue ( "r_lodbias", 2 );
				trap_Cvar_SetValue ( "r_colorbits", 16 );
				trap_Cvar_SetValue ( "r_depthbits", 16 );
				trap_Cvar_SetValue ( "r_mode", 3 );
				trap_Cvar_SetValue ( "r_picmip", 2 );
				trap_Cvar_SetValue ( "r_texturebits", 16 );
				trap_Cvar_SetValue ( "cg_shadows", 0 );
				trap_Cvar_SetValue ( "cg_brassTime", 0 );
				trap_Cvar_SetValue ( "r_fastSky", 1 );
				trap_Cvar_SetValue ( "r_inGameVideo", 0 );
				trap_Cvar_Set ( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
				break;
		}
	}
	else if ( Q_stricmp ( name, "ui_mousePitch" ) == 0 )
	{
		if ( val == 0 )
		{
			trap_Cvar_SetValue ( "m_pitch", 0.022f );
		}
		else
		{
			trap_Cvar_SetValue ( "m_pitch", -0.022f );
		}
	}
}

static void UI_RunMenuScript ( char **args )
{
	const char *name, *name2;
	char       buff[ 1024 ];
	const char *cmd;

	if ( String_Parse ( args, &name ) )
	{
		if ( Q_stricmp ( name, "StartServer" ) == 0 )
		{
			int   i, clients, oldclients;
			float skill;
			trap_Cvar_Set ( "cg_thirdPerson", "0" );
			trap_Cvar_Set ( "cg_cameraOrbit", "0" );
			trap_Cvar_Set ( "ui_singlePlayerActive", "0" );
			trap_Cvar_SetValue ( "dedicated", Com_Clamp ( 0, 2, ui_dedicated.integer ) );
			trap_Cvar_SetValue ( "g_gametype", Com_Clamp ( 0, 8, uiInfo.gameTypes[ ui_netGameType.integer ].gtEnum ) );
			trap_Cvar_Set ( "g_redTeam", UI_Cvar_VariableString ( "ui_teamName" ) );
			trap_Cvar_Set ( "g_blueTeam", UI_Cvar_VariableString ( "ui_opponentName" ) );
			trap_Cmd_ExecuteText ( EXEC_APPEND, va ( "wait ; wait ; map %s\n", uiInfo.mapList[ ui_currentNetMap.integer ].mapLoadName ) );
			skill = trap_Cvar_VariableValue ( "g_spSkill" );
			// set max clients based on spots
			oldclients = trap_Cvar_VariableValue ( "sv_maxClients" );
			clients = 0;

			for ( i = 0; i < PLAYERS_PER_TEAM; i++ )
			{
				int bot = trap_Cvar_VariableValue ( va ( "ui_blueteam%i", i + 1 ) );

				if ( bot >= 0 )
				{
					clients++;
				}

				bot = trap_Cvar_VariableValue ( va ( "ui_redteam%i", i + 1 ) );

				if ( bot >= 0 )
				{
					clients++;
				}
			}

			if ( clients == 0 )
			{
				clients = 8;
			}

			if ( oldclients > clients )
			{
				clients = oldclients;
			}

			trap_Cvar_Set ( "sv_maxClients", va ( "%d", clients ) );

			for ( i = 0; i < PLAYERS_PER_TEAM; i++ )
			{
				int bot = trap_Cvar_VariableValue ( va ( "ui_blueteam%i", i + 1 ) );

				if ( bot > 1 )
				{
					Com_sprintf ( buff, sizeof ( buff ), "addbot %s %f \n", UI_GetBotNameByNumber ( bot - 2 ), skill );
					trap_Cmd_ExecuteText ( EXEC_APPEND, buff );
				}

				bot = trap_Cvar_VariableValue ( va ( "ui_redteam%i", i + 1 ) );

				if ( bot > 1 )
				{
					Com_sprintf ( buff, sizeof ( buff ), "addbot %s %f \n", UI_GetBotNameByNumber ( bot - 2 ), skill );
					trap_Cmd_ExecuteText ( EXEC_APPEND, buff );
				}
			}
		}
		else if ( Q_stricmp ( name, "updateSPMenu" ) == 0 )
		{
			UI_SetCapFragLimits ( qtrue );
			UI_MapCountByGameType ( qtrue );
			ui_mapIndex.integer = UI_GetIndexFromSelection ( ui_currentMap.integer );
			trap_Cvar_Set ( "ui_mapIndex", va ( "%d", ui_mapIndex.integer ) );
			Menu_SetFeederSelection ( NULL, FEEDER_MAPS, ui_mapIndex.integer, "skirmish" );
			UI_GameType_HandleKey ( 0, NULL, K_MOUSE1, qfalse );
			UI_GameType_HandleKey ( 0, NULL, K_MOUSE2, qfalse );
		}
		else if ( Q_stricmp ( name, "resetDefaults" ) == 0 )
		{
			trap_Cmd_ExecuteText ( EXEC_APPEND, "exec default.cfg\n" );
			trap_Cmd_ExecuteText ( EXEC_APPEND, "cvar_restart\n" );
			Controls_SetDefaults();
			trap_Cvar_Set ( "com_introPlayed", "1" );
			trap_Cmd_ExecuteText ( EXEC_APPEND, "vid_restart\n" );
		}
		else if ( Q_stricmp ( name, "loadArenas" ) == 0 )
		{
			UI_LoadArenas();
			UI_MapCountByGameType ( qfalse );
			Menu_SetFeederSelection ( NULL, FEEDER_ALLMAPS, 0, "createserver" );
		}
		else if ( Q_stricmp ( name, "saveControls" ) == 0 )
		{
			Controls_SetConfig ( qtrue );
		}
		else if ( Q_stricmp ( name, "loadControls" ) == 0 )
		{
			Controls_GetConfig();
		}
		else if ( Q_stricmp ( name, "clearError" ) == 0 )
		{
			trap_Cvar_Set ( "com_errorMessage", "" );
		}
		else if ( Q_stricmp ( name, "loadGameInfo" ) == 0 )
		{
			/*      UI_ParseGameInfo("gameinfo.txt");
			      UI_LoadBestScores(uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum);*/
		}
		else if ( Q_stricmp ( name, "resetScores" ) == 0 )
		{
			UI_ClearScores();
		}
		else if ( Q_stricmp ( name, "RefreshServers" ) == 0 )
		{
			UI_StartServerRefresh ( qtrue );
			UI_BuildServerDisplayList ( qtrue );
		}
		else if ( Q_stricmp ( name, "RefreshFilter" ) == 0 )
		{
			UI_StartServerRefresh ( qfalse );
			UI_BuildServerDisplayList ( qtrue );
		}
		else if ( Q_stricmp ( name, "RunSPDemo" ) == 0 )
		{
			if ( uiInfo.demoAvailable )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, va ( "demo %s_%i\n", uiInfo.mapList[ ui_currentMap.integer ].mapLoadName, uiInfo.gameTypes[ ui_gameType.integer ].gtEnum ) );
			}
		}
		else if ( Q_stricmp ( name, "LoadDemos" ) == 0 )
		{
			UI_LoadDemos();
		}
		else if ( Q_stricmp ( name, "LoadMovies" ) == 0 )
		{
			UI_LoadMovies();
		}
		else if ( Q_stricmp ( name, "LoadMods" ) == 0 )
		{
			UI_LoadMods();
		}

//TA: tremulous menus
		else if ( Q_stricmp ( name, "LoadTeams" ) == 0 )
		{
			UI_LoadTremTeams();
		}
		else if ( Q_stricmp ( name, "JoinTeam" ) == 0 )
		{
			if ( ( cmd = uiInfo.tremTeamList[ uiInfo.tremTeamIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp ( name, "LoadHumanItems" ) == 0 )
		{
			UI_LoadTremHumanItems();
		}
		else if ( Q_stricmp ( name, "SpawnWithHumanItem" ) == 0 )
		{
			if ( ( cmd = uiInfo.tremHumanItemList[ uiInfo.tremHumanItemIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp ( name, "LoadAlienClasses" ) == 0 )
		{
			UI_LoadTremAlienClasses();
		}
		else if ( Q_stricmp ( name, "SpawnAsAlienClass" ) == 0 )
		{
			if ( ( cmd = uiInfo.tremAlienClassList[ uiInfo.tremAlienClassIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp ( name, "LoadHumanArmouryBuys" ) == 0 )
		{
			UI_LoadTremHumanArmouryBuys();
		}
		else if ( Q_stricmp ( name, "BuyFromArmoury" ) == 0 )
		{
			if ( ( cmd = uiInfo.tremHumanArmouryBuyList[ uiInfo.tremHumanArmouryBuyIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp ( name, "LoadHumanArmourySells" ) == 0 )
		{
			UI_LoadTremHumanArmourySells();
		}
		else if ( Q_stricmp ( name, "SellToArmoury" ) == 0 )
		{
			if ( ( cmd = uiInfo.tremHumanArmourySellList[ uiInfo.tremHumanArmourySellIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp ( name, "LoadAlienUpgrades" ) == 0 )
		{
			UI_LoadTremAlienUpgrades();

			//disallow the menu if it would be empty
			if ( uiInfo.tremAlienUpgradeCount <= 0 )
			{
				Menus_CloseAll();
			}
		}
		else if ( Q_stricmp ( name, "UpgradeToNewClass" ) == 0 )
		{
			if ( ( cmd = uiInfo.tremAlienUpgradeList[ uiInfo.tremAlienUpgradeIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp ( name, "LoadAlienBuilds" ) == 0 )
		{
			UI_LoadTremAlienBuilds();
		}
		else if ( Q_stricmp ( name, "BuildAlienBuildable" ) == 0 )
		{
			if ( ( cmd = uiInfo.tremAlienBuildList[ uiInfo.tremAlienBuildIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp ( name, "LoadHumanBuilds" ) == 0 )
		{
			UI_LoadTremHumanBuilds();
		}
		else if ( Q_stricmp ( name, "BuildHumanBuildable" ) == 0 )
		{
			if ( ( cmd = uiInfo.tremHumanBuildList[ uiInfo.tremHumanBuildIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp ( name, "PTRCRestore" ) == 0 )
		{
			int          len;
			char         text[ 16 ];
			fileHandle_t f;
			char         command[ 32 ];

			// load the file
			len = trap_FS_FOpenFile ( "ptrc.cfg", &f, FS_READ );

			if ( len > 0 && ( len < sizeof ( text ) - 1 ) )
			{
				trap_FS_Read ( text, len, f );
				text[ len ] = 0;
				trap_FS_FCloseFile ( f );

				Com_sprintf ( command, 32, "ptrcrestore %s", text );

				trap_Cmd_ExecuteText ( EXEC_APPEND, command );
			}
		}
//TA: tremulous menus

		else if ( Q_stricmp ( name, "playMovie" ) == 0 )
		{
			if ( uiInfo.previewMovie >= 0 )
			{
				trap_CIN_StopCinematic ( uiInfo.previewMovie );
			}

			trap_Cmd_ExecuteText ( EXEC_APPEND, va ( "cinematic %s.roq 2\n", uiInfo.movieList[ uiInfo.movieIndex ] ) );
		}
		else if ( Q_stricmp ( name, "RunMod" ) == 0 )
		{
			trap_Cvar_Set ( "fs_game", uiInfo.modList[ uiInfo.modIndex ].modName );
			trap_Cmd_ExecuteText ( EXEC_APPEND, "vid_restart;" );
		}
		else if ( Q_stricmp ( name, "RunDemo" ) == 0 )
		{
			trap_Cmd_ExecuteText ( EXEC_APPEND, va ( "demo %s\n", uiInfo.demoList[ uiInfo.demoIndex ] ) );
		}
		else if ( Q_stricmp ( name, "Tremulous" ) == 0 )
		{
			trap_Cvar_Set ( "fs_game", "" );
			trap_Cmd_ExecuteText ( EXEC_APPEND, "vid_restart;" );
		}
		else if ( Q_stricmp ( name, "closeJoin" ) == 0 )
		{
			if ( uiInfo.serverStatus.refreshActive )
			{
				UI_StopServerRefresh();
				uiInfo.serverStatus.nextDisplayRefresh = 0;
				uiInfo.nextServerStatusRefresh = 0;
				uiInfo.nextFindPlayerRefresh = 0;
				UI_BuildServerDisplayList ( qtrue );
			}
			else
			{
				Menus_CloseByName ( "joinserver" );
				Menus_OpenByName ( "main" );
			}
		}
		else if ( Q_stricmp ( name, "StopRefresh" ) == 0 )
		{
			UI_StopServerRefresh();
			uiInfo.serverStatus.nextDisplayRefresh = 0;
			uiInfo.nextServerStatusRefresh = 0;
			uiInfo.nextFindPlayerRefresh = 0;
		}
		else if ( Q_stricmp ( name, "UpdateFilter" ) == 0 )
		{
			if ( ui_netSource.integer == AS_LOCAL )
			{
				UI_StartServerRefresh ( qtrue );
			}

			UI_BuildServerDisplayList ( qtrue );
			UI_FeederSelection ( FEEDER_SERVERS, 0 );
		}
		else if ( Q_stricmp ( name, "ServerStatus" ) == 0 )
		{
			trap_LAN_GetServerAddressString ( ui_netSource.integer, uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ], uiInfo.serverStatusAddress, sizeof ( uiInfo.serverStatusAddress ) );
			UI_BuildServerStatus ( qtrue );
		}
		else if ( Q_stricmp ( name, "FoundPlayerServerStatus" ) == 0 )
		{
			Q_strncpyz ( uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[ uiInfo.currentFoundPlayerServer ], sizeof ( uiInfo.serverStatusAddress ) );
			UI_BuildServerStatus ( qtrue );
			Menu_SetFeederSelection ( NULL, FEEDER_FINDPLAYER, 0, NULL );
		}
		else if ( Q_stricmp ( name, "FindPlayer" ) == 0 )
		{
			UI_BuildFindPlayerList ( qtrue );
			// clear the displayed server status info
			uiInfo.serverStatusInfo.numLines = 0;
			Menu_SetFeederSelection ( NULL, FEEDER_FINDPLAYER, 0, NULL );
		}
		else if ( Q_stricmp ( name, "JoinServer" ) == 0 )
		{
			trap_Cvar_Set ( "cg_thirdPerson", "0" );
			trap_Cvar_Set ( "cg_cameraOrbit", "0" );
			trap_Cvar_Set ( "ui_singlePlayerActive", "0" );

			if ( uiInfo.serverStatus.currentServer >= 0 && uiInfo.serverStatus.currentServer < uiInfo.serverStatus.numDisplayServers )
			{
				trap_LAN_GetServerAddressString ( ui_netSource.integer, uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ], buff, 1024 );
				trap_Cmd_ExecuteText ( EXEC_APPEND, va ( "connect %s\n", buff ) );
			}
		}
		else if ( Q_stricmp ( name, "FoundPlayerJoinServer" ) == 0 )
		{
			trap_Cvar_Set ( "ui_singlePlayerActive", "0" );

			if ( uiInfo.currentFoundPlayerServer >= 0 && uiInfo.currentFoundPlayerServer < uiInfo.numFoundPlayerServers )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, va ( "connect %s\n", uiInfo.foundPlayerServerAddresses[ uiInfo.currentFoundPlayerServer ] ) );
			}
		}
		else if ( Q_stricmp ( name, "Quit" ) == 0 )
		{
			trap_Cvar_Set ( "ui_singlePlayerActive", "0" );
			trap_Cmd_ExecuteText ( EXEC_NOW, "quit" );
		}
		else if ( Q_stricmp ( name, "Controls" ) == 0 )
		{
			trap_Cvar_Set ( "cl_paused", "1" );
			trap_Key_SetCatcher ( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName ( "setup_menu2" );
		}
		else if ( Q_stricmp ( name, "Leave" ) == 0 )
		{
			trap_Cmd_ExecuteText ( EXEC_APPEND, "disconnect\n" );
			trap_Key_SetCatcher ( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName ( "main" );
		}
		else if ( Q_stricmp ( name, "ServerSort" ) == 0 )
		{
			int sortColumn;

			if ( Int_Parse ( args, &sortColumn ) )
			{
				// if same column we're already sorting on then flip the direction
				if ( sortColumn == uiInfo.serverStatus.sortKey )
				{
					uiInfo.serverStatus.sortDir = !uiInfo.serverStatus.sortDir;
				}

				// make sure we sort again
				UI_ServersSort ( sortColumn, qtrue );
			}
		}
		else if ( Q_stricmp ( name, "nextSkirmish" ) == 0 )
		{
			UI_StartSkirmish ( qtrue );
		}
		else if ( Q_stricmp ( name, "SkirmishStart" ) == 0 )
		{
			UI_StartSkirmish ( qfalse );
		}
		else if ( Q_stricmp ( name, "closeingame" ) == 0 )
		{
			trap_Key_SetCatcher ( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			trap_Cvar_Set ( "cl_paused", "0" );
			Menus_CloseAll();
		}
		else if ( Q_stricmp ( name, "voteMap" ) == 0 )
		{
			if ( ui_currentNetMap.integer >= 0 && ui_currentNetMap.integer < uiInfo.mapCount )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, va ( "callvote map %s\n", uiInfo.mapList[ ui_currentNetMap.integer ].mapLoadName ) );
			}
		}
		else if ( Q_stricmp ( name, "voteKick" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, va ( "callvote kick %s\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp ( name, "voteTeamKick" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, va ( "callteamvote teamkick %s\n", uiInfo.playerNames[ uiInfo.teamIndex ] ) );
			}
		}
		else if ( Q_stricmp ( name, "voteGame" ) == 0 )
		{
			if ( ui_netGameType.integer >= 0 && ui_netGameType.integer < uiInfo.numGameTypes )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, va ( "callvote g_gametype %i\n", uiInfo.gameTypes[ ui_netGameType.integer ].gtEnum ) );
			}
		}
		else if ( Q_stricmp ( name, "voteLeader" ) == 0 )
		{
			if ( uiInfo.teamIndex >= 0 && uiInfo.teamIndex < uiInfo.myTeamCount )
			{
				trap_Cmd_ExecuteText ( EXEC_APPEND, va ( "callteamvote leader %s\n", uiInfo.teamNames[ uiInfo.teamIndex ] ) );
			}
		}
		else if ( Q_stricmp ( name, "addBot" ) == 0 )
		{
			trap_Cmd_ExecuteText ( EXEC_APPEND, va ( "addbot %s %i %s\n", UI_GetBotNameByNumber ( uiInfo.botIndex ), uiInfo.skillIndex + 1, ( uiInfo.redBlue == 0 ) ? "Red" : "Blue" ) );
		}
		else if ( Q_stricmp ( name, "addFavorite" ) == 0 )
		{
			if ( ui_netSource.integer != AS_FAVORITES )
			{
				char name[ MAX_NAME_LENGTH ];
				char addr[ MAX_NAME_LENGTH ];
				int  res;

				trap_LAN_GetServerInfo ( ui_netSource.integer, uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ], buff, MAX_STRING_CHARS );
				name[ 0 ] = addr[ 0 ] = '\0';
				Q_strncpyz ( name,  Info_ValueForKey ( buff, "hostname" ), MAX_NAME_LENGTH );
				Q_strncpyz ( addr,  Info_ValueForKey ( buff, "addr" ), MAX_NAME_LENGTH );

				if ( strlen ( name ) > 0 && strlen ( addr ) > 0 )
				{
					res = trap_LAN_AddServer ( AS_FAVORITES, name, addr );

					if ( res == 0 )
					{
						// server already in the list
						Com_Printf ( "%s", trap_TranslateString ( "Favorite already in list\n" ) );
					}
					else if ( res == -1 )
					{
						// list full
						Com_Printf ( "%s", trap_TranslateString ( "Favorite list full\n" ) );
					}
					else
					{
						// successfully added
						Com_Printf ( trap_TranslateString ( "Added favorite server %s\n" ), addr );
					}
				}
			}
		}
		else if ( Q_stricmp ( name, "deleteFavorite" ) == 0 )
		{
			if ( ui_netSource.integer == AS_FAVORITES )
			{
				char addr[ MAX_NAME_LENGTH ];
				trap_LAN_GetServerInfo ( ui_netSource.integer, uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ], buff, MAX_STRING_CHARS );
				addr[ 0 ] = '\0';
				Q_strncpyz ( addr,  Info_ValueForKey ( buff, "addr" ), MAX_NAME_LENGTH );

				if ( strlen ( addr ) > 0 )
				{
					trap_LAN_RemoveServer ( AS_FAVORITES, addr );
				}
			}
		}
		else if ( Q_stricmp ( name, "createFavorite" ) == 0 )
		{
			if ( ui_netSource.integer == AS_FAVORITES )
			{
				char name[ MAX_NAME_LENGTH ];
				char addr[ MAX_NAME_LENGTH ];
				int  res;

				name[ 0 ] = addr[ 0 ] = '\0';
				Q_strncpyz ( name,  UI_Cvar_VariableString ( "ui_favoriteName" ), MAX_NAME_LENGTH );
				Q_strncpyz ( addr,  UI_Cvar_VariableString ( "ui_favoriteAddress" ), MAX_NAME_LENGTH );

				if ( strlen ( name ) > 0 && strlen ( addr ) > 0 )
				{
					res = trap_LAN_AddServer ( AS_FAVORITES, name, addr );

					if ( res == 0 )
					{
						// server already in the list
						Com_Printf ( "%s", trap_TranslateString ( "Favorite already in list\n" ) );
					}
					else if ( res == -1 )
					{
						// list full
						Com_Printf ( "%s", trap_TranslateString ( "Favorite list full\n" ) );
					}
					else
					{
						// successfully added
						Com_Printf ( trap_TranslateString ( "Added favorite server %s\n" ), addr );
					}
				}
			}
		}
		else if ( Q_stricmp ( name, "orders" ) == 0 )
		{
			const char *orders;

			if ( String_Parse ( args, &orders ) )
			{
				int selectedPlayer = trap_Cvar_VariableValue ( "cg_selectedPlayer" );

				if ( selectedPlayer < uiInfo.myTeamCount )
				{
					strcpy ( buff, orders );
					trap_Cmd_ExecuteText ( EXEC_APPEND, va ( buff, uiInfo.teamClientNums[ selectedPlayer ] ) );
					trap_Cmd_ExecuteText ( EXEC_APPEND, "\n" );
				}
				else
				{
					int i;

					for ( i = 0; i < uiInfo.myTeamCount; i++ )
					{
						if ( Q_stricmp ( UI_Cvar_VariableString ( "name" ), uiInfo.teamNames[ i ] ) == 0 )
						{
							continue;
						}

						strcpy ( buff, orders );
						trap_Cmd_ExecuteText ( EXEC_APPEND, va ( buff, uiInfo.teamNames[ i ] ) );
						trap_Cmd_ExecuteText ( EXEC_APPEND, "\n" );
					}
				}

				trap_Key_SetCatcher ( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set ( "cl_paused", "0" );
				Menus_CloseAll();
			}
		}
		else if ( Q_stricmp ( name, "voiceOrdersTeam" ) == 0 )
		{
			const char *orders;

			if ( String_Parse ( args, &orders ) )
			{
				int selectedPlayer = trap_Cvar_VariableValue ( "cg_selectedPlayer" );

				if ( selectedPlayer == uiInfo.myTeamCount )
				{
					trap_Cmd_ExecuteText ( EXEC_APPEND, orders );
					trap_Cmd_ExecuteText ( EXEC_APPEND, "\n" );
				}

				trap_Key_SetCatcher ( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set ( "cl_paused", "0" );
				Menus_CloseAll();
			}
		}
		else if ( Q_stricmp ( name, "voiceOrders" ) == 0 )
		{
			const char *orders;

			if ( String_Parse ( args, &orders ) )
			{
				int selectedPlayer = trap_Cvar_VariableValue ( "cg_selectedPlayer" );

				if ( selectedPlayer < uiInfo.myTeamCount )
				{
					strcpy ( buff, orders );
					trap_Cmd_ExecuteText ( EXEC_APPEND, va ( buff, uiInfo.teamClientNums[ selectedPlayer ] ) );
					trap_Cmd_ExecuteText ( EXEC_APPEND, "\n" );
				}

				trap_Key_SetCatcher ( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set ( "cl_paused", "0" );
				Menus_CloseAll();
			}
		}
		else if ( Q_stricmp ( name, "glCustom" ) == 0 )
		{
			trap_Cvar_Set ( "ui_glCustom", "4" );
		}
		else if ( Q_stricmp ( name, "update" ) == 0 )
		{
			if ( String_Parse ( args, &name2 ) )
			{
				UI_Update ( name2 );
			}
		}
		else
		{
			Com_Printf ( "unknown UI script %s\n", name );
		}
	}
}

static void UI_GetTeamColor ( vec4_t *color )
{
}

/*
==================
UI_MapCountByGameType
==================
*/
static int UI_MapCountByGameType ( qboolean singlePlayer )
{
	int i, c, game;
	c = 0;
	game = singlePlayer ? uiInfo.gameTypes[ ui_gameType.integer ].gtEnum : uiInfo.gameTypes[ ui_netGameType.integer ].gtEnum;

	for ( i = 0; i < uiInfo.mapCount; i++ )
	{
		uiInfo.mapList[ i ].active = qfalse;

		if ( uiInfo.mapList[ i ].typeBits & ( 1 << game ) )
		{
			if ( singlePlayer )
			{
				if ( ! ( uiInfo.mapList[ i ].typeBits & ( 1 << 2 ) ) )
				{
					continue;
				}
			}

			c++;
			uiInfo.mapList[ i ].active = qtrue;
		}
	}

	return c;
}

qboolean UI_hasSkinForBase ( const char *base, const char *team )
{
	char test[ 1024 ];

	Com_sprintf ( test, sizeof ( test ), "models/players/%s/%s/lower_default.skin", base, team );

	if ( trap_FS_FOpenFile ( test, NULL, FS_READ ) )
	{
		return qtrue;
	}

	Com_sprintf ( test, sizeof ( test ), "models/players/characters/%s/%s/lower_default.skin", base, team );

	if ( trap_FS_FOpenFile ( test, NULL, FS_READ ) )
	{
		return qtrue;
	}

	return qfalse;
}

/*
==================
UI_MapCountByTeam
==================
*/
static int UI_HeadCountByTeam ( void )
{
	static int init = 0;
	int        i, j, k, c, tIndex;

	c = 0;

	if ( !init )
	{
		for ( i = 0; i < uiInfo.characterCount; i++ )
		{
			uiInfo.characterList[ i ].reference = 0;

			for ( j = 0; j < uiInfo.teamCount; j++ )
			{
				if ( UI_hasSkinForBase ( uiInfo.characterList[ i ].base, uiInfo.teamList[ j ].teamName ) )
				{
					uiInfo.characterList[ i ].reference |= ( 1 << j );
				}
			}
		}

		init = 1;
	}

	tIndex = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_teamName" ) );

	// do names
	for ( i = 0; i < uiInfo.characterCount; i++ )
	{
		uiInfo.characterList[ i ].active = qfalse;

		for ( j = 0; j < TEAM_MEMBERS; j++ )
		{
			if ( uiInfo.teamList[ tIndex ].teamMembers[ j ] != NULL )
			{
				if ( uiInfo.characterList[ i ].reference & ( 1 << tIndex ) ) // && Q_stricmp(uiInfo.teamList[tIndex].teamMembers[j], uiInfo.characterList[i].name)==0) {
				{
					uiInfo.characterList[ i ].active = qtrue;
					c++;
					break;
				}
			}
		}
	}

	// and then aliases
	for ( j = 0; j < TEAM_MEMBERS; j++ )
	{
		for ( k = 0; k < uiInfo.aliasCount; k++ )
		{
			if ( uiInfo.aliasList[ k ].name != NULL )
			{
				if ( Q_stricmp ( uiInfo.teamList[ tIndex ].teamMembers[ j ], uiInfo.aliasList[ k ].name ) == 0 )
				{
					for ( i = 0; i < uiInfo.characterCount; i++ )
					{
						if ( uiInfo.characterList[ i ].headImage != -1 && uiInfo.characterList[ i ].reference & ( 1 << tIndex ) && Q_stricmp ( uiInfo.aliasList[ k ].ai, uiInfo.characterList[ i ].name ) == 0 )
						{
							if ( uiInfo.characterList[ i ].active == qfalse )
							{
								uiInfo.characterList[ i ].active = qtrue;
								c++;
							}

							break;
						}
					}
				}
			}
		}
	}

	return c;
}

/*
==================
UI_InsertServerIntoDisplayList
==================
*/
static void UI_InsertServerIntoDisplayList ( int num, int position )
{
	int i;

	if ( position < 0 || position > uiInfo.serverStatus.numDisplayServers )
	{
		return;
	}

	//
	uiInfo.serverStatus.numDisplayServers++;

	for ( i = uiInfo.serverStatus.numDisplayServers; i > position; i-- )
	{
		uiInfo.serverStatus.displayServers[ i ] = uiInfo.serverStatus.displayServers[ i - 1 ];
	}

	uiInfo.serverStatus.displayServers[ position ] = num;
}

/*
==================
UI_RemoveServerFromDisplayList
==================
*/
static void UI_RemoveServerFromDisplayList ( int num )
{
	int i, j;

	for ( i = 0; i < uiInfo.serverStatus.numDisplayServers; i++ )
	{
		if ( uiInfo.serverStatus.displayServers[ i ] == num )
		{
			uiInfo.serverStatus.numDisplayServers--;

			for ( j = i; j < uiInfo.serverStatus.numDisplayServers; j++ )
			{
				uiInfo.serverStatus.displayServers[ j ] = uiInfo.serverStatus.displayServers[ j + 1 ];
			}

			return;
		}
	}
}

/*
==================
UI_BinaryServerInsertion
==================
*/
static void UI_BinaryServerInsertion ( int num )
{
	int mid, offset, res, len;

	// use binary search to insert server
	len = uiInfo.serverStatus.numDisplayServers;
	mid = len;
	offset = 0;
	res = 0;

	while ( mid > 0 )
	{
		mid = len >> 1;
		//
		res = trap_LAN_CompareServers ( ui_netSource.integer, uiInfo.serverStatus.sortKey,
		                                uiInfo.serverStatus.sortDir, num, uiInfo.serverStatus.displayServers[ offset + mid ] );

		// if equal
		if ( res == 0 )
		{
			UI_InsertServerIntoDisplayList ( num, offset + mid );
			return;
		}
		// if larger
		else if ( res == 1 )
		{
			offset += mid;
			len -= mid;
		}
		// if smaller
		else
		{
			len -= mid;
		}
	}

	if ( res == 1 )
	{
		offset++;
	}

	UI_InsertServerIntoDisplayList ( num, offset );
}

/*
==================
UI_BuildServerDisplayList
==================
*/
static void UI_BuildServerDisplayList ( qboolean force )
{
	int        i, count, clients, maxClients, ping, game, len, visible;
	char       info[ MAX_STRING_CHARS ];
//  qboolean startRefresh = qtrue; TTimo: unused
	static int numinvisible;

	if ( ! ( force || uiInfo.uiDC.realTime > uiInfo.serverStatus.nextDisplayRefresh ) )
	{
		return;
	}

	// if we shouldn't reset
	if ( force == 2 )
	{
		force = 0;
	}

	// do motd updates here too
	trap_Cvar_VariableStringBuffer ( "cl_motdString", uiInfo.serverStatus.motd, sizeof ( uiInfo.serverStatus.motd ) );
	len = strlen ( uiInfo.serverStatus.motd );

	if ( len != uiInfo.serverStatus.motdLen )
	{
		uiInfo.serverStatus.motdLen = len;
		uiInfo.serverStatus.motdWidth = -1;
	}

	if ( force )
	{
		numinvisible = 0;
		// clear number of displayed servers
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		// set list box index to zero
		Menu_SetFeederSelection ( NULL, FEEDER_SERVERS, 0, NULL );
		// mark all servers as visible so we store ping updates for them
		trap_LAN_MarkServerVisible ( ui_netSource.integer, -1, qtrue );
	}

	// get the server count (comes from the master)
	count = trap_LAN_GetServerCount ( ui_netSource.integer );

	if ( count == -1 || ( ui_netSource.integer == AS_LOCAL && count == 0 ) )
	{
		// still waiting on a response from the master
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 500;
		return;
	}

	visible = qfalse;

	for ( i = 0; i < count; i++ )
	{
		// if we already got info for this server
		if ( !trap_LAN_ServerIsVisible ( ui_netSource.integer, i ) )
		{
			continue;
		}

		visible = qtrue;
		// get the ping for this server
		ping = trap_LAN_GetServerPing ( ui_netSource.integer, i );

		if ( ping > 0 || ui_netSource.integer == AS_FAVORITES )
		{
			trap_LAN_GetServerInfo ( ui_netSource.integer, i, info, MAX_STRING_CHARS );

			clients = atoi ( Info_ValueForKey ( info, "clients" ) );
			uiInfo.serverStatus.numPlayersOnServers += clients;

			if ( ui_browserShowEmpty.integer == 0 )
			{
				if ( clients == 0 )
				{
					trap_LAN_MarkServerVisible ( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			if ( ui_browserShowFull.integer == 0 )
			{
				maxClients = atoi ( Info_ValueForKey ( info, "sv_maxclients" ) );

				if ( clients == maxClients )
				{
					trap_LAN_MarkServerVisible ( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			if ( uiInfo.joinGameTypes[ ui_joinGameType.integer ].gtEnum != -1 )
			{
				game = atoi ( Info_ValueForKey ( info, "gametype" ) );

				if ( game != uiInfo.joinGameTypes[ ui_joinGameType.integer ].gtEnum )
				{
					trap_LAN_MarkServerVisible ( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			// make sure we never add a favorite server twice
			if ( ui_netSource.integer == AS_FAVORITES )
			{
				UI_RemoveServerFromDisplayList ( i );
			}

			// insert the server into the list
			UI_BinaryServerInsertion ( i );

			// done with this server
			if ( ping > 0 )
			{
				trap_LAN_MarkServerVisible ( ui_netSource.integer, i, qfalse );
				numinvisible++;
			}
		}
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime;

	// if there were no servers visible for ping updates
	if ( !visible )
	{
//    UI_StopServerRefresh();
//    uiInfo.serverStatus.nextDisplayRefresh = 0;
	}
}

typedef struct
{
	char *name, *altName;
} serverStatusCvar_t;

serverStatusCvar_t serverStatusCvars[] =
{
	{ "sv_hostname", "Name"      },
	{ "Address",     ""          },
	{ "gamename",    "Game name" },
	{ "g_gametype",  "Game type" },
	{ "mapname",     "Map"       },
	{ "version",     ""          },
	{ "protocol",    ""          },
	{ "timelimit",   ""          },
	{ "fraglimit",   ""          },
	{ NULL,          NULL        }
};

/*
==================
UI_SortServerStatusInfo
==================
*/
static void UI_SortServerStatusInfo ( serverStatusInfo_t *info )
{
	int  i, j, index;
	char *tmp1, *tmp2;

	// FIXME: if "gamename" == "baseq3" or "missionpack" then
	// replace the gametype number by FFA, CTF etc.
	//
	index = 0;

	for ( i = 0; serverStatusCvars[ i ].name; i++ )
	{
		for ( j = 0; j < info->numLines; j++ )
		{
			if ( !info->lines[ j ][ 1 ] || info->lines[ j ][ 1 ][ 0 ] )
			{
				continue;
			}

			if ( !Q_stricmp ( serverStatusCvars[ i ].name, info->lines[ j ][ 0 ] ) )
			{
				// swap lines
				tmp1 = info->lines[ index ][ 0 ];
				tmp2 = info->lines[ index ][ 3 ];
				info->lines[ index ][ 0 ] = info->lines[ j ][ 0 ];
				info->lines[ index ][ 3 ] = info->lines[ j ][ 3 ];
				info->lines[ j ][ 0 ] = tmp1;
				info->lines[ j ][ 3 ] = tmp2;

				//
				if ( strlen ( serverStatusCvars[ i ].altName ) )
				{
					info->lines[ index ][ 0 ] = serverStatusCvars[ i ].altName;
				}

				index++;
			}
		}
	}
}

/*
==================
UI_GetServerStatusInfo
==================
*/
static int UI_GetServerStatusInfo ( const char *serverAddress, serverStatusInfo_t *info )
{
	char *p, *score, *ping, *name;
	int  i, len;

	if ( !info )
	{
		trap_LAN_ServerStatus ( serverAddress, NULL, 0 );
		return qfalse;
	}

	memset ( info, 0, sizeof ( *info ) );

	if ( trap_LAN_ServerStatus ( serverAddress, info->text, sizeof ( info->text ) ) )
	{
		Q_strncpyz ( info->address, serverAddress, sizeof ( info->address ) );
		p = info->text;
		info->numLines = 0;
		info->lines[ info->numLines ][ 0 ] = "Address";
		info->lines[ info->numLines ][ 1 ] = "";
		info->lines[ info->numLines ][ 2 ] = "";
		info->lines[ info->numLines ][ 3 ] = info->address;
		info->numLines++;

		// get the cvars
		while ( p && *p )
		{
			p = strchr ( p, '\\' );

			if ( !p ) { break; }

			*p++ = '\0';

			if ( *p == '\\' )
			{
				break;
			}

			info->lines[ info->numLines ][ 0 ] = p;
			info->lines[ info->numLines ][ 1 ] = "";
			info->lines[ info->numLines ][ 2 ] = "";
			p = strchr ( p, '\\' );

			if ( !p ) { break; }

			*p++ = '\0';
			info->lines[ info->numLines ][ 3 ] = p;

			info->numLines++;

			if ( info->numLines >= MAX_SERVERSTATUS_LINES )
			{
				break;
			}
		}

		// get the player list
		if ( info->numLines < MAX_SERVERSTATUS_LINES - 3 )
		{
			// empty line
			info->lines[ info->numLines ][ 0 ] = "";
			info->lines[ info->numLines ][ 1 ] = "";
			info->lines[ info->numLines ][ 2 ] = "";
			info->lines[ info->numLines ][ 3 ] = "";
			info->numLines++;
			// header
			info->lines[ info->numLines ][ 0 ] = "num";
			info->lines[ info->numLines ][ 1 ] = "score";
			info->lines[ info->numLines ][ 2 ] = "ping";
			info->lines[ info->numLines ][ 3 ] = "name";
			info->numLines++;
			// parse players
			i = 0;
			len = 0;

			while ( p && *p )
			{
				if ( *p == '\\' )
				{
					*p++ = '\0';
				}

				if ( !p )
				{
					break;
				}

				score = p;
				p = strchr ( p, ' ' );

				if ( !p )
				{
					break;
				}

				*p++ = '\0';
				ping = p;
				p = strchr ( p, ' ' );

				if ( !p )
				{
					break;
				}

				*p++ = '\0';
				name = p;
				Com_sprintf ( &info->pings[ len ], sizeof ( info->pings ) - len, "%d", i );
				info->lines[ info->numLines ][ 0 ] = &info->pings[ len ];
				len += strlen ( &info->pings[ len ] ) + 1;
				info->lines[ info->numLines ][ 1 ] = score;
				info->lines[ info->numLines ][ 2 ] = ping;
				info->lines[ info->numLines ][ 3 ] = name;
				info->numLines++;

				if ( info->numLines >= MAX_SERVERSTATUS_LINES )
				{
					break;
				}

				p = strchr ( p, '\\' );

				if ( !p )
				{
					break;
				}

				*p++ = '\0';
				//
				i++;
			}
		}

		UI_SortServerStatusInfo ( info );
		return qtrue;
	}

	return qfalse;
}

/*
==================
stristr
==================
*/
static char *stristr ( char *str, char *charset )
{
	int i;

	while ( *str )
	{
		for ( i = 0; charset[ i ] && str[ i ]; i++ )
		{
			if ( toupper ( charset[ i ] ) != toupper ( str[ i ] ) ) { break; }
		}

		if ( !charset[ i ] ) { return str; }

		str++;
	}

	return NULL;
}

/*
==================
UI_BuildFindPlayerList
==================
*/
static void UI_BuildFindPlayerList ( qboolean force )
{
	static int         numFound, numTimeOuts;
	int                i, j, resend;
	serverStatusInfo_t info;
	char               name[ MAX_NAME_LENGTH + 2 ];
	char               infoString[ MAX_STRING_CHARS ];

	if ( !force )
	{
		if ( !uiInfo.nextFindPlayerRefresh || uiInfo.nextFindPlayerRefresh > uiInfo.uiDC.realTime )
		{
			return;
		}
	}
	else
	{
		memset ( &uiInfo.pendingServerStatus, 0, sizeof ( uiInfo.pendingServerStatus ) );
		uiInfo.numFoundPlayerServers = 0;
		uiInfo.currentFoundPlayerServer = 0;
		trap_Cvar_VariableStringBuffer ( "ui_findPlayer", uiInfo.findPlayerName, sizeof ( uiInfo.findPlayerName ) );
		Q_CleanStr ( uiInfo.findPlayerName );

		// should have a string of some length
		if ( !strlen ( uiInfo.findPlayerName ) )
		{
			uiInfo.nextFindPlayerRefresh = 0;
			return;
		}

		// set resend time
		resend = ui_serverStatusTimeOut.integer / 2 - 10;

		if ( resend < 50 )
		{
			resend = 50;
		}

		trap_Cvar_Set ( "cl_serverStatusResendTime", va ( "%d", resend ) );
		// reset all server status requests
		trap_LAN_ServerStatus ( NULL, NULL, 0 );
		//
		uiInfo.numFoundPlayerServers = 1;
		Com_sprintf ( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
		              sizeof ( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ] ),
		              "searching %d...", uiInfo.pendingServerStatus.num );
		numFound = 0;
		numTimeOuts++;
	}

	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ )
	{
		// if this pending server is valid
		if ( uiInfo.pendingServerStatus.server[ i ].valid )
		{
			// try to get the server status for this server
			if ( UI_GetServerStatusInfo ( uiInfo.pendingServerStatus.server[ i ].adrstr, &info ) )
			{
				//
				numFound++;

				// parse through the server status lines
				for ( j = 0; j < info.numLines; j++ )
				{
					// should have ping info
					if ( !info.lines[ j ][ 2 ] || !info.lines[ j ][ 2 ][ 0 ] )
					{
						continue;
					}

					// clean string first
					Q_strncpyz ( name, info.lines[ j ][ 3 ], sizeof ( name ) );
					Q_CleanStr ( name );

					// if the player name is a substring
					if ( stristr ( name, uiInfo.findPlayerName ) )
					{
						// add to found server list if we have space (always leave space for a line with the number found)
						if ( uiInfo.numFoundPlayerServers < MAX_FOUNDPLAYER_SERVERS - 1 )
						{
							//
							Q_strncpyz ( uiInfo.foundPlayerServerAddresses[ uiInfo.numFoundPlayerServers - 1 ],
							             uiInfo.pendingServerStatus.server[ i ].adrstr,
							             sizeof ( uiInfo.foundPlayerServerAddresses[ 0 ] ) );
							Q_strncpyz ( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
							             uiInfo.pendingServerStatus.server[ i ].name,
							             sizeof ( uiInfo.foundPlayerServerNames[ 0 ] ) );
							uiInfo.numFoundPlayerServers++;
						}
						else
						{
							// can't add any more so we're done
							uiInfo.pendingServerStatus.num = uiInfo.serverStatus.numDisplayServers;
						}
					}
				}

				Com_sprintf ( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
				              sizeof ( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ] ),
				              "searching %d/%d...", uiInfo.pendingServerStatus.num, numFound );
				// retrieved the server status so reuse this spot
				uiInfo.pendingServerStatus.server[ i ].valid = qfalse;
			}
		}

		// if empty pending slot or timed out
		if ( !uiInfo.pendingServerStatus.server[ i ].valid ||
		     uiInfo.pendingServerStatus.server[ i ].startTime < uiInfo.uiDC.realTime - ui_serverStatusTimeOut.integer )
		{
			if ( uiInfo.pendingServerStatus.server[ i ].valid )
			{
				numTimeOuts++;
			}

			// reset server status request for this address
			UI_GetServerStatusInfo ( uiInfo.pendingServerStatus.server[ i ].adrstr, NULL );
			// reuse pending slot
			uiInfo.pendingServerStatus.server[ i ].valid = qfalse;

			// if we didn't try to get the status of all servers in the main browser yet
			if ( uiInfo.pendingServerStatus.num < uiInfo.serverStatus.numDisplayServers )
			{
				uiInfo.pendingServerStatus.server[ i ].startTime = uiInfo.uiDC.realTime;
				trap_LAN_GetServerAddressString ( ui_netSource.integer, uiInfo.serverStatus.displayServers[ uiInfo.pendingServerStatus.num ],
				                                  uiInfo.pendingServerStatus.server[ i ].adrstr, sizeof ( uiInfo.pendingServerStatus.server[ i ].adrstr ) );
				trap_LAN_GetServerInfo ( ui_netSource.integer, uiInfo.serverStatus.displayServers[ uiInfo.pendingServerStatus.num ], infoString, sizeof ( infoString ) );
				Q_strncpyz ( uiInfo.pendingServerStatus.server[ i ].name, Info_ValueForKey ( infoString, "hostname" ), sizeof ( uiInfo.pendingServerStatus.server[ 0 ].name ) );
				uiInfo.pendingServerStatus.server[ i ].valid = qtrue;
				uiInfo.pendingServerStatus.num++;
				Com_sprintf ( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
				              sizeof ( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ] ),
				              "searching %d/%d...", uiInfo.pendingServerStatus.num, numFound );
			}
		}
	}

	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ )
	{
		if ( uiInfo.pendingServerStatus.server[ i ].valid )
		{
			break;
		}
	}

	// if still trying to retrieve server status info
	if ( i < MAX_SERVERSTATUSREQUESTS )
	{
		uiInfo.nextFindPlayerRefresh = uiInfo.uiDC.realTime + 25;
	}
	else
	{
		// add a line that shows the number of servers found
		if ( !uiInfo.numFoundPlayerServers )
		{
			Com_sprintf ( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ], sizeof ( uiInfo.foundPlayerServerAddresses[ 0 ] ), "no servers found" );
		}
		else
		{
			Com_sprintf ( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ], sizeof ( uiInfo.foundPlayerServerAddresses[ 0 ] ),
			              "%d server%s found with player %s", uiInfo.numFoundPlayerServers - 1,
			              uiInfo.numFoundPlayerServers == 2 ? "" : "s", uiInfo.findPlayerName );
		}

		uiInfo.nextFindPlayerRefresh = 0;
		// show the server status info for the selected server
		UI_FeederSelection ( FEEDER_FINDPLAYER, uiInfo.currentFoundPlayerServer );
	}
}

/*
==================
UI_BuildServerStatus
==================
*/
static void UI_BuildServerStatus ( qboolean force )
{
	if ( uiInfo.nextFindPlayerRefresh )
	{
		return;
	}

	if ( !force )
	{
		if ( !uiInfo.nextServerStatusRefresh || uiInfo.nextServerStatusRefresh > uiInfo.uiDC.realTime )
		{
			return;
		}
	}
	else
	{
		Menu_SetFeederSelection ( NULL, FEEDER_SERVERSTATUS, 0, NULL );
		uiInfo.serverStatusInfo.numLines = 0;
		// reset all server status requests
		trap_LAN_ServerStatus ( NULL, NULL, 0 );
	}

	if ( uiInfo.serverStatus.currentServer < 0 || uiInfo.serverStatus.currentServer > uiInfo.serverStatus.numDisplayServers || uiInfo.serverStatus.numDisplayServers == 0 )
	{
		return;
	}

	if ( UI_GetServerStatusInfo ( uiInfo.serverStatusAddress, &uiInfo.serverStatusInfo ) )
	{
		uiInfo.nextServerStatusRefresh = 0;
		UI_GetServerStatusInfo ( uiInfo.serverStatusAddress, NULL );
	}
	else
	{
		uiInfo.nextServerStatusRefresh = uiInfo.uiDC.realTime + 500;
	}
}

/*
==================
UI_FeederCount
==================
*/
static int UI_FeederCount ( float feederID )
{
	if ( feederID == FEEDER_HEADS )
	{
		return UI_HeadCountByTeam();
	}
	else if ( feederID == FEEDER_Q3HEADS )
	{
		return uiInfo.q3HeadCount;
	}
	else if ( feederID == FEEDER_CINEMATICS )
	{
		return uiInfo.movieCount;
	}
	else if ( feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS )
	{
		return UI_MapCountByGameType ( feederID == FEEDER_MAPS ? qtrue : qfalse );
	}
	else if ( feederID == FEEDER_SERVERS )
	{
		return uiInfo.serverStatus.numDisplayServers;
	}
	else if ( feederID == FEEDER_SERVERSTATUS )
	{
		return uiInfo.serverStatusInfo.numLines;
	}
	else if ( feederID == FEEDER_FINDPLAYER )
	{
		return uiInfo.numFoundPlayerServers;
	}
	else if ( feederID == FEEDER_PLAYER_LIST )
	{
		if ( uiInfo.uiDC.realTime > uiInfo.playerRefresh )
		{
			uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
			UI_BuildPlayerList();
		}

		return uiInfo.playerCount;
	}
	else if ( feederID == FEEDER_TEAM_LIST )
	{
		if ( uiInfo.uiDC.realTime > uiInfo.playerRefresh )
		{
			uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
			UI_BuildPlayerList();
		}

		return uiInfo.myTeamCount;
	}
	else if ( feederID == FEEDER_MODS )
	{
		return uiInfo.modCount;
	}
	else if ( feederID == FEEDER_DEMOS )
	{
		return uiInfo.demoCount;
	}

//TA: tremulous menus
	else if ( feederID == FEEDER_TREMTEAMS )
	{
		return uiInfo.tremTeamCount;
	}
	else if ( feederID == FEEDER_TREMHUMANITEMS )
	{
		return uiInfo.tremHumanItemCount;
	}
	else if ( feederID == FEEDER_TREMALIENCLASSES )
	{
		return uiInfo.tremAlienClassCount;
	}
	else if ( feederID == FEEDER_TREMHUMANARMOURYBUY )
	{
		return uiInfo.tremHumanArmouryBuyCount;
	}
	else if ( feederID == FEEDER_TREMHUMANARMOURYSELL )
	{
		return uiInfo.tremHumanArmourySellCount;
	}
	else if ( feederID == FEEDER_TREMALIENUPGRADE )
	{
		return uiInfo.tremAlienUpgradeCount;
	}
	else if ( feederID == FEEDER_TREMALIENBUILD )
	{
		return uiInfo.tremAlienBuildCount;
	}
	else if ( feederID == FEEDER_TREMHUMANBUILD )
	{
		return uiInfo.tremHumanBuildCount;
	}

//TA: tremulous menus

	return 0;
}

static const char *UI_SelectedMap ( int index, int *actual )
{
	int i, c;
	c = 0;
	*actual = 0;

	for ( i = 0; i < uiInfo.mapCount; i++ )
	{
		if ( uiInfo.mapList[ i ].active )
		{
			if ( c == index )
			{
				*actual = i;
				return uiInfo.mapList[ i ].mapName;
			}
			else
			{
				c++;
			}
		}
	}

	return "";
}

static const char *UI_SelectedHead ( int index, int *actual )
{
	int i, c;
	c = 0;
	*actual = 0;

	for ( i = 0; i < uiInfo.characterCount; i++ )
	{
		if ( uiInfo.characterList[ i ].active )
		{
			if ( c == index )
			{
				*actual = i;
				return uiInfo.characterList[ i ].name;
			}
			else
			{
				c++;
			}
		}
	}

	return "";
}

static int UI_GetIndexFromSelection ( int actual )
{
	int i, c;
	c = 0;

	for ( i = 0; i < uiInfo.mapCount; i++ )
	{
		if ( uiInfo.mapList[ i ].active )
		{
			if ( i == actual )
			{
				return c;
			}

			c++;
		}
	}

	return 0;
}

static void UI_UpdatePendingPings ( void )
{
	trap_LAN_ResetPings ( ui_netSource.integer );
	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
}

static const char *UI_FeederItemText ( float feederID, int index, int column, qhandle_t *handle )
{
	static char info[ MAX_STRING_CHARS ];
	static char hostname[ 1024 ];
	static char clientBuff[ 32 ];
	static int  lastColumn = -1;
	static int  lastTime = 0;
	*handle = -1;

	if ( feederID == FEEDER_HEADS )
	{
		int actual;
		return UI_SelectedHead ( index, &actual );
	}
	else if ( feederID == FEEDER_Q3HEADS )
	{
		if ( index >= 0 && index < uiInfo.q3HeadCount )
		{
			return uiInfo.q3HeadNames[ index ];
		}
	}
	else if ( feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS )
	{
		int actual;
		return UI_SelectedMap ( index, &actual );
	}
	else if ( feederID == FEEDER_SERVERS )
	{
		if ( index >= 0 && index < uiInfo.serverStatus.numDisplayServers )
		{
			int ping;

			if ( lastColumn != column || lastTime > uiInfo.uiDC.realTime + 5000 )
			{
				trap_LAN_GetServerInfo ( ui_netSource.integer, uiInfo.serverStatus.displayServers[ index ], info, MAX_STRING_CHARS );
				lastColumn = column;
				lastTime = uiInfo.uiDC.realTime;
			}

			ping = atoi ( Info_ValueForKey ( info, "ping" ) );

			if ( ping == -1 )
			{
				// if we ever see a ping that is out of date, do a server refresh
				// UI_UpdatePendingPings();
			}

			switch ( column )
			{
				case SORT_HOST:
					if ( ping <= 0 )
					{
						return Info_ValueForKey ( info, "addr" );
					}
					else
					{
						if ( ui_netSource.integer == AS_LOCAL )
						{
							Com_sprintf ( hostname, sizeof ( hostname ), "%s [%s]",
							              Info_ValueForKey ( info, "hostname" ),
							              netnames[ atoi ( Info_ValueForKey ( info, "nettype" ) ) ] );
							return hostname;
						}
						else
						{
							Com_sprintf ( hostname, sizeof ( hostname ), "%s", Info_ValueForKey ( info, "hostname" ) );

							return hostname;
						}
					}

				case SORT_MAP:
					return Info_ValueForKey ( info, "mapname" );

				case SORT_CLIENTS:
					Com_sprintf ( clientBuff, sizeof ( clientBuff ), "%s (%s)", Info_ValueForKey ( info, "clients" ), Info_ValueForKey ( info, "sv_maxclients" ) );
					return clientBuff;

				case SORT_PING:
					if ( ping <= 0 )
					{
						return "...";
					}
					else
					{
						return Info_ValueForKey ( info, "ping" );
					}
			}
		}
	}
	else if ( feederID == FEEDER_SERVERSTATUS )
	{
		if ( index >= 0 && index < uiInfo.serverStatusInfo.numLines )
		{
			if ( column >= 0 && column < 4 )
			{
				return uiInfo.serverStatusInfo.lines[ index ][ column ];
			}
		}
	}
	else if ( feederID == FEEDER_FINDPLAYER )
	{
		if ( index >= 0 && index < uiInfo.numFoundPlayerServers )
		{
			//return uiInfo.foundPlayerServerAddresses[index];
			return uiInfo.foundPlayerServerNames[ index ];
		}
	}
	else if ( feederID == FEEDER_PLAYER_LIST )
	{
		if ( index >= 0 && index < uiInfo.playerCount )
		{
			return uiInfo.playerNames[ index ];
		}
	}
	else if ( feederID == FEEDER_TEAM_LIST )
	{
		if ( index >= 0 && index < uiInfo.myTeamCount )
		{
			return uiInfo.teamNames[ index ];
		}
	}
	else if ( feederID == FEEDER_MODS )
	{
		if ( index >= 0 && index < uiInfo.modCount )
		{
			if ( uiInfo.modList[ index ].modDescr && *uiInfo.modList[ index ].modDescr )
			{
				return uiInfo.modList[ index ].modDescr;
			}
			else
			{
				return uiInfo.modList[ index ].modName;
			}
		}
	}
	else if ( feederID == FEEDER_CINEMATICS )
	{
		if ( index >= 0 && index < uiInfo.movieCount )
		{
			return uiInfo.movieList[ index ];
		}
	}
	else if ( feederID == FEEDER_DEMOS )
	{
		if ( index >= 0 && index < uiInfo.demoCount )
		{
			return uiInfo.demoList[ index ];
		}
	}

//TA: tremulous menus
	else if ( feederID == FEEDER_TREMTEAMS )
	{
		if ( index >= 0 && index < uiInfo.tremTeamCount )
		{
			return uiInfo.tremTeamList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMHUMANITEMS )
	{
		if ( index >= 0 && index < uiInfo.tremHumanItemCount )
		{
			return uiInfo.tremHumanItemList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMALIENCLASSES )
	{
		if ( index >= 0 && index < uiInfo.tremAlienClassCount )
		{
			return uiInfo.tremAlienClassList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMHUMANARMOURYBUY )
	{
		if ( index >= 0 && index < uiInfo.tremHumanArmouryBuyCount )
		{
			return uiInfo.tremHumanArmouryBuyList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMHUMANARMOURYSELL )
	{
		if ( index >= 0 && index < uiInfo.tremHumanArmourySellCount )
		{
			return uiInfo.tremHumanArmourySellList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMALIENUPGRADE )
	{
		if ( index >= 0 && index < uiInfo.tremAlienUpgradeCount )
		{
			return uiInfo.tremAlienUpgradeList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMALIENBUILD )
	{
		if ( index >= 0 && index < uiInfo.tremAlienBuildCount )
		{
			return uiInfo.tremAlienBuildList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMHUMANBUILD )
	{
		if ( index >= 0 && index < uiInfo.tremHumanBuildCount )
		{
			return uiInfo.tremHumanBuildList[ index ].text;
		}
	}

//TA: tremulous menus

	return "";
}

static qhandle_t UI_FeederItemImage ( float feederID, int index )
{
	if ( feederID == FEEDER_HEADS )
	{
		int actual;
		UI_SelectedHead ( index, &actual );
		index = actual;

		if ( index >= 0 && index < uiInfo.characterCount )
		{
			if ( uiInfo.characterList[ index ].headImage == -1 )
			{
				uiInfo.characterList[ index ].headImage = trap_R_RegisterShaderNoMip ( uiInfo.characterList[ index ].imageName );
			}

			return uiInfo.characterList[ index ].headImage;
		}
	}
	else if ( feederID == FEEDER_Q3HEADS )
	{
		if ( index >= 0 && index < uiInfo.q3HeadCount )
		{
			return uiInfo.q3HeadIcons[ index ];
		}
	}
	else if ( feederID == FEEDER_ALLMAPS || feederID == FEEDER_MAPS )
	{
		int actual;
		UI_SelectedMap ( index, &actual );
		index = actual;

		if ( index >= 0 && index < uiInfo.mapCount )
		{
			if ( uiInfo.mapList[ index ].levelShot == -1 )
			{
				uiInfo.mapList[ index ].levelShot = trap_R_RegisterShaderNoMip ( uiInfo.mapList[ index ].imageName );
			}

			return uiInfo.mapList[ index ].levelShot;
		}
	}

	return 0;
}

static void UI_FeederSelection ( float feederID, int index )
{
	static char info[ MAX_STRING_CHARS ];

	if ( feederID == FEEDER_HEADS )
	{
		int actual;
		UI_SelectedHead ( index, &actual );
		index = actual;

		if ( index >= 0 && index < uiInfo.characterCount )
		{
			trap_Cvar_Set ( "team_model", va ( "%s", uiInfo.characterList[ index ].base ) );
			trap_Cvar_Set ( "team_headmodel", va ( "*%s", uiInfo.characterList[ index ].name ) );
			updateModel = qtrue;
		}
	}
	else if ( feederID == FEEDER_Q3HEADS )
	{
		if ( index >= 0 && index < uiInfo.q3HeadCount )
		{
			trap_Cvar_Set ( "model", uiInfo.q3HeadNames[ index ] );
			trap_Cvar_Set ( "headmodel", uiInfo.q3HeadNames[ index ] );
			updateModel = qtrue;
		}
	}
	else if ( feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS )
	{
		int actual, map;
		map = ( feederID == FEEDER_ALLMAPS ) ? ui_currentNetMap.integer : ui_currentMap.integer;

		if ( uiInfo.mapList[ map ].cinematic >= 0 )
		{
			trap_CIN_StopCinematic ( uiInfo.mapList[ map ].cinematic );
			uiInfo.mapList[ map ].cinematic = -1;
		}

		UI_SelectedMap ( index, &actual );
		trap_Cvar_Set ( "ui_mapIndex", va ( "%d", index ) );
		ui_mapIndex.integer = index;

		if ( feederID == FEEDER_MAPS )
		{
			ui_currentMap.integer = actual;
			trap_Cvar_Set ( "ui_currentMap", va ( "%d", actual ) );
			uiInfo.mapList[ ui_currentMap.integer ].cinematic = trap_CIN_PlayCinematic ( va ( "%s.roq", uiInfo.mapList[ ui_currentMap.integer ].mapLoadName ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
			UI_LoadBestScores ( uiInfo.mapList[ ui_currentMap.integer ].mapLoadName, uiInfo.gameTypes[ ui_gameType.integer ].gtEnum );
			trap_Cvar_Set ( "ui_opponentModel", uiInfo.mapList[ ui_currentMap.integer ].opponentName );
			updateOpponentModel = qtrue;
		}
		else
		{
			ui_currentNetMap.integer = actual;
			trap_Cvar_Set ( "ui_currentNetMap", va ( "%d", actual ) );
			uiInfo.mapList[ ui_currentNetMap.integer ].cinematic = trap_CIN_PlayCinematic ( va ( "%s.roq", uiInfo.mapList[ ui_currentNetMap.integer ].mapLoadName ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
		}
	}
	else if ( feederID == FEEDER_SERVERS )
	{
		const char *mapName = NULL;
		uiInfo.serverStatus.currentServer = index;
		trap_LAN_GetServerInfo ( ui_netSource.integer, uiInfo.serverStatus.displayServers[ index ], info, MAX_STRING_CHARS );
		uiInfo.serverStatus.currentServerPreview = trap_R_RegisterShaderNoMip ( va ( "levelshots/%s", Info_ValueForKey ( info, "mapname" ) ) );

		if ( uiInfo.serverStatus.currentServerCinematic >= 0 )
		{
			trap_CIN_StopCinematic ( uiInfo.serverStatus.currentServerCinematic );
			uiInfo.serverStatus.currentServerCinematic = -1;
		}

		mapName = Info_ValueForKey ( info, "mapname" );

		if ( mapName && *mapName )
		{
			uiInfo.serverStatus.currentServerCinematic = trap_CIN_PlayCinematic ( va ( "%s.roq", mapName ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
		}
	}
	else if ( feederID == FEEDER_SERVERSTATUS )
	{
		//
	}
	else if ( feederID == FEEDER_FINDPLAYER )
	{
		uiInfo.currentFoundPlayerServer = index;

		//
		if ( index < uiInfo.numFoundPlayerServers - 1 )
		{
			// build a new server status for this server
			Q_strncpyz ( uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[ uiInfo.currentFoundPlayerServer ], sizeof ( uiInfo.serverStatusAddress ) );
			Menu_SetFeederSelection ( NULL, FEEDER_SERVERSTATUS, 0, NULL );
			UI_BuildServerStatus ( qtrue );
		}
	}
	else if ( feederID == FEEDER_PLAYER_LIST )
	{
		uiInfo.playerIndex = index;
	}
	else if ( feederID == FEEDER_TEAM_LIST )
	{
		uiInfo.teamIndex = index;
	}
	else if ( feederID == FEEDER_MODS )
	{
		uiInfo.modIndex = index;
	}
	else if ( feederID == FEEDER_CINEMATICS )
	{
		uiInfo.movieIndex = index;

		if ( uiInfo.previewMovie >= 0 )
		{
			trap_CIN_StopCinematic ( uiInfo.previewMovie );
		}

		uiInfo.previewMovie = -1;
	}
	else if ( feederID == FEEDER_DEMOS )
	{
		uiInfo.demoIndex = index;
	}

//TA: tremulous menus
	else if ( feederID == FEEDER_TREMTEAMS )
	{
		uiInfo.tremTeamIndex = index;
	}
	else if ( feederID == FEEDER_TREMHUMANITEMS )
	{
		uiInfo.tremHumanItemIndex = index;
	}
	else if ( feederID == FEEDER_TREMALIENCLASSES )
	{
		uiInfo.tremAlienClassIndex = index;
	}
	else if ( feederID == FEEDER_TREMHUMANARMOURYBUY )
	{
		uiInfo.tremHumanArmouryBuyIndex = index;
	}
	else if ( feederID == FEEDER_TREMHUMANARMOURYSELL )
	{
		uiInfo.tremHumanArmourySellIndex = index;
	}
	else if ( feederID == FEEDER_TREMALIENUPGRADE )
	{
		uiInfo.tremAlienUpgradeIndex = index;
	}
	else if ( feederID == FEEDER_TREMALIENBUILD )
	{
		uiInfo.tremAlienBuildIndex = index;
	}
	else if ( feederID == FEEDER_TREMHUMANBUILD )
	{
		uiInfo.tremHumanBuildIndex = index;
	}

//TA: tremulous menus
}

static void UI_Pause ( qboolean b )
{
	if ( b )
	{
		// pause the game and set the ui keycatcher
		trap_Cvar_Set ( "cl_paused", "1" );
		trap_Key_SetCatcher ( KEYCATCH_UI );
	}
	else
	{
		// unpause the game and clear the ui keycatcher
		trap_Key_SetCatcher ( trap_Key_GetCatcher() & ~KEYCATCH_UI );
		trap_Key_ClearStates();
		trap_Cvar_Set ( "cl_paused", "0" );
	}
}

static int UI_PlayCinematic ( const char *name, float x, float y, float w, float h )
{
	return trap_CIN_PlayCinematic ( name, x, y, w, h, ( CIN_loop | CIN_silent ) );
}

static void UI_StopCinematic ( int handle )
{
	if ( handle >= 0 )
	{
		trap_CIN_StopCinematic ( handle );
	}
	else
	{
		handle = abs ( handle );

		if ( handle == UI_MAPCINEMATIC )
		{
			if ( uiInfo.mapList[ ui_currentMap.integer ].cinematic >= 0 )
			{
				trap_CIN_StopCinematic ( uiInfo.mapList[ ui_currentMap.integer ].cinematic );
				uiInfo.mapList[ ui_currentMap.integer ].cinematic = -1;
			}
		}
		else if ( handle == UI_NETMAPCINEMATIC )
		{
			if ( uiInfo.serverStatus.currentServerCinematic >= 0 )
			{
				trap_CIN_StopCinematic ( uiInfo.serverStatus.currentServerCinematic );
				uiInfo.serverStatus.currentServerCinematic = -1;
			}
		}
		else if ( handle == UI_CLANCINEMATIC )
		{
			int i = UI_TeamIndexFromName ( UI_Cvar_VariableString ( "ui_teamName" ) );

			if ( i >= 0 && i < uiInfo.teamCount )
			{
				if ( uiInfo.teamList[ i ].cinematic >= 0 )
				{
					trap_CIN_StopCinematic ( uiInfo.teamList[ i ].cinematic );
					uiInfo.teamList[ i ].cinematic = -1;
				}
			}
		}
	}
}

static void UI_DrawCinematic ( int handle, float x, float y, float w, float h )
{
	trap_CIN_SetExtents ( handle, x, y, w, h );
	trap_CIN_DrawCinematic ( handle );
}

static void UI_RunCinematicFrame ( int handle )
{
	trap_CIN_RunCinematic ( handle );
}

/*
=================
PlayerModel_BuildList
=================
*/
static void UI_BuildQ3Model_List ( void )
{
	int  numdirs;
	int  numfiles;
	char dirlist[ 2048 ];
	char filelist[ 2048 ];
	char skinname[ 64 ];
	char scratch[ 256 ];
	char *dirptr;
	char *fileptr;
	int  i;
	int  j, k, dirty;
	int  dirlen;
	int  filelen;

	uiInfo.q3HeadCount = 0;

	// iterate directory of all player models
	numdirs = trap_FS_GetFileList ( "models/players", "/", dirlist, 2048 );
	dirptr = dirlist;

	for ( i = 0; i < numdirs && uiInfo.q3HeadCount < MAX_PLAYERMODELS; i++, dirptr += dirlen + 1 )
	{
		dirlen = strlen ( dirptr );

		if ( dirlen && dirptr[ dirlen - 1 ] == '/' ) { dirptr[ dirlen - 1 ] = '\0'; }

		if ( !strcmp ( dirptr, "." ) || !strcmp ( dirptr, ".." ) )
		{
			continue;
		}

		// iterate all skin files in directory
		numfiles = trap_FS_GetFileList ( va ( "models/players/%s", dirptr ), "tga", filelist, 2048 );
		fileptr = filelist;

		for ( j = 0; j < numfiles && uiInfo.q3HeadCount < MAX_PLAYERMODELS; j++, fileptr += filelen + 1 )
		{
			filelen = strlen ( fileptr );

			COM_StripExtension ( fileptr, skinname );

			// look for icon_????
			if ( Q_stricmpn ( skinname, "icon_", 5 ) == 0 && ! ( Q_stricmp ( skinname, "icon_blue" ) == 0 || Q_stricmp ( skinname, "icon_red" ) == 0 ) )
			{
				if ( Q_stricmp ( skinname, "icon_default" ) == 0 )
				{
					Q_strncpyz ( scratch, dirptr, sizeof ( scratch ) );
				}
				else
				{
					Com_sprintf ( scratch, sizeof ( scratch ), "%s/%s", dirptr, skinname + 5 );
				}

				dirty = 0;

				for ( k = 0; k < uiInfo.q3HeadCount; k++ )
				{
					if ( !Q_stricmp ( scratch, uiInfo.q3HeadNames[ uiInfo.q3HeadCount ] ) )
					{
						dirty = 1;
						break;
					}
				}

				if ( !dirty )
				{
					Q_strncpyz ( uiInfo.q3HeadNames[ uiInfo.q3HeadCount ], scratch, sizeof ( uiInfo.q3HeadNames[ uiInfo.q3HeadCount ] ) );
					uiInfo.q3HeadIcons[ uiInfo.q3HeadCount++ ] = trap_R_RegisterShaderNoMip ( va ( "models/players/%s/%s", dirptr, skinname ) );
				}
			}
		}
	}
}

/*
=================
UI_Init
=================
*/
void _UI_Init ( qboolean inGameLoad )
{
	const char *menuSet;
	int        start;

	BG_InitClassOverrides();
	BG_InitAllowedGameElements();

	//uiInfo.inGameLoad = inGameLoad;

	UI_RegisterCvars();
	UI_InitMemory();

	// cache redundant calulations
	trap_GetGlconfig ( &uiInfo.uiDC.glconfig );

	// for 640x480 virtualized screen
	uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * ( 1.0 / 480.0 );
	uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * ( 1.0 / 640.0 );

	if ( uiInfo.uiDC.glconfig.vidWidth * 480 > uiInfo.uiDC.glconfig.vidHeight * 640 )
	{
		// wide screen
		uiInfo.uiDC.bias = 0.5 * ( uiInfo.uiDC.glconfig.vidWidth - ( uiInfo.uiDC.glconfig.vidHeight * ( 640.0 / 480.0 ) ) );
	}
	else
	{
		// no wide screen
		uiInfo.uiDC.bias = 0;
	}

	//UI_Load();
	uiInfo.uiDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	uiInfo.uiDC.setColor = &UI_SetColor;
	uiInfo.uiDC.drawHandlePic = &UI_DrawHandlePic;
	uiInfo.uiDC.drawStretchPic = &trap_R_DrawStretchPic;
	uiInfo.uiDC.drawText = &Text_Paint;
	uiInfo.uiDC.textWidth = &Text_Width;
	uiInfo.uiDC.textHeight = &Text_Height;
	uiInfo.uiDC.registerModel = &trap_R_RegisterModel;
	uiInfo.uiDC.modelBounds = &trap_R_ModelBounds;
	uiInfo.uiDC.fillRect = &UI_FillRect;
	uiInfo.uiDC.drawRect = &_UI_DrawRect;
	uiInfo.uiDC.drawSides = &_UI_DrawSides;
	uiInfo.uiDC.drawTopBottom = &_UI_DrawTopBottom;
	uiInfo.uiDC.clearScene = &trap_R_ClearScene;
	uiInfo.uiDC.drawSides = &_UI_DrawSides;
	uiInfo.uiDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	uiInfo.uiDC.renderScene = &trap_R_RenderScene;
	uiInfo.uiDC.registerFont = &trap_R_RegisterFont;
	uiInfo.uiDC.ownerDrawItem = &UI_OwnerDraw;
	uiInfo.uiDC.getValue = &UI_GetValue;
	uiInfo.uiDC.ownerDrawVisible = &UI_OwnerDrawVisible;
	uiInfo.uiDC.runScript = &UI_RunMenuScript;
	uiInfo.uiDC.getTeamColor = &UI_GetTeamColor;
	uiInfo.uiDC.setCVar = trap_Cvar_Set;
	uiInfo.uiDC.getCVarString = trap_Cvar_VariableStringBuffer;
	uiInfo.uiDC.getCVarValue = trap_Cvar_VariableValue;
	uiInfo.uiDC.drawTextWithCursor = &Text_PaintWithCursor;
	uiInfo.uiDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	uiInfo.uiDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	uiInfo.uiDC.startLocalSound = &trap_S_StartLocalSound;
	uiInfo.uiDC.ownerDrawHandleKey = &UI_OwnerDrawHandleKey;
	uiInfo.uiDC.feederCount = &UI_FeederCount;
	uiInfo.uiDC.feederItemImage = &UI_FeederItemImage;
	uiInfo.uiDC.feederItemText = &UI_FeederItemText;
	uiInfo.uiDC.feederSelection = &UI_FeederSelection;
	uiInfo.uiDC.setBinding = &trap_Key_SetBinding;
	uiInfo.uiDC.getBindingBuf = &trap_Key_GetBindingBuf;
	uiInfo.uiDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	uiInfo.uiDC.executeText = &trap_Cmd_ExecuteText;
	uiInfo.uiDC.Error = &Com_Error;
	uiInfo.uiDC.Print = &Com_Printf;
	uiInfo.uiDC.Pause = &UI_Pause;
	uiInfo.uiDC.ownerDrawWidth = &UI_OwnerDrawWidth;
	uiInfo.uiDC.registerSound = &trap_S_RegisterSound;
	uiInfo.uiDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	uiInfo.uiDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	uiInfo.uiDC.playCinematic = &UI_PlayCinematic;
	uiInfo.uiDC.stopCinematic = &UI_StopCinematic;
	uiInfo.uiDC.drawCinematic = &UI_DrawCinematic;
	uiInfo.uiDC.runCinematicFrame = &UI_RunCinematicFrame;
	uiInfo.uiDC.translateString = &trap_TranslateString;

	Init_Display ( &uiInfo.uiDC );

	String_Init();

	uiInfo.uiDC.whiteShader = trap_R_RegisterShaderNoMip ( "white" );

	AssetCache();

	start = trap_Milliseconds();

	uiInfo.teamCount = 0;
	uiInfo.characterCount = 0;
	uiInfo.aliasCount = 0;

	/*  UI_ParseTeamInfo("teaminfo.txt");
	  UI_LoadTeams();
	  UI_ParseGameInfo("gameinfo.txt");*/

	menuSet = UI_Cvar_VariableString ( "ui_menuFiles" );

	if ( menuSet == NULL || menuSet[ 0 ] == '\0' )
	{
		menuSet = "ui/menus.txt";
	}

#if 0

	if ( uiInfo.inGameLoad )
	{
		UI_LoadMenus ( "ui/ingame.txt", qtrue );
	}
	else // bk010222: left this: UI_LoadMenus(menuSet, qtrue);
	{
	}

#else
	UI_LoadMenus ( menuSet, qtrue );
	UI_LoadMenus ( "ui/ingame.txt", qfalse );
	UI_LoadMenus ( "ui/tremulous.txt", qfalse );

	UI_LoadInfoPanes ( "ui/infopanes.def" );

	if ( uiInfo.uiDC.debug )
	{
		int i, j;

		for ( i = 0; i < uiInfo.tremInfoPaneCount; i++ )
		{
			Com_Printf ( "name: %s\n", uiInfo.tremInfoPanes[ i ].name );

			Com_Printf ( "text: %s\n", uiInfo.tremInfoPanes[ i ].text );

			for ( j = 0; j < uiInfo.tremInfoPanes[ i ].numGraphics; j++ )
			{
				Com_Printf ( "graphic %d: %d %d %d %d\n", j, uiInfo.tremInfoPanes[ i ].graphics[ j ].side,
				             uiInfo.tremInfoPanes[ i ].graphics[ j ].offset,
				             uiInfo.tremInfoPanes[ i ].graphics[ j ].width,
				             uiInfo.tremInfoPanes[ i ].graphics[ j ].height );
			}
		}
	}

#endif

	Menus_CloseAll();

	trap_LAN_LoadCachedServers();
	UI_LoadBestScores ( uiInfo.mapList[ ui_currentMap.integer ].mapLoadName, uiInfo.gameTypes[ ui_gameType.integer ].gtEnum );

	UI_BuildQ3Model_List();
	/*UI_LoadBots();*/

	// sets defaults for ui temp cvars
	uiInfo.effectsColor = gamecodetoui[ ( int ) trap_Cvar_VariableValue ( "color1" ) - 1 ];
	uiInfo.currentCrosshair = ( int ) trap_Cvar_VariableValue ( "cg_drawCrosshair" );
	trap_Cvar_Set ( "ui_mousePitch", ( trap_Cvar_VariableValue ( "m_pitch" ) >= 0 ) ? "0" : "1" );

	uiInfo.serverStatus.currentServerCinematic = -1;
	uiInfo.previewMovie = -1;

	if ( trap_Cvar_VariableValue ( "ui_TeamArenaFirstRun" ) == 0 )
	{
		trap_Cvar_Set ( "s_volume", "0.8" );
		trap_Cvar_Set ( "s_musicvolume", "0.5" );
		trap_Cvar_Set ( "ui_TeamArenaFirstRun", "1" );
	}

	trap_Cvar_Register ( NULL, "debug_protocol", "", 0 );

	trap_Cvar_Set ( "ui_actualNetGameType", va ( "%d", ui_netGameType.integer ) );

	// init Yes/No once for cl_language -> server browser
	Q_strncpyz ( translated_yes, DC->translateString ( "Yes" ), sizeof ( translated_yes ) );
	Q_strncpyz ( translated_no, DC->translateString ( "NO" ), sizeof ( translated_no ) );
}

/*
=================
UI_KeyEvent
=================
*/
void _UI_KeyEvent ( int key, qboolean down )
{
	if ( Menu_Count() > 0 )
	{
		menuDef_t *menu = Menu_GetFocused();

		if ( menu )
		{
			if ( key == K_ESCAPE && down && !Menus_AnyFullScreenVisible() )
			{
				Menus_CloseAll();
			}
			else
			{
				Menu_HandleKey ( menu, key, down );
			}
		}
		else
		{
			trap_Key_SetCatcher ( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			trap_Cvar_Set ( "cl_paused", "0" );
		}
	}

	//if ((s > 0) && (s != menu_null_sound)) {
	//  trap_S_StartLocalSound( s, CHAN_LOCAL_SOUND );
	//}
}

/*
=================
UI_MouseEvent
=================
*/
void _UI_MouseEvent ( int dx, int dy )
{
	// update mouse screen position
	uiInfo.uiDC.cursorx += dx;

	if ( uiInfo.uiDC.cursorx < 0 )
	{
		uiInfo.uiDC.cursorx = 0;
	}
	else if ( uiInfo.uiDC.cursorx > SCREEN_WIDTH )
	{
		uiInfo.uiDC.cursorx = SCREEN_WIDTH;
	}

	uiInfo.uiDC.cursory += dy;

	if ( uiInfo.uiDC.cursory < 0 )
	{
		uiInfo.uiDC.cursory = 0;
	}
	else if ( uiInfo.uiDC.cursory > SCREEN_HEIGHT )
	{
		uiInfo.uiDC.cursory = SCREEN_HEIGHT;
	}

	if ( Menu_Count() > 0 )
	{
		//menuDef_t *menu = Menu_GetFocused();
		//Menu_HandleMouseMove(menu, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
		Display_MouseMove ( NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory );
	}
}

void UI_LoadNonIngame ( void )
{
	const char *menuSet = UI_Cvar_VariableString ( "ui_menuFiles" );

	if ( menuSet == NULL || menuSet[ 0 ] == '\0' )
	{
		menuSet = "ui/menus.txt";
	}

	UI_LoadMenus ( menuSet, qfalse );
	uiInfo.inGameLoad = qfalse;
}

void _UI_SetActiveMenu ( uiMenuCommand_t menu )
{
	char buf[ 256 ];

	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	if ( Menu_Count() > 0 )
	{
		vec3_t v;
		v[ 0 ] = v[ 1 ] = v[ 2 ] = 0;

		switch ( menu )
		{
			case UIMENU_NONE:
				trap_Key_SetCatcher ( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set ( "cl_paused", "0" );
				Menus_CloseAll();

				return;

			case UIMENU_MAIN:
				//trap_Cvar_Set( "sv_killserver", "1" );
				trap_Key_SetCatcher ( KEYCATCH_UI );

				//trap_S_StartLocalSound( trap_S_RegisterSound("sound/misc/menu_background.wav", qfalse) , CHAN_LOCAL_SOUND );
				//trap_S_StartBackgroundTrack("sound/misc/menu_background.wav", NULL);
				if ( uiInfo.inGameLoad )
				{
					UI_LoadNonIngame();
				}

				Menus_CloseAll();
				Menus_ActivateByName ( "main" );
				trap_Cvar_VariableStringBuffer ( "com_errorMessage", buf, sizeof ( buf ) );

				if ( strlen ( buf ) )
				{
					if ( !ui_singlePlayerActive.integer )
					{
						Menus_ActivateByName ( "error_popmenu" );
					}
					else
					{
						trap_Cvar_Set ( "com_errorMessage", "" );
					}
				}

				return;

			case UIMENU_TEAM:
				trap_Key_SetCatcher ( KEYCATCH_UI );
				Menus_ActivateByName ( "team" );
				return;

			case UIMENU_POSTGAME:
				//trap_Cvar_Set( "sv_killserver", "1" );
				trap_Key_SetCatcher ( KEYCATCH_UI );

				if ( uiInfo.inGameLoad )
				{
					UI_LoadNonIngame();
				}

				Menus_CloseAll();
				Menus_ActivateByName ( "endofgame" );
				return;

			case UIMENU_INGAME:
				trap_Cvar_Set ( "cl_paused", "1" );
				trap_Key_SetCatcher ( KEYCATCH_UI );
				UI_BuildPlayerList();
				Menus_CloseAll();
				Menus_ActivateByName ( "ingame" );
				return;
		}
	}
}

qboolean _UI_IsFullscreen ( void )
{
	return Menus_AnyFullScreenVisible();
}

static connstate_t lastConnState;
static char        lastLoadingText[ MAX_INFO_VALUE ];

static void UI_ReadableSize ( char *buf, int bufsize, int value )
{
	if ( value > 1024 * 1024 * 1024 ) // gigs
	{
		Com_sprintf ( buf, bufsize, "%d", value / ( 1024 * 1024 * 1024 ) );
		Com_sprintf ( buf + strlen ( buf ), bufsize - strlen ( buf ), ".%02d GB",
		              ( value % ( 1024 * 1024 * 1024 ) ) * 100 / ( 1024 * 1024 * 1024 ) );
	}
	else if ( value > 1024 * 1024 ) // megs
	{
		Com_sprintf ( buf, bufsize, "%d", value / ( 1024 * 1024 ) );
		Com_sprintf ( buf + strlen ( buf ), bufsize - strlen ( buf ), ".%02d MB",
		              ( value % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
	}
	else if ( value > 1024 ) // kilos
	{
		Com_sprintf ( buf, bufsize, "%d KB", value / 1024 );
	}
	else // bytes
	{
		Com_sprintf ( buf, bufsize, "%d bytes", value );
	}
}

// Assumes time is in msec
static void UI_PrintTime ( char *buf, int bufsize, int time )
{
	time /= 1000; // change to seconds

	if ( time > 3600 ) // in the hours range
	{
		Com_sprintf ( buf, bufsize, "%d hr %d min", time / 3600, ( time % 3600 ) / 60 );
	}
	else if ( time > 60 ) // mins
	{
		Com_sprintf ( buf, bufsize, "%d min %d sec", time / 60, time % 60 );
	}
	else // secs
	{
		Com_sprintf ( buf, bufsize, "%d sec", time );
	}
}

void Text_PaintCenter ( float x, float y, float scale, vec4_t color, const char *text, float adjust )
{
	int len = Text_Width ( text, scale, 0 );
	Text_Paint ( x - len / 2, y, scale, color, text, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );
}

void Text_PaintCenter_AutoWrapped ( float x, float y, float xmax, float ystep, float scale, vec4_t color, const char *str, float adjust )
{
	int  width;
	char *s1, *s2, *s3;
	char c_bcp;
	char buf[ 1024 ];

	if ( !str || str[ 0 ] == '\0' )
	{
		return;
	}

	Q_strncpyz ( buf, str, sizeof ( buf ) );
	s1 = s2 = s3 = buf;

	while ( 1 )
	{
		do
		{
			s3++;
		}
		while ( *s3 != ' ' && *s3 != '\0' );

		c_bcp = *s3;
		*s3 = '\0';
		width = Text_Width ( s1, scale, 0 );
		*s3 = c_bcp;

		if ( width > xmax )
		{
			if ( s1 == s2 )
			{
				// fuck, don't have a clean cut, we'll overflow
				s2 = s3;
			}

			*s2 = '\0';
			Text_PaintCenter ( x, y, scale, color, s1, adjust );
			y += ystep;

			if ( c_bcp == '\0' )
			{
				// that was the last word
				// we could start a new loop, but that wouldn't be much use
				// even if the word is too long, we would overflow it (see above)
				// so just print it now if needed
				s2++;

				if ( *s2 != '\0' ) // if we are printing an overflowing line we have s2 == s3
				{
					Text_PaintCenter ( x, y, scale, color, s2, adjust );
				}

				break;
			}

			s2++;
			s1 = s2;
			s3 = s2;
		}
		else
		{
			s2 = s3;

			if ( c_bcp == '\0' ) // we reached the end
			{
				Text_PaintCenter ( x, y, scale, color, s1, adjust );
				break;
			}
		}
	}
}

static void UI_DisplayDownloadInfo ( const char *downloadName, float centerPoint, float yStart, float scale )
{
	static char dlText[] = "Downloading:";
	static char etaText[] = "Estimated time left:";
	static char xferText[] = "Transfer rate:";

	int         downloadSize, downloadCount, downloadTime;
	char        dlSizeBuf[ 64 ], totalSizeBuf[ 64 ], xferRateBuf[ 64 ], dlTimeBuf[ 64 ];
	int         xferRate;
	int         leftWidth;
	const char  *s;

	downloadSize = trap_Cvar_VariableValue ( "cl_downloadSize" );
	downloadCount = trap_Cvar_VariableValue ( "cl_downloadCount" );
	downloadTime = trap_Cvar_VariableValue ( "cl_downloadTime" );

	leftWidth = 320;

	UI_SetColor ( colorWhite );
	Text_PaintCenter ( centerPoint, yStart + 112, scale, colorWhite, dlText, 0 );
	Text_PaintCenter ( centerPoint, yStart + 192, scale, colorWhite, etaText, 0 );
	Text_PaintCenter ( centerPoint, yStart + 248, scale, colorWhite, xferText, 0 );

	if ( downloadSize > 0 )
	{
		s = va ( "%s (%d%%)", downloadName, downloadCount * 100 / downloadSize );
	}
	else
	{
		s = downloadName;
	}

	Text_PaintCenter ( centerPoint, yStart + 136, scale, colorWhite, s, 0 );

	UI_ReadableSize ( dlSizeBuf,   sizeof dlSizeBuf,   downloadCount );
	UI_ReadableSize ( totalSizeBuf,  sizeof totalSizeBuf,  downloadSize );

	if ( downloadCount < 4096 || !downloadTime )
	{
		Text_PaintCenter ( leftWidth, yStart + 216, scale, colorWhite, "estimating", 0 );
		Text_PaintCenter ( leftWidth, yStart + 160, scale, colorWhite, va ( "(%s of %s copied)", dlSizeBuf, totalSizeBuf ), 0 );
	}
	else
	{
		if ( ( uiInfo.uiDC.realTime - downloadTime ) / 1000 )
		{
			xferRate = downloadCount / ( ( uiInfo.uiDC.realTime - downloadTime ) / 1000 );
		}
		else
		{
			xferRate = 0;
		}

		UI_ReadableSize ( xferRateBuf, sizeof xferRateBuf, xferRate );

		// Extrapolate estimated completion time
		if ( downloadSize && xferRate )
		{
			int n = downloadSize / xferRate; // estimated time for entire d/l in secs

			// We do it in K (/1024) because we'd overflow around 4MB
			UI_PrintTime ( dlTimeBuf, sizeof dlTimeBuf,
			               ( n - ( ( ( downloadCount / 1024 ) * n ) / ( downloadSize / 1024 ) ) ) * 1000 );

			Text_PaintCenter ( leftWidth, yStart + 216, scale, colorWhite, dlTimeBuf, 0 );
			Text_PaintCenter ( leftWidth, yStart + 160, scale, colorWhite, va ( "(%s of %s copied)", dlSizeBuf, totalSizeBuf ), 0 );
		}
		else
		{
			Text_PaintCenter ( leftWidth, yStart + 216, scale, colorWhite, "estimating", 0 );

			if ( downloadSize )
			{
				Text_PaintCenter ( leftWidth, yStart + 160, scale, colorWhite, va ( "(%s of %s copied)", dlSizeBuf, totalSizeBuf ), 0 );
			}
			else
			{
				Text_PaintCenter ( leftWidth, yStart + 160, scale, colorWhite, va ( "(%s copied)", dlSizeBuf ), 0 );
			}
		}

		if ( xferRate )
		{
			Text_PaintCenter ( leftWidth, yStart + 272, scale, colorWhite, va ( "%s/Sec", xferRateBuf ), 0 );
		}
	}
}

/*
========================
UI_DrawConnectScreen

This will also be overlaid on the cgame info screen during loading
to prevent it from blinking away too rapidly on local or lan games.
========================
*/
void UI_DrawConnectScreen ( qboolean overlay )
{
	char            *s;
	uiClientState_t cstate;
	char            info[ MAX_INFO_VALUE ];
	char            text[ 256 ];
	float           centerPoint, yStart, scale;

	menuDef_t       *menu = Menus_FindByName ( "Connect" );

	if ( !overlay && menu )
	{
		Menu_Paint ( menu, qtrue );
	}

	if ( !overlay )
	{
		centerPoint = 320;
		yStart = 130;
		scale = 0.5f;
	}
	else
	{
		centerPoint = 320;
		yStart = 32;
		scale = 0.6f;
		return;
	}

	// see what information we should display
	trap_GetClientState ( &cstate );

	info[ 0 ] = '\0';

	if ( trap_GetConfigString ( CS_SERVERINFO, info, sizeof ( info ) ) )
	{
		Text_PaintCenter ( centerPoint, yStart, scale, colorWhite, va ( "Loading %s", Info_ValueForKey ( info, "mapname" ) ), 0 );
	}

	if ( !Q_stricmp ( cstate.servername, "localhost" ) )
	{
		Text_PaintCenter ( centerPoint, yStart + 48, scale, colorWhite, va ( "Starting up..." ), ITEM_TEXTSTYLE_SHADOWEDMORE );
	}
	else
	{
		strcpy ( text, va ( "Connecting to %s", cstate.servername ) );
		Text_PaintCenter ( centerPoint, yStart + 48, scale, colorWhite, text, ITEM_TEXTSTYLE_SHADOWEDMORE );
	}

	// display global MOTD at bottom
	Text_PaintCenter ( centerPoint, 600, scale, colorWhite, Info_ValueForKey ( cstate.updateInfoString, "motd" ), 0 );

	// print any server info (server full, bad version, etc)
	if ( cstate.connState < CA_CONNECTED )
	{
		Text_PaintCenter_AutoWrapped ( centerPoint, yStart + 176, 630, 20, scale, colorWhite, cstate.messageString, 0 );
	}

	if ( lastConnState > cstate.connState )
	{
		lastLoadingText[ 0 ] = '\0';
	}

	lastConnState = cstate.connState;

	switch ( cstate.connState )
	{
		case CA_CONNECTING:
			s = va ( "Awaiting connection...%i", cstate.connectPacketCount );
			break;

		case CA_CHALLENGING:
			s = va ( "Awaiting challenge...%i", cstate.connectPacketCount );
			break;

		case CA_CONNECTED:
			{
				char downloadName[ MAX_INFO_VALUE ];

				trap_Cvar_VariableStringBuffer ( "cl_downloadName", downloadName, sizeof ( downloadName ) );

				if ( *downloadName )
				{
					UI_DisplayDownloadInfo ( downloadName, centerPoint, yStart, scale );
					return;
				}
			}

			s = "Awaiting gamestate...";
			break;

		case CA_LOADING:
			return;

		case CA_PRIMED:
			return;

		default:
			return;
	}

	if ( Q_stricmp ( cstate.servername, "localhost" ) )
	{
		Text_PaintCenter ( centerPoint, yStart + 80, scale, colorWhite, s, 0 );
	}

	// password required / connection rejected information goes here
}

/*
================
cvars
================
*/

typedef struct
{
	vmCvar_t *vmCvar;
	char     *cvarName;
	char     *defaultString;
	int      cvarFlags;
} cvarTable_t;

vmCvar_t           ui_ffa_fraglimit;
vmCvar_t           ui_ffa_timelimit;

vmCvar_t           ui_tourney_fraglimit;
vmCvar_t           ui_tourney_timelimit;

vmCvar_t           ui_team_fraglimit;
vmCvar_t           ui_team_timelimit;
vmCvar_t           ui_team_friendly;

vmCvar_t           ui_ctf_capturelimit;
vmCvar_t           ui_ctf_timelimit;
vmCvar_t           ui_ctf_friendly;

vmCvar_t           ui_arenasFile;
vmCvar_t           ui_botsFile;
vmCvar_t           ui_spScores1;
vmCvar_t           ui_spScores2;
vmCvar_t           ui_spScores3;
vmCvar_t           ui_spScores4;
vmCvar_t           ui_spScores5;
vmCvar_t           ui_spAwards;
vmCvar_t           ui_spVideos;
vmCvar_t           ui_spSkill;

vmCvar_t           ui_spSelection;

vmCvar_t           ui_browserMaster;
vmCvar_t           ui_browserGameType;
vmCvar_t           ui_browserSortKey;
vmCvar_t           ui_browserShowFull;
vmCvar_t           ui_browserShowEmpty;

vmCvar_t           ui_brassTime;
vmCvar_t           ui_drawCrosshair;
vmCvar_t           ui_drawCrosshairNames;
vmCvar_t           ui_marks;

vmCvar_t           ui_server1;
vmCvar_t           ui_server2;
vmCvar_t           ui_server3;
vmCvar_t           ui_server4;
vmCvar_t           ui_server5;
vmCvar_t           ui_server6;
vmCvar_t           ui_server7;
vmCvar_t           ui_server8;
vmCvar_t           ui_server9;
vmCvar_t           ui_server10;
vmCvar_t           ui_server11;
vmCvar_t           ui_server12;
vmCvar_t           ui_server13;
vmCvar_t           ui_server14;
vmCvar_t           ui_server15;
vmCvar_t           ui_server16;

vmCvar_t           ui_redteam;
vmCvar_t           ui_redteam1;
vmCvar_t           ui_redteam2;
vmCvar_t           ui_redteam3;
vmCvar_t           ui_redteam4;
vmCvar_t           ui_redteam5;
vmCvar_t           ui_blueteam;
vmCvar_t           ui_blueteam1;
vmCvar_t           ui_blueteam2;
vmCvar_t           ui_blueteam3;
vmCvar_t           ui_blueteam4;
vmCvar_t           ui_blueteam5;
vmCvar_t           ui_teamName;
vmCvar_t           ui_dedicated;
vmCvar_t           ui_gameType;
vmCvar_t           ui_netGameType;
vmCvar_t           ui_actualNetGameType;
vmCvar_t           ui_joinGameType;
vmCvar_t           ui_netSource;
vmCvar_t           ui_serverFilterType;
vmCvar_t           ui_opponentName;
vmCvar_t           ui_menuFiles;
vmCvar_t           ui_currentTier;
vmCvar_t           ui_currentMap;
vmCvar_t           ui_currentNetMap;
vmCvar_t           ui_mapIndex;
vmCvar_t           ui_currentOpponent;
vmCvar_t           ui_selectedPlayer;
vmCvar_t           ui_selectedPlayerName;
vmCvar_t           ui_lastServerRefresh_0;
vmCvar_t           ui_lastServerRefresh_1;
vmCvar_t           ui_lastServerRefresh_2;
vmCvar_t           ui_lastServerRefresh_3;
vmCvar_t           ui_singlePlayerActive;
vmCvar_t           ui_scoreAccuracy;
vmCvar_t           ui_scoreImpressives;
vmCvar_t           ui_scoreExcellents;
vmCvar_t           ui_scoreCaptures;
vmCvar_t           ui_scoreDefends;
vmCvar_t           ui_scoreAssists;
vmCvar_t           ui_scoreGauntlets;
vmCvar_t           ui_scoreScore;
vmCvar_t           ui_scorePerfect;
vmCvar_t           ui_scoreTeam;
vmCvar_t           ui_scoreBase;
vmCvar_t           ui_scoreTimeBonus;
vmCvar_t           ui_scoreSkillBonus;
vmCvar_t           ui_scoreShutoutBonus;
vmCvar_t           ui_scoreTime;
vmCvar_t           ui_captureLimit;
vmCvar_t           ui_fragLimit;
vmCvar_t           ui_smallFont;
vmCvar_t           ui_bigFont;
vmCvar_t           ui_findPlayer;
vmCvar_t           ui_Q3Model;
vmCvar_t           ui_hudFiles;
vmCvar_t           ui_recordSPDemo;
vmCvar_t           ui_realCaptureLimit;
vmCvar_t           ui_realWarmUp;
vmCvar_t           ui_serverStatusTimeOut;

//TA: bank values
vmCvar_t           ui_bank;

// bk001129 - made static to avoid aliasing
static cvarTable_t cvarTable[] =
{
	{ &ui_ffa_fraglimit,       "ui_ffa_fraglimit",       "20",           CVAR_ARCHIVE                                    },
	{ &ui_ffa_timelimit,       "ui_ffa_timelimit",       "0",            CVAR_ARCHIVE                                    },

	{ &ui_tourney_fraglimit,   "ui_tourney_fraglimit",   "0",            CVAR_ARCHIVE                                    },
	{ &ui_tourney_timelimit,   "ui_tourney_timelimit",   "15",           CVAR_ARCHIVE                                    },

	{ &ui_team_fraglimit,      "ui_team_fraglimit",      "0",            CVAR_ARCHIVE                                    },
	{ &ui_team_timelimit,      "ui_team_timelimit",      "20",           CVAR_ARCHIVE                                    },
	{ &ui_team_friendly,       "ui_team_friendly",       "1",            CVAR_ARCHIVE                                    },

	{ &ui_ctf_capturelimit,    "ui_ctf_capturelimit",    "8",            CVAR_ARCHIVE                                    },
	{ &ui_ctf_timelimit,       "ui_ctf_timelimit",       "30",           CVAR_ARCHIVE                                    },
	{ &ui_ctf_friendly,        "ui_ctf_friendly",        "0",            CVAR_ARCHIVE                                    },

	{ &ui_arenasFile,          "g_arenasFile",           "",             CVAR_INIT | CVAR_ROM                            },
	{ &ui_botsFile,            "g_botsFile",             "",             CVAR_INIT | CVAR_ROM                            },
	{ &ui_spScores1,           "g_spScores1",            "",             CVAR_ARCHIVE | CVAR_ROM                         },
	{ &ui_spScores2,           "g_spScores2",            "",             CVAR_ARCHIVE | CVAR_ROM                         },
	{ &ui_spScores3,           "g_spScores3",            "",             CVAR_ARCHIVE | CVAR_ROM                         },
	{ &ui_spScores4,           "g_spScores4",            "",             CVAR_ARCHIVE | CVAR_ROM                         },
	{ &ui_spScores5,           "g_spScores5",            "",             CVAR_ARCHIVE | CVAR_ROM                         },
	{ &ui_spAwards,            "g_spAwards",             "",             CVAR_ARCHIVE | CVAR_ROM                         },
	{ &ui_spVideos,            "g_spVideos",             "",             CVAR_ARCHIVE | CVAR_ROM                         },
	{ &ui_spSkill,             "g_spSkill",              "2",            CVAR_ARCHIVE                                    },

	{ &ui_spSelection,         "ui_spSelection",         "",             CVAR_ROM                                        },

	{ &ui_browserMaster,       "ui_browserMaster",       "0",            CVAR_ARCHIVE                                    },
	{ &ui_browserGameType,     "ui_browserGameType",     "0",            CVAR_ARCHIVE                                    },
	{ &ui_browserSortKey,      "ui_browserSortKey",      "4",            CVAR_ARCHIVE                                    },
	{ &ui_browserShowFull,     "ui_browserShowFull",     "1",            CVAR_ARCHIVE                                    },
	{ &ui_browserShowEmpty,    "ui_browserShowEmpty",    "1",            CVAR_ARCHIVE                                    },

	{ &ui_brassTime,           "cg_brassTime",           "2500",         CVAR_ARCHIVE                                    },
	{ &ui_drawCrosshair,       "cg_drawCrosshair",       "4",            CVAR_ARCHIVE                                    },
	{ &ui_drawCrosshairNames,  "cg_drawCrosshairNames",  "1",            CVAR_ARCHIVE                                    },
	{ &ui_marks,               "cg_marks",               "1",            CVAR_ARCHIVE                                    },

	{ &ui_server1,             "server1",                "",             CVAR_ARCHIVE                                    },
	{ &ui_server2,             "server2",                "",             CVAR_ARCHIVE                                    },
	{ &ui_server3,             "server3",                "",             CVAR_ARCHIVE                                    },
	{ &ui_server4,             "server4",                "",             CVAR_ARCHIVE                                    },
	{ &ui_server5,             "server5",                "",             CVAR_ARCHIVE                                    },
	{ &ui_server6,             "server6",                "",             CVAR_ARCHIVE                                    },
	{ &ui_server7,             "server7",                "",             CVAR_ARCHIVE                                    },
	{ &ui_server8,             "server8",                "",             CVAR_ARCHIVE                                    },
	{ &ui_server9,             "server9",                "",             CVAR_ARCHIVE                                    },
	{ &ui_server10,            "server10",               "",             CVAR_ARCHIVE                                    },
	{ &ui_server11,            "server11",               "",             CVAR_ARCHIVE                                    },
	{ &ui_server12,            "server12",               "",             CVAR_ARCHIVE                                    },
	{ &ui_server13,            "server13",               "",             CVAR_ARCHIVE                                    },
	{ &ui_server14,            "server14",               "",             CVAR_ARCHIVE                                    },
	{ &ui_server15,            "server15",               "",             CVAR_ARCHIVE                                    },
	{ &ui_server16,            "server16",               "",             CVAR_ARCHIVE                                    },
	{ &ui_new,                 "ui_new",                 "0",            CVAR_TEMP                                       },
	{ &ui_debug,               "ui_debug",               "0",            CVAR_TEMP                                       },
	{ &ui_initialized,         "ui_initialized",         "0",            CVAR_TEMP                                       },
	{ &ui_teamName,            "ui_teamName",            "Pagans",       CVAR_ARCHIVE                                    },
	{ &ui_opponentName,        "ui_opponentName",        "Stroggs",      CVAR_ARCHIVE                                    },
	{ &ui_redteam,             "ui_redteam",             "Pagans",       CVAR_ARCHIVE                                    },
	{ &ui_blueteam,            "ui_blueteam",            "Stroggs",      CVAR_ARCHIVE                                    },
	{ &ui_dedicated,           "ui_dedicated",           "0",            CVAR_ARCHIVE                                    },
	{ &ui_gameType,            "ui_gametype",            "3",            CVAR_ARCHIVE                                    },
	{ &ui_joinGameType,        "ui_joinGametype",        "0",            CVAR_ARCHIVE                                    },
	{ &ui_netGameType,         "ui_netGametype",         "3",            CVAR_ARCHIVE                                    },
	{ &ui_actualNetGameType,   "ui_actualNetGametype",   "3",            CVAR_ARCHIVE                                    },
	{ &ui_redteam1,            "ui_redteam1",            "0",            CVAR_ARCHIVE                                    },
	{ &ui_redteam2,            "ui_redteam2",            "0",            CVAR_ARCHIVE                                    },
	{ &ui_redteam3,            "ui_redteam3",            "0",            CVAR_ARCHIVE                                    },
	{ &ui_redteam4,            "ui_redteam4",            "0",            CVAR_ARCHIVE                                    },
	{ &ui_redteam5,            "ui_redteam5",            "0",            CVAR_ARCHIVE                                    },
	{ &ui_blueteam1,           "ui_blueteam1",           "0",            CVAR_ARCHIVE                                    },
	{ &ui_blueteam2,           "ui_blueteam2",           "0",            CVAR_ARCHIVE                                    },
	{ &ui_blueteam3,           "ui_blueteam3",           "0",            CVAR_ARCHIVE                                    },
	{ &ui_blueteam4,           "ui_blueteam4",           "0",            CVAR_ARCHIVE                                    },
	{ &ui_blueteam5,           "ui_blueteam5",           "0",            CVAR_ARCHIVE                                    },
	{ &ui_netSource,           "ui_netSource",           "0",            CVAR_ARCHIVE                                    },
	{ &ui_menuFiles,           "ui_menuFiles",           "ui/menus.txt", CVAR_ARCHIVE                                    },
	{ &ui_currentTier,         "ui_currentTier",         "0",            CVAR_ARCHIVE                                    },
	{ &ui_currentMap,          "ui_currentMap",          "0",            CVAR_ARCHIVE                                    },
	{ &ui_currentNetMap,       "ui_currentNetMap",       "0",            CVAR_ARCHIVE                                    },
	{ &ui_mapIndex,            "ui_mapIndex",            "0",            CVAR_ARCHIVE                                    },
	{ &ui_currentOpponent,     "ui_currentOpponent",     "0",            CVAR_ARCHIVE                                    },
	{ &ui_selectedPlayer,      "cg_selectedPlayer",      "0",            CVAR_ARCHIVE                                    },
	{ &ui_selectedPlayerName,  "cg_selectedPlayerName",  "",             CVAR_ARCHIVE                                    },
	{ &ui_lastServerRefresh_0, "ui_lastServerRefresh_0", "",             CVAR_ARCHIVE                                    },
	{ &ui_lastServerRefresh_1, "ui_lastServerRefresh_1", "",             CVAR_ARCHIVE                                    },
	{ &ui_lastServerRefresh_2, "ui_lastServerRefresh_2", "",             CVAR_ARCHIVE                                    },
	{ &ui_lastServerRefresh_3, "ui_lastServerRefresh_3", "",             CVAR_ARCHIVE                                    },
	{ &ui_singlePlayerActive,  "ui_singlePlayerActive",  "0",            0                                               },
	{ &ui_scoreAccuracy,       "ui_scoreAccuracy",       "0",            CVAR_ARCHIVE                                    },
	{ &ui_scoreImpressives,    "ui_scoreImpressives",    "0",            CVAR_ARCHIVE                                    },
	{ &ui_scoreExcellents,     "ui_scoreExcellents",     "0",            CVAR_ARCHIVE                                    },
	{ &ui_scoreCaptures,       "ui_scoreCaptures",       "0",            CVAR_ARCHIVE                                    },
	{ &ui_scoreDefends,        "ui_scoreDefends",        "0",            CVAR_ARCHIVE                                    },
	{ &ui_scoreAssists,        "ui_scoreAssists",        "0",            CVAR_ARCHIVE                                    },
	{ &ui_scoreGauntlets,      "ui_scoreGauntlets",      "0",            CVAR_ARCHIVE                                    },
	{ &ui_scoreScore,          "ui_scoreScore",          "0",            CVAR_ARCHIVE                                    },
	{ &ui_scorePerfect,        "ui_scorePerfect",        "0",            CVAR_ARCHIVE                                    },
	{ &ui_scoreTeam,           "ui_scoreTeam",           "0 to 0",       CVAR_ARCHIVE                                    },
	{ &ui_scoreBase,           "ui_scoreBase",           "0",            CVAR_ARCHIVE                                    },
	{ &ui_scoreTime,           "ui_scoreTime",           "00:00",        CVAR_ARCHIVE                                    },
	{ &ui_scoreTimeBonus,      "ui_scoreTimeBonus",      "0",            CVAR_ARCHIVE                                    },
	{ &ui_scoreSkillBonus,     "ui_scoreSkillBonus",     "0",            CVAR_ARCHIVE                                    },
	{ &ui_scoreShutoutBonus,   "ui_scoreShutoutBonus",   "0",            CVAR_ARCHIVE                                    },
	{ &ui_fragLimit,           "ui_fragLimit",           "10",           0                                               },
	{ &ui_captureLimit,        "ui_captureLimit",        "5",            0                                               },
	{ &ui_smallFont,           "ui_smallFont",           "0.2",          CVAR_ARCHIVE                                    },
	{ &ui_bigFont,             "ui_bigFont",             "0.5",          CVAR_ARCHIVE                                    },
	{ &ui_findPlayer,          "ui_findPlayer",          "Sarge",        CVAR_ARCHIVE                                    },
	{ &ui_Q3Model,             "ui_q3model",             "0",            CVAR_ARCHIVE                                    },
	{ &ui_hudFiles,            "cg_hudFiles",            "ui/hud.txt",   CVAR_ARCHIVE                                    },
	{ &ui_recordSPDemo,        "ui_recordSPDemo",        "0",            CVAR_ARCHIVE                                    },
	{ &ui_teamArenaFirstRun,   "ui_teamArenaFirstRun",   "0",            CVAR_ARCHIVE                                    },
	{ &ui_realWarmUp,          "g_warmup",               "20",           CVAR_ARCHIVE                                    },
	{ &ui_realCaptureLimit,    "capturelimit",           "8",            CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART },
	{ &ui_serverStatusTimeOut, "ui_serverStatusTimeOut", "7000",         CVAR_ARCHIVE                                    },

	{ &ui_bank,                "ui_bank",                "0",            0                                               },
};

// bk001129 - made static to avoid aliasing
static int         cvarTableSize = sizeof ( cvarTable ) / sizeof ( cvarTable[ 0 ] );

/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars ( void )
{
	int         i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
	{
		trap_Cvar_Register ( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
	}
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars ( void )
{
	int         i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
	{
		trap_Cvar_Update ( cv->vmCvar );
	}
}

/*
=================
ArenaServers_StopRefresh
=================
*/
static void UI_StopServerRefresh ( void )
{
	int count;

	if ( !uiInfo.serverStatus.refreshActive )
	{
		// not currently refreshing
		return;
	}

	uiInfo.serverStatus.refreshActive = qfalse;
	Com_Printf ( "%d servers listed in browser with %d players.\n",
	             uiInfo.serverStatus.numDisplayServers,
	             uiInfo.serverStatus.numPlayersOnServers );
	count = trap_LAN_GetServerCount ( ui_netSource.integer );

	if ( count - uiInfo.serverStatus.numDisplayServers > 0 )
	{
		Com_Printf ( "%d servers not listed due to packet loss or pings higher than %d\n",
		             count - uiInfo.serverStatus.numDisplayServers,
		             ( int ) trap_Cvar_VariableValue ( "cl_maxPing" ) );
	}
}

/*
=================
UI_DoServerRefresh
=================
*/
static void UI_DoServerRefresh ( void )
{
	qboolean wait = qfalse;

	if ( !uiInfo.serverStatus.refreshActive )
	{
		return;
	}

	if ( ui_netSource.integer != AS_FAVORITES )
	{
		if ( ui_netSource.integer == AS_LOCAL )
		{
			if ( !trap_LAN_GetServerCount ( ui_netSource.integer ) )
			{
				wait = qtrue;
			}
		}
		else
		{
			if ( trap_LAN_GetServerCount ( ui_netSource.integer ) < 0 )
			{
				wait = qtrue;
			}
		}
	}

	if ( uiInfo.uiDC.realTime < uiInfo.serverStatus.refreshtime )
	{
		if ( wait )
		{
			return;
		}
	}

	// if still trying to retrieve pings
	if ( trap_LAN_UpdateVisiblePings ( ui_netSource.integer ) )
	{
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
	}
	else if ( !wait )
	{
		// get the last servers in the list
		UI_BuildServerDisplayList ( 2 );
		// stop the refresh
		UI_StopServerRefresh();
	}

	//
	UI_BuildServerDisplayList ( qfalse );
}

/*
=================
UI_StartServerRefresh
=================
*/
static void UI_StartServerRefresh ( qboolean full )
{
	int     i;
	char    *ptr;

	qtime_t q;
	trap_RealTime ( &q );
	trap_Cvar_Set ( va ( "ui_lastServerRefresh_%i", ui_netSource.integer ),
	                va ( "%04i-%02i-%02i %02i:%02i:%02i",
	                     1900 + q.tm_year, q.tm_mon + 1, q.tm_mday,
	                     q.tm_hour, q.tm_min, q.tm_sec ) );

	if ( !full )
	{
		UI_UpdatePendingPings();
		return;
	}

	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 1000;
	// clear number of displayed servers
	uiInfo.serverStatus.numDisplayServers = 0;
	uiInfo.serverStatus.numPlayersOnServers = 0;
	// mark all servers as visible so we store ping updates for them
	trap_LAN_MarkServerVisible ( ui_netSource.integer, -1, qtrue );
	// reset all the pings
	trap_LAN_ResetPings ( ui_netSource.integer );

	//
	if ( ui_netSource.integer == AS_LOCAL )
	{
		trap_Cmd_ExecuteText ( EXEC_NOW, "localservers\n" );
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
		return;
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 5000;

	if ( ui_netSource.integer == AS_GLOBAL )
	{
		if ( ui_netSource.integer == AS_GLOBAL )
		{
			i = 0;
		}
		else
		{
			i = 1;
		}

		ptr = UI_Cvar_VariableString ( "debug_protocol" );

		if ( strlen ( ptr ) )
		{
			trap_Cmd_ExecuteText ( EXEC_NOW, va ( "globalservers %d %s full empty\n", i, ptr ) );
		}
		else
		{
			trap_Cmd_ExecuteText ( EXEC_NOW, va ( "globalservers %d %d full empty\n", i, ( int ) trap_Cvar_VariableValue ( "protocol" ) ) );
		}
	}
}
