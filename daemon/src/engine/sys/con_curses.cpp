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

#include "qcommon/q_shared.h"
#include "qcommon/q_unicode.h"
#include "qcommon/qcommon.h"

#include "framework/ConsoleField.h"

// curses.h defines COLOR_*, which are already defined in q_shared.h
#undef COLOR_BLACK
#undef COLOR_RED
#undef COLOR_GREEN
#undef COLOR_YELLOW
#undef COLOR_BLUE
#undef COLOR_MAGENTA
#undef COLOR_CYAN
#undef COLOR_WHITE

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include <curses.h>

#ifdef BUILD_SERVER
#define TITLE         S_COLOR_GREEN "[ " S_COLOR_YELLOW CLIENT_WINDOW_TITLE " Server Console " S_COLOR_GREEN "]"
#else
#define TITLE         S_COLOR_GREEN "[ " S_COLOR_YELLOW CLIENT_WINDOW_TITLE " Console " S_COLOR_GREEN "]"
#endif
#define PROMPT        S_COLOR_YELLOW "-> "
#define INPUT_SCROLL  15
#define LOG_SCROLL    5
#define MAX_LOG_LINES 1024
#define LOG_BUF_SIZE  65536

// TTY Console prototypes
void CON_Shutdown_TTY();
void CON_Init_TTY();
char *CON_Input_TTY();
void CON_Print_TTY( const char *message );

static bool curses_on = false;
static bool dump_logs = false;
static Console::Field input_field(INT_MAX);
static WINDOW   *borderwin;
static WINDOW   *logwin;
static WINDOW   *inputwin;
static WINDOW   *clockwin;

static char     logbuf[ LOG_BUF_SIZE ];
static char     *insert = logbuf;
static int      scrollline = 0;
static int      lastline = 1;
static bool     forceRedraw = false;

#ifndef _WIN32
static int      stderr_fd;
#endif

#define LOG_LINES      ( LINES - 3 )

#define CURSES_NULL_COLOR -1

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
	static const chtype colour16map[2][32] = {
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

	if ( color == CURSES_NULL_COLOR || !com_ansiColor.Get() )
	{
		wattrset( win, COLOR_PAIR( 0 ) );
	}
	else if ( COLORS >= 256 && com_ansiColor.Get() )
	{
#ifdef A_RGB  //macro producing color attribute for a 64-bit chtype in pdcurses
		wattrset( win, A_RGB( (int)( g_color_table[color][0] * 31 ),
		    (int)( g_color_table[color][1] * 31 ), (int)( g_color_table[color][2] * 31 ), 0, 0, 0 ) );
#else
		wattrset( win, COLOR_PAIR( color + 9 ) ); // hard-wired below; see init_pair() calls
#endif
	}
	else
	{
		int index = com_ansiColor.Get() ? 1 : 0;

		if ( index >= (int) ARRAY_LEN( colour16map ) )
		{
			index = 0;
		}

		wattrset( win, COLOR_PAIR( colour16map[index][ color ] & 0xF ) | ( colour16map[index][color] & ~0xF ) );
	}
}

static INLINE void CON_UpdateCursor()
{
// pdcurses uses a different mechanism to move the cursor than ncurses
#ifdef _WIN32
	move( LINES - 1, Q_PrintStrlen( PROMPT ) + 8 + input_field.GetViewCursorPos() );
	wnoutrefresh( stdscr );
#else
	wmove( inputwin, 0, input_field.GetViewCursorPos() );
	wnoutrefresh( inputwin );
#endif
}

/*
==================
CON_ColorPrint
==================
*/
static void CON_ColorPrint( WINDOW *win, const char *msg, bool stripcodes )
{
	static wchar_t buffer[ MAXPRINTMSG ];
	int         length = 0;
	bool    noColour = false;

	CON_SetColor( win, CURSES_NULL_COLOR );

	while ( *msg )
	{
		if ( ( !noColour && Q_IsColorString( msg ) ) || *msg == '\n' )
		{
			noColour = false;

			// First empty the buffer
			if ( length > 0 )
			{
				buffer[ length ] = L'\0';
				waddwstr( win, buffer );
				length = 0;
			}

			if ( *msg == '\n' )
			{
				// Reset the color and then print a newline
				CON_SetColor( win, CURSES_NULL_COLOR );
				waddch( win, '\n' );
				msg++;
			}
			else
			{
				// Set the color
				CON_SetColor( win, *( msg + 1 ) == COLOR_NULL ? CURSES_NULL_COLOR : ColorIndex( *( msg + 1 ) ) );

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
					noColour = true; // guaranteed a colour control next
				}
			}
			else
			{
				noColour = false;
			}
			buffer[ length ] = (wchar_t) Q_UTF8_CodePoint( msg );
			msg += Q_UTF8_WidthCP( buffer[ length ]);
			length++;
		}
	}

	// Empty anything still left in the buffer
	if ( length > 0 )
	{
		buffer[ length ] = L'\0';
		waddwstr( win, buffer );
	}
}

/*
==================
CON_UpdateClock

Update the clock
==================
*/
static void CON_UpdateClock()
{
	qtime_t realtime;
	Com_Time( &realtime );
	werase( clockwin );
	CON_ColorPrint( clockwin, va( "^0[^3%02d%c%02d^0]^7 ", realtime.tm_hour, ( realtime.tm_sec & 1 ) ? ':' : ' ', realtime.tm_min ), true );
	wnoutrefresh( clockwin );
}

/*
==================
CON_Redraw

Redraw everything
==================
*/
static void CON_Redraw()
{
	int col;

	// Delete any existing windows
	if( logwin )
	{
#ifndef _WIN32
		struct winsize winsz;

		ioctl( fileno( stdout ), TIOCGWINSZ, &winsz );

		if ( winsz.ws_col < 12 || winsz.ws_row < 5 )
		{
			return;
		}
		resizeterm( winsz.ws_row, winsz.ws_col );
#endif

		delwin( logwin );
		delwin( borderwin );
		delwin( inputwin );
		erase();
		wnoutrefresh( stdscr );
	}

	// Create the log window
	logwin = newpad( MAX_LOG_LINES, COLS );
	scrollok( logwin, TRUE );
	idlok( logwin, TRUE );
	CON_ColorPrint( logwin, logbuf, true );
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
	pnoutrefresh( logwin, scrollline, 0, 1, 0, LOG_LINES, COLS );

	// Create the input field
	inputwin = newwin( 1, COLS - Q_PrintStrlen( PROMPT ) - 8, LINES - 1, Q_PrintStrlen( PROMPT ) + 8 );
	input_field.SetWidth(COLS - Q_PrintStrlen( PROMPT ) - 9);
	CON_ColorPrint( inputwin, Str::UTF32To8(input_field.GetViewText()).c_str(), false );
	CON_UpdateCursor();
	wnoutrefresh( inputwin );

	// Create the clock
	clockwin = newwin( 1, 8, LINES - 1, 0 );
	CON_UpdateClock();

	// Create the border
	CON_SetColor( stdscr, 2 );
	for (int i = 0; i < COLS; i++) {
		mvaddch(0, i, ACS_HLINE);
		mvaddch(LINES - 2, i, ACS_HLINE);
	}

	// Display the title and input prompt
	move( 0, ( COLS - Q_PrintStrlen( TITLE ) ) / 2 );
	CON_ColorPrint( stdscr, TITLE, true );
	move( LINES - 1, 8 );
	CON_ColorPrint( stdscr, PROMPT, true );

	wnoutrefresh( stdscr );
	doupdate();
}

/*
==================
CON_Resize

The window has just been resized, move everything back into place
==================
*/
#ifndef _WIN32
static void CON_Resize( int )
{
	// Don't call Redraw() directly, because it is slow, and we might miss more
	// resize signals while redrawing.
	forceRedraw = true;
}
#endif

/*
==================
CON_Clear_f
==================
*/
void CON_Clear_f()
{
	if ( !curses_on )
	{
		//CON_Clear_TTY();
		return;
	}

	// Clear the log and the window
	memset( logbuf, 0, sizeof( logbuf ) );
	werase( logwin );
	pnoutrefresh( logwin, scrollline, 0, 1, 0, LOG_LINES, COLS );

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
void CON_Shutdown()
{
	if ( !curses_on )
	{
		CON_Shutdown_TTY();
		return;
	}

	erase();
	refresh();
	endwin();
	dump_logs = curses_on;
	curses_on = false;

#ifndef _WIN32
	if ( stderr_fd >= 0 )
	{
		dup2( stderr_fd, STDERR_FILENO );
	}
#endif
}

/*
==================
CON_DumpLog

Used for dumping log text to the tty.
May be called on shutdown from a signal handler.
==================
*/
void CON_LogDump()
{
	if ( dump_logs )
	{
		const char *ptr = insert;
		int lines = 0;

		dump_logs = 0;

		while ( lines < 24 && --ptr >= logbuf )
		{
			if ( *ptr == '\n' )
			{
				++lines;
			}
		}

		while ( *ptr == '\n' && ptr < insert )
		{
			++ptr;
		}

		if ( insert - ptr )
		{
			fputs( "\nPartial log dump:\n\n", stderr );
			fwrite( ptr, 1, insert - ptr, stderr );
		}
	}
}

/*
==================
CON_Init

Initialize the console in curses mode, fall back to tty mode on failure
==================
*/
void CON_Init()
{
#ifndef _WIN32
	// If the process is backgrounded (running non-interactively),
	// then SIGTTIN or SIGTTOU is emitted; if not caught, turns into a SIGSTP
	signal( SIGTTIN, SIG_IGN );
	signal( SIGTTOU, SIG_IGN );

	// Make sure we're on a tty
	if ( !isatty( STDIN_FILENO ) || !isatty( STDOUT_FILENO ) )
	{
		CON_Init_TTY();
		return;
	}
#endif

	// Initialize curses and set up the root window
	if ( !curses_on )
	{
#ifndef _WIN32
		// Enable more colors
		const char* term = getenv("TERM");
		if (!strncmp(term, "xterm", 5) || !strncmp(term, "screen", 6))
			setenv("TERM", "xterm-256color", 1);
		setlocale(LC_CTYPE, "");
#endif

		initscr();
		cbreak();
		noecho();
		nonl();
		intrflush( stdscr, FALSE );
		nodelay( stdscr, TRUE );
		keypad( stdscr, TRUE );
		meta( stdscr, TRUE );
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

#ifdef SIGWINCH
		// Catch window resizes
		signal( SIGWINCH, CON_Resize );
#endif

		// Prevent bad libraries from messing up the console
#ifndef _WIN32
		stderr_fd = dup( STDERR_FILENO );
		close( STDERR_FILENO );
#endif

		curses_on = true;
	}

	CON_Redraw();
}

/*
==================
CON_Input
==================
*/
char *CON_Input()
{
	int         chr, num_chars = 0;
	static int  lasttime = -1;
	static enum {
		MODE_UNKNOWN,
		MODE_PLAIN,
		MODE_UTF8
	}           mode;

	if ( !curses_on )
	{
		return CON_Input_TTY();
	}

    /*
	if ( com_ansiColor->modified )
	{
		CON_Redraw();
		com_ansiColor->modified = false;
	}
    */

	if ( Com_Time( nullptr ) != lasttime )
	{
		lasttime = Com_Time( nullptr );
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
				if ( forceRedraw )
				{
					CON_Redraw();
					forceRedraw = false;
				}
				else if ( num_chars > 1 )
				{
					werase( inputwin );

					CON_ColorPrint( inputwin, Str::UTF32To8(input_field.GetViewText()).c_str(), false );
#ifdef _WIN32
					wrefresh( inputwin );  // If this is not done the cursor moves strangely
#else
					wnoutrefresh( inputwin );
#endif
					CON_UpdateCursor();
					doupdate();
				}

				return nullptr;

			case '\n':
			case '\r':
			case KEY_ENTER:
                Log::Notice( PROMPT S_COLOR_NULL "%s", Str::UTF32To8(input_field.GetText()).c_str() );
				input_field.RunCommand(com_consoleCommand.Get());
				werase( inputwin );
				wnoutrefresh( inputwin );
				continue;

			case 21: // Ctrl-U
				input_field.Clear();
				werase( inputwin );
				wnoutrefresh( inputwin );
				continue;

			case 11: // Ctrl-K
				input_field.DeleteEnd();
				continue;

			case '\t':
			case KEY_STAB:
				input_field.AutoComplete();
				continue;

			case '\f':
				forceRedraw = true;
				continue;

			case KEY_LEFT:
				input_field.CursorLeft();
				continue;

			case KEY_RIGHT:
				input_field.CursorRight();
				continue;

			case KEY_UP:
				input_field.HistoryPrev();
				continue;

			case KEY_DOWN:
				input_field.HistoryNext();
				continue;

			case 1: // Ctrl-A
			case KEY_HOME:
				input_field.CursorStart();
				continue;

			case 5: // Ctrl-E
			case KEY_END:
				input_field.CursorEnd();
				continue;

			case KEY_NPAGE:
				if ( lastline > scrollline + LOG_LINES )
				{
					scrollline += LOG_SCROLL;

					if ( scrollline + LOG_LINES > lastline )
					{
						scrollline = lastline - LOG_LINES;
					}

					pnoutrefresh( logwin, scrollline, 0, 1, 0, LOG_LINES, COLS );
				}
				if (scrollline >= lastline - LOG_LINES) {
					CON_SetColor(stdscr, 2);
					for (int i = COLS - 7; i < COLS - 1; i++)
						mvaddch(LINES - 2, i, ACS_HLINE);
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

					pnoutrefresh( logwin, scrollline, 0, 1, 0, LOG_LINES, COLS );
				}
				if (scrollline < lastline - LOG_LINES) {
					CON_SetColor(stdscr, 1);
					mvaddstr(LINES - 2, COLS - 7, "(more)");
				}

				continue;

			case '\b':
			case 127:
			case KEY_BACKSPACE:
				input_field.DeletePrev();
				continue;

			case 4: // Ctrl-D
			case KEY_DC:
				input_field.DeleteNext();
				continue;

			case 20: // Ctrl-T
				input_field.SwapWithNext();
				continue;
		}

		// Other characters
		if ( chr >= ' ' )
		{
			int width = 1;//Q_UTF8_WidthCP( chr );
			int count;
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

			std::u32string u32text;
			switch ( mode )
			{
			case MODE_UNKNOWN:
			case MODE_PLAIN:
				// convert non-ASCII to UTF-8, then insert
				// FIXME: assumes Latin-1
				for ( count = 0; count < width; ++count )
				{
					input_field.AddChar( ( chr >> ( ( width - count - 1 ) * 8 ) ) & 0xFF );
				}
				break;

			case MODE_UTF8:
				// UTF-8, but packed

				if ( width > 0 )
				{
					for ( count = 0; count < width; ++count )
					{
						buf[ count ] = ( chr >> ( ( width - count - 1 ) * 8 ) ) & 0xFF;
					}
					input_field.AddChar( Q_UTF8_CodePoint( buf ) );
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
	bool scroll = ( lastline > scrollline && lastline <= scrollline + LOG_LINES );

	if ( !curses_on )
	{
		CON_Print_TTY( msg );
		return;
	}

	// Print the message in the log window
	CON_ColorPrint( logwin, msg, true );
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

		pnoutrefresh( logwin, scrollline, 0, 1, 0, LOG_LINES, COLS );
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

	// Move the cursor back to the input field
	CON_UpdateCursor();
	doupdate();
}
