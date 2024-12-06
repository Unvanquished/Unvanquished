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

#ifndef MATH_HPP
#define MATH_HPP

#include <glm/vec3.hpp>
//because I'm lazy and glm::length2() is going to be used so often anyway
#include <glm/gtx/norm.hpp>

glm::vec3 SafeNormalize( glm::vec3 const& o );

#define xstr(s) str(s)
#define str(s) #s

#define normalize_warn( v )                  \
	do                                         \
	{                                          \
		if ( glm::length2( ( v ) ) == 0.f )      \
		{                                        \
			char const norm_warn_str[] = "length of 0 in " __FILE__ ": %s at " str( __LINE__ ) ", please report the bug"; \
			Log::Warn( norm_warn_str,  __func__ ); \
		}                                        \
	} while(0)

struct trajectory_t;

glm::vec3 BG_EvaluateTrajectory( const trajectory_t *tr, int atTime );
glm::vec3 BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime );

#endif
