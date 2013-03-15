/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "g_local.h"
#include "g_spawn.h"

/*
===============================================================================

PUSHMOVE

===============================================================================
*/

typedef struct
{
	gentity_t *ent;
	vec3_t    origin;
	vec3_t    angles;
	float     deltayaw;
} pushed_t;

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
		trap_Trace( &tr, ent->client->ps.origin, ent->r.mins, ent->r.maxs, ent->client->ps.origin, ent->s.number, ent->clipmask );
	}
	else
	{
		trap_Trace( &tr, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, ent->s.pos.trBase, ent->s.number, ent->clipmask );
	}

	if ( tr.startsolid )
	{
		return &g_entities[ tr.entityNum ];
	}

	return NULL;
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

Returns qfalse if the move is blocked
==================
*/
qboolean G_TryPushingEntity( gentity_t *check, gentity_t *pusher, vec3_t move, vec3_t amove )
{
	vec3_t    matrix[ 3 ], transpose[ 3 ];
	vec3_t    org, org2, move2;
	gentity_t *block;

	// EF_MOVER_STOP will just stop when contacting another entity
	// instead of pushing it, but entities can still ride on top of it
	if ( ( pusher->s.eFlags & EF_MOVER_STOP ) &&
	     check->s.groundEntityNum != pusher->s.number )
	{
		return qfalse;
	}

	//don't try to move buildables unless standing on a mover
	if ( check->s.eType == ET_BUILDABLE &&
	     check->s.groundEntityNum != pusher->s.number )
	{
		return qfalse;
	}

	// save off the old position
	if ( pushed_p > &pushed[ MAX_GENTITIES ] )
	{
		G_Error( "pushed_p > &pushed[MAX_GENTITIES]" );
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
		return qtrue;
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
		return qtrue;
	}

	// blocked
	return qfalse;
}

/*
============
G_MoverPush

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
If qfalse is returned, *obstacle will be the blocking entity
============
*/
qboolean G_MoverPush( gentity_t *pusher, vec3_t move, vec3_t amove, gentity_t **obstacle )
{
	int       i, e;
	gentity_t *check;
	vec3_t    mins, maxs;
	pushed_t  *p;
	int       entityList[ MAX_GENTITIES ];
	int       listedEntities;
	vec3_t    totalMins, totalMaxs;

	*obstacle = NULL;

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
		if ( check->s.eType != ET_ITEM && check->s.eType != ET_BUILDABLE &&
		     check->s.eType != ET_CORPSE && check->s.eType != ET_PLAYER &&
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
		if ( pusher->s.pos.trType == TR_SINE || pusher->s.apos.trType == TR_SINE )
		{
			G_Damage( check, pusher, pusher, NULL, NULL, 99999, 0, MOD_CRUSH );
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

		return qfalse;
	}

	return qtrue;
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

	obstacle = NULL;

	// make sure all group slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_p = pushed;

	for ( part = ent; part; part = part->groupChain )
	{
		if ( part->s.pos.trType == TR_STATIONARY &&
		     part->s.apos.trType == TR_STATIONARY )
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
			if ( part->s.pos.trType == TR_STATIONARY &&
			     part->s.apos.trType == TR_STATIONARY )
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
		if ( part->s.pos.trType == TR_LINEAR_STOP )
		{
			if ( level.time >= part->s.pos.trTime + part->s.pos.trDuration )
			{
				if ( part->reached )
				{
					part->reached( part );
				}
			}
		}

		if ( part->s.apos.trType == TR_LINEAR_STOP )
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
			ent->s.pos.trType = TR_STATIONARY;
			break;

		case MOVER_POS2:
			VectorCopy( ent->activatedPosition, ent->s.pos.trBase );
			ent->s.pos.trType = TR_STATIONARY;
			break;

		case MOVER_1TO2:
			VectorCopy( ent->restingPosition, ent->s.pos.trBase );
			VectorSubtract( ent->activatedPosition, ent->restingPosition, delta );
			f = 1000.0 / ent->s.pos.trDuration;
			VectorScale( delta, f, ent->s.pos.trDelta );
			ent->s.pos.trType = TR_LINEAR_STOP;
			break;

		case MOVER_2TO1:
			VectorCopy( ent->activatedPosition, ent->s.pos.trBase );
			VectorSubtract( ent->restingPosition, ent->activatedPosition, delta );
			f = 1000.0 / ent->s.pos.trDuration;
			VectorScale( delta, f, ent->s.pos.trDelta );
			ent->s.pos.trType = TR_LINEAR_STOP;
			break;

		case ROTATOR_POS1:
			VectorCopy( ent->restingPosition, ent->s.apos.trBase );
			ent->s.apos.trType = TR_STATIONARY;
			break;

		case ROTATOR_POS2:
			VectorCopy( ent->activatedPosition, ent->s.apos.trBase );
			ent->s.apos.trType = TR_STATIONARY;
			break;

		case ROTATOR_1TO2:
			VectorCopy( ent->restingPosition, ent->s.apos.trBase );
			VectorSubtract( ent->activatedPosition, ent->restingPosition, delta );
			f = 1000.0 / ent->s.apos.trDuration;
			VectorScale( delta, f, ent->s.apos.trDelta );
			ent->s.apos.trType = TR_LINEAR_STOP;
			break;

		case ROTATOR_2TO1:
			VectorCopy( ent->activatedPosition, ent->s.apos.trBase );
			VectorSubtract( ent->restingPosition, ent->activatedPosition, delta );
			f = 1000.0 / ent->s.apos.trDuration;
			VectorScale( delta, f, ent->s.apos.trDelta );
			ent->s.apos.trType = TR_LINEAR_STOP;
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
		SetMoverState( slave, moverState, time );
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
	qboolean restingPosition = qfalse;

	for ( ent = MasterOf( ent ); ent; ent = ent->groupChain )
	{
		if ( ent->moverState == MOVER_POS1 || ent->moverState == ROTATOR_POS1 )
		{
			restingPosition = qtrue;
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

void Use_BinaryMover( gentity_t *ent, gentity_t *other, gentity_t *activator );

void ReturnToPos1orApos1( gentity_t *ent )
{
	if ( GetMoverGroupState( ent ) != MOVER_POS2 )
	{
		return; // not every mover in the group has reached its endpoint yet
	}

	Use_BinaryMover( ent, ent, ent->activator );
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
		trap_AdjustAreaPortalState( ent, qfalse );
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
	qboolean  canClose = qtrue;

	numEntities = trap_EntitiesInBox( clipBrush->r.absmin, clipBrush->r.absmax, entityList, MAX_GENTITIES );

	//set brush solid
	trap_LinkEntity( ent->clipBrush );

	//see if any solid entities are inside the door
	for ( i = 0; i < numEntities; i++ )
	{
		check = &g_entities[ entityList[ i ] ];

		//only test items and players
		if ( check->s.eType != ET_ITEM && check->s.eType != ET_BUILDABLE &&
		     check->s.eType != ET_CORPSE && check->s.eType != ET_PLAYER &&
		     !check->physicsObject )
		{
			continue;
		}

		//test is this entity collides with this door
		if ( G_TestEntityPosition( check ) )
		{
			canClose = qfalse;
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
	ent->s.legsAnim = qfalse;

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

	G_FireAllTargetsOf( ent, ent->activator );
}

/*
================
Reached_BinaryMover
================
*/
void Reached_BinaryMover( gentity_t *ent )
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
		master->nextthink = MAX( master->nextthink, level.time + ent->config.wait.time );

		// fire targets
		if ( !ent->activator )
		{
			ent->activator = ent;
		}

		G_FireAllTargetsOf( ent, ent->activator );
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
			trap_AdjustAreaPortalState( ent, qfalse );
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
		master->nextthink = MAX( master->nextthink, level.time + ent->config.wait.time );

		// fire targets
		if ( !ent->activator )
		{
			ent->activator = ent;
		}

		G_FireAllTargetsOf( ent, ent->activator );
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
			trap_AdjustAreaPortalState( ent, qfalse );
		}
	}
	else
	{
		G_Error( "Reached_BinaryMover: bad moverState" );
	}
}

/*
================
Use_BinaryMover
================
*/
void Use_BinaryMover( gentity_t *ent, gentity_t *other, gentity_t *activator )
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
		Use_BinaryMover( ent->groupMaster, other, activator );
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
			trap_AdjustAreaPortalState( ent, qtrue );
		}
	}
	else if ( ent->moverState == MOVER_POS2 &&
	          !( groupState == MOVER_1TO2 || other == master ) )
	{
		// if all the way up, just delay before coming down
		master->think = ReturnToPos1orApos1;
		master->nextthink = MAX( master->nextthink, level.time + ent->config.wait.time );
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
			trap_AdjustAreaPortalState( ent, qtrue );
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
			trap_AdjustAreaPortalState( ent, qtrue );
		}
	}
	else if ( ent->moverState == ROTATOR_POS2 &&
	          !( groupState == MOVER_1TO2 || other == master ) )
	{
		// if all the way up, just delay before coming down
		master->think = ReturnToPos1orApos1;
		master->nextthink = MAX( master->nextthink, level.time + ent->config.wait.time );
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
			trap_AdjustAreaPortalState( ent, qtrue );
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
		ent->s.legsAnim = qtrue;

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
			trap_AdjustAreaPortalState( ent, qtrue );
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
		G_Error("No default speed was supplied to reset_moverspeed for entity #%i of type %s.\n", self->s.number, self->classname);

	reset_floatField(&self->speed, self->config.speed, self->eclass->config.speed, fallbackSpeed);

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

/*
================
InitMover

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
void InitMover( gentity_t *ent )
{
	float    light;
	vec3_t   color;
	qboolean lightSet, colorSet;
	char     *sound;
	char     *groupName;

	// if the "model2" key is set, use a separate model
	// for drawing, but clip against the brushes
	if ( ent->model2 )
	{
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	}

	// if the "noise" key is set, use a constant looping sound when moving
	if ( G_SpawnString( "noise", "", &sound ) )
	{
		ent->soundIndex = G_SoundIndex( sound );
	}

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

		ent->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}

	ent->use = Use_BinaryMover;
	ent->reached = Reached_BinaryMover;

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
	ent->s.eType = ET_MOVER;
	VectorCopy( ent->restingPosition, ent->r.currentOrigin );
	trap_LinkEntity( ent );

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->restingPosition, ent->s.pos.trBase );
}

void reset_rotatorspeed( gentity_t *self, float fallbackSpeed )
{
	vec3_t   move;
	float    angle;

	if(!fallbackSpeed)
		G_Error("No default speed was supplied to reset_rotatorspeed for entity #%i of type %s.\n", self->s.number, self->classname);

	// calculate time to reach second position from speed
	VectorSubtract( self->activatedPosition, self->restingPosition, move );
	angle = VectorLength( move );

	reset_floatField(&self->speed, self->config.speed, self->eclass->config.speed, fallbackSpeed);

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
	float    light;
	vec3_t   color;
	qboolean lightSet, colorSet;
	char     *sound;
	char     *groupName;

	// if the "model2" key is set, use a separate model
	// for drawing, but clip against the brushes
	if ( ent->model2 )
	{
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	}

	// if the "noise" key is set, use a constant looping sound when moving
	if ( G_SpawnString( "noise", "", &sound ) )
	{
		ent->soundIndex = G_SoundIndex( sound );
	}

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

		ent->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}

	ent->use = Use_BinaryMover;
	ent->reached = Reached_BinaryMover;

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
	ent->s.eType = ET_MOVER;
	VectorCopy( ent->restingPosition, ent->r.currentAngles );
	trap_LinkEntity( ent );

	ent->s.apos.trType = TR_STATIONARY;
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
void Blocked_Door( gentity_t *ent, gentity_t *other )
{
	// remove anything other than a client or buildable
	if ( !other->client && other->s.eType != ET_BUILDABLE )
	{
		G_FreeEntity( other );
		return;
	}

	if ( ent->damage )
	{
		G_Damage( other, ent, ent, NULL, NULL, ent->damage, 0, MOD_CRUSH );
	}

	if ( ent->spawnflags & 4 )
	{
		return; // crushers don't reverse
	}

	// reverse direction
	Use_BinaryMover( ent, ent, other );
}

/*
================
Touch_DoorTriggerSpectator
================
*/
static void Touch_DoorTriggerSpectator( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	int    i, axis;
	vec3_t origin, dir, angles;

	axis = ent->customNumber;
	VectorClear( dir );

	if ( fabs( other->s.origin[ axis ] - ent->r.absmax[ axis ] ) <
	     fabs( other->s.origin[ axis ] - ent->r.absmin[ axis ] ) )
	{
		origin[ axis ] = ent->r.absmin[ axis ] - 20;
		dir[ axis ] = -1;
	}
	else
	{
		origin[ axis ] = ent->r.absmax[ axis ] + 20;
		dir[ axis ] = 1;
	}

	for ( i = 0; i < 3; i++ )
	{
		if ( i == axis )
		{
			continue;
		}

		origin[ i ] = ( ent->r.absmin[ i ] + ent->r.absmax[ i ] ) * 0.5;
	}

	vectoangles( dir, angles );
	G_TeleportPlayer( other, origin, angles, 400.0f );
}

/*
================
manualDoorTriggerSpectator

This effectively creates a temporary door auto trigger so manually
triggers doors can be skipped by spectators
================
*/
static void manualDoorTriggerSpectator( gentity_t *door, gentity_t *player )
{
	gentity_t *other;
	gentity_t triggerHull;
	int       best, i;
	vec3_t    mins, maxs;

	//don't skip a door that is already open
	if ( door->moverState == MOVER_1TO2 ||
	     door->moverState == MOVER_POS2 ||
	     door->moverState == ROTATOR_1TO2 ||
	     door->moverState == ROTATOR_POS2 ||
	     door->moverState == MODEL_1TO2 ||
	     door->moverState == MODEL_POS2 )
	{
		return;
	}

	// find the bounds of everything on the group
	VectorCopy( door->r.absmin, mins );
	VectorCopy( door->r.absmax, maxs );

	for ( other = door->groupChain; other; other = other->groupChain )
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

	maxs[ best ] += 60;
	mins[ best ] -= 60;

	VectorCopy( mins, triggerHull.r.absmin );
	VectorCopy( maxs, triggerHull.r.absmax );
	triggerHull.customNumber = best;

	Touch_DoorTriggerSpectator( &triggerHull, player, NULL );
}

/*
================
manualTriggerSpectator

Trip to skip the closest door targeted by trigger
================
*/
void manualTriggerSpectator( gentity_t *trigger, gentity_t *player )
{
	gentity_t *currentTarget;
	gentity_t *targets[ MAX_GENTITIES ];
	int       i = 0, targetIndex, nameIndex;
	float     minDistance, currentDistance;



	//restrict this hack to trigger_multiple only for now
	if ( strcmp( trigger->classname, "sensor_player" ) )
	{
		return;
	}

	currentTarget = &g_entities[MAX_CLIENTS];
	//create a list of door entities this trigger targets
	while( ( currentTarget = G_FindNextTarget( currentTarget, &targetIndex, &nameIndex, trigger ) ) != NULL )
	{
		if ( !strcmp( currentTarget->classname, "func_door" ) )
		{
			targets[ i++ ] = currentTarget;
		}
	}

	//if more than 0 targets
	if ( i > 0 )
	{
		gentity_t *closest = NULL;
		minDistance = 0; //will be set to something higher as long as closest is NULL yet

		//pick the closest door
		for ( targetIndex = 0; targetIndex < i; targetIndex++ )
		{
			currentDistance = DistanceSquared( player->r.currentOrigin, targets[ targetIndex ]->r.currentOrigin );

			if ( closest == NULL || currentDistance < minDistance )
			{
				minDistance = currentDistance;
				closest = targets[ targetIndex ];
			}
		}

		//try and skip the door
		manualDoorTriggerSpectator( closest, player );
	}
}

/*
================
Touch_DoorTrigger
================
*/
void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	moverState_t groupState;

	//buildables don't trigger movers
	if ( other->s.eType == ET_BUILDABLE )
	{
		return;
	}

	groupState = GetMoverGroupState( ent->parent );

	if ( other->client && other->client->sess.spectatorState != SPECTATOR_NOT )
	{
		// if the door is not open and not opening
		if ( groupState != MOVER_POS2 && groupState != MOVER_1TO2 )
		{
			Touch_DoorTriggerSpectator( ent, other, trace );
		}
	}
	else if ( groupState != MOVER_1TO2 )
	{
		Use_BinaryMover( ent->parent, ent, other );
	}
}

void Think_MatchGroup( gentity_t *ent )
{
	if ( ent->flags & FL_GROUPSLAVE )
	{
		return;
	}
	MatchGroup( ent, ent->moverState, level.time );
}

/*
======================
Think_SpawnNewDoorTrigger

All of the parts of a door have been spawned, so create
a trigger that encloses all of them
======================
*/
void Think_SpawnNewDoorTrigger( gentity_t *ent )
{
	gentity_t *other;
	vec3_t    mins, maxs;
	int       i, best;

	// find the bounds of everything on the group
	VectorCopy( ent->r.absmin, mins );
	VectorCopy( ent->r.absmax, maxs );

	for ( other = ent->groupChain; other; other = other->groupChain )
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

	maxs[ best ] += ent->config.triggerRange;
	mins[ best ] -= ent->config.triggerRange;

	// create a trigger with this size
	other = G_Spawn();
	other->classname = "door_trigger";
	VectorCopy( mins, other->r.mins );
	VectorCopy( maxs, other->r.maxs );
	other->parent = ent;
	other->r.contents = CONTENTS_TRIGGER;
	other->touch = Touch_DoorTrigger;
	// remember the thinnest axis
	other->customNumber = best;
	trap_LinkEntity( other );

	if ( ent->moverState < MODEL_POS1 )
	{
		Think_MatchGroup( ent );
	}
}

void func_door_reset( gentity_t *self )
{
	reset_intField(&self->health, self->config.health, self->eclass->config.health, 0);
	reset_intField(&self->damage, self->config.damage, self->eclass->config.damage, 2);

	self->takedamage = !!self->health;

	reset_moverspeed( self, 400 );
}

void SP_func_door( gentity_t *ent )
{
	vec3_t abs_movedir;
	float  distance;
	vec3_t size;
	float  lip;
	char   *s;

	G_SpawnString( "sound2to1", "sound/movers/doors/dr1_strt.wav", &s );
	ent->sound2to1 = G_SoundIndex( s );
	G_SpawnString( "sound1to2", "sound/movers/doors/dr1_strt.wav", &s );
	ent->sound1to2 = G_SoundIndex( s );

	G_SpawnString( "soundPos2", "sound/movers/doors/dr1_end.wav", &s );
	ent->soundPos2 = G_SoundIndex( s );
	G_SpawnString( "soundPos1", "sound/movers/doors/dr1_end.wav", &s );
	ent->soundPos1 = G_SoundIndex( s );

	ent->blocked = Blocked_Door;
	ent->reset = func_door_reset;

	// default wait of 2 seconds
	if ( !ent->config.wait.time )
	{
		ent->config.wait.time = 2;
	}

	ent->config.wait.time *= 1000;

	// default trigger range of 72 units (saved in noise_index)
	G_SpawnInt( "range", "72", &ent->config.triggerRange );

	if ( ent->config.triggerRange < 0 )
	{
		ent->config.triggerRange = 72;
	}

	// default lip of 8 units
	G_SpawnFloat( "lip", "8", &lip );

	// first position at start
	VectorCopy( ent->s.origin, ent->restingPosition );

	// calculate second position
	trap_SetBrushModel( ent, ent->model );
	G_SetMovedir( ent->s.angles, ent->movedir );
	abs_movedir[ 0 ] = fabs( ent->movedir[ 0 ] );
	abs_movedir[ 1 ] = fabs( ent->movedir[ 1 ] );
	abs_movedir[ 2 ] = fabs( ent->movedir[ 2 ] );
	VectorSubtract( ent->r.maxs, ent->r.mins, size );
	distance = DotProduct( abs_movedir, size ) - lip;
	VectorMA( ent->restingPosition, distance, ent->movedir, ent->activatedPosition );

	// if "start_open", reverse position 1 and 2
	if ( ent->spawnflags & 1 )
	{
		vec3_t temp;

		VectorCopy( ent->activatedPosition, temp );
		VectorCopy( ent->s.origin, ent->activatedPosition );
		VectorCopy( temp, ent->restingPosition );
	}

	InitMover( ent );

	ent->nextthink = level.time + FRAMETIME;

	if ( ent->names[ 0 ] || ent->config.health ) //FIXME wont work yet with class fallbacks
	{
		// non touch/shoot doors
		ent->think = Think_MatchGroup;
	}
	else
	{
		ent->think = Think_SpawnNewDoorTrigger;
	}
}

void func_door_rotating_reset( gentity_t *self )
{
	reset_intField(&self->health, self->config.health, self->eclass->config.health, 0);

	self->takedamage = !!self->health;

	reset_rotatorspeed( self, 120 );
}

void SP_func_door_rotating( gentity_t *ent )
{
	char *s;

	G_SpawnString( "sound2to1", "sound/movers/doors/dr1_strt.wav", &s );
	ent->sound2to1 = G_SoundIndex( s );
	G_SpawnString( "sound1to2", "sound/movers/doors/dr1_strt.wav", &s );
	ent->sound1to2 = G_SoundIndex( s );

	G_SpawnString( "soundPos2", "sound/movers/doors/dr1_end.wav", &s );
	ent->soundPos2 = G_SoundIndex( s );
	G_SpawnString( "soundPos1", "sound/movers/doors/dr1_end.wav", &s );
	ent->soundPos1 = G_SoundIndex( s );

	ent->blocked = Blocked_Door;
	ent->reset = func_door_rotating_reset;

	// if speed is negative, positize it and add reverse flag
	if ( ent->config.speed < 0 )
	{
		ent->config.speed *= -1;
		ent->spawnflags |= 8;
	}

	// default of 2 seconds
	if ( !ent->config.wait.time )
	{
		ent->config.wait.time = 2;
	}

	ent->config.wait.time *= 1000;

	// default trigger range of 72 units (saved in noise_index)
	G_SpawnInt( "range", "72", &ent->config.triggerRange );

	if ( ent->config.triggerRange < 0 )
	{
		ent->config.triggerRange = 72;
	}

	// set the axis of rotation
	VectorClear( ent->movedir );
	VectorClear( ent->s.angles );

	if ( ent->spawnflags & 32 )
	{
		ent->movedir[ 2 ] = 1.0;
	}
	else if ( ent->spawnflags & 64 )
	{
		ent->movedir[ 0 ] = 1.0;
	}
	else
	{
		ent->movedir[ 1 ] = 1.0;
	}

	// reverse direction if necessary
	if ( ent->spawnflags & 8 )
	{
		VectorNegate( ent->movedir, ent->movedir );
	}

	G_SpawnFloat( "rotatorAngle", "0", &ent->rotatorAngle );

	// default distance of 90 degrees. This is something the mapper should not
	// leave out, so we'll tell him if he does.
	if ( !ent->rotatorAngle )
	{
		G_Printf( "%s at %s with no rotatorAngle set.\n",
		          ent->classname, vtos( ent->s.origin ) );

		ent->rotatorAngle = 90.0;
	}

	VectorCopy( ent->s.angles, ent->restingPosition );
	trap_SetBrushModel( ent, ent->model );
	VectorMA( ent->restingPosition, ent->rotatorAngle, ent->movedir, ent->activatedPosition );

	// if "start_open", reverse position 1 and 2
	if ( ent->spawnflags & 1 )
	{
		vec3_t temp;

		VectorCopy( ent->activatedPosition, temp );
		VectorCopy( ent->s.angles, ent->activatedPosition );
		VectorCopy( temp, ent->restingPosition );
		VectorNegate( ent->movedir, ent->movedir );
	}

	// set origin
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );

	InitRotator( ent );

	ent->nextthink = level.time + FRAMETIME;

	if ( ent->names[ 0 ] || ent->config.health ) //FIXME wont work yet with class fallbacks
	{
		// non touch/shoot doors
		ent->think = Think_MatchGroup;
	}
	else
	{
		ent->think = Think_SpawnNewDoorTrigger;
	}
}

void func_door_model_reset( gentity_t *self )
{
	reset_floatField(&self->speed, self->config.speed, self->eclass->config.speed, 200);
	reset_intField(&self->health, self->config.health, self->eclass->config.health, 0);

	self->takedamage = !!self->health;
}

void SP_func_door_model( gentity_t *ent )
{
	char      *s;
	float     light;
	vec3_t    color;
	qboolean  lightSet, colorSet;
	char      *sound;
	gentity_t *clipBrush;

	G_SpawnString( "sound2to1", "sound/movers/doors/dr1_strt.wav", &s );
	ent->sound2to1 = G_SoundIndex( s );
	G_SpawnString( "sound1to2", "sound/movers/doors/dr1_strt.wav", &s );
	ent->sound1to2 = G_SoundIndex( s );

	G_SpawnString( "soundPos2", "sound/movers/doors/dr1_end.wav", &s );
	ent->soundPos2 = G_SoundIndex( s );
	G_SpawnString( "soundPos1", "sound/movers/doors/dr1_end.wav", &s );
	ent->soundPos1 = G_SoundIndex( s );

	ent->reset = func_door_model_reset;

	//default wait of 2 seconds
	if ( ent->config.wait.time <= 0 )
	{
		ent->config.wait.time = 2;
	}

	ent->config.wait.time *= 1000;

	// default trigger range of 72 units (saved in noise_index)
	G_SpawnInt( "range", "72", &ent->config.triggerRange );

	if ( ent->config.triggerRange < 0 )
	{
		ent->config.triggerRange = 72;
	}

	//brush model
	clipBrush = ent->clipBrush = G_Spawn();
	clipBrush->model = ent->model;
	trap_SetBrushModel( clipBrush, clipBrush->model );
	clipBrush->s.eType = ET_INVISIBLE;
	trap_LinkEntity( clipBrush );

	//copy the bounds back from the clipBrush so the
	//triggers can be made
	VectorCopy( clipBrush->r.absmin, ent->r.absmin );
	VectorCopy( clipBrush->r.absmax, ent->r.absmax );
	VectorCopy( clipBrush->r.mins, ent->r.mins );
	VectorCopy( clipBrush->r.maxs, ent->r.maxs );

	G_SpawnVector( "modelOrigin", "0 0 0", ent->s.origin );

	G_SpawnVector( "scale", "1 1 1", ent->s.origin2 );

	// if the "model2" key is set, use a separate model
	// for drawing, but clip against the brushes
	if ( !ent->model2 )
	{
		G_Printf( S_COLOR_YELLOW "WARNING: func_door_model %d spawned with no model2 key\n", ent->s.number );
	}
	else
	{
		ent->s.modelindex = G_ModelIndex( ent->model2 );
	}

	// if the "noise" key is set, use a constant looping sound when moving
	if ( G_SpawnString( "noise", "", &sound ) )
	{
		ent->soundIndex = G_SoundIndex( sound );
	}

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

		ent->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}

	ent->use = Use_BinaryMover;

	ent->moverState = MODEL_POS1;
	ent->s.eType = ET_MODELDOOR;
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	ent->s.pos.trType = TR_STATIONARY;
	ent->s.pos.trTime = 0;
	ent->s.pos.trDuration = 0;
	VectorClear( ent->s.pos.trDelta );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	ent->s.apos.trType = TR_STATIONARY;
	ent->s.apos.trTime = 0;
	ent->s.apos.trDuration = 0;
	VectorClear( ent->s.apos.trDelta );

	ent->s.misc = ( int ) ent->animation[ 0 ]; //first frame
	ent->s.weapon = abs( ( int ) ent->animation[ 1 ] );  //number of frames

	//must be at least one frame -- mapper has forgotten animation key
	if ( ent->s.weapon == 0 )
	{
		ent->s.weapon = 1;
	}

	ent->s.torsoAnim = ent->s.weapon * ( 1000.0f / ent->config.speed ); //framerate

	trap_LinkEntity( ent );

	if ( !( ent->names[ 0 ] || ent->config.health ) ) //FIXME wont work yet with class fallbacks
	{
		ent->nextthink = level.time + FRAMETIME;
		ent->think = Think_SpawnNewDoorTrigger;
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
void Touch_Plat( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	// DONT_WAIT
	if ( ent->spawnflags & 1 )
	{
		return;
	}

	if ( !other->client || other->client->ps.stats[ STAT_HEALTH ] <= 0 )
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
void Touch_PlatCenterTrigger( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	if ( !other->client )
	{
		return;
	}

	if ( ent->parent->moverState == MOVER_POS1 )
	{
		Use_BinaryMover( ent->parent, ent, other );
	}
}

/*
================
SpawnPlatTrigger

Spawn a trigger in the middle of the plat's low position
Elevator cars require that the trigger extend through the entire low position,
not just sit on top of it.
================
*/
void SpawnPlatTrigger( gentity_t *ent )
{
	gentity_t *trigger;
	vec3_t    tmin, tmax;

	// the middle trigger will be a thin trigger just
	// above the starting position
	trigger = G_Spawn();
	trigger->classname = "plat_trigger";
	trigger->touch = Touch_PlatCenterTrigger;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->parent = ent;

	tmin[ 0 ] = ent->restingPosition[ 0 ] + ent->r.mins[ 0 ] + 33;
	tmin[ 1 ] = ent->restingPosition[ 1 ] + ent->r.mins[ 1 ] + 33;
	tmin[ 2 ] = ent->restingPosition[ 2 ] + ent->r.mins[ 2 ];

	tmax[ 0 ] = ent->restingPosition[ 0 ] + ent->r.maxs[ 0 ] - 33;
	tmax[ 1 ] = ent->restingPosition[ 1 ] + ent->r.maxs[ 1 ] - 33;
	tmax[ 2 ] = ent->restingPosition[ 2 ] + ent->r.maxs[ 2 ] + 8;

	if ( tmax[ 0 ] <= tmin[ 0 ] )
	{
		tmin[ 0 ] = ent->restingPosition[ 0 ] + ( ent->r.mins[ 0 ] + ent->r.maxs[ 0 ] ) * 0.5;
		tmax[ 0 ] = tmin[ 0 ] + 1;
	}

	if ( tmax[ 1 ] <= tmin[ 1 ] )
	{
		tmin[ 1 ] = ent->restingPosition[ 1 ] + ( ent->r.mins[ 1 ] + ent->r.maxs[ 1 ] ) * 0.5;
		tmax[ 1 ] = tmin[ 1 ] + 1;
	}

	VectorCopy( tmin, trigger->r.mins );
	VectorCopy( tmax, trigger->r.maxs );

	trap_LinkEntity( trigger );
}

void SP_func_plat( gentity_t *ent )
{
	float lip, height;
	char  *s;

	G_SpawnString( "sound2to1", "sound/movers/plats/pt1_strt.wav", &s );
	ent->sound2to1 = G_SoundIndex( s );
	G_SpawnString( "sound1to2", "sound/movers/plats/pt1_strt.wav", &s );
	ent->sound1to2 = G_SoundIndex( s );

	G_SpawnString( "soundPos2", "sound/movers/plats/pt1_end.wav", &s );
	ent->soundPos2 = G_SoundIndex( s );
	G_SpawnString( "soundPos1", "sound/movers/plats/pt1_end.wav", &s );
	ent->soundPos1 = G_SoundIndex( s );

	VectorClear( ent->s.angles );

	G_SpawnFloat( "lip", "8", &lip );

	reset_intField(&ent->damage, ent->config.damage, ent->eclass->config.damage, 2);

	if(!ent->config.wait.time)
		ent->config.wait.time = 1.0f;

	ent->config.wait.time *= 1000;

	// create second position
	trap_SetBrushModel( ent, ent->model );

	if ( !G_SpawnFloat( "height", "0", &height ) )
	{
		height = ( ent->r.maxs[ 2 ] - ent->r.mins[ 2 ] ) - lip;
	}

	// pos1 is the rest (bottom) position, pos2 is the top
	VectorCopy( ent->s.origin, ent->activatedPosition );
	VectorCopy( ent->activatedPosition, ent->restingPosition );
	ent->restingPosition[ 2 ] -= height;

	InitMover( ent );
	reset_moverspeed( ent, 400 );

	// touch function keeps the plat from returning while
	// a live player is standing on it
	ent->touch = Touch_Plat;

	ent->blocked = Blocked_Door;

	ent->parent = ent; // so it can be treated as a door

	// spawn the trigger if one hasn't been custom made
	if ( !ent->names [ 0 ] )
	{
		SpawnPlatTrigger( ent );
	}
}

/*
===============================================================================

BUTTON

===============================================================================
*/

void Touch_Button( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	if ( !other->client )
	{
		return;
	}

	if ( ent->moverState == MOVER_POS1 )
	{
		Use_BinaryMover( ent, other, other );
	}
}

void func_button_reset( gentity_t *self )
{
	reset_intField(&self->health, self->config.health, self->eclass->config.health, 0);

	self->takedamage = !!self->health;

	reset_moverspeed( self, 40 );
}

void SP_func_button( gentity_t *ent )
{
	vec3_t abs_movedir;
	float  distance;
	vec3_t size;
	float  lip;
	char   *s;

	G_SpawnString( "sound1to2", "sound/movers/switches/button1.wav", &s );
	ent->sound1to2 = G_SoundIndex( s );

	ent->reset = func_button_reset;

	if ( !ent->config.wait.time )
	{
		ent->config.wait.time = 1;
	}

	ent->config.wait.time *= 1000;

	// first position
	VectorCopy( ent->s.origin, ent->restingPosition );

	// calculate second position
	trap_SetBrushModel( ent, ent->model );

	G_SpawnFloat( "lip", "4", &lip );

	G_SetMovedir( ent->s.angles, ent->movedir );
	abs_movedir[ 0 ] = fabs( ent->movedir[ 0 ] );
	abs_movedir[ 1 ] = fabs( ent->movedir[ 1 ] );
	abs_movedir[ 2 ] = fabs( ent->movedir[ 2 ] );
	VectorSubtract( ent->r.maxs, ent->r.mins, size );
	distance = abs_movedir[ 0 ] * size[ 0 ] + abs_movedir[ 1 ] * size[ 1 ] + abs_movedir[ 2 ] * size[ 2 ] - lip;
	VectorMA( ent->restingPosition, distance, ent->movedir, ent->activatedPosition );

	if ( !ent->config.health ) //FIXME wont work yet with class fallbacks
	{
		// touchable button
		ent->touch = Touch_Button;
	}

	InitMover( ent );
}

/*
===============================================================================

TRAIN

===============================================================================
*/

#define TRAIN_START_OFF   1
#define TRAIN_BLOCK_STOPS 2

/*
===============
Think_BeginMoving

The wait time at a corner has completed, so start moving again
===============
*/
void Think_BeginMoving( gentity_t *ent )
{
	ent->s.pos.trTime = level.time;
	ent->s.pos.trType = TR_LINEAR_STOP;
}

/*
===============
Reached_Train
===============
*/
void Reached_Train( gentity_t *ent )
{
	gentity_t *next;
	vec3_t    move;
	float     length;

	// copy the appropriate values
	next = ent->nextPathSegment;

	if ( !next || !next->nextPathSegment )
	{
		return; // just stop
	}

	// fire all other targets
	G_FireAllTargetsOf( next, NULL );

	// set the new trajectory
	ent->nextPathSegment = next->nextPathSegment;
	VectorCopy( next->s.origin, ent->restingPosition );
	VectorCopy( next->nextPathSegment->s.origin, ent->activatedPosition );

	// if the path_corner has a speed, use that
	if ( next->config.speed )
	{
		ent->speed = next->config.speed;
	}
	else
	{
		// otherwise use the train's speed
		ent->speed = ent->config.speed;
	}

	if ( ent->speed < 1 )
	{
		ent->speed = 1;
	}

	// calculate duration
	VectorSubtract( ent->activatedPosition, ent->restingPosition, move );
	length = VectorLength( move );

	ent->s.pos.trDuration = length * 1000 / ent->speed;

	// Be sure to send to clients after any fast move case
	ent->r.svFlags &= ~SVF_NOCLIENT;

	// Fast move case
	if ( ent->s.pos.trDuration < 1 )
	{
		// As trDuration is used later in a division, we need to avoid that case now
		ent->s.pos.trDuration = 1;

		// Don't send entity to clients so it becomes really invisible
		ent->r.svFlags |= SVF_NOCLIENT;
	}

	// looping sound
	ent->s.loopSound = next->soundIndex;

	// start it going
	SetMoverState( ent, MOVER_1TO2, level.time );

	if ( ent->spawnflags & TRAIN_START_OFF )
	{
		ent->s.pos.trType = TR_STATIONARY;
		return;
	}

	// if there is a "wait" value on the target, don't start moving yet
	if ( next->config.wait.time )
	{
		ent->nextthink = level.time + next->config.wait.time * 1000;
		ent->think = Think_BeginMoving;
		ent->s.pos.trType = TR_STATIONARY;
	}
}

/*
================
Start_Train
================
*/
void Start_Train( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	vec3_t move;

	//recalculate duration as the mover is highly
	//unlikely to be right on a path_corner
	VectorSubtract( ent->activatedPosition, ent->restingPosition, move );
	ent->s.pos.trDuration = VectorLength( move ) * 1000 / ent->speed;
	SetMoverState( ent, MOVER_1TO2, level.time );

	ent->spawnflags &= ~TRAIN_START_OFF;
}

/*
================
Stop_Train
================
*/
void Stop_Train( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	vec3_t origin;

	//get current origin
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	VectorCopy( origin, ent->restingPosition );
	SetMoverState( ent, MOVER_POS1, level.time );

	ent->spawnflags |= TRAIN_START_OFF;
}

/*
================
Use_Train
================
*/
void Use_Train( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	if ( ent->spawnflags & TRAIN_START_OFF )
	{
		//train is currently not moving so start it
		Start_Train( ent, other, activator );
	}
	else
	{
		//train is moving so stop it
		Stop_Train( ent, other, activator );
	}
}

/*
===============
Think_SetupTrainTargets

Link all the corners together
===============
*/
void Think_SetupTrainTargets( gentity_t *ent )
{
	gentity_t *path, *next, *start;
	int targetIndex, nameIndex;

	ent->nextPathSegment = G_FindNextTarget( NULL, &targetIndex, &nameIndex, ent );

	if ( !ent->nextPathSegment )
	{
		G_Printf( "func_train at %s with an unfound target\n",
		          vtos( ent->r.absmin ) );
		return;
	}

	start = NULL;

	for ( path = ent->nextPathSegment; path != start; path = next )
	{
		if ( !start )
		{
			start = path;
		}

		if ( !path->targets[ 0 ].name )
		{
			G_Printf( "Train corner at %s without a target\n",
			          vtos( path->s.origin ) );
			return;
		}

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = NULL;

		do
		{
			next = G_FindNextTarget( next, &targetIndex, &nameIndex, path );

			if ( !next )
			{
				G_Printf( "Train corner at %s without a target path_corner\n",
				          vtos( path->s.origin ) );
				return;
			}
		}
		while ( strcmp( next->classname, "path_corner" ) );

		path->nextPathSegment = next;
	}

	// start the train moving from the first corner
	Reached_Train( ent );
}

/*
================
Blocked_Train
================
*/
void Blocked_Train( gentity_t *self, gentity_t *other )
{
	if ( self->spawnflags & TRAIN_BLOCK_STOPS )
	{
		Stop_Train( self, other, other );
	}
	else
	{
		if ( !other->client )
		{
			//whatever is blocking the train isn't a client

			//KILL!!1!!!
			G_Damage( other, self, self, NULL, NULL, 10000, 0, MOD_CRUSH );

			//buildables need to be handled differently since even when
			//dealth fatal amounts of damage they won't instantly become non-solid
			if ( other->s.eType == ET_BUILDABLE && other->spawned )
			{
				vec3_t    dir;
				gentity_t *tent;

				if ( other->buildableTeam == TEAM_ALIENS )
				{
					VectorCopy( other->s.origin2, dir );
					tent = G_TempEntity( other->s.origin, EV_ALIEN_BUILDABLE_EXPLOSION );
					tent->s.eventParm = DirToByte( dir );
				}
				else if ( other->buildableTeam == TEAM_HUMANS )
				{
					VectorSet( dir, 0.0f, 0.0f, 1.0f );
					tent = G_TempEntity( other->s.origin, EV_HUMAN_BUILDABLE_EXPLOSION );
					tent->s.eventParm = DirToByte( dir );
				}
			}

			//if it's still around free it
			if ( other )
			{
				G_FreeEntity( other );
			}

			return;
		}

		G_Damage( other, self, self, NULL, NULL, 10000, 0, MOD_CRUSH );
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
		reset_intField(&self->damage, self->config.damage, self->eclass->config.damage, 2);
	}

	trap_SetBrushModel( self, self->model );
	InitMover( self );
	reset_moverspeed( self, 100 );

	self->reached = Reached_Train;
	self->use = Use_Train;
	self->blocked = Blocked_Train;

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

void SP_func_static( gentity_t *ent )
{
	trap_SetBrushModel( ent, ent->model );
	InitMover( ent );
	reset_moverspeed( ent, 100 ); //TODO do we need this at all?
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
}

void SP_func_dynamic( gentity_t *ent )
{
	trap_SetBrushModel( ent, ent->model );

	InitMover( ent );
	reset_moverspeed( ent, 100 ); //TODO do we need this at all?

	ent->flags |= FL_GROUPSLAVE;

	trap_UnlinkEntity( ent );  // was linked in InitMover
	trap_AddPhysicsEntity( ent );
	trap_LinkEntity( ent );
}

/*
===============================================================================

ROTATING

===============================================================================
*/

void SP_func_rotating( gentity_t *ent )
{
	reset_floatField(&ent->speed, ent->config.speed, ent->eclass->config.speed, 400);

	// set the axis of rotation
	ent->s.apos.trType = TR_LINEAR;

	if ( ent->spawnflags & 4 )
	{
		ent->s.apos.trDelta[ 2 ] = ent->config.speed;
	}
	else if ( ent->spawnflags & 8 )
	{
		ent->s.apos.trDelta[ 0 ] = ent->config.speed;
	}
	else
	{
		ent->s.apos.trDelta[ 1 ] = ent->config.speed;
	}

	reset_intField(&ent->damage, ent->config.damage, ent->eclass->config.damage, 2);

	trap_SetBrushModel( ent, ent->model );
	InitMover( ent );
	reset_moverspeed( ent, 400 );

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );
	VectorCopy( ent->s.apos.trBase, ent->r.currentAngles );

	trap_LinkEntity( ent );
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

void SP_func_bobbing( gentity_t *ent )
{
	float height;
	float phase;

	ent->reset = func_bobbing_reset;

	G_SpawnFloat( "height", "32", &height );
	G_SpawnFloat( "phase", "0", &phase );

	reset_intField(&ent->damage, ent->config.damage, ent->eclass->config.damage, 2);

	trap_SetBrushModel( ent, ent->model );
	InitMover( ent );

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	ent->s.pos.trDuration = ent->config.speed * 1000;
	ent->s.pos.trTime = ent->s.pos.trDuration * phase;
	ent->s.pos.trType = TR_SINE;

	// set the axis of bobbing
	if ( ent->spawnflags & 1 )
	{
		ent->s.pos.trDelta[ 0 ] = height;
	}
	else if ( ent->spawnflags & 2 )
	{
		ent->s.pos.trDelta[ 1 ] = height;
	}
	else
	{
		ent->s.pos.trDelta[ 2 ] = height;
	}
}

/*
===============================================================================

PENDULUM

===============================================================================
*/

void SP_func_pendulum( gentity_t *ent )
{
	float frequency;
	float length;
	float phase;

	G_SpawnFloat( "phase", "0", &phase );

	reset_intField(&ent->damage, ent->config.damage, ent->eclass->config.damage, 2);

	trap_SetBrushModel( ent, ent->model );

	// find pendulum length
	length = fabs( ent->r.mins[ 2 ] );

	if ( length < 8 )
	{
		length = 8;
	}

	InitMover( ent );
	reset_moverspeed( ent, 30 );

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	VectorCopy( ent->s.angles, ent->s.apos.trBase );

	frequency = 1 / ( M_PI * 2 ) * sqrt( g_gravity.value / ( 3 * length ) );
	ent->s.apos.trDuration = 1000 / frequency;
	ent->s.apos.trTime = ent->s.apos.trDuration * phase;
	ent->s.apos.trType = TR_SINE;
	ent->s.apos.trDelta[ 2 ] = ent->config.speed;
}

/*
====================
G_KillBrushModel
====================
*/
void G_KillBrushModel( gentity_t *ent, gentity_t *activator )
{
  gentity_t *e;
  vec3_t mins, maxs;
  trace_t tr;

  for( e = &g_entities[ 0 ]; e < &g_entities[ level.num_entities ]; ++e )
  {
    if( !e->takedamage || !e->r.linked || !e->clipmask || ( e->client && e->client->noclip ) )
      continue;

    VectorAdd( e->r.currentOrigin, e->r.mins, mins );
    VectorAdd( e->r.currentOrigin, e->r.maxs, maxs );

    if( !trap_EntityContact( mins, maxs, ent ) )
      continue;

    trap_Trace( &tr, e->r.currentOrigin, e->r.mins, e->r.maxs,
                e->r.currentOrigin, e->s.number, e->clipmask );

    if( tr.entityNum != ENTITYNUM_NONE )
      G_Damage( e, ent, activator, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_CRUSH );
  }
}

/*
====================
Use_func_spawn
====================
*/
void Use_func_spawn( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
  if( ent->r.linked )
    trap_UnlinkEntity( ent );
  else
  {
    trap_LinkEntity( ent );
    if( !( ent->spawnflags & 2 ) )
      G_KillBrushModel( ent, activator );
  }
}

void func_spawn_reset( gentity_t *self )
{
	if( self->spawnflags & 1 )
		trap_LinkEntity( self );
	else
		trap_UnlinkEntity( self );
}
/*
====================
SP_func_spawn
====================
*/
void SP_func_spawn( gentity_t *ent )
{
  //ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  ent->s.eType = ET_MOVER;
  ent->moverState = MOVER_POS1;
  VectorCopy( ent->s.origin, ent->restingPosition );

  if( ent->model[ 0 ] == '*' )
    trap_SetBrushModel( ent, ent->model );
  else
  {
    ent->s.modelindex = G_ModelIndex( ent->model );
    VectorCopy( ent->s.angles, ent->s.apos.trBase );
  }

  ent->use = Use_func_spawn;
  ent->reset = func_spawn_reset;
}

void func_destructable_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod )
{
	self->takedamage = qfalse;
	trap_UnlinkEntity( self );

	G_RadiusDamage( self->restingPosition, attacker, self->splashDamage, self->splashRadius, self, MOD_TRIGGER_HURT );
	G_FireAllTargetsOf( self, attacker );
}


void func_destructable_reset( gentity_t *self )
{
	reset_intField(&self->health, self->config.health, self->eclass->config.health, 100);
	if(self->health < 0)
		self->health = 1;

	self->takedamage = qtrue;
}

/*
====================
Use_func_destructable
====================
*/
void func_destructable_act( gentityCallDefinition_t* target, gentity_t *self, gentity_t *caller, gentity_t *activator )
{
  if( self->r.linked )
  {
	self->takedamage = qfalse;
    trap_UnlinkEntity( self );
    if( self->health <= 0 )
    {
    	func_destructable_die( self, caller, activator, 0, MOD_UNKNOWN );
    }
  }
  else
  {
    trap_LinkEntity( self );
    G_KillBrushModel( self, activator );
		func_destructable_reset ( self );
    self->takedamage = qtrue;
  }
}

/*
====================
SP_func_destructable
====================
*/
void SP_func_destructable( gentity_t *ent )
{
  SP_ConditionFields( ent );

  G_SpawnInt( "damage", "0", &ent->splashDamage );
  G_SpawnInt( "radius", "0", &ent->splashRadius );

  //ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  ent->s.eType = ET_MOVER;
  ent->moverState = MOVER_POS1;
  VectorCopy( ent->s.origin, ent->restingPosition );

  if( ent->model[ 0 ] == '*' )
    trap_SetBrushModel( ent, ent->model );
  else
  {
    ent->s.modelindex = G_ModelIndex( ent->model );
    VectorCopy( ent->s.angles, ent->s.apos.trBase );
  }

  ent->reset = func_destructable_reset;
  ent->die = func_destructable_die;
  ent->act = func_destructable_act;

  if( ent->spawnflags & 1 )
    trap_UnlinkEntity( ent );
  else
  {
    trap_LinkEntity( ent );
    ent->takedamage = qtrue;
  }
}
