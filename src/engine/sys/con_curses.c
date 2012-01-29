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

#include <curses.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/ioctl.h>
#endif

#define TITLE "^4---[ ^3" CLIENT_WINDOW_TITLE " Console ^4]---"
#define PROMPT "^3-> "
#define INPUT_SCROLL 15
#define LOG_SCROLL 5
#define MAX_LOG_LINES 1024
#define LOG_BUF_SIZE 65536

// Functions from the tty console for fallback
void CON_Shutdown_tty(void);
void CON_Init_tty(void);
char *CON_Input_tty(void);
void CON_Print_tty(const char *message);
void CON_Clear_tty(void);

static qboolean curses_on = qfalse;
static field_t input_field;
static WINDOW *borderwin;
static WINDOW *logwin;
static WINDOW *inputwin;
static WINDOW *scrollwin;
static WINDOW *clockwin;

static char logbuf[LOG_BUF_SIZE];
static char *insert = logbuf;
static int scrollline = 0;
static int lastline = 1;

// The special characters look good on the win32 console but suck on other consoles
#ifdef _WIN32
#define SCRLBAR_CURSOR ACS_BLOCK
#define SCRLBAR_LINE ACS_VLINE
#define SCRLBAR_UP ACS_UARROW
#define SCRLBAR_DOWN ACS_DARROW
#else
#define SCRLBAR_CURSOR '#'
#define SCRLBAR_LINE ACS_VLINE
#define SCRLBAR_UP ACS_HLINE
#define SCRLBAR_DOWN ACS_HLINE
#endif

#define LOG_LINES (LINES - 4)
#define LOG_COLS (COLS - 3)

/*
==================
CON_SetColor

Use grey instead of black
==================
*/
static inline void CON_SetColor(WINDOW *win, int color)
{
	if (com_ansiColor && !com_ansiColor->integer)
		color = 7;
	if (color == 0)
		wattrset(win, COLOR_PAIR(color + 1) | A_BOLD);
	else
		wattrset(win, COLOR_PAIR(color + 1) | A_NORMAL);
}

/*
==================
CON_UpdateCursor

Update the cursor position
==================
*/
static inline void CON_UpdateCursor(void)
{
// pdcurses uses a different mechanism to move the cursor than ncurses
#ifdef _WIN32
	move(LINES - 1, Q_PrintStrlen(PROMPT) + 8 + input_field.cursor - input_field.scroll);
	wnoutrefresh(stdscr);
#else
	wmove(inputwin, 0, input_field.cursor - input_field.scroll);
	wnoutrefresh(inputwin);
#endif
}

/*
==================
CON_DrawScrollBar
==================
*/
static void CON_DrawScrollBar(void)
{
	int scroll;

	if (lastline <= LOG_LINES)
		scroll = 0;
	else
		scroll = scrollline * (LOG_LINES - 1) / (lastline - LOG_LINES);

	if (com_ansiColor && !com_ansiColor->integer)
		wbkgdset(scrollwin, SCRLBAR_LINE);
	else
		wbkgdset(scrollwin, SCRLBAR_LINE | COLOR_PAIR(6));
	werase(scrollwin);
	wbkgdset(scrollwin, ' ');
	CON_SetColor(scrollwin, 1);
	mvwaddch(scrollwin, scroll, 0, SCRLBAR_CURSOR);
	wnoutrefresh(scrollwin);
}

/*
==================
CON_ColorPrint
==================
*/
static void CON_ColorPrint(WINDOW *win, const char *msg, qboolean stripcodes)
{
	static char buffer[MAXPRINTMSG];
	int length = 0;

	CON_SetColor(win, 7);
	while(*msg) {
		if (Q_IsColorString(msg) || *msg == '\n') {
			// First empty the buffer
			if (length > 0) {
				buffer[length] = '\0';
				wprintw(win, "%s", buffer);
				length = 0;
			}

			if (*msg == '\n') {
				// Reset the color and then print a newline
				CON_SetColor(win, 7);
				waddch(win, '\n');
				msg++;
			} else {
				// Set the color
				CON_SetColor(win, ColorIndex(*(msg + 1)));
				if (stripcodes)
					msg += 2;
				else {
					if (length >= MAXPRINTMSG - 1)
						break;
					buffer[length] = *msg;
					length++;
					msg++;
					if (length >= MAXPRINTMSG - 1)
						break;
					buffer[length] = *msg;
					length++;
					msg++;
				}
			}
		} else {
			if (length >= MAXPRINTMSG - 1)
				break;

			buffer[length] = *msg;
			length++;
			msg++;
		}
	}

	// Empty anything still left in the buffer
	if( length > 0 ) {
		buffer[length] = '\0';
		wprintw(win, "%s", buffer);
	}
}

/*
==================
CON_UpdateClock

Update the clock
==================
*/
static void CON_UpdateClock(void)
{
	qtime_t realtime;
	Com_RealTime(&realtime);
	werase(clockwin);
	CON_ColorPrint(clockwin, va("^0[^3%02d%c%02d^0]^7 ", realtime.tm_hour, (realtime.tm_sec & 1) ? ':' : ' ', realtime.tm_min), qtrue);
	wnoutrefresh(clockwin);
}

/*
==================
CON_Resize

The window has just been resized, move everything back into place
==================
*/
static void CON_Resize(void)
{
#ifndef _WIN32
	struct winsize winsz = {0, };

	ioctl(fileno(stdout), TIOCGWINSZ, &winsz);
	if (winsz.ws_col < 12 || winsz.ws_row < 5)
		return;
	resizeterm(winsz.ws_row + 1, winsz.ws_col + 1);
	resizeterm(winsz.ws_row, winsz.ws_col);
	delwin(logwin);
	delwin(borderwin);
	delwin(inputwin);
	delwin(scrollwin);
	erase();
	wnoutrefresh(stdscr);
	CON_Init();
#endif
}

/*
==================
CON_Clear_f
==================
*/
void CON_Clear_f(void)
{
	if (!curses_on) {
// 		CON_Clear_tty();
		return;
	}

	// Clear the log and the window
	memset(logbuf, 0, sizeof(logbuf));
	werase(logwin);
	pnoutrefresh(logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1);

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
void CON_Shutdown(void)
{
	if (!curses_on) {
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
void CON_Init(void)
{
	int col;

#ifndef _WIN32
	// If the process is backgrounded (running non interactively)
	// then SIGTTIN or SIGTOU is emitted, if not caught, turns into a SIGSTP
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
#endif

	// Make sure we're on a tty
	if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
		CON_Init_tty();
		return;
	}

	// Initialize curses and set up the root window
	if (!curses_on) {
		SCREEN *test = newterm(NULL, stdout, stdin);
		if (!test) {
			CON_Init_tty();
			CON_Print_tty("Couldn't initialize curses, falling back to tty\n");
			return;
		}
		endwin();
		delscreen(test);
		initscr();
		cbreak();
		noecho();
		nonl();
		intrflush(stdscr, FALSE);
		nodelay(stdscr, TRUE);
		keypad(stdscr, TRUE);
		wnoutrefresh(stdscr);

		// Set up colors
		if (has_colors()) {
			use_default_colors();
			start_color();
			init_pair(1, COLOR_BLACK, -1);
			init_pair(2, COLOR_RED, -1);
			init_pair(3, COLOR_GREEN, -1);
			init_pair(4, COLOR_YELLOW, -1);
			init_pair(5, COLOR_BLUE, -1);
			init_pair(6, COLOR_CYAN, -1);
			init_pair(7, COLOR_MAGENTA, -1);
			init_pair(8, -1, -1);
		}

		// Prevent bad libraries from messing up the console
		fclose(stderr);
	}

	// Create the border
	borderwin = newwin(LOG_LINES + 2, LOG_COLS + 2, 1, 0);
	CON_SetColor(borderwin, 2);
	box(borderwin, 0, 0);
	wnoutrefresh(borderwin);

	// Create the log window
	logwin = newpad(MAX_LOG_LINES, LOG_COLS);
	scrollok(logwin, TRUE);
	idlok(logwin, TRUE);
	if (curses_on)
		CON_ColorPrint(logwin, logbuf, qtrue);
	getyx(logwin, lastline, col);
	if (col)
		lastline++;
	scrollline = lastline - LOG_LINES;
	if (scrollline < 0)
		scrollline = 0;
	pnoutrefresh(logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1);

	// Create the scroll bar
	scrollwin = newwin(LOG_LINES, 1, 2, COLS - 1);
	CON_DrawScrollBar();
	CON_SetColor(stdscr, 3);
	mvaddch(1, COLS - 1, SCRLBAR_UP);
	mvaddch(LINES - 2, COLS - 1, SCRLBAR_DOWN);

	// Create the input field
	inputwin = newwin(1, COLS - Q_PrintStrlen(PROMPT) - 8, LINES - 1, Q_PrintStrlen(PROMPT) + 8);
	input_field.widthInChars = COLS - Q_PrintStrlen(PROMPT) - 9;
	if (curses_on) {
		if (input_field.cursor < input_field.scroll)
			input_field.scroll = input_field.cursor;
		else if (input_field.cursor >= input_field.scroll + input_field.widthInChars)
			input_field.scroll = input_field.cursor - input_field.widthInChars + 1;
		CON_ColorPrint(inputwin, input_field.buffer + input_field.scroll, qfalse);
	}
	CON_UpdateCursor();
	wnoutrefresh(inputwin);

	// Create the clock
	clockwin = newwin(1, 8, LINES - 1, 0);
	CON_UpdateClock();

	// Display the title and input prompt
	move(0, (COLS - Q_PrintStrlen(TITLE)) / 2);
	CON_ColorPrint(stdscr, TITLE, qtrue);
	move(LINES - 1, 8);
	CON_ColorPrint(stdscr, PROMPT, qtrue);
	wnoutrefresh(stdscr);
	doupdate();

#ifndef _WIN32
	// Catch window resizes
	signal(SIGWINCH, (void *)CON_Resize);
#endif

	curses_on = qtrue;
}

/*
==================
CON_Input
==================
*/
char *CON_Input(void)
{
	int chr, num_chars = 0;
	static char text[MAX_EDIT_LINE];
	static int lasttime = -1;

	if (!curses_on)
		return CON_Input_tty();

	if (com_ansiColor->modified) {
		CON_Resize();
		com_ansiColor->modified = qfalse;
	}

	if (Com_RealTime(NULL) != lasttime) {
		lasttime = Com_RealTime(NULL);
		CON_UpdateClock();
		num_chars++;
	}

	while (1) {
		chr = getch();
		num_chars++;

		// Special characters
		switch (chr) {
		case ERR:
			if (num_chars > 1) {
				werase(inputwin);
				if (input_field.cursor < input_field.scroll) {
					input_field.scroll = input_field.cursor - INPUT_SCROLL;
					if (input_field.scroll < 0)
						input_field.scroll = 0;
				} else if (input_field.cursor >= input_field.scroll + input_field.widthInChars)
					input_field.scroll = input_field.cursor - input_field.widthInChars + INPUT_SCROLL;
				CON_ColorPrint(inputwin, input_field.buffer + input_field.scroll, qfalse);
#ifdef _WIN32
				wrefresh(inputwin); // If this is not done the cursor moves strangely
#else
				wnoutrefresh(inputwin);
#endif
				CON_UpdateCursor();
				doupdate();
			}
			return NULL;
		case '\n':
		case '\r':
		case KEY_ENTER:
			if (!input_field.buffer[0])
				continue;
			Hist_Add(input_field.buffer);
			strcpy(text, input_field.buffer);
			Field_Clear(&input_field);
			werase(inputwin);
			wnoutrefresh(inputwin);
			CON_UpdateCursor();
			//doupdate();
			Com_Printf(PROMPT "^7%s\n", text);
			return text;
		case '\t':
		case KEY_STAB:
			Field_AutoComplete(&input_field, PROMPT);
			input_field.cursor = strlen(input_field.buffer);
			continue;
		case '\f':
			CON_Resize();
			continue;
		case KEY_LEFT:
			if (input_field.cursor > 0)
				input_field.cursor--;
			continue;
		case KEY_RIGHT:
			if (input_field.cursor < strlen(input_field.buffer))
				input_field.cursor++;
			continue;
		case KEY_UP:
		{
			const char *field = Hist_Prev();
			if (field)
				strcpy( input_field.buffer, field );
			continue;
		}
		case KEY_DOWN:
		{
			const char *field = Hist_Next();
			if (field)
				strcpy( input_field.buffer, field );
			else
				Field_Clear(&input_field);
			continue;
		}
		case KEY_HOME:
			input_field.cursor = 0;
			continue;
		case KEY_END:
			input_field.cursor = strlen(input_field.buffer);
			continue;
		case KEY_NPAGE:
			if (lastline > scrollline + LOG_LINES) {
				scrollline += LOG_SCROLL;
				if (scrollline + LOG_LINES > lastline)
					scrollline = lastline - LOG_LINES;
				pnoutrefresh(logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1);
				CON_DrawScrollBar();
			}
			continue;
		case KEY_PPAGE:
			if (scrollline > 0) {
				scrollline -= LOG_SCROLL;
				if (scrollline < 0)
					scrollline = 0;
				pnoutrefresh(logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1);
				CON_DrawScrollBar();
			}
			continue;
		case '\b':
		case 127:
		case KEY_BACKSPACE:
			if (input_field.cursor <= 0)
				continue;
			input_field.cursor--;
			// Fall through
		case KEY_DC:
			if (input_field.cursor < strlen(input_field.buffer)) {
				memmove(input_field.buffer + input_field.cursor,
				        input_field.buffer + input_field.cursor + 1,
				        strlen(input_field.buffer) - input_field.cursor);
			}
			continue;
		}

		// Normal characters
		if (chr >= ' ' && chr < 256 && strlen(input_field.buffer) + 1 < sizeof(input_field.buffer)) {
			memmove(input_field.buffer + input_field.cursor + 1,
			        input_field.buffer + input_field.cursor,
			        strlen(input_field.buffer) - input_field.cursor);
			input_field.buffer[input_field.cursor] = chr;
			input_field.cursor++;
		}
	}
}

/*
==================
CON_Print
==================
*/
void CON_Print(const char *msg)
{
	int col;
	qboolean scroll = (lastline > scrollline && lastline <= scrollline + LOG_LINES);

	if (!curses_on) {
		CON_Print_tty(msg);
		return;
	}

	// Print the message in the log window
	CON_ColorPrint(logwin, msg, qtrue);
	getyx(logwin, lastline, col);
	if (col)
		lastline++;
	if (scroll) {
		scrollline = lastline - LOG_LINES;
		if (scrollline < 0)
			scrollline = 0;
		pnoutrefresh(logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1);
	}

	// Add the message to the log buffer
	if (insert + strlen(msg) >= logbuf + sizeof(logbuf)) {
		memmove(logbuf, logbuf + sizeof(logbuf) / 2, sizeof(logbuf) / 2);
		memset(logbuf + sizeof(logbuf) / 2, 0, sizeof(logbuf) / 2);
		insert -= sizeof(logbuf) / 2;
	}
	strcpy(insert, msg);
	insert += strlen(msg);

	// Update the scrollbar
	CON_DrawScrollBar();

	// Move the cursor back to the input field
	CON_UpdateCursor();
	doupdate();
}
