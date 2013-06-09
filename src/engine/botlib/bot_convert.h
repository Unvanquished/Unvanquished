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

#ifndef __BOT_VEC_H
#define __BOT_VEC_H

#include "bot_types.h"

//coordinate conversion
static inline void quake2recast( float vec[ 3 ] )
{
	float temp = vec[1];
	vec[1] = vec[2];
	vec[2] = temp;
}

static inline void recast2quake( float vec[ 3 ] )
{
	float temp = vec[1];
	vec[1] = vec[2];
	vec[2] = temp;
}

class rVec;
class qVec
{
	float v[ 3 ];

public:
	qVec( ) { }
	qVec( float x, float y, float z );
	qVec( const float vec[ 3 ] );
	qVec( rVec vec );

	inline float &operator[]( int i )
	{
		return v[ i ];
	}

	inline operator const float*() const
	{
		return v;
	}

	inline operator float*()
	{
		return v;
	}
};

class rVec
{
	float v[ 3 ];
public:
	rVec() { }
	rVec( float x, float y, float z );
	rVec( const float vec[ 3 ] );
	rVec( qVec vec );

	inline float &operator[]( int i )
	{
		return v[ i ];
	}
	
	inline operator const float*() const
	{
		return v;
	}

	inline operator float*()
	{
		return v;
	}
};

class rBounds
{
public:
	rVec mins, maxs;

	rBounds()
	{
		clear();
	}

	rBounds( const rBounds &b )
	{
		mins = b.mins;
		maxs = b.maxs;
	}

	rBounds( qVec min, qVec max );
	rBounds( const float min[ 3 ], const float max[ 3 ] );

	void addPoint( rVec p );
	void clear();
};

struct botRouteTargetInternal
{
	botRouteTargetType_t type;
	rVec pos;
	rVec polyExtents;

	botRouteTargetInternal() { }
	botRouteTargetInternal( const botRouteTarget_t &target );
};

#endif