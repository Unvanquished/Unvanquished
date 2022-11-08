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

#include "shared/bg_local.h"

#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>

//tells if all navmeshes loaded successfully
bool navMeshLoaded = false;

/*
========================
Navigation Mesh Loading
========================
*/

extern void BotAddSavedObstacles();
// FIXME: use nav handle instead of classes
bool G_BotNavInit()
{
	if ( navMeshLoaded )
	{
		return true;
	}

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
	BotAddSavedObstacles();
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
		return RadiusFromBounds2D( self_mins, self_maxs ) + BOT_OBSTACLE_AVOID_RANGE;
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
	       + RadiusFromBounds2D( self_mins, self_maxs ) + BOT_OBSTACLE_AVOID_RANGE;
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

static signed char BotGetMaxMoveSpeed( gentity_t *self )
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

// search for obstacle forward, and return pointer on it if any
static const gentity_t* BotGetPathBlocker( gentity_t *self, const glm::vec3 &dir )
{
	glm::vec3 playerMins, playerMaxs;
	trace_t trace;
	const float TRACE_LENGTH = BOT_OBSTACLE_AVOID_RANGE;

	if ( !( self && self->client ) )
	{
		return nullptr;
	}

	class_t pClass = static_cast<class_t>( self->client->ps.stats[STAT_CLASS] );
	BG_BoundingBox( pClass, &playerMins, &playerMaxs, nullptr, nullptr, nullptr );

	//account for how large we can step
	playerMins[2] += STEPSIZE;
	playerMaxs[2] += STEPSIZE;

	glm::vec3 origin = VEC2GLM( self->s.origin );
	glm::vec3 end = origin + TRACE_LENGTH * dir;

	trap_Trace( &trace, origin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT, 0 );
	if ( ( trace.fraction < 1.0f && trace.plane.normal[ 2 ] < MIN_WALK_NORMAL ) || g_entities[ trace.entityNum ].s.eType == entityType_t::ET_BUILDABLE )
	{
		return &g_entities[trace.entityNum];
	}
	return nullptr;
}

// checks if jumping would get rid of blocker
// return true if yes
static bool BotShouldJump( gentity_t *self, const gentity_t *blocker, const glm::vec3 &dir )
{
	glm::vec3 playerMins;
	glm::vec3 playerMaxs;
	float jumpMagnitude;
	trace_t trace;
	const float TRACE_LENGTH = BOT_OBSTACLE_AVOID_RANGE;

	//blocker is not on our team, so ignore
	if ( G_Team( self ) != G_Team( blocker ) )
	{
		return false;
	}

	//already normalized

	class_t pClass = static_cast<class_t>( self->client->ps.stats[STAT_CLASS] );
	BG_BoundingBox( pClass, &playerMins, &playerMaxs, nullptr, nullptr, nullptr );

	playerMins[2] += STEPSIZE;
	playerMaxs[2] += STEPSIZE;

	//Log::Debug(vtos(self->movedir));
	glm::vec3 origin = VEC2GLM( self->s.origin );
	glm::vec3 end = origin + TRACE_LENGTH * dir;

	//make sure we are moving into a block
	trap_Trace( &trace, origin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT, 0 );
	if ( trace.fraction >= 1.0f || blocker != &g_entities[trace.entityNum] )
	{
		return false;
	}

	jumpMagnitude = BG_Class( ( class_t )self->client->ps.stats[STAT_CLASS] )->jumpMagnitude;

	//find the actual height of our jump
	jumpMagnitude = Square( jumpMagnitude ) / ( self->client->ps.gravity * 2 );

	//prepare for trace
	playerMins[2] += jumpMagnitude;
	playerMaxs[2] += jumpMagnitude;

	//check if jumping will clear us of entity
	trap_Trace( &trace, origin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT, 0 );

	//if we can jump over it, then jump
	//note that we also test for a blocking barricade because barricades will collapse to let us through
	return blocker->s.modelindex == BA_A_BARRICADE || trace.fraction == 1.0f;
}

// Determine a new direction, which may be unchanged (forward) or sideway. It
// does try the following every second: one time left, one time right and one
// time forward until they find their way.
static void BotFindDefaultSteerTarget( gentity_t *self, glm::vec3 &dir )
{
	// turn this into a 2D problem
	dir[2] = 0;
	// a vector 90° from dir, pointing right and of same magnitude
	glm::vec3 right = glm::vec3(dir[1], -dir[0], 0);

	// We add a delta determined from clientNum to avoid bots
	// dancing in sync
	int time = (110 * self->num() + self->client->time1000) % 1000;

	// 280 instead of 700/2=350 because we want some left/right
	// asymetry in case it helps
	bool invert = time < 280;
	// half of the time we go straight instead
	bool sideways = time > 700;

	if ( sideways )
	{
		// 2*right + 1*dir means an angle of atan(2/1) = 63°. This
		// seems about right
		dir += right * (invert ? -1.0f : 1.0f)
			     * 2.0f;
	}
}

// Try to find a path around the obstacle by projecting 5
// traces in 15° steps in both directions.
// Return true if a path could be found.
// This function will always set "dir", even if it fails.
static bool BotFindSteerTarget( gentity_t *self, glm::vec3 &dir )
{
	glm::vec3 forward;
	glm::vec3 testPoint1, testPoint2;
	glm::vec3 playerMins, playerMaxs;
	float yaw1, yaw2;
	trace_t trace1, trace2;
	int i;
	glm::vec3 angles;

	if ( !( self && self->client ) )
	{
		return false;
	}

	//get bbox
	class_t pclass = static_cast<class_t>( self->client->ps.stats[STAT_CLASS] );
	BG_BoundingBox( pclass, &playerMins, &playerMaxs, nullptr, nullptr, nullptr );

	//account for stepsize
	playerMins[2] += STEPSIZE;
	playerMaxs[2] += STEPSIZE;

	//get the yaw (left/right) we dont care about up/down
	vectoangles( &dir[0], &angles[0] );
	yaw1 = yaw2 = angles[ YAW ];

	//directly infront of us is blocked, so dont test it
	yaw1 -= 15;
	yaw2 += 15;

	//forward vector is 2D
	forward[ 2 ] = 0;

	//find an unobstructed position
	//we check the full 180 degrees in front of us
	glm::vec3 origin = VEC2GLM( self->s.origin );
	for ( i = 0; i < 5; i++, yaw1 -= 15 , yaw2 += 15 )
	{
		//compute forward for right
		forward[0] = cosf( DEG2RAD( yaw1 ) );
		forward[1] = sinf( DEG2RAD( yaw1 ) );
		//forward is already normalized
		//try the right
		testPoint1 = origin + BOT_OBSTACLE_AVOID_RANGE * forward;

		//test it
		trap_Trace( &trace1, origin, playerMins, playerMaxs, testPoint1, self->s.number,
		            MASK_SHOT, 0 );

		//check if unobstructed
		if ( trace1.fraction >= 1.0f )
		{
			dir = forward;
			return true;
		}

		//compute forward for left
		forward[0] = cosf( DEG2RAD( yaw2 ) );
		forward[1] = sinf( DEG2RAD( yaw2 ) );
		//forward is already normalized
		//try the left
		testPoint2 = origin + BOT_OBSTACLE_AVOID_RANGE * forward;

		//test it
		trap_Trace( &trace2, origin, playerMins, playerMaxs, testPoint2, self->s.number,
		            MASK_SHOT, 0 );

		//check if unobstructed
		if ( trace2.fraction >= 1.0f )
		{
			dir = forward;
			return true;
		}
	}

	// We couldnt find a "smart" new position, try going forward or sideways
	BotFindDefaultSteerTarget( self, dir );
	return false;
}

// This function tries to detect obstacles and to find a way
// around them. It always sets dir as the output value.
// Returns true on error
static bool BotAvoidObstacles( gentity_t *self, glm::vec3 &dir )
{
	dir = self->botMind->nav().glm_dir();
	gentity_t const *blocker = BotGetPathBlocker( self, dir );

	if ( !blocker )
	{
		return false;
	}

	// ignore some stuff like geometry, movers...
	switch( blocker->s.eType )
	{
		case entityType_t::ET_GENERAL:
		case entityType_t::ET_MOVER:
			return false;
		default:
			break;
	}
	if ( BotShouldJump( self, blocker, dir ) )
	{
		BotJump( self );
		return false;
	}

	if ( BotFindSteerTarget( self, dir ) )
	{
		return false;
	}

	return true;
}

static void BotDirectionToUsercmd( gentity_t *self, const glm::vec3 &dir, usercmd_t *cmd )
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
	forwardmove = speed * glm::dot( forward, dir );
	rightmove = speed * glm::dot( right, dir );

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
static void BotSeek( gentity_t *self, glm::vec3 &direction )
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

// This function makes the bot move and aim at it's goal, trying
// to move faster if the skill is high enough, depending on it's
// class.
// Returns false on failure.
bool BotMoveToGoal( gentity_t *self )
{
	glm::vec3 dir;
	if ( BotAvoidObstacles( self, dir ) )
	{
		BotSeek( self, dir );
		return false;
	}

	BotSeek( self, dir );

	// dumb bots don't know how to be efficient
	if( self->botMind->botSkill.level < 5 )
	{
		return true;
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
		return true;
	}

	weaponMode_t wpm = WPM_NONE;
	int magnitude = 0;
	const playerState_t& ps  = self->client->ps;
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
	return true;
}

bool FindRouteToTarget( gentity_t *self, botTarget_t target, bool allowPartial )
{
	botRouteTarget_t routeTarget;
	BotTargetToRouteTarget( self, target, &routeTarget );
	return G_BotFindRoute( self->s.number, &routeTarget, allowPartial );
}
