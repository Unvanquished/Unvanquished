/*
===========================================================================
Copyright (C) 2008 Amanieu d'Antras

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

#ifdef USE_CURSES_W
#define _XOPEN_SOURCE 500
//#include <stdio.h>
#endif

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"


// curses.h defines COLOR_*, which are already defined in q_shared.h
#undef COLOR_BLACK
#undef COLOR_RED
#undef COLOR_GREEN
#undef COLOR_YELLOW
#undef COLOR_BLUE
#undef COLOR_MAGENTA
#undef COLOR_CYAN
#undef COLOR_WHITE

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <locale.h>
#ifndef _WIN32
#include <sys/ioctl.h>
#endif

#define _XOPEN_SOURCE_EXTENDED
#ifdef USE_CURSES_W
#include <wchar.h>
#endif
#include <curses.h>

#define TITLE         "^4---[ ^3" CLIENT_WINDOW_TITLE " Console ^4]---"
#define PROMPT        "^3-> "
#define INPUT_SCROLL  15
#define LOG_SCROLL    5
#define MAX_LOG_LINES 1024
#define LOG_BUF_SIZE  65536

// Functions from the tty console for fallback
void            CON_Shutdown_tty( void );
void            CON_Init_tty( void );
char            *CON_Input_tty( void );
void            CON_Print_tty( const char *message );
void            CON_Clear_tty( void );

static qboolean curses_on = qfalse;
static field_t  input_field;
static WINDOW   *borderwin;
static WINDOW   *logwin;
static WINDOW   *inputwin;
static WINDOW   *scrollwin;
static WINDOW   *clockwin;

static char     logbuf[ LOG_BUF_SIZE ];
static char     *insert = logbuf;
static int      scrollline = 0;
static int      lastline = 1;

// The special characters look good on the win32 console but suck on other consoles
#ifdef _WIN32
#define SCRLBAR_CURSOR ACS_BLOCK
#define SCRLBAR_LINE   ACS_VLINE
#define SCRLBAR_UP     ACS_UARROW
#define SCRLBAR_DOWN   ACS_DARROW
#else
#define SCRLBAR_CURSOR '#'
#define SCRLBAR_LINE   ACS_VLINE
#define SCRLBAR_UP     ACS_HLINE
#define SCRLBAR_DOWN   ACS_HLINE
#endif

#define LOG_LINES      ( LINES - 4 )
#define LOG_COLS       ( COLS - 3 )

/*
==================
CON_SetColor

Use grey instead of black
==================
*/
static void CON_SetColor( WINDOW *win, int color )
{
	// Approximations of g_color_table (q_math.c)
	// Colours are hard-wired below; see init_pair() calls
	static const int colour16map[2][32] = {
		{ // Variant 1 (xterm)
			1 | A_BOLD, 2,          3,          4,
			5,          6,          7,          8,
			4 | A_DIM,  8 | A_DIM,  8 | A_DIM,  8 | A_DIM,
			3 | A_DIM,  4 | A_DIM,  5 | A_DIM,  2 | A_DIM,
			4 | A_DIM,  4 | A_DIM,  6 | A_DIM,  7 | A_DIM,
			6 | A_DIM,  7 | A_DIM,  6 | A_DIM,  3 | A_BOLD,
			3 | A_DIM,  2,          2 | A_DIM,  4 | A_DIM,
			4 | A_DIM,  3 | A_DIM,  7,          4 | A_BOLD
		},
		{ // Variant 2 (vte)
			1 | A_BOLD, 2,          3,          4 | A_BOLD,
			5,          6,          7,          8,
			4        ,  8 | A_DIM,  8 | A_DIM,  8 | A_DIM,
			3 | A_DIM,  4,          5 | A_DIM,  2 | A_DIM,
			4 | A_DIM,  4 | A_DIM,  6 | A_DIM,  7 | A_DIM,
			6 | A_DIM,  7 | A_DIM,  6 | A_DIM,  3 | A_BOLD,
			3 | A_DIM,  2,          2 | A_DIM,  4 | A_DIM,
			4 | A_DIM,  3 | A_DIM,  7,          4 | A_BOLD
		}
	};

	if ( !com_ansiColor || !com_ansiColor->integer )
	{
		wattrset( win, COLOR_PAIR( 0 ) );
	}
	else if ( COLORS >= 256 && com_ansiColor->integer > 0 )
	{
		wattrset( win, COLOR_PAIR( color + 9 ) ); // hardwired below; see init_pair() calls
	}
	else
	{
		int index = abs( com_ansiColor->integer ) - 1;

		if ( index >= sizeof( colour16map ) / sizeof( colour16map[0] ) )
		{
			index = 0;
		}

		wattrset( win, COLOR_PAIR( colour16map[index][ color ] & 0xF ) | ( colour16map[index][color] & ~0xF ) );
	}
}

/*
==================
CON_UpdateCursor

Update the cursor position
==================
*/
#ifdef USE_CURSES_W
static ID_INLINE int CON_wcwidth( const char *s )
{
	int w = wcwidth( Q_UTF8CodePoint( s ) );
	return w < 0 ? 0 : w;
}

static int CON_CursorPosFromScroll( void )
{
	// if we have wide char support, we have ncurses, and probably wcwidth too.
	int i, p = Field_ScrollToOffset( &input_field ), c = 0;

	for ( i = input_field.scroll; i < input_field.cursor; ++i )
	{
		c += CON_wcwidth( input_field.buffer + p );
		p += Q_UTF8Width( input_field.buffer + p );
	}

	return c;
}
#endif

static ID_INLINE void CON_UpdateCursor( void )
{
// pdcurses uses a different mechanism to move the cursor than ncurses
#ifdef _WIN32
	move( LINES - 1, Q_PrintStrlen( PROMPT ) + 8 + input_field.cursor - input_field.scroll );
	wnoutrefresh( stdscr );
#elif defined USE_CURSES_W
	wmove( inputwin, 0, CON_CursorPosFromScroll() );
	wnoutrefresh( inputwin );
#else
	wmove( inputwin, 0, input_field.cursor - input_field.scroll );
	wnoutrefresh( inputwin );
#endif
}

static ID_INLINE void CON_CheckScroll( void )
{
	if ( input_field.cursor < input_field.scroll )
	{
		input_field.scroll = input_field.cursor;
	}
#ifdef USE_CURSES_W
	else
	{
		int c = CON_CursorPosFromScroll();

		while ( c >= input_field.widthInChars )
		{
			c -= CON_wcwidth( input_field.buffer + input_field.scroll++ );
		}
	}
#else
	else if ( input_field.cursor >= input_field.scroll + input_field.widthInChars )
	{
		input_field.scroll = input_field.cursor - input_field.widthInChars + 1;
	}
#endif
}

/*
==================
CON_DrawScrollBar
==================
*/
static void CON_DrawScrollBar( void )
{
	int scroll;

	if ( lastline <= LOG_LINES )
	{
		scroll = 0;
	}
	else
	{
		scroll = scrollline * ( LOG_LINES - 1 ) / ( lastline - LOG_LINES );
	}

	if ( com_ansiColor && !com_ansiColor->integer )
	{
		wbkgdset( scrollwin, SCRLBAR_LINE );
	}
	else
	{
		wbkgdset( scrollwin, SCRLBAR_LINE | COLOR_PAIR( 6 ) );
	}

	werase( scrollwin );
	wbkgdset( scrollwin, ' ' );
	CON_SetColor( scrollwin, 1 );
	mvwaddch( scrollwin, scroll, 0, SCRLBAR_CURSOR );
	wnoutrefresh( scrollwin );
}

/*
==================
CON_ColorPrint
==================
*/
static void CON_ColorPrint( WINDOW *win, const char *msg, qboolean stripcodes )
{
#ifdef USE_CURSES_W
	static wchar_t buffer[ MAXPRINTMSG ];
#else
	static char buffer[ MAXPRINTMSG ];
#endif
	int         length = 0;
	qboolean    noColour = qfalse;

	CON_SetColor( win, 7 );

	while ( *msg )
	{
		if ( ( !noColour && Q_IsColorString( msg ) ) || *msg == '\n' )
		{
			noColour = qfalse;

			// First empty the buffer
			if ( length > 0 )
			{
#ifdef USE_CURSES_W
				buffer[ length ] = L'\0';
				waddwstr( win, buffer );
#else
				buffer[ length ] = '\0';
				wprintw( win, "%s", buffer );
#endif
				length = 0;
			}

			if ( *msg == '\n' )
			{
				// Reset the color and then print a newline
				CON_SetColor( win, 7 );
				waddch( win, '\n' );
				msg++;
			}
			else
			{
				// Set the color
				CON_SetColor( win, ColorIndex( * ( msg + 1 ) ) );

				if ( stripcodes )
				{
					msg += 2;
				}
				else
				{
					if ( length >= MAXPRINTMSG - 1 )
					{
						break;
					}

					buffer[ length ] = *msg;
					length++;
					msg++;

					if ( length >= MAXPRINTMSG - 1 )
					{
						break;
					}

					buffer[ length ] = *msg;
					length++;
					msg++;
				}
			}
		}
		else
		{
			if ( length >= MAXPRINTMSG - 1 )
			{
				break;
			}

			if ( !noColour && *msg == Q_COLOR_ESCAPE && msg[1] == Q_COLOR_ESCAPE )
			{
				if ( stripcodes )
				{
					++msg;
				}
				else
				{
					noColour = qtrue; // guaranteed a colour control next
				}
			}
			else
			{
				noColour = qfalse;
			}
#ifdef USE_CURSES_W
			buffer[ length ] = (wchar_t) Q_UTF8CodePoint( msg );
			msg += Q_UTF8WidthCP( buffer[ length ]);
			length++;
#else
			{
				int chr = Q_UTF8CodePoint( msg );
				msg += Q_UTF8WidthCP( chr );
				buffer[ length++ ] = ( chr >= 0x100 ) ? '?' : chr;
			}
#endif
		}
	}

	// Empty anything still left in the buffer
	if ( length > 0 )
	{
#ifdef USE_CURSES_W
		buffer[ length ] = L'\0';
		waddwstr( win, buffer );
#else
		buffer[ length ] = '\0';
		wprintw( win, "%s", buffer );
#endif
	}
}

/*
==================
CON_UpdateClock

Update the clock
==================
*/
static void CON_UpdateClock( void )
{
	qtime_t realtime;
	Com_RealTime( &realtime );
	werase( clockwin );
	CON_ColorPrint( clockwin, va( "^0[^3%02d%c%02d^0]^7 ", realtime.tm_hour, ( realtime.tm_sec & 1 ) ? ':' : ' ', realtime.tm_min ), qtrue );
	wnoutrefresh( clockwin );
}

/*
==================
CON_Resize

The window has just been resized, move everything back into place
==================
*/
static void CON_Resize( void )
{
#ifndef _WIN32
	struct winsize winsz = { 0, };

	ioctl( fileno( stdout ), TIOCGWINSZ, &winsz );

	if ( winsz.ws_col < 12 || winsz.ws_row < 5 )
	{
		return;
	}

	resizeterm( winsz.ws_row + 1, winsz.ws_col + 1 );
	resizeterm( winsz.ws_row, winsz.ws_col );
	delwin( logwin );
	delwin( borderwin );
	delwin( inputwin );
	delwin( scrollwin );
	erase();
	wnoutrefresh( stdscr );
	CON_Init();
#endif
}

/*
==================
CON_Clear_f
==================
*/
void CON_Clear_f( void )
{
	if ( !curses_on )
	{
//              CON_Clear_tty();
		return;
	}

	// Clear the log and the window
	memset( logbuf, 0, sizeof( logbuf ) );
	werase( logwin );
	pnoutrefresh( logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1 );

	// Move the cursor back to the input field
	CON_UpdateCursor();
	doupdate();
}

/*
==================
CON_Shutdown

Never exit without calling this, or your terminal will be left in a pretty bad state
==================
*/
void CON_Shutdown( void )
{
	if ( !curses_on )
	{
		CON_Shutdown_tty();
		return;
	}

	endwin();
	curses_on = qfalse;
}

/*
==================
CON_Init

Initialize the console in curses mode, fall back to tty mode on failure
==================
*/
void CON_Init( void )
{
	int col;

#ifndef _WIN32
	// If the process is backgrounded (running non interactively)
	// then SIGTTIN or SIGTOU is emitted, if not caught, turns into a SIGSTP
	signal( SIGTTIN, SIG_IGN );
	signal( SIGTTOU, SIG_IGN );
#endif

	// Make sure we're on a tty
	if ( !isatty( STDIN_FILENO ) || !isatty( STDOUT_FILENO ) )
	{
		CON_Init_tty();
		return;
	}

	// Initialize curses and set up the root window
	if ( !curses_on )
	{
		SCREEN *test = newterm( NULL, stdout, stdin );

		if ( !test )
		{
			CON_Init_tty();
			CON_Print_tty( "Couldn't initialize curses, falling back to tty\n" );
			return;
		}

		endwin();
		delscreen( test );
		setlocale(LC_CTYPE, "");
		initscr();
		cbreak();
		noecho();
		nonl();
		intrflush( stdscr, FALSE );
		nodelay( stdscr, TRUE );
		keypad( stdscr, TRUE );
		wnoutrefresh( stdscr );

		// Set up colors
		if ( has_colors() )
		{
			// Mappings used in CON_SetColor()
			static const unsigned char colourmap[] = {
				0, // <- dummy entry
				// 8-colour terminal mappings (modified later with bold/dim)
				COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
				COLOR_BLUE, COLOR_CYAN, COLOR_MAGENTA, COLOR_WHITE,
				// 256-colour terminal mappings
				239, 196,  46, 226,  21,  51, 201, 231,
				208, 244, 250, 250,  28, 100,  18,  88,
				 94, 209,  30,  90,  33,  93,  68, 194,
				 29, 197, 124,  94, 173, 101, 229, 228
			};
			int i;

			use_default_colors();
			start_color();
			init_pair( 0, -1, -1 );

			for ( i = ( COLORS >= 256 ) ? 40 : 8; i; --i )
			{
				init_pair( i, colourmap[i], -1 );
			}
		}

		// Prevent bad libraries from messing up the console
		fclose( stderr );
	}

	// Create the border
	borderwin = newwin( LOG_LINES + 2, LOG_COLS + 2, 1, 0 );
	CON_SetColor( borderwin, 2 );
	box( borderwin, 0, 0 );
	wnoutrefresh( borderwin );

	// Create the log window
	logwin = newpad( MAX_LOG_LINES, LOG_COLS );
	scrollok( logwin, TRUE );
	idlok( logwin, TRUE );

	if ( curses_on )
	{
		CON_ColorPrint( logwin, logbuf, qtrue );
	}

	getyx( logwin, lastline, col );

	if ( col )
	{
		lastline++;
	}

	scrollline = lastline - LOG_LINES;

	if ( scrollline < 0 )
	{
		scrollline = 0;
	}

	pnoutrefresh( logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1 );

	// Create the scroll bar
	scrollwin = newwin( LOG_LINES, 1, 2, COLS - 1 );
	CON_DrawScrollBar();
	CON_SetColor( stdscr, 3 );
	mvaddch( 1, COLS - 1, SCRLBAR_UP );
	mvaddch( LINES - 2, COLS - 1, SCRLBAR_DOWN );

	// Create the input field
	inputwin = newwin( 1, COLS - Q_PrintStrlen( PROMPT ) - 8, LINES - 1, Q_PrintStrlen( PROMPT ) + 8 );
	input_field.widthInChars = COLS - Q_PrintStrlen( PROMPT ) - 9;

	if ( curses_on )
	{
		CON_CheckScroll();
		CON_ColorPrint( inputwin, input_field.buffer + Field_ScrollToOffset( &input_field ), qfalse );
	}

	CON_UpdateCursor();
	wnoutrefresh( inputwin );

	// Create the clock
	clockwin = newwin( 1, 8, LINES - 1, 0 );
	CON_UpdateClock();

	// Display the title and input prompt
	move( 0, ( COLS - Q_PrintStrlen( TITLE ) ) / 2 );
	CON_ColorPrint( stdscr, TITLE, qtrue );
	move( LINES - 1, 8 );
	CON_ColorPrint( stdscr, PROMPT, qtrue );
	wnoutrefresh( stdscr );
	doupdate();

#ifndef _WIN32
	// Catch window resizes
	signal( SIGWINCH, ( void * ) CON_Resize );
#endif

	curses_on = qtrue;
}

/*
==================
CON_Input
==================
*/
char *CON_Input( void )
{
	int         chr, num_chars = 0;
	static char text[ MAX_EDIT_LINE ];
	static int  lasttime = -1;
	static enum {
		MODE_UNKNOWN,
		MODE_PLAIN,
		MODE_UTF8
	}           mode;

	if ( !curses_on )
	{
		return CON_Input_tty();
	}

	if ( com_ansiColor->modified )
	{
		CON_Resize();
		com_ansiColor->modified = qfalse;
	}

	if ( Com_RealTime( NULL ) != lasttime )
	{
		lasttime = Com_RealTime( NULL );
		CON_UpdateClock();
		num_chars++;
	}

	while ( 1 )
	{
		chr = getch();
		num_chars++;

		// Special characters
		switch ( chr )
		{
			case ERR:
				if ( num_chars > 1 )
				{
					werase( inputwin );

					CON_CheckScroll(); // ( INPUT_SCROLL );

					CON_ColorPrint( inputwin, input_field.buffer + Field_ScrollToOffset( &input_field ), qfalse );
#ifdef _WIN32
					wrefresh( inputwin );  // If this is not done the cursor moves strangely
#else
					wnoutrefresh( inputwin );
#endif
					CON_UpdateCursor();
					doupdate();
				}

				return NULL;

			case '\n':
			case '\r':
			case KEY_ENTER:
				if ( !input_field.buffer[ 0 ] )
				{
					continue;
				}

				Q_snprintf( text, sizeof( text ), "\\%s",
				            input_field.buffer + ( input_field.buffer[ 0 ] == '\\' || input_field.buffer[ 0 ] == '/' ) );
				Hist_Add( text );
				Field_Clear( &input_field );
				werase( inputwin );
				wnoutrefresh( inputwin );
				CON_UpdateCursor();
				//doupdate();
				Com_Printf( PROMPT "^7%s\n", text + 1 );
				return text + 1;

			case 21: // Ctrl-U
				Field_Clear( &input_field );
				werase( inputwin );
				wnoutrefresh( inputwin );
				CON_UpdateCursor();
				continue;

			case 11: // Ctrl-K
				input_field.buffer[ Field_CursorToOffset( &input_field ) ] = '\0';
				continue;

			case '\t':
			case KEY_STAB:
				Field_AutoComplete( &input_field, PROMPT );
				input_field.cursor = Q_UTF8Strlen( input_field.buffer );
				continue;

			case '\f':
				CON_Resize();
				continue;

			case KEY_LEFT:
				if ( input_field.cursor > 0 )
				{
					input_field.cursor--;
				}

				continue;

			case KEY_RIGHT:
				if ( input_field.cursor < Q_UTF8Strlen( input_field.buffer ) )
				{
					input_field.cursor++;
				}

				continue;

			case KEY_UP:
				{
					const char *field = Hist_Prev();

					if ( field )
					{
						strcpy( input_field.buffer, field );
						goto key_end;
					}

					continue;
				}

			case KEY_DOWN:
				{
					const char *field = Hist_Next();

					if ( field )
					{
						strcpy( input_field.buffer, field );
						goto key_end;
					}
					else
					{
						Field_Clear( &input_field );
					}

					continue;
				}

			case 1: // Ctrl-A
			case KEY_HOME:
				input_field.cursor = 0;
				continue;

			case 5: // Ctrl-E
			case KEY_END:
				key_end:
				input_field.cursor = Q_UTF8Strlen( input_field.buffer );
				continue;

			case KEY_NPAGE:
				if ( lastline > scrollline + LOG_LINES )
				{
					scrollline += LOG_SCROLL;

					if ( scrollline + LOG_LINES > lastline )
					{
						scrollline = lastline - LOG_LINES;
					}

					pnoutrefresh( logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1 );
					CON_DrawScrollBar();
				}

				continue;

			case KEY_PPAGE:
				if ( scrollline > 0 )
				{
					scrollline -= LOG_SCROLL;

					if ( scrollline < 0 )
					{
						scrollline = 0;
					}

					pnoutrefresh( logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1 );
					CON_DrawScrollBar();
				}

				continue;

			case '\b':
			case 127:
			case KEY_BACKSPACE:
				if ( input_field.cursor <= 0 )
				{
					continue;
				}

				input_field.cursor--;

				// Fall through
			case 4: // Ctrl-D
			case KEY_DC:
				if ( input_field.cursor < Q_UTF8Strlen( input_field.buffer ) )
				{
					int offset = Field_CursorToOffset( &input_field );
					int width = Q_UTF8Width( input_field.buffer + offset );
					memmove( input_field.buffer + offset,
					         input_field.buffer + offset + width,
					         strlen( input_field.buffer + offset + width ) + 1 );
				}

				continue;

		}

		// Normal characters
		if ( chr >= ' ' )
		{
			int width = 1;//Q_UTF8WidthCP( chr );
			int count, offset;
			char buf[20];

			if ( chr >= 0x100 )
				continue; // !!!

			if ( chr >= 0xC2 && chr <= 0xF4 )
			{
				switch ( mode )
				{
				case MODE_UNKNOWN:
				case MODE_UTF8:
					timeout( 2 );
					count = ( chr < 0xE0 ) ? 1 : ( chr < 0xF0 ) ? 2 : 3;
					width = count + 1;
					while ( --count >= 0)
					{
						int ch = getch();

						if ( ch == ERR )
						{
							if ( mode == MODE_UNKNOWN )
							{
								mode = MODE_PLAIN;
								width -= count;
							}
							else
							{
								width = 0;
							}
							break;
						}

						chr = chr << 8 | ch;

						if ( ch < 0x80 || ch >= 0xC0 )
						{
							width -= count;
							mode = MODE_PLAIN;
							break;
						}
						mode = MODE_UTF8;
					}
					timeout( 0 );
					break;

				case MODE_PLAIN:
					break;
				}
			}
			else
			{
				switch ( mode )
				{
				case MODE_UNKNOWN:
					if ( chr >= 0x80 )
						mode = MODE_PLAIN;
				case MODE_PLAIN:
					break;
				case MODE_UTF8:
					if ( chr >= 0x80 )
					{
						width = 0;
					}
					break;
				}
			}

			switch ( mode )
			{
			case MODE_UNKNOWN:
			case MODE_PLAIN:
				// convert non-ASCII to UTF-8, then insert
				// FIXME: assumes Latin-1
				buf[0] = '\0';

				for ( count = 0; count <= width; ++count )
				{
					strcat( buf, Q_UTF8Encode( ( chr >> ( ( width - count ) * 8 ) ) & 0xFF ) );
				}

				count = strlen( buf );
				offset = Field_CursorToOffset( &input_field );
				
				memmove( input_field.buffer + offset + count,
				         input_field.buffer + offset,
				         strlen( input_field.buffer + offset ) + 1 );
				memcpy( input_field.buffer + offset, buf, count );

				input_field.cursor += width;
				break;

			case MODE_UTF8:
				// UTF8, but packed
				if ( width > 0 && strlen( input_field.buffer ) + width < sizeof( input_field.buffer ) )
				{
					offset = Field_CursorToOffset( &input_field );

					memmove( input_field.buffer + offset + width,
							 input_field.buffer + offset,
							 strlen( input_field.buffer + offset ) + 1 );

					input_field.cursor += ( mode == MODE_UTF8 ) ? 1 : width;
					--width;

					for ( count = 0; count <= width; ++count )
					{
						input_field.buffer[ offset + count ] = ( chr >> ( ( width - count ) * 8 ) ) & 0xFF;
					}
				}
				break;
			}
		}
	}
}

/*
==================
CON_Print
==================
*/
void CON_Print( const char *msg )
{
	int      col;
	qboolean scroll = ( lastline > scrollline && lastline <= scrollline + LOG_LINES );

	if ( !curses_on )
	{
		CON_Print_tty( msg );
		return;
	}

	// Print the message in the log window
	CON_ColorPrint( logwin, msg, qtrue );
	getyx( logwin, lastline, col );

	if ( col )
	{
		lastline++;
	}

	if ( scroll )
	{
		scrollline = lastline - LOG_LINES;

		if ( scrollline < 0 )
		{
			scrollline = 0;
		}

		pnoutrefresh( logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1 );
	}

	// Add the message to the log buffer
	if ( insert + strlen( msg ) >= logbuf + sizeof( logbuf ) )
	{
		memmove( logbuf, logbuf + sizeof( logbuf ) / 2, sizeof( logbuf ) / 2 );
		memset( logbuf + sizeof( logbuf ) / 2, 0, sizeof( logbuf ) / 2 );
		insert -= sizeof( logbuf ) / 2;
	}

	strcpy( insert, msg );
	insert += strlen( msg );

	// Update the scrollbar
	CON_DrawScrollBar();

	// Move the cursor back to the input field
	CON_UpdateCursor();
	doupdate();
}
