/*
===========================================================================

Copyright 2014 Unvanquished Development

This file is part of Daemon GPL Source.

Daemon GPL Source is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "engine/qcommon/q_shared.h"
#include "bg_public.h"

/**
 * @brief Given an origin, a target point, an initial velocity v0, and a gravity g,
 *        calculate the two possible pitch angles and corresponding directions for
 *        reaching the target on a parabolic trajectory.
 * @return true when angles contains the two possible pitch angles and dir1 and dir2
 *         contain the corresponding directions. false when the target is out of reach.
 */
bool BG_GetTrajectoryPitch( vec3_t origin, vec3_t target, float v0, float g,
                                vec2_t angles, vec3_t dir1, vec3_t dir2 )
{
	vec3_t t3;  // target relative to origin in 3D space
	vec2_t t2;  // relative target in 2D space (ignoring yaw angle)
	float  tmp; // temporary variable
	float  v02; // v0 * v0

	VectorSubtract( target, origin, t3 );

	// abort if the distance is very small
	if ( VectorLength( t3 ) < 0.1f )
	{
		return false;
	}

	v02 = v0 * v0;

	// create a 2D representation of the problem by cutting out the plane that contains
	// the trajectory
	t2[ 1 ] = t3[ 2 ];
	t2[ 0 ] = sqrt( t3[ 0 ] * t3[ 0 ] + t3[ 1 ] * t3[ 1 ] );

	// calculate the angles
	tmp = v02 * v02 - g * ( g * t2[ 0 ] * t2[ 0 ] + 2.0f * t2[ 1 ] * v02 );

	if ( tmp < 0.0f )
	{
		// target not reachable
		return false;
	}

	tmp = sqrt( tmp );

	angles[ 0 ] = atanf( ( v02 + tmp ) / ( g * t2[ 0 ] ) );
	angles[ 1 ] = atanf( ( v02 - tmp ) / ( g * t2[ 0 ] ) );

	// calculate the corresponding directions for both angles
	dir1[ 0 ] = dir2[ 0 ] = t3[ 0 ];
	dir1[ 1 ] = dir2[ 1 ] = t3[ 1 ];

	if ( fabs( t3[ 0 ] ) < 0.01f && fabs( t3[ 1 ] ) < 0.01f ) // epsilon to prevent DBZ
	{
		// if target is below or above origin, we already know the direction
		dir1[ 2 ] = dir2[ 2 ] = t3[ 2 ];
	}
	else
	{
		// normalize direction so it can be rotated more easily
		dir1[ 2 ] = dir2[ 2 ] = 0.0f;
		VectorNormalize( dir1 );
		VectorNormalize( dir2 );

		// angles are in ]-pi/2,pi/2[, so cos != 0
		dir1[ 2 ] = sqrt( 1.0f / cos( angles[ 0 ] ) - 1.0f );
		dir2[ 2 ] = sqrt( 1.0f / cos( angles[ 1 ] ) - 1.0f );

		if ( angles[ 0 ] < 0.0f )
		{
			dir1[ 2 ] = -dir1[ 2 ];
		}

		if ( angles[ 1 ] < 0.0f )
		{
			dir2[ 2 ] = -dir2[ 2 ];
		}
	}

	VectorNormalize( dir1 );
	VectorNormalize( dir2 );

	return true;
}

void BG_BuildEntityDescription( char *str, size_t size, entityState_t *es )
{
	Q_snprintf(str, size, "%s #%i", Com_EntityTypeName( es->eType ), es->number );
	str[ size -1 ] = '\0';
}

bool BG_IsMainStructure( buildable_t buildable )
{
	switch ( buildable )
	{
		case BA_A_OVERMIND:
		case BA_H_REACTOR:
			return true;

		default:
			return false;
	}
}


bool BG_IsMainStructure( entityState_t *es )
{
	if ( es->eType != ET_BUILDABLE ) return false;

	return BG_IsMainStructure( (buildable_t)es->modelindex );
}

/**
 * @brief Moves a point from the origin of a bounding box to its center.
 * @param point The origin, will be modified.
 * @param mins BBOX mins.
 * @param maxs BBOX maxs.
 */
void BG_MoveOriginToBBOXCenter( vec3_t point, const vec3_t mins, const vec3_t maxs )
{
	point[ 0 ] = point[ 0 ] + ( maxs[ 0 ] + mins[ 0 ] ) * 0.5f;
	point[ 1 ] = point[ 1 ] + ( maxs[ 1 ] + mins[ 1 ] ) * 0.5f;
	point[ 2 ] = point[ 2 ] + ( maxs[ 2 ] + mins[ 2 ] ) * 0.5f;
}
