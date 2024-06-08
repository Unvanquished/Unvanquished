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

// Conventions:
// - Always use rVec for a vector using the Recast coordinate system.
// - A glm::vec3 is assumed to use the Quake coordinate system.
class rVec
{
	float v[ 3 ];

public:
	// uninitialized, but you can do rVec() or rVec{} to init with zeroes
	rVec() = default;

	// uses recast convention for arguments (x z y from quake perspective)
	rVec( float x, float y, float z ) : v{ x, y, z } {}

	// uses quake convention for argument
	explicit rVec( const glm::vec3 &qvec ) : v{ qvec.x, qvec.z, qvec.y } {}

	static rVec Load( const float *p )
	{
		return { p[ 0 ], p[ 1 ], p[ 2 ] };
	}

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

	glm::vec3 ToQuake() const
	{
		return { v[0], v[2], v[1] };
	}
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
