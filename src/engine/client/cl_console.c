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

// console.c

#include <time.h>
#include "revision.h"
#include "client.h"

int g_console_field_width = 78;

#define CONSOLE_COLOR COLOR_WHITE //COLOR_BLACK

console_t con;

cvar_t    *con_conspeed;
cvar_t    *con_notifytime;
cvar_t    *con_autoclear;

// Color and alpha for console
cvar_t    *scr_conUseShader;

cvar_t    *scr_conColorAlpha;
cvar_t    *scr_conColorRed;
cvar_t    *scr_conColorBlue;
cvar_t    *scr_conColorGreen;

// Color and alpha for bar under console
cvar_t    *scr_conBarHeight;

cvar_t    *scr_conBarColorAlpha;
cvar_t    *scr_conBarColorRed;
cvar_t    *scr_conBarColorBlue;
cvar_t    *scr_conBarColorGreen;

cvar_t    *scr_conUseOld;
cvar_t    *scr_conBarSize;
cvar_t    *scr_conHeight;

#define DEFAULT_CONSOLE_WIDTH 78
#define MAX_CONSOLE_WIDTH   1024

static const vec4_t console_highlightcolor = { 0.5, 0.5, 0.2, 0.45 };

#define CON_LINE(y) ( ( (y) % con.totallines ) * con.linewidth )

// Buffer used by line-to-string code. Implementation detail.
static char lineString[ MAX_CONSOLE_WIDTH * 6 + 4 ];

static const char *Con_LineToString( int lineno, qboolean lf )
{
	const conChar_t *line = con.text + CON_LINE( lineno );
	int              s, d;

	for ( s = d = 0; line[ s ].ch && s < con.linewidth; ++s )
	{
		if ( line[ s ].ch < 0x80 )
		{
			lineString[ d++ ] = (char) line[ s ].ch;
		}
		else
		{
			strcpy( lineString + d, Q_UTF8Encode( line[ s ].ch ) );
			while ( lineString[ d ] ) { ++d; }
		}
	}

	if ( lf )
	{
		lineString[ d++ ] = '\n';
	}

	lineString[ d ] = '\0';
	return lineString;
}

static const char *Con_LineToColouredString( int lineno, qboolean lf )
{
	const conChar_t *line = con.text + CON_LINE( lineno );
	int              s, d, colour = 7;

	for ( s = d = 0; line[ s ].ch && s < con.linewidth; ++s )
	{
		if ( line[ s ].ink != colour )
		{
			colour = line[ s ].ink;
			lineString[ d++ ] = '^';
			lineString[ d++ ] = '0' + colour;
		}

		if ( line[ s ].ch < 0x80 )
		{
			lineString[ d++ ] = (char) line[ s ].ch;
		}
		else
		{
			strcpy( lineString + d, Q_UTF8Encode( line[ s ].ch ) );
			while ( lineString[ d ] ) { ++d; }
		}
	}

	if ( lf )
	{
		lineString[ d++ ] = '\n';
	}

	lineString[ d ] = '\0';
	return lineString;
}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f( void )
{
	con.acLength = 0;

	// ydnar: persistent console input is more useful
	// Arnout: added cvar
	if ( con_autoclear->integer )
	{
		Field_Clear( &g_consoleField );
	}

	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify();

	// ydnar: multiple console size support
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		cls.keyCatchers &= ~KEYCATCH_CONSOLE;
		con.desiredFrac = 0.0;
	}
	else
	{
		cls.keyCatchers |= KEYCATCH_CONSOLE;

		// short console
		if ( keys[ K_CTRL ].down )
		{
			con.desiredFrac = ( 5.0 * SMALLCHAR_HEIGHT ) / cls.glconfig.vidHeight;
		}
		// full console
		else if ( keys[ K_ALT ].down )
		{
			con.desiredFrac = 1.0;
		}
		// half-screen console
		else
		{
			con.desiredFrac = 0.5;
		}
	}
}

void Con_OpenConsole_f( void )
{
	if ( !( cls.keyCatchers & KEYCATCH_CONSOLE ) )
	{
		Con_ToggleConsole_f();
	}
}

/*
================
Con_Clear
================
*/
static INLINE void Con_Clear( void )
{
	int i;
	conChar_t fill = { '\0', ColorIndex( CONSOLE_COLOR ) };

	for ( i = 0; i < CON_TEXTSIZE; ++i )
	{
		con.text[i] = fill;
	}
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f( void )
{
	Con_Clear();
	Con_Bottom(); // go to end
}

/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f( void )
{
	int          l;
	fileHandle_t f;
	char         name[ MAX_STRING_CHARS ];

	l = Cmd_Argc();

	if ( l > 2 )
	{
		Com_Printf("%s", _( "usage: condump [filename]\n" ));
		return;
	}

	if ( l == 1 )
	{
		time_t now = time( NULL );
		strftime( name, sizeof( name ), "condump/%Y%m%d-%H%M%S%z.txt",
		          localtime( &now ) );
	}
	else
	{
		Q_snprintf( name, sizeof( name ), "condump/%s", Cmd_Argv( 1 ) );
	}

	Com_Printf(_( "Dumped console text to %s.\n"), name );

	f = FS_FOpenFileWrite( name );

	if ( !f )
	{
		Com_Printf("%s", _( "ERROR: couldn't open.\n" ));
		return;
	}

	// skip empty lines
	for ( l = con.current - con.totallines + 1; l <= con.current; l++ )
	{
		if ( con.text[ CON_LINE( l ) ].ch )
		{
			break;
		}
	}

	// write the remaining lines
	for ( ; l <= con.current; l++ )
	{
		const char *buffer = Con_LineToString( l, qtrue );
		FS_Write( buffer, strlen( buffer ), f );
	}

	FS_FCloseFile( f );
}

/*
================
Con_Search_f

Scroll up to the first console line containing a string
================
*/
void Con_Search_f( void )
{
	int   l, i;
	int   direction;
	int   c = Cmd_Argc();

	if ( c < 2 )
	{
		Com_Printf(_( "usage: %s <string1> <string2> <…>\n"), Cmd_Argv( 0 ) );
		return;
	}

	direction = Q_stricmp( Cmd_Argv( 0 ), "searchDown" ) ? -1 : 1;

	// check the lines
	for ( l = con.display - 1 + direction; l <= con.current && con.current - l < con.totallines; l += direction )
	{
		const char *buffer = Con_LineToString( l, qtrue );

		// Don't search commands
		for ( i = 1; i < c; i++ )
		{
			if ( Q_stristr( buffer, Cmd_Argv( i ) ) )
			{
				con.display = l + 1;

				if ( con.display > con.current )
				{
					con.display = con.current;
				}

				return;
			}
		}
	}
}

/*
================
Con_Grep_f

Find all console lines containing a string
================
*/
void Con_Grep_f( void )
{
	int    l;
	int    lastcolor;
	char  *search;
	char  *printbuf = NULL;
	size_t pbAlloc = 0, pbLength = 0;

	if ( Cmd_Argc() != 2 )
	{
		Com_Printf("%s", _( "usage: grep <string>\n" ));
		return;
	}

	// skip empty lines
	for ( l = con.current - con.totallines + 1; l <= con.current; l++ )
	{
		if ( con.text[ CON_LINE( l ) ].ch )
		{
			break;
		}
	}

	// check the remaining lines
	search = Cmd_Argv( 1 );
	lastcolor = 7;

	for ( ; l <= con.current; l++ )
	{
		const char *buffer = Con_LineToString( l, qfalse );

		if ( Q_stristr( buffer, search ) )
		{
			size_t i;

			buffer = Con_LineToColouredString( l, qtrue );
			i = strlen( buffer );

			if ( pbLength + i >= pbAlloc )
			{
				char *nb;
				// allocate in 16K chunks - more than adequate
				pbAlloc = ( pbLength + i + 1 + 16383) & ~16383;
				nb = Z_Malloc( pbAlloc );
				if( printbuf )
				{
					strcpy( nb, printbuf );
					Z_Free( printbuf );
				}
				printbuf = nb;
			}
			Q_strcat( printbuf, pbAlloc, buffer );
			pbLength += i;
		}
	}

	if( printbuf )
	{
		char tmpbuf[ MAXPRINTMSG ];
		int i;

		// print out in chunks so we don't go over the MAXPRINTMSG limit
		for ( i = 0; i < pbLength; i += MAXPRINTMSG - 1 )
		{
			Q_strncpyz( tmpbuf, printbuf + i, sizeof( tmpbuf ) );
			Com_Printf( "%s", tmpbuf );
		}

		Z_Free( printbuf );
	}
}

/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void )
{
	int i;

	for ( i = 0; i < NUM_CON_TIMES; i++ )
	{
		con.times[ i ] = 0;
	}
}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize( void )
{
	int   i, width, oldwidth, oldtotallines, numlines, numchars;
	conChar_t buf[ CON_TEXTSIZE ];

	if ( cls.glconfig.vidWidth )
	{
		if ( scr_conUseOld->integer )
		{
			width = cls.glconfig.vidWidth / SCR_ConsoleFontUnicharWidth( 'W' );
		}
		else
		{
			float adjust = 30;
			SCR_AdjustFrom640( &adjust, NULL, NULL, NULL );
			width = ( cls.glconfig.vidWidth - adjust ) / SCR_ConsoleFontUnicharWidth( 'W' );
		}

		g_consoleField.widthInChars = width - Q_PrintStrlen( cl_consolePrompt->string ) - 1;
	}
	else
	{
		width = 0;
	}

	if ( width == con.linewidth )
	{
		// nothing
	}
	else if ( width < 1 ) // video hasn't been initialized yet
	{
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		Con_Clear();

		con.current = con.totallines - 1;
		con.display = con.current;
	}
	else
	{
		SCR_AdjustFrom640( &con.xadjust, NULL, NULL, NULL );

		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if ( con.totallines < numlines )
		{
			numlines = con.totallines;
		}

		numchars = oldwidth;

		if ( con.linewidth < numchars )
		{
			numchars = con.linewidth;
		}

		Com_Memcpy( buf, con.text, sizeof( con.text ) );
		Con_Clear();

		for ( i = 0; i < numlines; i++ )
		{
			memcpy( con.text + ( con.totallines - 1 - i ) * con.linewidth,
			        buf + ( ( con.current - i + oldtotallines ) % oldtotallines ) * oldwidth,
			        numchars * sizeof( conChar_t ) );
		}

		con.current = con.totallines - 1;
		con.display = con.current;
	}

	g_console_field_width = g_consoleField.widthInChars = con.linewidth - 7 - ( cl_consolePrompt ? Q_UTF8Strlen( cl_consolePrompt->string ) : 0 );
}

/*
================
Con_Init
================
*/
void Con_Init( void )
{
	con_notifytime = Cvar_Get( "con_notifytime", "7", 0 );  // JPW NERVE increased per id req for obits
	con_conspeed = Cvar_Get( "scr_conspeed", "3", 0 );
	con_autoclear = Cvar_Get( "con_autoclear", "1", CVAR_ARCHIVE );

	// Defines cvar for color and alpha for console/bar under console
	scr_conUseShader = Cvar_Get( "scr_conUseShader", "0", CVAR_ARCHIVE );

	scr_conColorAlpha = Cvar_Get( "scr_conColorAlpha", "0.5", CVAR_ARCHIVE );
	scr_conColorRed = Cvar_Get( "scr_conColorRed", "0", CVAR_ARCHIVE );
	scr_conColorBlue = Cvar_Get( "scr_conColorBlue", "0.3", CVAR_ARCHIVE );
	scr_conColorGreen = Cvar_Get( "scr_conColorGreen", "0.23", CVAR_ARCHIVE );

	scr_conUseOld = Cvar_Get( "scr_conUseOld", "0", CVAR_ARCHIVE );

	scr_conBarHeight = Cvar_Get( "scr_conBarHeight", "2", CVAR_ARCHIVE );

	scr_conBarColorAlpha = Cvar_Get( "scr_conBarColorAlpha", "0.3", CVAR_ARCHIVE );
	scr_conBarColorRed = Cvar_Get( "scr_conBarColorRed", "1", CVAR_ARCHIVE );
	scr_conBarColorBlue = Cvar_Get( "scr_conBarColorBlue", "1", CVAR_ARCHIVE );
	scr_conBarColorGreen = Cvar_Get( "scr_conBarColorGreen", "1", CVAR_ARCHIVE );

	scr_conHeight = Cvar_Get( "scr_conHeight", "50", CVAR_ARCHIVE );

	scr_conBarSize = Cvar_Get( "scr_conBarSize", "2", CVAR_ARCHIVE );

	// Done defining cvars for console colors

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;

	Cmd_AddCommand( "toggleConsole", Con_ToggleConsole_f );
	Cmd_AddCommand( "clear", Con_Clear_f );
	Cmd_AddCommand( "condump", Con_Dump_f );
	Cmd_AddCommand( "search", Con_Search_f );
	Cmd_AddCommand( "searchDown", Con_Search_f );
	Cmd_AddCommand( "grep", Con_Grep_f );
}

/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed( qboolean skipnotify )
{
	int             i;
	conChar_t       *line;
	const conChar_t blank = { 0, ColorIndex( CONSOLE_COLOR ) };

	// mark time for transparent overlay
	if ( con.current >= 0 )
	{
		con.times[ con.current % NUM_CON_TIMES ] = skipnotify ? 0 : cls.realtime;
	}

	con.x = 0;

	if ( con.display == con.current )
	{
		con.display++;
	}

	con.current++;

	line = con.text + CON_LINE( con.current );

	for ( i = 0; i < con.linewidth; ++i )
	{
		line[ i ] = blank;
	}
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
#if defined( _WIN32 ) && defined( NDEBUG )
#pragma optimize( "g", off ) // SMF - msvc totally screws this function up with optimize on
#endif

void CL_ConsolePrint( char *txt )
{
	int      y;
	int      c, i, l;
	int      color;
	qboolean skipnotify = qfalse; // NERVE - SMF
	int      prev; // NERVE - SMF

	// NERVE - SMF - work around for text that shows up in console but not in notify
	if ( !Q_strncmp( txt, "[skipnotify]", 12 ) )
	{
		skipnotify = qtrue;
		txt += 12;
	}

	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer )
	{
		return;
	}

	if ( !con.initialized )
	{
		con.color[ 0 ] = con.color[ 1 ] = con.color[ 2 ] = con.color[ 3 ] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize();
		con.initialized = qtrue;
	}

	if ( !skipnotify && !( cls.keyCatchers & KEYCATCH_CONSOLE ) && strncmp( txt, "EXCL: ", 6 ) )
	{
		// feed the text to cgame
		Cmd_SaveCmdContext();
		Cmd_TokenizeString( txt );
		CL_GameConsoleText();
		Cmd_RestoreCmdContext();
	}

	color = ColorIndex( CONSOLE_COLOR );

	while ( ( c = *txt & 0xFF ) != 0 )
	{
		if ( Q_IsColorString( txt ) )
		{
			color = ( txt[ 1 ] == COLOR_NULL ) ? ColorIndex( CONSOLE_COLOR ) : ColorIndex( txt[ 1 ] );
			txt += 2;
			continue;
		}

		// count word length
		for ( i = l = 0; l < con.linewidth; ++l )
		{
			if ( txt[ i ] <= ' ' && txt[ i ] >= 0 )
			{
				break;
			}

			if ( txt[ i ] == Q_COLOR_ESCAPE && txt[ i + 1 ] == Q_COLOR_ESCAPE )
			{
				++i;
			}

			i += Q_UTF8Width( txt + i );
		}

		// word wrap
		if ( l != con.linewidth && ( con.x + l >= con.linewidth ) )
		{
			Con_Linefeed( skipnotify );
		}

		switch ( c )
		{
			case '\n':
				Con_Linefeed( skipnotify );
				break;

			case '\r':
				con.x = 0;
				break;

			case Q_COLOR_ESCAPE:
				if ( txt[ 1 ] == Q_COLOR_ESCAPE )
				{
					++txt;
				}

			default: // display character and advance
				y = con.current % con.totallines;
				// rain - sign extension caused the character to carry over
				// into the color info for high ascii chars; casting c to unsigned
				con.text[ y * con.linewidth + con.x ].ch = Q_UTF8CodePoint( txt );
				con.text[ y * con.linewidth + con.x ].ink = color;
				++con.x;

				if ( con.x >= con.linewidth )
				{
					Con_Linefeed( skipnotify );
					con.x = 0;
				}

				break;
		}

		txt += Q_UTF8Width( txt );
	}

	// mark time for transparent overlay
	if ( con.current >= 0 )
	{
		// NERVE - SMF
		if ( skipnotify )
		{
			prev = con.current % NUM_CON_TIMES - 1;

			if ( prev < 0 )
			{
				prev = NUM_CON_TIMES - 1;
			}

			con.times[ prev ] = 0;
		}
		else
		{
			// -NERVE - SMF
			con.times[ con.current % NUM_CON_TIMES ] = cls.realtime;
		}
	}
}

#if defined( _WIN32 ) && defined( NDEBUG )
#pragma optimize( "g", on ) // SMF - re-enabled optimization
#endif

/*
==============================================================================

DRAWING

==============================================================================
*/

/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput( void )
{
	int     y;
	char    prompt[ MAX_STRING_CHARS ];
	vec4_t  color;
	qtime_t realtime;

	if ( cls.state != CA_DISCONNECTED && !( cls.keyCatchers & KEYCATCH_CONSOLE ) )
	{
		return;
	}

	Com_RealTime( &realtime );

	y = con.vislines - ( SCR_ConsoleFontCharHeight() * 2 ) + 2;

	Com_sprintf( prompt,  sizeof( prompt ), "^0[^3%02d%c%02d^0]^7 %s", realtime.tm_hour, ( realtime.tm_sec & 1 ) ? ':' : ' ', realtime.tm_min, cl_consolePrompt->string );

	color[ 0 ] = 1.0f;
	color[ 1 ] = 1.0f;
	color[ 2 ] = 1.0f;
	color[ 3 ] = ( scr_conUseOld->integer ? 1.0f : con.displayFrac * 2.0f );

	SCR_DrawSmallStringExt( con.xadjust + cl_conXOffset->integer, y + 10, prompt, color, qfalse, qfalse );

	Q_CleanStr( prompt );
	Field_Draw( &g_consoleField, con.xadjust + cl_conXOffset->integer + SCR_ConsoleFontStringWidth( prompt, strlen( prompt ) ), y + 10, qtrue, qtrue, color[ 3 ] );
}

/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify( void )
{
	int   x, v;
	int   i;
	int   time;
	int   skip = 0;
	int   currentColor;

	conChar_t *text;

	currentColor = 7;
	re.SetColor( g_color_table[ currentColor ] );

	v = 0;

	for ( i = con.current - NUM_CON_TIMES + 1; i <= con.current; i++ )
	{
		if ( i < 0 )
		{
			continue;
		}

		time = con.times[ i % NUM_CON_TIMES ];

		if ( time == 0 )
		{
			continue;
		}

		time = cls.realtime - time;

		if ( time > con_notifytime->value * 1000 )
		{
			continue;
		}

		text = con.text + CON_LINE( i );

		if ( cl.snap.ps.pm_type != PM_INTERMISSION && cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) )
		{
			continue;
		}

		for ( x = 0; x < con.linewidth && text[ x ].ch; ++x )
		{
			if ( text[ x ].ch == ' ' )
			{
				continue;
			}

			if ( text[ x ].ink != currentColor )
			{
				currentColor = text[ x ].ink;
				re.SetColor( g_color_table[ currentColor ] );
			}

			SCR_DrawSmallUnichar( cl_conXOffset->integer + con.xadjust + ( x + 1 ) * SMALLCHAR_WIDTH, v, text[ x ].ch );
		}

		v += SMALLCHAR_HEIGHT;
	}

	re.SetColor( NULL );

	if ( cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) )
	{
		return;
	}

	// draw the chat line
	if ( cls.keyCatchers & KEYCATCH_MESSAGE )
	{
		if ( chat_irc )
		{
			char buf[ 128 ];

			SCR_DrawBigString( 8, v, "say_irc:", 1.0f, qfalse );
			skip = strlen( buf ) + 2;
		}

		Field_BigDraw( &chatField, skip * BIGCHAR_WIDTH, 232, qtrue, qtrue );

		v += BIGCHAR_HEIGHT;
	}
}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole( float frac )
{
	int    i, x, y;
	int    rows;
	int    row;
	int    lines;
//	qhandle_t    conShader;
	int    currentColor;
	vec4_t color;
	float  yVer;
	float  totalwidth;
	float  currentWidthLocation = 0;

	const int charHeight = SCR_ConsoleFontCharHeight();

	if ( scr_conUseOld->integer )
	{
		lines = cls.glconfig.vidHeight * frac;

		if ( lines <= 0 )
		{
			return;
		}

		if ( lines > cls.glconfig.vidHeight )
		{
			lines = cls.glconfig.vidHeight;
		}
	}
	else
	{
		lines = cls.glconfig.vidHeight * frac;
	}
	lines += charHeight / ( CONSOLE_FONT_VPADDING + 1 );

	// on wide screens, we will center the text
	if (!scr_conUseOld->integer)
	{
		con.xadjust = 15;
	}
	else
	{
		con.xadjust = 0;
	}

	SCR_AdjustFrom640 (&con.xadjust, NULL, NULL, NULL);

	// draw the background
	if ( scr_conUseOld->integer )
	{
		yVer = 5 + charHeight;
		y = frac * SCREEN_HEIGHT;

		if ( y < 1 )
		{
			y = 0;
		}
		else
		{
			if ( scr_conUseShader->integer )
			{
				SCR_DrawPic( 0, 0, SCREEN_WIDTH, y, cls.consoleShader );
			}
			else
			{
				// This will be overwritten, so i'll just abuse it here, no need to define another array
				color[ 0 ] = scr_conColorRed->value;
				color[ 1 ] = scr_conColorGreen->value;
				color[ 2 ] = scr_conColorBlue->value;
				color[ 3 ] = scr_conColorAlpha->value;

				SCR_FillRect( 0, 0, SCREEN_WIDTH, y, color );
			}
		}

		color[ 0 ] = scr_conBarColorRed->value;
		color[ 1 ] = scr_conBarColorGreen->value;
		color[ 2 ] = scr_conBarColorBlue->value;
		color[ 3 ] = scr_conBarColorAlpha->value;

		SCR_FillRect( 0, y, SCREEN_WIDTH, scr_conBarSize->value, color );
	}
	else
	{
		yVer = 10;
		SCR_AdjustFrom640( NULL, &yVer, NULL, NULL );
		yVer = floor( yVer + 5 + charHeight );

		color[ 0 ] = scr_conColorRed->value;
		color[ 1 ] = scr_conColorGreen->value;
		color[ 2 ] = scr_conColorBlue->value;
		color[ 3 ] = ( frac / con.finalFrac ) * 2 * scr_conColorAlpha->value;
		SCR_FillRect( 10, 10, 620, 460 * scr_conHeight->integer * 0.01, color );

		color[ 0 ] = scr_conBarColorRed->value;
		color[ 1 ] = scr_conBarColorGreen->value;
		color[ 2 ] = scr_conBarColorBlue->value;
		color[ 3 ] = ( frac / con.finalFrac ) * 2 * scr_conBarColorAlpha->value;
		SCR_FillRect( 10, 10, 620, 1, color );  //top
		SCR_FillRect( 10, 460 * scr_conHeight->integer * 0.01 + 10, 621, 1, color );  //bottom
		SCR_FillRect( 10, 10, 1, 460 * scr_conHeight->integer * 0.01, color );  //left
		SCR_FillRect( 630, 10, 1, 460 * scr_conHeight->integer * 0.01, color );  //right
	}

	// draw the version number

	color[ 0 ] = 1.0f;
	color[ 1 ] = 1.0f;
	color[ 2 ] = 1.0f;
	color[ 3 ] = ( scr_conUseOld->integer ? 0.75f : ( frac / con.finalFrac ) * 0.75f );
	re.SetColor( color );

	i = strlen( Q3_VERSION );
	totalwidth = SCR_ConsoleFontStringWidth( Q3_VERSION, i ) + cl_conXOffset->integer;

	if ( !scr_conUseOld->integer )
	{
		totalwidth += 30;
	}

	currentWidthLocation = cls.glconfig.vidWidth - totalwidth;

	for ( x = 0; x < i; x++ )
	{
		int ch = Q_UTF8CodePoint( &Q3_VERSION[ x ] );
		SCR_DrawConsoleFontUnichar( currentWidthLocation, yVer, ch );
		currentWidthLocation += SCR_ConsoleFontUnicharWidth( ch );
	}

	// engine string
	i = strlen( Q3_ENGINE );
	totalwidth = SCR_ConsoleFontStringWidth( Q3_ENGINE, i ) + cl_conXOffset->integer;

	if ( !scr_conUseOld->integer )
	{
		totalwidth += 30;
	}

	currentWidthLocation = cls.glconfig.vidWidth - totalwidth;

	for ( x = 0; x < i; x++ )
	{
		int ch = Q_UTF8CodePoint( &Q3_ENGINE[ x ] );
		SCR_DrawConsoleFontUnichar( currentWidthLocation, yVer + charHeight, ch );
		currentWidthLocation += SCR_ConsoleFontUnicharWidth( ch );
	}

	// draw the input prompt, user text, and cursor if desired
	// moved back here (have observed render issues to do with time taken)
	Con_DrawInput();

	// draw the text
	con.vislines = lines;
	rows = ( lines ) / SCR_ConsoleFontCharHeight() - 3; // rows of text to draw

	if ( scr_conUseOld->integer )
	{
		rows++;
	}

	y = lines - ( SCR_ConsoleFontCharHeight() * 3 ) + 10;

	// draw from the bottom up
	if ( con.display != con.current )
	{
		// draw arrows to show the buffer is backscrolled
		const int hatWidth = SCR_ConsoleFontUnicharWidth( '^' );

		color[ 0 ] = 1.0f;
		color[ 1 ] = 0.0f;
		color[ 2 ] = 0.0f;
		color[ 3 ] = ( scr_conUseOld->integer ? 1.0f : ( frac / con.finalFrac ) * 2.0f );
		re.SetColor( color );

		for ( x = 0; x < con.linewidth - ( scr_conUseOld->integer ? 0 : 4 ); x += 4 )
		{
			SCR_DrawConsoleFontUnichar( con.xadjust + ( x + 1 ) * hatWidth, y, '^' );
		}

		y -= charHeight;
		rows--;
	}

	row = con.display;

	if ( con.x == 0 )
	{
		row--;
	}

	currentColor = 7;
	color[ 0 ] = g_color_table[ currentColor ][ 0 ];
	color[ 1 ] = g_color_table[ currentColor ][ 1 ];
	color[ 2 ] = g_color_table[ currentColor ][ 2 ];
	color[ 3 ] = ( scr_conUseOld->integer ? 1.0f : ( frac / con.finalFrac ) * 2.0f );
	re.SetColor( color );

	for ( i = 0; i < rows; i++, y -= charHeight, row-- )
	{
		conChar_t *text;

		if ( row < 0 )
		{
			break;
		}

		if ( con.current - row >= con.totallines )
		{
			// past scrollback wrap point
			continue;
		}

		text = con.text + CON_LINE( row );

		currentWidthLocation = cl_conXOffset->integer;

		for ( x = 0; x < con.linewidth && text[x].ch; ++x )
		{
			if ( text[ x ].ink != currentColor )
			{
				currentColor = text[ x ].ink;
				color[ 0 ] = g_color_table[ currentColor ][ 0 ];
				color[ 1 ] = g_color_table[ currentColor ][ 1 ];
				color[ 2 ] = g_color_table[ currentColor ][ 2 ];
				color[ 3 ] = ( scr_conUseOld->integer ? 1.0f : 1.0f );
				re.SetColor( color );
			}

			SCR_DrawConsoleFontUnichar( con.xadjust + currentWidthLocation, y, text[ x ].ch );
			currentWidthLocation += SCR_ConsoleFontUnicharWidth( text[ x ].ch );
		}
	}

	re.SetColor( NULL );
}

extern cvar_t *con_drawnotify;

/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void )
{
	// check for console width changes from a vid mode change
	Con_CheckResize();

	// if disconnected, render console full screen
	if ( cls.state == CA_DISCONNECTED )
	{
		if ( !( cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) ) )
		{
			Con_DrawSolidConsole( 1.0 );
			return;
		}
	}

	if ( con.displayFrac )
	{
		Con_DrawSolidConsole( con.displayFrac );
	}
	else
	{
		// draw notify lines
		if ( cls.state == CA_ACTIVE && con_drawnotify->integer )
		{
			Con_DrawNotify();
		}
	}
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole( void )
{
	// decide on the destination height of the console
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		if ( scr_conUseOld->integer )
		{
			con.finalFrac = MAX( 0.10, 0.01 * scr_conHeight->integer );  // configured console percentage
		}
		else
		{
			con.finalFrac = scr_conHeight->integer * .01;
		}
	}
	else
	{
		con.finalFrac = 0; // none visible
	}

	// scroll towards the destination height
	if ( con.finalFrac < con.displayFrac )
	{
		con.displayFrac -= con_conspeed->value * cls.realFrametime * 0.001;

		if ( con.finalFrac > con.displayFrac )
		{
			con.displayFrac = con.finalFrac;
		}
	}
	else if ( con.finalFrac > con.displayFrac )
	{
		con.displayFrac += con_conspeed->value * cls.realFrametime * 0.001;

		if ( con.finalFrac < con.displayFrac )
		{
			con.displayFrac = con.finalFrac;
		}
	}
}

void Con_PageUp( void )
{
	con.display -= 2;

	if ( con.current - con.display >= con.totallines )
	{
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown( void )
{
	con.display += 2;

	if ( con.display > con.current )
	{
		con.display = con.current;
	}
}

void Con_Top( void )
{
	con.display = con.totallines;

	if ( con.current - con.display >= con.totallines )
	{
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom( void )
{
	con.display = con.current;
}

void Con_Close( void )
{
	if ( !com_cl_running->integer )
	{
		return;
	}

	Field_Clear( &g_consoleField );
	Con_ClearNotify();
	cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	con.finalFrac = 0; // none visible
	con.displayFrac = 0;
}
