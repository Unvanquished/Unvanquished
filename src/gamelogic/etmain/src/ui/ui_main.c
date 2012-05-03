/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/

#include "git_version.h"

#include "ui_local.h"

// NERVE - SMF
#define AXIS_TEAM   0
#define ALLIES_TEAM 1
#define SPECT_TEAM  2
// -NERVE - SMF

extern qboolean             g_waitingForKey;
extern qboolean             g_editingField;
extern itemDef_t            *g_editItem;

uiInfo_t                    uiInfo;

static const serverFilter_t serverFilters[] =
{
	{ "All", "" }
};

// For server browser

/*static const char *ETGameTypes[] = {
        "Single Player",
        "Cooperative",
        "Objective",
        "Stopwatch",
        "Campaign",
        "Last Man Standing"
};

static const char *shortETGameTypes[] = {
        "SP",
        "Coop",
        "Obj",
        "SW",
        "Cmpgn",
        "LMS"
};

static int const numETGameTypes = sizeof(ETGameTypes) / sizeof(const char*);*/

static const int  numServerFilters = sizeof( serverFilters ) / sizeof( serverFilter_t );

static char       *netnames[] =
{
	"???",
	"UDP",
	"IPX",
	NULL
};

static int        gamecodetoui[] = { 4, 2, 3, 0, 5, 1, 6 };
static int        uitogamecode[] = { 4, 6, 2, 3, 1, 5, 7 };

// NERVE - SMF - enabled for multiplayer
static void       UI_StartServerRefresh( qboolean full );
static void       UI_StopServerRefresh( void );
static void       UI_DoServerRefresh( void );
static void       UI_FeederSelection( float feederID, int index );
qboolean          UI_FeederSelectionClick( itemDef_t *item );
static void       UI_BuildServerDisplayList( qboolean force );
static void       UI_BuildServerStatus( qboolean force );
static void       UI_BuildFindPlayerList( qboolean force );
static int QDECL  UI_ServersQsortCompare( const void *arg1, const void *arg2 );
static int        UI_MapCountByGameType( qboolean singlePlayer );
static const char *UI_SelectedMap( qboolean singlePlayer, int index, int *actual );
static int        UI_GetIndexFromSelection( int actual );

static const char *UI_SelectedCampaign( int index, int *actual );
static int        UI_CampaignCount( qboolean singlePlayer );

qboolean          UI_CheckExecKey( int key );

// -NERVE - SMF - enabled for multiplayer

static void UI_ParseGameInfo( const char *teamFile );

//static void UI_ParseTeamInfo(const char *teamFile); // TTimo: unused

//int ProcessNewUI( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6 );

itemDef_t *Menu_FindItemByName( menuDef_t *menu, const char *p );
void      Menu_ShowItemByName( menuDef_t *menu, const char *p, qboolean bShow );

#define ITEM_GRENADES 1
#define ITEM_MEDKIT   2

#define ITEM_PISTOL   1

#define DEFAULT_PISTOL

#define PT_KNIFE      ( 1 )
#define PT_PISTOL     ( 1 << 2 )
#define PT_RIFLE      ( 1 << 3 )
#define PT_LIGHTONLY  ( 1 << 4 )
#define PT_GRENADES   ( 1 << 5 )
#define PT_EXPLOSIVES ( 1 << 6 )
#define PT_MEDKIT     ( 1 << 7 )

// TTimo
static char translated_yes[ 4 ], translated_no[ 4 ];

/*typedef struct {
        int     weapindex;

        const char  *desc;
//  int     flags;
        const char  *cvar;
        int     value;
        const char  *shadername;

        const char  *torso_anim;
        const char  *legs_anim;

//  const char  *large_shader;
} weaponType_t;*/

#define ENG_WEAPMASK_1 ( 0 | 1 | 2 )
#define ENG_WEAPMASK_2 ( 4 | 8 )

// NERVE - SMF - this is the weapon info list [what can and can't be used by character classes]
//   - This list is seperate from the actual text names in the listboxes for localization purposes.
//   - The list boxes look up this list by the cvar value.
// Gordon: stripped out some useless stuff, and moved some other stuff to generic class stuff

/*static weaponType_t weaponTypes[] = {
        { 0,          "NULL",           "none",     -1, "none",                 "",           ""            },

        { WP_COLT,        "1911 pistol",        "mp_weapon",  -1, "ui/assets/weapon_colt1911.tga",    "firing_pistolB_1",   "stand_pistolB"     },
        { WP_LUGER,       "Luger pistol",       "mp_weapon",  -1, "ui/assets/weapon_luger.tga",     "firing_pistolB_1",   "stand_pistolB"     },

        { WP_MP40,        "MP 40",          "mp_weapon",  0,  "ui/assets/weapon_mp40.tga",      "relaxed_idle_2h_1",  "relaxed_idle_2h_1"   },
        { WP_THOMPSON,      "Thompson",         "mp_weapon",  1,  "ui/assets/weapon_thompson.tga",    "relaxed_idle_2h_1",  "relaxed_idle_2h_1"   },
        { WP_STEN,        "Sten",           "mp_weapon",  2,  "ui/assets/weapon_sten.tga",      "relaxed_idle_2h_1",  "relaxed_idle_2h_1"   },

        { WP_PANZERFAUST,   "Panzerfaust",        "mp_weapon",  4,  "ui/assets/weapon_panzerfaust.tga",   "stand_panzer",     "stand_panzer"      },
        { WP_FLAMETHROWER,    "Flamethrower",       "mp_weapon",  6,  "ui/assets/weapon_flamethrower.tga",  "stand_machinegun",   "stand_machinegun"    },

        { WP_GRENADE_PINEAPPLE, "Pineapple grenade",    "mp_weapon_2",  8,  "ui/assets/weapon_grenade.tga",     "firing_pistolB_1",   "stand_pistolB"     },
        { WP_GRENADE_LAUNCHER,  "Stick grenade",      "mp_weapon_2",  8,  "ui/assets/weapon_grenade_ger.tga",   "firing_pistolB_1",   "stand_pistolB"     },

        { WP_DYNAMITE,      "Explosives",       "mp_item2",   -1, "ui/assets/weapon_dynamite.tga",    "firing_pistolB_1",   "stand_pistolB"     },

        { WP_KAR98,       "Kar98",                    "mp_weapon",  2,  "ui/assets/weapon_kar98.tga",     "stand_rifle",      "stand_rifle"     },
        { WP_CARBINE,     "M1 Garand",                "mp_weapon",  2,  "ui/assets/weapon_carbine.tga",     "stand_rifle",      "stand_rifle"     },

        { WP_FG42,        "FG42",           "mp_weapon",  7,  "ui/assets/weapon_fg42.tga",      "stand_rifle",      "stand_rifle"     },
        { WP_GARAND,      "M1 Garand",        "mp_weapon",  8,  "ui/assets/weapon_carbine.tga",     "stand_rifle",      "stand_rifle"     },
        { WP_MOBILE_MG42,   "Mobile MG42",        "mp_weapon",  9,  "ui/assets/weapon_mg42.tga",      "stand_rifle",      "stand_rifle"     },

        { WP_LANDMINE,      "Land Mines",       "mp_weapon_2",  4,  "ui/assets/weapon_landmine.tga",    "firing_pistolB_1",   "stand_pistolB"     },

        { WP_K43,       "K43",            "mp_weapon",  8,  "ui/assets/weapon_kar98.tga",     "stand_rifle",      "stand_rifle"     },
//  { WP_SATCHEL,     "Satchel Charges",      "mp_weapon",  10, "ui/assets/weapon_satchel.tga",     "firing_pistolB_1",   "stand_pistolB"     },
        { WP_TRIPMINE,      "Trip Mines",       "mp_weapon",  9,  "ui/assets/weapon_tripmine.tga",    "firing_pistolB_1",   "stand_pistolB"     },

        { 0,          NULL,           NULL,     -1,                     NULL,         NULL          },
};*/

typedef struct
{
	char *name;
	int  flags;
	char *shader;
} uiitemType_t;

#define UI_KNIFE_PIC  "window_knife_pic"
#define UI_PISTOL_PIC "window_pistol_pic"
#define UI_WEAPON_PIC "window_weapon_pic"
#define UI_ITEM1_PIC  "window_item1_pic"
#define UI_ITEM2_PIC  "window_item2_pic"

#if 0 // rain - not used
static uiitemType_t        itemTypes[] =
{
	{ UI_KNIFE_PIC,  PT_KNIFE,      "ui/assets/weapon_knife.tga"    },
	{ UI_PISTOL_PIC, PT_PISTOL,     "ui/assets/weapon_colt1911.tga" },

	{ UI_WEAPON_PIC, PT_RIFLE,      "ui/assets/weapon_mauser.tga"   },

	{ UI_ITEM1_PIC,  PT_MEDKIT,     "ui/assets/item_medkit.tga"     },

	{ UI_ITEM1_PIC,  PT_GRENADES,   "ui/assets/weapon_grenade.tga"  },
	{ UI_ITEM2_PIC,  PT_EXPLOSIVES, "ui/assets/weapon_dynamite.tga" },

	{ NULL,          0,             NULL                            }
};
#endif

extern displayContextDef_t *DC;

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
vmCvar_t         ui_new;
vmCvar_t         ui_debug;
vmCvar_t         ui_initialized;
vmCvar_t         ui_teamArenaFirstRun;

extern itemDef_t *g_bindItem;

void             _UI_Init( qboolean );
void             _UI_Shutdown( void );
void             _UI_KeyEvent( int key, qboolean down );
void             _UI_MouseEvent( int dx, int dy );
void             _UI_Refresh( int realtime );
qboolean         _UI_IsFullscreen( void );

#if __GNUC__ >= 4
#pragma GCC visibility push(default)
#endif

#ifdef IPHONE
int baseq3_ui_vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11 )
{
#else
intptr_t vmMain( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11 )
{
#endif // IPHONE

#if __GNUC__ >= 4
#pragma GCC visibility pop
#endif

	switch ( command )
	{
		case UI_GETAPIVERSION:
			return UI_API_VERSION;

		case UI_INIT:
			_UI_Init( arg0 );
			return 0;

		case UI_SHUTDOWN:
			_UI_Shutdown();
			return 0;

		case UI_KEY_EVENT:
			_UI_KeyEvent( arg0, arg1 );
			return 0;

		case UI_MOUSE_EVENT:
			_UI_MouseEvent( arg0, arg1 );
			return 0;

		case UI_REFRESH:
			_UI_Refresh( arg0 );
			return 0;

		case UI_IS_FULLSCREEN:
			return _UI_IsFullscreen();

		case UI_SET_ACTIVE_MENU:
			_UI_SetActiveMenu( arg0 );
			return 0;

		case UI_GET_ACTIVE_MENU:
			return _UI_GetActiveMenu();

		case UI_CONSOLE_COMMAND:
			return UI_ConsoleCommand( arg0 );

		case UI_DRAW_CONNECT_SCREEN:
			UI_DrawConnectScreen( arg0 );
			return 0;

		case UI_HASUNIQUECDKEY: // mod authors need to observe this
			return qtrue;

			// NERVE - SMF
		case UI_CHECKEXECKEY:
			return UI_CheckExecKey( arg0 );

		case UI_WANTSBINDKEYS:
			return ( g_waitingForKey && g_bindItem ) ? qtrue : qfalse;

			// Dushan
		case UI_REPORT_HIGHSCORE_RESPONSE:
			// Dushan: FIX ME
			return 0;

		case UI_AUTHORIZED:
			// Dushan: FIX ME
			return 0;
	}

	return -1;
}

void AssetCache()
{
	int n;

	//if (Assets.textFont == NULL) {
	//}
	//Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	uiInfo.uiDC.Assets.fxBasePic = trap_R_RegisterShaderNoMip( ART_FX_BASE );
	uiInfo.uiDC.Assets.fxPic[ 0 ] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	uiInfo.uiDC.Assets.fxPic[ 1 ] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	uiInfo.uiDC.Assets.fxPic[ 2 ] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	uiInfo.uiDC.Assets.fxPic[ 3 ] = trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	uiInfo.uiDC.Assets.fxPic[ 4 ] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	uiInfo.uiDC.Assets.fxPic[ 5 ] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	uiInfo.uiDC.Assets.fxPic[ 6 ] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
	uiInfo.uiDC.Assets.scrollBar = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	uiInfo.uiDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	uiInfo.uiDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	uiInfo.uiDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	uiInfo.uiDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	uiInfo.uiDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	uiInfo.uiDC.Assets.sliderBar = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	uiInfo.uiDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
	uiInfo.uiDC.Assets.checkboxCheck = trap_R_RegisterShaderNoMip( ASSET_CHECKBOX_CHECK );
	uiInfo.uiDC.Assets.checkboxCheckNot = trap_R_RegisterShaderNoMip( ASSET_CHECKBOX_CHECK_NOT );
	uiInfo.uiDC.Assets.checkboxCheckNo = trap_R_RegisterShaderNoMip( ASSET_CHECKBOX_CHECK_NO );

	for ( n = 0; n < NUM_CROSSHAIRS; n++ )
	{
		uiInfo.uiDC.Assets.crosshairShader[ n ] = trap_R_RegisterShaderNoMip( va( "gfx/2d/crosshair%c", 'a' + n ) );
		uiInfo.uiDC.Assets.crosshairAltShader[ n ] = trap_R_RegisterShaderNoMip( va( "gfx/2d/crosshair%c_alt", 'a' + n ) );
	}

	//uiInfo.newHighScoreSound = trap_S_RegisterSound("sound/feedback/voc_newhighscore.wav");

	/*  for ( n = 1; weaponTypes[n].shadername; n++ ) {
	                if ( weaponTypes[n].shadername )
	                        trap_R_RegisterShaderNoMip( weaponTypes[n].shadername );
	        }*/
	// -NERVE - SMF
}

void _UI_DrawSides( float x, float y, float w, float h, float size )
{
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= uiInfo.uiDC.xscale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void _UI_DrawTopBottom( float x, float y, float w, float h, float size )
{
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= uiInfo.uiDC.yscale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void _UI_DrawRect( float x, float y, float width, float height, float size, const float *color )
{
	trap_R_SetColor( color );

	_UI_DrawTopBottom( x, y, width, height, size );
	_UI_DrawSides( x, y, width, height, size );

	trap_R_SetColor( NULL );
}

// NERVE - SMF
void Text_SetActiveFont( int font )
{
	uiInfo.activeFont = font;
}

int Text_Width_Ext( const char *text, float scale, int limit, fontInfo_t *font )
{
	int         count, len;
	float       out = 0;
	glyphInfo_t *glyph;
	const char  *s = text;

	if ( text )
	{
		len = strlen( text );

		if ( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while ( s && *s && count < len )
		{
			if ( Q_IsColorString( s ) )
			{
				s += 2;
				continue;
			}
			else
			{
				glyph = &font->glyphs[( unsigned char ) * s ];
				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}

	return out * scale * font->glyphScale;
}

int Text_Width( const char *text, float scale, int limit )
{
	fontInfo_t *font = &uiInfo.uiDC.Assets.fonts[ uiInfo.activeFont ];

	return Text_Width_Ext( text, scale, limit, font );
}

int Multiline_Text_Width( const char *text, float scale, int limit )
{
	int         count, len;
	float       out = 0;
	float       width, widest = 0;
	glyphInfo_t *glyph;
	const char  *s = text;
	fontInfo_t  *font = &uiInfo.uiDC.Assets.fonts[ uiInfo.activeFont ];

	if ( text )
	{
		len = strlen( text );

		if ( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while ( s && *s && count < len )
		{
			if ( Q_IsColorString( s ) )
			{
				s += 2;
				continue;
			}
			else
			{
				if ( *s == '\n' )
				{
					width = out * scale * font->glyphScale;

					if ( width > widest )
					{
						widest = width;
					}

					out = 0;
				}
				else
				{
					glyph = &font->glyphs[( unsigned char ) * s ];
					out += glyph->xSkip;
				}

				s++;
				count++;
			}
		}
	}

	if ( widest > 0 )
	{
		width = out * scale * font->glyphScale;

		if ( width > widest )
		{
			widest = width;
		}

		return widest;
	}
	else
	{
		return out * scale * font->glyphScale;
	}
}

int Text_Height_Ext( const char *text, float scale, int limit, fontInfo_t *font )
{
	int         len, count;
	float       max;
	glyphInfo_t *glyph;
	const char  *s = text;

	max = 0;

	if ( text )
	{
		len = strlen( text );

		if ( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while ( s && *s && count < len )
		{
			if ( Q_IsColorString( s ) )
			{
				s += 2;
				continue;
			}
			else
			{
				glyph = &font->glyphs[( unsigned char ) * s ];  // NERVE - SMF - this needs to be an unsigned cast for localization

				if ( max < glyph->height )
				{
					max = glyph->height;
				}

				s++;
				count++;
			}
		}
	}

	return max * scale * font->glyphScale;
}

int Text_Height( const char *text, float scale, int limit )
{
	fontInfo_t *font = &uiInfo.uiDC.Assets.fonts[ uiInfo.activeFont ];

	return Text_Height_Ext( text, scale, limit, font );
}

int Multiline_Text_Height( const char *text, float scale, int limit )
{
	int         len, count;
	float       max;
	float       totalheight = 0;
	glyphInfo_t *glyph;
	const char  *s = text;
	fontInfo_t  *font = &uiInfo.uiDC.Assets.fonts[ uiInfo.activeFont ];

	max = 0;

	if ( text )
	{
		len = strlen( text );

		if ( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while ( s && *s && count < len )
		{
			if ( Q_IsColorString( s ) )
			{
				s += 2;
				continue;
			}
			else
			{
				if ( *s == '\n' )
				{
					if ( !totalheight )
					{
						totalheight += 5; // 5 is the vertical spacing that autowrap painting uses
					}

					totalheight += max;
					max = 0;
				}
				else
				{
					glyph = &font->glyphs[( unsigned char ) * s ];  // NERVE - SMF - this needs to be an unsigned cast for localization

					if ( max < glyph->height )
					{
						max = glyph->height;
					}
				}

				s++;
				count++;
			}
		}
	}

	if ( totalheight > 0 )
	{
		if ( !totalheight )
		{
			totalheight += 5; // 5 is the vertical spacing that autowrap painting uses
		}

		totalheight += max;
		return totalheight * scale * font->glyphScale;
	}
	else
	{
		return max * scale * font->glyphScale;
	}
}

void Text_PaintCharExt( float x, float y, float w, float h, float scalex, float scaley, float s, float t, float s2, float t2,
                        qhandle_t hShader )
{
	w *= scalex;
	h *= scaley;
	UI_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void Text_PaintChar( float x, float y, float w, float h, float scale, float s, float t, float s2, float t2, qhandle_t hShader )
{
	w *= scale;
	h *= scale;
	UI_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void Text_Paint_Ext( float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit,
                     int style, fontInfo_t *font )
{
	int         len, count;
	vec4_t      newColor;
	glyphInfo_t *glyph;
	int         index;

	scalex *= font->glyphScale;
	scaley *= font->glyphScale;

	if ( text )
	{
		const char *s = text;

		trap_R_SetColor( color );
		memcpy( &newColor[ 0 ], &color[ 0 ], sizeof( vec4_t ) );
		len = strlen( text );

		if ( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while ( s && *s && count < len )
		{
			index = ( unsigned char ) * s;

			// NERVE - SMF - don't draw tabs and newlines
			if ( index < 20 )
			{
				s++;
				count++;
				continue;
			}

			glyph = &font->glyphs[ index ];

			if ( Q_IsColorString( s ) )
			{
				if ( * ( s + 1 ) == COLOR_NULL )
				{
					memcpy( &newColor[ 0 ], &color[ 0 ], sizeof( vec4_t ) );
				}
				else
				{
					memcpy( newColor, g_color_table[ ColorIndex( * ( s + 1 ) ) ], sizeof( newColor ) );
					newColor[ 3 ] = color[ 3 ];
				}

				trap_R_SetColor( newColor );
				s += 2;
				continue;
			}
			else
			{
				float yadj = scaley * glyph->top;

				if ( style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE )
				{
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;

					colorBlack[ 3 ] = newColor[ 3 ];

					trap_R_SetColor( colorBlack );
					Text_PaintCharExt( x + ( glyph->pitch * scalex ) + ofs, y - yadj + ofs, glyph->imageWidth, glyph->imageHeight,
					                   scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph );
					trap_R_SetColor( newColor );

					colorBlack[ 3 ] = 1.0;
				}

				Text_PaintCharExt( x + ( glyph->pitch * scalex ), y - yadj, glyph->imageWidth, glyph->imageHeight, scalex, scaley,
				                   glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph );

				x += ( glyph->xSkip * scalex ) + adjust;
				s++;
				count++;
			}
		}

		trap_R_SetColor( NULL );
	}
}

void Text_Paint( float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style )
{
	fontInfo_t *font = &uiInfo.uiDC.Assets.fonts[ uiInfo.activeFont ];

	Text_Paint_Ext( x, y, scale, scale, color, text, adjust, limit, style, font );
}

// copied over from Text_Paint
// we use the bulk of Text_Paint to determine were we will hit the max width
// can be used for actual text printing, or dummy run to get the number of lines
// returns the next char to be printed after wrap, or the ending \0 of the string
// NOTE: this is clearly non-optimal implementation, see Item_Text_AutoWrap_Paint for one
// if color_save != NULL, use to keep track of the current color between wraps
char           *Text_AutoWrap_Paint_Chunk( float x, float y, int width, float scale, vec4_t color, char *text, float adjust,
    int limit, int style, qboolean dummy, vec4_t color_save )
{
	int         len, count;
	vec4_t      newColor;
	glyphInfo_t *glyph;
	float       useScale;
	fontInfo_t  *font = &uiInfo.uiDC.Assets.fonts[ uiInfo.activeFont ];
	int         index;
	char        *wrap_point = NULL;

	float       wrap_x = x + width;

	useScale = scale * font->glyphScale;

	if ( text )
	{
		char *s = text;

		trap_R_SetColor( color );
		memcpy( &newColor[ 0 ], &color[ 0 ], sizeof( vec4_t ) );
		len = strlen( text );

		if ( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while ( s && *s && count < len )
		{
			index = ( unsigned char ) * s;

			if ( *s == ' ' || *s == '\t' || *s == '\n' )
			{
				wrap_point = s;
			}

			// NERVE - SMF - don't draw tabs and newlines
			if ( index < 20 )
			{
				s++;
				count++;
				continue;
			}

			glyph = &font->glyphs[ index ]; // NERVE - SMF - this needs to be an unsigned cast for localization

			if ( Q_IsColorString( s ) )
			{
				if ( * ( s + 1 ) == COLOR_NULL )
				{
					memcpy( &newColor[ 0 ], &color[ 0 ], sizeof( vec4_t ) );
				}
				else
				{
					memcpy( newColor, g_color_table[ ColorIndex( * ( s + 1 ) ) ], sizeof( newColor ) );
					newColor[ 3 ] = color[ 3 ];
				}

				if ( !dummy )
				{
					trap_R_SetColor( newColor );
				}

				if ( color_save )
				{
					memcpy( &color_save[ 0 ], &newColor[ 0 ], sizeof( vec4_t ) );
				}

				s += 2;
				continue;
			}
			else
			{
				float yadj = useScale * glyph->top;

				if ( x + ( glyph->xSkip * useScale ) + adjust > wrap_x )
				{
					if ( wrap_point )
					{
						return wrap_point + 1; // the next char to be printed after line wrap
					}

					// we haven't found the wrap point .. cut
					return s;
				}

				if ( !dummy )
				{
					if ( style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE )
					{
						int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;

						colorBlack[ 3 ] = newColor[ 3 ];
						trap_R_SetColor( colorBlack );
						Text_PaintChar( x + ofs, y - yadj + ofs,
						                glyph->imageWidth,
						                glyph->imageHeight, useScale, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph );
						trap_R_SetColor( newColor );
						colorBlack[ 3 ] = 1.0;
					}

					Text_PaintChar( x, y - yadj,
					                glyph->imageWidth,
					                glyph->imageHeight, useScale, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph );
				}

				x += ( glyph->xSkip * useScale ) + adjust;
				s++;
				count++;
			}
		}

		if ( !dummy )
		{
			trap_R_SetColor( NULL );
		}
	}

	return text + strlen( text );
}

// count the lines that we will need to have to print with the given wrap parameters
int Count_Text_AutoWrap_Paint( float x, float y, int width, float scale, vec4_t color, const char *text, float adjust, int style )
{
	const char *ret, *end;
	int        i = 0;

	ret = text;
	end = text + strlen( text );

	do
	{
		ret = Text_AutoWrap_Paint_Chunk( x, y, width, scale, color, ( char * ) ret, adjust, 0, style, qtrue, NULL );
		i++;
	}
	while ( ret < end );

	return i;
}

void Text_AutoWrap_Paint( float x, float y, int width, int height, float scale, vec4_t color, const char *l_text, float adjust,
                          int style )
{
	char   text[ 1024 ];
	char   *ret, *end, *next;
	char   s;
	vec4_t aux_color, next_color;

	Q_strncpyz( text, l_text, sizeof( text ) - 1 );
	ret = text;
	end = text + strlen( text );
	memcpy( &aux_color[ 0 ], &color[ 0 ], sizeof( vec4_t ) );

	do
	{
		// one run to get the word wrap
		next = Text_AutoWrap_Paint_Chunk( x, y, width, scale, aux_color, ret, adjust, 0, style, qtrue, next_color );
		// now print - hack around a bit to avoid the word wrapped chars
		s = *next;
		*next = '\0';
		Text_Paint( x, y, scale, aux_color, ret, adjust, 0, style );
		*next = s;
		ret = next;
		memcpy( &aux_color[ 0 ], &next_color[ 0 ], sizeof( vec4_t ) );
		y += height;
	}
	while ( ret < end );
}

void Text_PaintWithCursor( float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit,
                           int style )
{
	int         len, count;
	vec4_t      newColor;
	glyphInfo_t *glyph, *glyph2;
	float       yadj;
	float       useScale;
	fontInfo_t  *font = &uiInfo.uiDC.Assets.fonts[ uiInfo.activeFont ];

	useScale = scale * font->glyphScale;

	if ( text )
	{
		const char *s = text;

		trap_R_SetColor( color );
		memcpy( &newColor[ 0 ], &color[ 0 ], sizeof( vec4_t ) );
		len = strlen( text );

		if ( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;
		glyph2 = &font->glyphs[( unsigned char ) cursor ];

		while ( s && *s && count < len )
		{
			glyph = &font->glyphs[( unsigned char ) * s ];  // NERVE - SMF - this needs to be an unsigned cast for localization
			//int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
			//float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);

			/*if ( Q_IsColorString( s ) ) {
			   memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
			   newColor[3] = color[3];
			   trap_R_SetColor( newColor );
			   s += 2;
			   continue;
			   } else */
			{
				yadj = useScale * glyph->top;

				if ( style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE )
				{
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;

					colorBlack[ 3 ] = newColor[ 3 ];
					trap_R_SetColor( colorBlack );
					Text_PaintChar( x + ( glyph->pitch * useScale ) + ofs, y - yadj + ofs,
					                glyph->imageWidth,
					                glyph->imageHeight, useScale, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph );
					colorBlack[ 3 ] = 1.0;
					trap_R_SetColor( newColor );
				}

				Text_PaintChar( x + ( glyph->pitch * useScale ), y - yadj,
				                glyph->imageWidth,
				                glyph->imageHeight, useScale, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph );

				// CG_DrawPic(x, y - yadj, scale * uiDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * uiDC.Assets.textFont.glyphs[text[i]].imageHeight, uiDC.Assets.textFont.glyphs[text[i]].glyph);
				yadj = useScale * glyph2->top;

				if ( count == cursorPos && !( ( uiInfo.uiDC.realTime / BLINK_DIVISOR ) & 1 ) )
				{
					Text_PaintChar( x + ( glyph->pitch * useScale ), y - yadj,
					                glyph2->imageWidth,
					                glyph2->imageHeight, useScale, glyph2->s, glyph2->t, glyph2->s2, glyph2->t2, glyph2->glyph );
				}

				x += ( glyph->xSkip * useScale );
				s++;
				count++;
			}
		}

		// need to paint cursor at end of text
		if ( cursorPos == len && !( ( uiInfo.uiDC.realTime / BLINK_DIVISOR ) & 1 ) )
		{
			yadj = useScale * glyph2->top;
			Text_PaintChar( x + ( glyph2->pitch * useScale ), y - yadj,
			                glyph2->imageWidth,
			                glyph2->imageHeight, useScale, glyph2->s, glyph2->t, glyph2->s2, glyph2->t2, glyph2->glyph );
		}

		trap_R_SetColor( NULL );
	}
}

static void Text_Paint_Limit( float *maxX, float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit )
{
	int         len, count;
	vec4_t      newColor;
	glyphInfo_t *glyph;

	if ( text )
	{
		const char *s = text;
		float      max = *maxX;
		float      useScale;
		fontInfo_t *font = &uiInfo.uiDC.Assets.fonts[ uiInfo.activeFont ];

		useScale = scale * font->glyphScale;

		trap_R_SetColor( color );
		len = strlen( text );

		if ( limit > 0 && len > limit )
		{
			len = limit;
		}

		count = 0;

		while ( s && *s && count < len )
		{
			glyph = &font->glyphs[( unsigned char ) * s ];  // NERVE - SMF - this needs to be an unsigned cast for localization

			if ( Q_IsColorString( s ) )
			{
				if ( * ( s + 1 ) == COLOR_NULL )
				{
					memcpy( &newColor[ 0 ], &color[ 0 ], sizeof( vec4_t ) );
				}
				else
				{
					memcpy( newColor, g_color_table[ ColorIndex( * ( s + 1 ) ) ], sizeof( newColor ) );
					newColor[ 3 ] = color[ 3 ];
				}

				trap_R_SetColor( newColor );
				s += 2;
				continue;
			}
			else
			{
				float yadj = useScale * glyph->top;

				if ( Text_Width( s, useScale, 1 ) + x > max )
				{
					*maxX = 0;
					break;
				}

				Text_PaintChar( x + ( glyph->pitch * useScale ), y - yadj,
				                glyph->imageWidth,
				                glyph->imageHeight, useScale, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph );
				x += ( glyph->xSkip * useScale ) + adjust;
				*maxX = x;
				count++;
				s++;
			}
		}

		trap_R_SetColor( NULL );
	}
}

void UI_ShowPostGame( qboolean newHigh )
{
	trap_Cvar_Set( "cg_cameraOrbit", "0" );
	trap_Cvar_Set( "cg_thirdPerson", "0" );
	trap_Cvar_Set( "sv_killserver", "1" );
	uiInfo.soundHighScore = newHigh;
	_UI_SetActiveMenu( UIMENU_POSTGAME );
}

/*
=================
_UI_Refresh
=================
*/

void UI_DrawCenteredPic( qhandle_t image, int w, int h )
{
	int x, y;

	x = ( SCREEN_WIDTH - w ) / 2;
	y = ( SCREEN_HEIGHT - h ) / 2;
	UI_DrawHandlePic( x, y, w, h, image );
}

int frameCount = 0;
int startTime;

#define UI_FPS_FRAMES 4
void _UI_Refresh( int realtime )
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

	if ( trap_Cvar_VariableValue( "ui_connecting" ) )
	{
		UI_DrawLoadPanel( qtrue, qfalse, qtrue );

		if ( !trap_Cvar_VariableValue( "ui_connecting" ) )
		{
			trap_Cvar_Set( "ui_connecting", "1" );
		}

		return;
	}

	// OSP - blackout if speclocked
	if ( ui_blackout.integer > 0 )
	{
		UI_FillRect( -10, -10, 650, 490, colorBlack );
	}

	if ( Menu_Count() > 0 )
	{
		// paint all the menus
		Menu_PaintAll();
		// refresh server browser list
		UI_DoServerRefresh();
		// refresh server status
		UI_BuildServerStatus( qfalse );
		// refresh find player list
		UI_BuildFindPlayerList( qfalse );
	}

	// draw cursor
	UI_SetColor( NULL );

	if ( Menu_Count() > 0 )
	{
		uiClientState_t cstate;

		trap_GetClientState( &cstate );

		if ( cstate.connState <= CA_DISCONNECTED || cstate.connState >= CA_ACTIVE )
		{
			UI_DrawHandlePic( uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory, 32, 32, uiInfo.uiDC.Assets.cursor );
		}
	}
}

/*
=================
_UI_Shutdown
=================
*/
void _UI_Shutdown( void )
{
	trap_LAN_SaveCachedServers();
}

char *defaultMenu = NULL;

char           *GetMenuBuffer( const char *filename )
{
	int          len;
	fileHandle_t f;
	static char  buf[ MAX_MENUFILE ];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( !f )
	{
		trap_Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return defaultMenu;
	}

	if ( len >= MAX_MENUFILE )
	{
		trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", filename, len, MAX_MENUFILE ) );
		trap_FS_FCloseFile( f );
		return defaultMenu;
	}

	trap_FS_Read( buf, len, f );
	buf[ len ] = 0;
	trap_FS_FCloseFile( f );
	//COM_Compress(buf);
	return buf;
}

qboolean Asset_Parse( int handle )
{
	pc_token_t token;
	const char *tempStr;

	if ( !trap_PC_ReadToken( handle, &token ) )
	{
		return qfalse;
	}

	if ( Q_stricmp( token.string, "{" ) != 0 )
	{
		return qfalse;
	}

	while ( 1 )
	{
		memset( &token, 0, sizeof( pc_token_t ) );

		if ( !trap_PC_ReadToken( handle, &token ) )
		{
			return qfalse;
		}

		if ( Q_stricmp( token.string, "}" ) == 0 )
		{
			return qtrue;
		}

		// font
		if ( Q_stricmp( token.string, "font" ) == 0 )
		{
			int pointSize, fontIndex;

			if ( !PC_Int_Parse( handle, &fontIndex ) || !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
			{
				return qfalse;
			}

			if ( fontIndex < 0 || fontIndex >= 6 )
			{
				return qfalse;
			}

			trap_R_RegisterFont( tempStr, pointSize, &uiInfo.uiDC.Assets.fonts[ fontIndex ] );
			uiInfo.uiDC.Assets.fontRegistered = qtrue;
			continue;
		}

		// gradientbar
		if ( Q_stricmp( token.string, "gradientbar" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( tempStr );
			continue;
		}

		// enterMenuSound
		if ( Q_stricmp( token.string, "menuEnterSound" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr, qtrue );
			continue;
		}

		// exitMenuSound
		if ( Q_stricmp( token.string, "menuExitSound" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr, qtrue );
			continue;
		}

		// itemFocusSound
		if ( Q_stricmp( token.string, "itemFocusSound" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr, qtrue );
			continue;
		}

		// menuBuzzSound
		if ( Q_stricmp( token.string, "menuBuzzSound" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr, qtrue );
			continue;
		}

		if ( Q_stricmp( token.string, "cursor" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &uiInfo.uiDC.Assets.cursorStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.cursor = trap_R_RegisterShaderNoMip( uiInfo.uiDC.Assets.cursorStr );
			continue;
		}

		if ( Q_stricmp( token.string, "fadeClamp" ) == 0 )
		{
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.fadeClamp ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "fadeCycle" ) == 0 )
		{
			if ( !PC_Int_Parse( handle, &uiInfo.uiDC.Assets.fadeCycle ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "fadeAmount" ) == 0 )
		{
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.fadeAmount ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "shadowX" ) == 0 )
		{
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.shadowX ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "shadowY" ) == 0 )
		{
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.shadowY ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "shadowColor" ) == 0 )
		{
			if ( !PC_Color_Parse( handle, &uiInfo.uiDC.Assets.shadowColor ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.shadowFadeClamp = uiInfo.uiDC.Assets.shadowColor[ 3 ];
			continue;
		}
	}

	return qfalse;
}

void UI_Report()
{
	String_Report();
}

void QDECL Com_DPrintf( const char *fmt, ... );

qboolean UI_ParseMenu( const char *menuFile )
{
	int        handle;
	pc_token_t token;

	Com_DPrintf( "Parsing menu file: %s\n", menuFile );

	handle = trap_PC_LoadSource( menuFile );

	if ( !handle )
	{
		return qfalse;
	}

	while ( 1 )
	{
		memset( &token, 0, sizeof( pc_token_t ) );

		if ( !trap_PC_ReadToken( handle, &token ) )
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

		if ( Q_stricmp( token.string, "assetGlobalDef" ) == 0 )
		{
			if ( Asset_Parse( handle ) )
			{
				continue;
			}
			else
			{
				break;
			}
		}

		if ( Q_stricmp( token.string, "menudef" ) == 0 )
		{
			// start a new menu
			Menu_New( handle );
		}
	}

	trap_PC_FreeSource( handle );
	return qtrue;
}

qboolean Load_Menu( int handle )
{
	pc_token_t token;

#ifdef LOCALIZATION_SUPPORT
	int        cl_language; // NERVE - SMF
#endif // LOCALIZATION_SUPPORT

	if ( !trap_PC_ReadToken( handle, &token ) )
	{
		return qfalse;
	}

	if ( token.string[ 0 ] != '{' )
	{
		return qfalse;
	}

	while ( 1 )
	{
		if ( !trap_PC_ReadToken( handle, &token ) )
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
		// NERVE - SMF - localization crap
		cl_language = atoi( UI_Cvar_VariableString( "cl_language" ) );

		if ( cl_language )
		{
			const char *s = NULL; // TTimo: init
			const char *filename;
			char       out[ 256 ];

//          char filename[256];

			COM_StripFilename( token.string, out );

			filename = COM_SkipPath( token.string );

			if ( cl_language == 1 )
			{
				s = va( "%s%s", out, "french/" );
			}
			else if ( cl_language == 2 )
			{
				s = va( "%s%s", out, "german/" );
			}
			else if ( cl_language == 3 )
			{
				s = va( "%s%s", out, "italian/" );
			}
			else if ( cl_language == 4 )
			{
				s = va( "%s%s", out, "spanish/" );
			}

			if ( UI_ParseMenu( va( "%s%s", s, filename ) ) )
			{
				continue;
			}
		}

		// -NERVE
#endif // LOCALIZATION_SUPPORT

		UI_ParseMenu( token.string );
	}

	return qfalse;
}

void UI_LoadMenus( const char *menuFile, qboolean reset )
{
	pc_token_t      token;
	int             handle;
	int             start;
	uiClientState_t cstate;

	start = trap_Milliseconds();
	trap_GetClientState( &cstate );

	if ( cstate.connState <= CA_DISCONNECTED )
	{
		trap_PC_AddGlobalDefine( "FUI" );
	}

	handle = trap_PC_LoadSource( menuFile );

	if ( !handle )
	{
		Com_Printf( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile );
		handle = trap_PC_LoadSource( "ui/menus.txt" );

		if ( !handle )
		{
			trap_Error( S_COLOR_RED "default menu file not found: ui_mp/menus.txt, unable to continue!\n" );
		}
	}

	ui_new.integer = 1;

	if ( reset )
	{
		Menu_Reset();
	}

	while ( 1 )
	{
		if ( !trap_PC_ReadToken( handle, &token ) )
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

		if ( Q_stricmp( token.string, "loadmenu" ) == 0 )
		{
			if ( Load_Menu( handle ) )
			{
				continue;
			}
			else
			{
				break;
			}
		}
	}

	Com_DPrintf( "UI menu load time = %d milli seconds\n", trap_Milliseconds() - start );

	trap_PC_FreeSource( handle );
}

void UI_Load()
{
	char      lastName[ 1024 ];
	menuDef_t *menu = Menu_GetFocused();
	char      *menuSet = UI_Cvar_VariableString( "ui_menuFiles" );

	if ( menu && menu->window.name )
	{
		strcpy( lastName, menu->window.name );
	}

	if ( menuSet == NULL || menuSet[ 0 ] == '\0' )
	{
		menuSet = "ui/menus.txt";
	}

	String_Init();

	UI_ParseGameInfo( "gameinfo.txt" );
	UI_LoadArenas();
	UI_LoadCampaigns();

	UI_LoadMenus( menuSet, qtrue );
	Menus_CloseAll();
	Menus_ActivateByName( lastName, qtrue );
}

static const char *handicapValues[] =
{
	"None", "95", "90", "85", "80", "75", "70", "65", "60", "55", "50", "45", "40", "35", "30", "25", "20", "15", "10", "5",
	NULL
};

//static int numHandicaps = sizeof(handicapValues) / sizeof(const char*); // TTimo: unused

static void UI_DrawHandicap( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	int i, h;

	h = Com_Clamp( 5, 100, trap_Cvar_VariableValue( "handicap" ) );
	i = 20 - h / 5;

	Text_Paint( rect->x, rect->y, scale, color, handicapValues[ i ], 0, 0, textStyle );
}

static void UI_DrawClanName( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	Text_Paint( rect->x, rect->y, scale, color, UI_Cvar_VariableString( "ui_teamName" ), 0, 0, textStyle );
}

static void UI_SetCapFragLimits( qboolean uiVars )
{
}

// ui_gameType assumes gametype 0 is -1 ALL and will not show
static void UI_DrawGameType( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	Text_Paint( rect->x, rect->y, scale, color, uiInfo.gameTypes[ ui_gameType.integer ].gameType, 0, 0, textStyle );
}

/*static void UI_DrawNetGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
        if (ui_netGameType.integer < 0 || ui_netGameType.integer > uiInfo.numGameTypes) {
                trap_Cvar_Set("ui_netGameType", "0");
                trap_Cvar_Set("ui_actualNetGameType", "0");
        }
  Text_Paint(rect->x, rect->y, scale, color, uiInfo.gameTypes[ui_netGameType.integer].gameType , 0, 0, textStyle);
}*/

/*static void UI_DrawJoinGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
        if (ui_joinGameType.integer < 0 || ui_joinGameType.integer > uiInfo.numJoinGameTypes) {
                trap_Cvar_Set("ui_joinGameType", "0");
        }
  Text_Paint(rect->x, rect->y, scale, color, trap_TranslateString( uiInfo.joinGameTypes[ui_joinGameType.integer].gameType ), 0, 0, textStyle);
}*/

static int UI_TeamIndexFromName( const char *name )
{
	int i;

	if ( name && *name )
	{
		for ( i = 0; i < uiInfo.teamCount; i++ )
		{
			if ( Q_stricmp( name, uiInfo.teamList[ i ].teamName ) == 0 )
			{
				return i;
			}
		}
	}

	return 0;
}

/*
==============
UI_DrawSaveGameShot
==============
*/
static void UI_DrawSaveGameShot( rectDef_t *rect, float scale, vec4_t color )
{
	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.savegameList[ uiInfo.savegameIndex ].sshotImage );
	trap_R_SetColor( NULL );
}

/*
==============
UI_DrawClanLogo
==============
*/
static void UI_DrawClanLogo( rectDef_t *rect, float scale, vec4_t color )
{
	int i;

	i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );

	if ( i >= 0 && i < uiInfo.teamCount )
	{
		trap_R_SetColor( color );

		if ( uiInfo.teamList[ i ].teamIcon == -1 )
		{
			uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip( uiInfo.teamList[ i ].imageName );
			uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip( va( "%s_metal", uiInfo.teamList[ i ].imageName ) );
			uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[ i ].imageName ) );
		}

		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon );
		trap_R_SetColor( NULL );
	}
}

/*
==============
UI_DrawClanCinematic
==============
*/
static void UI_DrawClanCinematic( rectDef_t *rect, float scale, vec4_t color )
{
	int i;

	i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );

	if ( i >= 0 && i < uiInfo.teamCount )
	{
		if ( uiInfo.teamList[ i ].cinematic >= -2 )
		{
			if ( uiInfo.teamList[ i ].cinematic == -1 )
			{
				uiInfo.teamList[ i ].cinematic =
				  trap_CIN_PlayCinematic( va( "%s.roq", uiInfo.teamList[ i ].imageName ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
			}

			if ( uiInfo.teamList[ i ].cinematic >= 0 )
			{
				trap_CIN_RunCinematic( uiInfo.teamList[ i ].cinematic );
				trap_CIN_SetExtents( uiInfo.teamList[ i ].cinematic, rect->x, rect->y, rect->w, rect->h );
				trap_CIN_DrawCinematic( uiInfo.teamList[ i ].cinematic );
			}
			else
			{
				uiInfo.teamList[ i ].cinematic = -2;
			}
		}
		else
		{
			trap_R_SetColor( color );
			UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon );
			trap_R_SetColor( NULL );
		}
	}
}

static void UI_DrawPreviewCinematic( rectDef_t *rect, float scale, vec4_t color )
{
	if ( uiInfo.previewMovie > -2 )
	{
		uiInfo.previewMovie =
		  trap_CIN_PlayCinematic( va( "%s.roq", uiInfo.movieList[ uiInfo.movieIndex ] ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );

		if ( uiInfo.previewMovie >= 0 )
		{
			trap_CIN_RunCinematic( uiInfo.previewMovie );
			trap_CIN_SetExtents( uiInfo.previewMovie, rect->x, rect->y, rect->w, rect->h );
			trap_CIN_DrawCinematic( uiInfo.previewMovie );
		}
		else
		{
			uiInfo.previewMovie = -2;
		}
	}
}

/*static void UI_DrawSkill(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
  int i;
        i = trap_Cvar_VariableValue( "g_spSkill" );
  if (i < 1 || i > numSkillLevels) {
        i = 1;
  }
  Text_Paint(rect->x, rect->y, scale, color, skillLevels[i-1],0, 0, textStyle);
}*/

static void UI_DrawTeamName( rectDef_t *rect, float scale, vec4_t color, qboolean blue, int textStyle )
{
	int i;

	i = UI_TeamIndexFromName( UI_Cvar_VariableString( ( blue ) ? "ui_blueTeam" : "ui_redTeam" ) );

	if ( i >= 0 && i < uiInfo.teamCount )
	{
		Text_Paint( rect->x, rect->y, scale, color, va( "%s: %s", ( blue ) ? "Blue" : "Red", uiInfo.teamList[ i ].teamName ), 0, 0,
		            textStyle );
	}
}

static void UI_DrawTeamMember( rectDef_t *rect, float scale, vec4_t color, qboolean blue, int num, int textStyle )
{
}

static void UI_DrawEffects( rectDef_t *rect, float scale, vec4_t color )
{
	UI_DrawHandlePic( rect->x, rect->y, 128, 8, uiInfo.uiDC.Assets.fxBasePic );
	UI_DrawHandlePic( rect->x + uiInfo.effectsColor * 16 + 8, rect->y, 16, 12, uiInfo.uiDC.Assets.fxPic[ uiInfo.effectsColor ] );
}

void UI_DrawMapPreview( rectDef_t *rect, float scale, vec4_t color, qboolean net )
{
	int map = ( net ) ? ui_currentNetMap.integer : ui_currentMap.integer;
	int game = net ? ui_netGameType.integer : uiInfo.gameTypes[ ui_gameType.integer ].gtEnum;

	if ( game == GT_WOLF_CAMPAIGN )
	{
		if ( map < 0 || map > uiInfo.campaignCount )
		{
			if ( net )
			{
				ui_currentNetMap.integer = 0;
				trap_Cvar_Set( "ui_currentNetMap", "0" );
			}
			else
			{
				ui_currentMap.integer = 0;
				trap_Cvar_Set( "ui_currentMap", "0" );
			}

			map = 0;
		}

		if ( uiInfo.campaignList[ map ].mapTC[ 0 ][ 0 ] && uiInfo.campaignList[ map ].mapTC[ 1 ][ 0 ] )
		{
			float x, y, w, h;
			int   i;

			x = rect->x;
			y = rect->y;
			w = rect->w;
			h = rect->h;
			UI_AdjustFrom640( &x, &y, &w, &h );
			trap_R_DrawStretchPic( x, y, w, h,
			                       uiInfo.campaignList[ map ].mapTC[ 0 ][ 0 ] / 1024.f,
			                       uiInfo.campaignList[ map ].mapTC[ 0 ][ 1 ] / 1024.f,
			                       uiInfo.campaignList[ map ].mapTC[ 1 ][ 0 ] / 1024.f,
			                       uiInfo.campaignList[ map ].mapTC[ 1 ][ 1 ] / 1024.f, uiInfo.campaignMap );

			for ( i = 0; i < uiInfo.campaignList[ map ].mapCount; i++ )
			{
				vec4_t colourFadedBlack = { 0.f, 0.f, 0.f, 0.4f };

				x = rect->x +
				    ( ( uiInfo.campaignList[ map ].mapInfos[ i ]->mappos[ 0 ] - uiInfo.campaignList[ map ].mapTC[ 0 ][ 0 ] ) / 650.f * rect->w );
				y = rect->y +
				    ( ( uiInfo.campaignList[ map ].mapInfos[ i ]->mappos[ 1 ] - uiInfo.campaignList[ map ].mapTC[ 0 ][ 1 ] ) / 650.f * rect->h );

				w = Text_Width( uiInfo.campaignList[ map ].mapInfos[ i ]->mapName, scale, 0 );

				if ( x + 10 + w > rect->x + rect->w )
				{
					UI_FillRect( x - w - 12 + 1, y - 6 + 1, 12 + w, 12, colourFadedBlack );
					UI_FillRect( x - w - 12, y - 6, 12 + w, 12, colorBlack );
				}
				else
				{
					UI_FillRect( x + 1, y - 6 + 1, 12 + w, 12, colourFadedBlack );
					UI_FillRect( x, y - 6, 12 + w, 12, colorBlack );
				}

				UI_DrawHandlePic( x - 8, y - 8, 16, 16, trap_R_RegisterShaderNoMip( "gfx/loading/pin_neutral" ) );

				if ( x + 10 + w > rect->x + rect->w )
				{
					Text_Paint( x - w - 10, y + 3, scale, colorWhite, uiInfo.campaignList[ map ].mapInfos[ i ]->mapName, 0, 0, 0 );
				}
				else
				{
					Text_Paint( x + 10, y + 3, scale, colorWhite, uiInfo.campaignList[ map ].mapInfos[ i ]->mapName, 0, 0, 0 );
				}
			}
		}
		else
		{
			UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip( "levelshots/unknownmap" ) );
		}

		return;
	}

	if ( map < 0 || map > uiInfo.mapCount )
	{
		if ( net )
		{
			ui_currentNetMap.integer = 0;
			trap_Cvar_Set( "ui_currentNetMap", "0" );
		}
		else
		{
			ui_currentMap.integer = 0;
			trap_Cvar_Set( "ui_currentMap", "0" );
		}

		map = 0;
	}

	if ( uiInfo.mapList[ map ].mappos[ 0 ] && uiInfo.mapList[ map ].mappos[ 1 ] )
	{
		float  x, y, w, h;
		vec2_t tl, br;
		vec4_t colourFadedBlack = { 0.f, 0.f, 0.f, 0.4f };

		tl[ 0 ] = uiInfo.mapList[ map ].mappos[ 0 ] - .5 * 650.f;

		if ( tl[ 0 ] < 0 )
		{
			tl[ 0 ] = 0;
		}

		br[ 0 ] = tl[ 0 ] + 650.f;

		if ( br[ 0 ] > 1024.f )
		{
			br[ 0 ] = 1024.f;
			tl[ 0 ] = br[ 0 ] - 650.f;
		}

		tl[ 1 ] = uiInfo.mapList[ map ].mappos[ 1 ] - .5 * 650.f;

		if ( tl[ 1 ] < 0 )
		{
			tl[ 1 ] = 0;
		}

		br[ 1 ] = tl[ 1 ] + 650.f;

		if ( br[ 1 ] > 1024.f )
		{
			br[ 1 ] = 1024.f;
			tl[ 1 ] = br[ 1 ] - 650.f;
		}

		x = rect->x;
		y = rect->y;
		w = rect->w;
		h = rect->h;
		UI_AdjustFrom640( &x, &y, &w, &h );
		trap_R_DrawStretchPic( x, y, w, h, tl[ 0 ] / 1024.f, tl[ 1 ] / 1024.f, br[ 0 ] / 1024.f, br[ 1 ] / 1024.f, uiInfo.campaignMap );

		x = rect->x + ( ( uiInfo.mapList[ map ].mappos[ 0 ] - tl[ 0 ] ) / 650.f * rect->w );
		y = rect->y + ( ( uiInfo.mapList[ map ].mappos[ 1 ] - tl[ 1 ] ) / 650.f * rect->h );

		w = Text_Width( uiInfo.mapList[ map ].mapName, scale, 0 );

		if ( x + 10 + w > rect->x + rect->w )
		{
			UI_FillRect( x - w - 12 + 1, y - 6 + 1, 12 + w, 12, colourFadedBlack );
			UI_FillRect( x - w - 12, y - 6, 12 + w, 12, colorBlack );
		}
		else
		{
			UI_FillRect( x + 1, y - 6 + 1, 12 + w, 12, colourFadedBlack );
			UI_FillRect( x, y - 6, 12 + w, 12, colorBlack );
		}

		UI_DrawHandlePic( x - 8, y - 8, 16, 16, trap_R_RegisterShaderNoMip( "gfx/loading/pin_neutral" ) );

		if ( x + 10 + w > rect->x + rect->w )
		{
			Text_Paint( x - w - 10, y + 3, scale, colorWhite, uiInfo.mapList[ map ].mapName, 0, 0, 0 );
		}
		else
		{
			Text_Paint( x + 10, y + 3, scale, colorWhite, uiInfo.mapList[ map ].mapName, 0, 0, 0 );
		}
	}
	else
	{
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip( "levelshots/unknownmap" ) );
	}

	/*if (uiInfo.mapList[map].levelShot == -1) {
	   uiInfo.mapList[map].levelShot = trap_R_RegisterShaderNoMip(uiInfo.mapList[map].imageName);
	   }

	   if (uiInfo.mapList[map].levelShot > 0) {
	   UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.mapList[map].levelShot);
	   } else {
	   UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip("levelshots/unknownmap"));
	   } */
}

void UI_DrawNetMapPreview( rectDef_t *rect, float scale, vec4_t color, qboolean net )
{
	if ( uiInfo.serverStatus.currentServerPreview > 0 )
	{
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.serverStatus.currentServerPreview );
	}
	else
	{
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip( "levelshots/unknownmap" ) );
	}
}

/*static void UI_DrawMapTimeToBeat(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
        int minutes, seconds, time;
        if (ui_currentMap.integer < 0 || ui_currentMap.integer > uiInfo.mapCount) {
                ui_currentMap.integer = 0;
                trap_Cvar_Set("ui_currentMap", "0");
        }

        time = uiInfo.mapList[ui_currentMap.integer].timeToBeat[uiInfo.gameTypes[ui_gameType.integer].gtEnum];

        minutes = time / 60;
        seconds = time % 60;

  Text_Paint(rect->x, rect->y, scale, color, va("%02i:%02i", minutes, seconds), 0, 0, textStyle);
}*/

static void UI_DrawMapCinematic( rectDef_t *rect, float scale, vec4_t color, qboolean net )
{
	int map = ( net ) ? ui_currentNetMap.integer : ui_currentMap.integer;
	int game = net ? ui_netGameType.integer : uiInfo.gameTypes[ ui_gameType.integer ].gtEnum;

	if ( game == GT_WOLF_CAMPAIGN )
	{
		if ( map < 0 || map > uiInfo.campaignCount )
		{
			if ( net )
			{
				ui_currentNetMap.integer = 0;
				trap_Cvar_Set( "ui_currentNetMap", "0" );
			}
			else
			{
				ui_currentMap.integer = 0;
				trap_Cvar_Set( "ui_currentMap", "0" );
			}

			map = 0;
		}

		/*    if( uiInfo.campaignList[map].campaignCinematic >= -1 ) {
		                        if( uiInfo.campaignList[map].campaignCinematic == -1 ) {
		                                uiInfo.campaignList[map].campaignCinematic = trap_CIN_PlayCinematic(va("%s.roq", uiInfo.campaignList[map].campaignShortName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
		                        }
		                        if( uiInfo.campaignList[map].campaignCinematic >= 0 ) {
		                                trap_CIN_RunCinematic( uiInfo.campaignList[map].campaignCinematic );
		                                trap_CIN_SetExtents( uiInfo.campaignList[map].campaignCinematic, rect->x, rect->y, rect->w, rect->h );
		                                trap_CIN_DrawCinematic( uiInfo.campaignList[map].campaignCinematic );
		                        } else {
		                                uiInfo.campaignList[map].campaignCinematic = -2;
		                        }
		                } else*/
		{
			UI_DrawMapPreview( rect, scale, color, net );
		}
		return;
	}

	if ( map < 0 || map > uiInfo.mapCount )
	{
		if ( net )
		{
			ui_currentNetMap.integer = 0;
			trap_Cvar_Set( "ui_currentNetMap", "0" );
		}
		else
		{
			ui_currentMap.integer = 0;
			trap_Cvar_Set( "ui_currentMap", "0" );
		}

		map = 0;
	}

	if ( uiInfo.mapList[ map ].cinematic >= -1 )
	{
		if ( uiInfo.mapList[ map ].cinematic == -1 )
		{
			uiInfo.mapList[ map ].cinematic =
			  trap_CIN_PlayCinematic( va( "%s.roq", uiInfo.mapList[ map ].mapLoadName ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
		}

		if ( uiInfo.mapList[ map ].cinematic >= 0 )
		{
			trap_CIN_RunCinematic( uiInfo.mapList[ map ].cinematic );
			trap_CIN_SetExtents( uiInfo.mapList[ map ].cinematic, rect->x, rect->y, rect->w, rect->h );
			trap_CIN_DrawCinematic( uiInfo.mapList[ map ].cinematic );
		}
		else
		{
			uiInfo.mapList[ map ].cinematic = -2;
		}
	}
	else
	{
		UI_DrawMapPreview( rect, scale, color, net );
	}
}

static void UI_DrawCampaignPreview( rectDef_t *rect, float scale, vec4_t color, qboolean net )
{
	int campaign = ( net ) ? ui_currentNetCampaign.integer : ui_currentCampaign.integer;

	if ( campaign < 0 || campaign > uiInfo.campaignCount )
	{
		if ( net )
		{
			ui_currentNetCampaign.integer = 0;
			trap_Cvar_Set( "ui_currentNetCampaign", "0" );
		}
		else
		{
			ui_currentCampaign.integer = 0;
			trap_Cvar_Set( "ui_currentCampaign", "0" );
		}

		campaign = 0;
	}

	if ( uiInfo.campaignList[ campaign ].campaignShot == -1 )
	{
		uiInfo.campaignList[ campaign ].campaignShot = trap_R_RegisterShaderNoMip( uiInfo.campaignList[ campaign ].campaignShotName );
	}

	if ( uiInfo.campaignList[ campaign ].campaignShot > 0 )
	{
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.campaignList[ campaign ].campaignShot );
	}
	else
	{
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip( "levelshots/unknownmap" ) );
	}
}

static void UI_DrawCampaignCinematic( rectDef_t *rect, float scale, vec4_t color, qboolean net )
{
	int campaign = ( net ) ? ui_currentNetCampaign.integer : ui_currentCampaign.integer;

	if ( campaign < 0 || campaign > uiInfo.campaignCount )
	{
		if ( net )
		{
			ui_currentNetCampaign.integer = 0;
			trap_Cvar_Set( "ui_currentNetCampaign", "0" );
		}
		else
		{
			ui_currentCampaign.integer = 0;
			trap_Cvar_Set( "ui_currentCampaign", "0" );
		}

		campaign = 0;
	}

	if ( uiInfo.campaignList[ campaign ].campaignCinematic >= -1 )
	{
		if ( uiInfo.campaignList[ campaign ].campaignCinematic == -1 )
		{
			uiInfo.campaignList[ campaign ].campaignCinematic =
			  trap_CIN_PlayCinematic( va( "%s.roq", uiInfo.campaignList[ campaign ].campaignShortName ), 0, 0, 0, 0,
			                          ( CIN_loop | CIN_silent ) );
		}

		if ( uiInfo.campaignList[ campaign ].campaignCinematic >= 0 )
		{
			trap_CIN_RunCinematic( uiInfo.campaignList[ campaign ].campaignCinematic );
			trap_CIN_SetExtents( uiInfo.campaignList[ campaign ].campaignCinematic, rect->x, rect->y, rect->w, rect->h );
			trap_CIN_DrawCinematic( uiInfo.campaignList[ campaign ].campaignCinematic );
		}
		else
		{
			uiInfo.campaignList[ campaign ].campaignCinematic = -2;
		}
	}
	else
	{
		UI_DrawCampaignPreview( rect, scale, color, net );
	}
}

static void UI_DrawCampaignName( rectDef_t *rect, float scale, vec4_t color, int textStyle, qboolean net )
{
	int campaign = ( net ) ? ui_currentNetCampaign.integer : ui_currentCampaign.integer;

	if ( campaign < 0 || campaign > uiInfo.campaignCount )
	{
		if ( net )
		{
			ui_currentNetCampaign.integer = 0;
			trap_Cvar_Set( "ui_currentNetCampaign", "0" );
		}
		else
		{
			ui_currentCampaign.integer = 0;
			trap_Cvar_Set( "ui_currentCampaign", "0" );
		}

		campaign = 0;
	}

	if ( !uiInfo.campaignList[ campaign ].unlocked )
	{
		return;
	}

	Text_Paint( rect->x, rect->y, scale, color, va( "%s", uiInfo.campaignList[ campaign ].campaignName ), 0, 0, textStyle );
}

void UI_DrawCampaignDescription( rectDef_t *rect, float scale, vec4_t color, float text_x, float text_y, int textStyle, int align,
                                 qboolean net )
{
	const char *p, *textPtr, *newLinePtr = NULL;
	char       buff[ 1024 ];
	int        height, len, textWidth, newLine, newLineWidth;
	float      y;
	rectDef_t  textRect;
	int        game = ( net ) ? ui_netGameType.integer : ui_netGameType.integer;

	if ( game == GT_WOLF_CAMPAIGN )
	{
		int campaign = ( net ) ? ui_currentNetMap.integer : ui_currentMap.integer;

		textPtr = uiInfo.campaignList[ campaign ].campaignDescription;
	}
	else if ( game == GT_WOLF_LMS )
	{
		int map = ( net ) ? ui_currentNetMap.integer : ui_currentMap.integer;

		textPtr = uiInfo.mapList[ map ].lmsbriefing;
	}
	else
	{
		int map = ( net ) ? ui_currentNetMap.integer : ui_currentMap.integer;

		textPtr = uiInfo.mapList[ map ].briefing;
	}

	if ( !textPtr || !*textPtr )
	{
		textPtr = "^1No text supplied";
	}

	height = Text_Height( textPtr, scale, 0 );

	textRect.x = 0;
	textRect.y = 0;
	textRect.w = rect->w;
	textRect.h = rect->h;

	//y = text_y;
	y = 0;
	len = 0;
	buff[ 0 ] = '\0';
	newLine = 0;
	newLineWidth = 0;
	p = textPtr;

	while ( p )
	{
		textWidth = DC->textWidth( buff, scale, 0 );

		if ( *p == ' ' || *p == '\t' || *p == '\n' || *p == '\0' || *p == '*' )
		{
			newLine = len;
			newLinePtr = p + 1;
			newLineWidth = textWidth;
		} /* else if( *p == '*' && *(p+1) == '*' ) {

                                           newLine = len;
                                           newLinePtr = p+2;
                                           newLineWidth = textWidth;
                                           } */

		if ( ( newLine && textWidth > rect->w ) || *p == '\n' || *p == '\0' || *p == '*' /*( *p == '*' && *(p+1) == '*' ) */ )
		{
			if ( len )
			{
				if ( align == ITEM_ALIGN_LEFT )
				{
					textRect.x = text_x;
				}
				else if ( align == ITEM_ALIGN_RIGHT )
				{
					textRect.x = text_x - newLineWidth;
				}
				else if ( align == ITEM_ALIGN_CENTER )
				{
					textRect.x = text_x - newLineWidth / 2;
				}

				textRect.y = y;

				textRect.x += rect->x;
				textRect.y += rect->y;
				//
				buff[ newLine ] = '\0';
				DC->drawText( textRect.x, textRect.y, scale, color, buff, 0, 0, textStyle );
			}

			if ( *p == '\0' )
			{
				break;
			}

			//
			y += height + 5;
			p = newLinePtr;
			len = 0;
			newLine = 0;
			newLineWidth = 0;
			continue;
		}

		buff[ len++ ] = *p++;

		if ( buff[ len - 1 ] == 13 )
		{
			buff[ len - 1 ] = ' ';
		}

		buff[ len ] = '\0';
	}
}

void UI_DrawGametypeDescription( rectDef_t *rect, float scale, vec4_t color, float text_x, float text_y, int textStyle, int align,
                                 qboolean net )
{
	const char *p, *textPtr = NULL, *newLinePtr = NULL;
	char       buff[ 1024 ];
	int        height, len, textWidth, newLine, newLineWidth, i;
	float      y;
	rectDef_t  textRect;

	int        game = ( net ) ? ui_netGameType.integer : ui_netGameType.integer;

	for ( i = 0; i < uiInfo.numGameTypes; i++ )
	{
		if ( uiInfo.gameTypes[ i ].gtEnum == game )
		{
			textPtr = uiInfo.gameTypes[ i ].gameTypeDescription;
			break;
		}
	}

	if ( i == uiInfo.numGameTypes )
	{
		textPtr = "Unknown";
	}

	height = Text_Height( textPtr, scale, 0 );

	textRect.x = 0;
	textRect.y = 0;
	textRect.w = rect->w;
	textRect.h = rect->h;

	//y = text_y;
	y = 0;
	len = 0;
	buff[ 0 ] = '\0';
	newLine = 0;
	newLineWidth = 0;
	p = textPtr;

	while ( p )
	{
		textWidth = DC->textWidth( buff, scale, 0 );

		if ( *p == ' ' || *p == '\t' || *p == '\n' || *p == '\0' )
		{
			newLine = len;
			newLinePtr = p + 1;
			newLineWidth = textWidth;
		}
		else if ( *p == '*' && * ( p + 1 ) == '*' )
		{
			newLine = len;
			newLinePtr = p + 2;
			newLineWidth = textWidth;
		}

		if ( ( newLine && textWidth > rect->w ) || *p == '\n' || *p == '\0' || ( *p == '*' && * ( p + 1 ) == '*' ) )
		{
			if ( len )
			{
				if ( align == ITEM_ALIGN_LEFT )
				{
					textRect.x = text_x;
				}
				else if ( align == ITEM_ALIGN_RIGHT )
				{
					textRect.x = text_x - newLineWidth;
				}
				else if ( align == ITEM_ALIGN_CENTER )
				{
					textRect.x = text_x - newLineWidth / 2;
				}

				textRect.y = y;

				textRect.x += rect->x;
				textRect.y += rect->y;
				//
				buff[ newLine ] = '\0';
				DC->drawText( textRect.x, textRect.y, scale, color, buff, 0, 0, textStyle );
			}

			if ( *p == '\0' )
			{
				break;
			}

			//
			y += height + 5;
			p = newLinePtr;
			len = 0;
			newLine = 0;
			newLineWidth = 0;
			continue;
		}

		buff[ len++ ] = *p++;

		if ( buff[ len - 1 ] == 13 )
		{
			buff[ len - 1 ] = ' ';
		}

		buff[ len ] = '\0';
	}
}

static void UI_DrawCampaignMapDescription( rectDef_t *rect, float scale, vec4_t color, float text_x, float text_y, int textStyle,
    int align, qboolean net, int number )
{
	int        campaign = ( net ) ? ui_currentNetCampaign.integer : ui_currentCampaign.integer;
	const char *p, *textPtr, *newLinePtr;
	char       buff[ 1024 ];
	int        height, len, textWidth, newLine, newLineWidth;
	float      y;
	rectDef_t  textRect;

	if ( campaign < 0 || campaign > uiInfo.campaignCount )
	{
		if ( net )
		{
			ui_currentNetCampaign.integer = 0;
			trap_Cvar_Set( "ui_currentNetCampaign", "0" );
		}
		else
		{
			ui_currentCampaign.integer = 0;
			trap_Cvar_Set( "ui_currentCampaign", "0" );
		}

		campaign = 0;
	}

	if ( !uiInfo.campaignList[ campaign ].unlocked || uiInfo.campaignList[ campaign ].progress < number ||
	     !uiInfo.campaignList[ campaign ].mapInfos[ number ] )
	{
		textPtr = "No information is available for this region.";
	}
	else
	{
		textPtr = uiInfo.campaignList[ campaign ].mapInfos[ number ]->briefing;
	}

	if ( !textPtr || !*textPtr )
	{
		textPtr = "^1No text supplied";
	}

	height = Text_Height( textPtr, scale, 0 );

	textRect.x = 0;
	textRect.y = 0;
	textRect.w = rect->w;
	textRect.h = rect->h;

	textWidth = 0;
	newLinePtr = NULL;
	y = text_y;
	len = 0;
	buff[ 0 ] = '\0';
	newLine = 0;
	newLineWidth = 0;
	p = textPtr;

	while ( p )
	{
		if ( *p == ' ' || *p == '\t' || *p == '\n' || *p == '\0' )
		{
			newLine = len;
			newLinePtr = p + 1;
			newLineWidth = textWidth;
		}

		textWidth = Text_Width( buff, scale, 0 );

		if ( ( newLine && textWidth > rect->w ) || *p == '\n' || *p == '\0' )
		{
			if ( len )
			{
				if ( align == ITEM_ALIGN_LEFT )
				{
					textRect.x = text_x;
				}
				else if ( align == ITEM_ALIGN_RIGHT )
				{
					textRect.x = text_x - newLineWidth;
				}
				else if ( align == ITEM_ALIGN_CENTER )
				{
					textRect.x = text_x - newLineWidth / 2;
				}

				textRect.y = y;

				textRect.x += rect->x;
				textRect.y += rect->y;

				//
				buff[ newLine ] = '\0';
				Text_Paint( textRect.x, textRect.y, scale, color, buff, 0, 0, textStyle );
			}

			if ( *p == '\0' )
			{
				break;
			}

			//
			y += height + 5;
			p = newLinePtr;
			len = 0;
			newLine = 0;
			newLineWidth = 0;
			continue;
		}

		buff[ len++ ] = *p++;

		if ( buff[ len - 1 ] == 13 )
		{
			buff[ len - 1 ] = ' ';
		}

		buff[ len ] = '\0';
	}
}

static void UI_DrawCampaignMapPreview( rectDef_t *rect, float scale, vec4_t color, qboolean net, int map )
{
	int campaign = ( net ) ? ui_currentNetCampaign.integer : ui_currentCampaign.integer;

	if ( campaign < 0 || campaign > uiInfo.campaignCount )
	{
		if ( net )
		{
			ui_currentNetCampaign.integer = 0;
			trap_Cvar_Set( "ui_currentNetCampaign", "0" );
		}
		else
		{
			ui_currentCampaign.integer = 0;
			trap_Cvar_Set( "ui_currentCampaign", "0" );
		}

		campaign = 0;
	}

	if ( uiInfo.campaignList[ campaign ].mapInfos[ map ] && uiInfo.campaignList[ campaign ].mapInfos[ map ]->levelShot == -1 )
	{
		uiInfo.campaignList[ campaign ].mapInfos[ map ]->levelShot =
		  trap_R_RegisterShaderNoMip( uiInfo.campaignList[ campaign ].mapInfos[ map ]->imageName );
	}

	if ( uiInfo.campaignList[ campaign ].mapInfos[ map ] && uiInfo.campaignList[ campaign ].mapInfos[ map ]->levelShot > 0 )
	{
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.campaignList[ campaign ].mapInfos[ map ]->levelShot );

		if ( uiInfo.campaignList[ campaign ].progress < map )
		{
			UI_DrawHandlePic( rect->x + 8, rect->y + 8, rect->w - 16, rect->h - 16,
			                  trap_R_RegisterShaderNoMip( "gfx/2d/friendlycross.tga" ) );
		}
	}
	else
	{
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip( "levelshots/unknownmap" ) );

		if ( uiInfo.campaignList[ campaign ].progress < map )
		{
			UI_DrawHandlePic( rect->x + 8, rect->y + 8, rect->w - 16, rect->h - 16,
			                  trap_R_RegisterShaderNoMip( "gfx/2d/friendlycross.tga" ) );
		}
	}
}

static void UI_DrawMissionBriefingMap( rectDef_t *rect )
{
	char             buffer[ 64 ];
	static qhandle_t image = -1;

	if ( image == -1 )
	{
		trap_Cvar_VariableStringBuffer( "mapname", buffer, 64 );
		image = trap_R_RegisterShaderNoMip( va( "levelshots/%s_cc.tga", buffer ) );
	}

	if ( image )
	{
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, image );
	}
	else
	{
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip( "levelshots/unknownmap" ) );
	}
}

static void UI_DrawMissionBriefingTitle( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	char    buffer[ 64 ];
	mapInfo *mi;

	trap_Cvar_VariableStringBuffer( "mapname", buffer, 64 );

	mi = UI_FindMapInfoByMapname( buffer );

	if ( !mi )
	{
		return;
	}

	Text_Paint( rect->x, rect->y, scale, color, va( "%s Objectives", mi->mapName ), 0, 0, textStyle );
}

static void UI_DrawMissionBriefingObjectives( rectDef_t *rect, float scale, vec4_t color, float text_x, float text_y,
    int textStyle, int align )
{
	const char *p, *textPtr, *newLinePtr;
	char       buff[ 1024 ];
	int        height, len, textWidth, newLine, newLineWidth;
	float      y;
	rectDef_t  textRect;

	char       buffer[ 64 ];
	mapInfo    *mi;

	trap_Cvar_VariableStringBuffer( "mapname", buffer, 64 );

	mi = UI_FindMapInfoByMapname( buffer );

	if ( !mi )
	{
		return;
	}

	textPtr = mi->objectives;

	height = Text_Height( textPtr, scale, 0 );

	textRect.x = 0;
	textRect.y = 0;
	textRect.w = rect->w;
	textRect.h = rect->h;

	textWidth = 0;
	newLinePtr = NULL;
	y = text_y;
	len = 0;
	buff[ 0 ] = '\0';
	newLine = 0;
	newLineWidth = 0;
	p = textPtr;

	while ( p )
	{
		if ( *p == ' ' || *p == '\t' || *p == '\n' || *p == '\0' )
		{
			newLine = len;
			newLinePtr = p + 1;
			newLineWidth = textWidth;
		}

		textWidth = Text_Width( buff, scale, 0 );

		if ( ( newLine && textWidth > rect->w ) || *p == '\n' || *p == '\0' )
		{
			if ( len )
			{
				if ( align == ITEM_ALIGN_LEFT )
				{
					textRect.x = text_x;
				}
				else if ( align == ITEM_ALIGN_RIGHT )
				{
					textRect.x = text_x - newLineWidth;
				}
				else if ( align == ITEM_ALIGN_CENTER )
				{
					textRect.x = text_x - newLineWidth / 2;
				}

				textRect.y = y;

				textRect.x += rect->x;
				textRect.y += rect->y;

				//
				buff[ newLine ] = '\0';
				Text_Paint( textRect.x, textRect.y, scale, color, buff, 0, 0, textStyle );
			}

			if ( *p == '\0' )
			{
				break;
			}

			//
			y += height + 5;
			p = newLinePtr;
			len = 0;
			newLine = 0;
			newLineWidth = 0;
			continue;
		}

		buff[ len++ ] = *p++;

		if ( buff[ len - 1 ] == 13 )
		{
			buff[ len - 1 ] = ' ';
		}

		buff[ len ] = '\0';
	}
}

static qboolean updateModel = qtrue;
static qboolean q3Model = qfalse;

static void UI_DrawPlayerModel( rectDef_t *rect )
{
	static playerInfo_t info;
	char                model[ MAX_QPATH ];
	char                team[ 256 ];
	char                head[ 256 ];
	vec3_t              viewangles;
	static vec3_t       moveangles = { 0, 0, 0 };

	if ( trap_Cvar_VariableValue( "ui_Q3Model" ) )
	{
		// NERVE - SMF
		int teamval;

		teamval = trap_Cvar_VariableValue( "mp_team" );

		if ( teamval == ALLIES_TEAM )
		{
			strcpy( model, "multi" );
		}
		else
		{
			strcpy( model, "multi_axis" );
		}

		// -NERVE - SMF

		strcpy( head, UI_Cvar_VariableString( "headmodel" ) );

		if ( !q3Model )
		{
			q3Model = qtrue;
			updateModel = qtrue;
		}

		team[ 0 ] = '\0';
	}
	else
	{
		strcpy( model, UI_Cvar_VariableString( "team_model" ) );
		strcpy( head, UI_Cvar_VariableString( "team_headmodel" ) );
		strcpy( team, UI_Cvar_VariableString( "ui_teamName" ) );

		if ( q3Model )
		{
			q3Model = qfalse;
			updateModel = qtrue;
		}
	}

	moveangles[ YAW ] += 1; // NERVE - SMF - TEMPORARY

	// compare new cvars to old cvars and see if we need to update
	{
		int v1, v2;

		v1 = trap_Cvar_VariableValue( "mp_team" );
		v2 = trap_Cvar_VariableValue( "ui_prevTeam" );

		if ( v1 != v2 )
		{
			trap_Cvar_Set( "ui_prevTeam", va( "%i", v1 ) );
			updateModel = qtrue;
		}

		v1 = trap_Cvar_VariableValue( "mp_playerType" );
		v2 = trap_Cvar_VariableValue( "ui_prevClass" );

		if ( v1 != v2 )
		{
			trap_Cvar_Set( "ui_prevClass", va( "%i", v1 ) );
			updateModel = qtrue;
		}

		v1 = trap_Cvar_VariableValue( "mp_weapon" );
		v2 = trap_Cvar_VariableValue( "ui_prevWeapon" );

		if ( v1 != v2 )
		{
			trap_Cvar_Set( "ui_prevWeapon", va( "%i", v1 ) );
			updateModel = qtrue;
		}
	}

	if ( updateModel )
	{
		// NERVE - SMF - TEMPORARY
		memset( &info, 0, sizeof( playerInfo_t ) );
		viewangles[ YAW ] = 180 - 10;
		viewangles[ PITCH ] = 0;
		viewangles[ ROLL ] = 0;
//      VectorClear( moveangles );
		UI_PlayerInfo_SetModel( &info, model );
		UI_PlayerInfo_SetInfo( &info, LEGS_IDLE, TORSO_STAND, viewangles, moveangles, -1, qfalse );
//      UI_RegisterClientModelname( &info, model, head, team);
		updateModel = qfalse;
	}
	else
	{
		VectorCopy( moveangles, info.moveAngles );
	}

//  info.moveAngles[YAW] += 1;
//   UI_PlayerInfo_SetInfo( &info, LEGS_IDLE, TORSO_STAND, viewangles, moveangles, WP_MP40, qfalse );
	UI_DrawPlayer( rect->x, rect->y, rect->w, rect->h, &info, uiInfo.uiDC.realTime / 2 );
}

/*static void UI_DrawNetSource(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
        if (ui_netSource.integer < 0 || ui_netSource.integer > numNetSources ) {
                ui_netSource.integer = 0;
        }
        Text_Paint(rect->x, rect->y, scale, color, trap_TranslateString( va("Source: %s", netSources[ui_netSource.integer]) ), 0, 0, textStyle);
}*/

/*static void UI_DrawNetMapPreview(rectDef_t *rect, float scale, vec4_t color) {

        if (uiInfo.serverStatus.currentServerPreview > 0) {
                UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.serverStatus.currentServerPreview);
        } else {
                UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip("levelshots/unknownmap"));
        }
}*/

/*static void UI_DrawNetMapCinematic(rectDef_t *rect, float scale, vec4_t color) {
        if (ui_currentNetMap.integer < 0 || ui_currentNetMap.integer > uiInfo.mapCount) {
                ui_currentNetMap.integer = 0;
                trap_Cvar_Set("ui_currentNetMap", "0");
        }

        if (uiInfo.serverStatus.currentServerCinematic >= 0) {
                trap_CIN_RunCinematic(uiInfo.serverStatus.currentServerCinematic);
                trap_CIN_SetExtents(uiInfo.serverStatus.currentServerCinematic, rect->x, rect->y, rect->w, rect->h);
                trap_CIN_DrawCinematic(uiInfo.serverStatus.currentServerCinematic);
        } else {
                UI_DrawNetMapPreview(rect, scale, color);
        }
}*/

static void UI_DrawNetFilter( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	if ( ui_serverFilterType.integer < 0 || ui_serverFilterType.integer > numServerFilters )
	{
		ui_serverFilterType.integer = 0;
	}

	Text_Paint( rect->x, rect->y, scale, color, va( "Filter: %s", serverFilters[ ui_serverFilterType.integer ].description ), 0, 0,
	            textStyle );
}

/*static void UI_DrawTier(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
  int i;
        i = trap_Cvar_VariableValue( "ui_currentTier" );
  if (i < 0 || i >= uiInfo.tierCount) {
        i = 0;
  }
  Text_Paint(rect->x, rect->y, scale, color, va("Tier: %s", uiInfo.tierList[i].tierName),0, 0, textStyle);
}

static void UI_DrawTierMap(rectDef_t *rect, int index) {
  int i;
        i = trap_Cvar_VariableValue( "ui_currentTier" );
  if (i < 0 || i >= uiInfo.tierCount) {
        i = 0;
  }

        if (uiInfo.tierList[i].mapHandles[index] == -1) {
                uiInfo.tierList[i].mapHandles[index] = trap_R_RegisterShaderNoMip(va("levelshots/%s", uiInfo.tierList[i].maps[index]));
        }

        UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.tierList[i].mapHandles[index]);
}

static const char *UI_EnglishMapName(const char *map) {
        int i;
        for (i = 0; i < uiInfo.mapCount; i++) {
                if (Q_stricmp(map, uiInfo.mapList[i].mapLoadName) == 0) {
                        return uiInfo.mapList[i].mapName;
                }
        }
        return "";
}

static void UI_DrawTierMapName(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
  int i, j;
        i = trap_Cvar_VariableValue( "ui_currentTier" );
  if (i < 0 || i >= uiInfo.tierCount) {
        i = 0;
  }
        j = trap_Cvar_VariableValue("ui_currentMap");
        if (j < 0 || j > MAPS_PER_TIER) {
                j = 0;
        }

  Text_Paint(rect->x, rect->y, scale, color, UI_EnglishMapName(uiInfo.tierList[i].maps[j]), 0, 0, textStyle);
}

static void UI_DrawTierGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
  int i, j;
        i = trap_Cvar_VariableValue( "ui_currentTier" );
  if (i < 0 || i >= uiInfo.tierCount) {
        i = 0;
  }
        j = trap_Cvar_VariableValue("ui_currentMap");
        if (j < 0 || j > MAPS_PER_TIER) {
                j = 0;
        }

  Text_Paint(rect->x, rect->y, scale, color, uiInfo.gameTypes[uiInfo.tierList[i].gameTypes[j]].gameType , 0, 0, textStyle);
}*/

/*
// TTimo: unused
static const char *UI_OpponentLeaderName() {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
        return uiInfo.teamList[i].teamMembers[0];
}
*/

/*
// TTimo: unused
static const char *UI_AIFromName(const char *name) {
        int j;
        for (j = 0; j < uiInfo.aliasCount; j++) {
                if (Q_stricmp(uiInfo.aliasList[j].name, name) == 0) {
                        return uiInfo.aliasList[j].ai;
                }
        }
        return "James";
}
*/

/*
// TTimo: unused
static const int UI_AIIndex(const char *name) {
        int j;
        for (j = 0; j < uiInfo.characterCount; j++) {
                if (Q_stricmp(name, uiInfo.characterList[j].name) == 0) {
                        return j;
                }
        }
        return 0;
}
*/

/*
// TTimo: unused
static const int UI_AIIndexFromName(const char *name) {
        int j;
        for (j = 0; j < uiInfo.aliasCount; j++) {
                if (Q_stricmp(uiInfo.aliasList[j].name, name) == 0) {
                        return UI_AIIndex(uiInfo.aliasList[j].ai);
                }
        }
        return 0;
}
*/

/*
// TTimo: unused
static const char *UI_OpponentLeaderHead() {
        const char *leader = UI_OpponentLeaderName();
        return UI_AIFromName(leader);
}
*/

/*
// TTimo: unused
static const char *UI_OpponentLeaderModel() {
        int i;
        const char *head = UI_OpponentLeaderHead();
        for (i = 0; i < uiInfo.characterCount; i++) {
                if (Q_stricmp(head, uiInfo.characterList[i].name) == 0) {
                        if (uiInfo.characterList[i].female) {
                                return "Janet";
                        } else {
                                return "James";
                        }
                }
        }
        return "James";
}
*/

/*static qboolean updateOpponentModel = qtrue;
static void UI_DrawOpponent(rectDef_t *rect) {
  static playerInfo_t info2;
  char model[MAX_QPATH];
  char headmodel[MAX_QPATH];
  char team[256];
        vec3_t  viewangles;
        vec3_t  moveangles;

        if (updateOpponentModel) {

                strcpy(model, UI_Cvar_VariableString("ui_opponentModel"));
          strcpy(headmodel, UI_Cvar_VariableString("ui_opponentModel"));
                team[0] = '\0';

        memset( &info2, 0, sizeof(playerInfo_t) );
        viewangles[YAW]   = 180 - 10;
        viewangles[PITCH] = 0;
        viewangles[ROLL]  = 0;
        VectorClear( moveangles );
        UI_PlayerInfo_SetModel( &info2, model);
        UI_PlayerInfo_SetInfo( &info2, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_MP40, qfalse );
        UI_RegisterClientModelname( &info2, model);
        updateOpponentModel = qfalse;
  }

  UI_DrawPlayer( rect->x, rect->y, rect->w, rect->h, &info2, uiInfo.uiDC.realTime / 2);

}*/

static void UI_NextOpponent()
{
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_opponentName" ) );
	int j = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );

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

	trap_Cvar_Set( "ui_opponentName", uiInfo.teamList[ i ].teamName );
}

static void UI_PriorOpponent()
{
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_opponentName" ) );
	int j = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );

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

	trap_Cvar_Set( "ui_opponentName", uiInfo.teamList[ i ].teamName );
}

static void UI_DrawPlayerLogo( rectDef_t *rect, vec3_t color )
{
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );

	if ( uiInfo.teamList[ i ].teamIcon == -1 )
	{
		uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip( uiInfo.teamList[ i ].imageName );
		uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip( va( "%s_metal", uiInfo.teamList[ i ].imageName ) );
		uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[ i ].imageName ) );
	}

	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon );
	trap_R_SetColor( NULL );
}

static void UI_DrawPlayerLogoMetal( rectDef_t *rect, vec3_t color )
{
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );

	if ( uiInfo.teamList[ i ].teamIcon == -1 )
	{
		uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip( uiInfo.teamList[ i ].imageName );
		uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip( va( "%s_metal", uiInfo.teamList[ i ].imageName ) );
		uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[ i ].imageName ) );
	}

	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon_Metal );
	trap_R_SetColor( NULL );
}

static void UI_DrawPlayerLogoName( rectDef_t *rect, vec3_t color )
{
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );

	if ( uiInfo.teamList[ i ].teamIcon == -1 )
	{
		uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip( uiInfo.teamList[ i ].imageName );
		uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip( va( "%s_metal", uiInfo.teamList[ i ].imageName ) );
		uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[ i ].imageName ) );
	}

	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon_Name );
	trap_R_SetColor( NULL );
}

static void UI_DrawOpponentLogo( rectDef_t *rect, vec3_t color )
{
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_opponentName" ) );

	if ( uiInfo.teamList[ i ].teamIcon == -1 )
	{
		uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip( uiInfo.teamList[ i ].imageName );
		uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip( va( "%s_metal", uiInfo.teamList[ i ].imageName ) );
		uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[ i ].imageName ) );
	}

	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon );
	trap_R_SetColor( NULL );
}

static void UI_DrawOpponentLogoMetal( rectDef_t *rect, vec3_t color )
{
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_opponentName" ) );

	if ( uiInfo.teamList[ i ].teamIcon == -1 )
	{
		uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip( uiInfo.teamList[ i ].imageName );
		uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip( va( "%s_metal", uiInfo.teamList[ i ].imageName ) );
		uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[ i ].imageName ) );
	}

	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon_Metal );
	trap_R_SetColor( NULL );
}

static void UI_DrawOpponentLogoName( rectDef_t *rect, vec3_t color )
{
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_opponentName" ) );

	if ( uiInfo.teamList[ i ].teamIcon == -1 )
	{
		uiInfo.teamList[ i ].teamIcon = trap_R_RegisterShaderNoMip( uiInfo.teamList[ i ].imageName );
		uiInfo.teamList[ i ].teamIcon_Metal = trap_R_RegisterShaderNoMip( va( "%s_metal", uiInfo.teamList[ i ].imageName ) );
		uiInfo.teamList[ i ].teamIcon_Name = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[ i ].imageName ) );
	}

	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[ i ].teamIcon_Name );
	trap_R_SetColor( NULL );
}

static void UI_DrawAllMapsSelection( rectDef_t *rect, float scale, vec4_t color, int textStyle, qboolean net )
{
}

static void UI_DrawOpponentName( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	Text_Paint( rect->x, rect->y, scale, color, UI_Cvar_VariableString( "ui_opponentName" ), 0, 0, textStyle );
}

static int UI_OwnerDrawWidth( int ownerDraw, float scale )
{
	int        i, h, value;
	const char *text;
	const char *s = NULL;

	switch ( ownerDraw )
	{
		case UI_HANDICAP:
			h = Com_Clamp( 5, 100, trap_Cvar_VariableValue( "handicap" ) );
			i = 20 - h / 5;
			s = handicapValues[ i ];
			break;

		case UI_CLANNAME:
			s = UI_Cvar_VariableString( "ui_teamName" );
			break;

		case UI_GAMETYPE:
			s = uiInfo.gameTypes[ ui_gameType.integer ].gameType;
			break;

			/*case UI_SKILL:
			   i = trap_Cvar_VariableValue( "g_spSkill" );
			   if (i < 1 || i > numSkillLevels) {
			   i = 1;
			   }
			   s = skillLevels[i-1];
			   break; */
		case UI_BLUETEAMNAME:
			i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_blueTeam" ) );

			if ( i >= 0 && i < uiInfo.teamCount )
			{
				s = va( "%s: %s", "Blue", uiInfo.teamList[ i ].teamName );
			}

			break;

		case UI_REDTEAMNAME:
			i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_redTeam" ) );

			if ( i >= 0 && i < uiInfo.teamCount )
			{
				s = va( "%s: %s", "Red", uiInfo.teamList[ i ].teamName );
			}

			break;

		case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
			value = trap_Cvar_VariableValue( va( "ui_blueteam%i", ownerDraw - UI_BLUETEAM1 + 1 ) );

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

			s = va( "%i. %s", ownerDraw - UI_BLUETEAM1 + 1, text );
			break;

		case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
			value = trap_Cvar_VariableValue( va( "ui_redteam%i", ownerDraw - UI_REDTEAM1 + 1 ) );

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

			s = va( "%i. %s", ownerDraw - UI_REDTEAM1 + 1, text );
			break;

			/*    case UI_NETSOURCE:
			                        if (ui_netSource.integer < 0 || ui_netSource.integer > uiInfo.numJoinGameTypes) {
			                                ui_netSource.integer = 0;
			                        }
			                        s = va("Source: %s", netSources[ui_netSource.integer]);
			                        break;*/
		case UI_NETFILTER:
			if ( ui_serverFilterType.integer < 0 || ui_serverFilterType.integer > numServerFilters )
			{
				ui_serverFilterType.integer = 0;
			}

			s = va( "Filter: %s", serverFilters[ ui_serverFilterType.integer ].description );
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
				s = trap_TranslateString( "Waiting for new key... Press ESCAPE to cancel" );
			}
			else
			{
				s = trap_TranslateString( "Press ENTER or CLICK to change, Press BACKSPACE to clear" );
			}

			break;

		case UI_SERVERREFRESHDATE:
			s = UI_Cvar_VariableString( va( "ui_lastServerRefresh_%i", ui_netSource.integer ) );
			break;

		default:
			break;
	}

	if ( s )
	{
		return Text_Width( s, scale, 0 );
	}

	return 0;
}

static void UI_DrawBotName( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
}

/*static void UI_DrawBotSkill(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
        if (uiInfo.skillIndex >= 0 && uiInfo.skillIndex < numSkillLevels) {
          Text_Paint(rect->x, rect->y, scale, color, skillLevels[uiInfo.skillIndex], 0, 0, textStyle);
        }
}*/

static void UI_DrawRedBlue( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	Text_Paint( rect->x, rect->y, scale, color, ( uiInfo.redBlue == 0 ) ? "Red" : "Blue", 0, 0, textStyle );
}

static void UI_DrawCrosshair( rectDef_t *rect, float scale, vec4_t color )
{
	float size = cg_crosshairSize.integer;

	if ( uiInfo.currentCrosshair < 0 || uiInfo.currentCrosshair >= NUM_CROSSHAIRS )
	{
		uiInfo.currentCrosshair = 0;
	}

	size = ( rect->w / 96.0f ) * ( ( size > 96.0f ) ? 96.0f : ( ( size < 24.0f ) ? 24.0f : size ) );

	trap_R_SetColor( uiInfo.xhairColor );
	UI_DrawHandlePic( rect->x + ( rect->w - size ) / 2, rect->y + ( rect->h - size ) / 2, size, size,
	                  uiInfo.uiDC.Assets.crosshairShader[ uiInfo.currentCrosshair ] );
	trap_R_SetColor( uiInfo.xhairColorAlt );
	UI_DrawHandlePic( rect->x + ( rect->w - size ) / 2, rect->y + ( rect->h - size ) / 2, size, size,
	                  uiInfo.uiDC.Assets.crosshairAltShader[ uiInfo.currentCrosshair ] );

	trap_R_SetColor( NULL );
}

/*
===============
UI_BuildPlayerList
===============
*/
static void UI_BuildPlayerList()
{
	uiClientState_t cs;
	int             n, count, team, team2, playerTeamNumber, muted;
	char            info[ MAX_INFO_STRING ];
	char            namebuf[ 64 ];

	trap_GetClientState( &cs );
	trap_GetConfigString( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
	uiInfo.playerNumber = cs.clientNum;
	uiInfo.teamLeader = atoi( Info_ValueForKey( info, "tl" ) );
	team = atoi( Info_ValueForKey( info, "t" ) );
	trap_GetConfigString( CS_SERVERINFO, info, sizeof( info ) );
	count = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	uiInfo.playerCount = 0;
	uiInfo.myTeamCount = 0;
	playerTeamNumber = 0;

	for ( n = 0; n < count; n++ )
	{
		trap_GetConfigString( CS_PLAYERS + n, info, MAX_INFO_STRING );

		if ( info[ 0 ] )
		{
			Q_strncpyz( namebuf, Info_ValueForKey( info, "n" ), sizeof( namebuf ) );
// fretn - dont expand colors twice, so: foo^^xbar -> foo^bar -> fooar
//          Q_CleanStr( namebuf );
			Q_strncpyz( uiInfo.playerNames[ uiInfo.playerCount ], namebuf, sizeof( uiInfo.playerNames[ 0 ] ) );
			muted = atoi( Info_ValueForKey( info, "mu" ) );

			if ( muted )
			{
				uiInfo.playerMuted[ uiInfo.playerCount ] = qtrue;
			}
			else
			{
				uiInfo.playerMuted[ uiInfo.playerCount ] = qfalse;
			}

			uiInfo.playerRefereeStatus[ uiInfo.playerCount ] = atoi( Info_ValueForKey( info, "ref" ) );
			uiInfo.playerCount++;
			team2 = atoi( Info_ValueForKey( info, "t" ) );

			if ( team2 == team )
			{
				Q_strncpyz( namebuf, Info_ValueForKey( info, "n" ), sizeof( namebuf ) );
// fretn - dont expand colors twice, so: foo^^xbar -> foo^bar -> fooar
//              Q_CleanStr( namebuf );
				Q_strncpyz( uiInfo.teamNames[ uiInfo.myTeamCount ], namebuf, sizeof( uiInfo.teamNames[ 0 ] ) );
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
		trap_Cvar_Set( "cg_selectedPlayer", va( "%d", playerTeamNumber ) );
	}

	n = trap_Cvar_VariableValue( "cg_selectedPlayer" );

	if ( n < 0 || n > uiInfo.myTeamCount )
	{
		n = 0;
	}

	if ( n < uiInfo.myTeamCount )
	{
		trap_Cvar_Set( "cg_selectedPlayerName", uiInfo.teamNames[ n ] );
	}
}

static void UI_DrawSelectedPlayer( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	if ( uiInfo.uiDC.realTime > uiInfo.playerRefresh )
	{
		uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
		UI_BuildPlayerList();
	}

	Text_Paint( rect->x, rect->y, scale, color,
	            ( uiInfo.teamLeader ) ? UI_Cvar_VariableString( "cg_selectedPlayerName" ) : UI_Cvar_VariableString( "name" ), 0, 0,
	            textStyle );
}

static void UI_DrawServerRefreshDate( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	int serverCount; // NERVE - SMF

	if ( uiInfo.serverStatus.refreshActive )
	{
		vec4_t lowLight, newColor;

		lowLight[ 0 ] = 0.8 * color[ 0 ];
		lowLight[ 1 ] = 0.8 * color[ 1 ];
		lowLight[ 2 ] = 0.8 * color[ 2 ];
		lowLight[ 3 ] = 0.8 * color[ 3 ];
		LerpColor( color, lowLight, newColor, 0.5 + 0.5 * sin( uiInfo.uiDC.realTime / PULSE_DIVISOR ) );
		// NERVE - SMF
		serverCount = trap_LAN_GetServerCount( ui_netSource.integer );

		if ( serverCount >= 0 )
		{
			Text_Paint( rect->x, rect->y, scale, newColor,
			            va( trap_TranslateString( "Getting info for %d servers (ESC to cancel)" ), serverCount ), 0, 0, textStyle );
		}
		else
		{
			Text_Paint( rect->x, rect->y, scale, newColor, trap_TranslateString( "Waiting for response from Master Server" ), 0, 0,
			            textStyle );
		}
	}
	else
	{
		char buff[ 64 ];

		Q_strncpyz( buff, UI_Cvar_VariableString( va( "ui_lastServerRefresh_%i", ui_netSource.integer ) ), 64 );
		Text_Paint( rect->x, rect->y, scale, color, va( trap_TranslateString( "Refresh Time: %s" ), buff ), 0, 0, textStyle );
	}
}

static void UI_DrawServerMOTD( rectDef_t *rect, float scale, vec4_t color )
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
					uiInfo.serverStatus.motdPaintX +=
					  Text_Width( &uiInfo.serverStatus.motd[ uiInfo.serverStatus.motdOffset ], scale, 1 ) - 1;
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
		Text_Paint_Limit( &maxX, uiInfo.serverStatus.motdPaintX, rect->y, scale, color,
		                  &uiInfo.serverStatus.motd[ uiInfo.serverStatus.motdOffset ], 0, 0 );

		if ( uiInfo.serverStatus.motdPaintX2 >= 0 )
		{
			float maxX2 = rect->x + rect->w - 2;

			Text_Paint_Limit( &maxX2, uiInfo.serverStatus.motdPaintX2, rect->y, scale, color, uiInfo.serverStatus.motd, 0,
			                  uiInfo.serverStatus.motdOffset );
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

static void UI_DrawKeyBindStatus( rectDef_t *rect, float scale, vec4_t color, int textStyle, float text_x, float text_y )
{
	//int ofs = 0; // TTimo: unused
	if ( Display_KeyBindPending() )
	{
		Text_Paint( rect->x + text_x, rect->y + text_y, scale, color,
		            trap_TranslateString( "Waiting for new key... Press ESCAPE to cancel" ), 0, 0, textStyle );
	}
	else
	{
		Text_Paint( rect->x + text_x, rect->y + text_y, scale, color,
		            trap_TranslateString( "Press ENTER or CLICK to change, Press BACKSPACE to clear" ), 0, 0, textStyle );
	}
}

static void UI_ParseGLConfig( void )
{
	char *eptr;

	uiInfo.numGlInfoLines = 0;

	eptr = uiInfo.uiDC.glconfig.extensions_string;

	while ( *eptr )
	{
		while ( *eptr && *eptr == ' ' )
		{
			*eptr++ = '\0';
		}

		// track start of valid string
		if ( *eptr && *eptr != ' ' )
		{
			uiInfo.glInfoLines[ uiInfo.numGlInfoLines++ ] = eptr;
		}

		if ( uiInfo.numGlInfoLines == GLINFO_LINES )
		{
			break; // Arnout: failsafe
		}

		while ( *eptr && *eptr != ' ' )
		{
			eptr++;
		}
	}

	uiInfo.numGlInfoLines += 4; // vendor, version and pixelformat + a whiteline
}

// FIXME: table drive
//
static void UI_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags,
                          int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle )
{
	rectDef_t rect;

	rect.x = x + text_x;
	rect.y = y + text_y;
	rect.w = w;
	rect.h = h;

	switch ( ownerDraw )
	{
			/*    case UI_TEAMFLAG:
			                        UI_DrawFlag( &rect );
			                        break;*/
		case UI_HANDICAP:
			UI_DrawHandicap( &rect, scale, color, textStyle );
			break;

		case UI_EFFECTS:
			UI_DrawEffects( &rect, scale, color );
			break;

		case UI_PLAYERMODEL:
			UI_DrawPlayerModel( &rect );
			break;

		case UI_CLANNAME:
			UI_DrawClanName( &rect, scale, color, textStyle );
			break;

		case UI_SAVEGAME_SHOT: // (SA)
			UI_DrawSaveGameShot( &rect, scale, color );
			break;

		case UI_CLANLOGO:
			UI_DrawClanLogo( &rect, scale, color );
			break;

		case UI_CLANCINEMATIC:
			UI_DrawClanCinematic( &rect, scale, color );
			break;

		case UI_PREVIEWCINEMATIC:
			UI_DrawPreviewCinematic( &rect, scale, color );
			break;

		case UI_GAMETYPE:
			UI_DrawGameType( &rect, scale, color, textStyle );
			break;

			/*    case UI_NETGAMETYPE:
			                        UI_DrawNetGameType(&rect, scale, color, textStyle);
			                        break;*/

			/*    case UI_JOINGAMETYPE:
			                        UI_DrawJoinGameType(&rect, scale, color, textStyle);
			                        break;*/
		case UI_MAPPREVIEW:
			UI_DrawMapPreview( &rect, scale, color, qtrue );
			break;

		case UI_NETMAPPREVIEW:
			UI_DrawNetMapPreview( &rect, scale, color, qtrue );
			break;

			/*    case UI_MAP_TIMETOBEAT:
			                        UI_DrawMapTimeToBeat(&rect, scale, color, textStyle);
			                        break;*/
		case UI_MAPCINEMATIC:
			UI_DrawMapCinematic( &rect, scale, color, qfalse );
			break;

		case UI_STARTMAPCINEMATIC:
			UI_DrawMapCinematic( &rect, scale, color, qtrue );
			break;

		case UI_CAMPAIGNCINEMATIC:
			UI_DrawCampaignCinematic( &rect, scale, color, qfalse );
			break;

		case UI_CAMPAIGNNAME:
			UI_DrawCampaignName( &rect, scale, color, textStyle, qfalse );
			break;

		case UI_CAMPAIGNDESCRIPTION:
			UI_DrawCampaignDescription( &rect, scale, color, text_x, text_y, textStyle, align, qtrue );
			break;

		case UI_GAMETYPEDESCRIPTION:
			UI_DrawGametypeDescription( &rect, scale, color, text_x, text_y, textStyle, align, qtrue );
			break;

		case UI_CAMPAIGNMAP1_SHOT:
		case UI_CAMPAIGNMAP2_SHOT:
		case UI_CAMPAIGNMAP3_SHOT:
		case UI_CAMPAIGNMAP4_SHOT:
		case UI_CAMPAIGNMAP5_SHOT:
		case UI_CAMPAIGNMAP6_SHOT:
			UI_DrawCampaignMapPreview( &rect, scale, color, qfalse, ownerDraw - UI_CAMPAIGNMAP1_SHOT );
			break;

		case UI_CAMPAIGNMAP1_TEXT:
		case UI_CAMPAIGNMAP2_TEXT:
		case UI_CAMPAIGNMAP3_TEXT:
		case UI_CAMPAIGNMAP4_TEXT:
		case UI_CAMPAIGNMAP5_TEXT:
		case UI_CAMPAIGNMAP6_TEXT:
			UI_DrawCampaignMapDescription( &rect, scale, color, text_x, text_y, textStyle, align, qfalse,
			                               ownerDraw - UI_CAMPAIGNMAP1_TEXT );
			break;

		case UI_MB_MAP:
			UI_DrawMissionBriefingMap( &rect );
			break;

		case UI_MB_TITLE:
			UI_DrawMissionBriefingTitle( &rect, scale, color, textStyle );
			break;

		case UI_MB_OBJECTIVES:
			UI_DrawMissionBriefingObjectives( &rect, scale, color, text_x, text_y, textStyle, align );
			break;

			/*    case UI_SKILL:
			                        UI_DrawSkill(&rect, scale, color, textStyle);
			                        break;*/
		case UI_BLUETEAMNAME:
			UI_DrawTeamName( &rect, scale, color, qtrue, textStyle );
			break;

		case UI_REDTEAMNAME:
			UI_DrawTeamName( &rect, scale, color, qfalse, textStyle );
			break;

		case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
			UI_DrawTeamMember( &rect, scale, color, qtrue, ownerDraw - UI_BLUETEAM1 + 1, textStyle );
			break;

		case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
			UI_DrawTeamMember( &rect, scale, color, qfalse, ownerDraw - UI_REDTEAM1 + 1, textStyle );
			break;

			/*    case UI_NETSOURCE:
			                        UI_DrawNetSource(&rect, scale, color, textStyle);
			                        break;*/

			/*case UI_NETMAPPREVIEW:
			   UI_DrawNetMapPreview(&rect, scale, color);
			   break;
			   case UI_NETMAPCINEMATIC:
			   UI_DrawNetMapCinematic(&rect, scale, color);
			   break; */
		case UI_NETFILTER:
			UI_DrawNetFilter( &rect, scale, color, textStyle );
			break;

			/*    case UI_TIER:
			                        UI_DrawTier(&rect, scale, color, textStyle);
			                        break;*/

			/*    case UI_OPPONENTMODEL:
			                        UI_DrawOpponent(&rect);
			                        break;*/

			/*    case UI_TIERMAP1:
			                        UI_DrawTierMap(&rect, 0);
			                        break;
			                case UI_TIERMAP2:
			                        UI_DrawTierMap(&rect, 1);
			                        break;
			                case UI_TIERMAP3:
			                        UI_DrawTierMap(&rect, 2);
			                        break;*/
		case UI_PLAYERLOGO:
			UI_DrawPlayerLogo( &rect, color );
			break;

		case UI_PLAYERLOGO_METAL:
			UI_DrawPlayerLogoMetal( &rect, color );
			break;

		case UI_PLAYERLOGO_NAME:
			UI_DrawPlayerLogoName( &rect, color );
			break;

		case UI_OPPONENTLOGO:
			UI_DrawOpponentLogo( &rect, color );
			break;

		case UI_OPPONENTLOGO_METAL:
			UI_DrawOpponentLogoMetal( &rect, color );
			break;

		case UI_OPPONENTLOGO_NAME:
			UI_DrawOpponentLogoName( &rect, color );
			break;

			/*    case UI_TIER_MAPNAME:
			                        UI_DrawTierMapName(&rect, scale, color, textStyle);
			                        break;
			                case UI_TIER_GAMETYPE:
			                        UI_DrawTierGameType(&rect, scale, color, textStyle);
			                        break;*/
		case UI_ALLMAPS_SELECTION:
			UI_DrawAllMapsSelection( &rect, scale, color, textStyle, qtrue );
			break;

		case UI_MAPS_SELECTION:
			UI_DrawAllMapsSelection( &rect, scale, color, textStyle, qfalse );
			break;

		case UI_OPPONENT_NAME:
			UI_DrawOpponentName( &rect, scale, color, textStyle );
			break;

		case UI_BOTNAME:
			UI_DrawBotName( &rect, scale, color, textStyle );
			break;

			/*    case UI_BOTSKILL:
			                        UI_DrawBotSkill(&rect, scale, color, textStyle);
			                        break;*/
		case UI_REDBLUE:
			UI_DrawRedBlue( &rect, scale, color, textStyle );
			break;

		case UI_CROSSHAIR:
			UI_DrawCrosshair( &rect, scale, color );
			break;

		case UI_SELECTEDPLAYER:
			UI_DrawSelectedPlayer( &rect, scale, color, textStyle );
			break;

		case UI_SERVERREFRESHDATE:
			UI_DrawServerRefreshDate( &rect, scale, color, textStyle );
			break;

		case UI_SERVERMOTD:
			UI_DrawServerMOTD( &rect, scale, color );
			break;

		case UI_KEYBINDSTATUS:
			UI_DrawKeyBindStatus( &rect, scale, color, textStyle, text_x, text_y );
			break;

		case UI_LOADPANEL:
			UI_DrawLoadPanel( qfalse, qtrue, qfalse );
			break;

		default:
			break;
	}
}

qboolean UI_OwnerDrawVisible( int flags )
{
	qboolean vis = qtrue;

	while ( flags )
	{
		if ( flags & UI_SHOW_FFA )
		{
			flags &= ~UI_SHOW_FFA;
		}

		if ( flags & UI_SHOW_NOTFFA )
		{
			vis = qfalse;
			flags &= ~UI_SHOW_NOTFFA;
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
				if ( ui_selectedPlayer.integer < uiInfo.myTeamCount &&
				     uiInfo.teamClientNums[ ui_selectedPlayer.integer ] == uiInfo.playerNumber )
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
				if ( !
				     ( ui_selectedPlayer.integer < uiInfo.myTeamCount &&
				       uiInfo.teamClientNums[ ui_selectedPlayer.integer ] == uiInfo.playerNumber ) )
				{
					vis = qfalse;
				}
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

		if ( flags & UI_SHOW_ANYTEAMGAME )
		{
			flags &= ~UI_SHOW_ANYTEAMGAME;
		}

		if ( flags & UI_SHOW_ANYNONTEAMGAME )
		{
			vis = qfalse;
			flags &= ~UI_SHOW_ANYNONTEAMGAME;
		}

		if ( flags & UI_SHOW_NETANYTEAMGAME )
		{
			flags &= ~UI_SHOW_NETANYTEAMGAME;
		}

		if ( flags & UI_SHOW_NETANYNONTEAMGAME )
		{
			vis = qfalse;
			flags &= ~UI_SHOW_NETANYNONTEAMGAME;
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
					if ( trap_Cvar_VariableValue( "sv_killserver" ) == 0 )
					{
						// wait on server to go down before playing sound
						trap_S_StartLocalSound( uiInfo.newHighScoreSound, CHAN_ANNOUNCER );
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

		if ( flags & UI_SHOW_CAMPAIGNMAP1EXISTS )
		{
			if ( uiInfo.campaignList[ ui_currentCampaign.integer ].mapCount < 1 )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_CAMPAIGNMAP1EXISTS;
		}

		if ( flags & UI_SHOW_CAMPAIGNMAP2EXISTS )
		{
			if ( uiInfo.campaignList[ ui_currentCampaign.integer ].mapCount < 2 )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_CAMPAIGNMAP2EXISTS;
		}

		if ( flags & UI_SHOW_CAMPAIGNMAP3EXISTS )
		{
			if ( uiInfo.campaignList[ ui_currentCampaign.integer ].mapCount < 3 )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_CAMPAIGNMAP3EXISTS;
		}

		if ( flags & UI_SHOW_CAMPAIGNMAP4EXISTS )
		{
			if ( uiInfo.campaignList[ ui_currentCampaign.integer ].mapCount < 4 )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_CAMPAIGNMAP4EXISTS;
		}

		if ( flags & UI_SHOW_CAMPAIGNMAP5EXISTS )
		{
			if ( uiInfo.campaignList[ ui_currentCampaign.integer ].mapCount < 5 )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_CAMPAIGNMAP5EXISTS;
		}

		if ( flags & UI_SHOW_CAMPAIGNMAP6EXISTS )
		{
			if ( uiInfo.campaignList[ ui_currentCampaign.integer ].mapCount < 6 )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_CAMPAIGNMAP6EXISTS;
		}

		if ( flags & UI_SHOW_SELECTEDCAMPAIGNMAPPLAYABLE )
		{
			int map = trap_Cvar_VariableValue( "ui_campaignmap" );

			if ( map > uiInfo.campaignList[ ui_currentCampaign.integer ].progress )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_SELECTEDCAMPAIGNMAPPLAYABLE;
		}

		if ( flags & UI_SHOW_SELECTEDCAMPAIGNMAPNOTPLAYABLE )
		{
			int map = trap_Cvar_VariableValue( "ui_campaignmap" );

			if ( map <= uiInfo.campaignList[ ui_currentCampaign.integer ].progress )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_SELECTEDCAMPAIGNMAPNOTPLAYABLE;
		}

		if ( flags & UI_SHOW_PLAYERMUTED )
		{
			if ( !uiInfo.playerMuted[ uiInfo.playerIndex ] )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_PLAYERMUTED;
		}

		if ( flags & UI_SHOW_PLAYERNOTMUTED )
		{
			if ( uiInfo.playerMuted[ uiInfo.playerIndex ] )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_PLAYERNOTMUTED;
		}

		if ( flags & UI_SHOW_PLAYERNOREFEREE )
		{
			if ( uiInfo.playerRefereeStatus[ uiInfo.playerIndex ] != RL_NONE )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_PLAYERNOREFEREE;
		}

		if ( flags & UI_SHOW_PLAYERREFEREE )
		{
			if ( uiInfo.playerRefereeStatus[ uiInfo.playerIndex ] != RL_REFEREE )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_PLAYERREFEREE;
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

static qboolean UI_Handicap_HandleKey( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		int h;

		h = Com_Clamp( 5, 100, trap_Cvar_VariableValue( "handicap" ) );

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

		trap_Cvar_Set( "handicap", va( "%i", h ) );
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_Effects_HandleKey( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		if ( key == K_MOUSE2 )
		{
			uiInfo.effectsColor--;
		}
		else
		{
			uiInfo.effectsColor++;
		}

		if ( uiInfo.effectsColor > 6 )
		{
			uiInfo.effectsColor = 0;
		}
		else if ( uiInfo.effectsColor < 0 )
		{
			uiInfo.effectsColor = 6;
		}

		trap_Cvar_SetValue( "color", uitogamecode[ uiInfo.effectsColor ] );
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_ClanName_HandleKey( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		int i;

		i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );

		if ( uiInfo.teamList[ i ].cinematic >= 0 )
		{
			trap_CIN_StopCinematic( uiInfo.teamList[ i ].cinematic );
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

		trap_Cvar_Set( "ui_teamName", uiInfo.teamList[ i ].teamName );
		updateModel = qtrue;
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_GameType_HandleKey( int flags, float *special, int key, qboolean resetMap )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		int oldCount = UI_MapCountByGameType( qtrue );

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

		trap_Cvar_Set( "ui_Q3Model", "0" );

		trap_Cvar_Set( "ui_gameType", va( "%d", ui_gameType.integer ) );
		UI_SetCapFragLimits( qtrue );
		UI_LoadBestScores( uiInfo.mapList[ ui_currentMap.integer ].mapLoadName, uiInfo.gameTypes[ ui_gameType.integer ].gtEnum );

		if ( resetMap && oldCount != UI_MapCountByGameType( qtrue ) )
		{
			trap_Cvar_Set( "ui_currentMap", "0" );
			Menu_SetFeederSelection( NULL, FEEDER_MAPS, 0, NULL );
		}

		return qtrue;
	}

	return qfalse;
}

/*static qboolean UI_NetGameType_HandleKey(int flags, float *special, int key) {
  if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {

                if (key == K_MOUSE2) {
                        ui_netGameType.integer--;
                } else {
                        ui_netGameType.integer++;
                }

        if (ui_netGameType.integer < 0) {
          ui_netGameType.integer = uiInfo.numGameTypes - 1;
                } else if (ui_netGameType.integer >= uiInfo.numGameTypes) {
          ui_netGameType.integer = 0;
        }

        trap_Cvar_Set( "ui_netGameType", va("%d", ui_netGameType.integer));
        trap_Cvar_Set( "ui_actualnetGameType", va("%d", uiInfo.gameTypes[ui_netGameType.integer].gtEnum));
        trap_Cvar_Set( "ui_currentNetMap", "0");
        trap_Cvar_Set( "ui_currentNetCampaign", "0");
        UI_MapCountByGameType(qfalse);
        Menu_SetFeederSelection(NULL, FEEDER_ALLMAPS, 0, NULL);
        Menu_SetFeederSelection(NULL, FEEDER_ALLCAMPAIGNS, 0, NULL);
        return qtrue;
  }
  return qfalse;
}*/

/*static qboolean UI_JoinGameType_HandleKey(int flags, float *special, int key) {
        if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {

                if (key == K_MOUSE2) {
                        ui_joinGameType.integer--;
                } else {
                        ui_joinGameType.integer++;
                }

                if (ui_joinGameType.integer < 0) {
                        ui_joinGameType.integer = uiInfo.numJoinGameTypes - 1;
                } else if (ui_joinGameType.integer >= uiInfo.numJoinGameTypes) {
                        ui_joinGameType.integer = 0;
                }

                trap_Cvar_Set( "ui_joinGameType", va("%d", ui_joinGameType.integer));
                UI_BuildServerDisplayList(qtrue);
                return qtrue;
        }
        return qfalse;
}*/

/*static qboolean UI_Skill_HandleKey(int flags, float *special, int key) {
  if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
        int i = trap_Cvar_VariableValue( "g_spSkill" );

                if (key == K_MOUSE2) {
                i--;
                } else {
                i++;
                }

        if (i < 1) {
                        i = numSkillLevels;
                } else if (i > numSkillLevels) {
          i = 1;
        }

        trap_Cvar_Set("g_spSkill", va("%i", i));
        return qtrue;
  }
  return qfalse;
}*/

static qboolean UI_TeamName_HandleKey( int flags, float *special, int key, qboolean blue )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		int i;

		i = UI_TeamIndexFromName( UI_Cvar_VariableString( ( blue ) ? "ui_blueTeam" : "ui_redTeam" ) );

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

		trap_Cvar_Set( ( blue ) ? "ui_blueTeam" : "ui_redTeam", uiInfo.teamList[ i ].teamName );

		return qtrue;
	}

	return qfalse;
}

static qboolean UI_TeamMember_HandleKey( int flags, float *special, int key, qboolean blue, int num )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		// 0 - None
		// 1 - Human
		// 2..NumCharacters - Bot
		char *cvar = va( blue ? "ui_blueteam%i" : "ui_redteam%i", num );
		int  value = trap_Cvar_VariableValue( cvar );

		if ( key == K_MOUSE2 )
		{
			value--;
		}
		else
		{
			value++;
		}

		if ( value >= uiInfo.characterCount + 2 )
		{
			value = 0;
		}
		else if ( value < 0 )
		{
			value = uiInfo.characterCount + 2 - 1;
		}

		trap_Cvar_Set( cvar, va( "%i", value ) );
		return qtrue;
	}

	return qfalse;
}

/*static qboolean UI_NetSource_HandleKey(int flags, float *special, int key) {
        if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
                if (key == K_MOUSE2) {
                        ui_netSource.integer--;
                } else {
                        ui_netSource.integer++;
                }

                if (ui_netSource.integer >= numNetSources) {
                        ui_netSource.integer = 0;
                } else if (ui_netSource.integer < 0) {
                        ui_netSource.integer = numNetSources - 1;
                }

                UI_BuildServerDisplayList(qtrue);
                if (ui_netSource.integer != AS_GLOBAL) {
                        UI_StartServerRefresh(qtrue);
                }
                trap_Cvar_Set( "ui_netSource", va("%d", ui_netSource.integer));
                return qtrue;
        }
        return qfalse;
}*/

static qboolean UI_NetFilter_HandleKey( int flags, float *special, int key )
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

		UI_BuildServerDisplayList( qtrue );
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_OpponentName_HandleKey( int flags, float *special, int key )
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

static qboolean UI_BotName_HandleKey( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
//      int game = trap_Cvar_VariableValue("g_gametype");
		int value = uiInfo.botIndex;

		if ( key == K_MOUSE2 )
		{
			value--;
		}
		else
		{
			value++;
		}

		if ( value >= uiInfo.characterCount + 2 )
		{
			value = 0;
		}
		else if ( value < 0 )
		{
			value = uiInfo.characterCount + 2 - 1;
		}

		uiInfo.botIndex = value;
		return qtrue;
	}

	return qfalse;
}

/*static qboolean UI_BotSkill_HandleKey(int flags, float *special, int key) {
  if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
                if (key == K_MOUSE2) {
                        uiInfo.skillIndex--;
                } else {
                        uiInfo.skillIndex++;
                }
                if (uiInfo.skillIndex >= numSkillLevels) {
                        uiInfo.skillIndex = 0;
                } else if (uiInfo.skillIndex < 0) {
                        uiInfo.skillIndex = numSkillLevels-1;
                }
        return qtrue;
  }
        return qfalse;
}*/

static qboolean UI_RedBlue_HandleKey( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		uiInfo.redBlue ^= 1;
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_Crosshair_HandleKey( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		if ( key == K_MOUSE2 )
		{
			uiInfo.currentCrosshair--;
		}
		else
		{
			uiInfo.currentCrosshair++;
		}

		if ( uiInfo.currentCrosshair >= NUM_CROSSHAIRS )
		{
			uiInfo.currentCrosshair = 0;
		}
		else if ( uiInfo.currentCrosshair < 0 )
		{
			uiInfo.currentCrosshair = NUM_CROSSHAIRS - 1;
		}

		trap_Cvar_Set( "cg_drawCrosshair", va( "%d", uiInfo.currentCrosshair ) );
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_SelectedPlayer_HandleKey( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		int selected;

		UI_BuildPlayerList();

		if ( !uiInfo.teamLeader )
		{
			return qfalse;
		}

		selected = trap_Cvar_VariableValue( "cg_selectedPlayer" );

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
			trap_Cvar_Set( "cg_selectedPlayerName", "Everyone" );
		}
		else
		{
			trap_Cvar_Set( "cg_selectedPlayerName", uiInfo.teamNames[ selected ] );
		}

		trap_Cvar_Set( "cg_selectedPlayer", va( "%d", selected ) );
	}

	return qfalse;
}

static qboolean UI_OwnerDrawHandleKey( int ownerDraw, int flags, float *special, int key )
{
	switch ( ownerDraw )
	{
		case UI_HANDICAP:
			return UI_Handicap_HandleKey( flags, special, key );
			break;

		case UI_EFFECTS:
			return UI_Effects_HandleKey( flags, special, key );
			break;

		case UI_CLANNAME:
			return UI_ClanName_HandleKey( flags, special, key );
			break;

		case UI_GAMETYPE:
			return UI_GameType_HandleKey( flags, special, key, qtrue );
			break;

			/*    case UI_NETGAMETYPE:
			          return UI_NetGameType_HandleKey(flags, special, key);
			          break;*/

			/*case UI_JOINGAMETYPE:
			   return UI_JoinGameType_HandleKey(flags, special, key);
			   break; */

			/*case UI_SKILL:
			   return UI_Skill_HandleKey(flags, special, key);
			   break; */
		case UI_BLUETEAMNAME:
			return UI_TeamName_HandleKey( flags, special, key, qtrue );
			break;

		case UI_REDTEAMNAME:
			return UI_TeamName_HandleKey( flags, special, key, qfalse );
			break;

		case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
			UI_TeamMember_HandleKey( flags, special, key, qtrue, ownerDraw - UI_BLUETEAM1 + 1 );
			break;

		case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
			UI_TeamMember_HandleKey( flags, special, key, qfalse, ownerDraw - UI_REDTEAM1 + 1 );
			break;

			/*    case UI_NETSOURCE:
			          UI_NetSource_HandleKey(flags, special, key);
			                        break;*/
		case UI_NETFILTER:
			UI_NetFilter_HandleKey( flags, special, key );
			break;

		case UI_OPPONENT_NAME:
			UI_OpponentName_HandleKey( flags, special, key );
			break;

		case UI_BOTNAME:
			return UI_BotName_HandleKey( flags, special, key );
			break;

			/*case UI_BOTSKILL:
			   return UI_BotSkill_HandleKey(flags, special, key);
			   break; */
		case UI_REDBLUE:
			UI_RedBlue_HandleKey( flags, special, key );
			break;

		case UI_CROSSHAIR:
			UI_Crosshair_HandleKey( flags, special, key );
			break;

		case UI_SELECTEDPLAYER:
			UI_SelectedPlayer_HandleKey( flags, special, key );
			break;

		default:
			break;
	}

	return qfalse;
}

static float UI_GetValue( int ownerDraw, int type )
{
	return 0;
}

/*
=================
UI_ServersQsortCompare
=================
*/
static int QDECL UI_ServersQsortCompare( const void *arg1, const void *arg2 )
{
	return trap_LAN_CompareServers( ui_netSource.integer, uiInfo.serverStatus.sortKey, uiInfo.serverStatus.sortDir, * ( int * ) arg1,
	                                * ( int * ) arg2 );
}

/*
=================
UI_ServersSort
=================
*/
void UI_ServersSort( int column, qboolean force )
{
	if ( !force )
	{
		if ( uiInfo.serverStatus.sortKey == column )
		{
			return;
		}
	}

	uiInfo.serverStatus.sortKey = column;
	qsort( &uiInfo.serverStatus.displayServers[ 0 ], uiInfo.serverStatus.numDisplayServers, sizeof( int ), UI_ServersQsortCompare );
}

/*
===============
UI_LoadMods
===============
*/
static void UI_LoadMods()
{
	int  numdirs;
	char dirlist[ 2048 ];
	char *dirptr;
	char *descptr;
	int  i;
	int  dirlen;

	uiInfo.modCount = 0;
	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof( dirlist ) );
	dirptr = dirlist;

	for ( i = 0; i < numdirs; i++ )
	{
		dirlen = strlen( dirptr ) + 1;
		descptr = dirptr + dirlen;
		uiInfo.modList[ uiInfo.modCount ].modName = String_Alloc( dirptr );
		uiInfo.modList[ uiInfo.modCount ].modDescr = String_Alloc( descptr );
		dirptr += dirlen + strlen( descptr ) + 1;
		uiInfo.modCount++;

		if ( uiInfo.modCount >= MAX_MODS )
		{
			break;
		}
	}
}

/*
===============
UI_LoadProfiles
===============
*/
static void UI_LoadProfiles()
{
	int  numdirs;
	char dirlist[ 2048 ];
	char *dirptr;

	//char  *descptr;
	int  i;
	int  dirlen;

	uiInfo.profileCount = 0;
	uiInfo.profileIndex = -1;
	numdirs = trap_FS_GetFileList( "profiles", "/", dirlist, sizeof( dirlist ) );
	dirptr = dirlist;

	for ( i = 0; i < numdirs; i++ )
	{
		dirlen = strlen( dirptr ) + 1;

		if ( dirptr[ 0 ] && Q_stricmp( dirptr, "." ) && Q_stricmp( dirptr, ".." ) )
		{
			int        handle;
			pc_token_t token;

			if ( !( handle = trap_PC_LoadSource( va( "profiles/%s/profile.dat", dirptr ) ) ) )
			{
				dirptr += dirlen;
				continue;
			}

			if ( !trap_PC_ReadToken( handle, &token ) )
			{
				trap_PC_FreeSource( handle );
				dirptr += dirlen;
				continue;
			}

			uiInfo.profileList[ uiInfo.profileCount ].name = String_Alloc( token.string );
			trap_PC_FreeSource( handle );

			uiInfo.profileList[ uiInfo.profileCount ].dir = String_Alloc( dirptr );
			uiInfo.profileCount++;

			/*if( uiInfo.profileCount == 1 ) {
			   int j;

			   uiInfo.profileIndex = 0;
			   trap_Cvar_Set( "ui_profile", token.string );

			   for( j = 0; j < Menu_Count(); j++ ) {
			   Menu_SetFeederSelection( Menu_Get(j), FEEDER_PROFILES, 0, NULL );
			   }
			   } */
			if ( uiInfo.profileIndex == -1 )
			{
				Q_CleanStr( token.string );
				Q_CleanDirName( token.string );

				if ( !Q_stricmp( token.string, cl_profile.string ) )
				{
					int j;

					uiInfo.profileIndex = i;
					trap_Cvar_Set( "ui_profile", uiInfo.profileList[ 0 ].name );
					trap_Cvar_Update( &ui_profile );

					for ( j = 0; j < Menu_Count(); j++ )
					{
						Menu_SetFeederSelection( Menu_Get( j ), FEEDER_PROFILES, uiInfo.profileIndex, NULL );
					}
				}
			}

			if ( uiInfo.profileCount >= MAX_PROFILES )
			{
				break;
			}
		}

		dirptr += dirlen;
	}

	if ( uiInfo.profileIndex == -1 )
	{
		int j;

		uiInfo.profileIndex = 0;
		trap_Cvar_Set( "ui_profile", uiInfo.profileList[ 0 ].name );
		trap_Cvar_Update( &ui_profile );

		for ( j = 0; j < Menu_Count(); j++ )
		{
			Menu_SetFeederSelection( Menu_Get( j ), FEEDER_PROFILES, 0, NULL );
		}
	}
}

/*
===============
UI_LoadTeams
===============
*/

/*
// TTimo: unused
static void UI_LoadTeams() {
        char  teamList[4096];
        char  *teamName;
        int   i, len, count;

        count = trap_FS_GetFileList( "", "team", teamList, 4096 );

        if (count) {
                teamName = teamList;
                for ( i = 0; i < count; i++ ) {
                        len = strlen( teamName );
                        UI_ParseTeamInfo(teamName);
                        teamName += len + 1;
                }
        }
}
*/

/*
==============
UI_DelSavegame
==============
*/
static void UI_DelSavegame()
{
	int ret;

	ret = trap_FS_Delete( va( "save/%s.svg", uiInfo.savegameList[ uiInfo.savegameIndex ].name ) );
	trap_FS_Delete( va( "save/images/%s.tga", uiInfo.savegameList[ uiInfo.savegameIndex ].name ) );

	if ( ret )
	{
		Com_Printf( "Deleted savegame: %s.svg\n", uiInfo.savegameList[ uiInfo.savegameIndex ].name );
	}
	else
	{
		Com_Printf( "Unable to delete savegame: %s.svg\n", uiInfo.savegameList[ uiInfo.savegameIndex ].name );
	}
}

/*
==============
UI_LoadSavegames
==============
*/
static void UI_LoadSavegames()
{
	char sglist[ 4096 ];
	char *sgname;
	int  i, len;

	uiInfo.savegameCount = trap_FS_GetFileList( "save", "svg", sglist, 4096 );

	if ( uiInfo.savegameCount )
	{
		if ( uiInfo.savegameCount > MAX_SAVEGAMES )
		{
			uiInfo.savegameCount = MAX_SAVEGAMES;
		}

		sgname = sglist;

		for ( i = 0; i < uiInfo.savegameCount; i++ )
		{
			len = strlen( sgname );

			if ( !Q_strncmp( sgname, "current", 7 ) )
			{
				// ignore current.svg since it has special uses and shouldn't be loaded directly
				i--;
				uiInfo.savegameCount -= 1;
				sgname += len + 1;
				continue;
			}

			if ( !Q_stricmp( sgname + len - 4, ".svg" ) )
			{
				sgname[ len - 4 ] = '\0';
			}

			Q_strupr( sgname );
			uiInfo.savegameList[ i ].name = String_Alloc( sgname );
			uiInfo.savegameList[ i ].sshotImage = trap_R_RegisterShaderNoMip( va( "save/images/%s.tga", uiInfo.savegameList[ i ].name ) );
			sgname += len + 1;
		}
	}
}

/*
===============
UI_LoadMovies
===============
*/
static void UI_LoadMovies()
{
	char movielist[ 4096 ];
	char *moviename;
	int  i, len;

	uiInfo.movieCount = trap_FS_GetFileList( "video", "roq", movielist, 4096 );

	if ( uiInfo.movieCount )
	{
		if ( uiInfo.movieCount > MAX_MOVIES )
		{
			uiInfo.movieCount = MAX_MOVIES;
		}

		moviename = movielist;

		for ( i = 0; i < uiInfo.movieCount; i++ )
		{
			len = strlen( moviename );

			if ( !Q_stricmp( moviename + len - 4, ".roq" ) )
			{
				moviename[ len - 4 ] = '\0';
			}

			Q_strupr( moviename );
			uiInfo.movieList[ i ] = String_Alloc( moviename );
			moviename += len + 1;
		}
	}
}

/*
===============
UI_LoadDemos
===============
*/
static void UI_LoadDemos()
{
	char demolist[ 30000 ];
	char demoExt[ 32 ];
	char *demoname;
	int  i, len;

	Com_sprintf( demoExt, sizeof( demoExt ), "dm_%d", ( int ) trap_Cvar_VariableValue( "protocol" ) );

	uiInfo.demoCount = trap_FS_GetFileList( "demos", demoExt, demolist, sizeof( demolist ) );

	Com_sprintf( demoExt, sizeof( demoExt ), ".dm_%d", ( int ) trap_Cvar_VariableValue( "protocol" ) );

	if ( uiInfo.demoCount )
	{
		if ( uiInfo.demoCount > MAX_DEMOS )
		{
			uiInfo.demoCount = MAX_DEMOS;
		}

		demoname = demolist;

		for ( i = 0; i < uiInfo.demoCount; i++ )
		{
			len = strlen( demoname );

			if ( !Q_stricmp( demoname + len - strlen( demoExt ), demoExt ) )
			{
				demoname[ len - strlen( demoExt ) ] = '\0';
			}

//          Q_strupr(demoname);
			uiInfo.demoList[ i ] = String_Alloc( demoname );
			demoname += len + 1;
		}
	}
}

/*
==============
UI_SetNextMap
==============
*/

/*
// TTimo: unused
static qboolean UI_SetNextMap(int actual, int index) {
        int i;
        for (i = actual + 1; i < uiInfo.mapCount; i++) {
                if (uiInfo.mapList[i].active) {
                        Menu_SetFeederSelection(NULL, FEEDER_MAPS, index + 1, "skirmish");
                        return qtrue;
                }
        }
        return qfalse;
}
*/

/*
==============
UI_StartSkirmish
==============
*/
static void UI_StartSkirmish( qboolean next )
{
}

void WM_setItemPic( char *name, const char *shader )
{
	menuDef_t *menu = Menu_GetFocused();
	itemDef_t *item;

	item = Menu_FindItemByName( menu, name );

	if ( item )
	{
		item->window.background = DC->registerShaderNoMip( shader );
	}
}

void WM_setVisibility( char *name, qboolean show )
{
	menuDef_t *menu = Menu_GetFocused();
	itemDef_t *item;

	item = Menu_FindItemByName( menu, name );

	if ( item )
	{
		if ( show )
		{
			item->window.flags |= WINDOW_VISIBLE;
		}
		else
		{
			item->window.flags &= ~( WINDOW_VISIBLE | WINDOW_MOUSEOVER );
		}
	}
}

qboolean UI_CheckExecKey( int key )
{
	menuDef_t *menu = Menu_GetFocused();

	if ( g_editingField )
	{
		return qtrue;
	}

	if ( key > 256 )
	{
		return qfalse;
	}

	if ( !menu )
	{
		if ( cl_bypassMouseInput.integer )
		{
			if ( !trap_Key_GetCatcher() )
			{
				trap_Cvar_Set( "cl_bypassMouseInput", "0" );
			}
		}

		return qfalse;
	}

	if ( menu->onKey[ key ] )
	{
		return qtrue;
	}

	return qfalse;
}

void WM_ActivateLimboChat()
{
	menuDef_t *menu;
	itemDef_t *itemdef;

	menu = Menu_GetFocused();
	menu = Menus_ActivateByName( "wm_limboChat", qtrue );

	if ( !menu || g_editItem )
	{
		return;
	}

	itemdef = Menu_FindItemByName( menu, "window_limbo_chat" );

	if ( itemdef )
	{
		itemdef->cursorPos = 0;
		g_editingField = qtrue;
		g_editItem = itemdef;
		DC->setOverstrikeMode( qtrue );
	}
}

// -NERVE - SMF

/*
==============
UI_Update
==============
*/
void UI_Update( const char *name )
{
	int val = trap_Cvar_VariableValue( name );

	if ( Q_stricmp( name, "ui_SetName" ) == 0 )
	{
		trap_Cvar_Set( "name", UI_Cvar_VariableString( "ui_Name" ) );
	}
	else if ( Q_stricmp( name, "ui_setRate" ) == 0 )
	{
		float rate = trap_Cvar_VariableValue( "ui_rate" );

		if ( rate >= 20000 )
		{
			trap_Cvar_Set( "ui_cl_maxpackets", "100" );
			trap_Cvar_Set( "ui_cl_packetdup", "2" );
		}
		else if ( rate >= 5000 )
		{
			trap_Cvar_Set( "ui_cl_maxpackets", "30" );
			trap_Cvar_Set( "ui_cl_packetdup", "1" );
		}
		else if ( rate >= 4000 )
		{
			trap_Cvar_Set( "ui_cl_maxpackets", "15" );
			trap_Cvar_Set( "ui_cl_packetdup", "2" );  // favor less prediction errors when there's packet loss
		}
		else
		{
			trap_Cvar_Set( "ui_cl_maxpackets", "15" );
			trap_Cvar_Set( "ui_cl_packetdup", "1" );  // favor lower bandwidth
		}
	}
	else if ( Q_stricmp( name, "ui_cl_packetdup" ) == 0 )
	{
		trap_Cvar_SetValue( "cl_packetdup", val );
	}
	else if ( Q_stricmp( name, "ui_cl_maxpackets" ) == 0 )
	{
		trap_Cvar_SetValue( "cl_maxpackets", val );
	}
	else if ( Q_stricmp( name, "ui_GetName" ) == 0 )
	{
		trap_Cvar_Set( "ui_Name", UI_Cvar_VariableString( "name" ) );
	}
	else if ( Q_stricmp( name, "r_colorbits" ) == 0 )
	{
		switch ( val )
		{
			case 0:
				trap_Cvar_SetValue( "r_depthbits", 0 );
				trap_Cvar_SetValue( "r_stencilbits", 0 );
				break;

			case 16:
				trap_Cvar_SetValue( "r_depthbits", 16 );
				trap_Cvar_SetValue( "r_stencilbits", 0 );
				break;

			case 32:
				trap_Cvar_SetValue( "r_depthbits", 24 );
				break;
		}
	}
	else if ( Q_stricmp( name, "ui_r_lodbias" ) == 0 )
	{
		switch ( val )
		{
			case 0:
				trap_Cvar_SetValue( "ui_r_subdivisions", 4 );
				break;

			case 1:
				trap_Cvar_SetValue( "ui_r_subdivisions", 12 );
				break;

			case 2:
				trap_Cvar_SetValue( "ui_r_subdivisions", 20 );
				break;
		}
	}
	else if ( Q_stricmp( name, "ui_glCustom" ) == 0 )
	{
		switch ( val )
		{
			case 0: // high quality
				trap_Cmd_ExecuteText( EXEC_APPEND, "exec preset_high_ui.cfg\n" );
				break;

			case 1: // normal
				trap_Cmd_ExecuteText( EXEC_APPEND, "exec preset_normal_ui.cfg\n" );
				break;

			case 2: // fast
				trap_Cmd_ExecuteText( EXEC_APPEND, "exec preset_fast_ui.cfg\n" );
				break;

			case 3: // fastest
				trap_Cmd_ExecuteText( EXEC_APPEND, "exec preset_fastest_ui.cfg\n" );
				break;
		}
	}
	else if ( Q_stricmp( name, "ui_mousePitch" ) == 0 )
	{
		if ( val == 0 )
		{
			trap_Cvar_SetValue( "m_pitch", 0.022f );
		}
		else
		{
			trap_Cvar_SetValue( "m_pitch", -0.022f );
		}
	}
}

/*
==============
UI_ResetUICvars
Resets all ui_... values to "" for use when exiting menus
==============
*/
void UI_ResetUICvars( void )
{
	// System Menu
	trap_Cvar_Set( "ui_com_hunkmegs", "" );
	trap_Cvar_Set( "ui_com_soundmegs", "" );
	trap_Cvar_Set( "ui_com_zonemegs", "" );
	trap_Cvar_Set( "ui_cl_maxpackets", "" );
	trap_Cvar_Set( "ui_cl_packetdup", "" );
	trap_Cvar_Set( "ui_s_khz", "" );
	trap_Cvar_Set( "ui_rate", "" );

	// Graphics Menu
	trap_Cvar_Set( "ui_r_mode", "" );
	trap_Cvar_Set( "ui_r_gamma", "" );
	trap_Cvar_Set( "ui_r_colorbits", "" );
	trap_Cvar_Set( "ui_r_fullscreen", "" );
	trap_Cvar_Set( "ui_r_lodbias", "" );
	trap_Cvar_Set( "ui_r_subdivisions", "" );
	trap_Cvar_Set( "ui_r_picmip", "" );
	trap_Cvar_Set( "ui_r_texturebits", "" );
	trap_Cvar_Set( "ui_r_depthbits", "" );
	trap_Cvar_Set( "ui_r_ext_compressed_textures", "" );
	trap_Cvar_Set( "ui_r_finish", "" );
	trap_Cvar_Set( "ui_r_dynamiclight", "" );
	trap_Cvar_Set( "ui_r_allowextensions", "" );
	trap_Cvar_Set( "ui_r_detailtextures", "" );
	trap_Cvar_Set( "ui_r_texturemode", "" );
	trap_Cvar_Set( "ui_cg_shadows", "" );
	trap_Cvar_Set( "ui_r_hdrrendering", "" );
	trap_Cvar_Set( "ui_r_bloom", "" );
	trap_Cvar_Set( "ui_r_normalmapping", "" );
	trap_Cvar_Set( "ui_r_parallaxmapping", "" );
}

/*
==============
UI_RunMenuScript
==============
*/
void UI_RunMenuScript( char **args )
{
	const char *name, *name2;
	char       *s;
	char       buff[ 1024 ];
	int        val; // NERVE - SMF
	menuDef_t  *menu;

	if ( String_Parse( args, &name ) )
	{
		if ( Q_stricmp( name, "StartServer" ) == 0 )
		{
			float skill;
			int   pb_sv, pb_cl;

			// DHM - Nerve
			if ( !ui_dedicated.integer )
			{
				pb_sv = ( int ) trap_Cvar_VariableValue( "sv_punkbuster" );
				pb_cl = ( int ) trap_Cvar_VariableValue( "cl_punkbuster" );

				if ( pb_sv && !pb_cl )
				{
					trap_Cvar_Set( "com_errorMessage",
					               "You must either disable PunkBuster on the Server, or enable PunkBuster on the Client before starting a non-dedicated server." );
					Menus_ActivateByName( "hostGamePopupError", qtrue );
					return;
				}
			}

			// dhm - Nerve

			trap_Cvar_Set( "ui_connecting", "1" );
			trap_Cvar_Set( "cg_thirdPerson", "0" );
			trap_Cvar_Set( "cg_cameraOrbit", "0" );
			trap_Cvar_Set( "ui_singlePlayerActive", "0" );
			trap_Cvar_SetValue( "dedicated", Com_Clamp( 0, 2, ui_dedicated.integer ) );
			trap_Cvar_SetValue( "g_gametype", Com_Clamp( 0, 8, ui_netGameType.integer ) );

			if ( ui_netGameType.integer == GT_WOLF_CAMPAIGN )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND,
				                      va( "wait ; wait ; map %s\n",
				                          uiInfo.campaignList[ ui_currentNetMap.integer ].mapInfos[ 0 ]->mapLoadName ) );
			}
			else
			{
				trap_Cmd_ExecuteText( EXEC_APPEND,
				                      va( "wait ; wait ; map %s\n", uiInfo.mapList[ ui_currentNetMap.integer ].mapLoadName ) );
			}

			skill = trap_Cvar_VariableValue( "g_spSkill" );

			// NERVE - SMF - set user cvars here
			// set timelimit
			val = trap_Cvar_VariableValue( "ui_userTimelimit" );

			if ( val && val != uiInfo.mapList[ ui_mapIndex.integer ].Timelimit )
			{
				trap_Cvar_Set( "g_userTimelimit", va( "%i", val ) );
			}
			else
			{
				trap_Cvar_Set( "g_userTimelimit", "0" );
			}

			// set axis respawn time
			val = trap_Cvar_VariableValue( "ui_userAxisRespawnTime" );

			if ( val && val != uiInfo.mapList[ ui_mapIndex.integer ].AxisRespawnTime )
			{
				trap_Cvar_Set( "g_userAxisRespawnTime", va( "%i", val ) );
			}
			else
			{
				trap_Cvar_Set( "g_userAxisRespawnTime", "0" );
			}

			// set allied respawn time
			val = trap_Cvar_VariableValue( "ui_userAlliedRespawnTime" );

			if ( val && val != uiInfo.mapList[ ui_mapIndex.integer ].AlliedRespawnTime )
			{
				trap_Cvar_Set( "g_userAlliedRespawnTime", va( "%i", val ) );
			}
			else
			{
				trap_Cvar_Set( "g_userAlliedRespawnTime", "0" );
			}

			// -NERVE - SMF
		}
		else if ( Q_stricmp( name, "updateSPMenu" ) == 0 )
		{
			UI_SetCapFragLimits( qtrue );
			UI_MapCountByGameType( qtrue );
			ui_mapIndex.integer = UI_GetIndexFromSelection( ui_currentMap.integer );
			trap_Cvar_Set( "ui_mapIndex", va( "%d", ui_mapIndex.integer ) );
			Menu_SetFeederSelection( NULL, FEEDER_MAPS, ui_mapIndex.integer, "skirmish" );
			ui_campaignIndex.integer = UI_GetIndexFromSelection( ui_currentCampaign.integer );
			trap_Cvar_Set( "ui_campaignIndex", va( "%d", ui_campaignIndex.integer ) );
			Menu_SetFeederSelection( NULL, FEEDER_CAMPAIGNS, ui_campaignIndex.integer, "selectcampaign" );
			UI_GameType_HandleKey( 0, 0, K_MOUSE1, qfalse );
			UI_GameType_HandleKey( 0, 0, K_MOUSE2, qfalse );
		}
		else if ( Q_stricmp( name, "resetDefaults" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n" );  // NERVE - SMF - changed order
			trap_Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n" );
			trap_Cmd_ExecuteText( EXEC_APPEND, "exec language.cfg\n" );  // NERVE - SMF
			trap_Cmd_ExecuteText( EXEC_APPEND, "setRecommended\n" );  // NERVE - SMF
			Controls_SetDefaults( qfalse );
			trap_Cvar_Set( "com_introPlayed", "1" );
			trap_Cvar_Set( "com_recommendedSet", "1" );  // NERVE - SMF
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
		}
		else if ( Q_stricmp( name, "getCDKey" ) == 0 )
		{
			char out[ 17 ];

			trap_GetCDKey( buff, 17 );
			trap_Cvar_Set( "cdkey1", "" );
			trap_Cvar_Set( "cdkey2", "" );
			trap_Cvar_Set( "cdkey3", "" );
			trap_Cvar_Set( "cdkey4", "" );

			if ( strlen( buff ) == CDKEY_LEN )
			{
				Q_strncpyz( out, buff, 5 );
				trap_Cvar_Set( "cdkey1", out );
				Q_strncpyz( out, buff + 4, 5 );
				trap_Cvar_Set( "cdkey2", out );
				Q_strncpyz( out, buff + 8, 5 );
				trap_Cvar_Set( "cdkey3", out );
				Q_strncpyz( out, buff + 12, 5 );
				trap_Cvar_Set( "cdkey4", out );
			}
		}
		else if ( Q_stricmp( name, "loadArenas" ) == 0 )
		{
			UI_LoadArenas();
			UI_MapCountByGameType( qfalse );
			Menu_SetFeederSelection( NULL, FEEDER_ALLMAPS, 0, NULL );
			UI_LoadCampaigns();
			Menu_SetFeederSelection( NULL, FEEDER_ALLCAMPAIGNS, 0, NULL );
		}
		else if ( Q_stricmp( name, "updateNetMap" ) == 0 )
		{
			Menu_SetFeederSelection( NULL, FEEDER_ALLMAPS, ui_currentNetMap.integer, NULL );
		}
		else if ( Q_stricmp( name, "saveControls" ) == 0 )
		{
			Controls_SetConfig( qtrue );
		}
		else if ( Q_stricmp( name, "loadControls" ) == 0 )
		{
			Controls_GetConfig();
		}
		else if ( Q_stricmp( name, "clearError" ) == 0 )
		{
			trap_Cvar_Set( "com_errorMessage", "" );
			trap_Cvar_Set( "com_errorDiagnoseIP", "" );
			trap_Cvar_Set( "com_missingFiles", "" );
		}
		else if ( Q_stricmp( name, "loadGameInfo" ) == 0 )
		{
			UI_ParseGameInfo( "gameinfo.txt" );
//          UI_LoadBestScores(uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum);
		}
		else if ( Q_stricmp( name, "resetScores" ) == 0 )
		{
			UI_ClearScores();
		}
		else if ( Q_stricmp( name, "RefreshServers" ) == 0 )
		{
			UI_StartServerRefresh( qtrue );
			UI_BuildServerDisplayList( qtrue );
		}
		else if ( Q_stricmp( name, "RefreshFilter" ) == 0 )
		{
			//UI_StartServerRefresh(qfalse);
			UI_StartServerRefresh( uiInfo.serverStatus.numDisplayServers ? qfalse : qtrue );  // if we don't have any valid servers, it's kinda safe to assume we would like to get a full new list
			UI_BuildServerDisplayList( qtrue );
		}
		else if ( Q_stricmp( name, "RunSPDemo" ) == 0 )
		{
			if ( uiInfo.demoAvailable )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND,
				                      va( "demo %s_%i", uiInfo.mapList[ ui_currentMap.integer ].mapLoadName,
				                          uiInfo.gameTypes[ ui_gameType.integer ].gtEnum ) );
			}
		}
		else if ( Q_stricmp( name, "LoadDemos" ) == 0 )
		{
			UI_LoadDemos();
		}
		else if ( Q_stricmp( name, "LoadMovies" ) == 0 )
		{
			UI_LoadMovies();

//----(SA)  added
		}
		else if ( Q_stricmp( name, "LoadSaveGames" ) == 0 )
		{
			// get the list
			UI_LoadSavegames();
		}
		else if ( Q_stricmp( name, "Loadgame" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "loadgame %s\n", uiInfo.savegameList[ uiInfo.savegameIndex ].name ) );
		}
		else if ( Q_stricmp( name, "Savegame" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "savegame %s\n", UI_Cvar_VariableString( "ui_savegame" ) ) );
		}
		else if ( Q_stricmp( name, "DelSavegame" ) == 0 )
		{
			UI_DelSavegame();
//----(SA)  end
		}
		else if ( Q_stricmp( name, "LoadMods" ) == 0 )
		{
			UI_LoadMods();
		}
		else if ( Q_stricmp( name, "playMovie" ) == 0 )
		{
			if ( uiInfo.previewMovie >= 0 )
			{
				trap_CIN_StopCinematic( uiInfo.previewMovie );
			}

			trap_Cmd_ExecuteText( EXEC_APPEND, va( "cinematic %s.roq 2\n", uiInfo.movieList[ uiInfo.movieIndex ] ) );
		}
		else if ( Q_stricmp( name, "RunMod" ) == 0 )
		{
			trap_Cvar_Set( "fs_game", uiInfo.modList[ uiInfo.modIndex ].modName );
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		}
		else if ( Q_stricmp( name, "RunDemo" ) == 0 )
		{
			if ( uiInfo.demoIndex >= 0 && uiInfo.demoIndex < uiInfo.demoCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "demo \"%s\"\n", uiInfo.demoList[ uiInfo.demoIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "deleteDemo" ) == 0 )
		{
			if ( uiInfo.demoIndex >= 0 && uiInfo.demoIndex < uiInfo.demoCount )
			{
				trap_FS_Delete( va( "demos/%s.dm_%d", uiInfo.demoList[ uiInfo.demoIndex ], ( int ) trap_Cvar_VariableValue( "protocol" ) ) );
			}
		}
		else if ( Q_stricmp( name, "closeJoin" ) == 0 )
		{
			if ( uiInfo.serverStatus.refreshActive )
			{
				UI_StopServerRefresh();
				uiInfo.serverStatus.nextDisplayRefresh = 0;
				uiInfo.nextServerStatusRefresh = 0;
				uiInfo.nextFindPlayerRefresh = 0;
				UI_BuildServerDisplayList( qtrue );
			}
			else
			{
				Menus_CloseByName( "joinserver" );
				Menus_OpenByName( "main" );
			}
		}
		else if ( Q_stricmp( name, "StopRefresh" ) == 0 )
		{
			UI_StopServerRefresh();
			uiInfo.serverStatus.nextDisplayRefresh = 0;
			uiInfo.nextServerStatusRefresh = 0;
			uiInfo.nextFindPlayerRefresh = 0;
		}
		else if ( Q_stricmp( name, "UpdateFilter" ) == 0 )
		{
			trap_Cvar_Update( &ui_netSource );

			if ( ui_netSource.integer == AS_LOCAL || !uiInfo.serverStatus.numDisplayServers )
			{
				UI_StartServerRefresh( qtrue );
			}

			UI_BuildServerDisplayList( qtrue );
			UI_FeederSelection( FEEDER_SERVERS, 0 );
		}
		else if ( Q_stricmp( name, "check_ServerStatus" ) == 0 )
		{
			s = UI_Cvar_VariableString( "com_errorDiagnoseIP" );
			menu = Menus_FindByName( "ingame_options" );

			if ( strlen( s ) && strcmp( s, "localhost" ) )
			{
				if ( menu )
				{
					Menu_ShowItemByName( menu, "ctr_serverinfo", qtrue );
				}
			}
			else
			{
				if ( menu )
				{
					Menu_ShowItemByName( menu, "ctr_serverinfo", qfalse );
				}
			}
		}
		else if ( Q_stricmp( name, "ServerStatus" ) == 0 )
		{
			// the server info dialog has been turned into a modal thing
			// it can be called in several situations
			if ( trap_Cvar_VariableValue( "ui_serverBrowser" ) == 1 )
			{
				// legacy, from the server browser
				trap_LAN_GetServerAddressString( ui_netSource.integer,
				                                 uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ],
				                                 uiInfo.serverStatusAddress, sizeof( uiInfo.serverStatusAddress ) );
				UI_BuildServerStatus( qtrue );
			}
			else
			{
				// use com_errorDiagnoseIP otherwise
				s = UI_Cvar_VariableString( "com_errorDiagnoseIP" );

				if ( strlen( s ) && strcmp( s, "localhost" ) )
				{
					trap_Cvar_VariableStringBuffer( "com_errorDiagnoseIP", uiInfo.serverStatusAddress,
					                                sizeof( uiInfo.serverStatusAddress ) );
					uiInfo.serverStatus.numDisplayServers = 1; // this is ugly, have to force a non zero display server count to emit the query
					UI_BuildServerStatus( qtrue );
				}
				else
				{
					// we can't close the menu from here, it's not open yet .. (that's the onOpen script)
					Com_Printf( "Can't show Server Info (not found, or local server)\n" );
				}
			}
		}
		else if ( Q_stricmp( name, "InGameServerStatus" ) == 0 )
		{
			uiClientState_t cstate;

			trap_GetClientState( &cstate );
			Q_strncpyz( uiInfo.serverStatusAddress, cstate.servername, sizeof( uiInfo.serverStatusAddress ) );
			UI_BuildServerStatus( qtrue );
		}
		else if ( Q_stricmp( name, "ServerStatus_diagnose" ) == 0 )
		{
			// query server and display the URL buttons if the error happened during a server connection situation
			s = UI_Cvar_VariableString( "com_errorDiagnoseIP" );
			menu = Menus_FindByName( "error_popmenu_diagnose" );

			if ( strlen( s ) && strcmp( s, "localhost" ) )
			{
				trap_Cvar_VariableStringBuffer( "com_errorDiagnoseIP", uiInfo.serverStatusAddress,
				                                sizeof( uiInfo.serverStatusAddress ) );
				uiInfo.serverStatus.numDisplayServers = 1; // this is ugly, have to force a non zero display server count to emit the query

				// toggle the "Server Info" button
				if ( menu )
				{
					Menu_ShowItemByName( menu, "serverinfo", qtrue );
				}

				UI_BuildServerStatus( qtrue );
			}
			else
			{
				// don't send getinfo packet, hide "Server Info" button
				if ( menu )
				{
					Menu_ShowItemByName( menu, "serverinfo", qfalse );
				}
			}
		}
		else if ( Q_stricmp( name, "FoundPlayerServerStatus" ) == 0 )
		{
			Q_strncpyz( uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[ uiInfo.currentFoundPlayerServer ],
			            sizeof( uiInfo.serverStatusAddress ) );
			UI_BuildServerStatus( qtrue );
			Menu_SetFeederSelection( NULL, FEEDER_FINDPLAYER, 0, NULL );
		}
		else if ( Q_stricmp( name, "FindPlayer" ) == 0 )
		{
			UI_BuildFindPlayerList( qtrue );
			// clear the displayed server status info
			uiInfo.serverStatusInfo.numLines = 0;
			Menu_SetFeederSelection( NULL, FEEDER_FINDPLAYER, 0, NULL );
		}
		else if ( Q_stricmp( name, "JoinServer" ) == 0 )
		{
			if ( uiInfo.serverStatus.currentServer >= 0 &&
			     uiInfo.serverStatus.currentServer < uiInfo.serverStatus.numDisplayServers )
			{
				Menus_CloseAll();
				trap_Cvar_Set( "ui_connecting", "1" );
				trap_Cvar_Set( "cg_thirdPerson", "0 " );
				trap_Cvar_Set( "cg_cameraOrbit", "0" );
				trap_Cvar_Set( "ui_singlePlayerActive", "0" );
				trap_LAN_GetServerAddressString( ui_netSource.integer,
				                                 uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ], buff,
				                                 1024 );
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", buff ) );
			}
		}
		else if ( Q_stricmp( name, "JoinDirectServer" ) == 0 )
		{
			Menus_CloseAll();
			trap_Cvar_Set( "ui_connecting", "1" );
			trap_Cvar_Set( "cg_thirdPerson", "0" );
			trap_Cvar_Set( "cg_cameraOrbit", "0" );
			trap_Cvar_Set( "ui_singlePlayerActive", "0" );
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", UI_Cvar_VariableString( "ui_connectToIPAddress" ) ) );
		}
		else if ( Q_stricmp( name, "FoundPlayerJoinServer" ) == 0 )
		{
			trap_Cvar_Set( "ui_singlePlayerActive", "0" );

			if ( uiInfo.currentFoundPlayerServer >= 0 && uiInfo.currentFoundPlayerServer < uiInfo.numFoundPlayerServers )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND,
				                      va( "connect %s\n", uiInfo.foundPlayerServerAddresses[ uiInfo.currentFoundPlayerServer ] ) );
			}
		}
		else if ( Q_stricmp( name, "Quit" ) == 0 )
		{
			trap_Cvar_Set( "ui_singlePlayerActive", "0" );
			trap_Cmd_ExecuteText( EXEC_NOW, "quit" );
		}
		else if ( Q_stricmp( name, "Controls" ) == 0 )
		{
			trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "setup_menu2", qtrue );
		}
		else if ( Q_stricmp( name, "Leave" ) == 0 )
		{
			// ATVI Wolfenstein Misc #460
			// if we are running a local server, make sure we kill it cleanly for other clients
			if ( trap_Cvar_VariableValue( "sv_running" ) )
			{
				trap_Cvar_Set( "sv_killserver", "1" );
			}
			else
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_CloseAll();
//          Menus_ActivateByName( "background_1", qtrue );
				Menus_ActivateByName( "backgroundmusic", qtrue );
				Menus_ActivateByName( "main_opener", qtrue );
			}
		}
		else if ( Q_stricmp( name, "ServerSort" ) == 0 )
		{
			int sortColumn;

			if ( Int_Parse( args, &sortColumn ) )
			{
				// if same column we're already sorting on then flip the direction
				if ( sortColumn == uiInfo.serverStatus.sortKey )
				{
					uiInfo.serverStatus.sortDir = !uiInfo.serverStatus.sortDir;
				}

				// make sure we sort again
				UI_ServersSort( sortColumn, qtrue );
			}
		}
		else if ( Q_stricmp( name, "ServerSortDown" ) == 0 )
		{
			int sortColumn;

			if ( Int_Parse( args, &sortColumn ) )
			{
				uiInfo.serverStatus.sortDir = 0;

				// make sure we sort again
				UI_ServersSort( sortColumn, qtrue );
			}
		}
		else if ( Q_stricmp( name, "nextSkirmish" ) == 0 )
		{
			UI_StartSkirmish( qtrue );
		}
		else if ( Q_stricmp( name, "SkirmishStart" ) == 0 )
		{
			UI_StartSkirmish( qfalse );
		}
		else if ( Q_stricmp( name, "closeingame" ) == 0 )
		{
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			trap_Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();
		}
		else if ( Q_stricmp( name, "voteMap" ) == 0 )
		{
			if ( ui_netGameType.integer == GT_WOLF_CAMPAIGN )
			{
				if ( ui_currentNetMap.integer >= 0 && ui_currentNetMap.integer < uiInfo.campaignCount )
				{
					trap_Cmd_ExecuteText( EXEC_APPEND,
					                      va( "callvote campaign %s\n",
					                          uiInfo.campaignList[ ui_currentNetMap.integer ].campaignShortName ) );
				}
			}
			else
			{
				if ( ui_currentNetMap.integer >= 0 && ui_currentNetMap.integer < uiInfo.mapCount )
				{
					trap_Cmd_ExecuteText( EXEC_APPEND,
					                      va( "callvote map %s\n", uiInfo.mapList[ ui_currentNetMap.integer ].mapLoadName ) );
				}
			}
		}
		else if ( Q_stricmp( name, "voteKick" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote kick \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "voteMute" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote mute \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "voteUnMute" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote unmute \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "voteReferee" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote referee \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "voteUnReferee" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote unreferee \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "voteGame" ) == 0 )
		{
			int ui_voteGameType = trap_Cvar_VariableValue( "ui_voteGameType" );

			if ( ui_voteGameType >= 0 && ui_voteGameType < uiInfo.numGameTypes )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote gametype %i\n", ui_voteGameType ) );
			}
		}
		else if ( Q_stricmp( name, "refGame" ) == 0 )
		{
			int ui_voteGameType = trap_Cvar_VariableValue( "ui_voteGameType" );

			if ( ui_voteGameType >= 0 && ui_voteGameType < uiInfo.numGameTypes )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref gametype %i\n", ui_voteGameType ) );
			}
		}
		else if ( Q_stricmp( name, "voteTimelimit" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote timelimit %f\n", trap_Cvar_VariableValue( "ui_voteTimelimit" ) ) );
		}
		else if ( Q_stricmp( name, "voteWarmupDamage" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND,
			                      va( "callvote warmupdamage %d\n", ( int ) trap_Cvar_VariableValue( "ui_voteWarmupDamage" ) ) );
		}
		else if ( Q_stricmp( name, "refTimelimit" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref timelimit %f\n", trap_Cvar_VariableValue( "ui_voteTimelimit" ) ) );
		}
		else if ( Q_stricmp( name, "refWarmupDamage" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref warmupdamage %d\n", ( int ) trap_Cvar_VariableValue( "ui_voteWarmupDamage" ) ) );
		}
		else if ( Q_stricmp( name, "voteInitToggles" ) == 0 )
		{
			char info[ MAX_INFO_STRING ];

			trap_GetConfigString( CS_SERVERTOGGLES, info, sizeof( info ) );
			trap_Cvar_Set( "ui_voteWarmupDamage", va( "%d", ( ( atoi( info ) & CV_SVS_WARMUPDMG ) >> 2 ) ) );

			trap_GetConfigString( CS_SERVERINFO, info, sizeof( info ) );
			trap_Cvar_Set( "ui_voteTimelimit", va( "%i", atoi( Info_ValueForKey( info, "timelimit" ) ) ) );
		}
		else if ( Q_stricmp( name, "voteLeader" ) == 0 )
		{
			if ( uiInfo.teamIndex >= 0 && uiInfo.teamIndex < uiInfo.myTeamCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "callteamvote leader %s\n", uiInfo.teamNames[ uiInfo.teamIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "addBot" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND,
			                      va( "addbot %s %i %s\n", uiInfo.characterList[ uiInfo.botIndex ].name, uiInfo.skillIndex + 1,
			                          ( uiInfo.redBlue == 0 ) ? "Red" : "Blue" ) );
		}
		else if ( Q_stricmp( name, "addFavorite" ) == 0 )
		{
			if ( ui_netSource.integer != AS_FAVORITES )
			{
				char name[ MAX_NAME_LENGTH ];
				char addr[ MAX_NAME_LENGTH ];
				int  res;

				trap_LAN_GetServerInfo( ui_netSource.integer,
				                        uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ], buff,
				                        MAX_STRING_CHARS );
				name[ 0 ] = addr[ 0 ] = '\0';
				Q_strncpyz( name, Info_ValueForKey( buff, "hostname" ), MAX_NAME_LENGTH );
				Q_strncpyz( addr, Info_ValueForKey( buff, "addr" ), MAX_NAME_LENGTH );

				if ( strlen( name ) > 0 && strlen( addr ) > 0 )
				{
					res = trap_LAN_AddServer( AS_FAVORITES, name, addr );

					if ( res == 0 )
					{
						// server already in the list
						Com_Printf( "%s", trap_TranslateString( "Favorite already in list\n" ) );
					}
					else if ( res == -1 )
					{
						// list full
						Com_Printf( "%s", trap_TranslateString( "Favorite list full\n" ) );
					}
					else
					{
						// successfully added
						Com_Printf( trap_TranslateString( "Added favorite server %s\n" ), addr );
					}
				}
			}
		}
		else if ( Q_stricmp( name, "deleteFavorite" ) == 0 )
		{
			if ( ui_netSource.integer == AS_FAVORITES )
			{
				char addr[ MAX_NAME_LENGTH ];

				trap_LAN_GetServerInfo( ui_netSource.integer,
				                        uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ], buff,
				                        MAX_STRING_CHARS );
				addr[ 0 ] = '\0';
				Q_strncpyz( addr, Info_ValueForKey( buff, "addr" ), MAX_NAME_LENGTH );

				if ( strlen( addr ) > 0 )
				{
					trap_LAN_RemoveServer( AS_FAVORITES, addr );
				}
			}
		}
		else if ( Q_stricmp( name, "createFavorite" ) == 0 )
		{
			if ( ui_netSource.integer == AS_FAVORITES )
			{
				char name[ MAX_NAME_LENGTH ];
				char addr[ MAX_NAME_LENGTH ];
				int  res;

				name[ 0 ] = addr[ 0 ] = '\0';
				Q_strncpyz( name, UI_Cvar_VariableString( "ui_favoriteName" ), MAX_NAME_LENGTH );
				Q_strncpyz( addr, UI_Cvar_VariableString( "ui_favoriteAddress" ), MAX_NAME_LENGTH );

				if ( strlen( name ) > 0 && strlen( addr ) > 0 )
				{
					res = trap_LAN_AddServer( AS_FAVORITES, name, addr );

					if ( res == 0 )
					{
						// server already in the list
						Com_Printf( "%s", trap_TranslateString( "Favorite already in list\n" ) );
					}
					else if ( res == -1 )
					{
						// list full
						Com_Printf( "%s", trap_TranslateString( "Favorite list full\n" ) );
					}
					else
					{
						// successfully added
						Com_Printf( trap_TranslateString( "Added favorite server %s\n" ), addr );
					}
				}
			}
		}
		else if ( Q_stricmp( name, "createFavoriteIngame" ) == 0 )
		{
			uiClientState_t cstate;
			char            name[ MAX_NAME_LENGTH ];
			char            addr[ MAX_NAME_LENGTH ];
			int             res;

			trap_GetClientState( &cstate );

			addr[ 0 ] = '\0';
			name[ 0 ] = '\0';
			Q_strncpyz( addr, cstate.servername, MAX_NAME_LENGTH );
			Q_strncpyz( name, cstate.servername, MAX_NAME_LENGTH );

			if ( *name && *addr && Q_stricmp( addr, "localhost" ) )
			{
				res = trap_LAN_AddServer( AS_FAVORITES, name, addr );

				if ( res == 0 )
				{
					// server already in the list
					Com_Printf( "%s", trap_TranslateString( "Favorite already in list\n" ) );
				}
				else if ( res == -1 )
				{
					// list full
					Com_Printf( "%s", trap_TranslateString( "Favorite list full\n" ) );
				}
				else
				{
					// successfully added
					Com_Printf( trap_TranslateString( "Added favorite server %s\n" ), addr );
				}
			}
		}
		else if ( Q_stricmp( name, "orders" ) == 0 )
		{
			const char *orders;

			if ( String_Parse( args, &orders ) )
			{
				int selectedPlayer = trap_Cvar_VariableValue( "cg_selectedPlayer" );

				if ( selectedPlayer < uiInfo.myTeamCount )
				{
					strcpy( buff, orders );
					trap_Cmd_ExecuteText( EXEC_APPEND, va( buff, uiInfo.teamClientNums[ selectedPlayer ] ) );
					trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
				}
				else
				{
					int i;

					for ( i = 0; i < uiInfo.myTeamCount; i++ )
					{
						if ( Q_stricmp( UI_Cvar_VariableString( "name" ), uiInfo.teamNames[ i ] ) == 0 )
						{
							continue;
						}

						strcpy( buff, orders );
						trap_Cmd_ExecuteText( EXEC_APPEND, va( buff, uiInfo.teamNames[ i ] ) );
						trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
					}
				}

				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		}
		else if ( Q_stricmp( name, "voiceOrdersTeam" ) == 0 )
		{
			const char *orders;

			if ( String_Parse( args, &orders ) )
			{
				int selectedPlayer = trap_Cvar_VariableValue( "cg_selectedPlayer" );

				if ( selectedPlayer == uiInfo.myTeamCount )
				{
					trap_Cmd_ExecuteText( EXEC_APPEND, orders );
					trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
				}

				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		}
		else if ( Q_stricmp( name, "voiceOrders" ) == 0 )
		{
			const char *orders;

			if ( String_Parse( args, &orders ) )
			{
				int selectedPlayer = trap_Cvar_VariableValue( "cg_selectedPlayer" );

				if ( selectedPlayer < uiInfo.myTeamCount )
				{
					strcpy( buff, orders );
					trap_Cmd_ExecuteText( EXEC_APPEND, va( buff, uiInfo.teamClientNums[ selectedPlayer ] ) );
					trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
				}

				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		}
		else if ( Q_stricmp( name, "glCustom" ) == 0 )
		{
			trap_Cvar_Set( "ui_glCustom", "4" );
		}
		else if ( Q_stricmp( name, "update" ) == 0 )
		{
			if ( String_Parse( args, &name2 ) )
			{
				UI_Update( name2 );
			}

			// NERVE - SMF
		}
		else if ( Q_stricmp( name, "startSingleplayer" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, "startSingleplayer\n" );
		}
		else if ( Q_stricmp( name, "showSpecScores" ) == 0 )
		{
			if ( atoi( UI_Cvar_VariableString( "ui_isSpectator" ) ) )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, "+scores\n" );
			}
		}
		else if ( Q_stricmp( name, "rconGame" ) == 0 )
		{
			if ( ui_netGameType.integer >= 0 && ui_netGameType.integer < uiInfo.numGameTypes )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "rcon g_gametype %i\n", ui_netGameType.integer ) );
			}
		}
		else if ( Q_stricmp( name, "rconMap" ) == 0 )
		{
			if ( ui_currentNetMap.integer >= 0 && ui_currentNetMap.integer < uiInfo.mapCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "rcon map %s\n", uiInfo.mapList[ ui_currentNetMap.integer ].mapLoadName ) );
			}
		}
		else if ( Q_stricmp( name, "refMap" ) == 0 )
		{
			if ( ui_netGameType.integer == GT_WOLF_CAMPAIGN )
			{
				if ( ui_currentNetMap.integer >= 0 && ui_currentNetMap.integer < uiInfo.campaignCount )
				{
					trap_Cmd_ExecuteText( EXEC_APPEND,
					                      va( "ref campaign %s\n",
					                          uiInfo.campaignList[ ui_currentNetMap.integer ].campaignShortName ) );
				}
			}
			else
			{
				if ( ui_currentNetMap.integer >= 0 && ui_currentNetMap.integer < uiInfo.mapCount )
				{
					trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref map %s\n", uiInfo.mapList[ ui_currentNetMap.integer ].mapLoadName ) );
				}
			}
		}
		else if ( Q_stricmp( name, "rconKick" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "rcon kick \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "refKick" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref kick \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "rconBan" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "rcon ban \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "refMute" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref mute \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "refUnMute" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref unmute \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "refMakeAxis" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref putaxis \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "refMakeAllied" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref putallies \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "refMakeSpec" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref remove \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "refUnReferee" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref unreferee \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "refMakeReferee" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref referee \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "rconMakeReferee" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "rcon makeReferee \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "rconRemoveReferee" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "rcon removeReferee \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "rconMute" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "rcon mute \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "rconUnMute" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "rcon unmute \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "refWarning" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				char buffer[ 128 ];

				trap_Cvar_VariableStringBuffer( "ui_warnreason", buffer, 128 );

				trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref warn \"%s\" \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ], buffer ) );
			}
		}
		else if ( Q_stricmp( name, "refWarmup" ) == 0 )
		{
			char buffer[ 128 ];

			trap_Cvar_VariableStringBuffer( "ui_warmup", buffer, 128 );

			trap_Cmd_ExecuteText( EXEC_APPEND, va( "ref warmup \"%s\"\n", buffer ) );
		}
		else if ( Q_stricmp( name, "ignorePlayer" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "ignore \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "unIgnorePlayer" ) == 0 )
		{
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "unignore \"%s\"\n", uiInfo.playerNames[ uiInfo.playerIndex ] ) );
			}
		}
		else if ( Q_stricmp( name, "loadCachedServers" ) == 0 )
		{
			trap_LAN_LoadCachedServers(); // load servercache.dat
		}
		else if ( Q_stricmp( name, "setupCampaign" ) == 0 )
		{
			trap_Cvar_Set( "ui_campaignmap", va( "%i", uiInfo.campaignList[ ui_currentCampaign.integer ].progress ) );
		}
		else if ( Q_stricmp( name, "playCampaign" ) == 0 )
		{
			int map = trap_Cvar_VariableValue( "ui_campaignmap" );

			if ( map <= uiInfo.campaignList[ ui_currentCampaign.integer ].progress )
			{
				//trap_Cmd_ExecuteText( EXEC_APPEND, va("spmap \"%s\"\n", uiInfo.campaignList[ui_currentCampaign.integer].mapInfos[uiInfo.campaignList[ui_currentCampaign.integer].progress]->mapLoadName));
				trap_Cmd_ExecuteText( EXEC_APPEND,
				                      va( "spmap \"%s\"\n",
				                          uiInfo.campaignList[ ui_currentCampaign.integer ].mapInfos[ map ]->mapLoadName ) );
			}
		}
		else if ( Q_stricmp( name, "loadProfiles" ) == 0 )
		{
			UI_LoadProfiles();
		}
		else if ( Q_stricmp( name, "createProfile" ) == 0 )
		{
			fileHandle_t f;
			char         buff[ MAX_CVAR_VALUE_STRING ];

			/*Q_strncpyz( cl_profile.string, ui_profile.string, sizeof(cl_profile.string) );
			   Q_CleanStr( cl_profile.string );
			   Q_CleanDirName( cl_profile.string );

			   trap_Cvar_Set( "cl_profile", cl_profile.string );
			   if( trap_FS_FOpenFile( va( "profiles/%s/profile.dat", cl_profile.string ), &f, FS_WRITE ) >= 0 ) {
			   trap_FS_Write( va( "\"%s\"", ui_profile.string ), strlen(ui_profile.string) + 2, f );
			   trap_FS_FCloseFile( f );
			   } */
			Q_strncpyz( buff, ui_profile.string, sizeof( buff ) );
			Q_CleanStr( buff );
			Q_CleanDirName( buff );

			if ( trap_FS_FOpenFile( va( "profiles/%s/profile.dat", buff ), &f, FS_WRITE ) >= 0 )
			{
				trap_FS_Write( va( "\"%s\"", ui_profile.string ), strlen( ui_profile.string ) + 2, f );
				trap_FS_FCloseFile( f );
			}

			trap_Cvar_Set( "name", ui_profile.string );
		}
		else if ( Q_stricmp( name, "clearPID" ) == 0 )
		{
			fileHandle_t f;

			// delete profile.pid from current profile
			if ( trap_FS_FOpenFile( va( "profiles/%s/profile.pid", cl_profile.string ), &f, FS_READ ) >= 0 )
			{
				trap_FS_FCloseFile( f );
				trap_FS_Delete( va( "profiles/%s/profile.pid", cl_profile.string ) );
			}
		}
		else if ( Q_stricmp( name, "applyProfile" ) == 0 )
		{
			Q_strncpyz( cl_profile.string, ui_profile.string, sizeof( cl_profile.string ) );
			Q_CleanStr( cl_profile.string );
			Q_CleanDirName( cl_profile.string );
			trap_Cvar_Set( "cl_profile", cl_profile.string );
		}
		else if ( Q_stricmp( name, "setDefaultProfile" ) == 0 )
		{
			fileHandle_t f;

			Q_strncpyz( cl_defaultProfile.string, ui_profile.string, sizeof( cl_profile.string ) );
			Q_CleanStr( cl_defaultProfile.string );
			Q_CleanDirName( cl_defaultProfile.string );
			trap_Cvar_Set( "cl_defaultProfile", cl_defaultProfile.string );

			if ( trap_FS_FOpenFile( "profiles/defaultprofile.dat", &f, FS_WRITE ) >= 0 )
			{
				trap_FS_Write( va( "\"%s\"", cl_defaultProfile.string ), strlen( cl_defaultProfile.string ) + 2, f );
				trap_FS_FCloseFile( f );
			}
		}
		else if ( Q_stricmp( name, "deleteProfile" ) == 0 )
		{
			char buff[ MAX_CVAR_VALUE_STRING ];

			Q_strncpyz( buff, ui_profile.string, sizeof( buff ) );
			Q_CleanStr( buff );
			Q_CleanDirName( buff );

			// can't delete active profile
			if ( Q_stricmp( buff, cl_profile.string ) )
			{
				if ( !Q_stricmp( buff, cl_defaultProfile.string ) )
				{
					// if deleting the default profile, set the default to the current active profile
					fileHandle_t f;

					trap_Cvar_Set( "cl_defaultProfile", cl_profile.string );

					if ( trap_FS_FOpenFile( "profiles/defaultprofile.dat", &f, FS_WRITE ) >= 0 )
					{
						trap_FS_Write( va( "\"%s\"", cl_profile.string ), strlen( cl_profile.string ) + 2, f );
						trap_FS_FCloseFile( f );
					}
				}

				trap_FS_Delete( va( "profiles/%s", buff ) );
			}
		}
		else if ( Q_stricmp( name, "renameProfileInit" ) == 0 )
		{
			trap_Cvar_Set( "ui_profile_renameto", ui_profile.string );
		}
		else if ( Q_stricmp( name, "renameProfile" ) == 0 )
		{
			fileHandle_t f, f2;
			int          len;
			char         buff[ MAX_CVAR_VALUE_STRING ];
			char         ui_renameprofileto[ MAX_CVAR_VALUE_STRING ];
			char         uiprofile[ MAX_CVAR_VALUE_STRING ];

			trap_Cvar_VariableStringBuffer( "ui_profile_renameto", ui_renameprofileto, sizeof( ui_renameprofileto ) );
			Q_strncpyz( buff, ui_renameprofileto, sizeof( buff ) );
			Q_CleanStr( buff );
			Q_CleanDirName( buff );

			Q_strncpyz( uiprofile, ui_profile.string, sizeof( uiprofile ) );
			Q_CleanStr( uiprofile );
			Q_CleanDirName( uiprofile );

			if ( trap_FS_FOpenFile( va( "profiles/%s/profile.dat", buff ), &f, FS_WRITE ) >= 0 )
			{
				trap_FS_Write( va( "\"%s\"", ui_renameprofileto ), strlen( ui_renameprofileto ) + 2, f );
				trap_FS_FCloseFile( f );
			}

			// FIXME: make this copying handle all files in the profiles directory
			if ( Q_stricmp( uiprofile, buff ) )
			{
				if ( trap_FS_FOpenFile( va( "profiles/%s/%s", buff, CONFIG_NAME ), &f, FS_WRITE ) >= 0 )
				{
					if ( ( len = trap_FS_FOpenFile( va( "profiles/%s/%s", uiprofile, CONFIG_NAME ), &f2, FS_READ ) ) >= 0 )
					{
						int i;

						for ( i = 0; i < len; i++ )
						{
							byte b;

							trap_FS_Read( &b, 1, f2 );
							trap_FS_Write( &b, 1, f );
						}

						trap_FS_FCloseFile( f2 );
					}

					trap_FS_FCloseFile( f );
				}

				if ( trap_FS_FOpenFile( va( "profiles/%s/servercache.dat", buff ), &f, FS_WRITE ) >= 0 )
				{
					if ( ( len = trap_FS_FOpenFile( va( "profiles/%s/servercache.dat", cl_profile.string ), &f2, FS_READ ) ) >= 0 )
					{
						int i;

						for ( i = 0; i < len; i++ )
						{
							byte b;

							trap_FS_Read( &b, 1, f2 );
							trap_FS_Write( &b, 1, f );
						}

						trap_FS_FCloseFile( f2 );
					}

					trap_FS_FCloseFile( f );
				}

				if ( !Q_stricmp( uiprofile, cl_defaultProfile.string ) )
				{
					// if renaming the default profile, set the default to the new profile
					trap_Cvar_Set( "cl_defaultProfile", buff );

					if ( trap_FS_FOpenFile( "profiles/defaultprofile.dat", &f, FS_WRITE ) >= 0 )
					{
						trap_FS_Write( va( "\"%s\"", buff ), strlen( buff ) + 2, f );
						trap_FS_FCloseFile( f );
					}
				}

				// if renaming the active profile, set active to new name
				if ( !Q_stricmp( uiprofile, cl_profile.string ) )
				{
					trap_Cvar_Set( "cl_profile", buff );
				}

				// delete the old profile
				trap_FS_Delete( va( "profiles/%s", uiprofile ) );
			}

			trap_Cvar_Set( "ui_profile", ui_renameprofileto );
			trap_Cvar_Set( "ui_profile_renameto", "" );
		}
		else if ( Q_stricmp( name, "initHostGameFeatures" ) == 0 )
		{
			int cvar = trap_Cvar_VariableValue( "g_maxlives" );

			if ( cvar )
			{
				trap_Cvar_Set( "ui_maxlives", "1" );
			}
			else
			{
				trap_Cvar_Set( "ui_maxlives", "0" );
			}

			cvar = trap_Cvar_VariableValue( "g_heavyWeaponRestriction" );

			if ( cvar == 100 )
			{
				trap_Cvar_Set( "ui_heavyWeaponRestriction", "0" );
			}
			else
			{
				trap_Cvar_Set( "ui_heavyWeaponRestriction", "1" );
			}
		}
		else if ( Q_stricmp( name, "toggleMaxLives" ) == 0 )
		{
			int ui_ml = trap_Cvar_VariableValue( "ui_maxlives" );

			if ( ui_ml )
			{
				trap_Cvar_Set( "g_maxlives", "5" );
			}
			else
			{
				trap_Cvar_Set( "g_maxlives", "0" );
			}
		}
		else if ( Q_stricmp( name, "toggleWeaponRestriction" ) == 0 )
		{
			int ui_hwr = trap_Cvar_VariableValue( "ui_heavyWeaponRestriction" );

			if ( ui_hwr )
			{
				trap_Cvar_Set( "g_heavyWeaponRestriction", "10" );
			}
			else
			{
				trap_Cvar_Set( "g_heavyWeaponRestriction", "100" );
			}
		}
		else if ( Q_stricmp( name, "openModURL" ) == 0 )
		{
			trap_Cvar_Set( "ui_finalURL", UI_Cvar_VariableString( "ui_modURL" ) );
		}
		else if ( Q_stricmp( name, "openServerURL" ) == 0 )
		{
			trap_Cvar_Set( "ui_finalURL", UI_Cvar_VariableString( "ui_URL" ) );
		}
		else if ( Q_stricmp( name, "validate_openURL" ) == 0 )
		{
			// this is the only one that effectively triggers the URL, after the disclaimers are done with
			// we use ui_finalURL as an auxiliary variable to gather URLs from various sources
			trap_openURL( UI_Cvar_VariableString( "ui_finalURL" ) );
		}
		else if ( Q_stricmp( name, "clientCheckVote" ) == 0 )
		{
			int flags = trap_Cvar_VariableValue( "cg_ui_voteFlags" );

			if ( flags == VOTING_DISABLED )
			{
				trap_Cvar_SetValue( "cg_ui_novote", 1 );
			}
			else
			{
				trap_Cvar_SetValue( "cg_ui_novote", 0 );
			}
		}
		else if ( Q_stricmp( name, "reconnect" ) == 0 )
		{
			// TODO: if dumped because of cl_allowdownload problem, toggle on first (we don't have appropriate support for this yet)
			trap_Cmd_ExecuteText( EXEC_APPEND, "reconnect\n" );
		}
		else if ( Q_stricmp( name, "redirect" ) == 0 )
		{
			// fretn
			char buf[ MAX_STRING_CHARS ];

			trap_Cvar_VariableStringBuffer( "com_errorMessage", buf, sizeof( buf ) );
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", buf ) );
			trap_Cvar_Set( "com_errorMessage", "" );
		}
		else if ( Q_stricmp( name, "updateGameType" ) == 0 )
		{
			trap_Cvar_Update( &ui_netGameType );

			/*if( ui_netGameType.integer == GT_WOLF_CAMPAIGN ) {
			   if( ui_mapIndex.integer >= UI_MapCountByGameType( qfalse ) )
			   } else {

			   } */
			if ( ui_mapIndex.integer >= UI_MapCountByGameType( qfalse ) )
			{
				Menu_SetFeederSelection( NULL, FEEDER_MAPS, 0, NULL );
				Menu_SetFeederSelection( NULL, FEEDER_ALLMAPS, 0, NULL );
			}
		}
		// ydnar
		else if ( Q_stricmp( name, "vidSave" ) == 0 )
		{
			int mode;

			// get mode
			mode = trap_Cvar_VariableValue( "r_mode" );

			// save mode to old mode
			trap_Cvar_SetValue( "r_oldMode", mode );
		}
		else if ( Q_stricmp( name, "vidReset" ) == 0 )
		{
			int oldMode;

			// get old mode
			oldMode = trap_Cvar_VariableValue( "r_oldMode" );

			if ( oldMode == 0 )
			{
				oldMode = 3;
			}

			// reset mode to old mode
			trap_Cvar_SetValue( "r_mode", oldMode );
			trap_Cvar_Set( "r_oldMode", "" );
		}
		else if ( Q_stricmp( name, "vidConfirm" ) == 0 )
		{
			trap_Cvar_Set( "r_oldMode", "" );
		}
		else if ( Q_stricmp( name, "graphicsCvarsGet" ) == 0 )
		{
			char ui_r_texturemode[ MAX_CVAR_VALUE_STRING ];
			trap_Cvar_CopyValue_i( "r_mode", "ui_r_mode" );
			trap_Cvar_CopyValue_i( "r_gamma", "ui_r_gamma" );
			trap_Cvar_CopyValue_i( "r_colorbits", "ui_r_colorbits" );
			trap_Cvar_CopyValue_i( "r_fullscreen", "ui_r_fullscreen" );
			trap_Cvar_CopyValue_i( "r_lodbias", "ui_r_lodbias" );
			trap_Cvar_CopyValue_i( "r_subdivisions", "ui_r_subdivisions" );
			trap_Cvar_CopyValue_i( "r_picmip", "ui_r_picmip" );
			trap_Cvar_CopyValue_i( "r_texturebits", "ui_r_texturebits" );
			trap_Cvar_CopyValue_i( "r_depthbits", "ui_r_depthbits" );
			trap_Cvar_CopyValue_i( "r_ext_compressed_textures", "ui_r_ext_compressed_textures" );
			trap_Cvar_CopyValue_i( "r_finish", "ui_r_finish" );
			trap_Cvar_CopyValue_i( "r_dynamiclight", "ui_r_dynamiclight" );
			trap_Cvar_CopyValue_i( "r_allowextensions", "ui_r_allowextensions" );
			trap_Cvar_CopyValue_i( "r_detailtextures", "ui_r_detailtextures" );
			trap_Cvar_CopyValue_i( "cg_shadows", "ui_cg_shadows" );
			trap_Cvar_CopyValue_i( "r_hdrrendering", "ui_r_hdrrendering" );
			trap_Cvar_CopyValue_i( "r_bloom", "r_bloom" );
			trap_Cvar_CopyValue_i( "r_normalmapping", "ui_r_normalmapping" );
			trap_Cvar_CopyValue_i( "r_parallaxmapping", "ui_r_parallaxmapping" );

			trap_Cvar_VariableStringBuffer( "r_texturemode", ui_r_texturemode, sizeof( ui_r_texturemode ) );
			trap_Cvar_Set( "ui_r_texturemode", ui_r_texturemode );
		}
		else if ( Q_stricmp( name, "graphicsCvarsReset" ) == 0 )
		{
			UI_ResetUICvars();
		}
		else if ( Q_stricmp( name, "graphicsCvarsApply" ) == 0 )
		{
			char ui_r_texturemode[ MAX_CVAR_VALUE_STRING ];
			trap_Cvar_CopyValue_i( "ui_r_mode", "r_mode" );
			trap_Cvar_CopyValue_i( "ui_r_gamma", "r_gamma" );
			trap_Cvar_CopyValue_i( "ui_r_colorbits", "r_colorbits" );
			trap_Cvar_CopyValue_i( "ui_r_fullscreen", "r_fullscreen" );
			trap_Cvar_CopyValue_i( "ui_r_lodbias", "r_lodbias" );
			trap_Cvar_CopyValue_i( "ui_r_subdivisions", "r_subdivisions" );
			trap_Cvar_CopyValue_i( "ui_r_picmip", "r_picmip" );
			trap_Cvar_CopyValue_i( "ui_r_texturebits", "r_texturebits" );
			trap_Cvar_CopyValue_i( "ui_r_depthbits", "r_depthbits" );
			trap_Cvar_CopyValue_i( "ui_r_ext_compressed_textures", "r_ext_compressed_textures" );
			trap_Cvar_CopyValue_i( "ui_r_finish", "r_finish" );
			trap_Cvar_CopyValue_i( "ui_r_dynamiclight", "r_dynamiclight" );
			trap_Cvar_CopyValue_i( "ui_r_allowextensions", "r_allowextensions" );
			trap_Cvar_CopyValue_i( "ui_r_detailtextures", "r_detailtextures" );
			trap_Cvar_CopyValue_i( "ui_cg_shadows", "cg_shadows" );
			trap_Cvar_CopyValue_i( "ui_r_hdrrendering", "r_hdrrendering" );
			trap_Cvar_CopyValue_i( "ui_r_bloom", "r_bloom" );
			trap_Cvar_CopyValue_i( "ui_r_normalmapping", "r_normalmapping" );
			trap_Cvar_CopyValue_i( "ui_r_parallaxmapping", "r_parallaxmapping" );

			trap_Cvar_VariableStringBuffer( "ui_r_texturemode", ui_r_texturemode, sizeof( ui_r_texturemode ) );
			trap_Cvar_Set( "r_texturemode", ui_r_texturemode );

			UI_ResetUICvars();
		}
		else if ( Q_stricmp( name, "profileCvarsGet" ) == 0 )
		{
			int ui_profile_mousePitch = trap_Cvar_VariableValue( "ui_mousePitch" );

			trap_Cvar_Set( "ui_profile_mousePitch", va( "%i", ui_profile_mousePitch ) );
		}
		else if ( Q_stricmp( name, "profileCvarsApply" ) == 0 )
		{
			int ui_handedness = trap_Cvar_VariableValue( "ui_handedness" );
			int ui_profile_mousePitch = trap_Cvar_VariableValue( "ui_profile_mousePitch" );

			trap_Cvar_Set( "ui_mousePitch", va( "%i", ui_profile_mousePitch ) );

			if ( ui_profile_mousePitch == 0 )
			{
				trap_Cvar_SetValue( "m_pitch", 0.022f );
			}
			else
			{
				trap_Cvar_SetValue( "m_pitch", -0.022f );
			}

			if ( ui_handedness == 0 )
			{
				// exec default.cfg
				trap_Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n" );
				Controls_SetDefaults( qfalse );
			}
			else
			{
				// exec default_left.cfg
				trap_Cmd_ExecuteText( EXEC_APPEND, "exec default_left.cfg\n" );
				Controls_SetDefaults( qtrue );
			}

			trap_Cvar_Set( "ui_handedness", "" );
			trap_Cvar_Set( "ui_profile_mousePitch", "" );
		}
		else if ( Q_stricmp( name, "profileCvarsReset" ) == 0 )
		{
			trap_Cvar_Set( "ui_handedness", "" );
			trap_Cvar_Set( "ui_profile_mousePitch", "" );
		}
		else if ( Q_stricmp( name, "defaultControls" ) == 0 )
		{
			int ui_handedness = trap_Cvar_VariableValue( "ui_handedness" );

			if ( ui_handedness == 0 )
			{
				// exec default.cfg
				trap_Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n" );
				Controls_SetDefaults( qfalse );
			}
			else
			{
				// exec default_left.cfg
				trap_Cmd_ExecuteText( EXEC_APPEND, "exec default_left.cfg\n" );
				Controls_SetDefaults( qtrue );
			}
		}
		else if ( Q_stricmp( name, "systemCvarsGet" ) == 0 )
		{
			trap_Cvar_CopyValue_i( "com_hunkmegs", "ui_com_hunkmegs" );
			trap_Cvar_CopyValue_i( "com_soundmegs", "ui_com_soundmegs" );
			trap_Cvar_CopyValue_i( "com_zonemegs", "ui_com_zonemegs" );
			trap_Cvar_CopyValue_i( "cl_maxpackets", "ui_cl_maxpackets" );
			trap_Cvar_CopyValue_i( "cl_packetdup", "ui_cl_packetdup" );
			trap_Cvar_CopyValue_i( "s_khz", "ui_s_khz" );
			trap_Cvar_CopyValue_i( "rate", "ui_rate" );
		}
		else if ( Q_stricmp( name, "systemCvarsReset" ) == 0 )
		{
			UI_ResetUICvars();
		}
		else if ( Q_stricmp( name, "systemCvarsApply" ) == 0 )
		{
			int ui_cl_maxpackets = trap_Cvar_VariableValue( "ui_cl_maxpackets" );
			int ui_cl_packetdup = trap_Cvar_VariableValue( "ui_cl_packetdup" );
			int ui_rate = trap_Cvar_VariableValue( "ui_rate" );

			if ( ui_rate == 0 )
			{
				ui_rate = 5000;
				ui_cl_maxpackets = 30;
				ui_cl_packetdup = 1;
			}

			trap_Cvar_CopyValue_i( "ui_com_hunkmegs", "com_hunkmegs" );
			trap_Cvar_CopyValue_i( "ui_com_soundmegs", "com_soundmegs" );
			trap_Cvar_CopyValue_i( "ui_com_zonemegs", "com_zonemegs" );
			trap_Cvar_CopyValue_i( "ui_s_khz", "s_khz" );
			trap_Cvar_Set( "cl_maxpackets", va( "%i", ui_cl_maxpackets ) );
			trap_Cvar_Set( "cl_packetdup", va( "%i", ui_cl_packetdup ) );
			trap_Cvar_Set( "rate", va( "%i", ui_rate ) );

			UI_ResetUICvars();
		}
		else
		{
			Com_Printf( "^3WARNING: unknown UI script %s\n", name );
		}
	}
}

static void UI_GetTeamColor( vec4_t *color )
{
}

/*
==================
UI_MapCountByGameType
==================
*/
static int UI_MapCountByGameType( qboolean singlePlayer )
{
	int i, c, game;

	c = 0;
	game = singlePlayer ? uiInfo.gameTypes[ ui_gameType.integer ].gtEnum : ui_netGameType.integer;

	if ( game == GT_WOLF_CAMPAIGN )
	{
		for ( i = 0; i < uiInfo.campaignCount; i++ )
		{
			if ( uiInfo.campaignList[ i ].typeBits & ( 1 << GT_WOLF ) )
			{
				c++;
			}
		}
	}
	else
	{
		for ( i = 0; i < uiInfo.mapCount; i++ )
		{
			uiInfo.mapList[ i ].active = qfalse;

			if ( uiInfo.mapList[ i ].typeBits & ( 1 << game ) )
			{
				if ( singlePlayer )
				{
					continue;
				}

				c++;
				uiInfo.mapList[ i ].active = qtrue;
			}
		}
	}

	return c;
}

/*
==================
UI_MapCountByCampaign
==================
*/
#if 0 // rain - unused
static int UI_MapCountByCampaign( qboolean singlePlayer )
{
	int campaign;
	int i, count = 0;

	campaign = singlePlayer ? ui_currentCampaign.integer : ui_currentNetCampaign.integer;

	for ( i = 0; i < uiInfo.campaignList[ campaign ].mapCount; i++ )
	{
		if ( singlePlayer && ( uiInfo.campaignList[ i ].typeBits & ( 1 << GT_SINGLE_PLAYER ) ) )
		{
			count++;
		}
		else if ( !singlePlayer && !( uiInfo.campaignList[ i ].typeBits & ( 1 << GT_SINGLE_PLAYER ) ) )
		{
			count++;
		}
	}

	return ( count );
}

#endif

/*
==================
UI_CampaignCount
==================
*/
static int UI_CampaignCount( qboolean singlePlayer )
{
	int i, c /*, game */;

	c = 0;
	//game = singlePlayer ? uiInfo.gameTypes[ui_gameType.integer].gtEnum : uiInfo.gameTypes[ui_netGameType.integer].gtEnum;

	for ( i = 0; i < uiInfo.campaignCount; i++ )
	{
		if ( singlePlayer && !( uiInfo.campaignList[ i ].typeBits & ( 1 << GT_SINGLE_PLAYER ) ) )
		{
			continue;
		}

		if ( uiInfo.campaignList[ i ].unlocked )
		{
			c++;
		}
	}

	return c;

	//return uiInfo.campaignCount;
}

/*
==================
UI_InsertServerIntoDisplayList
==================
*/
static void UI_InsertServerIntoDisplayList( int num, int position )
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
static void UI_RemoveServerFromDisplayList( int num )
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
static void UI_BinaryServerInsertion( int num )
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
		res = trap_LAN_CompareServers( ui_netSource.integer, uiInfo.serverStatus.sortKey,
		                               uiInfo.serverStatus.sortDir, num, uiInfo.serverStatus.displayServers[ offset + mid ] );

		// if equal
		if ( res == 0 )
		{
			UI_InsertServerIntoDisplayList( num, offset + mid );
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

	UI_InsertServerIntoDisplayList( num, offset );
}

/*
==================
UI_BuildServerDisplayList
==================
*/
static void UI_BuildServerDisplayList( qboolean force )
{
	int        i, count, clients, maxClients, ping, game, len, visible, friendlyFire, maxlives, punkbuster, antilag,
	           password, weaponrestricted, balancedteams;
	char       info[ MAX_STRING_CHARS ];

	//qboolean startRefresh = qtrue; // TTimo: unused
	static int numinvisible;

	game = 0; // NERVE - SMF - shut up compiler warning

	if ( !( force || uiInfo.uiDC.realTime > uiInfo.serverStatus.nextDisplayRefresh ) )
	{
		return;
	}

	// if we shouldn't reset
	if ( force == 2 )
	{
		force = 0;
	}

	// do motd updates here too
	trap_Cvar_VariableStringBuffer( "cl_motdString", uiInfo.serverStatus.motd, sizeof( uiInfo.serverStatus.motd ) );
	len = strlen( uiInfo.serverStatus.motd );

	if ( len == 0 )
	{
		strcpy( uiInfo.serverStatus.motd, va( "Enemy Territory - Version: %s", Q3_VERSION ) );
		len = strlen( uiInfo.serverStatus.motd );
	}

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
		Menu_SetFeederSelection( NULL, FEEDER_SERVERS, 0, NULL );
		// mark all servers as visible so we store ping updates for them
		trap_LAN_MarkServerVisible( ui_netSource.integer, -1, qtrue );
	}

	// get the server count (comes from the master)
	count = trap_LAN_GetServerCount( ui_netSource.integer );

	if ( count == -1 || ( ui_netSource.integer == AS_LOCAL && count == 0 ) )
	{
		// still waiting on a response from the master
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 500;
		uiInfo.serverStatus.currentServerPreview = 0;
		return;
	}

	if ( !uiInfo.serverStatus.numDisplayServers )
	{
		uiInfo.serverStatus.currentServerPreview = 0;
	}

	visible = qfalse;

	for ( i = 0; i < count; i++ )
	{
		// if we already got info for this server
		if ( !trap_LAN_ServerIsVisible( ui_netSource.integer, i ) )
		{
			continue;
		}

		visible = qtrue;
		// get the ping for this server
		ping = trap_LAN_GetServerPing( ui_netSource.integer, i );

		if ( ping > /*=*/ 0 || ui_netSource.integer == AS_FAVORITES )
		{
			trap_LAN_GetServerInfo( ui_netSource.integer, i, info, MAX_STRING_CHARS );

			clients = atoi( Info_ValueForKey( info, "clients" ) );
			uiInfo.serverStatus.numPlayersOnServers += clients;

			trap_Cvar_Update( &ui_browserShowEmptyOrFull );

			if ( ui_browserShowEmptyOrFull.integer )
			{
				maxClients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );

				if ( clients != maxClients && ( ( !clients && ui_browserShowEmptyOrFull.integer == 2 ) ||
				                                ( clients && ui_browserShowEmptyOrFull.integer == 1 ) ) )
				{
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}

				if ( clients && ( ( clients == maxClients && ui_browserShowEmptyOrFull.integer == 2 ) ||
				                  ( clients != maxClients && ui_browserShowEmptyOrFull.integer == 1 ) ) )
				{
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			trap_Cvar_Update( &ui_browserShowPasswordProtected );

			if ( ui_browserShowPasswordProtected.integer )
			{
				password = atoi( Info_ValueForKey( info, "needpass" ) );

				if ( ( password && ui_browserShowPasswordProtected.integer == 2 ) ||
				     ( !password && ui_browserShowPasswordProtected.integer == 1 ) )
				{
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			trap_Cvar_Update( &ui_browserShowFriendlyFire );

			if ( ui_browserShowFriendlyFire.integer )
			{
				friendlyFire = atoi( Info_ValueForKey( info, "friendlyFire" ) );

				if ( ( friendlyFire && ui_browserShowFriendlyFire.integer == 2 ) ||
				     ( !friendlyFire && ui_browserShowFriendlyFire.integer == 1 ) )
				{
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			trap_Cvar_Update( &ui_browserShowMaxlives );

			if ( ui_browserShowMaxlives.integer )
			{
				maxlives = atoi( Info_ValueForKey( info, "maxlives" ) );

				if ( ( maxlives && ui_browserShowMaxlives.integer == 2 ) || ( !maxlives && ui_browserShowMaxlives.integer == 1 ) )
				{
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			trap_Cvar_Update( &ui_browserShowPunkBuster );

			if ( ui_browserShowPunkBuster.integer )
			{
				punkbuster = atoi( Info_ValueForKey( info, "punkbuster" ) );

				if ( ( punkbuster && ui_browserShowPunkBuster.integer == 2 ) ||
				     ( !punkbuster && ui_browserShowPunkBuster.integer == 1 ) )
				{
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			trap_Cvar_Update( &ui_browserShowAntilag );

			if ( ui_browserShowAntilag.integer )
			{
				antilag = atoi( Info_ValueForKey( info, "g_antilag" ) );

				if ( ( antilag && ui_browserShowAntilag.integer == 2 ) || ( !antilag && ui_browserShowAntilag.integer == 1 ) )
				{
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			trap_Cvar_Update( &ui_browserShowWeaponsRestricted );

			if ( ui_browserShowWeaponsRestricted.integer )
			{
				weaponrestricted = atoi( Info_ValueForKey( info, "weaprestrict" ) );

				if ( ( weaponrestricted != 100 && ui_browserShowWeaponsRestricted.integer == 2 ) ||
				     ( weaponrestricted == 100 && ui_browserShowWeaponsRestricted.integer == 1 ) )
				{
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			trap_Cvar_Update( &ui_browserShowTeamBalanced );

			if ( ui_browserShowTeamBalanced.integer )
			{
				balancedteams = atoi( Info_ValueForKey( info, "balancedteams" ) );

				if ( ( balancedteams && ui_browserShowTeamBalanced.integer == 2 ) ||
				     ( !balancedteams && ui_browserShowTeamBalanced.integer == 1 ) )
				{
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			trap_Cvar_Update( &ui_joinGameType );

			if ( ui_joinGameType.integer != -1 )
			{
				game = atoi( Info_ValueForKey( info, "gametype" ) );

				if ( game != ui_joinGameType.integer )
				{
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			/*trap_Cvar_Update( &ui_serverFilterType );
			   if (ui_serverFilterType.integer > 0) {
			   if (Q_stricmp(Info_ValueForKey(info, "game"), serverFilters[ui_serverFilterType.integer].basedir) != 0) {
			   trap_LAN_MarkServerVisible(ui_netSource.integer, i, qfalse);
			   continue;
			   }
			   } */

			// make sure we never add a favorite server twice
			if ( ui_netSource.integer == AS_FAVORITES )
			{
				UI_RemoveServerFromDisplayList( i );
			}

			// insert the server into the list
			if ( uiInfo.serverStatus.numDisplayServers == 0 )
			{
				char *s = Info_ValueForKey( info, "mapname" );

				if ( s && *s )
				{
					uiInfo.serverStatus.currentServerPreview =
					  trap_R_RegisterShaderNoMip( va( "levelshots/%s", Info_ValueForKey( info, "mapname" ) ) );
				}
				else
				{
					uiInfo.serverStatus.currentServerPreview = trap_R_RegisterShaderNoMip( "levelshots/unknownmap" );
				}
			}

			UI_BinaryServerInsertion( i );

			// done with this server
			if ( ping > /*=*/ 0 )
			{
				trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
				numinvisible++;
			}
		}
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime;

	// if there were no servers visible for ping updates
	if ( !visible )
	{
//      UI_StopServerRefresh();
//      uiInfo.serverStatus.nextDisplayRefresh = 0;
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
static void UI_SortServerStatusInfo( serverStatusInfo_t *info )
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

			if ( !Q_stricmp( serverStatusCvars[ i ].name, info->lines[ j ][ 0 ] ) )
			{
				// swap lines
				tmp1 = info->lines[ index ][ 0 ];
				tmp2 = info->lines[ index ][ 3 ];
				info->lines[ index ][ 0 ] = info->lines[ j ][ 0 ];
				info->lines[ index ][ 3 ] = info->lines[ j ][ 3 ];
				info->lines[ j ][ 0 ] = tmp1;
				info->lines[ j ][ 3 ] = tmp2;

				//
				if ( strlen( serverStatusCvars[ i ].altName ) )
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
static int UI_GetServerStatusInfo( const char *serverAddress, serverStatusInfo_t *info )
{
	char      *p, *score, *ping, *name, *p_val = NULL, *p_name = NULL;
	menuDef_t *menu, *menu2; // we use the URL buttons in several menus
	int       i, len;

	if ( !info )
	{
		trap_LAN_ServerStatus( serverAddress, NULL, 0 );
		return qfalse;
	}

	memset( info, 0, sizeof( *info ) );

	if ( trap_LAN_ServerStatus( serverAddress, info->text, sizeof( info->text ) ) )
	{
		menu = Menus_FindByName( "serverinfo_popmenu" );
		menu2 = Menus_FindByName( "popupError" );

		Q_strncpyz( info->address, serverAddress, sizeof( info->address ) );
		p = info->text;
		info->numLines = 0;
		info->lines[ info->numLines ][ 0 ] = "Address";
		info->lines[ info->numLines ][ 1 ] = "";
		info->lines[ info->numLines ][ 2 ] = "";
		info->lines[ info->numLines ][ 3 ] = info->address;
		info->numLines++;
		// cleanup of the URL cvars
		trap_Cvar_Set( "ui_URL", "" );
		trap_Cvar_Set( "ui_modURL", "" );

		// get the cvars
		while ( p && *p )
		{
			p = strchr( p, '\\' );

			if ( !p )
			{
				break;
			}

			*p++ = '\0';

			if ( p_name )
			{
				if ( !Q_stricmp( p_name, "url" ) )
				{
					trap_Cvar_Set( "ui_URL", p_val );

					if ( menu )
					{
						Menu_ShowItemByName( menu, "serverURL", qtrue );
					}

					if ( menu2 )
					{
						Menu_ShowItemByName( menu2, "serverURL", qtrue );
					}
				}
				else if ( !Q_stricmp( p_name, "mod_url" ) )
				{
					trap_Cvar_Set( "ui_modURL", p_val );

					if ( menu )
					{
						Menu_ShowItemByName( menu, "modURL", qtrue );
					}

					if ( menu2 )
					{
						Menu_ShowItemByName( menu2, "modURL", qtrue );
					}
				}
			}

			if ( *p == '\\' )
			{
				break;
			}

			p_name = p;
			info->lines[ info->numLines ][ 0 ] = p;
			info->lines[ info->numLines ][ 1 ] = "";
			info->lines[ info->numLines ][ 2 ] = "";
			p = strchr( p, '\\' );

			if ( !p )
			{
				break;
			}

			*p++ = '\0';
			p_val = p;
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
				p = strchr( p, ' ' );

				if ( !p )
				{
					break;
				}

				*p++ = '\0';
				ping = p;
				p = strchr( p, ' ' );

				if ( !p )
				{
					break;
				}

				*p++ = '\0';
				name = p;
				Com_sprintf( &info->pings[ len ], sizeof( info->pings ) - len, "%d", i );
				info->lines[ info->numLines ][ 0 ] = &info->pings[ len ];
				len += strlen( &info->pings[ len ] ) + 1;
				info->lines[ info->numLines ][ 1 ] = score;
				info->lines[ info->numLines ][ 2 ] = ping;
				info->lines[ info->numLines ][ 3 ] = name;
				info->numLines++;

				if ( info->numLines >= MAX_SERVERSTATUS_LINES )
				{
					break;
				}

				p = strchr( p, '\\' );

				if ( !p )
				{
					break;
				}

				*p++ = '\0';
				//
				i++;
			}
		}

		UI_SortServerStatusInfo( info );
		return qtrue;
	}

	return qfalse;
}

/*
==================
stristr
==================
*/
static char    *stristr( char *str, char *charset )
{
	int i;

	while ( *str )
	{
		for ( i = 0; charset[ i ] && str[ i ]; i++ )
		{
			if ( toupper( charset[ i ] ) != toupper( str[ i ] ) )
			{
				break;
			}
		}

		if ( !charset[ i ] )
		{
			return str;
		}

		str++;
	}

	return NULL;
}

/*
==================
UI_BuildFindPlayerList
==================
*/
static void UI_BuildFindPlayerList( qboolean force )
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
		memset( &uiInfo.pendingServerStatus, 0, sizeof( uiInfo.pendingServerStatus ) );
		uiInfo.numFoundPlayerServers = 0;
		uiInfo.currentFoundPlayerServer = 0;
		trap_Cvar_VariableStringBuffer( "ui_findPlayer", uiInfo.findPlayerName, sizeof( uiInfo.findPlayerName ) );
		Q_CleanStr( uiInfo.findPlayerName );

		// should have a string of some length
		if ( !strlen( uiInfo.findPlayerName ) )
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

		trap_Cvar_Set( "cl_serverStatusResendTime", va( "%d", resend ) );
		// reset all server status requests
		trap_LAN_ServerStatus( NULL, NULL, 0 );
		//
		uiInfo.numFoundPlayerServers = 1;
		Com_sprintf( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
		             sizeof( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ] ),
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
			if ( UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[ i ].adrstr, &info ) )
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
					Q_strncpyz( name, info.lines[ j ][ 3 ], sizeof( name ) );
					Q_CleanStr( name );

					// if the player name is a substring
					if ( stristr( name, uiInfo.findPlayerName ) )
					{
						// add to found server list if we have space (always leave space for a line with the number found)
						if ( uiInfo.numFoundPlayerServers < MAX_FOUNDPLAYER_SERVERS - 1 )
						{
							//
							Q_strncpyz( uiInfo.foundPlayerServerAddresses[ uiInfo.numFoundPlayerServers - 1 ],
							            uiInfo.pendingServerStatus.server[ i ].adrstr, sizeof( uiInfo.foundPlayerServerAddresses[ 0 ] ) );
							Q_strncpyz( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
							            uiInfo.pendingServerStatus.server[ i ].name, sizeof( uiInfo.foundPlayerServerNames[ 0 ] ) );
							uiInfo.numFoundPlayerServers++;
						}
						else
						{
							// can't add any more so we're done
							uiInfo.pendingServerStatus.num = uiInfo.serverStatus.numDisplayServers;
						}
					}
				}

				Com_sprintf( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
				             sizeof( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ] ),
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
			UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[ i ].adrstr, NULL );
			// reuse pending slot
			uiInfo.pendingServerStatus.server[ i ].valid = qfalse;

			// if we didn't try to get the status of all servers in the main browser yet
			if ( uiInfo.pendingServerStatus.num < uiInfo.serverStatus.numDisplayServers )
			{
				uiInfo.pendingServerStatus.server[ i ].startTime = uiInfo.uiDC.realTime;
				trap_LAN_GetServerAddressString( ui_netSource.integer,
				                                 uiInfo.serverStatus.displayServers[ uiInfo.pendingServerStatus.num ],
				                                 uiInfo.pendingServerStatus.server[ i ].adrstr,
				                                 sizeof( uiInfo.pendingServerStatus.server[ i ].adrstr ) );
				trap_LAN_GetServerInfo( ui_netSource.integer, uiInfo.serverStatus.displayServers[ uiInfo.pendingServerStatus.num ],
				                        infoString, sizeof( infoString ) );
				Q_strncpyz( uiInfo.pendingServerStatus.server[ i ].name, Info_ValueForKey( infoString, "hostname" ),
				            sizeof( uiInfo.pendingServerStatus.server[ 0 ].name ) );
				uiInfo.pendingServerStatus.server[ i ].valid = qtrue;
				uiInfo.pendingServerStatus.num++;
				Com_sprintf( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
				             sizeof( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ] ),
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
			Com_sprintf( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
			             sizeof( uiInfo.foundPlayerServerAddresses[ 0 ] ), "no servers found" );
		}
		else
		{
			Com_sprintf( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
			             sizeof( uiInfo.foundPlayerServerAddresses[ 0 ] ), "%d server%s found with player %s",
			             uiInfo.numFoundPlayerServers - 1, uiInfo.numFoundPlayerServers == 2 ? "" : "s", uiInfo.findPlayerName );
		}

		uiInfo.nextFindPlayerRefresh = 0;
		// show the server status info for the selected server
		UI_FeederSelection( FEEDER_FINDPLAYER, uiInfo.currentFoundPlayerServer );
	}
}

/*
==================
UI_BuildServerStatus
==================
*/
static void UI_BuildServerStatus( qboolean force )
{
	menuDef_t       *menu;
	uiClientState_t cstate;

	trap_GetClientState( &cstate );

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
		Menu_SetFeederSelection( NULL, FEEDER_SERVERSTATUS, 0, NULL );
		uiInfo.serverStatusInfo.numLines = 0;
		// TTimo - reset the server URL / mod URL till we get the new ones
		// the URL buttons are used in the two menus, serverinfo_popmenu and error_popmenu_diagnose
		menu = Menus_FindByName( "serverinfo_popmenu" );

		if ( menu )
		{
			Menu_ShowItemByName( menu, "serverURL", qfalse );
			Menu_ShowItemByName( menu, "modURL", qfalse );
		}

		menu = Menus_FindByName( "popupError" );

		if ( menu )
		{
			Menu_ShowItemByName( menu, "serverURL", qfalse );
			Menu_ShowItemByName( menu, "modURL", qfalse );
		}

		// reset all server status requests
		trap_LAN_ServerStatus( NULL, NULL, 0 );
	}

	if ( cstate.connState < CA_CONNECTED )
	{
		if ( uiInfo.serverStatus.currentServer < 0 || uiInfo.serverStatus.currentServer > uiInfo.serverStatus.numDisplayServers ||
		     uiInfo.serverStatus.numDisplayServers == 0 )
		{
			return;
		}
	}

	if ( UI_GetServerStatusInfo( uiInfo.serverStatusAddress, &uiInfo.serverStatusInfo ) )
	{
		uiInfo.nextServerStatusRefresh = 0;
		UI_GetServerStatusInfo( uiInfo.serverStatusAddress, NULL );
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
static int UI_FeederCount( float feederID )
{
	if ( feederID == FEEDER_HEADS )
	{
		return uiInfo.characterCount;
	}
	else if ( feederID == FEEDER_Q3HEADS )
	{
		return uiInfo.q3HeadCount;
	}
	else if ( feederID == FEEDER_CINEMATICS )
	{
		return uiInfo.movieCount;
	}
	else if ( feederID == FEEDER_SAVEGAMES )
	{
		return uiInfo.savegameCount;
	}
	else if ( feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS )
	{
		return UI_MapCountByGameType( feederID == FEEDER_MAPS ? qtrue : qfalse );
	}
	else if ( feederID == FEEDER_CAMPAIGNS || feederID == FEEDER_ALLCAMPAIGNS )
	{
		return UI_CampaignCount( feederID == FEEDER_CAMPAIGNS ? qtrue : qfalse );
	}
	else if ( feederID == FEEDER_GLINFO )
	{
		return uiInfo.numGlInfoLines;
	}
	else if ( feederID == FEEDER_PROFILES )
	{
		return uiInfo.profileCount;
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

	return 0;
}

static const char *UI_SelectedMap( qboolean singlePlayer, int index, int *actual )
{
	int i, c, game;

	c = 0;
	game = singlePlayer ? uiInfo.gameTypes[ ui_gameType.integer ].gtEnum : ui_netGameType.integer;
	*actual = 0;

	if ( game == GT_WOLF_CAMPAIGN )
	{
		for ( i = 0; i < uiInfo.mapCount; i++ )
		{
			if ( uiInfo.campaignList[ i ].typeBits & ( 1 << GT_WOLF ) )
			{
				if ( c == index )
				{
					*actual = i;
					return uiInfo.campaignList[ i ].campaignName;
				}
				else
				{
					c++;
				}
			}
		}
	}
	else
	{
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
	}

	return "";
}

static const char *UI_SelectedCampaign( int index, int *actual )
{
	int i, c;

	c = 0;
	*actual = 0;

	for ( i = 0; i < uiInfo.campaignCount; i++ )
	{
		if ( ( uiInfo.campaignList[ i ].order == index ) && uiInfo.campaignList[ i ].unlocked )
		{
			*actual = i;
			//if(uiInfo.campaignList[i].unlocked) {
			return uiInfo.campaignList[ i ].campaignName;
			//} else {
			//  return va( "^1%s", uiInfo.campaignList[i].campaignName);
			//}
		}
	}

	return "";
}

static int UI_GetIndexFromSelection( int actual )
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

static void UI_UpdatePendingPings()
{
	trap_LAN_ResetPings( ui_netSource.integer );
	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
}

// NERVE - SMF
static void UI_FeederAddItem( float feederID, const char *name, int index )
{
}

// -NERVE - SMF

//----(SA)  added (whoops, this got nuked in a check-in...)
static const char *UI_FileText( char *fileName )
{
	int          len;
	fileHandle_t f;
	static char  buf[ MAX_MENUDEFFILE ];

//  return "flubber";

	len = trap_FS_FOpenFile( fileName, &f, FS_READ );

	if ( !f )
	{
		return NULL;
	}

	trap_FS_Read( buf, len, f );
	buf[ len ] = 0;
	trap_FS_FCloseFile( f );
	return &buf[ 0 ];
}

//----(SA)  end

const char     *UI_FeederItemText( float feederID, int index, int column, qhandle_t *handles, int *numhandles )
{
	static char info[ MAX_STRING_CHARS ];
	static char hostname[ 1024 ];
	static char clientBuff[ 32 ];
	static char pingstr[ 10 ];
	static int  lastColumn = -1;
	static int  lastTime = 0;

	*numhandles = 0;

	if ( feederID == FEEDER_HEADS )
	{
		if ( index >= 0 && index < uiInfo.characterCount )
		{
			return uiInfo.characterList[ index ].name;
		}
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

		return UI_SelectedMap( feederID == FEEDER_MAPS ? qtrue : qfalse, index, &actual );
	}
	else if ( feederID == FEEDER_CAMPAIGNS || feederID == FEEDER_ALLCAMPAIGNS )
	{
		int actual;

		return UI_SelectedCampaign( index, &actual );
	}
	else if ( feederID == FEEDER_GLINFO )
	{
		if ( index == 0 )
		{
			return ( va( "Vendor: %s", uiInfo.uiDC.glconfig.vendor_string ) );
		}
		else if ( index == 1 )
		{
			return va( "Version: %s: %s", uiInfo.uiDC.glconfig.version_string, uiInfo.uiDC.glconfig.renderer_string );
		}
		else if ( index == 2 )
		{
			return va( "Pixelformat: color(%d-bits) Z(%d-bits) stencil(%d-bits)", uiInfo.uiDC.glconfig.colorBits,
			           uiInfo.uiDC.glconfig.depthBits, uiInfo.uiDC.glconfig.stencilBits );
		}
		else if ( index >= 4 && index < uiInfo.numGlInfoLines )
		{
			return uiInfo.glInfoLines[ index - 4 ];
		}
		else
		{
			return "";
		}
	}
	else if ( feederID == FEEDER_SERVERS )
	{
		if ( index >= 0 && index < uiInfo.serverStatus.numDisplayServers )
		{
			int ping, game, antilag, needpass, friendlyfire, maxlives, punkbuster, weaponrestrictions, balancedteams,
			    serverload;

			if ( lastColumn != column || lastTime > uiInfo.uiDC.realTime + 5000 )
			{
				trap_LAN_GetServerInfo( ui_netSource.integer, uiInfo.serverStatus.displayServers[ index ], info, MAX_STRING_CHARS );
				lastColumn = column;
				lastTime = uiInfo.uiDC.realTime;
			}

			ping = atoi( Info_ValueForKey( info, "ping" ) );

			if ( ping == -1 )
			{
				// if we ever see a ping that is out of date, do a server refresh
				// UI_UpdatePendingPings();
			}

			switch ( column )
			{
				case SORT_HOST:

					//if( ping < 0 ) {
					if ( ping <= 0 )
					{
						return Info_ValueForKey( info, "addr" );
					}
					else
					{
						if ( ui_netSource.integer == AS_LOCAL )
						{
							Com_sprintf( hostname, sizeof( hostname ), "%s [%s]",
							             Info_ValueForKey( info, "hostname" ), netnames[ atoi( Info_ValueForKey( info, "nettype" ) ) ] );
							return hostname;
						}
						else
						{
							return Info_ValueForKey( info, "hostname" );
						}
					}

				case SORT_MAP:
					return Info_ValueForKey( info, "mapname" );

				case SORT_CLIENTS:
					Com_sprintf( clientBuff, sizeof( clientBuff ), "%s/%s", Info_ValueForKey( info, "clients" ),
					             Info_ValueForKey( info, "sv_maxclients" ) );
					return clientBuff;

				case SORT_GAME:
					game = atoi( Info_ValueForKey( info, "gametype" ) );

					if ( ping > /*=*/ 0 && game >= 0 && game < uiInfo.numGameTypes )
					{
						int i;

						//return ETGameTypes[game];
						//return shortETGameTypes[game];

						for ( i = 0; i < uiInfo.numGameTypes; i++ )
						{
							if ( uiInfo.gameTypes[ i ].gtEnum == game )
							{
								return uiInfo.gameTypes[ i ].gameTypeShort;
							}
						}

						if ( i == uiInfo.numGameTypes )
						{
							return "???";
						}
					}
					else
					{
						return "???";
					}

				case SORT_PING:

					//if (ping < 0) {
					if ( ping <= 0 )
					{
						return "...";
					}
					else
					{
						/*antilag = atoi(Info_ValueForKey(info, "g_antilag"));
						   if (!antilag)
						   Com_sprintf( pingstr, sizeof(pingstr), "^3%3i", ping );
						   else */

						serverload = atoi( Info_ValueForKey( info, "serverload" ) );

						if ( serverload == -1 )
						{
							Com_sprintf( pingstr, sizeof( pingstr ), " %3i", ping );
						}
						else if ( serverload > 75 )
						{
							Com_sprintf( pingstr, sizeof( pingstr ), "^1 %3i", ping );
						}
						else if ( serverload > 40 )
						{
							Com_sprintf( pingstr, sizeof( pingstr ), "^3 %3i", ping );
						}
						else
						{
							Com_sprintf( pingstr, sizeof( pingstr ), "^2 %3i", ping );
						}

						return pingstr;
					}

				case SORT_FILTERS:
					if ( ping <= 0 )
					{
						*numhandles = 0;
						return "";
					}
					else
					{
						*numhandles = 7;
						needpass = atoi( Info_ValueForKey( info, "needpass" ) );
						friendlyfire = atoi( Info_ValueForKey( info, "friendlyFire" ) );
						maxlives = atoi( Info_ValueForKey( info, "maxlives" ) );
						punkbuster = atoi( Info_ValueForKey( info, "punkbuster" ) );
						weaponrestrictions = atoi( Info_ValueForKey( info, "weaprestrict" ) );
						antilag = atoi( Info_ValueForKey( info, "g_antilag" ) );
						balancedteams = atoi( Info_ValueForKey( info, "balancedteams" ) );

						if ( needpass )
						{
							handles[ 0 ] = uiInfo.passwordFilter;
						}
						else
						{
							handles[ 0 ] = -1;
						}

						if ( friendlyfire )
						{
							handles[ 1 ] = uiInfo.friendlyFireFilter;
						}
						else
						{
							handles[ 1 ] = -1;
						}

						if ( maxlives )
						{
							handles[ 2 ] = uiInfo.maxLivesFilter;
						}
						else
						{
							handles[ 2 ] = -1;
						}

						if ( punkbuster )
						{
							handles[ 3 ] = uiInfo.punkBusterFilter;
						}
						else
						{
							handles[ 3 ] = -1;
						}

						if ( weaponrestrictions < 100 )
						{
							handles[ 4 ] = uiInfo.weaponRestrictionsFilter;
						}
						else
						{
							handles[ 4 ] = -1;
						}

						if ( antilag )
						{
							handles[ 5 ] = uiInfo.antiLagFilter;
						}
						else
						{
							handles[ 5 ] = -1;
						}

						if ( balancedteams )
						{
							handles[ 6 ] = uiInfo.teamBalanceFilter;
						}
						else
						{
							handles[ 6 ] = -1;
						}

						return "";
					}

				case SORT_FAVOURITES:
					*numhandles = 1;

					if ( trap_LAN_ServerIsInFavoriteList( ui_netSource.integer, uiInfo.serverStatus.displayServers[ index ] ) )
					{
						handles[ 0 ] = uiInfo.uiDC.Assets.checkboxCheck;
					}
					else
					{
						handles[ 0 ] = uiInfo.uiDC.Assets.checkboxCheckNot;
					}

					return "";
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
	else if ( feederID == FEEDER_SAVEGAMES )
	{
		if ( index >= 0 && index < uiInfo.savegameCount )
		{
			return uiInfo.savegameList[ index ].name;
		}
	}
	else if ( feederID == FEEDER_DEMOS )
	{
		if ( index >= 0 && index < uiInfo.demoCount )
		{
			return uiInfo.demoList[ index ];
		}
	}
	else if ( feederID == FEEDER_PROFILES )
	{
		if ( index >= 0 && index < uiInfo.profileCount )
		{
			char buff[ MAX_CVAR_VALUE_STRING ];

			Q_strncpyz( buff, uiInfo.profileList[ index ].name, sizeof( buff ) );
			Q_CleanStr( buff );
			Q_CleanDirName( buff );

			if ( !Q_stricmp( buff, cl_profile.string ) )
			{
				if ( !Q_stricmp( buff, cl_defaultProfile.string ) )
				{
					return ( va( "^7(Default) %s", uiInfo.profileList[ index ].name ) );
				}
				else
				{
					return ( va( "^7%s", uiInfo.profileList[ index ].name ) );
				}
			}
			else if ( !Q_stricmp( buff, cl_defaultProfile.string ) )
			{
				return ( va( "(Default) %s", uiInfo.profileList[ index ].name ) );
			}
			else
			{
				return uiInfo.profileList[ index ].name;
			}
		}
	}

	// -NERVE - SMF
	return "";
}

static qhandle_t UI_FeederItemImage( float feederID, int index )
{
	if ( feederID == FEEDER_HEADS )
	{
		if ( index >= 0 && index < uiInfo.characterCount )
		{
			if ( uiInfo.characterList[ index ].headImage == -1 )
			{
				uiInfo.characterList[ index ].headImage = trap_R_RegisterShaderNoMip( uiInfo.characterList[ index ].imageName );
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
		int game;

		UI_SelectedMap( feederID == FEEDER_MAPS ? qtrue : qfalse, index, &actual );
		index = actual;
		game = feederID == FEEDER_MAPS ? uiInfo.gameTypes[ ui_gameType.integer ].gtEnum : ui_netGameType.integer;

		if ( game == GT_WOLF_CAMPAIGN )
		{
			if ( index >= 0 && index < uiInfo.campaignCount )
			{
				if ( uiInfo.campaignList[ index ].campaignShot == -1 )
				{
					uiInfo.campaignList[ index ].campaignShot =
					  trap_R_RegisterShaderNoMip( uiInfo.campaignList[ index ].campaignShortName );
				}

				return uiInfo.campaignList[ index ].campaignShot;
			}
		}
		else if ( index >= 0 && index < uiInfo.mapCount )
		{
			if ( uiInfo.mapList[ index ].levelShot == -1 )
			{
				uiInfo.mapList[ index ].levelShot = trap_R_RegisterShaderNoMip( uiInfo.mapList[ index ].imageName );
			}

			return uiInfo.mapList[ index ].levelShot;
		}
	}
	else if ( feederID == FEEDER_ALLCAMPAIGNS || feederID == FEEDER_CAMPAIGNS )
	{
		int actual;

		UI_SelectedCampaign( index, &actual );
		index = actual;

		if ( index >= 0 && index < uiInfo.campaignCount )
		{
			if ( uiInfo.campaignList[ index ].campaignShot == -1 )
			{
				uiInfo.campaignList[ index ].campaignShot =
				  trap_R_RegisterShaderNoMip( uiInfo.campaignList[ index ].campaignShortName );
			}

			return uiInfo.campaignList[ index ].campaignShot;
		}
	}
	else if ( feederID == FEEDER_SAVEGAMES )
	{
		if ( index >= 0 && index < uiInfo.savegameCount )
		{
			if ( uiInfo.savegameList[ index ].sshotImage == -1 )
			{
				uiInfo.savegameList[ index ].sshotImage =
				  trap_R_RegisterShaderNoMip( va( "save/images/%s.tga", uiInfo.savegameList[ index ].name ) );
			}

			return uiInfo.savegameList[ index ].sshotImage;
		}
	}

	return 0;
}

void UI_FeederSelection( float feederID, int index )
{
	static char info[ MAX_STRING_CHARS ];

	if ( feederID == FEEDER_HEADS )
	{
		if ( index >= 0 && index < uiInfo.characterCount )
		{
			trap_Cvar_Set( "team_model", uiInfo.characterList[ index ].female ? "janet" : "james" );
			trap_Cvar_Set( "team_headmodel", va( "*%s", uiInfo.characterList[ index ].name ) );
			updateModel = qtrue;
		}
	}
	else if ( feederID == FEEDER_Q3HEADS )
	{
		if ( index >= 0 && index < uiInfo.q3HeadCount )
		{
			trap_Cvar_Set( "model", uiInfo.q3HeadNames[ index ] );
			trap_Cvar_Set( "headmodel", uiInfo.q3HeadNames[ index ] );
			updateModel = qtrue;
		}
	}
	else if ( feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS )
	{
		int actual, map;
		int game;

		map = ( feederID == FEEDER_ALLMAPS ) ? ui_currentNetMap.integer : ui_currentMap.integer;
		game = feederID == FEEDER_MAPS ? uiInfo.gameTypes[ ui_gameType.integer ].gtEnum : ui_netGameType.integer;

		/*if( game == GT_WOLF_CAMPAIGN ) {
		   if (uiInfo.campaignList[map].campaignCinematic >= 0) {
		   trap_CIN_StopCinematic(uiInfo.campaignList[map].campaignCinematic);
		   uiInfo.campaignList[map].campaignCinematic = -1;
		   }
		   } else
		   if (uiInfo.mapList[map].cinematic >= 0) {
		   trap_CIN_StopCinematic(uiInfo.mapList[map].cinematic);
		   uiInfo.mapList[map].cinematic = -1;
		   } */

		UI_SelectedMap( feederID == FEEDER_MAPS ? qtrue : qfalse, index, &actual );
		trap_Cvar_Set( "ui_mapIndex", va( "%d", index ) );
		ui_mapIndex.integer = index;

		// NERVE - SMF - setup advanced server vars
		if ( feederID == FEEDER_ALLMAPS && game != GT_WOLF_CAMPAIGN )
		{
			ui_currentMap.integer = actual;
			trap_Cvar_Set( "ui_currentMap", va( "%d", actual ) );

			/*      trap_Cvar_Set( "ui_userTimelimit", va( "%d", uiInfo.mapList[ui_currentMap.integer].Timelimit ) );
			                        trap_Cvar_Set( "ui_userAxisRespawnTime", va( "%d", uiInfo.mapList[ui_currentMap.integer].AxisRespawnTime ) );
			                        trap_Cvar_Set( "ui_userAlliedRespawnTime", va( "%d", uiInfo.mapList[ui_currentMap.integer].AlliedRespawnTime ) );*/
		}

		// -NERVE - SMF

		if ( feederID == FEEDER_MAPS )
		{
			ui_currentMap.integer = actual;
			trap_Cvar_Set( "ui_currentMap", va( "%d", actual ) );
//          uiInfo.mapList[ui_currentMap.integer].cinematic = trap_CIN_PlayCinematic(va("%s.roq", uiInfo.mapList[ui_currentMap.integer].mapLoadName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
//          UI_LoadBestScores(uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum);
//          trap_Cvar_Set("ui_opponentModel", uiInfo.mapList[ui_currentMap.integer].opponentName);
//          updateOpponentModel = qtrue;
		}
		else
		{
			ui_currentNetMap.integer = actual;
			trap_Cvar_Set( "ui_currentNetMap", va( "%d", actual ) );
//          uiInfo.mapList[ui_currentNetMap.integer].cinematic = trap_CIN_PlayCinematic(va("%s.roq", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
		}
	}
	else if ( feederID == FEEDER_CAMPAIGNS || feederID == FEEDER_ALLCAMPAIGNS )
	{
		int actual, campaign, campaignCount;

		campaign = ( feederID == FEEDER_ALLCAMPAIGNS ) ? ui_currentNetCampaign.integer : ui_currentCampaign.integer;
		campaignCount = UI_CampaignCount( feederID == FEEDER_CAMPAIGNS );

		if ( uiInfo.campaignList[ campaign ].campaignCinematic >= 0 )
		{
			trap_CIN_StopCinematic( uiInfo.campaignList[ campaign ].campaignCinematic );
			uiInfo.campaignList[ campaign ].campaignCinematic = -1;
		}

		trap_Cvar_Set( "ui_campaignIndex", va( "%d", index ) );
		ui_campaignIndex.integer = index;

		if ( index < 0 )
		{
			index = 0;
		}
		else if ( index >= campaignCount )
		{
			index = campaignCount - 1;
		}

		UI_SelectedCampaign( index, &actual );

		if ( feederID == FEEDER_ALLCAMPAIGNS )
		{
			ui_currentCampaign.integer = actual;
			trap_Cvar_Set( "ui_currentCampaign", va( "%d", actual ) );
		}

		if ( feederID == FEEDER_CAMPAIGNS )
		{
			ui_currentCampaign.integer = actual;
			trap_Cvar_Set( "ui_currentCampaign", va( "%d", actual ) );
//          uiInfo.campaignList[ui_currentCampaign.integer].campaignCinematic = trap_CIN_PlayCinematic(va("%s.roq", uiInfo.campaignList[ui_currentCampaign.integer].campaignShortName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
//          UI_LoadBestScores(uiInfo.mapList[ui_currentCampaign.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum);
//          trap_Cvar_Set("ui_opponentModel", uiInfo.mapList[ui_currentMap.integer].opponentName);
//          updateOpponentModel = qtrue;

			/*      if (uiInfo.campaignList[ui_currentCampaign.integer].campaignCinematic < 0) {
			                                uiInfo.campaignList[ui_currentCampaign.integer].campaignCinematic = -2;
			                        }*/

			ui_currentCampaignCompleted.integer =
			  ( uiInfo.campaignList[ ui_currentCampaign.integer ].progress == uiInfo.campaignList[ campaignCount - 1 ].mapCount );
			trap_Cvar_Set( "ui_currentCampaignCompleted",
			               va( "%i",
			                   ( uiInfo.campaignList[ ui_currentCampaign.integer ].progress ==
			                     uiInfo.campaignList[ campaignCount - 1 ].mapCount ) ) );
		}
		else
		{
			ui_currentNetCampaign.integer = actual;
			trap_Cvar_Set( "ui_currentNetCampaign", va( "%d", actual ) );
			uiInfo.campaignList[ ui_currentNetCampaign.integer ].campaignCinematic =
			  trap_CIN_PlayCinematic( va( "%s.roq", uiInfo.campaignList[ ui_currentNetCampaign.integer ].campaignShortName ), 0, 0,
			                          0, 0, ( CIN_loop | CIN_silent ) );
		}
	}
	else if ( feederID == FEEDER_GLINFO )
	{
		//
	}
	else if ( feederID == FEEDER_SERVERS )
	{
		const char *mapName = NULL;

		uiInfo.serverStatus.currentServer = index;
		trap_LAN_GetServerInfo( ui_netSource.integer, uiInfo.serverStatus.displayServers[ index ], info, MAX_STRING_CHARS );

		mapName = Info_ValueForKey( info, "mapname" );

		if ( mapName && *mapName )
		{
			uiInfo.serverStatus.currentServerPreview =
			  trap_R_RegisterShaderNoMip( va( "levelshots/%s", Info_ValueForKey( info, "mapname" ) ) );
		}
		else
		{
			uiInfo.serverStatus.currentServerPreview = trap_R_RegisterShaderNoMip( "levelshots/unknownmap" );
		}

		/*    if( uiInfo.serverStatus.currentServerCinematic >= 0 ) {
		                  trap_CIN_StopCinematic(uiInfo.serverStatus.currentServerCinematic);
		                        uiInfo.serverStatus.currentServerCinematic = -1;
		                }
		                mapName = Info_ValueForKey(info, "mapname");
		                if (mapName && *mapName) {
		                        uiInfo.serverStatus.currentServerCinematic = trap_CIN_PlayCinematic(va("%s.roq", mapName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
		                }*/
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
			Q_strncpyz( uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[ uiInfo.currentFoundPlayerServer ],
			            sizeof( uiInfo.serverStatusAddress ) );
			Menu_SetFeederSelection( NULL, FEEDER_SERVERSTATUS, 0, NULL );
			UI_BuildServerStatus( qtrue );
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
			trap_CIN_StopCinematic( uiInfo.previewMovie );
		}

		uiInfo.previewMovie = -1;
	}
	else if ( feederID == FEEDER_SAVEGAMES )
	{
		uiInfo.savegameIndex = index;
	}
	else if ( feederID == FEEDER_DEMOS )
	{
		uiInfo.demoIndex = index;
	}
	else if ( feederID == FEEDER_PROFILES )
	{
		uiInfo.profileIndex = index;
		trap_Cvar_Set( "ui_profile", uiInfo.profileList[ index ].name );
	}
}

extern void Item_ListBox_MouseEnter( itemDef_t *item, float x, float y, qboolean click );

qboolean UI_FeederSelectionClick( itemDef_t *item )
{
	listBoxDef_t *listPtr = ( listBoxDef_t * ) item->typeData;

	if ( item->special == FEEDER_SERVERS && !Menus_CaptureFuncActive() )
	{
		// ugly hack for favourites handling
		rectDef_t rect;

		Item_ListBox_MouseEnter( item, DC->cursorx, DC->cursory, qtrue );

		rect.x = item->window.rect.x + listPtr->columnInfo[ SORT_FAVOURITES ].pos;
		rect.y = item->window.rect.y + ( listPtr->cursorPos - listPtr->startPos ) * listPtr->elementHeight;
		rect.w = listPtr->columnInfo[ SORT_FAVOURITES ].width;
		rect.h = listPtr->columnInfo[ SORT_FAVOURITES ].width;

		if ( BG_CursorInRect( &rect ) )
		{
			char buff[ MAX_STRING_CHARS ];
			char addr[ MAX_NAME_LENGTH ];

			if ( trap_LAN_ServerIsInFavoriteList( ui_netSource.integer, uiInfo.serverStatus.displayServers[ listPtr->cursorPos ] ) )
			{
				trap_LAN_GetServerInfo( ui_netSource.integer,
				                        uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ], buff,
				                        MAX_STRING_CHARS );
				addr[ 0 ] = '\0';
				Q_strncpyz( addr, Info_ValueForKey( buff, "addr" ), MAX_NAME_LENGTH );

				if ( strlen( addr ) > 0 )
				{
					trap_LAN_RemoveServer( AS_FAVORITES, addr );

					if ( ui_netSource.integer == AS_FAVORITES )
					{
						UI_BuildServerDisplayList( qtrue );
						UI_FeederSelection( FEEDER_SERVERS, 0 );
					}
				}
			}
			else
			{
				char name[ MAX_NAME_LENGTH ];

				trap_LAN_GetServerInfo( ui_netSource.integer,
				                        uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ], buff,
				                        MAX_STRING_CHARS );
				addr[ 0 ] = '\0';
				name[ 0 ] = '\0';
				Q_strncpyz( addr, Info_ValueForKey( buff, "addr" ), MAX_NAME_LENGTH );
				Q_strncpyz( name, Info_ValueForKey( buff, "hostname" ), MAX_NAME_LENGTH );

				if ( *name && *addr )
				{
					trap_LAN_AddServer( AS_FAVORITES, name, addr );
				}
			}

			return qtrue;
		}
	}

	return qfalse;
}

/*
// TTimo: unused
static qboolean Team_Parse(char **p) {
  char *token;
  const char *tempStr;
        int i;

  token = COM_ParseExt(p, qtrue);

  if (token[0] != '{') {
        return qfalse;
  }

  while ( 1 ) {

        token = COM_ParseExt(p, qtrue);

        if (Q_stricmp(token, "}") == 0) {
          return qtrue;
        }

        if ( !token || token[0] == 0 ) {
          return qfalse;
        }

        if (token[0] == '{') {
          // seven tokens per line, team name and icon, and 5 team member names
          if (!String_Parse(p, &uiInfo.teamList[uiInfo.teamCount].teamName) || !String_Parse(p, &tempStr)) {
                return qfalse;
          }


                        uiInfo.teamList[uiInfo.teamCount].imageName = tempStr;
                uiInfo.teamList[uiInfo.teamCount].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[uiInfo.teamCount].imageName);
                  uiInfo.teamList[uiInfo.teamCount].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[uiInfo.teamCount].imageName));
                        uiInfo.teamList[uiInfo.teamCount].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[uiInfo.teamCount].imageName));

                        uiInfo.teamList[uiInfo.teamCount].cinematic = -1;

                        for (i = 0; i < TEAM_MEMBERS; i++) {
                                uiInfo.teamList[uiInfo.teamCount].teamMembers[i] = NULL;
                                if (!String_Parse(p, &uiInfo.teamList[uiInfo.teamCount].teamMembers[i])) {
                                        return qfalse;
                                }
                        }

          Com_Printf("Loaded team %s with team icon %s.\n", uiInfo.teamList[uiInfo.teamCount].teamName, tempStr);
          if (uiInfo.teamCount < MAX_TEAMS) {
                uiInfo.teamCount++;
          } else {
                Com_Printf("Too many teams, last team replaced!\n");
          }
          token = COM_ParseExt(p, qtrue);
          if (token[0] != '}') {
                return qfalse;
          }
        }
  }

  return qfalse;
}
*/

/*
// TTimo: unused
static qboolean Character_Parse(char **p) {
  char *token;
  const char *tempStr;

  token = COM_ParseExt(p, qtrue);

  if (token[0] != '{') {
        return qfalse;
  }


  while ( 1 ) {
        token = COM_ParseExt(p, qtrue);

        if (Q_stricmp(token, "}") == 0) {
          return qtrue;
        }

        if ( !token || token[0] == 0 ) {
          return qfalse;
        }

        if (token[0] == '{') {
          // two tokens per line, character name and sex
          if (!String_Parse(p, &uiInfo.characterList[uiInfo.characterCount].name) || !String_Parse(p, &tempStr)) {
                return qfalse;
          }

          uiInfo.characterList[uiInfo.characterCount].headImage = -1;
                        uiInfo.characterList[uiInfo.characterCount].imageName = String_Alloc(va("models/players/heads/%s/icon_default.tga", uiInfo.characterList[uiInfo.characterCount].name));

          if (tempStr && (tempStr[0] == 'f' || tempStr[0] == 'F')) {
                uiInfo.characterList[uiInfo.characterCount].female = qtrue;
          } else {
                uiInfo.characterList[uiInfo.characterCount].female = qfalse;
          }

          Com_Printf("Loaded %s character %s.\n", tempStr, uiInfo.characterList[uiInfo.characterCount].name);
          if (uiInfo.characterCount < MAX_HEADS) {
                uiInfo.characterCount++;
          } else {
                Com_Printf("Too many characters, last character replaced!\n");
          }

          token = COM_ParseExt(p, qtrue);
          if (token[0] != '}') {
                return qfalse;
          }
        }
  }

  return qfalse;
}
*/

/*
// TTimo: unused
static qboolean Alias_Parse(char **p) {
  char *token;

  token = COM_ParseExt(p, qtrue);

  if (token[0] != '{') {
        return qfalse;
  }

  while ( 1 ) {
        token = COM_ParseExt(p, qtrue);

        if (Q_stricmp(token, "}") == 0) {
          return qtrue;
        }

        if ( !token || token[0] == 0 ) {
          return qfalse;
        }

        if (token[0] == '{') {
          // three tokens per line, character name, bot alias, and preferred action a - all purpose, d - defense, o - offense
          if (!String_Parse(p, &uiInfo.aliasList[uiInfo.aliasCount].name) || !String_Parse(p, &uiInfo.aliasList[uiInfo.aliasCount].ai) || !String_Parse(p, &uiInfo.aliasList[uiInfo.aliasCount].action)) {
                return qfalse;
          }

          Com_Printf("Loaded character alias %s using character ai %s.\n", uiInfo.aliasList[uiInfo.aliasCount].name, uiInfo.aliasList[uiInfo.aliasCount].ai);
          if (uiInfo.aliasCount < MAX_ALIASES) {
                uiInfo.aliasCount++;
          } else {
                Com_Printf("Too many aliases, last alias replaced!\n");
          }

          token = COM_ParseExt(p, qtrue);
          if (token[0] != '}') {
                return qfalse;
          }
        }
  }

  return qfalse;
}
*/

// mode
// 0 - high level parsing
// 1 - team parsing
// 2 - character parsing

/*
// TTimo: unused
static void UI_ParseTeamInfo(const char *teamFile) {
        char  *token;
  char *p;
  char *buff = NULL;
  //int mode = 0; // TTimo: unused

  buff = GetMenuBuffer(teamFile);
  if (!buff) {
        return;
  }

  p = buff;

        while ( 1 ) {
                token = COM_ParseExt( &p, qtrue );
                if( !token || token[0] == 0 || token[0] == '}') {
                        break;
                }

                if ( Q_stricmp( token, "}" ) == 0 ) {
          break;
        }

        if (Q_stricmp(token, "teams") == 0) {

          if (Team_Parse(&p)) {
                continue;
          } else {
                break;
          }
        }

        if (Q_stricmp(token, "characters") == 0) {
          Character_Parse(&p);
        }

        if (Q_stricmp(token, "aliases") == 0) {
          Alias_Parse(&p);
        }
  }
}
*/

/*
==============
GameType_Parse
==============
*/
static qboolean GameType_Parse( char **p, qboolean join )
{
	char *token;

	token = COM_ParseExt( p, qtrue );

	if ( token[ 0 ] != '{' )
	{
		return qfalse;
	}

	if ( join )
	{
		uiInfo.numJoinGameTypes = 0;
	}
	else
	{
		uiInfo.numGameTypes = 0;
	}

	while ( 1 )
	{
		token = COM_ParseExt( p, qtrue );

		if ( Q_stricmp( token, "}" ) == 0 )
		{
			return qtrue;
		}

		if ( !token || token[ 0 ] == 0 )
		{
			return qfalse;
		}

		if ( token[ 0 ] == '{' )
		{
			if ( join )
			{
				if ( !String_Parse( p, &uiInfo.joinGameTypes[ uiInfo.numJoinGameTypes ].gameType ) ||
				     !Int_Parse( p, &uiInfo.joinGameTypes[ uiInfo.numJoinGameTypes ].gtEnum ) )
				{
					return qfalse;
				}
			}
			else
			{
				if ( !Int_Parse( p, &uiInfo.gameTypes[ uiInfo.numGameTypes ].gtEnum ) )
				{
					return qfalse;
				}

				if ( !String_Parse( p, &uiInfo.gameTypes[ uiInfo.numGameTypes ].gameType ) )
				{
					return qfalse;
				}

				if ( !String_Parse( p, &uiInfo.gameTypes[ uiInfo.numGameTypes ].gameTypeShort ) )
				{
					return qfalse;
				}

				if ( !String_Parse( p, &uiInfo.gameTypes[ uiInfo.numGameTypes ].gameTypeDescription ) )
				{
					return qfalse;
				}
			}

			if ( join )
			{
				if ( uiInfo.numJoinGameTypes < MAX_GAMETYPES )
				{
					uiInfo.numJoinGameTypes++;
				}
				else
				{
					Com_Printf( "Too many net game types, last one replace!\n" );
				}
			}
			else
			{
				if ( uiInfo.numGameTypes < MAX_GAMETYPES )
				{
					uiInfo.numGameTypes++;
				}
				else
				{
					Com_Printf( "Too many game types, last one replace!\n" );
				}
			}

			token = COM_ParseExt( p, qtrue );

			if ( token[ 0 ] != '}' )
			{
				return qfalse;
			}
		}
	}

	return qfalse;
}

#if 0
static qboolean MapList_Parse( char **p )
{
	char *token;

	token = COM_ParseExt( p, qtrue );

	if ( token[ 0 ] != '{' )
	{
		return qfalse;
	}

	uiInfo.mapCount = 0;

	while ( 1 )
	{
		token = COM_ParseExt( p, qtrue );

		if ( Q_stricmp( token, "}" ) == 0 )
		{
			return qtrue;
		}

		if ( !token || token[ 0 ] == 0 )
		{
			return qfalse;
		}

		if ( token[ 0 ] == '{' )
		{
			if ( !String_Parse( p, &uiInfo.mapList[ uiInfo.mapCount ].mapName ) ||
			     !String_Parse( p, &uiInfo.mapList[ uiInfo.mapCount ].mapLoadName ) ||
			     !Int_Parse( p, &uiInfo.mapList[ uiInfo.mapCount ].teamMembers ) )
			{
				return qfalse;
			}

			if ( !String_Parse( p, &uiInfo.mapList[ uiInfo.mapCount ].opponentName ) )
			{
				return qfalse;
			}

			uiInfo.mapList[ uiInfo.mapCount ].typeBits = 0;

			while ( 1 )
			{
				token = COM_ParseExt( p, qtrue );

				if ( token[ 0 ] >= '0' && token[ 0 ] <= '9' )
				{
					uiInfo.mapList[ uiInfo.mapCount ].typeBits |= ( 1 << ( token[ 0 ] - 0x030 ) );

					if ( !Int_Parse( p, &uiInfo.mapList[ uiInfo.mapCount ].timeToBeat[ token[ 0 ] - 0x30 ] ) )
					{
						return qfalse;
					}
				}
				else
				{
					break;
				}
			}

			//mapList[mapCount].imageName = String_Alloc(va("levelshots/%s", mapList[mapCount].mapLoadName));
			//if (uiInfo.mapCount == 0) {
			// only load the first cinematic, selection loads the others
			//  uiInfo.mapList[uiInfo.mapCount].cinematic = trap_CIN_PlayCinematic(va("%s.roq",uiInfo.mapList[uiInfo.mapCount].mapLoadName), qfalse, qfalse, qtrue, 0, 0, 0, 0);
			//}
			uiInfo.mapList[ uiInfo.mapCount ].cinematic = -1;
			uiInfo.mapList[ uiInfo.mapCount ].levelShot =
			  trap_R_RegisterShaderNoMip( va( "levelshots/%s_small", uiInfo.mapList[ uiInfo.mapCount ].mapLoadName ) );

			if ( uiInfo.mapCount < MAX_MAPS )
			{
				uiInfo.mapCount++;
			}
			else
			{
				Com_Printf( "Too many maps, last one replaced!\n" );
			}
		}
	}

	return qfalse;
}

#endif

static void UI_ParseGameInfo( const char *teamFile )
{
	char *token;
	char *p;
	char *buff = NULL;

	// int mode = 0; // TTimo: unused

	buff = GetMenuBuffer( teamFile );

	if ( !buff )
	{
		return;
	}

	p = buff;

	while ( 1 )
	{
		token = COM_ParseExt( &p, qtrue );

		if ( !token || token[ 0 ] == 0 || token[ 0 ] == '}' )
		{
			break;
		}

		if ( Q_stricmp( token, "}" ) == 0 )
		{
			break;
		}

		if ( Q_stricmp( token, "gametypes" ) == 0 )
		{
			if ( GameType_Parse( &p, qfalse ) )
			{
				continue;
			}
			else
			{
				break;
			}
		}

//      if (Q_stricmp(token, "maps") == 0) {
		// start a new menu
//          MapList_Parse(&p);
//      }
	}
}

static void UI_Pause( qboolean b )
{
	if ( b )
	{
		// pause the game and set the ui keycatcher
		trap_Cvar_Set( "cl_paused", "1" );
		trap_Key_SetCatcher( KEYCATCH_UI );
	}
	else
	{
		// unpause the game and clear the ui keycatcher
		trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
		trap_Key_ClearStates();
		trap_Cvar_Set( "cl_paused", "0" );
	}
}

/*
// TTimo: unused
static int UI_OwnerDraw_Width(int ownerDraw) {
  return 0;
}
*/

static int UI_PlayCinematic( const char *name, float x, float y, float w, float h )
{
	return trap_CIN_PlayCinematic( name, x, y, w, h, ( CIN_loop | CIN_silent ) );
}

static void UI_StopCinematic( int handle )
{
	if ( handle >= 0 )
	{
		trap_CIN_StopCinematic( handle );
	}
	else
	{
		handle = abs( handle );

		if ( handle == UI_MAPCINEMATIC )
		{
			if ( uiInfo.mapList[ ui_currentMap.integer ].cinematic >= 0 )
			{
				trap_CIN_StopCinematic( uiInfo.mapList[ ui_currentMap.integer ].cinematic );
				uiInfo.mapList[ ui_currentMap.integer ].cinematic = -1;
			}
		}
		else if ( handle == UI_NETMAPCINEMATIC )
		{
			if ( uiInfo.serverStatus.currentServerCinematic >= 0 )
			{
				trap_CIN_StopCinematic( uiInfo.serverStatus.currentServerCinematic );
				uiInfo.serverStatus.currentServerCinematic = -1;
			}
		}
		else if ( handle == UI_CLANCINEMATIC )
		{
			int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );

			if ( i >= 0 && i < uiInfo.teamCount )
			{
				if ( uiInfo.teamList[ i ].cinematic >= 0 )
				{
					trap_CIN_StopCinematic( uiInfo.teamList[ i ].cinematic );
					uiInfo.teamList[ i ].cinematic = -1;
				}
			}
		}
	}
}

static void UI_DrawCinematic( int handle, float x, float y, float w, float h )
{
	trap_CIN_SetExtents( handle, x, y, w, h );
	trap_CIN_DrawCinematic( handle );
}

static void UI_RunCinematicFrame( int handle )
{
	trap_CIN_RunCinematic( handle );
}

/*
=================
PlayerModel_BuildList
=================
*/

/*
// TTimo: unused
static void UI_BuildQ3Model_List( void )
{
        int   numdirs;
        int   numfiles;
        char  dirlist[2048];
        char  filelist[2048];
        char  skinname[64];
        char* dirptr;
        char* fileptr;
        int   i;
        int   j;
        int   dirlen;
        int   filelen;

        uiInfo.q3HeadCount = 0;

        // iterate directory of all player models
        numdirs = trap_FS_GetFileList("models/players", "/", dirlist, 2048 );
        dirptr  = dirlist;
        for (i=0; i<numdirs && uiInfo.q3HeadCount < MAX_PLAYERMODELS; i++,dirptr+=dirlen+1)
        {
                dirlen = strlen(dirptr);

                if (dirlen && dirptr[dirlen-1]=='/') dirptr[dirlen-1]='\0';

                if (!strcmp(dirptr,".") || !strcmp(dirptr,".."))
                        continue;

                // iterate all skin files in directory
                numfiles = trap_FS_GetFileList( va("models/players/%s",dirptr), "tga", filelist, 2048 );
                fileptr  = filelist;
                for (j=0; j<numfiles && uiInfo.q3HeadCount < MAX_PLAYERMODELS;j++,fileptr+=filelen+1)
                {
                        filelen = strlen(fileptr);

                        COM_StripExtension(fileptr,skinname);

                        // look for icon_????
                        if (Q_stricmpn(skinname, "icon_", 5) == 0 && !(Q_stricmp(skinname,"icon_blue") == 0 || Q_stricmp(skinname,"icon_red") == 0))
                        {
                                if (Q_stricmp(skinname, "icon_default") == 0) {
                                        Com_sprintf( uiInfo.q3HeadNames[uiInfo.q3HeadCount], sizeof(uiInfo.q3HeadNames[uiInfo.q3HeadCount]), dirptr);
                                } else {
                                        Com_sprintf( uiInfo.q3HeadNames[uiInfo.q3HeadCount], sizeof(uiInfo.q3HeadNames[uiInfo.q3HeadCount]), "%s/%s",dirptr, skinname + 5);
                                }
                                uiInfo.q3HeadIcons[uiInfo.q3HeadCount++] = trap_R_RegisterShaderNoMip(va("models/players/%s/%s",dirptr,skinname));
                        }

                }
        }

}
*/

/*
=================
UI_Init
=================
*/
void _UI_Init( qboolean inGameLoad )
{
	int start, x;

	//uiInfo.inGameLoad = inGameLoad;

	UI_RegisterCvars();
	UI_InitMemory();
	trap_PC_RemoveAllGlobalDefines();

	trap_Cvar_Set( "ui_menuFiles", "ui/menus.txt" );  // NERVE - SMF - we need to hardwire for wolfMP

	// cache redundant calulations
	trap_GetGlconfig( &uiInfo.uiDC.glconfig );

	UI_ParseGLConfig();

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
	uiInfo.uiDC.drawTextExt = &Text_Paint_Ext;
	uiInfo.uiDC.textWidth = &Text_Width;
	uiInfo.uiDC.textWidthExt = &Text_Width_Ext;
	uiInfo.uiDC.multiLineTextWidth = &Multiline_Text_Width;
	uiInfo.uiDC.textHeight = &Text_Height;
	uiInfo.uiDC.textHeightExt = &Text_Height_Ext;
	uiInfo.uiDC.multiLineTextHeight = &Multiline_Text_Height;
	uiInfo.uiDC.textFont = &Text_SetActiveFont;
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
	uiInfo.uiDC.fileText = &UI_FileText; //----(SA)  re-added
	uiInfo.uiDC.feederSelection = &UI_FeederSelection;
	uiInfo.uiDC.feederSelectionClick = &UI_FeederSelectionClick;
	uiInfo.uiDC.feederAddItem = &UI_FeederAddItem; // NERVE - SMF
	uiInfo.uiDC.setBinding = &trap_Key_SetBinding;
	uiInfo.uiDC.getBindingBuf = &trap_Key_GetBindingBuf;
	uiInfo.uiDC.getKeysForBinding = &trap_Key_KeysForBinding;
	uiInfo.uiDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	uiInfo.uiDC.keyIsDown = &trap_Key_IsDown;
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
	uiInfo.uiDC.translateString = &trap_TranslateString; // NERVE - SMF
	uiInfo.uiDC.checkAutoUpdate = &trap_CheckAutoUpdate; // DHM - Nerve
	uiInfo.uiDC.getAutoUpdate = &trap_GetAutoUpdate; // DHM - Nerve

	uiInfo.uiDC.descriptionForCampaign = &UI_DescriptionForCampaign;
	uiInfo.uiDC.nameForCampaign = &UI_NameForCampaign;
	uiInfo.uiDC.add2dPolys = &trap_R_Add2dPolys;
	uiInfo.uiDC.updateScreen = &trap_UpdateScreen;
	uiInfo.uiDC.getHunkData = &trap_GetHunkData;
	uiInfo.uiDC.getConfigString = &trap_GetConfigString;

	Init_Display( &uiInfo.uiDC );

	String_Init();

	uiInfo.uiDC.whiteShader = trap_R_RegisterShaderNoMip( "white" );

	AssetCache();

	uiInfo.passwordFilter = trap_R_RegisterShaderNoMip( "ui/assets/filter_pass.tga" );
	uiInfo.friendlyFireFilter = trap_R_RegisterShaderNoMip( "ui/assets/filter_ff.tga" );
	uiInfo.maxLivesFilter = trap_R_RegisterShaderNoMip( "ui/assets/filter_lives.tga" );
	uiInfo.punkBusterFilter = trap_R_RegisterShaderNoMip( "ui/assets/filter_pb.tga" );
	uiInfo.weaponRestrictionsFilter = trap_R_RegisterShaderNoMip( "ui/assets/filter_weap.tga" );
	uiInfo.antiLagFilter = trap_R_RegisterShaderNoMip( "ui/assets/filter_antilag.tga" );
	uiInfo.teamBalanceFilter = trap_R_RegisterShaderNoMip( "ui/assets/filter_balance.tga" );

	uiInfo.campaignMap = trap_R_RegisterShaderNoMip( "gfx/loading/camp_map.tga" );

	start = trap_Milliseconds();

	uiInfo.teamCount = 0;
	uiInfo.characterCount = 0;
	uiInfo.aliasCount = 0;

	UI_ParseGameInfo( "gameinfo.txt" );

	UI_LoadMenus( "ui/menus.txt", qfalse );

	Menus_CloseAll();

	trap_LAN_LoadCachedServers();
	UI_LoadBestScores( uiInfo.mapList[ 0 ].mapLoadName, uiInfo.gameTypes[ ui_gameType.integer ].gtEnum );

	// sets defaults for ui temp cvars
	// rain - bounds check array index, although I'm pretty sure this
	// stuff isn't used anymore...
	x = ( int ) trap_Cvar_VariableValue( "color" ) - 1;

	if ( x < 0 || x >= sizeof( gamecodetoui ) / sizeof( gamecodetoui[ 0 ] ) )
	{
		x = 0;
	}

	uiInfo.effectsColor = gamecodetoui[ x ];
	uiInfo.currentCrosshair = ( int ) trap_Cvar_VariableValue( "cg_drawCrosshair" );
	trap_Cvar_Set( "ui_mousePitch", ( trap_Cvar_VariableValue( "m_pitch" ) >= 0 ) ? "0" : "1" );

	uiInfo.serverStatus.currentServerCinematic = -1;
	uiInfo.previewMovie = -1;

	if ( trap_Cvar_VariableValue( "ui_TeamArenaFirstRun" ) == 0 )
	{
		trap_Cvar_Set( "s_volume", "0.8" );
		trap_Cvar_Set( "s_musicvolume", "0.5" );
		trap_Cvar_Set( "ui_TeamArenaFirstRun", "1" );
	}

	trap_Cvar_Register( NULL, "debug_protocol", "", 0 );

	// Arnout: default to Campaign
	//trap_Cvar_Set("ui_netGameType", "4");

	// init Yes/No once for cl_language -> server browser (punkbuster)
	Q_strncpyz( translated_yes, DC->translateString( "Yes" ), sizeof( translated_yes ) );
	Q_strncpyz( translated_no, DC->translateString( "NO" ), sizeof( translated_no ) );

	trap_AddCommand( "campaign" );
	trap_AddCommand( "listcampaigns" );
}

/*
=================
UI_KeyEvent
=================
*/
void _UI_KeyEvent( int key, qboolean down )
{
	static qboolean bypassKeyClear = qfalse;

	if ( Menu_Count() > 0 )
	{
		menuDef_t *menu = Menu_GetFocused();

		if ( menu )
		{
			if ( trap_Cvar_VariableValue( "cl_bypassMouseInput" ) )
			{
				bypassKeyClear = qtrue;
			}

//          if (key == K_ESCAPE && down && !Menus_AnyFullScreenVisible()) {
//              Menus_CloseAll();
//          } else {
//              Menu_HandleKey(menu, key, down );
//          }
			// always have the menus do the proper handling
			Menu_HandleKey( menu, key, down );
		}
		else
		{
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );

			// NERVE - SMF - we don't want to clear key states if bypassing input
			if ( !bypassKeyClear )
			{
				trap_Key_ClearStates();
			}

			if ( cl_bypassMouseInput.integer )
			{
				if ( !trap_Key_GetCatcher() )
				{
					trap_Cvar_Set( "cl_bypassMouseInput", 0 );
				}
			}

			bypassKeyClear = qfalse;

			trap_Cvar_Set( "cl_paused", "0" );
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
void _UI_MouseEvent( int dx, int dy )
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
		Display_MouseMove( NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory );
	}
}

void UI_LoadNonIngame()
{
	const char *menuSet = UI_Cvar_VariableString( "ui_menuFiles" );

	if ( menuSet == NULL || menuSet[ 0 ] == '\0' )
	{
		menuSet = "ui/menus.txt";
	}

	UI_LoadMenus( menuSet, qfalse );
	uiInfo.inGameLoad = qfalse;
}

//----(SA)  added
static uiMenuCommand_t menutype = UIMENU_NONE;

uiMenuCommand_t _UI_GetActiveMenu( void )
{
	return menutype;
}

//----(SA)  end

#define MISSING_FILES_MSG "The following packs are missing:"

void _UI_SetActiveMenu( uiMenuCommand_t menu )
{
	char buf[ 4096 ]; // com_errorMessage can go up to 4096
	char *missing_files;

	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	if ( Menu_Count() > 0 )
	{
		vec3_t v;

		v[ 0 ] = v[ 1 ] = v[ 2 ] = 0;

		menutype = menu; //----(SA)  added

		switch ( menu )
		{
			case UIMENU_NONE:
				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();

				return;

			case UIMENU_MAIN:
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_CloseAll();
				//Menus_ActivateByName( "background_1", qtrue );
				Menus_ActivateByName( "backgroundmusic", qtrue );  // Arnout: not nice, but best way to do it - putting the music in it's own menudef

				// makes sure it doesn't get restarted every time you reach the main menu
				if ( !cl_profile.string[ 0 ] )
				{
					//Menus_ActivateByName( "profilelogin", qtrue );
					// FIXME: initial profile popup
					// FIXED: handled in opener now
					Menus_ActivateByName( "main_opener", qtrue );
				}
				else
				{
					Menus_ActivateByName( "main_opener", qtrue );
				}

				trap_Cvar_VariableStringBuffer( "com_errorMessage", buf, sizeof( buf ) );

				// JPW NERVE stricmp() is silly but works, take a look at error.menu to see why.  I think this is bustified in q3ta
				// NOTE TTimo - I'm not sure Q_stricmp is useful to anything anymore
				// show_bug.cgi?id=507
				// TTimo - improved and tweaked that area a whole bunch
				if ( ( *buf ) && ( Q_stricmp( buf, ";" ) ) )
				{
					trap_Cvar_Set( "ui_connecting", "0" );

					if ( !Q_stricmpn( buf, "Invalid password", 16 ) )
					{
						trap_Cvar_Set( "com_errorMessage", trap_TranslateString( buf ) );   // NERVE - SMF
						Menus_ActivateByName( "popupPassword", qtrue );
					}
					else if ( strlen( buf ) > 5 && !Q_stricmpn( buf, "ET://", 5 ) )
					{
						// fretn
						Q_strncpyz( buf, buf + 5, sizeof( buf ) );
						Com_Printf( "Server is full, redirect to: %s\n", buf );

						switch ( ui_autoredirect.integer )
						{
								//auto-redirect
							case 1:
								trap_Cvar_Set( "com_errorMessage", "" );
								trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", buf ) );
								break;

								//prompt (default)
							default:
								trap_Cvar_Set( "com_errorMessage", buf );
								Menus_ActivateByName( "popupServerRedirect", qtrue );
								break;
						}
					}
					else
					{
						qboolean pb_enable = qfalse;

						if ( strstr( buf, "must be Enabled" ) )
						{
							pb_enable = qtrue;
						}

						trap_Cvar_Set( "com_errorMessage", trap_TranslateString( buf ) );   // NERVE - SMF

						// hacky, wanted to have the printout of missing files
						// text printing limitations force us to keep it all in a single message
						// NOTE: this works thanks to flip flop in UI_Cvar_VariableString
						if ( UI_Cvar_VariableString( "com_errorDiagnoseIP" ) [ 0 ] )
						{
							missing_files = UI_Cvar_VariableString( "com_missingFiles" );

							if ( missing_files[ 0 ] )
							{
								trap_Cvar_Set( "com_errorMessage",
								               va( "%s\n\n%s\n%s",
								                   UI_Cvar_VariableString( "com_errorMessage" ),
								                   trap_TranslateString( MISSING_FILES_MSG ), missing_files ) );
							}
						}

						if ( pb_enable )
						{
							Menus_ActivateByName( "popupError_pbenable", qtrue );
						}
						else
						{
							Menus_ActivateByName( "popupError", qtrue );
						}
					}
				}

				trap_S_FadeAllSound( 1.0f, 1000, qfalse );  // make sure sound fades up

#ifdef SAVEGAME_SUPPORT
				// ensure savegames are loadable
				trap_Cvar_Set( "g_reloading", "0" );
#endif // SAVEGAME_SUPPORT
				return;

			case UIMENU_TEAM:
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_ActivateByName( "team", qtrue );
				return;

			case UIMENU_NEED_CD:
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_ActivateByName( "needcd", qtrue );
				return;

			case UIMENU_BAD_CD_KEY:
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_ActivateByName( "badcd", qtrue );
				return;

			case UIMENU_INGAME:
				if ( g_gameType.integer == GT_SINGLE_PLAYER )
				{
					trap_Cvar_Set( "cl_paused", "1" );
				}

				trap_Key_SetCatcher( KEYCATCH_UI );
				UI_BuildPlayerList();
				Menu_SetFeederSelection( NULL, FEEDER_PLAYER_LIST, 0, NULL );
				Menus_CloseAll();
				//trap_Cvar_Set( "authLevel", "0" ); // just used for testing...
				Menus_ActivateByName( "ingame_main", qtrue );
				return;

				// NERVE - SMF
			case UIMENU_WM_QUICKMESSAGE:
				uiInfo.uiDC.cursorx = 639;
				uiInfo.uiDC.cursory = 479;
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_CloseAll();
				Menus_OpenByName( "wm_quickmessage" );
				return;

			case UIMENU_WM_QUICKMESSAGEALT:
				uiInfo.uiDC.cursorx = 639;
				uiInfo.uiDC.cursory = 479;
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_CloseAll();
				Menus_OpenByName( "wm_quickmessageAlt" );
				return;

			case UIMENU_WM_FTQUICKMESSAGE:
				uiInfo.uiDC.cursorx = 639;
				uiInfo.uiDC.cursory = 479;
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_CloseAll();
				Menus_OpenByName( "wm_ftquickmessage" );
				return;

			case UIMENU_WM_FTQUICKMESSAGEALT:
				uiInfo.uiDC.cursorx = 639;
				uiInfo.uiDC.cursory = 479;
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_CloseAll();
				Menus_OpenByName( "wm_ftquickmessageAlt" );
				return;

			case UIMENU_WM_TAPOUT:
				uiInfo.uiDC.cursorx = 639;
				uiInfo.uiDC.cursory = 479;
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_CloseAll();
				Menus_OpenByName( "tapoutmsg" );
				return;

			case UIMENU_WM_TAPOUT_LMS:
				uiInfo.uiDC.cursorx = 639;
				uiInfo.uiDC.cursory = 479;
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_CloseAll();
				Menus_OpenByName( "tapoutmsglms" );
				return;

			case UIMENU_WM_AUTOUPDATE:
				// TTimo - changing the auto-update strategy to a modal prompt
				Menus_OpenByName( "wm_autoupdate_modal" );
				return;
				// -NERVE - SMF

				// ydnar: say, team say, etc
			case -1://UIMENU_INGAME_MESSAGEMODE:
				//trap_Cvar_Set( "cl_paused", "1" );
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_OpenByName( "ingame_messagemode" );
				return;

				// Omni-bot BEGIN
				// cs: omnibot waypoint menu
			case UIMENU_INGAME_OMNIBOTMENU:
				uiInfo.uiDC.cursorx = 639;
				uiInfo.uiDC.cursory = 479;
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_CloseAll();
				Menus_OpenByName( "omnibot" );
				return;
				// Omni-bot END

			default:
				return; // TTimo: a lot of not handled
		}
	}
}

qboolean _UI_IsFullscreen( void )
{
	return Menus_AnyFullScreenVisible();
}

//static connstate_t    lastConnState;
//static char           lastLoadingText[MAX_INFO_VALUE];

void UI_ReadableSize( char *buf, int bufsize, int value )
{
	if ( value > 1024 * 1024 * 1024 )
	{
		// gigs
		Com_sprintf( buf, bufsize, "%d", value / ( 1024 * 1024 * 1024 ) );
		Com_sprintf( buf + strlen( buf ), bufsize - strlen( buf ), ".%02d GB",
		             ( value % ( 1024 * 1024 * 1024 ) ) * 100 / ( 1024 * 1024 * 1024 ) );
	}
	else if ( value > 1024 * 1024 )
	{
		// megs
		Com_sprintf( buf, bufsize, "%d", value / ( 1024 * 1024 ) );
		Com_sprintf( buf + strlen( buf ), bufsize - strlen( buf ), ".%02d MB", ( value % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
	}
	else if ( value > 1024 )
	{
		// kilos
		Com_sprintf( buf, bufsize, "%d KB", value / 1024 );
	}
	else
	{
		// bytes
		Com_sprintf( buf, bufsize, "%d bytes", value );
	}
}

// Assumes time is in sec
void UI_PrintTime( char *buf, int bufsize, int time )
{
	//time /= 1000;  // change to seconds

	if ( time > 3600 )
	{
		// in the hours range
		Com_sprintf( buf, bufsize, "%d hr %d min", time / 3600, ( time % 3600 ) / 60 );
	}
	else if ( time > 60 )
	{
		// mins
		Com_sprintf( buf, bufsize, "%d min %d sec", time / 60, time % 60 );
	}
	else
	{
		// secs
		Com_sprintf( buf, bufsize, "%d sec", time );
	}
}

void Text_PaintCenter( float x, float y, float scale, vec4_t color, const char *text, float adjust )
{
	int len = Text_Width( text, scale, 0 );

	Text_Paint( x - len / 2, y, scale, color, text, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );
}

#if 0 // rain - unused
#define ESTIMATES 80
static void UI_DisplayDownloadInfo( const char *downloadName, float centerPoint, float yStart, float scale )
{
	static char dlText[] = "Downloading:";
	static char etaText[] = "Estimated time left:";
	static char xferText[] = "Transfer rate:";
	static int  tleEstimates[ ESTIMATES ] = { 60,     60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
	                                        60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
	                                        60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
	                                        60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60
	                                        };
	static int  tleIndex = 0;

	int         downloadSize, downloadCount, downloadTime;
	char        dlSizeBuf[ 64 ], totalSizeBuf[ 64 ], xferRateBuf[ 64 ], dlTimeBuf[ 64 ];
	int         xferRate;
	const char  *s;

	vec4_t      bg_color = { 0.3f, 0.3f, 0.3f, 0.8f };

	downloadSize = trap_Cvar_VariableValue( "cl_downloadSize" );
	downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );
	downloadTime = trap_Cvar_VariableValue( "cl_downloadTime" );

	// Background
	UI_FillRect( 0, yStart + 185, 640, 83, bg_color );

	UI_SetColor( colorYellow );
	Text_Paint( 92, yStart + 210, scale, colorYellow, dlText, 0, 64, ITEM_TEXTSTYLE_SHADOWEDMORE );
	Text_Paint( 35, yStart + 235, scale, colorYellow, etaText, 0, 64, ITEM_TEXTSTYLE_SHADOWEDMORE );
	Text_Paint( 86, yStart + 260, scale, colorYellow, xferText, 0, 64, ITEM_TEXTSTYLE_SHADOWEDMORE );

	if ( downloadSize > 0 )
	{
		s = va( "%s (%d%%)", downloadName, downloadCount * 100 / downloadSize );
	}
	else
	{
		s = downloadName;
	}

	Text_Paint( 260, yStart + 210, scale, colorYellow, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );

	UI_ReadableSize( dlSizeBuf, sizeof dlSizeBuf, downloadCount );
	UI_ReadableSize( totalSizeBuf, sizeof totalSizeBuf, downloadSize );

	if ( downloadCount < 4096 || !downloadTime )
	{
		Text_PaintCenter( centerPoint, yStart + 235, scale, colorYellow, "estimating", 0 );
		Text_PaintCenter( centerPoint, yStart + 340, scale, colorYellow, va( "(%s of %s copied)", dlSizeBuf, totalSizeBuf ), 0 );
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

		UI_ReadableSize( xferRateBuf, sizeof xferRateBuf, xferRate );

		// Extrapolate estimated completion time
		if ( downloadSize && xferRate )
		{
			int n = downloadSize / xferRate; // estimated time for entire d/l in secs
			int timeleft = 0, i;

			// We do it in K (/1024) because we'd overflow around 4MB
			tleEstimates[ tleIndex ] = ( n - ( ( ( downloadCount / 1024 ) * n ) / ( downloadSize / 1024 ) ) );
			tleIndex++;

			if ( tleIndex >= ESTIMATES )
			{
				tleIndex = 0;
			}

			for ( i = 0; i < ESTIMATES; i++ )
			{
				timeleft += tleEstimates[ i ];
			}

			timeleft /= ESTIMATES;

			UI_PrintTime( dlTimeBuf, sizeof dlTimeBuf, timeleft );

			Text_Paint( 260, yStart + 235, scale, colorYellow, dlTimeBuf, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );
			Text_PaintCenter( centerPoint, yStart + 340, scale, colorYellow, va( "(%s of %s copied)", dlSizeBuf, totalSizeBuf ), 0 );
		}
		else
		{
			Text_PaintCenter( centerPoint, yStart + 235, scale, colorYellow, "estimating", 0 );

			if ( downloadSize )
			{
				Text_PaintCenter( centerPoint, yStart + 340, scale, colorYellow, va( "(%s of %s copied)", dlSizeBuf, totalSizeBuf ),
				                  0 );
			}
			else
			{
				Text_PaintCenter( centerPoint, yStart + 340, scale, colorYellow, va( "(%s copied)", dlSizeBuf ), 0 );
			}
		}

		if ( xferRate )
		{
			Text_Paint( 260, yStart + 260, scale, colorYellow, va( "%s/Sec", xferRateBuf ), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );
		}
	}
}

#endif

/*
========================
UI_DrawConnectScreen

This will also be overlaid on the cgame info screen during loading
to prevent it from blinking away too rapidly on local or lan games.
========================
*/
#define CP_LINEWIDTH 50

void UI_DrawConnectScreen( qboolean overlay )
{
//  static qboolean playingMusic = qfalse;

	if ( !overlay )
	{
		UI_DrawLoadPanel( qfalse, qfalse, qfalse );
	}
	else
	{
//      if( !playingMusic ) {
//          trap_S_StartBackgroundTrack( "sound/music/level_load.wav", "", 1000 );
//          playingMusic = qtrue;
//      }
	}

	/*  if( !overlay ) {
	                BG_DrawConnectScreen( qfalse );
	        }*/

	/*
	        char      *s;
	        uiClientState_t cstate;
	        char      info[MAX_INFO_VALUE];
	        char text[256];
	        float centerPoint, yStart, scale;
	        vec4_t color = { 0.3f, 0.3f, 0.3f, 0.8f };
	//  static qboolean playingMusic = qfalse;

	        char downloadName[MAX_INFO_VALUE];

	        menuDef_t *menu = Menus_FindByName("Connect");

	        if ( !overlay && menu ) {
	                Menu_Paint(menu, qtrue);
	        }

	        if (!overlay) {
	                centerPoint = 320;
	                yStart = 130;
	                scale = 0.4f;
	        } else {
	                centerPoint = 320;
	                yStart = 32;
	                scale = 0.6f;

	                // see what information we should display
	                trap_GetClientState( &cstate );


	                return;
	        }

	//  playingMusic = qfalse;

	        // see what information we should display
	        trap_GetClientState( &cstate );

	        info[0] = '\0';

	        if (!Q_stricmp(cstate.servername,"localhost")) {
	                Text_PaintCenter(centerPoint, yStart + 48, scale, colorWhite,va( "Enemy Territory - Version: %s", Q3_VERSION ), ITEM_TEXTSTYLE_SHADOWEDMORE);
	        } else {
	                strcpy(text, va( trap_TranslateString( "Connecting to %s" ), cstate.servername));
	                Text_PaintCenter(centerPoint, yStart + 48, scale, colorWhite,text , ITEM_TEXTSTYLE_SHADOWEDMORE);
	        }

	        // display global MOTD at bottom (don't draw during download, the space is already used)
	        // moved downloadName query up, this is used in CA_CONNECTED
	        trap_Cvar_VariableStringBuffer( "cl_downloadName", downloadName, sizeof(downloadName) );

	        if (!*downloadName) {
	                Text_PaintCenter(centerPoint, 475, scale, colorWhite, Info_ValueForKey( cstate.updateInfoString, "motd" ), 0);
	        }

	        // print any server info (server full, bad version, etc)
	        // DHM - Nerve :: This now accepts strings up to 256 chars long, and will break them up into multiple lines.
	        //          They are also now printed in Yellow for readability.
	        if ( cstate.connState < CA_CONNECTED ) {
	                char  *s;
	                char  ps[60];
	                int   i, len, index = 0, yPrint = yStart + 210;
	                qboolean neednewline = qfalse;

	                s = trap_TranslateString( cstate.messageString );
	                len = strlen( s );

	                for ( i = 0; i < len; i++, index++ ) {

	                        // copy to temp buffer
	                        ps[index] = s[i];

	                        if ( index > (CP_LINEWIDTH - 10) && i > 0 )
	                                neednewline = qtrue;

	                        // if out of temp buffer room OR end of string OR it is time to linebreak & we've found a space
	                        if ( (index >= 58) || (i == (len-1)) || (neednewline && s[i] == ' ') ) {
	                                ps[index+1] = '\0';

	                                DC->fillRect(0, yPrint - 17, 640, 22, color);
	                                Text_PaintCenter(centerPoint, yPrint, scale, colorYellow, ps, 0);

	                                neednewline = qfalse;
	                                yPrint += 22;   // next line
	                                index = -1;     // sigh, for loop will increment to 0
	                        }
	                }

	        }

	        if ( lastConnState > cstate.connState ) {
	                lastLoadingText[0] = '\0';
	        }
	        lastConnState = cstate.connState;

	        switch ( cstate.connState ) {
	        case CA_CONNECTING:
	                s = va( trap_TranslateString( "Awaiting connection...%i" ), cstate.connectPacketCount);
	                break;
	        case CA_CHALLENGING:
	                s = va( trap_TranslateString( "Awaiting challenge...%i" ), cstate.connectPacketCount);
	                break;
	        case CA_CONNECTED:
	                        if (*downloadName) {
	                                UI_DisplayDownloadInfo( downloadName, centerPoint, yStart, scale );
	                                return;
	                        }
	                s = trap_TranslateString( "Awaiting gamestate..." );
	                break;
	        case CA_LOADING:
	                return;
	        case CA_PRIMED:
	                return;
	        default:
	                return;
	        }


	        if (Q_stricmp(cstate.servername,"localhost")) {
	                Text_PaintCenter(centerPoint, yStart + 80, scale, colorWhite, s, 0);
	        }

	        // password required / connection rejected information goes here
	*/
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
	int      modificationCount; // for tracking changes
} cvarTable_t;

vmCvar_t ui_ffa_fraglimit;
vmCvar_t ui_ffa_timelimit;

vmCvar_t ui_team_fraglimit;
vmCvar_t ui_team_timelimit;
vmCvar_t ui_team_friendly;

vmCvar_t ui_ctf_capturelimit;
vmCvar_t ui_ctf_timelimit;
vmCvar_t ui_ctf_friendly;

vmCvar_t ui_arenasFile;
vmCvar_t ui_botsFile;
vmCvar_t ui_spScores1;
vmCvar_t ui_spScores2;
vmCvar_t ui_spScores3;
vmCvar_t ui_spScores4;
vmCvar_t ui_spScores5;
vmCvar_t ui_spAwards;
vmCvar_t ui_spVideos;
vmCvar_t ui_spSkill;

vmCvar_t ui_spSelection;
vmCvar_t ui_master;

vmCvar_t ui_brassTime;
vmCvar_t ui_drawCrosshair;
vmCvar_t ui_drawCrosshairNames;
vmCvar_t ui_drawCrosshairPickups; //----(SA) added
vmCvar_t ui_marks;

// JOSEPH 12-3-99
vmCvar_t ui_autoactivate;

// END JOSEPH

vmCvar_t ui_server1;
vmCvar_t ui_server2;
vmCvar_t ui_server3;
vmCvar_t ui_server4;
vmCvar_t ui_server5;
vmCvar_t ui_server6;
vmCvar_t ui_server7;
vmCvar_t ui_server8;
vmCvar_t ui_server9;
vmCvar_t ui_server10;
vmCvar_t ui_server11;
vmCvar_t ui_server12;
vmCvar_t ui_server13;
vmCvar_t ui_server14;
vmCvar_t ui_server15;
vmCvar_t ui_server16;

vmCvar_t ui_cdkeychecked;

vmCvar_t ui_selectedPlayer;
vmCvar_t ui_selectedPlayerName;
vmCvar_t ui_netSource;
vmCvar_t ui_menuFiles;
vmCvar_t ui_gameType;
vmCvar_t ui_netGameType;

//vmCvar_t  ui_actualNetGameType;
vmCvar_t ui_joinGameType;
vmCvar_t ui_dedicated;

vmCvar_t ui_clipboardName; // the name of the group for the current clipboard item //----(SA)  added

vmCvar_t ui_notebookCurrentPage; //----(SA)  added
vmCvar_t ui_clipboardName; // the name of the group for the current clipboard item //----(SA)  added

// NERVE - SMF - cvars for multiplayer
vmCvar_t ui_serverFilterType;
vmCvar_t ui_currentNetMap;
vmCvar_t ui_currentMap;
vmCvar_t ui_mapIndex;

vmCvar_t ui_browserMaster;
vmCvar_t ui_browserGameType;
vmCvar_t ui_browserSortKey;
vmCvar_t ui_browserShowEmptyOrFull;
vmCvar_t ui_browserShowPasswordProtected;
vmCvar_t ui_browserShowFriendlyFire; // NERVE - SMF
vmCvar_t ui_browserShowMaxlives; // NERVE - SMF
vmCvar_t ui_browserShowPunkBuster; // DHM - Nerve
vmCvar_t ui_browserShowAntilag; // TTimo
vmCvar_t ui_browserShowWeaponsRestricted;
vmCvar_t ui_browserShowTeamBalanced;

vmCvar_t ui_serverStatusTimeOut;

vmCvar_t ui_Q3Model;
vmCvar_t ui_headModel;
vmCvar_t ui_model;

vmCvar_t ui_limboOptions;
vmCvar_t ui_limboPrevOptions;
vmCvar_t ui_limboObjective;

vmCvar_t ui_cmd;

vmCvar_t ui_prevTeam;
vmCvar_t ui_prevClass;
vmCvar_t ui_prevWeapon;

vmCvar_t ui_limboMode;
vmCvar_t ui_objective;

vmCvar_t ui_team;
vmCvar_t ui_class;
vmCvar_t ui_weapon;

vmCvar_t ui_isSpectator;

vmCvar_t ui_friendlyFire;

vmCvar_t ui_userTimeLimit;
vmCvar_t ui_userAlliedRespawnTime;
vmCvar_t ui_userAxisRespawnTime;
vmCvar_t ui_glCustom; // JPW NERVE missing from q3ta

// -NERVE - SMF

vmCvar_t g_gameType;

vmCvar_t cl_profile;
vmCvar_t cl_defaultProfile;
vmCvar_t ui_profile;
vmCvar_t ui_currentNetCampaign;
vmCvar_t ui_currentCampaign;
vmCvar_t ui_campaignIndex;
vmCvar_t ui_currentCampaignCompleted;

// OSP
// cgame mappings
vmCvar_t ui_blackout; // For speclock
vmCvar_t cg_crosshairColor;
vmCvar_t cg_crosshairColorAlt;
vmCvar_t cg_crosshairAlpha;
vmCvar_t cg_crosshairAlphaAlt;
vmCvar_t cg_crosshairSize;

// OSP

vmCvar_t    cl_bypassMouseInput;

//bani
vmCvar_t    ui_autoredirect;

cvarTable_t cvarTable[] =
{
	{ &ui_glCustom,                     "ui_glCustom",                     "4",                          CVAR_ARCHIVE                   }, // JPW NERVE missing from q3ta
	{ &ui_ffa_fraglimit,                "ui_ffa_fraglimit",                "20",                         CVAR_ARCHIVE                   },
	{ &ui_ffa_timelimit,                "ui_ffa_timelimit",                "0",                          CVAR_ARCHIVE                   },

	{ &ui_team_fraglimit,               "ui_team_fraglimit",               "0",                          CVAR_ARCHIVE                   },
	{ &ui_team_timelimit,               "ui_team_timelimit",               "20",                         CVAR_ARCHIVE                   },
	{ &ui_team_friendly,                "ui_team_friendly",                "1",                          CVAR_ARCHIVE                   },

	{ &ui_ctf_capturelimit,             "ui_ctf_capturelimit",             "8",                          CVAR_ARCHIVE                   },
	{ &ui_ctf_timelimit,                "ui_ctf_timelimit",                "30",                         CVAR_ARCHIVE                   },
	{ &ui_ctf_friendly,                 "ui_ctf_friendly",                 "0",                          CVAR_ARCHIVE                   },

	{ &ui_arenasFile,                   "g_arenasFile",                    "",                           CVAR_INIT | CVAR_ROM           },
	{ &ui_botsFile,                     "g_botsFile",                      "",                           CVAR_INIT | CVAR_ROM           },
	{ &ui_spScores1,                    "g_spScores1",                     "",                           CVAR_ARCHIVE | CVAR_ROM        },
	{ &ui_spScores2,                    "g_spScores2",                     "",                           CVAR_ARCHIVE | CVAR_ROM        },
	{ &ui_spScores3,                    "g_spScores3",                     "",                           CVAR_ARCHIVE | CVAR_ROM        },
	{ &ui_spScores4,                    "g_spScores4",                     "",                           CVAR_ARCHIVE | CVAR_ROM        },
	{ &ui_spScores5,                    "g_spScores5",                     "",                           CVAR_ARCHIVE | CVAR_ROM        },
	{ &ui_spAwards,                     "g_spAwards",                      "",                           CVAR_ARCHIVE | CVAR_ROM        },
	{ &ui_spVideos,                     "g_spVideos",                      "",                           CVAR_ARCHIVE | CVAR_ROM        },
	{ &ui_spSkill,                      "g_spSkill",                       "2",                          CVAR_ARCHIVE | CVAR_LATCH      },

	// NERVE - SMF
	{ &ui_friendlyFire,                 "g_friendlyFire",                  "1",                          CVAR_ARCHIVE                   },

	{ &ui_userTimeLimit,                "ui_userTimeLimit",                "0",                          0                              },
	{ &ui_userAlliedRespawnTime,        "ui_userAlliedRespawnTime",        "0",                          0                              },
	{ &ui_userAxisRespawnTime,          "ui_userAxisRespawnTime",          "0",                          0                              },
	// -NERVE - SMF

// JPW NERVE
	{ &ui_teamArenaFirstRun,            "ui_teamArenaFirstRun",            "0",                          CVAR_ARCHIVE                   }, // so sound stuff latches, strange as that seems
// jpw

	{ &ui_spSelection,                  "ui_spSelection",                  "",                           CVAR_ROM                       },
	{ &ui_master,                       "ui_master",                       "0",                          CVAR_ARCHIVE                   },

	{ &ui_brassTime,                    "cg_brassTime",                    "2500",                       CVAR_ARCHIVE                   }, // JPW NERVE
	{ &ui_drawCrosshair,                "cg_drawCrosshair",                "4",                          CVAR_ARCHIVE                   },
	{ &ui_drawCrosshairNames,           "cg_drawCrosshairNames",           "1",                          CVAR_ARCHIVE                   },
	{ &ui_drawCrosshairPickups,         "cg_drawCrosshairPickups",         "1",                          CVAR_ARCHIVE                   }, //----(SA) added
	{ &ui_marks,                        "cg_marktime",                     "20000",                      CVAR_ARCHIVE                   },
	// JOSEPH 12-2-99
	{ &ui_autoactivate,                 "cg_autoactivate",                 "1",                          CVAR_ARCHIVE                   },
	// END JOSEPH

	{ &ui_server1,                      "server1",                         "",                           CVAR_ARCHIVE                   },
	{ &ui_server2,                      "server2",                         "",                           CVAR_ARCHIVE                   },
	{ &ui_server3,                      "server3",                         "",                           CVAR_ARCHIVE                   },
	{ &ui_server4,                      "server4",                         "",                           CVAR_ARCHIVE                   },
	{ &ui_server5,                      "server5",                         "",                           CVAR_ARCHIVE                   },
	{ &ui_server6,                      "server6",                         "",                           CVAR_ARCHIVE                   },
	{ &ui_server7,                      "server7",                         "",                           CVAR_ARCHIVE                   },
	{ &ui_server8,                      "server8",                         "",                           CVAR_ARCHIVE                   },
	{ &ui_server9,                      "server9",                         "",                           CVAR_ARCHIVE                   },
	{ &ui_server10,                     "server10",                        "",                           CVAR_ARCHIVE                   },
	{ &ui_server11,                     "server11",                        "",                           CVAR_ARCHIVE                   },
	{ &ui_server12,                     "server12",                        "",                           CVAR_ARCHIVE                   },
	{ &ui_server13,                     "server13",                        "",                           CVAR_ARCHIVE                   },
	{ &ui_server14,                     "server14",                        "",                           CVAR_ARCHIVE                   },
	{ &ui_server15,                     "server15",                        "",                           CVAR_ARCHIVE                   },
	{ &ui_server16,                     "server16",                        "",                           CVAR_ARCHIVE                   },

	{ &ui_dedicated,                    "ui_dedicated",                    "0",                          CVAR_ARCHIVE                   },
	{ &ui_cdkeychecked,                 "ui_cdkeychecked",                 "0",                          CVAR_ROM                       },
	{ &ui_selectedPlayer,               "cg_selectedPlayer",               "0",                          CVAR_ARCHIVE                   },
	{ &ui_selectedPlayerName,           "cg_selectedPlayerName",           "",                           CVAR_ARCHIVE                   },
	{ &ui_netSource,                    "ui_netSource",                    "1",                          CVAR_ARCHIVE                   },
	{ &ui_menuFiles,                    "ui_menuFiles",                    "ui/menus.txt",               CVAR_ARCHIVE                   },
	{ &ui_gameType,                     "ui_gametype",                     "3",                          CVAR_ARCHIVE                   },
	{ &ui_joinGameType,                 "ui_joinGametype",                 "-1",                         CVAR_ARCHIVE                   },
	{ &ui_netGameType,                  "ui_netGametype",                  "4",                          CVAR_ARCHIVE                   }, // NERVE - SMF - hardwired for now
//  { &ui_actualNetGameType, "ui_actualNetGametype", "5", CVAR_ARCHIVE },       // NERVE - SMF - hardwired for now

	{ &ui_notebookCurrentPage,          "ui_notebookCurrentPage",          "1",                          CVAR_ROM                       },
	{ &ui_clipboardName,                "cg_clipboardName",                "",                           CVAR_ROM                       },

	// NERVE - SMF - multiplayer cvars
	{ &ui_mapIndex,                     "ui_mapIndex",                     "0",                          CVAR_ARCHIVE                   },
	{ &ui_currentMap,                   "ui_currentMap",                   "0",                          CVAR_ARCHIVE                   },
	{ &ui_currentNetMap,                "ui_currentNetMap",                "0",                          CVAR_ARCHIVE                   },

	{ &ui_browserMaster,                "ui_browserMaster",                "0",                          CVAR_ARCHIVE                   },
	{ &ui_browserGameType,              "ui_browserGameType",              "0",                          CVAR_ARCHIVE                   },
	{ &ui_browserSortKey,               "ui_browserSortKey",               "4",                          CVAR_ARCHIVE                   },
	{ &ui_browserShowEmptyOrFull,       "ui_browserShowEmptyOrFull",       "0",                          CVAR_ARCHIVE                   },
	{ &ui_browserShowPasswordProtected, "ui_browserShowPasswordProtected", "0",                          CVAR_ARCHIVE                   },
	{ &ui_browserShowFriendlyFire,      "ui_browserShowFriendlyFire",      "0",                          CVAR_ARCHIVE                   },
	{ &ui_browserShowMaxlives,          "ui_browserShowMaxlives",          "0",                          CVAR_ARCHIVE                   },
	{ &ui_browserShowPunkBuster,        "ui_browserShowPunkBuster",        "0",                          CVAR_ARCHIVE                   },
	{ &ui_browserShowAntilag,           "ui_browserShowAntilag",           "0",                          CVAR_ARCHIVE                   },
	{ &ui_browserShowWeaponsRestricted, "ui_browserShowWeaponsRestricted", "0",                          CVAR_ARCHIVE                   },
	{ &ui_browserShowTeamBalanced,      "ui_browserShowTeamBalanced",      "0",                          CVAR_ARCHIVE                   },

	{ &ui_serverStatusTimeOut,          "ui_serverStatusTimeOut",          "7000",                       CVAR_ARCHIVE                   },

	{ &ui_Q3Model,                      "ui_Q3Model",                      "1",                          0                              },
	{ &ui_headModel,                    "headModel",                       "",                           0                              },

	{ &ui_limboOptions,                 "ui_limboOptions",                 "0",                          0                              },
	{ &ui_limboPrevOptions,             "ui_limboPrevOptions",             "0",                          0                              },
	{ &ui_limboObjective,               "ui_limboObjective",               "0",                          0                              },
	{ &ui_cmd,                          "ui_cmd",                          "",                           0                              },

	{ &ui_prevTeam,                     "ui_prevTeam",                     "-1",                         0                              },
	{ &ui_prevClass,                    "ui_prevClass",                    "-1",                         0                              },
	{ &ui_prevWeapon,                   "ui_prevWeapon",                   "-1",                         0                              },

	{ &ui_limboMode,                    "ui_limboMode",                    "0",                          0                              },
	{ &ui_objective,                    "ui_objective",                    "",                           0                              },

	{ &ui_team,                         "ui_team",                         "Axis",                       0                              },
	{ &ui_class,                        "ui_class",                        "Soldier",                    0                              },
	{ &ui_weapon,                       "ui_weapon",                       "MP 40",                      0                              },

	{ &ui_isSpectator,                  "ui_isSpectator",                  "1",                          0                              },
	// -NERVE - SMF

	{ &g_gameType,                      "g_gameType",                      "4",                          CVAR_SERVERINFO | CVAR_LATCH   },
	{ NULL,                             "cg_drawBuddies",                  "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_drawRoundTimer",               "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_showblood",                    "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_bloodFlash",                   "1.0",                        CVAR_ARCHIVE                   },
	{ NULL,                             "cg_autoReload",                   "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_noAmmoAutoSwitch",             "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_useWeapsForZoom",              "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_zoomDefaultSniper",            "20",                         CVAR_ARCHIVE                   },
	{ NULL,                             "cg_zoomstepsniper",               "2",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_voicespritetime",              "6000",                       CVAR_ARCHIVE                   },
	{ NULL,                             "cg_complaintPopUp",               "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_announcer",                    "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_printObjectiveInfo",           "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_useScreenshotJPEG",            "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_drawGun",                      "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_drawCompass",                  "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_drawRoundTimer",               "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_drawReinforcementTime",        "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_cursorHints",                  "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_crosshairPulse",               "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_drawCrosshairNames",           "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "cg_crosshairColor",               "White",                      CVAR_ARCHIVE                   },
	{ NULL,                             "cg_crosshairAlpha",               "1.0",                        CVAR_ARCHIVE                   },
	{ NULL,                             "cg_crosshairColorAlt",            "White",                      CVAR_ARCHIVE                   },
	{ NULL,                             "cg_coronafardist",                "1536",                       CVAR_ARCHIVE                   },
	{ NULL,                             "cg_wolfparticles",                "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "g_password",                      "none",                       CVAR_USERINFO                  },
	{ NULL,                             "g_antilag",                       "1",                          CVAR_SERVERINFO | CVAR_ARCHIVE },
	{ NULL,                             "g_warmup",                        "60",                         CVAR_ARCHIVE                   },
	{ NULL,                             "g_lms_roundlimit",                "3",                          CVAR_ARCHIVE                   },
	{ NULL,                             "g_lms_matchlimit",                "2",                          CVAR_ARCHIVE                   },
	{ NULL,                             "g_lms_followTeamOnly",            "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "g_heavyWeaponRestriction",        "100",                        CVAR_ARCHIVE | CVAR_SERVERINFO },
	{ &cl_profile,                      "cl_profile",                      "",                           CVAR_ROM                       },
	{ &cl_defaultProfile,               "cl_defaultProfile",               "",                           CVAR_ROM                       },
	{ &ui_profile,                      "ui_profile",                      "",                           CVAR_ROM                       },
	{ &ui_currentCampaign,              "ui_currentCampaign",              "0",                          CVAR_ARCHIVE                   },
	{ &ui_currentNetCampaign,           "ui_currentNetCampaign",           "0",                          CVAR_ARCHIVE                   },
	{ &ui_campaignIndex,                "ui_campaignIndex",                "0",                          CVAR_ARCHIVE                   },
	{ &ui_currentCampaignCompleted,     "ui_currentCampaignCompleted",     "0",                          CVAR_ARCHIVE                   },

	// START - TAT 9/16/2002
	// cvar used to implement context sensitive bot menu
	//      if this is set to "engineer", for instance, then only engineer commands will show up
	{ NULL,                             "cg_botMenuType",                  "0",                          CVAR_TEMP                      },
	// END - TAT 9/15/2002

	// OSP
	// cgame mappings
	{ &ui_blackout,                     "ui_blackout",                     "0",                          CVAR_ROM                       },
	{ &cg_crosshairAlpha,               "cg_crosshairAlpha",               "1.0",                        CVAR_ARCHIVE                   },
	{ &cg_crosshairAlphaAlt,            "cg_crosshairAlphaAlt",            "1.0",                        CVAR_ARCHIVE                   },
	{ &cg_crosshairColor,               "cg_crosshairColor",               "White",                      CVAR_ARCHIVE                   },
	{ &cg_crosshairColorAlt,            "cg_crosshairColorAlt",            "White",                      CVAR_ARCHIVE                   },
	{ &cg_crosshairSize,                "cg_crosshairSize",                "48",                         CVAR_ARCHIVE                   },
	// game mappings (for create server option)
	{ NULL,                             "bot_enable",                      "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "bot_minplayers",                  "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "g_altStopwatchMode",              "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "g_ipcomplaintlimit",              "3",                          CVAR_ARCHIVE                   },
	{ NULL,                             "g_complaintlimit",                "6",                          CVAR_ARCHIVE                   },
	{ NULL,                             "g_doWarmup",                      "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "g_inactivity",                    "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "g_maxLives",                      "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "refereePassword",                 "none",                       CVAR_ARCHIVE                   },
	{ NULL,                             "g_teamForceBalance",              "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "sv_maxRate",                      "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "g_spectatorInactivity",           "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "match_latejoin",                  "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "match_minplayers",                MATCH_MINPLAYERS,             CVAR_ARCHIVE                   },
	{ NULL,                             "match_mutespecs",                 "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "match_readypercent",              "100",                        CVAR_ARCHIVE                   },
	{ NULL,                             "match_timeoutcount",              "3",                          CVAR_ARCHIVE                   },
	{ NULL,                             "match_timeoutlength",             "180",                        CVAR_ARCHIVE                   },
	{ NULL,                             "match_warmupDamage",              "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "server_autoconfig",               "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "server_motd0",                    " ^NEnemy Territory ^7MOTD ", CVAR_ARCHIVE                   },
	{ NULL,                             "server_motd1",                    "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "server_motd2",                    "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "server_motd3",                    "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "server_motd4",                    "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "server_motd5",                    "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "team_maxPanzers",                 "-1",                         CVAR_ARCHIVE                   },
	{ NULL,                             "team_maxplayers",                 "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "team_nocontrols",                 "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_comp",                 "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_gametype",             "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_kick",                 "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_map",                  "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_mutespecs",            "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_nextmap",              "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_pub",                  "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_referee",              "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_shuffleteamsxp",       "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_swapteams",            "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_friendlyfire",         "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_timelimit",            "0",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_warmupdamage",         "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_antilag",              "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_muting",               "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_allow_kick",                 "1",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_limit",                      "5",                          CVAR_ARCHIVE                   },
	{ NULL,                             "vote_percent",                    "50",                         CVAR_ARCHIVE                   },
	// OSP

	//{ NULL, "ui_creatingprofile", "", CVAR_ARCHIVE },
	{ NULL,                             "ui_r_mode",                       "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "ui_r_gamma",                      "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "ui_rate",                         "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "ui_handedness",                   "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "ui_sensitivity",                  "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "ui_profile_mousePitch",           "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "ui_cg_shadows",                   "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "ui_r_hdrrendering",               "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "ui_r_bloom",                      "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "ui_r_normalmapping",              "",                           CVAR_ARCHIVE                   },
	{ NULL,                             "ui_r_parallaxmapping",            "",                           CVAR_ARCHIVE                   },

	{ &cl_bypassMouseInput,             "cl_bypassMouseInput",             "0",                          CVAR_TEMP                      },

	{ NULL,                             "g_oldCampaign",                   "",                           CVAR_ROM,                      },
	{ NULL,                             "g_currentCampaign",               "",                           CVAR_WOLFINFO | CVAR_ROM,      },
	{ NULL,                             "g_currentCampaignMap",            "0",                          CVAR_WOLFINFO | CVAR_ROM,      },

	{ NULL,                             "ui_showtooltips",                 "1",                          CVAR_ARCHIVE                   },

	//bani
	{ &ui_autoredirect,                 "ui_autoredirect",                 "0",                          CVAR_ARCHIVE                   },
};

int         cvarTableSize = sizeof( cvarTable ) / sizeof( cvarTable[ 0 ] );

/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars( void )
{
	int         i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
	{
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );

		if ( cv->vmCvar != NULL )
		{
			cv->modificationCount = cv->vmCvar->modificationCount;
		}
	}

	// OSP - Always force this to 0 on init
	trap_Cvar_Set( "ui_blackout", "0" );
	BG_setCrosshair( cg_crosshairColor.string, uiInfo.xhairColor, cg_crosshairAlpha.value, "cg_crosshairColor" );
	BG_setCrosshair( cg_crosshairColorAlt.string, uiInfo.xhairColorAlt, cg_crosshairAlphaAlt.value, "cg_crosshairColorAlt" );
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars( void )
{
	int         i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
	{
		if ( cv->vmCvar )
		{
			trap_Cvar_Update( cv->vmCvar );

			if ( cv->modificationCount != cv->vmCvar->modificationCount )
			{
				cv->modificationCount = cv->vmCvar->modificationCount;

				// OSP
				if ( cv->vmCvar == &cg_crosshairColor || cv->vmCvar == &cg_crosshairAlpha )
				{
					BG_setCrosshair( cg_crosshairColor.string, uiInfo.xhairColor, cg_crosshairAlpha.value, "cg_crosshairColor" );
				}

				if ( cv->vmCvar == &cg_crosshairColorAlt || cv->vmCvar == &cg_crosshairAlphaAlt )
				{
					BG_setCrosshair( cg_crosshairColorAlt.string, uiInfo.xhairColorAlt, cg_crosshairAlphaAlt.value,
					                 "cg_crosshairColorAlt" );
				}
			}
		}
	}
}

// NERVE - SMF

/*
=================
ArenaServers_StopRefresh
=================
*/
static void UI_StopServerRefresh( void )
{
	int count;

	if ( !uiInfo.serverStatus.refreshActive )
	{
		// not currently refreshing
		return;
	}

	uiInfo.serverStatus.refreshActive = qfalse;
	Com_Printf( "%d servers listed in browser with %d players.\n",
	            uiInfo.serverStatus.numDisplayServers, uiInfo.serverStatus.numPlayersOnServers );
	count = trap_LAN_GetServerCount( ui_netSource.integer );

	if ( count - uiInfo.serverStatus.numDisplayServers > 0 )
	{
		// TTimo - used to be about cl_maxping filtering, that was Q3 legacy, RTCW browser has much more filtering options
		Com_Printf( "%d servers not listed (filtered out by game browser settings)\n",
		            count - uiInfo.serverStatus.numDisplayServers );
	}
}

/*
=================
UI_DoServerRefresh
=================
*/
static void UI_DoServerRefresh( void )
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
			if ( !trap_LAN_GetServerCount( ui_netSource.integer ) )
			{
				wait = qtrue;
			}
		}
		else
		{
			if ( trap_LAN_GetServerCount( ui_netSource.integer ) < 0 )
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
	if ( trap_LAN_UpdateVisiblePings( ui_netSource.integer ) )
	{
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
	}
	else if ( !wait )
	{
		// get the last servers in the list
		UI_BuildServerDisplayList( 2 );
		// stop the refresh
		UI_StopServerRefresh();
	}

	//
	UI_BuildServerDisplayList( qfalse );
}

/*
=================
UI_StartServerRefresh
=================
*/
static void UI_StartServerRefresh( qboolean full )
{
	char    *ptr;
	char    buff[ 64 ];

	qtime_t q;

	trap_RealTime( &q );
	Com_sprintf( buff, sizeof( buff ), "%04i-%02i-%02i %02i:%02i:%02i",
	             1900 + q.tm_year, q.tm_mon + 1, q.tm_mday, q.tm_hour, q.tm_min, q.tm_sec );
	trap_Cvar_Set( va( "ui_lastServerRefresh_%i", ui_netSource.integer ), buff );

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
	trap_LAN_MarkServerVisible( ui_netSource.integer, -1, qtrue );
	// reset all the pings
	trap_LAN_ResetPings( ui_netSource.integer );

	//
	if ( ui_netSource.integer == AS_LOCAL )
	{
		trap_Cmd_ExecuteText( EXEC_APPEND, "localservers\n" );
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
		return;
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 5000;

	if ( ui_netSource.integer == AS_GLOBAL )
	{
		ptr = UI_Cvar_VariableString( "debug_protocol" );

		if ( *ptr )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "globalservers %d %s\n", 0, ptr ) );
		}
		else
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "globalservers %d %d\n", 0, ( int ) trap_Cvar_VariableValue( "protocol" ) ) );
		}
	}
}

// -NERVE - SMF

void UI_Campaign_f( void )
{
	char           str[ MAX_TOKEN_CHARS ];
	int            i;
	campaignInfo_t *campaign = NULL;

	UI_LoadArenas();
	UI_MapCountByGameType( qfalse );
	UI_LoadCampaigns();

	// find the campaign
	trap_Argv( 1, str, sizeof( str ) );

	for ( i = 0; i < uiInfo.campaignCount; i++ )
	{
		campaign = &uiInfo.campaignList[ i ];

		if ( !Q_stricmp( campaign->campaignShortName, str ) )
		{
			break;
		}
	}

	if ( i == uiInfo.campaignCount || !( campaign->typeBits & ( 1 << GT_WOLF ) ) )
	{
		Com_Printf( "Can't find campaign '%s'\n", str );
		return;
	}

	if ( !campaign->mapInfos[ 0 ] )
	{
		Com_Printf( "Corrupted campaign '%s'\n", str );
		return;
	}

	trap_Cvar_Set( "g_oldCampaign", "" );
	trap_Cvar_Set( "g_currentCampaign", campaign->campaignShortName );
	trap_Cvar_Set( "g_currentCampaignMap", "0" );

	// we got a campaign, start it
	trap_Cvar_Set( "g_gametype", va( "%i", GT_WOLF_CAMPAIGN ) );
#if 0

	if ( trap_Cvar_VariableValue( "developer" ) )
	{
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "devmap %s\n", campaign->mapInfos[ 0 ]->mapLoadName ) );
	}
	else
#endif
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "map %s\n", campaign->mapInfos[ 0 ]->mapLoadName ) );
}

void UI_ListCampaigns_f( void )
{
	int i, mpCampaigns;

	UI_LoadArenas();
	UI_MapCountByGameType( qfalse );
	UI_LoadCampaigns();

	mpCampaigns = 0;

	for ( i = 0; i < uiInfo.campaignCount; i++ )
	{
		if ( uiInfo.campaignList[ i ].typeBits & ( 1 << GT_WOLF ) )
		{
			mpCampaigns++;
		}
	}

	if ( mpCampaigns )
	{
		Com_Printf( "%i campaigns found:\n", mpCampaigns );
	}
	else
	{
		Com_Printf( "No campaigns found.\n" );
		return;
	}

	for ( i = 0; i < uiInfo.campaignCount; i++ )
	{
		if ( uiInfo.campaignList[ i ].typeBits & ( 1 << GT_WOLF ) )
		{
			Com_Printf( " %s\n", uiInfo.campaignList[ i ].campaignShortName );
		}
	}
}
