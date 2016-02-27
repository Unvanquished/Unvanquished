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

#include "framework/CommandSystem.h"

static const Color::Color console_color = Color::White;
static const int DEFAULT_CONSOLE_WIDTH = 78;
static const int MAX_CONSOLE_WIDTH   = 1024;

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

static const int ANIMATION_TYPE_NONE   = 0;
static const int ANIMATION_TYPE_SCROLL_DOWN = 1;
static const int ANIMATION_TYPE_FADE   = 2;
static const int ANIMATION_TYPE_BOTH   = 3;

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
Con_Clear_f
================
*/
void Con_Clear_f()
{
	consoleState.lines.clear();
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
	int argc = Cmd_Argc();

	if ( argc > 2 )
	{
		Cmd_PrintUsage("[<filename>]", nullptr);
		return;
	}

	std::string name = "condump/";
	if ( argc == 1 )
	{
		name += Cmd_Argv( 1 );
	}
	else
	{
		char timestr[32];
		time_t now = time( nullptr );
		strftime( timestr, sizeof( timestr ), "%Y%m%d-%H%M%S%z.txt",
		          localtime( &now ) );
		name += timestr;
	}

	fileHandle_t f = FS_FOpenFileWrite( name.c_str() );

	if ( !f )
	{
		Log::Warn("couldn't open." );
		return;
	}

	Log::Notice( "Dumped console text to %s.\n", name.c_str() );

	// write the remaining lines
	for ( const std::string& line : consoleState.lines )
	{
		std::string uncolored_line = Color::StripColors( line )  + '\n';
		FS_Write( uncolored_line.data(), uncolored_line.size(), f );
	}

	FS_FCloseFile( f );
}

template<class Iterator>
static Iterator Con_Search_f_Helper( const Iterator& begin, const Iterator& end,
								 const std::vector<std::string>& search_terms )
{
	return std::find_if( begin, end, [&search_terms]( const std::string& line )
	{
		std::string stripped = Color::StripColors( line );
		for ( const std::string& term : search_terms )
		{
			if ( stripped.find( term ) != std::string::npos )
			{
				return true;
			}
		}
		return false;
	});
}

/*
================
Con_Search_f

Scroll up to the first console line containing a string
================
*/
void Con_Search_f()
{
	const Cmd::Args& args = Cmd::GetCurrentArgs();

	if ( args.size() < 2 )
	{
		Cmd_PrintUsage("<string>…", nullptr);
		return;
	}

	std::vector<std::string> search_terms ( args.ArgVector().begin()+1, args.ArgVector().end() );

	if ( args[0] == "searchDown" )
	{
		auto found = Con_Search_f_Helper(
			consoleState.lines.begin() + consoleState.scrollLineIndex + 1,
			consoleState.lines.end(),
			search_terms );
		if ( found != consoleState.lines.end() )
		{
			consoleState.scrollLineIndex = found - consoleState.lines.begin();
		}
	}
	else
	{
		auto found = Con_Search_f_Helper(
			std::vector<std::string>::reverse_iterator( consoleState.lines.begin() + consoleState.scrollLineIndex ),
			consoleState.lines.rend(),
			search_terms );
		if ( found != consoleState.lines.rend() )
		{
			consoleState.scrollLineIndex = found.base() - consoleState.lines.begin() - 1;
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
	const Cmd::Args& args = Cmd::GetCurrentArgs();

	if ( args.size() != 2 )
	{
		Cmd_PrintUsage("<string>", nullptr);
		return;
	}

	std::vector<std::string> search = { args[1] };

	auto it = consoleState.lines.begin();
	while ( true )
	{
		it = Con_Search_f_Helper( it, consoleState.lines.end(), search );
		if ( it == consoleState.lines.end() )
		{
			break;
		}

		// print out in chunks so we don't go over the MAXPRINTMSG limit
		for (unsigned i = 0; i < it->size(); i += MAXPRINTMSG - 1 )
		{
			unsigned sub_size = std::min<unsigned>( it->size() - i, MAXPRINTMSG - 1 );
			std::string substring = it->substr(i, sub_size);
			Log::Notice( "%s", substring.c_str() );
		}

		++it;
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
	int textWidthInChars = 0;

	bool  ret = true;

	if ( cls.glconfig.vidWidth )
	{
		int consoleVidWidth = cls.glconfig.vidWidth - 2 * (consoleState.margin.sides + consoleState.padding.sides );
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
		consoleState.lines.clear();
		consoleState.bottomDisplayedLine = 0;
		consoleState.scrollLineIndex = 0;

		ret = false;
	}
	else
	{
		int oldwidth = consoleState.textWidthInChars;
		consoleState.textWidthInChars = textWidthInChars;

		if ( !consoleState.lines.empty() && oldwidth > 0 )
		{
			std::vector<std::string> old_lines;
			std::swap( old_lines, consoleState.lines );
			consoleState.lines.reserve( old_lines.size() );

			// Copy the lines, wrapping them when needed
			for ( const std::string& line : old_lines )
			{
				// Quick case for empty lines
				if ( line.empty() )
				{
					consoleState.lines.emplace_back();
					continue;
				}

				// Count the number of visible characters, adding a new line when too long
				const char* begin = line.c_str();
				int len = 0;
				for ( const auto& token : Color::Parser( line.c_str() ) )
				{
					if ( token.Type() == Color::Token::TokenType::CHARACTER || token.Type() == Color::Token::TokenType::ESCAPE )
					{
						len++;
					}
					if ( len > consoleState.textWidthInChars )
					{
						consoleState.lines.emplace_back( begin, token.Begin() );
						begin = token.Begin();
						len = 0;
					}
				}
				// add the remainder of the line
				consoleState.lines.emplace_back( begin, (&line.back()) + 1 );
			}
		}
		else
		{
			consoleState.lines.clear();
		}

		consoleState.bottomDisplayedLine = consoleState.lines.size() - 1;
		consoleState.scrollLineIndex = consoleState.lines.size() - 1;
	}

	if ( con_prompt )
	{
		// 8 spaces for clock, 1 for cursor
		g_console_field_width = consoleState.textWidthInChars - 9 - Color::StrlenNocolor( con_prompt->string );
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
	const int scrollLockMode = con_scrollLock ? con_scrollLock->integer : 0;

	//fall down to input line if scroll lock is configured to do so
	if(scrollLockMode <= 0)
	{
		consoleState.scrollLineIndex = consoleState.lines.size()-1;
	}

	//dont scroll a line down on a scrollock configured to do so
	if ( consoleState.scrollLineIndex >= consoleState.lines.size() - 1 && scrollLockMode < 3 )
	{
		consoleState.scrollLineIndex++;
	}

	consoleState.lines.emplace_back();
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

	Con_Linefeed();

	for ( const auto& token : Color::Parser( text ) )
	{
		if ( token.Type() == Color::Token::TokenType::COLOR )
		{
			consoleState.lines.back().append( token.Begin(), token.Size() );
			continue;
		}

		if ( !wordLen )
		{
			// count word length
			for ( const auto& wordtoken : Color::Parser( token.Begin() ) )
			{
				if ( wordtoken.Type() == Color::Token::TokenType::ESCAPE )
				{
					wordLen++;
				}
				else if ( wordtoken.Type() == Color::Token::TokenType::CHARACTER )
				{
					if ( Str::cisspace( *wordtoken.Begin() ) )
					{
						break;
					}
					wordLen++;
				}
			}

			// word wrap
			if ( consoleState.lines.back().size() + wordLen >= consoleState.textWidthInChars )
			{
				Con_Linefeed();
			}
		}

		switch ( *token.Begin() )
		{
			case '\n':
				Con_Linefeed();
				break;

			default: // display character and advance
				consoleState.lines.back().append( token.Begin(), token.Size() );
				if ( wordLen > 0 )
				{
					--wordLen;
				}

				if ( consoleState.lines.back().size() >= consoleState.textWidthInChars )
				{
					Con_Linefeed();
				}

				break;
		}

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
	const int consoleWidth = cls.glconfig.vidWidth - 2 * consoleState.margin.sides;

	// draw the background
	Color::Color color (
		con_colorRed->value,
		con_colorGreen->value,
		con_colorBlue->value,
		con_colorAlpha->value * consoleState.currentAlphaFactor
	);

	SCR_FillRect( consoleState.margin.sides, consoleState.margin.top, consoleWidth, consoleState.height, color );

	// draw the backgrounds borders
	Color::Color borderColor (
		con_borderColorRed->value,
		con_borderColorGreen->value,
		con_borderColorBlue->value,
		con_borderColorAlpha->value * consoleState.currentAlphaFactor
	);

	if ( con_margin->integer )
	{
		//top border
		SCR_FillRect( consoleState.margin.sides - consoleState.border.sides,
		              consoleState.margin.top - consoleState.border.top,
		              consoleWidth + consoleState.border.sides, consoleState.border.top, borderColor );
		//left border
		SCR_FillRect( consoleState.margin.sides - consoleState.border.sides, consoleState.margin.top,
		              consoleState.border.sides, consoleState.height + consoleState.border.bottom, borderColor );

		//right border
		SCR_FillRect( cls.glconfig.vidWidth - consoleState.margin.sides, consoleState.margin.top - consoleState.border.top,
		              consoleState.border.sides, consoleState.border.top + consoleState.height, borderColor );

		//bottom border
		SCR_FillRect( consoleState.margin.sides, consoleState.height + consoleState.margin.top + consoleState.border.top - consoleState.border.bottom,
		              consoleWidth + consoleState.border.sides, consoleState.border.bottom, borderColor );
	}
	else
	{
		//bottom border
		SCR_FillRect( 0, consoleState.height, consoleWidth, consoleState.border.bottom, borderColor );
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
	qtime_t realtime;

	Com_RealTime( &realtime );
	Com_sprintf( prompt,  sizeof( prompt ), "^0[^3%02d%c%02d^0]^7 %s", realtime.tm_hour, ( realtime.tm_sec & 1 ) ? ':' : ' ', realtime.tm_min, con_prompt->string );

	Color::Color color = console_color;
	color.SetAlpha( consoleState.currentAlphaFactor * overrideAlpha );

	SCR_DrawSmallStringExt( consoleState.margin.sides + consoleState.padding.sides, linePosition, prompt, color, false, false );

	Color::StripColors( prompt );
	Field_Draw( g_consoleField,
		consoleState.margin.sides + consoleState.padding.sides + SCR_ConsoleFontStringWidth( prompt, strlen( prompt ) ),
		linePosition, true, true, color.Alpha() );
}

void Con_DrawRightFloatingTextLine( const int linePosition, const Color::Color& color, const char* text )
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
	Color::Color color = Color::White;

	// draw the version number
	//ANIMATION_TYPE_FADE but also ANIMATION_TYPE_SCROLL_DOWN needs this, latter, since it might otherwise scroll out the console
	color.SetAlpha( 0.66f * consoleState.currentAnimationFraction );

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
	// draw arrows to show the buffer is backscrolled
	const int hatWidth = SCR_ConsoleFontUnicharWidth( '^' );

	Color::Color color = Color::White;
	color.SetAlpha( 0.66f * consoleState.currentAlphaFactor );
	re.SetColor( color );

	for ( i = 0; i < consoleState.textWidthInChars; i += 4 )
	{
		SCR_DrawConsoleFontUnichar( consoleState.margin.sides + consoleState.padding.sides + ( i + 1.5 ) * hatWidth, lineDrawPosition, '^' );
	}
}

void Con_DrawConsoleScrollbar()
{
	const int	freeConsoleHeight = consoleState.height - consoleState.padding.top - consoleState.padding.bottom;
	const float scrollBarX = cls.glconfig.vidWidth - consoleState.margin.sides - consoleState.padding.sides - 2 * consoleState.border.sides;
	const float scrollBarY = consoleState.margin.top + consoleState.border.top + consoleState.padding.top + freeConsoleHeight * 0.10f;
	const float scrollBarLength = freeConsoleHeight * 0.80f;
	const float scrollBarWidth = consoleState.border.sides * 2;

	const float scrollHandleLength = !consoleState.lines.empty()
	                                 ? scrollBarLength * std::min( 1.0f, (float) consoleState.visibleAmountOfLines / consoleState.lines.size() )
	                                 : 0;

	const float scrollBarLengthPerLine = ( scrollBarLength - scrollHandleLength ) / ( consoleState.lines.size() - consoleState.visibleAmountOfLines );
	// that may result in -NaN

	const float relativeScrollLineIndex = std::min( consoleState.visibleAmountOfLines, (int) consoleState.lines.size() );

	const float scrollHandlePostition = ( scrollBarLengthPerLine == scrollBarLengthPerLine )
	                                  ? scrollBarLengthPerLine * ( consoleState.bottomDisplayedLine - relativeScrollLineIndex )
	                                  : 0; // we may get this: +/- NaN is never equal to itself

	//draw the scrollBar
	Color::Color color ( 0.2f, 0.2f, 0.2f, 0.75f * consoleState.currentAlphaFactor );

	SCR_FillRect( scrollBarX, scrollBarY, scrollBarWidth, scrollBarLength, color );

	//draw the handle
	if ( scrollHandlePostition >= 0 && scrollHandleLength > 0 )
	{
		color = Color::Color( 0.5f, 0.5f, 0.5f, consoleState.currentAlphaFactor );

		SCR_FillRect( scrollBarX, scrollBarY + scrollHandlePostition, scrollBarWidth, scrollHandleLength, color );
	}
	else if ( !consoleState.lines.empty() ) //this happens when line appending gets us over the top position in a roll-lock situation (scrolling itself won't do that)
	{
		color = Color::Color(
			(-scrollHandlePostition * 5.0f)/10,
			0.5f,
			0.5f,
			consoleState.currentAlphaFactor
		);

		SCR_FillRect( scrollBarX, scrollBarY, scrollBarWidth, scrollHandleLength, color );
	}

	if(con_debug->integer) {
		Con_DrawRightFloatingTextLine( 6, Color::White, va( "Scrollbar (px): Size %d HandleSize %d Position %d", (int) scrollBarLength, (int) scrollHandleLength, (int) scrollHandlePostition ) );
	}
}

void Con_DrawScrollbackMarkerline( int lineDrawPosition )
{
	//to not run into the scrollbar
	const int scrollBarImpliedPadding = 2 * consoleState.padding.sides + 2 * consoleState.border.sides;

	Color::Color color( 0.8f, 0.0f, 0.0f, 0.5f * consoleState.currentAlphaFactor );

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
	float  lineDrawPosition, lineDrawLowestPosition;
	int    row;

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
		Con_DrawRightFloatingTextLine( 3, Color::White, va( "Buffer (lines): ScrollbackLength %d", (int) consoleState.lines.size() ) );
		Con_DrawRightFloatingTextLine( 4, Color::White, va( "Display (lines): From %d to %d (%d a %i px)",
			0, consoleState.scrollLineIndex, consoleState.visibleAmountOfLines, charHeight ) );
	}

	/*
	 * if we scrolled back, give feedback,
	 * unless it's the last line (which will be rendered partly transparent anyway)
	 * so that we dont indicate scrollback each time a single line gets added
	 */
	if ( floor( consoleState.bottomDisplayedLine ) < consoleState.lines.size() - 1 )
	{
		// draw arrows to show the buffer is backscrolled
		Con_DrawConsoleScrollbackIndicator( lineDrawPosition );
	}

	lineDrawPosition -= charHeight;

	row = consoleState.bottomDisplayedLine;

	lineDrawLowestPosition = lineDrawPosition;

	if ( consoleState.bottomDisplayedLine - floor( consoleState.bottomDisplayedLine ) != 0.0f )
	{
		lineDrawPosition += charHeight - ( consoleState.bottomDisplayedLine - floor( consoleState.bottomDisplayedLine ) ) * charHeight;
		++row;
	}


	Color::Color console_color_alpha = console_color;

	for ( ; row >= 0 && lineDrawPosition > textDistanceToTop; lineDrawPosition -= charHeight, row-- )
	{
		if ( row == consoleState.lastReadLineIndex - 1
			&& consoleState.lastReadLineIndex != consoleState.lines.size() - 1 )
		{
			Con_DrawScrollbackMarkerline( lineDrawPosition );
		}
		currentWidthLocation = consoleState.margin.sides + consoleState.padding.sides;

		console_color_alpha.SetAlpha(
			Con_MarginFadeAlpha( consoleState.currentAlphaFactor, lineDrawPosition, textDistanceToTop, lineDrawLowestPosition, charHeight )
		);

		re.SetColor( console_color_alpha );

        for ( const auto& token : Color::Parser( consoleState.lines[row].c_str(), console_color_alpha ) )
		{
			if ( token.Type() == Color::Token::TokenType::COLOR )
			{
				Color::Color color = token.Color();
				color.SetAlpha( console_color_alpha.Alpha() );
				re.SetColor( color );
			}
			else if ( token.Type() == Color::Token::TokenType::CHARACTER )
			{
				int ch = Q_UTF8_CodePoint( token.Begin() );
				SCR_DrawConsoleFontUnichar( currentWidthLocation, floor( lineDrawPosition + 0.5 ), ch );
				currentWidthLocation += SCR_ConsoleFontUnicharWidth( ch );
			}
			else if ( token.Type() == Color::Token::TokenType::ESCAPE )
			{
				int ch = Color::Constants::ESCAPE;
				SCR_DrawConsoleFontUnichar( currentWidthLocation, floor( lineDrawPosition + 0.5 ), ch );
				currentWidthLocation += SCR_ConsoleFontUnicharWidth( ch );
			}
		}
	}

	Con_DrawConsoleScrollbar( );

	re.SetColor( console_color_alpha ); //set back to white
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
			Con_DrawRightFloatingTextLine( 8, Color::White, va( "Animation: target %d current fraction %f alpha %f", (int) consoleState.isOpened, consoleState.currentAnimationFraction, consoleState.currentAlphaFactor) );
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
		&& !( cls.state == connstate_t::CA_DISCONNECTED && !( cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) ) ) )
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
			consoleState.lastReadLineIndex = consoleState.lines.size()-1;
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
	if(consoleState.lines.size() < consoleState.visibleAmountOfLines)
		return;

	consoleState.scrollLineIndex -= lines;

	if ( consoleState.scrollLineIndex <= consoleState.visibleAmountOfLines )
	{
		Con_ScrollToTop( );
	}
}

void Con_ScrollDown( int lines )
{
	consoleState.scrollLineIndex += lines;

	if ( consoleState.scrollLineIndex >= consoleState.lines.size() )
	{
		consoleState.scrollLineIndex = consoleState.lines.size() - 1;
	}
}

void Con_ScrollToMarkerLine()
{
	consoleState.scrollLineIndex = std::min(
		consoleState.lastReadLineIndex + consoleState.visibleAmountOfLines,
		(int)consoleState.lines.size() - 1 );
	//consoleState.bottomDisplayedLine = consoleState.scrollLineIndex;
}

void Con_ScrollToTop()
{
	consoleState.scrollLineIndex = consoleState.visibleAmountOfLines - 1;
	//consoleState.bottomDisplayedLine = consoleState.scrollLineIndex;
}

void Con_ScrollToBottom()
{
	consoleState.scrollLineIndex = consoleState.lines.size() - 1;
	//consoleState.bottomDisplayedLine = consoleState.currentLine;
}

void Con_JumpUp()
{
	if ( consoleState.lastReadLineIndex &&
		 consoleState.scrollLineIndex > consoleState.lastReadLineIndex + std::min(consoleState.visibleAmountOfLines, (int)consoleState.lines.size() - 1)
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
