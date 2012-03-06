/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the OpenWolf GPL Source Code (OpenWolf Source Code).  

OpenWolf Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenWolf Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the OpenWolf Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the OpenWolf 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

//
// string allocation/managment

#include "ui_shared.h"
#include "ui_local.h"			// For CS settings/retrieval


#define SCROLL_TIME_START                   500
#define SCROLL_TIME_ADJUST              150
#define SCROLL_TIME_ADJUSTOFFSET    40
#define SCROLL_TIME_FLOOR                   20

typedef struct scrollInfo_s
{
	int             nextScrollTime;
	int             nextAdjustTime;
	int             adjustValue;
	int             scrollKey;
	float           xStart;
	float           yStart;
	itemDef_t      *item;
	qboolean        scrollDir;
} scrollInfo_t;

static scrollInfo_t scrollInfo;

static void     (*captureFunc) (void *p) = NULL;
static void    *captureData = NULL;
static itemDef_t *itemCapture = NULL;	// item that has the mouse captured ( if any )

displayContextDef_t *DC = NULL;

qboolean        g_waitingForKey = qfalse;
qboolean        g_editingField = qfalse;

itemDef_t      *g_bindItem = NULL;
itemDef_t      *g_editItem = NULL;

menuDef_t       Menus[MAX_MENUS];	// defined menus
int             menuCount = 0;	// how many

// TTimo
// a stack for modal menus only, stores the menus to come back to
// (an item can be NULL, goes back to main menu / no action required)
menuDef_t      *modalMenuStack[MAX_MODAL_MENUS];
int             modalMenuCount = 0;

static qboolean debugMode = qfalse;

#define DOUBLE_CLICK_DELAY 300
static int      lastListBoxClickTime = 0;

void            Item_MouseLeave(itemDef_t * item);
void            Item_SetMouseOver(itemDef_t * item, qboolean focus);
void            Item_Paint(itemDef_t * item);
void            Item_RunScript(itemDef_t * item, qboolean * bAbort, const char *s);
void            Item_SetupKeywordHash(void);
void            Menu_SetupKeywordHash(void);
int             BindingIDFromName(const char *name);
qboolean        Item_Bind_HandleKey(itemDef_t * item, int key, qboolean down);
itemDef_t      *Menu_SetPrevCursorItem(menuDef_t * menu);
itemDef_t      *Menu_SetNextCursorItem(menuDef_t * menu);
static qboolean Menu_OverActiveItem(menuDef_t * menu, float x, float y);

#ifdef CGAME
#define MEM_POOL_SIZE  128 * 1024
#else
#define MEM_POOL_SIZE 2048 * 1024
#endif

static char     memoryPool[MEM_POOL_SIZE];
static int      allocPoint, outOfMemory;


void Tooltip_Initialize(itemDef_t * item)
{
	item->text = NULL;
	item->font = UI_FONT_COURBD_21;
	item->textalignx = 3;
	item->textaligny = 10;
	item->textscale = .2f;
	item->window.border = WINDOW_BORDER_FULL;
	item->window.borderSize = 1.f;
	item->window.flags &= ~WINDOW_VISIBLE;
	item->window.flags |= (WINDOW_DRAWALWAYSONTOP | WINDOW_AUTOWRAPPED);
	Vector4Set(item->window.backColor, .9f, .9f, .75f, 1.f);
	Vector4Set(item->window.borderColor, 0.f, 0.f, 0.f, 1.f);
	Vector4Set(item->window.foreColor, 0.f, 0.f, 0.f, 1.f);
}

void Tooltip_ComputePosition(itemDef_t * item)
{
	Rectangle      *itemRect = &item->window.rectClient;
	Rectangle      *tipRect = &item->toolTipData->window.rectClient;

	DC->textFont(item->toolTipData->font);

	// Set positioning based on item location
	tipRect->x = itemRect->x + (itemRect->w / 3);
	tipRect->y = itemRect->y + itemRect->h + 8;
	//tipRect->h = 14.0f;
	//tipRect->w = DC->textWidth( item->toolTipData->text, item->toolTipData->textscale, 0 ) + 6.0f;
	tipRect->h = DC->multiLineTextHeight(item->toolTipData->text, item->toolTipData->textscale, 0) + 9.f;
	tipRect->w = DC->multiLineTextWidth(item->toolTipData->text, item->toolTipData->textscale, 0) + 6.f;
	if((tipRect->w + tipRect->x) > 635.0f)
	{
		tipRect->x -= (tipRect->w + tipRect->x) - 635.0f;
	}

	item->toolTipData->parent = item->parent;
	item->toolTipData->type = ITEM_TYPE_TEXT;
	item->toolTipData->window.style = WINDOW_STYLE_FILLED;
	item->toolTipData->window.flags |= WINDOW_VISIBLE;
}


/*
===============
UI_Alloc
===============
*/
void           *UI_Alloc(int size)
{
	char           *p;

	if(allocPoint + size > MEM_POOL_SIZE)
	{
		outOfMemory = qtrue;
		if(DC->Print)
		{
			DC->Print("UI_Alloc: Failure. Out of memory!\n");
		}
		//DC->trap_Print(S_COLOR_YELLOW"WARNING: UI Out of Memory!\n");
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += (size + 15) & ~15;

	return p;
}

/*
===============
UI_InitMemory
===============
*/
void UI_InitMemory(void)
{
	allocPoint = 0;
	outOfMemory = qfalse;
}

qboolean UI_OutOfMemory()
{
	return outOfMemory;
}





#define HASH_TABLE_SIZE 2048
/*
================
return a hash value for the string
================
*/
static long hashForString(const char *str)
{
	int             i;
	long            hash;
	char            letter;

	hash = 0;
	i = 0;
	while(str[i] != '\0')
	{
		letter = tolower(str[i]);
		hash += (long)(letter) * (i + 119);
		i++;
	}
	hash &= (HASH_TABLE_SIZE - 1);
	return hash;
}

typedef struct stringDef_s
{
	struct stringDef_s *next;
	const char     *str;
} stringDef_t;

static int      strPoolIndex = 0;
static char     strPool[STRING_POOL_SIZE];

static int      strHandleCount = 0;
static stringDef_t *strHandle[HASH_TABLE_SIZE];


const char     *String_Alloc(const char *p)
{
	int             len;
	long            hash;
	stringDef_t    *str, *last;
	static const char *staticNULL = "";

	if(p == NULL)
	{
		return NULL;
	}

	if(*p == 0)
	{
		return staticNULL;
	}

	hash = hashForString(p);

	str = strHandle[hash];
	while(str)
	{
		if(strcmp(p, str->str) == 0)
		{
			return str->str;
		}
		str = str->next;
	}

	len = strlen(p);
	if(len + strPoolIndex + 1 < STRING_POOL_SIZE)
	{
		int             ph = strPoolIndex;

		strcpy(&strPool[strPoolIndex], p);
		strPoolIndex += len + 1;

		str = strHandle[hash];
		last = str;
		while(str && str->next)
		{
			str = str->next;
			last = str;
		}

		str = UI_Alloc(sizeof(stringDef_t));
		str->next = NULL;
		str->str = &strPool[ph];
		if(last)
		{
			last->next = str;
		}
		else
		{
			strHandle[hash] = str;
		}
		return &strPool[ph];
	}
	return NULL;
}

void String_Report()
{
	float           f;

	Com_Printf("Memory/String Pool Info\n");
	Com_Printf("----------------\n");
	f = strPoolIndex;
	f /= STRING_POOL_SIZE;
	f *= 100;
	Com_Printf("String Pool is %.1f%% full, %i bytes out of %i used.\n", f, strPoolIndex, STRING_POOL_SIZE);
	f = allocPoint;
	f /= MEM_POOL_SIZE;
	f *= 100;
	Com_Printf("Memory Pool is %.1f%% full, %i bytes out of %i used.\n", f, allocPoint, MEM_POOL_SIZE);
}

/*
=================
String_Init
=================
*/
void String_Init()
{
	int             i;

	for(i = 0; i < HASH_TABLE_SIZE; i++)
	{
		strHandle[i] = 0;
	}
	strHandleCount = 0;
	strPoolIndex = 0;
	menuCount = 0;
	modalMenuCount = 0;
	UI_InitMemory();
	Item_SetupKeywordHash();
	Menu_SetupKeywordHash();
	if(DC && DC->getBindingBuf)
	{
		Controls_GetConfig();
	}
}

/*
=================
LerpColor
	lerp and clamp each component of <a> and <b> into <c> by the fraction <t>
=================
*/
void LerpColor(vec4_t a, vec4_t b, vec4_t c, float t)
{
	int             i;

	for(i = 0; i < 4; i++)
	{
		c[i] = a[i] + t * (b[i] - a[i]);
		if(c[i] < 0)
		{
			c[i] = 0;
		}
		else if(c[i] > 1.0)
		{
			c[i] = 1.0;
		}
	}
}

/*
=================
Float_Parse
=================
*/
qboolean Float_Parse(char **p, float *f)
{
	char           *token;

	token = COM_ParseExt(p, qfalse);
	if(token && token[0] != 0)
	{
		*f = atof(token);
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*
=================
Color_Parse
=================
*/
qboolean Color_Parse(char **p, vec4_t * c)
{
	int             i;
	float           f = 0.0f;

	for(i = 0; i < 4; i++)
	{
		if(!Float_Parse(p, &f))
		{
			return qfalse;
		}
		(*c)[i] = f;
	}
	return qtrue;
}

/*
=================
Int_Parse
=================
*/
qboolean Int_Parse(char **p, int *i)
{
	char           *token;

	token = COM_ParseExt(p, qfalse);

	if(token && token[0] != 0)
	{
		*i = atoi(token);
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*
=================
Rect_Parse
=================
*/
qboolean Rect_Parse(char **p, rectDef_t * r)
{
	if(Float_Parse(p, &r->x))
	{
		if(Float_Parse(p, &r->y))
		{
			if(Float_Parse(p, &r->w))
			{
				if(Float_Parse(p, &r->h))
				{
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

/*
=================
String_Parse
=================
*/
qboolean String_Parse(char **p, const char **out)
{
	char           *token;

	token = COM_ParseExt(p, qfalse);
	if(token && token[0] != 0)
	{
		*(out) = String_Alloc(token);
		return qtrue;
	}
	return qfalse;
}

// NERVE - SMF
/*
=================
PC_Char_Parse
=================
*/
qboolean PC_Char_Parse(int handle, char *out)
{
	pc_token_t      token;

	if(!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	*(out) = token.string[0];
	return qtrue;
}

// -NERVE - SMF

/*
=================
PC_Script_Parse
=================
*/
qboolean PC_Script_Parse(int handle, const char **out)
{
	char            script[4096];
	pc_token_t      token;

	memset(script, 0, sizeof(script));
	// scripts start with { and have ; separated command lists.. commands are command, arg..
	// basically we want everything between the { } as it will be interpreted at run time

	if(!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}
	if(Q_stricmp(token.string, "{") != 0)
	{
		return qfalse;
	}

	while(1)
	{
		if(!trap_PC_ReadToken(handle, &token))
		{
			return qfalse;
		}

		if(Q_stricmp(token.string, "}") == 0)
		{
			*out = String_Alloc(script);
			return qtrue;
		}

		if(token.string[1] != '\0')
		{
			Q_strcat(script, 4096, va("\"%s\"", token.string));
		}
		else
		{
			Q_strcat(script, 4096, token.string);
		}
		Q_strcat(script, 4096, " ");
	}
	return qfalse;				// bk001105 - LCC   missing return value
}

// display, window, menu, item code
//

/*
==================
Init_Display

Initializes the display with a structure to all the drawing routines
 ==================
*/
void Init_Display(displayContextDef_t * dc)
{
	DC = dc;
}



// type and style painting

void GradientBar_Paint(rectDef_t * rect, vec4_t color)
{
	// gradient bar takes two paints
	DC->setColor(color);
	DC->drawHandlePic(rect->x, rect->y, rect->w, rect->h, DC->Assets.gradientBar);
	DC->setColor(NULL);
}


/*
==================
Window_Init

Initializes a window structure ( windowDef_t ) with defaults

==================
*/
void Window_Init(Window * w)
{
	memset(w, 0, sizeof(windowDef_t));
	w->borderSize = 1;
	w->foreColor[0] = w->foreColor[1] = w->foreColor[2] = w->foreColor[3] = 1.0;
	w->cinematic = -1;
}

void Fade(int *flags, float *f, float clamp, int *nextTime, int offsetTime, qboolean bFlags, float fadeAmount)
{
	if(*flags & (WINDOW_FADINGOUT | WINDOW_FADINGIN))
	{
		if(DC->realTime > *nextTime)
		{
			*nextTime = DC->realTime + offsetTime;
			if(*flags & WINDOW_FADINGOUT)
			{
				*f -= fadeAmount;
				if(bFlags && *f <= 0.0)
				{
					*flags &= ~(WINDOW_FADINGOUT | WINDOW_VISIBLE);
				}
			}
			else
			{
				*f += fadeAmount;
				if(*f >= clamp)
				{
					*f = clamp;
					if(bFlags)
					{
						*flags &= ~WINDOW_FADINGIN;
					}
				}
			}
		}
	}
}



void Window_Paint(Window * w, float fadeAmount, float fadeClamp, float fadeCycle)
{
	//float bordersize = 0;
	vec4_t          color;
	rectDef_t       fillRect = w->rect;

	if(debugMode)
	{
		color[0] = color[1] = color[2] = color[3] = 1;
		DC->drawRect(w->rect.x, w->rect.y, w->rect.w, w->rect.h, 1, color);
	}

	if(w == NULL || (w->style == 0 && w->border == 0))
	{
		return;
	}

	// FIXME: do right thing for right border type
	if(w->border != 0)
	{
		fillRect.x += w->borderSize;
		fillRect.y += w->borderSize;
		fillRect.w -= 2 * w->borderSize;
		fillRect.h -= 2 * w->borderSize;
	}

	if(w->style == WINDOW_STYLE_FILLED)
	{
		// box, but possible a shader that needs filled
		if(w->background)
		{
			Fade(&w->flags, &w->backColor[3], fadeClamp, &w->nextTime, fadeCycle, qtrue, fadeAmount);
			DC->setColor(w->backColor);
			DC->drawHandlePic(fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->background);
			DC->setColor(NULL);
		}
		else
		{
			DC->fillRect(fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->backColor);
		}
	}
	else if(w->style == WINDOW_STYLE_GRADIENT)
	{
		GradientBar_Paint(&fillRect, w->backColor);
		// gradient bar
	}
	else if(w->style == WINDOW_STYLE_SHADER)
	{
		if(w->flags & WINDOW_FORECOLORSET)
		{
			DC->setColor(w->foreColor);
		}
		DC->drawHandlePic(fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->background);
		DC->setColor(NULL);
	}
	else if(w->style == WINDOW_STYLE_TEAMCOLOR)
	{
		if(DC->getTeamColor)
		{
			DC->getTeamColor(&color);
			DC->fillRect(fillRect.x, fillRect.y, fillRect.w, fillRect.h, color);
		}
	}
	else if(w->style == WINDOW_STYLE_CINEMATIC)
	{
		if(w->cinematic == -1)
		{
			w->cinematic = DC->playCinematic(w->cinematicName, fillRect.x, fillRect.y, fillRect.w, fillRect.h);
			if(w->cinematic == -1)
			{
				w->cinematic = -2;
			}
		}
		if(w->cinematic >= 0)
		{
			DC->runCinematicFrame(w->cinematic);
			DC->drawCinematic(w->cinematic, fillRect.x, fillRect.y, fillRect.w, fillRect.h);
		}
	}

	if(w->border == WINDOW_BORDER_FULL)
	{
		// full
		// HACK HACK HACK
		if(w->style == WINDOW_STYLE_TEAMCOLOR)
		{
			if(color[0] > 0)
			{
				// red
				color[0] = 1;
				color[1] = color[2] = .5;

			}
			else
			{
				color[2] = 1;
				color[0] = color[1] = .5;
			}
			color[3] = 1;
			DC->drawRect(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, color);
		}
		else
		{
			DC->drawRect(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, w->borderColor);
		}
	}
	else if(w->border == WINDOW_BORDER_HORZ)
	{
		// top/bottom
		DC->setColor(w->borderColor);
		DC->drawTopBottom(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize);
		DC->setColor(NULL);
	}
	else if(w->border == WINDOW_BORDER_VERT)
	{
		// left right
		DC->setColor(w->borderColor);
		DC->drawSides(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize);
		DC->setColor(NULL);
	}
	else if(w->border == WINDOW_BORDER_KCGRADIENT)
	{
		// this is just two gradient bars along each horz edge
		rectDef_t       r = w->rect;

		r.h = w->borderSize;
		GradientBar_Paint(&r, w->borderColor);
		r.y = w->rect.y + w->rect.h - 1;
		GradientBar_Paint(&r, w->borderColor);
	}

}


void Item_SetScreenCoords(itemDef_t * item, float x, float y)
{

	if(item == NULL)
	{
		return;
	}

	item->window.rect.x = x + item->window.rectClient.x;
	item->window.rect.y = y + item->window.rectClient.y;
	item->window.rect.w = item->window.rectClient.w;
	item->window.rect.h = item->window.rectClient.h;

	// FIXME: do the proper thing for the right borders here?
	/*if( item->window.border != 0 ) {
	   item->window.rect.x += item->window.borderSize;
	   item->window.rect.y += item->window.borderSize;
	   item->window.rect.w -= 2 * item->window.borderSize;
	   item->window.rect.h -= 2 * item->window.borderSize;
	   } */

	// Don't let tooltips draw off the screen.
	if(item->toolTipData)
	{
		Item_SetScreenCoords(item->toolTipData, x, y);
		{
			float           val = (item->toolTipData->window.rect.x + item->toolTipData->window.rect.w) - 635.0f;

			if(val > 0.0f)
			{
				item->toolTipData->window.rectClient.x -= val;
				item->toolTipData->window.rect.x -= val;
			}
		}
	}

	// force the text rects to recompute
	item->textRect.w = 0;
	item->textRect.h = 0;
}

// FIXME: consolidate this with nearby stuff
void Item_UpdatePosition(itemDef_t * item)
{
	float           x, y;
	menuDef_t      *menu;

	if(item == NULL || item->parent == NULL)
	{
		return;
	}

	menu = item->parent;

	x = menu->window.rect.x;
	y = menu->window.rect.y;

	/*if (menu->window.border != 0) {
	   x += menu->window.borderSize;
	   y += menu->window.borderSize;
	   } */

	Item_SetScreenCoords(item, x, y);

}

// menus
void Menu_UpdatePosition(menuDef_t * menu)
{
	int             i;
	float           x, y;

	if(menu == NULL)
	{
		return;
	}

	x = menu->window.rect.x;
	y = menu->window.rect.y;

	/*if (menu->window.border != 0) {
	   x += menu->window.borderSize;
	   y += menu->window.borderSize;
	   } */

	for(i = 0; i < menu->itemCount; i++)
	{
		Item_SetScreenCoords(menu->items[i], x, y);
	}
}

void Menu_PostParse(menuDef_t * menu)
{
	if(menu == NULL)
	{
		return;
	}
	if(menu->fullScreen)
	{
		menu->window.rect.x = 0;
		menu->window.rect.y = 0;
		menu->window.rect.w = 640;
		menu->window.rect.h = 480;
	}
	Menu_UpdatePosition(menu);
}

itemDef_t      *Menu_ClearFocus(menuDef_t * menu)
{
	int             i;
	itemDef_t      *ret = NULL;

	if(menu == NULL)
	{
		return (NULL);
	}

	for(i = 0; i < menu->itemCount; i++)
	{
		if(menu->items[i]->window.flags & WINDOW_HASFOCUS)
		{
			ret = menu->items[i];
			menu->items[i]->window.flags &= ~WINDOW_HASFOCUS;
		}

		if(menu->items[i]->window.flags & WINDOW_MOUSEOVER)
		{
			Item_MouseLeave(menu->items[i]);
			Item_SetMouseOver(menu->items[i], qfalse);
		}

		if(menu->items[i]->leaveFocus)
		{
			Item_RunScript(menu->items[i], NULL, menu->items[i]->leaveFocus);
		}
	}

	return (ret);
}

qboolean IsVisible(int flags)
{
	return (flags & WINDOW_VISIBLE && !(flags & WINDOW_FADINGOUT));
}

qboolean Rect_ContainsPoint(rectDef_t * rect, float x, float y)
{
	if(rect)
	{
		if(x > rect->x && x < rect->x + rect->w && y > rect->y && y < rect->y + rect->h)
		{
			return qtrue;
		}
	}
	return qfalse;
}

int Menu_ItemsMatchingGroup(menuDef_t * menu, const char *name)
{
	int             i;
	int             count = 0;
	char           *pdest;
	int             wildcard = -1;	// if wildcard is set, it's value is the number of characters to compare


	pdest = strstr(name, "*");	// allow wildcard strings (ex.  "hide nb_*" would translate to "hide nb_pg1; hide nb_extra" etc)
	if(pdest)
	{
		wildcard = pdest - name;
	}

	for(i = 0; i < menu->itemCount; i++)
	{
		if(wildcard != -1)
		{
			if(Q_strncmp(menu->items[i]->window.name, name, wildcard) == 0 ||
			   (menu->items[i]->window.group && Q_strncmp(menu->items[i]->window.group, name, wildcard) == 0))
			{
				count++;
			}
		}
		else
		{
			if(Q_stricmp(menu->items[i]->window.name, name) == 0 ||
			   (menu->items[i]->window.group && Q_stricmp(menu->items[i]->window.group, name) == 0))
			{
				count++;
			}
		}
	}

	return count;
}

itemDef_t      *Menu_GetMatchingItemByNumber(menuDef_t * menu, int index, const char *name)
{
	int             i;
	int             count = 0;
	char           *pdest;
	int             wildcard = -1;	// if wildcard is set, it's value is the number of characters to compare

	pdest = strstr(name, "*");	// allow wildcard strings (ex.  "hide nb_*" would translate to "hide nb_pg1; hide nb_extra" etc)
	if(pdest)
	{
		wildcard = pdest - name;
	}

	for(i = 0; i < menu->itemCount; i++)
	{
		if(wildcard != -1)
		{
			if(Q_strncmp(menu->items[i]->window.name, name, wildcard) == 0 ||
			   (menu->items[i]->window.group && Q_strncmp(menu->items[i]->window.group, name, wildcard) == 0))
			{
				if(count == index)
				{
					return menu->items[i];
				}
				count++;
			}
		}
		else
		{
			if(Q_stricmp(menu->items[i]->window.name, name) == 0 ||
			   (menu->items[i]->window.group && Q_stricmp(menu->items[i]->window.group, name) == 0))
			{
				if(count == index)
				{
					return menu->items[i];
				}
				count++;
			}
		}
	}
	return NULL;
}

void Script_SetColor(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;
	int             i;
	float           f = 0.0f;
	vec4_t         *out;

	// expecting type of color to set and 4 args for the color
	if(String_Parse(args, &name))
	{
		out = NULL;
		if(Q_stricmp(name, "backcolor") == 0)
		{
			out = &item->window.backColor;
			item->window.flags |= WINDOW_BACKCOLORSET;
		}
		else if(Q_stricmp(name, "forecolor") == 0)
		{
			out = &item->window.foreColor;
			item->window.flags |= WINDOW_FORECOLORSET;
		}
		else if(Q_stricmp(name, "bordercolor") == 0)
		{
			out = &item->window.borderColor;
		}

		if(out)
		{
			for(i = 0; i < 4; i++)
			{
				if(!Float_Parse(args, &f))
				{
					return;
				}
				(*out)[i] = f;
			}
		}
	}
}

void Script_SetAsset(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name;

	// expecting name to set asset to
	if(String_Parse(args, &name))
	{
		// check for a model
		if(item->type == ITEM_TYPE_MODEL)
		{
		}
	}
}

void Script_SetBackground(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;

	// expecting name to set asset to
	if(String_Parse(args, &name))
	{
		item->window.background = DC->registerShaderNoMip(name);
	}
}




itemDef_t      *Menu_FindItemByName(menuDef_t * menu, const char *p)
{
	int             i;

	if(menu == NULL || p == NULL)
	{
		return NULL;
	}

	for(i = 0; i < menu->itemCount; i++)
	{
		if(Q_stricmp(p, menu->items[i]->window.name) == 0)
		{
			return menu->items[i];
		}
	}

	return NULL;
}

void Script_SetTeamColor(itemDef_t * item, qboolean * bAbort, char **args)
{
	if(DC->getTeamColor)
	{
		int             i;
		vec4_t          color;

		DC->getTeamColor(&color);
		for(i = 0; i < 4; i++)
		{
			item->window.backColor[i] = color[i];
		}
	}
}

void Script_SetItemColor(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *itemname = NULL;
	const char     *name = NULL;
	vec4_t          color;
	int             i;
	vec4_t         *out;

	// expecting type of color to set and 4 args for the color
	if(String_Parse(args, &itemname) && String_Parse(args, &name))
	{
		itemDef_t      *item2;
		int             j;
		int             count = Menu_ItemsMatchingGroup(item->parent, itemname);

		if(!Color_Parse(args, &color))
		{
			return;
		}

		for(j = 0; j < count; j++)
		{
			item2 = Menu_GetMatchingItemByNumber(item->parent, j, itemname);
			if(item2 != NULL)
			{
				out = NULL;
				if(Q_stricmp(name, "backcolor") == 0)
				{
					out = &item2->window.backColor;
				}
				else if(Q_stricmp(name, "forecolor") == 0)
				{
					out = &item2->window.foreColor;
					item2->window.flags |= WINDOW_FORECOLORSET;
				}
				else if(Q_stricmp(name, "bordercolor") == 0)
				{
					out = &item2->window.borderColor;
				}

				if(out)
				{
					for(i = 0; i < 4; i++)
					{
						(*out)[i] = color[i];
					}
				}
			}
		}
	}
}

void Script_SetMenuItemColor(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *menuname = NULL;
	const char     *itemname = NULL;
	const char     *name = NULL;
	vec4_t          color;
	int             i;
	vec4_t         *out;

	// expecting type of color to set and 4 args for the color
	if(String_Parse(args, &menuname) && String_Parse(args, &itemname) && String_Parse(args, &name))
	{
		menuDef_t      *menu = Menus_FindByName(menuname);
		itemDef_t      *item2;
		int             j;
		int             count;

		if(!menu)
		{
			return;
		}

		count = Menu_ItemsMatchingGroup(menu, itemname);

		if(!Color_Parse(args, &color))
		{
			return;
		}

		for(j = 0; j < count; j++)
		{
			item2 = Menu_GetMatchingItemByNumber(menu, j, itemname);
			if(item2 != NULL)
			{
				out = NULL;
				if(Q_stricmp(name, "backcolor") == 0)
				{
					out = &item2->window.backColor;
				}
				else if(Q_stricmp(name, "forecolor") == 0)
				{
					out = &item2->window.foreColor;
					item2->window.flags |= WINDOW_FORECOLORSET;
				}
				else if(Q_stricmp(name, "bordercolor") == 0)
				{
					out = &item2->window.borderColor;
				}

				if(out)
				{
					for(i = 0; i < 4; i++)
					{
						(*out)[i] = color[i];
					}
				}
			}
		}
	}
}


void Menu_ShowItemByName(menuDef_t * menu, const char *p, qboolean bShow)
{
	itemDef_t      *item;
	int             i;
	int             count = Menu_ItemsMatchingGroup(menu, p);

	for(i = 0; i < count; i++)
	{
		item = Menu_GetMatchingItemByNumber(menu, i, p);
		if(item != NULL)
		{
			if(bShow)
			{
				item->window.flags |= WINDOW_VISIBLE;
			}
			else
			{
				if(item->window.flags & WINDOW_MOUSEOVER)
				{
					Item_MouseLeave(item);
					Item_SetMouseOver(item, qfalse);
				}

				item->window.flags &= ~WINDOW_VISIBLE;

				// stop cinematics playing in the window
				if(item->window.cinematic >= 0)
				{
					DC->stopCinematic(item->window.cinematic);
					item->window.cinematic = -1;
				}
			}
		}
	}
}

void Menu_FadeItemByName(menuDef_t * menu, const char *p, qboolean fadeOut)
{
	itemDef_t      *item;
	int             i;
	int             count = Menu_ItemsMatchingGroup(menu, p);

	for(i = 0; i < count; i++)
	{
		item = Menu_GetMatchingItemByNumber(menu, i, p);
		if(item != NULL)
		{
			if(fadeOut)
			{
				item->window.flags |= (WINDOW_FADINGOUT | WINDOW_VISIBLE);
				item->window.flags &= ~WINDOW_FADINGIN;
			}
			else
			{
				item->window.flags |= (WINDOW_VISIBLE | WINDOW_FADINGIN);
				item->window.flags &= ~WINDOW_FADINGOUT;
			}
		}
	}
}

menuDef_t      *Menus_FindByName(const char *p)
{
	int             i;

	for(i = 0; i < menuCount; i++)
	{
		if(Q_stricmp(Menus[i].window.name, p) == 0)
		{
			return &Menus[i];
		}
	}
	return NULL;
}

void Menus_ShowByName(const char *p)
{
	menuDef_t      *menu = Menus_FindByName(p);

	if(menu)
	{
		Menus_Activate(menu);
	}
}

void Menus_OpenByName(const char *p)
{
	Menus_ActivateByName(p, qtrue);
}

static void Menu_RunCloseScript(menuDef_t * menu)
{
	if(menu && menu->window.flags & WINDOW_VISIBLE && menu->onClose)
	{
		itemDef_t       item;

		item.parent = menu;
		Item_RunScript(&item, NULL, menu->onClose);
	}
}

void Menus_CloseByName(const char *p)
{
	menuDef_t      *menu = Menus_FindByName(p);

	if(menu != NULL)
	{
		int             i;

		// Gordon: make sure no edit fields are left hanging
		for(i = 0; i < menu->itemCount; i++)
		{
			if(g_editItem == menu->items[i])
			{
				g_editingField = qfalse;
				g_editItem = NULL;
			}
		}


		menu->cursorItem = -1;
		Menu_ClearFocus(menu);
		Menu_RunCloseScript(menu);
		menu->window.flags &= ~(WINDOW_VISIBLE | WINDOW_HASFOCUS | WINDOW_MOUSEOVER);
		if(menu->window.flags & WINDOW_MODAL)
		{
			if(modalMenuCount <= 0)
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: tried closing a modal window with an empty modal stack!\n");
			}
			else
			{
				modalMenuCount--;
				// if modal doesn't have a parent, the stack item may be NULL .. just go back to the main menu then
				if(modalMenuStack[modalMenuCount])
				{
					Menus_ActivateByName(modalMenuStack[modalMenuCount]->window.name, qfalse);	// don't try to push the one we are opening to the stack
				}
			}
		}
	}
}

void Menus_CloseAll()
{
	int             i;

	for(i = 0; i < menuCount; i++)
	{
		Menu_RunCloseScript(&Menus[i]);
		Menus[i].window.flags &= ~(WINDOW_HASFOCUS | WINDOW_VISIBLE | WINDOW_MOUSEOVER);
	}
}


void Script_Show(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;

	if(String_Parse(args, &name))
	{
		Menu_ShowItemByName(item->parent, name, qtrue);
	}
}

void Script_Hide(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;

	if(String_Parse(args, &name))
	{
		Menu_ShowItemByName(item->parent, name, qfalse);
	}
}

void Script_FadeIn(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;

	if(String_Parse(args, &name))
	{
		Menu_FadeItemByName(item->parent, name, qfalse);
	}
}

void Script_FadeOut(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;

	if(String_Parse(args, &name))
	{
		Menu_FadeItemByName(item->parent, name, qtrue);
	}
}

void Script_Open(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;

	if(String_Parse(args, &name))
	{
		Menus_OpenByName(name);
	}
}

void Menu_FadeMenuByName(const char *p, qboolean * bAbort, qboolean fadeOut)
{
	itemDef_t      *item;
	int             i;
	menuDef_t      *menu = Menus_FindByName(p);

	if(menu)
	{
		for(i = 0; i < menu->itemCount; i++)
		{
			item = menu->items[i];
			if(fadeOut)
			{
				item->window.flags |= (WINDOW_FADINGOUT | WINDOW_VISIBLE);
				item->window.flags &= ~WINDOW_FADINGIN;
			}
			else
			{
				item->window.flags |= (WINDOW_VISIBLE | WINDOW_FADINGIN);
				item->window.flags &= ~WINDOW_FADINGOUT;
			}
		}
	}
}

void Script_FadeInMenu(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;

	if(String_Parse(args, &name))
	{
		Menu_FadeMenuByName(name, bAbort, qfalse);
	}
}

void Script_FadeOutMenu(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;

	if(String_Parse(args, &name))
	{
		Menu_FadeMenuByName(name, bAbort, qtrue);
	}
}

// DHM - Nerve

void Script_ConditionalOpen(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *cvar = NULL;
	const char     *name1 = NULL;
	const char     *name2 = NULL;
	float           val;
	char            buff[1024];
	int             testtype;	// 0: check val not 0

	// 1: check cvar not empty

	if(String_Parse(args, &cvar) && Int_Parse(args, &testtype) && String_Parse(args, &name1) && String_Parse(args, &name2))
	{

		switch (testtype)
		{
			default:
			case 0:
				val = DC->getCVarValue(cvar);
				if(val == 0.f)
				{
					Menus_OpenByName(name2);
				}
				else
				{
					Menus_OpenByName(name1);
				}
				break;
			case 1:
				DC->getCVarString(cvar, buff, sizeof(buff));
				if(!buff[0])
				{
					Menus_OpenByName(name2);
				}
				else
				{
					Menus_OpenByName(name1);
				}
				break;
		}
	}
}

void Script_ConditionalScript(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *cvar;
	const char     *script1;
	const char     *script2;
	const char     *token;
	float           val;
	char            buff[1024];
	int             testtype;	// 0: check val not 0

	// 1: check cvar not empty
	int             testval;

	if(String_Parse(args, &cvar) &&
	   Int_Parse(args, &testtype) &&
	   String_Parse(args, &token) && (token && *token == '(') &&
	   String_Parse(args, &script1) &&
	   String_Parse(args, &token) && (token && *token == ')') &&
	   String_Parse(args, &token) && (token && *token == '(') &&
	   String_Parse(args, &script2) && String_Parse(args, &token) && (token && *token == ')'))
	{

		switch (testtype)
		{
			default:
			case 0:
				val = DC->getCVarValue(cvar);
				if(val == 0.f)
				{
					Item_RunScript(item, bAbort, script2);
				}
				else
				{
					Item_RunScript(item, bAbort, script1);
				}
				break;
			case 1:
				DC->getCVarString(cvar, buff, sizeof(buff));
				if(!buff[0])
				{
					Item_RunScript(item, bAbort, script2);
				}
				else
				{
					Item_RunScript(item, bAbort, script1);
				}
				break;
			case 3:
				if(Int_Parse(args, &testval))
				{
					val = DC->getCVarValue(cvar);
					if(val != testval)
					{
						Item_RunScript(item, bAbort, script2);
					}
					else
					{
						Item_RunScript(item, bAbort, script1);
					}
				}
				break;
			case 2:
				// special tests
				if(!Q_stricmp(cvar, "UIProfileIsActiveProfile"))
				{
					char            ui_profileStr[256];
					char            cl_profileStr[256];

					DC->getCVarString("ui_profile", ui_profileStr, sizeof(ui_profileStr));
					Q_CleanStr(ui_profileStr);
					Q_CleanDirName(ui_profileStr);

					DC->getCVarString("cl_profile", cl_profileStr, sizeof(cl_profileStr));

					if(!Q_stricmp(ui_profileStr, cl_profileStr))
					{
						Item_RunScript(item, bAbort, script1);
					}
					else
					{
						Item_RunScript(item, bAbort, script2);
					}
				}
				else if(!Q_stricmp(cvar, "UIProfileValidName"))
				{
					char            ui_profileStr[256];
					char            ui_profileCleanedStr[256];

					DC->getCVarString("ui_profile", ui_profileStr, sizeof(ui_profileStr));
					Q_strncpyz(ui_profileCleanedStr, ui_profileStr, sizeof(ui_profileCleanedStr));
					Q_CleanStr(ui_profileCleanedStr);
					Q_CleanDirName(ui_profileCleanedStr);

					if(*ui_profileStr && *ui_profileCleanedStr)
					{
						Item_RunScript(item, bAbort, script1);
					}
					else
					{
						Item_RunScript(item, bAbort, script2);
					}

				}
				else if(!Q_stricmp(cvar, "UIProfileAlreadyExists"))
				{
					char            ui_profileCleanedStr[256];
					qboolean        alreadyExists = qfalse;
					fileHandle_t    f;

					DC->getCVarString("ui_profile", ui_profileCleanedStr, sizeof(ui_profileCleanedStr));
					Q_CleanStr(ui_profileCleanedStr);
					Q_CleanDirName(ui_profileCleanedStr);

					if(trap_FS_FOpenFile(va("profiles/%s/profile.dat", ui_profileCleanedStr), &f, FS_READ) >= 0)
					{
						alreadyExists = qtrue;
						trap_FS_FCloseFile(f);
					}

					if(alreadyExists)
					{
						Item_RunScript(item, bAbort, script1);
					}
					else
					{
						Item_RunScript(item, bAbort, script2);
					}
				}
				else if(!Q_stricmp(cvar, "UIProfileAlreadyExists_Rename"))
				{
					char            ui_profileCleanedStr[256];
					qboolean        alreadyExists = qfalse;
					fileHandle_t    f;

					DC->getCVarString("ui_profile_renameto", ui_profileCleanedStr, sizeof(ui_profileCleanedStr));
					Q_CleanStr(ui_profileCleanedStr);
					Q_CleanDirName(ui_profileCleanedStr);

					if(trap_FS_FOpenFile(va("profiles/%s/profile.dat", ui_profileCleanedStr), &f, FS_READ) >= 0)
					{
						alreadyExists = qtrue;
						trap_FS_FCloseFile(f);
					}

					if(alreadyExists)
					{
						Item_RunScript(item, bAbort, script1);
					}
					else
					{
						Item_RunScript(item, bAbort, script2);
					}
				}
				else if(!Q_stricmp(cvar, "ReadyToCreateProfile"))
				{
					char            ui_profileStr[256], ui_profileCleanedStr[256];
					int             ui_rate;
					qboolean        alreadyExists = qfalse;
					fileHandle_t    f;

					DC->getCVarString("ui_profile", ui_profileStr, sizeof(ui_profileStr));

					Q_strncpyz(ui_profileCleanedStr, ui_profileStr, sizeof(ui_profileCleanedStr));
					Q_CleanStr(ui_profileCleanedStr);
					Q_CleanDirName(ui_profileCleanedStr);

					if(trap_FS_FOpenFile(va("profiles/%s/profile.dat", ui_profileCleanedStr), &f, FS_READ) >= 0)
					{
						alreadyExists = qtrue;
						trap_FS_FCloseFile(f);
					}

					ui_rate = (int)DC->getCVarValue("ui_rate");

					if(!alreadyExists && *ui_profileStr && ui_rate > 0)
					{
						Item_RunScript(item, bAbort, script1);
					}
					else
					{
						Item_RunScript(item, bAbort, script2);
					}
				}
				else if(!Q_stricmp(cvar, "systemrestartIsRequired"))
				{
					int				ui_com_hunkmegs = DC->getCVarValue("ui_com_hunkmegs");
					int				ui_com_soundmegs = DC->getCVarValue("ui_com_soundmegs");
					int				ui_com_zonemegs = DC->getCVarValue("ui_com_zonemegs");
					int             ui_s_khz = DC->getCVarValue("ui_s_khz");

					int				com_hunkmegs = DC->getCVarValue("com_hunkmegs");
					int				com_soundmegs = DC->getCVarValue("com_soundmegs");
					int				com_zonemegs = DC->getCVarValue("com_zonemegs");
					int				s_khz = DC->getCVarValue("s_khz");
					
					if(ui_com_hunkmegs != com_hunkmegs ||
						ui_com_soundmegs != com_soundmegs ||
						ui_com_zonemegs != com_zonemegs ||
						ui_s_khz != s_khz)
					{
						Item_RunScript(item, bAbort, script1);
					}
					else
					{
						Item_RunScript(item, bAbort, script2);
					}
				}
				else if(!Q_stricmp(cvar, "vidrestartIsRequired"))
				{
					int             ui_r_mode = DC->getCVarValue("ui_r_mode");
					int             ui_r_colorbits = DC->getCVarValue("ui_r_colorbits");
					int             ui_r_fullscreen = DC->getCVarValue("ui_r_fullscreen");
					int             ui_r_texturebits = DC->getCVarValue("ui_r_texturebits");
					int             ui_r_depthbits = DC->getCVarValue("ui_r_depthbits");
					int             ui_r_ext_compressed_textures = DC->getCVarValue("ui_r_ext_compressed_textures");
					int             ui_r_allowextensions = DC->getCVarValue("ui_r_allowextensions");
					int             ui_r_detailtextures = DC->getCVarValue("ui_r_detailtextures");
					int             ui_r_subdivisions = DC->getCVarValue("ui_r_subdivisions");
					char            ui_r_texturemode[MAX_CVAR_VALUE_STRING];
					int				ui_cg_shadows = DC->getCVarValue("ui_cg_shadows");
					int				ui_r_hdrrendering = DC->getCVarValue("ui_r_hdrrendering");
					int				ui_r_bloom = DC->getCVarValue("ui_r_bloom");
					int				ui_r_normalmapping = DC->getCVarValue("ui_r_normalmapping");
					int				ui_r_parallaxmapping = DC->getCVarValue("ui_r_parallaxmapping");

					int             r_mode = DC->getCVarValue("r_mode");
					int             r_colorbits = DC->getCVarValue("r_colorbits");
					int             r_fullscreen = DC->getCVarValue("r_fullscreen");
					int             r_texturebits = DC->getCVarValue("r_texturebits");
					int             r_depthbits = DC->getCVarValue("r_depthbits");
					int             r_ext_compressed_textures = DC->getCVarValue("r_ext_compressed_textures");
					int             r_allowextensions = DC->getCVarValue("r_allowextensions");
					int             r_detailtextures = DC->getCVarValue("r_detailtextures");
					int             r_subdivisions = DC->getCVarValue("r_subdivisions");
					char            r_texturemode[MAX_CVAR_VALUE_STRING];
					int				cg_shadows = DC->getCVarValue("cg_shadows");
					int				r_hdrrendering = DC->getCVarValue("r_hdrrendering");
					int				r_bloom	= DC->getCVarValue("r_bloom");
					int				r_normalmapping	= DC->getCVarValue("r_normalmapping");
					int				r_parallaxmapping = DC->getCVarValue("r_parallaxmapping");

					trap_Cvar_VariableStringBuffer("ui_r_texturemode", ui_r_texturemode, sizeof(ui_r_texturemode));
					trap_Cvar_VariableStringBuffer("r_texturemode", r_texturemode, sizeof(r_texturemode));

					if(ui_r_subdivisions != r_subdivisions ||
					   ui_r_mode != r_mode ||
					   ui_r_colorbits != r_colorbits ||
					   ui_r_fullscreen != r_fullscreen ||
					   ui_r_texturebits != r_texturebits ||
					   ui_r_depthbits != r_depthbits ||
					   ui_r_ext_compressed_textures != r_ext_compressed_textures ||
					   ui_r_allowextensions != r_allowextensions ||
					   ui_r_detailtextures != r_detailtextures ||
					   Q_stricmp(r_texturemode, ui_r_texturemode) ||
					   ui_cg_shadows != cg_shadows || ui_r_hdrrendering != r_hdrrendering ||
					   ui_r_bloom != r_bloom || ui_r_normalmapping != r_normalmapping ||
					   ui_r_parallaxmapping != r_parallaxmapping)
					{
						Item_RunScript(item, bAbort, script1);
					}
					else
					{
						Item_RunScript(item, bAbort, script2);
					}
					/*} else if( !Q_stricmpn( cvar, "voteflags", 9 ) ) {
					   char info[MAX_INFO_STRING];
					   int voteflags = atoi(cvar + 9);

					   trap_Cvar_VariableStringBuffer( "cg_ui_voteFlags", info, sizeof(info) );

					   if( (atoi(info) & item->voteFlag) != item->voteFlag ) {
					   Item_RunScript( item, bAbort, script1 );
					   } else {
					   Item_RunScript( item, bAbort, script2 );
					   } */
#ifndef CGAMEDLL
				}
				else if(!Q_stricmpn(cvar, "serversort_", 11))
				{
					int             sorttype = atoi(cvar + 11);

					if(sorttype != uiInfo.serverStatus.sortKey)
					{
						Item_RunScript(item, bAbort, script2);
					}
					else
					{
						Item_RunScript(item, bAbort, script1);
					}
				}
				else if(!Q_stricmp(cvar, "ValidReplaySelected"))
				{
					if(uiInfo.demoIndex >= 0 && uiInfo.demoIndex < uiInfo.demoCount)
					{
						Item_RunScript(item, bAbort, script1);
					}
					else
					{
						Item_RunScript(item, bAbort, script2);
					}
#endif							// !CGAMEDLL
				}
				else if(!Q_stricmp(cvar, "ROldModeCheck"))
				{
					char            r_oldModeStr[256];
					int             r_oldMode;
					int             r_mode = DC->getCVarValue("r_mode");

					DC->getCVarString("r_oldMode", r_oldModeStr, sizeof(r_oldModeStr));
					r_oldMode = atoi(r_oldModeStr);

					if(*r_oldModeStr && r_oldMode != r_mode)
					{
						Item_RunScript(item, bAbort, script1);
					}
					else
					{
						if(r_oldMode == r_mode)
						{
							trap_Cvar_Set("r_oldMode", "");	// clear it
						}
						Item_RunScript(item, bAbort, script2);
					}
				}
				break;
		}
	}
}

// DHM - Nerve

void Script_Close(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;

	if(String_Parse(args, &name))
	{
		Menus_CloseByName(name);
	}
}

void Script_CloseAll(itemDef_t * item, qboolean * bAbort, char **args)
{
	Menus_CloseAll();
}

void Script_CloseAllOtherMenus(itemDef_t * item, qboolean * bAbort, char **args)
{
	int             i;

	for(i = 0; i < menuCount; i++)
	{
		if(&Menus[i] == item->parent)
		{
			continue;
		}
		Menu_RunCloseScript(&Menus[i]);
		Menus[i].window.flags &= ~(WINDOW_HASFOCUS | WINDOW_VISIBLE | WINDOW_MOUSEOVER);
	}
}

/*
==============
Script_Clipboard
==============
*/
void Script_Clipboard(itemDef_t * item, qboolean * bAbort, char **args)
{
}

/*
==============
Script_NotebookShowpage
	hide all notebook pages and show just the active one

	inc == 0	- show current page
	inc == val	- turn inc pages in the notebook (negative numbers are backwards)
	inc == 999	- key number.  +999 is jump to last page, -999 is jump to cover page
==============
*/
void Script_NotebookShowpage(itemDef_t * item, qboolean * bAbort, char **args)
{
}



void Menu_TransitionItemByName(menuDef_t * menu, const char *p, rectDef_t rectFrom, rectDef_t rectTo, int time, float amt)
{
	itemDef_t      *item;
	int             i;
	int             count = Menu_ItemsMatchingGroup(menu, p);

	for(i = 0; i < count; i++)
	{
		item = Menu_GetMatchingItemByNumber(menu, i, p);
		if(item != NULL)
		{
			item->window.flags |= (WINDOW_INTRANSITION | WINDOW_VISIBLE);
			item->window.offsetTime = time;
			memcpy(&item->window.rectClient, &rectFrom, sizeof(rectDef_t));
			memcpy(&item->window.rectEffects, &rectTo, sizeof(rectDef_t));
			item->window.rectEffects2.x = abs(rectTo.x - rectFrom.x) / amt;
			item->window.rectEffects2.y = abs(rectTo.y - rectFrom.y) / amt;
			item->window.rectEffects2.w = abs(rectTo.w - rectFrom.w) / amt;
			item->window.rectEffects2.h = abs(rectTo.h - rectFrom.h) / amt;
			Item_UpdatePosition(item);
		}
	}
}


void Script_Transition(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;
	rectDef_t       rectFrom, rectTo;
	int             time = 0;
	float           amt = 0.0f;

	if(String_Parse(args, &name))
	{
		if(Rect_Parse(args, &rectFrom) && Rect_Parse(args, &rectTo) && Int_Parse(args, &time) && Float_Parse(args, &amt))
		{
			Menu_TransitionItemByName(item->parent, name, rectFrom, rectTo, time, amt);
		}
	}
}


void Menu_OrbitItemByName(menuDef_t * menu, const char *p, float x, float y, float cx, float cy, int time)
{
	itemDef_t      *item;
	int             i;
	int             count = Menu_ItemsMatchingGroup(menu, p);

	for(i = 0; i < count; i++)
	{
		item = Menu_GetMatchingItemByNumber(menu, i, p);
		if(item != NULL)
		{
			item->window.flags |= (WINDOW_ORBITING | WINDOW_VISIBLE);
			item->window.offsetTime = time;
			item->window.rectEffects.x = cx;
			item->window.rectEffects.y = cy;
			item->window.rectClient.x = x;
			item->window.rectClient.y = y;
			Item_UpdatePosition(item);
		}
	}
}


void Script_Orbit(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;
	float           cx = 0.0f, cy = 0.0f, x = 0.0f, y = 0.0f;
	int             time = 0;

	if(String_Parse(args, &name))
	{
		if(Float_Parse(args, &x) && Float_Parse(args, &y) && Float_Parse(args, &cx) && Float_Parse(args, &cy) &&
		   Int_Parse(args, &time))
		{
			Menu_OrbitItemByName(item->parent, name, x, y, cx, cy, time);
		}
	}
}



void Script_SetFocus(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;
	itemDef_t      *focusItem;

	if(String_Parse(args, &name))
	{
		focusItem = Menu_FindItemByName(item->parent, name);
		if(focusItem && !(focusItem->window.flags & WINDOW_DECORATION) && !(focusItem->window.flags & WINDOW_HASFOCUS))
		{
			Menu_ClearFocus(item->parent);
			focusItem->window.flags |= WINDOW_HASFOCUS;
			if(focusItem->onFocus)
			{
				Item_RunScript(focusItem, NULL, focusItem->onFocus);
			}
			if(DC->Assets.itemFocusSound)
			{
				DC->startLocalSound(DC->Assets.itemFocusSound, CHAN_LOCAL_SOUND);
			}
		}
	}
}

void Script_ClearFocus(itemDef_t * item, qboolean * bAbort, char **args)
{
	Menu_ClearFocus(item->parent);
}

void Script_SetPlayerModel(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;

	if(String_Parse(args, &name))
	{
		DC->setCVar("team_model", name);
	}
}

void Script_SetPlayerHead(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;

	if(String_Parse(args, &name))
	{
		DC->setCVar("team_headmodel", name);
	}
}

// ATVI Wolfenstein Misc #304
// the parser misreads setCvar "bleh" ""
// you have to use clearCvar "bleh"
void Script_ClearCvar(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *cvar;

	if(String_Parse(args, &cvar))
	{
		DC->setCVar(cvar, "");
	}
}

void Script_SetCvar(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *cvar = NULL, *val = NULL;

	if(String_Parse(args, &cvar) && String_Parse(args, &val))
	{
		DC->setCVar(cvar, val);
	}
}

void Script_CopyCvar(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *cvar_src = NULL, *cvar_dst = NULL;

	if(String_Parse(args, &cvar_src) && String_Parse(args, &cvar_dst))
	{
		char            buff[256];

		DC->getCVarString(cvar_src, buff, 256);
		DC->setCVar(cvar_dst, buff);
	}
}

void Script_Exec(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *val = NULL;

	if(String_Parse(args, &val))
	{
		DC->executeText(EXEC_APPEND, va("%s ; ", val));
	}
}

void Script_ExecNOW(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *val = NULL;

	if(String_Parse(args, &val))
	{
		DC->executeText(EXEC_NOW, va("%s ; ", val));
	}
}

void Script_Play(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *val = NULL;

	if(String_Parse(args, &val))
	{
		DC->startLocalSound(DC->registerSound(val, qfalse), CHAN_LOCAL_SOUND);	// all sounds are not 3d
	}
}

void Script_playLooped(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *val = NULL;

	if(String_Parse(args, &val))
	{
		DC->stopBackgroundTrack();
		DC->startBackgroundTrack(val, val);
	}
}

// NERVE - SMF
void Script_AddListItem(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *itemname = NULL, *val = NULL, *name = NULL;
	itemDef_t      *t;

	if(String_Parse(args, &itemname) && String_Parse(args, &val) && String_Parse(args, &name))
	{
		t = Menu_FindItemByName(item->parent, itemname);
		if(t && t->special)
		{
			DC->feederAddItem(t->special, name, atoi(val));
		}
	}
}

// -NERVE - SMF
// DHM - Nerve
void Script_CheckAutoUpdate(itemDef_t * item, qboolean * bAbort, char **args)
{
	DC->checkAutoUpdate();
}

void Script_GetAutoUpdate(itemDef_t * item, qboolean * bAbort, char **args)
{
	DC->getAutoUpdate();
}

// DHM - Nerve

void Script_SetMenuFocus(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name;

	if(String_Parse(args, &name))
	{
		menuDef_t      *focusMenu = Menus_FindByName(name);

		if(focusMenu && !(focusMenu->window.flags & WINDOW_HASFOCUS))
		{
			Menu_ClearFocus(item->parent);
			focusMenu->window.flags |= WINDOW_HASFOCUS;
		}
	}
}

qboolean FileExists(char *filename)
{
	fileHandle_t    f;

	if(trap_FS_FOpenFile(filename, &f, FS_READ) < 0)
	{
		trap_FS_FCloseFile(f);
		return qfalse;
	}
	else
	{
		trap_FS_FCloseFile(f);
		return qtrue;
	}
}

qboolean Script_CheckProfile(char *profile_path)
{
	fileHandle_t    f;
	char            f_data[32];
	int             f_pid;
	char            com_pid[256];
	int             pid;

	if(trap_FS_FOpenFile(profile_path, &f, FS_READ) < 0)
	{
		//no profile found, we're ok
		return qtrue;
	}

	trap_FS_Read(&f_data, sizeof(f_data) - 1, f);

	DC->getCVarString("com_pid", com_pid, sizeof(com_pid));
	pid = atoi(com_pid);

	f_pid = atoi(f_data);
	if(f_pid != pid)
	{
		//pid doesn't match
		trap_FS_FCloseFile(f);
		return qfalse;
	}

	//we're all ok
	trap_FS_FCloseFile(f);
	return qtrue;
}

qboolean Script_WriteProfile(char *profile_path)
{
	fileHandle_t    f;
	char            com_pid[256];

	if(FileExists(profile_path))
	{
		trap_FS_Delete(profile_path);
	}

	if(trap_FS_FOpenFile(profile_path, &f, FS_WRITE) < 0)
	{
		Com_Printf("Script_WriteProfile: Can't write %s.\n", profile_path);
		return qfalse;
	}
	if(f < 0)
	{
		Com_Printf("Script_WriteProfile: Can't write %s.\n", profile_path);
		return qfalse;
	}

	DC->getCVarString("com_pid", com_pid, sizeof(com_pid));

	trap_FS_Write(com_pid, strlen(com_pid), f);

	trap_FS_FCloseFile(f);

	return qtrue;
}

void Script_ExecWolfConfig(itemDef_t * item, qboolean * bAbort, char **args)
{
	char            cl_profileStr[256];
	int             useprofile = 1;

	if(Int_Parse(args, &useprofile))
	{

		DC->getCVarString("cl_profile", cl_profileStr, sizeof(cl_profileStr));

		if(useprofile && cl_profileStr[0])
		{
			if(!Script_CheckProfile(va("profiles/%s/profile.pid", cl_profileStr)))
			{
#ifndef _DEBUG
				Com_Printf("^3WARNING: profile.pid found for profile '%s' - not executing %s\n", cl_profileStr, CONFIG_NAME);
#else
				DC->executeText(EXEC_NOW, va("exec profiles/%s/%s\n", cl_profileStr, CONFIG_NAME));
#endif							// _DEBUG
			}
			else
			{
				DC->executeText(EXEC_NOW, va("exec profiles/%s/%s\n", cl_profileStr, CONFIG_NAME));

				if(!Script_WriteProfile(va("profiles/%s/profile.pid", cl_profileStr)))
				{
					Com_Printf("^3WARNING: couldn't write profiles/%s/profile.pid\n", cl_profileStr);
				}
			}
		}
		else
		{
			DC->executeText(EXEC_NOW, va("exec %s\n", CONFIG_NAME));
		}
	}
}

void Script_SetEditFocus(itemDef_t * item, qboolean * bAbort, char **args)
{
	const char     *name = NULL;
	itemDef_t      *editItem;

	if(String_Parse(args, &name))
	{
		editItem = Menu_FindItemByName(item->parent, name);
		if(editItem && (editItem->type == ITEM_TYPE_EDITFIELD || editItem->type == ITEM_TYPE_NUMERICFIELD))
		{
			editFieldDef_t *editPtr = (editFieldDef_t *) editItem->typeData;

			Menu_ClearFocus(item->parent);
			editItem->window.flags |= WINDOW_HASFOCUS;
			if(editItem->onFocus)
			{
				Item_RunScript(editItem, NULL, editItem->onFocus);
			}
			if(DC->Assets.itemFocusSound)
			{
				DC->startLocalSound(DC->Assets.itemFocusSound, CHAN_LOCAL_SOUND);
			}

			// NERVE - SMF - reset scroll offset so we can see what we're editing
			if(editPtr)
			{
				editPtr->paintOffset = 0;
			}

			editItem->cursorPos = 0;
			g_editingField = qtrue;
			g_editItem = editItem;

			// the stupidest idea ever, let's just override the console, every ui element, user choice, etc
			// nuking this
			//% DC->setOverstrikeMode(qtrue);
		}
	}
}

void Script_Abort(itemDef_t * item, qboolean * bAbort, char **args)
{
	*bAbort = qtrue;
}


commandDef_t    commandList[] = {
	{"fadein", &Script_FadeIn},	// group/name
	{"fadeout", &Script_FadeOut},	// group/name
	{"show", &Script_Show},		// group/name
	{"hide", &Script_Hide},		// group/name
	{"setcolor", &Script_SetColor},	// works on this
	{"open", &Script_Open},		// menu
	{"fadeinmenu", &Script_FadeInMenu},	// menu
	{"fadeoutmenu", &Script_FadeOutMenu},	// menu

	{"conditionalopen", &Script_ConditionalOpen},	// DHM - Nerve:: cvar menu menu
	// opens first menu if cvar is true[non-zero], second if false
	{"conditionalscript", &Script_ConditionalScript},	// as conditonalopen, but then executes scripts

	{"close", &Script_Close},	// menu
	{"closeall", &Script_CloseAll},
	{"closeallothermenus", &Script_CloseAllOtherMenus},

	{"clipboard", &Script_Clipboard},	// show the current clipboard group by name
	{"showpage", &Script_NotebookShowpage},	//
	{"setasset", &Script_SetAsset},	// works on this
	{"setbackground", &Script_SetBackground},	// works on this
	{"setitemcolor", &Script_SetItemColor},	// group/name
	{"setmenuitemcolor", &Script_SetMenuItemColor},	// group/name
	{"setteamcolor", &Script_SetTeamColor},	// sets this background color to team color
	{"setfocus", &Script_SetFocus},	// sets this background color to team color
	{"clearfocus", &Script_ClearFocus},
	{"setplayermodel", &Script_SetPlayerModel},	// sets this background color to team color
	{"setplayerhead", &Script_SetPlayerHead},	// sets this background color to team color
	{"transition", &Script_Transition},	// group/name
	{"setcvar", &Script_SetCvar},	// group/name
	{"clearcvar", &Script_ClearCvar},
	{"copycvar", &Script_CopyCvar},
	{"exec", &Script_Exec},		// group/name
	{"execnow", &Script_ExecNOW},	// group/name
	{"play", &Script_Play},		// group/name
	{"playlooped", &Script_playLooped},	// group/name
	{"orbit", &Script_Orbit},	// group/name
	{"addlistitem", &Script_AddListItem},	// NERVE - SMF - special command to add text items to list box
	{"checkautoupdate", &Script_CheckAutoUpdate},	// DHM - Nerve
	{"getautoupdate", &Script_GetAutoUpdate},	// DHM - Nerve
	{"setmenufocus", &Script_SetMenuFocus},	// focus menu
	{"execwolfconfig", &Script_ExecWolfConfig},	// executes etconfig.cfg
	{"setEditFocus", &Script_SetEditFocus},
	{"abort", &Script_Abort},
};

int             scriptCommandCount = sizeof(commandList) / sizeof(commandDef_t);


void Item_RunScript(itemDef_t * item, qboolean * bAbort, const char *s)
{
	char            script[4096], *p;
	int             i;
	qboolean        bRan;
	qboolean        b_localAbort = qfalse;

	memset(script, 0, sizeof(script));
	if(item && s && s[0])
	{
		Q_strcat(script, 4096, s);
		p = script;
		while(1)
		{
			const char     *command = NULL;

			// expect command then arguments, ; ends command, NULL ends script
			if(!String_Parse(&p, &command))
			{
				return;
			}

			if(command[0] == ';' && command[1] == '\0')
			{
				continue;
			}

			bRan = qfalse;
			for(i = 0; i < scriptCommandCount; i++)
			{
				if(Q_stricmp(command, commandList[i].name) == 0)
				{
					(commandList[i].handler(item, &b_localAbort, &p));
					bRan = qtrue;

					if(b_localAbort)
					{
						if(bAbort)
						{
							*bAbort = b_localAbort;
						}
						return;
					}
					break;
				}
			}
			// not in our auto list, pass to handler
			if(!bRan)
			{
				DC->runScript(&p);
			}
		}
	}
}


qboolean Item_EnableShowViaCvar(itemDef_t * item, int flag)
{
	char            script[1024], *p;

	memset(script, 0, sizeof(script));
	if(item && item->enableCvar && *item->enableCvar && item->cvarTest && *item->cvarTest)
	{
		char            buff[1024];

		DC->getCVarString(item->cvarTest, buff, sizeof(buff));

		Q_strcat(script, 1024, item->enableCvar);
		p = script;
		while(1)
		{
			const char     *val = NULL;

			// expect value then ; or NULL, NULL ends list
			if(!String_Parse(&p, &val))
			{
				return (item->cvarFlags & flag) ? qfalse : qtrue;
			}

			if(val[0] == ';' && val[1] == '\0')
			{
				continue;
			}

			// enable it if any of the values are true
			if(item->cvarFlags & flag)
			{
				if(Q_stricmp(buff, val) == 0)
				{
					return qtrue;
				}
			}
			else
			{
				// disable it if any of the values are true
				if(Q_stricmp(buff, val) == 0)
				{
					return qfalse;
				}
			}

		}
		return (item->cvarFlags & flag) ? qfalse : qtrue;
	}
	return qtrue;
}


// OSP - display if we poll on a server toggle setting
// We want *current* settings, so this is a bit of a perf hit,
// but this is only during UI display

qboolean Item_SettingShow(itemDef_t * item, qboolean fVoteTest)
{
	char            info[MAX_INFO_STRING];

	if(fVoteTest)
	{
		trap_Cvar_VariableStringBuffer("cg_ui_voteFlags", info, sizeof(info));
		return ((atoi(info) & item->voteFlag) != item->voteFlag);
	}

	DC->getConfigString(CS_SERVERTOGGLES, info, sizeof(info));

	if(item->settingFlags & SVS_ENABLED_SHOW)
	{
		return (atoi(info) & item->settingTest);
	}
	if(item->settingFlags & SVS_DISABLED_SHOW)
	{
		return (!(atoi(info) & item->settingTest));
	}

	return (qtrue);
}


// will optionaly set focus to this item
qboolean Item_SetFocus(itemDef_t * item, float x, float y)
{
	int             i;
	itemDef_t      *oldFocus;
	sfxHandle_t    *sfx = &DC->Assets.itemFocusSound;
	qboolean        playSound = qfalse;
	menuDef_t      *parent;		// bk001206: = (menuDef_t*)item->parent;

	// sanity check, non-null, not a decoration and does not already have the focus
	if(item == NULL || item->window.flags & WINDOW_DECORATION || item->window.flags & WINDOW_HASFOCUS ||
	   !(item->window.flags & WINDOW_VISIBLE))
	{
		return qfalse;
	}

	// bk001206 - this can be NULL.
	parent = (menuDef_t *) item->parent;

	// items can be enabled and disabled based on cvars
	if(item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE))
	{
		return qfalse;
	}

	if(item->cvarFlags & (CVAR_SHOW | CVAR_HIDE) && !Item_EnableShowViaCvar(item, CVAR_SHOW))
	{
		return qfalse;
	}

	// OSP
	if((item->settingFlags & (SVS_ENABLED_SHOW | SVS_DISABLED_SHOW)) && !Item_SettingShow(item, qfalse))
	{
		return (qfalse);
	}
	if(item->voteFlag != 0 && !Item_SettingShow(item, qtrue))
	{
		return (qfalse);
	}

	oldFocus = Menu_ClearFocus(item->parent);

	if(item->type == ITEM_TYPE_TEXT)
	{
		rectDef_t       r;

		r = item->textRect;
		r.y -= r.h;
		if(Rect_ContainsPoint(&r, x, y))
		{
			item->window.flags |= WINDOW_HASFOCUS;
			if(item->focusSound)
			{
				sfx = &item->focusSound;
			}
			playSound = qtrue;
		}
		else
		{
			if(oldFocus)
			{
				oldFocus->window.flags |= WINDOW_HASFOCUS;
				if(oldFocus->onFocus)
				{
					Item_RunScript(oldFocus, NULL, oldFocus->onFocus);
				}
			}
		}
	}
	else
	{
		item->window.flags |= WINDOW_HASFOCUS;
		if(item->onFocus)
		{
			Item_RunScript(item, NULL, item->onFocus);
		}
		if(item->focusSound)
		{
			sfx = &item->focusSound;
		}
		playSound = qtrue;
	}

	if(playSound && sfx)
	{
		DC->startLocalSound(*sfx, CHAN_LOCAL_SOUND);
	}

	for(i = 0; i < parent->itemCount; i++)
	{
		if(parent->items[i] == item)
		{
			parent->cursorItem = i;
			break;
		}
	}

	return qtrue;
}

int Item_ListBox_MaxScroll(itemDef_t * item)
{
	listBoxDef_t   *listPtr = (listBoxDef_t *) item->typeData;
	int             count = DC->feederCount(item->special);
	int             max;

	if(item->window.flags & WINDOW_HORIZONTAL)
	{
		max = count - (int)(item->window.rect.w / listPtr->elementWidth);
	}
	else
	{
		max = count - (int)(item->window.rect.h / listPtr->elementHeight);
	}
	if(max < 0)
	{
		return 0;
	}
	return max;
}

int Item_ListBox_ThumbPosition(itemDef_t * item)
{
	float           max, pos, size;
	listBoxDef_t   *listPtr = (listBoxDef_t *) item->typeData;

	max = Item_ListBox_MaxScroll(item);
	if(item->window.flags & WINDOW_HORIZONTAL)
	{
		size = item->window.rect.w - (SCROLLBAR_SIZE * 2) - 2;
		if(max > 0)
		{
			pos = (size - SCROLLBAR_SIZE) / (float)max;
		}
		else
		{
			pos = 0;
		}
		pos *= listPtr->startPos;
		return item->window.rect.x + 1 + SCROLLBAR_SIZE + pos;
	}
	else
	{
		size = item->window.rect.h - (SCROLLBAR_SIZE * 2) - 2;
		if(max > 0)
		{
			pos = (size - SCROLLBAR_SIZE) / (float)max;
		}
		else
		{
			pos = 0;
		}
		pos *= listPtr->startPos;

		return item->window.rect.y + 1 + SCROLLBAR_SIZE + pos;
	}
}

int Item_ListBox_ThumbDrawPosition(itemDef_t * item)
{
	int             min, max;

	if(itemCapture == item)
	{
		if(item->window.flags & WINDOW_HORIZONTAL)
		{
			min = item->window.rect.x + SCROLLBAR_SIZE + 1;
			max = item->window.rect.x + item->window.rect.w - 2 * SCROLLBAR_SIZE - 1;
			if(DC->cursorx >= min + SCROLLBAR_SIZE / 2 && DC->cursorx <= max + SCROLLBAR_SIZE / 2)
			{
				return DC->cursorx - SCROLLBAR_SIZE / 2;
			}
			else
			{
				return Item_ListBox_ThumbPosition(item);
			}
		}
		else
		{
			min = item->window.rect.y + SCROLLBAR_SIZE + 1;
			max = item->window.rect.y + item->window.rect.h - 2 * SCROLLBAR_SIZE - 1;
			if(DC->cursory >= min + SCROLLBAR_SIZE / 2 && DC->cursory <= max + SCROLLBAR_SIZE / 2)
			{
				return DC->cursory - SCROLLBAR_SIZE / 2;
			}
			else
			{
				return Item_ListBox_ThumbPosition(item);
			}
		}
	}
	else
	{
		return Item_ListBox_ThumbPosition(item);
	}
}

float Item_Slider_ThumbPosition(itemDef_t * item)
{
	float           value, range, x;
	editFieldDef_t *editDef = item->typeData;

	if(item->text)
	{
		x = item->textRect.x + item->textRect.w + 8;
	}
	else
	{
		x = item->window.rect.x;
	}

	if(editDef == NULL && item->cvar)
	{
		return x;
	}

	value = DC->getCVarValue(item->cvar);

	if(value < editDef->minVal)
	{
		value = editDef->minVal;
	}
	else if(value > editDef->maxVal)
	{
		value = editDef->maxVal;
	}

	range = editDef->maxVal - editDef->minVal;
	value -= editDef->minVal;
	value /= range;
	//value /= (editDef->maxVal - editDef->minVal);
	value *= SLIDER_WIDTH;
	x += value;
	// vm fuckage
	//x = x + (((float)value / editDef->maxVal) * SLIDER_WIDTH);
	return x;
}

int Item_Slider_OverSlider(itemDef_t * item, float x, float y)
{
	rectDef_t       r;

	r.x = Item_Slider_ThumbPosition(item) - (SLIDER_THUMB_WIDTH / 2);
	//r.y = item->window.rect.y - 2;
	r.y = item->window.rect.y;
	r.w = SLIDER_THUMB_WIDTH;
	r.h = SLIDER_THUMB_HEIGHT;

	if(Rect_ContainsPoint(&r, x, y))
	{
		return WINDOW_LB_THUMB;
	}
	return 0;
}

int Item_ListBox_OverLB(itemDef_t * item, float x, float y)
{
	rectDef_t       r;
	listBoxDef_t   *listPtr;
	int             thumbstart;
	int             count;

	count = DC->feederCount(item->special);
	listPtr = (listBoxDef_t *) item->typeData;
	if(item->window.flags & WINDOW_HORIZONTAL)
	{
		// check if on left arrow
		r.x = item->window.rect.x;
		r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		r.h = r.w = SCROLLBAR_SIZE;
		if(Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_LEFTARROW;
		}
		// check if on right arrow
		r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		if(Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_RIGHTARROW;
		}
		// check if on thumb
		//thumbstart = Item_ListBox_ThumbPosition(item);
		thumbstart = Item_ListBox_ThumbDrawPosition(item);
		r.x = thumbstart;
		if(Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_THUMB;
		}
		r.x = item->window.rect.x + SCROLLBAR_SIZE;
		r.w = thumbstart - r.x;
		if(Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_PGUP;
		}
		r.x = thumbstart + SCROLLBAR_SIZE;
		r.w = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		if(Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_PGDN;
		}

		// hack hack
		r.x = item->window.rect.x;
		r.w = item->window.rect.w;
		if(Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_SOMEWHERE;
		}
	}
	else
	{
		r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		r.y = item->window.rect.y;
		r.h = r.w = SCROLLBAR_SIZE;
		if(Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_LEFTARROW;
		}
		r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		if(Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_RIGHTARROW;
		}
		//thumbstart = Item_ListBox_ThumbPosition(item);
		thumbstart = Item_ListBox_ThumbDrawPosition(item);
		r.y = thumbstart;
		if(Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_THUMB;
		}
		r.y = item->window.rect.y + SCROLLBAR_SIZE;
		r.h = thumbstart - r.y;
		if(Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_PGUP;
		}
		r.y = thumbstart + SCROLLBAR_SIZE;
		r.h = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		if(Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_PGDN;
		}

		// hack hack
		r.y = item->window.rect.y;
		r.h = item->window.rect.h;
		if(Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_SOMEWHERE;
		}
	}
	return 0;
}


void Item_ListBox_MouseEnter(itemDef_t * item, float x, float y, qboolean click)
{
	rectDef_t       r;
	listBoxDef_t   *listPtr = (listBoxDef_t *) item->typeData;

	item->window.flags &=
		~(WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN | WINDOW_LB_SOMEWHERE);
	item->window.flags |= Item_ListBox_OverLB(item, x, y);

	if(click)
	{
		if(item->window.flags & WINDOW_HORIZONTAL)
		{
			if(!
			   (item->window.
				flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN |
						 WINDOW_LB_SOMEWHERE)))
			{
				// check for selection hit as we have exausted buttons and thumb
				if(listPtr->elementStyle == LISTBOX_IMAGE)
				{
					r.x = item->window.rect.x;
					r.y = item->window.rect.y;
					r.h = item->window.rect.h - SCROLLBAR_SIZE;
					r.w = item->window.rect.w - listPtr->drawPadding;
					if(Rect_ContainsPoint(&r, x, y))
					{
						listPtr->cursorPos = (int)((x - r.x) / listPtr->elementWidth) + listPtr->startPos;
						if(listPtr->cursorPos >= listPtr->endPos)
						{
							listPtr->cursorPos = listPtr->endPos;
						}
					}
				}
				else
				{
					// text hit..
				}
			}
		}
		else if(!
				(item->window.
				 flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN |
						  WINDOW_LB_SOMEWHERE)))
		{
			r.x = item->window.rect.x;
			r.y = item->window.rect.y;
			r.w = item->window.rect.w - SCROLLBAR_SIZE;
			r.h = item->window.rect.h - listPtr->drawPadding;
			if(Rect_ContainsPoint(&r, x, y))
			{
				listPtr->cursorPos = (int)((y - 2 - r.y) / listPtr->elementHeight) + listPtr->startPos;
				if(listPtr->cursorPos > listPtr->endPos)
				{
					listPtr->cursorPos = listPtr->endPos;
				}
			}
		}
	}
}

void Item_MouseEnter(itemDef_t * item, float x, float y)
{
	rectDef_t       r;

	if(item)
	{
		r = item->textRect;
		r.y -= r.h;
		// in the text rect?

		// items can be enabled and disabled based on cvars
		if(item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE))
		{
			return;
		}

		if(item->cvarFlags & (CVAR_SHOW | CVAR_HIDE) && !Item_EnableShowViaCvar(item, CVAR_SHOW))
		{
			return;
		}

		// OSP - server settings too .. (mostly for callvote)
		if((item->settingFlags & (SVS_ENABLED_SHOW | SVS_DISABLED_SHOW)) && !Item_SettingShow(item, qfalse))
		{
			return;
		}
		if(item->voteFlag != 0 && !Item_SettingShow(item, qtrue))
		{
			return;
		}

		if(Rect_ContainsPoint(&r, x, y))
		{
			if(!(item->window.flags & WINDOW_MOUSEOVERTEXT))
			{
				Item_RunScript(item, NULL, item->mouseEnterText);
				item->window.flags |= WINDOW_MOUSEOVERTEXT;
			}
			if(!(item->window.flags & WINDOW_MOUSEOVER))
			{
				Item_RunScript(item, NULL, item->mouseEnter);
				item->window.flags |= WINDOW_MOUSEOVER;
			}

		}
		else
		{
			// not in the text rect
			if(item->window.flags & WINDOW_MOUSEOVERTEXT)
			{
				// if we were
				Item_RunScript(item, NULL, item->mouseExitText);
				item->window.flags &= ~WINDOW_MOUSEOVERTEXT;
			}
			if(!(item->window.flags & WINDOW_MOUSEOVER))
			{
				Item_RunScript(item, NULL, item->mouseEnter);
				item->window.flags |= WINDOW_MOUSEOVER;
			}

			if(item->type == ITEM_TYPE_LISTBOX)
			{
				Item_ListBox_MouseEnter(item, x, y, qfalse);
			}
		}
	}
}

void Item_MouseLeave(itemDef_t * item)
{
	if(item)
	{
		if(item->window.flags & WINDOW_MOUSEOVERTEXT)
		{
			Item_RunScript(item, NULL, item->mouseExitText);
			item->window.flags &= ~WINDOW_MOUSEOVERTEXT;
		}
		Item_RunScript(item, NULL, item->mouseExit);
		item->window.flags &= ~(WINDOW_LB_RIGHTARROW | WINDOW_LB_LEFTARROW);
	}
}

itemDef_t      *Menu_HitTest(menuDef_t * menu, float x, float y)
{
	int             i;

	for(i = 0; i < menu->itemCount; i++)
	{
		if(Rect_ContainsPoint(&menu->items[i]->window.rect, x, y))
		{
			return menu->items[i];
		}
	}
	return NULL;
}

void Item_SetMouseOver(itemDef_t * item, qboolean focus)
{
	if(item)
	{
		if(focus)
		{
			item->window.flags |= WINDOW_MOUSEOVER;
		}
		else
		{
			item->window.flags &= ~WINDOW_MOUSEOVER;
		}
	}
}


qboolean Item_OwnerDraw_HandleKey(itemDef_t * item, int key)
{
	if(item && DC->ownerDrawHandleKey)
	{
		return DC->ownerDrawHandleKey(item->window.ownerDraw, item->window.ownerDrawFlags, &item->special, key);
	}
	return qfalse;
}

qboolean Item_ListBox_HandleKey(itemDef_t * item, int key, qboolean down, qboolean force)
{
	listBoxDef_t   *listPtr = (listBoxDef_t *) item->typeData;
	int             count = DC->feederCount(item->special);
	int             max, viewmax;

	if(force || (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && item->window.flags & WINDOW_HASFOCUS))
	{
		max = Item_ListBox_MaxScroll(item);
		if(item->window.flags & WINDOW_HORIZONTAL)
		{
			viewmax = (item->window.rect.w / listPtr->elementWidth);
			if(key == K_LEFTARROW || key == K_KP_LEFTARROW)
			{
				if(!listPtr->notselectable)
				{
					listPtr->cursorPos--;
					if(listPtr->cursorPos < 0)
					{
						listPtr->cursorPos = 0;
					}
					if(listPtr->cursorPos < listPtr->startPos)
					{
						listPtr->startPos = listPtr->cursorPos;
					}
					if(listPtr->cursorPos >= listPtr->startPos + viewmax)
					{
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}
				else
				{
					listPtr->startPos--;
					if(listPtr->startPos < 0)
					{
						listPtr->startPos = 0;
					}
				}
				return qtrue;
			}
			if(key == K_RIGHTARROW || key == K_KP_RIGHTARROW)
			{
				if(!listPtr->notselectable)
				{
					listPtr->cursorPos++;
					if(listPtr->cursorPos < listPtr->startPos)
					{
						listPtr->startPos = listPtr->cursorPos;
					}
					if(listPtr->cursorPos >= count)
					{
						listPtr->cursorPos = count - 1;
					}
					if(listPtr->cursorPos >= listPtr->startPos + viewmax)
					{
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}
				else
				{
					listPtr->startPos++;
					if(listPtr->startPos >= count)
					{
						listPtr->startPos = count - 1;
					}
				}
				return qtrue;
			}
		}
		else
		{
			viewmax = (item->window.rect.h / listPtr->elementHeight);
			if(key == K_UPARROW || key == K_KP_UPARROW || key == K_MWHEELUP)
			{
				if(!listPtr->notselectable)
				{
					listPtr->cursorPos--;
					if(listPtr->cursorPos < 0)
					{
						listPtr->cursorPos = 0;
					}
					if(listPtr->cursorPos < listPtr->startPos)
					{
						listPtr->startPos = listPtr->cursorPos;
					}
					if(listPtr->cursorPos >= listPtr->startPos + viewmax)
					{
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}
				else
				{
					listPtr->startPos--;
					if(listPtr->startPos < 0)
					{
						listPtr->startPos = 0;
					}
				}
				return qtrue;
			}
			if(key == K_DOWNARROW || key == K_KP_DOWNARROW || key == K_MWHEELDOWN)
			{
				if(!listPtr->notselectable)
				{
					listPtr->cursorPos++;
					if(listPtr->cursorPos < listPtr->startPos)
					{
						listPtr->startPos = listPtr->cursorPos;
					}
					if(listPtr->cursorPos >= count)
					{
						listPtr->cursorPos = count - 1;
					}
					if(listPtr->cursorPos >= listPtr->startPos + viewmax)
					{
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}
				else
				{
					listPtr->startPos++;
					if(listPtr->startPos > max)
					{
						listPtr->startPos = max;
					}
				}
				return qtrue;
			}
		}
		// mouse hit
		if(key == K_MOUSE1 || key == K_MOUSE2)
		{
			Item_ListBox_MouseEnter(item, DC->cursorx, DC->cursory, qtrue);

			if(item->window.flags & WINDOW_LB_LEFTARROW)
			{
				listPtr->startPos--;
				if(listPtr->startPos < 0)
				{
					listPtr->startPos = 0;
				}
			}
			else if(item->window.flags & WINDOW_LB_RIGHTARROW)
			{
				// one down
				listPtr->startPos++;
				if(listPtr->startPos > max)
				{
					listPtr->startPos = max;
				}
			}
			else if(item->window.flags & WINDOW_LB_PGUP)
			{
				// page up
				listPtr->startPos -= viewmax;
				if(listPtr->startPos < 0)
				{
					listPtr->startPos = 0;
				}
			}
			else if(item->window.flags & WINDOW_LB_PGDN)
			{
				// page down
				listPtr->startPos += viewmax;
				if(listPtr->startPos > max)
				{
					listPtr->startPos = max;
				}
			}
			else if(item->window.flags & WINDOW_LB_THUMB)
			{
				// Display_SetCaptureItem(item);
			}
			else if(item->window.flags & WINDOW_LB_SOMEWHERE)
			{
				// do nowt
			}
			else
			{
				// select an item
				// Arnout: can't select something that doesn't exist
				if(listPtr->cursorPos >= count)
				{
					listPtr->cursorPos = count - 1;
				}

				if(item->cursorPos == listPtr->cursorPos && DC->realTime < lastListBoxClickTime && listPtr->doubleClick)
				{
					Item_RunScript(item, NULL, listPtr->doubleClick);
				}
				lastListBoxClickTime = DC->realTime + DOUBLE_CLICK_DELAY;

				if(item->cursorPos != listPtr->cursorPos)
				{
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}

				if(key == K_MOUSE1)
				{
					DC->feederSelectionClick(item);
				}

				if(key == K_MOUSE2 && listPtr->contextMenu)
				{
					menuDef_t      *menu = Menus_FindByName(listPtr->contextMenu);

					if(menu)
					{
						menu->window.rect.x = DC->cursorx;
						menu->window.rect.y = DC->cursory;

						Menu_UpdatePosition(menu);
						Menus_ActivateByName(listPtr->contextMenu, qtrue);
					}
				}
			}
			return qtrue;
		}
		if(key == K_HOME || key == K_KP_HOME)
		{
			// home
			listPtr->startPos = 0;
			return qtrue;
		}
		if(key == K_END || key == K_KP_END)
		{
			// end
			listPtr->startPos = max;
			return qtrue;
		}
		if(key == K_PGUP || key == K_KP_PGUP)
		{
			// page up
			if(!listPtr->notselectable)
			{
				listPtr->cursorPos -= viewmax;
				if(listPtr->cursorPos < 0)
				{
					listPtr->cursorPos = 0;
				}
				if(listPtr->cursorPos < listPtr->startPos)
				{
					listPtr->startPos = listPtr->cursorPos;
				}
				if(listPtr->cursorPos >= listPtr->startPos + viewmax)
				{
					listPtr->startPos = listPtr->cursorPos - viewmax + 1;
				}
				item->cursorPos = listPtr->cursorPos;
				DC->feederSelection(item->special, item->cursorPos);
			}
			else
			{
				listPtr->startPos -= viewmax;
				if(listPtr->startPos < 0)
				{
					listPtr->startPos = 0;
				}
			}
			return qtrue;
		}
		if(key == K_PGDN || key == K_KP_PGDN)
		{
			// page down
			if(!listPtr->notselectable)
			{
				listPtr->cursorPos += viewmax;
				if(listPtr->cursorPos < listPtr->startPos)
				{
					listPtr->startPos = listPtr->cursorPos;
				}
				if(listPtr->cursorPos >= count)
				{
					listPtr->cursorPos = count - 1;
				}
				if(listPtr->cursorPos >= listPtr->startPos + viewmax)
				{
					listPtr->startPos = listPtr->cursorPos - viewmax + 1;
				}
				item->cursorPos = listPtr->cursorPos;
				DC->feederSelection(item->special, item->cursorPos);
			}
			else
			{
				listPtr->startPos += viewmax;
				if(listPtr->startPos > max)
				{
					listPtr->startPos = max;
				}
			}
			return qtrue;
		}
	}
	return qfalse;
}

qboolean Item_CheckBox_HandleKey(itemDef_t * item, int key)
{
	if(Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && item->window.flags & WINDOW_HASFOCUS && item->cvar)
	{
		if(key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3)
		{
			// ATVI Wolfenstein Misc #462
			// added the flag to toggle via action script only
			if(!(item->cvarFlags & CVAR_NOTOGGLE))
			{
				if(item->type == ITEM_TYPE_TRICHECKBOX)
				{
					int             curvalue = DC->getCVarValue(item->cvar) + 1;

					if(curvalue > 2)
					{
						curvalue = 0;
					}
					DC->setCVar(item->cvar, va("%i", curvalue));
				}
				else
				{
					DC->setCVar(item->cvar, va("%i", !DC->getCVarValue(item->cvar)));
				}
			}
			return qtrue;
		}
	}
	return qfalse;
}

qboolean Item_YesNo_HandleKey(itemDef_t * item, int key)
{
	if(Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && item->window.flags & WINDOW_HASFOCUS && item->cvar)
	{
		if(key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3)
		{
			// ATVI Wolfenstein Misc #462
			// added the flag to toggle via action script only
			if(!(item->cvarFlags & CVAR_NOTOGGLE))
			{
				DC->setCVar(item->cvar, va("%i", !DC->getCVarValue(item->cvar)));
			}
			return qtrue;
		}
	}
	return qfalse;
}

int Item_Multi_CountSettings(itemDef_t * item)
{
	multiDef_t     *multiPtr = (multiDef_t *) item->typeData;

	if(multiPtr == NULL)
	{
		return 0;
	}
	return multiPtr->count;
}

int Item_Multi_FindCvarByValue(itemDef_t * item)
{
	char            buff[1024];
	float           value = 0;
	int             i;
	multiDef_t     *multiPtr = (multiDef_t *) item->typeData;

	if(multiPtr)
	{
		if(multiPtr->strDef)
		{
			DC->getCVarString(item->cvar, buff, sizeof(buff));
		}
		else
		{
			value = DC->getCVarValue(item->cvar);
		}
		for(i = 0; i < multiPtr->count; i++)
		{
			if(multiPtr->strDef)
			{
				if(Q_stricmp(buff, multiPtr->cvarStr[i]) == 0)
				{
					return i;
				}
			}
			else
			{
				if(multiPtr->cvarValue[i] == value)
				{
					return i;
				}
			}
		}
	}
	return 0;
}

const char     *Item_Multi_Setting(itemDef_t * item)
{
	char            buff[1024];
	float           value = 0;
	int             i;
	multiDef_t     *multiPtr = (multiDef_t *) item->typeData;

	if(multiPtr)
	{
		if(multiPtr->strDef)
		{
			DC->getCVarString(item->cvar, buff, sizeof(buff));
		}
		else
		{
			value = DC->getCVarValue(item->cvar);
		}
		for(i = 0; i < multiPtr->count; i++)
		{
			if(multiPtr->strDef)
			{
				if(Q_stricmp(buff, multiPtr->cvarStr[i]) == 0)
				{
					return multiPtr->cvarList[i];
				}
			}
			else
			{
				if(multiPtr->cvarValue[i] == value)
				{
					return multiPtr->cvarList[i];
				}
			}
		}
	}
	if(multiPtr->undefinedStr)
	{
		return multiPtr->undefinedStr;
	}
	else
	{
		return ((multiPtr->count == 0) ? "None Defined" : "Custom");
	}
}

qboolean Item_Multi_HandleKey(itemDef_t * item, int key)
{
	multiDef_t     *multiPtr = (multiDef_t *) item->typeData;

	if(multiPtr)
	{
		if(Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && item->window.flags & WINDOW_HASFOCUS && item->cvar)
		{
			if(key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3)
			{
				int             current = Item_Multi_FindCvarByValue(item);
				int             max = Item_Multi_CountSettings(item);

				if(key == K_MOUSE2)
				{
					current--;
				}
				else
				{
					current++;
				}

				if(current < 0)
				{
					current = max - 1;
				}
				else if(current >= max)
				{
					current = 0;
				}
				if(multiPtr->strDef)
				{
					DC->setCVar(item->cvar, multiPtr->cvarStr[current]);
				}
				else
				{
					float           value = multiPtr->cvarValue[current];

					if(((float)((int)value)) == value)
					{
						DC->setCVar(item->cvar, va("%i", (int)value));
					}
					else
					{
						DC->setCVar(item->cvar, va("%f", value));
					}
				}
				return qtrue;
			}
		}
	}
	return qfalse;
}

qboolean Item_TextField_HandleKey(itemDef_t * item, int key)
{
	char            buff[1024];
	int             len;
	itemDef_t      *newItem = NULL;
	editFieldDef_t *editPtr = (editFieldDef_t *) item->typeData;

	if(item->cvar)
	{

		memset(buff, 0, sizeof(buff));
		DC->getCVarString(item->cvar, buff, sizeof(buff));
		len = strlen(buff);


		if(editPtr->maxChars && len > editPtr->maxChars)
		{
			len = editPtr->maxChars;
		}

		// Gordon: make sure our cursorpos doesn't go oob, windows doesn't like negative memory copy operations :)
		if(item->cursorPos < 0 || item->cursorPos > len)
		{
			item->cursorPos = 0;
		}

		if(key & K_CHAR_FLAG)
		{
			key &= ~K_CHAR_FLAG;


			if(key == 'h' - 'a' + 1)
			{					// ctrl-h is backspace
				if(item->cursorPos > 0)
				{
					memmove(&buff[item->cursorPos - 1], &buff[item->cursorPos], len + 1 - item->cursorPos);
					item->cursorPos--;
					if(item->cursorPos < editPtr->paintOffset)
					{
						editPtr->paintOffset--;
					}
					buff[len] = '\0';
				}
				DC->setCVar(item->cvar, buff);
				return qtrue;
			}


			//
			// ignore any non printable chars
			//
			if(key < 32 || key == 127 || !item->cvar)
			{
				return qtrue;
			}

			if(item->type == ITEM_TYPE_NUMERICFIELD)
			{
				if((key < '0' || key > '9') && key != '.')
				{
					return qfalse;
				}
			}

			if(DC->getOverstrikeMode && !DC->getOverstrikeMode())
			{
				if((len == MAX_EDITFIELD - 1) || (editPtr->maxChars && len >= editPtr->maxChars))
				{
					return qtrue;
				}
				memmove(&buff[item->cursorPos + 1], &buff[item->cursorPos], len + 1 - item->cursorPos);
			}
			else
			{
				if(editPtr->maxChars && item->cursorPos >= editPtr->maxChars)
				{
					return qtrue;
				}
			}

			buff[item->cursorPos] = key;

			DC->setCVar(item->cvar, buff);

			if(item->cursorPos < len + 1)
			{
				item->cursorPos++;
				if(editPtr->maxPaintChars && item->cursorPos > editPtr->maxPaintChars)
				{
					editPtr->paintOffset++;
				}
			}

		}
		else
		{

			if(key == K_DEL || key == K_KP_DEL)
			{
				if(item->cursorPos < len)
				{
					memmove(buff + item->cursorPos, buff + item->cursorPos + 1, len - item->cursorPos);
					buff[len] = '\0';
					DC->setCVar(item->cvar, buff);
				}
				return qtrue;
			}

			if(key == K_RIGHTARROW || key == K_KP_RIGHTARROW)
			{
				if(editPtr->maxPaintChars && item->cursorPos >= editPtr->paintOffset + editPtr->maxPaintChars &&
				   item->cursorPos < len)
				{
					item->cursorPos++;
					editPtr->paintOffset++;
					return qtrue;
				}
				if(item->cursorPos < len)
				{
					item->cursorPos++;
				}
				return qtrue;
			}

			if(key == K_LEFTARROW || key == K_KP_LEFTARROW)
			{
				if(item->cursorPos > 0)
				{
					item->cursorPos--;
				}
				if(item->cursorPos < editPtr->paintOffset)
				{
					editPtr->paintOffset--;
				}
				return qtrue;
			}

			if(key == K_HOME || key == K_KP_HOME)
			{					// || ( tolower(key) == 'a' && trap_Key_IsDown( K_CTRL ) ) ) {
				item->cursorPos = 0;
				editPtr->paintOffset = 0;
				return qtrue;
			}

			if(key == K_END || key == K_KP_END)
			{					// ( tolower(key) == 'e' && trap_Key_IsDown( K_CTRL ) ) ) {
				item->cursorPos = len;
				if(item->cursorPos > editPtr->maxPaintChars)
				{
					editPtr->paintOffset = len - editPtr->maxPaintChars;
				}
				return qtrue;
			}

			if(key == K_INS || key == K_KP_INS)
			{
				DC->setOverstrikeMode(!DC->getOverstrikeMode());
				return qtrue;
			}
		}

		if(key == K_TAB || key == K_DOWNARROW || key == K_KP_DOWNARROW)
		{
			newItem = Menu_SetNextCursorItem(item->parent);
			if(newItem && (newItem->type == ITEM_TYPE_EDITFIELD || newItem->type == ITEM_TYPE_NUMERICFIELD))
			{
				g_editItem = newItem;
			}
		}

		if(key == K_UPARROW || key == K_KP_UPARROW)
		{
			newItem = Menu_SetPrevCursorItem(item->parent);
			if(newItem && (newItem->type == ITEM_TYPE_EDITFIELD || newItem->type == ITEM_TYPE_NUMERICFIELD))
			{
				g_editItem = newItem;
			}
		}

		// NERVE - SMF
		if(key == K_ENTER || key == K_KP_ENTER)
		{
			if(item->onAccept)
			{
				Item_RunScript(item, NULL, item->onAccept);
			}
		}
		// -NERVE - SMF

		if(key == K_ENTER || key == K_KP_ENTER || key == K_ESCAPE)
		{
			return qfalse;
		}

		return qtrue;
	}
	return qfalse;

}

static void Scroll_ListBox_AutoFunc(void *p)
{
	scrollInfo_t   *si = (scrollInfo_t *) p;

	if(DC->realTime > si->nextScrollTime)
	{
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		Item_ListBox_HandleKey(si->item, si->scrollKey, qtrue, qfalse);
		si->nextScrollTime = DC->realTime + si->adjustValue;
	}

	if(DC->realTime > si->nextAdjustTime)
	{
		si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if(si->adjustValue > SCROLL_TIME_FLOOR)
		{
			si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

static void Scroll_ListBox_ThumbFunc(void *p)
{
	scrollInfo_t   *si = (scrollInfo_t *) p;
	rectDef_t       r;
	int             pos, max;

	listBoxDef_t   *listPtr = (listBoxDef_t *) si->item->typeData;

	if(si->item->window.flags & WINDOW_HORIZONTAL)
	{
		if(DC->cursorx == si->xStart)
		{
			return;
		}
		r.x = si->item->window.rect.x + SCROLLBAR_SIZE + 1;
		r.y = si->item->window.rect.y + si->item->window.rect.h - SCROLLBAR_SIZE - 1;
		r.h = SCROLLBAR_SIZE;
		r.w = si->item->window.rect.w - (SCROLLBAR_SIZE * 2) - 2;
		max = Item_ListBox_MaxScroll(si->item);
		//
		pos = (DC->cursorx - r.x - SCROLLBAR_SIZE / 2) * max / (r.w - SCROLLBAR_SIZE);
		if(pos < 0)
		{
			pos = 0;
		}
		else if(pos > max)
		{
			pos = max;
		}
		listPtr->startPos = pos;
		si->xStart = DC->cursorx;
	}
	else if(DC->cursory != si->yStart)
	{

		r.x = si->item->window.rect.x + si->item->window.rect.w - SCROLLBAR_SIZE - 1;
		r.y = si->item->window.rect.y + SCROLLBAR_SIZE + 1;
		r.h = si->item->window.rect.h - (SCROLLBAR_SIZE * 2) - 2;
		r.w = SCROLLBAR_SIZE;
		max = Item_ListBox_MaxScroll(si->item);
		//
		pos = (DC->cursory - r.y - SCROLLBAR_SIZE / 2) * max / (r.h - SCROLLBAR_SIZE);
		if(pos < 0)
		{
			pos = 0;
		}
		else if(pos > max)
		{
			pos = max;
		}
		listPtr->startPos = pos;
		si->yStart = DC->cursory;
	}

	if(DC->realTime > si->nextScrollTime)
	{
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		// Arnout: clear doubleclicktime though!
		lastListBoxClickTime = 0;
		Item_ListBox_HandleKey(si->item, si->scrollKey, qtrue, qfalse);
		si->nextScrollTime = DC->realTime + si->adjustValue;
	}

	if(DC->realTime > si->nextAdjustTime)
	{
		si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if(si->adjustValue > SCROLL_TIME_FLOOR)
		{
			si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

static void Scroll_Slider_ThumbFunc(void *p)
{
	float           x, value, cursorx;
	scrollInfo_t   *si = (scrollInfo_t *) p;
	editFieldDef_t *editDef = si->item->typeData;

	if(si->item->text)
	{
		x = si->item->textRect.x + si->item->textRect.w + 8;
	}
	else
	{
		x = si->item->window.rect.x;
	}

	cursorx = DC->cursorx;

	if(cursorx < x)
	{
		cursorx = x;
	}
	else if(cursorx > x + SLIDER_WIDTH)
	{
		cursorx = x + SLIDER_WIDTH;
	}
	value = cursorx - x;
	value /= SLIDER_WIDTH;
	value *= (editDef->maxVal - editDef->minVal);
	value += editDef->minVal;
	DC->setCVar(si->item->cvar, va("%f", value));
}

void Item_StartCapture(itemDef_t * item, int key)
{
	int             flags;

	switch (item->type)
	{
		case ITEM_TYPE_EDITFIELD:
		case ITEM_TYPE_NUMERICFIELD:

		case ITEM_TYPE_LISTBOX:
		{
			flags = Item_ListBox_OverLB(item, DC->cursorx, DC->cursory);
			if(flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW))
			{
				scrollInfo.nextScrollTime = DC->realTime + SCROLL_TIME_START;
				scrollInfo.nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
				scrollInfo.adjustValue = SCROLL_TIME_START;
				scrollInfo.scrollKey = key;
				scrollInfo.scrollDir = (flags & WINDOW_LB_LEFTARROW) ? qtrue : qfalse;
				scrollInfo.item = item;
				captureData = &scrollInfo;
				captureFunc = &Scroll_ListBox_AutoFunc;
				itemCapture = item;
			}
			else if(flags & WINDOW_LB_THUMB)
			{
				scrollInfo.scrollKey = key;
				scrollInfo.item = item;
				scrollInfo.xStart = DC->cursorx;
				scrollInfo.yStart = DC->cursory;
				captureData = &scrollInfo;
				captureFunc = &Scroll_ListBox_ThumbFunc;
				itemCapture = item;
			}
			break;
		}
		case ITEM_TYPE_SLIDER:
		{
			flags = Item_Slider_OverSlider(item, DC->cursorx, DC->cursory);
			if(flags & WINDOW_LB_THUMB)
			{
				scrollInfo.scrollKey = key;
				scrollInfo.item = item;
				scrollInfo.xStart = DC->cursorx;
				scrollInfo.yStart = DC->cursory;
				captureData = &scrollInfo;
				captureFunc = &Scroll_Slider_ThumbFunc;
				itemCapture = item;
			}
			break;
		}
	}
}

void Item_StopCapture(itemDef_t * item)
{

}

qboolean Item_Slider_HandleKey(itemDef_t * item, int key, qboolean down)
{
	float           x, value, width, work;

	//DC->Print("slider handle key\n");
	if(item->window.flags & WINDOW_HASFOCUS && item->cvar && Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory))
	{
		if(key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3)
		{
			editFieldDef_t *editDef = item->typeData;

			if(editDef)
			{
				rectDef_t       testRect;

				width = SLIDER_WIDTH;
				if(item->text)
				{
					x = item->textRect.x + item->textRect.w + 8;
				}
				else
				{
					x = item->window.rect.x;
				}

				testRect = item->window.rect;
				testRect.x = x;
				value = (float)SLIDER_THUMB_WIDTH / 2;
				testRect.x -= value;
				//DC->Print("slider x: %f\n", testRect.x);
				testRect.w = (SLIDER_WIDTH + (float)SLIDER_THUMB_WIDTH / 2);
				//DC->Print("slider w: %f\n", testRect.w);
				if(Rect_ContainsPoint(&testRect, DC->cursorx, DC->cursory))
				{
					work = DC->cursorx - x;
					value = work / width;
					value *= (editDef->maxVal - editDef->minVal);
					// vm fuckage
					// value = (((float)(DC->cursorx - x)/ SLIDER_WIDTH) * (editDef->maxVal - editDef->minVal));
					value += editDef->minVal;
					DC->setCVar(item->cvar, va("%f", value));
					return qtrue;
				}
			}
		}
	}
//  DC->Print("slider handle key exit\n");
	return qfalse;
}


qboolean Item_HandleKey(itemDef_t * item, int key, qboolean down)
{
	int             realKey;

	realKey = key;
	if(realKey & K_CHAR_FLAG)
	{
		realKey &= ~K_CHAR_FLAG;
	}

	if(itemCapture)
	{
		Item_StopCapture(itemCapture);
		itemCapture = NULL;
		captureFunc = NULL;
		captureData = NULL;
	}
	else
	{
		// bk001206 - parentheses
		if(down && (realKey == K_MOUSE1 || realKey == K_MOUSE2 || realKey == K_MOUSE3))
		{
			Item_StartCapture(item, key);
		}
	}

	if(!down)
	{
		return qfalse;
	}

	if(realKey == K_ESCAPE && item->onEsc)
	{
		Item_RunScript(item, NULL, item->onEsc);
		return qtrue;
	}

	if(realKey == K_ENTER && item->onEnter)
	{
		Item_RunScript(item, NULL, item->onEnter);
		return qtrue;
	}

	switch (item->type)
	{
		case ITEM_TYPE_BUTTON:
			return qfalse;
			break;
		case ITEM_TYPE_RADIOBUTTON:
			return qfalse;
			break;
		case ITEM_TYPE_CHECKBOX:
		case ITEM_TYPE_TRICHECKBOX:
			return Item_CheckBox_HandleKey(item, key);
			break;
		case ITEM_TYPE_EDITFIELD:
		case ITEM_TYPE_NUMERICFIELD:
			//return Item_TextField_HandleKey(item, key);
			return qfalse;
			break;
		case ITEM_TYPE_COMBO:
			return qfalse;
			break;
		case ITEM_TYPE_LISTBOX:
			return Item_ListBox_HandleKey(item, key, down, qfalse);
			break;
		case ITEM_TYPE_YESNO:
			return Item_YesNo_HandleKey(item, key);
			break;
		case ITEM_TYPE_MULTI:
			return Item_Multi_HandleKey(item, key);
			break;
		case ITEM_TYPE_OWNERDRAW:
			return Item_OwnerDraw_HandleKey(item, key);
			break;
		case ITEM_TYPE_BIND:
			return Item_Bind_HandleKey(item, key, down);
			break;
		case ITEM_TYPE_SLIDER:
			return Item_Slider_HandleKey(item, key, down);
			break;
			//case ITEM_TYPE_IMAGE:
			//  Item_Image_Paint(item);
			//  break;
		default:
			return qfalse;
			break;
	}

	//return qfalse;
}

void Item_Action(itemDef_t * item)
{
	if(item)
	{
		Item_RunScript(item, NULL, item->action);
	}
}

itemDef_t      *Menu_SetPrevCursorItem(menuDef_t * menu)
{
	qboolean        wrapped = qfalse;
	int             oldCursor = menu->cursorItem;

	if(menu->cursorItem < 0)
	{
		menu->cursorItem = menu->itemCount - 1;
		wrapped = qtrue;
	}

	while(menu->cursorItem > -1)
	{

		menu->cursorItem--;
		if(menu->cursorItem < 0 && !wrapped)
		{
			wrapped = qtrue;
			menu->cursorItem = menu->itemCount - 1;
		}
		// NERVE - SMF
		if(menu->cursorItem < 0)
		{
			menu->cursorItem = oldCursor;
			return NULL;
		}
		// -NERVE - SMF

		if(Item_SetFocus(menu->items[menu->cursorItem], DC->cursorx, DC->cursory))
		{
			Menu_HandleMouseMove(menu, menu->items[menu->cursorItem]->window.rect.x + 1,
								 menu->items[menu->cursorItem]->window.rect.y + 1);
			return menu->items[menu->cursorItem];
		}
	}
	menu->cursorItem = oldCursor;
	return NULL;

}

itemDef_t      *Menu_SetNextCursorItem(menuDef_t * menu)
{

	qboolean        wrapped = qfalse;
	int             oldCursor;

	if(!menu)
	{
		return NULL;
	}

	oldCursor = menu->cursorItem;


	if(menu->cursorItem == -1)
	{
		menu->cursorItem = 0;
		wrapped = qtrue;
	}

	while(menu->cursorItem < menu->itemCount)
	{

		menu->cursorItem++;
		if(menu->cursorItem >= menu->itemCount)
		{						// (SA) had a problem 'tabbing' in dialogs with only one possible button
			if(!wrapped)
			{
				wrapped = qtrue;
				menu->cursorItem = 0;
			}
			else
			{
				return menu->items[oldCursor];
			}
		}

		if(Item_SetFocus(menu->items[menu->cursorItem], DC->cursorx, DC->cursory))
		{
			Menu_HandleMouseMove(menu, menu->items[menu->cursorItem]->window.rect.x + 1,
								 menu->items[menu->cursorItem]->window.rect.y + 1);
			return menu->items[menu->cursorItem];
		}
	}

	menu->cursorItem = oldCursor;
	return NULL;
}


static void Window_CloseCinematic(windowDef_t * window)
{
	if(window->style == WINDOW_STYLE_CINEMATIC && window->cinematic >= 0)
	{
		DC->stopCinematic(window->cinematic);
		window->cinematic = -1;
	}
}

static void Menu_CloseCinematics(menuDef_t * menu)
{
	if(menu)
	{
		int             i;

		Window_CloseCinematic(&menu->window);
		for(i = 0; i < menu->itemCount; i++)
		{
			Window_CloseCinematic(&menu->items[i]->window);
			if(menu->items[i]->type == ITEM_TYPE_OWNERDRAW)
			{
				DC->stopCinematic(0 - menu->items[i]->window.ownerDraw);
			}
		}
	}
}

static void Display_CloseCinematics()
{
	int             i;

	for(i = 0; i < menuCount; i++)
	{
		Menu_CloseCinematics(&Menus[i]);
	}
}

/*void  Menus_Activate(menuDef_t *menu) {
	menu->window.flags |= (WINDOW_HASFOCUS | WINDOW_VISIBLE);
	if (menu->onOpen) {
		itemDef_t item;
		item.parent = menu;
		Item_RunScript(&item, NULL, menu->onOpen);
	}

	if (menu->soundName && *menu->soundName) {
//		DC->stopBackgroundTrack();					// you don't want to do this since it will reset s_rawend
		DC->startBackgroundTrack(menu->soundName, menu->soundName);
	}

	Display_CloseCinematics();

}*/

void Menus_Activate(menuDef_t * menu)
{
	int             i;

	for(i = 0; i < menuCount; i++)
	{
		Menus[i].window.flags &= ~(WINDOW_HASFOCUS | WINDOW_MOUSEOVER);
	}

	menu->window.flags |= (WINDOW_HASFOCUS | WINDOW_VISIBLE);

// Omni-bot BEGIN
	// reset this so a newly opened menu can timeout ...
	menu->timedOut = qfalse;
// Omni-bot END

	if(menu->onOpen)
	{
		itemDef_t       item;

		item.parent = menu;
		Item_RunScript(&item, NULL, menu->onOpen);
	}

	// ydnar: set open time (note dc time may be 0, in which case refresh code sets this)
	menu->openTime = DC->realTime;

	if(menu->soundName && *menu->soundName)
	{
//      DC->stopBackgroundTrack();                  // you don't want to do this since it will reset s_rawend
		DC->startBackgroundTrack(menu->soundName, menu->soundName);
	}

	Display_CloseCinematics();

}

qboolean Menus_CaptureFuncActive(void)
{
	if(captureFunc)
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

int Display_VisibleMenuCount()
{
	int             i, count;

	count = 0;
	for(i = 0; i < menuCount; i++)
	{
		if(Menus[i].window.flags & (WINDOW_FORCED | WINDOW_VISIBLE))
		{
			count++;
		}
	}
	return count;
}

void Menus_HandleOOBClick(menuDef_t * menu, int key, qboolean down)
{
	if(menu)
	{
		int             i;

		// basically the behaviour we are looking for is if there are windows in the stack.. see if
		// the cursor is within any of them.. if not close them otherwise activate them and pass the
		// key on.. force a mouse move to activate focus and script stuff
		if(down && menu->window.flags & WINDOW_OOB_CLICK)
		{
			Menu_RunCloseScript(menu);
			menu->window.flags &= ~(WINDOW_HASFOCUS | WINDOW_VISIBLE | WINDOW_MOUSEOVER);
		}

		for(i = 0; i < menuCount; i++)
		{
			if(Menu_OverActiveItem(&Menus[i], DC->cursorx, DC->cursory))
			{
//              Menu_RunCloseScript(menu);          // NERVE - SMF - why do we close the calling menu instead of just removing the focus?
//              menu->window.flags &= ~(WINDOW_HASFOCUS | WINDOW_VISIBLE | WINDOW_MOUSEOVER);

				menu->window.flags &= ~(WINDOW_HASFOCUS | WINDOW_MOUSEOVER);
				Menus[i].window.flags |= (WINDOW_HASFOCUS | WINDOW_VISIBLE);

//              Menus_Activate(&Menus[i]);
				Menu_HandleMouseMove(&Menus[i], DC->cursorx, DC->cursory);
				Menu_HandleKey(&Menus[i], key, down);
			}
		}

		if(Display_VisibleMenuCount() == 0)
		{
			if(DC->Pause)
			{
				DC->Pause(qfalse);
			}
		}
		Display_CloseCinematics();
	}
}

static rectDef_t *Item_CorrectedTextRect(itemDef_t * item)
{
	static rectDef_t rect;

	memset(&rect, 0, sizeof(rectDef_t));
	if(item)
	{
		rect = item->textRect;
		if(rect.w)
		{
			rect.y -= rect.h;
		}
	}
	return &rect;
}

void Menu_HandleKey(menuDef_t * menu, int key, qboolean down)
{
	int             i;
	itemDef_t      *item = NULL;
	qboolean        inHandler = qfalse;

	Menu_HandleMouseMove(menu, DC->cursorx, DC->cursory);	// NERVE - SMF - fix for focus not resetting on unhidden buttons

	if(inHandler)
	{
		return;
	}

	// ydnar: enter key handling for the window supercedes item enter handling
	if(down && ((key == K_ENTER || key == K_KP_ENTER) && menu->onEnter))
	{
		itemDef_t       it;

		it.parent = menu;
		Item_RunScript(&it, NULL, menu->onEnter);
		return;
	}

	inHandler = qtrue;
	if(g_waitingForKey && down)
	{
		Item_Bind_HandleKey(g_bindItem, key, down);
		inHandler = qfalse;
		return;
	}

	if(g_editingField && down)
	{
		if(!Item_TextField_HandleKey(g_editItem, key))
		{
			g_editingField = qfalse;
			g_editItem = NULL;
			inHandler = qfalse;
			return;
		}
		else if(key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3)
		{
			g_editingField = qfalse;
			g_editItem = NULL;
			Display_MouseMove(NULL, DC->cursorx, DC->cursory);
		}
		else if(key == K_TAB || key == K_UPARROW || key == K_DOWNARROW)
		{
			return;
		}
	}

	if(menu == NULL)
	{
		inHandler = qfalse;
		return;
	}

	// see if the mouse is within the window bounds and if so is this a mouse click
	if(down && !(menu->window.flags & WINDOW_POPUP) && !Rect_ContainsPoint(&menu->window.rect, DC->cursorx, DC->cursory))
	{
		static qboolean inHandleKey = qfalse;

		// bk001206 - parentheses
		if(!inHandleKey && (key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3))
		{
			inHandleKey = qtrue;
			Menus_HandleOOBClick(menu, key, down);
			inHandleKey = qfalse;
			inHandler = qfalse;
			return;
		}
	}

	// get the item with focus
	for(i = 0; i < menu->itemCount; i++)
	{
		if(menu->items[i]->window.flags & WINDOW_HASFOCUS)
		{
			item = menu->items[i];
		}
	}

	if(item != NULL)
	{
		if(Item_HandleKey(item, key, down))
		{
			Item_Action(item);
			inHandler = qfalse;
			return;
		}
	}

	if(!down)
	{
		inHandler = qfalse;
		return;
	}

	// START - TAT 9/16/2002
	// we need to check and see if we're supposed to loop through the items to find the key press func
	if(!menu->itemHotkeyMode)
	{
		// END - TAT 9/16/2002

		if(key > 0 && key <= 255 && menu->onKey[key])
		{
			itemDef_t       it;

			it.parent = menu;
			Item_RunScript(&it, NULL, menu->onKey[key]);
			return;
		}

		// START - TAT 9/16/2002
	}
	else if(key > 0 && key <= 255)
	{
		itemDef_t      *it;

		// we're using the item hotkey mode, so we want to loop through all the items in this menu
		for(i = 0; i < menu->itemCount; i++)
		{
			it = menu->items[i];

			// is the hotkey for this the same as what was pressed?
			if(it->hotkey == key
			   // and is this item visible?
			   && Item_EnableShowViaCvar(it, CVAR_SHOW))
			{
				Item_RunScript(it, NULL, it->onKey);
				return;
			}
		}
	}

	// END - TAT 9/16/2002

	// default handling
	switch (key)
	{

		case K_F11:
			if(DC->getCVarValue("developer"))
			{
				debugMode ^= 1;
			}
			break;

		case K_F12:
			if(DC->getCVarValue("developer"))
			{
				DC->executeText(EXEC_APPEND, "screenshot\n");
			}
			break;
		case K_KP_UPARROW:
		case K_UPARROW:
			Menu_SetPrevCursorItem(menu);
			break;

		case K_ESCAPE:
			if(!g_waitingForKey && menu->onESC)
			{
				itemDef_t       it;

				it.parent = menu;
				Item_RunScript(&it, NULL, menu->onESC);
			}
			break;


		case K_ENTER:
		case K_KP_ENTER:
		case K_MOUSE3:
			if(item)
			{
				if(item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD)
				{
					item->cursorPos = 0;
					g_editingField = qtrue;
					g_editItem = item;
				}
				else
				{
					Item_Action(item);
				}
			}
			break;

		case K_TAB:
			if(DC->keyIsDown(K_SHIFT))
			{
				Menu_SetPrevCursorItem(menu);
			}
			else
			{
				Menu_SetNextCursorItem(menu);
			}
			break;
		case K_KP_DOWNARROW:
		case K_DOWNARROW:
			Menu_SetNextCursorItem(menu);
			break;

		case K_MOUSE1:
		case K_MOUSE2:
			if(item)
			{
				if(item->type == ITEM_TYPE_TEXT)
				{
					if(Rect_ContainsPoint(Item_CorrectedTextRect(item), DC->cursorx, DC->cursory))
					{
						Item_Action(item);
					}
				}
				else if(item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD)
				{
					if(Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory))
					{
						editFieldDef_t *editPtr = (editFieldDef_t *) item->typeData;

						// ydnar: fixme, make it set the insertion point correctly

						// NERVE - SMF - reset scroll offset so we can see what we're editing
						if(editPtr)
						{
							editPtr->paintOffset = 0;
						}

						item->cursorPos = 0;
						g_editingField = qtrue;
						g_editItem = item;

						// see elsewhere for venomous comment about this particular piece of "functionality"
						//% DC->setOverstrikeMode(qtrue);
					}
				}
				else
				{
					if(Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory))
					{
						Item_Action(item);
					}
				}
			}
			break;

		case K_JOY1:
		case K_JOY2:
		case K_JOY3:
		case K_JOY4:
		case K_AUX1:
		case K_AUX2:
		case K_AUX3:
		case K_AUX4:
		case K_AUX5:
		case K_AUX6:
		case K_AUX7:
		case K_AUX8:
		case K_AUX9:
		case K_AUX10:
		case K_AUX11:
		case K_AUX12:
		case K_AUX13:
		case K_AUX14:
		case K_AUX15:
		case K_AUX16:
			break;
	}
	inHandler = qfalse;
}

void ToWindowCoords(float *x, float *y, windowDef_t * window)
{
	/*if (window->border != 0) {
	 *x += window->borderSize;
	 *y += window->borderSize;
	 } */
	*x += window->rect.x;
	*y += window->rect.y;
}

void Rect_ToWindowCoords(rectDef_t * rect, windowDef_t * window)
{
	ToWindowCoords(&rect->x, &rect->y, window);
}

void Item_SetTextExtents(itemDef_t * item, int *width, int *height, const char *text)
{
	const char     *textPtr = (text) ? text : item->text;

	if(textPtr == NULL)
	{
		return;
	}

	*width = item->textRect.w;
	*height = item->textRect.h;

	// keeps us from computing the widths and heights more than once
	if(*width == 0 ||
	   (item->type == ITEM_TYPE_OWNERDRAW && item->textalignment == ITEM_ALIGN_CENTER) ||
	   item->textalignment == ITEM_ALIGN_CENTER2 || item->type == ITEM_TYPE_TIMEOUT_COUNTER)
	{							// ydnar
		//% int originalWidth = DC->textWidth(item->text, item->textscale, 0);
		int             originalWidth = DC->textWidth(textPtr, item->textscale, 0);

		if(item->type == ITEM_TYPE_OWNERDRAW &&
		   (item->textalignment == ITEM_ALIGN_CENTER || item->textalignment == ITEM_ALIGN_RIGHT))
		{
			originalWidth += DC->ownerDrawWidth(item->window.ownerDraw, item->textscale);
		}
		else if(item->type == ITEM_TYPE_EDITFIELD && item->textalignment == ITEM_ALIGN_CENTER && item->cvar)
		{
			char            buff[256];

			DC->getCVarString(item->cvar, buff, 256);
			originalWidth += DC->textWidth(buff, item->textscale, 0);
		}
		else if(item->textalignment == ITEM_ALIGN_CENTER2)
		{
			// NERVE - SMF - default centering case
			originalWidth += DC->textWidth(text, item->textscale, 0);
		}

		*width = DC->textWidth(textPtr, item->textscale, 0);
		*height = DC->textHeight(textPtr, item->textscale, 0);
		item->textRect.w = *width;
		item->textRect.h = *height;
		item->textRect.x = item->textalignx;
		item->textRect.y = item->textaligny;
		if(item->textalignment == ITEM_ALIGN_RIGHT)
		{
			item->textRect.x = item->textalignx - originalWidth;
		}
		else if(item->textalignment == ITEM_ALIGN_CENTER || item->textalignment == ITEM_ALIGN_CENTER2)
		{
			// NERVE - SMF - default centering case
			item->textRect.x = item->textalignx - originalWidth / 2;
		}

		ToWindowCoords(&item->textRect.x, &item->textRect.y, &item->window);
	}
}

void Item_TextColor(itemDef_t * item, vec4_t * newColor)
{
	vec4_t          lowLight;
	menuDef_t      *parent = (menuDef_t *) item->parent;

	Fade(&item->window.flags, &item->window.foreColor[3], parent->fadeClamp, &item->window.nextTime, parent->fadeCycle, qtrue,
		 parent->fadeAmount);

	if(item->window.flags & WINDOW_HASFOCUS && item->window.flags & WINDOW_FOCUSPULSE)
	{
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor(parent->focusColor, lowLight, *newColor, 0.5 + 0.5 * sin(DC->realTime / PULSE_DIVISOR));
	}
	else if(item->textStyle == ITEM_TEXTSTYLE_BLINK && !((DC->realTime / BLINK_DIVISOR) & 1))
	{
		lowLight[0] = 0.8 * item->window.foreColor[0];
		lowLight[1] = 0.8 * item->window.foreColor[1];
		lowLight[2] = 0.8 * item->window.foreColor[2];
		lowLight[3] = 0.8 * item->window.foreColor[3];
		LerpColor(item->window.foreColor, lowLight, *newColor, 0.5 + 0.5 * sin(DC->realTime / PULSE_DIVISOR));
	}
	else
	{
		memcpy(newColor, &item->window.foreColor, sizeof(vec4_t));
		// items can be enabled and disabled based on cvars
	}

	if(item->enableCvar && *item->enableCvar && item->cvarTest && *item->cvarTest)
	{
		if(item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE))
		{
			memcpy(newColor, &parent->disableColor, sizeof(vec4_t));
		}
	}
}

void Item_Text_AutoWrapped_Paint(itemDef_t * item)
{
	char            text[1024];
	const char     *p, *textPtr, *newLinePtr;
	char            buff[1024];
	int             width, height, len, textWidth, newLine, newLineWidth;
	qboolean        hasWhitespace;
	float           y;
	vec4_t          color;

	textWidth = 0;
	newLinePtr = NULL;

	if(item->text == NULL)
	{
		if(item->cvar == NULL)
		{
			return;
		}
		else
		{
			DC->getCVarString(item->cvar, text, sizeof(text));
			textPtr = text;
		}
	}
	else
	{
		textPtr = item->text;
	}
	if(*textPtr == '\0')
	{
		return;
	}
	Item_TextColor(item, &color);
	Item_SetTextExtents(item, &width, &height, textPtr);

	y = item->textaligny;
	len = 0;
	buff[0] = '\0';
	newLine = 0;
	newLineWidth = 0;
	hasWhitespace = qfalse;
	p = textPtr;
	while(p)
	{
		textWidth = DC->textWidth(buff, item->textscale, 0);
		if(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\0')
		{
			newLine = len;
			newLinePtr = p + 1;
			newLineWidth = textWidth;
			hasWhitespace = qtrue;
		}
		else if(!hasWhitespace && textWidth > item->window.rect.w)
		{
			newLine = len;
			newLinePtr = p;
			newLineWidth = textWidth;
		}
		if((newLine && textWidth > item->window.rect.w) || *p == '\n' || *p == '\0')
		{
			if(len)
			{
				if(item->textalignment == ITEM_ALIGN_LEFT)
				{
					item->textRect.x = item->textalignx;
				}
				else if(item->textalignment == ITEM_ALIGN_RIGHT)
				{
					item->textRect.x = item->textalignx - newLineWidth;
				}
				else if(item->textalignment == ITEM_ALIGN_CENTER)
				{
					item->textRect.x = item->textalignx - newLineWidth / 2;
				}
				item->textRect.y = y;
				ToWindowCoords(&item->textRect.x, &item->textRect.y, &item->window);
				//
				buff[newLine] = '\0';
				DC->drawText(item->textRect.x, item->textRect.y, item->textscale, color, buff, 0, 0, item->textStyle);
			}
			if(*p == '\0')
			{
				break;
			}
			//
			y += height + 5;
			p = newLinePtr;
			len = 0;
			newLine = 0;
			newLineWidth = 0;
			hasWhitespace = qfalse;
			continue;
		}
		buff[len++] = *p++;

		if(buff[len - 1] == 13)
		{
			buff[len - 1] = ' ';
		}

		buff[len] = '\0';
	}
}

void Item_Text_Wrapped_Paint(itemDef_t * item)
{
	char            text[1024];
	const char     *p, *start, *textPtr;
	char            buff[1024];
	int             width, height;
	float           x, y;
	vec4_t          color;

	// now paint the text and/or any optional images
	// default to left

	if(item->text == NULL)
	{
		if(item->cvar == NULL)
		{
			return;
		}
		else
		{
			DC->getCVarString(item->cvar, text, sizeof(text));
			textPtr = text;
		}
	}
	else
	{
		textPtr = item->text;
	}
	if(*textPtr == '\0')
	{
		return;
	}

	Item_TextColor(item, &color);
	Item_SetTextExtents(item, &width, &height, textPtr);

	x = item->textRect.x;
	y = item->textRect.y;
	start = textPtr;
	p = strchr(textPtr, '\r');
	while(p && *p)
	{
		strncpy(buff, start, p - start + 1);
		buff[p - start] = '\0';
		DC->drawText(x, y, item->textscale, color, buff, 0, 0, item->textStyle);
		y += height + 5;
		start += p - start + 1;
		p = strchr(p + 1, '\r');
	}
	DC->drawText(x, y, item->textscale, color, start, 0, 0, item->textStyle);
}

void Item_Text_Paint(itemDef_t * item)
{
	char            text[1024];
	const char     *textPtr;
	int             height, width;
	vec4_t          color;
	int             seconds;
	menuDef_t      *menu = (menuDef_t *) item->parent;

	if(item->window.flags & WINDOW_WRAPPED)
	{
		Item_Text_Wrapped_Paint(item);
		return;
	}
	if(item->window.flags & WINDOW_AUTOWRAPPED)
	{
		Item_Text_AutoWrapped_Paint(item);
		return;
	}

	if(item->text == NULL)
	{
		if(item->cvar == NULL)
		{
			return;
		}
		else
		{
			DC->getCVarString(item->cvar, text, sizeof(text));
			if(item->window.flags & WINDOW_TEXTASINT)
			{
				COM_StripExtension(text, text);
				item->textRect.w = 0;	// force recalculation
			}
			else if(item->window.flags & WINDOW_TEXTASFLOAT)
			{
				char           *s = va("%.2f", atof(text));

				Q_strncpyz(text, s, sizeof(text));
				item->textRect.w = 0;	// force recalculation
			}
			textPtr = text;
		}
	}
	else
	{
		textPtr = item->text;
	}

	// ydnar: handle counters
	if(item->type == ITEM_TYPE_TIMEOUT_COUNTER && menu != NULL && menu->openTime > 0)
	{
		// calc seconds remaining
		seconds = (menu->openTime + menu->timeout - DC->realTime + 999) / 1000;

		// build string
		if(seconds <= 2)
		{
			//Com_sprintf( text, 255, "^1%d", seconds );
			Com_sprintf(text, 255, item->text, "^1%d^*", seconds);
		}
		else
		{
			//Com_sprintf( text, 255, "%d", seconds );
			Com_sprintf(text, 255, item->text, "%d", seconds);
		}

		// set ptr
		textPtr = text;
	}

	// this needs to go here as it sets extents for cvar types as well
	Item_SetTextExtents(item, &width, &height, textPtr);

	if(*textPtr == '\0')
	{
		return;
	}


	Item_TextColor(item, &color);

	//FIXME: this is a fucking mess
/*
	adjust = 0;
	if (item->textStyle == ITEM_TEXTSTYLE_OUTLINED || item->textStyle == ITEM_TEXTSTYLE_OUTLINESHADOWED) {
		adjust = 0.5;
	}

	if (item->textStyle == ITEM_TEXTSTYLE_SHADOWED || item->textStyle == ITEM_TEXTSTYLE_OUTLINESHADOWED) {
		Fade(&item->window.flags, &DC->Assets.shadowColor[3], DC->Assets.fadeClamp, &item->window.nextTime, DC->Assets.fadeCycle, qfalse);
		DC->drawText(item->textRect.x + DC->Assets.shadowX, item->textRect.y + DC->Assets.shadowY, item->textscale, DC->Assets.shadowColor, textPtr, adjust);
	}
*/


//  if (item->textStyle == ITEM_TEXTSTYLE_OUTLINED || item->textStyle == ITEM_TEXTSTYLE_OUTLINESHADOWED) {
//      Fade(&item->window.flags, &item->window.outlineColor[3], DC->Assets.fadeClamp, &item->window.nextTime, DC->Assets.fadeCycle, qfalse);
//      /*
//      Text_Paint(item->textRect.x-1, item->textRect.y-1, item->textscale, item->window.foreColor, textPtr, adjust);
//      Text_Paint(item->textRect.x, item->textRect.y-1, item->textscale, item->window.foreColor, textPtr, adjust);
//      Text_Paint(item->textRect.x+1, item->textRect.y-1, item->textscale, item->window.foreColor, textPtr, adjust);
//      Text_Paint(item->textRect.x-1, item->textRect.y, item->textscale, item->window.foreColor, textPtr, adjust);
//      Text_Paint(item->textRect.x+1, item->textRect.y, item->textscale, item->window.foreColor, textPtr, adjust);
//      Text_Paint(item->textRect.x-1, item->textRect.y+1, item->textscale, item->window.foreColor, textPtr, adjust);
//      Text_Paint(item->textRect.x, item->textRect.y+1, item->textscale, item->window.foreColor, textPtr, adjust);
//      Text_Paint(item->textRect.x+1, item->textRect.y+1, item->textscale, item->window.foreColor, textPtr, adjust);
//      */
//      DC->drawText(item->textRect.x - 1, item->textRect.y + 1, item->textscale * 1.02, item->window.outlineColor, textPtr, adjust);
//  }

	DC->drawText(item->textRect.x, item->textRect.y, item->textscale, color, textPtr, 0, 0, item->textStyle);
}



void Item_TextField_Paint(itemDef_t * item)
{
	char            buff[1024];
	vec4_t          newColor, lowLight;
	int             offset;
	int             text_len = 0;	// screen length of the editfield text that will be printed
	int             field_offset;	// character offset in the editfield string
	int             screen_offset;	// offset on screen for precise placement
	menuDef_t      *parent = (menuDef_t *) item->parent;
	editFieldDef_t *editPtr = (editFieldDef_t *) item->typeData;

	Item_Text_Paint(item);

	buff[0] = '\0';

	if(item->cvar)
	{
		DC->getCVarString(item->cvar, buff, sizeof(buff));
	}

	parent = (menuDef_t *) item->parent;

	if(item->window.flags & WINDOW_HASFOCUS && item->window.flags & WINDOW_FOCUSPULSE)
	{
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor(parent->focusColor, lowLight, newColor, 0.5 + 0.5 * sin(DC->realTime / PULSE_DIVISOR));
	}
	else
	{
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	// NOTE: offset from the editfield prefix (like "Say: " in limbo menu)
	offset = (item->text && *item->text) ? 8 : 0;

	// TTimo
	// text length control
	// if the edit field goes beyond the available width, drop some characters at the beginning of the string and apply some offseting
	// FIXME: we could cache the text length and offseting, but given the low count of edit fields, I abstained for now
	// FIXME: this won't handle going back into the line of the editfield to the hidden area
	// start of text painting: item->textRect.x + item->textRect.w + offset
	// our window limit: item->window.rect.x + item->window.rect.w
	field_offset = -1;
	do
	{
		field_offset++;
		if(buff + editPtr->paintOffset + field_offset == '\0')
		{
			break;				// keep it safe
		}
		text_len = DC->textWidth(buff + editPtr->paintOffset + field_offset, item->textscale, 0);
	} while(text_len + item->textRect.x + item->textRect.w + offset > item->window.rect.x + item->window.rect.w);

	if(field_offset)
	{
		// we had to take out some chars to make it fit in, there is an additional screen offset to compute
		screen_offset = item->window.rect.x + item->window.rect.w - (text_len + item->textRect.x + item->textRect.w + offset);
	}
	else
	{
		screen_offset = 0;
	}

	if(item->window.flags & WINDOW_HASFOCUS && g_editingField)
	{
		char            cursor = DC->getOverstrikeMode()? '_' : '|';

		DC->drawTextWithCursor(item->textRect.x + item->textRect.w + offset + screen_offset, item->textRect.y, item->textscale,
							   newColor, buff + editPtr->paintOffset + field_offset,
							   item->cursorPos - editPtr->paintOffset - field_offset, cursor, editPtr->maxPaintChars,
							   item->textStyle);
	}
	else
	{
		DC->drawText(item->textRect.x + item->textRect.w + offset + screen_offset, item->textRect.y, item->textscale, newColor,
					 buff + editPtr->paintOffset + field_offset, 0, editPtr->maxPaintChars, item->textStyle);
	}
}

void Item_CheckBox_Paint(itemDef_t * item)
{
	vec4_t          newColor, lowLight;
	float           value;
	menuDef_t      *parent = (menuDef_t *) item->parent;
	qboolean        hasMultiText = qfalse;
	multiDef_t     *multiPtr = (multiDef_t *) item->typeData;

	value = (item->cvar) ? DC->getCVarValue(item->cvar) : 0;

	if(item->window.flags & WINDOW_HASFOCUS && item->window.flags & WINDOW_FOCUSPULSE)
	{
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor(parent->focusColor, lowLight, newColor, 0.5 + 0.5 * sin(DC->realTime / PULSE_DIVISOR));
	}
	else
	{
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	if(multiPtr && multiPtr->count)
	{
		hasMultiText = qtrue;
	}

	if(item->text)
	{
		Item_Text_Paint(item);
		if(item->type == ITEM_TYPE_TRICHECKBOX && value == 2)
		{
			DC->drawHandlePic(item->textRect.x + item->textRect.w + 8, item->window.rect.y, item->window.rect.h,
							  item->window.rect.h, DC->Assets.checkboxCheckNo);
		}
		else if(value)
		{
			DC->drawHandlePic(item->textRect.x + item->textRect.w + 8, item->window.rect.y, item->window.rect.h,
							  item->window.rect.h, DC->Assets.checkboxCheck);
		}
		else
		{
			DC->drawHandlePic(item->textRect.x + item->textRect.w + 8, item->window.rect.y, item->window.rect.h,
							  item->window.rect.h, DC->Assets.checkboxCheckNot);
		}

		if(hasMultiText)
		{
			vec4_t          colour;

			Item_TextColor(item, &colour);
			DC->drawText(item->textRect.x + item->textRect.w + 8 + item->window.rect.h + 4, item->textRect.y, item->textscale,
						 colour, Item_Multi_Setting(item), 0, 0, item->textStyle);
		}
	}
	else
	{
		if(item->type == ITEM_TYPE_TRICHECKBOX && value == 2)
		{
			DC->drawHandlePic(item->window.rect.x, item->window.rect.y, item->window.rect.h, item->window.rect.h,
							  DC->Assets.checkboxCheckNo);
		}
		else if(value)
		{
			DC->drawHandlePic(item->window.rect.x, item->window.rect.y, item->window.rect.h, item->window.rect.h,
							  DC->Assets.checkboxCheck);
		}
		else
		{
			DC->drawHandlePic(item->window.rect.x, item->window.rect.y, item->window.rect.h, item->window.rect.h,
							  DC->Assets.checkboxCheckNot);
		}

		if(hasMultiText)
		{
			vec4_t          colour;

			Item_TextColor(item, &colour);
			DC->drawText(item->window.rect.x + item->window.rect.h + 4, item->window.rect.y + item->textaligny, item->textscale,
						 colour, Item_Multi_Setting(item), 0, 0, item->textStyle);
		}
	}
}

void Item_YesNo_Paint(itemDef_t * item)
{
	vec4_t          newColor, lowLight;
	float           value;
	menuDef_t      *parent = (menuDef_t *) item->parent;

	value = (item->cvar) ? DC->getCVarValue(item->cvar) : 0;

	if(item->window.flags & WINDOW_HASFOCUS && item->window.flags & WINDOW_FOCUSPULSE)
	{
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor(parent->focusColor, lowLight, newColor, 0.5 + 0.5 * sin(DC->realTime / PULSE_DIVISOR));
	}
	else
	{
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	if(item->text)
	{
		Item_Text_Paint(item);
		DC->drawText(item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor,
					 (value != 0) ? DC->translateString("Yes") : DC->translateString("No"), 0, 0, item->textStyle);
	}
	else
	{
		DC->drawText(item->textRect.x, item->textRect.y, item->textscale, newColor, (value != 0) ? "Yes" : "No", 0, 0,
					 item->textStyle);
	}
}

void Item_Multi_Paint(itemDef_t * item)
{
	vec4_t          newColor, lowLight;
	const char     *text = "";
	menuDef_t      *parent = (menuDef_t *) item->parent;

	if(item->window.flags & WINDOW_HASFOCUS && item->window.flags & WINDOW_FOCUSPULSE)
	{
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor(parent->focusColor, lowLight, newColor, 0.5 + 0.5 * sin(DC->realTime / PULSE_DIVISOR));
	}
	else
	{
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	text = Item_Multi_Setting(item);

	if(item->text)
	{
		Item_Text_Paint(item);
		DC->drawText(item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor, text, 0, 0,
					 item->textStyle);
	}
	else
	{
		DC->drawText(item->textRect.x, item->textRect.y, item->textscale, newColor, text, 0, 0, item->textStyle);
	}
}


typedef struct
{
	char           *command;
	int             id;
	int             defaultbind1_right;
	int             defaultbind2_right;
	int             defaultbind1_left;
	int             defaultbind2_left;
	int             bind1;
	int             bind2;
} bind_t;

typedef struct
{
	char           *name;
	float           defaultvalue;
	float           value;
} configcvar_t;

// Gordon: These MUST be all lowercase now
static bind_t   g_bindings[] = {

	{"+forward", 'w', -1, K_UPARROW, -1, -1, -1},
	{"+back", 's', -1, K_DOWNARROW, -1, -1, -1},
	{"+moveleft", 'a', -1, K_LEFTARROW, -1, -1, -1},
	{"+moveright", 'd', -1, K_RIGHTARROW, -1, -1, -1},
	{"+moveup", K_SPACE, -1, K_KP_INS, -1, -1, -1},
	{"+movedown", 'c', -1, K_CTRL, -1, -1, -1},
	{"+leanright", 'e', -1, K_PGDN, -1, -1, -1},
	{"+leanleft", 'q', -1, K_DEL, -1, -1, -1},
	{"+prone", 'x', -1, K_SHIFT, -1, -1, -1},
	{"+attack", K_MOUSE1, -1, K_MOUSE1, -1, -1, -1},
	{"weapalt", K_MOUSE2, -1, K_MOUSE2, -1, -1, -1},
	{"weapprev", K_MWHEELDOWN, -1, K_MWHEELDOWN, -1, -1, -1},
	{"weapnext", K_MWHEELUP, -1, K_MWHEELUP, -1, -1, -1},
	{"weaponbank 10", '0', -1, '0', -1, -1, -1},
	{"weaponbank 1", '1', -1, '1', -1, -1, -1},
	{"weaponbank 2", '2', -1, '2', -1, -1, -1},
	{"weaponbank 3", '3', -1, '3', -1, -1, -1},
	{"weaponbank 4", '4', -1, '4', -1, -1, -1},
	{"weaponbank 5", '5', -1, '5', -1, -1, -1},
	{"weaponbank 6", '6', -1, '6', -1, -1, -1},
	{"weaponbank 7", '7', -1, '7', -1, -1, -1},
	{"weaponbank 8", '8', -1, '8', -1, -1, -1},
	{"weaponbank 9", '9', -1, '9', -1, -1, -1},
	{"+sprint", K_SHIFT, -1, K_MOUSE3, -1, -1, -1},
	{"+speed", K_CAPSLOCK, -1, K_CAPSLOCK, -1, -1, -1},
	{"+activate", 'f', -1, K_ENTER, -1, -1, -1},
	{"+zoom", 'b', -1, 'b', -1, -1, -1},
	{"+mapexpand", 'g', -1, '#', -1, -1, -1},
	{"+reload", 'r', -1, K_END, -1, -1, -1},
	{"+scores", K_TAB, -1, K_TAB, -1, -1, -1},
	{"+stats", K_ALT, -1, K_F9, -1, -1, -1},
	{"+topshots", K_CTRL, -1, K_F10, -1, -1, -1},
	{"toggleconsole", '`', '~', '`', '~', -1, -1},
	{"togglemenu", K_ESCAPE, -1, K_ESCAPE, -1, -1, -1},
	{"openlimbomenu", 'l', -1, 'l', -1, -1, -1},
	{"mvactivate", 'm', -1, 'm', -1, -1, -1},
	{"mapzoomout", ',', -1, '[', -1, -1, -1},
	{"mapzoomin", '.', -1, ']', -1, -1, -1},
	{"zoomin", '=', -1, '-', -1, -1, -1},
	{"zoomout", '-', -1, '=', -1, -1, -1},
	{"messagemode", 't', -1, 't', -1, -1, -1},
	{"messagemode2", 'y', -1, 'y', -1, -1, -1},
	{"messagemode3", 'u', -1, 'u', -1, -1, -1},
	{"mp_quickmessage", 'v', -1, 'v', -1, -1, -1},
	{"mp_fireteammsg", 'z', -1, 'c', -1, -1, -1},
	{"vote yes", K_F1, -1, K_F1, -1, -1, -1},
	{"vote no", K_F2, -1, K_F2, -1, -1, -1},
	{"ready", K_F3, -1, K_F3, -1, -1, -1},
	{"notready", K_F4, -1, K_F4, -1, -1, -1},
	{"autoscreenshot", K_F11, -1, K_F11, -1, -1, -1},
	{"autoRecord", K_F12, -1, K_F12, -1, -1, -1},
	{"mp_fireteamadmin", K_KP_ENTER, -1, K_KP_ENTER, -1, -1, -1},
	{"selectbuddy -1", K_KP_DEL, -1, K_KP_PLUS, -1, -1, -1},
	{"selectbuddy 0", K_KP_END, -1, K_KP_END, -1, -1, -1},
	{"selectbuddy 1", K_KP_DOWNARROW, -1, K_KP_DOWNARROW, -1, -1, -1},
	{"selectbuddy 2", K_KP_PGDN, -1, K_KP_PGDN, -1, -1, -1},
	{"selectbuddy 3", K_KP_LEFTARROW, -1, K_KP_LEFTARROW, -1, -1, -1},
	{"selectbuddy 4", K_KP_5, -1, K_KP_5, -1, -1, -1},
	{"selectbuddy 5", K_KP_RIGHTARROW, -1, K_KP_RIGHTARROW, -1, -1, -1},
	{"selectbuddy -2", K_KP_INS, -1, K_KP_MINUS, -1, -1, -1},

/*	{"+scores",         -1,				-1, -1, -1},
	{"+speed",			K_SHIFT,		-1, -1, -1},
	{"+forward",		K_UPARROW,		-1, -1, -1},
	{"+back",			K_DOWNARROW,	-1, -1, -1},
	{"+moveleft",		',',			-1, -1, -1},
	{"+moveright",		'.',			-1, -1, -1},
	{"+moveup",         K_SPACE,		-1, -1, -1},
	{"+movedown",		'c',			-1, -1, -1},
	{"+left",			K_LEFTARROW,	-1, -1, -1},
	{"+right",			K_RIGHTARROW,	-1, -1, -1},
	{"+strafe",         K_ALT,			-1, -1, -1},
	{"+lookup",         K_PGDN,         -1, -1, -1},
	{"+lookdown",		K_DEL,			-1, -1, -1},
	{"+mlook",			'/',			-1, -1, -1},
//	{"centerview",		K_END,			-1, -1, -1},	// this is an exploit nowadays
	{"+zoom",			'z',            -1, -1, -1},
	{"weaponbank 1",	'1',			-1, -1, -1},
	{"weaponbank 2",	'2',			-1, -1, -1},
	{"weaponbank 3",	'3',			-1, -1, -1},
	{"weaponbank 4",	'4',			-1, -1, -1},
	{"weaponbank 5",	'5',			-1, -1, -1},
	{"weaponbank 6",	'6',			-1, -1, -1},
	{"weaponbank 7",	'7',			-1, -1, -1},
	{"weaponbank 8",	'8',			-1, -1, -1},
	{"weaponbank 9",	'9',			-1, -1, -1},
	{"weaponbank 10",	'0',			-1, -1, -1},
	{"+attack",         K_CTRL,         -1, -1, -1},
	{"weapprev",		K_MWHEELDOWN,	-1, -1, -1},
	{"weapnext",		K_MWHEELUP,		-1, -1, -1},
	{"weapalt",			-1,				-1, -1, -1},
	{"weaplastused",	-1,				-1, -1, -1},//----(SA)	added
	{"weapnextinbank",	-1,				-1, -1, -1},//----(SA)	added
	{"weapprevinbank",	-1,				-1, -1, -1},//----(SA)	added
	{"+useitem",		K_ENTER,		-1, -1, -1},
	{"+button3",		K_MOUSE3,		-1, -1, -1},

	{"scoresUp",		-1,				-1, -1, -1},
	{"scoresDown",		-1,				-1, -1, -1},
	{"messagemode",     -1,				-1, -1, -1},
	{"messagemode2",	-1,             -1, -1, -1},
	{"messagemode3",	-1,             -1, -1, -1},

	{"+activate",		-1,				-1, -1, -1},
	{"zoomin",			-1,				-1, -1, -1},
	{"zoomout",			-1,				-1, -1, -1},
	{"+kick",			-1,				-1, -1, -1},
	{"+reload",         -1,             -1, -1, -1},
	{"+sprint",         -1,             -1, -1, -1},
	{"notebook",		K_TAB,          -1, -1, -1},
	{"help",			K_F1,           -1, -1, -1},
	{"+leanleft",       -1,             -1, -1, -1},
	{"+leanright",      -1,             -1, -1, -1},

	{"vote yes",		-1,				-1,	-1,	-1},
	{"vote no",			-1,				-1,	-1,	-1},
	{"openlimbomenu",   -1,             -1, -1, -1},
	{"mp_quickmessage",	-1,             -1, -1, -1},
	{"mp_fireteammsg",	-1,             -1, -1, -1},
	{"mp_fireteamadmin",-1,             -1, -1, -1},

	// OSP
	{"+stats",	        -1,             -1, -1, -1},
	{"+topshots",	    -1,             -1, -1, -1},
	{"+wstats",         -1,             -1, -1, -1},
	{"autoScreenshot",  -1,             -1, -1, -1},
	{"autoRecord",      -1,             -1, -1, -1},
	{"currenttime",     -1,             -1, -1, -1},
	{"statsdump",	    -1,             -1, -1, -1},
	{"mvallies",	    -1,             -1, -1, -1},
	{"mvaxis",		    -1,             -1, -1, -1},
	{"mvdel",		    -1,             -1, -1, -1},
	{"mvhide",		    -1,             -1, -1, -1},
	{"mvnone",		    -1,             -1, -1, -1},
	{"mvshow",		    -1,             -1, -1, -1},
	{"mvswap",		    -1,             -1, -1, -1},
	{"mvtoggle",	    -1,             -1, -1, -1},
	{"mvactivate",	    -1,             -1, -1, -1},
	// -OSP

	{"weapon 1",		-1,				-1, -1, -1},
	{"weapon 2",		-1,				-1, -1, -1},
	{"weapon 3",		-1,				-1, -1, -1},
	{"weapon 4",		-1,				-1, -1, -1},
	{"weapon 5",		-1,				-1, -1, -1},
	{"weapon 6",		-1,				-1, -1, -1},
	{"weapon 7",		-1,				-1, -1, -1},
	{"weapon 8",		-1,				-1, -1, -1},
	{"weapon 9",		-1,				-1, -1, -1},
	{"weapon 10",		-1,				-1, -1, -1},
	{"weapon 11",		-1,             -1, -1, -1},
	{"weapon 12",		-1,             -1, -1, -1},
	{"weapon 13",		-1,             -1, -1, -1},
	{"weapon 14",		-1,             -1, -1, -1},
	{"weapon 15",		-1,             -1, -1, -1},
	{"weapon 16",		-1,             -1, -1, -1},
	{"weapon 17",		-1,             -1, -1, -1},
	{"weapon 18",		-1,             -1, -1, -1},
	{"weapon 19",		-1,             -1, -1, -1},
	{"weapon 20",		-1,             -1, -1, -1},
	{"weapon 21",		-1,             -1, -1, -1},
	{"weapon 22",		-1,             -1, -1, -1},
	{"weapon 23",		-1,             -1, -1, -1},
	{"weapon 24",		-1,             -1, -1, -1},
	{"weapon 25",		-1,             -1, -1, -1},
	{"weapon 26",		-1,             -1, -1, -1},
	{"weapon 27",		-1,             -1, -1, -1},
	{"weapon 28",		-1,             -1, -1, -1},
	{"weapon 29",		-1,             -1, -1, -1},
	{"weapon 30",		-1,             -1, -1, -1},
	{"weapon 31",		-1,             -1, -1, -1},
	{"weapon 32",		-1,             -1, -1, -1},

	{"commandmap",		-1,             -1, -1, -1},
	{"buddy",			-1,             -1, -1, -1},
	{"stats",			-1,             -1, -1, -1},

	{"selectbuddy -1",	-1,				-1, -1, -1},
	{"selectbuddy -2",	-1,				-1, -1, -1},
	{"selectbuddy 0",	-1,				-1, -1, -1},
	{"selectbuddy 1",	-1,				-1, -1, -1},
	{"selectbuddy 2",	-1,				-1, -1, -1},
	{"selectbuddy 3",	-1,				-1, -1, -1},
	{"selectbuddy 4",	-1,				-1, -1, -1},
	{"selectbuddy 5",	-1,				-1, -1, -1},
	{"gotowaypoint",	-1,				-1, -1, -1},
	{"mapzoomin",		-1,				-1, -1, -1},
	{"mapzoomout",		-1,				-1, -1, -1},
	{"+mapexpand",		-1,				-1, -1, -1},

	{"+prone",			-1,             -1, -1, -1},*/
};


static const int g_bindCount = sizeof(g_bindings) / sizeof(bind_t);

/*
=================
Controls_GetConfig
=================
*/
void Controls_GetConfig(void)
{
	int             i;

	// iterate each command, get its numeric binding
	for(i = 0; i < g_bindCount; i++)
	{
		DC->getKeysForBinding(g_bindings[i].command, &g_bindings[i].bind1, &g_bindings[i].bind2);
	}
}

/*
=================
Controls_SetConfig
=================
*/
void Controls_SetConfig(qboolean restart)
{
	int             i;

	// iterate each command, get its numeric binding
	for(i = 0; i < g_bindCount; i++)
	{
		if(g_bindings[i].bind1 != -1)
		{
			DC->setBinding(g_bindings[i].bind1, g_bindings[i].command);

			if(g_bindings[i].bind2 != -1)
			{
				DC->setBinding(g_bindings[i].bind2, g_bindings[i].command);
			}
		}
	}

#if !defined( __MACOS__ )
	if(restart)
	{
		DC->executeText(EXEC_APPEND, "in_restart\n");
	}
#endif
	//trap_Cmd_ExecuteText( EXEC_APPEND, "in_restart\n" );
}

/*
=================
Controls_SetDefaults
=================
*/
void Controls_SetDefaults(qboolean lefthanded)
{
	int             i;

	// iterate each command, set its default binding
	for(i = 0; i < g_bindCount; i++)
	{
		g_bindings[i].bind1 = lefthanded ? g_bindings[i].defaultbind1_left : g_bindings[i].defaultbind1_right;
		g_bindings[i].bind2 = lefthanded ? g_bindings[i].defaultbind2_left : g_bindings[i].defaultbind2_right;
	}
}

int BindingIDFromName(const char *name)
{
	int             i;

	for(i = 0; i < g_bindCount; i++)
	{
		if(!Q_stricmp(name, g_bindings[i].command))
		{
			return i;
		}
	}

	return -1;
}

char            g_nameBind1[32];
char            g_nameBind2[32];

char           *BindingFromName(const char *cvar)
{
	int             b1, b2;

	DC->getKeysForBinding(cvar, &b1, &b2);

	if(b1 != -1)
	{
		DC->keynumToStringBuf(b1, g_nameBind1, 32);
		Q_strupr(g_nameBind1);

		if(b2 != -1)
		{
			DC->keynumToStringBuf(b2, g_nameBind2, 32);
			Q_strupr(g_nameBind2);
			Q_strcat(g_nameBind1, 32, DC->translateString(" or "));
			Q_strcat(g_nameBind1, 32, g_nameBind2);
		}
	}
	else
	{
		Q_strncpyz(g_nameBind1, "(?" "?" "?)", 32);
	}
	return g_nameBind1;			// NERVE - SMF
}

void Item_Slider_Paint(itemDef_t * item)
{
	vec4_t          newColor, lowLight;
	float           x, y, value;
	menuDef_t      *parent = (menuDef_t *) item->parent;

	value = (item->cvar) ? DC->getCVarValue(item->cvar) : 0;

	if(item->window.flags & WINDOW_HASFOCUS && item->window.flags & WINDOW_FOCUSPULSE)
	{
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor(parent->focusColor, lowLight, newColor, 0.5 + 0.5 * sin(DC->realTime / PULSE_DIVISOR));
	}
	else
	{
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	y = item->window.rect.y;
	if(item->text)
	{
		Item_Text_Paint(item);
		x = item->textRect.x + item->textRect.w + 8;
	}
	else
	{
		x = item->window.rect.x;
	}
	DC->setColor(newColor);
	//DC->drawHandlePic( x, y, SLIDER_WIDTH, SLIDER_HEIGHT, DC->Assets.sliderBar );
	DC->drawHandlePic(x, y + 1, SLIDER_WIDTH, SLIDER_HEIGHT, DC->Assets.sliderBar);

	x = Item_Slider_ThumbPosition(item);
	//DC->drawHandlePic( x - (SLIDER_THUMB_WIDTH / 2), y - 2, SLIDER_THUMB_WIDTH, SLIDER_THUMB_HEIGHT, DC->Assets.sliderThumb );
	DC->drawHandlePic(x - (SLIDER_THUMB_WIDTH / 2), y, SLIDER_THUMB_WIDTH, SLIDER_THUMB_HEIGHT, DC->Assets.sliderThumb);
}

void Item_Bind_Paint(itemDef_t * item)
{
	vec4_t          newColor, lowLight;
	float           value;
	int             maxChars = 0;
	menuDef_t      *parent = (menuDef_t *) item->parent;
	editFieldDef_t *editPtr = (editFieldDef_t *) item->typeData;

	if(editPtr)
	{
		maxChars = editPtr->maxPaintChars;
	}

	value = (item->cvar) ? DC->getCVarValue(item->cvar) : 0;

	if(item->window.flags & WINDOW_HASFOCUS && item->window.flags & WINDOW_FOCUSPULSE)
	{
		if(g_bindItem == item)
		{
			lowLight[0] = 0.8f * 1.0f;
			lowLight[1] = 0.8f * 0.0f;
			lowLight[2] = 0.8f * 0.0f;
			lowLight[3] = 0.8f * 1.0f;
		}
		else
		{
			lowLight[0] = 0.8f * parent->focusColor[0];
			lowLight[1] = 0.8f * parent->focusColor[1];
			lowLight[2] = 0.8f * parent->focusColor[2];
			lowLight[3] = 0.8f * parent->focusColor[3];
		}
		LerpColor(parent->focusColor, lowLight, newColor, 0.5 + 0.5 * sin(DC->realTime / PULSE_DIVISOR));
	}
	else
	{
		if(g_bindItem == item)
		{
			lowLight[0] = 0.8f * 1.0f;
			lowLight[1] = 0.8f * 0.0f;
			lowLight[2] = 0.8f * 0.0f;
			lowLight[3] = 0.8f * 1.0f;
			LerpColor(item->window.foreColor, lowLight, newColor, 0.5 + 0.5 * sin(DC->realTime / PULSE_DIVISOR));
		}
		else
		{
			memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
		}
	}

	if(item->text)
	{
		Item_Text_Paint(item);
		BindingFromName(item->cvar);
		DC->drawText(item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor, g_nameBind1, 0,
					 maxChars, item->textStyle);
	}
	else
	{
		DC->drawText(item->textRect.x, item->textRect.y, item->textscale, newColor, (value != 0) ? "FIXME" : "FIXME", 0, maxChars,
					 item->textStyle);
	}
}

qboolean Display_KeyBindPending()
{
	return g_waitingForKey;
}

qboolean Item_Bind_HandleKey(itemDef_t * item, int key, qboolean down)
{
	int             id;
	int             i;

	if(Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && !g_waitingForKey)
	{
		if(down && (key == K_MOUSE1 || key == K_ENTER))
		{
			g_waitingForKey = qtrue;
			g_bindItem = item;
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}
	else
	{
		if(!g_waitingForKey || g_bindItem == NULL)
		{
			return qfalse;
		}

		if(key & K_CHAR_FLAG)
		{
			return qtrue;
		}

		switch (key)
		{
			case K_ESCAPE:
				g_waitingForKey = qfalse;
				g_bindItem = NULL;
				return qtrue;

			case K_BACKSPACE:
				id = BindingIDFromName(item->cvar);
				if(id != -1)
				{
					g_bindings[id].bind1 = -1;
					g_bindings[id].bind2 = -1;
				}
				Controls_SetConfig(qtrue);
				g_waitingForKey = qfalse;
				g_bindItem = NULL;
				return qtrue;

		}
	}

	if(key != -1)
	{

		for(i = 0; i < g_bindCount; i++)
		{

			if(g_bindings[i].bind2 == key)
			{
				g_bindings[i].bind2 = -1;
			}

			if(g_bindings[i].bind1 == key)
			{
				g_bindings[i].bind1 = g_bindings[i].bind2;
				g_bindings[i].bind2 = -1;
			}
		}
	}


	id = BindingIDFromName(item->cvar);

	if(id != -1)
	{
		if(key == -1)
		{
			if(g_bindings[id].bind1 != -1)
			{
				DC->setBinding(g_bindings[id].bind1, "");
				g_bindings[id].bind1 = -1;
			}
			if(g_bindings[id].bind2 != -1)
			{
				DC->setBinding(g_bindings[id].bind2, "");
				g_bindings[id].bind2 = -1;
			}
		}
		else if(g_bindings[id].bind1 == -1)
		{
			g_bindings[id].bind1 = key;
		}
		else if(g_bindings[id].bind1 != key && g_bindings[id].bind2 == -1)
		{
			g_bindings[id].bind2 = key;
		}
		else
		{
			DC->setBinding(g_bindings[id].bind1, "");
			DC->setBinding(g_bindings[id].bind2, "");
			g_bindings[id].bind1 = key;
			g_bindings[id].bind2 = -1;
		}
	}

	Controls_SetConfig(qtrue);
	g_waitingForKey = qfalse;
	g_bindItem = NULL;

	return qtrue;
}



void AdjustFrom640(float *x, float *y, float *w, float *h)
{
	//*x = *x * DC->scale + DC->bias;
	*x *= DC->xscale;
	*y *= DC->yscale;
	*w *= DC->xscale;
	*h *= DC->yscale;
}

void Item_Model_Paint(itemDef_t * item)
{
	float           x, y, w, h;	//,xx;
	refdef_t        refdef;
	qhandle_t       hModel;
	refEntity_t     ent;
	vec3_t          mins, maxs, origin;
	vec3_t          angles;
	modelDef_t     *modelPtr = (modelDef_t *) item->typeData;
	int             backLerpWhole;

	if(modelPtr == NULL)
	{
		return;
	}

	if(!item->asset)
	{
		return;
	}

	hModel = item->asset;

	// setup the refdef
	memset(&refdef, 0, sizeof(refdef));
	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear(refdef.viewaxis);
	x = item->window.rect.x + 1;
	y = item->window.rect.y + 1;
	w = item->window.rect.w - 2;
	h = item->window.rect.h - 2;

	AdjustFrom640(&x, &y, &w, &h);

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	DC->modelBounds(hModel, mins, maxs);

	origin[2] = -0.5 * (mins[2] + maxs[2]);
	origin[1] = 0.5 * (mins[1] + maxs[1]);

	// calculate distance so the model nearly fills the box
	if(qtrue)
	{
		float           len = 0.5 * (maxs[2] - mins[2]);

		origin[0] = len / 0.268;	// len / tan( fov/2 )
		//origin[0] = len / tan(w/2);
	}
	else
	{
		origin[0] = item->textscale;
	}

#define NEWWAY
#ifdef NEWWAY
	refdef.fov_x = (modelPtr->fov_x) ? modelPtr->fov_x : w;
	refdef.fov_y = (modelPtr->fov_y) ? modelPtr->fov_y : h;
#else
	refdef.fov_x = (int)((float)refdef.width / 640.0f * 90.0f);
	xx = refdef.width / tan(refdef.fov_x / 360 * M_PI);
	refdef.fov_y = atan2(refdef.height, xx);
	refdef.fov_y *= (360 / M_PI);
#endif
	DC->clearScene();

	refdef.time = DC->realTime;

	// add the model

	memset(&ent, 0, sizeof(ent));

	//adjust = 5.0 * sin( (float)uis.realtime / 500 );
	//adjust = 360 % (int)((float)uis.realtime / 1000);
	//VectorSet( angles, 0, 0, 1 );

	// use item storage to track
	if(modelPtr->rotationSpeed)
	{
		if(DC->realTime > item->window.nextTime)
		{
			item->window.nextTime = DC->realTime + modelPtr->rotationSpeed;
			modelPtr->angle = (int)(modelPtr->angle + 1) % 360;
		}
	}
	VectorSet(angles, 0, modelPtr->angle, 0);
	AnglesToAxis(angles, ent.axis);

	ent.hModel = hModel;


	if(modelPtr->frameTime)
	{							// don't advance on the first frame
		modelPtr->backlerp += (((DC->realTime - modelPtr->frameTime) / 1000.0f) * (float)modelPtr->fps);
	}

	if(modelPtr->backlerp > 1)
	{
		backLerpWhole = floor(modelPtr->backlerp);

		modelPtr->frame += (backLerpWhole);
		if((modelPtr->frame - modelPtr->startframe) > modelPtr->numframes)
		{
			modelPtr->frame = modelPtr->startframe + modelPtr->frame % modelPtr->numframes;	// todo: ignoring loopframes

		}
		modelPtr->oldframe += (backLerpWhole);
		if((modelPtr->oldframe - modelPtr->startframe) > modelPtr->numframes)
		{
			modelPtr->oldframe = modelPtr->startframe + modelPtr->oldframe % modelPtr->numframes;	// todo: ignoring loopframes

		}
		modelPtr->backlerp = modelPtr->backlerp - backLerpWhole;
	}

	modelPtr->frameTime = DC->realTime;

	ent.frame = modelPtr->frame;
	ent.oldframe = modelPtr->oldframe;
	ent.backlerp = 1.0f - modelPtr->backlerp;

	VectorCopy(origin, ent.origin);
	VectorCopy(origin, ent.lightingOrigin);
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy(ent.origin, ent.oldorigin);

	DC->addRefEntityToScene(&ent);
	DC->renderScene(&refdef);

}


void Item_Image_Paint(itemDef_t * item)
{
	if(item == NULL)
	{
		return;
	}
	DC->drawHandlePic(item->window.rect.x + 1, item->window.rect.y + 1, item->window.rect.w - 2, item->window.rect.h - 2,
					  item->asset);
}

void Item_ListBox_Paint(itemDef_t * item)
{
	float           x, y, size, count, i, thumb;
	qhandle_t       image;
	qhandle_t       optionalImages[8];
	int             numOptionalImages;
	listBoxDef_t   *listPtr = (listBoxDef_t *) item->typeData;
	rectDef_t       fillRect = item->window.rect;

	/*if( item->window.borderSize ) {
	   fillRect.x += item->window.borderSize;
	   fillRect.y += item->window.borderSize;
	   fillRect.w -= 2 * item->window.borderSize;
	   fillRect.h -= 2 * item->window.borderSize;
	   } */

	// the listbox is horizontal or vertical and has a fixed size scroll bar going either direction
	// elements are enumerated from the DC and either text or image handles are acquired from the DC as well
	// textscale is used to size the text, textalignx and textaligny are used to size image elements
	// there is no clipping available so only the last completely visible item is painted
	count = DC->feederCount(item->special);
	// default is vertical if horizontal flag is not here
	if(item->window.flags & WINDOW_HORIZONTAL)
	{
		// draw scrollbar in bottom of the window
		// bar
		x = fillRect.x + 1;
		y = fillRect.y + fillRect.h - SCROLLBAR_SIZE - 1;
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowLeft);
		x += SCROLLBAR_SIZE - 1;
		size = fillRect.w - (SCROLLBAR_SIZE * 2);
		DC->drawHandlePic(x, y, size + 1, SCROLLBAR_SIZE, DC->Assets.scrollBar);
		x += size - 1;
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowRight);
		// thumb
		thumb = Item_ListBox_ThumbDrawPosition(item);	//Item_ListBox_ThumbPosition(item);
		if(thumb > x - SCROLLBAR_SIZE - 1)
		{
			thumb = x - SCROLLBAR_SIZE - 1;
		}
		DC->drawHandlePic(thumb, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarThumb);
		//
		listPtr->endPos = listPtr->startPos;
		size = fillRect.w - 2;
		// items
		// size contains max available space
		if(listPtr->elementStyle == LISTBOX_IMAGE)
		{
			// fit = 0;
			x = fillRect.x + 1;
			y = fillRect.y + 1;
			for(i = listPtr->startPos; i < count; i++)
			{
				// always draw at least one
				// which may overdraw the box if it is too small for the element
				image = DC->feederItemImage(item->special, i);
				if(image)
				{
					DC->drawHandlePic(x + 1, y + 1, listPtr->elementWidth - 2, listPtr->elementHeight - 2, image);
				}

				if(i == item->cursorPos)
				{
					DC->drawRect(x, y, listPtr->elementWidth - 1, listPtr->elementHeight - 1, item->window.borderSize,
								 item->window.borderColor);
				}

				size -= listPtr->elementWidth;
				if(size < listPtr->elementWidth)
				{
					listPtr->drawPadding = size;	//listPtr->elementWidth - size;
					break;
				}
				x += listPtr->elementWidth;
				listPtr->endPos++;
				// fit++;
			}
		}
		else
		{
			//
		}
	}
	else
	{
		// draw scrollbar to right side of the window
		x = fillRect.x + fillRect.w - SCROLLBAR_SIZE - 1;
		y = fillRect.y + 1;
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowUp);
		y += SCROLLBAR_SIZE - 1;

		listPtr->endPos = listPtr->startPos;
		size = fillRect.h - (SCROLLBAR_SIZE * 2);
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, size + 1, DC->Assets.scrollBar);
		y += size - 1;
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowDown);
		// thumb
		thumb = Item_ListBox_ThumbDrawPosition(item);	//Item_ListBox_ThumbPosition(item);
		if(thumb > y - SCROLLBAR_SIZE - 1)
		{
			thumb = y - SCROLLBAR_SIZE - 1;
		}
		DC->drawHandlePic(x, thumb, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarThumb);

		// adjust size for item painting
		size = fillRect.h /* - 2 */ ;
		if(listPtr->elementStyle == LISTBOX_IMAGE)
		{
			// fit = 0;
			x = fillRect.x + 1;
			y = fillRect.y + 1;
			for(i = listPtr->startPos; i < count; i++)
			{
				if(i == item->cursorPos)
				{
					DC->fillRect(x, y, listPtr->elementWidth - 1, listPtr->elementHeight - 1, item->window.outlineColor);
				}

				// always draw at least one
				// which may overdraw the box if it is too small for the element
				image = DC->feederItemImage(item->special, i);
				if(image)
				{
					DC->drawHandlePic(x + 1, y + 1, listPtr->elementWidth - 2, listPtr->elementHeight - 2, image);
				}

				if(i == item->cursorPos)
				{
					DC->drawRect(x, y, listPtr->elementWidth - 1, listPtr->elementHeight - 1, item->window.borderSize,
								 item->window.borderColor);
				}

				listPtr->endPos++;
				size -= listPtr->elementHeight;
				if(size < listPtr->elementHeight)
				{
					listPtr->drawPadding = size;	//listPtr->elementHeight - size;
					break;
				}
				y += listPtr->elementHeight;
				// fit++;
			}
		}
		else
		{
			x = fillRect.x /*+ 1 */ ;
			y = fillRect.y /*+ 1 */ ;
			for(i = listPtr->startPos; i < count; i++)
			{
				const char     *text;

				// always draw at least one
				// which may overdraw the box if it is too small for the element

				if(listPtr->numColumns > 0)
				{
					int             j, k;

					for(j = 0; j < listPtr->numColumns; j++)
					{
						text = DC->feederItemText(item->special, i, j, optionalImages, &numOptionalImages);
						if(numOptionalImages > 0)
						{
							for(k = 0; k < numOptionalImages; k++)
							{
								if(optionalImages[k] >= 0)
								{
									DC->drawHandlePic(x + listPtr->columnInfo[j].pos + k * listPtr->elementHeight + 1,
													  y + 1, listPtr->elementHeight - 2, listPtr->elementHeight - 2,
													  optionalImages[k]);
								}
							}
							//DC->drawHandlePic( x + 4 + listPtr->columnInfo[j].pos, y - 1 + listPtr->elementHeight / 2, listPtr->columnInfo[j].width, listPtr->columnInfo[j].width, optionalImage);
						}
						else if(text)
						{
							DC->drawText(x + 4 + listPtr->columnInfo[j].pos + item->textalignx,
										 y + listPtr->elementHeight + item->textaligny, item->textscale, item->window.foreColor,
										 text, 0, listPtr->columnInfo[j].maxChars, item->textStyle);
						}
					}
				}
				else
				{
					text = DC->feederItemText(item->special, i, 0, optionalImages, &numOptionalImages);
					if(numOptionalImages >= 0)
					{
						//DC->drawHandlePic(x + 4 + listPtr->elementHeight, y, listPtr->columnInfo[j].width, listPtr->columnInfo[j].width, optionalImage);
					}
					else if(text)
					{
						DC->drawText(x + 4 + item->textalignx, y + listPtr->elementHeight + item->textaligny, item->textscale,
									 item->window.foreColor, text, 0, 0, item->textStyle);
					}
				}

				if(i == item->cursorPos)
				{
					DC->fillRect(x, y, fillRect.w - SCROLLBAR_SIZE - 2, listPtr->elementHeight /* - 1 */ ,
								 item->window.outlineColor);
				}

				size -= listPtr->elementHeight;
				if(size < listPtr->elementHeight)
				{
					listPtr->drawPadding = size;	//listPtr->elementHeight - size;
					break;
				}
				listPtr->endPos++;
				y += listPtr->elementHeight;
				// fit++;
			}
		}
	}
}


void Item_OwnerDraw_Paint(itemDef_t * item)
{
	menuDef_t      *parent;

	if(item == NULL)
	{
		return;
	}

	parent = (menuDef_t *) item->parent;

	if(DC->ownerDrawItem)
	{
		vec4_t          color, lowLight;
		menuDef_t      *parent = (menuDef_t *) item->parent;

		Fade(&item->window.flags, &item->window.foreColor[3], parent->fadeClamp, &item->window.nextTime, parent->fadeCycle, qtrue,
			 parent->fadeAmount);
		memcpy(&color, &item->window.foreColor, sizeof(color));
		if(item->numColors > 0 && DC->getValue)
		{
			// if the value is within one of the ranges then set color to that, otherwise leave at default
			int             i;
			float           f = DC->getValue(item->window.ownerDraw, item->colorRangeType);

			for(i = 0; i < item->numColors; i++)
			{
				if(f >= item->colorRanges[i].low && f <= item->colorRanges[i].high)
				{
					memcpy(&color, &item->colorRanges[i].color, sizeof(color));
					break;
				}
			}
		}

		if(item->window.flags & WINDOW_HASFOCUS && item->window.flags & WINDOW_FOCUSPULSE)
		{
			lowLight[0] = 0.8 * parent->focusColor[0];
			lowLight[1] = 0.8 * parent->focusColor[1];
			lowLight[2] = 0.8 * parent->focusColor[2];
			lowLight[3] = 0.8 * parent->focusColor[3];
			LerpColor(parent->focusColor, lowLight, color, 0.5 + 0.5 * sin(DC->realTime / PULSE_DIVISOR));
		}
		else if(item->textStyle == ITEM_TEXTSTYLE_BLINK && !((DC->realTime / BLINK_DIVISOR) & 1))
		{
			lowLight[0] = 0.8 * item->window.foreColor[0];
			lowLight[1] = 0.8 * item->window.foreColor[1];
			lowLight[2] = 0.8 * item->window.foreColor[2];
			lowLight[3] = 0.8 * item->window.foreColor[3];
			LerpColor(item->window.foreColor, lowLight, color, 0.5 + 0.5 * sin(DC->realTime / PULSE_DIVISOR));
		}

		if(item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE))
		{
			memcpy(color, parent->disableColor, sizeof(vec4_t));
		}

		// gah wtf indentation!
		if(item->text)
		{
			Item_Text_Paint(item);
			if(item->text[0])
			{
				// +8 is an offset kludge to properly align owner draw items that have text combined with them
				DC->ownerDrawItem(item->textRect.x + item->textRect.w + 8, item->window.rect.y, item->window.rect.w,
								  item->window.rect.h, 0, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags,
								  item->alignment, item->special, item->textscale, color, item->window.background,
								  item->textStyle);
			}
			else
			{
				DC->ownerDrawItem(item->textRect.x + item->textRect.w, item->window.rect.y, item->window.rect.w,
								  item->window.rect.h, 0, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags,
								  item->alignment, item->special, item->textscale, color, item->window.background,
								  item->textStyle);
			}
		}
		else
		{
			DC->ownerDrawItem(item->window.rect.x, item->window.rect.y, item->window.rect.w, item->window.rect.h,
							  item->textalignx, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags,
							  item->alignment, item->special, item->textscale, color, item->window.background, item->textStyle);
		}
	}
}


void Item_Paint(itemDef_t * item)
{
	vec4_t          red;
	menuDef_t      *parent = (menuDef_t *) item->parent;

	red[0] = red[3] = 1;
	red[1] = red[2] = 0;

	if(item == NULL)
	{
		return;
	}

	if(DC->textFont)
	{
		DC->textFont(item->font);
	}

	if(item->window.flags & WINDOW_ORBITING)
	{
		if(DC->realTime > item->window.nextTime)
		{
			float           rx, ry, a, c, s, w, h;

			item->window.nextTime = DC->realTime + item->window.offsetTime;
			// translate
			w = item->window.rectClient.w / 2;
			h = item->window.rectClient.h / 2;
			rx = item->window.rectClient.x + w - item->window.rectEffects.x;
			ry = item->window.rectClient.y + h - item->window.rectEffects.y;
			a = 3 * M_PI / 180;
			c = cos(a);
			s = sin(a);
			item->window.rectClient.x = (rx * c - ry * s) + item->window.rectEffects.x - w;
			item->window.rectClient.y = (rx * s + ry * c) + item->window.rectEffects.y - h;
			Item_UpdatePosition(item);

		}
	}


	if(item->window.flags & WINDOW_INTRANSITION)
	{
		if(DC->realTime > item->window.nextTime)
		{
			int             done = 0;

			item->window.nextTime = DC->realTime + item->window.offsetTime;
			// transition the x,y
			if(item->window.rectClient.x == item->window.rectEffects.x)
			{
				done++;
			}
			else
			{
				if(item->window.rectClient.x < item->window.rectEffects.x)
				{
					item->window.rectClient.x += item->window.rectEffects2.x;
					if(item->window.rectClient.x > item->window.rectEffects.x)
					{
						item->window.rectClient.x = item->window.rectEffects.x;
						done++;
					}
				}
				else
				{
					item->window.rectClient.x -= item->window.rectEffects2.x;
					if(item->window.rectClient.x < item->window.rectEffects.x)
					{
						item->window.rectClient.x = item->window.rectEffects.x;
						done++;
					}
				}
			}
			if(item->window.rectClient.y == item->window.rectEffects.y)
			{
				done++;
			}
			else
			{
				if(item->window.rectClient.y < item->window.rectEffects.y)
				{
					item->window.rectClient.y += item->window.rectEffects2.y;
					if(item->window.rectClient.y > item->window.rectEffects.y)
					{
						item->window.rectClient.y = item->window.rectEffects.y;
						done++;
					}
				}
				else
				{
					item->window.rectClient.y -= item->window.rectEffects2.y;
					if(item->window.rectClient.y < item->window.rectEffects.y)
					{
						item->window.rectClient.y = item->window.rectEffects.y;
						done++;
					}
				}
			}
			if(item->window.rectClient.w == item->window.rectEffects.w)
			{
				done++;
			}
			else
			{
				if(item->window.rectClient.w < item->window.rectEffects.w)
				{
					item->window.rectClient.w += item->window.rectEffects2.w;
					if(item->window.rectClient.w > item->window.rectEffects.w)
					{
						item->window.rectClient.w = item->window.rectEffects.w;
						done++;
					}
				}
				else
				{
					item->window.rectClient.w -= item->window.rectEffects2.w;
					if(item->window.rectClient.w < item->window.rectEffects.w)
					{
						item->window.rectClient.w = item->window.rectEffects.w;
						done++;
					}
				}
			}
			if(item->window.rectClient.h == item->window.rectEffects.h)
			{
				done++;
			}
			else
			{
				if(item->window.rectClient.h < item->window.rectEffects.h)
				{
					item->window.rectClient.h += item->window.rectEffects2.h;
					if(item->window.rectClient.h > item->window.rectEffects.h)
					{
						item->window.rectClient.h = item->window.rectEffects.h;
						done++;
					}
				}
				else
				{
					item->window.rectClient.h -= item->window.rectEffects2.h;
					if(item->window.rectClient.h < item->window.rectEffects.h)
					{
						item->window.rectClient.h = item->window.rectEffects.h;
						done++;
					}
				}
			}

			Item_UpdatePosition(item);

			if(done == 4)
			{
				item->window.flags &= ~WINDOW_INTRANSITION;
			}

		}
	}

	if(item->window.ownerDrawFlags && DC->ownerDrawVisible)
	{
		if(!DC->ownerDrawVisible(item->window.ownerDrawFlags))
		{
			item->window.flags &= ~(WINDOW_VISIBLE | WINDOW_MOUSEOVER);
		}
		else
		{
			item->window.flags |= WINDOW_VISIBLE;
		}
	}

	if(item->cvarFlags & (CVAR_SHOW | CVAR_HIDE))
	{
		if(!Item_EnableShowViaCvar(item, CVAR_SHOW))
		{
			return;
		}
	}

	// OSP
	if((item->settingFlags & (SVS_ENABLED_SHOW | SVS_DISABLED_SHOW)) && !Item_SettingShow(item, qfalse))
	{
		return;
	}
	if(item->voteFlag != 0 && !Item_SettingShow(item, qtrue))
	{
		return;
	}

	if(item->window.flags & WINDOW_TIMEDVISIBLE)
	{
	}

	if(!(item->window.flags & WINDOW_VISIBLE))
	{
		return;
	}

	// paint the rect first..
	Window_Paint(&item->window, parent->fadeAmount, parent->fadeClamp, parent->fadeCycle);

	if(debugMode)
	{
		vec4_t          color;
		rectDef_t      *r = Item_CorrectedTextRect(item);

		color[1] = color[3] = 1;
		color[0] = color[2] = 0;
		DC->drawRect(r->x, r->y, r->w, r->h, 1, color);
	}

	//DC->drawRect(item->window.rect.x, item->window.rect.y, item->window.rect.w, item->window.rect.h, 1, red);

	switch (item->type)
	{
		case ITEM_TYPE_OWNERDRAW:
			Item_OwnerDraw_Paint(item);
			break;
		case ITEM_TYPE_TEXT:
		case ITEM_TYPE_BUTTON:
		case ITEM_TYPE_TIMEOUT_COUNTER:
			Item_Text_Paint(item);
			break;
		case ITEM_TYPE_RADIOBUTTON:
			break;
		case ITEM_TYPE_CHECKBOX:
		case ITEM_TYPE_TRICHECKBOX:
			Item_CheckBox_Paint(item);
			break;
		case ITEM_TYPE_EDITFIELD:
		case ITEM_TYPE_NUMERICFIELD:
			Item_TextField_Paint(item);
			break;
		case ITEM_TYPE_COMBO:
			break;
		case ITEM_TYPE_LISTBOX:
			Item_ListBox_Paint(item);
			break;
//      case ITEM_TYPE_IMAGE:
//          Item_Image_Paint(item);
//          break;
		case ITEM_TYPE_MENUMODEL:
			Item_Model_Paint(item);
			break;
		case ITEM_TYPE_MODEL:
			Item_Model_Paint(item);
			break;
		case ITEM_TYPE_YESNO:
			Item_YesNo_Paint(item);
			break;
		case ITEM_TYPE_MULTI:
			Item_Multi_Paint(item);
			break;
		case ITEM_TYPE_BIND:
			Item_Bind_Paint(item);
			break;
		case ITEM_TYPE_SLIDER:
			Item_Slider_Paint(item);
			break;
		default:
			break;
	}
}

void Menu_Init(menuDef_t * menu)
{
	memset(menu, 0, sizeof(menuDef_t));
	menu->cursorItem = -1;
	menu->fadeAmount = DC->Assets.fadeAmount;
	menu->fadeClamp = DC->Assets.fadeClamp;
	menu->fadeCycle = DC->Assets.fadeCycle;
	// START - TAT 9/16/2002
	// by default, do NOT use item hotkey mode
	menu->itemHotkeyMode = qfalse;
	// END - TAT 9/16/2002
	Window_Init(&menu->window);
}

itemDef_t      *Menu_GetFocusedItem(menuDef_t * menu)
{
	int             i;

	if(menu)
	{
		for(i = 0; i < menu->itemCount; i++)
		{
			if(menu->items[i]->window.flags & WINDOW_HASFOCUS)
			{
				return menu->items[i];
			}
		}
	}
	return NULL;
}

menuDef_t      *Menu_GetFocused()
{
	int             i;

	for(i = 0; i < menuCount; i++)
	{
		if(Menus[i].window.flags & WINDOW_HASFOCUS && Menus[i].window.flags & WINDOW_VISIBLE)
		{
			return &Menus[i];
		}
	}
	return NULL;
}

void Menu_ScrollFeeder(menuDef_t * menu, int feeder, qboolean down)
{
	if(menu)
	{
		int             i;

		for(i = 0; i < menu->itemCount; i++)
		{
			if(menu->items[i]->special == feeder)
			{
				Item_ListBox_HandleKey(menu->items[i], (down) ? K_DOWNARROW : K_UPARROW, qtrue, qtrue);
				return;
			}
		}
	}
}



void Menu_SetFeederSelection(menuDef_t * menu, int feeder, int index, const char *name)
{
	if(menu == NULL)
	{
		if(name == NULL)
		{
			menu = Menu_GetFocused();
		}
		else
		{
			menu = Menus_FindByName(name);
		}
	}

	if(menu)
	{
		int             i;

		for(i = 0; i < menu->itemCount; i++)
		{
			if(menu->items[i]->special == feeder)
			{
				if(index == 0)
				{
					listBoxDef_t   *listPtr = (listBoxDef_t *) menu->items[i]->typeData;

					listPtr->cursorPos = 0;
					listPtr->startPos = 0;
				}
				menu->items[i]->cursorPos = index;
				DC->feederSelection(menu->items[i]->special, menu->items[i]->cursorPos);
				return;
			}
		}
	}
}

qboolean Menus_AnyFullScreenVisible()
{
	int             i;

	for(i = 0; i < menuCount; i++)
	{
		if(Menus[i].window.flags & WINDOW_VISIBLE && Menus[i].fullScreen)
		{
			return qtrue;
		}
	}
	return qfalse;
}

menuDef_t      *Menus_ActivateByName(const char *p, qboolean modalStack)
{
	int             i;
	menuDef_t      *m = NULL;
	menuDef_t      *focus = Menu_GetFocused();

	for(i = 0; i < menuCount; i++)
	{
		if(Q_stricmp(Menus[i].window.name, p) == 0)
		{
			m = &Menus[i];
			Menus_Activate(m);
			if(modalStack && m->window.flags & WINDOW_MODAL)
			{
				if(modalMenuCount >= MAX_MODAL_MENUS)
				{
					Com_Error(ERR_DROP, "MAX_MODAL_MENUS exceeded\n");
				}
				modalMenuStack[modalMenuCount++] = focus;
			}
			break;				// Arnout: found it, don't continue searching as we might unfocus the menu we just activated again.
		}
		else
		{
			Menus[i].window.flags &= ~(WINDOW_HASFOCUS | WINDOW_MOUSEOVER);
		}
	}
	Display_CloseCinematics();
	return m;
}


void Item_Init(itemDef_t * item)
{
	memset(item, 0, sizeof(itemDef_t));
	item->textscale = 0.55f;

	// default hotkey to -1
	item->hotkey = -1;

	Window_Init(&item->window);
}

void Menu_HandleMouseMove(menuDef_t * menu, float x, float y)
{
	int             i, pass;
	qboolean        focusSet = qfalse;

	itemDef_t      *overItem;

	if(menu == NULL)
	{
		return;
	}

	if(!(menu->window.flags & (WINDOW_VISIBLE | WINDOW_FORCED)))
	{
		return;
	}

	if(itemCapture)
	{
		if(itemCapture->type == ITEM_TYPE_LISTBOX)
		{
			// NERVE - SMF - lose capture if out of client rect
			if(!Rect_ContainsPoint(&itemCapture->window.rect, x, y))
			{
				Item_StopCapture(itemCapture);
				itemCapture = NULL;
				captureFunc = NULL;
				captureData = NULL;
			}

		}
		//Item_MouseMove(itemCapture, x, y);
		return;
	}

	if(g_waitingForKey || g_editingField)
	{
		return;
	}

	// FIXME: this is the whole issue of focus vs. mouse over..
	// need a better overall solution as i don't like going through everything twice
	for(pass = 0; pass < 2; pass++)
	{
		for(i = 0; i < menu->itemCount; i++)
		{
			// turn off focus each item
			// menu->items[i].window.flags &= ~WINDOW_HASFOCUS;

			if(!(menu->items[i]->window.flags & (WINDOW_VISIBLE | WINDOW_FORCED)))
			{
				continue;
			}

			// items can be enabled and disabled based on cvars
			if(menu->items[i]->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(menu->items[i], CVAR_ENABLE))
			{
				continue;
			}

			if(menu->items[i]->cvarFlags & (CVAR_SHOW | CVAR_HIDE) && !Item_EnableShowViaCvar(menu->items[i], CVAR_SHOW))
			{
				continue;
			}

			// OSP - server settings too
			if((menu->items[i]->settingFlags & (SVS_ENABLED_SHOW | SVS_DISABLED_SHOW)) &&
			   !Item_SettingShow(menu->items[i], qfalse))
			{
				continue;
			}
			if(menu->items[i]->voteFlag != 0 && !Item_SettingShow(menu->items[i], qtrue))
			{
				continue;
			}


			if(Rect_ContainsPoint(&menu->items[i]->window.rect, x, y))
			{
				if(pass == 1)
				{
					overItem = menu->items[i];
					if(overItem->type == ITEM_TYPE_TEXT && overItem->text)
					{
						if(!Rect_ContainsPoint(Item_CorrectedTextRect(overItem), x, y))
						{
							continue;
						}
					}
					// if we are over an item
					if(IsVisible(overItem->window.flags))
					{
						// different one
						Item_MouseEnter(overItem, x, y);
						// Item_SetMouseOver(overItem, qtrue);

						// if item is not a decoration see if it can take focus
						if(!focusSet)
						{
							focusSet = Item_SetFocus(overItem, x, y);
						}
					}
				}
			}
			else if(menu->items[i]->window.flags & WINDOW_MOUSEOVER)
			{
				Item_MouseLeave(menu->items[i]);
				Item_SetMouseOver(menu->items[i], qfalse);
			}
		}
	}
}

void Menu_Paint(menuDef_t * menu, qboolean forcePaint)
{
	int             i;
	itemDef_t      *item = NULL;


	if(menu == NULL)
	{
		return;
	}

	if(!(menu->window.flags & WINDOW_VISIBLE) && !forcePaint)
	{
		return;
	}

	if(menu->window.ownerDrawFlags && DC->ownerDrawVisible && !DC->ownerDrawVisible(menu->window.ownerDrawFlags))
	{
		return;
	}

	if(forcePaint)
	{
		menu->window.flags |= WINDOW_FORCED;
	}

	// draw the background if necessary
	if(menu->fullScreen)
	{
		// implies a background shader
		// FIXME: make sure we have a default shader if fullscreen is set with no background
		DC->drawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, menu->window.background);
	}
	else if(menu->window.background)
	{
		// this allows a background shader without being full screen
		//UI_DrawHandlePic(menu->window.rect.x, menu->window.rect.y, menu->window.rect.w, menu->window.rect.h, menu->backgroundShader);
	}

	// paint the background and or border
	Window_Paint(&menu->window, menu->fadeAmount, menu->fadeClamp, menu->fadeCycle);

	for(i = 0; i < menu->itemCount; i++)
	{
		Item_Paint(menu->items[i]);
		if(menu->items[i]->window.flags & WINDOW_MOUSEOVER)
		{
			item = menu->items[i];
		}
	}

	// OSP draw tooltip data if we have it
	if(DC->getCVarValue("ui_showtooltips") &&
	   item != NULL && item->toolTipData != NULL && item->toolTipData->text != NULL && *item->toolTipData->text)
	{
		Item_Paint(item->toolTipData);
	}

	// ydnar: handle timeout here
	if(menu->openTime == 0)
	{
		menu->openTime = DC->realTime;
	}
	else if(!menu->timedOut && menu->window.flags & WINDOW_VISIBLE &&
			menu->timeout > 0 && menu->onTimeout != NULL && menu->openTime + menu->timeout <= DC->realTime)
	{
		itemDef_t       it;

		it.parent = menu;
		Item_RunScript(&it, NULL, menu->onTimeout);
// Omni-bot BEGIN
		menu->timedOut = qtrue;
// Omni-bot END
	}

	if(debugMode)
	{
		vec4_t          color;

		color[0] = color[2] = color[3] = 1;
		color[1] = 0;
		DC->drawRect(menu->window.rect.x, menu->window.rect.y, menu->window.rect.w, menu->window.rect.h, 1, color);
	}
}

/*
===============
Item_ValidateTypeData
===============
*/
void Item_ValidateTypeData(itemDef_t * item)
{
	if(item->typeData)
	{
		return;
	}

	if(item->type == ITEM_TYPE_LISTBOX)
	{
		item->typeData = UI_Alloc(sizeof(listBoxDef_t));
		memset(item->typeData, 0, sizeof(listBoxDef_t));
	}
	else if(item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD || item->type == ITEM_TYPE_YESNO ||
			item->type == ITEM_TYPE_BIND || item->type == ITEM_TYPE_SLIDER || item->type == ITEM_TYPE_TEXT)
	{
		item->typeData = UI_Alloc(sizeof(editFieldDef_t));
		memset(item->typeData, 0, sizeof(editFieldDef_t));
		if(item->type == ITEM_TYPE_EDITFIELD)
		{
			if(!((editFieldDef_t *) item->typeData)->maxPaintChars)
			{
				((editFieldDef_t *) item->typeData)->maxPaintChars = MAX_EDITFIELD;
			}
		}
	}
	else if(item->type == ITEM_TYPE_MULTI || item->type == ITEM_TYPE_CHECKBOX || item->type == ITEM_TYPE_TRICHECKBOX)
	{
		item->typeData = UI_Alloc(sizeof(multiDef_t));
	}
	else if(item->type == ITEM_TYPE_MODEL)
	{
		item->typeData = UI_Alloc(sizeof(modelDef_t));
	}
	else if(item->type == ITEM_TYPE_MENUMODEL)
	{
		item->typeData = UI_Alloc(sizeof(modelDef_t));
	}
}

/*
========================
Item_ValidateTooltipData
========================
*/
qboolean Item_ValidateTooltipData(itemDef_t * item)
{
	if(item->toolTipData != NULL)
	{
		return (qtrue);
	}

	item->toolTipData = UI_Alloc(sizeof(itemDef_t));
	if(item->toolTipData == NULL)
	{
		return (qfalse);
	}

	Item_Init(item->toolTipData);
	Tooltip_Initialize(item->toolTipData);

	return (qtrue);
}

/*
===============
Keyword Hash
===============
*/

#define KEYWORDHASH_SIZE    512

typedef struct keywordHash_s
{
	char           *keyword;
	                qboolean(*func) (itemDef_t * item, int handle);
	struct keywordHash_s *next;
} keywordHash_t;

int KeywordHash_Key(char *keyword)
{
	register int    hash, i;

	hash = 0;
	for(i = 0; keyword[i] != '\0'; i++)
	{
		if(keyword[i] >= 'A' && keyword[i] <= 'Z')
		{
			hash += (keyword[i] + ('a' - 'A')) * (119 + i);
		}
		else
		{
			hash += keyword[i] * (119 + i);
		}
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20)) & (KEYWORDHASH_SIZE - 1);
	return hash;
}

void KeywordHash_Add(keywordHash_t * table[], keywordHash_t * key)
{
	int             hash;

	hash = KeywordHash_Key(key->keyword);
/*
	if (table[hash]) {
		int collision = qtrue;
	}
*/
	key->next = table[hash];
	table[hash] = key;
}

keywordHash_t  *KeywordHash_Find(keywordHash_t * table[], char *keyword)
{
	keywordHash_t  *key;
	int             hash;

	hash = KeywordHash_Key(keyword);
	for(key = table[hash]; key; key = key->next)
	{
		if(!Q_stricmp(key->keyword, keyword))
		{
			return key;
		}
	}
	return NULL;
}

/*
===============
Item Keyword Parse functions
===============
*/

// name <string>
qboolean ItemParse_name(itemDef_t * item, int handle)
{
	if(!PC_String_Parse(handle, &item->window.name))
	{
		return qfalse;
	}
	return qtrue;
}

// name <string>
qboolean ItemParse_focusSound(itemDef_t * item, int handle)
{
	const char     *temp = NULL;

	if(!PC_String_Parse(handle, &temp))
	{
		return qfalse;
	}
	item->focusSound = DC->registerSound(temp, qtrue);
	return qtrue;
}


// text <string>
qboolean ItemParse_text(itemDef_t * item, int handle)
{
	if(!PC_String_Parse(handle, &item->text))
	{
		return qfalse;
	}
	return qtrue;
}

//----(SA)  added

// textfile <string>
// read an external textfile into item->text
qboolean ItemParse_textfile(itemDef_t * item, int handle)
{
	const char     *newtext;
	pc_token_t      token;

	if(!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	newtext = DC->fileText(token.string);
	item->text = String_Alloc(newtext);

	return qtrue;
}

//----(SA)

// group <string>
qboolean ItemParse_group(itemDef_t * item, int handle)
{
	if(!PC_String_Parse(handle, &item->window.group))
	{
		return qfalse;
	}
	return qtrue;
}


// asset_model <string>
qboolean ItemParse_asset_model(itemDef_t * item, int handle)
{
	const char     *temp = NULL;
	modelDef_t     *modelPtr;

	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t *) item->typeData;

	if(!PC_String_Parse(handle, &temp))
	{
		return qfalse;
	}
	if(!(item->asset))
	{
		item->asset = DC->registerModel(temp);
//      modelPtr->angle = rand() % 360;
	}
	return qtrue;
}

// asset_shader <string>
qboolean ItemParse_asset_shader(itemDef_t * item, int handle)
{
	const char     *temp = NULL;

	if(!PC_String_Parse(handle, &temp))
	{
		return qfalse;
	}
	item->asset = DC->registerShaderNoMip(temp);
	return qtrue;
}

// model_origin <number> <number> <number>
qboolean ItemParse_model_origin(itemDef_t * item, int handle)
{
	modelDef_t     *modelPtr;

	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t *) item->typeData;

	if(PC_Float_Parse(handle, &modelPtr->origin[0]))
	{
		if(PC_Float_Parse(handle, &modelPtr->origin[1]))
		{
			if(PC_Float_Parse(handle, &modelPtr->origin[2]))
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

// model_fovx <number>
qboolean ItemParse_model_fovx(itemDef_t * item, int handle)
{
	modelDef_t     *modelPtr;

	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t *) item->typeData;

	if(!PC_Float_Parse(handle, &modelPtr->fov_x))
	{
		return qfalse;
	}
	return qtrue;
}

// model_fovy <number>
qboolean ItemParse_model_fovy(itemDef_t * item, int handle)
{
	modelDef_t     *modelPtr;

	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t *) item->typeData;

	if(!PC_Float_Parse(handle, &modelPtr->fov_y))
	{
		return qfalse;
	}
	return qtrue;
}

// model_rotation <integer>
qboolean ItemParse_model_rotation(itemDef_t * item, int handle)
{
	modelDef_t     *modelPtr;

	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t *) item->typeData;

	if(!PC_Int_Parse(handle, &modelPtr->rotationSpeed))
	{
		return qfalse;
	}
	return qtrue;
}

// model_angle <integer>
qboolean ItemParse_model_angle(itemDef_t * item, int handle)
{
	modelDef_t     *modelPtr;

	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t *) item->typeData;

	if(!PC_Int_Parse(handle, &modelPtr->angle))
	{
		return qfalse;
	}
	return qtrue;
}

// model_animplay <int(startframe)> <int(numframes)> <int(loopframes)> <int(fps)>
qboolean ItemParse_model_animplay(itemDef_t * item, int handle)
{
	modelDef_t     *modelPtr;

	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t *) item->typeData;

	modelPtr->animated = 1;

	if(!PC_Int_Parse(handle, &modelPtr->startframe))
	{
		return qfalse;
	}
	if(!PC_Int_Parse(handle, &modelPtr->numframes))
	{
		return qfalse;
	}
	if(!PC_Int_Parse(handle, &modelPtr->loopframes))
	{
		return qfalse;
	}
	if(!PC_Int_Parse(handle, &modelPtr->fps))
	{
		return qfalse;
	}

	modelPtr->frame = modelPtr->startframe + 1;
	modelPtr->oldframe = modelPtr->startframe;
	modelPtr->backlerp = 0.0f;
	modelPtr->frameTime = DC->realTime;
	return qtrue;
}


// rect <rectangle>
qboolean ItemParse_rect(itemDef_t * item, int handle)
{
	return (PC_Rect_Parse(handle, &item->window.rectClient));
}

// NERVE - SMF
// origin <integer, integer>
qboolean ItemParse_origin(itemDef_t * item, int handle)
{
	int             x = 0, y = 0;

	if(!PC_Int_Parse(handle, &x))
	{
		return qfalse;
	}
	if(!PC_Int_Parse(handle, &y))
	{
		return qfalse;
	}

	item->window.rectClient.x += x;
	item->window.rectClient.y += y;

	return qtrue;
}

// -NERVE - SMF

// style <integer>
qboolean ItemParse_style(itemDef_t * item, int handle)
{
	if(!PC_Int_Parse(handle, &item->window.style))
	{
		return qfalse;
	}
	return qtrue;
}

// decoration
qboolean ItemParse_decoration(itemDef_t * item, int handle)
{
	item->window.flags |= WINDOW_DECORATION;
	return qtrue;
}

// textasint
qboolean ItemParse_textasint(itemDef_t * item, int handle)
{
	item->window.flags |= WINDOW_TEXTASINT;
	return qtrue;
}


// textasfloat
qboolean ItemParse_textasfloat(itemDef_t * item, int handle)
{
	item->window.flags |= WINDOW_TEXTASFLOAT;
	return qtrue;
}


// notselectable
qboolean ItemParse_notselectable(itemDef_t * item, int handle)
{
	listBoxDef_t   *listPtr;

	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t *) item->typeData;
	if(item->type == ITEM_TYPE_LISTBOX && listPtr)
	{
		listPtr->notselectable = qtrue;
	}
	return qtrue;
}

// manually wrapped
qboolean ItemParse_wrapped(itemDef_t * item, int handle)
{
	item->window.flags |= WINDOW_WRAPPED;
	return qtrue;
}

// auto wrapped
qboolean ItemParse_autowrapped(itemDef_t * item, int handle)
{
	item->window.flags |= WINDOW_AUTOWRAPPED;
	return qtrue;
}


// horizontalscroll
qboolean ItemParse_horizontalscroll(itemDef_t * item, int handle)
{
	item->window.flags |= WINDOW_HORIZONTAL;
	return qtrue;
}

// type <integer>
qboolean ItemParse_type(itemDef_t * item, int handle)
{
	if(!PC_Int_Parse(handle, &item->type))
	{
		return qfalse;
	}
	Item_ValidateTypeData(item);
	return qtrue;
}

// elementwidth, used for listbox image elements
// uses textalignx for storage
qboolean ItemParse_elementwidth(itemDef_t * item, int handle)
{
	listBoxDef_t   *listPtr;

	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t *) item->typeData;
	if(!PC_Float_Parse(handle, &listPtr->elementWidth))
	{
		return qfalse;
	}
	return qtrue;
}

// elementheight, used for listbox image elements
// uses textaligny for storage
qboolean ItemParse_elementheight(itemDef_t * item, int handle)
{
	listBoxDef_t   *listPtr;

	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t *) item->typeData;
	if(!PC_Float_Parse(handle, &listPtr->elementHeight))
	{
		return qfalse;
	}
	return qtrue;
}

// feeder <float>
qboolean ItemParse_feeder(itemDef_t * item, int handle)
{
	if(!PC_Float_Parse(handle, &item->special))
	{
		return qfalse;
	}
	return qtrue;
}

// elementtype, used to specify what type of elements a listbox contains
// uses textstyle for storage
qboolean ItemParse_elementtype(itemDef_t * item, int handle)
{
	listBoxDef_t   *listPtr;

	Item_ValidateTypeData(item);
	if(!item->typeData)
	{
		return qfalse;
	}
	listPtr = (listBoxDef_t *) item->typeData;
	if(!PC_Int_Parse(handle, &listPtr->elementStyle))
	{
		return qfalse;
	}
	return qtrue;
}

// columns sets a number of columns and an x pos and width per..
qboolean ItemParse_columns(itemDef_t * item, int handle)
{
	int             num = 0, i;
	listBoxDef_t   *listPtr;

	Item_ValidateTypeData(item);
	if(!item->typeData)
	{
		return qfalse;
	}
	listPtr = (listBoxDef_t *) item->typeData;
	if(PC_Int_Parse(handle, &num))
	{
		if(num > MAX_LB_COLUMNS)
		{
			num = MAX_LB_COLUMNS;
		}
		listPtr->numColumns = num;
		for(i = 0; i < num; i++)
		{
			int             pos = 0, width = 0, maxChars = 0;

			if(PC_Int_Parse(handle, &pos) && PC_Int_Parse(handle, &width) && PC_Int_Parse(handle, &maxChars))
			{
				listPtr->columnInfo[i].pos = pos;
				listPtr->columnInfo[i].width = width;
				listPtr->columnInfo[i].maxChars = maxChars;
			}
			else
			{
				return qfalse;
			}
		}
	}
	else
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_border(itemDef_t * item, int handle)
{
	if(!PC_Int_Parse(handle, &item->window.border))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_bordersize(itemDef_t * item, int handle)
{
	if(!PC_Float_Parse(handle, &item->window.borderSize))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_visible(itemDef_t * item, int handle)
{
	int             i = 0;

	if(!PC_Int_Parse(handle, &i))
	{
		return qfalse;
	}
	if(i)
	{
		item->window.flags |= WINDOW_VISIBLE;
	}
	return qtrue;
}

qboolean ItemParse_ownerdraw(itemDef_t * item, int handle)
{
	if(!PC_Int_Parse(handle, &item->window.ownerDraw))
	{
		return qfalse;
	}
	item->type = ITEM_TYPE_OWNERDRAW;
	return qtrue;
}

qboolean ItemParse_align(itemDef_t * item, int handle)
{
	if(!PC_Int_Parse(handle, &item->alignment))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_textalign(itemDef_t * item, int handle)
{
	if(!PC_Int_Parse(handle, &item->textalignment))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_textalignx(itemDef_t * item, int handle)
{
	if(!PC_Float_Parse(handle, &item->textalignx))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_textaligny(itemDef_t * item, int handle)
{
	if(!PC_Float_Parse(handle, &item->textaligny))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_textscale(itemDef_t * item, int handle)
{
	if(!PC_Float_Parse(handle, &item->textscale))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_textstyle(itemDef_t * item, int handle)
{
	if(!PC_Int_Parse(handle, &item->textStyle))
	{
		return qfalse;
	}
	return qtrue;
}

//----(SA)  added for forcing a font for a given item
qboolean ItemParse_textfont(itemDef_t * item, int handle)
{
	if(!PC_Int_Parse(handle, &item->font))
	{
		return qfalse;
	}
	return qtrue;
}

//----(SA)  end

qboolean ItemParse_backcolor(itemDef_t * item, int handle)
{
	int             i;
	float           f = 0.0f;

	for(i = 0; i < 4; i++)
	{
		if(!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}
		item->window.backColor[i] = f;
	}
	return qtrue;
}

qboolean ItemParse_forecolor(itemDef_t * item, int handle)
{
	int             i;
	float           f = 0.0f;

	for(i = 0; i < 4; i++)
	{
		if(!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}
		item->window.foreColor[i] = f;
		item->window.flags |= WINDOW_FORECOLORSET;
	}
	return qtrue;
}

qboolean ItemParse_bordercolor(itemDef_t * item, int handle)
{
	int             i;
	float           f = 0.0f;

	for(i = 0; i < 4; i++)
	{
		if(!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}
		item->window.borderColor[i] = f;
	}
	return qtrue;
}

qboolean ItemParse_outlinecolor(itemDef_t * item, int handle)
{
	if(!PC_Color_Parse(handle, &item->window.outlineColor))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_background(itemDef_t * item, int handle)
{
	const char     *temp = NULL;

	if(!PC_String_Parse(handle, &temp))
	{
		return qfalse;
	}
	item->window.background = DC->registerShaderNoMip(temp);
	return qtrue;
}

qboolean ItemParse_cinematic(itemDef_t * item, int handle)
{
	if(!PC_String_Parse(handle, &item->window.cinematicName))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_doubleClick(itemDef_t * item, int handle)
{
	listBoxDef_t   *listPtr;

	Item_ValidateTypeData(item);
	if(!item->typeData)
	{
		return qfalse;
	}

	listPtr = (listBoxDef_t *) item->typeData;

	if(!PC_Script_Parse(handle, &listPtr->doubleClick))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_onEsc(itemDef_t * item, int handle)
{
	if(!PC_Script_Parse(handle, &item->onEsc))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_onEnter(itemDef_t * item, int handle)
{
	if(!PC_Script_Parse(handle, &item->onEnter))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_contextMenu(itemDef_t * item, int handle)
{
	listBoxDef_t   *listPtr;

	Item_ValidateTypeData(item);
	if(!item->typeData)
	{
		return qfalse;
	}

	listPtr = (listBoxDef_t *) item->typeData;

	if(!PC_String_Parse(handle, &listPtr->contextMenu))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_onFocus(itemDef_t * item, int handle)
{
	if(!PC_Script_Parse(handle, &item->onFocus))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_leaveFocus(itemDef_t * item, int handle)
{
	if(!PC_Script_Parse(handle, &item->leaveFocus))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_mouseEnter(itemDef_t * item, int handle)
{
	if(!PC_Script_Parse(handle, &item->mouseEnter))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_mouseExit(itemDef_t * item, int handle)
{
	if(!PC_Script_Parse(handle, &item->mouseExit))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_mouseEnterText(itemDef_t * item, int handle)
{
	if(!PC_Script_Parse(handle, &item->mouseEnterText))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_mouseExitText(itemDef_t * item, int handle)
{
	if(!PC_Script_Parse(handle, &item->mouseExitText))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_action(itemDef_t * item, int handle)
{
	if(!PC_Script_Parse(handle, &item->action))
	{
		return qfalse;
	}
	return qtrue;
}

// NERVE - SMF
qboolean ItemParse_accept(itemDef_t * item, int handle)
{
	if(!PC_Script_Parse(handle, &item->onAccept))
	{
		return qfalse;
	}
	return qtrue;
}

// -NERVE - SMF

qboolean ItemParse_special(itemDef_t * item, int handle)
{
	if(!PC_Float_Parse(handle, &item->special))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_cvarTest(itemDef_t * item, int handle)
{
	if(!PC_String_Parse(handle, &item->cvarTest))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean ItemParse_cvar(itemDef_t * item, int handle)
{
	editFieldDef_t *editPtr;

	Item_ValidateTypeData(item);
	if(!PC_String_Parse(handle, &item->cvar))
	{
		return qfalse;
	}
	Q_strlwr((char *)item->cvar);
	if(item->typeData)
	{
		editPtr = (editFieldDef_t *) item->typeData;
		editPtr->minVal = -1;
		editPtr->maxVal = -1;
		editPtr->defVal = -1;
	}
	return qtrue;
}

qboolean ItemParse_maxChars(itemDef_t * item, int handle)
{
	editFieldDef_t *editPtr;
	int             maxChars = 0;

	Item_ValidateTypeData(item);
	if(!item->typeData)
	{
		return qfalse;
	}

	if(!PC_Int_Parse(handle, &maxChars))
	{
		return qfalse;
	}
	editPtr = (editFieldDef_t *) item->typeData;
	editPtr->maxChars = maxChars;
	return qtrue;
}

qboolean ItemParse_maxPaintChars(itemDef_t * item, int handle)
{
	editFieldDef_t *editPtr;
	int             maxChars = 0;

	Item_ValidateTypeData(item);
	if(!item->typeData)
	{
		return qfalse;
	}

	if(!PC_Int_Parse(handle, &maxChars))
	{
		return qfalse;
	}
	editPtr = (editFieldDef_t *) item->typeData;
	editPtr->maxPaintChars = maxChars;
	return qtrue;
}



qboolean ItemParse_cvarFloat(itemDef_t * item, int handle)
{
	editFieldDef_t *editPtr;

	Item_ValidateTypeData(item);
	if(!item->typeData)
	{
		return qfalse;
	}
	editPtr = (editFieldDef_t *) item->typeData;
	if(PC_String_Parse(handle, &item->cvar) &&
	   PC_Float_Parse(handle, &editPtr->defVal) &&
	   PC_Float_Parse(handle, &editPtr->minVal) && PC_Float_Parse(handle, &editPtr->maxVal))
	{
		return qtrue;
	}
	return qfalse;
}

qboolean ItemParse_cvarStrList(itemDef_t * item, int handle)
{
	pc_token_t      token;
	multiDef_t     *multiPtr;
	int             pass;

	Item_ValidateTypeData(item);
	if(!item->typeData)
	{
		return qfalse;
	}
	multiPtr = (multiDef_t *) item->typeData;
	multiPtr->count = 0;
	multiPtr->strDef = qtrue;

	if(!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}
	if(*token.string != '{')
	{
		return qfalse;
	}

	pass = 0;
	while(1)
	{
		if(!trap_PC_ReadToken(handle, &token))
		{
			PC_SourceError(handle, "end of file inside menu item\n");
			return qfalse;
		}

		if(*token.string == '}')
		{
			return qtrue;
		}

		if(*token.string == ',' || *token.string == ';')
		{
			continue;
		}

		if(pass == 0)
		{
			multiPtr->cvarList[multiPtr->count] = String_Alloc(token.string);
			pass = 1;
		}
		else
		{
			multiPtr->cvarStr[multiPtr->count] = String_Alloc(token.string);
			pass = 0;
			multiPtr->count++;
			if(multiPtr->count >= MAX_MULTI_CVARS)
			{
				return qfalse;
			}
		}

	}
	return qfalse;				// bk001205 - LCC missing return value
}

qboolean ItemParse_cvarFloatList(itemDef_t * item, int handle)
{
	pc_token_t      token;
	multiDef_t     *multiPtr;

	Item_ValidateTypeData(item);
	if(!item->typeData)
	{
		return qfalse;
	}
	multiPtr = (multiDef_t *) item->typeData;
	multiPtr->count = 0;
	multiPtr->strDef = qfalse;

	if(!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}
	if(*token.string != '{')
	{
		return qfalse;
	}

	while(1)
	{
		if(!trap_PC_ReadToken(handle, &token))
		{
			PC_SourceError(handle, "end of file inside menu item\n");
			return qfalse;
		}

		if(*token.string == '}')
		{
			return qtrue;
		}

		if(*token.string == ',' || *token.string == ';')
		{
			continue;
		}

		multiPtr->cvarList[multiPtr->count] = String_Alloc(token.string);
		if(!PC_Float_Parse(handle, &multiPtr->cvarValue[multiPtr->count]))
		{
			return qfalse;
		}

		multiPtr->count++;
		if(multiPtr->count >= MAX_MULTI_CVARS)
		{
			return qfalse;
		}

	}
	return qfalse;				// bk001205 - LCC missing return value
}

qboolean ItemParse_cvarListUndefined(itemDef_t * item, int handle)
{
	pc_token_t      token;
	multiDef_t     *multiPtr;

	Item_ValidateTypeData(item);
	if(!item->typeData)
	{
		return qfalse;
	}
	multiPtr = (multiDef_t *) item->typeData;

	multiPtr->undefinedStr = NULL;

	if(!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	multiPtr->undefinedStr = String_Alloc(token.string);

	return qtrue;
}

qboolean ParseColorRange(itemDef_t * item, int handle, int type)
{
	colorRangeDef_t color;

	if(item->numColors && type != item->colorRangeType)
	{
		PC_SourceError(handle, "both addColorRange and addColorRangeRel - set within same itemdef\n");
		return qfalse;
	}

	item->colorRangeType = type;

	if(PC_Float_Parse(handle, &color.low) && PC_Float_Parse(handle, &color.high) && PC_Color_Parse(handle, &color.color))
	{
		if(item->numColors < MAX_COLOR_RANGES)
		{
			memcpy(&item->colorRanges[item->numColors], &color, sizeof(color));
			item->numColors++;
		}
		return qtrue;
	}
	return qfalse;
}

qboolean ItemParse_addColorRangeRel(itemDef_t * item, int handle)
{
	return ParseColorRange(item, handle, RANGETYPE_RELATIVE);
}

qboolean ItemParse_addColorRange(itemDef_t * item, int handle)
{
	return ParseColorRange(item, handle, RANGETYPE_ABSOLUTE);
}



qboolean ItemParse_ownerdrawFlag(itemDef_t * item, int handle)
{
	int             i = 0;

	if(!PC_Int_Parse(handle, &i))
	{
		return qfalse;
	}
	item->window.ownerDrawFlags |= i;
	return qtrue;
}

qboolean ItemParse_enableCvar(itemDef_t * item, int handle)
{
	if(PC_Script_Parse(handle, &item->enableCvar))
	{
		item->cvarFlags = CVAR_ENABLE;
		return qtrue;
	}
	return qfalse;
}

qboolean ItemParse_disableCvar(itemDef_t * item, int handle)
{
	if(PC_Script_Parse(handle, &item->enableCvar))
	{
		item->cvarFlags = CVAR_DISABLE;
		return qtrue;
	}
	return qfalse;
}

qboolean ItemParse_noToggle(itemDef_t * item, int handle)
{
	item->cvarFlags |= CVAR_NOTOGGLE;
	return qtrue;
}

qboolean ItemParse_showCvar(itemDef_t * item, int handle)
{
	if(PC_Script_Parse(handle, &item->enableCvar))
	{
		item->cvarFlags = CVAR_SHOW;
		return qtrue;
	}
	return qfalse;
}

qboolean ItemParse_hideCvar(itemDef_t * item, int handle)
{
	if(PC_Script_Parse(handle, &item->enableCvar))
	{
		item->cvarFlags = CVAR_HIDE;
		return qtrue;
	}
	return qfalse;
}

// START - TAT 9/16/2002
qboolean ItemParse_execKey(itemDef_t * item, int handle)
{
	char            keyname;

	// read in the hotkey
	if(!PC_Char_Parse(handle, &keyname))
	{
		return qfalse;
	}

	// store it in the hotkey field
	item->hotkey = keyname;

	// read in the command to execute
	if(!PC_Script_Parse(handle, &item->onKey))
	{
		return qfalse;
	}

	return qtrue;
}

// END - TAT 9/16/2002

// OSP - server setting tags
qboolean ItemParse_settingDisabled(itemDef_t * item, int handle)
{
	qboolean        fResult = PC_Int_Parse(handle, &item->settingTest);

	if(fResult)
	{
		item->settingFlags = SVS_DISABLED_SHOW;
	}
	return (fResult);
}

qboolean ItemParse_settingEnabled(itemDef_t * item, int handle)
{
	qboolean        fResult = PC_Int_Parse(handle, &item->settingTest);

	if(fResult)
	{
		item->settingFlags = SVS_ENABLED_SHOW;
	}
	return (fResult);
}

qboolean ItemParse_tooltip(itemDef_t * item, int handle)
{
	return (Item_ValidateTooltipData(item) && PC_String_Parse(handle, &item->toolTipData->text));
}

qboolean ItemParse_tooltipalignx(itemDef_t * item, int handle)
{
	return (Item_ValidateTooltipData(item) && PC_Float_Parse(handle, &item->toolTipData->textalignx));
}

qboolean ItemParse_tooltipaligny(itemDef_t * item, int handle)
{
	return (Item_ValidateTooltipData(item) && PC_Float_Parse(handle, &item->toolTipData->textaligny));
}

qboolean ItemParse_voteFlag(itemDef_t * item, int handle)
{
	return (PC_Int_Parse(handle, &item->voteFlag));
}

keywordHash_t   itemParseKeywords[] = {
	{"accept", ItemParse_accept, NULL}
	,							// NERVE - SMF
	{"action", ItemParse_action, NULL}
	,
	{"addColorRange", ItemParse_addColorRange, NULL}
	,
	{"addColorRangeRel", ItemParse_addColorRangeRel, NULL}
	,
	{"align", ItemParse_align, NULL}
	,
	{"asset_model", ItemParse_asset_model, NULL}
	,
	{"asset_shader", ItemParse_asset_shader, NULL}
	,
	{"autowrapped", ItemParse_autowrapped, NULL}
	,
	{"backcolor", ItemParse_backcolor, NULL}
	,
	{"background", ItemParse_background, NULL}
	,
	{"border", ItemParse_border, NULL}
	,
	{"bordercolor", ItemParse_bordercolor, NULL}
	,
	{"bordersize", ItemParse_bordersize, NULL}
	,
	{"cinematic", ItemParse_cinematic, NULL}
	,
	{"columns", ItemParse_columns, NULL}
	,
	{"contextmenu", ItemParse_contextMenu, NULL}
	,
	{"cvar", ItemParse_cvar, NULL}
	,
	{"cvarFloat", ItemParse_cvarFloat, NULL}
	,
	{"cvarFloatList", ItemParse_cvarFloatList, NULL}
	,
	{"cvarStrList", ItemParse_cvarStrList, NULL}
	,
	{"cvarListUndefined", ItemParse_cvarListUndefined, NULL}
	,
	{"cvarTest", ItemParse_cvarTest, NULL}
	,
	{"decoration", ItemParse_decoration, NULL}
	,
	{"textasint", ItemParse_textasint, NULL}
	,
	{"textasfloat", ItemParse_textasfloat, NULL}
	,
	{"disableCvar", ItemParse_disableCvar, NULL}
	,
	{"doubleclick", ItemParse_doubleClick, NULL}
	,
	{"onEsc", ItemParse_onEsc, NULL}
	,
	{"onEnter", ItemParse_onEnter, NULL}
	,
	{"elementheight", ItemParse_elementheight, NULL}
	,
	{"elementtype", ItemParse_elementtype, NULL}
	,
	{"elementwidth", ItemParse_elementwidth, NULL}
	,
	{"enableCvar", ItemParse_enableCvar, NULL}
	,
	{"execKey", ItemParse_execKey, NULL}
	,
	{"feeder", ItemParse_feeder, NULL}
	,
	{"focusSound", ItemParse_focusSound, NULL}
	,
	{"forecolor", ItemParse_forecolor, NULL}
	,
	{"group", ItemParse_group, NULL}
	,
	{"hideCvar", ItemParse_hideCvar, NULL}
	,
	{"horizontalscroll", ItemParse_horizontalscroll, NULL}
	,
	{"leaveFocus", ItemParse_leaveFocus, NULL}
	,
	{"maxChars", ItemParse_maxChars, NULL}
	,
	{"maxPaintChars", ItemParse_maxPaintChars, NULL}
	,
	{"model_angle", ItemParse_model_angle, NULL}
	,
	{"model_animplay", ItemParse_model_animplay, NULL}
	,
	{"model_fovx", ItemParse_model_fovx, NULL}
	,
	{"model_fovy", ItemParse_model_fovy, NULL}
	,
	{"model_origin", ItemParse_model_origin, NULL}
	,
	{"model_rotation", ItemParse_model_rotation, NULL}
	,
	{"mouseEnter", ItemParse_mouseEnter, NULL}
	,
	{"mouseEnterText", ItemParse_mouseEnterText, NULL}
	,
	{"mouseExit", ItemParse_mouseExit, NULL}
	,
	{"mouseExitText", ItemParse_mouseExitText, NULL}
	,
	{"name", ItemParse_name, NULL}
	,
	{"noToggle", ItemParse_noToggle, NULL}
	,							// TTimo: use with ITEM_TYPE_YESNO and an action script (see sv_punkbuster)
	{"notselectable", ItemParse_notselectable, NULL}
	,
	{"onFocus", ItemParse_onFocus, NULL}
	,
	{"origin", ItemParse_origin, NULL}
	,							// NERVE - SMF
	{"outlinecolor", ItemParse_outlinecolor, NULL}
	,
	{"ownerdraw", ItemParse_ownerdraw, NULL}
	,
	{"ownerdrawFlag", ItemParse_ownerdrawFlag, NULL}
	,
	{"rect", ItemParse_rect, NULL}
	,
	{"settingDisabled", ItemParse_settingDisabled, NULL}
	,							// OSP
	{"settingEnabled", ItemParse_settingEnabled, NULL}
	,							// OSP
	{"showCvar", ItemParse_showCvar, NULL}
	,
	{"special", ItemParse_special, NULL}
	,
	{"style", ItemParse_style, NULL}
	,
	{"text", ItemParse_text, NULL}
	,
	{"textalign", ItemParse_textalign, NULL}
	,
	{"textalignx", ItemParse_textalignx, NULL}
	,
	{"textaligny", ItemParse_textaligny, NULL}
	,
	{"textfile", ItemParse_textfile, NULL}
	,							//----(SA) added
	{"textfont", ItemParse_textfont, NULL}
	,							// (SA)
	{"textscale", ItemParse_textscale, NULL}
	,
	{"textstyle", ItemParse_textstyle, NULL}
	,
	{"tooltip", ItemParse_tooltip, NULL}
	,
	{"tooltipalignx", ItemParse_tooltipalignx, NULL}
	,
	{"tooltipaligny", ItemParse_tooltipaligny, NULL}
	,
	{"type", ItemParse_type, NULL}
	,
	{"visible", ItemParse_visible, NULL}
	,
	{"voteFlag", ItemParse_voteFlag, NULL}
	,							// OSP - vote check
	{"wrapped", ItemParse_wrapped, NULL}
	,

	{NULL, NULL, NULL}
};

keywordHash_t  *itemParseKeywordHash[KEYWORDHASH_SIZE];

/*
===============
Item_SetupKeywordHash
===============
*/
void Item_SetupKeywordHash(void)
{
	int             i;

	memset(itemParseKeywordHash, 0, sizeof(itemParseKeywordHash));
	for(i = 0; itemParseKeywords[i].keyword; i++)
	{
		KeywordHash_Add(itemParseKeywordHash, &itemParseKeywords[i]);
	}
}

/*
===============
Item_Parse
===============
*/
qboolean Item_Parse(int handle, itemDef_t * item)
{
	pc_token_t      token;
	keywordHash_t  *key;


	if(!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}
	if(*token.string != '{')
	{
		return qfalse;
	}
	while(1)
	{
		if(!trap_PC_ReadToken(handle, &token))
		{
			PC_SourceError(handle, "end of file inside menu item\n");
			return qfalse;
		}

		if(*token.string == '}')
		{
			return qtrue;
		}

		key = KeywordHash_Find(itemParseKeywordHash, token.string);
		if(!key)
		{
			PC_SourceError(handle, "unknown menu item keyword %s", token.string);
			continue;
		}
		if(!key->func(item, handle))
		{
			PC_SourceError(handle, "couldn't parse menu item keyword %s", token.string);
			return qfalse;
		}
	}
	return qfalse;				// bk001205 - LCC missing return value
}


// Item_InitControls
// init's special control types
void Item_InitControls(itemDef_t * item)
{
	if(item == NULL)
	{
		return;
	}
	if(item->type == ITEM_TYPE_LISTBOX)
	{
		listBoxDef_t   *listPtr = (listBoxDef_t *) item->typeData;

		item->cursorPos = 0;
		if(listPtr)
		{
			listPtr->cursorPos = 0;
			listPtr->startPos = 0;
			listPtr->endPos = 0;
		}
	}

	if(item->toolTipData != NULL)
	{
		Tooltip_ComputePosition(item);
	}
}

/*
===============
Menu Keyword Parse functions
===============
*/

qboolean MenuParse_name(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_String_Parse(handle, &menu->window.name))
	{
		return qfalse;
	}
	if(Q_stricmp(menu->window.name, "main") == 0)
	{
		// default main as having focus
		//menu->window.flags |= WINDOW_HASFOCUS;
	}
	return qtrue;
}

qboolean MenuParse_fullscreen(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Int_Parse(handle, (int *)&menu->fullScreen))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean MenuParse_rect(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	return (PC_Rect_Parse(handle, &menu->window.rect));
}

qboolean MenuParse_style(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Int_Parse(handle, &menu->window.style))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean MenuParse_visible(itemDef_t * item, int handle)
{
	int             i = 0;
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Int_Parse(handle, &i))
	{
		return qfalse;
	}
	if(i)
	{
		menu->window.flags |= WINDOW_VISIBLE;
	}
	return qtrue;
}

qboolean MenuParse_onOpen(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Script_Parse(handle, &menu->onOpen))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean MenuParse_onClose(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Script_Parse(handle, &menu->onClose))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean MenuParse_onESC(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Script_Parse(handle, &menu->onESC))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean MenuParse_onEnter(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Script_Parse(handle, &menu->onEnter))
	{
		return qfalse;
	}
	return qtrue;
}

// ydnar: menu timeout function
qboolean MenuParse_onTimeout(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Int_Parse(handle, &menu->timeout))
	{
		return qfalse;
	}
	if(!PC_Script_Parse(handle, &menu->onTimeout))
	{
		return qfalse;
	}
	return qtrue;
}



qboolean MenuParse_border(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Int_Parse(handle, &menu->window.border))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean MenuParse_borderSize(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Float_Parse(handle, &menu->window.borderSize))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean MenuParse_backcolor(itemDef_t * item, int handle)
{
	int             i;
	float           f = 0.0f;
	menuDef_t      *menu = (menuDef_t *) item;

	for(i = 0; i < 4; i++)
	{
		if(!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}
		menu->window.backColor[i] = f;
	}
	return qtrue;
}

qboolean MenuParse_forecolor(itemDef_t * item, int handle)
{
	int             i;
	float           f = 0.0f;
	menuDef_t      *menu = (menuDef_t *) item;

	for(i = 0; i < 4; i++)
	{
		if(!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}
		menu->window.foreColor[i] = f;
		menu->window.flags |= WINDOW_FORECOLORSET;
	}
	return qtrue;
}

qboolean MenuParse_bordercolor(itemDef_t * item, int handle)
{
	int             i;
	float           f = 0.0f;
	menuDef_t      *menu = (menuDef_t *) item;

	for(i = 0; i < 4; i++)
	{
		if(!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}
		menu->window.borderColor[i] = f;
	}
	return qtrue;
}

qboolean MenuParse_focuscolor(itemDef_t * item, int handle)
{
	int             i;
	float           f = 0.0f;
	menuDef_t      *menu = (menuDef_t *) item;

	for(i = 0; i < 4; i++)
	{
		if(!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}
		menu->focusColor[i] = f;
	}
	item->window.flags |= WINDOW_FOCUSPULSE;
	return qtrue;
}

qboolean MenuParse_disablecolor(itemDef_t * item, int handle)
{
	int             i;
	float           f = 0.0f;
	menuDef_t      *menu = (menuDef_t *) item;

	for(i = 0; i < 4; i++)
	{
		if(!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}
		menu->disableColor[i] = f;
	}
	return qtrue;
}


qboolean MenuParse_outlinecolor(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Color_Parse(handle, &menu->window.outlineColor))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean MenuParse_background(itemDef_t * item, int handle)
{
	const char     *buff = NULL;
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_String_Parse(handle, &buff))
	{
		return qfalse;
	}
	menu->window.background = DC->registerShaderNoMip(buff);
	return qtrue;
}

qboolean MenuParse_cinematic(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_String_Parse(handle, &menu->window.cinematicName))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean MenuParse_ownerdrawFlag(itemDef_t * item, int handle)
{
	int             i = 0;
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Int_Parse(handle, &i))
	{
		return qfalse;
	}
	menu->window.ownerDrawFlags |= i;
	return qtrue;
}

qboolean MenuParse_ownerdraw(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Int_Parse(handle, &menu->window.ownerDraw))
	{
		return qfalse;
	}
	return qtrue;
}


// decoration
qboolean MenuParse_popup(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	menu->window.flags |= WINDOW_POPUP;
	return qtrue;
}


qboolean MenuParse_outOfBounds(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	menu->window.flags |= WINDOW_OOB_CLICK;
	return qtrue;
}

qboolean MenuParse_soundLoop(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_String_Parse(handle, &menu->soundName))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean MenuParse_fadeClamp(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Float_Parse(handle, &menu->fadeClamp))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean MenuParse_fadeAmount(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Float_Parse(handle, &menu->fadeAmount))
	{
		return qfalse;
	}
	return qtrue;
}


qboolean MenuParse_fadeCycle(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Int_Parse(handle, &menu->fadeCycle))
	{
		return qfalse;
	}
	return qtrue;
}


qboolean MenuParse_itemDef(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	if(menu->itemCount < MAX_MENUITEMS)
	{
		menu->items[menu->itemCount] = UI_Alloc(sizeof(itemDef_t));
		Item_Init(menu->items[menu->itemCount]);
		if(!Item_Parse(handle, menu->items[menu->itemCount]))
		{
			return qfalse;
		}
		menu->items[menu->itemCount]->parent = menu;
		Item_InitControls(menu->items[menu->itemCount++]);

		// START - TAT 9/16/2002
		// If we are storing the hotkeys in the items, we have a little problem, in that
		//      people check with the menu to see if we have a hotkey (see UI_CheckExecKey)
		//      So we sort of need to stuff the hotkey back into the menu for that to work
		//      only do this at all if we're using the item hotkey mode
		//      NOTE:  we couldn't do this earlier because the menu wasn't set, and I don't know
		//      what would happen if we tried to set the menu before the parse had succeeded...
		if(menu->itemHotkeyMode && menu->items[menu->itemCount - 1]->hotkey >= 0)
		{
			menu->onKey[menu->items[menu->itemCount - 1]->hotkey] = String_Alloc(menu->items[menu->itemCount - 1]->onKey);
		}
		// END - TAT 9/16/2002
	}
	return qtrue;
}

// NERVE - SMF
qboolean MenuParse_execKey(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;
	char            keyname = 0;
	short int       keyindex;

	if(!PC_Char_Parse(handle, &keyname))
	{
		return qfalse;
	}
	keyindex = keyname;

	if(!PC_Script_Parse(handle, &menu->onKey[keyindex]))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_execKeyInt(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;
	int             keyname = 0;

	if(!PC_Int_Parse(handle, &keyname))
	{
		return qfalse;
	}

	if(!PC_Script_Parse(handle, &menu->onKey[keyname]))
	{
		return qfalse;
	}
	return qtrue;
}

// -NERVE - SMF

qboolean MenuParse_drawAlwaysOnTop(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	menu->window.flags |= WINDOW_DRAWALWAYSONTOP;
	return qtrue;
}

// START - TAT 9/16/2002
// parse the command to set if we're looping through all items to find the current hotkey
qboolean MenuParse_itemHotkeyMode(itemDef_t * item, int handle)
{
	// like MenuParse_fullscreen - reading an int
	menuDef_t      *menu = (menuDef_t *) item;

	if(!PC_Int_Parse(handle, (int *)&menu->itemHotkeyMode))
	{
		return qfalse;
	}
	return qtrue;
}

// END - TAT 9/16/2002

// TTimo
qboolean MenuParse_modal(itemDef_t * item, int handle)
{
	menuDef_t      *menu = (menuDef_t *) item;

	menu->window.flags |= WINDOW_MODAL;
	return qtrue;
}

keywordHash_t   menuParseKeywords[] = {
	{"name", MenuParse_name, NULL}
	,
	{"fullscreen", MenuParse_fullscreen, NULL}
	,
	{"rect", MenuParse_rect, NULL}
	,
	{"style", MenuParse_style, NULL}
	,
	{"visible", MenuParse_visible, NULL}
	,
	{"onOpen", MenuParse_onOpen, NULL}
	,
	{"onClose", MenuParse_onClose, NULL}
	,
	{"onTimeout", MenuParse_onTimeout, NULL}
	,							// ydnar: menu timeout function
	{"onESC", MenuParse_onESC, NULL}
	,
	{"onEnter", MenuParse_onEnter, NULL}
	,
	{"border", MenuParse_border, NULL}
	,
	{"borderSize", MenuParse_borderSize, NULL}
	,
	{"backcolor", MenuParse_backcolor, NULL}
	,
	{"forecolor", MenuParse_forecolor, NULL}
	,
	{"bordercolor", MenuParse_bordercolor, NULL}
	,
	{"focuscolor", MenuParse_focuscolor, NULL}
	,
	{"disablecolor", MenuParse_disablecolor, NULL}
	,
	{"outlinecolor", MenuParse_outlinecolor, NULL}
	,
	{"background", MenuParse_background, NULL}
	,
	{"ownerdraw", MenuParse_ownerdraw, NULL}
	,
	{"ownerdrawFlag", MenuParse_ownerdrawFlag, NULL}
	,
	{"outOfBoundsClick", MenuParse_outOfBounds, NULL}
	,
	{"soundLoop", MenuParse_soundLoop, NULL}
	,
	{"itemDef", MenuParse_itemDef, NULL}
	,
	{"cinematic", MenuParse_cinematic, NULL}
	,
	{"popup", MenuParse_popup, NULL}
	,
	{"fadeClamp", MenuParse_fadeClamp, NULL}
	,
	{"fadeCycle", MenuParse_fadeCycle, NULL}
	,
	{"fadeAmount", MenuParse_fadeAmount, NULL}
	,
	{"execKey", MenuParse_execKey, NULL}
	,							// NERVE - SMF
	{"execKeyInt", MenuParse_execKeyInt, NULL}
	,							// NERVE - SMF
	{"alwaysontop", MenuParse_drawAlwaysOnTop, NULL}
	,
	{"modal", MenuParse_modal, NULL}
	,

// START - TAT 9/16/2002
	// parse the command to set if we're looping through all items to find the current hotkey
	{"itemHotkeyMode", MenuParse_itemHotkeyMode, NULL}
	,
// END - TAT 9/16/2002
	{NULL, NULL, NULL}
};

keywordHash_t  *menuParseKeywordHash[KEYWORDHASH_SIZE];

/*
===============
Menu_SetupKeywordHash
===============
*/
void Menu_SetupKeywordHash(void)
{
	int             i;

	memset(menuParseKeywordHash, 0, sizeof(menuParseKeywordHash));
	for(i = 0; menuParseKeywords[i].keyword; i++)
	{
		KeywordHash_Add(menuParseKeywordHash, &menuParseKeywords[i]);
	}
}

/*
===============
Menu_Parse
===============
*/
qboolean Menu_Parse(int handle, menuDef_t * menu)
{
	pc_token_t      token;
	keywordHash_t  *key;

	if(!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}
	if(*token.string != '{')
	{
		return qfalse;
	}

	while(1)
	{

		memset(&token, 0, sizeof(pc_token_t));
		if(!trap_PC_ReadToken(handle, &token))
		{
			PC_SourceError(handle, "end of file inside menu\n");
			return qfalse;
		}

		if(*token.string == '}')
		{
			return qtrue;
		}

		key = KeywordHash_Find(menuParseKeywordHash, token.string);
		if(!key)
		{
			PC_SourceError(handle, "unknown menu keyword %s", token.string);
			continue;
		}
		if(!key->func((itemDef_t *) menu, handle))
		{
			PC_SourceError(handle, "couldn't parse menu keyword %s", token.string);
			return qfalse;
		}
	}
	return qfalse;				// bk001205 - LCC missing return value
}

/*
===============
Menu_New
===============
*/
void Menu_New(int handle)
{
	menuDef_t      *menu = &Menus[menuCount];

	if(menuCount < MAX_MENUS)
	{
		Menu_Init(menu);
		if(Menu_Parse(handle, menu))
		{
			Menu_PostParse(menu);
			menuCount++;
		}
	}
}

int Menu_Count()
{
	return menuCount;
}

menuDef_t      *Menu_Get(int handle)
{
	if(handle >= 0 && handle < menuCount)
	{
		return &Menus[handle];
	}
	else
	{
		return NULL;
	}
}

void Menu_PaintAll()
{
	int             i;

	if(captureFunc)
	{
		captureFunc(captureData);
	}

	for(i = 0; i < menuCount; i++)
	{
		if(Menus[i].window.flags & WINDOW_DRAWALWAYSONTOP)
		{
			continue;
		}
		Menu_Paint(&Menus[i], qfalse);
	}

	for(i = 0; i < menuCount; i++)
	{
		if(Menus[i].window.flags & WINDOW_DRAWALWAYSONTOP)
		{
			Menu_Paint(&Menus[i], qfalse);
		}
	}

	if(debugMode)
	{
		vec4_t          v = { 1, 1, 1, 1 };
		DC->textFont(UI_FONT_COURBD_21);
		DC->drawText(5, 10, .2, v, va("fps: %.2f", DC->FPS), 0, 0, 0);
		DC->drawText(5, 20, .2, v, va("mouse: %i %i", DC->cursorx, DC->cursory), 0, 0, 0);
	}
}

void Menu_Reset()
{
	menuCount = 0;
}

displayContextDef_t *Display_GetContext()
{
	return DC;
}

//static float captureX; // TTimo: unused
//static float captureY; // TTimo: unused

void           *Display_CaptureItem(int x, int y)
{
	int             i;

	for(i = 0; i < menuCount; i++)
	{
		// turn off focus each item
		// menu->items[i].window.flags &= ~WINDOW_HASFOCUS;
		if(Rect_ContainsPoint(&Menus[i].window.rect, x, y))
		{
			return &Menus[i];
		}
	}
	return NULL;
}


// FIXME:
qboolean Display_MouseMove(void *p, int x, int y)
{
	int             i;
	menuDef_t      *menu = p;

//  menu = Menu_GetFocused();

	if(menu == NULL)
	{
		menu = Menu_GetFocused();
		if(menu)
		{
			if(menu->window.flags & WINDOW_POPUP)
			{
				Menu_HandleMouseMove(menu, x, y);
				return qtrue;
			}
		}
		for(i = 0; i < menuCount; i++)
		{
			Menu_HandleMouseMove(&Menus[i], x, y);
		}
	}
	else
	{
		menu->window.rect.x += x;
		menu->window.rect.y += y;
		Menu_UpdatePosition(menu);
	}
	return qtrue;

}

int Display_CursorType(int x, int y)
{
	int             i;

	for(i = 0; i < menuCount; i++)
	{
		rectDef_t       r2;

		r2.x = Menus[i].window.rect.x - 3;
		r2.y = Menus[i].window.rect.y - 3;
		r2.w = r2.h = 7;
		if(Rect_ContainsPoint(&r2, x, y))
		{
			return CURSOR_SIZER;
		}
	}
	return CURSOR_ARROW;
}


void Display_HandleKey(int key, qboolean down, int x, int y)
{
	menuDef_t      *menu = Display_CaptureItem(x, y);

	if(menu == NULL)
	{
		menu = Menu_GetFocused();
	}
	if(menu)
	{
		Menu_HandleKey(menu, key, down);
	}
}

static void Window_CacheContents(windowDef_t * window)
{
	if(window)
	{
		if(window->cinematicName)
		{
			int             cin = DC->playCinematic(window->cinematicName, 0, 0, 0, 0);

			DC->stopCinematic(cin);
		}
	}
}


static void Item_CacheContents(itemDef_t * item)
{
	if(item)
	{
		Window_CacheContents(&item->window);
	}

}

static void Menu_CacheContents(menuDef_t * menu)
{
	if(menu)
	{
		int             i;

		Window_CacheContents(&menu->window);
		for(i = 0; i < menu->itemCount; i++)
		{
			Item_CacheContents(menu->items[i]);
		}

		if(menu->soundName && *menu->soundName)
		{
			DC->registerSound(menu->soundName, qtrue);
		}
	}

}

void Display_CacheAll()
{
	int             i;

	for(i = 0; i < menuCount; i++)
	{
		Menu_CacheContents(&Menus[i]);
	}
}


static qboolean Menu_OverActiveItem(menuDef_t * menu, float x, float y)
{
	if(menu && menu->window.flags & (WINDOW_VISIBLE | WINDOW_FORCED))
	{
		if(Rect_ContainsPoint(&menu->window.rect, x, y))
		{
			int             i;

			for(i = 0; i < menu->itemCount; i++)
			{
				// turn off focus each item
				// menu->items[i]->window.flags &= ~WINDOW_HASFOCUS;

				if(!(menu->items[i]->window.flags & (WINDOW_VISIBLE | WINDOW_FORCED)))
				{
					continue;
				}

				if(menu->items[i]->window.flags & WINDOW_DECORATION)
				{
					continue;
				}

				if(Rect_ContainsPoint(&menu->items[i]->window.rect, x, y))
				{
					itemDef_t      *overItem = menu->items[i];

					if(overItem->type == ITEM_TYPE_TEXT && overItem->text)
					{
						if(Rect_ContainsPoint(Item_CorrectedTextRect(overItem), x, y))
						{
							return qtrue;
						}
						else
						{
							continue;
						}
					}
					else
					{
						return qtrue;
					}
				}
			}

		}
	}
	return qfalse;
}

/*
=================
PC_String_Parse_Trans

NERVE - SMF - translates string
=================
*/
qboolean PC_String_Parse_Trans(int handle, const char **out)
{
	pc_token_t      token;

	if(!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	*(out) = String_Alloc(DC->translateString(token.string));
	return qtrue;
}

/*
=================
PC_Rect_Parse
=================
*/
qboolean PC_Rect_Parse(int handle, rectDef_t * r)
{
	if(PC_Float_Parse(handle, &r->x))
	{
		if(PC_Float_Parse(handle, &r->y))
		{
			if(PC_Float_Parse(handle, &r->w))
			{
				if(PC_Float_Parse(handle, &r->h))
				{
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

// digibob
// Panel Handling
// ======================================================
panel_button_t *bg_focusButton;

qboolean BG_RectContainsPoint(float x, float y, float w, float h, float px, float py)
{
	if(px > x && px < x + w && py > y && py < y + h)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean BG_CursorInRect(rectDef_t * rect)
{
	return BG_RectContainsPoint(rect->x, rect->y, rect->w, rect->h, DC->cursorx, DC->cursory);
}

void BG_PanelButton_RenderEdit(panel_button_t * button)
{
	qboolean        useCvar = button->data[0] ? qfalse : qtrue;
	int             offset = -1;

	if(useCvar)
	{
		char            buffer[256 + 1];

		trap_Cvar_VariableStringBuffer(button->text, buffer, sizeof(buffer));

		if(BG_PanelButtons_GetFocusButton() == button && ((DC->realTime / 1000) % 2))
		{
			if(trap_Key_GetOverstrikeMode())
			{
				Q_strcat(buffer, sizeof(buffer), "^0|");
			}
			else
			{
				Q_strcat(buffer, sizeof(buffer), "^0_");
			}
		}
		else
		{
			Q_strcat(buffer, sizeof(buffer), " ");
		}

		do
		{
			offset++;
			if(buffer + offset == '\0')
			{
				break;
			}
		} while(DC->textWidthExt(buffer + offset, button->font->scalex, 0, button->font->font) > button->rect.w);

		DC->drawTextExt(button->rect.x, button->rect.y + button->rect.h, button->font->scalex, button->font->scaley,
						button->font->colour, va("^7%s", buffer + offset), 0, 0, button->font->style, button->font->font);
	}
	else
	{
		char           *s;

		if(BG_PanelButtons_GetFocusButton() == button && ((DC->realTime / 1000) % 2))
		{
			if(DC->getOverstrikeMode())
			{
				s = va("^7%s^0|", button->text);
			}
			else
			{
				s = va("^7%s^0_", button->text);
			}
		}
		else
		{
			s = va("^7%s ", button->text);	// space hack to make the text not blink
		}

		do
		{
			offset++;
			if(s + offset == '\0')
			{
				break;
			}
		} while(DC->textWidthExt(s + offset, button->font->scalex, 0, button->font->font) > button->rect.w);

		DC->drawTextExt(button->rect.x, button->rect.y + button->rect.h, button->font->scalex, button->font->scaley,
						button->font->colour, s + offset, 0, 0, button->font->style, button->font->font);
	}
}

qboolean BG_PanelButton_EditClick(panel_button_t * button, int key)
{
	if(key == K_MOUSE1)
	{
		if(!BG_CursorInRect(&button->rect) && BG_PanelButtons_GetFocusButton() == button)
		{
			BG_PanelButtons_SetFocusButton(NULL);
			if(button->onFinish)
			{
				button->onFinish(button);
			}
			return qfalse;
		}
		else
		{
			BG_PanelButtons_SetFocusButton(button);
			return qtrue;
		}
	}
	else if(BG_PanelButtons_GetFocusButton() != button)
	{
		return qfalse;
	}
	else
	{
		char            buffer[256];
		char           *s = NULL;
		int             len, maxlen;
		qboolean        useCvar = button->data[0] ? qfalse : qtrue;

		if(useCvar)
		{
			maxlen = sizeof(buffer);
			DC->getCVarString(button->text, buffer, sizeof(buffer));
			len = strlen(buffer);
		}
		else
		{
			maxlen = button->data[0];
			s = (char *)button->text;
			len = strlen(s);
		}

		if(key & K_CHAR_FLAG)
		{
			key &= ~K_CHAR_FLAG;

			if(key == 'h' - 'a' + 1)
			{					// ctrl-h is backspace
				if(len)
				{
					if(useCvar)
					{
						buffer[len - 1] = '\0';
						DC->setCVar(button->text, buffer);
					}
					else
					{
						s[len - 1] = '\0';
					}
				}
				return qtrue;
			}

			if(key < 32 || key == 127)
			{
				return qtrue;
			}

			if(button->data[1])
			{
				if(key < '0' || key > '9')
				{
					if(button->data[1] == 2)
					{
						return qtrue;
					}
					else if(!(len == 0 && key == '-'))
					{
						return qtrue;
					}
				}
			}

			if(len >= maxlen - 1)
			{
				return qtrue;
			}

			if(useCvar)
			{
				buffer[len] = key;
				buffer[len + 1] = '\0';
				trap_Cvar_Set(button->text, buffer);
			}
			else
			{
				s[len] = key;
				s[len + 1] = '\0';
			}
			return qtrue;
		}
		else
		{
			// Gordon: FIXME: have this work with all our stuff (use data[x] to store cursorpos etc)
/*			if ( key == K_DEL || key == K_KP_DEL ) {
				if ( item->cursorPos < len ) {
					memmove( buff + item->cursorPos, buff + item->cursorPos + 1, len - item->cursorPos);
					DC->setCVar(item->cvar, buff);
				}
				return qtrue;
			}*/

/*			if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW )
			{
				if (editPtr->maxPaintChars && item->cursorPos >= editPtr->paintOffset + editPtr->maxPaintChars && item->cursorPos < len) {
					item->cursorPos++;
					editPtr->paintOffset++;
					return qtrue;
				}
				if (item->cursorPos < len) {
					item->cursorPos++;
				}
				return qtrue;
			}

			if ( key == K_LEFTARROW || key == K_KP_LEFTARROW )
			{
				if ( item->cursorPos > 0 ) {
					item->cursorPos--;
				}
				if (item->cursorPos < editPtr->paintOffset) {
					editPtr->paintOffset--;
				}
				return qtrue;
			}

			if ( key == K_HOME || key == K_KP_HOME) {// || ( tolower(key) == 'a' && trap_Key_IsDown( K_CTRL ) ) ) {
				item->cursorPos = 0;
				editPtr->paintOffset = 0;
				return qtrue;
			}

			if ( key == K_END || key == K_KP_END)  {// ( tolower(key) == 'e' && trap_Key_IsDown( K_CTRL ) ) ) {
				item->cursorPos = len;
				if(item->cursorPos > editPtr->maxPaintChars) {
					editPtr->paintOffset = len - editPtr->maxPaintChars;
				}
				return qtrue;
			}

			if ( key == K_INS || key == K_KP_INS ) {
				DC->setOverstrikeMode(!DC->getOverstrikeMode());
				return qtrue;
			}*/

			if(key == K_ENTER || key == K_KP_ENTER)
			{
				if(button->onFinish)
				{
					button->onFinish(button);
				}
				BG_PanelButtons_SetFocusButton(NULL);
				return qfalse;
			}
		}
	}

	return qtrue;
}

qboolean BG_PanelButtonsKeyEvent(int key, qboolean down, panel_button_t ** buttons)
{
	panel_button_t *button;

	if(BG_PanelButtons_GetFocusButton())
	{
		for(; *buttons; buttons++)
		{
			button = (*buttons);

			if(button == BG_PanelButtons_GetFocusButton())
			{
				if(button->onKeyDown && down)
				{
					if(!button->onKeyDown(button, key))
					{
						if(BG_PanelButtons_GetFocusButton())
						{
							return qfalse;
						}
					}
					else
					{
						return qtrue;
					}
				}
				if(button->onKeyUp && !down)
				{
					if(!button->onKeyUp(button, key))
					{
						if(BG_PanelButtons_GetFocusButton())
						{
							return qfalse;
						}
					}
					else
					{
						return qtrue;
					}
				}
			}
		}
	}

	if(down)
	{
		for(; *buttons; buttons++)
		{
			button = (*buttons);

			if(button->onKeyDown)
			{
				if(BG_CursorInRect(&button->rect))
				{
					if(button->onKeyDown(button, key))
					{
						return qtrue;
					}
				}
			}
		}
	}
	else
	{
		for(; *buttons; buttons++)
		{
			button = (*buttons);

			if(button->onKeyUp && BG_CursorInRect(&button->rect))
			{
				if(button->onKeyUp(button, key))
				{
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

void BG_PanelButtonsSetup(panel_button_t ** buttons)
{
	panel_button_t *button;

	for(; *buttons; buttons++)
	{
		button = (*buttons);

		if(button->shaderNormal)
		{
			button->hShaderNormal = trap_R_RegisterShaderNoMip(button->shaderNormal);
		}
	}
}

panel_button_t *BG_PanelButtonsGetHighlightButton(panel_button_t ** buttons)
{
	panel_button_t *button;

	for(; *buttons; buttons++)
	{
		button = (*buttons);

		if(button->onKeyDown && BG_CursorInRect(&button->rect))
		{
			return button;
		}
	}

	return NULL;
}

void BG_PanelButtonsRender(panel_button_t ** buttons)
{
	panel_button_t *button;

	for(; *buttons; buttons++)
	{
		button = (*buttons);

		if(button->onDraw)
		{
			button->onDraw(button);
		}
	}
}

void BG_PanelButtonsRender_TextExt(panel_button_t * button, const char *text)
{
	float           x = button->rect.x;
	float           w = button->rect.w;

	if(!button->font)
	{
		return;
	}

	if(button->font->align == ITEM_ALIGN_CENTER)
	{
		w = DC->textWidthExt(text, button->font->scalex, 0, button->font->font);

		x += ((button->rect.w - w) * 0.5f);
	}
	else if(button->font->align == ITEM_ALIGN_RIGHT)
	{
		x += button->rect.w - DC->textWidthExt(text, button->font->scalex, 0, button->font->font);
	}

	if(button->data[1])
	{
		vec4_t          clrBdr = { 0.5f, 0.5f, 0.5f, 1.f };
		vec4_t          clrBck = { 0.f, 0.f, 0.f, 0.8f };

		DC->fillRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, clrBck);
		DC->drawRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, 1, clrBdr);
	}
	DC->drawTextExt(x, button->rect.y + button->data[0], button->font->scalex, button->font->scaley, button->font->colour, text,
					0, 0, button->font->style, button->font->font);
}

void BG_PanelButtonsRender_Text(panel_button_t * button)
{
	BG_PanelButtonsRender_TextExt(button, button->text);
}

void BG_PanelButtonsRender_Img(panel_button_t * button)
{
	vec4_t          clr = { 1.f, 1.f, 1.f, 1.f };

	if(button->data[0])
	{
		clr[0] = button->data[1] / 255.f;
		clr[1] = button->data[2] / 255.f;
		clr[2] = button->data[3] / 255.f;
		clr[3] = button->data[4] / 255.f;

		trap_R_SetColor(clr);
	}

	if(button->data[5])
	{
		DC->drawRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, 1, clr);
	}
	else
	{
		DC->drawHandlePic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, button->hShaderNormal);
	}

	if(button->data[0])
	{
		trap_R_SetColor(NULL);
	}
}

panel_button_t *BG_PanelButtons_GetFocusButton(void)
{
	return bg_focusButton;
}

void BG_PanelButtons_SetFocusButton(panel_button_t * button)
{
	bg_focusButton = button;
}

void BG_FitTextToWidth_Ext(char *instr, float scale, float w, int size, fontInfo_t * font)
{
	char            buffer[1024];
	char           *s, *p, *c, *ls;
	int             l;

	Q_strncpyz(buffer, instr, 1024);
	memset(instr, 0, size);

	c = s = instr;
	p = buffer;
	ls = NULL;
	l = 0;
	while(*p)
	{
		*c = *p++;
		l++;

		if(*c == ' ')
		{
			ls = c;
		}						// store last space, to try not to break mid word

		c++;

		if(*p == '\n')
		{
			s = c + 1;
			l = 0;
		}
		else if(DC->textWidthExt(s, scale, 0, font) > w)
		{
			if(ls)
			{
				*ls = '\n';
				s = ls + 1;
			}
			else
			{
				*c = *(c - 1);
				*(c - 1) = '\n';
				s = c++;
			}

			ls = NULL;
			l = 0;
		}
	}

	if(c != buffer && (*(c - 1) != '\n'))
	{
		*c++ = '\n';
	}

	*c = '\0';
}
