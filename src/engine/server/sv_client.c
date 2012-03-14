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
#include "../qcommon/md4.h"

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
sent to that ip.
=================
*/
void SV_GetChallenge( netadr_t from )
{
	int         i;
	int         oldest;
	int         oldestTime;
	challenge_t *challenge;

	// ignore if we are in single player
	if ( SV_GameIsSinglePlayer() )
	{
		return;
	}

	if ( SV_TempBanIsBanned( from ) )
	{
		NET_OutOfBandPrint( NS_SERVER, from, "print\n%s\n", sv_tempbanmessage->string );
		return;
	}

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
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
		challenge->connected = qfalse;
		i = oldest;
	}

#if !defined( AUTHORIZE_SUPPORT )

	// FIXME: deal with restricted filesystem
	if ( 1 )
	{
#else

	// if they are on a lan address, send the challengeResponse immediately
	if ( Sys_IsLANAddress( from ) )
	{
#endif
		challenge->pingTime = svs.time;

		if ( sv_onlyVisibleClients->integer )
		{
			NET_OutOfBandPrint( NS_SERVER, from, "challengeResponse %i %i", challenge->challenge, sv_onlyVisibleClients->integer );
		}
		else
		{
			NET_OutOfBandPrint( NS_SERVER, from, "challengeResponse %i", challenge->challenge );
		}

		return;
	}

#ifdef AUTHORIZE_SUPPORT

	// look up the authorize server's IP
	if ( !svs.authorizeAddress.ip[ 0 ] && svs.authorizeAddress.type != NA_BAD )
	{
		Com_Printf( "Resolving %s\n", AUTHORIZE_SERVER_NAME );

		if ( !NET_StringToAdr( AUTHORIZE_SERVER_NAME, &svs.authorizeAddress, NA_UNSPEC ) )
		{
			Com_Printf( "Couldn't resolve address\n" );
			return;
		}

		svs.authorizeAddress.port = BigShort( PORT_AUTHORIZE );
		Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", AUTHORIZE_SERVER_NAME,
		            svs.authorizeAddress.ip[ 0 ], svs.authorizeAddress.ip[ 1 ],
		            svs.authorizeAddress.ip[ 2 ], svs.authorizeAddress.ip[ 3 ],
		            BigShort( svs.authorizeAddress.port ) );
	}

	// if they have been challenging for a long time and we
	// haven't heard anything from the authoirze server, go ahead and
	// let them in, assuming the id server is down
	if ( svs.time - challenge->firstTime > AUTHORIZE_TIMEOUT )
	{
		Com_DPrintf( "authorize server timed out\n" );

		challenge->pingTime = svs.time;

		if ( sv_onlyVisibleClients->integer )
		{
			NET_OutOfBandPrint( NS_SERVER, challenge->adr,
			                    "challengeResponse %i %i", challenge->challenge, sv_onlyVisibleClients->integer );
		}
		else
		{
			NET_OutOfBandPrint( NS_SERVER, challenge->adr,
			                    "challengeResponse %i", challenge->challenge );
		}

		return;
	}

	// otherwise send their ip to the authorize server
	if ( svs.authorizeAddress.type != NA_BAD )
	{
		cvar_t *fs;
		char   game[ 1024 ];

		game[ 0 ] = 0;
		fs = Cvar_Get( "fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO );

		if ( fs && fs->string[ 0 ] != 0 )
		{
			strcpy( game, fs->string );
		}

		Com_DPrintf( "sending getIpAuthorize for %s\n", NET_AdrToString( from ) );
		fs = Cvar_Get( "sv_allowAnonymous", "0", CVAR_SERVERINFO );

		// NERVE - SMF - fixed parsing on sv_allowAnonymous
		NET_OutOfBandPrint( NS_SERVER, svs.authorizeAddress,
		                    "getIpAuthorize %i %i.%i.%i.%i %s %i",  svs.challenges[ i ].challenge,
		                    from.ip[ 0 ], from.ip[ 1 ], from.ip[ 2 ], from.ip[ 3 ], game, fs->integer );
	}

#endif // AUTHORIZE_SUPPORT
}

#ifdef AUTHORIZE_SUPPORT

/*
====================
SV_AuthorizeIpPacket

A packet has been returned from the authorize server.
If we have a challenge adr for that ip, send the
challengeResponse to it
====================
*/
void SV_AuthorizeIpPacket( netadr_t from )
{
	int  challenge;
	int  i;
	char *s;
	char *r;
	char ret[ 1024 ];

	if ( !NET_CompareBaseAdr( from, svs.authorizeAddress ) )
	{
		Com_Printf( "SV_AuthorizeIpPacket: not from authorize server\n" );
		return;
	}

	challenge = atoi( Cmd_Argv( 1 ) );

	for ( i = 0; i < MAX_CHALLENGES; i++ )
	{
		if ( svs.challenges[ i ].challenge == challenge )
		{
			break;
		}
	}

	if ( i == MAX_CHALLENGES )
	{
		Com_Printf( "SV_AuthorizeIpPacket: challenge not found\n" );
		return;
	}

	// send a packet back to the original client
	svs.challenges[ i ].pingTime = svs.time;
	s = Cmd_Argv( 2 );
	r = Cmd_Argv( 3 );  // reason

	if ( !Q_stricmp( s, "ettest" ) )
	{
		if ( Cvar_VariableValue( "fs_restrict" ) )
		{
			// a demo client connecting to a demo server
			NET_OutOfBandPrint( NS_SERVER, svs.challenges[ i ].adr,
			                    "challengeResponse %i", svs.challenges[ i ].challenge );
			return;
		}

		// they are a demo client trying to connect to a real server
		NET_OutOfBandPrint( NS_SERVER, svs.challenges[ i ].adr, "print\nServer is not a demo server\n" );
		// clear the challenge record so it won't timeout and let them through
		memset( &svs.challenges[ i ], 0, sizeof( svs.challenges[ i ] ) );
		return;
	}

	if ( !Q_stricmp( s, "accept" ) )
	{
		if ( sv_onlyVisibleClients->integer )
		{
			NET_OutOfBandPrint( NS_SERVER, svs.challenges[ i ].adr,
			                    "challengeResponse %i %i", svs.challenges[ i ].challenge, sv_onlyVisibleClients->integer );
		}
		else
		{
			NET_OutOfBandPrint( NS_SERVER, svs.challenges[ i ].adr,
			                    "challengeResponse %i", svs.challenges[ i ].challenge );
		}

		return;
	}

	if ( !Q_stricmp( s, "unknown" ) )
	{
		if ( !r )
		{
			NET_OutOfBandPrint( NS_SERVER, svs.challenges[ i ].adr, "print\nAwaiting CD key authorization\n" );
		}
		else
		{
			sprintf( ret, "print\n%s\n", r );
			NET_OutOfBandPrint( NS_SERVER, svs.challenges[ i ].adr, ret );
		}

		// clear the challenge record so it won't timeout and let them through
		memset( &svs.challenges[ i ], 0, sizeof( svs.challenges[ i ] ) );
		return;
	}

	// authorization failed
	if ( !r )
	{
		NET_OutOfBandPrint( NS_SERVER, svs.challenges[ i ].adr, "print\nSomeone is using this CD Key\n" );
	}
	else
	{
		sprintf( ret, "print\n%s\n", r );
		NET_OutOfBandPrint( NS_SERVER, svs.challenges[ i ].adr, ret );
	}

	// clear the challenge record so it won't timeout and let them through
	memset( &svs.challenges[ i ], 0, sizeof( svs.challenges[ i ] ) );
}

#endif // AUTHORIZE_SUPPORT

/*
==================
SV_ApproveGuid

Returns a false value if and only if the client with this cl_guid
should not be allowed to enter the server.

A cl_guid string must have length 32 and consist of characters
'0' through '9' and 'A' through 'F'.
==================
*/
qboolean SV_ApproveGuid( const char *guid )
{
	int  i;
	char c;
	int  length;

	if ( sv_requireValidGuid->integer > 0 )
	{
		length = strlen( guid );  // could avoid this extra linear-time computation

		if ( length != 32 )
		{
			return qfalse;
		}

		for ( i = 31; i >= 0; )
		{
			c = guid[ i-- ];

			if ( !( ( '0' <= c && c <= '9' ) ||
			        ( 'A' <= c && c <= 'F' ) ) )
			{
				return qfalse;
			}
		}
	}

	return qtrue;
}

/*
==================
SV_DirectConnect

A "connect" OOB command has been received
==================
*/
void SV_DirectConnect( netadr_t from )
{
	char                userinfo[ MAX_INFO_STRING ];
	int                 i;
	client_t            *cl, *newcl;
	MAC_STATIC client_t temp;
	sharedEntity_t      *ent;
	int                 clientNum;
	int                 version;
	int                 qport;
	int                 challenge;
	char                *password;
	int                 startIndex;
	char                *denied;
	int                 count;
	char                *ip;
	int                 oldInfoLen2 = strlen( userinfo );
	int                 newInfoLen2;

	Com_DPrintf( "SVC_DirectConnect ()\n" );

	Q_strncpyz( userinfo, Cmd_Argv( 1 ), sizeof( userinfo ) );

	// DHM - Nerve :: Update Server allows any protocol to connect
	// NOTE TTimo: but we might need to store the protocol around for potential non http/ftp clients
	version = atoi( Info_ValueForKey( userinfo, "protocol" ) );

	if ( version != com_protocol->integer )
	{
		NET_OutOfBandPrint( NS_SERVER, from, "print\nServer uses protocol version %i (yours is %i).\n", com_protocol->integer, version );
		Com_DPrintf( "    rejected connect from version %i\n", version );
		return;
	}

	challenge = atoi( Info_ValueForKey( userinfo, "challenge" ) );
	qport = atoi( Info_ValueForKey( userinfo, "qport" ) );

	if ( SV_TempBanIsBanned( from ) )
	{
		NET_OutOfBandPrint( NS_SERVER, from, "print\n%s\n", sv_tempbanmessage->string );
		return;
	}

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
				Com_DPrintf( "%s:reconnect rejected : too soon\n", NET_AdrToString( from ) );
				return;
			}

			break;
		}
	}

	// block connections for invalid GUIDs
	if ( !SV_ApproveGuid( Info_ValueForKey( userinfo, "cl_guid" ) ) )
	{
		NET_OutOfBandPrint( NS_SERVER, from, "print\nGet legit, bro.\n" );
		Com_DPrintf( "Invalid cl_guid, rejected connect from %s\n", NET_AdrToString( from ) );
		return;
	}

	// See comment made in SV_UserinfoChanged() regarding handicap.

	while ( qtrue )
	{
		// Unfortunately the string fuctions such as strlen() and Info_RemoveKey()
		// are quite expensive for large userinfo strings.  Imagine if someone
		// bombarded the server with connect packets.  That would result in very bad
		// server hitches.  We need to fix that.
		Info_RemoveKey( userinfo, "handicap" );
		newInfoLen2 = strlen( userinfo );

		if ( oldInfoLen2 == newInfoLen2 ) { break; } // userinfo wasn't modified.

		oldInfoLen2 = newInfoLen2;
	}

	if ( NET_IsLocalAddress( from ) )
	{
		ip = "localhost";
	}
	else
	{
		ip = ( char * ) NET_AdrToString( from );
	}

	if ( ( strlen( ip ) + strlen( userinfo ) + 4 ) >= MAX_INFO_STRING )
	{
		NET_OutOfBandPrint( NS_SERVER, from,
		                    "print\nUserinfo string length exceeded.  "
		                    "Try removing setu cvars from your config.\n" );
		return;
	}

	Info_SetValueForKey( userinfo, "ip", ip );

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
			NET_OutOfBandPrint( NS_SERVER, from, "print\n[err_dialog]No or bad challenge for address.\n" );
			return;
		}

		// force the IP key/value pair so the game can filter based on ip
		Info_SetValueForKey( userinfo, "ip", NET_AdrToString( from ) );

		if ( svs.challenges[ i ].firstPing == 0 )
		{
			ping = svs.time - svs.challenges[ i ].pingTime;
			svs.challenges[ i ].firstPing = ping;
		}
		else
		{
			ping = svs.challenges[ i ].firstPing;
		}

		Com_Printf( "Client %i connecting with %i challenge ping\n", i, ping );
		svs.challenges[ i ].connected = qtrue;

		// never reject a LAN client based on ping
		if ( !Sys_IsLANAddress( from ) )
		{
			if ( sv_minPing->value && ping < sv_minPing->value )
			{
				NET_OutOfBandPrint( NS_SERVER, from, "print\n[err_dialog]Server is for high pings only\n" );
				Com_DPrintf( "Client %i rejected on a too low ping\n", i );
				return;
			}

			if ( sv_maxPing->value && ping > sv_maxPing->value )
			{
				NET_OutOfBandPrint( NS_SERVER, from, "print\n[err_dialog]Server is for low pings only\n" );
				Com_DPrintf( "Client %i rejected on a too high ping: %i\n", i, ping );
				return;
			}
		}
	}
	else
	{
		// force the "ip" info key to "localhost"
		Info_SetValueForKey( userinfo, "ip", "localhost" );
	}

	newcl = &temp;
	memset( newcl, 0, sizeof( client_t ) );

	// if there is already a slot for this ip, reuse it
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

	newcl = NULL;

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

				if ( cl->netchan.remoteAddress.type == NA_BOT )
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
				Com_Error( ERR_FATAL, "server is full on local connect\n" );
				return;
			}
		}
		else
		{
			NET_OutOfBandPrint( NS_SERVER, from, "print\n%s\n", sv_fullmsg->string );
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
	*newcl = temp;
	clientNum = newcl - svs.clients;
	ent = SV_GentityNum( clientNum );
	newcl->gentity = ent;

	// save the challenge
	newcl->challenge = challenge;

	// save the address
	Netchan_Setup( NS_SERVER, &newcl->netchan, from, qport );
	// init the netchan queue

	// save the userinfo
	Q_strncpyz( newcl->userinfo, userinfo, sizeof( newcl->userinfo ) );

	// get the game a chance to reject this connection or modify the userinfo
	denied = ( char * ) VM_Call( gvm, GAME_CLIENT_CONNECT, clientNum, qtrue, qfalse );  // firstTime = qtrue

	if ( denied )
	{
		// we can't just use VM_ArgPtr, because that is only valid inside a VM_Call
		denied = VM_ExplicitArgPtr( gvm, ( intptr_t ) denied );

		NET_OutOfBandPrint( NS_SERVER, from, "print\n[err_dialog]%s\n", denied );
		Com_DPrintf( "Game rejected a connection: %s.\n", denied );
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
	int         i;
	challenge_t *challenge;
	qboolean    isBot = qfalse;

	if ( drop->state == CS_ZOMBIE )
	{
		return; // already dropped
	}

	if ( drop->gentity && ( drop->gentity->r.svFlags & SVF_BOT ) )
	{
		isBot = qtrue;
	}
	else
	{
		if ( drop->netchan.remoteAddress.type == NA_BOT )
		{
			isBot = qtrue;
		}
	}

	if ( !isBot )
	{
		// see if we already have a challenge for this ip
		challenge = &svs.challenges[ 0 ];

		for ( i = 0; i < MAX_CHALLENGES; i++, challenge++ )
		{
			if ( NET_CompareAdr( drop->netchan.remoteAddress, challenge->adr ) )
			{
				challenge->connected = qfalse;
				break;
			}
		}

		// Kill any download
		SV_CloseDownload( drop );
	}

	// Free all allocated data on the client structure
	SV_FreeClient( drop );

	if ( ( !SV_GameIsSinglePlayer() ) || ( !isBot ) )
	{
		// tell everyone why they got dropped

		// Gordon: we want this displayed elsewhere now
		SV_SendServerCommand( NULL, "print \"%s" S_COLOR_WHITE " %s\n\"", drop->name, reason );
	}

	Com_DPrintf( "Going to CS_ZOMBIE for %s\n", drop->name );
	drop->state = CS_ZOMBIE; // become free in a few seconds

	if ( drop->download )
	{
		FS_FCloseFile( drop->download );
		drop->download = 0;
	}

	// call the prog function for removing a client
	// this will remove the body, among other things
	VM_Call( gvm, GAME_CLIENT_DISCONNECT, drop - svs.clients );

	// add the disconnect command
	SV_SendServerCommand( drop, "disconnect \"%s\"\n", reason );

	if ( drop->netchan.remoteAddress.type == NA_BOT )
	{
		SV_BotFreeClient( drop - svs.clients );
	}

	// nuke user info
	SV_SetUserinfo( drop - svs.clients, "" );

	// if this was the last client on the server, send a heartbeat
	// to the master so it is known the server is empty
	// send a heartbeat now so the master will get up to date info
	// if there is already a slot for this ip, reuse it
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
	client->pureAuthentic = 0;
	client->gotCP = qfalse;

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
		MSG_WriteDeltaEntity( &msg, &nullstate, base, qtrue );
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
	VM_Call( gvm, GAME_CLIENT_BEGIN, client - svs.clients );
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
		FS_FCloseFile( cl->download );
	}

	cl->download = 0;
	*cl->downloadName = 0;

	// Free the temporary buffer space
	for ( i = 0; i < MAX_DOWNLOAD_WINDOW; i++ )
	{
		if ( cl->downloadBlocks[ i ] )
		{
			Z_Free( cl->downloadBlocks[ i ] );
			cl->downloadBlocks[ i ] = NULL;
		}
	}
}

/*
==================
SV_StopDownload_f

Abort a download if in progress
==================
*/
void SV_StopDownload_f( client_t *cl )
{
	if ( *cl->downloadName )
	{
		Com_DPrintf( "clientDownload: %d : file \"%s\" aborted\n", ( int )( cl - svs.clients ), cl->downloadName );
	}

	SV_CloseDownload( cl );
}

/*
==================
SV_DoneDownload_f

Downloads are finished
==================
*/
void SV_DoneDownload_f( client_t *cl )
{
	Com_DPrintf( "clientDownload: %s Done\n", cl->name );
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
void SV_NextDownload_f( client_t *cl )
{
	int block = atoi( Cmd_Argv( 1 ) );

	if ( block == cl->downloadClientBlock )
	{
		Com_DPrintf( "clientDownload: %d : client acknowledge of block %d\n", ( int )( cl - svs.clients ), block );

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
void SV_BeginDownload_f( client_t *cl )
{
	// Kill any existing download
	SV_CloseDownload( cl );

	//bani - stop us from printing dupe messages
	if ( strcmp( cl->downloadName, Cmd_Argv( 1 ) ) )
	{
		cl->downloadnotify = DLNOTIFY_ALL;
	}

	// cl->downloadName is non-zero now, SV_WriteDownloadToClient will see this and open
	// the file itself
	Q_strncpyz( cl->downloadName, Cmd_Argv( 1 ), sizeof( cl->downloadName ) );
}

/*
==================
SV_WWWDownload_f
==================
*/
void SV_WWWDownload_f( client_t *cl )
{
	char *subcmd = Cmd_Argv( 1 );

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
			Com_Printf( "WARNING: dupe wwwdl ack from client '%s'\n", cl->name );
		}

		cl->bWWWing = qtrue;
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
		cl->download = 0;
		*cl->downloadName = 0;
		cl->bWWWing = qfalse;
		return;
	}
	else if ( !Q_stricmp( subcmd, "fail" ) )
	{
		cl->download = 0;
		*cl->downloadName = 0;
		cl->bWWWing = qfalse;
		cl->bFallback = qtrue;
		// send a reconnect
		SV_SendClientGameState( cl );
		return;
	}
	else if ( !Q_stricmp( subcmd, "chkfail" ) )
	{
		Com_Printf( "WARNING: client '%s' reports that the redirect download for '%s' had wrong checksum.\n", cl->name,
		            cl->downloadName );
		Com_Printf( "         you should check your download redirect configuration.\n" );
		cl->download = 0;
		*cl->downloadName = 0;
		cl->bWWWing = qfalse;
		cl->bFallback = qtrue;
		// send a reconnect
		SV_SendClientGameState( cl );
		return;
	}

	Com_Printf( "SV_WWWDownload: unknown wwwdl subcommand '%s' for client '%s'\n", subcmd, cl->name );
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
static qboolean SV_CheckFallbackURL( client_t *cl, msg_t *msg )
{
	if ( !sv_wwwFallbackURL->string || strlen( sv_wwwFallbackURL->string ) == 0 )
	{
		return qfalse;
	}

	Com_Printf( "clientDownload: sending client '%s' to fallback URL '%s'\n", cl->name, sv_wwwFallbackURL->string );

	MSG_WriteByte( msg, svc_download );
	MSG_WriteShort( msg, -1 );  // block -1 means ftp/http download
	MSG_WriteString( msg, sv_wwwFallbackURL->string );
	MSG_WriteLong( msg, 0 );
	MSG_WriteLong( msg, 2 );  // DL_FLAG_URL

	return qtrue;
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
	int      idPack;
	char     errorMessage[ 1024 ];
	int      download_flag;

	qboolean bTellRate = qfalse; // verbosity

	if ( !*cl->downloadName )
	{
		return; // Nothing being downloaded
	}

	if ( cl->bWWWing )
	{
		return; // The client acked and is downloading with ftp/http
	}

	// CVE-2006-2082
	// validate the download against the list of pak files
	if ( !FS_VerifyPak( cl->downloadName ) )
	{
		// will drop the client and leave it hanging on the other side. good for him
		SV_DropClient( cl, "illegal download request" );
		return;
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

		idPack = FS_idPak( cl->downloadName, BASEGAME );

		// sv_allowDownload and idPack checks
		if ( !sv_allowDownload->integer || idPack )
		{
			// cannot auto-download file
			if ( idPack )
			{
				Com_Printf( "clientDownload: %d : \"%s\" cannot download id pk3 files\n", ( int )( cl - svs.clients ), cl->downloadName );
				Com_sprintf( errorMessage, sizeof( errorMessage ), "Cannot autodownload official pk3 file \"%s\"", cl->downloadName );
			}
			else
			{
				Com_Printf( "clientDownload: %d : \"%s\" download disabled", ( int )( cl - svs.clients ), cl->downloadName );

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
			}

			SV_BadDownload( cl, msg );
			MSG_WriteString( msg, errorMessage );  // (could SV_DropClient isntead?)

			return;
		}

		// www download redirect protocol
		// NOTE: this is called repeatedly while a client connects. Maybe we should sort of cache the message or something
		// FIXME: we need to abstract this to an independant module for maximum configuration/usability by server admins
		// FIXME: I could rework that, it's crappy
		if ( sv_wwwDownload->integer )
		{
			if ( cl->bDlOK )
			{
				if ( !cl->bFallback )
				{
					fileHandle_t handle;
					int          downloadSize = FS_SV_FOpenFileRead( cl->downloadName, &handle );

					if ( downloadSize )
					{
						FS_FCloseFile( handle );  // don't keep open, we only care about the size

						Q_strncpyz( cl->downloadURL, va( "%s/%s", sv_wwwBaseURL->string, cl->downloadName ),
						            sizeof( cl->downloadURL ) );

						//bani - prevent multiple download notifications
						if ( cl->downloadnotify & DLNOTIFY_REDIRECT )
						{
							cl->downloadnotify &= ~DLNOTIFY_REDIRECT;
							Com_Printf( "Redirecting client '%s' to %s\n", cl->name, cl->downloadURL );
						}

						// once cl->downloadName is set (and possibly we have our listening socket), let the client know
						cl->bWWWDl = qtrue;
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
						Com_Printf( "ERROR: Client '%s': couldn't extract file size for %s\n", cl->name, cl->downloadName );
					}
				}
				else
				{
					cl->bFallback = qfalse;

					if ( SV_CheckFallbackURL( cl, msg ) )
					{
						return;
					}

					Com_Printf( "Client '%s': falling back to regular downloading for failed file %s\n", cl->name,
					            cl->downloadName );
				}
			}
			else
			{
				if ( SV_CheckFallbackURL( cl, msg ) )
				{
					return;
				}

				Com_Printf( "Client '%s' is not configured for www download\n", cl->name );
			}
		}

		// find file
		cl->bWWWDl = qfalse;
		cl->downloadSize = FS_SV_FOpenFileRead( cl->downloadName, &cl->download );

		if ( cl->downloadSize <= 0 )
		{
			Com_Printf( "clientDownload: %d : \"%s\" file not found on server\n", ( int )( cl - svs.clients ), cl->downloadName );
			Com_sprintf( errorMessage, sizeof( errorMessage ), "File \"%s\" not found on server for autodownloading.\n",
			             cl->downloadName );
			SV_BadDownload( cl, msg );
			MSG_WriteString( msg, errorMessage );  // (could SV_DropClient isntead?)
			return;
		}

		// is valid source, init
		cl->downloadCurrentBlock = cl->downloadClientBlock = cl->downloadXmitBlock = 0;
		cl->downloadCount = 0;
		cl->downloadEOF = qfalse;

		bTellRate = qtrue;
	}

	// Perform any reads that we need to
	while ( cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW && cl->downloadSize != cl->downloadCount )
	{
		curindex = ( cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW );

		if ( !cl->downloadBlocks[ curindex ] )
		{
			cl->downloadBlocks[ curindex ] = Z_Malloc( MAX_DOWNLOAD_BLKSIZE );
		}

		cl->downloadBlockSize[ curindex ] = FS_Read( cl->downloadBlocks[ curindex ], MAX_DOWNLOAD_BLKSIZE, cl->download );

		if ( cl->downloadBlockSize[ curindex ] < 0 )
		{
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

		cl->downloadEOF = qtrue; // We have added the EOF block
	}

	// Loop up to window size times based on how many blocks we can fit in the
	// client snapMsec and rate

	// based on the rate, how many bytes can we fit in the snapMsec time of the client
	// normal rate / snapshotMsec calculation
	rate = cl->rate;

	// show_bug.cgi?id=509
	// for autodownload, we use a seperate max rate value
	// we do this everytime because the client might change it's rate during the download
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

		Com_DPrintf( "clientDownload: %d : writing block %d\n", ( int )( cl - svs.clients ), cl->downloadXmitBlock );

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
static void SV_Disconnect_f( client_t *cl )
{
	SV_DropClient( cl, "disconnected" );
}

/*
=================
SV_VerifyPaks_f

If we are pure, disconnect the client if they do no meet the following conditions:

1. the first two checksums match our view of cgame and ui DLLs
   Wolf specific: the checksum is the checksum of the pk3 we found the DLL in
2. there are no any additional checksums that we do not have

This routine would be a bit simpler with a goto but i abstained

=================
*/
static void SV_VerifyPaks_f( client_t *cl )
{
	int        nChkSum1, nChkSum2, nClientPaks, nServerPaks, i, j, nCurArg;
	int        nClientChkSum[ 1024 ];
	int        nServerChkSum[ 1024 ];
	const char *pPaks, *pArg;
	qboolean   bGood = qtrue;

	// if we are pure, we "expect" the client to load certain things from
	// certain pk3 files, namely we want the client to have loaded the
	// ui and cgame that we think should be loaded based on the pure setting
	if ( sv_pure->integer != 0 )
	{
		bGood = qtrue;
		nChkSum1 = nChkSum2 = 0;

		bGood = ( FS_FileIsInPAK( Sys_GetDLLName( "cgame" ), &nChkSum1 ) == 1 );

		if ( bGood )
		{
			bGood = ( FS_FileIsInPAK( Sys_GetDLLName( "ui" ), &nChkSum2 ) == 1 );
		}

		nClientPaks = Cmd_Argc();

		// start at arg 2 ( skip serverId cl_paks )
		nCurArg = 1;

		pArg = Cmd_Argv( nCurArg++ );

		if ( !pArg )
		{
			bGood = qfalse;
		}
		else
		{
			// show_bug.cgi?id=475
			// we may get incoming cp sequences from a previous checksumFeed, which we need to ignore
			// since serverId is a frame count, it always goes up
			if ( atoi( pArg ) < sv.checksumFeedServerId )
			{
				Com_DPrintf( "ignoring outdated cp command from client %s\n", cl->name );
				return;
			}
		}

		// we basically use this while loop to avoid using 'goto' :)
		while ( bGood )
		{
			// must be at least 6: "cl_paks cgame ui @ firstref ... numChecksums"
			// numChecksums is encoded
			if ( nClientPaks < 6 )
			{
				bGood = qfalse;
				break;
			}

			// verify first to be the cgame checksum
			pArg = Cmd_Argv( nCurArg++ );

			if ( !pArg || *pArg == '@' || atoi( pArg ) != nChkSum1 )
			{
				Com_Printf( "nChkSum1 %d == %d\n", atoi( pArg ), nChkSum1 );
				bGood = qfalse;
				break;
			}

			// verify the second to be the ui checksum
			pArg = Cmd_Argv( nCurArg++ );

			if ( !pArg || *pArg == '@' || atoi( pArg ) != nChkSum2 )
			{
				Com_Printf( "nChkSum2 %d == %d\n", atoi( pArg ), nChkSum2 );
				bGood = qfalse;
				break;
			}

			// should be sitting at the delimeter now
			pArg = Cmd_Argv( nCurArg++ );

			if ( *pArg != '@' )
			{
				bGood = qfalse;
				break;
			}

			// store checksums since tokenization is not re-entrant
			for ( i = 0; nCurArg < nClientPaks; i++ )
			{
				nClientChkSum[ i ] = atoi( Cmd_Argv( nCurArg++ ) );
			}

			// store number to compare against (minus one cause the last is the number of checksums)
			nClientPaks = i - 1;

			// make sure none of the client check sums are the same
			// so the client can't send 5 the same checksums
			for ( i = 0; i < nClientPaks; i++ )
			{
				for ( j = 0; j < nClientPaks; j++ )
				{
					if ( i == j )
					{
						continue;
					}

					if ( nClientChkSum[ i ] == nClientChkSum[ j ] )
					{
						bGood = qfalse;
						break;
					}
				}

				if ( bGood == qfalse )
				{
					break;
				}
			}

			if ( bGood == qfalse )
			{
				break;
			}

			// get the pure checksums of the pk3 files loaded by the server
			pPaks = FS_LoadedPakPureChecksums();
			Cmd_TokenizeString( pPaks );
			nServerPaks = Cmd_Argc();

			if ( nServerPaks > 1024 )
			{
				nServerPaks = 1024;
			}

			for ( i = 0; i < nServerPaks; i++ )
			{
				nServerChkSum[ i ] = atoi( Cmd_Argv( i ) );
			}

			// check if the client has provided any pure checksums of pk3 files not loaded by the server
			for ( i = 0; i < nClientPaks; i++ )
			{
				for ( j = 0; j < nServerPaks; j++ )
				{
					if ( nClientChkSum[ i ] == nServerChkSum[ j ] )
					{
						break;
					}
				}

				if ( j >= nServerPaks )
				{
					bGood = qfalse;
					break;
				}
			}

			if ( bGood == qfalse )
			{
				break;
			}

			// check if the number of checksums was correct
			nChkSum1 = sv.checksumFeed;

			for ( i = 0; i < nClientPaks; i++ )
			{
				nChkSum1 ^= nClientChkSum[ i ];
			}

			nChkSum1 ^= nClientPaks;

			if ( nChkSum1 != nClientChkSum[ nClientPaks ] )
			{
				bGood = qfalse;
				break;
			}

			// break out
			break;
		}

		cl->gotCP = qtrue;

		if ( bGood )
		{
			cl->pureAuthentic = 1;
		}
		else
		{
			cl->pureAuthentic = 0;
			cl->nextSnapshotTime = -1;
			cl->state = CS_ACTIVE;
			SV_SendClientSnapshot( cl );
			SV_SendServerCommand( cl, "disconnect \"%s\"", "This is a pure server. This is caused by corrupted or missing files. Try turning on AutoDownload." );
			SV_SendServerCommand( cl, "Unpure client detected. Invalid .PK3 files referenced!" );
		}
	}
}

/*
=================
SV_ResetPureClient_f
=================
*/
static void SV_ResetPureClient_f( client_t *cl )
{
	cl->pureAuthentic = 0;
	cl->gotCP = qfalse;
}

#ifdef USE_HUB_SERVER

/*
==================
SV_SendUserinfoToowHub

Send userinfo string to hub server if we were able to resolve it.
Hub messages are authenticated using a secret key and MD4 digests
(mostly because MD4 was available in ET:XreaL already).
==================
*/
void SV_SendUserinfoToowHub( const char *userinfo )
{
	int length;

	// we use this buffer for both computing the MD4 and then the final
	// message we send; the MD4 hexdigest is 32 chars long, less than
	// MAX_CVAR_VALUE_STRING, so we're good (but still add safety); the
	// contents are "key-or-MD4\nuserinfo\nthelonguserinfodata" which
	// should explain the length calculation
	char buffer[ MAX_CVAR_VALUE_STRING + 1 + 8 + 1 + MAX_INFO_STRING + 4 ];

	// these buffers are for the actual MD4 and its hexdigest, each with
	// a small safety margin
	char md4[ 16 + 2 ];
	char digest[ 32 + 2 ];

	if ( svs.owHubAddress.type != NA_BAD )
	{
		// initialize the buffers (paranoid!)
		memset( buffer, 0, sizeof( buffer ) );
		memset( md4, 0, sizeof( md4 ) );
		memset( digest, 0, sizeof( digest ) );

		// key + payload to authenticate
		length = _snprintf( buffer, sizeof( buffer ) - 2, "%s\nuserinfo\n%s",
		                    sv_owHubKey->string, userinfo );
		assert( length != -1 );
		assert( length <= sizeof( buffer ) - 2 );
		assert( length == strlen( buffer ) );

		// silly (byte*) casts to avoid silly warnings;
		mdfour( ( byte * ) md4, ( byte * ) buffer, length );
		mdfour_hex( ( byte * ) md4, digest );
		assert( strlen( digest ) == 32 );

		// MD4 hexdigest + payload to send
		length = _snprintf( buffer, sizeof( buffer ) - 2, "%s\nuserinfo\n%s",
		                    digest, userinfo );
		assert( length != -1 );
		assert( length <= sizeof( buffer ) - 2 );

		NET_OutOfBandPrint( NS_SERVER, svs.owHubAddress,
		                    "%s", buffer );

		Com_DPrintf( "Sent userinfo to |ALPHA| Hub.\n" );
	}
}

#endif

/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C friendly form.
=================
*/
void SV_UserinfoChanged( client_t *cl )
{
	char *val;
//	char           *ip;
	int  i;
//	int        len;

	// In the ugly [commented out] code below, handicap is supposed to be
	// either missing or a valid int between 1 and 100.
	// It's safe therefore to stick with that policy and just remove it.
	// ET never uses handicap anyways.  Unfortunately it's possible
	// to have a key such as handicap appear more than once in the userinfo.
	// So we remove every instance of it.
	int oldInfoLen = strlen( cl->userinfo );
	int newInfoLen;

	while ( qtrue )
	{
		Info_RemoveKey( cl->userinfo, "handicap" );
		newInfoLen = strlen( cl->userinfo );

		if ( oldInfoLen == newInfoLen ) { break; } // userinfo wasn't modified.

		oldInfoLen = newInfoLen;
	}

	// name for C code
	Q_strncpyz( cl->name, Info_ValueForKey( cl->userinfo, "name" ), sizeof( cl->name ) );

	// rate command

	// if the client is on the same subnet as the server and we aren't running an
	// internet public server, assume they don't need a rate choke
	if ( Sys_IsLANAddress( cl->netchan.remoteAddress ) && com_dedicated->integer != 2 && sv_lanForceRate->integer == 1 )
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

	//donmichelangelo: This code isn't used in ET and may cause an overflow in the userinfo string

	/*
	val = Info_ValueForKey(cl->userinfo, "handicap");
	if(strlen(val))
	{
	        i = atoi(val);
	        if(i <= -100 || i > 100 || strlen(val) > 4)
	        {
	                Info_SetValueForKey(cl->userinfo, "handicap", "0");
	        }
	}
	*/

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
	cl->hasVoip = ( atoi( val ) == 1 ) ? qtrue : qfalse;
#endif

	// TTimo
	// maintain the IP information
	// this is set in SV_DirectConnect (directly on the server, not transmitted), may be lost when client updates it's userinfo
	// the banning code relies on this being consistently present
	// zinx - modified to always keep this consistent, instead of only
	// when "ip" is 0-length, so users can't supply their own IP
	//Com_DPrintf("Maintain IP in userinfo for '%s'\n", cl->name);
	if ( !NET_IsLocalAddress( cl->netchan.remoteAddress ) )
	{
		Info_SetValueForKey( cl->userinfo, "ip", NET_AdrToString( cl->netchan.remoteAddress ) );
	}
	else
	{
		// force the "ip" info key to "localhost" for local clients
		Info_SetValueForKey( cl->userinfo, "ip", "localhost" );
	}

	// TTimo
	// download prefs of the client
	val = Info_ValueForKey( cl->userinfo, "cl_wwwDownload" );
	cl->bDlOK = qfalse;

	if ( strlen( val ) )
	{
		i = atoi( val );

		if ( i != 0 )
		{
			cl->bDlOK = qtrue;
		}
	}
}

/*
==================
SV_UpdateUserinfo_f
==================
*/
static void SV_UpdateUserinfo_f( client_t *cl )
{
	Q_strncpyz( cl->userinfo, Cmd_Argv( 1 ), sizeof( cl->userinfo ) );

	SV_UserinfoChanged( cl );
	// call prog code to allow overrides
	VM_Call( gvm, GAME_CLIENT_USERINFO_CHANGED, cl - svs.clients );
}

#ifdef USE_VOIP
static
void SV_UpdateVoipIgnore( client_t *cl, const char *idstr, qboolean ignore )
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
static void SV_Voip_f( client_t *cl )
{
	const char *cmd = Cmd_Argv( 1 );

	if ( strcmp( cmd, "ignore" ) == 0 )
	{
		SV_UpdateVoipIgnore( cl, Cmd_Argv( 2 ), qtrue );
	}
	else if ( strcmp( cmd, "unignore" ) == 0 )
	{
		SV_UpdateVoipIgnore( cl, Cmd_Argv( 2 ), qfalse );
	}
	else if ( strcmp( cmd, "muteall" ) == 0 )
	{
		cl->muteAllVoip = qtrue;
	}
	else if ( strcmp( cmd, "unmuteall" ) == 0 )
	{
		cl->muteAllVoip = qfalse;
	}
}

#endif

typedef struct
{
	char     *name;
	void ( *func )( client_t *cl );
	qboolean allowedpostmapchange;
} ucmd_t;

static ucmd_t ucmds[] =
{
	{ "userinfo",   SV_UpdateUserinfo_f,  qfalse },
	{ "disconnect", SV_Disconnect_f,      qtrue  },
	{ "cp",         SV_VerifyPaks_f,      qfalse },
	{ "vdr",        SV_ResetPureClient_f, qfalse },
	{ "download",   SV_BeginDownload_f,   qfalse },
	{ "nextdl",     SV_NextDownload_f,    qfalse },
	{ "stopdl",     SV_StopDownload_f,    qfalse },
	{ "donedl",     SV_DoneDownload_f,    qfalse },
#ifdef USE_VOIP
	{ "voip",       SV_Voip_f },
#endif
	{ "wwwdl",      SV_WWWDownload_f,     qfalse },
	{ NULL,         NULL }
};

/*
==================
SV_ExecuteClientCommand

Also called by bot code
==================
*/

// The value below is how many extra characters we reserve for every instance of '$' in a
// ut_radio, say, or similar client command.  Some jump maps have very long $location's.
// On these maps, it may be possible to crash the server if a carefully-crafted
// client command is sent.  The constant below may require further tweaking.  For example,
// a text of "$location" would have a total computed length of 25, because "$location" has
// 9 characters, and we increment that by 16 for the '$'.
#define STRLEN_INCREMENT_PER_DOLLAR_VAR 16

// Don't allow more than this many dollared-strings (e.g. $location) in a client command
// such as ut_radio and say.  Keep this value low for safety, in case some things like
// $location expand to very large strings in some maps.  There is really no reason to have
// more than 6 dollar vars (such as $weapon or $location) in things you tell other people.
#define MAX_DOLLAR_VARS 6

// When a radio text (as in "ut_radio 1 1 text") is sent, weird things start to happen
// when the text gets to be greater than 118 in length.  When the text is really large the
// server will crash.  There is an in-between gray zone above 118, but I don't really want
// to go there.  This is the maximum length of radio text that can be sent, taking into
// account increments due to presence of '$'.
#define MAX_RADIO_STRLEN 118

// Don't allow more than this text length in a command such as say.  I pulled this
// value out of my ass because I don't really know exactly when problems start to happen.
// This value takes into account increments due to the presence of '$'.
#define MAX_SAY_STRLEN 256

void SV_ExecuteClientCommand( client_t *cl, const char *s, qboolean clientOK, qboolean premaprestart )
{
	ucmd_t   *u;
	qboolean bProcessed = qfalse;
	int      argsFromOneMaxlen;
	int      charCount;
	int      dollarCount;
	int      i;
	char     *arg;
	qboolean exploitDetected;

	Com_DPrintf( "EXCL: %s\n", s );
	Cmd_TokenizeString( s );

	// see if it is a server level command
	for ( u = ucmds; u->name; u++ )
	{
		if ( !strcmp( Cmd_Argv( 0 ), u->name ) )
		{
			if ( premaprestart && !u->allowedpostmapchange )
			{
				continue;
			}

			u->func( cl );
			bProcessed = qtrue;
			break;
		}
	}

	if ( clientOK )
	{
		// pass unknown strings to the game
		if ( !u->name && sv.state == SS_GAME )
		{
			argsFromOneMaxlen = -1;

			if ( Q_stricmp( "say", Cmd_Argv( 0 ) ) == 0 ||
			     Q_stricmp( "say_team", Cmd_Argv( 0 ) ) == 0 ||
			     Q_stricmp( "say_fireteam", Cmd_Argv( 0 ) ) == 0 )
			{
				argsFromOneMaxlen = MAX_SAY_STRLEN;
			}
			else if ( Q_stricmp( "tell", Cmd_Argv( 0 ) ) == 0 )
			{
				// A command will look like "tell 12 hi" or "tell foo hi".  The "12"
				// and "foo" in the examples will be counted towards MAX_SAY_STRLEN,
				// plus the space.
				argsFromOneMaxlen = MAX_SAY_STRLEN;
			}

			/*else if (Q_stricmp("ut_radio", Cmd_Argv(0)) == 0) {
			        // We add 4 to this value because in a command such as
			        // "ut_radio 1 1 affirmative", the args at indices 1 and 2 each
			        // have length 1 and there is a space after them.
			        argsFromOneMaxlen = MAX_RADIO_STRLEN + 4;
			}*/
			if ( argsFromOneMaxlen >= 0 )
			{
				exploitDetected = qfalse;
				charCount = 0;
				dollarCount = 0;

				for ( i = Cmd_Argc() - 1; i >= 1; i-- )
				{
					arg = Cmd_Argv( i );

					while ( *arg )
					{
						if ( ++charCount > argsFromOneMaxlen )
						{
							exploitDetected = qtrue;
							break;
						}

						if ( *arg == '$' )
						{
							if ( ++dollarCount > MAX_DOLLAR_VARS )
							{
								exploitDetected = qtrue;
								break;
							}

							charCount += STRLEN_INCREMENT_PER_DOLLAR_VAR;

							if ( charCount > argsFromOneMaxlen )
							{
								exploitDetected = qtrue;
								break;
							}
						}

						arg++;
					}

					if ( exploitDetected ) { break; }

					if ( i != 1 ) // Cmd_ArgsFrom() will add space
					{
						if ( ++charCount > argsFromOneMaxlen )
						{
							exploitDetected = qtrue;
							break;
						}
					}
				}

				if ( exploitDetected )
				{
					Com_Printf( "Buffer overflow exploit radio/say, possible attempt from %s\n",
					            NET_AdrToString( cl->netchan.remoteAddress ) );
					SV_SendServerCommand( cl, "print \"Chat dropped due to message length constraints.\n\"" );
					return;
				}
			}

			else if ( Q_stricmp( "callvote", Cmd_Argv( 0 ) ) == 0 )
			{
				Com_Printf( "Callvote from %s (client #%i, %s): %s\n",
				            cl->name, ( int )( cl - svs.clients ),
				            NET_AdrToString( cl->netchan.remoteAddress ), Cmd_Args() );
			}

			VM_Call( gvm, GAME_CLIENT_COMMAND, cl - svs.clients );
		}
	}
	else if ( !bProcessed )
	{
		Com_DPrintf( "client text ignored for %s: %s\n", cl->name, Cmd_Argv( 0 ) );
	}
}

/*
===============
SV_ClientCommand
===============
*/
static qboolean SV_ClientCommand( client_t *cl, msg_t *msg, qboolean premaprestart )
{
	int        seq;
	const char *s;
	qboolean   clientOk = qtrue;
	qboolean   floodprotect = qtrue;

	seq = MSG_ReadLong( msg );
	s = MSG_ReadString( msg );

	// see if we have already executed it
	if ( cl->lastClientCommand >= seq )
	{
		return qtrue;
	}

	Com_DPrintf( "clientCommand: %s : %i : %s\n", cl->name, seq, s );

	// drop the connection if we have somehow lost commands
	if ( seq > cl->lastClientCommand + 1 )
	{
		Com_Printf( "Client %s lost %i clientCommands\n", cl->name, seq - cl->lastClientCommand + 1 );
		SV_DropClient( cl, "Lost reliable commands" );
		return qfalse;
	}

	// Gordon: AHA! Need to steal this for some other stuff BOOKMARK
	// NERVE - SMF - some server game-only commands we cannot have flood protect
	if ( !Q_strncmp( "team", s, 4 ) || !Q_strncmp( "setspawnpt", s, 10 ) || !Q_strncmp( "score", s, 5 ) || !Q_stricmp( "forcetapout", s ) )
	{
//      Com_DPrintf( "Skipping flood protection for: %s\n", s );
		floodprotect = qfalse;
	}

	// malicious users may try using too many string commands
	// to lag other players.  If we decide that we want to stall
	// the command, we will stop processing the rest of the packet,
	// including the usercmd.  This causes flooders to lag themselves
	// but not other people
	// We don't do this when the client hasn't been active yet since its
	// normal to spam a lot of commands when downloading
	if ( !com_cl_running->integer && cl->state >= CS_ACTIVE && // (SA) this was commented out in Wolf.  Did we do that?
	     sv_floodProtect->integer && svs.time < cl->nextReliableTime && floodprotect )
	{
		// ignore any other text messages from this client but let them keep playing
		// TTimo - moved the ignored verbose to the actual processing in SV_ExecuteClientCommand, only printing if the core doesn't intercept
		clientOk = qfalse;
	}

	// don't allow another command for 800 msec
	if ( floodprotect && svs.time >= cl->nextReliableTime )
	{
		cl->nextReliableTime = svs.time + 800;
	}

	SV_ExecuteClientCommand( cl, s, clientOk, premaprestart );

	cl->lastClientCommand = seq;
	Com_sprintf( cl->lastClientCommandString, sizeof( cl->lastClientCommandString ), "%s", s );

	return qtrue; // continue procesing
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

	VM_Call( gvm, GAME_CLIENT_THINK, cl - svs.clients );
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
static void SV_UserMove( client_t *cl, msg_t *msg, qboolean delta )
{
	int       i, key;
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

	// use the checksum feed in the key
	key = sv.checksumFeed;
	// also use the message acknowledge
	key ^= cl->messageAcknowledge;
	// also use the last acknowledged server command in the key
	key ^= Com_HashKey( cl->reliableCommands[ cl->reliableAcknowledge & ( MAX_RELIABLE_COMMANDS - 1 ) ], 32 );

	memset( &nullcmd, 0, sizeof( nullcmd ) );
	oldcmd = &nullcmd;

	for ( i = 0; i < cmdCount; i++ )
	{
		cmd = &cmds[ i ];
		MSG_ReadDeltaUsercmdKey( msg, key, oldcmd, cmd );
//      MSG_ReadDeltaUsercmd( msg, oldcmd, cmd );
		oldcmd = cmd;
	}

	// save time for ping calculation
	cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = svs.time;

	// TTimo
	// catch the no-cp-yet situation before SV_ClientEnterWorld
	// if CS_ACTIVE, then it's time to trigger a new gamestate emission
	// if not, then we are getting remaining parasite usermove commands, which we should ignore
	if ( sv_pure->integer != 0 && cl->pureAuthentic == 0 && !cl->gotCP )
	{
		if ( cl->state == CS_ACTIVE )
		{
			// we didn't get a cp yet, don't assume anything and just send the gamestate all over again
			Com_DPrintf( "%s: didn't get cp command, resending gamestate\n", cl->name );
			SV_SendClientGameState( cl );
		}

		return;
	}

	// if this is the first usercmd we have received
	// this gamestate, put the client into the world
	if ( cl->state == CS_PRIMED )
	{
		SV_ClientEnterWorld( cl, &cmds[ 0 ] );
		// the moves can be processed normaly
	}

	// a bad cp command was sent, drop the client
	if ( sv_pure->integer != 0 && cl->pureAuthentic == 0 )
	{
		SV_DropClient( cl, "Cannot validate pure client!" );
		return;
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
		//if ( cmds[i].serverTime > svs.time + 3000 ) {
		//  continue;
		//}
		if ( !SV_GameIsSinglePlayer() )
		{
			// We need to allow this in single player, where loadgame's can cause the player to freeze after reloading if we do this check
			// don't execute if this is an old cmd which is already executed
			// these old cmds are included when cl_packetdup > 0
			if ( cmds[ i ].serverTime <= cl->lastUsercmd.serverTime )
			{
				// Q3_MISSIONPACK
//          if ( cmds[i].serverTime > cmds[cmdCount-1].serverTime ) {
				continue; // from just before a map_restart
			}
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

static qboolean SV_ShouldIgnoreVoipSender( const client_t *cl )
{
	if ( !sv_voip->integer )
	{
		return qtrue; // VoIP disabled on this server.
	}
	else if ( !cl->hasVoip ) // client doesn't have VoIP support?!
	{
		return qtrue;
	}

	// !!! FIXME: implement player blacklist.

	return qfalse; // don't ignore.
}

static
void SV_UserVoip( client_t *cl, msg_t *msg )
{
	int                sender, generation, sequence, frames, packetsize;
	uint8_t            recips[( MAX_CLIENTS + 7 ) / 8 ];
	int                flags;
	byte               encoded[ sizeof( cl->voipPacket[ 0 ]->data ) ];
	client_t           *client = NULL;
	voipServerPacket_t *packet = NULL;
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

		packet = Z_Malloc( sizeof( *packet ) );
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
			Com_DPrintf( "%s : ignoring pre map_restart / outdated client message\n", cl->name );
			return;
		}

		// if we can tell that the client has dropped the last
		// gamestate we sent them, resend it
		if ( cl->messageAcknowledge > cl->gamestateMessageNum )
		{
			Com_DPrintf( "%s : dropped gamestate, resending\n", cl->name );
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

			if ( !SV_ClientCommand( cl, msg, qtrue ) )
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

		if ( !SV_ClientCommand( cl, msg, qfalse ) )
		{
			return; // we couldn't execute it because of the flood protection
		}

		if ( cl->state == CS_ZOMBIE )
		{
			return; // disconnect command
		}
	}
	while ( 1 );

	// read the usercmd_t
	if ( c == clc_move )
	{
		SV_UserMove( cl, msg, qtrue );
	}
	else if ( c == clc_moveNoDelta )
	{
		SV_UserMove( cl, msg, qfalse );
	}
	else if ( c == clc_voip )
	{
#ifdef USE_VOIP
		SV_UserVoip( cl, msg );
#endif
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
