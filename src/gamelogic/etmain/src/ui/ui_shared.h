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

#ifndef __UI_SHARED_H
#define __UI_SHARED_H


#include "../../../../engine/qcommon/q_shared.h"
#include "../../../../engine/renderer/tr_types.h"
#include "keycodes.h"

#include "../../ui/menudef.h"

#define MAX_MENUNAME 32
#define MAX_ITEMTEXT 64
#define MAX_ITEMACTION 64
#define MAX_MENUDEFFILE 4096
#define MAX_MENUFILE 32768
#define MAX_MENUS 128
//#define MAX_MENUITEMS 256
#define MAX_MENUITEMS 128		// JPW NERVE q3ta was 96
#define MAX_COLOR_RANGES 10
#define MAX_MODAL_MENUS 16

#define WINDOW_MOUSEOVER        0x00000001	// mouse is over it, non exclusive
#define WINDOW_HASFOCUS         0x00000002	// has cursor focus, exclusive
#define WINDOW_VISIBLE          0x00000004	// is visible
#define WINDOW_GREY             0x00000008	// is visible but grey ( non-active )
#define WINDOW_DECORATION       0x00000010	// for decoration only, no mouse, keyboard, etc..
#define WINDOW_FADINGOUT        0x00000020	// fading out, non-active
#define WINDOW_FADINGIN         0x00000040	// fading in
#define WINDOW_MOUSEOVERTEXT    0x00000080	// mouse is over it, non exclusive
#define WINDOW_INTRANSITION     0x00000100	// window is in transition
#define WINDOW_FORECOLORSET     0x00000200	// forecolor was explicitly set ( used to color alpha images or not )
#define WINDOW_HORIZONTAL       0x00000400	// for list boxes and sliders, vertical is default this is set of horizontal
#define WINDOW_LB_LEFTARROW     0x00000800	// mouse is over left/up arrow
#define WINDOW_LB_RIGHTARROW    0x00001000	// mouse is over right/down arrow
#define WINDOW_LB_THUMB         0x00002000	// mouse is over thumb
#define WINDOW_LB_PGUP          0x00004000	// mouse is over page up
#define WINDOW_LB_PGDN          0x00008000	// mouse is over page down
#define WINDOW_ORBITING         0x00010000	// item is in orbit
#define WINDOW_OOB_CLICK        0x00020000	// close on out of bounds click
#define WINDOW_WRAPPED          0x00040000	// manually wrap text
#define WINDOW_AUTOWRAPPED      0x00080000	// auto wrap text
#define WINDOW_FORCED           0x00100000	// forced open
#define WINDOW_POPUP            0x00200000	// popup
#define WINDOW_BACKCOLORSET     0x00400000	// backcolor was explicitly set
#define WINDOW_TIMEDVISIBLE     0x00800000	// visibility timing ( NOT implemented )
#define WINDOW_IGNORE_HUDALPHA  0x01000000	// window will apply cg_hudAlpha value to colors unless this flag is set
#define WINDOW_DRAWALWAYSONTOP  0x02000000
#define WINDOW_MODAL            0x04000000	// window is modal, the window to go back to is stored in a stack
#define WINDOW_FOCUSPULSE       0x08000000
#define WINDOW_TEXTASINT        0x10000000
#define WINDOW_TEXTASFLOAT      0x20000000
#define WINDOW_LB_SOMEWHERE     0x40000000

// CGAME cursor type bits
#define CURSOR_NONE             0x00000001
#define CURSOR_ARROW            0x00000002
#define CURSOR_SIZER            0x00000004

#ifdef CGAME
#define STRING_POOL_SIZE    128 * 1024
#else
#define STRING_POOL_SIZE    384 * 1024
#endif

#define MAX_STRING_HANDLES  4096
#define MAX_SCRIPT_ARGS     12
#define MAX_EDITFIELD       256

#define ART_FX_BASE         "menu/art/fx_base"
#define ART_FX_BLUE         "menu/art/fx_blue"
#define ART_FX_CYAN         "menu/art/fx_cyan"
#define ART_FX_GREEN        "menu/art/fx_grn"
#define ART_FX_RED          "menu/art/fx_red"
#define ART_FX_TEAL         "menu/art/fx_teal"
#define ART_FX_WHITE        "menu/art/fx_white"
#define ART_FX_YELLOW       "menu/art/fx_yel"

#define ASSET_GRADIENTBAR           "ui/assets/gradientbar2.tga"
#define ASSET_SCROLLBAR             "ui/assets/scrollbar.tga"
#define ASSET_SCROLLBAR_ARROWDOWN   "ui/assets/scrollbar_arrow_dwn_a.tga"
#define ASSET_SCROLLBAR_ARROWUP     "ui/assets/scrollbar_arrow_up_a.tga"
#define ASSET_SCROLLBAR_ARROWLEFT   "ui/assets/scrollbar_arrow_left.tga"
#define ASSET_SCROLLBAR_ARROWRIGHT  "ui/assets/scrollbar_arrow_right.tga"
#define ASSET_SCROLL_THUMB          "ui/assets/scrollbar_thumb.tga"
#define ASSET_SLIDER_BAR            "ui/assets/slider2.tga"
#define ASSET_SLIDER_THUMB          "ui/assets/sliderbutt_1.tga"
#define ASSET_CHECKBOX_CHECK        "ui/assets/check.tga"
#define ASSET_CHECKBOX_CHECK_NOT    "ui/assets/check_not.tga"
#define ASSET_CHECKBOX_CHECK_NO     "ui/assets/check_no.tga"

#define SCROLLBAR_SIZE      16.0
#define SLIDER_WIDTH        96.0
#define SLIDER_HEIGHT       10.0	// 16.0
#define SLIDER_THUMB_WIDTH  12.0
#define SLIDER_THUMB_HEIGHT 12.0	// 20.0
#define NUM_CROSSHAIRS      10

typedef struct scriptDef_s
{
	const char     *command;
	const char     *args[MAX_SCRIPT_ARGS];
} scriptDef_t;


typedef struct rectDef_s
{
	float           x;			// horiz position
	float           y;			// vert position
	float           w;			// width
	float           h;			// height;
} rectDef_t;

typedef rectDef_t Rectangle;

// FIXME: do something to separate text vs window stuff
typedef struct
{
	Rectangle       rect;		// client coord rectangle
	Rectangle       rectClient;	// screen coord rectangle
	const char     *name;		//
	const char     *model;		//
	const char     *group;		// if it belongs to a group
	const char     *cinematicName;	// cinematic name
	int             cinematic;	// cinematic handle
	int             style;		//
	int             border;		//
	int             ownerDraw;	// ownerDraw style
	int             ownerDrawFlags;	// show flags for ownerdraw items
	float           borderSize;	//
	int             flags;		// visible, focus, mouseover, cursor
	Rectangle       rectEffects;	// for various effects
	Rectangle       rectEffects2;	// for various effects
	int             offsetTime;	// time based value for various effects
	int             nextTime;	// time next effect should cycle
	vec4_t          foreColor;	// text color
	vec4_t          backColor;	// border color
	vec4_t          borderColor;	// border color
	vec4_t          outlineColor;	// border color
	qhandle_t       background;	// background asset
} windowDef_t;

typedef windowDef_t Window;


typedef struct
{
	vec4_t          color;
	int             type;
	float           low;
	float           high;
} colorRangeDef_t;

// FIXME: combine flags into bitfields to save space
// FIXME: consolidate all of the common stuff in one structure for menus and items
// THINKABOUTME: is there any compelling reason not to have items contain items
// and do away with a menu per say.. major issue is not being able to dynamically allocate
// and destroy stuff.. Another point to consider is adding an alloc free call for vm's and have
// the engine just allocate the pool for it based on a cvar
// many of the vars are re-used for different item types, as such they are not always named appropriately
// the benefits of c++ in DOOM will greatly help crap like this
// FIXME: need to put a type ptr that points to specific type info per type
//
#define MAX_LB_COLUMNS 16

typedef struct columnInfo_s
{
	int             pos;
	int             width;
	int             maxChars;
} columnInfo_t;

typedef struct listBoxDef_s
{
	int             startPos;
	int             endPos;
	int             drawPadding;
	int             cursorPos;
	float           elementWidth;
	float           elementHeight;
	int             elementStyle;
	int             numColumns;
	columnInfo_t    columnInfo[MAX_LB_COLUMNS];
	const char     *doubleClick;
	const char     *contextMenu;
	qboolean        notselectable;
} listBoxDef_t;

typedef struct editFieldDef_s
{
	float           minVal;		//  edit field limits
	float           maxVal;		//
	float           defVal;		//
	float           range;		//
	int             maxChars;	// for edit fields
	int             maxPaintChars;	// for edit fields
	int             paintOffset;	//
} editFieldDef_t;

#define MAX_MULTI_CVARS 32

typedef struct multiDef_s
{
	const char     *cvarList[MAX_MULTI_CVARS];
	const char     *cvarStr[MAX_MULTI_CVARS];
	float           cvarValue[MAX_MULTI_CVARS];
	int             count;
	qboolean        strDef;
	const char     *undefinedStr;
} multiDef_t;

typedef struct modelDef_s
{
	int             angle;
	vec3_t          origin;
	float           fov_x;
	float           fov_y;
	int             rotationSpeed;

	int             animated;
	int             startframe;
	int             numframes;
	int             loopframes;
	int             fps;

	int             frame;
	int             oldframe;
	float           backlerp;
	int             frameTime;
} modelDef_t;

#define CVAR_ENABLE     0x00000001
#define CVAR_DISABLE    0x00000002
#define CVAR_SHOW       0x00000004
#define CVAR_HIDE       0x00000008
#define CVAR_NOTOGGLE   0x00000010

// OSP - "setting" flags for items
#define SVS_DISABLED_SHOW   0x01
#define SVS_ENABLED_SHOW    0x02

#define UI_MAX_TEXT_LINES 64

typedef struct itemDef_s
{
	Window          window;		// common positional, border, style, layout info
	Rectangle       textRect;	// rectangle the text ( if any ) consumes
	int             type;		// text, button, radiobutton, checkbox, textfield, listbox, combo
	int             alignment;	// left center right
	int             textalignment;	// ( optional ) alignment for text within rect based on text width
	float           textalignx;	// ( optional ) text alignment x coord
	float           textaligny;	// ( optional ) text alignment x coord
	float           textscale;	// scale percentage from 72pts
	int             font;		// (SA)
	int             textStyle;	// ( optional ) style, normal and shadowed are it for now
	const char     *text;		// display text
	void           *parent;		// menu owner
	qhandle_t       asset;		// handle to asset
	const char     *mouseEnterText;	// mouse enter script
	const char     *mouseExitText;	// mouse exit script
	const char     *mouseEnter;	// mouse enter script
	const char     *mouseExit;	// mouse exit script
	const char     *action;		// select script
	const char     *onAccept;	// NERVE - SMF - run when the users presses the enter key
	const char     *onFocus;	// select script
	const char     *leaveFocus;	// select script
	const char     *cvar;		// associated cvar
	const char     *cvarTest;	// associated cvar for enable actions
	const char     *enableCvar;	// enable, disable, show, or hide based on value, this can contain a list
	int             cvarFlags;	//  what type of action to take on cvarenables
	sfxHandle_t     focusSound;
	int             numColors;	// number of color ranges
	colorRangeDef_t colorRanges[MAX_COLOR_RANGES];
	int             colorRangeType;	// either
	float           special;	// used for feeder id's etc.. diff per type
	int             cursorPos;	// cursor position in characters
	void           *typeData;	// type specific data ptr's

	// START - TAT 9/16/2002
	//      For the bot menu, we have context sensitive menus
	//      the way it works, we could have multiple items in a menu with the same hotkey
	//      so in the mission pack, we search through all the menu items to find the one that is applicable to this key press
	//      so the item has to store both the hotkey and the command to execute
	int             hotkey;
	const char     *onKey;
	// END - TAT 9/16/2002

	// OSP - on-the-fly enable/disable of items
	int             settingTest;
	int             settingFlags;
	int             voteFlag;

	const char     *onEsc;
	const char     *onEnter;

	struct itemDef_s *toolTipData;	// OSP - Tag an item to this item for auto-help popups

} itemDef_t;

typedef struct
{
	Window          window;
	const char     *font;		// font
	qboolean        fullScreen;	// covers entire screen
	int             itemCount;	// number of items;
	int             fontIndex;	//
	int             cursorItem;	// which item as the cursor
	int             fadeCycle;	//
	float           fadeClamp;	//
	float           fadeAmount;	//
	const char     *onOpen;		// run when the menu is first opened
	const char     *onClose;	// run when the menu is closed
	const char     *onESC;		// run when the escape key is hit
	const char     *onEnter;	// run when the enter key is hit

	int             timeout;	// ydnar: milliseconds until menu times out
	int             openTime;	// ydnar: time menu opened

// Omni-bot BEGIN
	qboolean        timedOut;	// cs: some menus we only want to timeout once every time they are opened
// Omni-bot END

	const char     *onTimeout;	// ydnar: run when menu times out

	const char     *onKey[255];	// NERVE - SMF - execs commands when a key is pressed
	const char     *soundName;	// background loop sound for menu

	vec4_t          focusColor;	// focus color for items
	vec4_t          disableColor;	// focus color for items
	itemDef_t      *items[MAX_MENUITEMS];	// items this menu contains

	// START - TAT 9/16/2002
	// should we search through all the items to find the hotkey instead of using the onKey array?
	//      The bot command menu needs to do this, see note above
	qboolean        itemHotkeyMode;
	// END - TAT 9/16/2002
} menuDef_t;

typedef struct
{
	const char     *fontStr;
	const char     *cursorStr;
	const char     *gradientStr;
	fontInfo_t      fonts[6];
	qhandle_t       cursor;
	qhandle_t       gradientBar;
	qhandle_t       scrollBarArrowUp;
	qhandle_t       scrollBarArrowDown;
	qhandle_t       scrollBarArrowLeft;
	qhandle_t       scrollBarArrowRight;
	qhandle_t       scrollBar;
	qhandle_t       scrollBarThumb;
	qhandle_t       buttonMiddle;
	qhandle_t       buttonInside;
	qhandle_t       solidBox;
	qhandle_t       sliderBar;
	qhandle_t       sliderThumb;
	qhandle_t       checkboxCheck;
	qhandle_t       checkboxCheckNot;
	qhandle_t       checkboxCheckNo;
	sfxHandle_t     menuEnterSound;
	sfxHandle_t     menuExitSound;
	sfxHandle_t     menuBuzzSound;
	sfxHandle_t     itemFocusSound;
	float           fadeClamp;
	int             fadeCycle;
	float           fadeAmount;
	float           shadowX;
	float           shadowY;
	vec4_t          shadowColor;
	float           shadowFadeClamp;
	qboolean        fontRegistered;

	// player settings
	qhandle_t       fxBasePic;
	qhandle_t       fxPic[7];
	qhandle_t       crosshairShader[NUM_CROSSHAIRS];
	qhandle_t       crosshairAltShader[NUM_CROSSHAIRS];
	
	fontInfo_t		textFont;
	fontInfo_t		smallFont;
	fontInfo_t		bigFont;
} cachedAssets_t;

typedef struct
{
	const char     *name;
	void            (*handler) (itemDef_t * item, qboolean * bAbort, char **args);
} commandDef_t;

typedef struct
{
	qhandle_t(*registerShaderNoMip) (const char *p);
	void            (*setColor) (const vec4_t v);
	void            (*drawHandlePic) (float x, float y, float w, float h, qhandle_t asset);
	void            (*drawStretchPic) (float x, float y, float w, float h, float s1, float t1, float s2, float t2,
									   qhandle_t hShader);
	void            (*drawText) (float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit,
								 int style);
	void            (*drawTextExt) (float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust,
									int limit, int style, fontInfo_t * font);
	int             (*textWidth) (const char *text, float scale, int limit);
	int             (*textWidthExt) (const char *text, float scale, int limit, fontInfo_t * font);
	int             (*multiLineTextWidth) (const char *text, float scale, int limit);
	int             (*textHeight) (const char *text, float scale, int limit);
	int             (*textHeightExt) (const char *text, float scale, int limit, fontInfo_t * font);
	int             (*multiLineTextHeight) (const char *text, float scale, int limit);
	void            (*textFont) (int font);	// NERVE - SMF
	                qhandle_t(*registerModel) (const char *p);
	void            (*modelBounds) (qhandle_t model, vec3_t min, vec3_t max);
	void            (*fillRect) (float x, float y, float w, float h, const vec4_t color);
	void            (*drawRect) (float x, float y, float w, float h, float size, const vec4_t color);
	void            (*drawSides) (float x, float y, float w, float h, float size);
	void            (*drawTopBottom) (float x, float y, float w, float h, float size);
	void            (*clearScene) ();
	void            (*addRefEntityToScene) (const refEntity_t * re);
	void            (*renderScene) (const refdef_t * fd);
	void            (*registerFont) (const char *pFontname, int pointSize, fontInfo_t * font);
	void            (*ownerDrawItem) (float x, float y, float w, float h, float text_x, float text_y, int ownerDraw,
									  int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader,
									  int textStyle);
	float           (*getValue) (int ownerDraw, int type);
	                qboolean(*ownerDrawVisible) (int flags);
	void            (*runScript) (char **p);
	void            (*getTeamColor) (vec4_t * color);
	void            (*getCVarString) (const char *cvar, char *buffer, int bufsize);
	float           (*getCVarValue) (const char *cvar);
	void            (*setCVar) (const char *cvar, const char *value);
	void            (*drawTextWithCursor) (float x, float y, float scale, vec4_t color, const char *text, int cursorPos,
										   char cursor, int limit, int style);
	void            (*setOverstrikeMode) (qboolean b);
	                qboolean(*getOverstrikeMode) ();
	void            (*startLocalSound) (sfxHandle_t sfx, int channelNum);
	                qboolean(*ownerDrawHandleKey) (int ownerDraw, int flags, float *special, int key);
	int             (*feederCount) (float feederID);
	const char     *(*feederItemText) (float feederID, int index, int column, qhandle_t * handles, int *numhandles);
	const char     *(*fileText) (char *flieName);
	                qhandle_t(*feederItemImage) (float feederID, int index);
	void            (*feederSelection) (float feederID, int index);
	                qboolean(*feederSelectionClick) (itemDef_t * item);
	void            (*feederAddItem) (float feederID, const char *name, int index);	// NERVE - SMF
	char           *(*translateString) (const char *string);	// NERVE - SMF
	void            (*checkAutoUpdate) ();	// DHM - Nerve
	void            (*getAutoUpdate) ();	// DHM - Nerve

	void            (*keynumToStringBuf) (int keynum, char *buf, int buflen);
	void            (*getBindingBuf) (int keynum, char *buf, int buflen);
	void            (*getKeysForBinding) (const char *binding, int *key1, int *key2);

	                qboolean(*keyIsDown) (int keynum);

	void            (*setBinding) (int keynum, const char *binding);
	void            (*executeText) (int exec_when, const char *text);
	void            (*Error) (int level, const char *error, ...) __attribute__ ((format (printf, 2, 3)));
	void            (*Print) (const char *msg, ...) __attribute__ ((format (printf, 1, 2)));
	void            (*Pause) (qboolean b);
	int             (*ownerDrawWidth) (int ownerDraw, float scale);
	                sfxHandle_t(*registerSound) (const char *name, qboolean compressed);
	void            (*startBackgroundTrack) (const char *intro, const char *loop);
	void            (*stopBackgroundTrack) ();
	int             (*playCinematic) (const char *name, float x, float y, float w, float h);
	void            (*stopCinematic) (int handle);
	void            (*drawCinematic) (int handle, float x, float y, float w, float h);
	void            (*runCinematicFrame) (int handle);

	// Gordon: campaign stuffs
	const char     *(*descriptionForCampaign) (void);
	const char     *(*nameForCampaign) (void);
	void            (*add2dPolys) (polyVert_t * verts, int numverts, qhandle_t hShader);
	void            (*updateScreen) (void);
	void            (*getHunkData) (int *hunkused, int *hunkexpected);
	int             (*getConfigString) (int index, char *buff, int buffsize);


	float           yscale;
	float           xscale;
	float           bias;
	int             realTime;
	int             frameTime;
	int             cursorx;
	int             cursory;
	qboolean        debug;

	cachedAssets_t  Assets;

	glconfig_t      glconfig;
	qhandle_t       whiteShader;
	qhandle_t       gradientImage;
	qhandle_t       cursor;
	float           FPS;
} displayContextDef_t;

const char     *String_Alloc(const char *p);
void            String_Init();
void            String_Report();
void            Init_Display(displayContextDef_t * dc);
void            Display_ExpandMacros(char *buff);
void            Menu_Init(menuDef_t * menu);
void            Item_Init(itemDef_t * item);
void            Menu_PostParse(menuDef_t * menu);
menuDef_t      *Menu_GetFocused();
void            Menu_HandleKey(menuDef_t * menu, int key, qboolean down);
void            Menu_HandleMouseMove(menuDef_t * menu, float x, float y);
void            Menu_ScrollFeeder(menuDef_t * menu, int feeder, qboolean down);
qboolean        Float_Parse(char **p, float *f);
qboolean        Color_Parse(char **p, vec4_t * c);
qboolean        Int_Parse(char **p, int *i);
qboolean        Rect_Parse(char **p, rectDef_t * r);
qboolean        String_Parse(char **p, const char **out);
qboolean        Script_Parse(char **p, const char **out);
void            PC_SourceError(int handle, char *format, ...);
void            PC_SourceWarning(int handle, char *format, ...);
qboolean        PC_Float_Parse(int handle, float *f);
qboolean        PC_Color_Parse(int handle, vec4_t * c);
qboolean        PC_Int_Parse(int handle, int *i);
qboolean        PC_Rect_Parse(int handle, rectDef_t * r);
qboolean        PC_String_Parse(int handle, const char **out);
qboolean        PC_Script_Parse(int handle, const char **out);
qboolean        PC_Char_Parse(int handle, char *out);	// NERVE - SMF
int             Menu_Count();
menuDef_t      *Menu_Get(int handle);
void            Menu_New(int handle);
void            Menu_PaintAll();
menuDef_t      *Menus_ActivateByName(const char *p, qboolean modalStack);
void            Menu_Reset();
qboolean        Menus_AnyFullScreenVisible();
void            Menus_Activate(menuDef_t * menu);
qboolean        Menus_CaptureFuncActive(void);

displayContextDef_t *Display_GetContext();
void           *Display_CaptureItem(int x, int y);
qboolean        Display_MouseMove(void *p, int x, int y);
int             Display_CursorType(int x, int y);
qboolean        Display_KeyBindPending();
void            Menus_OpenByName(const char *p);
menuDef_t      *Menus_FindByName(const char *p);
void            Menus_ShowByName(const char *p);
void            Menus_CloseByName(const char *p);
void            Display_HandleKey(int key, qboolean down, int x, int y);
void            LerpColor(vec4_t a, vec4_t b, vec4_t c, float t);
void            Menus_CloseAll();
void            Menu_Paint(menuDef_t * menu, qboolean forcePaint);
void            Menu_SetFeederSelection(menuDef_t * menu, int feeder, int index, const char *name);
void            Display_CacheAll();

// TTimo
void            Menu_ShowItemByName(menuDef_t * menu, const char *p, qboolean bShow);

void           *UI_Alloc(int size);
void            UI_InitMemory(void);
qboolean        UI_OutOfMemory();

void            Controls_GetConfig(void);
void            Controls_SetConfig(qboolean restart);
void            Controls_SetDefaults(qboolean lefthanded);

int             trap_PC_AddGlobalDefine(char *define);
int             trap_PC_RemoveAllGlobalDefines(void);
int             trap_PC_LoadSource(const char *filename);
int             trap_PC_FreeSource(int handle);
int             trap_PC_ReadToken(int handle, pc_token_t * pc_token);
int             trap_PC_SourceFileAndLine(int handle, char *filename, int *line);
int             trap_PC_UnReadToken(int handle);


//
// panelhandling
//

typedef struct panel_button_s panel_button_t;

typedef struct panel_button_text_s
{
	float           scalex, scaley;
	vec4_t          colour;
	int             style;
	int             align;
	fontInfo_t     *font;
} panel_button_text_t;

typedef         qboolean(*panel_button_key_down) (panel_button_t *, int);
typedef         qboolean(*panel_button_key_up) (panel_button_t *, int);
typedef void    (*panel_button_render) (panel_button_t *);
typedef void    (*panel_button_postprocess) (panel_button_t *);

// Button struct
struct panel_button_s
{
	// compile time stuff
	// ======================
	const char     *shaderNormal;

	// text
	const char     *text;

	// rect
	rectDef_t       rect;

	// data
	int             data[8];

	// "font"
	panel_button_text_t *font;

	// functions
	panel_button_key_down onKeyDown;
	panel_button_key_up onKeyUp;
	panel_button_render onDraw;
	panel_button_postprocess onFinish;

	// run-time stuff
	// ======================
	qhandle_t       hShaderNormal;
};

void            BG_PanelButton_RenderEdit(panel_button_t * button);
qboolean        BG_PanelButton_EditClick(panel_button_t * button, int key);
qboolean        BG_PanelButtonsKeyEvent(int key, qboolean down, panel_button_t ** buttons);
void            BG_PanelButtonsSetup(panel_button_t ** buttons);
void            BG_PanelButtonsRender(panel_button_t ** buttons);
void            BG_PanelButtonsRender_Text(panel_button_t * button);
void            BG_PanelButtonsRender_TextExt(panel_button_t * button, const char *text);
void            BG_PanelButtonsRender_Img(panel_button_t * button);
panel_button_t *BG_PanelButtonsGetHighlightButton(panel_button_t ** buttons);
void            BG_PanelButtons_SetFocusButton(panel_button_t * button);
panel_button_t *BG_PanelButtons_GetFocusButton(void);

qboolean        BG_RectContainsPoint(float x, float y, float w, float h, float px, float py);
qboolean        BG_CursorInRect(rectDef_t * rect);

void            BG_FitTextToWidth_Ext(char *instr, float scale, float w, int size, fontInfo_t * font);

void            AdjustFrom640(float *x, float *y, float *w, float *h);
void            SetupRotatedThing(polyVert_t * verts, vec2_t org, float w, float h, vec_t angle);

#endif
