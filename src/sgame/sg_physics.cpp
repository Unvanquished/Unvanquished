/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

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

#include "sg_local.h"
#include "sg_cm_world.h"

/*
================
G_Bounce

================
*/
static void G_Bounce( gentity_t *ent, trace_t *trace )
{
	float minNormal = 0.707f;
	bool invert = false;

	// reflect the velocity on the trace plane
	int hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	glm::vec3 velocity;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, &velocity[0] );
	float dot = glm::dot( velocity, VEC2GLM( trace->plane.normal ) );
	velocity -= 2.f * dot * VEC2GLM( trace->plane.normal );
	VectorCopy( velocity, ent->s.pos.trDelta );

	if ( ent->s.eType == entityType_t::ET_BUILDABLE )
	{
		minNormal = BG_Buildable( ent->s.modelindex )->minNormal;
		invert = BG_Buildable( ent->s.modelindex )->invertNormal;
	}

	// cut the velocity to keep from bouncing forever
	if ( ( trace->plane.normal[ 2 ] >= minNormal ||
	       ( invert && trace->plane.normal[ 2 ] <= -minNormal ) ) &&
	     trace->entityNum == ENTITYNUM_WORLD )
	{
		VectorScale( ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta );
	}
	else
	{
		VectorScale( ent->s.pos.trDelta, 0.3f, ent->s.pos.trDelta );
	}

	if ( VectorLength( ent->s.pos.trDelta ) < 10 )
	{
		VectorMA( trace->endpos, 0.5f, trace->plane.normal, trace->endpos );  // make sure it is off ground
		G_SetOrigin( ent, VEC2GLM( trace->endpos ) );
		ent->s.groundEntityNum = trace->entityNum;
		VectorCopy( trace->plane.normal, ent->s.origin2 );
		VectorSet( ent->s.pos.trDelta, 0.0f, 0.0f, 0.0f );
		return;
	}

	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin );
	ent->s.pos.trTime = level.time;
}

#define PHYSICS_TIME 200

/*
================
G_Physics

================
*/
void G_Physics( gentity_t *ent )
{
	// if groundentity has been set to ENTITYNUM_NONE, it may have been pushed off an edge
	if ( ent->s.groundEntityNum == ENTITYNUM_NONE )
	{
		if ( ent->s.eType == entityType_t::ET_BUILDABLE )
		{
			if ( ent->s.pos.trType != BG_Buildable( ent->s.modelindex )->traj )
			{
				ent->s.pos.trType = BG_Buildable( ent->s.modelindex )->traj;
				ent->s.pos.trTime = level.time;
			}
		}
		else if ( ent->s.pos.trType != trType_t::TR_GRAVITY )
		{
			ent->s.pos.trType = trType_t::TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	glm::vec3 origin;
	trace_t tr;
	if ( ent->s.pos.trType == trType_t::TR_STATIONARY )
	{
		// check think function
		G_RunThink( ent );

		//check floor infrequently
		if ( ent->nextPhysicsTime < level.time )
		{
			origin = VEC2GLM( ent->r.currentOrigin );
			origin -= 2.f * VEC2GLM( ent->s.origin2 );

			G_CM_Trace( &tr, VEC2GLM( ent->r.currentOrigin ), VEC2GLM( ent->r.mins ), VEC2GLM( ent->r.maxs ), origin, ent->num(), ent->clipmask, 0, traceType_t::TT_AABB );

			if ( tr.fraction == 1.0f )
			{
				ent->s.groundEntityNum = ENTITYNUM_NONE;
			}

			ent->nextPhysicsTime = level.time + PHYSICS_TIME;
		}

		return;
	}

	// trace a line from the previous position to the current position

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, &origin[0] );

	G_CM_Trace( &tr, VEC2GLM( ent->r.currentOrigin ), VEC2GLM( ent->r.mins ), VEC2GLM( ent->r.maxs ), origin, ent->num(), ent->clipmask, 0, traceType_t::TT_AABB );

	VectorCopy( tr.endpos, ent->r.currentOrigin );

	if ( tr.startsolid )
	{
		tr.fraction = 0.f;
	}

	G_CM_LinkEntity( ent );  // FIXME: avoid this for stationary?

	// check think function
	G_RunThink( ent );

	if ( tr.fraction == 1.0f )
	{
		return;
	}

	// if it is in a nodrop volume, remove it
	int contents = G_CM_PointContents( VEC2GLM( ent->r.currentOrigin ), -1 );

	if ( contents & CONTENTS_NODROP )
	{
		G_FreeEntity( ent );
		return;
	}

	G_Bounce( ent, &tr );
}
