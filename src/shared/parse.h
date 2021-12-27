/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

#ifndef SHARED_PARSE_H_
#define SHARED_PARSE_H_

#include "engine/qcommon/q_shared.h"

//token types
enum class tokenType_t {
    TT_STRING, // string
    TT_LITERAL, // literal
    TT_NUMBER, // number
    TT_NAME, // name
    TT_PUNCTUATION, // punctuation
};

struct pc_token_t {
    tokenType_t type;
    int   subtype;
    int   intvalue;
    float floatvalue;
    char  string[ 1024 ];
    int   line;
    int   linescrossed;
};

int Parse_AddGlobalDefine(const char *string);
int Parse_LoadSourceHandle(const char *filename, int (*openFunc)(Str::StringRef, fileHandle_t &));
int Parse_FreeSourceHandle(int handle);
bool Parse_ReadTokenHandle(int handle, pc_token_t *pc_token);
int Parse_SourceFileAndLine(int handle, char (&filename)[MAX_QPATH], int *line);

/*
===============
Parse_WordListSplitter

Tokenize a list like "a, b, c" to "a", "b", "c".

This is a partial implementation of an input interator: it can only go forward,
and you don't know how many elements you will be able to read until you've read
them all.

Exemple usage:
	for (Parse_WordListSplitter iterator(string); *iterator; ++iterator)
	{
		printf("C str: »%s«\n", *parser);
	}

This parser is quite tolerant: you can use one delimiter or another,
or a variant of that: "a b,c ,d,,e    f" is equivalent to "a, b, c, d, e, f".

The const char * it returns for each element (operator*) are not valid once
the destructor is called.
===============
*/
// We could have a template here to specify the delimiter.
// But compilation times are large enough as is.
class Parse_WordListSplitter {
public:
	Parse_WordListSplitter(std::string str);

	const char *operator*() const;

	Parse_WordListSplitter& operator++(); // only ++i is implemented. there is no i++.

private:
	std::string _string; // the string
	size_t _start; // start of this chunk
	size_t _stop;  // stop of this chunk
};

#endif // SHARED_PARSE_H_
