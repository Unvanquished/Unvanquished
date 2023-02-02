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

#include "common/FileSystem.h"
#include "bot_nav_shared.h"

// File APIs...
#ifdef BUILD_CGAME
#include "engine/client/cg_api.h"
#else
#include "sgame/sg_trapcalls.h"
#endif

NavgenMapIdentification GetNavgenMapId( Str::StringRef mapName )
{
	std::string bspPath = "maps/" + mapName + ".bsp";
	const FS::LoadedPakInfo* pak = FS::PakPath::LocateFile( bspPath );
	if ( !pak )
	{
		Sys::Drop( "Can't locate %s in paks", bspPath );
	}

	NavgenMapIdentification mapId;

	switch ( pak->type )
	{
	case FS::pakType_t::PAK_ZIP:
		if ( !pak->version.empty() ) // non-legacy pak - has meaningful checksum
		{
			mapId.method = NavgenMapIdentification::PAK_CHECKSUM;
			mapId.pakChecksum = pak->realChecksum.value();
			break;
		}

		DAEMON_FALLTHROUGH;
	case FS::pakType_t::PAK_DIR:
		mapId.method = NavgenMapIdentification::BSP_SIZE;
		int handle;
		int length = trap_FS_OpenPakFile( bspPath.c_str(), handle );
		if ( length <= 0 )
		{
			Sys::Drop( "Can't find length of %s", bspPath );
		}
		trap_FS_FCloseFile( handle );
		mapId.bspSize = length;
	}

	return mapId;
}

// Returns a non-empty string on error
std::string GetNavmeshHeader( fileHandle_t f, NavMeshSetHeader& header, Str::StringRef mapName )
{
	if ( sizeof(header) != trap_FS_Read( &header, sizeof( header ), f ) )
	{
		return "File too small";
	}
	SwapNavMeshSetHeader( header );

	if ( header.magic != NAVMESHSET_MAGIC )
	{
		return "File is wrong magic";
	}

	// In principle we only need NAVMESHSET_VERSION, but people probably won't remember to change it
	// so add some extra checks
	if ( header.version != NAVMESHSET_VERSION ||
	     header.productVersionHash != ProductVersionHash() ||
	     header.headerSize != sizeof(header) )
	{
		return "File is wrong version";
	}

	NavgenMapIdentification mapId = GetNavgenMapId( mapName );
	if ( 0 != memcmp( &header.mapId, &mapId, sizeof(mapId) ) )
	{
		return "Map is different version";
	}

	NavgenConfig defaultConfig = NavgenConfig::Default();
	// Do not compare cellHeight when validating as it can be
	// automatically increased while generating navMeshes.
	if ( &header.config.stepSize == &defaultConfig.stepSize
		&& &header.config.excludeCaulk == &defaultConfig.excludeCaulk
		&& &header.config.excludeSky == &defaultConfig.excludeSky
		&& &header.config.filterGaps == &defaultConfig.filterGaps )
	{
		return "Navgen config changed";
	}

	return "";
}
