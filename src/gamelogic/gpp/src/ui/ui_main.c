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

/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/

#include "ui_local.h"

uiInfo_t          uiInfo;

static const char *const netSources[] =
{
	"LAN",
	"Internet",
	"Favorites"
};

static const int  numNetSources = ARRAY_LEN( netSources );

static const char *const netnames[] =
{
	"???",
	"UDP",
	"IPX",
	NULL
};

/*
================
cvars
================
*/

typedef struct
{
	vmCvar_t *vmCvar;
	char     *cvarName;
	char     *defaultString;
	int      cvarFlags;
}

cvarTable_t;

vmCvar_t                   ui_assetScale;

vmCvar_t                   ui_browserShowFull;
vmCvar_t                   ui_browserShowEmpty;

vmCvar_t                   ui_dedicated;
vmCvar_t                   ui_netSource;
vmCvar_t                   ui_selectedMap;
vmCvar_t                   ui_lastServerRefresh_0;
vmCvar_t                   ui_lastServerRefresh_1;
vmCvar_t                   ui_lastServerRefresh_2;
vmCvar_t                   ui_lastServerRefresh_3;
vmCvar_t                   ui_lastServerRefresh_0_time;
vmCvar_t                   ui_lastServerRefresh_1_time;
vmCvar_t                   ui_lastServerRefresh_2_time;
vmCvar_t                   ui_lastServerRefresh_3_time;
vmCvar_t                   ui_smallFont;
vmCvar_t                   ui_bigFont;
vmCvar_t                   ui_findPlayer;
vmCvar_t                   ui_serverStatusTimeOut;
vmCvar_t                   ui_textWrapCache;
vmCvar_t                   ui_developer;

vmCvar_t                   ui_emoticons;
vmCvar_t                   ui_winner;
vmCvar_t                   ui_chatCommands;
vmCvar_t                   ui_chatPromptColours;
vmCvar_t                   cl_profile;
vmCvar_t                   cl_defaultProfile;
vmCvar_t                   ui_profile;

vmCvar_t                   ui_menuFiles;
vmCvar_t                   ui_ingameFiles;
vmCvar_t                   ui_teamFiles;
vmCvar_t                   ui_helpFiles;

static const cvarTable_t   cvarTable[] =
{
	{ &ui_assetScale,          "ui_assetScale",               "1",                         CVAR_ARCHIVE | CVAR_LATCH },

	{ &ui_browserShowFull,     "ui_browserShowFull",          "1",                         CVAR_ARCHIVE              },
	{ &ui_browserShowEmpty,    "ui_browserShowEmpty",         "1",                         CVAR_ARCHIVE              },

	{ &ui_dedicated,           "ui_dedicated",                "0",                         CVAR_ARCHIVE              },
	{ &ui_netSource,           "ui_netSource",                "1",                         CVAR_ARCHIVE              },
	{ &ui_selectedMap,         "ui_selectedMap",              "0",                         CVAR_ARCHIVE              },
	{ &ui_lastServerRefresh_0, "ui_lastServerRefresh_0",      "",                          CVAR_ARCHIVE              },
	{ &ui_lastServerRefresh_1, "ui_lastServerRefresh_1",      "",                          CVAR_ARCHIVE              },
	{ &ui_lastServerRefresh_2, "ui_lastServerRefresh_2",      "",                          CVAR_ARCHIVE              },
	{ &ui_lastServerRefresh_3, "ui_lastServerRefresh_3",      "",                          CVAR_ARCHIVE              },
	{ &ui_lastServerRefresh_0, "ui_lastServerRefresh_0_time", "",                          CVAR_ARCHIVE              },
	{ &ui_lastServerRefresh_1, "ui_lastServerRefresh_1_time", "",                          CVAR_ARCHIVE              },
	{ &ui_lastServerRefresh_2, "ui_lastServerRefresh_2_time", "",                          CVAR_ARCHIVE              },
	{ &ui_lastServerRefresh_3, "ui_lastServerRefresh_3_time", "",                          CVAR_ARCHIVE              },
	{ &ui_smallFont,           "ui_smallFont",                "0.2",                       CVAR_ARCHIVE | CVAR_LATCH },
	{ &ui_bigFont,             "ui_bigFont",                  "0.5",                       CVAR_ARCHIVE | CVAR_LATCH },
	{ &ui_findPlayer,          "ui_findPlayer",               "",                          CVAR_ARCHIVE              },
	{ &ui_serverStatusTimeOut, "ui_serverStatusTimeOut",      "7000",                      CVAR_ARCHIVE              },
	{ &ui_textWrapCache,       "ui_textWrapCache",            "1",                         CVAR_ARCHIVE              },
	{ &ui_developer,           "ui_developer",                "0",                         CVAR_ARCHIVE | CVAR_CHEAT },
	{ &ui_emoticons,           "cg_emoticons",                "1",                         CVAR_LATCH | CVAR_ARCHIVE },
	{ &ui_winner,              "ui_winner",                   "",                          CVAR_ROM                  },
	{ &cl_profile,             "cl_profile",                  "",                          CVAR_ROM                  },
	{ &cl_defaultProfile,      "cl_defaultProfile",           "",                          CVAR_ROM                  },
	{ &ui_profile,             "ui_profile",                  "",                          CVAR_ROM                  },
	{ &ui_chatCommands,        "ui_chatCommands",             "1",                         CVAR_ARCHIVE              },
	{ &ui_chatPromptColours,   "ui_chatPromptColors",         "0",                         CVAR_ARCHIVE              },

	{ &ui_menuFiles,           "ui_menuFiles",                "ui/menu/menus.txt",         CVAR_ARCHIVE              },
	{ &ui_ingameFiles,         "ui_ingameFiles",              "ui/menu/ingame/ingame.txt", CVAR_ARCHIVE              },
	{ &ui_teamFiles,           "ui_teamFiles",                "ui/menu/team/team.txt",     CVAR_ARCHIVE              },
	{ &ui_helpFiles,           "ui_helpFiles",                "ui/menu/help/help.txt",     CVAR_ARCHIVE              }
};

static const int           cvarTableSize = ARRAY_LEN( cvarTable );

static char                translated_yes[ 4 ], translated_no[ 4 ];

extern displayContextDef_t *DC;

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
void     UI_Init( qboolean );
void     UI_Shutdown( void );
void     UI_KeyEvent( int key, int chr, int flags );
void     UI_MouseEvent( int dx, int dy );
int      UI_MousePosition( void );
void     UI_SetMousePosition( int x, int y );
void     UI_Refresh( int realtime );
qboolean UI_IsFullscreen( void );
void     UI_SetActiveMenu( uiMenuCommand_t menu );

Q_EXPORT intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3,
                          int arg4, int arg5, int arg6, int arg7,
                          int arg8, int arg9, int arg10, int arg11 )
{
	switch ( command )
	{
		case UI_GETAPIVERSION:
			return UI_API_VERSION;

		case UI_INIT:
			UI_Init( arg0 );
			return 0;

		case UI_SHUTDOWN:
			UI_Shutdown();
			return 0;

		case UI_KEY_EVENT:
			if ( arg1 & ( 1 << KEYEVSTATE_CHAR ) )
			{
				arg0 &= ~K_CHAR_FLAG;
				arg0 |= ( arg1 & ( 1 << KEYEVSTATE_BIT ) ) ? K_CHAR_FLAG : 0;
				UI_KeyEvent( 0, arg0, arg1 );
			}
			else
			{
				UI_KeyEvent( arg0, 0, arg1 );
			}
			return 0;

		case UI_MOUSE_EVENT:
			UI_MouseEvent( arg0, arg1 );
			return 0;

		case UI_MOUSE_POSITION:
			return UI_MousePosition();

		case UI_SET_MOUSE_POSITION:
			UI_SetMousePosition( arg0, arg1 );
			return 0;

		case UI_REFRESH:
			UI_Refresh( arg0 );
			return 0;

		case UI_IS_FULLSCREEN:
			return UI_IsFullscreen();

		case UI_SET_ACTIVE_MENU:
			UI_SetActiveMenu( arg0 );
			return 0;

		case UI_CONSOLE_COMMAND:
			return UI_ConsoleCommand( arg0 );

		case UI_DRAW_CONNECT_SCREEN:
			UI_DrawConnectScreen( arg0 );
			return 0;

		default:
			trap_Error( va( "vmMain(): unknown ui command %i", command ) );
	}

	return -1;
}

void AssetCache( void )
{
	int i;

	uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	uiInfo.uiDC.Assets.scrollBar = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	uiInfo.uiDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	uiInfo.uiDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	uiInfo.uiDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	uiInfo.uiDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	uiInfo.uiDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	uiInfo.uiDC.Assets.sliderBar = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	uiInfo.uiDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );

	if ( ui_emoticons.integer )
	{
		uiInfo.uiDC.Assets.emoticonCount = BG_LoadEmoticons(
		                                     uiInfo.uiDC.Assets.emoticons, MAX_EMOTICONS );
	}
	else
	{
		uiInfo.uiDC.Assets.emoticonCount = 0;
	}

	for ( i = 0; i < uiInfo.uiDC.Assets.emoticonCount; i++ )
	{
		uiInfo.uiDC.Assets.emoticons[ i ].shader = trap_R_RegisterShaderNoMip(
		      va( "emoticons/%s_%dx1.tga", uiInfo.uiDC.Assets.emoticons[ i ].name,
		          uiInfo.uiDC.Assets.emoticons[ i ].width ) );
	}
}

void UI_DrawSides( float x, float y, float w, float h, float size )
{
	float sizeY;

	UI_AdjustFrom640( &x, &y, &w, &h );
	sizeY = size * uiInfo.uiDC.yscale;
	size *= uiInfo.uiDC.xscale;

	trap_R_DrawStretchPic( x, y + sizeY, size, h - ( sizeY * 2.0f ), 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y + sizeY, size, h - ( sizeY * 2.0f ), 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void UI_DrawTopBottom( float x, float y, float w, float h, float size )
{
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= uiInfo.uiDC.yscale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRect( float x, float y, float width, float height, float size, const float *color )
{
	trap_R_SetColor( color );

	UI_DrawTopBottom( x, y, width, height, size );
	UI_DrawSides( x, y, width, height, size );

	trap_R_SetColor( NULL );
}

/*
==================
UI_ServerInfoIsValid

Return false if the infostring contains nonprinting characters,
 or if the hostname is blank/undefined
==================
*/
static qboolean UI_ServerInfoIsValid( char *info )
{
	char *c;
	int  len = 0;

	for ( c = info; *c; c++ )
	{
		if ( !isprint( *c ) )
		{
			return qfalse;
		}
	}

	for ( c = Info_ValueForKey( info, "hostname" ); *c; c++ )
	{
		if ( isgraph( *c ) )
		{
			len++;
		}
	}

	if ( len )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*
==================
UI_InsertServerIntoDisplayList
==================
*/
static void UI_InsertServerIntoDisplayList( int num, int position )
{
	int         i;
	static char info[ MAX_STRING_CHARS ];

	if ( position < 0 || position > uiInfo.serverStatus.numDisplayServers )
	{
		return;
	}

	trap_LAN_GetServerInfo( ui_netSource.integer, num, info, MAX_STRING_CHARS );

	if ( !UI_ServerInfoIsValid( info ) )  // don't list servers with invalid info
	{
		return;
	}

	uiInfo.serverStatus.numDisplayServers++;

	for ( i = uiInfo.serverStatus.numDisplayServers; i > position; i-- )
	{
		uiInfo.serverStatus.displayServers[ i ] = uiInfo.serverStatus.displayServers[ i - 1 ];
	}

	uiInfo.serverStatus.displayServers[ position ] = num;
}

/*
==================
UI_RemoveServerFromDisplayList
==================
*/
static void UI_RemoveServerFromDisplayList( int num )
{
	int         i, j;
	static char info[ MAX_STRING_CHARS ];

	for ( i = 0; i < uiInfo.serverStatus.numDisplayServers; i++ )
	{
		if ( uiInfo.serverStatus.displayServers[ i ] == num )
		{
			uiInfo.serverStatus.numDisplayServers--;

			trap_LAN_GetServerInfo( ui_netSource.integer, num, info, MAX_STRING_CHARS );

			for ( j = i; j < uiInfo.serverStatus.numDisplayServers; j++ )
			{
				uiInfo.serverStatus.displayServers[ j ] = uiInfo.serverStatus.displayServers[ j + 1 ];
			}

			return;
		}
	}
}

/*
==================
UI_BinaryServerInsertion
==================
*/
static void UI_BinaryServerInsertion( int num )
{
	int mid, offset, res, len;

	// use binary search to insert server
	len = uiInfo.serverStatus.numDisplayServers;
	mid = len;
	offset = 0;
	res = 0;

	while ( mid > 0 )
	{
		mid = len >> 1;
		//
		res = trap_LAN_CompareServers( ui_netSource.integer, uiInfo.serverStatus.sortKey,
		                               uiInfo.serverStatus.sortDir, num, uiInfo.serverStatus.displayServers[ offset + mid ] );
		// if equal

		if ( res == 0 )
		{
			UI_InsertServerIntoDisplayList( num, offset + mid );
			return;
		}

		// if larger
		else if ( res == 1 )
		{
			offset += mid;
			len -= mid;
		}

		// if smaller
		else
		{
			len -= mid;
		}
	}

	if ( res == 1 )
	{
		offset++;
	}

	UI_InsertServerIntoDisplayList( num, offset );
}

typedef struct
{
	char *name, *altName;
}

serverStatusCvar_t;

static const serverStatusCvar_t serverStatusCvars[] =
{
	{ "sv_hostname", "Name"      },
	{ "Address",     ""          },
	{ "gamename",    "Game name" },
	{ "mapname",     "Map"       },
	{ "version",     ""          },
	{ "protocol",    ""          },
	{ "timelimit",   ""          },
	{ NULL,          NULL        }
};

/*
==================
UI_SortServerStatusInfo
==================
*/

static int UI_SortServerStatusCompare( const void *a, const void *b )
{
	const char **la = ( const char ** ) a;
	const char **lb = ( const char ** ) b;

	return strcmp( la[ 0 ], lb[ 0 ] );
}

static void UI_SortServerStatusInfo( serverStatusInfo_t *info )
{
	int  i, j, index;
	char *tmp1, *tmp2;

	index = 0;

	for ( i = 0; serverStatusCvars[ i ].name; i++ )
	{
		for ( j = 0; j < info->numLines; j++ )
		{
			if ( !info->lines[ j ][ 1 ] || info->lines[ j ][ 1 ][ 0 ] )
			{
				continue;
			}

			if ( !Q_stricmp( serverStatusCvars[ i ].name, info->lines[ j ][ 0 ] ) )
			{
				// swap lines
				tmp1 = info->lines[ index ][ 0 ];
				tmp2 = info->lines[ index ][ 3 ];
				info->lines[ index ][ 0 ] = info->lines[ j ][ 0 ];
				info->lines[ index ][ 3 ] = info->lines[ j ][ 3 ];
				info->lines[ j ][ 0 ] = tmp1;
				info->lines[ j ][ 3 ] = tmp2;
				//

				if ( strlen( serverStatusCvars[ i ].altName ) )
				{
					info->lines[ index ][ 0 ] = serverStatusCvars[ i ].altName;
				}

				index++;
			}
		}
	}

	// sort remaining cvars
	qsort( info->lines + index,
	       info->numLines - index, sizeof( info->lines[ 0 ] ),
	       UI_SortServerStatusCompare );
}

/*
==================
UI_GetServerStatusInfo
==================
*/
static int UI_GetServerStatusInfo( const char *serverAddress, serverStatusInfo_t *info )
{
	char *p, *score, *ping, *name;
	int  i, len;

	if ( !info )
	{
		trap_LAN_ServerStatus( serverAddress, NULL, 0 );
		return qfalse;
	}

	memset( info, 0, sizeof( *info ) );

	if ( trap_LAN_ServerStatus( serverAddress, info->text, sizeof( info->text ) ) )
	{
		Q_strncpyz( info->address, serverAddress, sizeof( info->address ) );
		p = info->text;
		info->numLines = 0;
		info->lines[ info->numLines ][ 0 ] = "Address";
		info->lines[ info->numLines ][ 1 ] = "";
		info->lines[ info->numLines ][ 2 ] = "";
		info->lines[ info->numLines ][ 3 ] = info->address;
		info->numLines++;
		// get the cvars

		while ( p && *p )
		{
			p = strchr( p, '\\' );

			if ( !p ) { break; }

			*p++ = '\0';

			if ( *p == '\\' )
			{
				break;
			}

			info->lines[ info->numLines ][ 0 ] = p;
			info->lines[ info->numLines ][ 1 ] = "";
			info->lines[ info->numLines ][ 2 ] = "";

			p = strchr( p, '\\' );

			if ( !p ) { break; }

			*p++ = '\0';

			info->lines[ info->numLines ][ 3 ] = p;
			info->numLines++;

			if ( info->numLines >= MAX_SERVERSTATUS_LINES )
			{
				break;
			}
		}

		UI_SortServerStatusInfo( info );

		// get the player list
		if ( info->numLines < MAX_SERVERSTATUS_LINES - 3 )
		{
			// empty line
			info->lines[ info->numLines ][ 0 ] = "";
			info->lines[ info->numLines ][ 1 ] = "";
			info->lines[ info->numLines ][ 2 ] = "";
			info->lines[ info->numLines ][ 3 ] = "";
			info->numLines++;
			// header
			info->lines[ info->numLines ][ 0 ] = "num";
			info->lines[ info->numLines ][ 1 ] = "score";
			info->lines[ info->numLines ][ 2 ] = "ping";
			info->lines[ info->numLines ][ 3 ] = "name";
			info->numLines++;
			// parse players
			i = 0;
			len = 0;

			while ( p && *p )
			{
				if ( *p == '\\' )
				{
					*p++ = '\0';
				}

				if ( !p )
				{
					break;
				}

				score = p;

				p = strchr( p, ' ' );

				if ( !p )
				{
					break;
				}

				*p++ = '\0';

				ping = p;

				p = strchr( p, ' ' );

				if ( !p )
				{
					break;
				}

				*p++ = '\0';

				name = p;

				Com_sprintf( &info->pings[ len ], sizeof( info->pings ) - len, "%d", i );

				info->lines[ info->numLines ][ 0 ] = &info->pings[ len ];

				len += strlen( &info->pings[ len ] ) + 1;

				info->lines[ info->numLines ][ 1 ] = score;
				info->lines[ info->numLines ][ 2 ] = ping;
				info->lines[ info->numLines ][ 3 ] = name;
				info->numLines++;

				if ( info->numLines >= MAX_SERVERSTATUS_LINES )
				{
					break;
				}

				p = strchr( p, '\\' );

				if ( !p )
				{
					break;
				}

				*p++ = '\0';

				//
				i++;
			}
		}

		return qtrue;
	}

	return qfalse;
}

/*
==================
stristr
==================
*/
static char *stristr( char *str, char *charset )
{
	int i;

	while ( *str )
	{
		for ( i = 0; charset[ i ] && str[ i ]; i++ )
		{
			if ( toupper( charset[ i ] ) != toupper( str[ i ] ) ) { break; }
		}

		if ( !charset[ i ] ) { return str; }

		str++;
	}

	return NULL;
}

/*
==================
UI_BuildFindPlayerList
==================
*/
static void UI_FeederSelection( int feederID, int index );

static void UI_BuildFindPlayerList( qboolean force )
{
	static int         numFound, numTimeOuts;
	int                i, j, k, resend;
	serverStatusInfo_t info;
	char               name[ MAX_NAME_LENGTH + 2 ];
	char               infoString[ MAX_STRING_CHARS ];
	qboolean           duplicate;

	if ( !force )
	{
		if ( !uiInfo.nextFindPlayerRefresh || uiInfo.nextFindPlayerRefresh > uiInfo.uiDC.realTime )
		{
			return;
		}
	}
	else
	{
		memset( &uiInfo.pendingServerStatus, 0, sizeof( uiInfo.pendingServerStatus ) );
		uiInfo.numFoundPlayerServers = 0;
		uiInfo.currentFoundPlayerServer = 0;
		trap_Cvar_VariableStringBuffer( "ui_findPlayer", uiInfo.findPlayerName, sizeof( uiInfo.findPlayerName ) );
		Q_CleanStr( uiInfo.findPlayerName );
		// should have a string of some length

		if ( !strlen( uiInfo.findPlayerName ) )
		{
			uiInfo.nextFindPlayerRefresh = 0;
			return;
		}

		// set resend time
		resend = ui_serverStatusTimeOut.integer / 2 - 10;

		if ( resend < 50 )
		{
			resend = 50;
		}

		trap_Cvar_Set( "cl_serverStatusResendTime", va( "%d", resend ) );
		// reset all server status requests
		trap_LAN_ServerStatus( NULL, NULL, 0 );
		//
		uiInfo.numFoundPlayerServers = 1;
		Com_sprintf( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
		             sizeof( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ] ),
		             "searching %d…", uiInfo.pendingServerStatus.num );
		numFound = 0;
		numTimeOuts++;
	}

	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ )
	{
		// if this pending server is valid

		if ( uiInfo.pendingServerStatus.server[ i ].valid )
		{
			// try to get the server status for this server

			if ( UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[ i ].adrstr, &info ) )
			{
				//
				numFound++;
				// parse through the server status lines

				for ( j = 0; j < info.numLines; j++ )
				{
					// should have ping info

					if ( !info.lines[ j ][ 2 ] || !info.lines[ j ][ 2 ][ 0 ] )
					{
						continue;
					}

					// clean string first
					Q_strncpyz( name, info.lines[ j ][ 3 ], sizeof( name ) );

					Q_CleanStr( name );

					duplicate = qfalse;

					for ( k = 0; k < uiInfo.numFoundPlayerServers - 1; k++ )
					{
						if ( Q_strncmp( uiInfo.foundPlayerServerAddresses[ k ],
						                uiInfo.pendingServerStatus.server[ i ].adrstr,
						                MAX_ADDRESSLENGTH ) == 0 )
						{
							duplicate = qtrue;
						}
					}

					// if the player name is a substring
					if ( stristr( name, uiInfo.findPlayerName ) && !duplicate )
					{
						// add to found server list if we have space (always leave space for a line with the number found)

						if ( uiInfo.numFoundPlayerServers < MAX_FOUNDPLAYER_SERVERS - 1 )
						{
							//
							Q_strncpyz( uiInfo.foundPlayerServerAddresses[ uiInfo.numFoundPlayerServers - 1 ],
							            uiInfo.pendingServerStatus.server[ i ].adrstr,
							            sizeof( uiInfo.foundPlayerServerAddresses[ 0 ] ) );
							Q_strncpyz( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
							            uiInfo.pendingServerStatus.server[ i ].name,
							            sizeof( uiInfo.foundPlayerServerNames[ 0 ] ) );
							uiInfo.numFoundPlayerServers++;
						}
						else
						{
							// can't add any more so we're done
							uiInfo.pendingServerStatus.num = uiInfo.serverStatus.numDisplayServers;
						}
					}
				}

				Com_sprintf( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
				             sizeof( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ] ),
				             "searching %d/%d…", numFound, uiInfo.pendingServerStatus.num );
				// retrieved the server status so reuse this spot
				uiInfo.pendingServerStatus.server[ i ].valid = qfalse;
			}
		}

		// if empty pending slot or timed out
		if ( !uiInfo.pendingServerStatus.server[ i ].valid ||
		     uiInfo.pendingServerStatus.server[ i ].startTime < uiInfo.uiDC.realTime - ui_serverStatusTimeOut.integer )
		{
			if ( uiInfo.pendingServerStatus.server[ i ].valid )
			{
				numTimeOuts++;
			}

			// reset server status request for this address
			UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[ i ].adrstr, NULL );

			// reuse pending slot
			uiInfo.pendingServerStatus.server[ i ].valid = qfalse;

			// if we didn't try to get the status of all servers in the main browser yet
			if ( uiInfo.pendingServerStatus.num < uiInfo.serverStatus.numDisplayServers )
			{
				uiInfo.pendingServerStatus.server[ i ].startTime = uiInfo.uiDC.realTime;
				trap_LAN_GetServerAddressString( ui_netSource.integer,
				                                 uiInfo.serverStatus.displayServers[ uiInfo.pendingServerStatus.num ],
				                                 uiInfo.pendingServerStatus.server[ i ].adrstr,
				                                 sizeof( uiInfo.pendingServerStatus.server[ i ].adrstr ) );

				trap_LAN_GetServerInfo( ui_netSource.integer,
				                        uiInfo.serverStatus.displayServers[ uiInfo.pendingServerStatus.num ],
				                        infoString, sizeof( infoString ) );

				Q_strncpyz( uiInfo.pendingServerStatus.server[ i ].name,
				            Info_ValueForKey( infoString, "hostname" ),
				            sizeof( uiInfo.pendingServerStatus.server[ 0 ].name ) );

				uiInfo.pendingServerStatus.server[ i ].valid = qtrue;
				uiInfo.pendingServerStatus.num++;
				Com_sprintf( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
				             sizeof( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ] ),
				             "searching %d/%d…", numFound, uiInfo.pendingServerStatus.num );
			}
		}
	}

	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ )
	{
		if ( uiInfo.pendingServerStatus.server[ i ].valid )
		{
			break;
		}
	}

	// if still trying to retrieve server status info
	if ( i < MAX_SERVERSTATUSREQUESTS )
	{
		uiInfo.nextFindPlayerRefresh = uiInfo.uiDC.realTime + 25;
	}
	else
	{
		// add a line that shows the number of servers found

		if ( !uiInfo.numFoundPlayerServers )
		{
			Com_sprintf( uiInfo.foundPlayerServerNames[ uiInfo.numFoundPlayerServers - 1 ],
			             sizeof( uiInfo.foundPlayerServerAddresses[ 0 ] ), "no servers found" );
		}
		else
		{
		        int count = uiInfo.numFoundPlayerServers - 1;
			Com_sprintf( uiInfo.foundPlayerServerNames[ count ],
			             sizeof( uiInfo.foundPlayerServerAddresses[ 0 ] ),
			             N_( "%d server found with player %s",
			                 "%d servers found with player %s", count ),
                                     count, uiInfo.findPlayerName );
		}

		uiInfo.nextFindPlayerRefresh = 0;
		// show the server status info for the selected server
		UI_FeederSelection( FEEDER_FINDPLAYER, uiInfo.currentFoundPlayerServer );
	}
}

/*
==================
UI_BuildServerStatus
==================
*/
static void UI_BuildServerStatus( qboolean force )
{
	if ( uiInfo.nextFindPlayerRefresh )
	{
		return;
	}

	if ( !force )
	{
		if ( !uiInfo.nextServerStatusRefresh || uiInfo.nextServerStatusRefresh > uiInfo.uiDC.realTime )
		{
			return;
		}
	}
	else
	{
		Menu_SetFeederSelection( NULL, FEEDER_SERVERSTATUS, 0, NULL );
		uiInfo.serverStatusInfo.numLines = 0;
		// reset all server status requests
		trap_LAN_ServerStatus( NULL, NULL, 0 );
	}

	if ( uiInfo.serverStatus.currentServer < 0 ||
	     uiInfo.serverStatus.currentServer > uiInfo.serverStatus.numDisplayServers ||
	     uiInfo.serverStatus.numDisplayServers == 0 )
	{
		return;
	}

	if ( UI_GetServerStatusInfo( uiInfo.serverStatusAddress, &uiInfo.serverStatusInfo ) )
	{
		uiInfo.nextServerStatusRefresh = 0;
		UI_GetServerStatusInfo( uiInfo.serverStatusAddress, NULL );
	}
	else
	{
		uiInfo.nextServerStatusRefresh = uiInfo.uiDC.realTime + 500;
	}
}

/*
==================
UI_BuildServerDisplayList
==================
*/
static void UI_BuildServerDisplayList( qboolean force )
{
	int        i, count, clients, maxClients, ping, len, visible;
	char       info[ MAX_STRING_CHARS ];
	static int numinvisible;

	if ( !( force || uiInfo.uiDC.realTime > uiInfo.serverStatus.nextDisplayRefresh ) )
	{
		return;
	}

	// if we shouldn't reset
	if ( force == 2 )
	{
		force = 0;
	}

	// do motd updates here too
	trap_Cvar_VariableStringBuffer( "cl_motdString", uiInfo.serverStatus.motd, sizeof( uiInfo.serverStatus.motd ) );

	len = strlen( uiInfo.serverStatus.motd );

	if ( len != uiInfo.serverStatus.motdLen )
	{
		uiInfo.serverStatus.motdLen = len;
		uiInfo.serverStatus.motdWidth = -1;
	}

	if ( force )
	{
		numinvisible = 0;
		// clear number of displayed servers
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		// set list box index to zero
		Menu_SetFeederSelection( NULL, FEEDER_SERVERS, 0, NULL );
		// mark all servers as visible so we store ping updates for them
		trap_LAN_MarkServerVisible( ui_netSource.integer, -1, qtrue );
	}

	// get the server count (comes from the master)
	count = trap_LAN_GetServerCount( ui_netSource.integer );

	if ( count == -1 || ( ui_netSource.integer == AS_LOCAL && count == 0 ) )
	{
		// still waiting on a response from the master
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 500;
		return;
	}

	visible = qfalse;

	for ( i = 0; i < count; i++ )
	{
		// if we already got info for this server

		if ( !trap_LAN_ServerIsVisible( ui_netSource.integer, i ) )
		{
			continue;
		}

		visible = qtrue;
		// get the ping for this server
		ping = trap_LAN_GetServerPing( ui_netSource.integer, i );

		if ( ping > 0 || ui_netSource.integer == AS_FAVORITES )
		{
			trap_LAN_GetServerInfo( ui_netSource.integer, i, info, MAX_STRING_CHARS );

			clients = atoi( Info_ValueForKey( info, "clients" ) );
			uiInfo.serverStatus.numPlayersOnServers += clients;

			if ( ui_browserShowEmpty.integer == 0 )
			{
				if ( clients == 0 )
				{
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			if ( ui_browserShowFull.integer == 0 )
			{
				maxClients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );

				if ( clients == maxClients )
				{
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			// make sure we never add a favorite server twice
			if ( ui_netSource.integer == AS_FAVORITES )
			{
				UI_RemoveServerFromDisplayList( i );
			}

			// insert the server into the list
			UI_BinaryServerInsertion( i );

			// done with this server
			if ( ping > 0 )
			{
				trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
				numinvisible++;
			}
		}
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime;

	// if there were no servers visible for ping updates

	if ( !visible )
	{
//    UI_StopServerRefresh();
//    uiInfo.serverStatus.nextDisplayRefresh = 0;
	}
}

/*
=================
UI_StopServerRefresh
=================
*/
static void UI_StopServerRefresh( void )
{
	int count;

	if ( !uiInfo.serverStatus.refreshActive )
	{
		// not currently refreshing
		return;
	}

	uiInfo.serverStatus.refreshActive = qfalse;
	Com_Printf(_( "%d servers listed in browser with %d players.\n"),
	            uiInfo.serverStatus.numDisplayServers,
	            uiInfo.serverStatus.numPlayersOnServers );
	count = trap_LAN_GetServerCount( ui_netSource.integer );

	if ( count - uiInfo.serverStatus.numDisplayServers > 0 )
	{
		Com_Printf(_( "%d servers not listed due to packet loss, invalid info,"
		            " or pings higher than %d\n"),
		            count - uiInfo.serverStatus.numDisplayServers,
		            ( int ) trap_Cvar_VariableValue( "cl_maxPing" ) );
	}
}

/*
=================
UI_DoServerRefresh
=================
*/
static void UI_DoServerRefresh( void )
{
	qboolean wait = qfalse;

	if ( !uiInfo.serverStatus.refreshActive )
	{
		return;
	}

	if ( ui_netSource.integer != AS_FAVORITES )
	{
		if ( ui_netSource.integer == AS_LOCAL )
		{
			if ( !trap_LAN_GetServerCount( ui_netSource.integer ) )
			{
				wait = qtrue;
			}
		}
		else
		{
			if ( trap_LAN_GetServerCount( ui_netSource.integer ) < 0 )
			{
				wait = qtrue;
			}
		}
	}

	if ( uiInfo.uiDC.realTime < uiInfo.serverStatus.refreshtime )
	{
		if ( wait )
		{
			return;
		}
	}

	// if still trying to retrieve pings
	if ( trap_LAN_UpdateVisiblePings( ui_netSource.integer ) )
	{
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
	}
	else if ( !wait )
	{
		// get the last servers in the list
		UI_BuildServerDisplayList( 2 );
		// stop the refresh
		UI_StopServerRefresh();
	}

	//
	UI_BuildServerDisplayList( qfalse );
}

/*
=================
UI_UpdatePendingPings
=================
*/
static void UI_UpdatePendingPings( void )
{
	trap_LAN_ResetPings( ui_netSource.integer );
	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
}

/*
=================
UI_StartServerRefresh
=================
*/
static void UI_StartServerRefresh( qboolean full )
{
	int     time;
	qtime_t q;

	time = trap_RealTime( &q );
	trap_Cvar_Set( va( "ui_lastServerRefresh_%i_time", ui_netSource.integer ),
	               va( "%i", time ) );
	trap_Cvar_Set( va( "ui_lastServerRefresh_%i", ui_netSource.integer ),
	               va( "%04i-%02i-%02i %02i:%02i:%02i",
	                   1900 + q.tm_year, q.tm_mon + 1, q.tm_mday,
	                   q.tm_hour, q.tm_min, q.tm_sec ) );

	if ( !full )
	{
		UI_UpdatePendingPings();
		return;
	}

	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 1000;
	// clear number of displayed servers
	uiInfo.serverStatus.numDisplayServers = 0;
	uiInfo.serverStatus.numPlayersOnServers = 0;
	// mark all servers as visible so we store ping updates for them
	trap_LAN_MarkServerVisible( ui_netSource.integer, -1, qtrue );
	// reset all the pings
	trap_LAN_ResetPings( ui_netSource.integer );
	//

	if ( ui_netSource.integer == AS_LOCAL )
	{
		trap_Cmd_ExecuteText( EXEC_APPEND, "localservers\n" );
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
		return;
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 5000;

	if ( ui_netSource.integer == AS_GLOBAL )
	{
		qboolean global = ui_netSource.integer == AS_GLOBAL;

		trap_Cmd_ExecuteText( EXEC_APPEND, va( "globalservers %d %d full empty\n",
		                                       global ? 0 : 1,
		                                       (int) trap_Cvar_VariableValue( "protocol" ) ) );
	}
}

int frameCount = 0;
int startTime;

#define UI_FPS_FRAMES 4
void UI_Refresh( int realtime )
{
	static int index;
	static int previousTimes[ UI_FPS_FRAMES ];

	//if( !( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
	//  return;
	//}

	uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realtime;

	previousTimes[ index % UI_FPS_FRAMES ] = uiInfo.uiDC.frameTime;
	index++;

	if ( index > UI_FPS_FRAMES )
	{
		int i, total;
		// average multiple frames together to smooth changes out a bit
		total = 0;

		for ( i = 0; i < UI_FPS_FRAMES; i++ )
		{
			total += previousTimes[ i ];
		}

		if ( !total )
		{
			total = 1;
		}

		uiInfo.uiDC.FPS = 1000 * UI_FPS_FRAMES / total;
	}

	UI_UpdateCvars();

	if ( Menu_Count() > 0 )
	{
		Menu_UpdateAll();
		Menu_PaintAll();
		UI_DoServerRefresh();
		UI_BuildServerStatus( qfalse );
		UI_BuildFindPlayerList( qfalse );
		UI_UpdateNews( qfalse );
	}

	// draw cursor
	UI_SetColor( NULL );

	if ( trap_Key_GetCatcher() == KEYCATCH_UI && !trap_Cvar_VariableValue( "ui_hideCursor" ) )
	{
		UI_DrawHandlePic( uiInfo.uiDC.cursorx - ( 16.0f * uiInfo.uiDC.aspectScale ), uiInfo.uiDC.cursory - 16.0f,
		                  32.0f * uiInfo.uiDC.aspectScale, 32.0f, uiInfo.uiDC.Assets.cursor );
	}
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown( void )
{
	trap_LAN_SaveCachedServers();

	UI_R_UnregisterFont( uiInfo.uiDC.Assets.textFont.handle );
	UI_R_UnregisterFont( uiInfo.uiDC.Assets.smallFont.handle );
	UI_R_UnregisterFont( uiInfo.uiDC.Assets.bigFont.handle );

	UIS_Shutdown( );
}

qboolean Asset_Parse( int handle )
{
	pc_token_t token;
	const char *tempStr;
	const char *fallbackFont = "fonts/unifont.ttf";

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return qfalse;
	}

	if ( Q_stricmp( token.string, "{" ) != 0 )
	{
		return qfalse;
	}

	while ( 1 )
	{
		memset( &token, 0, sizeof( pc_token_t ) );

		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			return qfalse;
		}

		if ( Q_stricmp( token.string, "}" ) == 0 )
		{
			return qtrue;
		}

		// font
		if ( Q_stricmp( token.string, "fallbackfont" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &fallbackFont ) )
			{
				return qfalse;
			}
			continue;
		}

		// font
		if ( Q_stricmp( token.string, "font" ) == 0 )
		{
			int pointSize;

			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
			{
				return qfalse;
			}

			trap_R_RegisterFont( tempStr, fallbackFont, pointSize, &uiInfo.uiDC.Assets.textFont );
			uiInfo.uiDC.Assets.fontRegistered = qtrue;
			continue;
		}

		if ( Q_stricmp( token.string, "smallFont" ) == 0 )
		{
			int pointSize;

			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
			{
				return qfalse;
			}

			trap_R_RegisterFont( tempStr, fallbackFont, pointSize, &uiInfo.uiDC.Assets.smallFont );
			continue;
		}

		if ( Q_stricmp( token.string, "bigFont" ) == 0 )
		{
			int pointSize;

			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
			{
				return qfalse;
			}

			trap_R_RegisterFont( tempStr, fallbackFont, pointSize, &uiInfo.uiDC.Assets.bigFont );
			continue;
		}

		// gradientbar
		if ( Q_stricmp( token.string, "gradientbar" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( tempStr );
			continue;
		}

		// enterMenuSound
		if ( Q_stricmp( token.string, "menuEnterSound" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// exitMenuSound
		if ( Q_stricmp( token.string, "menuExitSound" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// itemFocusSound
		if ( Q_stricmp( token.string, "itemFocusSound" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// menuBuzzSound
		if ( Q_stricmp( token.string, "menuBuzzSound" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if ( Q_stricmp( token.string, "cursor" ) == 0 )
		{
			if ( !PC_String_Parse( handle, &uiInfo.uiDC.Assets.cursorStr ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.cursor = trap_R_RegisterShaderNoMip( uiInfo.uiDC.Assets.cursorStr );
			continue;
		}

		if ( Q_stricmp( token.string, "fadeClamp" ) == 0 )
		{
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.fadeClamp ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "fadeCycle" ) == 0 )
		{
			if ( !PC_Int_Parse( handle, &uiInfo.uiDC.Assets.fadeCycle ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "fadeAmount" ) == 0 )
		{
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.fadeAmount ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "shadowX" ) == 0 )
		{
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.shadowX ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "shadowY" ) == 0 )
		{
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.shadowY ) )
			{
				return qfalse;
			}

			continue;
		}

		if ( Q_stricmp( token.string, "shadowColor" ) == 0 )
		{
			if ( !PC_Color_Parse( handle, &uiInfo.uiDC.Assets.shadowColor ) )
			{
				return qfalse;
			}

			uiInfo.uiDC.Assets.shadowFadeClamp = uiInfo.uiDC.Assets.shadowColor[ 3 ];
			continue;
		}
	}

	return qfalse;
}

void UI_Report( void )
{
	String_Report();
}

qboolean UI_ParseMenu( const char *menuFile )
{
	int        handle;
	pc_token_t token;

	handle = trap_Parse_LoadSource( menuFile );

	if ( !handle )
	{
		Com_Printf( _( S_COLOR_YELLOW  "WARNING: Menu file %s not found\n"),
		            menuFile );
		return qfalse;
	}

	while ( 1 )
	{
		memset( &token, 0, sizeof( pc_token_t ) );

		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			break;
		}

		//if( Q_stricmp( token, "{" ) ) {
		//  Com_Printf("%s", _( "Missing { in menu file\n" ));
		//  break;
		//}

		//if( menuCount == MAX_MENUS ) {
		//  Com_Printf("%s", _( "Too many menus!\n" ));
		//  break;
		//}

		if ( token.string[ 0 ] == '}' )
		{
			break;
		}

		if ( Q_stricmp( token.string, "assetGlobalDef" ) == 0 )
		{
			if ( Asset_Parse( handle ) )
			{
				continue;
			}
			else
			{
				break;
			}
		}

		if ( Q_stricmp( token.string, "menudef" ) == 0 )
		{
			// start a new menu
			Menu_New( handle );
		}
	}

	trap_Parse_FreeSource( handle );
	return qtrue;
}

qboolean Load_Menu( int handle )
{
	pc_token_t token;
#ifdef LOCALIZATION_SUPPORT
	int        cl_lang;
#endif

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return qfalse;
	}

	if ( token.string[ 0 ] != '{' )
	{
		return qfalse;
	}

	while ( 1 )
	{
		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			return qfalse;
		}

		if ( token.string[ 0 ] == 0 )
		{
			return qfalse;
		}

		if ( token.string[ 0 ] == '}' )
		{
			return qtrue;
		}

#ifdef LOCALIZATION_SUPPORT
		cl_lang = atoi( UI_Cvar_VariableString( "cl_language" ) );

		if ( cl_lang )
		{
			const char *s = NULL;
			const char *fname;
			char       out[ 256 ];

			COM_StripFilename( token.string, out );

			fname = COM_SkipPath( token.string );

			// NOTE : cl_language 0 - English

			if ( cl_lang == 1 )
			{
				s = va( "%s%s", out, "french/" );
			}
			else if ( cl_lang == 2 )
			{
				s = va( "%s%s", out, "german/" );
			}
			else if ( cl_lang == 3 )
			{
				s = va( "%s%s", out, "italian/" );
			}
			else if ( cl_lang == 4 )
			{
				s = va( "%s%s", out, "spanish/" );
			}
			else if ( cl_lang == 5 )
			{
				s = va( "%s%s", out, "japanese/" );
			}

			if ( UI_ParseMenu( va( "%s%s", s, fname ) ) )
			{
				continue;
			}
		}

#endif // LOCALIZATION_SUPPORT

		UI_ParseMenu( token.string );
	}

	return qfalse;
}

void UI_LoadMenus( const char *menuFile, qboolean reset )
{
	pc_token_t token;
	int        handle;
	int        start;
	char       assetScale[ 20 ];

	start = trap_Milliseconds();

	handle = trap_Parse_LoadSource( menuFile );

	if ( !handle )
	{
		trap_Error( va( S_COLOR_RED "menu list '%s' not found, unable to continue!", menuFile ) );
	}

	if ( reset )
	{
		Menu_Reset();
	}

	trap_Cvar_VariableStringBuffer( "ui_assetScale", assetScale, sizeof( assetScale ) );
	trap_Parse_AddGlobalDefine( va( "ASSET_SCALE %f", assetScale[ 0 ] ? atof( assetScale ) : 1.0f ) );

	while ( 1 )
	{
		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			break;
		}

		if ( token.string[ 0 ] == 0 || token.string[ 0 ] == '}' )
		{
			break;
		}

		if ( token.string[ 0 ] == '}' )
		{
			break;
		}

		if ( Q_stricmp( token.string, "loadmenu" ) == 0 )
		{
			if ( Load_Menu( handle ) )
			{
				continue;
			}
			else
			{
				break;
			}
		}
	}

	Com_Printf(_( "UI menu file '%s' loaded in %d msec\n"), menuFile, trap_Milliseconds() - start );

	trap_Parse_FreeSource( handle );
}

void UI_LoadHelp( const char *helpFile )
{
	pc_token_t token;
	int        handle, start;
	char       title[ 32 ], buffer[ 1024 ];

	start = trap_Milliseconds();

	handle = trap_Parse_LoadSource( helpFile );

	if ( !handle )
	{
		Com_Printf( _( S_COLOR_YELLOW  "WARNING: help file '%s' not found!\n"),
		            helpFile );
		return;
	}

	if ( !trap_Parse_ReadToken( handle, &token ) ||
	     token.string[ 0 ] == 0 || token.string[ 0 ] != '{' )
	{
		Com_Printf( _( S_COLOR_YELLOW  "WARNING: help file '%s' does not start with "
		            "'{'\n"), helpFile );
		return;
	}

	uiInfo.helpCount = 0;
	title[ 0 ] = 0;

	while ( 1 )
	{
		if ( !trap_Parse_ReadToken( handle, &token ) ||
		     token.string[ 0 ] == 0 || token.string[ 0 ] == '}' )
		{
			break;
		}

		if ( token.string[ 0 ] == '{' )
		{
			buffer[ 0 ] = 0;
			Q_strcat( buffer, sizeof( buffer ), title );
			Q_strcat( buffer, sizeof( buffer ), "\n\n" );

			while ( trap_Parse_ReadToken( handle, &token ) &&
			        token.string[ 0 ] != 0 && token.string[ 0 ] != '}' )
			{
				Q_strcat( buffer, sizeof( buffer ), token.string );
			}

			uiInfo.helpList[ uiInfo.helpCount ].text = String_Alloc( title );
			uiInfo.helpList[ uiInfo.helpCount ].v.text = String_Alloc( buffer );
			uiInfo.helpList[ uiInfo.helpCount ].type = INFOTYPE_TEXT;
			uiInfo.helpCount++;
			title[ 0 ] = 0;
		}
		else
		{
			Q_strcat( title, sizeof( title ), token.string );
		}
	}

	trap_Parse_FreeSource( handle );

	Com_Printf(_( "UI help file '%s' loaded in %d msec (%d infopanes)\n"),
	            helpFile, trap_Milliseconds() - start, uiInfo.helpCount );
}

void UI_Load( void )
{
	char      lastName[ 1024 ];
	menuDef_t *menu = Menu_GetFocused();

	if ( menu && menu->window.name )
	{
		strcpy( lastName, menu->window.name );
	}

	String_Init();

	UI_LoadMenus( ui_menuFiles.string, qtrue );
	UI_LoadMenus( ui_ingameFiles.string, qfalse );
	UI_LoadMenus( ui_teamFiles.string, qfalse );
	UI_LoadHelp( ui_helpFiles.string );
	Menus_CloseAll();
	Menus_ActivateByName( lastName );
}

/*
===============
UI_GetCurrentAlienStage
===============
*/
static stage_t UI_GetCurrentAlienStage( void )
{
	char    buffer[ MAX_TOKEN_CHARS ];
	stage_t stage, dummy;

	trap_Cvar_VariableStringBuffer( "ui_stages", buffer, sizeof( buffer ) );
	sscanf( buffer, "%d %d", ( int * ) &stage, ( int * ) &dummy );

	return stage;
}

/*
===============
UI_GetCurrentHumanStage
===============
*/
static stage_t UI_GetCurrentHumanStage( void )
{
	char    buffer[ MAX_TOKEN_CHARS ];
	stage_t stage, dummy;

	trap_Cvar_VariableStringBuffer( "ui_stages", buffer, sizeof( buffer ) );
	sscanf( buffer, "%d %d", ( int * ) &dummy, ( int * ) &stage );

	return stage;
}

/*
===============
UI_DrawInfoPane
===============
*/
static void UI_DrawInfoPane( menuItem_t *item, rectDef_t *rect, float text_x, float text_y,
                             float scale, int textalign, int textvalign, vec4_t color, int textStyle )
{
	int        value = 0;
	const char *s = "";
	char       *string = "";

	int        class, credits;
	char       ui_currentClass[ MAX_STRING_CHARS ];

	trap_Cvar_VariableStringBuffer( "ui_currentClass", ui_currentClass, MAX_STRING_CHARS );

	sscanf( ui_currentClass, "%d %d", &class, &credits );

	switch ( item->type )
	{
		case INFOTYPE_TEXT:
			s = item->v.text;
			break;

		case INFOTYPE_CLASS:
			value = ( BG_ClassCanEvolveFromTo( class, item->v.pclass, credits,
			                                   UI_GetCurrentAlienStage(), 0 ) +
			          ALIEN_CREDITS_PER_KILL - 1 ) / ALIEN_CREDITS_PER_KILL;

			if ( value < 1 )
			{
				s = va( "%s\n\n%s",
				        BG_ClassConfig( item->v.pclass )->humanName,
				        BG_Class( item->v.pclass )->info );
			}
			else
			{
				s = va( "%s\n\n%s\n\nFrags: %d",
				        BG_ClassConfig( item->v.pclass )->humanName,
				        BG_Class( item->v.pclass )->info,
				        value );
			}

			break;

		case INFOTYPE_WEAPON:
			value = BG_Weapon( item->v.weapon )->price;

			if ( value == 0 )
			{
				s = va( "%s\n\n%s\n\nCredits: Free",
				        BG_Weapon( item->v.weapon )->humanName,
				        BG_Weapon( item->v.weapon )->info );
			}
			else
			{
				s = va( "%s\n\n%s\n\nCredits: %d",
				        BG_Weapon( item->v.weapon )->humanName,
				        BG_Weapon( item->v.weapon )->info,
				        value );
			}

			break;

		case INFOTYPE_UPGRADE:
			value = BG_Upgrade( item->v.upgrade )->price;

			if ( value == 0 )
			{
				s = va( "%s\n\n%s\n\nCredits: Free",
				        BG_Upgrade( item->v.upgrade )->humanName,
				        BG_Upgrade( item->v.upgrade )->info );
			}
			else
			{
				s = va( "%s\n\n%s\n\nCredits: %d",
				        BG_Upgrade( item->v.upgrade )->humanName,
				        BG_Upgrade( item->v.upgrade )->info,
				        value );
			}

			break;

		case INFOTYPE_BUILDABLE:
			value = BG_Buildable( item->v.buildable )->buildPoints;

			switch ( BG_Buildable( item->v.buildable )->team )
			{
				case TEAM_ALIENS:
					string = "Sentience";
					break;

				case TEAM_HUMANS:
					string = "Power";
					break;

				default:
					break;
			}

			if ( value == 0 )
			{
				s = va( "%s\n\n%s",
				        BG_Buildable( item->v.buildable )->humanName,
				        BG_Buildable( item->v.buildable )->info );
			}
			else
			{
				s = va( "%s\n\n%s\n\n%s: %d",
				        BG_Buildable( item->v.buildable )->humanName,
				        BG_Buildable( item->v.buildable )->info,
				        string, value );
			}

			break;
	}

	UI_DrawTextBlock( rect, text_x, text_y, color, scale,
	                  textalign, textvalign, textStyle, s );
}

static void UI_DrawServerMapPreview( rectDef_t *rect, float scale, vec4_t color )
{
	if ( uiInfo.serverStatus.currentServerCinematic >= 0 )
	{
		trap_CIN_RunCinematic( uiInfo.serverStatus.currentServerCinematic );
		trap_CIN_SetExtents( uiInfo.serverStatus.currentServerCinematic, rect->x, rect->y, rect->w, rect->h );
		trap_CIN_DrawCinematic( uiInfo.serverStatus.currentServerCinematic );
	}
	else if ( uiInfo.serverStatus.currentServerPreview > 0 )
	{
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.serverStatus.currentServerPreview );
	}
	else
	{
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip( "gfx/2d/load_screen" ) );
	}
}

static void UI_DrawSelectedMapPreview( rectDef_t *rect, float scale, vec4_t color )
{
	int map = ui_selectedMap.integer;

	if ( map < 0 || map > uiInfo.mapCount )
	{
		ui_selectedMap.integer = 0;
		trap_Cvar_Set( "ui_selectedMap", "0" );
		map = 0;
	}

	if ( uiInfo.mapList[ map ].cinematic >= -1 )
	{
		if ( uiInfo.mapList[ map ].cinematic == -1 )
		{
			uiInfo.mapList[ map ].cinematic = trap_CIN_PlayCinematic( va( "%s.roq", uiInfo.mapList[ map ].mapLoadName ),
			                                  0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
		}

		if ( uiInfo.mapList[ map ].cinematic >= 0 )
		{
			trap_CIN_RunCinematic( uiInfo.mapList[ map ].cinematic );
			trap_CIN_SetExtents( uiInfo.mapList[ map ].cinematic, rect->x, rect->y, rect->w, rect->h );
			trap_CIN_DrawCinematic( uiInfo.mapList[ map ].cinematic );
		}
		else
		{
			uiInfo.mapList[ map ].cinematic = -2;
		}
	}
	else
	{
		if ( uiInfo.mapList[ map ].levelShot == -1 )
		{
			uiInfo.mapList[ map ].levelShot = trap_R_RegisterShaderNoMip( uiInfo.mapList[ map ].imageName );
		}

		if ( uiInfo.mapList[ map ].levelShot > 0 )
		{
			UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.mapList[ map ].levelShot );
		}
		else
		{
			UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip( "gfx/2d/load_screen" ) );
		}
	}
}

static void UI_DrawSelectedMapName( rectDef_t *rect, float scale, vec4_t color, int textStyle )
{
	int map = ui_selectedMap.integer;

	if ( map >= 0 && map < uiInfo.mapCount )
	{
		UI_Text_Paint( rect->x, rect->y, scale, color, uiInfo.mapList[ map ].mapName, 0, textStyle );
	}
}

static const char *UI_OwnerDrawText( int ownerDraw )
{
	const char *s = NULL;

	switch ( ownerDraw )
	{
		case UI_NETSOURCE:
			if ( ui_netSource.integer < 0 || ui_netSource.integer >= numNetSources )
			{
				ui_netSource.integer = 0;
			}

			s = netSources[ ui_netSource.integer ];
			break;

		case UI_KEYBINDSTATUS:
			if ( Display_KeyBindPending() )
			{
				s = "Waiting for new key… Press ESCAPE to cancel";
			}
			else
			{
				s = "Press ENTER or CLICK to change, Press BACKSPACE to clear";
			}

			break;

		case UI_SERVERREFRESHDATE:
			if ( uiInfo.serverStatus.refreshActive )
			{
#define MAX_DOTS 5
				int numServers = trap_LAN_GetServerCount( ui_netSource.integer );
				int numDots = ( uiInfo.uiDC.realTime / 500 ) % ( MAX_DOTS + 1 );
				char dots[ MAX_DOTS + 1 ];
				int i;

				for ( i = 0; i < numDots; i++ )
				{
					dots[ i ] = '.';
				}

				dots[ i ] = '\0';

				s = numServers < 0 ? va( "Waiting for response%s", dots ) :
				    va( "Getting info for %d servers (ESC to cancel)%s", numServers, dots );
			}
			else
			{
				s = va( "Refresh Time: %s", UI_Cvar_VariableString( va( "ui_lastServerRefresh_%i", ui_netSource.integer ) ) );
			}

			break;

		case UI_SERVERMOTD:
			s = uiInfo.serverStatus.motd;
			break;

		default:
			break;
	}

	return s;
}

static int UI_OwnerDrawWidth( int ownerDraw, float scale )
{
	const char *s = NULL;

	switch ( ownerDraw )
	{
		case UI_NETSOURCE:
		case UI_KEYBINDSTATUS:
		case UI_SERVERREFRESHDATE:
		case UI_SERVERMOTD:
			s = UI_OwnerDrawText( ownerDraw );
			break;

		default:
			break;
	}

	if ( s )
	{
		return UI_Text_Width( s, scale );
	}

	return 0;
}

/*
===============
UI_BuildPlayerList
===============
*/
static void UI_BuildPlayerList( void )
{
	uiClientState_t cs;
	int   n, count, team, team2, playerTeamNumber;
	char  info[ MAX_INFO_STRING ];

	trap_GetClientState( &cs );
	trap_GetConfigString( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
	uiInfo.playerNumber = cs.clientNum;
	team = atoi( Info_ValueForKey( info, "t" ) );
	trap_GetConfigString( CS_SERVERINFO, info, sizeof( info ) );
	count = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	uiInfo.playerCount = 0;
	uiInfo.myTeamCount = 0;
	uiInfo.myPlayerIndex = 0;
	playerTeamNumber = 0;

	for ( n = 0; n < count; n++ )
	{
		trap_GetConfigString( CS_PLAYERS + n, info, MAX_INFO_STRING );

		if ( info[ 0 ] )
		{
			Com_ClientListParse( &uiInfo.ignoreList[ uiInfo.playerCount ],
			                     Info_ValueForKey( info, "ig" ) );
			Q_strncpyz( uiInfo.rawPlayerNames[ uiInfo.playerCount ],
			            Info_ValueForKey( info, "n" ), MAX_NAME_LENGTH );
			Q_strncpyz( uiInfo.playerNames[ uiInfo.playerCount ],
			            Info_ValueForKey( info, "n" ), MAX_NAME_LENGTH );
			Q_CleanStr( uiInfo.playerNames[ uiInfo.playerCount ] );
			uiInfo.clientNums[ uiInfo.playerCount ] = n;

			if ( n == uiInfo.playerNumber )
			{
				uiInfo.myPlayerIndex = uiInfo.playerCount;
			}

			uiInfo.playerCount++;

			team2 = atoi( Info_ValueForKey( info, "t" ) );

			if ( team2 == team )
			{
				Q_strncpyz( uiInfo.rawTeamNames[ uiInfo.myTeamCount ],
				            Info_ValueForKey( info, "n" ), MAX_NAME_LENGTH );
				Q_strncpyz( uiInfo.teamNames[ uiInfo.myTeamCount ],
				            Info_ValueForKey( info, "n" ), MAX_NAME_LENGTH );
				Q_CleanStr( uiInfo.teamNames[ uiInfo.myTeamCount ] );
				uiInfo.teamClientNums[ uiInfo.myTeamCount ] = n;

				if ( uiInfo.playerNumber == n )
				{
					playerTeamNumber = uiInfo.myTeamCount;
				}

				uiInfo.myTeamCount++;
			}
		}
	}
}

static void UI_DrawGLInfo( rectDef_t *rect, float scale, int textalign, int textvalign,
                           vec4_t color, int textStyle, float text_x, float text_y )
{
	char      buffer[ 4096 ];

	Com_sprintf( buffer, sizeof( buffer ), "VENDOR: %s\nVERSION: %s\n"
	             "PIXELFORMAT: color(%d-bits) Z(%d-bits) stencil(%d-bits)\n%s",
	             uiInfo.uiDC.glconfig.vendor_string, uiInfo.uiDC.glconfig.renderer_string,
	             uiInfo.uiDC.glconfig.colorBits, uiInfo.uiDC.glconfig.depthBits,
	             uiInfo.uiDC.glconfig.stencilBits, uiInfo.uiDC.glconfig.extensions_string );

	UI_DrawTextBlock( rect, text_x, text_y, color, scale,
	                  textalign, textvalign, textStyle, buffer );
}

// FIXME: table drive
//
static void UI_OwnerDraw( float x, float y, float w, float h,
                          float text_x, float text_y, int ownerDraw,
                          int ownerDrawFlags, int align,
                          int textalign, int textvalign, float borderSize,
                          float scale, vec4_t foreColor, vec4_t backColor,
                          qhandle_t shader,
                          int textStyle )
{
	rectDef_t       rect;

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	switch ( ownerDraw )
	{
		case UI_TEAMINFOPANE:
			UI_DrawInfoPane( &uiInfo.teamList[ uiInfo.teamIndex ],
			                 &rect, text_x, text_y, scale, textalign, textvalign, foreColor, textStyle );
			break;

		case UI_ACLASSINFOPANE:
			UI_DrawInfoPane( &uiInfo.alienClassList[ uiInfo.alienClassIndex ],
			                 &rect, text_x, text_y, scale, textalign, textvalign, foreColor, textStyle );
			break;

		case UI_AUPGRADEINFOPANE:
			UI_DrawInfoPane( &uiInfo.alienUpgradeList[ uiInfo.alienUpgradeIndex ],
			                 &rect, text_x, text_y, scale, textalign, textvalign, foreColor, textStyle );
			break;

		case UI_HITEMINFOPANE:
			UI_DrawInfoPane( &uiInfo.humanItemList[ uiInfo.humanItemIndex ],
			                 &rect, text_x, text_y, scale, textalign, textvalign, foreColor, textStyle );
			break;

		case UI_HBUYINFOPANE:
			UI_DrawInfoPane( &uiInfo.humanArmouryBuyList[ uiInfo.humanArmouryBuyIndex ],
			                 &rect, text_x, text_y, scale, textalign, textvalign, foreColor, textStyle );
			break;

		case UI_HSELLINFOPANE:
			UI_DrawInfoPane( &uiInfo.humanArmourySellList[ uiInfo.humanArmourySellIndex ],
			                 &rect, text_x, text_y, scale, textalign, textvalign, foreColor, textStyle );
			break;

		case UI_ABUILDINFOPANE:
			UI_DrawInfoPane( &uiInfo.alienBuildList[ uiInfo.alienBuildIndex ],
			                 &rect, text_x, text_y, scale, textalign, textvalign, foreColor, textStyle );
			break;

		case UI_HBUILDINFOPANE:
			UI_DrawInfoPane( &uiInfo.humanBuildList[ uiInfo.humanBuildIndex ],
			                 &rect, text_x, text_y, scale, textalign, textvalign, foreColor, textStyle );
			break;

		case UI_HELPINFOPANE:
			UI_DrawInfoPane( &uiInfo.helpList[ uiInfo.helpIndex ],
			                 &rect, text_x, text_y, scale, textalign, textvalign, foreColor, textStyle );
			break;

		case UI_NETMAPPREVIEW:
			UI_DrawServerMapPreview( &rect, scale, foreColor );
			break;

		case UI_SELECTEDMAPPREVIEW:
			UI_DrawSelectedMapPreview( &rect, scale, foreColor );
			break;

		case UI_SELECTEDMAPNAME:
			UI_DrawSelectedMapName( &rect, scale, foreColor, textStyle );
			break;

		case UI_GLINFO:
			UI_DrawGLInfo( &rect, scale, textalign, textvalign, foreColor, textStyle, text_x, text_y );
			break;

		default:
			break;
	}
}

static qboolean UI_OwnerDrawVisible( int flags )
{
	qboolean vis = qtrue;
	uiClientState_t cs;
	team_t          team;
	char            info[ MAX_INFO_STRING ];

	trap_GetClientState( &cs );
	trap_GetConfigString( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
	team = atoi( Info_ValueForKey( info, "t" ) );

	while ( flags )
	{
		if ( flags & UI_SHOW_NOTSPECTATING )
		{
			if ( team == TEAM_NONE )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_NOTSPECTATING;
		}

		if ( flags & UI_SHOW_VOTEACTIVE )
		{
			if ( !trap_Cvar_VariableValue( "ui_voteActive" ) )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_VOTEACTIVE;
		}

		if ( flags & UI_SHOW_CANVOTE )
		{
			if ( trap_Cvar_VariableValue( "ui_voteActive" ) )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_CANVOTE;
		}

		if ( flags & UI_SHOW_TEAMVOTEACTIVE )
		{
			if ( team == TEAM_ALIENS )
			{
				if ( !trap_Cvar_VariableValue( "ui_alienTeamVoteActive" ) )
				{
					vis = qfalse;
				}
			}
			else if ( team == TEAM_HUMANS )
			{
				if ( !trap_Cvar_VariableValue( "ui_humanTeamVoteActive" ) )
				{
					vis = qfalse;
				}
			}

			flags &= ~UI_SHOW_TEAMVOTEACTIVE;
		}

		if ( flags & UI_SHOW_CANTEAMVOTE )
		{
			if ( team == TEAM_ALIENS )
			{
				if ( trap_Cvar_VariableValue( "ui_alienTeamVoteActive" ) )
				{
					vis = qfalse;
				}
			}
			else if ( team == TEAM_HUMANS )
			{
				if ( trap_Cvar_VariableValue( "ui_humanTeamVoteActive" ) )
				{
					vis = qfalse;
				}
			}

			flags &= ~UI_SHOW_CANTEAMVOTE;
		}

		if ( flags & UI_SHOW_FAVORITESERVERS )
		{
			// this assumes you only put this type of display flag on something showing in the proper context

			if ( ui_netSource.integer != AS_FAVORITES )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_FAVORITESERVERS;
		}

		if ( flags & UI_SHOW_NOTFAVORITESERVERS )
		{
			// this assumes you only put this type of display flag on something showing in the proper context

			if ( ui_netSource.integer == AS_FAVORITES )
			{
				vis = qfalse;
			}

			flags &= ~UI_SHOW_NOTFAVORITESERVERS;
		}
		else
		{
			flags = 0;
		}
	}

	return vis;
}

static qboolean UI_NetSource_HandleKey( int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER )
	{
		if ( key == K_MOUSE2 )
		{
			ui_netSource.integer--;
		}
		else
		{
			ui_netSource.integer++;
		}

		if ( ui_netSource.integer >= numNetSources )
		{
			ui_netSource.integer = 0;
		}
		else if ( ui_netSource.integer < 0 )
		{
			ui_netSource.integer = numNetSources - 1;
		}

		UI_BuildServerDisplayList( qtrue );

		if ( ui_netSource.integer != AS_GLOBAL )
		{
			UI_StartServerRefresh( qtrue );
		}

		trap_Cvar_Set( "ui_netSource", va( "%d", ui_netSource.integer ) );
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_OwnerDrawHandleKey( int ownerDraw, int key )
{
	switch ( ownerDraw )
	{
		case UI_NETSOURCE:
			UI_NetSource_HandleKey( key );
			break;

		default:
			break;
	}

	return qfalse;
}

/*
=================
UI_ServersQsortCompare
=================
*/
static int QDECL UI_ServersQsortCompare( const void *arg1, const void *arg2 )
{
	return trap_LAN_CompareServers( ui_netSource.integer, uiInfo.serverStatus.sortKey,
	                                uiInfo.serverStatus.sortDir, * ( int * ) arg1, * ( int * ) arg2 );
}

/*
=================
UI_ServersSort
=================
*/
void UI_ServersSort( int column, qboolean force )
{
	if ( !force )
	{
		if ( uiInfo.serverStatus.sortKey == column )
		{
			return;
		}
	}

	uiInfo.serverStatus.sortKey = column;
	qsort( &uiInfo.serverStatus.displayServers[ 0 ], uiInfo.serverStatus.numDisplayServers,
	       sizeof( int ), UI_ServersQsortCompare );
}

/*
===============
UI_LoadTeams
===============
*/
static void UI_LoadTeams( void )
{
	uiInfo.teamCount = 4;

	uiInfo.teamList[ 0 ].text = "Aliens";
	uiInfo.teamList[ 0 ].cmd = "cmd team aliens\n";
	uiInfo.teamList[ 0 ].type = INFOTYPE_TEXT;
	uiInfo.teamList[ 0 ].v.text =
	  "The Alien Team\n\n"
	  "The Aliens' strengths are in movement and the ability to "
	  "quickly construct new bases quickly. They possess a range "
	  "of abilities including basic melee attacks, movement-"
	  "crippling poisons and more.";

	uiInfo.teamList[ 1 ].text = "Humans";
	uiInfo.teamList[ 1 ].cmd = "cmd team humans\n";
	uiInfo.teamList[ 1 ].type = INFOTYPE_TEXT;
	uiInfo.teamList[ 1 ].v.text =
	  "The Human Team\n\n"
	  "The humans are the masters of technology. Although their "
	  "bases take long to construct, their automated defense "
	  "ensures they stay built. A wide range of upgrades and "
	  "weapons are available to the humans, each contributing "
	  "to eradicate the alien threat.";

	uiInfo.teamList[ 2 ].text = "Spectate";
	uiInfo.teamList[ 2 ].cmd = "cmd team spectate\n";
	uiInfo.teamList[ 2 ].type = INFOTYPE_TEXT;
	uiInfo.teamList[ 2 ].v.text = "Watch the game without playing.";

	uiInfo.teamList[ 3 ].text = "Auto select";
	uiInfo.teamList[ 3 ].cmd = "cmd team auto\n";
	uiInfo.teamList[ 3 ].type = INFOTYPE_TEXT;
	uiInfo.teamList[ 3 ].v.text = "Join the team with the least players.";
}

/*
===============
UI_AddClass
===============
*/

static void UI_AddClass( class_t class )
{
	uiInfo.alienClassList[ uiInfo.alienClassCount ].text =
	  BG_ClassConfig( class )->humanName;
	uiInfo.alienClassList[ uiInfo.alienClassCount ].cmd =
	  String_Alloc( va( "cmd class %s\n", BG_Class( class )->name ) );
	uiInfo.alienClassList[ uiInfo.alienClassCount ].type = INFOTYPE_CLASS;

	uiInfo.alienClassList[ uiInfo.alienClassCount ].v.pclass = class;

	uiInfo.alienClassCount++;
}

/*
===============
UI_LoadAlienClasses
===============
*/
static void UI_LoadAlienClasses( void )
{
	uiInfo.alienClassCount = 0;

	if ( BG_ClassIsAllowed( PCL_ALIEN_LEVEL0 ) )
	{
		UI_AddClass( PCL_ALIEN_LEVEL0 );
	}

	if ( BG_ClassIsAllowed( PCL_ALIEN_BUILDER0_UPG ) &&
	     BG_ClassAllowedInStage( PCL_ALIEN_BUILDER0_UPG, UI_GetCurrentAlienStage() ) )
	{
		UI_AddClass( PCL_ALIEN_BUILDER0_UPG );
	}
	else if ( BG_ClassIsAllowed( PCL_ALIEN_BUILDER0 ) )
	{
		UI_AddClass( PCL_ALIEN_BUILDER0 );
	}
}

/*
===============
UI_AddItem
===============
*/
static void UI_AddItem( weapon_t weapon )
{
	uiInfo.humanItemList[ uiInfo.humanItemCount ].text =
	  BG_Weapon( weapon )->humanName;
	uiInfo.humanItemList[ uiInfo.humanItemCount ].cmd =
	  String_Alloc( va( "cmd class %s\n", BG_Weapon( weapon )->name ) );
	uiInfo.humanItemList[ uiInfo.humanItemCount ].type = INFOTYPE_WEAPON;
	uiInfo.humanItemList[ uiInfo.humanItemCount ].v.weapon = weapon;

	uiInfo.humanItemCount++;
}

/*
===============
UI_LoadHumanItems
===============
*/
static void UI_LoadHumanItems( void )
{
	uiInfo.humanItemCount = 0;

	if ( BG_WeaponIsAllowed( WP_MACHINEGUN ) )
	{
		UI_AddItem( WP_MACHINEGUN );
	}

	if ( BG_WeaponIsAllowed( WP_HBUILD ) )
	{
		UI_AddItem( WP_HBUILD );
	}
}

/*
===============
UI_ParseCarriageList
===============
*/
static void UI_ParseCarriageList( void )
{
	int  i;
	char carriageCvar[ MAX_TOKEN_CHARS ];
	char *iterator;
	char buffer[ MAX_TOKEN_CHARS ];
	char *bufPointer;

	trap_Cvar_VariableStringBuffer( "ui_carriage", carriageCvar, sizeof( carriageCvar ) );
	iterator = carriageCvar;

	uiInfo.weapons = 0;
	uiInfo.upgrades = 0;

	//simple parser to give rise to weapon/upgrade list

	while ( iterator && iterator[ 0 ] != '$' )
	{
		bufPointer = buffer;

		if ( iterator[ 0 ] == 'W' )
		{
			iterator++;

			while ( iterator[ 0 ] != ' ' )
			{
				*bufPointer++ = *iterator++;
			}

			*bufPointer++ = '\n';

			i = atoi( buffer );

			uiInfo.weapons |= ( 1 << i );
		}
		else if ( iterator[ 0 ] == 'U' )
		{
			iterator++;

			while ( iterator[ 0 ] != ' ' )
			{
				*bufPointer++ = *iterator++;
			}

			*bufPointer++ = '\n';

			i = atoi( buffer );

			uiInfo.upgrades |= ( 1 << i );
		}

		iterator++;
	}
}

/*
===============
UI_LoadHumanArmouryBuys
===============
*/
static void UI_LoadHumanArmouryBuys( void )
{
	int i, j = 0;
	stage_t stage = UI_GetCurrentHumanStage();
	int slots = 0;

	UI_ParseCarriageList();

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		if ( uiInfo.weapons & ( 1 << i ) )
		{
			slots |= BG_Weapon( i )->slots;
		}
	}

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( uiInfo.upgrades & ( 1 << i ) )
		{
			slots |= BG_Upgrade( i )->slots;
		}
	}

	uiInfo.humanArmouryBuyCount = 0;

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		if ( BG_Weapon( i )->team == TEAM_HUMANS &&
		     BG_Weapon( i )->purchasable &&
		     BG_WeaponAllowedInStage( i, stage ) &&
		     BG_WeaponIsAllowed( i ) &&
		     !( BG_Weapon( i )->slots & slots ) &&
		     !( uiInfo.weapons & ( 1 << i ) ) )
		{
			uiInfo.humanArmouryBuyList[ j ].text = BG_Weapon( i )->humanName;
			uiInfo.humanArmouryBuyList[ j ].cmd =
			  String_Alloc( va( "cmd buy %s\n", BG_Weapon( i )->name ) );
			uiInfo.humanArmouryBuyList[ j ].type = INFOTYPE_WEAPON;
			uiInfo.humanArmouryBuyList[ j ].v.weapon = i;

			j++;

			uiInfo.humanArmouryBuyCount++;
		}
	}

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( BG_Upgrade( i )->team == TEAM_HUMANS &&
		     BG_Upgrade( i )->purchasable &&
		     BG_UpgradeAllowedInStage( i, stage ) &&
		     BG_UpgradeIsAllowed( i ) &&
		     !( BG_Upgrade( i )->slots & slots ) &&
		     !( uiInfo.upgrades & ( 1 << i ) ) )
		{
			uiInfo.humanArmouryBuyList[ j ].text = BG_Upgrade( i )->humanName;
			uiInfo.humanArmouryBuyList[ j ].cmd =
			  String_Alloc( va( "cmd buy %s\n", BG_Upgrade( i )->name ) );
			uiInfo.humanArmouryBuyList[ j ].type = INFOTYPE_UPGRADE;
			uiInfo.humanArmouryBuyList[ j ].v.upgrade = i;

			j++;

			uiInfo.humanArmouryBuyCount++;
		}
	}
}

/*
===============
UI_LoadHumanArmourySells
===============
*/
static void UI_LoadHumanArmourySells( void )
{
	int i, j = 0;

	uiInfo.humanArmourySellCount = 0;
	UI_ParseCarriageList();

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		if ( uiInfo.weapons & ( 1 << i ) )
		{
			uiInfo.humanArmourySellList[ j ].text = BG_Weapon( i )->humanName;
			uiInfo.humanArmourySellList[ j ].cmd =
			  String_Alloc( va( "cmd sell %s\n", BG_Weapon( i )->name ) );
			uiInfo.humanArmourySellList[ j ].type = INFOTYPE_WEAPON;
			uiInfo.humanArmourySellList[ j ].v.weapon = i;

			j++;

			uiInfo.humanArmourySellCount++;
		}
	}

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( uiInfo.upgrades & ( 1 << i ) )
		{
			uiInfo.humanArmourySellList[ j ].text = BG_Upgrade( i )->humanName;
			uiInfo.humanArmourySellList[ j ].cmd =
			  String_Alloc( va( "cmd sell %s\n", BG_Upgrade( i )->name ) );
			uiInfo.humanArmourySellList[ j ].type = INFOTYPE_UPGRADE;
			uiInfo.humanArmourySellList[ j ].v.upgrade = i;

			j++;

			uiInfo.humanArmourySellCount++;
		}
	}
}

/*
===============
UI_ArmouryRefreshCb
===============
*/
static void UI_ArmouryRefreshCb( void *data )
{
	int oldWeapons = uiInfo.weapons;
	int oldUpgrades = uiInfo.upgrades;

	UI_ParseCarriageList();

	if ( uiInfo.weapons != oldWeapons || uiInfo.upgrades != oldUpgrades )
	{
		UI_LoadHumanArmouryBuys();
		UI_LoadHumanArmourySells();
		UI_RemoveCaptureFunc();
	}
}

/*
===============
UI_LoadAlienUpgrades
===============
*/
static void UI_LoadAlienUpgrades( void )
{
	int     i, j = 0;

	int     class, credits;
	char    ui_currentClass[ MAX_STRING_CHARS ];
	stage_t stage = UI_GetCurrentAlienStage();

	trap_Cvar_VariableStringBuffer( "ui_currentClass", ui_currentClass, MAX_STRING_CHARS );

	sscanf( ui_currentClass, "%d %d", &class, &credits );

	uiInfo.alienUpgradeCount = 0;

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		if ( BG_ClassCanEvolveFromTo( class, i, credits, stage, 0 ) >= 0 )
		{
			uiInfo.alienUpgradeList[ j ].text = BG_ClassConfig( i )->humanName;
			uiInfo.alienUpgradeList[ j ].cmd =
			  String_Alloc( va( "cmd class %s\n", BG_Class( i )->name ) );
			uiInfo.alienUpgradeList[ j ].type = INFOTYPE_CLASS;
			uiInfo.alienUpgradeList[ j ].v.pclass = i;

			j++;

			uiInfo.alienUpgradeCount++;
		}
	}
}

/*
===============
UI_LoadAlienBuilds
===============
*/
static void UI_LoadAlienBuilds( void )
{
	int     i, j = 0;
	stage_t stage;

	UI_ParseCarriageList();
	stage = UI_GetCurrentAlienStage();

	uiInfo.alienBuildCount = 0;

	for ( i = BA_NONE + 1; i < BA_NUM_BUILDABLES; i++ )
	{
		if ( BG_Buildable( i )->team == TEAM_ALIENS &&
		     BG_Buildable( i )->buildWeapon & uiInfo.weapons &&
		     BG_BuildableAllowedInStage( i, stage ) &&
		     BG_BuildableIsAllowed( i ) )
		{
			uiInfo.alienBuildList[ j ].text = BG_Buildable( i )->humanName;
			uiInfo.alienBuildList[ j ].cmd =
			  String_Alloc( va( "cmd build %s\n", BG_Buildable( i )->name ) );
			uiInfo.alienBuildList[ j ].type = INFOTYPE_BUILDABLE;
			uiInfo.alienBuildList[ j ].v.buildable = i;

			j++;

			uiInfo.alienBuildCount++;
		}
	}
}

/*
===============
UI_LoadHumanBuilds
===============
*/
static void UI_LoadHumanBuilds( void )
{
	int     i, j = 0;
	stage_t stage;

	UI_ParseCarriageList();
	stage = UI_GetCurrentHumanStage();

	uiInfo.humanBuildCount = 0;

	for ( i = BA_NONE + 1; i < BA_NUM_BUILDABLES; i++ )
	{
		if ( BG_Buildable( i )->team == TEAM_HUMANS &&
		     BG_Buildable( i )->buildWeapon & uiInfo.weapons &&
		     BG_BuildableAllowedInStage( i, stage ) &&
		     BG_BuildableIsAllowed( i ) )
		{
			uiInfo.humanBuildList[ j ].text = BG_Buildable( i )->humanName;
			uiInfo.humanBuildList[ j ].cmd =
			  String_Alloc( va( "cmd build %s\n", BG_Buildable( i )->name ) );
			uiInfo.humanBuildList[ j ].type = INFOTYPE_BUILDABLE;
			uiInfo.humanBuildList[ j ].v.buildable = i;

			j++;

			uiInfo.humanBuildCount++;
		}
	}
}

/*
===============
UI_LoadProfiles
===============
*/
static void UI_LoadProfiles()
{
	int             numdirs;
	char            dirlist[ 2048 ];
	char           *dirptr;

	//char  *descptr;
	int             i;
	int             dirlen;

	uiInfo.profileCount = 0;
	uiInfo.profileIndex = -1;
	numdirs = trap_FS_GetFileList( "profiles", "/", dirlist, sizeof( dirlist ) );
	dirptr = dirlist;

	for ( i = 0; i < numdirs; i++ )
	{
		dirlen = strlen( dirptr ) + 1;

		if ( dirptr[ 0 ] && Q_stricmp( dirptr, "." ) && Q_stricmp( dirptr, ".." ) )
		{
			int  handle;
			char token[ 40 ];

			if ( trap_FS_FOpenFile( va( "profiles/%s/profile.dat", dirptr ), &handle, FS_READ ) == -1 )
			{
				dirptr += dirlen;
				continue;
			}

			memset( token, 0, sizeof( token ) );

			trap_FS_Read( token, sizeof( token ) - 1, handle );
			Q_CleanStr( token );
			Q_CleanDirName( token );

			if ( !token[ 0 ] )
			{
				strcpy( token, "UnnamedPlayer" );
			}

			uiInfo.profileList[ uiInfo.profileCount ].name = String_Alloc( token );
			trap_FS_FCloseFile( handle );

			uiInfo.profileList[ uiInfo.profileCount ].dir = String_Alloc( dirptr );
			uiInfo.profileCount++;

			/*if( uiInfo.profileCount == 1 ) {
			   int j;

			   uiInfo.profileIndex = 0;
			   trap_Cvar_Set( "ui_profile", token );

			   for( j = 0; j < Menu_Count(); j++ ) {
			   Menu_SetFeederSelection( Menu_Get(j), FEEDER_PROFILES, 0, NULL );
			   }
			   } */
			if ( uiInfo.profileIndex == -1 )
			{
				Q_CleanStr( token );
				Q_CleanDirName( token );

				if ( !Q_stricmp( token, cl_profile.string ) )
				{
					int             j;

					uiInfo.profileIndex = i;

					for ( j = 0; j < Menu_Count(); j++ )
					{
						Menu_SetFeederSelection( Menu_Get( j ), FEEDER_PROFILES, uiInfo.profileIndex, NULL );
					}
				}
			}

			if ( uiInfo.profileCount >= MAX_PROFILES )
			{
				break;
			}
		}

		dirptr += dirlen;
	}

	if ( uiInfo.profileIndex == -1 )
	{
		int             j;

		uiInfo.profileIndex = 0;

		for ( j = 0; j < Menu_Count(); j++ )
		{
			Menu_SetFeederSelection( Menu_Get( j ), FEEDER_PROFILES, 0, NULL );
		}
	}
}

/*
===============
UI_LoadMods
===============
*/
static void UI_LoadMods( void )
{
	int   numdirs;
	char  dirlist[ 2048 ];
	char  *dirptr;
	char  *descptr;
	int   i;
	int   dirlen;

	uiInfo.modCount = 0;
	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof( dirlist ) );
	dirptr = dirlist;

	for ( i = 0; i < numdirs; i++ )
	{
		dirlen = strlen( dirptr ) + 1;
		descptr = dirptr + dirlen;
		uiInfo.modList[ uiInfo.modCount ].modName = String_Alloc( dirptr );
		uiInfo.modList[ uiInfo.modCount ].modDescr = String_Alloc( descptr );
		dirptr += dirlen + strlen( descptr ) + 1;
		uiInfo.modCount++;

		if ( uiInfo.modCount >= MAX_MODS )
		{
			break;
		}
	}
}

/*
===============
UI_LoadMovies
===============
*/
static void UI_LoadMovies( void )
{
	char  movielist[ 4096 ];
	char  *moviename;
	int   i, len;

	uiInfo.movieCount = trap_FS_GetFileList( "video", "roq", movielist, 4096 );

	if ( uiInfo.movieCount )
	{
		if ( uiInfo.movieCount > MAX_MOVIES )
		{
			uiInfo.movieCount = MAX_MOVIES;
		}

		moviename = movielist;

		for ( i = 0; i < uiInfo.movieCount; i++ )
		{
			len = strlen( moviename );

			if ( !Q_stricmp( moviename +  len - 4, ".roq" ) )
			{
				moviename[ len - 4 ] = '\0';
			}

			Q_strupr( moviename );
			uiInfo.movieList[ i ] = String_Alloc( moviename );
			moviename += len + 1;
		}
	}
}

/*
===============
UI_LoadDemos
===============
*/
static void UI_LoadDemos( void )
{
	char  demolist[ 4096 ];
	char demoExt[ 32 ];
	char  *demoname;
	int   i, len;

	Com_sprintf( demoExt, sizeof( demoExt ), "dm_%d", ( int ) trap_Cvar_VariableValue( "protocol" ) );

	uiInfo.demoCount = trap_FS_GetFileList( "demos", demoExt, demolist, 4096 );

	Com_sprintf( demoExt, sizeof( demoExt ), ".dm_%d", ( int ) trap_Cvar_VariableValue( "protocol" ) );

	if ( uiInfo.demoCount )
	{
		if ( uiInfo.demoCount > MAX_DEMOS )
		{
			uiInfo.demoCount = MAX_DEMOS;
		}

		demoname = demolist;

		for ( i = 0; i < uiInfo.demoCount; i++ )
		{
			len = strlen( demoname );

			if ( !Q_stricmp( demoname +  len - strlen( demoExt ), demoExt ) )
			{
				demoname[ len - strlen( demoExt ) ] = '\0';
			}

			uiInfo.demoList[ i ] = String_Alloc( demoname );
			demoname += len + 1;
		}
	}
}

static void UI_Update( const char *name )
{
	int val = trap_Cvar_VariableValue( name );

	if ( Q_stricmp( name, "ui_SetName" ) == 0 )
	{
		trap_Cvar_Set( "name", UI_Cvar_VariableString( "ui_Name" ) );
	}
	else if ( Q_stricmp( name, "ui_setRate" ) == 0 )
	{
		float rate = trap_Cvar_VariableValue( "rate" );

		if ( rate >= 5000 )
		{
			trap_Cvar_Set( "cl_maxpackets", "30" );
			trap_Cvar_Set( "cl_packetdup", "1" );
		}
		else if ( rate >= 4000 )
		{
			trap_Cvar_Set( "cl_maxpackets", "15" );
			trap_Cvar_Set( "cl_packetdup", "2" );  // favor less prediction errors when there's packet loss
		}
		else
		{
			trap_Cvar_Set( "cl_maxpackets", "15" );
			trap_Cvar_Set( "cl_packetdup", "1" );  // favor lower bandwidth
		}
	}
	else if ( Q_stricmp( name, "ui_GetName" ) == 0 )
	{
		trap_Cvar_Set( "ui_Name", UI_Cvar_VariableString( "name" ) );
	}
	else if ( Q_stricmp( name, "r_colorbits" ) == 0 )
	{
		switch ( val )
		{
			case 0:
				trap_Cvar_SetValue( "r_depthbits", 0 );
				trap_Cvar_SetValue( "r_stencilbits", 0 );
				break;

			case 16:
				trap_Cvar_SetValue( "r_depthbits", 16 );
				trap_Cvar_SetValue( "r_stencilbits", 0 );
				break;

			case 32:
				trap_Cvar_SetValue( "r_depthbits", 24 );
				break;
		}
	}
	else if ( Q_stricmp( name, "r_lodbias" ) == 0 )
	{
	        if ( (unsigned int) val < 3 )
		{
			trap_Cvar_SetValue( "r_subdivisions", 4 + 8 * val );
		}
	}
	else if ( Q_stricmp( name, "ui_glCustom" ) == 0 )
	{
		switch ( val )
		{
			case 0: // high quality
				trap_Cvar_SetValue( "r_subdivisions", 4 );
				trap_Cvar_SetValue( "r_vertexlighting", 0 );
				trap_Cvar_SetValue( "r_lodbias", 0 );
				trap_Cvar_SetValue( "r_colorbits", 32 );
				trap_Cvar_SetValue( "r_depthbits", 24 );
				trap_Cvar_SetValue( "r_picmip", 0 );
				trap_Cvar_SetValue( "r_texturebits", 32 );
				trap_Cvar_SetValue( "r_fastSky", 0 );
				trap_Cvar_SetValue( "r_inGameVideo", 1 );
				trap_Cvar_SetValue( "cg_shadows", 1 );
				trap_Cvar_SetValue( "cg_bounceParticles", 1 );
				trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
				break;

			case 1: // intermediate
				trap_Cvar_SetValue( "r_subdivisions", 12 );
				trap_Cvar_SetValue( "r_vertexlighting", 0 );
				trap_Cvar_SetValue( "r_lodbias", 0 );
				trap_Cvar_SetValue( "r_colorbits", 0 );
				trap_Cvar_SetValue( "r_depthbits", 24 );
				trap_Cvar_SetValue( "r_picmip", 1 );
				trap_Cvar_SetValue( "r_texturebits", 0 );
				trap_Cvar_SetValue( "r_fastSky", 0 );
				trap_Cvar_SetValue( "r_inGameVideo", 1 );
				trap_Cvar_SetValue( "cg_shadows", 0 );
				trap_Cvar_SetValue( "cg_bounceParticles", 0 );
				trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
				break;

			case 2: // fast
				trap_Cvar_SetValue( "r_subdivisions", 8 );
				trap_Cvar_SetValue( "r_vertexlighting", 0 );
				trap_Cvar_SetValue( "r_lodbias", 1 );
				trap_Cvar_SetValue( "r_colorbits", 0 );
				trap_Cvar_SetValue( "r_depthbits", 0 );
				trap_Cvar_SetValue( "r_picmip", 1 );
				trap_Cvar_SetValue( "r_texturebits", 0 );
				trap_Cvar_SetValue( "r_fastSky", 1 );
				trap_Cvar_SetValue( "r_inGameVideo", 0 );
				trap_Cvar_SetValue( "cg_shadows", 0 );
				trap_Cvar_SetValue( "cg_bounceParticles", 0 );
				trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
				break;

			case 3: // fastest
				trap_Cvar_SetValue( "r_subdivisions", 20 );
				trap_Cvar_SetValue( "r_vertexlighting", 1 );
				trap_Cvar_SetValue( "r_lodbias", 2 );
				trap_Cvar_SetValue( "r_colorbits", 16 );
				trap_Cvar_SetValue( "r_depthbits", 16 );
				trap_Cvar_SetValue( "r_picmip", 2 );
				trap_Cvar_SetValue( "r_texturebits", 16 );
				trap_Cvar_SetValue( "r_fastSky", 1 );
				trap_Cvar_SetValue( "r_inGameVideo", 0 );
				trap_Cvar_SetValue( "cg_shadows", 0 );
				trap_Cvar_SetValue( "cg_bounceParticles", 0 );
				trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
				break;
		}
	}
	else if ( Q_stricmp( name, "ui_mousePitch" ) == 0 )
	{
		if ( val == 0 )
		{
			trap_Cvar_SetValue( "m_pitch", 0.022f );
		}
		else
		{
			trap_Cvar_SetValue( "m_pitch", -0.022f );
		}
	}
}

//FIXME: lookup table
static void UI_RunMenuScript( char **args )
{
	const char *name, *name2;
	const char *cmd;

	if ( String_Parse( args, &name ) )
	{
		if ( Q_stricmp( name, "StartServer" ) == 0 )
		{
			trap_Cvar_SetValue( "dedicated", Com_Clamp( 0, 2, ui_dedicated.integer ) );
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n",
			                                       Quote( uiInfo.mapList[ ui_selectedMap.integer ].mapLoadName ) ) );
		}
		else if ( Q_stricmp( name, "resetDefaults" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n" );
			trap_Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n" );
			Controls_SetDefaults();
			trap_Cvar_Set( "com_introPlayed", "1" );
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
		}
		else if ( Q_stricmp( name, "loadArenas" ) == 0 )
		{
			UI_LoadArenas();
			Menu_SetFeederSelection( NULL, FEEDER_MAPS, 0, "createserver" );
		}
		else if ( Q_stricmp( name, "loadServerInfo" ) == 0 )
		{
			UI_ServerInfo();
		}
		else if ( Q_stricmp( name, "getNews" ) == 0 )
		{
			UI_UpdateNews( qtrue );
		}
		else if ( Q_stricmp( name, "saveControls" ) == 0 )
		{
			Controls_SetConfig( qtrue );
		}
		else if ( Q_stricmp( name, "loadControls" ) == 0 )
		{
			Controls_GetConfig();
		}
		else if ( Q_stricmp( name, "clearError" ) == 0 )
		{
			trap_Cvar_Set( "com_errorMessage", "" );
		}
		else if ( Q_stricmp( name, "RefreshServers" ) == 0 )
		{
			UI_StartServerRefresh( qtrue );
			UI_BuildServerDisplayList( qtrue );
		}
		else if ( Q_stricmp( name, "InitServerList" ) == 0 )
		{
			int time = trap_RealTime( NULL );
			int last;
			int sortColumn;

			// set up default sorting

			if ( !uiInfo.serverStatus.sorted && Int_Parse( args, &sortColumn ) )
			{
				uiInfo.serverStatus.sortKey = sortColumn;
				uiInfo.serverStatus.sortDir = 0;
			}

			// refresh if older than 3 days or if list is empty
			last = atoi( UI_Cvar_VariableString( va( "ui_lastServerRefresh_%i_time",
			                                     ui_netSource.integer ) ) );

			if ( trap_LAN_GetServerCount( ui_netSource.integer ) < 1 ||
			     ( time - last ) > 3600 )
			{
				UI_StartServerRefresh( qtrue );
				UI_BuildServerDisplayList( qtrue );
			}
		}
		else if ( Q_stricmp( name, "RefreshFilter" ) == 0 )
		{
			UI_StartServerRefresh( qfalse );
			UI_BuildServerDisplayList( qtrue );
		}
		else if ( Q_stricmp( name, "LoadDemos" ) == 0 )
		{
			UI_LoadDemos();
		}
		else if ( Q_stricmp( name, "LoadMovies" ) == 0 )
		{
			UI_LoadMovies();
		}
		else if ( Q_stricmp( name, "LoadMods" ) == 0 )
		{
			UI_LoadMods();
		}
		else if ( Q_stricmp( name, "LoadTeams" ) == 0 )
		{
			UI_LoadTeams();
		}
		else if ( Q_stricmp( name, "JoinTeam" ) == 0 )
		{
			if ( ( cmd = uiInfo.teamList[ uiInfo.teamIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp( name, "LoadHumanItems" ) == 0 )
		{
			UI_LoadHumanItems();
		}
		else if ( Q_stricmp( name, "SpawnWithHumanItem" ) == 0 )
		{
			if ( ( cmd = uiInfo.humanItemList[ uiInfo.humanItemIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp( name, "LoadAlienClasses" ) == 0 )
		{
			UI_LoadAlienClasses();
		}
		else if ( Q_stricmp( name, "SpawnAsAlienClass" ) == 0 )
		{
			if ( ( cmd = uiInfo.alienClassList[ uiInfo.alienClassIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp( name, "LoadHumanArmouryBuys" ) == 0 )
		{
			UI_LoadHumanArmouryBuys();
		}
		else if ( Q_stricmp( name, "BuyFromArmoury" ) == 0 )
		{
			if ( ( cmd = uiInfo.humanArmouryBuyList[ uiInfo.humanArmouryBuyIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, cmd );
			}

			UI_InstallCaptureFunc( UI_ArmouryRefreshCb, NULL, 1000 );
		}
		else if ( Q_stricmp( name, "LoadHumanArmourySells" ) == 0 )
		{
			UI_LoadHumanArmourySells();
		}
		else if ( Q_stricmp( name, "SellToArmoury" ) == 0 )
		{
			if ( ( cmd = uiInfo.humanArmourySellList[ uiInfo.humanArmourySellIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, cmd );
			}

			UI_InstallCaptureFunc( UI_ArmouryRefreshCb, NULL, 1000 );
		}
		else if ( Q_stricmp( name, "LoadAlienUpgrades" ) == 0 )
		{
			UI_LoadAlienUpgrades();

			//disallow the menu if it would be empty

			if ( uiInfo.alienUpgradeCount <= 0 )
			{
				Menus_CloseAll();
			}
		}
		else if ( Q_stricmp( name, "UpgradeToNewClass" ) == 0 )
		{
			if ( ( cmd = uiInfo.alienUpgradeList[ uiInfo.alienUpgradeIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp( name, "LoadAlienBuilds" ) == 0 )
		{
			UI_LoadAlienBuilds();
		}
		else if ( Q_stricmp( name, "BuildAlienBuildable" ) == 0 )
		{
			if ( ( cmd = uiInfo.alienBuildList[ uiInfo.alienBuildIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp( name, "LoadHumanBuilds" ) == 0 )
		{
			UI_LoadHumanBuilds();
		}
		else if ( Q_stricmp( name, "BuildHumanBuildable" ) == 0 )
		{
			if ( ( cmd = uiInfo.humanBuildList[ uiInfo.humanBuildIndex ].cmd ) )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, cmd );
			}
		}
		else if ( Q_stricmp( name, "Say" ) == 0 )
		{
			char buffer[ MAX_CVAR_VALUE_STRING ];
			trap_Cvar_VariableStringBuffer( "ui_sayBuffer", buffer, sizeof( buffer ) );

			if ( !buffer[ 0 ] )
			{
				;
			}
			else if ( ui_chatCommands.integer && ( buffer[ 0 ] == '/' ||
			                                       buffer[ 0 ] == '\\' ) )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "%s\n", buffer + 1 ) );
			}
			else if ( uiInfo.chatTeam )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "say_team %s\n", Quote( buffer ) ) );
			}
			else if ( uiInfo.chatAdmin )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "a %s\n", Quote( buffer ) ) );
			}
			else if ( uiInfo.chatIRC )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "irc_say %s\n", Quote( buffer ) ) );
			}
			else
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "say %s\n", Quote( buffer ) ) );
			}
		}
		else if ( Q_stricmp( name, "SayKeydown" ) == 0 )
		{
			if ( ui_chatCommands.integer )
			{
				char buffer[ MAX_CVAR_VALUE_STRING ];
				trap_Cvar_VariableStringBuffer( "ui_sayBuffer", buffer, sizeof( buffer ) );

				if ( buffer[ 0 ] == '/' || buffer[ 0 ] == '\\' )
				{
					Menus_ReplaceActiveByName( "say_command" );
				}
				else if ( uiInfo.chatTeam )
				{
					Menus_ReplaceActiveByName( "say_team" );
				}
				else if ( uiInfo.chatAdmin )
				{
					Menus_ReplaceActiveByName( "a" );
				}
				else if ( uiInfo.chatIRC )
				{
					Menus_ReplaceActiveByName( "irc_say" );
				}
				else
				{
					Menus_ReplaceActiveByName( "say" );
				}
			}
		}
		else if ( Q_stricmp( name, "playMovie" ) == 0 )
		{
			if ( uiInfo.previewMovie >= 0 )
			{
				trap_CIN_StopCinematic( uiInfo.previewMovie );
			}

			trap_Cmd_ExecuteText( EXEC_APPEND, va( "cinematic %s.roq 2\n", Quote( uiInfo.movieList[ uiInfo.movieIndex ] ) ) );
		}
		else if ( Q_stricmp( name, "RunMod" ) == 0 )
		{
			trap_Cvar_Set( "fs_game", uiInfo.modList[ uiInfo.modIndex ].modName );
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
		}
		else if ( Q_stricmp( name, "RunDemo" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "demo %s\n", Quote( uiInfo.demoList[ uiInfo.demoIndex ] ) ) );
		}
		else if ( Q_stricmp( name, "Tremulous" ) == 0 )
		{
			trap_Cvar_Set( "fs_game", "" );
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
		}
		else if ( Q_stricmp( name, "closeJoin" ) == 0 )
		{
			if ( uiInfo.serverStatus.refreshActive )
			{
				UI_StopServerRefresh();
				uiInfo.serverStatus.nextDisplayRefresh = 0;
				uiInfo.nextServerStatusRefresh = 0;
				uiInfo.nextFindPlayerRefresh = 0;
				UI_BuildServerDisplayList( qtrue );
			}
			else
			{
				Menus_CloseByName( "joinserver" );
				Menus_ActivateByName( "main" );
			}
		}
		else if ( Q_stricmp( name, "StopRefresh" ) == 0 )
		{
			UI_StopServerRefresh();
			uiInfo.serverStatus.nextDisplayRefresh = 0;
			uiInfo.nextServerStatusRefresh = 0;
			uiInfo.nextFindPlayerRefresh = 0;
		}
		else if ( Q_stricmp( name, "UpdateFilter" ) == 0 )
		{
			if ( ui_netSource.integer == AS_LOCAL )
			{
				UI_StartServerRefresh( qtrue );
			}

			UI_BuildServerDisplayList( qtrue );
			UI_FeederSelection( FEEDER_SERVERS, 0 );
		}
		else if ( Q_stricmp( name, "ServerStatus" ) == 0 )
		{
			trap_LAN_GetServerAddressString( ui_netSource.integer,
			                                 uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ],
			                                 uiInfo.serverStatusAddress, sizeof( uiInfo.serverStatusAddress ) );
			UI_BuildServerStatus( qtrue );
		}
		else if ( Q_stricmp( name, "FoundPlayerServerStatus" ) == 0 )
		{
			Q_strncpyz( uiInfo.serverStatusAddress,
			            uiInfo.foundPlayerServerAddresses[ uiInfo.currentFoundPlayerServer ],
			            sizeof( uiInfo.serverStatusAddress ) );
			UI_BuildServerStatus( qtrue );
			Menu_SetFeederSelection( NULL, FEEDER_FINDPLAYER, 0, NULL );
		}
		else if ( Q_stricmp( name, "FindPlayer" ) == 0 )
		{
			UI_BuildFindPlayerList( qtrue );
			// clear the displayed server status info
			uiInfo.serverStatusInfo.numLines = 0;
			Menu_SetFeederSelection( NULL, FEEDER_FINDPLAYER, 0, NULL );
		}
		else if ( Q_stricmp( name, "JoinServer" ) == 0 )
		{
			if ( uiInfo.serverStatus.currentServer >= 0 &&
			     uiInfo.serverStatus.currentServer < uiInfo.serverStatus.numDisplayServers )
			{
				char buff[ 1024 ];
				trap_LAN_GetServerAddressString( ui_netSource.integer,
				                                 uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ],
				                                 buff, 1024 );
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", Quote( buff ) ) );
			}
		}
		else if ( Q_stricmp( name, "FoundPlayerJoinServer" ) == 0 )
		{
			if ( uiInfo.currentFoundPlayerServer >= 0 &&
			     uiInfo.currentFoundPlayerServer < uiInfo.numFoundPlayerServers )
			{
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n",
				                                       Quote( uiInfo.foundPlayerServerAddresses[ uiInfo.currentFoundPlayerServer ] ) ) );
			}
		}
		else if ( Q_stricmp( name, "Quit" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
		}
		else if ( Q_stricmp( name, "Leave" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "main" );
		}
		else if ( Q_stricmp( name, "ServerSort" ) == 0 )
		{
			int sortColumn;

			if ( Int_Parse( args, &sortColumn ) )
			{
				// if same column we're already sorting on then flip the direction

				if ( sortColumn == uiInfo.serverStatus.sortKey )
				{
					uiInfo.serverStatus.sortDir = !uiInfo.serverStatus.sortDir;
				}

				// make sure we sort again
				UI_ServersSort( sortColumn, qtrue );

				uiInfo.serverStatus.sorted = qtrue;
			}
		}
		else if ( Q_stricmp( name, "closeingame" ) == 0 )
		{
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			trap_Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();
		}
		else if ( Q_strnicmp( name, "vote", 4 ) == 0 )
		{
			static const struct {
				const char vote[16];
				const char call[16];
				enum {
					V_NONE, V_MAP, V_PLAYER, V_TEAMMATE
				}          type;
				qboolean   reason;
			} voteInfo[] = {
				{ "Draw",           "draw",       V_NONE,     qtrue  },
				{ "Map",            "map",        V_MAP,      qfalse },
				{ "NextMap",        "nextmap",    V_MAP,      qfalse },
				{ "Kick",           "kick",       V_PLAYER,   qtrue  },
				{ "Spectate",       "spectate",   V_PLAYER,   qtrue  },
				{ "Mute",           "mute",       V_PLAYER,   qtrue  },
				{ "UnMute",         "unmute",     V_PLAYER,   qfalse },
				{ "TeamKick",       "kick",       V_TEAMMATE, qtrue  },
				{ "TeamSpectate",   "spectate",   V_TEAMMATE, qtrue  },
				{ "TeamDenyBuild",  "denybuild",  V_TEAMMATE, qtrue  },
				{ "TeamAllowBuild", "allowbuild", V_TEAMMATE, qfalse },
			};
			int i;

			for ( i = 0; i < ARRAY_LEN( voteInfo ); ++i )
			{
				if ( Q_stricmp( name + 4, voteInfo[i].vote ) == 0 )
				{
					char *buffer = "";

					if ( voteInfo[i].reason )
					{
						char rawbuffer[ MAX_CVAR_VALUE_STRING ];
						trap_Cvar_VariableStringBuffer( "ui_reason", rawbuffer, sizeof( rawbuffer ) );
						buffer = Quote( rawbuffer );
					}

					switch ( voteInfo[i].type )
					{
					case V_NONE:;
						trap_Cmd_ExecuteText( EXEC_APPEND,
						                      va( "callvote %s %s\n",
						                          voteInfo[i].call, buffer ) );
						break;

					case V_MAP:
						if ( ui_selectedMap.integer >= 0 && ui_selectedMap.integer < uiInfo.mapCount )
						{
							trap_Cmd_ExecuteText( EXEC_APPEND,
							                      va( "callvote %s %s %s\n",
							                          voteInfo[i].call,
							                          Quote( uiInfo.mapList[ ui_selectedMap.integer ].mapLoadName ),
							                          buffer ) );
						}
						break;

					case V_PLAYER:;
						if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
						{
							trap_Cmd_ExecuteText( EXEC_APPEND,
							                      va( "callvote %s %d %s\n",
							                          voteInfo[i].call,
							                          uiInfo.clientNums[ uiInfo.playerIndex ],
							                          buffer ) );
						}
						break;

					case V_TEAMMATE:;
						if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount )
						{
							trap_Cmd_ExecuteText( EXEC_APPEND,
							                      va( "callteamvote %s %d %s\n",
							                          voteInfo[i].call,
							                          uiInfo.clientNums[ uiInfo.playerIndex ],
							                          buffer ) );
						}
						break;
					}

					trap_Cvar_Set( "ui_reason", "" );
					break;
				}
			}
		}
		else if ( Q_stricmp( name, "addFavorite" ) == 0 )
		{
			if ( ui_netSource.integer != AS_FAVORITES )
			{
				char buff[ MAX_STRING_CHARS ];
				char name[ MAX_NAME_LENGTH ];
				char addr[ MAX_NAME_LENGTH ];
				int res;

				trap_LAN_GetServerInfo( ui_netSource.integer,
				                        uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ],
				                        buff, MAX_STRING_CHARS );
				name[ 0 ] = addr[ 0 ] = '\0';
				Q_strncpyz( name,  Info_ValueForKey( buff, "hostname" ), MAX_NAME_LENGTH );
				Q_strncpyz( addr,  Info_ValueForKey( buff, "addr" ), MAX_NAME_LENGTH );

				if ( strlen( name ) > 0 && strlen( addr ) > 0 )
				{
					res = trap_LAN_AddServer( AS_FAVORITES, name, addr );

					if ( res == 0 )
					{
						// server already in the list
						Com_Printf( "%s", _( "Favorite already in list\n" ));
					}
					else if ( res == -1 )
					{
						// list full
						Com_Printf( "%s", _( "Favorite list full\n" ));
					}
					else
					{
						// successfully added
						Com_Printf(_( "Added favorite server %s\n"), addr );
					}
				}
			}
		}

		else if ( Q_stricmp( name, "loadProfiles" ) == 0 )
		{
			UI_LoadProfiles();
		}
		else if ( Q_stricmp( name, "createProfile" ) == 0 )
		{
			fileHandle_t    f;
			char            buff[ MAX_CVAR_VALUE_STRING ];

			/*Q_strncpyz( cl_profile.string, ui_profile.string, sizeof(cl_profile.string) );
			  Q_CleanStr( cl_profile.string );
			  Q_CleanDirName( cl_profile.string );

			  trap_Cvar_Set( "cl_profile", cl_profile.string );
			  if( trap_FS_FOpenFile( va( "profiles/%s/profile.dat", cl_profile.string ), &f, FS_WRITE ) >= 0 ) {
			  trap_FS_Write( ui_profile.string, strlen(ui_profile.string), f );
			  trap_FS_FCloseFile( f );
			  } */
			Q_strncpyz( buff, ui_profile.string, sizeof( buff ) );
			if( buff[0] )
			{
				Q_CleanStr( buff );
				Q_CleanDirName( buff );

				if ( trap_FS_FOpenFile( va( "profiles/%s/profile.dat", buff ), &f, FS_WRITE ) >= 0 )
				{
					trap_FS_Write( ui_profile.string, strlen( ui_profile.string ), f );
					trap_FS_FCloseFile( f );
				}

				trap_Cvar_Set( "name", ui_profile.string );
			}
			else
			{
				trap_Cvar_Set( "com_errorMessage", "You did not enter a profile!" );
				Menus_ActivateByName( "error_popmenu" );
			}
		}
		else if ( Q_stricmp( name, "clearPID" ) == 0 )
		{
			fileHandle_t    f;

			// delete profile.pid from current profile
			if ( trap_FS_FOpenFile( va( "profiles/%s/profile.pid", cl_profile.string ), &f, FS_READ ) >= 0 )
			{
				trap_FS_FCloseFile( f );
				trap_FS_Delete( va( "profiles/%s/profile.pid", cl_profile.string ) );
			}
		}
		else if ( Q_stricmp( name, "applyProfile" ) == 0 )
		{
			Q_strncpyz( cl_profile.string, uiInfo.profileList[ uiInfo.profileIndex ].name, sizeof( cl_profile.string ) );
			Q_CleanStr( cl_profile.string );
			Q_CleanDirName( cl_profile.string );
			trap_Cvar_Set( "cl_profile", cl_profile.string );
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "exec profiles/%s/autogen.cfg\n", cl_profile.string ) );
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "exec profiles/%s/autoexec.cfg\n", cl_profile.string ) );
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
		}
		else if ( Q_stricmp( name, "setDefaultProfile" ) == 0 )
		{
			fileHandle_t    f;

			Q_strncpyz( cl_defaultProfile.string, uiInfo.profileList[ uiInfo.profileIndex ].name, sizeof( cl_profile.string ) );
			Q_CleanStr( cl_defaultProfile.string );
			Q_CleanDirName( cl_defaultProfile.string );
			trap_Cvar_Set( "cl_defaultProfile", cl_defaultProfile.string );

			if ( trap_FS_FOpenFile( "profiles/defaultprofile.dat", &f, FS_WRITE ) >= 0 )
			{
				trap_FS_Write( cl_defaultProfile.string, strlen( cl_defaultProfile.string ), f );
				trap_FS_FCloseFile( f );
			}
		}
		else if ( Q_stricmp( name, "deleteProfile" ) == 0 )
		{
			char            buff[ MAX_CVAR_VALUE_STRING ];

			Q_strncpyz( buff, uiInfo.profileList[ uiInfo.profileIndex ].name, sizeof( buff ) );
			Q_CleanStr( buff );
			Q_CleanDirName( buff );

			// can't delete active profile
			if ( Q_stricmp( buff, cl_profile.string ) )
			{
				if ( !Q_stricmp( buff, cl_defaultProfile.string ) )
				{
					// if deleting the default profile, set the default to the current active profile
					fileHandle_t    f;

					trap_Cvar_Set( "cl_defaultProfile", cl_profile.string );

					if ( trap_FS_FOpenFile( "profiles/defaultprofile.dat", &f, FS_WRITE ) >= 0 )
					{
						trap_FS_Write( cl_profile.string, strlen( cl_profile.string ), f );
						trap_FS_FCloseFile( f );
					}
				}

				trap_FS_Delete( va( "profiles/%s", buff ) );
			}
		}
		else if ( Q_stricmp( name, "renameProfileInit" ) == 0 )
		{
			trap_Cvar_Set( "ui_profile_renameto", ui_profile.string );
		}
		else if ( Q_stricmp( name, "renameProfile" ) == 0 )
		{
			fileHandle_t    f, f2;
			int             len;
			char            buff[ MAX_CVAR_VALUE_STRING ];
			char            ui_renameprofileto[ MAX_CVAR_VALUE_STRING ];
			char            uiprofile[ MAX_CVAR_VALUE_STRING ];

			trap_Cvar_VariableStringBuffer( "ui_profile_renameto", ui_renameprofileto, sizeof( ui_renameprofileto ) );
			Q_strncpyz( buff, ui_renameprofileto, sizeof( buff ) );
			Q_CleanStr( buff );
			Q_CleanDirName( buff );

			Q_strncpyz( uiprofile, ui_profile.string, sizeof( uiprofile ) );
			Q_CleanStr( uiprofile );
			Q_CleanDirName( uiprofile );

			if ( trap_FS_FOpenFile( va( "profiles/%s/profile.dat", buff ), &f, FS_WRITE ) >= 0 )
			{
				trap_FS_Write( ui_renameprofileto, strlen( ui_renameprofileto ), f );
				trap_FS_FCloseFile( f );
			}

			// FIXME: make this copying handle all files in the profiles directory
			if ( Q_stricmp( uiprofile, buff ) )
			{
				if ( trap_FS_FOpenFile( va( "profiles/%s/%s", buff, CONFIG_NAME ), &f, FS_WRITE ) >= 0 )
				{
					if ( ( len = trap_FS_FOpenFile( va( "profiles/%s/%s", uiprofile, CONFIG_NAME ), &f2, FS_READ ) ) >= 0 )
					{
						int             i;

						for ( i = 0; i < len; i++ )
						{
							byte            b;

							trap_FS_Read( &b, 1, f2 );
							trap_FS_Write( &b, 1, f );
						}

						trap_FS_FCloseFile( f2 );
					}

					trap_FS_FCloseFile( f );
				}

				if ( trap_FS_FOpenFile( va( "profiles/%s/servercache.dat", buff ), &f, FS_WRITE ) >= 0 )
				{
					if ( ( len = trap_FS_FOpenFile( va( "profiles/%s/servercache.dat", cl_profile.string ), &f2, FS_READ ) ) >= 0 )
					{
						int             i;

						for ( i = 0; i < len; i++ )
						{
							byte            b;

							trap_FS_Read( &b, 1, f2 );
							trap_FS_Write( &b, 1, f );
						}

						trap_FS_FCloseFile( f2 );
					}

					trap_FS_FCloseFile( f );
				}

				if ( !Q_stricmp( uiprofile, cl_defaultProfile.string ) )
				{
					// if renaming the default profile, set the default to the new profile
					trap_Cvar_Set( "cl_defaultProfile", buff );

					if ( trap_FS_FOpenFile( "profiles/defaultprofile.dat", &f, FS_WRITE ) >= 0 )
					{
						trap_FS_Write( buff, strlen( buff ), f );
						trap_FS_FCloseFile( f );
					}
				}

				// if renaming the active profile, set active to new name
				if ( !Q_stricmp( uiprofile, cl_profile.string ) )
				{
					trap_Cvar_Set( "cl_profile", buff );
				}

				// delete the old profile
				trap_FS_Delete( va( "profiles/%s", uiprofile ) );
			}

			trap_Cvar_Set( "ui_profile", ui_renameprofileto );
			trap_Cvar_Set( "ui_profile_renameto", "" );
		}
		else if ( Q_stricmp( name, "deleteFavorite" ) == 0 )
		{
			if ( ui_netSource.integer == AS_FAVORITES )
			{
				char buff[ MAX_STRING_CHARS ];
				char addr[ MAX_NAME_LENGTH ];
				trap_LAN_GetServerInfo( ui_netSource.integer,
				                        uiInfo.serverStatus.displayServers[ uiInfo.serverStatus.currentServer ],
				                        buff, MAX_STRING_CHARS );
				addr[ 0 ] = '\0';
				Q_strncpyz( addr,  Info_ValueForKey( buff, "addr" ), MAX_NAME_LENGTH );

				if ( strlen( addr ) > 0 )
				{
					trap_LAN_RemoveServer( AS_FAVORITES, addr );
				}
			}
		}
		else if ( Q_stricmp( name, "createFavorite" ) == 0 )
		{
			if ( ui_netSource.integer == AS_FAVORITES )
			{
				char name[ MAX_NAME_LENGTH ];
				char addr[ MAX_NAME_LENGTH ];
				int res;

				name[ 0 ] = addr[ 0 ] = '\0';
				Q_strncpyz( name,  UI_Cvar_VariableString( "ui_favoriteName" ), MAX_NAME_LENGTH );
				Q_strncpyz( addr,  UI_Cvar_VariableString( "ui_favoriteAddress" ), MAX_NAME_LENGTH );

				if ( strlen( name ) > 0 && strlen( addr ) > 0 )
				{
					res = trap_LAN_AddServer( AS_FAVORITES, name, addr );

					if ( res == 0 )
					{
						// server already in the list
						Com_Printf( "%s", _("Favorite already in list\n" ));
					}
					else if ( res == -1 )
					{
						// list full
						Com_Printf("%s", _( "Favorite list full\n" ));
					}
					else
					{
						// successfully added
						Com_Printf(_( "Added favorite server %s\n"), addr );
					}
				}
			}
		}
		else if ( Q_stricmp( name, "glCustom" ) == 0 )
		{
			trap_Cvar_Set( "ui_glCustom", "4" );
		}
		else if ( Q_stricmp( name, "update" ) == 0 )
		{
			if ( String_Parse( args, &name2 ) )
			{
				UI_Update( name2 );
			}
		}
		else if ( Q_stricmp( name, "InitIgnoreList" ) == 0 )
		{
			UI_BuildPlayerList();
		}
		else if ( Q_stricmp( name, "ToggleIgnore" ) == 0 )
		{
			if ( uiInfo.ignoreIndex >= 0 && uiInfo.ignoreIndex < uiInfo.playerCount )
			{
				if ( Com_ClientListContains( &uiInfo.ignoreList[ uiInfo.myPlayerIndex ],
				                             uiInfo.clientNums[ uiInfo.ignoreIndex ] ) )
				{
					Com_ClientListRemove( &uiInfo.ignoreList[ uiInfo.myPlayerIndex ],
					                      uiInfo.clientNums[ uiInfo.ignoreIndex ] );
					trap_Cmd_ExecuteText( EXEC_APPEND, va( "unignore %i\n",
					                                       uiInfo.clientNums[ uiInfo.ignoreIndex ] ) );
				}
				else
				{
					Com_ClientListAdd( &uiInfo.ignoreList[ uiInfo.myPlayerIndex ],
					                   uiInfo.clientNums[ uiInfo.ignoreIndex ] );
					trap_Cmd_ExecuteText( EXEC_APPEND, va( "ignore %i\n",
					                                       uiInfo.clientNums[ uiInfo.ignoreIndex ] ) );
				}
			}
		}
		else if ( Q_stricmp( name, "IgnorePlayer" ) == 0 )
		{
			if ( uiInfo.ignoreIndex >= 0 && uiInfo.ignoreIndex < uiInfo.playerCount )
			{
				if ( !Com_ClientListContains( &uiInfo.ignoreList[ uiInfo.myPlayerIndex ],
				                              uiInfo.clientNums[ uiInfo.ignoreIndex ] ) )
				{
					Com_ClientListAdd( &uiInfo.ignoreList[ uiInfo.myPlayerIndex ],
					                   uiInfo.clientNums[ uiInfo.ignoreIndex ] );
					trap_Cmd_ExecuteText( EXEC_APPEND, va( "ignore %i\n",
					                                       uiInfo.clientNums[ uiInfo.ignoreIndex ] ) );
				}
			}
		}
		else if ( Q_stricmp( name, "UnIgnorePlayer" ) == 0 )
		{
			if ( uiInfo.ignoreIndex >= 0 && uiInfo.ignoreIndex < uiInfo.playerCount )
			{
				if ( Com_ClientListContains( &uiInfo.ignoreList[ uiInfo.myPlayerIndex ],
				                             uiInfo.clientNums[ uiInfo.ignoreIndex ] ) )
				{
					Com_ClientListRemove( &uiInfo.ignoreList[ uiInfo.myPlayerIndex ],
					                      uiInfo.clientNums[ uiInfo.ignoreIndex ] );
					trap_Cmd_ExecuteText( EXEC_APPEND, va( "unignore %i\n",
					                                       uiInfo.clientNums[ uiInfo.ignoreIndex ] ) );
				}
			}
		}
		else
		{
			Com_Printf(_( "unknown UI script %s\n"), name );
		}
	}
}

static int UI_FeederInitialise( int feederID );

/*
==================
UI_FeederCount
==================
*/
static int UI_FeederCount( int feederID )
{
	if ( feederID == FEEDER_CINEMATICS )
	{
		return uiInfo.movieCount;
	}
	else if ( feederID == FEEDER_MAPS )
	{
		return uiInfo.mapCount;
	}
	else if ( feederID == FEEDER_SERVERS )
	{
		return uiInfo.serverStatus.numDisplayServers;
	}
	else if ( feederID == FEEDER_SERVERSTATUS )
	{
		return uiInfo.serverStatusInfo.numLines;
	}
	else if ( feederID == FEEDER_NEWS )
	{
		return uiInfo.newsInfo.numLines;
	}
	else if ( feederID == FEEDER_FINDPLAYER )
	{
		return uiInfo.numFoundPlayerServers;
	}
	else if ( feederID == FEEDER_PLAYER_LIST )
	{
		if ( uiInfo.uiDC.realTime > uiInfo.playerRefresh )
		{
			uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
			UI_BuildPlayerList();
		}

		return uiInfo.playerCount;
	}
	else if ( feederID == FEEDER_TEAM_LIST )
	{
		if ( uiInfo.uiDC.realTime > uiInfo.playerRefresh )
		{
			uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
			UI_BuildPlayerList();
		}

		return uiInfo.myTeamCount;
	}
	else if ( feederID == FEEDER_IGNORE_LIST )
	{
		return uiInfo.playerCount;
	}
	else if ( feederID == FEEDER_HELP_LIST )
	{
		return uiInfo.helpCount;
	}
	else if ( feederID == FEEDER_MODS )
	{
		return uiInfo.modCount;
	}
	else if ( feederID == FEEDER_DEMOS )
	{
		return uiInfo.demoCount;
	}
	else if ( feederID == FEEDER_TREMTEAMS )
	{
		return uiInfo.teamCount;
	}
	else if ( feederID == FEEDER_TREMHUMANITEMS )
	{
		return uiInfo.humanItemCount;
	}
	else if ( feederID == FEEDER_TREMALIENCLASSES )
	{
		return uiInfo.alienClassCount;
	}
	else if ( feederID == FEEDER_TREMHUMANARMOURYBUY )
	{
		return uiInfo.humanArmouryBuyCount;
	}
	else if ( feederID == FEEDER_TREMHUMANARMOURYSELL )
	{
		return uiInfo.humanArmourySellCount;
	}
	else if ( feederID == FEEDER_TREMALIENUPGRADE )
	{
		return uiInfo.alienUpgradeCount;
	}
	else if ( feederID == FEEDER_TREMALIENBUILD )
	{
		return uiInfo.alienBuildCount;
	}
	else if ( feederID == FEEDER_TREMHUMANBUILD )
	{
		return uiInfo.humanBuildCount;
	}
	else if ( feederID == FEEDER_LANGUAGES )
	{
		return uiInfo.numLanguages;
	}
	else if ( feederID == FEEDER_PROFILES )
	{
		return uiInfo.profileCount;
	}
	else if ( feederID == FEEDER_RESOLUTIONS )
	{
		if ( UI_FeederInitialise( feederID ) == uiInfo.numResolutions )
		{
			return uiInfo.numResolutions + 1;
		}
		else
		{
			return uiInfo.numResolutions;
		}
	}

	return 0;
}

static const char *UI_SelectedMap( int index, int *actual )
{
	int i, c;
	c = 0;
	*actual = 0;

	for ( i = 0; i < uiInfo.mapCount; i++ )
	{
		if ( c == index )
		{
			*actual = i;
			return uiInfo.mapList[ i ].mapName;
		}
		else
		{
			c++;
		}
	}

	return "";
}

static int GCD( int a, int b )
{
	int c;

	while ( b != 0 )
	{
		c = a % b;
		a = b;
		b = c;
	}

	return a;
}

static const char *UI_DisplayAspectString( int w, int h )
{
	int gcd = GCD( w, h );

	w /= gcd;
	h /= gcd;

	// For some reason 8:5 is usually referred to as 16:10
	if ( w == 8 && h == 5 )
	{
		w = 16;
		h = 10;
	}

	return va( "%d:%d", w, h );
}

static const char *UI_FeederItemText( int feederID, int index, int column, qhandle_t *handle )
{
	if ( handle )
	{
		*handle = -1;
	}

	if ( feederID == FEEDER_MAPS )
	{
		int actual;
		return UI_SelectedMap( index, &actual );
	}
	else if ( feederID == FEEDER_SERVERS )
	{
		if ( index >= 0 && index < UI_FeederCount( feederID ) )
		{
			static char info[ MAX_STRING_CHARS ];
			static char clientBuff[ 32 ];
			static char cleaned[ MAX_STRING_CHARS ];
			static int  lastColumn = -1;
			static int  lastTime = 0;
			int         ping;

			if ( lastColumn != column || lastTime > uiInfo.uiDC.realTime + 5000 )
			{
				trap_LAN_GetServerInfo( ui_netSource.integer, uiInfo.serverStatus.displayServers[ index ],
				                        info, MAX_STRING_CHARS );
				lastColumn = column;
				lastTime = uiInfo.uiDC.realTime;
			}

			ping = atoi( Info_ValueForKey( info, "ping" ) );

			UI_EscapeEmoticons( cleaned, Info_ValueForKey( info, "hostname" ), sizeof( cleaned ) );

			switch ( column )
			{
				case SORT_HOST:
					if ( ping <= 0 )
					{
						return Info_ValueForKey( info, "addr" );
					}
					else
					{
						static char hostname[ 1024 ];

						if ( ui_netSource.integer == AS_LOCAL )
						{
							Com_sprintf( hostname, sizeof( hostname ), "%s [%s]", cleaned,
							             netnames[ atoi( Info_ValueForKey( info, "nettype" ) ) ] );
							return hostname;
						}
						else
						{
							char *text;
							char *label;

							label = Info_ValueForKey( info, "label" );

							if ( label[ 0 ] )
							{
								// First char of the label response is a sorting tag. Skip it.
								label += 1;

								Com_sprintf( hostname, sizeof( hostname ), "%s %s",
								             label, cleaned );
							}
							else
							{
								Com_sprintf( hostname, sizeof( hostname ), "%s",
								             cleaned );
							}

							// Strip leading whitespace
							text = hostname;

							while ( *text != '\0' && *text == ' ' )
							{
								text++;
							}

							return text;
						}
					}

				case SORT_GAME:
					return Info_ValueForKey( info, "game" ) [ 0 ] ? Info_ValueForKey( info, "game" ) : "main";

				case SORT_MAP:
					return Info_ValueForKey( info, "mapname" );

				case SORT_CLIENTS:
					Com_sprintf( clientBuff, sizeof( clientBuff ), "%s (%s)",
					             Info_ValueForKey( info, "clients" ), Info_ValueForKey( info, "sv_maxclients" ) );
					return clientBuff;

				case SORT_PING:
					if ( ping <= 0 )
					{
						return "…";
					}
					else
					{
						return Info_ValueForKey( info, "ping" );
					}
			}
		}
	}
	else if ( feederID == FEEDER_SERVERSTATUS )
	{
		if ( index >= 0 && index < uiInfo.serverStatusInfo.numLines )
		{
			if ( column >= 0 && column < 4 )
			{
				return uiInfo.serverStatusInfo.lines[ index ][ column ];
			}
		}
	}
	else if ( feederID == FEEDER_NEWS )
	{
		if ( index >= 0 && index < uiInfo.newsInfo.numLines )
		{
			return uiInfo.newsInfo.text[ index ];
		}
	}
	else if ( feederID == FEEDER_FINDPLAYER )
	{
		if ( index >= 0 && index < uiInfo.numFoundPlayerServers )
		{
			return uiInfo.foundPlayerServerNames[ index ];
		}
	}
	else if ( feederID == FEEDER_PLAYER_LIST )
	{
		if ( index >= 0 && index < uiInfo.playerCount )
		{
			return uiInfo.playerNames[ index ];
		}
	}
	else if ( feederID == FEEDER_TEAM_LIST )
	{
		if ( index >= 0 && index < uiInfo.myTeamCount )
		{
			return uiInfo.teamNames[ index ];
		}
	}
	else if ( feederID == FEEDER_IGNORE_LIST )
	{
		if ( index >= 0 && index < uiInfo.playerCount )
		{
			switch ( column )
			{
				case 1:
					// am I ignoring him
					return Com_ClientListContains( &uiInfo.ignoreList[ uiInfo.myPlayerIndex ],
					                               uiInfo.clientNums[ index ] ) ? "X" : "";

				case 2:
					// is he ignoring me
					return Com_ClientListContains( &uiInfo.ignoreList[ index ],
					                               uiInfo.playerNumber ) ? "X" : "";

				default:
					return uiInfo.playerNames[ index ];
			}
		}
	}
	else if ( feederID == FEEDER_HELP_LIST )
	{
		if ( index >= 0 && index < uiInfo.helpCount )
		{
			return uiInfo.helpList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_MODS )
	{
		if ( index >= 0 && index < uiInfo.modCount )
		{
			if ( uiInfo.modList[ index ].modDescr && *uiInfo.modList[ index ].modDescr )
			{
				return uiInfo.modList[ index ].modDescr;
			}
			else
			{
				return uiInfo.modList[ index ].modName;
			}
		}
	}
	else if ( feederID == FEEDER_CINEMATICS )
	{
		if ( index >= 0 && index < uiInfo.movieCount )
		{
			return uiInfo.movieList[ index ];
		}
	}
	else if ( feederID == FEEDER_DEMOS )
	{
		if ( index >= 0 && index < uiInfo.demoCount )
		{
			return uiInfo.demoList[ index ];
		}
	}
	else if ( feederID == FEEDER_TREMTEAMS )
	{
		if ( index >= 0 && index < uiInfo.teamCount )
		{
			return uiInfo.teamList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMHUMANITEMS )
	{
		if ( index >= 0 && index < uiInfo.humanItemCount )
		{
			return uiInfo.humanItemList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMALIENCLASSES )
	{
		if ( index >= 0 && index < uiInfo.alienClassCount )
		{
			return uiInfo.alienClassList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMHUMANARMOURYBUY )
	{
		if ( index >= 0 && index < uiInfo.humanArmouryBuyCount )
		{
			return uiInfo.humanArmouryBuyList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMHUMANARMOURYSELL )
	{
		if ( index >= 0 && index < uiInfo.humanArmourySellCount )
		{
			return uiInfo.humanArmourySellList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMALIENUPGRADE )
	{
		if ( index >= 0 && index < uiInfo.alienUpgradeCount )
		{
			return uiInfo.alienUpgradeList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMALIENBUILD )
	{
		if ( index >= 0 && index < uiInfo.alienBuildCount )
		{
			return uiInfo.alienBuildList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_TREMHUMANBUILD )
	{
		if ( index >= 0 && index < uiInfo.humanBuildCount )
		{
			return uiInfo.humanBuildList[ index ].text;
		}
	}
	else if ( feederID == FEEDER_LANGUAGES )
	{
		if ( index >= 0 && index < uiInfo.numLanguages )
		{
			return uiInfo.languages[ index ].name;
		}
	}
	else if ( feederID == FEEDER_PROFILES )
	{
		if( index >= 0 && index < uiInfo.profileCount )
		{
			return uiInfo.profileList[ index ].name;
		}
	}
	else if ( feederID == FEEDER_RESOLUTIONS )
	{
		static char resolution[ MAX_STRING_CHARS ];
		int w, h;

		if ( index >= 0 && index < uiInfo.numResolutions )
		{
			w = uiInfo.resolutions[ index ].w;
			h = uiInfo.resolutions[ index ].h;

			Com_sprintf( resolution, sizeof( resolution ), "%dx%d (%s)", w, h,
			             UI_DisplayAspectString( w, h ) );

			return resolution;
		}

		w = ( int ) trap_Cvar_VariableValue( "r_customwidth" );
		h = ( int ) trap_Cvar_VariableValue( "r_customheight" );
		Com_sprintf( resolution, sizeof( resolution ), "Custom (%dx%d)", w, h );

		return resolution;
	}

	return "";
}

static qhandle_t UI_FeederItemImage( int feederID, int index )
{
	if ( feederID == FEEDER_MAPS )
	{
		int actual;
		UI_SelectedMap( index, &actual );
		index = actual;

		if ( index >= 0 && index < uiInfo.mapCount )
		{
			if ( uiInfo.mapList[ index ].levelShot == -1 )
			{
				uiInfo.mapList[ index ].levelShot = trap_R_RegisterShaderNoMip( uiInfo.mapList[ index ].imageName );
			}

			return uiInfo.mapList[ index ].levelShot;
		}
	}

	return 0;
}

static void UI_FeederSelection( int feederID, int index )
{
	static char info[ MAX_STRING_CHARS ];

	if ( feederID == FEEDER_MAPS )
	{
		int actual, map;
		map = ui_selectedMap.integer;

		if ( uiInfo.mapList[ map ].cinematic >= 0 )
		{
			trap_CIN_StopCinematic( uiInfo.mapList[ map ].cinematic );
			uiInfo.mapList[ map ].cinematic = -1;
		}

		UI_SelectedMap( index, &actual );

		ui_selectedMap.integer = actual;
		trap_Cvar_Set( "ui_selectedMap", va( "%d", actual ) );
		uiInfo.mapList[ ui_selectedMap.integer ].cinematic =
		  trap_CIN_PlayCinematic( va( "%s.roq", uiInfo.mapList[ ui_selectedMap.integer ].mapLoadName ),
		                          0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
	}
	else if ( feederID == FEEDER_SERVERS )
	{
		const char *mapName = NULL;

		uiInfo.serverStatus.currentServer = index;
		trap_LAN_GetServerInfo( ui_netSource.integer, uiInfo.serverStatus.displayServers[ index ],
		                        info, MAX_STRING_CHARS );
		uiInfo.serverStatus.currentServerPreview =
		  trap_R_RegisterShaderNoMip( va( "levelshots/%s", Info_ValueForKey( info, "mapname" ) ) );

		if ( uiInfo.serverStatus.currentServerCinematic >= 0 )
		{
			trap_CIN_StopCinematic( uiInfo.serverStatus.currentServerCinematic );
			uiInfo.serverStatus.currentServerCinematic = -1;
		}

		mapName = Info_ValueForKey( info, "mapname" );

		if ( mapName && *mapName )
		{
			uiInfo.serverStatus.currentServerCinematic =
			  trap_CIN_PlayCinematic( va( "%s.roq", mapName ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
		}
	}
	else if ( feederID == FEEDER_SERVERSTATUS )
	{
	}
	else if ( feederID == FEEDER_FINDPLAYER )
	{
		uiInfo.currentFoundPlayerServer = index;
		//

		if ( index < uiInfo.numFoundPlayerServers - 1 )
		{
			// build a new server status for this server
			Q_strncpyz( uiInfo.serverStatusAddress,
			            uiInfo.foundPlayerServerAddresses[ uiInfo.currentFoundPlayerServer ],
			            sizeof( uiInfo.serverStatusAddress ) );
			Menu_SetFeederSelection( NULL, FEEDER_SERVERSTATUS, 0, NULL );
			UI_BuildServerStatus( qtrue );
		}
	}
	else if ( feederID == FEEDER_PLAYER_LIST )
	{
		uiInfo.playerIndex = index;
	}
	else if ( feederID == FEEDER_TEAM_LIST )
	{
		uiInfo.teamPlayerIndex = index;
	}
	else if ( feederID == FEEDER_IGNORE_LIST )
	{
		uiInfo.ignoreIndex = index;
	}
	else if ( feederID == FEEDER_HELP_LIST )
	{
		uiInfo.helpIndex = index;
	}
	else if ( feederID == FEEDER_MODS )
	{
		uiInfo.modIndex = index;
	}
	else if ( feederID == FEEDER_CINEMATICS )
	{
		uiInfo.movieIndex = index;

		if ( uiInfo.previewMovie >= 0 )
		{
			trap_CIN_StopCinematic( uiInfo.previewMovie );
		}

		uiInfo.previewMovie = -1;
	}
	else if ( feederID == FEEDER_DEMOS )
	{
		uiInfo.demoIndex = index;
	}
	else if ( feederID == FEEDER_TREMTEAMS )
	{
		uiInfo.teamIndex = index;
	}
	else if ( feederID == FEEDER_TREMHUMANITEMS )
	{
		uiInfo.humanItemIndex = index;
	}
	else if ( feederID == FEEDER_TREMALIENCLASSES )
	{
		uiInfo.alienClassIndex = index;
	}
	else if ( feederID == FEEDER_TREMHUMANARMOURYBUY )
	{
		uiInfo.humanArmouryBuyIndex = index;
	}
	else if ( feederID == FEEDER_TREMHUMANARMOURYSELL )
	{
		uiInfo.humanArmourySellIndex = index;
	}
	else if ( feederID == FEEDER_TREMALIENUPGRADE )
	{
		uiInfo.alienUpgradeIndex = index;
	}
	else if ( feederID == FEEDER_TREMALIENBUILD )
	{
		uiInfo.alienBuildIndex = index;
	}
	else if ( feederID == FEEDER_TREMHUMANBUILD )
	{
		uiInfo.humanBuildIndex = index;
	}
	else if ( feederID == FEEDER_LANGUAGES )
	{
		trap_Cvar_Set( "language", uiInfo.languages[ index ].lang );
		uiInfo.languageIndex = index;
	}
	else if ( feederID == FEEDER_PROFILES )
	{
		uiInfo.profileIndex = index;
	}
	else if ( feederID == FEEDER_RESOLUTIONS )
	{
		if ( index >= 0 && index < uiInfo.numResolutions )
		{
			trap_Cvar_Set( "r_customwidth", va( "%d", uiInfo.resolutions[ index ].w ) );
			trap_Cvar_Set( "r_customheight", va( "%d", uiInfo.resolutions[ index ].h ) );
			trap_Cvar_Set( "r_mode", "-1" );
		}

		uiInfo.resolutionIndex = index;
	}
}

static int UI_FeederInitialise( int feederID )
{
	if ( feederID == FEEDER_RESOLUTIONS )
	{
		int i;
		glconfig_t config;
		int w, h;

		trap_GetGlconfig( &config );
		w = config.vidWidth;
		h = config.vidHeight;

		for ( i = 0; i < uiInfo.numResolutions; i++ )
		{
			if ( w == uiInfo.resolutions[ i ].w && h == uiInfo.resolutions[ i ].h )
			{
				return i;
			}
		}

		return uiInfo.numResolutions;
	}
	
	if ( feederID == FEEDER_LANGUAGES )
	{
		int i;
		char lang[25];
		
		trap_Cvar_VariableStringBuffer( "language", lang, sizeof( lang ) );
		
		for ( i = 0; i < uiInfo.numLanguages; i++ )
		{
			if( !Q_stricmp( lang, uiInfo.languages[ i ].lang ) )
			{
				return i;
			}
		}
	}

	return 0;
}

static void UI_Pause( qboolean b )
{
	if ( b )
	{
		// pause the game and set the ui keycatcher
		trap_Cvar_Set( "cl_paused", "1" );
		trap_Key_SetCatcher( KEYCATCH_UI );
	}
	else
	{
		// unpause the game and clear the ui keycatcher
		trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
		trap_Key_ClearStates();
		trap_Cvar_Set( "cl_paused", "0" );
	}
}

static int UI_PlayCinematic( const char *name, float x, float y, float w, float h )
{
	return trap_CIN_PlayCinematic( name, x, y, w, h, ( CIN_loop | CIN_silent ) );
}

static void UI_StopCinematic( int handle )
{
	if ( handle >= 0 )
	{
		trap_CIN_StopCinematic( handle );
	}
	else
	{
		handle = abs( handle );

		if ( handle == UI_NETMAPCINEMATIC )
		{
			if ( uiInfo.serverStatus.currentServerCinematic >= 0 )
			{
				trap_CIN_StopCinematic( uiInfo.serverStatus.currentServerCinematic );
				uiInfo.serverStatus.currentServerCinematic = -1;
			}
		}
	}
}

static void UI_DrawCinematic( int handle, float x, float y, float w, float h )
{
	trap_CIN_SetExtents( handle, x, y, w, h );
	trap_CIN_DrawCinematic( handle );
}

static void UI_RunCinematicFrame( int handle )
{
	trap_CIN_RunCinematic( handle );
}

static float UI_GetValue( int ownerDraw )
{
	return 0.0f;
}

/*
=================
UI_ParseResolutions
=================
*/
void UI_ParseResolutions( void )
{
	char        buf[ MAX_STRING_CHARS ];
	char        w[ 16 ], h[ 16 ];
	char        *p;
	const char  *out;
	char        *s = NULL;

	trap_Cvar_VariableStringBuffer( "r_availableModes", buf, sizeof( buf ) );
	p = buf;
	uiInfo.numResolutions = 0;

	while ( String_Parse( &p, &out ) )
	{
		Q_strncpyz( w, out, sizeof( w ) );
		s = strchr( w, 'x' );

		if ( !s )
		{
			return;
		}

		*s++ = '\0';
		Q_strncpyz( h, s, sizeof( h ) );

		uiInfo.resolutions[ uiInfo.numResolutions ].w = atoi( w );
		uiInfo.resolutions[ uiInfo.numResolutions ].h = atoi( h );
		uiInfo.numResolutions++;
	}
}

/*
=================
UI_ParseLanguages
=================
*/
void UI_ParseLanguages( void )
{
	char        buf[ MAX_STRING_CHARS ], temp[ MAX_TOKEN_CHARS ];
	int         index = 0, lang = 0;
	qboolean    quoted = qfalse;
	char        *p;
	
	trap_Cvar_VariableStringBuffer( "trans_languages", buf, sizeof( buf ) );
	p = buf;
	memset( &temp, 0, sizeof( temp ) );
	while( p && *p )
	{
		if( *p == '"' && quoted )
		{
			uiInfo.languages[ lang++ ].name = String_Alloc( temp );
			quoted = qfalse;
			index = 0;
		}
			
		else if( *p == '"' || quoted )
		{
			if( !quoted ) { p++; }
			quoted = qtrue;
			temp[ index++ ] = *p;
		}
		p++;
	}
	trap_Cvar_VariableStringBuffer( "trans_encodings", buf, sizeof( buf ) );
	p = buf;
	memset( &temp, 0, sizeof( temp ) );
	lang = 0;
	while( p && *p )
	{
		if( *p == '"' && quoted )
		{
			uiInfo.languages[ lang++ ].lang = String_Alloc( temp );
			quoted = qfalse;
			index = 0;
		}
			
		else if( *p == '"' || quoted )
		{
			if( !quoted ) { p++; }
			quoted = qtrue;
			temp[ index++ ] = *p;
		}
		p++;
	}
	
	uiInfo.numLanguages = lang;
}
/*
=================
UI_Init
=================
*/
void UI_Init( qboolean inGameLoad )
{
	int start;

	trap_SyscallABIVersion( SYSCALL_ABI_VERSION_MAJOR, SYSCALL_ABI_VERSION_MINOR );

	BG_InitClassConfigs();
	BG_InitAllowedGameElements();

	uiInfo.inGameLoad = inGameLoad;

	UI_RegisterCvars();
	UI_InitMemory();

	// cache redundant calulations
	trap_GetGlconfig( &uiInfo.uiDC.glconfig );

	// for 640x480 virtualized screen
	uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * ( 1.0f / 480.0f );
	uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * ( 1.0f / 640.0f );

	// wide screen
	uiInfo.uiDC.aspectScale = ( ( 640.0f * uiInfo.uiDC.glconfig.vidHeight ) /
	                            ( 480.0f * uiInfo.uiDC.glconfig.vidWidth ) );

	uiInfo.uiDC.smallFontScale = trap_Cvar_VariableValue( "ui_smallFont" );
	uiInfo.uiDC.bigFontScale = trap_Cvar_VariableValue( "ui_bigFont" );

	uiInfo.uiDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	uiInfo.uiDC.setColor = &UI_SetColor;
	uiInfo.uiDC.drawHandlePic = &UI_DrawHandlePic;
	uiInfo.uiDC.drawNoStretchPic = &UI_DrawNoStretchPic;
	uiInfo.uiDC.drawStretchPic = &trap_R_DrawStretchPic;
	uiInfo.uiDC.registerModel = &trap_R_RegisterModel;
	uiInfo.uiDC.modelBounds = &trap_R_ModelBounds;
	uiInfo.uiDC.fillRect = &UI_FillRect;
	uiInfo.uiDC.drawRect = &UI_DrawRect;
	uiInfo.uiDC.drawSides = &UI_DrawSides;
	uiInfo.uiDC.drawTopBottom = &UI_DrawTopBottom;
	uiInfo.uiDC.clearScene = &trap_R_ClearScene;
	uiInfo.uiDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	uiInfo.uiDC.renderScene = &trap_R_RenderScene;
	uiInfo.uiDC.registerFont = &trap_R_RegisterFont;
	uiInfo.uiDC.glyph = &UI_R_Glyph;
	uiInfo.uiDC.glyphChar = &UI_R_GlyphChar;
	uiInfo.uiDC.freeCachedGlyphs = &UI_R_UnregisterFont;
	uiInfo.uiDC.ownerDrawItem = &UI_OwnerDraw;
	uiInfo.uiDC.getValue = &UI_GetValue;
	uiInfo.uiDC.ownerDrawVisible = &UI_OwnerDrawVisible;
	uiInfo.uiDC.runScript = &UI_RunMenuScript;
	uiInfo.uiDC.setCVar = trap_Cvar_Set;
	uiInfo.uiDC.getCVarString = trap_Cvar_VariableStringBuffer;
	uiInfo.uiDC.getCVarValue = trap_Cvar_VariableValue;
	uiInfo.uiDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	uiInfo.uiDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	uiInfo.uiDC.startLocalSound = &trap_S_StartLocalSound;
	uiInfo.uiDC.ownerDrawHandleKey = &UI_OwnerDrawHandleKey;
	uiInfo.uiDC.feederCount = &UI_FeederCount;
	uiInfo.uiDC.feederItemImage = &UI_FeederItemImage;
	uiInfo.uiDC.feederItemText = &UI_FeederItemText;
	uiInfo.uiDC.feederSelection = &UI_FeederSelection;
	uiInfo.uiDC.feederInitialise = &UI_FeederInitialise;
	uiInfo.uiDC.setBinding = &trap_Key_SetBinding;
	uiInfo.uiDC.getBindingBuf = &trap_Key_GetBindingBuf;
	uiInfo.uiDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	uiInfo.uiDC.executeText = &trap_Cmd_ExecuteText;
	uiInfo.uiDC.Error = &Com_Error;
	uiInfo.uiDC.Print = &Com_Printf;
	uiInfo.uiDC.Pause = &UI_Pause;
	uiInfo.uiDC.ownerDrawWidth = &UI_OwnerDrawWidth;
	uiInfo.uiDC.ownerDrawText = &UI_OwnerDrawText;
	uiInfo.uiDC.registerSound = &trap_S_RegisterSound;
	uiInfo.uiDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	uiInfo.uiDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	uiInfo.uiDC.playCinematic = &UI_PlayCinematic;
	uiInfo.uiDC.stopCinematic = &UI_StopCinematic;
	uiInfo.uiDC.drawCinematic = &UI_DrawCinematic;
	uiInfo.uiDC.runCinematicFrame = &UI_RunCinematicFrame;

	Init_Display( &uiInfo.uiDC );

	String_Init();

	uiInfo.uiDC.whiteShader = trap_R_RegisterShaderNoMip( "white" );

	AssetCache();

	start = trap_Milliseconds();

	UI_LoadMenus( "ui/menus.txt", qtrue );
	UI_LoadMenus( "ui/ingame.txt", qfalse );
	UI_LoadMenus( "ui/tremulous.txt", qfalse );
	UI_LoadHelp( "ui/help.txt" );

	Menus_CloseAll();

	trap_LAN_LoadCachedServers();

	// sets defaults for ui temp cvars
	trap_Cvar_Set( "ui_mousePitch", ( trap_Cvar_VariableValue( "m_pitch" ) >= 0 ) ? "0" : "1" );

	uiInfo.serverStatus.currentServerCinematic = -1;
	uiInfo.previewMovie = -1;

	UI_ParseResolutions();
	UI_ParseLanguages();
}

/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent( int key, int chr, int flags )
{
	if ( Menu_Count() > 0 )
	{
		menuDef_t *menu = Menu_GetFocused();

		if ( menu )
		{
			if ( key == K_ESCAPE && ( flags & ( 1 << KEYEVSTATE_DOWN ) ) && !Menus_AnyFullScreenVisible() )
			{
				Menus_CloseAll();
			}
			else
			{
				Menu_HandleKey( menu, key, chr, !!( flags & ( 1 << KEYEVSTATE_DOWN ) ) );
			}
		}
		else
		{
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			trap_Cvar_Set( "cl_paused", "0" );
		}
	}
}

/*
=================
UI_MouseEvent
=================
*/
void UI_MouseEvent( int dx, int dy )
{
	// update mouse screen position
	uiInfo.uiDC.cursorx += ( dx * uiInfo.uiDC.aspectScale );

	if ( uiInfo.uiDC.cursorx < 0 )
	{
		uiInfo.uiDC.cursorx = 0;
	}
	else if ( uiInfo.uiDC.cursorx > SCREEN_WIDTH )
	{
		uiInfo.uiDC.cursorx = SCREEN_WIDTH;
	}

	uiInfo.uiDC.cursory += dy;

	if ( uiInfo.uiDC.cursory < 0 )
	{
		uiInfo.uiDC.cursory = 0;
	}
	else if ( uiInfo.uiDC.cursory > SCREEN_HEIGHT )
	{
		uiInfo.uiDC.cursory = SCREEN_HEIGHT;
	}

	uiInfo.uiDC.cursordx = dx * uiInfo.uiDC.aspectScale;
	uiInfo.uiDC.cursordy = dy;

	if ( Menu_Count() > 0 )
	{
		Display_MouseMove( NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory );
	}
}

/*
=================
UI_MousePosition
=================
*/
int UI_MousePosition( void )
{
	return ( int ) rint( uiInfo.uiDC.cursorx ) |
	       ( int ) rint( uiInfo.uiDC.cursory ) << 16;
}

/*
=================
UI_SetMousePosition
=================
*/
void UI_SetMousePosition( int x, int y )
{
	uiInfo.uiDC.cursorx = x;
	uiInfo.uiDC.cursory = y;

	if ( Menu_Count() > 0 )
	{
		Display_MouseMove( NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory );
	}
}

void UI_SetActiveMenu( uiMenuCommand_t menu )
{
	char buf[ 256 ];

	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached

	if ( Menu_Count() > 0 )
	{
		vec3_t v;
		v[ 0 ] = v[ 1 ] = v[ 2 ] = 0;

		switch ( menu )
		{
			case UIMENU_NONE:
				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();

				return;

			case UIMENU_MAIN:
				trap_Cvar_Set( "sv_killserver", "1" );
				trap_Key_SetCatcher( KEYCATCH_UI );
				Menus_CloseAll();
				Menus_ActivateByName( "main" );
				if( !cl_profile.string[0] )
				{
					Menus_ActivateByName( "profile_firstrun" );
				}
				buf[ 0 ] = '\0';
				trap_Cvar_VariableStringBuffer( "com_errorMessage", buf, sizeof( buf ) );

				if ( strlen( buf ) )
				{
					if ( trap_Cvar_VariableValue( "com_errorCode" ) == ERR_SERVERDISCONNECT )
					{
						Menus_ActivateByName( "drop_popmenu" );
					}
					else
					{
						Menus_ActivateByName( "error_popmenu" );
					}
				}

				return;

			case UIMENU_INGAME:
				trap_Cvar_Set( "cl_paused", "1" );
				trap_Key_SetCatcher( KEYCATCH_UI );
				UI_BuildPlayerList();
				Menus_CloseAll();
				Menus_ActivateByName( "ingame" );

				return;

			default:
				return;
		}
	}
}

qboolean UI_IsFullscreen( void )
{
	return Menus_AnyFullScreenVisible();
}

static connstate_t  lastConnState;
static char     lastLoadingText[ MAX_INFO_VALUE ];

static void UI_ReadableSize( char *buf, int bufsize, int value )
{
	if ( value > 1024 * 1024 * 1024 )
	{
		// gigs
		Com_sprintf( buf, bufsize, "%d", value / ( 1024 * 1024 * 1024 ) );
		Com_sprintf( buf + strlen( buf ), bufsize - strlen( buf ), ".%02d GB",
		             ( value % ( 1024 * 1024 * 1024 ) ) * 100 / ( 1024 * 1024 * 1024 ) );
	}
	else if ( value > 1024 * 1024 )
	{
		// megs
		Com_sprintf( buf, bufsize, "%d", value / ( 1024 * 1024 ) );
		Com_sprintf( buf + strlen( buf ), bufsize - strlen( buf ), ".%02d MB",
		             ( value % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
	}
	else if ( value > 1024 )
	{
		// kilos
		Com_sprintf( buf, bufsize, "%d KB", value / 1024 );
	}
	else
	{
		// bytes
		Com_sprintf( buf, bufsize, "%d bytes", value );
	}
}

// Assumes time is in msec
static void UI_PrintTime( char *buf, int bufsize, int time )
{
	time /= 1000; // change to seconds

	if ( time > 3600 )
	{
		// in the hours range
		Com_sprintf( buf, bufsize, "%d hr %d min", time / 3600, ( time % 3600 ) / 60 );
	}
	else if ( time > 60 )
	{
		// mins
		Com_sprintf( buf, bufsize, "%d min %d sec", time / 60, time % 60 );
	}
	else
	{
		// secs
		Com_sprintf( buf, bufsize, "%d sec", time );
	}
}

// FIXME: move to ui_shared.c?
void Text_PaintCenter( float x, float y, float scale, vec4_t color, const char *text, float adjust )
{
	int len = UI_Text_Width( text, scale );
	UI_Text_Paint( x - len / 2, y, scale, color, text, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );
}

void Text_PaintCenter_AutoWrapped( float x, float y, float xmax, float ystep, float scale, vec4_t color, const char *str, float adjust )
{
	int width;
	char *s1, *s2, *s3;
	char c_bcp;
	char buf[ 1024 ];

	if ( !str || str[ 0 ] == '\0' )
	{
		return;
	}

	Q_strncpyz( buf, str, sizeof( buf ) );

	s1 = s2 = s3 = buf;

	while ( 1 )
	{
		do
		{
			s3++;
		}
		while ( *s3 != ' ' && *s3 != '\0' );

		c_bcp = *s3;

		*s3 = '\0';

		width = UI_Text_Width( s1, scale );

		*s3 = c_bcp;

		if ( width > xmax )
		{
			if ( s1 == s2 )
			{
				// fuck, don't have a clean cut, we'll overflow
				s2 = s3;
			}

			*s2 = '\0';
			Text_PaintCenter( x, y, scale, color, s1, adjust );
			y += ystep;

			if ( c_bcp == '\0' )
			{
				// that was the last word
				// we could start a new loop, but that wouldn't be much use
				// even if the word is too long, we would overflow it (see above)
				// so just print it now if needed
				s2++;

				if ( *s2 != '\0' ) // if we are printing an overflowing line we have s2 == s3
				{
					Text_PaintCenter( x, y, scale, color, s2, adjust );
				}

				break;
			}

			s2++;
			s1 = s2;
			s3 = s2;
		}
		else
		{
			s2 = s3;

			if ( c_bcp == '\0' ) // we reached the end
			{
				Text_PaintCenter( x, y, scale, color, s1, adjust );
				break;
			}
		}
	}
}

static void UI_DisplayDownloadInfo( const char *downloadName, float centerPoint, float yStart, float scale )
{
	static const char dlText[] = "Downloading:";
	static const char etaText[] = "Estimated time left:";
	static const char xferText[] = "Transfer rate:";

	int downloadSize, downloadCount, downloadTime;
	char dlSizeBuf[ 64 ], totalSizeBuf[ 64 ], xferRateBuf[ 64 ], dlTimeBuf[ 64 ];
	int xferRate;
	int leftWidth;
	const char *s;

	downloadSize = trap_Cvar_VariableValue( "cl_downloadSize" );
	downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );
	downloadTime = trap_Cvar_VariableValue( "cl_downloadTime" );

	leftWidth = 320;

	UI_SetColor( colorWhite );
	Text_PaintCenter( centerPoint, yStart + 112, scale, colorWhite, dlText, 0 );
	Text_PaintCenter( centerPoint, yStart + 192, scale, colorWhite, etaText, 0 );
	Text_PaintCenter( centerPoint, yStart + 248, scale, colorWhite, xferText, 0 );

	if ( downloadSize > 0 )
	{
		s = va( "%s (%d%%)", downloadName, ( int )( ( float ) downloadCount * 100.0f / downloadSize ) );
	}
	else
	{
		s = downloadName;
	}

	Text_PaintCenter( centerPoint, yStart + 136, scale, colorWhite, s, 0 );

	UI_ReadableSize( dlSizeBuf,   sizeof dlSizeBuf,   downloadCount );
	UI_ReadableSize( totalSizeBuf,  sizeof totalSizeBuf,  downloadSize );

	if ( downloadCount < 4096 || !downloadTime )
	{
		Text_PaintCenter( leftWidth, yStart + 216, scale, colorWhite, "estimating", 0 );
		Text_PaintCenter( leftWidth, yStart + 160, scale, colorWhite, va( "(%s of %s copied)", dlSizeBuf, totalSizeBuf ), 0 );
	}
	else
	{
		if ( ( uiInfo.uiDC.realTime - downloadTime ) / 1000 )
		{
			xferRate = downloadCount / ( ( uiInfo.uiDC.realTime - downloadTime ) / 1000 );
		}
		else
		{
			xferRate = 0;
		}

		UI_ReadableSize( xferRateBuf, sizeof xferRateBuf, xferRate );

		// Extrapolate estimated completion time

		if ( downloadSize && xferRate )
		{
			int n = downloadSize / xferRate; // estimated time for entire d/l in secs

			// We do it in K (/1024) because we'd overflow around 4MB
			UI_PrintTime( dlTimeBuf, sizeof dlTimeBuf,
			              ( n - ( ( ( downloadCount / 1024 ) * n ) / ( downloadSize / 1024 ) ) ) * 1000 );

			Text_PaintCenter( leftWidth, yStart + 216, scale, colorWhite, dlTimeBuf, 0 );
			Text_PaintCenter( leftWidth, yStart + 160, scale, colorWhite, va( "(%s of %s copied)", dlSizeBuf, totalSizeBuf ), 0 );
		}
		else
		{
			Text_PaintCenter( leftWidth, yStart + 216, scale, colorWhite, "estimating", 0 );

			if ( downloadSize )
			{
				Text_PaintCenter( leftWidth, yStart + 160, scale, colorWhite, va( "(%s of %s copied)", dlSizeBuf, totalSizeBuf ), 0 );
			}
			else
			{
				Text_PaintCenter( leftWidth, yStart + 160, scale, colorWhite, va( "(%s copied)", dlSizeBuf ), 0 );
			}
		}

		if ( xferRate )
		{
			Text_PaintCenter( leftWidth, yStart + 272, scale, colorWhite, va( "%s/Sec", xferRateBuf ), 0 );
		}
	}
}

/*
========================
UI_DrawConnectScreen
========================
*/
void UI_DrawConnectScreen( qboolean overlay )
{
	char      *s;
	uiClientState_t cstate;
	char      info[ MAX_INFO_VALUE ];
	char text[ 256 ];
	float centerPoint, yStart, scale;

	menuDef_t *menu = Menus_FindByName( "Connect" );

	if ( !overlay && menu )
	{
		Menu_Paint( menu, qtrue );
	}

	if ( !overlay )
	{
		centerPoint = 320;
		yStart = 130;
		scale = 0.5f;
	}
	else
	{
		centerPoint = 320;
		yStart = 32;
		scale = 0.6f;
		return;
	}

	// see what information we should display
	trap_GetClientState( &cstate );

	info[ 0 ] = '\0';

	if ( trap_GetConfigString( CS_SERVERINFO, info, sizeof( info ) ) )
	{
		Text_PaintCenter( centerPoint, yStart, scale, colorWhite, va( "Loading %s", Info_ValueForKey( info, "mapname" ) ), 0 );
	}

	if ( !Q_stricmp( cstate.servername, "localhost" ) )
	{
		Text_PaintCenter( centerPoint, yStart + 48, scale, colorWhite,
		                  "Starting up…", ITEM_TEXTSTYLE_SHADOWEDMORE );
	}
	else
	{
		Com_sprintf( text, sizeof( text ), "Connecting to %s", cstate.servername );
		Text_PaintCenter( centerPoint, yStart + 48, scale, colorWhite, text, ITEM_TEXTSTYLE_SHADOWEDMORE );
	}

	// display global MOTD at bottom
	Text_PaintCenter( centerPoint, 600, scale, colorWhite, Info_ValueForKey( cstate.updateInfoString, "motd" ), 0 );

	// print any server info (server full, bad version, etc)
	if ( cstate.connState < CA_CONNECTED )
	{
		Text_PaintCenter( centerPoint, yStart + 176, scale, colorWhite, cstate.messageString, 0 );
	}

	if ( lastConnState > cstate.connState )
	{
		lastLoadingText[ 0 ] = '\0';
	}

	lastConnState = cstate.connState;

	switch ( cstate.connState )
	{
		case CA_CONNECTING:
			s = va( "Awaiting connection…%i", cstate.connectPacketCount );
			break;

		case CA_CHALLENGING:
			s = va( "Awaiting challenge…%i", cstate.connectPacketCount );
			break;

		case CA_CONNECTED:
			{
				char downloadName[ MAX_INFO_VALUE ];

				trap_Cvar_VariableStringBuffer( "cl_downloadName", downloadName, sizeof( downloadName ) );

				if ( *downloadName )
				{
					UI_DisplayDownloadInfo( downloadName, centerPoint, yStart, scale );
					return;
				}
			}

			s = "Awaiting gamestate…";
			break;

		case CA_LOADING:
			return;

		case CA_PRIMED:
			return;

		default:
			return;
	}

	if ( Q_stricmp( cstate.servername, "localhost" ) )
	{
		Text_PaintCenter( centerPoint, yStart + 80, scale, colorWhite, s, 0 );
	}

	// password required / connection rejected information goes here
}

/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars( void )
{
	int     i;
	const cvarTable_t *cv;

	for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
	{
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
	}
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars( void )
{
	int     i;
	const cvarTable_t *cv;

	for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
	{
		trap_Cvar_Update( cv->vmCvar );
	}
}

/*
=================
UI_UpdateNews
=================
*/
void UI_UpdateNews( qboolean begin )
{
	char newsString[ MAX_NEWS_STRING ];
	const char *c;
	const char *wrapped;
	int line = 0;
	int linePos = 0;
	qboolean finished;

	if ( begin && !uiInfo.newsInfo.refreshActive )
	{
		uiInfo.newsInfo.refreshtime = uiInfo.uiDC.realTime + 10000;
		uiInfo.newsInfo.refreshActive = qtrue;
	}
	else if ( !uiInfo.newsInfo.refreshActive ) // do nothing
	{
		return;
	}
	else if ( uiInfo.uiDC.realTime > uiInfo.newsInfo.refreshtime )
	{
		strcpy( uiInfo.newsInfo.text[ 0 ],
		        "^1Error: Timed out while contacting the server." );
		uiInfo.newsInfo.numLines = 1;
		return;
	}

	// start the news fetching
	finished = trap_GetNews( begin );

	// parse what comes back. Parse newlines and otherwise chop when necessary
	trap_Cvar_VariableStringBuffer( "cl_newsString", newsString,
	                                sizeof( newsString ) );

	// FIXME remove magic width constant
	wrapped = Item_Text_Wrap( newsString, 0.25f, 325 * uiInfo.uiDC.aspectScale );

	for ( c = wrapped; *c != '\0'; ++c )
	{
		if ( linePos == ( MAX_NEWS_LINEWIDTH - 1 ) || *c == '\n' )
		{
			uiInfo.newsInfo.text[ line ][ linePos ] = '\0';

			if ( line == ( MAX_NEWS_LINES  - 1 ) )
			{
				break;
			}

			linePos = 0;
			line++;

			if ( *c != '\n' )
			{
				uiInfo.newsInfo.text[ line ][ linePos ] = *c;
				linePos++;
			}
		}
		else if ( isprint( *c ) )
		{
			uiInfo.newsInfo.text[ line ][ linePos ] = *c;
			linePos++;
		}
	}

	uiInfo.newsInfo.text[ line ] [ linePos ] = '\0';
	uiInfo.newsInfo.numLines = line + 1;

	if ( finished )
	{
		uiInfo.newsInfo.refreshActive = qfalse;
	}
}

const char *gettext( const char *msgid )
{
	QVM_STATIC char buffer[ 32000 ];
	char *buf = buffer;
	trap_Gettext( buf, msgid, sizeof( buffer ) );
	return buf;
}
