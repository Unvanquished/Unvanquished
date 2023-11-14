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

#include "common/Common.h"

#include "DetourAssert.h"
#include "RecastAssert.h"

#include "sgame/sg_local.h"
#include "bot_local.h"

static const char* NAVCON_HEADER_PREFIX = "navcon";
static const int NAVCON_VERSION = 3;

static Cvar::Range<Cvar::Cvar<int>> maxNavNodes(
	"bot_maxNavNodes", "maximum number of nodes in navmesh", Cvar::NONE, 4096, 0, 65535);

int numNavData = 0;
NavData_t BotNavData[ MAX_NAV_DATA ];

LinearAllocator alloc( 1024 * 1024 * 16 );
FastLZCompressor comp;

// Recast uses NDEBUG to determine whether assertions are enabled.
// Make sure this is in sync with DEBUG_BUILD
#if defined(DEBUG_BUILD) != !defined(NDEBUG)
#error
#endif

#ifdef DEBUG_BUILD
static void FailAssertion(const char* expression, const char* file, int line)
{
	DAEMON_ASSERT_CALLSITE_HELPER(
		file, "Detour assert", line, , , false, Str::Format("\"%s\" is false", expression));
}
#endif

void BotAssertionInit()
{
#ifdef DEBUG_BUILD
	dtAssertFailSetCustom( FailAssertion );
	rcAssertFailSetCustom( FailAssertion );
#endif
}

void BotSaveOffMeshConnections( NavData_t *nav )
{
	fileHandle_t f = 0;

	std::string mapname = Cvar::GetValue( "mapname" );
	std::string filePath =
		Str::Format( "maps/%s-%s.navcon", mapname, BG_Class( nav->species )->name );
	trap_FS_FOpenFile( filePath.c_str(), &f, fsMode_t::FS_WRITE);

	if ( !f )
	{
		Log::Warn( "Failed to open navcon file %f", filePath );
		return;
	}

	Log::Debug("Saving format version %d navcon file %s", NAVCON_VERSION, filePath );

	int conCount = nav->process.con.offMeshConCount;
	char *s = va( "%s %d\n", NAVCON_HEADER_PREFIX, NAVCON_VERSION );

	trap_FS_Write( s, strlen( s ), f );

	for ( int n = 0; n < conCount; n++ )
	{
		// userids is unused.
		s = va( "%.0f %.0f %.0f %.0f %.0f %.0f %.0f %d %d %d\n",
			nav->process.con.verts[ ( 6 * n ) + 0 ],
			nav->process.con.verts[ ( 6 * n ) + 1 ],
			nav->process.con.verts[ ( 6 * n ) + 2 ],
			nav->process.con.verts[ ( 6 * n ) + 3 ],
			nav->process.con.verts[ ( 6 * n ) + 4 ],
			nav->process.con.verts[ ( 6 * n ) + 5 ],
			nav->process.con.rad[ n ],
			nav->process.con.flags[ n ],
			nav->process.con.areas[ n ],
			nav->process.con.dirs[ n ] );

		trap_FS_Write( s, strlen( s ), f );
	}

	trap_FS_FCloseFile( f );

	Log::Debug( "Saved %d connections to navcon file %s", conCount, filePath );
}

static void BotLoadOffMeshConnections( const char *species, OffMeshConnections &con )
{
	con.offMeshConCount = 0;

	fileHandle_t f = 0;

	std::string mapname = Cvar::GetValue("mapname");
	std::string filePath = Str::Format( "maps/%s-%s.navcon", mapname, species );
	int len = BG_FOpenGameOrPakPath( filePath, f );

	if ( !f )
	{
		Log::Debug("Failed to load \"maps/%s-%s.navcon\"", mapname.c_str(), species );
		return;
	}

	int version;
	trap_FS_Read( &version, sizeof( version ), f );

	version = LittleLong( version );

	Log::Debug("Loading format version %d navcon file %s", version, filePath );
	// Old binary format.
	if ( version == 2 )
	{
		int conCount;
		trap_FS_Read( &conCount, sizeof( conCount ), f );

		conCount = LittleLong( conCount );

		if ( conCount > con.MAX_CON )
		{
			Log::Warn( "Too many connections in navcon file %s", filePath );
			trap_FS_FCloseFile( f );
			return;
		}

		con.offMeshConCount = conCount;

		trap_FS_Read( con.verts, sizeof( float ) * 6 * conCount, f );
		SwapArray( con.verts, conCount * 6 );

		trap_FS_Read( con.rad, sizeof( float ) * conCount, f );
		SwapArray( con.rad, conCount );

		trap_FS_Read( con.flags, sizeof( unsigned short ) * conCount, f );
		SwapArray( con.flags, conCount );

		trap_FS_Read( con.areas, sizeof( unsigned char ) * conCount, f );
		trap_FS_Read( con.dirs, sizeof( unsigned char ) * conCount, f );

		// userids is loaded but unused.
		trap_FS_Read( con.userids, sizeof( unsigned int ) * conCount, f );
		SwapArray( con.userids, conCount );

		Log::Debug( "Loaded %d connections from navcon file %s", conCount, filePath );
		trap_FS_FCloseFile( f );

		return;
	}

	// Seek to the beginning of the file.
	trap_FS_Seek( f, 0, fsOrigin_t::FS_SEEK_SET );

	// New text format.
	char *navcon = (char*) BG_Alloc( len + 1 );

	trap_FS_Read( navcon, len, f );
	trap_FS_FCloseFile( f );
	navcon[ len ] = '\0';

	int lineLength = strcspn( navcon, "\n" );

	// Do not read numbers from the next line with sscanf;
	navcon[ lineLength ] = '\0';

	if ( strncmp( navcon, NAVCON_HEADER_PREFIX, strlen( NAVCON_HEADER_PREFIX ) ) != 0 )
	{
		Log::Warn( "Unknown format for navcon file %s", filePath );
		return;
	}

	// The dummy is used to detect when there are more fields than needed.
	char dummy;

	int scanned = sscanf( navcon, va( "%s %%d %%c", NAVCON_HEADER_PREFIX ), &version, &dummy );

	if ( scanned != 1 )
	{
		if ( scanned > 1 )
		{
			Log::Warn( "Malformed navcon header, extra field after format version in navcon file %s: %s", filePath, navcon );
			return;
		}

		Log::Warn( "Missing format version for navcon file %s", filePath );
		return;
	}

	if ( version != NAVCON_VERSION )
	{
		if ( version == 2 )
		{
			Log::Warn("Format version %d should not be a text format for navcon file %s", version, filePath );
			return;
		}

		Log::Warn("Unknown format version %d for navcon file %s", version, filePath );
		return;
	}

	con.offMeshConCount = 0;

	for( int pos = lineLength + 1, entry = 0; pos < len; pos += lineLength + 1, entry++ )
	{
		lineLength = strcspn( navcon + pos, "\n" );

		// Do not read numbers from the next line with sscanf;
		navcon[ pos + lineLength ] = '\0';

		if ( con.offMeshConCount > con.MAX_CON )
		{
			Log::Warn( "Too many connections in navcon file %s", filePath );
			break;
		}

		char *line = navcon + pos;

		scanned = sscanf( line, "%f %f %f %f %f %f %f %hu %hhu %hhu %c",
			&con.verts[ ( 6 * con.offMeshConCount ) + 0 ],
			&con.verts[ ( 6 * con.offMeshConCount ) + 1 ],
			&con.verts[ ( 6 * con.offMeshConCount ) + 2 ],
			&con.verts[ ( 6 * con.offMeshConCount ) + 3 ],
			&con.verts[ ( 6 * con.offMeshConCount ) + 4 ],
			&con.verts[ ( 6 * con.offMeshConCount ) + 5 ],
			&con.rad[ con.offMeshConCount ],
			&con.flags[ con.offMeshConCount ],
			&con.areas[ con.offMeshConCount ],
			&con.dirs[ con.offMeshConCount ],
			&dummy );

		if ( scanned != 10 )
		{
			if ( scanned > 10 )
			{
				Log::Warn( "Malformed navcon entry %d in navcon file %s, more than %d fields: %s", entry, filePath, 10, line );
			}
			else
			{
				Log::Warn( "Malformed navcon entry %d in navcon file %s, %d numbers instead of %d: %s", entry, filePath, scanned, 10, line );
			}
			// Read next entry in place.
			continue;
		}

		// userids is unused.
		con.userids[ con.offMeshConCount ] = 0U;
		con.offMeshConCount++;
	}

	Log::Debug( "Loaded %d connections from navcon file %s", con.offMeshConCount, filePath );

	BG_Free( navcon );
	return;
}

// Returns UNINITIALIZED (if cache is invalidated),
// LOAD_FAILED (for cached failure or internal error), or LOADED
static navMeshStatus_t BotLoadNavMesh( int f, const NavgenConfig &config, const char *species, NavData_t &nav )
{
	constexpr auto internalErrorStatus = navMeshStatus_t::LOAD_FAILED;

	NavMeshSetHeader header;
	std::string error = GetNavmeshHeader( f, config, header, Cvar::GetValue( "mapname" ) );
	if ( !error.empty() )
	{
		Log::Warn( "Loading navmesh for %s failed: %s", species, error );
		trap_FS_FCloseFile( f );
		return navMeshStatus_t::UNINITIALIZED;
	}
	else if ( header.numTiles < 0 )
	{
		char errorBuf[256] = {};
		trap_FS_Read( errorBuf, sizeof(errorBuf) - 1, f );
		Log::Warn( "Can't load navmesh for %s: Cached navmesh generation failure (%s)", species, errorBuf );
		trap_FS_FCloseFile( f );
		return navMeshStatus_t::LOAD_FAILED;
	}

	BotLoadOffMeshConnections( species, nav.process.con );

	nav.mesh = dtAllocNavMesh();

	if ( !nav.mesh )
	{
		Log::Warn("Unable to allocate nav mesh" );
		trap_FS_FCloseFile( f );
		return internalErrorStatus;
	}

	dtStatus status = nav.mesh->init( &header.params );

	if ( dtStatusFailed( status ) )
	{
		Log::Warn("Could not init navmesh" );
		dtFreeNavMesh( nav.mesh );
		nav.mesh = nullptr;
		trap_FS_FCloseFile( f );
		return internalErrorStatus;
	}

	nav.cache = dtAllocTileCache();

	if ( !nav.cache )
	{
		Log::Warn("Could not allocate tile cache" );
		dtFreeNavMesh( nav.mesh );
		nav.mesh = nullptr;
		trap_FS_FCloseFile( f );
		return internalErrorStatus;
	}

	status = nav.cache->init( &header.cacheParams, &alloc, &comp, &nav.process );

	if ( dtStatusFailed( status ) )
	{
		Log::Warn("Could not init tile cache" );
		dtFreeNavMesh( nav.mesh );
		dtFreeTileCache( nav.cache );
		nav.mesh = nullptr;
		nav.cache = nullptr;
		trap_FS_FCloseFile( f );
		return internalErrorStatus;
	}

	for ( int i = 0; i < header.numTiles; i++ )
	{
		NavMeshTileHeader tileHeader;

		trap_FS_Read( &tileHeader, sizeof( tileHeader ), f );

		SwapNavMeshTileHeader( tileHeader );

		if ( !tileHeader.tileRef || !tileHeader.dataSize )
		{
			Log::Warn("Null Tile in navmesh" );
			dtFreeNavMesh( nav.mesh );
			dtFreeTileCache( nav.cache );
			nav.cache = nullptr;
			nav.mesh = nullptr;
			trap_FS_FCloseFile( f );
			return navMeshStatus_t::UNINITIALIZED;
		}

		unsigned char *data = ( unsigned char * ) dtAlloc( tileHeader.dataSize, DT_ALLOC_PERM );

		if ( !data )
		{
			Log::Warn("Failed to allocate memory for tile data" );
			dtFreeNavMesh( nav.mesh );
			dtFreeTileCache( nav.cache );
			nav.cache = nullptr;
			nav.mesh = nullptr;
			trap_FS_FCloseFile( f );
			return internalErrorStatus;
		}

		memset( data, 0, tileHeader.dataSize );

		trap_FS_Read( data, tileHeader.dataSize, f );

		if ( LittleLong( 1 ) != 1 )
		{
			dtTileCacheHeaderSwapEndian( data, tileHeader.dataSize );
		}

		dtCompressedTileRef tile = 0;
		status = nav.cache->addTile( data, tileHeader.dataSize, DT_TILE_FREE_DATA, &tile );

		if ( dtStatusFailed( status ) )
		{
			Log::Warn("Failed to add tile to navmesh" );
			dtFree( data );
			dtFreeTileCache( nav.cache );
			dtFreeNavMesh( nav.mesh );
			nav.cache = nullptr;
			nav.mesh = nullptr;
			trap_FS_FCloseFile( f );
			return internalErrorStatus;
		}

		if ( tile )
		{
			nav.cache->buildNavMeshTile( tile, nav.mesh );
		}
	}

	trap_FS_FCloseFile( f );
	return navMeshStatus_t::LOADED;
}

void G_BotShutdownNav()
{
	for ( int i = 0; i < numNavData; i++ )
	{
		NavData_t *nav = &BotNavData[ i ];

		if ( nav->cache )
		{
			dtFreeTileCache( nav->cache );
			nav->cache = nullptr;
		}

		if ( nav->mesh )
		{
			dtFreeNavMesh( nav->mesh );
			nav->mesh = nullptr;
		}

		if ( nav->query )
		{
			dtFreeNavMeshQuery( nav->query );
			nav->query = nullptr;
		}

		nav->process.con.reset();
		nav->species = PCL_NONE;
	}

	NavEditShutdown();
	numNavData = 0;
}

navMeshStatus_t G_BotSetupNav( const NavgenConfig &config, class_t species )
{
	if ( numNavData == MAX_NAV_DATA )
	{
		Log::Warn( "maximum number of navigation meshes exceeded" );
		return navMeshStatus_t::LOAD_FAILED;
	}

	NavData_t *nav = &BotNavData[ numNavData ];

	int f;
	std::string mapname = Cvar::GetValue( "mapname" );
	std::string filePath = NavmeshFilename( mapname, species );
	BG_FOpenGameOrPakPath( filePath, f );

	if ( !f )
	{
		return navMeshStatus_t::UNINITIALIZED;
	}

	Log::Notice( " loading navigation mesh file '%s'...", filePath );

	const char *speciesName = BG_Class( species )->name;
	navMeshStatus_t loadStatus = BotLoadNavMesh( f, config, speciesName, *nav );
	if ( loadStatus != navMeshStatus_t::LOADED )
	{
		return loadStatus;
	}

	nav->species = species;
	nav->query = dtAllocNavMeshQuery();

	if ( !nav->query )
	{
		Log::Notice(
			"Could not allocate Detour Navigation Mesh Query for navmesh %s", speciesName );
		return navMeshStatus_t::LOAD_FAILED;
	}

	if ( dtStatusFailed( nav->query->init( nav->mesh, maxNavNodes.Get() ) ) )
	{
		Log::Notice( "Could not init Detour Navigation Mesh Query for navmesh %s", speciesName );
		return navMeshStatus_t::LOAD_FAILED;
	}

	nav->filter.setIncludeFlags( POLYFLAGS_WALK );
	nav->filter.setExcludeFlags( POLYFLAGS_DISABLED );
	numNavData++;
	return navMeshStatus_t::LOADED;
}
