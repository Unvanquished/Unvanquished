/*
===========================================================================
Copyright (C) 2012 Unvanquished Developers

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// nav.h -- navigation mesh definitions
#ifndef __NAV_H
#define __NAV_H

#include "bot_types.h"
#include "../../libs/fastlz/fastlz.h"

// should be the same as in rest of engine
#define STEPSIZE 18
#define MIN_WALK_NORMAL 0.7f

static const int NAVMESHSET_MAGIC = 'M'<<24 | 'S'<<16 | 'E'<<8 | 'T'; //'MSET';
static const int NAVMESHSET_VERSION = 2;

struct NavMeshSetHeader
{
	int magic;
	int version;
	int numTiles;
	dtNavMeshParams params;
	dtTileCacheParams cacheParams;
};

struct NavMeshTileHeader
{
	dtCompressedTileRef tileRef;
	int dataSize;
};

struct FastLZCompressor : public dtTileCacheCompressor
{
	virtual int maxCompressedSize( const int bufferSize )
	{
		return ( int ) ( bufferSize * 1.05f );
	}

	virtual dtStatus compress( const unsigned char *buffer, const int bufferSize,
							   unsigned char *compressed, const int /*maxCompressedSize*/, int *compressedSize )
	{

		*compressedSize = fastlz_compress( ( const void *const ) buffer, bufferSize, compressed );
		return DT_SUCCESS;
	}

	virtual dtStatus decompress( const unsigned char *compressed, const int compressedSize,
								 unsigned char *buffer, const int maxBufferSize, int *bufferSize )
	{
		*bufferSize = fastlz_decompress( compressed, compressedSize, buffer, maxBufferSize );
		return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
	}
};

struct MeshProcess : public dtTileCacheMeshProcess
{
	virtual void process( struct dtNavMeshCreateParams *params, unsigned char *polyAreas, unsigned short *polyFlags )
	{
		// Update poly flags from areas.
		for ( int i = 0; i < params->polyCount; ++i )
		{
			if ( polyAreas[ i ] == DT_TILECACHE_WALKABLE_AREA )
			{
				polyAreas[ i ] = POLYAREA_GROUND;
				polyFlags[ i ] = POLYFLAGS_WALK;
			}
			else if ( polyAreas[ i ] == POLYAREA_WATER )
			{
				polyFlags[ i ] = POLYFLAGS_SWIM;
			}
			else if ( polyAreas[ i ] == POLYAREA_DOOR )
			{
				polyFlags[ i ] = POLYFLAGS_WALK | POLYFLAGS_DOOR;
			}
		}
	}
};

struct LinearAllocator : public dtTileCacheAlloc
{
	unsigned char* buffer;
	int capacity;
	int top;
	int high;

	LinearAllocator( const int cap ) : buffer(0), capacity(0), top(0), high(0)
	{
		resize(cap);
	}

	~LinearAllocator()
	{
		free( buffer );
	}

	void resize( const int cap )
	{
		if ( buffer )
		{
			free( buffer );
		}

		buffer = ( unsigned char* ) malloc( cap );
		capacity = cap;
	}

	virtual void reset()
	{
		high = dtMax( high, top );
		top = 0;
	}

	virtual void* alloc( const int size )
	{
		if ( !buffer )
		{
			return 0;
		}

		if ( top + size > capacity )
		{
			return 0;
		}

		unsigned char* mem = &buffer[ top ];
		top += size;
		return mem;
	}

	virtual void free( void* /*ptr*/ ) { }
	int getHighSize() { return high; }
};
#endif

