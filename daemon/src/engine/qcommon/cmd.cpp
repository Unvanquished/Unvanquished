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

// cmd_c -- Quake script command processing module

#include "qcommon/q_shared.h"
#include "qcommon.h"
#include "client/keys.h"

#include "framework/CommandSystem.h"

static const int MAX_CMD_BUFFER = 131072;

struct cmdContext_t
{
	int  argc;
	char *argv[ MAX_STRING_TOKENS ]; // points into cmd.tokenized
	char tokenized[ BIG_INFO_STRING + MAX_STRING_TOKENS ]; // will have 0 bytes inserted
	char cmd[ BIG_INFO_STRING ]; // the original command we received (no token processing)
};

static cmdContext_t cmd;

/*
=============================================================================

                                        COMMAND EXECUTION

=============================================================================
*/

/*
============
Cmd_SaveCmdContext
============
*/
void Cmd_SaveCmdContext()
{
	Cmd::SaveArgs();
}

/*
============
Cmd_RestoreCmdContext
============
*/
void Cmd_RestoreCmdContext()
{
	Cmd::LoadArgs();
}

/*
============
Cmd_PrintUsage
============
*/
void Cmd_PrintUsage( const char *syntax, const char *description )
{
	if(!description)
	{
		Com_Printf( "%s: %s %s\n", "usage", Cmd_Argv( 0 ), syntax );
	}
	else
	{
		Com_Printf( "%s: %s %s — %s\n", "usage",  Cmd_Argv( 0 ), syntax, description );
	}
}

/*
============
Cmd_Argc
============
*/
int Cmd_Argc()
{
	const Cmd::Args& args = Cmd::GetCurrentArgs();
	return args.Argc();
}

/*
============
Cmd_Argv
============
*/
const char *Cmd_Argv( int arg )
{
	if (arg >= Cmd_Argc() || arg < 0) {
		return "";
	}

	const Cmd::Args& args = Cmd::GetCurrentArgs();
	const std::string& res = args.Argv(arg);
	static char buffer[100][1024];

	Q_strncpyz(buffer[arg], res.c_str(), sizeof(buffer[arg]));

	return buffer[arg];
}

/*
============
Cmd_Args

Returns a single string containing argv(arg) to argv(argc()-1)
============
*/
char *Cmd_ArgsFrom( int arg )
{
	static char cmd_args[ BIG_INFO_STRING ];

	const Cmd::Args& args = Cmd::GetCurrentArgs();
	const std::string& res = args.EscapedArgs(arg);
	Q_strncpyz(cmd_args, res.c_str(), BIG_INFO_STRING);

	return cmd_args;
}

/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char           *Cmd_Args()
{
	return Cmd_ArgsFrom( 1 );
}

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
The text is copied to a separate buffer and 0 characters
are inserted in the appropriate place. The argv array
will point into this temporary buffer.
============
*/
static void Tokenise( const char *text, char *textOut, bool tokens, bool ignoreQuotes )
{
	// Assumption:
	// if ( tokens ) cmd.argc == 0 && textOut == cmd.tokenized
	// textOut points to a large buffer
	// text which is processed here is already of limited length...
	char *textOutStart = textOut;

	*textOut = '\0'; // initial NUL-termination in case of early exit

	while ( 1 )
	{
		if ( tokens && cmd.argc == MAX_STRING_TOKENS )
		{
			goto done; // this is usually something malicious
		}

		while ( 1 )
		{
			// skip whitespace
			while ( *text > '\0' && *text <= ' ' )
			{
				text++;
			}

			if ( !*text )
			{
				goto done; // all tokens parsed
			}

			// skip // comments
			if ( text[ 0 ] == '/' && text[ 1 ] == '/' )
			{
				goto done; // all tokens parsed
			}

			// skip /* */ comments
			if ( text[ 0 ] == '/' && text[ 1 ] == '*' )
			{
				// two increments, avoiding matching /*/
				++text;
				while ( *++text && !( text[ 0 ] == '*' && text[ 1 ] == '/' ) ) {}

				if ( !*text )
				{
					goto done; // all tokens parsed
				}

				text += 2;
			}
			else
			{
				break; // we are ready to parse a token
			}
		}

		// regular token
		if ( tokens )
		{
			cmd.argv[ cmd.argc ] = textOut;
			cmd.argc++;
		}

		while ( *text < 0 || *text > ' ' )
		{
			if ( ignoreQuotes || text[ 0 ] != '"' )
			{
				// copy until next space, quote or EOT, handling backslashes
				while ( *text < 0 || ( *text > ' ' && *text != '"' ) )
				{
					if ( *text == '\\' && (++text, (*text >= 0 && *text < ' ') ) )
					{
						break;
					}

					*textOut++ = *text++;
				}
			}
			else
			{
				// quoted string :-)
				// copy until next " or EOT, handling backslashes
				// missing trailing " is fine
				++text;

				while ( *text && *text != '"' )
				{
					if ( *text == '\\' && (++text, (*text >= 0 && *text < ' ') ) )
					{
						break;
					}

					*textOut++ = *text++;
				}

				if ( *text ) ++text; // terminating ", if any
			}
		}

		*textOut++ = tokens ? '\0' : ' ';

		if ( !*text )
		{
			goto done; // all tokens parsed
		}
	}

	done:

	if ( textOut > textOutStart )
	{
		// will be NUL-terminated if in token mode
		// otherwise there'll be a space there...
		*--textOut = '\0';
	}
}

/*
============
Cmd_TokenizeString
============
*/
void Cmd_TokenizeString( const char *text_in )
{
	Cmd::Args args(text_in);
	Cmd::SetCurrentArgs(args);
}

/*
============
EscapeString

Internal.
Escape all '\' '"' '$', '/' if marking a comment, and ';' if not in quotation marks.
Optionally wrap in ""
============
*/
static const int ESCAPEBUFFER_SIZE = BIG_INFO_STRING;
static char *GetEscapeBuffer()
{
	static char escapeBuffer[ 4 ][ ESCAPEBUFFER_SIZE ];
	static int escapeIndex = -1;

	escapeIndex = ( escapeIndex + 1 ) & 3;
	return escapeBuffer[ escapeIndex ];
}

static const char *EscapeString( const char *in, bool quote )
{
	char        *escapeBuffer = GetEscapeBuffer();
	char        *out = escapeBuffer;
	const char  *end = escapeBuffer + ESCAPEBUFFER_SIZE - 1 - !!quote;
	bool    quoted = false;
	bool    forcequote = false;

	if ( quote )
	{
		*out++ = '"';
	}

	while ( *in && out < end )
	{
		char c = *in++;

		if ( forcequote )
		{
			forcequote = false;
			goto doquote;
		}

		switch ( c )
		{
		case '/':
			// only quote "//" and "/*"
			if ( *in != '/' && *in != '*' ) break;
			forcequote = true;
			goto doquote;
		case ';':
			// no need to quote semicolons if in ""
			quoted = true;
			if ( quote ) break;
		case '"':
		case '$':
		case '\\':
			doquote:
			quoted = true;
			*out++ = '\\'; // could set out == end - is fine
			break;
		}

		// keep quotes if we have white space
		if ( c >= '\0' && c <= ' ' ) quoted = true;

		// if out == end, overrun (but within escapeBuffer)
		*out++ = c;
	}

	if ( out > end )
	{
		out -= 2; // an escape overran; allow overwrite
	}

	if ( quote )
	{
		// we have an empty string or something was escaped
		if ( quoted || out == escapeBuffer + 1 )
		{
			*out++ = '"';
		}
		else
		{
			memmove( escapeBuffer, escapeBuffer + 1, --out - escapeBuffer );
		}
	}

	*out = '\0';
	return escapeBuffer;
}

/*
============
Cmd_QuoteString

Adds escapes (see above), wraps in quotation marks.
============
*/
const char *Cmd_QuoteString( const char *in )
{
	return EscapeString( in, true );
}

/*
============
Cmd_QuoteStringBuffer

Cmd_QuoteString for VM usage
============
*/
void Cmd_QuoteStringBuffer( const char *in, char *buffer, int size )
{
	Q_strncpyz( buffer, Cmd_QuoteString( in ), size );
}

/*
===================
Cmd_UnquoteString

Usually passed a raw partial command string
String length is UNCHECKED
===================
*/
const char *Cmd_UnquoteString( const char *str )
{
	char *escapeBuffer = GetEscapeBuffer();
	Tokenise( str, escapeBuffer, false, false );
	return escapeBuffer;
}

struct proxyInfo_t{
	xcommand_t cmd;
	completionFunc_t complete;
};

//Contains the commands given through the C interface
std::unordered_map<std::string, proxyInfo_t, Str::IHash, Str::IEqual> proxies;

Cmd::CompletionResult completeMatches;
std::string completedPrefix;

//Is registered in the new command system for all the commands registered through the C interface.
class ProxyCmd: public Cmd::CmdBase {
	public:
		ProxyCmd(): Cmd::CmdBase(Cmd::PROXY_FOR_OLD) {}

		void Run(const Cmd::Args& args) const OVERRIDE {
			proxyInfo_t proxy = proxies[args.Argv(0)];
			proxy.cmd();
		}

		Cmd::CompletionResult Complete(int argNum, const Cmd::Args& args, Str::StringRef prefix) const OVERRIDE {
			static char buffer[4096];
			proxyInfo_t proxy = proxies[args.Argv(0)];

			if (!proxy.complete) {
				return {};
			}
			completedPrefix = prefix;
			completeMatches.clear();

			//Completing an empty arg, we add a space to mimic the old autocompletion behavior
			if (args.Argc() == argNum) {
				Q_strncpyz(buffer, (args.ConcatArgs(0) + " ").c_str(), sizeof(buffer));
			} else {
				Q_strncpyz(buffer, args.ConcatArgs(0).c_str(), sizeof(buffer));
			}

			//Some completion handlers expect tokenized arguments
			Cmd::Args savedArgs = Cmd::GetCurrentArgs();
			Cmd::SetCurrentArgs(args);

			proxy.complete(buffer, argNum + 1);

			Cmd::SetCurrentArgs(savedArgs);

			return completeMatches;
		}
};

ProxyCmd myProxyCmd;

void Cmd_OnCompleteMatch(const char* s) {
    if (Str::IsIPrefix(completedPrefix, s)) {
        completeMatches.push_back({s, ""});
    }
}
/*
============
Cmd_AddCommand
============
*/
void Cmd_AddCommand( const char *cmd_name, xcommand_t function )
{
	proxies[cmd_name] = proxyInfo_t{function, nullptr};

	//VMs do not properly clean up commands so we avoid creating a command if there is already one
	if (not Cmd::CommandExists(cmd_name)) {
		Cmd::AddCommand(cmd_name, myProxyCmd, "--");
	}
}

/*
============
Cmd_SetCommandCompletionFunc
============
*/
void Cmd_SetCommandCompletionFunc( const char *command, completionFunc_t complete )
{
	proxies[command].complete = complete;
}

/*
============
Cmd_RemoveCommand
============
*/
void Cmd_RemoveCommand( const char *cmd_name )
{
	proxies.erase(cmd_name);
	Cmd::RemoveCommand(cmd_name);
}
