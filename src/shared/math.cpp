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

// for trajectory_t
#include "engine/qcommon/q_shared.h"
// for VEC2GLM
#include "shared/bg_public.h"

#include "shared/math.hpp"

#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>

glm::vec3 SafeNormalize( glm::vec3 const& o )
{
	return glm::length2( o ) != 0.f ? glm::normalize( o ) : o;
}

glm::vec3 BG_EvaluateTrajectory( const trajectory_t *tr, int atTime )
{
	float deltaTime;
	float phase;

	glm::vec3 trBase = VEC2GLM( tr->trBase );
	glm::vec3 trDelta = VEC2GLM( tr->trDelta );
	switch ( tr->trType )
	{
		case trType_t::TR_STATIONARY:
		case trType_t::TR_INTERPOLATE:
			return trBase;

		case trType_t::TR_LINEAR:
			deltaTime = ( atTime - tr->trTime ) * 0.001f; // milliseconds to seconds
			return trBase + deltaTime * trDelta;

		case trType_t::TR_SINE:
			deltaTime = ( atTime - tr->trTime ) / static_cast<float>( tr->trDuration );
			phase = sinf( deltaTime * M_PI * 2.f );
			return trBase + phase * trDelta;

		case trType_t::TR_LINEAR_STOP:
			if ( atTime > tr->trTime + tr->trDuration )
			{
				atTime = tr->trTime + tr->trDuration;
			}

			deltaTime = ( atTime - tr->trTime ) * 0.001f; // milliseconds to seconds

			if ( deltaTime < 0.0f )
			{
				deltaTime = 0.0f;
			}

			return trBase + deltaTime * trDelta;

		case trType_t::TR_GRAVITY:
			deltaTime = ( atTime - tr->trTime ) * 0.001f; // milliseconds to seconds
			trBase += deltaTime * trDelta;
			trBase[ 2 ] -= 0.5f * DEFAULT_GRAVITY * deltaTime * deltaTime; // FIXME: local gravity...
			return trBase;

		case trType_t::TR_BUOYANCY:
			deltaTime = ( atTime - tr->trTime ) * 0.001f; // milliseconds to seconds
			trBase += deltaTime * trDelta;
			trBase[ 2 ] += 0.5f * DEFAULT_GRAVITY * deltaTime * deltaTime; // FIXME: local gravity...
			return trBase;

		default:
			Sys::Drop( "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime );
	}
}

// determines velocity at a given time
glm::vec3 BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime )
{
	float deltaTime;
	float phase;
	glm::vec3 trDelta = VEC2GLM( tr->trDelta );

	switch ( tr->trType )
	{
		case trType_t::TR_STATIONARY:
		case trType_t::TR_INTERPOLATE:
			return glm::vec3();

		case trType_t::TR_LINEAR:
			return trDelta;

		case trType_t::TR_SINE:
			deltaTime = ( atTime - tr->trTime ) / static_cast<float>( tr->trDuration );
			phase = cosf( deltaTime * M_PI * 2.f );  // derivative of sin = cos
			phase *= 2.f * M_PI * 1000.f / tr->trDuration;
			return trDelta * phase;

		case trType_t::TR_LINEAR_STOP:
			if ( atTime > tr->trTime + tr->trDuration || atTime < tr->trTime )
			{
				return glm::vec3();
			}
			return trDelta;

		case trType_t::TR_GRAVITY:
			deltaTime = ( atTime - tr->trTime ) * 0.001f; // milliseconds to seconds
			trDelta[ 2 ] -= DEFAULT_GRAVITY * deltaTime; // FIXME: local gravity...
			return trDelta;

		case trType_t::TR_BUOYANCY:
			deltaTime = ( atTime - tr->trTime ) * 0.001f; // milliseconds to seconds
			trDelta[ 2 ] += DEFAULT_GRAVITY * deltaTime; // FIXME: local gravity...
			return trDelta;

		default:
			Sys::Drop( "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
	}
}
