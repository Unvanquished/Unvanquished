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


#include "g_local.h"
#include "g_bot.h"
#include "../../engine/botlib/nav.h"

#ifndef RAND_MAX
#define RAND_MAX 32768
#endif

botMemory_t g_botMind[MAX_CLIENTS];

void BotDPrintf(const char* fmt, ...) {
	if(g_bot_debug.integer) {
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

#define allocNode( T ) ( T * ) BG_Alloc( sizeof( T ) );

#define stringify( v ) va( #v " %d", v )

AINode_t *ReadNodeList( int handle );
AINode_t *ReadConditionNode( int handle );
AINode_t *ReadActionNode( int handle );

void CheckToken( int handle, const char *tokenValueName, const char *nodename, const pc_token_t *token, int requiredType )
{
	char filename[ MAX_QPATH ];
	int line;

	if ( token->type != requiredType )
	{
		trap_Parse_SourceFileAndLine( handle, filename, &line );
		BotDPrintf( S_COLOR_RED "ERROR: When parsing %s, Invalid %s %s after %s on line %d\n", filename, tokenValueName, token->string, nodename, line );
	}
}

AINode_t *ReadConditionNode( int handle )
{
	pc_token_t token;
	AICondition_t *condition;

	trap_Parse_ReadToken( handle, &token );

	condition = allocNode( AICondition_t );
	condition->type = CONDITION_NODE;

	// read not or bool
	if ( !Q_stricmp( token.string, "not" ) )
	{
		condition->op = OP_NOT;

		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			BotDPrintf( S_COLOR_RED "ERROR: no token after operator not\n" );
		}
	}
	else
	{
		condition->op = OP_BOOL;
	}

	if ( !Q_stricmp( token.string, "buildingIsDamaged" ) )
	{
		condition->info = CON_DAMAGEDBUILDING;
	}
	else if ( !Q_stricmp( token.string, "alertedToEnemy" ) )
	{
		condition->info = CON_ENEMY;
	}
	else if ( !Q_stricmp( token.string, "haveWeapon" ) )
	{
		condition->info = CON_WEAPON;

		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "weapon", "condition haveWeapon", &token, TT_NUMBER );

		condition->param1.type = VALUE_INT;
		condition->param1.value.i = token.intvalue;
	}
	else if ( !Q_stricmp( token.string, "haveUpgrade" ) ) 
	{
		condition->info = CON_UPGRADE;

		trap_Parse_ReadToken( handle, &token );
		
		CheckToken( handle, "upgrade", "condition haveUpgrade", &token, TT_NUMBER );

		condition->param1.type = VALUE_INT;
		condition->param1.value.i = token.intvalue;
	}
	else if ( !Q_stricmp( token.string, "teamateHasWeapon" ) )
	{
		condition->info = CON_TEAM_WEAPON;
		
		trap_Parse_ReadToken( handle, &token );		
		
		CheckToken( handle, "weapon", "condition teamateHasWeapon", &token, TT_NUMBER );

		condition->param1.type = VALUE_INT;
		condition->param1.value.i = token.intvalue;
	}
	else if ( !Q_stricmp( token.string, "botTeam" ) )
	{
		condition->info = CON_TEAM;

		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "operator", "condition botTeam", &token, TT_NUMBER );

		condition->op = ( AIConditionOperator_t )token.intvalue;

		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "team", "condition botTeam", &token, TT_NUMBER );

		condition->param1.type = VALUE_INT;
		condition->param1.value.i = token.intvalue;
	}
	else if ( !Q_stricmp( token.string, "percentHealth" ) )
	{
		condition->info = CON_HEALTH;

		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "operator", "condition percentHealth", &token, TT_NUMBER );

		condition->op = ( AIConditionOperator_t ) token.intvalue;

		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "value", "condition percentHealth", &token, TT_NUMBER );

		condition->param1.type = VALUE_FLOAT;
		condition->param1.value.f = token.floatvalue;
	}
	else if ( !Q_stricmp( token.string, "weapon" ) )
	{
		condition->info = CON_CURWEAPON;

		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "operator", "condition weapon", &token, TT_NUMBER );
		condition->op = ( AIConditionOperator_t ) token.intvalue;

		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "weapon", "condition weapon", &token, TT_NUMBER );

		condition->param1.type = VALUE_INT;
		condition->param1.value.i = token.intvalue;
	}
	else if ( !Q_stricmp( token.string, "distanceTo" ) )
	{
		condition->info = CON_DISTANCE;

		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "entity", "condition distanceTo", &token, TT_NUMBER );

		condition->param1.value.i = token.intvalue;

		trap_Parse_ReadToken( handle, &token );
		
		CheckToken( handle, "operator", "condition distanceTo", &token, TT_NUMBER );

		condition->op = ( AIConditionOperator_t ) token.intvalue;

		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "value", "condition distanceTo", &token, TT_NUMBER );

		condition->param2.value.i = token.intvalue;
	}
	else if ( !Q_stricmp ( token.string, "baseRushScore" ) )
	{
		condition->info = CON_BASERUSH;

		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "operator", "condition baseRushScore", &token, TT_NUMBER );

		condition->op = ( AIConditionOperator_t ) token.intvalue;

		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "value", "condition baseRushScore", &token, TT_NUMBER );

		condition->param1.value.f = token.floatvalue;
	}
	else if ( !Q_stricmp( token.string, "healScore" ) )
	{
		condition->info = CON_HEAL;

		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "operator", "condition healScore", &token, TT_NUMBER );

		condition->op = ( AIConditionOperator_t ) token.intvalue;

		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "value", "condition healScore", &token, TT_NUMBER );
		
		condition->param1.value.f = token.floatvalue;
	}
	else
	{
		BotDPrintf( S_COLOR_RED "ERROR: unknown condition %s\n", token.string );
	}

	trap_Parse_ReadToken( handle, &token );

	if ( Q_stricmp( token.string, "{" ) )
	{
		int line;
		char filename[ MAX_QPATH ];
		trap_Parse_SourceFileAndLine( handle, filename, &line );
		BotDPrintf( S_COLOR_RED "ERROR: When parsing %s, found %s expecting { on line %d\n", filename, token.string, line );
	}

	while ( Q_stricmp( token.string, "}" ) )
	{
		trap_Parse_ReadToken( handle, &token );

		if ( !Q_stricmp( token.string, "selector" ) )
		{
			condition->child = ReadNodeList( handle );
		}
		else if ( !Q_stricmp( token.string, "action" ) )
		{
			condition->child = ReadActionNode( handle );
		}
		else if ( !Q_stricmp( token.string, "condition" ) )
		{
			condition->child = ReadConditionNode( handle );
		}
		else if ( Q_stricmp( token.string, "}" ) )
		{
			char filename[ MAX_QPATH ];
			int line;
			trap_Parse_SourceFileAndLine( handle, filename, &line );
			BotDPrintf( S_COLOR_RED "ERROR: When parsing %s, invalid token on line %d found: %s\n", filename, line, token.string );
		}
	}
	return ( AINode_t * ) condition;
}

AINode_t *ReadActionNode( int handle )
{
	pc_token_t token;
	AIActionNode_t *node;
	trap_Parse_ReadToken( handle, &token );

	if ( !Q_stricmp( token.string, "heal" ) )
	{
		node = allocNode( AIActionNode_t );
		node->action = ACTION_HEAL;
	}
	else if ( !Q_stricmp( token.string, "fight" ) )
	{
		node = allocNode( AIActionNode_t );
		node->action = ACTION_FIGHT;
	}
	else if ( !Q_stricmp( token.string, "roam" ) )
	{
		node = allocNode( AIActionNode_t );
		node->action = ACTION_ROAM;
	}
	else if ( !Q_stricmp( token.string, "equip" ) )
	{
		AIBuy_t *realAction = allocNode( AIBuy_t );
		realAction->action = ACTION_BUY;
		realAction->numUpgrades = 0;
		realAction->weapon = WP_NONE;
		node = ( AIActionNode_t * ) realAction;
	}
	else if ( !Q_stricmp( token.string, "buy" ) )
	{
		AIBuy_t *realAction = allocNode( AIBuy_t );
		realAction->action = ACTION_BUY;
		
		trap_Parse_ReadToken( handle, &token );

		CheckToken( handle, "weapon", "action buy", &token, TT_NUMBER );

		realAction->weapon = ( weapon_t ) token.intvalue;
		realAction->numUpgrades = 0;
		node = ( AIActionNode_t * ) realAction;
	}
	else if ( !Q_stricmp( token.string, "flee" ) )
	{
		node = allocNode( AIActionNode_t );
		node->action = ACTION_FLEE;
	}
	else if ( !Q_stricmp( token.string, "repair" ) )
	{
		node = allocNode( AIActionNode_t );
		node->action = ACTION_REPAIR;
	}
	else if ( !Q_stricmp( token.string, "evolve" ) )
	{
		node = allocNode( AIActionNode_t );
		node->action = ACTION_EVOLVE;
	}
	else if ( !Q_stricmp( token.string, "rush" ) )
	{
		node = allocNode( AIActionNode_t );
		node->action = ACTION_RUSH;
	}
	else
	{
		BotDPrintf( S_COLOR_RED "ERROR: Invalid token %s on line %d\n", token.string, token.line );
	}

	node->type = ACTION_NODE;
	return ( AINode_t * ) node;
}

AINode_t *ReadNodeList( int handle )
{
	pc_token_t token;
	AINodeList_t *list;

	trap_Parse_ReadToken( handle, &token );

	list = allocNode( AINodeList_t );

	if ( !Q_stricmp( token.string, "sequence" ) )
	{
		list->selector = SELECTOR_SEQUENCE;
		trap_Parse_ReadToken( handle, &token );
	}
	else if ( !Q_stricmp( token.string, "priority" ) )
	{
		list->selector = SELECTOR_PRIORITY;

		trap_Parse_ReadToken( handle, &token );
	}
	else if ( !Q_stricmp( token.string, "{" ) )
	{
		list->selector = SELECTOR_SELECTOR;
	}
	else
	{
		BotDPrintf( S_COLOR_RED "ERROR: Invalid token %s on line %d\n", token.string, token.line );
	}

	if ( Q_stricmp( token.string, "{" ) )
	{
		BotDPrintf( S_COLOR_RED "ERROR: Invalid token %s on line %d\n", token.string, token.line );
	}

	while( Q_stricmp( token.string, "}" ) )
	{
		trap_Parse_ReadToken( handle, &token );

		if ( !Q_stricmp( token.string, "selector" ) )
		{
			list->list[ list->numNodes ] = ReadNodeList( handle );
			list->numNodes++;
		}
		else if ( !Q_stricmp( token.string, "action" ) )
		{
			list->list[ list->numNodes ] = ReadActionNode( handle );
			list->numNodes++;
		}
		else if ( !Q_stricmp( token.string, "condition" ) )
		{
			list->list[ list->numNodes ] = ReadConditionNode( handle );
			list->numNodes++;
		}
		else if ( Q_stricmp( token.string, "}" ) )
		{
			BotDPrintf( S_COLOR_RED "ERROR: Invalid token %s on line %d\n", token.string, token.line );
		}
	}
	list->type = SELECTOR_NODE;
	return ( AINode_t * ) list;
}

#define D( T ) trap_Parse_AddGlobalDefine( stringify( T ) )

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
		list->maxTrees *= 2;
		AIBehaviorTree_t **trees = ( AIBehaviorTree_t ** ) BG_Alloc( sizeof( AIBehaviorTree_t * ) * list->maxTrees );
		memcpy( trees, list->trees, sizeof( AIBehaviorTree_t * ) * list->numTrees );
		BG_Free( list->trees );
		list->trees = trees;
	}

	list->trees[ list->numTrees ] = tree;
	list->numTrees++;
}

void RemoveTreeFromList( AITreeList_t *list, AIBehaviorTree_t *tree )
{
	for ( int i = 0; i < list->numTrees; i++ )
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
	list->maxTrees = 0;
	list->numTrees = 0;
}

AIBehaviorTree_t * ReadBehaviorTree( const char *name )
{
	for ( int i = 0; i < treeList.numTrees; i++ )
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

	// add condition ops
	trap_Parse_AddGlobalDefine( va( "lt %d", OP_LESSTHAN ) );
	trap_Parse_AddGlobalDefine( va( "lte %d", OP_LESSTHANEQUAL ) );
	trap_Parse_AddGlobalDefine( va( "gt %d", OP_GREATERTHAN ) );
	trap_Parse_AddGlobalDefine( va( "gte %d", OP_GREATERTHANEQUAL ) );
	trap_Parse_AddGlobalDefine( va( "eq %d", OP_EQUAL ) );
	trap_Parse_AddGlobalDefine( va( "neq %d", OP_NEQUAL ) );

	char treefilename[ MAX_QPATH ];
	Q_strncpyz( treefilename, va( "bots/%s.bt", name ), sizeof( treefilename ) );

	int handle = trap_Parse_LoadSource( treefilename );
	if ( !handle )
	{
		G_Printf( S_COLOR_RED "ERROR: Cannot load behavior tree %s: File not found\n", treefilename );
		return NULL;
	}

	pc_token_t token;
	trap_Parse_ReadToken( handle, &token );

	AIBehaviorTree_t *tree;
	tree = ( AIBehaviorTree_t * ) BG_Alloc( sizeof( AIBehaviorTree_t ) );

	Q_strncpyz( tree->name, name, sizeof( tree->name ) );

	if ( !Q_stricmp( token.string, "selector" ) )
	{
		tree->root = ReadNodeList( handle );
	}
	else if ( !Q_stricmp( token.string, "condition" ) )
	{
		tree->root = ReadConditionNode( handle );
	}
	else if ( !Q_stricmp( token.string, "action" ) )
	{
		tree->root = ReadActionNode( handle );
	}
	else
	{
		BotDPrintf( S_COLOR_RED "ERROR: Invalid token %s on line %d\n", token.string, token.line );
		BG_Free( tree );
		tree = NULL;
	}

	if ( tree )
	{
		AddTreeToList( &treeList, tree );
	}

	trap_Parse_FreeSource( handle );
	return tree;
}

void FreeNodeList( AINodeList_t *node );

void FreeConditionNode( AICondition_t *node )
{
	if ( !node->child )
	{
		return;
	}

	if ( *node->child == SELECTOR_NODE )
	{
		FreeNodeList( ( AINodeList_t * )node->child );
	}
	else if ( *node->child == CONDITION_NODE )
	{
		FreeConditionNode( ( AICondition_t * ) node->child );
	}
	else if ( *node->child == ACTION_NODE )
	{
		BG_Free( node->child );
	}
	BG_Free( node );
}

void FreeNodeList( AINodeList_t *node )
{
	for( int i = 0; i < node->numNodes; i++ )
	{
		if ( *node->list[ i ] == SELECTOR_NODE )
		{
			FreeNodeList( ( AINodeList_t * )node->list[ i ] );
		}
		else if ( *node->list[ i ] == CONDITION_NODE )
		{
			FreeConditionNode( ( AICondition_t * ) node->list[ i ] );
		}
		else if ( *node->list[ i ] == ACTION_NODE )
		{
			BG_Free( node->list[ i ] );
		}
	}
	BG_Free( node );
}

void FreeBehaviorTree( AIBehaviorTree_t *tree )
{
	if ( tree )
	{
		if ( *tree->root == SELECTOR_NODE )
		{
			FreeNodeList( ( AINodeList_t * ) tree->root );
		}
		else if ( *tree->root == CONDITION_NODE )
		{
			FreeConditionNode( ( AICondition_t * ) tree->root );
		}
		else if ( *tree->root == ACTION_NODE )
		{
			BG_Free( tree->root );
		}
		else
		{
			G_Printf( "ERROR: invalid root node for behavior tree %s\n", tree->name );
		}

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
float BotGetBaseRushScore(gentity_t *ent) {

	switch(ent->s.weapon) {
		case WP_GRENADE:	return 0.7f;
		case WP_BLASTER:	return 0.1f;
		case WP_LUCIFER_CANNON: return 1.0f;
		case WP_MACHINEGUN: return 0.5f;
		case WP_PULSE_RIFLE: return 0.7f;
		case WP_LAS_GUN: return 0.7f;
		case WP_SHOTGUN: return 0.2f;
		case WP_CHAINGUN: if(BG_InventoryContainsUpgrade(UP_BATTLESUIT,ent->client->ps.stats))
							  return 0.5f;
						  else
							  return 0.2f;
		case WP_HBUILD: return 0.0f;
		case WP_ABUILD: return 0.0f;
		case WP_ABUILD2: return 0.0f;
		case WP_ALEVEL0: return 0.0f;
		case WP_ALEVEL1: return 0.2f;
		case WP_ALEVEL2: return 0.5f;
		case WP_ALEVEL2_UPG: return 0.7f;
		case WP_ALEVEL3: return 0.8f;
		case WP_ALEVEL3_UPG: return 0.9f;
		case WP_ALEVEL4: return 1.0f;
		default: return 0.5f;
	}
}

float BotGetHealScore( gentity_t *self )
{
	float distToHealer = 0;
	float percentHealth = 0;
	float maxHealth = BG_Class( ( class_t ) self->client->ps.stats[ STAT_CLASS ] )->health;

	if ( self->client->ps.stats[ STAT_TEAM ] == TEAM_ALIENS )
	{
		if( self->botMind->closestBuildings[ BA_A_BOOSTER ].ent )
		{
			distToHealer = self->botMind->closestBuildings[ BA_A_BOOSTER ].distance;
		} 
		else if( self->botMind->closestBuildings[ BA_A_OVERMIND ].ent )
		{
			distToHealer = self->botMind->closestBuildings[ BA_A_OVERMIND ].distance;
		} 
		else if( self->botMind->closestBuildings[ BA_A_SPAWN ].ent )
		{
			distToHealer = self->botMind->closestBuildings[BA_A_SPAWN].distance;
		}
	}
	else
	{
		distToHealer = self->botMind->closestBuildings[ BA_H_MEDISTAT ].distance;
	}

	percentHealth = ( (float) self->client->ps.stats[STAT_HEALTH] )/ maxHealth;

	distToHealer = MAX( MIN( MAX_HEAL_DIST, distToHealer ), MAX_HEAL_DIST * ( 3.0f / 4.0f ) );

	if ( percentHealth == 1.0f )
	{
		return 1.0f;
	}
	return percentHealth * distToHealer / MAX_HEAL_DIST;
}

float BotGetEnemyPriority(gentity_t *self, gentity_t *ent) {
	float enemyScore;
	float distanceScore;
	distanceScore = Distance(self->s.origin,ent->s.origin);

	if(ent->client) {
		switch(ent->s.weapon) {
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
		default: enemyScore = 0.5; break;
		}
	} else {
		switch(ent->s.modelindex) {
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
		default: enemyScore = 0.5; break;

		}
	}
	return enemyScore * 1000 / distanceScore;
}
/*
=======================
Entity Querys
=======================
*/
gentity_t* BotFindBuilding(gentity_t *self, int buildingType, int range) {
	float minDistance = -1;
	gentity_t* closestBuilding = NULL;
	float newDistance;
	float rangeSquared = Square(range);
	gentity_t *target = &g_entities[MAX_CLIENTS];

	for(int  i = MAX_CLIENTS; i < level.num_entities; i++, target++ ) {
		if(!target->inuse)
			continue;
		if( target->s.eType == ET_BUILDABLE && target->s.modelindex == buildingType && (target->buildableTeam == TEAM_ALIENS || (target->powered && target->spawned)) && target->health > 0) {
			newDistance = DistanceSquared( self->s.origin, target->s.origin);
			if(newDistance > rangeSquared)
				continue;
			if( newDistance < minDistance || minDistance == -1) {
				minDistance = newDistance;
				closestBuilding = target;
			}
		}
	}
	return closestBuilding;
}

//overloading FTW
gentity_t* BotFindBuilding(gentity_t *self, int buildingType) {
	float minDistance = -1;
	gentity_t* closestBuilding = NULL;
	float newDistance;
	gentity_t *target = &g_entities[MAX_CLIENTS];

	for( int i = MAX_CLIENTS; i < level.num_entities; i++, target++ ) {
		if(!target->inuse)
			continue;
		if( target->s.eType == ET_BUILDABLE && target->s.modelindex == buildingType && (target->buildableTeam == TEAM_ALIENS || (target->powered && target->spawned)) && target->health > 0) {
			newDistance = DistanceSquared(self->s.origin,target->s.origin);
			if( newDistance < minDistance|| minDistance == -1) {
				minDistance = newDistance;
				closestBuilding = target;
			}
		}
	}
	return closestBuilding;
}

void BotFindClosestBuildings(gentity_t *self, botEntityAndDistance_t *closest) {

	memset(closest, 0, sizeof(botEntityAndDistance_t) * BA_NUM_BUILDABLES );

	for(gentity_t *testEnt = &g_entities[MAX_CLIENTS];testEnt<&g_entities[level.num_entities-1];testEnt++) {
		float newDist;
		//ignore entities that arnt in use
		if(!testEnt->inuse)
			continue;

		//ignore dead targets
		if(testEnt->health <= 0)
			continue;

		//skip non buildings
		if(testEnt->s.eType != ET_BUILDABLE)
			continue;

		//skip human buildings that are currently building or arn't powered
		if(testEnt->buildableTeam == TEAM_HUMANS && (!testEnt->powered || !testEnt->spawned))
			continue;

		newDist = Distance(self->s.origin,testEnt->s.origin);

		botEntityAndDistance_t *ent = &closest[ testEnt->s.modelindex ];
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

gentity_t* BotFindBestEnemy( gentity_t *self ) {
	float bestVisibleEnemyScore = 0;
	float bestInvisibleEnemyScore = 0;
	gentity_t *bestVisibleEnemy = NULL;
	gentity_t *bestInvisibleEnemy = NULL;

	for(gentity_t *target=g_entities;target<&g_entities[level.num_entities-1];target++) {
		float newScore;
		//ignore entities that arnt in use
		if(!target->inuse)
			continue;

		//ignore dead targets
		if(target->health <= 0)
			continue;

		//ignore buildings if we cant attack them
		if(target->s.eType == ET_BUILDABLE && (!g_bot_attackStruct.integer || self->client->ps.stats[STAT_CLASS] == PCL_ALIEN_LEVEL0))
			continue;

		//ignore neutrals
		if(BotGetTeam(target) == TEAM_NONE)
			continue;

		//ignore teamates
		if(BotGetTeam(target) == BotGetTeam(self))
			continue;

		//ignore spectators
		if(target->client) {
			if(target->client->sess.spectatorState != SPECTATOR_NOT)
				continue;
		}

		if(DistanceSquared(self->s.origin,target->s.origin) > Square(ALIENSENSE_RANGE))
			continue;

		newScore = BotGetEnemyPriority(self, target);

		if(newScore > bestVisibleEnemyScore && BotEntityIsVisible(self,target,MASK_SHOT)) {
			//store the new score and the index of the entity
			bestVisibleEnemyScore = newScore;
			bestVisibleEnemy = target;
		} else if(newScore > bestInvisibleEnemyScore && BotGetTeam(self) == TEAM_ALIENS) {
			bestInvisibleEnemyScore = newScore;
			bestInvisibleEnemy = target;
		}
	}
	if(bestVisibleEnemy || BotGetTeam(self) == TEAM_HUMANS)
		return bestVisibleEnemy;
	else
		return bestInvisibleEnemy;
}

gentity_t* BotFindClosestEnemy( gentity_t *self ) {
	gentity_t* closestEnemy = NULL;
	float minDistance = Square(ALIENSENSE_RANGE);
	for(gentity_t *target = g_entities;target<&g_entities[level.num_entities-1];target++) {
		float newDistance;
		//ignore entities that arnt in use
		if(!target->inuse)
			continue;

		//ignore dead targets
		if(target->health <= 0)
			continue;

		//ignore buildings if we cant attack them
		if(target->s.eType == ET_BUILDABLE && (!g_bot_attackStruct.integer || self->client->ps.stats[STAT_CLASS] == PCL_ALIEN_LEVEL0))
			continue;

		//ignore neutrals
		if(BotGetTeam(target) == TEAM_NONE)
			continue;

		//ignore teamates
		if(BotGetTeam(target) == BotGetTeam(self))
			continue;

		//ignore spectators
		if(target->client) {
			if(target->client->sess.spectatorState != SPECTATOR_NOT)
				continue;
		}
		newDistance = DistanceSquared(self->s.origin,target->s.origin);
		if(newDistance <= minDistance) {
			minDistance = newDistance;
			closestEnemy = target;
		}
	}
	return closestEnemy;
}

botTarget_t BotGetRushTarget(gentity_t *self) {
	botTarget_t target;
	gentity_t* rushTarget = NULL;
	if(BotGetTeam(self) == TEAM_HUMANS) {
		if(self->botMind->closestBuildings[BA_A_SPAWN].ent) {
			rushTarget = self->botMind->closestBuildings[BA_A_SPAWN].ent;
		} else if(self->botMind->closestBuildings[BA_A_OVERMIND].ent) {
			rushTarget = self->botMind->closestBuildings[BA_A_OVERMIND].ent;
		}
	} else {//team aliens
		if(self->botMind->closestBuildings[BA_H_SPAWN].ent) {
			rushTarget = self->botMind->closestBuildings[BA_H_SPAWN].ent;
		} else if(self->botMind->closestBuildings[BA_H_REACTOR].ent) {
			rushTarget = self->botMind->closestBuildings[BA_H_REACTOR].ent;
		}
	}
	BotSetTarget(&target,rushTarget,NULL);
	return target;
}

botTarget_t BotGetRetreatTarget(gentity_t *self) {
	botTarget_t target;
	gentity_t* retreatTarget = NULL;
	//FIXME, this seems like it could be done better...
	if(self->client->ps.stats[STAT_TEAM] == TEAM_HUMANS) {
		if(self->botMind->closestBuildings[BA_H_REACTOR].ent) {
			retreatTarget = self->botMind->closestBuildings[BA_H_REACTOR].ent;
		}
	} else {
		if(self->botMind->closestBuildings[BA_A_OVERMIND].ent) {
			retreatTarget = self->botMind->closestBuildings[BA_A_OVERMIND].ent;
		}
	}
	BotSetTarget(&target,retreatTarget,NULL);
	return target;
}

/*
========================
BotTarget Helpers
========================
*/

void BotSetTarget(botTarget_t *target, gentity_t *ent, vec3_t *pos) {
	if(ent) {
		target->ent = ent;
		VectorClear(target->coord);
		target->inuse = qtrue;
	} else if(pos) {
		target->ent = NULL;
		VectorCopy(*pos,target->coord);
		target->inuse = qtrue;
	} else {
		target->ent = NULL;
		VectorClear(target->coord);
		target->inuse = qfalse;
	}
}

void BotSetGoal(gentity_t *self, gentity_t *ent, vec3_t *pos) {
	BotSetTarget(&self->botMind->goal, ent, pos);
}

qboolean BotTargetInAttackRange(gentity_t *self, botTarget_t target) {
	float range, secondaryRange;
	vec3_t forward,right,up;
	vec3_t muzzle;
	vec3_t maxs, mins;
	vec3_t targetPos;
	trace_t trace;
	float width = 0, height = 0;

	AngleVectors( self->client->ps.viewangles, forward, right, up);
	CalcMuzzlePoint( self, forward, right, up , muzzle);
	BotGetTargetPos(target,targetPos);
	switch(self->client->ps.weapon) {
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
			if(self->client->ps.ammo > 0)
				secondaryRange = 900;
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
			range = FLAMER_SPEED * FLAMER_LIFETIME / 1000.0f - 20.0f;
			secondaryRange = 0;
			width = height = FLAMER_SIZE;
			// Correct muzzle so that the missile does not start in the ceiling
			VectorMA( muzzle, -7.0f, up, muzzle );

			// Correct muzzle so that the missile fires from the player's hand
			VectorMA( muzzle, 4.5f, right, muzzle);
			break;
		case WP_SHOTGUN:
			range = (50 * 8192)/SHOTGUN_SPREAD; //50 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		case WP_MACHINEGUN:
			range = (100 * 8192)/RIFLE_SPREAD; //100 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		case WP_CHAINGUN:
			range = (60 * 8192)/CHAINGUN_SPREAD; //60 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		default:
			range = 4098 * 4; //large range for guns because guns have large ranges :)
			secondaryRange = 0; //no secondary attack
		}
	VectorSet(maxs, width, width, width);
	VectorSet(mins, -width, -width, -height);

	trap_Trace(&trace,muzzle,mins,maxs,targetPos,self->s.number,MASK_SHOT);

	if(self->client->ps.stats[STAT_TEAM] != BotGetTeam(&g_entities[trace.entityNum]) && Distance(muzzle, trace.endpos) <= MAX(range, secondaryRange))
		return qtrue;
	else
		return qfalse;
}

qboolean BotTargetIsVisible( gentity_t *self, botTarget_t target, int mask ) {
	trace_t trace;
	vec3_t  muzzle, targetPos;
	vec3_t  forward, right, up;

	AngleVectors( self->client->ps.viewangles, forward, right, up );
	CalcMuzzlePoint( self, forward, right, up, muzzle );
	BotGetTargetPos(target, targetPos);

	if(!trap_InPVS(muzzle,targetPos))
		return qfalse;

	trap_TraceNoEnts( &trace, muzzle, NULL, NULL,targetPos, self->s.number, mask);

	if( trace.surfaceFlags & SURF_NOIMPACT )
		return qfalse;

	//target is in range
	if( (trace.entityNum == BotGetTargetEntityNumber(target) || trace.fraction == 1.0f) && !trace.startsolid )
		return qtrue;
	return qfalse;
}
/*
========================
Bot Aiming
========================
*/
void BotGetIdealAimLocation(gentity_t *self, botTarget_t target, vec3_t aimLocation) {
	vec3_t mins;
	//get the position of the target
	BotGetTargetPos(target, aimLocation);

	if(BotGetTargetType(target) != ET_BUILDABLE && BotTargetIsEntity(target) && BotGetTeam(target) == TEAM_HUMANS) {

		//aim at head
		aimLocation[2] += target.ent->r.maxs[2] * 0.85;

	} else if(BotGetTargetType(target) == ET_BUILDABLE || BotGetTeam(target) == TEAM_ALIENS) {
		//make lucifer cannons aim ahead based on the target's velocity
		if(self->client->ps.weapon == WP_LUCIFER_CANNON && self->botMind->botSkill.level >= 5) {
			VectorMA(aimLocation, Distance(self->s.origin, aimLocation) / LCANNON_SPEED, target.ent->s.pos.trDelta, aimLocation);
		}
	} else {
		//get rid of 'bobing' motion when aiming at waypoints by making the aimlocation the same height above ground as our viewheight
		VectorCopy(self->botMind->route[0],aimLocation);
		VectorCopy(BG_ClassConfig((class_t) self->client->ps.stats[STAT_CLASS])->mins,mins);
		aimLocation[2] +=  self->client->ps.viewheight - mins[2] - 8;
	}
}

int BotGetAimPredictionTime( gentity_t *self )
{
	return ( 10 - self->botMind->botSkill.level ) * 100 * ( ( ( float ) rand() )/ RAND_MAX );
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
	vec3_t steer;
	vec3_t viewOrigin;
	vec3_t newAim;
	vec3_t angles;

	gentity_t *enemy = self->botMind->goal.ent;
	if ( self->botMind->futureAimTime <= level.time )
	{
		int predictTime = BotGetAimPredictionTime( self );
		BotPredictPosition( self, enemy, self->botMind->futureAim, predictTime );
		self->botMind->futureAimTime = level.time + predictTime;
	}

	BG_GetClientViewOrigin( &self->client->ps, viewOrigin );
	VectorSubtract( self->botMind->futureAim, viewOrigin, desired );
	VectorNormalize( desired );
	AngleVectors( self->client->ps.viewangles, current, NULL, NULL );

	VectorSubtract( desired, current, steer );

	float length = VectorNormalize( steer );

	if ( length < 0.1 )
	{
		VectorScale( steer, length, steer );
	}
	else
	{
		VectorScale( steer, 0.1, steer );
	}
	VectorAdd( current, steer, newAim );

	vectoangles( newAim, angles );

	for ( int i = 0; i < 3; i++ )
	{
		self->botMind->cmdBuffer.angles[ i ] = ANGLE2SHORT( angles[ i ] );
	}
}

void BotAimAtLocation( gentity_t *self, vec3_t target )
{
	vec3_t aimVec, aimAngles, viewBase;

	usercmd_t *rAngles = &self->botMind->cmdBuffer;

	if( ! (self && self->client) )
		return;

	BG_GetClientViewOrigin( &self->client->ps, viewBase);
	VectorSubtract( target, viewBase, aimVec );

	vectoangles( aimVec, aimAngles );

	VectorSet(self->client->ps.delta_angles, 0.0f, 0.0f, 0.0f);

	for(int i = 0; i < 3; i++ ) {
		aimAngles[i] = ANGLE2SHORT(aimAngles[i]);
	}

	//save bandwidth
	SnapVector(aimAngles);
	rAngles->angles[0] = aimAngles[0];
	rAngles->angles[1] = aimAngles[1];
	rAngles->angles[2] = aimAngles[2];
}

void BotSlowAim( gentity_t *self, vec3_t target, float slowAmount) {
	vec3_t viewBase;
	vec3_t aimVec, forward;
	vec3_t skilledVec;
	float length;
	float slow;

	if( !(self && self->client) )
		return;
	//clamp to 0-1
	slow = Com_Clamp(0.1f,1.0,slowAmount);

	//get the point that the bot is aiming from
	BG_GetClientViewOrigin(&self->client->ps,viewBase);

	//get the Vector from the bot to the enemy (ideal aim Vector)
	VectorSubtract( target, viewBase, aimVec );
	length = VectorNormalize(aimVec);

	//take the current aim Vector
	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL);

	float cosAngle = DotProduct(forward,aimVec);
	cosAngle = (cosAngle + 1)/2;
	cosAngle = 1 - cosAngle;
	cosAngle = Com_Clamp(0.1,0.5,cosAngle);
	VectorLerp( forward, aimVec, slow * (cosAngle), skilledVec);

	//now find a point to return, this point will be aimed at
	VectorMA(viewBase, length, skilledVec,target);
}

float BotAimNegligence(gentity_t *self, botTarget_t target) {
	vec3_t forward;
	vec3_t ideal;
	vec3_t targetPos;
	vec3_t viewPos;
	float angle;
	AngleVectors(self->client->ps.viewangles,forward, NULL, NULL);
	BG_GetClientViewOrigin(&self->client->ps,viewPos);
	BotGetIdealAimLocation(self, target, targetPos);
	VectorSubtract(targetPos,viewPos,ideal);
	VectorNormalize(ideal);
	angle = DotProduct(ideal,forward);
	angle = RAD2DEG(acos(angle));
	return angle;
}
/*
========================
Bot Team Querys
========================
*/

int FindBots(int *botEntityNumbers, int maxBots, team_t team) {
	gentity_t *testEntity;
	int numBots = 0;
	memset(botEntityNumbers,0,sizeof(int)*maxBots);
	for(int i=0;i<MAX_CLIENTS;i++) {
		testEntity = &g_entities[i];
		if(testEntity->r.svFlags & SVF_BOT) {
			if(testEntity->client->pers.teamSelection == team && numBots < maxBots) {
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

	if(self->client->pers.teamSelection == TEAM_HUMANS)
		sq = &level.humanSpawnQueue;

	else if(self->client->pers.teamSelection == TEAM_ALIENS)
		sq = &level.alienSpawnQueue;
	else
		return qfalse;

	i = sq->front;

	if(G_GetSpawnQueueLength(sq)) {
		do {
			if( !(g_entities[sq->clients[ i ]].r.svFlags & SVF_BOT)) {
				if(i < sq->front)
					lastPlayerPos = i + MAX_CLIENTS - sq->front;
				else
					lastPlayerPos = i - sq->front;
			}

			if(sq->clients[ i ] == self->s.number) {
				if(i < sq->front)
					botPos = i + MAX_CLIENTS - sq->front;
				else
					botPos = i - sq->front;
			}

			i = QUEUE_PLUS1(i);
		} while(i != QUEUE_PLUS1(sq->back));
	}

	if(botPos < lastPlayerPos)
		return qtrue;
	else
		return qfalse;
}

qboolean BotTeamateHasWeapon(gentity_t *self, int weapon) {
	int botNumbers[MAX_CLIENTS];
	int i;
	int numBots = FindBots(botNumbers, MAX_CLIENTS, ( team_t ) self->client->ps.stats[STAT_TEAM]);

	for(i=0;i<numBots;i++) {
		gentity_t *bot = &g_entities[botNumbers[i]];
		if(bot == self)
			continue;
		if(BG_InventoryContainsWeapon(weapon, bot->client->ps.stats))
			return qtrue;
	}
	return qfalse;
}

/*
========================
Boolean Functions for determining actions
========================
*/

botTarget_t BotGetRoamTarget(gentity_t *self) {
	botTarget_t target;
	int numTiles = 0;
	const dtNavMesh *navMesh = self->botMind->navQuery->getAttachedNavMesh();
	numTiles = navMesh->getMaxTiles();
	const dtMeshTile *tile;
	vec3_t targetPos;

	//pick a random tile
	do {
		tile = navMesh->getTile(rand() % numTiles);
	} while(!tile->header->vertCount);

	//pick a random vertex in the tile
	int vertStart = 3 * (rand() % tile->header->vertCount);

	//convert from recast to quake3
	float *v = &tile->verts[vertStart];
	VectorCopy(v, targetPos);
	recast2quake(targetPos);

	BotSetTarget(&target, NULL, &targetPos);
	return target;
}

/*
========================
Misc Bot Stuff
========================
*/
void BotFireWeapon(weaponMode_t mode, usercmd_t *botCmdBuffer) {
	if(mode == WPM_PRIMARY) {
		usercmdPressButton(botCmdBuffer->buttons, BUTTON_ATTACK);
	} else if(mode == WPM_SECONDARY) {
		usercmdPressButton(botCmdBuffer->buttons, BUTTON_ATTACK2);
	} else if(mode == WPM_TERTIARY) {
		usercmdPressButton(botCmdBuffer->buttons, BUTTON_USE_HOLDABLE);
	}
}
void BotClassMovement(gentity_t *self, qboolean inAttackRange) {
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	switch(self->client->ps.stats[STAT_CLASS]) {
	case PCL_ALIEN_LEVEL0:
		BotDodge(self, botCmdBuffer);
		break;
	case PCL_ALIEN_LEVEL1:
	case PCL_ALIEN_LEVEL1_UPG:
		if(BotTargetIsPlayer(self->botMind->goal) && (self->botMind->goal.ent->client->ps.stats[STAT_STATE] & SS_GRABBED) && inAttackRange) {
			if(self->botMind->botSkill.level == 10) {
				botCmdBuffer->forwardmove = 0;
				botCmdBuffer->upmove = 0;
				BotDodge(self, botCmdBuffer); //only move if skill == 10 because otherwise we wont aim fast enough to not lose grab
			} else {
				botCmdBuffer->forwardmove = 0;
				botCmdBuffer->rightmove = 0;
				botCmdBuffer->upmove = 0;
			}
		} else {
			BotDodge(self, botCmdBuffer);
		}
		break;
	case PCL_ALIEN_LEVEL2:
	case PCL_ALIEN_LEVEL2_UPG:
		if(self->botMind->numCorners == 1) {
			if(self->client->time1000 % 300 == 0)
				botCmdBuffer->upmove = 127;
			BotDodge(self, botCmdBuffer);
		}
		break;
	case PCL_ALIEN_LEVEL3:
		break;
	case PCL_ALIEN_LEVEL3_UPG:
		if(BotGetTargetType(self->botMind->goal) == ET_BUILDABLE && self->client->ps.ammo > 0
			&& inAttackRange) {
				//dont move when sniping buildings
				botCmdBuffer->forwardmove = 0;
				botCmdBuffer->rightmove = 0;
				botCmdBuffer->upmove = 0;
		}
		break;
	case PCL_ALIEN_LEVEL4:
		//use rush to approach faster
		if(!inAttackRange)
			BotFireWeapon(WPM_SECONDARY, botCmdBuffer);
		break;
	default: break;
	}
}
void BotFireWeaponAI(gentity_t *self) {
	float distance;
	vec3_t targetPos;
	vec3_t forward,right,up;
	vec3_t muzzle;
	trace_t trace;
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	AngleVectors(self->client->ps.viewangles,forward,right,up);
	CalcMuzzlePoint(self,forward,right,up,muzzle);
	BotGetIdealAimLocation(self,self->botMind->goal,targetPos);

	trap_Trace(&trace,muzzle,NULL,NULL,targetPos,-1,MASK_SHOT);
	distance = Distance(muzzle, trace.endpos);
	switch(self->s.weapon) {
	case WP_ABUILD:
		if(distance <= ABUILDER_CLAW_RANGE)
			BotFireWeapon(WPM_SECONDARY, botCmdBuffer);
		else
			usercmdPressButton(botCmdBuffer->buttons, BUTTON_GESTURE); //make cute granger sounds to ward off the would be attackers
		break;
	case WP_ABUILD2:
		if(distance <= ABUILDER_CLAW_RANGE)
			BotFireWeapon(WPM_SECONDARY, botCmdBuffer); //swipe
		else
			BotFireWeapon(WPM_TERTIARY, botCmdBuffer); //blob launcher
		break;
	case WP_ALEVEL0:
		break; //auto hit
	case WP_ALEVEL1:
		BotFireWeapon(WPM_PRIMARY, botCmdBuffer); //basi swipe
		break;
	case WP_ALEVEL1_UPG:
		if(distance <= LEVEL1_CLAW_U_RANGE)
			BotFireWeapon(WPM_PRIMARY, botCmdBuffer); //basi swipe
		/*
		else
		BotFireWeapn(WPM_SECONDARY,botCmdBuffer); //basi poisen
		*/
		break;
	case WP_ALEVEL2:
		BotFireWeapon(WPM_PRIMARY, botCmdBuffer); //mara swipe
		break;
	case WP_ALEVEL2_UPG:
		if(distance <= LEVEL2_CLAW_U_RANGE)
			BotFireWeapon(WPM_PRIMARY,botCmdBuffer); //mara swipe
		else
			BotFireWeapon(WPM_SECONDARY, botCmdBuffer); //mara lightning
		break;
	case WP_ALEVEL3:
		if(distance > LEVEL3_CLAW_RANGE && self->client->ps.stats[ STAT_MISC ] < LEVEL3_POUNCE_TIME) {
			botCmdBuffer->angles[PITCH] = ANGLE2SHORT(-CalcPounceAimPitch(self,self->botMind->goal)); //compute and apply correct aim pitch to hit target
			BotFireWeapon(WPM_SECONDARY, botCmdBuffer); //goon pounce
		} else
			BotFireWeapon(WPM_PRIMARY, botCmdBuffer); //goon chomp
		break;
	case WP_ALEVEL3_UPG:
		if(self->client->ps.ammo > 0 && distance > LEVEL3_CLAW_UPG_RANGE) {
			botCmdBuffer->angles[PITCH] = ANGLE2SHORT(-CalcBarbAimPitch(self, self->botMind->goal)); //compute and apply correct aim pitch to hit target
			BotFireWeapon(WPM_TERTIARY, botCmdBuffer); //goon barb
		} else if(distance > LEVEL3_CLAW_UPG_RANGE && self->client->ps.stats[ STAT_MISC ] < LEVEL3_POUNCE_TIME_UPG) {
			botCmdBuffer->angles[PITCH] = ANGLE2SHORT(-CalcPounceAimPitch(self, self->botMind->goal)); //compute and apply correct aim pitch to hit target
			BotFireWeapon(WPM_SECONDARY,botCmdBuffer); //goon pounce
		} else
			BotFireWeapon(WPM_PRIMARY, botCmdBuffer); //goon chomp
		break;
	case WP_ALEVEL4:
		if (distance > LEVEL4_CLAW_RANGE && self->client->ps.stats[STAT_MISC] < LEVEL4_TRAMPLE_CHARGE_MAX)
			BotFireWeapon(WPM_SECONDARY,botCmdBuffer); //rant charge
		else
			BotFireWeapon(WPM_PRIMARY,botCmdBuffer); //rant swipe
		break;
	case WP_LUCIFER_CANNON:
		if( self->client->ps.stats[STAT_MISC] < LCANNON_CHARGE_TIME_MAX * Com_Clamp(0.5,1,random()))
			BotFireWeapon(WPM_PRIMARY, botCmdBuffer);
		break;
	default: BotFireWeapon(WPM_PRIMARY,botCmdBuffer);
	}
}

extern "C" void G_BotLoadBuildLayout() {
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
	if( !g_bot_buildLayout.string[0] || !Q_stricmp(g_bot_buildLayout.string, "*BUILTIN*" ) )
		return;

	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
	trap_Print("Attempting to Load Bot Build Layout File...\n");
	len = trap_FS_FOpenFile( va( "layouts/%s/%s.dat", map, g_bot_buildLayout.string ),
		&f, FS_READ );
	if( len < 0 )
	{
		G_Printf( "ERROR: layout %s could not be opened\n", g_bot_buildLayout.string );
		return;
	}
	layoutHead = layout = (char *)BG_Alloc( len + 1 );
	trap_FS_Read( layout, len, f );
	layout[ len ] = '\0';
	trap_FS_FCloseFile( f );
	while( *layout )
	{
		if( i >= sizeof( line ) - 1 )
		{
			G_Printf( S_COLOR_RED "ERROR: line overflow in %s before \"%s\"\n",
				va( "layouts/%s/%s.dat", map, g_bot_buildLayout.string ), line );
			break;
		}
		line[ i++ ] = *layout;
		line[ i ] = '\0';
		if( *layout == '\n' )
		{
			i = 0;
			sscanf( line, "%s %f %f %f %f %f %f %f %f %f %f %f %f\n",
				buildName,
				&origin[ 0 ], &origin[ 1 ], &origin[ 2 ],
				&angles[ 0 ], &angles[ 1 ], &angles[ 2 ],
				&origin2[ 0 ], &origin2[ 1 ], &origin2[ 2 ],
				&angles2[ 0 ], &angles2[ 1 ], &angles2[ 2 ] );

			buildable = BG_BuildableByName( buildName )->number;
			if( buildable <= BA_NONE || buildable >= BA_NUM_BUILDABLES )
				G_Printf( S_COLOR_YELLOW "WARNING: bad buildable name (%s) in layout."
				" skipping\n", buildName );
			else if(level.botBuildLayout.numBuildings == MAX_BOT_BUILDINGS) {
				G_Printf( S_COLOR_YELLOW "WARNING: reached max buildings for bot layout (%d)."
					" skipping\n",MAX_BOT_BUILDINGS );
			} else if(BG_Buildable(buildable)->team == TEAM_HUMANS && level.botBuildLayout.numBuildings < MAX_BOT_BUILDINGS) {
				level.botBuildLayout.buildings[level.botBuildLayout.numBuildings].type = buildable;
				VectorCopy(origin, level.botBuildLayout.buildings[level.botBuildLayout.numBuildings].origin);
				VectorCopy(origin2, level.botBuildLayout.buildings[level.botBuildLayout.numBuildings].normal);
				level.botBuildLayout.numBuildings++;
			}

		}
		layout++;
	}
	BG_Free( layoutHead );
}

void BotSetSkillLevel(gentity_t *self, int skill) {
	self->botMind->botSkill.level = skill;
	//different aim for different teams
	if(self->botMind->botTeam == TEAM_HUMANS) {
		self->botMind->botSkill.aimSlowness = (float) skill / 10;
		self->botMind->botSkill.aimShake = (int) (10 - skill);
	} else {
		self->botMind->botSkill.aimSlowness = (float) skill / 10;
		self->botMind->botSkill.aimShake = (int) (10 - skill);
	}
}

/*
=======================
Bot management functions
=======================
*/
static struct {
	int count;
	struct {
		char *name;
		qboolean inUse;
	} name[MAX_CLIENTS];
} botNames[NUM_TEAMS];

static void G_BotListTeamNames(gentity_t *ent, const char *heading, team_t team, const char *marker)
{
	int i;

	if (botNames[team].count)
	{
		ADMP(heading);
		ADMBP_begin();

		for (i = 0; i < botNames[team].count; ++i)
		{
			ADMBP( va( "  %s^7 %s\n", botNames[team].name[i].inUse ? marker : " ", botNames[team].name[i].name ) );
		}

		ADMBP_end();
	}
}

extern "C" void G_BotListNames(gentity_t *ent)
{
	G_BotListTeamNames(ent, QQ( N_("^3Alien bot names:\n") ), TEAM_ALIENS, "^1*");
	G_BotListTeamNames(ent, QQ( N_("^3Human bot names:\n") ), TEAM_HUMANS, "^5*");
}

extern "C" qboolean G_BotClearNames(void)
{
	int i;

	for (i = 0; i < botNames[TEAM_ALIENS].count; ++i)
		if (botNames[TEAM_ALIENS].name[i].inUse)
			return qfalse;

	for (i = 0; i < botNames[TEAM_HUMANS].count; ++i)
		if (botNames[TEAM_HUMANS].name[i].inUse)
			return qfalse;

	for (i = 0; i < botNames[TEAM_ALIENS].count; ++i)
		BG_Free( botNames[TEAM_ALIENS].name[i].name );

	for (i = 0; i < botNames[TEAM_HUMANS].count; ++i)
		BG_Free( botNames[TEAM_HUMANS].name[i].name );

	botNames[TEAM_ALIENS].count = 0;
	botNames[TEAM_HUMANS].count = 0;

	return qtrue;
}

extern "C" int G_BotAddNames(team_t team, int arg, int last)
{
	int  i = botNames[team].count;
	int  added = 0;
	char name[MAX_NAME_LENGTH];

	while (arg < last && i < MAX_CLIENTS)
	{
		int j, t;

		trap_Argv(arg++, name, sizeof(name));

		// name already in the list? (quick check, including colours & invalid)
		for (t = 1; t < NUM_TEAMS; ++t)
			for (j = 0; j < botNames[t].count; ++j)
				if (!Q_stricmp(botNames[t].name[j].name, name))
					goto next;

		botNames[team].name[i].name = (char *)BG_Alloc(strlen(name) + 1);
		strcpy(botNames[team].name[i].name, name);

		botNames[team].count = ++i;
		++added;

		next:;
	}

	return added;
}

static char *G_BotSelectName(team_t team)
{
	unsigned int i, choice;

	if (botNames[team].count < 1)
		return NULL;

	choice = rand() % botNames[team].count;

	for (i = 0; i < botNames[team].count; ++i)
	{
		if (!botNames[team].name[choice].inUse)
			return botNames[team].name[choice].name;
		choice = (choice + 1) % botNames[team].count;
	}

	return NULL;
}

static void G_BotNameUsed(team_t team, const char *name, qboolean inUse)
{
	unsigned int i;

	for (i = 0; i < botNames[team].count; ++i)
	{
		if (!Q_stricmp(name, botNames[team].name[i].name))
		{
			botNames[team].name[i].inUse = inUse;
			return;
		}
	}
}

extern "C" void G_BotSetDefaults( int clientNum, team_t team, int skill, const char* behavior )
{
	botMemory_t *botMind;

	botMind = g_entities[clientNum].botMind = &g_botMind[clientNum];

	botMind->enemyLastSeen = 0;
	botMind->botTeam = team;
	botMind->pathCorridor = &pathCorridor[clientNum];
	memset( botMind->runningNodes, 0, sizeof( botMind->runningNodes ) );
	botMind->numRunningNodes = 0;
	botMind->currentNode = NULL;
	botMind->numCorners = 0;
	
	botMind->behaviorTree = ReadBehaviorTree( behavior );

	if ( !botMind->behaviorTree )
	{
		G_Printf( "Problem when loading behavior tree %s, trying default\n", behavior );
		botMind->behaviorTree = ReadBehaviorTree( behavior );
	}

	BotSetSkillLevel(&g_entities[clientNum], skill);

	g_entities[clientNum].r.svFlags |= SVF_BOT;

	if ( team != TEAM_NONE )
	{
		level.clients[clientNum].sess.restartTeam = team;
	}
}

qboolean G_BotAdd( char *name, team_t team, int skill, const char *behavior ) {
	int clientNum;
	char userinfo[MAX_INFO_STRING];
	const char* s = 0;
	gentity_t *bot;
	bool autoname = false;

	if(!navMeshLoaded) {
		trap_Print("No Navigation Mesh file is available for this map\n");
		return qfalse;
	}

	// find what clientNum to use for bot
	clientNum = trap_BotAllocateClient(0);

	if(clientNum < 0) {
		trap_Print("no more slots for bot\n");
		return qfalse;
	}
	bot = &g_entities[ clientNum ];
	bot->r.svFlags |= SVF_BOT;
	bot->inuse = qtrue;

	if(!Q_stricmp(name, "*")) {
		name = G_BotSelectName(team);
		autoname = name != NULL;
	}

	//default bot data
	G_BotSetDefaults( clientNum, team, skill, behavior );

	// register user information
	userinfo[0] = '\0';
	Info_SetValueForKey( userinfo, "name", name ? name : "" ); // allow defaulting
	Info_SetValueForKey( userinfo, "rate", "25000" );
	Info_SetValueForKey( userinfo, "snaps", "20" );
	if (autoname)
		Info_SetValueForKey( userinfo, "autoname", name );

	//so we can connect if server is password protected
	if(g_needpass.integer == 1)
		Info_SetValueForKey( userinfo, "password", g_password.string);

	trap_SetUserinfo( clientNum, userinfo );

	// have it connect to the game as a normal client
	if((s = ClientBotConnect(clientNum, qtrue, team)))
	{
		// won't let us join
		trap_Print(s);
		return qfalse;
	}

	if (autoname)
		G_BotNameUsed(team, name, qtrue);

	ClientBegin( clientNum );
	return qtrue;
}

void G_BotDel( int clientNum ) {
	gentity_t *bot = &g_entities[clientNum];
	char userinfo[MAX_INFO_STRING];
	const char *autoname;

	if( !( bot->r.svFlags & SVF_BOT ) || !bot->botMind) {
		trap_Print( va("'^7%s^7' is not a bot\n", bot->client->pers.netname) );
		return;
	}

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	autoname = Info_ValueForKey( userinfo, "autoname" );
	if (autoname && *autoname)
		G_BotNameUsed(BotGetTeam(bot), autoname, qfalse);

	trap_SendServerCommand( -1, va( "print_tr %s %s", QQ( N_("$1$^7 disconnected\n") ),
		                        Quote( bot->client->pers.netname ) ) );
	trap_DropClient(clientNum, "disconnected");
}

void G_BotFreeBehaviorTrees( void )
{
	for ( int i = 0; i < treeList.numTrees; i++ )
	{
		AIBehaviorTree_t *tree = treeList.trees[ i ];
		FreeBehaviorTree( tree );
	}
	FreeTreeList( &treeList );
}

extern "C" void G_BotDelAll(void)
{
	int i;

	for(i=0;i<MAX_CLIENTS;i++) {
		if(g_entities[i].r.svFlags & SVF_BOT && level.clients[i].pers.connected != CON_DISCONNECTED)
			G_BotDel(i);
	}

	for (i = 0; i < botNames[TEAM_ALIENS].count; ++i)
		botNames[TEAM_ALIENS].name[i].inUse = qfalse;

	for (i = 0; i < botNames[TEAM_HUMANS].count; ++i)
		botNames[TEAM_HUMANS].name[i].inUse = qfalse;

	G_BotFreeBehaviorTrees();
}

AINodeStatus_t BotActionFight( gentity_t *self, AIActionNode_t *node )
{
	team_t myTeam = ( team_t ) self->client->ps.stats[ STAT_TEAM ];

	if ( self->botMind->currentNode != node )
	{
		if ( !BotChangeTarget( self, self->botMind->bestEnemy.ent, NULL ) )
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

	if ( BotGetTeam( self->botMind->goal ) == myTeam || BotGetTeam( self->botMind->goal ) == TEAM_NONE )
	{
		return STATUS_FAILURE;
	}

	// the enemy has died
	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_SUCCESS;
	}

	if ( self->botMind->goal.ent->client && self->botMind->goal.ent->client->sess.spectatorState != SPECTATOR_NOT )
	{
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
	if( myTeam == TEAM_ALIENS && DistanceToGoalSquared( self ) <= Square( ALIENSENSE_RANGE ) ) 
	{
		self->botMind->enemyLastSeen = level.time;
	}

	if ( !BotTargetIsVisible( self, self->botMind->goal, MASK_SHOT ) )
	{
		botTarget_t proposedTarget;
		BotSetTarget( &proposedTarget, self->botMind->bestEnemy.ent, NULL );
		
		//we can see another enemy (not our target)
		if(self->botMind->bestEnemy.ent && self->botMind->goal.ent != self->botMind->bestEnemy.ent && BotPathIsWalkable(self,proposedTarget))
		{	//change targets
			return STATUS_FAILURE;
		}
		else if ( level.time - self->botMind->enemyLastSeen > BOT_ENEMY_CHASETIME )
		{
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
		
		if ( ( inAttackRange && myTeam == TEAM_HUMANS ) || self->botMind->numCorners == 1 )
		{

			BotAimAtEnemy( self );
		
			//update the path corridor
			UpdatePathCorridor(self);

			self->botMind->cmdBuffer.forwardmove = 127;

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
					self->botMind->cmdBuffer.forwardmove = -127;
				}
				else if(DistanceToGoalSquared(self) <= Square(MIN_HUMAN_DANCE_DIST)) { //we wont hit this if skill < 5
					//we will be moving toward enemy, strafe too
					//the result: we go around the enemy
					self->botMind->cmdBuffer.rightmove = BotGetStrafeDirection();

					//also try to dodge if high enough level
					//all the conditions are there so we dont stop moving forward when we cant dodge anyway
					if(self->client->ps.weapon != WP_PAIN_SAW && self->botMind->botSkill.level >= 7
						&& self->client->ps.stats[STAT_STAMINA] > STAMINA_SLOW_LEVEL + STAMINA_DODGE_TAKE
						&& !(self->client->ps.pm_flags & ( PMF_TIME_LAND | PMF_CHARGE ))
						&& self->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{
						usercmdPressButton(self->botMind->cmdBuffer.buttons, BUTTON_DODGE);
						self->botMind->cmdBuffer.forwardmove = 0;
					}
				} 
				else if ( DistanceToGoalSquared( self ) >= Square( MAX_HUMAN_DANCE_DIST ) && self->client->ps.weapon != WP_PAIN_SAW ) 
				{
					//we should be >= MAX_HUMAN_DANCE_DIST from the enemy
					//dont go forward
					self->botMind->cmdBuffer.forwardmove = 0;

					//strafe randomly
					self->botMind->cmdBuffer.rightmove = BotGetStrafeDirection();
				}

				if ( inAttackRange && BotGetTargetType( self->botMind->goal ) == ET_BUILDABLE )
				{
					self->botMind->cmdBuffer.forwardmove = 0;
					self->botMind->cmdBuffer.rightmove = 0;
					self->botMind->cmdBuffer.upmove = 0;
				}

				if(self->client->ps.stats[STAT_STAMINA] > STAMINA_SLOW_LEVEL && self->botMind->botSkill.level >= 5) 
				{
					usercmdPressButton(self->botMind->cmdBuffer.buttons, BUTTON_SPRINT);
				} 
				else
				{
					//dont sprint or dodge, we are about to slow
					usercmdReleaseButton(self->botMind->cmdBuffer.buttons, BUTTON_SPRINT);
					usercmdReleaseButton(self->botMind->cmdBuffer.buttons, BUTTON_DODGE);
				}
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

AINodeStatus_t BotActionFlee( gentity_t *self, AIActionNode_t *node )
{
	if ( node != self->botMind->currentNode )
	{
		if ( !BotChangeTarget( self, BotGetRetreatTarget( self ) ) )
		{
			return STATUS_FAILURE;
		}
	}

	if( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	if( DistanceToGoalSquared( self ) < Square( 70 ) )
	{
		return STATUS_SUCCESS;
	}
	else
	{
		BotMoveToGoal(self);
	}

	return STATUS_RUNNING;
}

AINodeStatus_t BotActionRoam( gentity_t *self, AIActionNode_t *node )
{
	// we are just starting to roam, get a target location
	if ( node != self->botMind->currentNode )
	{
		botTarget_t target = BotGetRoamTarget( self );
		if ( !BotChangeTarget( self, target ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( DistanceToGoalSquared(self) < Square( 70 ) )
	{
		return STATUS_SUCCESS;
	}
	else
	{
		BotMoveToGoal( self );
	}
	return STATUS_RUNNING;
}

botTarget_t BotGetMoveToTarget( gentity_t *self, AIMoveTo_t *node )
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

float RadiusFromBounds2D(vec3_t mins, vec3_t maxs) {
	float rad1 = sqrt(Square(mins[0]) + Square(mins[1]));
	float rad2 = sqrt(Square(maxs[0]) + Square(maxs[1]));
	if(rad1 > rad2)
		return rad1;
	else
		return rad2;
}

float BotGetGoalRadius( gentity_t *self ) {
	if(BotTargetIsEntity(self->botMind->goal)) {
		botTarget_t *t = &self->botMind->goal;
		if(t->ent->s.modelindex == BA_H_MEDISTAT || t->ent->s.modelindex == BA_A_BOOSTER) {
			return self->r.maxs[0] + t->ent->r.maxs[0];
		} else {
			return RadiusFromBounds2D(t->ent->r.mins, t->ent->r.maxs) + RadiusFromBounds2D(self->r.mins, self->r.maxs);
		}
	} else {
		return RadiusFromBounds2D(self->r.mins, self->r.maxs);
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

AINodeStatus_t BotActionMoveTo( gentity_t *self, AIMoveTo_t *node )
{
	if ( node != self->botMind->currentNode )
	{
		if ( !BotChangeTarget( self, BotGetMoveToTarget( self, node ) ) )
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

	float radius;
	if ( node->range == -1 )
	{
		radius = BotGetGoalRadius( self );
	}
	else
	{
		radius = node->range;
	}
	
	if ( DistanceToGoal2DSquared( self ) <= Square( radius ) && self->botMind->numCorners == 1 )
	{
		return STATUS_SUCCESS;
	}

	return STATUS_RUNNING;
}

AINodeStatus_t BotActionRush( gentity_t *self, AIActionNode_t *node )
{
	if ( self->botMind->currentNode != node )
	{
		if ( !BotChangeTarget( self, BotGetRushTarget( self ) ) )
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

AINodeStatus_t BotEvaluateNode( gentity_t *self, AINode_t *node );

qboolean NodeIsRunning( gentity_t *self, AINode_t *node )
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

AINodeStatus_t BotEvaluateNodeList( gentity_t *self, AINodeList_t *nodeList )
{
	int i;

	switch ( nodeList->selector )
	{
		case SELECTOR_PRIORITY:
			for( i = 0; i < nodeList->numNodes; i++ )
			{
				AINodeStatus_t status = BotEvaluateNode( self, nodeList->list[ i ] );
				if ( status == STATUS_FAILURE )
				{
					continue;
				}
				return status;
			}
			return STATUS_FAILURE;
		case SELECTOR_SEQUENCE:
			i = 0;

			// find a previously running node and start there
			for ( i = nodeList->numNodes - 1; i > 0; i-- )
			{
				if ( NodeIsRunning( self, nodeList->list[ i ] ) )
				{
					break;
				}
			}

			for( ; i < nodeList->numNodes; i++ )
			{
				AINodeStatus_t status = BotEvaluateNode( self, nodeList->list[ i ] );
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
		case SELECTOR_SELECTOR:
			i = 0;

			// find a previously running node and start there
			for ( i = nodeList->numNodes - 1; i > 0; i-- )
			{
				if ( NodeIsRunning( self, nodeList->list[ i ] ) )
				{
					break;
				}
			}

			for( ; i < nodeList->numNodes; i++ )
			{
				AINodeStatus_t status = BotEvaluateNode( self, nodeList->list[ i ] );
				if( status == STATUS_FAILURE )
				{
					continue;
				}
				return status;
			}
			return STATUS_FAILURE;
		case SELECTOR_CONCURRENT:
			i = 0;
			{
				int numSTATUS_FAILURE = 0;
				for( ; i < nodeList->numNodes; i++ )
				{
					AINodeStatus_t status = BotEvaluateNode( self, nodeList->list[ i ] );

					if ( status == STATUS_FAILURE )
					{
						numSTATUS_FAILURE++;

						if ( numSTATUS_FAILURE < nodeList->maxFail )
						{
							continue;
						}
					}

					return status;
				}
			}
			return STATUS_FAILURE;
		default:
			return STATUS_FAILURE;
	}
}

#define BotCondition( leftValue, rightValue, op, success ) \
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
AINodeStatus_t BotEvaluateCondition( gentity_t *self, AICondition_t *node )
{
	qboolean success = qfalse;

	switch( node->info )
	{
		case CON_BUILDING:
			{
				gentity_t *building = NULL;
				building = self->botMind->closestBuildings[ node->param1.value.i ].ent;
				BotBoolCondition( building, node->op, success );
			}
			break;
		case CON_WEAPON:
			BotBoolCondition( BG_InventoryContainsWeapon( node->param1.value.i, self->client->ps.stats ), node->op, success );
			break;
		case CON_DAMAGEDBUILDING:
			BotBoolCondition( self->botMind->closestDamagedBuilding.ent, node->op, success );
			break;
		case CON_CVAR:
			BotBoolCondition( ( ( ( vmCvar_t * ) node->param1.value.ptr )->integer ), node->op, success );
			break;
		case CON_CURWEAPON:
			BotCondition( self->client->ps.weapon, node->param1.value.i, node->op, success );
			break;
		case CON_TEAM:
			BotCondition( self->client->ps.stats[ STAT_TEAM ], node->param1.value.i, node->op, success );
			break;
		case CON_ENEMY:
			if ( level.time - self->botMind->timeFoundEnemy <= BOT_REACTION_TIME )
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

			if ( node->op == OP_NOT )
			{
				success = ( qboolean ) ( ( int ) !success );
			}
			break;
		case CON_UPGRADE:
			{
				BotBoolCondition( BG_InventoryContainsUpgrade( node->param1.value.i, self->client->ps.stats ), node->op, success );

				if ( node->op == OP_NOT )
				{
					if ( BG_UpgradeIsActive( node->param1.value.i, self->client->ps.stats ) )
					{
						success = qfalse;
					}
				}
				else if ( node->op == OP_BOOL )
				{
					if ( BG_UpgradeIsActive( node->param1.value.i, self->client->ps.stats ) )
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

				BotCondition( ( health / maxHealth ), node->param1.value.f, node->op, success );
			}
			break;
		case CON_AMMO:
			BotCondition( PercentAmmoRemaining( BG_PrimaryWeapon( self->client->ps.stats ), &self->client->ps ), node->param1.value.f, node->op, success );
			break;
		case CON_TEAM_WEAPON:
			BotBoolCondition( BotTeamateHasWeapon( self, node->param1.value.i ), node->op, success );
			break;
		case CON_DISTANCE:
			{
				float distance = 0;

				if ( node->param1.value.i < BA_NUM_BUILDABLES )
				{
					distance = self->botMind->closestBuildings[ node->param1.value.i ].distance;
				}
				else if ( node->param1.value.i == E_GOAL )
				{
					distance = DistanceToGoal( self );
				}
				else if ( node->param1.value.i == E_ENEMY )
				{
					distance = self->botMind->bestEnemy.distance;
				}
				else if ( node->param1.value.i == E_DAMAGEDBUILDING )
				{
					distance = self->botMind->closestDamagedBuilding.distance;
				}

				BotCondition( distance, node->param2.value.i, node->op, success );
			}
			break;
		case CON_BASERUSH:
			{
				float score = BotGetBaseRushScore( self );

				BotCondition( score, node->param1.value.f, node->op, success );
			}
			break;
		case CON_HEAL:
			{
				float score = BotGetHealScore( self );
				BotCondition( score, node->param1.value.f, node->op, success );
			}
			break;
	}

	if ( success )
	{
		if ( node->child )
		{
			return BotEvaluateNode( self, node->child );
		}
		else
		{
			return STATUS_SUCCESS;
		}
	}

	return STATUS_FAILURE;
}

AINodeStatus_t BotEvaluateAction( gentity_t *self, AIActionNode_t *node )
{
	AINodeStatus_t status = STATUS_FAILURE;

	switch ( node->action )
	{
		case ACTION_ROAM:
			status = BotActionRoam( self, node );
			break;
		case ACTION_BUY:
			status = BotActionBuy( self, ( AIBuy_t * ) node );
			break;
		case ACTION_FIGHT:
			status = BotActionFight( self, node );
			break;
		case ACTION_FLEE:
			status = BotActionFlee( self, node );
			break;
		case ACTION_HEAL:
			if ( self->client->ps.stats[ STAT_TEAM ] == TEAM_HUMANS )
			{
				status = BotActionHealH( self, node );
			}
			else
			{
				status = BotActionHealA( self, node );
			}
			break;
		case ACTION_REPAIR:
			status = BotActionRepair( self, node );
			break;
		case ACTION_EVOLVE:
			status = BotActionEvolve( self, node );
			break;
		case ACTION_MOVETO:
			status = BotActionMoveTo( self, ( AIMoveTo_t * ) node );
			break;
		case ACTION_RUSH:
			status = BotActionRush( self, node );
			break;
		default:
			status = STATUS_FAILURE;
			break;
	}

	if ( status == STATUS_RUNNING )
	{
		self->botMind->currentNode = node;
	}

	if ( self->botMind->currentNode == node && status != STATUS_RUNNING )
	{
		self->botMind->currentNode = NULL;
	}

	return status;
}

AINodeStatus_t BotEvaluateNode( gentity_t *self, AINode_t *node )
{
	AINodeStatus_t status = STATUS_FAILURE;
	switch ( *node )
	{
		case SELECTOR_NODE:
			status = BotEvaluateNodeList( self, ( AINodeList_t * ) node );
			break;
		case ACTION_NODE:
			status = BotEvaluateAction( self, ( AIActionNode_t * )node );
			break;
		case CONDITION_NODE:
			status = BotEvaluateCondition( self, ( AICondition_t * ) node );
			break;
	}

	// reset running information on node success
	if ( NodeIsRunning( self, node ) && status == STATUS_SUCCESS )
	{
		memset( self->botMind->runningNodes, 0, sizeof( self->botMind->runningNodes ) );
		self->botMind->numRunningNodes = 0;
	}

	// store running information
	if ( status == STATUS_RUNNING )
	{
		if ( self->botMind->numRunningNodes == MAX_NODE_DEPTH )
		{
			G_Printf( "ERROR: MAX_NODE_DEPTH exceeded\n" );
			return status;
		}

		// clear out previous running list when we hit a running leaf node
		if ( *node == ACTION_NODE )
		{
			memset( self->botMind->runningNodes, 0, sizeof( self->botMind->runningNodes ) );
			self->botMind->numRunningNodes = 0;
		}

		int i = self->botMind->numRunningNodes;
		self->botMind->numRunningNodes++;
		self->botMind->runningNodes[ i ] = node;
	}

	return status;
}

/*
=======================
Bot Thinks
=======================
*/
extern "C" void G_BotThink( gentity_t *self) {
	char buf[MAX_STRING_CHARS];
	self->botMind->cmdBuffer = self->client->pers.cmd;
	usercmd_t &botCmdBuffer = self->botMind->cmdBuffer;
	vec3_t     nudge;

	//reset command buffer
	usercmdClearButtons(botCmdBuffer.buttons);

	// for nudges, e.g. spawn blocking
	nudge[0] = botCmdBuffer.doubleTap ? botCmdBuffer.forwardmove : 0;
	nudge[1] = botCmdBuffer.doubleTap ? botCmdBuffer.rightmove : 0;
	nudge[2] = botCmdBuffer.doubleTap ? botCmdBuffer.upmove : 0;

	botCmdBuffer.forwardmove = 0;
	botCmdBuffer.rightmove = 0;
	botCmdBuffer.upmove = 0;
	botCmdBuffer.doubleTap = 0;

	//acknowledge recieved server commands
	//MUST be done
	while(trap_BotGetServerCommand(self->client->ps.clientNum, buf, sizeof(buf)));

	//self->client->pers.cmd.forwardmove = 127;
	//return;
	//update closest structs
	gentity_t* bestEnemy = BotFindBestEnemy(self);

	botTarget_t target;
	BotSetTarget(&target, bestEnemy, NULL);

	//if we do not already have an enemy, and we found an enemy, update the time that we found an enemy
	if(!self->botMind->bestEnemy.ent && bestEnemy && level.time - self->botMind->enemyLastSeen > BOT_ENEMY_CHASETIME && BotAimNegligence(self,target) > 45) {
		self->botMind->timeFoundEnemy = level.time;
	}

	self->botMind->bestEnemy.ent = bestEnemy;
	self->botMind->bestEnemy.distance = (!bestEnemy) ? 0 : Distance(self->s.origin,bestEnemy->s.origin);

	BotFindClosestBuildings(self, self->botMind->closestBuildings);

	gentity_t* bestDamaged = BotFindDamagedFriendlyStructure(self);
	self->botMind->closestDamagedBuilding.ent = bestDamaged;
	self->botMind->closestDamagedBuilding.distance = (!bestDamaged) ? 0 :  Distance(self->s.origin,bestDamaged->s.origin);

	//use medkit when hp is low
	if(self->health < BOT_USEMEDKIT_HP && BG_InventoryContainsUpgrade(UP_MEDKIT,self->client->ps.stats))
		BG_ActivateUpgrade(UP_MEDKIT,self->client->ps.stats);

	//infinite funds cvar
	if(g_bot_infinite_funds.integer)
		G_AddCreditToClient(self->client, HUMAN_MAX_CREDITS, qtrue);

	//hacky ping fix
	self->client->ps.ping = rand() % 50 + 50;
	
	if ( !self->botMind->behaviorTree )
	{
		G_Printf( "ERROR: NULL behavior tree\n" );
		return;
	}

	BotEvaluateNode( self, self->botMind->behaviorTree->root );

	// if we were nudged...
	VectorAdd( self->client->ps.velocity, nudge, self->client->ps.velocity );

	self->client->pers.cmd = botCmdBuffer;
}

extern "C" void G_BotSpectatorThink( gentity_t *self ) {
	char buf[MAX_STRING_CHARS];
	//hacky ping fix
	self->client->ps.ping = rand() % 50 + 50;

	//acknowledge recieved console messages
	//MUST be done
	while(trap_BotGetServerCommand(self->client->ps.clientNum, buf, sizeof(buf)));

	if( self->client->ps.pm_flags & PMF_QUEUED) {
		//we're queued to spawn, all good
		//check for humans in the spawn que
		{
			spawnQueue_t *sq;
			if(self->client->pers.teamSelection == TEAM_HUMANS)
				sq = &level.humanSpawnQueue;
			else if(self->client->pers.teamSelection == TEAM_ALIENS)
				sq = &level.alienSpawnQueue;
			else
				sq = NULL;

			if( sq && PlayersBehindBotInSpawnQueue( self )) {
				G_RemoveFromSpawnQueue( sq, self->s.number );
				G_PushSpawnQueue( sq, self->s.number );
			}
		}
		return;
	}

	//reset stuff
	BotSetGoal(self, NULL, NULL);
	self->botMind->bestEnemy.ent = NULL;
	self->botMind->timeFoundEnemy = 0;
	self->botMind->enemyLastSeen = 0;

	if( self->client->sess.restartTeam == TEAM_NONE ) {
		int teamnum = self->client->pers.teamSelection;
		int clientNum = self->client->ps.clientNum;

		if( teamnum == TEAM_HUMANS ) {
			self->client->pers.classSelection = PCL_HUMAN;
			self->client->ps.stats[STAT_CLASS] = PCL_HUMAN;
			self->botMind->navQuery = navQuerys[PCL_HUMAN];
			self->botMind->navFilter = &navFilters[PCL_HUMAN];
			//we want to spawn with rifle unless it is disabled or we need to build
			if(g_bot_rifle.integer)
				self->client->pers.humanItemSelection = WP_MACHINEGUN;
			else
				self->client->pers.humanItemSelection = WP_HBUILD;

			G_PushSpawnQueue( &level.humanSpawnQueue, clientNum );
		} else if( teamnum == TEAM_ALIENS) {
			self->client->pers.classSelection = PCL_ALIEN_LEVEL0;
			self->client->ps.stats[STAT_CLASS] = PCL_ALIEN_LEVEL0;
			self->botMind->navQuery = navQuerys[PCL_ALIEN_LEVEL0];
			self->botMind->navFilter = &navFilters[PCL_ALIEN_LEVEL0];
			G_PushSpawnQueue( &level.alienSpawnQueue, clientNum );
		}
	}
	else
	{
		G_ChangeTeam( self, self->client->sess.restartTeam );
		self->client->sess.restartTeam = TEAM_NONE;
	}
}

extern "C" void G_BotIntermissionThink( gclient_t *client )
{
	client->readyToExit = qtrue;
}

extern "C" void G_BotInit( void )
{
	G_BotNavInit( );
	if ( treeList.maxTrees == 0 )
	{
		InitTreeList( &treeList );
	}
}

extern "C" void G_BotCleanup( int restart )
{
	if ( !restart )
	{
		int i;

		for ( i = 0; i < MAX_CLIENTS; ++i)
			if ( g_entities[i].r.svFlags & SVF_BOT && level.clients[i].pers.connected != CON_DISCONNECTED )
				G_BotDel(i);

		G_BotClearNames();
	}
	G_BotFreeBehaviorTrees();
	G_BotNavCleanup();
}
