/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// scriplib.c

#include "cmdlib.h"
#include "mathlib.h"
#include "inout.h"
#include "scriplib.h"
#include "vfs.h"


typedef struct
{
	char            filename[1024];
	char           *buffer, *script_p, *end_p;
	int             line;
} script_t;

#define	MAX_INCLUDES	8
static script_t scriptstack[MAX_INCLUDES];
static script_t *script;

char            token[MAXTOKEN];
qboolean        endofscript;
qboolean        tokenready;		// only qtrue if UnGetToken was just called

/*
==============
AddScriptToStack
==============
*/
void AddScriptToStack(const char *filename, int index)
{
	int             size;

	script++;

	if(script == &scriptstack[MAX_INCLUDES])
		Error("script file exceeded MAX_INCLUDES");

	strcpy(script->filename, ExpandPath(filename));

	size = vfsLoadFile(script->filename, (void **)&script->buffer, index);

	if(size == -1)
		Sys_Printf("Script file %s was not found\n", script->filename);
	else
	{
		if(index > 0)
			Sys_Printf("entering %s (%d)\n", script->filename, index + 1);
		else
			Sys_Printf("entering %s\n", script->filename);
	}

	script->line = 1;

	script->script_p = script->buffer;
	script->end_p = script->buffer + size;
}


/*
==============
LoadScriptFile
==============
*/
void LoadScriptFile(const char *filename, int index)
{
	script = scriptstack;
	AddScriptToStack(filename, index);

	endofscript = qfalse;
	tokenready = qfalse;
}


/*
==============
ParseFromMemory
==============
*/
void ParseFromMemory(char *buffer, int size)
{
	script = scriptstack;
	script++;

	if(script == &scriptstack[MAX_INCLUDES])
		Error("script file exceeded MAX_INCLUDES");

	strcpy(script->filename, "memory buffer");

	script->buffer = buffer;
	script->line = 1;
	script->script_p = script->buffer;
	script->end_p = script->buffer + size;

	endofscript = qfalse;
	tokenready = qfalse;
}


/*
==============
UnGetToken

Signals that the current token was not used, and should be reported
for the next GetToken.  Note that

GetToken (qtrue);
UnGetToken ();
GetToken (qfalse);

could cross a line boundary.
==============
*/
void UnGetToken(void)
{
	tokenready = qtrue;
}


qboolean EndOfScript(qboolean crossline)
{
	if(!crossline)
		Error("Line %i is incomplete\n", script->line);

	if(!strcmp(script->filename, "memory buffer"))
	{
		endofscript = qtrue;
		return qfalse;
	}

	if(script->buffer == NULL)
		Sys_Printf("WARNING: Attempt to free already freed script buffer\n");
	else
		free(script->buffer);
	script->buffer = NULL;
	if(script == scriptstack + 1)
	{
		endofscript = qtrue;
		return qfalse;
	}
	script--;
	Sys_Printf("returning to %s\n", script->filename);
	return GetToken(crossline);
}

/*
==============
GetToken
==============
*/
// *INDENT-OFF*
qboolean GetToken(qboolean crossline)
{
	char           *token_p;

	// ydnar: dummy testing
	if(script == NULL || script->buffer == NULL)
		return qfalse;

	if(tokenready)				// is a token already waiting?
	{
		tokenready = qfalse;
		return qtrue;
	}

	if((script->script_p >= script->end_p) || (script->script_p == NULL))
		return EndOfScript(crossline);

skipspace:
	// skip whitespace
	while(*script->script_p <= ' ')
	{
		if(script->script_p >= script->end_p)
			return EndOfScript(crossline);
		
		if(*script->script_p++ == '\n')
		{
			if(!crossline)
				Error("Line %i is incomplete\n", script->line);
			
			script->line++;
		}
	}

	if(script->script_p >= script->end_p)
		return EndOfScript(crossline);

	// skip ; # // comments
	if(*script->script_p == ';' || *script->script_p == '#' || (*script->script_p == '/' && script->script_p[1] == '/'))
	{
		if(!crossline)
			Error("Line %i is incomplete\n", script->line);
		
		while(*script->script_p && *script->script_p++ != '\n')
		{
			if(script->script_p >= script->end_p)
				return EndOfScript(crossline);
		}
		script->line++;
		goto skipspace;
	}

	// skip /* */ comments
#if 1
	if(*script->script_p == '/' && script->script_p[1] == '*')
	{
		if(!crossline)
			Error("Line %i is incomplete\n", script->line);
		
		script->script_p += 2;
		while(*script->script_p && (*script->script_p != '*' || script->script_p[1] != '/'))
		{
			if(*script->script_p == '\n')
			{
				script->line++;
			}
			script->script_p++;
			
			if(script->script_p >= script->end_p)
				return EndOfScript(crossline);
		}
		if(*script->script_p)
		{
			script->script_p += 2;
		}
		goto skipspace;
	}
#endif

	// copy token
	token_p = token;

	// check for a quotation
	if(*script->script_p == '"')
	{
		// quoted token
		script->script_p++;
		while(*script->script_p != '"')
		{
			*token_p++ = *script->script_p++;
			
			if(script->script_p == script->end_p)
				break;
			
			if(token_p == &token[MAXTOKEN])
				Error("Token too large on line %i\n", script->line);
		}
		script->script_p++;
	}
	// check for a number
	else if((*script->script_p >= '0' && *script->script_p <= '9')									||
			(*script->script_p == '-' && script->script_p[1] >= '0' && script->script_p[1] <= '9')	||
			(*script->script_p == '+' && script->script_p[1] >= '0' && script->script_p[1] <= '9')	||
			(*script->script_p == '.' && script->script_p[1] >= '0' && script->script_p[1] <= '9')	||
			(*script->script_p == '-' && script->script_p[1] == '.' && script->script_p[2] >= '0' && script->script_p[2] <= '9')
	)
	{
		do
		{
			*token_p++ = *script->script_p++;
			
			if(script->script_p == script->end_p)
				break;
			
			if(token_p == &token[MAXTOKEN])
				Error("Token too large on line %i\n", script->line);
		
		} while((*script->script_p >= '0' && *script->script_p <= '9') || *script->script_p == '.' );

		// parse the exponent
		if(*script->script_p == 'e' || *script->script_p == 'E')
		{
			*token_p++ = *script->script_p;
			
			script->script_p++;
			if(*script->script_p == '-' || *script->script_p == '+')
			{
				*token_p++ = *script->script_p++;
			}

			do
			{
				*token_p++ = *script->script_p++;
			
				if(script->script_p == script->end_p)
					break;
			
				if(token_p == &token[MAXTOKEN])
					Error("Token too large on line %i\n", script->line);
				
			} while(*script->script_p >= '0' && *script->script_p <= '9');
		}
	}
	// check for a regular word
	// we still allow forward and back slashes in name tokens for pathnames
	// and also colons for drive letters
	else if((*script->script_p >= 'a' && *script->script_p <= 'z')	||
			(*script->script_p >= 'A' && *script->script_p <= 'Z')	||
			(*script->script_p == '_')								||
			(*script->script_p == '/')								||
			(*script->script_p == '\\')								||
			(*script->script_p == '$')	)
	{
		do
		{
			*token_p++ = *script->script_p++;
			
			if(script->script_p == script->end_p)
				break;
			
			if(token_p == &token[MAXTOKEN])
				Error("Token too large on line %i\n", script->line);
		}
		while
		(
			   (*script->script_p >= 'a' && *script->script_p <= 'z')	||
			   (*script->script_p >= 'A' && *script->script_p <= 'Z')	||
			   (*script->script_p == '_') 								||
			   (*script->script_p == '-') 								||
			   (*script->script_p >= '0' && *script->script_p <= '9')	||
			   (*script->script_p == '/')								||
			   (*script->script_p == '\\')								||
			   (*script->script_p == ':')								||
			   (*script->script_p == '.')								||
			   (*script->script_p == '$')								||
			   (*script->script_p == '@')
		);
	}
	else
	{
		// single character punctuation
		*token_p++ = *script->script_p++;
		
		if(token_p == &token[MAXTOKEN])
			Error("Token too large on line %i\n", script->line);
	}

	// add tailing zero
	*token_p = 0;

	if(!strcmp(token, "$include"))
	{
		GetToken(qfalse);
		AddScriptToStack(token, 0);
		return GetToken(crossline);
	}

	return qtrue;
}
// *INDENT-ON*

/*
==============
GetLine
==============
*/
int GetLine(void)
{
	return script->line;
}

/*
==============
TokenAvailable

Returns qtrue if there is another token on the line
==============
*/
qboolean TokenAvailable(void)
{
	int             oldLine;
	qboolean        r;

	oldLine = script->line;
	r = GetToken(qtrue);
	if(!r)
	{
		return qfalse;
	}
	UnGetToken();
	if(oldLine == script->line)
	{
		return qtrue;
	}
	return qfalse;
}


void MatchToken(char *match)
{
	GetToken(qtrue);

	if(strcmp(token, match))
	{
		Error("MatchToken: \"%s\" != \"%s\" at line %i in file '%s'", token, match, script->line, script->filename);
	}
}

/*
=================
SkipBracedSection

The next token should be an open brace.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
void SkipBracedSection()
{
	int             depth;

	depth = 0;
	do
	{
		GetToken(qtrue);
		if(token[1] == 0)
		{
			if(token[0] == '{')
			{
				depth++;
			}
			else if(token[0] == '}')
			{
				depth--;
			}
		}
	} while(depth && script->script_p < script->end_p);
}

/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine()
{
	char           *p;
	int             c;

	p = script->script_p;
	while((c = *p++) != 0)
	{
		if(c == '\n')
		{
			script->line++;
			break;
		}
	}

	script->script_p = p;
}

void Parse1DMatrix(int x, vec_t * m)
{
	int             i;

	MatchToken("(");

	for(i = 0; i < x; i++)
	{
		GetToken(qfalse);
		m[i] = atof(token);
	}

	MatchToken(")");
}

void Parse2DMatrix(int y, int x, vec_t * m)
{
	int             i;

	MatchToken("(");

	for(i = 0; i < y; i++)
	{
		Parse1DMatrix(x, m + i * x);
	}

	MatchToken(")");
}

void Parse3DMatrix(int z, int y, int x, vec_t * m)
{
	int             i;

	MatchToken("(");

	for(i = 0; i < z; i++)
	{
		Parse2DMatrix(y, x, m + i * x * y);
	}

	MatchToken(")");
}


void Write1DMatrix(FILE * f, int x, vec_t * m)
{
	int             i;

	fprintf(f, " ( ");
	for(i = 0; i < x; i++)
	{
		if(m[i] == (int)m[i])
		{
			fprintf(f, "%i ", (int)m[i]);
		}
		else
		{
			fprintf(f, "%f ", m[i]);
		}
	}
	fprintf(f, " ) ");
}

void Write2DMatrix(FILE * f, int y, int x, vec_t * m)
{
	int             i;

	fprintf(f, " ( ");
	for(i = 0; i < y; i++)
	{
		Write1DMatrix(f, x, m + i * x);
		fprintf(f, " ");
	}
	fprintf(f, " ) ");
}


void Write3DMatrix(FILE * f, int z, int y, int x, vec_t * m)
{
	int             i;

	fprintf(f, "(\n");
	for(i = 0; i < z; i++)
	{
		Write2DMatrix(f, y, x, m + i * (x * y));
	}
	fprintf(f, ")\n");
}
