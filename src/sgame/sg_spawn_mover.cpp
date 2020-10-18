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

#define DEFAULT_FUNC_TRAIN_SPEED 100

/*
===============================================================================

PUSHMOVE

===============================================================================
*/

struct pushed_t
{
	gentity_t *ent;
	vec3_t    origin;
	vec3_t    angles;
	float     deltayaw;
};

pushed_t pushed[ MAX_GENTITIES ], *pushed_p;

/*
============
G_TestEntityPosition

============
*/
gentity_t *G_TestEntityPosition( gentity_t *ent )
{
	trace_t tr;

	if ( ent->client )
	{
		trap_Trace( &tr, ent->client->ps.origin, ent->r.mins, ent->r.maxs, ent->client->ps.origin,
		            ent->s.number, ent->clipmask, 0 );
	}
	else
	{
		trap_Trace( &tr, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, ent->s.pos.trBase,
		            ent->s.number, ent->clipmask, 0 );
	}

	if ( tr.startsolid )
	{
		return &g_entities[ tr.entityNum ];
	}

	return nullptr;
}

/*
================
G_CreateRotationMatrix
================
*/
void G_CreateRotationMatrix( vec3_t angles, vec3_t matrix[ 3 ] )
{
	AngleVectors( angles, matrix[ 0 ], matrix[ 1 ], matrix[ 2 ] );
	VectorInverse( matrix[ 1 ] );
}

/*
================
G_TransposeMatrix
================
*/
void G_TransposeMatrix( vec3_t matrix[ 3 ], vec3_t transpose[ 3 ] )
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
void G_RotatePoint( vec3_t point, vec3_t matrix[ 3 ] )
{
	vec3_t tvec;

	VectorCopy( point, tvec );
	point[ 0 ] = DotProduct( matrix[ 0 ], tvec );
	point[ 1 ] = DotProduct( matrix[ 1 ], tvec );
	point[ 2 ] = DotProduct( matrix[ 2 ], tvec );
}

/*
==================
G_TryPushingEntity

Returns false if the move is blocked
==================
*/
bool G_TryPushingEntity( gentity_t *check, gentity_t *pusher, vec3_t move, vec3_t amove )
{
	vec3_t    matrix[ 3 ], transpose[ 3 ];
	vec3_t    org, org2, move2;
	gentity_t *block;

	// EF_MOVER_STOP will just stop when contacting another entity
	// instead of pushing it, but entities can still ride on top of it
	if ( ( pusher->s.eFlags & EF_MOVER_STOP ) &&
	     check->s.groundEntityNum != pusher->s.number )
	{
		return false;
	}

	//don't try to move buildables unless standing on a mover
	if ( check->s.eType == entityType_t::ET_BUILDABLE &&
	     check->s.groundEntityNum != pusher->s.number )
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

	if ( check->client )
	{
		VectorSubtract( check->client->ps.origin, pusher->r.currentOrigin, org );
	}
	else
	{
		VectorSubtract( check->s.pos.trBase, pusher->r.currentOrigin, org );
	}

	VectorCopy( org, org2 );
	G_RotatePoint( org2, matrix );
	VectorSubtract( org2, org, move2 );
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
	if ( check->s.groundEntityNum != pusher->s.number )
	{
		check->s.groundEntityNum = ENTITYNUM_NONE;
	}

	block = G_TestEntityPosition( check );

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

		trap_LinkEntity( check );
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
bool G_MoverPush( gentity_t *pusher, vec3_t move, vec3_t amove, gentity_t **obstacle )
{
	int       i, e;
	gentity_t *check;
	vec3_t    mins, maxs;
	pushed_t  *p;
	int       entityList[ MAX_GENTITIES ];
	int       listedEntities;
	vec3_t    totalMins, totalMaxs;

	*obstacle = nullptr;

	// mins/maxs are the bounds at the destination
	// totalMins / totalMaxs are the bounds for the entire move
	if ( pusher->r.currentAngles[ 0 ] || pusher->r.currentAngles[ 1 ] || pusher->r.currentAngles[ 2 ]
	     || amove[ 0 ] || amove[ 1 ] || amove[ 2 ] )
	{
		float radius;

		radius = RadiusFromBounds( pusher->r.mins, pusher->r.maxs );

		for ( i = 0; i < 3; i++ )
		{
			mins[ i ] = pusher->r.currentOrigin[ i ] + move[ i ] - radius;
			maxs[ i ] = pusher->r.currentOrigin[ i ] + move[ i ] + radius;
			totalMins[ i ] = mins[ i ] - move[ i ];
			totalMaxs[ i ] = maxs[ i ] - move[ i ];
		}
	}
	else
	{
		for ( i = 0; i < 3; i++ )
		{
			mins[ i ] = pusher->r.absmin[ i ] + move[ i ];
			maxs[ i ] = pusher->r.absmax[ i ] + move[ i ];
		}

		VectorCopy( pusher->r.absmin, totalMins );
		VectorCopy( pusher->r.absmax, totalMaxs );

		for ( i = 0; i < 3; i++ )
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
	trap_UnlinkEntity( pusher );

	listedEntities = trap_EntitiesInBox( totalMins, totalMaxs, entityList, MAX_GENTITIES );

	// move the pusher to its final position
	VectorAdd( pusher->r.currentOrigin, move, pusher->r.currentOrigin );
	VectorAdd( pusher->r.currentAngles, amove, pusher->r.currentAngles );
	trap_LinkEntity( pusher );

	// see if any solid entities are inside the final position
	for ( e = 0; e < listedEntities; e++ )
	{
		check = &g_entities[ entityList[ e ] ];

		// only push items and players
		if ( check->s.eType != entityType_t::ET_ITEM && check->s.eType != entityType_t::ET_BUILDABLE &&
		     check->s.eType != entityType_t::ET_CORPSE && check->s.eType != entityType_t::ET_PLAYER &&
		     !check->physicsObject )
		{
			continue;
		}

		// if the entity is standing on the pusher, it will definitely be moved
		if ( check->s.groundEntityNum != pusher->s.number )
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
		for ( p = pushed_p - 1; p >= pushed; p-- )
		{
			VectorCopy( p->origin, p->ent->s.pos.trBase );
			VectorCopy( p->angles, p->ent->s.apos.trBase );

			if ( p->ent->client )
			{
				p->ent->client->ps.delta_angles[ YAW ] = p->deltayaw;
				VectorCopy( p->origin, p->ent->client->ps.origin );
			}

			trap_LinkEntity( p->ent );
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
void G_MoverGroup( gentity_t *ent )
{
	vec3_t    move, amove;
	gentity_t *part, *obstacle;
	vec3_t    origin, angles;

	obstacle = nullptr;

	// make sure all group slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_p = pushed;

	for ( part = ent; part; part = part->groupChain )
	{
		if ( part->s.pos.trType == trType_t::TR_STATIONARY &&
		     part->s.apos.trType == trType_t::TR_STATIONARY )
		{
			continue;
		}

		// get current position
		BG_EvaluateTrajectory( &part->s.pos, level.time, origin );
		BG_EvaluateTrajectory( &part->s.apos, level.time, angles );
		VectorSubtract( origin, part->r.currentOrigin, move );
		VectorSubtract( angles, part->r.currentAngles, amove );

		if ( !G_MoverPush( part, move, amove, &obstacle ) )
		{
			break; // move was blocked
		}
	}

	if ( part )
	{
		// go back to the previous position
		for ( part = ent; part; part = part->groupChain )
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
			trap_LinkEntity( part );
		}

		// if the pusher has a "blocked" function, call it
		if ( ent->blocked )
		{
			ent->blocked( ent, obstacle );
		}

		return;
	}

	// the move succeeded
	for ( part = ent; part; part = part->groupChain )
	{
		// call the reached function if time is at or past end point
		if ( part->s.pos.trType == trType_t::TR_LINEAR_STOP )
		{
			if ( level.time >= part->s.pos.trTime + part->s.pos.trDuration )
			{
				if ( part->reached )
				{
					part->reached( part );
				}
			}
		}

		if ( part->s.apos.trType == trType_t::TR_LINEAR_STOP )
		{
			if ( level.time >= part->s.apos.trTime + part->s.apos.trDuration )
			{
				if ( part->reached )
				{
					part->reached( part );
				}
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
void SetMoverState( gentity_t *ent, moverState_t moverState, int time )
{
	vec3_t delta;
	float  f;

	ent->moverState = moverState;

	ent->s.pos.trTime = time;
	ent->s.apos.trTime = time;

	switch ( moverState )
	{
		case MOVER_POS1:
			VectorCopy( ent->restingPosition, ent->s.pos.trBase );
			ent->s.pos.trType = trType_t::TR_STATIONARY;

			if ( !Q_stricmp( ent->classname, "func_door" ) && ( ent->names[0] || ent->takedamage  ) )
			{
				vec3_t mins, maxs;
				VectorAdd( ent->restingPosition, ent->r.mins, mins );
				VectorAdd( ent->restingPosition, ent->r.maxs, maxs );
				trap_BotAddObstacle( mins, maxs, &ent->obstacleHandle );
			}
			break;

		case MOVER_POS2:
			VectorCopy( ent->activatedPosition, ent->s.pos.trBase );
			ent->s.pos.trType = trType_t::TR_STATIONARY;

			if ( !Q_stricmp( ent->classname, "func_door" ) && ( ent->names[0] || ent->takedamage ) )
			{
				if ( ent->obstacleHandle )
				{
					trap_BotRemoveObstacle( ent->obstacleHandle );
					ent->obstacleHandle = 0;
				}
			}
			break;

		case MOVER_1TO2:
			VectorCopy( ent->restingPosition, ent->s.pos.trBase );
			VectorSubtract( ent->activatedPosition, ent->restingPosition, delta );
			f = 1000.0 / ent->s.pos.trDuration;
			VectorScale( delta, f, ent->s.pos.trDelta );
			ent->s.pos.trType = trType_t::TR_LINEAR_STOP;
			break;

		case MOVER_2TO1:
			VectorCopy( ent->activatedPosition, ent->s.pos.trBase );
			VectorSubtract( ent->restingPosition, ent->activatedPosition, delta );
			f = 1000.0 / ent->s.pos.trDuration;
			VectorScale( delta, f, ent->s.pos.trDelta );
			ent->s.pos.trType = trType_t::TR_LINEAR_STOP;
			break;

		case ROTATOR_POS1:
			VectorCopy( ent->restingPosition, ent->s.apos.trBase );
			ent->s.apos.trType = trType_t::TR_STATIONARY;
			break;

		case ROTATOR_POS2:
			VectorCopy( ent->activatedPosition, ent->s.apos.trBase );
			ent->s.apos.trType = trType_t::TR_STATIONARY;
			break;

		case ROTATOR_1TO2:
			VectorCopy( ent->restingPosition, ent->s.apos.trBase );
			VectorSubtract( ent->activatedPosition, ent->restingPosition, delta );
			f = 1000.0 / ent->s.apos.trDuration;
			VectorScale( delta, f, ent->s.apos.trDelta );
			ent->s.apos.trType = trType_t::TR_LINEAR_STOP;
			break;

		case ROTATOR_2TO1:
			VectorCopy( ent->activatedPosition, ent->s.apos.trBase );
			VectorSubtract( ent->restingPosition, ent->activatedPosition, delta );
			f = 1000.0 / ent->s.apos.trDuration;
			VectorScale( delta, f, ent->s.apos.trDelta );
			ent->s.apos.trType = trType_t::TR_LINEAR_STOP;
			break;

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

	trap_LinkEntity( ent );
}

/*
================
MatchGroup

All entities in a mover group will move from pos1 to pos2
================
*/
void MatchGroup( gentity_t *groupLeader, int moverState, int time )
{
	gentity_t *slave;

	for ( slave = groupLeader; slave; slave = slave->groupChain )
	{
		SetMoverState( slave, (moverState_t) moverState, time );
	}
}

/*
================
MasterOf
================
*/
gentity_t *MasterOf( gentity_t *ent )
{
	if ( ent->groupMaster )
	{
		return ent->groupMaster;
	}
	else
	{
		return ent;
	}
}

/*
================
GetMoverGroupState

Returns a MOVER_* value representing the phase (either one
 of pos1, 1to2, pos2, or 2to1) of a mover group as a whole.
================
*/
moverState_t GetMoverGroupState( gentity_t *ent )
{
	bool restingPosition = false;

	for ( ent = MasterOf( ent ); ent; ent = ent->groupChain )
	{
		if ( ent->moverState == MOVER_POS1 || ent->moverState == ROTATOR_POS1 )
		{
			restingPosition = true;
		}
		else if ( ent->moverState == MOVER_1TO2 || ent->moverState == ROTATOR_1TO2 )
		{
			return MOVER_1TO2;
		}
		else if ( ent->moverState == MOVER_2TO1 || ent->moverState == ROTATOR_2TO1 )
		{
			return MOVER_2TO1;
		}
	}

	if ( restingPosition )
	{
		return MOVER_POS1;
	}
	else
	{
		return MOVER_POS2;
	}
}

/*
================
ReturnToPos1orApos1

Used only by a master movers.
================
*/

void BinaryMover_act( gentity_t *ent, gentity_t *other, gentity_t *activator );

void ReturnToPos1orApos1( gentity_t *ent )
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
void Think_ClosedModelDoor( gentity_t *ent )
{
	// play sound
	if ( ent->soundPos1 )
	{
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 );
	}

	// close areaportals
	if ( ent->groupMaster == ent || !ent->groupMaster )
	{
		trap_AdjustAreaPortalState( ent, false );
	}

	ent->moverState = MODEL_POS1;
}

/*
================
Think_CloseModelDoor
================
*/
void Think_CloseModelDoor( gentity_t *ent )
{
	int       entityList[ MAX_GENTITIES ];
	int       numEntities, i;
	gentity_t *clipBrush = ent->clipBrush;
	gentity_t *check;
	bool  canClose = true;

	numEntities = trap_EntitiesInBox( clipBrush->r.absmin, clipBrush->r.absmax, entityList, MAX_GENTITIES );

	//set brush solid
	trap_LinkEntity( ent->clipBrush );

	//see if any solid entities are inside the door
	for ( i = 0; i < numEntities; i++ )
	{
		check = &g_entities[ entityList[ i ] ];

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
			canClose = false;
		}
	}

	//something is blocking this door
	if ( !canClose )
	{
		//set brush non-solid
		trap_UnlinkEntity( ent->clipBrush );

		ent->nextthink = level.time + ent->config.wait.time;
		return;
	}

	//toggle door state
	ent->s.legsAnim = false;

	// play sound
	if ( ent->sound2to1 )
	{
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
	}

	ent->moverState = MODEL_2TO1;

	ent->think = Think_ClosedModelDoor;
	ent->nextthink = level.time + ent->config.speed;
}

/*
================
Think_OpenModelDoor
================
*/
void Think_OpenModelDoor( gentity_t *ent )
{
	//set brush non-solid
	trap_UnlinkEntity( ent->clipBrush );

	// stop the looping sound
	ent->s.loopSound = 0;

	// play sound
	if ( ent->soundPos2 )
	{
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos2 );
	}

	ent->moverState = MODEL_POS2;

	// return to pos1 after a delay
	ent->think = Think_CloseModelDoor;
	ent->nextthink = level.time + ent->config.wait.time;

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
void BinaryMover_reached( gentity_t *ent )
{
	gentity_t *master = MasterOf( ent );

	// stop the looping sound
	ent->s.loopSound = 0;

	if ( ent->moverState == MOVER_1TO2 )
	{
		// reached pos2
		SetMoverState( ent, MOVER_POS2, level.time );

		// play sound
		if ( ent->soundPos2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos2 );
		}

		// return to pos1 after a delay
		master->think = ReturnToPos1orApos1;
		master->nextthink = std::max( master->nextthink, level.time + (int) ent->config.wait.time );

		// fire targets
		if ( !ent->activator )
		{
			ent->activator = ent;
		}

		G_FireEntity( ent, ent->activator );
	}
	else if ( ent->moverState == MOVER_2TO1 )
	{
		// reached pos1
		SetMoverState( ent, MOVER_POS1, level.time );

		// play sound
		if ( ent->soundPos1 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 );
		}

		// close areaportals
		if ( ent->groupMaster == ent || !ent->groupMaster )
		{
			trap_AdjustAreaPortalState( ent, false );
		}
	}
	else if ( ent->moverState == ROTATOR_1TO2 )
	{
		// reached pos2
		SetMoverState( ent, ROTATOR_POS2, level.time );

		// play sound
		if ( ent->soundPos2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos2 );
		}

		// return to apos1 after a delay
		master->think = ReturnToPos1orApos1;
		master->nextthink = std::max( master->nextthink, level.time + (int) ent->config.wait.time );

		// fire targets
		if ( !ent->activator )
		{
			ent->activator = ent;
		}

		G_FireEntity( ent, ent->activator );
	}
	else if ( ent->moverState == ROTATOR_2TO1 )
	{
		// reached pos1
		SetMoverState( ent, ROTATOR_POS1, level.time );

		// play sound
		if ( ent->soundPos1 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 );
		}

		// close areaportals
		if ( ent->groupMaster == ent || !ent->groupMaster )
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
	if ( ent->names[ 0 ] && other && other->client )
	{
		return;
	}

	// only the master should be used
	if ( ent->flags & FL_GROUPSLAVE )
	{
		BinaryMover_act( ent->groupMaster, other, activator );
		return;
	}

	ent->activator = activator;

	master = MasterOf( ent );
	groupState = GetMoverGroupState( ent );

	for ( ent = master; ent; ent = ent->groupChain )
	{
	//ind
	if ( ent->moverState == MOVER_POS1 )
	{
		// start moving 50 msec later, because if this was player-
		// triggered, level.time hasn't been advanced yet
		SetMoverState( ent, MOVER_1TO2, level.time + 50 );

		// starting sound
		if ( ent->sound1to2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}

		// looping sound
		ent->s.loopSound = ent->soundIndex;

		// open areaportal
		if ( ent->groupMaster == ent || !ent->groupMaster )
		{
			trap_AdjustAreaPortalState( ent, true );
		}
	}
	else if ( ent->moverState == MOVER_POS2 &&
	          !( groupState == MOVER_1TO2 || other == master ) )
	{
		// if all the way up, just delay before coming down
		master->think = ReturnToPos1orApos1;
		master->nextthink = std::max( master->nextthink, level.time + (int) ent->config.wait.time );
	}
	else if ( ent->moverState == MOVER_POS2 &&
	          ( groupState == MOVER_1TO2 || other == master ) )
	{
		// start moving 50 msec later, because if this was player-
		// triggered, level.time hasn't been advanced yet
		SetMoverState( ent, MOVER_2TO1, level.time + 50 );

		// starting sound
		if ( ent->sound2to1 )
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );

		// looping sound
		ent->s.loopSound = ent->soundIndex;

		// open areaportal
		if ( ent->groupMaster == ent || !ent->groupMaster )
			trap_AdjustAreaPortalState( ent, true );
	}
	else if ( ent->moverState == MOVER_2TO1 )
	{
		// only partway down before reversing
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.pos.trTime;

		if ( partial > total )
		{
			partial = total;
		}

		SetMoverState( ent, MOVER_1TO2, level.time - ( total - partial ) );

		if ( ent->sound1to2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}
	}
	else if ( ent->moverState == MOVER_1TO2 )
	{
		// only partway up before reversing
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.pos.trTime;

		if ( partial > total )
		{
			partial = total;
		}

		SetMoverState( ent, MOVER_2TO1, level.time - ( total - partial ) );

		if ( ent->sound2to1 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
		}
	}
	else if ( ent->moverState == ROTATOR_POS1 )
	{
		// start moving 50 msec later, because if this was player-
		// triggered, level.time hasn't been advanced yet
		SetMoverState( ent, ROTATOR_1TO2, level.time + 50 );

		// starting sound
		if ( ent->sound1to2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}

		// looping sound
		ent->s.loopSound = ent->soundIndex;

		// open areaportal
		if ( ent->groupMaster == ent || !ent->groupMaster )
		{
			trap_AdjustAreaPortalState( ent, true );
		}
	}
	else if ( ent->moverState == ROTATOR_POS2 &&
	          !( groupState == MOVER_1TO2 || other == master ) )
	{
		// if all the way up, just delay before coming down
		master->think = ReturnToPos1orApos1;
		master->nextthink = std::max( master->nextthink, level.time + (int) ent->config.wait.time );
	}
	else if ( ent->moverState == ROTATOR_POS2 &&
	          ( groupState == MOVER_1TO2 || other == master ) )
	{
		// start moving 50 msec later, because if this was player-
		// triggered, level.time hasn't been advanced yet
		SetMoverState( ent, ROTATOR_2TO1, level.time + 50 );

		// starting sound
		if ( ent->sound2to1 )
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );

		// looping sound
		ent->s.loopSound = ent->soundIndex;

		// open areaportal
		if ( ent->groupMaster == ent || !ent->groupMaster )
			trap_AdjustAreaPortalState( ent, true );
	}
	else if ( ent->moverState == ROTATOR_2TO1 )
	{
		// only partway down before reversing
		total = ent->s.apos.trDuration;
		partial = level.time - ent->s.apos.trTime;

		if ( partial > total )
		{
			partial = total;
		}

		SetMoverState( ent, ROTATOR_1TO2, level.time - ( total - partial ) );

		if ( ent->sound1to2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}
	}
	else if ( ent->moverState == ROTATOR_1TO2 )
	{
		// only partway up before reversing
		total = ent->s.apos.trDuration;
		partial = level.time - ent->s.apos.trTime;

		if ( partial > total )
		{
			partial = total;
		}

		SetMoverState( ent, ROTATOR_2TO1, level.time - ( total - partial ) );

		if ( ent->sound2to1 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
		}
	}
	else if ( ent->moverState == MODEL_POS1 )
	{
		//toggle door state
		ent->s.legsAnim = true;

		ent->think = Think_OpenModelDoor;
		ent->nextthink = level.time + ent->config.speed;

		// starting sound
		if ( ent->sound1to2 )
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}

		// looping sound
		ent->s.loopSound = ent->soundIndex;

		// open areaportal
		if ( ent->groupMaster == ent || !ent->groupMaster )
		{
			trap_AdjustAreaPortalState( ent, true );
		}

		ent->moverState = MODEL_1TO2;
	}
	else if ( ent->moverState == MODEL_POS2 )
	{
		// if all the way up, just delay before coming down
		ent->nextthink = level.time + ent->config.wait.time;
	}
	//outd
	}
}

void reset_moverspeed( gentity_t *self, float fallbackSpeed )
{
	vec3_t   move;
	float    distance;

	if(!fallbackSpeed)
		Sys::Drop("No default speed was supplied to reset_moverspeed for entity #%i of type %s.\n", self->s.number, self->classname);

	G_ResetFloatField(&self->speed, true, self->config.speed, self->eclass->config.speed, fallbackSpeed);

	// reset duration only for linear movement else func_bobbing will not move
	if ( self->s.pos.trType != trType_t::TR_SINE )
	{
		// calculate time to reach second position from speed
		VectorSubtract( self->activatedPosition, self->restingPosition, move );
		distance = VectorLength( move );

		VectorScale( move, self->speed, self->s.pos.trDelta );
		self->s.pos.trDuration = distance * 1000 / self->speed;

		if ( self->s.pos.trDuration <= 0 )
		{
			self->s.pos.trDuration = 1;
		}
	}
}

static void SP_ConstantLightField( gentity_t *self )
{
	bool  lightSet, colorSet;
	float     light;
	vec3_t    color;

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat( "light", "100", &light );
	colorSet = G_SpawnVector( "color", "1 1 1", color );

	if ( lightSet || colorSet )
	{
		int r, g, b, i;

		r = color[ 0 ] * 255;

		if ( r > 255 )
		{
			r = 255;
		}

		g = color[ 1 ] * 255;

		if ( g > 255 )
		{
			g = 255;
		}

		b = color[ 2 ] * 255;

		if ( b > 255 )
		{
			b = 255;
		}

		i = light / 4;

		if ( i > 255 )
		{
			i = 255;
		}

		self->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}
}

/*
================
InitMover

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
void InitMover( gentity_t *ent )
{
	char     *groupName;

	// if the "model2" key is set, use a separate model
	// for drawing, but clip against the brushes
	if ( ent->model2 )
	{
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	}

	SP_ConstantLightField( ent );

	ent->act = BinaryMover_act;
	ent->reached = BinaryMover_reached;

	if ( G_SpawnString( "group", "", &groupName ) )
	{
		ent->groupName = G_CopyString( groupName );
	}
	else if ( G_SpawnString( "team", "", &groupName ) )
	{
		G_WarnAboutDeprecatedEntityField( ent, "group", "team", ENT_V_RENAMED );
		ent->groupName = G_CopyString( groupName );
	}

	ent->moverState = MOVER_POS1;
	ent->s.eType = entityType_t::ET_MOVER;
	ent->r.contents |= CONTENTS_MOVER;
	VectorCopy( ent->restingPosition, ent->r.currentOrigin );
	trap_LinkEntity( ent );

	ent->s.pos.trType = trType_t::TR_STATIONARY;
	VectorCopy( ent->restingPosition, ent->s.pos.trBase );
}

void reset_rotatorspeed( gentity_t *self, float fallbackSpeed )
{
	vec3_t   move;
	float    angle;

	if(!fallbackSpeed)
		Sys::Drop("No default speed was supplied to reset_rotatorspeed for entity #%i of type %s.\n", self->s.number, self->classname);

	// calculate time to reach second position from speed
	VectorSubtract( self->activatedPosition, self->restingPosition, move );
	angle = VectorLength( move );

	G_ResetFloatField(&self->speed, true, self->config.speed, self->eclass->config.speed, fallbackSpeed);

	VectorScale( move, self->speed, self->s.apos.trDelta );
	self->s.apos.trDuration = angle * 1000 / self->speed;

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
void InitRotator( gentity_t *ent )
{
	char     *groupName;

	// if the "model2" key is set, use a separate model
	// for drawing, but clip against the brushes
	if ( ent->model2 )
	{
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	}

	SP_ConstantLightField( ent );

	ent->act = BinaryMover_act;
	ent->reached = BinaryMover_reached;

	if ( G_SpawnString( "group", "", &groupName ) )
	{
		ent->groupName = G_CopyString( groupName );
	}
	else if ( G_SpawnString( "team", "", &groupName ) )
	{
		G_WarnAboutDeprecatedEntityField( ent, "group", "team", ENT_V_RENAMED );
		ent->groupName = G_CopyString( groupName );
	}

	ent->moverState = ROTATOR_POS1;
	ent->s.eType = entityType_t::ET_MOVER;
	ent->r.contents |= CONTENTS_MOVER;
	VectorCopy( ent->restingPosition, ent->r.currentAngles );
	trap_LinkEntity( ent );

	ent->s.apos.trType = trType_t::TR_STATIONARY;
	VectorCopy( ent->restingPosition, ent->s.apos.trBase );
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
void func_door_block( gentity_t *self, gentity_t *other )
{
	// remove anything other than a client or buildable
	if ( !other->client && other->s.eType != entityType_t::ET_BUILDABLE )
	{
		G_FreeEntity( other );
		return;
	}

	if ( self->damage )
	{
		other->entity->Damage((float)self->damage, self, Util::nullopt, Util::nullopt, 0, MOD_CRUSH);
	}

	if ( self->spawnflags & 4 )
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
void door_trigger_touch( gentity_t *self, gentity_t *other, trace_t* )
{
	moverState_t groupState;

	//buildables don't trigger movers
	if ( other->s.eType == entityType_t::ET_BUILDABLE )
	{
		return;
	}

	groupState = GetMoverGroupState( self->parent );

	if ( groupState != MOVER_1TO2 )
	{
		BinaryMover_act( self->parent, self, other );
	}
}

void Think_MatchGroup( gentity_t *self )
{
	if ( self->flags & FL_GROUPSLAVE )
	{
		return;
	}
	MatchGroup( self, self->moverState, level.time );
}

/*
======================
Think_SpawnNewDoorTrigger

All of the parts of a door have been spawned, so create
a trigger that encloses all of them
======================
*/
void Think_SpawnNewDoorTrigger( gentity_t *self )
{
	gentity_t *other;
	vec3_t    mins, maxs;
	int       i, best;

	// find the bounds of everything on the group
	VectorCopy( self->r.absmin, mins );
	VectorCopy( self->r.absmax, maxs );

	for ( other = self->groupChain; other; other = other->groupChain )
	{
		AddPointToBounds( other->r.absmin, mins, maxs );
		AddPointToBounds( other->r.absmax, mins, maxs );
	}

	// find the thinnest axis, which will be the one we expand
	best = 0;

	for ( i = 1; i < 3; i++ )
	{
		if ( maxs[ i ] - mins[ i ] < maxs[ best ] - mins[ best ] )
		{
			best = i;
		}
	}

	maxs[ best ] += self->config.triggerRange;
	mins[ best ] -= self->config.triggerRange;

	// create a trigger with this size
	other = G_NewEntity();
	other->classname = S_DOOR_SENSOR;
	VectorCopy( mins, other->r.mins );
	VectorCopy( maxs, other->r.maxs );
	other->parent = self;
	other->r.contents = CONTENTS_TRIGGER;
	other->touch = door_trigger_touch;
	// remember the thinnest axis
	other->customNumber = best;
	trap_LinkEntity( other );

	if ( self->moverState < MODEL_POS1 )
	{
		Think_MatchGroup( self );
	}
}

void func_door_reset( gentity_t *self )
{
	G_ResetIntField(&self->health, true, self->config.health, self->eclass->config.health, 0);
	G_ResetIntField(&self->damage, true, self->config.damage, self->eclass->config.damage, 2);

	self->takedamage = !!self->health;

	reset_moverspeed( self, 400 );
}

void func_door_use( gentity_t *self, gentity_t *caller, gentity_t *activator )
{
	if (GetMoverGroupState( self ) != MOVER_1TO2 )
		BinaryMover_act( self, caller, activator );
}

void SP_func_door( gentity_t *self )
{
	vec3_t abs_movedir;
	float  distance;
	vec3_t size;
	float  lip;

	if( !self->sound1to2 )
	{
		self->sound1to2 = G_SoundIndex( "sound/movers/doors/dr1_strt" );
	}
	if( !self->sound2to1 )
	{
		self->sound2to1 = G_SoundIndex( "sound/movers/doors/dr1_strt" );
	}
	if( !self->soundPos1 )
	{
		self->soundPos1 = G_SoundIndex( "sound/movers/doors/dr1_end" );
	}
	if( !self->soundPos2 )
	{
		self->soundPos2 = G_SoundIndex( "sound/movers/doors/dr1_end" );
	}

	self->blocked = func_door_block;
	self->reset = func_door_reset;
	self->use = func_door_use;

	// default wait of 2 seconds
	if ( !self->config.wait.time )
	{
		self->config.wait.time = 2;
	}

	self->config.wait.time *= 1000;

	// default trigger range of 72 units (saved in noise_index)
	G_SpawnInt( "range", "72", &self->config.triggerRange );

	if ( self->config.triggerRange < 0 )
	{
		self->config.triggerRange = 72;
	}

	// default lip of 8 units
	G_SpawnFloat( "lip", "8", &lip );

	// first position at start
	VectorCopy( self->s.origin, self->restingPosition );

	// calculate second position
	trap_SetBrushModel( self, self->model );
	G_SetMovedir( self->s.angles, self->movedir );
	abs_movedir[ 0 ] = fabs( self->movedir[ 0 ] );
	abs_movedir[ 1 ] = fabs( self->movedir[ 1 ] );
	abs_movedir[ 2 ] = fabs( self->movedir[ 2 ] );
	VectorSubtract( self->r.maxs, self->r.mins, size );
	distance = DotProduct( abs_movedir, size ) - lip;
	VectorMA( self->restingPosition, distance, self->movedir, self->activatedPosition );

	// if "start_open", reverse position 1 and 2
	if ( self->spawnflags & 1 )
	{
		vec3_t temp;

		VectorCopy( self->activatedPosition, temp );
		VectorCopy( self->s.origin, self->activatedPosition );
		VectorCopy( temp, self->restingPosition );
	}

	InitMover( self );

	self->nextthink = level.time + FRAMETIME;

	if ( self->names[ 0 ] || self->config.health ) //FIXME wont work yet with class fallbacks
	{
		// non touch/shoot doors
		self->think = Think_MatchGroup;
	}
	else
	{
		self->think = Think_SpawnNewDoorTrigger;
	}
}

void func_door_rotating_reset( gentity_t *self )
{
	G_ResetIntField(&self->health, true, self->config.health, self->eclass->config.health, 0);

	self->takedamage = !!self->health;

	reset_rotatorspeed( self, 120 );
}

void SP_func_door_rotating( gentity_t *self )
{
	if( !self->sound1to2 )
	{
		self->sound1to2 = G_SoundIndex( "sound/movers/doors/dr1_strt" );
	}
	if( !self->sound2to1 )
	{
		self->sound2to1 = G_SoundIndex( "sound/movers/doors/dr1_strt" );
	}
	if( !self->soundPos1 )
	{
		self->soundPos1 = G_SoundIndex( "sound/movers/doors/dr1_end" );
	}
	if( !self->soundPos2 )
	{
		self->soundPos2 = G_SoundIndex( "sound/movers/doors/dr1_end" );
	}

	self->blocked = func_door_block;
	self->reset = func_door_rotating_reset;
	self->use = func_door_use;

	// if speed is negative, positize it and add reverse flag
	if ( self->config.speed < 0 )
	{
		self->config.speed *= -1;
		self->spawnflags |= 8;
	}

	// default of 2 seconds
	if ( !self->config.wait.time )
	{
		self->config.wait.time = 2;
	}

	self->config.wait.time *= 1000;

	// default trigger range of 72 units (saved in noise_index)
	G_SpawnInt( "range", "72", &self->config.triggerRange );

	if ( self->config.triggerRange < 0 )
	{
		self->config.triggerRange = 72;
	}

	// set the axis of rotation
	VectorClear( self->movedir );
	VectorClear( self->s.angles );

	if ( self->spawnflags & 32 )
	{
		self->movedir[ 2 ] = 1.0;
	}
	else if ( self->spawnflags & 64 )
	{
		self->movedir[ 0 ] = 1.0;
	}
	else
	{
		self->movedir[ 1 ] = 1.0;
	}

	// reverse direction if necessary
	if ( self->spawnflags & 8 )
	{
		VectorNegate( self->movedir, self->movedir );
	}

	G_SpawnFloat( "rotatorAngle", "0", &self->rotatorAngle );

	// default distance of 90 degrees. This is something the mapper should not
	// leave out, so we'll tell him if he does.
	if ( !self->rotatorAngle )
	{
		Log::Warn( "%s at %s with no rotatorAngle set.",
		          self->classname, vtos( self->s.origin ) );

		self->rotatorAngle = 90.0;
	}

	VectorCopy( self->s.angles, self->restingPosition );
	trap_SetBrushModel( self, self->model );
	VectorMA( self->restingPosition, self->rotatorAngle, self->movedir, self->activatedPosition );

	// if "start_open", reverse position 1 and 2
	if ( self->spawnflags & 1 )
	{
		vec3_t temp;

		VectorCopy( self->activatedPosition, temp );
		VectorCopy( self->s.angles, self->activatedPosition );
		VectorCopy( temp, self->restingPosition );
		VectorNegate( self->movedir, self->movedir );
	}

	// set origin
	VectorCopy( self->s.origin, self->s.pos.trBase );
	VectorCopy( self->s.pos.trBase, self->r.currentOrigin );

	InitRotator( self );

	self->nextthink = level.time + FRAMETIME;

	if ( self->names[ 0 ] || self->config.health ) //FIXME wont work yet with class fallbacks
	{
		// non touch/shoot doors
		self->think = Think_MatchGroup;
	}
	else
	{
		self->think = Think_SpawnNewDoorTrigger;
	}
}

void func_door_model_reset( gentity_t *self )
{
	G_ResetFloatField(&self->speed, true, self->config.speed, self->eclass->config.speed, 200);
	G_ResetIntField(&self->health, true, self->config.health, self->eclass->config.health, 0);

	self->takedamage = !!self->health;

	self->s.torsoAnim = self->s.weapon * ( 1000.0f / self->speed ); //framerate
	if ( self->s.torsoAnim <= 0 )
	{
		self->s.torsoAnim = 1;
	}
}

void SP_func_door_model( gentity_t *self )
{
	char      *sound;
	gentity_t *clipBrush;

	if( !self->sound1to2 )
	{
		self->sound1to2 = G_SoundIndex( "sound/movers/doors/dr1_strt" );
	}
	if( !self->sound2to1 )
	{
		self->sound2to1 = G_SoundIndex( "sound/movers/doors/dr1_strt" );
	}
	if( !self->soundPos1 )
	{
		self->soundPos1 = G_SoundIndex( "sound/movers/doors/dr1_end" );
	}
	if( !self->soundPos2 )
	{
		self->soundPos2 = G_SoundIndex( "sound/movers/doors/dr1_end" );
	}

	self->reset = func_door_model_reset;
	self->use = func_door_use;

	//default wait of 2 seconds
	if ( self->config.wait.time <= 0 )
	{
		self->config.wait.time = 2;
	}

	self->config.wait.time *= 1000;

	// default trigger range of 72 units (saved in noise_index)
	G_SpawnInt( "range", "72", &self->config.triggerRange );

	if ( self->config.triggerRange < 0 )
	{
		self->config.triggerRange = 72;
	}

	//brush model
	clipBrush = self->clipBrush = G_NewEntity();
	clipBrush->model = self->model;
	trap_SetBrushModel( clipBrush, clipBrush->model );
	clipBrush->s.eType = entityType_t::ET_INVISIBLE;

	//copy the bounds back from the clipBrush so the
	//triggers can be made
	VectorCopy( clipBrush->r.absmin, self->r.absmin );
	VectorCopy( clipBrush->r.absmax, self->r.absmax );
	VectorCopy( clipBrush->r.mins, self->r.mins );
	VectorCopy( clipBrush->r.maxs, self->r.maxs );

	G_SpawnVector( "modelOrigin", "0 0 0", self->s.origin );

	G_SpawnVector( "scale", "1 1 1", self->s.origin2 );

	// if the "model2" key is set, use a separate model
	// for drawing, but clip against the brushes
	if ( !self->model2 )
	{
		Log::Warn("func_door_model %d spawned with no model2 key", self->s.number );
	}
	else
	{
		self->s.modelindex = G_ModelIndex( self->model2 );
	}

	// if the "noise" key is set, use a constant looping sound when moving
	if ( G_SpawnString( "noise", "", &sound ) )
	{
		self->soundIndex = G_SoundIndex( sound );
	}

	SP_ConstantLightField( self );

	self->act = BinaryMover_act;

	self->moverState = MODEL_POS1;
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

	self->s.misc = ( int ) self->animation[ 0 ]; //first frame
	self->s.weapon = abs( ( int ) self->animation[ 1 ] );  //number of frames

	//must be at least one frame -- mapper has forgotten animation key
	if ( self->s.weapon == 0 )
	{
		self->s.weapon = 1;
	}

	self->s.torsoAnim = 1; // stub value to avoid sigfpe

	trap_LinkEntity( self );

	if ( !( self->names[ 0 ] || self->config.health ) ) //FIXME wont work yet with class fallbacks
	{
		self->nextthink = level.time + FRAMETIME;
		self->think = Think_SpawnNewDoorTrigger;
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
void Touch_Plat( gentity_t *ent, gentity_t *other, trace_t* )
{
	// DONT_WAIT
	if ( ent->spawnflags & 1 )
	{
		return;
	}

	if ( !other->client || Entities::IsDead( other ) )
	{
		return;
	}

	// delay return-to-pos1 by one second
	if ( ent->moverState == MOVER_POS2 )
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
void Touch_PlatCenterTrigger( gentity_t *ent, gentity_t *other, trace_t* )
{
	if ( !other->client )
	{
		return;
	}

	if ( ent->parent->moverState == MOVER_POS1 )
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
void SpawnPlatSensor( gentity_t *self )
{
	gentity_t *sensor;
	vec3_t    tmin, tmax;

	// the middle trigger will be a thin trigger just
	// above the starting position
	sensor = G_NewEntity();
	sensor->classname = S_PLAT_SENSOR;
	sensor->touch = Touch_PlatCenterTrigger;
	sensor->r.contents = CONTENTS_TRIGGER;
	sensor->parent = self;

	tmin[ 0 ] = self->restingPosition[ 0 ] + self->r.mins[ 0 ] + 33;
	tmin[ 1 ] = self->restingPosition[ 1 ] + self->r.mins[ 1 ] + 33;
	tmin[ 2 ] = self->restingPosition[ 2 ] + self->r.mins[ 2 ];

	tmax[ 0 ] = self->restingPosition[ 0 ] + self->r.maxs[ 0 ] - 33;
	tmax[ 1 ] = self->restingPosition[ 1 ] + self->r.maxs[ 1 ] - 33;
	tmax[ 2 ] = self->restingPosition[ 2 ] + self->r.maxs[ 2 ] + 8;

	if ( tmax[ 0 ] <= tmin[ 0 ] )
	{
		tmin[ 0 ] = self->restingPosition[ 0 ] + ( self->r.mins[ 0 ] + self->r.maxs[ 0 ] ) * 0.5;
		tmax[ 0 ] = tmin[ 0 ] + 1;
	}

	if ( tmax[ 1 ] <= tmin[ 1 ] )
	{
		tmin[ 1 ] = self->restingPosition[ 1 ] + ( self->r.mins[ 1 ] + self->r.maxs[ 1 ] ) * 0.5;
		tmax[ 1 ] = tmin[ 1 ] + 1;
	}

	VectorCopy( tmin, sensor->r.mins );
	VectorCopy( tmax, sensor->r.maxs );

	trap_LinkEntity( sensor );
}

void SP_func_plat( gentity_t *self )
{
	float lip, height;

	if( !self->sound1to2 )
	{
		self->sound1to2 = G_SoundIndex( "sound/movers/plats/pt1_strt" );
	}
	if( !self->sound2to1 )
	{
		self->sound2to1 = G_SoundIndex( "sound/movers/plats/pt1_strt" );
	}
	if( !self->soundPos1 )
	{
		self->soundPos1 = G_SoundIndex( "sound/movers/plats/pt1_end" );
	}
	if( !self->soundPos2 )
	{
		self->soundPos2 = G_SoundIndex( "sound/movers/plats/pt1_end" );
	}

	VectorClear( self->s.angles );

	G_SpawnFloat( "lip", "8", &lip );

	G_ResetIntField(&self->damage, true, self->config.damage, self->eclass->config.damage, 2);

	if(!self->config.wait.time)
		self->config.wait.time = 1.0f;

	self->config.wait.time *= 1000;

	// create second position
	trap_SetBrushModel( self, self->model );

	if ( !G_SpawnFloat( "height", "0", &height ) )
	{
		height = ( self->r.maxs[ 2 ] - self->r.mins[ 2 ] ) - lip;
	}

	// pos1 is the rest (bottom) position, pos2 is the top
	VectorCopy( self->s.origin, self->activatedPosition );
	VectorCopy( self->activatedPosition, self->restingPosition );
	self->restingPosition[ 2 ] -= height;

	InitMover( self );
	reset_moverspeed( self, 400 );

	// touch function keeps the plat from returning while
	// a live player is standing on it
	self->touch = Touch_Plat;

	self->blocked = func_door_block;

	self->parent = self; // so it can be treated as a door

	// spawn the trigger if one hasn't been custom made
	if ( !self->names [ 0 ] )
	{
		SpawnPlatSensor( self );
	}
}

/*
===============================================================================

BUTTON

===============================================================================
*/

void Touch_Button( gentity_t *ent, gentity_t *other, trace_t* )
{
	if ( !other->client )
	{
		return;
	}

	if ( ent->moverState == MOVER_POS1 )
	{
		BinaryMover_act( ent, other, other );
	}
}

void func_button_use( gentity_t *self, gentity_t *caller, gentity_t *activator )
{
	if ( self->moverState == MOVER_POS1 )
		BinaryMover_act( self, caller, activator );
}

void func_button_reset( gentity_t *self )
{
	G_ResetIntField(&self->health, true, self->config.health, self->eclass->config.health, 0);

	self->takedamage = !!self->health;

	reset_moverspeed( self, 40 );
}

void SP_func_button( gentity_t *self )
{
	vec3_t abs_movedir;
	float  distance;
	vec3_t size;
	float  lip;

	if( !self->sound1to2 )
	{
		self->sound1to2 = G_SoundIndex( "sound/movers/switches/button1" );
	}

	self->reset = func_button_reset;

	if ( !self->config.wait.time )
	{
		self->config.wait.time = 1;
	}

	self->config.wait.time *= 1000;

	// first position
	VectorCopy( self->s.origin, self->restingPosition );

	// calculate second position
	trap_SetBrushModel( self, self->model );

	G_SpawnFloat( "lip", "4", &lip );

	G_SetMovedir( self->s.angles, self->movedir );
	abs_movedir[ 0 ] = fabs( self->movedir[ 0 ] );
	abs_movedir[ 1 ] = fabs( self->movedir[ 1 ] );
	abs_movedir[ 2 ] = fabs( self->movedir[ 2 ] );
	VectorSubtract( self->r.maxs, self->r.mins, size );
	distance = abs_movedir[ 0 ] * size[ 0 ] + abs_movedir[ 1 ] * size[ 1 ] + abs_movedir[ 2 ] * size[ 2 ] - lip;
	VectorMA( self->restingPosition, distance, self->movedir, self->activatedPosition );

	if ( !self->config.health ) //FIXME wont work yet with class fallbacks
	{
		// touchable button
		self->touch = Touch_Button;
	}

	self->use = func_button_use;

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

void Stop_Train( gentity_t *self );

/*
===============
Think_BeginMoving

The wait time at a corner has completed, so start moving again
===============
*/
void Think_BeginMoving( gentity_t *self )
{
	self->s.pos.trTime = level.time;
	self->s.pos.trType = trType_t::TR_LINEAR_STOP;
}

/*
===============
Reached_Train
===============
*/

void func_train_reached( gentity_t *self )
{
	gentity_t *next;
	vec3_t    move;
	float     length;

	// copy the appropriate values
	next = self->nextPathSegment;

	if ( !next || !next->nextPathSegment )
	{
		return; // just stop
	}

	// fire all other targets
	G_FireEntity( next, nullptr );

	// set the new trajectory
	self->nextPathSegment = next->nextPathSegment;
	VectorCopy( next->s.origin, self->restingPosition );
	VectorCopy( next->nextPathSegment->s.origin, self->activatedPosition );

	// if the path_corner has a speed, use that otherwise use the train's speed
	G_ResetFloatField( &self->speed, true, next->config.speed, next->eclass->config.speed, 0);
	if(!self->speed) {
		G_ResetFloatField(&self->speed, true, self->config.speed, self->eclass->config.speed, DEFAULT_FUNC_TRAIN_SPEED);
	}

	// calculate duration
	VectorSubtract( self->activatedPosition, self->restingPosition, move );
	length = VectorLength( move );

	self->s.pos.trDuration = length * 1000 / self->speed;

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
	self->s.loopSound = next->soundIndex;

	// start it going
	SetMoverState( self, MOVER_1TO2, level.time );

	if ( self->spawnflags & TRAIN_START_OFF )
	{
		self->s.pos.trType = trType_t::TR_STATIONARY;
		return;
	}

	if ( next->spawnflags & CORNER_PAUSE )
	{
		Stop_Train( self );
	}
	// if there is a "wait" value on the target, don't start moving yet
	else if ( next->config.wait.time )
	{
		self->nextthink = level.time + next->config.wait.time * 1000;
		self->think = Think_BeginMoving;
		self->s.pos.trType = trType_t::TR_STATIONARY;
	}
}

/*
================
Start_Train
================
*/
void Start_Train( gentity_t *self )
{
	vec3_t move;

	//recalculate duration as the mover is highly
	//unlikely to be right on a path_corner
	VectorSubtract( self->activatedPosition, self->restingPosition, move );
	self->s.pos.trDuration = VectorLength( move ) * 1000 / self->speed;
	SetMoverState( self, MOVER_1TO2, level.time );

	self->spawnflags &= ~TRAIN_START_OFF;
}

/*
================
Stop_Train
================
*/
void Stop_Train( gentity_t *self )
{
	vec3_t origin;

	//get current origin
	BG_EvaluateTrajectory( &self->s.pos, level.time, origin );
	VectorCopy( origin, self->restingPosition );
	SetMoverState( self, MOVER_POS1, level.time );

	self->spawnflags |= TRAIN_START_OFF;
}

/*
================
Use_Train
================
*/
void func_train_act( gentity_t *ent, gentity_t*, gentity_t* )
{
	if ( ent->spawnflags & TRAIN_START_OFF )
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
void Think_SetupTrainTargets( gentity_t *self )
{
	gentity_t *path, *next, *start;
	int targetIndex;

	self->nextPathSegment = G_IterateTargets( nullptr, &targetIndex, self );

	if ( !self->nextPathSegment )
	{
		Log::Warn( "func_train at %s with an unfound target",
		          vtos( self->r.absmin ) );
		return;
	}

	start = nullptr;

	for ( path = self->nextPathSegment; path != start; path = next )
	{
		if ( !start )
		{
			start = path;
		}

		if ( !path->targetCount )
		{
			Log::Warn( "Train corner at %s without a target",
			          vtos( path->s.origin ) );
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
				          vtos( path->s.origin ) );
				return;
			}
		}
		while ( strcmp( next->classname, S_PATH_CORNER ) );

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
void func_train_blocked( gentity_t *self, gentity_t *other )
{
	if ( self->spawnflags & TRAIN_BLOCK_STOPS )
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
				vec3_t    dir;
				gentity_t *tent;

				if ( other->buildableTeam == TEAM_ALIENS )
				{
					VectorCopy( other->s.origin2, dir );
					tent = G_NewTempEntity( other->s.origin, EV_ALIEN_BUILDABLE_EXPLOSION );
					tent->s.eventParm = DirToByte( dir );
				}
				else if ( other->buildableTeam == TEAM_HUMANS )
				{
					VectorSet( dir, 0.0f, 0.0f, 1.0f );
					tent = G_NewTempEntity( other->s.origin, EV_HUMAN_BUILDABLE_EXPLOSION );
					tent->s.eventParm = DirToByte( dir );
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

	if ( self->spawnflags & TRAIN_BLOCK_STOPS )
	{
		self->damage = 0;
	}
	else
	{
		G_ResetIntField(&self->damage, true, self->config.damage, self->eclass->config.damage, 2);
	}

	trap_SetBrushModel( self, self->model );
	InitMover( self );
	reset_moverspeed( self, DEFAULT_FUNC_TRAIN_SPEED );

	self->reached = func_train_reached;
	self->act = func_train_act;
	self->blocked = func_train_blocked;

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
	char *gradingTexture;
	float gradingDistance;
	char *reverbEffect;
	float reverbDistance;
	float reverbIntensity;

	trap_SetBrushModel( self, self->model );
	InitMover( self );
	reset_moverspeed( self, 100 ); //TODO do we need this at all?
	VectorCopy( self->s.origin, self->s.pos.trBase );
	VectorCopy( self->s.origin, self->r.currentOrigin );

	// HACK: Not really a mover, so unset the content flag
	self->r.contents &= ~CONTENTS_MOVER;

	// check if this func_static has a colorgrading texture
	if( self->model[0] == '*' &&
	    G_SpawnString( "gradingTexture", "", &gradingTexture ) ) {
		G_SpawnFloat( "gradingDistance", "250", &gradingDistance );

		G_GradingTextureIndex( va( "%s %f %s", self->model + 1,
					   gradingDistance, gradingTexture ) );
	}

	// check if this func_static has a colorgrading texture
	if( self->model[0] == '*' &&
	    G_SpawnString( "reverbEffect", "", &reverbEffect ) ) {
		G_SpawnFloat( "reverbDistance", "250", &reverbDistance );
		G_SpawnFloat( "reverbIntensity", "1", &reverbIntensity );

		reverbIntensity = Com_Clamp( 0.0f, 2.0f, reverbIntensity );
		G_ReverbEffectIndex( va( "%s %f %s %f", self->model + 1,
					   reverbDistance, reverbEffect, reverbIntensity ) );
	}
}

void SP_func_dynamic( gentity_t *self )
{
	trap_SetBrushModel( self, self->model );

	InitMover( self );
	reset_moverspeed( self, 100 ); //TODO do we need this at all?

	self->flags |= FL_GROUPSLAVE;

	trap_UnlinkEntity( self );  // was linked in InitMover
	trap_LinkEntity( self );
}

/*
===============================================================================

ROTATING

===============================================================================
*/

void SP_func_rotating( gentity_t *self )
{
	G_ResetFloatField(&self->speed, false, self->config.speed, self->eclass->config.speed, 400);

	// set the axis of rotation
	self->s.apos.trType = trType_t::TR_LINEAR;

	if ( self->spawnflags & 4 )
	{
		self->s.apos.trDelta[ 2 ] = self->config.speed;
	}
	else if ( self->spawnflags & 8 )
	{
		self->s.apos.trDelta[ 0 ] = self->config.speed;
	}
	else
	{
		self->s.apos.trDelta[ 1 ] = self->config.speed;
	}

	G_ResetIntField(&self->damage, true, self->config.damage, self->eclass->config.damage, 2);

	trap_SetBrushModel( self, self->model );
	InitMover( self );
	reset_moverspeed( self, 400 );

	VectorCopy( self->s.origin, self->s.pos.trBase );
	VectorCopy( self->s.pos.trBase, self->r.currentOrigin );
	VectorCopy( self->s.apos.trBase, self->r.currentAngles );
}

/*
===============================================================================

BOBBING

===============================================================================
*/

void func_bobbing_reset( gentity_t *self )
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

	G_ResetIntField(&self->damage, true, self->config.damage, self->eclass->config.damage, 2);

	trap_SetBrushModel( self, self->model );
	InitMover( self );

	VectorCopy( self->s.origin, self->s.pos.trBase );
	VectorCopy( self->s.origin, self->r.currentOrigin );

	self->s.pos.trDuration = self->config.speed * 1000;
	self->s.pos.trTime = self->s.pos.trDuration * phase;
	self->s.pos.trType = trType_t::TR_SINE;

	// set the axis of bobbing
	if ( self->spawnflags & 1 )
	{
		self->s.pos.trDelta[ 0 ] = height;
	}
	else if ( self->spawnflags & 2 )
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
	float frequency;
	float length;
	float phase;

	G_SpawnFloat( "phase", "0", &phase );

	G_ResetIntField(&self->damage, true, self->config.damage, self->eclass->config.damage, 2);

	trap_SetBrushModel( self, self->model );

	// find pendulum length
	length = fabs( self->r.mins[ 2 ] );

	if ( length < 8 )
	{
		length = 8;
	}

	InitMover( self );
	reset_moverspeed( self, 30 );

	VectorCopy( self->s.origin, self->s.pos.trBase );
	VectorCopy( self->s.origin, self->r.currentOrigin );

	VectorCopy( self->s.angles, self->s.apos.trBase );

	frequency = 1 / ( M_PI * 2 ) * sqrt( g_gravity.value / ( 3 * length ) );
	self->s.apos.trDuration = 1000 / frequency;
	self->s.apos.trTime = self->s.apos.trDuration * phase;
	self->s.apos.trType = trType_t::TR_SINE;
	self->s.apos.trDelta[ 2 ] = self->speed;
}

/*
====================
Use_func_spawn
====================
*/
void func_spawn_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	vec3_t    mins, maxs;

	if( self->r.linked )
	{
		if ( self->obstacleHandle )
		{
			trap_BotRemoveObstacle( self->obstacleHandle );
			self->obstacleHandle = 0;
		}
		trap_UnlinkEntity( self );
	}
	else
	{
		VectorAdd( self->restingPosition, self->r.mins, mins );
		VectorAdd( self->restingPosition, self->r.maxs, maxs );
		trap_BotAddObstacle( mins, maxs, &self->obstacleHandle );
		trap_LinkEntity( self );
		if( !( self->spawnflags & 2 ) )
			G_KillBrushModel( self, activator );
	}
}

void func_spawn_reset( gentity_t *self )
{
	vec3_t    mins, maxs;

	if( self->spawnflags & 1 )
	{
		VectorAdd( self->restingPosition, self->r.mins, mins );
		VectorAdd( self->restingPosition, self->r.maxs, maxs );
		trap_BotAddObstacle( mins, maxs, &self->obstacleHandle );
		trap_LinkEntity( self );
	}
	else
	{
		if ( self->obstacleHandle )
		{
			trap_BotRemoveObstacle( self->obstacleHandle );
			self->obstacleHandle = 0;
		}
		trap_UnlinkEntity( self );
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
  self->moverState = MOVER_POS1;
  VectorCopy( self->s.origin, self->restingPosition );

  if( self->model[ 0 ] == '*' )
    trap_SetBrushModel( self, self->model );
  else
  {
    self->s.modelindex = G_ModelIndex( self->model );
    VectorCopy( self->s.angles, self->s.apos.trBase );
  }

  self->act = func_spawn_act;
  self->reset = func_spawn_reset;
}

void func_destructable_die( gentity_t *self, gentity_t*, gentity_t *attacker, int )
{
	self->takedamage = false;
	trap_UnlinkEntity( self );

	G_RadiusDamage( self->restingPosition, attacker, self->splashDamage, self->splashRadius, self,
	                DAMAGE_KNOCKBACK, MOD_TRIGGER_HURT );
}


void func_destructable_reset( gentity_t *self )
{
	G_ResetIntField(&self->health, true, self->config.health, self->eclass->config.health, 100);
	self->takedamage = true;
}

/*
====================
Use_func_destructable
====================
*/
void func_destructable_act( gentity_t *self, gentity_t *caller, gentity_t *activator )
{
  if( self->r.linked )
  {
	self->takedamage = false;
    trap_UnlinkEntity( self );
    if( self->health <= 0 )
    {
    	func_destructable_die( self, caller, activator, MOD_UNKNOWN );
    }
  }
  else
  {
    trap_LinkEntity( self );
    G_KillBrushModel( self, activator );
		func_destructable_reset ( self );
    self->takedamage = true;
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

  //ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  self->s.eType = entityType_t::ET_MOVER;
  self->r.contents |= CONTENTS_MOVER;
  self->moverState = MOVER_POS1;
  VectorCopy( self->s.origin, self->restingPosition );

  if( self->model[ 0 ] == '*' )
    trap_SetBrushModel( self, self->model );
  else
  {
    self->s.modelindex = G_ModelIndex( self->model );
    VectorCopy( self->s.angles, self->s.apos.trBase );
  }

  self->reset = func_destructable_reset;
  self->die = func_destructable_die;
  self->act = func_destructable_act;

  if( !( self->spawnflags & 1 ) )
  {
    trap_LinkEntity( self );
    self->takedamage = true;
  }
}
