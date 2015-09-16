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
#include "sg_bot_ai.h"
#include "sg_bot_util.h"
#include "CBSE.h"

/*
======================
g_bot_ai.c

This file contains the implementation of the different behavior tree nodes

On each frame, the behavior tree for each bot is evaluated starting from the root node
Each node returns either STATUS_SUCCESS, STATUS_RUNNING, or STATUS_FAILURE depending on their logic
The return values are used in various sequences and selectors to change the execution of the tree
======================
*/

bool isBinaryOp( AIOpType_t op )
{
	switch ( op )
	{
		case OP_GREATERTHAN:
		case OP_GREATERTHANEQUAL:
		case OP_LESSTHAN:
		case OP_LESSTHANEQUAL:
		case OP_EQUAL:
		case OP_NEQUAL:
		case OP_AND:
		case OP_OR:
			return true;
		default: return false;
	}
}

bool isUnaryOp( AIOpType_t op )
{
	return op == OP_NOT;
}

// functions for using values specified in the bt
AIValue_t AIBoxFloat( float f )
{
	AIValue_t t;
	t.expType = EX_VALUE;
	t.valType = VALUE_FLOAT;
	t.l.floatValue = f;
	return t;
}

AIValue_t AIBoxInt( int i )
{
	AIValue_t t;
	t.expType = EX_VALUE;
	t.valType = VALUE_INT;
	t.l.intValue = i;
	return t;
}

AIValue_t AIBoxString( char *s )
{
	AIValue_t t;
	t.expType = EX_VALUE;
	t.valType = VALUE_STRING;
	t.l.stringValue = BG_strdup( s );
	return t;
}

float AIUnBoxFloat( AIValue_t v )
{
	switch ( v.valType )
	{
		case VALUE_FLOAT:
			return v.l.floatValue;
		case VALUE_INT:
			return ( float ) v.l.intValue;
		default:
			return 0.0f;
	}
}

int AIUnBoxInt( AIValue_t v )
{
	switch ( v.valType )
	{
		case VALUE_FLOAT:
			return ( int ) v.l.floatValue;
		case VALUE_INT:
			return v.l.intValue;
		default:
			return 0;
	}
}

const char *AIUnBoxString( AIValue_t v )
{
	static char empty[] = "";

	switch ( v.valType )
	{
		case VALUE_FLOAT:
			return va( "%f", v.l.floatValue );
		case VALUE_INT:
			return va( "%d", v.l.intValue );
		case VALUE_STRING:
			return v.l.stringValue;
		default:
			return empty;
	}
}

double AIUnBoxDouble( AIValue_t v )
{
	switch ( v.valType )
	{
		case VALUE_FLOAT:
			return ( double ) v.l.floatValue;
		case VALUE_INT:
			return ( double ) v.l.intValue;
		default:
			return 0.0;
	}
}

void AIDestroyValue( AIValue_t v )
{
	switch ( v.valType )
	{
		case VALUE_STRING:
			BG_Free( v.l.stringValue );
			break;

		default:
			break;
	}
}

botEntityAndDistance_t AIEntityToGentity( gentity_t *self, AIEntity_t e )
{
	static const botEntityAndDistance_t nullEntity = { nullptr, HUGE_QFLT };
	botEntityAndDistance_t              ret = nullEntity;

	if ( e > E_NONE && e < E_NUM_BUILDABLES )
	{
		return self->botMind->closestBuildings[ e ];
	}
	else if ( e == E_ENEMY )
	{
		return self->botMind->bestEnemy;
	}
	else if ( e == E_DAMAGEDBUILDING )
	{
		return self->botMind->closestDamagedBuilding;
	}
	else if ( e == E_GOAL )
	{
		ret.ent = self->botMind->goal.ent;
		ret.distance = DistanceToGoal( self );
		return ret;
	}
	else if ( e == E_SELF )
	{
		ret.ent = self;
		ret.distance = 0;
		return ret;
	}
	
	return ret;
}

static bool NodeIsRunning( gentity_t *self, AIGenericNode_t *node )
{
	int i;
	for ( i = 0; i < self->botMind->numRunningNodes; i++ )
	{
		if ( self->botMind->runningNodes[ i ] == node )
		{
			return true;
		}
	}
	return false;
}

/*
======================
Sequences and Selectors

A sequence or selector contains a list of child nodes which are evaluated
based on a combination of the child node return values and the internal logic
of the sequence or selector

A selector evaluates its child nodes like an if ( ) else if ( ) loop
It starts at the first child node, and if the node did not fail, it returns its status
if the node failed, it evaluates the next child node in the list
A selector will fail if all of its child nodes fail

A sequence evaluates its child nodes like a series of statements
It starts at the first previously running child node, and if the node does not succeed, it returns its status
If the node succeeded, it evaluates the next child node in the list
A sequence will succeed if all of its child nodes succeed

A concurrent node will always evaluate all of its child nodes unless one fails
if one fails, the concurrent node will stop executing nodes and return failure
A concurrent node succeeds if none of its child nodes fail
======================
*/
AINodeStatus_t BotSelectorNode( gentity_t *self, AIGenericNode_t *node )
{
	AINodeList_t *selector = ( AINodeList_t * ) node;
	int i = 0;

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

AINodeStatus_t BotConcurrentNode( gentity_t *self, AIGenericNode_t *node )
{
	AINodeList_t *con = ( AINodeList_t * ) node;
	int i = 0;

	for ( ; i < con->numNodes; i++ )
	{
		AINodeStatus_t status = BotEvaluateNode( self, con->list[ i ] );

		if ( status == STATUS_FAILURE )
		{
			return STATUS_FAILURE;
		}
	}
	return STATUS_SUCCESS;
}

/*
======================
Decorators

Decorators are used to add functionality to the child node
======================
*/
AINodeStatus_t BotDecoratorTimer( gentity_t *self, AIGenericNode_t *node )
{
	AIDecoratorNode_t *dec = ( AIDecoratorNode_t * ) node;

	if ( level.time > dec->data[ self->s.number ] )
	{
		AINodeStatus_t status = BotEvaluateNode( self, dec->child );

		if ( status == STATUS_FAILURE )
		{
			dec->data[ self->s.number ] = level.time + AIUnBoxInt( dec->params[ 0 ] );
		}

		return status;
	}

	return STATUS_FAILURE;
}

AINodeStatus_t BotDecoratorReturn( gentity_t *self, AIGenericNode_t *node )
{
	AIDecoratorNode_t *dec = ( AIDecoratorNode_t * ) node;
	
	AINodeStatus_t status = ( AINodeStatus_t ) AIUnBoxInt( dec->params[ 0 ] );

	BotEvaluateNode( self, dec->child );
	return status;
}

bool EvalConditionExpression( gentity_t *self, AIExpType_t *exp );

double EvalFunc( gentity_t *self, AIExpType_t *exp )
{
	AIValueFunc_t *v = ( AIValueFunc_t * ) exp;
	AIValue_t vt = v->func( self, v->params );
	double vd = AIUnBoxDouble( vt );
	AIDestroyValue( vt );
	return vd;
}

// using double because it has enough precision to exactly represent both a float and an int
double EvalValue( gentity_t *self, AIExpType_t *exp )
{
	AIValue_t *v = ( AIValue_t * ) exp;

	if ( *exp == EX_FUNC )
	{
		return EvalFunc( self, exp );
	}

	if ( *exp != EX_VALUE )
	{
		return ( double ) EvalConditionExpression( self, exp );
	}

	return AIUnBoxDouble( *v );
}

bool EvaluateBinaryOp( gentity_t *self, AIExpType_t *exp )
{
	AIBinaryOp_t *o = ( AIBinaryOp_t * ) exp;

	switch ( o->opType )
	{
		case OP_LESSTHAN:
			return EvalValue( self, o->exp1 ) < EvalValue( self, o->exp2 );
		case OP_LESSTHANEQUAL:
			return EvalValue( self, o->exp1 ) <= EvalValue( self, o->exp2 );
		case OP_GREATERTHAN:
			return EvalValue( self, o->exp1 ) > EvalValue( self, o->exp2 );
		case OP_GREATERTHANEQUAL:
			return EvalValue( self, o->exp1 ) >= EvalValue( self, o->exp2 );
		case OP_EQUAL:
			return EvalValue( self, o->exp1 ) == EvalValue( self, o->exp2 );
		case OP_NEQUAL:
			return EvalValue( self, o->exp1 ) != EvalValue( self, o->exp2 );
		case OP_AND:
			return EvalConditionExpression( self, o->exp1 ) && EvalConditionExpression( self, o->exp2 );
		case OP_OR:
			return EvalConditionExpression( self, o->exp1 ) || EvalConditionExpression( self, o->exp2 );
		default:
			return false;
	}
}

bool EvaluateUnaryOp( gentity_t *self, AIExpType_t *exp )
{
	AIUnaryOp_t *o = ( AIUnaryOp_t * ) exp;
	return !EvalConditionExpression( self, o->exp );
}

bool EvalConditionExpression( gentity_t *self, AIExpType_t *exp )
{
	if ( *exp == EX_OP )
	{
		AIOp_t *op = ( AIOp_t * ) exp;

		if ( isBinaryOp( op->opType ) )
		{
			return EvaluateBinaryOp( self, exp );
		}
		else if ( isUnaryOp( op->opType ) )
		{
			return EvaluateUnaryOp( self, exp );
		}
	}
	else if ( *exp  == EX_VALUE )
	{
		return EvalValue( self, exp ) != 0.0;
	}
	else if ( *exp == EX_FUNC )
	{
		return EvalFunc( self, exp ) != 0.0;
	}

	return false;
}

/*
======================
BotConditionNode

Runs the child node if the condition expression is true
If there is no child node, returns success if the conditon expression is true
returns failure otherwise
======================
*/
AINodeStatus_t BotConditionNode( gentity_t *self, AIGenericNode_t *node )
{
	bool success = false;

	AIConditionNode_t *con = ( AIConditionNode_t * ) node;

	success = EvalConditionExpression( self, con->exp );
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

/*
======================
BotBehaviorNode

Runs the root node of a behavior tree
A behavior tree may contain multiple other behavior trees which are run in this way
======================
*/
AINodeStatus_t BotBehaviorNode( gentity_t *self, AIGenericNode_t *node )
{
	AIBehaviorTree_t *tree = ( AIBehaviorTree_t * ) node;
	return BotEvaluateNode( self, tree->root );
}

/*
======================
BotEvaluateNode

Generic node running routine that properly handles 
running information for sequences and selectors
This should always be used instead of the node->run function pointer
======================
*/
AINodeStatus_t BotEvaluateNode( gentity_t *self, AIGenericNode_t *node )
{
	AINodeStatus_t status = node->run( self, node );

	// reset the current node if it finishes
	// we do this so we can re-pathfind on the next entrance
	if ( ( status == STATUS_SUCCESS || status == STATUS_FAILURE ) && self->botMind->currentNode == node )
	{
		self->botMind->currentNode = nullptr;
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

		if ( !NodeIsRunning( self, node ) )
		{
			self->botMind->runningNodes[ self->botMind->numRunningNodes++ ] = node;
		}
	}

	return status;
}

/*
======================
Action Nodes

Action nodes are always the leaves of the behavior tree
They make the bot do a specific thing while leaving decision making
to the rest of the behavior tree
======================
*/

AINodeStatus_t BotActionFireWeapon( gentity_t *self, AIGenericNode_t* ) 
{
	if ( WeaponIsEmpty( BG_GetPlayerWeapon( &self->client->ps ), &self->client->ps ) && self->client->pers.team == TEAM_HUMANS )
	{
		G_ForceWeaponChange( self, WP_BLASTER );
	}

	if ( BG_GetPlayerWeapon( &self->client->ps ) == WP_HBUILD )
	{
		G_ForceWeaponChange( self, WP_BLASTER );
	}

	BotFireWeaponAI( self );
	return STATUS_SUCCESS;
}

AINodeStatus_t BotActionActivateUpgrade( gentity_t *self, AIGenericNode_t *node )
{
	AIActionNode_t *action = ( AIActionNode_t * ) node;
	upgrade_t u = ( upgrade_t ) AIUnBoxInt( action->params[ 0 ] );

	if ( !BG_UpgradeIsActive( u, self->client->ps.stats ) &&
		BG_InventoryContainsUpgrade( u, self->client->ps.stats ) )
	{
		BG_ActivateUpgrade( u, self->client->ps.stats );
	}
	return STATUS_SUCCESS;
}

AINodeStatus_t BotActionDeactivateUpgrade( gentity_t *self, AIGenericNode_t *node )
{
	AIActionNode_t *action = ( AIActionNode_t * ) node;
	upgrade_t u = ( upgrade_t ) AIUnBoxInt( action->params[ 0 ] );

	if ( BG_UpgradeIsActive( u, self->client->ps.stats ) &&
		BG_InventoryContainsUpgrade( u, self->client->ps.stats ) )
	{
		BG_DeactivateUpgrade( u, self->client->ps.stats );
	}
	return STATUS_SUCCESS;
}

AINodeStatus_t BotActionAimAtGoal( gentity_t *self, AIGenericNode_t* )
{
	if ( BotGetTargetTeam( self->botMind->goal ) != self->client->pers.team )
	{
		BotAimAtEnemy( self );
	}
	else
	{
		vec3_t pos;
		BotGetTargetPos( self->botMind->goal, pos );
		BotSlowAim( self, pos, 0.5 );
		BotAimAtLocation( self, pos );
	}
	return STATUS_SUCCESS;
}

AINodeStatus_t BotActionMoveToGoal( gentity_t *self, AIGenericNode_t* )
{
	BotMoveToGoal( self );
	return STATUS_RUNNING;
}

AINodeStatus_t BotActionMoveInDir( gentity_t *self, AIGenericNode_t *node )
{
	AIActionNode_t *a = ( AIActionNode_t * ) node;
	int dir = AIUnBoxInt( a->params[ 0 ] );
	if ( a->nparams == 2 )
	{
		dir |= AIUnBoxInt( a->params[ 1 ] );
	}
	BotMoveInDir( self, dir );
	return STATUS_SUCCESS;
}

AINodeStatus_t BotActionStandStill( gentity_t *self, AIGenericNode_t* )
{
	BotStandStill( self );
	return STATUS_SUCCESS;
}

AINodeStatus_t BotActionStrafeDodge( gentity_t *self, AIGenericNode_t* )
{
	BotStrafeDodge( self );
	return STATUS_SUCCESS;
}

AINodeStatus_t BotActionAlternateStrafe( gentity_t *self, AIGenericNode_t* )
{
	BotAlternateStrafe( self );
	return STATUS_SUCCESS;
}

AINodeStatus_t BotActionClassDodge( gentity_t *self, AIGenericNode_t* )
{
	BotClassMovement( self, BotTargetInAttackRange( self, self->botMind->goal ) );
	return STATUS_SUCCESS;
}

AINodeStatus_t BotActionChangeGoal( gentity_t *self, AIGenericNode_t *node )
{
	AIActionNode_t *a = ( AIActionNode_t * ) node;
	AIEntity_t et = ( AIEntity_t ) AIUnBoxInt( a->params[ 0 ] );
	botEntityAndDistance_t e = AIEntityToGentity( self, et );

	if ( !BotChangeGoalEntity( self, e.ent ) )
	{
		return STATUS_FAILURE;
	}

	self->botMind->currentNode = node;
	return STATUS_SUCCESS;
}

AINodeStatus_t BotActionEvolveTo( gentity_t *self, AIGenericNode_t *node )
{
	AIActionNode_t *action = ( AIActionNode_t * ) node;
	class_t c = ( class_t )  AIUnBoxInt( action->params[ 0 ] );

	if ( self->client->ps.stats[ STAT_CLASS ] == c )
	{
		return STATUS_SUCCESS;
	}

	if ( BotEvolveToClass( self, c ) )
	{
		return STATUS_SUCCESS;
	}

	return STATUS_FAILURE;
}

AINodeStatus_t BotActionSay( gentity_t *self, AIGenericNode_t *node )
{
	AIActionNode_t *action = ( AIActionNode_t * ) node;
	const char *str = AIUnBoxString( action->params[ 0 ] );
	saymode_t   say = ( saymode_t ) AIUnBoxInt( action->params[ 1 ] );
	G_Say( self, say, str );
	return STATUS_SUCCESS;
}

// TODO: Move decision making out of these actions and into the rest of the behavior tree
AINodeStatus_t BotActionFight( gentity_t *self, AIGenericNode_t *node )
{
	team_t myTeam = ( team_t ) self->client->pers.team;

	if ( self->botMind->currentNode != node )
	{
		if ( !BotChangeGoalEntity( self, self->botMind->bestEnemy.ent ) )
		{
			return STATUS_FAILURE;
		}

		self->botMind->currentNode = node;
		self->botMind->enemyLastSeen = level.time;
		return STATUS_RUNNING;
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	if ( !BotEnemyIsValid( self, self->botMind->goal.ent ) )
	{
		return STATUS_SUCCESS;
	}

	if ( !self->botMind->nav.havePath )
	{
		return STATUS_FAILURE;
	}

	if ( WeaponIsEmpty( BG_GetPlayerWeapon( &self->client->ps ), &self->client->ps ) && myTeam == TEAM_HUMANS )
	{
		G_ForceWeaponChange( self, WP_BLASTER );
	}

	if ( BG_GetPlayerWeapon( &self->client->ps ) == WP_HBUILD )
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
		BotSetTarget( &proposedTarget, self->botMind->bestEnemy.ent, nullptr );

		//we can see another enemy (not our target) so switch to it
		if ( self->botMind->bestEnemy.ent && self->botMind->goal.ent != self->botMind->bestEnemy.ent && BotPathIsWalkable( self, proposedTarget ) )
		{
			return STATUS_SUCCESS;
		}
		else if ( level.time - self->botMind->enemyLastSeen >= g_bot_chasetime.integer )
		{
			return STATUS_SUCCESS;
		}
		else
		{
			BotMoveToGoal( self );
			return STATUS_RUNNING;
		}
	}
	else
	{
		bool inAttackRange = BotTargetInAttackRange( self, self->botMind->goal );
		self->botMind->enemyLastSeen = level.time;

		if ( ( inAttackRange && myTeam == TEAM_HUMANS ) || self->botMind->nav.directPathToGoal )
		{
			BotAimAtEnemy( self );

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

				BotSprint( self, true );
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
		self->botMind->currentNode = node;
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	if ( GoalInRange( self, 70 ) )
	{
		return STATUS_SUCCESS;
	}
	else
	{
		BotMoveToGoal( self );
	}

	return STATUS_RUNNING;
}

AINodeStatus_t BotActionRoamInRadius( gentity_t *self, AIGenericNode_t *node )
{
	AIActionNode_t *a = ( AIActionNode_t * ) node;
	AIEntity_t e = ( AIEntity_t ) AIUnBoxInt( a->params[ 0 ] );
	float radius = AIUnBoxFloat( a->params[ 1 ] );

	if ( node != self->botMind->currentNode )
	{
		vec3_t point;
		botEntityAndDistance_t ent = AIEntityToGentity( self, e );

		if ( !ent.ent )
		{
			return STATUS_FAILURE;
		}

		if ( !trap_BotFindRandomPointInRadius( self->s.number, ent.ent->s.origin, point, radius ) )
		{
			return STATUS_FAILURE;
		}

		if ( !BotChangeGoalPos( self, point ) )
		{
			return STATUS_FAILURE;
		}
		self->botMind->currentNode = node;
	}

	if ( self->botMind->nav.directPathToGoal && GoalInRange( self, 70 ) )
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
		self->botMind->currentNode = node;
	}

	if ( self->botMind->nav.directPathToGoal && GoalInRange( self, 70 ) )
	{
		return STATUS_SUCCESS;
	}
	else
	{
		BotMoveToGoal( self );
	}
	return STATUS_RUNNING;
}

botTarget_t BotGetMoveToTarget( gentity_t *self, AIEntity_t e )
{
	botTarget_t target;
	botEntityAndDistance_t en = AIEntityToGentity( self, e );
	BotSetTarget( &target, en.ent, nullptr );
	return target;
}

AINodeStatus_t BotActionMoveTo( gentity_t *self, AIGenericNode_t *node )
{
	float radius = 0;
	AIActionNode_t *moveTo = ( AIActionNode_t * ) node;
	AIEntity_t ent = ( AIEntity_t ) AIUnBoxInt( moveTo->params[ 0 ] );
	
	if ( moveTo->nparams > 1 )
	{
		radius = MAX( AIUnBoxFloat( moveTo->params[ 1 ] ), 0.0f );
	}

	if ( node != self->botMind->currentNode )
	{
		if ( !BotChangeGoal( self, BotGetMoveToTarget( self, ent ) ) )
		{
			return STATUS_FAILURE;
		}
		else
		{
			self->botMind->currentNode = node;
			return STATUS_RUNNING;
		}
	}

	if ( self->botMind->goal.ent )
	{
		// Don't move to dead targets.
		if ( G_Dead( self->botMind->goal.ent ) )
		{
			return STATUS_FAILURE;
		}
	}

	BotMoveToGoal( self );

	if ( radius == 0 )
	{
		radius = BotGetGoalRadius( self );
	}

	if ( DistanceToGoal2DSquared( self ) <= Square( radius ) && self->botMind->nav.directPathToGoal )
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
			self->botMind->currentNode = node;
			return STATUS_RUNNING;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	// Can only rush living targets.
	if ( !G_Alive( self->botMind->goal.ent ) )
	{
		return STATUS_FAILURE;
	}

	if ( !GoalInRange( self, 100 ) )
	{
		BotMoveToGoal( self );
	}
	return STATUS_RUNNING;
}

AINodeStatus_t BotActionHeal( gentity_t *self, AIGenericNode_t *node )
{
	if ( self->client->pers.team == TEAM_HUMANS )
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
AINodeStatus_t BotActionEvolve ( gentity_t *self, AIGenericNode_t* )
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
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL3 ) &&
	          ( !BG_ClassUnlocked( PCL_ALIEN_LEVEL3_UPG ) ||!g_bot_level2upg.integer ||
	            !g_bot_level3upg.integer ) && g_bot_level3.integer )
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
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL2 ) && g_bot_level2.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL2 ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL1 ) && g_bot_level1.integer )
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
	gentity_t *healTarget = nullptr;

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

	if ( self->client->pers.team != TEAM_ALIENS )
	{
		return STATUS_FAILURE;
	}

	if ( self->botMind->currentNode != node )
	{
		// already fully healed
		if ( self->entity->Get<HealthComponent>()->FullHealth() )
		{
			return STATUS_FAILURE;
		}

		if ( !BotChangeGoalEntity( self, healTarget ) )
		{
			return STATUS_FAILURE;
		}

		self->botMind->currentNode = node;
	}

	//we are fully healed now
	if ( self->entity->Get<HealthComponent>()->FullHealth() )
	{
		return STATUS_SUCCESS;
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	// Can't heal at dead targets.
	if ( G_Dead( self->botMind->goal.ent ) )
	{
		return STATUS_FAILURE;
	}

	if ( !GoalInRange( self, 100 ) )
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
	bool fullyHealed = self->entity->Get<HealthComponent>()->FullHealth() &&
	                   BG_InventoryContainsUpgrade( UP_MEDKIT, self->client->ps.stats );

	if ( self->client->pers.team != TEAM_HUMANS )
	{
		return STATUS_FAILURE;
	}

	if ( self->botMind->currentNode != node )
	{
		if ( fullyHealed )
		{
			return STATUS_FAILURE;
		}

		if ( !BotChangeGoalEntity( self, self->botMind->closestBuildings[ BA_H_MEDISTAT ].ent ) )
		{
			return STATUS_FAILURE;
		}
		self->botMind->currentNode = node;
	}

	if ( fullyHealed )
	{
		return STATUS_SUCCESS;
	}

	//safety check
	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	// Can't heal at dead targets.
	if ( G_Dead( self->botMind->goal.ent ) )
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
		self->botMind->currentNode = node;
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	// Can only repair alive targets.
	if ( !G_Alive( self->botMind->goal.ent ) )
	{
		return STATUS_FAILURE;
	}

	if ( self->botMind->goal.ent->entity->Get<HealthComponent>()->FullHealth() )
	{
		return STATUS_SUCCESS;
	}

	if ( BG_GetPlayerWeapon( &self->client->ps ) != WP_HBUILD )
	{
		G_ForceWeaponChange( self, WP_HBUILD );
	}

	AngleVectors( self->client->ps.viewangles, forward, nullptr, nullptr );
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
	AIActionNode_t *buy = ( AIActionNode_t * ) node;
	weapon_t  weapon;
	upgrade_t upgrades[4];
	int numUpgrades;
	int i;

	if ( buy->nparams == 0 )
	{
		// equip action
		BotGetDesiredBuy( self, &weapon, upgrades, &numUpgrades );
	}
	else
	{
		// first parameter should always be a weapon
		weapon = ( weapon_t ) AIUnBoxInt( buy->params[ 0 ] );

		if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS )
		{
			BotDPrintf( S_COLOR_YELLOW "WARNING: parameter 1 to action buy out of range\n" );
			weapon = WP_NONE;
		}

		numUpgrades = 0;

		// other parameters are always upgrades
		for ( i = 1; i < buy->nparams; i++ )
		{
			upgrades[ numUpgrades ] = ( upgrade_t ) AIUnBoxInt( buy->params[ i ] );

			if ( upgrades[ numUpgrades ] <= UP_NONE || upgrades[ numUpgrades ] >= UP_NUM_UPGRADES )
			{
				BotDPrintf( S_COLOR_YELLOW "WARNING: parameter %d to action buy out of range\n", i + 1 );
				continue;
			}

			numUpgrades++;
		}
	}

	if ( !g_bot_buy.integer )
	{
		return STATUS_FAILURE;
	}

	if ( BotGetEntityTeam( self ) != TEAM_HUMANS )
	{
		return STATUS_FAILURE;
	}

	//check if we already have everything
	if ( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) || weapon == WP_NONE )
	{
		int numContain = 0;

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
		botEntityAndDistance_t *ngoal;

		ngoal = &self->botMind->closestBuildings[ BA_H_ARMOURY ];

		if ( !ngoal->ent )
		{
			return STATUS_FAILURE; // no suitable goal found
		}

		if ( !BotChangeGoalEntity( self, ngoal->ent ) )
		{
			return STATUS_FAILURE;
		}
		self->botMind->currentNode = node;
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	// Can't buy at dead targets.
	if ( G_Dead( self->botMind->goal.ent ) )
	{
		return STATUS_FAILURE;
	}

	if ( !self->botMind->goal.ent->powered )
	{
		return STATUS_FAILURE;
	}

	if ( GoalInRange( self, ENTITY_BUY_RANGE ) )
	{
		if ( numUpgrades )
		{
			BotSellAll( self );
		}
		else if ( weapon != WP_NONE )
		{
			BotSellWeapons( self );
		}

		if ( weapon != WP_NONE )
		{
			BotBuyWeapon( self, weapon );
		}

		for ( i = 0; i < numUpgrades; i++ )
		{
			BotBuyUpgrade( self, upgrades[i] );
		}

		// make sure that we're not using the blaster
		if ( weapon != WP_NONE )
		{
			G_ForceWeaponChange( self, weapon );
		}
		
		return STATUS_SUCCESS;
	}

	BotMoveToGoal( self );
	return STATUS_RUNNING;
}
