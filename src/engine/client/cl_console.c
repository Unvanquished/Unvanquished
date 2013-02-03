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

console_t consoleState;

cvar_t    *con_animationSpeed;
cvar_t    *con_animationType;
cvar_t    *con_notifytime;
cvar_t    *con_autoclear;

cvar_t	  *con_prompt;

cvar_t    *con_margin;

cvar_t    *con_borderWidth;
cvar_t    *con_borderColorAlpha;
cvar_t    *con_borderColorRed;
cvar_t    *con_borderColorBlue;
cvar_t    *con_borderColorGreen;

cvar_t    *con_horizontalPadding;

cvar_t    *con_height;
cvar_t    *con_colorAlpha;
cvar_t    *con_colorRed;
cvar_t    *con_colorBlue;
cvar_t    *con_colorGreen;

#define ANIMATION_TYPE_NONE   0
#define ANIMATION_TYPE_SCROLL_DOWN 1
#define ANIMATION_TYPE_FADE   2
#define ANIMATION_TYPE_BOTH   3

#define DEFAULT_CONSOLE_WIDTH 78
#define MAX_CONSOLE_WIDTH   1024

#define CON_LINE(line) ( ( (line) % consoleState.scrollbackLengthInLines ) * consoleState.textWidthInChars )

// Buffer used by line-to-string code. Implementation detail.
static char lineString[ MAX_CONSOLE_WIDTH * 6 + 4 ];

static const char *Con_LineToString( int lineno, qboolean lf )
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
	// ydnar: persistent console input is more useful
	if ( con_autoclear->integer )
	{
		Field_Clear( &g_consoleField );
	}

	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify();

	if (consoleState.isOpened) {
		cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	} else {
		cls.keyCatchers |= KEYCATCH_CONSOLE;
	}
}

void Con_OpenConsole_f( void )
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
static INLINE void Con_Clear( void )
{
	int i;
	conChar_t fill = { '\0', ColorIndex( CONSOLE_COLOR ) };

	for ( i = 0; i < CON_TEXTSIZE; ++i )
	{
		consoleState.text[i] = fill;
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
	Con_ScrollToBottom(); // go to end
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
	for ( l = consoleState.currentLine - consoleState.scrollbackLengthInLines + 1; l <= consoleState.currentLine; l++ )
	{
		if ( consoleState.text[ CON_LINE( l ) ].ch )
		{
			break;
		}
	}

	// write the remaining lines
	for ( ; l <= consoleState.currentLine; l++ )
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
	for ( l = consoleState.bottomDisplayedLine - 1 + direction; l <= consoleState.currentLine && consoleState.currentLine - l < consoleState.scrollbackLengthInLines; l += direction )
	{
		const char *buffer = Con_LineToString( l, qtrue );

		// Don't search commands
		for ( i = 1; i < c; i++ )
		{
			if ( Q_stristr( buffer, Cmd_Argv( i ) ) )
			{
				consoleState.bottomDisplayedLine = l + 1;

				if ( consoleState.bottomDisplayedLine > consoleState.currentLine )
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
	for ( l = consoleState.currentLine - consoleState.scrollbackLengthInLines + 1; l <= consoleState.currentLine; l++ )
	{
		if ( consoleState.text[ CON_LINE( l ) ].ch )
		{
			break;
		}
	}

	// check the remaining lines
	search = Cmd_Argv( 1 );
	lastcolor = 7;

	for ( ; l <= consoleState.currentLine; l++ )
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
		consoleState.times[ i ] = 0;
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
	int   i, textWidthInChars, oldwidth, oldtotallines, numlines, numchars;
	conChar_t buf[ CON_TEXTSIZE ];

	if ( cls.glconfig.vidWidth )
	{
		const int consoleVidWidth = cls.glconfig.vidWidth - 2 * (consoleState.horizontalVidMargin + consoleState.horizontalVidPadding );
		textWidthInChars = consoleVidWidth / SCR_ConsoleFontUnicharWidth( 'W' );

		if( 2 * con_horizontalPadding->value >= consoleVidWidth )
		{
			Cvar_Reset(con_horizontalPadding->name);

			//to be sure, its not the caus of this happening and resulting in a loop
			Cvar_Reset(con_borderWidth->name);
			Cvar_Reset(con_margin->name);
		}

		if (con_height->value > 100.0f || con_height->value < 1.0f )
		{
			Cvar_Reset(con_height->name);
		}

		if (con_height->value < con_margin->value || consoleState.visibleAmountOfLines < 1)
		{
			Cvar_Reset(con_height->name);
			Cvar_Reset(con_margin->name);
		}

		if (con_animationSpeed->value <= 0.0f)
		{
			Cvar_Reset(con_animationSpeed->name);
		}
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
		consoleState.scrollbackLengthInLines = CON_TEXTSIZE / consoleState.textWidthInChars;
		Con_Clear();

		consoleState.currentLine = consoleState.scrollbackLengthInLines - 1;
		consoleState.bottomDisplayedLine = consoleState.currentLine;
	}
	else
	{
		oldwidth = consoleState.textWidthInChars;
		consoleState.textWidthInChars = textWidthInChars;
		oldtotallines = consoleState.scrollbackLengthInLines;
		consoleState.scrollbackLengthInLines = CON_TEXTSIZE / consoleState.textWidthInChars;
		numlines = oldtotallines;

		if ( consoleState.scrollbackLengthInLines < numlines )
		{
			numlines = consoleState.scrollbackLengthInLines;
		}

		numchars = oldwidth;

		if ( consoleState.textWidthInChars < numchars )
		{
			numchars = consoleState.textWidthInChars;
		}

		Com_Memcpy( buf, consoleState.text, sizeof( consoleState.text ) );
		Con_Clear();

		for ( i = 0; i < numlines; i++ )
		{
			memcpy( consoleState.text + ( consoleState.scrollbackLengthInLines - 1 - i ) * consoleState.textWidthInChars,
			        buf + ( ( consoleState.currentLine - i + oldtotallines ) % oldtotallines ) * oldwidth,
			        numchars * sizeof( conChar_t ) );
		}

		consoleState.currentLine = consoleState.scrollbackLengthInLines - 1;
		consoleState.bottomDisplayedLine = consoleState.currentLine;
	}

	g_console_field_width = g_consoleField.widthInChars = consoleState.textWidthInChars - 8 - ( con_prompt ? Q_UTF8Strlen( con_prompt->string ) : 0 );
}

/*
================
Con_Init
================
*/
void Con_Init( void )
{
	con_notifytime = Cvar_Get( "con_notifytime", "7", 0 );  // JPW NERVE increased per id req for obits
	con_animationSpeed = Cvar_Get( "con_animationSpeed", "3", 0 );
	con_animationType = Cvar_Get( "con_animationType", "2", 0 );
	con_autoclear = Cvar_Get( "con_autoclear", "1", CVAR_ARCHIVE );

	con_prompt = Cvar_Get( "con_prompt", "^3->", CVAR_ARCHIVE );

	con_margin = Cvar_Get( "con_margin", "10", CVAR_ARCHIVE );

	con_height = Cvar_Get( "con_height", "55", CVAR_ARCHIVE );
	con_colorRed = Cvar_Get( "con_colorRed", "0", CVAR_ARCHIVE );
	con_colorBlue = Cvar_Get( "con_colorBlue", "0.3", CVAR_ARCHIVE );
	con_colorGreen = Cvar_Get( "con_colorGreen", "0.18", CVAR_ARCHIVE );
	con_colorAlpha = Cvar_Get( "con_colorAlpha", "0.5", CVAR_ARCHIVE );

	con_horizontalPadding = Cvar_Get( "con_horizontalPadding", "0", CVAR_ARCHIVE );

	con_borderWidth = Cvar_Get( "con_borderWidth", "1", CVAR_ARCHIVE );
	con_borderColorRed = Cvar_Get( "con_borderColorRed", "1", CVAR_ARCHIVE );
	con_borderColorBlue = Cvar_Get( "con_borderColorBlue", "1", CVAR_ARCHIVE );
	con_borderColorGreen = Cvar_Get( "con_borderColorGreen", "1", CVAR_ARCHIVE );
	con_borderColorAlpha = Cvar_Get( "con_borderColorAlpha", "0.2", CVAR_ARCHIVE );

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
	if ( consoleState.currentLine >= 0 )
	{
		consoleState.times[ consoleState.currentLine % NUM_CON_TIMES ] = skipnotify ? 0 : cls.realtime;
	}

	consoleState.x = 0;

	if ( consoleState.bottomDisplayedLine == consoleState.currentLine )
	{
		consoleState.bottomDisplayedLine++;
	}

	consoleState.currentLine++;

	line = consoleState.text + CON_LINE( consoleState.currentLine );

	for ( i = 0; i < consoleState.textWidthInChars; ++i )
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

	if ( !consoleState.initialized )
	{
		consoleState.textWidthInChars = -1;
		Con_CheckResize();
		consoleState.initialized = qtrue;
	}

	if ( !skipnotify && !consoleState.isOpened && strncmp( txt, "EXCL: ", 6 ) )
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
		for ( i = l = 0; l < consoleState.textWidthInChars; ++l )
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
		if ( l != consoleState.textWidthInChars && ( consoleState.x + l >= consoleState.textWidthInChars ) )
		{
			Con_Linefeed( skipnotify );
		}

		switch ( c )
		{
			case '\n':
				Con_Linefeed( skipnotify );
				break;

			case '\r':
				consoleState.x = 0;
				break;

			case Q_COLOR_ESCAPE:
				if ( txt[ 1 ] == Q_COLOR_ESCAPE )
				{
					++txt;
				}

			default: // display character and advance
				y = consoleState.currentLine % consoleState.scrollbackLengthInLines;
				// rain - sign extension caused the character to carry over
				// into the color info for high ascii chars; casting c to unsigned
				consoleState.text[ y * consoleState.textWidthInChars + consoleState.x ].ch = Q_UTF8CodePoint( txt );
				consoleState.text[ y * consoleState.textWidthInChars + consoleState.x ].ink = color;
				++consoleState.x;

				if ( consoleState.x >= consoleState.textWidthInChars )
				{
					Con_Linefeed( skipnotify );
					consoleState.x = 0;
				}

				break;
		}

		txt += Q_UTF8Width( txt );
	}

	// mark time for transparent overlay
	if ( consoleState.currentLine >= 0 )
	{
		// NERVE - SMF
		if ( skipnotify )
		{
			prev = consoleState.currentLine % NUM_CON_TIMES - 1;

			if ( prev < 0 )
			{
				prev = NUM_CON_TIMES - 1;
			}

			consoleState.times[ prev ] = 0;
		}
		else
		{
			// -NERVE - SMF
			consoleState.times[ consoleState.currentLine % NUM_CON_TIMES ] = cls.realtime;
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
Con_DrawBackground

Draws the background of the console (on the virtual 640x480 resolution)
================
*/
void Con_DrawBackground( int virtualHeight )
{
	vec4_t color;
	const int virtualMargin = MAX( 0, con_margin->integer );
	const int virtualConsoleWidth = SCREEN_WIDTH - (2 * virtualMargin);

	// draw the background
	color[ 0 ] = con_colorRed->value;
	color[ 1 ] = con_colorGreen->value;
	color[ 2 ] = con_colorBlue->value;
	color[ 3 ] = con_colorAlpha->value * consoleState.currentAlphaFactor;

	SCR_FillRect( virtualMargin, virtualMargin, virtualConsoleWidth, virtualHeight, color );

	// draw the backgrounds borders
	color[ 0 ] = con_borderColorRed->value;
	color[ 1 ] = con_borderColorGreen->value;
	color[ 2 ] = con_borderColorBlue->value;
	color[ 3 ] = con_borderColorAlpha->value * consoleState.currentAlphaFactor;

	if (virtualMargin)
	{
		SCR_FillRect( virtualMargin - consoleState.borderWidth, virtualMargin - consoleState.borderWidth,
		              virtualConsoleWidth + consoleState.borderWidth, consoleState.borderWidth, color );  //top
		SCR_FillRect( virtualMargin - consoleState.borderWidth, virtualMargin,
		              consoleState.borderWidth, virtualHeight + consoleState.borderWidth, color );  //left
		SCR_FillRect( SCREEN_WIDTH - virtualMargin, virtualMargin - consoleState.borderWidth,
		              consoleState.borderWidth, virtualHeight + consoleState.borderWidth, color );  //right
		SCR_FillRect( virtualMargin, virtualHeight + virtualMargin,
		              virtualConsoleWidth + consoleState.borderWidth, consoleState.borderWidth, color );  //bottom
	}
	else
	{
		SCR_FillRect( 0, virtualHeight, SCREEN_WIDTH, consoleState.borderWidth, color );
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

	SCR_DrawSmallStringExt( consoleState.horizontalVidMargin + consoleState.horizontalVidPadding, linePosition, prompt, color, qfalse, qfalse );

	Q_CleanStr( prompt );
	Field_Draw( &g_consoleField, consoleState.horizontalVidMargin + consoleState.horizontalVidPadding + SCR_ConsoleFontStringWidth( prompt, strlen( prompt ) ), linePosition, qtrue, qtrue, color[ 3 ] );
}

void Con_DrawAboutTextLine( const int positionFromTop, const char* text )
{
	int i, x;
	float currentWidthLocation = 0;
	const int charHeight = SCR_ConsoleFontCharHeight();

	i = strlen( text );
	currentWidthLocation = cls.glconfig.vidWidth
	                     - SCR_ConsoleFontStringWidth( text, i )
	                     - consoleState.horizontalVidMargin - consoleState.horizontalVidPadding;

	for ( x = 0; x < i; x++ )
	{
		int ch = Q_UTF8CodePoint( &text[ x ] );
		SCR_DrawConsoleFontUnichar( currentWidthLocation, positionFromTop, ch );
		currentWidthLocation += SCR_ConsoleFontUnicharWidth( ch );
	}
}

/*
================
Con_DrawAboutText

Draws the build and copyright info onto the console
================
*/
void Con_DrawAboutText( void )
{
	int i, x;
	vec4_t color;
	float currentWidthLocation = 0;

	const int charHeight = SCR_ConsoleFontCharHeight();
	const int positionFromTop = consoleState.verticalVidMargin
	                          + consoleState.verticalVidPaddingTop
	                          + consoleState.topBorderWidth
	                          + charHeight;

	// draw the version number
	color[ 0 ] = 1.0f;
	color[ 1 ] = 1.0f;
	color[ 2 ] = 1.0f;
	//ANIMATION_TYPE_FADE but also ANIMATION_TYPE_SCROLL_DOWN needs this, latter, since it might otherwise scroll out the console
	color[ 3 ] = 0.66f * consoleState.currentAnimationFraction;
	re.SetColor( color );

	Con_DrawAboutTextLine( positionFromTop, Q3_VERSION );
	Con_DrawAboutTextLine( positionFromTop + charHeight, Q3_ENGINE );
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
	const int charHeight = SCR_ConsoleFontCharHeight();

	const int virtualHeight = (SCREEN_HEIGHT - con_margin->integer) * con_height->integer * 0.01;
	const int scrollBarLength = (virtualHeight - 2 * charHeight);

	color[ 0 ] = 1.0f;
	color[ 1 ] = 1.0f;
	color[ 2 ] = 1.0f;
	color[ 3 ] = 0.66f * consoleState.currentAlphaFactor;
	re.SetColor( color );

	for ( i = 0; i < consoleState.textWidthInChars; i += 4 )
	{
		SCR_DrawConsoleFontUnichar( consoleState.horizontalVidMargin + consoleState.horizontalVidPadding + ( i + 1 ) * hatWidth, lineDrawPosition, '^' );
	}
}

/**
 * @param virtualHeight height in  640x480 virtual resolution
 */
void Con_DrawConsoleScrollbar( int virtualHeight )
{
	vec4_t color;

}

/*
================
Con_MarginFadeAlpha
================
*/
static float Con_MarginFadeAlpha( float alpha, int lineDrawPosition, int topMargin, int charHeight )
{
	if ( lineDrawPosition < topMargin || lineDrawPosition >= topMargin + charHeight )
	{
		return alpha;
	}

	return alpha * (float)( lineDrawPosition - topMargin ) / (float) charHeight;
}


/*
================
Con_DrawConsoleContent
================
*/
void Con_DrawConsoleContent( int currentConsoleVidHeight, int currentConsoleVirtualHeight )
{
	float  currentWidthLocation = 0;
	int    i, x, lineDrawPosition;
	int    row;
	int    currentColor;
	vec4_t color;

	const int charHeight = SCR_ConsoleFontCharHeight();
	const int charPadding = SCR_ConsoleFontCharVPadding();
	const int textDistanceToTop = consoleState.verticalVidMargin
	                            + consoleState.verticalVidPaddingTop
	                            + consoleState.topBorderWidth
	                            - charPadding - 1;

	// draw from the bottom up
	lineDrawPosition = currentConsoleVidHeight
	                 + consoleState.verticalVidMargin
	                 - consoleState.verticalVidPaddingBottom
	                 - consoleState.topBorderWidth
	                 - charPadding - 1;

	if (lineDrawPosition <= textDistanceToTop)
	{
		return;
	}

	// draw the input prompt, user text, and cursor if desired
	// moved back here (have observed render issues to do with time taken)
	Con_DrawInput( lineDrawPosition, Con_MarginFadeAlpha( 1, lineDrawPosition, textDistanceToTop, charHeight ) );
	lineDrawPosition -= charHeight;

	if (lineDrawPosition <= textDistanceToTop)
	{
		return;
	}

	// if we scrolled back, give feedback
	if ( consoleState.bottomDisplayedLine != consoleState.currentLine )
	{
		// draw arrows to show the buffer is backscrolled
		Con_DrawConsoleScrollbackIndicator( lineDrawPosition );
		lineDrawPosition -= charHeight;
		Con_DrawConsoleScrollbar( currentConsoleVirtualHeight );
	}

	row = consoleState.bottomDisplayedLine;

	if ( consoleState.x == 0 )
	{
		row--;
	}

	currentColor = 7;
	color[ 0 ] = g_color_table[ currentColor ][ 0 ];
	color[ 1 ] = g_color_table[ currentColor ][ 1 ];
	color[ 2 ] = g_color_table[ currentColor ][ 2 ];

	for ( ; row >= 0 && lineDrawPosition > textDistanceToTop; lineDrawPosition -= charHeight, row-- )
	{
		conChar_t *text;

		if ( consoleState.currentLine - row >= consoleState.scrollbackLengthInLines )
		{
			// past scrollback wrap point
			continue;
		}

		text = consoleState.text + CON_LINE( row );

		currentWidthLocation = consoleState.horizontalVidMargin + consoleState.horizontalVidPadding;

		for ( x = 0; x < consoleState.textWidthInChars && text[x].ch; ++x )
		{
			if ( text[ x ].ink != currentColor )
			{
				currentColor = text[ x ].ink;
				color[ 0 ] = g_color_table[ currentColor ][ 0 ];
				color[ 1 ] = g_color_table[ currentColor ][ 1 ];
				color[ 2 ] = g_color_table[ currentColor ][ 2 ];
			}

			color[ 3 ] = Con_MarginFadeAlpha( consoleState.currentAlphaFactor, lineDrawPosition, textDistanceToTop, charHeight );
			re.SetColor( color );

			SCR_DrawConsoleFontUnichar( currentWidthLocation, lineDrawPosition, text[ x ].ch );
			currentWidthLocation += SCR_ConsoleFontUnicharWidth( text[ x ].ch );
		}
	}

	re.SetColor( NULL );
}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawAnimatedConsole( void )
{
	float  vidXMargin, vidYMargin;
	int    animatedConsoleVidHeight;
	float  animatedConsoleVirtualHeight;
	int    animatedConsoleVerticalPaddingTotal;

	const int charHeight = SCR_ConsoleFontCharHeight();
	const int charPadding = SCR_ConsoleFontCharVPadding();

	if ( consoleState.currentAnimationFraction <= 0 )
	{
		return;
	}

	consoleState.borderWidth = MAX( 0, con_borderWidth->integer );

	if(con_margin->value > 0) {
		vidXMargin = con_margin->value;
		vidYMargin = con_margin->value;
		consoleState.topBorderWidth = consoleState.borderWidth;
	} else {
		vidXMargin = - con_margin->value;
		vidYMargin = 0;
		consoleState.topBorderWidth = 0;
	}
	SCR_AdjustFrom640( &vidXMargin, &vidYMargin, NULL, NULL );

	consoleState.verticalVidMargin = vidYMargin;
	consoleState.horizontalVidMargin = vidXMargin;
	consoleState.verticalVidPaddingTop = floor( vidYMargin * 0.3f );
	consoleState.verticalVidPaddingBottom = MAX( 3, consoleState.verticalVidPaddingTop );

	animatedConsoleVerticalPaddingTotal = consoleState.verticalVidPaddingTop + consoleState.verticalVidPaddingBottom;

	// on wide screens, this will lead to somewhat of a centering of the text
	if(con_horizontalPadding->integer)
	{
		float horizontalVidPadding = con_horizontalPadding->value;
		SCR_AdjustFrom640( &horizontalVidPadding, NULL, NULL, NULL );
		consoleState.horizontalVidPadding = horizontalVidPadding;
	}
	else
	{
		consoleState.horizontalVidPadding = floor( vidXMargin * 0.3f );
	}

	animatedConsoleVidHeight = ( cls.glconfig.vidHeight - 2 * consoleState.verticalVidMargin ) * con_height->integer * 0.01;
	// clip to a multiple of the character height, plus padding
	animatedConsoleVidHeight -= ( animatedConsoleVidHeight - animatedConsoleVerticalPaddingTotal - charPadding ) % charHeight;
	// ... and ensure that at least three lines are visible
	animatedConsoleVidHeight = MAX( 3 * charHeight + animatedConsoleVerticalPaddingTotal, animatedConsoleVidHeight );

	animatedConsoleVirtualHeight = animatedConsoleVidHeight * SCREEN_HEIGHT / cls.glconfig.vidHeight;

	//only do scroll animation if the type is set
	if ( con_animationType->integer & ANIMATION_TYPE_SCROLL_DOWN)
	{
		animatedConsoleVidHeight *= consoleState.currentAnimationFraction;
		animatedConsoleVirtualHeight *= consoleState.currentAnimationFraction;
	}

	if ( animatedConsoleVidHeight > cls.glconfig.vidHeight )
	{
		animatedConsoleVidHeight = cls.glconfig.vidHeight;
	}

	//only do fade animation if the type is set
	consoleState.currentAlphaFactor = ( con_animationType->integer & ANIMATION_TYPE_FADE ) ? consoleState.currentAnimationFraction : 1.0f;

	consoleState.visibleAmountOfLines = ( animatedConsoleVidHeight - animatedConsoleVerticalPaddingTotal )
	                                    / charHeight //rowheight in pixel -> amount of rows
	                                    - 1 ; // sine we work with points but use charHeight spaces

	//now do the actual drawing
	Con_DrawBackground( animatedConsoleVirtualHeight );

	//build info, projectname/copyrights, meta informatin or similar
	Con_DrawAboutText();

	//input, scrollbackindicator, scrollback text
	Con_DrawConsoleContent( animatedConsoleVidHeight, animatedConsoleVirtualHeight );
}

extern cvar_t *con_drawnotify;

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

	for ( i = consoleState.currentLine - NUM_CON_TIMES + 1; i <= consoleState.currentLine; i++ )
	{
		if ( i < 0 )
		{
			continue;
		}

		time = consoleState.times[ i % NUM_CON_TIMES ];

		if ( time == 0 )
		{
			continue;
		}

		time = cls.realtime - time;

		if ( time > con_notifytime->value * 1000 )
		{
			continue;
		}

		text = consoleState.text + CON_LINE( i );

		if ( cl.snap.ps.pm_type != PM_INTERMISSION && (cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME )) )
		{
			continue;
		}

		for ( x = 0; x < consoleState.textWidthInChars && text[ x ].ch; ++x )
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

			SCR_DrawSmallUnichar( consoleState.horizontalVidMargin + consoleState.horizontalVidPadding + ( x + 1 ) * SMALLCHAR_WIDTH, v, text[ x ].ch );
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
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void )
{
	// check for console width changes from a vid mode change
	Con_CheckResize();

	// render console if flag is set or is within an animation but also in special disconnected states
	if (( cls.state == CA_DISCONNECTED && !( cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) ) )
		|| consoleState.isOpened || consoleState.currentAnimationFraction > 0)
	{
		Con_DrawAnimatedConsole( );
	}
	// draw notify lines, but only if console isn't opened
	else if ( cls.state == CA_ACTIVE && con_drawnotify->integer )
	{
		Con_DrawNotify( );
	}
}

//================================================================

/*
==================
Con_RunConsole

Update the state each frame,
like scrolling it up or down, or setting the opening flag
==================
*/
void Con_RunConsole( void )
{
	//check whether or not the console should be in opened state
	consoleState.isOpened = cls.keyCatchers & KEYCATCH_CONSOLE;

	if ( consoleState.isOpened < consoleState.currentAnimationFraction )
	{
		consoleState.currentAnimationFraction -= con_animationSpeed->value * cls.realFrametime * 0.001;

		if ( consoleState.currentAnimationFraction < 0  || con_animationType->integer == ANIMATION_TYPE_NONE )
		{
			consoleState.currentAnimationFraction = 0;
		}
	}
	else if ( consoleState.isOpened > consoleState.currentAnimationFraction )
	{
		consoleState.currentAnimationFraction += con_animationSpeed->value * cls.realFrametime * 0.001;

		if ( consoleState.currentAnimationFraction > 1  || con_animationType->integer == ANIMATION_TYPE_NONE  )
		{
			consoleState.currentAnimationFraction = 1;
		}
	}
}

void Con_PageUp( void )
{
	consoleState.bottomDisplayedLine -= 2;

	if ( consoleState.currentLine - consoleState.bottomDisplayedLine >= consoleState.scrollbackLengthInLines )
	{
		consoleState.bottomDisplayedLine = consoleState.currentLine - consoleState.scrollbackLengthInLines + 1;
	}
}

void Con_PageDown( void )
{
	consoleState.bottomDisplayedLine += 2;

	if ( consoleState.bottomDisplayedLine > consoleState.currentLine )
	{
		consoleState.bottomDisplayedLine = consoleState.currentLine;
	}
}

void Con_ScrollToTop( void )
{
	consoleState.bottomDisplayedLine = consoleState.scrollbackLengthInLines;

	if ( consoleState.currentLine - consoleState.bottomDisplayedLine >= consoleState.scrollbackLengthInLines )
	{
		consoleState.bottomDisplayedLine = consoleState.currentLine - consoleState.scrollbackLengthInLines + 1;
	}
}

void Con_ScrollToBottom( void )
{
	consoleState.bottomDisplayedLine = consoleState.currentLine;
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
	consoleState.isOpened = qfalse;

	//instant disappearance, if we need it for situations where this is not called by the user
	consoleState.currentAnimationFraction = 0;
}
