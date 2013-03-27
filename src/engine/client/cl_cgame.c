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

// cl_cgame.c  -- client system interaction with client game

#include "client.h"

#ifdef USE_MUMBLE
#include "libmumblelink.h"
#endif

#include "../qcommon/crypto.h"

#define __(x) Trans_GettextGame(x)
#define C__(x, y) Trans_PgettextGame(x, y)
#define P__(x, y, c) Trans_GettextGamePlural(x, y, c)

static void ( *completer )( const char *s ) = NULL;

// NERVE - SMF
void                   Key_GetBindingBuf( int keynum, int team, char *buf, int buflen );
void                   Key_KeynumToStringBuf( int keynum, char *buf, int buflen );

// -NERVE - SMF

// ydnar: can we put this in a header, pls?
void Key_GetBindingByString( const char *binding, int team, int *key1, int *key2 );

/*
====================
CL_GetGameState
====================
*/
void CL_GetGameState( gameState_t *gs )
{
	*gs = cl.gameState;
}

/*
====================
CL_GetGlconfig
====================
*/
void CL_GetGlconfig( glconfig_t *glconfig )
{
	*glconfig = cls.glconfig;
}

/*
====================
CL_GetUserCmd
====================
*/
qboolean CL_GetUserCmd( int cmdNumber, usercmd_t *ucmd )
{
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if ( cmdNumber > cl.cmdNumber )
	{
		Com_Error( ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber );
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if ( cmdNumber <= cl.cmdNumber - CMD_BACKUP )
	{
		return qfalse;
	}

	*ucmd = cl.cmds[ cmdNumber & CMD_MASK ];

	return qtrue;
}

int CL_GetCurrentCmdNumber( void )
{
	return cl.cmdNumber;
}

/*
====================
CL_GetParseEntityState
====================
*/
qboolean CL_GetParseEntityState( int parseEntityNumber, entityState_t *state )
{
	// can't return anything that hasn't been parsed yet
	if ( parseEntityNumber >= cl.parseEntitiesNum )
	{
		Com_Error( ERR_DROP, "CL_GetParseEntityState: %i >= %i", parseEntityNumber, cl.parseEntitiesNum );
	}

	// can't return anything that has been overwritten in the circular buffer
	if ( parseEntityNumber <= cl.parseEntitiesNum - MAX_PARSE_ENTITIES )
	{
		return qfalse;
	}

	*state = cl.parseEntities[ parseEntityNumber & ( MAX_PARSE_ENTITIES - 1 ) ];
	return qtrue;
}

/*
====================
CL_GetCurrentSnapshotNumber
====================
*/
void CL_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime )
{
	*snapshotNumber = cl.snap.messageNum;
	*serverTime = cl.snap.serverTime;
}

/*
====================
CL_GetSnapshot
====================
*/
qboolean CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot )
{
	clSnapshot_t *clSnap;
	int          i, count;

	if ( snapshotNumber > cl.snap.messageNum )
	{
		Com_Error( ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum" );
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if ( cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP )
	{
		return qfalse;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.snapshots[ snapshotNumber & PACKET_MASK ];

	if ( !clSnap->valid )
	{
		return qfalse;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if ( cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES )
	{
		return qfalse;
	}

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	memcpy( snapshot->areamask, clSnap->areamask, sizeof( snapshot->areamask ) );
	snapshot->ps = clSnap->ps;
	count = clSnap->numEntities;

	if ( count > MAX_ENTITIES_IN_SNAPSHOT )
	{
		Com_DPrintf( "CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}

	snapshot->numEntities = count;

	for ( i = 0; i < count; i++ )
	{
		snapshot->entities[ i ] = cl.parseEntities[( clSnap->parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 ) ];
	}

	// FIXME: configstring changes and server commands!!!

	return qtrue;
}

/*
==============
CL_SetUserCmdValue
==============
*/
void CL_SetUserCmdValue( int userCmdValue, int flags, float sensitivityScale, int mpIdentClient )
{
	cl.cgameUserCmdValue = userCmdValue;
	cl.cgameFlags = flags;
	cl.cgameSensitivity = sensitivityScale;
	cl.cgameMpIdentClient = mpIdentClient; // NERVE - SMF
}

/*
==================
CL_SetClientLerpOrigin
==================
*/
void CL_SetClientLerpOrigin( float x, float y, float z )
{
	cl.cgameClientLerpOrigin[ 0 ] = x;
	cl.cgameClientLerpOrigin[ 1 ] = y;
	cl.cgameClientLerpOrigin[ 2 ] = z;
}

/*
=====================
CL_CompleteCgameCommand
=====================
*/
void CL_CompleteCgameCommand( char *args, int argNum )
{
	Field_CompleteCgame( argNum );
}

/*
=====================
CL_CgameCompletion
=====================
*/
void CL_CgameCompletion( void ( *callback )( const char *s ), int argNum )
{
	completer = callback;
	VM_Call( cgvm, CG_COMPLETE_COMMAND, argNum );
	completer = NULL;
}

/*
==============
CL_AddCgameCommand
==============
*/
void CL_AddCgameCommand( const char *cmdName )
{
	Cmd_AddCommand( cmdName, NULL );
	Cmd_SetCommandCompletionFunc( cmdName, CL_CompleteCgameCommand );
}

/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified( void )
{
	const char  *old, *s;
	int         i, index;
	const char  *dup;
	gameState_t oldGs;
	int         len;

	index = atoi( Cmd_Argv( 1 ) );

	if ( index < 0 || index >= MAX_CONFIGSTRINGS )
	{
		Com_Error( ERR_DROP, "CL_ConfigstringModified: bad index %i", index );
	}

//  s = Cmd_Argv(2);
	// get everything after "cs <num>"
	//s = Cmd_ArgsFrom( 2 );
	s = Cmd_Argv( 2 );

	old = cl.gameState.stringData + cl.gameState.stringOffsets[ index ];

	if ( !strcmp( old, s ) )
	{
		return; // unchanged
	}

	// build the new gameState_t
	oldGs = cl.gameState;

	memset( &cl.gameState, 0, sizeof( cl.gameState ) );

	// leave the first 0 for uninitialized strings
	cl.gameState.dataCount = 1;

	for ( i = 0; i < MAX_CONFIGSTRINGS; i++ )
	{
		if ( i == index )
		{
			dup = s;
		}
		else
		{
			dup = oldGs.stringData + oldGs.stringOffsets[ i ];
		}

		if ( !dup[ 0 ] )
		{
			continue; // leave with the default empty string
		}

		len = strlen( dup );

		if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS )
		{
			Com_Error( ERR_DROP, "MAX_GAMESTATE_CHARS exceeded" );
		}

		// append it to the gameState string buffer
		cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
		memcpy( cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1 );
		cl.gameState.dataCount += len + 1;
	}

	if ( index == CS_SYSTEMINFO )
	{
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}
}

/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
qboolean CL_GetServerCommand( int serverCommandNumber )
{
	const char  *s;
	char        *cmd;
	static char bigConfigString[ BIG_INFO_STRING ];
	int         argc;

	// if we have irretrievably lost a reliable command, drop the connection
	if ( serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS )
	{
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if ( clc.demoplaying )
		{
			return qfalse;
		}

		Com_Error( ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out" );
	}

	if ( serverCommandNumber > clc.serverCommandSequence )
	{
		Com_Error( ERR_DROP, "CL_GetServerCommand: requested a command not received" );
	}

	s = clc.serverCommands[ serverCommandNumber & ( MAX_RELIABLE_COMMANDS - 1 ) ];
	clc.lastExecutedServerCommand = serverCommandNumber;

	if ( cl_showServerCommands->integer )
	{
		// NERVE - SMF
		Com_Printf( "serverCommand: %i : %s\n", serverCommandNumber, s );
	}

rescan:
	Cmd_TokenizeString( s );
	cmd = Cmd_Argv( 0 );
	argc = Cmd_Argc();

	if ( !strcmp( cmd, "disconnect" ) )
	{
		// NERVE - SMF - allow server to indicate why they were disconnected
		if ( argc >= 2 )
		{
			Com_Error( ERR_SERVERDISCONNECT, "Server disconnected: %s", Cmd_Argv( 1 ) );
		}
		else
		{
			Com_Error( ERR_SERVERDISCONNECT, "Server disconnected" );
		}
	}

	if ( !strcmp( cmd, "bcs0" ) )
	{
		Com_sprintf( bigConfigString, BIG_INFO_STRING, "cs %s %s", Cmd_Argv( 1 ), Cmd_QuoteString( Cmd_Argv( 2 ) ) );
		return qfalse;
	}

	if ( !strcmp( cmd, "bcs1" ) )
	{
		s = Cmd_QuoteString( Cmd_Argv( 2 ) );

		if ( strlen( bigConfigString ) + strlen( s ) >= BIG_INFO_STRING )
		{
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}

		strcat( bigConfigString, s );
		return qfalse;
	}

	if ( !strcmp( cmd, "bcs2" ) )
	{
		s = Cmd_QuoteString( Cmd_Argv( 2 ) );

		if ( strlen( bigConfigString ) + strlen( s ) + 1 >= BIG_INFO_STRING )
		{
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}

		strcat( bigConfigString, s );
		strcat( bigConfigString, "\"" );
		s = bigConfigString;
		goto rescan;
	}

	if ( !strcmp( cmd, "cs" ) )
	{
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		return qtrue;
	}

	if ( !strcmp( cmd, "map_restart" ) )
	{
		// clear outgoing commands before passing
		// the restart to the cgame
		memset( cl.cmds, 0, sizeof( cl.cmds ) );
		return qtrue;
	}

	if ( !strcmp( cmd, "popup" ) )
	{
		// direct server to client popup request, bypassing cgame
		if ( cls.state == CA_ACTIVE && !clc.demoplaying )
		{
			Rocket_DocumentAction( Cmd_Argv(1), "open" );
		}
		return qfalse;
	}

	if ( !strcmp( cmd, "pubkey_decrypt" ) )
	{
		char         buffer[ MAX_STRING_CHARS ] = "pubkey_identify ";
		unsigned int msg_len = MAX_STRING_CHARS - 16;
		mpz_t        message;

		if ( argc == 1 )
		{
			Com_Printf("%s", _( "^3Server sent a pubkey_decrypt command, but sent nothing to decrypt!\n" ));
			return qfalse;
		}

		mpz_init_set_str( message, Cmd_Argv( 1 ), 16 );

		if ( rsa_decrypt( &private_key, &msg_len, ( unsigned char * ) buffer + 16, message ) )
		{
			nettle_mpz_set_str_256_u( message, msg_len, ( unsigned char * ) buffer + 16 );
			mpz_get_str( buffer + 16, 16, message );
			CL_AddReliableCommand( buffer );
		}

		mpz_clear( message );
		return qfalse;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return qtrue;
}

// DHM - Nerve :: Copied from server to here

/*
====================
CL_SetExpectedHunkUsage

  Sets com_expectedhunkusage, so the client knows how to draw the percentage bar
====================
*/
void CL_SetExpectedHunkUsage( const char *mapname )
{
	int  handle;
	char *memlistfile = "hunkusage.dat";
	char *buf;
	char *buftrav;
	char *token;
	int  len;

	len = FS_FOpenFileByMode( memlistfile, &handle, FS_READ );

	if ( len >= 0 )
	{
		// the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value
		buf = ( char * ) Z_Malloc( len + 1 );
		memset( buf, 0, len + 1 );

		FS_Read( ( void * ) buf, len, handle );
		FS_FCloseFile( handle );

		// now parse the file, filtering out the current map
		buftrav = buf;

		while ( ( token = COM_Parse( &buftrav ) ) != NULL && token[ 0 ] )
		{
			if ( !Q_stricmp( token, ( char * ) mapname ) )
			{
				// found a match
				token = COM_Parse( &buftrav );  // read the size

				if ( token && *token )
				{
					// this is the usage
					com_expectedhunkusage = atoi( token );
					Z_Free( buf );
					return;
				}
			}
		}

		Z_Free( buf );
	}

	// just set it to a negative number,so the cgame knows not to draw the percent bar
	com_expectedhunkusage = -1;
}

/*
====================
CL_CM_LoadMap

Just adds default parameters that cgame doesn't need to know about
====================
*/
void CL_CM_LoadMap( const char *mapname )
{
	int checksum;

	// DHM - Nerve :: If we are not running the server, then set expected usage here
	if ( !com_sv_running->integer )
	{
		CL_SetExpectedHunkUsage( mapname );
	}
	else
	{
		// TTimo
		// catch here when a local server is started to avoid outdated com_errorDiagnoseIP
		Cvar_Set( "com_errorDiagnoseIP", "" );
	}

	CM_LoadMap( mapname, qtrue, &checksum );
}

/*
====================
CL_ShutdownCGame
====================
*/
void CL_ShutdownCGame( void )
{
	cls.keyCatchers &= ~KEYCATCH_CGAME;
	cls.cgameStarted = qfalse;

	if ( !cgvm )
	{
		return;
	}

	Rocket_Shutdown();
	VM_Call( cgvm, CG_SHUTDOWN );
	VM_Free( cgvm );
	cgvm = NULL;
}

//
// libRocket UI stuff
//

/*
 * ====================
 * GetClientState
 * ====================
 */
static void GetClientState( uiClientState_t *state )
{
	state->connectPacketCount = clc.connectPacketCount;
	state->connState = cls.state;
	Q_strncpyz( state->servername, cls.servername, sizeof( state->servername ) );
	Q_strncpyz( state->updateInfoString, cls.updateInfoString, sizeof( state->updateInfoString ) );
	Q_strncpyz( state->messageString, clc.serverMessage, sizeof( state->messageString ) );
	state->clientNum = cl.snap.ps.clientNum;
}

/*
 * ====================
 * LAN_LoadCachedServers
 * ====================
 */
void LAN_LoadCachedServers( void )
{
	int          size;
	fileHandle_t fileIn;
	char         filename[ MAX_QPATH ];

	cls.numglobalservers = cls.numfavoriteservers = 0;
	cls.numGlobalServerAddresses = 0;

	if ( cl_profile->string[ 0 ] )
	{
		Com_sprintf( filename, sizeof( filename ), "profiles/%s/servercache.dat", cl_profile->string );
	}
	else
	{
		Q_strncpyz( filename, "servercache.dat", sizeof( filename ) );
	}

	// Arnout: moved to mod/profiles dir
	//if (FS_SV_FOpenFileRead(filename, &fileIn)) {
	if ( FS_FOpenFileRead( filename, &fileIn, qtrue ) )
	{
		FS_Read( &cls.numglobalservers, sizeof( int ), fileIn );
		FS_Read( &cls.numfavoriteservers, sizeof( int ), fileIn );
		FS_Read( &size, sizeof( int ), fileIn );

		if ( size == sizeof( cls.globalServers ) + sizeof( cls.favoriteServers ) )
		{
			FS_Read( &cls.globalServers, sizeof( cls.globalServers ), fileIn );
			FS_Read( &cls.favoriteServers, sizeof( cls.favoriteServers ), fileIn );
		}
		else
		{
			cls.numglobalservers = cls.numfavoriteservers = 0;
			cls.numGlobalServerAddresses = 0;
		}

		FS_FCloseFile( fileIn );
	}
}

/*
 * ====================
 * LAN_SaveServersToCache
 * ====================
 */
void LAN_SaveServersToCache( void )
{
	int          size;
	fileHandle_t fileOut;
	char         filename[ MAX_QPATH ];

	if ( cl_profile->string[ 0 ] )
	{
		Com_sprintf( filename, sizeof( filename ), "profiles/%s/servercache.dat", cl_profile->string );
	}
	else
	{
		Q_strncpyz( filename, "servercache.dat", sizeof( filename ) );
	}

	// Arnout: moved to mod/profiles dir
	//fileOut = FS_SV_FOpenFileWrite(filename);
	fileOut = FS_FOpenFileWrite( filename );
	FS_Write( &cls.numglobalservers, sizeof( int ), fileOut );
	FS_Write( &cls.numfavoriteservers, sizeof( int ), fileOut );
	size = sizeof( cls.globalServers ) + sizeof( cls.favoriteServers );
	FS_Write( &size, sizeof( int ), fileOut );
	FS_Write( &cls.globalServers, sizeof( cls.globalServers ), fileOut );
	FS_Write( &cls.favoriteServers, sizeof( cls.favoriteServers ), fileOut );
	FS_FCloseFile( fileOut );
}

/*
 * ====================
 * GetNews
 * ====================
 */
qboolean GetNews( qboolean begin )
{
	if ( begin ) // if not already using curl, start the download
	{
		CL_RequestMotd();
		Cvar_Set( "cl_newsString", "Retrieving…" );
	}

	if ( Cvar_VariableString( "cl_newsString" ) [ 0 ] == 'R' )
	{
		return qfalse;
	}
	else
	{
		return qtrue;
	}
}

/*
 * ====================
 * LAN_ResetPings
 * ====================
 */
static void LAN_ResetPings( int source )
{
	int          count, i;
	serverInfo_t *servers = NULL;

	count = 0;

	switch ( source )
	{
		case AS_LOCAL:
			servers = &cls.localServers[ 0 ];
			count = MAX_OTHER_SERVERS;
			break;

		case AS_GLOBAL:
			servers = &cls.globalServers[ 0 ];
			count = MAX_GLOBAL_SERVERS;
			break;

		case AS_FAVORITES:
			servers = &cls.favoriteServers[ 0 ];
			count = MAX_OTHER_SERVERS;
			break;
	}

	if ( servers )
	{
		for ( i = 0; i < count; i++ )
		{
			servers[ i ].ping = -1;
		}
	}
}

/*
 * ====================
 * LAN_AddServer
 * ====================
 */
static int LAN_AddServer( int source, const char *name, const char *address )
{
	int          max, *count, i;
	netadr_t     adr;
	serverInfo_t *servers = NULL;
	max = MAX_OTHER_SERVERS;
	count = 0;

	switch ( source )
	{
		case AS_LOCAL:
			count = &cls.numlocalservers;
			servers = &cls.localServers[ 0 ];
			break;

		case AS_GLOBAL:
			max = MAX_GLOBAL_SERVERS;
			count = &cls.numglobalservers;
			servers = &cls.globalServers[ 0 ];
			break;

		case AS_FAVORITES:
			count = &cls.numfavoriteservers;
			servers = &cls.favoriteServers[ 0 ];
			break;
	}

	if ( servers && *count < max )
	{
		NET_StringToAdr( address, &adr, NA_UNSPEC );

		for ( i = 0; i < *count; i++ )
		{
			if ( NET_CompareAdr( servers[ i ].adr, adr ) )
			{
				break;
			}
		}

		if ( i >= *count )
		{
			servers[ *count ].adr = adr;
			Q_strncpyz( servers[ *count ].hostName, name, sizeof( servers[ *count ].hostName ) );
			servers[ *count ].visible = qtrue;
			( *count ) ++;
			return 1;
		}

		return 0;
	}

	return -1;
}

/*
 * ====================
 * LAN_RemoveServer
 * ====================
 */
static void LAN_RemoveServer( int source, const char *addr )
{
	int          *count, i;
	serverInfo_t *servers = NULL;
	count = 0;

	switch ( source )
	{
		case AS_LOCAL:
			count = &cls.numlocalservers;
			servers = &cls.localServers[ 0 ];
			break;

		case AS_GLOBAL:
			count = &cls.numglobalservers;
			servers = &cls.globalServers[ 0 ];
			break;

		case AS_FAVORITES:
			count = &cls.numfavoriteservers;
			servers = &cls.favoriteServers[ 0 ];
			break;
	}

	if ( servers )
	{
		netadr_t comp;
		NET_StringToAdr( addr, &comp, NA_UNSPEC );

		for ( i = 0; i < *count; i++ )
		{
			if ( NET_CompareAdr( comp, servers[ i ].adr ) )
			{
				int j = i;

				while ( j < *count - 1 )
				{
					Com_Memcpy( &servers[ j ], &servers[ j + 1 ], sizeof( servers[ j ] ) );
					j++;
				}

				( *count )--;
				break;
			}
		}
	}
}

/*
 * ====================
 * LAN_GetServerCount
 * ====================
 */
static int LAN_GetServerCount( int source )
{
	switch ( source )
	{
		case AS_LOCAL:
			return cls.numlocalservers;

		case AS_GLOBAL:
			return cls.numglobalservers;

		case AS_FAVORITES:
			return cls.numfavoriteservers;
	}

	return 0;
}

/*
 * ====================
 * LAN_GetLocalServerAddressString
 * ====================
 */
static void LAN_GetServerAddressString( int source, int n, char *buf, int buflen )
{
	switch ( source )
	{
		case AS_LOCAL:
			if ( n >= 0 && n < MAX_OTHER_SERVERS )
			{
				Q_strncpyz( buf, NET_AdrToStringwPort( cls.localServers[ n ].adr ), buflen );
				return;
			}

			break;

		case AS_GLOBAL:
			if ( n >= 0 && n < MAX_GLOBAL_SERVERS )
			{
				Q_strncpyz( buf, NET_AdrToStringwPort( cls.globalServers[ n ].adr ), buflen );
				return;
			}

			break;

		case AS_FAVORITES:
			if ( n >= 0 && n < MAX_OTHER_SERVERS )
			{
				Q_strncpyz( buf, NET_AdrToStringwPort( cls.favoriteServers[ n ].adr ), buflen );
				return;
			}

			break;
	}

	buf[ 0 ] = '\0';
}

/*
 * ====================
 * LAN_GetServerInfo
 * ====================
 */
static void LAN_GetServerInfo( int source, int n, char *buf, int buflen )
{
	char         info[ MAX_STRING_CHARS ];
	serverInfo_t *server = NULL;

	info[ 0 ] = '\0';

	switch ( source )
	{
		case AS_LOCAL:
			if ( n >= 0 && n < MAX_OTHER_SERVERS )
			{
				server = &cls.localServers[ n ];
			}

			break;

		case AS_GLOBAL:
			if ( n >= 0 && n < MAX_GLOBAL_SERVERS )
			{
				server = &cls.globalServers[ n ];
			}

			break;

		case AS_FAVORITES:
			if ( n >= 0 && n < MAX_OTHER_SERVERS )
			{
				server = &cls.favoriteServers[ n ];
			}

			break;
	}

	if ( server && buf )
	{
		buf[ 0 ] = '\0';
		Info_SetValueForKey( info, "hostname", server->hostName, qfalse );
		Info_SetValueForKey( info, "serverload", va( "%i", server->load ), qfalse );
		Info_SetValueForKey( info, "mapname", server->mapName, qfalse );
		Info_SetValueForKey( info, "label", server->label, qfalse );
		Info_SetValueForKey( info, "clients", va( "%i", server->clients ), qfalse );
		Info_SetValueForKey( info, "bots", va( "%i", server->bots ), qfalse );
		Info_SetValueForKey( info, "sv_maxclients", va( "%i", server->maxClients ), qfalse );
		Info_SetValueForKey( info, "ping", va( "%i", server->ping ), qfalse );
		Info_SetValueForKey( info, "minping", va( "%i", server->minPing ), qfalse );
		Info_SetValueForKey( info, "maxping", va( "%i", server->maxPing ), qfalse );
		Info_SetValueForKey( info, "game", server->game, qfalse );
		Info_SetValueForKey( info, "nettype", va( "%i", server->netType ), qfalse );
		Info_SetValueForKey( info, "addr", NET_AdrToStringwPort( server->adr ), qfalse );
		Info_SetValueForKey( info, "friendlyFire", va( "%i", server->friendlyFire ), qfalse );   // NERVE - SMF
		Info_SetValueForKey( info, "needpass", va( "%i", server->needpass ), qfalse );   // NERVE - SMF
		Info_SetValueForKey( info, "gamename", server->gameName, qfalse );  // Arnout
		Q_strncpyz( buf, info, buflen );
	}
	else
	{
		if ( buf )
		{
			buf[ 0 ] = '\0';
		}
	}
}

/*
 * ====================
 * LAN_GetServerPing
 * ====================
 */
static int LAN_GetServerPing( int source, int n )
{
	serverInfo_t *server = NULL;

	switch ( source )
	{
		case AS_LOCAL:
			if ( n >= 0 && n < MAX_OTHER_SERVERS )
			{
				server = &cls.localServers[ n ];
			}

			break;

		case AS_GLOBAL:
			if ( n >= 0 && n < MAX_GLOBAL_SERVERS )
			{
				server = &cls.globalServers[ n ];
			}

			break;

		case AS_FAVORITES:
			if ( n >= 0 && n < MAX_OTHER_SERVERS )
			{
				server = &cls.favoriteServers[ n ];
			}

			break;
	}

	if ( server )
	{
		return server->ping;
	}

	return -1;
}

/*
 * ====================
 * LAN_GetServerPtr
 * ====================
 */
static serverInfo_t *LAN_GetServerPtr( int source, int n )
{
	switch ( source )
	{
		case AS_LOCAL:
			if ( n >= 0 && n < MAX_OTHER_SERVERS )
			{
				return &cls.localServers[ n ];
			}

			break;

		case AS_GLOBAL:
			if ( n >= 0 && n < MAX_GLOBAL_SERVERS )
			{
				return &cls.globalServers[ n ];
			}

			break;

		case AS_FAVORITES:
			if ( n >= 0 && n < MAX_OTHER_SERVERS )
			{
				return &cls.favoriteServers[ n ];
			}

			break;
	}

	return NULL;
}

#define FEATURED_MAXPING 200

/*
 * ====================
 * LAN_CompareServers
 * ====================
 */
static int LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 )
{
	int          res;
	serverInfo_t *server1, *server2;
	char         name1[ MAX_NAME_LENGTH ], name2[ MAX_NAME_LENGTH ];

	server1 = LAN_GetServerPtr( source, s1 );
	server2 = LAN_GetServerPtr( source, s2 );

	if ( !server1 || !server2 )
	{
		return 0;
	}

	// featured servers on top
	if ( ( server1->label[ 0 ] && server1->ping <= FEATURED_MAXPING ) ||
		( server2->label[ 0 ] && server2->ping <= FEATURED_MAXPING ) )
	{
		res = Q_strnicmp( server1->label, server2->label, MAX_FEATLABEL_CHARS );

		if ( res )
		{
			return -res;
		}
	}

	res = 0;

	switch ( sortKey )
	{
		case SORT_HOST:
			//% res = Q_stricmp( server1->hostName, server2->hostName );
			Q_strncpyz( name1, server1->hostName, sizeof( name1 ) );
			Q_CleanStr( name1 );
			Q_strncpyz( name2, server2->hostName, sizeof( name2 ) );
			Q_CleanStr( name2 );
			res = Q_stricmp( name1, name2 );
			break;

		case SORT_MAP:
			res = Q_stricmp( server1->mapName, server2->mapName );
			break;

		case SORT_CLIENTS:
			if ( server1->clients < server2->clients )
			{
				res = -1;
			}
			else if ( server1->clients > server2->clients )
			{
				res = 1;
			}
			else
			{
				res = 0;
			}

			break;

		case SORT_GAME:
			if ( server1->gameName < server2->gameName )
			{
				res = -1;
			}
			else if ( server1->gameName > server2->gameName )
			{
				res = 1;
			}
			else
			{
				res = 0;
			}

			break;

		case SORT_PING:
			if ( server1->ping < server2->ping )
			{
				res = -1;
			}
			else if ( server1->ping > server2->ping )
			{
				res = 1;
			}
			else
			{
				res = 0;
			}

			break;
	}

	if ( sortDir )
	{
		if ( res < 0 )
		{
			return 1;
		}

		if ( res > 0 )
		{
			return -1;
		}

		return 0;
	}

	return res;
}

/*
 * ====================
 * LAN_GetPingQueueCount
 * ====================
 */
static int LAN_GetPingQueueCount( void )
{
	return ( CL_GetPingQueueCount() );
}

/*
 * ====================
 * LAN_ClearPing
 * ====================
 */
static void LAN_ClearPing( int n )
{
	CL_ClearPing( n );
}

/*
 * ====================
 * LAN_GetPing
 * ====================
 */
static void LAN_GetPing( int n, char *buf, int buflen, int *pingtime )
{
	CL_GetPing( n, buf, buflen, pingtime );
}

/*
 * ====================
 * LAN_GetPingInfo
 * ====================
 */
static void LAN_GetPingInfo( int n, char *buf, int buflen )
{
	CL_GetPingInfo( n, buf, buflen );
}

/*
 * ====================
 * LAN_MarkServerVisible
 * ====================
 */
static void LAN_MarkServerVisible( int source, int n, qboolean visible )
{
	if ( n == -1 )
	{
		int          count = MAX_OTHER_SERVERS;
		serverInfo_t *server = NULL;

		switch ( source )
		{
			case AS_LOCAL:
				server = &cls.localServers[ 0 ];
				break;

			case AS_GLOBAL:
				server = &cls.globalServers[ 0 ];
				count = MAX_GLOBAL_SERVERS;
				break;

			case AS_FAVORITES:
				server = &cls.favoriteServers[ 0 ];
				break;
		}

		if ( server )
		{
			for ( n = 0; n < count; n++ )
			{
				server[ n ].visible = visible;
			}
		}
	}
	else
	{
		switch ( source )
		{
			case AS_LOCAL:
				if ( n >= 0 && n < MAX_OTHER_SERVERS )
				{
					cls.localServers[ n ].visible = visible;
				}

				break;

			case AS_GLOBAL:
				if ( n >= 0 && n < MAX_GLOBAL_SERVERS )
				{
					cls.globalServers[ n ].visible = visible;
				}

				break;

			case AS_FAVORITES:
				if ( n >= 0 && n < MAX_OTHER_SERVERS )
				{
					cls.favoriteServers[ n ].visible = visible;
				}

				break;
		}
	}
}

/*
 * =======================
 * LAN_ServerIsVisible
 * =======================
 */
static int LAN_ServerIsVisible( int source, int n )
{
	switch ( source )
	{
		case AS_LOCAL:
			if ( n >= 0 && n < MAX_OTHER_SERVERS )
			{
				return cls.localServers[ n ].visible;
			}

			break;

		case AS_GLOBAL:
			if ( n >= 0 && n < MAX_GLOBAL_SERVERS )
			{
				return cls.globalServers[ n ].visible;
			}

			break;

		case AS_FAVORITES:
			if ( n >= 0 && n < MAX_OTHER_SERVERS )
			{
				return cls.favoriteServers[ n ].visible;
			}

			break;
	}

	return qfalse;
}

/*
 * =======================
 * LAN_UpdateVisiblePings
 * =======================
 */
qboolean LAN_UpdateVisiblePings( int source )
{
	return CL_UpdateVisiblePings_f( source );
}

/*
 * ====================
 * LAN_GetServerStatus
 * ====================
 */
int LAN_GetServerStatus( char *serverAddress, char *serverStatus, int maxLen )
{
	return CL_ServerStatus( serverAddress, serverStatus, maxLen );
}

/*
 * =======================
 * LAN_ServerIsInFavoriteList
 * =======================
 */
qboolean LAN_ServerIsInFavoriteList( int source, int n )
{
	int          i;
	serverInfo_t *server = NULL;

	switch ( source )
	{
		case AS_LOCAL:
			if ( n >= 0 && n < MAX_OTHER_SERVERS )
			{
				server = &cls.localServers[ n ];
			}

			break;

		case AS_GLOBAL:
			if ( n >= 0 && n < MAX_GLOBAL_SERVERS )
			{
				server = &cls.globalServers[ n ];
			}

			break;

		case AS_FAVORITES:
			if ( n >= 0 && n < MAX_OTHER_SERVERS )
			{
				return qtrue;
			}

			break;
	}

	if ( !server )
	{
		return qfalse;
	}

	for ( i = 0; i < cls.numfavoriteservers; i++ )
	{
		if ( NET_CompareAdr( cls.favoriteServers[ i ].adr, server->adr ) )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
 * ====================
 * Key_KeynumToStringBuf
 * ====================
 */
void Key_KeynumToStringBuf( int keynum, char *buf, int buflen )
{
	Q_strncpyz( buf, Key_KeynumToString( keynum ), buflen );
}

/*
 * ====================
 * Key_GetBindingBuf
 * ====================
 */
void Key_GetBindingBuf( int keynum, int team, char *buf, int buflen )
{
	const char *value;

	value = Key_GetBinding( keynum, team );

	if ( value )
	{
		Q_strncpyz( buf, value, buflen );
	}
	else
	{
		*buf = 0;
	}
}

/*
 * ====================
 * Key_GetCatcher
 * ====================
 */
int Key_GetCatcher( void )
{
	return cls.keyCatchers;
}

/*
 * ====================
 * Ket_SetCatcher
 * ====================
 */
void Key_SetCatcher( int catcher )
{
	// NERVE - SMF - console overrides everything
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		cls.keyCatchers = catcher | KEYCATCH_CONSOLE;
	}
	else
	{
		cls.keyCatchers = catcher;
	}
}


static int FloatAsInt( float f )
{
	floatint_t fi;

	fi.f = f;
	return fi.i;
}

//static int numtraces = 0;

/*
====================
CL_CgameSystemCalls

The cgame module is making a system call
====================
*/
intptr_t CL_CgameSystemCalls( intptr_t *args )
{
	switch ( args[ 0 ] )
	{
		case CG_PRINT:
			Com_Printf( "%s", ( char * ) VMA( 1 ) );
			return 0;

		case CG_ERROR:
			Com_Error( ERR_DROP, "%s", ( char * ) VMA( 1 ) );

		case CG_MILLISECONDS:
			return Sys_Milliseconds();

		case CG_CVAR_REGISTER:
			Cvar_Register( VMA( 1 ), VMA( 2 ), VMA( 3 ), args[ 4 ] );
			return 0;

		case CG_CVAR_UPDATE:
			Cvar_Update( VMA( 1 ) );
			return 0;

		case CG_CVAR_SET:
			Cvar_Set( VMA( 1 ), VMA( 2 ) );
			return 0;

		case CG_CVAR_VARIABLESTRINGBUFFER:
			VM_CheckBlock( args[2], args[3], "CVARVSB" );
			Cvar_VariableStringBuffer( VMA( 1 ), VMA( 2 ), args[ 3 ] );
			return 0;

		case CG_CVAR_LATCHEDVARIABLESTRINGBUFFER:
			VM_CheckBlock( args[2], args[3], "CVARLVSB" );
			Cvar_LatchedVariableStringBuffer( VMA( 1 ), VMA( 2 ), args[ 3 ] );
			return 0;

		case CG_CVAR_VARIABLEINTEGERVALUE:
			return Cvar_VariableIntegerValue( VMA( 1 ) );

		case CG_ARGC:
			return Cmd_Argc();

		case CG_ARGV:
			VM_CheckBlock( args[2], args[3], "ARGV" );
			Cmd_ArgvBuffer( args[ 1 ], VMA( 2 ), args[ 3 ] );
			return 0;

		case CG_ARGS:
			VM_CheckBlock( args[1], args[2], "ARGS" );
			Cmd_ArgsBuffer( VMA( 1 ), args[ 2 ] );
			return 0;

		case CG_LITERAL_ARGS:
			// FIXME
			VM_CheckBlock( args[1], args[2], "LARGS" );
			Cmd_LiteralArgsBuffer( VMA( 1 ), args[ 2 ] );
//                      Cmd_ArgsBuffer(VMA(1), args[2]);
			return 0;

		case CG_GETDEMOSTATE:
			return CL_DemoState();

		case CG_GETDEMOPOS:
			return CL_DemoPos();

		case CG_FS_FOPENFILE:
			return FS_FOpenFileByMode( VMA( 1 ), VMA( 2 ), args[ 3 ] );

		case CG_FS_READ:
			VM_CheckBlock( args[1], args[2], "FSREAD" );
			FS_Read2( VMA( 1 ), args[ 2 ], args[ 3 ] );
			return 0;

		case CG_FS_WRITE:
			VM_CheckBlock( args[1], args[2], "FSWRITE" );
			return FS_Write( VMA( 1 ), args[ 2 ], args[ 3 ] );

		case CG_FS_FCLOSEFILE:
			FS_FCloseFile( args[ 1 ] );
			return 0;

		case CG_FS_GETFILELIST:
			VM_CheckBlock( args[3], args[4], "FSGFL" );
			return FS_GetFileList( VMA( 1 ), VMA( 2 ), VMA( 3 ), args[ 4 ] );

		case CG_FS_DELETEFILE:
			return FS_Delete( VMA( 1 ) );

		case CG_SENDCONSOLECOMMAND:
			Cbuf_AddText( VMA( 1 ) );
			return 0;

		case CG_ADDCOMMAND:
			CL_AddCgameCommand( VMA( 1 ) );
			return 0;

		case CG_REMOVECOMMAND:
			Cmd_RemoveCommand( VMA( 1 ) );
			return 0;

		case CG_COMPLETE_CALLBACK:
			if ( completer )
			{
				completer( VMA( 1 ) );
			}

			return 0;

		case CG_SENDCLIENTCOMMAND:
			CL_AddReliableCommand( VMA( 1 ) );
			return 0;

		case CG_UPDATESCREEN:
			SCR_UpdateScreen();
			return 0;

		case CG_CM_LOADMAP:
			CL_CM_LoadMap( VMA( 1 ) );
			return 0;

		case CG_CM_NUMINLINEMODELS:
			return CM_NumInlineModels();

		case CG_CM_INLINEMODEL:
			return CM_InlineModel( args[ 1 ] );

		case CG_CM_TEMPBOXMODEL:
			return CM_TempBoxModel( VMA( 1 ), VMA( 2 ), qfalse );

		case CG_CM_TEMPCAPSULEMODEL:
			return CM_TempBoxModel( VMA( 1 ), VMA( 2 ), qtrue );

		case CG_CM_POINTCONTENTS:
			return CM_PointContents( VMA( 1 ), args[ 2 ] );

		case CG_CM_TRANSFORMEDPOINTCONTENTS:
			return CM_TransformedPointContents( VMA( 1 ), args[ 2 ], VMA( 3 ), VMA( 4 ) );

		case CG_CM_BOXTRACE:
			CM_BoxTrace( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMA( 4 ), VMA( 5 ), args[ 6 ], args[ 7 ], TT_AABB );
			return 0;

		case CG_CM_TRANSFORMEDBOXTRACE:
			CM_TransformedBoxTrace( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMA( 4 ), VMA( 5 ), args[ 6 ], args[ 7 ], VMA( 8 ), VMA( 9 ), TT_AABB );
			return 0;

		case CG_CM_CAPSULETRACE:
			CM_BoxTrace( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMA( 4 ), VMA( 5 ), args[ 6 ], args[ 7 ], TT_CAPSULE );
			return 0;

		case CG_CM_TRANSFORMEDCAPSULETRACE:
			CM_TransformedBoxTrace( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMA( 4 ), VMA( 5 ), args[ 6 ], args[ 7 ], VMA( 8 ), VMA( 9 ), TT_CAPSULE );
			return 0;

		case CG_CM_BISPHERETRACE:
			CM_BiSphereTrace( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMF( 4 ), VMF( 5 ), args[ 6 ], args[ 7 ] );
			return 0;

		case CG_CM_TRANSFORMEDBISPHERETRACE:
			CM_TransformedBiSphereTrace( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMF( 4 ), VMF( 5 ), args[ 6 ], args[ 7 ], VMA( 8 ) );
			return 0;

		case CG_CM_MARKFRAGMENTS:
			return re.MarkFragments( args[ 1 ], VMA( 2 ), VMA( 3 ), args[ 4 ], VMA( 5 ), args[ 6 ], VMA( 7 ) );

		case CG_R_PROJECTDECAL:
			re.ProjectDecal( args[ 1 ], args[ 2 ], VMA( 3 ), VMA( 4 ), VMA( 5 ), args[ 6 ], args[ 7 ] );
			return 0;

		case CG_R_CLEARDECALS:
			re.ClearDecals();
			return 0;

		case CG_S_STARTSOUND:
			S_StartSound( VMA( 1 ), args[ 2 ], args[ 3 ], args[ 4 ] );
			return 0;

		case CG_S_STARTSOUNDEX:
			S_StartSoundEx( VMA( 1 ), args[ 2 ], args[ 3 ], args[ 4 ] );
			return 0;

		case CG_S_STARTLOCALSOUND:
			S_StartLocalSound( args[ 1 ], args[ 2 ] );
			return 0;

		case CG_S_CLEARLOOPINGSOUNDS:
			S_ClearLoopingSounds( args[ 1 ] );
			return 0;

		case CG_S_CLEARSOUNDS:

			/*if(args[1] == 0)
			{
			        S_ClearSounds(qtrue, qfalse);
			}
			else if(args[1] == 1)
			{
			        S_ClearSounds(qtrue, qtrue);
			}*/
			return 0;

		case CG_S_ADDLOOPINGSOUND:
			S_AddLoopingSound( args[ 1 ], VMA( 2 ), VMA( 3 ), args[ 4 ] );
			return 0;

		case CG_S_ADDREALLOOPINGSOUND:
			S_AddRealLoopingSound( args[ 1 ], VMA( 2 ), VMA( 3 ), args[ 4 ] );
			return 0;

		case CG_S_STOPLOOPINGSOUND:
			S_StopLoopingSound( args[ 1 ] );
			return 0;

		case CG_S_STOPSTREAMINGSOUND:
			// FIXME
			//S_StopEntStreamingSound(args[1]);
			return 0;

		case CG_S_UPDATEENTITYPOSITION:
			S_UpdateEntityPosition( args[ 1 ], VMA( 2 ) );
			return 0;

		case CG_S_GETVOICEAMPLITUDE:
			return S_GetVoiceAmplitude( args[ 1 ] );

		case CG_S_GETSOUNDLENGTH:
			return S_GetSoundLength( args[ 1 ] );

			// ydnar: for looped sound starts
		case CG_S_GETCURRENTSOUNDTIME:
			return S_GetCurrentSoundTime();

		case CG_S_RESPATIALIZE:
			S_Respatialize( args[ 1 ], VMA( 2 ), VMA( 3 ), args[ 4 ] );
			return 0;

		case CG_S_REGISTERSOUND:
#ifdef DOOMSOUND ///// (SA) DOOMSOUND
			return S_RegisterSound( VMA( 1 ) );
#else
			return S_RegisterSound( VMA( 1 ), args[ 2 ] );
#endif ///// (SA) DOOMSOUND

		case CG_S_STARTBACKGROUNDTRACK:
			//S_StartBackgroundTrack(VMA(1), VMA(2), args[3]);  //----(SA)  added fadeup time
			S_StartBackgroundTrack( VMA( 1 ), VMA( 2 ) );
			return 0;

		case CG_S_FADESTREAMINGSOUND:
			// FIXME
			//S_FadeStreamingSound(VMF(1), args[2], args[3]); //----(SA)  added music/all-streaming options
			return 0;

		case CG_S_STARTSTREAMINGSOUND:
			// FIXME
			//return S_StartStreamingSound(VMA(1), VMA(2), args[3], args[4], args[5]);
			return 0;

		case CG_R_LOADWORLDMAP:
			re.SetWorldVisData( CM_ClusterPVS( -1 ) );
			re.LoadWorld( VMA( 1 ) );
			return 0;

		case CG_R_REGISTERMODEL:
			return re.RegisterModel( VMA( 1 ) );

		case CG_R_REGISTERSKIN:
			return re.RegisterSkin( VMA( 1 ) );

			//----(SA)  added
		case CG_R_GETSKINMODEL:
			return re.GetSkinModel( args[ 1 ], VMA( 2 ), VMA( 3 ) );

		case CG_R_GETMODELSHADER:
			return re.GetShaderFromModel( args[ 1 ], args[ 2 ], args[ 3 ] );
			//----(SA)  end

		case CG_R_REGISTERSHADER:
			return re.RegisterShader( VMA( 1 ) );

		case CG_R_REGISTERFONT:
			re.RegisterFontVM( VMA( 1 ), VMA( 2 ), args[ 3 ], VMA( 4 ) );
			return 0;

		case CG_R_REGISTERSHADERNOMIP:
			return re.RegisterShaderNoMip( VMA( 1 ) );

#if defined( USE_REFLIGHT )
		case CG_R_REGISTERSHADERLIGHTATTENUATION:
			return re.RegisterShaderLightAttenuation( VMA( 1 ) );
#endif

		case CG_R_CLEARSCENE:
			re.ClearScene();
			return 0;

		case CG_R_ADDREFENTITYTOSCENE:
			re.AddRefEntityToScene( VMA( 1 ) );
			return 0;

#if defined( USE_REFLIGHT )
		case CG_R_ADDREFLIGHTSTOSCENE:
			re.AddRefLightToScene( VMA( 1 ) );
			return 0;
#endif

		case CG_R_ADDPOLYTOSCENE:
			re.AddPolyToScene( args[ 1 ], args[ 2 ], VMA( 3 ) );
			return 0;

		case CG_R_ADDPOLYSTOSCENE:
			re.AddPolysToScene( args[ 1 ], args[ 2 ], VMA( 3 ), args[ 4 ] );
			return 0;

		case CG_R_ADDPOLYBUFFERTOSCENE:
			re.AddPolyBufferToScene( VMA( 1 ) );
			return 0;

		case CG_R_ADDLIGHTTOSCENE:
			re.AddLightToScene( VMA( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), args[ 7 ], args[ 8 ] );
			return 0;

		case CG_R_ADDADDITIVELIGHTTOSCENE:
			re.AddAdditiveLightToScene( VMA( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ) );
			return 0;

		case CG_FS_SEEK:
			return FS_Seek( args[ 1 ], args[ 2 ], args[ 3 ] );

		case CG_R_ADDCORONATOSCENE:
			re.AddCoronaToScene( VMA( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), args[ 6 ], args[ 7 ] );
			return 0;

		case CG_R_SETFOG:
			re.SetFog( args[ 1 ], args[ 2 ], args[ 3 ], VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ) );
			return 0;

		case CG_R_SETGLOBALFOG:
			re.SetGlobalFog( args[ 1 ], args[ 2 ], VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ) );
			return 0;

		case CG_R_RENDERSCENE:
			re.RenderScene( VMA( 1 ) );
			return 0;

		case CG_R_SAVEVIEWPARMS:
			re.SaveViewParms();
			return 0;

		case CG_R_RESTOREVIEWPARMS:
			re.RestoreViewParms();
			return 0;

		case CG_R_SETCOLOR:
			re.SetColor( VMA( 1 ) );
			return 0;

			// Tremulous
		case CG_R_SETCLIPREGION:
			re.SetClipRegion( VMA( 1 ) );
			return 0;

		case CG_R_DRAWSTRETCHPIC:
			re.DrawStretchPic( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ), VMF( 8 ), args[ 9 ] );
			return 0;

		case CG_R_DRAWROTATEDPIC:
			re.DrawRotatedPic( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ), VMF( 8 ), args[ 9 ], VMF( 10 ) );
			return 0;

		case CG_R_DRAWSTRETCHPIC_GRADIENT:
			re.DrawStretchPicGradient( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ), VMF( 8 ), args[ 9 ], VMA( 10 ), args[ 11 ] );
			return 0;

		case CG_R_DRAW2DPOLYS:
			re.Add2dPolys( VMA( 1 ), args[ 2 ], args[ 3 ] );
			return 0;

		case CG_R_MODELBOUNDS:
			re.ModelBounds( args[ 1 ], VMA( 2 ), VMA( 3 ) );
			return 0;

		case CG_R_LERPTAG:
			return re.LerpTag( VMA( 1 ), VMA( 2 ), VMA( 3 ), args[ 4 ] );

		case CG_GETGLCONFIG:
			CL_GetGlconfig( VMA( 1 ) );
			return 0;

		case CG_GETGAMESTATE:
			CL_GetGameState( VMA( 1 ) );
			return 0;

		case CG_GETCURRENTSNAPSHOTNUMBER:
			CL_GetCurrentSnapshotNumber( VMA( 1 ), VMA( 2 ) );
			return 0;

		case CG_GETSNAPSHOT:
			return CL_GetSnapshot( args[ 1 ], VMA( 2 ) );

		case CG_GETSERVERCOMMAND:
			return CL_GetServerCommand( args[ 1 ] );

		case CG_GETCURRENTCMDNUMBER:
			return CL_GetCurrentCmdNumber();

		case CG_GETUSERCMD:
			return CL_GetUserCmd( args[ 1 ], VMA( 2 ) );

		case CG_SETUSERCMDVALUE:
			CL_SetUserCmdValue( args[ 1 ], args[ 2 ], VMF( 3 ), args[ 4 ] );
			return 0;

		case CG_SETCLIENTLERPORIGIN:
			CL_SetClientLerpOrigin( VMF( 1 ), VMF( 2 ), VMF( 3 ) );
			return 0;

		case CG_MEMORY_REMAINING:
			return Hunk_MemoryRemaining();

		case CG_KEY_ISDOWN:
			return Key_IsDown( args[ 1 ] );

		case CG_KEY_GETCATCHER:
			return Key_GetCatcher();

		case CG_KEY_SETCATCHER:
			Key_SetCatcher( args[ 1 ] );
			return 0;

		case CG_KEY_GETKEY:
			return Key_GetKey( VMA( 1 ), 0 ); // FIXME BIND

		case CG_KEY_GETOVERSTRIKEMODE:
			return Key_GetOverstrikeMode();

		case CG_KEY_SETOVERSTRIKEMODE:
			Key_SetOverstrikeMode( args[ 1 ] );
			return 0;

		case CG_S_STOPBACKGROUNDTRACK:
			S_StopBackgroundTrack();
			return 0;

		case CG_REAL_TIME:
			return Com_RealTime( VMA( 1 ) );

		case CG_SNAPVECTOR:
			Q_SnapVector( VMA( 1 ) );
			return 0;

		case CG_CIN_PLAYCINEMATIC:
			return CIN_PlayCinematic( VMA( 1 ), args[ 2 ], args[ 3 ], args[ 4 ], args[ 5 ], args[ 6 ] );

		case CG_CIN_STOPCINEMATIC:
			return CIN_StopCinematic( args[ 1 ] );

		case CG_CIN_RUNCINEMATIC:
			return CIN_RunCinematic( args[ 1 ] );

		case CG_CIN_DRAWCINEMATIC:
			CIN_DrawCinematic( args[ 1 ] );
			return 0;

		case CG_CIN_SETEXTENTS:
			CIN_SetExtents( args[ 1 ], args[ 2 ], args[ 3 ], args[ 4 ], args[ 5 ] );
			return 0;

		case CG_R_REMAP_SHADER:
			re.RemapShader( VMA( 1 ), VMA( 2 ), VMA( 3 ) );
			return 0;

		case CG_GET_ENTITY_TOKEN:
			VM_CheckBlock( args[1], args[2], "GETET" );
			return re.GetEntityToken( VMA( 1 ), args[ 2 ] );

		case CG_INGAME_POPUP:
			if ( cls.state == CA_ACTIVE && !clc.demoplaying )
			{
				Rocket_DocumentAction( VMA( 1 ), "open" );
			}

			return 0;

		case CG_INGAME_CLOSEPOPUP:
			return 0;

		case CG_KEY_GETBINDINGBUF:
			VM_CheckBlock( args[3], args[4], "KEYGBB" );
			Key_GetBindingBuf( args[ 1 ], args[ 2 ], VMA( 3 ), args[ 4 ] );
			return 0;

		case CG_KEY_SETBINDING:
			Key_SetBinding( args[ 1 ], args[ 2 ], VMA( 3 ) ); // FIXME BIND
			return 0;

		case CG_PARSE_ADD_GLOBAL_DEFINE:
			return Parse_AddGlobalDefine( VMA( 1 ) );

		case CG_PARSE_LOAD_SOURCE:
			return Parse_LoadSourceHandle( VMA( 1 ) );

		case CG_PARSE_FREE_SOURCE:
			return Parse_FreeSourceHandle( args[ 1 ] );

		case CG_PARSE_READ_TOKEN:
			return Parse_ReadTokenHandle( args[ 1 ], VMA( 2 ) );

		case CG_PARSE_SOURCE_FILE_AND_LINE:
			return Parse_SourceFileAndLine( args[ 1 ], VMA( 2 ), VMA( 3 ) );

		case CG_KEY_KEYNUMTOSTRINGBUF:
			VM_CheckBlock( args[2], args[3], "KEYNTSB" );
			Key_KeynumToStringBuf( args[ 1 ], VMA( 2 ), args[ 3 ] );
			return 0;

		case CG_S_FADEALLSOUNDS:
			// FIXME
			//S_FadeAllSounds(VMF(1), args[2], args[3]);
			return 0;

		case CG_R_INPVS:
			return re.inPVS( VMA( 1 ), VMA( 2 ) );

		case CG_R_INPVVS:
			return re.inPVVS( VMA( 1 ), VMA( 2 ) );

		case CG_GETHUNKDATA:
			Com_GetHunkInfo( VMA( 1 ), VMA( 2 ) );
			return 0;

			//bani - dynamic shaders
		case CG_R_LOADDYNAMICSHADER:
			return re.LoadDynamicShader( VMA( 1 ), VMA( 2 ) );

			// fretn - render to texture
		case CG_R_RENDERTOTEXTURE:
			re.RenderToTexture( args[ 1 ], args[ 2 ], args[ 3 ], args[ 4 ], args[ 5 ] );
			return 0;

			//bani
		case CG_R_GETTEXTUREID:
			return re.GetTextureId( VMA( 1 ) );

			//bani - flush gl rendering buffers
		case CG_R_FINISH:
			re.Finish();
			return 0;

		case CG_GETDEMONAME:
			VM_CheckBlock( args[1], args[2], "GETDM" );
			CL_DemoName( VMA( 1 ), args[ 2 ] );
			return 0;

		case CG_R_LIGHTFORPOINT:
			return re.LightForPoint( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMA( 4 ) );

		case CG_S_SOUNDDURATION:
			return S_SoundDuration( args[ 1 ] );

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )
		case CG_R_REGISTERANIMATION:
			return re.RegisterAnimation( VMA( 1 ) );

		case CG_R_CHECKSKELETON:
			return re.CheckSkeleton( VMA( 1 ), args[ 2 ], args[ 3 ] );

		case CG_R_BUILDSKELETON:
			return re.BuildSkeleton( VMA( 1 ), args[ 2 ], args[ 3 ], args[ 4 ], VMF( 5 ), args[ 6 ] );

		case CG_R_BLENDSKELETON:
			return re.BlendSkeleton( VMA( 1 ), VMA( 2 ), VMF( 3 ) );

		case CG_R_BONEINDEX:
			return re.BoneIndex( args[ 1 ], VMA( 2 ) );

		case CG_R_ANIMNUMFRAMES:
			return re.AnimNumFrames( args[ 1 ] );

		case CG_R_ANIMFRAMERATE:
			return re.AnimFrameRate( args[ 1 ] );
#endif

		case CG_REGISTER_BUTTON_COMMANDS:
			CL_RegisterButtonCommands( VMA( 1 ) );
			return 0;

		case CG_GETCLIPBOARDDATA:
			VM_CheckBlock( args[1], args[2], "GETCLIP" );

			if ( cl_allowPaste->integer )
			{
				CL_GetClipboardData( VMA(1), args[2], args[3] );
			}
			else
			{
				( (char *) VMA( 1 ) )[0] = '\0';
			}
			return 0;

		case CG_QUOTESTRING:
			VM_CheckBlock( args[ 2 ], args[ 3 ], "QUOTE" );
			Cmd_QuoteStringBuffer( VMA( 1 ), VMA( 2 ), args[ 3 ] );
			return 0;

		case CG_GETTEXT:
			VM_CheckBlock( args[ 1 ], args[ 3 ], "CGGETTEXT" );
			strncpy( VMA(1), __(VMA(2)), args[3] );
			return 0;

		case CG_PGETTEXT:
			VM_CheckBlock( args[ 1 ], args[ 4 ], "CGPGETTEXT" );
			strncpy( VMA( 1 ), C__( VMA( 2 ), VMA( 3 ) ), args[ 4 ] );
			return 0;

		case CG_GETTEXT_PLURAL:
			VM_CheckBlock( args[ 1 ], args[ 5 ], "CGGETTEXTP" );
			strncpy( VMA( 1 ), P__( VMA( 2 ), VMA( 3 ), args[ 4 ] ), args[ 5 ] );
			return 0;

		case CG_R_GLYPH:
			re.GlyphVM( args[1], VMA(2), VMA(3) );
			return 0;

		case CG_R_GLYPHCHAR:
			re.GlyphCharVM( args[1], args[2], VMA(3) );
			return 0;

		case CG_R_UREGISTERFONT:
			re.UnregisterFontVM( args[1] );
			return 0;

		case CG_KEY_SETTEAM:
			Key_SetTeam( args[1] ); // for binding selection
			return 0;

		case CG_REGISTERVISTEST:
			return re.RegisterVisTest();

		case CG_ADDVISTESTTOSCENE:
			re.AddVisTestToScene( args[1], VMA(2), VMF(3) );
			return 0;

		case CG_CHECKVISIBILITY:
			return re.CheckVisibility( args[1] );

		case CG_UNREGISTERVISTEST:
			re.UnregisterVisTest( args[1] );
			return 0;

		case CG_SETCOLORGRADING:
			re.SetColorGrading( args[1] );
			return 0;

		case CG_ROCKET_INIT:
			Rocket_Init();
			return 0;

		case CG_ROCKET_SHUTDOWN:
			Rocket_Shutdown();
			return 0;

		case CG_ROCKET_LOADDOCUMENT:
			Rocket_LoadDocument( VMA(1) );
			return 0;

		case CG_ROCKET_LOADCURSOR:
			Rocket_LoadCursor( VMA(1) );
			return 0;

		case CG_ROCKET_DOCUMENTACTION:
			Rocket_DocumentAction( VMA(1), VMA(2) );
			return 0;

		case CG_ROCKET_GETEVENT:
			Rocket_GetEvent( VMA(1), args[2] );
			return 0;

		case CG_ROCKET_DELELTEEVENT:
			Rocket_DeleteEvent();
			return 0;

		case CG_ROCKET_REGISTERDATASOURCE:
			Rocket_RegisterDataSource( VMA(1) );
			return 0;

		case CG_ROCKET_DSADDROW:
			Rocket_DSAddRow( VMA(1), VMA(2), VMA(3) );
			return 0;

		case CG_LAN_LOADCACHEDSERVERS:
			LAN_LoadCachedServers();
			return 0;

		case CG_LAN_SAVECACHEDSERVERS:
			LAN_SaveServersToCache();
			return 0;

		case CG_LAN_ADDSERVER:
			return LAN_AddServer( args[ 1 ], VMA( 2 ), VMA( 3 ) );

		case CG_LAN_REMOVESERVER:
			LAN_RemoveServer( args[ 1 ], VMA( 2 ) );
			return 0;

		case CG_LAN_GETPINGQUEUECOUNT:
			return LAN_GetPingQueueCount();

		case CG_LAN_CLEARPING:
			LAN_ClearPing( args[ 1 ] );
			return 0;

		case CG_LAN_GETPING:
			VM_CheckBlock( args[2], args[3], "UILANGP" );
			LAN_GetPing( args[ 1 ], VMA( 2 ), args[ 3 ], VMA( 4 ) );
			return 0;

		case CG_LAN_GETPINGINFO:
			VM_CheckBlock( args[2], args[3], "UILANGPI" );
			LAN_GetPingInfo( args[ 1 ], VMA( 2 ), args[ 3 ] );
			return 0;

		case CG_LAN_GETSERVERCOUNT:
			return LAN_GetServerCount( args[ 1 ] );

		case CG_LAN_GETSERVERADDRESSSTRING:
			VM_CheckBlock( args[3], args[4], "UILANGSAS" );
			LAN_GetServerAddressString( args[ 1 ], args[ 2 ], VMA( 3 ), args[ 4 ] );
			return 0;

		case CG_LAN_GETSERVERINFO:
			VM_CheckBlock( args[3], args[4], "UILANGSI" );
			LAN_GetServerInfo( args[ 1 ], args[ 2 ], VMA( 3 ), args[ 4 ] );
			return 0;

		case CG_LAN_GETSERVERPING:
			return LAN_GetServerPing( args[ 1 ], args[ 2 ] );

		case CG_LAN_MARKSERVERVISIBLE:
			LAN_MarkServerVisible( args[ 1 ], args[ 2 ], args[ 3 ] );
			return 0;

		case CG_LAN_SERVERISVISIBLE:
			return LAN_ServerIsVisible( args[ 1 ], args[ 2 ] );

		case CG_LAN_UPDATEVISIBLEPINGS:
			return LAN_UpdateVisiblePings( args[ 1 ] );

		case CG_LAN_RESETPINGS:
			LAN_ResetPings( args[ 1 ] );
			return 0;

		case CG_LAN_SERVERSTATUS:
			VM_CheckBlock( args[2], args[3], "UILANGSS" );
			return LAN_GetServerStatus( VMA( 1 ), VMA( 2 ), args[ 3 ] );

		case CG_LAN_SERVERISINFAVORITELIST:
			return LAN_ServerIsInFavoriteList( args[ 1 ], args[ 2 ] );

		case CG_GETNEWS:
			return GetNews( args[ 1 ] );

		case CG_LAN_COMPARESERVERS:
			return LAN_CompareServers( args[ 1 ], args[ 2 ], args[ 3 ], args[ 4 ], args[ 5 ] );

		case CG_CMD_EXECUTETEXT:
			Cbuf_ExecuteText( args[ 1 ], VMA( 2 ) );
			return 0;

		case CG_ROCKET_DSCLEARTABLE:
			Rocket_DSClearTable( VMA(1), VMA(2) );
			return 0;

		case CG_ROCKET_SETINNERRML:
			Rocket_SetInnerRML( VMA(1), VMA(2), VMA(3) );
			return 0;

		case CG_ROCKET_GETEVENTPARAMETERS:
			Rocket_GetEventParameters( VMA(1), args[2] );
			return 0;

		case CG_ROCKET_REGISTERDATAFORMATTER:
			Rocket_RegisterDataFormatter( VMA(1) );
			return 0;

		case CG_ROCKET_DATAFORMATTERRAWDATA:
			Rocket_DataFormatterRawData( args[1], VMA(2), args[3], VMA(4), args[5] );
			return 0;

		case CG_ROCKET_DATAFORMATTERFORMATTEDDATA:
			Rocket_DataFormatterFormattedData( args[1], VMA(2) );
			return 0;

		case CG_ROCKET_GETATTRIBUTE:
			Rocket_GetAttribute( VMA(1), VMA(2), VMA(3), VMA(4), args[5] );
			return 0;

		case CG_ROCKET_SETATTRIBUTE:
			Rocket_SetAttribute( VMA(1), VMA(2), VMA(3), VMA(4) );
			return 0;

		case CG_ROCKET_REGISTERELEMENT:
			Rocket_RegisterElement( VMA(1) );
			return 0;

		case CG_ROCKET_SETELEMENTDIMENSIONS:
			Rocket_SetElementDimensions( VMF(1), VMF(2) );
			return 0;

		case CG_ROCKET_GETELEMENTTAG:
			Rocket_GetElementTag( VMA(1), args[ 2 ] );
			return 0;

		case CG_ROCKET_KEYTOQUAKE:
			return Rocket_ToQuakeKey( args[ 1 ] );

		default:
			Com_Error( ERR_DROP, "Bad cgame system trap: %ld", ( long int ) args[ 0 ] );
	}

	return 0;
}

/*
====================
CL_UpdateLevelHunkUsage

  This updates the "hunkusage.dat" file with the current map and its hunk usage count

  This is used for level loading, so we can show a percentage bar dependent on the amount
  of hunk memory allocated so far

  This will be slightly inaccurate if some settings like sound quality are changed, but these
  things should only account for a small variation (hopefully)
====================
*/
void CL_UpdateLevelHunkUsage( void )
{
	int  handle;
	char *memlistfile = "hunkusage.dat";
	char *buf, *outbuf;
	char *buftrav, *outbuftrav;
	char *token;
	char outstr[ 256 ];
	int  len, memusage;

	memusage = Cvar_VariableIntegerValue( "com_hunkused" ) + Cvar_VariableIntegerValue( "hunk_soundadjust" );

	len = FS_FOpenFileByMode( memlistfile, &handle, FS_READ );

	if ( len >= 0 )
	{
		// the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value
		buf = ( char * ) Z_Malloc( len + 1 );
		outbuf = ( char * ) Z_Malloc( len + 1 );

		FS_Read( ( void * ) buf, len, handle );
		FS_FCloseFile( handle );

		// now parse the file, filtering out the current map
		buftrav = buf;
		outbuftrav = outbuf;
		outbuftrav[ 0 ] = '\0';

		while ( ( token = COM_Parse( &buftrav ) ) != NULL && token[ 0 ] )
		{
			if ( !Q_stricmp( token, cl.mapname ) )
			{
				// found a match
				token = COM_Parse( &buftrav );  // read the size

				if ( token && token[ 0 ] )
				{
					if ( atoi( token ) == memusage )
					{
						// if it is the same, abort this process
						Z_Free( buf );
						Z_Free( outbuf );
						return;
					}
				}
			}
			else
			{
				// send it to the outbuf
				Q_strcat( outbuftrav, len + 1, token );
				Q_strcat( outbuftrav, len + 1, " " );
				token = COM_Parse( &buftrav );  // read the size

				if ( token && token[ 0 ] )
				{
					Q_strcat( outbuftrav, len + 1, token );
					Q_strcat( outbuftrav, len + 1, "\n" );
				}
				else
				{
					Z_Free( buf );
					Z_Free( outbuf );
					Com_Error( ERR_DROP, "hunkusage.dat file is corrupt" );
				}
			}
		}

		handle = FS_FOpenFileWrite( memlistfile );

		if ( handle < 0 )
		{
			Z_Free( buf );
			Z_Free( outbuf );
			Com_Error( ERR_DROP, "cannot create %s", memlistfile );
		}

		// input file is parsed, now output to the new file
		len = strlen( outbuf );

		if ( FS_Write( ( void * ) outbuf, len, handle ) != len )
		{
			Z_Free( buf );
			Z_Free( outbuf );
			Com_Error( ERR_DROP, "cannot write to %s", memlistfile );
		}

		FS_FCloseFile( handle );

		Z_Free( buf );
		Z_Free( outbuf );
	}

	// now append the current map to the current file
	FS_FOpenFileByMode( memlistfile, &handle, FS_APPEND );

	if ( handle < 0 )
	{
		Com_Error( ERR_DROP, "cannot write to hunkusage.dat, check disk full" );
	}

	Com_sprintf( outstr, sizeof( outstr ), "%s %i\n", cl.mapname, memusage );
	FS_Write( outstr, strlen( outstr ), handle );
	FS_FCloseFile( handle );

	// now just open it and close it, so it gets copied to the pak dir
	len = FS_FOpenFileByMode( memlistfile, &handle, FS_READ );

	if ( len >= 0 )
	{
		FS_FCloseFile( handle );
	}
}

/*
=============
CL_InitUI

Start the cgame so we can load rocket
=============
*/

void CL_InitUI( void )
{
	int v;
	cgvm = VM_Create( "cgame", CL_CgameSystemCalls, Cvar_VariableValue( "vm_cgame" ) );

	if ( !cgvm )
	{
		Com_Error( ERR_DROP, "VM_Create on cgame failed" );
	}

	VM_Call( cgvm, CG_INIT_ROCKET );
}



/*
====================
CL_InitCGame

Should only by called by CL_StartHunkUsers
====================
*/
void CL_InitCGame( void )
{
	const char *info;
	const char *mapname;
	int        t1, t2;

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SERVERINFO ];
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cl.mapname, sizeof( cl.mapname ), "maps/%s.bsp", mapname );

	if ( !cgvm )
	{
		cgvm = VM_Create( "cgame", CL_CgameSystemCalls, Cvar_VariableValue( "vm_cgame" ) );

		if ( !cgvm )
		{
			Com_Error( ERR_DROP, "VM_Create on cgame failed" );
		}
	}

	cls.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	//bani - added clc.demoplaying, since some mods need this at init time, and drawactiveframe is too late for them
	VM_Call( cgvm, CG_INIT, clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum, clc.demoplaying );

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	Com_DPrintf( "CL_InitCGame: %5.2fs\n", ( t2 - t1 ) / 1000.0 );

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re.EndRegistration();

	// make sure everything is paged in
	if ( !Sys_LowPhysicalMemory() )
	{
		Com_TouchMemory();
	}

	// Ridah, update the memory usage file
	CL_UpdateLevelHunkUsage();

	CL_WriteClientLog( va("`~=-----------------=~`\n MAP: %s \n`~=-----------------=~`\n", mapname ) );

//  if( cl_autorecord->integer ) {
//      Cvar_Set( "g_synchronousClients", "1" );
//  }
}

void CL_InitCGameCVars( void )
{
	vm_t *cgv_vm = VM_Create( "cgame", CL_CgameSystemCalls, Cvar_VariableValue( "vm_cgame" ) );

	if ( !cgv_vm )
	{
		Com_Error( ERR_DROP, "VM_Create on cgame failed" );
	}

	VM_Call( cgv_vm, CG_INIT_CVARS );

	VM_Free( cgv_vm );
}

/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
qboolean CL_GameCommand( void )
{
	if ( !cgvm )
	{
		return qfalse;
	}

	return VM_Call( cgvm, CG_CONSOLE_COMMAND );
}

/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
qboolean CL_GameConsoleText( void )
{
	if ( !cgvm )
	{
		return qfalse;
	}

	return VM_Call( cgvm, CG_CONSOLE_TEXT );
}

/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering( stereoFrame_t stereo )
{
	/*  static int x = 0;
	        if(!((++x) % 20)) {
	                Com_Printf( "numtraces: %i\n", numtraces / 20 );
	                numtraces = 0;
	        } else {
	        }*/

	VM_Call( cgvm, CG_DRAW_ACTIVE_FRAME, cl.serverTime, stereo, clc.demoplaying );
	VM_Debug( 0 );
}

/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the Internet, it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

#define RESET_TIME 500

void CL_AdjustTimeDelta( void )
{
//	int             resetTime;
	int newDelta;
	int deltaDelta;

	cl.newSnapshots = qfalse;

	// the delta never drifts when replaying a demo
	if ( clc.demoplaying )
	{
		return;
	}

	// if the current time is WAY off, just correct to the current value

	/*
	        if(com_sv_running->integer)
	        {
	                resetTime = 100;
	        }
	        else
	        {
	                resetTime = RESET_TIME;
	        }
	*/

	newDelta = cl.snap.serverTime - cls.realtime;
	deltaDelta = abs( newDelta - cl.serverTimeDelta );

	if ( deltaDelta > RESET_TIME )
	{
		cl.serverTimeDelta = newDelta;
		cl.oldServerTime = cl.snap.serverTime; // FIXME: is this a problem for cgame?
		cl.serverTime = cl.snap.serverTime;

		if ( cl_showTimeDelta->integer )
		{
			Com_Printf( "<RESET> " );
		}
	}
	else if ( deltaDelta > 100 )
	{
		// fast adjust, cut the difference in half
		if ( cl_showTimeDelta->integer )
		{
			Com_Printf( "<FAST> " );
		}

		cl.serverTimeDelta = ( cl.serverTimeDelta + newDelta ) >> 1;
	}
	else
	{
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if ( com_timescale->value == 0 || com_timescale->value == 1 )
		{
			if ( cl.extrapolatedSnapshot )
			{
				cl.extrapolatedSnapshot = qfalse;
				cl.serverTimeDelta -= 2;
			}
			else
			{
				// otherwise, move our sense of time forward to minimize total latency
				cl.serverTimeDelta++;
			}
		}
	}

	if ( cl_showTimeDelta->integer )
	{
		Com_Printf("%i ", cl.serverTimeDelta );
	}
}

/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot( void )
{
	// ignore snapshots that don't have entities
	if ( cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE )
	{
		return;
	}

	cls.state = CA_ACTIVE;

	// set the timedelta so we are exactly on this first frame
	cl.serverTimeDelta = cl.snap.serverTime - cls.realtime;
	cl.oldServerTime = cl.snap.serverTime;

	clc.timeDemoBaseTime = cl.snap.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if ( cl_activeAction->string[ 0 ] )
	{
		Cbuf_AddText( cl_activeAction->string );
		Cbuf_AddText( "\n" );
		Cvar_Set( "activeAction", "" );
	}

#ifdef USE_MUMBLE

	if ( ( cl_useMumble->integer ) && !mumble_islinked() )
	{
		int ret = mumble_link( CLIENT_WINDOW_TITLE );
		Com_Printf("%s", ret == 0 ? _("Mumble: Linking to Mumble application okay\n") : _( "Mumble: Linking to Mumble application failed\n" ) );
	}

#endif

#ifdef USE_VOIP

	if ( !clc.speexInitialized )
	{
		int i;
		speex_bits_init( &clc.speexEncoderBits );
		speex_bits_reset( &clc.speexEncoderBits );

		clc.speexEncoder = speex_encoder_init( &speex_nb_mode );

		speex_encoder_ctl( clc.speexEncoder, SPEEX_GET_FRAME_SIZE,
		                   &clc.speexFrameSize );
		speex_encoder_ctl( clc.speexEncoder, SPEEX_GET_SAMPLING_RATE,
		                   &clc.speexSampleRate );

		clc.speexPreprocessor = speex_preprocess_state_init( clc.speexFrameSize,
		                        clc.speexSampleRate );

		i = 1;
		speex_preprocess_ctl( clc.speexPreprocessor,
		                      SPEEX_PREPROCESS_SET_DENOISE, &i );

		i = 1;
		speex_preprocess_ctl( clc.speexPreprocessor,
		                      SPEEX_PREPROCESS_SET_AGC, &i );

		for ( i = 0; i < MAX_CLIENTS; i++ )
		{
			speex_bits_init( &clc.speexDecoderBits[ i ] );
			speex_bits_reset( &clc.speexDecoderBits[ i ] );
			clc.speexDecoder[ i ] = speex_decoder_init( &speex_nb_mode );
			clc.voipIgnore[ i ] = qfalse;
			clc.voipGain[ i ] = 1.0f;
		}

		clc.speexInitialized = qtrue;
		clc.voipMuteAll = qfalse;
		Cmd_AddCommand( "voip", CL_Voip_f );
		Cvar_Set( "cl_voipSendTarget", "spatial" );
		Com_Memset( clc.voipTargets, ~0, sizeof( clc.voipTargets ) );
	}

#endif
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime( void )
{
	// getting a valid frame message ends the connection process
	if ( cls.state != CA_ACTIVE )
	{
		if ( cls.state != CA_PRIMED )
		{
			return;
		}

		if ( clc.demoplaying )
		{
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if ( !clc.firstDemoFrameSkipped )
			{
				clc.firstDemoFrameSkipped = qtrue;
				return;
			}

			CL_ReadDemoMessage();
		}

		if ( cl.newSnapshots )
		{
			cl.newSnapshots = qfalse;
			CL_FirstSnapshot();
		}

		if ( cls.state != CA_ACTIVE )
		{
			return;
		}
	}

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if ( !cl.snap.valid )
	{
		Com_Error( ERR_DROP, "CL_SetCGameTime: !cl.snap.valid" );
	}

	if ( sv_paused->integer && cl_paused->integer && com_sv_running->integer )
	{
		// paused
		return;
	}

	if ( cl.snap.serverTime < cl.oldFrameServerTime )
	{
		// Ridah, if this is a localhost, then we are probably loading a savegame
		if ( !Q_stricmp( cls.servername, "localhost" ) )
		{
			// do nothing?
			CL_FirstSnapshot();
		}
		else
		{
			Com_Error( ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime" );
		}
	}

	cl.oldFrameServerTime = cl.snap.serverTime;

	// get our current view of time

	if ( clc.demoplaying && cl_freezeDemo->integer )
	{
		// cl_freezeDemo is used to lock a demo in place for single frame advances
	}
	else
	{
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better
		// smoothness or better responsiveness.
		int tn;

		tn = cl_timeNudge->integer;

		if ( tn < -30 )
		{
			tn = -30;
		}
		else if ( tn > 30 )
		{
			tn = 30;
		}

		cl.serverTime = cls.realtime + cl.serverTimeDelta - tn;

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if ( cl.serverTime < cl.oldServerTime )
		{
			cl.serverTime = cl.oldServerTime;
		}

		cl.oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		if ( cls.realtime + cl.serverTimeDelta >= cl.snap.serverTime - 5 )
		{
			cl.extrapolatedSnapshot = qtrue;
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if ( cl.newSnapshots )
	{
		CL_AdjustTimeDelta();
	}

	if ( !clc.demoplaying )
	{
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definitely
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on
	if ( cl_timedemo->integer )
	{
		if ( !clc.timeDemoStart )
		{
			clc.timeDemoStart = Sys_Milliseconds();
		}

		clc.timeDemoFrames++;
		cl.serverTime = clc.timeDemoBaseTime + clc.timeDemoFrames * 50;
	}

	while ( cl.serverTime >= cl.snap.serverTime )
	{
		// feed another message, which should change
		// the contents of cl.snap
		CL_ReadDemoMessage();

		if ( cls.state != CA_ACTIVE )
		{
			Cvar_Set( "timescale", "1" );
			return; // end of demo
		}
	}
}

/*
====================
CL_GetTag
====================
*/
qboolean CL_GetTag( int clientNum, const char *tagname, orientation_t * or )
{
	if ( !cgvm )
	{
		return qfalse;
	}

	// the current design of CG_GET_TAG is inappropriate for modules in sandboxed formats
	//  (the direct pointer method to pass the tag name would work only with modules in native format)
	//return VM_Call( cgvm, CG_GET_TAG, clientNum, tagname, or );
	return qfalse;
}
