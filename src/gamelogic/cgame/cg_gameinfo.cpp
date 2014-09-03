/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
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
int CG_ParseInfos( char *buf, int max, char *infos[] )
{
	char *token;
	int  count;
	char key[ MAX_TOKEN_CHARS ];
	char info[ MAX_INFO_STRING ];

	count = 0;

	while ( 1 )
	{
		token = COM_Parse( &buf );

		if ( !token[ 0 ] )
		{
			break;
		}

		if ( strcmp( token, "{" ) )
		{
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max )
		{
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		info[ 0 ] = '\0';

		while ( 1 )
		{
			token = COM_ParseExt( &buf, qtrue );

			if ( !token[ 0 ] )
			{
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}

			if ( !strcmp( token, "}" ) )
			{
				break;
			}

			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, qfalse );

			if ( !token[ 0 ] )
			{
				strcpy( token, "<NULL>" );
			}

			Info_SetValueForKey( info, key, token, qfalse );
		}

		//NOTE: extra space for arena number
		infos[ count ] = (char*) BG_Alloc( strlen( info ) + strlen( "\\num\\" ) + strlen( va( "%d", MAX_ARENAS ) ) + 1 );

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

	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( !f )
	{
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}

	if ( len >= MAX_ARENAS_TEXT )
	{
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_ARENAS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[ len ] = 0;
	trap_FS_FCloseFile( f );

	cg_numArenas += CG_ParseInfos( buf, MAX_ARENAS - cg_numArenas, &cg_arenaInfos[ cg_numArenas ] );
}

/*
=================
CG_MapNameCompare
=================
*/
static int CG_MapNameCompare( const void *a, const void *b )
{
	mapInfo_t *A = ( mapInfo_t * ) a;
	mapInfo_t *B = ( mapInfo_t * ) b;

	return Q_stricmp( A->mapName, B->mapName );
}

/*
===============
CG_LoadArenas
===============
*/
void CG_LoadArenas( void )
{
	int  numdirs;
	char filename[ 128 ];
	char dirlist[ 4096 ];
	char *dirptr;
	int  i, n;
	int  dirlen;
	char *type;

	cg_numArenas = 0;
	rocketInfo.data.mapCount = 0;

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList( "scripts", ".arena", dirlist, sizeof( dirlist ) );
	dirptr = dirlist;

	for ( i = 0; i < numdirs; i++, dirptr += dirlen + 1 )
	{
		dirlen = strlen( dirptr );
		strcpy( filename, "scripts/" );
		strcat( filename, dirptr );
		CG_LoadArenasFromFile( filename );
	}

	trap_Print( va( "[skipnotify]%i arenas parsed\n", cg_numArenas ) );

	for ( n = 0; n < cg_numArenas; n++ )
	{
		// determine type
		type = Info_ValueForKey( cg_arenaInfos[ n ], "type" );
		// if no type specified, it will be treated as "ffa"

		rocketInfo.data.mapList[ rocketInfo.data.mapCount ].cinematic = -1;
		rocketInfo.data.mapList[ rocketInfo.data.mapCount ].mapLoadName = BG_strdup( Info_ValueForKey( cg_arenaInfos[ n ], "map" ) );
		rocketInfo.data.mapList[ rocketInfo.data.mapCount ].mapName = BG_strdup( Info_ValueForKey( cg_arenaInfos[ n ], "longname" ) );
		rocketInfo.data.mapList[ rocketInfo.data.mapCount ].levelShot = -1;
		rocketInfo.data.mapList[ rocketInfo.data.mapCount ].imageName = BG_strdup( va( "levelshots/%s", rocketInfo.data.mapList[ rocketInfo.data.mapCount ].mapLoadName ) );

		rocketInfo.data.mapCount++;

		if ( rocketInfo.data.mapCount >= MAX_MAPS )
		{
			break;
		}
	}

	qsort( rocketInfo.data.mapList, rocketInfo.data.mapCount, sizeof( mapInfo_t ), CG_MapNameCompare );
}
