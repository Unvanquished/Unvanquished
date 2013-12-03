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
#include "g_bot_util.h"
#include "../../engine/botlib/bot_types.h"

//tells if all navmeshes loaded successfully
qboolean navMeshLoaded = qfalse;

/*
========================
Navigation Mesh Loading
========================
*/

// FIXME: use nav handle instead of classes
void G_BotNavInit()
{
	int i;

	Com_Printf( "==== Bot Navigation Initialization ==== \n" );

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		classModelConfig_t *model;
		botClass_t bot;
		bot.polyFlagsInclude = POLYFLAGS_WALK;
		bot.polyFlagsExclude = POLYFLAGS_DISABLED;

		model = BG_ClassModelConfig( i );
		if ( model->navMeshClass )
		{
			if ( BG_ClassModelConfig( model->navMeshClass )->navMeshClass )
			{
				Com_Printf( S_ERROR "class '%s': navmesh reference target class '%s' must have its own navmesh\n",
				            BG_Class( i )->name, BG_Class( model->navMeshClass )->name );
				return;
			}

			continue;
		}

		Q_strncpyz( bot.name, BG_Class( i )->name, sizeof( bot.name ) );

		if ( !trap_BotSetupNav( &bot, &model->navHandle ) )
		{
			return;
		}
	}
	navMeshLoaded = qtrue;
}

void G_BotNavCleanup( void )
{
	trap_BotShutdownNav();
	navMeshLoaded = qfalse;
}

void G_BotDisableArea( vec3_t origin, vec3_t mins, vec3_t maxs )
{
	trap_BotDisableArea( origin, mins, maxs );
}

void G_BotEnableArea( vec3_t origin, vec3_t mins, vec3_t maxs )
{
	trap_BotEnableArea( origin, mins, maxs );
}

void BotSetNavmesh( gentity_t  *self, class_t newClass )
{
	int navHandle;
	const classModelConfig_t *model;

	if ( newClass == PCL_NONE )
	{
		return;
	}

	model = BG_ClassModelConfig( newClass );
	navHandle = model->navMeshClass
	          ? BG_ClassModelConfig( model->navMeshClass )->navHandle
	          : model->navHandle;

	trap_BotSetNavMesh( self->s.number, navHandle );
}

/*
========================
Bot Navigation Querys
========================
*/
float RadiusFromBounds2D( vec3_t mins, vec3_t maxs )
{
	float rad1s = Square( mins[0] ) + Square( mins[1] );
	float rad2s = Square( maxs[0] ) + Square( maxs[1] );
	return sqrt( MAX( rad1s, rad2s ) );
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

qboolean GoalInRange( gentity_t *self, float r )
{
	gentity_t *ent = NULL;

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return ( Distance( self->s.origin, self->botMind->nav.tpos ) < r );
	}

	while ( ( ent = G_IterateEntitiesWithinRadius( ent, self->s.origin, r ) ) )
	{
		if ( ent == self->botMind->goal.ent )
		{
			return qtrue;
		}
	}

	return qfalse;
}

int DistanceToGoal2DSquared( gentity_t *self )
{
	vec3_t vec;
	vec3_t goalPos;

	BotGetTargetPos( self->botMind->goal, goalPos );

	VectorSubtract( goalPos, self->s.origin, vec );

	return Square( vec[ 0 ] ) + Square( vec[ 1 ] );
}

int DistanceToGoal( gentity_t *self )
{
	vec3_t targetPos;
	vec3_t selfPos;
	//safety check for morons who use this incorrectly
	if ( !( self->botMind ) )
	{
		return -1;
	}
	BotGetTargetPos( self->botMind->goal, targetPos );
	VectorCopy( self->s.origin, selfPos );
	return Distance( selfPos, targetPos );
}

int DistanceToGoalSquared( gentity_t *self )
{
	vec3_t targetPos;
	vec3_t selfPos;
	//safety check for morons who use this incorrectly
	if ( !( self->botMind ) )
	{
		return -1;
	}
	BotGetTargetPos( self->botMind->goal, targetPos );
	VectorCopy( self->s.origin, selfPos );
	return DistanceSquared( selfPos, targetPos );
}

qboolean BotPathIsWalkable( gentity_t *self, botTarget_t target )
{
	vec3_t selfPos, targetPos;
	vec3_t viewNormal;
	botTrace_t trace;

	BG_GetClientNormal( &self->client->ps, viewNormal );
	VectorMA( self->s.origin, self->r.mins[2], viewNormal, selfPos );
	BotGetTargetPos( target, targetPos );

	if ( !trap_BotNavTrace( self->s.number, &trace, selfPos, targetPos ) )
	{
		return qfalse;
	}

	if ( trace.frac >= 1.0f )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

void BotFindRandomPointOnMesh( gentity_t *self, vec3_t point )
{
	trap_BotFindRandomPoint( self->s.number, point );
}

/*
========================
Local Bot Navigation
========================
*/

signed char BotGetMaxMoveSpeed( gentity_t *self )
{
	if ( usercmdButtonPressed( self->botMind->cmdBuffer.buttons, BUTTON_WALKING ) )
	{
		return 63;
	}

	return 127;
}

void BotStrafeDodge( gentity_t *self )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;
	signed char speed = BotGetMaxMoveSpeed( self );

	if ( self->client->time1000 >= 500 )
	{
		botCmdBuffer->rightmove = speed;
	}
	else
	{
		botCmdBuffer->rightmove = -speed;
	}

	if ( ( self->client->time10000 % 2000 ) < 1000 )
	{
		botCmdBuffer->rightmove *= -1;
	}

	if ( ( self->client->time1000 % 300 ) >= 100 && ( self->client->time10000 % 3000 ) > 2000 )
	{
		botCmdBuffer->rightmove = 0;
	}
}

void BotMoveInDir( gentity_t *self, uint32_t dir )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;
	signed char speed = BotGetMaxMoveSpeed( self );

	if ( dir & MOVE_FORWARD )
	{
		botCmdBuffer->forwardmove = speed;
	}
	else if ( dir & MOVE_BACKWARD )
	{
		botCmdBuffer->forwardmove = -speed;
	}

	if ( dir & MOVE_RIGHT )
	{
		botCmdBuffer->rightmove = speed;
	}
	else if ( dir & MOVE_LEFT )
	{
		botCmdBuffer->rightmove = -speed;
	}
}

void BotAlternateStrafe( gentity_t *self )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;
	signed char speed = BotGetMaxMoveSpeed( self );

	if ( level.time % 8000 < 4000 )
	{
		botCmdBuffer->rightmove = speed;
	}
	else
	{
		botCmdBuffer->rightmove = -speed;
	}
}

void BotStandStill( gentity_t *self )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	BotWalk( self, qfalse );
	BotSprint( self, qfalse );
	botCmdBuffer->forwardmove = 0;
	botCmdBuffer->rightmove = 0;
	botCmdBuffer->upmove = 0;
}

qboolean BotJump( gentity_t *self )
{
	int staminaJumpCost;

	if ( self->client->pers.team == TEAM_HUMANS )
	{
		staminaJumpCost = BG_Class( self->client->ps.stats[ STAT_CLASS ] )->staminaJumpCost;

		if ( self->client->ps.stats[STAT_STAMINA] < staminaJumpCost )
		{
			return qfalse;
		}
	}

	self->botMind->cmdBuffer.upmove = 127;
	return qtrue;
}

qboolean BotSprint( gentity_t *self, qboolean enable )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;
	int       staminaJumpCost;

	if ( !enable )
	{
		usercmdReleaseButton( botCmdBuffer->buttons, BUTTON_SPRINT );
		return qfalse;
	}

	staminaJumpCost = BG_Class( self->client->ps.stats[ STAT_CLASS ] )->staminaJumpCost;

	if ( self->client->pers.team == TEAM_HUMANS
	     && self->client->ps.stats[ STAT_STAMINA ] > staminaJumpCost
	     && self->botMind->botSkill.level >= 5 )
	{
		usercmdPressButton( botCmdBuffer->buttons, BUTTON_SPRINT );
		BotWalk( self, qfalse );
		return qtrue;
	}
	else
	{
		usercmdReleaseButton( botCmdBuffer->buttons, BUTTON_SPRINT );
		return qfalse;
	}
}

void BotWalk( gentity_t *self, qboolean enable )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	if ( !enable )
	{
		if ( usercmdButtonPressed( botCmdBuffer->buttons, BUTTON_WALKING ) )
		{
			usercmdReleaseButton( botCmdBuffer->buttons, BUTTON_WALKING );
			botCmdBuffer->forwardmove *= 2;
			botCmdBuffer->rightmove *= 2;
		}
		return;
	}

	if ( !usercmdButtonPressed( botCmdBuffer->buttons, BUTTON_WALKING ) )
	{
		usercmdPressButton( botCmdBuffer->buttons, BUTTON_WALKING );
		botCmdBuffer->forwardmove /= 2;
		botCmdBuffer->rightmove /= 2;
	}
}

#define STEPSIZE 18.0f
gentity_t* BotGetPathBlocker( gentity_t *self, const vec3_t dir )
{
	vec3_t playerMins, playerMaxs;
	vec3_t end;
	trace_t trace;
	const int TRACE_LENGTH = BOT_OBSTACLE_AVOID_RANGE;

	if ( !( self && self->client ) )
	{
		return NULL;
	}

	BG_ClassBoundingBox( ( class_t ) self->client->ps.stats[STAT_CLASS], playerMins, playerMaxs, NULL, NULL, NULL );

	//account for how large we can step
	playerMins[2] += STEPSIZE;
	playerMaxs[2] += STEPSIZE;

	VectorMA( self->s.origin, TRACE_LENGTH, dir, end );

	trap_Trace( &trace, self->s.origin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT );
	if ( ( trace.fraction < 1.0f && trace.plane.normal[ 2 ] < 0.7f ) || g_entities[ trace.entityNum ].s.eType == ET_BUILDABLE )
	{
		return &g_entities[trace.entityNum];
	}
	else
	{
		return NULL;
	}
}

qboolean BotShouldJump( gentity_t *self, gentity_t *blocker, const vec3_t dir )
{
	vec3_t playerMins;
	vec3_t playerMaxs;
	float jumpMagnitude;
	trace_t trace;
	const int TRACE_LENGTH = BOT_OBSTACLE_AVOID_RANGE;
	vec3_t end;

	//blocker is not on our team, so ignore
	if ( BotGetEntityTeam( self ) != BotGetEntityTeam( blocker ) )
	{
		return qfalse;
	}

	//already normalized

	BG_ClassBoundingBox( ( class_t ) self->client->ps.stats[STAT_CLASS], playerMins, playerMaxs, NULL, NULL, NULL );

	playerMins[2] += STEPSIZE;
	playerMaxs[2] += STEPSIZE;

	//trap_Print(vtos(self->movedir));
	VectorMA( self->s.origin, TRACE_LENGTH, dir, end );

	//make sure we are moving into a block
	trap_Trace( &trace, self->s.origin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT );
	if ( trace.fraction >= 1.0f || blocker != &g_entities[trace.entityNum] )
	{
		return qfalse;
	}

	jumpMagnitude = BG_Class( ( class_t )self->client->ps.stats[STAT_CLASS] )->jumpMagnitude;

	//find the actual height of our jump
	jumpMagnitude = Square( jumpMagnitude ) / ( self->client->ps.gravity * 2 );

	//prepare for trace
	playerMins[2] += jumpMagnitude;
	playerMaxs[2] += jumpMagnitude;

	//check if jumping will clear us of entity
	trap_Trace( &trace, self->s.origin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT );

	//if we can jump over it, then jump
	//note that we also test for a blocking barricade because barricades will collapse to let us through
	if ( blocker->s.modelindex == BA_A_BARRICADE || trace.fraction == 1.0f )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

qboolean BotFindSteerTarget( gentity_t *self, vec3_t dir )
{
	vec3_t forward;
	vec3_t testPoint1, testPoint2;
	vec3_t playerMins, playerMaxs;
	float yaw1, yaw2;
	trace_t trace1, trace2;
	int i;
	vec3_t angles;

	if ( !( self && self->client ) )
	{
		return qfalse;
	}

	//get bbox
	BG_ClassBoundingBox( ( class_t ) self->client->ps.stats[STAT_CLASS], playerMins, playerMaxs, NULL, NULL, NULL );

	//account for stepsize
	playerMins[2] += STEPSIZE;
	playerMaxs[2] += STEPSIZE;

	//get the yaw (left/right) we dont care about up/down
	vectoangles( dir, angles );
	yaw1 = yaw2 = angles[ YAW ];

	//directly infront of us is blocked, so dont test it
	yaw1 -= 15;
	yaw2 += 15;

	//forward vector is 2D
	forward[ 2 ] = 0;

	//find an unobstructed position
	//we check the full 180 degrees in front of us
	for ( i = 0; i < 5; i++, yaw1 -= 15 , yaw2 += 15 )
	{
		//compute forward for right
		forward[0] = cos( DEG2RAD( yaw1 ) );
		forward[1] = sin( DEG2RAD( yaw1 ) );
		//forward is already normalized
		//try the right
		VectorMA( self->s.origin, BOT_OBSTACLE_AVOID_RANGE, forward, testPoint1 );

		//test it
		trap_Trace( &trace1, self->s.origin, playerMins, playerMaxs, testPoint1, self->s.number, MASK_SHOT );

		//check if unobstructed
		if ( trace1.fraction >= 1.0f )
		{
			VectorCopy( forward, dir );
			return qtrue;
		}

		//compute forward for left
		forward[0] = cos( DEG2RAD( yaw2 ) );
		forward[1] = sin( DEG2RAD( yaw2 ) );
		//forward is already normalized
		//try the left
		VectorMA( self->s.origin, BOT_OBSTACLE_AVOID_RANGE, forward, testPoint2 );

		//test it
		trap_Trace( &trace2, self->s.origin, playerMins, playerMaxs, testPoint2, self->s.number, MASK_SHOT );

		//check if unobstructed
		if ( trace2.fraction >= 1.0f )
		{
			VectorCopy( forward, dir );
			return qtrue;
		}
	}

	//we couldnt find a new position
	return qfalse;
}
qboolean BotAvoidObstacles( gentity_t *self, vec3_t dir )
{
	gentity_t *blocker;

	blocker = BotGetPathBlocker( self, dir );

	if ( blocker )
	{
		if ( BotShouldJump( self, blocker, dir ) )
		{
			BotJump( self );
			return qfalse;
		}
		else if ( !BotFindSteerTarget( self, dir ) )
		{
			vec3_t angles;
			vec3_t right;
			vectoangles( dir, angles );
			AngleVectors( angles, dir, right, NULL );

			if ( ( self->client->time10000 % 2000 ) < 1000 )
			{
				VectorCopy( right, dir );
			}
			else
			{
				VectorNegate( right, dir );
			}
			dir[ 2 ] = 0;
			VectorNormalize( dir );
		}
		return qtrue;
	}
	return qfalse;
}

//copy of PM_CheckLadder in bg_pmove.c
qboolean BotOnLadder( gentity_t *self )
{
	vec3_t forward, end;
	vec3_t mins, maxs;
	trace_t trace;

	if ( !BG_ClassHasAbility( ( class_t ) self->client->ps.stats[ STAT_CLASS ], SCA_CANUSELADDERS ) )
	{
		return qfalse;
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );

	forward[ 2 ] = 0.0f;
	BG_ClassBoundingBox( ( class_t ) self->client->ps.stats[ STAT_CLASS ], mins, maxs, NULL, NULL, NULL );
	VectorMA( self->s.origin, 1.0f, forward, end );

	trap_Trace( &trace, self->s.origin, mins, maxs, end, self->s.number, MASK_PLAYERSOLID );

	if ( trace.fraction < 1.0f && trace.surfaceFlags & SURF_LADDER )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

void BotDirectionToUsercmd( gentity_t *self, vec3_t dir, usercmd_t *cmd )
{
	vec3_t forward;
	vec3_t right;

	float forwardmove;
	float rightmove;
	signed char speed = BotGetMaxMoveSpeed( self );

	AngleVectors( self->client->ps.viewangles, forward, right, NULL );
	forward[2] = 0;
	VectorNormalize( forward );
	right[2] = 0;
	VectorNormalize( right );

	// get direction and non-optimal magnitude
	forwardmove = speed * DotProduct( forward, dir );
	rightmove = speed * DotProduct( right, dir );

	// find optimal magnitude to make speed as high as possible
	if ( Q_fabs( forwardmove ) > Q_fabs( rightmove ) )
	{
		float highestforward = forwardmove < 0 ? -speed : speed;

		float highestright = highestforward * rightmove / forwardmove;

		cmd->forwardmove = ClampChar( highestforward );
		cmd->rightmove = ClampChar( highestright );
	}
	else
	{
		float highestright = rightmove < 0 ? -speed : speed;

		float highestforward = highestright * forwardmove / rightmove;

		cmd->forwardmove = ClampChar( highestforward );
		cmd->rightmove = ClampChar( highestright );
	}
}

void BotSeek( gentity_t *self, vec3_t direction )
{
	vec3_t viewOrigin;
	vec3_t seekPos;

	BG_GetClientViewOrigin( &self->client->ps, viewOrigin );

	VectorNormalize( direction );

	// move directly toward the target
	BotDirectionToUsercmd( self, direction, &self->botMind->cmdBuffer );

	VectorMA( viewOrigin, 100, direction, seekPos );

	// slowly change our aim to point to the target
	BotSlowAim( self, seekPos, 0.6 );
	BotAimAtLocation( self, seekPos );
}

/*
=========================
Global Bot Navigation
=========================
*/

void BotClampPos( gentity_t *self )
{
	float height = self->client->ps.origin[ 2 ];
	vec3_t origin;
	trace_t trace;
	vec3_t mins, maxs;
	VectorSet( origin, self->botMind->nav.pos[ 0 ], self->botMind->nav.pos[ 1 ], height );
	BG_ClassBoundingBox( self->client->ps.stats[ STAT_CLASS ], mins, maxs, NULL, NULL, NULL );
	trap_Trace( &trace, self->client->ps.origin, mins, maxs, origin, self->client->ps.clientNum, MASK_PLAYERSOLID );
	G_SetOrigin( self, trace.endpos );
	VectorCopy( trace.endpos, self->client->ps.origin );
}

void BotMoveToGoal( gentity_t *self )
{
	int    staminaJumpCost;
	vec3_t dir;
	VectorCopy( self->botMind->nav.dir, dir );

	if ( dir[ 2 ] < 0 )
	{
		dir[ 2 ] = 0;
		VectorNormalize( dir );
	}

	BotAvoidObstacles( self, dir );
	BotSeek( self, dir );

	staminaJumpCost = BG_Class( self->client->ps.stats[ STAT_CLASS ] )->staminaJumpCost;

	//dont sprint or dodge if we dont have enough stamina and are about to slow
	if ( self->client->pers.team == TEAM_HUMANS
	     && self->client->ps.stats[ STAT_STAMINA ] < staminaJumpCost )
	{
		usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

		usercmdReleaseButton( botCmdBuffer->buttons, BUTTON_SPRINT );
		usercmdReleaseButton( botCmdBuffer->buttons, BUTTON_DODGE );

		// walk to regain stamina
		BotWalk( self, qtrue );
	}
}

qboolean FindRouteToTarget( gentity_t *self, botTarget_t target, qboolean allowPartial )
{
	botRouteTarget_t routeTarget;
	BotTargetToRouteTarget( self, target, &routeTarget );
	return trap_BotFindRoute( self->s.number, &routeTarget, allowPartial );
}
