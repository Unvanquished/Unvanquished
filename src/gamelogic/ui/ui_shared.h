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

#ifndef __UI_SHARED_H
#define __UI_SHARED_H

#include "../../engine/qcommon/q_shared.h"
#include "../../engine/renderer/tr_types.h"
#include "../../engine/client/keycodes.h"

#include "../../../main/ui/menudef.h"

#define MAX_MENUDEFFILE            4096
#define MAX_MENUFILE               32768
#define MAX_MENUS                  256
#define MAX_MENUITEMS              256
#define MAX_COLOR_RANGES           10
#define MAX_OPEN_MENUS             16

#define WINDOW_MOUSEOVER           0x00000001 // mouse is over it, non exclusive
#define WINDOW_HASFOCUS            0x00000002 // has cursor focus, exclusive
#define WINDOW_VISIBLE             0x00000004 // is visible
#define WINDOW_GREY                0x00000008 // is visible but grey ( non-active )
#define WINDOW_DECORATION          0x00000010 // for decoration only, no mouse, keyboard, etc..
#define WINDOW_FADINGOUT           0x00000020 // fading out, non-active
#define WINDOW_FADINGIN            0x00000040 // fading in
#define WINDOW_MOUSEOVERTEXT       0x00000080 // mouse is over it, non exclusive
#define WINDOW_INTRANSITION        0x00000100 // window is in transition
#define WINDOW_FORECOLORSET        0x00000200 // forecolor was explicitly set ( used to color alpha images or not )
#define WINDOW_HORIZONTAL          0x00000400 // for list boxes and sliders, vertical is default this is set of horizontal
#define WINDOW_LB_UPARROW          0x00000800 // mouse is over up arrow
#define WINDOW_LB_DOWNARROW        0x00001000 // mouse is over down arrow
#define WINDOW_LB_THUMB            0x00002000 // mouse is over thumb
#define WINDOW_LB_PGUP             0x00004000 // mouse is over page up
#define WINDOW_LB_PGDN             0x00008000 // mouse is over page down
#define WINDOW_ORBITING            0x00010000 // item is in orbit
#define WINDOW_OOB_CLICK           0x00020000 // close on out of bounds click
#define WINDOW_WRAPPED             0x00080000 // wrap text
#define WINDOW_FORCED              0x00100000 // forced open
#define WINDOW_POPUP               0x00200000 // popup
#define WINDOW_BACKCOLORSET        0x00400000 // backcolor was explicitly set
#define WINDOW_TIMEDVISIBLE        0x00800000 // visibility timing ( NOT implemented )
#define WINDOW_DRAG                0x01000000 // KTW: Added for draggable windows
#define WINDOW_NOSTRETCH           0x02000000 // Keep the window's aspect ratio constant

// CGAME cursor type bits
#define CURSOR_NONE                0x00000001
#define CURSOR_ARROW               0x00000002
#define CURSOR_SIZER               0x00000004

#ifdef BUILD_CGAME
#define STRING_POOL_SIZE           128 * 1024
#else
#define STRING_POOL_SIZE           384 * 1024
#endif

#define MAX_EDITFIELD              256
#define ITEM_VALUE_OFFSET          8

#define ASSET_GRADIENTBAR          "ui/assets/gradientbar2.tga"
#define ASSET_SCROLLBAR            "ui/assets/scrollbar.tga"
#define ASSET_SCROLLBAR_ARROWDOWN  "ui/assets/scrollbar_arrow_dwn_a.tga"
#define ASSET_SCROLLBAR_ARROWUP    "ui/assets/scrollbar_arrow_up_a.tga"
#define ASSET_SCROLLBAR_ARROWLEFT  "ui/assets/scrollbar_arrow_left.tga"
#define ASSET_SCROLLBAR_ARROWRIGHT "ui/assets/scrollbar_arrow_right.tga"
#define ASSET_SCROLL_THUMB         "ui/assets/scrollbar_thumb.tga"
#define ASSET_SLIDER_BAR           "ui/assets/slider2.tga"
#define ASSET_SLIDER_THUMB         "ui/assets/sliderbutt_1.tga"

#define SCROLLBAR_ARROW_SIZE       16.0f
#define SCROLLBAR_ARROW_WIDTH      ( SCROLLBAR_ARROW_SIZE * DC->aspectScale )
#define SCROLLBAR_ARROW_HEIGHT     SCROLLBAR_ARROW_SIZE
#define SCROLLBAR_SLIDER_X(_item)      ( _item->window.rect.x + _item->window.rect.w - \
                                         SCROLLBAR_ARROW_WIDTH - DC->aspectScale )
#define SCROLLBAR_SLIDER_Y(_item)      ( SCROLLBAR_Y(_item) + SCROLLBAR_ARROW_HEIGHT )
#define SCROLLBAR_SLIDER_HEIGHT(_item) ( _item->window.rect.h - ( SCROLLBAR_ARROW_HEIGHT * 2.0f ) - 2.0f )
#define SCROLLBAR_X(_item)             ( _item->window.rect.x + DC->aspectScale )
#define SCROLLBAR_Y(_item)             ( _item->window.rect.y + 1.0f )
#define SCROLLBAR_W(_item)             ( SCROLLBAR_SLIDER_X(_item) - SCROLLBAR_X(_item))
#define SCROLLBAR_H(_item)             ( _item->window.rect.h - 2.0f )

#define SLIDER_WIDTH        ( 96.0f * DC->aspectScale )
#define SLIDER_HEIGHT       16.0f
#define SLIDER_THUMB_WIDTH  ( 12.0f * DC->aspectScale )
#define SLIDER_THUMB_HEIGHT 20.0f

#define _( text )              Gettext( text )
#define N_( text )             text
#define C_( ctxt, text )       Pgettext( ctxt, text )
#define G_( text )             Pgettext( Trans_GenderContext( gender ), text )
#define P_( one, many, count ) GettextPlural( (one), (many), (count) )

typedef struct
{
	float x; // horiz position
	float y; // vert position
	float w; // width
	float h; // height;
}

rectDef_t;

typedef rectDef_t Rectangle;

// FIXME: do something to separate text vs window stuff

typedef struct
{
	Rectangle  rect; // client coord rectangle
	int        aspectBias; // direction in which to aspect compensate
	Rectangle  rectClient; // screen coord rectangle
	const char *name; //
	const char *group; // if it belongs to a group
	const char *cinematicName; // cinematic name
	int        cinematic; // cinematic handle
	int        style; //
	int        border; //
	int        ownerDraw; // ownerDraw style
	int        ownerDrawFlags; // show flags for ownerdraw items
	float      borderSize; //
	int        flags; // visible, focus, mouseover, cursor
	Rectangle  rectEffects; // for various effects
	Rectangle  rectEffects2; // for various effects
	int        offsetTime; // time based value for various effects
	int        nextTime; // time next effect should cycle
	vec4_t     foreColor; // text color
	vec4_t     backColor; // border color
	vec4_t     borderColor; // border color
	vec4_t     outlineColor; // border color
	qhandle_t  background; // background asset
}

windowDef_t;

typedef windowDef_t Window;

typedef struct
{
	vec4_t color;
	float  low;
	float  high;
}

colorRangeDef_t;

// FIXME: combine flags into bitfields to save space
// FIXME: consolidate all of the common stuff in one structure for menus and items
// THINKABOUTME: is there any compelling reason not to have items contain items
// and do away with a menu per say.. major issue is not being able to dynamically allocate
// and destroy stuff.. Another point to consider is adding an alloc free call for VMs and have
// the engine just allocate the pool for it based on a cvar
// many of the vars are re-used for different item types, as such they are not always named appropriately
// the benefits of c++ in DOOM will greatly help crap like this
// FIXME: need to put a type ptr that points to specific type info per type
//
#define MAX_LB_COLUMNS 16

typedef struct columnInfo_s
{
	int pos;
	int width;
	int align;
}

columnInfo_t;

typedef struct listBoxDef_s
{
	int          startPos;
	int          endPos;
	int          cursorPos;

	float        elementWidth;
	float        elementHeight;
	int          elementStyle;
	int          dropItems;

	int          numColumns;
	columnInfo_t columnInfo[ MAX_LB_COLUMNS ];

	const char   *doubleClick;

	qboolean     notselectable;
	qboolean     noscrollbar;

	qboolean     resetonfeederchange;
	int          lastFeederCount;
}

listBoxDef_t;

typedef struct cycleDef_s
{
	int cursorPos;
}

cycleDef_t;

typedef struct editFieldDef_s
{
	float minVal; //  edit field limits
	float maxVal; //
	float defVal; //
	float range; //
	int   maxChars; // for edit fields
	int   maxPaintChars; // for edit fields
	int   maxFieldWidth; // for edit fields
	int   paintOffset; //
}

editFieldDef_t;

#define MAX_MULTI_CVARS 32

typedef struct multiDef_s
{
	const char *cvarList[ MAX_MULTI_CVARS ];
	const char *cvarStr[ MAX_MULTI_CVARS ];
	float      cvarValue[ MAX_MULTI_CVARS ];
	int        count;
	qboolean   strDef;
}

multiDef_t;

typedef struct modelDef_s
{
	int    angle;
	vec3_t origin;
	float  fov_x;
	float  fov_y;
	int    rotationSpeed;
}

modelDef_t;

#define CVAR_ENABLE  0x00000001
#define CVAR_DISABLE 0x00000002
#define CVAR_SHOW    0x00000004
#define CVAR_HIDE    0x00000008

typedef enum
{
  TYPE_ANY = -1,
  TYPE_NONE,
  TYPE_LIST,
  TYPE_EDIT,
  TYPE_MULTI,
  TYPE_COMBO,
  TYPE_MODEL
} itemDataType_t;

typedef struct menuDef_s menuDef_t;

typedef struct itemDef_s
{
	Window          window; // common positional, border, style, layout info
	Rectangle       textRect; // rectangle the text ( if any ) consumes
	int             type; // text, button, radiobutton, checkbox, textfield, listbox, combo
	int             alignment; // left center right
	int             textalignment; // ( optional ) alignment for text within rect based on text width
	int             textvalignment; // ( optional ) alignment for text within rect based on text width
	float           textalignx; // ( optional ) text alignment x coord
	float           textaligny; // ( optional ) text alignment x coord
	float           textscale; // scale percentage from 72pts
	int             textStyle; // ( optional ) style, plain and shadowed are it for now
	const char      *text; // display text
	menuDef_t       *parent; // menu owner
	qhandle_t       asset; // handle to asset
	const char      *mouseEnterText; // mouse enter script
	const char      *mouseExitText; // mouse exit script
	const char      *mouseEnter; // mouse enter script
	const char      *mouseExit; // mouse exit script
	const char      *action; // select script
	const char      *onFocus; // select script
	const char      *leaveFocus; // select script
	const char      *onTextEntry; // called when text entered
	const char      *onCharEntry; // called when text entered
	const char      *cvar; // associated cvar
	const char      *cvarTest; // associated cvar for enable actions
	const char      *enableCvar; // enable, disable, show, or hide based on value, this can contain a list
	int             cvarFlags; //  what type of action to take on cvarenables
	sfxHandle_t     focusSound;
	int             numColors; // number of color ranges
	colorRangeDef_t colorRanges[ MAX_COLOR_RANGES ];
	int             feederID; // where to get data for this item
	int             cursorPos; // cursor position in characters
	union
	{
		void           *data;
		listBoxDef_t   *list;
		editFieldDef_t *edit;
		multiDef_t     *multi;
		cycleDef_t     *cycle;
		modelDef_t     *model;
	} typeData; // type specific data pointers
}

itemDef_t;

struct menuDef_s
{
	Window     window;
	const char *font; // font
	qboolean   fullScreen; // covers entire screen
	int        itemCount; // number of items;
	int        fontIndex; //
	int        cursorItem; // which item as the cursor
	int        fadeCycle; //
	float      fadeClamp; //
	float      fadeAmount; //
	const char *onOpen; // run when the menu is first opened
	const char *onClose; // run when the menu is closed
	const char *onESC; // run when the menu is closed
	const char *onKEY[ MAX_KEYS ]; // run when key is pressed
	const char *soundName; // background loop sound for menu
	vec4_t     focusColor; // focus color for items
	vec4_t     disableColor; // focus color for items
	itemDef_t  *items[ MAX_MENUITEMS ]; // items this menu contains
};

typedef struct
{
	const char  *fontStr;
	const char  *cursorStr;
	const char  *gradientStr;
	fontMetrics_t textFont, smallFont, bigFont;
	qhandle_t   cursor;
	qhandle_t   gradientBar;
	qhandle_t   scrollBarArrowUp;
	qhandle_t   scrollBarArrowDown;
	qhandle_t   scrollBarArrowLeft;
	qhandle_t   scrollBarArrowRight;
	qhandle_t   scrollBar;
	qhandle_t   scrollBarThumb;
	qhandle_t   buttonMiddle;
	qhandle_t   buttonInside;
	qhandle_t   solidBox;
	qhandle_t   sliderBar;
	qhandle_t   sliderThumb;
	sfxHandle_t menuEnterSound;
	sfxHandle_t menuExitSound;
	sfxHandle_t menuBuzzSound;
	sfxHandle_t itemFocusSound;
	float       fadeClamp;
	int         fadeCycle;
	float       fadeAmount;
	float       shadowX;
	float       shadowY;
	vec4_t      shadowColor;
	float       shadowFadeClamp;
	qboolean    fontRegistered;
	qboolean    dynFontRegistered;
	emoticon_t  emoticons[ MAX_EMOTICONS ];
	int         emoticonCount;
}

cachedAssets_t;

typedef struct
{
	const char *name;
	void ( *handler )( itemDef_t *item, char **args );
}

commandDef_t;

typedef struct
{
	qhandle_t ( *registerShader )( const char *p, RegisterShaderFlags_t flags );
	void ( *setColor )( const vec4_t v );
	void ( *drawHandlePic )( float x, float y, float w, float h, qhandle_t asset );
	void ( *drawNoStretchPic ) ( float x, float y, float w, float h, qhandle_t asset );
	void ( *drawStretchPic )( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
	qhandle_t ( *registerModel )( const char *p );
	void ( *modelBounds )( qhandle_t model, vec3_t min, vec3_t max );
	void ( *fillRect )( float x, float y, float w, float h, const vec4_t color );
	void ( *drawRect )( float x, float y, float w, float h, float size, const vec4_t color );
	void ( *drawSides )( float x, float y, float w, float h, float size );
	void ( *drawTopBottom )( float x, float y, float w, float h, float size );
	void ( *clearScene )( void );
	void ( *addRefEntityToScene )( const refEntity_t *re );
	void ( *renderScene )( const refdef_t *fd );
	void ( *registerFont )( const char *pFontname, const char *pFallback, int pointSize, fontMetrics_t *out );
	void ( *glyph )( fontHandle_t, const char *str, glyphInfo_t *glyph );
	void ( *glyphChar )( fontHandle_t, int ch, glyphInfo_t *glyph );
	void ( *freeCachedGlyphs )( fontHandle_t );

	void ( *ownerDrawItem )( rectDef_t *rect, float text_x,
	                         float text_y, int ownerDraw, int ownerDrawFlags,
	                         int align, int textalign, int textvalign,
	                         float borderSize, float scale, vec4_t foreColor,
	                         vec4_t backColor, qhandle_t shader, int textStyle );
	float( *getValue )( int ownerDraw );
	qboolean( *ownerDrawVisible )( int flags );
	void ( *runScript )( char **p );
	void ( *getCVarString )( const char *cvar, char *buffer, int bufsize );
	void ( *getCVarLatchedString )( const char *cvar, char *buffer, int bufsize );
	float( *getCVarValue )( const char *cvar );
	void ( *setCVar )( const char *cvar, const char *value );
	void ( *drawTextWithCursor )( float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int clipped, int style );
	void ( *setOverstrikeMode )( qboolean b );
	qboolean( *getOverstrikeMode )( void );
	void ( *startLocalSound )( sfxHandle_t sfx, int channelNum );
	qboolean( *ownerDrawHandleKey )( int ownerDraw, int key );
	int ( *feederCount )( int feederID );
	const char *( *feederItemText )( int feederID, int index, int column, qhandle_t *handle );
	qhandle_t ( *feederItemImage )( int feederID, int index );
	void ( *feederSelection )( int feederID, int index );
	int ( *feederInitialise )( int feederID );
	void ( *keynumToStringBuf )( int keynum, char *buf, int buflen );
	void ( *getBindingBuf )( int keynum, int team, char *buf, int buflen );
	void ( *setBinding )( int keynum, int team, const char *binding );
	void ( *executeText )( int exec_when, const char *text );
	void ( *Error )( int level, const char *error, ... ) PRINTF_LIKE(2);
	void ( *Print )( const char *msg, ... ) PRINTF_LIKE(1);
	void ( *Pause )( qboolean b );
	int ( *ownerDrawWidth )( int ownerDraw, float scale );
	const char *( *ownerDrawText )( int ownerDraw );
	sfxHandle_t ( *registerSound )( const char *name, qboolean compressed );
	void ( *startBackgroundTrack )( const char *intro, const char *loop );
	void ( *stopBackgroundTrack )( void );
	int ( *playCinematic )( const char *name, float x, float y, float w, float h );
	void ( *stopCinematic )( int handle );
	void ( *drawCinematic )( int handle, float x, float y, float w, float h );
	void ( *runCinematicFrame )( int handle );

	float          yscale;
	float          xscale;
	float          aspectScale;
	int            realTime;
	int            frameTime;
	int            unscaledCursorx;
	int            unscaledCursory;
	float          cursorx;
	float          cursory;
	float          cursordx;
	float          cursordy;
	float          smallFontScale;
	float          bigFontScale;
	qboolean       debug;

	cachedAssets_t Assets;

	glconfig_t     glconfig;
	qhandle_t      whiteShader;
	qhandle_t      gradientImage;
	qhandle_t      cursor;
	float          FPS;
}

displayContextDef_t;

const char          *String_Alloc( const char *p );
void                String_Init( void );
void                String_Report( void );
void                Init_Display( displayContextDef_t *dc );
void                Menu_Init( menuDef_t *menu );
void                Item_Init( itemDef_t *item );
void                Menu_PostParse( menuDef_t *menu );
menuDef_t           *Menu_GetFocused( void );
void                Menu_HandleKey( menuDef_t *menu, int key, int chr, qboolean down );
void                Menu_HandleMouseMove( menuDef_t *menu, float x, float y );
void                Menu_ScrollFeeder( menuDef_t *menu, int feeder, qboolean down );
qboolean            Float_Parse( char **p, float *f );
qboolean            Color_Parse( char **p, vec4_t *c );
qboolean            Int_Parse( char **p, int *i );
qboolean            Rect_Parse( char **p, rectDef_t *r );
qboolean            String_Parse( char **p, const char **out );
qboolean            Script_Parse( char **p, const char **out );
qboolean            PC_Float_Parse( int handle, float *f );
qboolean            PC_Color_Parse( int handle, vec4_t *c );
qboolean            PC_Int_Parse( int handle, int *i );
qboolean            PC_Rect_Parse( int handle, rectDef_t *r );
qboolean            PC_String_Parse( int handle, const char **out );
qboolean            PC_Script_Parse( int handle, const char **out );
int                 Menu_Count( void );
void                Menu_New( int handle );
void                Menu_UpdateAll( void );
void                Menu_PaintAll( void );
menuDef_t           *Menus_ActivateByName( const char *p );
menuDef_t           *Menus_ReplaceActiveByName( const char *p );
void                Menu_Reset( void );
qboolean            Menus_AnyFullScreenVisible( void );
void                Menus_Activate( menuDef_t *menu );
qboolean            Menus_ReplaceActive( menuDef_t *menu );

displayContextDef_t *Display_GetContext( void );
void                *Display_CaptureItem( int x, int y );
qboolean            Display_MouseMove( void *p, float x, float y );
int                 Display_CursorType( int x, int y );
qboolean            Display_KeyBindPending( void );
menuDef_t           *Menus_FindByName( const char *p );
void                Menus_CloseByName( const char *p );
void                Display_HandleKey( int key, int chr, qboolean down, int x, int y );
void                LerpColor( vec4_t a, vec4_t b, vec4_t c, float t );
void                Menus_CloseAll( void );
void                Menu_Update( menuDef_t *menu );
void                Menu_Paint( menuDef_t *menu, qboolean forcePaint );
void                Menu_SetFeederSelection( menuDef_t *menu, int feeder, int index, const char *name );
void                Display_CacheAll( void );
menuDef_t           *Menu_Get( int handle );

typedef void ( CaptureFunc )( void *p );

void        UI_InstallCaptureFunc( CaptureFunc *f, void *data, int timeout );
void        UI_RemoveCaptureFunc( void );

void        *UI_Alloc( int size );
void        UI_InitMemory( void );
qboolean    UI_OutOfMemory( void );
void        UIS_Shutdown( void );

void        Controls_GetConfig( void );
void        Controls_SetConfig( qboolean restart );
void        Controls_SetDefaults( void );

void        trap_R_SetClipRegion( const float *region );

//for cg_draw.c
void        Item_Text_Wrapped_Paint( itemDef_t *item );
const char  *Item_Text_Wrap( const char *text, float scale, float width );
void        UI_DrawTextBlock( rectDef_t *rect, float text_x, float text_y, vec4_t color,
                              float scale, int textalign, int textvalign,
                              int textStyle, const char *text );
void        UI_Text_Paint( float x, float y, float scale, vec4_t color, const char *text, float adjust, int style );
void        UI_Text_Paint_Limit( float *maxX, float x, float y, float scale,
                                 vec4_t color, const char *text, float adjust );
float       UI_Text_Width( const char *text, float scale );
float       UI_Text_Height( const char *text, float scale );
float       UI_Text_EmWidth( float scale );
float       UI_Text_EmHeight( float scale );
float       UI_Text_LineHeight( float scale );
qboolean    UI_Text_IsEmoticon( const char *s, qboolean *escaped, int *length, qhandle_t *h, int *width );
void        UI_EscapeEmoticons( char *dest, const char *src, int destsize );
glyphInfo_t *UI_Glyph( const fontMetrics_t *, const char *str );
int         trap_Parse_AddGlobalDefine( const char *define );
int         trap_Parse_LoadSource( const char *filename );
int         trap_Parse_FreeSource( int handle );
int         trap_Parse_ReadToken( int handle, pc_token_t *pc_token );
int         trap_Parse_SourceFileAndLine( int handle, char *filename, int *line );

void        BindingFromName( const char *cvar );

int         UI_GetChatColour( int mode, int team );

extern char g_nameBind1[ 32 ];
extern char g_nameBind2[ 32 ];

void       UI_R_Glyph( fontHandle_t, const char *str, glyphInfo_t *glyph );
void       UI_R_GlyphChar( fontHandle_t, int ch, glyphInfo_t *glyph );
void       UI_R_UnregisterFont( fontHandle_t );

int ui_CursorToOffset( const char *buf, int cursor );
int ui_OffsetToCursor( const char *buf, int offset );
const char *Gettext( const char *msgid ) PRINTF_TRANSLATE_ARG(1);
const char *Pgettext( const char *ctxt, const char *msgid ) PRINTF_TRANSLATE_ARG(2);
const char *GettextPlural( const char *msgid, const char *msgid2, int number ) PRINTF_TRANSLATE_ARG(1) PRINTF_TRANSLATE_ARG(2);
#endif
