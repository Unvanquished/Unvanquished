/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

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

#ifndef RAND_MAX
#define RAND_MAX 32768
#endif

botMemory_t g_botMind[MAX_CLIENTS];

void BotDPrintf( const char* fmt, ... )
{
	if ( g_bot_debug.integer )
	{
		va_list argptr;
		char    text[ 1024 ];

		va_start( argptr, fmt );
		Q_vsnprintf( text, sizeof( text ), fmt, argptr );
		va_end( argptr );

		trap_Print( text );
	}
}

/*
=======================
Behavior Tree
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

pc_token_list *CreateTokenList( int handle )
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

void FreeTokenList( pc_token_list *list )
{
	pc_token_list *current = list;
	while( current )
	{
		pc_token_list *node = current;
		current = current->next;

		BG_Free( node );
	}
}

AIGenericNode_t *ReadNode( pc_token_list **tokenlist );

void CheckToken( const char *tokenValueName, const char *nodename, const pc_token_t *token, int requiredType )
{
	if ( token->type != requiredType )
	{
		BotDPrintf( S_COLOR_RED "ERROR: Invalid %s %s after %s on line %d\n", tokenValueName, token->string, nodename, token->line );
	}
}

AIConditionOperator_t ReadConditionOperator( pc_token_list *tokenlist )
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

AIGenericNode_t *ReadConditionNode( pc_token_list **tokenlist )
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

AIGenericNode_t *ReadActionNode( pc_token_list **tokenlist )
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

AIGenericNode_t *ReadNodeList( pc_token_list **tokenlist )
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

AIGenericNode_t *ReadNode( pc_token_list **tokenlist )
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

AITreeList_t treeList;

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
			if ( i + 1 < list->numTrees )
			{
				memmove( &list->trees[ i ], &list->trees[ i + 1 ], sizeof( AIBehaviorTree_t * ) );
			}

			list->numTrees--;
		}
	}
}

void FreeTreeList( AITreeList_t *list )
{
	BG_Free( list->trees );
	list->trees = NULL;
	list->maxTrees = 0;
	list->numTrees = 0;
}

AIBehaviorTree_t * ReadBehaviorTree( const char *name )
{
	int i;
	char treefilename[ MAX_QPATH ];
	int handle;
	pc_token_list *list;
	AIBehaviorTree_t *tree;
	pc_token_list *current;
	AIGenericNode_t *node;

	for ( i = 0; i < treeList.numTrees; i++ )
	{
		AIBehaviorTree_t *tree = treeList.trees[ i ];
		if ( !Q_stricmp( tree->name, name ) )
		{
			return tree;
		}
	}

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
	D( E_A_LEECH );
	D( E_H_SPAWN );
	D( E_H_MGTURRET );
	D( E_H_TESLAGEN );
	D( E_H_ARMOURY );
	D( E_H_DCC );
	D( E_H_MEDISTAT );
	D( E_H_REACTOR );
	D( E_H_REPEATER );
	D( E_H_DRILL );
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

	list = CreateTokenList( handle );
	
	tree = ( AIBehaviorTree_t * ) BG_Alloc( sizeof( AIBehaviorTree_t ) );

	Q_strncpyz( tree->name, name, sizeof( tree->name ) );

	current = list;

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
		AddTreeToList( &treeList, tree );
	}

	FreeTokenList( list );
	trap_Parse_FreeSource( handle );
	return tree;
}

void FreeNode( AIGenericNode_t *node );

void FreeConditionNode( AIConditionNode_t *node )
{
	FreeNode( node->child );
	BG_Free( node );
}

void FreeNodeList( AINodeList_t *node )
{
	int i;
	for ( i = 0; i < node->numNodes; i++ )
	{
		FreeNode( node->list[ i ] );
	}
	BG_Free( node );
}

void FreeNode( AIGenericNode_t *node )
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

/*
=======================
Scoring functions for logic
=======================
*/
float BotGetBaseRushScore( gentity_t *ent )
{

	switch ( ent->s.weapon )
	{
		case WP_GRENADE:
			return 0.7f;
		case WP_BLASTER:
			return 0.1f;
		case WP_LUCIFER_CANNON:
			return 1.0f;
		case WP_MACHINEGUN:
			return 0.5f;
		case WP_PULSE_RIFLE:
			return 0.7f;
		case WP_LAS_GUN:
			return 0.7f;
		case WP_SHOTGUN:
			return 0.2f;
		case WP_CHAINGUN:
			if ( BG_InventoryContainsUpgrade( UP_BATTLESUIT, ent->client->ps.stats ) )
			{
				return 0.5f;
			}
			else
			{
				return 0.2f;
			}
		case WP_HBUILD:
			return 0.0f;
		case WP_ABUILD:
			return 0.0f;
		case WP_ABUILD2:
			return 0.0f;
		case WP_ALEVEL0:
			return 0.0f;
		case WP_ALEVEL1:
			return 0.2f;
		case WP_ALEVEL2:
			return 0.5f;
		case WP_ALEVEL2_UPG:
			return 0.7f;
		case WP_ALEVEL3:
			return 0.8f;
		case WP_ALEVEL3_UPG:
			return 0.9f;
		case WP_ALEVEL4:
			return 1.0f;
		default:
			return 0.5f;
	}
}

float BotGetHealScore( gentity_t *self )
{
	float distToHealer = 0;
	float percentHealth = 0;
	float maxHealth = BG_Class( ( class_t ) self->client->ps.stats[ STAT_CLASS ] )->health;

	if ( self->client->ps.stats[ STAT_TEAM ] == TEAM_ALIENS )
	{
		if ( self->botMind->closestBuildings[ BA_A_BOOSTER ].ent )
		{
			distToHealer = self->botMind->closestBuildings[ BA_A_BOOSTER ].distance;
		}
		else if ( self->botMind->closestBuildings[ BA_A_OVERMIND ].ent )
		{
			distToHealer = self->botMind->closestBuildings[ BA_A_OVERMIND ].distance;
		}
		else if ( self->botMind->closestBuildings[ BA_A_SPAWN ].ent )
		{
			distToHealer = self->botMind->closestBuildings[BA_A_SPAWN].distance;
		}
	}
	else
	{
		distToHealer = self->botMind->closestBuildings[ BA_H_MEDISTAT ].distance;
	}

	percentHealth = ( ( float ) self->client->ps.stats[STAT_HEALTH] ) / maxHealth;

	distToHealer = MAX( MIN( MAX_HEAL_DIST, distToHealer ), MAX_HEAL_DIST * ( 3.0f / 4.0f ) );

	if ( percentHealth == 1.0f )
	{
		return 1.0f;
	}
	return percentHealth * distToHealer / MAX_HEAL_DIST;
}

float BotGetEnemyPriority( gentity_t *self, gentity_t *ent )
{
	float enemyScore;
	float distanceScore;
	distanceScore = Distance( self->s.origin, ent->s.origin );

	if ( ent->client )
	{
		switch ( ent->s.weapon )
		{
			case WP_ALEVEL0:
				enemyScore = 0.1;
				break;
			case WP_ALEVEL1:
			case WP_ALEVEL1_UPG:
				enemyScore = 0.3;
				break;
			case WP_ALEVEL2:
				enemyScore = 0.4;
				break;
			case WP_ALEVEL2_UPG:
				enemyScore = 0.7;
				break;
			case WP_ALEVEL3:
				enemyScore = 0.7;
				break;
			case WP_ALEVEL3_UPG:
				enemyScore = 0.8;
				break;
			case WP_ALEVEL4:
				enemyScore = 1.0;
				break;
			case WP_BLASTER:
				enemyScore = 0.2;
				break;
			case WP_MACHINEGUN:
				enemyScore = 0.4;
				break;
			case WP_PAIN_SAW:
				enemyScore = 0.4;
				break;
			case WP_LAS_GUN:
				enemyScore = 0.4;
				break;
			case WP_MASS_DRIVER:
				enemyScore = 0.4;
				break;
			case WP_CHAINGUN:
				enemyScore = 0.6;
				break;
			case WP_FLAMER:
				enemyScore = 0.6;
				break;
			case WP_PULSE_RIFLE:
				enemyScore = 0.5;
				break;
			case WP_LUCIFER_CANNON:
				enemyScore = 1.0;
				break;
			default:
				enemyScore = 0.5;
				break;
		}
	}
	else
	{
		switch ( ent->s.modelindex )
		{
			case BA_H_MGTURRET:
				enemyScore = 0.6;
				break;
			case BA_H_REACTOR:
				enemyScore = 0.5;
				break;
			case BA_H_TESLAGEN:
				enemyScore = 0.7;
				break;
			case BA_H_SPAWN:
				enemyScore = 0.9;
				break;
			case BA_H_ARMOURY:
				enemyScore = 0.8;
				break;
			case BA_H_MEDISTAT:
				enemyScore = 0.6;
				break;
			case BA_H_DCC:
				enemyScore = 0.5;
				break;
			case BA_H_DRILL:
				enemyScore = 0.7;
				break;
			case BA_A_ACIDTUBE:
				enemyScore = 0.7;
				break;
			case BA_A_SPAWN:
				enemyScore = 0.9;
				break;
			case BA_A_OVERMIND:
				enemyScore = 0.5;
				break;
			case BA_A_HIVE:
				enemyScore = 0.7;
				break;
			case BA_A_BOOSTER:
				enemyScore = 0.4;
				break;
			case BA_A_TRAPPER:
				enemyScore = 0.8;
				break;
			case BA_A_LEECH:
				enemyScore = 0.7;
				break;
			default:
				enemyScore = 0.5;
				break;

		}
	}
	return enemyScore * 1000 / distanceScore;
}
/*
=======================
Entity Querys
=======================
*/
gentity_t* BotFindBuilding( gentity_t *self, int buildingType, int range )
{
	float minDistance = -1;
	gentity_t* closestBuilding = NULL;
	float newDistance;
	float rangeSquared = Square( range );
	gentity_t *target = &g_entities[MAX_CLIENTS];
	int i;

	for ( i = MAX_CLIENTS; i < level.num_entities; i++, target++ )
	{
		if ( !target->inuse )
		{
			continue;
		}
		if ( target->s.eType == ET_BUILDABLE && target->s.modelindex == buildingType && ( target->buildableTeam == TEAM_ALIENS || ( target->powered && target->spawned ) ) && target->health > 0 )
		{
			newDistance = DistanceSquared( self->s.origin, target->s.origin );
			if ( range && newDistance > rangeSquared )
			{
				continue;
			}
			if ( newDistance < minDistance || minDistance == -1 )
			{
				minDistance = newDistance;
				closestBuilding = target;
			}
		}
	}
	return closestBuilding;
}

void BotFindClosestBuildings( gentity_t *self, botEntityAndDistance_t *closest )
{
	gentity_t *testEnt;
	botEntityAndDistance_t *ent;
	memset( closest, 0, sizeof( botEntityAndDistance_t ) * BA_NUM_BUILDABLES );

	for ( testEnt = &g_entities[MAX_CLIENTS]; testEnt < &g_entities[level.num_entities - 1]; testEnt++ )
	{
		float newDist;
		//ignore entities that arnt in use
		if ( !testEnt->inuse )
		{
			continue;
		}

		//ignore dead targets
		if ( testEnt->health <= 0 )
		{
			continue;
		}

		//skip non buildings
		if ( testEnt->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		//skip human buildings that are currently building or arn't powered
		if ( testEnt->buildableTeam == TEAM_HUMANS && ( !testEnt->powered || !testEnt->spawned ) )
		{
			continue;
		}

		newDist = Distance( self->s.origin, testEnt->s.origin );

		ent = &closest[ testEnt->s.modelindex ];
		if ( newDist < ent->distance || ent->distance == 0 )
		{
			ent->ent = testEnt;
			ent->distance = newDist;
		}
	}
}

qboolean BotEntityIsVisible( gentity_t *self, gentity_t *target, int mask )
{
	botTarget_t bt;
	BotSetTarget( &bt, target, NULL );
	return BotTargetIsVisible( self, bt, mask );
}

gentity_t* BotFindBestEnemy( gentity_t *self )
{
	float bestVisibleEnemyScore = 0;
	float bestInvisibleEnemyScore = 0;
	gentity_t *bestVisibleEnemy = NULL;
	gentity_t *bestInvisibleEnemy = NULL;
	gentity_t *target;

	for ( target = g_entities; target < &g_entities[level.num_entities - 1]; target++ )
	{
		float newScore;
		//ignore entities that arnt in use
		if ( !target->inuse )
		{
			continue;
		}

		//ignore dead targets
		if ( target->health <= 0 )
		{
			continue;
		}

		//ignore buildings if we cant attack them
		if ( target->s.eType == ET_BUILDABLE && ( !g_bot_attackStruct.integer || self->client->ps.stats[STAT_CLASS] == PCL_ALIEN_LEVEL0 ) )
		{
			continue;
		}

		//ignore neutrals
		if ( BotGetEntityTeam( target ) == TEAM_NONE )
		{
			continue;
		}

		//ignore teamates
		if ( BotGetEntityTeam( target ) == BotGetEntityTeam( self ) )
		{
			continue;
		}

		//ignore spectators
		if ( target->client )
		{
			if ( target->client->sess.spectatorState != SPECTATOR_NOT )
			{
				continue;
			}
		}

		if ( DistanceSquared( self->s.origin, target->s.origin ) > Square( ALIENSENSE_RANGE ) )
		{
			continue;
		}

		newScore = BotGetEnemyPriority( self, target );

		if ( newScore > bestVisibleEnemyScore && BotEntityIsVisible( self, target, MASK_SHOT ) )
		{
			//store the new score and the index of the entity
			bestVisibleEnemyScore = newScore;
			bestVisibleEnemy = target;
		}
		else if ( newScore > bestInvisibleEnemyScore && BotGetEntityTeam( self ) == TEAM_ALIENS )
		{
			bestInvisibleEnemyScore = newScore;
			bestInvisibleEnemy = target;
		}
	}
	if ( bestVisibleEnemy || BotGetEntityTeam( self ) == TEAM_HUMANS )
	{
		return bestVisibleEnemy;
	}
	else
	{
		return bestInvisibleEnemy;
	}
}

gentity_t* BotFindClosestEnemy( gentity_t *self )
{
	gentity_t* closestEnemy = NULL;
	float minDistance = Square( ALIENSENSE_RANGE );
	gentity_t *target;

	for ( target = g_entities; target < &g_entities[level.num_entities - 1]; target++ )
	{
		float newDistance;
		//ignore entities that arnt in use
		if ( !target->inuse )
		{
			continue;
		}

		//ignore dead targets
		if ( target->health <= 0 )
		{
			continue;
		}

		//ignore buildings if we cant attack them
		if ( target->s.eType == ET_BUILDABLE && ( !g_bot_attackStruct.integer || self->client->ps.stats[STAT_CLASS] == PCL_ALIEN_LEVEL0 ) )
		{
			continue;
		}

		//ignore neutrals
		if ( BotGetEntityTeam( target ) == TEAM_NONE )
		{
			continue;
		}

		//ignore teamates
		if ( BotGetEntityTeam( target ) == BotGetEntityTeam( self ) )
		{
			continue;
		}

		//ignore spectators
		if ( target->client )
		{
			if ( target->client->sess.spectatorState != SPECTATOR_NOT )
			{
				continue;
			}
		}
		newDistance = DistanceSquared( self->s.origin, target->s.origin );
		if ( newDistance <= minDistance )
		{
			minDistance = newDistance;
			closestEnemy = target;
		}
	}
	return closestEnemy;
}

botTarget_t BotGetRushTarget( gentity_t *self )
{
	botTarget_t target;
	gentity_t* rushTarget = NULL;
	if ( BotGetEntityTeam( self ) == TEAM_HUMANS )
	{
		if ( self->botMind->closestBuildings[BA_A_SPAWN].ent )
		{
			rushTarget = self->botMind->closestBuildings[BA_A_SPAWN].ent;
		}
		else if ( self->botMind->closestBuildings[BA_A_OVERMIND].ent )
		{
			rushTarget = self->botMind->closestBuildings[BA_A_OVERMIND].ent;
		}
	}
	else    //team aliens
	{
		if ( self->botMind->closestBuildings[BA_H_SPAWN].ent )
		{
			rushTarget = self->botMind->closestBuildings[BA_H_SPAWN].ent;
		}
		else if ( self->botMind->closestBuildings[BA_H_REACTOR].ent )
		{
			rushTarget = self->botMind->closestBuildings[BA_H_REACTOR].ent;
		}
	}
	BotSetTarget( &target, rushTarget, NULL );
	return target;
}

botTarget_t BotGetRetreatTarget( gentity_t *self )
{
	botTarget_t target;
	gentity_t* retreatTarget = NULL;
	//FIXME, this seems like it could be done better...
	if ( self->client->ps.stats[STAT_TEAM] == TEAM_HUMANS )
	{
		if ( self->botMind->closestBuildings[BA_H_REACTOR].ent )
		{
			retreatTarget = self->botMind->closestBuildings[BA_H_REACTOR].ent;
		}
	}
	else
	{
		if ( self->botMind->closestBuildings[BA_A_OVERMIND].ent )
		{
			retreatTarget = self->botMind->closestBuildings[BA_A_OVERMIND].ent;
		}
	}
	BotSetTarget( &target, retreatTarget, NULL );
	return target;
}

/*
========================
BotTarget Helpers
========================
*/

void BotSetTarget( botTarget_t *target, gentity_t *ent, vec3_t *pos )
{
	if ( ent )
	{
		target->ent = ent;
		VectorClear( target->coord );
		target->inuse = qtrue;
	}
	else if ( pos )
	{
		target->ent = NULL;
		VectorCopy( *pos, target->coord );
		target->inuse = qtrue;
	}
	else
	{
		target->ent = NULL;
		VectorClear( target->coord );
		target->inuse = qfalse;
	}
}

qboolean BotTargetIsEntity( botTarget_t target )
{
	return ( target.ent && target.ent->inuse );
}

qboolean BotTargetIsPlayer( botTarget_t target )
{
	return ( target.ent && target.ent->inuse && target.ent->client );
}

int BotGetTargetEntityNumber( botTarget_t target )
{
	if ( BotTargetIsEntity( target ) )
	{
		return target.ent->s.number;
	}
	else
	{
		return ENTITYNUM_NONE;
	}
}

void BotGetTargetPos( botTarget_t target, vec3_t rVec )
{
	if ( BotTargetIsEntity( target ) )
	{
		VectorCopy( target.ent->s.origin, rVec );
	}
	else
	{
		VectorCopy( target.coord, rVec );
	}
}

team_t BotGetEntityTeam( gentity_t *ent )
{
	if ( !ent )
	{
		return TEAM_NONE;
	}
	if ( ent->client )
	{
		return ( team_t )ent->client->ps.stats[STAT_TEAM];
	}
	else if ( ent->s.eType == ET_BUILDABLE )
	{
		return ent->buildableTeam;
	}
	else
	{
		return TEAM_NONE;
	}
}

team_t BotGetTargetTeam( botTarget_t target )
{
	if ( BotTargetIsEntity( target ) )
	{
		return BotGetEntityTeam( target.ent );
	}
	else
	{
		return TEAM_NONE;
	}
}

int BotGetTargetType( botTarget_t target )
{
	if ( BotTargetIsEntity( target ) )
	{
		return target.ent->s.eType;
	}
	else
	{
		return -1;
	}
}

qboolean BotChangeGoal( gentity_t *self, botTarget_t target )
{
	if ( !target.inuse )
	{
		return qfalse;
	}

	if ( FindRouteToTarget( self, target ) & ( ROUTE_PARTIAL | ROUTE_FAILED ) )
	{
		return qfalse;
	}

	self->botMind->goal = target;
	self->botMind->directPathToGoal = qfalse;
	return qtrue;
}

qboolean BotChangeGoalEntity( gentity_t *self, gentity_t *goal )
{
	botTarget_t target;
	BotSetTarget( &target, goal, NULL );
	return BotChangeGoal( self, target );
}

qboolean BotChangeGoalPos( gentity_t *self, vec3_t goal )
{
	botTarget_t target;
	BotSetTarget( &target, NULL, ( vec3_t * ) &goal );
	return BotChangeGoal( self, target );
}

qboolean BotTargetInAttackRange( gentity_t *self, botTarget_t target )
{
	float range, secondaryRange;
	vec3_t forward, right, up;
	vec3_t muzzle;
	vec3_t maxs, mins;
	vec3_t targetPos;
	trace_t trace;
	float width = 0, height = 0;

	AngleVectors( self->client->ps.viewangles, forward, right, up );
	CalcMuzzlePoint( self, forward, right, up , muzzle );
	BotGetTargetPos( target, targetPos );
	switch ( self->client->ps.weapon )
	{
		case WP_ABUILD:
			range = ABUILDER_CLAW_RANGE;
			secondaryRange = 0;
			width = height = ABUILDER_CLAW_WIDTH;
			break;
		case WP_ABUILD2:
			range = ABUILDER_CLAW_RANGE;
			secondaryRange = 300; //An arbitrary value for the blob launcher, has nothing to do with actual range
			width = height = ABUILDER_CLAW_WIDTH;
			break;
		case WP_ALEVEL0:
			range = LEVEL0_BITE_RANGE;
			secondaryRange = 0;
			break;
		case WP_ALEVEL1:
			range = LEVEL1_CLAW_RANGE;
			secondaryRange = 0;
			width = height = LEVEL1_CLAW_WIDTH;
			break;
		case WP_ALEVEL1_UPG:
			range = LEVEL1_CLAW_RANGE;
			secondaryRange = LEVEL1_PCLOUD_RANGE;
			width = height = LEVEL1_CLAW_WIDTH;
			break;
		case WP_ALEVEL2:
			range = LEVEL2_CLAW_RANGE;
			secondaryRange = 0;
			width = height = LEVEL2_CLAW_WIDTH;
			break;
		case WP_ALEVEL2_UPG:
			range = LEVEL2_CLAW_U_RANGE;
			secondaryRange = LEVEL2_AREAZAP_RANGE;
			width = height = LEVEL2_CLAW_WIDTH;
			break;
		case WP_ALEVEL3:
			range = LEVEL3_CLAW_RANGE;
			//need to check if we can pounce to the target
			secondaryRange = LEVEL3_POUNCE_JUMP_MAG; //An arbitrary value for pounce, has nothing to do with actual range
			width = height = LEVEL3_CLAW_WIDTH;
			break;
		case WP_ALEVEL3_UPG:
			range = LEVEL3_CLAW_RANGE;
			//we can pounce, or we have barbs
			secondaryRange = LEVEL3_POUNCE_JUMP_MAG_UPG; //An arbitrary value for pounce and barbs, has nothing to do with actual range
			if ( self->client->ps.ammo > 0 )
			{
				secondaryRange = 900;
			}
			width = height = LEVEL3_CLAW_WIDTH;
			break;
		case WP_ALEVEL4:
			range = LEVEL4_CLAW_RANGE;
			secondaryRange = 0; //Using 0 since tyrant rush is basically just movement, not a ranged attack
			width = height = LEVEL4_CLAW_WIDTH;
			break;
		case WP_HBUILD:
			range = 100;
			secondaryRange = 0;
			break;
		case WP_PAIN_SAW:
			range = PAINSAW_RANGE;
			secondaryRange = 0;
			break;
		case WP_FLAMER:
			range = FLAMER_SPEED * FLAMER_LIFETIME / 1000.0f - 100.0f;
			secondaryRange = 0;
			width = height = FLAMER_SIZE;
			// Correct muzzle so that the missile does not start in the ceiling
			VectorMA( muzzle, -7.0f, up, muzzle );

			// Correct muzzle so that the missile fires from the player's hand
			VectorMA( muzzle, 4.5f, right, muzzle );
			break;
		case WP_SHOTGUN:
			range = ( 50 * 8192 ) / SHOTGUN_SPREAD; //50 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		case WP_MACHINEGUN:
			range = ( 100 * 8192 ) / RIFLE_SPREAD; //100 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		case WP_CHAINGUN:
			range = ( 60 * 8192 ) / CHAINGUN_SPREAD; //60 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		default:
			range = 4098 * 4; //large range for guns because guns have large ranges :)
			secondaryRange = 0; //no secondary attack
	}
	VectorSet( maxs, width, width, width );
	VectorSet( mins, -width, -width, -height );

	trap_Trace( &trace, muzzle, mins, maxs, targetPos, self->s.number, MASK_SHOT );

	if ( self->client->ps.stats[STAT_TEAM] != BotGetEntityTeam( &g_entities[trace.entityNum] ) 
		&& BotGetEntityTeam( &g_entities[ trace.entityNum ] ) != TEAM_NONE
		&& Distance( muzzle, trace.endpos ) <= MAX( range, secondaryRange ) )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

qboolean BotTargetIsVisible( gentity_t *self, botTarget_t target, int mask )
{
	trace_t trace;
	vec3_t  muzzle, targetPos;
	vec3_t  forward, right, up;

	AngleVectors( self->client->ps.viewangles, forward, right, up );
	CalcMuzzlePoint( self, forward, right, up, muzzle );
	BotGetTargetPos( target, targetPos );

	if ( !trap_InPVS( muzzle, targetPos ) )
	{
		return qfalse;
	}

	if ( mask == CONTENTS_SOLID )
	{
		trap_TraceNoEnts( &trace, muzzle, NULL, NULL, targetPos, self->s.number, mask );
	}
	else
	{
		trap_Trace( &trace, muzzle, NULL, NULL, targetPos, self->s.number, mask );
	}

	if ( trace.surfaceFlags & SURF_NOIMPACT )
	{
		return qfalse;
	}

	//target is in range
	if ( ( trace.entityNum == BotGetTargetEntityNumber( target ) || trace.fraction == 1.0f ) && !trace.startsolid )
	{
		return qtrue;
	}
	return qfalse;
}
/*
========================
Bot Aiming
========================
*/
void BotGetIdealAimLocation( gentity_t *self, botTarget_t target, vec3_t aimLocation )
{
	//get the position of the target
	BotGetTargetPos( target, aimLocation );

	if ( BotGetTargetType( target ) != ET_BUILDABLE && BotTargetIsEntity( target ) && BotGetTargetTeam( target ) == TEAM_HUMANS )
	{

		//aim at head
		aimLocation[2] += target.ent->r.maxs[2] * 0.85;

	}
	else if ( BotGetTargetType( target ) == ET_BUILDABLE || BotGetTargetTeam( target ) == TEAM_ALIENS )
	{
		//make lucifer cannons aim ahead based on the target's velocity
		if ( self->client->ps.weapon == WP_LUCIFER_CANNON && self->botMind->botSkill.level >= 5 )
		{
			VectorMA( aimLocation, Distance( self->s.origin, aimLocation ) / LCANNON_SPEED, target.ent->s.pos.trDelta, aimLocation );
		}
	}
}

int BotGetAimPredictionTime( gentity_t *self )
{
	return ( 10 - self->botMind->botSkill.level ) * 100 * MAX( ( ( float ) rand() ) / RAND_MAX, 0.5f );
}

void BotPredictPosition( gentity_t *self, gentity_t *predict, vec3_t pos, int time )
{
	botTarget_t target;
	vec3_t aimLoc;
	BotSetTarget( &target, predict, NULL );
	BotGetIdealAimLocation( self, target, aimLoc );
	VectorMA( aimLoc, time / 1000.0f, predict->s.apos.trDelta, pos );
}

void BotAimAtEnemy( gentity_t *self )
{
	vec3_t desired;
	vec3_t current;
	vec3_t viewOrigin;
	vec3_t newAim;
	vec3_t angles;
	int i;
	float frac;
	gentity_t *enemy = self->botMind->goal.ent;

	if ( self->botMind->futureAimTime < level.time )
	{
		int predictTime = self->botMind->futureAimTimeInterval = BotGetAimPredictionTime( self );
		BotPredictPosition( self, enemy, self->botMind->futureAim, predictTime );
		self->botMind->futureAimTime = level.time + predictTime;
	}

	BG_GetClientViewOrigin( &self->client->ps, viewOrigin );
	VectorSubtract( self->botMind->futureAim, viewOrigin, desired );
	VectorNormalize( desired );
	AngleVectors( self->client->ps.viewangles, current, NULL, NULL );

	frac = ( 1.0f - ( ( float ) ( self->botMind->futureAimTime - level.time ) ) / self->botMind->futureAimTimeInterval );
	VectorLerp( current, desired, frac, newAim );

	VectorSet( self->client->ps.delta_angles, 0, 0, 0 );
	vectoangles( newAim, angles );

	for ( i = 0; i < 3; i++ )
	{
		self->botMind->cmdBuffer.angles[ i ] = ANGLE2SHORT( angles[ i ] );
	}
}

void BotAimAtLocation( gentity_t *self, vec3_t target )
{
	vec3_t aimVec, aimAngles, viewBase;
	int i;
	usercmd_t *rAngles = &self->botMind->cmdBuffer;

	if ( ! ( self && self->client ) )
	{
		return;
	}

	BG_GetClientViewOrigin( &self->client->ps, viewBase );
	VectorSubtract( target, viewBase, aimVec );

	vectoangles( aimVec, aimAngles );

	VectorSet( self->client->ps.delta_angles, 0.0f, 0.0f, 0.0f );

	for ( i = 0; i < 3; i++ )
	{
		aimAngles[i] = ANGLE2SHORT( aimAngles[i] );
	}

	//save bandwidth
	SnapVector( aimAngles );
	rAngles->angles[0] = aimAngles[0];
	rAngles->angles[1] = aimAngles[1];
	rAngles->angles[2] = aimAngles[2];
}

void BotSlowAim( gentity_t *self, vec3_t target, float slowAmount )
{
	vec3_t viewBase;
	vec3_t aimVec, forward;
	vec3_t skilledVec;
	float length;
	float slow;
	float cosAngle;

	if ( !( self && self->client ) )
	{
		return;
	}
	//clamp to 0-1
	slow = Com_Clamp( 0.1f, 1.0, slowAmount );

	//get the point that the bot is aiming from
	BG_GetClientViewOrigin( &self->client->ps, viewBase );

	//get the Vector from the bot to the enemy (ideal aim Vector)
	VectorSubtract( target, viewBase, aimVec );
	length = VectorNormalize( aimVec );

	//take the current aim Vector
	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );

	cosAngle = DotProduct( forward, aimVec );
	cosAngle = ( cosAngle + 1 ) / 2;
	cosAngle = 1 - cosAngle;
	cosAngle = Com_Clamp( 0.1, 0.5, cosAngle );
	VectorLerp( forward, aimVec, slow * ( cosAngle ), skilledVec );

	//now find a point to return, this point will be aimed at
	VectorMA( viewBase, length, skilledVec, target );
}

float BotAimNegligence( gentity_t *self, botTarget_t target )
{
	vec3_t forward;
	vec3_t ideal;
	vec3_t targetPos;
	vec3_t viewPos;
	float angle;
	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	BG_GetClientViewOrigin( &self->client->ps, viewPos );
	BotGetIdealAimLocation( self, target, targetPos );
	VectorSubtract( targetPos, viewPos, ideal );
	VectorNormalize( ideal );
	angle = DotProduct( ideal, forward );
	angle = RAD2DEG( acos( angle ) );
	return angle;
}

/*
========================
Bot Team Querys
========================
*/

int FindBots( int *botEntityNumbers, int maxBots, team_t team )
{
	gentity_t *testEntity;
	int numBots = 0;
	int i;
	memset( botEntityNumbers, 0, sizeof( int )*maxBots );
	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		testEntity = &g_entities[i];
		if ( testEntity->r.svFlags & SVF_BOT )
		{
			if ( testEntity->client->pers.teamSelection == team && numBots < maxBots )
			{
				botEntityNumbers[numBots++] = i;
			}
		}
	}
	return numBots;
}

qboolean PlayersBehindBotInSpawnQueue( gentity_t *self )
{
	//this function only checks if there are Humans in the SpawnQueue
	//which are behind the bot
	int i;
	int botPos = 0, lastPlayerPos = 0;
	spawnQueue_t *sq;

	if ( self->client->pers.teamSelection == TEAM_HUMANS )
	{
		sq = &level.humanSpawnQueue;
	}

	else if ( self->client->pers.teamSelection == TEAM_ALIENS )
	{
		sq = &level.alienSpawnQueue;
	}
	else
	{
		return qfalse;
	}

	i = sq->front;

	if ( G_GetSpawnQueueLength( sq ) )
	{
		do
		{
			if ( !( g_entities[sq->clients[ i ]].r.svFlags & SVF_BOT ) )
			{
				if ( i < sq->front )
				{
					lastPlayerPos = i + MAX_CLIENTS - sq->front;
				}
				else
				{
					lastPlayerPos = i - sq->front;
				}
			}

			if ( sq->clients[ i ] == self->s.number )
			{
				if ( i < sq->front )
				{
					botPos = i + MAX_CLIENTS - sq->front;
				}
				else
				{
					botPos = i - sq->front;
				}
			}

			i = QUEUE_PLUS1( i );
		}
		while ( i != QUEUE_PLUS1( sq->back ) );
	}

	if ( botPos < lastPlayerPos )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

qboolean BotTeamateHasWeapon( gentity_t *self, int weapon )
{
	int botNumbers[MAX_CLIENTS];
	int i;
	int numBots = FindBots( botNumbers, MAX_CLIENTS, ( team_t ) self->client->ps.stats[STAT_TEAM] );

	for ( i = 0; i < numBots; i++ )
	{
		gentity_t *bot = &g_entities[botNumbers[i]];
		if ( bot == self )
		{
			continue;
		}
		if ( BG_InventoryContainsWeapon( weapon, bot->client->ps.stats ) )
		{
			return qtrue;
		}
	}
	return qfalse;
}

/*
========================
Boolean Functions for determining actions
========================
*/

botTarget_t BotGetRoamTarget( gentity_t *self )
{
	botTarget_t target;
	vec3_t targetPos;

	BotFindRandomPointOnMesh( self, targetPos );
	BotSetTarget( &target, NULL, &targetPos );
	return target;
}

/*
========================
Misc Bot Stuff
========================
*/
void BotFireWeapon( weaponMode_t mode, usercmd_t *botCmdBuffer )
{
	if ( mode == WPM_PRIMARY )
	{
		usercmdPressButton( botCmdBuffer->buttons, BUTTON_ATTACK );
	}
	else if ( mode == WPM_SECONDARY )
	{
		usercmdPressButton( botCmdBuffer->buttons, BUTTON_ATTACK2 );
	}
	else if ( mode == WPM_TERTIARY )
	{
		usercmdPressButton( botCmdBuffer->buttons, BUTTON_USE_HOLDABLE );
	}
}
void BotClassMovement( gentity_t *self, qboolean inAttackRange )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	switch ( self->client->ps.stats[STAT_CLASS] )
	{
		case PCL_ALIEN_LEVEL0:
			BotStrafeDodge( self );
			break;
		case PCL_ALIEN_LEVEL1:
		case PCL_ALIEN_LEVEL1_UPG:
			if ( BotTargetIsPlayer( self->botMind->goal ) && ( self->botMind->goal.ent->client->ps.stats[STAT_STATE] & SS_GRABBED ) && inAttackRange )
			{
				if ( self->botMind->botSkill.level == 10 )
				{
					BotStandStill( self );
					BotStrafeDodge( self );//only move if skill == 10 because otherwise we wont aim fast enough to not lose grab
				}
				else
				{
					BotStandStill( self );
				}
			}
			else
			{
				BotStrafeDodge( self );
			}
			break;
		case PCL_ALIEN_LEVEL2:
		case PCL_ALIEN_LEVEL2_UPG:
			if ( self->botMind->directPathToGoal )
			{
				if ( self->client->time1000 % 300 == 0 )
				{
					BotJump( self );
				}
				BotStrafeDodge( self );
			}
			break;
		case PCL_ALIEN_LEVEL3:
			break;
		case PCL_ALIEN_LEVEL3_UPG:
			if ( BotGetTargetType( self->botMind->goal ) == ET_BUILDABLE && self->client->ps.ammo > 0
			        && inAttackRange )
			{
				//dont move when sniping buildings
				BotStandStill( self );
			}
			break;
		case PCL_ALIEN_LEVEL4:
			//use rush to approach faster
			if ( !inAttackRange )
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );
			}
			break;
		default:
			break;
	}
}
void BotFireWeaponAI( gentity_t *self )
{
	float distance;
	vec3_t targetPos;
	vec3_t forward, right, up;
	vec3_t muzzle;
	trace_t trace;
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	AngleVectors( self->client->ps.viewangles, forward, right, up );
	CalcMuzzlePoint( self, forward, right, up, muzzle );
	BotGetIdealAimLocation( self, self->botMind->goal, targetPos );

	trap_Trace( &trace, muzzle, NULL, NULL, targetPos, -1, MASK_SHOT );
	distance = Distance( muzzle, trace.endpos );
	switch ( self->s.weapon )
	{
		case WP_ABUILD:
			if ( distance <= ABUILDER_CLAW_RANGE )
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );
			}
			else
			{
				usercmdPressButton( botCmdBuffer->buttons, BUTTON_GESTURE );    //make cute granger sounds to ward off the would be attackers
			}
			break;
		case WP_ABUILD2:
			if ( distance <= ABUILDER_CLAW_RANGE )
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );    //swipe
			}
			else
			{
				BotFireWeapon( WPM_TERTIARY, botCmdBuffer );    //blob launcher
			}
			break;
		case WP_ALEVEL0:
			break; //auto hit
		case WP_ALEVEL1:
			BotFireWeapon( WPM_PRIMARY, botCmdBuffer ); //basi swipe
			break;
		case WP_ALEVEL1_UPG:
			if ( distance <= LEVEL1_CLAW_U_RANGE )
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );    //basi swipe
			}
			/*
			else
			BotFireWeapn(WPM_SECONDARY,botCmdBuffer); //basi poisen
			*/
			break;
		case WP_ALEVEL2:
			BotFireWeapon( WPM_PRIMARY, botCmdBuffer ); //mara swipe
			break;
		case WP_ALEVEL2_UPG:
			if ( distance <= LEVEL2_CLAW_U_RANGE )
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );    //mara swipe
			}
			else
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );    //mara lightning
			}
			break;
		case WP_ALEVEL3:
			if ( distance > LEVEL3_CLAW_RANGE && self->client->ps.stats[ STAT_MISC ] < LEVEL3_POUNCE_TIME )
			{
				botCmdBuffer->angles[PITCH] = ANGLE2SHORT( -CalcPounceAimPitch( self, self->botMind->goal ) ); //compute and apply correct aim pitch to hit target
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer ); //goon pounce
			}
			else
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );    //goon chomp
			}
			break;
		case WP_ALEVEL3_UPG:
			if ( self->client->ps.ammo > 0 && distance > LEVEL3_CLAW_UPG_RANGE )
			{
				botCmdBuffer->angles[PITCH] = ANGLE2SHORT( -CalcBarbAimPitch( self, self->botMind->goal ) ); //compute and apply correct aim pitch to hit target
				BotFireWeapon( WPM_TERTIARY, botCmdBuffer ); //goon barb
			}
			else if ( distance > LEVEL3_CLAW_UPG_RANGE && self->client->ps.stats[ STAT_MISC ] < LEVEL3_POUNCE_TIME_UPG )
			{
				botCmdBuffer->angles[PITCH] = ANGLE2SHORT( -CalcPounceAimPitch( self, self->botMind->goal ) ); //compute and apply correct aim pitch to hit target
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer ); //goon pounce
			}
			else
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );    //goon chomp
			}
			break;
		case WP_ALEVEL4:
			if ( distance > LEVEL4_CLAW_RANGE && self->client->ps.stats[STAT_MISC] < LEVEL4_TRAMPLE_CHARGE_MAX )
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );    //rant charge
			}
			else
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );    //rant swipe
			}
			break;
		case WP_LUCIFER_CANNON:
			if ( self->client->ps.stats[STAT_MISC] < LCANNON_CHARGE_TIME_MAX * Com_Clamp( 0.5, 1, random() ) )
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );
			}
			break;
		default:
			BotFireWeapon( WPM_PRIMARY, botCmdBuffer );
	}
}

void G_BotLoadBuildLayout()
{
	fileHandle_t f;
	int len;
	char *layout, *layoutHead;
	char map[ MAX_QPATH ];
	char buildName[ MAX_TOKEN_CHARS ];
	buildable_t buildable;
	vec3_t origin = { 0.0f, 0.0f, 0.0f };
	vec3_t angles = { 0.0f, 0.0f, 0.0f };
	vec3_t origin2 = { 0.0f, 0.0f, 0.0f };
	vec3_t angles2 = { 0.0f, 0.0f, 0.0f };
	char line[ MAX_STRING_CHARS ];
	int i = 0;
	level.botBuildLayout.numBuildings = 0;
	if ( !g_bot_buildLayout.string[0] || !Q_stricmp( g_bot_buildLayout.string, "*BUILTIN*" ) )
	{
		return;
	}

	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
	trap_Print( "Attempting to Load Bot Build Layout File...\n" );
	len = trap_FS_FOpenFile( va( "layouts/%s/%s.dat", map, g_bot_buildLayout.string ),
	                         &f, FS_READ );
	if ( len < 0 )
	{
		G_Printf( "ERROR: layout %s could not be opened\n", g_bot_buildLayout.string );
		return;
	}
	layoutHead = layout = ( char * )BG_Alloc( len + 1 );
	trap_FS_Read( layout, len, f );
	layout[ len ] = '\0';
	trap_FS_FCloseFile( f );
	while ( *layout )
	{
		if ( i >= sizeof( line ) - 1 )
		{
			G_Printf( S_COLOR_RED "ERROR: line overflow in %s before \"%s\"\n",
			          va( "layouts/%s/%s.dat", map, g_bot_buildLayout.string ), line );
			break;
		}
		line[ i++ ] = *layout;
		line[ i ] = '\0';
		if ( *layout == '\n' )
		{
			i = 0;
			sscanf( line, "%s %f %f %f %f %f %f %f %f %f %f %f %f\n",
			        buildName,
			        &origin[ 0 ], &origin[ 1 ], &origin[ 2 ],
			        &angles[ 0 ], &angles[ 1 ], &angles[ 2 ],
			        &origin2[ 0 ], &origin2[ 1 ], &origin2[ 2 ],
			        &angles2[ 0 ], &angles2[ 1 ], &angles2[ 2 ] );

			buildable = BG_BuildableByName( buildName )->number;
			if ( buildable <= BA_NONE || buildable >= BA_NUM_BUILDABLES )
				G_Printf( S_COLOR_YELLOW "WARNING: bad buildable name (%s) in layout."
				          " skipping\n", buildName );
			else if ( level.botBuildLayout.numBuildings == MAX_BOT_BUILDINGS )
			{
				G_Printf( S_COLOR_YELLOW "WARNING: reached max buildings for bot layout (%d)."
				          " skipping\n", MAX_BOT_BUILDINGS );
			}
			else if ( BG_Buildable( buildable )->team == TEAM_HUMANS && level.botBuildLayout.numBuildings < MAX_BOT_BUILDINGS )
			{
				level.botBuildLayout.buildings[level.botBuildLayout.numBuildings].type = buildable;
				VectorCopy( origin, level.botBuildLayout.buildings[level.botBuildLayout.numBuildings].origin );
				VectorCopy( origin2, level.botBuildLayout.buildings[level.botBuildLayout.numBuildings].normal );
				level.botBuildLayout.numBuildings++;
			}

		}
		layout++;
	}
	BG_Free( layoutHead );
}

void BotSetSkillLevel( gentity_t *self, int skill )
{
	self->botMind->botSkill.level = skill;
	//different aim for different teams
	if ( self->botMind->botTeam == TEAM_HUMANS )
	{
		self->botMind->botSkill.aimSlowness = ( float ) skill / 10;
		self->botMind->botSkill.aimShake = ( int ) ( 10 - skill );
	}
	else
	{
		self->botMind->botSkill.aimSlowness = ( float ) skill / 10;
		self->botMind->botSkill.aimShake = ( int ) ( 10 - skill );
	}
}

/*
=======================
Bot management functions
=======================
*/
static struct
{
	int count;
	struct
	{
		char *name;
		qboolean inUse;
	} name[MAX_CLIENTS];
} botNames[NUM_TEAMS];

static void G_BotListTeamNames( gentity_t *ent, const char *heading, team_t team, const char *marker )
{
	int i;

	if ( botNames[team].count )
	{
		ADMP( heading );
		ADMBP_begin();

		for ( i = 0; i < botNames[team].count; ++i )
		{
			ADMBP( va( "  %s^7 %s\n", botNames[team].name[i].inUse ? marker : " ", botNames[team].name[i].name ) );
		}

		ADMBP_end();
	}
}

void G_BotListNames( gentity_t *ent )
{
	G_BotListTeamNames( ent, QQ( N_( "^3Alien bot names:\n" ) ), TEAM_ALIENS, "^1*" );
	G_BotListTeamNames( ent, QQ( N_( "^3Human bot names:\n" ) ), TEAM_HUMANS, "^5*" );
}

qboolean G_BotClearNames( void )
{
	int i;

	for ( i = 0; i < botNames[TEAM_ALIENS].count; ++i )
		if ( botNames[TEAM_ALIENS].name[i].inUse )
		{
			return qfalse;
		}

	for ( i = 0; i < botNames[TEAM_HUMANS].count; ++i )
		if ( botNames[TEAM_HUMANS].name[i].inUse )
		{
			return qfalse;
		}

	for ( i = 0; i < botNames[TEAM_ALIENS].count; ++i )
	{
		BG_Free( botNames[TEAM_ALIENS].name[i].name );
	}

	for ( i = 0; i < botNames[TEAM_HUMANS].count; ++i )
	{
		BG_Free( botNames[TEAM_HUMANS].name[i].name );
	}

	botNames[TEAM_ALIENS].count = 0;
	botNames[TEAM_HUMANS].count = 0;

	return qtrue;
}

int G_BotAddNames( team_t team, int arg, int last )
{
	int  i = botNames[team].count;
	int  added = 0;
	char name[MAX_NAME_LENGTH];

	while ( arg < last && i < MAX_CLIENTS )
	{
		int j, t;

		trap_Argv( arg++, name, sizeof( name ) );

		// name already in the list? (quick check, including colours & invalid)
		for ( t = 1; t < NUM_TEAMS; ++t )
			for ( j = 0; j < botNames[t].count; ++j )
				if ( !Q_stricmp( botNames[t].name[j].name, name ) )
				{
					goto next;
				}

		botNames[team].name[i].name = ( char * )BG_Alloc( strlen( name ) + 1 );
		strcpy( botNames[team].name[i].name, name );

		botNames[team].count = ++i;
		++added;

next:
		;
	}

	return added;
}

static char *G_BotSelectName( team_t team )
{
	unsigned int i, choice;

	if ( botNames[team].count < 1 )
	{
		return NULL;
	}

	choice = rand() % botNames[team].count;

	for ( i = 0; i < botNames[team].count; ++i )
	{
		if ( !botNames[team].name[choice].inUse )
		{
			return botNames[team].name[choice].name;
		}
		choice = ( choice + 1 ) % botNames[team].count;
	}

	return NULL;
}

static void G_BotNameUsed( team_t team, const char *name, qboolean inUse )
{
	unsigned int i;

	for ( i = 0; i < botNames[team].count; ++i )
	{
		if ( !Q_stricmp( name, botNames[team].name[i].name ) )
		{
			botNames[team].name[i].inUse = inUse;
			return;
		}
	}
}

qboolean G_BotSetDefaults( int clientNum, team_t team, int skill, const char* behavior )
{
	botMemory_t *botMind;
	gentity_t *self = &g_entities[ clientNum ];
	botMind = self->botMind = &g_botMind[clientNum];

	botMind->enemyLastSeen = 0;
	botMind->botTeam = team;
	BotSetNavmesh( self, self->client->ps.stats[ STAT_CLASS ] );

	memset( botMind->runningNodes, 0, sizeof( botMind->runningNodes ) );
	botMind->numRunningNodes = 0;
	botMind->currentNode = NULL;
	botMind->directPathToGoal = qfalse;

	botMind->behaviorTree = ReadBehaviorTree( behavior );

	if ( !botMind->behaviorTree )
	{
		G_Printf( "Problem when loading behavior tree %s, trying default\n", behavior );
		botMind->behaviorTree = ReadBehaviorTree( "default" );

		if ( !botMind->behaviorTree )
		{
			G_Printf( "Problem when loading default behavior tree\n" );
			return qfalse;
		}
	}

	BotSetSkillLevel( &g_entities[clientNum], skill );

	g_entities[clientNum].r.svFlags |= SVF_BOT;

	if ( team != TEAM_NONE )
	{
		level.clients[clientNum].sess.restartTeam = team;
	}

	self->pain = BotPain;

	return qtrue;
}

qboolean G_BotAdd( char *name, team_t team, int skill, const char *behavior )
{
	int clientNum;
	char userinfo[MAX_INFO_STRING];
	const char* s = 0;
	gentity_t *bot;
	qboolean autoname = qfalse;
	qboolean okay;

	if ( !navMeshLoaded )
	{
		trap_Print( "No Navigation Mesh file is available for this map\n" );
		return qfalse;
	}

	// find what clientNum to use for bot
	clientNum = trap_BotAllocateClient( 0 );

	if ( clientNum < 0 )
	{
		trap_Print( "no more slots for bot\n" );
		return qfalse;
	}
	bot = &g_entities[ clientNum ];
	bot->r.svFlags |= SVF_BOT;
	bot->inuse = qtrue;

	if ( !Q_stricmp( name, "*" ) )
	{
		name = G_BotSelectName( team );
		autoname = name != NULL;
	}

	//default bot data
	okay = G_BotSetDefaults( clientNum, team, skill, behavior );

	// register user information
	userinfo[0] = '\0';
	Info_SetValueForKey( userinfo, "name", name ? name : "", qfalse ); // allow defaulting
	Info_SetValueForKey( userinfo, "rate", "25000", qfalse );
	Info_SetValueForKey( userinfo, "snaps", "20", qfalse );
	if ( autoname )
	{
		Info_SetValueForKey( userinfo, "autoname", name, qfalse );
	}

	//so we can connect if server is password protected
	if ( g_needpass.integer == 1 )
	{
		Info_SetValueForKey( userinfo, "password", g_password.string, qfalse );
	}

	trap_SetUserinfo( clientNum, userinfo );

	// have it connect to the game as a normal client
	if ( ( s = ClientBotConnect( clientNum, qtrue, team ) ) )
	{
		// won't let us join
		trap_Print( s );
		okay = qfalse;
	}

	if ( !okay )
	{
		G_BotDel( clientNum );
		return qfalse;
	}

	if ( autoname )
	{
		G_BotNameUsed( team, name, qtrue );
	}

	ClientBegin( clientNum );
	bot->pain = BotPain; // ClientBegin resets the pain function
	return qtrue;
}

void G_BotDel( int clientNum )
{
	gentity_t *bot = &g_entities[clientNum];
	char userinfo[MAX_INFO_STRING];
	const char *autoname;

	if ( !( bot->r.svFlags & SVF_BOT ) || !bot->botMind )
	{
		trap_Print( va( "'^7%s^7' is not a bot\n", bot->client->pers.netname ) );
		return;
	}

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	autoname = Info_ValueForKey( userinfo, "autoname" );
	if ( autoname && *autoname )
	{
		G_BotNameUsed( BotGetEntityTeam( bot ), autoname, qfalse );
	}

	trap_SendServerCommand( -1, va( "print_tr %s %s", QQ( N_( "$1$^7 disconnected\n" ) ),
	                                Quote( bot->client->pers.netname ) ) );
	trap_DropClient( clientNum, "disconnected" );
}

void G_BotFreeBehaviorTrees( void )
{
	int i;
	for ( i = 0; i < treeList.numTrees; i++ )
	{
		AIBehaviorTree_t *tree = treeList.trees[ i ];
		FreeBehaviorTree( tree );
	}
	FreeTreeList( &treeList );
}

void G_BotDelAllBots( void )
{
	int i;

	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( g_entities[i].r.svFlags & SVF_BOT && level.clients[i].pers.connected != CON_DISCONNECTED )
		{
			G_BotDel( i );
		}
	}

	for ( i = 0; i < botNames[TEAM_ALIENS].count; ++i )
	{
		botNames[TEAM_ALIENS].name[i].inUse = qfalse;
	}

	for ( i = 0; i < botNames[TEAM_HUMANS].count; ++i )
	{
		botNames[TEAM_HUMANS].name[i].inUse = qfalse;
	}
}

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
			vec3_t pos;
			BotAimAtEnemy( self );

			BotGetTargetPos( self->botMind->goal, pos );

			//update the path corridor
			trap_BotUpdatePath( self->s.number, pos, NULL, &self->botMind->directPathToGoal );

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

float RadiusFromBounds2D( vec3_t mins, vec3_t maxs )
{
	float rad1 = sqrt( Square( mins[0] ) + Square( mins[1] ) );
	float rad2 = sqrt( Square( maxs[0] ) + Square( maxs[1] ) );
	if ( rad1 > rad2 )
	{
		return rad1;
	}
	else
	{
		return rad2;
	}
}

float BotGetGoalRadius( gentity_t *self )
{
	if ( BotTargetIsEntity( self->botMind->goal ) )
	{
		botTarget_t *t = &self->botMind->goal;
		if ( t->ent->s.modelindex == BA_H_MEDISTAT || t->ent->s.modelindex == BA_A_BOOSTER )
		{
			return self->r.maxs[0] + t->ent->r.maxs[0];
		}
		else
		{
			return RadiusFromBounds2D( t->ent->r.mins, t->ent->r.maxs ) + RadiusFromBounds2D( self->r.mins, self->r.maxs );
		}
	}
	else
	{
		return RadiusFromBounds2D( self->r.mins, self->r.maxs );
	}
}

float DistanceToGoal2DSquared( gentity_t *self )
{
	vec3_t vec;
	vec3_t goalPos;

	BotGetTargetPos( self->botMind->goal, goalPos );

	VectorSubtract( goalPos, self->s.origin, vec );

	return Square( vec[ 0 ] ) + Square( vec[ 1 ] );
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

AINodeStatus_t BotEvaluateNode( gentity_t *self, AIGenericNode_t *node );

qboolean NodeIsRunning( gentity_t *self, AIGenericNode_t *node )
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

void BotPain( gentity_t *self, gentity_t *attacker, int damage )
{
	if ( BotGetEntityTeam( attacker ) != TEAM_NONE && BotGetEntityTeam( attacker ) != self->client->ps.stats[ STAT_TEAM ] )
	{
		if ( attacker->s.eType == ET_PLAYER )
		{
			self->botMind->bestEnemy.ent = attacker;
			self->botMind->bestEnemy.distance = Distance( self->s.origin, attacker->s.origin );
			self->botMind->enemyLastSeen = level.time;
			self->botMind->timeFoundEnemy = level.time - g_bot_reactiontime.integer; // alert immediately
		}
	}
}

void BotSearchForEnemy( gentity_t *self )
{
	if ( !self->botMind->bestEnemy.ent || level.time - self->botMind->enemyLastSeen > g_bot_chasetime.integer )
	{
		botTarget_t target;
		gentity_t *enemy = BotFindBestEnemy( self );

		if ( enemy )
		{
			BotSetTarget( &target, enemy, NULL );

			if ( enemy->s.eType != ET_PLAYER || ( enemy->s.eType == ET_PLAYER 
				&& ( self->client->ps.stats[ STAT_TEAM ] == TEAM_ALIENS || BotAimNegligence( self, target ) <= g_bot_fov.value / 2 ) ) )
			{
				self->botMind->bestEnemy.ent = enemy;
				self->botMind->bestEnemy.distance = Distance( self->s.origin, enemy->s.origin );
				self->botMind->enemyLastSeen = level.time;
				self->botMind->timeFoundEnemy = level.time;
			}
		}
		else
		{
			self->botMind->bestEnemy.ent = NULL;
		}
	}
}

/*
=======================
Bot Thinks
=======================
*/
void G_BotThink( gentity_t *self )
{
	char buf[MAX_STRING_CHARS];
	usercmd_t *botCmdBuffer;
	vec3_t     nudge;
	gentity_t *bestDamaged;

	self->botMind->cmdBuffer = self->client->pers.cmd;
	botCmdBuffer = &self->botMind->cmdBuffer;

	//reset command buffer
	usercmdClearButtons( botCmdBuffer->buttons );

	// for nudges, e.g. spawn blocking
	nudge[0] = botCmdBuffer->doubleTap ? botCmdBuffer->forwardmove : 0;
	nudge[1] = botCmdBuffer->doubleTap ? botCmdBuffer->rightmove : 0;
	nudge[2] = botCmdBuffer->doubleTap ? botCmdBuffer->upmove : 0;

	botCmdBuffer->forwardmove = 0;
	botCmdBuffer->rightmove = 0;
	botCmdBuffer->upmove = 0;
	botCmdBuffer->doubleTap = 0;

	//acknowledge recieved server commands
	//MUST be done
	while ( trap_BotGetServerCommand( self->client->ps.clientNum, buf, sizeof( buf ) ) );

	BotSearchForEnemy( self );

	BotFindClosestBuildings( self, self->botMind->closestBuildings );

	bestDamaged = BotFindDamagedFriendlyStructure( self );
	self->botMind->closestDamagedBuilding.ent = bestDamaged;
	self->botMind->closestDamagedBuilding.distance = ( !bestDamaged ) ? 0 :  Distance( self->s.origin, bestDamaged->s.origin );

	//use medkit when hp is low
	if ( self->health < BOT_USEMEDKIT_HP && BG_InventoryContainsUpgrade( UP_MEDKIT, self->client->ps.stats ) )
	{
		BG_ActivateUpgrade( UP_MEDKIT, self->client->ps.stats );
	}

	//infinite funds cvar
	if ( g_bot_infinite_funds.integer )
	{
		G_AddCreditToClient( self->client, HUMAN_MAX_CREDITS, qtrue );
	}

	//hacky ping fix
	self->client->ps.ping = rand() % 50 + 50;

	if ( !self->botMind->behaviorTree )
	{
		G_Printf( "ERROR: NULL behavior tree\n" );
		return;
	}

	BotEvaluateNode( self, ( AIGenericNode_t * ) self->botMind->behaviorTree->root );

	// if we were nudged...
	VectorAdd( self->client->ps.velocity, nudge, self->client->ps.velocity );

	self->client->pers.cmd = self->botMind->cmdBuffer;
}

void G_BotSpectatorThink( gentity_t *self )
{
	char buf[MAX_STRING_CHARS];
	//hacky ping fix
	self->client->ps.ping = rand() % 50 + 50;

	//acknowledge recieved console messages
	//MUST be done
	while ( trap_BotGetServerCommand( self->client->ps.clientNum, buf, sizeof( buf ) ) );

	if ( self->client->ps.pm_flags & PMF_QUEUED )
	{
		//we're queued to spawn, all good
		//check for humans in the spawn que
		{
			spawnQueue_t *sq;
			if ( self->client->pers.teamSelection == TEAM_HUMANS )
			{
				sq = &level.humanSpawnQueue;
			}
			else if ( self->client->pers.teamSelection == TEAM_ALIENS )
			{
				sq = &level.alienSpawnQueue;
			}
			else
			{
				sq = NULL;
			}

			if ( sq && PlayersBehindBotInSpawnQueue( self ) )
			{
				G_RemoveFromSpawnQueue( sq, self->s.number );
				G_PushSpawnQueue( sq, self->s.number );
			}
		}
		return;
	}

	//reset stuff
	BotSetTarget( &self->botMind->goal, NULL, NULL );
	self->botMind->bestEnemy.ent = NULL;
	self->botMind->timeFoundEnemy = 0;
	self->botMind->enemyLastSeen = 0;

	if ( self->client->sess.restartTeam == TEAM_NONE )
	{
		int teamnum = self->client->pers.teamSelection;
		int clientNum = self->client->ps.clientNum;

		if ( teamnum == TEAM_HUMANS )
		{
			self->client->pers.classSelection = PCL_HUMAN;
			self->client->ps.stats[STAT_CLASS] = PCL_HUMAN;
			BotSetNavmesh( self, PCL_HUMAN );
			//we want to spawn with rifle unless it is disabled or we need to build
			if ( g_bot_rifle.integer )
			{
				self->client->pers.humanItemSelection = WP_MACHINEGUN;
			}
			else
			{
				self->client->pers.humanItemSelection = WP_HBUILD;
			}

			G_PushSpawnQueue( &level.humanSpawnQueue, clientNum );
		}
		else if ( teamnum == TEAM_ALIENS )
		{
			self->client->pers.classSelection = PCL_ALIEN_LEVEL0;
			self->client->ps.stats[STAT_CLASS] = PCL_ALIEN_LEVEL0;
			BotSetNavmesh( self, PCL_ALIEN_LEVEL0 );
			G_PushSpawnQueue( &level.alienSpawnQueue, clientNum );
		}
	}
	else
	{
		G_ChangeTeam( self, self->client->sess.restartTeam );
		self->client->sess.restartTeam = TEAM_NONE;
	}
}

void G_BotIntermissionThink( gclient_t *client )
{
	client->readyToExit = qtrue;
}

void G_BotInit( void )
{
	G_BotNavInit( );
	if ( treeList.maxTrees == 0 )
	{
		InitTreeList( &treeList );
	}
}

void G_BotCleanup( int restart )
{
	if ( !restart )
	{
		int i;

		for ( i = 0; i < MAX_CLIENTS; ++i )
		{
			if ( g_entities[i].r.svFlags & SVF_BOT && level.clients[i].pers.connected != CON_DISCONNECTED )
			{
				G_BotDel( i );
			}
		}

		G_BotClearNames();
	}
	G_BotFreeBehaviorTrees();
	G_BotNavCleanup();
}
