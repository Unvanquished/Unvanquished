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
// arena and bot info
//
static int  cg_numArenas;
static char *cg_arenaInfos[ MAX_ARENAS ];

/*
===============
CG_ParseInfos
===============
*/
int CG_ParseInfos( const char *buf, int max, char *infos[] )
{
	char *token;
	int  count = 0;
	char key[ MAX_TOKEN_CHARS ];
	char info[ MAX_INFO_STRING ];

	auto infoPostfixLen = strlen( "\\num\\" ) + strlen( va( "%d", MAX_ARENAS ) );

	while ( 1 )
	{
		token = COM_Parse( &buf );

		if ( !token[ 0 ] )
		{
			break;
		}

		if ( strcmp( token, "{" ) )
		{
			Log::Notice( "Missing { in info file\n" );
			break;
		}

		if ( count == max )
		{
			Log::Notice( "Max infos exceeded\n" );
			break;
		}

		info[ 0 ] = '\0';

		while ( 1 )
		{
			token = COM_ParseExt( &buf, true );

			if ( !token[ 0 ] )
			{
				Log::Notice( "Unexpected end of info file\n" );
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
				strcpy( token, "<NULL>" );
			}

			Info_SetValueForKey( info, key, token, false );
		}

		//NOTE: extra space for arena number
		infos[ count ] = (char*) BG_Alloc( strlen( info ) + infoPostfixLen + 1 );

		if ( infos[ count ] )
		{
			strcpy( infos[ count ], info );
			count++;
		}
	}

	return count;
}

/*
===============
CG_LoadArenasFromFile

TODO: consider deleting, unless we want to display 'longname' somewhere
===============
*/
void CG_LoadArenasFromFile( char *filename )
{
	std::error_code err;
	std::string text = FS::PakPath::ReadFile( filename, err );
	if ( err )
	{
		Log::Warn( "file not found: %s", filename );
		return;
	}

	if ( text.size() >= MAX_ARENAS_TEXT )
	{
		Log::Warn( "file too large: %s is %i, max allowed is %i",
			filename, text.size(), MAX_ARENAS_TEXT );
		return;
	}

	cg_numArenas += CG_ParseInfos( text.c_str(), MAX_ARENAS - cg_numArenas, &cg_arenaInfos[ cg_numArenas ] );
}

/*
===============
CG_LoadArenas
===============
*/
void CG_LoadArenas()
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
