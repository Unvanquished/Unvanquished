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

// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"

qboolean scr_initialized; // ready to draw

cvar_t   *cl_timegraph;
cvar_t   *cl_debuggraph;
cvar_t   *cl_graphheight;
cvar_t   *cl_graphscale;
cvar_t   *cl_graphshift;

/*
================
SCR_DrawNamedPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawNamedPic( float x, float y, float width, float height, const char *picname )
{
	qhandle_t hShader;

	assert( width != 0 );

	hShader = re.RegisterShader( picname );
	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

/*
================
SCR_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void SCR_AdjustFrom640( float *x, float *y, float *w, float *h )
{
	float xscale;
	float yscale;

#if 0

	// adjust for wide screens
	if ( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 )
1
	{
		*x += 0.5 * ( cls.glconfig.vidWidth - ( cls.glconfig.vidHeight * 640 / 480 ) );
	}

#endif

	// scale for screen sizes
	xscale = cls.glconfig.vidWidth / 640.0;
	yscale = cls.glconfig.vidHeight / 480.0;

	if ( x )
	{
		*x *= xscale;
	}

	if ( y )
	{
		*y *= yscale;
	}

	if ( w )
	{
		*w *= xscale;
	}

	if ( h )
	{
		*h *= yscale;
	}
}

/*
================
SCR_FillRect

Coordinates are 640*480 virtual values
=================
*/
void SCR_FillRect( float x, float y, float width, float height, const float *color )
{
	re.SetColor( color );

	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cls.whiteShader );

	re.SetColor( NULL );
}

/*
================
SCR_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawPic( float x, float y, float width, float height, qhandle_t hShader )
{
	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

static glyphInfo_t *Glyph( int ch )
{
	static glyphInfo_t glyphs[8];
	static int index = 0;
	glyphInfo_t *glyph = &glyphs[ index++ & 7 ];

	re.GlyphChar( &cls.consoleFont, ch, glyph );

	return glyph;
}

/*
** SCR_DrawUnichar
** chars are drawn at 640*480 virtual screen size
*/
static void SCR_DrawUnichar( int x, int y, float size, int ch )
{
	float ax, ay, aw, ah;

	if ( ch == ' ' )
	{
		return;
	}

	if ( y < -size )
	{
		return;
	}

	ax = x;
	ay = y;
	aw = size;
	ah = size;
	SCR_AdjustFrom640( &ax, &ay, &aw, &ah );

	if( cls.useLegacyConsoleFace )
	{
		int row, col;
		float frow, fcol;

		if ( ch >= 0x100 ) { ch = 0; }

		row = ch >> 4;
		col = ch & 15;

		frow = row * 0.0625;
		fcol = col * 0.0625;
		size = 0.0625;

		re.DrawStretchPic( ax, ay, aw, ah,
				fcol, frow,
				fcol + size, frow + size,
				cls.charSetShader );
	}
	else
	{
	glyphInfo_t *glyph = Glyph( ch );

	re.DrawStretchPic( ax, ay, aw, glyph->imageHeight,
		glyph->s, glyph->t,
		glyph->s2, glyph->t2,
		glyph->glyph );
	}
}

void SCR_DrawConsoleFontUnichar( float x, float y, int ch )
{
	if ( cls.useLegacyConsoleFont )
	{
		SCR_DrawSmallUnichar( ( int ) x, ( int ) y, ch );
		return;
	}

	if ( ch != ' ' )
	{
		glyphInfo_t *glyph = Glyph( ch );
		float       yadj = glyph->top;
		float       xadj = ( SCR_ConsoleFontUnicharWidth( ch ) - glyph->xSkip ) / 2.0;

		re.DrawStretchPic( x + xadj, y - yadj, glyph->imageWidth, glyph->imageHeight,
		                   glyph->s, glyph->t,
		                   glyph->s2, glyph->t2,
		                   glyph->glyph );
	}
}

void SCR_DrawConsoleFontChar( float x, float y, const char *s )
{
	SCR_DrawConsoleFontUnichar( x, y, Q_UTF8CodePoint( s ) );
}

/*
** SCR_DrawSmallUnichar
** small chars are drawn at native screen resolution
*/
void SCR_DrawSmallUnichar( int x, int y, int ch )
{
	int   row, col;
	float frow, fcol;
	float size;

	if ( ch < 0x100 || cls.useLegacyConsoleFont )
	{
		if ( ch == ' ' ) {
			return;
		}

		if ( y < -SMALLCHAR_HEIGHT ) {
			return;
		}

		if ( ch >= 0x100 ) { ch = 0; }

		row = ch>>4;
		col = ch&15;

		frow = row*0.0625;
		fcol = col*0.0625;
		size = 0.0625;

		// adjust for baseline
		re.DrawStretchPic( x, y - (int)( SMALLCHAR_HEIGHT / ( CONSOLE_FONT_VPADDING + 1 ) ),
		                SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT,
				fcol, frow,
				fcol + size, frow + size,
				cls.charSetShader );
	} else {
		glyphInfo_t *glyph = Glyph( ch );

		re.DrawStretchPic( x, y, SMALLCHAR_WIDTH, glyph->imageHeight,
				glyph->s,
				glyph->t,
				glyph->s2,
				glyph->t2,
				glyph->glyph );
	}
}

/*
==================
SCR_DrawBigString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/

void SCR_DrawStringExt( int x, int y, float size, const char *string, float *setColor, qboolean forceColor, qboolean noColorEscape )
{
	vec4_t     color;
	const char *s;
	int        xx;
	qboolean   noColour = qfalse;

	// draw the drop shadow
	color[ 0 ] = color[ 1 ] = color[ 2 ] = 0;
	color[ 3 ] = setColor[ 3 ];
	re.SetColor( color );
	s = string;
	xx = x;

	while ( *s )
	{
		int ch;

		if ( !noColorEscape && Q_IsColorString( s ) )
		{
			s += 2;
			continue;
		}

		if ( !noColorEscape && *s == Q_COLOR_ESCAPE && s[1] == Q_COLOR_ESCAPE )
		{
			++s;
		}

		ch = Q_UTF8CodePoint( s );
		SCR_DrawUnichar( xx + 2, y + 2, size, ch );
		xx += size;
		s += Q_UTF8WidthCP( ch );
	}

	// draw the colored text
	s = string;
	xx = x;
	re.SetColor( setColor );

	while ( *s )
	{
		int ch;

		if ( !noColour && Q_IsColorString( s ) )
		{
			if ( !forceColor )
			{
				if ( * ( s + 1 ) == COLOR_NULL )
				{
					memcpy( color, setColor, sizeof( color ) );
				}
				else
				{
					memcpy( color, g_color_table[ ColorIndex( * ( s + 1 ) ) ], sizeof( color ) );
				}

				color[ 3 ] = setColor[ 3 ];
				re.SetColor( color );
			}

			if ( !noColorEscape )
			{
				s += 2;
				continue;
			}
		}
		else if ( !noColour && *s == Q_COLOR_ESCAPE && s[1] == Q_COLOR_ESCAPE )
		{
			if ( !noColorEscape )
			{
				++s;
			}
			else
			{
				noColour = qtrue;
			}
		}
		else
		{
			noColour = qfalse;
		}

		ch = Q_UTF8CodePoint( s );
		SCR_DrawUnichar( xx, y, size, ch );
		xx += size;
		s += Q_UTF8WidthCP( ch );
	}

	re.SetColor( NULL );
}

void SCR_DrawBigString( int x, int y, const char *s, float alpha, qboolean noColorEscape )
{
	float color[ 4 ];

	color[ 0 ] = color[ 1 ] = color[ 2 ] = 1.0;
	color[ 3 ] = alpha;
	SCR_DrawStringExt( x, y, BIGCHAR_WIDTH, s, color, qfalse, noColorEscape );
}

void SCR_DrawBigStringColor( int x, int y, const char *s, vec4_t color, qboolean noColorEscape )
{
	SCR_DrawStringExt( x, y, BIGCHAR_WIDTH, s, color, qtrue, noColorEscape );
}

/*
==================
SCR_DrawSmallString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawSmallStringExt( int x, int y, const char *string, float *setColor, qboolean forceColor, qboolean noColorEscape )
{
	vec4_t     color;
	const char *s;
	float      xx;
	qboolean   noColour = qfalse;

	// draw the colored text
	s = string;
	xx = x;
	re.SetColor( setColor );

	while ( *s )
	{
		int ch;

		if ( !noColour && Q_IsColorString( s ) )
		{
			if ( !forceColor )
			{
				if ( * ( s + 1 ) == COLOR_NULL )
				{
					memcpy( color, setColor, sizeof( color ) );
				}
				else
				{
					memcpy( color, g_color_table[ ColorIndex( * ( s + 1 ) ) ], sizeof( color ) );
				}

				color[ 3 ] = setColor[ 3 ];
				re.SetColor( color );
			}

			if ( !noColorEscape )
			{
				s += 2;
				continue;
			}
		}
		else if ( !noColour && *s == Q_COLOR_ESCAPE && s[1] == Q_COLOR_ESCAPE )
		{
			if ( !noColorEscape )
			{
				++s;
			}
			else
			{
				noColour = qtrue;
			}
		}
		else
		{
			noColour = qfalse;
		}

		ch = Q_UTF8CodePoint( s );
		SCR_DrawConsoleFontUnichar( xx, y, ch );
		xx += SCR_ConsoleFontUnicharWidth( ch );
		s += Q_UTF8WidthCP( ch );
	}

	re.SetColor( NULL );
}

/*
** SCR_Strlen -- skips color escape codes
*/
static int SCR_Strlen( const char *str )
{
	const char *s = str;
	int        count = 0;

	while ( *s )
	{
		if ( Q_IsColorString( s ) )
		{
			s += 2;
		}
		else if ( *s == Q_COLOR_ESCAPE && s[1] == Q_COLOR_ESCAPE )
		{
			++s;
		}

		{
			count++;
			s += Q_UTF8Width( s );
		}
	}

	return count;
}

/*
** SCR_GetBigStringWidth
*/
int SCR_GetBigStringWidth( const char *str )
{
	return SCR_Strlen( str ) * BIGCHAR_WIDTH;
}

//===============================================================================

/*
=================
SCR_DrawDemoRecording
=================
*/
void SCR_DrawDemoRecording( void )
{
	if ( !clc.demorecording )
	{
		return;
	}

	//bani
	Cvar_Set( "cl_demooffset", va( "%d", FS_FTell( clc.demofile ) ) );
}

#ifdef USE_VOIP

/*
=================
SCR_DrawVoipMeter
=================
*/
void SCR_DrawVoipMeter( void )
{
	char buffer[ 16 ];
	char string[ 256 ];
	int  limit, i;

	if ( !cl_voipShowMeter->integer )
	{
		return; // player doesn't want to show meter at all.
	}
	else if ( !cl_voipSend->integer )
	{
		return; // not recording at the moment.
	}
	else if ( cls.state != CA_ACTIVE )
	{
		return; // not connected to a server.
	}
	else if ( !clc.voipEnabled )
	{
		return; // server doesn't support VoIP.
	}
	else if ( clc.demoplaying )
	{
		return; // playing back a demo.
	}
	else if ( !cl_voip->integer )
	{
		return; // client has VoIP support disabled.
	}

	limit = ( int )( clc.voipPower * 10.0f );

	if ( limit > 10 )
	{
		limit = 10;
	}

	for ( i = 0; i < limit; i++ )
	{
		buffer[ i ] = '*';
	}

	while ( i < 10 )
	{
		buffer[ i++ ] = ' ';
	}

	buffer[ i ] = '\0';

	sprintf( string, "VoIP: [%s]", buffer );
	SCR_DrawStringExt( 320 - strlen( string ) * 4, 10, 8, string, g_color_table[ 7 ], qtrue, qfalse );
}

/*
=================
SCR_DrawVoipSender
=================
*/
void SCR_DrawVoipSender( void )
{
	char string[ 256 ];
	char teamColor;

	// Little bit of a hack here, but it's the only thing i could come up with
	if ( cls.voipTime > cls.realtime )
	{
		if ( !cl_voipShowSender->integer )
		{
			return; // They don't want this on :(
		}
		else if ( cls.state != CA_ACTIVE )
		{
			return; // not connected to a server.
		}
		else if ( !clc.voipEnabled )
		{
			return; // server doesn't support VoIP.
		}
		else if ( clc.demoplaying )
		{
			return; // playing back a demo.
		}
		else if ( !cl_voip->integer )
		{
			return; // client has VoIP support disabled.
		}

		switch ( atoi( Info_ValueForKey(cl.gameState.stringData +
			cl.gameState.stringOffsets[CS_PLAYERS + cls.voipSender], "t") ) )
		{
			case TEAM_ALIENS: teamColor = '1'; break;
			case TEAM_HUMANS: teamColor = '4'; break;
			default: teamColor = '3';
		}

		sprintf( string, "VoIP: ^%c%s", teamColor, Info_ValueForKey(cl.gameState.stringData +
		cl.gameState.stringOffsets[CS_PLAYERS + cls.voipSender], "t" ) );

		if ( cl_voipShowSender->integer == 1 ) // Lower right-hand corner, above HUD
		{
			SCR_DrawStringExt( 320 - strlen( string ) * -8, 365, 8, string, g_color_table[ 7 ], qtrue, qtrue );
		}
		else if ( cl_voipShowSender->integer == 2 ) // Lower left-hand corner, above HUD
		{
			SCR_DrawStringExt( 320 - strlen( string ) * 17, 365, 8, string, g_color_table[ 7 ], qtrue, qtrue );
		}
		else if ( cl_voipShowSender->integer == 3 ) // Top right-hand corner, below lag-o-meter/time
		{
			SCR_DrawStringExt( 320 - strlen( string ) * -9, 100, 8, string, g_color_table[ 7 ], qtrue, qtrue );
		}
		else if ( cl_voipShowSender->integer == 4 ) // Top center, below VOIP bar when it's displayed
		{
			SCR_DrawStringExt( 320 - strlen( string ) * 4, 30, 8, string, g_color_table[ 7 ], qtrue, qtrue );
		}
		else if ( cl_voipShowSender->integer == 5 ) // Bottom center, above HUD
		{
			SCR_DrawStringExt( 320 - strlen( string ) * 4, 400, 8, string, g_color_table[ 7 ], qtrue, qtrue );
		}
		else
		{
			SCR_DrawStringExt( 320 - strlen( string ) * -8, 380, 8, string, g_color_table[ 7 ], qtrue, qtrue );
		}
	}
}

#endif

/*
===============================================================================

DEBUG GRAPH

===============================================================================
*/

typedef struct
{
	float value;
	int   color;
} graphsamp_t;

static int         current;
static graphsamp_t values[ 1024 ];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph( float value, int color )
{
	values[ current & 1023 ].value = value;
	values[ current & 1023 ].color = color;
	current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph( void )
{
	int   a, x, y, w, i, h;
	float v;
//	int             color;

	//
	// draw the graph
	//
	w = cls.glconfig.vidWidth;
	x = 0;
	y = cls.glconfig.vidHeight;
	re.SetColor( g_color_table[ 0 ] );
	re.DrawStretchPic( x, y - cl_graphheight->integer, w, cl_graphheight->integer, 0, 0, 0, 0, cls.whiteShader );
	re.SetColor( NULL );

	for ( a = 0; a < w; a++ )
	{
		i = ( current - 1 - a + 1024 ) & 1023;
		v = values[ i ].value;
//		color = values[i].color;
		v = v * cl_graphscale->integer + cl_graphshift->integer;

		if ( v < 0 )
		{
			v += cl_graphheight->integer * ( 1 + ( int )( -v / cl_graphheight->integer ) );
		}

		h = ( int ) v % cl_graphheight->integer;
		re.DrawStretchPic( x + w - 1 - a, y - h, 1, h, 0, 0, 0, 0, cls.whiteShader );
	}
}

//=============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init( void )
{
	cl_timegraph = Cvar_Get( "timegraph", "0", CVAR_CHEAT );
	cl_debuggraph = Cvar_Get( "debuggraph", "0", CVAR_CHEAT );
	cl_graphheight = Cvar_Get( "graphheight", "32", CVAR_CHEAT );
	cl_graphscale = Cvar_Get( "graphscale", "1", CVAR_CHEAT );
	cl_graphshift = Cvar_Get( "graphshift", "0", CVAR_CHEAT );

	scr_initialized = qtrue;
}

//=======================================================

/*
==================
SCR_DrawScreenField

This will be called twice if rendering in stereo mode
==================
*/
void SCR_DrawScreenField( stereoFrame_t stereoFrame )
{
	extern qboolean mouseActive; // see sdl_input.c

	re.BeginFrame( stereoFrame );

	// wide aspect ratio screens need to have the sides cleared
	// unless they are displaying game renderings
	if ( cls.state != CA_ACTIVE )
	{
		if ( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 )
		{
			re.SetColor( g_color_table[ 0 ] );
			re.DrawStretchPic( 0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader );
			re.SetColor( NULL );
		}
	}

	if ( uivm && !VM_Call( uivm, UI_IS_FULLSCREEN ) )
	{
		switch ( cls.state )
		{
			default:
				Com_Error( ERR_FATAL, "SCR_DrawScreenField: bad cls.state" );

			case CA_CINEMATIC:
				SCR_DrawCinematic();
				break;

			case CA_DISCONNECTED:
				// force menu up
				S_StopAllSounds();
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
				break;

			case CA_CONNECTING:
			case CA_CHALLENGING:
			case CA_CONNECTED:
				// connecting clients will only show the connection dialog
				// refresh to update the time
				VM_Call( uivm, UI_REFRESH, cls.realtime );
				VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qfalse );
				break;

			case CA_LOADING:
			case CA_PRIMED:
				// draw the game information screen and loading progress
				CL_CGameRendering( stereoFrame );

				// also draw the connection information, so it doesn't
				// flash away too briefly on local or LAN games
				//if (!com_sv_running->value || Cvar_VariableIntegerValue("sv_cheats")) // Ridah, don't draw useless text if not in dev mode
				VM_Call( uivm, UI_REFRESH, cls.realtime );
				VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qtrue );
				break;

			case CA_ACTIVE:
				CL_CGameRendering( stereoFrame );
				SCR_DrawDemoRecording();
#ifdef USE_VOIP
				SCR_DrawVoipMeter();
				SCR_DrawVoipSender();
#endif
				break;
		}
	}

	// the menu draws next
	if ( cls.keyCatchers & KEYCATCH_UI && uivm )
	{
		VM_Call( uivm, UI_REFRESH, cls.realtime );
	}

	// console draws next
	Con_DrawConsole();

	if ( uivm && ( CL_UIOwnsMouse() || !mouseActive ) ) {
		// TODO (after no compatibility needed with alpha 8): replace with UI_DRAW_CURSOR
		VM_Call( uivm, UI_MOUSE_POSITION, qtrue );
	}

	// debug graph can be drawn on top of anything
	if ( cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer )
	{
		SCR_DrawDebugGraph();
	}
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen( void )
{
	static int recursive = 0;

	if ( !scr_initialized )
	{
		return; // not initialized yet
	}

	if ( ++recursive >= 2 )
	{
		recursive = 0;
		// Gordon: i'm breaking this again, because we've removed most of our cases but still have one which will not fix easily
		return;
//      Com_Error( ERR_FATAL, "SCR_UpdateScreen: recursively called" );
	}

	recursive = 1;

	// If there is no VM, there are also no rendering commands issued. Stop the renderer in
	// that case.
	if ( uivm || com_dedicated->integer )
	{
		// XXX
//		extern cvar_t* r_anaglyphMode;
		// if running in stereo, we need to draw the frame twice
		if ( cls.glconfig.stereoEnabled )
		{
			SCR_DrawScreenField( STEREO_LEFT );
			SCR_DrawScreenField( STEREO_RIGHT );
		}
		else
		{
			SCR_DrawScreenField( STEREO_CENTER );
		}

		if ( com_speeds->integer )
		{
			re.EndFrame( &time_frontend, &time_backend );
		}
		else
		{
			re.EndFrame( NULL, NULL );
		}
	}

	recursive = 0;
}

float SCR_ConsoleFontUnicharWidth( int ch )
{
	return cls.useLegacyConsoleFont
	       ? SMALLCHAR_WIDTH
	       : Glyph( ch )->xSkip + cl_consoleFontKerning->value;
}

float SCR_ConsoleFontCharWidth( const char *s )
{
	return SCR_ConsoleFontUnicharWidth( Q_UTF8CodePoint( s ) );
}

float SCR_ConsoleFontCharHeight( void )
{
	return cls.useLegacyConsoleFont
	       ? SMALLCHAR_HEIGHT
	       : cls.consoleFont.glyphBlock[0]['I'].imageHeight + CONSOLE_FONT_VPADDING * cl_consoleFontSize->value;
}

float SCR_ConsoleFontCharVPadding( void )
{
	return cls.useLegacyConsoleFont
	       ? 0
	       : MAX( 0, -cls.consoleFont.glyphBlock[0]['g'].bottom >> 6);
}

float SCR_ConsoleFontStringWidth( const char* s, int len )
{
	float width = 0;

	if( cls.useLegacyConsoleFont )
	{
		if( cls.useLegacyConsoleFace )
		{
			return len * SMALLCHAR_WIDTH;
		}
		else
		{
			int l = 0;
			const char *str = s;

			while( *str && len > 0 )
			{
				l++;
				str += Q_UTF8Width( str );
				len--;
			}

			return l * SMALLCHAR_WIDTH;
		}
	}

	while( *s && len > 0 )
	{
		width += SCR_ConsoleFontCharWidth( s );

		s += Q_UTF8Width( s );
		len--;
	}

	return (width);
}
