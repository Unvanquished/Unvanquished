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

#ifndef BOT_PARSE_H_
#define BOT_PARSE_H_

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
	pc_token_list *prev;
	pc_token_list *next;
};

struct AITreeList_t
{
	AIBehaviorTree_t **trees;
	int numTrees;
	int maxTrees;
};

void              InitTreeList( AITreeList_t *list );
void              AddTreeToList( AITreeList_t *list, AIBehaviorTree_t *tree );
void              RemoveTreeFromList( AITreeList_t *list, AIBehaviorTree_t *tree );
void              FreeTreeList( AITreeList_t *list );

pc_token_list *CreateTokenList( int handle );
void           FreeTokenList( pc_token_list *list );

AIGenericNode_t  *ReadNode( pc_token_list **tokenlist );
AIGenericNode_t  *ReadConditionNode( pc_token_list **tokenlist );
AIGenericNode_t  *ReadActionNode( pc_token_list **tokenlist );
AIGenericNode_t  *ReadNodeList( pc_token_list **tokenlist );
AIBehaviorTree_t *ReadBehaviorTree( const char *name, AITreeList_t *list );

void FreeBehaviorTree( AIBehaviorTree_t *tree );
void FreeActionNode( AIActionNode_t *action );
void FreeConditionNode( AIConditionNode_t *node );
void FreeNodeList( AINodeList_t *node );
void FreeNode( AIGenericNode_t *node );
void FreeOp( AIOp_t *op );
void FreeExpression( AIExpType_t *exp );
void FreeValueFunc( AIValueFunc_t *v );
void FreeValue( AIValue_t *v );

#endif
