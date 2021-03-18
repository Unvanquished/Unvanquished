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
===============
*/
static void CG_LoadArenasFromFile( char *filename )
{
	int          len;
	fileHandle_t f;
	char         buf[ MAX_ARENAS_TEXT ];

	len = trap_FS_FOpenFile( filename, &f, fsMode_t::FS_READ );

	if ( !f )
	{
		Log::Warn( "%sfile not found: %s", Color::ToString( Color::Red ), filename );
		return;
	}

	if ( len >= MAX_ARENAS_TEXT )
	{
		Log::Warn( "%sfile too large: %s is %i, max allowed is %i",
			Color::ToString( Color::Red ), filename, len, MAX_ARENAS_TEXT );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[ len ] = 0;
	trap_FS_FCloseFile( f );

	cg_numArenas += CG_ParseInfos( buf, MAX_ARENAS - cg_numArenas, &cg_arenaInfos[ cg_numArenas ] );
}

/*
===============
CG_MapLoadNameCompare
===============
*/
static int CG_MapLoadNameCompare( const void *a, const void *b )
{
	mapInfo_t *A = ( mapInfo_t * ) a;
	mapInfo_t *B = ( mapInfo_t * ) b;

	return Q_stricmp( A->mapLoadName, B->mapLoadName );
}

/*
===============
CG_LoadArenas
===============
*/
void CG_LoadArenas()
{
	int  numdirs;
	char filename[ 128 ];
	char dirlist[ 4096 ];
	char *dirptr;
	int  i, n;
	int  dirlen;

	cg_numArenas = 0;
	rocketInfo.data.mapCount = 0;

	// get all directories from meta
	numdirs = trap_FS_GetFileListRecursive( "meta", ".arena", dirlist, sizeof( dirlist ) );
	dirptr = dirlist;

	for ( i = 0; i < numdirs; i++, dirptr += dirlen + 1 )
	{
		dirlen = strlen( dirptr );
		Q_strncpyz( filename, "meta/", sizeof filename );
		Q_strcat( filename, sizeof filename, dirptr );
		CG_LoadArenasFromFile( filename );
	}

	Log::Warn( S_SKIPNOTIFY "%i arenas parsed\n", cg_numArenas );

	for ( n = 0; n < cg_numArenas; n++ )
	{
		rocketInfo.data.mapList[ rocketInfo.data.mapCount ].mapLoadName = BG_strdup( Info_ValueForKey( cg_arenaInfos[ n ], "map" ) );
		rocketInfo.data.mapList[ rocketInfo.data.mapCount ].mapName = BG_strdup( Info_ValueForKey( cg_arenaInfos[ n ], "longname" ) );
		rocketInfo.data.mapCount++;

		if ( rocketInfo.data.mapCount >= MAX_MAPS )
		{
			break;
		}
	}

	qsort( rocketInfo.data.mapList, rocketInfo.data.mapCount, sizeof( mapInfo_t ), CG_MapLoadNameCompare );
}
