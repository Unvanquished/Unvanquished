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

// bg_slidemove.c -- part of bg_pmove functionality

#include "../../engine/qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

/*

input: origin, velocity, bounds, groundPlane, trace function

output: origin, velocity, impacts, stairup boolean

*/

/*
==================
PM_SlideMove

Returns qtrue if the velocity was clipped in some way
==================
*/
#define MAX_CLIP_PLANES 5
qboolean  PM_SlideMove( qboolean gravity )
{
	int     bumpcount, numbumps;
	vec3_t  dir;
	float   d;
	int     numplanes;
	vec3_t  planes[ MAX_CLIP_PLANES ];
	vec3_t  primal_velocity;
	vec3_t  clipVelocity;
	int     i, j, k;
	trace_t trace;
	vec3_t  end;
	float   time_left;
	float   into;
	vec3_t  endVelocity;
	vec3_t  endClipVelocity;

	numbumps = 4;

	VectorCopy( pm->ps->velocity, primal_velocity );
	VectorCopy( pm->ps->velocity, endVelocity );

	if ( gravity )
	{
		endVelocity[ 2 ] -= pm->ps->gravity * pml.frametime;
		pm->ps->velocity[ 2 ] = ( pm->ps->velocity[ 2 ] + endVelocity[ 2 ] ) * 0.5;
		primal_velocity[ 2 ] = endVelocity[ 2 ];

		if ( pml.groundPlane )
		{
			// slide along the ground plane
			PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );
		}
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if ( pml.groundPlane )
	{
		numplanes = 1;
		VectorCopy( pml.groundTrace.plane.normal, planes[ 0 ] );
	}
	else
	{
		numplanes = 0;
	}

	// never turn against original velocity
	VectorNormalize2( pm->ps->velocity, planes[ numplanes ] );
	numplanes++;

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ )
	{
		// calculate position we are trying to move to
		VectorMA( pm->ps->origin, time_left, pm->ps->velocity, end );

		// see if we can make it there
		pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask );

		if ( trace.allsolid )
		{
			// entity is completely trapped in another solid
			pm->ps->velocity[ 2 ] = 0; // don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		if ( trace.fraction > 0 )
		{
			// actually covered some distance
			VectorCopy( trace.endpos, pm->ps->origin );
		}

		if ( trace.fraction == 1 )
		{
			break; // moved the entire distance
		}

		// save entity for contact
		PM_AddTouchEnt( trace.entityNum );

		time_left -= time_left * trace.fraction;

		if ( numplanes >= MAX_CLIP_PLANES )
		{
			// this shouldn't really happen
			VectorClear( pm->ps->velocity );
			return qtrue;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for ( i = 0; i < numplanes; i++ )
		{
			if ( DotProduct( trace.plane.normal, planes[ i ] ) > 0.99 )
			{
				VectorAdd( trace.plane.normal, pm->ps->velocity, pm->ps->velocity );
				break;
			}
		}

		if ( i < numplanes )
		{
			continue;
		}

		VectorCopy( trace.plane.normal, planes[ numplanes ] );
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0; i < numplanes; i++ )
		{
			into = DotProduct( pm->ps->velocity, planes[ i ] );

			if ( into >= 0.1 )
			{
				continue; // move doesn't interact with the plane
			}

			// see how hard we are hitting things
			if ( -into > pml.impactSpeed )
			{
				pml.impactSpeed = -into;
			}

			// slide along the plane
			PM_ClipVelocity( pm->ps->velocity, planes[ i ], clipVelocity );

			// slide along the plane
			PM_ClipVelocity( endVelocity, planes[ i ], endClipVelocity );

			// see if there is a second plane that the new move enters
			for ( j = 0; j < numplanes; j++ )
			{
				if ( j == i )
				{
					continue;
				}

				if ( DotProduct( clipVelocity, planes[ j ] ) >= 0.1 )
				{
					continue; // move doesn't interact with the plane
				}

				// try clipping the move to the plane
				PM_ClipVelocity( clipVelocity, planes[ j ], clipVelocity );
				PM_ClipVelocity( endClipVelocity, planes[ j ], endClipVelocity );

				// see if it goes back into the first clip plane
				if ( DotProduct( clipVelocity, planes[ i ] ) >= 0 )
				{
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct( planes[ i ], planes[ j ], dir );
				VectorNormalize( dir );
				d = DotProduct( dir, pm->ps->velocity );
				VectorScale( dir, d, clipVelocity );

				CrossProduct( planes[ i ], planes[ j ], dir );
				VectorNormalize( dir );
				d = DotProduct( dir, endVelocity );
				VectorScale( dir, d, endClipVelocity );

				// see if there is a third plane the new move enters
				for ( k = 0; k < numplanes; k++ )
				{
					if ( k == i || k == j )
					{
						continue;
					}

					if ( DotProduct( clipVelocity, planes[ k ] ) >= 0.1 )
					{
						continue; // move doesn't interact with the plane
					}

					// stop dead at a tripple plane interaction
					VectorClear( pm->ps->velocity );
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy( clipVelocity, pm->ps->velocity );
			VectorCopy( endClipVelocity, endVelocity );
			break;
		}
	}

	if ( gravity )
	{
		VectorCopy( endVelocity, pm->ps->velocity );
	}

	// don't change velocity if in a timer (FIXME: is this correct?)
	if ( pm->ps->pm_time )
	{
		VectorCopy( primal_velocity, pm->ps->velocity );
	}

	return ( bumpcount != 0 );
}

/*
==================
PM_StepEvent
==================
*/
void PM_StepEvent( const vec3_t from, const vec3_t to, const vec3_t normal )
{
	float  size;
	vec3_t delta, dNormal;

	VectorSubtract( from, to, delta );
	VectorCopy( delta, dNormal );
	VectorNormalize( dNormal );

	size = DotProduct( normal, dNormal ) * VectorLength( delta );

	if ( size > 0.0f )
	{
		if ( size > 2.0f )
		{
			if ( size < 7.0f )
			{
				PM_AddEvent( EV_STEPDN_4 );
			}
			else if ( size < 11.0f )
			{
				PM_AddEvent( EV_STEPDN_8 );
			}
			else if ( size < 15.0f )
			{
				PM_AddEvent( EV_STEPDN_12 );
			}
			else
			{
				PM_AddEvent( EV_STEPDN_16 );
			}
		}
	}
	else
	{
		size = fabs( size );

		if ( size > 2.0f )
		{
			if ( size < 7.0f )
			{
				PM_AddEvent( EV_STEP_4 );
			}
			else if ( size < 11.0f )
			{
				PM_AddEvent( EV_STEP_8 );
			}
			else if ( size < 15.0f )
			{
				PM_AddEvent( EV_STEP_12 );
			}
			else
			{
				PM_AddEvent( EV_STEP_16 );
			}
		}
	}

	if ( pm->debugLevel > 1 )
	{
		Com_Printf( "%i:stepped\n", c_pmove );
	}
}

/*
==================
PM_StepSlideMove
==================
*/
qboolean PM_StepSlideMove( qboolean gravity, qboolean predictive )
{
	vec3_t   start_o, start_v;
	vec3_t   down_o, down_v;
	trace_t  trace;
	vec3_t   normal;
	vec3_t   step_v, step_vNormal;
	vec3_t   up, down;
	float    stepSize;
	qboolean stepped = qfalse;

	BG_GetClientNormal( pm->ps, normal );

	VectorCopy( pm->ps->origin, start_o );
	VectorCopy( pm->ps->velocity, start_v );

	if ( PM_SlideMove( gravity ) == 0 )
	{
		VectorCopy( start_o, down );
		VectorMA( down, -STEPSIZE, normal, down );
		pm->trace( &trace, start_o, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask );

		//we can step down
		if ( trace.fraction > 0.01f && trace.fraction < 1.0f &&
		     !trace.allsolid && pml.groundPlane != qfalse )
		{
			if ( pm->debugLevel > 1 )
			{
				Com_Printf( "%d: step down\n", c_pmove );
			}

			stepped = qtrue;
		}
	}
	else
	{
		VectorCopy( start_o, down );
		VectorMA( down, -STEPSIZE, normal, down );
		pm->trace( &trace, start_o, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask );

		// never step up when you still have up velocity
		if ( DotProduct( trace.plane.normal, pm->ps->velocity ) > 0.0f &&
		     ( trace.fraction == 1.0f || DotProduct( trace.plane.normal, normal ) < 0.7f ) )
		{
			return stepped;
		}

		// never step up when flying upwards with the jetpack
		if ( pm->ps->velocity[ 2 ] > 0.0f && ( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE ) )
		{
			return stepped;
		}

		VectorCopy( pm->ps->origin, down_o );
		VectorCopy( pm->ps->velocity, down_v );

		VectorCopy( start_o, up );
		VectorMA( up, STEPSIZE, normal, up );

		// test the player position if they were a stepheight higher
		pm->trace( &trace, start_o, pm->mins, pm->maxs, up, pm->ps->clientNum, pm->tracemask );

		if ( trace.allsolid )
		{
			if ( pm->debugLevel > 1 )
			{
				Com_Printf( "%i:bend can't step\n", c_pmove );
			}

			return stepped; // can't step up
		}

		VectorSubtract( trace.endpos, start_o, step_v );
		VectorCopy( step_v, step_vNormal );
		VectorNormalize( step_vNormal );

		stepSize = DotProduct( normal, step_vNormal ) * VectorLength( step_v );
		// try slidemove from this position
		VectorCopy( trace.endpos, pm->ps->origin );
		VectorCopy( start_v, pm->ps->velocity );

		if ( PM_SlideMove( gravity ) == 0 )
		{
			if ( pm->debugLevel > 1 )
			{
				Com_Printf( "%d: step up\n", c_pmove );
			}

			stepped = qtrue;
		}

		// push down the final amount
		VectorCopy( pm->ps->origin, down );
		VectorMA( down, -stepSize, normal, down );
		pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask );

		if ( !trace.allsolid )
		{
			VectorCopy( trace.endpos, pm->ps->origin );
		}

		if ( trace.fraction < 1.0f )
		{
			PM_ClipVelocity( pm->ps->velocity, trace.plane.normal, pm->ps->velocity );
		}
	}

	if ( !predictive && stepped )
	{
		PM_StepEvent( start_o, pm->ps->origin, normal );
	}

	return stepped;
}

/*
==================
PM_PredictStepMove
==================
*/
qboolean PM_PredictStepMove( void )
{
	vec3_t   velocity, origin;
	float    impactSpeed;
	qboolean stepped = qfalse;

	VectorCopy( pm->ps->velocity, velocity );
	VectorCopy( pm->ps->origin, origin );
	impactSpeed = pml.impactSpeed;

	if ( PM_StepSlideMove( qfalse, qtrue ) )
	{
		stepped = qtrue;
	}

	VectorCopy( velocity, pm->ps->velocity );
	VectorCopy( origin, pm->ps->origin );
	pml.impactSpeed = impactSpeed;

	return stepped;
}
