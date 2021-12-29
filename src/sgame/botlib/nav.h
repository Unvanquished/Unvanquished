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

// nav.h -- navigation mesh definitions
#ifndef BOTLIB_NAV_H_
#define BOTLIB_NAV_H_

#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourTileCache.h"
#include "DetourTileCacheBuilder.h"
#include "fastlz/fastlz.h"

// should be the same as in rest of engine
#define STEPSIZE 18
#define MIN_WALK_NORMAL 0.7f

static const int NAVMESHSET_MAGIC = 'M'<<24 | 'S'<<16 | 'E'<<8 | 'T'; //'MSET';
static const int NAVMESHSET_VERSION = 3;

enum navPolyFlags
{
	POLYFLAGS_DISABLED = 0,
	POLYFLAGS_WALK     = 1 << 0,
	POLYFLAGS_JUMP     = 1 << 1,
	POLYFLAGS_POUNCE   = 1 << 2,
	POLYFLAGS_WALLWALK = 1 << 3,
	POLYFLAGS_LADDER   = 1 << 4,
	POLYFLAGS_DROPDOWN = 1 << 5,
	POLYFLAGS_DOOR     = 1 << 6,
	POLYFLAGS_TELEPORT = 1 << 7,
	POLYFLAGS_CROUCH   = 1 << 8,
	POLYFLAGS_SWIM     = 1 << 9,
	POLYFLAGS_ALL      = 0xffff, // All abilities.
};

enum navPolyAreas
{
	POLYAREA_GROUND     = 1 << 0,
	POLYAREA_LADDER     = 1 << 1,
	POLYAREA_WATER      = 1 << 2,
	POLYAREA_DOOR       = 1 << 3,
	POLYAREA_JUMPPAD    = 1 << 4,
	POLYAREA_TELEPORTER = 1 << 5,
};

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

template<class T> static inline void SwapArray( T block[], size_t len )
{
	if ( LittleLong( 1 ) != 1 )
	{
		for ( size_t i = 0; i < len; i++ )
		{
			dtSwapEndian( &block[ i ] );
		}
	}
}

static inline void SwapNavMeshSetHeader( NavMeshSetHeader &header )
{
	SwapArray( ( unsigned int * ) &header, sizeof( header ) / sizeof( unsigned int ) );
}

static inline void SwapNavMeshTileHeader( NavMeshTileHeader &header )
{
	if ( LittleLong( 1 ) != 1 )
	{
		dtSwapEndian( &header.dataSize );
		dtSwapEndian( &header.tileRef );
	}
}

struct FastLZCompressor : public dtTileCacheCompressor
{
	virtual int maxCompressedSize( const int bufferSize )
	{
		return ( int ) ( bufferSize * 1.05f );
	}

	virtual dtStatus compress( const unsigned char *buffer, const int bufferSize,
							   unsigned char *compressed, const int /*maxCompressedSize*/, int *compressedSize )
	{

		*compressedSize = fastlz_compress( ( const void * ) buffer, bufferSize, compressed );
		return DT_SUCCESS;
	}

	virtual dtStatus decompress( const unsigned char *compressed, const int compressedSize,
								 unsigned char *buffer, const int maxBufferSize, int *bufferSize )
	{
		*bufferSize = fastlz_decompress( compressed, compressedSize, buffer, maxBufferSize );
		return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
	}
};



struct BasicMeshProcess : public dtTileCacheMeshProcess
{
	void process( struct dtNavMeshCreateParams *params, unsigned char *polyAreas, unsigned short *polyFlags ) override
	{
		// Update poly flags from areas.
		for ( int i = 0; i < params->polyCount; ++i )
		{
			if ( polyAreas[ i ] == DT_TILECACHE_WALKABLE_AREA )
			{
				polyAreas[ i ] = navPolyAreas::POLYAREA_GROUND;
				polyFlags[ i ] = navPolyFlags::POLYFLAGS_WALK;
			}
			else if ( polyAreas[ i ] == navPolyAreas::POLYAREA_WATER )
			{
				polyFlags[ i ] = navPolyFlags::POLYFLAGS_SWIM;
			}
			else if ( polyAreas[ i ] == navPolyAreas::POLYAREA_DOOR )
			{
				polyFlags[ i ] = navPolyFlags::POLYFLAGS_WALK | navPolyFlags::POLYFLAGS_DOOR;
			}
		}
	}
};

struct LinearAllocator : public dtTileCacheAlloc
{
	unsigned char* buffer;
	size_t capacity;
	size_t top;
	size_t high;

	LinearAllocator( const size_t cap ) : buffer(nullptr), capacity(0), top(0), high(0)
	{
		resize(cap);
	}

	~LinearAllocator() override
	{
		free( buffer );
	}

	void resize( const size_t cap )
	{
		if ( buffer )
		{
			free( buffer );
		}

		buffer = ( unsigned char* ) malloc( cap );
		capacity = cap;
	}

	void reset() override
	{
		high = dtMax( high, top );
		top = 0;
	}

	void* alloc( const size_t size ) override
	{
		if ( !buffer )
		{
			return nullptr;
		}

		if ( top + size > capacity )
		{
			return nullptr;
		}

		unsigned char* mem = &buffer[ top ];
		top += size;
		return mem;
	}

	void free( void* /*ptr*/ ) override { }
	size_t getHighSize() { return high; }
};

#endif
