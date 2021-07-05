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

//tells if all navmeshes loaded successfully
bool navMeshLoaded = false;

/*
========================
Navigation Mesh Loading
========================
*/

// FIXME: use nav handle instead of classes
void G_BotNavInit()
{
	int i;

	Log::Notice( "==== Bot Navigation Initialization ==== \n" );

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
				Log::Warn( "class '%s': navmesh reference target class '%s' must have its own navmesh",
				            BG_Class( i )->name, BG_Class( model->navMeshClass )->name );
				return;
			}

			continue;
		}

		Q_strncpyz( bot.name, BG_Class( i )->name, sizeof( bot.name ) );

		if ( !G_BotSetupNav( &bot, &model->navHandle ) )
		{
			return;
		}
	}
	navMeshLoaded = true;
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
	navHandle = model->navMeshClass
	          ? BG_ClassModelConfig( model->navMeshClass )->navHandle
	          : model->navHandle;

	G_BotSetNavMesh( self->s.number, navHandle );
}

/*
========================
Bot Navigation Querys
========================
*/

static float RadiusFromBounds2D( const vec3_t mins, const vec3_t maxs )
{
	float rad1s = Square( mins[0] ) + Square( mins[1] );
	float rad2s = Square( maxs[0] ) + Square( maxs[1] );
	return sqrtf( std::max( rad1s, rad2s ) );
}

float BotGetGoalRadius( const gentity_t *self )
{
	botTarget_t &t = self->botMind->goal;
	if ( t.targetsCoordinates() )
	{
		// we check the coord to be (almost) in our bounding box
		return RadiusFromBounds2D( self->r.mins, self->r.maxs );
	}
	else
	{
		// we don't check if the entity is valid: an outdated result is
		// better than failing here
		const gentity_t *target = t.getTargetedEntity();
		if ( target->s.modelindex == BA_H_MEDISTAT || target->s.modelindex == BA_A_BOOSTER )
		{
			// we want to be quite close to medistat.
			// TODO(freem): is this really what we want for booster?
			return self->r.maxs[0] + target->r.maxs[0];
		}
		else
		{
			return RadiusFromBounds2D( target->r.mins, target->r.maxs ) + RadiusFromBounds2D( self->r.mins, self->r.maxs );
		}
	}
}

bool GoalInRange( const gentity_t *self, float r )
{
	gentity_t *ent = nullptr;
	// we don't need to check the goal is valid here

	if ( self->botMind->goal.targetsCoordinates() )
	{
		return ( Distance( self->s.origin, self->botMind->nav.tpos ) < r );
	}

	while ( ( ent = G_IterateEntitiesWithinRadius( ent, self->s.origin, r ) ) )
	{
		if ( ent == self->botMind->goal.getTargetedEntity() )
		{
			return true;
		}
	}

	return false;
}

int DistanceToGoal2DSquared( const gentity_t *self )
{
	vec3_t vec;
	vec3_t goalPos;

	self->botMind->goal.getPos( goalPos );

	VectorSubtract( goalPos, self->s.origin, vec );

	return Square( vec[ 0 ] ) + Square( vec[ 1 ] );
}

int DistanceToGoal( const gentity_t *self )
{
	vec3_t targetPos;
	vec3_t selfPos;
	//safety check for morons who use this incorrectly
	if ( !( self->botMind ) )
	{
		return -1;
	}
	self->botMind->goal.getPos( targetPos );
	VectorCopy( self->s.origin, selfPos );
	return Distance( selfPos, targetPos );
}

int DistanceToGoalSquared( const gentity_t *self )
{
	vec3_t targetPos;
	vec3_t selfPos;
	//safety check for morons who use this incorrectly
	if ( !( self->botMind ) )
	{
		return -1;
	}
	self->botMind->goal.getPos( targetPos );
	VectorCopy( self->s.origin, selfPos );
	return DistanceSquared( selfPos, targetPos );
}

bool BotPathIsWalkable( const gentity_t *self, botTarget_t target )
{
	vec3_t selfPos, targetPos;
	vec3_t viewNormal;
	botTrace_t trace;

	BG_GetClientNormal( &self->client->ps, viewNormal );
	VectorMA( self->s.origin, self->r.mins[2], viewNormal, selfPos );
	target.getPos( targetPos );

	if ( !G_BotNavTrace( self->s.number, &trace, selfPos, targetPos ) )
	{
		return false;
	}

	return trace.frac >= 1.0f;
}

// TODO: remove
void BotFindRandomPointOnMesh( const gentity_t *self, vec3_t point )
{
	BotFindRandomPoint( self->s.number, point );
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
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;
	int jumpCost = BG_Class( self->client->ps.stats[ STAT_CLASS ] )->staminaJumpCost;
	int stamina = self->client->ps.stats[ STAT_STAMINA ];
	bool sprinting = usercmdButtonPressed( botCmdBuffer->buttons, BUTTON_SPRINT );

	if ( !enable )
	{
		if ( sprinting )
		{
			usercmdReleaseButton( botCmdBuffer->buttons, BUTTON_SPRINT );
			BotWalk( self, true );
		}
		return false;
	}

	if ( self->client->pers.team == TEAM_HUMANS
	     && self->botMind->botSkill.level >= 5 )
	{
		if ( sprinting && stamina <= jumpCost )
		{
			usercmdReleaseButton( botCmdBuffer->buttons, BUTTON_SPRINT );
			BotWalk( self, true );
			return false;
		}
		if ( !sprinting && stamina <= 2 * jumpCost )
		{
			return false;
		}
	}
	usercmdPressButton( botCmdBuffer->buttons, BUTTON_SPRINT );
	BotWalk( self, false );
	return true;
}

void BotWalk( gentity_t *self, bool enable )
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
		return nullptr;
	}

	BG_ClassBoundingBox( ( class_t ) self->client->ps.stats[STAT_CLASS], playerMins, playerMaxs, nullptr, nullptr, nullptr );

	//account for how large we can step
	playerMins[2] += STEPSIZE;
	playerMaxs[2] += STEPSIZE;

	VectorMA( self->s.origin, TRACE_LENGTH, dir, end );

	trap_Trace( &trace, self->s.origin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT, 0 );
	if ( ( trace.fraction < 1.0f && trace.plane.normal[ 2 ] < 0.7f ) || g_entities[ trace.entityNum ].s.eType == entityType_t::ET_BUILDABLE )
	{
		return &g_entities[trace.entityNum];
	}
	return nullptr;
}

bool BotShouldJump( gentity_t *self, gentity_t *blocker, const vec3_t dir )
{
	vec3_t playerMins;
	vec3_t playerMaxs;
	float jumpMagnitude;
	trace_t trace;
	const int TRACE_LENGTH = BOT_OBSTACLE_AVOID_RANGE;
	vec3_t end;

	//blocker is not on our team, so ignore
	if ( G_Team( self ) != G_Team( blocker ) )
	{
		return false;
	}

	//already normalized

	BG_ClassBoundingBox( ( class_t ) self->client->ps.stats[STAT_CLASS], playerMins, playerMaxs, nullptr, nullptr, nullptr );

	playerMins[2] += STEPSIZE;
	playerMaxs[2] += STEPSIZE;

	//Log::Debug(vtos(self->movedir));
	VectorMA( self->s.origin, TRACE_LENGTH, dir, end );

	//make sure we are moving into a block
	trap_Trace( &trace, self->s.origin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT, 0 );
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
	trap_Trace( &trace, self->s.origin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT, 0 );

	//if we can jump over it, then jump
	//note that we also test for a blocking barricade because barricades will collapse to let us through
	return blocker->s.modelindex == BA_A_BARRICADE || trace.fraction == 1.0f;
}

bool BotFindSteerTarget( gentity_t *self, vec3_t dir )
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
		return false;
	}

	//get bbox
	BG_ClassBoundingBox( ( class_t ) self->client->ps.stats[STAT_CLASS], playerMins, playerMaxs, nullptr, nullptr, nullptr );

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
		forward[0] = cosf( DEG2RAD( yaw1 ) );
		forward[1] = sinf( DEG2RAD( yaw1 ) );
		//forward is already normalized
		//try the right
		VectorMA( self->s.origin, BOT_OBSTACLE_AVOID_RANGE, forward, testPoint1 );

		//test it
		trap_Trace( &trace1, self->s.origin, playerMins, playerMaxs, testPoint1, self->s.number,
		            MASK_SHOT, 0 );

		//check if unobstructed
		if ( trace1.fraction >= 1.0f )
		{
			VectorCopy( forward, dir );
			return true;
		}

		//compute forward for left
		forward[0] = cosf( DEG2RAD( yaw2 ) );
		forward[1] = sinf( DEG2RAD( yaw2 ) );
		//forward is already normalized
		//try the left
		VectorMA( self->s.origin, BOT_OBSTACLE_AVOID_RANGE, forward, testPoint2 );

		//test it
		trap_Trace( &trace2, self->s.origin, playerMins, playerMaxs, testPoint2, self->s.number,
		            MASK_SHOT, 0 );

		//check if unobstructed
		if ( trace2.fraction >= 1.0f )
		{
			VectorCopy( forward, dir );
			return true;
		}
	}

	//we couldnt find a new position
	return false;
}
bool BotAvoidObstacles( gentity_t *self, vec3_t dir )
{
	gentity_t *blocker;

	blocker = BotGetPathBlocker( self, dir );

	if ( blocker )
	{
		if ( BotShouldJump( self, blocker, dir ) )
		{
			BotJump( self );
			return false;
		}
		else if ( !BotFindSteerTarget( self, dir ) )
		{
			vec3_t angles;
			vec3_t right;
			vectoangles( dir, angles );
			AngleVectors( angles, dir, right, nullptr );

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
		return true;
	}
	return false;
}

void BotDirectionToUsercmd( gentity_t *self, vec3_t dir, usercmd_t *cmd )
{
	vec3_t forward;
	vec3_t right;

	float forwardmove;
	float rightmove;
	signed char speed = BotGetMaxMoveSpeed( self );

	AngleVectors( self->client->ps.viewangles, forward, right, nullptr );
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
	BG_ClassBoundingBox( self->client->ps.stats[ STAT_CLASS ], mins, maxs, nullptr, nullptr, nullptr );
	trap_Trace( &trace, self->client->ps.origin, mins, maxs, origin, self->client->ps.clientNum,
	            MASK_PLAYERSOLID, 0 );
	G_SetOrigin( self, trace.endpos );
	VectorCopy( trace.endpos, self->client->ps.origin );
}

void BotMoveToGoal( gentity_t *self )
{
	vec3_t dir;
	VectorCopy( self->botMind->nav.dir, dir );

	if ( dir[ 2 ] < 0 )
	{
		dir[ 2 ] = 0;
		VectorNormalize( dir );
	}

	BotAvoidObstacles( self, dir );
	BotSeek( self, dir );

	// dumb bots don't know how to be efficient
	if( self->botMind->botSkill.level < 5 )
	{
		return;
	}

	team_t targetTeam = TEAM_NONE;
	if ( self->botMind->goal.targetsValidEntity() )
	{
		targetTeam = G_Team( self->botMind->goal.getTargetedEntity() );
	}

	usercmd_t &botCmdBuffer = self->botMind->cmdBuffer;
	switch ( self->client->pers.team )
	{
		case TEAM_HUMANS:
			// if target is friendly, reach it quickly
			if ( G_Team( self ) == targetTeam )
			{
				BotSprint( self, true );
			}
			break;
		case TEAM_ALIENS:
			// if target is friendly, let's go there quickly (heal, poison) by using trick moves
			// when available (still need to implement wall walking, but that will be more complex)
			if ( G_Team( self ) == targetTeam )
			{
				switch ( self->s.weapon )
				{
					case WP_ALEVEL1:
						if ( self->client->ps.weaponCharge <= 50 )
						{
							botCmdBuffer.angles[PITCH] = ANGLE2SHORT( -CalcAimPitch( self, self->botMind->goal, LEVEL1_POUNCE_MINPITCH ) / 3 );
							BotFireWeapon( WPM_SECONDARY, &botCmdBuffer );
						}
						break;
					case WP_ALEVEL2:
					case WP_ALEVEL2_UPG:
						BotJump( self );
						break;
					case WP_ALEVEL3:
						if ( self->client->ps.weaponCharge < LEVEL3_POUNCE_TIME )
						{
							botCmdBuffer.angles[PITCH] = ANGLE2SHORT( -CalcAimPitch( self, self->botMind->goal, LEVEL3_POUNCE_JUMP_MAG ) / 3 );
							BotFireWeapon( WPM_SECONDARY, &botCmdBuffer );
						}
						break;
					case WP_ALEVEL3_UPG:
						if ( self->client->ps.weaponCharge < LEVEL3_POUNCE_TIME_UPG )
						{
							botCmdBuffer.angles[PITCH] = ANGLE2SHORT( -CalcAimPitch( self, self->botMind->goal, LEVEL3_POUNCE_JUMP_MAG_UPG ) / 3 );
							BotFireWeapon( WPM_SECONDARY, &botCmdBuffer );
						}
						break;
					case WP_ALEVEL4:
						BotFireWeapon( WPM_SECONDARY, &botCmdBuffer );
						break;
				}
			}
			break;
		default:
			;
	}
}

bool FindRouteToTarget( gentity_t *self, botTarget_t target, bool allowPartial )
{
	botRouteTarget_t routeTarget;
	BotTargetToRouteTarget( self, target, &routeTarget );
	return G_BotFindRoute( self->s.number, &routeTarget, allowPartial );
}
