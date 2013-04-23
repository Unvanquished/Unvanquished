/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

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
#include "g_bot.h"

/*
=======================
Behavior Tree Creation
=======================
*/

static void BotInitNode( AINode_t type, AINodeRunner func, void *node )
{
	AIGenericNode_t *n = ( AIGenericNode_t * ) node;
	n->type = type;
	n->run = func;
}

#define allocNode( T ) ( T * ) BG_Alloc( sizeof( T ) );
#define stringify( v ) va( #v " %d", v )
#define D( T ) trap_Parse_AddGlobalDefine( stringify( T ) )

// FIXME: copied from parse.c
#define P_LOGIC_GEQ        7
#define P_LOGIC_LEQ        8
#define P_LOGIC_EQ         9
#define P_LOGIC_UNEQ       10

#define P_LOGIC_NOT        36
#define P_LOGIC_GREATER    37
#define P_LOGIC_LESS       38

typedef struct pc_token_list_s
{
	pc_token_t token;
	struct pc_token_list_s *prev;
	struct pc_token_list_s *next;
} pc_token_list;

static pc_token_list *CreateTokenList( int handle )
{
	pc_token_t token;
	char filename[ MAX_QPATH ];
	pc_token_list *current = NULL;
	pc_token_list *root = NULL;

	while ( trap_Parse_ReadToken( handle, &token ) )
	{
		pc_token_list *list = ( pc_token_list * ) BG_Alloc( sizeof( pc_token_list ) );
		
		if ( current )
		{
			list->prev = current;
			current->next = list;
		}
		else
		{
			list->prev = list;
			root = list;
		}
		
		current = list;
		current->next = NULL;

		current->token = token;
		trap_Parse_SourceFileAndLine( handle, filename, &current->token.line );
	}

	return root;
}

static void FreeTokenList( pc_token_list *list )
{
	pc_token_list *current = list;
	while( current )
	{
		pc_token_list *node = current;
		current = current->next;

		BG_Free( node );
	}
}

static AIGenericNode_t *ReadNode( pc_token_list **tokenlist );
static void FreeNode( AIGenericNode_t *node );

static void CheckToken( const char *tokenValueName, const char *nodename, const pc_token_t *token, int requiredType )
{
	if ( token->type != requiredType )
	{
		BotDPrintf( S_COLOR_RED "ERROR: Invalid %s %s after %s on line %d\n", tokenValueName, token->string, nodename, token->line );
	}
}

static AIConditionOperator_t ReadConditionOperator( pc_token_list *tokenlist )
{
	CheckToken( "operator", "condition", &tokenlist->token, TT_PUNCTUATION );

	switch( tokenlist->token.subtype )
	{
		case P_LOGIC_LESS:
			return OP_LESSTHAN;
		case P_LOGIC_LEQ:
			return OP_LESSTHANEQUAL;
		case P_LOGIC_GREATER:
			return OP_GREATERTHAN;
		case P_LOGIC_GEQ:
			return OP_GREATERTHANEQUAL;
		case P_LOGIC_EQ:
			return OP_EQUAL;
		case P_LOGIC_UNEQ:
			return OP_NEQUAL;
		case P_LOGIC_NOT:
			return OP_NOT;
		default:
			return OP_BOOL;
	}
}

/*
======================
ReadConditionNode

Parses and creates an AIConditionNode_t from a token list
The token list pointer is modified to point to the beginning of the next node text block

A condition node has the form:
condition [expression] {
	child node
}

or the form:
condition [expression]

[expression] can be either builtinValue, !builtinValue, or builtinValue [op] constant
where [op] is one of >=, <=, ==, !=
depending on the context

FIXME: allow arbitrary mixing of expressions and operators instead of hardcoding the options
======================
*/
static AIGenericNode_t *ReadConditionNode( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;
	AIConditionNode_t *condition;

	condition = allocNode( AIConditionNode_t );
	BotInitNode( CONDITION_NODE, BotConditionNode, condition );

	// read not or bool
	if ( current->token.subtype == P_LOGIC_NOT )
	{
		condition->op = OP_NOT;
		current = current->next;
		if ( !current )
		{
			BotDPrintf( S_COLOR_RED "ERROR: no token after operator not\n" );
		}
	}
	else
	{
		condition->op = OP_BOOL;
	}

	if ( !Q_stricmp( current->token.string, "buildingIsDamaged" ) )
	{
		condition->info = CON_DAMAGEDBUILDING;
	}
	else if ( !Q_stricmp( current->token.string, "alertedToEnemy" ) )
	{
		condition->info = CON_ENEMY;
	}
	else if ( !Q_stricmp( current->token.string, "haveWeapon" ) )
	{
		condition->info = CON_WEAPON;

		current = current->next;

		CheckToken( "weapon", "condition haveWeapon", &current->token, TT_NUMBER );

		condition->param1.type = VALUE_INT;
		condition->param1.value.i = current->token.intvalue;
	}
	else if ( !Q_stricmp( current->token.string, "haveUpgrade" ) )
	{
		condition->info = CON_UPGRADE;

		current = current->next;

		CheckToken( "upgrade", "condition haveUpgrade", &current->token, TT_NUMBER );

		condition->param1.type = VALUE_INT;
		condition->param1.value.i = current->token.intvalue;
	}
	else if ( !Q_stricmp( current->token.string, "teamateHasWeapon" ) )
	{
		condition->info = CON_TEAM_WEAPON;

		current = current->next;

		CheckToken( "weapon", "condition teamateHasWeapon", &current->token, TT_NUMBER );

		condition->param1.type = VALUE_INT;
		condition->param1.value.i = current->token.intvalue;
	}
	else if ( !Q_stricmp( current->token.string, "botTeam" ) )
	{
		condition->info = CON_TEAM;

		current = current->next;
		condition->op = ReadConditionOperator( current );

		current = current->next;

		CheckToken( "team", "condition botTeam", &current->token, TT_NUMBER );

		condition->param1.type = VALUE_INT;
		condition->param1.value.i = current->token.intvalue;
	}
	else if ( !Q_stricmp( current->token.string, "percentHealth" ) )
	{
		condition->info = CON_HEALTH;

		current = current->next;
		condition->op = ReadConditionOperator( current );

		current = current->next;

		CheckToken( "value", "condition percentHealth", &current->token, TT_NUMBER );

		condition->param1.type = VALUE_FLOAT;
		condition->param1.value.f = current->token.floatvalue;
	}
	else if ( !Q_stricmp( current->token.string, "weapon" ) )
	{
		condition->info = CON_CURWEAPON;

		current = current->next;
		condition->op = ReadConditionOperator( current );

		current = current->next;

		CheckToken( "weapon", "condition weapon", &current->token, TT_NUMBER );

		condition->param1.type = VALUE_INT;
		condition->param1.value.i = current->token.intvalue;
	}
	else if ( !Q_stricmp( current->token.string, "distanceTo" ) )
	{
		condition->info = CON_DISTANCE;

		current = current->next;

		CheckToken( "entity", "condition distanceTo", &current->token, TT_NUMBER );

		condition->param1.value.i = current->token.intvalue;

		current = current->next;

		condition->op = ReadConditionOperator( current );

		current = current->next;

		CheckToken( "value", "condition distanceTo", &current->token, TT_NUMBER );

		condition->param2.value.i = current->token.intvalue;
	}
	else if ( !Q_stricmp ( current->token.string, "baseRushScore" ) )
	{
		condition->info = CON_BASERUSH;

		current = current->next;

		condition->op = ReadConditionOperator( current );

		current = current->next;

		CheckToken( "value", "condition baseRushScore", &current->token, TT_NUMBER );

		condition->param1.value.f = current->token.floatvalue;
	}
	else if ( !Q_stricmp( current->token.string, "healScore" ) )
	{
		condition->info = CON_HEAL;

		current = current->next;

		condition->op = ReadConditionOperator( current );

		current = current->next;

		CheckToken( "value", "condition healScore", &current->token, TT_NUMBER );

		condition->param1.value.f = current->token.floatvalue;
	}
	else
	{
		BotDPrintf( S_COLOR_RED "ERROR: unknown condition %s\n", current->token.string );
	}

	current = current->next;

	if ( Q_stricmp( current->token.string, "{" ) )
	{
		// this condition node has no child nodes
		*tokenlist = current;
		return ( AIGenericNode_t * ) condition;
	}

	current = current->next;

	condition->child = ReadNode( &current );

	if ( Q_stricmp( current->token.string, "}" ) )
	{
		BotDPrintf( S_COLOR_RED "ERROR: invalid token on line %d found %s expected }\n", current->token.line, current->token.string );
	}

	*tokenlist = current->next;

	return ( AIGenericNode_t * ) condition;
}

/*
======================
ReadActionNode

Parses and creates an AIGenericNode_t with the type ACTION_NODE from a token list
The token list pointer is modified to point to the beginning of the next node text block after reading

An action node has the form:
action [type]

Where [type] defines which action to execute
======================
*/
static AIGenericNode_t *ReadActionNode( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;

	AIGenericNode_t *node;

	if ( !Q_stricmp( current->token.string, "heal" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionHeal, node );
	}
	else if ( !Q_stricmp( current->token.string, "fight" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionFight, node );
	}
	else if ( !Q_stricmp( current->token.string, "roam" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionRoam, node );
	}
	else if ( !Q_stricmp( current->token.string, "equip" ) )
	{
		AIBuyNode_t *realAction = allocNode( AIBuyNode_t );
		realAction->numUpgrades = 0;
		realAction->weapon = WP_NONE;

		BotInitNode( ACTION_NODE, BotActionBuy, realAction );
		node = ( AIGenericNode_t * ) realAction;
	}
	else if ( !Q_stricmp( current->token.string, "buy" ) )
	{
		AIBuyNode_t *realAction = allocNode( AIBuyNode_t );

		current = current->next;

		CheckToken( "weapon", "action buy", &current->token, TT_NUMBER );

		realAction->weapon = ( weapon_t ) current->token.intvalue;
		realAction->numUpgrades = 0;

		BotInitNode( ACTION_NODE, BotActionBuy, realAction );
		node = ( AIGenericNode_t * ) realAction;
	}
	else if ( !Q_stricmp( current->token.string, "flee" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionFlee, node );
	}
	else if ( !Q_stricmp( current->token.string, "repair" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionRepair, node );
	}
	else if ( !Q_stricmp( current->token.string, "evolve" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionEvolve, node );
	}
	else if ( !Q_stricmp( current->token.string, "rush" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionRush, node );
	}
	else
	{
		BotDPrintf( S_COLOR_RED "ERROR: Invalid token %s on line %d\n", current->token.string, current->token.line );
	}

	*tokenlist = current->next;
	return ( AIGenericNode_t * ) node;
}

/*
======================
ReadNodeList

Parses and creates an AINodeList_t from a token list
The token list pointer is modified to point to the beginning of the next node text block after reading

A node list has one of these forms:
selector sequence {
[ one or more child nodes ]
}

selector priority {
[ one or more child nodes ]
}

selector {
[ one or more child nodes ]
}
======================
*/
static AIGenericNode_t *ReadNodeList( pc_token_list **tokenlist )
{
	AINodeList_t *list;
	pc_token_list *current = *tokenlist;

	list = allocNode( AINodeList_t );

	if ( !Q_stricmp( current->token.string, "sequence" ) )
	{
		BotInitNode( SELECTOR_NODE, BotSequenceNode, list );
		current = current->next;
	}
	else if ( !Q_stricmp( current->token.string, "priority" ) )
	{
		BotInitNode( SELECTOR_NODE, BotPriorityNode, list );
		current = current->next;
	}
	else if ( !Q_stricmp( current->token.string, "{" ) )
	{
		BotInitNode( SELECTOR_NODE, BotSelectorNode, list );
	}
	else
	{
		BotDPrintf( S_COLOR_RED "ERROR: Invalid token %s on line %d\n", current->token.string, current->token.line );
	}

	if ( Q_stricmp( current->token.string, "{" ) )
	{
		BotDPrintf( S_COLOR_RED "ERROR: Invalid token %s on line %d\n", current->token.string, current->token.line );
	}

	current = current->next;

	while ( Q_stricmp( current->token.string, "}" ) )
	{
		AIGenericNode_t *node = ReadNode( &current );

		if ( node && list->numNodes >= MAX_NODE_LIST )
		{
			BotDPrintf( "ERROR: Max selector children limit exceeded at line %d\n", current->token.line );
		}
		else if ( node )
		{
			list->list[ list->numNodes ] = node;
			list->numNodes++;
		}
	}

	*tokenlist = current->next;
	return ( AIGenericNode_t * ) list;
}

/*
======================
ReadNode

Parses and creates an AIGenericNode_t from a token list
The token list pointer is modified to point to the next node text block after reading

This function delegates the reading to the sub functions
ReadNodeList, ReadActionNode, and ReadConditionNode depending on the first token in the list
======================
*/

static AIGenericNode_t *ReadNode( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;
	AIGenericNode_t *node;

	if ( !Q_stricmp( current->token.string, "selector" ) )
	{
		current = current->next;
		node = ReadNodeList( &current );
	}
	else if ( !Q_stricmp( current->token.string, "action" ) )
	{
		current = current->next;
		node = ReadActionNode( &current );
	}
	else if ( !Q_stricmp( current->token.string, "condition" ) )
	{
		current = current->next;
		node = ReadConditionNode( &current );
	}
	else
	{
		BotDPrintf( S_COLOR_RED "ERROR: invalid token on line %d found: %s\n", current->token.line, current->token.string );
		node = NULL;
	}

	*tokenlist = current;
	return node;
}

void InitTreeList( AITreeList_t *list )
{
	list->trees = ( AIBehaviorTree_t ** ) BG_Alloc( sizeof( AIBehaviorTree_t * ) * 10 );
	list->maxTrees = 10;
	list->numTrees = 0;
}

void AddTreeToList( AITreeList_t *list, AIBehaviorTree_t *tree )
{
	if ( list->maxTrees == list->numTrees )
	{
		AIBehaviorTree_t **trees = ( AIBehaviorTree_t ** ) BG_Alloc( sizeof( AIBehaviorTree_t * ) * list->maxTrees );
		list->maxTrees *= 2;
		memcpy( trees, list->trees, sizeof( AIBehaviorTree_t * ) * list->numTrees );
		BG_Free( list->trees );
		list->trees = trees;
	}

	list->trees[ list->numTrees ] = tree;
	list->numTrees++;
}

void RemoveTreeFromList( AITreeList_t *list, AIBehaviorTree_t *tree )
{
	int i;

	for ( i = 0; i < list->numTrees; i++ )
	{
		AIBehaviorTree_t *testTree = list->trees[ i ];
		if ( !Q_stricmp( testTree->name, tree->name ) )
		{
			FreeBehaviorTree( testTree );
			memmove( &list->trees[ i ], &list->trees[ i + 1 ], sizeof( AIBehaviorTree_t * ) * ( list->numTrees - i - 1 ) );
			list->numTrees--;
		}
	}
}

void FreeTreeList( AITreeList_t *list )
{
	int i;
	for ( i = 0; i < list->numTrees; i++ )
	{
		AIBehaviorTree_t *tree = list->trees[ i ];
		FreeBehaviorTree( tree );
	}

	BG_Free( list->trees );
	list->trees = NULL;
	list->maxTrees = 0;
	list->numTrees = 0;
}

/*
======================
ReadBehaviorTree

Load a behavior tree of the given name from a file
======================
*/
AIBehaviorTree_t * ReadBehaviorTree( const char *name, AITreeList_t *list )
{
	int i;
	char treefilename[ MAX_QPATH ];
	int handle;
	pc_token_list *tokenlist;
	AIBehaviorTree_t *tree;
	pc_token_list *current;
	AIGenericNode_t *node;

	// check if this behavior tree has already been loaded
	for ( i = 0; i < list->numTrees; i++ )
	{
		AIBehaviorTree_t *tree = list->trees[ i ];
		if ( !Q_stricmp( tree->name, name ) )
		{
			return tree;
		}
	}

	// add preprocessor defines for use in the behavior tree
	// add upgrades
	D( UP_LIGHTARMOUR );
	D( UP_HELMET );
	D( UP_MEDKIT );
	D( UP_BATTPACK );
	D( UP_JETPACK );
	D( UP_BATTLESUIT );
	D( UP_GRENADE );

	// add weapons
	D( WP_MACHINEGUN );
	D( WP_PAIN_SAW );
	D( WP_SHOTGUN );
	D( WP_LAS_GUN );
	D( WP_MASS_DRIVER );
	D( WP_CHAINGUN );
	D( WP_FLAMER );
	D( WP_PULSE_RIFLE );
	D( WP_LUCIFER_CANNON );
	D( WP_GRENADE );
	D( WP_HBUILD );

	// add teams
	D( TEAM_ALIENS );
	D( TEAM_HUMANS );

	// add AIEntitys
	D( E_NONE );
	D( E_A_SPAWN );
	D( E_A_OVERMIND );
	D( E_A_BARRICADE );
	D( E_A_ACIDTUBE );
	D( E_A_TRAPPER );
	D( E_A_BOOSTER );
	D( E_A_HIVE );
	D( E_H_SPAWN );
	D( E_H_MGTURRET );
	D( E_H_TESLAGEN );
	D( E_H_ARMOURY );
	D( E_H_DCC );
	D( E_H_MEDISTAT );
	D( E_H_REACTOR );
	D( E_H_REPEATER );
	D( E_GOAL );
	D( E_ENEMY );
	D( E_DAMAGEDBUILDING );

	Q_strncpyz( treefilename, va( "bots/%s.bt", name ), sizeof( treefilename ) );

	handle = trap_Parse_LoadSource( treefilename );
	if ( !handle )
	{
		G_Printf( S_COLOR_RED "ERROR: Cannot load behavior tree %s: File not found\n", treefilename );
		return NULL;
	}

	tokenlist = CreateTokenList( handle );
	
	tree = ( AIBehaviorTree_t * ) BG_Alloc( sizeof( AIBehaviorTree_t ) );

	Q_strncpyz( tree->name, name, sizeof( tree->name ) );

	current = tokenlist;

	node = ReadNode( &current );
	if ( node )
	{
		tree->root = ( AINode_t * ) node;
	}
	else
	{
		BG_Free( tree );
		tree = NULL;
	}

	if ( tree )
	{
		AddTreeToList( list, tree );
	}

	FreeTokenList( tokenlist );
	trap_Parse_FreeSource( handle );
	return tree;
}

static void FreeConditionNode( AIConditionNode_t *node )
{
	FreeNode( node->child );
	BG_Free( node );
}

static void FreeNodeList( AINodeList_t *node )
{
	int i;
	for ( i = 0; i < node->numNodes; i++ )
	{
		FreeNode( node->list[ i ] );
	}
	BG_Free( node );
}

static void FreeNode( AIGenericNode_t *node )
{
	if ( !node )
	{
		return;
	}

	if ( node->type == SELECTOR_NODE )
	{
		FreeNodeList( ( AINodeList_t * ) node );
	}
	else if ( node->type == CONDITION_NODE )
	{
		FreeConditionNode( ( AIConditionNode_t * ) node );
	}
	else if ( node->type == ACTION_NODE )
	{
		BG_Free( node );
	}
}

void FreeBehaviorTree( AIBehaviorTree_t *tree )
{
	if ( tree )
	{
		FreeNode( ( AIGenericNode_t * ) tree->root );

		BG_Free( tree );
	}
	else
	{
		G_Printf( "WARNING: Attempted to free NULL behavior tree\n" );
	}
}

static qboolean NodeIsRunning( gentity_t *self, AIGenericNode_t *node )
{
	int i;
	for ( i = 0; i < self->botMind->numRunningNodes; i++ )
	{
		if ( self->botMind->runningNodes[ i ] == node )
		{
			return qtrue;
		}
	}
	return qfalse;
}

/*
	Behavior tree control-flow nodes
*/
AINodeStatus_t BotSelectorNode( gentity_t *self, AIGenericNode_t *node )
{
	AINodeList_t *selector = ( AINodeList_t * ) node;
	int i;

	i = 0;

	// find a previously running node and start there
	for ( i = selector->numNodes - 1; i > 0; i-- )
	{
		if ( NodeIsRunning( self, selector->list[ i ] ) )
		{
			break;
		}
	}

	for ( ; i < selector->numNodes; i++ )
	{
		AINodeStatus_t status = BotEvaluateNode( self, selector->list[ i ] );
		if ( status == STATUS_FAILURE )
		{
			continue;
		}
		return status;
	}
	return STATUS_FAILURE;
}

AINodeStatus_t BotSequenceNode( gentity_t *self, AIGenericNode_t *node )
{
	AINodeList_t *sequence = ( AINodeList_t * ) node;
	int i = 0;

	// find a previously running node and start there
	for ( i = sequence->numNodes - 1; i > 0; i-- )
	{
		if ( NodeIsRunning( self, sequence->list[ i ] ) )
		{
			break;
		}
	}

	for ( ; i < sequence->numNodes; i++ )
	{
		AINodeStatus_t status = BotEvaluateNode( self, sequence->list[ i ] );
		if ( status == STATUS_FAILURE )
		{
			return STATUS_FAILURE;
		}

		if ( status == STATUS_RUNNING )
		{
			return STATUS_RUNNING;
		}
	}
	return STATUS_SUCCESS;
}

AINodeStatus_t BotPriorityNode( gentity_t *self, AIGenericNode_t *node )
{
	AINodeList_t *priority = ( AINodeList_t * ) node;
	int i;

	for ( i = 0; i < priority->numNodes; i++ )
	{
		AINodeStatus_t status = BotEvaluateNode( self, priority->list[ i ] );
		if ( status == STATUS_FAILURE )
		{
			continue;
		}
		return status;
	}
	return STATUS_FAILURE;
}

AINodeStatus_t BotParallelNode( gentity_t *self, AIGenericNode_t *node )
{
	AINodeList_t *parallel = ( AINodeList_t * ) node;
	int i = 0;
	int numFailure = 0;

	for ( ; i < parallel->numNodes; i++ )
	{
		AINodeStatus_t status = BotEvaluateNode( self, parallel->list[ i ] );

		if ( status == STATUS_FAILURE )
		{
			numFailure++;

			if ( numFailure < parallel->maxFail )
			{
				continue;
			}
		}

		return status;
	}
	return STATUS_FAILURE;
}

#define BotCompare( leftValue, rightValue, op, success ) \
do \
{\
	success = qfalse;\
	switch ( op )\
	{\
		case OP_LESSTHAN:\
			if ( leftValue < rightValue )\
			{\
				success = qtrue;\
			}\
			break;\
		case OP_LESSTHANEQUAL:\
			if ( leftValue <= rightValue )\
			{\
				success = qtrue;\
			}\
			break;\
		case OP_GREATERTHAN:\
			if ( leftValue > rightValue )\
			{\
				success = qtrue;\
			}\
			break;\
		case OP_GREATERTHANEQUAL:\
			if ( leftValue >= rightValue )\
			{\
				success = qtrue;\
			}\
			break;\
		case OP_EQUAL:\
			if ( leftValue == rightValue )\
			{\
				success = qtrue;\
			}\
			break;\
		case OP_NEQUAL:\
			if ( leftValue != rightValue )\
			{\
				success = qtrue;\
			}\
			break;\
		case OP_NONE:\
		case OP_NOT:\
		case OP_BOOL:\
			break;\
	}\
} while ( 0 );

#define BotBoolCondition( left, op, s ) \
do \
{ \
	s = qfalse; \
	switch( op )\
	{\
	case OP_BOOL:\
		if ( left )\
		{\
			s = qtrue;\
		}\
		break;\
	case OP_NOT:\
		if ( !left )\
		{\
			s = qtrue;\
		}\
		break;\
	case OP_NONE:\
	case OP_LESSTHAN:\
	case OP_LESSTHANEQUAL:\
	case OP_GREATERTHAN:\
	case OP_GREATERTHANEQUAL:\
	case OP_EQUAL:\
	case OP_NEQUAL:\
		break;\
	}\
} while( 0 );

// TODO: rewrite how condition nodes work in order to increase their flexibility
AINodeStatus_t BotConditionNode( gentity_t *self, AIGenericNode_t *node )
{
	qboolean success = qfalse;

	AIConditionNode_t *con = ( AIConditionNode_t * ) node;

	switch ( con->info )
	{
		case CON_BUILDING:
		{
			gentity_t *building = NULL;
			building = self->botMind->closestBuildings[ con->param1.value.i ].ent;
			BotBoolCondition( building, con->op, success );
		}
		break;
		case CON_WEAPON:
			BotBoolCondition( BG_InventoryContainsWeapon( con->param1.value.i, self->client->ps.stats ), con->op, success );
			break;
		case CON_DAMAGEDBUILDING:
			BotBoolCondition( self->botMind->closestDamagedBuilding.ent, con->op, success );
			break;
		case CON_CVAR:
			BotBoolCondition( ( ( ( vmCvar_t * ) con->param1.value.ptr )->integer ), con->op, success );
			break;
		case CON_CURWEAPON:
			BotCompare( self->client->ps.weapon, con->param1.value.i, con->op, success );
			break;
		case CON_TEAM:
			BotCompare( self->client->ps.stats[ STAT_TEAM ], con->param1.value.i, con->op, success );
			break;
		case CON_ENEMY:
			if ( level.time - self->botMind->timeFoundEnemy < g_bot_reactiontime.integer )
			{
				success = qfalse;
			}
			else if ( !self->botMind->bestEnemy.ent )
			{
				success = qfalse;
			}
			else
			{
				success = qtrue;
			}

			if ( con->op == OP_NOT )
			{
				success = ( qboolean ) ( ( int ) !success );
			}
			break;
		case CON_UPGRADE:
		{
			BotBoolCondition( BG_InventoryContainsUpgrade( con->param1.value.i, self->client->ps.stats ), con->op, success );

			if ( con->op == OP_NOT )
			{
				if ( BG_UpgradeIsActive( con->param1.value.i, self->client->ps.stats ) )
				{
					success = qfalse;
				}
			}
			else if ( con->op == OP_BOOL )
			{
				if ( BG_UpgradeIsActive( con->param1.value.i, self->client->ps.stats ) )
				{
					success = qtrue;
				}
			}
		}
		break;
		case CON_HEALTH:
		{
			float health = self->health;
			float maxHealth = BG_Class( ( class_t ) self->client->ps.stats[ STAT_CLASS ] )->health;

			BotCompare( ( health / maxHealth ), con->param1.value.f, con->op, success );
		}
		break;
		case CON_AMMO:
			BotCompare( PercentAmmoRemaining( BG_PrimaryWeapon( self->client->ps.stats ), &self->client->ps ), con->param1.value.f, con->op, success );
			break;
		case CON_TEAM_WEAPON:
			BotBoolCondition( BotTeamateHasWeapon( self, con->param1.value.i ), con->op, success );
			break;
		case CON_DISTANCE:
		{
			float distance = 0;

			if ( con->param1.value.i < BA_NUM_BUILDABLES )
			{
				distance = self->botMind->closestBuildings[ con->param1.value.i ].distance;
			}
			else if ( con->param1.value.i == E_GOAL )
			{
				distance = DistanceToGoal( self );
			}
			else if ( con->param1.value.i == E_ENEMY )
			{
				distance = self->botMind->bestEnemy.distance;
			}
			else if ( con->param1.value.i == E_DAMAGEDBUILDING )
			{
				distance = self->botMind->closestDamagedBuilding.distance;
			}

			BotCompare( distance, con->param2.value.i, con->op, success );
		}
		break;
		case CON_BASERUSH:
		{
			float score = BotGetBaseRushScore( self );

			BotCompare( score, con->param1.value.f, con->op, success );
		}
		break;
		case CON_HEAL:
		{
			float score = BotGetHealScore( self );
			BotCompare( score, con->param1.value.f, con->op, success );
		}
		break;
	}

	if ( success )
	{
		if ( con->child )
		{
			return BotEvaluateNode( self, con->child );
		}
		else
		{
			return STATUS_SUCCESS;
		}
	}

	return STATUS_FAILURE;
}

AINodeStatus_t BotEvaluateNode( gentity_t *self, AIGenericNode_t *node )
{
	AINodeStatus_t status = node->run( self, node );

	// maintain current node data for action nodes
	// they use this to determine if they need to pathfind again
	if ( node->type == ACTION_NODE )
	{
		if ( status == STATUS_RUNNING )
		{
			self->botMind->currentNode = node;
		}

		if ( self->botMind->currentNode == node && status != STATUS_RUNNING )
		{
			self->botMind->currentNode = NULL;
		}
	}

	// reset running information on node success so sequences and selectors reset their state
	if ( NodeIsRunning( self, node ) && status == STATUS_SUCCESS )
	{
		memset( self->botMind->runningNodes, 0, sizeof( self->botMind->runningNodes ) );
		self->botMind->numRunningNodes = 0;
	}

	// store running information for sequence nodes and selector nodes
	if ( status == STATUS_RUNNING )
	{
		if ( self->botMind->numRunningNodes == MAX_NODE_DEPTH )
		{
			G_Printf( "ERROR: MAX_NODE_DEPTH exceeded\n" );
			return status;
		}

		// clear out previous running list when we hit a running leaf node
		// this insures that only 1 node in a sequence or selector has the running state
		if ( node->type == ACTION_NODE )
		{
			memset( self->botMind->runningNodes, 0, sizeof( self->botMind->runningNodes ) );
			self->botMind->numRunningNodes = 0;
		}

		self->botMind->runningNodes[ self->botMind->numRunningNodes++ ] = node;
	}

	return status;
}

/*
	Behavior tree action nodes
*/
AINodeStatus_t BotActionFight( gentity_t *self, AIGenericNode_t *node )
{
	team_t myTeam = ( team_t ) self->client->ps.stats[ STAT_TEAM ];

	if ( self->botMind->currentNode != node )
	{
		if ( !BotChangeGoalEntity( self, self->botMind->bestEnemy.ent ) )
		{
			return STATUS_FAILURE;
		}
		else
		{
			return STATUS_RUNNING;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	if ( BotGetTargetTeam( self->botMind->goal ) == myTeam || BotGetTargetTeam( self->botMind->goal ) == TEAM_NONE )
	{
		self->botMind->bestEnemy.ent = NULL;
		return STATUS_FAILURE;
	}

	// the enemy has died
	if ( self->botMind->goal.ent->health <= 0 )
	{
		self->botMind->bestEnemy.ent = NULL;
		return STATUS_SUCCESS;
	}

	if ( self->botMind->goal.ent->client && self->botMind->goal.ent->client->sess.spectatorState != SPECTATOR_NOT )
	{
		self->botMind->bestEnemy.ent = NULL;
		return STATUS_FAILURE;
	}

	if ( WeaponIsEmpty( ( weapon_t )self->client->ps.weapon, self->client->ps ) && myTeam == TEAM_HUMANS )
	{
		G_ForceWeaponChange( self, WP_BLASTER );
	}

	if ( self->client->ps.weapon == WP_HBUILD )
	{
		G_ForceWeaponChange( self, WP_BLASTER );
	}

	//aliens have radar so they will always 'see' the enemy if they are in radar range
	if ( myTeam == TEAM_ALIENS && DistanceToGoalSquared( self ) <= Square( ALIENSENSE_RANGE ) )
	{
		self->botMind->enemyLastSeen = level.time;
	}

	if ( !BotTargetIsVisible( self, self->botMind->goal, CONTENTS_SOLID ) )
	{
		botTarget_t proposedTarget;
		BotSetTarget( &proposedTarget, self->botMind->bestEnemy.ent, NULL );

		//we can see another enemy (not our target)
		if ( self->botMind->bestEnemy.ent && self->botMind->goal.ent != self->botMind->bestEnemy.ent && BotPathIsWalkable( self, proposedTarget ) )
		{
			//change targets
			BotChangeGoal( self, proposedTarget );
			return STATUS_RUNNING;
		}
		else if ( level.time - self->botMind->enemyLastSeen > g_bot_chasetime.integer )
		{
			self->botMind->bestEnemy.ent = NULL;
			return STATUS_FAILURE;
		}
		else
		{
			BotMoveToGoal( self );
			return STATUS_RUNNING;
		}
	}
	else
	{
		qboolean inAttackRange = BotTargetInAttackRange( self, self->botMind->goal );
		self->botMind->enemyLastSeen = level.time;

		if ( ( inAttackRange && myTeam == TEAM_HUMANS ) || self->botMind->directPathToGoal )
		{
			botRouteTarget_t routeTarget;
			BotAimAtEnemy( self );

			BotTargetToRouteTarget( self, self->botMind->goal, &routeTarget );

			//update the path corridor
			trap_BotUpdatePath( self->s.number, &routeTarget, NULL, &self->botMind->directPathToGoal );

			BotMoveInDir( self, MOVE_FORWARD );

			if ( inAttackRange || self->client->ps.weapon == WP_PAIN_SAW )
			{
				BotFireWeaponAI( self );
			}

			if ( myTeam == TEAM_HUMANS )
			{
				if ( self->botMind->botSkill.level >= 3 && DistanceToGoalSquared( self ) < Square( MAX_HUMAN_DANCE_DIST )
				        && ( DistanceToGoalSquared( self ) > Square( MIN_HUMAN_DANCE_DIST ) || self->botMind->botSkill.level < 5 )
				        && self->client->ps.weapon != WP_PAIN_SAW )
				{
					BotMoveInDir( self, MOVE_BACKWARD );
				}
				else if ( DistanceToGoalSquared( self ) <= Square( MIN_HUMAN_DANCE_DIST ) ) //we wont hit this if skill < 5
				{
					//we will be moving toward enemy, strafe too
					//the result: we go around the enemy
					BotAlternateStrafe( self );

					if ( self->client->ps.weapon != WP_PAIN_SAW )
					{
						BotDodge( self );
					}
				}
				else if ( DistanceToGoalSquared( self ) >= Square( MAX_HUMAN_DANCE_DIST ) && self->client->ps.weapon != WP_PAIN_SAW )
				{
					if ( DistanceToGoalSquared( self ) - Square( MAX_HUMAN_DANCE_DIST ) < 100 )
					{
						BotStandStill( self );
					}

					BotStrafeDodge( self );
				}

				if ( inAttackRange && BotGetTargetType( self->botMind->goal ) == ET_BUILDABLE )
				{
					BotStandStill( self );
				}

				BotSprint( self, qtrue );
			}
			else if ( myTeam == TEAM_ALIENS )
			{
				BotClassMovement( self, inAttackRange );
			}
		}
		else
		{
			BotMoveToGoal( self );
		}
	}
	return STATUS_RUNNING;
}

AINodeStatus_t BotActionFlee( gentity_t *self, AIGenericNode_t *node )
{
	if ( node != self->botMind->currentNode )
	{
		if ( !BotChangeGoal( self, BotGetRetreatTarget( self ) ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	if ( DistanceToGoalSquared( self ) < Square( 70 ) )
	{
		return STATUS_SUCCESS;
	}
	else
	{
		BotMoveToGoal( self );
	}

	return STATUS_RUNNING;
}

AINodeStatus_t BotActionRoam( gentity_t *self, AIGenericNode_t *node )
{
	// we are just starting to roam, get a target location
	if ( node != self->botMind->currentNode )
	{
		botTarget_t target = BotGetRoamTarget( self );
		if ( !BotChangeGoal( self, target ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( DistanceToGoalSquared( self ) < Square( 70 ) )
	{
		return STATUS_SUCCESS;
	}
	else
	{
		BotMoveToGoal( self );
	}
	return STATUS_RUNNING;
}

botTarget_t BotGetMoveToTarget( gentity_t *self, AIMoveToNode_t *node )
{
	botTarget_t target;
	gentity_t *ent = NULL;

	if ( node->ent < BA_NUM_BUILDABLES )
	{
		ent = self->botMind->closestBuildings[ node->ent ].ent;
	}
	else if ( node->ent == E_ENEMY )
	{
		ent = self->botMind->bestEnemy.ent;
	}
	else if ( node->ent == E_DAMAGEDBUILDING )
	{
		ent = self->botMind->closestDamagedBuilding.ent;
	}

	BotSetTarget( &target, ent, NULL );
	return target;
}

AINodeStatus_t BotActionMoveTo( gentity_t *self, AIGenericNode_t *node )
{
	float radius;
	AIMoveToNode_t *moveTo = ( AIMoveToNode_t * ) node;
	if ( node != self->botMind->currentNode )
	{
		if ( !BotChangeGoal( self, BotGetMoveToTarget( self, moveTo ) ) )
		{
			return STATUS_FAILURE;
		}
		else
		{
			return STATUS_RUNNING;
		}
	}

	if ( self->botMind->goal.ent )
	{
		// died
		if ( self->botMind->goal.ent->health < 0 )
		{
			return STATUS_FAILURE;
		}
	}

	BotMoveToGoal( self );

	if ( moveTo->range == -1 )
	{
		radius = BotGetGoalRadius( self );
	}
	else
	{
		radius = moveTo->range;
	}

	if ( DistanceToGoal2DSquared( self ) <= Square( radius ) && self->botMind->directPathToGoal )
	{
		return STATUS_SUCCESS;
	}

	return STATUS_RUNNING;
}

AINodeStatus_t BotActionRush( gentity_t *self, AIGenericNode_t *node )
{
	if ( self->botMind->currentNode != node )
	{
		if ( !BotChangeGoal( self, BotGetRushTarget( self ) ) )
		{
			return STATUS_FAILURE;
		}
		else
		{
			return STATUS_RUNNING;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_FAILURE;
	}

	if ( DistanceToGoalSquared( self ) > Square( 100 ) )
	{
		BotMoveToGoal( self );
	}
	return STATUS_RUNNING;
}

AINodeStatus_t BotActionHeal( gentity_t *self, AIGenericNode_t *node )
{
	if ( self->client->ps.stats[ STAT_TEAM ] == TEAM_HUMANS )
	{
		return BotActionHealH( self, node );
	}
	else
	{
		return BotActionHealA( self, node );
	}
}

/*
	alien specific actions
*/
AINodeStatus_t BotActionEvolve ( gentity_t *self, AIGenericNode_t *node )
{
	AINodeStatus_t status = STATUS_FAILURE;
	if ( !g_bot_evolve.integer )
	{
		return status;
	}

	if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL4 ) && g_bot_level4.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL4 ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL3_UPG ) && g_bot_level3upg.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL3_UPG ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL3 ) && ( !BG_ClassAllowedInStage( PCL_ALIEN_LEVEL3_UPG, ( stage_t ) g_alienStage.integer ) || !g_bot_level2upg.integer || !g_bot_level3upg.integer ) && g_bot_level3.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL3 ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL2_UPG ) && g_bot_level2upg.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL2_UPG ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL2 ) && ( g_humanStage.integer == 0 || !g_bot_level2upg.integer )  && g_bot_level2.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL2 ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL1_UPG ) && g_humanStage.integer == 0 && g_bot_level1upg.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL1_UPG ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL1 ) && g_humanStage.integer == 0 && g_bot_level1.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL1 ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL0 ) )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL0 ) )
		{
			status = STATUS_SUCCESS;
		}
	}

	return status;
}

AINodeStatus_t BotActionHealA( gentity_t *self, AIGenericNode_t *node )
{
	const int maxHealth = BG_Class( ( class_t )self->client->ps.stats[STAT_CLASS] )->health;
	gentity_t *healTarget = NULL;
	float distToHealer = 0;

	if ( self->botMind->closestBuildings[BA_A_BOOSTER].ent )
	{
		healTarget = self->botMind->closestBuildings[BA_A_BOOSTER].ent;
	}
	else if ( self->botMind->closestBuildings[BA_A_OVERMIND].ent )
	{
		healTarget = self->botMind->closestBuildings[BA_A_OVERMIND].ent;
	}
	else if ( self->botMind->closestBuildings[BA_A_SPAWN].ent )
	{
		healTarget = self->botMind->closestBuildings[BA_A_SPAWN].ent;
	}

	if ( !healTarget )
	{
		return STATUS_FAILURE;
	}

	if ( self->client->ps.stats[STAT_TEAM] != TEAM_ALIENS )
	{
		return STATUS_FAILURE;
	}

	//we are fully healed
	if ( maxHealth == self->client->ps.stats[STAT_HEALTH] )
	{
		return STATUS_SUCCESS;
	}

	if ( self->botMind->currentNode != node )
	{
		if ( !BotChangeGoalEntity( self, healTarget ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	//target has died, signal goal is unusable
	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_FAILURE;
	}

	if ( DistanceToGoalSquared( self ) > Square( 70 ) )
	{
		BotMoveToGoal( self );
	}
	return STATUS_RUNNING;
}

/*
	human specific actions
*/
AINodeStatus_t BotActionHealH( gentity_t *self, AIGenericNode_t *node )
{
	vec3_t targetPos;
	vec3_t myPos;

	if ( self->client->ps.stats[STAT_TEAM] != TEAM_HUMANS )
	{
		return STATUS_FAILURE;
	}

	//we are fully healed
	if ( BG_Class( ( class_t )self->client->ps.stats[STAT_CLASS] )->health <= self->health && BG_InventoryContainsUpgrade( UP_MEDKIT, self->client->ps.stats ) )
	{
		return STATUS_SUCCESS;
	}

	if ( self->botMind->currentNode != node )
	{
		if ( !BotChangeGoalEntity( self, self->botMind->closestBuildings[ BA_H_MEDISTAT ].ent ) )
		{
			return STATUS_FAILURE;
		}
	}

	//safety check
	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	//the medi has died so signal that the goal is unusable
	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_FAILURE;
	}

	//this medi is no longer powered so signal that the goal is unusable
	if ( !self->botMind->goal.ent->powered )
	{
		return STATUS_FAILURE;
	}

	BotGetTargetPos( self->botMind->goal, targetPos );
	VectorCopy( self->s.origin, myPos );
	targetPos[2] += BG_BuildableModelConfig( BA_H_MEDISTAT )->maxs[2];
	myPos[2] += self->r.mins[2]; //mins is negative

	//keep moving to the medi until we are on top of it
	if ( DistanceSquared( myPos, targetPos ) > Square( BG_BuildableModelConfig( BA_H_MEDISTAT )->mins[1] ) )
	{
		BotMoveToGoal( self );
	}
	return STATUS_RUNNING;
}
AINodeStatus_t BotActionRepair( gentity_t *self, AIGenericNode_t *node )
{
	vec3_t forward;
	vec3_t targetPos;
	vec3_t selfPos;

	if ( node != self->botMind->currentNode )
	{
		if ( !BotChangeGoalEntity( self, self->botMind->closestDamagedBuilding.ent ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_FAILURE;
	}

	if ( self->botMind->goal.ent->health >= BG_Buildable( ( buildable_t )self->botMind->goal.ent->s.modelindex )->health )
	{
		return STATUS_SUCCESS;
	}

	if ( self->client->ps.weapon != WP_HBUILD )
	{
		G_ForceWeaponChange( self, WP_HBUILD );
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	BotGetTargetPos( self->botMind->goal, targetPos );
	VectorMA( self->s.origin, self->r.maxs[1], forward, selfPos );

	//move to the damaged building until we are in range
	if ( !BotTargetIsVisible( self, self->botMind->goal, MASK_SHOT ) || DistanceToGoalSquared( self ) > Square( 100 ) )
	{
		BotMoveToGoal( self );
	}
	else
	{
		//aim at the buildable
		BotSlowAim( self, targetPos, 0.5 );
		BotAimAtLocation( self, targetPos );
		// we automatically heal a building if close enough and aiming at it
	}
	return STATUS_RUNNING;
}
AINodeStatus_t BotActionBuy( gentity_t *self, AIGenericNode_t *node )
{
	AIBuyNode_t *buy = ( AIBuyNode_t * ) node;

	weapon_t weapon = buy->weapon;
	upgrade_t *upgrades = buy->upgrades;
	int numUpgrades = buy->numUpgrades;
	int i;

	if ( weapon == WP_NONE && numUpgrades == 0 )
	{
		BotGetDesiredBuy( self, &weapon, upgrades, &numUpgrades );
	}

	if ( !g_bot_buy.integer )
	{
		return STATUS_FAILURE;
	}
	if ( BotGetEntityTeam( self ) != TEAM_HUMANS )
	{
		return STATUS_FAILURE;
	}
	if ( numUpgrades && upgrades[0] == UP_AMMO && BG_Weapon( (weapon_t)self->client->ps.stats[ STAT_WEAPON ] )->usesEnergy )
	{
		if ( !self->botMind->closestBuildings[ BA_H_ARMOURY ].ent &&
		     !self->botMind->closestBuildings[ BA_H_REPEATER ].ent &&
		     !self->botMind->closestBuildings[ BA_H_REACTOR ].ent )
		{
			return STATUS_FAILURE; //wanted ammo for energy? no armoury, repeater or reactor, so fail
		}
	}
	else if ( !self->botMind->closestBuildings[BA_H_ARMOURY].ent )
	{
		return STATUS_FAILURE;    //no armoury, so fail
	}

	//check if we already have everything
	if ( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) || weapon == WP_NONE )
	{
		int numContain = 0;

		//cant buy more than 3 upgrades
		if ( numUpgrades > 3 )
		{
			return STATUS_FAILURE;
		}

		if ( numUpgrades == 0 )
		{
			return STATUS_FAILURE;
		}

		for ( i = 0; i < numUpgrades; i++ )
		{
			if ( BG_InventoryContainsUpgrade( upgrades[i], self->client->ps.stats ) )
			{
				numContain++;
			}
		}
		//we have every upgrade we want to buy
		if ( numContain == numUpgrades )
		{
			return STATUS_FAILURE;
		}
	}

	if ( self->botMind->currentNode != node )
	{
		//default to armoury
		const botEntityAndDistance_t *ent = &self->botMind->closestBuildings[ BA_H_ARMOURY ];

		//wanting energy ammo only? look for closer repeater or reactor
		if ( numUpgrades && upgrades[0] == UP_AMMO && BG_Weapon( (weapon_t)self->client->ps.stats[ STAT_WEAPON ] )->usesEnergy)
		{
#define DISTANCE(obj) ( self->botMind->closestBuildings[ obj ].ent ? self->botMind->closestBuildings[ obj ].distance : INT_MAX )
			//repeater closest? use that
			if ( DISTANCE( BA_H_REPEATER ) < DISTANCE( BA_H_REACTOR ) )
			{
				if ( DISTANCE( BA_H_REPEATER ) < DISTANCE( BA_H_ARMOURY ) )
				{
					ent = &self->botMind->closestBuildings[ BA_H_REPEATER ];
				}
			}
			//reactor closest? use that
			else if ( DISTANCE( BA_H_REACTOR ) < DISTANCE( BA_H_ARMOURY ) )
			{
				ent = &self->botMind->closestBuildings[ BA_H_REACTOR ];
			}
		}
		if ( !BotChangeGoalEntity( self, ent->ent ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}
	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_FAILURE;
	}

	if ( DistanceToGoalSquared( self ) > 100 * 100 )
	{
		BotMoveToGoal( self );
		return STATUS_RUNNING;
	}
	else
	{

		if ( numUpgrades && upgrades[0] != UP_AMMO )
		{
			BotSellAll( self );
		}
		else if ( weapon != WP_NONE )
		{
			BotSellWeapons( self );
		}

		BotBuyWeapon( self, weapon );

		for ( i = 0; i < numUpgrades; i++ )
		{
			BotBuyUpgrade( self, upgrades[i] );
		}

		// make sure that we're not using the blaster
		G_ForceWeaponChange( self, weapon );
		
		return STATUS_SUCCESS;
	}
}