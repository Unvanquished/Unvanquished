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

#include "server.h"
#include "common/Assert.h"
#include "CryptoChallenge.h"
#include "framework/Rcon.h"

#include "framework/CommandSystem.h"
#include "framework/CvarSystem.h"
#include "framework/Network.h"

serverStatic_t svs; // persistent server info
server_t       sv; // local server
GameVM         gvm; // game virtual machine

cvar_t         *sv_fps; // time rate for running non-clients
cvar_t         *sv_timeout; // seconds without any message
cvar_t         *sv_zombietime; // seconds to sink messages after disconnect
cvar_t         *sv_privatePassword; // password for the privateClient slots
cvar_t         *sv_allowDownload;
cvar_t         *sv_maxclients;

cvar_t         *sv_privateClients; // number of clients reserved for password
cvar_t         *sv_hostname;
cvar_t         *sv_statsURL;
cvar_t         *sv_master[ MAX_MASTER_SERVERS ]; // master server IP addresses
cvar_t         *sv_reconnectlimit; // minimum seconds between connect messages
cvar_t         *sv_padPackets; // add nop bytes to messages
cvar_t         *sv_killserver; // menu system can set to 1 to shut server down
cvar_t         *sv_mapname;
cvar_t         *sv_serverid;
cvar_t         *sv_maxRate;

cvar_t         *sv_pure;
cvar_t         *sv_floodProtect;
cvar_t         *sv_lanForceRate; // TTimo - dedicated 1 (LAN) server forces local client rates to 99999 (bug #491)

cvar_t         *sv_dl_maxRate;

cvar_t *sv_showAverageBPS; // NERVE - SMF - net debugging

cvar_t *sv_wwwDownload; // server does a www dl redirect
cvar_t *sv_wwwBaseURL; // base URL for redirect

// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you lose the ability to control the download usage
cvar_t *sv_wwwDlDisconnected;
cvar_t *sv_wwwFallbackURL; // URL to send to if an http/ftp fails or is refused client side

//bani
cvar_t *sv_packetdelay;

// fretn
cvar_t *sv_fullmsg;


namespace Cvar {
template<>
std::string GetCvarTypeName<ServerPrivate>()
{
	return "server private";
}

} // namespace Cvar

bool ParseCvarValue(Str::StringRef value, ServerPrivate& result)
{
	int intermediate = 0;
	if ( Str::ParseInt(intermediate, value) &&
		intermediate >= int(ServerPrivate::Public) &&
		intermediate <= int(ServerPrivate::LanOnly) )
	{
		result = ServerPrivate(intermediate);
		return true;
	}
	return false;
}

std::string SerializeCvarValue(ServerPrivate value)
{
	return std::to_string(int(value));
}

Cvar::Cvar<ServerPrivate> isPrivate(
	"server.private",
	"Controls how much the server advertises: "
	"0 - Advertise everything, "
	"1 - Don't advertise but reply to status queries, "
	"2 - Don't reply to status queries but accept connections, "
	"3 - Only accept LAN connections.",
#if BUILD_CLIENT || BUILD_TTY_CLIENT
	Cvar::ROM, ServerPrivate::LanOnly
#elif BUILD_SERVER
	Cvar::NONE, ServerPrivate::Public
#else
	#error
#endif
);

bool SV_Private(ServerPrivate level)
{
	return isPrivate.Get() >= level;
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
======================
SV_AddServerCommand

The given command will be transmitted to the client, and is guaranteed to
not have future snapshot_t executed before it is executed
======================
*/
void SV_AddServerCommand( client_t *client, const char *cmd )
{
	int index, i;

	client->reliableSequence++;

	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	// we check == instead of >= so a broadcast print added by SV_DropClient()
	// doesn't cause a recursive drop client
	if ( client->reliableSequence - client->reliableAcknowledge == MAX_RELIABLE_COMMANDS + 1 )
	{
		Log::Debug("===== pending server commands =====");

		for ( i = client->reliableAcknowledge + 1; i <= client->reliableSequence; i++ )
		{
			Log::Debug( "cmd %5d: %s", i, client->reliableCommands[ i & ( MAX_RELIABLE_COMMANDS - 1 ) ] );
		}

		Log::Debug( "cmd %5d: %s", i, cmd );
		SV_DropClient( client, "Server command overflow" );
		return;
	}

	index = client->reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	Q_strncpyz( client->reliableCommands[ index ], cmd, sizeof( client->reliableCommands[ index ] ) );
}

/*
=================
SV_SendServerCommand

Sends a reliable command string to be interpreted by
the client game module: "cp", "print", "chat", etc
A nullptr client will broadcast to all clients
=================
*/
void QDECL PRINTF_LIKE(2) SV_SendServerCommand( client_t *cl, const char *fmt, ... )
{
	va_list  argptr;
	byte     message[ MAX_MSGLEN ];
	client_t *client;
	int      j;

	va_start( argptr, fmt );
	Q_vsnprintf( ( char * ) message, sizeof( message ), fmt, argptr );
	va_end( argptr );

	// do not forward server command messages that would be too big to clients
	// ( q3infoboom / q3msgboom stuff )
	if ( strlen( ( char * ) message ) > 1022 )
	{
		return;
	}

	if ( cl != nullptr )
	{
		SV_AddServerCommand( cl, ( char * ) message );
		return;
	}

	if ( Com_IsDedicatedServer() )
	{
		if ( !strncmp( ( char * ) message, "print_tr_p ", 11 ) )
		{
			SV_PrintTranslatedText( ( const char * ) message, true, true );
		}
		else if ( !strncmp( ( char * ) message, "print_tr ", 9 ) )
		{
			SV_PrintTranslatedText( ( const char * ) message, true, false );
		}

		// hack to echo broadcast prints to console
		else if ( !strncmp( ( char * ) message, "print ", 6 ) )
		{
			Log::Notice( "Broadcast: %s", Cmd_UnquoteString( ( char * ) message + 6 ) );
		}
	}

	// send the data to all relevent clients
	for ( j = 0, client = svs.clients; j < sv_maxclients->integer; j++, client++ )
	{
		if ( client->state < clientState_t::CS_PRIMED )
		{
			continue;
		}

		// Ridah, don't need to send messages to AI
		if ( SV_IsBot(client) )
		{
			continue;
		}

		// done.
		SV_AddServerCommand( client, ( char * ) message );
	}
}

/*
==============================================================================

MASTER SERVER FUNCTIONS

==============================================================================
*/
struct MasterServer
{
	int index;
	netadr_t ipv4;
	netadr_t ipv6;
	std::string challenge;
	netadrtype_t challenge_address_type;

	MasterServer()
	{
		static int cvar_index = 0;
		ASSERT_LT(cvar_index, MAX_MASTER_SERVERS);
		index = cvar_index++;
		Clear();
	}

	void Clear()
	{
		challenge_address_type = netadrtype_t::NA_BAD;
		ipv4.type = netadrtype_t::NA_BAD;
		ipv6.type = netadrtype_t::NA_BAD;
		challenge.clear();
	}

	bool Empty() const
	{
		return ipv4.type == netadrtype_t::NA_BAD && ipv6.type == netadrtype_t::NA_BAD;
	}

	cvar_t* Cvar() const
	{
		return sv_master[index];
	}

	bool CvarIsSet() const
	{
		return sv_master[index]->string && sv_master[index]->string[0];
	}
};

static std::array<MasterServer, MAX_MASTER_SERVERS> masterServers;

static void SV_ResolveMasterServers()
{
	int netenabled = Cvar_VariableIntegerValue( "net_enabled" );

	for ( MasterServer& master : masterServers )
	{
		if ( !master.CvarIsSet() )
		{
			master.Clear();
			continue;
		}

		// see if we haven't already resolved the name
		// resolving usually causes hitches on win95, so only
		// do it when needed
		if ( master.Cvar()->modified || master.Empty() )
		{
			master.Cvar()->modified = false;

			if ( netenabled & NET_ENABLEV4 )
			{
				Log::Notice( "Resolving %s (IPv4)\n", master.Cvar()->string );
				int res = NET_StringToAdr( master.Cvar()->string, &master.ipv4, netadrtype_t::NA_IP );

				if ( res == 2 )
				{
					// if no port was specified, use the default master port
					master.ipv4.port = BigShort( PORT_MASTER );
				}

				if ( res )
				{
					Log::Notice( "%s resolved to %s\n", master.Cvar()->string, NET_AdrToStringwPort( master.ipv4 ) );
				}
				else
				{
					Log::Notice( "%s has no IPv4 address.\n", master.Cvar()->string );
				}
			}

			if ( netenabled & NET_ENABLEV6 )
			{
				Log::Notice( "Resolving %s (IPv6)\n", master.Cvar()->string );
				int res = NET_StringToAdr( master.Cvar()->string, &master.ipv6, netadrtype_t::NA_IP6 );

				if ( res == 2 )
				{
					// if no port was specified, use the default master port
					master.ipv6.port = BigShort( PORT_MASTER );
				}

				if ( res )
				{
					Log::Notice( "%s resolved to %s\n", master.Cvar()->string, NET_AdrToStringwPort( master.ipv6 ) );
				}
				else
				{
					Log::Notice( "%s has no IPv6 address.\n", master.Cvar()->string );
				}
			}

			if ( master.Empty() )
			{
				// if the address failed to resolve, clear it
				// so we don't take repeated dns hits
				Log::Notice( "Couldn't resolve address: %s\n", master.Cvar()->string );
				Cvar_Set( master.Cvar()->name, "" );
				master.Cvar()->modified = false;
				continue;
			}
		}
	}
}

/*
================
SV_NET_Config

Network connections being reconfigured. May need to redo some lookups.
================
*/
void SV_NET_Config()
{
	for ( MasterServer& master : masterServers )
	{
		master.Clear();
	}
}

/*
================
SV_MasterHeartbeat

Send a message to the masters every few minutes to
let it know we are alive, and log information.
We will also have a heartbeat sent when a server
changes from empty to non-empty, and full to non-full,
but not on every player enter or exit.
================
*/
static const int HEARTBEAT_MSEC = (300 * 1000);
#define HEARTBEAT_GAME "Unvanquished"
#define HEARTBEAT_DEAD "Unvanquished-dead"

void SV_MasterHeartbeat( const char *hbname )
{
	int netenabled = Cvar_VariableIntegerValue( "net_enabled" );

	if ( SV_Private(ServerPrivate::NoAdvertise)
		|| !( netenabled & ( NET_ENABLEV4 | NET_ENABLEV6 ) ) )
	{
		return; // only dedicated servers send heartbeats
	}

	// if not time yet, don't send anything
	if ( svs.time < svs.nextHeartbeatTime )
	{
		return;
	}

	svs.nextHeartbeatTime = svs.time + HEARTBEAT_MSEC;

	SV_ResolveMasterServers();

	// send to group masters
	for ( MasterServer& master : masterServers )
	{
		if ( master.Empty() )
		{
			continue;
		}

		Log::Notice( "Sending heartbeat to %s", master.Cvar()->string );

		// this command should be changed if the server info / status format
		// ever incompatibly changes

		if ( master.ipv4.type != netadrtype_t::NA_BAD )
		{
			Net::OutOfBandPrint( netsrc_t::NS_SERVER, master.ipv4, "heartbeat %s\n", hbname );
		}

		if ( master.ipv6.type != netadrtype_t::NA_BAD )
		{
			Net::OutOfBandPrint( netsrc_t::NS_SERVER, master.ipv6, "heartbeat %s\n", hbname );
		}
	}
}

/*
=================
SV_MasterShutdown

Informs all masters that this server is going down
=================
*/
void SV_MasterShutdown()
{
	// send a heartbeat right now
	svs.nextHeartbeatTime = -9999;
	SV_MasterHeartbeat( HEARTBEAT_DEAD );  // NERVE - SMF - changed to flatline

	// send it again to minimize chance of drops
//  svs.nextHeartbeatTime = -9999;
//  SV_MasterHeartbeat( HEARTBEAT_DEAD );

	// when the master tries to poll the server, it won't respond, so
	// it will be removed from the list
}

/*
=================
SV_MasterGameStat
=================
*/
void SV_MasterGameStat( const char *data )
{
	netadr_t adr;

	if ( SV_Private(ServerPrivate::NoAdvertise) )
	{
		return; // only dedicated servers send stats
	}

	Log::Notice( "Resolving %s", MASTER_SERVER_NAME );

	switch ( NET_StringToAdr( MASTER_SERVER_NAME, &adr, netadrtype_t::NA_UNSPEC ) )
	{
		case 0:
			Log::Warn( "Couldn't resolve master address: %s", MASTER_SERVER_NAME );
			return;

		case 2:
			adr.port = BigShort( PORT_MASTER );

		default:
			break;
	}

	Log::Notice( "%s resolved to %s", MASTER_SERVER_NAME,
	            NET_AdrToStringwPort( adr ) );

	Log::Notice( "Sending gamestat to %s", MASTER_SERVER_NAME );
	Net::OutOfBandPrint( netsrc_t::NS_SERVER, adr, "gamestat %s", data );
}

/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see about the server
and all connected players.  Used for getting detailed information after
the simple info query.
================
*/
void SVC_Status( netadr_t from, const Cmd::Args& args )
{
	if ( SV_Private(ServerPrivate::NoStatus) )
	{
		return;
	}

	InfoMap info_map;
	Cvar::PopulateInfoMap(CVAR_SERVERINFO, info_map);

	if ( args.Argc() > 1 && InfoValidItem(args.Argv(1)) )
	{
		// echo back the parameter to status. so master servers can use it as a challenge
		// to prevent timed spoofed reply packets that add ghost servers
		info_map["challenge"] = args.Argv(1);
	}

	std::string status;
	for ( int i = 0; i < sv_maxclients->integer; i++ )
	{
		client_t* cl = &svs.clients[ i ];

		if ( cl->state >= clientState_t::CS_CONNECTED )
		{
			playerState_t* ps = SV_GameClientNum( i );
			status +=  Str::Format( "%i %i \"%s\"\n", ps->persistant[ PERS_SCORE ], cl->ping, cl->name );
		}
	}

	Net::OutOfBandPrint( netsrc_t::NS_SERVER, from, "statusResponse\n%s\n%s",
		InfoMapToString( info_map ), status );
}

/*
================
SVC_Info

Responds with a short info message that should be enough to determine
if a user is interested in a server to do a full status
================
*/
void SVC_Info( netadr_t from, const Cmd::Args& args )
{
	if ( SV_Private(ServerPrivate::NoStatus) )
	{
		return;
	}

	SV_ResolveMasterServers();

	// don't count privateclients
	int botCount = 0;
	int count = 0;

	for ( int i = sv_privateClients->integer; i < sv_maxclients->integer; i++ )
	{
		if ( svs.clients[ i ].state >= clientState_t::CS_CONNECTED )
		{
			if ( SV_IsBot(&svs.clients[ i ]) )
			{
				++botCount;
			}
			else
			{
				++count;
			}
		}
	}

	InfoMap info_map;

	if ( args.Argc() > 1 && InfoValidItem(args.Argv(1)) )
	{
		std::string  challenge = args.Argv(1);
		// echo back the parameter to status. so master servers can use it as a challenge
		// to prevent timed spoofed reply packets that add ghost servers
		info_map["challenge"] = challenge;

		// If the master server listens on IPv4 and IPv6, we want to send the
		// most recent challenge received from it over the OTHER protocol
		for ( MasterServer& master : masterServers )
		{
			// First, see if the challenge was sent by this master server
			if ( !NET_CompareBaseAdr( from, master.ipv4 ) && !NET_CompareBaseAdr( from, master.ipv6 ) )
			{
				continue;
			}

			// It was - if the saved challenge is for the other protocol, send it and record the current one
			if ( master.challenge_address_type == netadrtype_t::NA_IP ||
				 master.challenge_address_type == netadrtype_t::NA_IP6 )
			{
				if ( master.challenge_address_type != from.type )
				{
					info_map["challenge2"] = master.challenge;
					master.challenge_address_type = from.type;
					master.challenge = challenge;
					break;
				}
			}

			// Otherwise record the current one regardless and check the next server
			master.challenge_address_type = from.type;
			master.challenge = challenge;
		}
	}

	info_map["protocol"] = std::to_string( PROTOCOL_VERSION );
	info_map["hostname"] = sv_hostname->string;
	info_map["serverload"] = std::to_string( svs.serverLoad );
	info_map["mapname"] = sv_mapname->string;
	info_map["clients"] = std::to_string( count );
	info_map["bots"] = std::to_string( botCount );
	info_map["sv_maxclients"] = std::to_string( sv_maxclients->integer - sv_privateClients->integer );
	info_map["pure"] = std::to_string( sv_pure->integer );

	if ( sv_statsURL->string[0] )
	{
		info_map["stats"] = sv_statsURL->string;
	}

	info_map["gamename"] = GAMENAME_STRING;  // Arnout: to be able to filter out Quake servers

	Net::OutOfBandPrint( netsrc_t::NS_SERVER, from, "infoResponse\n%s", InfoMapToString( info_map ) );
}

/*
 * Sends back a simple reply
 * Used to check if the server is online without sending any other info
 */
void SVC_Ping( netadr_t from, const Cmd::Args& )
{
	if ( SV_Private(ServerPrivate::NoStatus) )
	{
		return;
	}

	Net::OutOfBandPrint( netsrc_t::NS_SERVER, from, "ack\n" );
}

/*
=================
SV_CheckDRDoS

DRDoS stands for "Distributed Reflected Denial of Service".
See here: http://www.lemuria.org/security/application-drdos.html

Returns false if we're good.  true return value means we need to block.
If the address isn't NA_IP, it's automatically denied.
=================
*/
bool SV_CheckDRDoS( netadr_t from )
{
	int        i;
	int        globalCount;
	int        specificCount;
	receipt_t  *receipt;
	netadr_t   exactFrom;
	int        oldest;
	int        oldestTime;
	static int lastGlobalLogTime = 0;
	static int lastSpecificLogTime = 0;

	// Usually the network is smart enough to not allow incoming UDP packets
	// with a source address being a spoofed LAN address.  Even if that's not
	// the case, sending packets to other hosts in the LAN is not a big deal.
	// NA_LOOPBACK qualifies as a LAN address.
	if ( Sys_IsLANAddress( from ) ) { return false; }

	exactFrom = from;

	if ( from.type == netadrtype_t::NA_IP )
	{
		from.ip[ 3 ] = 0; // xx.xx.xx.0
	}
	else if ( from.type == netadrtype_t::NA_IP6 )
	{
		memset( from.ip6 + 7, 0, 9 ); // mask to /56
	}
	else
	{
		// So we got a connectionless packet but it's not IPv4, so
		// what is it?  I don't care, it doesn't matter, we'll just block it.
		// This probably won't even happen.
		return true;
	}

	// Count receipts in last 2 seconds.
	globalCount = 0;
	specificCount = 0;
	receipt = &svs.infoReceipts[ 0 ];
	oldest = 0;
	oldestTime = 0x7fffffff;

	for ( i = 0; i < MAX_INFO_RECEIPTS; i++, receipt++ )
	{
		if ( receipt->time + 2000 > svs.time )
		{
			if ( receipt->time )
			{
				// When the server starts, all receipt times are at zero.  Furthermore,
				// svs.time is close to zero.  We check that the receipt time is already
				// set so that during the first two seconds after server starts, queries
				// from the master servers don't get ignored.  As a consequence a potentially
				// unlimited number of getinfo+getstatus responses may be sent during the
				// first frame of a server's life.
				globalCount++;
			}

			if ( NET_CompareBaseAdr( from, receipt->adr ) )
			{
				specificCount++;
			}
		}

		if ( receipt->time < oldestTime )
		{
			oldestTime = receipt->time;
			oldest = i;
		}
	}

	if ( globalCount == MAX_INFO_RECEIPTS ) // All receipts happened in last 2 seconds.
	{
		if ( lastGlobalLogTime + 1000 <= svs.time ) // Limit one log every second.
		{
			Log::Notice( "Detected flood of getinfo/getstatus connectionless packets" );
			lastGlobalLogTime = svs.time;
		}

		return true;
	}

	if ( specificCount >= 3 ) // Already sent 3 to this IP address in last 2 seconds.
	{
		if ( lastSpecificLogTime + 1000 <= svs.time ) // Limit one log every second.
		{
			Log::Notice( "Possible DRDoS attack to address %i.%i.%i.%i, ignoring getinfo/getstatus connectionless packet",
			            exactFrom.ip[ 0 ], exactFrom.ip[ 1 ], exactFrom.ip[ 2 ], exactFrom.ip[ 3 ] );
			lastSpecificLogTime = svs.time;
		}

		return true;
	}

	receipt = &svs.infoReceipts[ oldest ];
	receipt->adr = from;
	receipt->time = svs.time;
	return false;
}

/*
===============
SVC_RemoteCommand

An rcon packet arrived from the network.
Shift down the remaining args
Redirect all printfs
===============
*/

class RconEnvironment: public Cmd::DefaultEnvironment {
public:
	RconEnvironment(netadr_t from)
		: from(from), bufferSize(MAX_MSGLEN - prefix.size() - 1)
	{}

	virtual void Print(Str::StringRef text) OVERRIDE {
		if (text.size() + buffer.size() > bufferSize - 1) {
			Flush();
		}

		buffer += text;
		buffer += '\n';
	}

	void Flush() {
		Net::OutOfBandPrint(netsrc_t::NS_SERVER, from, "%s\n%s", prefix, buffer);
		buffer.clear();
	}

	static void PrintError(netadr_t to, const std::string& message)
	{
		Net::OutOfBandPrint(netsrc_t::NS_SERVER, to, "error\n%s", message);
	}

private:
	netadr_t from;
	std::size_t bufferSize;
	std::string buffer;
	std::string prefix = "print";
};

static int RemoteCommandThrottle()
{
	static int lasttime = 0;
	int time = Com_Milliseconds();
	int delta = time - lasttime;
	lasttime = time;

	return delta;
}

void SVC_RemoteCommand( netadr_t from, const Cmd::Args& args )
{
	int throttle_delta = RemoteCommandThrottle();


	if ( throttle_delta < 180 )
	{
		return;
	}

	Rcon::Message message = Rcon::Message::decode(from, args);

	std::string invalid_reason;

	if ( !message.acceptable(&invalid_reason) )
	{
		// If the rconpassword is bad and one just happned recently, don't spam the log file, just die.
		if ( throttle_delta < 600 )
		{
			return;
		}

		Log::Notice( "Bad rcon from %s:\n%s\n%s\n",
			NET_AdrToString( from ),
			invalid_reason.c_str(),
			args.ConcatArgs(2).c_str() );

		if ( !SV_Private(ServerPrivate::NoStatus) )
		{
			RconEnvironment::PrintError( from, invalid_reason );
		}
	}
	else
	{
		Log::Notice( "Rcon from %s:\n%s\n", NET_AdrToString( from ), message.command().c_str() );

		// start redirecting all print outputs to the packet
		auto env = RconEnvironment(from);
		Cmd::ExecuteCommand(message.command(), true, &env);
		Cmd::ExecuteCommandBuffer();
		env.Flush();
	}

}

static void SVC_RconInfo( netadr_t from, const Cmd::Args& )
{
	if ( SV_Private(ServerPrivate::NoStatus) )
	{
		return;
	}

	int duration_seconds = std::chrono::duration_cast<std::chrono::seconds>(Challenge::Timeout()).count();
	std::string rcon_info_string = InfoMapToString({
		{"secure",     std::to_string(Rcon::cvar_server_secure.Get())},
		{"encryption", "AES256"},
		{"key",        "SHA256"},
		{"challenge",  std::to_string(Rcon::cvar_server_secure.Get() >= 2)},
		{"timeout",    std::to_string(duration_seconds)},
	});
	Net::OutOfBandPrint( netsrc_t::NS_SERVER, from, "rconInfoResponse\n%s\n", rcon_info_string );
}


/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket( netadr_t from, msg_t *msg )
{
	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );  // skip the -1 marker

	if ( !Q_strncmp( "connect", ( char * ) &msg->data[ 4 ], 7 ) )
	{
		Huff_Decompress( msg, 12 );
	}

	Cmd::Args args(MSG_ReadStringLine( msg ));

	if ( args.Argc() <= 0 )
	{
		return;
	}

	Log::Debug( "SV packet %s : %s", NET_AdrToString( from ), args.Argv(0).c_str() );

	if ( args.Argv(0) == "getstatus" )
	{
		if ( SV_CheckDRDoS( from ) ) { return; }

		SVC_Status( from, args );
	}
	else if ( args.Argv(0) == "getinfo" )
	{
		if ( SV_CheckDRDoS( from ) ) { return; }

		SVC_Info( from, args );
	}
	else if ( args.Argv(0) == "getchallenge" )
	{
		SV_GetChallenge( from );
	}
	else if ( args.Argv(0) == "connect" )
	{
		SV_DirectConnect( from, args );
	}
	else if ( args.Argv(0) == "rcon" || args.Argv(0) == "srcon" )
	{
		SVC_RemoteCommand( from, args );
	}
	else if ( args.Argv(0) == "rconinfo" )
	{
		SVC_RconInfo( from, args );
	}
	else if ( args.Argv(0) == "disconnect" )
	{
		// if a client starts up a local server, we may see some spurious
		// server disconnect messages when their new server sees our final
		// sequenced messages to the old client
	}
	else if ( args.Argv(0) == "ping" )
	{
		SVC_Ping( from, args );
	}
	else
	{
		Log::Debug( "bad connectionless packet from %s: %s", NET_AdrToString( from ), args.ConcatArgs(0).c_str() );
	}
}

//============================================================================

/*
=================
SV_ReadPackets
=================
*/
void SV_PacketEvent( netadr_t from, msg_t *msg )
{
	int      i;
	client_t *cl;
	int      qport;

	// check for connectionless packet (0xffffffff) first
	if ( msg->cursize >= 4 && * ( int * ) msg->data == -1 )
	{
		SV_ConnectionlessPacket( from, msg );
		return;
	}

	// read the qport out of the message so we can fix up
	// stupid address translating routers
	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );  // sequence number
	qport = MSG_ReadShort( msg ) & 0xffff;

	// find which client the message is from
	for ( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if ( cl->state == clientState_t::CS_FREE )
		{
			continue;
		}

		if ( !NET_CompareBaseAdr( from, cl->netchan.remoteAddress ) )
		{
			continue;
		}

		// it is possible to have multiple clients from a single IP
		// address, so they are differentiated by the qport variable
		if ( cl->netchan.qport != qport )
		{
			continue;
		}

		// the IP port can't be used to differentiate them, because
		// some address translating routers periodically change UDP
		// port assignments
		if ( cl->netchan.remoteAddress.port != from.port )
		{
			Log::Notice( "SV_PacketEvent: fixing up a translated port" );
			cl->netchan.remoteAddress.port = from.port;
		}

		// make sure it is a valid, in sequence packet
		if ( Netchan_Process( &cl->netchan, msg ) )
		{
			// zombie clients still need to do the Netchan_Process
			// to make sure they don't need to retransmit the final
			// reliable message, but they don't do any other processing
			if ( cl->state != clientState_t::CS_ZOMBIE )
			{
				cl->lastPacketTime = svs.time; // don't timeout
				SV_ExecuteClientMessage( cl, msg );
			}
		}

		return;
	}

	// if we received a sequenced packet from an address we don't recognize,
	// send an out of band disconnect packet to it
	Net::OutOfBandPrint( netsrc_t::NS_SERVER, from, "disconnect" );
}

/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void SV_CalcPings()
{
	int           i, j;
	client_t      *cl;
	int           total, count;
	int           delta;
	playerState_t *ps;

	for ( i = 0; i < sv_maxclients->integer; i++ )
	{
		cl = &svs.clients[ i ];

		if ( cl->state != clientState_t::CS_ACTIVE )
		{
			cl->ping = 999;
			continue;
		}

		if ( !cl->gentity )
		{
			cl->ping = 999;
			continue;
		}

		if ( SV_IsBot(cl) )
		{
			cl->ping = 0;
			continue;
		}

		total = 0;
		count = 0;

		for ( j = 0; j < PACKET_BACKUP; j++ )
		{
			if ( cl->frames[ j ].messageAcked <= 0 )
			{
				continue;
			}

			delta = cl->frames[ j ].messageAcked - cl->frames[ j ].messageSent;
			count++;
			total += delta;
		}

		if ( !count )
		{
			cl->ping = 999;
		}
		else
		{
			cl->ping = total / count;

			if ( cl->ping > 999 )
			{
				cl->ping = 999;
			}
		}

		// let the game module know about the ping
		ps = SV_GameClientNum( i );
		ps->ping = cl->ping;
	}
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->integer
seconds, drop the conneciton.  Server time is used instead of
realtime to avoid dropping the local client while debugging.

When a client is dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts()
{
	int      i;
	client_t *cl;
	int      droppoint;
	int      zombiepoint;

	droppoint = svs.time - 1000 * sv_timeout->integer;
	zombiepoint = svs.time - 1000 * sv_zombietime->integer;

	for ( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		// message times may be wrong across a changelevel
		if ( cl->lastPacketTime > svs.time )
		{
			cl->lastPacketTime = svs.time;
		}

		if ( cl->state == clientState_t::CS_ZOMBIE && cl->lastPacketTime < zombiepoint )
		{
			// using the client id cause the cl->name is empty at this point
			Log::Debug( "Going from CS_ZOMBIE to CS_FREE for client %d", i );
			cl->state = clientState_t::CS_FREE; // can now be reused

			continue;
		}

		if ( cl->state >= clientState_t::CS_CONNECTED && cl->lastPacketTime < droppoint )
		{
			// wait several frames so a debugger session doesn't
			// cause a timeout
			if ( ++cl->timeoutCount > 5 )
			{
				SV_DropClient( cl, "timed out" );
				cl->state = clientState_t::CS_FREE; // don't bother with zombie state
			}
		}
		else
		{
			cl->timeoutCount = 0;
		}
	}
}

/*
==================
SV_CheckPaused
==================
*/
bool SV_CheckPaused()
{
	int      count;
	client_t *cl;
	int      i;

	if ( !cl_paused->integer )
	{
		return false;
	}

	// only pause if there is just a single client connected
	count = 0;

	for ( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if ( cl->state >= clientState_t::CS_CONNECTED && cl->netchan.remoteAddress.type != netadrtype_t::NA_BOT )
		{
			count++;
		}
	}

	if ( count > 1 )
	{
		// don't pause
		if ( sv_paused->integer )
		{
			Cvar_Set( "sv_paused", "0" );
		}

		return false;
	}

	if ( !sv_paused->integer )
	{
		Cvar_Set( "sv_paused", "1" );
	}

	return true;
}

/*
==================
SV_FrameMsec
Return time in millseconds until processing of the next server frame.
==================
*/
int SV_FrameMsec()
{
	if( sv_fps )
	{
		int frameMsec;

		frameMsec = 1000.0f / sv_fps->value;

		if( frameMsec < sv.timeResidual )
		{
			return 0;
		}
		else
		{
			return frameMsec - sv.timeResidual;
		}
	}
	else
	{
		return 1;
	}
}


/*
==================
SV_Frame

Player movement occurs as a result of packet events, which
happen before SV_Frame is called
==================
*/
void SV_Frame( int msec )
{
	int        frameMsec;
	int        startTime;
	char       mapname[ MAX_QPATH ];
	int        frameStartTime = 0, frameEndTime;
	static int start, end;

	start = Sys_Milliseconds();
	svs.stats.idle += ( double )( start - end ) / 1000;

	// the menu kills the server with this cvar
	if ( sv_killserver->integer )
	{
		SV_Shutdown( "Server was killed." );
		Cvar_Set( "sv_killserver", "0" );
		return;
	}

	if ( !com_sv_running->integer )
	{
		return;
	}

	// allow pause if only the local client is connected
	if ( SV_CheckPaused() )
	{
		return;
	}

	frameStartTime = Sys_Milliseconds();

	// if it isn't time for the next frame, do nothing
	if ( sv_fps->integer < 1 )
	{
		Cvar_Set( "sv_fps", "10" );
	}

	frameMsec = 1000 / sv_fps->integer;

	sv.timeResidual += msec;

	if ( Com_IsDedicatedServer() && sv.timeResidual < frameMsec )
	{
		// NET_Sleep will give the OS time slices until either get a packet
		// or time enough for a server frame has gone by
		NET_Sleep( frameMsec - sv.timeResidual );
		return;
	}

	// if time is about to hit the 32nd bit, kick all clients
	// and clear sv.time, rather
	// than checking for negative time wraparound everywhere.
	// 2giga-milliseconds = 23 days, so it won't be too often
	if ( svs.time > 0x70000000 )
	{
		Q_strncpyz( mapname, sv_mapname->string, MAX_QPATH );
		SV_Shutdown( "Restarting server due to time wrapping" );
		// TTimo
		// show_bug.cgi?id=388
		// there won't be a map_restart if you have shut down the server
		// since it doesn't restart a non-running server
		// instead, re-run the current map
		Cmd::BufferCommandText(va("map %s", mapname));
		return;
	}

	// this can happen considerably earlier when lots of clients play and the map doesn't change
	if ( svs.nextSnapshotEntities >= 0x7FFFFFFE - svs.numSnapshotEntities )
	{
		Q_strncpyz( mapname, sv_mapname->string, MAX_QPATH );
		SV_Shutdown( "Restarting server due to numSnapshotEntities wrapping" );
		// TTimo see above
		Cmd::BufferCommandText(va("map %s", mapname));
		return;
	}

	if ( sv.restartTime && sv.time >= sv.restartTime )
	{
		sv.restartTime = 0;
		Cmd::BufferCommandText("map_restart 0");
		return;
	}

	// update infostrings if anything has been changed
	if ( cvar_modifiedFlags & CVAR_SERVERINFO )
	{
		SV_SetConfigstring( CS_SERVERINFO, Cvar_InfoString( CVAR_SERVERINFO, false ) );
		cvar_modifiedFlags &= ~CVAR_SERVERINFO;
	}

	if ( cvar_modifiedFlags & CVAR_SYSTEMINFO )
	{
		SV_SetConfigstring( CS_SYSTEMINFO, Cvar_InfoString( CVAR_SYSTEMINFO, true ) );
		cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	}

	if ( com_speeds->integer )
	{
		startTime = Sys_Milliseconds();
	}
	else
	{
		startTime = 0; // quite a compiler warning
	}

	// update ping based on the all received frames
	SV_CalcPings();

	// run the game simulation in chunks
	while ( sv.timeResidual >= frameMsec )
	{
		sv.timeResidual -= frameMsec;
		svs.time += frameMsec;
		sv.time += frameMsec;

		// let everything in the world think and move
		gvm.GameRunFrame( sv.time );
	}

	if ( com_speeds->integer )
	{
		time_game = Sys_Milliseconds() - startTime;
	}

	// check timeouts
	SV_CheckTimeouts();

	// send messages back to the clients
	SV_SendClientMessages();

	// send a heartbeat to the master if needed
	SV_MasterHeartbeat( HEARTBEAT_GAME );

	frameEndTime = Sys_Milliseconds();

	svs.totalFrameTime += ( frameEndTime - frameStartTime );
	svs.currentFrameIndex++;

	//if( svs.currentFrameIndex % 50 == 0 )
	//  Log::Notice( "currentFrameIndex: %i\n", svs.currentFrameIndex );

	if ( svs.currentFrameIndex == SERVER_PERFORMANCECOUNTER_FRAMES )
	{
		int averageFrameTime;

		averageFrameTime = svs.totalFrameTime / SERVER_PERFORMANCECOUNTER_FRAMES;

		svs.sampleTimes[ svs.currentSampleIndex % SERVER_PERFORMANCECOUNTER_SAMPLES ] = averageFrameTime;
		svs.currentSampleIndex++;

		if ( svs.currentSampleIndex > SERVER_PERFORMANCECOUNTER_SAMPLES )
		{
			int totalTime, i;

			totalTime = 0;

			for ( i = 0; i < SERVER_PERFORMANCECOUNTER_SAMPLES; i++ )
			{
				totalTime += svs.sampleTimes[ i ];
			}

			if ( !totalTime )
			{
				totalTime = 1;
			}

			averageFrameTime = totalTime / SERVER_PERFORMANCECOUNTER_SAMPLES;

			svs.serverLoad = ( averageFrameTime / ( float ) frameMsec ) * 100;
		}

		//Log::Notice( "serverload: %i (%i/%i)\n", svs.serverLoad, averageFrameTime, frameMsec );

		svs.totalFrameTime = 0;
		svs.currentFrameIndex = 0;
	}

	// collect timing statistics
	end = Sys_Milliseconds();
	svs.stats.active += ( ( double )( end - start ) ) / 1000;

	if ( ++svs.stats.count == STATFRAMES )
	{
		svs.stats.latched_active = svs.stats.active;
		svs.stats.latched_idle = svs.stats.idle;
		svs.stats.latched_packets = svs.stats.packets;
		svs.stats.active = 0;
		svs.stats.idle = 0;
		svs.stats.packets = 0;
		svs.stats.count = 0;
	}
}

/*
========================
 SV_PrintTranslatedText
========================
 */
#define TRANSLATE_FUNC        Trans_GettextGame
#define PLURAL_TRANSLATE_FUNC Trans_GettextGamePlural
#include "qcommon/print_translated.h"

void SV_PrintTranslatedText( const char *text, bool broadcast, bool plural )
{
	Cmd_SaveCmdContext();
	Cmd_TokenizeString( text );
	Log::Notice( "%s%s", broadcast ? "Broadcast: " : "", TranslateText_Internal( plural, 1 ) );
	Cmd_RestoreCmdContext();
}


//============================================================================
