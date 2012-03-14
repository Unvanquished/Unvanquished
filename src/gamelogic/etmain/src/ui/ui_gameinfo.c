/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

//
// gameinfo.c
//

#include "ui_local.h"

//
// arena and bot info
//

int         ui_numBots;
static char *ui_botInfos[ MAX_BOTS ];

static int  ui_numArenas;

//static char       *ui_arenaInfos[MAX_ARENAS];

//static int        ui_numSinglePlayerArenas; // TTimo: unused
//static int        ui_numSpecialSinglePlayerArenas; // TTimo: unused

/*
===============
UI_ParseInfos
===============
*/
int UI_ParseInfos( char *buf, int max, char *infos[], int totalmax )
{
	char *token;
	int  count;
	char key[ MAX_TOKEN_CHARS ];
	char info[ MAX_INFO_STRING ];

	count = 0;

	while( 1 )
	{
		token = COM_Parse( &buf );

		if( !token[ 0 ] )
		{
			break;
		}

		if( strcmp( token, "{" ) )
		{
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if( count == max )
		{
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		info[ 0 ] = '\0';

		while( 1 )
		{
			token = COM_ParseExt( &buf, qtrue );

			if( !token[ 0 ] )
			{
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}

			if( !strcmp( token, "}" ) )
			{
				break;
			}

			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, qfalse );

			if( !token[ 0 ] )
			{
				strcpy( token, "<NULL>" );
			}

			Info_SetValueForKey( info, key, token );
		}

		//NOTE: extra space for arena number
		infos[ count ] = UI_Alloc( strlen( info ) + strlen( "\\num\\" ) + strlen( va( "%d", totalmax ) ) + 1 );

		if( infos[ count ] )
		{
			strcpy( infos[ count ], info );
			count++;
		}
	}

	return count;
}

/*
===============
UI_LoadArenasFromFile
===============
*/
static void UI_LoadArenasFromFile( char *filename )
{
	/*  int       len;
	        fileHandle_t  f;
	        char      buf[MAX_ARENAS_TEXT];

	        len = trap_FS_FOpenFile( filename, &f, FS_READ );
	        if ( !f ) {
	                trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
	                return;
	        }
	        if ( len >= MAX_ARENAS_TEXT ) {
	                trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT ) );
	                trap_FS_FCloseFile( f );
	                return;
	        }

	        trap_FS_Read( buf, len, f );
	        buf[len] = 0;
	        trap_FS_FCloseFile( f );

	        ui_numArenas += UI_ParseInfos( buf, MAX_ARENAS - ui_numArenas, &ui_arenaInfos[ui_numArenas], MAX_ARENAS );*/

	int        handle;
	pc_token_t token;

	handle = trap_PC_LoadSource( filename );

	if( !handle )
	{
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}

	if( !trap_PC_ReadToken( handle, &token ) )
	{
		trap_PC_FreeSource( handle );
		return;
	}

	if( *token.string != '{' )
	{
		trap_PC_FreeSource( handle );
		return;
	}

	uiInfo.mapList[ uiInfo.mapCount ].cinematic = -1;
	uiInfo.mapList[ uiInfo.mapCount ].levelShot = -1;
	uiInfo.mapList[ uiInfo.mapCount ].typeBits = 0;

	while( trap_PC_ReadToken( handle, &token ) )
	{
		if( *token.string == '}' )
		{
			if( !uiInfo.mapList[ uiInfo.mapCount ].typeBits )
			{
				uiInfo.mapList[ uiInfo.mapCount ].typeBits |= ( 1 << GT_WOLF );
			}

			uiInfo.mapCount++;

			if( uiInfo.mapCount >= MAX_MAPS )
			{
				break;
			}

			if( !trap_PC_ReadToken( handle, &token ) )
			{
				// eof
				trap_PC_FreeSource( handle );
				return;
			}

			if( *token.string != '{' )
			{
				trap_Print( va( S_COLOR_RED "unexpected token '%s' inside: %s\n", token.string, filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
		else if( !Q_stricmp( token.string, "map" ) )
		{
			if( !PC_String_Parse( handle, &uiInfo.mapList[ uiInfo.mapCount ].mapLoadName ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
		else if( !Q_stricmp( token.string, "longname" ) )
		{
			if( !PC_String_Parse( handle, &uiInfo.mapList[ uiInfo.mapCount ].mapName ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
		else if( !Q_stricmp( token.string, "briefing" ) )
		{
			if( !PC_String_Parse( handle, &uiInfo.mapList[ uiInfo.mapCount ].briefing ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
		else if( !Q_stricmp( token.string, "lmsbriefing" ) )
		{
			if( !PC_String_Parse( handle, &uiInfo.mapList[ uiInfo.mapCount ].lmsbriefing ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}

			/*} else if( !Q_stricmp( token.string, "objectives" ) ) {
			   if( !PC_String_Parse( handle, &uiInfo.mapList[uiInfo.mapCount].objectives ) ) {
			   trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
			   trap_PC_FreeSource( handle );
			   return;
			   } */
		}
		else if( !Q_stricmp( token.string, "timelimit" ) )
		{
			if( !PC_Int_Parse( handle, &uiInfo.mapList[ uiInfo.mapCount ].Timelimit ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
		else if( !Q_stricmp( token.string, "axisrespawntime" ) )
		{
			if( !PC_Int_Parse( handle, &uiInfo.mapList[ uiInfo.mapCount ].AxisRespawnTime ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
		else if( !Q_stricmp( token.string, "alliedrespawntime" ) )
		{
			if( !PC_Int_Parse( handle, &uiInfo.mapList[ uiInfo.mapCount ].AlliedRespawnTime ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
		else if( !Q_stricmp( token.string, "type" ) )
		{
			if( !trap_PC_ReadToken( handle, &token ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
			else
			{
				if( strstr( token.string, "wolfsp" ) )
				{
					uiInfo.mapList[ uiInfo.mapCount ].typeBits |= ( 1 << GT_SINGLE_PLAYER );
				}

				if( strstr( token.string, "wolflms" ) )
				{
					uiInfo.mapList[ uiInfo.mapCount ].typeBits |= ( 1 << GT_WOLF_LMS );
				}

				if( strstr( token.string, "wolfmp" ) )
				{
					uiInfo.mapList[ uiInfo.mapCount ].typeBits |= ( 1 << GT_WOLF );
				}

				if( strstr( token.string, "wolfsw" ) )
				{
					uiInfo.mapList[ uiInfo.mapCount ].typeBits |= ( 1 << GT_WOLF_STOPWATCH );
				}
			}
		}
		else if( !Q_stricmp( token.string, "mapposition_x" ) )
		{
			if( !PC_Float_Parse( handle, &uiInfo.mapList[ uiInfo.mapCount ].mappos[ 0 ] ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
		else if( !Q_stricmp( token.string, "mapposition_y" ) )
		{
			if( !PC_Float_Parse( handle, &uiInfo.mapList[ uiInfo.mapCount ].mappos[ 1 ] ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
	}

	trap_PC_FreeSource( handle );
	return;
}

/*
===============
UI_LoadArenas
===============
*/
void UI_LoadArenas( void )
{
	int  numdirs;

//  vmCvar_t    arenasFile;
	char filename[ 128 ];
	char dirlist[ 1024 ];
	char *dirptr;
	int  i /*, n */;
	int  dirlen;

	//char      *type, *str;

	ui_numArenas = 0;
	uiInfo.mapCount = 0;

	/*  NERVE - SMF - commented out
	        trap_Cvar_Register( &arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM );
	        if( *arenasFile.string ) {
	                UI_LoadArenasFromFile(arenasFile.string);
	        }
	        else {
	                UI_LoadArenasFromFile("scripts/arenas.txt");
	        }
	*/
	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList( "scripts", ".arena", dirlist, 1024 );
	dirptr = dirlist;

	for( i = 0; i < numdirs; i++, dirptr += dirlen + 1 )
	{
		dirlen = strlen( dirptr );
		strcpy( filename, "scripts/" );
		strcat( filename, dirptr );
		UI_LoadArenasFromFile( filename );
	}

//  trap_DPrint( va( "%i arenas parsed\n", ui_numArenas ) ); // JPW NERVE pulled per atvi req

	/*  if (UI_OutOfMemory()) {
	                trap_Print(S_COLOR_YELLOW"WARNING: not anough memory in pool to load all arenas\n");
	        }*/

	/*  for( n = 0; n < ui_numArenas; n++ ) {
	                // determine type

	                uiInfo.mapList[uiInfo.mapCount].cinematic = -1;
	                uiInfo.mapList[uiInfo.mapCount].mapLoadName = String_Alloc(Info_ValueForKey(ui_arenaInfos[n], "map"));
	                uiInfo.mapList[uiInfo.mapCount].mapName = String_Alloc(Info_ValueForKey(ui_arenaInfos[n], "longname"));
	                uiInfo.mapList[uiInfo.mapCount].levelShot = -1;
	                uiInfo.mapList[uiInfo.mapCount].imageName = String_Alloc(va("levelshots/%s", uiInfo.mapList[uiInfo.mapCount].mapLoadName));
	                uiInfo.mapList[uiInfo.mapCount].typeBits = 0;

	                uiInfo.mapList[uiInfo.mapCount].briefing = String_Alloc(Info_ValueForKey(ui_arenaInfos[n], "briefing"));
	                uiInfo.mapList[uiInfo.mapCount].lmsbriefing = String_Alloc(Info_ValueForKey(ui_arenaInfos[n], "lmsbriefing"));
	//    uiInfo.mapList[uiInfo.mapCount].objectives = String_Alloc(Info_ValueForKey(ui_arenaInfos[n], "objectives"));*/
	// Gordon: cant use "\" in a key/pair translating * to \n for the moment, reeally should be using PC_ parsing stuff for this too
	// eek, no ; either.....

	// Arnout: THIS IS BAD DO NOT MODIFY A STRING AFTER IT IS ALLOCATED

	/*{
	   char* p;
	   while(p = strchr(uiInfo.mapList[uiInfo.mapCount].description, '*')) {
	   *p = '\n';
	   }

	   while(p = strchr(uiInfo.mapList[uiInfo.mapCount].objectives, '*')) {
	   *p = '\n';
	   }
	   } */

	// NERVE - SMF
	// set timelimit

	/*    str = Info_ValueForKey( ui_arenaInfos[n], "Timelimit" );
	                if ( *str )
	                        uiInfo.mapList[uiInfo.mapCount].Timelimit = atoi( str );
	                else
	                        uiInfo.mapList[uiInfo.mapCount].Timelimit = 0;

	                // set axis respawn time
	                str = Info_ValueForKey( ui_arenaInfos[n], "AxisRespawnTime" );
	                if ( *str )
	                        uiInfo.mapList[uiInfo.mapCount].AxisRespawnTime = atoi( str );
	                else
	                        uiInfo.mapList[uiInfo.mapCount].AxisRespawnTime = 0;

	                // set allied respawn time
	                str = Info_ValueForKey( ui_arenaInfos[n], "AlliedRespawnTime" );
	                if ( *str )
	                        uiInfo.mapList[uiInfo.mapCount].AlliedRespawnTime = atoi( str );
	                else
	                        uiInfo.mapList[uiInfo.mapCount].AlliedRespawnTime = 0;
	                // -NERVE - SMF

	                type = Info_ValueForKey( ui_arenaInfos[n], "type" );
	                if( *type ) {
	                        // NERVE - SMF
	                        if( strstr( type, "wolfsp" ) ) {
	                                uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_SINGLE_PLAYER);
	                        }
	                        if( strstr( type, "wolflms" ) ) {
	                                uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_WOLF_LMS);
	                        }
	                        if( strstr( type, "wolfmp" ) ) {
	                                uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_WOLF);
	                        }
	                        if( strstr( type, "wolfsw" ) ) {
	                                uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_WOLF_STOPWATCH);
	                        }
	                        // -NERVE - SMF
	                } else { // Gordon: default is wolf now
	                        uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_WOLF);
	                }

	                str = Info_ValueForKey( ui_arenaInfos[n], "mapposition_x" );
	                if ( *str ) {
	                        uiInfo.mapList[uiInfo.mapCount].mappos[0] = atof(str);
	                } else {
	                        uiInfo.mapList[uiInfo.mapCount].mappos[0] = 0.f;
	                }

	                str = Info_ValueForKey( ui_arenaInfos[n], "mapposition_y" );
	                if ( *str ) {
	                        uiInfo.mapList[uiInfo.mapCount].mappos[1] = atof(str);
	                } else {
	                        uiInfo.mapList[uiInfo.mapCount].mappos[1] = 0.f;
	                }

	                uiInfo.mapCount++;
	                if (uiInfo.mapCount >= MAX_MAPS) {
	                        break;
	                }
	        }*/
}

mapInfo        *UI_FindMapInfoByMapname( const char *name )
{
	int i;

	if( uiInfo.mapCount == 0 )
	{
		UI_LoadArenas();
	}

	for( i = 0; i < uiInfo.mapCount; i++ )
	{
		if( !Q_stricmp( uiInfo.mapList[ i ].mapLoadName, name ) )
		{
			return &uiInfo.mapList[ i ];
		}
	}

	return NULL;
}

// Campaign files

/*
===============
UI_LoadCampaignsFromFile
===============
*/

/*static void UI_LoadCampaignsFromFile( const char *filename, campaignInfo_t *campaignList, int *campaignCount, int maxCampaigns ) {
        int handle, i;
        pc_token_t token;

        handle = trap_PC_LoadSource( filename );

        if( !handle ) {
                trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
                return;
        }

        if( !trap_PC_ReadToken( handle, &token ) )
                return;
        if( *token.string != '{' )
                return;

        while ( trap_PC_ReadToken( handle, &token ) ) {
                if( *token.string == '}' ) {
                        if( campaignList[*campaignCount].initial ) {
                                campaignList[*campaignCount].unlocked = qtrue;
                                // Always unlock the initial campaign
                        }

                        *campaignCount++;

                        if( *campaignCount >= maxCampaigns || !trap_PC_ReadToken( handle, &token ) ) {
                                // eof or max campaigns reached
                                trap_PC_FreeSource( handle );
                                return;
                        }

                        if( *token.string != '{' ) {
                                //campaignList[*campaignCount].order = -1;

                                trap_Print( va( S_COLOR_RED "unexpected token '%s' inside: %s\n", token.string, filename ) );
                                trap_PC_FreeSource( handle );
                                return;
                        }
                } else if( !Q_stricmp( token.string, "shortname" ) ) {
                        if( !PC_String_Parse( handle, &campaignList[*campaignCount].campaignShortName ) ) {
                                trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
                                trap_PC_FreeSource( handle );
                                return;
                        }
                } else if( !Q_stricmp( token.string, "name" ) ) {
                        if( !PC_String_Parse( handle, &campaignList[*campaignCount].campaignName ) ) {
                                trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
                                trap_PC_FreeSource( handle );
                                return;
                        }
                } else if( !Q_stricmp( token.string, "description" ) ) {
                        if( !PC_String_Parse( handle, &campaignList[*campaignCount].campaignDescription ) ) {
                                trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
                                trap_PC_FreeSource( handle );
                                return;
                        }
                } else if( !Q_stricmp( token.string, "image" ) ) {
                        if( !PC_String_Parse( handle, &campaignList[*campaignCount].campaignShotName ) ) {
                                trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
                                trap_PC_FreeSource( handle );
                                return;
                        } else {
                                campaignList[*campaignCount].campaignShot = -1;
                        }
                } else if( !Q_stricmp( token.string, "initial" ) ) {
                        campaignList[*campaignCount].initial = qtrue;
                } else if( !Q_stricmp( token.string, "next" ) ) {
                        if( !PC_String_Parse( handle, &campaignList[*campaignCount].nextCampaignShortName ) ) {
                                trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
                                trap_PC_FreeSource( handle );
                                return;
                        }
                } else if( !Q_stricmp( token.string, "type" ) ) {
                        if( !trap_PC_ReadToken( handle, &token ) ) {
                                trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
                                trap_PC_FreeSource( handle );
                                return;
                        }

                        if( strstr( token.string, "wolfsp" ) ) {
                                campaignList[*campaignCount].typeBits |= (1 << GT_SINGLE_PLAYER);
                        }
                        if( strstr( token.string, "wolfmp" ) ) {
                                campaignList[*campaignCount].typeBits |= (1 << GT_WOLF);
                        }
                        if( strstr( token.string, "wolfsw" ) ) {
                                campaignList[*campaignCount].typeBits |= (1 << GT_WOLF_STOPWATCH);
                        }
                        if( strstr( token.string, "wolflms" ) ) {
                                campaignList[*campaignCount].typeBits |= (1 << GT_WOLF_LMS);
                        }
                } else if( !Q_stricmp( token.string, "maps" ) ) {
                        if( !PC_String_Parse( handle, &campaignList[*campaignCount].maps ) ) {
                                trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
                                trap_PC_FreeSource( handle );
                                return;
                        }
                }
        }

        trap_PC_FreeSource( handle );
}

static void UI_LinkCampaignsToArenas( void ) {
        int i;

        for( i = 0; i < uiInfo.campaignCount; i++ ) {
                char *ptr, mapname[128], *mapnameptr;

                // find the mapInfo's that match our mapnames
                uiInfo.campaignList[i].mapCount = 0;

                ptr = uiInfo.campaignList[i].maps;
                while( *ptr ) {
                        mapnameptr = mapname;
                        while( *ptr && *ptr != ';' ) {
                                *mapnameptr++ = *ptr++;
                        }
                        if( *ptr )
                                ptr++;
                        *mapnameptr = '\0';
                        for( i = 0; i < uiInfo.mapCount; i++ ) {
                                if( !Q_stricmp( uiInfo.mapList[i].mapLoadName, mapname ) ) {
                                        campaignList[*campaignCount].mapInfos[campaignList[*campaignCount].mapCount++] = &uiInfo.mapList[i];
                                        break;
                                }
                        }
                }
        }
}*/

static void UI_LoadCampaignsFromFile( const char *filename )
{
	int        handle, i;
	pc_token_t token;

	handle = trap_PC_LoadSource( filename );

	if( !handle )
	{
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}

	if( !trap_PC_ReadToken( handle, &token ) )
	{
		trap_PC_FreeSource( handle );
		return;
	}

	if( *token.string != '{' )
	{
		trap_PC_FreeSource( handle );
		return;
	}

	while( trap_PC_ReadToken( handle, &token ) )
	{
		if( *token.string == '}' )
		{
			if( uiInfo.campaignList[ uiInfo.campaignCount ].initial )
			{
				if( uiInfo.campaignList[ uiInfo.campaignCount ].typeBits & ( 1 << GT_SINGLE_PLAYER ) )
				{
					uiInfo.campaignList[ uiInfo.campaignCount ].unlocked = qtrue;
				}

				// Always unlock the initial SP campaign
			}

			uiInfo.campaignList[ uiInfo.campaignCount ].campaignCinematic = -1;
			uiInfo.campaignList[ uiInfo.campaignCount ].campaignShot = -1;

			uiInfo.campaignCount++;

			if( !trap_PC_ReadToken( handle, &token ) )
			{
				// eof
				trap_PC_FreeSource( handle );
				return;
			}

			if( *token.string != '{' )
			{
				//uiInfo.campaignList[uiInfo.campaignCount].order = -1;

				trap_Print( va( S_COLOR_RED "unexpected token '%s' inside: %s\n", token.string, filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
		else if( !Q_stricmp( token.string, "shortname" ) )
		{
			if( !PC_String_Parse( handle, &uiInfo.campaignList[ uiInfo.campaignCount ].campaignShortName ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
		else if( !Q_stricmp( token.string, "name" ) )
		{
			if( !PC_String_Parse( handle, &uiInfo.campaignList[ uiInfo.campaignCount ].campaignName ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
		else if( !Q_stricmp( token.string, "description" ) )
		{
			if( !PC_String_Parse( handle, &uiInfo.campaignList[ uiInfo.campaignCount ].campaignDescription ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
		else if( !Q_stricmp( token.string, "image" ) )
		{
			if( !PC_String_Parse( handle, &uiInfo.campaignList[ uiInfo.campaignCount ].campaignShotName ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
			else
			{
				uiInfo.campaignList[ uiInfo.campaignCount ].campaignShot = -1;
			}
		}
		else if( !Q_stricmp( token.string, "initial" ) )
		{
			uiInfo.campaignList[ uiInfo.campaignCount ].initial = qtrue;
		}
		else if( !Q_stricmp( token.string, "next" ) )
		{
			if( !PC_String_Parse( handle, &uiInfo.campaignList[ uiInfo.campaignCount ].nextCampaignShortName ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}
		}
		else if( !Q_stricmp( token.string, "type" ) )
		{
			if( !trap_PC_ReadToken( handle, &token ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}

			if( strstr( token.string, "wolfsp" ) )
			{
				uiInfo.campaignList[ uiInfo.campaignCount ].typeBits |= ( 1 << GT_SINGLE_PLAYER );
			}

			if( strstr( token.string, "wolfmp" ) )
			{
				uiInfo.campaignList[ uiInfo.campaignCount ].typeBits |= ( 1 << GT_WOLF );
			}

			if( strstr( token.string, "wolfsw" ) )
			{
				uiInfo.campaignList[ uiInfo.campaignCount ].typeBits |= ( 1 << GT_WOLF_STOPWATCH );
			}

			if( strstr( token.string, "wolflms" ) )
			{
				uiInfo.campaignList[ uiInfo.campaignCount ].typeBits |= ( 1 << GT_WOLF_LMS );
			}
		}
		else if( !Q_stricmp( token.string, "maps" ) )
		{
			char *ptr, mapname[ 128 ], *mapnameptr;

			if( !trap_PC_ReadToken( handle, &token ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}

			// find the mapInfo's that match our mapnames
			uiInfo.campaignList[ uiInfo.campaignCount ].mapCount = 0;

			ptr = token.string;

			while( *ptr )
			{
				mapnameptr = mapname;

				while( *ptr && *ptr != ';' )
				{
					*mapnameptr++ = *ptr++;
				}

				if( *ptr )
				{
					ptr++;
				}

				*mapnameptr = '\0';

				for( i = 0; i < uiInfo.mapCount; i++ )
				{
					if( !Q_stricmp( uiInfo.mapList[ i ].mapLoadName, mapname ) )
					{
						uiInfo.campaignList[ uiInfo.campaignCount ].mapInfos[ uiInfo.campaignList[ uiInfo.campaignCount ].mapCount++ ] =
						  &uiInfo.mapList[ i ];
						break;
					}
				}
			}
		}
		else if( !Q_stricmp( token.string, "maptc" ) )
		{
			if( !trap_PC_ReadToken( handle, &token ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}

			uiInfo.campaignList[ uiInfo.campaignCount ].mapTC[ 0 ][ 0 ] = token.floatvalue;

			if( !trap_PC_ReadToken( handle, &token ) )
			{
				trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
				trap_PC_FreeSource( handle );
				return;
			}

			uiInfo.campaignList[ uiInfo.campaignCount ].mapTC[ 0 ][ 1 ] = token.floatvalue;

			uiInfo.campaignList[ uiInfo.campaignCount ].mapTC[ 1 ][ 0 ] = 650 + uiInfo.campaignList[ uiInfo.campaignCount ].mapTC[ 0 ][ 0 ];
			uiInfo.campaignList[ uiInfo.campaignCount ].mapTC[ 1 ][ 1 ] = 650 + uiInfo.campaignList[ uiInfo.campaignCount ].mapTC[ 0 ][ 1 ];

			/*if( !trap_PC_ReadToken( handle, &token ) ) {
			   trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
			   trap_PC_FreeSource( handle );
			   return;
			   }

			   uiInfo.campaignList[uiInfo.campaignCount].mapTC[1][0] = token.floatvalue + uiInfo.campaignList[uiInfo.campaignCount].mapTC[0][0];

			   if( !trap_PC_ReadToken( handle, &token ) ) {
			   trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
			   trap_PC_FreeSource( handle );
			   return;
			   }

			   uiInfo.campaignList[uiInfo.campaignCount].mapTC[1][1] = token.floatvalue + uiInfo.campaignList[uiInfo.campaignCount].mapTC[0][1]; */
		}
	}

	trap_PC_FreeSource( handle );
}

const char     *UI_DescriptionForCampaign( void )
{
	int  i = 0, j = 0;
	char *mapname;
	char info[ MAX_INFO_STRING ];

	trap_GetConfigString( CS_SERVERINFO, info, sizeof( info ) );

	mapname = Info_ValueForKey( info, "mapname" );

	for( ; i < uiInfo.campaignCount; i++ )
	{
		for( ; j < uiInfo.campaignList[ i ].mapCount; j++ )
		{
			if( !Q_stricmp( mapname, uiInfo.campaignList[ i ].mapInfos[ j ]->mapName ) )
			{
				return uiInfo.campaignList[ i ].campaignDescription;
			}
		}
	}

	return NULL;
}

const char     *UI_NameForCampaign( void )
{
	int  i = 0, j = 0;
	char *mapname;
	char info[ MAX_INFO_STRING ];

	trap_GetConfigString( CS_SERVERINFO, info, sizeof( info ) );

	mapname = Info_ValueForKey( info, "mapname" );

	for( ; i < uiInfo.campaignCount; i++ )
	{
		for( ; j < uiInfo.campaignList[ i ].mapCount; j++ )
		{
			if( !Q_stricmp( mapname, uiInfo.campaignList[ i ].mapInfos[ j ]->mapName ) )
			{
				return uiInfo.campaignList[ i ].campaignName;
			}
		}
	}

	return NULL;
}

/*
===============
UI_FindCampaignInCampaignList
===============
*/
int UI_FindCampaignInCampaignList( const char *shortName )
{
	int i;

	if( !shortName )
	{
		return ( -1 );
	}

	for( i = 0; i < uiInfo.campaignCount; i++ )
	{
		if( !Q_stricmp( uiInfo.campaignList[ i ].campaignShortName, shortName ) )
		{
			return ( i );
		}
	}

	return ( -1 );
}

/*
===============
UI_LoadCampaigns
===============
*/
void UI_LoadCampaigns( void )
{
	int  numdirs;
	char filename[ 128 ];
	char dirlist[ 1024 ];
	char *dirptr;
	int  i, j;
	int  dirlen;
	long hash;
	char *ch;

	uiInfo.campaignCount = 0;
	memset( &uiInfo.campaignList, 0, sizeof( uiInfo.campaignList ) );

	// get all campaigns from .campaign files
	numdirs = trap_FS_GetFileList( "scripts", ".campaign", dirlist, 1024 );
	dirptr = dirlist;

	for( i = 0; i < numdirs && uiInfo.campaignCount < MAX_CAMPAIGNS; i++, dirptr += dirlen + 1 )
	{
		dirlen = strlen( dirptr );
		strcpy( filename, "scripts/" );
		strcat( filename, dirptr );
		UI_LoadCampaignsFromFile( filename );
		//UI_LoadCampaignsFromFile( filename, uiInfo.campaignList, &uiInfo.campaignCount, MAX_CAMPAIGNS );
		//UI_LinkCampaignsToArenas();
	}

//  trap_DPrint( va( "%i campaigns parsed\n", ui_numCampaigns ) ); // JPW NERVE pulled per atvi req
	if( UI_OutOfMemory() )
	{
		trap_Print( S_COLOR_YELLOW "WARNING: not anough memory in pool to load all campaigns\n" );
	}

	// Sort the campaigns for single player

	// first, find the initial campaign
	for( i = 0; i < uiInfo.campaignCount; i++ )
	{
		if( !( uiInfo.campaignList[ i ].typeBits & ( 1 << GT_SINGLE_PLAYER ) ) )
		{
			continue;
		}

		if( uiInfo.campaignList[ i ].initial )
		{
			uiInfo.campaignList[ i ].order = 0;
			break;
		}
	}

	// now use the initial nextCampaignShortName to find the next one, etc etc for single player campaigns
	// rain - don't let i go above the maximum number of campaigns
	while( i < MAX_CAMPAIGNS )
	{
		j = UI_FindCampaignInCampaignList( uiInfo.campaignList[ i ].nextCampaignShortName );

		if( j == -1 )
		{
			break;
		}

		uiInfo.campaignList[ j ].order = uiInfo.campaignList[ i ].order + 1;
		i = j;
	}

	// Load the campaign save
	BG_LoadCampaignSave( va( "profiles/%s/campaign.dat", cl_profile.string ), &uiInfo.campaignStatus, cl_profile.string );

	for( i = 0; i < uiInfo.campaignCount; i++ )
	{
		// generate hash for campaign shortname
		for( hash = 0, ch = ( char * ) uiInfo.campaignList[ i ].campaignShortName; *ch != '\0'; ch++ )
		{
			hash += ( long )( tolower( *ch ) ) * ( ( ch - uiInfo.campaignList[ i ].campaignShortName ) + 119 );
		}

		// find the entry in the campaignsave
		for( j = 0; j < uiInfo.campaignStatus.header.numCampaigns; j++ )
		{
			if( hash == uiInfo.campaignStatus.campaigns[ j ].shortnameHash )
			{
				uiInfo.campaignList[ i ].unlocked = qtrue;
				uiInfo.campaignList[ i ].progress = uiInfo.campaignStatus.campaigns[ j ].progress;
				uiInfo.campaignList[ i ].cpsCampaign = &uiInfo.campaignStatus.campaigns[ j ];
			}
		}

		/*if( !uiInfo.campaignStatus.header.numCampaigns ||
		   j == uiInfo.campaignStatus.header.numCampaigns ) {
		   // not found, so not unlocked
		   uiInfo.campaignList[i].unlocked = qfalse;
		   uiInfo.campaignList[i].progress = 0;
		   uiInfo.campaignList[i].cpsCampaign = NULL;
		   } */
	}
}

/*
===============
UI_LoadBotsFromFile
===============
*/
static void UI_LoadBotsFromFile( char *filename )
{
	int          len;
	fileHandle_t f;
	char         buf[ MAX_BOTS_TEXT ];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if( !f )
	{
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}

	if( len >= MAX_BOTS_TEXT )
	{
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_BOTS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[ len ] = 0;
	trap_FS_FCloseFile( f );

	COM_Compress( buf );

	ui_numBots += UI_ParseInfos( buf, MAX_BOTS - ui_numBots, &ui_botInfos[ ui_numBots ], MAX_BOTS );
}

/*
===============
UI_LoadBots
===============
*/
void UI_LoadBots( void )
{
	vmCvar_t botsFile;
	int      numdirs;
	char     filename[ 128 ];
	char     dirlist[ 1024 ];
	char     *dirptr;
	int      i;
	int      dirlen;

	ui_numBots = 0;

	trap_Cvar_Register( &botsFile, "g_botsFile", "", CVAR_INIT | CVAR_ROM );

	if( *botsFile.string )
	{
		UI_LoadBotsFromFile( botsFile.string );
	}
	else
	{
		UI_LoadBotsFromFile( "scripts/bots.txt" );
	}

	// get all bots from .bot files
	numdirs = trap_FS_GetFileList( "scripts", ".bot", dirlist, 1024 );
	dirptr = dirlist;

	for( i = 0; i < numdirs; i++, dirptr += dirlen + 1 )
	{
		dirlen = strlen( dirptr );
		strcpy( filename, "scripts/" );
		strcat( filename, dirptr );
		UI_LoadBotsFromFile( filename );
	}

	trap_Print( va( "%i bots parsed\n", ui_numBots ) );
}

/*
===============
UI_GetBotInfoByNumber
===============
*/
char           *UI_GetBotInfoByNumber( int num )
{
	if( num < 0 || num >= ui_numBots )
	{
		trap_Print( va( S_COLOR_RED "Invalid bot number: %i\n", num ) );
		return NULL;
	}

	return ui_botInfos[ num ];
}

/*
===============
UI_GetBotInfoByName
===============
*/
char           *UI_GetBotInfoByName( const char *name )
{
	int  n;
	char *value;

	for( n = 0; n < ui_numBots; n++ )
	{
		value = Info_ValueForKey( ui_botInfos[ n ], "name" );

		if( !Q_stricmp( value, name ) )
		{
			return ui_botInfos[ n ];
		}
	}

	return NULL;
}

int UI_GetNumBots()
{
	return ui_numBots;
}

char           *UI_GetBotNameByNumber( int num )
{
	char *info = UI_GetBotInfoByNumber( num );

	if( info )
	{
		return Info_ValueForKey( info, "name" );
	}

	return "Sarge";
}
