/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

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

// cg_snapshot.c -- things that happen on snapshot transition,
// not necessarily every single rendered frame

#include "cg_local.h"

/*
==================
CG_ResetEntity
==================
*/
static void CG_ResetEntity( centity_t *cent )
{
	// if the previous snapshot this entity was updated in is at least
	// an event window back in time then we can reset the previous event
	if ( cent->snapShotTime < cg.time - EVENT_VALID_MSEC )
	{
		cent->previousEvent = 0;
	}

	cent->trailTime = cg.snap->serverTime;

	VectorCopy( cent->currentState.origin, cent->lerpOrigin );
	VectorCopy( cent->currentState.angles, cent->lerpAngles );

	if ( cent->currentState.eType == ET_PLAYER )
	{
		CG_ResetPlayerEntity( cent );
	}
}

/*
===============
CG_TransitionEntity

cent->nextState is moved to cent->currentState and events are fired
===============
*/
static void CG_TransitionEntity( centity_t *cent )
{
	cent->currentState = cent->nextState;
	cent->currentValid = true;

	// reset if the entity wasn't in the last frame or was teleported
	if ( !cent->interpolate )
	{
		CG_ResetEntity( cent );
	}

	// clear the next state.  if will be set by the next CG_SetNextSnap
	cent->interpolate = false;

	// check for events
	CG_CheckEvents( cent );
}

/*
==================
CG_SetInitialSnapshot

This will only happen on the very first snapshot.
At all other times, CG_TransitionSnapshot is used instead.

FIXME: Also called by map_restart?
==================
*/
void CG_SetInitialSnapshot( snapshot_t *snap )
{
	centity_t     *cent;
	entityState_t *state;

	cg.snap = snap;

	BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].currentState, false );

	// sort out solid entities
	CG_BuildSolidList();

	CG_ExecuteServerCommands( snap );

	// set our local weapon selection pointer to
	// what the server has indicated the current weapon is
	CG_Respawn();

	for ( unsigned i = 0; i < cg.snap->entities.size(); i++ )
	{
		state = &cg.snap->entities[ i ];
		cent = &cg_entities[ state->number ];

		memcpy( &cent->currentState, state, sizeof( entityState_t ) );
		//cent->currentState = *state;
		cent->interpolate = false;
		cent->currentValid = true;

		CG_ResetEntity( cent );

		// check for events
		CG_CheckEvents( cent );
	}

	// Need this check because the initial weapon for spec isn't always WP_NONE
	if ( snap->ps.persistant[ PERS_TEAM ] == TEAM_NONE )
	{
		Rocket_ShowHud( WP_NONE );
	}
	else
	{
		Rocket_ShowHud( BG_GetPlayerWeapon( &snap->ps ) );
	}
}

/*
===================
CG_TransitionSnapshot

The transition point from snap to nextSnap has passed
===================
*/
static void CG_TransitionSnapshot()
{
	centity_t  *cent;
	snapshot_t *oldFrame;
	int        oldWeapon;

	if ( !cg.snap )
	{
		CG_Error( "CG_TransitionSnapshot: NULL cg.snap" );
	}

	if ( !cg.nextSnap )
	{
		CG_Error( "CG_TransitionSnapshot: NULL cg.nextSnap" );
	}

	// execute any server string commands before transitioning entities
	CG_ExecuteServerCommands( cg.nextSnap );

	// clear the currentValid flag for all entities in the existing snapshot
	for ( unsigned i = 0; i < cg.snap->entities.size(); i++ )
	{
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		cent->currentValid = false;
	}

	// move nextSnap to snap and do the transitions
	oldFrame = cg.snap;
	cg.snap = cg.nextSnap;

	// Need to store the previous weapon because BG_PlayerStateToEntityState might change it
	// so the CG_OnPlayerWeaponChange callback is never called
	oldWeapon = oldFrame->ps.weapon;

	BG_PlayerStateToEntityState( &cg.snap->ps, &cg_entities[ cg.snap->ps.clientNum ].currentState, false );
	cg_entities[ cg.snap->ps.clientNum ].interpolate = false;

	for ( unsigned i = 0; i < cg.snap->entities.size(); i++ )
	{
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		CG_TransitionEntity( cent );

		// remember time of snapshot this entity was last updated in
		cent->snapShotTime = cg.snap->serverTime;
	}

	cg.nextSnap = nullptr;

	// check for playerstate transition events
	if ( oldFrame )
	{
		playerState_t *ops, *ps;

		ops = &oldFrame->ps;
		ps = &cg.snap->ps;

		// teleporting checks are irrespective of prediction
		if ( ( ps->eFlags ^ ops->eFlags ) & EF_TELEPORT_BIT )
		{
			cg.thisFrameTeleport = true; // will be cleared by prediction code
		}

		// if we are not doing client side movement prediction for any
		// reason, then the client events and view changes will be issued now
		if ( cg.demoPlayback || ( cg.snap->ps.pm_flags & PMF_FOLLOW ) ||
		     cg_nopredict.integer || cg.pmoveParams.synchronous )
		{
			CG_TransitionPlayerState( ps, ops );
		}

		// Callbacks for changes in playerState like weapon/class/team
		if ( oldWeapon != ps->weapon )
		{
			CG_OnPlayerWeaponChange();
		}

		if ( ops->stats[ STAT_ITEMS ] != ps->stats[ STAT_ITEMS ] )
		{
			CG_OnPlayerUpgradeChange();
		}
	}
}

/*
===================
CG_SetNextSnap

A new snapshot has just been read in from the client system.
===================
*/
static void CG_SetNextSnap( snapshot_t *snap )
{
	entityState_t *es;
	centity_t     *cent;

	cg.nextSnap = snap;

	BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].nextState, false );
	cg_entities[ cg.snap->ps.clientNum ].interpolate = true;

	// check for extrapolation errors
	for ( unsigned num = 0; num < snap->entities.size(); num++ )
	{
		es = &snap->entities[ num ];
		cent = &cg_entities[ es->number ];

		memcpy( &cent->nextState, es, sizeof( entityState_t ) );
		//cent->nextState = *es;

		// if this frame is a teleport, or the entity wasn't in the
		// previous frame, don't interpolate
		if ( !cent->currentValid || ( ( cent->currentState.eFlags ^ es->eFlags ) & EF_TELEPORT_BIT ) )
		{
			cent->interpolate = false;
		}
		else
		{
			cent->interpolate = true;
		}
	}

	// if the next frame is a teleport for the playerstate, we
	// can't interpolate during demos
	if ( cg.snap && ( ( snap->ps.eFlags ^ cg.snap->ps.eFlags ) & EF_TELEPORT_BIT ) )
	{
		cg.nextFrameTeleport = true;
	}
	else
	{
		cg.nextFrameTeleport = false;
	}

	// if changing follow mode, don't interpolate
	if ( cg.nextSnap->ps.clientNum != cg.snap->ps.clientNum )
	{
		cg.nextFrameTeleport = true;
	}

	// if changing server restarts, don't interpolate
	if ( ( cg.nextSnap->snapFlags ^ cg.snap->snapFlags ) & SNAPFLAG_SERVERCOUNT )
	{
		cg.nextFrameTeleport = true;
		CG_OnMapRestart();
	}

	// sort out solid entities
	CG_BuildSolidList();
}

/*
========================
CG_ReadNextSnapshot

This is the only place new snapshots are requested
This may increment cgs.processedSnapshotNum multiple
times if the client system fails to return a
valid snapshot.
========================
*/
static snapshot_t *CG_ReadNextSnapshot()
{
	bool   r;
	snapshot_t *dest;

	if ( cg.latestSnapshotNum > cgs.processedSnapshotNum + 1000 )
	{
		CG_Printf( "WARNING: CG_ReadNextSnapshot: way out of range, %i > %i\n",
		           cg.latestSnapshotNum, cgs.processedSnapshotNum );
	}

	while ( cgs.processedSnapshotNum < cg.latestSnapshotNum )
	{
		// decide which of the two slots to load it into
		if ( cg.snap == &cg.activeSnapshots[ 0 ] )
		{
			dest = &cg.activeSnapshots[ 1 ];
		}
		else
		{
			dest = &cg.activeSnapshots[ 0 ];
		}

		// try to read the snapshot from the client system
		cgs.processedSnapshotNum++;
		r = trap_GetSnapshot( cgs.processedSnapshotNum, dest );

		// if it succeeded, return
		if ( r )
		{
			CG_AddLagometerSnapshotInfo( dest );
			return dest;
		}

		// a GetSnapshot will return failure if the snapshot
		// never arrived, or  is so old that its entities
		// have been shoved off the end of the circular
		// buffer in the client system.

		// record as a dropped packet
		CG_AddLagometerSnapshotInfo( nullptr );

		// If there are additional snapshots, continue trying to
		// read them.
	}

	// nothing left to read
	return nullptr;
}

/*
============
CG_ProcessSnapshots

We are trying to set up a renderable view, so determine
what the simulated time is, and try to get snapshots
both before and after that time if available.

If we don't have a valid cg.snap after exiting this function,
then a 3D game view cannot be rendered.  This should only happen
right after the initial connection.  After cg.snap has been valid
once, it will never turn invalid.

Even if cg.snap is valid, cg.nextSnap may not be, if the snapshot
hasn't arrived yet (it becomes an extrapolating situation instead
of an interpolating one)

============
*/
void CG_ProcessSnapshots()
{
	snapshot_t *snap;
	int        n;

	// see what the latest snapshot the client system has is
	trap_GetCurrentSnapshotNumber( &n, &cg.latestSnapshotTime );

	if ( n != cg.latestSnapshotNum )
	{
		if ( n < cg.latestSnapshotNum )
		{
			// this should never happen
			CG_Error( "CG_ProcessSnapshots: n (%i) < cg.latestSnapshotNum (%i)", n, cg.latestSnapshotNum );
		}

		cg.latestSnapshotNum = n;
	}

	// If we have yet to receive a snapshot, check for it.
	// Once we have gotten the first snapshot, cg.snap will
	// always have valid data for the rest of the game
	while ( !cg.snap )
	{
		snap = CG_ReadNextSnapshot();

		if ( !snap )
		{
			// we can't continue until we get a snapshot
			return;
		}

		// set our weapon selection to what
		// the playerstate is currently using
		if ( !( snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) )
		{
			CG_SetInitialSnapshot( snap );
		}
	}

	// loop until we either have a valid nextSnap with a serverTime
	// greater than cg.time to interpolate towards, or we run
	// out of available snapshots
	do
	{
		// if we don't have a nextframe, try and read a new one in
		if ( !cg.nextSnap )
		{
			snap = CG_ReadNextSnapshot();

			// if we still don't have a nextframe, we will just have to
			// extrapolate
			if ( !snap )
			{
				break;
			}

			CG_SetNextSnap( snap );

			// if time went backwards, we have a level restart
			if ( cg.nextSnap->serverTime < cg.snap->serverTime )
			{
				CG_Error( "CG_ProcessSnapshots: Server time went backwards" );
			}
		}

		// if our time is < nextFrame's, we have a nice interpolating state
		if ( cg.time >= cg.snap->serverTime && cg.time < cg.nextSnap->serverTime )
		{
			break;
		}

		// we have passed the transition from nextFrame to frame
		CG_TransitionSnapshot();
	}
	while ( 1 );

	// assert our valid conditions upon exiting
	if ( cg.snap == nullptr )
	{
		CG_Error( "CG_ProcessSnapshots: cg.snap == NULL" );
	}

	if ( cg.time < cg.snap->serverTime )
	{
		// this can happen right after a vid_restart
		cg.time = cg.snap->serverTime;
	}

	if ( cg.nextSnap != nullptr && cg.nextSnap->serverTime <= cg.time )
	{
		CG_Error( "CG_ProcessSnapshots: cg.nextSnap->serverTime <= cg.time" );
	}
}
