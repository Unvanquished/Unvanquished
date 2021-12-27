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

#include "sg_bot_util.h"
#include "botlib/bot_types.h"
#include "botlib/bot_api.h"
#include "shared/bot_nav_shared.h"

#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/rotate_vector.hpp>

//tells if all navmeshes loaded successfully
bool navMeshLoaded = false;

/*
========================
Navigation Mesh Loading
========================
*/

// FIXME: use nav() handle instead of classes
bool G_BotNavInit()
{
	Log::Notice( "==== Bot Navigation Initialization ====" );

	for ( class_t i : RequiredNavmeshes() )
	{
		classModelConfig_t *model;
		botClass_t bot;
		bot.polyFlagsInclude = POLYFLAGS_WALK;
		bot.polyFlagsExclude = POLYFLAGS_DISABLED;

		model = BG_ClassModelConfig( i );
		ASSERT_EQ( model->navMeshClass, PCL_NONE ); // shouldn't load this if we are going to use another class's mesh

		Q_strncpyz( bot.name, BG_Class( i )->name, sizeof( bot.name ) );

		if ( !G_BotSetupNav( &bot, &model->navHandle ) )
		{
			return false;
		}
	}
	navMeshLoaded = true;
	return true;
}

void G_BotNavCleanup()
{
	G_BotShutdownNav();
	navMeshLoaded = false;
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
	if ( model->navMeshClass )
	{
		ASSERT_EQ( BG_ClassModelConfig( model->navMeshClass )->navMeshClass, PCL_NONE );
		navHandle = BG_ClassModelConfig( model->navMeshClass )->navHandle;
	}
	else
	{
		navHandle = model->navHandle;
	}

	G_BotSetNavMesh( self->s.number, navHandle );
}

/*
========================
Bot Navigation Querys
========================
*/

static float RadiusFromBounds2D( const glm::vec3& mins, const glm::vec3& maxs )
{
	float rad1s = Square( mins[0] ) + Square( mins[1] );
	float rad2s = Square( maxs[0] ) + Square( maxs[1] );
	return sqrtf( std::max( rad1s, rad2s ) );
}

float BotGetGoalRadius( const gentity_t *self )
{
	botTarget_t &t = self->botMind->goal;
	glm::vec3 self_mins = VEC2GLM( self->r.mins );
	glm::vec3 self_maxs = VEC2GLM( self->r.maxs );
	if ( t.targetsCoordinates() )
	{
		// we check the coord to be (almost) in our bounding box
		return RadiusFromBounds2D( self_mins, self_maxs );
	}

	// we don't check if the entity is valid: an outdated result is
	// better than failing here
	const gentity_t *target = t.getTargetedEntity();
	if ( target->s.modelindex == BA_H_MEDISTAT || target->s.modelindex == BA_A_BOOSTER )
	{
		// we want to be quite close to medistat.
		// TODO(freem): is this really what we want for booster?
		return self->r.maxs[0] + target->r.maxs[0];
	}

	return RadiusFromBounds2D( VEC2GLM( target->r.mins ), VEC2GLM( target->r.maxs ) )
	       + RadiusFromBounds2D( self_mins, self_maxs );
}

bool GoalInRange( const gentity_t *self, float r )
{
	gentity_t *ent = nullptr;
	// we don't need to check the goal is valid here

	if ( self->botMind->goal.targetsCoordinates() )
	{
		return ( glm::distance( VEC2GLM( self->s.origin ), self->botMind->nav().glm_tpos() ) < r );
	}

	while ( ( ent = G_IterateEntitiesWithinRadius( ent, VEC2GLM( self->s.origin ), r ) ) )
	{
		if ( ent == self->botMind->goal.getTargetedEntity() )
		{
			return true;
		}
	}

	return false;
}

float DistanceToGoal2DSquared( const gentity_t *self )
{
	glm::vec3 vec = self->botMind->goal.getPos() - VEC2GLM( self->s.origin );

	return Square( vec[ 0 ] ) + Square( vec[ 1 ] );
}

float DistanceToGoal( const gentity_t *self )
{
	glm::vec3 targetPos = self->botMind->goal.getPos();
	glm::vec3 selfPos = VEC2GLM( self->s.origin );
	return glm::distance( selfPos, targetPos );
}

float DistanceToGoalSquared( const gentity_t *self )
{
	glm::vec3 targetPos = self->botMind->goal.getPos();
	glm::vec3 selfPos = VEC2GLM( self->s.origin );
	return glm::distance2( selfPos, targetPos );
}

bool BotPathIsWalkable( const gentity_t *self, botTarget_t target )
{
	botTrace_t trace;

	glm::vec3 viewNormal = BG_GetClientNormal( &self->client->ps );
	glm::vec3 selfPos = VEC2GLM( self->s.origin ) + self->r.mins[2] * viewNormal;

	if ( !G_BotNavTrace( self->s.number, &trace, selfPos, target.getPos() ) )
	{
		return false;
	}

	return trace.frac >= 1.0f;
}

/*
========================
Local Bot Navigation
========================
*/

signed char BotGetMaxMoveSpeed( gentity_t *self )
{
	if ( usercmdButtonPressed( self->botMind->cmdBuffer.buttons, BTN_WALKING ) )
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

	BotWalk( self, false );
	BotSprint( self, false );
	botCmdBuffer->forwardmove = 0;
	botCmdBuffer->rightmove = 0;
	botCmdBuffer->upmove = 0;
}

bool BotJump( gentity_t *self )
{
	if ( self->client->pers.team == TEAM_HUMANS )
	{
		int staminaJumpCost = BG_Class( self->client->ps.stats[ STAT_CLASS ] )->staminaJumpCost;

		if ( self->client->ps.stats[STAT_STAMINA] < staminaJumpCost )
		{
			return false;
		}
	}

	self->botMind->cmdBuffer.upmove = 127;
	return true;
}

bool BotSprint( gentity_t *self, bool enable )
{
	self->botMind->willSprint( enable );
	BotWalk( self, !enable );
	return enable;
}

void BotWalk( gentity_t *self, bool enable )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	if ( !enable )
	{
		if ( usercmdButtonPressed( botCmdBuffer->buttons, BTN_WALKING ) )
		{
			usercmdReleaseButton( botCmdBuffer->buttons, BTN_WALKING );
			botCmdBuffer->forwardmove *= 2;
			botCmdBuffer->rightmove *= 2;
		}
		return;
	}

	if ( !usercmdButtonPressed( botCmdBuffer->buttons, BTN_WALKING ) )
	{
		usercmdPressButton( botCmdBuffer->buttons, BTN_WALKING );
		botCmdBuffer->forwardmove /= 2;
		botCmdBuffer->rightmove /= 2;
	}
}

// A function which does a proper vertical scan for path is required.
// To do that, several points must be kept in mind:
//
// * stepsize can be negative
// * stepsize and jump magnitude cumulate
// * bots can not always jump (stamina)
// * bots can not always crouch (CMaxs == maxs)
// * traced entity *can* have dimensions
// * a binary search would *not* work, as it is possible to
//   have holes both above and under the traced entity
// * evaluate wether steering would be faster than crouching
//
// Traces use heavy maths, so better try to avoid them as much
// as possible.
// Hint: scan for biggest picture, and area in ranges around
// detected obstacles, reccursively call self as long as range
// is tall enough to pass through. Done by manipulating maxs
// and mins.
//
// result is written into, so that the actual height/angle of
// the possible jump can be calculated.
//
//TODO: optimize when traced object is a "physical entity" (building/player)
//TODO: detect missiles (fire on ground, rockets slowly moving, etc) to dodge them
//TODO move "forward" calculation to caller to avoid recalc each time
static bool detectWalk(
		glm::vec3 const& origin, glm::vec3 const& dir,
		glm::vec3 const& mins, glm::vec3 const& maxs,
		float floor, float ceiling,
		int entityNum,
		glm::vec3 &result )
{
	trace_t trace;
	ASSERT( mins[2] <= maxs[2] );
	ASSERT( mins[2] <= 0.f );
	float height = maxs[2] - mins[2];
	glm::vec3 tmins = mins + glm::vec3( 0.f, 0.f, floor );
	glm::vec3 tmaxs = maxs + glm::vec3( 0.f, 0.f, ceiling );

	trap_Trace( &trace, origin, tmins, tmaxs, dir, entityNum, MASK_SHOT, 0 );

	if ( trace.fraction == 0.0f )
	{
		return false;
	}

	if ( trace.fraction >= 1.0f )
	{
		result = dir;
		return true;
	}
	float offset_trace = trace.endpos[2] - origin[2];
	bool check = false;

	// avoid find same obstacle anew. Maybe +1 is a bit extreme, but should work.
	check = offset_trace + height + 1.f < tmaxs[2];
	if ( check && detectWalk( origin, dir, mins, maxs, offset_trace + fabs( mins[2] ) + 1.f, ceiling, entityNum, result ) )
	{
		result = dir;
		return true;
	}

	// avoid find same obstacle anew. Maybe -- is a bit extreme, but should work.
	check = tmins[2] < offset_trace - height - 1.f;
	if ( check && detectWalk( origin, dir, mins, maxs, floor, offset_trace - fabs( maxs[2] ) - 1.f, entityNum, result ) )
	{
		result = dir;
		return true;
	}

	return false;
}

static bool steer( gentity_t *self, glm::vec3 &dir )
{
	class_t pClass = static_cast<class_t>( self->client->ps.stats[ STAT_CLASS ] );
	glm::vec3 pMins, pMaxs, pCMaxs;
	BG_BoundingBox( pClass, pMins, pMaxs );
	pCMaxs = BG_CrouchBoundingBox( pClass );

	glm::vec3 origin = VEC2GLM( self->s.origin );
	float jumpMagnitude = BG_Class( pClass )->jumpMagnitude;
	jumpMagnitude = Square( jumpMagnitude ) / ( self->client->ps.gravity * 2 );

	bool canJump = BG_Class( pClass )->staminaJumpCost < self->client->ps.stats[STAT_STAMINA];
	bool canCrouch = pMaxs != pCMaxs;

	float lower  = 0.f - STEPSIZE;
	float higher = 0.f + STEPSIZE;
	float jump   = 0.f + STEPSIZE + jumpMagnitude;
	int entityNum = self->s.number;

	glm::vec3 forward = glm::normalize( glm::vec3( dir[0], dir[1], 0 ) );
	glm::vec3 tmp_fwd;

	float yaw = 0;
	// walk or jump straight {
	tmp_fwd = origin + BOT_OBSTACLE_AVOID_RANGE * forward;
	if ( detectWalk( origin, tmp_fwd, pMins, pMaxs, lower, higher, entityNum, dir ) )
	{
		return true;
	}
	if ( canJump && detectWalk( origin, tmp_fwd, pMins, pMaxs, higher, jump, entityNum, dir ) )
	{
		return true;
	}
	// }

	// try walking or jumping on lower angles {
	for ( yaw = 15; yaw < 30; yaw += 15 )
	{
		// +yaw
		glm::vec3 fw_inc = glm::rotateZ( forward, yaw );
		tmp_fwd = origin + BOT_OBSTACLE_AVOID_RANGE * fw_inc;
		if ( detectWalk( origin, tmp_fwd, pMins, pMaxs, lower, higher, entityNum, dir ) )
		{
			return true;
		}
		if ( canJump && detectWalk( origin, tmp_fwd, pMins, pMaxs, higher, jump, entityNum, dir ) )
		{
			return true;
		}

		// -yaw
		glm::vec3 fw_dec = glm::rotateZ( forward, 0 - yaw );
		tmp_fwd = origin + BOT_OBSTACLE_AVOID_RANGE * fw_dec;
		if ( detectWalk( origin, tmp_fwd, pMins, pMaxs, lower, higher, entityNum, dir ) )
		{
			return true;
		}
		if ( canJump && detectWalk( origin, tmp_fwd, pMins, pMaxs, higher, jump, entityNum, dir ) )
		{
			return true;
		}
	}
	// }

	// try crouching on lower angles {
	if ( canCrouch )
	{
		yaw = 0;
		tmp_fwd = origin + BOT_OBSTACLE_AVOID_RANGE * forward;
		if ( detectWalk( origin, tmp_fwd, pMins, pCMaxs, lower, higher, entityNum, dir ) )
		{
			return true;
		}
		for ( yaw = 15; yaw < 30; yaw += 15 )
		{
			// +yaw
			glm::vec3 fw_inc = glm::rotateZ( forward, yaw );
			tmp_fwd = origin + BOT_OBSTACLE_AVOID_RANGE * fw_inc;
			if ( detectWalk( origin, tmp_fwd, pMins, pCMaxs, lower, higher, entityNum, dir ) )
			{
				return true;
			}

			// +yaw
			glm::vec3 fw_dec = glm::rotateZ( forward, 0 - yaw );
			tmp_fwd = origin + BOT_OBSTACLE_AVOID_RANGE * fw_dec;
			if ( detectWalk( origin, tmp_fwd, pMins, pCMaxs, lower, higher, entityNum, dir ) )
			{
				return true;
			}
		}
	}
	// }

	//try all on bigger angles {
	for ( yaw = 30; yaw < 30; yaw += 15 )
	{
		// +yaw
		glm::vec3 fw_inc = glm::rotateZ( forward, yaw );
		tmp_fwd = origin + BOT_OBSTACLE_AVOID_RANGE * fw_inc;
		if ( detectWalk( origin, tmp_fwd, pMins, pMaxs, lower, higher, entityNum, dir ) )
		{
			return true;
		}
		if ( canJump && detectWalk( origin, tmp_fwd, pMins, pMaxs, higher, jump, entityNum, dir ) )
		{
			return true;
		}
		if ( canCrouch && detectWalk( origin, tmp_fwd, pMins, pCMaxs, lower, higher, entityNum, dir ) )
		{
			return true;
		}

		// -yaw
		glm::vec3 fw_dec = glm::rotateZ( forward, 0 - yaw );
		tmp_fwd = origin + BOT_OBSTACLE_AVOID_RANGE * fw_dec;
		if ( detectWalk( origin, tmp_fwd, pMins, pMaxs, lower, higher, entityNum, dir ) )
		{
			return true;
		}
		if ( canJump && detectWalk( origin, tmp_fwd, pMins, pMaxs, higher, jump, entityNum, dir ) )
		{
			return true;
		}
		if ( canCrouch && detectWalk( origin, tmp_fwd, pMins, pMaxs, lower, higher, entityNum, dir ) )
		{
			return true;
		}
	}
	// }
	return false;
}

static bool walkable( trace_t const& trace )
{
	return trace.fraction >= 1.0f || trace.plane.normal[2] >= 0.7f;
}

// verify if the way in a direction is free
// if yes, return true and updates the forward param
static bool checkWalkable( glm::vec3 const& origin, float yaw,
		glm::vec3 const& mins, glm::vec3 const& maxs,
		int entityNum, glm::vec3 &forward )
{
	trace_t trace;
	forward = glm::vec3( cosf( DEG2RAD( yaw ) ), sinf( DEG2RAD( yaw ) ), 0 );
	glm::vec3 end = origin + BOT_OBSTACLE_AVOID_RANGE * forward;
	trap_Trace( &trace, origin, mins, maxs, end, entityNum, MASK_SHOT, 0 );
	return walkable( trace );
}

// this should allow to check for all heights if passable.
// Astute readers will notice though that, in case of a jump,
// only peak heights of jumps will be checked, but currently
// there's no real way to efficiently know the position of
// a non-entity blocker, that is: geometry.
// If that would be possible, it would be possible to implement
// more efficient and less "buggy" version of this.
// Note that mins and maxs are passed by value, since they are
// modified inside but we don't want to pollute caller's values.
gentity_t const* getBlocker(
		float stepsize,
		glm::vec3 mins, glm::vec3 maxs,
		glm::vec3 const& start, glm::vec3 const& end,
		int entityNum )
{
	trace_t trace;
	mins[2] -= stepsize;
	maxs[2] -= stepsize;
	for ( int i = -1; i <= 1; ++i, mins[2] += stepsize, maxs[2] += stepsize )
	{
		trap_Trace( &trace, start, mins, maxs, end, entityNum, MASK_SHOT, 0 );
		if ( walkable( trace ) )
		{
			return nullptr;
		}
	}
	return &g_entities[trace.entityNum];
}

// return true if a path has been found
// This works by tracing forward. If the trace finds an
// obstacle, the bot will test several ways to go around:
// * jump
// * crouch
// * steer around, by right or left side
//
// If any of those tests succeed, returns true.
// Otherwise, return false and try to find a random path.
bool botMemory_t::findPath( gentity_t *self, glm::vec3 &dir )
{
	ASSERT( self && self->client );

	int entityNum = self->s.number;
	class_t pClass = static_cast<class_t>( self->client->ps.stats[STAT_CLASS] );
	glm::vec3 origin = VEC2GLM( self->s.origin );
	glm::vec3 playerMins, playerMaxs;
	BG_BoundingBox( pClass, playerMins, playerMaxs );
	gentity_t const* blocker;

	//step 1: get the planar direction
	dir[ 2 ] = 0;
	dir = glm::normalize( dir );
	glm::vec3 end = origin + BOT_OBSTACLE_AVOID_RANGE * dir;

	//step 2: detect obstacles {
	blocker = getBlocker( STEPSIZE, playerMins, playerMaxs, origin, end, entityNum );
	if ( nullptr == blocker )
	{
		return true;
	}
	// }

	//step 3: try to jump over {
	float jumpMagnitude = BG_Class( pClass )->jumpMagnitude;
	jumpMagnitude = Square( jumpMagnitude ) / ( self->client->ps.gravity * 2 );
	// FIXME: this does not really checks for the 3 correct jump values.
	//TODO FIXME urrrrhggg
	trace_t trace;
	glm::vec3 tstart = origin; tstart[2] += jumpMagnitude;
	glm::vec3 tend   = end   ; tend  [2] += jumpMagnitude;
	glm::vec3 tmins = playerMins; tmins[2] -= STEPSIZE;
	glm::vec3 tmaxs = playerMaxs; tmaxs[2] -= STEPSIZE;
	for ( int i = -1; i <= 1; ++i, tmins[2] += STEPSIZE, tmaxs[2] += STEPSIZE )
	{
		trap_Trace( &trace, tstart, tmins, tmaxs, tend, entityNum, MASK_SHOT, 0 );
		//TODO: this code is fairly broken, since there's no
		//guarantee that the barricade is low enough to be passable
		//though a jump, especially for classes like tyrants
		if ( walkable( trace ) ||
				( G_Team( blocker ) == G_Team( blocker )
					&& blocker->s.modelindex == BA_A_BARRICADE ) )
		{
			BotJump( self );
			return true;
		}
	}
	// }

	//new step 4: try to duck through {
	//TODO: use team-agnostic code to check for crouch possibility
	//      (esp since before bs would not allow crouching)
	glm::vec3 crouchMaxs = BG_CrouchBoundingBox( pClass );
	if ( crouchMaxs != playerMaxs )
	{
		ASSERT( G_Team( self ) != TEAM_ALIENS );
		blocker = getBlocker( STEPSIZE, playerMins, crouchMaxs, origin, end, entityNum );
		if ( nullptr == blocker )
		{
			self->botMind->willCrouch( true );
			return true;
		}
	}

	// }

	//step 4: try to go around it {
	//get the yaw (left/right)
	//(we don't care about up/down since working on horizontal plane only)
	//TODO: This code is even buggier than all previous steps, since
	//it does not verifies crouching, jumping, nor care about stepsize.
	glm::vec3 angles;
	vectoangles( &dir[0], &angles[0] );

	//find an unobstructed position
	//we check the full 180 degrees in front of us
	bool havePath = false;
	glm::vec3 forward;
	for ( float i = 15; i < 90; i += 15 )
	{
		if ( checkWalkable( origin, angles[ YAW ] + i, playerMins, playerMaxs, entityNum, forward ) )
		{
			havePath = true;
			break;
		}

		if ( checkWalkable( origin, angles[ YAW ] - i, playerMins, playerMaxs, entityNum, forward ) )
		{
			havePath = true;
			break;
		}
	}

	if ( havePath )
	{
		dir = forward;
		return true;
	}
	// }

	//step 5: try to find a path anyway {
	glm::vec3 right;
	vectoangles( &dir[0], &angles[0] );
	AngleVectors( &angles[0], &dir[0], &right[0], nullptr );

	if ( ( self->client->time10000 % 2000 ) < 1000 )
	{
		dir = right;
	}
	else
	{
		dir = -right;
	}
	// }
	return false;
}

void BotDirectionToUsercmd( gentity_t *self, const glm::vec3 &dir, usercmd_t *cmd )
{
	glm::vec3 forward;
	glm::vec3 right;

	float forwardmove;
	float rightmove;
	signed char speed = BotGetMaxMoveSpeed( self );

	AngleVectors( VEC2GLM( self->client->ps.viewangles ), &forward, &right, nullptr );
	forward[2] = 0;
	forward = glm::normalize( forward );
	right[2] = 0;
	right = glm::normalize( right );

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
	else if ( rightmove != 0 )
	{
		float highestright = rightmove < 0 ? -speed : speed;

		float highestforward = highestright * forwardmove / rightmove;

		cmd->forwardmove = ClampChar( highestforward );
		cmd->rightmove = ClampChar( highestright );
	}
	else
	{
		cmd->forwardmove = 0;
		cmd->rightmove = 0;
	}
}

// Makes bot aim more or less slowly in a direction
void BotSeek( gentity_t *self, glm::vec3 &direction )
{
	glm::vec3 viewOrigin = BG_GetClientViewOrigin( &self->client->ps );

	direction = glm::normalize( direction );

	// move directly toward the target
	BotDirectionToUsercmd( self, direction, &self->botMind->cmdBuffer );

	glm::vec3 seekPos = viewOrigin + 100.f * direction;

	// slowly change our aim to point to the target
	BotSlowAim( self, seekPos, 0.6 );
	BotAimAtLocation( self, seekPos );
}

/*
=========================
Global Bot Navigation
=========================
*/

void BotMoveToGoal( gentity_t *self )
{
	const playerState_t& ps = self->client->ps;

	glm::vec3 dir = self->botMind->nav().glm_dir();
	if ( !steer( self, dir ) )
	{
		//TODO: find new target on next frame
	}

	dir[ 2 ] = 0;
	dir = glm::normalize( dir );

	BotSeek( self, dir );

	// dumb bots don't know how to be efficient
	if( self->botMind->botSkill.move() < 5 )
	{
		return;
	}

	team_t targetTeam = TEAM_NONE;
	if ( self->botMind->goal.targetsValidEntity() )
	{
		targetTeam = G_Team( self->botMind->goal.getTargetedEntity() );
	}

	// if target is friendly, let's go there quickly (heal, poison) by using trick moves
	// when available (still need to implement wall walking, but that will be more complex)
	if ( G_Team( self ) != targetTeam )
	{
		return;
	}

	weaponMode_t wpm = WPM_NONE;
	int magnitude = 0;
	switch ( ps.stats [ STAT_CLASS ] )
	{
		case PCL_HUMAN_NAKED:
		case PCL_HUMAN_LIGHT:
		case PCL_HUMAN_MEDIUM:
		case PCL_HUMAN_BSUIT:
			BotSprint( self, true );
			break;
		//those classes do not really have capabilities allowing them to be
		//significantly faster while fleeing (except jumps, but that also
		//makes them easier to hit I'd say)
		case PCL_ALIEN_BUILDER0:
		case PCL_ALIEN_BUILDER0_UPG:
		case PCL_ALIEN_LEVEL0:
			break;
		case PCL_ALIEN_LEVEL1:
			if ( ps.weaponCharge <= 50 )//I don't remember why 50
			{
				wpm = WPM_SECONDARY;
				magnitude = LEVEL1_POUNCE_MINPITCH;
			}
			break;
		case PCL_ALIEN_LEVEL2:
		case PCL_ALIEN_LEVEL2_UPG:
			BotJump( self );
			break;
		case PCL_ALIEN_LEVEL3:
			if ( ps.weaponCharge < LEVEL3_POUNCE_TIME )
			{
				wpm = WPM_SECONDARY;
				magnitude = LEVEL3_POUNCE_JUMP_MAG;
			}
		break;
		case PCL_ALIEN_LEVEL3_UPG:
			if ( ps.weaponCharge < LEVEL3_POUNCE_TIME_UPG )
			{
				wpm = WPM_SECONDARY;
				magnitude = LEVEL3_POUNCE_JUMP_MAG_UPG;
			}
			break;
		case PCL_ALIEN_LEVEL4:
			wpm = WPM_SECONDARY;
		break;
	}
	if ( wpm != WPM_NONE )
	{
		usercmd_t &botCmdBuffer = self->botMind->cmdBuffer;
		if ( magnitude )
		{
			glm::vec3 target = self->botMind->nav().glm_tpos();
			botCmdBuffer.angles[PITCH] = ANGLE2SHORT( -CalcAimPitch( self, target, magnitude ) / 3 );
		}
		BotFireWeapon( wpm, &botCmdBuffer );
	}
}

bool FindRouteToTarget( gentity_t *self, botTarget_t target, bool allowPartial )
{
	botRouteTarget_t routeTarget;
	BotTargetToRouteTarget( self, target, &routeTarget );
	return G_BotFindRoute( self->s.number, &routeTarget, allowPartial );
}
