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

#include "revision.h"
#include "client.h"
#include "qcommon/q_unicode.h"
#include "framework/LogSystem.h"


#define CONSOLE_COLOR COLOR_WHITE //COLOR_BLACK
#define DEFAULT_CONSOLE_WIDTH 78
#define MAX_CONSOLE_WIDTH   1024

int g_console_field_width = DEFAULT_CONSOLE_WIDTH;

console_t consoleState;

cvar_t    *con_animationSpeed;
cvar_t    *con_animationType;

cvar_t    *con_autoclear;

/**
 * 0: no scroll lock at all, scroll down on any message arriving
 * 1: lock scrolling if in scrollback, but scroll down for send message/entered commands
 * 2: lock scrolling if in scrollback, even for own output
 * 3: always lock scrolling
 */
cvar_t    *con_scrollLock;

cvar_t	  *con_prompt;

cvar_t    *con_borderWidth;
cvar_t    *con_borderColorAlpha;
cvar_t    *con_borderColorRed;
cvar_t    *con_borderColorBlue;
cvar_t    *con_borderColorGreen;

cvar_t    *con_margin;
cvar_t    *con_horizontalPadding;

cvar_t    *con_height;
cvar_t    *con_colorAlpha;
cvar_t    *con_colorRed;
cvar_t    *con_colorBlue;
cvar_t    *con_colorGreen;

/**
 * allows for debugging the console without using the consoles scrollback,
 * which might otherwise end in loops or unnecessary verbose output
 */
cvar_t    *con_debug;

#define ANIMATION_TYPE_NONE   0
#define ANIMATION_TYPE_SCROLL_DOWN 1
#define ANIMATION_TYPE_FADE   2
#define ANIMATION_TYPE_BOTH   3

#define CON_LINE(line) ( ( (line) % consoleState.maxScrollbackLengthInLines ) * consoleState.textWidthInChars )

// Buffer used by line-to-string code. Implementation detail.
static char lineString[ MAX_CONSOLE_WIDTH * 6 + 5 ];

static const char *Con_LineToString( int lineno, bool lf )
{
	const conChar_t *line = consoleState.text + CON_LINE( lineno );
	int              s, d;

	for ( s = d = 0; line[ s ].ch && s < consoleState.textWidthInChars; ++s )
	{
		if ( line[ s ].ch < 0x80 )
		{
			lineString[ d++ ] = (char) line[ s ].ch;
		}
		else
		{
			strcpy( lineString + d, Q_UTF8_Encode( line[ s ].ch ) );
			while ( lineString[ d ] ) { ++d; }
		}
	}

	if ( lf )
	{
#ifdef _WIN32
		lineString[ d++ ] = '\r';
#endif
		lineString[ d++ ] = '\n';
	}

	lineString[ d ] = '\0';
	return lineString;
}

static const char *Con_LineToColouredString( int lineno, bool lf )
{
	const conChar_t *line = consoleState.text + CON_LINE( lineno );
	int              s, d, colour = 7;

	for ( s = d = 0; line[ s ].ch && s < consoleState.textWidthInChars; ++s )
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
			strcpy( lineString + d, Q_UTF8_Encode( line[ s ].ch ) );
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
void Con_ToggleConsole_f()
{
	// ydnar: persistent console input is more useful
	if ( con_autoclear->integer )
	{
		g_consoleField.Clear();
	}

	g_consoleField.SetWidth(g_console_field_width);

	if (consoleState.isOpened) {
		cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	} else {
		cls.keyCatchers |= KEYCATCH_CONSOLE;
	}
}

void Con_OpenConsole_f()
{
	if ( !consoleState.isOpened )
	{
		Con_ToggleConsole_f();
	}
}

/*
================
Con_Clear
================
*/
static INLINE void Con_Clear()
{
	int i;
	conChar_t fill = { '\0', ColorIndex( CONSOLE_COLOR ) };

	for ( i = 0; i < CON_TEXTSIZE; ++i )
	{
		consoleState.text[i] = fill;
	}

	consoleState.usedScrollbackLengthInLines = 0;
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f()
{
	Con_Clear();
	Con_ScrollToBottom(); // go to end
}

/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f()
{
	int          l;
	fileHandle_t f;
	char         name[ MAX_STRING_CHARS ];

	l = Cmd_Argc();

	if ( l > 2 )
	{
		Cmd_PrintUsage("[<filename>]", nullptr);
		return;
	}

	if ( l == 1 )
	{
		time_t now = time( nullptr );
		strftime( name, sizeof( name ), "condump/%Y%m%d-%H%M%S%z.txt",
		          localtime( &now ) );
	}
	else
	{
		Q_snprintf( name, sizeof( name ), "condump/%s", Cmd_Argv( 1 ) );
	}

	f = FS_FOpenFileWrite( name );

	if ( !f )
	{
		Com_Log(LOG_ERROR, "couldn't open." );
		return;
	}

	Com_Printf( "Dumped console text to %s.\n", name );

	// skip empty lines
	for ( l = consoleState.currentLine - consoleState.maxScrollbackLengthInLines + 1; l <= consoleState.currentLine; l++ )
	{
		if ( consoleState.text[ CON_LINE( l ) ].ch )
		{
			break;
		}
	}

	// write the remaining lines
	for ( ; l <= consoleState.currentLine; l++ )
	{
		const char *buffer = Con_LineToString( l, true );
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
void Con_Search_f()
{
	int   l, i;
	int   direction;
	int   c = Cmd_Argc();

	if ( c < 2 )
	{
		Cmd_PrintUsage("<string>…", nullptr);
		return;
	}

	direction = Q_stricmp( Cmd_Argv( 0 ), "searchDown" ) ? -1 : 1;

	// check the lines
	for ( l = consoleState.scrollLineIndex - 1 + direction; l <= consoleState.currentLine && consoleState.currentLine - l < consoleState.maxScrollbackLengthInLines; l += direction )
	{
		const char *buffer = Con_LineToString( l, true );

		// Don't search commands
		for ( i = 1; i < c; i++ )
		{
			if ( Q_stristr( buffer, Cmd_Argv( i ) ) )
			{
				consoleState.scrollLineIndex = l + 1;

				if ( consoleState.scrollLineIndex > consoleState.currentLine )
				{
					consoleState.bottomDisplayedLine = consoleState.currentLine;
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
void Con_Grep_f()
{
	int    l;
	const char  *search;
	char  *printbuf = nullptr;
	size_t pbAlloc = 0, pbLength = 0;

	if ( Cmd_Argc() != 2 )
	{
		Cmd_PrintUsage("<string>", nullptr);
		return;
	}

	// skip empty lines
	for ( l = consoleState.currentLine - consoleState.maxScrollbackLengthInLines + 1; l <= consoleState.currentLine; l++ )
	{
		if ( consoleState.text[ CON_LINE( l ) ].ch )
		{
			break;
		}
	}

	// check the remaining lines
	search = Cmd_Argv( 1 );

	for ( ; l <= consoleState.currentLine; l++ )
	{
		const char *buffer = Con_LineToString( l, false );

		if ( Q_stristr( buffer, search ) )
		{
			size_t i;

			buffer = Con_LineToColouredString( l, true );
			i = strlen( buffer );

			if ( pbLength + i >= pbAlloc )
			{
				char *nb;
				// allocate in 16K chunks - more than adequate
				pbAlloc = ( pbLength + i + 1 + 16383) & ~16383;
				nb = (char*) Z_Malloc( pbAlloc );
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

		// print out in chunks so we don't go over the MAXPRINTMSG limit
		for (unsigned i = 0; i < pbLength; i += MAXPRINTMSG - 1 )
		{
			Q_strncpyz( tmpbuf, printbuf + i, sizeof( tmpbuf ) );
			Com_Printf( "%s", tmpbuf );
		}

		Z_Free( printbuf );
	}
}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
bool Con_CheckResize()
{
	int   i, textWidthInChars, oldwidth, oldtotallines, numlines, numchars;
	conChar_t buf[ CON_TEXTSIZE ];
	bool  ret = true;

	if ( cls.glconfig.vidWidth )
	{
		const int consoleVidWidth = cls.glconfig.vidWidth - 2 * (consoleState.margin.sides + consoleState.padding.sides );
		textWidthInChars = consoleVidWidth / SCR_ConsoleFontUnicharWidth( 'W' );
	}
	else
	{
		textWidthInChars = 0;
	}

	if ( textWidthInChars == consoleState.textWidthInChars )
	{
		// nothing
	}
	else if ( textWidthInChars < 1 ) // video hasn't been initialized yet
	{
		consoleState.textWidthInChars = DEFAULT_CONSOLE_WIDTH;
		consoleState.maxScrollbackLengthInLines = CON_TEXTSIZE / consoleState.textWidthInChars;
		Con_Clear();

		consoleState.currentLine = consoleState.maxScrollbackLengthInLines - 1;
		consoleState.bottomDisplayedLine = consoleState.currentLine;
		consoleState.scrollLineIndex = consoleState.currentLine;

		ret = false;
	}
	else
	{
		oldwidth = consoleState.textWidthInChars;
		consoleState.textWidthInChars = textWidthInChars;
		oldtotallines = consoleState.maxScrollbackLengthInLines;
		consoleState.maxScrollbackLengthInLines = CON_TEXTSIZE / consoleState.textWidthInChars;

		if ( oldtotallines > 0 && oldwidth > 0 && consoleState.usedScrollbackLengthInLines > 0 ) {

			numlines = std::min( {consoleState.maxScrollbackLengthInLines, consoleState.usedScrollbackLengthInLines, oldtotallines} );
			numchars = std::min( oldwidth, consoleState.textWidthInChars );

			Com_Memcpy( buf, consoleState.text, sizeof( consoleState.text ) );
			Con_Clear();

			for ( i = 0; i < numlines; i++ ) {
				conChar_t* destination = consoleState.text + ( consoleState.maxScrollbackLengthInLines - 1 - i ) * consoleState.textWidthInChars;
				memcpy( destination,
				    buf + ( ( consoleState.currentLine - i - 1 + oldtotallines ) % oldtotallines ) * oldwidth,
				    numchars * sizeof( conChar_t ) );
			}
			consoleState.usedScrollbackLengthInLines = numlines;
		} else {
			Con_Clear();
		}
		consoleState.currentLine = consoleState.maxScrollbackLengthInLines;
		consoleState.bottomDisplayedLine = consoleState.currentLine;
		consoleState.scrollLineIndex = consoleState.currentLine;
	}

	if ( con_prompt )
	{
		char prompt[ MAX_STRING_CHARS ];

		Q_strncpyz( prompt, con_prompt->string, sizeof( prompt ) );
		Q_CleanStr( prompt );
		// 8 spaces for clock, 1 for cursor
		g_console_field_width = consoleState.textWidthInChars - 9 - Q_UTF8_Strlen( prompt );
		g_consoleField.SetWidth(g_console_field_width);
	}

	return ret;
}

/*
================
Con_Init
================
*/
void Con_Init()
{
	con_animationSpeed = Cvar_Get( "con_animationSpeed", "3", 0 );
	con_animationType = Cvar_Get( "con_animationType", "2", 0 );

	con_autoclear = Cvar_Get( "con_autoclear", "1", 0 );
	con_scrollLock = Cvar_Get( "con_scrollLock", "2", 0 );

	con_prompt = Cvar_Get( "con_prompt", "^3->", 0 );

	con_height = Cvar_Get( "con_height", "55", 0 );
	con_colorRed = Cvar_Get( "con_colorRed", "0", 0 );
	con_colorBlue = Cvar_Get( "con_colorBlue", "0.3", 0 );
	con_colorGreen = Cvar_Get( "con_colorGreen", "0.18", 0 );
	con_colorAlpha = Cvar_Get( "con_colorAlpha", "0.5", 0 );

	con_margin = Cvar_Get( "con_margin", "10", 0 );
	con_horizontalPadding = Cvar_Get( "con_horizontalPadding", "0", 0 );

	con_borderWidth = Cvar_Get( "con_borderWidth", "1", 0 );
	con_borderColorRed = Cvar_Get( "con_borderColorRed", "1", 0 );
	con_borderColorBlue = Cvar_Get( "con_borderColorBlue", "1", 0 );
	con_borderColorGreen = Cvar_Get( "con_borderColorGreen", "1", 0 );
	con_borderColorAlpha = Cvar_Get( "con_borderColorAlpha", "0.2", 0 );

	con_debug = Cvar_Get( "con_debug", "0", 0 );

	// Done defining cvars for console colors

	g_consoleField.Clear();
	g_consoleField.SetWidth(g_console_field_width);

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
void Con_Linefeed()
{
	int             i;
	conChar_t       *line;
	const conChar_t blank = { 0, ColorIndex( CONSOLE_COLOR ) };
	const int scrollLockMode = con_scrollLock ? con_scrollLock->integer : 0;

	consoleState.horizontalCharOffset = 0;

	//fall down to input line if scroll lock is configured to do so
	if(scrollLockMode <= 0)
	{
		consoleState.scrollLineIndex = consoleState.currentLine;
	}

	//dont scroll a line down on a scrollock configured to do so
	if ( consoleState.scrollLineIndex >= consoleState.currentLine && scrollLockMode < 3 )
	{
		consoleState.scrollLineIndex++;
	}

	consoleState.currentLine++;

	if( consoleState.usedScrollbackLengthInLines < consoleState.maxScrollbackLengthInLines )
		consoleState.usedScrollbackLengthInLines++;

	line = consoleState.text + CON_LINE( consoleState.currentLine );

	for ( i = 0; i < consoleState.textWidthInChars; ++i )
	{
		line[ i ] = blank;
	}
}

/*
================
CL_InternalConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
#if defined( _WIN32 ) && defined( NDEBUG )
#pragma optimize( "g", off ) // SMF - msvc totally screws this function up with optimize on
#endif

bool CL_InternalConsolePrint( const char *text )
{
	int      y;
	int      c, i;
	int      color;
	int      wordLen = 0;

	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer )
	{
		return true;
	}

	if ( !consoleState.initialized )
	{
		consoleState.textWidthInChars = -1;
		consoleState.initialized = Con_CheckResize();
	}

	//Video hasn't been initialized
	if ( ! cls.glconfig.vidWidth ) {
		return false;
	}

	// NERVE - SMF - work around for text that shows up in console but not in notify
	if ( !Q_strncmp( text, S_SKIPNOTIFY, 12 ) )
	{
		text += 12;
	}
	else if ( !consoleState.isOpened && strncmp( text, "EXCL: ", 6 ) )
	{
		// feed the text to cgame
		Cmd_SaveCmdContext();
		cgvm.CGameConsoleLine( text );
	}

	color = ColorIndex( CONSOLE_COLOR );

	while ( ( c = *text & 0xFF ) != 0 )
	{
		if ( Q_IsColorString( text ) )
		{
			color = ( text[ 1 ] == COLOR_NULL ) ? ColorIndex( CONSOLE_COLOR ) : ColorIndex( text[ 1 ] );
			text += 2;
			continue;
		}

		if ( !wordLen )
		{
			// count word length
			for ( i = 0; ; ++wordLen )
			{
				if ( text[ i ] <= ' ' && text[ i ] >= 0 )
				{
					break;
				}
				if ( Q_IsColorString( text + i ) )
				{
					i += 2;
					continue;
				}
				if ( text[ i ] == Q_COLOR_ESCAPE && text[ i + 1 ] == Q_COLOR_ESCAPE )
				{
					++i;
				}

				i += Q_UTF8_Width( text + i );
			}

			// word wrap
			if ( consoleState.horizontalCharOffset + wordLen >= consoleState.textWidthInChars
			       && consoleState.horizontalCharOffset > 0 )
			{
				Con_Linefeed();
			}
		}

		switch ( c )
		{
			case '\n':
				Con_Linefeed( );
				break;

			case '\r':
				consoleState.horizontalCharOffset = 0;
				break;

			case Q_COLOR_ESCAPE:
				if ( text[ 1 ] == Q_COLOR_ESCAPE )
				{
					++text;
				}
				/* no break */

			default: // display character and advance
				y = consoleState.currentLine % consoleState.maxScrollbackLengthInLines;
				// rain - sign extension caused the character to carry over
				// into the color info for high ascii chars; casting c to unsigned
				consoleState.text[ y * consoleState.textWidthInChars + consoleState.horizontalCharOffset ].ch = Q_UTF8_CodePoint( text );
				consoleState.text[ y * consoleState.textWidthInChars + consoleState.horizontalCharOffset ].ink = color;
				++consoleState.horizontalCharOffset;
				if ( wordLen > 0 )
				{
					--wordLen;
				}

				if ( consoleState.horizontalCharOffset >= consoleState.textWidthInChars )
				{
					Con_Linefeed();
				}

				break;
		}

		text += Q_UTF8_Width( text );
	}

	return true;
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
Con_DrawBackground

Draws the background of the console (on the virtual 640x480 resolution)
================
*/
void Con_DrawBackground()
{
	vec4_t color;
	const int consoleWidth = cls.glconfig.vidWidth - 2 * consoleState.margin.sides;

	// draw the background
	color[ 0 ] = con_colorRed->value;
	color[ 1 ] = con_colorGreen->value;
	color[ 2 ] = con_colorBlue->value;
	color[ 3 ] = con_colorAlpha->value * consoleState.currentAlphaFactor;

	SCR_FillRect( consoleState.margin.sides, consoleState.margin.top, consoleWidth, consoleState.height, color );

	// draw the backgrounds borders
	color[ 0 ] = con_borderColorRed->value;
	color[ 1 ] = con_borderColorGreen->value;
	color[ 2 ] = con_borderColorBlue->value;
	color[ 3 ] = con_borderColorAlpha->value * consoleState.currentAlphaFactor;

	if ( con_margin->integer )
	{
		//top border
		SCR_FillRect( consoleState.margin.sides - consoleState.border.sides,
		              consoleState.margin.top - consoleState.border.top,
		              consoleWidth + consoleState.border.sides, consoleState.border.top, color );
		//left border
		SCR_FillRect( consoleState.margin.sides - consoleState.border.sides, consoleState.margin.top,
		              consoleState.border.sides, consoleState.height + consoleState.border.bottom, color );

		//right border
		SCR_FillRect( cls.glconfig.vidWidth - consoleState.margin.sides, consoleState.margin.top - consoleState.border.top,
		              consoleState.border.sides, consoleState.border.top + consoleState.height, color );

		//bottom border
		SCR_FillRect( consoleState.margin.sides, consoleState.height + consoleState.margin.top + consoleState.border.top - consoleState.border.bottom,
		              consoleWidth + consoleState.border.sides, consoleState.border.bottom, color );
	}
	else
	{
		//bottom border
		SCR_FillRect( 0, consoleState.height, consoleWidth, consoleState.border.bottom, color );
	}
}

/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput( int linePosition, float overrideAlpha )
{
	char    prompt[ MAX_STRING_CHARS ];
	vec4_t  color;
	qtime_t realtime;

	Com_RealTime( &realtime );
	Com_sprintf( prompt,  sizeof( prompt ), "^0[^3%02d%c%02d^0]^7 %s", realtime.tm_hour, ( realtime.tm_sec & 1 ) ? ':' : ' ', realtime.tm_min, con_prompt->string );

	color[ 0 ] = 1.0f;
	color[ 1 ] = 1.0f;
	color[ 2 ] = 1.0f;
	color[ 3 ] = consoleState.currentAlphaFactor * overrideAlpha;

	SCR_DrawSmallStringExt( consoleState.margin.sides + consoleState.padding.sides, linePosition, prompt, color, false, false );

	Q_CleanStr( prompt );
	Field_Draw( g_consoleField, consoleState.margin.sides + consoleState.padding.sides + SCR_ConsoleFontStringWidth( prompt, strlen( prompt ) ), linePosition, true, true, color[ 3 ] );
}

void Con_DrawRightFloatingTextLine( const int linePosition, const float *color, const char* text )
{
	int i, x;
	float currentWidthLocation = 0;

	const int charHeight = SCR_ConsoleFontCharHeight();
	const int positionFromTop = consoleState.margin.top
	                          + consoleState.border.top
	                          + consoleState.padding.top
	                          + charHeight;

	i = strlen( text );
	currentWidthLocation = cls.glconfig.vidWidth
	                     - SCR_ConsoleFontStringWidth( text, i )
	                     - consoleState.margin.sides - consoleState.padding.sides;

	re.SetColor( color );

	for ( x = 0; x < i; x++ )
	{
		int ch = Q_UTF8_CodePoint( &text[ x ] );
		SCR_DrawConsoleFontUnichar( currentWidthLocation, positionFromTop + ( linePosition * charHeight ), ch );
		currentWidthLocation += SCR_ConsoleFontUnicharWidth( ch );
	}
}

/*
================
Con_DrawAboutText

Draws the build and copyright info onto the console
================
*/
void Con_DrawAboutText()
{
	vec4_t color;

	// draw the version number
	color[ 0 ] = 1.0f;
	color[ 1 ] = 1.0f;
	color[ 2 ] = 1.0f;
	//ANIMATION_TYPE_FADE but also ANIMATION_TYPE_SCROLL_DOWN needs this, latter, since it might otherwise scroll out the console
	color[ 3 ] = 0.66f * consoleState.currentAnimationFraction;

	Con_DrawRightFloatingTextLine( 0, color, Q3_VERSION );
	Con_DrawRightFloatingTextLine( 1, color, Q3_ENGINE );
}

/*
================
Con_DrawConsoleScrollbackIndicator
================
*/
void Con_DrawConsoleScrollbackIndicator( int lineDrawPosition )
{
	int i;
	vec4_t color;
	// draw arrows to show the buffer is backscrolled
	const int hatWidth = SCR_ConsoleFontUnicharWidth( '^' );

	color[ 0 ] = 1.0f;
	color[ 1 ] = 1.0f;
	color[ 2 ] = 1.0f;
	color[ 3 ] = 0.66f * consoleState.currentAlphaFactor;
	re.SetColor( color );

	for ( i = 0; i < consoleState.textWidthInChars; i += 4 )
	{
		SCR_DrawConsoleFontUnichar( consoleState.margin.sides + consoleState.padding.sides + ( i + 1.5 ) * hatWidth, lineDrawPosition, '^' );
	}
}

void Con_DrawConsoleScrollbar()
{
	vec4_t color;
	const int	freeConsoleHeight = consoleState.height - consoleState.padding.top - consoleState.padding.bottom;
	const float scrollBarX = cls.glconfig.vidWidth - consoleState.margin.sides - consoleState.padding.sides - 2 * consoleState.border.sides;
	const float scrollBarY = consoleState.margin.top + consoleState.border.top + consoleState.padding.top + freeConsoleHeight * 0.10f;
	const float scrollBarLength = freeConsoleHeight * 0.80f;
	const float scrollBarWidth = consoleState.border.sides * 2;

	const float scrollHandleLength = consoleState.usedScrollbackLengthInLines
	                                 ? scrollBarLength * std::min( 1.0f, (float) consoleState.visibleAmountOfLines / consoleState.usedScrollbackLengthInLines )
	                                 : 0;

	const float scrollBarLengthPerLine = ( scrollBarLength - scrollHandleLength ) / ( consoleState.usedScrollbackLengthInLines - consoleState.visibleAmountOfLines );
	// that may result in -NaN

	const float relativeScrollLineIndex = consoleState.currentLine - consoleState.usedScrollbackLengthInLines
				+ std::min(consoleState.visibleAmountOfLines, consoleState.usedScrollbackLengthInLines);

	const float scrollHandlePostition = ( scrollBarLengthPerLine == scrollBarLengthPerLine )
	                                  ? scrollBarLengthPerLine * ( consoleState.bottomDisplayedLine - relativeScrollLineIndex )
	                                  : 0; // we may get this: +/- NaN is never equal to itself

	//draw the scrollBar
	color[ 0 ] = 0.2f;
	color[ 1 ] = 0.2f;
	color[ 2 ] = 0.2f;
	color[ 3 ] = 0.75f * consoleState.currentAlphaFactor;

	SCR_FillRect( scrollBarX, scrollBarY, scrollBarWidth, scrollBarLength, color );

	//draw the handle
	if ( scrollHandlePostition >= 0 && scrollHandleLength > 0 )
	{
		color[ 0 ] = 0.5f;
		color[ 1 ] = 0.5f;
		color[ 2 ] = 0.5f;
		color[ 3 ] = consoleState.currentAlphaFactor;

		SCR_FillRect( scrollBarX, scrollBarY + scrollHandlePostition, scrollBarWidth, scrollHandleLength, color );
	}
	else if ( consoleState.usedScrollbackLengthInLines ) //this happens when line appending gets us over the top position in a roll-lock situation (scrolling itself won't do that)
	{
		color[ 0 ] = (-scrollHandlePostition * 5.0f)/10;
		color[ 1 ] = 0.5f;
		color[ 2 ] = 0.5f;
		color[ 3 ] = consoleState.currentAlphaFactor;

		SCR_FillRect( scrollBarX, scrollBarY, scrollBarWidth, scrollHandleLength, color );
	}

	if(con_debug->integer) {
		Con_DrawRightFloatingTextLine( 6, nullptr, va( "Scrollbar (px): Size %d HandleSize %d Position %d", (int) scrollBarLength, (int) scrollHandleLength, (int) scrollHandlePostition ) );
	}
}

void Con_DrawScrollbackMarkerline( int lineDrawPosition )
{
	vec4_t color;

	//to not run into the scrollbar
	const int scrollBarImpliedPadding = 2 * consoleState.padding.sides + 2 * consoleState.border.sides;

	color[ 0 ] = 0.8f;
	color[ 1 ] = 0.0f;
	color[ 2 ] = 0.0f;
	color[ 3 ] = 0.5f * consoleState.currentAlphaFactor;

	SCR_FillRect(	consoleState.margin.sides + consoleState.border.sides + consoleState.padding.sides/2,
					lineDrawPosition + SCR_ConsoleFontCharVPadding(),
					cls.glconfig.vidWidth - 2 * consoleState.margin.sides - 2 * consoleState.border.sides - consoleState.padding.sides/2 - scrollBarImpliedPadding,
					1, color );
}

/*
================
Con_MarginFadeAlpha
================
*/
static float Con_MarginFadeAlpha( float alpha, float lineDrawPosition, int topMargin, int bottomMargin, int charHeight )
{
	if ( lineDrawPosition > bottomMargin && lineDrawPosition <= bottomMargin + charHeight )
	{
		return alpha * (bottomMargin + charHeight - lineDrawPosition) / (float) charHeight;
	}

	if ( lineDrawPosition < topMargin || lineDrawPosition >= topMargin + charHeight )
	{
		return alpha;
	}

	return alpha * (lineDrawPosition - topMargin) / (float) charHeight;
}


/*
================
Con_DrawConsoleContent
================
*/
void Con_DrawConsoleContent()
{
	float  currentWidthLocation = 0;
	int    x;
	float  lineDrawPosition, lineDrawLowestPosition;
	int    row;
	int    currentColor;
	vec4_t color;

	const int charHeight = SCR_ConsoleFontCharHeight();
	const int charPadding = SCR_ConsoleFontCharVPadding();
	const int textDistanceToTop = consoleState.margin.top
	                            + consoleState.padding.top
	                            + consoleState.border.top
	                            - charPadding - 1;

	// draw from the bottom up
	lineDrawPosition = consoleState.height
	                 + consoleState.margin.top
	                 - consoleState.padding.bottom
	                 - consoleState.border.top
	                 - charPadding - 1;

	if (lineDrawPosition <= textDistanceToTop)
	{
		return;
	}

	// draw the input prompt, user text, and cursor if desired
	// moved back here (have observed render issues to do with time taken)
	Con_DrawInput( lineDrawPosition, Con_MarginFadeAlpha( 1, lineDrawPosition, textDistanceToTop, lineDrawPosition, charHeight ) );
	lineDrawPosition -= charHeight;

	if (lineDrawPosition <= textDistanceToTop)
	{
		return;
	}

	if(con_debug->integer) {
		Con_DrawRightFloatingTextLine( 3, nullptr, va( "Buffer (lines): ScrollbackLength %d/%d  CurrentIndex %d", consoleState.usedScrollbackLengthInLines, consoleState.maxScrollbackLengthInLines, consoleState.currentLine) );
		Con_DrawRightFloatingTextLine( 4, nullptr, va( "Display (lines): From %d to %d (%d a %i px)", consoleState.currentLine-consoleState.maxScrollbackLengthInLines, consoleState.scrollLineIndex, consoleState.visibleAmountOfLines, charHeight ) );
	}

	/*
	 * if we scrolled back, give feedback,
	 * unless it's the last line (which will be rendered partly transparent anyway)
	 * so that we dont indicate scrollback each time a single line gets added
	 */
	if ( floor( consoleState.bottomDisplayedLine ) < consoleState.currentLine - 1 )
	{
		// draw arrows to show the buffer is backscrolled
		Con_DrawConsoleScrollbackIndicator( lineDrawPosition );
	}

	lineDrawPosition -= charHeight;

	row = consoleState.bottomDisplayedLine;

	if ( consoleState.horizontalCharOffset == 0 )
	{
		row--;
	}

	lineDrawLowestPosition = lineDrawPosition;

	if ( consoleState.bottomDisplayedLine - floor( consoleState.bottomDisplayedLine ) != 0.0f )
	{
		lineDrawPosition += charHeight - ( consoleState.bottomDisplayedLine - floor( consoleState.bottomDisplayedLine ) ) * charHeight;
		++row;
	}

	currentColor = 7;
	color[ 0 ] = g_color_table[ currentColor ][ 0 ];
	color[ 1 ] = g_color_table[ currentColor ][ 1 ];
	color[ 2 ] = g_color_table[ currentColor ][ 2 ];

	for ( ; row >= 0 && lineDrawPosition > textDistanceToTop; lineDrawPosition -= charHeight, row-- )
	{
		conChar_t *text;

		if ( consoleState.currentLine - row >= consoleState.maxScrollbackLengthInLines )
		{
			// past scrollback wrap point
			continue;
		}

		if ( row == consoleState.lastReadLineIndex - 1
			&& consoleState.lastReadLineIndex != consoleState.currentLine
			&& consoleState.currentLine - consoleState.lastReadLineIndex < consoleState.usedScrollbackLengthInLines)
		{
			Con_DrawScrollbackMarkerline( lineDrawPosition );
		}

		text = consoleState.text + CON_LINE( row );

		currentWidthLocation = consoleState.margin.sides + consoleState.padding.sides;

		for ( x = 0; x < consoleState.textWidthInChars && text[x].ch; ++x )
		{
			if ( text[ x ].ink != currentColor )
			{
				currentColor = text[ x ].ink;
				color[ 0 ] = g_color_table[ currentColor ][ 0 ];
				color[ 1 ] = g_color_table[ currentColor ][ 1 ];
				color[ 2 ] = g_color_table[ currentColor ][ 2 ];
			}

			color[ 3 ] = Con_MarginFadeAlpha( consoleState.currentAlphaFactor, lineDrawPosition, textDistanceToTop, lineDrawLowestPosition, charHeight );
			re.SetColor( color );

			SCR_DrawConsoleFontUnichar( currentWidthLocation, floor( lineDrawPosition + 0.5 ), text[ x ].ch );
			currentWidthLocation += SCR_ConsoleFontUnicharWidth( text[ x ].ch );
		}
	}

	Con_DrawConsoleScrollbar( );

	re.SetColor( nullptr ); //set back to white
}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawAnimatedConsole()
{
	vec4_t contentClipping;

	Con_DrawBackground( );

	//clip about text and content to the console
	contentClipping [ 0 ] = consoleState.margin.sides + consoleState.border.sides; //x
	contentClipping [ 1 ] = consoleState.margin.top + consoleState.border.top; //y
	contentClipping [ 2 ] = cls.glconfig.vidWidth - consoleState.margin.sides - consoleState.border.sides; //x-end
	contentClipping [ 3 ] = consoleState.margin.top + consoleState.border.top + consoleState.height ; //y-end
	re.SetClipRegion( contentClipping );


	//build info, projectname/copyrights, meta informatin or similar
	Con_DrawAboutText();

	if(con_debug->integer) {
			Con_DrawRightFloatingTextLine( 8, nullptr, va( "Animation: target %d current fraction %f alpha %f", (int) consoleState.isOpened, consoleState.currentAnimationFraction, consoleState.currentAlphaFactor) );
	}

	//input, scrollbackindicator, scrollback text
	Con_DrawConsoleContent( );

	re.SetClipRegion( nullptr ); //unclip
}

/*
==================
Con_UpdateConsoleState
updates the consoleState
==================
*/
void Con_UpdateConsoleState()
{
	float  horizontalMargin, verticalMargin;
	int    totalVerticalPadding;

	const int charHeight = SCR_ConsoleFontCharHeight();
	const int charPadding = SCR_ConsoleFontCharVPadding();

	/*
	 * calculate margin and border
	 * we will treat the border in pixel (as opposed to margins and paddings)
	 * to allow for nice looking 1px borders, as well as to prevent
	 * different widths for horizontal and vertical borders due to different resolution-ratios
	 * since that isn't as nice looking as with areas
	 */
	consoleState.border.bottom = std::max( 0, con_borderWidth->integer );

	if(con_margin->value > 0) {
		horizontalMargin = con_margin->value;
		verticalMargin = con_margin->value;
		consoleState.border.sides = consoleState.border.bottom;
		consoleState.border.top = consoleState.border.bottom;
	} else {
		horizontalMargin = - con_margin->value;
		verticalMargin = 0;
		consoleState.border.sides = 0;
		consoleState.border.top = 0;
	}

	SCR_AdjustFrom640( &horizontalMargin, &verticalMargin, nullptr, nullptr );

	consoleState.margin.top = verticalMargin;
	consoleState.margin.bottom = verticalMargin;
	consoleState.margin.sides = horizontalMargin;

	/*
	 * calculate padding
	 */
	consoleState.padding.top = floor( verticalMargin * 0.3f );
	consoleState.padding.bottom = std::max( 3, consoleState.padding.top );

	// on wide screens, this will lead to somewhat of a centering of the text
	if(con_horizontalPadding->integer)
	{
		float horizontalVidPadding = con_horizontalPadding->value;
		SCR_AdjustFrom640( &horizontalVidPadding, nullptr, nullptr, nullptr );
		consoleState.padding.sides = horizontalVidPadding;
	}
	else
	{
		consoleState.padding.sides = floor( horizontalMargin * 0.3f );
	}

	/*
	 * calculate global alpha factor
	 * apply the fade animation if the type is set, otherwise remain completly visible
	 */
	consoleState.currentAlphaFactor = ( con_animationType->integer & ANIMATION_TYPE_FADE ) ? consoleState.currentAnimationFraction : 1.0f;

	/*
	 * calculate current console height
	 */
	consoleState.height = con_height->integer * 0.01 * (cls.glconfig.vidHeight
						- consoleState.margin.top - consoleState.margin.bottom
						- consoleState.border.top - consoleState.border.bottom
						);

	totalVerticalPadding = consoleState.padding.top + consoleState.padding.bottom;

	// clip to a multiple of the character height, plus padding
	consoleState.height -= ( consoleState.height - totalVerticalPadding - charPadding ) % charHeight;
	// ... and ensure that at least three lines are visible
	consoleState.height = std::max( 3 * charHeight + totalVerticalPadding, consoleState.height );


	//animate via scroll animation if the type is set
	if ( con_animationType->integer & ANIMATION_TYPE_SCROLL_DOWN)
	{
		consoleState.height *= consoleState.currentAnimationFraction;
	}

	if ( consoleState.height > cls.glconfig.vidHeight )
	{
		consoleState.height = cls.glconfig.vidHeight;
	}

	/*
	 * calculate current amount of visible lines after we learned about the current height
	 */

	consoleState.visibleAmountOfLines = ( consoleState.height - consoleState.padding.top - consoleState.padding.bottom )
	                                    / charHeight //rowheight in pixel -> amount of rows
	                                    - 2 ; // dont count the input and the scrollbackindicator

}

/*
==================
Con_RunAnimatedConsole
runs each render-frame to update the console state accordingly
==================
*/
void Con_RunAnimatedConsole()
{
	int consoleVidWidth;

	if (con_height->value > 100.0f || con_height->value < 1.0f )
	{
		Cvar_Reset(con_height->name);
	}
	if (con_animationSpeed->value <= 0.0f)
	{
		Cvar_Reset(con_animationSpeed->name);
	}

	Con_UpdateConsoleState( );

	//now check everything that is depending on the consolestate
	if (con_height->value < con_margin->value || ( consoleState.visibleAmountOfLines < 1 && consoleState.currentAnimationFraction == 1.0f ) )
	{
		Cvar_Reset(con_height->name);
		Cvar_Reset(con_margin->name);
		Con_UpdateConsoleState( ); //recalculate
	}

	consoleVidWidth = cls.glconfig.vidWidth - 2 * (consoleState.margin.sides + consoleState.padding.sides );

	if( 2 * con_horizontalPadding->value >= consoleVidWidth )
	{
		Cvar_Reset(con_horizontalPadding->name);

		//to be sure, its not the caus of this happening and resulting in a loop
		Cvar_Reset(con_borderWidth->name);
		Cvar_Reset(con_margin->name);
		Con_UpdateConsoleState( );  //recalculate
	}

	// check for console width changes from a vid mode change
	Con_CheckResize( );
}


/*
==================
Con_DrawConsole
runs each render-frame
==================
*/
void Con_DrawConsole()
{
	// render console only if flag is set or is within an animation but also in special disconnected states
	if ( !consoleState.isOpened && consoleState.currentAnimationFraction <= 0
		&& !( cls.state == CA_DISCONNECTED && !( cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) ) ) )
		return;


	Con_RunAnimatedConsole( );
	Con_DrawAnimatedConsole( );
}

//================================================================

/*
==================
Con_RunConsole

runs each frame once independend wheter or not the console is going to be rendered or not
==================
*/

static std::vector<std::string> cl_consoleBufferedLines;
static std::mutex cl_bufferedLinesLock;

void Con_RunConsole()
{
	//check whether or not the console should be in opened state
	consoleState.isOpened = cls.keyCatchers & KEYCATCH_CONSOLE;

	if ( !consoleState.isOpened && consoleState.currentAnimationFraction >= 0.0f )
	{
		consoleState.currentAnimationFraction -= con_animationSpeed->value * cls.realFrametime * 0.001;

		if ( consoleState.currentAnimationFraction <= 0.0f  || con_animationType->integer == ANIMATION_TYPE_NONE )
		{	//we are closed, do some last onClose work
			consoleState.currentAnimationFraction = 0.0f;
			consoleState.lastReadLineIndex = consoleState.currentLine;
		}
	}
	else if ( consoleState.isOpened && consoleState.currentAnimationFraction <= 1.0f)
	{
		consoleState.currentAnimationFraction += con_animationSpeed->value * cls.realFrametime * 0.001;

		if ( consoleState.currentAnimationFraction > 1.0f  || con_animationType->integer == ANIMATION_TYPE_NONE  )
		{
			consoleState.currentAnimationFraction = 1.0f;
		}
	}

	if(consoleState.currentAnimationFraction > 0)
	{
		const float scrollDifference = std::max( 0.5f, fabsf( consoleState.bottomDisplayedLine - consoleState.scrollLineIndex ) );
		if( consoleState.bottomDisplayedLine < consoleState.scrollLineIndex )
		{
			consoleState.bottomDisplayedLine += con_animationSpeed->value * cls.realFrametime * 0.005 * scrollDifference;
			if( consoleState.bottomDisplayedLine > consoleState.scrollLineIndex || con_animationType->integer == ANIMATION_TYPE_NONE )
			{
				consoleState.bottomDisplayedLine = consoleState.scrollLineIndex;
			}
		}
		else if ( consoleState.bottomDisplayedLine > consoleState.scrollLineIndex )
		{
			consoleState.bottomDisplayedLine -= con_animationSpeed->value * cls.realFrametime * 0.005 * scrollDifference;
			if( consoleState.bottomDisplayedLine < consoleState.scrollLineIndex || con_animationType->integer == ANIMATION_TYPE_NONE )
			{
				consoleState.bottomDisplayedLine = consoleState.scrollLineIndex;
			}
		}
	} else {
		/*
		 * if we are not visible, its better to do an instant update
		 * skipping it entirely would lead to retrospective/delayed scrolling on opening
		 */
		consoleState.bottomDisplayedLine = consoleState.scrollLineIndex;
	}

	std::vector<std::string> lines;
	{
		std::lock_guard<std::mutex> guard(cl_bufferedLinesLock);
		lines = std::move(cl_consoleBufferedLines);
	}
	for (const auto& line : lines) {
		CL_InternalConsolePrint( line.c_str() );
	}
}

// Definition of the graphical console log target
// TODO split with the HUD from it

void CL_ConsolePrint(std::string text) {
	std::lock_guard<std::mutex> guard(cl_bufferedLinesLock);
	cl_consoleBufferedLines.push_back(std::move(text));
}

class GraphicalTarget : public Log::Target {
	public:
		GraphicalTarget() {
			this->Register(Log::GRAPHICAL_CONSOLE);
		}

		virtual bool Process(const std::vector<Log::Event>& events) OVERRIDE {
			// for some demos we don't want to ever show anything on the console
			// flush the buffer
			if ( cl_noprint && cl_noprint->integer )
			{
				return true;
			}

			//Video hasn't been initialized
			if ( ! cls.glconfig.vidWidth ) {
				return false;
			}

			for (const auto& event : events) {
				CL_ConsolePrint(event.text);
				CL_ConsolePrint("\n");
			}
			return true;
		}
};

static GraphicalTarget gui;


void Con_PageUp()
{
	Con_ScrollUp( consoleState.visibleAmountOfLines/2 );
}

void Con_PageDown()
{
	Con_ScrollDown( consoleState.visibleAmountOfLines/2 );
}

void Con_ScrollUp( int lines )
{
	//do not scroll if there isn't enough to scroll
	if(consoleState.usedScrollbackLengthInLines < consoleState.visibleAmountOfLines)
		return;

	consoleState.scrollLineIndex -= lines;

	if ( consoleState.scrollLineIndex < consoleState.currentLine - consoleState.usedScrollbackLengthInLines
			+ std::min(consoleState.visibleAmountOfLines, consoleState.usedScrollbackLengthInLines) )
	{
		Con_ScrollToTop( );
	}
}

void Con_ScrollDown( int lines )
{
	consoleState.scrollLineIndex += lines;

	if ( consoleState.scrollLineIndex > consoleState.currentLine )
	{
		consoleState.scrollLineIndex = consoleState.currentLine;
	}
}

void Con_ScrollToMarkerLine()
{
	consoleState.scrollLineIndex = std::max(consoleState.lastReadLineIndex, consoleState.currentLine - consoleState.usedScrollbackLengthInLines)
			+ std::min(consoleState.visibleAmountOfLines, consoleState.usedScrollbackLengthInLines);
	//consoleState.bottomDisplayedLine = consoleState.scrollLineIndex;
}

void Con_ScrollToTop()
{
	consoleState.scrollLineIndex = consoleState.currentLine
			- consoleState.usedScrollbackLengthInLines
			+ std::min(consoleState.visibleAmountOfLines, consoleState.usedScrollbackLengthInLines);
	//consoleState.bottomDisplayedLine = consoleState.scrollLineIndex;
}

void Con_ScrollToBottom()
{
	consoleState.scrollLineIndex = consoleState.currentLine;
	//consoleState.bottomDisplayedLine = consoleState.currentLine;
}

void Con_JumpUp()
{
	if ( consoleState.lastReadLineIndex &&
		 consoleState.scrollLineIndex > consoleState.lastReadLineIndex + std::min(consoleState.visibleAmountOfLines, consoleState.usedScrollbackLengthInLines)
	   //&& consoleState.currentLine - consoleState.lastReadLineIndex > consoleState.visibleAmountOfLines
	   )
	{
		Con_ScrollToMarkerLine( );
	}
	else
	{
		Con_ScrollToTop( );
	}

}

void Con_Close()
{
	if ( !com_cl_running->integer )
	{
		return;
	}

	g_consoleField.Clear();
	cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	consoleState.isOpened = false;

	//instant disappearance, if we need it for situations where this is not called by the user
	consoleState.currentAnimationFraction = 0;
}
