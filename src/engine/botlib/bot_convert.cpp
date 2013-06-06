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
		polyExtents[ i ] = ( polyExtents[ i ] < 0 ) ? -polyExtents[ 0 ] : polyExtents[ 0 ];
	}
}