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
#include "g_bot_ai.h"
#include "g_bot_util.h"

qboolean isBinaryOp( AIOpType_t op )
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
			return qtrue;
		default: return qfalse;
	}
}

qboolean isUnaryOp( AIOpType_t op )
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

AIValue_t AIBoxToken( const pc_token_t *token )
{
	if ( ( float ) token->intvalue != token->floatvalue )
	{
		return AIBoxFloat( token->floatvalue );
	}
	else
	{
		return AIBoxInt( token->intvalue );
	}
}

float AIUnBoxFloat( AIValue_t v )
{
	switch ( v.valType )
	{
		case VALUE_FLOAT:
			return v.l.floatValue;
		case VALUE_INT:
			return ( float ) v.l.intValue;
	}
	return 0.0f;
}

int AIUnBoxInt( AIValue_t v )
{
	switch ( v.valType )
	{
		case VALUE_FLOAT:
			return ( int ) v.l.floatValue;
		case VALUE_INT:
			return v.l.intValue;
	}
	return 0;
}

double AIUnBoxDouble( AIValue_t v )
{
	switch ( v.valType )
	{
		case VALUE_FLOAT:
			return ( double ) v.l.floatValue;
		case VALUE_INT:
			return ( double ) v.l.intValue;
	}
	return 0.0;
}

botEntityAndDistance_t *AIEntityToGentity( gentity_t *self, AIEntity_t e )
{
	if ( e > BA_NONE && e < BA_NUM_BUILDABLES )
	{
		if ( !self->botMind->closestBuildings[ e ].ent )
		{
			return NULL;
		}
		return &self->botMind->closestBuildings[ e ];
	}
	else if ( e == E_ENEMY )
	{
		if ( !self->botMind->bestEnemy.ent )
		{
			return NULL;
		}
		return &self->botMind->bestEnemy;
	}
	else if ( e == E_DAMAGEDBUILDING )
	{
		if ( !self->botMind->closestDamagedBuilding.ent )
		{
			return NULL;
		}
		return &self->botMind->closestDamagedBuilding;
	}
	else
	{
		return NULL;
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
qboolean EvalConditionExpression( gentity_t *self, AIExpType_t *exp );

double EvalFunc( gentity_t *self, AIExpType_t *exp )
{
	AIValueFunc_t *v = ( AIValueFunc_t * ) exp;

	return AIUnBoxDouble( v->func( self, v->params ) );
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

qboolean EvaluateBinaryOp( gentity_t *self, AIExpType_t *exp )
{
	AIBinaryOp_t *o = ( AIBinaryOp_t * ) exp;
	qboolean      ret = qfalse;

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
			return qfalse;
	}
}

qboolean EvaluateUnaryOp( gentity_t *self, AIExpType_t *exp )
{
	AIUnaryOp_t *o = ( AIUnaryOp_t * ) exp;
	return !EvalConditionExpression( self, o->exp );
}

qboolean EvalConditionExpression( gentity_t *self, AIExpType_t *exp )
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

	return qfalse;
}

AINodeStatus_t BotConditionNode( gentity_t *self, AIGenericNode_t *node )
{
	qboolean success = qfalse;

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
		botEntityAndDistance_t *ent = AIEntityToGentity( self, e );

		if ( !ent )
		{
			return STATUS_FAILURE;
		}

		if ( !trap_BotFindRandomPointInRadius( self->s.number, ent->ent->s.origin, point, radius ) )
		{
			return STATUS_FAILURE;
		}

		if ( !BotChangeGoalPos( self, point ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( self->botMind->directPathToGoal && GoalInRange( self, 70 ) )
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

	if ( self->botMind->directPathToGoal && GoalInRange( self, 70 ) )
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
	gentity_t *ent = NULL;
	botEntityAndDistance_t *en;
	en = AIEntityToGentity( self, e );

	if ( en )
	{
		ent = en->ent;
	}

	BotSetTarget( &target, ent, NULL );
	return target;
}

AINodeStatus_t BotActionMoveTo( gentity_t *self, AIGenericNode_t *node )
{
	float radius = 0;
	AIActionNode_t *moveTo = ( AIActionNode_t * ) node;
	AIEntity_t ent = ( AIEntity_t ) AIUnBoxInt( moveTo->params[ 0 ] );
	
	if ( moveTo->nparams > 1 )
	{
		radius = MAX( AIUnBoxFloat( moveTo->params[ 1 ] ), 0 );
	}

	if ( node != self->botMind->currentNode )
	{
		if ( !BotChangeGoal( self, BotGetMoveToTarget( self, ent ) ) )
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

	if ( radius == 0 )
	{
		radius = BotGetGoalRadius( self );
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

	if ( !GoalInRange( self, 100 ) )
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

	if ( self->botMind->currentNode != node )
	{
		// already fully healed
		if ( maxHealth == self->client->ps.stats[ STAT_HEALTH ] )
		{
			return STATUS_FAILURE;
		}

		if ( !BotChangeGoalEntity( self, healTarget ) )
		{
			return STATUS_FAILURE;
		}
	}

	//we are fully healed now
	if ( maxHealth == self->client->ps.stats[STAT_HEALTH] )
	{
		return STATUS_SUCCESS;
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
	qboolean fullyHealed = BG_Class( self->client->ps.stats[ STAT_CLASS ] )->health <= self->client->ps.stats[ STAT_HEALTH ] &&
	                       BG_InventoryContainsUpgrade( UP_MEDKIT, self->client->ps.stats );

	if ( self->client->ps.stats[STAT_TEAM] != TEAM_HUMANS )
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
	AIActionNode_t *buy = ( AIActionNode_t * ) node;
	
	weapon_t  weapon;
	upgrade_t upgrades[3];
	int numUpgrades = MAX( buy->nparams - 1, 0 );
	int i;

	if ( buy->nparams == 0 )
	{
		BotGetDesiredBuy( self, &weapon, upgrades, &numUpgrades );
	}
	else
	{
		if ( buy->nparams >= 1 )
		{
			weapon = AIUnBoxInt( buy->params[ 0 ] );

			if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS )
			{
				BotDPrintf( S_COLOR_YELLOW "WARNING: parameter 1 to action buy out of range\n" );
				weapon = WP_NONE;
			}
		}

		if ( numUpgrades )
		{
			int j = 0;
			int n = numUpgrades;
			for ( i = 0; i < n; i++ )
			{
				upgrades[ i - j ] = AIUnBoxInt( buy->params[ i + 1 ] );

				if ( upgrades[ i - j ] <= UP_NONE || upgrades[ i - j ] >= UP_NUM_UPGRADES )
				{
					BotDPrintf( S_COLOR_YELLOW "WARNING: parameter %d to action buy out of range\n", i + 1 );
					numUpgrades--;
					j++;
				}
			}
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

	if ( !GoalInRange( self, 100 ) )
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
