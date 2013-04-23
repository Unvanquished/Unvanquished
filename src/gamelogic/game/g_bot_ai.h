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
#ifndef __BOT_AI_HEADER
#define __BOT_AI_HEADER
#include "g_local.h"

typedef enum
{
	E_NONE,
	E_A_SPAWN,
	E_A_OVERMIND,
	E_A_BARRICADE,
	E_A_ACIDTUBE,
	E_A_TRAPPER,
	E_A_BOOSTER,
	E_A_HIVE,
	E_H_SPAWN,
	E_H_MGTURRET,
	E_H_TESLAGEN,
	E_H_ARMOURY,
	E_H_DCC,
	E_H_MEDISTAT,
	E_H_REACTOR,
	E_H_REPEATER,
	E_GOAL,
	E_ENEMY,
	E_DAMAGEDBUILDING
} AIEntity_t;

typedef enum
{
	STATUS_FAILURE = 0,
	STATUS_SUCCESS,
	STATUS_RUNNING
} AINodeStatus_t;

typedef enum
{
  SELECTOR_NODE,
  ACTION_NODE,
  CONDITION_NODE
} AINode_t;

typedef struct
{
	char name[ MAX_QPATH ];
	AINode_t *root;
} AIBehaviorTree_t;

struct AIGenericNode_s;

typedef AINodeStatus_t ( *AINodeRunner )( gentity_t *, struct AIGenericNode_s * );

// all nodes must conform to this interface
typedef struct AIGenericNode_s
{
	AINode_t type;
	AINodeRunner run;
} AIGenericNode_t;

#define MAX_NODE_LIST 20
typedef struct
{
	AINode_t type;
	AINodeRunner run;
	AIGenericNode_t *list[ MAX_NODE_LIST ];
	int numNodes;
	int maxFail;
} AINodeList_t;

typedef struct
{
	AINode_t type;
	AINodeRunner run;
	weapon_t weapon;
	upgrade_t upgrades[ 3 ];
	int numUpgrades;
} AIBuyNode_t;

typedef struct
{
	AINode_t type;
	AINodeRunner run;
	AIEntity_t ent;
	float range;
} AIMoveToNode_t;

typedef enum
{
	CON_BUILDING,
	CON_DAMAGEDBUILDING,
	CON_CVAR,
	CON_WEAPON,
	CON_UPGRADE,
	CON_CURWEAPON,
	CON_TEAM,
	CON_ENEMY,
	CON_HEALTH,
	CON_AMMO,
	CON_TEAM_WEAPON,
	CON_DISTANCE,
	CON_BASERUSH,
	CON_HEAL
} AIConditionInfo_t;

typedef enum
{
	VALUE_FLOAT,
	VALUE_INT,
	VALUE_PTR
} AIConditionValue_t;

typedef struct
{
	AIConditionValue_t type;
	union
	{
		float f;
		int   i;
		void *ptr;
	} value;
} AIConditionParameter_t;

typedef enum
{
	OP_NONE,
	OP_LESSTHAN,
	OP_LESSTHANEQUAL,
	OP_GREATERTHAN,
	OP_GREATERTHANEQUAL,
	OP_EQUAL,
	OP_NEQUAL,
	OP_NOT,
	OP_BOOL
} AIConditionOperator_t;

typedef struct
{
	AINode_t type;
	AINodeRunner run;
	AIGenericNode_t *child;
	AIConditionOperator_t op;
	AIConditionInfo_t info;
	AIConditionParameter_t param1;
	AIConditionParameter_t param2;
} AIConditionNode_t;

typedef struct
{
	AIBehaviorTree_t **trees;
	int numTrees;
	int maxTrees;
} AITreeList_t;

void              FreeBehaviorTree( AIBehaviorTree_t *tree );
AIBehaviorTree_t *ReadBehaviorTree( const char *name, AITreeList_t *list );
void              FreeTreeList( AITreeList_t *list );
void              InitTreeList( AITreeList_t *list );

// standard behavior tree control-flow nodes
AINodeStatus_t BotEvaluateNode( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotConditionNode( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotSelectorNode( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotSequenceNode( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotPriorityNode( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotParallelNode( gentity_t *self, AIGenericNode_t *node );

// action nodes
AINodeStatus_t BotActionFight( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionBuy( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionRepair( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionEvolve ( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionHealH( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionHealA( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionHeal( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionFlee( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionRoam( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionMoveTo( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionRush( gentity_t *self, AIGenericNode_t *node );
#endif