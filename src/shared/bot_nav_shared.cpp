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
#include "shared/CommonProxies.h"
#include "bot_nav_shared.h"

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

static void ParseOption( Str::StringRef name, Str::StringRef value, Str::StringRef file, NavgenConfig &config )
{
	float floatValue;
	if ( !Str::ToFloat( value, floatValue ) )
	{
		floatValue = 0;
	}

	int boolValue;
	if ( !Str::ParseInt( boolValue, value ) )
	{
		boolValue = -1;
	}

	if ( Str::IsIEqual( name, "cellHeight" ) )
	{
		if ( floatValue > 0 )
		{
			config.requestedCellHeight = floatValue;
			return;
		}
	}
	else if ( Str::IsIEqual( name, "stepSize" ) )
	{
		if ( floatValue > 0 )
		{
			config.stepSize = floatValue;
			return;
		}
	}
	else if ( Str::IsIEqual( name, "excludeCaulk" ) )
	{
		if ( !( boolValue & ~1 ) )
		{
			config.excludeCaulk = boolValue;
			return;
		}
	}
	else if ( Str::IsIEqual( name, "excludeSky" ) )
	{
		if ( !( boolValue & ~1 ) )
		{
			config.excludeSky = boolValue;
			return;
		}
	}
	else if ( Str::IsIEqual( name, "filterGaps" ) )
	{
		if ( !( boolValue & ~1 ) )
		{
			config.filterGaps = boolValue;
			return;
		}
	}
	else if ( Str::IsIEqual( name, "generatePatchTris" ) )
	{
		if ( !( boolValue & ~1 ) )
		{
			config.generatePatchTris = boolValue;
			return;
		}
	}
	else if ( Str::IsIEqual( name, "autojump" ) )
	{
		if ( floatValue < 0.f || 1.f < floatValue )
		{
			Log::Warn( "%s: incorrect value for autojump '%f': must be between 0 and 1", file, floatValue );
		}
		else if ( floatValue > 0.0f )
		{
			config.autojumpSecurity = floatValue;
			return;
		}
	}
	else if ( Str::IsIEqual( name, "cellSizeFactor" ) )
	{
		if ( floatValue < 0.1f || 10.f < floatValue )
		{
			Log::Warn( "%s: incorrect value for cellSizeFactor '%f': must be between 0.1 and 10", file, floatValue );
		}
		else if ( floatValue > 0.0f )
		{
			config.cellSizeFactor = floatValue;
			return;
		}
	}
	else if ( Str::IsIEqual( name, "crouch" ) )
	{
		if ( !( boolValue & ~1 ) )
		{
			config.crouchSupport = boolValue;
			return;
		}
	}
	else if ( Str::IsIEqual( name, "walkableRadiusFactor" ) )
	{
		if ( floatValue < 0.1f || 10.f < floatValue )
		{
			Log::Warn( "%s: incorrect value for walkableRadiusFactor '%f': must be between 0.1 and 10", file, floatValue );
		}
		else
		{
			config.walkableRadiusFactor = floatValue;
			return;
		}
	}
	else
	{
		Log::Warn( "%s: unknown navgen setting '%s'", file, name );
		return;
	}

	Log::Warn( "'%s' is not a valid value for %s", value, name );
}

// A configuration file can be placed in the VFS or in <homepath>/game/ that will
// customize the values in NavgenConfig (the former daemonmap flags).
// To set the default for all maps, place it at /default.navcfg
// To set values for a specific map, place it at /maps/<mapname>.navcfg
// For example:
//
// /* C or C++-style comments can be used in here since it's based on COM_Parse */
// filtergaps 1
// excludeCaulk 0
// cellheight 2.5
NavgenConfig ReadNavgenConfig( Str::StringRef mapName )
{
	NavgenConfig config = NavgenConfig::Default();
	for ( const std::string &path :
			{ std::string( "default.navcfg" ), Str::Format( "maps/%s.navcfg", mapName ) } )
	{
		int f;
		int len = BG_FOpenGameOrPakPath( path, f );
		if ( len >= 0 )
		{
			std::string buf;
			buf.resize( len );
			buf.resize( trap_FS_Read( &buf[ 0 ], len, f ) );
			trap_FS_FCloseFile( f );
			const char *p = buf.c_str();
			while ( true )
			{
				const char *token = COM_Parse( &p );
				if ( !p )
				{
					break;
				}

				std::string name = token;
				const char *value = COM_ParseExt( &p, false );

				if ( !*value )
				{
					Log::Warn( "%s: Missing value after '%s'", path, name );
					continue;
				}

				ParseOption( name, value, path, config );

				while ( *( token = COM_ParseExt( &p, false ) ) )
				{
					Log::Warn( "%s: Junk at end of line starting with '%s'", path, name );
				}
			}
		}
	}
	return config;
}

// Returns a non-empty string on error
std::string GetNavmeshHeader( fileHandle_t f, const NavgenConfig& config, NavMeshSetHeader& header, Str::StringRef mapName )
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

	if ( 0 != memcmp( &header.config, &config, sizeof(NavgenConfig) ) )
	{
		return "Navgen config changed";
	}

	return "";
}
