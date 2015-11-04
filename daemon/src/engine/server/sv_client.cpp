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

// sv_client.c -- server code for dealing with clients

#include "server.h"
#include "CryptoChallege.h"

static void SV_CloseDownload( client_t *cl );

/*
=================
SV_GetChallenge

A "getchallenge" OOB command has been received
Returns a challenge number that can be used
in a subsequent connectResponse command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.

If we are authorizing, a challenge request will cause a packet
to be sent to the authorize server.

When an authorizeip is returned, a challenge response will be
sent to that IP address.
=================
*/
void SV_GetChallenge( netadr_t from )
{
	int         i;
	int         oldest;
	int         oldestTime;
	challenge_t *challenge;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this IP address
	challenge = &svs.challenges[ 0 ];

	for ( i = 0; i < MAX_CHALLENGES; i++, challenge++ )
	{
		if ( !challenge->connected && NET_CompareAdr( from, challenge->adr ) )
		{
			break;
		}

		if ( challenge->time < oldestTime )
		{
			oldestTime = challenge->time;
			oldest = i;
		}
	}

	if ( i == MAX_CHALLENGES )
	{
		// this is the first time this client has asked for a challenge
		challenge = &svs.challenges[ oldest ];

		challenge->challenge = ( ( rand() << 16 ) ^ rand() ) ^ svs.time;
		challenge->adr = from;
		challenge->firstTime = svs.time;
		challenge->firstPing = 0;
		challenge->time = svs.time;
		challenge->connected = false;
		i = oldest;
	}

	challenge->pingTime = svs.time;

	NET_OutOfBandPrint( NS_SERVER, from, "challengeResponse %i", challenge->challenge );

	return;
}

void SV_GetChallengeNew( netadr_t from )
{
	auto challenge = ChallengeManager::Get().GenerateChallenge( from );
	NET_OutOfBandPrint( NS_SERVER, from, "challengeResponseNew %s\n", challenge.c_str() );
}

/*
==================
SV_DirectConnect

A "connect" OOB command has been received
==================
*/
void SV_DirectConnect( netadr_t from, const Cmd::Args& args )
{
	char                userinfo[ MAX_INFO_STRING ];
	int                 i;
	client_t            *cl, *newcl;
	client_t            temp;
	sharedEntity_t      *ent;
	int                 clientNum;
	int                 version;
	int                 qport;
	int                 challenge;
	const char                *password;
	int                 startIndex;
	bool            denied;
	char                reason[ MAX_STRING_CHARS ];
	int                 count;
	const char          *ip;
#ifdef HAVE_GEOIP
	const char          *country = nullptr;
#endif

	if ( args.Argc() < 2 )
	{
		return;
	}

	Com_DPrintf( "SVC_DirectConnect ()\n" );

	Q_strncpyz( userinfo, args.Argv(1).c_str(), sizeof( userinfo ) );

	// DHM - Nerve :: Update Server allows any protocol to connect
	// NOTE TTimo: but we might need to store the protocol around for potential non http/ftp clients
	version = atoi( Info_ValueForKey( userinfo, "protocol" ) );

	if ( version != PROTOCOL_VERSION )
	{
		NET_OutOfBandPrint( NS_SERVER, from, "print\nServer uses protocol version %i (yours is %i).", PROTOCOL_VERSION, version );
		Com_DPrintf( "    rejected connect from version %i\n", version );
		return;
	}

	challenge = atoi( Info_ValueForKey( userinfo, "challenge" ) );
	qport = atoi( Info_ValueForKey( userinfo, "qport" ) );

	// quick reject
	for ( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		// DHM - Nerve :: This check was allowing clients to reconnect after zombietime(2 secs)
		//if ( cl->state == CS_FREE ) {
		//continue;
		//}
		if ( NET_CompareBaseAdr( from, cl->netchan.remoteAddress )
		     && ( cl->netchan.qport == qport
		          || from.port == cl->netchan.remoteAddress.port ) )
		{
			if ( ( svs.time - cl->lastConnectTime )
			     < ( sv_reconnectlimit->integer * 1000 ) )
			{
				Com_DPrintf( "%s: reconnect rejected: too soon\n", NET_AdrToString( from ) );
				return;
			}

			break;
		}
	}

	if ( NET_IsLocalAddress( from ) )
	{
		ip = "localhost";
	}
	else
	{
		ip = NET_AdrToString( from );
	}

	if ( ( strlen( ip ) + strlen( userinfo ) + 4 ) >= MAX_INFO_STRING )
	{
		NET_OutOfBandPrint( NS_SERVER, from,
		                    "print\nUserinfo string length exceeded.  "
		                    "Try removing setu cvars from your config." );
		return;
	}

	Info_SetValueForKey( userinfo, "ip", ip, false );

	// see if the challenge is valid (local clients don't need to challenge)
	if ( !NET_IsLocalAddress( from ) )
	{
		int ping;

		for ( i = 0; i < MAX_CHALLENGES; i++ )
		{
			if ( NET_CompareAdr( from, svs.challenges[ i ].adr ) )
			{
				if ( challenge == svs.challenges[ i ].challenge )
				{
					break; // good
				}
			}
		}

		if ( i == MAX_CHALLENGES )
		{
			NET_OutOfBandPrint( NS_SERVER, from, "print\n[err_dialog]No or bad challenge for address." );
			return;
		}

		// force the IP address key/value pair, so the game can filter based on it
		Info_SetValueForKey( userinfo, "ip", NET_AdrToString( from ), false );

		if ( svs.challenges[ i ].firstPing == 0 )
		{
			ping = svs.time - svs.challenges[ i ].pingTime;
			svs.challenges[ i ].firstPing = ping;
		}
		else
		{
			ping = svs.challenges[ i ].firstPing;
		}

#ifdef HAVE_GEOIP
		country = NET_GeoIP_Country( &from );

		if ( country )
		{
			Com_Printf( "Client %i connecting from %s with %i challenge ping\n", i, country, ping );
		}
		else
		{
			Com_Printf( "Client %i connecting from somewhere unknown with %i challenge ping\n", i, ping );
		}
#else
		Com_Printf( "Client %i connecting with %i challenge ping\n", i, ping );
#endif

		svs.challenges[ i ].connected = true;

		// never reject a LAN client based on ping
		if ( !Sys_IsLANAddress( from ) )
		{
			if ( sv_minPing->value && ping < sv_minPing->value )
			{
				NET_OutOfBandPrint( NS_SERVER, from, "print\n[err_dialog]Server is for high pings only" );
				Com_DPrintf( "Client %i rejected on a too low ping\n", i );
				return;
			}

			if ( sv_maxPing->value && ping > sv_maxPing->value )
			{
				NET_OutOfBandPrint( NS_SERVER, from, "print\n[err_dialog]Server is for low pings only" );
				Com_DPrintf( "Client %i rejected on a too high ping: %i\n", i, ping );
				return;
			}
		}
	}
	else
	{
		// force the "ip" info key to "localhost"
		Info_SetValueForKey( userinfo, "ip", "localhost", false );
	}

	newcl = &temp;
	memset( newcl, 0, sizeof( client_t ) );

	// if there is already a slot for this IP address, reuse it
	for ( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if ( cl->state == CS_FREE )
		{
			continue;
		}

		if ( NET_CompareBaseAdr( from, cl->netchan.remoteAddress )
		     && ( cl->netchan.qport == qport
		          || from.port == cl->netchan.remoteAddress.port ) )
		{
			Com_Printf( "%s:reconnect\n", NET_AdrToString( from ) );
			newcl = cl;

			// this doesn't work because it nukes the players userinfo

//			// disconnect the client from the game first so any flags the
//			// player might have are dropped
//			VM_Call( gvm, GAME_CLIENT_DISCONNECT, newcl - svs.clients );
			//
			goto gotnewcl;
		}
	}

	// find a client slot
	// if "sv_privateClients" is set > 0, then that number
	// of client slots will be reserved for connections that
	// have "password" set to the value of "sv_privatePassword"
	// Info requests will report the maxclients as if the private
	// slots didn't exist, to prevent people from trying to connect
	// to a full server.
	// This is to allow us to reserve a couple slots here on our
	// servers so we can play without having to kick people.

	// check for privateClient password
	password = Info_ValueForKey( userinfo, "password" );

	if ( !strcmp( password, sv_privatePassword->string ) )
	{
		startIndex = 0;
	}
	else
	{
		// skip past the reserved slots
		startIndex = sv_privateClients->integer;
	}

	newcl = nullptr;

	for ( i = startIndex; i < sv_maxclients->integer; i++ )
	{
		cl = &svs.clients[ i ];

		if ( cl->state == CS_FREE )
		{
			newcl = cl;
			break;
		}
	}

	if ( !newcl )
	{
		if ( NET_IsLocalAddress( from ) )
		{
			count = 0;

			for ( i = startIndex; i < sv_maxclients->integer; i++ )
			{
				cl = &svs.clients[ i ];

				if ( SV_IsBot(cl) )
				{
					count++;
				}
			}

			// if they're all bots
			if ( count >= sv_maxclients->integer - startIndex )
			{
				SV_DropClient( &svs.clients[ sv_maxclients->integer - 1 ], "only bots on server" );
				newcl = &svs.clients[ sv_maxclients->integer - 1 ];
			}
			else
			{
				Com_Error( ERR_FATAL, "server is full on local connect" );
			}
		}
		else
		{
			NET_OutOfBandPrint( NS_SERVER, from, "print\n%s", sv_fullmsg->string );
			Com_DPrintf( "Rejected a connection.\n" );
			return;
		}
	}

	// we got a newcl, so reset the reliableSequence and reliableAcknowledge
	cl->reliableAcknowledge = 0;
	cl->reliableSequence = 0;

gotnewcl:
	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = std::move(temp);
	clientNum = newcl - svs.clients;
	ent = SV_GentityNum( clientNum );
	newcl->gentity = ent;
	ent->r.svFlags = 0;

#ifdef HAVE_GEOIP

	if ( country )
	{
		Info_SetValueForKey( userinfo, "geoip", country, false );
	}
#endif

	// save the challenge
	newcl->challenge = challenge;

	// save the address
	Netchan_Setup( NS_SERVER, &newcl->netchan, from, qport );
	// init the netchan queue

	// Save the pubkey
	Q_strncpyz( newcl->pubkey, Info_ValueForKey( userinfo, "pubkey" ), sizeof( newcl->pubkey ) );
	Info_RemoveKey( userinfo, "pubkey", false );
	// save the userinfo
	Q_strncpyz( newcl->userinfo, userinfo, sizeof( newcl->userinfo ) );

	// get the game a chance to reject this connection or modify the userinfo
	denied = gvm.GameClientConnect( reason, sizeof( reason ), clientNum, true, false );  // firstTime = true

	if ( denied )
	{
		NET_OutOfBandPrint( NS_SERVER, from, "print\n[err_dialog]%s", reason );
		Com_DPrintf( "Game rejected a connection: %s.\n", reason );
		return;
	}

	SV_UserinfoChanged( newcl );

	// DHM - Nerve :: Clear out firstPing now that client is connected
	svs.challenges[ i ].firstPing = 0;

	// send the connect packet to the client
	NET_OutOfBandPrint( NS_SERVER, from, "connectResponse" );

	Com_DPrintf( "Going from CS_FREE to CS_CONNECTED for %s\n", newcl->name );

	newcl->state = CS_CONNECTED;
	newcl->nextSnapshotTime = svs.time;
	newcl->lastPacketTime = svs.time;
	newcl->lastConnectTime = svs.time;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	newcl->gamestateMessageNum = -1;

	// if this was the first client on the server, or the last client
	// the server can hold, send a heartbeat to the master.
	count = 0;

	for ( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if ( svs.clients[ i ].state >= CS_CONNECTED )
		{
			count++;
		}
	}

	if ( count == 1 || count == sv_maxclients->integer )
	{
		SV_Heartbeat_f();
	}
}

/*
=====================
SV_FreeClient

Destructor for data allocated in a client structure
=====================
*/
void SV_FreeClient( client_t *client )
{
#ifdef USE_VOIP
	int index;

	for ( index = client->queuedVoipIndex; index < client->queuedVoipPackets; index++ )
	{
		index %= ARRAY_LEN( client->voipPacket );

		Z_Free( client->voipPacket[ index ] );
	}

	client->queuedVoipPackets = 0;
#endif

	// NA_BOT happens to be the default value for address types (value 0) and are
	// never for clients that send challenges. For NA_BOT, skip the checks for
	// challenges as it makes NET_CompareAdr yell at us.
	if (client->netchan.remoteAddress.type != NA_BOT) {
		// see if we already have a challenge for this IP address
		challenge_t* challenge = &svs.challenges[ 0 ];
		for (int i = 0; i < MAX_CHALLENGES; i++, challenge++)
		{
			if ( NET_CompareAdr( client->netchan.remoteAddress, challenge->adr ) )
			{
				challenge->connected = false;
				break;
			}
		}
	}

	SV_Netchan_FreeQueue( client );
	SV_CloseDownload( client );
}

/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing -- SV_FinalCommand() will handle that
=====================
*/
void SV_DropClient( client_t *drop, const char *reason )
{
	if ( drop->state == CS_ZOMBIE )
	{
		return; // already dropped
	}
	Com_DPrintf( "Going to CS_ZOMBIE for %s\n", drop->name );
	drop->state = CS_ZOMBIE; // become free in a few seconds

	// call the prog function for removing a client
	// this will remove the body, among other things
	gvm.GameClientDisconnect( drop - svs.clients );

	if ( SV_IsBot(drop) )
	{
		SV_BotFreeClient( drop - svs.clients );
	}
	else
	{
		// tell everyone why they got dropped
		// Gordon: we want this displayed elsewhere now
		SV_SendServerCommand( nullptr, "print %s\"^* \"%s\"\n\"", Cmd_QuoteString( drop->name ), Cmd_QuoteString( reason ) );

		// add the disconnect command
		SV_SendServerCommand( drop, "disconnect %s\n", Cmd_QuoteString( reason ) );
	}

	// nuke user info
	SV_SetUserinfo( drop - svs.clients, "" );

	SV_FreeClient( drop );

	// if this was the last client on the server, send a heartbeat
	// to the master so it is known the server is empty
	// send a heartbeat now so the master will get up to date info
	// if there is already a slot for this IP address, reuse it
	int i;
	for ( i = 0; i < sv_maxclients->integer; i++ )
	{
		if ( svs.clients[ i ].state >= CS_CONNECTED )
		{
			break;
		}
	}

	if ( i == sv_maxclients->integer )
	{
		SV_Heartbeat_f();
	}
}

/*
================
SV_SendClientGameState

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each new map load.

It will be resent if the client acknowledges a later message but has
the wrong gamestate.
================
*/
void SV_SendClientGameState( client_t *client )
{
	int           start;
	entityState_t *base, nullstate;
	msg_t         msg;
	byte          msgBuffer[ MAX_MSGLEN ];

	Com_DPrintf( "SV_SendClientGameState() for %s\n", client->name );
	Com_DPrintf( "Going from CS_CONNECTED to CS_PRIMED for %s\n", client->name );
	client->state = CS_PRIMED;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	client->gamestateMessageNum = client->netchan.outgoingSequence;

	MSG_Init( &msg, msgBuffer, sizeof( msgBuffer ) );

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	MSG_WriteLong( &msg, client->lastClientCommand );

	// send any server commands waiting to be sent first.
	// we have to do this cause we send the client->reliableSequence
	// with a gamestate and it sets the clc.serverCommandSequence at
	// the client side
	SV_UpdateServerCommandsToClient( client, &msg );

	// send the gamestate
	MSG_WriteByte( &msg, svc_gamestate );
	MSG_WriteLong( &msg, client->reliableSequence );

	// write the configstrings
	for ( start = 0; start < MAX_CONFIGSTRINGS; start++ )
	{
		if ( sv.configstrings[ start ][ 0 ] )
		{
			MSG_WriteByte( &msg, svc_configstring );
			MSG_WriteShort( &msg, start );
			MSG_WriteBigString( &msg, sv.configstrings[ start ] );
		}
	}

	// write the baselines
	memset( &nullstate, 0, sizeof( nullstate ) );

	for ( start = 0; start < MAX_GENTITIES; start++ )
	{
		base = &sv.svEntities[ start ].baseline;

		if ( !base->number )
		{
			continue;
		}

		MSG_WriteByte( &msg, svc_baseline );
		MSG_WriteDeltaEntity( &msg, &nullstate, base, true );
	}

	MSG_WriteByte( &msg, svc_EOF );

	MSG_WriteLong( &msg, client - svs.clients );

	// write the checksum feed
	MSG_WriteLong( &msg, sv.checksumFeed );

	// NERVE - SMF - debug info
	Com_DPrintf( "Sending %i bytes in gamestate to client: %li\n", msg.cursize, ( long )( client - svs.clients ) );

	// deliver this to the client
	SV_SendMessageToClient( &msg, client );
}

/*
==================
SV_ClientEnterWorld
==================
*/
void SV_ClientEnterWorld( client_t *client, usercmd_t *cmd )
{
	int            clientNum;
	sharedEntity_t *ent;

	Com_DPrintf( "Going from CS_PRIMED to CS_ACTIVE for %s\n", client->name );
	client->state = CS_ACTIVE;

	// set up the entity for the client
	clientNum = client - svs.clients;
	ent = SV_GentityNum( clientNum );
	ent->s.number = clientNum;
	client->gentity = ent;

	client->deltaMessage = -1;
	client->nextSnapshotTime = svs.time; // generate a snapshot immediately
	client->lastUsercmd = *cmd;

	// call the game begin function
	gvm.GameClientBegin( client - svs.clients );
}

/*
============================================================

CLIENT COMMAND EXECUTION

============================================================
*/

/*
==================
SV_CloseDownload

clear/free any download vars
==================
*/
static void SV_CloseDownload( client_t *cl )
{
	int i;

	// EOF
	if ( cl->download )
	{
		delete cl->download;
		cl->download = nullptr;
	}

	*cl->downloadName = 0;

	// Free the temporary buffer space
	for ( i = 0; i < MAX_DOWNLOAD_WINDOW; i++ )
	{
		if ( cl->downloadBlocks[ i ] )
		{
			Z_Free( cl->downloadBlocks[ i ] );
			cl->downloadBlocks[ i ] = nullptr;
		}
	}
}

/*
==================
SV_StopDownload_f

Abort a download if in progress
==================
*/
void SV_StopDownload_f( client_t *cl, const Cmd::Args& )
{
	if ( *cl->downloadName )
	{
		Com_DPrintf( "clientDownload: %d: file \"%s^7\" aborted\n", ( int )( cl - svs.clients ), cl->downloadName );
	}

	SV_CloseDownload( cl );
}

/*
==================
SV_DoneDownload_f

Downloads are finished
==================
*/
void SV_DoneDownload_f( client_t *cl, const Cmd::Args& )
{
	if ( cl->state == CS_ACTIVE )
	{
		return;
	}

	Com_DPrintf( "clientDownload: %s^7 Done\n", cl->name );
	// resend the game state to update any clients that entered during the download
	SV_SendClientGameState( cl );
}

/*
==================
SV_NextDownload_f

The argument will be the last acknowledged block from the client, it should be
the same as cl->downloadClientBlock
==================
*/
void SV_NextDownload_f( client_t *cl, const Cmd::Args& args )
{
	int block;
	if (args.Argc() < 2 or not Str::ParseInt(block, args.Argv(1))) {
		return;
	}

	if ( block == cl->downloadClientBlock )
	{
		Com_DPrintf( "clientDownload: %d: client acknowledge of block %d\n", ( int )( cl - svs.clients ), block );

		// Find out if we are done.  A zero-length block indicates EOF
		if ( cl->downloadBlockSize[ cl->downloadClientBlock % MAX_DOWNLOAD_WINDOW ] == 0 )
		{
			Com_Printf( "clientDownload: %d : file \"%s\" completed\n", ( int )( cl - svs.clients ), cl->downloadName );
			SV_CloseDownload( cl );
			return;
		}

		cl->downloadSendTime = svs.time;
		cl->downloadClientBlock++;
		return;
	}

	// We aren't getting an acknowledge for the correct block, drop the client
	// FIXME: this is bad... the client will never parse the disconnect message
	//          because the cgame isn't loaded yet
	SV_DropClient( cl, "broken download" );
}

/*
==================
SV_BeginDownload_f
==================
*/
void SV_BeginDownload_f( client_t *cl, const Cmd::Args& args )
{
	// Kill any existing download
	SV_CloseDownload( cl );

	if (args.Argc() < 2) {
		return;
	}

	//bani - stop us from printing dupe messages
	if (args.Argv(1) != cl->downloadName)
	{
		cl->downloadnotify = DLNOTIFY_ALL;
	}

	// cl->downloadName is non-zero now, SV_WriteDownloadToClient will see this and open
	// the file itself
	Q_strncpyz( cl->downloadName, args.Argv(1).c_str(), sizeof( cl->downloadName ) );
}

/*
==================
SV_WWWDownload_f
==================
*/
void SV_WWWDownload_f( client_t *cl, const Cmd::Args& args )
{
	if (args.Argc() < 2) {
		return;
	}

	const char *subcmd = args.Argv(1).c_str();

	// only accept wwwdl commands for clients which we first flagged as wwwdl ourselves
	if ( !cl->bWWWDl )
	{
		Com_Printf( "SV_WWWDownload: unexpected wwwdl '%s' for client '%s'\n", subcmd, cl->name );
		SV_DropClient( cl, va( "SV_WWWDownload: unexpected wwwdl %s", subcmd ) );
		return;
	}

	if ( !Q_stricmp( subcmd, "ack" ) )
	{
		if ( cl->bWWWing )
		{
			Com_Logf(LOG_WARN, "dupe wwwdl ack from client '%s'", cl->name );
		}

		cl->bWWWing = true;
		return;
	}
	else if ( !Q_stricmp( subcmd, "bbl8r" ) )
	{
		SV_DropClient( cl, "acking disconnected download mode" );
		return;
	}

	// below for messages that only happen during/after download
	if ( !cl->bWWWing )
	{
		Com_Printf( "SV_WWWDownload: unexpected wwwdl '%s' for client '%s'\n", subcmd, cl->name );
		SV_DropClient( cl, va( "SV_WWWDownload: unexpected wwwdl %s", subcmd ) );
		return;
	}

	if ( !Q_stricmp( subcmd, "done" ) )
	{
		*cl->downloadName = 0;
		cl->bWWWing = false;
		return;
	}
	else if ( !Q_stricmp( subcmd, "fail" ) )
	{
		*cl->downloadName = 0;
		cl->bWWWing = false;
		cl->bFallback = true;
		// send a reconnect
		SV_SendClientGameState( cl );
		return;
	}
	else if ( !Q_stricmp( subcmd, "chkfail" ) )
	{
		Com_Logf(LOG_WARN, "client '%s' reports that the redirect download for '%s' had wrong checksum.\n\tYou should check your download redirect configuration.",
				cl->name, cl->downloadName );
		*cl->downloadName = 0;
		cl->bWWWing = false;
		cl->bFallback = true;
		// send a reconnect
		SV_SendClientGameState( cl );
		return;
	}

	Com_Printf("SV_WWWDownload: unknown wwwdl subcommand '%s' for client '%s'\n", subcmd, cl->name );
	SV_DropClient( cl, va( "SV_WWWDownload: unknown wwwdl subcommand '%s'", subcmd ) );
}

// abort an attempted download
void SV_BadDownload( client_t *cl, msg_t *msg )
{
	MSG_WriteByte( msg, svc_download );
	MSG_WriteShort( msg, 0 );  // client is expecting block zero
	MSG_WriteLong( msg, -1 );  // illegal file size

	*cl->downloadName = 0;
}

/*
==================
SV_CheckFallbackURL

sv_wwwFallbackURL can be used to redirect clients to a web URL in case direct ftp/http didn't work (or is disabled on client's end)
return true when a redirect URL message was filled up
when the cvar is set to something, the download server will effectively never use a legacy download strategy
==================
*/
static bool SV_CheckFallbackURL( client_t *cl, const char* pakName, msg_t *msg )
{
	if ( !sv_wwwFallbackURL->string || strlen( sv_wwwFallbackURL->string ) == 0 )
	{
		return false;
	}

	Com_Printf( "clientDownload: sending client '%s' to fallback URL '%s'\n", cl->name, sv_wwwFallbackURL->string );

	Q_strncpyz(cl->downloadURL, va("%s/%s", sv_wwwFallbackURL->string, pakName), sizeof(cl->downloadURL));

	MSG_WriteByte( msg, svc_download );
	MSG_WriteShort( msg, -1 );  // block -1 means ftp/http download
	MSG_WriteString( msg, va( "%s/%s", sv_wwwFallbackURL->string, pakName ) );
	MSG_WriteLong( msg, 0 );
	MSG_WriteLong( msg, 0 );

	return true;
}

/*
==================
SV_WriteDownloadToClient

Check to see if the client wants a file, open it if needed and start pumping the client
Fill up msg with data
==================
*/
void SV_WriteDownloadToClient( client_t *cl, msg_t *msg )
{
	int      curindex;
	int      rate;
	int      blockspersnap;
	char     errorMessage[ 1024 ];
	int      download_flag;

	bool bTellRate = false; // verbosity

	if ( !*cl->downloadName )
	{
		return; // Nothing being downloaded
	}

	if ( cl->bWWWing )
	{
		return; // The client acked and is downloading with ftp/http
	}

	if ( !cl->download )
	{
		// We open the file here

		//bani - prevent duplicate download notifications
		if ( cl->downloadnotify & DLNOTIFY_BEGIN )
		{
			cl->downloadnotify &= ~DLNOTIFY_BEGIN;
			Com_Printf( "clientDownload: %d : beginning \"%s\"\n", ( int )( cl - svs.clients ), cl->downloadName );
		}

		if ( !sv_allowDownload->integer )
		{
			Com_Printf( "clientDownload: %d : \"%s\" download disabled\n", ( int )( cl - svs.clients ), cl->downloadName );

			if ( sv_pure->integer )
			{
				Com_sprintf( errorMessage, sizeof( errorMessage ),
							 "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
							 "You will need to get this file elsewhere before you " "can connect to this pure server.\n",
							 cl->downloadName );
			}
			else
			{
				Com_sprintf( errorMessage, sizeof( errorMessage ),
							 "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
							 "Set autodownload to No in your settings and you might be "
							 "able to connect even if you don't have the file.\n", cl->downloadName );
			}

			SV_BadDownload( cl, msg );
			MSG_WriteString( msg, errorMessage );  // (could SV_DropClient instead?)

			return;
		}

		// www download redirect protocol
		// NOTE: this is called repeatedly while a client connects. Maybe we should sort of cache the message or something
		// FIXME: we need to abstract this to an independent module for maximum configuration/usability by server admins
		// FIXME: I could rework that, it's crappy
		if ( sv_wwwDownload->integer )
		{
			std::string name, version;
			Util::optional<uint32_t> checksum;
			int downloadSize = 0;
			bool success = FS::ParsePakName(cl->downloadName, cl->downloadName + strlen(cl->downloadName), name, version, checksum);
			if (success) {
				const FS::PakInfo* pak = checksum ? FS::FindPak(name, version) : FS::FindPak(name, version, *checksum);
				if (pak) {
					try {
						downloadSize = FS::RawPath::OpenRead(pak->path).Length();
					} catch (std::system_error&) {
						success = false;
					}
				} else
					success = false;
			}
			std::string pakName = name + "_" + version + ".pk3";

			if ( !cl->bFallback )
			{
				if ( success )
				{
					Q_strncpyz( cl->downloadURL, va("%s/%s", sv_wwwBaseURL->string, pakName.c_str()),
								sizeof( cl->downloadURL ) );

					//bani - prevent multiple download notifications
					if ( cl->downloadnotify & DLNOTIFY_REDIRECT )
					{
						cl->downloadnotify &= ~DLNOTIFY_REDIRECT;
						Com_Printf( "Redirecting client '%s' to %s\n", cl->name, cl->downloadURL );
					}

					// once cl->downloadName is set (and possibly we have our listening socket), let the client know
					cl->bWWWDl = true;
					MSG_WriteByte( msg, svc_download );
					MSG_WriteShort( msg, -1 );  // block -1 means ftp/http download
					// compatible with legacy svc_download protocol: [size] [size bytes]
					// download URL, size of the download file, download flags
					MSG_WriteString( msg, cl->downloadURL );
					MSG_WriteLong( msg, downloadSize );
					download_flag = 0;

					if ( sv_wwwDlDisconnected->integer )
					{
						download_flag |= ( 1 << DL_FLAG_DISCON );
					}

					MSG_WriteLong( msg, download_flag );  // flags
					return;
				}
				else
				{
					// that should NOT happen - even regular download would fail then anyway
					Com_Logf(LOG_ERROR, "Client '%s': couldn't extract file size for %s", cl->name, cl->downloadName );
				}
			}
			else
			{
				cl->bFallback = false;
				cl->bWWWDl = true;

				if ( SV_CheckFallbackURL( cl, pakName.c_str(), msg ) )
				{
					return;
				}

				Com_Logf(LOG_ERROR, "Client '%s': falling back to regular downloading for failed file %s", cl->name,
							cl->downloadName );
			}
		}

		// find file
		cl->bWWWDl = false;
		std::string name, version;
		Util::optional<uint32_t> checksum;
		bool success = FS::ParsePakName(cl->downloadName, cl->downloadName + strlen(cl->downloadName), name, version, checksum);
		if (success) {
			const FS::PakInfo* pak = checksum ? FS::FindPak(name, version) : FS::FindPak(name, version, *checksum);
			if (pak) {
				try {
					cl->download = new FS::File(FS::RawPath::OpenRead(pak->path));
					cl->downloadSize = cl->download->Length();
				} catch (std::system_error&) {
					success = false;
				}
			} else
				success = false;
		}

		if ( !success )
		{
			Com_Printf( "clientDownload: %d : \"%s\" file not found on server\n", ( int )( cl - svs.clients ), cl->downloadName );
			Com_sprintf( errorMessage, sizeof( errorMessage ), "File \"%s\" not found on server for autodownloading.\n",
			             cl->downloadName );
			SV_BadDownload( cl, msg );
			MSG_WriteString( msg, errorMessage );  // (could SV_DropClient instead?)
			return;
		}

		// is valid source, init
		cl->downloadCurrentBlock = cl->downloadClientBlock = cl->downloadXmitBlock = 0;
		cl->downloadCount = 0;
		cl->downloadEOF = false;

		bTellRate = true;
	}

	// Perform any reads that we need to
	while ( cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW && cl->downloadSize != cl->downloadCount )
	{
		curindex = ( cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW );

		if ( !cl->downloadBlocks[ curindex ] )
		{
			cl->downloadBlocks[ curindex ] = ( byte* ) Z_Malloc( MAX_DOWNLOAD_BLKSIZE );
		}

		try {
			cl->downloadBlockSize[ curindex ] = cl->download->Read(cl->downloadBlocks[ curindex ], MAX_DOWNLOAD_BLKSIZE);
		} catch (std::system_error&) {
			// EOF right now
			cl->downloadCount = cl->downloadSize;
			break;
		}

		cl->downloadCount += cl->downloadBlockSize[ curindex ];

		// Load in next block
		cl->downloadCurrentBlock++;
	}

	// Check to see if we have eof condition and add the EOF block
	if ( cl->downloadCount == cl->downloadSize &&
	     !cl->downloadEOF && cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW )
	{
		cl->downloadBlockSize[ cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW ] = 0;
		cl->downloadCurrentBlock++;

		cl->downloadEOF = true; // We have added the EOF block
	}

	// Loop up to window size times based on how many blocks we can fit in the
	// client snapMsec and rate

	// based on the rate, how many bytes can we fit in the snapMsec time of the client
	rate = cl->rate;

	// show_bug.cgi?id=509
	// for autodownload, we use a separate max rate value
	// we do this everytime because the client might change its rate during the download
	if ( sv_dl_maxRate->integer < rate )
	{
		rate = sv_dl_maxRate->integer;

		if ( bTellRate )
		{
			Com_Printf( "'%s' downloading at sv_dl_maxrate (%d)\n", cl->name, sv_dl_maxRate->integer );
		}
	}
	else if ( bTellRate )
	{
		Com_Printf( "'%s' downloading at rate %d\n", cl->name, rate );
	}

	if ( !rate )
	{
		blockspersnap = 1;
	}
	else
	{
		blockspersnap = ( ( rate * cl->snapshotMsec ) / 1000 + MAX_DOWNLOAD_BLKSIZE ) / MAX_DOWNLOAD_BLKSIZE;
	}

	if ( blockspersnap < 0 )
	{
		blockspersnap = 1;
	}

	while ( blockspersnap-- )
	{
		// Write out the next section of the file, if we have already reached our window,
		// automatically start retransmitting

		if ( cl->downloadClientBlock == cl->downloadCurrentBlock )
		{
			return; // Nothing to transmit
		}

		if ( cl->downloadXmitBlock == cl->downloadCurrentBlock )
		{
			// We have transmitted the complete window, should we start resending?

			//FIXME:  This uses a hardcoded one second timeout for lost blocks
			//the timeout should be based on client rate somehow
			if ( svs.time - cl->downloadSendTime > 1000 )
			{
				cl->downloadXmitBlock = cl->downloadClientBlock;
			}
			else
			{
				return;
			}
		}

		// Send current block
		curindex = ( cl->downloadXmitBlock % MAX_DOWNLOAD_WINDOW );

		MSG_WriteByte( msg, svc_download );
		MSG_WriteShort( msg, cl->downloadXmitBlock );

		// block zero is special, contains file size
		if ( cl->downloadXmitBlock == 0 )
		{
			MSG_WriteLong( msg, cl->downloadSize );
		}

		MSG_WriteShort( msg, cl->downloadBlockSize[ curindex ] );

		// Write the block
		if ( cl->downloadBlockSize[ curindex ] )
		{
			MSG_WriteData( msg, cl->downloadBlocks[ curindex ], cl->downloadBlockSize[ curindex ] );
		}

		Com_DPrintf( "clientDownload: %d: writing block %d\n", ( int )( cl - svs.clients ), cl->downloadXmitBlock );

		// Move on to the next block
		// It will get sent with next snap shot.  The rate will keep us in line.
		cl->downloadXmitBlock++;

		cl->downloadSendTime = svs.time;
	}
}

/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately  FIXME: move to game?
=================
*/
static void SV_Disconnect_f( client_t *cl, const Cmd::Args& )
{
	SV_DropClient( cl, "disconnected" );
}

/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C friendly form.
=================
*/
void SV_UserinfoChanged( client_t *cl )
{
	const char *val;
	int  i;

	// name for C code
	Q_strncpyz( cl->name, Info_ValueForKey( cl->userinfo, "name" ), sizeof( cl->name ) );

	// rate command

	// if the client is on the same subnet as the server and we aren't running an
	// Internet server, assume that they don't need a rate choke
	if ( Sys_IsLANAddress( cl->netchan.remoteAddress ) && isLanOnly.Get() && sv_lanForceRate->integer == 1 )
	{
		cl->rate = 99999; // lans should not rate limit
	}
	else
	{
		val = Info_ValueForKey( cl->userinfo, "rate" );

		if ( strlen( val ) )
		{
			i = atoi( val );
			cl->rate = i;

			if ( cl->rate < 1000 )
			{
				cl->rate = 1000;
			}
			else if ( cl->rate > 90000 )
			{
				cl->rate = 90000;
			}
		}
		else
		{
			cl->rate = 5000;
		}
	}

	// snaps command
	val = Info_ValueForKey( cl->userinfo, "snaps" );

	if ( strlen( val ) )
	{
		i = atoi( val );

		if ( i < 1 )
		{
			i = 1;
		}
		else if ( i > sv_fps->integer )
		{
			i = sv_fps->integer;
		}

		cl->snapshotMsec = 1000 / i;
	}
	else
	{
		cl->snapshotMsec = 50;
	}

#ifdef USE_VOIP
	// in the future, (val) will be a protocol version string, so only
	//  accept explicitly 1, not generally non-zero.
	val = Info_ValueForKey( cl->userinfo, "cl_voip" );
	cl->hasVoip = ( atoi( val ) == 1 ) ? true : false;
#endif

	// TTimo
	// maintain the IP information
	// this is set in SV_DirectConnect (directly on the server, not transmitted), may be lost when client updates its userinfo
	// the banning code relies on this being consistently present
	// zinx - modified to always keep this consistent, instead of only
	// when "ip" is 0-length, so users can't supply their own IP address
	//Com_DPrintf("Maintain IP address in userinfo for '%s'\n", cl->name);
	if ( !NET_IsLocalAddress( cl->netchan.remoteAddress ) )
	{
		Info_SetValueForKey( cl->userinfo, "ip", NET_AdrToString( cl->netchan.remoteAddress ), false );
#ifdef HAVE_GEOIP
		Info_SetValueForKey( cl->userinfo, "geoip", NET_GeoIP_Country( &cl->netchan.remoteAddress ), false );
#endif
	}
	else
	{
		// force the "ip" info key to "localhost" for local clients
		Info_SetValueForKey( cl->userinfo, "ip", "localhost", false );
#ifdef HAVE_GEOIP
		Info_SetValueForKey( cl->userinfo, "geoip", nullptr, false );
#endif
	}
}

/*
==================
SV_UpdateUserinfo_f
==================
*/
static void SV_UpdateUserinfo_f( client_t *cl, const Cmd::Args& args )
{
	if (args.Argc() < 2) {
		return;
	}

	Q_strncpyz(cl->userinfo, args.Argv(1).c_str(), sizeof(cl->userinfo)); // FIXME QUOTING INFO

	SV_UserinfoChanged( cl );
	// call prog code to allow overrides
	gvm.GameClientUserInfoChanged( cl - svs.clients );
}

#ifdef USE_VOIP
static
void SV_UpdateVoipIgnore( client_t *cl, const char *idstr, bool ignore )
{
	if ( ( *idstr >= '0' ) && ( *idstr <= '9' ) )
	{
		const int id = atoi( idstr );

		if ( ( id >= 0 ) && ( id < MAX_CLIENTS ) )
		{
			cl->ignoreVoipFromClient[ id ] = ignore;
		}
	}
}

/*
==================
SV_Voip_f
==================
*/
static void SV_Voip_f( client_t *cl, const Cmd::Args& args )
{
	if (args.Argc() < 2) {
		return;
	}
	const char *cmd = args.Argv(1).c_str();

	if ( strcmp( cmd, "ignore" ) == 0 and args.Argc() >= 3)
	{
		SV_UpdateVoipIgnore( cl, args.Argv(2).c_str(), true );
	}
	else if ( strcmp( cmd, "unignore" ) == 0 and args.Argc() >= 3)
	{
		SV_UpdateVoipIgnore( cl, args.Argv(2).c_str(), false );
	}
	else if ( strcmp( cmd, "muteall" ) == 0 )
	{
		cl->muteAllVoip = true;
	}
	else if ( strcmp( cmd, "unmuteall" ) == 0 )
	{
		cl->muteAllVoip = false;
	}
}

#endif

typedef struct
{
	const char *name;
	void ( *func )( client_t *cl, const Cmd::Args& args );
	bool allowedpostmapchange;
} ucmd_t;

static ucmd_t ucmds[] =
{
	{ "userinfo",   SV_UpdateUserinfo_f,  false },
	{ "disconnect", SV_Disconnect_f,      true  },
	{ "download",   SV_BeginDownload_f,   false },
	{ "nextdl",     SV_NextDownload_f,    false },
	{ "stopdl",     SV_StopDownload_f,    false },
	{ "donedl",     SV_DoneDownload_f,    false },
#ifdef USE_VOIP
	{ "voip",       SV_Voip_f,            false },
#endif
	{ "wwwdl",      SV_WWWDownload_f,     false },
	{ nullptr,         nullptr, false}
};

/*
==================
SV_ExecuteClientCommand

Also called by bot code
==================
*/

Log::Logger clientCommands("server.clientCommands");
void SV_ExecuteClientCommand( client_t *cl, const char *s, bool clientOK, bool premaprestart )
{
	ucmd_t   *u;
	bool bProcessed = false;

	Com_DPrintf( "EXCL: %s\n", s );
	Cmd::Args args(s);

	if (args.Argc() == 0) {
		return;
	}

    clientCommands.Debug("Client %s sent command '%s'", cl->name, s);
	for (u = ucmds; u->name; u++) {
		if (args.Argv(0) == u->name) {
			if (premaprestart && !u->allowedpostmapchange) {
				continue;
			}

			u->func(cl, args);
			bProcessed = true;
			break;
		}
	}

	if ( clientOK )
	{
		// pass unknown strings to the game
		if ( !u->name && sv.state == SS_GAME )
		{
			gvm.GameClientCommand( cl - svs.clients, s );
		}
	}
	else if ( !bProcessed )
	{
		Com_DPrintf( "client text ignored for %s^7: %s\n", cl->name, args.Argv(0).c_str());
	}
}

/*
===============
SV_ClientCommand
===============
*/
static bool SV_ClientCommand( client_t *cl, msg_t *msg, bool premaprestart )
{
	int        seq;
	const char *s;
	bool   clientOk = true;
	bool   floodprotect = true;

	seq = MSG_ReadLong( msg );
	s = MSG_ReadString( msg );

	// see if we have already executed it
	if ( cl->lastClientCommand >= seq )
	{
		return true;
	}

	Com_DPrintf( "clientCommand: %s^7 : %i : %s\n", cl->name, seq, s );

	// drop the connection if we have somehow lost commands
	if ( seq > cl->lastClientCommand + 1 )
	{
		Com_Printf( "Client %s lost %i clientCommands\n", cl->name, seq - cl->lastClientCommand + 1 );
		SV_DropClient( cl, "Lost reliable commands" );
		return false;
	}

	// Gordon: AHA! Need to steal this for some other stuff BOOKMARK
	// NERVE - SMF - some server game-only commands we cannot have flood protect
	if ( !Q_strncmp( "team", s, 4 ) || !Q_strncmp( "setspawnpt", s, 10 ) || !Q_strncmp( "score", s, 5 ) || !Q_stricmp( "forcetapout", s ) )
	{
//      Com_DPrintf( "Skipping flood protection for: %s\n", s );
		floodprotect = false;
	}

	// malicious users may try using too many string commands
	// to lag other players.  If we decide that we want to stall
	// the command, we will stop processing the rest of the packet,
	// including the usercmd.  This causes flooders to lag themselves
	// but not other people
	// We don't do this when the client hasn't been active yet, since it is
	// by protocol to spam a lot of commands when downloading
	if ( !com_cl_running->integer && cl->state >= CS_ACTIVE && // (SA) this was commented out in Wolf.  Did we do that?
	     sv_floodProtect->integer && svs.time < cl->nextReliableTime && floodprotect )
	{
		// ignore any other text messages from this client but let them keep playing
		// TTimo - moved the ignored verbose to the actual processing in SV_ExecuteClientCommand, only printing if the core doesn't intercept
		clientOk = false;
	}

	// don't allow another command for 800 msec
	if ( floodprotect && svs.time >= cl->nextReliableTime )
	{
		cl->nextReliableTime = svs.time + 800;
	}

	SV_ExecuteClientCommand( cl, s, clientOk, premaprestart );

	cl->lastClientCommand = seq;
	Com_sprintf( cl->lastClientCommandString, sizeof( cl->lastClientCommandString ), "%s", s );

	return true; // continue processing
}

//==================================================================================

/*
==================
SV_ClientThink

Also called by bot code
==================
*/
void SV_ClientThink( client_t *cl, usercmd_t *cmd )
{
	cl->lastUsercmd = *cmd;

	if ( cl->state != CS_ACTIVE )
	{
		return; // may have been kicked during the last usercmd
	}

	gvm.GameClientThink( cl - svs.clients );
}

/*
==================
SV_UserMove

The message usually contains all the movement commands
that were in the last three packets, so that the information
in dropped packets can be recovered.

On very fast clients, there may be multiple usercmd packed into
each of the backup packets.
==================
*/
static void SV_UserMove( client_t *cl, msg_t *msg, bool delta )
{
	int       i;
	int       cmdCount;
	usercmd_t nullcmd;
	usercmd_t cmds[ MAX_PACKET_USERCMDS ];
	usercmd_t *cmd, *oldcmd;

	if ( delta )
	{
		cl->deltaMessage = cl->messageAcknowledge;
	}
	else
	{
		cl->deltaMessage = -1;
	}

	cmdCount = MSG_ReadByte( msg );

	if ( cmdCount < 1 )
	{
		Com_Printf( "cmdCount < 1\n" );
		return;
	}

	if ( cmdCount > MAX_PACKET_USERCMDS )
	{
		Com_Printf( "cmdCount > MAX_PACKET_USERCMDS\n" );
		return;
	}

	memset( &nullcmd, 0, sizeof( nullcmd ) );
	oldcmd = &nullcmd;

	for ( i = 0; i < cmdCount; i++ )
	{
		cmd = &cmds[ i ];
		MSG_ReadDeltaUsercmd( msg, oldcmd, cmd );
//      MSG_ReadDeltaUsercmd( msg, oldcmd, cmd );
		oldcmd = cmd;
	}

	// save time for ping calculation
	cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = svs.time;

	// if this is the first usercmd we have received
	// this gamestate, put the client into the world
	if ( cl->state == CS_PRIMED )
	{
		SV_ClientEnterWorld( cl, &cmds[ 0 ] );
		// the moves can be processed normaly
	}

	if ( cl->state != CS_ACTIVE )
	{
		cl->deltaMessage = -1;
		return;
	}

	// usually, the first couple commands will be duplicates
	// of ones we have previously received, but the servertimes
	// in the commands will cause them to be immediately discarded
	for ( i = 0; i < cmdCount; i++ )
	{
		// if this is a cmd from before a map_restart ignore it
		if ( cmds[ i ].serverTime > cmds[ cmdCount - 1 ].serverTime )
		{
			continue;
		}

		// extremely lagged or cmd from before a map_restart
		if ( cmds[ i ].serverTime <= cl->lastUsercmd.serverTime )
		{
			continue;
		}

		SV_ClientThink( cl, &cmds[ i ] );
	}
}

/*
=====================
SV_ParseBinaryMessage
=====================
*/
static void SV_ParseBinaryMessage( client_t *cl, msg_t *msg )
{
	int size;

	MSG_BeginReadingUncompressed( msg );

	size = msg->cursize - msg->readcount;

	if ( size <= 0 || size > MAX_BINARY_MESSAGE )
	{
		return;
	}

	SV_GameBinaryMessageReceived( cl - svs.clients, ( char * ) &msg->data[ msg->readcount ], size, cl->lastUsercmd.serverTime );
}

#ifdef USE_VOIP

/*
==================
SV_ShouldIgnoreVoipSender

Blocking of voip packets based on source client
==================
*/

static bool SV_ShouldIgnoreVoipSender( const client_t *cl )
{
	if ( !sv_voip->integer )
	{
		return true; // VoIP disabled on this server.
	}
	else if ( !cl->hasVoip ) // client doesn't have VoIP support?!
	{
		return true;
	}

	// !!! FIXME: implement player blacklist.

	return false; // don't ignore.
}

static
void SV_UserVoip( client_t *cl, msg_t *msg )
{
	int                sender, generation, sequence, frames, packetsize;
	uint8_t            recips[( MAX_CLIENTS + 7 ) / 8 ];
	int                flags;
	byte               encoded[ sizeof( cl->voipPacket[ 0 ]->data ) ];
	client_t           *client = nullptr;
	voipServerPacket_t *packet = nullptr;
	int                i;

	sender = cl - svs.clients;
	generation = MSG_ReadByte( msg );
	sequence = MSG_ReadLong( msg );
	frames = MSG_ReadByte( msg );
	MSG_ReadData( msg, recips, sizeof( recips ) );
	flags = MSG_ReadByte( msg );
	packetsize = MSG_ReadShort( msg );

	if ( msg->readcount > msg->cursize )
	{
		return; // short/invalid packet, bail.
	}

	if ( packetsize > sizeof( encoded ) )  // overlarge packet?
	{
		int bytesleft = packetsize;

		while ( bytesleft )
		{
			int br = bytesleft;

			if ( br > sizeof( encoded ) )
			{
				br = sizeof( encoded );
			}

			MSG_ReadData( msg, encoded, br );
			bytesleft -= br;
		}

		return; // overlarge packet, bail.
	}

	MSG_ReadData( msg, encoded, packetsize );

	if ( SV_ShouldIgnoreVoipSender( cl ) )
	{
		return; // Blacklisted, disabled, etc.
	}

	// !!! FIXME: see if we read past end of msg...

	// !!! FIXME: reject if not speex narrowband codec.
	// !!! FIXME: decide if this is bogus data?

	// decide who needs this VoIP packet sent to them...
	for ( i = 0, client = svs.clients; i < sv_maxclients->integer; i++, client++ )
	{
		if ( client->state != CS_ACTIVE )
		{
			continue; // not in the game yet, don't send to this guy.
		}
		else if ( i == sender )
		{
			continue; // don't send voice packet back to original author.
		}
		else if ( !client->hasVoip )
		{
			continue; // no VoIP support, or unsupported protocol
		}
		else if ( client->muteAllVoip )
		{
			continue; // client is ignoring everyone.
		}
		else if ( client->ignoreVoipFromClient[ sender ] )
		{
			continue; // client is ignoring this talker.
		}
		else if ( *cl->downloadName ) // !!! FIXME: possible to DoS?
		{
			continue; // no VoIP allowed if downloading, to save bandwidth.
		}

		if ( Com_IsVoipTarget( recips, sizeof( recips ), i ) )
		{
			flags |= VOIP_DIRECT;
		}
		else
		{
			flags &= ~VOIP_DIRECT;
		}

		if ( !( flags & ( VOIP_SPATIAL | VOIP_DIRECT ) ) )
		{
			continue; // not addressed to this player.
		}

		// Transmit this packet to the client.
		if ( client->queuedVoipPackets >= ARRAY_LEN( client->voipPacket ) )
		{
			Com_Printf( "Too many VoIP packets queued for client #%d\n", i );
			continue; // no room for another packet right now.
		}

		packet = ( voipServerPacket_t* ) Z_Malloc( sizeof( *packet ) );
		packet->sender = sender;
		packet->frames = frames;
		packet->len = packetsize;
		packet->generation = generation;
		packet->sequence = sequence;
		packet->flags = flags;
		memcpy( packet->data, encoded, packetsize );

		client->voipPacket[( client->queuedVoipIndex + client->queuedVoipPackets ) % ARRAY_LEN( client->voipPacket ) ] = packet;
		client->queuedVoipPackets++;
	}
}

#endif

/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/

/*
===================
SV_ExecuteClientMessage

Parse a client packet
===================
*/
void SV_ExecuteClientMessage( client_t *cl, msg_t *msg )
{
	int c;
	int serverId;

	MSG_Bitstream( msg );

	serverId = MSG_ReadLong( msg );
	cl->messageAcknowledge = MSG_ReadLong( msg );

	if ( cl->messageAcknowledge < 0 )
	{
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
#ifndef NDEBUG
		SV_DropClient( cl, "DEBUG: illegible client message" );
#endif
		return;
	}

	cl->reliableAcknowledge = MSG_ReadLong( msg );

	// NOTE: when the client message is fux0red the acknowledgement numbers
	// can be out of range, this could cause the server to send thousands of server
	// commands which the server thinks are not yet acknowledged in SV_UpdateServerCommandsToClient
	if ( cl->reliableAcknowledge < cl->reliableSequence - MAX_RELIABLE_COMMANDS )
	{
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
#ifndef NDEBUG
		SV_DropClient( cl, "DEBUG: illegible client message" );
#endif
		cl->reliableAcknowledge = cl->reliableSequence;
		return;
	}

	// if this is a usercmd from a previous gamestate,
	// ignore it or retransmit the current gamestate
	//
	// if the client was downloading, let it stay at whatever serverId and
	// gamestate it was at.  This allows it to keep downloading even when
	// the gamestate changes.  After the download is finished, we'll
	// notice and send it a new game state
	//
	// show_bug.cgi?id=536
	// don't drop as long as previous command was a nextdl, after a dl is done, downloadName is set back to ""
	// but we still need to read the next message to move to next download or send gamestate
	// I don't like this hack though, it must have been working fine at some point, suspecting the fix is somewhere else
	if ( serverId != sv.serverId && !*cl->downloadName && !strstr( cl->lastClientCommandString, "nextdl" ) )
	{
		if ( serverId >= sv.restartedServerId && serverId < sv.serverId )
		{
			// TTimo - use a comparison here to catch multiple map_restart
			// they just haven't caught the map_restart yet
			Com_DPrintf( "%s^7: ignoring pre map_restart / outdated client message\n", cl->name );
			return;
		}

		// if we can tell that the client has dropped the last
		// gamestate we sent them, resend it
		if ( cl->messageAcknowledge > cl->gamestateMessageNum )
		{
			Com_DPrintf( "%s^7: dropped gamestate, resending\n", cl->name );
			SV_SendClientGameState( cl );
		}

		// read optional clientCommand strings
		do
		{
			c = MSG_ReadByte( msg );

			if ( c == clc_EOF )
			{
				break;
			}

			if ( c != clc_clientCommand )
			{
				break;
			}

			if ( !SV_ClientCommand( cl, msg, true ) )
			{
				return; // we couldn't execute it because of the flood protection
			}

			if ( cl->state == CS_ZOMBIE )
			{
				return; // disconnect command
			}
		}
		while ( 1 );

		return;
	}

	// read optional clientCommand strings
	do
	{
		c = MSG_ReadByte( msg );

		if ( c == clc_EOF )
		{
			break;
		}

		if ( c != clc_clientCommand )
		{
			break;
		}

		if ( !SV_ClientCommand( cl, msg, false ) )
		{
			return; // we couldn't execute it because of the flood protection
		}

		if ( cl->state == CS_ZOMBIE )
		{
			return; // disconnect command
		}
	}
	while ( 1 );

	if ( c == clc_voip )
	{
#ifdef USE_VOIP
		SV_UserVoip( cl, msg );
		c = MSG_ReadByte( msg );
#endif
	}

	// read the usercmd_t
	if ( c == clc_move )
	{
		SV_UserMove( cl, msg, true );
	}
	else if ( c == clc_moveNoDelta )
	{
		SV_UserMove( cl, msg, false );
	}
	else if ( c != clc_EOF )
	{
		Com_Printf( "WARNING: bad command byte for client %i\n", ( int )( cl - svs.clients ) );
	}

	SV_ParseBinaryMessage( cl, msg );

//  if ( msg->readcount != msg->cursize ) {
//      Com_Printf( "WARNING: Junk at end of packet for client %i\n", cl - svs.clients );
//  }
}
