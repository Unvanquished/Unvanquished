/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

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
#include "qcommon.h"

//script flags
const int SCFL_NOERRORS            = 0x0001;
const int SCFL_NOWARNINGS          = 0x0002;
const int SCFL_NOSTRINGWHITESPACES = 0x0004;
const int SCFL_NOSTRINGESCAPECHARS = 0x0008;
const int SCFL_PRIMITIVE           = 0x0010;
const int SCFL_NOBINARYNUMBERS     = 0x0020;
const int SCFL_NONUMBERVALUES      = 0x0040;

//string sub type
//---------------
//    the length of the string
//literal sub type
//----------------
//    the ASCII code of the literal
//number sub type
//---------------
const int TT_DECIMAL         = 0x0008;
const int TT_HEX             = 0x0100;
const int TT_OCTAL           = 0x0200;
const int TT_BINARY          = 0x0400;
const int TT_FLOAT           = 0x0800;
const int TT_INTEGER         = 0x1000;
const int TT_LONG            = 0x2000;
const int TT_UNSIGNED        = 0x4000;
//punctuation sub type
//--------------------
const int P_RSHIFT_ASSIGN    = 1;
const int P_LSHIFT_ASSIGN    = 2;
const int P_PARMS            = 3;
const int P_PRECOMPMERGE     = 4;

const int P_LOGIC_AND        = 5;
const int P_LOGIC_OR         = 6;
const int P_LOGIC_GEQ        = 7;
const int P_LOGIC_LEQ        = 8;
const int P_LOGIC_EQ         = 9;
const int P_LOGIC_UNEQ       = 10;

const int P_MUL_ASSIGN       = 11;
const int P_DIV_ASSIGN       = 12;
const int P_MOD_ASSIGN       = 13;
const int P_ADD_ASSIGN       = 14;
const int P_SUB_ASSIGN       = 15;
const int P_INC              = 16;
const int P_DEC              = 17;

const int P_BIN_AND_ASSIGN   = 18;
const int P_BIN_OR_ASSIGN    = 19;
const int P_BIN_XOR_ASSIGN   = 20;
const int P_RSHIFT           = 21;
const int P_LSHIFT           = 22;

const int P_POINTERREF       = 23;
const int P_CPP1             = 24;
const int P_CPP2             = 25;
const int P_MUL              = 26;
const int P_DIV              = 27;
const int P_MOD              = 28;
const int P_ADD              = 29;
const int P_SUB              = 30;
const int P_ASSIGN           = 31;

const int P_BIN_AND          = 32;
const int P_BIN_OR           = 33;
const int P_BIN_XOR          = 34;
const int P_BIN_NOT          = 35;

const int P_LOGIC_NOT        = 36;
const int P_LOGIC_GREATER    = 37;
const int P_LOGIC_LESS       = 38;

const int P_REF              = 39;
const int P_COMMA            = 40;
const int P_SEMICOLON        = 41;
const int P_COLON            = 42;
const int P_QUESTIONMARK     = 43;

const int P_PARENTHESESOPEN  = 44;
const int P_PARENTHESESCLOSE = 45;
const int P_BRACEOPEN        = 46;
const int P_BRACECLOSE       = 47;
const int P_SQBRACKETOPEN    = 48;
const int P_SQBRACKETCLOSE   = 49;
const int P_BACKSLASH        = 50;

const int P_PRECOMP          = 51;
const int P_DOLLAR           = 52;

//name sub type
//-------------
//    the length of the name

//punctuation
struct punctuation_t
{
	const char           *p; //punctuation character(s)
	int                  n; //punctuation indication
	punctuation_t        *next; //next punctuation
};

//token
// value type, held pointers are not owned.
struct token_t
{
	char              string[MAX_TOKEN_CHARS]; //available token
	int               type; //last read token type
	int               subtype; //last read token sub type
	unsigned long int intvalue; //integer value
	double            floatvalue; //floating point value
	char              *whitespace_p; //start of white space before token
	char              *endwhitespace_p; //start of white space before token
	int               line; //line the token was on
	int               linescrossed; //lines crossed in white space
	token_t           *next; //next token in chain

	token_t();
	inline size_t size() const
	{
		return strlen(this->string);
	}
	inline const char* c_str() const
	{
		return this->string;
	}
	inline Str::StringRef sref() const
	{
		return Str::StringRef(this->string);
	}
	inline bool is(Str::StringRef other) const
	{
		return other.compare(string) == 0;
	}
	inline bool isNot(Str::StringRef other) const
	{
		return other.compare(string) != 0;
	}
	inline bool startsWith(char other) const
	{
		return *this->string == other;
	}
	inline bool startsWith(Str::StringRef other) const
	{
		return Str::IsPrefix(this->string, other);
	}
	inline void setText(Str::StringRef text)
	{
		assert(text.size() < MAX_TOKEN_CHARS);
		size_t n = std::min(size_t(MAX_TOKEN_CHARS - 1), text.size());
		strncpy(this->string, text.c_str(), n);
		this->string[n] = '\0';
	}
	inline void clearText()
	{
		this->string[0] = '\0';
	}
	void clear()
	{
		string[0] = '\0';
		type = 0;
		subtype = 0;
		intvalue = 0;
		floatvalue = 0.0;
		whitespace_p = nullptr;
		endwhitespace_p = nullptr;
		line = 0;
		linescrossed = 0;
		next = nullptr;
	}
};

token_t::token_t()
{
	clear();
}

//script file
struct script_t
{
	char            filename[1024]; //file name of the script
	char            *buffer; //buffer containing the script
	char            *script_p; //current pointer in the script
	char            *end_p; //pointer to the end of the script
	char            *lastscript_p; //script pointer before reading token
	char            *whitespace_p; //begin of the white space
	char            *endwhitespace_p;
	int             length; //length of the script in bytes
	int             line; //current line in script
	int             lastline; //line before reading token
	int             tokenavailable; //set by UnreadLastToken
	int             flags; //several script flags
	punctuation_t   *punctuations; //the punctuations used in the script
	punctuation_t   **punctuationtable;
	token_t         token; //available token
	script_t        *next; //next script in a chain
};

const int DEFINE_FIXED = 0x0001;

const int BUILTIN_LINE = 1;
const int BUILTIN_FILE = 2;
const int BUILTIN_DATE = 3;
const int BUILTIN_TIME = 4;
const int BUILTIN_STDC = 5;

const int INDENT_IF    = 0x0001;
const int INDENT_ELSE  = 0x0002;
const int INDENT_ELIF  = 0x0004;
const int INDENT_IFDEF = 0x0008;
const int INDENT_IFNDEF= 0x0010;

//macro definitions
struct define_t
{
	char            *name; //define name
	int             flags; //define flags
	int             builtin; // > 0 if builtin define
	int             numparms; //number of define parameters
	token_t         *parms; //define parameters
	token_t         *tokens; //macro tokens (possibly containing parm tokens)
	define_t        *next; //next defined macro in a list

	define_t        *hashnext; //next define in the hash chain
};

//indents
//used for conditional compilation directives:
//#if, #else, #elif, #ifdef, #ifndef
struct indent_t
{
	int             type; //indent type
	int             skip; //true if skipping current indent
	script_t        *script; //script the indent was in
	indent_t        *next; //next indent on the indent stack
};

//source file
struct source_t
{
	char          filename[ MAX_QPATH ]; //file name of the script
	char          includepath[ MAX_QPATH ]; //path to include files
	punctuation_t *punctuations; //punctuations to use
	script_t      *scriptstack; //stack with scripts of the source
	token_t       *tokens; //tokens to read first
	define_t      *defines; //list with macro definitions
	define_t      **definehash; //hash chain with defines
	indent_t      *indentstack; //stack with indents
	int           skip; // > 0 if skipping conditional code
	token_t       token; //last read token
};

const int MAX_DEFINEPARMS = 128;

//directive name with parse function
struct directive_t
{
	const char *name;
	bool ( *func )( source_t& source );
};

const int DEFINEHASHSIZE = 1024;

static bool Parse_ReadToken( source_t& source, token_t *token );
static bool Parse_AddDefineToSourceFromString( source_t& source,
    char *string );

static int             numtokens;

//list with global defines added to every source loaded
static define_t        *globaldefines;

const int MAX_SOURCEFILES = 64;

static source_t* sourceFiles[MAX_SOURCEFILES];

//longer punctuations first
static punctuation_t   Default_Punctuations[] =
{
	//binary operators
	{ ">>=", P_RSHIFT_ASSIGN,    nullptr },
	{ "<<=", P_LSHIFT_ASSIGN,    nullptr },
	//
	{ "...", P_PARMS,            nullptr },
	//define merge operator
	{ "##", P_PRECOMPMERGE,     nullptr },
	//logic operators
	{ "&&", P_LOGIC_AND,        nullptr },
	{ "||", P_LOGIC_OR,         nullptr },
	{ ">=", P_LOGIC_GEQ,        nullptr },
	{ "<=", P_LOGIC_LEQ,        nullptr },
	{ "==", P_LOGIC_EQ,         nullptr },
	{ "!=", P_LOGIC_UNEQ,       nullptr },
	//arithmatic operators
	{ "*=", P_MUL_ASSIGN,       nullptr },
	{ "/=", P_DIV_ASSIGN,       nullptr },
	{ "%=", P_MOD_ASSIGN,       nullptr },
	{ "+=", P_ADD_ASSIGN,       nullptr },
	{ "-=", P_SUB_ASSIGN,       nullptr },
	{ "++", P_INC,              nullptr },
	{ "--", P_DEC,              nullptr },
	//binary operators
	{ "&=", P_BIN_AND_ASSIGN,   nullptr },
	{ "|=", P_BIN_OR_ASSIGN,    nullptr },
	{ "^=", P_BIN_XOR_ASSIGN,   nullptr },
	{ ">>", P_RSHIFT,           nullptr },
	{ "<<", P_LSHIFT,           nullptr },
	//reference operators
	{ "->", P_POINTERREF,       nullptr },
	//C++
	{ "::", P_CPP1,             nullptr },
	{ ".*", P_CPP2,             nullptr },
	//arithmatic operators
	{ "*",  P_MUL,              nullptr },
	{ "/",  P_DIV,              nullptr },
	{ "%",  P_MOD,              nullptr },
	{ "+",  P_ADD,              nullptr },
	{ "-",  P_SUB,              nullptr },
	{ "=",  P_ASSIGN,           nullptr },
	//binary operators
	{ "&",  P_BIN_AND,          nullptr },
	{ "|",  P_BIN_OR,           nullptr },
	{ "^",  P_BIN_XOR,          nullptr },
	{ "~",  P_BIN_NOT,          nullptr },
	//logic operators
	{ "!",  P_LOGIC_NOT,        nullptr },
	{ ">",  P_LOGIC_GREATER,    nullptr },
	{ "<",  P_LOGIC_LESS,       nullptr },
	//reference operator
	{ ".",  P_REF,              nullptr },
	//separators
	{ ",",  P_COMMA,            nullptr },
	{ ";",  P_SEMICOLON,        nullptr },
	//label indication
	{ ":",  P_COLON,            nullptr },
	//if statement
	{ "?",  P_QUESTIONMARK,     nullptr },
	//embracements
	{ "(",  P_PARENTHESESOPEN,  nullptr },
	{ ")",  P_PARENTHESESCLOSE, nullptr },
	{ "{",  P_BRACEOPEN,        nullptr },
	{ "}",  P_BRACECLOSE,       nullptr },
	{ "[",  P_SQBRACKETOPEN,    nullptr },
	{ "]",  P_SQBRACKETCLOSE,   nullptr },
	//
	{ "\\", P_BACKSLASH,        nullptr },
	//precompiler operator
	{ "#",  P_PRECOMP,          nullptr },
	{ "$",  P_DOLLAR,           nullptr },
	{ nullptr, 0 }
};

// Make sure we these types don't get complicated until we are ready.
// We malloc them, memset or memcpy them.
// When this is supported on all platforms we should enable this:
//static_assert(std::is_trivially_destructible<source_t>::value);
//static_assert(std::is_trivially_destructible<script_t>::value);
//static_assert(std::is_trivially_destructible<token_t>::value);
//static_assert(std::is_trivially_destructible<source_t>::value);

static void Parse_CheckScriptFileNameSize( const char* filename)
{
	size_t expectedSize = sizeof(script_t::filename);
	if (strlen(filename) >= expectedSize)
		throw std::runtime_error(Str::Format("Script filename longer than allowed (max: %d): %s", 
			(int) expectedSize, filename));
}

/*
===============
Parse_CreatePunctuationTable
===============
*/
static void Parse_CreatePunctuationTable( script_t& script, punctuation_t *punctuations )
{
	int           i;
	punctuation_t *p, *lastp, *newp;

	//get memory for the table
	if ( !script.punctuationtable )
	{
		script.punctuationtable = ( punctuation_t ** )
		                           Z_Malloc( 256 * sizeof( punctuation_t * ) );
		if (!script.punctuationtable)
			throw std::bad_alloc();
	}
	else // Z_Malloc returns memory already zero'd, but have to re-zero on this path.
		Com_Memset( script.punctuationtable, 0, 256 * sizeof( punctuation_t * ) );

	//add the punctuations in the list to the punctuation table
	for ( i = 0; punctuations[ i ].p; i++ )
	{
		newp = &punctuations[ i ];
		lastp = nullptr;

		//sort the punctuations in this table entry on length (longer punctuations first)
		for ( p = script.punctuationtable[( unsigned int ) newp->p[ 0 ] ]; p; p = p->next )
		{
			if ( strlen( p->p ) < strlen( newp->p ) )
			{
				newp->next = p;

				if ( lastp ) { lastp->next = newp; }
				else { script.punctuationtable[( unsigned int ) newp->p[ 0 ] ] = newp; }

				break;
			}

			lastp = p;
		}

		if ( !p )
		{
			newp->next = nullptr;

			if ( lastp ) { lastp->next = newp; }
			else { script.punctuationtable[( unsigned int ) newp->p[ 0 ] ] = newp; }
		}
	}
}

/*
===============
Parse_ScriptError
===============
*/
static void QDECL PRINTF_LIKE(2) Parse_ScriptError( script_t& script, const char *str, ... )
{
	char    text[ 1024 ];
	va_list ap;

	if ( script.flags & SCFL_NOERRORS ) { return; }

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
	Com_Printf( "file %s, line %d: %s\n", script.filename, script.line, text );
}

/*
===============
Parse_ScriptWarning
===============
*/
static void QDECL PRINTF_LIKE(2) Parse_ScriptWarning( script_t& script, const char *str, ... )
{
	char    text[ 1024 ];
	va_list ap;

	if ( script.flags & SCFL_NOWARNINGS ) { return; }

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
	Com_Printf( "file %s, line %d: %s\n", script.filename, script.line, text );
}

/*
===============
Parse_SetScriptPunctuations
===============
*/
static void Parse_SetScriptPunctuations( script_t& script, punctuation_t *p )
{
	if ( p ) { Parse_CreatePunctuationTable( script, p ); }
	else { Parse_CreatePunctuationTable( script, Default_Punctuations ); }

	if ( p ) { script.punctuations = p; }
	else { script.punctuations = Default_Punctuations; }
}

/*
===============
Parse_ReadWhiteSpace
===============
*/
static bool Parse_ReadWhiteSpace( script_t& script )
{
	while ( 1 )
	{
		//skip white space
		while ( *script.script_p <= ' ' )
		{
			if ( !*script.script_p ) { return false; }

			if ( *script.script_p == '\n' ) { script.line++; }

			script.script_p++;
		}

		//skip comments
		if ( *script.script_p == '/' )
		{
			//comments //
			if ( * ( script.script_p + 1 ) == '/' )
			{
				script.script_p++;

				do
				{
					script.script_p++;

					if ( !*script.script_p ) { return false; }
				}
				while ( *script.script_p != '\n' );

				script.line++;
				script.script_p++;

				if ( !*script.script_p ) { return false; }

				continue;
			}
			//comments /* */
			else if ( * ( script.script_p + 1 ) == '*' )
			{
				script.script_p++;

				do
				{
					script.script_p++;

					if ( !*script.script_p ) { return false; }

					if ( *script.script_p == '\n' ) { script.line++; }
				}
				while ( !( *script.script_p == '*' && * ( script.script_p + 1 ) == '/' ) );

				script.script_p++;

				if ( !*script.script_p ) { return false; }

				script.script_p++;

				if ( !*script.script_p ) { return false; }

				continue;
			}
		}

		break;
	}

	return true;
}

/*
===============
Parse_ReadEscapeCharacter
===============
*/
static bool Parse_ReadEscapeCharacter( script_t& script, char *ch )
{
	int c, val, i;

	//step over the leading '\\'
	script.script_p++;

	//determine the escape character
	switch ( *script.script_p )
	{
		case '\\':
			c = '\\';
			break;

		case 'n':
			c = '\n';
			break;

		case 'r':
			c = '\r';
			break;

		case 't':
			c = '\t';
			break;

		case 'v':
			c = '\v';
			break;

		case 'b':
			c = '\b';
			break;

		case 'f':
			c = '\f';
			break;

		case 'a':
			c = '\a';
			break;

		case '\'':
			c = '\'';
			break;

		case '\"':
			c = '\"';
			break;

		case '\?':
			c = '\?';
			break;

		case 'x':
			{
				script.script_p++;

				for ( i = 0, val = 0;; i++, script.script_p++ )
				{
					c = *script.script_p;

					if ( c >= '0' && c <= '9' ) { c = c - '0'; }
					else if ( c >= 'A' && c <= 'Z' ) { c = c - 'A' + 10; }
					else if ( c >= 'a' && c <= 'z' ) { c = c - 'a' + 10; }
					else { break; }

					val = ( val << 4 ) + c;
				}

				script.script_p--;

				if ( val > 0xFF )
				{
					Parse_ScriptWarning( script, "too large value in escape character" );
					val = 0xFF;
				}

				c = val;
				break;
			}

		default: //NOTE: decimal ASCII code, NOT octal
			{
				if ( *script.script_p < '0' || *script.script_p > '9' ) { Parse_ScriptError( script, "unknown escape char" ); }

				for ( i = 0, val = 0;; i++, script.script_p++ )
				{
					c = *script.script_p;

					if ( c >= '0' && c <= '9' ) { c = c - '0'; }
					else { break; }

					val = val * 10 + c;
				}

				script.script_p--;

				if ( val > 0xFF )
				{
					Parse_ScriptWarning( script, "too large value in escape character" );
					val = 0xFF;
				}

				c = val;
				break;
			}
	}

	//step over the escape character or the last digit of the number
	script.script_p++;
	//store the escape character
	*ch = c;
	//successfully read escape character
	return true;
}

/*
===============
Parse_ReadString

Reads C-like string. Escape characters are interpretted.
Quotes are included with the string.
Reads two strings with a white space between them as one string.
===============
*/
static bool Parse_ReadString( script_t& script, token_t *token, int quote )
{
	int  len, tmpline;
	char *tmpscript_p;

	if ( quote == '\"' ) { token->type = TT_STRING; }
	else { token->type = TT_LITERAL; }

	len = 0;
	//leading quote
	token->string[ len++ ] = *script.script_p++;

	while ( 1 )
	{
		//minus 3 because trailing double quote and zero have to be appended
		if ( len >= MAX_TOKEN_CHARS - 3 )
		{
			Parse_ScriptError( script, "string longer than MAX_TOKEN_CHARS = %d", MAX_TOKEN_CHARS );
			return false;
		}

		//if there is an escape character and
		//if escape characters inside a string are allowed
		if ( *script.script_p == '\\' && !( script.flags & SCFL_NOSTRINGESCAPECHARS ) )
		{
			if ( !Parse_ReadEscapeCharacter( script, &token->string[ len ] ) )
			{
				token->string[ len ] = 0;
				return false;
			}

			len++;
		}
		//if a trailing quote
		else if ( *script.script_p == quote )
		{
			//step over the double quote
			script.script_p++;

			//if white spaces in a string are not allowed
			if ( script.flags & SCFL_NOSTRINGWHITESPACES )
				break;

			tmpscript_p = script.script_p;
			tmpline = script.line;

			//read unusefull stuff between possible two following strings
			if ( !Parse_ReadWhiteSpace( script ) )
			{
				script.script_p = tmpscript_p;
				script.line = tmpline;
				break;
			}

			//if there's no leading double qoute
			if ( *script.script_p != quote )
			{
				script.script_p = tmpscript_p;
				script.line = tmpline;
				break;
			}

			//step over the new leading double quote
			script.script_p++;
		}
		else
		{
			if ( *script.script_p == '\0' )
			{
				token->string[ len ] = '\0';
				Parse_ScriptError( script, "missing trailing quote" );
				return false;
			}

			if ( *script.script_p == '\n' )
			{
				token->string[ len ] = '\0';
				Parse_ScriptError( script, "newline inside string %s", token->string );
				return false;
			}

			token->string[ len++ ] = *script.script_p++;
		}
	}

	//trailing quote
	token->string[ len++ ] = quote;
	//end string with a zero
	token->string[ len ] = '\0';
	//the sub type is the length of the string
	token->subtype = len;
	return true;
}

/*
===============
Parse_ReadName
===============
*/
static int Parse_ReadName( script_t& script, token_t *token )
{
	int  len = 0;
	char c;

	token->type = TT_NAME;

	do
	{
		token->string[ len++ ] = *script.script_p++;

		if ( len >= MAX_TOKEN_CHARS )
		{
			Parse_ScriptError( script, "name longer than MAX_TOKEN_CHARS = %d", MAX_TOKEN_CHARS );
			return 0;
		}

		c = *script.script_p;
	}
	while ( ( c >= 'a' && c <= 'z' ) ||
	        ( c >= 'A' && c <= 'Z' ) ||
	        ( c >= '0' && c <= '9' ) ||
	        c == '_' );

	token->string[ len ] = '\0';
	//the sub type is the length of the name
	token->subtype = len;
	return 1;
}

/*
===============
Parse_NumberValue
===============
*/
static void Parse_NumberValue( const char *string, int subtype, unsigned long int *intvalue,
                               double *floatvalue )
{
	unsigned long int dotfound = 0;

	*intvalue = 0;
	*floatvalue = 0;

	//floating point number
	if ( subtype & TT_FLOAT )
	{
		while ( *string )
		{
			if ( *string == '.' )
			{
				if ( dotfound ) { return; }

				dotfound = 10;
				string++;
			}

			if ( dotfound )
			{
				*floatvalue = *floatvalue + ( double )( *string - '0' ) /
				              ( double ) dotfound;
				dotfound *= 10;
			}
			else
			{
				*floatvalue = *floatvalue * 10.0 + ( double )( *string - '0' );
			}

			string++;
		}

		*intvalue = ( unsigned long ) * floatvalue;
	}
	else if ( subtype & TT_DECIMAL )
	{
		while ( *string ) { *intvalue = *intvalue * 10 + ( *string++ - '0' ); }

		*floatvalue = *intvalue;
	}
	else if ( subtype & TT_HEX )
	{
		//step over the leading 0x or 0X
		string += 2;

		while ( *string )
		{
			*intvalue <<= 4;

			if ( *string >= 'a' && *string <= 'f' ) { *intvalue += *string - 'a' + 10; }
			else if ( *string >= 'A' && *string <= 'F' ) { *intvalue += *string - 'A' + 10; }
			else { *intvalue += *string - '0'; }

			string++;
		}

		*floatvalue = *intvalue;
	}
	else if ( subtype & TT_OCTAL )
	{
		//step over the first zero
		string += 1;

		while ( *string ) { *intvalue = ( *intvalue << 3 ) + ( *string++ - '0' ); }

		*floatvalue = *intvalue;
	}
	else if ( subtype & TT_BINARY )
	{
		//step over the leading 0b or 0B
		string += 2;

		while ( *string ) { *intvalue = ( *intvalue << 1 ) + ( *string++ - '0' ); }

		*floatvalue = *intvalue;
	}
}

/*
===============
Parse_ReadNumber
===============
*/
static bool Parse_ReadNumber( script_t& script, token_t *token )
{
	int  len = 0, i;
	int  octal, dot;
	char c;

	token->type = TT_NUMBER;

	//check for a hexadecimal number
	if ( *script.script_p == '0' &&
	     ( * ( script.script_p + 1 ) == 'x' ||
	       * ( script.script_p + 1 ) == 'X' ) )
	{
		token->string[ len++ ] = *script.script_p++;
		token->string[ len++ ] = *script.script_p++;
		c = *script.script_p;

		//hexadecimal
		while ( ( c >= '0' && c <= '9' ) ||
		        ( c >= 'a' && c <= 'f' ) ||
		        ( c >= 'A' && c <= 'A' ) )
		{
			token->string[ len++ ] = *script.script_p++;

			if ( len >= MAX_TOKEN_CHARS )
			{
				Parse_ScriptError( script, "hexadecimal number longer than MAX_TOKEN_CHARS = %d", MAX_TOKEN_CHARS );
				return false;
			}

			c = *script.script_p;
		}

		token->subtype |= TT_HEX;
	}

#ifdef BINARYNUMBERS
	//check for a binary number
	else if ( *script.script_p == '0' &&
	          ( * ( script.script_p + 1 ) == 'b' ||
	            * ( script.script_p + 1 ) == 'B' ) )
	{
		token->string[ len++ ] = *script.script_p++;
		token->string[ len++ ] = *script.script_p++;
		c = *script.script_p;

		//binary
		while ( c == '0' || c == '1' )
		{
			token->string[ len++ ] = *script.script_p++;

			if ( len >= MAX_TOKEN_CHARS )
			{
				Parse_ScriptError( script, "binary number longer than MAX_TOKEN_CHARS = %d", MAX_TOKEN_CHARS );
				return false;
			}

			c = *script.script_p;
		}

		token->subtype |= TT_BINARY;
	}

#endif //BINARYNUMBERS
	else //decimal or octal integer or floating point number
	{
		octal = false;
		dot = false;

		if ( *script.script_p == '0' ) { octal = true; }

		while ( 1 )
		{
			c = *script.script_p;

			if ( c == '.' ) { dot = true; }
			else if ( c == '8' || c == '9' ) { octal = false; }
			else if ( c < '0' || c > '9' ) { break; }

			token->string[ len++ ] = *script.script_p++;

			if ( len >= MAX_TOKEN_CHARS - 1 )
			{
				Parse_ScriptError( script, "number longer than MAX_TOKEN_CHARS = %d", MAX_TOKEN_CHARS );
				return false;
			}
		}

		if ( octal ) { token->subtype |= TT_OCTAL; }
		else { token->subtype |= TT_DECIMAL; }

		if ( dot ) { token->subtype |= TT_FLOAT; }
	}

	for ( i = 0; i < 2; i++ )
	{
		c = *script.script_p;

		//check for a LONG number
		if ( ( c == 'l' || c == 'L' )
		     && !( token->subtype & TT_LONG ) )
		{
			script.script_p++;
			token->subtype |= TT_LONG;
		}
		//check for an UNSIGNED number
		else if ( ( c == 'u' || c == 'U' )
		          && !( token->subtype & ( TT_UNSIGNED | TT_FLOAT ) ) )
		{
			script.script_p++;
			token->subtype |= TT_UNSIGNED;
		}
	}

	token->string[ len ] = '\0';
	Parse_NumberValue( token->string, token->subtype, &token->intvalue, &token->floatvalue );

	if ( !( token->subtype & TT_FLOAT ) ) { token->subtype |= TT_INTEGER; }

	return true;
}

/*
===============
Parse_ReadPunctuation
===============
*/
static bool Parse_ReadPunctuation( script_t& script, token_t *token )
{
	int           len;
	const char          *p;
	punctuation_t *punc;

	for ( punc = script.punctuationtable[( unsigned int ) * script.script_p ]; punc; punc = punc->next )
	{
		p = punc->p;
		len = strlen( p );

		//if the script contains at least as much characters as the punctuation
		if ( script.script_p + len <= script.end_p )
		{
			//if the script contains the punctuation
			if ( !strncmp( script.script_p, p, len ) )
			{
				Q_strncpyz( token->string, p, MAX_TOKEN_CHARS );
				script.script_p += len;
				token->type = TT_PUNCTUATION;
				//sub type is the number of the punctuation
				token->subtype = punc->n;
				return true;
			}
		}
	}

	return false;
}

/*
===============
Parse_ReadPrimitive
===============
*/
static bool Parse_ReadPrimitive( script_t& script, token_t *token )
{
	int len;

	len = 0;

	while ( *script.script_p > ' ' && *script.script_p != ';' )
	{
		if ( len >= MAX_TOKEN_CHARS )
		{
			Parse_ScriptError( script, "primitive token longer than MAX_TOKEN_CHARS = %d", MAX_TOKEN_CHARS );
			return false;
		}

		token->string[ len++ ] = *script.script_p++;
	}

	if ( len >= MAX_TOKEN_CHARS )
	{
		// The last len++ made len==MAX_TOKEN_CHARS, which will overflow.
		// Bring it back down and ensure we null terminate.
		len = MAX_TOKEN_CHARS - 1;
	}

	token->string[ len ] = '\0';

	//copy the token into the script structure
	script.token = *token; // was memcpy

	//primitive reading successful
	return true;
}

/*
===============
Parse_ReadScriptToken
===============
*/
static bool Parse_ReadScriptToken( script_t& script, token_t *token )
{
	//if there is a token available (from UnreadToken)
	if ( script.tokenavailable )
	{
		script.tokenavailable = 0;
		*token = script.token; // Was memcpy
		return true;
	}

	//save script pointer
	script.lastscript_p = script.script_p;
	//save line counter
	script.lastline = script.line;
	//start of the white space
	script.whitespace_p = script.script_p;

	//clear the token stuff
	token->clear(); // Was memset
	token->whitespace_p = script.script_p;

	//read unusefull stuff
	if ( !Parse_ReadWhiteSpace( script ) ) { return false; }

	script.endwhitespace_p = script.script_p;
	token->endwhitespace_p = script.script_p;
	//line the token is on
	token->line = script.line;
	//number of lines crossed before token
	token->linescrossed = script.line - script.lastline;

	//if there is a leading double quote
	if ( *script.script_p == '\"' )
	{
		if ( !Parse_ReadString( script, token, '\"' ) ) { return false; }
	}
	//if there is a literal
	else if ( *script.script_p == '\'' )
	{
		//if (!Parse_ReadLiteral(script, token)) return 0;
		if ( !Parse_ReadString( script, token, '\'' ) ) { return false; }
	}
	//if there is a number
	else if ( ( *script.script_p >= '0' && *script.script_p <= '9' ) ||
	          ( *script.script_p == '.' &&
	            ( * ( script.script_p + 1 ) >= '0' && * ( script.script_p + 1 ) <= '9' ) ) )
	{
		if ( !Parse_ReadNumber( script, token ) ) { return false; }
	}
	//if this is a primitive script
	else if ( script.flags & SCFL_PRIMITIVE )
	{
		return Parse_ReadPrimitive( script, token );
	}
	//if there is a name
	else if ( ( *script.script_p >= 'a' && *script.script_p <= 'z' ) ||
	          ( *script.script_p >= 'A' && *script.script_p <= 'Z' ) ||
	          *script.script_p == '_' )
	{
		if ( !Parse_ReadName( script, token ) ) { return false; }
	}
	//check for punctuations
	else if ( !Parse_ReadPunctuation( script, token ) )
	{
		Parse_ScriptError( script, "can't read token" );
		return false;
	}

	//copy the token into the script structure
	script.token = *token; // was memcpy
	//successfully read a token
	return true;
}

/*
===============
Parse_StripDoubleQuotes
===============
*/
static void Parse_StripDoubleQuotes(char *str)
{
	size_t len = strlen(str);
	if (len >= 2 && str[0] == '\"' && str[len - 1] == '\"')
	{
		memcpy(str, str + 1, len - 2);
		str[len - 2] = '\0';
	}
}

// Return a copy of the given string minus any surrounding quotes.
static std::string Parse_StripDoubleQuotesCopy(const char *str)
{
	size_t len = strlen(str);
	std::string result;
	result.reserve(len);
	if (len >= 2 && str[0] == '\"' && str[len - 1] == '\"')
		result.assign(str + 1, str + len - 1);
	else
		result = str;
	return result;
}

/*
===============
Parse_EndOfScript
===============
*/
static int Parse_EndOfScript( script_t& script )
{
	return script.script_p >= script.end_p;
}

static void Parse_InitScript(script_t& script, const char* filename, char* source, size_t length)
{
	Q_strncpyz(script.filename, filename, sizeof(script.filename));
	script.buffer = source;
	script.length = length;
	script.script_p = script.buffer;
	script.lastscript_p = script.buffer; // pointer before reading token
	script.end_p = &script.buffer[length];
	script.tokenavailable = 0;
	script.line = 1;
	script.lastline = 1;
	Parse_SetScriptPunctuations(script, nullptr);
}

/*
===============
Parse_LoadScriptFile
===============
*/
static script_t *Parse_LoadScriptFile( const char *filename )
{
	Parse_CheckScriptFileNameSize(filename);

	fileHandle_t fp;
	int length = FS_FOpenFileRead( filename, &fp, false );
	if ( !fp ) { return nullptr; }

	char* buffer = (char*) Z_Malloc( sizeof( script_t ) + length + 1 );
	if (!buffer)
	{
		FS_FCloseFile(fp);
		// Throw because caller can't tell the difference between file not found
		// and insufficient memory and will assume the former and will
		// try to open other files otherwise. See caller.
		throw std::bad_alloc();
	}
	char* source = buffer + sizeof(script_t);
	auto actualLength = FS_Read(source, length, fp);

	FS_FCloseFile(fp);

	// Any error might happen to give an unexpected length.
	// Files read in text mode may result in unexpected lengths.
	// Check to guard against both situations and avoid random crashes.
	// We expect binary mode. A text mode read would look like an error.
	if (actualLength != length)
	{
		Z_Free(buffer);
		throw std::runtime_error(Str::Format("File %s has an unexpected size. Expected %d, actual: %d",
			filename, length, actualLength));
	}
	source[length] = '\0';

	script_t& script = *(script_t*)buffer;
	Parse_InitScript(script, filename, source, length);
	return &script;
}

/*
===============
Parse_LoadScriptMemory
===============
*/
static script_t *Parse_LoadScriptMemory( const char *ptr, int length, const char *name )
{
	Parse_CheckScriptFileNameSize(name);

	char* buffer = (char*) Z_Malloc( sizeof( script_t ) + length + 1 );
	if (!buffer)
		throw std::bad_alloc();

	script_t& script = *(script_t*)buffer;
	char* source = buffer + sizeof(script_t);
	Com_Memcpy(source, ptr, length);
	source[length] = '\0';
	Parse_InitScript(script, name, source, length);
	return &script;
}

/*
===============
Parse_FreeScript
===============
*/
static void Parse_FreeScript( script_t& script )
{
	if ( script.punctuationtable ) { Z_Free( script.punctuationtable ); }

	Z_Free( &script );
}

/*
===============
Parse_SourceError
===============
*/
static void QDECL PRINTF_LIKE(2) Parse_SourceError( const source_t& source, const char *str, ... )
{
	char    text[ 1024 ];
	va_list ap;

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
	Com_Printf( "file %s, line %d: %s\n", source.scriptstack->filename, source.scriptstack->line, text );
}

/*
===============
Parse_SourceWarning
===============
*/
static void QDECL PRINTF_LIKE(2) Parse_SourceWarning( const source_t& source, const char *str, ... )
{
	char    text[ 1024 ];
	va_list ap;

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
	Com_Printf( "file %s, line %d: %s\n", source.scriptstack->filename, source.scriptstack->line, text );
}

/*
===============
Parse_PushIndent
===============
*/
static void Parse_PushIndent( source_t& source, int type, int skip )
{
	indent_t *indent;

	indent = ( indent_t * ) Z_Malloc( sizeof( indent_t ) );
	if (!indent)
		throw std::bad_alloc();
	indent->type = type;
	indent->script = source.scriptstack;
	indent->skip = ( skip != 0 );
	source.skip += indent->skip;
	indent->next = source.indentstack;
	source.indentstack = indent;
}

/*
===============
Parse_PopIndent
===============
*/
static void Parse_PopIndent( source_t& source, int *type, int *skip )
{
	indent_t *indent;

	*type = 0;
	*skip = 0;

	indent = source.indentstack;

	if ( !indent ) { return; }

	//must be an indent from the current script
	if ( source.indentstack->script != source.scriptstack ) { return; }

	*type = indent->type;
	*skip = indent->skip;
	source.indentstack = source.indentstack->next;
	source.skip -= indent->skip;
	Z_Free( indent );
}

/*
===============
Parse_PushScript
===============
*/
static void Parse_PushScript( source_t& source, script_t *script )
{
	script_t *s;

	for ( s = source.scriptstack; s; s = s->next )
	{
		if ( !Q_stricmp( s->filename, script->filename ) )
		{
			Parse_SourceError( source, "%s recursively included", script->filename );
			return;
		}
	}

	//push the script on the script stack
	script->next = source.scriptstack;
	source.scriptstack = script;
}

/*
===============
Parse_CopyToken
===============
*/
static token_t *Parse_CopyToken( token_t *token )
{
	token_t *t;

	t = ( token_t * ) Z_Malloc( sizeof( token_t ) );

	if ( !t )
	{
		Com_Error( ERR_FATAL, "out of token space" );
		throw std::bad_alloc(); // No one will check return. Throw if we reach here.
	}

	*t = *token; // was memcpy
	t->next = nullptr;
	numtokens++;
	return t;
}

/*
===============
Parse_FreeToken
===============
*/
static void Parse_FreeToken( token_t *token )
{
	Z_Free( token );
	numtokens--;
}

/*
===============
Parse_ReadSourceToken
===============
*/
static bool Parse_ReadSourceToken( source_t& source, token_t *token )
{
	token_t  *t;
	script_t *script;
	int      type, skip, lines;

	lines = 0;

	//if there's no token already available
	while ( !source.tokens )
	{
		//if there's a token to read from the script
		if ( Parse_ReadScriptToken( *source.scriptstack, token ) )
		{
			token->linescrossed += lines;
			return true;
		}

		// if lines were crossed before the end of the script, count them
		lines += source.scriptstack->line - source.scriptstack->lastline;

		//if at the end of the script
		if ( Parse_EndOfScript( *source.scriptstack ) )
		{
			//remove all indents of the script
			while ( source.indentstack &&
			        source.indentstack->script == source.scriptstack )
			{
				Parse_SourceWarning( source, "missing #endif" );
				Parse_PopIndent( source, &type, &skip );
			}
		}

		//if this was the initial script
		if ( !source.scriptstack->next ) { return false; }

		//remove the script and return to the last one
		script = source.scriptstack;
		source.scriptstack = source.scriptstack->next;
		Parse_FreeScript( *script );
	}

	//copy the already available token
	*token = *source.tokens; // was memcpy
	//free the read token
	t = source.tokens;
	source.tokens = source.tokens->next;
	Parse_FreeToken( t );
	return true;
}

/*
===============
Parse_UnreadSourceToken
===============
*/
static int Parse_UnreadSourceToken( source_t& source, token_t *token )
{
	token_t *t;

	t = Parse_CopyToken( token );
	t->next = source.tokens;
	source.tokens = t;
	return true;
}

/*
===============
Parse_ReadDefineParms
===============
*/
static bool Parse_ReadDefineParms( source_t& source, define_t *define, token_t **parms, int maxparms )
{
	token_t token, *t, *last;
	int     i, done, lastcomma, numparms, indent;

	if ( !Parse_ReadSourceToken( source, &token ) )
	{
		Parse_SourceError( source, "define %s missing parms", define->name );
		return false;
	}

	if ( define->numparms > maxparms )
	{
		Parse_SourceError( source, "define with more than %d parameters", maxparms );
		return false;
	}

	for ( i = 0; i < define->numparms; i++ ) 
		parms[ i ] = nullptr;

	//if no leading "("
	if ( token.isNot( "(" ) )
	{
		Parse_UnreadSourceToken( source, &token );
		Parse_SourceError( source, "define %s missing parms", define->name );
		return false;
	}

	//read the define parameters
	for ( done = 0, numparms = 0, indent = 0; !done; )
	{
		if ( numparms >= maxparms )
		{
			Parse_SourceError( source, "define %s with too many parms", define->name );
			return false;
		}

		if ( numparms >= define->numparms )
		{
			Parse_SourceWarning( source, "define %s has too many parms", define->name );
			return false;
		}

		parms[ numparms ] = nullptr;
		lastcomma = 1;
		last = nullptr;

		while ( !done )
		{
			if ( !Parse_ReadSourceToken( source, &token ) )
			{
				Parse_SourceError( source, "define %s incomplete", define->name );
				return false;
			}

			if ( token.is( "," ) )
			{
				if ( indent <= 0 )
				{
					if ( lastcomma ) { Parse_SourceWarning( source, "too many comma's" ); }

					lastcomma = 1;
					break;
				}
			}

			lastcomma = 0;

			if ( token.is("(" ) )
			{
				indent++;
				continue;
			}
			else if ( token.is( ")" )  )
			{
				if ( --indent <= 0 )
				{
					if ( !parms[ define->numparms - 1 ] )
					{
						Parse_SourceWarning( source, "too few define parms" );
					}

					done = 1;
					break;
				}
			}

			if ( numparms < define->numparms )
			{
				t = Parse_CopyToken( &token );
				t->next = nullptr;

				if ( last ) { last->next = t; }
				else { parms[ numparms ] = t; }

				last = t;
			}
		}

		numparms++;
	}

	return true;
}

/*
===============
Parse_StringizeTokens
===============
*/
static bool Parse_StringizeTokens( token_t *tokens, token_t *token )
{
	token_t *t;

	token->type = TT_STRING;
	token->whitespace_p = nullptr;
	token->endwhitespace_p = nullptr;
	token->clearText();

	std::string result( "\"" );
	for ( t = tokens; t; t = t->next )
	{
		const auto& text = t->sref();
		result.append(text.c_str(), text.size());
	}
	result += '\"';
	if (result.size() >= MAX_TOKEN_CHARS)
		return false;
	t->setText(result);

	return true;
}

/*
===============
Parse_MergeTokens
===============
*/
static bool Parse_MergeTokens( source_t& source, token_t& t1, token_t& t2 )
{
	//merging of a name with a name or number
	if ( t1.type == TT_NAME && ( t2.type == TT_NAME || t2.type == TT_NUMBER ) )
	{
		size_t resultLen = t1.size() + t2.size();
		if (resultLen >= MAX_TOKEN_CHARS)
		{
			Parse_SourceError(source, "Can't merge %s with %s. Length exceeds limit of %d chars",
				t1.c_str(), t2.c_str(), MAX_TOKEN_CHARS);
			return false;
		}
		std::string result;
		result.reserve(resultLen);
		result = t1.sref();
		result.append(t2.sref());
		t1.setText(result);
		return true;
	}

	//merging of two strings
	if ( t1.type == TT_STRING && t2.type == TT_STRING )
	{
		size_t resultLen = t1.size() + t2.size();
		// Tokenizer should have made at least 4 characters from
		// two string tokens: i.e. """". or it's a bug.
		assert(resultLen >= 4);
		resultLen -= 2;
		if (resultLen >= MAX_TOKEN_CHARS)
		{
			Parse_SourceError(source, "Can't merge %s with %s. Length exceeds limit of %d chars",
				t1.c_str(), t2.c_str(), MAX_TOKEN_CHARS);
			return false;
		}
		std::string result;
		result.reserve(resultLen); // We will loose 1 set of quotes - 2 chars.
		result.append(t1.c_str(), t1.size() - 1); // Drop one end quote here.
		result.append(t2.c_str() + 1, t2.size() - 1); // Drop leading quote here.
		t1.setText(result);
		return true;
	}
	Parse_SourceError(source, "Can't merge %s (type %d) with %s (type %d). Tokens must be of same type.",
		t1.c_str(), t1.type, t2.c_str(), t2.type);

	//FIXME: merging of two number of the same sub type
	return false;
}

/*
===============
Parse_NameHash
===============
*/
//char primes[16] = {1, 3, 5, 7, 11, 13, 17, 19, 23, 27, 29, 31, 37, 41, 43, 47};
static int Parse_NameHash( const char *name )
{
	int hash, i;

	hash = 0;

	for ( i = 0; name[ i ] != '\0'; i++ )
	{
		hash += name[ i ] * ( 119 + i );
	}

	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) ) & ( DEFINEHASHSIZE - 1 );
	return hash;
}

/*
===============
Parse_AddDefineToHash
===============
*/
static void Parse_AddDefineToHash( define_t *define, define_t **definehash )
{
	int hash;

	hash = Parse_NameHash( define->name );
	define->hashnext = definehash[ hash ];
	definehash[ hash ] = define;
}

/*
===============
Parse_FindHashedDefine
===============
*/
static define_t *Parse_FindHashedDefine( define_t **definehash, const char *name )
{
	define_t *d;
	int      hash;

	hash = Parse_NameHash( name );

	for ( d = definehash[ hash ]; d; d = d->hashnext )
	{
		if ( !strcmp( d->name, name ) ) { return d; }
	}

	return nullptr;
}

/*
===============
Parse_FindDefineParm
===============
*/
static int Parse_FindDefineParm( define_t *define, const char *name )
{
	token_t *p;
	int     i;

	i = 0;

	for ( p = define->parms; p; p = p->next )
	{
		if ( !strcmp( p->string, name ) ) { return i; }

		i++;
	}

	return -1;
}

/*
===============
Parse_FreeDefine
===============
*/
static void Parse_FreeDefine( define_t *define )
{
	token_t *t, *next;

	//free the define parameters
	for ( t = define->parms; t; t = next )
	{
		next = t->next;
		Parse_FreeToken( t );
	}

	//free the define tokens
	for ( t = define->tokens; t; t = next )
	{
		next = t->next;
		Parse_FreeToken( t );
	}

	//free the define
	Z_Free( define );
}

/*
===============
Parse_ExpandBuiltinDefine
===============
*/
static bool Parse_ExpandBuiltinDefine( source_t& source, token_t *deftoken, define_t *define,
                                       token_t **firsttoken, token_t **lasttoken )
{
	token_t *token;
	time_t  t;

	char    *curtime;

	token = Parse_CopyToken( deftoken );

	switch ( define->builtin )
	{
		case BUILTIN_LINE:
			{
				sprintf( token->string, "%d", deftoken->line );
				token->intvalue = deftoken->line;
				token->floatvalue = deftoken->line;
				token->type = TT_NUMBER;
				token->subtype = TT_DECIMAL | TT_INTEGER;
				*firsttoken = token;
				*lasttoken = token;
				break;
			}

		case BUILTIN_FILE:
			{
				strcpy( token->string, source.scriptstack->filename );
				token->type = TT_NAME;
				token->subtype = strlen( token->string );
				*firsttoken = token;
				*lasttoken = token;
				break;
			}

		case BUILTIN_DATE:
			{
				t = time( nullptr );
				curtime = ctime( &t );
				strcpy( token->string, "\"" );
				strncat( token->string, curtime + 4, 7 );
				strncat( token->string + 7, curtime + 20, 4 );
				strcat( token->string, "\"" );
				free( curtime );
				token->type = TT_NAME;
				token->subtype = strlen( token->string );
				*firsttoken = token;
				*lasttoken = token;
				break;
			}

		case BUILTIN_TIME:
			{
				t = time( nullptr );
				curtime = ctime( &t );
				strcpy( token->string, "\"" );
				strncat( token->string, curtime + 11, 8 );
				strcat( token->string, "\"" );
				free( curtime );
				token->type = TT_NAME;
				token->subtype = strlen( token->string );
				*firsttoken = token;
				*lasttoken = token;
				break;
			}

		case BUILTIN_STDC:
		default:
			{
				*firsttoken = nullptr;
				*lasttoken = nullptr;
				break;
			}
	}

	return true;
}

/*
===============
Parse_ExpandDefine
===============
*/
static bool Parse_ExpandDefine( source_t& source, token_t *deftoken, define_t *define,
                                token_t **firsttoken, token_t **lasttoken )
{
	token_t *parms[ MAX_DEFINEPARMS ], *dt, *pt, *t;
	token_t *t1, *t2, *first, *last, *nextpt, token;
	int     parmnum, i;

	//if it is a builtin define
	if ( define->builtin )
	{
		return Parse_ExpandBuiltinDefine( source, deftoken, define, firsttoken, lasttoken );
	}

	//if the define has parameters
	if ( define->numparms )
	{
		if ( !Parse_ReadDefineParms( source, define, parms, MAX_DEFINEPARMS ) ) { return false; }
	}

	//empty list at first
	first = nullptr;
	last = nullptr;

	//create a list with tokens of the expanded define
	for ( dt = define->tokens; dt; dt = dt->next )
	{
		parmnum = -1;

		//if the token is a name, it could be a define parameter
		if ( dt->type == TT_NAME )
		{
			parmnum = Parse_FindDefineParm( define, dt->string );
		}

		//if it is a define parameter
		if ( parmnum >= 0 )
		{
			for ( pt = parms[ parmnum ]; pt; pt = pt->next )
			{
				t = Parse_CopyToken( pt );
				//add the token to the list
				t->next = nullptr;

				if ( last ) { last->next = t; }
				else { first = t; }

				last = t;
			}
		}
		else
		{
			//if stringizing operator
			if ( dt->string[ 0 ] == '#' && dt->string[ 1 ] == '\0' )
			{
				//the stringizing operator must be followed by a define parameter
				if ( dt->next ) { parmnum = Parse_FindDefineParm( define, dt->next->string ); }
				else { parmnum = -1; }

				if ( parmnum >= 0 )
				{
					//step over the stringizing operator
					dt = dt->next;

					//stringize the define parameter tokens
					if ( !Parse_StringizeTokens( parms[ parmnum ], &token ) )
					{
						Parse_SourceError( source, "can't stringize tokens" );
						return false;
					}

					t = Parse_CopyToken( &token );
				}
				else
				{
					Parse_SourceWarning( source, "stringizing operator without define parameter" );
					continue;
				}
			}
			else
			{
				t = Parse_CopyToken( dt );
			}

			//add the token to the list
			t->next = nullptr;

			if ( last ) { last->next = t; }
			else { first = t; }

			last = t;
		}
	}

	//check for the merging operator
	for ( t = first; t; )
	{
		if ( t->next )
		{
			//if the merging operator
			if ( t->next->string[ 0 ] == '#' && t->next->string[ 1 ] == '#' )
			{
				t1 = t;
				t2 = t->next->next;

				if ( t2 )
				{
					if ( !Parse_MergeTokens( source, *t1, *t2 ) ) // T1 gets the merged result.
					{
						return false;
					}

					Parse_FreeToken( t1->next );
					t1->next = t2->next;

					if ( t2 == last ) { last = t1; }

					Parse_FreeToken( t2 );
					continue;
				}
			}
		}

		t = t->next;
	}

	//store the first and last token of the list
	*firsttoken = first;
	*lasttoken = last;

	//free all the parameter tokens
	for ( i = 0; i < define->numparms; i++ )
	{
		for ( pt = parms[ i ]; pt; pt = nextpt )
		{
			nextpt = pt->next;
			Parse_FreeToken( pt );
		}
	}

	return true;
}

/*
===============
Parse_ExpandDefineIntoSource
===============
*/
static bool Parse_ExpandDefineIntoSource( source_t& source, token_t *deftoken, define_t *define )
{
	token_t *firsttoken, *lasttoken;

	if ( !Parse_ExpandDefine( source, deftoken, define, &firsttoken, &lasttoken ) ) { return false; }

	if ( firsttoken && lasttoken )
	{
		lasttoken->next = source.tokens;
		source.tokens = firsttoken;
		return true;
	}

	return false;
}

/*
===============
Parse_ConvertPath
===============
*/
static std::string Parse_ConvertPath( const char *path )
{
	std::string result;
	result.reserve(strlen(path));

	//remove double path separators
	for ( const char* ptr = path; *ptr; )
	{
		if ((*ptr == '\\' || *ptr == '/') &&
			(*(ptr + 1) == '\\' || *(ptr + 1) == '/'))
		{
			result.push_back(*ptr++);
			++ptr;
		}
		else
			result.push_back(*ptr++);
	}
	return result;
}

/*
===============
Parse_ReadLine

reads a token from the current line, continues reading on the next
line only if a backslash '\' is encountered.
===============
*/
static bool Parse_ReadLine( source_t& source, token_t *token )
{
	int crossline;

	crossline = 0;

	do
	{
		if ( !Parse_ReadSourceToken( source, token ) ) { return false; }

		if ( token->linescrossed > crossline )
		{
			Parse_UnreadSourceToken( source, token );
			return false;
		}

		crossline = 1;
	}
	while ( !strcmp( token->string, "\\" ) );

	return true;
}

/*
===============
Parse_OperatorPriority
===============
*/
struct operator_t
{
	int               op;
	int               priority;
	int               parentheses;
	operator_t        *prev, *next;
};

struct value_t
{
	signed long int intvalue;
	double          floatvalue;
	int             parentheses;
	value_t        *prev, *next;
};

static int Parse_OperatorPriority( int op )
{
	switch ( op )
	{
		case P_MUL:
			return 15;

		case P_DIV:
			return 15;

		case P_MOD:
			return 15;

		case P_ADD:
			return 14;

		case P_SUB:
			return 14;

		case P_LOGIC_AND:
			return 7;

		case P_LOGIC_OR:
			return 6;

		case P_LOGIC_GEQ:
			return 12;

		case P_LOGIC_LEQ:
			return 12;

		case P_LOGIC_EQ:
			return 11;

		case P_LOGIC_UNEQ:
			return 11;

		case P_LOGIC_NOT:
			return 16;

		case P_LOGIC_GREATER:
			return 12;

		case P_LOGIC_LESS:
			return 12;

		case P_RSHIFT:
			return 13;

		case P_LSHIFT:
			return 13;

		case P_BIN_AND:
			return 10;

		case P_BIN_OR:
			return 8;

		case P_BIN_XOR:
			return 9;

		case P_BIN_NOT:
			return 16;

		case P_COLON:
			return 5;

		case P_QUESTIONMARK:
			return 5;
	}

	return 0;
}

const int MAX_VALUES    = 64;
const int MAX_OPERATORS = 64;

/*
===============
Parse_EvaluateTokens
===============
*/
static bool Parse_EvaluateTokens( source_t& source, token_t *tokens, signed long int *intvalue,
                                  double *floatvalue, int integer )
{
	operator_t *o, *firstoperator, *lastoperator;
	value_t    *v, *firstvalue, *lastvalue, *v1, *v2;
	token_t    *t;
	int        brace = 0;
	int        parentheses = 0;
	int        error = 0;
	int        lastwasvalue = 0;
	int        negativevalue = 0;
	int        questmarkintvalue = 0;
	double     questmarkfloatvalue = 0;
	int        gotquestmarkvalue = false;
	operator_t operator_heap[ MAX_OPERATORS ];
	int        numoperators = 0;
	value_t    value_heap[ MAX_VALUES ];
	int        numvalues = 0;

	firstoperator = lastoperator = nullptr;
	firstvalue = lastvalue = nullptr;

	if ( intvalue ) { *intvalue = 0; }

	if ( floatvalue ) { *floatvalue = 0; }

	auto AllocValue = [&](value_t*& val)->bool
	{
		if (numvalues >= MAX_VALUES)
		{
			Parse_SourceError(source, "out of value space");
			error = 1;
			return false;
		}
		val = &value_heap[ numvalues++ ];
		return true;
	};
	auto AllocOperator = [&](operator_t*& op)->bool
	{
        if (numoperators >= MAX_OPERATORS)
		{
			Parse_SourceError(source, "out of operator space");
			error = 1;
			return false;
		}
		op = &operator_heap[ numoperators++ ];
		return true;
	};

	for ( t = tokens; t; t = t->next )
	{
		switch ( t->type )
		{
			case TT_NAME:
				{
					if ( lastwasvalue || negativevalue )
					{
						Parse_SourceError( source, "syntax error in #if/#elif" );
						error = 1;
						break;
					}

					if ( strcmp( t->string, "defined" ) )
					{
						Parse_SourceError( source, "undefined name %s in #if/#elif", t->string );
						error = 1;
						break;
					}

					t = t->next;

					if ( !strcmp( t->string, "(" ) )
					{
						brace = true;
						t = t->next;
					}

					if ( !t || t->type != TT_NAME )
					{
						Parse_SourceError( source, "defined without name in #if/#elif" );
						error = 1;
						break;
					}

					if (!AllocValue(v))
						break;

					if ( Parse_FindHashedDefine( source.definehash, t->string ) )
					{
						v->intvalue = 1;
						v->floatvalue = 1;
					}
					else
					{
						v->intvalue = 0;
						v->floatvalue = 0;
					}

					v->parentheses = parentheses;
					v->next = nullptr;
					v->prev = lastvalue;

					if ( lastvalue ) { lastvalue->next = v; }
					else { firstvalue = v; }

					lastvalue = v;

					if ( brace )
					{
						t = t->next;

						if ( !t || strcmp( t->string, ")" ) )
						{
							Parse_SourceError( source, "defined without ) in #if/#elif" );
							error = 1;
							break;
						}
					}

					brace = false;
					// defined() creates a value
					lastwasvalue = 1;
					break;
				}

			case TT_NUMBER:
				{
					if ( lastwasvalue )
					{
						Parse_SourceError( source, "syntax error in #if/#elif" );
						error = 1;
						break;
					}

					if (!AllocValue(v))
						break;

					if ( negativevalue )
					{
						v->intvalue = - ( signed int ) t->intvalue;
						v->floatvalue = -t->floatvalue;
					}
					else
					{
						v->intvalue = t->intvalue;
						v->floatvalue = t->floatvalue;
					}

					v->parentheses = parentheses;
					v->next = nullptr;
					v->prev = lastvalue;

					if ( lastvalue ) { lastvalue->next = v; }
					else { firstvalue = v; }

					lastvalue = v;
					//last token was a value
					lastwasvalue = 1;
					//
					negativevalue = 0;
					break;
				}

			case TT_PUNCTUATION:
				{
					if ( negativevalue )
					{
						Parse_SourceError( source, "misplaced minus sign in #if/#elif" );
						error = 1;
						break;
					}

					if ( t->subtype == P_PARENTHESESOPEN )
					{
						parentheses++;
						break;
					}
					else if ( t->subtype == P_PARENTHESESCLOSE )
					{
						parentheses--;

						if ( parentheses < 0 )
						{
							Parse_SourceError( source, "too many ) in #if/#elsif" );
							error = 1;
						}

						break;
					}

					//check for invalid operators on floating point values
					if ( !integer )
					{
						if ( t->subtype == P_BIN_NOT || t->subtype == P_MOD ||
						     t->subtype == P_RSHIFT || t->subtype == P_LSHIFT ||
						     t->subtype == P_BIN_AND || t->subtype == P_BIN_OR ||
						     t->subtype == P_BIN_XOR )
						{
							Parse_SourceError( source, "illegal operator %s on floating point operands", t->string );
							error = 1;
							break;
						}
					}

					switch ( t->subtype )
					{
						case P_LOGIC_NOT:
						case P_BIN_NOT:
							{
								if ( lastwasvalue )
								{
									Parse_SourceError( source, "! or ~ after value in #if/#elif" );
									error = 1;
									break;
								}

								break;
							}

						case P_INC:
						case P_DEC:
							{
								Parse_SourceError( source, "++ or -- used in #if/#elif" );
								break;
							}

						case P_SUB:
							{
								if ( !lastwasvalue )
								{
									negativevalue = 1;
									break;
								}
							}

						case P_MUL:
						case P_DIV:
						case P_MOD:
						case P_ADD:

						case P_LOGIC_AND:
						case P_LOGIC_OR:
						case P_LOGIC_GEQ:
						case P_LOGIC_LEQ:
						case P_LOGIC_EQ:
						case P_LOGIC_UNEQ:

						case P_LOGIC_GREATER:
						case P_LOGIC_LESS:

						case P_RSHIFT:
						case P_LSHIFT:

						case P_BIN_AND:
						case P_BIN_OR:
						case P_BIN_XOR:

						case P_COLON:
						case P_QUESTIONMARK:
							{
								if ( !lastwasvalue )
								{
									Parse_SourceError( source, "operator %s after operator in #if/#elif", t->string );
									error = 1;
									break;
								}

								break;
							}

						default:
							{
								Parse_SourceError( source, "invalid operator %s in #if/#elif", t->string );
								error = 1;
								break;
							}
					}

					if ( !error && !negativevalue )
					{
						if (!AllocOperator(o))
							break;
						o->op = t->subtype;
						o->priority = Parse_OperatorPriority( t->subtype );
						o->parentheses = parentheses;
						o->next = nullptr;
						o->prev = lastoperator;

						if ( lastoperator ) { lastoperator->next = o; }
						else { firstoperator = o; }

						lastoperator = o;
						lastwasvalue = 0;
					}

					break;
				}

			default:
				{
					Parse_SourceError( source, "unknown %s in #if/#elif", t->string );
					error = 1;
					break;
				}
		}

		if ( error ) { break; }
	}

	if ( !error )
	{
		if ( !lastwasvalue )
		{
			Parse_SourceError( source, "trailing operator in #if/#elif" );
			error = 1;
		}
		else if ( parentheses )
		{
			Parse_SourceError( source, "too many ( in #if/#elif" );
			error = 1;
		}
	}

	//
	gotquestmarkvalue = false;
	questmarkintvalue = 0;
	questmarkfloatvalue = 0;

	//while there are operators
	while ( !error && firstoperator )
	{
		v = firstvalue;

		for ( o = firstoperator; o->next; o = o->next )
		{
			//if the current operator is nested deeper in parentheses
			//than the next operator
			if ( o->parentheses > o->next->parentheses ) { break; }

			//if the current and next operator are nested equally deep in parentheses
			if ( o->parentheses == o->next->parentheses )
			{
				//if the priority of the current operator is equal or higher
				//than the priority of the next operator
				if ( o->priority >= o->next->priority ) { break; }
			}

			//if the arity of the operator isn't equal to 1
			if ( o->op != P_LOGIC_NOT
			     && o->op != P_BIN_NOT ) { v = v->next; }

			//if there's no value or no next value
			if ( !v )
			{
				Parse_SourceError( source, "mising values in #if/#elif" );
				error = 1;
				break;
			}
		}

		if ( error ) { break; }

		v1 = v;
		v2 = v->next;

		switch ( o->op )
		{
			case P_LOGIC_NOT:
				v1->intvalue = !v1->intvalue;
				v1->floatvalue = !v1->floatvalue;
				break;

			case P_BIN_NOT:
				v1->intvalue = ~v1->intvalue;
				break;

			case P_MUL:
				v1->intvalue *= v2->intvalue;
				v1->floatvalue *= v2->floatvalue;
				break;

			case P_DIV:
				if ( !v2->intvalue || !v2->floatvalue )
				{
					Parse_SourceError( source, "divide by zero in #if/#elif" );
					error = 1;
					break;
				}

				v1->intvalue /= v2->intvalue;
				v1->floatvalue /= v2->floatvalue;
				break;

			case P_MOD:
				if ( !v2->intvalue )
				{
					Parse_SourceError( source, "divide by zero in #if/#elif" );
					error = 1;
					break;
				}

				v1->intvalue %= v2->intvalue;
				break;

			case P_ADD:
				v1->intvalue += v2->intvalue;
				v1->floatvalue += v2->floatvalue;
				break;

			case P_SUB:
				v1->intvalue -= v2->intvalue;
				v1->floatvalue -= v2->floatvalue;
				break;

			case P_LOGIC_AND:
				v1->intvalue = v1->intvalue && v2->intvalue;
				v1->floatvalue = v1->floatvalue && v2->floatvalue;
				break;

			case P_LOGIC_OR:
				v1->intvalue = v1->intvalue || v2->intvalue;
				v1->floatvalue = v1->floatvalue || v2->floatvalue;
				break;

			case P_LOGIC_GEQ:
				v1->intvalue = v1->intvalue >= v2->intvalue;
				v1->floatvalue = v1->floatvalue >= v2->floatvalue;
				break;

			case P_LOGIC_LEQ:
				v1->intvalue = v1->intvalue <= v2->intvalue;
				v1->floatvalue = v1->floatvalue <= v2->floatvalue;
				break;

			case P_LOGIC_EQ:
				v1->intvalue = v1->intvalue == v2->intvalue;
				v1->floatvalue = v1->floatvalue == v2->floatvalue;
				break;

			case P_LOGIC_UNEQ:
				v1->intvalue = v1->intvalue != v2->intvalue;
				v1->floatvalue = v1->floatvalue != v2->floatvalue;
				break;

			case P_LOGIC_GREATER:
				v1->intvalue = v1->intvalue > v2->intvalue;
				v1->floatvalue = v1->floatvalue > v2->floatvalue;
				break;

			case P_LOGIC_LESS:
				v1->intvalue = v1->intvalue < v2->intvalue;
				v1->floatvalue = v1->floatvalue < v2->floatvalue;
				break;

			case P_RSHIFT:
				v1->intvalue >>= v2->intvalue;
				break;

			case P_LSHIFT:
				v1->intvalue <<= v2->intvalue;
				break;

			case P_BIN_AND:
				v1->intvalue &= v2->intvalue;
				break;

			case P_BIN_OR:
				v1->intvalue |= v2->intvalue;
				break;

			case P_BIN_XOR:
				v1->intvalue ^= v2->intvalue;
				break;

			case P_COLON:
				{
					if ( !gotquestmarkvalue )
					{
						Parse_SourceError( source, ": without ? in #if/#elif" );
						error = 1;
						break;
					}

					if ( integer )
					{
						if ( !questmarkintvalue ) { v1->intvalue = v2->intvalue; }
					}
					else
					{
						if ( !questmarkfloatvalue ) { v1->floatvalue = v2->floatvalue; }
					}

					gotquestmarkvalue = false;
					break;
				}

			case P_QUESTIONMARK:
				{
					if ( gotquestmarkvalue )
					{
						Parse_SourceError( source, "? after ? in #if/#elif" );
						error = 1;
						break;
					}

					questmarkintvalue = v1->intvalue;
					questmarkfloatvalue = v1->floatvalue;
					gotquestmarkvalue = true;
					break;
				}
		}

		if ( error ) { break; }

		//if not an operator with arity 1
		if ( o->op != P_LOGIC_NOT
		     && o->op != P_BIN_NOT )
		{
			//remove the second value if not question mark operator
			if ( o->op != P_QUESTIONMARK ) { v = v->next; }

			if ( v->prev ) { v->prev->next = v->next; }
			else { firstvalue = v->next; }

			if ( v->next ) { v->next->prev = v->prev; }
			else { lastvalue = v->prev; }
		}

		//remove the operator
		if ( o->prev ) { o->prev->next = o->next; }
		else { firstoperator = o->next; }

		if ( o->next ) { o->next->prev = o->prev; }
		else { lastoperator = o->prev; }
	}

	if ( firstvalue )
	{
		if ( intvalue ) { *intvalue = firstvalue->intvalue; }

		if ( floatvalue ) { *floatvalue = firstvalue->floatvalue; }
	}

	for ( o = firstoperator; o; o = lastoperator )
	{
		lastoperator = o->next;
	}

	for ( v = firstvalue; v; v = lastvalue )
	{
		lastvalue = v->next;
	}

	if ( !error ) { return true; }

	if ( intvalue ) { *intvalue = 0; }

	if ( floatvalue ) { *floatvalue = 0; }

	return false;
}

/*
===============
Parse_Evaluate
===============
*/
static int Parse_Evaluate( source_t& source, signed long int *intvalue,
                           double *floatvalue, int integer )
{
	token_t  token, *firsttoken, *lasttoken;
	token_t  *t, *nexttoken;
	define_t *define;
	int      defined = false;

	if ( intvalue ) { *intvalue = 0; }

	if ( floatvalue ) { *floatvalue = 0; }

	if ( !Parse_ReadLine( source, &token ) )
	{
		Parse_SourceError( source, "no value after #if/#elif" );
		return false;
	}

	firsttoken = nullptr;
	lasttoken = nullptr;

	do
	{
		//if the token is a name
		if ( token.type == TT_NAME )
		{
			if ( defined )
			{
				defined = false;
				t = Parse_CopyToken( &token );
				t->next = nullptr;

				if ( lasttoken ) { lasttoken->next = t; }
				else { firsttoken = t; }

				lasttoken = t;
			}
			else if ( token.is("defined") )
			{
				defined = true;
				t = Parse_CopyToken( &token );
				t->next = nullptr;

				if ( lasttoken ) { lasttoken->next = t; }
				else { firsttoken = t; }

				lasttoken = t;
			}
			else
			{
				//then it must be a define
				define = Parse_FindHashedDefine( source.definehash, token.c_str() );

				if ( !define )
				{
					Parse_SourceError( source, "can't evaluate %s, not defined", token.c_str() );
					return false;
				}

				if ( !Parse_ExpandDefineIntoSource( source, &token, define ) ) { return false; }
			}
		}
		//if the token is a number or a punctuation
		else if ( token.type == TT_NUMBER || token.type == TT_PUNCTUATION )
		{
			t = Parse_CopyToken( &token );
			t->next = nullptr;

			if ( lasttoken ) { lasttoken->next = t; }
			else { firsttoken = t; }

			lasttoken = t;
		}
		else //can't evaluate the token
		{
			Parse_SourceError( source, "can't evaluate %s", token.c_str() );
			return false;
		}
	}
	while ( Parse_ReadLine( source, &token ) );

	if ( !Parse_EvaluateTokens( source, firsttoken, intvalue, floatvalue, integer ) ) { return false; }

	for ( t = firsttoken; t; t = nexttoken )
	{
		nexttoken = t->next;
		Parse_FreeToken( t );
	}

	return true;
}

/*
===============
Parse_DollarEvaluate
===============
*/
static bool Parse_DollarEvaluate( source_t& source, signed long int *intvalue,
                                  double *floatvalue, int integer )
{
	int      indent;
	bool defined = false;
	token_t  token, *firsttoken, *lasttoken;
	token_t  *t, *nexttoken;
	define_t *define;

	if ( intvalue ) { *intvalue = 0; }

	if ( floatvalue ) { *floatvalue = 0; }

	if ( !Parse_ReadSourceToken( source, &token ) )
	{
		Parse_SourceError( source, "no leading ( after $evalint/$evalfloat" );
		return false;
	}

	if ( !Parse_ReadSourceToken( source, &token ) )
	{
		Parse_SourceError( source, "nothing to evaluate" );
		return false;
	}

	indent = 1;
	firsttoken = nullptr;
	lasttoken = nullptr;

	do
	{
		//if the token is a name
		if ( token.type == TT_NAME )
		{
			if ( defined )
			{
				defined = false;
				t = Parse_CopyToken( &token );
				t->next = nullptr;

				if ( lasttoken ) { lasttoken->next = t; }
				else { firsttoken = t; }

				lasttoken = t;
			}
			else if ( token.is( "defined" ) )
			{
				defined = true;
				t = Parse_CopyToken( &token );
				t->next = nullptr;

				if ( lasttoken ) { lasttoken->next = t; }
				else { firsttoken = t; }

				lasttoken = t;
			}
			else
			{
				//then it must be a define
				define = Parse_FindHashedDefine( source.definehash, token.c_str() );

				if ( !define )
				{
					for ( t = firsttoken; t; t = nexttoken )
					{
						nexttoken = t->next;
						Parse_FreeToken( t );
					}
					Parse_SourceError( source, "can't evaluate %s, not defined", token.c_str() );
					return false;
				}

				if ( !Parse_ExpandDefineIntoSource( source, &token, define ) ) 
				{
					for ( t = firsttoken; t; t = nexttoken )
					{
						nexttoken = t->next;
						Parse_FreeToken( t );
					}
					return false;
				}
			}
		}
		//if the token is a number or a punctuation
		else if ( token.type == TT_NUMBER || token.type == TT_PUNCTUATION )
		{
			if (token.startsWith( '(' )) { indent++; }
			else if (token.startsWith( ')')) { indent--; }

			if ( indent <= 0 ) { break; }

			t = Parse_CopyToken( &token );
			t->next = nullptr;

			if ( lasttoken ) { lasttoken->next = t; }
			else { firsttoken = t; }

			lasttoken = t;
		}
		else //can't evaluate the token
		{
			Parse_SourceError( source, "can't evaluate %s", token.c_str() );
			return false;
		}
	}
	while ( Parse_ReadSourceToken( source, &token ) );

	if ( !Parse_EvaluateTokens( source, firsttoken, intvalue, floatvalue, integer ) ) { return false; }

	for ( t = firsttoken; t; t = nexttoken )
	{
		nexttoken = t->next;
		Parse_FreeToken( t );
	}

	return true;
}

/*
===============
Parse_Directive_include
===============
*/
static bool Parse_Directive_include( source_t& source )
{
	script_t *script = nullptr;
	token_t  token;
	std::string path;

	if ( source.skip > 0 ) { return true; }

	if ( !Parse_ReadSourceToken( source, &token ) )
	{
		Parse_SourceError( source, "#include without file name" );
		return false;
	}

	if ( token.linescrossed > 0 )
	{
		Parse_SourceError( source, "#include without file name" );
		return false;
	}

	if ( token.type == TT_STRING )
	{
		std::string path = Parse_StripDoubleQuotesCopy(token.c_str());
		path = Parse_ConvertPath( path.c_str() );
		script = Parse_LoadScriptFile( path.c_str() );

		if ( !script )
		{
			path = source.includepath;
			path += token.c_str();
			script = Parse_LoadScriptFile( path.c_str() );
		}
	}
	else if ( token.type == TT_PUNCTUATION && token.startsWith('<'))
	{
		path = source.includepath;

		while ( Parse_ReadSourceToken( source, &token ) )
		{
			if ( token.linescrossed > 0 )
			{
				Parse_UnreadSourceToken( source, &token );
				break;
			}

			if ( token.type == TT_PUNCTUATION && token.startsWith( '>' ))
				break;

			path += token.c_str();
		}

		if ( !token.startsWith( '>' ))
		{
			Parse_SourceWarning( source, "#include missing trailing >" );
		}

		if ( ! path.length() )
		{
			Parse_SourceError( source, "#include without file name between < >" );
			return false;
		}

		path = Parse_ConvertPath( path.c_str() );
		script = Parse_LoadScriptFile( path.c_str() );
	}
	else
	{
		Parse_SourceError( source, "#include without file name" );
		return false;
	}

	if ( !script )
	{
		Parse_SourceError( source, "file %s not found", path.c_str() );
		return false;
	}

	Parse_PushScript( source, script );
	return true;
}

/*
===============
Parse_WhiteSpaceBeforeToken
===============
*/
static int Parse_WhiteSpaceBeforeToken( token_t *token )
{
	return token->endwhitespace_p - token->whitespace_p > 0;
}

/*
===============
Parse_ClearTokenWhiteSpace
===============
*/
static void Parse_ClearTokenWhiteSpace( token_t *token )
{
	token->whitespace_p = nullptr;
	token->endwhitespace_p = nullptr;
	token->linescrossed = 0;
}

/*
===============
Parse_Directive_undef
===============
*/
static bool Parse_Directive_undef( source_t& source )
{
	token_t  token;
	define_t *define, *lastdefine;
	int      hash;

	if ( source.skip > 0 ) { return true; }

	if ( !Parse_ReadLine( source, &token ) )
	{
		Parse_SourceError( source, "undef without name" );
		return false;
	}

	if ( token.type != TT_NAME )
	{
		Parse_UnreadSourceToken( source, &token );
		Parse_SourceError( source, "expected name, found %s", token.c_str() );
		return false;
	}

	hash = Parse_NameHash( token.c_str() );

	for ( lastdefine = nullptr, define = source.definehash[ hash ]; define; define = define->hashnext )
	{
		if ( !strcmp( define->name, token.c_str() ) )
		{
			if ( define->flags & DEFINE_FIXED )
			{
				Parse_SourceWarning( source, "can't undef %s", token.c_str() );
			}
			else
			{
				if ( lastdefine ) { lastdefine->hashnext = define->hashnext; }
				else { source.definehash[ hash ] = define->hashnext; }

				Parse_FreeDefine( define );
			}

			break;
		}

		lastdefine = define;
	}

	return true;
}

/*
===============
Parse_Directive_elif
===============
*/
static bool Parse_Directive_elif( source_t& source )
{
	signed long int value;
	int             type, skip;

	Parse_PopIndent( source, &type, &skip );

	if ( !type || type == INDENT_ELSE )
	{
		Parse_SourceError( source, "misplaced #elif" );
		return false;
	}

	if ( !Parse_Evaluate( source, &value, nullptr, true ) ) { return false; }

	skip = ( value == 0 );
	Parse_PushIndent( source, INDENT_ELIF, skip );
	return true;
}

/*
===============
Parse_Directive_if
===============
*/
static bool Parse_Directive_if( source_t& source )
{
	signed long int value;
	int             skip;

	if ( !Parse_Evaluate( source, &value, nullptr, true ) ) { return false; }

	skip = ( value == 0 );
	Parse_PushIndent( source, INDENT_IF, skip );
	return true;
}

/*
===============
Parse_Directive_line
===============
*/
static bool Parse_Directive_line( source_t& source )
{
	Parse_SourceError( source, "#line directive not supported" );
	return false;
}

/*
===============
Parse_Directive_error
===============
*/
static bool Parse_Directive_error( source_t& source )
{
	token_t token;

	Parse_ReadSourceToken( source, &token );
	Parse_SourceError( source, "#error directive: %s", token.c_str() );
	return false;
}

/*
===============
Parse_Directive_pragma
===============
*/
static bool Parse_Directive_pragma( source_t& source )
{
	token_t token;

	Parse_SourceWarning( source, "#pragma directive not supported" );

	while ( Parse_ReadLine( source, &token ) ) {; }

	return true;
}

/*
===============
Parse_UnreadSignToken
===============
*/
static void Parse_UnreadSignToken( source_t& source )
{
	token_t token;

	token.line = source.scriptstack->line;
	token.whitespace_p = source.scriptstack->script_p;
	token.endwhitespace_p = source.scriptstack->script_p;
	token.linescrossed = 0;
	token.setText( "-" );
	token.type = TT_PUNCTUATION;
	token.subtype = P_SUB;
	Parse_UnreadSourceToken( source, &token );
}

/*
===============
Parse_Directive_eval
===============
*/
static bool Parse_Directive_eval( source_t& source )
{
	char buf[128];
	signed long int value;
	token_t         token;

	if ( !Parse_Evaluate( source, &value, nullptr, true ) ) { return false; }

	token.line = source.scriptstack->line;
	token.whitespace_p = source.scriptstack->script_p;
	token.endwhitespace_p = source.scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(buf, "%ld", std::abs( value ) );
	token.setText(buf);
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
	Parse_UnreadSourceToken( source, &token );

	if ( value < 0 ) { Parse_UnreadSignToken( source ); }

	return true;
}

/*
===============
Parse_Directive_evalfloat
===============
*/
static bool Parse_Directive_evalfloat( source_t& source )
{
	double  value;
	token_t token;
	char buf[128];

	if ( !Parse_Evaluate( source, nullptr, &value, false ) ) { return false; }

	token.line = source.scriptstack->line;
	token.whitespace_p = source.scriptstack->script_p;
	token.endwhitespace_p = source.scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(buf, "%1.2f", std::fabs( value ) );
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
	token.setText(buf);
	Parse_UnreadSourceToken( source, &token );

	if ( value < 0 ) { Parse_UnreadSignToken( source ); }

	return true;
}

/*
===============
Parse_DollarDirective_evalint
===============
*/
static bool Parse_DollarDirective_evalint( source_t& source )
{
	char buf[128];
	signed long int value;
	token_t         token;

	if ( !Parse_DollarEvaluate( source, &value, nullptr, true ) ) { return false; }

	token.line = source.scriptstack->line;
	token.whitespace_p = source.scriptstack->script_p;
	token.endwhitespace_p = source.scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(buf, "%ld", std::abs( value ) );
	token.setText(buf);
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
	token.intvalue = value;
	token.floatvalue = value;
	Parse_UnreadSourceToken( source, &token );

	if ( value < 0 ) { Parse_UnreadSignToken( source ); }

	return true;
}

/*
===============
Parse_DollarDirective_evalfloat
===============
*/
static bool Parse_DollarDirective_evalfloat( source_t& source )
{
	char buf[128];
	double  value;
	token_t token;

	if ( !Parse_DollarEvaluate( source, nullptr, &value, false ) ) { return false; }

	token.line = source.scriptstack->line;
	token.whitespace_p = source.scriptstack->script_p;
	token.endwhitespace_p = source.scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(buf, "%1.2f", std::fabs( value ) );
	token.setText(buf);
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
	token.intvalue = ( unsigned long ) value;
	token.floatvalue = value;
	Parse_UnreadSourceToken( source, &token );

	if ( value < 0 ) { Parse_UnreadSignToken( source ); }

	return true;
}

/*
===============
Parse_ReadDollarDirective
===============
*/
directive_t DollarDirectives[ 20 ] =
{
	{ "evalint",   Parse_DollarDirective_evalint   },
	{ "evalfloat", Parse_DollarDirective_evalfloat },
	{ nullptr,     nullptr                         }
};

static bool Parse_ReadDollarDirective( source_t& source )
{
	token_t token;
	int     i;

	//read the directive name
	if ( !Parse_ReadSourceToken( source, &token ) )
	{
		Parse_SourceError( source, "found $ without name" );
		return false;
	}

	//directive name must be on the same line
	if ( token.linescrossed > 0 )
	{
		Parse_UnreadSourceToken( source, &token );
		Parse_SourceError( source, "found $ at end of line" );
		return false;
	}

	//if it's a name
	if ( token.type == TT_NAME )
	{
		//find the precompiler directive
		for ( i = 0; DollarDirectives[ i ].name; i++ )
		{
			if ( !strcmp( DollarDirectives[ i ].name, token.c_str() ) )
			{
				return DollarDirectives[ i ].func( source );
			}
		}
	}

	Parse_UnreadSourceToken( source, &token );
	Parse_SourceError( source, "unknown precompiler directive %s", token.c_str() );
	return false;
}

/*
===============
Parse_Directive_if_def
===============
*/
static bool Parse_Directive_if_def( source_t& source, int type )
{
	token_t  token;
	define_t *d;
	int      skip;

	if ( !Parse_ReadLine( source, &token ) )
	{
		Parse_SourceError( source, "#ifdef without name" );
		return false;
	}

	if ( token.type != TT_NAME )
	{
		Parse_UnreadSourceToken( source, &token );
		Parse_SourceError( source, "expected name after #ifdef, found %s", token.c_str() );
		return false;
	}

	d = Parse_FindHashedDefine( source.definehash, token.c_str() );
	skip = ( type == INDENT_IFDEF ) == ( d == nullptr );
	Parse_PushIndent( source, type, skip );
	return true;
}

/*
===============
Parse_Directive_ifdef
===============
*/
static bool Parse_Directive_ifdef( source_t& source )
{
	return Parse_Directive_if_def( source, INDENT_IFDEF );
}

/*
===============
Parse_Directive_ifndef
===============
*/
static bool Parse_Directive_ifndef( source_t& source )
{
	return Parse_Directive_if_def( source, INDENT_IFNDEF );
}

/*
===============
Parse_Directive_else
===============
*/
static bool Parse_Directive_else( source_t& source )
{
	int type, skip;

	Parse_PopIndent( source, &type, &skip );

	if ( !type )
	{
		Parse_SourceError( source, "misplaced #else" );
		return false;
	}

	if ( type == INDENT_ELSE )
	{
		Parse_SourceError( source, "#else after #else" );
		return false;
	}

	Parse_PushIndent( source, INDENT_ELSE, !skip );
	return true;
}

/*
===============
Parse_Directive_endif
===============
*/
static bool Parse_Directive_endif( source_t& source )
{
	int type, skip;

	Parse_PopIndent( source, &type, &skip );

	if ( !type )
	{
		Parse_SourceError( source, "misplaced #endif" );
		return false;
	}

	return true;
}

/*
===============
Parse_CheckTokenString
===============
*/
static bool Parse_CheckTokenString( source_t& source, const char *str )
{
	token_t tok;

	if ( !Parse_ReadToken( source, &tok ) ) 
		return false;

	if ( tok.is( str ) )
		return true;

	Parse_UnreadSourceToken( source, &tok );
	return false;
}

/*
===============
Parse_Directive_define
===============
*/
static bool Parse_Directive_define( source_t& source )
{
	token_t  token, *t, *last;
	define_t *define;

	if ( source.skip > 0 ) { return true; }

	if ( !Parse_ReadLine( source, &token ) )
	{
		Parse_SourceError( source, "#define without name" );
		return false;
	}

	if ( token.type != TT_NAME )
	{
		Parse_UnreadSourceToken( source, &token );
		Parse_SourceError( source, "expected name after #define, found %s", token.c_str() );
		return false;
	}

	//check if the define already exists
	define = Parse_FindHashedDefine( source.definehash, token.c_str() );

	if ( define )
	{
		if ( define->flags & DEFINE_FIXED )
		{
			Parse_SourceError( source, "can't redefine %s", token.c_str() );
			return false;
		}

		Parse_SourceWarning( source, "redefinition of %s", token.c_str() );
		//unread the define name before executing the #undef directive
		Parse_UnreadSourceToken( source, &token );

		if ( !Parse_Directive_undef( source ) )
		{
			return false;
		}
	}

	//allocate define
	define = ( define_t * ) Z_Malloc( sizeof( define_t ) + strlen( token.c_str() ) + 1 );
	if (!define)
		throw std::bad_alloc();
	define->name = ( char * ) define + sizeof( define_t );
	strcpy( define->name, token.c_str() );
	//add the define to the source
	Parse_AddDefineToHash( define, source.definehash );

	//if nothing is defined, just return
	if ( !Parse_ReadLine( source, &token ) ) { return true; }

	//if it is a define with parameters
	if ( !Parse_WhiteSpaceBeforeToken( &token ) && token.is( "(" ) )
	{
		//read the define parameters
		last = nullptr;

		if ( !Parse_CheckTokenString( source, ")" ) )
		{
			while ( 1 )
			{
				if ( !Parse_ReadLine( source, &token ) )
				{
					Parse_SourceError( source, "expected define parameter" );
					return false;
				}

				//if it isn't a name
				if ( token.type != TT_NAME )
				{
					Parse_SourceError( source, "invalid define parameter" );
					return false;
				}

				if ( Parse_FindDefineParm( define, token.c_str() ) >= 0 )
				{
					Parse_SourceError( source, "two the same define parameters" );
					return false;
				}

				//add the define parm
				t = Parse_CopyToken( &token );
				Parse_ClearTokenWhiteSpace( t );
				t->next = nullptr;

				if ( last ) { last->next = t; }
				else { define->parms = t; }

				last = t;
				define->numparms++;

				//read next token
				if ( !Parse_ReadLine( source, &token ) )
				{
					Parse_SourceError( source, "define parameters not terminated" );
					return false;
				}

				if ( token.is(")") ) { break; }

				//then it must be a comma
				if ( token.isNot(",") )
				{
					Parse_SourceError( source, "define not terminated" );
					return false;
				}
			}
		}

		if ( !Parse_ReadLine( source, &token ) ) { return true; }
	}

	//read the defined stuff
	last = nullptr;

	do
	{
		t = Parse_CopyToken( &token );

		if ( t->type == TT_NAME && t->is(define->name) )
		{
			Parse_SourceError( source, "recursive define (removed recursion)" );
			continue;
		}

		Parse_ClearTokenWhiteSpace( t );
		t->next = nullptr;

		if ( last ) { last->next = t; }
		else { define->tokens = t; }

		last = t;
	}
	while ( Parse_ReadLine( source, &token ) );

	if ( last )
	{
		//check for merge operators at the beginning or end
		if ( define->tokens->is( "##" ) || last->is("##" ))
		{
			Parse_SourceError( source, "define with misplaced ##" );
			return false;
		}
	}

	return true;
}

/*
===============
Parse_ReadDirective
===============
*/
directive_t Directives[ 20 ] =
{
	{ "if",        Parse_Directive_if        },
	{ "ifdef",     Parse_Directive_ifdef     },
	{ "ifndef",    Parse_Directive_ifndef    },
	{ "elif",      Parse_Directive_elif      },
	{ "else",      Parse_Directive_else      },
	{ "endif",     Parse_Directive_endif     },
	{ "include",   Parse_Directive_include   },
	{ "define",    Parse_Directive_define    },
	{ "undef",     Parse_Directive_undef     },
	{ "line",      Parse_Directive_line      },
	{ "error",     Parse_Directive_error     },
	{ "pragma",    Parse_Directive_pragma    },
	{ "eval",      Parse_Directive_eval      },
	{ "evalfloat", Parse_Directive_evalfloat },
	{ nullptr,        nullptr                      }
};

static int Parse_ReadDirective( source_t& source )
{
	token_t token;
	int     i;

	//read the directive name
	if ( !Parse_ReadSourceToken( source, &token ) )
	{
		Parse_SourceError( source, "found # without name" );
		return false;
	}

	//directive name must be on the same line
	if ( token.linescrossed > 0 )
	{
		Parse_UnreadSourceToken( source, &token );
		Parse_SourceError( source, "found # at end of line" );
		return false;
	}

	//if it's a name
	if ( token.type == TT_NAME )
	{
		//find the precompiler directive
		for ( i = 0; Directives[ i ].name; i++ )
		{
			if ( !strcmp( Directives[ i ].name, token.c_str() ) )
			{
				return Directives[ i ].func( source );
			}
		}
	}

	Parse_SourceError( source, "unknown precompiler directive %s", token.c_str() );
	return false;
}

/*
===============
Parse_UnreadToken
===============
*/
static void Parse_UnreadToken( source_t& source, token_t *token )
{
	Parse_UnreadSourceToken( source, token );
}

/*
===============
Parse_ReadEnumeration

It is assumed that the 'enum' token has already been consumed
This is fairly basic: it doesn't catch some fairly obvious errors like nested
enums, and enumerated names conflict with #define parameters
===============
*/
static bool Parse_ReadEnumeration( source_t& source )
{
	token_t newtoken;
	int     value;

	if ( !Parse_ReadToken( source, &newtoken ) )
	{
		return false;
	}

	if ( newtoken.type != TT_PUNCTUATION || newtoken.subtype != P_BRACEOPEN )
	{
		Parse_SourceError( source, "Found %s when expecting {",
		                   newtoken.c_str() );
		return false;
	}

	for ( value = 0;; value++ )
	{
		token_t name;

		// read the name
		if ( !Parse_ReadToken( source, &name ) )
		{
			break;
		}

		// it's ok for the enum to end immediately
		if ( name.type == TT_PUNCTUATION && name.subtype == P_BRACECLOSE )
		{
			if ( !Parse_ReadToken( source, &name ) )
			{
				break;
			}

			// ignore trailing semicolon
			if ( name.type != TT_PUNCTUATION || name.subtype != P_SEMICOLON )
			{
				Parse_UnreadToken( source, &name );
			}

			return true;
		}

		// ... but not for it to do anything else
		if ( name.type != TT_NAME )
		{
			Parse_SourceError( source, "Found %s when expecting identifier",
			                   name.c_str() );
			return false;
		}

		if ( !Parse_ReadToken( source, &newtoken ) )
		{
			break;
		}

		if ( newtoken.type != TT_PUNCTUATION )
		{
			Parse_SourceError( source, "Found %s when expecting , or = or }",
			                   newtoken.c_str() );
			return false;
		}

		if ( newtoken.subtype == P_ASSIGN )
		{
			int neg = 1;

			if ( !Parse_ReadToken( source, &newtoken ) )
			{
				break;
			}

			// Parse_ReadToken doesn't seem to read negative numbers, so we do it
			// ourselves
			if ( newtoken.type == TT_PUNCTUATION && newtoken.subtype == P_SUB )
			{
				neg = -1;

				// the next token should be the number
				if ( !Parse_ReadToken( source, &newtoken ) )
				{
					break;
				}
			}

			if ( newtoken.type != TT_NUMBER || !( newtoken.subtype & TT_INTEGER ) )
			{
				Parse_SourceError( source, "Found %s when expecting integer",
				                   newtoken.c_str() );
				return false;
			}

			// this is somewhat silly, but cheap to check
			if ( neg == -1 && ( newtoken.subtype & TT_UNSIGNED ) )
			{
				Parse_SourceWarning( source,
					"Value in enumeration is negative and unsigned" );
			}

			// set the new define value
			value = newtoken.intvalue * neg;

			if ( !Parse_ReadToken( source, &newtoken ) )
			{
				break;
			}
		}

		if ( newtoken.type != TT_PUNCTUATION || ( newtoken.subtype != P_COMMA &&
		     newtoken.subtype != P_BRACECLOSE ) )
		{
			Parse_SourceError( source, "Found %s when expecting , or }",
			                   newtoken.c_str() );
			return false;
		}

		if ( !Parse_AddDefineToSourceFromString( source, va( "%s %d\n", name.c_str(),
		     value ) ) )
		{
			Parse_SourceWarning( source, "Couldn't add define to source: %s = %d",
			                     name.c_str(), value );
			return false;
		}

		if ( newtoken.subtype == P_BRACECLOSE )
		{
			if ( !Parse_ReadToken( source, &name ) )
			{
				break;
			}

			// ignore trailing semicolon
			if ( name.type != TT_PUNCTUATION || name.subtype != P_SEMICOLON )
			{
				Parse_UnreadToken( source, &name );
			}

			return true;
		}
	}

	// got here if a ReadToken returned false
	return false;
}

/*
===============
Parse_ReadToken
===============
*/
static bool Parse_ReadToken( source_t& source, token_t *token )
{
	define_t *define;

	while ( 1 )
	{
		if ( !Parse_ReadSourceToken( source, token ) )
			return false;

		//check for precompiler directives
		if ( token->type == TT_PUNCTUATION && token->startsWith('#'))
		{
			//read the precompiler directive
			if ( !Parse_ReadDirective( source ) )
				return false;
			continue;
		}

		if ( token->type == TT_PUNCTUATION && token->startsWith('$'))
		{
			//read the precompiler directive
			if ( !Parse_ReadDollarDirective( source ) ) 
				return false;
			continue;
		}

		if ( token->type == TT_NAME && token->is("enum"))
		{
			if ( !Parse_ReadEnumeration( source ) )
				return false;
			continue;
		}

		// recursively concatenate strings that are behind each other still resolving defines
		if ( token->type == TT_STRING )
		{
			token_t newtoken;

			if ( Parse_ReadToken( source, &newtoken ) )
			{
				if ( newtoken.type == TT_STRING )
				{
					if (!Parse_MergeTokens(source, *token, newtoken))
						return false;
				}
				else
				{
					Parse_UnreadToken( source, &newtoken );
				}
			}
		}

		//if skipping source because of conditional compilation
		if ( source.skip )
			continue;

		//if the token is a name
		if ( token->type == TT_NAME )
		{
			//check if the name is a define macro
			define = Parse_FindHashedDefine( source.definehash, token->c_str() );

			//if it is a define macro
			if ( define )
			{
				//expand the defined macro
				if ( !Parse_ExpandDefineIntoSource( source, token, define ) )
					return false;
				continue;
			}
		}

		//copy token for unreading
		source.token = *token; // was memcpy
		//found a token
		return true;
	}
}

/*
===============
Parse_DefineFromString
===============
*/
static define_t *Parse_DefineFromString( const char *string )
{
	script_t *script;
	source_t src;
	token_t  *t;
	int      res, i;
	define_t *def;

	script = Parse_LoadScriptMemory( string, strlen( string ), "*extern" );
	//create a new source
	Com_Memset( &src, 0, sizeof( source_t ) );
	Q_strncpyz( src.filename, "*extern", MAX_QPATH );
	src.scriptstack = script;
	src.definehash = (define_t**) Z_Malloc( DEFINEHASHSIZE * sizeof( define_t * ) );
	if (!src.definehash)
		throw std::bad_alloc();
	//create a define from the source
	res = Parse_Directive_define( src );

	//free any tokens if left
	for ( t = src.tokens; t; t = src.tokens )
	{
		src.tokens = src.tokens->next;
		Parse_FreeToken( t );
	}

	def = nullptr;

	for ( i = 0; i < DEFINEHASHSIZE; i++ )
	{
		if ( src.definehash[ i ] )
		{
			def = src.definehash[ i ];
			break;
		}
	}

	//
	Z_Free( src.definehash );
	//
	Parse_FreeScript( *script );

	//if the define was created successfully
	if ( res > 0 ) { return def; }

	//free the define is created
	if ( src.defines ) { Parse_FreeDefine( def ); }

	//
	return nullptr;
}

/*
===============
Parse_AddDefineToSourceFromString
===============
*/
static bool Parse_AddDefineToSourceFromString( source_t& source,
    char *string )
{
	Parse_PushScript( source, Parse_LoadScriptMemory( string, strlen( string ),
	                  "*extern" ) );
	return Parse_Directive_define( source );
}

/*
===============
Parse_AddGlobalDefine

adds or overrides a global define that will be added to all opened sources
===============
*/
bool Parse_AddGlobalDefine( const char *string )
{
	define_t *define, *prev, *curr;

	define = Parse_DefineFromString( string );

	if ( !define )
	{
		return false;
	}

	prev = nullptr;

	for ( curr = globaldefines; curr; curr = curr->next )
	{
		if ( !strcmp( curr->name, define->name ) )
		{
			define->next = curr->next;
			Parse_FreeDefine( curr );

			if ( prev )
			{
				prev->next = define;
			}
			else
			{
				globaldefines = define;
			}

			break;
		}

		prev = curr;
	}

	if ( !curr )
	{
		define->next = globaldefines;
		globaldefines = define;
	}

	return true;
}

/*
===============
Parse_CopyDefine
===============
*/
static define_t *Parse_CopyDefine( source_t& source, define_t *define )
{
	define_t *newdefine;
	token_t  *token, *newtoken, *lasttoken;

	newdefine = ( define_t * ) Z_Malloc( sizeof( define_t ) + strlen( define->name ) + 1 );
	//copy the define name
	newdefine->name = ( char * ) newdefine + sizeof( define_t );
	strcpy( newdefine->name, define->name );
	newdefine->flags = define->flags;
	newdefine->builtin = define->builtin;
	newdefine->numparms = define->numparms;
	//the define is not linked
	newdefine->next = nullptr;
	newdefine->hashnext = nullptr;
	//copy the define tokens
	newdefine->tokens = nullptr;

	for ( lasttoken = nullptr, token = define->tokens; token; token = token->next )
	{
		newtoken = Parse_CopyToken( token );
		newtoken->next = nullptr;

		if ( lasttoken ) { lasttoken->next = newtoken; }
		else { newdefine->tokens = newtoken; }

		lasttoken = newtoken;
	}

	//copy the define parameters
	newdefine->parms = nullptr;

	for ( lasttoken = nullptr, token = define->parms; token; token = token->next )
	{
		newtoken = Parse_CopyToken( token );
		newtoken->next = nullptr;

		if ( lasttoken ) { lasttoken->next = newtoken; }
		else { newdefine->parms = newtoken; }

		lasttoken = newtoken;
	}

	return newdefine;
}

/*
===============
Parse_AddGlobalDefinesToSource
===============
*/
static void Parse_AddGlobalDefinesToSource( source_t& source )
{
	define_t *define, *newdefine;

	for ( define = globaldefines; define; define = define->next )
	{
		newdefine = Parse_CopyDefine( source, define );
		Parse_AddDefineToHash( newdefine, source.definehash );
	}
}

/*
===============
Parse_LoadSourceFile
===============
*/
static source_t* Parse_LoadSourceFile( const char *filename )
{
	script_t *script;

	script = Parse_LoadScriptFile( filename );
	if ( !script ) { return nullptr; }

	script->next = nullptr;

	source_t* pSource = (source_t*) Z_Malloc( sizeof( source_t ) );
	if (!pSource)
		throw std::bad_alloc();
	auto& source = *pSource;
	Q_strncpyz( source.filename, filename, MAX_QPATH );
	source.scriptstack = script;
	source.tokens = nullptr;
	source.defines = nullptr;
	source.indentstack = nullptr;
	source.skip = 0;

	source.definehash = (define_t**) Z_Malloc( DEFINEHASHSIZE * sizeof( define_t * ) );
	Parse_AddGlobalDefinesToSource( source );
	return pSource;
}

/*
===============
Parse_FreeSource
===============
*/
static void Parse_FreeSource( source_t& source )
{
	script_t *script;
	token_t  *token;
	define_t *define;
	indent_t *indent;
	int      i;

	//Parse_PrintDefineHashTable(source.definehash);
	//free all the scripts
	while ( source.scriptstack )
	{
		script = source.scriptstack;
		source.scriptstack = source.scriptstack->next;
		Parse_FreeScript( *script );
	}

	//free all the tokens
	while ( source.tokens )
	{
		token = source.tokens;
		source.tokens = source.tokens->next;
		Parse_FreeToken( token );
	}

	for ( i = 0; i < DEFINEHASHSIZE; i++ )
	{
		while ( source.definehash[ i ] )
		{
			define = source.definehash[ i ];
			source.definehash[ i ] = source.definehash[ i ]->hashnext;
			Parse_FreeDefine( define );
		}
	}

	//free all indents
	while ( source.indentstack )
	{
		indent = source.indentstack;
		source.indentstack = source.indentstack->next;
		Z_Free( indent );
	}

	//
	if ( source.definehash ) { Z_Free( source.definehash ); }

	//free the source itself
	Z_Free( &source );
}

/*
===============
Parse_LoadSourceHandle
===============
*/
int Parse_LoadSourceHandle( const char *filename )
{
	source_t* source;
	int      i;

	for ( i = 1; i < MAX_SOURCEFILES; i++ )
	{
		if ( !sourceFiles[ i ] )
			break;
	}

	if ( i >= MAX_SOURCEFILES )
		return 0;

	source = Parse_LoadSourceFile( filename );

	if ( !source )
		return 0;

	sourceFiles[ i ] = source;
	return i;
}

/*
===============
Parse_FreeSourceHandle
===============
*/
bool Parse_FreeSourceHandle( int handle )
{
	if ( handle < 1 || handle >= MAX_SOURCEFILES )
		return false;

	if ( !sourceFiles[ handle ] )
		return false;

	Parse_FreeSource( *sourceFiles[ handle ] );
	sourceFiles[ handle ] = nullptr;
	return true;
}

/*
===============
Parse_ReadTokenHandle
===============
*/
bool Parse_ReadTokenHandle( int handle, pc_token_t *pc_token )
{
	token_t token;
	bool     ret;

	if ( handle < 1 || handle >= MAX_SOURCEFILES )
		return false;

	if ( !sourceFiles[ handle ] )
		return false;

	ret = Parse_ReadToken( *sourceFiles[ handle ], &token );
	strcpy( pc_token->string, token.c_str() );
	pc_token->type = token.type;
	pc_token->subtype = token.subtype;
	pc_token->intvalue = token.intvalue;
	pc_token->floatvalue = token.floatvalue;

	if ( pc_token->type == TT_STRING )
		Parse_StripDoubleQuotes( pc_token->string );

	return ret;
}

/*
===============
Parse_SourceFileAndLine
===============
*/
bool Parse_SourceFileAndLine( int handle, char *filename, int *line )
{
	if ( handle < 1 || handle >= MAX_SOURCEFILES )
		return false;

	if ( !sourceFiles[ handle ] )
		return false;

	strcpy( filename, sourceFiles[ handle ]->filename );

	if ( sourceFiles[ handle ]->scriptstack )
		*line = sourceFiles[ handle ]->scriptstack->line;
	else
		*line = 0;

	return true;
}
