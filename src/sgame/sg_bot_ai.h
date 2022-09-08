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
#include "sg_local.h"
#include "sg_bot_local.h"

#ifndef BOT_AI_HEADER_
#define BOT_AI_HEADER_

// integer constants given to the behavior tree to use as parameters
// values E_A_SPAWN to E_H_REACTOR are meant to have the same
// integer values as the corresponding enum in buildable_t
// TODO: get rid of dependence on buildable_t
enum AIEntity_t
{
	E_NONE,
	E_A_SPAWN,
	E_A_OVERMIND,
	E_A_BARRICADE,
	E_A_ACIDTUBE,
	E_A_TRAPPER,
	E_A_BOOSTER,
	E_A_HIVE,
	E_A_LEECH,
	E_A_SPIKER,
	E_H_SPAWN,
	E_H_MGTURRET,
	E_H_ROCKETPOD,
	E_H_ARMOURY,
	E_H_MEDISTAT,
	E_H_DRILL,
	E_H_REACTOR,
	E_NUM_BUILDABLES,
	E_GOAL = E_NUM_BUILDABLES,
	E_ENEMY,
	E_DAMAGEDBUILDING,
	E_FRIENDLYBUILDING, // closest alive friendly building
	E_ENEMYBUILDING,    // closest alive enemy building
	E_SELF
};

// all behavior tree nodes must return one of
// these status when finished
enum AINodeStatus_t
{
	STATUS_FAILURE = 0,
	STATUS_SUCCESS,
	STATUS_RUNNING
};

// behavior tree node types
enum AINode_t
{
	SELECTOR_NODE,
	ACTION_NODE,
	CONDITION_NODE,
	BEHAVIOR_NODE,
	DECORATOR_NODE
};

struct AIGenericNode_t;
using AINodeRunner = AINodeStatus_t (*)( gentity_t *self, AIGenericNode_t *node );

// all behavior tree nodes must conform to this interface
struct AIGenericNode_t
{
	AINode_t type;
	AINodeRunner run;
};

#define MAX_NODE_LIST 20
struct AINodeList_t
{
	AINode_t type;
	AINodeRunner run;
	AIGenericNode_t *list[ MAX_NODE_LIST ];
	int numNodes;
};

struct AIBehaviorTree_t
{
	AINode_t     type;
	AINodeRunner run;
	char name[ MAX_QPATH ];
	AIGenericNode_t *root;
};

// operations used in condition nodes
// ordered according to precedence
// lower values == higher precedence
enum AIOpType_t
{
	OP_NOT,
	OP_LESSTHAN,
	OP_LESSTHANEQUAL,
	OP_GREATERTHAN,
	OP_GREATERTHANEQUAL,
	OP_EQUAL,
	OP_NEQUAL,
	OP_AND,
	OP_OR,
	OP_NONE
};

// types of expressions in condition nodes
enum AIExpType_t
{
	EX_OP,
	EX_VALUE,
	EX_FUNC
};

enum AIValueType_t
{
	VALUE_FLOAT,
	VALUE_INT,
	VALUE_STRING
};

struct AIValue_t
{
	AIExpType_t             expType;
	AIValueType_t           valType;

	union
	{
		float floatValue;
		int   intValue;
		char  *stringValue;
	} l;
};

using AIFunc = AIValue_t (*)( gentity_t *self, const AIValue_t *params );

struct AIValueFunc_t
{
	AIExpType_t   expType;
	AIFunc        func;
	AIValue_t     *params;
	int           nparams;
};

// all ops must conform to this interface
struct AIOp_t
{
	AIExpType_t expType;
	AIOpType_t  opType;
};

struct AIBinaryOp_t
{
	AIExpType_t expType;
	AIOpType_t  opType;
	AIExpType_t *exp1;
	AIExpType_t *exp2;
};

struct AIUnaryOp_t
{
	AIExpType_t expType;
	AIOpType_t  opType;
	AIExpType_t *exp;
};

struct AIConditionNode_t
{
	AINode_t        type;
	AINodeRunner    run;
	AIGenericNode_t *child;
	AIExpType_t     *exp;
};

struct AIDecoratorNode_t
{
	AINode_t        type;
	AINodeRunner    run;
	AIGenericNode_t *child;
	AIValue_t       *params;
	int             nparams;
	int             data[ MAX_CLIENTS ]; // bot specific data
};

struct AIActionNode_t
{
	AINode_t     type;
	AINodeRunner run;
	AIValue_t    *params;
	int          nparams;
};

bool isBinaryOp( AIOpType_t op );
bool isUnaryOp( AIOpType_t op );

AIValue_t AIBoxFloat( float f );
AIValue_t AIBoxInt( int i );
AIValue_t AIBoxString( char *s );

float       AIUnBoxFloat( AIValue_t v );
int         AIUnBoxInt( AIValue_t v );
double      AIUnBoxDouble( AIValue_t v );
const char *AIUnBoxString( AIValue_t v );

void AIDestroyValue( AIValue_t v );

botEntityAndDistance_t AIEntityToGentity( gentity_t *self, AIEntity_t e );

// standard behavior tree control-flow nodes
AINodeStatus_t BotEvaluateNode( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotConditionNode( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotFallbackNode( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotSelectorNode( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotSequenceNode( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotConcurrentNode( gentity_t *self, AIGenericNode_t *node );

// decorator nodes
AINodeStatus_t BotDecoratorInvert( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotDecoratorTimer( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotDecoratorReturn( gentity_t *self, AIGenericNode_t *node );

// included behavior trees
AINodeStatus_t BotBehaviorNode( gentity_t *self, AIGenericNode_t *node );

// action nodes
AINodeStatus_t BotActionChangeGoal( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionMoveToGoal( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionFireWeapon( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionAimAtGoal( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionAlternateStrafe( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionStrafeDodge( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionMoveInDir( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionClassDodge( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionTeleport( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionActivateUpgrade( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionDeactivateUpgrade( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionEvolveTo( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionSay( gentity_t *self, AIGenericNode_t *node );

AINodeStatus_t BotActionFight( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionGetInSafety( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionBuy( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionRepair( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionEvolve ( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionHeal( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionFlee( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionRoam( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionRoamInRadius( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionMoveTo( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionRush( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionSuicide( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionJump( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionResetStuckTime( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionGesture( gentity_t *self, AIGenericNode_t* );

#endif
