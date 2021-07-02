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

#ifndef __BOT_AI_HEADER
#define __BOT_AI_HEADER

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

enum AIValueType_t
{
	VALUE_FLOAT,
	VALUE_INT,
	VALUE_STRING
};

class AIValue_t;
class AIExpression_t
{
public:
	virtual AIValue_t eval( gentity_t *self ) const = 0;
};

class AIValue_t : public AIExpression_t // FIXME: turn this into AIValue?
{
public:
	AIValue_t(bool);
	AIValue_t(int);
	AIValue_t(float);
	AIValue_t(const char *);
	AIValue_t(AIValue_t const&);
	AIValue_t(AIValue_t&&);
	AIValue_t& operator=(AIValue_t const&) = delete;
	AIValue_t& operator=(AIValue_t&&) = delete;
	~AIValue_t();
	explicit operator bool() const;
	explicit operator int() const;
	explicit operator float() const;
	explicit operator double() const;
	explicit operator const char *() const;
	AIValue_t eval( gentity_t *self ) const { return *this; };

private:
	AIValueType_t valType;
	union
	{
		float floatValue;
		int   intValue;
		char  *stringValue;
	} l;
};

using AIFunc = AIValue_t (*)( gentity_t *self, const std::vector<AIValue_t> &params );

class AIValueFunc_t : public AIExpression_t
{
public:
	AIValueFunc_t(AIValueType_t retType_, AIFunc func_, std::vector<AIValue_t> params_)
		: retType(retType_), func(func_), params(std::move(params_)) {};
	AIValue_t eval( gentity_t *self ) const {
		return func( self, params );
	}
private:
	AIValueType_t retType;
	AIFunc        func;
	std::vector<AIValue_t> params;
};

class AIOp_t : public AIExpression_t
{
public:
	AIOp_t(AIOpType_t t) : opType(t) {}
protected:
	AIOpType_t opType;
};

class AIBinaryOp_t : public AIOp_t
{
public:
	AIBinaryOp_t(AIOpType_t type, std::unique_ptr<AIExpression_t> exp1_, std::unique_ptr<AIExpression_t> exp2_)
		: AIOp_t(type), exp1(std::move(exp1_)), exp2(std::move(exp2_)) {};
	AIValue_t eval( gentity_t *self ) const;
private:
	std::unique_ptr<AIExpression_t> exp1;
	std::unique_ptr<AIExpression_t> exp2;
};

class AIUnaryOp_t : public AIOp_t
{
public:
	AIUnaryOp_t(AIOpType_t type, std::unique_ptr<AIExpression_t> exp_)
		: AIOp_t(type), exp(std::move(exp_)) {}
	AIValue_t eval( gentity_t *self ) const;
private:
	std::unique_ptr<AIExpression_t> exp;
};

bool isBinaryOp( AIOpType_t op );
bool isUnaryOp( AIOpType_t op );

// behavior tree node types
enum AINode_t
{
	SELECTOR_NODE,
	ACTION_NODE,
	CONDITION_NODE,
	BEHAVIOR_NODE,
	DECORATOR_NODE
};

struct AIGenericNode;
typedef AINodeStatus_t ( *AINodeRunner )( gentity_t *, AIGenericNode * );

// all behavior tree nodes must conform to this interface
struct AIGenericNode
{
	AINode_t type;
	AINodeRunner run;
};

struct pc_token_list;
struct AINodeList : public AIGenericNode
{
	std::vector<std::shared_ptr<AIGenericNode>> list;
	AINodeList(AINodeRunner, pc_token_list **);
};

struct AIBehaviorTree : public AIGenericNode
{
	AIBehaviorTree( std::string filename );
	std::string name;
	std::shared_ptr<AIGenericNode> root;
};
using AITreeList = std::vector<std::shared_ptr<AIBehaviorTree>>;

struct AIConditionNode : public AIGenericNode
{
	AIConditionNode(pc_token_list **);
	std::shared_ptr<AIGenericNode> child;
	std::unique_ptr<AIExpression_t> exp;
};

struct AIDecoratorNode : public AIGenericNode
{
	AIDecoratorNode( pc_token_list **list );
	std::shared_ptr<AIGenericNode> child;
	std::vector<AIValue_t> params;
	int        data[ MAX_CLIENTS ]; // bot specific data
};

struct AIActionNode : public AIGenericNode
{
	AIActionNode( pc_token_list **list );
	std::vector<AIValue_t> params;
};

botEntityAndDistance_t AIEntityToGentity( gentity_t *self, AIEntity_t e );

// standard behavior tree control-flow nodes
AINodeStatus_t BotEvaluateNode( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotConditionNode( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotSelectorNode( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotSequenceNode( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotConcurrentNode( gentity_t *self, AIGenericNode *node );

// decorator nodes
AINodeStatus_t BotDecoratorTimer( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotDecoratorReturn( gentity_t *self, AIGenericNode *node );

// included behavior trees
AINodeStatus_t BotBehaviorNode( gentity_t *self, AIGenericNode *node );

// action nodes
AINodeStatus_t BotActionChangeGoal( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionMoveToGoal( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionFireWeapon( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionAimAtGoal( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionAlternateStrafe( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionStrafeDodge( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionMoveInDir( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionClassDodge( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionTeleport( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionActivateUpgrade( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionDeactivateUpgrade( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionEvolveTo( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionSay( gentity_t *self, AIGenericNode *node );

AINodeStatus_t BotActionFight( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionBuy( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionRepair( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionEvolve ( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionHealH( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionHealA( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionHeal( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionFlee( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionRoam( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionRoamInRadius( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionMoveTo( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionRush( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionSuicide( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionJump( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionResetStuckTime( gentity_t *self, AIGenericNode *node );
AINodeStatus_t BotActionGesture( gentity_t *self, AIGenericNode* );
#endif
