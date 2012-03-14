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

// cg_window.c: cgame window handling
// ----------------------------------
// 24 Jul 02
// rhea@OrangeSmoothie.org
//
#include "cg_local.h"
#include "../ui/ui_shared.h"

extern pmove_t cg_pmove;                                        // cg_predict.c

vec4_t         colorClear = { 0.0f, 0.0f, 0.0f, 0.0f };         // Transparent
vec4_t         colorBrown1 = { 0.3f, 0.2f, 0.1f, 0.9f };        // Brown
vec4_t         colorGreen1 = { 0.21f, 0.3f, 0.0f, 0.9f };       // Greenish (darker)
vec4_t         colorGreen2 = { 0.305f, 0.475f, 0.305f, 0.48f }; // Slightly off from default fill

void CG_createStatsWindow( void )
{
	cg_window_t *sw = CG_windowAlloc( WFX_TEXTSIZING | WFX_FADEIN | /*WFX_SCROLLUP| */ WFX_TRUETYPE, 110 );

	cg.statsWindow = sw;

	if ( sw == NULL )
	{
		return;
	}

	// Window specific
	sw->id         = WID_STATS;
	sw->fontScaleX = cf_wstats.value * 0.2f;
	sw->fontScaleY = cf_wstats.value * 0.2f;
//  sw->x = (cg.snap->ps.pm_type == PM_INTERMISSION) ?  10 : 160;
//  sw->y = (cg.snap->ps.pm_type == PM_INTERMISSION) ? -20 : -7;    // Align from bottom minus offset and height
	sw->x          = ( cg.snap->ps.pm_type == PM_INTERMISSION ) ? 10 : 4;
	sw->y          = ( cg.snap->ps.pm_type == PM_INTERMISSION ) ? -20 : -100; // Align from bottom minus offset and height
}

void CG_createTopShotsWindow( void )
{
	cg_window_t *sw = CG_windowAlloc( WFX_TEXTSIZING | WFX_FLASH | WFX_FADEIN | WFX_SCROLLUP | WFX_TRUETYPE, 190 );

	cg.topshotsWindow = sw;

	if ( sw == NULL )
	{
		return;
	}

	// Window specific
	sw->id            = WID_TOPSHOTS;
	sw->fontScaleX    = cf_wtopshots.value * 0.2f;
	sw->fontScaleY    = cf_wtopshots.value * 0.2f;
	sw->x             = ( cg.snap->ps.pm_type == PM_INTERMISSION ) ? -10 : -20;
	sw->y             = ( cg.snap->ps.pm_type == PM_INTERMISSION ) ? -20 : -60; // Align from bottom minus offset and height
	sw->flashMidpoint = sw->flashPeriod * 0.8f;
	memcpy( &sw->colorBackground2, &colorGreen2, sizeof( vec4_t ) );
}

void CG_createMOTDWindow( void )
{
	const char *str = CG_ConfigString( CS_CUSTMOTD + 0 );

	if ( str != NULL && *str != 0 )
	{
		int         i;
		cg_window_t *sw = CG_windowAlloc( WFX_TEXTSIZING | WFX_FADEIN, 500 );

		cg.motdWindow = sw;

		if ( sw == NULL )
		{
			return;
		}

		// Window specific
		sw->id            = WID_MOTD;
		sw->fontScaleX    = 1.0f;
		sw->fontScaleY    = 1.0f;
		sw->x             = 10;
		sw->y             = -36;
		sw->flashMidpoint = sw->flashPeriod * 0.8f;
		memcpy( &sw->colorBackground2, &colorGreen2, sizeof( vec4_t ) );

		// Copy all MOTD info into the window
		cg.windowCurrent = sw;

		for ( i = 0; i < MAX_MOTDLINES; i++ )
		{
			str = CG_ConfigString( CS_CUSTMOTD + i );

			if ( str != NULL && *str != 0 )
			{
				CG_printWindow( ( char * )str );
			}
			else
			{
				break;
			}
		}
	}
}

/*
void CG_createWstatsMsgWindow(void)
{
        cg_window_t *sw = CG_windowAlloc(WFX_TEXTSIZING|WFX_FLASH, 0);

        cg.msgWstatsWindow = sw;
        if(sw == NULL) return;

        // Window specific
        sw->id = WID_NONE;
        sw->fontScaleX = 1.0f;
        sw->fontScaleY = 1.0f;
        sw->x = 300;
        sw->y = -1;
        sw->flashPeriod = 800;
        sw->flashMidpoint = sw->flashPeriod * 0.5f;

        memcpy(&sw->colorBorder, &colorClear, sizeof(vec4_t));
        memcpy(&sw->colorBackground, &colorBrown1, sizeof(vec4_t));
        memcpy(&sw->colorBackground2, &colorGreen1, sizeof(vec4_t));

        cg.windowCurrent = sw;
        CG_printWindow("^3Stats (+wstats)");
}


void CG_createWtopshotsMsgWindow(void)
{
        cg_window_t *sw = CG_windowAlloc(WFX_TEXTSIZING|WFX_FLASH, 0);

        cg.msgWtopshotsWindow = sw;
        if(sw == NULL) return;

        // Window specific
        sw->id = WID_NONE;
        sw->fontScaleX = 1.0f;
        sw->fontScaleY = 1.0f;
        sw->x = -20;
        sw->y = -1;
        sw->flashPeriod = 1000;
        sw->flashMidpoint = sw->flashPeriod * 0.7f;

        memcpy(&sw->colorBorder, &colorClear, sizeof(vec4_t));
        memcpy(&sw->colorBackground, &colorBrown1, sizeof(vec4_t));
        memcpy(&sw->colorBackground2, &colorGreen1, sizeof(vec4_t));

        cg.windowCurrent = sw;
        CG_printWindow("^3Rankings (+wtopshots)");
}
*/

/*
void CG_createDemoHelpWindow(void)
{
        int i;
        cg_window_t *sw = CG_windowAlloc(WFX_TEXTSIZING|WFX_FADEIN|WFX_TRUETYPE, 250);
        const char *help[] = {
                "^2TAB      ^Nscores",
                "^2F1-F5    ^Navidemo record",
                "^2F11-F12  ^Nscreenshot",
                " ", " ",
                "^2KP_DOWN  ^Nslow down (--)",
                "^2KP_LEFT  ^Nslow down (-)",
                "^2KP_UP    ^Nspeed up (++)",
                "^2KP_RIGHT ^Nspeed up (+)",
                "^2SPACE    ^Nnormal speed",
                " ", " ",
                "^2ENTER    ^NExternal view",
                "^2LFT/RGHT ^NChange angle",
                "^2UP/DOWN  ^NMove in/out"
        };
        const char *mvhelp[] = {
                " ", " ",
                "^2MOUSE1   ^NSelect/move view",
                "^2MOUSE2   ^NSwap w/main view",
                "^2MOUSE3   ^NToggle on/off",
                "^2SHIFT    ^NHold to resize",
                "^2KP_PGUP  ^NEnable a view",
                "^2KP_PGDN  ^NClose a view"
        };

        cg.demohelpWindow = sw;
        if(sw == NULL) return;

        // Window specific
        sw->id = WID_DEMOHELP;
        sw->fontScaleX = 0.2f;
        sw->fontScaleY = 0.3f;
        sw->x = 2;
        sw->y = 155;
        sw->flashMidpoint = sw->flashPeriod * 0.8f;
        memcpy(&sw->colorBackground2, &colorGreen2, sizeof(vec4_t));

        cg.windowCurrent = sw;
        for(i=0; i<sizeof(help)/sizeof(char *); i++) {
                CG_printWindow((char*)help[i]);
        }

        if(cg.mvTotalClients > 1) {
                for(i=0; i<sizeof(mvhelp)/sizeof(char *); i++) {
                        CG_printWindow((char*)mvhelp[i]);
                }
        }

        CG_printWindow(" ");
        CG_printWindow("^2BACKSPACE ^Nhelp on/off");
}
*/

/*
char *CG_getBindKeyName(const char *cmd, char *buf, int len)
{
        int j;

        for(j=0; j<256; j++) {
                trap_Key_GetBindingBuf(j, buf, len);
                if(*buf == 0) continue;

                if(!Q_stricmp(buf, cmd)) {
                        trap_Key_KeynumToStringBuf(j, buf, MAX_STRING_TOKENS);
                        Q_strupr(buf);
                        return(buf);
                }
        }

        Q_strncpyz(buf, va("(%s)", cmd), len);
        return(buf);
}

typedef struct
{
        char *cmd;
        char *info;
} helpType_t;

void CG_createSpecHelpWindow(void)
{
        int i, maxlen = 0;
        char *format, buf[MAX_STRING_TOKENS];
        cg_window_t *sw = CG_windowAlloc(WFX_TEXTSIZING|WFX_FADEIN|WFX_TRUETYPE, 250);
        const helpType_t help[] =
        {
                { "+zoom", "hold for pointer" },
                { "+attack", "window move/resize" },
                { "+sprint", "hold to resize" },
                { "weapnext", "window on/off" },
                { "weapprev", "swap w/main view"},
                { NULL, NULL },
                { NULL, NULL },
                { NULL, NULL },
                { NULL, NULL },
                { "weapalt", "swingcam toggle" },
                { "togglespechelp", "help on/off" }
        };

        cg.spechelpWindow = sw;
        if(sw == NULL) return;

        // Window specific
        sw->id = WID_SPECHELP;
        sw->fontScaleX = 0.2f;
        sw->fontScaleY = 0.3f;
        sw->x = 2;
        sw->y = 155;
        sw->flashMidpoint = sw->flashPeriod * 0.8f;
        memcpy(&sw->colorBackground2, &colorGreen2, sizeof(vec4_t));

        for(i=0; i<sizeof(help)/sizeof(helpType_t); i++) {
                if(help[i].cmd != NULL) {
                        int len = strlen(CG_getBindKeyName(help[i].cmd, buf, sizeof(buf)));
                        if(len > maxlen) {
                                maxlen = len;
                        }
                }
        }

        cg.windowCurrent = sw;
        format = va("^2%%%ds ^N%%s", maxlen);

        for(i=0; i<sizeof(help)/sizeof(helpType_t); i++) {
                if(help[i].cmd == NULL) {
                        CG_printWindow(" ");
                } else {
                        CG_printWindow(va(format, CG_getBindKeyName(help[i].cmd, buf, sizeof(buf)), help[i].info));
                }
        }
}
*/

//////////////////////////////////////////////
//////////////////////////////////////////////
//
//      WINDOW HANDLING AND PRIMITIVES
//
//////////////////////////////////////////////
//////////////////////////////////////////////

// Windowing system setup
void CG_windowInit( void )
{
	int i;

	cg.winHandler.numActiveWindows = 0;

	for ( i = 0; i < MAX_WINDOW_COUNT; i++ )
	{
		cg.winHandler.window[ i ].inuse = qfalse;
	}

	cg.msgWstatsWindow    = NULL;
	cg.msgWtopshotsWindow = NULL;
	cg.statsWindow        = NULL;
	cg.topshotsWindow     = NULL;
}

// Window stuct "constructor" with some common defaults
void CG_windowReset( cg_window_t *w, int fx, int startupLength )
{
	vec4_t colorGeneralBorder = { 0.5f, 0.35f, 0.25f, 0.5f };
	vec4_t colorGeneralFill   = { 0.3f, 0.45f, 0.3f, 0.5f };

	w->effects       = fx;
	w->fontScaleX    = 0.25;
	w->fontScaleY    = 0.25;
	w->flashPeriod   = 1000;
	w->flashMidpoint = w->flashPeriod / 2;
	w->id            = WID_NONE;
	w->inuse         = qtrue;
	w->lineCount     = 0;
	w->state         = ( fx >= WFX_FADEIN ) ? WSTATE_START : WSTATE_COMPLETE;
	w->targetTime    = ( startupLength > 0 ) ? startupLength : 0;
	w->time          = trap_Milliseconds();
	w->x             = 0;
	w->y             = 0;

	memcpy( &w->colorBorder, &colorGeneralBorder, sizeof( vec4_t ) );
	memcpy( &w->colorBackground, &colorGeneralFill, sizeof( vec4_t ) );
}

// Reserve a window
cg_window_t    *CG_windowAlloc( int fx, int startupLength )
{
	int                i;
	cg_window_t        *w;
	cg_windowHandler_t *wh = &cg.winHandler;

	if ( wh->numActiveWindows == MAX_WINDOW_COUNT )
	{
		return ( NULL );
	}

	for ( i = 0; i < MAX_WINDOW_COUNT; i++ )
	{
		w = &wh->window[ i ];

		if ( w->inuse == qfalse )
		{
			CG_windowReset( w, fx, startupLength );
			wh->activeWindows[ wh->numActiveWindows++ ] = i;
			return ( w );
		}
	}

	// Fail if we're a full airplane
	return ( NULL );
}

// Free up a window reservation
void CG_windowFree( cg_window_t *w )
{
	int                i, j;
	cg_windowHandler_t *wh = &cg.winHandler;

	if ( w == NULL )
	{
		return;
	}

	if ( w->effects >= WFX_FADEIN && w->state != WSTATE_OFF && w->inuse == qtrue )
	{
		w->state = WSTATE_SHUTDOWN;
		w->time  = trap_Milliseconds();
		return;
	}

	for ( i = 0; i < wh->numActiveWindows; i++ )
	{
		if ( w == &wh->window[ wh->activeWindows[ i ] ] )
		{
			for ( j = i; j < wh->numActiveWindows; j++ )
			{
				if ( j + 1 < wh->numActiveWindows )
				{
					wh->activeWindows[ j ] = wh->activeWindows[ j + 1 ];
				}
			}

			w->id    = WID_NONE;
			w->inuse = qfalse;
			w->state = WSTATE_OFF;

			CG_removeStrings( w );

			wh->numActiveWindows--;

			break;
		}
	}
}

void CG_windowCleanup( void )
{
	int                i;
	cg_window_t        *w;
	cg_windowHandler_t *wh = &cg.winHandler;

	for ( i = 0; i < wh->numActiveWindows; i++ )
	{
		w = &wh->window[ wh->activeWindows[ i ] ];

		if ( !w->inuse || w->state == WSTATE_OFF )
		{
			CG_windowFree( w );
			i--;
		}
	}
}

void CG_demoAviFPSDraw( void )
{
	qboolean fKeyDown =
	  cgs.fKeyPressed[ K_F1 ] | cgs.fKeyPressed[ K_F2 ] | cgs.fKeyPressed[ K_F3 ] | cgs.fKeyPressed[ K_F4 ] | cgs.fKeyPressed[ K_F5 ];

	if ( cg.demoPlayback && fKeyDown && cgs.aviDemoRate >= 0 )
	{
		CG_DrawStringExt( 42, 425,
		                  ( ( cgs.aviDemoRate > 0 ) ? va( "^3Record AVI @ ^7%d^2fps", cgs.aviDemoRate ) : "^1Stop AVI Recording" ),
		                  colorWhite, qfalse, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT - 2, 0 );
	}
}

void CG_demoTimescaleDraw( void )
{
	if ( cg.demoPlayback && cgs.timescaleUpdate > cg.time && demo_drawTimeScale.integer != 0 )
	{
		char *s = va( "^3TimeScale: ^7%.1f", cg_timescale.value );
		int  w  = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;

		CG_FillRect( 42 - 2, 400, w + 5, SMALLCHAR_HEIGHT + 3, colorDkGreen );
		CG_DrawRect( 42 - 2, 400, w + 5, SMALLCHAR_HEIGHT + 3, 1, colorMdYellow );

		CG_DrawStringExt( 42, 400, s, colorWhite, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
	}
}

// Main window-drawing handler
void CG_windowDraw( void )
{
	int         h, x, y, i, j, milli, t_offset, tmp;
	cg_window_t *w;
	qboolean    fCleanup = qfalse;

	// Gordon: FIXME, the limbomenu var no longer exists
	qboolean    fAllowMV = ( cg.snap != NULL && cg.snap->ps.pm_type != PM_INTERMISSION /*&& !cg.limboMenu */ );
	vec4_t      *bg;
	vec4_t      textColor, borderColor, bgColor;

	if ( cg.winHandler.numActiveWindows == 0 )
	{
		// Draw these for demoplayback no matter what
		CG_demoAviFPSDraw();
		CG_demoTimescaleDraw();
		return;
	}

	milli = trap_Milliseconds();
	memcpy( textColor, colorWhite, sizeof( vec4_t ) );

	// Mouse cursor position for MV highlighting (offset for cursor pointer position)
	// Also allow for swingcam toggling
	if ( cg.mvTotalClients > 0 && fAllowMV )
	{
		CG_cursorUpdate();
	}

	for ( i = 0; i < cg.winHandler.numActiveWindows; i++ )
	{
		w = &cg.winHandler.window[ cg.winHandler.activeWindows[ i ] ];

		if ( !w->inuse || w->state == WSTATE_OFF )
		{
			fCleanup = qtrue;
			continue;
		}

		// Multiview rendering has its own handling

		if ( w->effects & WFX_MULTIVIEW )
		{
			if ( w != cg.mvCurrentMainview && fAllowMV )
			{
				CG_mvDraw( w );
			}

			continue;
		}

		if ( w->effects & WFX_TEXTSIZING )
		{
			CG_windowNormalizeOnText( w );
			w->effects &= ~WFX_TEXTSIZING;
		}

		bg             = ( ( w->effects & WFX_FLASH ) &&
		                   ( milli % w->flashPeriod ) > w->flashMidpoint ) ? &w->colorBackground2 : &w->colorBackground;

		h              = w->h;
		x              = w->x;
		y              = w->y;
		t_offset       = milli - w->time;
		textColor[ 3 ] = 1.0f;
		memcpy( &borderColor, w->colorBorder, sizeof( vec4_t ) );
		memcpy( &bgColor, bg, sizeof( vec4_t ) );

		// TODO: Add in support for ALL scrolling effects
		if ( w->state == WSTATE_START )
		{
			tmp = w->targetTime - t_offset;

			if ( w->effects & WFX_SCROLLUP )
			{
				if ( tmp > 0 )
				{
					y += ( 480 - y ) * tmp / w->targetTime;   //(100 * tmp / w->targetTime) / 100;
				}
				else
				{
					w->state = WSTATE_COMPLETE;
				}

				w->curY = y;
			}

			if ( w->effects & WFX_FADEIN )
			{
				if ( tmp > 0 )
				{
					textColor[ 3 ] = ( float )( ( float )t_offset / ( float )w->targetTime );
				}
				else
				{
					w->state = WSTATE_COMPLETE;
				}
			}
		}
		else if ( w->state == WSTATE_SHUTDOWN )
		{
			tmp = w->targetTime - t_offset;

			if ( w->effects & WFX_SCROLLUP )
			{
				if ( tmp > 0 )
				{
					y = w->curY + ( 480 - w->y ) * t_offset / w->targetTime;  //(100 * t_offset / w->targetTime) / 100;
				}

				if ( tmp < 0 || y >= 480 )
				{
					w->state = WSTATE_OFF;
					fCleanup = qtrue;
					continue;
				}
			}

			if ( w->effects & WFX_FADEIN )
			{
				if ( tmp > 0 )
				{
					textColor[ 3 ] -= ( float )( ( float )t_offset / ( float )w->targetTime );
				}
				else
				{
					textColor[ 3 ] = 0.0f;
					w->state       = WSTATE_OFF;
				}
			}
		}

		borderColor[ 3 ] *= textColor[ 3 ];
		bgColor[ 3 ]     *= textColor[ 3 ];

		CG_FillRect( x, y, w->w, h, bgColor );
		CG_DrawRect( x, y, w->w, h, 1, borderColor );

		x += 5;
		y -= ( w->effects & WFX_TRUETYPE ) ? 3 : 0;

		for ( j = w->lineCount - 1; j >= 0; j-- )
		{
			if ( w->effects & WFX_TRUETYPE )
			{
//              CG_Text_Paint(x, y + h, w->fontScale, textColor, (char*)w->lineText[j], 0.0f, 0, 0);
				CG_Text_Paint_Ext( x, y + h, w->fontScaleX, w->fontScaleY, textColor,
				                   ( char * )w->lineText[ j ], 0.0f, 0, 0, &cgs.media.limboFont2 );
			}

			h -= ( w->lineHeight[ j ] + 3 );

			if ( !( w->effects & WFX_TRUETYPE ) )
			{
				CG_DrawStringExt2( x, y + h, ( char * )w->lineText[ j ], textColor, qfalse, qtrue, w->fontWidth, w->fontHeight, 0 );
			}
		}
	}

	// Wedge in MV info overlay
	if ( cg.mvTotalClients > 0 && fAllowMV )
	{
		CG_mvOverlayDisplay();
	}

	// Extra rate info
	CG_demoAviFPSDraw();
	CG_demoTimescaleDraw();

	// Mouse cursor lays on top of everything
	if ( cg.mvTotalClients > 0 && cg.time < cgs.cursorUpdate && fAllowMV )
	{
		//CG_DrawPic(cgs.cursorX - CURSOR_OFFSETX, cgs.cursorY - CURSOR_OFFSETY, 32, 32, cgs.media.cursor);
		CG_DrawPic( cgDC.cursorx, cgDC.cursory, 32, 32, cgs.media.cursorIcon );
	}

	if ( fCleanup )
	{
		CG_windowCleanup();
	}
}

// Set the window width and height based on the windows text/font parameters
void CG_windowNormalizeOnText( cg_window_t *w )
{
	int i, tmp;

	if ( w == NULL )
	{
		return;
	}

	w->w = 0;
	w->h = 0;

	if ( !( w->effects & WFX_TRUETYPE ) )
	{
		w->fontWidth  = w->fontScaleX * WINDOW_FONTWIDTH;
		w->fontHeight = w->fontScaleY * WINDOW_FONTHEIGHT;
	}

	for ( i = 0; i < w->lineCount; i++ )
	{
		if ( w->effects & WFX_TRUETYPE )
		{
			tmp = CG_Text_Width_Ext( ( char * )w->lineText[ i ], w->fontScaleX, 0, &cgs.media.limboFont2 );
		}
		else
		{
			tmp = CG_DrawStrlen( ( char * )w->lineText[ i ] ) * w->fontWidth;
		}

		if ( tmp > w->w )
		{
			w->w = tmp;
		}
	}

	for ( i = 0; i < w->lineCount; i++ )
	{
		if ( w->effects & WFX_TRUETYPE )
		{
			w->lineHeight[ i ] = CG_Text_Height_Ext( ( char * )w->lineText[ i ], w->fontScaleY, 0, &cgs.media.limboFont2 );
		}
		else
		{
			w->lineHeight[ i ] = w->fontHeight;
		}

		w->h += w->lineHeight[ i ] + 3;
	}

	// Border + margins
	w->w += 10;
	w->h += 3;

	// Set up bottom alignment
	if ( w->x < 0 )
	{
		w->x += 640 - w->w;
	}

	if ( w->y < 0 )
	{
		w->y += 480 - w->h;
	}
}

void CG_printWindow( char *str )
{
	int         pos = 0, pos2 = 0;
	char        buf[ MAX_STRING_CHARS ];
	cg_window_t *w  = cg.windowCurrent;

	if ( w == NULL )
	{
		return;
	}

	// Silly logic for a strict format
	Q_strncpyz( buf, str, MAX_STRING_CHARS );

	while ( buf[ pos ] > 0 && w->lineCount < MAX_WINDOW_LINES )
	{
		if ( buf[ pos ] == '\n' )
		{
			if ( pos2 == pos )
			{
				if ( !CG_addString( w, " " ) )
				{
					return;
				}
			}
			else
			{
				buf[ pos ] = 0;

				if ( !CG_addString( w, buf + pos2 ) )
				{
					return;
				}
			}

			pos2 = ++pos;
			continue;
		}

		pos++;
	}

	if ( pos2 < pos )
	{
		CG_addString( w, buf + pos2 );
	}
}

//
// String buffer handling
//
void CG_initStrings( void )
{
	int i;

	for ( i = 0; i < MAX_STRINGS; i++ )
	{
		cg.aStringPool[ i ].fActive  = qfalse;
		cg.aStringPool[ i ].str[ 0 ] = 0;
	}
}

qboolean CG_addString( cg_window_t *w, char *buf )
{
	int i;

	// Check if we're reusing the current buf
	if ( w->lineText[ w->lineCount ] != NULL )
	{
		for ( i = 0; i < MAX_STRINGS; i++ )
		{
			if ( !cg.aStringPool[ i ].fActive )
			{
				continue;
			}

			if ( w->lineText[ w->lineCount ] == ( char * )&cg.aStringPool[ i ].str )
			{
				w->lineCount++;
				cg.aStringPool[ i ].fActive = qtrue;
				strcpy( cg.aStringPool[ i ].str, buf );

				return ( qtrue );
			}
		}
	}

	for ( i = 0; i < MAX_STRINGS; i++ )
	{
		if ( !cg.aStringPool[ i ].fActive )
		{
			cg.aStringPool[ i ].fActive   = qtrue;
			strcpy( cg.aStringPool[ i ].str, buf );
			w->lineText[ w->lineCount++ ] = ( char * )&cg.aStringPool[ i ].str;

			return ( qtrue );
		}
	}

	return ( qfalse );
}

void CG_removeStrings( cg_window_t *w )
{
	int i, j;

	for ( i = 0; i < w->lineCount; i++ )
	{
		char *ref = w->lineText[ i ];

		for ( j = 0; j < MAX_STRINGS; j++ )
		{
			if ( !cg.aStringPool[ j ].fActive )
			{
				continue;
			}

			if ( ref == ( char * )&cg.aStringPool[ j ].str )
			{
				w->lineText[ i ]             = NULL;
				cg.aStringPool[ j ].fActive  = qfalse;
				cg.aStringPool[ j ].str[ 0 ] = 0;

				break;
			}
		}
	}
}

//
// cgame cursor handling
//

// Mouse overlay for controlling multiview windows
void CG_cursorUpdate( void )
{
	int                i, j, x;
	float              nx, ny;
	int                nSelectedWindow = -1;
	cg_window_t        *w;
	cg_windowHandler_t *wh             = &cg.winHandler;
	qboolean           fFound          = qfalse, fUpdateOverlay = qfalse;
	qboolean           fSelect, fResize;

	// Get cursor current position (when connected to a server)
	nx                 = cgs.cursorX;
	ny                 = cgs.cursorY;
	fSelect            = cgs.fSelect;
	fResize            = cgs.fResize;

	// For mm4
	cg.mvCurrentActive = cg.mvCurrentMainview;

	// For overlay highlights
	for ( i = 0; i < cg.mvTotalClients; i++ )
	{
		cg.mvOverlay[ i ].fActive = qfalse;
	}

	for ( i = wh->numActiveWindows - 1; i >= 0; i-- )
	{
		w = &wh->window[ wh->activeWindows[ i ] ];

		if ( ( w->effects & WFX_MULTIVIEW ) && w != cg.mvCurrentMainview )
		{
			// Mouse/window detection
			// If the current window is selected, and the button is down, then allow the update
			// to occur, as quick mouse movements can move it past the window borders
			if ( !fFound &&
			     ( ( ( w->mvInfo & MV_SELECTED ) && fSelect ) ||
			       ( !fSelect && nx >= w->x && nx < w->x + w->w && ny >= w->y && ny < w->y + w->h ) ) )
			{
				if ( !( w->mvInfo & MV_SELECTED ) )
				{
					w->mvInfo      |= MV_SELECTED;
					nSelectedWindow = i;
				}

				// If not dragging/resizing, prime for later update
				if ( !fSelect )
				{
					w->m_x = -1.0f;
					w->m_y = -1.0f;
				}
				else
				{
					if ( w->m_x > 0 && w->m_y > 0 )
					{
						if ( fResize )
						{
							w->w += nx - w->m_x;

							if ( w->x + w->w > 640 - 2 )
							{
								w->w = 640 - 2 - w->x;
							}

							if ( w->w < 64 )
							{
								w->w = 64;
							}

							w->h += ny - w->m_y;

							if ( w->y + w->h > 480 - 2 )
							{
								w->h = 480 - 2 - w->y;
							}

							if ( w->h < 48 )
							{
								w->h = 48;
							}
						}
						else
						{
							w->x += nx - w->m_x;

							if ( w->x + w->w > 640 - 2 )
							{
								w->x = 640 - 2 - w->w;
							}

							if ( w->x < 2 )
							{
								w->x = 2;
							}

							w->y += ny - w->m_y;

							if ( w->y + w->h > 480 - 2 )
							{
								w->y = 480 - 2 - w->h;
							}

							if ( w->y < 2 )
							{
								w->y = 2;
							}
						}
					}

					w->m_x = nx;
					w->m_y = ny;
				}

				fFound             = qtrue;
				cg.mvCurrentActive = w;

				// Reset mouse info for window if it loses focuse
			}
			else if ( w->mvInfo & MV_SELECTED )
			{
				fUpdateOverlay = qtrue;
				w->m_x         = -1.0f;
				w->m_y         = -1.0f;
				w->mvInfo     &= ~MV_SELECTED;

				if ( fFound )
				{
					break;          // Small optimization: we've found a new window, and cleared the old focus
				}
			}
		}
	}

	nx = ( float )( MVINFO_RIGHT - ( MVINFO_TEXTSIZE * 3 ) );
	ny = ( float )( MVINFO_TOP + ( MVINFO_TEXTSIZE + 1 ) );

	// Highlight corresponding active window's overlay element
	if ( fFound )
	{
		for ( i = 0; i < cg.mvTotalClients; i++ )
		{
			if ( cg.mvOverlay[ i ].pID == ( cg.mvCurrentActive->mvInfo & MV_PID ) )
			{
				cg.mvOverlay[ i ].fActive = qtrue;
				break;
			}
		}
	}
	// Check MV overlay detection here for better perf with more text elements
	// General boundary check
	else
	{
		// Ugh, have to loop through BOTH team lists
		int vOffset = 0;

		for ( i = TEAM_AXIS; i <= TEAM_ALLIES; i++ )
		{
			if ( cg.mvTotalTeam[ i ] == 0 )
			{
				continue;
			}

			if ( cgs.cursorX >= nx && cgs.cursorY >= ny && cgs.cursorX < MVINFO_RIGHT &&
			     cgs.cursorY < ny + ( cg.mvTotalTeam[ i ] * ( MVINFO_TEXTSIZE + 1 ) ) )
			{
				int pos = ( int )( cgs.cursorY - ny ) / ( MVINFO_TEXTSIZE + 1 );

				if ( pos < cg.mvTotalTeam[ i ] )
				{
					int x = MVINFO_RIGHT - cg.mvOverlay[ ( cg.mvTeamList[ i ][ pos ] ) ].width;
					int y = MVINFO_TOP + vOffset + ( ( pos + 1 ) * ( MVINFO_TEXTSIZE + 1 ) );

					// See if we're really over something
					if ( cgs.cursorX >= x && cgs.cursorY >= y && cgs.cursorX <= MVINFO_RIGHT && cgs.cursorY <= y + MVINFO_TEXTSIZE )
					{
						// Perform any other window handling here for MV
						// views based on element selection
						cg.mvOverlay[ ( cg.mvTeamList[ i ][ pos ] ) ].fActive = qtrue;

						w                                                     = CG_mvClientLocate( cg.mvOverlay[ ( cg.mvTeamList[ i ][ pos ] ) ].pID );

						if ( w != NULL )
						{
							cg.mvCurrentActive = w;
						}

						if ( fSelect )
						{
							if ( w != NULL )
							{
								// Swap window-view with mainview
								if ( w != cg.mvCurrentMainview )
								{
									CG_mvMainviewSwap( w );
								}
							}
							else
							{
								// Swap non-view with mainview
								cg.mvCurrentMainview->mvInfo = ( cg.mvCurrentMainview->mvInfo & ~MV_PID ) |
								                               ( cg.mvOverlay[ cg.mvTeamList[ i ][ pos ] ].pID & MV_PID );
								fUpdateOverlay               = qtrue;
							}
						}
					}
				}
			}

			vOffset += ( cg.mvTotalTeam[ i ] + 2 ) * ( MVINFO_TEXTSIZE + 1 );
			ny      += vOffset;
		}
	}

	// If we have a new highlight, reorder so our highlight is always
	// drawn last (on top of all other windows)
	if ( nSelectedWindow >= 0 )
	{
		fUpdateOverlay = qtrue;
		x              = wh->activeWindows[ nSelectedWindow ];

		for ( j = nSelectedWindow; j < wh->numActiveWindows - 1; j++ )
		{
			wh->activeWindows[ j ] = wh->activeWindows[ j + 1 ];
		}

		wh->activeWindows[ wh->numActiveWindows - 1 ] = x;
	}

	// Finally, sync the overlay, if needed
	if ( fUpdateOverlay )
	{
		CG_mvOverlayUpdate();
	}
}
