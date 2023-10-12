/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "sg_local.h"
#include "sg_spawn.h"
#include "Entities.h"
#include "CBSE.h"
#include "sg_cm_world.h"

#include <string>

#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/range.hpp>

#define DEFAULT_FUNC_TRAIN_SPEED 100

static Cvar::Range<Cvar::Cvar<int>> g_bot_ignoreSmallObstacles(
		"g_bot_ignoreSmallObstacles",
		"this allows bots to ignore \"negligible\" navmesh blockers."
		"0 is disabled. Should only be enabled on maps with known problem."
		"Maximum is 18, which is bots' stepsize.", Cvar::NONE, 0, 0, 18 );

constexpr int ACTIVATE = 0x80; // spawnflags with this bit can be triggered by `+activate`
/*
===============================================================================

PUSHMOVE

===============================================================================
*/

struct pushed_t
{
	gentity_t *ent;
	glm::vec3 origin;
	glm::vec3 angles;
	float     deltayaw;
};

pushed_t pushed[ MAX_GENTITIES ], *pushed_p;

/*
============
G_TestEntityPosition

============
*/
static gentity_t *G_TestEntityPosition( gentity_t const *ent )
{
	trace_t tr;

	if ( ent->client )
	{
		G_CM_Trace( &tr, VEC2GLM( ent->client->ps.origin ), VEC2GLM( ent->r.mins ), VEC2GLM( ent->r.maxs ), VEC2GLM( ent->client->ps.origin ), ent->num(), ent->clipmask, 0, traceType_t::TT_AABB );
	}
	else
	{
		G_CM_Trace( &tr, VEC2GLM( ent->s.pos.trBase ), VEC2GLM( ent->r.mins ), VEC2GLM( ent->r.maxs ), VEC2GLM( ent->s.pos.trBase ), ent->num(), ent->clipmask, 0, traceType_t::TT_AABB );
	}

	return tr.startsolid ? &g_entities[ tr.entityNum ] : nullptr;
}

static bool G_SpawnVector( const char *key, const char *defaultString, glm::vec3& out )
{
	const char     *s;
	bool present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[ 0 ], &out[ 1 ], &out[ 2 ] );
	return present;
}

/*
================
G_CreateRotationMatrix
================
*/
static void G_CreateRotationMatrix( glm::vec3 const& angles, vec3_t matrix[ 3 ] )
{
	AngleVectors( &angles[0], matrix[ 0 ], matrix[ 1 ], matrix[ 2 ] );
	VectorInverse( matrix[ 1 ] );
}

/*
================
G_TransposeMatrix
================
*/
static void G_TransposeMatrix( vec3_t matrix[ 3 ], vec3_t transpose[ 3 ] )
{
	int i, j;

	for ( i = 0; i < 3; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			transpose[ i ][ j ] = matrix[ j ][ i ];
		}
	}
}

/*
================
G_RotatePoint
================
*/
static void G_RotatePoint( glm::vec3 & point, vec3_t matrix[ 3 ] )
{
	glm::vec3 tvec = point;
	point[ 0 ] = glm::dot( VEC2GLM( matrix[ 0 ] ), tvec );
	point[ 1 ] = glm::dot( VEC2GLM( matrix[ 1 ] ), tvec );
	point[ 2 ] = glm::dot( VEC2GLM( matrix[ 2 ] ), tvec );
}

/*
==================
G_TryPushingEntity

Returns false if the move is blocked
==================
*/
static bool G_TryPushingEntity( gentity_t *check, gentity_t *pusher, glm::vec3 const& move, glm::vec3 const& amove )
{
	vec3_t    matrix[ 3 ], transpose[ 3 ];

	// EF_MOVER_STOP will just stop when contacting another entity
	// instead of pushing it, but entities can still ride on top of it
	if ( ( pusher->s.eFlags & EF_MOVER_STOP ) &&
	     check->s.groundEntityNum != pusher->num() )
	{
		return false;
	}

	//don't try to move buildables unless standing on a mover
	if ( check->s.eType == entityType_t::ET_BUILDABLE &&
	     check->s.groundEntityNum != pusher->num() )
	{
		return false;
	}

	// save off the old position
	if ( pushed_p > &pushed[ MAX_GENTITIES ] )
	{
		Sys::Drop( "pushed_p > &pushed[MAX_GENTITIES]" );
	}

	pushed_p->ent = check;
	VectorCopy( check->s.pos.trBase, pushed_p->origin );
	VectorCopy( check->s.apos.trBase, pushed_p->angles );

	if ( check->client )
	{
		pushed_p->deltayaw = check->client->ps.delta_angles[ YAW ];
		VectorCopy( check->client->ps.origin, pushed_p->origin );
	}

	pushed_p++;

	// try moving the contacted entity
	// figure movement due to the pusher's amove
	G_CreateRotationMatrix( amove, transpose );
	G_TransposeMatrix( transpose, matrix );

	glm::vec3 org, org2, move2;
	if ( check->client )
	{
		org = VEC2GLM( check->client->ps.origin ) - VEC2GLM( pusher->r.currentOrigin );
	}
	else
	{
		org = VEC2GLM( check->s.pos.trBase ) - VEC2GLM( pusher->r.currentOrigin );
	}

	org2 = org;
	G_RotatePoint( org2, matrix );
	move2 = org2 - org;
	// add movement
	VectorAdd( check->s.pos.trBase, move, check->s.pos.trBase );
	VectorAdd( check->s.pos.trBase, move2, check->s.pos.trBase );

	if ( check->client )
	{
		VectorAdd( check->client->ps.origin, move, check->client->ps.origin );
		VectorAdd( check->client->ps.origin, move2, check->client->ps.origin );
		// make sure the client's view rotates when on a rotating mover
		check->client->ps.delta_angles[ YAW ] += ANGLE2SHORT( amove[ YAW ] );
	}

	// may have pushed them off an edge
	if ( check->s.groundEntityNum != pusher->num() )
	{
		check->s.groundEntityNum = ENTITYNUM_NONE;
	}

	gentity_t* block = G_TestEntityPosition( check );

	if ( !block )
	{
		// pushed ok
		if ( check->client )
		{
			VectorCopy( check->client->ps.origin, check->r.currentOrigin );
		}
		else
		{
			VectorCopy( check->s.pos.trBase, check->r.currentOrigin );
		}

		G_CM_LinkEntity( check );
		return true;
	}

	// if it is ok to leave in the old position, do it
	// this is only relevent for riding entities, not pushed
	// Sliding trapdoors can cause this.
	VectorCopy( ( pushed_p - 1 )->origin, check->s.pos.trBase );

	if ( check->client )
	{
		VectorCopy( ( pushed_p - 1 )->origin, check->client->ps.origin );
	}

	VectorCopy( ( pushed_p - 1 )->angles, check->s.apos.trBase );
	block = G_TestEntityPosition( check );

	if ( !block )
	{
		check->s.groundEntityNum = ENTITYNUM_NONE;
		pushed_p--;
		return true;
	}

	// blocked
	return false;
}

/*
============
G_MoverPush

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
If false is returned, *obstacle will be the blocking entity
============
*/
static bool G_MoverPush( gentity_t *pusher, glm::vec3 const& move, glm::vec3 const& amove, gentity_t **obstacle )
{
	int       entityList[ MAX_GENTITIES ];
	glm::vec3 mins, maxs;
	glm::vec3 totalMins, totalMaxs;

	*obstacle = nullptr;

	// mins/maxs are the bounds at the destination
	// totalMins / totalMaxs are the bounds for the entire move
	if ( pusher->r.currentAngles[ 0 ]
			|| pusher->r.currentAngles[ 1 ]
			|| pusher->r.currentAngles[ 2 ]
			|| amove[ 0 ]
			|| amove[ 1 ]
			|| amove[ 2 ] )
	{
		float radius = RadiusFromBounds( pusher->r.mins, pusher->r.maxs );

		mins = VEC2GLM( pusher->r.currentOrigin ) + move - radius;
		maxs = VEC2GLM( pusher->r.currentOrigin ) + move + radius;
		totalMins = mins - move;
		totalMaxs = maxs - move;
	}
	else
	{
		mins = VEC2GLM( pusher->r.absmin ) + move;
		maxs = VEC2GLM( pusher->r.absmax ) + move;
		totalMins = VEC2GLM( pusher->r.absmin );
		totalMaxs = VEC2GLM( pusher->r.absmax );

		for ( int i = 0; i < 3; i++ )
		{
			if ( move[ i ] > 0 )
			{
				totalMaxs[ i ] += move[ i ];
			}
			else
			{
				totalMins[ i ] += move[ i ];
			}
		}
	}

	// unlink the pusher so we don't get it in the entityList
	G_CM_UnlinkEntity( pusher );

	int listedEntities = G_CM_AreaEntities( totalMins, totalMaxs, entityList, MAX_GENTITIES );

	// move the pusher to its final position
	VectorAdd( pusher->r.currentOrigin, move, pusher->r.currentOrigin );
	VectorAdd( pusher->r.currentAngles, amove, pusher->r.currentAngles );
	G_CM_LinkEntity( pusher );

	// see if any solid entities are inside the final position
	for ( int e = 0; e < listedEntities; e++ )
	{
		gentity_t *check = &g_entities[ entityList[ e ] ];

		// only push items and players
		if ( check->s.eType != entityType_t::ET_ITEM && check->s.eType != entityType_t::ET_BUILDABLE &&
		     check->s.eType != entityType_t::ET_CORPSE && check->s.eType != entityType_t::ET_PLAYER &&
		     !check->physicsObject )
		{
			continue;
		}

		// if the entity is standing on the pusher, it will definitely be moved
		if ( check->s.groundEntityNum != pusher->num() )
		{
			// see if the ent needs to be tested
			if ( check->r.absmin[ 0 ] >= maxs[ 0 ]
			     || check->r.absmin[ 1 ] >= maxs[ 1 ]
			     || check->r.absmin[ 2 ] >= maxs[ 2 ]
			     || check->r.absmax[ 0 ] <= mins[ 0 ]
			     || check->r.absmax[ 1 ] <= mins[ 1 ]
			     || check->r.absmax[ 2 ] <= mins[ 2 ] )
			{
				continue;
			}

			// see if the ent's bbox is inside the pusher's final position
			// this does allow a fast moving object to pass through a thin entity...
			if ( !G_TestEntityPosition( check ) )
			{
				continue;
			}
		}

		// the entity needs to be pushed
		if ( G_TryPushingEntity( check, pusher, move, amove ) )
		{
			continue;
		}

		// the move was blocked an entity

		// bobbing entities are instant-kill and never get blocked
		if ( pusher->s.pos.trType == trType_t::TR_SINE || pusher->s.apos.trType == trType_t::TR_SINE )
		{
			Entities::Kill(check, pusher, MOD_CRUSH);
			continue;
		}

		// save off the obstacle so we can call the block function (crush, etc)
		*obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for ( pushed_t* p = pushed_p - 1; p >= pushed; p-- )
		{
			VectorCopy( p->origin, p->ent->s.pos.trBase );
			VectorCopy( p->angles, p->ent->s.apos.trBase );

			if ( p->ent->client )
			{
				p->ent->client->ps.delta_angles[ YAW ] = p->deltayaw;
				VectorCopy( p->origin, p->ent->client->ps.origin );
			}

			G_CM_LinkEntity( p->ent );
		}

		return false;
	}

	return true;
}

/*
=================
G_MoverGroup
=================
*/
static void G_MoverGroup( gentity_t *ent )
{
	gentity_t *obstacle = nullptr;

	// make sure all group slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_p = pushed;

	gentity_t* part = ent;
	for ( ; part; part = part->mapEntity.groupChain )
	{
		if ( part->s.pos.trType == trType_t::TR_STATIONARY &&
		     part->s.apos.trType == trType_t::TR_STATIONARY )
		{
			continue;
		}

		// get current position
		glm::vec3 origin = BG_EvaluateTrajectory( &part->s.pos, level.time );
		glm::vec3 angles = BG_EvaluateTrajectory( &part->s.apos, level.time );
		glm::vec3 move = origin - VEC2GLM( part->r.currentOrigin );
		glm::vec3 amove = angles - VEC2GLM( part->r.currentAngles );

		if ( !G_MoverPush( part, move, amove, &obstacle ) )
		{
			break; // move was blocked
		}
	}

	if ( part )
	{
		// go back to the previous position
		for ( part = ent; part; part = part->mapEntity.groupChain )
		{
			if ( part->s.pos.trType == trType_t::TR_STATIONARY &&
			     part->s.apos.trType == trType_t::TR_STATIONARY )
			{
				continue;
			}

			part->s.pos.trTime += level.time - level.previousTime;
			part->s.apos.trTime += level.time - level.previousTime;
			BG_EvaluateTrajectory( &part->s.pos, level.time, part->r.currentOrigin );
			BG_EvaluateTrajectory( &part->s.apos, level.time, part->r.currentAngles );
			G_CM_LinkEntity( part );
		}

		// if the pusher has a "blocked" function, call it
		if ( ent->mapEntity.blocked )
		{
			ent->mapEntity.blocked( ent, obstacle );
		}

		return;
	}

	// the move succeeded
	for ( part = ent; part; part = part->mapEntity.groupChain )
	{
		// call the reached function if time is at or past end point
		if ( part->mapEntity.reached )
		{
			if ( part->s.pos.trType == trType_t::TR_LINEAR_STOP
					&& level.time >= part->s.pos.trTime + part->s.pos.trDuration )
			{
				part->mapEntity.reached( part );
			}

			if ( part->s.apos.trType == trType_t::TR_LINEAR_STOP
					&& level.time >= part->s.apos.trTime + part->s.apos.trDuration )
			{
				part->mapEntity.reached( part );
			}
		}
	}
}

/*
================
G_RunMover

================
*/
void G_RunMover( gentity_t *ent )
{
	// if not a groupmaster, don't do anything, because
	// the master will handle everything
	if ( ent->flags & FL_GROUPSLAVE )
	{
		return;
	}

	G_MoverGroup( ent );

	// check think function
	G_RunThink( ent );
}

/*
============================================================================

GENERAL MOVERS

Doors, plats, and buttons are all binary (two position) movers
Pos1 is "at rest", pos2 is "activated"
============================================================================
*/

/*
===============
SetMoverState
===============
*/

static bool IsDoor( const gentity_t *ent )
{
	return 0 == Q_strnicmp( "func_door", ent->classname.c_str(), strlen( "func_door" ) );
}

// a mover is automatic if:
//
// * it does not react to damages
// * it has no targetName nor aliases
// * it has no master or it's master is automatic
//
// TODO: sometimes targetName or aliases are abused by
// mappers, especially the "name" alias. This break bots
// for now, but not sure how this should be handled.
static bool IsAutomaticMover( const gentity_t *ent )
{
	return true
		&& ent->mapEntity.config.health == 0
		&& ent->mapEntity.names[0].empty()
		// is group master or group master is auto
		&& ( ( ent->flags & FL_GROUPSLAVE ) == 0 || IsAutomaticMover( ent->mapEntity.groupMaster ) )
		;
}

// This function adds obstacles for doors that the bot can't open,
// that is doors that are not automatic, or doors that are already opened.
static void BotHandleDoor( gentity_t *ent, moverState_t moverState )
{
	if ( !IsDoor( ent ) || IsAutomaticMover( ent ) )
	{
		return;
	}
	glm::vec3 mins = VEC2GLM( ent->r.absmin );
	glm::vec3 maxs = VEC2GLM( ent->r.absmax );
	float smallObstacle = g_bot_ignoreSmallObstacles.Get();
	// this is a hack that allows to ignore "irrelevant" doors,
	// implemented for nova's electric fence support
	switch ( moverState )
	{
		case MOVER_POS1:
		case MOVER_POS2:
		case ROTATOR_POS1:
		case ROTATOR_POS2:
			{
				glm::vec3 absdiff = glm::abs( ent->mapEntity.activatedPosition - ent->mapEntity.restingPosition );
				if ( glm::any( glm::greaterThan( absdiff, glm::vec3( smallObstacle ) ) ) )
				{
					G_BotRemoveObstacle( ent->num() );
					G_BotAddObstacle( mins, maxs, ent->num() );
				}
			}
			break;

		case MODEL_POS1: // TODO: have bots treat them as obstacles too, because they *are* obstacles.
		case MODEL_POS2: // TODO: have bots treat them as obstacles too, because they *are* obstacles.
		case ROTATOR_1TO2:
		case ROTATOR_2TO1:
		case MOVER_1TO2:
		case MOVER_2TO1:
		default:
			break;
	}
}

static void SetMoverState( gentity_t *ent, moverState_t moverState, int time )
{
	glm::vec3 delta;
	float  f;

	ent->mapEntity.moverState = moverState;

	ent->s.pos.trTime = time;
	ent->s.apos.trTime = time;

	switch ( moverState )
	{
		case MOVER_POS1:
			VectorCopy( ent->mapEntity.restingPosition, ent->s.pos.trBase );
			ent->s.pos.trType = trType_t::TR_STATIONARY;
			break;

		case MOVER_POS2:
			VectorCopy( ent->mapEntity.activatedPosition, ent->s.pos.trBase );
			ent->s.pos.trType = trType_t::TR_STATIONARY;
			break;

		case MOVER_1TO2:
			VectorCopy( ent->mapEntity.restingPosition, ent->s.pos.trBase );
			delta = ent->mapEntity.activatedPosition - ent->mapEntity.restingPosition;
			f = 1000.0 / ent->s.pos.trDuration;
			VectorScale( delta, f, ent->s.pos.trDelta );
			ent->s.pos.trType = trType_t::TR_LINEAR_STOP;
			break;

		case MOVER_2TO1:
			VectorCopy( ent->mapEntity.activatedPosition, ent->s.pos.trBase );
			delta = ent->mapEntity.restingPosition - ent->mapEntity.activatedPosition;
			f = 1000.0 / ent->s.pos.trDuration;
			VectorScale( delta, f, ent->s.pos.trDelta );
			ent->s.pos.trType = trType_t::TR_LINEAR_STOP;
			break;

		case ROTATOR_POS1:
			VectorCopy( ent->mapEntity.restingPosition, ent->s.apos.trBase );
			ent->s.apos.trType = trType_t::TR_STATIONARY;
			break;

		case ROTATOR_POS2:
			VectorCopy( ent->mapEntity.activatedPosition, ent->s.apos.trBase );
			ent->s.apos.trType = trType_t::TR_STATIONARY;
			break;

		case ROTATOR_1TO2:
			VectorCopy( ent->mapEntity.restingPosition, ent->s.apos.trBase );
			delta = ent->mapEntity.activatedPosition - ent->mapEntity.restingPosition;
			f = 1000.0 / ent->s.apos.trDuration;
			VectorScale( delta, f, ent->s.apos.trDelta );
			ent->s.apos.trType = trType_t::TR_LINEAR_STOP;
			break;

		case ROTATOR_2TO1:
			VectorCopy( ent->mapEntity.activatedPosition, ent->s.apos.trBase );
			delta = ent->mapEntity.restingPosition - ent->mapEntity.activatedPosition;
			f = 1000.0 / ent->s.apos.trDuration;
			VectorScale( delta, f, ent->s.apos.trDelta );
			ent->s.apos.trType = trType_t::TR_LINEAR_STOP;
			break;

		// TODO: have bots treat them as obstacles too, because they *are* obstacles.
		case MODEL_POS1:
			break;

		case MODEL_POS2:
			break;

		default:
			break;
	}

	if ( moverState >= MOVER_POS1 && moverState <= MOVER_2TO1 )
	{
		BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->r.currentOrigin );
	}

	if ( moverState >= ROTATOR_POS1 && moverState <= ROTATOR_2TO1 )
	{
		BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles );
	}

	G_CM_LinkEntity( ent );

	BotHandleDoor( ent, moverState );
}

/*
================
MatchGroup

All entities in a mover group will move from pos1 to pos2
================
*/
static void MatchGroup( gentity_t *groupLeader, int moverState, int time )
{
	for ( gentity_t* slave = groupLeader; slave; slave = slave->mapEntity.groupChain )
	{
		SetMoverState( slave, (moverState_t) moverState, time );
	}
}

/*
================
MasterOf
================
*/
static gentity_t *MasterOf( gentity_t *ent )
{
	return ent->mapEntity.groupMaster ? ent->mapEntity.groupMaster : ent;
}

/*
================
GetMoverGroupState

Returns a MOVER_* value representing the phase (either one
 of pos1, 1to2, pos2, or 2to1) of a mover group as a whole.
================
*/
static moverState_t GetMoverGroupState( gentity_t *ent )
{
	bool restingPosition = false;

	for ( ent = MasterOf( ent ); ent; ent = ent->mapEntity.groupChain )
	{
		if ( ent->mapEntity.moverState == MOVER_POS1 || ent->mapEntity.moverState == ROTATOR_POS1 )
		{
			restingPosition = true;
		}
		else if ( ent->mapEntity.moverState == MOVER_1TO2 || ent->mapEntity.moverState == ROTATOR_1TO2 )
		{
			return MOVER_1TO2;
		}
		else if ( ent->mapEntity.moverState == MOVER_2TO1 || ent->mapEntity.moverState == ROTATOR_2TO1 )
		{
			return MOVER_2TO1;
		}
	}

	return restingPosition ? MOVER_POS1 : MOVER_POS2;
}

/*
================
ReturnToPos1orApos1

Used only by a master movers.
================
*/

void BinaryMover_act( gentity_t *ent, gentity_t *other, gentity_t *activator );

static void ReturnToPos1orApos1( gentity_t *ent )
{
	if ( GetMoverGroupState( ent ) != MOVER_POS2 )
	{
		return; // not every mover in the group has reached its endpoint yet
	}

	BinaryMover_act( ent, ent, ent->activator );
}

/*
================
Think_ClosedModelDoor
================
*/
static void Think_ClosedModelDoor( gentity_t *ent )
{
	// play sound
	if ( ent->mapEntity.soundPos1 )
	{
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.soundPos1 );
	}

	// close areaportals
	if ( ent->mapEntity.groupMaster == ent || !ent->mapEntity.groupMaster )
	{
		trap_AdjustAreaPortalState( ent, false );
	}

	ent->mapEntity.moverState = MODEL_POS1;
}

/*
================
Think_CloseModelDoor
================
*/
static void Think_CloseModelDoor( gentity_t *ent )
{
	int       entityList[ MAX_GENTITIES ];
	gentity_t *clipBrush = ent->mapEntity.clipBrush;

	int numEntities = G_CM_AreaEntities( VEC2GLM( clipBrush->r.absmin ), VEC2GLM( clipBrush->r.absmax ), entityList, MAX_GENTITIES );

	//set brush solid
	G_CM_LinkEntity( ent->mapEntity.clipBrush );

	//see if any solid entities are inside the door
	for ( int i = 0; i < numEntities; i++ )
	{
		gentity_t *check = &g_entities[ entityList[ i ] ];

		//only test items and players
		if ( check->s.eType != entityType_t::ET_ITEM && check->s.eType != entityType_t::ET_BUILDABLE &&
		     check->s.eType != entityType_t::ET_CORPSE && check->s.eType != entityType_t::ET_PLAYER &&
		     !check->physicsObject )
		{
			continue;
		}

		//test is this entity collides with this door
		if ( G_TestEntityPosition( check ) )
		{
			//something is blocking this door
			//set brush non-solid
			G_CM_UnlinkEntity( ent->mapEntity.clipBrush );

			ent->nextthink = level.time + ent->mapEntity.config.wait.time;
			return;
		}
	}

	//toggle door state
	ent->s.legsAnim = false;

	// play sound
	if ( ent->mapEntity.sound2to1 )
	{
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.sound2to1 );
	}

	ent->mapEntity.moverState = MODEL_2TO1;

	ent->think = Think_ClosedModelDoor;
	ent->nextthink = level.time + ent->mapEntity.config.speed;
}

/*
================
Think_OpenModelDoor
================
*/
static void Think_OpenModelDoor( gentity_t *ent )
{
	//set brush non-solid
	G_CM_UnlinkEntity( ent->mapEntity.clipBrush );

	// stop the looping sound
	ent->s.loopSound = 0;

	// play sound
	if ( ent->mapEntity.soundPos2 )
	{
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.soundPos2 );
	}

	ent->mapEntity.moverState = MODEL_POS2;

	// return to pos1 after a delay
	ent->think = Think_CloseModelDoor;
	ent->nextthink = level.time + ent->mapEntity.config.wait.time;

	// fire targets
	if ( !ent->activator )
	{
		ent->activator = ent;
	}

	G_FireEntity( ent, ent->activator );
}

/*
================
Reached_BinaryMover
================
*/
static void BinaryMover_reached( gentity_t *ent )
{
	gentity_t *master = MasterOf( ent );

	// stop the looping sound
	ent->s.loopSound = 0;

	if ( ent->mapEntity.moverState == MOVER_1TO2 )
	{
		// reached pos2
		SetMoverState( ent, MOVER_POS2, level.time );

		// play sound
		if ( ent->mapEntity.soundPos2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.soundPos2 );
		}

		// return to pos1 after a delay
		master->think = ReturnToPos1orApos1;
		master->nextthink = std::max( master->nextthink, level.time + (int) ent->mapEntity.config.wait.time );

		// fire targets
		if ( !ent->activator )
		{
			ent->activator = ent;
		}

		G_FireEntity( ent, ent->activator );
	}
	else if ( ent->mapEntity.moverState == MOVER_2TO1 )
	{
		// reached pos1
		SetMoverState( ent, MOVER_POS1, level.time );

		// play sound
		if ( ent->mapEntity.soundPos1 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.soundPos1 );
		}

		// close areaportals
		if ( ent->mapEntity.groupMaster == ent || !ent->mapEntity.groupMaster )
		{
			trap_AdjustAreaPortalState( ent, false );
		}
	}
	else if ( ent->mapEntity.moverState == ROTATOR_1TO2 )
	{
		// reached pos2
		SetMoverState( ent, ROTATOR_POS2, level.time );

		// play sound
		if ( ent->mapEntity.soundPos2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.soundPos2 );
		}

		// return to apos1 after a delay
		master->think = ReturnToPos1orApos1;
		master->nextthink = std::max( master->nextthink, level.time + static_cast<int>( ent->mapEntity.config.wait.time ) );

		// fire targets
		if ( !ent->activator )
		{
			ent->activator = ent;
		}

		G_FireEntity( ent, ent->activator );
	}
	else if ( ent->mapEntity.moverState == ROTATOR_2TO1 )
	{
		// reached pos1
		SetMoverState( ent, ROTATOR_POS1, level.time );

		// play sound
		if ( ent->mapEntity.soundPos1 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.soundPos1 );
		}

		// close areaportals
		if ( ent->mapEntity.groupMaster == ent || !ent->mapEntity.groupMaster )
		{
			trap_AdjustAreaPortalState( ent, false );
		}
	}
	else
	{
		Sys::Drop( "Reached_BinaryMover: bad moverState" );
	}
}

/*
================
Use_BinaryMover
================
*/
void BinaryMover_act( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	int total;
	int partial;
	gentity_t *master;
	moverState_t groupState;

	// if this is a non-client-usable door return
	if ( ent->mapEntity.names[ 0 ].size() && other && other->client )
	{
		return;
	}

	if ( ent->mapEntity.locked )
	{
		if ( activator->client != nullptr
		     && activator->client->lastLockWarnTime + 3000 < level.time )
		{
			activator->client->lastLockWarnTime = level.time;
			CPx( activator->num(), "cp_tr " QQ(N_("^1This door is locked!")) );
		}
		return;
	}

	// only the master should be used
	if ( ent->flags & FL_GROUPSLAVE )
	{
		BinaryMover_act( ent->mapEntity.groupMaster, other, activator );
		return;
	}

	ent->activator = activator;

	master = MasterOf( ent );
	groupState = GetMoverGroupState( ent );

	for ( ent = master; ent; ent = ent->mapEntity.groupChain )
	{
	//ind
	if ( ent->mapEntity.moverState == MOVER_POS1 )
	{
		// start moving 50 msec later, because if this was player-
		// triggered, level.time hasn't been advanced yet
		SetMoverState( ent, MOVER_1TO2, level.time + 50 );

		// starting sound
		if ( ent->mapEntity.sound1to2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.sound1to2 );
		}

		// looping sound
		ent->s.loopSound = ent->mapEntity.soundIndex;

		// open areaportal
		if ( ent->mapEntity.groupMaster == ent || !ent->mapEntity.groupMaster )
		{
			trap_AdjustAreaPortalState( ent, true );
		}
	}
	else if ( ent->mapEntity.moverState == MOVER_POS2 &&
	          !( groupState == MOVER_1TO2 || other == master ) )
	{
		// if all the way up, just delay before coming down
		master->think = ReturnToPos1orApos1;
		master->nextthink = std::max( master->nextthink, level.time + (int) ent->mapEntity.config.wait.time );
	}
	else if ( ent->mapEntity.moverState == MOVER_POS2 &&
	          ( groupState == MOVER_1TO2 || other == master ) )
	{
		// start moving 50 msec later, because if this was player-
		// triggered, level.time hasn't been advanced yet
		SetMoverState( ent, MOVER_2TO1, level.time + 50 );

		// starting sound
		if ( ent->mapEntity.sound2to1 )
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.sound2to1 );

		// looping sound
		ent->s.loopSound = ent->mapEntity.soundIndex;

		// open areaportal
		if ( ent->mapEntity.groupMaster == ent || !ent->mapEntity.groupMaster )
			trap_AdjustAreaPortalState( ent, true );

		//HACK: this is really ugly, should have specialise code,
		//but I'm both lazy, buzy to fix old mess, and annoyed by
		//said years-old mess
		ent->health = ent->mapEntity.config.health;
	}
	else if ( ent->mapEntity.moverState == MOVER_2TO1 )
	{
		// only partway down before reversing
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.pos.trTime;

		if ( partial > total )
		{
			partial = total;
		}

		SetMoverState( ent, MOVER_1TO2, level.time - ( total - partial ) );

		if ( ent->mapEntity.sound1to2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.sound1to2 );
		}
	}
	else if ( ent->mapEntity.moverState == MOVER_1TO2 )
	{
		// only partway up before reversing
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.pos.trTime;

		if ( partial > total )
		{
			partial = total;
		}

		SetMoverState( ent, MOVER_2TO1, level.time - ( total - partial ) );

		if ( ent->mapEntity.sound2to1 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.sound2to1 );
		}
	}
	else if ( ent->mapEntity.moverState == ROTATOR_POS1 )
	{
		// start moving 50 msec later, because if this was player-
		// triggered, level.time hasn't been advanced yet
		SetMoverState( ent, ROTATOR_1TO2, level.time + 50 );

		// starting sound
		if ( ent->mapEntity.sound1to2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.sound1to2 );
		}

		// looping sound
		ent->s.loopSound = ent->mapEntity.soundIndex;

		// open areaportal
		if ( ent->mapEntity.groupMaster == ent || !ent->mapEntity.groupMaster )
		{
			trap_AdjustAreaPortalState( ent, true );
		}
	}
	else if ( ent->mapEntity.moverState == ROTATOR_POS2 &&
	          !( groupState == MOVER_1TO2 || other == master ) )
	{
		// if all the way up, just delay before coming down
		master->think = ReturnToPos1orApos1;
		master->nextthink = std::max( master->nextthink, level.time + (int) ent->mapEntity.config.wait.time );
	}
	else if ( ent->mapEntity.moverState == ROTATOR_POS2 &&
	          ( groupState == MOVER_1TO2 || other == master ) )
	{
		// start moving 50 msec later, because if this was player-
		// triggered, level.time hasn't been advanced yet
		SetMoverState( ent, ROTATOR_2TO1, level.time + 50 );

		// starting sound
		if ( ent->mapEntity.sound2to1 )
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.sound2to1 );

		// looping sound
		ent->s.loopSound = ent->mapEntity.soundIndex;

		// open areaportal
		if ( ent->mapEntity.groupMaster == ent || !ent->mapEntity.groupMaster )
			trap_AdjustAreaPortalState( ent, true );

		//HACK: this is really ugly, should have specialise code,
		//but I'm both lazy, buzy to fix old mess, and annoyed by
		//said years-old mess
		ent->health = ent->mapEntity.config.health;
	}
	else if ( ent->mapEntity.moverState == ROTATOR_2TO1 )
	{
		// only partway down before reversing
		total = ent->s.apos.trDuration;
		partial = level.time - ent->s.apos.trTime;

		if ( partial > total )
		{
			partial = total;
		}

		SetMoverState( ent, ROTATOR_1TO2, level.time - ( total - partial ) );

		if ( ent->mapEntity.sound1to2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.sound1to2 );
		}
	}
	else if ( ent->mapEntity.moverState == ROTATOR_1TO2 )
	{
		// only partway up before reversing
		total = ent->s.apos.trDuration;
		partial = level.time - ent->s.apos.trTime;

		if ( partial > total )
		{
			partial = total;
		}

		SetMoverState( ent, ROTATOR_2TO1, level.time - ( total - partial ) );

		if ( ent->mapEntity.sound2to1 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.sound2to1 );
		}
	}
	else if ( ent->mapEntity.moverState == MODEL_POS1 )
	{
		//toggle door state
		ent->s.legsAnim = true;

		ent->think = Think_OpenModelDoor;
		ent->nextthink = level.time + ent->mapEntity.config.speed;

		// starting sound
		if ( ent->mapEntity.sound1to2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->mapEntity.sound1to2 );
		}

		// looping sound
		ent->s.loopSound = ent->mapEntity.soundIndex;

		// open areaportal
		if ( ent->mapEntity.groupMaster == ent || !ent->mapEntity.groupMaster )
		{
			trap_AdjustAreaPortalState( ent, true );
		}

		ent->mapEntity.moverState = MODEL_1TO2;
	}
	else if ( ent->mapEntity.moverState == MODEL_POS2 )
	{
		// if all the way up, just delay before coming down
		ent->nextthink = level.time + ent->mapEntity.config.wait.time;
	}
	//outd
	}
}

static void G_ResetFloatField( float* result, bool fallbackIfNegative, float instanceField, float classField, float fallback )
{
	if(instanceField && (instanceField > 0 || !fallbackIfNegative))
	{
		*result = instanceField;
	}
	else if (classField && (classField > 0 || !fallbackIfNegative))
	{
		*result = classField;
	}
	else
	{
		*result = fallback;
	}
}

static void reset_moverspeed( gentity_t *self, float fallbackSpeed )
{
	if ( !fallbackSpeed )
	{
		Sys::Drop( "No default speed was supplied to reset_moverspeed for entity #%i of type %s.", self->num(), self->classname );
	}

	G_ResetFloatField(&self->mapEntity.speed, true, self->mapEntity.config.speed, self->mapEntity.eclass->config.speed, fallbackSpeed);

	// reset duration only for linear movement else func_bobbing will not move
	if ( self->s.pos.trType != trType_t::TR_SINE )
	{
		// calculate time to reach second position from speed
		glm::vec3 move = self->mapEntity.activatedPosition - self->mapEntity.restingPosition;
		float distance = glm::length( move );

		VectorScale( move, self->mapEntity.speed, self->s.pos.trDelta );
		self->s.pos.trDuration = distance * 1000 / self->mapEntity.speed;

		if ( self->s.pos.trDuration <= 0 )
		{
			self->s.pos.trDuration = 1;
		}
	}
}

static void SP_ConstantLightField( gentity_t *self )
{
	float     light;
	glm::vec3 color;

	// if the "color" or "light" keys are set, setup constantLight
	bool lightSet = G_SpawnFloat( "light", "100", &light );
	bool colorSet = G_SpawnVector( "color", "1 1 1", color );

	if ( lightSet || colorSet )
	{
		glm::ivec3 rgb( color * 255.f );
		rgb = glm::clamp( rgb, 0, 255 );

		int i = light / 4;
		if ( i > 255 )
		{
			i = 255;
		}

		self->s.constantLight = rgb.r | ( rgb.g << 8 ) | ( rgb.b << 16 ) | ( i << 24 );
	}
}

/*
================
InitMover

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
static void InitMover( gentity_t *ent )
{
	// if the "model2" key is set, use a separate model
	// for drawing, but clip against the brushes
	if ( ent->mapEntity.model2.size() )
	{
		ent->s.modelindex2 = G_ModelIndex( ent->mapEntity.model2 );
	}

	SP_ConstantLightField( ent );

	ent->act = BinaryMover_act;
	ent->mapEntity.reached = BinaryMover_reached;

	const char *groupName;
	if ( G_SpawnString( "group", "", &groupName ) )
	{
		ent->mapEntity.groupName = groupName;
	}
	else if ( G_SpawnString( "team", "", &groupName ) )
	{
		G_WarnAboutDeprecatedEntityField( ent, "group", "team", ENT_V_RENAMED );
		ent->mapEntity.groupName = groupName;
	}

	ent->mapEntity.moverState = MOVER_POS1;
	ent->s.eType = entityType_t::ET_MOVER;
	ent->r.contents |= CONTENTS_MOVER;
	VectorCopy( ent->mapEntity.restingPosition, ent->r.currentOrigin );
	G_CM_LinkEntity( ent );

	ent->s.pos.trType = trType_t::TR_STATIONARY;
	VectorCopy( ent->mapEntity.restingPosition, ent->s.pos.trBase );
}

static void reset_rotatorspeed( gentity_t *self, float fallbackSpeed )
{
	if( !fallbackSpeed )
	{
		Sys::Drop("No default speed was supplied to reset_rotatorspeed for entity #%i of type %s.", self->num(), self->classname );
	}

	// calculate time to reach second position from speed
	glm::vec3 move = self->mapEntity.activatedPosition - self->mapEntity.restingPosition;
	float angle = glm::length( move );

	G_ResetFloatField(&self->mapEntity.speed, true, self->mapEntity.config.speed, self->mapEntity.eclass->config.speed, fallbackSpeed);

	VectorScale( move, self->mapEntity.speed, self->s.apos.trDelta );
	self->s.apos.trDuration = angle * 1000 / self->mapEntity.speed;

	if ( self->s.apos.trDuration <= 0 )
	{
		self->s.apos.trDuration = 1;
	}
}

/*
================
InitRotator

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
static void InitRotator( gentity_t *ent )
{
	// if the "model2" key is set, use a separate model
	// for drawing, but clip against the brushes
	if ( ent->mapEntity.model2.size() )
	{
		ent->s.modelindex2 = G_ModelIndex( ent->mapEntity.model2 );
	}

	SP_ConstantLightField( ent );

	ent->act = BinaryMover_act;
	ent->mapEntity.reached = BinaryMover_reached;

	const char     *groupName;
	if ( G_SpawnString( "group", "", &groupName ) )
	{
		ent->mapEntity.groupName = groupName;
	}
	else if ( G_SpawnString( "team", "", &groupName ) )
	{
		G_WarnAboutDeprecatedEntityField( ent, "group", "team", ENT_V_RENAMED );
		ent->mapEntity.groupName = groupName;
	}

	ent->mapEntity.moverState = ROTATOR_POS1;
	ent->s.eType = entityType_t::ET_MOVER;
	ent->r.contents |= CONTENTS_MOVER;
	VectorCopy( ent->mapEntity.restingPosition, ent->r.currentAngles );
	G_CM_LinkEntity( ent );

	ent->s.apos.trType = trType_t::TR_STATIONARY;
	VectorCopy( ent->mapEntity.restingPosition, ent->s.apos.trBase );
}

/*
===============================================================================

DOOR

A use can be triggered either by a touch function, by being shot, or by being
targeted by another entity.

===============================================================================
*/

/*
================
Blocked_Door
================
*/
static void func_door_block( gentity_t *self, gentity_t *other )
{
	// remove anything other than a client or buildable
	if ( !other->client && other->s.eType != entityType_t::ET_BUILDABLE )
	{
		G_FreeEntity( other );
		return;
	}

	if ( self->damage )
	{
		other->Damage( static_cast<float>( self->damage ), self, Util::nullopt, Util::nullopt, 0, MOD_CRUSH);
	}

	if ( self->mapEntity.spawnflags & 4 )
	{
		return; // crushers don't reverse
	}

	// reverse direction
	BinaryMover_act( self, self, other );
}

/*
================
Touch_DoorTrigger
================
*/
static void door_trigger_touch( gentity_t *self, gentity_t *other, trace_t* )
{
	//buildables don't trigger movers
	if ( other->s.eType == entityType_t::ET_BUILDABLE )
	{
		return;
	}

	moverState_t groupState = GetMoverGroupState( self->parent );

	if ( groupState != MOVER_1TO2 )
	{
		BinaryMover_act( self->parent, self, other );
	}
}

static void Think_MatchGroup( gentity_t *self )
{
	if ( self->flags & FL_GROUPSLAVE )
	{
		return;
	}
	MatchGroup( self, self->mapEntity.moverState, level.time );
}

/*
======================
Think_SpawnNewDoorTrigger

All of the parts of a door have been spawned, so create
a trigger that encloses all of them
======================
*/
static void Think_SpawnNewDoorTrigger( gentity_t *self )
{
	gentity_t *other;

	// find the bounds of everything on the group
	glm::vec3 mins = VEC2GLM( self->r.absmin );
	glm::vec3 maxs = VEC2GLM( self->r.absmax );

	for ( other = self->mapEntity.groupChain; other; other = other->mapEntity.groupChain )
	{
		AddPointToBounds( other->r.absmin, &mins[0], &maxs[0] );
		AddPointToBounds( other->r.absmax, &mins[0], &maxs[0] );
	}

	// find the thinnest axis, which will be the one we expand
	int best = 0;

	glm::vec3 diff = maxs - mins;
	for ( int i = 1; i < 3; i++ )
	{
		if ( diff[ i ] < diff[ best ] )
		{
			best = i;
		}
	}

	maxs[ best ] += self->mapEntity.config.triggerRange;
	mins[ best ] -= self->mapEntity.config.triggerRange;

	// create a trigger with this size
	other = G_NewEntity( NO_CBSE );
	other->classname = S_DOOR_SENSOR;
	VectorCopy( mins, other->r.mins );
	VectorCopy( maxs, other->r.maxs );
	other->parent = self;
	other->r.contents = CONTENTS_TRIGGER;
	other->touch = door_trigger_touch;
	// remember the thinnest axis
	other->customNumber = best;
	G_CM_LinkEntity( other );

	if ( self->mapEntity.moverState < MODEL_POS1 )
	{
		Think_MatchGroup( self );
	}
}

static void Think_MoverDeath( gentity_t *self, gentity_t*, gentity_t* attacker, meansOfDeath_t )
{
	if ( self->mapEntity.moverState == MOVER_POS1 || self->mapEntity.moverState == ROTATOR_POS1 )
	{
		BinaryMover_act( self, nullptr, attacker );
	}
}

static void func_door_reset( gentity_t *self )
{
	G_ResetIntField(&self->health, true, self->mapEntity.config.health, self->mapEntity.eclass->config.health, 0);
	G_ResetIntField(&self->damage, true, self->mapEntity.config.damage, self->mapEntity.eclass->config.damage, 2);

	reset_moverspeed( self, 400 );
}

static void func_door_use( gentity_t *self, gentity_t *caller, gentity_t *activator )
{
	if (GetMoverGroupState( self ) != MOVER_1TO2 )
		BinaryMover_act( self, caller, activator );
}

void SP_func_door( gentity_t *self )
{
	float  lip;

	if( !self->mapEntity.sound1to2 )
	{
		self->mapEntity.sound1to2 = G_SoundIndex( "sound/movers/doors/dr1_strt" );
	}
	if( !self->mapEntity.sound2to1 )
	{
		self->mapEntity.sound2to1 = G_SoundIndex( "sound/movers/doors/dr1_strt" );
	}
	if( !self->mapEntity.soundPos1 )
	{
		self->mapEntity.soundPos1 = G_SoundIndex( "sound/movers/doors/dr1_end" );
	}
	if( !self->mapEntity.soundPos2 )
	{
		self->mapEntity.soundPos2 = G_SoundIndex( "sound/movers/doors/dr1_end" );
	}

	self->mapEntity.blocked = func_door_block;
	self->reset = func_door_reset;

	// default wait of 2 seconds
	if ( !self->mapEntity.config.wait.time )
	{
		self->mapEntity.config.wait.time = 2;
	}

	self->mapEntity.config.wait.time *= 1000;

	// default trigger range of 72 units (saved in noise_index)
	G_SpawnInt( "range", "72", &self->mapEntity.config.triggerRange );

	if ( self->mapEntity.config.triggerRange < 0 )
	{
		self->mapEntity.config.triggerRange = 72;
	}

	// default lip of 8 units
	G_SpawnFloat( "lip", "8", &lip );

	// first position at start
	self->mapEntity.restingPosition = VEC2GLM( self->s.origin );

	// calculate second position
	G_CM_SetBrushModel( self, self->mapEntity.model );

	glm::vec3 angles = VEC2GLM( self->s.angles );
	G_SetMovedir( angles, self->mapEntity.movedir );
	VectorCopy( angles, self->s.angles );

	glm::vec3 abs_movedir = glm::abs( self->mapEntity.movedir );
	glm::vec3 size = VEC2GLM( self->r.maxs ) - VEC2GLM( self->r.mins );
	float distance = glm::dot( abs_movedir, size ) - lip;
	self->mapEntity.activatedPosition = self->mapEntity.restingPosition + distance * self->mapEntity.movedir;

	// if "start_open", reverse position 1 and 2
	if ( self->mapEntity.spawnflags & 1 )
	{
		self->mapEntity.restingPosition = self->mapEntity.activatedPosition;
		self->mapEntity.activatedPosition = VEC2GLM( self->s.origin );
	}

	InitMover( self );

	self->nextthink = level.time + FRAMETIME;
	self->health = self->mapEntity.config.health;

	if ( self->mapEntity.names[ 0 ].size() || self->mapEntity.config.health ) //FIXME wont work yet with class fallbacks
	{
		// non touch/shoot doors
		self->think = Think_MatchGroup;
		if ( self->mapEntity.config.health )
		{
			self->health = self->mapEntity.config.health;
			self->die = Think_MoverDeath;
		}
	}
	else
	{
		self->think = Think_SpawnNewDoorTrigger;
	}

	if ( self->mapEntity.spawnflags & ACTIVATE )
	{
		self->use = func_door_use;
		self->touch = nullptr;
		self->think = nullptr;
	}
}

static void func_door_rotating_reset( gentity_t *self )
{
	G_ResetIntField(&self->health, true, self->mapEntity.config.health, self->mapEntity.eclass->config.health, 0);
	G_ResetIntField(&self->damage, true, self->mapEntity.config.damage, self->mapEntity.eclass->config.damage, 2);

	reset_rotatorspeed( self, 120 );
}

void SP_func_door_rotating( gentity_t *self )
{
	if( !self->mapEntity.sound1to2 )
	{
		self->mapEntity.sound1to2 = G_SoundIndex( "sound/movers/doors/dr1_strt" );
	}
	if( !self->mapEntity.sound2to1 )
	{
		self->mapEntity.sound2to1 = G_SoundIndex( "sound/movers/doors/dr1_strt" );
	}
	if( !self->mapEntity.soundPos1 )
	{
		self->mapEntity.soundPos1 = G_SoundIndex( "sound/movers/doors/dr1_end" );
	}
	if( !self->mapEntity.soundPos2 )
	{
		self->mapEntity.soundPos2 = G_SoundIndex( "sound/movers/doors/dr1_end" );
	}

	self->mapEntity.blocked = func_door_block;
	self->reset = func_door_rotating_reset;

	// if speed is negative, positize it and add reverse flag
	if ( self->mapEntity.config.speed < 0 )
	{
		self->mapEntity.config.speed *= -1;
		self->mapEntity.spawnflags |= 8;
	}

	// default of 2 seconds
	if ( !self->mapEntity.config.wait.time )
	{
		self->mapEntity.config.wait.time = 2;
	}

	self->mapEntity.config.wait.time *= 1000;

	// default trigger range of 72 units (saved in noise_index)
	G_SpawnInt( "range", "72", &self->mapEntity.config.triggerRange );

	if ( self->mapEntity.config.triggerRange < 0 )
	{
		self->mapEntity.config.triggerRange = 72;
	}

	// set the axis of rotation
	self->mapEntity.movedir = glm::vec3();
	VectorClear( self->s.angles );

	if ( self->mapEntity.spawnflags & 32 )
	{
		self->mapEntity.movedir[ 2 ] = 1.0;
	}
	else if ( self->mapEntity.spawnflags & 64 )
	{
		self->mapEntity.movedir[ 0 ] = 1.0;
	}
	else
	{
		self->mapEntity.movedir[ 1 ] = 1.0;
	}

	// reverse direction if necessary
	if ( self->mapEntity.spawnflags & 8 )
	{
		self->mapEntity.movedir = -self->mapEntity.movedir;
	}

	G_SpawnFloat( "rotatorAngle", "0", &self->mapEntity.rotatorAngle );

	if ( self->mapEntity.rotatorAngle == 0.f )
	{
		self->mapEntity.rotatorAngle = 90.0;
	}

	self->mapEntity.restingPosition = VEC2GLM( self->s.angles );
	G_CM_SetBrushModel( self, self->mapEntity.model );
	self->mapEntity.activatedPosition = self->mapEntity.restingPosition + self->mapEntity.rotatorAngle * self->mapEntity.movedir;

	// if "start_open", reverse position 1 and 2
	if ( self->mapEntity.spawnflags & 1 )
	{
		self->mapEntity.restingPosition = self->mapEntity.activatedPosition;
		self->mapEntity.activatedPosition = VEC2GLM( self->s.angles );
		self->mapEntity.movedir = -self->mapEntity.movedir;
	}

	// set origin
	VectorCopy( self->s.origin, self->s.pos.trBase );
	VectorCopy( self->s.pos.trBase, self->r.currentOrigin );

	InitRotator( self );

	self->nextthink = level.time + FRAMETIME;
	self->health = self->mapEntity.config.health;

	if ( self->mapEntity.names[ 0 ].size() || self->mapEntity.config.health ) //FIXME wont work yet with class fallbacks
	{
		// non touch/shoot doors
		self->think = Think_MatchGroup;
		if ( self->mapEntity.config.health )
		{
			self->die = Think_MoverDeath;
		}
	}
	else
	{
		self->think = Think_SpawnNewDoorTrigger;
	}

	if ( self->mapEntity.spawnflags & ACTIVATE )
	{
		self->use = func_door_use;
		self->touch = nullptr;
		self->think = nullptr;
	}
}

static void func_door_model_reset( gentity_t *self )
{
	G_ResetFloatField(&self->mapEntity.speed, true, self->mapEntity.config.speed, self->mapEntity.eclass->config.speed, 200);
	G_ResetIntField(&self->health, true, self->mapEntity.config.health, self->mapEntity.eclass->config.health, 0);

	self->s.torsoAnim = self->s.weapon * ( 1000.0f / self->mapEntity.speed ); //framerate
	if ( self->s.torsoAnim <= 0 )
	{
		self->s.torsoAnim = 1;
	}
}

void SP_func_door_model( gentity_t *self )
{
	if( !self->mapEntity.sound1to2 )
	{
		self->mapEntity.sound1to2 = G_SoundIndex( "sound/movers/doors/dr1_strt" );
	}
	if( !self->mapEntity.sound2to1 )
	{
		self->mapEntity.sound2to1 = G_SoundIndex( "sound/movers/doors/dr1_strt" );
	}
	if( !self->mapEntity.soundPos1 )
	{
		self->mapEntity.soundPos1 = G_SoundIndex( "sound/movers/doors/dr1_end" );
	}
	if( !self->mapEntity.soundPos2 )
	{
		self->mapEntity.soundPos2 = G_SoundIndex( "sound/movers/doors/dr1_end" );
	}

	self->reset = func_door_model_reset;

	//default wait of 2 seconds
	if ( self->mapEntity.config.wait.time <= 0 )
	{
		self->mapEntity.config.wait.time = 2;
	}

	self->mapEntity.config.wait.time *= 1000;

	// default trigger range of 72 units (saved in noise_index)
	G_SpawnInt( "range", "72", &self->mapEntity.config.triggerRange );

	if ( self->mapEntity.config.triggerRange < 0 )
	{
		self->mapEntity.config.triggerRange = 72;
	}

	//brush model
	gentity_t* clipBrush = self->mapEntity.clipBrush = G_NewEntity( NO_CBSE );
	clipBrush->mapEntity.model = self->mapEntity.model;
	G_CM_SetBrushModel( clipBrush, clipBrush->mapEntity.model );
	clipBrush->s.eType = entityType_t::ET_INVISIBLE;

	//copy the bounds back from the clipBrush so the
	//triggers can be made
	VectorCopy( clipBrush->r.absmin, self->r.absmin );
	VectorCopy( clipBrush->r.absmax, self->r.absmax );
	VectorCopy( clipBrush->r.mins, self->r.mins );
	VectorCopy( clipBrush->r.maxs, self->r.maxs );

	glm::vec3 tmp;
	G_SpawnVector( "modelOrigin", "0 0 0", tmp );
	VectorCopy( tmp, self->s.origin );

	G_SpawnVector( "scale", "1 1 1", tmp );
	VectorCopy( tmp, self->s.origin2 );

	// if the "model2" key is set, use a separate model
	// for drawing, but clip against the brushes
	if ( !self->mapEntity.model2.size() )
	{
		Log::Warn("func_door_model %d spawned with no model2 key", self->num() );
	}
	else
	{
		self->s.modelindex = G_ModelIndex( self->mapEntity.model2 );
	}

	// if the "noise" key is set, use a constant looping sound when moving
	const char* sound;
	if ( G_SpawnString( "noise", "", &sound ) )
	{
		self->mapEntity.soundIndex = G_SoundIndex( sound );
	}

	SP_ConstantLightField( self );

	self->act = BinaryMover_act;

	self->mapEntity.moverState = MODEL_POS1;
	self->s.eType = entityType_t::ET_MODELDOOR;
	self->r.contents |= CONTENTS_MOVER; // TODO: Check if this is the right thing to add the flag to
	VectorCopy( self->s.origin, self->s.pos.trBase );
	self->s.pos.trType = trType_t::TR_STATIONARY;
	self->s.pos.trTime = 0;
	self->s.pos.trDuration = 0;
	VectorClear( self->s.pos.trDelta );
	VectorCopy( self->s.angles, self->s.apos.trBase );
	self->s.apos.trType = trType_t::TR_STATIONARY;
	self->s.apos.trTime = 0;
	self->s.apos.trDuration = 0;
	VectorClear( self->s.apos.trDelta );

	self->s.misc = static_cast<int>( self->animation[ 0 ] ); //first frame
	self->s.weapon = abs( static_cast<int>( self->animation[ 1 ] ) );  //number of frames

	//must be at least one frame -- mapper has forgotten animation key
	if ( self->s.weapon == 0 )
	{
		self->s.weapon = 1;
	}

	self->s.torsoAnim = 1; // stub value to avoid sigfpe

	G_CM_LinkEntity( self );

	if ( !( self->mapEntity.names[ 0 ].size() || self->mapEntity.config.health ) ) //FIXME wont work yet with class fallbacks
	{
		self->nextthink = level.time + FRAMETIME;
		self->think = Think_SpawnNewDoorTrigger;
	}

	if ( self->mapEntity.spawnflags & ACTIVATE )
	{
		self->use = func_door_use;
		self->touch = nullptr;
		self->think = nullptr;
	}
}

/*
===============================================================================

platform

===============================================================================
*/

/*
==============
Touch_Plat

Don't allow to descend if a live player is on it
===============
*/
static void Touch_Plat( gentity_t *ent, gentity_t *other, trace_t* )
{
	// DONT_WAIT
	if ( ent->mapEntity.spawnflags & 1 )
	{
		return;
	}

	if ( !other->client || Entities::IsDead( other ) )
	{
		return;
	}

	// delay return-to-pos1 by one second
	if ( ent->mapEntity.moverState == MOVER_POS2 )
	{
		ent->nextthink = level.time + 1000;
	}
}

/*
==============
Touch_PlatCenterTrigger

If the plat is at the bottom position, start it going up
===============
*/
static void Touch_PlatCenterTrigger( gentity_t *ent, gentity_t *other, trace_t* )
{
	if ( !other->client )
	{
		return;
	}

	if ( ent->parent->mapEntity.moverState == MOVER_POS1 )
	{
		BinaryMover_act( ent->parent, ent, other );
	}
}

/*
================
SpawnPlatSensor

Spawn a trigger in the middle of the plat's low position
Elevator cars require that the trigger extend through the entire low position,
not just sit on top of it.
================
*/
static void SpawnPlatSensor( gentity_t *self )
{
	// the middle trigger will be a thin trigger just
	// above the starting position
	gentity_t *sensor = G_NewEntity( NO_CBSE );
	sensor->classname = S_PLAT_SENSOR;
	sensor->touch = Touch_PlatCenterTrigger;
	sensor->r.contents = CONTENTS_TRIGGER;
	sensor->parent = self;

	glm::vec3 tmin = self->mapEntity.restingPosition + VEC2GLM( self->r.mins ) + 33.f;
	tmin[2] -= 33.f;

	glm::vec3 tmax = self->mapEntity.restingPosition + VEC2GLM( self->r.maxs ) - 33.f;
	tmax[2] += 33 + 8;

	if ( tmax[ 0 ] <= tmin[ 0 ] )
	{
		tmin[ 0 ] = self->mapEntity.restingPosition[ 0 ] + ( self->r.mins[ 0 ] + self->r.maxs[ 0 ] ) * 0.5;
		tmax[ 0 ] = tmin[ 0 ] + 1;
	}

	if ( tmax[ 1 ] <= tmin[ 1 ] )
	{
		tmin[ 1 ] = self->mapEntity.restingPosition[ 1 ] + ( self->r.mins[ 1 ] + self->r.maxs[ 1 ] ) * 0.5;
		tmax[ 1 ] = tmin[ 1 ] + 1;
	}

	VectorCopy( tmin, sensor->r.mins );
	VectorCopy( tmax, sensor->r.maxs );

	G_CM_LinkEntity( sensor );
}

void SP_func_plat( gentity_t *self )
{
	float lip, height;

	if( !self->mapEntity.sound1to2 )
	{
		self->mapEntity.sound1to2 = G_SoundIndex( "sound/movers/plats/pt1_strt" );
	}
	if( !self->mapEntity.sound2to1 )
	{
		self->mapEntity.sound2to1 = G_SoundIndex( "sound/movers/plats/pt1_strt" );
	}
	if( !self->mapEntity.soundPos1 )
	{
		self->mapEntity.soundPos1 = G_SoundIndex( "sound/movers/plats/pt1_end" );
	}
	if( !self->mapEntity.soundPos2 )
	{
		self->mapEntity.soundPos2 = G_SoundIndex( "sound/movers/plats/pt1_end" );
	}

	VectorClear( self->s.angles );

	G_SpawnFloat( "lip", "8", &lip );

	G_ResetIntField(&self->damage, true, self->mapEntity.config.damage, self->mapEntity.eclass->config.damage, 2);

	if(!self->mapEntity.config.wait.time)
		self->mapEntity.config.wait.time = 1.0f;

	self->mapEntity.config.wait.time *= 1000;

	// create second position
	G_CM_SetBrushModel( self, self->mapEntity.model );

	if ( !G_SpawnFloat( "height", "0", &height ) )
	{
		height = ( self->r.maxs[ 2 ] - self->r.mins[ 2 ] ) - lip;
	}

	// pos1 is the rest (bottom) position, pos2 is the top
	self->mapEntity.activatedPosition = VEC2GLM( self->s.origin );
	self->mapEntity.restingPosition = self->mapEntity.activatedPosition;
	self->mapEntity.restingPosition[ 2 ] -= height;

	InitMover( self );
	reset_moverspeed( self, 400 );

	// touch function keeps the plat from returning while
	// a live player is standing on it
	self->touch = Touch_Plat;

	self->mapEntity.blocked = func_door_block;

	self->parent = self; // so it can be treated as a door

	// spawn the trigger if one hasn't been custom made
	if ( self->mapEntity.names[ 0 ].empty() )
	{
		SpawnPlatSensor( self );
	}
}

/*
===============================================================================

BUTTON

===============================================================================
*/

static void Touch_Button( gentity_t *ent, gentity_t *other, trace_t* )
{
	if ( !other->client )
	{
		return;
	}

	if ( ent->mapEntity.moverState == MOVER_POS1 )
	{
		BinaryMover_act( ent, other, other );
	}
}

static void func_button_use( gentity_t *self, gentity_t *caller, gentity_t *activator )
{
	if ( self->mapEntity.moverState == MOVER_POS1 )
		BinaryMover_act( self, caller, activator );
}

static void func_button_reset( gentity_t *self )
{
	G_ResetIntField(&self->health, true, self->mapEntity.config.health, self->mapEntity.eclass->config.health, 0);

	reset_moverspeed( self, 40 );
}

void SP_func_button( gentity_t *self )
{
	if( !self->mapEntity.sound1to2 )
	{
		self->mapEntity.sound1to2 = G_SoundIndex( "sound/movers/switches/button1" );
	}

	self->reset = func_button_reset;

	if ( !self->mapEntity.config.wait.time )
	{
		self->mapEntity.config.wait.time = 1;
	}

	self->mapEntity.config.wait.time *= 1000;

	// first position
	self->mapEntity.restingPosition = VEC2GLM( self->s.origin );

	// calculate second position
	G_CM_SetBrushModel( self, self->mapEntity.model );

	float  lip;
	G_SpawnFloat( "lip", "4", &lip );

	glm::vec3 angles = VEC2GLM( self->s.angles );
	G_SetMovedir( angles, self->mapEntity.movedir );
	VectorCopy( angles, self->s.angles );

	glm::vec3 size = VEC2GLM( self->r.maxs ) - VEC2GLM( self->r.mins );
	glm::vec3 temp = glm::abs( self->mapEntity.movedir ) * size;
	float distance = std::accumulate( glm::begin( temp ), glm::end( temp ), -lip );
	self->mapEntity.activatedPosition = self->mapEntity.restingPosition + distance * self->mapEntity.movedir;

	// touchable button by default
	self->touch = Touch_Button;

	if ( self->mapEntity.spawnflags & ACTIVATE ) //keep the same value as for doors
	{
		self->use = func_button_use;
		self->touch = nullptr;
	}

	if ( self->mapEntity.config.health ) //FIXME wont work yet with class fallbacks
	{
		self->die = Think_MoverDeath;
		self->touch = nullptr;
	}

	InitMover( self );
}

/*
===============================================================================

TRAIN

===============================================================================
*/

#define TRAIN_START_OFF   1
#define TRAIN_BLOCK_STOPS 2
#define CORNER_PAUSE      1

static void Stop_Train( gentity_t *self );

/*
===============
Think_BeginMoving

The wait time at a corner has completed, so start moving again
===============
*/
static void Think_BeginMoving( gentity_t *self )
{
	self->s.pos.trTime = level.time;
	self->s.pos.trType = trType_t::TR_LINEAR_STOP;
}

/*
===============
Reached_Train
===============
*/

static void func_train_reached( gentity_t *self )
{
	// copy the appropriate values
	gentity_t *next = self->nextPathSegment;

	if ( !next || !next->nextPathSegment )
	{
		return; // just stop
	}

	// fire all other targets
	G_FireEntity( next, nullptr );

	// set the new trajectory
	self->nextPathSegment = next->nextPathSegment;
	self->mapEntity.restingPosition = VEC2GLM( next->s.origin );
	self->mapEntity.activatedPosition = VEC2GLM( next->nextPathSegment->s.origin );

	// if the path_corner has a speed, use that otherwise use the train's speed
	G_ResetFloatField( &self->mapEntity.speed, true, next->mapEntity.config.speed, next->mapEntity.eclass->config.speed, 0);
	if(!self->mapEntity.speed) {
		G_ResetFloatField(&self->mapEntity.speed, true, self->mapEntity.config.speed, self->mapEntity.eclass->config.speed, DEFAULT_FUNC_TRAIN_SPEED);
	}

	// calculate duration
	glm::vec3 move = self->mapEntity.activatedPosition - self->mapEntity.restingPosition;
	float length = glm::length( move );

	self->s.pos.trDuration = length * 1000 / self->mapEntity.speed;

	// Be sure to send to clients after any fast move case
	self->r.svFlags &= ~SVF_NOCLIENT;

	// Fast move case
	if ( self->s.pos.trDuration < 1 )
	{
		// As trDuration is used later in a division, we need to avoid that case now
		self->s.pos.trDuration = 1;

		// Don't send entity to clients so it becomes really invisible
		self->r.svFlags |= SVF_NOCLIENT;
	}

	// looping sound
	self->s.loopSound = next->mapEntity.soundIndex;

	// start it going
	SetMoverState( self, MOVER_1TO2, level.time );

	if ( self->mapEntity.spawnflags & TRAIN_START_OFF )
	{
		self->s.pos.trType = trType_t::TR_STATIONARY;
		return;
	}

	if ( next->mapEntity.spawnflags & CORNER_PAUSE )
	{
		Stop_Train( self );
	}
	// if there is a "wait" value on the target, don't start moving yet
	else if ( next->mapEntity.config.wait.time )
	{
		self->nextthink = level.time + next->mapEntity.config.wait.time * 1000;
		self->think = Think_BeginMoving;
		self->s.pos.trType = trType_t::TR_STATIONARY;
	}
}

/*
================
Start_Train
================
*/
static void Start_Train( gentity_t *self )
{
	//recalculate duration as the mover is highly
	//unlikely to be right on a path_corner
	glm::vec3 move = self->mapEntity.activatedPosition - self->mapEntity.restingPosition;
	self->s.pos.trDuration = glm::length( move ) * 1000 / self->mapEntity.speed;
	self->s.pos.trDuration = std::max( 1, self->s.pos.trDuration );

	SetMoverState( self, MOVER_1TO2, level.time );

	self->mapEntity.spawnflags &= ~TRAIN_START_OFF;
}

/*
================
Stop_Train
================
*/
static void Stop_Train( gentity_t *self )
{
	self->mapEntity.restingPosition = BG_EvaluateTrajectory( &self->s.pos, level.time );
	SetMoverState( self, MOVER_POS1, level.time );

	self->mapEntity.spawnflags |= TRAIN_START_OFF;
}

/*
================
Use_Train
================
*/
static void func_train_act( gentity_t *ent, gentity_t*, gentity_t* )
{
	if ( ent->mapEntity.spawnflags & TRAIN_START_OFF )
	{
		//train is currently not moving so start it
		Start_Train( ent );
	}
	else
	{
		//train is moving so stop it
		Stop_Train( ent );
	}
}

/*
===============
Think_SetupTrainTargets

Link all the corners together
===============
*/
static void Think_SetupTrainTargets( gentity_t *self )
{
	int targetIndex;

	self->nextPathSegment = G_IterateTargets( nullptr, &targetIndex, self );

	if ( !self->nextPathSegment )
	{
		Log::Warn( "func_train at %s with an unfound target",
		          glm::to_string( VEC2GLM( self->r.absmin ) ).c_str() );
		return;
	}

	gentity_t *next, *start = nullptr;
	for ( gentity_t* path = self->nextPathSegment; path != start; path = next )
	{
		if ( !start )
		{
			start = path;
		}

		if ( path->mapEntity.targets.empty() )
		{
			Log::Warn( "Train corner at %s without a target",
			          glm::to_string( VEC2GLM( path->s.origin ) ) );
			return;
		}

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = nullptr;

		do
		{
			next = G_IterateTargets( next, &targetIndex, path );

			if ( !next )
			{
				Log::Warn( "Train corner at %s without a referenced " S_PATH_CORNER,
				          glm::to_string( VEC2GLM( path->s.origin ) ) );
				return;
			}
		}
		while ( next->classname != S_PATH_CORNER );

		path->nextPathSegment = next;
	}

	// start the train moving from the first corner
	func_train_reached( self );
}

/*
================
Blocked_Train
================
*/
static void func_train_blocked( gentity_t *self, gentity_t *other )
{
	if ( self->mapEntity.spawnflags & TRAIN_BLOCK_STOPS )
	{
		Stop_Train( self );
	}
	else
	{
		if ( !other->client )
		{
			//whatever is blocking the train isn't a client

			//KILL!!1!!!
			Entities::Kill(other, self, MOD_CRUSH);

			//buildables need to be handled differently since even when
			//dealt fatal amounts of damage they won't instantly become non-solid
			if ( other->s.eType == entityType_t::ET_BUILDABLE && other->spawned )
			{
				if ( other->buildableTeam == TEAM_ALIENS )
				{
					glm::vec3 dir = VEC2GLM( other->s.origin2 );
					gentity_t *tent = G_NewTempEntity( VEC2GLM( other->s.origin ), EV_ALIEN_BUILDABLE_EXPLOSION );
					tent->s.eventParm = DirToByte( &dir[0] );
				}
				else if ( other->buildableTeam == TEAM_HUMANS )
				{
					glm::vec3 dir( 0.f, 0.f, 1.f );
					gentity_t *tent = G_NewTempEntity( VEC2GLM( other->s.origin ), EV_HUMAN_BUILDABLE_EXPLOSION );
					tent->s.eventParm = DirToByte( &dir[0] );
				}
			}

			G_FreeEntity( other );

			return;
		}

		Entities::Kill(other, self, MOD_CRUSH);
	}
}

void SP_func_train( gentity_t *self )
{
	VectorClear( self->s.angles );

	if ( self->mapEntity.spawnflags & TRAIN_BLOCK_STOPS )
	{
		self->damage = 0;
	}
	else
	{
		G_ResetIntField(&self->damage, true, self->mapEntity.config.damage, self->mapEntity.eclass->config.damage, 2);
	}

	G_CM_SetBrushModel( self, self->mapEntity.model );
	InitMover( self );
	reset_moverspeed( self, DEFAULT_FUNC_TRAIN_SPEED );

	self->mapEntity.reached = func_train_reached;
	self->act = func_train_act;
	self->mapEntity.blocked = func_train_blocked;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets;
}

/*
===============================================================================

STATIC

===============================================================================
*/

void SP_func_static( gentity_t *self )
{
	const char *gradingTexture;
	float gradingDistance;
	const char *reverbEffect;
	float reverbDistance;
	float reverbIntensity;

	G_CM_SetBrushModel( self, self->mapEntity.model );
	InitMover( self );
	reset_moverspeed( self, 100 ); //TODO do we need this at all?
	VectorCopy( self->s.origin, self->s.pos.trBase );
	VectorCopy( self->s.origin, self->r.currentOrigin );

	// HACK: Not really a mover, so unset the content flag
	self->r.contents &= ~CONTENTS_MOVER;

	// check if this func_static has a colorgrading texture
	if( self->mapEntity.model.c_str()[0] == '*'
			&& G_SpawnString( "gradingTexture", "", &gradingTexture ) )
	{
		G_SpawnFloat( "gradingDistance", "250", &gradingDistance );

		G_GradingTextureIndex( va( "%s %f %s", self->mapEntity.model.c_str() + 1,
					   gradingDistance, gradingTexture ) );
	}

	// check if this func_static has a colorgrading texture
	if( self->mapEntity.model.c_str()[0] == '*'
			&& G_SpawnString( "reverbEffect", "", &reverbEffect ) )
	{
		G_SpawnFloat( "reverbDistance", "250", &reverbDistance );
		G_SpawnFloat( "reverbIntensity", "1", &reverbIntensity );

		reverbIntensity = Math::Clamp( reverbIntensity, 0.0f, 2.0f );
		G_ReverbEffectIndex( va( "%s %f %s %f", self->mapEntity.model.c_str() + 1,
					   reverbDistance, reverbEffect, reverbIntensity ) );
	}
}

void SP_func_dynamic( gentity_t *self )
{
	G_CM_SetBrushModel( self, self->mapEntity.model );

	InitMover( self );
	reset_moverspeed( self, 100 ); //TODO do we need this at all?

	self->flags |= FL_GROUPSLAVE;

	G_CM_UnlinkEntity( self );  // was linked in InitMover
	G_CM_LinkEntity( self );
}

/*
===============================================================================

ROTATING

QUAKED func_rotating (0 .5 .8) ? START_ON - X_AXIS Y_AXIS
You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.
"model2"  .md3 model to also draw
"speed"   determines how fast it moves; default value is 100.
"dmg"   damage to inflict when blocked (2 default)
"color"   constantLight color
"light"   constantLight radius

===============================================================================
*/

void SP_func_rotating( gentity_t *self )
{
	G_ResetFloatField(&self->mapEntity.speed, false, self->mapEntity.config.speed, self->mapEntity.eclass->config.speed, 100);

	// set the axis of rotation
	self->s.apos.trType = trType_t::TR_LINEAR;

	if ( self->mapEntity.spawnflags & 4 )
	{
		self->s.apos.trDelta[ 2 ] = self->mapEntity.speed;
	}
	else if ( self->mapEntity.spawnflags & 8 )
	{
		self->s.apos.trDelta[ 0 ] = self->mapEntity.speed;
	}
	else
	{
		self->s.apos.trDelta[ 1 ] = self->mapEntity.speed;
	}

	G_ResetIntField(&self->damage, true, self->mapEntity.config.damage, self->mapEntity.eclass->config.damage, 2);

	G_CM_SetBrushModel( self, self->mapEntity.model );
	InitMover( self );
	reset_moverspeed( self, 100 );

	VectorCopy( self->s.origin, self->s.pos.trBase );
	VectorCopy( self->s.pos.trBase, self->r.currentOrigin );
	VectorCopy( self->s.apos.trBase, self->r.currentAngles );
}

/*
===============================================================================

BOBBING

===============================================================================
*/

static void func_bobbing_reset( gentity_t *self )
{
	reset_moverspeed( self, 4 );
}

void SP_func_bobbing( gentity_t *self )
{
	float height;
	float phase;

	self->reset = func_bobbing_reset;

	G_SpawnFloat( "height", "32", &height );
	G_SpawnFloat( "phase", "0", &phase );

	G_ResetIntField(&self->damage, true, self->mapEntity.config.damage, self->mapEntity.eclass->config.damage, 2);

	G_CM_SetBrushModel( self, self->mapEntity.model );
	InitMover( self );
	reset_moverspeed( self, 4 );

	VectorCopy( self->s.origin, self->s.pos.trBase );
	VectorCopy( self->s.origin, self->r.currentOrigin );

	self->s.pos.trDuration = self->mapEntity.speed * 1000;
	self->s.pos.trTime = self->s.pos.trDuration * phase;
	self->s.pos.trType = trType_t::TR_SINE;

	// set the axis of bobbing
	if ( self->mapEntity.spawnflags & 1 )
	{
		self->s.pos.trDelta[ 0 ] = height;
	}
	else if ( self->mapEntity.spawnflags & 2 )
	{
		self->s.pos.trDelta[ 1 ] = height;
	}
	else
	{
		self->s.pos.trDelta[ 2 ] = height;
	}
}

/*
===============================================================================

PENDULUM

===============================================================================
*/

void SP_func_pendulum( gentity_t *self )
{
	float phase;
	G_SpawnFloat( "phase", "0", &phase );

	G_ResetIntField(&self->damage, true, self->mapEntity.config.damage, self->mapEntity.eclass->config.damage, 2);

	G_CM_SetBrushModel( self, self->mapEntity.model );

	// find pendulum length
	float length = fabs( self->r.mins[ 2 ] );

	if ( length < 8.0f )
	{
		length = 8.0f;
	}

	InitMover( self );
	reset_moverspeed( self, 30 );

	VectorCopy( self->s.origin, self->s.pos.trBase );
	VectorCopy( self->s.origin, self->r.currentOrigin );

	VectorCopy( self->s.angles, self->s.apos.trBase );

	float frequency = 1.0f / ( M_PI * 2.0f ) * sqrtf( g_gravity.Get() / ( 3.0f * length ) );
	self->s.apos.trDuration = 1000 / frequency;
	self->s.apos.trTime = self->s.apos.trDuration * phase;
	self->s.apos.trType = trType_t::TR_SINE;
	self->s.apos.trDelta[ 2 ] = self->mapEntity.speed;
}

static void G_KillBrushModel( gentity_t *ent, gentity_t *activator )
{
	gentity_t *e;
	vec3_t mins, maxs;
	trace_t tr;

	for( e = &g_entities[ 0 ]; e < &g_entities[ level.num_entities ]; ++e )
	{
		if( !e->r.linked || !e->clipmask )
			continue;

		VectorAdd( e->r.currentOrigin, e->r.mins, mins );
		VectorAdd( e->r.currentOrigin, e->r.maxs, maxs );

		if( !G_CM_EntityContact( VEC2GLM( mins ), VEC2GLM( maxs ), ent, traceType_t::TT_AABB ) )
			continue;

		G_CM_Trace( &tr, VEC2GLM( e->r.currentOrigin ), VEC2GLM( e->r.mins ), VEC2GLM( e->r.maxs ), VEC2GLM( e->r.currentOrigin ), e->num(), e->clipmask, 0, traceType_t::TT_AABB );

		if( tr.entityNum != ENTITYNUM_NONE ) {
			Entities::Kill(e, activator, MOD_CRUSH);
		}
	}
}

/*
====================
Use_func_spawn
====================
*/
static void func_spawn_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	if( self->r.linked )
	{
		G_BotRemoveObstacle( self->num() );
		G_CM_UnlinkEntity( self );
	}
	else
	{
		glm::vec3 mins = self->mapEntity.restingPosition + VEC2GLM( self->r.mins );
		glm::vec3 maxs = self->mapEntity.restingPosition + VEC2GLM( self->r.maxs );
		G_BotAddObstacle( mins, maxs, self->num() );
		G_CM_LinkEntity( self );
		if( !( self->mapEntity.spawnflags & 2 ) )
		{
			G_KillBrushModel( self, activator );
		}
	}
}

static void func_spawn_reset( gentity_t *self )
{
	if( self->mapEntity.spawnflags & 1 )
	{
		glm::vec3 mins = self->mapEntity.restingPosition + VEC2GLM( self->r.mins );
		glm::vec3 maxs = self->mapEntity.restingPosition + VEC2GLM( self->r.maxs );
		G_BotAddObstacle( mins, maxs, self->num() );
		G_CM_LinkEntity( self );
	}
	else
	{
		G_BotRemoveObstacle( self->num() );
		G_CM_UnlinkEntity( self );
	}
}
/*
====================
SP_func_spawn
====================
*/
void SP_func_spawn( gentity_t *self )
{
	//ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	self->s.eType = entityType_t::ET_MOVER;
	self->r.contents |= CONTENTS_MOVER;
	self->mapEntity.moverState = MOVER_POS1;
	VectorCopy( self->s.origin, self->mapEntity.restingPosition );

	if ( self->mapEntity.model.c_str()[0] == '*' )
	{
		G_CM_SetBrushModel( self, self->mapEntity.model );
	}
	else
	{
		self->s.modelindex = G_ModelIndex( self->mapEntity.model );
		VectorCopy( self->s.angles, self->s.apos.trBase );
	}

	self->act = func_spawn_act;
	self->reset = func_spawn_reset;
}

static void func_destructable_die( gentity_t *self, gentity_t*, gentity_t *attacker, meansOfDeath_t )
{
	G_FireEntity( self, attacker );
	G_BotRemoveObstacle( self->num() );
	G_CM_UnlinkEntity( self );

	G_RadiusDamage( self->mapEntity.restingPosition, attacker, self->splashDamage, self->splashRadius, self,
	                DAMAGE_KNOCKBACK, MOD_TRIGGER_HURT );
}


static void func_destructable_reset( gentity_t *self )
{
	glm::vec3 mins = VEC2GLM( self->r.absmin );
	glm::vec3 maxs = VEC2GLM( self->r.absmax );
	G_BotAddObstacle( mins, maxs, self->num() );
	G_ResetIntField(&self->health, true, self->mapEntity.config.health, self->mapEntity.eclass->config.health, 100);
}

/*
====================
Use_func_destructable
====================
*/
static void func_destructable_act( gentity_t *self, gentity_t *caller, gentity_t *activator )
{
  if( self->r.linked )
  {
    G_CM_UnlinkEntity( self );
    if( self->health <= 0 )
    {
    	func_destructable_die( self, caller, activator, MOD_UNKNOWN );
    }
  }
  else
  {
    G_CM_LinkEntity( self );
    G_KillBrushModel( self, activator );
		func_destructable_reset ( self );
  }
}

/*
====================
SP_func_destructable
====================
*/
void SP_func_destructable( gentity_t *self )
{
	SP_ConditionFields( self );

	G_SpawnInt( "damage", "0", &self->splashDamage );
	G_SpawnInt( "radius", "0", &self->splashRadius );

	if ( ( self->splashDamage != 0 && self->splashRadius == 0 )
			|| ( self->splashDamage == 0 && self->splashRadius != 0 ) )
	{
		Log::Warn( "Destructible entity %d have only one of: \"radius, damage\", which makes no sense.", self->num() );
	}

	//ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	self->s.eType = entityType_t::ET_MOVER;
	self->r.contents |= CONTENTS_MOVER;
	self->mapEntity.moverState = MOVER_POS1;
	VectorCopy( self->s.origin, self->mapEntity.restingPosition );

	if ( self->mapEntity.model[0] == '*' )
	{
		G_CM_SetBrushModel( self, self->mapEntity.model );
	}
	else
	{
		self->s.modelindex = G_ModelIndex( self->mapEntity.model );
		VectorCopy( self->s.angles, self->s.apos.trBase );
	}

	self->reset = func_destructable_reset;
	self->die = func_destructable_die;
	self->act = func_destructable_act;

	if( !( self->mapEntity.spawnflags & 1 ) )
	{
		G_CM_LinkEntity( self );
	}
}
