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
#include "../sys/sys_local.h"

#include "libmumblelink.h"
#include "../qcommon/crypto.h"

#include "../framework/CommonVMServices.h"
#include "../framework/CommandSystem.h"

#define __(x) Trans_GettextGame(x)
#define C__(x, y) Trans_PgettextGame(x, y)
#define P__(x, y, c) Trans_GettextGamePlural(x, y, c)

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
==================
CL_CGameStats
==================
*/
void CL_CGameStats( void )
{
	int nFrames = cl_cgameSyscallStats->integer;

	if (nFrames > 0 && cls.framecount % nFrames == 0 && cls.nCgameSyscalls != 0) {
		Com_Printf("Average over %i frames: %f syscalls (R: %f CM: %f U: %f S: %f)\n", nFrames,
				float(cls.nCgameSyscalls) / nFrames,
				float(cls.nCgameRenderSyscalls) / nFrames,
				float(cls.nCgamePhysicsSyscalls) / nFrames,
				float(cls.nCgameUselessSyscalls) / nFrames,
				float(cls.nCgameSoundSyscalls) / nFrames
			);
		cls.nCgameSyscalls = cls.nCgameRenderSyscalls = cls.nCgamePhysicsSyscalls = cls.nCgameUselessSyscalls = cls.nCgameSoundSyscalls = 0;
	}
}

/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified( Cmd::Args& csCmd )
{
	const char  *old, *s;
	int         i, index;
	const char  *dup;
	gameState_t oldGs;
	int         len;

	if (csCmd.Argc() < 3) {
		Com_Error( ERR_DROP, "CL_ConfigstringModified: wrong command received" );
	}

	index = atoi( csCmd.Argv(1).c_str() );

	if ( index < 0 || index >= MAX_CONFIGSTRINGS )
	{
		Com_Error( ERR_DROP, "CL_ConfigstringModified: bad index %i", index );
	}

//  s = Cmd_Argv(2);
	// get everything after "cs <num>"
	//s = Cmd_ArgsFrom( 2 );
	s = csCmd.Argv(2).c_str();

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
CL_HandleServerCommand
CL_GetServerCommand
===================
*/
bool CL_HandleServerCommand(Str::StringRef text) {
	static char bigConfigString[ BIG_INFO_STRING ];
	Cmd::Args args(text);

	if (args.Argc() == 0) {
		return qfalse;
	}

	auto cmd = args.Argv(0);
	int argc = args.Argc();

	if (cmd == "disconnect") {
		// NERVE - SMF - allow server to indicate why they were disconnected
		if (argc >= 2) {
			Com_Error(ERR_SERVERDISCONNECT, "Server disconnected: %s", args.Argv(1).c_str());
		} else {
			Com_Error(ERR_SERVERDISCONNECT, "Server disconnected");
		}
	}

	// bcs0 to bcs2 are used by the server to send info strings that are bigger than the size of a packet.
	// See also SV_UpdateConfigStrings
	// bcs0 starts a new big config string
	// bcs1 continues it
	// bcs2 finishes it and feeds it back as a new command sent by the server (bcs0 makes it a cs command)
	if (cmd == "bcs0") {
		if (argc >= 3) {
			Com_sprintf(bigConfigString, BIG_INFO_STRING, "cs %s %s", args.Argv(1).c_str(), args.EscapedArgs(2).c_str());
		}
		return qfalse;
	}

	if (cmd == "bcs1") {
		if (argc >= 3) {
			const char* s = Cmd_QuoteString( args[2].c_str() );

			if (strlen(bigConfigString) + strlen(s) >= BIG_INFO_STRING) {
				Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
			}

			Q_strcat(bigConfigString, sizeof(bigConfigString), s);
		}
		return qfalse;
	}

	if (cmd == "bcs2") {
		if (argc >= 3) {
			const char* s = Cmd_QuoteString( args[2].c_str() );

			if (strlen(bigConfigString) + strlen(s) + 1 >= BIG_INFO_STRING) {
				Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
			}

			Q_strcat(bigConfigString, sizeof(bigConfigString), s);
			Q_strcat(bigConfigString, sizeof(bigConfigString), "\"");
			return CL_HandleServerCommand(bigConfigString);
		}
		return qfalse;
	}

	if (cmd == "cs") {
		CL_ConfigstringModified(args);
		Cmd_TokenizeString(text.c_str());
		return qtrue;
	}

	if (cmd == "map_restart") {
		// clear outgoing commands before passing
		// the restart to the cgame
		memset(cl.cmds, 0, sizeof(cl.cmds));
		Cmd_TokenizeString(text.c_str());
		return qtrue;
	}

	if (cmd == "popup") {
		// direct server to client popup request, bypassing cgame
		if (cls.state == CA_ACTIVE && !clc.demoplaying && argc >=1) {
			Rocket_DocumentAction(args.Argv(1).c_str(), "open");
		}
		return qfalse;
	}

	if (cmd == "pubkey_decrypt") {
		char         buffer[ MAX_STRING_CHARS ] = "pubkey_identify ";
		unsigned int msg_len = MAX_STRING_CHARS - 16;
		mpz_t        message;

		if (argc == 1) {
			Com_Printf("%s", "^3Server sent a pubkey_decrypt command, but sent nothing to decrypt!\n");
			return qfalse;
		}

		mpz_init_set_str(message, args.Argv(1).c_str(), 16);

		if (rsa_decrypt(&private_key, &msg_len, (unsigned char *) buffer + 16, message)) {
			nettle_mpz_set_str_256_u(message, msg_len, (unsigned char *) buffer + 16);
			mpz_get_str(buffer + 16, 16, message);
			CL_AddReliableCommand(buffer);
		}

		mpz_clear(message);
		return qfalse;
	}

	Cmd_TokenizeString(text.c_str());
	return qtrue;
}

// Get the server command, does client-specific handling
// that may block the propagation of the command to cgame.
// If the propagation is not blocked then it tokenizes the
// command.
// Returns false if the command was blacked.
qboolean CL_GetServerCommand( int serverCommandNumber )
{
	const char  *s;

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

	return CL_HandleServerCommand(s);
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

	len = FS_FOpenFileRead( memlistfile, &handle, qfalse );

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
	void* buffer;
	FS_ReadFile( mapname, ( void ** ) &buffer );

	if ( !buffer )
	{
		Com_Error( ERR_DROP, "Couldn't load %s", mapname );
	}

	CM_LoadMap( mapname, buffer, qtrue );

	FS_FreeFile( buffer );
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
	cgvm->CGameShutdown();
	cgvm = nullptr;
}

//
// libRocket UI stuff
//

/*
 * ====================
 * GetClientState
 * ====================
 */
static void GetClientState( cgClientState_t *state )
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

	cls.numglobalservers = cls.numfavoriteservers = 0;
	cls.numGlobalServerAddresses = 0;

	if ( FS_FOpenFileRead( "servercache.dat", &fileIn, qtrue ) != -1 )
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

	fileOut = FS_FOpenFileWrite( "servercache.dat" );
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

		if ( Cmd_Argc() == 1 )
		{
			Com_Log(LOG_ERROR, "Server sent a pubkey_decrypt command, but sent nothing to decrypt!" );
			return qfalse;
		}

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

	Rocket_SetActiveContext( catcher );
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
	/*
	cls.nCgameSyscalls ++;

	switch ( args[ 0 ] )
	{
		case CG_PRINT:
			Com_Printf( "%s", ( char * ) VMA( 1 ) );
			return 0;

		case CG_ERROR:
			Com_Error( ERR_DROP, "%s", ( char * ) VMA( 1 ) );
			return 0; //silence warning and have a fallback behavior if Com_Error behavior changes

		case CG_LOG:
			Com_LogEvent( (log_event_t*) VMA( 1 ), NULL );
			return 0;

		case CG_MILLISECONDS:
			return Sys_Milliseconds();

		case CG_CVAR_REGISTER:
			Cvar_Register( (vmCvar_t*) VMA( 1 ), (char*) VMA( 2 ), (char*) VMA( 3 ), args[ 4 ] );
			return 0;

		case CG_CVAR_UPDATE:
			Cvar_Update( (vmCvar_t*) VMA( 1 ) );
			return 0;

		case CG_CVAR_SET:
			Cvar_Set( (char*) VMA( 1 ), (char*) VMA( 2 ) );
			return 0;

		case CG_CVAR_VARIABLESTRINGBUFFER:
			cls.nCgameUselessSyscalls ++;
			Cvar_VariableStringBuffer( (char*) VMA( 1 ), (char*) VMA( 2 ), args[ 3 ] );
			return 0;

		case CG_CVAR_LATCHEDVARIABLESTRINGBUFFER:
			cls.nCgameUselessSyscalls ++;
			Cvar_LatchedVariableStringBuffer( (char*) VMA( 1 ), (char*) VMA( 2 ), args[ 3 ] );
			return 0;

		case CG_CVAR_VARIABLEINTEGERVALUE:
			cls.nCgameUselessSyscalls ++;
			return Cvar_VariableIntegerValue( (char*) VMA( 1 ) );

		case CG_CVAR_VARIABLEVALUE:
			cls.nCgameUselessSyscalls ++;
			return FloatAsInt( Cvar_VariableValue( (char*) VMA( 1 ) ) );

		case CG_ARGC:
			cls.nCgameUselessSyscalls ++;
			return Cmd_Argc();

		case CG_ARGV:
			cls.nCgameUselessSyscalls ++;
			Cmd_ArgvBuffer( args[ 1 ], (char*) VMA( 2 ), args[ 3 ] );
			return 0;

		case CG_ESCAPED_ARGS:
			cls.nCgameUselessSyscalls ++;
			Cmd_EscapedArgsBuffer( (char*) VMA( 1 ), args[ 2 ] );
			return 0;

		case CG_LITERAL_ARGS:
			cls.nCgameUselessSyscalls ++;
			Cmd_LiteralArgsBuffer((char*) VMA( 1 ), args[ 2 ] );
			return 0;

		case CG_GETDEMOSTATE:
			return CL_DemoState();

		case CG_GETDEMOPOS:
			return CL_DemoPos();

		case CG_FS_FOPENFILE:
			return FS_Game_FOpenFileByMode( (char*) VMA( 1 ), (fileHandle_t*) VMA( 2 ), (fsMode_t) args[ 3 ] );

		case CG_FS_READ:
			FS_Read( VMA( 1 ), args[ 2 ], args[ 3 ] );
			return 0;

		case CG_FS_WRITE:
			return FS_Write( VMA( 1 ), args[ 2 ], args[ 3 ] );

		case CG_FS_FCLOSEFILE:
			FS_FCloseFile( args[ 1 ] );
			return 0;

		case CG_FS_GETFILELIST:
			return FS_GetFileList( (char*) VMA( 1 ), (char*) VMA( 2 ), (char*) VMA( 3 ), args[ 4 ] );

		case CG_FS_DELETEFILE:
			return FS_Delete( (char*) VMA( 1 ) );

		case CG_FS_LOADPAK:
			return FS_LoadPak( ( char * ) VMA( 1 ) );

		case CG_SENDCONSOLECOMMAND:
			Cmd::BufferCommandText( (char*) VMA( 1 ) );
			return 0;

		case CG_ADDCOMMAND:
			CL_AddCgameCommand( (char*) VMA( 1 ) );
			return 0;

		case CG_REMOVECOMMAND:
			Cmd_RemoveCommand( (char*) VMA( 1 ) );
			return 0;

		case CG_COMPLETE_CALLBACK:
			Cmd_OnCompleteMatch((char*) VMA(1));
			return 0;

		case CG_SENDCLIENTCOMMAND:
			CL_AddReliableCommand( (char*) VMA( 1 ) );
			return 0;

		case CG_UPDATESCREEN:
			SCR_UpdateScreen();
			return 0;

		case CG_CM_LOADMAP:
			cls.nCgamePhysicsSyscalls ++;
			CL_CM_LoadMap( (char*) VMA( 1 ) );
			return 0;

		case CG_CM_NUMINLINEMODELS:
			cls.nCgamePhysicsSyscalls ++;
			return CM_NumInlineModels();

		case CG_CM_INLINEMODEL:
			cls.nCgamePhysicsSyscalls ++;
			return CM_InlineModel( args[ 1 ] );

		case CG_CM_TEMPBOXMODEL:
			cls.nCgamePhysicsSyscalls ++;
			return CM_TempBoxModel( (float*) VMA( 1 ), (float*) VMA( 2 ), qfalse );

		case CG_CM_TEMPCAPSULEMODEL:
			cls.nCgamePhysicsSyscalls ++;
			return CM_TempBoxModel( (float*) VMA( 1 ), (float*) VMA( 2 ), qtrue );

		case CG_CM_POINTCONTENTS:
			cls.nCgamePhysicsSyscalls ++;
			return CM_PointContents( (float*) VMA( 1 ), args[ 2 ] );

		case CG_CM_TRANSFORMEDPOINTCONTENTS:
			cls.nCgamePhysicsSyscalls ++;
			return CM_TransformedPointContents( (float*) VMA( 1 ), args[ 2 ], (float*) VMA( 3 ),
			                                    (float*) VMA( 4 ) );

		case CG_CM_BOXTRACE:
			cls.nCgamePhysicsSyscalls ++;
			CM_BoxTrace( (trace_t*) VMA( 1 ), (float*) VMA( 2 ), (float*) VMA( 3 ),
			             (float*) VMA( 4 ), (float*) VMA( 5 ), args[ 6 ], args[ 7 ], args[ 8 ],
			             TT_AABB );
			return 0;

		case CG_CM_TRANSFORMEDBOXTRACE:
			cls.nCgamePhysicsSyscalls ++;
			CM_TransformedBoxTrace( (trace_t*) VMA( 1 ), (float*) VMA( 2 ), (float*) VMA( 3 ),
			                        (float*) VMA( 4 ), (float*) VMA( 5 ), args[ 6 ], args[ 7 ],
			                        args[ 8 ], (float*) VMA( 9 ), (float*) VMA( 10 ), TT_AABB );
			return 0;

		case CG_CM_CAPSULETRACE:
			cls.nCgamePhysicsSyscalls ++;
			CM_BoxTrace( (trace_t*) VMA( 1 ), (float*) VMA( 2 ), (float*) VMA( 3 ),
			             (float*) VMA( 4 ), (float*) VMA( 5 ), args[ 6 ], args[ 7 ], args[ 8 ],
			             TT_CAPSULE );
			return 0;

		case CG_CM_TRANSFORMEDCAPSULETRACE:
			cls.nCgamePhysicsSyscalls ++;
			CM_TransformedBoxTrace( (trace_t*) VMA( 1 ), (float*) VMA( 2 ), (float*) VMA( 3 ),
			                        (float*) VMA( 4 ), (float*) VMA( 5 ), args[ 6 ], args[ 7 ],
			                        args[ 8 ], (float*) VMA( 9 ), (float*) VMA( 10 ), TT_CAPSULE );
			return 0;

		case CG_CM_BISPHERETRACE:
			cls.nCgamePhysicsSyscalls ++;
			CM_BiSphereTrace( (trace_t*) VMA( 1 ), (float*) VMA( 2 ), (float*) VMA( 3 ), VMF( 4 ),
			                  VMF( 5 ), args[ 6 ], args[ 7 ], args[ 8 ] );
			return 0;

		case CG_CM_TRANSFORMEDBISPHERETRACE:
			cls.nCgamePhysicsSyscalls ++;
			CM_TransformedBiSphereTrace( (trace_t*) VMA( 1 ), (float*) VMA( 2 ), (float*) VMA( 3 ),
			                             VMF( 4 ), VMF( 5 ), args[ 6 ], args[ 7 ], args[ 8 ],
			                             (float*) VMA( 8 ) );
			return 0;

		case CG_CM_MARKFRAGMENTS:
			cls.nCgamePhysicsSyscalls ++;
			return re.MarkFragments( args[ 1 ], (vec3_t*) VMA( 2 ), (float*) VMA( 3 ), args[ 4 ],
			                         (float*) VMA( 5 ), args[ 6 ], (markFragment_t*) VMA( 7 ) );

		case CG_R_PROJECTDECAL:
			cls.nCgameRenderSyscalls ++;
			re.ProjectDecal( args[ 1 ], args[ 2 ], (vec3_t*) VMA( 3 ), (float*) VMA( 4 ),
			                 (float*) VMA( 5 ), args[ 6 ], args[ 7 ] );
			return 0;

		case CG_R_CLEARDECALS:
			cls.nCgameRenderSyscalls ++;
			re.ClearDecals();
			return 0;

		case CG_S_STARTSOUND:
			cls.nCgameSoundSyscalls ++;
			Audio::StartSound( args[ 2 ], (float*) VMA( 1 ), args[ 4 ] );
			return 0;

		case CG_S_STARTLOCALSOUND:
			cls.nCgameSoundSyscalls ++;
			Audio::StartLocalSound( args[ 1 ] );
			return 0;

		case CG_S_CLEARLOOPINGSOUNDS:
			cls.nCgameSoundSyscalls ++;
			Audio::ClearAllLoopingSounds();
			return 0;

		case CG_S_CLEARSOUNDS:
			cls.nCgameSoundSyscalls ++;
			return 0;

		case CG_S_ADDLOOPINGSOUND:
		case CG_S_ADDREALLOOPINGSOUND:
			cls.nCgameSoundSyscalls ++;
			if( args[ 2 ] )
			{
				Audio::UpdateEntityPosition( args[ 1 ], (float*) VMA( 2 ) );
			}
			if( args[ 3 ] )
			{
				Audio::UpdateEntityVelocity( args[ 1 ], (float*) VMA( 3 ) );
			}

			Audio::AddEntityLoopingSound( args[ 1 ], args[ 4 ]);
			return 0;

		case CG_S_STOPLOOPINGSOUND:
			cls.nCgameSoundSyscalls ++;
			Audio::ClearLoopingSoundsForEntity( args[ 1 ] );
			return 0;

		case CG_S_STOPSTREAMINGSOUND:
			cls.nCgameSoundSyscalls ++;
			// FIXME
			//S_StopEntStreamingSound(args[1]);
			return 0;

		case CG_S_UPDATEENTITYPOSITION:
			cls.nCgameSoundSyscalls ++;
			Audio::UpdateEntityPosition( args[ 1 ], (float*) VMA( 2 ) );
			return 0;

		case CG_S_RESPATIALIZE:
			cls.nCgameSoundSyscalls ++;
			if (args[ 1 ] >= 0 and args[ 1 ] < MAX_GENTITIES) {
				Audio::UpdateEntityPosition( args[ 1 ], (float*) VMA( 2 ));
			}
			Audio::UpdateListener(args[ 1 ], (vec3_t*) VMA( 3 ) );
			return 0;

		case CG_S_REGISTERSOUND:
			cls.nCgameSoundSyscalls ++;
			return Audio::RegisterSFX( (char*) VMA( 1 ) );

		case CG_S_STARTBACKGROUNDTRACK:
			cls.nCgameSoundSyscalls ++;
			Audio::StartMusic( (char*) VMA( 1 ), (char*) VMA( 2 ) );
			return 0;

		case CG_S_FADESTREAMINGSOUND:
			cls.nCgameSoundSyscalls ++;
			// FIXME
			//S_FadeStreamingSound(VMF(1), args[2], args[3]); //----(SA)  added music/all-streaming options
			return 0;

		case CG_S_STARTSTREAMINGSOUND:
			cls.nCgameSoundSyscalls ++;
			// FIXME
			//return S_StartStreamingSound(VMA(1), VMA(2), args[3], args[4], args[5]);
			return 0;

		case CG_R_LOADWORLDMAP:
			cls.nCgameRenderSyscalls ++;
			re.SetWorldVisData( CM_ClusterPVS( -1 ) );
			re.LoadWorld( (char*) VMA( 1 ) );
			return 0;

		case CG_R_REGISTERMODEL:
			cls.nCgameRenderSyscalls ++;
			return re.RegisterModel( (char*) VMA( 1 ) );

		case CG_R_REGISTERSKIN:
			cls.nCgameRenderSyscalls ++;
			return re.RegisterSkin( (char*) VMA( 1 ) );

			//----(SA)  added
		case CG_R_GETSKINMODEL:
			cls.nCgameRenderSyscalls ++;
			return re.GetSkinModel( args[ 1 ], (char*) VMA( 2 ), (char*) VMA( 3 ) );

		case CG_R_GETMODELSHADER:
			cls.nCgameRenderSyscalls ++;
			return re.GetShaderFromModel( args[ 1 ], args[ 2 ], args[ 3 ] );
			//----(SA)  end

		case CG_R_REGISTERSHADER:
			cls.nCgameRenderSyscalls ++;
			return re.RegisterShader( (char*) VMA( 1 ), (RegisterShaderFlags_t) args[ 2 ] );

		case CG_R_REGISTERFONT:
			cls.nCgameRenderSyscalls ++;
			re.RegisterFontVM( (char*) VMA( 1 ), (char*) VMA( 2 ), args[ 3 ], (fontMetrics_t*) VMA( 4 ) );
			return 0;

		case CG_R_CLEARSCENE:
			cls.nCgameRenderSyscalls ++;
			re.ClearScene();
			return 0;

		case CG_R_ADDREFENTITYTOSCENE:
			cls.nCgameRenderSyscalls ++;
			re.AddRefEntityToScene( (refEntity_t*) VMA( 1 ) );
			return 0;

		case CG_R_ADDREFLIGHTSTOSCENE:
			cls.nCgameRenderSyscalls ++;
			re.AddRefLightToScene( (refLight_t*) VMA( 1 ) );
			return 0;

		case CG_R_ADDPOLYTOSCENE:
			cls.nCgameRenderSyscalls ++;
			re.AddPolyToScene( args[ 1 ], args[ 2 ], (polyVert_t*) VMA( 3 ) );
			return 0;

		case CG_R_ADDPOLYSTOSCENE:
			cls.nCgameRenderSyscalls ++;
			re.AddPolysToScene( args[ 1 ], args[ 2 ], (polyVert_t*) VMA( 3 ), args[ 4 ] );
			return 0;

		case CG_R_ADDPOLYBUFFERTOSCENE:
			cls.nCgameRenderSyscalls ++;
			re.AddPolyBufferToScene( (polyBuffer_t*) VMA( 1 ) );
			return 0;

		case CG_R_ADDLIGHTTOSCENE:
			cls.nCgameRenderSyscalls ++;
			re.AddLightToScene( (float*) VMA( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), args[ 7 ], args[ 8 ] );
			return 0;

		case CG_R_ADDADDITIVELIGHTTOSCENE:
			cls.nCgameRenderSyscalls ++;
			re.AddAdditiveLightToScene( (float*) VMA( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ) );
			return 0;

		case CG_FS_SEEK:
			return FS_Seek( args[ 1 ], args[ 2 ], args[ 3 ] );

		case CG_R_ADDCORONATOSCENE:
			cls.nCgameRenderSyscalls ++;
			re.AddCoronaToScene( (float*) VMA( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), args[ 6 ], args[ 7 ] );
			return 0;

		case CG_R_SETFOG:
			cls.nCgameRenderSyscalls ++;
			re.SetFog( args[ 1 ], args[ 2 ], args[ 3 ], VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ) );
			return 0;

		case CG_R_SETGLOBALFOG:
			cls.nCgameRenderSyscalls ++;
			re.SetGlobalFog( args[ 1 ], args[ 2 ], VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ) );
			return 0;

		case CG_R_RENDERSCENE:
			cls.nCgameRenderSyscalls ++;
			re.RenderScene( (refdef_t*) VMA( 1 ) );
			return 0;

		case CG_R_SAVEVIEWPARMS:
			cls.nCgameRenderSyscalls ++;
			re.SaveViewParms();
			return 0;

		case CG_R_RESTOREVIEWPARMS:
			cls.nCgameRenderSyscalls ++;
			re.RestoreViewParms();
			return 0;

		case CG_R_SETCOLOR:
			cls.nCgameRenderSyscalls ++;
			re.SetColor( (float*) VMA( 1 ) );
			return 0;

			// Tremulous
		case CG_R_SETCLIPREGION:
			cls.nCgameRenderSyscalls ++;
			re.SetClipRegion( (float*) VMA( 1 ) );
			return 0;

		case CG_R_DRAWSTRETCHPIC:
			cls.nCgameRenderSyscalls ++;
			re.DrawStretchPic( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ), VMF( 8 ), args[ 9 ] );
			return 0;

		case CG_R_DRAWROTATEDPIC:
			cls.nCgameRenderSyscalls ++;
			re.DrawRotatedPic( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ), VMF( 8 ), args[ 9 ], VMF( 10 ) );
			return 0;

		case CG_R_DRAWSTRETCHPIC_GRADIENT:
			cls.nCgameRenderSyscalls ++;
			re.DrawStretchPicGradient( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ), VMF( 8 ), args[ 9 ], (float*) VMA( 10 ), args[ 11 ] );
			return 0;

		case CG_R_DRAW2DPOLYS:
			cls.nCgameRenderSyscalls ++;
			re.Add2dPolys( (polyVert_t*) VMA( 1 ), args[ 2 ], args[ 3 ] );
			return 0;

		case CG_R_MODELBOUNDS:
			cls.nCgameRenderSyscalls ++;
			re.ModelBounds( args[ 1 ], (float*) VMA( 2 ), (float*) VMA( 3 ) );
			return 0;

		case CG_R_LERPTAG:
			cls.nCgameRenderSyscalls ++;
			return re.LerpTag( (orientation_t*) VMA( 1 ), (refEntity_t*) VMA( 2 ), (char*) VMA( 3 ), args[ 4 ] );

		case CG_GETGLCONFIG:
			CL_GetGlconfig( (glconfig_t*) VMA( 1 ) );
			return 0;

		case CG_GETGAMESTATE:
			CL_GetGameState( (gameState_t*) VMA( 1 ) );
			return 0;

		case CG_GETCLIENTSTATE:
			GetClientState( (cgClientState_t*) VMA( 1 ) );
			return 0;

		case CG_GETCURRENTSNAPSHOTNUMBER:
			CL_GetCurrentSnapshotNumber( (int*) VMA( 1 ), (int*) VMA( 2 ) );
			return 0;

		case CG_GETSNAPSHOT:
			return CL_GetSnapshot( args[ 1 ], (snapshot_t*) VMA( 2 ) );

		case CG_GETSERVERCOMMAND:
			return CL_GetServerCommand( args[ 1 ] );

		case CG_GETCURRENTCMDNUMBER:
			return CL_GetCurrentCmdNumber();

		case CG_GETUSERCMD:
			return CL_GetUserCmd( args[ 1 ], (usercmd_t*) VMA( 2 ) );

		case CG_SETUSERCMDVALUE:
			CL_SetUserCmdValue( args[ 1 ], args[ 2 ], VMF( 3 ), args[ 4 ] );
			return 0;

		case CG_SETCLIENTLERPORIGIN:
			CL_SetClientLerpOrigin( VMF( 1 ), VMF( 2 ), VMF( 3 ) );
			return 0;

		case CG_MEMORY_REMAINING:
			cls.nCgameUselessSyscalls ++;
			return Hunk_MemoryRemaining();

		case CG_KEY_ISDOWN:
			return Key_IsDown( args[ 1 ] );

		case CG_KEY_GETCATCHER:
			return Key_GetCatcher();

		case CG_KEY_SETCATCHER:
			Key_SetCatcher( args[ 1 ] );
			return 0;

		case CG_KEY_GETKEY:
			return Key_GetKey( (char*) VMA( 1 ), 0 ); // FIXME BIND

		case CG_KEY_GETOVERSTRIKEMODE:
			return Key_GetOverstrikeMode();

		case CG_KEY_SETOVERSTRIKEMODE:
			Key_SetOverstrikeMode( args[ 1 ] );
			return 0;

		case CG_S_STOPBACKGROUNDTRACK:
			cls.nCgameSoundSyscalls ++;
			Audio::StopMusic();
			return 0;

		case CG_REAL_TIME:
			return Com_RealTime( (qtime_t*) VMA( 1 ) );

		case CG_SNAPVECTOR:
			cls.nCgameUselessSyscalls ++;
			SnapVector( (float*) VMA( 1 ) );
			return 0;

		case CG_CIN_PLAYCINEMATIC:
			return CIN_PlayCinematic( (char*) VMA( 1 ), args[ 2 ], args[ 3 ], args[ 4 ], args[ 5 ], args[ 6 ] );

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
			cls.nCgameRenderSyscalls ++;
			re.RemapShader( (char*) VMA( 1 ), (char*) VMA( 2 ), (char*) VMA( 3 ) );
			return 0;

		case CG_GET_ENTITY_TOKEN:
			return re.GetEntityToken( (char*) VMA( 1 ), args[ 2 ] );

		case CG_INGAME_POPUP:
			if ( cls.state == CA_ACTIVE && !clc.demoplaying )
			{
				Rocket_DocumentAction( (const char *) VMA( 1 ), "open" );
			}

			return 0;

		case CG_INGAME_CLOSEPOPUP:
			return 0;

		case CG_KEY_GETBINDINGBUF:
			Key_GetBindingBuf( args[ 1 ], args[ 2 ], (char*) VMA( 3 ), args[ 4 ] );
			return 0;

		case CG_KEY_SETBINDING:
			Key_SetBinding( args[ 1 ], args[ 2 ], (char*) VMA( 3 ) ); // FIXME BIND
			return 0;

		case CG_PARSE_ADD_GLOBAL_DEFINE:
			cls.nCgameUselessSyscalls ++;
			return Parse_AddGlobalDefine( (char*) VMA( 1 ) );

		case CG_PARSE_LOAD_SOURCE:
			cls.nCgameUselessSyscalls ++;
			return Parse_LoadSourceHandle( (char*) VMA( 1 ) );

		case CG_PARSE_FREE_SOURCE:
			cls.nCgameUselessSyscalls ++;
			return Parse_FreeSourceHandle( args[ 1 ] );

		case CG_PARSE_READ_TOKEN:
			cls.nCgameUselessSyscalls ++;
			return Parse_ReadTokenHandle( args[ 1 ], (pc_token_t*) VMA( 2 ) );

		case CG_PARSE_SOURCE_FILE_AND_LINE:
			cls.nCgameUselessSyscalls ++;
			return Parse_SourceFileAndLine( args[ 1 ], (char*) VMA( 2 ), (int*) VMA( 3 ) );

		case CG_KEY_KEYNUMTOSTRINGBUF:
			cls.nCgameUselessSyscalls ++;
			Key_KeynumToStringBuf( args[ 1 ], (char*) VMA( 2 ), args[ 3 ] );
			return 0;

		case CG_S_FADEALLSOUNDS:
			cls.nCgameSoundSyscalls ++;
			// FIXME
			//S_FadeAllSounds(VMF(1), args[2], args[3]);
			return 0;

		case CG_R_INPVS:
			cls.nCgameRenderSyscalls ++;
			return re.inPVS( (float*) VMA( 1 ), (float*) VMA( 2 ) );

		case CG_R_INPVVS:
			cls.nCgameRenderSyscalls ++;
			return re.inPVVS( (float*) VMA( 1 ), (float*) VMA( 2 ) );

		case CG_GETHUNKDATA:
			cls.nCgameUselessSyscalls ++;
			Com_GetHunkInfo( (int*) VMA( 1 ), (int*) VMA( 2 ) );
			return 0;

			//bani - dynamic shaders
		case CG_R_LOADDYNAMICSHADER:
			cls.nCgameRenderSyscalls ++;
			return re.LoadDynamicShader( (char*) VMA( 1 ), (char*) VMA( 2 ) );

			// fretn - render to texture
		case CG_R_RENDERTOTEXTURE:
			cls.nCgameRenderSyscalls ++;
			re.RenderToTexture( args[ 1 ], args[ 2 ], args[ 3 ], args[ 4 ], args[ 5 ] );
			return 0;

		//bani - flush gl rendering buffers
		case CG_R_FINISH:
			cls.nCgameRenderSyscalls ++;
			re.Finish();
			return 0;

		case CG_GETDEMONAME:
			CL_DemoName( (char*) VMA( 1 ), args[ 2 ] );
			return 0;

		case CG_R_LIGHTFORPOINT:
			cls.nCgameRenderSyscalls ++;
			return re.LightForPoint( (float*) VMA( 1 ), (float*) VMA( 2 ), (float*) VMA( 3 ), (float*) VMA( 4 ) );

		case CG_R_REGISTERANIMATION:
			cls.nCgameRenderSyscalls ++;
			return re.RegisterAnimation( (char*) VMA( 1 ) );

		case CG_R_CHECKSKELETON:
			cls.nCgameRenderSyscalls ++;
			return re.CheckSkeleton( (refSkeleton_t*) VMA( 1 ), args[ 2 ], args[ 3 ] );

		case CG_R_BUILDSKELETON:
			cls.nCgameRenderSyscalls ++;
			return re.BuildSkeleton( (refSkeleton_t*) VMA( 1 ), args[ 2 ], args[ 3 ], args[ 4 ], VMF( 5 ), args[ 6 ] );

		case CG_R_BLENDSKELETON:
			cls.nCgameRenderSyscalls ++;
			return re.BlendSkeleton( (refSkeleton_t*) VMA( 1 ), (refSkeleton_t*) VMA( 2 ), VMF( 3 ) );

		case CG_R_BONEINDEX:
			cls.nCgameRenderSyscalls ++;
			return re.BoneIndex( args[ 1 ], (char*) VMA( 2 ) );

		case CG_R_ANIMNUMFRAMES:
			cls.nCgameRenderSyscalls ++;
			return re.AnimNumFrames( args[ 1 ] );

		case CG_R_ANIMFRAMERATE:
			cls.nCgameRenderSyscalls ++;
			return re.AnimFrameRate( args[ 1 ] );

		case CG_REGISTER_BUTTON_COMMANDS:
			CL_RegisterButtonCommands( (char*) VMA( 1 ) );
			return 0;

		case CG_GETCLIPBOARDDATA:

			if ( cl_allowPaste->integer )
			{
				CL_GetClipboardData( (char*) VMA(1), args[2], (clipboard_t) args[3] );
			}
			else
			{
				( (char *) VMA( 1 ) )[0] = '\0';
			}
			return 0;

		case CG_QUOTESTRING:
			cls.nCgameUselessSyscalls ++;
			Cmd_QuoteStringBuffer( (char*) VMA( 1 ), (char*) VMA( 2 ), args[ 3 ] );
			return 0;

		case CG_GETTEXT:
			cls.nCgameUselessSyscalls ++;
			Q_strncpyz( (char*) VMA(1), __( (char*) VMA( 2 ) ), args[3] );
			return 0;

		case CG_PGETTEXT:
			cls.nCgameUselessSyscalls ++;
			Q_strncpyz( (char*) VMA( 1 ), C__( (char*) VMA( 2 ), (char*) VMA( 3 ) ), args[ 4 ] );
			return 0;

		case CG_GETTEXT_PLURAL:
			cls.nCgameUselessSyscalls ++;
			Q_strncpyz( (char*) VMA( 1 ), P__( (char*) VMA( 2 ), (char*) VMA( 3 ), args[ 4 ] ), args[ 5 ] );
			return 0;

		case CG_R_GLYPH:
			cls.nCgameRenderSyscalls ++;
			re.GlyphVM( args[1], (char*) VMA(2), (glyphInfo_t*) VMA(3) );
			return 0;

		case CG_R_GLYPHCHAR:
			cls.nCgameRenderSyscalls ++;
			re.GlyphCharVM( args[1], args[2], (glyphInfo_t*) VMA(3) );
			return 0;

		case CG_R_UREGISTERFONT:
			cls.nCgameRenderSyscalls ++;
			re.UnregisterFontVM( args[1] );
			return 0;

		case CG_NOTIFY_TEAMCHANGE:
			CL_OnTeamChanged( args[1] );
			return 0;

		case CG_REGISTERVISTEST:
			return re.RegisterVisTest();

		case CG_ADDVISTESTTOSCENE:
			re.AddVisTestToScene( args[1], (float*) VMA(2), VMF(3), VMF(4) );
			return 0;

		case CG_CHECKVISIBILITY:
			cls.nCgameRenderSyscalls ++;
			return FloatAsInt( re.CheckVisibility( args[1] ) );

		case CG_UNREGISTERVISTEST:
			cls.nCgameRenderSyscalls ++;
			re.UnregisterVisTest( args[1] );
			return 0;

		case CG_SETCOLORGRADING:
			cls.nCgameRenderSyscalls ++;
			re.SetColorGrading( args[1], args[2] );
			return 0;

		case CG_CM_DISTANCETOMODEL:
			cls.nCgamePhysicsSyscalls ++;
			return FloatAsInt( CM_DistanceToModel( (float*) VMA(1), args[2] ) );

		case CG_R_SCISSOR_ENABLE:
			cls.nCgameRenderSyscalls ++;
			re.ScissorEnable( args[1] );
			return 0;

		case CG_R_SCISSOR_SET:
			cls.nCgameRenderSyscalls ++;
			re.ScissorSet( args[1], args[2], args[3], args[4] );
			return 0;

		case CG_R_GETSHADERNAMEFROMHANDLE:
			Q_strncpyz( (char *) VMA( 2 ), re.ShaderNameFromHandle( args[ 1 ] ), args[ 3 ] );
			return 0;

		case CG_ROCKET_INIT:
			Rocket_Init();
			return 0;

		case CG_ROCKET_SHUTDOWN:
			Rocket_Shutdown();
			return 0;

		case CG_ROCKET_LOADDOCUMENT:
			Rocket_LoadDocument( (const char *) VMA(1) );
			return 0;

		case CG_ROCKET_LOADCURSOR:
			Rocket_LoadCursor( (const char *) VMA(1) );
			return 0;

		case CG_ROCKET_DOCUMENTACTION:
			Rocket_DocumentAction( (const char *) VMA(1), (const char *) VMA(2) );
			return 0;

		case CG_ROCKET_GETEVENT:
			return Rocket_GetEvent();

		case CG_ROCKET_DELELTEEVENT:
			Rocket_DeleteEvent();
			return 0;

		case CG_ROCKET_REGISTERDATASOURCE:
			Rocket_RegisterDataSource( (const char *) VMA(1) );
			return 0;

		case CG_ROCKET_DSADDROW:
			Rocket_DSAddRow( (const char *) VMA(1), (const char *) VMA(2), (const char *) VMA(3) );
			return 0;

		case CG_LAN_LOADCACHEDSERVERS:
			LAN_LoadCachedServers();
			return 0;

		case CG_LAN_SAVECACHEDSERVERS:
			LAN_SaveServersToCache();
			return 0;

		case CG_LAN_ADDSERVER:
			return LAN_AddServer( args[ 1 ], (const char *) VMA( 2 ), (const char *) VMA( 3 ) );

		case CG_LAN_REMOVESERVER:
			LAN_RemoveServer( args[ 1 ], (const char *) VMA( 2 ) );
			return 0;

		case CG_LAN_GETPINGQUEUECOUNT:
			return LAN_GetPingQueueCount();

		case CG_LAN_CLEARPING:
			LAN_ClearPing( args[ 1 ] );
			return 0;

		case CG_LAN_GETPING:
			LAN_GetPing( args[ 1 ], (char *)VMA( 2 ), args[ 3 ], (int*) VMA( 4 ) );
			return 0;

		case CG_LAN_GETPINGINFO:
			LAN_GetPingInfo( args[ 1 ], (char *) VMA( 2 ), args[ 3 ] );
			return 0;

		case CG_LAN_GETSERVERCOUNT:
			return LAN_GetServerCount( args[ 1 ] );

		case CG_LAN_GETSERVERADDRESSSTRING:
			LAN_GetServerAddressString( args[ 1 ], args[ 2 ], (char *) VMA( 3 ), args[ 4 ] );
			return 0;

		case CG_LAN_GETSERVERINFO:
			LAN_GetServerInfo( args[ 1 ], args[ 2 ], (char *) VMA( 3 ), args[ 4 ] );
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
			return LAN_GetServerStatus( (char *) VMA( 1 ), (char *) VMA( 2 ), args[ 3 ] );

		case CG_LAN_SERVERISINFAVORITELIST:
			return LAN_ServerIsInFavoriteList( args[ 1 ], args[ 2 ] );

		case CG_GETNEWS:
			return GetNews( args[ 1 ] );

		case CG_LAN_COMPARESERVERS:
			return LAN_CompareServers( args[ 1 ], args[ 2 ], args[ 3 ], args[ 4 ], args[ 5 ] );

		case CG_ROCKET_DSCLEARTABLE:
			Rocket_DSClearTable( (const char *) VMA(1), (const char *) VMA(2) );
			return 0;

		case CG_ROCKET_SETINNERRML:
			Rocket_SetInnerRML( "", "", (const char *) VMA(1), args[2] );
			return 0;

		case CG_ROCKET_GETEVENTPARAMETERS:
			Rocket_GetEventParameters( (char *) VMA(1), args[2] );
			return 0;

		case CG_ROCKET_REGISTERDATAFORMATTER:
			Rocket_RegisterDataFormatter( (const char *) VMA(1) );
			return 0;

		case CG_ROCKET_DATAFORMATTERRAWDATA:
			Rocket_DataFormatterRawData( args[1], (char *) VMA(2), args[3], (char *) VMA(4), args[5] );
			return 0;

		case CG_ROCKET_DATAFORMATTERFORMATTEDDATA:
			Rocket_DataFormatterFormattedData( args[1], (const char *) VMA(2), args[3] );
			return 0;

		case CG_ROCKET_GETATTRIBUTE:
			Rocket_GetAttribute( "", "", (const char *) VMA(1), (char *) VMA(2), args[3] );
			return 0;

		case CG_ROCKET_SETATTRIBUTE:
			Rocket_SetAttribute( "", "", (const char *) VMA(1), (const char *) VMA(2) );
			return 0;

		case CG_ROCKET_GETPROPERTY:
			Rocket_GetProperty( (const char *) VMA(1), VMA(2), args[ 3 ], (rocketVarType_t) args[ 4 ] );
			return 0;

		case CG_ROCKET_REGISTERELEMENT:
			Rocket_RegisterElement( (const char *) VMA(1) );
			return 0;

		case CG_ROCKET_SETELEMENTDIMENSIONS:
			Rocket_SetElementDimensions( VMF(1), VMF(2) );
			return 0;

		case CG_ROCKET_GETELEMENTTAG:
			Rocket_GetElementTag( (char *)VMA(1), args[ 2 ] );
			return 0;

		case CG_ROCKET_GETELEMENTABSOLUTEOFFSET:
			Rocket_GetElementAbsoluteOffset( (float*) VMA(1), (float*) VMA(2) );
			return 0;

		case CG_ROCKET_QUAKETORML:
			Rocket_QuakeToRMLBuffer( (const char *) VMA(1), (char *) VMA(2), args[3] );
			return 0;

		case CG_ROCKET_SETCLASS:
			Rocket_SetClass( (const char *) VMA(1), args[ 2 ] );
			return 0;

		case CG_ROCKET_SETPROPERYBYID:
			Rocket_SetPropertyById( "", (const char *) VMA(1), (const char *) VMA(2) );
			return 0;

		case CG_ROCKET_INITHUDS:
			Rocket_InitializeHuds( args[1] );
			return 0;

		case CG_ROCKET_LOADUNIT:
			Rocket_LoadUnit( (const char *) VMA(1) );
			return 0;

		case CG_ROCKET_ADDUNITTOHUD:
			Rocket_AddUnitToHud( args[1], (const char *) VMA(2) );
			return 0;

		case CG_ROCKET_SHOWHUD:
			Rocket_ShowHud( args[1] );
			return 0;

		case CG_ROCKET_CLEARHUD:
			Rocket_ClearHud( args[1] );
			return 0;

		case CG_ROCKET_ADDTEXT:
			Rocket_AddTextElement( ( const char * ) VMA(1), ( const char *) VMA(2), VMF(3), VMF(4) );
			return 0;

		case CG_ROCKET_CLEARTEXT:
			Rocket_ClearText();
			return 0;

		case CG_PREPAREKEYUP:
			IN_PrepareKeyUp();
			return 0;

		case CG_R_SETALTSHADERTOKENS:
			cls.nCgameRenderSyscalls ++;
			re.SetAltShaderTokens( ( const char * )VMA(1) );
			return 0;

		case CG_S_UPDATEENTITYVELOCITY:
			cls.nCgameSoundSyscalls ++;
			Audio::UpdateEntityVelocity( args[ 1 ], (float*) VMA( 2 ) );
			return 0;

		case CG_S_SETREVERB:
			cls.nCgameSoundSyscalls ++;
			Audio::SetReverb( args[ 1 ], (const char*) VMA( 2 ), VMF( 3 ) );
			return 0;

		case CG_S_BEGINREGISTRATION:
			cls.nCgameSoundSyscalls ++;
			Audio::BeginRegistration();
			return 0;

		case CG_S_ENDREGISTRATION:
			cls.nCgameSoundSyscalls ++;
			Audio::EndRegistration();
			return 0;
		case CG_ROCKET_REGISTERPROPERTY:
			Rocket_RegisterProperty( ( const char * ) VMA( 1 ), ( const char * ) VMA( 2 ), args[ 3 ], args[ 4 ], ( const char * ) VMA( 5 ) );
			return 0;

		case CG_ROCKET_SHOWSCOREBOARD:
			Rocket_ShowScoreboard( ( const char * ) VMA( 1 ), args[ 2 ] );
			return 0;

		case CG_ROCKET_SETDATASELECTINDEX:
			Rocket_SetDataSelectIndex( args[ 1 ] );
			return 0;

		default:
			Com_Error( ERR_DROP, "Bad cgame system trap: %ld", ( long int ) args[ 0 ] );
			exit(1); // silence warning, and make sure this behaves as expected, if Com_Error's behavior changes
	}
*/
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
	const char *memlistfile = "hunkusage.dat";
	char *buf, *outbuf;
	char *buftrav, *outbuftrav;
	char *token;
	char outstr[ 256 ];
	int  len, memusage;

	memusage = Cvar_VariableIntegerValue( "com_hunkused" ) + Cvar_VariableIntegerValue( "hunk_soundadjust" );

	len = FS_FOpenFileRead( memlistfile, &handle, qfalse );

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
	handle = FS_FOpenFileAppend( memlistfile );

	if ( handle == 0 )
	{
		Com_Error( ERR_DROP, "cannot write to hunkusage.dat, check disk full" );
	}

	Com_sprintf( outstr, sizeof( outstr ), "%s %i\n", cl.mapname, memusage );
	FS_Write( outstr, strlen( outstr ), handle );
	FS_FCloseFile( handle );

	// now just open it and close it, so it gets copied to the pak dir
	len = FS_FOpenFileRead( memlistfile, &handle, qfalse );

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

std::unique_ptr<CGameVM> CL_InitUI( void )
{
	auto vm = std::unique_ptr<CGameVM>(new CGameVM());

	if (vm->Start()) {
		return vm;
	}
	Com_Error(ERR_DROP, "Couldn't load the cgame VM");

	return nullptr;
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


	cls.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	//bani - added clc.demoplaying, since some mods need this at init time, and drawactiveframe is too late for them
	cgvm->CGameInit(clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum, clc.demoplaying);

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	Com_DPrintf( "CL_InitCGame: %5.2fs\n", ( t2 - t1 ) / 1000.0 );

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re.EndRegistration();

	// make sure everything is paged in
	Com_TouchMemory();

	// Ridah, update the memory usage file
	CL_UpdateLevelHunkUsage();

	// Cause any input while loading to be dropped and forget what's pressed
	IN_DropInputsForFrame();
	CL_ClearKeys();
	Key_ClearStates();
}

void CL_InitCGameCVars( void )
{/* TODO I don't understand that
	vm_t *cgv_vm = VM_Create( "cgame", CL_CgameSystemCalls, (vmInterpret_t) Cvar_VariableIntegerValue( "vm_cgame" ) );

	if ( !cgv_vm )
	{
		Com_Error( ERR_DROP, "VM_Create on cgame failed" );
	}

	VM_Call( cgv_vm, CG_INIT_CVARS );

	VM_Free( cgv_vm );*/
}

/*
====================
CL_GameCommandHandler
====================
*/
void CL_GameCommandHandler( void )
{
	/* TODO this will be useless
	VM_Call( cgvm, CG_CONSOLE_COMMAND );
	*/
}

/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering( stereoFrame_t stereo )
{
	cgvm->CGameDrawActiveFrame(cl.serverTime, stereo, clc.demoplaying);
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
		Cmd::BufferCommandText(cl_activeAction->string);
		Cvar_Set( "activeAction", "" );
	}

	if ( ( cl_useMumble->integer ) && !mumble_islinked() )
	{
		int ret = mumble_link( CLIENT_WINDOW_TITLE );
		Com_Printf("%s", ret == 0 ? "Mumble: Linking to Mumble application okay\n" : "Mumble: Linking to Mumble application failed\n" );
	}

#ifdef USE_VOIP

	if ( !clc.speexInitialized )
	{
		int i;
		speex_bits_init( &clc.speexEncoderBits );
		speex_bits_reset( &clc.speexEncoderBits );

		clc.speexEncoder = speex_encoder_init( speex_lib_get_mode( SPEEX_MODEID_NB ) );

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
			clc.speexDecoder[ i ] = speex_decoder_init( speex_lib_get_mode( SPEEX_MODEID_NB ) );
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
qboolean CL_GetTag( int clientNum, const char *tagname, orientation_t * orientation )
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

/**
 * is notified by teamchanges.
 * while most notifications will come from the cgame, due to game semantics,
 * other code may assume a change to a non-team "0", like e.g. /disconnect
 */
void  CL_OnTeamChanged( int newTeam )
{
	if(p_team->integer == newTeam
			&& p_team->modificationCount > 0 ) //to make sure, we run the hook initially as well
	{
		return;
	}

	Cvar_SetValue( p_team->name, newTeam );

	/* set all team specific teambindinds */
	Key_SetTeam( newTeam );

	/*
	 * execute a possibly team aware config each time the team was changed.
	 * the user can use the cvars p_team or p_teamname (if the cgame sets it) within that config
	 * to e.g. execute team specific configs, like cg_<team>Config did previously, but with less dependency on the cgame
	 */
	Cmd::BufferCommandText( "exec -f " TEAMCONFIG_NAME );
}

static VM::VMParams cgameParams("cgame");

CGameVM::CGameVM(): VM::VMBase("cgame", cgameParams), services(new VM::CommonVMServices(*this, "CGame", Cmd::CGAME_VM))
{
}

bool CGameVM::Start()
{
	int version = this->Create();

	if (version < 0)
	{
		return false;
	}

	if ( version != CGAME_API_VERSION ) {
		Com_Error( ERR_DROP, "CGame ABI mismatch, expected %d, got %d", CGAME_API_VERSION, version );
	}

	return true;
}

CGameVM::~CGameVM()
{
	this->Free();
}

void CGameVM::CGameInit(int serverMessageNum, int serverCommandSequence, int clientNum, int demoplaying)
{
	this->SendMsg<CGameInitMsg>(serverMessageNum, serverCommandSequence, clientNum, demoplaying);
}

void CGameVM::CGameShutdown()
{
	this->SendMsg<CGameShutdownMsg>();
}

void CGameVM::CGameDrawActiveFrame(int serverTime, stereoFrame_t stereoView, bool demoPlayback)
{
	this->SendMsg<CGameDrawActiveFrameMsg>(serverTime, stereoView, demoPlayback);
}

int CGameVM::CGameCrosshairPlayer()
{
	int player;
	this->SendMsg<CGameCrosshairPlayerMsg>(player);
	return player;
}

void CGameVM::CGameKeyEvent(int key, bool down)
{
	this->SendMsg<CGameKeyEventMsg>(key, down);
}

void CGameVM::CGameMouseEvent(int dx, int dy)
{
	this->SendMsg<CGameMouseEventMsg>(dx, dy);
}

std::vector<std::string> CGameVM::CGameVoipString()
{
	std::vector<std::string> strings;
	this->SendMsg<CGameVoipStringMsg>(strings);
	return std::move(strings);
}

void CGameVM::CGameInitCvars()
{
	this->SendMsg<CGameInitCvarsMsg>();
}

void CGameVM::CGameRocketInit()
{
	this->SendMsg<CGameRocketInitMsg>();
}

void CGameVM::CGameRocketFrame()
{
	this->SendMsg<CGameRocketFrameMsg>();
}

void CGameVM::CGameRocketFormatData(int handle)
{
	this->SendMsg<CGameRocketFormatDataMsg>(handle);
}

void CGameVM::CGameRocketRenderElement()
{
	this->SendMsg<CGameRocketRenderElementMsg>();
}

float CGameVM::CGameRocketProgressbarValue()
{
	float value;
	this->SendMsg<CGameRocketProgressbarValueMsg>(value);
	return value;
}

void CGameVM::Syscall(uint32_t id, IPC::Reader reader, IPC::Channel& channel)
{
	int major = id >> 16;
	int minor = id & 0xffff;
	if (major == VM::QVM) {
		this->QVMSyscall(minor, reader, channel);

	} else if (major < VM::LAST_COMMON_SYSCALL) {
		services->Syscall(major, minor, std::move(reader), channel);

	} else {
		Com_Error(ERR_DROP, "Bad major game syscall number: %d", major);
	}
}

void CGameVM::QVMSyscall(int index, IPC::Reader& reader, IPC::Channel& channel)
{
	switch (index) {
		//TODO please fill me
	default:
		Com_Error(ERR_DROP, "Bad game system trap: %d", index);
	}
}
