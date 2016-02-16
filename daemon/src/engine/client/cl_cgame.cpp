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
#include "cg_msgdef.h"

#include "mumblelink/libmumblelink.h"
#include "qcommon/crypto.h"

#include "framework/CommonVMServices.h"
#include "framework/CommandSystem.h"
#include "framework/CvarSystem.h"

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
CL_GetUserCmd
====================
*/
bool CL_GetUserCmd( int cmdNumber, usercmd_t *ucmd )
{
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if ( cmdNumber > cl.cmdNumber )
	{
		Com_Error( errorParm_t::ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber );
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if ( cmdNumber <= cl.cmdNumber - CMD_BACKUP )
	{
		return false;
	}

	*ucmd = cl.cmds[ cmdNumber & CMD_MASK ];

	return true;
}

int CL_GetCurrentCmdNumber()
{
	return cl.cmdNumber;
}

/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified( Cmd::Args& csCmd )
{
	if (csCmd.Argc() < 3) {
		Com_Error( errorParm_t::ERR_DROP, "CL_ConfigstringModified: wrong command received" );
	}

	int index = atoi( csCmd.Argv(1).c_str() );

	if ( index < 0 || index >= MAX_CONFIGSTRINGS )
	{
		Com_Error( errorParm_t::ERR_DROP, "CL_ConfigstringModified: bad index %i", index );
	}

	if ( cl.gameState[index] == csCmd.Argv(2) )
	{
		return;
	}

	cl.gameState[index] = csCmd.Argv(2);

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
bool CL_HandleServerCommand(Str::StringRef text, std::string& newText) {
	static char bigConfigString[ BIG_INFO_STRING ];
	Cmd::Args args(text);

	if (args.Argc() == 0) {
		return false;
	}

	auto cmd = args.Argv(0);
	int argc = args.Argc();

	if (cmd == "disconnect") {
		// NERVE - SMF - allow server to indicate why they were disconnected
		if (argc >= 2) {
			Com_Error(errorParm_t::ERR_SERVERDISCONNECT, "Server disconnected: %s", args.Argv(1).c_str());
		} else {
			Com_Error(errorParm_t::ERR_SERVERDISCONNECT, "Server disconnected");
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
		return false;
	}

	if (cmd == "bcs1") {
		if (argc >= 3) {
			const char* s = Cmd_QuoteString( args[2].c_str() );

			if (strlen(bigConfigString) + strlen(s) >= BIG_INFO_STRING) {
				Com_Error(errorParm_t::ERR_DROP, "bcs exceeded BIG_INFO_STRING");
			}

			Q_strcat(bigConfigString, sizeof(bigConfigString), s);
		}
		return false;
	}

	if (cmd == "bcs2") {
		if (argc >= 3) {
			const char* s = Cmd_QuoteString( args[2].c_str() );

			if (strlen(bigConfigString) + strlen(s) + 1 >= BIG_INFO_STRING) {
				Com_Error(errorParm_t::ERR_DROP, "bcs exceeded BIG_INFO_STRING");
			}

			Q_strcat(bigConfigString, sizeof(bigConfigString), s);
			Q_strcat(bigConfigString, sizeof(bigConfigString), "\"");
			newText = bigConfigString;
			return CL_HandleServerCommand(bigConfigString, newText);
		}
		return false;
	}

	if (cmd == "cs") {
		CL_ConfigstringModified(args);
		return true;
	}

	if (cmd == "map_restart") {
		// clear outgoing commands before passing
		// the restart to the cgame
		memset(cl.cmds, 0, sizeof(cl.cmds));
		return true;
	}

	if (cmd == "popup") {
		// direct server to client popup request, bypassing cgame
		if (cls.state == connstate_t::CA_ACTIVE && !clc.demoplaying && argc >=1) {
			// TODO: Pass to the cgame
		}
		return false;
	}

	if (cmd == "pubkey_decrypt") {
		char         buffer[ MAX_STRING_CHARS ] = "pubkey_identify ";
		NettleLength msg_len = MAX_STRING_CHARS - 16;
		mpz_t        message;

		if (argc == 1) {
			Log::Notice("%s", "^3Server sent a pubkey_decrypt command, but sent nothing to decrypt!\n");
			return false;
		}

		mpz_init_set_str(message, args.Argv(1).c_str(), 16);

		if (rsa_decrypt(&private_key, &msg_len, (unsigned char *) buffer + 16, message)) {
			nettle_mpz_set_str_256_u(message, msg_len, (unsigned char *) buffer + 16);
			mpz_get_str(buffer + 16, 16, message);
			CL_AddReliableCommand(buffer);
		}

		mpz_clear(message);
		return false;
	}

	return true;
}

// Get the server commands, does client-specific handling
// that may block the propagation of a command to cgame.
// If the propagation is not blocked then it puts the command
// in commands.
void CL_FillServerCommands(std::vector<std::string>& commands, int start, int end)
{
	// if we have irretrievably lost a reliable command, drop the connection
	if ( start <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS )
	{
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if ( clc.demoplaying )
		{
			return;
		}

		Com_Error( errorParm_t::ERR_DROP, "CL_FillServerCommand: a reliable command was cycled out" );
	}

	if ( end > clc.serverCommandSequence )
	{
		Com_Error( errorParm_t::ERR_DROP, "CL_FillServerCommand: requested a command not received" );
	}

	for (int i = start; i <= end; i++) {
		const char* s = clc.serverCommands[ i & ( MAX_RELIABLE_COMMANDS - 1 ) ];

		std::string cmdText = s;
		if (CL_HandleServerCommand(s, cmdText)) {
			commands.push_back(std::move(cmdText));
		}
	}
}

/*
====================
CL_GetSnapshot
====================
*/
bool CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot )
{
	clSnapshot_t *clSnap;

	if ( snapshotNumber > cl.snap.messageNum )
	{
		Com_Error( errorParm_t::ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum" );
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if ( cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP )
	{
		return false;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.snapshots[ snapshotNumber & PACKET_MASK ];

	if ( !clSnap->valid )
	{
		return false;
	}

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	memcpy( snapshot->areamask, clSnap->areamask, sizeof( snapshot->areamask ) );
	snapshot->ps = clSnap->ps;
	snapshot->entities = clSnap->entities;

	CL_FillServerCommands(snapshot->serverCommands, clc.lastExecutedServerCommand + 1, clSnap->serverCommandNum);
	clc.lastExecutedServerCommand = clSnap->serverCommandNum;

	return true;
}

/*
====================
CL_ShutdownCGame
====================
*/
void CL_ShutdownCGame()
{
	cls.keyCatchers &= ~KEYCATCH_CGAME;
	cls.cgameStarted = false;

	if ( !cgvm.IsActive() )
	{
		return;
	}

	cgvm.CGameShutdown();
	cgvm.Free();
}

//
// libRocket UI stuff
//

/*
 * ====================
 * GetNews
 * ====================
 */
bool GetNews( bool begin )
{
	if ( begin ) // if not already using curl, start the download
	{
		CL_RequestMotd();
		Cvar_Set( "cl_newsString", "Retrieving…" );
	}

	return Cvar_VariableString( "cl_newsString" ) [ 0 ] == 'R';
}

/*
 * ====================
 * LAN_ResetPings
 * ====================
 */
static void LAN_ResetPings( int source )
{
	int          count, i;
	serverInfo_t *servers = nullptr;

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
 * LAN_GetServerInfo
 * ====================
 */
static void LAN_GetServerInfo( int source, int n, char *buf, int buflen )
{
	char         info[ MAX_STRING_CHARS ];
	serverInfo_t *server = nullptr;

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
		Info_SetValueForKey( info, "hostname", server->hostName, false );
		Info_SetValueForKey( info, "serverload", va( "%i", server->load ), false );
		Info_SetValueForKey( info, "mapname", server->mapName, false );
		Info_SetValueForKey( info, "label", server->label, false );
		Info_SetValueForKey( info, "clients", va( "%i", server->clients ), false );
		Info_SetValueForKey( info, "bots", va( "%i", server->bots ), false );
		Info_SetValueForKey( info, "sv_maxclients", va( "%i", server->maxClients ), false );
		Info_SetValueForKey( info, "ping", va( "%i", server->ping ), false );
		Info_SetValueForKey( info, "minping", va( "%i", server->minPing ), false );
		Info_SetValueForKey( info, "maxping", va( "%i", server->maxPing ), false );
		Info_SetValueForKey( info, "game", server->game, false );
		Info_SetValueForKey( info, "nettype", Util::enum_str(server->netType), false );
		Info_SetValueForKey( info, "addr", NET_AdrToStringwPort( server->adr ), false );
		Info_SetValueForKey( info, "friendlyFire", va( "%i", server->friendlyFire ), false );   // NERVE - SMF
		Info_SetValueForKey( info, "needpass", va( "%i", server->needpass ), false );   // NERVE - SMF
		Info_SetValueForKey( info, "gamename", server->gameName, false );  // Arnout
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
	serverInfo_t *server = nullptr;

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
 * LAN_MarkServerVisible
 * ====================
 */
static void LAN_MarkServerVisible( int source, int n, bool visible )
{
	if ( n == -1 )
	{
		int          count = MAX_OTHER_SERVERS;
		serverInfo_t *server = nullptr;

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
			Log::Warn("Server sent a pubkey_decrypt command, but sent nothing to decrypt!" );
			return false;
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

	return false;
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
int Key_GetCatcher()
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

/*
====================
CL_InitCGame

Should only by called by CL_StartHunkUsers
====================
*/
void CL_InitCGame()
{
	const char *info;
	const char *mapname;
	int        t1, t2;

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.gameState[ CS_SERVERINFO ].c_str();
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cl.mapname, sizeof( cl.mapname ), "maps/%s.bsp", mapname );


	cls.state = connstate_t::CA_LOADING;

	// init for this gamestate
	cgvm.CGameInit(clc.serverMessageSequence, clc.clientNum);

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = connstate_t::CA_PRIMED;

	t2 = Sys_Milliseconds();

	Log::Debug( "CL_InitCGame: %5.2fs", ( t2 - t1 ) / 1000.0 );

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re.EndRegistration();

	// Cause any input while loading to be dropped and forget what's pressed
	IN_DropInputsForFrame();
	CL_ClearKeys();
	Key_ClearStates();
}

/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering()
{
	cgvm.CGameDrawActiveFrame(cl.serverTime, clc.demoplaying);
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

static const int RESET_TIME = 500;

void CL_AdjustTimeDelta()
{
//	int             resetTime;
	int newDelta;
	int deltaDelta;

	cl.newSnapshots = false;

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
			Log::Notice( "<RESET> " );
		}
	}
	else if ( deltaDelta > 100 )
	{
		// fast adjust, cut the difference in half
		if ( cl_showTimeDelta->integer )
		{
			Log::Notice( "<FAST> " );
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
				cl.extrapolatedSnapshot = false;
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
		Log::Notice("%i ", cl.serverTimeDelta );
	}
}

/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot()
{
	// ignore snapshots that don't have entities
	if ( cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE )
	{
		return;
	}

	cls.state = connstate_t::CA_ACTIVE;

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
		Log::Notice("%s", ret == 0 ? "Mumble: Linking to Mumble application okay\n" : "Mumble: Linking to Mumble application failed\n" );
	}

	// resend userinfo upon entering the game, as some cvars may
    // not have had the CVAR_USERINFO flag set until loading cgame
	cvar_modifiedFlags |= CVAR_USERINFO;
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime()
{
	// getting a valid frame message ends the connection process
	if ( cls.state != connstate_t::CA_ACTIVE )
	{
		if ( cls.state != connstate_t::CA_PRIMED )
		{
			return;
		}

		if ( clc.demoplaying )
		{
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if ( !clc.firstDemoFrameSkipped )
			{
				clc.firstDemoFrameSkipped = true;
				return;
			}

			CL_ReadDemoMessage();
		}

		if ( cl.newSnapshots )
		{
			cl.newSnapshots = false;
			CL_FirstSnapshot();
		}

		if ( cls.state != connstate_t::CA_ACTIVE )
		{
			return;
		}
	}

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if ( !cl.snap.valid )
	{
		Com_Error( errorParm_t::ERR_DROP, "CL_SetCGameTime: !cl.snap.valid" );
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
			Com_Error( errorParm_t::ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime" );
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
			cl.extrapolatedSnapshot = true;
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

		if ( cls.state != connstate_t::CA_ACTIVE )
		{
			Cvar_Set( "timescale", "1" );
			return; // end of demo
		}
	}
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

CGameVM::CGameVM(): VM::VMBase("cgame"), services(nullptr), cmdBuffer("client")
{
}

void CGameVM::Start()
{
	services = std::unique_ptr<VM::CommonVMServices>(new VM::CommonVMServices(*this, "CGame", Cmd::CGAME_VM));
	uint32_t version = this->Create();
	if ( version != CGAME_API_VERSION ) {
		Com_Error( errorParm_t::ERR_DROP, "CGame ABI mismatch, expected %d, got %d", CGAME_API_VERSION, version );
	}
	this->CGameStaticInit();
}

void CGameVM::CGameStaticInit()
{
	this->SendMsg<CGameStaticInitMsg>(Sys_Milliseconds());
}

void CGameVM::CGameInit(int serverMessageNum, int clientNum)
{
	this->SendMsg<CGameInitMsg>(serverMessageNum, clientNum, cls.glconfig, cl.gameState);
}

void CGameVM::CGameShutdown()
{
	// Ignore errors when shutting down
	try {
		this->SendMsg<CGameShutdownMsg>();
		this->Free();
	} catch (Sys::DropErr&) {}
	services = nullptr;
}

void CGameVM::CGameDrawActiveFrame(int serverTime,  bool demoPlayback)
{
	this->SendMsg<CGameDrawActiveFrameMsg>(serverTime, demoPlayback);
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

void CGameVM::CGameMousePosEvent(int x, int y)
{
	this->SendMsg<CGameMousePosEventMsg>(x, y);
}

void CGameVM::CGameFocusEvent(bool focus)
{
	this->SendMsg<CGameFocusEventMsg>(focus);
}


void CGameVM::CGameTextInputEvent(int c)
{
	this->SendMsg<CGameTextInptEvent>(c);
}

void CGameVM::CGameRocketInit()
{
	this->SendMsg<CGameRocketInitMsg>(cls.glconfig);
}

void CGameVM::CGameRocketFrame()
{
	cgClientState_t state;
	state.connectPacketCount = clc.connectPacketCount;
	state.connState = cls.state;
	Q_strncpyz( state.servername, cls.servername, sizeof( state.servername ) );
	Q_strncpyz( state.updateInfoString, cls.updateInfoString, sizeof( state.updateInfoString ) );
	Q_strncpyz( state.messageString, clc.serverMessage, sizeof( state.messageString ) );
	state.clientNum = cl.snap.ps.clientNum;
	this->SendMsg<CGameRocketFrameMsg>(state);
}

void CGameVM::CGameConsoleLine(const std::string& str)
{
	this->SendMsg<CGameConsoleLineMsg>(str);
}

void CGameVM::Syscall(uint32_t id, Util::Reader reader, IPC::Channel& channel)
{
	int major = id >> 16;
	int minor = id & 0xffff;
	if (major == VM::QVM) {
		this->QVMSyscall(minor, reader, channel);

	} else if (major == VM::COMMAND_BUFFER) {
		this->cmdBuffer.Syscall(minor, reader, channel);

	} else if (major < VM::LAST_COMMON_SYSCALL) {
		services->Syscall(major, minor, std::move(reader), channel);

	} else {
		Sys::Drop("Bad major game syscall number: %d", major);
	}
}

void CGameVM::QVMSyscall(int index, Util::Reader& reader, IPC::Channel& channel)
{
	switch (index) {
		case CG_SENDCLIENTCOMMAND:
			IPC::HandleMsg<SendClientCommandMsg>(channel, std::move(reader), [this] (const std::string& command) {
				CL_AddReliableCommand(command.c_str());
			});
			break;

		case CG_UPDATESCREEN:
			IPC::HandleMsg<UpdateScreenMsg>(channel, std::move(reader), [this]  {
				SCR_UpdateScreen();
			});
			break;

		case CG_CM_MARKFRAGMENTS:
			// TODO wow this is very ugly and expensive, find something better?
			// plus we have a lot of const casts for the vector buffers
			IPC::HandleMsg<CMMarkFragmentsMsg>(channel, std::move(reader), [this] (std::vector<std::array<float, 3>> points, std::array<float, 3> projection, int maxPoints, int maxFragments, std::vector<std::array<float, 3>>& pointBuffer, std::vector<markFragment_t>& fragmentBuffer) {
				pointBuffer.resize(maxPoints);
				fragmentBuffer.resize(maxFragments);
				int numFragments = re.MarkFragments(points.size(), (vec3_t*)points.data(), projection.data(), maxPoints, (float*) pointBuffer.data(), maxFragments, fragmentBuffer.data());
				fragmentBuffer.resize(numFragments);
			});
			break;

		case CG_GETCURRENTSNAPSHOTNUMBER:
			IPC::HandleMsg<GetCurrentSnapshotNumberMsg>(channel, std::move(reader), [this] (int& number, int& serverTime) {
				number = cl.snap.messageNum;
				serverTime = cl.snap.serverTime;
			});
			break;

		case CG_GETSNAPSHOT:
			IPC::HandleMsg<GetSnapshotMsg>(channel, std::move(reader), [this] (int number, bool& res, snapshot_t& snapshot) {
				res = CL_GetSnapshot(number, &snapshot);
			});
			break;

		case CG_GETCURRENTCMDNUMBER:
			IPC::HandleMsg<GetCurrentCmdNumberMsg>(channel, std::move(reader), [this] (int& number) {
				number = CL_GetCurrentCmdNumber();
			});
			break;

		case CG_GETUSERCMD:
			IPC::HandleMsg<GetUserCmdMsg>(channel, std::move(reader), [this] (int number, bool& res, usercmd_t& cmd) {
				res = CL_GetUserCmd(number, &cmd);
			});
			break;

		case CG_SETUSERCMDVALUE:
			IPC::HandleMsg<SetUserCmdValueMsg>(channel, std::move(reader), [this] (int stateValue, int flags, float scale) {
				cl.cgameUserCmdValue = stateValue;
				cl.cgameFlags = flags;
				cl.cgameSensitivity = scale;
			});
			break;

		case CG_GET_ENTITY_TOKEN:
			IPC::HandleMsg<GetEntityTokenMsg>(channel, std::move(reader), [this] (int len, bool& res, std::string& token) {
				std::unique_ptr<char[]> buffer(new char[len]);
				res = re.GetEntityToken(buffer.get(), len);
				token.assign(buffer.get(), len);
			});
			break;

		case CG_REGISTER_BUTTON_COMMANDS:
			IPC::HandleMsg<RegisterButtonCommandsMsg>(channel, std::move(reader), [this] (const std::string& commands) {
				CL_RegisterButtonCommands(commands.c_str());
			});
			break;

		case CG_GETCLIPBOARDDATA:
			IPC::HandleMsg<GetClipboardDataMsg>(channel, std::move(reader), [this] (int len, std::string& data) {
				if (cl_allowPaste->integer) {
					std::unique_ptr<char[]> buffer(new char[len]);
					CL_GetClipboardData(buffer.get(), len);
					data.assign(buffer.get(), len);
				}
			});
			break;

		case CG_QUOTESTRING:
			IPC::HandleMsg<QuoteStringMsg>(channel, std::move(reader), [this] (int len, const std::string& input, std::string& output) {
				std::unique_ptr<char[]> buffer(new char[len]);
				Cmd_QuoteStringBuffer(input.c_str(), buffer.get(), len);
				output.assign(buffer.get(), len);
			});
			break;

		case CG_GETTEXT:
			IPC::HandleMsg<GettextMsg>(channel, std::move(reader), [this] (int len, const std::string& input, std::string& output) {
				std::unique_ptr<char[]> buffer(new char[len]);
				Q_strncpyz(buffer.get(), __(input.c_str()), len);
				output.assign(buffer.get(), len);
			});
			break;

		case CG_PGETTEXT:
			IPC::HandleMsg<PGettextMsg>(channel, std::move(reader), [this] (int len, const std::string& context, const std::string& input, std::string& output) {
				std::unique_ptr<char[]> buffer(new char[len]);
				Q_strncpyz(buffer.get(), C__(context.c_str(), input.c_str()), len);
				output.assign(buffer.get(), len);
			});
			break;

		case CG_GETTEXT_PLURAL:
			IPC::HandleMsg<GettextPluralMsg>(channel, std::move(reader), [this] (int len, const std::string& input1, const std::string& input2, int number, std::string& output) {
				std::unique_ptr<char[]> buffer(new char[len]);
				Q_strncpyz(buffer.get(), P__(input1.c_str(), input2.c_str(), number), len);
				output.assign(buffer.get(), len);
			});
			break;

		case CG_NOTIFY_TEAMCHANGE:
			IPC::HandleMsg<NotifyTeamChangeMsg>(channel, std::move(reader), [this] (int team) {
				CL_OnTeamChanged(team);
			});
			break;

		case CG_PREPAREKEYUP:
			IPC::HandleMsg<PrepareKeyUpMsg>(channel, std::move(reader), [this] {
				IN_PrepareKeyUp();
			});
			break;

		case CG_GETNEWS:
			IPC::HandleMsg<GetNewsMsg>(channel, std::move(reader), [this] (bool force, bool& res) {
				res = GetNews(force);
			});
			break;

		// All sounds

        case CG_S_REGISTERSOUND:
            IPC::HandleMsg<Audio::RegisterSoundMsg>(channel, std::move(reader), [this] (const std::string& sample, int& handle) {
                handle = Audio::RegisterSFX(sample.c_str());
            });
            break;

		// All renderer

		case CG_R_SETALTSHADERTOKENS:
			IPC::HandleMsg<Render::SetAltShaderTokenMsg>(channel, std::move(reader), [this] (const std::string& tokens) {
				re.SetAltShaderTokens(tokens.c_str());
			});
			break;

		case CG_R_GETSHADERNAMEFROMHANDLE:
			IPC::HandleMsg<Render::GetShaderNameFromHandleMsg>(channel, std::move(reader), [this] (int handle, std::string& name) {
			    name = re.ShaderNameFromHandle(handle);
			});
			break;

		case CG_R_INPVVS:
			IPC::HandleMsg<Render::InPVVSMsg>(channel, std::move(reader), [this] (const std::array<float, 3>& p1, const std::array<float, 3>& p2, bool& res) {
				res = re.inPVVS(p1.data(), p2.data());
			});
			break;

		case CG_R_LOADWORLDMAP:
			IPC::HandleMsg<Render::LoadWorldMapMsg>(channel, std::move(reader), [this] (const std::string& mapName) {
				re.SetWorldVisData(CM_ClusterPVS(-1));
				re.LoadWorld(mapName.c_str());
			});
			break;

		case CG_R_REGISTERMODEL:
			IPC::HandleMsg<Render::RegisterModelMsg>(channel, std::move(reader), [this] (const std::string& name, int& handle) {
				handle = re.RegisterModel(name.c_str());
			});
			break;

		case CG_R_REGISTERSKIN:
			IPC::HandleMsg<Render::RegisterSkinMsg>(channel, std::move(reader), [this] (const std::string& name, int& handle) {
				handle = re.RegisterSkin(name.c_str());
			});
			break;

		case CG_R_REGISTERSHADER:
			IPC::HandleMsg<Render::RegisterShaderMsg>(channel, std::move(reader), [this] (const std::string& name, int flags, int& handle) {
				handle = re.RegisterShader(name.c_str(), (RegisterShaderFlags_t) flags);
			});
			break;

		case CG_R_REGISTERFONT:
			IPC::HandleMsg<Render::RegisterFontMsg>(channel, std::move(reader), [this] (const std::string& name, const std::string& fallbackName, int pointSize, fontMetrics_t& font) {
				re.RegisterFontVM(name.c_str(), fallbackName.c_str(), pointSize, &font);
			});
			break;

		case CG_R_MODELBOUNDS:
			IPC::HandleMsg<Render::ModelBoundsMsg>(channel, std::move(reader), [this] (int handle, std::array<float, 3>& mins, std::array<float, 3>& maxs) {
				re.ModelBounds(handle, mins.data(), maxs.data());
			});
			break;

		case CG_R_LERPTAG:
			IPC::HandleMsg<Render::LerpTagMsg>(channel, std::move(reader), [this] (const refEntity_t& entity, const std::string& tagName, int startIndex, orientation_t& tag, int& res) {
				res = re.LerpTag(&tag, &entity, tagName.c_str(), startIndex);
			});
			break;

		case CG_R_REMAP_SHADER:
			IPC::HandleMsg<Render::RemapShaderMsg>(channel, std::move(reader), [this] (const std::string& oldShader, const std::string& newShader, const std::string& timeOffset) {
				re.RemapShader(oldShader.c_str(), newShader.c_str(), timeOffset.c_str());
			});
			break;

		case CG_R_INPVS:
			IPC::HandleMsg<Render::InPVSMsg>(channel, std::move(reader), [this] (const std::array<float, 3>& p1, const std::array<float, 3>& p2, bool& res) {
				res = re.inPVS(p1.data(), p2.data());
			});
			break;

		case CG_R_LIGHTFORPOINT:
			IPC::HandleMsg<Render::LightForPointMsg>(channel, std::move(reader), [this] (std::array<float, 3> point, std::array<float, 3>& ambient, std::array<float, 3>& directed, std::array<float, 3>& dir, int& res) {
				res = re.LightForPoint(point.data(), ambient.data(), directed.data(), dir.data());
			});
			break;

		case CG_R_REGISTERANIMATION:
			IPC::HandleMsg<Render::RegisterAnimationMsg>(channel, std::move(reader), [this] (const std::string& name, int& handle) {
				handle = re.RegisterAnimation(name.c_str());
			});
			break;

		case CG_R_BUILDSKELETON:
			IPC::HandleMsg<Render::BuildSkeletonMsg>(channel, std::move(reader), [this] (int anim, int startFrame, int endFrame, float frac, bool clearOrigin, refSkeleton_t& skel, int& res) {
				res = re.BuildSkeleton(&skel, anim, startFrame, endFrame, frac, clearOrigin);
			});
			break;

		case CG_R_BONEINDEX:
			IPC::HandleMsg<Render::BoneIndexMsg>(channel, std::move(reader), [this] (int model, const std::string& boneName, int& index) {
				index = re.BoneIndex(model, boneName.c_str());
			});
			break;

		case CG_R_ANIMNUMFRAMES:
			IPC::HandleMsg<Render::AnimNumFramesMsg>(channel, std::move(reader), [this] (int anim, int& res) {
				res = re.AnimNumFrames(anim);
			});
			break;

		case CG_R_ANIMFRAMERATE:
			IPC::HandleMsg<Render::AnimFrameRateMsg>(channel, std::move(reader), [this] (int anim, int& res) {
				res = re.AnimFrameRate(anim);
			});
			break;

		case CG_REGISTERVISTEST:
			IPC::HandleMsg<Render::RegisterVisTestMsg>(channel, std::move(reader), [this] (int& handle) {
				handle = re.RegisterVisTest();
			});
			break;

		case CG_CHECKVISIBILITY:
			IPC::HandleMsg<Render::CheckVisibilityMsg>(channel, std::move(reader), [this] (int handle, float& res) {
				res = re.CheckVisibility(handle);
			});
			break;

		case CG_R_GETTEXTURESIZE:
			IPC::HandleMsg<Render::GetTextureSizeMsg>(channel, std::move(reader), [this] (qhandle_t handle, int& x, int& y) {
				re.GetTextureSize(handle, &x, &y);
			});
			break;

		case CG_R_GENERATETEXTURE:
			IPC::HandleMsg<Render::GenerateTextureMsg>(channel, std::move(reader), [this] (std::vector<byte> data, int x, int y, qhandle_t& handle) {
				handle = re.GenerateTexture(data.data(), x, y);
			});
			break;

		// All keys

		case CG_KEY_GETCATCHER:
			IPC::HandleMsg<Key::GetCatcherMsg>(channel, std::move(reader), [this] (int& catcher) {
				catcher = Key_GetCatcher();
			});
			break;

		case CG_KEY_SETCATCHER:
			IPC::HandleMsg<Key::SetCatcherMsg>(channel, std::move(reader), [this] (int catcher) {
				Key_SetCatcher(catcher);
			});
			break;

		case CG_KEY_GETKEYNUMFORBINDS:
			IPC::HandleMsg<Key::GetKeynumForBindsMsg>(channel, std::move(reader), [this] (int team, const std::vector<std::string>& binds, std::vector<std::vector<int>>& result) {
                for (const auto& bind : binds) {
                    result.push_back({});
                    for (int i = 0; i < Util::ordinal(keyNum_t::MAX_KEYS); i++) {
                        char buffer[MAX_STRING_CHARS];

                        Key_GetBindingBuf(i, team, buffer, MAX_STRING_CHARS);
                        if (bind == buffer) {
                            result.back().push_back(i);
                            continue;
                        }
                        Key_GetBindingBuf(0, team, buffer, MAX_STRING_CHARS);
                        if (bind == buffer) {
                            result.back().push_back(i);
                            continue;
                        }
                    }
                }
			});
			break;

		case CG_KEY_KEYNUMTOSTRINGBUF:
			IPC::HandleMsg<Key::KeyNumToStringMsg>(channel, std::move(reader), [this] (int keynum, std::string& result) {
				result = Key_KeynumToString(keynum);
			});
			break;

		case CG_KEY_SETBINDING:
			IPC::HandleMsg<Key::SetBindingMsg>(channel, std::move(reader), [this] (int keyNum, int team, std::string cmd) {
				Key_SetBinding(keyNum, team, cmd.c_str());
			});
			break;

		case CG_KEY_CLEARCMDBUTTONS:
			IPC::HandleMsg<Key::ClearCmdButtonsMsg>(channel, std::move(reader), [this] {
				CL_ClearCmdButtons();
			});
			break;

		case CG_KEY_CLEARSTATES:
			IPC::HandleMsg<Key::ClearStatesMsg>(channel, std::move(reader), [this] {
				Key_ClearStates();
			});
			break;

		case CG_KEY_KEYSDOWN:
			IPC::HandleMsg<Key::KeysDownMsg>(channel, std::move(reader), [this] (std::vector<int> keys, std::vector<int>& list) {
				list.reserve(keys.size());
				for (unsigned i = 0; i < keys.size(); ++i)
				{
					if (keys[i] == Util::ordinal(keyNum_t::K_KP_NUMLOCK))
					{
						list.push_back(IN_IsNumLockDown());
					}
					else
					{
						list.push_back(Key_IsDown( keys[i] ));
					}
				}
			});
			break;

		// Mouse

		case CG_MOUSE_SETMOUSEMODE:
			IPC::HandleMsg<Mouse::SetMouseMode>(channel, std::move(reader), &IN_SetMouseMode);
			break;

		// All LAN

		case CG_LAN_GETSERVERCOUNT:
			IPC::HandleMsg<LAN::GetServerCountMsg>(channel, std::move(reader), [this] (int source, int& count) {
				count = LAN_GetServerCount(source);
			});
			break;

		case CG_LAN_GETSERVERINFO:
			IPC::HandleMsg<LAN::GetServerInfoMsg>(channel, std::move(reader), [this] (int source, int n, int len, std::string& info) {
				std::unique_ptr<char[]> buffer(new char[len]);
				LAN_GetServerInfo(source, n, buffer.get(), len);
				info.assign(buffer.get(), len);
			});
			break;

		case CG_LAN_GETSERVERPING:
			IPC::HandleMsg<LAN::GetServerPingMsg>(channel, std::move(reader), [this] (int source, int n, int& ping) {
				ping = LAN_GetServerPing(source, n);
			});
			break;

		case CG_LAN_MARKSERVERVISIBLE:
			IPC::HandleMsg<LAN::MarkServerVisibleMsg>(channel, std::move(reader), [this] (int source, int n, bool visible) {
				LAN_MarkServerVisible(source, n, visible);
			});
			break;

		case CG_LAN_SERVERISVISIBLE:
			IPC::HandleMsg<LAN::ServerIsVisibleMsg>(channel, std::move(reader), [this] (int source, int n, bool& visible) {
				visible = LAN_ServerIsVisible(source, n);
			});
			break;

		case CG_LAN_UPDATEVISIBLEPINGS:
			IPC::HandleMsg<LAN::UpdateVisiblePingsMsg>(channel, std::move(reader), [this] (int source, bool& res) {
				res = CL_UpdateVisiblePings_f(source);
			});
			break;

		case CG_LAN_RESETPINGS:
			IPC::HandleMsg<LAN::ResetPingsMsg>(channel, std::move(reader), [this] (int n) {
				LAN_ResetPings(n);
			});
			break;

		case CG_LAN_SERVERSTATUS:
			IPC::HandleMsg<LAN::ServerStatusMsg>(channel, std::move(reader), [this] (const std::string& serverAddress, int len, std::string& status, int& res) {
				std::unique_ptr<char[]> buffer(new char[len]);
				res = CL_ServerStatus(serverAddress.c_str(), buffer.get(), len);
				status.assign(buffer.get(), len);
			});
			break;

		case CG_LAN_RESETSERVERSTATUS:
			IPC::HandleMsg<LAN::ResetServerStatusMsg>(channel, std::move(reader), [this] {
				CL_ServerStatus(nullptr, nullptr, 0);
			});
			break;

	default:
		Sys::Drop("Bad CGame QVM syscall minor number: %d", index);
	}
}

//TODO move somewhere else
template<typename Func, typename Id, typename... MsgArgs> void HandleMsg(IPC::Message<Id, MsgArgs...>, Util::Reader reader, Func&& func)
{
    using Message = IPC::Message<Id, MsgArgs...>;

    typename IPC::detail::MapTuple<typename Message::Inputs>::type inputs;
    reader.FillTuple<0>(Util::TypeListFromTuple<typename Message::Inputs>(), inputs);

    Util::apply(std::forward<Func>(func), std::move(inputs));
}

template<typename Msg, typename Func> void HandleMsg(Util::Reader reader, Func&& func)
{
    HandleMsg(Msg(), std::move(reader), std::forward<Func>(func));
}

CGameVM::CmdBuffer::CmdBuffer(std::string name): IPC::CommandBufferHost(name) {
}

void CGameVM::CmdBuffer::HandleCommandBufferSyscall(int major, int minor, Util::Reader& reader) {
	if (major == VM::QVM) {
		switch (minor) {

			// All sounds

			case CG_S_STARTSOUND:
				HandleMsg<Audio::StartSoundMsg>(std::move(reader), [this] (bool isPositional, Vec3 origin, int entityNum, int sfx) {
					Audio::StartSound((isPositional ? -1 : entityNum), origin, sfx);
				});
				break;

			case CG_S_STARTLOCALSOUND:
				HandleMsg<Audio::StartLocalSoundMsg>(std::move(reader), [this] (int sfx) {
					Audio::StartLocalSound(sfx);
				});
				break;

			case CG_S_CLEARLOOPINGSOUNDS:
				HandleMsg<Audio::ClearLoopingSoundsMsg>(std::move(reader), [this] {
					Audio::ClearAllLoopingSounds();
				});
				break;

			case CG_S_ADDLOOPINGSOUND:
				HandleMsg<Audio::AddLoopingSoundMsg>(std::move(reader), [this] (int entityNum, int sfx) {
					Audio::AddEntityLoopingSound(entityNum, sfx);
				});
				break;

			case CG_S_STOPLOOPINGSOUND:
				HandleMsg<Audio::StopLoopingSoundMsg>(std::move(reader), [this] (int entityNum) {
					Audio::ClearLoopingSoundsForEntity(entityNum);
				});
				break;

			case CG_S_UPDATEENTITYPOSITION:
				HandleMsg<Audio::UpdateEntityPositionMsg>(std::move(reader), [this] (int entityNum, Vec3 position) {
					Audio::UpdateEntityPosition(entityNum, position);
				});
				break;

			case CG_S_RESPATIALIZE:
				HandleMsg<Audio::RespatializeMsg>(std::move(reader), [this] (int entityNum, const std::array<Vec3, 3>& axis) {
					Audio::UpdateListener(entityNum, axis.data());
				});
				break;

			case CG_S_STARTBACKGROUNDTRACK:
				HandleMsg<Audio::StartBackgroundTrackMsg>(std::move(reader), [this] (const std::string& intro, const std::string& loop) {
					Audio::StartMusic(intro.c_str(), loop.c_str());
				});
				break;

			case CG_S_STOPBACKGROUNDTRACK:
				HandleMsg<Audio::StopBackgroundTrackMsg>(std::move(reader), [this] {
					Audio::StopMusic();
				});
				break;

			case CG_S_UPDATEENTITYVELOCITY:
				HandleMsg<Audio::UpdateEntityVelocityMsg>(std::move(reader), [this] (int entityNum, Vec3 velocity) {
					Audio::UpdateEntityVelocity(entityNum, velocity);
				});
				break;

			case CG_S_SETREVERB:
				HandleMsg<Audio::SetReverbMsg>(std::move(reader), [this] (int slotNum, const std::string& name, float ratio) {
					Audio::SetReverb(slotNum, name.c_str(), ratio);
				});
				break;

			case CG_S_BEGINREGISTRATION:
				HandleMsg<Audio::BeginRegistrationMsg>(std::move(reader), [this] {
					Audio::BeginRegistration();
				});
				break;

			case CG_S_ENDREGISTRATION:
				HandleMsg<Audio::EndRegistrationMsg>(std::move(reader), [this] {
					Audio::EndRegistration();
				});
				break;

            // All renderer

            case CG_R_SCISSOR_ENABLE:
                HandleMsg<Render::ScissorEnableMsg>(std::move(reader), [this] (bool enable) {
                    re.ScissorEnable(enable);
                });
                break;

            case CG_R_SCISSOR_SET:
                HandleMsg<Render::ScissorSetMsg>(std::move(reader), [this] (int x, int y, int w, int h) {
                    re.ScissorSet(x, y, w, h);
                });
                break;

            case CG_R_CLEARSCENE:
                HandleMsg<Render::ClearSceneMsg>(std::move(reader), [this] {
                    re.ClearScene();
                });
                break;

            case CG_R_ADDREFENTITYTOSCENE:
                HandleMsg<Render::AddRefEntityToSceneMsg>(std::move(reader), [this] (refEntity_t&& entity) {
                    re.AddRefEntityToScene(&entity);
                });
                break;

            case CG_R_ADDPOLYTOSCENE:
                HandleMsg<Render::AddPolyToSceneMsg>(std::move(reader), [this] (int shader, const std::vector<polyVert_t>& verts) {
                    re.AddPolyToScene(shader, verts.size(), verts.data());
                });
                break;

            case CG_R_ADDPOLYSTOSCENE:
                HandleMsg<Render::AddPolysToSceneMsg>(std::move(reader), [this] (int shader, const std::vector<polyVert_t>& verts, int numVerts, int numPolys) {
                    re.AddPolysToScene(shader, numVerts, verts.data(), numPolys);
                });
                break;

            case CG_R_ADDLIGHTTOSCENE:
                HandleMsg<Render::AddLightToSceneMsg>(std::move(reader), [this] (const std::array<float, 3>& point, float radius, float intensity, float r, float g, float b, int shader, int flags) {
                    re.AddLightToScene(point.data(), radius, intensity, r, g, b, shader, flags);
                });
                break;

            case CG_R_ADDADDITIVELIGHTTOSCENE:
                HandleMsg<Render::AddAdditiveLightToSceneMsg>(std::move(reader), [this] (const std::array<float, 3>& point, float intensity, float r, float g, float b) {
                    re.AddAdditiveLightToScene(point.data(), intensity, r, g, b);
                });
                break;

            case CG_R_SETCOLOR:
                HandleMsg<Render::SetColorMsg>(std::move(reader), [this] (const Color::Color& color) {
                    re.SetColor(color);
                });
                break;

            case CG_R_SETCLIPREGION:
                HandleMsg<Render::SetClipRegionMsg>(std::move(reader), [this] (const std::array<float, 4>& region) {
                    re.SetClipRegion(region.data());
                });
                break;

            case CG_R_RESETCLIPREGION:
                HandleMsg<Render::ResetClipRegionMsg>(std::move(reader), [this] {
                    re.SetClipRegion(nullptr);
                });
                break;

            case CG_R_DRAWSTRETCHPIC:
                HandleMsg<Render::DrawStretchPicMsg>(std::move(reader), [this] (float x, float y, float w, float h, float s1, float t1, float s2, float t2, int shader) {
                    re.DrawStretchPic(x, y, w, h, s1, t1, s2, t2, shader);
                });
                break;

            case CG_R_DRAWROTATEDPIC:
                HandleMsg<Render::DrawRotatedPicMsg>(std::move(reader), [this] (float x, float y, float w, float h, float s1, float t1, float s2, float t2, int shader, float angle) {
                    re.DrawRotatedPic(x, y, w, h, s1, t1, s2, t2, shader, angle);
                });
                break;

            case CG_ADDVISTESTTOSCENE:
                HandleMsg<Render::AddVisTestToSceneMsg>(std::move(reader), [this] (int handle, const std::array<float, 3>& pos, float depthAdjust, float area) {
                    re.AddVisTestToScene(handle, pos.data(), depthAdjust, area);
                });
                break;

            case CG_UNREGISTERVISTEST:
                HandleMsg<Render::UnregisterVisTestMsg>(std::move(reader), [this] (int handle) {
                    re.UnregisterVisTest(handle);
                });
                break;

            case CG_SETCOLORGRADING:
                HandleMsg<Render::SetColorGradingMsg>(std::move(reader), [this] (int slot, int shader) {
                    re.SetColorGrading(slot, shader);
                });
                break;

            case CG_R_RENDERSCENE:
                HandleMsg<Render::RenderSceneMsg>(std::move(reader), [this] (refdef_t rd) {
                    re.RenderScene(&rd);
                });
				break;

			case CG_R_ADD2DPOLYSINDEXED:
				HandleMsg<Render::Add2dPolysIndexedMsg>(std::move(reader), [this] (std::vector<polyVert_t> polys, int numPolys, std::vector<int> indicies, int numIndicies, int trans_x, int trans_y, qhandle_t shader) {
					re.Add2dPolysIndexed(polys.data(), numPolys, indicies.data(), numIndicies, trans_x, trans_y, shader);
				});
                break;

		default:
			Sys::Drop("Bad minor CGame QVM Command Buffer number: %d", minor);
		}

	} else {
		Sys::Drop("Bad major CGame Command Buffer number: %d", major);
	}
}

