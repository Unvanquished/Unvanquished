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

// cg_predict.c -- this file generates cg.predictedPlayerState by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement.
// It also handles local physics interaction, like fragments bouncing off walls

#include "cg_local.h"

static  pmove_t   cg_pmove;

static  int       cg_numSolidEntities;
static  centity_t *cg_solidEntities[ MAX_ENTITIES_IN_SNAPSHOT ];
static  int       cg_numTriggerEntities;
static  centity_t *cg_triggerEntities[ MAX_ENTITIES_IN_SNAPSHOT ];

/*
====================
CG_BuildSolidList

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection

It also adds content flags to allow for more specific traces in synchronized code.
====================
*/
void CG_BuildSolidList()
{
	centity_t     *cent;
	snapshot_t    *snap;
	entityState_t *ent;

	cg_numSolidEntities = 0;
	cg_numTriggerEntities = 0;

	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport )
	{
		snap = cg.nextSnap;
	}
	else
	{
		snap = cg.snap;
	}

	for ( unsigned i = 0; i < snap->entities.size(); i++ )
	{
		cent = &cg_entities[ snap->entities[ i ].number ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM || ent->eType == ET_PUSHER || ent->eType == ET_TELEPORTER )
		{
			cg_triggerEntities[ cg_numTriggerEntities ] = cent;
			cg_numTriggerEntities++;
			continue;
		}

		if ( cent->nextState.solid && ent->eType != ET_MISSILE )
		{
			cent->contents |= CONTENTS_SOLID;

			// retreive some content flags from the entity type
			switch ( ent->eType )
			{
				case ET_MOVER:
				case ET_MODELDOOR:
					cent->contents |= CONTENTS_MOVER;
					break;

				default:
					break;
			}

			cg_solidEntities[ cg_numSolidEntities ] = cent;
			cg_numSolidEntities++;
			continue;
		}
	}
}

/*
====================
CG_ClipMoveToEntities

====================
*/
static void CG_ClipMoveToEntities( const vec3_t start, const vec3_t mins,
                                   const vec3_t maxs, const vec3_t end, int skipNumber,
                                   int mask, int skipmask, trace_t *tr, traceType_t collisionType )
{
	int           i, x, zd, zu;
	trace_t       trace;
	entityState_t *ent;
	clipHandle_t  cmodel;
	vec3_t        tmins, tmaxs;
	vec3_t        bmins, bmaxs;
	vec3_t        origin, angles;
	centity_t     *cent;

	// calculate bounding box of the trace
	ClearBounds( tmins, tmaxs );
	AddPointToBounds( start, tmins, tmaxs );
	AddPointToBounds( end, tmins, tmaxs );
	if( mins )
		VectorAdd( mins, tmins, tmins );
	if( maxs )
		VectorAdd( maxs, tmaxs, tmaxs );

	for ( i = 0; i < cg_numSolidEntities; i++ )
	{
		if ( i < cg_numSolidEntities )
		{
			cent = cg_solidEntities[ i ];
		}
		else
		{
			cent = &cg.predictedPlayerEntity;
		}

		ent = &cent->currentState;

		if ( ent->number == skipNumber )
		{
			continue;
		}

		if ( !( cent->contents & mask ) )
		{
			continue;
		}

		if ( cent->contents & skipmask )
		{
			continue;
		}

		if ( ent->solid == SOLID_BMODEL )
		{
			// special value for bmodel
			cmodel = trap_CM_InlineModel( ent->modelindex );
			VectorCopy( cent->lerpAngles, angles );
			BG_EvaluateTrajectory( &cent->currentState.pos, cg.physicsTime, origin );
		}
		else
		{
			// encoded bbox
			x = ( ent->solid & 255 );
			zd = ( ( ent->solid >> 8 ) & 255 );
			zu = ( ( ent->solid >> 16 ) & 255 ) - 32;

			bmins[ 0 ] = bmins[ 1 ] = -x;
			bmaxs[ 0 ] = bmaxs[ 1 ] = x;
			bmins[ 2 ] = -zd;
			bmaxs[ 2 ] = zu;

			if ( i == cg_numSolidEntities )
			{
				BG_ClassBoundingBox( ( ent->misc >> 8 ) & 0xFF, bmins, bmaxs, nullptr, nullptr, nullptr );
			}

			VectorAdd( cent->lerpOrigin, bmins, bmins );
			VectorAdd( cent->lerpOrigin, bmaxs, bmaxs );

			if( !BoundsIntersect( bmins, bmaxs, tmins, tmaxs ) )
				continue;

			cmodel = trap_CM_TempBoxModel( bmins, bmaxs );
			VectorCopy( vec3_origin, angles );
			VectorCopy( vec3_origin, origin );
		}

		switch ( collisionType )
		{
		case TT_CAPSULE:
			trap_CM_TransformedCapsuleTrace( &trace, start, end, mins, maxs, cmodel, mask, skipmask,
			                                 origin, angles );
			break;

		case TT_AABB:
			trap_CM_TransformedBoxTrace( &trace, start, end, mins, maxs, cmodel, mask, skipmask,
			                             origin, angles );
			break;

		case TT_BISPHERE:
			assert( maxs != nullptr );
			assert( mins != nullptr );
			trap_CM_TransformedBiSphereTrace( &trace, start, end, mins[ 0 ], maxs[ 0 ], cmodel,
			                                  mask, skipmask, origin );
			break;

		default: // Shouldn't Happen
			assert( 0 );
		}

		if ( trace.allsolid || trace.fraction < tr->fraction )
		{
			trace.entityNum = ent->number;

			if ( tr->lateralFraction < trace.lateralFraction )
			{
				float oldLateralFraction = tr->lateralFraction;
				*tr = trace;
				tr->lateralFraction = oldLateralFraction;
			}
			else
			{
				*tr = trace;
			}
		}
		else if ( trace.startsolid )
		{
			tr->startsolid = true;
			tr->entityNum = ent->number;
		}

		if ( tr->allsolid )
		{
			return;
		}
	}
}

/*
================
CG_Trace
================
*/
void CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs,
               const vec3_t end, int skipNumber, int mask, int skipmask )
{
	trace_t t;

	vec3_t mymins = {0.0f, 0.0f, 0.0f};
	vec3_t mymaxs = {0.0f, 0.0f, 0.0f};

	if (mins) {
		VectorCopy(mins, mymins);
	}
	if (maxs) {
		VectorCopy(maxs, mymaxs);
	}

	trap_CM_BoxTrace( &t, start, end, mymins, mymaxs, 0, mask, skipmask );
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities( start, mins, maxs, end, skipNumber, mask, skipmask, &t, TT_AABB );

	*result = t;
}

/*
================
CG_CapTrace
================
*/
void  CG_CapTrace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs,
                   const vec3_t end, int skipNumber, int mask, int skipmask )
{
	trace_t t;

	vec3_t mymins = {0.0f, 0.0f, 0.0f};
	vec3_t mymaxs = {0.0f, 0.0f, 0.0f};

	if (mins) {
		VectorCopy(mins, mymins);
	}
	if (maxs) {
		VectorCopy(maxs, mymaxs);
	}

	trap_CM_CapsuleTrace( &t, start, end, mymins, mymaxs, 0, mask, skipmask );
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities( start, mins, maxs, end, skipNumber, mask, skipmask, &t, TT_CAPSULE );

	*result = t;
}

/*
================
CG_BiSphereTrace
================
*/
void CG_BiSphereTrace( trace_t *result, const vec3_t start, const vec3_t end,
                       const float startRadius, const float endRadius, int skipNumber, int mask,
                       int skipmask )
{
	trace_t t;
	vec3_t  mins, maxs;

	mins[ 0 ] = startRadius;
	maxs[ 0 ] = endRadius;

	trap_CM_BiSphereTrace( &t, start, end, startRadius, endRadius, 0, mask, skipmask );
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities( start, mins, maxs, end, skipNumber, mask, skipmask, &t, TT_BISPHERE );

	*result = t;
}

/*
================
CG_PointContents
================
*/
int   CG_PointContents( const vec3_t point, int passEntityNum )
{
	int           i;
	entityState_t *ent;
	centity_t     *cent;
	clipHandle_t  cmodel;
	int           contents;

	contents = trap_CM_PointContents( point, 0 );

	for ( i = 0; i < cg_numSolidEntities; i++ )
	{
		cent = cg_solidEntities[ i ];

		ent = &cent->currentState;

		if ( ent->number == passEntityNum )
		{
			continue;
		}

		if ( ent->solid != SOLID_BMODEL ) // special value for bmodel
		{
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );

		if ( !cmodel )
		{
			continue;
		}

		contents |= trap_CM_TransformedPointContents( point, cmodel, ent->origin, ent->angles );
	}

	return contents;
}

/*
========================
CG_InterpolatePlayerState

Generates cg.predictedPlayerState by interpolating between
cg.snap->player_state and cg.nextFrame->player_state
========================
*/
static void CG_InterpolatePlayerState( bool grabAngles )
{
	float         f;
	int           i;
	playerState_t *out;
	snapshot_t    *prev, *next;

	out = &cg.predictedPlayerState;
	prev = cg.snap;
	next = cg.nextSnap;

	*out = cg.snap->ps;

	// if we are still allowing local input, short circuit the view angles
	if ( grabAngles )
	{
		usercmd_t cmd;
		int       cmdNum;

		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd( cmdNum, &cmd );

		PM_UpdateViewAngles( out, &cmd );
	}

	// if the next frame is a teleport, we can't lerp to it
	if ( cg.nextFrameTeleport )
	{
		return;
	}

	if ( !next || next->serverTime <= prev->serverTime )
	{
		return;
	}

	f = ( float )( cg.time - prev->serverTime ) / ( next->serverTime - prev->serverTime );

	i = next->ps.bobCycle;

	if ( i < prev->ps.bobCycle )
	{
		i += 256; // handle wraparound
	}

	out->bobCycle = prev->ps.bobCycle + f * ( i - prev->ps.bobCycle );

	for ( i = 0; i < 3; i++ )
	{
		out->origin[ i ] = prev->ps.origin[ i ] + f * ( next->ps.origin[ i ] - prev->ps.origin[ i ] );

		if ( !grabAngles )
		{
			out->viewangles[ i ] = LerpAngle( prev->ps.viewangles[ i ], next->ps.viewangles[ i ], f );
		}

		out->velocity[ i ] = prev->ps.velocity[ i ] +
		                     f * ( next->ps.velocity[ i ] - prev->ps.velocity[ i ] );
	}
}

/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and items
=========================
*/
static void CG_TouchTriggerPrediction()
{
	int           i;
	trace_t       trace;
	entityState_t *ent;
	clipHandle_t  cmodel;
	centity_t     *cent;
	bool      spectator;

	// dead clients don't activate triggers
	if ( cg.predictedPlayerState.stats[ STAT_HEALTH ] <= 0 )
	{
		return;
	}

	spectator = ( cg.predictedPlayerState.pm_type == PM_SPECTATOR );

	if ( cg.predictedPlayerState.pm_type != PM_NORMAL && !spectator )
	{
		return;
	}

	for ( i = 0; i < cg_numTriggerEntities; i++ )
	{
		cent = cg_triggerEntities[ i ];
		ent = &cent->currentState;

		if ( ent->solid != SOLID_BMODEL )
		{
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );

		if ( !cmodel )
		{
			continue;
		}

		trap_CM_BoxTrace( &trace, cg.predictedPlayerState.origin, cg.predictedPlayerState.origin,
		                  cg_pmove.mins, cg_pmove.maxs, cmodel, MASK_ALL, 0 );

		if ( !trace.startsolid )
		{
			continue;
		}

		if ( ent->eType == ET_TELEPORTER )
		{
			cg.hyperspace = true;
		}
	}
}

static int CG_IsUnacceptableError( playerState_t *ps, playerState_t *pps )
{
	vec3_t delta;
	int    i;

	if ( pps->pm_type != ps->pm_type ||
	     pps->pm_flags != ps->pm_flags ||
	     pps->pm_time != ps->pm_time )
	{
		return 1;
	}

	VectorSubtract( pps->origin, ps->origin, delta );

	if ( VectorLengthSquared( delta ) > 0.1f * 0.1f )
	{
		if ( cg_showmiss.integer )
		{
			CG_Printf( "origin delta: %.2f  ", VectorLength( delta ) );
		}

		return 2;
	}

	VectorSubtract( pps->velocity, ps->velocity, delta );

	if ( VectorLengthSquared( delta ) > 0.1f * 0.1f )
	{
		if ( cg_showmiss.integer )
		{
			CG_Printf( "velocity delta: %.2f  ", VectorLength( delta ) );
		}

		return 3;
	}

	if ( pps->weaponTime != ps->weaponTime ||
	     pps->gravity != ps->gravity ||
	     pps->speed != ps->speed ||
	     pps->delta_angles[ 0 ] != ps->delta_angles[ 0 ] ||
	     pps->delta_angles[ 1 ] != ps->delta_angles[ 1 ] ||
	     pps->delta_angles[ 2 ] != ps->delta_angles[ 2 ] ||
	     pps->groundEntityNum != ps->groundEntityNum )
	{
		return 4;
	}

	if ( pps->legsTimer != ps->legsTimer ||
	     pps->legsAnim != ps->legsAnim ||
	     pps->torsoTimer != ps->torsoTimer ||
	     pps->torsoAnim != ps->torsoAnim ||
	     pps->movementDir != ps->movementDir )
	{
		return 5;
	}

	VectorSubtract( pps->grapplePoint, ps->grapplePoint, delta );

	if ( VectorLengthSquared( delta ) > 0.1f * 0.1f )
	{
		return 6;
	}

	if ( pps->eFlags != ps->eFlags )
	{
		return 7;
	}

	if ( pps->eventSequence != ps->eventSequence )
	{
		return 8;
	}

	for ( i = 0; i < MAX_EVENTS; i++ )
	{
		if ( pps->events[ i ] != ps->events[ i ] ||
		     pps->eventParms[ i ] != ps->eventParms[ i ] )
		{
			return 9;
		}
	}

	if ( pps->externalEvent != ps->externalEvent ||
	     pps->externalEventParm != ps->externalEventParm ||
	     pps->externalEventTime != ps->externalEventTime )
	{
		return 10;
	}

	if ( pps->clientNum != ps->clientNum ||
	     pps->weapon != ps->weapon ||
	     pps->weaponstate != ps->weaponstate )
	{
		return 11;
	}

	if ( fabs( AngleDelta( ps->viewangles[ 0 ], pps->viewangles[ 0 ] ) ) > 1.0f ||
	     fabs( AngleDelta( ps->viewangles[ 1 ], pps->viewangles[ 1 ] ) ) > 1.0f ||
	     fabs( AngleDelta( ps->viewangles[ 2 ], pps->viewangles[ 2 ] ) ) > 1.0f )
	{
		return 12;
	}

	if ( pps->viewheight != ps->viewheight )
	{
		return 13;
	}

	if ( pps->damageEvent != ps->damageEvent ||
	     pps->damageYaw != ps->damageYaw ||
	     pps->damagePitch != ps->damagePitch ||
	     pps->damageCount != ps->damageCount )
	{
		return 14;
	}

	for ( i = 0; i < MAX_STATS; i++ )
	{
		if ( pps->stats[ i ] != ps->stats[ i ] )
		{
			return 15;
		}
	}

	for ( i = 0; i < MAX_PERSISTANT; i++ )
	{
		if ( pps->persistant[ i ] != ps->persistant[ i ] )
		{
			return 16;
		}
	}

	if ( pps->generic1 != ps->generic1 ||
	     pps->loopSound != ps->loopSound )
	{
		return 19;
	}

	return 0;
}

/*
=================
CG_PredictPlayerState

Generates cg.predictedPlayerState for the current cg.time
cg.predictedPlayerState is guaranteed to be valid after exiting.

For demo playback, this will be an interpolation between two valid
playerState_t.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new snapshot will usually have one or more new usercmds over the last,
but we simulate all unacknowledged commands each time, not just the new ones.
This means that in case of an Internet connection, quite a few pmoves may be
issued each frame.

OPTIMIZE: Don't re-simulate unless the newly arrived snapshot's playerState_t
differs from the predicted one.  This would require saving all intermediate
playerState_ts during prediction.

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/
void CG_PredictPlayerState()
{
	int           cmdNum, current, i;
	playerState_t oldPlayerState;
	usercmd_t     oldestCmd;
	usercmd_t     latestCmd;
	int           stateIndex = 0, predictCmd = 0;

	cg.hyperspace = false; // will be set if touching a trigger_teleport

	// if this is the first frame we must guarantee
	// predictedPlayerState is valid even if there is some
	// other error condition
	if ( !cg.validPPS )
	{
		cg.validPPS = true;
		cg.predictedPlayerState = cg.snap->ps;
	}

	// demo playback just copies the moves
	if ( cg.demoPlayback || ( cg.snap->ps.pm_flags & PMF_FOLLOW ) )
	{
		CG_InterpolatePlayerState( false );
		return;
	}

	// non-predicting local movement will grab the latest angles
	if ( cg_nopredict.integer || cg.pmoveParams.synchronous )
	{
		CG_InterpolatePlayerState( true );
		return;
	}

	// prepare for pmove
	cg_pmove.ps = &cg.predictedPlayerState;
	cg_pmove.pmext = &cg.pmext;
	cg_pmove.trace = CG_Trace;
	cg_pmove.pointcontents = CG_PointContents;
	cg_pmove.debugLevel = cg_debugMove.integer;

	if ( cg_pmove.ps->pm_type == PM_DEAD )
	{
		cg_pmove.tracemask = MASK_DEADSOLID;
	}
	else
	{
		cg_pmove.tracemask = MASK_PLAYERSOLID;
	}

	if ( cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
	{
		cg_pmove.tracemask = MASK_DEADSOLID; // spectators can fly through bodies
	}

	cg_pmove.noFootsteps = 0;

	// save the state before the pmove so we can detect transitions
	oldPlayerState = cg.predictedPlayerState;

	current = trap_GetCurrentCmdNumber();

	// if we don't have the commands right after the snapshot, we
	// can't accurately predict a current position, so just freeze at
	// the last good position we had
	cmdNum = current - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &oldestCmd );

	if ( oldestCmd.serverTime > cg.snap->ps.commandTime &&
	     oldestCmd.serverTime < cg.time )
	{
		// special check for map_restart
		if ( cg_showmiss.integer )
		{
			CG_Printf( "exceeded PACKET_BACKUP on commands\n" );
		}

		return;
	}

	// get the latest command so we can know which commands are from previous map_restarts
	trap_GetUserCmd( current, &latestCmd );

	// get the most recent information we have, even if
	// the server time is beyond our current cg.time,
	// because predicted player positions are going to
	// be ahead of everything else anyway
	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport )
	{
		cg.predictedPlayerState = cg.nextSnap->ps;
		cg.physicsTime = cg.nextSnap->serverTime;
	}
	else
	{
		cg.predictedPlayerState = cg.snap->ps;
		cg.physicsTime = cg.snap->serverTime;
	}

	cg_pmove.pmove_fixed = cg.pmoveParams.fixed; // | cg_pmove_fixed.integer;
	cg_pmove.pmove_msec = cg.pmoveParams.msec;
	cg_pmove.pmove_accurate = cg.pmoveParams.accurate;

	// Like the comments described above, a player's state is entirely
	// re-predicted from the last valid snapshot every client frame, which
	// can be really, really, really slow.  Every old command has to be
	// run again.  For every client frame that is *not* directly after a
	// snapshot, this is unnecessary, since we have no new information.
	// For those, we'll play back the predictions from the last frame and
	// predict only the newest commands.  Essentially, we'll be doing
	// an incremental predict instead of a full predict.
	//
	// If we have a new snapshot, we can compare its player state's command
	// time to the command times in the queue to find a match.  If we find
	// a matching state, and the predicted version has not deviated, we can
	// use the predicted state as a base - and also do an incremental predict.
	//
	// With this method, we get incremental predicts on every client frame
	// except a frame following a new snapshot in which there was a prediction
	// error.  This yeilds anywhere from a 15% to 40% performance increase,
	// depending on how much of a bottleneck the CPU is.
	if ( cg_optimizePrediction.integer )
	{
		if ( cg.nextFrameTeleport || cg.thisFrameTeleport )
		{
			// do a full predict
			cg.lastPredictedCommand = 0;
			cg.stateTail = cg.stateHead;
			predictCmd = current - CMD_BACKUP + 1;
		}
		// cg.physicsTime is the current snapshot's serverTime if it's the same
		// as the last one
		else if ( cg.physicsTime == cg.lastServerTime )
		{
			// we have no new information, so do an incremental predict
			predictCmd = cg.lastPredictedCommand + 1;
		}
		else
		{
			// we have a new snapshot
			int      i;
			int      errorcode;
			bool error = true;

			// loop through the saved states queue
			for ( i = cg.stateHead; i != cg.stateTail;
			      i = ( i + 1 ) % NUM_SAVED_STATES )
			{
				// if we find a predicted state whose commandTime matches the snapshot
				// player state's commandTime
				if ( cg.savedPmoveStates[ i ].commandTime !=
				     cg.predictedPlayerState.commandTime )
				{
					continue;
				}

				// make sure the state differences are acceptable
				errorcode = CG_IsUnacceptableError( &cg.predictedPlayerState,
				                                    &cg.savedPmoveStates[ i ] );

				if ( errorcode )
				{
					if ( cg_showmiss.integer )
					{
						CG_Printf( "error code %d at %d\n", errorcode, cg.time );
					}

					break;
				}

				// this one is almost exact, so we'll copy it in as the starting point
				*cg_pmove.ps = cg.savedPmoveStates[ i ];
				// advance the head
				cg.stateHead = ( i + 1 ) % NUM_SAVED_STATES;

				// set the next command to predict
				predictCmd = cg.lastPredictedCommand + 1;

				// a saved state matched, so flag it
				error = false;
				break;
			}

			// if no saved states matched
			if ( error )
			{
				// do a full predict
				cg.lastPredictedCommand = 0;
				cg.stateTail = cg.stateHead;
				predictCmd = current - CMD_BACKUP + 1;
			}
		}

		// keep track of the server time of the last snapshot so we
		// know when we're starting from a new one in future calls
		cg.lastServerTime = cg.physicsTime;
		stateIndex = cg.stateHead;
	}

	for ( cmdNum = current - CMD_BACKUP + 1; cmdNum <= current; cmdNum++ )
	{
		// get the command
		trap_GetUserCmd( cmdNum, &cg_pmove.cmd );

		if ( cg_pmove.pmove_fixed )
		{
			PM_UpdateViewAngles( cg_pmove.ps, &cg_pmove.cmd );
		}

		// don't do anything if the time is before the snapshot player time
		if ( cg_pmove.cmd.serverTime <= cg.predictedPlayerState.commandTime )
		{
			continue;
		}

		// don't do anything if the command was from a previous map_restart
		if ( cg_pmove.cmd.serverTime > latestCmd.serverTime )
		{
			continue;
		}

		// check for a prediction error from the last frame;
		// on a LAN, this will often be the exact value
		// from the snapshot, but on a WAN we will have
		// to predict several commands to get to the point
		// we want to compare
		if ( cg.predictedPlayerState.commandTime == oldPlayerState.commandTime )
		{
			vec3_t delta;
			float  len;

			if ( cg.thisFrameTeleport )
			{
				// a teleport will not cause an error decay
				VectorClear( cg.predictedError );

				if ( cg_showmiss.integer )
				{
					CG_Printf( "PredictionTeleport\n" );
				}

				cg.thisFrameTeleport = false;
			}
			else
			{
				vec3_t adjusted, new_angles;
				CG_AdjustPositionForMover( cg.predictedPlayerState.origin,
				                           cg.predictedPlayerState.groundEntityNum, cg.physicsTime, cg.oldTime, adjusted, cg.predictedPlayerState.viewangles, new_angles );

				if ( cg_showmiss.integer )
				{
					if ( !VectorCompare( oldPlayerState.origin, adjusted ) )
					{
						CG_Printf( "prediction error\n" );
					}
				}

				VectorSubtract( oldPlayerState.origin, adjusted, delta );
				len = VectorLength( delta );

				if ( len > 0.1 )
				{
					if ( cg_showmiss.integer )
					{
						CG_Printf( "Prediction miss: %f\n", len );
					}

					if ( cg_errorDecay.integer )
					{
						int   t;
						float f;

						t = cg.time - cg.predictedErrorTime;
						f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;

						if ( f < 0 )
						{
							f = 0;
						}

						if ( f > 0 && cg_showmiss.integer )
						{
							CG_Printf( "Double prediction decay: %f\n", f );
						}

						VectorScale( cg.predictedError, f, cg.predictedError );
					}
					else
					{
						VectorClear( cg.predictedError );
					}

					VectorAdd( delta, cg.predictedError, cg.predictedError );
					cg.predictedErrorTime = cg.oldTime;
				}
			}
		}

		// don't predict gauntlet firing, which is only supposed to happen
		// when it actually inflicts damage
		for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
		{
			cg_pmove.autoWeaponHit[ i ] = false;
		}

		if ( cg_pmove.pmove_fixed )
		{
			cg_pmove.cmd.serverTime = ( ( cg_pmove.cmd.serverTime + cg.pmoveParams.msec - 1 ) /
			                            cg.pmoveParams.msec ) * cg.pmoveParams.msec;
		}

		if ( !cg_optimizePrediction.integer )
		{
			Pmove( &cg_pmove );
		}
		else if ( cg_optimizePrediction.integer && ( cmdNum >= predictCmd ||
		          ( stateIndex + 1 ) % NUM_SAVED_STATES == cg.stateHead ) )
		{
			Pmove( &cg_pmove );
			// record the last predicted command
			cg.lastPredictedCommand = cmdNum;

			// if we haven't run out of space in the saved states queue
			if ( ( stateIndex + 1 ) % NUM_SAVED_STATES != cg.stateHead )
			{
				// save the state for the false case ( of cmdNum >= predictCmd )
				// in later calls to this function
				cg.savedPmoveStates[ stateIndex ] = *cg_pmove.ps;
				stateIndex = ( stateIndex + 1 ) % NUM_SAVED_STATES;
				cg.stateTail = stateIndex;
			}
		}
		else
		{
			*cg_pmove.ps = cg.savedPmoveStates[ stateIndex ];
			stateIndex = ( stateIndex + 1 ) % NUM_SAVED_STATES;
		}

		// add push trigger movement effects
		CG_TouchTriggerPrediction();

		// check for predictable events that changed from previous predictions
		//CG_CheckChangedPredictableEvents(&cg.predictedPlayerState);
	}

	// adjust for the movement of the groundentity
	CG_AdjustPositionForMover( cg.predictedPlayerState.origin,
	                           cg.predictedPlayerState.groundEntityNum,
	                           cg.physicsTime, cg.time, cg.predictedPlayerState.origin, cg.predictedPlayerState.viewangles, cg.predictedPlayerState.viewangles);

	// fire events and other transition triggered things
	CG_TransitionPlayerState( &cg.predictedPlayerState, &oldPlayerState );
}
