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

#include "bot_convert.h"

rVec::rVec( float x, float y, float z )
{
	v[ 0 ] = x;
	v[ 1 ] = y;
	v[ 2 ] = z;
}

rVec::rVec( const float vec[ 3 ] )
{
	v[ 0 ] = vec[ 0 ];
	v[ 1 ] = vec[ 1 ];
	v[ 2 ] = vec[ 2 ];
}

rVec::rVec( qVec vec )
{
	v[ 0 ] = vec[ 0 ];
	v[ 1 ] = vec[ 1 ];
	v[ 2 ] = vec[ 2 ];
	quake2recast( v );
}

qVec::qVec( float x, float y, float z )
{
	v[ 0 ] = x;
	v[ 1 ] = y;
	v[ 2 ] = z;
}

qVec::qVec( const float vec[ 3 ] )
{
	v[ 0 ] = vec[ 0 ];
	v[ 1 ] = vec[ 1 ];
	v[ 2 ] = vec[ 2 ];
}

qVec::qVec( rVec vec )
{
	v[ 0 ] = vec[ 0 ];
	v[ 1 ] = vec[ 1 ];
	v[ 2 ] = vec[ 2 ];
	recast2quake( v );
}

rBounds::rBounds( qVec min, qVec max )
{
	clear();
	addPoint( min );
	addPoint( max );
}

rBounds::rBounds( const float min[ 3 ], const float max[ 3 ] )
{
	for ( int i = 0; i < 3; i++ )
	{
		mins[ i ] = min[ i ];
		maxs[ i ] = max[ i ];
	}
}

void rBounds::addPoint( rVec p )
{
	if ( p[ 0 ] < mins[ 0 ] )
	{
		mins[ 0 ] = p[ 0 ];
	}

	if ( p[ 0 ] > maxs[ 0 ] )
	{
		maxs[ 0 ] = p[ 0 ];
	}

	if ( p[ 1 ] < mins[ 1 ] )
	{
		mins[ 1 ] = p[ 1 ];
	}

	if ( p[ 1 ] > maxs[ 1 ] )
	{
		maxs[ 1 ] = p[ 1 ];
	}

	if ( p[ 2 ] < mins[ 2 ] )
	{
		mins[ 2 ] = p[ 2 ];
	}

	if ( p[ 2 ] > maxs[ 2 ] )
	{
		maxs[ 2 ] = p[ 2 ];
	}
}

void rBounds::clear()
{
	mins[ 0 ] = mins[ 1 ] = mins[ 2 ] = 99999;
	maxs[ 0 ] = maxs[ 1 ] = maxs[ 2 ] = -99999;
}

botRouteTargetInternal::botRouteTargetInternal( const botRouteTarget_t &target ) :
	type( target.type ), pos( qVec( target.pos ) ), polyExtents( qVec( target.polyExtents ) )
{
	for ( int i = 0; i < 3; i++ )
	{
		polyExtents[ i ] = ( polyExtents[ i ] < 0 ) ? -polyExtents[ i ] : polyExtents[ i ];
	}
}
