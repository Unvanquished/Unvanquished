/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.

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

#ifndef __BOT_PARSE_HEADER
#define __BOT_PARSE_HEADER

#include "shared/parse.h"
#include "sg_bot_ai.h"

#define allocNode(T) ( T * ) BG_Alloc( sizeof( T ) );
#define stringify2(T, val) va( #T " %d", val )
#define D2(T, val) Parse_AddGlobalDefine( stringify2( T, val ) )
#define D(T) D2(T, T)

// FIXME: copied from parse.c
#define P_LOGIC_AND        4
#define P_LOGIC_OR         5
#define P_LOGIC_GEQ        6
#define P_LOGIC_LEQ        7
#define P_LOGIC_EQ         8
#define P_LOGIC_UNEQ       9

#define P_LOGIC_NOT        35
#define P_LOGIC_GREATER    36
#define P_LOGIC_LESS       37

struct pc_token_stripped_t
{
	tokenType_t type;
	int   subtype;
	int   intvalue;
	float floatvalue;
	char  *string;
	int   line;
};

struct pc_token_list
{
	pc_token_stripped_t token;
	struct pc_token_list *prev;
	struct pc_token_list *next;
};

pc_token_list *CreateTokenList( int handle );
void           FreeTokenList( pc_token_list *list );

std::shared_ptr<AIGenericNode>  ReadNode( pc_token_list **tokenlist );
std::shared_ptr<AIBehaviorTree> ReadBehaviorTree( const char *name, AITreeList *tree );

struct parseError {
	parseError(const char *_message, int _line = 0)
		: message(_message), line(_line) {};
	const char *message;
	int line;
};
void WarnAboutParseError( parseError p );

#endif
