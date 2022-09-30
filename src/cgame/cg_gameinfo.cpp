/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

//
// gameinfo.c
//

#include "common/FileSystem.h"
#include "cg_local.h"

//
// arena info
//
static std::vector<arenaInfo_t> arenaList;

/*
===============
CG_ParseInfos
===============
*/
static void CG_ParseInfos( Str::StringRef text )
{
	const char *buf = text.c_str();
	const char *token;
	char key[ MAX_TOKEN_CHARS ];

	while ( true )
	{
		token = COM_Parse( &buf );

		if ( !token[ 0 ] )
		{
			break;
		}

		if ( strcmp( token, "{" ) )
		{
			Log::Warn( "Missing { in arena file" );
			break;
		}

		arenaInfo_t arenaInfo;

		while ( true )
		{
			token = COM_ParseExt( &buf, true );

			if ( !token[ 0 ] )
			{
				Log::Warn( "Unexpected end of arena file" );
				break;
			}

			if ( !strcmp( token, "}" ) )
			{
				break;
			}

			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, false );

			if ( !token[ 0 ] )
			{
				token = "<NULL>";
			}

			/* Standard in all games. */
			if ( strcmp( key, "map" ) == 0 )
			{
				arenaInfo.name = token;
			}
			/* Standard in all games. */
			else if ( strcmp( key, "longname" ) == 0 )
			{
				arenaInfo.longName = token;
			}
			/* Also used in Smokin' Guns and Q3 Rally. */
			else if ( strcmp( key, "author" ) == 0 )
			{
				arenaInfo.authors.push_back( token );
			}

			/* Some other common keys:

			* type: list of game types as words in a string,
			  example: "tourney ffa team",
			  Used by almost games that has various game modes:
			  Quake 3, Quake Live, World of Padman, Q3 Rally,
			  Urban Terror, Return to Castle Wolfenstein…
			* bots: list of bot identity as words in a string,
			  example: "sarge, uriel",
			  used by Quake 3, Q3 Rally…
			* bot_skill: bot skill level,
			  example: 3,
			  used by Quake Live.
			* timelimit: timelimit in minutes,
			  example: 10,
			  used by Quake 3, Quake Live, Wild West,
			  Return to Castle Wolfenstein,
			  Wolfenstein: Enemy Territory…
			* fraglimit: amount of frags to end a game,
			  example: 20.
			  used by Quake 3, Quake Live…
			* briefing: a string displayed to the player
			  before he enters the match,
			  used by Wolfenstein: Enemy Territory.
			* desc_line1 (2, 3 ,4): lines of text describing
			  the map,
			  used by Smokin' Guns.

			And others.

			They are ignored for now as we don't make use of them.

			As this code does not make use of “Info” storage format,
			we can also implement list of things by using multiple
			time the same key name, like the ogg metadata format
			does.

			Example, instead of:

			  type "tourney ffa team"

			We can implement:

			  type "tourney"
			  type "ffa"
			  type "team"

			This is also similar to how it is done in the .mapinfo
			files from Xonotic (using a different file format). */
		}

		if ( !arenaInfo.name.empty() )
		{
			arenaList.push_back( arenaInfo );
		}
	}
}

/*
===============
CG_LoadArenasFromFile
===============
*/
static bool CG_LoadArenasFromFile( Str::StringRef filename, bool legacy = false )
{
	if ( legacy )
	{
		Log::Debug( "Loading legacy arena file: %s", filename );
	}
	else
	{
		Log::Debug( "Loading arena file: %s", filename );
	}

	std::error_code err;
	std::string text = FS::PakPath::ReadFile( filename, err );

	if ( err )
	{
		if ( !legacy )
		{
			Log::Warn( "Arena file not found: %s", filename );
		}

		return false;
	}

	CG_ParseInfos( text );

	return true;
}

/*
===============
CG_LoadArenas
===============
*/
void CG_LoadArenas( Str::StringRef mapname )
{
	arenaList.clear();

	std::string filename = Str::Format( "meta/%s/%s.arena", mapname, mapname );

	if ( !CG_LoadArenasFromFile( filename ) )
	{
		/* Legacy games may store more than one map metadata in a
		single .arena file, the .arena file not having a basename
		in common with any map.

		The .arena files (whatever the amount of map described in)
		may be stored in various folders. Many games use the scripts/
		folder, some may use the maps/ one (like Smokin' Guns).

		Some games like Quake 3 and Quake Live also reads a file
		named scripts/arenas.txt and Quake Live also reads files
		with .sp_arena extension in script/ for single-player
		training maps. */

		char dirlist[ 4096 ];
		int numdirs = trap_FS_GetFileListRecursive( "", ".arena", dirlist, sizeof( dirlist ) );
		char *dirptr = dirlist;

		for ( int i = 0, dirlen = 0; i < numdirs; i++, dirptr += dirlen + 1 )
		{
			dirlen = strlen( dirptr );

			CG_LoadArenasFromFile( dirptr, true );
		}

		CG_LoadArenasFromFile( "scripts/arenas.txt", true );
	}

	Log::Debug( "%i arenas parsed", arenaList.size() );
}

arenaInfo_t CG_GetArenaInfo( Str::StringRef mapName )
{
	for ( arenaInfo_t &arenaInfo : arenaList )
	{
		if ( arenaInfo.name == mapName )
		{
			return arenaInfo;
		}
	}

	return {};
}

/*
===============
CG_LoadMapList
===============
*/
void CG_LoadMapList()
{
	rocketInfo.data.mapList.clear();
	for (const std::string& mapName : FS::GetAvailableMaps(false))
	{
		rocketInfo.data.mapList.push_back( {mapName} );
	}

	std::sort(
		rocketInfo.data.mapList.begin(), rocketInfo.data.mapList.end(),
		[]( const mapInfo_t& a, const mapInfo_t& b ) { return Q_stricmp( a.mapLoadName.c_str(), b.mapLoadName.c_str() ) < 0; }
	);
}
