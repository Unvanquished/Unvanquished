/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2013 Daemon Developers
All rights reserved.

This file is part of the Daemon BSD Source Code (Daemon Source Code).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS

===========================================================================
*/

#ifndef BOTLIB_CONVERT_H_
#define BOTLIB_CONVERT_H_

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
	qVec( ) = default;
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
	rVec() = default;
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

	botRouteTargetInternal() = default;
	botRouteTargetInternal( const botRouteTarget_t &target );
};

#endif
